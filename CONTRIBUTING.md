# Contributing to Phostide

Thanks for helping. Short path:

1. Read [docs/architecture.md](docs/architecture.md) for how the host, TAP, and paint path fit together.
2. Follow [docs/development.md](docs/development.md) to build, iterate themes (ThemeLab), and verify on a live taskbar.
3. Open an Issue to discuss larger changes, then send a Pull Request.

## Requirements

- Windows 11 22H2+ (x64) for live taskbar testing
- Visual Studio 2022 with C++ desktop workload and Windows SDK **10.0.22621+**
- Node.js 20+ for the marketing site under `site/`

## Native

From the repo root:

```powershell
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
  -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" |
  Select-Object -First 1
& $msbuild "src\Phostide.sln" /p:Configuration=Release /p:Platform=x64 /m:1 /v:minimal
```

Outputs land in `out\Release\` (`Phostide.exe`, `ExplorerTAP.dll`, `ThemeLab.exe`, …). Full reinject / log checks are in the [development runbook](docs/development.md).

## Site

```powershell
cd site
npm install
npm run dev
```

## Pull requests

- Keep changes focused; match existing style in the area you touch.
- For theme paint changes, note which `ThemeId` you verified and whether ThemeLab and/or live TAP were checked.
- Do not commit secrets, local absolute paths, or machine-specific ops scrap.
