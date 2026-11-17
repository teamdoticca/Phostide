import type { Metadata } from "next";
import Link from "next/link";
import {
  IconArrowLeft,
  IconArrowRight,
  IconEye,
  IconLayers,
  IconUndo,
  IconWave,
} from "@/components/Icons";
import { SettingsShot } from "@/components/SettingsShot";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { ThemeStrip } from "@/components/ThemeStrip";
import { pageMetadata } from "@/lib/seo";
import { themeById } from "@/lib/themes";

export const metadata: Metadata = pageMetadata({
  title: "OLED notes",
  description:
    "How Phostide protects OLED taskbars: moving fill while you work, hybrid idle Care when you step away, and per-monitor control — without hiding the bar.",
  path: "/docs/oled",
});

export default function OledPage() {
  const space = themeById("space");

  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <SiteHeader active="/docs/oled" />

      <section className="mx-auto max-w-site px-6 pb-12 pt-16 sm:px-10 sm:pb-16 sm:pt-20">
        <Link
          href="/docs"
          className="inline-flex items-center gap-2 text-sm text-soft hover:text-white"
        >
          <IconArrowLeft />
          Docs
        </Link>
        <p className="mt-6 eyebrow">OLED notes</p>
        <h1 className="mt-3 max-w-3xl text-4xl font-semibold leading-tight tracking-tight sm:text-5xl">
          An OLED taskbar solution that keeps the bar usable.
        </h1>
        <p className="mt-5 max-w-2xl text-lg text-soft">
          OLED pixels light themselves. A bright strip that never changes ages
          faster than the rest of the panel — and the Windows taskbar is one of
          the most static bright shapes on the desktop. Most OLED tools hide it
          or cover it. Phostide keeps it visible: moving fill while you work,
          idle Care when you step away, and per‑monitor control for OLED + LCD
          desks. Standalone — no Windhawk, no overlay window.
        </p>
      </section>

      <section className="border-y border-line">
        <div className="mx-auto flex max-w-site justify-between px-6 py-3 text-xs text-soft sm:px-10">
          <span>Space · dark fill that keeps moving</span>
          <span>1920 × 48 · real Explorer BackgroundFill</span>
        </div>
        <ThemeStrip theme={space} />
      </section>

      <section className="mx-auto grid max-w-site gap-5 px-6 py-16 sm:px-10 sm:py-20 md:grid-cols-2">
        <div className="rounded-xl border border-ember/40 bg-panel p-8">
          <IconWave className="h-5 w-5 text-ember" />
          <h2 className="mt-5 text-xl font-semibold">Moving fill while you work</h2>
          <p className="mt-2 text-soft">
            Five built‑in themes paint Explorer&apos;s real{" "}
            <code className="text-zinc-100">BackgroundFill</code> so the
            background is not one locked bright block. Intensity stays under
            your control — Space and Aurora stay the quietest everyday OLED
            choices.
          </p>
        </div>
        <div className="rounded-xl border border-line bg-panel p-8">
          <IconEye className="h-5 w-5 text-soft" />
          <h2 className="mt-5 text-xl font-semibold">Idle Care when you leave</h2>
          <p className="mt-2 text-soft">
            OLED Care dims the fill after idle. Full AFK dims every enabled bar;
            while you keep working, leave the cursor on one display and the other
            can dim alone. Optional experimental chrome dim / clock micro‑nudge
            softens residual Windows UI on idle bars only.
          </p>
        </div>
        <div className="rounded-xl border border-line bg-panel p-8">
          <IconLayers className="h-5 w-5 text-soft" />
          <h2 className="mt-5 text-xl font-semibold">OLED‑only on multi‑monitor</h2>
          <p className="mt-2 text-soft">
            Theme and Care the OLED; leave an LCD stock from the Monitors tab —
            or run different looks per display. Rendering stays one shared Auto /
            GPU / CPU choice. Not auto‑hide, not a floating overlay.
          </p>
        </div>
        <div className="rounded-xl border border-line bg-panel p-8">
          <IconUndo className="h-5 w-5 text-soft" />
          <h2 className="mt-5 text-xl font-semibold">Quit restores Windows</h2>
          <p className="mt-2 text-soft">
            Exit from the tray and stock brushes (and any Care chrome changes)
            come back. Phostide does not patch system files.
          </p>
        </div>

        <div className="rounded-xl border border-line bg-panel p-8 md:col-span-2">
          <h2 className="text-xl font-semibold">OLED Care in Settings</h2>
          <p className="mt-3 text-soft">
            Settings › OLED Care: Idle after (Off / 1 / 5 / 10 / 30 minutes), Idle
            fill intensity, optional Dim icons/tray/clock, optional Micro‑nudge
            clock, Night boost, Pause when locked, Pause in fullscreen (default
            on — freezes the covered display only so other bars keep moving).
            Attendance for split‑monitor
            idle follows the <strong className="font-medium text-zinc-100">cursor</strong>
            . Use{" "}
            <strong className="font-medium text-zinc-100">
              OLED everyday preset
            </strong>{" "}
            for a care defaults pack only — it does not change your Taskbar theme
            or intensity. Care only paints monitors left on in Monitors. The{" "}
            <strong className="font-medium text-zinc-100">Monitors</strong> tab
            shows live status (painting / fullscreen / locked / off) per display.
          </p>
          <div className="mt-6 grid gap-4 lg:grid-cols-2">
            <SettingsShot id="oled-care" />
            <SettingsShot id="monitors" />
          </div>
        </div>

        <div className="rounded-xl border border-line bg-panel p-8 md:col-span-2">
          <h2 className="text-xl font-semibold">Honest limits</h2>
          <ul className="mt-3 list-disc space-y-2 pl-5 text-soft">
            <li>
              Phostide reduces how long the same bright taskbar pixels stay
              locked — moving fill while you work, deeper idle Care when you step
              away. It is <strong className="font-medium text-zinc-100">not</strong>{" "}
              a burn‑in warranty or a full OLED pixel‑shift replacement.
            </li>
            <li>
              Themes move the background fill. Icons, Start, Search, tray, and
              clock stay Windows chrome unless you turn on experimental chrome dim
              / clock nudge — those depend on Explorer XAML names and may quietly
              no‑op after a Windows update.
            </li>
            <li>
              Hybrid idle: walk away and every enabled bar dims; while you work,
              only bars without the cursor can reach Idle after alone. Care never
              paints monitors you turned off.
            </li>
            <li>
              On OLED + LCD desks, uncheck the LCD in Monitors (or use a quieter
              theme/intensity on the OLED alone) so Care and motion target the
              panel that needs them.
            </li>
            <li>
              Still useful without Phostide: show clock seconds; hide the date if
              you do not need it — fewer static bright glyphs.
            </li>
          </ul>
        </div>

        <div className="rounded-xl border border-line bg-panel p-8 md:col-span-2">
          <h2 className="text-xl font-semibold">Other approaches</h2>
          <p className="mt-3 text-soft">
            Auto‑hide, dimmer overlays, and idle‑fade mods solve the same OLED
            worry differently. See how they compare to a usable bar with moving
            fill plus idle Care.
          </p>
          <Link
            href="/docs/compare"
            className="mt-4 inline-flex items-center gap-2 text-sm text-ember"
          >
            Compare approaches <IconArrowRight />
          </Link>
        </div>

        <div className="flex justify-between pt-4 text-sm md:col-span-2">
          <Link
            href="/docs/themes"
            className="inline-flex items-center gap-2 text-soft hover:text-white"
          >
            <IconArrowLeft />
            Themes
          </Link>
          <Link
            href="/docs/compare"
            className="inline-flex items-center gap-2 text-ember"
          >
            Next: Compare <IconArrowRight />
          </Link>
        </div>
      </section>

      <SiteFooter />
    </div>
  );
}
