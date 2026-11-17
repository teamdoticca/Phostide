# Phostide site design

Port of the UX Pilot suite (page `UE6tIDfhvVybZY9Cmzx3`): black OLED ground, ember accent, Inter, honest taskbar PNG strips.

## Tokens

| Token | Value | Role |
| --- | --- | --- |
| `void` | `#000000` | Page ground |
| `panel` | `#0B0B0C` | Raised surfaces |
| `line` | `#1C1C1F` | Borders / dividers |
| `soft` | `#8A8A93` | Secondary text |
| `ember` | `#FF8A3D` | Accent / CTAs |
| `emberdim` | `#3A1E0C` | Ember wash behind recommended cards |

## Typography

- Sans: Inter (`--font-sans`)
- Mono: IBM Plex Mono (`--font-mono`) for paths

## Theme previews

Use `/themes/*.webm` (VP9, Native 48px from `PaintThemePixels` / ThemeLab) with `/themes/*.png` posters. Default clips ~12s @ 20fps; Space is a full ~35s film loop (nebula → stars → nebula). Regenerate via `scripts/export-theme-previews.ps1`. `ThemeStrip` plays on hover only; idle sheen cues the pointer.

## Settings screenshots

Real Settings client crops live under `/images/settings/settings-{taskbar,oled-care,monitors,system}.png` (mirrored in `assets/readme/settings/` for GitHub README). Capture from a running `Phostide.exe` Settings window: dark client only (~720×752), no OS chrome, PNG. Prefer Care + Monitors for pause/status docs; keep all four tabs in sync when the UI changes. `SettingsShot` renders them on install / OLED / FAQ. Local overview: `/images/settings/preview.html`.

## Screens mapped

| Route | UX Pilot screen |
| --- | --- |
| `/` | Home |
| `/docs` | Docs |
| `/docs/install` | Install |
| `/docs/themes` | Themes |
| `/docs/oled` | OLED Notes |
| `/docs/troubleshooting` (+ `#privacy`) | Troubleshooting |
| `/docs/privacy` | Privacy (standalone; also summarized on troubleshooting) |
