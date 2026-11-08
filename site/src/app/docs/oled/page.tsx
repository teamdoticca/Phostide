import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "OLED notes",
  description:
    "How Phostide approaches OLED taskbar burn-in without autohide or overlays.",
};

export default function OledPage() {
  return (
    <div className="prose-docs">
      <h1 className="font-[family-name:var(--font-display)] text-4xl font-semibold tracking-tight">
        OLED notes
      </h1>
      <p className="mt-4 max-w-2xl text-lg">
        Windows taskbars are often bright, static, and always on screen —
        exactly the pattern OLED panels dislike. Phostide attacks that pattern
        at the source: the fill itself.
      </p>

      <h2>What Phostide does</h2>
      <ul>
        <li>
          Keeps the bar visible and interactive (unlike autohide workflows).
        </li>
        <li>
          Moves background luminance continuously across the full fill region.
        </li>
        <li>
          Prefers restrained brightness so icons stay legible and average pixel
          stress stays lower.
        </li>
      </ul>

      <h2>What it is not</h2>
      <ul>
        <li>Not a warranty or a guarantee that burn-in cannot occur.</li>
        <li>Not a pixel-shift of the whole desktop.</li>
        <li>Not a transparent overlay window sitting above Explorer.</li>
      </ul>

      <h2>Practical habits</h2>
      <ul>
        <li>Use a theme you can leave on all day without distraction.</li>
        <li>Lower intensity if the bar feels busy under dense icon sets.</li>
        <li>
          Pair with normal Windows OLED hygiene: avoid static bright UI parked
          for weeks, and use panel pixel cleaning tools when the OEM provides
          them.
        </li>
      </ul>

      <h2>Why Explorer composition</h2>
      <p>
        Overlays can desync, clip, or fight DWM. Painting{" "}
        <code>Rectangle#BackgroundFill</code> means themes travel with the real
        taskbar — multi-monitor bars included — and restore cleanly when the
        host exits.
      </p>
    </div>
  );
}
