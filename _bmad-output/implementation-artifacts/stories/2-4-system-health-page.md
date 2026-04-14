# Story 2.4: System Health Page

Status: done

Ultimate context engine analysis completed — comprehensive developer guide created.

## Story

As a user,
I want detailed system status showing WiFi, APIs, heap, uptime, and storage,
so that I can diagnose issues.

## Acceptance Criteria

1. **Navigation:** The System Health page remains linked from the dashboard header (`/health.html`). Layout stays consistent with the dark dashboard (`style.css`, `.dashboard`, `.card`).

2. **Data source:** On load, **`GET /api/status`** returns **`{ ok: true, data: { ... } }`**. The payload **extends** the device’s real telemetry so the UI can render every card below. **No auto-refresh** and no background polling loop — only user-triggered refresh.

3. **WiFi card:** Shows **network name (SSID)** where applicable, **signal (RSSI)**, and **IP** (STA). If in AP mode, show AP SSID / AP IP / client count per what `WiFi` APIs allow — plain language if a field is unavailable.

4. **APIs card:** **OpenSky**, **AeroAPI**, and **CDN** (FlightWall CDN) each show a **colored status dot** derived from `SystemStatus` levels (`ok` / `warning` / `error`) and a **short status line** (message from subsystem or a synthesized summary). Map subsystem keys to labels: `opensky`, `aeroapi`, `cdn`.

5. **API quota card:** Shows **used** (or session / best-effort counter — see Dev Notes), **limit** (4,000 for OpenSky free tier per PRD), and a **rate / pace** indicator (e.g. estimated polls per 30-day month from current `fetch_interval`, or equivalent language). If the device is **over-pace** vs the limit (estimate > 4,000 **or** a firmer rule you document), the card uses **amber** styling (class or inline token matching dashboard warning color).

6. **Device card:** **Uptime** (human-readable from millis since boot), **free heap** (bytes or KiB), **LittleFS** usage **total / used** (or used + free if total not exposed — document the chosen API).

7. **Flight data card:** **Last fetch** (relative time since last successful fetch attempt or last completed cycle — define one and use consistently), **flights in range** (state vector count from last fetch), **enriched** (enriched flight count from last fetch), **logos matched** (show **0** or **—** until Epic 3 logo pipeline exists; do not fake counts).

8. **Refresh control:** A **Refresh** button re-runs **`GET /api/status`** and replaces all card contents. **No** periodic refresh timer.

## Tasks / Subtasks

- [x] Task 1: Firmware — extend `/api/status` (AC: #2–#7)
  - [x] Locate **`WebPortal::_handleGetStatus`** and **`SystemStatus::toJson`** — today `data` is only the six subsystem objects (`wifi`, `opensky`, `aeroapi`, `cdn`, `nvs`, `littlefs`) with `level` / `message` / `timestamp`.
  - [x] Design a **single JSON object** under `data` that includes:
    - **`subsystems`**: nested mirror of current subsystem map **or** keep flat keys and add **additional** sibling keys — pick one structure, document it in `WebPortal.cpp` header comment, and **update the health page** accordingly.
    - **`wifi_detail`** (or `network`): SSID, RSSI, IP, mode.
    - **`device`**: `uptime_ms`, `free_heap`, LittleFS stats.
    - **`flight`**: last-fetch metadata + last counts (see runtime capture below).
    - **`quota`**: limit, optional used counter, `fetch_interval_s`, `estimated_monthly_polls`, `over_pace` boolean.
  - [x] **Runtime flight stats:** Add a small **thread-safe or loop-owned snapshot** updated at the end of each fetch in **`main.cpp`** (e.g. last fetch wall clock / `millis`, `state_vectors`, `enriched_flights`, `logos_matched` placeholder). **Do not** read the flight queue from the HTTP handler for heavy copies unless already safe; prefer atomic or copied scalars set right after `fetchFlights`.

- [x] Task 2: Firmware — quota "used" (AC: #5)
  - [x] **MVP options** (choose one and document in completion notes):
    - **A)** `fetches_since_boot` counter only, label in UI as **"Since boot"** to avoid implying calendar-month accuracy, **or**
    - **B)** NVS-backed monthly counter with coarse reset (higher scope — only if time permits).
  - [x] **Pace / amber:** Reuse the Story **2.2** monthly estimate:
    `estimated_monthly_polls = round(2_592_000 / fetch_interval_seconds)` **(OpenSky poll proxy)**; set **`over_pace`** when `estimated_monthly_polls > 4000`. If you also surface **`fetches_since_boot`**, keep the amber rule aligned with the epic (estimate vs limit is sufficient for warning).

- [x] Task 3: **`health.html` / `health.js`** (AC: #1, #8)
  - [x] Replace the minimal subsystem list with **distinct cards** matching the epic: WiFi, APIs, API Quota, Device, Flight Data.
  - [x] Implement **`refresh()`** bound to the Refresh button; initial load calls the same function.
  - [x] Map API levels to **dot colors** (reuse `--success`, warning `#d29922` or UX `--warning`, `--error`).
  - [x] **Relative time** for last fetch: simple `formatAgo(ms)` in JS is fine.

- [x] Task 4: **`style.css`**
  - [x] Add compact card/list styles if needed: status row with dot + label, quota highlight for `.over-pace` / warning.

- [x] Task 5: Build & test
  - [x] `pio run`; regenerate / upload LittleFS if assets changed.
  - [ ] Manual: open health page STA, verify all sections populate; tap Refresh; confirm no auto-polling (watch network tab).

### Review Findings

- [x] [Review][Patch] AP fallback reports STA details instead of AP diagnostics, so the health page violates the AP-mode card behavior in AC3 when WiFi recovery is active. `SystemStatus::toExtendedJson()` treated `WIFI_AP_STA` as normal STA mode and serialized `WiFi.SSID()`, `WiFi.RSSI()`, and `WiFi.localIP()`, but `WiFiManager::_startAPFallback()` explicitly enters `WIFI_AP_STA` for recovery mode. Fixed by treating `WIFI_AP_STA` as AP-mode diagnostics in the health payload. [firmware/core/SystemStatus.cpp:58]
- [x] [Review][Defer] `/api/status` still reads and writes subsystem `String` messages without synchronization across async HTTP requests and WiFi/event updates. [firmware/core/SystemStatus.cpp:25] — deferred, pre-existing

## Dev Notes

### Current baseline (avoid surprises)

- **`health.js` today** only lists subsystem keys from `/api/status` — this story **replaces** that UI and likely **changes JSON shape** under `data`.
- **`GET /api/status`** currently: `ConfigManager` not involved; handler builds `SystemStatus::toJson(data)` only.
- **Flight fetch loop** lives in **`main.cpp` `loop()`**; variables like **`g_lastFetchMs`** exist but are not exposed via API yet.

### Threading / core guardrails

- **ESPAsyncWebServer** runs on async TCP; **`loop()`** runs on Core 1. Any scalar stats written from `loop()` and read from the status handler should use **`std::atomic`** for multi-word consistency or a **single struct snapshot** written atomically (padding/alignment) — avoid torn reads.

### “Logos matched” placeholder

- Epic 3 introduces real logo matching. Until then, expose **`logos_matched: 0`** or **`null`** with UI copy **“—”** so the card layout is stable.

### Previous story intelligence

- **2.1:** `FW.get`, `FW.showToast`, `/health.html` routing — already wired.
- **2.3:** Network reboot flow — health page should remain a **read-only** diagnostic surface; do not add `POST /api/reset` here (Story **2.5**).

### Architecture compliance

- Prefer keeping JSON assembly in **`WebPortal::_handleGetStatus`** or a thin **`SystemStatus` / `Diagnostics` helper** in `core/` — avoid duplicating HTTP logic in `main.cpp` beyond updating the shared snapshot.

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` — Story 2.4]
- [Source: `_bmad-output/planning-artifacts/prd.md` — FR9, OpenSky 4k tier, system health concepts]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` — health card, no auto-refresh, warning colors]
- [Source: `firmware/adapters/WebPortal.cpp` — `_handleGetStatus`]
- [Source: `firmware/core/SystemStatus.h/.cpp` — subsystems, levels]
- [Source: `firmware/src/main.cpp` — fetch loop, `g_lastFetchMs`]
- [Source: `firmware/data-src/health.html`, `health.js` — rewrite]
- [Source: `_bmad-output/implementation-artifacts/2-2-timing-controls-and-api-usage-estimate.md` — monthly poll estimate formula]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, RAM 15.7%, Flash 55.5%
- No unit test framework configured for this ESP32 firmware project (embedded, manual verification)

### Completion Notes List

1. **JSON structure decision:** Chose `data.subsystems` (nested mirror of existing six subsystem objects) + sibling keys `wifi_detail`, `device`, `flight`, `quota`. Documented in `WebPortal.cpp` header comment. Existing `toJson()` still available for backward compatibility.

2. **Thread safety:** Added `std::atomic` scalars in `main.cpp` (`g_statsLastFetchMs`, `g_statsStateVectors`, `g_statsEnrichedFlights`, `g_statsLogosMatched`, `g_statsFetchesSinceBoot`) written from `loop()` on Core 1, read via `getFlightStatsSnapshot()` from the HTTP handler on async TCP task. No heavy struct copies or queue reads needed.

3. **Quota MVP:** Chose **Option A** — `fetches_since_boot` counter only, labeled "Since boot" in UI to avoid implying calendar-month accuracy. Pace formula: `estimated_monthly_polls = 2,592,000 / fetch_interval_seconds`, amber when > 4,000.

4. **Logos matched:** Exposed as `0` in JSON, displayed as "—" in UI until Epic 3 logo pipeline.

5. **WiFi detail:** STA mode shows SSID, RSSI, IP. AP mode shows AP SSID, AP IP, client count. Off mode shows "Off".

6. **LittleFS stats:** Using `LittleFS.totalBytes()` and `LittleFS.usedBytes()` — both available on ESP32 Arduino LittleFS.

7. **No auto-refresh:** Health page uses manual Refresh button only. Initial load calls `refresh()` once; no setInterval or polling.

8. **`SystemStatus::levelName` made public** so it can be reused externally if needed (was private).

### Change Log

- 2026-04-02: Implemented Story 2.4 — System Health Page with extended /api/status endpoint, five health cards (WiFi, APIs, API Quota, Device, Flight Data), thread-safe flight stats, and quota pace tracking.

### File List

- `firmware/core/SystemStatus.h` — added `FlightStatsSnapshot` struct, `toExtendedJson()`, made `levelName` public
- `firmware/core/SystemStatus.cpp` — implemented `toExtendedJson()` with wifi_detail, device, flight, quota sections
- `firmware/src/main.cpp` — added atomic flight stats globals, `getFlightStatsSnapshot()`, stats update after each fetch
- `firmware/adapters/WebPortal.cpp` — updated header comment, added extern for `getFlightStatsSnapshot`, switched `_handleGetStatus` to use `toExtendedJson`
- `firmware/data-src/health.html` — rewritten with five distinct cards (WiFi, APIs, API Quota, Device, Flight Data) + Refresh button
- `firmware/data-src/health.js` — rewritten with card renderers, `formatUptime`, `formatAgo`, `formatBytes`, refresh logic
- `firmware/data-src/style.css` — added `.health-loading`, `.status-row`, `.status-label`, `.status-value`, `.status-dot`, `.over-pace` styles
- `firmware/data/health.html.gz` — regenerated
- `firmware/data/health.js.gz` — regenerated
- `firmware/data/style.css.gz` — regenerated
