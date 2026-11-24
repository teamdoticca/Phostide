# Phostide architecture

## Product facts for readers and agents

- Standalone Windows 11 **OLED taskbar solution** (MIT). **No Windhawk**, no separate overlay tool, no admin for Setup.exe.
- Moving fill while you work + hybrid idle Care when you leave + per-monitor control. Themes are the look layer; OLED Care is the idle layer.
- Exactly five built-in themes (Rainbow, Sea Waves, Weather, Space, Aurora). **No** end-user theme editor in the shipped app.
- Paints Explorer `BackgroundFill` — not an overlay, not auto-hide. Quit restores stock brushes.
- Settings: **Taskbar** (theme/intensity), **OLED Care**, **Monitors** (on/off, EDID model names + Identify, live paint/pause status), **System** (shared Auto/GPU/CPU rendering).
- Public briefs: [llms.txt](https://phostide.doticca.com/llms.txt), [FAQ](https://phostide.doticca.com/docs/faq).
- OLED Care XAML whitelist: [oled-care-xaml.md](oled-care-xaml.md).

## Goal

Paint the real Windows 11 taskbar background (`Rectangle#BackgroundFill`) with OLED-safe animated themes so static taskbar pixels do not burn in, and optionally idle-dim residual chrome.

## Path

1. `Phostide.exe` (tray host) injects `ExplorerTAP.dll` into Explorer via a window hook + XAML Diagnostics TAP.
2. TAP finds each taskbar’s `BackgroundFill` shape and remembers the original brush.
3. Effect engine renders into a `CompositionDrawingSurface` and assigns a `CompositionSurfaceBrush` as the Fill.
4. Host polls OLED Care with **hybrid idle**: classic session `GetLastInputInfo` AFK dims every bar; while the session is active, each poll attributes only the cursor monitor so an unused secondary can idle alone. Idle slots get lower fill intensity and per-slot `careActive` (ThemeCommand v6+). Leaving full AFK wakes every bar on the next care poll (~200 ms while dimmed).
5. Host also polls foreground fullscreen: a window that covers a monitor’s `rcMonitor` sets that slot’s `pauseActive` (ThemeCommand **v7**) so TAP freezes paint on the covered bar only; session lock still sets global `paused`. SoftSuspend (tray Enable) stays separate.
6. Optional `ChromeCareService` dims whitelisted icons/tray/clock (incl. Search pill) and can micro-nudge the clock host — only on idle trays.
7. Tray Enable / host stop uses **SoftSuspend**: restore stock fills but keep `BackgroundFill` refs so the next host can Bind again. TAP watches `Local\Phostide.SingleInstance` and must `ReleaseMutex` after Wait so a killed host can restart. Full Exit restores brushes + chrome.
8. Version-stamped TAP upgrades bounce Explorer only when a TAP is still mapped into the running shell (`Local\Phostide.TAP.Ready` exists — the TAP holds it for Explorer's lifetime) **and** `tap.version` disagrees. Setup (`ssPostInstall` / uninstall) and the host use the same rule, so a fresh shell (first install, after reboot, after Setup's own bounce) is never bounced twice. After a bounce the host waits ~15 s for XAML Diagnostics and retries inject if Bound never signals. Host Exit mid-bounce relaunches Explorer (`FinishPendingExplorerRestart`); Setup also ensures `Shell_TrayWnd` exists before finishing.

## Multi-monitor

Settings has four tabs:

| Tab | Scope |
|-----|--------|
| **Taskbar** | Apply to **All** or monitor *N*: theme + intensity (preview, then Apply) |
| **OLED Care** | Hybrid idle fill dim, optional experimental chrome/clock care, night boost, pause when locked, pause in fullscreen (default on — covered display only); everyday preset is care-only |
| **Monitors** | Per-display on/off (default all on), rows named from the EDID model (DisplayConfig target names, host only); **Identify** shows a numbered card on each display for 3 s. Off → stock Windows bar. Live status: painting / fullscreen / locked / off / paused |
| **System** | App-global: Auto/GPU/CPU rendering, Start with Windows, updates |

Host builds ThemeCommand **v7** slots (`deviceHash`, theme, intensity, enabled, `careActive`, `pauseActive`) plus shared `renderMode` and OLED Care opacities/`careFlags`. Top-level `careActive` is the OR of slots; global `paused` is session lock. TAP binds each `BackgroundFill` to its tray HWND (`Shell_TrayWnd` / `Shell_SecondaryTrayWnd`), hashes the monitor device name, and paints or unbinds from the matching slot — so identical resolutions still get distinct looks. Fullscreen-paused slots keep the last frame and drop out of the UI timer when every live slot is paused.

## Rendering

Settings → **System** exposes **Auto / GPU / CPU** for every bar. Auto prefers D3D11 GPU paint (`Themes.hlsl`) and falls back to CPU. GPU is smoother; CPU is the safe fallback when D3D11 cannot start.

**ThemeLab** (`ThemeLab.exe`) compiles and previews `Themes.hlsl` without injecting into Explorer — use it for GPU theme iteration. Live taskbar verification still goes through `Phostide.exe` + TAP. ThemeLab is a developer tool; it is not an end-user theme editor in the product UI.

## Themes

Procedural effects implementing `ITaskbarEffect`: Rainbow, SeaWaves, Weather, Space, Aurora.

## Site

`site/` is a Next.js landing at [phostide.doticca.com](https://phostide.doticca.com) (Vercel). Download CTA points at the latest GitHub Release asset.
