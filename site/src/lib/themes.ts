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
  /**
   * Measured from the WebM capture (ThemePreviewExport, 100% intensity):
   * mean luma 0-255 over every pixel of every frame, and mean absolute
   * frame-to-frame luma change at 20 fps. Re-measure when captures change.
   */
  measured: { meanLuma: number; motion: number };
  /** 1-5 meters relative to the other four themes, from `measured`. */
  brightness: 1 | 2 | 3 | 4 | 5;
  movement: 1 | 2 | 3 | 4 | 5;
  /** Anything the averages hide. */
  note?: string;
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
    measured: { meanLuma: 61.5, motion: 0.22 },
    brightness: 5,
    movement: 1,
  },
  {
    id: "sea",
    name: "Sea Waves",
    blurb: "Side-on ocean silhouette with foam and spray.",
    detail: "Side-on ocean silhouette with foam under the crest and breaking spray.",
    previewSrc: "/themes/sea.png",
    previewVideo: "/themes/sea.webm",
    measured: { meanLuma: 31.3, motion: 1.41 },
    brightness: 2,
    movement: 4,
  },
  {
    id: "weather",
    name: "Weather",
    blurb: "Night storm sky with purple flashes and soft bolts.",
    detail:
      "Dark blue night storm — slate clouds, localized purple flash, soft branching bolts.",
    previewSrc: "/themes/weather.png",
    previewVideo: "/themes/weather.webm",
    measured: { meanLuma: 52.2, motion: 0.75 },
    brightness: 3,
    movement: 3,
  },
  {
    id: "space",
    name: "Space",
    blurb: "Mostly black with slow, scattered stars.",
    detail: "Mostly black with slow, scattered stars — quiet motion for OLED.",
    previewSrc: "/themes/space.png",
    previewVideo: "/themes/space.webm",
    measured: { meanLuma: 56.5, motion: 1.64 },
    brightness: 4,
    movement: 5,
    note: "Near-black for most of its 35-second loop, then a bright nebula drifts through. Its average is higher than it looks at a glance.",
  },
  {
    id: "aurora",
    name: "Aurora",
    blurb: "Falling magenta curtains drifting across black.",
    detail: "Falling magenta/violet curtains that drift along the bar over black.",
    previewSrc: "/themes/aurora.png",
    previewVideo: "/themes/aurora.webm",
    measured: { meanLuma: 21.8, motion: 0.33 },
    brightness: 1,
    movement: 2,
  },
];

export function themeById(id: ThemeId): ThemeInfo {
  return themes.find((t) => t.id === id) ?? themes[0];
}
