# Phostide site: marketing review and redesign brief

Reviewed 2026-11-22 against the live site (https://phostide.doticca.com, v0.1.37), the repo docs, and a competitor scan. This brief is for the UI/UX agent that will implement it. Read `site/PRODUCT.md`, `site/DESIGN.md` and `/AGENTS.md` first. Every product fact below matches them; **do not add claims they do not support**.

### Status (2026-11-22)

**P1 done (2026-11-24)** except WinGet:
- **Home page.** New sections: Problem (static-light illustration, ~2,900 h/yr), Trade-off (hide / dim / cover, then Phostide), Multi-monitor (three-screen illustration plus Monitors tab and status glossary), Trust (six facts, SmartScreen note, "What Phostide won't do") and a six-question FAQ with FAQPage JSON-LD (`src/lib/homeFaq.ts`). The old "Inside the bar" cards and Privacy chips are folded into Trust. S4 (a day with Phostide) is the hero.
- **Compare page.** Adds monitor Taskbar Detection and overlay apps. Has a ✓/◐/✕ matrix with the Phostide column highlighted, which becomes stacked cards on phones.
- **Kept aligned.** `/docs/faq` gained the firmware and laptop questions, and `llms.txt` / `llms-full.txt` were updated to match.
- **Not verified.** The PC Gamer article body could not be fetched, so the Problem section cites only the headline (a two-year, 6,000-hour test showing burn-in increasing). A taskbar-specific claim was not added.
- **WinGet is still blocked.** The winget-pkgs PR for 0.1.31 is still open.
- **Arm support was left unstated.** The installer allows `x64compatible`, so copy says only "x64".

**P0 committed in 38923a6:**
- New hero copy.
- The hero now walks through three moments (work, away, game) over a true-scale bar: a code editor while you work, the same screen when you step away, and a fullscreen game scene that covers the bar.
- Non-Windows visitors get a "Send link to my PC" CTA.
- New title, description, keywords, JSON-LD feature list and OG card. `scripts/build-icons.py` `compose_og` was updated as well.
- The default is now 35% everywhere, and "paused" is in the Monitors status lists.
- Nav: Install · Themes · OLED notes · Compare · Docs.
- Agent-brief links moved to the footer, and the FAQ subtitle was fixed.

**Corrections found while implementing:**
- **WinGet is not live.** `Doticca.Phostide` is not in `microsoft/winget-pkgs` (GitHub API 404), so the site does not advertise `winget install` yet. Add it once the PR merges.
- **The site already runs Vercel Web Analytics and Speed Insights** (`src/app/layout.tsx`). §7 is out of date on this point. The open owner decision is narrower: `/docs/privacy` should disclose this *site* analytics, kept separate from the app's no-telemetry promise.

---

## 0. Verdict in one paragraph

The product is unusually good for its category: it is the only tool that keeps the taskbar visible, moves its fill, dims it when you leave, dims unused displays on their own, and pauses for games, all for free, signed and open source. The site undersells it. The home page is short (four sections, about 2,400px), uses engineer vocabulary ("BackgroundFill", "D3D11", "hybrid idle Care", "AFK"), never shows the problem, never names the real alternatives (auto-hide and the monitor's own *Taskbar Detection*), and the hero shows the product as a 16px strip under a grey placeholder window. A visitor who arrives worried about burn-in leaves without seeing *why the taskbar is the risk*, *why the usual fixes hurt*, or *proof that this is safe to install*. The direction: **lead with the fear, show the trade-off, then show a calm product that removes it**, and make trust the second pillar of the page.

---

## 1. Market context the site must reflect

**Audience size is growing fast.** TrendForce counts 2.735M OLED monitors shipped in 2025 (+92% YoY), with about +51% forecast for 2026, and OLED laptops are on a similar curve (12.6M OLED notebooks in 2026). The top monitor brands are ASUS, Samsung, MSI, LG and Dell.

**The fear is specific and named.** In forums and guides, the taskbar is the static element owners mention most. Phrases people search: *"OLED burn in taskbar"*, *"hide taskbar OLED"*, *"auto hide taskbar OLED"*, *"dim taskbar OLED"*. The site uses none of them in headings.

**What owners do today, and why each one hurts:**

| Option | What it costs the user | Present on our Compare page? |
|---|---|---|
| Windows auto-hide (the default advice) | You lose the clock, tray and open apps. The animation is laggy on OLED (MS Tech Community thread). Awkward on multi-monitor. | Yes |
| Monitor firmware *Taskbar Detection* (ASUS OLED Care+, MSI OLED Care 2.0, Philips Taskbar Dimmer) | Only on some newer models. It dims a fixed region all the time, and it does not know when you're away or which screen you use. In Monitors Unboxed's long-term test, the MSI MPG 321URX still showed taskbar shadowing at about 21 months with it on. | **No. This is the biggest gap.** |
| Overlay dimmers (OLED Guard Pro at €19.99, AutoMonOffinator, AHK scripts) | They cover icons and fight z-order and notifications. Digital Trends found Guard Pro broke Snap Layouts. Some get VirusTotal hits. | Generic only |
| Windhawk mods (auto-hide when maximized) | You need Windhawk. They still auto-hide. There are injection and anti-cheat caveats. | Generic only |
| Theme tools (TranslucentTB, RainbowTaskbar) | Look only, no OLED story. RainbowTaskbar is the closest visually. | No |

**Warranties.** Dell/Alienware, MSI, Gigabyte, Corsair and most new ASUS models now cover burn-in for 3 years. Fear-only copy is weaker than it was in 2023. The better angles: *"keep the panel clean past the warranty"*, *"laptops and older panels have no such cover"*, and *"don't give up your taskbar to get there"*. Use these as angles only; never state warranty terms as fact on our site.

**Copy lesson from the category.** Reviewers punish "prevents burn-in" claims. Our honest "reduces how long the same pixels stay bright" voice is an asset, so keep it. OLED Guard Pro's line "Protect the pixels that stay still" is the best headline in the category. We need one at least that clear.

**Positioning statement** (internal, not copy):
> For Windows 11 OLED owners who won't give up a visible taskbar, Phostide is the free, open-source taskbar care that moves the bar while you work, rests it when you leave, and pauses it when you game. It works on any OLED, laptops and older panels included, alongside your monitor's own OLED Care, and it never covers the bar with an overlay.

**Unique, defensible claims** (all true per PRODUCT.md, so lean on them):
1. The taskbar **stays visible**: no auto-hide, no overlay on top of icons.
2. It **knows when you're away** and which **screens you're not using**. This is a big edge over firmware detection, which dims the same region all day.
3. **Games keep the GPU:** paint freezes on the fullscreen display only.
4. **Any OLED:** laptops, every monitor brand, no special firmware needed.
5. **Quit and it's gone:** stock bar restored, no system files touched.
6. **Free, MIT, signed, no admin, no account, no telemetry.**

---

## 2. Messaging

### 2.1 Vocabulary rules for the home page and marketing surfaces

| Don't say (home, hero, OG, store) | Say instead |
|---|---|
| BackgroundFill, Explorer BackgroundFill | "the taskbar's own background" |
| D3D11 GPU / CPU fallback | "runs on your GPU, falls back gracefully" (or omit) |
| hybrid idle Care, AFK | "dims when you step away" / "dims screens you're not using" |
| SoftSuspend, TAP, chrome, XAML | omit (docs only) |
| "moving fill" | "a slow, moving background" |
| "OLED Care" as an unexplained label | introduce it once: "OLED Care, the part that dims the bar when you're away" |

Technical words stay in `/docs/*`, where the audience wants them.

### 2.2 Hero copy

Current H1: *"Burn-in care for the Windows 11 taskbar on OLED screens."* It is accurate and good for SEO but flat, and "care" is vague.

Recommended:

- **Eyebrow** (keeps the keyword): `OLED taskbar burn-in protection for Windows 11 · Free & open source`
- **H1:** **Keep your taskbar. Spare your OLED.**
- **Sub:** "The taskbar is the one thing that never leaves your screen. Phostide keeps its background slowly moving while you work, dims it when you step away, and pauses it when you game, so the same pixels never sit bright for hours. Your icons stay put. Quit, and the normal taskbar is back."
- **Primary CTA:** `Download for Windows 11`, with small text under it: `Signed installer · no admin · 2-minute setup` (only keep "2-minute" if timing it confirms that).
- **Secondary:** a copyable `winget install Doticca.Phostide` chip (WinGet manifests already ship in `packaging/winget`, but the site never mentions it). Portable zip moves to the download section.

Alternative H1s to A/B test or choose from:
- "The taskbar never moves. Now its pixels do."
- "Your OLED's most static pixels, finally in motion."
- "Burn-in care that doesn't hide your taskbar."

### 2.3 The message pillars, in page order

1. **The problem:** the taskbar is the brightest thing that never moves.
2. **The trade-off:** today you hide it, dim it all day, or cover it.
3. **The answer:** it moves while you work, rests when you leave, and pauses when you play.
4. **Built for real desks:** only the OLED, unused screens dim on their own, games untouched.
5. **Safe to install:** no overlay, no capture, nothing left behind, open source.
6. **Honest limits:** what it doesn't do. This builds trust; keep it.

---

## 3. Redesign: the home page

Keep the true-black OLED ground: the audience is OLED owners, and a near-black page is on-brand. Make that point once in a footnote: "This page is true black, kind to your OLED." Move to **one idea per screen**, larger type, and far fewer bordered cards. Let the **theme light be the only colour on the page**, with ember for CTAs only. The current three-card grid and the four-chip privacy grid read as generic SaaS.

### S1 · Hero (rebuild the visual)

- **The problem:** the product is a 48px strip, and at hero scale it becomes a 16px sliver under a grey skeleton window (`HeroDisplay.tsx`). Nobody can see the motion or the dimming.
- **Fix:** a full-bleed **real desktop at true scale**: a real Windows 11 screenshot or photo, wallpaper dark, a real app window. Add a **magnifier callout** over the taskbar that shows the strip at 3–4×, with the live theme video inside it. On mobile, show only the magnified taskbar.
- Keep the theme chips and the Idle toggle, but make the demo self-explaining: auto-cycle *Working → Away (dims) → Back* every ~8s with a tiny state label ("Working", "Stepped away: OLED Care dims the bar"). Today the toggle is easy to miss and its result is subtle.
- Best upgrade: a 10–15s **real camera clip of an OLED monitor**, with a short macro of the taskbar moving. A real photo of a real panel is worth more than any render here. Use honest captures only (`DESIGN.md` already requires real captures).

### S2 · The problem (new)

- Headline: **"The one bright thing on your OLED that never moves."**
- Visual: a desktop silhouette with a "static-hours heat map". The taskbar glows hot, the rest stays cool. Label it as an illustration, not data.
- Simple maths you can state: 8 hours a day × 365 days ≈ **2,900 hours a year** of the same bar in the same place.
- One sourced line: in Monitors Unboxed's 2-year OLED test, taskbar shadowing appeared even with the monitor's taskbar detection on (link the PC Gamer or Tom's Hardware article). Keep it factual and linked.

### S3 · Today's trade-off (new; replaces part of /docs/compare on home)

- Three compact columns: **Hide it** (auto-hide), **Dim it all day** (monitor Taskbar Detection), **Cover it** (overlay apps). Each gets one line on what it costs.
- Then a fourth, highlighted: **Phostide: keep it, keep it moving.**
- End with a link: "Full comparison →".
- Tone: respectful. Say plainly that monitor OLED Care is good and that Phostide works **alongside** it. Never sound like we attack hardware vendors.

### S4 · How it works: a day with Phostide (replaces "Inside the bar")

- A horizontal timeline or scrubber with three states, each a looping strip:
  1. **While you work:** the background drifts slowly (theme video).
  2. **When you step away:** OLED Care dims the bar after 5 minutes (the default; configurable 1–30 minutes).
  3. **When you game:** fullscreen pauses the paint on that display only, and your GPU stays on the game.
- Optional fourth chip: **Night boost**, which dims sooner and darker in the evening.
- Do not use the words BackgroundFill or D3D11 here.

### S5 · Multi-monitor (new; the strongest differentiator, currently buried)

- Diagram: an OLED and an LCD side by side, with the cursor on one screen. The **other screen's bar dims on its own**. The LCD can stay a stock Windows bar.
- Copy: "Theme only your OLED. Screens you're not using rest on their own."
- Show the real **Monitors tab** screenshot (`/images/settings/settings-monitors.png`) with its live status (painting / fullscreen / locked / off / paused).

### S6 · Themes (bring back, but as a gallery, not a picker)

- Five strips at a true taskbar aspect ratio, full width, stacked, each autoplaying on view (with `prefers-reduced-motion` respected).
- Under each: one line plus two honest meters: **Darkness** and **Motion**. Space and Aurora are the darkest; Weather and Sea Waves are busier. Tag Space/Aurora "Best for everyday OLED".
- Say clearly that there are five built-in looks and no editor (see AGENTS.md).

### S7 · Inside the app (new)

- Four Settings tabs as a tabbed screenshot viewer using the real crops: Taskbar · OLED Care · Monitors · System.
- One line each. This proves it's a real, finished app; today the screenshots appear only deep in the docs.
- Mention the tray shortcuts: Pause for 1 hour, Pause until restart, Theme submenu. These matter to people who present and stream.

### S8 · Safe to install (rebuild the "Privacy" section into "Trust")

The second biggest objection after burn-in is *"is this sketchy?"*. The tool injects into Explorer, and the audience knows the Windhawk and anti-cheat discourse. Answer it head-on:

- **Not an overlay, no screen capture.** It changes only the taskbar's background brush.
- **Quit restores everything.** Stock brushes return, and no system files are modified.
- **Signed by Doticca SRL**, per-user install, no admin.
- **No account, no telemetry**; the only network use is the optional update check.
- **Open source (MIT).** Link the repo and the folder that holds the injected helper, and say in one sentence what it does.
- **Games:** it runs inside Explorer, not inside games, and pauses on fullscreen displays. ⚠️ Do **not** write "anti-cheat safe" unless the team has tested Vanguard, FACEIT, EAC and BattlEye. If tested, list exactly what was tested and when.
- **SmartScreen / antivirus:** a short honest note with a link to troubleshooting. If the team confirms clean VirusTotal results, link the report for each release.

### S9 · Honest limits (keep and promote; this is a brand asset)

Short "What Phostide won't do" list: it is not a burn-in warranty; it reduces static time, it doesn't make burn-in impossible; icons and the clock stay Windows UI unless experimental chrome care is on; Windows 11 22H2+ on x64 only.

### S10 · FAQ (new on home, 6 items, with FAQPage JSON-LD)

1. Will it slow my games? (fullscreen pause, GPU path)
2. Does it work with my monitor's own OLED Care / Taskbar Detection? (yes, it's complementary; both can run)
3. Does it work on OLED laptops? (yes, any Windows 11 x64 display)
4. What happens after a Windows update? (it restores on exit; experimental chrome care may no-op; link troubleshooting)
5. Does it hide my taskbar? (no)
6. Is it really free? (MIT, no account, no paid tier)

Only answer what the product facts support. For the games question, see the warning in S8.

### S11 · Final CTA

`Download for Windows 11`, with the winget chip, portable zip, requirements (Windows 11 22H2+, x64), the version and a link to release notes.

### Navigation

- Current: Docs · Install · Themes · OLED notes · Help & Privacy · GitHub · Download. The labels are inconsistent: "Help & Privacy" opens Troubleshooting, and "OLED notes" lights up on /docs/compare.
- Proposed: **How it works · Themes · Compare · Docs · GitHub · [Download]**. Put Install, FAQ, Troubleshooting and Privacy inside Docs and the footer.
- Show the GitHub star count only once it is meaningful; at 0 stars, hide it.

### Mobile

- People often discover the tool on a phone (Reddit, YouTube), where "Download Setup.exe" is a dead end.
- On non-Windows user agents, replace the primary CTA with **"Phostide is for Windows 11 PCs"** plus **Copy link** / **Share** (Web Share API). Add a secondary "Get it on your PC: phostide.doticca.com" line.
- Do not collect emails; that conflicts with the privacy stance.

---

## 4. Page-level comments

**Docs hub (`/docs`)**
- The hero says "Agent briefs: llms.txt · llms-full.txt". That line is for machines, so move it to the footer.
- The release/requirements card is good; keep it.

**Compare (`/docs/compare`)**
- Add a **"Monitor firmware: Taskbar Detection (ASUS / MSI / Philips)"** column. It is the alternative most buyers already own. Frame Phostide as complementary: firmware dims a fixed area all day, while Phostide adds motion, away-awareness, per-screen rest and a fullscreen pause, and also covers laptops and panels without that feature.
- Add a **"Paid overlay apps"** row, generic (no brand bashing): they cover the bar, and Phostide does not.
- Turn the 4-column table into a ✓ / ✕ / partial visual matrix with a highlighted Phostide column. Keep the row order identical on mobile (stacked cards).
- The title is good for SEO; keep it.

**OLED notes (`/docs/oled`)**
- Make this an SEO pillar: **"How to prevent OLED burn-in on Windows 11 (taskbar, icons, clock)"**. Give honest, tool-agnostic advice first (dark mode, brightness, auto-hide trade-off, clock seconds, panel refresh/pixel-clean cycles, screensaver or display-off timeouts, firmware care), then Phostide's role.
- Ranking for the generic query is the biggest organic lever. Be the most honest page on the subject.

**Themes (`/docs/themes`)**
- Says "Default intensity is 45%". README/AGENTS say **35%**. Fix after confirming in code (see §6).
- Add the Darkness/Motion meters from S6.

**FAQ (`/docs/faq`)**
- The subtitle "Short answers agents (and humans) need" is machine-first copy on a human page. Change it to "Short answers to common questions."
- Add the home FAQ items (monitor OLED Care, laptops, games). Add FAQPage JSON-LD.

**Install (`/docs/install`)**
- Add a **WinGet** method as Step 0 or an alternative tab.
- Add "Uninstall in one step" near the top. It reduces install anxiety.

**Troubleshooting**
- Strong and honest. Add a top-of-page "Copy diagnostics → open an issue" callout.

---

## 5. SEO, metadata and sharing

- **`<title>`**: "Phostide — OLED care for the real Windows taskbar" → **"Phostide: OLED taskbar burn-in protection for Windows 11 (free)"**.
- **Meta description**: remove "BackgroundFill" and "hybrid idle Care". Proposed: "Keep your Windows 11 taskbar visible and spare your OLED. Phostide slowly moves the taskbar background, dims it when you step away and pauses for games. Free, open source, signed."
- **OG image (`/og.png`)** still says **"Animated themes, painted into your real taskbar."** That is the old theming positioning, so redo it with the new H1 and the taskbar strip. Note: PRODUCT.md mentions `og-ai.jpg` for OG, but the live tag points to `og.png`, so pick one.
- **GitHub repo description** says "OLED-safe animated themes…". "OLED-safe" is an overclaim against our honest-limits voice. Change it to "Burn-in care for the Windows 11 taskbar on OLED: moving background, idle dimming, per-monitor. Not an overlay." (This is a repo setting, not site code; flag it to the owner.)
- **JSON-LD**: add `SoftwareApplication` (`operatingSystem: Windows 11`, `applicationCategory: UtilitiesApplication`, `offers.price: 0`, `softwareVersion`, `downloadUrl`, `license`). Add `FAQPage` on FAQ and home. No `aggregateRating` until real ratings exist.
- **New content pages**, each solving a search intent honestly:
  - `/oled-taskbar-burn-in` (or the OLED notes pillar above)
  - `/docs/compare/auto-hide`, "Auto-hide vs Phostide on OLED"
  - `/docs/compare/monitor-taskbar-detection`, "Using Phostide with ASUS / MSI OLED Care"
  - `/docs/oled-laptops`, "OLED laptops: taskbar care without firmware features"
- Keep `llms.txt` / `llms-full.txt` aligned (AGENTS.md rule).

---

## 6. Consistency bugs to fix in the same pass

1. **Default intensity:** the site (Themes, FAQ, Troubleshooting) says **45%**, while README, AGENTS.md and llms.txt say **35%**. In the code, a fresh install uses 35% (`Settings::intensity = 35` in `src/Common/PhostideCommon.h`); 45% is only the old fallback for each monitor. Change the site to **35%** (fresh-install default with Aurora).
2. **Monitors live status:** the site lists "painting / fullscreen / locked / off" and omits **paused**, which AGENTS.md includes.
3. **Nav:** "Help & Privacy" points to Troubleshooting; "OLED notes" is the active item on `/docs/compare`.
4. **OG image tagline:** themes-first copy, now outdated (§5).
5. **Version prominence:** the hero pill shows "v0.1.37". There have been 37 releases, several on the same day (2026-11-21/22). To a newcomer this reads as "unstable beta". Either drop the version from the hero (keep it in the download block) or label it honestly ("Early release"). Plan a **1.0 launch moment** before outreach.
6. Fine print under the hero CTA is ~11px grey. Raise it to 13px or more.

---

## 7. Conversion and measurement

- **One primary action per screen:** Download. Secondary actions (zip, winget, GitHub) are visually quieter.
- **Download click → thank-you state:** after the click, show a "Next: 3 steps" panel (run Setup → tray icon → pick your OLED in Welcome). Add a SmartScreen hint if relevant. This cuts drop-off between download and first run.
- **Measurement (decision for the owner):** today nobody can tell what the site converts. Release assets show single-digit downloads per version. Options: cookieless, privacy-friendly page analytics (Vercel Web Analytics or Plausible) counting only page views and download clicks, disclosed on the Privacy page as **site** analytics, separate from the app's no-telemetry promise. Alternatively, rely on GitHub release download counts only. Don't add anything without the owner's call; it touches the privacy brand.

---

## 8. Proof assets to produce (no faking)

- **No invented testimonials, ratings, user counts or benchmarks** (PRODUCT.md constraint).
- Produce real ones instead:
  - **A measured performance card:** CPU %, GPU %, RAM on a stated machine, GPU and CPU paths, with the method linked.
  - **A real camera clip** of an OLED monitor and an OLED laptop running Space/Aurora, plus the idle dim.
  - **A per-release VirusTotal link**, if clean.
  - The **signed-by** detail shown as a Windows properties screenshot.
  - Later, real quotes from the GitHub issues or Reddit launch thread, used with permission.

---

## 9. Off-site distribution (for the owner; informs site copy)

- **Microsoft Store listing:** it adds trust, eases SmartScreen friction and makes the tool discoverable. Add the Store badge to the site when it's live.
- **WinGet:** already packaged; publicise it on the site and README.
- **Launch post** on r/OLED_Gaming, r/Monitors and r/Windows11, with the 15s clip, honest limits and a "free, MIT, no account" line. The audience distrusts hype; our honest voice fits.
- **Outreach** to Monitors Unboxed / Hardware Unboxed (they run the long-term burn-in test), and to Neowin, gHacks, Windows Central and XDA tool roundups. Also list on AlternativeTo (alternative to auto-hide, TranslucentTB, OLED Guard Pro).
- Keep the README GIF and site hero in sync with the new message.

---

## 10. Priorities for the implementing agent

| P | Item | Section |
|---|---|---|
| P0 | Fix consistency bugs (intensity default, paused status, nav labels) | §6 |
| P0 | New hero copy + rebuilt hero visual (true-scale desktop + magnified taskbar + auto-cycling states) | §2.2, S1 |
| P0 | Mobile CTA for non-Windows visitors | §3 Mobile |
| P0 | Title, meta description, OG image, SoftwareApplication JSON-LD | §5 |
| P1 | New home sections: Problem, Trade-off, Day-with-Phostide, Multi-monitor | S2–S5 |
| P1 | Trust section rebuild + home FAQ with FAQPage schema | S8, S10 |
| P1 | Compare page: add the monitor firmware column + visual matrix | §4 |
| P1 | WinGet on home + install docs | §2.2, §4 |
| P2 | Themes gallery with Darkness/Motion meters, Settings tab viewer | S6, S7 |
| P2 | SEO pillar pages (OLED notes rewrite, laptops, firmware pairing) | §5 |
| P2 | Post-download "next steps" state | §7 |
| Owner | Analytics decision, anti-cheat testing, VirusTotal, Store listing, repo description, 1.0 launch | §5, §7, §8, §9 |

**Guardrails:** exactly five themes; no theme editor; not an overlay; not auto-hide; no Windhawk dependency; the OLED everyday preset is care-only; never "prevents burn-in" or "OLED-safe"; no invented proof. When copy changes, keep README, `site/public/llms.txt`, `llms-full.txt` and `/docs/faq` aligned (AGENTS.md).

---

### Sources (competitor and market scan)

- TrendForce OLED monitor shipments: https://trendforce.com/presscenter/news/20260326-12985.html
- OLED notebooks (TrendForce via TechPowerUp): https://www.techpowerup.com/354258/global-oled-notebook-shipments-forecast-to-grow-over-40-yoy-in-2027
- Monitors Unboxed long-term test: https://www.pcgamer.com/hardware/gaming-monitors/after-two-years-and-over-6-000-hours-monitors-unboxeds-long-term-oled-gaming-monitor-test-shows-increasing-burn-in
- ASUS OLED Care / Taskbar Detection: https://rog.asus.com/us/articles/gaming-monitors/how-to-use-asus-oled-care-to-protect-your-rog-oled-monitor/
- MSI OLED Care 2.0: https://www.msi.com/blog/how-msi-oled-care-prevents-oled-from-burn-in
- OLED Guard Pro (Steam): https://store.steampowered.com/app/4558060 · Digital Trends hands-on: https://digitaltrends.com/computing/oled-guard-pro-hands-on
- Auto-hide lag on OLED: https://techcommunity.microsoft.com/discussions/windows11/windows-11-auto-hide-taskbar-animation-is-laggy--low-fps-very-noticeable-on-oled/4475675
- Windhawk injection/anti-cheat note: https://github.com/ramensoftware/windhawk/wiki/Injection-targets-and-critical-system-processes
- TranslucentTB: https://github.com/TranslucentTB/TranslucentTB · RainbowTaskbar: https://github.com/ad2017gd/RainbowTaskbar
- Burn-in warranty roundup: https://www.tomshardware.com/monitors/asus-and-msi-compete-over-oled-monitor-burn-in-warranty-lengths-up-to-3-years-of-coverage

Not verified: Reddit sentiment (search returned no Reddit threads; spot-check before quoting), search volumes, LG's exact warranty terms.
