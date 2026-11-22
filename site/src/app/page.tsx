import type { Metadata } from "next";
import Link from "next/link";
import {
  IconDownload,
  IconGrid,
  IconLayers,
  IconUndo,
} from "@/components/Icons";
import { JsonLd } from "@/components/JsonLd";
import { LatestDownload } from "@/components/LatestDownload";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { HeroDisplay } from "@/components/HeroDisplay";
import { FALLBACK_TAG, resolveReleaseAssets } from "@/lib/releases";
import { pageMetadata, SITE_DESCRIPTION, SITE_NAME, SITE_TAGLINE } from "@/lib/seo";
import { themes } from "@/lib/themes";

export const metadata: Metadata = pageMetadata({
  title: `${SITE_NAME} — ${SITE_TAGLINE}`,
  description: SITE_DESCRIPTION,
  path: "/",
  absoluteTitle: true,
});

export default async function Home() {
  const release = await resolveReleaseAssets();
  const tag = release.tag ?? FALLBACK_TAG;

  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <JsonLd softwareVersion={tag} />
      <SiteHeader />

      <section className="border-b border-line">
        <div className="mx-auto flex min-h-[calc(100svh-4rem)] w-full max-w-site flex-col justify-center px-6 py-10 sm:px-10 sm:py-10">
          <div className="grid gap-8 lg:grid-cols-12 lg:items-end lg:gap-10">
            <div className="lg:col-span-7">
              <span className="inline-flex items-center gap-2 rounded-full border border-line px-3 py-1 text-xs text-soft">
                <span className="h-1.5 w-1.5 rounded-full bg-ember" />
                {tag} · Free · MIT · Windows 11
              </span>
              <h1 className="mt-6 text-4xl font-semibold leading-[1.05] tracking-tight sm:text-5xl lg:text-6xl">
                Burn-in care for the Windows 11 taskbar on OLED screens.
              </h1>
            </div>
            <div className="lg:col-span-5">
              <p className="max-w-xl text-base leading-relaxed text-soft sm:text-lg">
                <span className="sm:hidden">
                  Phostide keeps the taskbar background slowly moving while you
                  work and dims it when you step away.
                </span>
                <span className="hidden sm:inline">
                  The taskbar never leaves your screen. Phostide keeps its
                  background slowly moving while you work and dims it when you
                  step away, so it never sits bright and still for hours. Your
                  icons stay put, and quitting brings back the normal bar.
                </span>
              </p>
              <div className="mt-6 flex flex-col gap-3 sm:mt-7 sm:flex-row sm:flex-wrap sm:items-center">
                <LatestDownload className="btn-ember">
                  Download Setup.exe
                </LatestDownload>
                <LatestDownload kind="zip" className="btn-ghost">
                  Portable zip
                </LatestDownload>
              </div>
              <p className="mt-3 text-xs text-soft sm:mt-4">
                Dims after 5 idle minutes by default · signed installer, no
                admin rights, no account
              </p>
            </div>
          </div>
          <div className="mt-7 sm:mt-10">
            <HeroDisplay themes={themes} />
          </div>
        </div>
      </section>

      <section className="border-b border-line bg-panel">
        <div className="mx-auto grid max-w-site gap-6 px-6 py-20 sm:px-10 sm:py-24 lg:grid-cols-3">
          <div className="mb-2 lg:col-span-3">
            <p className="eyebrow">How it works</p>
            <h2 className="mt-2 text-3xl font-semibold tracking-tight sm:text-4xl">
              Inside the bar, not on top of it.
            </h2>
          </div>
          {[
            {
              icon: IconLayers,
              title: "Real background fill",
              body: "Motion paints Explorer’s own BackgroundFill — D3D11 GPU when the hardware allows, CPU fallback. No floating window covering the bar.",
            },
            {
              icon: IconGrid,
              title: "Idle Care + per monitor",
              body: "OLED Care dims when you AFK (and can dim an unused display alone while you work). Fullscreen pause freezes the covered display so games keep the GPU. Theme only the OLED; leave an LCD stock.",
            },
            {
              icon: IconUndo,
              title: "Quit restores it",
              body: "Close Phostide from the tray and Windows’ normal taskbar (and Care chrome) returns. Nothing left behind.",
            },
          ].map((step) => (
            <div
              key={step.title}
              className="rounded-xl border border-line bg-black p-7"
            >
              <step.icon className="h-5 w-5 text-ember" />
              <h3 className="mt-5 text-lg font-semibold">{step.title}</h3>
              <p className="mt-2 text-sm leading-relaxed text-soft">{step.body}</p>
            </div>
          ))}
        </div>
      </section>

      <section className="mx-auto grid max-w-site items-center gap-12 px-6 py-20 sm:px-10 sm:py-24 lg:grid-cols-2 lg:gap-16">
        <div>
          <p className="eyebrow">Privacy</p>
          <h2 className="mt-2 text-3xl font-semibold tracking-tight sm:text-4xl">
            Local only. That&apos;s the whole policy.
          </h2>
          <p className="mt-5 max-w-lg leading-relaxed text-soft">
            No account to create, no telemetry, no network calls needed to run.
            The source is open under MIT, so you can read every line.
          </p>
        </div>
        <ul className="grid grid-cols-2 gap-4 text-sm">
          {["No account", "No telemetry", "MIT open source", "Signed installer"].map(
            (label) => (
              <li key={label} className="rounded-xl border border-line p-5">
                <p className="font-medium">{label}</p>
              </li>
            )
          )}
        </ul>
      </section>

      <section id="download" className="mx-auto max-w-site px-6 pb-20 sm:px-10 sm:pb-28">
        <div className="rounded-2xl border border-line bg-panel p-8 sm:p-12">
          <div className="mb-10 flex flex-col gap-4 sm:flex-row sm:items-end sm:justify-between">
            <div>
              <p className="eyebrow">Download · {tag}</p>
              <h2 className="mt-2 text-3xl font-semibold tracking-tight sm:text-4xl">
                Get Phostide for Windows 11
              </h2>
            </div>
            <a
              href="https://github.com/teamdoticca/Phostide"
              className="text-sm text-soft hover:text-white"
              rel="noreferrer"
            >
              github.com/teamdoticca/Phostide ↗
            </a>
          </div>
          <div className="grid gap-5 lg:grid-cols-2">
            <div className="flex flex-col rounded-xl border border-ember/50 bg-emberdim/40 p-7">
              <div className="flex flex-wrap items-center justify-between gap-2">
                <span className="text-lg font-semibold">Setup.exe</span>
                <span className="rounded bg-ember px-2 py-1 text-xs font-semibold text-black">
                  Recommended
                </span>
              </div>
              <p className="mt-2 text-sm text-soft">
                Signed, per‑user install. Adds a tray icon and Start menu entry.
                No admin rights.
              </p>
              <LatestDownload className="btn-ember mt-8 self-start">
                <span className="inline-flex items-center gap-2">
                  <IconDownload className="h-4 w-4" />
                  Download installer
                </span>
              </LatestDownload>
            </div>
            <div className="flex flex-col rounded-xl border border-line bg-black p-7">
              <span className="text-lg font-semibold">Portable zip</span>
              <p className="mt-2 text-sm text-soft">
                Unzip anywhere and run. Nothing installed — delete the folder to
                remove.
              </p>
              <LatestDownload kind="zip" className="btn-ghost mt-8 self-start">
                Download zip
              </LatestDownload>
            </div>
          </div>
          <p className="mt-6 text-xs text-soft">
            Both are published on GitHub Releases. Free forever under the MIT
            License.{" "}
            <Link href="/docs" className="text-ember hover:underline">
              Read the docs
            </Link>
          </p>
        </div>
      </section>

      <SiteFooter />
    </div>
  );
}
