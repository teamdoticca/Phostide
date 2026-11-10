import type { Metadata } from "next";
import Link from "next/link";
import { IconArrowLeft, IconGithub } from "@/components/Icons";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { pageMetadata } from "@/lib/seo";

export const metadata: Metadata = pageMetadata({
  title: "Troubleshooting",
  description:
    "Fix theme not showing, Explorer restarts, SmartScreen warnings, and restore the default taskbar.",
  path: "/docs/troubleshooting",
});

const fixes = [
  {
    id: "fix-theme",
    title: "Theme not showing",
    items: [
      "Check you're on Windows 11 22H2 or newer, x64.",
      "Make sure ExplorerTAP.dll sits next to Phostide.exe.",
      "Quit from the tray and open Phostide again.",
      "Pick a different theme from the tray, then switch back.",
    ],
  },
  {
    id: "fix-explorer",
    title: "After Explorer restarts",
    items: [
      "If Explorer crashed or was restarted, the theme may drop.",
      "Quit Phostide from the tray and start it again.",
      "To restart Explorer yourself: Task Manager › Windows Explorer › Restart.",
    ],
  },
  {
    id: "fix-smartscreen",
    title: "SmartScreen warning",
    items: [
      "New releases can trigger SmartScreen until they build reputation.",
      'Check the publisher shows Doticca SRL.',
      'Click "More info" › "Run anyway". Only download from GitHub Releases.',
    ],
  },
  {
    id: "fix-restore",
    title: "Restore the default taskbar",
    items: [
      "Right‑click the tray icon › Quit. The normal bar returns.",
      "Still odd? Restart Windows Explorer from Task Manager.",
      "Reset settings by deleting %APPDATA%\\Phostide\\settings.json.",
    ],
  },
];

export default function TroubleshootingPage() {
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
        <p className="mt-6 eyebrow">Troubleshooting</p>
        <h1 className="mt-3 text-4xl font-semibold tracking-tight sm:text-5xl">
          Common fixes
        </h1>
        <p className="mt-5 max-w-2xl text-lg text-soft">
          Most issues are solved in under a minute. If not, open an issue on
          GitHub.
        </p>
        <div className="mt-12 grid gap-5 lg:grid-cols-2">
          {fixes.map((fix) => (
            <div
              key={fix.id}
              id={fix.id}
              className="rounded-xl border border-line bg-panel p-7"
            >
              <h2 className="text-lg font-semibold">{fix.title}</h2>
              <ul className="mt-4 list-disc space-y-2 pl-5 text-sm text-soft">
                {fix.items.map((item) => (
                  <li key={item}>{item}</li>
                ))}
              </ul>
            </div>
          ))}
        </div>
      </section>

      <section id="privacy" className="border-y border-line bg-panel">
        <div className="mx-auto grid max-w-site items-center gap-12 px-6 py-16 sm:px-10 sm:py-20 lg:grid-cols-2 lg:gap-16">
          <div>
            <p className="eyebrow">Privacy</p>
            <h2 className="mt-2 text-3xl font-semibold tracking-tight sm:text-4xl">
              Local only.
            </h2>
            <p className="mt-5 max-w-lg leading-relaxed text-soft">
              Phostide runs entirely on your PC. There&apos;s no account, no
              telemetry, and it doesn&apos;t send your data anywhere. The only
              thing it saves is your settings file. The code is open under MIT,
              so you can check for yourself.
            </p>
            <a
              href="https://github.com/teamdoticca/Phostide"
              className="mt-7 inline-flex items-center gap-2 rounded-lg border border-line bg-black px-5 py-3 text-sm hover:border-zinc-600"
              rel="noreferrer"
            >
              <IconGithub className="h-4 w-4" />
              github.com/teamdoticca/Phostide
            </a>
            <p className="mt-4 text-sm text-soft">
              Full write‑up:{" "}
              <Link href="/docs/privacy" className="text-ember hover:underline">
                Privacy page
              </Link>
            </p>
          </div>
          <ul className="grid grid-cols-2 gap-4 text-sm">
            {["Local only", "No account", "No telemetry", "MIT open source"].map(
              (label) => (
                <li
                  key={label}
                  className="rounded-xl border border-line bg-black p-5"
                >
                  <p className="font-medium">{label}</p>
                </li>
              )
            )}
          </ul>
        </div>
      </section>

      <SiteFooter />
    </div>
  );
}
