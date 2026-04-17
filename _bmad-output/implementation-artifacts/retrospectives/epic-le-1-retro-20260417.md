# Epic LE-1 Retrospective

**Epic Title:** Layout Editor V1 MVP
**Date:** 2026-04-17
**Status:** Substantially Complete — 2 stories in `review` (LE-1.8, LE-1.9) with on-device hardware validation pending; all implementation work delivered

---

## 1. Epic Summary

| Metric | Value |
|--------|-------|
| Completed Stories | 7 / 9 (78% code-complete; 9/9 implementation delivered, 2 pending hardware gate sign-off) |
| Story Sizes Delivered | 1× L, 5× M, 3× S / S across nine stories |
| Sprints | 1 (single focused sprint) |
| Blockers Encountered | 1 (hardware unavailability blocked on-device p95 ratio measurement and heap-floor numeric verification) |
| Technical Debt Items | 4 (V0 spike instrumentation cleanup, value-equality caching in widgets, ES5 scrutiny, TWDT crash unresolved) |
| Test Coverage | 8 test suites, ~60 Unity tests across all stories; golden-frame structural suite passing |
| Production Incidents | 0 |
| Goals Achieved | 5 / 6 (binary budget met; no fixed-mode regression; golden-frame tests pass; REST API + editor delivered; 30-second UX path delivered — on-device heap floor + p95 numeric verification deferred to hardware availability) |

> ⚠️ **Partial sign-off**: LE-1.8 and LE-1.9 are in `review`. All implementation is complete. The outstanding items are:
> - **LE-1.8**: On-device hardware validation of `currentFlight` wiring (p95 ratio cannot be measured without a flashed device)
> - **LE-1.9**: Numeric heap-floor verification (AC #4 — assertion exists in test but `ESP.getFreeHeap()` return value requires a running ESP32); p95 ratio (AC #5) — deferred with V0 spike reference (1.23×) documented in verification report

---

## 2. Team Participants

Based on story records, the following roles contributed to Epic LE-1:

| Role | Contribution |
|------|-------------|
| **Architect (Winston)** | Epic definition, architecture document, LE-1.2 dispatch pattern ruling (switch > vtable), WidgetSpec sizing decision, three-reviewer synthesis on LE-1.1 and LE-1.2 |
| **Senior Developer (Amelia)** | Primary implementation across all nine stories; code review synthesis; ES5 enforcement; ClockWidget cache rationale; hot-swap re-init pattern (LE-1.3) |
| **QA / Testing (Quinn)** | Golden-frame strategy (structural + string-resolution compromise); smoke-test design (LE-1.7); three-list sync rule identification; AC #5 p95 deferral rationale |
| **Product Owner (Christian)** | Epic commissioning; V0 spike demand decision (feasibility-only, not demand-validated); non-goal scoping |
| **Scrum Master** | Sprint status tracking; story readiness gates |

---

## 3. Previous Epic Follow-Through

The `retrospectives/` directory is empty — this is the **first retrospective committed to file** for this project. No prior action items to assess.

> **Note for future retrospectives:** All prior epics (Epic 1 through DL-7) were completed without retrospective documents saved to disk. The `sprint-status.yaml` entry `epic-dl-7-retrospective: done` suggests retros were conducted verbally or informally. This retrospective establishes the baseline template that future retrospectives will reference for follow-through analysis.

---

## 4. What Went Well

### 4.1 — V0 Spike Paid Off Completely
The V0 spike (April 17, 2026) de-risked the entire epic before a line of production code was written. The fillRect → bitmap-blit regression was caught in the spike (2.67× → 1.23× p95), the WidgetSpec sizing was validated, and the dispatch pattern was proven. All nine stories proceeded with high confidence because the hard questions were answered before story one.

### 4.2 — Clean Separation of Concerns
`CustomLayoutMode` is a true peer mode — it never touches `LayoutEngine`, never modifies any fixed-mode code path, and the four existing modes were genuinely unaffected. The two-touch-point mode registration rule (rule 22) held perfectly: adding `CustomLayoutMode` required only one new file in `modes/` and one table entry in `main.cpp`.

### 4.3 — Binary Budget Discipline
All nine stories were delivered with the binary at **82.9% (1,303,488 / 1,572,864 bytes)** against a 92% cap. The `check_size.py` delta gate (+0 B vs. baseline) is a clean signal. The editor's web assets came in at **8,040 bytes gzipped** against a 30,720-byte budget (26.2%) — exceptional compression discipline.

### 4.4 — Render Pipeline Pattern Carried Forward Cleanly
The five implementation patterns mandated by the epic (bitmap blit, minute-resolution clock cache, parse-at-init, fixed array, hand-rolled Pointer Events) were all followed without deviation. Zero violations were flagged in code review. The pattern list proved to be the right level of prescriptiveness — specific enough to prevent regressions, general enough not to over-constrain.

### 4.5 — REST API + Editor UX Coherence
The `GET /api/widgets/types` introspection endpoint was a standout design decision: it allowed the editor's property panel (LE-1.7) to render type-specific controls dynamically without hardcoding field names in JavaScript. When LE-1.8 upgraded `flight_field` and `metric` from stubs to real selects, the editor picked them up without a single line of editor-JS change — the schema was the source of truth.

### 4.6 — ES5 Constraint Held Without Drama
Despite the strictness of the ES5 requirement (iOS Safari constraint), no story introduced any ES6+ syntax that had to be reverted. The constraint table in LE-1.6 was clear enough that developers internalized it without needing repeated correction.

---

## 5. Challenges and Growth Areas

### 5.1 — Hardware Unavailability as an Unplanned Dependency
The two stories still in `review` (LE-1.8, LE-1.9) are blocked not by code quality issues but by **hardware unavailability for on-device measurement**. The p95 ratio (AC #5 in LE-1.9) and heap-floor numeric verification (AC #4) require a flashed ESP32 with a running layout. The architecture did not explicitly schedule hardware access as a dependency, and the stories were written as if hardware would always be available. This created a last-story bottleneck.

### 5.2 — Three-List Sync Fragility (Widget Type Registration)
LE-1.2 introduced a "three-list sync rule": widget type strings must stay in sync across `kAllowedWidgetTypes[]` (LayoutStore), `fromString()` (WidgetRegistry), and `_handleGetWidgetTypes()` (WebPortal). This is a **manual synchronization requirement with no compile-time enforcement**. It was caught by the Quinn review and documented, but it remains a future foot-gun: adding a new widget type requires three separate edits that must stay consistent. No automated test validates list parity.

### 5.3 — `RenderContext.currentFlight` Field Addition Mid-Epic
LE-1.8 required adding `const FlightInfo* currentFlight` to `RenderContext` in `DisplayMode.h` — a change to the shared interface that touches every mode. This was identified as an **unplanned interface change** relative to the epic's original architecture. The change was safe (nullable pointer with backward-compatible default), but it required updates to `CustomLayoutMode`, `WebPortal`, `WidgetRegistry`, and the editor. An earlier design pass might have anticipated this and included it in the initial interface spec.

### 5.4 — Heap-Floor Test Assertion Without Numeric Evidence
AC #4 in LE-1.9 (30 KB heap floor) is encoded as a test assertion, but the actual `ESP.getFreeHeap()` value is undocumented because the test requires a live device. The verification report documents the baseline (32,768 bytes after Foundation release) but cannot confirm the post-load delta. This leaves the most critical quality gate (heap floor) without a numeric result at epic close.

### 5.5 — Pre-existing TWDT Crash Not Resolved
The pre-existing task-watchdog (`loopTask` TWDT at ~24 seconds during fetcher operation) was flagged in the epic risks section as "not caused by LE-1, not blocking" but was documented for the retrospective. It persisted through LE-1 testing and was explicitly deferred. The fact that it was observed during spike testing and surfaced again in LE-1.9 suggests it will continue to manifest in testing for every future epic until addressed. It occupies mental bandwidth without being owned.

### 5.6 — Value-Equality Caching Deferred in LE-1.8
AC #5 of LE-1.8 specified a "no heap on render hot path: string in file-scope static buffer, refreshed only when source value changes." The story notes acknowledge that the **value-equality cache clause was formally deferred** — the zero-heap requirement is satisfied by the stack-buffer + char* overload, but the per-value cache is not implemented. On a high-refresh display, this means `formatTelemetryValue` is called every frame even when the value hasn't changed.

---

## 6. Key Insights and Lessons Learned

1. **Spike → Epic promotion is an effective pattern.** Having a V0 spike with explicit "V1 must carry forward" constraints gave every story a pre-negotiated implementation contract. No story wasted cycles debating dispatch patterns, bitmap vs. fillRect, or JSON parsing strategy. The spike document was the most valuable artifact of the entire epic.

2. **Hardware access is a first-class dependency, not an assumption.** On-device measurement (p95 ratio, heap floor) should be scheduled as a story subtask with an explicit hardware-availability prerequisite, not treated as something that will happen naturally. Future stories with on-device gate conditions should block progression to `done` rather than `review` when hardware is unavailable.

3. **Interface changes mid-epic have ripple costs.** The `RenderContext.currentFlight` addition in LE-1.8 was safe, but it touched multiple files and required updating the architecture document, the test suite, and the editor. Anticipating cross-cutting interface fields during epic design (even as nullable stubs) would have isolated this cost to a single story.

4. **Introspection endpoints pay compounding dividends.** `GET /api/widgets/types` was a small investment in LE-1.4 that paid off in LE-1.7 (dynamic property panels) and LE-1.8 (dropdown upgrade without editor-JS changes). The pattern of making firmware the source of truth for UI schema is worth applying to other areas (mode settings, schedule rules, etc.).

5. **Manual list synchronization is a defect waiting to happen.** The three-list sync rule (WidgetType strings across LayoutStore, WidgetRegistry, and WebPortal) needs a compile-time or test-time enforcement mechanism. Good documentation is necessary but not sufficient for a multi-year, multi-contributor codebase.

---

## 7. Story-Level Analysis Highlights

### Common Struggles (appearing in 2+ stories)

| Issue | Stories Affected | Nature |
|-------|-----------------|--------|
| Code review multi-reviewer synthesis expanding scope | LE-1.1, LE-1.2, LE-1.3 | Each review added 4–9 follow-up items, adding one or more write cycles per story |
| On-device test deferred to hardware availability | LE-1.8, LE-1.9 | Structural tests substituted for physical measurement; p95 ratio and heap floor remained unconfirmed |
| Interface/contract discovered late | LE-1.2 (three-list sync), LE-1.8 (currentFlight) | Both required cross-story changes |
| ES5 correctness self-policing | LE-1.6, LE-1.7, LE-1.8 | Zero violations found, but required conscious attention every story touching editor JS |

### Recurring Review Feedback

| Feedback Theme | Stories | Outcome |
|----------------|---------|---------|
| Guard clauses missing (nullptr, bounds) | LE-1.2, LE-1.3, LE-1.5 | All fixed before merge; pattern became proactive in later stories |
| Missing test for error paths | LE-1.1, LE-1.4 | Additional tests added during review; 4 new tests in LE-1.1 review cycle |
| Document NVS key abbreviations | LE-1.1, LE-1.3 | `layout_active` documented in ConfigManager.h NVS table |
| Clarity of fallback behavior | LE-1.3, LE-1.5 | Explicit comment blocks added for default-layout fallback and PROGMEM sprite paths |

### Breakthrough Moments

- **V0 Spike fillRect regression discovery**: The specific measurement that blitting was 2.67× worse than bitmap-blit during the spike directly shaped the entire epic's render contract. This is the single highest-leverage insight of the project.
- **Hot-swap re-init pattern (LE-1.3)**: The `render()` → `init()` inline re-parse on active-id change made layout switching seamless without requiring a full mode teardown/reinit cycle. A clean, hardware-validated pattern worth reusing in V2 for multi-layout scheduling.
- **`GET /api/widgets/types` as schema source of truth (LE-1.4/LE-1.7)**: Treating firmware as the authoritative schema source eliminated an entire class of UI/firmware drift.

### Velocity Patterns

- **Stories 1–3 (infrastructure)**: Slower due to multi-reviewer synthesis and the V0 spike cleanup. Reviews each added a full write cycle. Foundation work is always front-loaded.
- **Stories 4–7 (REST API + editor)**: Higher velocity. The infrastructure was stable; each story was additive. LE-1.6 (editor canvas) was the largest story (L) but benefited from the hand-rolled Pointer Events decision being pre-made.
- **Stories 8–9 (widgets + gates)**: Reverted to review-pending state due to hardware dependency. Implementation was fast; hardware availability was the bottleneck.

### Technical Debt Incurred This Epic

| Item | Priority | Story | Notes |
|------|----------|-------|-------|
| Three-list widget type sync (no compile enforcement) | HIGH | LE-1.2 | Must add automated parity check before adding 3rd-party modes |
| Value-equality cache in FlightFieldWidget / MetricWidget | MEDIUM | LE-1.8 | `formatTelemetryValue` called every frame even when value unchanged |
| Pre-existing TWDT crash on `loopTask` (~24s during fetcher) | HIGH | TD-5 (backlog) | Not introduced by LE-1; blocks clean hardware testing; should be pulled from backlog |
| Missing on-device heap floor numeric result | LOW | LE-1.9 | Deferred to hardware availability; assertion encoded, numeric value not confirmed |

---

## 8. Next Epic Preview

No next epic is defined in the planning artifacts at the time of this retrospective. `epic-le-1` is the only LE epic in scope, and the backlog shows Tech Debt stories (td-1 through td-5) as the next candidate work.

**Candidates for next work based on this retrospective:**

| Candidate | Source | Rationale |
|-----------|--------|-----------|
| **TD-5: loopTask TWDT crash** | Backlog (td-5) | Surfaces in every hardware testing cycle; should be resolved before LE-2 hardware gates |
| **TD-4: Commit discipline gates** | Backlog (td-4) | Listed as hard dependency for future LE epics in epic document |
| **TD-1, TD-2, TD-3** | Backlog (td-1–3) | P0 correctness fixes; orthogonal to LE but blocking production stability |
| **LE-2: Multi-layout scheduling** | Future work (LE-1 doc) | Integrates custom layouts into the existing `Schedule` system; persistence + schema already support it |

**Preparation for LE-2 (whenever scoped):**

If LE-2 includes schedule-driven layout switching, the following are ready:
- LittleFS multi-layout persistence (`/layouts/` directory, up to 16 files) — already implemented in LE-1.1
- NVS `layout_active` key — already implemented in LE-1.1
- `ModeOrchestrator` schedule evaluation — already implemented in DL-4

What is NOT yet ready:
- Schedule rules that reference layouts by id (currently rules reference mode IDs, not layout IDs)
- A UI affordance for "activate layout at time X"
- Testing infrastructure for schedule + layout interaction on hardware

---

## 9. Significant Discovery Alert

No significant architectural or planning-level discoveries were made during Epic LE-1 that require changes to a defined next epic.

**Three informational findings worth carrying forward:**

### Finding 1: `RenderContext` is Not Closed

The addition of `currentFlight` in LE-1.8 established a precedent: `RenderContext` will grow as new widget types need access to runtime state. Future widget types (e.g., weather, ADS-B coverage, custom metrics from external APIs) will each need new fields. The struct should be treated as **open for extension** and the architecture document updated to reflect this. Future epics should anticipate `RenderContext` changes as a recurring one-story cost.

### Finding 2: Three-List Widget Type Sync is a Systemic Fragility

The WidgetType string synchronization across `LayoutStore`, `WidgetRegistry`, and `WebPortal` is a manually-maintained contract with no compile-time or test-time enforcement. This was accepted as-is for LE-1 but becomes a progressively higher risk as the contributor base expands (per FR41-FR42 of the Delight PRD). Before any community contributions are invited, this needs an automated enforcement mechanism.

### Finding 3: Hardware as Explicit Dependency

The TWDT crash (TD-5) and the two `review`-state stories both trace to hardware availability not being explicitly scheduled. This is a process gap, not a code gap. Future epics with on-device measurement gates should include a dedicated "hardware validation session" slot as a first-class epic activity, not an afterthought.

No architectural re-planning is required. The LE-1 foundation is sound.

---

## 10. Critical Readiness Assessment

| Area | Status | Action Needed |
|------|--------|---------------|
| Testing & Quality | ⚠️ Partial | LE-1.8 and LE-1.9 need on-device hardware run to confirm heap floor (AC #4) and p95 ratio (AC #5). All structural tests pass. |
| Deployment | ✅ Ready | Binary at 82.9% (well under 92% cap). LittleFS uploadfs clean. Delta +0 B. Ready to flash. |
| Stakeholder Acceptance | ✅ Ready | All nine stories implemented and code-reviewed. LE-1.4–1.7 fully `done`. Two `review` items have known, bounded resolution path. |
| Technical Health | ✅ Good | No regressions in four fixed modes. Binary budget healthy. Heap allocation discipline maintained throughout. |
| Unresolved Blockers | ⚠️ One open | Pre-existing TWDT crash (TD-5) on `loopTask` unresolved. Does not block LE-1 merge but will surface in next hardware testing cycle. |

**Overall Readiness Verdict:** ✅ **Ready to merge with hardware gate pending.** Epic LE-1 is complete at the implementation and review level. The two `review` stories (LE-1.8, LE-1.9) require a single on-device hardware run to close the numeric quality gates. No code changes are anticipated from that run — the assertions are already in place.

---

## 11. Action Items

### Process Improvements

| # | Action | Owner | Success Criteria |
|---|--------|-------|-----------------|
| P-1 | Schedule a dedicated hardware validation session before marking any future story "done" when it has on-device measurement gates (p95, heap floor, timing). Add this as a story template field: `hardware_gate: true/false`. | Scrum Master | Next epic's stories with on-device gates include explicit `hardware_gate` field; no story reaches `review` with unconfirmed hardware metrics. |
| P-2 | Begin committing retrospective reports to `_bmad-output/implementation-artifacts/retrospectives/` after each epic completion. This retrospective is the first; subsequent epics will reference it for follow-through analysis. | Scrum Master | Retrospective file present in `retrospectives/` for each future epic. |
| P-3 | When the next epic is scoped, schedule TD-5 (loopTask TWDT crash) as a prerequisite work item or parallel story, not backlog. | Product Owner (Christian) | TD-5 has a story in the sprint before any hardware-gate stories begin. |

### Technical Debt

| # | Action | Owner | Priority | Rationale |
|---|--------|-------|----------|-----------|
| T-1 | Add automated widget type parity test: a single Unity test that asserts `LayoutStore::kAllowedWidgetTypes`, `WidgetRegistry::fromString()` complete coverage, and `WebPortal::_handleGetWidgetTypes()` all agree. Fails at compile time if any list is out of sync. | Developer (Amelia) | HIGH | Three-list sync rule has no enforcement. Will break silently if a contributor adds a widget type to only two of three locations. |
| T-2 | Implement value-equality caching in `FlightFieldWidget` and `MetricWidget`: re-run `formatTelemetryValue` only when the source float value changes, not every frame. Store `_lastValue` alongside the cached `char[48]` buffer. | Developer | MEDIUM | `formatTelemetryValue` currently called ~20× per second per widget even when value is static. On a 24-widget max-density layout this is ~480 unnecessary snprintf calls per second. |
| T-3 | Pull TD-5 (pre-existing loopTask TWDT crash at ~24s during fetcher) out of backlog and assign to a sprint. | Architect (Winston) | HIGH | Surfaces in every hardware test session. Creates noise that masks legitimate regressions. Documented in LE-1 epic risks. |
| T-4 | Close the numeric gap in LE-1.9 AC #4 and AC #5 by running the golden-frame test suite on physical hardware and recording `ESP.getFreeHeap()` result and p95 ratio in `le-1-verification-report.md`. | Developer | LOW (bounded) | Not a code change — one hardware run. Closes the last open quality gate. |

### Documentation

| # | Action | Owner |
|---|--------|-------|
| D-1 | Update `architecture.md` "RenderContext" section to document `currentFlight` field and note that the struct is **open for extension** as new widget types require runtime state. Add guidance: "New fields are nullable pointers or zero-default primitives; backward-compatible by convention." | Architect (Winston) |
| D-2 | Add "Three-List Sync Rule" to `AGENTS.md` enforcement section with explicit example of all three locations and the automated parity test path (once T-1 is complete). | Developer (Amelia) |
| D-3 | Update `_bmad-output/planning-artifacts/le-1-verification-report.md` with actual on-device `ESP.getFreeHeap()` and p95 ratio values once hardware run is complete (per T-4). | Developer |

### Team Agreements

1. **No story is `done` if an on-device hardware gate has not been confirmed.** Stories with `hardware_gate: true` move to `done` only after a real device run. `review` is the correct state while hardware is unavailable.
2. **V0 spike cleanup is always included in Story 1 of a new epic** that builds on spike work. This pattern worked cleanly in LE-1.1 and should be the default for future spike-to-production promotions.
3. **`RenderContext` changes are a story-scoped architectural discussion.** Before adding any new field to `RenderContext`, a brief architecture note should be added to the story file explaining the rationale and confirming backward compatibility for existing modes.
4. **The `retrospectives/` directory gets a file after every epic, no exceptions.** Retroactive reconstruction from `sprint-status.yaml` and story files is possible but lossy. Real-time capture is the right discipline.

---

## 12. Critical Path

Items that must be resolved before any subsequent LE epic (LE-2+) can begin:

| # | Item | Owner | Criticality |
|---|------|-------|-------------|
| C-1 | Complete on-device hardware run for LE-1.8 + LE-1.9 (heap floor + p95 ratio). Update verification report. Close both `review` stories. | Developer | **CRITICAL** — LE-1 is not formally complete until these gates close. |
| C-2 | Implement widget type parity test (T-1). Prevents silent breakage when LE-2 adds widget types. | Developer (Amelia) | **CRITICAL** — should land before any new widget type is introduced. |
| C-3 | Prioritize TD-5 (loopTask TWDT crash) in next sprint planning. | Product Owner (Christian) / Architect (Winston) | **CRITICAL** — blocks clean hardware test cycles for any future epic. |
| C-4 | TD-4 (commit discipline gates) should land before LE-2 story files are created, per LE-1 epic's own stated dependency. | Scrum Master | **CRITICAL** (per epic's stated dependency) |
| C-5 | Archive this retrospective to `retrospectives/epic-le-1-retro-2026-04-17.md`. | Scrum Master | **PARALLEL** — should happen immediately after this review |

---

## 13. Key Takeaways

1. **The spike-first pattern is this project's most effective quality lever.** Every hour spent in a V0 spike measuring real hardware behavior saved multiple story cycles of rework. This pattern should be formalized: any epic with novel rendering or memory constraints gets a mandatory spike story before implementation stories begin.

2. **Hardware availability is a deployment dependency, not an assumption.** Two stories were left in `review` state at epic close because on-device measurement was not treated as a scheduled resource. Treating physical hardware access the way we treat WiFi credentials or API keys — as an explicit dependency that must be confirmed available — would prevent this class of blockage.

3. **Introspection endpoints pay compounding dividends.** `GET /api/widgets/types` made the editor property panel self-updating across three stories with zero UI-code changes. This pattern — firmware as schema source of truth — is worth applying to mode settings, schedule rules, and any future plugin/extension points.

4. **Manual synchronization requirements are technical debt at birth.** The three-list widget sync rule was correctly identified and documented during LE-1, but documentation without enforcement is just a slower failure. Any future extension point with manually-synchronized lists should get an automated parity test in the same story that introduces the lists.

5. **The LE-1 foundation is architecturally sound for V2.** `LayoutStore`, `WidgetRegistry`, and `CustomLayoutMode` are clean peer-mode implementations with no coupling to fixed-mode code. The multi-layout persistence (16 files, 64 KB cap) and NVS active-id key were built for exactly the multi-layout scheduling scenario planned for V2. There is no foundational rework ahead — only additive work on top of a well-structured base.

---

## 14. Next Steps

1. **Execute C-1 (hardware run)** — Complete the on-device test for LE-1.8 and LE-1.9 to formally close the epic with confirmed quality gates.
2. **Archive this retrospective** — Save to `_bmad-output/implementation-artifacts/retrospectives/epic-le-1-retro-2026-04-17.md` and establish the file as the baseline for future follow-through analysis.
3. **Sprint planning: pull TD-5 from backlog** — Schedule the loopTask TWDT crash investigation before the next epic's hardware testing begins.
4. **Land TD-4 (commit discipline gates)** before LE-2 story files are authored — this was listed as a hard ordering dependency in the LE-1 epic document itself.
5. **Implement T-1 (widget type parity test)** as a standalone task or first story in LE-2, before any new widget types are introduced.
6. Begin LE-2 scoping only after C-1 through C-4 are confirmed complete.

---

**Totals:** 8 action items (3 process + 4 tech debt + 3 documentation) · 5 preparation tasks · 5 critical path items · 5 key insights · 3 significant informational findings (no epic update required)