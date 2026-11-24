# Doticca Scoop bucket

[Scoop](https://scoop.sh) bucket for [Phostide](https://phostide.doticca.com), the OLED taskbar solution for Windows 11.

```powershell
scoop bucket add doticca https://github.com/teamdoticca/scoop-bucket
scoop install doticca/phostide
```

Start Phostide from **Start menu › Scoop Apps › Phostide**; it lives in the tray. Update with `scoop update phostide` (Settings › System › Update now points there too). `scoop uninstall phostide` exits Phostide, restores the stock taskbar and removes its Start with Windows entry.

The package is the signed portable zip from [GitHub Releases](https://github.com/teamdoticca/Phostide/releases). Settings stay in `%APPDATA%\Phostide`, so they survive updates and reinstalls.

---

Maintainers: this folder is the source of the bucket repo. After promoting a release, run `scripts\update-scoop-manifest.ps1 -Version x.y.z` in the Phostide repo and copy `bucket\phostide.json` into the bucket repo.
