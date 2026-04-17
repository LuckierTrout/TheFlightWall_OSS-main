# PR Review Checklist

Use this checklist when reviewing a pull request against TheFlightWall_OSS.
The order is deliberate — structural concerns first, then correctness, then
style. Reject on any **required** item that fails; comment on advisory items.

## 1. Scope (required)

- [ ] **Diff is confined to the declared story zone.** Open the story file
      referenced in the commit subject (e.g. `td-1:` → `stories/td-1-*.md`).
      Read its `zone:` frontmatter list. Every file touched by this PR must
      match one of those globs. If the diff crosses story boundaries, reject
      and ask the author to split the PR. *(Rationale: prevents the
      fn-1.6 / dl-1.5 mixed-diff incident from 2026-04-13 recurring.
      Enforced by convention + this checklist; commit-msg hook catches the
      simpler case of missing prefix.)*

- [ ] **Commit subjects all carry the story-ID prefix.** Verify `git log`
      on the branch shows every commit starts with `td-N:` / `le-X.Y:` /
      `fn-X.Y:` / `dl-X.Y:` / `ds-X.Y:`. The local hook should have caught
      this; CI does not yet, so reviewer is the final gate.

- [ ] **Branch name matches the story's `branch:` frontmatter.**

## 2. Correctness (required)

- [ ] **Each Acceptance Criterion in the story has a corresponding code
      or test artifact.** Walk the story's ACs top-to-bottom; for each one,
      confirm there is a matching change or justification.

- [ ] **New and existing tests pass.** Verify the PR build output. For
      firmware changes, `~/.platformio/penv/bin/pio run` must complete
      without warnings that are new since `main`.

- [ ] **Binary size and heap budget respected** (firmware changes only).
      `check_size.py` passes. If the story includes a numeric render-time,
      heap, or binary delta gate, the PR includes a measurement.

## 3. Safety (required for firmware)

- [ ] **No blocking work on Core 0** (display task). Cross-core shared
      state uses `std::atomic` or FreeRTOS queues, never raw `volatile`.

- [ ] **No new `String` allocations on per-frame hot paths.** Prefer
      fixed `char[]` buffers + `snprintf`.

- [ ] **ArduinoJson v7 conventions followed** (no deprecated
      `StaticJsonDocument`/`DynamicJsonDocument`; filter docs used for
      large payloads).

- [ ] **Web JS is ES5-only** (no arrow fns, `let`/`const`, template
      literals). `FW.get/post/del` used for API calls, `FW.showToast` for
      notifications.

## 4. Documentation (advisory)

- [ ] Story's `File List` matches what the PR actually touched.
- [ ] CLAUDE.md / AGENTS.md updated if the PR changes the build,
      test, or operational workflow.
- [ ] No orphan TODO comments left behind.

## 5. Process (advisory)

- [ ] Story `Status:` updated to `review` or `ready-for-review` (internal
      file) and to `in-progress` in `sprint-status.yaml` while work is
      active; bumped to `done` in `sprint-status.yaml` on merge.
- [ ] Retrospective or sprint note recorded if the story uncovered a
      cross-cutting issue.

---

*This checklist lives at `_bmad-output/implementation-artifacts/review-checklist.md`
and is referenced by AGENTS.md. Updates should go through the normal story
process — add a new item via a TD story if a recurring review issue warrants
a permanent gate.*
