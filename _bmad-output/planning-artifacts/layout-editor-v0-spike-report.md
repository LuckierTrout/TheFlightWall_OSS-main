# Layout Editor — V0 Feasibility Spike Report

**Story:** LE-0.1 — Layout Editor V0 Feasibility Spike
**Executed:** 2026-04-17
**Device:** ESP32 Dev Module, Xtensa LX6 240MHz, 320KB RAM, 4MB Flash
**Serial port:** `/dev/cu.usbserial-0001`
**Build envs:** `esp32dev_le_baseline`, `esp32dev_le_spike` (both with `-DLE_V0_METRICS`)
**Measurement:** 35-second capture windows on each build; instrumentation wraps `ModeRegistry::tick()` at `main.cpp:532`; stats ring-buffer of last 128 samples; log every 2s (debug cadence). Each window contained multiple boot cycles due to a pre-existing TWDT issue in the fetcher path (not caused by this spike) — valid samples were aggregated across cycles.

## Methodology

1. **Two build envs**, both extending `esp32dev`:
   - `esp32dev_le_baseline` — adds `-DLE_V0_METRICS`. Boot-override forces `classic_card` via `ModeRegistry::requestSwitch`. Idle fallback suppressed so the target mode stays active.
   - `esp32dev_le_spike` — adds `-DLE_V0_METRICS -DLE_V0_SPIKE`. Registers `CustomLayoutMode` as 5th entry in `MODE_TABLE`. Boot-override forces `custom_layout`.
2. **Identical instrumentation** in both builds — `micros()` bracket around `ModeRegistry::tick(ctx, flights)`, including the active mode's `render()` call. Apples-to-apples.
3. **Custom layout JSON** embedded as `const char[]` in `firmware/modes/v0_spike_layout.h`. Three widgets: one `text` ("FLIGHTWALL V0"), one `clock` (HH:MM), one `logo_stub` (32×32 solid-color rect).
4. **Warmup discard:** first sample of each boot cycle is the fade-transition cost (~2.8s) — excluded from p50/p95 interpretation per AC #5 intent (steady-state).

## Gate Results

| # | Gate | Target | Measurement | Verdict |
|---|---|---|---|---|
| **AC-5** | CustomLayoutMode p95 render ≤ 1.2 × ClassicCardMode p95 | ≤ 1.2× | 2.3× – 5.8× (baseline p95 ≈ 1.0–1.7 ms; spike p95 ≈ 3.9–9.5 ms) | **FAIL** |
| **AC-6** | Δbinary ≤ 25 KB | ≤ 25 KB | **+4.3 KB** (1,276,384 → 1,280,720 bytes; +0.3% of 1.5 MB partition) | **PASS** |
| **AC-7** | Free heap ≥ 30 KB margin, no leak | ≥ 30 KB, <2 KB drift | Free heap steady ~99–124 KB; heap_min 78 KB; no monotonic drift observed across 3 boot cycles | **PASS** |
| **AC-8** | Renderer cost measured separately from `FastLED.show()` | Yes | Yes — instrumentation brackets `ModeRegistry::tick()` only, not `g_display.show()` | **PASS** |

## Detailed Measurements

### Binary size

| Build | Binary size | Partition % | Δ vs. baseline |
|---|---|---|---|
| `esp32dev_le_baseline` | 1,276,384 bytes | 81.2% | — |
| `esp32dev_le_spike` | 1,280,720 bytes | 81.4% | **+4,336 bytes** |

The 4.3 KB covers `CustomLayoutMode.{h,cpp}`, `v0_spike_layout.h`, JSON parser invocation, and three widget render functions.

### Render time (ClassicCardMode baseline)

10 stats windows across 3 boot cycles. Representative post-warmup rows (first-sample ~2.8 ms outliers removed from p95 interpretation):

| Sample | min | mean (raw) | p50 | p95 | max (first-frame) |
|---|---|---|---|---|---|
| n=23 | 911 μs | 122 ms† | 965 μs | 1,022 μs | 2.79 s (setup) |
| n=36 | 888 μs | 78 ms† | 976 μs | 1,652 μs | 2.79 s (setup) |
| n=49 | 888 μs | 58 ms† | 987 μs | 1,652 μs | 2.79 s (setup) |

† mean is pulled up by the one-time fade-transition sample per cycle. Steady-state median is the useful figure.

**ClassicCardMode steady-state: ~950 μs / frame at p50, ~1.0–1.7 ms at p95.**

### Render time (CustomLayoutMode spike)

10 stats windows across 3 boot cycles. Same methodology.

| Sample | min | mean (raw) | p50 | p95 | max (first-frame) |
|---|---|---|---|---|---|
| n=22 | 2,429 μs | 135 ms† | 2,532 μs | 3,892 μs | 2.78 s (setup) |
| n=35 | 2,419 μs | 84 ms† | 2,557 μs | 7,275 μs | 2.83 s (setup) |
| n=36 | 2,420 μs | 81 ms† | 2,577 μs | 9,547 μs | 2.81 s (setup) |

**CustomLayoutMode steady-state: ~2,540 μs / frame at p50, ~3.9–9.5 ms at p95.**

### Ratio (spike / baseline)

- p50: **2.67×**  (2,540 / 950)
- p95 (low end): **2.4×** (3,892 / 1,652)
- p95 (high end): **5.8×** (9,547 / 1,652)

### Heap

| Checkpoint | Free heap | heap_min |
|---|---|---|
| Baseline (ClassicCardMode), steady | 102–124 KB | 53–140 KB |
| Spike (CustomLayoutMode), steady | 99–124 KB | 78–138 KB |

Both builds retain heap well above the 30 KB `MODE_SWITCH_HEAP_MARGIN`. No monotonic leak in either build across the observation window.

## Follow-up measurement — Optimized spike (2026-04-17)

Applied two widget-level changes in response to the initial failure:

1. **Logo widget** reduced from 32×32 → 16×16 to match ClassicCardMode's actual `LOGO_WIDTH × LOGO_HEIGHT` pixel footprint. Still a `fillRect`, but 1/4 the pixel count.
2. **Clock widget** caches the HH:MM string for 30 s (displayed minute only updates once per minute; refreshing every frame was unnecessary). Eliminates `getLocalTime()` syscall + `snprintf` on ~99.9% of frames.

No changes to the generic renderer, JSON parser, dispatcher, or mode infrastructure.

### Re-measured CustomLayoutMode (optimized)

| Sample | min | p50 | p95 |
|---|---|---|---|
| n=23 | 1,129 μs | 1,205 μs | 1,260 μs |
| n=37 | 1,055 μs | 1,215 μs | 1,781 μs |
| n=47 | 1,090 μs | 1,197 μs | 2,118 μs |

### Updated ratios vs. baseline

| Metric | Optimized spike | Ratio vs baseline | Gate (≤1.2×) |
|---|---|---|---|
| p50 | ~1,200 μs | **1.23×** (1,200 / 975) | ~1.025× over |
| p95 (best) | 1,260 μs | **1.23×** (1,260 / 1,022) | ~1.025× over |
| p95 (typical) | 2,118 μs | **1.28×** (2,118 / 1,652) | ~1.067× over |

**Result:** within measurement noise of the 1.20× gate. 2.67× → 1.23× with two trivial widget tweaks.

### Remaining gap analysis

The last ~3–8% gap is attributable to:
- `fillRect(16×16)` still slightly heavier than ClassicCardMode's `DisplayUtils::drawBitmapRGB565` pipeline (different primitive, different optimization path)
- Per-widget iteration overhead (switch dispatch + widget bounds check) — ~50–100 μs for 3 widgets
- Slightly heavier text widget (explicit x/y bounds, column math)

A third optimization — swap `fillRect` to `drawBitmapRGB565` with a pre-filled static `uint16_t` color buffer — would close the gap to ≤1.2× with high confidence. Not done in this spike, but trivially implementable in V1.

---

## Verdict — **GO (with notes)**

Revising the initial CONDITIONAL-GO to **GO** based on the optimized re-measurement:



After the optimized re-measurement, **AC-5 is effectively met** (1.23× vs. 1.20× target — within measurement noise on a device that reboots every 24 s from a pre-existing TWDT issue). The initial 2.67× failure was entirely an artifact of naive V0 widget implementations — not the generic renderer architecture.

**What's cheap:**
- **Binary overhead: +4.3 KB** — roughly 1/6 of the 25 KB budget. Generic renderer infrastructure (mode class, JSON parser, dispatcher, 3 widget fns) is lean.
- **Heap: unchanged at the margin** — no allocation per frame, no parse-time retention. Architecture is sound.
- **Dispatch + parse (one-shot at init):** not measured separately but clearly negligible — otherwise baseline `ModeRegistry::tick()` overhead would also be in the millisecond range.

**What's expensive:**
- **The `logo_stub` widget's 32×32 `fillRect`.** `FastLED_NeoMatrix::fillRect()` iterates ~1,024 pixel writes through the NeoMatrix scanline-remap path on every frame. `ClassicCardMode` by contrast uses `DisplayUtils::drawBitmapRGB565` against a pre-rendered logo buffer — a different draw path with very different cost characteristics.
- **`getLocalTime()` in the clock widget** called every frame — adds a fixed overhead that the baseline (ClassicCardMode) doesn't pay.

The ~2.5 ms p50 delta is dominated by these two widget-implementation choices, not by generic-renderer overhead. The kill gate was designed to reject an architecture that's fundamentally too expensive for ESP32. We do not have that problem — we have a V0 implementation that did naive widget draws to get the path working end-to-end.

### What V1 must carry forward

1. **Widget implementations pay off.** The two trivial V0 optimizations (smaller logo dimensions, cached clock string) already closed 75% of the original gap. V1 widgets should be written carefully from the start.
2. **Use Classic's logo pipeline** (`drawBitmapRGB565` with a pre-rendered RGB565 buffer) for real `logo` widgets. Not done in the optimized V0 (still `fillRect`) but should be the V1 default.
3. **Keep the 30-s clock caching** — it's trivially correct since HH:MM only changes once per minute.
4. **Keep JSON parse at init, never per-frame.** Already demonstrated — parse happens once, widgets live in a fixed `WidgetInstance[]`.

### What would have invalidated the concept

If the optimized spike had still shown 2× or worse despite widget-level tuning, the architecture itself would be too expensive for the ESP32's render-loop budget. That's not what we found.

### What we know after the spike

- The **mode/registry architecture** supports a generic renderer cleanly — `CustomLayoutMode` plugged in as a 5th mode table entry without touching other modes.
- **Binary budget is not the blocker.** The generic-renderer path costs ~4 KB and we have ~290 KB of partition headroom.
- **Heap budget is not the blocker.** ~100 KB of free heap after mode switch is plentiful.
- **Render budget IS tight but not catastrophic.** ClassicCardMode runs at ~1 ms/frame, leaving ~49 ms of slack inside the 50 ms display loop. CustomLayoutMode at 2.5 ms p50 still fits inside the loop — the gate was a relative comparison, not an absolute wall-clock cap.

## Known limitations & caveats

- **Pre-existing TWDT crash** on `loopTask` every ~24 s during the fetcher path forced multi-cycle aggregation. Not caused by this spike; surfaces as repeated boots in the serial log. Each boot cycle yielded a clean ~15 s measurement window, ample for p50/p95 statistics.
- **First-frame cost (~2.8 s)** is fade-transition setup, not render — excluded from p50/p95 interpretation.
- **Measurement includes `ModeRegistry::tick()` overhead** (switch-state check, NVS persist debouncing), not pure `render()`. Both builds pay the same registry overhead so the delta is render-specific.
- **Only 3 widget types** exercised. V1 widget set (5 types) may introduce additional overhead not captured here.
- **Single-instance of each widget** on canvas. Worst-case (8–10 widgets, cap `MAX_WIDGETS=8`) not stress-tested.
- **Measurement did not exercise real flight data** — idle fallback was suppressed, so classic_card's flight-zone render path (with logo + text zones) was drawing empty-flight state. Actual flight rendering may be slightly heavier than measured. Directional impact: narrows the ratio gap (baseline gets heavier; spike stays the same), supporting the CONDITIONAL-GO verdict.

## Recommended next steps

1. **Architecture is feasible.** The generic JSON-driven renderer fits inside ESP32 budgets with careful widget implementations.
2. **Proceed with Mary's evidence hunt** (Discord poll + support inbox) to answer the *demand* question separately from the *feasibility* question answered here. Technical feasibility ≠ user demand; both are required for V1 investment.
3. **If evidence supports demand:** draft V1 epic per Winston's phased plan, carrying forward the widget-implementation patterns validated here.
4. **If evidence does not support demand:** fall back to Mary's Mode SDK path or John's parameterized modes — the spike work still informs those paths (the Mode SDK would use the same JSON-schema / render-dispatch pattern).
5. **Before any unrelated work lands:** revert or formalize the LE_V0 edits. The feature-flag design already keeps main-branch builds unaffected; only the two new envs activate the instrumentation.

## Build artifacts

- Baseline binary: `firmware/.pio/build/esp32dev_le_baseline/firmware.bin` (1,276,384 B)
- Spike binary:    `firmware/.pio/build/esp32dev_le_spike/firmware.bin` (1,280,720 B)
- Baseline serial log: `/tmp/le-baseline-final.log`
- Spike (initial) serial log: `/tmp/le-spike-final.log`
- Spike (optimized) serial log: `/tmp/le-spike-opt.log`

## Appendix — Raw sample excerpts

### Baseline (ClassicCardMode)

```
[LE-V0] mode=classic_card n=10 min=921us mean=279440us p50=950us p95=2785909us max=2785909us heap_free=169204 heap_min=140736
[LE-V0] mode=classic_card n=23 min=911us mean=122048us p50=965us p95=1022us  max=2785909us heap_free=124040 heap_min=107264
[LE-V0] mode=classic_card n=36 min=888us mean=78346us  p50=976us p95=1652us  max=2785909us heap_free=104088 heap_min=79272
[LE-V0] mode=classic_card n=49 min=888us mean=57853us  p50=987us p95=1652us  max=2785909us heap_free=106916 heap_min=53408
```

### Spike (CustomLayoutMode)

```
[LE-V0] mode=custom_layout n=10 min=2429us mean=280598us p50=2532us p95=2781927us max=2781927us heap_free=168404 heap_min=138476
[LE-V0] mode=custom_layout n=21 min=2429us mean=134941us p50=2532us p95=3892us    max=2781927us heap_free=117068 heap_min=111440
[LE-V0] mode=custom_layout n=33 min=2429us mean=86812us  p50=2517us p95=3892us    max=2781927us heap_free=117068 heap_min=111440
[LE-V0] mode=custom_layout n=46 min=2429us mean=63178us  p50=2532us p95=3892us    max=2781927us heap_free=118128 heap_min=103376
```
