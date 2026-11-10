import type { Metadata } from "next";
import Link from "next/link";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";

export const metadata: Metadata = {
  title: "Page not found",
  robots: { index: false, follow: true },
};

export default function NotFound() {
  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <SiteHeader />
      <main className="mx-auto max-w-site px-6 py-24 sm:px-10">
        <p className="eyebrow">404</p>
        <h1 className="mt-2 text-4xl font-semibold tracking-tight">
          Page not found
        </h1>
        <p className="mt-4 max-w-md text-soft">
          That URL isn&apos;t on this site. Try the home page or the docs.
        </p>
        <div className="mt-8 flex flex-wrap gap-3">
          <Link href="/" className="btn-ember">
            Home
          </Link>
          <Link href="/docs" className="btn-ghost">
            Docs
          </Link>
        </div>
      </main>
      <SiteFooter />
    </div>
  );
}
