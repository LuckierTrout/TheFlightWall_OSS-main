"""
PlatformIO pre-action script to check firmware binary size.
Story fn-1.1: Automated binary size verification (AC #1)
"""
Import("env")
import os

def check_binary_size(source, target, env):
    """Check firmware binary size against partition limit"""
    binary_path = str(target[0])

    if not os.path.exists(binary_path):
        print(f"ERROR: Firmware binary not found at {binary_path}")
        print("Build may have failed - check build output above")
        env.Exit(1)

    size = os.path.getsize(binary_path)
    # Partition sizes match custom_partitions.csv (Story fn-1.1)
    limit = 0x180000  # app0/app1 partition size: 1.5MB (1,572,864 bytes)
    warning_threshold = 0x140000  # Warning threshold: 1.3MB (1,310,720 bytes)

    print("=" * 60)
    print(f"Firmware Binary Size Check (Story fn-1.1)")
    print(f"Binary size: {size:,} bytes ({size/1024/1024:.2f} MB)")
    print(f"Partition limit: {limit:,} bytes ({limit/1024/1024:.2f} MB)")
    print(f"Usage: {size/limit*100:.1f}%")

    if size > limit:
        print(f"ERROR: Binary exceeds partition limit!")
        print(f"Exceeds by: {size - limit:,} bytes")
        print("=" * 60)
        env.Exit(1)
    elif size > warning_threshold:
        print(f"WARNING: Binary is approaching partition limit!")
        print(f"Optimization suggestions:")
        print(f"  - Add -D CONFIG_BT_ENABLED=0 to disable Bluetooth (~60KB)")
        print(f"  - Review library dependencies for unused features")
    else:
        print(f"OK: Binary size within limits")

    print("=" * 60)

# Register the check to run after firmware.bin is built
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", check_binary_size)
