# Layout Editor — Evidence Hunt (3-Day Plan)

**Owner:** Christian
**Started:** 2026-04-17
**Target completion:** 2026-04-20
**Status:** in-progress

## Purpose

Before committing to a drag-and-drop layout editor, gather evidence to decide between three candidate directions surfaced in the 2026-04-17 party-mode roundtable:

- **(J) V0 — Remix templates** — 9-zone logo placement + accent color + telemetry field swap. No editor.
- **(C) V1/V2 — Drag-and-drop layout editor** — the original request.
- **(M) Mode SDK + parameterized modes** — declarative community-authored modes, LaMetric/Tidbyt pattern.

Decision framework: **falsification criteria are pre-registered below.** Do not retro-fit the verdict to match the preferred outcome.

## Falsification Criteria (pre-registered — do not edit after data collection)

Tag every signal collected as one of:
- **TWEAK** — "wish I could nudge the existing layout" (logo position, color, field swap)
- **NEW-DATA** — "wish it could show X data type" (weather, METARs, NOTAMs, second airport, etc.)
- **AUTHORSHIP** — "wish I could build my own layout from scratch"
- **OTHER** — unrelated complaints/asks (hardware, install, data accuracy, etc.)

**Decision rules:**

| Pattern in collected signals | Verdict |
|---|---|
| < 20% total mention layout/position/drag | **John wins** → ship V0 (parameterized modes only), kill editor concept |
| > 40% tagged NEW-DATA | **Mary's SDK path wins** → ship Mode SDK as primary direction |
| > 40% tagged TWEAK, < 20% AUTHORSHIP | **Logo placement / parameterized modes** — build 9-zone logo + field-swap feature, no editor |
| > 30% tagged AUTHORSHIP | **Christian's instinct wins** → proceed with drag-drop editor (but still review competitive evidence first) |
| Mixed, no clear signal | Default to **parameterized modes this sprint + Mode SDK next quarter**; defer editor indefinitely |

Also evaluate qualitatively:
- Is the pattern different for **OSS builders** vs **commercial buyers**? If so, the two audiences may warrant different roadmap tracks.
- Is any single loud voice (N=1) skewing the tally? Discount outliers.

## Day 1 AM — GitHub Issues + Discussions (~2h)

- [ ] Search `issues` and `discussions` on the FlightWall GitHub repo for terms: `layout`, `customize`, `move`, `logo`, `position`, `add`, `mode`, `widget`, `arrange`, `drag`, `custom`
- [ ] For each hit, record: issue/discussion URL, one-line summary, tag (TWEAK / NEW-DATA / AUTHORSHIP / OTHER), audience guess (OSS builder / commercial / unknown)
- [ ] Count totals per tag
- [ ] Capture any direct quotes that vividly express the pain

Output: append a table to this doc under `## Day 1 AM Results`.

## Day 1 AM Results

**Executed:** 2026-04-17 by Mary (research agent)
**Method:** `gh issue list --repo LuckierTrout/TheFlightWall_OSS-main --state all --limit 100` + `gh api repos/LuckierTrout/TheFlightWall_OSS-main/discussions`

**Repo metadata (from `gh api`):**
- Created: 2026-04-03 (14 days old at time of hunt)
- Stars: 0, Forks: 0, Watchers: 0, Subscribers: 0
- `has_discussions: false` (discussions disabled — API returned HTTP 410 "Discussions are disabled for this repo")
- `open_issues_count: 0`, total issues returned: 0

| Source | Quote / summary | Tag | Audience |
|---|---|---|---|
| `gh issue list` (all states) | Zero issues — empty array returned | N/A | N/A |
| `gh api .../discussions` | HTTP 410 "Discussions are disabled for this repo" | N/A | N/A |

**Count-by-tag summary:**
- TWEAK: 0
- NEW-DATA: 0
- AUTHORSHIP: 0
- OTHER: 0
- **Total signal: N=0**

**Finding:** No GitHub signal. The public OSS repo is 14 days old, has zero community engagement (0 stars, 0 forks, 0 issues, discussions disabled). This is a **null finding** — not evidence that users don't want layout customization, just that the OSS channel hasn't had time to accumulate any signal. Do NOT weight the absence of AUTHORSHIP asks here as evidence against the editor concept. Treat Day 1 AM as inconclusive and rely on Days 1 PM + 2 + 2-evening + 3 to break the tie.

## Day 1 PM — External Community Scan (~2h)

- [ ] r/led: search "FlightWall" + "LED matrix layout" + "ESP32 flight display"
- [ ] r/esp32: same
- [ ] Hackaday comments on LaMetric, Tidbyt, Awtrix articles — pain patterns
- [ ] Adafruit forum — MatrixPortal / LED-matrix threads
- [ ] Record same fields as Day 1 AM

Output: append to `## Day 1 PM Results`.

## Day 1 PM Results

**Executed:** 2026-04-17 by Mary
**Method:** WebSearch (Reddit, Hackaday, Adafruit, general) + WebFetch on high-signal results
**Queries run:** `"TheFlightWall" site:reddit.com`, `"FlightWall" ESP32 LED display reddit`, `ESP32 LED matrix flight tracker display customize layout`, `"flight tracker" LED matrix display forum.adafruit.com OR hackaday.com customize`

| Source | Quote / summary | Tag | Audience |
|---|---|---|---|
| `"TheFlightWall" site:reddit.com` — search returned zero indexed Reddit posts/comments | No Reddit discussion of TheFlightWall found via Google/Bing indexes | N/A | N/A |
| [Hackster.io: "A Flight Tracker for Your Wall"](https://www.hackster.io/news/a-flight-tracker-for-your-wall-04d5db817193) | Coverage article, no end-user pain quotes surfaced — editorial only | N/A | unknown |
| [rzeldent/esp32-flightradar24-ttgo (GitHub)](https://github.com/rzeldent/esp32-flightradar24-ttgo) | Adjacent OSS project, TTGO form factor — surfaced by search but no layout-related issues visible in first-page results | N/A | OSS builder |
| [smartbutnot/flightportal (GitHub)](https://github.com/smartbutnot/flightportal) | Adafruit MatrixPortal flight-overhead project — adjacent prior art, no layout-editor asks surfaced | N/A | OSS builder |
| [FlightTrackerLED product page](https://flighttrackerled.com/) — commercial competitor | Product page explicitly markets "fully customizable — you can adjust it to fit your space, preferences, and how you want flight data presented" as a selling point (vendor claim, not user pain) | TWEAK (supply-side signal — vendor thinks this matters) | commercial buyer |
| Forum.adafruit.com / hackaday.com search for flight-tracker + customize | Zero relevant results returned | N/A | N/A |

**Count-by-tag summary:**
- TWEAK: 1 (supply-side / vendor positioning, not a user complaint)
- NEW-DATA: 0
- AUTHORSHIP: 0
- OTHER: 0
- **Total user-voice signal: N=0**

**Finding:** No FlightWall-specific community chatter exists yet outside the vendor's own channels. The single TWEAK-ish signal is a competitor (FlightTrackerLED) positioning "customize how data is presented" as a differentiator — this is vendor marketing, not end-user pain. Null finding on the public web for FlightWall user voice; pivoted to adjacent-category signals in Day 2.

## Day 2 — Competitor Review Mining (~3h)

Focus on 1-star and 2-star reviews (dissatisfiers reveal the real JTBD):

- [ ] **LaMetric Time + LaMetric Sky** — Amazon, Reddit (r/LaMetric)
- [ ] **Tidbyt** — Reddit (r/tidbyt), Amazon
- [ ] **Ulanzi TC001 / Awtrix** — Amazon, r/awtrix
- [ ] **Divoom Pixoo** (secondary — different form factor)

For each negative review, tag the complaint: LAYOUT / APPS-LIMITED / APP-BROKE / UGLY / DATA-QUALITY / HARDWARE / PRICE / OTHER.

**Competitive hypothesis to test:** "can't customize layout" is NOT the top dissatisfier in this category. If it is the top dissatisfier, Christian's instinct was right and the market has space for a layout editor.

Output: append to `## Day 2 Results`.

## Day 2 Results

**Executed:** 2026-04-17 by Mary
**Method:** WebSearch on competitor complaint terms + WebFetch on thread-level sources for direct quotes. Products covered: Tidbyt, LaMetric Time, LaMetric Sky, Ulanzi TC001 / Awtrix. Divoom Pixoo covered only via comparison articles (secondary, as doc noted).
**Caveat:** Amazon review pages returned 403/503 to WebFetch — fell back to aggregator summaries (Sleepline, HowToGeek) and first-party forums (discuss.tidbyt.com, help.lametric.com, Blueforcer/awtrix3 GitHub issues). Direct 1-star Amazon verbatim coverage is therefore thinner than planned.

| Source | Quote / summary | Tag | Audience |
|---|---|---|---|
| [Tidbyt forum: "Layout customization + Clear acrylic glass" (2022)](https://discuss.tidbyt.com/t/layout-customization-clear-acrylic-glass/689) | OP: "Would you be able to add some sort of layout customization feature like they did?" — asks for split-screen / 4-corner layout like a competitor. 5 distinct users engaged; thread fizzled on diffuser-panel tangent. | LAYOUT | commercial |
| [Tidbyt forum: "My Ugly Rant for a Spectacular Product"](https://discuss.tidbyt.com/t/my-ugly-rant-for-a-spectacular-product/5813) | Primary pains (summary): apps don't get updated ("MONTHS since I've seen a single change"), "80% of those apps are so very niche," unresponsive support, scheduling bugs, Tidbyt Plus paywall on basic features. | APPS-LIMITED, APP-BROKE, PRICE, OTHER (support) | commercial |
| [Tidbyt forum: "Regret"](https://discuss.tidbyt.com/t/regret/6650) | Summary: poor customer support, QC problems ("no quality control happening"), shipping credit failures, hardware reliability issues, would not recommend. | HARDWARE, OTHER (support), PRICE | commercial |
| [HowToGeek Tidbyt review](https://www.howtogeek.com/120704/tidbyt-review/) | Cons: can't wall-mount (USB-C sticks out back), text hard to read at desk distance, "out of luck if you don't like the dark walnut wood look," "pricey," "more of a conversation piece" than utility. | HARDWARE, UGLY (finish not layout), PRICE, OTHER | commercial |
| [macwright.com: "Tidbyt without the company" (2025)](https://macwright.com/2025/04/12/tidbyt-second-life) | Company acquihired by Modal → user self-hosting. Critiques pixlet/Starlark as "very niche, limited, and overall just weird" for self-hosters. Wants JS/Python-native authorship path. | OTHER (company shutdown), AUTHORSHIP (wants own-authored apps in familiar language) | OSS builder |
| [Tidbyt forum: "I tried to make a better App Store for Tidbyt"](https://discuss.tidbyt.com/t/i-tried-to-make-a-better-app-store-for-tidbyt/6574) | Third-party rebuilt the app-store UX because "discovering new apps, finding similar ones, and even basic functionalities are challenging in the current app store." Not layout — app-discovery / curation UX. | APPS-LIMITED (discovery) | commercial + OSS hybrid |
| [Tidbyt forum: "Mimic LaMetric Time"](https://discuss.tidbyt.com/t/mimic-la-metric-time/5442) | User wants holiday-themed animations auto-swapped onto existing WeatherClock app (Christmas tree 12/25, Halloween icon 10/31, Easter). | NEW-DATA (conditional holiday content) | commercial |
| [LaMetric Sky help forum — app crashes, pairing failures, Home Assistant integration broken for a year (summary)](https://help.lametric.com/support/discussions/forums/6000242477) | Can't add device, blank setup menus, pairing hangs, pattern selection does nothing, HA integration broken since firmware change ~1yr ago. Missing touch/sound-reactive features. | APP-BROKE, HARDWARE, APPS-LIMITED | commercial |
| Sleepline aggregated LaMetric Time review (covers Amazon feedback indirectly) | "Very disappointed set of apps. Very disappointed that you're stuck with default apps." WiFi drops repeatedly. Build-quality failures <2yr (PCB components desoldered). Slow refund/support. 8x8 graphics limit what apps can show. | APPS-LIMITED, HARDWARE, APP-BROKE, OTHER (support) | commercial |
| [Blueforcer/awtrix3 issue #553 — random reboots on Ulanzi TC001](https://github.com/Blueforcer/awtrix3/issues/553) | Device reboots 1-2x/day with no triggering event, brief colored dot before restart. | HARDWARE | OSS builder |
| [Blueforcer/awtrix3 issue #771 — won't boot with PSU connected](https://github.com/Blueforcer/awtrix3/issues/771) | TC001 beeps continuously and won't boot when plugged into some PSUs. | HARDWARE | OSS builder |
| [Blueforcer/awtrix3 issue #67 — "Clock is dead"](https://github.com/Blueforcer/awtrix3/issues/67) | Device bricked, frozen on time display, won't accept reset or reflash. | HARDWARE | OSS builder |
| [Ulanzi TC001 Privacy Review (Unwanted.cloud)](https://unwanted.cloud/ulanzi-tc001-smart-pixel-clock-privacy-review/) | Concern: stock firmware phones home; workaround is flashing AWTRIX to regain control. (Implicitly AUTHORSHIP — user wants control.) | OTHER (privacy), AUTHORSHIP (flash-your-own) | OSS builder |

**Count-by-tag summary (Day 2 competitor mining, N=13 distinct signals):**
- LAYOUT: 1 (~8%)
- APPS-LIMITED: 4 (~31%) — includes "apps too niche," "stuck with default apps," poor discovery, missing capabilities
- APP-BROKE / APPS-NOT-UPDATED: 3 (~23%)
- HARDWARE: 6 (~46%) — most common single category, spans reboots/bricking/build-quality/form-factor
- UGLY: 1 (~8%) — about walnut finish, not layout
- DATA-QUALITY: 0
- PRICE: 3 (~23%)
- OTHER (support, company direction, privacy): 5 (~38%)
- AUTHORSHIP: 2 (~15%) — macwright self-host + Ulanzi-flash-AWTRIX
- NEW-DATA: 1 (~8%) — holiday animations

*(Percentages sum >100% because several threads carry multiple complaint tags.)*

**Answer to the key question — "Is 'can't customize layout' the TOP dissatisfier in this category?":**
**No.** In the competitor category, layout-customization appears as a complaint in roughly 1 of 13 signals (~8%). The dominant dissatisfiers are **HARDWARE reliability** (~46%), **apps being limited or broken** (~54% combined), and **support/company-direction concerns** (~38%). Layout customization is present but is a minority complaint — it does not lead the category.

## Day 2 Evening — Community Question (~1h)

- [ ] Post to FlightWall Discord / IG comments / whatever channel reaches real users:
  > *"If you could change ONE thing about your FlightWall display, what would it be? (No wrong answers — just curious what's on your mind.)"*
- [ ] Do NOT lead, do NOT mention "layout editor." Open-ended only.
- [ ] Collect responses over 24h, aim for ≥ 20. Tag per the scheme above.
- [ ] If < 10 responses, extend one extra day.

Output: append to `## Day 2 Evening Results`.

## Day 3 — Commercial Buyer Signals (~2h)

- [ ] theflightwall.com support inbox — last 90 days, what do buyers actually ask for / complain about?
- [ ] Return / refund reasons if any
- [ ] Any customer-sourced photos — how are real units deployed? What goes on the wall around the FlightWall (dictates what role it plays in the room)?

Output: append to `## Day 3 Results`.

## Research Notes from Mary

**Author:** Mary (business analyst agent), executing Days 1-2 of the hunt on 2026-04-17.
**Scope executed:** Section A (GitHub), Section B (external community scan), Section C (competitor review mining). Day 2-evening (Discord/IG poll) and Day 3 (support inbox, returns, photos) were **not** executed — those require Christian's direct access and are flagged below.

### What I found
- **Null findings on FlightWall's own channels.** Repo is 14 days old, 0 stars, 0 forks, 0 issues, discussions disabled. No Reddit / Hackaday / Adafruit hits for "TheFlightWall" or "FlightWall" + ESP32 as of this search. This is expected for a brand-new OSS release — don't over-interpret.
- **Competitor category's top dissatisfier is hardware reliability, not layout.** Across 13 distinct signals from Tidbyt, LaMetric Time/Sky, and Ulanzi TC001/Awtrix communities, HARDWARE complaints dominate (~46%), followed by APPS-LIMITED/BROKEN (~54% combined), then SUPPORT/company-direction (~38%). LAYOUT shows up once (~8%).
- **The one concrete layout-customization ask I found** (Tidbyt forum thread #689) is specifically about **multi-app split-screen** ("4 corners of the LED panel"), not freeform drag-and-drop. That's closer to John's V0 (parameterized 9-zone template) than to Christian's V1/V2 (freeform editor).
- **Two AUTHORSHIP signals surfaced, both from OSS/self-host builders**: Tom MacWright wanting a non-Starlark path for self-hosted Tidbyts, and Ulanzi-flash-AWTRIX users wanting to escape stock firmware. Both are about **owning the runtime/language**, not drag-drop layout. This aligns with Mary's Mode-SDK direction more than with a GUI editor.
- **One NEW-DATA ask** (Tidbyt "Mimic LaMetric Time") was for conditional/holiday content, not a new data source.
- **Vendor-side signal:** competitor FlightTrackerLED markets "fully customizable — adjust how flight data is presented" as a selling point, which suggests *vendors believe* this matters for conversion even if end-user complaint volume is low. Worth weighing separately from user pain.

### What I could not find
- **Any FlightWall end-user voice in public.** No reddit, hackaday, adafruit, or forum posts mentioning TheFlightWall by name outside Hackster/Threads coverage.
- **Direct Amazon 1-star verbatim for LaMetric / Tidbyt.** Amazon review pages returned 403/503 to automated fetches; I relied on aggregator summaries (Sleepline, HowToGeek) and first-party support forums. Verbatim 1-star Amazon quotes should be spot-checked manually before the verdict is locked.
- **r/awtrix / r/LaMetric direct thread content.** Searches with `site:reddit.com` returned zero indexed hits for layout-specific complaints; subreddits may be small or not indexed. A manual Reddit-native search on each sub would tighten this.
- **Photos of installed FlightWalls in the wild.** This is a Day 3 item and needs Christian's access to customer-submitted photos / social tags.

### Methodological caveats
- **Small sample (N=13 competitor signals).** Don't treat the 8% LAYOUT rate as a precise estimate — it's directional only. Confidence interval at this N is wide.
- **Audience skew toward commercial buyers.** Competitor data is heavily commercial (Tidbyt/LaMetric owners on vendor forums). OSS-builder voice is underrepresented in Day 2 — only ~3 of 13 signals. If the FlightWall OSS audience diverges from commercial buyers, Day 2 may undercount AUTHORSHIP demand.
- **Survivorship bias in forum rants.** The Tidbyt "ugly rant" and "regret" threads are self-selected complainers; they reliably surface pains but overweight negativity vs. silent-happy majority. Good for JTBD dissatisfier hunting; bad for market sizing.
- **No quantitative weighting applied.** Each signal counts as 1 regardless of thread engagement/upvotes. A forum rant with 40 replies is not weighted more than a single comment. Consider upvote-weighting before the final tally.

### What still needs Christian (do NOT skip these)
1. **Day 2 Evening — Discord / IG open-ended poll.** Target ≥20 responses with the non-leading prompt in the doc. This is the single most important data source because it's FlightWall-specific and captures unprompted JTBD. Cannot be run by an agent.
2. **Day 3 — theflightwall.com support inbox, last 90 days.** Refund/return reasons, repeat asks, photo submissions. This is the commercial-buyer ground truth and the only place to resolve the OSS-vs-commercial audience-split question called out in the falsification criteria.
3. **Manual Reddit-native searches** on r/LaMetric, r/tidbyt, r/awtrix for "layout" / "customize" / "wish" — Google's site: operator missed most of these; in-site search will catch more.
4. **Spot-check 5-10 actual Amazon 1-star reviews** for LaMetric Time and Tidbyt to validate the aggregator-reported themes with verbatim quotes.

### Explicit reminder
Per the task rules, I have **not** applied the falsification criteria, **not** written the Verdict section, and **not** drafted any epic. Data collection only. Synthesis is Christian's call after Day 2 Evening + Day 3 are in.

## Synthesis

- [ ] Tally all tags across Days 1-3
- [ ] Apply the decision rule in the Falsification Criteria table
- [ ] Note any audience-split pattern (OSS vs commercial JTBD divergence)
- [ ] Record verdict below
- [ ] Return to party mode or planning and draft the chosen direction's epic

## Verdict

_(Populate after Day 3 synthesis. Lock before drafting any epic.)_

- **Primary JTBD:**
- **Winning direction (J / C / M / hybrid):**
- **Audience split?** (y/n, describe)
- **Epic(s) to draft next:**

## Notes

- Party-mode transcript with John, Sally, Winston, Amelia, Mary is the source of the three candidate directions. Decisions here should cite specific evidence, not argument quality.
- Budget for this hunt: ~10 hours over 3 days. If it runs long, the scope is wrong — stop and reassess.
- This doc is the ONLY artifact to produce this week on the layout-editor question. No stories, no epics, no code until the verdict is written.
