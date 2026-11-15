# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

## Stack

Next.js App Router under `site/`, deployed on Vercel project `phostide` (https://phostide.doticca.com).

## Users

Windows 11 desktop users with OLED (or burn-in-sensitive) displays who want animated taskbar themes without autohide or overlays. Secondary: enthusiasts evaluating a local MIT utility from Doticca.

## Product Purpose

Phostide paints OLED-safe animated themes into the real Windows 11 taskbar background fill (Explorer `BackgroundFill`), not a floating overlay. Success: visitor understands the mechanism, trusts safety, and downloads the latest GitHub Release.

## Positioning

Themes live inside Explorer’s own fill via a TAP helper — icons stay on top; brushes restore on exit. Neighboring tools that overlay windows or only autohide the bar cannot truthfully claim this. Phostide is standalone: no Windhawk or separate overlay required.

## Capabilities

- Themes: Rainbow, Sea Waves, Weather, Space, Aurora (exactly five; no end-user theme editor)
- Settings tabs: Taskbar (theme + intensity), OLED Care, Monitors (on/off), System (shared Auto/GPU/CPU rendering, Start with Windows, updates)
- OLED Care tab: hybrid idle (session AFK dims all; while active, unused displays can dim alone via cursor / focused window); optional experimental chrome dim + clock micro-nudge; night boost; OLED everyday preset is **care-only** (does not change Taskbar theme/intensity); applies only to Monitors-enabled bars
- SoftSuspend on tray Enable / host stop keeps `BackgroundFill` refs so the next host can rebind without a fragile rescan; Exit restores stock brushes
- Rendering: Auto / GPU / CPU (D3D11 Composition paint when available; CPU pixel fallback). `Themes.hlsl` ships with the app for the GPU path
- Tray host + ExplorerTAP injection
- Local settings; no account / no telemetry (as documented)
- Docs: install, FAQ, themes, OLED, compare, troubleshooting, privacy
- Agent briefs: `/llms.txt`, `/llms-full.txt`

## Constraints

- Landing is Persuade; `/docs/*` remains documentation (Read)
- Brand commitments: cyan/blue luminous wave on OLED black; do not use `/og-ai.jpg` (baked wordmark) as hero background — wave-only art for hero; keep og-ai for social OG if desired
- No invented customers, benchmarks, or pricing
- Download CTA points at public GitHub Releases for teamdoticca/Phostide
- MIT license; Windows 11 22H2+ x64; per-user Setup.exe (no admin)
- Do not claim Windhawk dependency, overlay mechanism, auto-hide, or a theme editor

## Confirmed assets

- `assets/brand/` and `site/public/`: Screen Tide mark (solid screen + white bezel + ember under wavy edge); tray/exe/setup ICOs from `scripts/build-icons.py`; theme preview JPGs under `/themes/*.jpg`; `og-ai.jpg` for Open Graph
- Product name: Phostide

## Voice

Direct, technical-clear, calm confidence. No hype gamification.
