# Story <ID>: <Title>

<!--
Frontmatter fields below are REQUIRED (enforced by TD-4).
- `branch:` the kebab-case branch name for this story. Must match the commit-msg
  prefix convention (see AGENTS.md).
- `zone:`   list of path globs the story is allowed to touch. Reviewers reject
  PRs whose diff escapes this zone (see review-checklist.md item #1).
-->

branch: <kebab-case-branch-name>
zone:
  - <path/glob/1>
  - <path/glob/2>

Status: draft

<!-- Optional: validation note, workflow refs -->

## Story

As a **<role>**,
I want **<capability>**,
so that **<benefit>**.

## Acceptance Criteria

1. **Given** ... **When** ... **Then** ...
2. ...

## Tasks / Subtasks

- [ ] **Task 1: <Name>** (AC: #N)
  - [ ] <concrete sub-action with file path>

- [ ] **Task N: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5 MB OTA partition
  - [ ] New and existing tests pass

## Dev Notes

<Context, constraints, file references, code snippets. Explain the *why*
and any non-obvious invariants that an implementer needs to know. Keep this
section tight — comment-worthy information only.>

## File List

- <path> (new|modified)
- ...
