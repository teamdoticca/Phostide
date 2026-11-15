# OLED Care — Win11 taskbar XAML whitelist (spike)

Phostide’s TAP historically paints only `Rectangle#BackgroundFill`. Idle Chrome Dim / Clock Care need a **shallow** whitelist of shell-owned elements. Names below come from public Windows 11 taskbar styling references (Windhawk / RamenSoftware guide) and are treated as best-effort — OS updates may rename them.

Idle is hybrid: classic session `GetLastInputInfo` AFK dims all bars; while the session is active, attendance follows the **cursor monitor only** so an unused secondary can idle alone (a leftover focused window on another display must not keep that bar awake). Leaving full AFK wakes every bar on the next poll (~200 ms while dimmed). Fill intensity and chrome dim follow per-slot `careActive` (ThemeCommand v6) — chrome maps to a tray via hit-test HWND or shared `XamlRoot` with that bar’s `BackgroundFill` (no global `careActive` OR fallback).

## Containers / buttons

| Match | How | Role |
|-------|-----|------|
| `BackgroundFill` | `x:Name` + `Shapes.Rectangle` | Theme fill (existing) |
| `Taskbar.TaskListButton` | type contains | Pinned / running app buttons |
| `Taskbar.ExperienceToggleButton` | type ends-with | Start / Widgets / Task view toggles |
| `Taskbar.SearchBoxButton` / `SearchUx.SearchUI.SearchBoxButton` / `SearchPillButton` / `SearchIconButton` | type ends-with | Taskbar Search pill (must dim with chrome care) |
| `SearchBoxButton` / `SearchBox` | `x:Name` | Search host name fallback |
| `LaunchListButton` | `x:Name` | Start button host |
| `SystemTray.SystemTrayFrame` | type ends-with | Notification area root |
| `SystemTrayFrameGrid` | `x:Name` | Tray layout grid |

## Clock / date

| Match | How | Role |
|-------|-----|------|
| `TimeInnerTextBlock` | `x:Name` | Clock time digits |
| `DateInnerTextBlock` | `x:Name` | Date line |
| Parent of time/date (≤6 ancestors) | walk | Clock **nudge** target (transform) |

Win10-era `ClockButton` is **not** used on the XAML taskbar (22H2+).

## Safety rules

- No deep unbounded recursion from arbitrary roots (Explorer instability).
- Missing names → graceful no-op; chrome care stays optional / experimental.
- Always store original `Opacity` / `RenderTransform` and restore on care-off, unbind, and Exit.
- Do not replace or hide the clock; dim / micro-nudge only while idle care is active.

## Verification

After inject, `%TEMP%\Phostide\tap.log` should show `Chrome registered …` lines when OLED Care → Chrome Dim is on and elements are found.
