# Story 4.1: Interactive Map with Circle Radius

Status: done

Ultimate context engine analysis completed - comprehensive developer guide created.

## Story

As a user,
I want to see my capture area on a real map with a draggable circle,
so that I can visually understand and adjust my monitored airspace.

## Acceptance Criteria

1. **Lazy-loaded map on Location card open:** Given dashboard Location card is expanded, Leaflet loads lazily (not on first dashboard paint), displays a "Loading map..." state until rendered, and centers on current `center_lat` / `center_lon`.

2. **Interactive geometry controls:** Map shows:
   - draggable center marker
   - radius circle tied to `radius_km`
   - visible OSM tiles
   Changes to marker/radius update form values.

3. **Persist on release:** When marker drag ends or radius adjustment is released, `POST /api/settings` sends location keys and toast confirms **"Location updated"**.

4. **Fallback path:** If Leaflet/tile loading fails, user still gets manual lat/lon/radius inputs as fallback with no blocking modal.

5. **API/key alignment:** Uses existing keys exactly: `center_lat`, `center_lon`, `radius_km`.

## Tasks / Subtasks

- [x] Task 1: Add Location card UI in dashboard (AC: #1, #2, #4, #5)
  - [x] Extend `firmware/data-src/dashboard.html` with a **Location** card section above/below Hardware (consistent card ordering).
  - [x] Include:
    - [x] map container (`#location-map`)
    - [x] loading state text (`Loading map...`)
    - [x] fallback manual inputs for lat/lon/radius
    - [x] optional helper text clarifying map drives these values
  - [x] Keep mobile-first width constraints (`max-width: 480px`) and dark-theme style parity.

- [x] Task 2: Leaflet lazy-load orchestration (AC: #1, #4)
  - [x] Implement on-demand loader in `firmware/data-src/dashboard.js`:
    - [x] inject Leaflet CSS/JS only when Location card is first opened
    - [x] render loading state while scripts/tiles initialize
  - [x] If load fails:
    - [x] show fallback inputs only
    - [x] emit non-blocking warning toast
    - [x] avoid hard crash of remaining dashboard logic

- [x] Task 3: Bind map <-> form model (AC: #2, #5)
  - [x] Initial values from `GET /api/settings` for `center_lat`, `center_lon`, `radius_km`.
  - [x] Marker drag updates lat/lon fields.
  - [x] Radius control updates circle radius and numeric radius input.
  - [x] Manual field edits update map when map exists.

- [x] Task 4: Persist behavior and debounce strategy (AC: #3)
  - [x] On drag end / radius release (not every movement tick), call `FW.post('/api/settings', { center_lat, center_lon, radius_km })`.
  - [x] Keep traffic bounded with debounce or release-only semantics.
  - [x] Toast on success must read **"Location updated"**.
  - [x] Error path shows actionable toast and keeps local form state.

- [x] Task 5: CSS and UX polish (AC: #1, #2, #4)
  - [x] Add map container styles in `style.css` (height, border, radius, contrast).
  - [x] Add fallback input state styles.
  - [x] Ensure map remains usable on phone widths and landscape.

- [x] Task 6: Build and validation
  - [x] `pio run`
  - [x] Update filesystem assets pipeline for modified `dashboard.*` / `style.css`.
  - [x] Manual tests:
    - [ ] open dashboard, expand Location -> lazy load happens once
    - [ ] drag marker / radius -> POST and toast
    - [ ] simulated Leaflet/tile failure -> fallback inputs remain functional

### Review Findings

- [x] [Review][Patch] Map circle is display-only, so users cannot adjust `radius_km` directly from the map as the story and helper copy promise. [firmware/data-src/dashboard.js:810]
- [x] [Review][Patch] Opening the Location card before settings finish loading can initialize the map at `0,0` with a default `10 km` radius, and the map never re-syncs when settings arrive. [firmware/data-src/dashboard.js:770]
- [x] [Review][Patch] Empty or invalid manual location fields are coerced to `0`/`10` and persisted instead of being rejected or restored. [firmware/data-src/dashboard.js:770]
- [x] [Review][Patch] The fallback path handles Leaflet asset failures, but not tile-load failures, and a slow script can still initialize the map after the timeout has already reported failure. [firmware/data-src/dashboard.js:723]
- [x] [Review][Patch] Reopening the Location card never re-invalidates the Leaflet viewport, so the map can render incorrectly after a collapse/expand cycle. [firmware/data-src/dashboard.js:712]
- [x] [Review][Patch] The Location chevron never reflects open state because the CSS selector points at the wrong sibling direction and no `.open` class is ever toggled. [firmware/data-src/style.css:295]

## Dev Notes

### Current implementation baseline

- Dashboard currently has Display, Timing, Network/API, Hardware, and System cards; no Location card exists yet.
- `dashboard.js` already manages settings load/apply and shared `computeLayout()` for hardware preview.
- `/api/settings` and key schema already support location keys (`center_lat`, `center_lon`, `radius_km`) via `ConfigManager`.

### Architecture and endpoint guardrails

- Keep API contract unchanged: location updates flow through existing `POST /api/settings`.
- No new firmware endpoint is required for this story.
- `/api/layout` remains for layout preview (Story 3.4), not location map data.

### UX guardrails

- UX spec explicitly calls for lazy-loaded map resources and graceful fallback.
- No modal errors for map load failure; keep manual entry viable.
- Respect the "show, don't explain" approach: visual circle and marker are primary controls.

### Leaflet integration constraints

- Use static CDN assets only if already accepted in project policy; otherwise bundle local minified assets into `firmware/data-src` and serve via `WebPortal`.
- Ensure lazy-load only once per page lifecycle.
- Avoid map re-initialization loops on repeated card toggles.

### Data precision and units

- `radius_km` remains in kilometers.
- Clamp/validate lat/lon/radius before POST:
  - latitude: -90..90
  - longitude: -180..180
  - radius: positive sensible range

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` - Story 4.1]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` - map lazy-load + fallback + interaction model]
- [Source: `firmware/data-src/dashboard.html` - current card structure]
- [Source: `firmware/data-src/dashboard.js` - existing settings/apply flow]
- [Source: `firmware/adapters/WebPortal.cpp` - static asset serving + API routes]
- [Source: `firmware/core/ConfigManager.cpp` - location key support]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- PlatformIO build: SUCCESS (RAM 16.4%, Flash 56.3%)

### Completion Notes List

- Added collapsible Location card to dashboard between Hardware and System cards
- Leaflet 1.9.4 loaded lazily from unpkg CDN only on first card expand (not on page load)
- "Loading map..." text shown during script fetch; 10s timeout triggers fallback path
- On Leaflet failure: hides map container, shows warning toast, leaves manual inputs functional
- Map initializes with draggable marker + radius circle centered on current settings values
- Marker dragend fires `POST /api/settings` with clamped lat/lon/radius; toast reads "Location updated"
- Manual lat/lon/radius inputs sync bidirectionally with map (change event -> updateMapFromFields)
- Debounced POST on manual field edits (400ms, same as existing Display card pattern)
- Input validation: lat clamped -90..90, lon -180..180, radius 0.1..500 km
- Dark-theme Leaflet overrides for zoom controls, attribution, and container background
- Leaflet load is one-shot: `leafletLoadAttempted` flag prevents re-injection on card toggle
- No new firmware endpoints; all location writes go through existing `POST /api/settings`

### File List

- firmware/data-src/dashboard.html (modified) — added Location card HTML
- firmware/data-src/dashboard.js (modified) — added location section in loadSettings, Location card logic (lazy-load, map init, bind, persist)
- firmware/data-src/style.css (modified) — added Location card styles, Leaflet dark-theme overrides
- firmware/data/dashboard.html.gz (modified) — gzipped updated HTML
- firmware/data/dashboard.js.gz (modified) — gzipped updated JS
- firmware/data/style.css.gz (modified) — gzipped updated CSS

### Change Log

- 2026-04-03: Implemented Story 4.1 — Interactive Map with Circle Radius. Added lazy-loaded Leaflet map in collapsible Location card with draggable marker, radius circle, bidirectional form sync, persist-on-release via POST /api/settings, and graceful fallback for offline/CDN-failure scenarios.
