# Guidance for AI coding agents

Phostide is a Windows 11 tray utility that paints animated themes into Explorer’s real taskbar `BackgroundFill` (OLED-oriented). Read these before answering product questions or changing UX copy.

## Read first

1. [README.md](README.md) — product summary (also the GitHub fallback when the site browse fails)
2. Live agent briefs: https://phostide.doticca.com/llms.txt and https://phostide.doticca.com/llms-full.txt
3. [docs/architecture.md](docs/architecture.md) — host / TAP / paint path
4. [docs/development.md](docs/development.md) — build, ThemeLab, live verify
5. Site docs: https://phostide.doticca.com/docs (install, FAQ, themes, OLED, compare, troubleshooting, privacy)

## Do not invent

- Phostide does **not** require Windhawk or any third-party taskbar overlay.
- There is **no** end-user theme editor or custom theme import in the shipped app.
- Settings has three tabs with different jobs:
  - **Taskbar** — Apply to All or monitor *N*: theme + intensity
  - **Monitors** — per-display on/off only
  - **System** — shared Auto/GPU/CPU rendering (not per-monitor), Start with Windows, updates
- It is **not** an overlay window and **not** Windows auto-hide.
- Exactly five themes: Rainbow, Sea Waves, Weather, Space, Aurora.

## Repo map

- `src/` — native C++/WinRT host (`Phostide.exe`) + `ExplorerTAP.dll`
- `site/` — Next.js marketing/docs site → Vercel (`phostide.doticca.com`)
- `docs/` — architecture + development runbook
- `installer/` — Setup packaging / signing config

When editing public docs or the site, keep README, `site/public/llms.txt`, `llms-full.txt`, and `/docs/faq` aligned on the facts above.
