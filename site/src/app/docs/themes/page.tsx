import type { Metadata } from "next";
import { ThemePreview } from "@/components/ThemePreview";
import { themes } from "@/lib/themes";

export const metadata: Metadata = {
  title: "Themes",
  description:
    "Rainbow, Sea Waves, Weather, Space, and Aurora — Phostide taskbar themes.",
};

export default function ThemesPage() {
  return (
    <div className="prose-docs">
      <h1 className="font-[family-name:var(--font-display)] text-4xl font-semibold tracking-tight">
        Themes
      </h1>
      <p className="mt-4 max-w-2xl text-lg">
        Every theme paints the full taskbar background so static chrome does
        not sit on the same OLED pixels. Motion is procedural Composition
        surfaces — not screenshots or video loops.
      </p>

      <div className="mt-12 space-y-12">
        {themes.map((theme, index) => (
          <section key={theme.id} className="border-t border-[var(--line)] pt-8">
            <ThemePreview
              theme={theme}
              className="theme-preview--wide"
              sizes="(max-width: 768px) 100vw, 720px"
            />
            <h2 className="!mt-5">
              {index}. {theme.name}
            </h2>
            <p>{theme.detail}</p>
          </section>
        ))}
      </div>

      <h2>Choosing intensity</h2>
      <p>
        Intensity scales effect energy without turning the bar into a light
        strip. Prefer the lowest setting that still reads as continuous motion
        under your icons — that is the OLED-friendly default.
      </p>

      <h2>Paint rules of thumb</h2>
      <ul>
        <li>Full-bar every-pixel paint for anti burn-in.</li>
        <li>No sideways hue or star marquees that feel like tickers.</li>
        <li>Cinematic loops stay forward-only (especially Space).</li>
        <li>Calm time scales keep the bar usable day to day.</li>
      </ul>
    </div>
  );
}
