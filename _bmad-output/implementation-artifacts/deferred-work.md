## Deferred from: code review (2026-04-02) — 1-3-freertos-dual-core-task-architecture.md

- **Serial vs LOG for dynamic fetch line** (`firmware/src/main.cpp` ~228): Story asks for LOG macros; dynamic string logging uses `Serial.println`. Dev Agent Record notes exception — leave until log helpers exist or policy changes.

- **ConfigManager cross-core read during `applyJson`**: Display task reads getters on Core 0 while Core 1 may update structs — confirm thread-safety or add a short critical section / snapshot pattern.

## Deferred from: code review (2026-04-02) — 2-1-dashboard-layout-display-settings-and-toast-system.md

- **`POST /api/settings` pending-body cleanup remains incomplete** (`firmware/adapters/WebPortal.cpp` ~31): aborted or over-length chunked uploads can leave stale `g_pendingBodies` entries because cleanup only happens on a few explicit paths inside the body handler; defer because this is a pre-existing WebPortal issue, not introduced by Story 2.1’s new static-asset routes.

## Deferred from: code review (2026-04-03) — 2-4-system-health-page.md

- **Unsynchronized subsystem `String` access in `/api/status`** (`firmware/core/SystemStatus.cpp` ~25): `SystemStatus::set()` and `toJson()`/`toExtendedJson()` still share mutable `String` message fields across async HTTP reads and WiFi/event writes without synchronization. Real issue, but pre-existing before Story 2.4.
