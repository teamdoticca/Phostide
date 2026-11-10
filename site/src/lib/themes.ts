export type ThemeId = "rainbow" | "sea" | "weather" | "space" | "aurora";

export type ThemeInfo = {
  id: ThemeId;
  name: string;
  blurb: string;
  detail: string;
  /** Still poster from PaintThemePixels (1920×48). */
  previewSrc: string;
  /** Animated Native-48px capture (WebM VP9). */
  previewVideo: string;
};

export const themes: ThemeInfo[] = [
  {
    id: "rainbow",
    name: "Rainbow",
    blurb: "Soft color bands drifting slowly across the bar.",
    detail:
      "Soft color bands drifting slowly across the bar — calm shimmer, not a scrolling rainbow.",
    previewSrc: "/themes/rainbow.png",
    previewVideo: "/themes/rainbow.webm",
  },
  {
    id: "sea",
    name: "Sea Waves",
    blurb: "Side-on ocean silhouette with foam and spray.",
    detail: "Side-on ocean silhouette with foam under the crest and breaking spray.",
    previewSrc: "/themes/sea.png",
    previewVideo: "/themes/sea.webm",
  },
  {
    id: "weather",
    name: "Weather",
    blurb: "Night storm sky with purple flashes and soft bolts.",
    detail:
      "Dark blue night storm — slate clouds, localized purple flash, soft branching bolts.",
    previewSrc: "/themes/weather.png",
    previewVideo: "/themes/weather.webm",
  },
  {
    id: "space",
    name: "Space",
    blurb: "Mostly black with slow, scattered stars.",
    detail: "Mostly black with slow, scattered stars — quiet motion for OLED.",
    previewSrc: "/themes/space.png",
    previewVideo: "/themes/space.webm",
  },
  {
    id: "aurora",
    name: "Aurora",
    blurb: "Falling magenta curtains drifting across black.",
    detail: "Falling magenta/violet curtains that drift along the bar over black.",
    previewSrc: "/themes/aurora.png",
    previewVideo: "/themes/aurora.webm",
  },
];

export function themeById(id: ThemeId): ThemeInfo {
  return themes.find((t) => t.id === id) ?? themes[0];
}
