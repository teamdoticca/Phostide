import Image from "next/image";
import Link from "next/link";
import { LatestDownload } from "@/components/LatestDownload";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { ThemePreview } from "@/components/ThemePreview";
import { themes } from "@/lib/themes";

export default function Home() {
  return (
    <div className="relative min-h-screen overflow-x-hidden">
      <SiteHeader />

      <section className="relative flex min-h-[100svh] items-end overflow-hidden">
        <div className="absolute inset-0" aria-hidden>
          <Image
            src="/og-ai.jpg"
            alt=""
            fill
            priority
            className="object-cover object-center"
            style={{ animation: "phostide-drift 28s ease-in-out infinite alternate" }}
            sizes="100vw"
          />
          <div className="absolute inset-0 bg-gradient-to-t from-[var(--bg0)] via-[rgba(5,7,10,0.55)] to-[rgba(5,7,10,0.35)]" />
          <div className="absolute inset-0 bg-[radial-gradient(ellipse_at_center,transparent_0%,rgba(5,7,10,0.45)_100%)]" />
        </div>

        <div className="relative z-10 mx-auto flex w-full max-w-6xl flex-col gap-6 px-6 pb-20 pt-32 sm:pb-24">
          <h1 className="animate-rise font-[family-name:var(--font-display)] text-[clamp(3.5rem,12vw,7.5rem)] font-semibold leading-[0.92] tracking-[-0.04em]">
            Phostide
          </h1>
          <p className="animate-rise-delay-1 max-w-xl text-xl leading-snug text-[var(--ink)] sm:text-2xl">
            OLED-safe themes on the real Windows taskbar.
          </p>
          <p className="animate-rise-delay-2 max-w-lg text-base leading-relaxed text-[var(--muted)] sm:text-lg">
            Rainbow, Sea Waves, Weather, Space, and Aurora paint Explorer’s
            own background fill — not an overlay.
          </p>
          <div className="animate-rise-delay-3 flex flex-wrap items-center gap-3 pt-2">
            <LatestDownload />
            <Link href="/docs" className="btn-ghost">
              Documentation
            </Link>
          </div>
          <p className="font-[family-name:var(--font-mono)] text-xs text-[var(--muted)]">
            Windows 11 22H2+ · x64 · MIT
          </p>
        </div>
      </section>

      <main>
        <section className="mx-auto w-full max-w-6xl px-6 py-24 sm:py-28">
          <h2 className="font-[family-name:var(--font-display)] text-3xl font-semibold tracking-tight sm:text-4xl">
            Real taskbar pixels
          </h2>
          <p className="mt-4 max-w-2xl text-lg leading-relaxed text-[var(--muted)]">
            On Windows 11 the taskbar is XAML inside{" "}
            <code className="font-[family-name:var(--font-mono)] text-[var(--ink)]">
              explorer.exe
            </code>
            . Phostide injects a tiny TAP helper, finds{" "}
            <code className="font-[family-name:var(--font-mono)] text-[var(--ink)]">
              Rectangle#BackgroundFill
            </code>
            , and paints animated Composition surfaces into that brush. Icons
            stay on top. When Phostide exits, brushes restore.
          </p>

          <dl className="mt-10 grid gap-x-8 gap-y-6 border-t border-[var(--line)] pt-6 sm:grid-cols-3">
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
              <div key={item.title}>
                <dt className="font-[family-name:var(--font-display)] text-base font-medium">
                  {item.title}
                </dt>
                <dd className="mt-1.5 text-sm leading-relaxed text-[var(--muted)]">
                  {item.body}
                </dd>
              </div>
            ))}
          </dl>
        </section>

        <section
          id="themes"
          className="border-y border-[var(--line)] bg-[var(--bg1)]"
        >
          <div className="mx-auto w-full max-w-6xl px-6 py-24 sm:py-28">
            <div className="flex flex-col gap-4 sm:flex-row sm:items-end sm:justify-between">
              <div>
                <h2 className="font-[family-name:var(--font-display)] text-3xl font-semibold tracking-tight sm:text-4xl">
                  Themes
                </h2>
                <p className="mt-3 max-w-xl text-[var(--muted)]">
                  Procedural effects tuned for motion first, spectacle second —
                  full-bar paint for anti burn-in.
                </p>
              </div>
              <Link
                href="/docs/themes"
                className="text-sm text-[var(--accent)] underline-offset-4 hover:underline"
              >
                Theme guide →
              </Link>
            </div>

            <ul className="mt-14 grid gap-8 sm:grid-cols-2 lg:grid-cols-3">
              {themes.map((theme) => (
                <li key={theme.id} className="group">
                  <ThemePreview theme={theme} />
                  <h3 className="mt-4 font-[family-name:var(--font-display)] text-xl font-medium transition-transform duration-200 group-hover:-translate-y-0.5">
                    {theme.name}
                  </h3>
                  <p className="mt-2 text-sm leading-relaxed text-[var(--muted)]">
                    {theme.blurb}
                  </p>
                </li>
              ))}
            </ul>
          </div>
        </section>

        <section className="relative mx-auto grid w-full max-w-6xl gap-12 px-6 py-24 sm:grid-cols-[1.1fr_0.9fr] sm:items-center sm:py-28">
          <div>
            <h2 className="font-[family-name:var(--font-display)] text-3xl font-semibold tracking-tight sm:text-4xl">
              Built for OLED desktops
            </h2>
            <p className="mt-4 max-w-xl text-lg leading-relaxed text-[var(--muted)]">
              Static taskbar chrome is a classic burn-in pattern. Phostide keeps
              every background pixel in motion at restrained luminance so the
              bar stays readable and the panel stays healthier.
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
          <div className="relative aspect-square overflow-hidden rounded-sm">
            <Image
              src="/phostide-mark-oled.png"
              alt=""
              fill
              className="object-cover"
              sizes="(max-width: 640px) 100vw, 40vw"
              style={{ animation: "mark-breathe 6s ease-in-out infinite" }}
            />
            <div className="pointer-events-none absolute inset-0 bg-gradient-to-tr from-[var(--bg0)]/40 via-transparent to-transparent" />
          </div>
        </section>

        <section className="border-t border-[var(--line)] bg-[linear-gradient(180deg,rgba(94,234,212,0.06),transparent_40%),var(--bg0)]">
          <div className="mx-auto flex w-full max-w-6xl flex-col items-start gap-6 px-6 py-24 sm:py-28">
            <h2 className="font-[family-name:var(--font-display)] text-3xl font-semibold tracking-tight sm:text-4xl">
              Get Phostide
            </h2>
            <p className="max-w-xl text-lg text-[var(--muted)]">
              Download the latest Windows x64 release from GitHub. Unpack, run{" "}
              <code className="font-[family-name:var(--font-mono)] text-[var(--ink)]">
                Phostide.exe
              </code>
              , pick a theme from the tray.
            </p>
            <div className="flex flex-wrap gap-3">
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
