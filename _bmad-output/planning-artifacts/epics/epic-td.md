## Epic td: Tech debt — correctness, tool hygiene, and robustness

Small, high-leverage fixes that are not user-facing features: cross-core correctness, commit discipline, native tests, and runtime safety around network fetch paths.

### Story td.1: Atomic calibration flag

As a **firmware engineer**,
I want **the cross-core calibration flag to be safe under the Xtensa memory model**,
So that **no torn reads or lost updates occur between the Core 0 render loop and Core 1 config writer**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the calibration flag is migrated to `std::atomic<bool>` with explicit memory ordering **When** the firmware is reviewed **Then** no raw `volatile bool` access remains at cross-core sites.

---

### Story td.2: hardwareConfigChanged combined geometry+mapping fix

As a **device user**,
I want **when I change both display geometry and mapping in one settings save, both changes to apply**,
So that **config saves are not silently partially-applied**.

**Status:** backlog

**Acceptance Criteria:**

**Given** a single save changes geometry and mapping **When** the apply path runs **Then** geometry and mapping outcomes match the user’s requested pair.

---

### Story td.3: OTA self-check native test harness

As a **firmware engineer**,
I want **the OTA self-check logic covered by native-host tests**,
So that **regressions are caught without flashing hardware for every change**.

**Status:** backlog

**Acceptance Criteria:**

**Given** `pio test` for the native OTA self-check suite **When** it runs on the CI/host **Then** core self-check branches are exercised without device upload where possible.

---

### Story td.4: Per-story commit discipline with enforceable gates

As a **maintainer**,
I want **story-scoped commits and hooks that reject invalid prefixes**,
So that **PRs stay reviewable and cross-story bleed is prevented**.

**Status:** backlog

**Acceptance Criteria:**

**Given** `scripts/install-hooks.sh` is applied **When** a commit message lacks a valid story prefix **Then** `commit-msg` rejects it (merge/revert exempt).

---

### Story td.5: loopTask task-watchdog abort during fetcher operation

As a **firmware engineer**,
I want **the loop task watchdog configured so long fetcher work does not false-trigger**,
So that **background fetches cannot spuriously reset the device**.

**Status:** backlog

**Acceptance Criteria:**

**Given** fetcher operations that can exceed the default TWDT window **When** they run on Core 1 **Then** the watchdog policy matches the measured worst-case duration or the task is exempted appropriately.
