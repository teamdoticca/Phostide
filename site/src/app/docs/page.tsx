import type { Metadata } from "next";
import Link from "next/link";
import { LatestDownload } from "@/components/LatestDownload";

export const metadata: Metadata = {
  title: "Documentation",
  description:
    "Install Phostide, choose themes, and keep your OLED taskbar healthy.",
};

const pages = [
  {
    href: "/docs/install",
    title: "Install & run",
    body: "Download the release, launch the tray host, and confirm themes bind.",
  },
  {
    href: "/docs/themes",
    title: "Themes",
    body: "Rainbow, Sea Waves, Weather, Space, and Aurora — what each paint does.",
  },
  {
    href: "/docs/oled",
    title: "OLED notes",
    body: "Why motion matters, luminance habits, and what Phostide does not claim.",
  },
  {
    href: "/docs/troubleshooting",
    title: "Troubleshooting",
    body: "Inject without killing Explorer, log paths, and common bind failures.",
  },
  {
    href: "/docs/privacy",
    title: "Privacy & permissions",
    body: "What runs locally, what permissions mean, and what never leaves your PC.",
  },
];

export default function DocsIndex() {
  return (
    <div className="prose-docs">
      <h1 className="font-[family-name:var(--font-display)] text-4xl font-semibold tracking-tight">
        Documentation
      </h1>
      <p className="mt-4 max-w-2xl text-lg">
        Phostide paints animated themes into the real Windows 11 taskbar
        background. Start with install, then skim themes and OLED guidance.
      </p>

      <div className="mt-8 flex flex-wrap gap-3">
        <LatestDownload />
        <a
          href="https://github.com/teamdoticca/Phostide"
          className="btn-ghost"
        >
          Source on GitHub
        </a>
      </div>

      <ul className="mt-14 grid list-none gap-6 !p-0 sm:grid-cols-2">
        {pages.map((page) => (
          <li
            key={page.href}
            className="!m-0 border-t border-[var(--line)] pt-5"
          >
            <Link
              href={page.href}
              className="!text-[var(--ink)] !no-underline hover:!text-[var(--accent)]"
            >
              <span className="font-[family-name:var(--font-display)] text-xl font-medium">
                {page.title}
              </span>
            </Link>
            <p className="!mt-2 text-sm">{page.body}</p>
          </li>
        ))}
      </ul>
    </div>
  );
}
