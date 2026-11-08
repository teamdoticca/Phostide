import Link from "next/link";

export function SiteFooter() {
  return (
    <footer className="mx-auto flex w-full max-w-6xl flex-col gap-4 border-t border-[var(--line)] px-6 py-10 text-sm text-[var(--muted)] sm:flex-row sm:items-center sm:justify-between">
      <span>© {new Date().getFullYear()} Doticca · Phostide · MIT</span>
      <div className="flex flex-wrap gap-5">
        <Link className="hover:text-[var(--ink)]" href="/docs">
          Docs
        </Link>
        <a
          className="hover:text-[var(--ink)]"
          href="https://github.com/teamdoticca/Phostide/releases"
        >
          Releases
        </a>
        <a className="hover:text-[var(--ink)]" href="https://www.doticca.com">
          doticca.com
        </a>
      </div>
    </footer>
  );
}
