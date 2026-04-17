
# Story td-4: Per-Story Commit Discipline with Enforceable Gates

branch: td-4-commit-discipline-gates
zone:
  - .githooks/**
  - scripts/install-hooks.sh
  - _bmad-output/implementation-artifacts/stories/**
  - _bmad-output/implementation-artifacts/review-checklist.md
  - AGENTS.md

Status: draft

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a **reviewer** (primary) and **contributor** (secondary),
I want **commits to be enforced to belong to a single declared story and stay within that story's declared path zone**,
So that **PR diffs are scoped, reviewable, and cross-story bleed (like the fn-1.6 / dl-1.5 mixed-diff incident on 2026-04-13) cannot recur silently**.

## Acceptance Criteria

1. **Given** a developer starts a new story **When** they copy the story template at `_bmad-output/implementation-artifacts/stories/_template.md` **Then** the template declares `branch:` (string, required) and `zone:` (list of path globs, required) as mandatory frontmatter fields at the top of the story file **And** the template is created if absent **And** it follows the same layout shape used by existing in-flight stories (e.g., `fn-1-4-ota-self-check-and-rollback-detection.md`, `td-1-atomic-calibration-flag.md`).

2. **Given** the set of stories in `_bmad-output/implementation-artifacts/stories/` whose `Status:` is NOT `done` (case-insensitive) **When** backfill is complete **Then** every such story contains `branch:` and `zone:` frontmatter at the top, matching its actual work area **And** the enumerated backfill list (story IDs + count) is captured in Dev Notes of the implementation PR. Completed stories (`Status: done`) are out of scope.

3. **Given** a commit message that does NOT match the regex `^(td-[0-9]+|fn-[0-9]+\.[0-9]+|dl-[0-9]+\.[0-9]+|ds-[0-9]+\.[0-9]+):[[:space:]].+` (case-insensitive) **When** the `commit-msg` hook at `.githooks/commit-msg` runs **Then** the commit is rejected with exit code 1 **And** the rejection message shows the valid story-ID formats **And** the rejection message references AGENTS.md for rationale **And** the rejection message echoes back the first line of the user's attempted commit message.

4. **Given** a merge commit (message starts with `Merge ` or `Revert `, OR the second arg to the hook is `merge`/`squash`) **When** the `commit-msg` hook runs **Then** the regex check is skipped **And** the commit is allowed through.

5. **Given** an empty commit message (zero non-whitespace characters) **When** the `commit-msg` hook runs **Then** the commit is rejected with a distinct error message that does NOT conflate empty-message rejection with story-ID-format rejection.

6. **Given** a fresh clone of the repository **When** the developer runs `bash scripts/install-hooks.sh` once **Then** the script runs `git config core.hooksPath .githooks` **And** prints a success line confirming the value of `core.hooksPath` (read back via `git config --get core.hooksPath`) **And** running the script a second time succeeds with no error and no duplicate configuration (idempotent).

7. **Given** a reviewer opens a PR for review **When** they open `_bmad-output/implementation-artifacts/review-checklist.md` **Then** the first checklist item reads: "Diff confined to declared story zone (see story frontmatter `zone:` field)? If not, reject PR." **And** the file structure leaves room for future checklist items to be appended.

8. **Given** a new contributor or agent reads AGENTS.md **When** they look for commit / hook guidance **Then** they find a section documenting (a) that commit discipline is enforced via a git hook, (b) the single install command `bash scripts/install-hooks.sh`, (c) the commit-message format requirement with examples, (d) the zone-confinement review rule that the hook cannot enforce.

9. **Given** the hook is installed **When** the developer runs the three self-tests **Then** `git commit -m "wip: foo"` is REJECTED with the story-ID-format error **And** `git commit -m "td-1: atomic flag"` is ACCEPTED **And** `git commit -m "TD-1: atomic flag"` is ACCEPTED (case-insensitive). Self-test terminal output is pasted into the Dev Agent Record of the implementation PR.

10. **Given** the implementation is complete **When** CLAUDE.md and README.md are re-read **Then** any existing story-workflow references remain accurate **And** no previously-correct section has been silently invalidated by this change (cross-link check).

## Tasks / Subtasks

- [ ] **Task 1: Author `.githooks/commit-msg` hook script** (AC: #3, #4, #5)
  - [ ] Create `.githooks/commit-msg` with shebang `#!/usr/bin/env bash` and `chmod +x`.
  - [ ] Read first line of the commit-message file (arg `$1`) into `subject`.
  - [ ] Short-circuit-allow if subject starts with `Merge ` or `Revert `, or if `$2` is `merge`/`squash`.
  - [ ] Short-circuit-reject with distinct error if `subject` is empty / whitespace-only.
  - [ ] Match subject against regex `^(td-[0-9]+|fn-[0-9]+\.[0-9]+|dl-[0-9]+\.[0-9]+|ds-[0-9]+\.[0-9]+):[[:space:]].+` case-insensitively (use `shopt -s nocasematch` or lowercase-transform).
  - [ ] On mismatch: emit the exact rejection message block shown in Dev Notes, echo the received subject, and `exit 1`.
  - [ ] On match: `exit 0`.
  - [ ] Rely only on bash builtins + POSIX tools; must work on macOS default bash 3.2 and Linux bash 4+.

- [ ] **Task 2: Author `scripts/install-hooks.sh`** (AC: #6)
  - [ ] Create `scripts/install-hooks.sh` with shebang `#!/usr/bin/env bash` and `chmod +x`.
  - [ ] Run `git config core.hooksPath .githooks` inside the repo root.
  - [ ] Read back `git config --get core.hooksPath` and fail with a clear error if it does not equal `.githooks`.
  - [ ] Print a success line, e.g. `[install-hooks] core.hooksPath = .githooks`.
  - [ ] Verify idempotency: running twice in a row produces the same output and exit code 0.

- [ ] **Task 3: Create or update the story template** (AC: #1)
  - [ ] If `_bmad-output/implementation-artifacts/stories/_template.md` already exists, update it to add `branch:` and `zone:` frontmatter.
  - [ ] If it does not exist, create it using `fn-1-4-ota-self-check-and-rollback-detection.md` as the structural reference (`# Story`, `Status:`, `## Story`, `## Acceptance Criteria`, `## Tasks / Subtasks`, `## Dev Notes`, `### File List`).
  - [ ] Mark `branch:` and `zone:` as REQUIRED in inline comments inside the template.

- [ ] **Task 4: Backfill `branch:` and `zone:` frontmatter on in-flight stories** (AC: #2)
  - [ ] Enumerate stories in `_bmad-output/implementation-artifacts/stories/` with `Status:` != `done` (case-insensitive match; `complete`, `review`, `ready-for-review`, `ready-for-dev`, `in-progress`, `blocked-hardware`, `draft`, `Ready for Review` all count as in-flight).
  - [ ] Expected in-flight set at drafting time (from repo snapshot on 2026-04-17): dl-7-1, ds-2-2, dl-4-1, fn-1-2, dl-1-4, ds-3-4, fn-1-1, dl-7-2, ds-2-1, ds-3-1, fn-2-2, ds-3-5, dl-1-5, ds-1-3, dl-3-1, ds-1-5, dl-7-3, fn-1-3, ds-1-4, fn-3-2, dl-4-2, dl-5-1, ds-3-2, td-1, ds-3-3, ds-1-2, dl-1-1, fn-3-1, ds-1-1, fn-1-7, fn-1-6 — approx. 31 stories. Re-enumerate at implementation time; final list goes in Dev Agent Record.
  - [ ] For each in-flight story: insert `branch: <kebab-case>` and `zone:` list matching the actual work area immediately after the `# Story` heading, matching the format used by this story and by td-1 where applicable.
  - [ ] Do NOT modify stories with `Status: done` (or any case-variant thereof). Sunk cost; out of scope.

- [ ] **Task 5: Create reviewer checklist file** (AC: #7)
  - [ ] Create `_bmad-output/implementation-artifacts/review-checklist.md`.
  - [ ] First item (verbatim): "Diff confined to declared story zone (see story frontmatter `zone:` field)? If not, reject PR."
  - [ ] Leave headings/structure such that future items can be appended without restructuring.

- [ ] **Task 6: Update AGENTS.md with hook + commit-format section** (AC: #8)
  - [ ] Add a section (e.g., `## Commit Discipline`) that documents the four required bullets: (a) hook enforcement, (b) `bash scripts/install-hooks.sh` install, (c) commit-message format with examples, (d) zone-confinement review rule.
  - [ ] Keep prose terse; no marketing copy.

- [ ] **Task 7: Run the three self-tests and capture output** (AC: #9)
  - [ ] With the hook installed, run the three self-test `git commit` invocations listed in Dev Notes.
  - [ ] Paste terminal output into the Dev Agent Record / Completion Notes of the implementation PR (not this draft).

- [ ] **Task 8: Cross-link check** (AC: #10)
  - [ ] Re-read CLAUDE.md and README.md; verify no existing workflow reference has been invalidated by the new hook or checklist file.
  - [ ] If a reference is now stale, fix it in the same PR.

## Dev Notes

### Rationale — three-layer enforcement, not redundant

Each layer catches what the others miss:

1. **Template** (`_template.md` with `branch:` + `zone:` frontmatter) — a declarative contract. Without it, no hook can know what "in zone" means for a given story. Prevents the "we never wrote it down" failure mode.
2. **Hook** (`.githooks/commit-msg`) — a reflexive gate at commit time. Prevents the "I forgot which story I'm on" failure mode. Zero friction, runs in milliseconds, no network.
3. **Reviewer checklist** (`review-checklist.md` item 1) — the human backstop for semantic zone confinement that the hook cannot evaluate without parsing every story's `zone:` field against the actual diff. Also catches force-pushed branches where the hook never ran.

Collapsing any one of the three leaves a gap.

### Triggering incident

On 2026-04-13, a single commit mixed fn-1.6 (dashboard firmware card) changes with dl-1.5 (story-5) changes. Reviewer surfaced the issue in `deferred-work.md`. This story's gates are designed to make that specific class of mix-up impossible-by-default for the hook-caught cases, and cheap-to-catch-in-review for the rest.

### Commit-msg rejection message (exact template)

The hook must emit this exact block on format rejection (AC #3):

```
[commit-msg] Commit subject must start with a story ID prefix:
  td-N: <summary>        (tech debt)
  fn-X.Y: <summary>      (feature)
  dl-X.Y: <summary>      (dl stream)
  ds-X.Y: <summary>      (ds stream)
Case-insensitive. See AGENTS.md for details.
Received: <first line of commit message>
```

Empty-message rejection (AC #5) uses a distinct message, e.g.:

```
[commit-msg] Commit message is empty. Please provide a subject line with a story ID prefix (see AGENTS.md).
```

### Hook portability

- Shebang: `#!/usr/bin/env bash` (not `/bin/sh` — we need `[[ ... =~ ... ]]` and `shopt`).
- Must work on macOS default bash 3.2 (no associative arrays needed, no `mapfile`/`readarray`).
- Rely only on POSIX tools + bash builtins. No `sed -E` portability traps in required paths; if needed, use `[[ ... =~ ... ]]`.

### Case-insensitive matching

Two acceptable approaches — pick one:

- `shopt -s nocasematch; [[ "$subject" =~ $pattern ]]`
- Lowercase transform: `lower=$(printf '%s' "$subject" | tr '[:upper:]' '[:lower:]'); [[ "$lower" =~ $pattern ]]`

Storage/filename convention remains lowercase (e.g., `td-1`, `fn-1.6`) regardless of input casing.

### Merge/revert exemption

Skip the regex check when:

- `$subject` starts with `Merge ` (default merge-commit message)
- `$subject` starts with `Revert ` (default revert message)
- `$2` (source of the commit message, when provided) is `merge` or `squash`

Empty-message rejection still fires for merges only if the commit has truly empty content — normal merges produce a default non-empty `Merge branch ...` subject, so this is not an expected collision.

### Backfill scope

Only stories with `Status:` != `done` (case-insensitive) receive `branch:` and `zone:` frontmatter. Status variants seen in the tree include: `done`, `Done`, `complete`, `Complete`, `review`, `ready-for-review`, `Ready for Review`, `ready-for-dev`, `in-progress`, `blocked-hardware`, `draft`. Everything except `done`/`Done` is treated as in-flight.

Enumeration must be re-run at implementation time (story statuses move). The count and list captured here (~31 stories as of 2026-04-17) is for sizing, not authority.

### Self-test script (AC #9)

After `bash scripts/install-hooks.sh`:

```bash
# Expect REJECTED with story-ID-format error
git commit --allow-empty -m "wip: foo"

# Expect ACCEPTED
git commit --allow-empty -m "td-1: atomic flag"

# Expect ACCEPTED (case-insensitive)
git commit --allow-empty -m "TD-1: atomic flag"
```

Implementer pastes the three actual terminal outputs into the Dev Agent Record.

### Known limitations

- **Force-pushes bypass hooks.** A force-pushed branch whose commits were crafted without the hook installed will sail past local enforcement. The reviewer checklist (Task 5) is the second layer that catches this at PR time.
- **Hook cannot validate `zone:` confinement.** The hook only checks the subject-line prefix. Matching the commit's file list against the story's declared `zone:` globs requires parsing YAML frontmatter and evaluating globs against the diff — deliberately out of scope for this story. The reviewer checklist item handles zone confinement semantically; a future story may automate it.
- **No husky / Node tooling.** This repo is pure C++/PlatformIO + Python smoke tests. We do not introduce a Node toolchain to manage hooks. Pure git + bash.

### Rejection flow in prose

1. Developer runs `git commit -m "wip: fix stuff"`.
2. `.githooks/commit-msg` runs (because `core.hooksPath` was set by the install script).
3. Subject `"wip: fix stuff"` does not match the story-ID prefix regex.
4. Hook prints the rejection block, echoes the received subject, exits 1.
5. Git aborts the commit. Working tree and index are unchanged.

### Project Structure Notes

- `.githooks/` does not currently exist — it is created by this story.
- `scripts/` does not currently exist — it is created by this story.
- `_bmad-output/implementation-artifacts/review-checklist.md` is new.
- `_bmad-output/implementation-artifacts/stories/_template.md` may or may not exist; implementer must check and create/update accordingly.
- `AGENTS.md` exists at repo root and is the correct home for the install instruction per workspace convention.

### References

- [Source: deferred-work.md — 2026-04-13 fn-1.6 / dl-1.5 mixed-diff incident]
- [Source: AGENTS.md — workspace facts, BMAD workflow commands]
- [Source: `_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md` — story format reference]
- [Source: `_bmad-output/implementation-artifacts/stories/td-1-atomic-calibration-flag.md` — TD story format reference]
- [Source: git-scm docs — `core.hooksPath` config, `commit-msg` hook contract]

### Dependencies

**This story depends on:**
- Nothing. Pure tooling/process story; no code or story dependencies.

**Stories that depend on this:**
- All future stories (they use the template and are enforced by the hook).
- Any story that would otherwise silently mix concerns is now caught.

### Testing Strategy

Manual + self-test only. No Unity / pytest coverage for this story.

- Verify hook runs and rejects: three self-tests in AC #9.
- Verify install script idempotency: run `bash scripts/install-hooks.sh` twice, both succeed.
- Verify backfill: `grep -L '^branch:' _bmad-output/implementation-artifacts/stories/*.md` should only list done stories after Task 4.
- Verify reviewer checklist: open the file, confirm item 1 is exactly the locked-in line.

### Risk Mitigation

1. **Accidentally blocking a valid merge.** Mitigation: merge-commit exemption in AC #4. Tested by performing a local merge with the hook installed.
2. **Breaking existing contributors.** Mitigation: hook only activates after `bash scripts/install-hooks.sh`. Until then, contributors are unaffected. AGENTS.md update (Task 6) surfaces the install step.
3. **Backfilling the wrong zone on a story.** Mitigation: author the zone based on the story's actual touched files (grep-able via its File List section). When in doubt, err wide; reviewers will tighten at PR time.
4. **Force-push bypass.** Acknowledged as known limitation. Second-layer catch is the reviewer checklist.

## Dev Agent Record

### Agent Model Used

Claude Opus 4.7 (1M context) — Story Preparation (Bob, Technical Scrum Master)

### Debug Log References

N/A — story creation phase.

### Completion Notes List

N/A — to be filled in by implementer.

### File List

- `.githooks/commit-msg` (new, executable)
- `scripts/install-hooks.sh` (new, executable)
- `_bmad-output/implementation-artifacts/stories/_template.md` (new or update)
- `_bmad-output/implementation-artifacts/review-checklist.md` (new)
- `AGENTS.md` (update — add `## Commit Discipline` section)
- `_bmad-output/implementation-artifacts/stories/*.md` (frontmatter backfill on in-flight stories only; ~31 files as of 2026-04-17 — re-enumerate at implementation time)

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-17 | Story drafted | Bob (Technical Scrum Master) |
