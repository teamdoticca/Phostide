import type { Metadata } from "next";
import Link from "next/link";
import { IconArrowLeft, IconGithub } from "@/components/Icons";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { pageMetadata } from "@/lib/seo";

export const metadata: Metadata = pageMetadata({
  title: "Privacy",
  description:
    "Phostide is local only — no account, no telemetry, MIT open source.",
  path: "/docs/privacy",
});

export default function PrivacyPage() {
  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <SiteHeader active="/docs/troubleshooting" />

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
          Local only. That&apos;s the whole policy.
        </h1>
        <p className="mt-5 max-w-2xl text-lg text-soft">
          Phostide runs entirely on your PC. There&apos;s no account, no
          telemetry, and it doesn&apos;t need network access to work. The only
          thing it saves is your theme choice in a local settings file.
        </p>

        <ul className="mt-12 grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
          {["Local only", "No account", "No telemetry", "MIT open source"].map(
            (label) => (
              <li
                key={label}
                className="rounded-xl border border-line bg-panel p-5 text-sm font-medium"
              >
                {label}
              </li>
            )
          )}
        </ul>

        <div className="mt-12 max-w-2xl space-y-4 text-soft leading-relaxed">
          <p>
            Settings live at{" "}
            <code className="rounded bg-panel px-1.5 py-0.5 font-mono text-sm text-zinc-100">
              %APPDATA%\Phostide\settings.json
            </code>
            . Delete that file anytime to reset.
          </p>
          <p>
            The installer and portable builds are published on GitHub Releases
            and signed by Doticca SRL. Source code is available under the MIT
            License so you can read every line.
          </p>
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
