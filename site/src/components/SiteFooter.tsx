import Link from "next/link";

export function SiteFooter() {
  return (
    <footer className="mx-auto flex w-full max-w-[72rem] flex-col gap-4 border-t border-[var(--line)] px-6 py-10 text-sm text-[var(--muted)] sm:flex-row sm:items-center sm:justify-between sm:px-8">
      <span>© {new Date().getFullYear()} Doticca · Phostide · MIT</span>
      <div className="flex flex-wrap gap-6">
        <Link className="link-nav" href="/docs">
          Docs
        </Link>
        <a
          className="link-nav"
          href="https://github.com/teamdoticca/Phostide/releases"
        >
          Releases
        </a>
        <a className="link-nav" href="https://www.doticca.com">
          doticca.com
        </a>
      </div>
    </footer>
  );
}
