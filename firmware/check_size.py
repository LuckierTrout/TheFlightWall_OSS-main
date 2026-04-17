"""
PlatformIO post-action script to check firmware binary size.

Story fn-1.1: Automated binary size verification.
Story LE-1.9: Added 92% hard cap + main-branch delta check + SKIP_DELTA_CHECK.

Gates (descending severity):
  - 100% absolute cap  (1,572,864 bytes) — device will not boot if exceeded
  - 92%  hard cap      (1,447,034 bytes) — LE-1.9 budget gate [int(0x180000*0.92)]
  - 83%  warning       (1,305,477 bytes) — print-only heads-up
  - Delta vs. baseline (184,320 bytes / 180 KB) — fails if a single merge
                                                  consumes too much headroom
"""
Import("env")
import os

# Partition + gate constants
PARTITION_SIZE = 0x180000                          # 1,572,864 bytes — app0/app1 OTA partition
CAP_100_PCT    = PARTITION_SIZE                    # 1,572,864 bytes — absolute brick limit
CAP_92_PCT     = int(PARTITION_SIZE * 0.92)        # 1,447,034 bytes — LE-1.9 hard cap
WARN_83_PCT    = int(PARTITION_SIZE * 0.83)        # 1,305,477 bytes — warning threshold (83%)

# Delta cap: a single merge must not grow the binary by more than 180 KB.
DELTA_CAP_BYTES = 180 * 1024                       # 184,320 bytes


def get_main_baseline_size(project_dir):
    """
    Returns the recorded binary size at the last main-branch merge, or None
    if the baseline artifact is not available.

    Strategy: a plain-text file at `firmware/.firmware_baseline_size`
    containing a single integer. This file is maintained manually or by
    a CI release step and committed alongside the source.

    `project_dir` is the PlatformIO project directory (i.e. firmware/),
    passed in explicitly because SCons' Python action context does not
    define __file__.
    """
    baseline_path = os.path.join(project_dir, ".firmware_baseline_size")
    if not os.path.exists(baseline_path):
        return None
    try:
        with open(baseline_path, "r") as f:
            return int(f.read().strip())
    except (ValueError, OSError) as e:
        print(f"WARNING: .firmware_baseline_size is unreadable or corrupt ({e}). Delta check skipped.")
        return None


def check_binary_size(source, target, env):
    """Check firmware binary size against partition + delta limits."""
    binary_path = str(target[0])

    if not os.path.exists(binary_path):
        print(f"ERROR: Firmware binary not found at {binary_path}")
        print("Build may have failed - check build output above")
        env.Exit(1)

    size = os.path.getsize(binary_path)

    print("=" * 60)
    print(f"Firmware Binary Size Check (LE-1.9 budget gate)")
    print(f"Binary size: {size:,} bytes ({size/1024/1024:.2f} MB)")
    print(f"Partition:   {PARTITION_SIZE:,} bytes ({PARTITION_SIZE/1024/1024:.2f} MB)")
    print(f"Usage:       {size/PARTITION_SIZE*100:.1f}%")
    print(f"Caps:        92% = {CAP_92_PCT:,}  |  100% = {CAP_100_PCT:,}")

    # ---------------------------------------------------------------
    # Absolute caps (descending severity)
    # ---------------------------------------------------------------
    if size > CAP_100_PCT:
        print(f"FATAL: Binary {size:,} bytes exceeds partition ({CAP_100_PCT:,}). Device will not boot.")
        print(f"       Overage: {size - CAP_100_PCT:,} bytes")
        print("=" * 60)
        env.Exit(1)
    elif size > CAP_92_PCT:
        print(f"ERROR: Binary {size:,} bytes exceeds 92% cap ({CAP_92_PCT:,}). Build rejected.")
        print(f"       Usage: {size/PARTITION_SIZE*100:.1f}%  Overage: {size - CAP_92_PCT:,} bytes")
        print("=" * 60)
        env.Exit(1)
    elif size > WARN_83_PCT:
        print(f"WARNING: Binary {size:,} bytes is above 83% warning threshold ({WARN_83_PCT:,}).")
        print(f"         Usage: {size/PARTITION_SIZE*100:.1f}%")
        print(f"         Optimization suggestions:")
        print(f"           - Add -D CONFIG_BT_ENABLED=0 to disable Bluetooth (~60KB)")
        print(f"           - Review library dependencies for unused features")
    else:
        print(f"OK: Binary size within warning threshold.")

    # ---------------------------------------------------------------
    # Delta check vs. main-branch baseline (LE-1.9)
    # ---------------------------------------------------------------
    baseline = get_main_baseline_size(env.subst("$PROJECT_DIR"))
    if baseline is not None:
        delta = size - baseline
        if delta > DELTA_CAP_BYTES:
            print(f"ERROR: Binary grew {delta:+,} bytes vs. main baseline ({baseline:,} bytes).")
            print(f"       Delta cap: {DELTA_CAP_BYTES:,} bytes (180 KB). Exceeded by {delta - DELTA_CAP_BYTES:,} bytes.")
            print("=" * 60)
            env.Exit(1)
        else:
            print(f"Delta vs. main baseline: {delta:+,} bytes (cap: {DELTA_CAP_BYTES:,} / 180 KB)")
    else:
        skip_env = os.environ.get("SKIP_DELTA_CHECK", "")
        if skip_env.lower() not in ("1", "true", "yes"):
            print(f"INFO: No main baseline file found (firmware/.firmware_baseline_size). Delta check skipped.")
            print(f"      To suppress this message, set SKIP_DELTA_CHECK=1.")

    print("=" * 60)


# Register the check to run after firmware.bin is built
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", check_binary_size)
