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
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { ThemeStrip } from "@/components/ThemeStrip";
import { pageMetadata } from "@/lib/seo";
import { themeById } from "@/lib/themes";

export const metadata: Metadata = pageMetadata({
  title: "OLED notes",
  description:
    "Why a still taskbar matters on OLED, and how Phostide keeps the fill moving.",
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
          Why a still taskbar matters on an OLED screen.
        </h1>
        <p className="mt-5 max-w-2xl text-lg text-soft">
          OLED pixels light themselves. When the same bright shape sits in the
          same spot for hours, those pixels age faster than the rest. The
          Windows taskbar is one of the most static bright strips on the
          desktop — which is why many OLED tools simply hide it. Phostide is
          standalone (no Windhawk) and paints the real fill instead of covering
          or hiding the bar — five built‑in themes, no theme editor.
        </p>
      </section>

      <section className="border-y border-line">
        <div className="mx-auto flex max-w-site justify-between px-6 py-3 text-xs text-soft sm:px-10">
          <span>Space theme · mostly black, always moving</span>
          <span>1920 × 48, real taskbar painter</span>
        </div>
        <ThemeStrip theme={space} />
      </section>

      <section className="mx-auto grid max-w-site gap-5 px-6 py-16 sm:px-10 sm:py-20 md:grid-cols-2">
        <div className="rounded-xl border border-ember/40 bg-panel p-8">
          <IconWave className="h-5 w-5 text-ember" />
          <h2 className="mt-5 text-xl font-semibold">It keeps the fill moving</h2>
          <p className="mt-2 text-soft">
            Phostide replaces the flat taskbar background with slow motion on a
            dark base, so the background isn&apos;t one fixed block of light.
            Intensity in Settings lets you keep that motion quiet.
          </p>
        </div>
        <div className="rounded-xl border border-line bg-panel p-8">
          <IconEye className="h-5 w-5 text-soft" />
          <h2 className="mt-5 text-xl font-semibold">It&apos;s not auto‑hide</h2>
          <p className="mt-2 text-soft">
            The taskbar stays visible and usable. You don&apos;t have to hunt
            the screen edge every time you need Start, pinned apps, or the
            clock.
          </p>
        </div>
        <div className="rounded-xl border border-line bg-panel p-8">
          <IconLayers className="h-5 w-5 text-soft" />
          <h2 className="mt-5 text-xl font-semibold">It&apos;s not an overlay</h2>
          <p className="mt-2 text-soft">
            Nothing floats on top of the bar. The fill is drawn inside Explorer
            (the real <code className="text-zinc-100">BackgroundFill</code>),
            behind your icons, tray, and clock.
          </p>
        </div>
        <div className="rounded-xl border border-line bg-panel p-8">
          <IconUndo className="h-5 w-5 text-soft" />
          <h2 className="mt-5 text-xl font-semibold">Quitting restores it</h2>
          <p className="mt-2 text-soft">
            Exit from the tray and the normal Windows taskbar brushes come
            straight back. Phostide does not patch system files.
          </p>
        </div>

        <div className="rounded-xl border border-line bg-panel p-8 md:col-span-2">
          <h2 className="text-xl font-semibold">Honest limits</h2>
          <ul className="mt-3 list-disc space-y-2 pl-5 text-soft">
            <li>
              Icons, Start, the tray, and the clock are still drawn by Windows
              and stay static — Phostide only changes the background fill.
            </li>
            <li>
              Motion reduces how long the same background pixels stay locked
              bright; it is not a burn‑in warranty.
            </li>
            <li>
              For the darkest everyday look, start with Space or Aurora and keep
              intensity modest.
            </li>
            <li>
              On multi‑monitor setups you can theme only the OLED panel (Monitors
              tab) or use a quieter theme/intensity on that display alone (Taskbar
              › Apply to that monitor).
            </li>
          </ul>
        </div>

        <div className="rounded-xl border border-line bg-panel p-8 md:col-span-2">
          <h2 className="text-xl font-semibold">Other approaches</h2>
          <p className="mt-3 text-soft">
            Auto‑hide, dimmer overlays, and idle‑fade mods solve the same OLED
            worry differently. See how they compare to painting the real
            Explorer fill.
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
