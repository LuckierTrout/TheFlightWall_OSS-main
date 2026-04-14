# Story ds-3.2: NVS Mode Persistence & Boot Restore

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a FlightWall user,
I want my display mode selection saved across power cycles,
So that the device boots into my preferred mode without requiring manual reselection.

## Acceptance Criteria

1. **Given** a successful **display** mode switch (registry active mode changes and remains stable), **When** persistence runs, **Then** the active mode id is stored in NVS namespace **`flightwall`** under key **`disp_mode`** (9 characters — **within** the 15-character NVS key limit, **NFR S6**). **Do not** add a second key named **`display_mode`** (epic text uses that name; **this product** standardizes on **`disp_mode`** already used by **`ConfigManager::getDisplayMode` / `setDisplayMode`**). If a one-time migration from an older **`display_mode`** key is ever found in the wild, read it once, write **`disp_mode`**, delete the legacy key — **document** if implemented.

2. **Given** **`ConfigManager::setDisplayMode(const char*)`** is the **single** writer for **`disp_mode`** from firmware (aside from tests), **When** **`ModeRegistry::tickNvsPersist()`** debounces a post-switch write, **Then** it still calls **`ConfigManager::setDisplayMode`** — **no** raw **`Preferences`** writes for **`disp_mode`** outside **`ConfigManager`** (keeps AR7 collision audit centralized).

3. **Given** boot sequence in **`main.cpp`** after **`g_display.initialize()`**, **When** **`ConfigManager::getDisplayMode()`** returns a mode id **present** in **`MODE_TABLE`**, **Then** **`ModeRegistry::requestSwitch(savedMode.c_str())`** succeeds; **When** the id is **unknown** or **`requestSwitch`** returns **false**, **Then** **`ModeRegistry::requestSwitch("classic_card")`** runs **and** **`ConfigManager::setDisplayMode("classic_card")`** is invoked **immediately** so NVS no longer stores an invalid id (**FR28** / epic invalid-mode case).

4. **Given** **Foundation Release** upgrade path (no **`disp_mode`** key has ever been written), **When** the device boots the **first** time after firmware that includes mode persistence, **Then** runtime behavior defaults to **`classic_card`** (already **`getDisplayMode()`** default) **and** **`GET /api/display/modes`** reports **`upgrade_notification: true`** per **FR29** / epic — implement by ensuring NVS flag **`upg_notif`** is **1** when **`disp_mode`** key is **absent** on first boot **or** by explicit **`setUpgNotif(true)`** in **`setup()`** once; **coordinate** with **`POST /api/display/notification/dismiss`** (already clears **`upg_notif`**) and **ds-3.6** banner UX.

5. **Given** a **valid** saved mode that **fails** activation on first **`ModeRegistry::tick()`** (heap guard or **`init()`** false — **rare** at boot), **Then** registry restore logic leaves **`classic_card`** (or prior safe mode) active **and** NVS is **corrected** to the **actually active** registry mode id **within** the same boot window (may require a small hook after first successful **`tick`** or explicit **`setDisplayMode`** from **`ModeRegistry`** error path — **document** chosen approach in Dev Agent Record; **no** silent drift between NVS and **`getActiveModeId()`**).

6. **Given** **`ModeOrchestrator`** / dashboard **manual** mode (Delight), **When** the user’s choice is committed to **`ModeRegistry`**, **Then** **`disp_mode`** eventually matches (**debounced** write is acceptable); **orchestrator** state itself **need not** duplicate NVS if **`ModeRegistry`** remains source of truth for **`disp_mode`** — document interaction if orchestrator adds its own NVS later.

7. **Given** **`test_mode_registry`** / **`test_config_manager`** tests, **When** **`pio test`** runs, **Then** NVS read/write tests for **`disp_mode`** continue to pass; add tests for **invalid boot id → NVS rewritten** if harness supports it.

8. **Given** **`pio run`**, **Then** no new warnings.

## Tasks / Subtasks

- [x] Task 1: Boot NVS correction (AC: #3)
  - [x] 1.1: After fallback **`requestSwitch("classic_card")`** when saved mode invalid, call **`ConfigManager::setDisplayMode("classic_card")`**
  - [x] 1.2: Log once at **`LOG_W`** when correction occurs

- [x] Task 2: Upgrade notification coherence (AC: #4)
  - [x] 2.1: Ensure **`upg_notif`** or equivalent reflects "first run after upgrade" per epic; align **`GET`** with **`setup()`** semantics
  - [x] 2.2: Verify **`POST /api/display/notification/dismiss`** clears flag and **`GET`** returns **`false`**

- [x] Task 3: Post-switch / failed-switch NVS (AC: #5)
  - [x] 3.1: Trace **`ModeRegistry::executeSwitch`** failure paths; add **`setDisplayMode`** where NVS would otherwise stay stale
  - [x] 3.2: Avoid double-write storms (respect debounce where appropriate)

- [x] Task 4: Documentation + tests (AC: #1, #2, #7, #8)
  - [x] 4.1: Comment in **`ConfigManager.h`** / epic cross-ref: **`disp_mode`** canonical name
  - [x] 4.2: Extend tests if missing cases for invalid NVS boot

## Dev Notes

### Already implemented (verify, do not duplicate)

- **`ConfigManager::getDisplayMode` / `setDisplayMode`** — **`firmware/core/ConfigManager.cpp`** ~688–713, key **`disp_mode`**
- **Boot `requestSwitch`** after **`initialize()`** — **`firmware/src/main.cpp`** ~688–697
- **Registry debounced NVS** — **`ModeRegistry::tickNvsPersist`** → **`setDisplayMode`**
- **`GET` `upgrade_notification`** — **`upg_notif`** in **`WebPortal::_handleGetDisplayModes`**
- **Dismiss route** — **`POST /api/display/notification/dismiss`**

### Epic vs product naming

| Epic text | Product |
|-----------|---------|
| NVS key `display_mode` | **`disp_mode`** |

[Source: `_bmad-output/planning-artifacts/epics/epic-ds-3.md` Story ds-3.2, `ConfigManager.h`]

### Dependencies

- **ds-1.5** boot ordering (**`initialize`** before **`requestSwitch`**) — assumed present.
- **ds-3.1** GET/POST display routes — assumed present for **`upgrade_notification`** surface.

### Out of scope

- **ds-3.3** — Mode Picker UI
- **ds-3.6** — full banner (may consume **`upgrade_notification`**)

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-3.md` (Story ds-3.2)
- Architecture: `_bmad-output/planning-artifacts/architecture.md` — **D6** NVS debounce
- Prior: `_bmad-output/implementation-artifacts/stories/ds-3-1-display-mode-api-endpoints.md`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

No debug issues encountered.

### Completion Notes List

- **Task 1 (AC #3):** Added `ConfigManager::setDisplayMode("classic_card")` call in `main.cpp` boot mode restore when `requestSwitch()` rejects an invalid saved mode. LOG_W message updated to indicate NVS correction.
- **Task 2 (AC #4):** Added `ConfigManager::hasDisplayMode()`, `setUpgNotif()`, `getUpgNotif()` to centralize `upg_notif` NVS access (AR7). In `setup()`, detects Foundation Release upgrade (no `disp_mode` key) and sets `upg_notif=1`. Refactored `WebPortal.cpp` to use these centralized methods instead of raw `Preferences` calls.
- **Task 3 (AC #5):** In all three `ModeRegistry::executeSwitch()` failure paths (heap guard, factory null, init fail), set `_nvsWritePending = true` so `tickNvsPersist()` corrects NVS to the actually-active mode after debounce. Added fallback in `tickNvsPersist()` to write `classic_card` when no mode is active (all failed). Respects existing 2-second debounce — no double-write storms.
- **Task 4 (AC #1, #2, #7, #8):** Added canonical name documentation to `ConfigManager.h`. Added 4 new tests: `test_failed_init_corrects_nvs_after_debounce`, `test_has_display_mode_false_when_absent`, `test_has_display_mode_true_after_set`, `test_upg_notif_roundtrip`. No new build warnings.

### Implementation Approach (AC #5 Documentation)

AC #5 NVS correction uses the existing debounced `tickNvsPersist()` mechanism rather than immediate writes. In each `executeSwitch()` failure path, `_nvsWritePending` and `_lastSwitchMs` are set so the next `tickNvsPersist()` call (after 2-second debounce) persists the actually-active mode ID. When `_activeModeIndex == MODE_INDEX_NONE` (no mode active), NVS is corrected to `"classic_card"` as the safe default.

### File List

- firmware/src/main.cpp (modified — boot NVS correction + upgrade notification detection)
- firmware/core/ConfigManager.h (modified — added hasDisplayMode, setUpgNotif, getUpgNotif; canonical name docs)
- firmware/core/ConfigManager.cpp (modified — implemented hasDisplayMode, setUpgNotif, getUpgNotif)
- firmware/core/ModeRegistry.cpp (modified — NVS correction in all failure paths + fallback in tickNvsPersist)
- firmware/adapters/WebPortal.cpp (modified — refactored upg_notif to use ConfigManager methods)
- firmware/test/test_mode_registry/test_main.cpp (modified — added 4 new tests)

## Previous story intelligence

- **ds-3.1** added **`ModeRegistry`**-backed GET and **`upg_notif`**; **ds-3.2** tightens **NVS ↔ boot ↔ active mode** invariants.

## Git intelligence summary

Touches **`main.cpp`**, **`ConfigManager`**, possibly **`ModeRegistry.cpp`** for failure-path NVS sync.

## Project context reference

`_bmad-output/project-context.md` — NVS namespace **`flightwall`**, debounced writes.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
