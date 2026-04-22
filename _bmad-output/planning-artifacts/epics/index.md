# Epics (sharded)

This index exists so `bmad-assist` treats `epics/` as the primary sharded epic source (see `resolve_doc_path` in bmad-assist). Epic files not linked below are still loaded as orphans in filename order.

## Foundation / setup

- [FN-1 — OTA partitions, upload, settings export/import](epic-fn-1.md)
- [FN-2 — NTP time sync + night-mode scheduler](epic-fn-2.md)
- [FN-3 — Wizard test-your-wall, timezone auto-detect](epic-fn-3.md)

## Display system

- [DS-1 — Display abstraction + ClassicCardMode](epic-ds-1.md)
- [DS-2 — LiveFlightCardMode](epic-ds-2.md)
- [DS-3 — Mode orchestrator + manual selection](epic-ds-3.md)

## Delight

- [DL-1 through DL-7](epic-dl-1.md) — progressive polish (see individual shards)

## Layout Editor

- [LE-1 — Layout Editor V1 MVP](epic-le-1.md) _(closed 2026-04-22)_
- [LE-2 — Extended catalog, WidgetFont, dashboard live preview](epic-le-2.md) _(le-2.0 / le-2.1 landed; le-2.7 regression-gate in backlog)_

## Hardware

- [HW-1 — HUB75 Composite Display Migration](epic-hw-1.md) _(planning; depends on LE-1 closed + td-7 landed, both done)_

## Tech debt

- [TD epic shard](epic-td.md) — evergreen
