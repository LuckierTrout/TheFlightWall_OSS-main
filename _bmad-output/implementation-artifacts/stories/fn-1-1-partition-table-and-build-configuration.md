# Story fn-1.1: Partition Table & Build Configuration

Status: ready-for-dev

## Story

As a **developer**,
I want **a dual-OTA partition table and firmware version build flag**,
So that **the device supports over-the-air updates with a known firmware identity**.

## Acceptance Criteria

1. **Given** the current firmware binary **When** built via `pio run` **Then** the binary size is measured and logged to stdout **And** if the binary exceeds 1.3MB, a warning is logged to stdout and noted in the 'Completion Notes List' section of this story with optimization recommendations (disable Bluetooth via `-D CONFIG_BT_ENABLED=0`, strip unused library features) before proceeding

2. **Given** `firmware/custom_partitions.csv` updated with dual-OTA layout: nvs 0x5000/20KB (offset 0x9000), otadata 0x2000/8KB (offset 0xE000), app0 0x180000/1.5MB (offset 0x10000), app1 0x180000/1.5MB (offset 0x190000), spiffs 0xF0000/960KB (offset 0x310000) **When** `platformio.ini` references the custom partition table and includes `-D FW_VERSION=\"1.0.0\"` in build_flags **Then** `pio run` builds successfully with no errors **And** the compiled binary is under 1.5MB

3. **Given** the new partition layout is flashed via USB **When** the device boots **Then** LittleFS mounts successfully on the 960KB spiffs partition **And** the following existing functionality is verified to work correctly: device boots successfully; web dashboard accessible at device IP; flight data fetches and displays on LED matrix; logo management functions (upload, list, delete); calibration tools function **And** `pio run -t uploadfs` uploads web assets and logos to LittleFS with at least 500KB free remaining

4. **Given** `FW_VERSION` is defined as a build flag **When** any source file references `FW_VERSION` **Then** the version string is available at compile time **And** the version string accurately reflects the value defined in the build flag when accessed at runtime

## Tasks / Subtasks

- [ ] **Task 1: Measure current binary size baseline** (AC: #1)
  - [ ] Run `pio run` and record binary size from build output
  - [ ] Document current size as baseline for comparison
  - [ ] If >1.3MB, document optimization recommendations

- [ ] **Task 2: Update custom_partitions.csv with dual-OTA layout** (AC: #2)
  - [ ] Modify `firmware/custom_partitions.csv` with exact offsets and sizes:
    - nvs: 0x9000, 20KB (0x5000)
    - otadata: 0xE000, 8KB (0x2000)
    - app0: 0x10000, 1.5MB (0x180000)
    - app1: 0x190000, 1.5MB (0x180000)
    - spiffs: 0x310000, 960KB (0xF0000)
  - [ ] Verify total does not exceed 4MB flash

- [ ] **Task 3: Add FW_VERSION build flag to platformio.ini** (AC: #2, #4)
  - [ ] Add `-D FW_VERSION=\"1.0.0\"` to build_flags section
  - [ ] Ensure proper escaping for string macro
  - [ ] Add `-D CONFIG_BT_ENABLED=0` if binary is large (optimization)

- [ ] **Task 4: Build and verify firmware compiles** (AC: #2)
  - [ ] Run `pio run` and verify no errors
  - [ ] Confirm binary size under 1.5MB
  - [ ] Document final binary size

- [ ] **Task 5: Flash and test on hardware** (AC: #3)
  - [ ] Flash via USB: `pio run -t upload`
  - [ ] Verify device boots successfully
  - [ ] Verify LittleFS mounts on 960KB partition
  - [ ] Test all existing functionality:
    - Flight data pipeline fetches and displays
    - Web dashboard accessible at device IP
    - Logo management works
    - Calibration tools function

- [ ] **Task 6: Upload filesystem and verify space** (AC: #3)
  - [ ] Run `pio run -t uploadfs`
  - [ ] Verify web assets upload successfully
  - [ ] Verify at least 500KB free remaining in LittleFS
  - [ ] Test served assets work correctly in browser

- [ ] **Task 7: Verify FW_VERSION accessible in code** (AC: #4)
  - [ ] Add test reference to FW_VERSION in code (e.g., log on boot)
  - [ ] Verify version string compiles and logs correctly

## Dev Notes

### Critical Architecture Constraints

**Partition Layout Math (MUST be exact):**
```
Total Flash: 4MB (0x400000)
Bootloader:  0x1000 - 0x8FFF (reserved, not in CSV)
Partitions start at 0x9000

| Name    | Type | SubType | Offset   | Size     | End      |
|---------|------|---------|----------|----------|----------|
| nvs     | data | nvs     | 0x9000   | 0x5000   | 0xDFFF   | 20KB
| otadata | data | ota     | 0xE000   | 0x2000   | 0xFFFF   | 8KB
| app0    | app  | ota_0   | 0x10000  | 0x180000 | 0x18FFFF | 1.5MB
| app1    | app  | ota_1   | 0x190000 | 0x180000 | 0x30FFFF | 1.5MB
| spiffs  | data | spiffs  | 0x310000 | 0xF0000  | 0x3FFFFF | 960KB
```

**BREAKING CHANGE WARNING:**
- This partition change requires a full USB reflash
- NVS will be erased - all saved settings will be lost
- LittleFS will be erased - all uploaded logos will be lost
- Users should export settings first (but that endpoint doesn't exist yet - this is the first Foundation story)

**Binary Size Targets:**
- Warning threshold: 1.3MB (1,363,148 bytes)
- Hard limit: 1.5MB (1,572,864 bytes) - must fit in app0/app1 partition
- Current MVP partition has 2MB app space - we're cutting it to 1.5MB

**Optimization Flags (if needed):**
```ini
build_flags =
    -D CONFIG_BT_ENABLED=0        ; Saves ~60KB - Bluetooth disabled
    -D CONFIG_BTDM_CTRL_MODE_BLE_ONLY=1  ; If BLE needed, saves some
    -Os                            ; Optimize for size (already default)
```

### Current vs Target Partition Layout

**Current (MVP - from exploration):**
```csv
# Name,    Type,  SubType, Offset,   Size
nvs,       data,  nvs,     0x9000,   0x5000      # 20KB
otadata,   data,  ota,     0xe000,   0x2000      # 8KB
app0,      app,   ota_0,   0x10000,  0x200000    # 2MB (single app)
spiffs,    data,  spiffs,  0x210000, 0x1F0000   # ~1.9MB
```

**Target (Foundation dual-OTA):**
```csv
# Name,    Type,  SubType, Offset,   Size
nvs,       data,  nvs,     0x9000,   0x5000      # 20KB
otadata,   data,  ota,     0xe000,   0x2000      # 8KB
app0,      app,   ota_0,   0x10000,  0x180000    # 1.5MB
app1,      app,   ota_1,   0x190000, 0x180000    # 1.5MB (NEW)
spiffs,    data,  spiffs,  0x310000, 0xF0000    # 960KB (reduced)
```

**Space Trade-off:**
- App space: 2MB → 1.5MB per slot (but now have 2 slots for OTA)
- LittleFS: ~1.9MB → 960KB (56% reduction)
- LittleFS budget: ~80KB web assets + ~198KB logos (~2KB each) + ~682KB free

### FW_VERSION Macro Usage

**In platformio.ini:**
```ini
build_flags =
    -D FW_VERSION=\"1.0.0\"
```

**In C++ code (will be used in later stories):**
```cpp
#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"  // Fallback for IDE/testing
#endif

Serial.printf("Firmware version: %s\n", FW_VERSION);
```

**Note:** The escaped quotes `\"` in platformio.ini produce a string literal. The preprocessor will replace `FW_VERSION` with `"1.0.0"`.

### Testing Standards

**Build Verification:**
- `pio run` must complete with 0 errors
- Check `.pio/build/esp32dev/firmware.bin` size
- Size logged in build output as "Building .pio/build/esp32dev/firmware.bin"

**Hardware Verification:**
- Boot log shows LittleFS mount success
- Web server responds at device IP
- Flight data displays on LED matrix
- All dashboard sections load

**Filesystem Verification:**
```bash
# Check LittleFS space after upload
# Will be visible in serial output or via /api/status endpoint
```

### Project Structure Notes

**Files to modify:**
1. `firmware/custom_partitions.csv` - Partition table definition
2. `firmware/platformio.ini` - Build flags and partition reference

**platformio.ini already has:**
- `board_build.filesystem = littlefs` ✓
- `board_build.partitions = custom_partitions.csv` ✓

**Only changes needed in platformio.ini:**
- Add `-D FW_VERSION=\"1.0.0\"` to build_flags

### References

- [Source: architecture.md#Decision F1: Partition Table — Dual OTA Layout]
- [Source: architecture.md#Decision F2: OTA Handler Architecture — WebPortal + main.cpp] (for FW_VERSION usage context)
- [Source: prd.md#Update Mechanism] - Partition table explanation
- [Source: prd.md#Flash Partition Layout] - 4MB total with custom CSV
- [Source: epic-fn-1.md#Story fn-1.1] - Full acceptance criteria
- [Source: project-context.md] - Build from `firmware/` with `pio run`

### Dependencies

**This story has NO code dependencies** - it's the foundation for all Foundation release stories.

**Stories that depend on this:**
- fn-1.2: ConfigManager Expansion (needs partition table for NVS)
- fn-1.3: OTA Upload Endpoint (needs app1 partition for OTA)
- fn-1.4: OTA Self-Check (needs dual-OTA partitions)
- fn-1.5: Settings Export (needs working firmware)
- fn-1.6: Dashboard Firmware Card (needs FW_VERSION)
- fn-1.7: Settings Import (needs working wizard)

### Risk Mitigation

1. **Binary too large:** If >1.5MB, add `-D CONFIG_BT_ENABLED=0`. BT is not used in MVP.
2. **LittleFS mount fails:** Verify partition offset/size math. Use `pio run -t erase` before flash if needed.
3. **Existing functionality broken:** This is a regression risk. Run full smoke test after flash.

## Dev Agent Record

### Agent Model Used

_To be filled by implementing agent_

### Debug Log References

_To be filled during implementation_

### Completion Notes List

_To be filled during implementation_

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive context | BMad |

### File List

_To be filled during implementation - expected:_
- `firmware/custom_partitions.csv` (MODIFIED)
- `firmware/platformio.ini` (MODIFIED)
