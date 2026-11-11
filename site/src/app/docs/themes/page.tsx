import type { Metadata } from "next";
import Link from "next/link";
import { IconArrowLeft, IconArrowRight, IconSliders } from "@/components/Icons";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { ThemeStrip } from "@/components/ThemeStrip";
import { pageMetadata } from "@/lib/seo";
import { themes } from "@/lib/themes";

export const metadata: Metadata = pageMetadata({
  title: "Themes",
  description:
    "Five built-in Phostide themes — real taskbar captures, switch from Settings.",
  path: "/docs/themes",
});

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
            Each strip is a real Native‑48px capture of the taskbar painter.
            Hover a strip on desktop to play the loop.
          </p>
        </div>
        <div className="rounded-xl border border-line bg-panel p-6 text-sm lg:col-span-5">
          <p className="inline-flex items-center gap-2 font-medium">
            <IconSliders className="text-ember" />
            How to switch
          </p>
          <ol className="mt-3 list-decimal space-y-2 pl-5 text-soft">
            <li>Right‑click the tray icon › Settings…</li>
            <li>On the Taskbar tab, pick a theme and set intensity.</li>
            <li>Click Apply — the live bar updates immediately.</li>
          </ol>
          <p className="mt-4 text-soft">
            Default intensity is 45%. Lower keeps OLED quieter; higher makes
            motion and color stronger. All five themes use the same GPU or CPU
            painter (System › RENDERING) — the look matches; GPU is smoother and
            lighter on the CPU.
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
                  {theme.detail}
                </span>
              </div>
              <span className="text-sm text-soft">
                {String(i + 1).padStart(2, "0")}
              </span>
            </div>
            <ThemeStrip theme={theme} rounded />
          </div>
        ))}

        <div className="rounded-xl border border-line bg-panel p-6 text-sm text-soft">
          <p className="font-medium text-zinc-100">Tips</p>
          <ul className="mt-3 list-disc space-y-2 pl-5">
            <li>
              Space and Aurora stay darkest — good everyday OLED choices.
            </li>
            <li>
              Weather and Sea Waves are busier; dial intensity down if you want
              calmer motion.
            </li>
            <li>
              Tray › Enable Phostide unchecks to pause painting without losing
              your settings.
            </li>
          </ul>
        </div>

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
