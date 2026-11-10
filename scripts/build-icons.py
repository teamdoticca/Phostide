#!/usr/bin/env python3
"""Build Phostide brand icons from the Screen Tide mark.

Mark: solid screen (#111) + white inset bezel + ember (#FF8A3D) under the wavy
edge. Sources: assets/brand/kit/phostide-mark.svg (+ -small for tiny sizes).

Writes tray/exe/setup ICOs, site favicons, Host phostide.ico, and OG card.
"""

from __future__ import annotations

import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont, ImageOps

ROOT = Path(__file__).resolve().parents[1]
BRAND = ROOT / "assets" / "brand"
KIT = BRAND / "kit"
SITE_PUBLIC = ROOT / "site" / "public"
SITE_APP = ROOT / "site" / "src" / "app"
HOST = ROOT / "src" / "Host"
THEME_STRIP = SITE_PUBLIC / "themes" / "aurora.png"

MARK_SVG = KIT / "phostide-mark.svg"
MARK_SMALL_SVG = KIT / "phostide-mark-small.svg"

# Optional: Cursor logo-design rasterizer (Chrome). Falls back to SVG→PNG via Pillow if absent.
RENDER_PNG = (
    Path.home()
    / ".cursor"
    / "skills"
    / "logo-design"
    / "scripts"
    / "render_png.py"
)

VOID = (0, 0, 0, 255)
EMBER = (255, 138, 61, 255)
INK = (244, 244, 245, 255)
SOFT = (138, 138, 147, 255)


def _font(size: int, *, bold: bool = True) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    candidates = [
        r"C:\Windows\Fonts\segoeuib.ttf" if bold else r"C:\Windows\Fonts\segoeui.ttf",
        r"C:\Windows\Fonts\arialbd.ttf" if bold else r"C:\Windows\Fonts\arial.ttf",
        r"C:\Windows\Fonts\arial.ttf",
    ]
    for path in candidates:
        try:
            return ImageFont.truetype(path, size)
        except OSError:
            continue
    return ImageFont.load_default()


def _rasterize_svg(svg: Path, size: int) -> Image.Image:
    """Rasterize SVG at exact size (transparent). Prefer render_png.py / Chrome."""
    if not svg.exists():
        raise FileNotFoundError(f"Missing mark SVG: {svg}")

    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / f"mark-{size}.png"
        if RENDER_PNG.exists():
            subprocess.run(
                [
                    sys.executable,
                    str(RENDER_PNG),
                    str(svg),
                    "-o",
                    str(out),
                    "--size",
                    str(size),
                ],
                check=True,
                capture_output=True,
            )
            return Image.open(out).convert("RGBA")

        # Fallback: cairosvg if installed
        try:
            import cairosvg  # type: ignore

            cairosvg.svg2png(
                url=str(svg),
                write_to=str(out),
                output_width=size,
                output_height=size,
            )
            return Image.open(out).convert("RGBA")
        except Exception as exc:  # noqa: BLE001
            raise RuntimeError(
                "Need logo-design render_png.py or cairosvg to rasterize the mark SVG"
            ) from exc


def make_mark(size: int) -> Image.Image:
    """Full mark for app / marketing sizes."""
    return _rasterize_svg(MARK_SVG, size)


def make_tray_mark(size: int) -> Image.Image:
    """Small-size cut (thicker bezel) for tray / favicon 16–48."""
    src = MARK_SMALL_SVG if MARK_SMALL_SVG.exists() else MARK_SVG
    return _rasterize_svg(src, size)


def write_png(path: Path, img: Image.Image) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    img.convert("RGBA").save(path, format="PNG", optimize=True)


def png_bytes(img: Image.Image) -> bytes:
    import io

    buf = io.BytesIO()
    img.convert("RGBA").save(buf, format="PNG", optimize=True)
    return buf.getvalue()


def _ico_dir_entry(width: int, height: int, offset: int, data_len: int) -> bytes:
    w = 0 if width >= 256 else width
    h = 0 if height >= 256 else height
    return struct.pack("<BBBBHHII", w, h, 0, 0, 1, 32, data_len, offset)


def build_ico(path: Path, sizes: list[int]) -> None:
    frames = [png_bytes(make_tray_mark(s) if s <= 48 else make_mark(s)) for s in sizes]
    count = len(frames)
    offset = 6 + 16 * count
    entries = bytearray()
    blob = bytearray()
    for s, data in zip(sizes, frames):
        entries.extend(_ico_dir_entry(s, s, offset + len(blob), len(data)))
        blob.extend(data)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(struct.pack("<HHH", 0, 1, count) + entries + blob)


def write_svg_copies() -> None:
    """Publish the kit master SVG as the brand + site mark."""
    raw = MARK_SVG.read_bytes()
    (BRAND / "phostide-mark.svg").write_bytes(raw)
    (SITE_PUBLIC / "phostide-mark.svg").write_bytes(raw)
    # Favicon SVG = small cut when available
    small = MARK_SMALL_SVG if MARK_SMALL_SVG.exists() else MARK_SVG
    (SITE_PUBLIC / "favicon.svg").write_bytes(small.read_bytes())


def compose_og(path: Path) -> None:
    """1200×630 social card: brand-first mark + copy + theme strip."""
    w, h = 1200, 630
    card = Image.new("RGB", (w, h), (0, 0, 0))
    draw = ImageDraw.Draw(card)

    mark = make_mark(160)
    card.paste(mark, (72, 180), mark)

    title_font = _font(84)
    sub_font = _font(30, bold=False)
    draw.text((280, 198), "Phostide", font=title_font, fill=(244, 244, 245))
    draw.text(
        (280, 310),
        "Animated themes, painted into your real taskbar.",
        font=sub_font,
        fill=(138, 138, 147),
    )
    draw.text(
        (280, 360),
        "Free · MIT · Windows 11",
        font=_font(24, bold=False),
        fill=(138, 138, 147),
    )
    draw.rounded_rectangle((280, 420, 420, 426), radius=3, fill=(255, 138, 61))

    strip_h = 48
    if THEME_STRIP.exists():
        strip = Image.open(THEME_STRIP).convert("RGB")
        strip = ImageOps.fit(strip, (w, strip_h), method=Image.Resampling.LANCZOS)
        card.paste(strip, (0, h - strip_h))
    else:
        draw.rectangle((0, h - strip_h, w, h), fill=(255, 138, 61))

    path.parent.mkdir(parents=True, exist_ok=True)
    card.save(path, format="PNG", optimize=True)
    card.save(path.with_suffix(".jpg"), format="JPEG", quality=92, optimize=True)


def write_webmanifest() -> None:
    manifest = """{
  "name": "Phostide",
  "short_name": "Phostide",
  "icons": [
    { "src": "/icon-192.png", "sizes": "192x192", "type": "image/png" },
    { "src": "/icon-512.png", "sizes": "512x512", "type": "image/png" }
  ],
  "theme_color": "#000000",
  "background_color": "#000000",
  "display": "browser"
}
"""
    (SITE_PUBLIC / "manifest.webmanifest").write_text(manifest, encoding="utf-8")


def main() -> None:
    BRAND.mkdir(parents=True, exist_ok=True)
    SITE_PUBLIC.mkdir(parents=True, exist_ok=True)
    SITE_APP.mkdir(parents=True, exist_ok=True)

    if not MARK_SVG.exists():
        raise SystemExit(f"Missing {MARK_SVG} — finish the logo kit first.")

    write_svg_copies()

    master = make_mark(1024)
    write_png(BRAND / "phostide-1024.png", master)
    write_png(BRAND / "phostide-1024-transparent.png", master)

    ico_sizes = [16, 20, 24, 32, 48, 64, 256]
    build_ico(BRAND / "phostide.ico", ico_sizes)
    build_ico(BRAND / "favicon.ico", [16, 32, 48])
    build_ico(BRAND / "setup-phostide.ico", ico_sizes)

    write_png(BRAND / "favicon-32.png", make_tray_mark(32))
    write_png(BRAND / "apple-touch-icon.png", make_mark(180))
    write_png(BRAND / "icon-192.png", make_mark(192))
    write_png(BRAND / "icon-512.png", make_mark(512))
    write_png(BRAND / "phostide-tray-master.png", make_tray_mark(1024))

    sizes_dir = BRAND / "png"
    for s in ico_sizes + [180, 192, 512]:
        write_png(
            sizes_dir / f"phostide-{s}.png",
            make_tray_mark(s) if s <= 48 else make_mark(s),
        )

    compose_og(BRAND / "og.png")
    (BRAND / "og-ai.jpg").write_bytes((BRAND / "og.jpg").read_bytes())

    host_ico = HOST / "phostide.ico"
    host_ico.write_bytes((BRAND / "phostide.ico").read_bytes())

    for name in (
        "favicon.ico",
        "favicon-32.png",
        "apple-touch-icon.png",
        "icon-192.png",
        "icon-512.png",
        "og.png",
        "og.jpg",
        "og-ai.jpg",
        "phostide-mark.svg",
    ):
        src = BRAND / name
        if src.exists():
            (SITE_PUBLIC / name).write_bytes(src.read_bytes())

    write_png(SITE_PUBLIC / "mark-64.png", make_mark(64))
    write_png(SITE_PUBLIC / "phostide-mark-oled.png", make_mark(512))
    write_webmanifest()

    (SITE_APP / "favicon.ico").write_bytes((BRAND / "favicon.ico").read_bytes())
    write_png(SITE_APP / "icon.png", make_mark(512))
    write_png(SITE_APP / "apple-touch-icon.png", make_mark(180))
    (SITE_APP / "opengraph-image.png").write_bytes((BRAND / "og.png").read_bytes())
    (SITE_APP / "opengraph-image.jpg").write_bytes((BRAND / "og.jpg").read_bytes())

    print(f"Wrote Screen Tide icons under {BRAND}")
    print(f"Host ICO (exe/tray): {host_ico}")
    print(f"Setup ICO: {BRAND / 'setup-phostide.ico'}")
    print("Site public + app icons / OG updated")


if __name__ == "__main__":
    main()
