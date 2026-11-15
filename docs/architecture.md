# Phostide architecture

## Product facts for readers and agents

- Standalone Windows 11 utility (MIT). **No Windhawk**, no separate overlay tool, no admin for Setup.exe.
- Exactly five built-in themes (Rainbow, Sea Waves, Weather, Space, Aurora). **No** end-user theme editor in the shipped app.
- Paints Explorer `BackgroundFill` — not an overlay, not auto-hide. Quit restores stock brushes.
- Settings: **Taskbar** (theme/intensity), **OLED Care**, **Monitors** (on/off), **System** (shared Auto/GPU/CPU rendering).
- Public briefs: [llms.txt](https://phostide.doticca.com/llms.txt), [FAQ](https://phostide.doticca.com/docs/faq).
- OLED Care XAML whitelist: [oled-care-xaml.md](oled-care-xaml.md).

## Goal

Paint the real Windows 11 taskbar background (`Rectangle#BackgroundFill`) with OLED-safe animated themes so static taskbar pixels do not burn in, and optionally idle-dim residual chrome.

## Path

1. `Phostide.exe` (tray host) injects `ExplorerTAP.dll` into Explorer via a window hook + XAML Diagnostics TAP.
2. TAP finds each taskbar’s `BackgroundFill` shape and remembers the original brush.
3. Effect engine renders into a `CompositionDrawingSurface` and assigns a `CompositionSurfaceBrush` as the Fill.
4. Host polls OLED Care per display: recent `GetLastInputInfo` is attributed to the cursor’s and foreground window’s monitors; idle displays get lower fill intensity and per-slot `careActive` (ThemeCommand v6). Chrome dim / clock nudge apply only on idle trays.
5. Optional `ChromeCareService` dims whitelisted icons/tray/clock elements and can micro-nudge the clock host.
6. On host exit/crash, TAP restores original brushes and chrome opacity/transforms.

## Multi-monitor

Settings has four tabs:

| Tab | Scope |
|-----|--------|
| **Taskbar** | Apply to **All** or monitor *N*: theme + intensity (preview, then Apply) |
| **OLED Care** | Idle fill dim, optional experimental chrome/clock care, night boost, pause when locked |
| **Monitors** | Per-display on/off (default all on). Off → stock Windows bar |
| **System** | App-global: Auto/GPU/CPU rendering, Start with Windows, updates |

Host builds ThemeCommand **v6** slots (`deviceHash`, theme, intensity, enabled, `careActive`) plus shared `renderMode` and OLED Care opacities/`careFlags`. Top-level `careActive` is the OR of slots. TAP binds each `BackgroundFill` to its tray HWND (`Shell_TrayWnd` / `Shell_SecondaryTrayWnd`), hashes the monitor device name, and paints or unbinds from the matching slot — so identical resolutions still get distinct looks.

## Rendering

Settings → **System** exposes **Auto / GPU / CPU** for every bar. Auto prefers D3D11 GPU paint (`Themes.hlsl`) and falls back to CPU. GPU is smoother; CPU is the safe fallback when D3D11 cannot start.

**ThemeLab** (`ThemeLab.exe`) compiles and previews `Themes.hlsl` without injecting into Explorer — use it for GPU theme iteration. Live taskbar verification still goes through `Phostide.exe` + TAP. ThemeLab is a developer tool; it is not an end-user theme editor in the product UI.

## Themes

Procedural effects implementing `ITaskbarEffect`: Rainbow, SeaWaves, Weather, Space, Aurora.

## Site

`site/` is a Next.js landing at [phostide.doticca.com](https://phostide.doticca.com) (Vercel). Download CTA points at the latest GitHub Release asset.
