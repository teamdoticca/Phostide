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
      className={`mx-auto flex w-full max-w-6xl items-center justify-between px-6 py-6 ${
        solid ? "" : "absolute inset-x-0 top-0 z-20"
      }`}
    >
      <Link href="/" className="flex items-center gap-2.5">
        <Image
          src="/mark-64.png"
          alt=""
          width={28}
          height={28}
          className="h-7 w-7"
          priority
        />
        <span className="font-[family-name:var(--font-display)] text-lg font-semibold tracking-tight">
          Phostide
        </span>
      </Link>
      <nav className="flex items-center gap-5 text-sm text-[var(--muted)]">
        {links.map((link) =>
          link.external ? (
            <a
              key={link.href}
              href={link.href}
              className="transition-colors hover:text-[var(--ink)]"
              rel="noreferrer"
            >
              {link.label}
            </a>
          ) : (
            <Link
              key={link.href}
              href={link.href}
              className="transition-colors hover:text-[var(--ink)]"
            >
              {link.label}
            </Link>
          )
        )}
      </nav>
    </header>
  );
}
