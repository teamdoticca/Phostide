# Phostide architecture

## Goal

Paint the real Windows 11 taskbar background (`Rectangle#BackgroundFill`) with OLED-safe animated themes so static taskbar pixels do not burn in.

## Path

1. `Phostide.exe` (tray host) injects `ExplorerTAP.dll` into Explorer via a window hook + XAML Diagnostics TAP.
2. TAP finds each taskbar’s `BackgroundFill` shape and remembers the original brush.
3. Effect engine renders into a `CompositionDrawingSurface` and assigns a `CompositionSurfaceBrush` as the Fill.
4. On host exit/crash, TAP restores original brushes.

## Rendering

Settings expose **Auto / GPU / CPU**. Auto prefers D3D11 GPU paint (`Themes.hlsl`) and falls back to CPU. GPU is smoother; CPU is the safe fallback when D3D11 cannot start.

**ThemeLab** (`ThemeLab.exe`) compiles and previews `Themes.hlsl` without injecting into Explorer — use it for GPU theme iteration. Live taskbar verification still goes through `Phostide.exe` + TAP.

## Themes

Procedural effects implementing `ITaskbarEffect`: Rainbow, SeaWaves, Weather, Space, Aurora.

## Site

`site/` is a Next.js landing at [phostide.doticca.com](https://phostide.doticca.com) (Vercel). Download CTA points at the latest GitHub Release asset.
