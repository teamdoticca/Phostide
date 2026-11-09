export type ThemeId = "rainbow" | "sea" | "weather" | "space" | "aurora";

export type ThemeInfo = {
  id: ThemeId;
  name: string;
  blurb: string;
  detail: string;
  /** Real PaintThemePixels capture (1920×48 taskbar strip). */
  previewSrc: string;
};

export const themes: ThemeInfo[] = [
  {
    id: "rainbow",
    name: "Rainbow",
    blurb: "Soft color bands drifting slowly across the bar.",
    detail:
      "Soft color bands drifting slowly across the bar — calm shimmer, not a scrolling rainbow.",
    previewSrc: "/themes/rainbow.png",
  },
  {
    id: "sea",
    name: "Sea Waves",
    blurb: "Dark blue water with gentle moving waves.",
    detail: "Dark blue water with gentle moving waves across the full bar.",
    previewSrc: "/themes/sea.png",
  },
  {
    id: "weather",
    name: "Weather",
    blurb: "A small sky scene with drifting weather.",
    detail: "A small sky scene with drifting weather — rain and short flashes.",
    previewSrc: "/themes/weather.png",
  },
  {
    id: "space",
    name: "Space",
    blurb: "Mostly black with slow, scattered stars.",
    detail: "Mostly black with slow, scattered stars — quiet motion for OLED.",
    previewSrc: "/themes/space.png",
  },
  {
    id: "aurora",
    name: "Aurora",
    blurb: "Faint northern-light ribbons over black.",
    detail: "Faint northern-light ribbons over black — quiet and colorful.",
    previewSrc: "/themes/aurora.png",
  },
];

export function themeById(id: ThemeId): ThemeInfo {
  return themes.find((t) => t.id === id) ?? themes[0];
}
