import Image from "next/image";
import Link from "next/link";

const links = [
  { href: "/#themes", label: "Themes" },
  { href: "/docs", label: "Docs" },
  { href: "https://github.com/teamdoticca/Phostide", label: "GitHub", external: true },
];

export function SiteHeader({ solid = false }: { solid?: boolean }) {
  return (
    <header
      className={`mx-auto flex w-full max-w-[72rem] items-center justify-between px-6 py-5 sm:px-8 sm:py-6 ${
        solid ? "border-b border-[var(--line)] bg-[var(--bg0)]" : "absolute inset-x-0 top-0 z-20"
      }`}
    >
      <Link href="/" className="group flex items-center gap-2.5">
        <Image
          src="/mark-64.png"
          alt=""
          width={28}
          height={28}
          className="h-7 w-7 transition-transform duration-200 group-hover:scale-105"
          priority
        />
        <span className="font-[family-name:var(--font-display)] text-lg font-bold tracking-tight">
          Phostide
        </span>
      </Link>
      <nav className="flex items-center gap-6 text-sm text-[var(--muted)]">
        {links.map((link) =>
          link.external ? (
            <a
              key={link.href}
              href={link.href}
              className="link-nav"
              rel="noreferrer"
            >
              {link.label}
            </a>
          ) : (
            <Link key={link.href} href={link.href} className="link-nav">
              {link.label}
            </Link>
          )
        )}
      </nav>
    </header>
  );
}
