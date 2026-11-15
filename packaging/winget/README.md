# Winget packaging (Doticca.Phostide)

Source of truth for [WinGet](https://learn.microsoft.com/windows/package-manager/) manifests before they are copied into a PR against [`microsoft/winget-pkgs`](https://github.com/microsoft/winget-pkgs).

Live install (after the community repo merges):

```powershell
winget install Doticca.Phostide
```

## Layout

```
packaging/winget/
  README.md
  Doticca.Phostide/
    <version>/
      Doticca.Phostide.yaml
      Doticca.Phostide.locale.en-US.yaml
      Doticca.Phostide.installer.yaml
```

In `winget-pkgs` the same three files live under:

`manifests/d/Doticca/Phostide/<version>/`

## Prerequisites

- [WinGet](https://aka.ms/getwinget) (App Installer)
- Optional: [wingetcreate](https://github.com/microsoft/winget-create) for updates / submit
- Local smoke tests need admin once: `winget settings --enable LocalManifestFiles`

## Validate locally

```powershell
winget validate --manifest packaging\winget\Doticca.Phostide\0.1.31
```

Optional install from the folder (close running `Phostide.exe` first — Inno `AppMutex` aborts silent setup):

```powershell
winget settings --enable LocalManifestFiles   # elevated, once
winget install --manifest packaging\winget\Doticca.Phostide\0.1.31 -h
```

Installer is per-user Inno (`PrivilegesRequired=lowest`). Winget passes `/VERYSILENT`; post-install launch uses `skipifsilent`, so a silent install should not start the tray app.

## First submit (new package)

1. Confirm the GitHub Release Setup URL and recompute SHA-256 of that exact file.
2. Author or adjust the three YAMLs under `Doticca.Phostide/<version>/`.
3. `winget validate --manifest ...` and preferably a local `--manifest` install.
4. Publish to `winget-pkgs` with either path below.

### Option A — wingetcreate (recommended)

```powershell
winget install Microsoft.WingetCreate
# PAT needs repo scope on your fork of microsoft/winget-pkgs
wingetcreate submit packaging\winget\Doticca.Phostide\0.1.31 -t <GITHUB_TOKEN>
```

Or generate from the Setup URL, then edit locale text before submit:

```powershell
wingetcreate new https://github.com/teamdoticca/Phostide/releases/download/v0.1.31/Phostide-Setup-0.1.31-win-x64.exe
```

### Option B — manual fork / PR

1. Fork `microsoft/winget-pkgs`.
2. Copy `packaging/winget/Doticca.Phostide/0.1.31/*` to `manifests/d/Doticca/Phostide/0.1.31/` on a branch.
3. Open a PR titled: `New package: Doticca.Phostide version 0.1.31`

PR body (reviewers):

```text
## New package: Doticca.Phostide 0.1.31

- Publisher: Doticca · Package: Phostide · License: MIT
- Installer: signed Inno Setup, per-user (PrivilegesRequired=lowest), x64
- URL: https://github.com/teamdoticca/Phostide/releases/download/v0.1.31/Phostide-Setup-0.1.31-win-x64.exe
- Site / docs: https://phostide.doticca.com

Phostide paints into Explorer’s real taskbar BackgroundFill (same ExplorerTAP / DesktopWindowXamlSource technique used by TranslucentTB). Signed with Doticca Trusted Signing. Restores taskbar chrome on Exit. Not an overlay window and does not require Windhawk.
```

Wait for automated validation. If PUA flags appear, reply with the signed-release context above.

## Updating an existing version

After a new GitHub Release:

```powershell
# Recompute hash of the published Setup.exe, then either edit YAMLs by hand or:
wingetcreate update Doticca.Phostide -u <SetupUrl> -v <ver> --submit
```

Example:

```powershell
wingetcreate update Doticca.Phostide `
  -u https://github.com/teamdoticca/Phostide/releases/download/v0.1.32/Phostide-Setup-0.1.32-win-x64.exe `
  -v 0.1.32 --submit
```

Also keep a copy under `packaging/winget/Doticca.Phostide/<ver>/` in this repo so the next agent has a source of truth.

## Manifest field reminders

| Field | Value |
| --- | --- |
| PackageIdentifier | `Doticca.Phostide` |
| InstallerType | `inno` |
| Scope | `user` |
| Architecture | `x64` |
| ProductCode | `{A7C3E9F1-4B2D-4E8A-9C1F-6D5E0B8A2F31}_is1` (Inno AppId) |
| Homepage | https://phostide.doticca.com |
