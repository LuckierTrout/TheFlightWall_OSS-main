## Learned User Preferences

- When triaging external analysis or assessment reports, the user may ask for **codebase-only** scope (product code and tests), excluding BMAD outputs and agent or LLM documentation collateral.

## Learned Workspace Facts

- Bundled web sources for the firmware portal live under `firmware/data-src/`; served assets are gzip-compressed under `firmware/data/`. After editing a bundled source file, regenerate the matching `.gz` from `firmware/` (for example `gzip -9 -c data-src/common.js > data/common.js.gz`).
- Firmware uses PlatformIO; run builds from `firmware/` with `pio run`. If `pio` is missing from the agent shell PATH, a typical local install on macOS exposes `~/.platformio/penv/bin/pio`.
- GitHub CLI pull-request flows (`gh pr …`) require a configured `git remote` pointing at GitHub; without a remote, PR-based `/code-review` style workflows cannot load or comment on a PR.
- There is no root `project-context.md`; BMAD and implementation work should lean on `_bmad-output/planning-artifacts/` (for example `architecture.md`, `epics.md`) and story files under `_bmad-output/implementation-artifacts/`.
- When there is no git baseline (no commits or empty diff), BMAD `bmad-code-review` can still scope review from the story’s declared file list and approximate diffs with `git diff --no-index` for new or untracked files.
- For code-only static analysis or reviews, scope primarily to `firmware/` (for example `src/`, `core/`, `adapters/`, `interfaces/`, `models/`, `config/`, `utils/`, `data-src/`, `platformio.ini`, `custom_partitions.csv`); include `firmware/test/` and repo-root `tests/` when test coverage matters; omit `_bmad/`, `_bmad-output/`, `.bmad-assist/`, typical agent or editor dirs (for example `.claude/`, `.cursor/`, `.agents/`), `docs/`, generated `firmware/data/`, and root metadata-only files such as `README.md`, `AGENTS.md`, and `LICENSE`.
- Smoke-test the onboard web portal against a running device with `python3 tests/smoke/test_web_portal_smoke.py --base-url http://<host>` (or set `FLIGHTWALL_BASE_URL`).
- On macOS, use `/dev/cu.*` (not `tty.*`) for PlatformIO `--port` / `--upload-port`. Only one process may hold the serial device—exit `pio device monitor` (Ctrl+C) before `upload` or filesystem upload.
- The web portal is served by the ESP32 (flash both app and LittleFS from `firmware/`, e.g. `pio run -e esp32dev -t upload` and `-t uploadfs`). First-time setup: join AP `FlightWall-Setup`, open `http://192.168.4.1/`; on the home network use `http://flightwall.local/` or the device’s LAN IP.
