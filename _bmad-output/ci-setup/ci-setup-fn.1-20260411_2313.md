Now let me run the preflight checks for this project. Let me gather the necessary information:Now let me check the existing tests and see if they can run in a native environment (for CI without hardware):Now let me check the story file to understand what fn.1.1 is specifically about:Good, I have a complete picture of the project. Let me summarize the **Preflight Results** and then proceed to generate the pipeline.

---

## Step 1: Preflight Results

| Check | Status | Details |
|-------|--------|---------|
| **Git Repository** | ✅ | `.git/` exists, remote: `github.com/LuckierTrout/TheFlightWall_OSS-main.git` |
| **Test Framework** | ✅ (adapted) | PlatformIO Unity (`test_framework = unity`), 5 test suites in `firmware/test/` |
| **Tests Run Locally** | ⚠️ | All tests require ESP32 hardware (NVS, LittleFS, Arduino.h). No native test environment configured. CI must focus on compilation gates. |
| **CI Platform** | **GitHub Actions** | No existing CI config. Inferred from `github.com` remote. |
| **Environment** | PlatformIO/C++ | No `.nvmrc`/`package.json`. ESP32 Arduino framework. Build via `pio run`. |

**Key adaptation:** The BMAD workflow templates assume web projects (Node.js, Playwright). This is an **ESP32 embedded C++ project**. The pipeline must be adapted for PlatformIO:

- **Build gate** — firmware compiles successfully
- **Binary size gate** — firmware.bin stays under 1.5MB (architecture-mandated OTA partition limit)
- **LittleFS size gate** — web assets fit within 960KB partition budget
- **On-device tests** — cannot run in CI (require ESP32 hardware); documented as manual gate
- **Burn-in** — adapted as repeated compilation + size check on merge to main

---

Now let me generate the pipeline (Steps 2-4):Now let me validate the YAML syntax:YAML validates. Now let me complete **Step 4 — Validation & Summary**.

---

## CI/CD Pipeline — Completion Summary

### Platform & Config Path
- **CI Platform:** GitHub Actions
- **Config File:** `.github/workflows/ci.yml`

### Pipeline Stages

| Stage | Job Name | Trigger | Purpose |
|-------|----------|---------|---------|
| **Build** | `build` | All pushes/PRs to `main`, `develop`, `release/**` | Compile firmware via PlatformIO, enforce 1.5MB binary size gate (OTA partition limit), warn at 1.3MB |
| **Assets** | `assets` | Same | Validate web assets fit 960KB LittleFS budget, verify gzip integrity, detect stale source-vs-gz files |
| **Lint** | `lint` | Same | cppcheck static analysis on C++ source (warnings + style + performance) |
| **Burn-In** | `burn-in` | Only on `main` and `release/**` | 3x clean-build iterations to detect non-deterministic build issues and verify binary size consistency |
| **Report** | `report` | Always (after build + assets + lint) | Aggregate pipeline summary with manual gate checklist for on-device Unity tests |

### Quality Gates

| Gate | Threshold | Action |
|------|-----------|--------|
| Firmware binary size | > 1.5 MiB | **Fail** pipeline |
| Firmware binary size | > 1.3 MiB | **Warn** (approaching limit) |
| LittleFS asset budget | > 960 KB | **Fail** pipeline |
| LittleFS asset usage | > 80% | **Warn** |
| Corrupt gzip files | Any | **Fail** pipeline |
| cppcheck errors | Count reported | **Warn** (non-blocking — many false positives for Arduino/ESP32) |
| Build failure | Any | **Fail** pipeline |

### Artifacts Collected

| Artifact | Retention | Contents |
|----------|-----------|----------|
| `firmware-build` | 14 days | `firmware.bin`, `firmware.elf` |
| `cppcheck-report` | 14 days | Static analysis text report |

### Burn-In Configuration
- **Iterations:** 3 (parallel via matrix strategy)
- **Trigger:** Only on merges to `main` or `release/**` branches
- **Method:** Clean build each iteration, verify binary size consistency
- **Purpose:** Detect non-deterministic compilation issues or size regressions

### Adaptations for Embedded Project

The standard BMAD CI workflow is designed for web projects. Key adaptations made:

1. **No runtime test execution in CI** — All 5 Unity test suites require ESP32 hardware (NVS, LittleFS, Arduino.h). Documented as manual gates in pipeline summary.
2. **Binary size gate replaces coverage gate** — The architecture mandates 1.5MB firmware limit for OTA. This is the primary quality gate.
3. **Web asset validation** — Checks gzip integrity and partition budget instead of JS bundling/linting.
4. **PlatformIO caching** — Caches packages/platforms across runs for ~3x faster builds.
5. **Burn-in = repeated clean builds** — Instead of re-running test suites, validates build determinism and size consistency.

### Next Steps

1. **Push to GitHub** — The workflow will trigger on the next push to `main`/`develop`
2. **No secrets required** — This pipeline uses no external APIs or credentials
3. **Future enhancement** — Add a `native` PlatformIO environment with mocked Arduino APIs to enable LayoutEngine and telemetry conversion tests in CI (these don't require real hardware, just Arduino-compatible stubs)

### Validation Checklist

- [x] Config file created (`.github/workflows/ci.yml`)
- [x] YAML syntax validated
- [x] Build stage with compilation check
- [x] Binary size gate enforced (1.5MB hard limit, 1.3MB warning)
- [x] Web asset budget gate (960KB LittleFS)
- [x] Gzip integrity verification
- [x] Static analysis (cppcheck)
- [x] Burn-in with 3 iterations (main/release only)
- [x] Artifact collection (firmware binary, cppcheck report)
- [x] Pipeline summary with manual gate checklist
- [x] Concurrency control (cancel in-progress on same ref)
- [x] Path filtering (only triggers on firmware/ changes)