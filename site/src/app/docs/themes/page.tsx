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
    "Five built-in Phostide themes for everyday OLED use — real taskbar captures; pair with OLED Care for idle.",
  path: "/docs/themes",
});

function Meter({ label, value }: { label: string; value: number }) {
  return (
    <>
      <dt className="text-soft">{label}</dt>
      <dd className="flex items-center gap-1" aria-label={`${label}: ${value} of 5`}>
        {[1, 2, 3, 4, 5].map((n) => (
          <span
            key={n}
            aria-hidden="true"
            className={`h-2 w-5 rounded-full ${n <= value ? "bg-zinc-200" : "bg-white/10"}`}
          />
        ))}
      </dd>
    </>
  );
}

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
            Five looks for everyday OLED use
          </h1>
          <p className="mt-5 max-w-xl text-lg text-soft">
            Themes keep the fill moving while you work — the everyday layer of
            Phostide&apos;s OLED care. Pair with{" "}
            <Link href="/docs/oled" className="text-ember hover:underline">
              OLED Care
            </Link>{" "}
            for idle. Each strip is a real Native‑48px capture. No theme editor;
            no Windhawk or overlay tool required.
          </p>
        </div>
        <div className="rounded-xl border border-line bg-panel p-6 text-sm lg:col-span-5">
          <p className="inline-flex items-center gap-2 font-medium">
            <IconSliders className="text-ember" />
            How to switch
          </p>
          <ol className="mt-3 list-decimal space-y-2 pl-5 text-soft">
            <li>Right‑click the tray icon › Settings…</li>
            <li>
              On the Taskbar tab, choose Apply to All or a monitor number, then
              pick theme and intensity.
            </li>
            <li>Click Apply — the live bar(s) update immediately.</li>
          </ol>
          <p className="mt-4 text-soft">
            Default intensity is 35% on a fresh install. Lower keeps OLED quieter; higher makes
            motion and color stronger. All five themes use the same GPU or CPU
            painter (System › RENDERING) — the look matches; GPU is smoother and
            lighter on the CPU.
          </p>
        </div>
      </section>

      <section className="mx-auto max-w-site space-y-5 px-6 pb-20 sm:px-10 sm:pb-24">
        <p className="max-w-3xl text-sm leading-relaxed text-soft">
          Hover a strip to play it. The meters compare the five themes with
          each other, measured from these captures at 100% intensity: average
          brightness over every pixel and frame, and how much the picture
          changes from one frame to the next. A lower intensity makes every
          theme dimmer.
        </p>
        {themes.map((theme, i) => (
          <div
            key={theme.id}
            id={`t-${theme.id}`}
            className="rounded-xl border border-line bg-panel p-6"
          >
            <div className="mb-4 grid gap-4 md:grid-cols-[minmax(0,1fr)_auto] md:items-start">
              <div>
                <p className="flex items-baseline gap-3">
                  <span className="text-sm tabular-nums text-soft">
                    {String(i + 1).padStart(2, "0")}
                  </span>
                  <span className="text-lg font-semibold">{theme.name}</span>
                </p>
                <p className="mt-1 text-sm text-soft">{theme.detail}</p>
                {theme.note ? (
                  <p className="mt-2 max-w-xl text-sm text-zinc-300">{theme.note}</p>
                ) : null}
              </div>
              <dl className="grid grid-cols-[auto_auto] items-center gap-x-4 gap-y-2 text-sm">
                <Meter label="Brightness" value={theme.brightness} />
                <Meter label="Motion" value={theme.movement} />
              </dl>
            </div>
            <ThemeStrip theme={theme} rounded />
          </div>
        ))}

        <div className="rounded-xl border border-line bg-panel p-6 text-sm text-soft">
          <p className="font-medium text-zinc-100">Tips</p>
          <ul className="mt-3 list-disc space-y-2 pl-5">
            <li>
              Aurora is the darkest on average and the best everyday OLED
              choice. Space is near-black most of the time but brightens for
              its nebula pass.
            </li>
            <li>
              Weather and Sea Waves are busier; dial intensity down if you want
              calmer motion.
            </li>
            <li>
              Tray › Enable Phostide unchecks to pause painting without losing
              your settings.
            </li>
            <li>
              Use the Monitors tab to leave one display stock while theming the
              others — or set a different theme per bar from Taskbar › Apply to.
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
