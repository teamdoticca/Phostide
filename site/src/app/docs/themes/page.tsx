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
        Pick a look from the tray. Previews below are real frames from the same
        painter that fills your taskbar — not moodboard art.
      </p>

      <div className="mt-12 space-y-10">
        {themes.map((theme, index) => (
          <section key={theme.id} className="border-t border-[var(--line)] pt-8">
            <ThemePreview theme={theme} size="docs" />
            <h2 className="!mt-5">
              {index}. {theme.name}
            </h2>
            <p>{theme.detail}</p>
          </section>
        ))}
      </div>

      <h2>Brightness tip</h2>
      <p>
        Start with a medium intensity. If the bar feels too bright or busy,
        turn it down. You still want soft motion — that is what helps the
        screen — just not a flashlight under your icons.
      </p>

      <h2>Good habits</h2>
      <ul>
        <li>Leave a theme running while you work; that is the point.</li>
        <li>Switch looks from the tray whenever you like.</li>
        <li>Quit Phostide anytime to restore the normal Windows bar.</li>
      </ul>
    </div>
  );
}
