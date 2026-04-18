## Resolved items

- **Calibration mode uses `volatile bool` instead of `std::atomic<bool>`** (`firmware/adapters/NeoMatrixDisplay.h`): Folded into BF-1 (`bf-1-mode-switch-auto-yield-preempts-test-patterns`). Both `_calibrationMode` and `_positioningMode` converted to `std::atomic<bool>` with explicit `memory_order_acquire` / `memory_order_release` ordering. The auto-yield path in BF-1 reads `isCalibrationMode()` from Core 0 while Core 1 writes via the `/api/calibration/start|stop` endpoints — fixing the atomic was a hard prerequisite for that story rather than independent tech debt. TD-1 story marked superseded-by-bf-1. (Resolved 2026-04-18)

- **Serial vs LOG for dynamic fetch line** (`firmware/src/main.cpp` ~702): Wrapped in `#if LOG_LEVEL >= 2` guard. (Resolved 2026-04-03)

- **ConfigManager cross-core read during `applyJson`**: Investigated — `g_configMutex` already synchronizes all getter/setter access via `ConfigLockGuard` RAII pattern. No action needed. (Confirmed 2026-04-03)

- **`POST /api/settings` pending-body cleanup** (`firmware/adapters/WebPortal.cpp`): Added `request->onDisconnect()` handler to clean up stale `g_pendingBodies` entries on client disconnect. (Resolved 2026-04-03)

- **Unsynchronized subsystem `String` access in `/api/status`** (`firmware/core/SystemStatus.cpp`): Added `SemaphoreHandle_t _mutex` to SystemStatus; `set()`, `get()`, and `toJson()` now take/give mutex. `toJson()` snapshots under lock, serializes outside. (Resolved 2026-04-03)

## Deferred from: code review of fn-1-6-dashboard-firmware-card-and-ota-upload-ui.md (2026-04-13)

- **Mixed-story diff surface** — `dashboard.html` / `dashboard.js` / `style.css` / `WebPortal.cpp` diffs bundle fn-1.6 with dl-1.5 (Display Mode) and other changes; prefer per-story commits or scoped diffs for reviewability.

## Deferred from: code review of le-1-10-widget-field-picker.md (2026-04-17)

- **Unity suites run compile-only in typical agent/CI flows** — On-device `pio test` execution remains a manual gate for full pass; pre-existing constraint across firmware stories.

## Open items

- **OTA self-check Unity tests require hardware upload** (`firmware/test/test_ota_self_check/`): `pio test` on `esp32dev` defaults to build+upload; agents/CI without a connected board cannot execute the suite. Deferred from code review of `fn-1-4-ota-self-check-and-rollback-detection.md` (2026-04-12).

- **`hardwareConfigChanged` if/else if misses combined geometry+mapping changes** (`firmware/src/main.cpp` displayTask): When both geometry AND mapping change in one settings save, only the geometry branch executes. The runtime mapping reconfiguration is skipped, requiring a reboot to apply both. Should apply mapping after geometry detection, or restructure to handle both. (Surfaced by code review 2026-04-03, Story 4.2)
