import type { Metadata } from "next";
import Link from "next/link";
import { IconArrowLeft, IconGithub } from "@/components/Icons";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { pageMetadata } from "@/lib/seo";

export const metadata: Metadata = pageMetadata({
  title: "Privacy",
  description:
    "Phostide is local by default — no account, no telemetry; optional GitHub update checks only.",
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
              . That file holds your theme, intensity, Start with Windows
              preference, enable flag, and (if any) pending update tag/URL from
              a check you already ran. Delete it anytime to reset.
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
              Checking for updates (automatic about once a day while running, or
              when you click Check for updates) contacts GitHub&apos;s public
              Releases API for this repository. Themes keep working if that
              request fails or you are offline.
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
