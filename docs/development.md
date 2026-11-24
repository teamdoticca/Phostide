# Phostide development runbook

How to rebuild ExplorerTAP themes and verify them on a live Win11 taskbar.
Product architecture: [docs/architecture.md](architecture.md).

## Product vs developer inject

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

Settings file: `%APPDATA%\Phostide\settings.json` (`theme` / `intensity` fallbacks, app-global `renderMode`, OLED Care fields including `pauseWhenLocked` / `pauseWhenFullscreen`, `monitorSettings`, `disabledMonitors`, `enabled`, …). ThemeCommand **v7** sends per-monitor paint slots (theme, intensity, enabled, `careActive`, `pauseActive`), shared `renderMode`, and OLED Care opacities/`careFlags`. Idle is **hybrid**: session AFK (`GetLastInputInfo`) dims all; while active, cursor / focused-window attendance can idle a secondary alone. Fullscreen pause (default on) sets per-slot `pauseActive` for covered monitors; session lock still uses global `paused`. Settings › Monitors shows live status (`painting` / `fullscreen` / `locked` / `off` / `paused`). SoftSuspend keeps fill refs across host Enable/stop; watch-mutex must `ReleaseMutex` or the next `Phostide.exe` exits on `ERROR_ALREADY_EXISTS`. XAML whitelist: [oled-care-xaml.md](oled-care-xaml.md). Settings screenshots: crop the Settings client (~720×752 PNG) into `site/public/images/settings/` and mirror under `assets/readme/settings/` (see `site/DESIGN.md`).

Multi-monitor verify: after reinject, `%TEMP%\Phostide\tap.log` should show two `Bound fill … hash=… hwnd=…` lines with **different** hashes, then `slot[0]` / `slot[1]` with matching hashes and distinct theme/intensity when Settings differ per display.

Paint entry points live in `src/ExplorerTAP/PixelThemes.cpp` (`PaintRainbow`, `PaintSeaWavesGerstner`, `PaintWeather`, `PaintSpace`, `PaintAurora`). Dispatch: `PaintThemePixels`.

## Build

From the repo root:

```powershell
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
  -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" |
  Select-Object -First 1
& $msbuild "src\Phostide.sln" /p:Configuration=Release /p:Platform=x64 /m:1 /v:minimal
```

Outputs: `out\Release\ExplorerTAP.dll`, `out\Release\Phostide.exe`, `out\Release\ThemeLab.exe`, `out\Release\PhostideTests.exe`.

Unit tests: run `out\Release\PhostideTests.exe` (exit code 0 = all pass). CI (`.github/workflows/ci.yml`) runs the same build + tests and the site lint/build.

## First-run Welcome

With no `settings.json`, the host saves the defaults (Aurora 35%, OLED Care idle 5 min) and, once inject finishes, opens Settings on a Welcome page (no tab row): "Which displays are OLED?" reusing the Monitors rows. **Done** applies and closes; **Customize** applies and continues on the Taskbar tab. To test it without losing your own settings, start the host with `PHOSTIDE_SETTINGS_DIR` pointing at an empty folder.

## In-app Update now

Settings › System › **Update now** (`UpdateInstall.cpp`) only downloads `Phostide-Setup-*.exe` from `https://github.com/teamdoticca/Phostide/releases/download/`, into `%TEMP%\Phostide\update`. It accepts the file only if its Authenticode signer equals the host's (an unsigned dev host requires `CN=Doticca SRL`) and its file version is newer than `kAppVersion`. Then it runs Setup with `/SILENT /SUPPRESSMSGBOXES /NORESTART /RELAUNCH=1 /LOG="%TEMP%\Phostide\setup.log"` and the host exits. `/RELAUNCH=1` makes Setup start Phostide at `ssDone` (silent mode skips the postinstall `[Run]` entry). Any failure opens the release page. Testing the download against a real release from a dev build needs a build that reports an older version than that release.

Scoop installs (`…\apps\phostide\<version|current>\Phostide.exe`, see `IsScoopInstallPath`) never run Setup: Update now shows "update with: scoop update phostide" instead, so Scoop and Setup copies never mix.

## Scoop bucket

Source: `packaging/scoop/` (README + `bucket/phostide.json`), published as the `teamdoticca/scoop-bucket` repo. The manifest installs the signed portable zip; `pre_uninstall` runs `Phostide.exe /exit` (and on uninstall drops a `Run\Phostide` value that points into the Scoop dir), `post_install` restarts Phostide after an update that stopped it. After promoting a release: `.\scripts\update-scoop-manifest.ps1 -Version x.y.z` (downloads the release zip, rewrites version/url/hash), then copy `bucket/phostide.json` to the bucket repo.

`scripts/package-release.ps1 -InstallerOnly` rebuilds and signs just Setup from the existing signed stage (installer script fixes).

## Dev reinject ritual (fresh DLL)

Use when a new paint landed and Bound must be proven. Prefer this over guessing from a stale session.

1. Stop Phostide: `Get-Process Phostide -EA SilentlyContinue | Stop-Process -Force`
2. Restart Explorer (dev only):
   - `taskkill /F /IM explorer.exe`
   - wait until gone; `Start-Process explorer.exe`
3. Wait for `Shell_TrayWnd` (`FindWindow`), then **settle ~15 seconds** more (product bounce uses 15 s).
   - Rushing causes `ixde hr=0x80070490` → `ixde gave up` (can stick for that Explorer process lifetime — bounce again).
   - Prefer `Stop-Process` then a clean start, or tray Exit / `Phostide.exe /exit`, so the SingleInstance mutex is released; after Force-kill, confirm the next host actually stays up.
4. Ensure settings theme/intensity as needed under `%APPDATA%\Phostide\settings.json`.
5. The host copies the DLL itself to `%LOCALAPPDATA%\Phostide\tap\ExplorerTAP-<version>.dll` (+ `Themes.hlsl`). If Explorer still maps that file the copy is reused only when sizes match — bounce Explorer (step 2) after rebuilding the TAP at the same version.
   - A **signed** `Phostide.exe` refuses to inject a TAP that is not signed by the same publisher (`inject.log`: `TAP signature …`). Unsigned dev builds skip the check.
6. **Clear** `%TEMP%\Phostide\tap.log` before launch (stale “Bound fill” lines false-positive).
7. Start `out\Release\Phostide.exe`.
8. Poll `%TEMP%\Phostide\tap.log` (**Unicode**) for:
   - `ixde succeeded`
   - `Bound fill <W>x<H>` with real size (e.g. `2560x48`, not `8x8` alone)
9. Confirm event: `Local\Phostide.TAP.Bound` is **SET** (`OpenEvent` + signaled).

Also useful: `%TEMP%\Phostide\inject.log` (`copied versioned dll`, `TAP signature ok`, `hook ok`, `ready wait=…`, `copy failed`). Logs and `tap.version` stay under `%TEMP%\Phostide\`; only the loaded DLL copies moved to `%LOCALAPPDATA%`.

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

Sea Waves and Space were unlocked to match marketing preview assets (`site/public/themes/*.png` / `*.webm`). Prefer retuning toward those targets over inventing a new look; keep Gerstner + forward cinematic loop constraints.

## Quick “theme=N live?” check

```powershell
Get-Content "$env:APPDATA\Phostide\settings.json"
Get-Content "$env:TEMP\Phostide\tap.log" -Tail 15 -Encoding Unicode
# Bound event SET?
```

Then visually confirm the taskbar theme.
