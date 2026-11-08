export type ThemeId = "rainbow" | "sea" | "weather" | "space" | "aurora";

export type ThemeInfo = {
  id: ThemeId;
  name: string;
  blurb: string;
  detail: string;
  barClass: string;
};

export const themes: ThemeInfo[] = [
  {
    id: "rainbow",
    name: "Rainbow",
    blurb: "Fixed spectrum across the bar with quiet atmospheric life — no hue marquee.",
    detail:
      "ROYGBIV mapped across taskbar thickness so every pixel keeps moving. Tuned for low average luminance instead of a ticker-style sweep.",
    barClass: "theme-bar theme-bar--rainbow",
  },
  {
    id: "sea",
    name: "Sea Waves",
    blurb: "Gerstner-style swell that rolls the full bar so no strip stays fixed.",
    detail:
      "Layered ocean motion painted edge-to-edge. Locked paint path: calm time scale, full-bar coverage for OLED anti burn-in.",
    barClass: "theme-bar theme-bar--sea",
  },
  {
    id: "weather",
    name: "Weather",
    blurb: "Storm loop with slanted rain and brief lightning flashes.",
    detail:
      "Cloud-dark luminance with intermittent flashes. Designed to keep contrast readable under tray icons while pixels keep shifting.",
    barClass: "theme-bar theme-bar--weather",
  },
  {
    id: "space",
    name: "Space",
    blurb: "Forward cinematic nebula into a sparse starfield — no rewind.",
    detail:
      "Depth motion toward the camera, not snowfall. Locked paint path: forward-only loop with no fade-in at phase wrap.",
    barClass: "theme-bar theme-bar--space",
  },
  {
    id: "aurora",
    name: "Aurora",
    blurb: "Green→teal→violet curtains with a standing undulation.",
    detail:
      "Soft vertical curtains that breathe in place. OLED-friendly mid tones with continuous standing-wave motion across the fill.",
    barClass: "theme-bar theme-bar--aurora",
  },
];
