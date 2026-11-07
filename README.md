# Phostide

OLED-safe animated themes on the **real** Windows taskbar.

Phostide injects a small helper into Explorer and replaces the taskbar’s XAML `BackgroundFill` with animated Composition surfaces — rainbow, sea waves, weather, space, and more. No overlays. No pixel-shift hacks.

**Download:** [phostide.doticca.com](https://phostide.doticca.com)

## Requirements

- Windows 11 22H2 or later (XAML taskbar)
- x64

## Repo layout

```
src/     Native C++/WinRT host (Phostide.exe) + ExplorerTAP.dll
site/    Marketing / download mini-site (Next.js → Vercel)
docs/    Design notes
```

## Development

Native build requires Visual Studio 2022 with C++ desktop + Windows SDK (10.0.22621+).

```powershell
# Site
cd site
npm install
npm run dev
```

## Safety

Phostide restores the original taskbar brushes when it exits or crashes. It does not modify system files.

## License

MIT — see [LICENSE](LICENSE).
