## Learned User Preferences

## Learned Workspace Facts

- Firmware uses PlatformIO; run builds from `firmware/` with `pio run`. If `pio` is missing from the agent shell PATH, a typical local install on macOS exposes `~/.platformio/penv/bin/pio`.
- GitHub CLI pull-request flows (`gh pr …`) require a configured `git remote` pointing at GitHub; without a remote, PR-based `/code-review` style workflows cannot load or comment on a PR.
- There is no root `project-context.md`; BMAD and implementation work should lean on `_bmad-output/planning-artifacts/` (for example `architecture.md`, `epics.md`) and story files under `_bmad-output/implementation-artifacts/`.
- When there is no git baseline (no commits or empty diff), BMAD `bmad-code-review` can still scope review from the story’s declared file list and approximate diffs with `git diff --no-index` for new or untracked files.
