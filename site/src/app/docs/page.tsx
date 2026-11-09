import type { Metadata } from "next";
import Link from "next/link";
import { LatestDownload } from "@/components/LatestDownload";

export const metadata: Metadata = {
  title: "How to use Phostide",
  description:
    "Install Phostide, pick a theme, and learn how it helps OLED screens.",
};

const pages = [
  {
    href: "/docs/install",
    title: "Install & run",
    body: "Download, open the app, and pick a theme from the tray.",
  },
  {
    href: "/docs/themes",
    title: "Themes",
    body: "Rainbow, Sea Waves, Weather, Space, and Aurora — what each one looks like.",
  },
  {
    href: "/docs/oled",
    title: "OLED notes",
    body: "Why a moving taskbar helps screens, in plain language.",
  },
  {
    href: "/docs/troubleshooting",
    title: "Troubleshooting",
    body: "If the theme does not show up, start here.",
  },
  {
    href: "/docs/privacy",
    title: "Privacy & permissions",
    body: "What runs on your PC, and what never leaves it.",
  },
];

export default function DocsIndex() {
  return (
    <div className="prose-docs">
      <h1 className="font-[family-name:var(--font-display)] text-4xl font-semibold tracking-tight">
        How to use Phostide
      </h1>
      <p className="mt-4 max-w-2xl text-lg">
        Phostide is a small free app for Windows 11. It puts soft animated
        themes on your taskbar so the bar looks better — and moving pixels help
        protect OLED screens from burn-in. Start with install.
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
