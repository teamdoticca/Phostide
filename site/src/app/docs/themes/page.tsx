import type { Metadata } from "next";
import Link from "next/link";
import { IconArrowLeft, IconArrowRight, IconSliders } from "@/components/Icons";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { ThemeStrip } from "@/components/ThemeStrip";
import { themes } from "@/lib/themes";

export const metadata: Metadata = {
  title: "Themes",
  description:
    "Five built-in Phostide themes — real taskbar captures, switch from the tray.",
};

export default function ThemesPage() {
  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <SiteHeader active="/docs/themes" />

      <section className="mx-auto grid max-w-site items-end gap-10 px-6 pb-12 pt-16 sm:px-10 sm:pt-20 lg:grid-cols-12">
        <div className="lg:col-span-7">
          <Link
            href="/docs"
            className="inline-flex items-center gap-2 text-sm text-soft hover:text-white"
          >
            <IconArrowLeft />
            Docs
          </Link>
          <p className="mt-6 eyebrow">Themes</p>
          <h1 className="mt-3 text-4xl font-semibold tracking-tight sm:text-5xl">
            Five built‑in themes
          </h1>
          <p className="mt-5 max-w-xl text-lg text-soft">
            Each strip below is a real capture of the taskbar running that theme.
          </p>
        </div>
        <div className="rounded-xl border border-line bg-panel p-6 text-sm lg:col-span-5">
          <p className="inline-flex items-center gap-2 font-medium">
            <IconSliders className="text-ember" />
            Switching themes
          </p>
          <p className="mt-2 text-soft">
            Right‑click the Phostide tray icon › pick a theme. It changes right
            away and is remembered next time.
          </p>
        </div>
      </section>

      <section className="mx-auto max-w-site space-y-5 px-6 pb-20 sm:px-10 sm:pb-24">
        {themes.map((theme, i) => (
          <div
            key={theme.id}
            id={`t-${theme.id}`}
            className="rounded-xl border border-line bg-panel p-6"
          >
            <div className="mb-4 flex flex-wrap items-baseline justify-between gap-2">
              <div>
                <span className="text-lg font-semibold">{theme.name}</span>
                <span className="ml-0 mt-1 block text-sm text-soft sm:ml-4 sm:mt-0 sm:inline">
                  {theme.blurb}
                </span>
              </div>
              <span className="text-sm text-soft">
                {String(i + 1).padStart(2, "0")}
              </span>
            </div>
            <ThemeStrip theme={theme} rounded />
          </div>
        ))}
        <div className="flex justify-between pt-6 text-sm">
          <Link
            href="/docs/install"
            className="inline-flex items-center gap-2 text-soft hover:text-white"
          >
            <IconArrowLeft />
            Install &amp; run
          </Link>
          <Link
            href="/docs/oled"
            className="inline-flex items-center gap-2 text-ember"
          >
            Next: OLED notes <IconArrowRight />
          </Link>
        </div>
      </section>

      <SiteFooter />
    </div>
  );
}
