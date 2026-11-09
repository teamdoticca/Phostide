import Image from "next/image";
import Link from "next/link";
import { LatestDownload } from "@/components/LatestDownload";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { ThemePreview } from "@/components/ThemePreview";
import { themes } from "@/lib/themes";

export default function Home() {
  return (
    <div className="relative min-h-screen overflow-x-hidden bg-[var(--bg0)]">
      <SiteHeader />

      {/* Hero — wave-only atmosphere; single brand wordmark */}
      <section className="relative flex min-h-[100svh] items-end overflow-hidden">
        <div className="absolute inset-0" aria-hidden>
          <Image
            src="/hero-wave.jpg"
            alt=""
            fill
            priority
            className="hero-wave object-cover object-[center_60%]"
            sizes="100vw"
          />
          <div className="absolute inset-0 bg-gradient-to-t from-[var(--bg0)] via-[rgba(2,4,8,0.55)] to-[rgba(2,4,8,0.25)]" />
          <div className="absolute inset-0 bg-[radial-gradient(ellipse_at_70%_40%,rgba(56,189,248,0.12),transparent_55%)]" />
        </div>

        <div className="relative z-10 mx-auto flex w-full max-w-[72rem] flex-col gap-5 px-6 pb-16 pt-28 sm:px-8 sm:pb-24">
          <h1 className="animate-rise max-w-[11ch] font-[family-name:var(--font-display)] text-[clamp(4.25rem,14vw,8.5rem)] font-bold leading-[0.86] tracking-[-0.045em] text-[var(--ink)]">
            Phostide
          </h1>
          <p className="animate-rise-delay-1 max-w-xl text-[1.35rem] leading-snug text-[var(--ink)] sm:text-2xl">
            OLED-safe themes on the real Windows taskbar.
          </p>
          <p className="animate-rise-delay-2 max-w-md text-base leading-relaxed text-[var(--muted)] sm:text-lg">
            Rainbow, Sea Waves, Weather, Space, and Aurora paint Explorer’s own
            background fill — not an overlay.
          </p>
          <div className="animate-rise-delay-3 flex flex-wrap items-center gap-3 pt-3">
            <LatestDownload />
            <Link href="/docs" className="btn-ghost">
              Documentation
            </Link>
          </div>
          <p className="font-[family-name:var(--font-mono)] text-[0.7rem] tracking-wide text-[var(--muted)]">
            Windows 11 22H2+ · x64 · MIT
          </p>
        </div>
      </section>

      <main>
        {/* Proof — one job */}
        <section className="relative border-t border-[var(--line)]">
          <div className="mx-auto grid w-full max-w-[72rem] gap-12 px-6 py-24 sm:px-8 sm:py-32 lg:grid-cols-[1.15fr_0.85fr] lg:gap-16">
            <div>
              <h2 className="font-[family-name:var(--font-display)] text-[clamp(2.25rem,5vw,3.75rem)] font-bold leading-[0.95] tracking-[-0.035em]">
                Real taskbar pixels
              </h2>
              <p className="mt-6 max-w-xl text-lg leading-relaxed text-[var(--muted)]">
                On Windows 11 the taskbar is XAML inside{" "}
                <code className="font-[family-name:var(--font-mono)] text-[var(--ink)]">
                  explorer.exe
                </code>
                . Phostide injects a tiny TAP helper, finds{" "}
                <code className="font-[family-name:var(--font-mono)] text-[var(--ink)]">
                  Rectangle#BackgroundFill
                </code>
                , and paints animated Composition surfaces into that brush.
                Icons stay on top. When Phostide exits, brushes restore.
              </p>
            </div>
            <ul className="flex flex-col justify-center gap-0 divide-y divide-[var(--line)] border-y border-[var(--line)]">
              {[
                {
                  title: "Not autohide",
                  body: "The bar stays usable. Pixels keep moving so OLED panels age more evenly.",
                },
                {
                  title: "Not an overlay",
                  body: "No window on top of the taskbar. Themes live in Explorer’s own fill.",
                },
                {
                  title: "Safe exit",
                  body: "Original brushes return on quit or crash. No system files are modified.",
                },
              ].map((item) => (
                <li key={item.title} className="py-5">
                  <p className="font-[family-name:var(--font-display)] text-xl font-semibold tracking-tight">
                    {item.title}
                  </p>
                  <p className="mt-1.5 text-sm leading-relaxed text-[var(--muted)]">
                    {item.body}
                  </p>
                </li>
              ))}
            </ul>
          </div>
        </section>

        {/* Themes — stacked light plates, not equal cards */}
        <section id="themes" className="relative">
          <div className="mx-auto flex w-full max-w-[72rem] flex-col gap-4 px-6 pb-8 pt-20 sm:px-8 sm:pt-28">
            <div className="flex flex-col gap-3 sm:flex-row sm:items-end sm:justify-between">
              <h2 className="font-[family-name:var(--font-display)] text-[clamp(2.25rem,5vw,3.75rem)] font-bold leading-[0.95] tracking-[-0.035em]">
                Themes
              </h2>
              <Link
                href="/docs/themes"
                className="text-sm text-[var(--accent)] underline-offset-4 transition-colors hover:text-[var(--ink)] hover:underline"
              >
                Theme guide
              </Link>
            </div>
            <p className="max-w-xl text-[var(--muted)]">
              Procedural light tuned for motion first — full-bar paint for anti
              burn-in.
            </p>
          </div>

          <div className="mx-auto flex w-full max-w-[72rem] flex-col gap-5 px-6 pb-24 sm:px-8 sm:pb-32">
            {themes.map((theme, i) => (
              <article
                key={theme.id}
                className="theme-plate group relative overflow-hidden"
                style={{ animationDelay: `${i * 40}ms` }}
              >
                <ThemePreview
                  theme={theme}
                  className="theme-preview--plate"
                  sizes="(max-width: 1152px) 100vw, 72rem"
                />
                <div className="pointer-events-none absolute inset-0 bg-gradient-to-r from-[rgba(2,4,8,0.82)] via-[rgba(2,4,8,0.35)] to-transparent" />
                <div className="absolute inset-y-0 left-0 flex max-w-md flex-col justify-end p-6 sm:p-8">
                  <h3 className="font-[family-name:var(--font-display)] text-2xl font-bold tracking-tight sm:text-3xl">
                    {theme.name}
                  </h3>
                  <p className="mt-2 text-sm leading-relaxed text-[var(--muted)] sm:text-base">
                    {theme.blurb}
                  </p>
                </div>
              </article>
            ))}
          </div>
        </section>

        {/* OLED */}
        <section className="relative overflow-hidden border-y border-[var(--line)]">
          <div
            className="pointer-events-none absolute -right-24 top-1/2 h-[28rem] w-[28rem] -translate-y-1/2 rounded-full opacity-40 blur-3xl"
            style={{
              background:
                "radial-gradient(circle, rgba(94,234,212,0.35), transparent 65%)",
            }}
            aria-hidden
          />
          <div className="relative mx-auto grid w-full max-w-[72rem] items-center gap-12 px-6 py-24 sm:px-8 sm:py-32 lg:grid-cols-2">
            <div>
              <h2 className="font-[family-name:var(--font-display)] text-[clamp(2.25rem,5vw,3.75rem)] font-bold leading-[0.95] tracking-[-0.035em]">
                Built for OLED desktops
              </h2>
              <p className="mt-6 max-w-xl text-lg leading-relaxed text-[var(--muted)]">
                Static taskbar chrome is a classic burn-in pattern. Phostide
                keeps every background pixel in motion at restrained luminance
                so the bar stays readable and the panel stays healthier.
              </p>
              <div className="mt-8 flex flex-wrap gap-3">
                <Link href="/docs/oled" className="btn-ghost">
                  OLED notes
                </Link>
                <Link href="/docs/install" className="btn-ghost">
                  Install guide
                </Link>
              </div>
            </div>
            <div className="relative mx-auto aspect-square w-full max-w-md overflow-hidden">
              <Image
                src="/phostide-mark-oled.png"
                alt=""
                fill
                className="object-cover"
                sizes="(max-width: 640px) 100vw, 28rem"
                style={{ animation: "mark-breathe 7s ease-in-out infinite" }}
              />
            </div>
          </div>
        </section>

        {/* Download */}
        <section className="relative">
          <div className="download-glow pointer-events-none absolute inset-0" aria-hidden />
          <div className="relative mx-auto flex w-full max-w-[72rem] flex-col items-start gap-6 px-6 py-24 sm:px-8 sm:py-32">
            <h2 className="font-[family-name:var(--font-display)] text-[clamp(2.25rem,5vw,3.75rem)] font-bold leading-[0.95] tracking-[-0.035em]">
              Get Phostide
            </h2>
            <p className="max-w-xl text-lg text-[var(--muted)]">
              Download the latest Windows x64 release from GitHub. Unpack, run{" "}
              <code className="font-[family-name:var(--font-mono)] text-[var(--ink)]">
                Phostide.exe
              </code>
              , pick a theme from the tray.
            </p>
            <div className="flex flex-wrap gap-3 pt-2">
              <LatestDownload />
              <Link href="/docs/troubleshooting" className="btn-ghost">
                Troubleshooting
              </Link>
            </div>
          </div>
        </section>
      </main>

      <SiteFooter />
    </div>
  );
}
