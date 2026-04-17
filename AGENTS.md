## Learned User Preferences

- When triaging external analysis or assessment reports, the user may ask for **codebase-only** scope (product code and tests), excluding BMAD outputs and agent or LLM documentation collateral.

## Learned Workspace Facts

- Bundled web sources for the firmware portal live under `firmware/data-src/`; served assets are gzip-compressed under `firmware/data/`. After editing a bundled source file, regenerate the matching `.gz` from `firmware/` (for example `gzip -9 -c data-src/common.js > data/common.js.gz`).
- Firmware uses PlatformIO; run builds from `firmware/` with `pio run`. If `pio` is missing from the agent shell PATH, a typical local install on macOS exposes `~/.platformio/penv/bin/pio`.
- GitHub CLI pull-request flows (`gh pr …`) require a configured `git remote` pointing at GitHub; without a remote, PR-based `/code-review` style workflows cannot load or comment on a PR.
- BMAD planning and implementation work lean on `_bmad-output/planning-artifacts/` (for example `architecture.md`, `epics.md`) and story files under `_bmad-output/implementation-artifacts/`. For **`bmad-assist` `create_story`**, the bundled workflow still resolves `**/project-context.md` when `project-context` is listed under `compiler.strategic_context.create_story.include` in `bmad-assist.yaml`—add `_bmad-output/project-context.md` (for example via the generate-project-context workflow) or adjust that include list.
- The **bmad-assist dashboard** is started with `bmad-assist serve --project <repo-root>` (default URL `http://127.0.0.1:9600/`). In `serve`, `-p` / `--port` is the HTTP port, not the project path. If the default port is busy, `serve` may pick the next available port (for example 9602). Root `package.json` defines `bmad:serve` (for example `pnpm run bmad:serve`) to run `bmad-assist serve`, preferring `.venv/bin/bmad-assist` when that executable exists. When the dashboard **Start** loop runs `bmad-assist run` as a subprocess, optional verbosity/debug flags come from the **`serve` process environment** (`BMAD_DASHBOARD_RUN_VERBOSE`, `BMAD_DASHBOARD_RUN_DEBUG`, `BMAD_DASHBOARD_RUN_FULL_STREAM`) or from optional `dashboard.run_with_verbose` / `run_with_debug` / `run_with_full_stream` booleans in `bmad-assist.yaml` (requires a `bmad-assist` build that implements `build_dashboard_bmad_run_args` in `bmad_assist.dashboard.server`).
- For **`bmad-assist run`**, `.bmad-assist/state.yaml` values such as `current_epic` and `current_story` should match keys under `development_status` in `_bmad-output/implementation-artifacts/sprint-status.yaml`, or the runner may skip with “story not found in sprint-status.” The **active epic list** comes from **non-done stories parsed from epic markdown** in planning artifacts (expected `### Story …` section headings); **`sprint-status.yaml` merges status onto those story IDs only** and does not create rows for backlog keys that never appear as parsed stories—so **No epics found in project** can happen when every loaded story is already `done` while remaining work exists only in sprint-status or in epic documents the parser does not treat as stories (for example tables without those section headers).
- When there is no git baseline (no commits or empty diff), BMAD `bmad-code-review` can still scope review from the story’s declared file list and approximate diffs with `git diff --no-index` for new or untracked files.
- For code-only static analysis or reviews, scope primarily to `firmware/` (for example `src/`, `core/`, `adapters/`, `interfaces/`, `models/`, `config/`, `utils/`, `data-src/`, `platformio.ini`, `custom_partitions.csv`); include `firmware/test/` and repo-root `tests/` when test coverage matters; omit `_bmad/`, `_bmad-output/`, `.bmad-assist/`, typical agent or editor dirs (for example `.claude/`, `.cursor/`, `.agents/`), `docs/`, generated `firmware/data/`, and root metadata-only files such as `README.md`, `AGENTS.md`, and `LICENSE`.
- Smoke-test the onboard web portal against a running device with `python3 tests/smoke/test_web_portal_smoke.py --base-url http://<host>` (or set `FLIGHTWALL_BASE_URL`).
- On macOS, use `/dev/cu.*` (not `tty.*`) for PlatformIO `--port` / `--upload-port`. Only one process may hold the serial device—exit `pio device monitor` (Ctrl+C) before `upload` or filesystem upload.
- On **macOS**, Python **subprocess `fork`** from a **multi-threaded** process (common under **Cursor** with certain interpreters) can **SIGSEGV** in Apple’s Network stack (`multi-threaded process forked` / `crashed on child side of fork pre-exec`). A frequent dev mitigation is `OBJC_DISABLE_INITIALIZE_FORK_SAFETY=YES` in the environment; using a **stable** interpreter (for example Python 3.12) for editor tooling instead of a very new Homebrew Python reduces how often this appears.
- The web portal is served by the ESP32 (flash both app and LittleFS from `firmware/`, e.g. `pio run -e esp32dev -t upload` and `-t uploadfs`). First-time setup: join AP `FlightWall-Setup`, open `http://192.168.4.1/`; on the home network use `http://flightwall.local/` or the device’s LAN IP.

## Commit Discipline (Story TD-4)

This repository enforces a per-story commit convention to keep diffs
reviewable and prevent cross-story bleed (see the 2026-04-13 fn-1.6 / dl-1.5
mixed-diff incident).

**Install the local hooks once per clone:**

```bash
bash scripts/install-hooks.sh
```

This sets `core.hooksPath=.githooks` and installs `commit-msg`, which rejects
any commit whose subject does not begin with a valid story-ID prefix.

**Valid subject prefixes** (case-insensitive; `.` or `-` accepted between
version numbers):

| Prefix | Example | Scope |
| --- | --- | --- |
| `td-N:` | `td-1: atomic calibration flag` | Tech debt |
| `le-X.Y:` | `le-1.1: layout store CRUD` | Layout Editor epic |
| `fn-X.Y:` | `fn-1.4: OTA self-check` | fn epic |
| `dl-X.Y:` | `dl-7.3: OTA pull dashboard` | dl epic |
| `ds-X.Y:` | `ds-3.1: display mode API` | ds epic |

Merge and revert auto-messages are exempt. Empty messages are rejected.

**Story frontmatter is required.** Every new story file in
`_bmad-output/implementation-artifacts/stories/` carries `branch:` and
`zone:` fields (see `stories/_template.md`). The `zone:` field is the
source of truth reviewers use to check that a PR stays within the story's
declared scope — see `_bmad-output/implementation-artifacts/review-checklist.md`
item #1.

**PR reviewers** should run through the full checklist at
`_bmad-output/implementation-artifacts/review-checklist.md` before approving.
The first four items (scope, correctness, safety) are required; remaining items
are advisory.

**Known limitation:** git hooks are local-only and are bypassed on force-push or
when a contributor has not run `install-hooks.sh`. PR review is the second layer;
the review-checklist codifies that layer.

## Dev-only PlatformIO Environments (Layout Editor)

`firmware/platformio.ini` carries two auxiliary environments beyond the
production `[env:esp32dev]`:

- **`[env:esp32dev_le_baseline]`** — extends `esp32dev` with `-D LE_V0_METRICS`.
  Compiled for render-budget measurement runs against the fixed `ClassicCardMode`
  baseline.
- **`[env:esp32dev_le_spike]`** — extends `esp32dev` with `-D LE_V0_METRICS
  -D LE_V0_SPIKE`. Adds the legacy `CustomLayoutMode` entry to the mode table
  (guarded by `#ifdef LE_V0_SPIKE` in `firmware/src/main.cpp`) so the generic
  renderer can be exercised for spike measurements.

These are **dev-only performance-measurement scaffolds**, not shipping builds.
Production firmware is always built from `[env:esp32dev]` with neither flag.
The `LE_V0_METRICS` instrumentation hooks were removed in Story le-1.1 (V0
Spike Cleanup); re-introducing them requires re-wiring timing around
`ModeRegistry::tick()`. The `LE_V0_SPIKE` mode-table guard remains in place
until Story LE-1.3 replaces the spike mode with the full LayoutStore-backed
`CustomLayoutMode`.
