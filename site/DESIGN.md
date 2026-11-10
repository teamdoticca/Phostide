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

Use `/themes/*.webm` (VP9, ~6s @ 20fps Native 48px from `PaintThemePixels` / ThemeLab) with `/themes/*.png` posters. Regenerate via `scripts/export-theme-previews.ps1`. Render with `ThemeStrip` (`<video>` + poster, full-bleed, no fake moodboard chrome).

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
