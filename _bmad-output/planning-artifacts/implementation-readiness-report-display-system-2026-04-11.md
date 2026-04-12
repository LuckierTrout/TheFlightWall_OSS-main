---
stepsCompleted:
  - step-01-document-discovery
  - step-02-prd-analysis
  - step-03-epic-coverage-validation
  - step-04-ux-alignment
  - step-05-epic-quality-review
  - step-06-final-assessment
assessmentScope: display-system
date: '2026-04-11'
project_name: TheFlightWall_OSS-main
documentsUsed:
  prd: _bmad-output/planning-artifacts/prd-display-system.md
  epics: _bmad-output/planning-artifacts/epics-display-system.md
  ux: _bmad-output/planning-artifacts/ux-design-specification-display-system.md
  architecture: _bmad-output/planning-artifacts/architecture.md
assessorNote: Scoped to Display System track; monolithic architecture.md includes Display System extension (line ~1710+) and later Delight section—only Display-relevant portions were cross-checked for alignment.
---

# Implementation Readiness Assessment Report

**Date:** 2026-04-11  
**Project:** TheFlightWall_OSS-main  
**Scope:** Display System Release

---

## Document Discovery

### Assessment document set (Display System)

| Type | File |
|------|------|
| PRD | `prd-display-system.md` |
| Epics & stories | `epics-display-system.md` |
| UX | `ux-design-specification-display-system.md` |
| Architecture | `architecture.md` (includes Display System extension) |

### Other planning artifacts (not in scope for this pass)

Whole documents also exist for Foundation, Delight, and aggregate `prd.md` / `epics.md` / `ux-design-specification.md`. **No whole+sharded duplicate** pairs were found (no `*/index.md` shards under `planning-artifacts`).

### Issues from discovery

- **None** for duplicate formats. **Scope note:** `architecture.md` is shared across releases; implementers must anchor on the **Display System Release — Architecture Extension** section for this track.

---

## PRD Analysis

### Functional Requirements

| ID | Requirement (summary) |
|----|------------------------|
| FR1 | Interchangeable display modes: activation, drawing, teardown, reported heap before activation |
| FR2 | Report memory requirement before activation |
| FR3 | Rendering context: matrix, zone bounds, brightness |
| FR4 | Render within zone bounds only |
| FR5 | Registry of shipped modes |
| FR6 | Full lifecycle: teardown → heap validate → init → render |
| FR7 | Serialize concurrent switches; queue during in-progress transition |
| FR8 | Validate sufficient heap before activation |
| FR9 | Restore previous mode on heap failure |
| FR10 | Enumerate modes with names for API/UI |
| FR11 | Classic three-line card pixel-identical to recorded baseline |
| FR12 | Cycle flights at configurable interval (2+ flights) |
| FR13 | Classic Card meets same parity criteria as FR11 |
| FR14 | Enriched card: airline, route, altitude, speed, heading, V/S |
| FR15 | Adaptive field drop order (V/S first, then speed) |
| FR16 | Vertical direction distinct visual |
| FR17–FR26 | Mode Picker UX, notification, nav, errors, transition states |
| FR27–FR29 | NVS store/restore; default Classic after Foundation upgrade |
| FR30–FR33 | HTTP read/list, activate, transition state, error reason |
| FR34–FR36 | All output via active mode; single pipeline commit; always valid active mode |

**Total FRs:** 36

### Non-Functional Requirements

| ID | Theme | Requirement (summary) |
|----|--------|------------------------|
| NFR P1–P4 | Performance | Switch &lt;2s perceived; 16ms frame budget; picker &lt;1s; enumeration &lt;10ms, no heap alloc |
| NFR S1–S6 | Stability / memory | 100 switches heap stable; rapid switch safe; stack ≤512B in render; 30KB floor after init; brightness read-only; NVS key safe |
| NFR C1–C4 | Compatibility | Classic parity; no Foundation regression; cooperative scheduling; picker matches dashboard tech/style |

**Total NFRs:** 14 (labeled P1–P4, S1–S6, C1–C4)

### Additional requirements / constraints (from PRD body)

- IoT: cooperative main loop, no new render tasks; `NeoMatrixDisplay` owns `show()`; pipeline unchanged; security model unchanged (LAN trust).
- Critical path called out: J6 migration → interface → registry → Classic → Live → Picker → NVS; heap guard parallelizable.
- Explicit deferrals: mode-specific settings, animated transitions, scheduling, server-rendered thumbnails.

### PRD completeness (initial)

Strong: numbered FR/NFR, journeys J1–J6, measurable outcomes table, risks and mitigations. **Watch item:** FR32 asks the **API** to distinguish “switch initiated” vs “switch complete”; UX spec instead uses **synchronous POST** and client-side “Switching…” while the request is in flight—see Epic Coverage and UX sections.

---

## Epic Coverage Validation

### Epic FR coverage (from `epics-display-system.md` FR Coverage Map)

All **FR1–FR36** are mapped to Epic 1, 2, or 3 with explicit lines in the FR Coverage Map.

### Coverage matrix (summary)

| Status | Count |
|--------|-------|
| Covered in epics | 36 / 36 |
| Missing | 0 |

**Coverage percentage:** 100% (by epic map)

### NFR and architecture/UX inventory in epics

- NFRs P1–P4, S1–S6, C1–C4 appear in epics’ NFR Coverage Map with epic assignments.
- Additional architecture rows (AR1–AR8) and UX-DR1–UX-DR12 are inventoried and mapped.

### Missing FR coverage

**None.**

### Traceability gaps (planning level, not missing FR rows)

1. **FR32 vs planned API shape** — PRD requires API transition state **initiated vs complete**. Story 3.1 specifies a **blocking POST** that returns `active` or `failed`. UX aligns with “no polling,” implying “initiated” is **client-side** during the POST. **Recommendation:** Either amend PRD FR32 to “API reports completed switch or failure; UI may show in-flight state while awaiting response,” or add a non-polling API pattern that still satisfies literal FR32 (document the chosen interpretation in release API docs).

2. **FR7 wording vs Story 1.3** — PRD says **queuing** during transition; Story 1.3 uses **queue-of-one, latest wins** (discard older pending). Functionally serializes switches; **wording** differs from strict FIFO queue. **Recommendation:** Align PRD FR7 or story text so “serialized with at-most-one pending (latest wins)” is explicit.

---

## UX Alignment Assessment

### UX document status

**Found:** `ux-design-specification-display-system.md` — aligned with PRD journeys, Mode Picker cadence, synchronous switch flow, schematic previews, heap error copy, upgrade banner behavior.

### UX ↔ PRD

- **Aligned:** Remote-control interaction, &lt;2s transition, dual confirmation (wall + phone), one-time notification, Classic parity as invisible success.
- **Tension:** FR32 (API transition states) vs UX “synchronous POST, no polling” — UX resolves “initiated” as UI state; PRD still names API. Resolve before calling API “done.”

### UX ↔ Architecture (Display System section)

- **Aligned:** DisplayMode, ModeRegistry, client-side CSS Grid wireframes, dual-source dismissal **concept**, ConfigManager NVS, WebPortal endpoints pattern.
- **Misaligned (critical):** Concrete names diverge:

| Item | UX spec | Architecture | Epics |
|------|---------|--------------|-------|
| localStorage key | `mode_notification_dismissed` (and narrative variants) | `flightwall_mode_notif_seen` | `mode_notif_seen` |
| Dismiss firmware flag | `POST /api/display/ack-notification` | `POST /api/display/ack-notification` | `POST /api/display/notification/dismiss` |

**Impact:** Three different localStorage keys and two different dismiss endpoint paths appear across artifacts. Implementation will guess wrong without a single source of truth.

### Warnings

- Normalize **one** localStorage key and **one** dismiss endpoint; update UX, architecture, epics, and PRD API wording together.
- After normalization, regenerate any API contract docs you maintain under `docs/`.

---

## Epic Quality Review

### Checklist (summary)

| Epic | User value | Independence | Notes |
|------|------------|--------------|-------|
| Epic 1 | Yes (device renders via modes; Classic parity) | Epic 2 & 3 depend on Epic 1 only — OK | Title is architecture-heavy; body states user verifiable outcome |
| Epic 2 | Yes (richer telemetry view) | Depends on Epic 1 — OK | Clear |
| Epic 3 | Yes (picker, persistence, API) | Depends on Epic 1 — OK; parallel with Epic 2 after Epic 1 — OK | Matches dependency note |

### Critical violations

- **None** for “epic is only a technical milestone with no user outcome.”

### Major issues

1. **Cross-artifact contract drift** (notification dismiss path + localStorage) — see UX section; this is the highest-risk defect for implementation.
2. **Persona on early Epic 1 stories** — Stories 1.1–1.3 use “As a firmware developer.” Strict BMAD user-story practice prefers a user or system-owner persona even for foundational work, or explicit “enabler story” labeling. **Acceptable for brownfield firmware** if the team explicitly allows developer-facing stories.

### Minor concerns

- FR32 / synchronous API clarification (see above).
- FR7 “queue” vs “latest wins” one-slot pending (see above).
- Epic inventory shortens FR11 text vs PRD; coverage map still references FR11 — low risk if acceptance criteria keep full parity language (they do via NFR C1 / AC).

### Acceptance criteria quality

- Stories widely use **Given/When/Then**; error paths appear for API and heap failure; **good testability** overall.

### Dependency analysis

- Within Epic 1, ordering 1.1→1.5 is linear and coherent; no forward reference to “Story 1.6” or Epic 3 inside Epic 1 beyond declared interfaces.
- Starter template / greenfield DB rules **N/A** (embedded brownfield).

---

## Summary and Recommendations

### Overall readiness status

**NEEDS WORK** — Planning content is rich and FR coverage is complete, but **notification/API/localStorage contracts disagree across UX, architecture, and epics**, and **FR32** needs an explicit interpretation relative to the synchronous POST design.

### Critical issues requiring immediate action

1. **Single canonical contract** for upgrade notification: choose `localStorage` key name, dismiss endpoint path (`ack-notification` vs `notification/dismiss`), and document in PRD “release API documentation” bullets + all three artifacts.
2. **Resolve FR32:** Document whether “initiated” is exclusively client-side during POST, or extend API (without violating “no polling” if you add optional status resource).

### Recommended next steps

1. Edit `ux-design-specification-display-system.md`, `architecture.md` (Display System subsection), and `epics-display-system.md` Story 3.1 / 3.6 so **endpoint paths and storage keys match exactly**.
2. Add one paragraph to **PRD** (Mode Switch API or NFR) locking the FR32 interpretation to the synchronous UX pattern, **or** adjust FR32 text to match.
3. Align **FR7** wording with Story 1.3’s “at-most-one pending, latest wins” semantics.
4. After edits, run PRD validation again if you use `prd-display-system-validation-report.md` as a gate.

### Final note

This assessment found **contract misalignment** (high severity) and **two traceability clarifications** (FR32, FR7) across otherwise strong PRD, UX, architecture, and epic materials. Fix the cross-doc API/storage drift **before** Phase 4 implementation to avoid rework in `WebPortal`, `dashboard.js`, and NVS handling.

---

**Implementation Readiness Assessment Complete**

Report path: `_bmad-output/planning-artifacts/implementation-readiness-report-display-system-2026-04-11.md`

For BMAD next steps, invoke the **bmad-help** skill if you want workflow routing after this gate.
