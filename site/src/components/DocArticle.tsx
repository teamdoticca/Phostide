import Link from "next/link";
import type { ReactNode } from "react";
import { IconArrowLeft, IconArrowRight } from "@/components/Icons";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";

type NavLink = { href: string; label: string };

/** Read-mode page shell for long-form docs: one column, generous measure. */
export function DocArticle({
  active,
  back = { href: "/docs", label: "Docs" },
  title,
  lede,
  children,
  related,
  prev,
  next,
}: {
  active: string;
  back?: NavLink;
  title: string;
  lede: ReactNode;
  children: ReactNode;
  related?: (NavLink & { body: string })[];
  prev?: NavLink;
  next?: NavLink;
}) {
  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <SiteHeader active={active} />
      <main className="mx-auto max-w-site px-6 py-16 sm:px-10 sm:py-20">
        <Link
          href={back.href}
          className="inline-flex items-center gap-2 text-sm text-soft hover:text-white"
        >
          <IconArrowLeft />
          {back.label}
        </Link>
        <h1 className="mt-6 max-w-3xl text-4xl font-semibold leading-[1.1] tracking-tight sm:text-5xl">
          {title}
        </h1>
        <div className="mt-6 max-w-2xl text-lg leading-relaxed text-soft">{lede}</div>

        <div className="doc-prose mt-14 max-w-2xl">{children}</div>

        {related && related.length > 0 ? (
          <section className="mt-20 max-w-3xl border-t border-line pt-10">
            <h2 className="text-xl font-semibold">Related</h2>
            <ul className="mt-6 grid gap-6 sm:grid-cols-2">
              {related.map((r) => (
                <li key={r.href}>
                  <Link href={r.href} className="group block">
                    <span className="inline-flex items-center gap-2 font-medium text-zinc-100 group-hover:text-ember">
                      {r.label} <IconArrowRight />
                    </span>
                    <span className="mt-1 block text-sm text-soft">{r.body}</span>
                  </Link>
                </li>
              ))}
            </ul>
          </section>
        ) : null}

        {prev || next ? (
          <div className="mt-12 flex flex-wrap justify-between gap-4 text-sm">
            {prev ? (
              <Link href={prev.href} className="inline-flex items-center gap-2 text-soft hover:text-white">
                <IconArrowLeft />
                {prev.label}
              </Link>
            ) : (
              <span />
            )}
            {next ? (
              <Link href={next.href} className="inline-flex items-center gap-2 text-ember">
                {next.label} <IconArrowRight />
              </Link>
            ) : null}
          </div>
        ) : null}
      </main>
      <SiteFooter />
    </div>
  );
}
