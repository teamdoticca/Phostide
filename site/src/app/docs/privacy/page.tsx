import type { Metadata } from "next";
import Link from "next/link";
import { IconArrowLeft, IconGithub } from "@/components/Icons";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { pageMetadata } from "@/lib/seo";

export const metadata: Metadata = pageMetadata({
  title: "Privacy",
  description:
    "Phostide is local by default — no account, no telemetry; optional GitHub update checks and signed Setup downloads for a found update only.",
  path: "/docs/privacy",
});

export default function PrivacyPage() {
  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <SiteHeader active="/docs" />

      <section className="mx-auto max-w-site px-6 py-16 sm:px-10 sm:py-20">
        <Link
          href="/docs"
          className="inline-flex items-center gap-2 text-sm text-soft hover:text-white"
        >
          <IconArrowLeft />
          Docs
        </Link>
        <p className="mt-6 eyebrow">Privacy</p>
        <h1 className="mt-3 max-w-2xl text-4xl font-semibold tracking-tight sm:text-5xl">
          Local by default. That&apos;s the policy.
        </h1>
        <p className="mt-5 max-w-2xl text-lg text-soft">
          Phostide does not require an account, does not ship telemetry, and
          does not need the network to paint themes. What it stores and when it
          talks to the network is listed below.
        </p>

        <ul className="mt-12 grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
          {[
            "No account",
            "No telemetry",
            "Themes work offline",
            "MIT open source",
          ].map((label) => (
            <li
              key={label}
              className="rounded-xl border border-line bg-panel p-5 text-sm font-medium"
            >
              {label}
            </li>
          ))}
        </ul>

        <div className="mt-12 max-w-2xl space-y-8 text-soft leading-relaxed">
          <div>
            <h2 className="text-xl font-semibold text-zinc-100">
              What stays on your PC
            </h2>
            <p className="mt-3">
              Settings live at{" "}
              <code className="rounded bg-panel px-1.5 py-0.5 font-mono text-sm text-zinc-100">
                %APPDATA%\Phostide\settings.json
              </code>
              . That file holds fallback theme/intensity, app‑global render
              mode, optional per‑monitor theme/intensity and disabled‑monitor
              lists, Start with Windows preference, enable flag, and (if any)
              pending update tag/URL from a check you already ran. Delete it
              anytime to reset.
            </p>
            <p className="mt-3">
              Start with Windows also writes a value under the current‑user Run
              key so Windows can launch Phostide at sign‑in. Clearing the
              setting in the app removes that value.
            </p>
          </div>

          <div>
            <h2 className="text-xl font-semibold text-zinc-100">
              Optional network use
            </h2>
            <p className="mt-3">
              Checking for updates (automatic about once a day while running,
              unless you turn it off in Settings › System, or when you click
              Check for updates) contacts GitHub&apos;s public
              Releases API for this repository (with Include pre-releases on,
              its release list instead of only the latest release). Themes keep
              working if that request fails or you are offline.
            </p>
            <p className="mt-3">
              When a check finds a new version, Phostide downloads that
              release&apos;s signed Setup from GitHub into your temp folder
              (on a metered connection only once you click Update now) and
              checks it is signed by Doticca SRL and newer than what you have.
              It installs only when you click Update now or Install update.
              Nothing about you is sent along.
            </p>
          </div>

          <div>
            <h2 className="text-xl font-semibold text-zinc-100">
              This website
            </h2>
            <p className="mt-3">
              Everything above is about the app. This site,
              phostide.doticca.com, is hosted on Vercel and uses Vercel Web
              Analytics and Speed Insights to count page views and measure how
              fast pages load. Neither sets cookies, and we see only aggregate
              numbers such as pages visited, referrers, country, browser and
              device type. No personal profile is built, and nothing is sold or
              shared for advertising.
            </p>
            <p className="mt-3">
              The site sends no custom events, so clicks such as Download
              aren&apos;t tracked. The app itself never talks to Vercel.{" "}
              <a
                href="https://vercel.com/docs/analytics/privacy-policy"
                className="text-zinc-200 underline decoration-line underline-offset-4 hover:decoration-zinc-400"
                rel="noreferrer"
              >
                How Vercel Analytics handles data
              </a>
            </p>
          </div>

          <div>
            <h2 className="text-xl font-semibold text-zinc-100">
              Builds &amp; source
            </h2>
            <p className="mt-3">
              The installer and portable builds are published on GitHub Releases
              and signed by Doticca SRL. Source is MIT so you can read every
              line.
            </p>
          </div>
        </div>

        <a
          href="https://github.com/teamdoticca/Phostide"
          className="mt-10 inline-flex items-center gap-2 rounded-lg border border-line bg-panel px-5 py-3 text-sm hover:border-zinc-600"
          rel="noreferrer"
        >
          <IconGithub className="h-4 w-4" />
          github.com/teamdoticca/Phostide
        </a>
      </section>

      <SiteFooter />
    </div>
  );
}
