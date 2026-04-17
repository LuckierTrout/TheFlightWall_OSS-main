<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: le-1.9 -->
<!-- Phase: retrospective -->
<!-- Timestamp: 20260417T224752Z -->
<compiled-workflow>
<mission><![CDATA[

Run after epic completion to review overall success, extract lessons learned, and explore if new information emerged that might impact the next epic

Target: Epic le-1
Generate retrospective report with extraction markers.

]]></mission>
<context>
<file id="ed7fe483" path="_bmad-output/project-context.md" label="PROJECT CONTEXT"><![CDATA[

---
project_name: TheFlightWall_OSS-main
date: '2026-04-12'
---

# Project Context for AI Agents

Lean rules for implementing FlightWall (ESP32 LED flight display + captive-portal web UI). Prefer existing patterns in `firmware/` over new abstractions.

## Technology Stack

- **Firmware:** C++11, ESP32 (Arduino/PlatformIO), FastLED + Adafruit GFX + FastLED NeoMatrix, ArduinoJson ^7.4.2.
- **Web on device:** ESPAsyncWebServer (**mathieucarbou fork**), AsyncTCP (**Carbou fork**), LittleFS (`board_build.filesystem = littlefs`), custom `custom_partitions.csv` (~2MB app + ~2MB LittleFS).
- **Dashboard assets:** Editable sources under `firmware/data-src/`; served bundles are **gzip** under `firmware/data/`. After editing a source file, regenerate the matching `.gz` from `firmware/` (e.g. `gzip -9 -c data-src/common.js > data/common.js.gz`).

## Critical Implementation Rules

- **Core pinning:** Display/task driving LEDs on **Core 0**; WiFi, HTTP server, and flight fetch pipeline on **Core 1** (FastLED + WiFi ISR constraints).
- **Config:** `ConfigManager` + NVS; debounce writes; atomic saves; use category getters; `POST /api/settings` JSON envelope `{ ok, data, error, code }` pattern for REST responses.
- **Heap / concurrency:** Cap concurrent web clients (~2–3); stream LittleFS reads; use ArduinoJson filter/streaming for large JSON; avoid full-file RAM buffering for uploads.
- **WiFi:** WiFiManager-style state machine (AP setup → STA → reconnect / AP fallback); mDNS `flightwall.local` in STA.
- **Structure:** Extend hexagonal layout — `firmware/core/`, `firmware/adapters/` (e.g. `WebPortal.cpp`), `firmware/interfaces/`, `firmware/models/`, `firmware/config/`, `firmware/utils/`.
- **Tooling:** Build from `firmware/` with `pio run`. On macOS serial: use `/dev/cu.*` (not `tty.*`); release serial monitor before upload.
- **Scope for code reviews:** Product code under `firmware/` and tests under `firmware/test/` and repo `tests/`; do not treat BMAD-only paths as product defects unless the task says so.

## Planning Artifacts

- Requirements and design: `_bmad-output/planning-artifacts/` (`architecture.md`, `epics.md`, PRDs).
- Stories and sprint line items: `_bmad-output/implementation-artifacts/` (e.g. `sprint-status.yaml`, per-story markdown).

## BMAD / bmad-assist

- **`bmad-assist.yaml`** at repo root configures providers and phases; `paths.project_knowledge` points at `_bmad-output/planning-artifacts/`, `paths.output_folder` at `_bmad-output/`.
- **This file** (`project-context.md`) is resolved at `_bmad-output/project-context.md` or `docs/project-context.md` (see `bmad-assist` compiler `find_project_context_file`).
- Keep **`sprint-status.yaml`** story keys aligned with `.bmad-assist/state.yaml` (`current_story`, `current_epic`) when using `bmad-assist run` so phases do not skip with “story not found”.


]]></file>
<file id="8c5456f3" path="_bmad-output/planning-artifacts/prd-delight-validation-report.md" label="DOCUMENTATION"><![CDATA[

---
validationTarget: '_bmad-output/planning-artifacts/prd-delight.md'
validationDate: '2026-04-11T18:00:00-0400'
inputDocuments:
  - '_bmad-output/planning-artifacts/prd.md'
  - '_bmad-output/planning-artifacts/prd-foundation.md'
  - '_bmad-output/planning-artifacts/prd-display-system.md'
  - '_bmad-output/planning-artifacts/architecture.md'
  - 'docs/index.md'
  - 'docs/architecture.md'
  - 'docs/component-inventory.md'
  - 'docs/project-overview.md'
  - 'docs/source-tree-analysis.md'
  - 'docs/development-guide.md'
  - 'docs/api-contracts.md'
  - 'docs/setup-guide.md'
  - 'docs/device-setup-and-flash-guide.md'
  - 'docs/device-setup-and-flash-printable.md'
validationStepsCompleted:
  - 'step-v-01-discovery'
  - 'step-v-02-format-detection'
  - 'step-v-03-density-validation'
  - 'step-v-04-brief-coverage-validation'
  - 'step-v-05-measurability-validation'
  - 'step-v-06-traceability-validation'
  - 'step-v-07-implementation-leakage-validation'
  - 'step-v-08-domain-compliance-validation'
  - 'step-v-09-project-type-validation'
  - 'step-v-10-smart-validation'
  - 'step-v-11-holistic-quality-validation'
  - 'step-v-12-completeness-validation'
validationStatus: COMPLETE
holisticQualityRating: '5/5 - Excellent'
overallStatus: Pass
---

# PRD Validation Report

**PRD Being Validated:** `_bmad-output/planning-artifacts/prd-delight.md`  
**Validation Date:** 2026-04-11T18:00:00-0400

## Input Documents

- `_bmad-output/planning-artifacts/prd-delight.md` (primary)
- `_bmad-output/planning-artifacts/prd.md`
- `_bmad-output/planning-artifacts/prd-foundation.md`
- `_bmad-output/planning-artifacts/prd-display-system.md`
- `_bmad-output/planning-artifacts/architecture.md`
- `docs/index.md`
- `docs/architecture.md`
- `docs/component-inventory.md`
- `docs/project-overview.md`
- `docs/source-tree-analysis.md`
- `docs/development-guide.md`
- `docs/api-contracts.md`
- `docs/setup-guide.md`
- `docs/device-setup-and-flash-guide.md`
- `docs/device-setup-and-flash-printable.md`

**Discovery note:** No additional reference documents beyond the PRD `inputDocuments` list (treated as `none`).

## Validation Findings

## Format Detection

**Frontmatter Classification:**

- `classification.projectType`: `iot_embedded`
- `classification.domain`: `general`

**PRD Structure (## headers, in order):**

- Executive Summary  
- Success Criteria  
- User Journeys  
- IoT/Embedded Specific Requirements  
- Project Scoping & Phased Development  
- Functional Requirements  
- Non-Functional Requirements  

**BMAD Core Sections Present:**

- Executive Summary: Present  
- Success Criteria: Present  
- Product Scope: Present (`## Project Scoping & Phased Development` — authoritative scope, MVP, phased roadmap)  
- User Journeys: Present  
- Functional Requirements: Present  
- Non-Functional Requirements: Present  

**Format Classification:** BMAD Standard  
**Core Sections Present:** 6/6  

## Information Density Validation

**Anti-Pattern Violations:**

**Conversational Filler:** 0 occurrences (no systematic matches for “It is important to note”, “In order to”, “The system will allow users to…”, etc.)

**Wordy Phrases:** 0 occurrences (no matches for “Due to the fact that”, “At this point in time”, etc.)

**Redundant Phrases:** 0 occurrences  

**Total Violations:** 0  

**Severity Assessment:** Pass  

**Recommendation:** PRD demonstrates strong information density with minimal filler.

## Product Brief Coverage

**Status:** N/A — No Product Brief was provided as input.

## Measurability Validation

### Functional Requirements

**Total FRs Analyzed:** 43  

**Format violations:** 0 (PRD uses consistent “The system can…” / “The owner can…” capability form; acceptable for embedded product.)

**Subjective adjectives (strict scan):** 0 blocking — **Informational only:** FR10 / FR12 use perceptual language (“animated”, “without visible tearing”) that is partially covered by quantified NFR1/NFR2; acceptable for display hardware but relies on human judgment in edge cases.

**Vague quantifiers:** 0 (prior issues on FR6, FR30, FR33–FR35 resolved.)

**Implementation leakage (strict scan):** 0 blocking — IoT terms (ESP32, NVS, LED matrix, GitHub, partition table path) are **capability- and constraint-relevant** for this product class; **FR41** naming “compile-time mode registry” is an explicit extension contract, not arbitrary stack detail.

**FR violations (Warning/Critical):** 0  

### Non-Functional Requirements

**Total NFRs Analyzed:** 21  

**Missing metrics:** 0 blocking — NFR4, NFR14, NFR18, NFR19 now include concrete bounds, per-API behavior, or verification-oriented dashboard outcomes.

**Incomplete template:** 0  

**NFR violations (Warning/Critical):** 0  

### Overall Assessment

**Total requirements:** 64  
**Blocking violations:** 0  
**Informational notes:** Perceptual wording on FR10/FR12; FR43 references “competent hobbyist” (inherent for onboarding doc tests — mitigate with checklist-style QA).

**Severity:** Pass  

**Recommendation:** Requirements are testable for downstream epics and QA; optional hardening: add acceptance checklist pointers for FR43.

## Traceability Validation

### Chain Validation

**Executive Summary → Success Criteria:** Intact — six Delight capabilities align with user, business, technical, and measurable outcomes.

**Success Criteria → User Journeys:** Intact — **Journey 0** supports self-service onboarding; **Journey 4** supports contributor-ready architecture; other criteria map to Journeys 1–3.

**User Journeys → Functional Requirements:** Intact — **FR43** ↔ Journey 0; **FR41–FR42** ↔ Journey 4; scope note clarifies Display System inheritance vs Delight formalization.

**Scope → FR Alignment:** Intact — six MVP capabilities map to FR groups; **FR41–FR43** extend traceability without contradicting MVP cut lines (contributor *guide* still Growth).

### Orphan Elements

**Orphan Functional Requirements:** 0  

**Unsupported Success Criteria:** 0  

**User Journeys Without FRs:** 0  

### Traceability Matrix

| Capability / Theme | Executive Summary / Success Source | User Journey Support | FR Coverage | Status |
|------------------|--------------------------------------|-------------------------|-------------|--------|
| Clock Mode idle fallback | Capabilities + user success | Journeys 0, 1, 3 | FR1–FR4 | Covered |
| Departures Board | Capabilities + user success | Journeys 0, 1, 3 | FR5–FR9 | Covered |
| Animated transitions | Capabilities + user success | Journeys 1, 3 | FR10–FR12, NFR1–NFR2 | Covered |
| Mode scheduling | Capabilities + user success | Journey 1 | FR13–FR17, FR36, FR39 | Covered |
| Mode-specific settings | Capabilities | Journey 1 | FR18–FR20, FR40 | Covered |
| OTA Pull | Capabilities + user success | Journey 2 | FR21–FR31, FR37–FR38 | Covered |
| Resilience / API degradation | Technical + integration | Journeys 2, 0 (implicit stability) | FR32–FR35, NFR18–NFR21 | Covered |
| Self-service onboarding | Business success | **Journey 0** | **FR43** | Covered |
| Contributor extensibility | Business success | **Journey 4** | **FR41–FR42** | Covered |

**Total Traceability Issues:** 0  

**Severity:** Pass  

**Recommendation:** Traceability chain is complete after Journey 0 and FR41–FR43 additions.

## Implementation Leakage Validation

### Leakage by Category

**Frontend / backend frameworks:** 0  
**Databases:** 0  
**Cloud platforms (generic):** 0  
**Infrastructure:** 0  
**Libraries:** 0  

**Other / IoT-specific terms:** Present in FRs/NFRs where they express **platform constraints or observable behavior** (ESP32, NVS, WiFi, GitHub Releases, `custom_partitions.csv`, mbedTLS) — consistent with prior validation guidance for embedded PRDs.

### Summary

**Total implementation-leakage violations (strict):** 0  

**Severity:** Pass  

**Recommendation:** No significant leakage; platform nouns remain appropriate for IoT.

## Domain Compliance Validation

**Domain:** general  
**Complexity:** Low — no regulated-domain mandatory sections  

**Assessment:** N/A — no special domain compliance requirements.

## Project-Type Compliance Validation

**Project Type:** `iot_embedded` (from PRD frontmatter)

### Required Sections (from `project-types.csv`)

| Required (signal) | PRD location | Status |
|-------------------|--------------|--------|
| hardware_reqs | IoT/Embedded → Hardware Requirements | Present |
| connectivity_protocol | IoT/Embedded → Connectivity Protocol | Present |
| power_profile | IoT/Embedded → Power Profile | Present |
| security_model | IoT/Embedded → Security Model | Present |
| update_mechanism | IoT/Embedded → Update Mechanism | Present |

### Excluded Sections

| Skip (signal) | Present in PRD? |
|---------------|-----------------|
| visual_ui | Absent ✓ |
| browser_support | Absent ✓ |

**Compliance Score:** 100%  

**Severity:** Pass  

**Recommendation:** IoT/embedded section set matches BMAD project-type expectations.

## SMART Requirements Validation

**Total Functional Requirements:** 43  

### Scoring Summary

**All scores ≥ 3 (all categories):** ~98% (41–42 of 43, depending on scorer strictness on perceptual FR10/FR12 and FR43)  
**All scores ≥ 4:** ~93%  
**Overall average (estimated):** ~4.8/5.0  

### Flagged FRs (Informational — any SMART dimension &lt; 4)

| FR # | Note |
|------|------|
| FR10 | Measurable partly delegated to NFR1/NFR2; “animated” is qualitative. |
| FR12 | “Tearing” / artifacts judged visually; pair with test protocol. |
| FR43 | “Competent hobbyist” is inherently qualitative; pair with doc checklist / dry-run acceptance. |

**Severity:** Pass (no FR with dimension &lt; 3 requiring rewrite)

**Recommendation:** Optional test annex: scripted checklist for FR43; visual test notes for FR10/FR12.

## Holistic Quality Assessment

### Document Flow & Coherence

**Assessment:** Excellent — vision → capabilities → journeys → IoT constraints → scope → FR/NFR reads as one narrative; inherited vs Delight scope is explicit (Journey 0 inherited note, FR scope note).

### Dual Audience Effectiveness

**Humans:** Strong executive skim (capabilities table), strong maker/contributor paths (Journeys 0 and 4).  
**LLMs:** Clear `##` structure, numbered FR/NFR, frontmatter classification — excellent for epic/story decomposition.

**Dual Audience Score:** 5/5  

### BMAD PRD Principles Compliance

| Principle | Status | Notes |
|-----------|--------|-------|
| Information density | Met | |
| Measurability | Met | Prior gaps closed; minor perceptual FRs noted informational only |
| Traceability | Met | Journeys 0, 4 and FR41–FR43 close prior gaps |
| Domain awareness | Met | General domain; N/A compliance |
| Zero anti-patterns | Met | |
| Dual audience | Met | |
| Markdown format | Met | |

**Principles met:** 7/7  

### Overall Quality Rating

**Rating:** 5/5 — Excellent  

### Top 3 Improvements (Polish)

1. Add a short **acceptance checklist** (or link to QA doc) for **FR43** so “competent hobbyist” is operationalized in test.  
2. Document **one** recommended visual/photo test protocol for **FR10/FR12** (tearing/animation) to remove remaining subjectivity in QA.  
3. When the partition table gains a second OTA slot row, add a **one-line cross-check** in NFR14 that both slots match (already implied by “smaller slot” rule).

### Summary

**This PRD is:** BMAD-standard, IoT-complete, traceable, and ready for downstream UX/architecture/epic work.

**To make it even stronger:** Operationalize FR43 and FR10/FR12 in test artifacts (optional, not blocking).

## Completeness Validation

### Template Completeness

**Template variables:** 0 unresolved `{placeholder}` / `{{mustache}}` patterns. Literal `{owner}/{repo}` in GitHub API prose is documentation, not an authoring placeholder.

### Content Completeness by Section

| Section | Status | Notes |
|---------|--------|-------|
| Executive Summary | Complete | |
| Success Criteria | Complete | |
| Project Scoping & Phased Development | Complete | Serves Product Scope role |
| User Journeys | Complete | Journeys 0–4 + summary table |
| IoT/Embedded Specific Requirements | Complete | |
| Functional Requirements | Complete | 43 FRs, grouped |
| Non-Functional Requirements | Complete | 21 NFRs |

### Section-Specific Completeness

- **Success criteria measurability:** Strong (business onboarding now tied to Journey 0 + FR43).  
- **User journeys:** Cover first-time setup, owner, guest, contributor.  
- **FRs vs MVP:** Aligned; Growth items explicitly deferred.  
- **NFRs:** Metrics and integration behavior specified.

### Frontmatter Completeness

- `stepsCompleted`: Present  
- `classification`: Present  
- `inputDocuments`: Present  
- `date` in YAML: Present (`2026-04-11`, aligned with document body)

**Overall completeness:** High  

**Critical gaps:** 0  
**Minor gaps:** 0  

**Severity:** Pass  

### Post-validation fix (menu **F**)

- Added `date: '2026-04-11'` to `_bmad-output/planning-artifacts/prd-delight.md` frontmatter for parity with the body and other artifacts.

---

## Final Summary (Step 13)

| Check | Result |
|--------|--------|
| Format | BMAD Standard, 6/6 |
| Information density | Pass |
| Product Brief | N/A |
| Measurability | Pass |
| Traceability | Pass |
| Implementation leakage | Pass |
| Domain compliance | N/A (general) |
| Project-type (IoT) | 100% |
| SMART (FRs) | Pass (informational notes only) |
| Holistic quality | 5/5 — Excellent |
| Completeness | Pass |

**Critical issues:** None  

**Warnings:** None  

**Strengths:** Closed prior gaps (Journey 0, FR41–FR43, tightened FR6/30/33–35, NFR4/14/18/19); clear inherited vs Delight scope; strong IoT section.

**Recommendation:** PRD is in good shape for implementation planning. Optional: add QA checklist hooks for FR43 and visual tests for FR10/FR12.

**Simple fix applied (F):** `date: '2026-04-11'` added to Delight PRD frontmatter; completeness minor gap cleared (see Completeness Validation).

---

**What would you like to do next?**

- **[R] Review Detailed Findings** — Walk this report section by section.  
- **[E] Use Edit Workflow** — `bmad-edit-prd` with this report as context.  
- **[F] Fix Simpler Items** — Ask for any other small tweaks (template text, headers, wording).  
- **[X] Exit** — Validation complete; consider `bmad-help` for next workflow.  


]]></file>
<file id="893ad01d" path="_bmad-output/planning-artifacts/architecture.md" label="ARCHITECTURE"><![CDATA[

---
stepsCompleted: [1, 2, 3, 4, 5, 6, 7, 8]
lastStep: 8
status: 'complete'
completedAt: '2026-04-02'
extensionStarted: '2026-04-11'
extensionPrd: 'prd-foundation.md'
extensionStarted2: '2026-04-11'
extensionPrd2: 'prd-display-system.md'
extensionCompleted2: '2026-04-11'
extensionStatus2: 'complete'
extensionStarted3: '2026-04-11'
extensionPrd3: 'prd-delight.md'
extensionCompleted3: '2026-04-11'
extensionStatus3: 'complete'
inputDocuments: ['_bmad-output/planning-artifacts/prd.md', '_bmad-output/planning-artifacts/ux-design-specification.md', '_bmad-output/planning-artifacts/prd-validation-report.md', '_bmad-output/planning-artifacts/prd-foundation.md', '_bmad-output/planning-artifacts/prd-foundation-validation-report.md', '_bmad-output/planning-artifacts/prd-display-system.md', '_bmad-output/planning-artifacts/prd-display-system-validation-report.md', '_bmad-output/planning-artifacts/prd-delight.md', '_bmad-output/planning-artifacts/prd-delight-validation-report.md', 'docs/index.md', 'docs/project-overview.md', 'docs/architecture.md', 'docs/source-tree-analysis.md', 'docs/component-inventory.md', 'docs/development-guide.md', 'docs/api-contracts.md']
workflowType: 'architecture'
project_name: 'TheFlightWall_OSS-main'
user_name: 'Christian'
date: '2026-04-11'
---

# Architecture Decision Document

_This document builds collaboratively through step-by-step discovery. Sections are appended as we work through each architectural decision together._

## Project Context Analysis

### Requirements Overview

**Functional Requirements (48 FRs across 9 groups):**

| Group | FRs | Architectural Impact |
|-------|-----|---------------------|
| Device Setup & Onboarding | FR1-FR8 | New: captive portal, AP mode, wizard flow, WiFi transition |
| Configuration Management | FR9-FR14 | New: ConfigManager, NVS persistence, hot-reload + reboot paths |
| Display Calibration & Preview | FR15-FR18 | New: web-to-firmware communication for test patterns, canvas data endpoint |
| Flight Data Display | FR19-FR25 | Existing: fetch → enrich → display pipeline (must not regress) |
| Airline Logo Display | FR26-FR29 | New: LogoManager, LittleFS file I/O, ICAO lookup, fallback sprite |
| Responsive Display Layout | FR30-FR33 | New: LayoutEngine with zone calculation and breakpoint detection |
| Logo Management | FR34-FR37 | New: file upload endpoint, validation, LittleFS management |
| System Operations | FR38-FR48 | New: FreeRTOS tasks, mDNS, GPIO handlers, API usage tracking |

**Non-Functional Requirements (architectural drivers):**

| NFR | Target | Architecture Decision Driver |
|-----|--------|------------------------------|
| Hot-reload latency | <1 second | ConfigManager must notify display task directly, no polling |
| Page load time | <3 seconds | Gzipped assets, minimal JS, no external dependencies except Leaflet |
| Time to first flight | <60 seconds | WiFi connect + OAuth + first fetch must be pipelined efficiently |
| Uptime | 30+ days | Heap fragmentation management, watchdog timers, graceful error recovery |
| WiFi recovery | <60 seconds | Auto-reconnect with configurable timeout → AP fallback |
| RAM ceiling | <280KB | Stream LittleFS files, limit concurrent web clients to 2-3, ArduinoJson filter API |
| Flash budget | 4MB total, 500KB headroom | 2MB app + 2MB LittleFS partition. Gzip all web assets. Monitor usage. |
| Concurrent operations | Web server + flight pipeline + display | FreeRTOS task pinning: display Core 0, WiFi/web/API Core 1 |

**UX Specification (architectural implications):**

| UX Decision | Architecture Requirement |
|-------------|------------------------|
| Two HTML pages (wizard + dashboard) | ESPAsyncWebServer serves different content based on WiFi mode |
| Single settings API endpoint | `POST /api/settings` handler parses JSON, applies to ConfigManager, returns applied keys + reboot flag |
| Gzipped static assets | LittleFS stores `.gz` files, ESPAsyncWebServer serves with `Content-Encoding: gzip` |
| Leaflet lazy-load | Dashboard JS dynamically loads Leaflet only when Location section opened |
| mDNS | `ESPmDNS.begin("flightwall")` on WiFi connect |
| WiFi scan in wizard | `WiFi.scanNetworks(true)` async in AP_STA mode, results via `/api/wifi/scan` |
| Toast + hot-reload | Settings POST response drives client-side toast; display settings bypass reboot |
| Triple feedback | Browser canvas (client-side) + LED wall (via firmware) update from same settings change |
| RGB565 logo preview | Client-side only — no architecture impact (FileReader API in browser) |

### Scale & Complexity

- **Primary domain:** IoT/Embedded with companion web UI
- **Complexity level:** Medium — multiple subsystems (web server, config manager, display engine, logo manager, WiFi manager) but single-user, no auth, no database, no cloud backend
- **Estimated new architectural components:** 6 (ConfigManager, WebPortal, WiFiManager, LogoManager, LayoutEngine, DisplayTask)
- **Existing components to preserve:** 5 (FlightDataFetcher, OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, NeoMatrixDisplay)

### Technical Constraints & Dependencies

| Constraint | Impact |
|-----------|--------|
| ESP32 single WiFi radio | AP and STA share hardware — AP_STA for scan, mode switching for setup→operation transition |
| 4MB flash total | Partition carefully: 2MB app + 2MB LittleFS. No OTA in MVP. Every KB of web assets matters. |
| 520KB SRAM | ~280KB usable after stack/WiFi. ESPAsyncWebServer + TLS + JSON parsing compete for heap. |
| FastLED disables interrupts | Display task MUST be on Core 0, WiFi/web on Core 1. Non-negotiable. |
| ESPAsyncWebServer connection limit | 2-3 concurrent clients max. No auto-refreshing dashboard. No WebSocket push. |
| ArduinoJson heap pressure | Use streaming/filter API to reduce per-parse allocation from ~16KB to ~2-4KB |
| NVS write endurance | ~100,000 write cycles per key. Not a concern for config changes, but don't write on every slider drag — debounce. |

### Cross-Cutting Concerns

1. **Configuration lifecycle** — Compile-time defaults → NVS read on boot → runtime changes via web UI → NVS write on save → hot-reload or reboot. Every component that reads config must support this chain.

2. **WiFi mode management** — AP mode (setup) → STA mode (operation) → AP fallback (WiFi failure). Web server behavior changes per mode (wizard vs. dashboard). mDNS only in STA mode.

3. **Concurrent task safety** — Display task (Core 0) and web server + flight pipeline (Core 1) share data structures (current flights, config values). Needs mutex or task-safe data passing.

4. **Flash budget tracking** — Firmware size + web assets + logos must fit in 4MB with headroom. Logo uploads could fill LittleFS. Need usage tracking and upload rejection when full.

5. **Error propagation** — API failures, WiFi drops, and NVS errors must propagate to both the LED display (status messages) and the web UI (System Health page) with specific error information.

## Technology Stack & Foundation

### Existing Stack (Locked)

| Layer | Technology | Version | Notes |
|-------|-----------|---------|-------|
| Language | C++ (Arduino) | C++11 | ESP32 Arduino framework dialect |
| Platform | ESP32 | espressif32 | PlatformIO managed |
| Build | PlatformIO | Latest | `platformio.ini` configuration |
| LED Control | FastLED | ^3.6.0 | WS2812B driver |
| Graphics | Adafruit GFX | ^1.11.9 | Text/shape rendering |
| Matrix | FastLED NeoMatrix | ^1.2 | Tiled matrix abstraction |
| JSON | ArduinoJson | ^7.4.2 | API response parsing |
| Networking | WiFi + HTTPClient | ESP32 core | Built-in Arduino ESP32 |
| TLS | WiFiClientSecure | ESP32 core | HTTPS for API calls |

### New Dependencies (To Add)

| Library | Source | Version | Notes |
|---------|--------|---------|-------|
| ESPAsyncWebServer | `mathieucarbou/ESPAsyncWebServer` | ^3.6.0 | **Use Carbou fork** — actively maintained, fixes memory leaks in original `me-no-dev` repo. Used by ESPHome. |
| AsyncTCP | `mathieucarbou/AsyncTCP` | Latest | **Use Carbou fork** — fixes race conditions that cause crashes under load. Required by ESPAsyncWebServer. |
| LittleFS | ESP32 core | Built-in | Replaces SPIFFS (better wear leveling, directory support). Requires `board_build.filesystem = littlefs` in platformio.ini. |
| ESPmDNS | ESP32 core | Built-in | `flightwall.local` device discovery. Zero-config networking. |
| Leaflet.js | Client-side | ~1.9 | Interactive map. ~25KB gzipped. Lazy-loaded. Stored in `firmware/data/leaflet/` or loaded from CDN. |

**platformio.ini additions:**
```ini
lib_deps =
    mathieucarbou/ESPAsyncWebServer @ ^3.6.0
    ; AsyncTCP pulled automatically as dependency
    fastled/FastLED @ ^3.6.0
    adafruit/Adafruit GFX Library @ ^1.11.9
    marcmerlin/FastLED NeoMatrix @ ^1.2
    bblanchon/ArduinoJson @ ^7.4.2

board_build.filesystem = littlefs
board_build.partitions = custom_partitions.csv
```

### Architecture Pattern

**Extending existing Hexagonal (Ports & Adapters):**
- New components follow the same pattern — abstract interfaces where swappability matters, concrete implementations where it doesn't
- ConfigManager is a new "core" component (not an adapter — it's internal)
- WebPortal is a new adapter (it's an interface to the outside world — the browser)
- WiFiManager is a new adapter (manages the WiFi hardware port)
- LogoManager is a new core component (internal file management)
- LayoutEngine is a new core component (internal display logic)

### Flash Partition Layout

**Custom partition table required** — the default ESP32 partition doesn't provide the 2MB/2MB split. Create `firmware/custom_partitions.csv`:

```
# Name,    Type,  SubType, Offset,   Size
nvs,       data,  nvs,     0x9000,   0x5000
otadata,   data,  ota,     0xe000,   0x2000
app0,      app,   ota_0,   0x10000,  0x200000
spiffs,    data,  spiffs,  0x210000, 0x1F0000
```

**MVP Partition Budget:**
```
4MB Flash Total
├── 2MB — Application (firmware)
│   └── ~1.2MB estimated firmware + ~800KB headroom
└── ~1.9MB — LittleFS
    ├── ~40KB — Web assets (gzipped HTML/JS/CSS)
    ├── ~28KB — Leaflet (gzipped JS + CSS)
    ├── ~100KB — Realistic logo usage (~50 logos at 2KB each)
    └── ~1.7MB — Unused headroom
```

**Phase 2 OTA Partition Strategy (architecture note):**

LittleFS is heavily over-provisioned for MVP. Realistic usage is ~170KB of ~1.9MB available. This slack is the OTA budget for Phase 2:

| Layout | App | OTA | LittleFS | Requires Reflash |
|--------|-----|-----|----------|-----------------|
| **MVP** | 2MB | — | 1.9MB | No (initial) |
| **Phase 2 (generous)** | 1.5MB | 1.5MB | 896KB | Yes |
| **Phase 2 (tight)** | 1.5MB | 1.5MB | 512KB | Yes |

Even the "tight" Phase 2 layout provides 512KB LittleFS — enough for web assets (68KB) + 200 logos (400KB) with headroom. Document this tradeoff now so Phase 2 OTA implementation isn't a surprise.

### Web Assets Location

Web assets live at **`firmware/data/`** (not project root `data/`). This matches the non-standard project structure where all source is under `firmware/`.

```
firmware/
├── data/                    ← LittleFS content, uploaded via `pio run -t uploadfs`
│   ├── style.css.gz
│   ├── common.js.gz
│   ├── wizard.html.gz
│   ├── wizard.js.gz
│   ├── dashboard.html.gz
│   ├── dashboard.js.gz
│   └── leaflet/
│       ├── leaflet.min.js.gz
│       └── leaflet.min.css.gz
├── custom_partitions.csv    ← Custom partition table
├── platformio.ini
├── src/
├── core/
├── adapters/
└── ...
```

**Upload command:** `cd firmware && pio run -t uploadfs`

### Development Workflow

**No change from existing:** PlatformIO build → USB flash → serial monitor. Web assets are pre-gzipped externally (using `gzip -9`) and placed in `firmware/data/`, uploaded to LittleFS via `pio run -t uploadfs`.

**First implementation story** should be: add ESPAsyncWebServer + LittleFS to `platformio.ini`, create custom partition table, verify firmware still builds and runs with the new dependencies, and serve a minimal "hello world" HTML page from LittleFS.

## Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. ConfigManager design — Singleton with category struct getters
2. Inter-task communication — Hybrid (atomic flags for config, queue for flight data)
3. WiFi state machine — Dedicated WiFiManager with callbacks
4. API endpoint design — 11 REST endpoints, consistent JSON response format
5. Component integration — Initialization order, dependency graph, read-only display task
6. Error handling — Centralized SystemStatus registry

**Deferred Decisions (Post-MVP):**
- OTA update mechanism (Phase 2 — requires repartitioning)
- WebSocket push for live dashboard updates (not needed — no auto-refresh)
- Bluetooth functionality (no use case defined)

### Decision 1: ConfigManager — Singleton with Category Struct Getters

**Pattern:** Centralized singleton, initialized first, accessed by all components. Config values grouped into category structs for clean API surface and efficient access.

```cpp
struct DisplayConfig {
    uint8_t brightness;
    uint8_t textColorR, textColorG, textColorB;
};

struct LocationConfig {
    double centerLat, centerLon, radiusKm;
};

struct HardwareConfig {
    uint8_t tilesX, tilesY, tilePixels, displayPin;
    uint8_t originCorner, scanDirection, zigzag;
};

struct TimingConfig {
    uint16_t fetchIntervalSeconds, displayCycleSeconds;
};

struct NetworkConfig {
    String wifiSsid, wifiPassword;
    String openSkyClientId, openSkyClientSecret;
    String aeroApiKey;
};

class ConfigManager {
public:
    static void init();
    static DisplayConfig getDisplay();
    static LocationConfig getLocation();
    static HardwareConfig getHardware();
    static TimingConfig getTiming();
    static NetworkConfig getNetwork();
    static ApplyResult applyJson(const JsonObject& settings);
    static void schedulePersist(uint16_t delayMs = 2000);
    static void onChange(std::function<void()> callback);
    static bool requiresReboot(const char* key);
};

struct ApplyResult {
    std::vector<String> applied;
    bool rebootRequired;
};
```

**Config value chain:** Compile-time default → NVS stored value → RAM cache (struct fields)

**NVS Write Debouncing:**
- `applyJson()` updates RAM cache instantly (hot-reload works immediately)
- `schedulePersist(2000)` queues NVS write after 2-second quiet period
- If user drags slider through 50 values in 3 seconds, NVS gets one write at the end
- Reboot-required keys bypass debouncing — persist to NVS immediately before restart

**Reboot-required keys:** `wifi_ssid`, `wifi_password`, `opensky_client_id`, `opensky_client_secret`, `aeroapi_key`, `display_pin`
**Hot-reload keys:** `brightness`, `text_color_r/g/b`, `fetch_interval`, `display_cycle`, `tiles_x`, `tiles_y`, `center_lat`, `center_lon`, `radius_km`, wiring flags

### Decision 2: Inter-Task Communication — Hybrid

**Config changes:** Atomic flag signals display task to re-read from ConfigManager
```cpp
std::atomic<bool> configChanged{false};

// Core 1: web server applies setting
ConfigManager::applyJson(settings);
configChanged.store(true);

// Core 0: display task checks each frame
if (configChanged.exchange(false)) {
    displayConfig = ConfigManager::getDisplay();
    hardwareConfig = ConfigManager::getHardware();
}
```

**Flight data:** FreeRTOS queue with overwrite semantics
```cpp
QueueHandle_t flightQueue;

// Core 1: flight pipeline completes
xQueueOverwrite(flightQueue, &enrichedFlights);

// Core 0: display task reads latest
FlightData flights;
if (xQueuePeek(flightQueue, &flights, 0) == pdTRUE) {
    renderFlights(flights);
}
```

**Why hybrid:** Config changes are lightweight signals (flag + struct copy). Flight data needs proper thread-safe transfer (queue). `xQueueOverwrite` ensures display always gets latest data.

### Decision 3: WiFi State Machine — WiFiManager

**States:**
```cpp
enum WiFiState {
    WIFI_AP_SETUP,
    WIFI_CONNECTING,
    WIFI_STA_CONNECTED,
    WIFI_STA_RECONNECTING,
    WIFI_AP_FALLBACK
};
```

**Transitions:**
```
POWER ON → [Has WiFi config?]
         No → AP_SETUP
         Yes → CONNECTING → Success → STA_CONNECTED
                          → Timeout → AP_FALLBACK
STA_CONNECTED → WiFi lost → STA_RECONNECTING → Reconnects → STA_CONNECTED
                                               → Timeout → AP_FALLBACK
```

**GPIO overrides:**
- Short press: show IP on LEDs for 5 seconds (no state change)
- Long press during boot: force AP_SETUP regardless of saved config

**Callbacks:** WebPortal, display task, and FlightDataFetcher register for state change notifications.

### Decision 4: Web API Endpoints

| Method | Endpoint | Purpose | Reboot |
|--------|----------|---------|--------|
| `GET` | `/` | wizard.html (AP) or dashboard.html (STA) | No |
| `GET` | `/api/status` | System health JSON | No |
| `GET` | `/api/settings` | All current config values | No |
| `POST` | `/api/settings` | Apply partial config update | Maybe |
| `GET` | `/api/wifi/scan` | Async WiFi scan results | No |
| `POST` | `/api/reboot` | Save + reboot | Yes |
| `POST` | `/api/reset` | Factory reset — erase NVS | Yes |
| `GET` | `/api/logos` | List uploaded logos | No |
| `POST` | `/api/logos` | Upload logo .bin (multipart) | No |
| `DELETE` | `/api/logos/:name` | Delete specific logo | No |
| `GET` | `/api/layout` | Current zone layout (initial load only) | No |

**`/api/layout` note:** Used for initial dashboard load only. During live editing, canvas calculates zones client-side using the shared zone algorithm.

**Response envelope:**
```json
{ "ok": true, "data": { ... } }
{ "ok": false, "error": "human-readable message", "code": "MACHINE_CODE" }
```

### Decision 5: Component Integration

**Dependency Graph:**
```
ConfigManager ← (everything depends on this)
SystemStatus ← (everything reports to this)
WiFiManager ← WebPortal
LogoManager ← WebPortal, DisplayTask
LayoutEngine ← DisplayTask, dashboard.js (shared algorithm)
FlightDataFetcher ← main loop
    ├── OpenSkyFetcher
    ├── AeroAPIFetcher
    └── FlightWallFetcher
DisplayTask (Core 0, read-only):
    ├── ConfigManager (atomic flag + struct copy)
    ├── flightQueue (FreeRTOS queue)
    ├── LogoManager (ICAO lookup)
    ├── LayoutEngine (zone positions)
    └── SystemStatus (error display)
```

**Initialization order:**
```
1. ConfigManager::init()
2. SystemStatus::init()
3. LogoManager::init()
4. LayoutEngine::init()
5. WiFiManager::init()
6. WebPortal::init()
7. FlightDataFetcher::init()
8. xTaskCreatePinnedToCore(displayTask, "display", 8192, NULL, 1, NULL, 0)
9. loop() runs flight fetch cycle on Core 1
```

**Rules:**
- ConfigManager initializes first — all others depend on it
- DisplayTask is read-only — never writes config, never calls APIs
- WebPortal is the only write path from the browser
- Existing adapters gain ConfigManager dependency but keep interface contracts unchanged

### Decision 6: Error Handling — SystemStatus Registry

```cpp
class SystemStatus {
public:
    static void init();
    static void set(Subsystem sys, StatusLevel level, const String& message);
    static SubsystemStatus get(Subsystem sys);
    static JsonObject toJson();
    static void setMetric(const String& key, const String& value);
    static String getMetric(const String& key);
};
```

**Subsystem health:**

| Subsystem | OK | Warning | Error |
|-----------|-----|---------|-------|
| WIFI | "Connected, -52 dBm" | "Weak signal, -78 dBm" | "Disconnected, retrying..." |
| OPENSKY | "Authenticated ✓" | "Token expiring in 5m" | "401 Unauthorized — check credentials" |
| AEROAPI | "Connected ✓" | "429 rate limited, retry 30s" | "503 Service unavailable" |
| CDN | "Reachable ✓" | — | "Unreachable — using ICAO codes" |
| NVS | "OK" | — | "Write failed — config may not persist" |
| LITTLEFS | "847KB / 1.9MB used" | "90%+ full" | "Mount failed" |

**API Call Counter — RAM + Hourly NVS Sync:**
- Running count in RAM, synced to NVS once per hour and on graceful reboot
- Loses at most 1 hour of data on unexpected reboot — acceptable for budget indicator
- Resets on 1st of month

### Shared Zone Calculation Algorithm

**Critical:** Implemented identically in C++ (LayoutEngine) and JavaScript (dashboard.js). If they diverge, preview won't match LEDs.

```
INPUT: tilesX, tilesY, tilePixels
COMPUTE:
    matrixWidth  = tilesX * tilePixels
    matrixHeight = tilesY * tilePixels

    if matrixHeight < 32:      mode = "compact"
    else if matrixHeight < 48: mode = "full"
    else:                      mode = "expanded"

    logoZone     = { x: 0, y: 0, w: matrixHeight, h: matrixHeight }
    flightZone   = { x: matrixHeight, y: 0, w: matrixWidth - matrixHeight, h: floor(matrixHeight/2) }
    telemetryZone = { x: matrixHeight, y: floor(matrixHeight/2), w: matrixWidth - matrixHeight, h: matrixHeight - floor(matrixHeight/2) }
OUTPUT: mode, logoZone, flightZone, telemetryZone
```

**Test vectors:**

| Config | Matrix | Mode | Logo | Flight Card | Telemetry |
|--------|--------|------|------|-------------|-----------|
| 10x2 @ 16px | 160x32 | full | 0,0 → 32x32 | 32,0 → 128x16 | 32,16 → 128x16 |
| 5x2 @ 16px | 80x32 | full | 0,0 → 32x32 | 32,0 → 48x16 | 32,16 → 48x16 |
| 12x3 @ 16px | 192x48 | expanded | 0,0 → 48x48 | 48,0 → 144x24 | 48,24 → 144x24 |
| 10x1 @ 16px | 160x16 | compact | 0,0 → 16x16 | 16,0 → 144x8 | 16,8 → 144x8 |

### Decision Impact — Implementation Sequence

1. ConfigManager + SystemStatus (foundation)
2. WiFiManager (enables web server and API calls)
3. WebPortal + API routes (enables browser interaction)
4. LogoManager (LittleFS file management)
5. LayoutEngine (zone calculation — shared algorithm in C++)
6. DisplayTask refactor (FreeRTOS, new layout, logos)
7. Existing adapter updates (ConfigManager integration)
8. Web assets (shared zone algorithm in JS, build dashboard)

## Implementation Patterns & Consistency Rules

### Existing Codebase Conventions (All New Code Must Follow)

The existing firmware establishes conventions that new components must match. These are rules derived from working code.

### Naming Patterns

**C++ Classes:** `PascalCase`
- Existing: `FlightDataFetcher`, `NeoMatrixDisplay`, `OpenSkyFetcher`, `AeroAPIFetcher`
- New: `ConfigManager`, `WiFiManager`, `WebPortal`, `LogoManager`, `LayoutEngine`, `SystemStatus`

**C++ Methods:** `camelCase`
- Existing: `fetchFlights()`, `fetchStateVectors()`, `displayMessage()`, `showLoading()`
- New: `applyJson()`, `schedulePersist()`, `getDisplay()`, `setState()`

**C++ Member Variables:** `_camelCase` (underscore prefix for private members)
- Existing: `_matrix`, `_leds`, `_matrixWidth`, `_currentFlightIndex`, `_stateFetcher`

**C++ Structs:** `PascalCase` name with `snake_case` fields
- Existing: `FlightInfo { String ident_icao; String operator_code; }`
- New: `DisplayConfig { uint8_t brightness; uint8_t text_color_r; }`
- **Rule:** Struct fields use `snake_case` to match NVS keys and JSON field names. One convention across all three layers — zero mental translation.

**Constants:** `UPPER_SNAKE_CASE` | **Namespaces:** `PascalCase` | **Files:** `PascalCase.h/.cpp` | **Headers:** `#pragma once`

### Config Structs (Corrected to snake_case Fields)

```cpp
struct DisplayConfig {
    uint8_t brightness;
    uint8_t text_color_r, text_color_g, text_color_b;
};

struct LocationConfig {
    double center_lat, center_lon, radius_km;
};

struct HardwareConfig {
    uint8_t tiles_x, tiles_y, tile_pixels, display_pin;
    uint8_t origin_corner, scan_dir, zigzag;
};

struct TimingConfig {
    uint16_t fetch_interval, display_cycle;
};

struct NetworkConfig {
    String wifi_ssid, wifi_password;
    String opensky_client_id, opensky_client_secret;
    String aeroapi_key;
};
```

### NVS Key Naming

**Convention:** `snake_case`, max 15 characters. Abbreviation rule: if natural name exceeds 15 chars, document abbreviation in `ConfigManager.h`:

```cpp
// NVS key abbreviations (15-char limit):
// os_client_id   = opensky_client_id
// os_client_sec  = opensky_client_secret
// scan_dir       = scan_direction
```

| NVS Key | Type | Default | Category |
|---------|------|---------|----------|
| `brightness` | uint8 | 5 | display |
| `text_color_r` | uint8 | 255 | display |
| `text_color_g` | uint8 | 255 | display |
| `text_color_b` | uint8 | 255 | display |
| `center_lat` | double | 37.7749 | location |
| `center_lon` | double | -122.4194 | location |
| `radius_km` | double | 10.0 | location |
| `tiles_x` | uint8 | 10 | hardware |
| `tiles_y` | uint8 | 2 | hardware |
| `tile_pixels` | uint8 | 16 | hardware |
| `display_pin` | uint8 | 25 | hardware |
| `origin_corner` | uint8 | 0 | hardware |
| `scan_dir` | uint8 | 0 | hardware |
| `zigzag` | uint8 | 0 | hardware |
| `fetch_interval` | uint16 | 30 | timing |
| `display_cycle` | uint16 | 3 | timing |
| `wifi_ssid` | string | "" | network |
| `wifi_password` | string | "" | network |
| `os_client_id` | string | "" | network |
| `os_client_sec` | string | "" | network |
| `aeroapi_key` | string | "" | network |
| `api_calls` | uint32 | 0 | metrics |
| `api_month` | uint8 | 0 | metrics |

**NVS namespace:** `"flightwall"`

### API Call Counter — Monthly Reset Strategy

**Primary (NTP):** On WiFi STA connect, `configTime(0, 0, "pool.ntp.org")` syncs time. Compare current month to `api_month` in NVS — if different, reset `api_calls` to 0 and update `api_month`.

**Fallback (manual):** If NTP fails, counter continues. Dashboard shows "Reset API Counter" button for manual reset.

### API JSON Field Names

**Convention:** `snake_case` in all JSON — matches struct fields and NVS keys.

```json
{ "brightness": 25, "text_color_r": 200, "fetch_interval": 120 }
```

### Logging Pattern — Compile-Time Log Levels

**File:** `utils/Log.h`

```cpp
#pragma once

#ifndef LOG_LEVEL
#define LOG_LEVEL 2  // Default: info
#endif

#define LOG_E(tag, msg) do { if (LOG_LEVEL >= 1) Serial.println("[" tag "] ERROR: " msg); } while(0)
#define LOG_I(tag, msg) do { if (LOG_LEVEL >= 2) Serial.println("[" tag "] " msg); } while(0)
#define LOG_V(tag, msg) do { if (LOG_LEVEL >= 3) Serial.println("[" tag "] " msg); } while(0)
```

**platformio.ini:** `-D LOG_LEVEL=3` for development, `-D LOG_LEVEL=1` for production.
**Tag format:** String literal matching class name. Macros compile to nothing at level 0.

### Error Handling Pattern

Boolean return + output parameter (matches existing codebase). Plus SystemStatus reporting and log macros:

```cpp
if (!fetchStateVectors(...)) {
    SystemStatus::set(OPENSKY, ERROR, "Fetch failed — HTTP " + String(httpCode));
    LOG_E("OpenSky", ("Fetch failed — HTTP " + String(httpCode)).c_str());
    return false;
}
```

**No exceptions.** No `try/catch`.

### Include Pattern

System/Arduino first → Library second → Project last. Forward declarations where possible.

### Memory Management

`String` for dynamic text, `const char*` for constants. Avoid `new`/`delete`. Stream LittleFS files.

### Web Asset Patterns

**JS Fetch Error Handling (mandatory):**

```javascript
async function applySettings(data) {
    try {
        const res = await fetch('/api/settings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });
        const json = await res.json();
        if (!json.ok) { showToast(json.error, 'error'); return null; }
        showToast('Settings saved', 'success');
        return json;
    } catch (err) {
        showToast('Connection failed', 'error');
        return null;
    }
}
```

**Rule:** Every `fetch()` call must check `json.ok` and call `showToast()` on failure.

### Enforcement Guidelines

**All AI agents implementing stories MUST:**

1. Follow naming conventions — check existing files before writing new code
2. Use `snake_case` for struct fields, NVS keys, and JSON fields
3. Use `#pragma once` in all headers
4. Use boolean return + output param for fallible operations
5. Report user-visible errors via `SystemStatus::set()`
6. Log via `LOG_E`/`LOG_I`/`LOG_V` from `utils/Log.h` — never raw `Serial.println`
7. Use `String` for dynamic text, `const char*` for constants
8. Stream LittleFS files — never buffer entire files in RAM
9. Place new classes in appropriate directory: `core/`, `adapters/`, `interfaces/`, `models/`, `utils/`
10. Every JS `fetch()` must check `json.ok` and handle failure with `showToast()`
11. Document NVS key abbreviations in `ConfigManager.h` if key exceeds 15 characters

## Project Structure & Boundaries

### Complete Project Directory Structure

```
firmware/
├── platformio.ini                     # UPDATED: new libs + partition + littlefs
├── custom_partitions.csv              # NEW: 2MB app + 1.9MB LittleFS
│
├── src/
│   └── main.cpp                       # UPDATED: init sequence + FreeRTOS task creation
│
├── core/
│   ├── FlightDataFetcher.h            # EXISTING (updated: ConfigManager dependency)
│   ├── FlightDataFetcher.cpp          # EXISTING (updated: ConfigManager dependency)
│   ├── ConfigManager.h                # NEW: singleton, category structs, NVS, debounce
│   ├── ConfigManager.cpp              # NEW
│   ├── LayoutEngine.h                 # NEW: zone calculation (shared algorithm)
│   ├── LayoutEngine.cpp               # NEW
│   ├── LogoManager.h                  # NEW: LittleFS CRUD, ICAO lookup, fallback
│   ├── LogoManager.cpp                # NEW
│   ├── SystemStatus.h                 # NEW: subsystem health registry
│   └── SystemStatus.cpp               # NEW
│
├── adapters/
│   ├── OpenSkyFetcher.h               # EXISTING (updated: ConfigManager)
│   ├── OpenSkyFetcher.cpp             # EXISTING (updated: ConfigManager)
│   ├── AeroAPIFetcher.h               # EXISTING (updated: ConfigManager)
│   ├── AeroAPIFetcher.cpp             # EXISTING (updated: ConfigManager)
│   ├── FlightWallFetcher.h            # EXISTING (unchanged)
│   ├── FlightWallFetcher.cpp          # EXISTING (unchanged)
│   ├── NeoMatrixDisplay.h             # EXISTING (updated: LayoutEngine, LogoManager)
│   ├── NeoMatrixDisplay.cpp           # EXISTING (updated: zones, logos, telemetry)
│   ├── WebPortal.h                    # NEW: ESPAsyncWebServer, API routes
│   ├── WebPortal.cpp                  # NEW
│   ├── WiFiManager.h                  # NEW: state machine, AP/STA, mDNS, scan
│   └── WiFiManager.cpp                # NEW
│
├── interfaces/
│   ├── BaseDisplay.h                  # EXISTING (unchanged)
│   ├── BaseFlightFetcher.h            # EXISTING (unchanged)
│   └── BaseStateVectorFetcher.h       # EXISTING (unchanged)
│
├── models/
│   ├── FlightInfo.h                   # EXISTING (unchanged)
│   ├── StateVector.h                  # EXISTING (unchanged)
│   └── AirportInfo.h                  # EXISTING (unchanged)
│
├── config/
│   ├── APIConfiguration.h             # EXISTING — compile-time defaults (see migration rule)
│   ├── WiFiConfiguration.h            # EXISTING — compile-time defaults
│   ├── UserConfiguration.h            # EXISTING — compile-time defaults
│   ├── HardwareConfiguration.h        # EXISTING — compile-time defaults
│   └── TimingConfiguration.h          # EXISTING — compile-time defaults
│
├── utils/
│   ├── GeoUtils.h                     # EXISTING (unchanged)
│   └── Log.h                          # NEW: compile-time log level macros
│
├── data/                              # NEW: LittleFS initial image → pio run -t uploadfs
│   ├── style.css.gz
│   ├── common.js.gz
│   ├── wizard.html.gz
│   ├── wizard.js.gz
│   ├── dashboard.html.gz
│   ├── dashboard.js.gz
│   └── leaflet/
│       ├── leaflet.min.js.gz
│       └── leaflet.min.css.gz
│
└── test/
    ├── test_config_manager/
    │   └── test_main.cpp
    └── test_layout_engine/
        └── test_main.cpp
```

### Config Header Migration Rule

Existing `config/` headers remain in place as compile-time default value source. **Only `ConfigManager.cpp` includes them.** Adapters migrate from direct config header reads to ConfigManager struct getters:

```cpp
// BEFORE: #include "config/APIConfiguration.h"
//         APIConfiguration::OPENSKY_CLIENT_ID
// AFTER:  #include "core/ConfigManager.h"
//         ConfigManager::getNetwork().opensky_client_id
```

### Runtime LittleFS Layout

```
LittleFS at runtime:
├── *.css.gz, *.html.gz, *.js.gz   ← from firmware/data/ (build-time)
├── leaflet/*.gz                     ← from firmware/data/ (build-time)
└── logos/                           ← created at runtime by LogoManager::init()
    ├── UAL.bin                      ← user-uploaded
    └── ...
```

Web assets at root, logos in `/logos/` subdirectory. LogoManager creates directory on init.

### Architectural Boundaries

**Adapter Boundary:** Browser ↔ WebPortal, WiFi Hardware ↔ WiFiManager, LEDs ↔ NeoMatrixDisplay, APIs ↔ Fetchers
**Core Boundary:** ConfigManager (config state), LayoutEngine (zones), LogoManager (files), SystemStatus (health), FlightDataFetcher (pipeline)
**Data Boundary:** Core 1 → Core 0 via atomic flag + FreeRTOS queue. Core 0 is read-only.

### NeoMatrixDisplay Refactoring Note

Do not split preemptively. If file exceeds ~400 lines after updates, consider extracting `FlightRenderer` for rendering logic, keeping NeoMatrixDisplay focused on hardware.

### Requirements to Structure Mapping

| Epic/Group | Key Files |
|-----------|-----------|
| Setup & Onboarding (FR1-8) | WiFiManager, WebPortal, wizard.html/js |
| Config Management (FR9-14) | ConfigManager, WebPortal, dashboard.html/js |
| Calibration & Preview (FR15-18) | WebPortal, LayoutEngine, dashboard.js |
| Flight Data (FR19-25) | FlightDataFetcher, NeoMatrixDisplay |
| Logo Display (FR26-29) | LogoManager, NeoMatrixDisplay |
| Responsive Layout (FR30-33) | LayoutEngine, NeoMatrixDisplay |
| Logo Management (FR34-37) | LogoManager, WebPortal, dashboard.js |
| System Operations (FR38-48) | SystemStatus, WiFiManager, main.cpp |

### Testing Expectations

**Unit tests required:** ConfigManager (NVS, defaults, applyJson, debounce), LayoutEngine (4 test vectors from shared algorithm)
**Not required:** Hardware-dependent adapters (WiFi, Web, Display, API fetchers)

### External Integration Points

| Service | Auth | Protocol |
|---------|------|----------|
| OpenSky Network | OAuth2 | HTTPS REST |
| FlightAware AeroAPI | API key | HTTPS REST |
| FlightWall CDN | None | HTTPS GET |
| OpenStreetMap | None | HTTPS tiles (client-side) |
| NTP | None | UDP |

### Data Flow

Boot → ConfigManager loads NVS → WiFiManager starts AP/STA → WebPortal serves UI → FlightDataFetcher runs fetch cycle (Core 1) → DisplayTask renders (Core 0) → User changes setting → POST /api/settings → ConfigManager updates RAM → atomic flag → display re-reads → NVS persists after 2s debounce

## Architecture Validation Results

### Coherence Validation ✅

**Decision Compatibility:** All technology choices compatible. mathieucarbou/ESPAsyncWebServer, LittleFS, ArduinoJson, and FastLED coexist on ESP32 Arduino without known conflicts.

**Pattern Consistency:** `snake_case` flows consistently across struct fields → NVS keys → JSON fields. Logging macros, error handling, and include patterns are uniform. No contradictions.

**Structure Alignment:** Directory structure maps directly to hexagonal architecture. Config migration rule is clear. New files in correct directories.

### Requirements Coverage ✅

**All 48 FRs covered.** Every functional requirement maps to at least one architectural component with a clear file location.

**All NFRs addressed.** Performance (hot-reload <1s, page load <3s), reliability (30+ day uptime, WiFi recovery), resource constraints (RAM, flash), concurrency (FreeRTOS dual-core) all have explicit architectural support.

### Implementation Readiness ✅

**Decision Completeness:** 6 core decisions with code examples, interfaces, and rationale. Versions pinned.
**Structure Completeness:** Every file listed as EXISTING/NEW/UPDATED. Migration rules documented.
**Pattern Completeness:** 11 enforcement rules, NVS key table, API endpoint table, JSON examples, JS fetch pattern, logging macros.

### Gap Analysis

| Priority | Gap | Resolution |
|----------|-----|-----------|
| Minor | Fallback airplane sprite storage | PROGMEM constant in LogoManager.h — 32x32 RGB565 array (~2KB flash) |
| Minor | GPIO reset pin not in HardwareConfig | Add `reset_pin` to HardwareConfig, default GPIO 0 (BOOT button), NVS key `reset_pin` |
| Minor | Watchdog timer | ESP32 task watchdog enabled by default. Rule: no task blocks >5 seconds |

No critical gaps.

### Architecture Completeness Checklist

- [x] Project context analyzed (48 FRs, NFRs, UX spec)
- [x] Scale assessed (Medium — IoT with web UI)
- [x] Constraints identified (flash, RAM, single radio, interrupt conflict)
- [x] Cross-cutting concerns mapped (config, WiFi, tasks, flash, errors)
- [x] Technology stack locked with specific forks and versions
- [x] Custom partition table specified
- [x] 6 core architectural decisions documented
- [x] Implementation patterns extracted from existing codebase
- [x] NVS key table with abbreviation rules
- [x] Logging macros with compile-time levels
- [x] 11 enforcement guidelines for AI agents
- [x] Complete directory tree (22 existing + 22 new files)
- [x] Architectural boundaries defined
- [x] Requirements mapped to files
- [x] Runtime LittleFS layout documented
- [x] Testing expectations set
- [x] Shared zone algorithm with test vectors

### Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION
**Confidence Level:** High

**Key Strengths:**
- Clean extension of existing hexagonal architecture — no breaking changes
- Comprehensive NVS key table eliminates config ambiguity
- Shared zone algorithm with test vectors ensures canvas/LED parity
- snake_case consistency across struct/NVS/JSON eliminates naming drift
- Party mode reviews caught critical issues (fork choice, debouncing, naming, logging)

**Areas for Future Enhancement:**
- ~~OTA firmware updates (Phase 2 — partition strategy documented)~~ → Addressed in Foundation Release below
- WebSocket push (deferred — not needed for set-and-forget UI)
- NeoMatrixDisplay split (if exceeds ~400 lines)

### Implementation Handoff

**First Implementation Priority:**
1. Add mathieucarbou/ESPAsyncWebServer + LittleFS to platformio.ini
2. Create custom_partitions.csv
3. Create utils/Log.h
4. Create core/ConfigManager.h/.cpp with NVS + category structs + fallbacks
5. Create core/SystemStatus.h/.cpp
6. Verify firmware builds and existing flight pipeline still works

---

# Foundation Release — Architecture Extension

_Extends the MVP architecture with OTA firmware updates, night mode/brightness scheduling, and onboarding polish. All MVP architectural decisions remain in effect — this section adds new decisions and updates existing ones where the Foundation PRD introduces changes._

## Foundation Release — Project Context Analysis

### Requirements Overview

**Functional Requirements (37 FRs across 6 groups):**

| Group | FRs | Architectural Impact |
|-------|-----|---------------------|
| OTA Firmware Updates | FR1-FR11 | **Major:** Partition table change (dual-OTA), new upload endpoint in WebPortal, self-check + rollback in boot sequence, `Update.h` integration |
| Settings Migration | FR12-FR14 | **Minor:** New `/api/settings/export` GET endpoint, client-side JSON import in wizard (~40 lines JS, no new server endpoint) |
| Night Mode & Brightness | FR15-FR24 | **Medium:** NTP integration via `configTzTime()`, 5 new ConfigManager keys, time-based brightness scheduler in main loop, IANA-to-POSIX timezone mapping |
| Onboarding Polish | FR25-FR29 | **Minor:** Wizard Step 6 extension — reuses existing calibration pattern and canvas preview endpoints |
| Dashboard UI | FR30-FR34 | **Minor:** New Firmware card and Night Mode card, rollback detection toast |
| System & Resilience | FR35-FR37 | **Medium:** Dual-OTA partition coexistence, OTA failure recovery path, new NVS config keys |

**Non-Functional Requirements (architectural drivers):**

| NFR | Target | Architecture Decision Driver |
|-----|--------|------------------------------|
| OTA upload speed | ≤30s for 1.5MB binary | Stream chunks via `Update.write()` — no RAM buffering |
| NTP sync time | ≤10s after WiFi connect | `configTzTime()` with pool.ntp.org + time.nist.gov |
| Brightness hot-reload | ≤1s (unchanged from MVP) | Scheduler overrides brightness via existing ConfigManager hot-reload path |
| Firmware binary size | ≤1.5MB | Pre-implementation size gate at 1.3MB; reduced from 2MB MVP budget |
| LittleFS free space | ≥500KB after assets + logos | 896KB total - ~278KB used = ~618KB free |
| OTA self-check window | ≤30s boot-to-mark-valid | Sequential gate: WiFi (15s) + web server (2s) + buffer (13s) |
| Uptime with night mode | 30+ days | Non-blocking time comparison per loop iteration — no I/O, no blocking |
| Night mode scheduler overhead | ≤1% main loop impact | Single `localtime_r()` + integer comparison per iteration |

### Scale & Complexity

- **Scope:** Narrower than MVP — fewer new components, but partition change has wide blast radius
- **New architectural components:** 0 new classes (OTA upload in WebPortal, self-check in main.cpp, scheduler in main loop)
- **Updated components:** ConfigManager (5 new keys), WebPortal (2 new endpoints), main.cpp (self-check + scheduler), custom_partitions.csv, wizard HTML/JS
- **Complexity driver:** OTA self-check is the highest-risk code path — boot-time safety mechanism that must be bulletproof

### Technical Constraints (Foundation-Specific)

| Constraint | Impact |
|-----------|--------|
| Dual-OTA partitions require 3MB for two app slots | LittleFS shrinks from ~2MB to ~896KB (56% reduction) |
| Firmware binary must fit in 1.5MB partition | Tighter than MVP's 2MB budget — pre-implementation size gate required |
| One-time USB reflash required for partition migration | All NVS + LittleFS data erased — settings export mitigates |
| No built-in IANA-to-POSIX mapping on ESP32 | Need explicit mapping decision: browser-side, PROGMEM table, or LittleFS file |
| `Update.h` streams from ESPAsyncWebServer multipart callback | Must verify Carbou fork's `onUpload` callback chunk integration with `Update.write()` |
| NTP requires WiFi STA mode | Night mode schedule inactive in AP mode and until first NTP sync |
| ESP32 RTC drift ~5 ppm | ~0.4s/day — NTP re-sync every 24h keeps schedule accurate |

### Cross-Cutting Concerns (Foundation Additions)

1. **Flash budget governance** — LittleFS reduced 56%. Firmware binary capped at 1.5MB. Both limits must be tracked and enforced. Pre-implementation size gate at 1.3MB.

2. **OTA resilience chain** — Upload validation → stream-to-flash → reboot → self-check → mark valid OR watchdog rollback. This is a new failure recovery path that integrates with WiFiManager (WiFi connect check) and WebPortal (server start check) during boot.

3. **Time management (linear dependency, not cross-cutting)** — WiFi STA → `configTzTime()` → `localtime_r()` valid → scheduler compares time → overrides brightness via ConfigManager. NTP failure is *contained* — only the scheduler is affected. No cascade to other subsystems.

4. **Settings portability** — Export (`/api/settings/export`) bridges the one-time partition migration. Import is client-side only (JSON file → pre-fill wizard fields). No new server-side import endpoint.

### Existing Architecture Unchanged

The following MVP decisions remain in full effect with no modifications:

- Hexagonal (Ports & Adapters) pattern
- ConfigManager singleton with category struct getters
- Inter-task communication (atomic flags for config, FreeRTOS queue for flights)
- WiFi state machine (WiFiManager)
- SystemStatus error registry
- FreeRTOS dual-core task pinning (display Core 0, everything else Core 1)
- All naming conventions, NVS key patterns, logging macros, and enforcement rules
- Web asset patterns (fetch + json.ok + showToast)

## Foundation Release — Technology Stack

**Starter Template Evaluation: N/A (Brownfield Extension)**

No new external dependencies. All Foundation capabilities use libraries already in the ESP32 Arduino core:

| Library | Source | Purpose |
|---------|--------|---------|
| `Update.h` | ESP32 Arduino core | OTA flash write (stream chunks to inactive partition) |
| `esp_ota_ops.h` | ESP-IDF | `esp_ota_mark_app_valid_cancel_rollback()`, partition management |
| `time.h` / `configTzTime()` | ESP32 Arduino core | NTP sync with POSIX timezone, `localtime_r()` |
| `esp_sntp.h` | ESP-IDF | NTP sync status callback |

The MVP dependency stack (ESPAsyncWebServer Carbou fork, AsyncTCP, FastLED, ArduinoJson, LittleFS, ESPmDNS) remains unchanged.

## Foundation Release — Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. Partition table — dual OTA layout with exact offsets and sizes
2. OTA handler architecture — upload in WebPortal, self-check in main.cpp
3. OTA self-check — WiFi-OR-timeout strategy with 60-second window
4. IANA-to-POSIX timezone mapping — browser-side JS table
5. Night mode scheduler — non-blocking main loop check via ConfigManager

**Deferred Decisions (Post-Foundation):**
- OTA pull from GitHub releases (Delight Release)
- OTA firmware signing (Phase 3 — public distribution)
- Sunrise/sunset-aware brightness scheduling (Phase 2)

### Decision F1: Partition Table — Dual OTA Layout

**Updated `firmware/custom_partitions.csv`:**

```
# Name,    Type,  SubType, Offset,    Size,      Notes
nvs,       data,  nvs,     0x9000,    0x5000,    # 20KB — config persistence
otadata,   data,  ota,     0xE000,    0x2000,    # 8KB — OTA partition tracking
app0,      app,   ota_0,   0x10000,   0x180000,  # 1.5MB — active firmware
app1,      app,   ota_1,   0x190000,  0x180000,  # 1.5MB — OTA staging
spiffs,    data,  spiffs,  0x310000,  0xF0000,   # 960KB — LittleFS
```

**Flash Budget (Foundation):**

```
4MB Flash Total
├── 20KB  — NVS (config persistence)
├── 8KB   — OTA data (partition tracking)
├── 1.5MB — app0 (active firmware)
├── 1.5MB — app1 (OTA staging)
└── 960KB — LittleFS
    ├── ~80KB  — Web assets (gzipped HTML/JS/CSS)
    ├── ~198KB — 99 bundled logos (~2KB each)
    └── ~682KB — Free headroom
```

**Breaking change from MVP:** LittleFS shrinks from ~1.9MB to ~960KB. One-time USB reflash required — erases NVS + LittleFS. Settings export mitigates reconfiguration.

**Pre-implementation gate:** Measure current firmware binary size. If exceeding 1.3MB, optimize before adding Foundation code (disable Bluetooth with `CONFIG_BT_ENABLED=0`, strip unused library features).

### Decision F2: OTA Handler Architecture — WebPortal + main.cpp

**No new class.** OTA is split across two existing locations:

| Concern | Location | Implementation |
|---------|----------|---------------|
| Upload endpoint (`POST /api/ota/upload`) | `WebPortal.cpp` | ESPAsyncWebServer `onUpload` handler |
| Binary validation (size + magic byte) | `WebPortal.cpp` | Check `Content-Length` vs partition size, first byte == `0xE9` |
| Stream-to-flash | `WebPortal.cpp` | `Update.begin()` → `Update.write()` per chunk → `Update.end()` |
| Abort on failure | `WebPortal.cpp` | `Update.abort()` on stream error or WiFi interruption |
| Progress reporting | `WebPortal.cpp` | Bytes written / total sent via upload handler callback |
| Reboot after success | `WebPortal.cpp` | `ESP.restart()` after successful `Update.end(true)` |
| Self-check + mark valid | `main.cpp` | Boot sequence, after WiFiManager + WebPortal init |
| Rollback detection | `main.cpp` | `esp_ota_check_rollback_is_possible()` at boot → SystemStatus |
| Version string | `platformio.ini` | `-D FW_VERSION=\"1.0.0\"` build flag |

**Upload handler flow:**

```cpp
// In WebPortal::init()
server.on("/api/ota/upload", HTTP_POST,
    // Response handler (called after upload completes)
    [](AsyncWebServerRequest *request) {
        bool success = !Update.hasError();
        request->send(200, "application/json",
            success ? "{\"ok\":true,\"message\":\"Rebooting...\"}"
                    : "{\"ok\":false,\"error\":\"Update failed\"}");
        if (success) { delay(500); ESP.restart(); }
    },
    // Upload handler (called per chunk)
    [](AsyncWebServerRequest *request, String filename,
       size_t index, uint8_t *data, size_t len, bool final) {
        if (index == 0) {
            // First chunk — validate magic byte and begin
            // Note: request->contentLength() is the multipart body size, NOT the file size.
            // Use UPDATE_SIZE_UNKNOWN and let Update.h track actual bytes written.
            // Final size validation happens at Update.end().
            if (data[0] != 0xE9) {
                request->send(400, "application/json",
                    "{\"ok\":false,\"error\":\"Not a valid ESP32 firmware image\"}");
                return;
            }
            // Get max OTA partition size from the partition table
            const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
            if (!Update.begin(partition->size)) {
                request->send(500, "application/json",
                    "{\"ok\":false,\"error\":\"Failed to begin update\"}");
                return;
            }
        }
        // Write chunk
        if (Update.write(data, len) != len) {
            Update.abort();
            return;
        }
        if (final) {
            if (!Update.end(true)) {
                // Update.end() validates written size — rejects if corrupt
                return;
            }
        }
    }
);
```

**Firmware version:**

```ini
# platformio.ini
build_flags =
    -D FW_VERSION=\"1.0.0\"
    -D LOG_LEVEL=2
```

```cpp
// In /api/status response
doc["firmware_version"] = FW_VERSION;
doc["rollback_detected"] = rollbackDetected;
```

### Decision F3: OTA Self-Check — WiFi-OR-Timeout

**Strategy:** Mark firmware valid when WiFi connects OR after 60-second timeout — whichever comes first. No self-HTTP-request. No web server check.

**Rationale:**
- WiFi connection proves network stack works (catches most firmware bugs)
- 60-second timeout prevents false rollback during temporary network outages (router reboot, ISP flap)
- Crash-on-boot is caught by the watchdog timer regardless (firmware never reaches the check)
- No self-HTTP-request avoids complexity and race conditions with async server startup

**Timeout budget:**

| Scenario | Time to mark valid | Outcome |
|----------|-------------------|---------|
| Good firmware + WiFi works | 5-15s (WiFi connect time) | Mark valid ✅ |
| Good firmware + WiFi temporarily down | 60s (timeout) | Mark valid with warning ✅ |
| Bad firmware (crashes before 60s) | Never | Watchdog rollback ✅ |
| Bad firmware (WiFi code broken, doesn't crash) | 60s (timeout) | Mark valid ⚠️ — but device is reachable via AP fallback for re-flash |

**Implementation:**

```cpp
// main.cpp — global state
static uint32_t bootTime;
static bool otaMarkedValid = false;

// In setup(), after all init:
bootTime = millis();

// In loop(), before flight fetch:
if (!otaMarkedValid) {
    bool wifiUp = (WiFi.status() == WL_CONNECTED);
    bool timeoutExpired = (millis() - bootTime) > 60000;

    if (wifiUp || timeoutExpired) {
        esp_ota_mark_app_valid_cancel_rollback();
        otaMarkedValid = true;
        if (!wifiUp) {
            SystemStatus::set(OTA, WARNING, "Marked valid on timeout — WiFi not verified");
            LOG_I("OTA", "Marked valid on timeout, WiFi not connected");
        } else {
            LOG_I("OTA", "Marked valid, WiFi connected in " +
                  String((millis() - bootTime) / 1000) + "s");
        }
    }
}
```

**Rollback detection (also in main.cpp setup):**

```cpp
// In setup(), early — detect if bootloader rolled us back from a failed update
#include "esp_ota_ops.h"

const esp_partition_t* lastInvalid = esp_ota_get_last_invalid_partition();
if (lastInvalid != NULL) {
    // A previous OTA partition was marked invalid by the bootloader — rollback occurred
    rollbackDetected = true;
    SystemStatus::set(OTA, WARNING, "Firmware rolled back to previous version");
    LOG_I("OTA", "Rollback detected — previous partition was invalid");
}

// Note: esp_ota_check_rollback_is_possible() answers a different question
// ("can I roll back?"), NOT "did I just roll back?" — don't use it for detection.
// esp_ota_get_last_invalid_partition() returns non-NULL only when the bootloader
// actually invalidated a partition, which is the true rollback signal.
```

### Decision F4: IANA-to-POSIX Timezone Mapping — Browser-Side

**Strategy:** Browser converts IANA timezone to POSIX string client-side. ESP32 stores and uses the POSIX string directly. Zero firmware/LittleFS overhead.

**JS mapping table:** ~50-80 common timezones in a JS object. Generated from IANA `tzdata`. Stored in wizard.js and dashboard.js (compresses well in gzip).

```javascript
// timezone-map.js (embedded in wizard.js and dashboard.js)
const TZ_MAP = {
    "America/New_York":    "EST5EDT,M3.2.0,M11.1.0",
    "America/Chicago":     "CST6CDT,M3.2.0,M11.1.0",
    "America/Denver":      "MST7MDT,M3.2.0,M11.1.0",
    "America/Los_Angeles": "PST8PDT,M3.2.0,M11.1.0",
    "America/Anchorage":   "AKST9AKDT,M3.2.0,M11.1.0",
    "Pacific/Honolulu":    "HST10",
    "Europe/London":       "GMT0BST,M3.5.0/1,M10.5.0",
    "Europe/Berlin":       "CET-1CEST,M3.5.0,M10.5.0/3",
    "Europe/Paris":        "CET-1CEST,M3.5.0,M10.5.0/3",
    "Asia/Tokyo":          "JST-9",
    "Asia/Shanghai":       "CST-8",
    "Australia/Sydney":    "AEST-10AEDT,M10.1.0,M4.1.0/3",
    // ... ~50-80 entries total
};

function getTimezoneConfig() {
    const iana = Intl.DateTimeFormat().resolvedOptions().timeZone;
    const posix = TZ_MAP[iana] || null;
    return { iana, posix };
}
```

**Wizard flow:**
1. Auto-detect: `Intl.DateTimeFormat().resolvedOptions().timeZone` → `"America/Los_Angeles"`
2. Lookup: `TZ_MAP["America/Los_Angeles"]` → `"PST8PDT,M3.2.0,M11.1.0"`
3. Show dropdown with IANA names, pre-selected to detected timezone
4. Send POSIX string to ESP32 via `POST /api/settings { "timezone": "PST8PDT,M3.2.0,M11.1.0" }`
5. If lookup fails (unknown IANA): show text field for manual POSIX entry

**Dashboard timezone selector:** Same dropdown + mapping. User can change timezone post-setup.

**ESP32 usage:**
```cpp
String tz = ConfigManager::getSchedule().timezone;  // POSIX string
configTzTime(tz.c_str(), "pool.ntp.org", "time.nist.gov");
```

### Decision F5: Night Mode Scheduler — Non-Blocking Main Loop

**Architecture:** Brightness schedule check runs in the Core 1 main loop, adjacent to the flight fetch cycle. Non-blocking — single `getLocalTime()` + integer comparison per iteration.

**Schedule times stored as minutes since midnight** (uint16, 0-1439):
- 23:00 = 1380, 07:00 = 420, 00:00 = 0, 12:30 = 750

**Midnight-crossing logic:**

```cpp
bool inDimWindow;
if (dimStart <= dimEnd) {
    // Same-day: e.g., 09:00-17:00
    inDimWindow = (currentMinutes >= dimStart && currentMinutes < dimEnd);
} else {
    // Midnight crossing: e.g., 23:00-07:00
    inDimWindow = (currentMinutes >= dimStart || currentMinutes < dimEnd);
}
```

**Interaction with manual brightness:**
- When schedule is **enabled** and device is **in dim window**: scheduler overrides brightness
- When schedule is **enabled** and device is **outside dim window**: normal brightness from ConfigManager
- When schedule is **disabled**: scheduler does nothing, manual brightness applies
- If user manually changes brightness while in dim window: schedule re-overrides on next check (1-second loop)
- Dashboard should indicate when schedule is actively overriding brightness

**NTP sync tracking:**

```cpp
// Callback registered once in setup()
sntp_set_time_sync_notification_cb([](struct timeval *tv) {
    ntpSynced = true;
    LOG_I("NTP", "Time synchronized");
    SystemStatus::set(NTP, OK, "Clock synced");
});

// NTP re-sync happens automatically every 1 hour (LWIP default)
// If re-sync fails, device continues using last known time
```

**NTP re-sync interval:** ESP32 LWIP SNTP defaults to 1-hour re-sync. Sufficient for <5 ppm RTC drift (~0.4s/day). No custom interval needed.

### Decision F6: ConfigManager Expansion — New Keys & Struct

**5 new NVS keys:**

| NVS Key | Type | Default | Category | Notes |
|---------|------|---------|----------|-------|
| `timezone` | string | `"UTC0"` | schedule | POSIX timezone string, max 40 chars |
| `sched_enabled` | uint8 | 0 | schedule | 0 = disabled, 1 = enabled |
| `sched_dim_start` | uint16 | 1380 | schedule | Minutes since midnight (23:00) |
| `sched_dim_end` | uint16 | 420 | schedule | Minutes since midnight (07:00) |
| `sched_dim_brt` | uint8 | 10 | schedule | Brightness during dim window (0-255) |

```cpp
// NVS key abbreviations (15-char limit):
// sched_enabled   = schedule_enabled
// sched_dim_start = schedule_dim_start
// sched_dim_end   = schedule_dim_end
// sched_dim_brt   = schedule_dim_brightness
```

**New ConfigManager category struct:**

```cpp
struct ScheduleConfig {
    bool enabled;
    uint16_t dim_start;      // minutes since midnight (0-1439)
    uint16_t dim_end;        // minutes since midnight (0-1439)
    uint8_t dim_brightness;  // 0-255
    String timezone;         // POSIX timezone string
};
```

**ConfigManager additions:**
```cpp
static ScheduleConfig getSchedule();
// Reboot-required keys: unchanged (timezone change takes effect on next NTP sync, no reboot)
// Hot-reload keys: sched_enabled, sched_dim_start, sched_dim_end, sched_dim_brt, timezone
```

All 5 new keys are **hot-reload** — no reboot required. Timezone change calls `configTzTime()` with the new POSIX string immediately.

### Decision F7: API Endpoint Additions

**2 new endpoints:**

| Method | Endpoint | Purpose | Auth | Response |
|--------|----------|---------|------|----------|
| `POST` | `/api/ota/upload` | Multipart firmware upload | None (local network) | `{ ok: true, message: "Rebooting..." }` or `{ ok: false, error: "..." }` |
| `GET` | `/api/settings/export` | Download all config as JSON file | None | `Content-Disposition: attachment; filename=flightwall-settings.json` |

**Updated existing endpoint — `/api/status`:**

New fields in response:
```json
{
    "ok": true,
    "data": {
        "firmware_version": "1.0.0",
        "rollback_detected": false,
        "ntp_synced": true,
        "schedule_active": true,
        "uptime_seconds": 86400,
        // ... existing fields ...
    }
}
```

**Settings export format (`/api/settings/export`):**

```json
{
    "flightwall_settings_version": 1,
    "exported_at": "2026-04-11T15:30:00",
    "wifi_ssid": "MyNetwork",
    "wifi_password": "secret123",
    "os_client_id": "...",
    "os_client_sec": "...",
    "aeroapi_key": "...",
    "center_lat": 37.7749,
    "center_lon": -122.4194,
    "radius_km": 10.0,
    "tiles_x": 10,
    "tiles_y": 2,
    "tile_pixels": 16,
    "display_pin": 25,
    "origin_corner": 0,
    "scan_dir": 0,
    "zigzag": 0,
    "brightness": 40,
    "text_color_r": 255,
    "text_color_g": 255,
    "text_color_b": 255,
    "fetch_interval": 30,
    "display_cycle": 3,
    "timezone": "PST8PDT,M3.2.0,M11.1.0",
    "sched_enabled": 1,
    "sched_dim_start": 1380,
    "sched_dim_end": 420,
    "sched_dim_brt": 10
}
```

All NVS keys dumped as flat JSON. Includes `flightwall_settings_version` for forward compatibility. Import is client-side only — wizard JS reads the file and pre-fills form fields.

### Foundation Decision Impact — Implementation Sequence

**Sequential foundation (must be in order):**
1. **Partition table update** (`custom_partitions.csv`) — must be first, requires USB reflash
2. **ConfigManager expansion** (5 new keys + ScheduleConfig struct) — foundation for everything else

**OTA track (sequential, after step 2):**
3. **OTA upload endpoint** in WebPortal — core Foundation capability
4. **OTA self-check + rollback detection** in main.cpp — safety mechanism for all future OTA updates

**Time track (sequential, parallelizable with OTA track after step 2):**
5. **NTP integration** (`configTzTime()` after WiFi connect) — enables night mode
6. **Night mode scheduler** in main loop — uses NTP + ConfigManager

**Settings track (after step 2, parallelizable with both tracks above):**
7. **Settings export endpoint** — migration tool for partition change

**UI integration (after all tracks complete):**
8. **Dashboard cards** (Firmware, Night Mode) — UI for new capabilities
9. **Wizard Step 6** (Test Your Wall) + timezone auto-detect — onboarding polish
10. **Settings import** in wizard JS — client-side only, references exported JSON

**Parallelization note:** Steps 3-4, 5-6, and 7 are independent tracks that all depend on step 2 but not on each other. For solo dev, linear order is fine. For sprint planning, these represent parallelizable work streams.

### Cross-Decision Dependencies

```
custom_partitions.csv
    └── OTA upload endpoint (needs dual partitions)
        └── OTA self-check (validates after OTA reboot)

ConfigManager (ScheduleConfig)
    └── NTP integration (uses timezone from config)
        └── Night mode scheduler (uses NTP time + schedule config)

Settings export endpoint
    └── Settings import in wizard (reads export file)

Wizard Step 6 (Test Your Wall)
    └── Timezone auto-detect in wizard (IANA-to-POSIX JS mapping)
```

## Foundation Release — Implementation Patterns & Consistency Rules

### Foundation-Specific Conflict Points

The MVP architecture established 11 enforcement rules. The Foundation Release introduces 4 new pattern areas where AI agents could make inconsistent choices.

### OTA Upload Handler Pattern

**Validation-first, fail-fast:**
- Check `Content-Length` against `MAX_OTA_SIZE` before calling `Update.begin()` — reject oversized before allocating flash
- Check first byte == `0xE9` (ESP32 image magic byte) on first chunk — reject non-firmware immediately
- Every error returns `{ ok: false, error: "human-readable message" }` with appropriate HTTP status code

**Streaming pattern:**
```cpp
// First chunk (index == 0): validate and begin
if (contentLength > MAX_OTA_SIZE) { /* reject 400 */ }
if (data[0] != 0xE9) { /* reject 400 */ }
if (!Update.begin(contentLength)) { /* reject 500 */ }

// Every chunk: write and verify
if (Update.write(data, len) != len) { Update.abort(); return; }

// Final chunk: finalize
if (final) { Update.end(true); }
```

**Anti-pattern:** Never buffer the entire binary in RAM before writing. The binary can be 1.5MB — ESP32 has ~280KB usable RAM. Always stream chunk-by-chunk via `Update.write()`.

### Time Management Pattern

**Non-blocking time access (mandatory):**
```cpp
struct tm now;
if (!getLocalTime(&now, 0)) return;  // timeout=0, NON-BLOCKING
```

**Rule:** Always pass `timeout=0` to `getLocalTime()`. Never block the main loop waiting for time. If time isn't available, skip the time-dependent operation gracefully.

**NTP sync state tracking:**
```cpp
// Register callback once in setup()
sntp_set_time_sync_notification_cb([](struct timeval *tv) {
    ntpSynced = true;
});

// Check state via atomic bool — never poll NTP status
std::atomic<bool> ntpSynced{false};
```

**Anti-pattern:** Polling `sntp_get_sync_status()` in a loop. Use the callback to set a flag and check the flag.

### Schedule Time Convention

**Minutes since midnight (uint16, 0-1439):**
- Compact storage (2 bytes per time vs. 5-byte HH:MM string)
- Simple arithmetic for midnight-crossing comparison
- No string parsing at runtime on the ESP32

**Conversion (client-side JS only):**
```javascript
// HH:MM string → minutes since midnight
function timeToMinutes(hhmm) {
    const [h, m] = hhmm.split(':').map(Number);
    return h * 60 + m;
}

// minutes since midnight → HH:MM string
function minutesToTime(mins) {
    const h = String(Math.floor(mins / 60)).padStart(2, '0');
    const m = String(mins % 60).padStart(2, '0');
    return `${h}:${m}`;
}
```

**Anti-pattern:** Storing schedule times as HH:MM strings in NVS or passing time strings between firmware components. Always use `uint16` minutes in firmware.

### Settings Export/Import Boundary

**Export: server-side.**
- `GET /api/settings/export` dumps all ConfigManager keys as flat JSON
- `Content-Disposition: attachment; filename=flightwall-settings.json`
- Includes `flightwall_settings_version: 1` for forward compatibility

**Import: client-side only.**
- Wizard JS reads the file via `FileReader API`
- Pre-fills form fields for user review
- User confirms before settings are applied via normal `POST /api/settings`
- Unrecognized keys in the file are silently ignored
- No server-side import endpoint — no new attack surface

**Anti-pattern:** Building a `POST /api/settings/import` endpoint that blindly applies a JSON file. Import is always mediated through the wizard UI with user confirmation.

### Updated Enforcement Guidelines

**All AI agents implementing Foundation stories MUST also follow (in addition to MVP rules 1-11):**

12. OTA upload must validate before writing — size check + magic byte on first chunk, fail-fast with `{ ok: false, error: "..." }` JSON response
13. OTA upload must stream directly to flash via `Update.write()` per chunk — never buffer the full binary in RAM
14. `getLocalTime()` must always use `timeout=0` (non-blocking) — never block the main loop waiting for time
15. Schedule times stored as `uint16` minutes since midnight (0-1439) — no string time representations in firmware
16. Settings import is client-side only — no server-side import endpoint or file parsing on the ESP32

## Foundation Release — Project Structure & Boundaries

### File Change Map

No new files. All Foundation changes fit into existing components:

| File | Change | Foundation Purpose |
|------|--------|-------------------|
| `firmware/custom_partitions.csv` | UPDATED | Dual OTA partition layout (app0 1.5MB + app1 1.5MB + spiffs 960KB) |
| `firmware/platformio.ini` | UPDATED | `-D FW_VERSION=\"1.0.0\"` build flag, potential `-D CONFIG_BT_ENABLED=0` |
| `firmware/src/main.cpp` | UPDATED | OTA self-check + rollback detection at boot, NTP init after WiFi, brightness scheduler in loop() |
| `firmware/core/ConfigManager.h` | UPDATED | ScheduleConfig struct, `getSchedule()`, 5 new NVS key abbreviations |
| `firmware/core/ConfigManager.cpp` | UPDATED | ScheduleConfig NVS read/write/defaults, timezone hot-reload calls `configTzTime()` |
| `firmware/core/SystemStatus.h` | UPDATED | New subsystem entries: `OTA`, `NTP` |
| `firmware/adapters/WebPortal.h` | UPDATED | OTA upload + settings export endpoint declarations |
| `firmware/adapters/WebPortal.cpp` | UPDATED | `/api/ota/upload` multipart handler, `/api/settings/export` GET handler, updated `/api/status` response |
| `firmware/data-src/dashboard.html` | UPDATED | Firmware card, Night Mode card, rollback warning toast |
| `firmware/data-src/dashboard.js` | UPDATED | OTA upload UI with progress, night mode settings, timezone selector dropdown, TZ_MAP |
| `firmware/data-src/wizard.html` | UPDATED | Step 6 (Test Your Wall), timezone field in Location step |
| `firmware/data-src/wizard.js` | UPDATED | Step 6 confirmation/back-nav/RGB test logic, timezone auto-detect, TZ_MAP, settings import via FileReader |
| `firmware/data/*.gz` | REBUILT | Gzipped copies of all updated web assets |

### Requirements to File Mapping

| FR Group | Key Files |
|----------|-----------|
| OTA Updates (FR1-FR11) | `WebPortal.cpp` (upload endpoint, validation, streaming), `main.cpp` (self-check, rollback detection), `custom_partitions.csv` |
| Settings Migration (FR12-FR14) | `WebPortal.cpp` (`/api/settings/export`), `wizard.js` (FileReader import, pre-fill fields) |
| Night Mode (FR15-FR24) | `ConfigManager` (ScheduleConfig, 5 keys), `main.cpp` (NTP init, scheduler), `dashboard.js` (Night Mode card) |
| Onboarding Polish (FR25-FR29) | `wizard.html` (Step 6 UI), `wizard.js` (Step 6 logic, timezone auto-detect, TZ_MAP) |
| Dashboard UI (FR30-FR34) | `dashboard.html` (Firmware + Night Mode cards), `dashboard.js` (OTA upload, schedule settings, rollback toast) |
| System & Resilience (FR35-FR37) | `main.cpp` (self-check gate), `ConfigManager` (new NVS keys), `SystemStatus` (OTA + NTP subsystems) |

### Architectural Boundaries — No Changes

All Foundation changes respect the existing hexagonal architecture:

- **WebPortal** remains the sole browser adapter — OTA upload and settings export are new routes, not new boundary crossings
- **ConfigManager** remains the config core — 5 new keys follow the existing category struct pattern
- **main.cpp** remains the orchestrator — self-check and scheduler are boot/loop concerns, not new components
- **SystemStatus** remains the health registry — OTA and NTP are new subsystem entries
- **Core 0 (display) remains read-only** — brightness scheduler runs on Core 1 and uses the existing ConfigManager hot-reload path to affect display brightness

### Foundation Data Flow

```
Boot → ConfigManager::init() (loads 5 new schedule keys from NVS)
     → SystemStatus::init() (registers OTA + NTP subsystems)
     → WiFiManager::init() → WiFi connects
     → configTzTime(timezone, "pool.ntp.org", "time.nist.gov")  ← NEW
     → sntp callback → ntpSynced = true                          ← NEW
     → WebPortal::init() (registers /api/ota/upload, /api/settings/export)
     → OTA self-check: WiFi connected OR 60s timeout → mark valid ← NEW
     → Rollback detection → SystemStatus::set(OTA, WARNING, ...) ← NEW
     → loop():
         Flight fetch cycle (unchanged)
         checkBrightnessSchedule()  ← NEW (non-blocking, Core 1)
```

### External Integration Points — Foundation Additions

| Service | Protocol | Purpose | New? |
|---------|----------|---------|------|
| NTP (pool.ntp.org, time.nist.gov) | UDP port 123 | Time sync for night mode scheduler | **Yes** |

No other new external integrations. OTA is local (browser → ESP32 over LAN).

## Foundation Release — Architecture Validation Results

### Coherence Validation ✅

**Decision Compatibility:** All 7 Foundation decisions (F1-F7) are compatible with each other and with the 6 MVP decisions. No technology conflicts. Partition table change (F1) enables OTA (F2/F3), NTP (F4/F5) enables scheduler (F5), ConfigManager expansion (F6) supports both. Clean dependency chain with no cycles.

**Pattern Consistency:** Foundation patterns (rules 12-16) extend MVP patterns (rules 1-11) without contradictions. `snake_case` flows consistently through new NVS keys (`sched_enabled`, `sched_dim_start`, etc.) → JSON fields → ScheduleConfig struct fields. Logging and error handling follow existing macros and boolean-return patterns.

**Structure Alignment:** All Foundation changes fit into existing files and directories. No new classes, no new boundary crossings. WebPortal gets new routes (consistent pattern). ConfigManager gets a new category struct (consistent pattern). main.cpp gets boot-time and loop-time additions (consistent with existing init sequence and flight fetch loop).

### Requirements Coverage ✅

**All 37 Functional Requirements covered.** Every FR maps to at least one architectural decision with explicit code location:
- OTA (FR1-FR11): Decisions F1, F2, F3 — WebPortal.cpp, main.cpp, custom_partitions.csv
- Settings Migration (FR12-FR14): Decision F7 — WebPortal.cpp, wizard.js
- Night Mode (FR15-FR24): Decisions F4, F5, F6 — ConfigManager, main.cpp, dashboard.js, wizard.js
- Onboarding (FR25-FR29): Structure map — wizard.html/js (reuses existing calibration endpoints)
- Dashboard UI (FR30-FR34): Structure map — dashboard.html/js
- System (FR35-FR37): Decisions F1, F2, F3, F6 — main.cpp, ConfigManager

**All NFRs architecturally addressed:**
- Performance: streaming OTA (≤30s), non-blocking scheduler (≤1%), NTP (≤10s), hot-reload (≤1s)
- Reliability: self-check gate, watchdog rollback, NVS persistence, NTP re-sync, graceful NTP failure
- Resource constraints: 1.5MB firmware cap, 682KB LittleFS free, streaming (no RAM buffer), ≤256 bytes NVS

### Implementation Readiness ✅

**Decision Completeness:** 7 decisions with code examples, interface definitions, and rationale. Partition sizes specified with exact offsets. NVS key table complete with types, defaults, and abbreviations. API endpoints specified with request/response formats.

**Structure Completeness:** File change map covers all 13 affected files. Requirements-to-file mapping is explicit. No orphaned files, no unmapped FRs.

**Pattern Completeness:** 16 enforcement rules (11 MVP + 5 Foundation). OTA streaming, time management, schedule convention, and settings boundary patterns all specified with code examples and anti-patterns.

### Architectural Overrides

**FR5 override (self-check simplification):**

The PRD specifies FR5 as: *"Device can verify its own health after booting new firmware (WiFi, web server, OTA endpoint reachability) before marking the partition as valid."*

The architecture simplifies this to: WiFi connects OR 60-second timeout. No web server check, no self-HTTP-request to `/api/ota/upload`.

**Rationale:** The self-HTTP-request adds complexity and race conditions (async server may not be fully initialized when the check runs). WiFi connection proves the network stack works. If the web server failed to initialize, the firmware would crash before reaching the self-check — caught by the watchdog. The timeout path catches temporary network outages without false rollback. The PRD's intent (don't brick the device) is fully preserved by the simpler approach.

**Story specs MUST reference this architectural decision (F3), not the raw FR5 wording, to prevent re-implementation of the self-HTTP-request.**

### Gap Analysis

| Priority | Gap | Resolution |
|----------|-----|-----------|
| Minor | `MAX_OTA_SIZE` derivation | Use `esp_ota_get_next_update_partition(NULL)->size` to get partition capacity at runtime. No hardcoded constant. Already reflected in F2 code example. |
| Minor | Rollback detection API | Use `esp_ota_get_last_invalid_partition()` (returns non-NULL only after actual rollback). Do NOT use `esp_ota_check_rollback_is_possible()` which answers "can I roll back?" not "did I just roll back?" Already corrected in F3 code example. |
| Minor | Multipart Content-Length | `request->contentLength()` returns multipart body size, not file size. Use `UPDATE_SIZE_UNKNOWN` with `Update.begin(partition->size)` and let `Update.end()` validate. Already corrected in F2 code example. |
| Minor | 60s timeout path testability | Timeout path verified by code inspection + one-time manual test (boot with WiFi unavailable). Not expected in automated test coverage. |
| Minor | Settings export/import round-trip | Export includes `flightwall_settings_version` and `exported_at` metadata fields. Import must ignore these (they're not config keys). Wizard JS should silently skip unrecognized keys per FR14. |
| Minor | Manual brightness during dim window | Schedule re-overrides brightness on next check (~1s). Dashboard should indicate "Schedule active — brightness overridden" when in dim window. Document as known behavior, not a bug. |
| Minor | TZ_MAP completeness | Ship with ~50-80 common timezones. Manual POSIX entry fallback for unlisted zones. Expandable in future releases. |

No critical gaps.

### Foundation Architecture Completeness Checklist

- [x] Foundation PRD context analyzed (37 FRs, NFRs, constraints)
- [x] Scale assessed (narrow extension, partition change is main blast radius)
- [x] Constraints identified (1.5MB firmware cap, 960KB LittleFS, NTP dependency)
- [x] Cross-cutting concerns mapped (flash budget, OTA resilience, time management)
- [x] No new external dependencies (all ESP32 core libraries)
- [x] 7 Foundation decisions documented with code examples
- [x] Partition table specified with exact offsets
- [x] Implementation patterns extended (rules 12-16)
- [x] NVS key table expanded (5 new keys with abbreviations)
- [x] 2 new API endpoints specified with request/response formats
- [x] Complete file change map (13 files, all UPDATED)
- [x] Requirements-to-file mapping complete (37 FRs → specific files)
- [x] Architectural boundaries unchanged
- [x] Implementation sequence with parallelization annotations
- [x] FR5 architectural override documented with rationale
- [x] All party mode refinements incorporated (rollback API, multipart handling, parallelization)

### Foundation Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION
**Confidence Level:** High

**Key Strengths:**
- Clean extension of MVP architecture — no breaking changes to existing patterns
- OTA self-check is simplified and robust (WiFi-OR-timeout avoids false rollbacks)
- Rollback detection uses correct ESP-IDF API (`esp_ota_get_last_invalid_partition()`)
- Browser-side timezone mapping avoids firmware/LittleFS overhead
- Non-blocking scheduler with zero new inter-core communication
- All enforcement rules are additive (no changes to MVP rules 1-11)
- Party mode reviews caught 3 code-level issues before implementation (rollback API, multipart Content-Length, FR5 override)

**Areas for Future Enhancement:**
- OTA pull from GitHub releases (Delight Release — already in PRD pipeline)
- Firmware signing for public distribution (Phase 3)
- Sunrise/sunset-aware brightness scheduling (Phase 2)
- Expanded TZ_MAP coverage beyond initial ~50-80 timezones

### Foundation Implementation Handoff

**Pre-implementation gate:**
1. Measure current firmware binary size (`~/.platformio/penv/bin/pio run` → check `.pio/build/*/firmware.bin`)
2. If exceeding 1.3MB: disable Bluetooth (`-D CONFIG_BT_ENABLED=0`), review library compile flags
3. Proceed only when binary fits comfortably under 1.5MB with headroom for Foundation additions

**First implementation priority:**
1. Update `custom_partitions.csv` to dual-OTA layout
2. Add `-D FW_VERSION=\"1.0.0\"` to platformio.ini build_flags
3. Expand ConfigManager with ScheduleConfig struct and 5 new NVS keys
4. Add `/api/ota/upload` handler in WebPortal
5. Add OTA self-check + rollback detection in main.cpp setup()
6. Verify firmware builds, OTA partitions are recognized, and existing functionality works

---

# Display System Release — Architecture Extension

_Extends the MVP + Foundation architecture with a pluggable display mode system, mode registry, Classic Card migration, Live Flight Card mode, and Mode Picker UI. All prior architectural decisions remain in effect — this section adds new decisions and updates existing ones where the Display System PRD introduces changes._

## Display System Release — Project Context Analysis

### Requirements Overview

**Functional Requirements (36 FRs across 8 groups):**

| Group | FRs | Architectural Impact |
|-------|-----|---------------------|
| DisplayMode Interface | FR1-FR4 | **Major:** New abstract interface with lifecycle semantics (init/render/teardown), memory reporting, rendering context with zone bounds and read-only brightness |
| Mode Registry & Lifecycle | FR5-FR10 | **Major:** New `core/ModeRegistry` component — static mode table, serialized switch execution, heap validation before activation, mode enumeration for API/UI |
| Classic Card Mode | FR11-FR13 | **High-risk migration:** Extract rendering logic from `NeoMatrixDisplay::renderZoneFlight()` and sub-methods (~140 lines) into standalone mode class with pixel-parity validation |
| Live Flight Card Mode | FR14-FR16 | **Medium:** New mode using existing telemetry fields (altitude_kft, speed_mph, track_deg, vertical_rate_fps) already in FlightInfo; adaptive field dropping for smaller panels |
| Mode Picker UI | FR17-FR26 | **Medium:** New dashboard section with schematic wireframe previews, activation control, transition state feedback, one-time upgrade notification |
| Mode Persistence | FR27-FR29 | **Minor:** Single NVS key (`display_mode`) for active mode; boot restoration with Classic Card default on first upgrade |
| Mode Switch API | FR30-FR33 | **Minor:** 2 new HTTP endpoints following existing `/api/*` JSON envelope pattern; transition state reporting |
| Display Infrastructure | FR34-FR36 | **Major refactoring:** NeoMatrixDisplay splits responsibilities — hardware ownership vs. rendering delegation; frame commit (`FastLED.show()`) separated from mode rendering |

**Non-Functional Requirements (architectural drivers):**

| NFR | Target | Architecture Decision Driver |
|-----|--------|------------------------------|
| Mode switch latency | <2 seconds (user-perceived) | Full lifecycle (teardown → heap check → init → first render) within budget; cooperative scheduling on display task |
| Per-frame budget | <16ms, non-blocking | Mode render must complete within 60fps budget; no network I/O, no filesystem I/O on hot path |
| Mode Picker load | <1 second | Static mode metadata; no heap allocation for enumeration |
| Registry enumeration | <10ms, zero heap | Mode list served from BSS/data segment; no dynamic allocation for API response |
| Heap stability | No net loss after 100 switches | Fixed-size heap allocations per mode; sequential teardown → init with no overlapping allocations |
| Rapid-switch safety | 10 toggles in 3 seconds | Switch serialization via state enum in display task; queue/ignore during in-progress transition |
| Stack discipline | ≤512 bytes stack-local per render frame | Large buffers heap-allocated at mode init, reused across frames |
| Heap floor | ≥30KB free after mode activation | Heap guard validates before init; empirical measurement during integration |
| NVS namespace safety | No collision with Foundation keys | `display_mode` key uses existing `"flightwall"` namespace with verified unique prefix |
| Pixel parity | Classic Card identical to pre-migration output | GFX draw-call sequence comparison; visual verification across 5+ flight cards |
| Foundation compatibility | OTA, night mode, NTP, settings, health all unaffected | Modes isolated from all subsystems via read-only rendering context |
| Cooperative scheduling | No new FreeRTOS tasks for mode rendering | Mode render runs within existing display task on Core 0; no interrupt-driven frame push |
| Dashboard consistency | Mode Picker matches existing card/navigation patterns | Same CSS, same JS fetch patterns, no third-party script runtime additions |

### Scale & Complexity

- **Primary domain:** IoT/Embedded with companion web UI (unchanged)
- **Complexity level:** Medium-High — plugin architecture on memory-constrained MCU with cross-core coordination
- **New architectural components:** 4 (ModeRegistry, ClassicCardMode, LiveFlightCardMode, DisplayMode interface)
- **New directory:** `firmware/modes/` for mode implementations
- **Updated components:** NeoMatrixDisplay (responsibility split), WebPortal (2 new endpoints), main.cpp (display task integration), dashboard HTML/JS (Mode Picker UI)
- **Existing components unchanged:** FlightDataFetcher, OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, WiFiManager, ConfigManager (no new keys except `display_mode`), LayoutEngine, LogoManager, SystemStatus

### Technical Constraints (Display System–Specific)

| Constraint | Impact |
|-----------|--------|
| ESP32 non-compacting heap allocator | Repeated mode switches risk heap fragmentation; modes must use fixed-size allocations and free all memory during teardown |
| Single display task on Core 0 | Mode rendering, switch execution, and lifecycle management all run cooperatively on one task — no parallel mode operations |
| Cross-core API boundary | Mode switch requests arrive from WebPortal (Core 1 async TCP) but execute on display task (Core 0) — requires atomic flag coordination |
| `FastLED.show()` disables interrupts | Frame commit must remain in display task after render returns; modes must NOT call `FastLED.show()` directly |
| 8KB display task stack | Per-frame rendering must stay within stack budget; large buffers allocated at mode init on heap |
| Shared logo buffer (2KB) | `_logoBuffer[1024]` in NeoMatrixDisplay is reused across frames; modes receive it via rendering context, not owned per-mode |
| Existing calibration/positioning mode flags | These are primitive boolean checks in the display task, NOT the template for the mode system — DisplayMode requires proper lifecycle semantics |

### Cross-Cutting Concerns (Display System Additions)

1. **Rendering context as isolation boundary** — Modes receive a bundled struct containing: matrix reference (GFX primitives), zone bounds (LayoutResult), pre-computed text color, brightness (read-only), flight data reference, shared logo buffer reference, and display cycle timing. Modes have zero dependency on ConfigManager, WiFiManager, or any other subsystem. This isolation is the architectural guarantee for NFR C2 (Foundation compatibility).

2. **Cross-core mode switch coordination** — Browser → POST API → WebPortal sets atomic switch request flag → display task checks flag at top of loop iteration (before any render call) → executes teardown/init → clears flag. Mirrors the existing `g_configChanged` atomic pattern. No new concurrency primitives, no mutexes, no FreeRTOS queues for mode switching.

3. **NeoMatrixDisplay responsibility split** — Post-refactoring, NeoMatrixDisplay owns: (a) hardware initialization (LED strip, matrix object, brightness), (b) frame commit (`FastLED.show()`), (c) shared resources (logo buffer, matrix dimensions), (d) emergency fallback renderer (`displaySingleFlightCard()` retained for FR36 — ensures a valid rendering path even when no mode can init). Rendering logic migrates to mode classes. Display task orchestrates: `registry.getActiveMode()->render(context)` then `display.show()`.

4. **Heap lifecycle management** — Only one mode active at any time. During switch: outgoing mode's `teardown()` frees all heap → registry measures `ESP.getFreeHeap()` → compares against incoming mode's `getMemoryRequirement()` → if sufficient, calls `init()` → if insufficient, re-initializes previous mode and returns error. No overlapping mode allocations. 100-switch stability validated empirically.

5. **Flight cycling state migration** — `_currentFlightIndex` and `_lastCycleMs` currently live in NeoMatrixDisplay and the display task. Post-extraction, each mode owns its own cycling state (index + timer). The rendering context passes the full flight vector; modes decide how to cycle. This costs ~12 bytes per mode (trivial for `getMemoryRequirement()`).

6. **Zone descriptor metadata for Mode Picker** — Each mode exposes static metadata describing its zone layout (grid dimensions, zone labels, row/col positions, spans). The GET API returns this alongside mode name/ID. Dashboard JS renders schematic wireframe previews client-side using CSS Grid with `<div>` elements — no canvas, no SVG, no server-rendered bitmaps. Zone layout is fully data-driven from each mode's `getZoneLayout()` metadata; new modes get schematics automatically with zero UI code changes.

7. **Upgrade notification via dual-source dismissal** — The one-time "New display modes available" banner (FR25-FR26) uses dual-source dismissal for robustness: firmware returns an `upgrade_notification` flag in the `GET /api/display/modes` response (set when no prior `display_mode` NVS key exists on first boot), AND browser checks `localStorage.getItem('flightwall_mode_notif_seen')`. Banner shows only when both conditions are true (API flag true AND localStorage not set). Dismissal clears both: `localStorage.setItem()` on client + `POST /api/display/ack-notification` to clear the firmware flag. Either source being cleared independently prevents reappearance.

8. **Heap baseline measurement prerequisite** — Before any Display System code is written, measure free heap after full Foundation Release boot (WiFi connected, web server active, NTP synced, flight data in queue). This establishes the budget ceiling for mode `getMemoryRequirement()` values and validates whether the 30KB floor (NFR S4) is achievable.

### Existing Architecture Unchanged

The following MVP + Foundation decisions remain in full effect with no modifications:

- Hexagonal (Ports & Adapters) pattern
- ConfigManager singleton with category struct getters (no new config structs — only one new NVS key)
- Inter-task communication (atomic flags for config, FreeRTOS queue for flights)
- WiFi state machine (WiFiManager)
- SystemStatus error registry
- FreeRTOS dual-core task pinning (display Core 0, everything else Core 1)
- All naming conventions, NVS key patterns, logging macros, and enforcement rules (1-16)
- Web asset patterns (fetch + json.ok + showToast)
- OTA self-check, rollback detection, night mode scheduler
- Flash partition layout (unchanged from Foundation)

## Display System Release — Technology Stack

**Starter Template Evaluation: N/A (Brownfield Extension)**

No new external dependencies. All Display System capabilities use the existing ESP32 Arduino framework and libraries already in the technology stack:

| Component | Implementation | Source |
|-----------|---------------|--------|
| DisplayMode interface | Abstract C++ class | New firmware code |
| ModeRegistry | Static C++ class | New firmware code |
| ClassicCardMode | C++ class (extracted from NeoMatrixDisplay) | New firmware code |
| LiveFlightCardMode | C++ class | New firmware code |
| Mode Switch API | ESPAsyncWebServer routes | Existing dependency |
| Mode Picker UI | HTML/JS/CSS in dashboard | Existing web asset pattern |
| NVS persistence | ESP32 Preferences API | Existing dependency |
| Rendering primitives | Adafruit GFX via FastLED_NeoMatrix | Existing dependency |

The MVP + Foundation dependency stack (ESPAsyncWebServer Carbou fork, AsyncTCP, FastLED, Adafruit GFX, FastLED NeoMatrix, ArduinoJson, LittleFS, ESPmDNS, Update.h, esp_ota_ops.h, time.h/configTzTime) remains unchanged.

## Display System Release — Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. D1 — DisplayMode interface with RenderContext (every mode and the registry depend on this contract)
2. D2 — ModeRegistry with static table, cooperative switch serialization, heap guard
3. D3 — NeoMatrixDisplay responsibility split (rendering extracted, hardware retained)
4. D4 — Display task integration via ModeRegistry::tick() + atomic flag

**Important Decisions (Shape Architecture):**
5. D5 — Mode Switch API endpoints (GET list + POST activate)
6. D6 — NVS persistence for active mode selection

**Supporting Decisions:**
7. D7 — Mode Picker UI with client-side wireframes and localStorage notification

**Deferred Decisions (Post-Display System):**
- Mode-specific user settings (per-mode NVS keys and UI panels)
- Animated transitions (blank transition only in this release)
- Mode scheduling (auto-switch by time of day)
- Server-rendered preview thumbnails

### Decision D1: DisplayMode Interface — Abstract Class with RenderContext

**The central abstraction. Every mode implements this contract.**

```cpp
// Forward declarations
struct RenderContext;
struct FlightInfo;
struct ModeZoneDescriptor;

class DisplayMode {
public:
    virtual ~DisplayMode() = default;
    virtual bool init(const RenderContext& ctx) = 0;
    virtual void render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) = 0;
    virtual void teardown() = 0;
    virtual const char* getName() const = 0;
    virtual const ModeZoneDescriptor& getZoneDescriptor() const = 0;
};
```

**Note:** `getMemoryRequirement()` is intentionally NOT in the virtual interface. Memory requirement is a static property of the mode class, needed BEFORE instantiation for the heap guard. It is exposed via a static function pointer in `ModeEntry` (see D2). This avoids a throwaway allocation just to query memory needs.

**Rendering context — bundles everything a mode needs, nothing it shouldn't have:**

```cpp
struct RenderContext {
    Adafruit_NeoMatrix* matrix;     // GFX drawing primitives (write to buffer only)
    LayoutResult layout;             // zone bounds (logo, flight, telemetry)
    uint16_t textColor;              // pre-computed from DisplayConfig
    uint8_t brightness;              // read-only — managed by night mode scheduler
    uint16_t* logoBuffer;            // shared 2KB buffer from NeoMatrixDisplay
    uint16_t displayCycleMs;         // cycle interval for modes that rotate flights
};
```

**Zone descriptor — static metadata for Mode Picker UI wireframes:**

```cpp
struct ZoneRegion {
    const char* label;    // e.g., "Airline", "Route", "Altitude"
    uint8_t relX, relY;   // relative position within mode's canvas (0-100%)
    uint8_t relW, relH;   // relative dimensions (0-100%)
};

struct ModeZoneDescriptor {
    const char* description;        // human-readable mode description
    const ZoneRegion* regions;      // static array of labeled regions
    uint8_t regionCount;
};
```

**Key design choices:**
- `RenderContext` passed by const reference — modes cannot modify shared state
- No ConfigManager dependency in modes — text color and timing pre-computed by caller
- `logoBuffer` is a shared resource owned by NeoMatrixDisplay, not per-mode (avoids doubling 2KB heap usage)
- `getZoneDescriptor()` returns static data (PROGMEM-safe) — zero heap for API serialization
- `init()` returns bool — false means mode failed to initialize (heap allocation failure or resource unavailable)
- **Modes own their flight cycling state** — `_currentFlightIndex` and `_lastCycleMs` migrate from NeoMatrixDisplay into each mode's private members (~12 bytes per mode)
- **Empty flight vector is valid input to `render()`** — modes decide their own idle/loading state (ClassicCardMode shows "...", LiveFlightCardMode may show a different idle display)
- **Modes must NOT call `FastLED.show()`** — they write to the pixel buffer only; frame commit is the display task's responsibility

### Decision D2: ModeRegistry — Static Table with Cooperative Switch Serialization

```cpp
enum class SwitchState : uint8_t {
    IDLE,
    REQUESTED,
    SWITCHING
};

struct ModeEntry {
    const char* id;                           // e.g., "classic_card"
    const char* displayName;                  // e.g., "Classic Card"
    DisplayMode* (*factory)();                // factory function returning new instance
    uint32_t (*memoryRequirement)();          // static function — no instance needed
    uint8_t priority;                         // display order in Mode Picker
};

class ModeRegistry {
public:
    static void init(const ModeEntry* table, uint8_t count);
    static bool requestSwitch(const char* modeId);    // called from Core 1 (API)
    static void tick(const RenderContext& ctx,         // called from display task (Core 0)
                     const std::vector<FlightInfo>& flights);
    static DisplayMode* getActiveMode();
    static const char* getActiveModeId();
    static const ModeEntry* getModeTable();
    static uint8_t getModeCount();
    static SwitchState getSwitchState();
    static const char* getLastError();

private:
    static const ModeEntry* _table;
    static uint8_t _count;
    static DisplayMode* _activeMode;
    static uint8_t _activeModeIndex;
    static std::atomic<uint8_t> _requestedIndex;   // cross-core: Core 1 writes, Core 0 reads
    static SwitchState _switchState;
    static char _lastError[64];
    static bool _nvsWritePending;                   // debounced NVS persistence
    static unsigned long _lastSwitchMs;
};
```

**Static allocation:** `_table` is a pointer to a const array in BSS/data (defined in main.cpp). `_activeMode` is the only heap pointer — the single active mode instance. `_requestedIndex` is atomic for cross-core safety. `_lastError` is a fixed-size char array — no String allocation.

**Switch flow (inside `tick()`, runs on Core 0):**

1. Check `_requestedIndex` — if different from `_activeModeIndex`, begin switch
2. Set `_switchState = SWITCHING`
3. Call `_activeMode->teardown()` — frees mode's internal heap buffers, but **do NOT delete the mode object yet** (the ~32-64 byte shell remains for safe restoration)
4. Measure `ESP.getFreeHeap()`
5. Call `_table[requestedIndex].memoryRequirement()` — static function, no instantiation
6. **If heap sufficient:**
   - `delete _activeMode` — free the old shell
   - Create new mode via `_table[requestedIndex].factory()`
   - Call `init(ctx)` on new mode
   - If `init()` succeeds: update `_activeModeIndex`, set `_nvsWritePending = true`, clear error
   - If `init()` fails: `delete` the new mode, re-create previous mode via factory, call `init(ctx)`, set error "Mode initialization failed"
7. **If heap insufficient:**
   - Call `_activeMode->init(ctx)` to re-initialize the still-allocated previous mode
   - Set error "Insufficient memory for [mode name]"
8. Set `_switchState = IDLE`
9. **NVS debounce:** If `_nvsWritePending` and `millis() - _lastSwitchMs > 2000`, write to NVS via `ConfigManager::setDisplayMode()`, clear flag

**Key design choices:**
- **Teardown-before-delete pattern** — previous mode object shell stays allocated until new mode is confirmed, enabling safe restoration without re-allocation
- **`init()` failure treated identically to heap guard failure** — restore previous mode, set error, propagate to API
- **`tick()` is allowed to exceed the 16ms frame budget during SWITCHING state** — the wall goes briefly blank (fillScreen(0) during teardown), consistent with the PRD's "brief blank transition" design. This is a documented accepted tradeoff, not a bug
- **NVS write debounced** — 2-second quiet period after last switch, same pattern as ConfigManager's `schedulePersist()`. Prevents 10 NVS writes during rapid-switch stress test (NFR S2)

### Decision D3: NeoMatrixDisplay Refactoring — Responsibility Split

**Post-refactoring, NeoMatrixDisplay retains three roles:**

1. **Hardware owner** — LED strip initialization, matrix object creation, brightness control
2. **Frame committer** — `show()` method wrapping `FastLED.show()`, called by display task after mode render
3. **Shared resource provider** — logo buffer, matrix reference, dimensions, layout result

**What gets extracted from NeoMatrixDisplay:**
- `renderZoneFlight()`, `renderFlightZone()`, `renderTelemetryZone()`, `renderLogoZone()` → `ClassicCardMode`
- `_currentFlightIndex`, `_lastCycleMs` → per-mode internal state
- `displayFlights()` → no longer called from display task (see BaseDisplay note below)

**What stays in NeoMatrixDisplay:**
- Hardware lifecycle: `initialize()`, `clear()`, `rebuildMatrix()`, `reconfigureFromConfig()`
- Brightness control: `updateBrightness()`
- Frame commit: new `show()` method
- Rendering context factory: new `buildRenderContext()` method
- Status display: `displayMessage()`, `showLoading()`, `displayLoadingScreen()`
- Emergency fallback: `displaySingleFlightCard()` retained for FR36 (no-mode-active safety net)
- Calibration/positioning: `setCalibrationMode()`, `setPositioningMode()`, `renderCalibrationPattern()`, `renderPositioningPattern()`
- Shared resources: `_logoBuffer[1024]`, `_matrix`, `_layout`
- Helper methods: `drawTextLine()`, `truncateToColumns()`, `drawBitmapRGB565()`, `formatTelemetryValue()` — these are rendering utilities. Modes that need them can either reimplement or we extract them into a shared `utils/DisplayUtils.h`

**New public API additions:**

```cpp
// Rendering context factory — assembles context from NeoMatrixDisplay internals
RenderContext buildRenderContext() const;

// Frame commit — separated from rendering
void show();  // wraps FastLED.show()

// Emergency fallback (FR36 — ensures valid display when no mode can init)
void displayFallbackCard(const std::vector<FlightInfo>& flights);
```

**`buildRenderContext()` implementation:**

```cpp
RenderContext NeoMatrixDisplay::buildRenderContext() const {
    DisplayConfig disp = ConfigManager::getDisplay();
    TimingConfig timing = ConfigManager::getTiming();
    return {
        _matrix,
        _layout,
        _matrix->Color(disp.text_color_r, disp.text_color_g, disp.text_color_b),
        disp.brightness,
        const_cast<uint16_t*>(_logoBuffer),
        static_cast<uint16_t>(timing.display_cycle * 1000)
    };
}
```

**Optimization:** The display task caches the `RenderContext` and only rebuilds it when `g_configChanged` fires. Saves ~20 ConfigManager reads per second at 20fps with zero behavioral difference.

**BaseDisplay interface — retained, not deprecated:**

`BaseDisplay` defines `initialize()`, `clear()`, and `displayFlights()`. NeoMatrixDisplay continues to implement `initialize()` and `clear()` (called from `main.cpp setup()`). `displayFlights()` remains implemented but is no longer called from the display task — the task uses `ModeRegistry::tick()` + `show()` instead. No breaking interface change needed. The method can route to `displayFallbackCard()` internally for backward compatibility.

### Decision D4: Display Task Integration — Cached Context + ModeRegistry::tick()

**New globals in main.cpp:**

```cpp
// Static mode table — built at compile time, stored in BSS/data
static DisplayMode* classicCardFactory() { return new ClassicCardMode(); }
static DisplayMode* liveFlightCardFactory() { return new LiveFlightCardMode(); }
static uint32_t classicCardMemReq() { return ClassicCardMode::MEMORY_REQUIREMENT; }
static uint32_t liveFlightCardMemReq() { return LiveFlightCardMode::MEMORY_REQUIREMENT; }

static const ModeEntry MODE_TABLE[] = {
    { "classic_card", "Classic Card", classicCardFactory, classicCardMemReq, 0 },
    { "live_flight",  "Live Flight Card", liveFlightCardFactory, liveFlightCardMemReq, 1 },
};
static constexpr uint8_t MODE_COUNT = sizeof(MODE_TABLE) / sizeof(MODE_TABLE[0]);
```

**In `setup()`:**

```cpp
ModeRegistry::init(MODE_TABLE, MODE_COUNT);
// Restore last active mode from NVS, default to "classic_card"
String savedMode = ConfigManager::getDisplayMode();
if (!ModeRegistry::requestSwitch(savedMode.c_str())) {
    ModeRegistry::requestSwitch("classic_card");
}
```

**In `displayTask()` — revised rendering block:**

```cpp
// Cache rendering context — rebuild only on config change
static RenderContext cachedCtx;
static bool ctxInitialized = false;

if (g_configChanged.exchange(false) || !ctxInitialized) {
    // ... existing config change handling (brightness, hardware) ...
    cachedCtx = g_display.buildRenderContext();
    ctxInitialized = true;
}

// Priority order (unchanged from existing):
// 1. Calibration mode
// 2. Positioning mode
// 3. Status messages
// 4. Mode rendering (NEW — replaces direct renderFlight call)

if (g_display.isCalibrationMode()) {
    g_display.renderCalibrationPattern();
} else if (g_display.isPositioningMode()) {
    g_display.renderPositioningPattern();
} else if (statusMessageVisible) {
    // ... existing status message handling ...
} else {
    // Read latest flight data from queue
    FlightDisplayData *ptr = nullptr;
    std::vector<FlightInfo> empty;
    const auto& flights = (g_flightQueue && xQueuePeek(g_flightQueue, &ptr, 0) == pdTRUE && ptr)
                          ? ptr->flights : empty;

    // Tick mode registry — handles switch requests + renders active mode
    ModeRegistry::tick(cachedCtx, flights);
}

// Frame commit — always last, regardless of which path rendered
g_display.show();
```

**Cross-core coordination pattern:**

```
Browser → POST /api/display/mode → WebPortal handler (Core 1, async TCP task)
    → ModeRegistry::requestSwitch(modeId) → sets _requestedIndex atomic
Display task (Core 0) → ModeRegistry::tick() checks _requestedIndex at top of call
    → If changed: executes full switch lifecycle within tick()
    → Sets _switchState for API polling
```

This mirrors the existing `g_configChanged` pattern — no new concurrency primitives, no mutexes, no FreeRTOS queues for mode switching.

### Decision D5: Mode Switch API — Two Endpoints

Following existing `/api/*` JSON envelope pattern. Uses nested path `/api/display/` to scope mode operations (new pattern, but logically groups display-related endpoints for future extensibility).

**`GET /api/display/modes`** — list available modes + active mode + transition state:

```json
{
    "ok": true,
    "data": {
        "active": "live_flight",
        "switch_state": "idle",
        "modes": [
            {
                "id": "classic_card",
                "name": "Classic Card",
                "description": "Three-line flight card: airline, route, aircraft",
                "zones": [
                    { "label": "Logo", "x": 0, "y": 0, "w": 20, "h": 100 },
                    { "label": "Airline", "x": 20, "y": 0, "w": 80, "h": 33 },
                    { "label": "Route", "x": 20, "y": 33, "w": 80, "h": 33 },
                    { "label": "Aircraft", "x": 20, "y": 66, "w": 80, "h": 34 }
                ]
            },
            {
                "id": "live_flight",
                "name": "Live Flight Card",
                "description": "Enriched telemetry: altitude, speed, heading, vertical rate",
                "zones": [
                    { "label": "Logo", "x": 0, "y": 0, "w": 20, "h": 100 },
                    { "label": "Airline", "x": 20, "y": 0, "w": 40, "h": 50 },
                    { "label": "Route", "x": 60, "y": 0, "w": 40, "h": 50 },
                    { "label": "Alt", "x": 20, "y": 50, "w": 20, "h": 50 },
                    { "label": "Spd", "x": 40, "y": 50, "w": 20, "h": 50 },
                    { "label": "Hdg", "x": 60, "y": 50, "w": 20, "h": 50 },
                    { "label": "VRate", "x": 80, "y": 50, "w": 20, "h": 50 }
                ]
            }
        ]
    }
}
```

**`POST /api/display/mode`** — request mode switch:

Request: `{ "mode": "live_flight" }`

Success: `{ "ok": true, "data": { "switching_to": "live_flight" } }`

Error (heap guard): `{ "ok": false, "error": "Insufficient memory for Live Flight Card", "code": "HEAP_INSUFFICIENT" }`

Error (unknown mode): `{ "ok": false, "error": "Unknown mode: radar_sweep", "code": "MODE_NOT_FOUND" }`

Error (in progress): `{ "ok": false, "error": "Mode switch already in progress", "code": "SWITCH_IN_PROGRESS" }`

**Synchronous mode activation:** The POST endpoint blocks until the mode switch is complete (new mode rendering) or fails (heap guard, unknown mode). The response contains the confirmed active mode — no client-side polling needed. "Switching..." in the UI is the POST in flight; "Active" is the 200 response. This gives ESPHome-grade state honesty with zero client-side complexity. Sub-2-second switch means the POST typically returns in under 2 seconds.

**NFR P4 compliance:** `GET /api/display/modes` serializes the static mode table (BSS/data) + reads atomic `_activeModeIndex` and `_switchState`. Zero heap allocation. Completes in <10ms.

### Decision D6: NVS Persistence — Single Key with Debounced Write

**NVS key:** `display_mode` (12 chars, within 15-char limit)
**Namespace:** `"flightwall"` (existing — verified non-colliding with all 23+ existing keys per NFR S6)
**Type:** String (mode ID, e.g., `"classic_card"`, `"live_flight"`)
**Default:** `"classic_card"` (preserves pre-upgrade behavior per FR29)

**Persistence timing:** Debounced — `ModeRegistry::tick()` sets `_nvsWritePending` flag after successful switch. Write occurs when flag is set AND >2 seconds have elapsed since last switch. This prevents 10 NVS writes during rapid-switch stress testing (NFR S2) while ensuring the selection persists for power-cycle recovery (FR27-FR28).

**ConfigManager integration:**

```cpp
// New in ConfigManager
static String getDisplayMode();          // reads "display_mode" from NVS, returns "classic_card" if absent
static void setDisplayMode(const String& modeId);  // writes "display_mode" to NVS
```

Two new methods, no new config struct. The `display_mode` key is NOT hot-reload and NOT reboot-required — it's a fire-and-forget debounced write after each successful switch.

**Boot restoration:** In `setup()`, `ConfigManager::getDisplayMode()` reads from NVS → passed to `ModeRegistry::requestSwitch()`. If key doesn't exist (first boot after upgrade from Foundation Release), returns `"classic_card"` default.

### Decision D7: Mode Picker UI — Dashboard Section with Client-Side Wireframes

**Dashboard integration:**
- New section in `dashboard.html` positioned #2 in card order (after Display, before Timing) — "remote control" cadence demands top-of-fold placement
- Loads mode list via `GET /api/display/modes` on page load
- Renders mode cards with schematic wireframe previews from zone metadata
- Entire non-active mode card is the tap target (not a button within the card — Philips Hue scene picker pattern)
- Tap triggers synchronous `POST /api/display/mode` — returns after mode is rendering or fails

**Wireframe rendering:** CSS Grid with `<div>` elements, data-driven from zone metadata JSON. Each mode's API response includes `grid` (rows/cols) and `zones` array (label, row, col, rowSpan, colSpan). JS creates a grid container and positions zone divs via `grid-row`/`grid-column`. ~5 lines of rendering JS. No canvas, no SVG, no server-rendered bitmaps. New modes get schematics automatically — zero UI code changes.

**Transition feedback:**
- Tap non-active card → card enters "Switching..." state (pulsing border, dimmed, `.switching` CSS class). All sibling cards enter `.disabled` state (opacity 0.5, `pointer-events: none`).
- Synchronous POST returns → on success: new card gets `.active`, previous card returns to idle, siblings re-enabled. On error: tapped card shows scoped `--warning` error message, previous card remains `.active`, siblings re-enabled.
- No polling — "Switching..." is the POST in flight, "Active" is the 200 response.

**Upgrade notification (FR25-FR26):**
- Dual-source check: API returns `upgrade_notification: true` in `GET /api/display/modes` response AND browser checks `localStorage.getItem('flightwall_mode_notif_seen')`
- If both conditions met: show informational banner with "Try Live Flight Card" (primary action — activates mode immediately) + "Browse Modes" (scrolls to Mode Picker) + dismiss X
- All three actions clear both sources: `localStorage.setItem('flightwall_mode_notif_seen', '1')` + `POST /api/display/ack-notification` to clear firmware flag
- Either source cleared independently prevents reappearance

**Files affected:**
- `firmware/data-src/dashboard.html` — Mode Picker section container (`<div id="mode-picker">`) + notification banner markup
- `firmware/data-src/dashboard.js` — mode API calls, CSS Grid schematic rendering, card state management, notification logic (~40 lines total)
- `firmware/data-src/style.css` — mode card states (idle/switching/active/disabled/error), schematic zone styling, notification banner (~58 lines total)
- `firmware/data/*.gz` — rebuilt gzipped copies of all updated web assets

### Display System Decision Impact — Implementation Sequence

**Critical path (sequential):**
1. **DisplayMode interface + RenderContext** (`interfaces/DisplayMode.h`) — everything depends on this contract
2. **ModeRegistry** (`core/ModeRegistry.h/.cpp`) — switch logic, heap guard, tick()
3. **ClassicCardMode** (`modes/ClassicCardMode.h/.cpp`) — extraction from NeoMatrixDisplay with pixel-parity validation (highest-risk step)
4. **NeoMatrixDisplay refactoring** — remove extracted methods, add `show()` + `buildRenderContext()`, retain fallback
5. **Display task integration** (`main.cpp`) — replace renderFlight with ModeRegistry::tick() + show()
6. **LiveFlightCardMode** (`modes/LiveFlightCardMode.h/.cpp`) — second mode, validates the abstraction works

**Parallel with steps 3-6:**
7. **Mode Switch API** (`WebPortal.cpp`) — GET + POST endpoints, can be stubbed early
8. **NVS persistence** (`ConfigManager.h/.cpp`) — `getDisplayMode()` / `setDisplayMode()`, trivial addition

**After all firmware complete:**
9. **Mode Picker UI** (`dashboard.html/js/css`) — depends on working API endpoints
10. **Gzip rebuild** — all updated web assets

**Heap baseline measurement** must occur before step 1 — measure `ESP.getFreeHeap()` after full Foundation boot to establish the mode memory budget.

### Cross-Decision Dependencies

```
DisplayMode interface (D1)
    └── ModeRegistry (D2) — uses DisplayMode*, ModeEntry, RenderContext
        ├── ClassicCardMode — implements DisplayMode
        ├── LiveFlightCardMode — implements DisplayMode
        └── Display task (D4) — calls ModeRegistry::tick()
            └── NeoMatrixDisplay (D3) — provides RenderContext + show()

Mode Switch API (D5) — calls ModeRegistry::requestSwitch() + reads state
    └── Mode Picker UI (D7) — calls API, renders wireframes from zone descriptors

NVS persistence (D6) — called from ModeRegistry::tick() after successful switch
    └── Boot restoration in setup() — reads NVS, calls requestSwitch()
```

## Display System Release — Implementation Patterns & Consistency Rules

### Display System–Specific Conflict Points

The MVP architecture established 11 enforcement rules. The Foundation Release added 5 (rules 12-16). The Display System Release introduces 7 new pattern areas where AI agents could make inconsistent choices about the mode system.

### Mode Implementation Pattern

**File location:** All mode classes go in `firmware/modes/`. One `.h/.cpp` pair per mode. Mode files are NOT adapters (they don't touch hardware directly) and NOT core (they're pluggable rendering strategies).

```
firmware/modes/
├── ClassicCardMode.h
├── ClassicCardMode.cpp
├── LiveFlightCardMode.h
└── LiveFlightCardMode.cpp
```

**Class structure:**

```cpp
// modes/ClassicCardMode.h
#pragma once
#include "interfaces/DisplayMode.h"

class ClassicCardMode : public DisplayMode {
public:
    static constexpr uint32_t MEMORY_REQUIREMENT = 64;  // bytes — cycling state only

    bool init(const RenderContext& ctx) override;
    void render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) override;
    void teardown() override;
    const char* getName() const override;
    const ModeZoneDescriptor& getZoneDescriptor() const override;

private:
    // Per-mode cycling state (migrated from NeoMatrixDisplay)
    size_t _currentFlightIndex = 0;
    unsigned long _lastCycleMs = 0;
};
```

**Rules:**
- Constructor does NO heap allocation — all allocation happens in `init()`
- `teardown()` frees ALL heap allocated by `init()` — the mode object shell (~32-64 bytes) remains for safe restoration
- `MEMORY_REQUIREMENT` is a `static constexpr` — reflects worst-case heap allocated during `init()`, NOT the mode object size
- Modes store their own cycling state (index + timer) — do not rely on external cycling

### Rendering Context Discipline

**Modes must ONLY access data through `RenderContext`.** This is the critical isolation boundary that guarantees NFR C2 (Foundation feature compatibility).

**Allowed:**
```cpp
void ClassicCardMode::render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) {
    ctx.matrix->setCursor(x, y);                                       // ✅ GFX primitives via context
    ctx.matrix->setTextColor(ctx.textColor);                           // ✅ pre-computed color from context
    LogoManager::loadLogo(f.operator_icao, ctx.logoBuffer);            // ✅ shared buffer from context
    if (millis() - _lastCycleMs >= ctx.displayCycleMs) { }             // ✅ timing from context
    DisplayUtils::drawTextLine(ctx.matrix, x, y, text, ctx.textColor); // ✅ shared utility
}
```

**Forbidden:**
```cpp
void ClassicCardMode::render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) {
    DisplayConfig disp = ConfigManager::getDisplay();  // ❌ NEVER read ConfigManager from modes
    FastLED.show();                                     // ❌ NEVER call FastLED.show() from modes
    ctx.matrix->setBrightness(50);                      // ❌ NEVER modify brightness from modes
    uint8_t brt = ConfigManager::getSchedule().dim_brightness;  // ❌ NEVER access schedule config
}
```

**Anti-pattern: caching RenderContext.** Modes receive `ctx` by const reference each call. Do not store a copy — the display task may update the cached context between frames (on config change). Always use the passed-in reference.

### Shared Rendering Utilities

Helper methods currently in NeoMatrixDisplay that modes need. Extract into `utils/DisplayUtils.h` as free functions:

```cpp
// utils/DisplayUtils.h
#pragma once
#include <Adafruit_NeoMatrix.h>
#include <Arduino.h>

namespace DisplayUtils {
    void drawTextLine(Adafruit_NeoMatrix* matrix, int16_t x, int16_t y,
                      const String& text, uint16_t color);
    String truncateToColumns(const String& text, int maxColumns);
    String formatTelemetryValue(double value, const char* suffix, int decimals = 0);
    void drawBitmapRGB565(Adafruit_NeoMatrix* matrix, int16_t x, int16_t y,
                          uint16_t w, uint16_t h, const uint16_t* bitmap,
                          uint16_t zoneW, uint16_t zoneH);
}
```

**Rule:** Modes use `DisplayUtils::` functions. Do NOT copy rendering helpers into each mode class. Do NOT add NeoMatrixDisplay as a dependency of modes.

### Mode Registration Pattern

**Adding a new mode requires exactly two touch points:**

1. Create `modes/NewMode.h/.cpp` implementing `DisplayMode`
2. Add one entry to `MODE_TABLE[]` in `main.cpp`:

```cpp
static DisplayMode* newModeFactory() { return new NewMode(); }
static uint32_t newModeMemReq() { return NewMode::MEMORY_REQUIREMENT; }

static const ModeEntry MODE_TABLE[] = {
    { "classic_card", "Classic Card", classicCardFactory, classicCardMemReq, 0 },
    { "live_flight",  "Live Flight Card", liveFlightCardFactory, liveFlightCardMemReq, 1 },
    { "new_mode",     "New Mode", newModeFactory, newModeMemReq, 2 },  // ← one line
};
```

**Anti-pattern:** Do NOT modify ModeRegistry, NeoMatrixDisplay, WebPortal, FlightDataFetcher, or dashboard JS to add a mode. If a mode addition requires changes beyond the two touch points above, the architecture has a defect.

### Mode Picker UI Patterns

**Mode list rendering:** Follow existing dashboard card pattern — same CSS classes, same card layout structure. Mode cards are rendered dynamically from the `GET /api/display/modes` JSON response. The HTML template contains only a `<div id="mode-picker">` container (~10 lines); JS builds cards from API data.

**API interaction:** Every `fetch()` call follows existing rule 10: check `json.ok`, call `showToast()` on failure. The POST is synchronous — it returns after the mode is rendering (or fails). No polling loop.

```javascript
async function activateMode(modeId) {
    const cards = document.querySelectorAll('.mode-card:not(.active)');
    const card = document.querySelector(`[data-mode="${modeId}"]`);
    card.classList.add('switching');
    cards.forEach(c => { if (c !== card) c.classList.add('disabled'); });
    try {
        const res = await fetch('/api/display/mode', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ mode: modeId })
        });
        const json = await res.json();
        if (!json.ok) {
            card.classList.remove('switching');
            showCardError(card, json.error);
        } else {
            document.querySelector('.mode-card.active').classList.remove('active');
            card.classList.remove('switching');
            card.classList.add('active');
        }
    } catch (err) {
        card.classList.remove('switching');
        showToast('Connection failed', 'error');
    }
    cards.forEach(c => c.classList.remove('disabled'));
}
```

**Wireframe rendering:** CSS Grid with `<div>` zone elements. Grid dimensions and zone positions from API metadata. ~5 lines of JS to render. No canvas, no SVG, no external drawing libraries.

**Transition state:** Synchronous — card shows "Switching..." with `.switching` CSS class while POST is in flight. Sibling cards get `.disabled`. POST response resolves to "Active" or scoped error. No polling needed.

### Updated Enforcement Guidelines

**All AI agents implementing Display System stories MUST also follow (in addition to rules 1-16):**

17. Mode classes go in `firmware/modes/` — one `.h/.cpp` pair per mode
18. Modes must ONLY access data through `RenderContext` — zero `ConfigManager`, `WiFiManager`, or `SystemStatus` reads from inside a mode
19. Modes must NOT call `FastLED.show()` — frame commit is exclusively the display task's responsibility after `ModeRegistry::tick()` returns
20. All heap allocation in modes happens in `init()`, all deallocation in `teardown()` — constructors and per-frame `render()` must not allocate heap
21. Shared rendering helpers (`drawTextLine`, `truncateToColumns`, `formatTelemetryValue`, `drawBitmapRGB565`) must be called from `utils/DisplayUtils.h` — do not duplicate into mode classes
22. Adding a new mode requires exactly two touch points: new file in `modes/` + one line in `MODE_TABLE[]` in main.cpp — if more changes are needed, the architecture has a defect
23. `MEMORY_REQUIREMENT` is a `static constexpr` on each mode class — reflects worst-case heap from `init()`, not object size

## Display System Release — Project Structure & File Change Map

### Complete Project Directory Structure (Display System Additions)

```
firmware/
├── platformio.ini                          # UPDATED: build_src_filter + build_flags
├── custom_partitions.csv                   # UNCHANGED (Foundation layout)
│
├── interfaces/
│   ├── BaseDisplay.h                       # UNCHANGED (retained, not deprecated)
│   ├── BaseFlightFetcher.h                 # UNCHANGED
│   ├── BaseStateVectorFetcher.h            # UNCHANGED
│   └── DisplayMode.h                       # ← NEW: DisplayMode abstract class,
│                                           #         RenderContext, ModeZoneDescriptor,
│                                           #         ZoneRegion, SwitchState enum
│                                           #         Includes FlightInfo.h and LayoutEngine.h
│                                           #         directly (transitive for all modes)
│
├── core/
│   ├── ConfigManager.h                     # UPDATED: +getDisplayMode(), +setDisplayMode()
│   ├── ConfigManager.cpp                   # UPDATED: +getDisplayMode(), +setDisplayMode() impl
│   ├── ModeRegistry.h                      # ← NEW: ModeRegistry static class + ModeEntry struct
│   ├── ModeRegistry.cpp                    # ← NEW: init(), requestSwitch(), tick(),
│   │                                       #         switch lifecycle, NVS debounce
│   ├── FlightDataFetcher.h                 # UNCHANGED
│   ├── FlightDataFetcher.cpp               # UNCHANGED
│   ├── LayoutEngine.h                      # UNCHANGED
│   ├── LayoutEngine.cpp                    # UNCHANGED
│   ├── LogoManager.h                       # UNCHANGED
│   ├── LogoManager.cpp                     # UNCHANGED
│   ├── SystemStatus.h                      # UNCHANGED
│   └── SystemStatus.cpp                    # UNCHANGED
│
├── modes/                                  # ← NEW DIRECTORY
│   ├── ClassicCardMode.h                   # ← NEW: ClassicCardMode declaration
│   ├── ClassicCardMode.cpp                 # ← NEW: extracted from NeoMatrixDisplay
│   │                                       #         renderZoneFlight, renderFlightZone,
│   │                                       #         renderTelemetryZone, renderLogoZone
│   ├── LiveFlightCardMode.h                # ← NEW: LiveFlightCardMode declaration
│   └── LiveFlightCardMode.cpp              # ← NEW: telemetry-enriched flight card
│
├── adapters/
│   ├── NeoMatrixDisplay.h                  # UPDATED: +show(), +buildRenderContext(),
│   │                                       #          +displayFallbackCard()
│   │                                       #          -renderZoneFlight(), -renderFlightZone(),
│   │                                       #          -renderTelemetryZone(), -renderLogoZone()
│   ├── NeoMatrixDisplay.cpp                # UPDATED: ~140 lines extracted to ClassicCardMode;
│   │                                       #          +show(), +buildRenderContext(),
│   │                                       #          +displayFallbackCard() impls
│   ├── WebPortal.h                         # UPDATED: +_handleGetDisplayModes(),
│   │                                       #          +_handlePostDisplayMode()
│   ├── WebPortal.cpp                       # UPDATED: +GET /api/display/modes handler,
│   │                                       #          +POST /api/display/mode handler
│   ├── WiFiManager.h                       # UNCHANGED
│   ├── WiFiManager.cpp                     # UNCHANGED
│   ├── AeroAPIFetcher.h                    # UNCHANGED
│   ├── AeroAPIFetcher.cpp                  # UNCHANGED
│   ├── FlightWallFetcher.h                 # UNCHANGED
│   ├── FlightWallFetcher.cpp               # UNCHANGED
│   ├── OpenSkyFetcher.h                    # UNCHANGED
│   └── OpenSkyFetcher.cpp                  # UNCHANGED
│
├── utils/
│   ├── GeoUtils.h                          # UNCHANGED
│   ├── Log.h                               # UNCHANGED
│   ├── DisplayUtils.h                      # ← NEW: shared rendering free functions
│   └── DisplayUtils.cpp                    # ← NEW: drawTextLine, truncateToColumns,
│                                           #         formatTelemetryValue, drawBitmapRGB565
│
├── models/
│   ├── FlightInfo.h                        # UNCHANGED
│   ├── StateVector.h                       # UNCHANGED
│   └── AirportInfo.h                       # UNCHANGED
│
├── config/
│   ├── HardwareConfiguration.h             # UNCHANGED
│   ├── TimingConfiguration.h               # UNCHANGED
│   ├── WiFiConfiguration.h                 # UNCHANGED
│   ├── APIConfiguration.h                  # UNCHANGED
│   └── UserConfiguration.h                 # UNCHANGED
│
├── src/
│   └── main.cpp                            # UPDATED: +MODE_TABLE[], +factory functions,
│                                           #          +ModeRegistry::init() in setup(),
│                                           #          +heap baseline log after full init,
│                                           #          displayTask() replaces renderFlight
│                                           #          with ModeRegistry::tick()+show()
│
├── data-src/                               # Web asset sources
│   ├── dashboard.html                      # UPDATED: +Mode Picker section in nav + content
│   ├── dashboard.js                        # UPDATED: +mode API calls, +CSS Grid schematic
│   │                                       #          rendering, +card state mgmt, +notification
│   ├── style.css                           # UPDATED: +mode card styling
│   ├── health.html                         # UNCHANGED
│   ├── health.js                           # UNCHANGED
│   ├── common.js                           # UNCHANGED
│   ├── wizard.html                         # UNCHANGED
│   └── wizard.js                           # UNCHANGED
│
├── data/                                   # Gzipped assets (served by WebPortal)
│   ├── dashboard.html.gz                   # REBUILD after dashboard.html changes
│   ├── dashboard.js.gz                     # REBUILD after dashboard.js changes
│   ├── style.css.gz                        # REBUILD after style.css changes
│   ├── common.js.gz                        # UNCHANGED
│   ├── health.html.gz                      # UNCHANGED
│   ├── health.js.gz                        # UNCHANGED
│   ├── wizard.html.gz                      # UNCHANGED
│   ├── wizard.js.gz                        # UNCHANGED
│   ├── test.txt                            # UNCHANGED
│   └── logos/                              # UNCHANGED (99 ICAO .bin files)
│
└── test/
    ├── test_config_manager/test_main.cpp   # UNCHANGED (existing test)
    ├── test_layout_engine/test_main.cpp    # UNCHANGED (existing test)
    ├── test_logo_manager/test_main.cpp     # UNCHANGED (existing test)
    ├── test_telemetry_conversion/test_main.cpp  # UNCHANGED (existing test)
    └── test_mode_registry/test_main.cpp    # ← NEW: ModeRegistry switch lifecycle,
                                            #         heap guard, NVS debounce tests
```

### File Change Summary

| Action | Count | Files |
|--------|-------|-------|
| **NEW** | 10 | `interfaces/DisplayMode.h`, `core/ModeRegistry.h`, `core/ModeRegistry.cpp`, `modes/ClassicCardMode.h`, `modes/ClassicCardMode.cpp`, `modes/LiveFlightCardMode.h`, `modes/LiveFlightCardMode.cpp`, `utils/DisplayUtils.h`, `utils/DisplayUtils.cpp`, `test/test_mode_registry/test_main.cpp` |
| **UPDATED** | 12 | `platformio.ini`, `adapters/NeoMatrixDisplay.h`, `adapters/NeoMatrixDisplay.cpp`, `adapters/WebPortal.h`, `adapters/WebPortal.cpp`, `core/ConfigManager.h`, `core/ConfigManager.cpp`, `src/main.cpp`, `data-src/dashboard.html`, `data-src/dashboard.js`, `data-src/style.css`, `data/*.gz` (3 rebuilds) |
| **UNCHANGED** | 30+ | All other firmware files, config headers, models, fetchers, WiFiManager, LayoutEngine, LogoManager, SystemStatus, wizard assets, health assets |
| **NEW DIRECTORY** | 1 | `firmware/modes/` |

### PlatformIO Build Configuration Update

**Current `platformio.ini` `build_src_filter`:**

```ini
build_src_filter =
    +<*.cpp>
    +<**/*.cpp>
    +<../adapters/*.cpp>
    +<../core/*.cpp>
```

**Required additions:**

```ini
build_src_filter =
    +<*.cpp>
    +<**/*.cpp>
    +<../adapters/*.cpp>
    +<../core/*.cpp>
    +<../modes/*.cpp>          # ← NEW: ClassicCardMode, LiveFlightCardMode
    +<../utils/*.cpp>          # ← NEW: DisplayUtils
```

**Required `build_flags` addition:**

```ini
build_flags =
    -I .
    -I include
    -I src
    -I adapters
    -I core
    -I models
    -I interfaces
    -I utils
    -I config
    -I modes                   # ← NEW: mode headers
    -D LOG_LEVEL=3
```

Without these changes, mode and utility `.cpp` files will not compile. This was identified as a **blocker** during party mode review.

### Architectural Boundaries

**API Boundaries (HTTP):**

| Endpoint | Method | Handler | Cross-Core | Purpose |
|----------|--------|---------|------------|---------|
| `/api/display/modes` | GET | WebPortal (Core 1) | Read-only atomic | List modes, active mode, switch state |
| `/api/display/mode` | POST | WebPortal (Core 1) | Atomic write | Request mode switch |
| `/api/settings` | GET/POST | WebPortal (Core 1) | Unchanged | Existing settings (no new keys) |
| `/api/status` | GET | WebPortal (Core 1) | Unchanged | Health/status |

**Component Boundaries (Firmware):**

```
┌──────────────────────────────────────────────────────────────┐
│  Core 1 (WiFi/Web/API task)                                  │
│  ┌────────────────┐                                          │
│  │   WebPortal    │─── POST /api/display/mode ──────────┐   │
│  │  (HTTP handler)│─── GET /api/display/modes (read) ─┐  │   │
│  └────────────────┘                                   │  │   │
└───────────────────────────────────────────────────────┼──┼───┘
                                                        │  │
    atomic read ────────────────────────────────────────┘  │
    atomic write (requestSwitch) ──────────────────────────┘
                                                        │  │
┌───────────────────────────────────────────────────────┼──┼───┐
│  Core 0 (Display task)                                │  │   │
│  ┌────────────────┐   ┌──────────────┐                │  │   │
│  │  ModeRegistry  │──→│ ActiveMode   │ (DisplayMode*) │  │   │
│  │  ::tick()      │   │ .render()    │                │  │   │
│  └───────┬────────┘   └──────┬───────┘                │  │   │
│          │                   │                                │
│          │           ┌───────▼────────┐                       │
│          │           │  LogoManager   │ (static utility —     │
│          │           │  ::loadLogo()  │  accepted dependency) │
│          │           └────────────────┘                       │
│  ┌───────▼──────────────┐   ┌────────────────────────┐       │
│  │  NeoMatrixDisplay    │   │  DisplayUtils          │       │
│  │  .buildRenderContext()│   │  (free functions)      │       │
│  │  .show()             │   │  drawTextLine, etc.    │       │
│  └──────────────────────┘   └────────────────────────┘       │
└──────────────────────────────────────────────────────────────┘
```

**Data Boundaries:**

| Boundary | Direction | Mechanism |
|----------|-----------|-----------|
| WebPortal → ModeRegistry | Core 1 → Core 0 | `std::atomic<uint8_t> _requestedIndex` |
| ModeRegistry → ConfigManager | Core 0 → NVS | `setDisplayMode()` (debounced 2s) |
| ConfigManager → ModeRegistry | NVS → Core 0 | `getDisplayMode()` at boot only |
| NeoMatrixDisplay → Modes | Core 0 internal | `RenderContext` const ref (isolation barrier) |
| Modes → LogoManager | Core 0 internal | `LogoManager::loadLogo()` static call (accepted utility dependency — fills shared buffer from RenderContext) |
| Modes → NeoMatrixDisplay | **FORBIDDEN** | Modes never access NeoMatrixDisplay directly |
| Modes → ConfigManager | **FORBIDDEN** | Modes never read config directly (rule 18) |
| Dashboard JS → API | Browser → Core 1 | fetch() JSON envelope, synchronous POST for mode switch |

### Requirements to Structure Mapping

**FR Category → File Mapping:**

| FR Group | FRs | Primary Files | Supporting Files |
|----------|-----|---------------|-----------------|
| DisplayMode Interface | FR1-FR4 | `interfaces/DisplayMode.h` | `models/FlightInfo.h`, `core/LayoutEngine.h` (included transitively) |
| Mode Registry & Lifecycle | FR5-FR10 | `core/ModeRegistry.h`, `core/ModeRegistry.cpp` | `src/main.cpp` (MODE_TABLE), `test/test_mode_registry/test_main.cpp` |
| Classic Card Mode | FR11-FR13 | `modes/ClassicCardMode.h`, `modes/ClassicCardMode.cpp` | `utils/DisplayUtils.h/.cpp`, `adapters/NeoMatrixDisplay.cpp` (extraction source), `core/LogoManager.h` (loadLogo) |
| Live Flight Card Mode | FR14-FR16 | `modes/LiveFlightCardMode.h`, `modes/LiveFlightCardMode.cpp` | `utils/DisplayUtils.h/.cpp`, `models/FlightInfo.h` (telemetry fields), `core/LogoManager.h` (loadLogo) |
| Mode Picker UI | FR17-FR26 | `data-src/dashboard.html`, `data-src/dashboard.js`, `data-src/style.css` | `data/*.gz` (rebuild) |
| Mode Persistence | FR27-FR29 | `core/ConfigManager.h/.cpp` | `core/ModeRegistry.cpp` (debounce), `src/main.cpp` (boot restore) |
| Mode Switch API | FR30-FR33 | `adapters/WebPortal.h`, `adapters/WebPortal.cpp` | `core/ModeRegistry.h` (requestSwitch, getters) |
| Display Infrastructure | FR34-FR36 | `adapters/NeoMatrixDisplay.h/.cpp`, `src/main.cpp` | `core/ModeRegistry.cpp` (tick integration), `platformio.ini` (build config) |

**Cross-Cutting Concerns → File Mapping:**

| Concern | Files Touched |
|---------|--------------|
| RenderContext isolation (NFR C2) | `interfaces/DisplayMode.h` (struct def), `adapters/NeoMatrixDisplay.cpp` (buildRenderContext), all `modes/*.cpp` (consumers) |
| Cross-core coordination | `core/ModeRegistry.h` (atomic), `adapters/WebPortal.cpp` (requestSwitch caller), `src/main.cpp` (tick caller) |
| Heap lifecycle (NFR S1, S3, S4) | `core/ModeRegistry.cpp` (switch flow), all `modes/*.cpp` (init/teardown) |
| Heap baseline measurement | `src/main.cpp` — `ESP.getFreeHeap()` log after full init, before `ModeRegistry::init()` |
| NVS debounce | `core/ModeRegistry.cpp` (2s timer), `core/ConfigManager.cpp` (read/write) |
| Shared rendering utilities | `utils/DisplayUtils.h/.cpp` (extracted helpers), `modes/*.cpp` (consumers) |
| PlatformIO build discovery | `platformio.ini` (build_src_filter, build_flags) |

### Include Path Conventions for Mode Files

Mode source files use path-from-firmware-root includes, consistent with the existing codebase pattern in `main.cpp`:

```cpp
// modes/ClassicCardMode.cpp — canonical include style
#include "interfaces/DisplayMode.h"    // ✅ base class (transitively includes FlightInfo.h, LayoutEngine.h)
#include "utils/DisplayUtils.h"        // ✅ shared rendering helpers
#include "core/LogoManager.h"          // ✅ accepted static utility dependency
#include "modes/ClassicCardMode.h"     // ✅ own header
```

`DisplayMode.h` includes `"models/FlightInfo.h"` and `"core/LayoutEngine.h"` directly — modes receive these transitively and do not need to include them separately.

### Integration Points

**Internal Communication:**
- Display task calls `ModeRegistry::tick()` every frame (~20fps) — passes cached `RenderContext` + flight vector
- `ModeRegistry::tick()` delegates to `_activeMode->render()` during IDLE, or executes switch lifecycle during REQUESTED/SWITCHING
- After `tick()` returns, display task calls `g_display.show()` — separates rendering from frame commit
- WebPortal calls `ModeRegistry::requestSwitch()` from async handler — sets atomic flag only, returns immediately
- `ModeRegistry::tick()` calls `ConfigManager::setDisplayMode()` after debounce timer expires (2s post-switch)

**External Integrations:**
- No new external API calls or network dependencies
- Mode Picker UI communicates exclusively through existing `/api/*` pattern (fetch + JSON envelope)

**Data Flow (Mode Switch):**

```
Browser click "Activate"
  → POST /api/display/mode { "mode": "live_flight" }
  → WebPortal::_handlePostDisplayMode() [Core 1]
  → ModeRegistry::requestSwitch("live_flight") → sets _requestedIndex atomic
  → returns { "ok": true, "switching_to": "live_flight" }
  → Browser polls GET /api/display/modes at 500ms

Next display task tick [Core 0]:
  → ModeRegistry::tick() detects _requestedIndex != _activeModeIndex
  → _activeMode->teardown() → frees mode heap
  → ESP.getFreeHeap() check vs memoryRequirement()
  → factory() → new mode → init(ctx)
  → success: update index, set NVS pending, clear error
  → _switchState = IDLE

After 2s debounce [Core 0, inside tick()]:
  → ConfigManager::setDisplayMode("live_flight") → NVS write

Next browser poll:
  → GET /api/display/modes returns switch_state: "idle", active: "live_flight"
  → UI updates to show Live Flight Card as active
```

### Development Workflow Integration

**Build process:** PlatformIO compiles all `.cpp` files per `build_src_filter` in `platformio.ini`. The updated filter includes `modes/` and `utils/` directories alongside existing `adapters/` and `core/`.

**Gzip rebuild:** After modifying `data-src/*.html`, `data-src/*.js`, or `data-src/*.css`:
```bash
gzip -c firmware/data-src/dashboard.html > firmware/data/dashboard.html.gz
gzip -c firmware/data-src/dashboard.js > firmware/data/dashboard.js.gz
gzip -c firmware/data-src/style.css > firmware/data/style.css.gz
```

**Upload sequence:** `~/.platformio/penv/bin/pio run` (build) → `~/.platformio/penv/bin/pio run -t uploadfs` (flash filesystem) → `~/.platformio/penv/bin/pio run -t upload` (flash firmware)

**Heap baseline prerequisite:** Before implementing any Display System code, flash current Foundation firmware and record `ESP.getFreeHeap()` from serial monitor after full boot (WiFi connected, web server active, NTP synced, flight data in queue). This establishes the mode memory budget and validates whether the 30KB floor (NFR S4) is achievable.

### Party Mode Refinements Incorporated

- `platformio.ini` promoted from UNCHANGED to UPDATED (blocker — modes/ and utils/ wouldn't compile)
- `LogoManager::loadLogo()` documented as accepted static utility dependency from modes (not a rule 18 violation)
- `DisplayMode.h` specified to include `FlightInfo.h` and `LayoutEngine.h` directly for transitive access
- `test/test_mode_registry/test_main.cpp` added to NEW files (signals testing intent for highest-risk component)
- Heap baseline measurement location specified: `ESP.getFreeHeap()` log in `setup()` after full init
- `ModeRegistry → ConfigManager::setDisplayMode()` added to data boundaries table
- Pixel parity validation (NFR C1) noted as hardware visual validation — not automatable in unit tests

---

# Delight Release — Architecture Extension

_Extends the MVP + Foundation + Display System architecture with Clock Mode, Departures Board, animated transitions, mode scheduling, mode-specific settings, and OTA Pull from GitHub Releases. All prior architectural decisions remain in effect — this section adds new decisions and updates existing ones where the Delight PRD introduces changes._

## Delight Release — Project Context Analysis

### Requirements Overview

**Functional Requirements (43 FRs across 9 groups):**

| Group | FRs | Architectural Impact |
|-------|-----|---------------------|
| Clock Mode | FR1-FR4 | **Minor:** New DisplayMode implementation (rule 22 — two touch points); idle-fallback trigger integrated with mode orchestration |
| Departures Board | FR5-FR9 | **Minor:** New DisplayMode with multi-row rendering, dynamic row add/remove, configurable row count (rule 22 — two touch points) |
| Mode Transitions | FR10-FR12 | **Medium:** Fade transition phase added to ModeRegistry switch lifecycle; dual RGB565 frame buffers (~20KB transient allocation) |
| Mode Scheduling | FR13-FR17, FR36, FR39 | **Major:** Mode orchestration state machine (MANUAL / SCHEDULED / IDLE_FALLBACK); time-based rules stored in NVS; priority resolution between schedule, idle-fallback, and manual switching |
| Mode Configuration | FR18-FR20, FR40 | **Medium:** Per-mode settings with NVS key prefix convention; mode-scoped settings schema in static metadata; dynamic Mode Picker settings panels |
| OTA Firmware Updates | FR21-FR31, FR37-FR38 | **Major:** New `OTAUpdater` component — GitHub Releases API client, version comparison, binary download + SHA-256 verification; dashboard OTA Pull UI; shares pre-check sequence with Foundation's OTA Push |
| System Resilience | FR32-FR35 | **Medium:** Watchdog recovery to default mode, config key migration with safe defaults, heap pre-check for OTA + transitions, graceful API degradation |
| Contributor Extensibility | FR41-FR42 | **Minor:** Formalizes existing DisplayMode plugin contract from Display System; compile-time registry already established |
| Documentation & Onboarding | FR43 | **None:** Documentation validation, no architectural impact |

**Non-Functional Requirements (architectural drivers):**

| NFR | Target | Architecture Decision Driver |
|-----|--------|------------------------------|
| Transition framerate | ≥15fps on 160x32 | Dual RGB565 frame buffers with blend-per-pixel; buffers transient during switch only |
| Transition duration | <1 second | Fixed frame count (e.g., 15 frames at 15fps = 1s max) |
| Idle fallback latency | ~30s (one poll interval) | Mode orchestrator monitors flight count after each fetch cycle |
| Departures Board row update | <50ms | In-place row mutation during render(), no full-screen redraw per row change |
| OTA download + reboot | <60s on 10Mbps | Stream-to-partition (same as Foundation push), add GitHub API + SHA-256 overhead |
| Schedule trigger accuracy | ±5 seconds | Non-blocking time check in main loop, same pattern as night mode brightness scheduler |
| Heap floor | ≥80KB free during normal operation | Transition buffers allocated only during switch, freed immediately after |
| Firmware binary size | ≤2,097,152 bytes (2 MiB OTA slot) | Monitor binary size; OTAUpdater adds ~10-15KB |
| No heap degradation | <5% loss over 24 hours | Transition buffers fully freed after each switch; mode init/teardown cycle tested |
| 30-day uptime | Continuous | Schedule timer, OTA check, and transition buffers must not leak memory |

### Scale & Complexity

- **Primary domain:** IoT/Embedded with companion web UI (unchanged)
- **Complexity level:** Medium — two new modes are straightforward plugin implementations (rule 22); mode orchestration state machine and OTA Pull add meaningful coordination complexity
- **New architectural component:** 1 (`core/OTAUpdater` — GitHub API client with version check, download, SHA-256 verification)
- **New lightweight constructs:** Mode scheduling logic (static class or main-loop function, same pattern as Foundation brightness scheduler); fade transition as private method in ModeRegistry
- **New mode implementations (rule 22):** ClockMode, DeparturesBoardMode (two touch points each — `modes/*.h/.cpp` + `MODE_TABLE[]` entry)
- **Updated components:** ModeRegistry (fade transition phase, idle-fallback hook), WebPortal (OTA Pull endpoints, schedule API, mode-settings API), ConfigManager (schedule rules, per-mode settings, OTA Pull keys), main.cpp (mode scheduler in loop, OTA pre-check integration), dashboard HTML/JS (Mode Picker settings panels, schedule UI, OTA Pull UI)
- **Existing components unchanged:** FlightDataFetcher, OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, WiFiManager, LayoutEngine, LogoManager, SystemStatus (add OTA_PULL subsystem entry), ClassicCardMode, LiveFlightCardMode, DisplayUtils

### Technical Constraints (Delight-Specific)

| Constraint | Impact |
|-----------|--------|
| Dual RGB565 frame buffers for fade = ~20KB heap | 160 x 32 x 2 bytes/pixel = 10,240 bytes per buffer, ~20KB total. Buffers are transient — allocate on switch, free after transition completes. Never coexist with OTA download. Well within 80KB heap floor budget. |
| NVS 15-char key limit for schedule rules | Multiple schedule rules need indexed keys (e.g., `sched_r0_start`, `sched_r0_end`, `sched_r0_mode`) — fixed max rule count with 3 keys per rule |
| NVS 15-char key limit for per-mode settings | Prefix convention: `m_` (2) + mode ID abbrev (≤5) + `_` (1) + setting name (≤7) = max 15 chars. Mode ID abbreviations enforced by documentation and code review. |
| GitHub Releases API rate limit (60 req/hr unauthenticated) | OTA check is manual and non-blocking; HTTP 429 → "try again later", no auto-retry |
| GitHub binary download over HTTPS (~1.5MB) | Uses same `Update.h` stream-to-partition path as Foundation push; WiFi must remain stable during download |
| NTP required for scheduling + Clock Mode | Schedule inactive until first NTP sync; Clock Mode falls back to uptime-based display if NTP unavailable |
| Per-mode settings expand NVS usage | Each mode adds 2-5 NVS keys; total NVS key count grows from ~28 to ~40+; well within NVS partition capacity (20KB) |
| OTA pre-check must tear down active mode + free transition buffers | Shared pre-check sequence for both Pull and Push OTA paths — extracted as common utility |

### Cross-Cutting Concerns (Delight Additions)

1. **Transition frame buffer lifecycle** — Dual RGB565 buffers (~20KB total) allocated only during mode switch, freed immediately after fade animation completes. Never coexist with OTA download. ModeRegistry's switch flow gains a fade phase between teardown and init: `teardown old → alloc buffers → capture outgoing frame → init new mode → render N blend frames → free buffers → IDLE`. Implemented as `ModeRegistry::_executeFadeTransition()` private method — not a separate component. If Growth phase adds wipe/scroll, extract at that point.

2. **Mode orchestration state machine** — Three mode-switch sources now exist. Modeled as explicit states with defined transitions:

    **States:**
    - `MANUAL` — user's last explicit selection (NVS persisted via `display_mode` key)
    - `SCHEDULED` — time-based rule is currently active
    - `IDLE_FALLBACK` — zero flights triggered Clock Mode

    **Priority:** `SCHEDULED > IDLE_FALLBACK > MANUAL`

    **Transitions:**
    - Schedule rule fires → enter `SCHEDULED`, switch to scheduled mode
    - Schedule rule expires → exit `SCHEDULED`, restore `MANUAL` selection
    - Zero flights detected (while not `SCHEDULED`) → enter `IDLE_FALLBACK`, switch to Clock Mode
    - Flights return (while `IDLE_FALLBACK`) → exit `IDLE_FALLBACK`, restore `MANUAL` selection
    - Zero flights detected (while `SCHEDULED`) → stay `SCHEDULED` (schedule wins per FR17)
    - User manual switch → update `MANUAL` selection in NVS; if in `IDLE_FALLBACK`, exit fallback; if in `SCHEDULED`, manual switch is temporary override until next schedule tick re-evaluates

3. **OTA Pull + OTA Push coexistence** — Both write to the inactive OTA partition. Both need the same pre-check sequence: verify ≥80KB free heap, tear down active mode (free mode heap + any transition buffers), suspend rendering, show progress on LEDs. Extract shared pre-OTA logic into a common function. Push (Foundation) receives binary from browser multipart upload via WebPortal. Pull (Delight) fetches binary from GitHub CDN via OTAUpdater. Post-write SHA-256 verification (Pull only — Push uses `Update.end()` built-in CRC), boot swap, and reboot are shared.

4. **OTA Pull failure modes** — The PRD specifies SHA-256 verification via companion `.sha256` release asset. Explicit failure handling:
    - `.sha256` asset missing from GitHub Release → reject update, show "Release missing integrity file — contact maintainer"
    - Download succeeds but SHA-256 mismatch → reject, do not activate partition, show "Firmware integrity check failed"
    - Download interrupted (WiFi drop, timeout) → firmware unchanged (inactive partition has partial write, never activated), show "Download failed — tap to retry"
    - GitHub API unreachable → show "Unable to check for updates — try again later", no auto-retry
    - HTTP 429 rate limited → same as unreachable, with explicit "rate limited" message per NFR20

5. **Per-mode NVS key namespacing** — Mode-specific settings use a strict prefix convention:
    - Format: `m_{abbrev}_{setting}` where mode ID abbreviation is ≤5 chars, setting name is ≤7 chars
    - Examples: `m_clock_format` (12/24h), `m_depbd_rows` (max rows), `m_depbd_fields` (field selection), `m_lfcrd_fields` (Live Flight Card telemetry fields)
    - Mode abbreviations: `clock` (5), `depbd` (5), `lfcrd` (5) — documented in ConfigManager alongside existing NVS key abbreviation table
    - Modes declare their settings schema in static metadata (same pattern as `ModeZoneDescriptor`) — name, type, default, min/max for the Mode Picker UI to render settings panels dynamically
    - **Rule:** New mode abbreviations must be ≤5 chars. Enforced by documentation and PR review.

6. **Schedule rules NVS storage** — Fixed maximum of 8 schedule rules with indexed NVS keys:
    - `sched_r{N}_start` (uint16, minutes since midnight)
    - `sched_r{N}_end` (uint16, minutes since midnight)
    - `sched_r{N}_mode` (string, mode ID)
    - `sched_r_count` (uint8, active rule count)
    - Total: up to 25 additional NVS keys at max capacity (8 rules x 3 keys + count)
    - Same minutes-since-midnight convention as Foundation's night mode (rule 15)

7. **NTP dependency expansion** — Foundation introduced NTP for brightness scheduling. Delight adds two more consumers: Clock Mode display and mode scheduling. All three share the same `configTzTime()` sync — no additional NTP configuration. Consequence of NTP failure expands: scheduling degrades gracefully (rules don't fire until NTP syncs), Clock Mode shows uptime-based time or a static indicator. Flight rendering unaffected.

### Existing Architecture Unchanged

The following MVP + Foundation + Display System decisions remain in full effect with no modifications:

- Hexagonal (Ports & Adapters) pattern
- ConfigManager singleton with category struct getters
- Inter-task communication (atomic flags for config, FreeRTOS queue for flights)
- WiFi state machine (WiFiManager)
- SystemStatus error registry
- FreeRTOS dual-core task pinning (display Core 0, everything else Core 1)
- All naming conventions, NVS key patterns, logging macros, and enforcement rules (1-23)
- Web asset patterns (fetch + json.ok + showToast)
- DisplayMode interface, RenderContext isolation, ModeRegistry static table
- OTA self-check, rollback detection, night mode brightness scheduler
- Flash partition layout (unchanged from Foundation — dual OTA 1.5MB + 960KB LittleFS)

## Delight Release — Technology Stack

**Starter Template Evaluation: N/A (Brownfield Extension)**

No new external dependencies required for the core Delight capabilities (Clock Mode, Departures Board, transitions, scheduling, mode-specific settings). All render through the existing DisplayMode interface using Adafruit GFX via FastLED NeoMatrix.

**One new external integration — GitHub Releases API:**

| Component | Implementation | Source |
|-----------|---------------|--------|
| ClockMode | C++ class implementing DisplayMode | New firmware code |
| DeparturesBoardMode | C++ class implementing DisplayMode | New firmware code |
| Fade transition | Private method in ModeRegistry | New firmware code |
| Mode scheduling | Static class or main-loop function | New firmware code |
| Mode-specific settings | ConfigManager expansion + Mode Picker UI | Existing pattern extension |
| OTAUpdater | GitHub Releases API client | New firmware code |
| GitHub Releases API | `api.github.com` REST endpoint | Unauthenticated HTTPS GET |
| SHA-256 verification | `mbedtls_sha256` | ESP32 core (built-in mbedTLS) |

**New library dependency: None.** SHA-256 is available via ESP32's built-in mbedTLS stack (`mbedtls/sha256.h`), already linked for HTTPS TLS. GitHub API responses parsed with existing ArduinoJson. Binary download uses existing `HTTPClient` + `Update.h`.

The MVP + Foundation + Display System dependency stack (ESPAsyncWebServer Carbou fork, AsyncTCP, FastLED, Adafruit GFX, FastLED NeoMatrix, ArduinoJson, LittleFS, ESPmDNS, Update.h, esp_ota_ops.h, time.h/configTzTime, mbedTLS) remains unchanged.

## Delight Release — Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. DL1 — Fade transition in ModeRegistry (RGB888 dual buffer lifecycle, blend algorithm)
2. DL2 — Mode orchestration state machine (MANUAL/SCHEDULED/IDLE_FALLBACK)
3. DL3 — OTAUpdater component (GitHub API client, incremental SHA-256, FreeRTOS download task)
4. DL4 — Schedule rules NVS storage (indexed keys, max 8 rules)

**Important Decisions (Shape Architecture):**
5. DL5 — Per-mode settings schema (NVS prefix convention, static metadata, nullable in ModeEntry)
6. DL6 — API endpoint additions (OTA Pull with status polling, schedule CRUD, mode settings)

**Deferred Decisions (Post-Delight):**
- Multiple transition types (wipe, scroll) — fade only in MVP; extract TransitionEngine if Growth adds more
- Departures Board layout variants — single layout in MVP
- Schedule templates ("Home office," "Weekend") — manual entry only
- OTA firmware signing — trust based on GitHub repo URL for now

### Decision DL1: Fade Transition — ModeRegistry Private Method with Transient RGB888 Buffers

**Approach:** Fade is a phase within the existing `ModeRegistry::tick()` switch lifecycle, implemented as `_executeFadeTransition()` private method. No separate component.

**Buffer strategy:**
- Two RGB888 buffers matching FastLED's native `CRGB` format: `160 * 32 * 3 = 15,360 bytes per buffer`, ~30KB total
- `malloc()` at transition start, `free()` immediately after last blend frame
- If `malloc()` fails (heap too low): skip fade, do instant cut (current behavior) — graceful degradation, not an error
- **Why RGB888, not RGB565:** FastLED stores pixels as `CRGB` (3 bytes RGB888). The `leds` array is directly accessible. Using RGB888 avoids color space conversion in both capture and blend — `memcpy(outgoing, leds, PIXEL_COUNT * 3)` captures the frame. Blend operates natively. No precision loss.

**Blend algorithm:**
```cpp
static constexpr uint8_t FADE_STEPS = 15;  // ~1 second at 15fps

void ModeRegistry::_executeFadeTransition(const RenderContext& ctx,
                                           const std::vector<FlightInfo>& flights) {
    const uint16_t pixelCount = ctx.layout.matrix_width * ctx.layout.matrix_height;
    const size_t bufSize = pixelCount * 3;

    uint8_t* outgoing = (uint8_t*)malloc(bufSize);
    uint8_t* incoming = (uint8_t*)malloc(bufSize);
    if (!outgoing || !incoming) {
        free(outgoing); free(incoming);
        LOG_I("ModeReg", "Fade buffer alloc failed, instant cut");
        return;  // caller proceeds with instant cut
    }

    // Capture outgoing frame from FastLED's CRGB array
    memcpy(outgoing, leds, bufSize);

    // Render one frame of new mode to capture incoming
    _activeMode->render(ctx, flights);
    memcpy(incoming, leds, bufSize);

    // Blend N frames
    for (uint8_t step = 1; step <= FADE_STEPS; step++) {
        for (uint16_t i = 0; i < pixelCount; i++) {
            uint16_t idx = i * 3;
            leds[i].r = (outgoing[idx]     * (FADE_STEPS - step) + incoming[idx]     * step) / FADE_STEPS;
            leds[i].g = (outgoing[idx + 1] * (FADE_STEPS - step) + incoming[idx + 1] * step) / FADE_STEPS;
            leds[i].b = (outgoing[idx + 2] * (FADE_STEPS - step) + incoming[idx + 2] * step) / FADE_STEPS;
        }
        FastLED.show();
        delay(1000 / FADE_STEPS);  // ~66ms per frame at 15fps
    }

    free(outgoing);
    free(incoming);
}
```

**Note:** `delay()` is acceptable inside the fade loop because this runs on Core 0 (display task) during `SWITCHING` state — no other rendering occurs. The main loop on Core 1 is unaffected.

**Updated switch flow in `tick()`:**
```
1. _switchState = SWITCHING
2. _activeMode->teardown()
3. delete _activeMode
4. Heap check against memoryRequirement()
5. If sufficient: factory() → new mode → init(ctx)
6. If init() succeeds: _executeFadeTransition(ctx, flights)
7. _switchState = IDLE, set NVS pending
---
If heap insufficient or init() fails: restore previous mode, set error
If fade buffer alloc fails: instant cut (no regression from current behavior)
```

### Decision DL2: Mode Orchestration — ModeOrchestrator Static Class

**Approach:** Lightweight `core/ModeOrchestrator.h/.cpp` static class. Runs in the Core 1 main loop alongside the existing brightness scheduler. Calls `ModeRegistry::requestSwitch()` when state transitions require a mode change.

```cpp
enum class OrchestratorState : uint8_t {
    MANUAL,          // User's explicit selection (default)
    SCHEDULED,       // Time-based rule is active
    IDLE_FALLBACK    // Zero flights → Clock Mode
};

class ModeOrchestrator {
public:
    static void init();
    static void tick();                              // called from Core 1 main loop, non-blocking
    static void onManualSwitch(const char* modeId);  // called from WebPortal on user switch
    static OrchestratorState getState();
    static const char* getActiveScheduleRuleName();  // for dashboard status
    static const char* getManualModeId();

private:
    static OrchestratorState _state;
    static char _manualModeId[16];                   // NVS-persisted user selection
    static int8_t _activeRuleIndex;                  // -1 if no rule active
    static unsigned long _lastTickMs;
};
```

**Flight count signaling:**
```cpp
// In main.cpp — global atomic, updated by FlightDataFetcher after queue write
std::atomic<uint8_t> g_flightCount{0};

// In FlightDataFetcher, after xQueueOverwrite:
g_flightCount.store(enrichedFlights.size());

// ModeOrchestrator reads this — no queue peek, no cross-core conflict
uint8_t flights = g_flightCount.load();
```

**State machine (inside `tick()`, runs ~1/sec on Core 1):**

```cpp
void ModeOrchestrator::tick() {
    if (millis() - _lastTickMs < 1000) return;  // 1-second interval
    _lastTickMs = millis();

    struct tm now;
    if (!getLocalTime(&now, 0)) return;  // non-blocking; skip if NTP not synced
    uint16_t currentMin = now.tm_hour * 60 + now.tm_min;

    // 1. Evaluate schedule rules (highest priority)
    ModeScheduleConfig sched = ConfigManager::getModeSchedule();
    int8_t matchingRule = -1;
    for (uint8_t i = 0; i < sched.rule_count; i++) {
        if (sched.rules[i].enabled && timeInWindow(currentMin,
                sched.rules[i].start_min, sched.rules[i].end_min)) {
            matchingRule = i;
            break;  // first match wins
        }
    }

    if (matchingRule >= 0) {
        if (_state != SCHEDULED || _activeRuleIndex != matchingRule) {
            _state = SCHEDULED;
            _activeRuleIndex = matchingRule;
            ModeRegistry::requestSwitch(sched.rules[matchingRule].mode_id);
        }
        return;  // schedule active — skip idle fallback check
    }

    // No schedule rule active — exit SCHEDULED if we were in it
    if (_state == SCHEDULED) {
        _state = MANUAL;
        _activeRuleIndex = -1;
        ModeRegistry::requestSwitch(_manualModeId);
        return;
    }

    // 2. Check idle fallback (only when not scheduled)
    uint8_t flights = g_flightCount.load();
    if (flights == 0 && _state == MANUAL) {
        _state = IDLE_FALLBACK;
        ModeRegistry::requestSwitch("clock");
    } else if (flights > 0 && _state == IDLE_FALLBACK) {
        _state = MANUAL;
        ModeRegistry::requestSwitch(_manualModeId);
    }
}
```

**Boot state:** Always starts as `MANUAL`. First `tick()` call (~1 second after boot) evaluates schedule rules and flight count. If a schedule matches, transitions to `SCHEDULED` within that first tick. If zero flights, transitions to `IDLE_FALLBACK`.

**Manual switch handling:**
```cpp
void ModeOrchestrator::onManualSwitch(const char* modeId) {
    strncpy(_manualModeId, modeId, 15);
    _manualModeId[15] = '\0';
    if (_state == IDLE_FALLBACK) {
        _state = MANUAL;
    }
    // If SCHEDULED: manual switch takes effect, but next tick may re-apply schedule
    // This is documented behavior per PRD Journey 1 climax: "schedule takes priority when explicitly configured"
}
```

**Midnight-crossing time window check (same convention as Foundation brightness scheduler):**
```cpp
static bool timeInWindow(uint16_t current, uint16_t start, uint16_t end) {
    if (start <= end) {
        return (current >= start && current < end);
    } else {
        return (current >= start || current < end);
    }
}
```

### Decision DL3: OTAUpdater — GitHub Releases API Client with Incremental SHA-256

**Approach:** New `core/OTAUpdater.h/.cpp` static class. HTTP client that checks GitHub Releases, downloads binary with incremental SHA-256 verification, streams to partition. Download runs in a one-shot FreeRTOS task to avoid blocking the async web server.

```cpp
enum class OTAState : uint8_t {
    IDLE,
    CHECKING,
    AVAILABLE,
    DOWNLOADING,
    VERIFYING,
    REBOOTING,
    ERROR
};

class OTAUpdater {
public:
    static void init(const char* repoOwner, const char* repoName);

    // Version check — called from API handler, runs synchronously (~1-2s)
    static bool checkForUpdate();

    // Download — spawns FreeRTOS task, returns immediately
    static bool startDownload();

    // State accessors for API polling
    static OTAState getState();
    static uint8_t getProgress();        // 0-100%
    static const char* getNewVersion();
    static const char* getReleaseNotes();
    static const char* getLastError();
    static const char* getInstalledVersion();  // returns FW_VERSION

private:
    static char _repoOwner[32];
    static char _repoName[32];
    static char _binUrl[256];
    static char _shaUrl[256];
    static char _newVersion[16];
    static char _releaseNotes[512];      // truncated if longer
    static char _lastError[128];
    static OTAState _state;
    static uint8_t _progress;
    static TaskHandle_t _downloadTask;

    static void _downloadTaskFunc(void* param);
};
```

**Check flow (`checkForUpdate`):**
1. `_state = CHECKING`
2. `GET https://api.github.com/repos/{owner}/{repo}/releases/latest`
3. Parse JSON via ArduinoJson: extract `tag_name`, `body` (release notes), iterate `assets[]` to find `.bin` and `.sha256` URLs
4. Compare `tag_name` with `FW_VERSION` — simple string inequality (not semver)
5. If different: store URLs, version, notes; `_state = AVAILABLE`; return true
6. If same: `_state = IDLE`; return false
7. If HTTP error or 429: `_state = ERROR`; set error message; return false

**Download flow (`_downloadTaskFunc`, runs in FreeRTOS task):**
1. `_state = DOWNLOADING`
2. Shared pre-OTA sequence: `ModeRegistry::prepareForOTA()` — teardown active mode, free buffers, show "Updating..." on LEDs
3. Verify ≥80KB free heap; if not, set error, `_state = ERROR`, return
4. Download `.sha256` file first (~64 bytes); parse as **plain 64-character hex string** (no filename, no prefix). If missing or unparseable: set error "Release missing integrity file", `_state = ERROR`, return
5. Get next OTA partition: `esp_ota_get_next_update_partition(NULL)`
6. `Update.begin(partition->size)`
7. Initialize `mbedtls_sha256_context`
8. Stream `.bin` via `HTTPClient` in chunks:
   - `Update.write(chunk, len)` — write to flash
   - `mbedtls_sha256_update(&ctx, chunk, len)` — feed to hasher
   - Update `_progress = (bytesWritten * 100) / totalSize`
9. After final chunk: `mbedtls_sha256_finish(&ctx, computedHash)`
10. `_state = VERIFYING`
11. Compare `computedHash` against downloaded expected hash
    - If mismatch: `Update.abort()`, set error "Firmware integrity check failed", `_state = ERROR`, return
12. `Update.end(true)` — finalize partition
13. `_state = REBOOTING`
14. `delay(500); ESP.restart()`

**Error recovery:**
- Download interrupted (WiFi drop): `Update.abort()`, `_state = ERROR`, "Download failed — tap to retry"
- Any HTTP error during download: `Update.abort()`, `_state = ERROR`, specific message
- The inactive partition is never activated on failure — firmware unchanged
- After error, user can call `startDownload()` again (retry)

**Shared pre-OTA extraction:**
```cpp
// New public method in ModeRegistry
static void prepareForOTA() {
    // Teardown active mode — frees mode heap + any transition buffers
    if (_activeMode) {
        _activeMode->teardown();
        delete _activeMode;
        _activeMode = nullptr;
    }
    _switchState = SWITCHING;  // prevents new switch requests during OTA
}

// Used by both:
// - WebPortal::_handleOTAUpload() (Foundation push) — call before Update.begin()
// - OTAUpdater::_downloadTaskFunc() (Delight pull) — call before download
```

### Decision DL4: Schedule Rules NVS Storage — Fixed Max with Indexed Keys

**Approach:** Fixed maximum of 8 rules with indexed NVS keys. New `ModeScheduleConfig` struct in ConfigManager.

```cpp
struct ScheduleRule {
    uint16_t start_min;     // minutes since midnight (0-1439)
    uint16_t end_min;       // minutes since midnight (0-1439)
    char mode_id[16];       // full mode ID string (e.g., "clock", "departures")
    bool enabled;
};

struct ModeScheduleConfig {
    ScheduleRule rules[8];
    uint8_t rule_count;     // 0-8, number of defined rules
};
```

**NVS keys (per rule):**

| Key Pattern | Type | Example | Chars |
|------------|------|---------|-------|
| `sched_r{N}_start` | uint16 | `sched_r0_start` = 360 | 15 |
| `sched_r{N}_end` | uint16 | `sched_r0_end` = 1320 | 13 |
| `sched_r{N}_mode` | string | `sched_r0_mode` = `"clock"` | 14 |
| `sched_r_count` | uint8 | 2 | 13 |

All keys within 15-char NVS limit. Mode ID stored as full string value (values have no length limit in NVS). N ranges 0-7.

**ConfigManager additions:**
```cpp
static ModeScheduleConfig getModeSchedule();
static void setModeSchedule(const ModeScheduleConfig& config);
```

All schedule keys are **hot-reload** — no reboot required. `setModeSchedule()` writes all rule keys in a batch, then `ModeOrchestrator::tick()` picks up changes on its next iteration (~1 second).

**Rule conflict resolution:** Rules evaluated in index order (0 first). First matching rule wins. Overlapping time windows resolved by index priority — user arranges rules via the dashboard, display order is priority order.

### Decision DL5: Per-Mode Settings Schema — Static Metadata with NVS Prefix Convention

**Approach:** Modes declare settings via static metadata struct (same pattern as `ModeZoneDescriptor`). ConfigManager reads/writes using the `m_{abbrev}_{setting}` NVS key prefix. Mode Picker UI renders settings panels dynamically from the schema returned by the API.

```cpp
struct ModeSettingDef {
    const char* key;          // NVS key suffix (≤7 chars)
    const char* label;        // UI display name
    const char* type;         // "uint8", "uint16", "bool", "enum"
    int32_t defaultValue;
    int32_t minValue;
    int32_t maxValue;
    const char* enumOptions;  // comma-separated for enum type, NULL otherwise
};

struct ModeSettingsSchema {
    const char* modeAbbrev;           // ≤5 chars, used for NVS key prefix
    const ModeSettingDef* settings;
    uint8_t settingCount;
};
```

**Clock Mode settings:**
```cpp
// NVS key: m_clock_format (14 chars)
static const ModeSettingDef CLOCK_SETTINGS[] = {
    { "format", "Time Format", "enum", 0, 0, 1, "24h,12h" }
};
static const ModeSettingsSchema CLOCK_SCHEMA = { "clock", CLOCK_SETTINGS, 1 };
```

**Departures Board settings:**
```cpp
// NVS keys: m_depbd_rows (12 chars), m_depbd_fields (14 chars)
static const ModeSettingDef DEPBD_SETTINGS[] = {
    { "rows", "Max Rows", "uint8", 4, 1, 4, NULL },
    { "fields", "Telemetry Fields", "enum", 0, 0, 2, "alt+spd,alt+hdg,spd+hdg" }
};
static const ModeSettingsSchema DEPBD_SCHEMA = { "depbd", DEPBD_SETTINGS, 2 };
```

**ModeEntry expansion (backward-compatible):**
```cpp
struct ModeEntry {
    const char* id;
    const char* displayName;
    DisplayMode* (*factory)();
    uint32_t (*memoryRequirement)();
    uint8_t priority;
    const ModeSettingsSchema* settingsSchema;  // NEW — nullable for modes with no settings
};
```

Existing modes (ClassicCardMode, LiveFlightCardMode) set `settingsSchema` to `nullptr`. API returns empty `settings: []` for nullable schemas. No breaking change to existing mode table entries.

**ConfigManager per-mode helpers:**
```cpp
static int32_t getModeSetting(const char* modeAbbrev, const char* settingKey, int32_t defaultValue);
static void setModeSetting(const char* modeAbbrev, const char* settingKey, int32_t value);
// Constructs NVS key as: "m_" + modeAbbrev + "_" + settingKey
```

**Key convention enforcement:**
- Mode abbreviation: ≤5 chars (documented alongside existing NVS abbreviation table in ConfigManager.h)
- Setting name: ≤7 chars
- Total: `m_` (2) + abbrev (≤5) + `_` (1) + setting (≤7) = ≤15 chars
- Abbreviations: `clock` (5), `depbd` (5), `lfcrd` (5), `clsic` (5)

### Decision DL6: API Endpoint Additions — OTA Pull with Status Polling + Schedule CRUD

**5 new endpoints:**

| Method | Endpoint | Purpose | Response |
|--------|----------|---------|----------|
| `GET` | `/api/ota/check` | Check GitHub for new firmware version | `{ ok, data: { available, version, release_notes, current_version } }` |
| `POST` | `/api/ota/pull` | Start firmware download (spawns FreeRTOS task) | `{ ok, data: { started: true } }` |
| `GET` | `/api/ota/status` | Poll download progress and state | `{ ok, data: { state, progress, error } }` |
| `GET` | `/api/schedule` | Get mode schedule rules | `{ ok, data: { rules: [...], orchestrator_state } }` |
| `POST` | `/api/schedule` | Save mode schedule rules | `{ ok, data: { applied: true } }` |

**Updated existing endpoints:**

| Endpoint | Changes |
|----------|---------|
| `GET /api/display/modes` | Add `settings` array per mode with schema + current values; `null` for modes without settings |
| `POST /api/display/mode` | Accept optional `settings` object for per-mode config changes |
| `GET /api/status` | Add `ota_available` (bool), `ota_version` (string or null), `orchestrator_state` ("manual"/"scheduled"/"idle_fallback"), `active_schedule_rule` (index or -1) |

**`GET /api/ota/check` response:**
```json
{
    "ok": true,
    "data": {
        "available": true,
        "version": "2.4.1",
        "current_version": "2.4.0",
        "release_notes": "Bug fix for Departures Board row alignment..."
    }
}
```

**`GET /api/ota/status` response (during download):**
```json
{
    "ok": true,
    "data": {
        "state": "downloading",
        "progress": 42
    }
}
```

**`GET /api/schedule` response:**
```json
{
    "ok": true,
    "data": {
        "orchestrator_state": "scheduled",
        "active_rule_index": 0,
        "rules": [
            { "start": 360, "end": 1320, "mode": "departures", "enabled": true },
            { "start": 1320, "end": 360, "mode": "clock", "enabled": true }
        ]
    }
}
```

**`POST /api/ota/pull` behavior:**
- Validates that `OTAUpdater::getState()` is `AVAILABLE` (must check first)
- Calls `OTAUpdater::startDownload()` which spawns a FreeRTOS task
- Returns immediately with `{ ok: true }`
- Dashboard polls `GET /api/ota/status` at 500ms interval for progress
- Matches the existing mode-switch polling pattern from Display System

### Delight Decision Impact — Implementation Sequence

**Sequential foundation:**
1. **ConfigManager expansion** — ModeScheduleConfig struct, per-mode setting helpers, `getModeSchedule()` / `setModeSchedule()`, `getModeSetting()` / `setModeSetting()`
2. **ModeEntry expansion** — add `settingsSchema` field, update MODE_TABLE for existing modes with `nullptr`

**Mode implementations (sequential, after step 2):**
3. **ClockMode** (`modes/ClockMode.h/.cpp`) — simplest mode, proves NTP time display works
4. **DeparturesBoardMode** (`modes/DeparturesBoardMode.h/.cpp`) — multi-row rendering, configurable rows

**Orchestration track (after step 3, needs Clock Mode for idle fallback):**
5. **ModeOrchestrator** (`core/ModeOrchestrator.h/.cpp`) — state machine, schedule evaluation, idle fallback
6. **Fade transition** — `ModeRegistry::_executeFadeTransition()` private method + buffer management

**OTA track (independent of steps 3-6, after step 1):**
7. **OTAUpdater** (`core/OTAUpdater.h/.cpp`) — GitHub API client, incremental SHA-256, FreeRTOS download task
8. **ModeRegistry::prepareForOTA()** — shared pre-OTA sequence for both Push and Pull

**API + UI (after all firmware tracks complete):**
9. **WebPortal endpoints** — OTA Pull (check/pull/status), schedule (GET/POST), mode settings expansion
10. **Dashboard UI** — OTA Pull UI (banner, release notes, progress), schedule timeline editor, per-mode settings panels in Mode Picker
11. **Gzip rebuild** — all updated web assets

### Cross-Decision Dependencies

```
ConfigManager (DL4 schedule keys + DL5 per-mode settings)
    ├── ModeOrchestrator (DL2) — reads schedule rules
    │   └── ModeRegistry::requestSwitch() — triggers mode changes
    │       └── _executeFadeTransition (DL1) — runs during switch
    └── OTAUpdater (DL3) — reads repo config
        └── ModeRegistry::prepareForOTA() — shared pre-OTA

ModeEntry.settingsSchema (DL5)
    └── GET /api/display/modes (DL6) — includes settings in response
        └── Mode Picker UI — renders per-mode settings panels

g_flightCount atomic (DL2)
    ← FlightDataFetcher (writes after queue update)
    → ModeOrchestrator::tick() (reads for idle fallback)
```

## Delight Release — Implementation Patterns & Consistency Rules

### Delight-Specific Conflict Points

The MVP architecture established 11 enforcement rules. The Foundation Release added 5 (rules 12-16). The Display System Release added 7 (rules 17-23). The Delight Release introduces 7 new pattern areas where AI agents could make inconsistent choices.

### Orchestrator Interaction Pattern

**Who calls `ModeRegistry::requestSwitch()` — and when:**

ModeOrchestrator is the *only* component that evaluates schedule rules and idle fallback. WebPortal routes manual switches through `ModeOrchestrator::onManualSwitch()`, which records the user's preference and then delegates to `requestSwitch()` if appropriate.

**`requestSwitch()` is called from exactly two methods:**
1. `ModeOrchestrator::tick()` — when state machine transitions require a mode change
2. `ModeOrchestrator::onManualSwitch()` — when user selects a mode via dashboard

**Allowed:**
```cpp
// WebPortal handler for POST /api/display/mode:
ModeOrchestrator::onManualSwitch(modeId);  // ✅ always go through orchestrator

// ModeOrchestrator::tick() — evaluates state machine:
ModeRegistry::requestSwitch(targetModeId);  // ✅ orchestrator owns the decision
```

**Forbidden:**
```cpp
// WebPortal directly calling ModeRegistry:
ModeRegistry::requestSwitch(modeId);  // ❌ bypasses orchestrator state tracking

// Mode class requesting its own switch:
ModeRegistry::requestSwitch("clock");  // ❌ modes never initiate switches

// Timer callback or config-change handler calling requestSwitch:
void onConfigChange() { ModeRegistry::requestSwitch(...); }  // ❌ only orchestrator calls this
```

**Anti-pattern:** Calling `requestSwitch()` from anywhere except the two orchestrator methods. The orchestrator maintains state (MANUAL/SCHEDULED/IDLE_FALLBACK) that must stay consistent with the actual active mode. A direct `requestSwitch()` from any other call site would desync the state machine.

### Cross-Core Atomic Signaling Pattern

**`std::atomic` globals declared in `main.cpp` only:**

DL2 introduces `std::atomic<uint8_t> g_flightCount` as a cross-core signaling mechanism. This is a lightweight pattern for Core 1 → Core 0 communication without queues.

**Allowed:**
```cpp
// main.cpp — declaration
std::atomic<uint8_t> g_flightCount{0};

// FlightDataFetcher (Core 1) — writer
g_flightCount.store(enrichedFlights.size());

// ModeOrchestrator (Core 1) — reader
uint8_t flights = g_flightCount.load();
```

**Forbidden:**
```cpp
// Mode class reading atomic:
void ClockMode::render(...) {
    uint8_t f = g_flightCount.load();  // ❌ modes use RenderContext, not globals
}

// Adapter reading atomic:
void WebPortal::handleStatus() {
    uint8_t f = g_flightCount.load();  // ❌ adapters use core API, not globals
}
```

**Anti-pattern:** Declaring new `std::atomic` globals outside `main.cpp`, or reading/writing them from modes or adapters. Cross-core atomics are system-level plumbing — only core orchestration code (`main.cpp`, `ModeOrchestrator`, `FlightDataFetcher`) should touch them.

### OTA Pull Streaming Pattern

**Incremental hash — never post-hoc:**

The SHA-256 hash must be computed incrementally during the download stream. Each chunk is fed to both `Update.write()` and `mbedtls_sha256_update()` in the same loop iteration.

```cpp
// Correct pattern:
while (remaining > 0) {
    int len = stream->readBytes(buf, chunkSize);
    Update.write(buf, len);                      // write to flash
    mbedtls_sha256_update(&ctx, buf, len);       // feed to hasher
    _progress = (written * 100) / totalSize;     // update progress
}
mbedtls_sha256_finish(&ctx, computed);
// compare computed vs expected BEFORE Update.end()
```

**Anti-pattern:** Downloading the full binary, calling `Update.end()`, then trying to read back the partition to verify. `Update.h` provides no read-back API. Hash *during* streaming, verify *before* finalize.

**Error path discipline — always `Update.abort()` after `Update.begin()`:**

Once `Update.begin()` has been called, every error path must call `Update.abort()` before returning. This releases the flash write lock and leaves the inactive partition unchanged.

```cpp
// Correct error handling:
if (hashMismatch) {
    Update.abort();                              // ✅ always abort on failure
    _state = OTAState::ERROR;
    strncpy(_lastError, "Integrity check failed", sizeof(_lastError));
    return;
}

// Anti-pattern:
if (downloadFailed) {
    _state = OTAState::ERROR;                    // ❌ forgot Update.abort()
    return;                                       // flash write lock still held
}
```

**FreeRTOS task lifecycle:**
- `startDownload()` spawns a one-shot task on Core 1 (pinned, 8KB stack, priority 1)
- Task sets `_downloadTask = nullptr` *before* calling `vTaskDelete(NULL)` — prevents dangling handle reads from main thread
- Only one download task at a time — `startDownload()` rejects if `_downloadTask != nullptr`

```cpp
// End of _downloadTaskFunc:
_downloadTask = nullptr;   // ✅ clear handle before delete
vTaskDelete(NULL);         // task self-deletes

// startDownload() guard:
if (_downloadTask != nullptr) {
    return false;          // download already in progress
}
```

**Anti-pattern:** Creating a persistent FreeRTOS task that polls for OTA. The download is a one-shot operation triggered by the user.

### Pre-OTA Sequence Pattern

**Shared between Foundation Push and Delight Pull:**

Both OTA paths (browser upload and GitHub download) must call `ModeRegistry::prepareForOTA()` before `Update.begin()`. This frees mode heap and prevents the display task from interfering during the flash write.

**`prepareForOTA()` must set `_switchState = SWITCHING`** to block the display task's `tick()` loop. Without this flag, Core 0 could call `tick()` on a deleted mode pointer during the flash write.

```cpp
// ModeRegistry::prepareForOTA():
static void prepareForOTA() {
    _switchState = SWITCHING;  // ✅ blocks display task tick() loop
    if (_activeMode) {
        _activeMode->teardown();
        delete _activeMode;
        _activeMode = nullptr;
    }
}

// Foundation Push (WebPortal OTA upload handler):
ModeRegistry::prepareForOTA();  // then Update.begin(contentLength)

// Delight Pull (OTAUpdater download task):
ModeRegistry::prepareForOTA();  // then Update.begin(partitionSize)
```

**Anti-pattern:** Calling `Update.begin()` without first tearing down the active mode. The mode may hold 20-30KB of heap (transition buffers, render state) that OTA needs. Also: implementing `prepareForOTA()` without setting `_switchState = SWITCHING`.

### Schedule Rule NVS Pattern

**Indexed key convention — matches Foundation brightness schedule approach:**

Schedule rules use indexed keys where N is 0-7. The count key `sched_r_count` tells how many rules are defined.

**Complete NVS key set per rule:**

| Key Pattern | Type | Example | Chars |
|------------|------|---------|-------|
| `sched_r{N}_start` | uint16 | `sched_r0_start` = 360 | 15 |
| `sched_r{N}_end` | uint16 | `sched_r0_end` = 1320 | 13 |
| `sched_r{N}_mode` | string | `sched_r0_mode` = `"clock"` | 14 |
| `sched_r{N}_ena` | uint8 | `sched_r0_ena` = 1 | 13 |
| `sched_r_count` | uint8 | 2 | 13 |

All keys within 15-char NVS limit. The enabled flag uses `_ena` (not `_enabled` which would be 17 chars).

**Read pattern (getModeSchedule):**
```cpp
ModeScheduleConfig config;
config.rule_count = nvs.getUChar("sched_r_count", 0);
for (uint8_t i = 0; i < config.rule_count; i++) {
    char key[16];
    snprintf(key, sizeof(key), "sched_r%d_start", i);
    config.rules[i].start_min = nvs.getUShort(key, 0);
    snprintf(key, sizeof(key), "sched_r%d_end", i);
    config.rules[i].end_min = nvs.getUShort(key, 0);
    snprintf(key, sizeof(key), "sched_r%d_mode", i);
    // ... string read for mode_id
    snprintf(key, sizeof(key), "sched_r%d_ena", i);
    config.rules[i].enabled = nvs.getUChar(key, 1);  // default enabled
}
```

**Write pattern (setModeSchedule) — rules always compacted:**
```cpp
// Rules are stored compacted — no index gaps.
// When a rule is deleted from the middle, higher-index rules shift down.
nvs.putUChar("sched_r_count", config.rule_count);
for (uint8_t i = 0; i < config.rule_count; i++) {
    char key[16];
    snprintf(key, sizeof(key), "sched_r%d_start", i);
    nvs.putUShort(key, config.rules[i].start_min);
    snprintf(key, sizeof(key), "sched_r%d_end", i);
    nvs.putUShort(key, config.rules[i].end_min);
    snprintf(key, sizeof(key), "sched_r%d_mode", i);
    // ... string write for mode_id
    snprintf(key, sizeof(key), "sched_r%d_ena", i);
    nvs.putUChar(key, config.rules[i].enabled ? 1 : 0);
}
```

**Anti-pattern:** Serializing schedule rules as a single JSON blob in one NVS string key. NVS string values are limited to ~4000 bytes and JSON parsing adds heap pressure. Use indexed keys — same pattern as the Foundation brightness schedule.

**Anti-pattern:** Leaving index gaps when deleting rules. Rules are always compacted — when rule 1 of 3 is deleted, rules 2 and 3 shift down to indices 0 and 1, and `sched_r_count` decrements to 2.

### Per-Mode Settings NVS Pattern

**Key construction — always via ConfigManager helpers:**

Mode settings use the `m_{abbrev}_{setting}` NVS key pattern. Never construct these keys manually in mode code or WebPortal handlers.

```cpp
// Correct — through ConfigManager:
int32_t fmt = ConfigManager::getModeSetting("clock", "format", 0);  // reads "m_clock_format"
ConfigManager::setModeSetting("depbd", "rows", 3);                   // writes "m_depbd_rows"

// Forbidden — manual key construction:
nvs.getUChar("m_clock_format", 0);  // ❌ bypasses ConfigManager, no default handling
```

**Schema declaration — co-located with mode class:**

Each mode's `ModeSettingDef[]` and `ModeSettingsSchema` are declared as `static const` in the mode's `.h` file, not in a centralized registry.

```cpp
// modes/ClockMode.h — settings schema co-located with mode
static const ModeSettingDef CLOCK_SETTINGS[] = {
    { "format", "Time Format", "enum", 0, 0, 1, "24h,12h" }
};
static const ModeSettingsSchema CLOCK_SCHEMA = { "clock", CLOCK_SETTINGS, 1 };
```

**Anti-pattern:** Putting all mode settings definitions in ConfigManager or a single registry file. Settings are owned by the mode — the schema lives next to the code that uses it.

**Anti-pattern:** Hardcoding setting names in API response builders. WebPortal handlers for `GET /api/display/modes` must iterate `ModeEntry.settingsSchema` dynamically to build the settings JSON. If someone adds a setting to the schema but the API handler has hardcoded field names, the new setting becomes invisible to the dashboard.

```cpp
// Correct — dynamic iteration:
if (entry.settingsSchema) {
    for (uint8_t i = 0; i < entry.settingsSchema->settingCount; i++) {
        const ModeSettingDef& def = entry.settingsSchema->settings[i];
        // build JSON from def.key, def.label, def.type, etc.
    }
}

// Forbidden — hardcoded:
json["settings"][0]["key"] = "format";  // ❌ breaks when schema changes
```

### Fade Buffer Lifecycle Pattern

**Allocate late, free early:**

Transition buffers are `malloc()`'d at the start of `_executeFadeTransition()` and `free()`'d immediately after the last blend frame. They never persist beyond the transition call.

**Rule:** If `malloc()` returns `nullptr` for either buffer, free both and return immediately. The caller treats this as "instant cut" — the mode still switches, just without the fade animation. This is graceful degradation, not an error.

```cpp
uint8_t* outgoing = (uint8_t*)malloc(bufSize);
uint8_t* incoming = (uint8_t*)malloc(bufSize);
if (!outgoing || !incoming) {
    free(outgoing); free(incoming);  // safe: free(nullptr) is no-op
    LOG_I("ModeReg", "Fade buffer alloc failed, instant cut");
    return;  // not an error — graceful degradation
}
// ... blend loop ...
free(outgoing);
free(incoming);
```

**Anti-pattern:** Pre-allocating persistent transition buffers at boot. The ~30KB would reduce available heap for the entire lifetime. Allocate only during the ~1-second transition window.

### Updated Enforcement Guidelines

**All AI agents implementing Delight stories MUST also follow (in addition to rules 1-23):**

24. Mode switches from WebPortal must go through `ModeOrchestrator::onManualSwitch()` — `ModeRegistry::requestSwitch()` is called from exactly two methods: `ModeOrchestrator::tick()` and `ModeOrchestrator::onManualSwitch()` — no other call sites
25. OTA SHA-256 verification must be computed incrementally during streaming via `mbedtls_sha256_update()` per chunk — never attempt post-download partition read-back. On any error after `Update.begin()`, always call `Update.abort()` before returning
26. Both OTA paths (Push and Pull) must call `ModeRegistry::prepareForOTA()` before `Update.begin()` — `prepareForOTA()` must set `_switchState = SWITCHING` to block the display task's `tick()` loop
27. Fade transition buffers are `malloc()`'d at transition start and `free()`'d immediately after the last blend frame — never persist transition buffers beyond the fade call, and never treat allocation failure as an error (use instant cut fallback)
28. Per-mode settings NVS keys must be read/written exclusively through `ConfigManager::getModeSetting()` / `setModeSetting()` — never construct `m_{abbrev}_{key}` keys manually. API handlers must iterate `settingsSchema` dynamically, never hardcode setting names
29. Mode settings schemas (`ModeSettingDef[]` + `ModeSettingsSchema`) are declared as `static const` in the mode's own `.h` file — never centralize settings definitions in ConfigManager or a registry
30. Cross-core `std::atomic` globals are declared in `main.cpp` only — modes and adapters must not read or write atomic globals directly; use `RenderContext` fields or core API methods instead

### Delight Release — Party Mode Refinements (Patterns)

**Reviewers:** Winston (Architect), Amelia (Developer), Quinn (QA)

**8 refinements incorporated:**

1. Rule 24 tightened to enumerate exactly two allowed `requestSwitch()` call sites (Winston)
2. New rule 30 added for cross-core atomic signaling discipline (Winston)
3. Schedule NVS pattern: added explicit `sched_r{N}_ena` key with 14-char name (Winston)
4. OTA task pattern: `_downloadTask = nullptr` before `vTaskDelete(NULL)` to prevent dangling handle (Amelia)
5. OTA error pattern: always `Update.abort()` on any error after `Update.begin()` — added to rule 25 (Amelia)
6. Pre-OTA pattern: explicitly state `prepareForOTA()` must set `_switchState = SWITCHING` — added to rule 26 (Amelia)
7. Per-mode settings anti-pattern: API handlers must iterate schema dynamically, never hardcode — added to rule 28 (Quinn)
8. Schedule delete convention: rules always compacted with no index gaps — added to NVS pattern (Quinn)

## Delight Release — Project Structure & File Change Map

### Complete Project Directory Structure (Delight Additions)

```
firmware/
├── platformio.ini                          # UNCHANGED (modes/ and utils/ already in build filter;
│                                           #  mbedTLS bundled in ESP-IDF, no extra flags needed)
├── custom_partitions.csv                   # UNCHANGED (Foundation dual-OTA layout)
│
├── interfaces/
│   ├── BaseDisplay.h                       # UNCHANGED
│   ├── BaseFlightFetcher.h                 # UNCHANGED
│   ├── BaseStateVectorFetcher.h            # UNCHANGED
│   └── DisplayMode.h                       # UPDATED: +ModeSettingDef struct,
│                                           #          +ModeSettingsSchema struct
│                                           #          (mode contract — settings declaration)
│
├── core/
│   ├── ConfigManager.h                     # UPDATED: +ModeScheduleConfig, +ScheduleRule structs,
│   │                                       #          +getModeSchedule(), +setModeSchedule(),
│   │                                       #          +getModeSetting(), +setModeSetting(),
│   │                                       #          +sched_r{N} NVS key abbreviation docs
│   ├── ConfigManager.cpp                   # UPDATED: schedule NVS read/write with compaction,
│   │                                       #          per-mode NVS helpers (m_{abbrev}_{key})
│   ├── ModeRegistry.h                      # UPDATED: +_executeFadeTransition() private,
│   │                                       #          +prepareForOTA() public static,
│   │                                       #          ModeEntry +settingsSchema field
│   ├── ModeRegistry.cpp                    # UPDATED: fade transition blend loop,
│   │                                       #          prepareForOTA() implementation
│   │                                       #          (sets _switchState = SWITCHING)
│   ├── ModeOrchestrator.h                  # ← NEW: OrchestratorState enum, static class decl
│   ├── ModeOrchestrator.cpp                # ← NEW: tick(), onManualSwitch(), state machine,
│   │                                       #         schedule evaluation, idle fallback,
│   │                                       #         timeInWindow() helper,
│   │                                       #         extern g_flightCount declaration
│   ├── OTAUpdater.h                        # ← NEW: OTAState enum, static class decl,
│   │                                       #         version/progress/error accessors
│   ├── OTAUpdater.cpp                      # ← NEW: checkForUpdate() GitHub API,
│   │                                       #         _downloadTaskFunc() FreeRTOS task,
│   │                                       #         incremental SHA-256 via mbedTLS,
│   │                                       #         startDownload() task spawning
│   ├── FlightDataFetcher.h                 # UNCHANGED
│   ├── FlightDataFetcher.cpp               # UPDATED: +g_flightCount.store() after queue write,
│   │                                       #          +extern std::atomic<uint8_t> g_flightCount
│   ├── LayoutEngine.h                      # UNCHANGED
│   ├── LayoutEngine.cpp                    # UNCHANGED
│   ├── LogoManager.h                       # UNCHANGED
│   ├── LogoManager.cpp                     # UNCHANGED
│   ├── SystemStatus.h                      # UPDATED: +OTA_PULL subsystem entry
│   └── SystemStatus.cpp                    # UPDATED: +OTA_PULL subsystem registration
│
├── modes/
│   ├── ClassicCardMode.h                   # UNCHANGED (settingsSchema nullptr is in MODE_TABLE
│   ├── ClassicCardMode.cpp                 #  in main.cpp, not in mode .h files)
│   ├── LiveFlightCardMode.h                # UNCHANGED
│   ├── LiveFlightCardMode.cpp              # UNCHANGED
│   ├── ClockMode.h                         # ← NEW: ClockMode class, CLOCK_SETTINGS[],
│   │                                       #         CLOCK_SCHEMA static const,
│   │                                       #         NTP time display, 12/24h format setting
│   ├── ClockMode.cpp                       # ← NEW: render() with time formatting,
│   │                                       #         getModeSetting() for format preference
│   ├── DeparturesBoardMode.h              # ← NEW: DeparturesBoardMode class, DEPBD_SETTINGS[],
│   │                                       #         DEPBD_SCHEMA static const,
│   │                                       #         configurable rows + telemetry fields
│   └── DeparturesBoardMode.cpp            # ← NEW: multi-row rendering, dynamic row
│                                           #         add/remove, in-place row mutation
│
├── adapters/
│   ├── NeoMatrixDisplay.h                  # UNCHANGED
│   ├── NeoMatrixDisplay.cpp                # UNCHANGED
│   ├── WebPortal.h                         # UPDATED: +_handleOTACheck(), +_handleOTAPull(),
│   │                                       #          +_handleOTAStatus(), +_handleGetSchedule(),
│   │                                       #          +_handlePostSchedule()
│   ├── WebPortal.cpp                       # UPDATED: 5 new route handlers,
│   │                                       #          updated /api/display/modes (settings array,
│   │                                       #          iterates settingsSchema dynamically),
│   │                                       #          updated POST /api/display/mode (settings obj,
│   │                                       #          routes through onManualSwitch()),
│   │                                       #          updated /api/status (ota + orchestrator)
│   ├── WiFiManager.h                       # UNCHANGED
│   ├── WiFiManager.cpp                     # UNCHANGED
│   ├── AeroAPIFetcher.h                    # UNCHANGED
│   ├── AeroAPIFetcher.cpp                  # UNCHANGED
│   ├── FlightWallFetcher.h                 # UNCHANGED
│   ├── FlightWallFetcher.cpp               # UNCHANGED
│   ├── OpenSkyFetcher.h                    # UNCHANGED
│   └── OpenSkyFetcher.cpp                  # UNCHANGED
│
├── utils/
│   ├── GeoUtils.h                          # UNCHANGED
│   ├── Log.h                               # UNCHANGED
│   ├── DisplayUtils.h                      # UNCHANGED
│   └── DisplayUtils.cpp                    # UNCHANGED
│
├── models/
│   ├── FlightInfo.h                        # UNCHANGED
│   ├── StateVector.h                       # UNCHANGED
│   └── AirportInfo.h                       # UNCHANGED
│
├── config/
│   ├── HardwareConfiguration.h             # UNCHANGED
│   ├── TimingConfiguration.h               # UNCHANGED
│   ├── WiFiConfiguration.h                 # UNCHANGED
│   ├── APIConfiguration.h                  # UNCHANGED
│   └── UserConfiguration.h                 # UNCHANGED
│
├── src/
│   └── main.cpp                            # UPDATED: +std::atomic<uint8_t> g_flightCount,
│                                           #          +ModeOrchestrator::init() in setup(),
│                                           #          +OTAUpdater::init() in setup(),
│                                           #          +ModeOrchestrator::tick() in Core 1 loop,
│                                           #          MODE_TABLE entries +settingsSchema field
│                                           #          (nullptr for existing, schema ptr for new)
│
├── data-src/
│   ├── dashboard.html                      # UPDATED: +OTA Pull section (check/download/progress),
│   │                                       #          +Schedule timeline editor section,
│   │                                       #          +per-mode settings panels in Mode Picker
│   ├── dashboard.js                        # UPDATED: +OTA Pull API calls (check/pull/status poll),
│   │                                       #          +schedule CRUD (GET/POST /api/schedule),
│   │                                       #          +per-mode settings rendering from schema,
│   │                                       #          +orchestrator state display
│   ├── style.css                           # UPDATED: +OTA progress bar, +schedule timeline,
│   │                                       #          +mode settings panel styling
│   ├── health.html                         # UNCHANGED
│   ├── health.js                           # UNCHANGED
│   ├── common.js                           # UNCHANGED
│   ├── wizard.html                         # UNCHANGED
│   └── wizard.js                           # UNCHANGED
│
├── data/
│   ├── dashboard.html.gz                   # REBUILD after dashboard.html changes
│   ├── dashboard.js.gz                     # REBUILD after dashboard.js changes
│   ├── style.css.gz                        # REBUILD after style.css changes
│   ├── common.js.gz                        # UNCHANGED
│   ├── health.html.gz                      # UNCHANGED
│   ├── health.js.gz                        # UNCHANGED
│   ├── wizard.html.gz                      # UNCHANGED
│   ├── wizard.js.gz                        # UNCHANGED
│   ├── test.txt                            # UNCHANGED
│   └── logos/                              # UNCHANGED
│
└── test/
    ├── test_config_manager/test_main.cpp   # UNCHANGED
    ├── test_layout_engine/test_main.cpp    # UNCHANGED
    ├── test_logo_manager/test_main.cpp     # UNCHANGED
    ├── test_telemetry_conversion/test_main.cpp  # UNCHANGED
    ├── test_mode_registry/test_main.cpp    # UPDATED: +fade transition lifecycle,
    │                                       #          +prepareForOTA() state tests
    ├── test_mode_orchestrator/test_main.cpp # ← NEW: state machine transitions,
    │                                       #         schedule evaluation, idle fallback,
    │                                       #         manual switch interaction
    ├── test_ota_updater/test_main.cpp      # ← NEW: version comparison, state transitions,
    │                                       #         SHA-256 verification logic (mock HTTP)
    └── test_config_schedule/test_main.cpp  # ← NEW: schedule NVS read/write,
                                            #         rule compaction on delete,
                                            #         index shifting, count tracking
```

### File Change Summary

| Action | Count | Files |
|--------|-------|-------|
| **NEW** | 8 | `core/ModeOrchestrator.h`, `core/ModeOrchestrator.cpp`, `core/OTAUpdater.h`, `core/OTAUpdater.cpp`, `modes/ClockMode.h`, `modes/ClockMode.cpp`, `modes/DeparturesBoardMode.h`, `modes/DeparturesBoardMode.cpp` |
| **NEW (tests)** | 3 | `test/test_mode_orchestrator/test_main.cpp`, `test/test_ota_updater/test_main.cpp`, `test/test_config_schedule/test_main.cpp` |
| **UPDATED** | 14 | `interfaces/DisplayMode.h`, `core/ConfigManager.h`, `core/ConfigManager.cpp`, `core/ModeRegistry.h`, `core/ModeRegistry.cpp`, `core/FlightDataFetcher.cpp`, `core/SystemStatus.h`, `core/SystemStatus.cpp`, `adapters/WebPortal.h`, `adapters/WebPortal.cpp`, `src/main.cpp`, `data-src/dashboard.html`, `data-src/dashboard.js`, `data-src/style.css` |
| **UPDATED (tests)** | 1 | `test/test_mode_registry/test_main.cpp` |
| **REBUILD** | 3 | `data/dashboard.html.gz`, `data/dashboard.js.gz`, `data/style.css.gz` |
| **UNCHANGED** | 30+ | All other firmware files, config headers, models, fetchers, WiFiManager, LayoutEngine, LogoManager, utils, wizard assets, health assets, existing mode .h/.cpp files |
| **NEW DIRECTORY** | 0 | All directories already exist from Display System |

### Struct Placement Clarification

Settings-related structs are split across two files by concern:

| Struct | File | Rationale |
|--------|------|-----------|
| `ModeSettingDef` | `interfaces/DisplayMode.h` | Part of the mode contract — modes declare settings schemas |
| `ModeSettingsSchema` | `interfaces/DisplayMode.h` | Part of the mode contract — modes declare settings schemas |
| `ModeEntry.settingsSchema` field | `core/ModeRegistry.h` | Registry owns mode table entries; field is nullable pointer |
| `ScheduleRule` | `core/ConfigManager.h` | Config storage concern — not part of mode contract |
| `ModeScheduleConfig` | `core/ConfigManager.h` | Config storage concern — not part of mode contract |

### Atomic Extern Pattern

`g_flightCount` is declared in `main.cpp` (rule 30). Files that need access use `extern`:

```cpp
// In core/FlightDataFetcher.cpp — writer:
extern std::atomic<uint8_t> g_flightCount;
// After xQueueOverwrite:
g_flightCount.store(enrichedFlights.size());

// In core/ModeOrchestrator.cpp — reader:
extern std::atomic<uint8_t> g_flightCount;
// In tick():
uint8_t flights = g_flightCount.load();
```

No other files should contain this extern declaration.

### Architectural Boundaries

**API Boundaries (HTTP):**

| Endpoint | Method | Handler | Cross-Core | Purpose |
|----------|--------|---------|------------|---------|
| `/api/ota/check` | GET | WebPortal (Core 1) | None | Check GitHub for new firmware |
| `/api/ota/pull` | POST | WebPortal (Core 1) | Spawns FreeRTOS task | Start firmware download |
| `/api/ota/status` | GET | WebPortal (Core 1) | Read OTAUpdater state | Poll download progress |
| `/api/schedule` | GET | WebPortal (Core 1) | None | Get mode schedule rules |
| `/api/schedule` | POST | WebPortal (Core 1) | None | Save mode schedule rules |
| `/api/display/modes` | GET | WebPortal (Core 1) | Read-only atomic | **UPDATED:** +settings array per mode |
| `/api/display/mode` | POST | WebPortal (Core 1) | Via ModeOrchestrator | **UPDATED:** → onManualSwitch(), +settings obj |
| `/api/status` | GET | WebPortal (Core 1) | Unchanged | **UPDATED:** +ota_available, +orchestrator_state |

**Component Boundaries (Firmware):**

```
┌──────────────────────────────────────────────────────────────────┐
│  Core 1 (WiFi/Web/API task + main loop)                          │
│  ┌────────────────┐   ┌──────────────────┐                       │
│  │   WebPortal    │   │ ModeOrchestrator │ (runs in main loop)   │
│  │  (HTTP routes) │   │   ::tick()       │                       │
│  └───────┬────────┘   └────────┬─────────┘                       │
│          │                     │                                  │
│   onManualSwitch()────────────→│                                  │
│          │                     │ requestSwitch()                  │
│          │                     ├─────────────────→ atomic write   │
│          │                     │                                  │
│  ┌───────┴────────┐   ┌───────┴──────────┐                       │
│  │  OTAUpdater    │   │ FlightDataFetcher│                       │
│  │  (FreeRTOS     │   │  g_flightCount   │                       │
│  │   download     │   │  .store()        │                       │
│  │   task)        │   └──────────────────┘                       │
│  └───────┬────────┘                                              │
└──────────┼───────────────────────────────────────────────────────┘
           │ prepareForOTA() — executes on caller's core (Core 1),
           │ but sets _switchState flag read by Core 0
           │
┌──────────┼───────────────────────────────────────────────────────┐
│  Core 0  │(Display task)                                         │
│  ┌───────▼──────────┐   ┌──────────────────┐                     │
│  │  ModeRegistry    │   │   ActiveMode     │ (DisplayMode*)      │
│  │  ::tick()        │──→│   .render()      │                     │
│  │  checks          │   └──────────────────┘                     │
│  │  _switchState    │                                            │
│  │  fade transition │                                            │
│  └──────────────────┘                                            │
│                                                                  │
│  ┌──────────────────┐   ┌──────────────────┐                     │
│  │ NeoMatrixDisplay │   │  DisplayUtils    │                     │
│  │ .show()          │   │  (free functions)│                     │
│  └──────────────────┘   └──────────────────┘                     │
└──────────────────────────────────────────────────────────────────┘
```

**Data Boundaries:**

| Boundary | Direction | Mechanism |
|----------|-----------|-----------|
| WebPortal → ModeOrchestrator | Core 1 internal | `onManualSwitch()` call (rule 24) |
| ModeOrchestrator → ModeRegistry | Core 1 → Core 0 | `requestSwitch()` atomic write |
| FlightDataFetcher → ModeOrchestrator | Core 1 internal | `g_flightCount` atomic (rule 30) |
| OTAUpdater → ModeRegistry | FreeRTOS task (Core 1) → Core 0 | `prepareForOTA()` executes on caller's core; sets `_switchState` flag that Core 0 reads — cross-core side effect, not cross-core call |
| OTAUpdater → Update.h | FreeRTOS task → flash | Stream-to-partition with incremental SHA-256 (rule 25) |
| ConfigManager → ModeOrchestrator | NVS → Core 1 | `getModeSchedule()` on tick() |
| ConfigManager → Modes | NVS → Core 0 | `getModeSetting()` called in mode `init()`, result cached; never in `render()` |
| Dashboard JS → OTA API | Browser → Core 1 | fetch() + 500ms polling for status |
| Dashboard JS → Schedule API | Browser → Core 1 | fetch() CRUD |
| Modes → ConfigManager | **Via init() only** | `getModeSetting()` in `init()`, cache result; never direct NVS access |
| Modes → NeoMatrixDisplay | **FORBIDDEN** | Modes never access NeoMatrixDisplay directly |
| Modes → g_flightCount | **FORBIDDEN** | Modes never read atomic globals (rule 30) |

### Requirements to Structure Mapping

**FR Group → File Mapping:**

| FR Group | FRs | Primary Files | Supporting Files |
|----------|-----|---------------|-----------------|
| Clock Mode | FR1-FR4 | `modes/ClockMode.h`, `modes/ClockMode.cpp` | `interfaces/DisplayMode.h` (schema structs), `core/ConfigManager` (getModeSetting), `main.cpp` (MODE_TABLE entry) |
| Departures Board | FR5-FR9 | `modes/DeparturesBoardMode.h`, `modes/DeparturesBoardMode.cpp` | `utils/DisplayUtils.h` (rendering), `core/LogoManager.h` (logos), `main.cpp` (MODE_TABLE entry) |
| Mode Transitions | FR10-FR12 | `core/ModeRegistry.h`, `core/ModeRegistry.cpp` | `interfaces/DisplayMode.h` (SwitchState) |
| Mode Scheduling | FR13-FR17, FR36, FR39 | `core/ModeOrchestrator.h`, `core/ModeOrchestrator.cpp` | `core/ConfigManager` (getModeSchedule), `main.cpp` (tick() in loop) |
| Mode Configuration | FR18-FR20, FR40 | `core/ConfigManager.h/.cpp`, mode `.h` files (schemas) | `adapters/WebPortal.cpp` (/api/display/modes iterates schema), `data-src/dashboard.js` (settings panels) |
| OTA Firmware Pull | FR21-FR31, FR37-FR38 | `core/OTAUpdater.h`, `core/OTAUpdater.cpp` | `core/ModeRegistry` (prepareForOTA), `adapters/WebPortal.cpp` (3 endpoints), `data-src/dashboard.js` (OTA UI) |
| System Resilience | FR32-FR35 | `core/ModeOrchestrator.h/.cpp`, `core/OTAUpdater.h/.cpp`, `core/ModeRegistry.h/.cpp`, `core/ConfigManager.h/.cpp` | **FR32** (watchdog → known-good mode): `ModeOrchestrator` boot/restore default mode, integrated from `main.cpp` loop. **FR33** (safe defaults for new Delight keys): `ConfigManager` first-read merge — absent keys get built-in defaults without clobbering existing NVS. **FR34** (heap / transition guards): OTA download blocked or warned below 80KB free heap — `OTAUpdater`; mode transition dual-buffer allocation or MVP fallback — `ModeRegistry` (`_executeFadeTransition`, alloc failure → instant cut). **FR35** (API + matrix responsiveness): flight pipelines — `FlightDataFetcher.cpp`, `OpenSkyFetcher.cpp`, `AeroAPIFetcher.cpp`; idle fallback and display continuity — `ModeOrchestrator`; GitHub/OTA unavailable — `OTAUpdater` + dashboard (`WebPortal`, `data-src/dashboard.js`). |
| Contributor Extensibility | FR41-FR42 | `interfaces/DisplayMode.h` (contract), `main.cpp` (MODE_TABLE) | Existing rule 22 (two touch points) |
| Documentation | FR43 | No firmware files | External documentation only |

**Cross-Cutting Concerns → File Mapping:**

| Concern | Files Touched |
|---------|--------------|
| Fade transition buffer lifecycle (DL1) | `core/ModeRegistry.h/.cpp` (_executeFadeTransition, malloc/free) |
| Mode orchestration state machine (DL2) | `core/ModeOrchestrator.h/.cpp`, `adapters/WebPortal.cpp` (onManualSwitch), `core/FlightDataFetcher.cpp` (g_flightCount), `src/main.cpp` (atomic decl + tick) |
| OTA Pull integrity (DL3) | `core/OTAUpdater.h/.cpp`, `core/ModeRegistry.cpp` (prepareForOTA) |
| Schedule NVS storage (DL4) | `core/ConfigManager.h/.cpp`, `core/ModeOrchestrator.cpp` |
| Per-mode settings schema (DL5) | `interfaces/DisplayMode.h` (structs), all new `modes/*.h` (schema decls), `core/ConfigManager.h/.cpp` (helpers), `adapters/WebPortal.cpp` (API iteration) |
| API endpoint additions (DL6) | `adapters/WebPortal.h/.cpp`, `data-src/dashboard.html/.js/.css` |
| Cross-core atomic signaling | `src/main.cpp` (declaration), `core/FlightDataFetcher.cpp` (extern + store), `core/ModeOrchestrator.cpp` (extern + load) |
| Gzip rebuild | `data/*.gz` (3 files) |

### Integration Points

**Internal Communication:**
- ModeOrchestrator runs in Core 1 main loop (~1/sec tick) — evaluates schedule rules, checks g_flightCount, calls `requestSwitch()` when state transitions require mode change
- OTAUpdater `checkForUpdate()` runs synchronously from WebPortal handler (~1-2s blocking) — acceptable for async web server single-request handling
- OTAUpdater `startDownload()` spawns FreeRTOS task, returns immediately — WebPortal handler responds with `{ ok: true }`; dashboard polls `/api/ota/status` at 500ms
- Fade transition runs on Core 0 inside `ModeRegistry::tick()` during SWITCHING state — `delay()` is acceptable since no other rendering occurs during transition

**External Integrations:**

| Service | Protocol | Purpose | New? |
|---------|----------|---------|------|
| GitHub Releases API | HTTPS GET | Check for new firmware version, download binary + .sha256 | **Yes** |
| NTP (pool.ntp.org) | UDP port 123 | Time for Clock Mode display + schedule evaluation | Existing (Foundation) |

**Data Flow (OTA Pull):**

```
Dashboard "Check for Updates" click
  → GET /api/ota/check
  → OTAUpdater::checkForUpdate()
  → GET https://api.github.com/repos/{owner}/{repo}/releases/latest
  → Parse JSON: tag_name, assets[].browser_download_url
  → Compare tag_name vs FW_VERSION
  → Return { available, version, release_notes }

Dashboard "Install Update" click
  → POST /api/ota/pull
  → OTAUpdater::startDownload() → spawns FreeRTOS task, returns immediately
  → { ok: true, data: { started: true } }
  → Dashboard polls GET /api/ota/status at 500ms

FreeRTOS download task (Core 1):
  → ModeRegistry::prepareForOTA()  (sets _switchState = SWITCHING on Core 1,
                                     blocking Core 0's tick() loop)
  → Download .sha256 file (64 hex chars)
  → Update.begin(partitionSize)
  → Stream .bin in chunks:
      Update.write(chunk) + mbedtls_sha256_update(chunk)
      _progress = bytesWritten * 100 / totalSize
  → mbedtls_sha256_finish() → compare
  → If match: Update.end(true) → ESP.restart()
  → If mismatch: Update.abort() → _state = ERROR
  → _downloadTask = nullptr → vTaskDelete(NULL)
```

**Data Flow (Mode Scheduling):**

```
ModeOrchestrator::tick() [Core 1, ~1/sec]:
  → getLocalTime(&now, 0)  // non-blocking (rule 14)
  → currentMin = hour * 60 + min
  → ConfigManager::getModeSchedule()  // read NVS
  → Evaluate rules in index order (first match wins)
  → If rule matches and state != SCHEDULED:
      _state = SCHEDULED → requestSwitch(rule.mode_id)
  → If no rule matches and state == SCHEDULED:
      _state = MANUAL → requestSwitch(_manualModeId)
  → If no rule matches and g_flightCount == 0 and state == MANUAL:
      _state = IDLE_FALLBACK → requestSwitch("clock")
  → If g_flightCount > 0 and state == IDLE_FALLBACK:
      _state = MANUAL → requestSwitch(_manualModeId)
```

### Delight Release — Party Mode Refinements (Structure)

**Reviewers:** Winston (Architect), Amelia (Developer), Quinn (QA)

**7 refinements incorporated:**

1. `SystemStatus.cpp` promoted to UPDATED — subsystem registration code lives in `.cpp` (Winston)
2. Boundary diagram: `prepareForOTA()` labeled as executing on caller's core (Core 1) with cross-core side effect via `_switchState` flag (Winston)
3. Removed `ClassicCardMode.h` and `LiveFlightCardMode.h` from UPDATED — settingsSchema is in MODE_TABLE in `main.cpp`, not in mode `.h` files (Amelia)
4. Struct placement clarified: `ModeSettingDef`/`ModeSettingsSchema` in `DisplayMode.h`, `settingsSchema` field added to `ModeEntry` in `ModeRegistry.h` (Amelia)
5. `extern std::atomic<uint8_t> g_flightCount` pattern documented for `FlightDataFetcher.cpp` and `ModeOrchestrator.cpp` (Amelia)
6. Added third test file: `test_config_schedule/test_main.cpp` for schedule NVS read/write + compaction logic (Quinn)
7. FR32-FR35 mapping clarified as "existing coverage, no Delight changes needed" for API degradation FRs (Quinn)

## Delight Release — Architecture Validation

### Coherence Validation

**Decision Compatibility:**

All 6 Delight decisions (DL1-DL6) form a consistent dependency chain with no contradictions:
- DL1 (fade transition) integrates into existing ModeRegistry switch lifecycle without modifying the DisplayMode interface
- DL2 (orchestrator) uses the existing `requestSwitch()` atomic mechanism from Display System — no new cross-core primitives needed
- DL3 (OTA Pull) shares `prepareForOTA()` with Foundation's OTA Push — both use the same `Update.h` stream-to-partition path
- DL4 (schedule NVS) and DL5 (per-mode settings NVS) use different key prefixes (`sched_r*` vs `m_*`) — no namespace collisions
- DL6 (API endpoints) follows the existing `{ ok, data/error }` JSON envelope from MVP — consistent pattern

No technology conflicts. No version incompatibilities. mbedTLS is bundled in ESP-IDF — no new external dependencies.

**Pattern Consistency:**

Rules 24-30 are additive to rules 1-23 with no contradictions:
- Rule 24 (orchestrator routing) extends rule 22 (two touch points) — modes don't initiate switches in either system
- Rule 25 (incremental SHA-256) extends rules 12-13 (OTA streaming) — same chunk-by-chunk discipline
- Rule 26 (prepareForOTA) applies to both Foundation Push and Delight Pull — unified pre-OTA sequence
- Rule 27 (fade buffers) follows rule 20 (heap in init/teardown) — transient allocation pattern
- Rule 28 (per-mode NVS helpers) follows rule 11 (NVS abbreviations in ConfigManager.h)
- Rule 30 (atomic globals in main.cpp) follows existing cross-core pattern

Naming conventions: `snake_case` flows consistently through new NVS keys (`sched_r0_start`, `m_clock_format`) → JSON fields → struct fields. PascalCase for classes (`ModeOrchestrator`, `OTAUpdater`), camelCase for methods (`onManualSwitch`, `checkForUpdate`).

**Structure Alignment:**

All Delight changes fit within existing hexagonal architecture boundaries:
- New core components (`ModeOrchestrator`, `OTAUpdater`) in `core/` — correct placement for coordination logic
- New modes (`ClockMode`, `DeparturesBoardMode`) in `modes/` — follows rule 17
- WebPortal gets new routes — same adapter pattern, no new boundary crossings
- ConfigManager gets new category helpers — consistent with existing struct pattern
- No new directories needed — Display System already created `modes/`, `utils/`

### Requirements Coverage

**All 43 Functional Requirements covered:**

| FR Group | FRs | Decision(s) | Coverage |
|----------|-----|-------------|----------|
| Clock Mode | FR1-FR4 | DL2, DL5 | ClockMode implementation + orchestrator idle fallback + 12/24h setting |
| Departures Board | FR5-FR9 | DL5 | DeparturesBoardMode implementation + configurable rows/fields settings |
| Mode Transitions | FR10-FR12 | DL1 | Fade transition with RGB888 dual buffers, double-buffering prevents tearing |
| Mode Scheduling | FR13-FR17, FR36, FR39 | DL2, DL4, DL6 | Orchestrator state machine + NVS persistence + schedule API endpoints |
| Mode Configuration | FR18-FR20, FR40 | DL5, DL6 | Per-mode settings schema + NVS helpers + Mode Picker UI + manual switch |
| OTA Firmware Pull | FR21-FR31, FR37-FR38 | DL3, DL6 | OTAUpdater component + GitHub API + SHA-256 + FreeRTOS task + 3 API endpoints |
| System Resilience | FR32-FR35 | DL1, DL3, existing | Watchdog → MANUAL boot, safe defaults, heap pre-check, API degradation (existing fetcher handling) |
| Contributor Extensibility | FR41-FR42 | Existing (rule 22) | DisplayMode contract + MODE_TABLE + Mode Picker lists all registered modes |
| Documentation | FR43 | N/A | No architectural impact — documentation validation only |

**All 21 Non-Functional Requirements architecturally addressed:**

| NFR | Target | Architectural Support |
|-----|--------|----------------------|
| NFR1 | ≥15fps transitions | DL1: FADE_STEPS=15, ~66ms/frame blend loop on Core 0 |
| NFR2 | <1 second transitions | DL1: 15 frames x ~66ms = ~1s max |
| NFR3 | ~30s idle fallback | DL2: ModeOrchestrator tick() checks g_flightCount each second |
| NFR4 | <50ms row update | DeparturesBoardMode in-place render(), no full-screen redraw |
| NFR5 | <60s OTA | DL3: stream-to-partition, ~10-15s for 1.5MB on 10Mbps |
| NFR6 | ±5s schedule accuracy | DL2: 1/sec tick with getLocalTime() non-blocking |
| NFR7 | <1s page load | Existing: gzipped assets, no external deps |
| NFR8 | <2s setting apply | DL5: ConfigManager hot-reload, no reboot |
| NFR9 | 30-day uptime | DL1 buffer lifecycle (malloc/free), DL3 task self-delete, no persistent allocations |
| NFR10 | 10s watchdog | Existing: FreeRTOS watchdog |
| NFR11 | Default mode after watchdog | DL2: ModeOrchestrator boots MANUAL state |
| NFR12 | OTA never bricks | DL3: Update.abort() on failure + Foundation dual-partition |
| NFR13 | Heap ceiling | Existing: MEMORY_REQUIREMENT check in switch flow |
| NFR14 | 2MB firmware | Monitor binary size; OTAUpdater adds ~10-15KB |
| NFR15 | NVS survives updates | NVS partition separate from app partitions |
| NFR16 | 80KB free heap | DL3: pre-download check; DL1: malloc fallback to instant cut |
| NFR17 | No heap degradation | DL1: malloc/free in single call; DL3: task self-deletes |
| NFR18 | API unavailability | Existing fetcher handling + DL3 check failure → ERROR state |
| NFR19 | Dashboard status line | DL6: GET /api/status + SystemStatus OTA_PULL entry |
| NFR20 | GitHub 429 handling | DL3: "try again later" message, no auto-retry |
| NFR21 | 10s network latency | Existing timeout handling in HTTPClient |

**Orphan check:** 0 FRs without architectural support. 0 decisions without FR traceability.

### Implementation Readiness

**Decision Completeness:** 6 decisions with code examples, interface definitions, state machine diagrams, and rationale. Implementation sequence specified with dependency graph. Cross-decision dependencies mapped.

**Structure Completeness:** Complete file change map — 11 new files (8 source + 3 test), 15 updated files. Requirements-to-file mapping explicit for all 43 FRs. Struct placement clarified across DisplayMode.h, ModeRegistry.h, and ConfigManager.h.

**Pattern Completeness:** 30 enforcement rules (11 MVP + 5 Foundation + 7 Display System + 7 Delight). All rules include code examples and anti-patterns. Cross-core signaling, OTA streaming, orchestrator routing, fade buffer lifecycle, per-mode settings NVS, and schedule NVS patterns all specified.

### Gap Analysis

**Critical Gaps:** 0

**Important Gaps:** 0

**Minor Notes (not blocking):**

1. **FR26 LED progress display** — Architecture specifies `prepareForOTA()` shows "Updating..." text on LEDs, but the exact progress bar rendering is an implementation detail. OTA download task can write progress directly to the LED array after `prepareForOTA()`.

2. **Clock Mode NTP fallback** — Architecture notes "falls back to uptime-based display if NTP unavailable" but doesn't specify exact fallback format. Implementation detail for ClockMode.render().

3. **Schedule rule naming** — Dashboard UI will need auto-generated labels (e.g., "Rule 1: 06:00-22:00 → Departures"). Pure UI rendering — no architectural decision needed.

### Architecture Completeness Checklist

**Requirements Analysis**
- [x] Project context analyzed (43 FRs, 21 NFRs, 5 party mode refinements from context review)
- [x] Scale and complexity assessed (medium — 2 new modes + orchestration + OTA)
- [x] Technical constraints identified (NVS key limits, heap budget, GitHub rate limit)
- [x] Cross-cutting concerns mapped (7 concerns including buffer lifecycle, state machine, OTA coexistence)

**Architectural Decisions**
- [x] 6 critical/important decisions documented with code examples
- [x] No new external dependencies (mbedTLS bundled in ESP-IDF)
- [x] Implementation sequence with dependency graph
- [x] Deferred decisions explicitly noted (multiple transitions, schedule templates, OTA signing)

**Implementation Patterns**
- [x] 7 new enforcement rules (24-30) with examples and anti-patterns
- [x] All rules additive — no changes to MVP (1-11), Foundation (12-16), or Display System (17-23)
- [x] Cross-core signaling pattern documented
- [x] OTA error discipline specified (Update.abort() on every error path)

**Project Structure**
- [x] Complete file change map (11 new + 15 updated + 3 rebuild)
- [x] Architectural boundaries with cross-core diagram
- [x] Data boundaries table with 12 entries
- [x] Requirements-to-file mapping for all FR groups
- [x] Atomic extern pattern documented

### Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION

**Confidence Level:** High — all FRs/NFRs covered, 3 party mode reviews with 23 total refinements incorporated, no critical or important gaps.

**Key Strengths:**
- Orchestrator state machine (DL2) cleanly separates three mode-switch sources with defined priority
- OTA Pull (DL3) reuses Foundation's streaming pattern — minimal new code surface
- Per-mode settings schema (DL5) follows existing ModeZoneDescriptor pattern — familiar to agents
- Fade transition (DL1) degrades gracefully on heap pressure — never blocks or crashes

**Architectural Overrides:**
- Context analysis section references "~20KB RGB565" buffers (from party mode round 1); decisions section corrects this to "~30KB RGB888" (party mode round 2). The decisions section is authoritative.
- FR35 API degradation marked "existing coverage" — no new Delight code needed for fetcher resilience

## Display System Release — Architecture Validation

### Coherence Validation

**Decision Compatibility:**

All 7 decisions (D1-D7) form a consistent dependency chain with no contradictions:
- D1 (interface) defines the contract that D2 (registry) manages and D3 (NeoMatrixDisplay) supports
- D4 (display task) integrates D2 and D3 at the system level
- D5 (API) and D7 (UI) operate through D2's public interface
- D6 (NVS) is called from D2's tick() debounce path
- The D1/D2 `getMemoryRequirement()` contradiction (instance method vs static function pointer) was caught and resolved in party mode review #2
- No version conflicts — all existing ESP32 Arduino libraries remain at current versions

**Pattern Consistency:**

- Enforcement rules 17-23 are additive to rules 1-16 with no conflicts
- Include path convention (path-from-firmware-root) is consistent with existing codebase
- API endpoint pattern (`/api/display/*`) follows existing `/api/*` JSON envelope convention
- NVS key pattern (`display_mode`) follows existing namespace convention
- Web asset pattern (fetch + json.ok + showToast) carried forward to Mode Picker

**Structure Alignment:**

- File change map covers all architectural decisions with explicit file locations
- PlatformIO build config updated for new directories (blocker caught in party mode #3)
- Boundaries clearly enforce: modes never touch ConfigManager/WiFiManager/SystemStatus (rule 18), never call FastLED.show() (rule 19)
- Two-touch-point extensibility pattern (rule 22) verified against file structure

### Requirements Coverage

**All 36 Functional Requirements covered.** Every FR maps to at least one architectural decision with explicit code location:
- DisplayMode Interface (FR1-FR4): Decision D1 — `interfaces/DisplayMode.h`
- Mode Registry & Lifecycle (FR5-FR10): Decision D2 — `core/ModeRegistry.h/.cpp`, `src/main.cpp`
- Classic Card Mode (FR11-FR13): Decision D3 — `modes/ClassicCardMode.h/.cpp`
- Live Flight Card Mode (FR14-FR16): New mode — `modes/LiveFlightCardMode.h/.cpp`
- Mode Picker UI (FR17-FR26): Decision D7 — `data-src/dashboard.html/js/css`
- Mode Persistence (FR27-FR29): Decision D6 — `core/ConfigManager.h/.cpp`, `src/main.cpp`
- Mode Switch API (FR30-FR33): Decision D5 — `adapters/WebPortal.h/.cpp`
- Display Infrastructure (FR34-FR36): Decisions D3 + D4 — `adapters/NeoMatrixDisplay.h/.cpp`, `src/main.cpp`

**All 12 NFRs architecturally addressed:**
- Performance: cooperative rendering within 16ms frame budget (P2), sub-2s mode switch (P1), zero-heap API enumeration (P4), <1s UI load (P3)
- Stability: sequential teardown→init for 100-switch heap stability (S1), switch serialization for rapid-switch safety (S2), heap-in-init discipline (S3), 30KB heap floor guard (S4), read-only brightness (S5), verified NVS namespace (S6)
- Compatibility: RenderContext isolation guarantees Foundation features untouched (C2), pixel parity via extraction validation (C1), cooperative scheduling preserved (C3), dashboard consistency (C4)

### Implementation Readiness

**Decision Completeness:** 7 decisions with code examples, struct definitions, switch flow pseudocode, API request/response formats, and rationale. Memory requirement pattern resolved (static constexpr + function pointer). Switch restoration pattern specified (teardown-but-don't-delete).

**Structure Completeness:** 10 NEW files + 12 UPDATED files + 1 new directory. PlatformIO build config updated. Include path conventions documented. Requirements-to-file mapping explicit. No orphaned files, no unmapped FRs.

**Pattern Completeness:** 23 enforcement rules (11 MVP + 5 Foundation + 7 Display System). Mode implementation pattern, rendering context discipline, shared utilities pattern, registration pattern, and UI patterns all specified with code examples and anti-patterns.

### Revised Implementation Sequence

**Critical path (three-phase ClassicCardMode extraction):**

1. **Heap baseline measurement** — `ESP.getFreeHeap()` log after full Foundation boot, before any Display System code
2. **DisplayMode interface + RenderContext** (`interfaces/DisplayMode.h`) — everything depends on this contract
3. **ModeRegistry** (`core/ModeRegistry.h/.cpp`) — switch logic, heap guard, tick(), terminal fallback path
4. **DisplayUtils extraction** (`utils/DisplayUtils.h/.cpp`) — extract shared helpers from NeoMatrixDisplay first
5. **ClassicCardMode — three-phase extraction:**
   - 5a: Create ClassicCardMode with COPIED rendering logic (NeoMatrixDisplay keeps its methods — both paths active)
   - 5b: Validate pixel parity — run both old and new paths against same flight data, compare output on hardware (human-in-the-loop gate)
   - 5c: Remove extracted methods from NeoMatrixDisplay, add `show()` + `buildRenderContext()` + `displayFallbackCard()`
6. **Display task integration** (`main.cpp`) — replace renderFlight with ModeRegistry::tick() + show()
7. **LiveFlightCardMode** (`modes/LiveFlightCardMode.h/.cpp`) — second mode, validates the abstraction works

**Parallel with steps 4-7:**
8. **Mode Switch API** (`WebPortal.cpp`) — GET + POST endpoints, can be stubbed early
9. **NVS persistence** (`ConfigManager.h/.cpp`) — `getDisplayMode()` / `setDisplayMode()`, trivial addition

**After all firmware complete:**
10. **Mode Picker UI** (`dashboard.html/js/css`) — depends on working API endpoints
11. **Gzip rebuild** — all updated web assets

### Architectural Overrides

**FR4 zone bounds — developer discipline, not code enforcement:**

FR4 states "display mode can render flight data within its allocated zone bounds without affecting pixels outside its zone." The architecture provides zone bounds via `LayoutResult` in `RenderContext`, but does not enforce clipping at the GFX layer — modes receive the full `Adafruit_NeoMatrix*` object. Adafruit GFX/NeoMatrix does not natively support clipping rectangles in the way needed. Zone discipline is enforced by developer practice, code review, and enforcement rule 18 (RenderContext discipline). Story acceptance criteria for each mode should include "visual verification that rendering stays within zone bounds."

**NFR P2 LogoManager exception — grandfathered filesystem I/O:**

NFR P2 prohibits filesystem I/O on the hot path. `LogoManager::loadLogo()` reads from LittleFS and is called from mode `render()` methods. This is inherited behavior from the pre-mode-system codebase — the same LittleFS reads occurred in `NeoMatrixDisplay::renderLogoZone()` before extraction. LittleFS reads for 2KB logo files on ESP32 flash complete in <1ms, well within the 16ms frame budget. NFR P2's filesystem prohibition applies to NEW mode code; LogoManager access is the single grandfathered exception.

### Terminal Fallback Path (FR36 Safety Net)

The D2 switch flow specifies teardown-but-don't-delete for safe restoration. Party mode review identified a gap: what happens if re-initialization of the previous mode ALSO fails?

**Complete terminal path:**

```
Switch requested → teardown current → heap check fails → re-init previous
                                                            ↓
                                                     init() succeeds → restored ✅
                                                     init() fails → _activeMode = nullptr
                                                            ↓
                                                     Display task detects nullptr:
                                                     g_display.displayFallbackCard(flights) → FR36 ✅
```

`displayFallbackCard()` uses `displaySingleFlightCard()` — the legacy single-flight renderer that has zero heap dependencies. This is the terminal safety net. If reached, the wall displays a degraded single-flight card, and the error propagates to the Mode Picker UI via `getLastError()`.

### Flight Cycling Pattern (Highest-Risk Extraction Point)

Party mode review identified flight cycling timing as the single most likely implementation error. The cycling check must use the configured interval from RenderContext, not the frame rate:

```cpp
void ClassicCardMode::render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) {
    if (flights.empty()) {
        // Mode owns its idle state — show "..." or blank
        ctx.matrix->fillScreen(0);
        return;
    }

    // Advance flight index on configured interval (NOT every frame)
    // render() is called ~20fps (50ms). displayCycleMs is ~5000ms.
    // This check passes once every ~100 frames.
    if (millis() - _lastCycleMs >= ctx.displayCycleMs) {
        _currentFlightIndex = (_currentFlightIndex + 1) % flights.size();
        _lastCycleMs = millis();
    }

    // Bounds check in case flights vector shrank between cycles
    if (_currentFlightIndex >= flights.size()) {
        _currentFlightIndex = 0;
    }

    const auto& flight = flights[_currentFlightIndex];
    // ... render the current flight using DisplayUtils helpers ...
}
```

**Anti-pattern:** Do NOT use `delay()` or `vTaskDelay()` inside `render()`. Do NOT assume render() is called at any specific frequency. Always use `millis()` delta against `ctx.displayCycleMs`.

### NFR Validation Notes

**NFR S1/S2 stress testing:**

100-switch heap stability (S1) and 10-in-3-seconds rapid-switch safety (S2) require hardware stress testing that cannot be automated in unit tests. Implementation handoff requirement: add a `#ifdef DEBUG` serial command handler in `main.cpp` that accepts a `STRESS_SWITCH` command — triggers 100 sequential mode switches with before/after `ESP.getFreeHeap()` measurement and pass/fail output to serial. This is a one-time integration validation, not CI.

**NFR C1 pixel parity:**

Classic Card pixel parity is a human-in-the-loop validation gate. The implementing agent produces the extracted ClassicCardMode code. The human validates on physical hardware by comparing pre-extraction and post-extraction output for at least 5 distinct flight cards. Story acceptance criteria must reflect this split: agent delivers code, human confirms visual parity.

### Gap Analysis

| Priority | Gap | Resolution |
|----------|-----|-----------|
| Minor | `_switchState` read cross-core without atomic | ESP32 Xtensa: byte-sized enum reads are hardware-atomic. Safe in practice. Note for implementor: change to `std::atomic<SwitchState>` if portability desired. |
| Minor | FR7 "queuing" vs atomic overwrite | Architecture uses last-writer-wins during IDLE, SWITCH_IN_PROGRESS rejection during SWITCHING. Matches FR7 intent — concurrent requests serialized, not dropped silently. |
| Minor | `displayFallbackCard()` vs `displaySingleFlightCard()` naming | `displayFallbackCard()` wraps existing `displaySingleFlightCard()` logic. Implementor keeps internal method, adds public wrapper. |

No critical gaps. No important gaps.

### Display System Architecture Completeness Checklist

- [x] Display System PRD context analyzed (36 FRs, 12 NFRs, constraints)
- [x] Scale assessed (medium-high — plugin architecture on memory-constrained MCU)
- [x] Constraints identified (ESP32 heap, single display task, cross-core coordination)
- [x] Cross-cutting concerns mapped (RenderContext isolation, heap lifecycle, NVS debounce)
- [x] No new external dependencies (all existing ESP32 Arduino libraries)
- [x] 7 Display System decisions documented with code examples
- [x] Implementation patterns extended (rules 17-23)
- [x] NVS key table expanded (1 new key: `display_mode`)
- [x] 2 new API endpoints specified with request/response formats
- [x] Complete file change map (10 NEW + 12 UPDATED files)
- [x] Requirements-to-file mapping complete (36 FRs → specific files)
- [x] PlatformIO build config updated (blocker caught in party mode #3)
- [x] Architectural boundaries documented with forbidden paths
- [x] Three-phase ClassicCardMode extraction sequence specified
- [x] Terminal fallback path for FR36 documented
- [x] Flight cycling pattern with anti-patterns documented
- [x] NFR S1/S2 stress test mechanism specified
- [x] NFR C1 human-in-the-loop gate documented
- [x] All party mode refinements incorporated (3 sessions, 17+ refinements total)

### Display System Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION
**Confidence Level:** High

**Key Strengths:**
- Clean plugin architecture on ESP32 — modes isolated via RenderContext with zero subsystem coupling
- Three-phase extraction sequence prevents the highest-risk failure (ClassicCardMode pixel parity regression)
- Heap lifecycle management with terminal fallback path — wall always displays something, even after cascading failures
- Two-touch-point extensibility (rule 22) — adding a mode is one file + one line
- Cross-core coordination mirrors existing `g_configChanged` pattern — no new concurrency primitives
- NVS debounce prevents flash wear during rapid switching
- Party mode reviews across 3 sessions caught: build blocker (platformio.ini), D1/D2 contradiction (memory requirement), extraction ordering risk, terminal fallback gap, cycling timing as highest-risk point
- All 23 enforcement rules are additive — no changes to MVP (1-11) or Foundation (12-16) rules

**Areas for Future Enhancement:**
- Mode-specific user settings (per-mode NVS keys and UI panels)
- Animated transitions between modes (blank transition only in this release)
- Mode scheduling (auto-switch by time of day)
- GFX clipping enforcement for third-party mode safety
- Automated pixel parity testing (screenshot comparison tooling)

### Display System Implementation Handoff

**Pre-implementation gates:**
1. Measure heap baseline: `ESP.getFreeHeap()` after full Foundation boot (WiFi + web server + NTP + flight data). This establishes the mode memory budget.
2. If free heap is below 60KB: investigate memory reduction before proceeding (30KB floor + headroom for two modes).
3. Photograph/record current Classic Card display with 5+ distinct flights — this is the pixel parity reference.

**First implementation priority:**
1. Create `interfaces/DisplayMode.h` (RenderContext, ModeZoneDescriptor, DisplayMode abstract class)
2. Create `core/ModeRegistry.h/.cpp` (static table, tick(), switch lifecycle with terminal fallback)
3. Extract `utils/DisplayUtils.h/.cpp` from NeoMatrixDisplay helpers
4. Create `modes/ClassicCardMode.h/.cpp` (copy — not extract — rendering logic from NeoMatrixDisplay)
5. Validate pixel parity on hardware (human gate)
6. Refactor NeoMatrixDisplay (remove extracted methods, add show() + buildRenderContext())
7. Update `platformio.ini` build_src_filter and build_flags
8. Integrate display task in `main.cpp` (MODE_TABLE, ModeRegistry::init(), tick()+show())
9. Verify firmware builds and ClassicCardMode renders correctly through ModeRegistry

**NFR validation requirements:**
- Add `#ifdef DEBUG` serial stress test handler for S1/S2 validation
- NFR C1 pixel parity is a manual hardware gate — agent delivers code, human confirms


]]></file>
<file id="8934f9af" path="_bmad-output/planning-artifacts/epic-le-1-layout-editor.md" label="EPIC"><![CDATA[

---
epicId: LE-1
title: Layout Editor V1 MVP
status: planned
owner: Winston (Architect)
createdDate: '2026-04-17'
inputDocuments:
  - _bmad-output/planning-artifacts/layout-editor-v0-spike-report.md
  - _bmad-output/planning-artifacts/layout-editor-evidence-hunt.md
  - _bmad-output/planning-artifacts/architecture.md
  - _bmad-output/planning-artifacts/epics-display-system.md
  - _bmad-output/planning-artifacts/epics-delight.md
---

# Epic LE-1: Layout Editor V1 MVP

## Summary

Epic LE-1 delivers the **Layout Editor V1 MVP** — user-authored display layouts composed via a drag-and-drop editor on the captive-portal dashboard, rendered on the device by a new generic `CustomLayoutMode`. The V0 spike (2026-04-17) validated technical feasibility: a JSON-driven renderer coexists with the four fixed modes at **+4.3 KB flash, no measurable heap pressure, and p95 render time 1.23× ClassicCardMode baseline** after two widget-level optimizations. Architecture is proven; this epic promotes the spike scaffold to production, expands the widget set from three to five, adds the editor UX, and ships a persistence + REST layer. The LayoutEngine shared-zone algorithm used by the four fixed modes is **untouched** — CustomLayoutMode is a peer mode that bypasses it by design.

---

## Goals

- Ship a production-quality `CustomLayoutMode` as the 5th entry in the `ModeRegistry` mode table.
- Provide a dashboard editor that an owner can use to create, edit, save, activate, and delete a custom layout in under 30 seconds on a phone.
- Offer five closed widget types at launch: `text`, `clock`, `logo`, `flight_field`, `metric`.
- Persist layouts as LittleFS JSON files; a single NVS key tracks the active layout id.
- Maintain strict parity with existing quality gates: binary under 92% partition, ≥ 30 KB heap margin, render p95 ≤ 1.2× ClassicCardMode baseline.
- Reuse the existing RGB565 logo pipeline (Story 4-3) for the `logo` widget.

## Non-Goals (V1)

- Multi-layout scheduling. V1 supports authoring multiple layouts but only one is active at a time. Schedule-driven layout switching is V2.
- Animations, transitions, or time-based widget visibility.
- Conditional visibility rules (e.g., "show metric X only when ground speed > 100 kts").
- Pixel-precise default drag. Snap-to-8px is the default; 1px is only available behind an Advanced disclosure.
- Custom hex color picker. V1 exposes an ~8-color LED-friendly palette.
- Web-based preview of **real device state**. The editor previews the layout with placeholder/sample data only — the device is not the source of truth during editing.
- Community marketplace, import/export of third-party layouts, or remote sharing.

## User Value

The Layout Editor serves **two distinct audiences**, and the epic is deliberately designed to deliver to both without branching the codebase:

- **OSS builder / authorship audience** — Contributors and hobbyists who want creative control over what their wall displays. Today, adding a visual variant requires a C++ mode, a registry entry, a recompile, and a flash cycle. LE-1 collapses that loop to "open editor → drag widgets → save" with no toolchain required.
- **Commercial buyer / personalization audience** — Non-technical owners who want their FlightWall to say their name, show their route pairs, or match a theme without touching firmware. The dashboard editor is the only surface they ever see.

The audience split matters for scope decisions: the authorship audience tolerates rough edges to gain flexibility, while the personalization audience demands the 30-second-to-save UX path. V1 optimizes for the shared baseline; both audiences get the same tool.

---

## Architecture

### Mode model

`CustomLayoutMode` is the 5th `ModeEntry` in the `main.cpp` mode table. It coexists with `ClassicCardMode`, `LiveFlightCardMode`, `ClockMode`, and `DeparturesBoardMode`. The four fixed modes are **untouched**.

- The existing `LayoutEngine` shared-zone algorithm continues to serve the four fixed modes. CustomLayoutMode **does not** call `LayoutEngine`; it renders directly from its parsed widget list into the display buffer.
- Lifecycle is standard `DisplayMode`: `init()` loads and parses the active layout JSON once, populates a fixed `WidgetInstance[]` array, and releases the `JsonDocument`. `render()` iterates the array and dispatches via a static table. `teardown()` is a no-op on persistence; only the in-memory array is freed.
- Worst-case memory requirement is computed from the fixed array cap (~24 widgets) and the largest widget's per-instance footprint.

### Persistence

- **LittleFS** stores layout documents at `/layouts/<id>.json`. Each file is 2–8 KB; the `/layouts/` directory is capped at **64 KB total** and **16 layouts max**. LayoutStore enforces both on write.
- **NVS** stores only `layout_active` (12-character key, matching the 15-char NVS limit). This is the one key that's read at boot to resolve which layout to load into CustomLayoutMode.
- Logo bitmaps continue to live in the existing LogoManager store (RGB565, LittleFS). The `logo` widget references them by id — no bitmap bytes are embedded in layout JSON.

### Widget set (V1)

A **closed registry** of five widget types:

| Type           | Purpose                                                                      | V0 status        |
| -------------- | ---------------------------------------------------------------------------- | ---------------- |
| `text`         | Static or templated text with font preset and alignment.                     | Proven in spike. |
| `clock`        | Current time, 12/24h. Cached at minute resolution.                           | Proven in spike. |
| `logo`         | Reference to a stored RGB565 logo by id.                                     | Proven in spike (stub bitmap). |
| `flight_field` | Live flight telemetry field (callsign, altitude, speed, heading, vert rate). | **New in V1.**   |
| `metric`       | Derived numeric readouts (e.g., active flight count, uptime, heap).          | **New in V1.**   |

The three spike widgets form the proven subset. `flight_field` and `metric` are net-new in LE-1.8 and follow the same dispatch pattern.

### Layout JSON schema

Annotated example of the V1 schema. Same shape as the spike with V1 widget types added:

```json
{
  "id": "my-home-wall",
  "name": "Home Wall",
  "v": 1,
  "canvas": { "w": 160, "h": 32 },
  "bg": "#000000",
  "widgets": [
    {
      "id": "w1",
      "type": "text",
      "x": 0, "y": 0, "w": 96, "h": 10,
      "content": "WELCOME HOME",
      "font": "M",
      "align": "left",
      "color": "#FFAA00"
    },
    {
      "id": "w2",
      "type": "clock",
      "x": 100, "y": 0, "w": 60, "h": 10,
      "format": "24h",
      "font": "M",
      "color": "#FFFFFF"
    },
    {
      "id": "w3",
      "type": "logo",
      "x": 0, "y": 12, "w": 32, "h": 20,
      "logo_id": "united_1"
    },
    {
      "id": "w4",
      "type": "flight_field",
      "x": 34, "y": 12, "w": 70, "h": 10,
      "field": "callsign",
      "font": "M",
      "color": "#00FF88"
    },
    {
      "id": "w5",
      "type": "metric",
      "x": 106, "y": 12, "w": 54, "h": 10,
      "metric": "flight_count",
      "format": "{} flights",
      "font": "S",
      "color": "#AAAAAA"
    }
  ]
}
```

Fields common to every widget: `id`, `type`, `x`, `y`, `w`, `h`. Type-specific fields are documented per widget in LE-1.2 / LE-1.5 / LE-1.8. `v: 1` is the schema version; LayoutStore rejects unknown versions.

### Render path

Validated dispatch pattern carried forward from the V0 spike:

```cpp
// Inside CustomLayoutMode::render()
for (uint8_t i = 0; i < widgetCount_; ++i) {
    const WidgetInstance& w = widgets_[i];
    switch (w.type) {
        case WidgetType::Text:        renderText(w, ctx); break;
        case WidgetType::Clock:       renderClock(w, ctx); break;
        case WidgetType::Logo:        renderLogo(w, ctx); break;
        case WidgetType::FlightField: renderFlightField(w, ctx); break;
        case WidgetType::Metric:      renderMetric(w, ctx); break;
    }
}
```

- Render functions are stateless singletons. No per-frame heap allocation.
- No virtual dispatch; a `switch` on a packed enum is materially cheaper on ESP32 than a v-table indirection for a loop this hot.
- `WidgetInstance` is a fixed-size POD (cap ~24 widgets). Total mode memory is predictable at compile time.

### Editor architecture

- **Hand-rolled Pointer Events**, not a DnD library. ES5-only constraint plus iOS Safari quirks made a library a liability in the spike prototype.
- **Single `<canvas>`** rendered at **4× CSS scale** with `imageSmoothingEnabled = false`. Nearest-neighbor upscale. No anti-aliasing anywhere in the editor preview — the device itself has no sub-pixel smoothing, so the editor must not lie.
- **Full re-render on any change.** No dirty-rect optimizations; the canvas is 160×32 × 4 = small enough to redraw at 60fps on a phone.
- **Browser-only state during editing.** The ESP32 is **not** the source of truth while editing. The editor holds the full layout document in JavaScript; only `POST /api/layouts/:id` on save hits the device. This keeps the editor responsive even on a slow WiFi link and removes the device from the edit-debug loop.
- `touch-action: none` on the canvas element to prevent iOS Safari from consuming pointer gestures.

### REST API

All endpoints use the standard envelope `{ ok, data, error, code }`.

| Method | Path                            | Purpose                                                         |
| ------ | ------------------------------- | --------------------------------------------------------------- |
| GET    | `/api/layouts`                  | List layouts: `[{ id, name, size_bytes, modified }]`.           |
| POST   | `/api/layouts`                  | Create a new layout from a JSON body. Returns the assigned id.  |
| GET    | `/api/layouts/:id`              | Fetch full layout document.                                     |
| PUT    | `/api/layouts/:id`              | Replace layout document. Schema-validated before commit.        |
| DELETE | `/api/layouts/:id`              | Delete layout. Rejects if currently active.                     |
| POST   | `/api/layouts/:id/activate`     | Set `layout_active` in NVS. Triggers `CustomLayoutMode` reload on next mode switch. |
| GET    | `/api/widgets/types`            | Return widget type descriptors (min dims, fields, enums) for editor introspection. |

`GET /api/widgets/types` is the introspection surface that lets the editor render widget palettes and property panels without hardcoding widget schemas in JS — the firmware is the source of truth for what each widget accepts.

---

## Implementation Pattern Requirements (non-negotiable)

These five patterns are carried forward verbatim from the V0 spike's "V1 must carry forward" section. Each is enforced in code review for every LE-1 story.

1. **Bitmap-blit path, not fillRect.** Use `DisplayUtils::drawBitmapRGB565` (or equivalent) for any large-area paint. `fillRect` is materially more expensive on this hardware. The spike demonstrated this after the p95 regression was traced to logo fillRect calls.
2. **Cache time-derived state at minute resolution.** Clock strings, date strings, any formatted time output — compute once per minute boundary, not per frame. The spike's initial p95 was visibly hurt by per-frame `snprintf` of the clock string.
3. **Parse JSON at mode init only.** `CustomLayoutMode::init()` parses the layout, populates `WidgetInstance[]`, then frees the `JsonDocument`. `render()` must not see JSON.
4. **Fixed `WidgetInstance[]` array, no dynamic containers.** Cap raised from 8 (spike) to ~24 (V1). No `std::vector`, no heap growth at runtime.
5. **Hand-rolled Pointer Events in the editor, no DnD library.** Single `<canvas>` at 4× scale, `imageSmoothingEnabled = false`, full re-render on every change.

Any story PR that violates one of these without an explicit architecture-sign-off note is rejected.

---

## Story Breakdown

Nine stories. Size T-shirts: S = ~0.5 day, M = ~1 day, L = ~2 days.

| Story   | Title                                                        | Size | Description                                                                                              |
| ------- | ------------------------------------------------------------ | ---- | -------------------------------------------------------------------------------------------------------- |
| LE-1.1  | LayoutStore: LittleFS CRUD + schema validation              | M    | File I/O, id allocation, 64KB/16-file cap enforcement, JSON schema validator. Includes V0 spike cleanup. |
| LE-1.2  | WidgetRegistry + core 3 widget types (text, clock, logo)    | M    | Dispatch table, `WidgetInstance` POD, render functions for the three proven spike widgets.               |
| LE-1.3  | CustomLayoutMode promoted from spike to production          | M    | Remove `#ifdef LE_V0_SPIKE` gating, wire into permanent mode table entry, worst-case memory reporting.   |
| LE-1.4  | REST endpoints `/api/layouts/*` + activate flow             | S    | Seven endpoints, standard envelope, integrate with `ModeRegistry::switchMode`.                           |
| LE-1.5  | LogoStore reuse + `logo` widget (real bitmap)                | M    | Replace spike's stub with real RGB565 blit via LogoManager. Reuses Story 4-3 pipeline.                   |
| LE-1.6  | Editor page: canvas, pointer DnD, selection, resize          | L    | Single canvas at 4× scale, Pointer Events, snap-to-8px, selection rect, resize handles.                  |
| LE-1.7  | Editor property panel + widget palette + save/load UX        | M    | Right-side property panel, left-side palette, `/api/widgets/types` integration, save toast.              |
| LE-1.8  | `flight_field` and `metric` widgets                          | M    | Two new widget types; telemetry field bindings; `metric` format-string support.                          |
| LE-1.9  | Golden-frame regression tests + binary/heap budget gate     | S    | Byte-equal frame comparison for canonical layouts; CI gate on flash % and heap margin.                   |

**BMAD / bmad-assist:** Story IDs (`le-1.1` … `le-1.9`) are declared in the sharded epic [`epics/epic-le-1.md`](epics/epic-le-1.md) (`### Story le-1.x:` headings). This file keeps the roadmap table; full acceptance criteria live in per-story files under `_bmad-output/implementation-artifacts/stories/`.

**LE-1.1 includes the V0 spike cleanup as a subtask** — see "V0 Spike Cleanup" below for the exact revert list.

---

## V0 Spike Cleanup (prerequisites folded into LE-1.1)

The V0 spike left instrumentation and override paths behind development flags. LE-1.1 is the first story that touches the mode table and filesystem layout, so the cleanup lands there.

Items to revert or formalize:

1. **Remove `#ifdef LE_V0_METRICS` instrumentation block** in `firmware/src/main.cpp`:
   - The ring buffer
   - `le_v0_record`
   - `le_v0_maybe_log`
   - The wrap around `ModeRegistry::tick()`
2. **Remove `#ifdef LE_V0_METRICS` boot-override block** in `main.cpp`'s `setup()` mode-selection area (the code that forces the spike mode at boot for measurement).
3. **Remove `#ifdef LE_V0_METRICS` early-return guard** in `ModeOrchestrator::onIdleFallback()` (the block that prevented idle fallback from stealing the measurement mode).
4. **Remove `statusMessageVisible = false;` bypass** under `LE_V0_METRICS` in `displayTask` (overlay suppression for clean frame timing).
5. **Delete `firmware/modes/v0_spike_layout.h`** — the hardcoded spike layout is replaced by LittleFS-loaded JSON in LE-1.1.
6. **Resolve the `#ifdef LE_V0_SPIKE` guard around the mode-table entry**. Two acceptable landings:
   - **(a) Unconditional** once LE-1.3 lands and `CustomLayoutMode` is a real mode. This is the recommended path.
   - **(b) Compile-time feature flag** controlled by LE-1 config, if we want a kill-switch for commercial SKUs. Decide at LE-1.3 review.
7. **Fate of `[env:esp32dev_le_baseline]` and `[env:esp32dev_le_spike]` in `platformio.ini`.** Recommendation: **keep both as dev-only measurement environments** for future spike work (baseline vs. experimental comparisons are a repeatable pattern). Document their purpose in `AGENTS.md` so future contributors know they're scaffolding for performance measurement, not production builds.

Everything above is bounded, mechanical, and test-covered. No design judgment required at cleanup time.

---

## Quality Gates (carried forward from spike)

| Gate                         | Threshold                                                                                            | Enforced in |
| ---------------------------- | ---------------------------------------------------------------------------------------------------- | ----------- |
| **Binary size**              | ≤ 92% of 1.5 MB OTA partition. Current baseline ~81%. LE-1 total budget: ~170 KB.                    | LE-1.9      |
| **Heap margin**              | Active mode + loaded layout retains ≥ **30 KB free heap**. ModeSwitch invariant, unchanged from Delight release. | LE-1.3, LE-1.9 |
| **Render p95**               | `CustomLayoutMode::render()` with full V1 widget set at realistic density (6–8 widgets) ≤ **1.2× `ClassicCardMode::render()`** p95 on an equivalent-complexity scene. | LE-1.9 |
| **Golden frames**            | Byte-equal rendered output for a canonical set of test layouts across builds.                        | LE-1.9      |
| **No fixed-mode regression** | ClassicCardMode, LiveFlightCardMode, ClockMode, DeparturesBoardMode p95 and heap unchanged (±2%).    | LE-1.9      |

Render p95 must be **re-measured in LE-1.9** using the same instrumentation pattern the spike introduced (ring buffer + minute-wall clock). The spike's 1.23× number is a signal of feasibility, not a V1 acceptance number — V1 has more widgets and a realistic layout density.

---

## Risks & Mitigations

| Risk                                                                                      | Mitigation                                                                                                                                                     |
| ----------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Demand signal not validated.** The evidence hunt (Mary) was inconclusive; Christian chose to proceed on technical feasibility alone. | If V1 adoption is poor, the **Mode SDK path** (Mary's alternative) reuses the same JSON schema + dispatch pattern. Sunk cost is recoverable — LayoutStore, WidgetRegistry, and the render dispatch survive a pivot to "SDK for contributors" framing. Documented as graceful fallback. |
| **Binary bloat.** Five widget types + editor JS bundle + seven new API endpoints could push past 92% partition. | Gate enforced in LE-1.9 before merge. **Fallback:** compile-flag disable unused widget types to produce a lean SKU build.                                      |
| **ES5 DnD on mobile.** iOS Safari touch event quirks broke early DnD-library prototypes. | Hand-rolled Pointer Events. `touch-action: none` on canvas. Tested on iOS Safari and Android Chrome in LE-1.6.                                                 |
| **Pre-existing TWDT crash on `loopTask` (~24s during fetcher operation).** | Observed during spike; **not caused by LE-1, not blocking**. Flag as a separate tech-debt item if it persists through LE-1.9 testing. Surface in retrospective. |
| **Interference with fixed modes.** New mode could regress ClassicCardMode, etc.          | Mitigated by design — `CustomLayoutMode` is a 5th mode and does not touch `LayoutEngine` or any fixed-mode code path. LE-1.9 golden frames cover the four fixed modes too. |
| **Schema drift across versions.**                                                         | `v: 1` field in layout JSON; LayoutStore rejects unknown versions on load and logs a clear error. V2 schema changes require a migrator.                        |

---

## Dependencies on Other Stories

- **TD-4 (commit-discipline gates)** — should land **before LE-1 work starts**. TD-4 introduces `branch:` and `zone:` frontmatter discipline; every LE-1 story file should include those fields from day one. This is the one hard ordering dependency.
- **TD-1 (atomic calibration flag)** and **TD-2 (hardware-config combined-change bug)** — P0 correctness fixes, **orthogonal** to LE-1. No hard dependency; can run in parallel.
- **TD-3 (OTA self-check native tests)** — orthogonal. Can land before or after LE-1.
- **Story 3-4 (Canvas Preview, done)** — LE-1.6 Editor page **supersedes** the read-only preview for custom layouts. The existing preview paths for Classic / LiveFlight / Clock / Departures are **unchanged**.
- **Story 4-3 (Logo Upload, done)** — LE-1.5 reuses the RGB565 logo pipeline and `LogoManager` unchanged. No forking, no duplication.

---

## Success Criteria (demand-neutral, behaviour-level)

Shipped V1 means all of the following are observably true:

1. At least one user-authored layout renders correctly on the device via `CustomLayoutMode`.
2. The dashboard editor supports **add / drag / resize / style / save** of a widget, end-to-end, in under **30 seconds on mobile**.
3. **Golden-frame tests pass** (LE-1.9) for the canonical layout set across builds.
4. **Binary stays under 92%** of the 1.5 MB OTA partition.
5. **No regression** in the four fixed modes' render budget (p95 within ±2%) or heap margin.
6. All nine LE-1 stories are complete, reviewed, and merged, with the V0 spike cleanup list fully reverted or formalized.

These are behaviour-level outcomes. None of them require a demand-signal threshold; they describe a working product independent of how many people adopt it.

---

## Future Work (V2+, out of scope)

- **Multiple layouts + time-based schedule selection.** Wire custom layouts into the existing `Schedule` system so an owner can run Layout A in the morning and Layout B at night. Schema and persistence already support multiple layouts; this is a scheduler integration, not a storage change.
- **Animation and transition widgets.** Scrolling text, fade-in panels, marquee tickers. Non-trivial because it forces a re-think of the "stateless render fn" pattern; likely its own epic.
- **Conditional widget visibility.** Predicates like "show only when ground speed > 100 kts" or "show only between 0600 and 2200". Requires a small expression language.
- **Community layout marketplace.** Import/export layouts; browse shared layouts from a registry. Depends on V1 adoption signal.
- **Mode SDK (Mary's alternate path).** Not necessarily built, but documented as a strategic possibility. If the personalization audience turns out to be small and the authorship audience is what we actually have, the same JSON schema + dispatch pattern becomes the foundation for a contributor-facing SDK. The V1 investment is not stranded in that pivot.

---


]]></file>
<file id="aebfc658" path="_bmad-output/implementation-artifacts/sprint-status.yaml" label="SPRINT STATUS"><![CDATA[

# Generated by bmad-assist on 2026-04-17T22:45:22
# Sprint Status File - TheFlightWall_OSS-main
#
generated: '2026-04-12T19:09:01'
last_updated: '2026-04-17T18:44:25'
project: TheFlightWall_OSS-main
project_key: NOKEY
tracking_system: file-system
story_location: _bmad-output/implementation-artifacts

development_status:
  # Epic 1
  epic-1: done
  1-1-project-infrastructure: done
  1-2-configmanager-and-systemstatus: done
  1-3-freertos-dual-core-task-architecture: done
  1-4-wifi-manager-and-network-connectivity: done
  1-5-web-portal-and-settings-api: done
  1-6-setup-wizard-wifi-and-api-keys: done
  1-7-setup-wizard-location-hardware-and-review: done
  1-8-led-progress-states-and-first-flight: done
  epic-1-retrospective: done
  # Epic 2
  epic-2: done
  2-1-dashboard-layout-display-settings-and-toast-system: done
  2-2-timing-controls-and-api-usage-estimate: done
  2-3-network-and-api-settings-with-reboot-flow: done
  2-4-system-health-page: done
  2-5-factory-reset-and-gpio-button: done
  epic-2-retrospective: done
  # Epic 3
  epic-3: done
  3-1-layout-engine-and-shared-zone-algorithm: done
  3-2-logo-manager-and-fallback-sprite: done
  3-3-zone-based-flight-rendering-with-logos-and-telemetry: done
  3-4-canvas-layout-preview-in-dashboard: done
  epic-3-retrospective: done
  # Epic 4
  epic-4: done
  4-1-interactive-map-with-circle-radius: done
  4-2-display-calibration-and-test-pattern: done
  4-3-logo-upload-with-rgb565-preview: done
  4-4-logo-list-management-and-deletion: done
  epic-4-retrospective: done
  # Epic fn-1
  epic-fn-1: done
  # Epic fn
  fn-1-1-partition-table-and-build-configuration: done
  fn-1-2-configmanager-expansion-schedule-keys-and-systemstatus: done
  fn-1-3-ota-upload-endpoint: done
  fn-1-4-ota-self-check-and-rollback-detection: done
  fn-1-5-settings-export-endpoint: done
  fn-1-6-dashboard-firmware-card-and-ota-upload-ui: done
  fn-1-7-settings-import-in-setup-wizard: done
  # Epic fn-1
  epic-fn-1-retrospective: done
  # Epic fn-2
  epic-fn-2: done
  # Epic fn
  fn-2-1-ntp-time-sync-and-timezone-configuration: done
  fn-2-2-night-mode-brightness-scheduler: done
  fn-2-3-dashboard-night-mode-card: done
  # Epic fn-2
  epic-fn-2-retrospective: done
  # Epic fn-3
  epic-fn-3: done
  # Epic fn
  fn-3-1-wizard-step-6-test-your-wall: done
  fn-3-2-wizard-timezone-auto-detect: done
  # Epic fn-3
  epic-fn-3-retrospective: done
  # Epic ds-1
  epic-ds-1: done
  # Epic ds
  ds-1-1-heap-baseline-measurement-and-core-abstractions: done
  ds-1-2-displayutils-extraction: done
  ds-1-3-moderegistry-and-static-registration-system: done
  ds-1-4-classic-card-mode-implementation: done
  ds-1-5-display-pipeline-integration: done
  # Epic ds-1
  epic-ds-1-retrospective: done
  # Epic ds-2
  epic-ds-2: done
  # Epic ds
  ds-2-1-live-flight-card-mode-enriched-telemetry-rendering: done
  ds-2-2-adaptive-field-dropping-and-vertical-direction-indicator: done
  # Epic ds-2
  epic-ds-2-retrospective: done
  # Epic ds-3
  epic-ds-3: done
  # Epic ds
  ds-3-1-display-mode-api-endpoints: done
  ds-3-2-nvs-mode-persistence-and-boot-restore: done
  ds-3-3-mode-picker-section-cards-and-schematic-previews: done
  ds-3-4-mode-switching-flow-and-transition-states: done
  ds-3-5-accessibility-and-responsive-layout: done
  ds-3-6-upgrade-notification-banner: done
  # Epic ds-3
  epic-ds-3-retrospective: done
  # Epic dl-1
  epic-dl-1: done
  # Epic dl
  dl-1-1-clock-mode-time-display: done
  dl-1-2-idle-fallback-to-clock-mode: done
  dl-1-3-manual-mode-switching-via-orchestrator: done
  dl-1-4-watchdog-recovery-to-default-mode: done
  dl-1-5-orchestrator-state-feedback-in-dashboard: done
  # Epic dl-1
  epic-dl-1-retrospective: done
  # Epic dl-2
  epic-dl-2: done
  # Epic dl
  dl-2-1-departures-board-multi-row-rendering: done
  dl-2-2-dynamic-row-add-and-remove-on-flight-changes: done
  # Epic dl-2
  epic-dl-2-retrospective: done
  # Epic dl-3
  epic-dl-3: done
  # Epic dl
  dl-3-1-fade-transition-between-display-modes: done
  # Epic dl-3
  epic-dl-3-retrospective: done
  # Epic dl-4
  epic-dl-4: done
  # Epic dl
  dl-4-1-schedule-rules-storage-and-orchestrator-integration: done
  dl-4-2-schedule-management-dashboard-ui: done
  # Epic dl-4
  epic-dl-4-retrospective: done
  # Epic dl-5
  epic-dl-5: done
  # Epic dl
  dl-5-1-per-mode-settings-panels-in-mode-picker: done
  # Epic dl-5
  epic-dl-5-retrospective: done
  # Epic dl-6
  epic-dl-6: done
  # Epic dl
  dl-6-1-ota-version-check-against-github-releases: done
  dl-6-2-update-notification-and-release-notes-in-dashboard: done
  # Epic dl-6
  epic-dl-6-retrospective: done
  # Epic dl-7
  epic-dl-7: done
  # Epic dl
  dl-7-1-ota-download-with-incremental-sha-256-verification: done
  dl-7-2-ota-failure-handling-rollback-and-retry: done
  dl-7-3-ota-pull-dashboard-ui-and-status-polling: done
  # Epic dl-7
  epic-dl-7-retrospective: done
  # Epic td
  epic-td: backlog
  td-1-atomic-calibration-flag: backlog
  td-2-hardware-config-combined-change-fix: backlog
  td-3-ota-self-check-native-tests: backlog
  td-4-commit-discipline-gates: backlog
  td-5-loop-task-twdt-during-fetcher: backlog
  # Epic le-1
  epic-le-1: in-progress
  # Epic le
  le-1-1-layout-store-littlefs-crud: done
  le-1-2-widget-registry-and-core-widgets: done
  le-1-3-custom-layout-mode-production: done
  le-1-4-layouts-rest-api: done
  le-1-5-logo-widget-rgb565-pipeline: done
  le-1-6-editor-canvas-and-drag-drop: done
  le-1-7-editor-property-panel-and-save-ux: done
  le-1-8-flight-field-and-metric-widgets: done
  le-1-9-golden-frame-tests-and-budget-gate: review


]]></file>
</context>
<variables>
<var name="architecture_file" file_id="893ad01d" description="System architecture for context" load_strategy="EMBEDDED" token_approx="59787">embedded in prompt, file id: 893ad01d</var>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">system-generated</var>
<var name="description">Run after epic completion to review overall success, extract lessons learned, and explore if new information emerged that might impact the next epic</var>
<var name="document_output_language">English</var>
<var name="document_project_file" description="Brownfield project documentation (optional)" load_strategy="INDEX_GUIDED">_bmad-output/planning-artifacts/*.md</var>
<var name="epic_num">le-1</var>
<var name="epics_file" file_id="8934f9af" description="The completed epic for retrospective" load_strategy="EMBEDDED" token_approx="5795">embedded in prompt, file id: 8934f9af</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="instructions">_bmad/bmm/workflows/4-implementation/retrospective/instructions.md</var>
<var name="name">retrospective</var>
<var name="next_epic_num"></var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="prd_file" file_id="8c5456f3" description="Product requirements for context" load_strategy="EMBEDDED" token_approx="3576">embedded in prompt, file id: 8c5456f3</var>
<var name="prev_epic_num"></var>
<var name="previous_retrospective_file" description="Previous epic's retrospective (optional)" load_strategy="SELECTIVE_LOAD">previous_retrospective (not found)</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="712">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="required_inputs">[{"agent_manifest": "{project-root}/_bmad/_config/agent-manifest.csv"}]</var>
<var name="retrospectives_folder">_bmad-output/implementation-artifacts</var>
<var name="sprint_status" file_id="aebfc658">embedded in prompt, file id: aebfc658</var>
<var name="sprint_status_file">_bmad-output/implementation-artifacts</var>
<var name="story_directory">_bmad-output/implementation-artifacts/stories</var>
<var name="template">False</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
</variables>
<instructions><![CDATA[

# Retrospective - Epic Completion Review Instructions

<critical>You MUST have already loaded and processed: {project-root}/_bmad/bmm/workflows/4-implementation/retrospective/workflow.yaml</critical>
<critical>Communicate all responses in English and language MUST be tailored to expert</critical>
<critical>Generate all documents in English</critical>
<critical>⚠️ ABSOLUTELY NO TIME ESTIMATES - NEVER mention hours, days, weeks, months, or ANY time-based predictions. AI has fundamentally changed development speed - what once took teams weeks/months can now be done by one person in hours. DO NOT give ANY time estimates whatsoever.</critical>
<critical>SCOPE LIMITATION: You are an AUTOMATED RETROSPECTIVE GENERATOR. Your ONLY output should be a structured retrospective report wrapped in extraction markers. Do NOT use party-mode dialogue. Do NOT wait for user input. Do NOT write files. Generate the complete retrospective in ONE response.</critical>

<critical>
  DOCUMENT OUTPUT: Retrospective analysis. Concise insights, lessons learned, action items. User skill level (expert) affects conversation style ONLY, not retrospective content.
</critical>

<workflow>

<critical>AUTOMATED MODE: This retrospective runs WITHOUT user interaction.
- Analyze all provided context (epic, stories, sprint-status, previous retro)
- Generate a SINGLE structured retrospective report
- Wrap output in extraction markers
- Do NOT use party-mode dialogue format
- Do NOT wait for user input
- Do NOT save files directly</critical>

<step n="1" goal="Epic Discovery - Find Completed Epic with Priority Logic">

<!-- managed programmatically by bmad-assist -->

<!-- managed programmatically by bmad-assist -->
<action>Read ALL development_status entries</action>
<action>Find the highest epic number with at least one story marked "done"</action>
<action>Extract epic number from keys like "epic-X-retrospective" or story keys like "X-Y-story-name"</action>
<action>Set {{detected_epic}} = highest epic number found with completed stories</action>

<check if="{{detected_epic}} found">
  <action>Set {{epic_number}} = {{detected_epic}}</action>
</check>
<action>Once {{epic_number}} is determined, verify epic completion status</action>

<!-- managed programmatically by bmad-assist -->

<action>Count total stories found for this epic</action>
<action>Count stories with status = "done"</action>
<action>Collect list of pending story keys (status != "done")</action>
<action>Determine if complete: true if all stories are done, false otherwise</action>

<check if="epic is not complete">
  <action>Set {{partial_retrospective}} = true</action>
  <action>Note incomplete stories for inclusion in retrospective report</action>
</check>

<check if="epic is complete">
  <action>Note full completion for inclusion in retrospective report</action>
</check>

</step>

<step n="0.5" goal="Discover and load project documents"><!-- input discovery handled by compiler --><note>After discovery, these content variables are available: {epics_content} (selective load for this epic), {architecture_content}, {prd_content}, {document_project_content}</note>
</step>

<step n="2" goal="Deep Story Analysis - Extract Lessons from Implementation">

<action>For each story in epic {{epic_number}}, read the complete story file from /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/{{epic_number}}-{{story_num}}-\*.md</action>

<action>Extract and analyze from each story:</action>

**Dev Notes and Struggles:**

- Look for sections like "## Dev Notes", "## Implementation Notes", "## Challenges", "## Development Log"
- Identify where developers struggled or made mistakes
- Note unexpected complexity or gotchas discovered
- Record technical decisions that didn't work out as planned
- Track where estimates were way off (too high or too low)

**Review Feedback Patterns:**

- Look for "## Review", "## Code Review", "## SM Review", "## Scrum Master Review" sections
- Identify recurring feedback themes across stories
- Note which types of issues came up repeatedly
- Track quality concerns or architectural misalignments
- Document praise or exemplary work called out in reviews

**Lessons Learned:**

- Look for "## Lessons Learned", "## Retrospective Notes", "## Takeaways" sections within stories
- Extract explicit lessons documented during development
- Identify "aha moments" or breakthroughs
- Note what would be done differently
- Track successful experiments or approaches

**Technical Debt Incurred:**

- Look for "## Technical Debt", "## TODO", "## Known Issues", "## Future Work" sections
- Document shortcuts taken and why
- Track debt items that affect next epic
- Note severity and priority of debt items

**Testing and Quality Insights:**

- Look for "## Testing", "## QA Notes", "## Test Results" sections
- Note testing challenges or surprises
- Track bug patterns or regression issues
- Document test coverage gaps

<action>Synthesize patterns across all stories:</action>

**Common Struggles:**

- Identify issues that appeared in 2+ stories (e.g., "3 out of 5 stories had API authentication issues")
- Note areas where team consistently struggled
- Track where complexity was underestimated

**Recurring Review Feedback:**

- Identify feedback themes (e.g., "Error handling was flagged in every review")
- Note quality patterns (positive and negative)
- Track areas where team improved over the course of epic

**Breakthrough Moments:**

- Document key discoveries (e.g., "Story 3 discovered the caching pattern we used for rest of epic")
- Note when team velocity improved dramatically
- Track innovative solutions worth repeating

**Velocity Patterns:**

- Calculate average completion time per story
- Note velocity trends (e.g., "First 2 stories took 3x longer than estimated")
- Identify which types of stories went faster/slower

**Team Collaboration Highlights:**

- Note moments of excellent collaboration mentioned in stories
- Track where pair programming or mob programming was effective
- Document effective problem-solving sessions

<action>Store this synthesis - these patterns will drive the retrospective report</action>

</step>

<step n="3" goal="Load and Integrate Previous Epic Retrospective">

<action>Calculate previous epic number:  = {{epic_number}} - 1</action>

<check if=" >= 1">
  <action>Search for previous retrospective using pattern: /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/epic--retro-*.md</action>

  <check if="previous retro found">
    <action>Read the complete previous retrospective file</action>

    <action>Extract key elements:</action>
    - **Action items committed**: What did the team agree to improve?
    - **Lessons learned**: What insights were captured?
    - **Process improvements**: What changes were agreed upon?
    - **Technical debt flagged**: What debt was documented?
    - **Team agreements**: What commitments were made?
    - **Preparation tasks**: What was needed for this epic?

    <action>Cross-reference with current epic execution:</action>

    **Action Item Follow-Through:**
    - For each action item from Epic  retro, check if it was completed
    - Look for evidence in current epic's story records
    - Mark each action item: ✅ Completed, ⏳ In Progress, ❌ Not Addressed

    **Lessons Applied:**
    - For each lesson from Epic , check if team applied it in Epic {{epic_number}}
    - Look for evidence in dev notes, review feedback, or outcomes
    - Document successes and missed opportunities

    **Process Improvements Effectiveness:**
    - For each process change agreed to in Epic , assess if it helped
    - Did the change improve velocity, quality, or team satisfaction?
    - Should we keep, modify, or abandon the change?

    **Technical Debt Status:**
    - For each debt item from Epic , check if it was addressed
    - Did unaddressed debt cause problems in Epic {{epic_number}}?
    - Did the debt grow or shrink?

    <action>Prepare "continuity insights" for inclusion in the retrospective report</action>

    <action>Identify wins where previous lessons were applied successfully:</action>
    - Document specific examples of applied learnings
    - Note positive impact on Epic {{epic_number}} outcomes
    - Note team growth and improvement

    <action>Identify missed opportunities where previous lessons were ignored:</action>
    - Document where team repeated previous mistakes
    - Note impact of not applying lessons (without blame)
    - Explore barriers that prevented application

  </check>

  <check if="no previous retro found">
    <action>Set {{first_retrospective}} = true</action>
  </check>
</check>

<check if=" &lt; 1">
  <action>Set {{first_retrospective}} = true</action>
</check>

</step>

<step n="4" goal="Preview Next Epic with Change Detection">

<action>Calculate next epic number:  = {{epic_number}} + 1</action>

<action>Attempt to load next epic using selective loading strategy:</action>

**Try sharded first (more specific):**
<action>Check if file exists: /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/planning-artifacts/epic\*/epic-.md</action>

<check if="sharded epic file found">
  <action>Load /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/planning-artifacts/*epic*/epic-.md</action>
  <action>Set {{next_epic_source}} = "sharded"</action>
</check>

**Fallback to whole document:**
<check if="sharded epic not found">
<action>Check if file exists: /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/planning-artifacts/epic\*.md</action>

  <check if="whole epic file found">
    <action>Load entire epics document</action>
    <action>Extract Epic  section</action>
    <action>Set {{next_epic_source}} = "whole"</action>
  </check>
</check>

<check if="next epic found">
  <action>Analyze next epic for:</action>
  - Epic title and objectives
  - Planned stories and complexity estimates
  - Dependencies on Epic {{epic_number}} work
  - New technical requirements or capabilities needed
  - Potential risks or unknowns
  - Business goals and success criteria

<action>Identify dependencies on completed work:</action>

- What components from Epic {{epic_number}} does Epic  rely on?
- Are all prerequisites complete and stable?
- Any incomplete work that creates blocking dependencies?

<action>Note potential gaps or preparation needed:</action>

- Technical setup required (infrastructure, tools, libraries)
- Knowledge gaps to fill (research, training, spikes)
- Refactoring needed before starting next epic
- Documentation or specifications to create

<action>Check for technical prerequisites:</action>

- APIs or integrations that must be ready
- Data migrations or schema changes needed
- Testing infrastructure requirements
- Deployment or environment setup

<action>Set {{next_epic_exists}} = true</action>
</check>

<check if="next epic NOT found">
  <action>Set {{next_epic_exists}} = false</action>
</check>

</step>

<step n="5" goal="Initialize Retrospective Context">

<action>Load agent configurations from {agent_manifest}</action>
<action>Identify which agents participated in Epic {{epic_number}} based on story records</action>
<action>Ensure key roles present: Product Owner, Scrum Master, Devs, Testing/QA, Architect</action>
<action>Compile epic summary metrics: completed stories, total stories, completion percentage, blocker count, debt count, coverage info, incident count, goals met, total goals</action>
<action>Store all context for consolidation into the retrospective document in step 11</action>

</step>

<step n="6" goal="Epic Review Analysis - What Went Well, What Didn't">

<action>Analyze successes from the deep story analysis (Step 2) and synthesize into themes</action>

<action>Identify and document success themes:</action>
- Areas of strong delivery and quality
- Process improvements that worked well
- Technical decisions that paid off
- Collaboration highlights
- Specific stories that exemplified best practices

<action>Analyze challenges and struggles from story analysis and synthesize into themes</action>

<action>Identify and document challenge themes:</action>
- Recurring blockers or friction points
- Systemic issues (not individual blame) that slowed delivery
- Complexity underestimation patterns
- Communication or coordination gaps
- Technical decisions that created problems

<action>For each challenge, identify the systemic root cause rather than individual attribution</action>

<action>Derive key insights that cut across successes and challenges:</action>
- Patterns that reveal process improvement opportunities
- Architectural or technical lessons
- Team dynamics insights
- Planning and estimation learnings

<check if="previous retrospective exists">
  <action>Cross-reference previous action items and assess follow-through for inclusion in report</action>
  <action>Note which previous lessons were applied and their impact</action>
  <action>Note which previous lessons were not applied and the consequence</action>
  <action>Synthesize continuity narrative for the retrospective document</action>
</check>

<action>Store all success themes, challenge themes, and insights for consolidation into step 11</action>

</step>

<step n="7" goal="Next Epic Preparation Analysis - Identify Readiness and Gaps">

<check if="{{next_epic_exists}} == false">
  <action>Note that no next epic is defined; skip preparation analysis and proceed to step 8</action>
</check>

<check if="{{next_epic_exists}} == true">
  <action>Analyze all preparation dimensions for Epic :</action>

  - Dependencies on Epic {{epic_number}} work: what is needed and whether it is complete and stable
  - Technical setup and infrastructure gaps
  - Knowledge gaps and research needs
  - Documentation or specification work required
  - Testing infrastructure requirements
  - Refactoring or debt reduction needs
  - External dependencies (APIs, integrations, etc.)

  <action>Categorize preparation items:</action>
  - CRITICAL: Must complete before epic starts
  - PARALLEL: Can happen during early stories without blocking
  - NICE-TO-HAVE: Would help but not blocking

  <action>For each preparation item, note:</action>
  - Description of work needed
  - Assigned owner (specific agent or role)
  - Criticality category
  - Rationale (what goes wrong if skipped)

  <action>Store preparation plan for consolidation into step 11</action>
</check>

</step>

<step n="8" goal="Synthesize Action Items with Significant Change Detection">

<action>Synthesize themes from Epic {{epic_number}} review analysis into actionable improvements</action>

<action>Create specific action items with:</action>

- Clear description of the action
- Assigned owner (specific agent or role)
- Category (process, technical, documentation, team, etc.)
- Success criteria (how we'll know it's done)

<action>Ensure action items are SMART:</action>

- Specific: Clear and unambiguous
- Measurable: Can verify completion
- Achievable: Realistic given constraints
- Relevant: Addresses real issues from retro
- Time-bound: Has clear deadline or dependency trigger

<action>Compile technical debt items with owner, priority, and rationale</action>
<action>Compile documentation needs with owner</action>
<action>Compile team agreements emerging from the analysis</action>

<action>CRITICAL ANALYSIS - Detect if discoveries require epic updates</action>

<action>Check if any of the following are true based on retrospective analysis:</action>

- Architectural assumptions from planning proven wrong during Epic {{epic_number}}
- Major scope changes or descoping occurred that affects next epic
- Technical approach needs fundamental change for Epic 
- Dependencies discovered that Epic  doesn't account for
- User needs significantly different than originally understood
- Performance/scalability concerns that affect Epic  design
- Security or compliance issues discovered that change approach
- Integration assumptions proven incorrect
- Team capacity or skill gaps more severe than planned
- Technical debt level unsustainable without intervention

<check if="significant discoveries detected">
  <action>Document each significant change with impact description</action>
  <action>Document wrong assumptions in current Epic  plan</action>
  <action>Document actual reality revealed by Epic {{epic_number}}</action>
  <action>List likely changes needed to Epic </action>
  <action>Set {{epic_update_needed}} = true</action>
  <action>Add epic review session to critical path</action>
</check>

<check if="no significant discoveries">
  <action>Set {{epic_update_needed}} = false</action>
</check>

<action>Store all action items, debt items, documentation needs, team agreements, and significant discovery findings for consolidation into step 11</action>

</step>

<step n="9" goal="Critical Readiness Assessment">

<action>Assess testing and quality state from story records:</action>
- What verification has been completed per story records and QA notes
- Any open testing concerns or gaps
- Test coverage status

<!-- managed programmatically by bmad-assist -->
- Whether deliverables are live in production, scheduled, or pending
- Any deployment dependencies that affect Epic  timing

<action>Assess stakeholder acceptance status from story records and epic notes:</action>
- Whether stakeholder sign-off is documented
- Any pending feedback or acceptance criteria not yet confirmed

<action>Assess technical health and codebase stability from story dev notes and debt analysis:</action>
- Overall stability signals from implementation records
- Areas of fragility or risk flagged by developers

<action>Assess unresolved blockers carried forward:</action>
- Any blockers from sprint-status not marked resolved
- Technical issues noted in stories as unresolved

<action>For each area with concerns, note what work is needed and add to critical path if applicable</action>

<action>Synthesize readiness assessment:</action>
- Testing &amp; Quality status
- Deployment status
- Stakeholder Acceptance status
- Technical Health status
- Unresolved Blockers status
- Overall readiness verdict

<action>Store readiness assessment for consolidation into step 11</action>

</step>

<step n="10" goal="Final Synthesis Before Output">

<action>Compile all analysis from steps 1-9 into a coherent set of sections ready for the retrospective document</action>
<action>Finalize key takeaways list (top 3-5 insights)</action>
<action>Finalize complete action item list with owners</action>
<action>Finalize preparation task list with owners and criticality</action>
<action>Finalize critical path item list</action>
<action>Finalize readiness assessment summary</action>
<action>Confirm {{epic_update_needed}} flag and significant discoveries summary</action>
<action>Count totals: action_count, prep_task_count, critical_count, insight_count, significant_discovery_count</action>

</step>

<step n="11" goal="Generate Retrospective Report - Single Consolidated Output">

<action>Generate the complete retrospective report as valid markdown, wrapped in extraction markers as shown below. This is the ONLY output produced by this workflow.</action>

<!-- RETROSPECTIVE_REPORT_START -->
# Epic {{epic_number}} Retrospective

**Epic Title:** {{epic_title}}
**Date:** system-generated
**Status:** {{#if partial_retrospective}}Partial Retrospective (epic not fully complete){{else}}Complete{{/if}}

---

## 1. Epic Summary

| Metric | Value |
|--------|-------|
| Completed Stories | {{completed_stories}} / {{total_stories}} ({{completion_percentage}}%) |
| Story Points Delivered | {{actual_points}}{{#if planned_points}} (planned: {{planned_points}}){{/if}} |
| Sprints | {{actual_sprints}}{{#if planned_sprints}} (planned: {{planned_sprints}}){{/if}} |
| Blockers Encountered | {{blocker_count}} |
| Technical Debt Items | {{debt_count}} |
| Test Coverage | {{coverage_info}} |
| Production Incidents | {{incident_count}} |
| Goals Achieved | {{goals_met}} / {{total_goals}} |

{{#if partial_retrospective}}
> ⚠️ **Partial Retrospective**: The following stories were not yet complete at the time of this retrospective:
{{pending_story_list}}
{{/if}}

---

## 2. Team Participants

{{list_participating_agents}}

---

## 3. Previous Epic Follow-Through

{{#if first_retrospective}}
This is the first retrospective for this project. No prior commitments to assess.
{{else}}
### Action Items from Epic  Retrospective

{{prev_action_items_follow_through_table}}

### Lessons Applied

{{prev_lessons_applied_summary}}

### Process Improvements Effectiveness

{{prev_process_improvements_effectiveness}}

### Technical Debt Carried Forward

{{prev_technical_debt_status}}

### Continuity Insights

{{continuity_insights_narrative}}
{{/if}}

---

## 4. What Went Well

{{success_themes_list}}

---

## 5. Challenges and Growth Areas

{{challenge_themes_list}}

> All challenges are attributed to systems and processes, not individuals.

---

## 6. Key Insights and Lessons Learned

{{key_insights_list}}

---

## 7. Story-Level Analysis Highlights

### Common Struggles (appearing in 2+ stories)

{{common_struggles_list}}

### Recurring Review Feedback

{{recurring_review_feedback_list}}

### Breakthrough Moments

{{breakthrough_moments_list}}

### Velocity Patterns

{{velocity_patterns_summary}}

### Technical Debt Incurred This Epic

{{technical_debt_incurred_list}}

---

## 8. Next Epic Preview

{{#if next_epic_exists}}
### Epic : {{next_epic_title}}

**Dependencies on Epic {{epic_number}}:**
{{list_dependencies}}

**Potential Risks and Unknowns:**
{{next_epic_risks_list}}

### Preparation Plan

**Critical Preparation (must complete before epic starts):**
{{list_critical_prep_items_with_owners}}

**Parallel Preparation (can happen during early stories):**
{{list_parallel_prep_items_with_owners}}

**Nice-to-Have Preparation (not blocking):**
{{list_nice_to_have_prep_items}}
{{else}}
Epic  is not yet defined. Preparation analysis will be performed when the next epic is planned.
{{/if}}

---

## 9. Significant Discovery Alert

{{#if epic_update_needed}}
> 🚨 **EPIC UPDATE REQUIRED** 🚨

The following significant discoveries from Epic {{epic_number}} require review and potential update of Epic  before it begins.

### Significant Changes Identified

{{significant_changes_list}}

### Incorrect Assumptions in Current Epic  Plan

{{wrong_assumptions_list}}

### Actual Reality Revealed by Epic {{epic_number}}

{{actual_reality_list}}

### Recommended Changes to Epic 

{{list_likely_changes_needed}}

### Recommended Actions

1. Review and update Epic  definition based on new learnings
2. Update affected stories in Epic  to reflect reality
3. Consider updating architecture or technical specifications if applicable
4. Hold alignment session before starting Epic 
{{#if prd_update_needed}}5. Update PRD sections affected by new understanding{{/if}}

> **Do NOT start Epic  until this review is complete.**
{{else}}
No significant discoveries were identified that require changes to Epic . The current plan remains sound.
{{/if}}

---

## 10. Critical Readiness Assessment

| Area | Status | Action Needed |
|------|--------|---------------|
| Testing &amp; Quality | {{quality_status}} | {{quality_action_needed}} |
| Deployment | {{deployment_status}} | {{deployment_action_needed}} |
| Stakeholder Acceptance | {{acceptance_status}} | {{acceptance_action_needed}} |
| Technical Health | {{stability_status}} | {{stability_action_needed}} |
| Unresolved Blockers | {{blocker_status}} | {{blocker_action_needed}} |

**Overall Readiness Verdict:** {{overall_readiness_verdict}}

---

## 11. Action Items

### Process Improvements

{{process_action_items_table}}

### Technical Debt

{{technical_debt_action_items_table}}

### Documentation

{{documentation_action_items_table}}

### Team Agreements

{{team_agreements_list}}

---

## 12. Critical Path

Items that must be resolved before Epic  can begin:

{{critical_path_items_table}}

---

## 13. Key Takeaways

1. {{key_lesson_1}}
2. {{key_lesson_2}}
3. {{key_lesson_3}}
{{#if key_lesson_4}}4. {{key_lesson_4}}{{/if}}
{{#if key_lesson_5}}5. {{key_lesson_5}}{{/if}}

---

## 14. Next Steps

1. Execute preparation tasks — complete all critical path items before Epic 
2. Review action items in next standup — ensure ownership is clear and progress is tracked
{{#if epic_update_needed}}3. **IMPORTANT:** Schedule Epic  planning review session to address significant discoveries before kickoff{{else}}3. Begin Epic  when all critical path items are confirmed complete{{/if}}
4. Use this retrospective as input to the next retrospective's follow-through analysis

---

**Totals:** {{action_count}} action items · {{prep_task_count}} preparation tasks · {{critical_count}} critical path items · {{insight_count}} key insights · {{significant_discovery_count}} significant discoveries

{{#if significant_discovery_count > 0}}
> ⚠️ **REMINDER:** Epic update required before starting Epic 
{{/if}}
<!-- RETROSPECTIVE_REPORT_END -->

</step>

</workflow>

]]></instructions>
<output-template></output-template>
</compiled-workflow>