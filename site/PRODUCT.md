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

Themes live inside Explorer’s own fill via a TAP helper — icons stay on top; brushes restore on exit. Neighboring tools that overlay windows or only autohide the bar cannot truthfully claim this.

## Capabilities

- Themes: Rainbow, Sea Waves, Weather, Space, Aurora
- Tray host + ExplorerTAP injection
- Local settings; no account / no telemetry (as documented)
- Docs: install, themes, OLED, troubleshooting, privacy

## Constraints

- Landing is Persuade; `/docs/*` remains documentation (Read)
- Brand commitments: cyan/blue luminous wave on OLED black; do not use `/og-ai.jpg` (baked wordmark) as hero background — wave-only art for hero; keep og-ai for social OG if desired
- No invented customers, benchmarks, or pricing
- Download CTA points at public GitHub Releases for teamdoticca/Phostide
- MIT license; Windows 11 22H2+ x64

## Confirmed assets

- `assets/brand/` and `site/public/`: cyan-wave mark, tray chip icons, theme preview JPGs under `/themes/*.jpg`, `og-ai.jpg` for Open Graph
- Product name: Phostide

## Voice

Direct, technical-clear, calm confidence. No hype gamification.

## Open decisions

- Exact hero wave asset file (crop of wave-only master vs dedicated hero render) — decide during redesign build
