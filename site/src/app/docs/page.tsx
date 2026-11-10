import type { Metadata } from "next";
import Link from "next/link";
import {
  IconArrowRight,
  IconDisplay,
  IconDownload,
  IconGithub,
  IconLock,
  IconPalette,
  IconWrench,
} from "@/components/Icons";
import { LatestDownload } from "@/components/LatestDownload";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { resolveReleaseAssets } from "@/lib/releases";

export const metadata: Metadata = {
  title: "Docs",
  description: "Install Phostide, pick a theme, and fix the odd hiccup.",
};

const cards = [
  {
    href: "/docs/install",
    icon: IconDownload,
    title: "Install & run",
    body: "Setup.exe, portable zip, where files go, uninstall.",
  },
  {
    href: "/docs/themes",
    icon: IconPalette,
    title: "Themes",
    body: "The five built‑in looks and how to switch from the tray.",
  },
  {
    href: "/docs/oled",
    icon: IconDisplay,
    title: "OLED notes",
    body: "Why a static taskbar matters on OLED, and what Phostide does about it.",
  },
  {
    href: "/docs/troubleshooting",
    icon: IconWrench,
    title: "Troubleshooting",
    body: "Theme not showing, Explorer restarts, SmartScreen, reset.",
  },
  {
    href: "/docs/privacy",
    icon: IconLock,
    title: "Privacy",
    body: "Local only. No account, no telemetry, open source.",
  },
  {
    href: "https://github.com/teamdoticca/Phostide",
    icon: IconGithub,
    title: "Source & releases",
    body: "github.com/teamdoticca/Phostide",
    external: true,
  },
];

export default async function DocsPage() {
  const { tag } = await resolveReleaseAssets();
  const releaseTag = tag ?? "v0.1.5";

  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <SiteHeader active="/docs" />

      <section className="border-b border-line">
        <div className="mx-auto grid max-w-site gap-10 px-6 py-16 sm:px-10 sm:py-20 lg:grid-cols-12">
          <div className="lg:col-span-7">
            <p className="eyebrow">Docs · {releaseTag}</p>
            <h1 className="mt-3 text-4xl font-semibold leading-tight tracking-tight sm:text-5xl">
              Everything you need to run Phostide.
            </h1>
            <p className="mt-5 max-w-xl text-lg text-soft">
              Short pages. Install it, pick a theme, fix the odd hiccup.
              That&apos;s about it.
            </p>
            <div className="mt-8 flex flex-wrap gap-3">
              <LatestDownload className="btn-ember">
                Download Setup.exe
              </LatestDownload>
              <LatestDownload kind="zip" className="btn-ghost">
                Portable zip
              </LatestDownload>
            </div>
          </div>
          <div className="lg:col-span-5">
            <div className="divide-y divide-line rounded-2xl border border-line bg-panel text-sm">
              {[
                ["Release", releaseTag],
                ["Requires", "Windows 11 22H2+ · x64"],
                ["Install", "Per‑user, no admin"],
                ["Signed by", "Doticca SRL"],
                ["License", "MIT"],
              ].map(([k, v]) => (
                <div key={k} className="flex justify-between gap-4 px-6 py-4">
                  <span className="text-soft">{k}</span>
                  <span className="text-right">{v}</span>
                </div>
              ))}
            </div>
          </div>
        </div>
      </section>

      <section className="mx-auto max-w-site px-6 py-16 sm:px-10 sm:py-20">
        <h2 className="mb-8 text-2xl font-semibold">Pages</h2>
        <div className="grid gap-5 md:grid-cols-2 lg:grid-cols-3">
          {cards.map((card) => {
            const className =
              "group rounded-xl border border-line bg-panel p-7 transition-colors hover:border-ember/60";
            const content = (
              <>
                <card.icon className="h-5 w-5 text-ember" />
                <h3 className="mt-5 text-lg font-semibold">{card.title}</h3>
                <p className="mt-2 text-sm text-soft">{card.body}</p>
                <span className="mt-6 inline-flex items-center gap-1 text-sm text-soft group-hover:text-white">
                  {card.external ? "Open" : "Read"}{" "}
                  <IconArrowRight />
                </span>
              </>
            );
            return card.external ? (
              <a key={card.href} href={card.href} className={className} rel="noreferrer">
                {content}
              </a>
            ) : (
              <Link key={card.href} href={card.href} className={className}>
                {content}
              </Link>
            );
          })}
        </div>
      </section>

      <SiteFooter />
    </div>
  );
}
