import Link from "next/link";

const nav = [
  { href: "/docs", label: "Docs", match: "/docs" },
  { href: "/docs/install", label: "Install", match: "/docs/install" },
  { href: "/docs/themes", label: "Themes", match: "/docs/themes" },
  { href: "/docs/oled", label: "OLED notes", match: "/docs/oled" },
  {
    href: "/docs/troubleshooting",
    label: "Help & Privacy",
    match: "/docs/troubleshooting",
  },
];

export function SiteHeader({ active }: { active?: string }) {
  return (
    <header className="sticky top-0 z-30 border-b border-line bg-black/90 backdrop-blur">
      <div className="mx-auto flex h-16 max-w-site items-center justify-between px-6 sm:px-10">
        <Link
          href="/"
          className="flex items-center gap-3 text-[15px] font-semibold text-zinc-100"
        >
          <span className="grid h-7 w-7 place-items-center rounded-md bg-ember text-xs font-bold text-black">
            P
          </span>
          Phostide
        </Link>
        <nav className="hidden items-center gap-8 text-sm text-soft md:flex">
          {nav.map((item) => {
            const isActive =
              active === item.match ||
              (item.match === "/docs" && active === "/docs");
            return (
              <Link
                key={item.href}
                href={item.href}
                className={
                  isActive ? "text-white" : "transition-colors hover:text-white"
                }
              >
                {item.label}
              </Link>
            );
          })}
        </nav>
        <div className="flex items-center gap-2 sm:gap-3">
          <a
            href="https://github.com/teamdoticca/Phostide"
            className="hidden px-3 py-2 text-sm text-soft transition-colors hover:text-white sm:inline"
            rel="noreferrer"
          >
            GitHub
          </a>
          <Link href="/docs/install#setup" className="btn-ember-sm">
            Download
          </Link>
        </div>
      </div>
    </header>
  );
}
