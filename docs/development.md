# Phostide development runbook

How agents rebuild ExplorerTAP themes and verify them on a live Win11 taskbar.
Product SoT for architecture: [docs/architecture.md](architecture.md).

## Product vs agent inject

**Normal product start (what users get):** launch `out\Release\Phostide.exe`.
Host `InjectExplorerTap` finds `Shell_TrayWnd`, loads `ExplorerTAP.dll` via `WH_CALLWNDPROC`, waits for `Local\Phostide.TAP.Ready`.
**It does not kill or restart Explorer.**

**GPU theme iteration (preferred):** use `out\Release\ThemeLab.exe`. It compiles `src\ExplorerTAP\Themes.hlsl` directly (F5 or auto-reload on save). **Do not restart Explorer** just because ThemeLab was killed/relaunched — ThemeLab never loads into Explorer.

**Live taskbar / TAP DLL iteration:** Explorer must often be restarted so the old DLL unloads and XAML Diagnostics (`ixde`) can rebind cleanly. That restart is a **dev ritual only**, not product behavior — and only needed when verifying the in-Explorer paint path, not ThemeLab.

## Theme indices (`ThemeId`)

| Index | Theme | Notes |
|------:|-------|-------|
| 0 | Rainbow | Fixed ROYGBIV across thickness + atmospheric life (no hue marquee) |
| 1 | Sea Waves | Gerstner full-bar (deep teal / cyan foam toward marketing preview) |
| 2 | Weather | Storm loop (slanted rain + lightning) |
| 3 | Space | Forward cinematic nebula → stars (no rewind / no start fade-in) |
| 4 | Aurora | Green→teal→violet curtains, standing undulation |

Settings file: `%APPDATA%\Phostide\settings.json` (`theme`, `intensity`, `enabled`, …).

Paint entry points live in `src/ExplorerTAP/PixelThemes.cpp` (`PaintRainbow`, `PaintSeaWavesGerstner`, `PaintWeather`, `PaintSpace`, `PaintAurora`). Dispatch: `PaintThemePixels`.

## Build

```powershell
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
  -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" |
  Select-Object -First 1
& $msbuild "Z:\Doticca\Phostide\src\Phostide.sln" /p:Configuration=Release /p:Platform=x64 /m:1 /v:minimal
```

Outputs: `out\Release\ExplorerTAP.dll`, `out\Release\Phostide.exe`.

## Dev reinject ritual (fresh DLL)

Use when a new paint landed and Bound must be proven. Prefer this over guessing from a stale session.

1. Stop Phostide: `Get-Process Phostide -EA SilentlyContinue | Stop-Process -Force`
2. Restart Explorer (dev only):
   - `taskkill /F /IM explorer.exe`
   - wait until gone; `Start-Process explorer.exe`
3. Wait for `Shell_TrayWnd` (`FindWindow`), then **settle ~10–12 seconds** more.
   - Rushing causes `ixde hr=0x80070490` → `ixde gave up`.
4. Ensure settings theme/intensity as needed under `%APPDATA%\Phostide\settings.json`.
5. Copy DLL (must succeed):
   - `Copy-Item out\Release\ExplorerTAP.dll $env:TEMP\Phostide\ExplorerTAP.dll -Force`
6. **Clear** `%TEMP%\Phostide\tap.log` before launch (stale “Bound fill” lines false-positive).
7. Start `out\Release\Phostide.exe`.
8. Poll `%TEMP%\Phostide\tap.log` (**Unicode**) for:
   - `ixde succeeded`
   - `Bound fill <W>x<H>` with real size (e.g. `2560x48`, not `8x8` alone)
9. Confirm event: `Local\Phostide.TAP.Bound` is **SET** (`OpenEvent` + signaled).

Also useful: `%TEMP%\Phostide\inject.log` (`copied dll`, `hook ok`, `ready wait=…`, `copy failed`).

### Log path pitfall

TAP logs live under **`%TEMP%\Phostide\`**, not `%TEMP%\tap.log`. Searching the wrong path looks “empty” while inject is fine.

### Soft Bound note

A later `Bound fill 8x8` can appear after a good `2560x48` bind; if `Local\Phostide.TAP.Bound` stays SET, treat the large bind as success.

## Theme paint lessons (do not regress)

- **No sideways hue/star marquee** — feels like a ticker, not atmosphere (Rainbow / Space).
- **Snow-like vertical particle fall** is wrong for Space; use radial / toward-camera motion.
- **Cinematic loops:** forward-only; no camera rewind; no fade-in at phase wrap.
- **Full-bar every-pixel paint** for OLED anti burn-in.
- Pass `TaskbarEdge` for edge-aware thickness mapping.
- Calm time scale often `tCalm = t * ~0.18–0.22`.

## Theme paint note

Sea Waves and Space were unlocked to match marketing preview JPGs (`site/public/themes/*.jpg`). Prefer retuning toward those targets over inventing a new look; keep Gerstner + forward cinematic loop constraints.

## Quick “theme=N live?” check

```powershell
Get-Content "$env:APPDATA\Phostide\settings.json"
Get-Content "$env:TEMP\Phostide\tap.log" -Tail 15 -Encoding Unicode
# Bound event SET?
```

Then visually confirm the taskbar theme.
