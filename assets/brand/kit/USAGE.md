# Phostide logo kit

## Idea
Solid OLED screen with a white border (bezel). Ember tide sits **below** the wavy bottom edge and closes the parallelogram. No corona rings.

## Colour

| Role | Hex | Use |
| --- | --- | --- |
| Void / screen | `#111111` | Solid display body |
| Ember | `#FF8A3D` | Tide under the wave |
| Bezel | `#FFFFFF` | Border around the screen |
| On void screen lift | `#2A2A2E` | Screen body on pure black |

One-colour: use `phostide-mark-mono.svg` (closed silhouette).

## Files
- `phostide-mark.svg` — colour master
- `phostide-mark-small.svg` — 16–48px (thicker bezel)
- `phostide-mark-mono.svg` — black silhouette
- `phostide-horizontal.svg` / `phostide-stacked.svg` — lockups
- `phostide-on-void.svg` — OLED black ground
- `web/` — favicon set

## Ship everywhere
Run from repo root after editing the master SVG:

```powershell
python scripts/build-icons.py
```

That regenerates:
- `assets/brand/phostide.ico` + `setup-phostide.ico` + PNG sizes
- `src/Host/phostide.ico` (exe + tray via `Phostide.rc`)
- `site/public/` favicons, mark SVG, OG
- `site/src/app/` Next metadata icons

Then rebuild the Host (`Phostide.exe`) and re-run `scripts/package-release.ps1` so Setup.exe picks up `setup-phostide.ico`.

## Clear space
Keep clear space ≥ half the bezel stroke on all sides.

## Minimum sizes
- Colour with bezel: **16px** (use small cut)
- Mono silhouette: **12px**

## Don’t
- Put ember inside the screen
- Replace the wavy edge with colour
- Reintroduce corona rings unless intentionally revised
- Stretch or add drop shadows
