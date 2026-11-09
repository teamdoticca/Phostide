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

Use `/themes/*.png` — real `PaintThemePixels` captures at 1920×48. Render with `ThemeStrip` (full-bleed strip, no fake moodboard chrome).

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
