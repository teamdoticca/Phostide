#!/usr/bin/env python3
"""Build Phostide brand icons by resampling the photographic wave master.

Master: assets/brand/phostide-1024.png (OLED void + luminous cyan/blue ribbon).
Tray (≤32): assets/brand/phostide-tray-master.png — frosted chip + cyan glow +
boosted wave so the mark reads on Win11 dark taskbars.
OG card: assets/brand/og-ai.jpg (kept as the site Open Graph hero).
"""

from __future__ import annotations

import struct
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageEnhance, ImageFilter, ImageOps

ROOT = Path(__file__).resolve().parents[1]
BRAND = ROOT / "assets" / "brand"
SITE_PUBLIC = ROOT / "site" / "public"
SITE_APP = ROOT / "site" / "src" / "app"
HOST = ROOT / "src" / "Host"

MASTER_PNG = BRAND / "phostide-1024.png"
MASTER_AI_JPG = BRAND / "phostide-icon-master.jpg"
TRAY_MASTER_PNG = BRAND / "phostide-tray-master.png"
OG_AI = BRAND / "og-ai.jpg"

BG = (7, 8, 12, 255)  # #07080c — matches site OLED void

# Tray plate: cool frosted chip (never pure black at ≤32).
TRAY_CHIP = (236, 242, 248, 255)  # near-white frost
TRAY_CHIP_INNER = (224, 234, 244, 255)  # slight inner depth
TRAY_GLOW = (120, 230, 255, 255)  # luminous cyan disc under the wave
TRAY_THRESHOLD = 32  # sizes ≤ this use tray treatment


def load_master() -> Image.Image:
    """Load the square wave master (PNG preferred, AI JPG fallback)."""
    if MASTER_PNG.exists():
        img = Image.open(MASTER_PNG).convert("RGBA")
    elif MASTER_AI_JPG.exists():
        img = Image.open(MASTER_AI_JPG).convert("RGBA")
    else:
        raise FileNotFoundError(
            f"Missing icon master. Expected {MASTER_PNG} or {MASTER_AI_JPG}"
        )
    if img.size != (1024, 1024):
        img = ImageOps.fit(img, (1024, 1024), method=Image.Resampling.LANCZOS)
    return img


def ensure_opaque_bg(img: Image.Image) -> Image.Image:
    """Composite onto OLED void if alpha is present."""
    if img.mode != "RGBA":
        img = img.convert("RGBA")
    bg = Image.new("RGBA", img.size, BG)
    return Image.alpha_composite(bg, img)


def make_transparent(img: Image.Image) -> Image.Image:
    """Punch near-black to alpha so the ribbon can sit on arbitrary dark UIs."""
    rgba = img.convert("RGBA")
    r, g, b, _a = rgba.split()
    gray = Image.merge("RGB", (r, g, b)).convert("L")
    alpha = gray.point(
        lambda v: 0 if v < 10 else (255 if v >= 48 else int(255 * (v - 10) / 38.0))
    )
    return Image.merge("RGBA", (r, g, b, alpha))


def _rounded_rect_mask(size: int, radius: float) -> Image.Image:
    """Anti-aliased rounded-rect alpha mask at `size`."""
    scale = 4
    big = size * scale
    r = int(radius * scale)
    mask = Image.new("L", (big, big), 0)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle((0, 0, big - 1, big - 1), radius=r, fill=255)
    return mask.resize((size, size), Image.Resampling.LANCZOS)


def _radial_glow(size: int, color: tuple[int, int, int, int], strength: float = 0.55) -> Image.Image:
    """Soft circular cyan bloom centered on the plate (drawn oversized, then blurred)."""
    cr, cg, cb, _ = color
    layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)
    # Concentric discs approximate a radial falloff; blur finishes the glow.
    cx = cy = size // 2
    max_r = int(size * 0.42)
    steps = 18
    for i in range(steps, 0, -1):
        t = i / steps
        r = int(max_r * t)
        # Stronger core (almost white-cyan) so ≤16px still shows a halo.
        fall = (1.0 - t) ** 1.35
        a = int(255 * strength * (0.35 + 0.65 * fall))
        tone = 0.55 + 0.45 * fall
        fill = (
            min(255, int(cr + (255 - cr) * tone * 0.35)),
            min(255, int(cg + (255 - cg) * tone * 0.25)),
            min(255, int(cb)),
            a,
        )
        draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=fill)
    return layer.filter(ImageFilter.GaussianBlur(radius=max(2, size // 40)))


def _boost_wave(wave: Image.Image, *, hot: bool = False) -> Image.Image:
    """Thicken + brighten the cyan ribbon for tray readability."""
    r, g, b, a = wave.split()
    rgb = Image.merge("RGB", (r, g, b))
    rgb = ImageEnhance.Color(rgb).enhance(1.45 if hot else 1.32)
    rgb = ImageEnhance.Brightness(rgb).enhance(1.35 if hot else 1.22)
    rgb = ImageEnhance.Contrast(rgb).enhance(1.28 if hot else 1.16)
    r, g, b = rgb.split()

    # Expand alpha slightly so the ribbon reads thicker after downsample.
    thicken = 2 if hot else 1
    a_fat = a.filter(ImageFilter.MaxFilter(size=thicken * 2 + 1))
    a_fat = ImageEnhance.Brightness(a_fat).enhance(1.15 if hot else 1.08)
    # Soft white-cyan outer glow from the thickened alpha.
    glow_a = a_fat.filter(ImageFilter.GaussianBlur(radius=6 if hot else 4))
    glow_a = ImageEnhance.Brightness(glow_a).enhance(0.85 if hot else 0.65)
    glow = Image.merge(
        "RGBA",
        (
            Image.new("L", wave.size, 230),
            Image.new("L", wave.size, 250),
            Image.new("L", wave.size, 255),
            glow_a,
        ),
    )
    core = Image.merge("RGBA", (r, g, b, a_fat))
    return Image.alpha_composite(glow, core)


def build_tray_master(master: Image.Image, size: int = 1024) -> Image.Image:
    """Frosted chip + cyan glow disc + boosted wave — dedicated dark-taskbar surface."""
    # Plate with subtle inner wash (not flat white cardboard).
    plate = Image.new("RGBA", (size, size), TRAY_CHIP)
    inner = Image.new("RGBA", (size, size), TRAY_CHIP_INNER)
    inset = int(size * 0.06)
    inner_mask = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(inner_mask)
    draw.ellipse(
        (inset, inset, size - inset - 1, size - inset - 1),
        fill=90,
    )
    inner_mask = inner_mask.filter(ImageFilter.GaussianBlur(radius=size // 18))
    plate = Image.composite(inner, plate, inner_mask)

    radius = size * 0.22
    chip_mask = _rounded_rect_mask(size, radius)
    plate.putalpha(chip_mask)

    # Luminous cyan disc behind the wave.
    glow = _radial_glow(size, TRAY_GLOW, strength=0.58)
    ga = ImageChops.multiply(glow.split()[3], chip_mask)
    glow.putalpha(ga)
    plate = Image.alpha_composite(plate, glow)

    # Wave extracted from OLED master, boosted for tiny-size legibility.
    wave = make_transparent(ensure_opaque_bg(master))
    if wave.size != (size, size):
        wave = wave.resize((size, size), Image.Resampling.LANCZOS)
    wave = _boost_wave(wave, hot=True)
    # Keep wave inside the rounded chip.
    wr, wg, wb, wa = wave.split()
    wa = ImageChops.multiply(wa, chip_mask)
    wave = Image.merge("RGBA", (wr, wg, wb, wa))
    return Image.alpha_composite(plate, wave)


def resample_mark(
    master: Image.Image,
    size: int,
    *,
    transparent_bg: bool = False,
    tray_master: Image.Image | None = None,
) -> Image.Image:
    """Downscale master. Tray sizes (≤32) use the frosted-chip tray master."""
    use_tray = tray_master is not None and size <= TRAY_THRESHOLD and not transparent_bg

    if use_tray:
        base = tray_master
        out = base.resize((size, size), Image.Resampling.LANCZOS)
        # Extra punch after downsample — almost white-cyan core at 16px.
        out = ImageEnhance.Contrast(out).enhance(1.22 if size <= 16 else 1.12)
        out = ImageEnhance.Color(out).enhance(1.28 if size <= 16 else 1.16)
        out = ImageEnhance.Brightness(out).enhance(1.10 if size <= 16 else 1.05)
        out = out.filter(
            ImageFilter.UnsharpMask(
                radius=0.55 if size <= 16 else 0.7,
                percent=160 if size <= 16 else 130,
                threshold=1,
            )
        )
        return out.convert("RGBA")

    base = master if transparent_bg else ensure_opaque_bg(master)
    if transparent_bg:
        base = make_transparent(base)

    out = base.resize((size, size), Image.Resampling.LANCZOS)

    # Mid sizes (48): light ribbon boost only — still OLED plate.
    if size == 48 and not transparent_bg:
        out = ImageEnhance.Contrast(out).enhance(1.08)
        out = ImageEnhance.Color(out).enhance(1.10)
        out = ImageEnhance.Brightness(out).enhance(1.04)

    return out.convert("RGBA")


def write_png(path: Path, img: Image.Image) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path, format="PNG", optimize=True)


def png_bytes(img: Image.Image) -> bytes:
    import io

    buf = io.BytesIO()
    img.save(buf, format="PNG", optimize=True)
    return buf.getvalue()


def _ico_dir_entry(width: int, height: int, offset: int, data_len: int) -> bytes:
    w = 0 if width >= 256 else width
    h = 0 if height >= 256 else height
    return struct.pack("<BBBBHHII", w, h, 0, 0, 1, 32, data_len, offset)


def build_ico(
    path: Path,
    master: Image.Image,
    sizes: list[int],
    *,
    tray_master: Image.Image | None = None,
) -> None:
    """Write multi-resolution ICO with embedded PNG frames (Vista+)."""
    frames: list[bytes] = []
    for s in sizes:
        frames.append(
            png_bytes(resample_mark(master, s, transparent_bg=False, tray_master=tray_master))
        )

    count = len(frames)
    offset = 6 + 16 * count
    entries = bytearray()
    blob = bytearray()
    for s, data in zip(sizes, frames):
        entries.extend(_ico_dir_entry(s, s, offset + len(blob), len(data)))
        blob.extend(data)

    header = struct.pack("<HHH", 0, 1, count)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(header + entries + blob)


def write_svg(path: Path) -> None:
    """Fallback SVG: denser multi-strand cyan/blue ribbon (not the old thin peach arc)."""
    svg = """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1024 1024" role="img" aria-label="Phostide">
  <title>Phostide</title>
  <defs>
    <linearGradient id="core" x1="0%" y1="50%" x2="100%" y2="50%">
      <stop offset="0%" stop-color="#22d3ee"/>
      <stop offset="35%" stop-color="#38bdf8"/>
      <stop offset="70%" stop-color="#2563eb"/>
      <stop offset="100%" stop-color="#67e8f9"/>
    </linearGradient>
    <linearGradient id="edge" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#a5f3fc" stop-opacity="0.85"/>
      <stop offset="50%" stop-color="#60a5fa" stop-opacity="0.55"/>
      <stop offset="100%" stop-color="#22d3ee" stop-opacity="0.75"/>
    </linearGradient>
    <filter id="glow" x="-35%" y="-80%" width="170%" height="260%">
      <feGaussianBlur stdDeviation="18" result="blur"/>
      <feMerge>
        <feMergeNode in="blur"/>
        <feMergeNode in="blur"/>
        <feMergeNode in="SourceGraphic"/>
      </feMerge>
    </filter>
    <filter id="soft" x="-20%" y="-50%" width="140%" height="200%">
      <feGaussianBlur stdDeviation="6" result="b"/>
      <feMerge>
        <feMergeNode in="b"/>
        <feMergeNode in="SourceGraphic"/>
      </feMerge>
    </filter>
  </defs>
  <rect width="1024" height="1024" fill="#07080c"/>
  <!-- Soft bloom under the ribbon -->
  <path d="M120 540 C 260 420, 420 680, 560 520 S 820 380, 920 500"
        fill="none" stroke="#38bdf8" stroke-width="90" stroke-linecap="round"
        opacity="0.18" filter="url(#glow)"/>
  <!-- Outer strands -->
  <path d="M140 560 C 280 430, 430 690, 570 530 S 800 400, 900 520"
        fill="none" stroke="url(#edge)" stroke-width="42" stroke-linecap="round"
        opacity="0.55" filter="url(#soft)"/>
  <path d="M130 520 C 270 400, 410 660, 550 500 S 810 360, 910 480"
        fill="none" stroke="url(#edge)" stroke-width="36" stroke-linecap="round"
        opacity="0.45" filter="url(#soft)"/>
  <!-- Bright core ribbon -->
  <path d="M150 540 C 290 420, 430 680, 560 520 S 800 390, 890 500"
        fill="none" stroke="url(#core)" stroke-width="28" stroke-linecap="round"
        filter="url(#glow)"/>
  <!-- Hot cyan spine -->
  <path d="M180 535 C 310 430, 440 650, 565 525 S 780 405, 870 495"
        fill="none" stroke="#e0faff" stroke-width="10" stroke-linecap="round"
        opacity="0.9" filter="url(#soft)"/>
  <!-- Fine sparkles -->
  <circle cx="280" cy="470" r="3" fill="#ffffff" opacity="0.9"/>
  <circle cx="420" cy="590" r="2" fill="#a5f3fc" opacity="0.75"/>
  <circle cx="610" cy="500" r="2.5" fill="#ffffff" opacity="0.85"/>
  <circle cx="760" cy="430" r="2" fill="#67e8f9" opacity="0.7"/>
  <circle cx="520" cy="545" r="1.5" fill="#ffffff" opacity="0.65"/>
</svg>
"""
    path.write_text(svg, encoding="utf-8")


def compose_og_from_ai(path: Path) -> None:
    """Publish og-ai.jpg as the site OG card (1200×630 PNG)."""
    if not OG_AI.exists():
        raise FileNotFoundError(f"Missing OG master: {OG_AI}")
    src = Image.open(OG_AI).convert("RGB")
    # Cover-fit into Open Graph 1.91:1 without letterboxing the void.
    fitted = ImageOps.fit(src, (1200, 630), method=Image.Resampling.LANCZOS)
    path.parent.mkdir(parents=True, exist_ok=True)
    fitted.save(path, format="PNG", optimize=True)
    # Keep a JPEG sibling for lighter social caches if needed.
    jpg_path = path.with_suffix(".jpg")
    fitted.save(jpg_path, format="JPEG", quality=92, optimize=True)


def main() -> None:
    BRAND.mkdir(parents=True, exist_ok=True)

    # If AI JPG exists and PNG is older/missing, refresh PNG from JPG once.
    if MASTER_AI_JPG.exists():
        ai = Image.open(MASTER_AI_JPG).convert("RGBA")
        if ai.size != (1024, 1024):
            ai = ImageOps.fit(ai, (1024, 1024), method=Image.Resampling.LANCZOS)
        opaque = ensure_opaque_bg(ai)
        write_png(MASTER_PNG, opaque)
        write_png(BRAND / "phostide-1024-transparent.png", make_transparent(opaque))

    master = load_master()
    write_svg(BRAND / "phostide-mark.svg")

    # Canonical OLED masters (large icons + OG stay premium dark).
    write_png(MASTER_PNG, ensure_opaque_bg(master))
    write_png(BRAND / "phostide-1024-transparent.png", make_transparent(master))

    # Dedicated tray master: frosted chip + cyan glow + hot wave.
    tray_master = build_tray_master(master, 1024)
    write_png(TRAY_MASTER_PNG, tray_master)

    ico_sizes = [16, 20, 24, 32, 48, 64, 256]
    build_ico(BRAND / "phostide.ico", master, ico_sizes, tray_master=tray_master)
    build_ico(BRAND / "favicon.ico", master, [16, 32, 48], tray_master=tray_master)
    build_ico(BRAND / "setup-phostide.ico", master, ico_sizes, tray_master=tray_master)

    write_png(BRAND / "favicon-32.png", resample_mark(master, 32, tray_master=tray_master))
    write_png(BRAND / "apple-touch-icon.png", resample_mark(master, 180))
    write_png(BRAND / "icon-192.png", resample_mark(master, 192))
    write_png(BRAND / "icon-512.png", resample_mark(master, 512))

    sizes_dir = BRAND / "png"
    for s in ico_sizes + [180, 192, 512]:
        write_png(
            sizes_dir / f"phostide-{s}.png",
            resample_mark(master, s, tray_master=tray_master),
        )

    # OG: ship og-ai as the hero (not the weak geometric compose_og).
    compose_og_from_ai(BRAND / "og.png")
    # Also keep a direct public copy of the AI OG for clarity.
    if OG_AI.exists():
        (BRAND / "og.jpg").write_bytes(OG_AI.read_bytes())

    host_ico = HOST / "phostide.ico"
    host_ico.write_bytes((BRAND / "phostide.ico").read_bytes())

    SITE_PUBLIC.mkdir(parents=True, exist_ok=True)
    SITE_APP.mkdir(parents=True, exist_ok=True)
    for name in (
        "favicon.ico",
        "favicon-32.png",
        "apple-touch-icon.png",
        "icon-192.png",
        "icon-512.png",
        "og.png",
    ):
        src = BRAND / name
        (SITE_PUBLIC / name).write_bytes(src.read_bytes())
    # Prefer JPEG OG in public as well (same visual as og-ai).
    if (BRAND / "og.jpg").exists():
        (SITE_PUBLIC / "og.jpg").write_bytes((BRAND / "og.jpg").read_bytes())

    (SITE_APP / "favicon.ico").write_bytes((BRAND / "favicon.ico").read_bytes())
    write_png(SITE_APP / "icon.png", resample_mark(master, 512))
    write_png(SITE_APP / "apple-touch-icon.png", resample_mark(master, 180))
    (SITE_APP / "opengraph-image.png").write_bytes((BRAND / "og.png").read_bytes())
    if (BRAND / "og.jpg").exists():
        (SITE_APP / "opengraph-image.jpg").write_bytes((BRAND / "og.jpg").read_bytes())

    print(f"Wrote icons under {BRAND}")
    print(f"Master: {MASTER_PNG}")
    print(f"Tray master: {TRAY_MASTER_PNG}")
    print(f"OG from: {OG_AI}")
    print(f"Host ICO: {host_ico}")
    print("Site public + app icons updated")
    print(f"Tray treatment applied to sizes ≤{TRAY_THRESHOLD}px")


if __name__ == "__main__":
    main()
