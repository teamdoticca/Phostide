import type { Metadata } from "next";
import Link from "next/link";
import { IconDownload } from "@/components/Icons";
import {
  HomeFaqSection,
  MultiMonitorSection,
  ProblemSection,
  TradeOffSection,
  TrustSection,
} from "@/components/HomeSections";
import { JsonLd } from "@/components/JsonLd";
import { LatestDownload } from "@/components/LatestDownload";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { HeroDisplay } from "@/components/HeroDisplay";
import { HeroDownload } from "@/components/HeroDownload";
import { FALLBACK_TAG, resolveReleaseAssets } from "@/lib/releases";
import {
  HOME_TITLE,
  pageMetadata,
  SITE_DESCRIPTION,
  SITE_URL,
} from "@/lib/seo";
import { themes } from "@/lib/themes";

export const metadata: Metadata = pageMetadata({
  title: HOME_TITLE,
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
              <span className="inline-flex items-center gap-2 rounded-full border border-line px-3 py-1 text-[13px] text-soft">
                <span className="h-1.5 w-1.5 rounded-full bg-ember" />
                OLED burn-in care for Windows 11
                <span className="hidden sm:inline"> · Free &amp; open source</span>
              </span>
              <h1 className="mt-6 text-[2.25rem] font-semibold leading-[1.04] tracking-[-0.035em] min-[400px]:text-[2.5rem] sm:text-6xl xl:text-7xl">
                <span className="block">Keep your taskbar.</span>
                <span className="block">Spare your OLED.</span>
              </h1>
            </div>
            <div className="lg:col-span-5">
              <p className="max-w-xl text-base leading-relaxed text-soft sm:text-lg">
                The taskbar is the one thing that never leaves your screen.
                Phostide keeps its background slowly moving while you work, dims
                it when you step away and pauses it when you game, so the same
                pixels never sit bright for hours. Your icons stay put. Quit, and
                the normal taskbar is back.
              </p>
              <div className="mt-6 sm:mt-7">
                <HeroDownload
                  setupUrl={release.setupUrl}
                  zipUrl={release.zipUrl}
                  pageUrl={SITE_URL}
                />
              </div>
            </div>
          </div>
          <div className="mt-7 sm:mt-10">
            <HeroDisplay themes={themes} />
          </div>
        </div>
      </section>

      <ProblemSection />
      <TradeOffSection />
      <MultiMonitorSection />
      <TrustSection />
      <HomeFaqSection />

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
