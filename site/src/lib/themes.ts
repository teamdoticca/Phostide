export type ThemeId = "rainbow" | "sea" | "weather" | "space" | "aurora";

export type ThemeInfo = {
  id: ThemeId;
  name: string;
  blurb: string;
  detail: string;
  previewSrc: string;
  /** CSS fallback used only when images are unavailable. */
  barClass: string;
};

export const themes: ThemeInfo[] = [
  {
    id: "rainbow",
    name: "Rainbow",
    blurb: "Soft color bands across the bar — calm shimmer, not a scrolling rainbow.",
    detail:
      "Classic colors stacked through the height of the taskbar, with a gentle glow so every part of the bar keeps moving.",
    previewSrc: "/themes/rainbow.jpg",
    barClass: "theme-bar theme-bar--rainbow",
  },
  {
    id: "sea",
    name: "Sea Waves",
    blurb: "Slow ocean swell that rolls along the whole taskbar.",
    detail:
      "Deep blues and soft foam highlights. The motion covers the full bar so nothing sits still for long.",
    previewSrc: "/themes/sea.jpg",
    barClass: "theme-bar theme-bar--sea",
  },
  {
    id: "weather",
    name: "Weather",
    blurb: "Rain on a dark sky, with short flashes of lightning.",
    detail:
      "A simple storm loop: slanted rain, then a bright flash. Easy to see, easy on the eyes when you leave it running.",
    previewSrc: "/themes/weather.jpg",
    barClass: "theme-bar theme-bar--weather",
  },
  {
    id: "space",
    name: "Space",
    blurb: "Fly past a warm nebula toward the stars.",
    detail:
      "A short space trip that always moves forward — glowing cloud, then a field of stars — then it loops again.",
    previewSrc: "/themes/space.jpg",
    barClass: "theme-bar theme-bar--space",
  },
  {
    id: "aurora",
    name: "Aurora",
    blurb: "Northern-light curtains in green, teal, and violet.",
    detail:
      "Soft light that waves in place across the bar — quiet, colorful, and made for long desktop sessions.",
    previewSrc: "/themes/aurora.jpg",
    barClass: "theme-bar theme-bar--aurora",
  },
];
