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
          taskbar is about the most static thing on a Windows screen.
        </p>
      </section>

      <section className="border-y border-line">
        <div className="mx-auto flex max-w-site justify-between px-6 py-3 text-xs text-soft sm:px-10">
          <span>Space theme · mostly black, always moving</span>
          <span>1920 × 48, real taskbar</span>
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
          </p>
        </div>
        <div className="rounded-xl border border-line bg-panel p-8">
          <IconEye className="h-5 w-5 text-soft" />
          <h2 className="mt-5 text-xl font-semibold">It&apos;s not auto‑hide</h2>
          <p className="mt-2 text-soft">
            The taskbar stays visible and usable. You don&apos;t have to hunt for
            it at the screen edge.
          </p>
        </div>
        <div className="rounded-xl border border-line bg-panel p-8">
          <IconLayers className="h-5 w-5 text-soft" />
          <h2 className="mt-5 text-xl font-semibold">It&apos;s not an overlay</h2>
          <p className="mt-2 text-soft">
            Nothing floats on top of the bar. The fill is drawn inside Explorer,
            behind your icons, tray and clock.
          </p>
        </div>
        <div className="rounded-xl border border-line bg-panel p-8">
          <IconUndo className="h-5 w-5 text-soft" />
          <h2 className="mt-5 text-xl font-semibold">Quitting restores it</h2>
          <p className="mt-2 text-soft">
            Close Phostide from the tray and the normal Windows taskbar comes
            straight back.
          </p>
        </div>
        <p className="mt-4 text-sm text-soft md:col-span-2">
          Note: icons and the clock are still drawn by Windows and stay static.
          Phostide only changes the background.
        </p>
        <div className="flex justify-between pt-4 text-sm md:col-span-2">
          <Link
            href="/docs/themes"
            className="inline-flex items-center gap-2 text-soft hover:text-white"
          >
            <IconArrowLeft />
            Themes
          </Link>
          <Link
            href="/docs/troubleshooting"
            className="inline-flex items-center gap-2 text-ember"
          >
            Next: Troubleshooting <IconArrowRight />
          </Link>
        </div>
      </section>

      <SiteFooter />
    </div>
  );
}
