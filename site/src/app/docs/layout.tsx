import Link from "next/link";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";

const nav = [
  { href: "/docs", label: "Overview" },
  { href: "/docs/install", label: "Install & run" },
  { href: "/docs/themes", label: "Themes" },
  { href: "/docs/oled", label: "OLED notes" },
  { href: "/docs/troubleshooting", label: "Troubleshooting" },
  { href: "/docs/privacy", label: "Privacy" },
];

export default function DocsLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <div className="min-h-screen">
      <div className="border-b border-[var(--line)] bg-[var(--bg0)]/90 backdrop-blur-sm">
        <SiteHeader solid />
      </div>
      <div className="mx-auto grid w-full max-w-6xl gap-10 px-6 py-12 lg:grid-cols-[220px_1fr]">
        <aside className="lg:sticky lg:top-8 lg:self-start">
          <p className="font-[family-name:var(--font-display)] text-sm font-semibold tracking-tight text-[var(--ink)]">
            Documentation
          </p>
          <nav className="mt-4 flex flex-row flex-wrap gap-x-4 gap-y-2 lg:flex-col lg:gap-2">
            {nav.map((item) => (
              <Link
                key={item.href}
                href={item.href}
                className="text-sm text-[var(--muted)] transition-colors hover:text-[var(--accent)]"
              >
                {item.label}
              </Link>
            ))}
          </nav>
        </aside>
        <article className="min-w-0 pb-16">{children}</article>
      </div>
      <SiteFooter />
    </div>
  );
}
