# Story dl-4.2: Schedule Management Dashboard UI

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want to create, edit, and delete mode schedule rules through the dashboard,
So that I can configure the wall's automated behavior quickly without touching firmware.

## Acceptance Criteria

1. **Given** **`WebPortal`** after **`dl-4.1`** (**`ModeScheduleConfig`** / **`getModeSchedule()`** / **`setModeSchedule()`** exist), **When** the client calls **`GET /api/schedule`**, **Then** the response is **`{ ok: true, data: { ... } }`** where **`data`** includes:
   - **`rules`**: array (length **0–8**) of objects **`{ index, start_min, end_min, mode_id, enabled }`** mirroring **`ScheduleRule`** / **`ModeScheduleConfig`** (**minutes** **0–1439**, **`mode_id`** string, **`enabled`** **0/1**).
   - **`orchestrator_state`**: **`"manual"`** | **`"scheduled"`** | **`"idle_fallback"`** — same semantics as **`GET /api/display/modes`** (**`ModeOrchestrator::getStateString()`**).
   - **`active_rule_index`**: **signed int** **`-1`** when not **`SCHEDULED`** or no matching rule; otherwise **0–7** for the **winning** rule index (**epic** **FR39** / transparency). If **`dl-4.1`** did not yet expose this on **`ModeOrchestrator`**, add **`getActiveScheduleRuleIndex()`** (or equivalent) as part of this story, sourced from orchestrator state — **not** recomputed ad hoc in **`WebPortal`** with divergent logic.

2. **Given** a **POST** to **`/api/schedule`** with a JSON body **`{ "rules": [ ... ] }`** (full replacement list, max **8** entries), **When** validation passes, **Then** **`ConfigManager::setModeSchedule(...)`** persists to **NVS**, response is **`{ ok: true, data: { applied: true } }`** (**epic**), and **no** direct **`ModeRegistry::requestSwitch`** from the handler (**Rule 24** — orchestrator **`tick`** picks up schedule on **next** **~1s** cycle after NVS/cache update).

3. **Given** **`POST`** payload with **invalid** data (**`start_min`/`end_min`** out of range, unknown **`mode_id`**, **>8** rules, malformed JSON), **When** the handler runs, **Then** respond **`400`** with **`ok: false`**, a **`code`** string (**e.g.** **`INVALID_SCHEDULE`**, **`UNKNOWN_MODE`**), and a short **`message`** — mirror **`_sendJsonError`** style used elsewhere in **`WebPortal.cpp`**.

4. **Given** the owner **edits** a rule **in place** (same **`index`** in the posted array), **When** **`POST`** succeeds, **Then** that slot’s **NVS** keys update and **lower indices** are unchanged (**epic**).

5. **Given** the owner **deletes** a rule **from the middle** of the list, **When** the client posts the **compacted** array (no gaps; length decremented), **Then** **`setModeSchedule`** persists the compacted order, **`sched_r_count`** matches array length, and **no** orphaned **NVS** slots remain for indices **≥ new count** (**clear** or overwrite stale keys — document choice to match **`dl-4.1`**).

6. **Given** the schedule **UI** section, **When** it loads, **Then** it fetches **`GET /api/schedule`** and **`GET /api/display/modes`** (reuse **`modes[].name`** for labels and **`modes[].id`** for values — **epic** **NFR7**). **Active** rule(s) are **visually** distinct from inactive rows (**FR39** — e.g. CSS class on row when **`index === active_rule_index`** and **`orchestrator_state === "scheduled"`**).

7. **Given** **`dashboard.html`** / **`dashboard.js`**, **When** this story ships, **Then** add a **card** section (same **`section.card`** / heading patterns as **Night Mode** in **`dashboard.html`** ~263+) with: list/add/edit/delete controls, time inputs using **minutes** internally but **owner-local display** for times (**`toLocaleTimeString`** or existing dashboard time helpers if any; **do not** invent a new timezone source — schedule times follow **device local time** post-**NTP**, consistent with orchestrator).

8. **Given** **`firmware/data-src/dashboard.js`** is edited, **When** the change is complete, **Then** regenerate the served gzip asset per project convention (**`gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz`** from **`firmware/`**).

9. **Given** **`pio test`** / **`pio run`** for **`firmware/`**, **Then** add or extend tests: e.g. **`WebPortal`** schedule **JSON** serialize roundtrip **or** **`ConfigManager::setModeSchedule`** integration from a **test** harness if **`WebPortal`** is hard to unit-test — at minimum **no** regressions and **no** new warnings.

## Tasks / Subtasks

- [x] Task 1 (**`WebPortal.cpp`**) — register **`GET /api/schedule`**, **`POST /api/schedule`** (body handler like **`POST /api/display/mode`** ~308–382); build **`rules`** from **`ConfigManager::getModeSchedule()`**; **`active_rule_index`** + **`orchestrator_state`** (**AC: #1–#5**)
- [x] Task 2 (**`ModeOrchestrator`**) — expose **active rule index** if missing after **`dl-4.1`** (**AC: #1**)
- [x] Task 3 (**`dashboard.html`**) — schedule card markup (**AC: #6–#7**)
- [x] Task 4 (**`dashboard.js`**) — load/save rules, error toasts, polling optional (or refresh on save + link to **`/api/status`** **NTP** if needed for "schedule inactive" messaging) (**AC: #6–#7**)
- [x] Task 5 — **`dashboard.js.gz`** regeneration (**AC: #8**)
- [x] Task 6 — Tests (**AC: #9**)

## Dev Notes

### Prerequisite

- **`dl-4-1-schedule-rules-storage-and-orchestrator-integration`** must be **implemented** (or merged) so **`getModeSchedule` / `setModeSchedule`** and orchestrator **schedule** **tick** exist. If **`dl-4-1`** is still **ready-for-dev**, complete it **before** or **in lockstep** with this story.

### API patterns (copy, do not reinvent)

- **Body POST**: **`WebPortal.cpp`** **`POST /api/display/mode`** — chunked body, **`deserializeJson`**, **`request->send(200, ...)`** only when **`index + len == total`**.
- **Mode list + names**: **`_handleGetDisplayModes`** — reuse for populating mode dropdowns.
- **Dashboard card + dirty sections**: **Night Mode** — **`dirtySections`**, **`markSectionDirty('nightmode')`**, **`FW.get('/api/status')`** polling pattern ~3500+ in **`dashboard.js`**.

### JSON shape (suggested contract)

**GET `data`:**

```json
{
  "rules": [
    { "index": 0, "start_min": 480, "end_min": 1320, "mode_id": "departures_board", "enabled": 1 }
  ],
  "orchestrator_state": "manual",
  "active_rule_index": -1
}
```

**POST body:** `{ "rules": [ { "start_min", "end_min", "mode_id", "enabled" }, ... ] }` — **`index`** optional if client always sends ordered full list; server validates **order = array order**.

### Out of scope

- **Settings export/import** including mode schedule (**fn-1.5** / future) unless trivial **reuse** of **`dumpSettingsJson`** — **not** required here.
- **Setup wizard** exposure of schedule.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-4.md` — Story **dl-4.2**
- Prior story: `_bmad-output/implementation-artifacts/stories/dl-4-1-schedule-rules-storage-and-orchestrator-integration.md`
- **`WebPortal`**: `firmware/adapters/WebPortal.cpp` — routes ~296+, **`_handleGetDisplayModes`**
- Portal UI: `firmware/data-src/dashboard.html`, `firmware/data-src/dashboard.js`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- `pio run` — build SUCCESS, 79.9% flash usage (1,256,752 / 1,572,864 bytes)
- `pio test -f test_mode_schedule --without-uploading --without-testing` — compiled successfully, all 26 tests (21 existing + 5 new)
- No new warnings introduced

### Completion Notes List

- **Task 2**: Added `getActiveScheduleRuleIndex()` public static getter to ModeOrchestrator.h/cpp, returning `_activeRuleIndex`. Sourced from orchestrator state as required by AC #1 (not recomputed in WebPortal).
- **Task 1**: Registered `GET /api/schedule` and `POST /api/schedule` in WebPortal.cpp. GET handler builds JSON from `ConfigManager::getModeSchedule()` with `rules[]`, `orchestrator_state`, and `active_rule_index`. POST handler: chunked body accumulation (same pattern as POST /api/display/mode), full JSON deserialization, validates each rule (start_min/end_min 0-1439, enabled 0/1, mode_id existence in ModeRegistry), calls `ConfigManager::setModeSchedule()` — no `ModeRegistry::requestSwitch` per Rule 24. Error responses use `_sendJsonError` with codes INVALID_SCHEDULE, UNKNOWN_MODE, INVALID_JSON.
- **Task 3**: Added "Mode Schedule" card section to dashboard.html between Night Mode and System cards. Includes orchestrator state badge, rules list container, empty state text, and "Add Rule" button.
- **Task 4**: Implemented `loadScheduleRules()` fetching `/api/display/modes` (for dropdown labels) and `/api/schedule`. Rule rows built dynamically with time inputs (using existing `minutesToTime`/`timeToMinutes` helpers), mode dropdown, enabled checkbox, and delete button. Changes auto-save via `POST /api/schedule` with compacted array. Active rule highlighted with `sched-rule-active` CSS class when `orchestrator_state === "scheduled"`.
- **Task 5**: Regenerated dashboard.js.gz, dashboard.html.gz, and style.css.gz per project convention.
- **Task 6**: Added 5 new tests to test_mode_schedule: `test_active_rule_index_default_negative_one`, `test_active_rule_index_returns_matching_rule`, `test_active_rule_index_resets_on_manual_switch`, `test_schedule_json_serialize_roundtrip`, `test_schedule_compaction_no_orphans`. Total test count: 26 (21 + 5).

### File List

- firmware/adapters/WebPortal.h (modified — added _handleGetSchedule/_handlePostSchedule declarations)
- firmware/adapters/WebPortal.cpp (modified — added GET/POST /api/schedule route registrations and handler implementations)
- firmware/core/ModeOrchestrator.h (modified — added getActiveScheduleRuleIndex() declaration)
- firmware/core/ModeOrchestrator.cpp (modified — added getActiveScheduleRuleIndex() implementation)
- firmware/data-src/dashboard.html (modified — added Mode Schedule card section)
- firmware/data-src/dashboard.js (modified — added schedule card JS: load, render, save, add/edit/delete rules)
- firmware/data-src/style.css (modified — added .sched-* CSS rules for schedule card)
- firmware/data/dashboard.js.gz (regenerated)
- firmware/data/dashboard.html.gz (regenerated)
- firmware/data/style.css.gz (regenerated)
- firmware/test/test_mode_schedule/test_main.cpp (modified — added 5 new dl-4.2 tests)

## Change Log

- **2026-04-14**: Implemented all 6 tasks — GET/POST /api/schedule endpoints, ModeOrchestrator active rule index getter, dashboard HTML schedule card, JS schedule management logic (load/save/add/edit/delete), gzip asset regeneration, and 5 new unit tests. Build passes with no new warnings. Status: review.

## Previous story intelligence

- **`dl-4.1`** adds **NVS** + **orchestrator** evaluation; **`dl-4.2`** is the **HTTP + dashboard** surface. **Persist** only through **`ConfigManager::setModeSchedule`**; **never** call **`ModeRegistry::requestSwitch`** from **`WebPortal`** for schedule edits.

## Git intelligence summary

Touches **`WebPortal.cpp`**, **`ModeOrchestrator.*`** (if index API missing), **`dashboard.html`**, **`dashboard.js`**, **`firmware/data/dashboard.js.gz`**, tests.

## Project context reference

`_bmad-output/project-context.md` — **REST** patterns, **LittleFS** gzip assets.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
