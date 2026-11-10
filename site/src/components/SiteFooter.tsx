import Image from "next/image";
import Link from "next/link";

export function SiteFooter() {
  return (
    <footer className="border-t border-line">
      <div className="mx-auto flex max-w-site flex-col gap-3 px-6 py-8 text-sm text-soft sm:flex-row sm:items-center sm:justify-between sm:px-10">
        <span>Phostide · a Doticca utility · MIT</span>
        <nav aria-label="Footer" className="flex flex-wrap gap-5">
          <Link href="/docs" className="hover:text-white">
            Docs
          </Link>
          <Link href="/docs/privacy" className="hover:text-white">
            Privacy
          </Link>
          <a
            href="https://github.com/teamdoticca/Phostide"
            className="hover:text-white"
            rel="noreferrer"
          >
            GitHub
          </a>
        </nav>
      </div>
      <div className="strip">
        <Image
          src="/themes/rainbow.png"
          alt=""
          width={1920}
          height={48}
          className="h-auto w-full"
          unoptimized
        />
      </div>
    </footer>
  );
}
