---
docType: sprint-plan
scope: TD backlog + Epic LE-1 (Layout Editor V1 MVP)
owner: Bob (Scrum Master)
produced: 2026-04-17
inputs:
  - _bmad-output/planning-artifacts/epic-le-1-layout-editor.md
  - _bmad-output/planning-artifacts/layout-editor-v0-spike-report.md
  - _bmad-output/implementation-artifacts/stories/le-1-1-layout-store-littlefs-crud.md
  - _bmad-output/implementation-artifacts/stories/le-1-2-widget-registry-and-core-widgets.md
  - _bmad-output/implementation-artifacts/stories/le-1-3-custom-layout-mode-production.md
  - _bmad-output/implementation-artifacts/stories/le-1-4-layouts-rest-api.md
  - _bmad-output/implementation-artifacts/stories/le-1-5-logo-widget-rgb565-pipeline.md
  - _bmad-output/implementation-artifacts/stories/le-1-6-editor-canvas-and-drag-drop.md
  - _bmad-output/implementation-artifacts/stories/le-1-7-editor-property-panel-and-save-ux.md
  - _bmad-output/implementation-artifacts/stories/le-1-8-flight-field-and-metric-widgets.md
  - _bmad-output/implementation-artifacts/stories/le-1-9-golden-frame-tests-and-budget-gate.md
  - _bmad-output/implementation-artifacts/stories/td-1-atomic-calibration-flag.md
  - _bmad-output/implementation-artifacts/stories/td-2-hardware-config-combined-change-fix.md
  - _bmad-output/implementation-artifacts/stories/td-3-ota-self-check-native-tests.md
  - _bmad-output/implementation-artifacts/stories/td-4-commit-discipline-gates.md
  - _bmad-output/implementation-artifacts/stories/td-5-loop-task-twdt-during-fetcher.md
  - _bmad-output/implementation-artifacts/sprint-status.yaml
---

# Sprint Plan — TD backlog + Layout Editor V1 MVP integrated schedule

**Produced:** 2026-04-17
**Author:** Bob (Scrum Master)
**Scope:** 14 stories across 5 sprints — 5 tech-debt (TD-1…TD-5) + 9 LE-1 (Layout Editor V1 MVP).

> **Amendment 2026-04-17:** TD-5 (loopTask TWDT during fetcher) added after the initial plan was drafted. Recommended placement: **Sprint N+1 alongside TD-3** — orthogonal to LE-1, unblocks LE-1.9 measurement quality (the spike's ~24-s reboot cycles). Size S–M. Not re-threaded through the per-sprint breakdown below — treat as an N+1 addition. See `td-5-loop-task-twdt-during-fetcher.md`.

## Purpose

This plan sequences the full remaining backlog into an executable multi-sprint schedule. It resolves cross-cutting dependencies (notably the TD-4 → LE-1.1 ordering), documents exit criteria per sprint, and proposes the `sprint-status.yaml` update to register these 13 stories as active work. It is prescriptive, not exploratory — assignments are locked from prior party-mode rounds.

---

## 1. Sprint numbering

- The current `sprint-status.yaml` shows all prior epics (Epic 1 through DL-7) as `done`. There is no in-flight sprint block today.
- **Sprint N** = the next sprint to start after this plan is approved. For the purposes of this doc, N is the immediate next sprint window.
- **Sprint N+1 … N+4** = subsequent sprints. Five sprints total to close the combined backlog.
- "Size" t-shirts follow the LE-1 epic doc: **S ≈ 0.5 day, M ≈ 1 day, L ≈ 2 days, XS ≈ 0.25 day**.

---

## 2. Summary timeline

| Sprint | Story  | Title                                                         | Priority | Size | Owner (TBD) | Notes                                                                 |
| ------ | ------ | ------------------------------------------------------------- | -------- | ---- | ----------- | --------------------------------------------------------------------- |
| N      | TD-4   | Commit-discipline gates                                       | P1       | S–M  | TBD         | **Land first in sprint** — blocks LE-1 branching                      |
| N      | TD-1   | Atomic calibration flag                                       | P0       | XS   | TBD         | Pairs with TD-2 (display correctness)                                 |
| N      | TD-2   | Hardware-config combined-change fix                           | P0       | S    | TBD         | Pairs with TD-1                                                       |
| N+1    | TD-3   | OTA self-check native tests                                   | P1       | M    | TBD         | Orthogonal; do while OTA is quiet                                     |
| N+1    | LE-1.1 | LayoutStore: LittleFS CRUD + schema validation               | P0       | M    | TBD         | **Spike cleanup folded in as Task 1.** Dependency root.               |
| N+1    | LE-1.2 | WidgetRegistry + core 3 widgets (text/clock/logo)            | P0       | M    | TBD         | Parallel with LE-1.1 after Task 1 of 1.1 lands                        |
| N+1    | LE-1.3 | CustomLayoutMode promoted to production                      | P0       | M    | TBD         | Stretch goal if capacity permits                                      |
| N+2    | LE-1.3 | CustomLayoutMode production (if not pulled forward)          | P0       | M    | TBD         | Defaults here                                                         |
| N+2    | LE-1.4 | REST endpoints `/api/layouts/*` + activate                   | P0       | S    | TBD         | **Parallel with LE-1.3** (different dev)                              |
| N+2    | LE-1.5 | Logo widget + RGB565 pipeline reuse                          | P0       | M    | TBD         | Stretch — starts after 1.3 lands                                      |
| N+3    | LE-1.5 | Logo widget (if not pulled forward)                          | P0       | M    | TBD         | Defaults here                                                         |
| N+3    | LE-1.6 | Editor canvas + pointer DnD                                  | P0       | L    | TBD         | **Biggest story in the epic.** Owns most of the sprint.               |
| N+3    | LE-1.8 | flight_field + metric widgets                                | P0       | M    | TBD         | **Parallel with LE-1.6** (different dev)                              |
| N+4    | LE-1.7 | Editor property panel + save UX                              | P0       | M    | TBD         | Depends on LE-1.6                                                     |
| N+4    | LE-1.9 | Golden-frame tests + binary/heap budget gate                 | P0       | S    | TBD         | **Final gate.** Depends on all prior LE-1 stories.                    |

Workload per sprint (nominal dev-days, single dev): **N ≈ 1.75 d, N+1 ≈ 3.5 d, N+2 ≈ 2.5 d, N+3 ≈ 3 d, N+4 ≈ 1.5 d.** Two-dev parallelism compresses N+2 and N+3 materially.

---

## 3. Per-sprint breakdown

### Sprint N (current) — TD-1 + TD-2 + TD-4

**Theme:** Correctness + process foundation. Ship nothing user-visible; harden the rails before the 9-story LE-1 burst begins.

**Stories:**
- **TD-4** (`td-4-commit-discipline-gates.md`) — P1, S–M. **Must land first in the sprint.** Introduces `branch:` and `zone:` frontmatter enforcement + commit-msg hook. Every LE-1 story branch should be created *after* this is merged so the gate catches any deviation from day one.
- **TD-1** (`td-1-atomic-calibration-flag.md`) — P0, XS. Atomicity fix for the calibration flag. Correctness; no user-visible feature.
- **TD-2** (`td-2-hardware-config-combined-change-fix.md`) — P0, S. Combined-change bug in the hardware-config apply path. Pairs with TD-1 — both are display-subsystem correctness and share review context.

**Rationale:**
- TD-1 + TD-2 are classic pair-ship: same subsystem, same reviewer, low interaction cost.
- TD-4 is preventative overhead; deferring it costs nine LE-1 branches' worth of retroactive cleanup.
- Total sprint load is roughly 1 dev-week even at conservative estimates. Slack absorbs review churn.

**Exit criteria:**
- [ ] Commit-msg hook rejects commits without a valid `zone:` trailer on LE-1 / TD branches (manual smoke).
- [ ] Story frontmatter linter runs green against the existing story corpus.
- [ ] TD-1 and TD-2 merged; manual display smoke test on device confirms calibration flag + hardware-config combined change paths both behave.
- [ ] No regression in existing `pio test` suites.

---

### Sprint N+1 — TD-3 + LE-1.1 + LE-1.2 (+ LE-1.3 stretch)

**Theme:** Foundation. Lay the dependency root for all subsequent LE-1 work.

**Stories:**
- **TD-3** (`td-3-ota-self-check-native-tests.md`) — P1, M. Native-host test refactor for the OTA self-check logic. Orthogonal to LE-1; fine to run in parallel. Landing it here (rather than later) avoids collision with any future OTA feature work and keeps the test pyramid healthy while we add nine new stories.
- **LE-1.1** (`le-1-1-layout-store-littlefs-crud.md`) — M. LayoutStore with LittleFS CRUD, 64 KB / 16-file cap, schema validator, NVS `layout_active` key. **Task 1 is the V0 spike cleanup** (the seven-item revert list from the epic doc — instrumentation block, boot override, idle-fallback guard, status-bypass, `v0_spike_layout.h` deletion, `#ifdef LE_V0_SPIKE` resolution at LE-1.3 review, retention of `[env:esp32dev_le_baseline]` / `[env:esp32dev_le_spike]` as documented dev envs).
- **LE-1.2** (`le-1-2-widget-registry-and-core-widgets.md`) — M. WidgetRegistry dispatch table, `WidgetInstance` POD, render functions for `text`, `clock`, `logo` (logo still stub-bitmap here; real pipeline in LE-1.5).
- **LE-1.3 (stretch)** — pull forward only if LE-1.1 + LE-1.2 + TD-3 complete before the sprint midpoint.

**Rationale:**
- LE-1.1 is the single dependency root: it blocks LE-1.2, LE-1.3, and LE-1.4. Landing it first unlocks maximum parallelism downstream.
- LE-1.2 can start the moment LE-1.1's LayoutStore + schema types are merged; the two stories interlock cleanly.
- Folding spike cleanup into LE-1.1 Task 1 keeps the revert in the same PR as the new code it enables — easier review than a standalone cleanup story.
- TD-3 is a one-dev-day refactor with zero surface-area collision with LE-1; opportunistic placement here.

**Parallelism note:** Two-dev configuration — Dev A takes LE-1.1, Dev B takes TD-3 while waiting, then picks up LE-1.2 once 1.1 exposes types.

**Exit criteria:**
- [ ] `GET /api/layouts` returns an empty list on a fresh device (via unit test or manual curl).
- [ ] Schema validator rejects a layout with `v: 2` and unknown widget type.
- [ ] Text, clock, logo (stub) render via `WidgetRegistry` in unit-level test harness.
- [ ] All spike cleanup items from the epic doc's V0 Spike Cleanup section are reverted or formalized.
- [ ] No change in binary size beyond the LE-1.1/1.2 delta budget (~40 KB combined ceiling).

---

### Sprint N+2 — LE-1.3 + LE-1.4 (parallel) + LE-1.5 stretch

**Theme:** Mode is live. Surface is live.

**Stories:**
- **LE-1.3** (`le-1-3-custom-layout-mode-production.md`) — M. Promote `CustomLayoutMode` from spike to production. Remove `#ifdef LE_V0_SPIKE` gating, add permanent mode-table entry in `main.cpp`, implement worst-case memory reporting for the mode.
- **LE-1.4** (`le-1-4-layouts-rest-api.md`) — S. All seven `/api/layouts/*` + `/api/widgets/types` endpoints. Standard JSON envelope. Wires `activate` to `ModeRegistry::switchMode`.
- **LE-1.5 (stretch)** (`le-1-5-logo-widget-rgb565-pipeline.md`) — M. If LE-1.3 lands early and capacity exists, start the logo widget real-bitmap pipeline.

**Rationale:**
- LE-1.3 and LE-1.4 share no files: LE-1.3 touches `modes/` + `core/ModeRegistry` + `main.cpp`; LE-1.4 touches `adapters/WebPortal` + `core/LayoutStore`. **Clean two-dev parallelism** — both depend only on LE-1.1 + LE-1.2.
- LE-1.4 is sized S, deliberately: the endpoints are mechanical once LayoutStore exists.
- Keep LE-1.5 as a stretch-only pull to avoid blocking LE-1.6 in N+3 if LE-1.3 slips.

**Exit criteria:**
- [ ] CustomLayoutMode appears in `/api/display/modes` as the 5th mode.
- [ ] Switching to the custom mode with no active layout renders a clean empty canvas (no crash).
- [ ] `POST /api/layouts` creates a layout; `POST /api/layouts/:id/activate` persists `layout_active` in NVS.
- [ ] `GET /api/widgets/types` returns descriptors for `text`, `clock`, `logo` (2 remaining widget types stubbed as "TBD" entries until LE-1.8).
- [ ] Heap margin ≥ 30 KB with CustomLayoutMode active (partial check — full gate is in LE-1.9).
- [ ] No regression in the four fixed modes.

---

### Sprint N+3 — LE-1.5 + LE-1.6 + LE-1.8 (partial parallel)

**Theme:** Widgets round out, editor comes online. Biggest sprint in the plan.

**Stories:**
- **LE-1.5** (`le-1-5-logo-widget-rgb565-pipeline.md`) — M. Real RGB565 logo blit via `LogoManager`. Replaces the spike stub. Depends on LE-1.3.
- **LE-1.6** (`le-1-6-editor-canvas-and-drag-drop.md`) — **L**. The big one. Single `<canvas>` at 4× scale, hand-rolled Pointer Events, `touch-action: none`, selection, resize handles, snap-to-8px. Depends on LE-1.4.
- **LE-1.8** (`le-1-8-flight-field-and-metric-widgets.md`) — M. Two new widget types: `flight_field` (telemetry bindings) and `metric` (format-string support). Depends on LE-1.3.

**Rationale:**
- LE-1.6 is explicitly the tallest pole in the whole epic; it earns the most sprint capacity.
- LE-1.8 and LE-1.6 touch different zones (firmware widgets vs. web editor JS) — clean parallel work for a two-dev team.
- LE-1.5 is the smallest of the three and can be back-loaded if LE-1.6 proves harder than expected (Pointer Events on iOS Safari is the known risk factor).

**Parallelism note:** Dev A on LE-1.6 all sprint. Dev B on LE-1.5 → LE-1.8 (M + M fits inside a sprint comfortably).

**Exit criteria:**
- [ ] Real logo blits render in CustomLayoutMode; fillRect is **not** used (pattern requirement #1 from epic doc).
- [ ] On-device browser test: can drag a widget on iOS Safari and Android Chrome. Snap-to-8px fires. Resize handles work.
- [ ] `flight_field` renders live callsign/altitude/speed/heading/vert-rate from `StateVector`.
- [ ] `metric` renders `flight_count`, `uptime`, `heap` with format-string templating.
- [ ] `GET /api/widgets/types` now returns all five widget descriptors.
- [ ] Heap margin ≥ 30 KB with the full widget set loaded.

---

### Sprint N+4 — LE-1.7 + LE-1.9

**Theme:** UX polish + quality gate. Ship.

**Stories:**
- **LE-1.7** (`le-1-7-editor-property-panel-and-save-ux.md`) — M. Right-side property panel, left-side widget palette, `/api/widgets/types` introspection, save toast, save-under-a-name flow. Depends on LE-1.6.
- **LE-1.9** (`le-1-9-golden-frame-tests-and-budget-gate.md`) — S. Byte-equal frame comparison for canonical layouts; CI gate on binary % and heap margin; render p95 re-measurement. Depends on all prior LE-1 stories.

**Rationale:**
- LE-1.9 is deliberately last: it is the exit gate, not a feature story. It must run after every prior story has merged.
- LE-1.7 lands first in the sprint so LE-1.9 has the complete editor surface to golden-frame against.
- Sprint capacity is intentionally light (M + S) to leave room for review iterations and budget-gate remediation (e.g., shaving bytes if we cross 92%).

**Exit criteria (epic exit criteria):**
- [ ] At least one user-authored layout renders correctly on device (Epic Success Criterion #1).
- [ ] Mobile end-to-end: add → drag → resize → style → save in under 30s (Criterion #2).
- [ ] Golden-frame tests pass for the canonical layout set (Criterion #3).
- [ ] Binary ≤ 92% of 1.5 MB OTA partition (Criterion #4).
- [ ] Fixed-mode render p95 within ±2% of pre-LE-1 baseline (Criterion #5).
- [ ] Render p95 for `CustomLayoutMode` ≤ 1.2× `ClassicCardMode` at realistic (6–8 widget) density.
- [ ] All nine LE-1 stories merged; retrospective scheduled (`bmad-retrospective`).

---

## 4. Dependency graph

### Hard dependencies (blocks / blocked-by)

```
TD-4 ──▶ LE-1.1 (commit gates must exist before LE-1 branches are cut)
LE-1.1 ──▶ LE-1.2
LE-1.1 ──▶ LE-1.3
LE-1.1 ──▶ LE-1.4
LE-1.2 ──▶ LE-1.3
LE-1.2 ──▶ LE-1.5
LE-1.2 ──▶ LE-1.8
LE-1.3 ──▶ LE-1.5   (logo widget needs CustomLayoutMode live)
LE-1.3 ──▶ LE-1.8   (flight_field needs render context)
LE-1.4 ──▶ LE-1.6   (editor needs REST API)
LE-1.4 ──▶ LE-1.7
LE-1.6 ──▶ LE-1.7
LE-1.{1..8} ──▶ LE-1.9   (final gate, depends on everything)

TD-1, TD-2, TD-3 ─┬─ no hard deps on LE-1
                  └─ no hard deps on each other
```

### Non-blocking couplings

- TD-1 + TD-2 share review context (display subsystem) — pair them in the same PR review session.
- LE-1.3 + LE-1.4 are maximally parallel (touch different files, share only the LE-1.1/LE-1.2 foundation).
- LE-1.5 + LE-1.6 + LE-1.8 can proceed as an overlapping triad in N+3 with a two-dev team.

---

## 5. Quality gates per sprint

| Sprint | Gate                                                                                                                                                                   |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| N      | TD-4 commit-msg hook live; TD-1 + TD-2 device smoke passes; `pio test` green.                                                                                          |
| N+1    | LayoutStore CRUD under test; schema validator rejects bad input; spike cleanup list fully reverted or formalized; TD-3 native test suite passes.                       |
| N+2    | CustomLayoutMode is the 5th mode in `/api/display/modes`; empty canvas renders without crash; all seven `/api/layouts/*` endpoints return standard envelope.           |
| N+3    | Real logos blit (no fillRect); DnD works on iOS Safari + Android Chrome; all 5 widget types live via `/api/widgets/types`; heap margin ≥ 30 KB.                        |
| N+4    | **Epic exit gate (LE-1.9):** binary ≤ 92% partition, heap ≥ 30 KB, render p95 ≤ 1.2× ClassicCardMode baseline, golden frames byte-equal, fixed-mode p95 within ±2%.    |

If an exit gate fails at the end of a sprint, the failing story does not merge; the sprint either extends or the downstream sprint absorbs the slip. No silent carry-over.

---

## 6. Proposed `sprint-status.yaml` update

**Do not apply automatically.** This is a proposed addition to `_bmad-output/implementation-artifacts/sprint-status.yaml` for the user to review and apply. The current file ends at line 151 with all prior epics marked `done`; no keys are removed — 17 new keys are appended (4 TD + 9 LE-1 stories + LE-1 epic marker + 3 sprint-meta fields).

### Proposed diff (conceptual — append after the last `epic-dl-7-retrospective: done` line)

```yaml
# ───────────────────────────────────────────────────────────
# Active backlog — appended 2026-04-17 per sprint-plan-le-1-and-td.md
# ───────────────────────────────────────────────────────────

# Tech-debt backlog (4 stories)
td-1-atomic-calibration-flag: planned                    # Sprint N,  P0, XS
td-2-hardware-config-combined-change-fix: planned        # Sprint N,  P0, S
td-3-ota-self-check-native-tests: planned                # Sprint N+1, P1, M
td-4-commit-discipline-gates: planned                    # Sprint N,  P1, S-M  (land first)

# Epic LE-1: Layout Editor V1 MVP
epic-le-1: planned
le-1-1-layout-store-littlefs-crud: planned               # Sprint N+1, M  (includes V0 spike cleanup)
le-1-2-widget-registry-and-core-widgets: planned         # Sprint N+1, M
le-1-3-custom-layout-mode-production: planned            # Sprint N+2, M
le-1-4-layouts-rest-api: planned                         # Sprint N+2, S  (parallel with 1.3)
le-1-5-logo-widget-rgb565-pipeline: planned              # Sprint N+3, M
le-1-6-editor-canvas-and-drag-drop: planned              # Sprint N+3, L
le-1-7-editor-property-panel-and-save-ux: planned        # Sprint N+4, M
le-1-8-flight-field-and-metric-widgets: planned          # Sprint N+3, M  (parallel with 1.6)
le-1-9-golden-frame-tests-and-budget-gate: planned       # Sprint N+4, S  (final gate)
```

And update the top-of-file metadata:

```yaml
last_updated: '2026-04-17T00:00:00'
# (all other top-level fields unchanged)
```

### Notes on the proposal

- Status values use `planned` consistent with the existing convention; they transition through `in-progress` → `review` → `done` as stories move.
- No existing keys are modified. This is a strict append.
- No Sprint N block is added because the existing file does not use a sprint-grouping structure — per-story status is sufficient. If a sprint-grouping block is desired later, it can be retrofitted.

---

## 7. Risks to the plan

| Risk                                                                                                                    | Impact  | Mitigation                                                                                                                                               |
| ----------------------------------------------------------------------------------------------------------------------- | ------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **LE-1.6 slips.** The L-size editor DnD is the single biggest estimation risk. iOS Safari pointer quirks bit the spike prototype. | High    | Timebox to N+3. If not done by sprint end, push LE-1.7 to N+5 and keep LE-1.9 as the epic gate regardless — do not ship editor UI without its property panel. |
| **TWDT crash on `loopTask` (~24s) observed during V0 spike.** Pre-existing, not caused by LE-1.                         | Medium  | Out of scope for this plan. **Open TD-5 as a follow-up investigation.** Flag at each on-device testing milestone (N+2, N+3, N+4) and reassess severity.  |
| **Demand signal not validated.** Mary's Discord poll + support inbox evidence hunt was pending; decision to proceed was on technical feasibility alone. | Medium  | Checkpoint at end of N+2: if adoption evidence contradicts the plan, **cut scope** to LE-1.1 + LE-1.2 + LE-1.3 + LE-1.4 + LE-1.9 (API + renderer only, no editor UI). This is a clean graceful-fallback line. |
| **Binary-budget blowout.** 290 KB of partition headroom at current 81%, LE-1 budgeted ~170 KB (ceiling 92%).            | Medium  | `check_size.py` on every PR. **Hard rule: any single story adding > 40 KB stops the line.** Optimize before merging the next story.                      |
| **Spike cleanup collides with LE-1.1 Task 1.** The revert list touches `main.cpp` and `ModeOrchestrator`; parallel work on those files during N+1 could conflict. | Low     | Serialize: cleanup commits land first in the LE-1.1 PR, before the LayoutStore code. Dev B on LE-1.2 rebases after.                                      |
| **Two-dev parallelism unavailable.** Plan assumes two devs in N+2 and N+3 for the full parallel throughput.             | Low     | Single-dev fallback: extend N+2 and N+3 by ~1 week each. Epic lands in N+6 instead of N+4. No scope change, just schedule.                               |

---

## 8. Next actions (checklist)

- [ ] **Review this plan** (Christian + whoever owns dev capacity).
- [ ] **Apply the `sprint-status.yaml` diff** from §6 — strict append, no existing keys touched.
- [ ] (Optional) Run `bmad-validate-prd` against each LE-1 story file before dev pickup to catch any drafted-but-incomplete frontmatter.
- [ ] **Kick off TD-4 FIRST** in Sprint N — the commit-discipline gate must be in place before any LE-1 branch is cut.
- [ ] **Open TD-5** as a follow-up investigation for the TWDT crash observed during the V0 spike (do not add to Sprint N — just create the story file as `planned` so it's on the board).
- [ ] At end of Sprint N+2, **checkpoint the demand signal**: if Mary's evidence has landed and contradicts the plan, pivot to the scope-cut fallback in §7.
- [ ] At end of each sprint, run `bmad-sprint-status` for surfacing blockers into the retrospective pipeline.

---

## Appendix — Files referenced

| Path                                                                                                                        | Role                                          |
| --------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------- |
| `_bmad-output/planning-artifacts/epic-le-1-layout-editor.md`                                                                | Source of truth for LE-1 scope + dependencies |
| `_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md`                                                          | Validated patterns + cleanup list             |
| `_bmad-output/implementation-artifacts/stories/le-1-1-layout-store-littlefs-crud.md`                                        | LE-1.1 story file (draft)                     |
| `_bmad-output/implementation-artifacts/stories/le-1-2-widget-registry-and-core-widgets.md`                                  | LE-1.2 story file (draft)                     |
| `_bmad-output/implementation-artifacts/stories/le-1-3-custom-layout-mode-production.md`                                     | LE-1.3 story file (draft)                     |
| `_bmad-output/implementation-artifacts/stories/le-1-4-layouts-rest-api.md`                                                  | LE-1.4 story file (draft)                     |
| `_bmad-output/implementation-artifacts/stories/le-1-5-logo-widget-rgb565-pipeline.md`                                       | LE-1.5 story file (draft)                     |
| `_bmad-output/implementation-artifacts/stories/le-1-6-editor-canvas-and-drag-drop.md`                                       | LE-1.6 story file (draft)                     |
| `_bmad-output/implementation-artifacts/stories/le-1-7-editor-property-panel-and-save-ux.md`                                 | LE-1.7 story file (draft)                     |
| `_bmad-output/implementation-artifacts/stories/le-1-8-flight-field-and-metric-widgets.md`                                   | LE-1.8 story file (draft)                     |
| `_bmad-output/implementation-artifacts/stories/le-1-9-golden-frame-tests-and-budget-gate.md`                                | LE-1.9 story file (draft)                     |
| `_bmad-output/implementation-artifacts/stories/td-1-atomic-calibration-flag.md`                                             | TD-1 story file (draft)                       |
| `_bmad-output/implementation-artifacts/stories/td-2-hardware-config-combined-change-fix.md`                                 | TD-2 story file (draft)                       |
| `_bmad-output/implementation-artifacts/stories/td-3-ota-self-check-native-tests.md`                                         | TD-3 story file (draft)                       |
| `_bmad-output/implementation-artifacts/stories/td-4-commit-discipline-gates.md`                                             | TD-4 story file (draft)                       |
| `_bmad-output/implementation-artifacts/sprint-status.yaml`                                                                  | Tracking file — **proposed diff in §6, not yet applied** |
