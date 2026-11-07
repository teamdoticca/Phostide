import { LatestDownload } from "@/components/LatestDownload";

const themes = [
  {
    name: "Rainbow",
    blurb: "Slow HSV sweep across the bar — constant pixel motion, low brightness.",
  },
  {
    name: "Sea Waves",
    blurb: "Soft sine layers that drift so no strip of the taskbar stays fixed.",
  },
  {
    name: "Weather",
    blurb: "Cloud noise and gentle luminance flicker — calm, OLED-friendly.",
  },
  {
    name: "Space",
    blurb: "Sparse starfield with a slow nebula wash behind your icons.",
  },
];

export default function Home() {
  return (
    <div className="relative min-h-screen overflow-hidden">
      <div
        aria-hidden
        className="pointer-events-none absolute inset-x-0 bottom-0 h-16 opacity-80"
        style={{
          background:
            "linear-gradient(90deg, #5eead4 0%, #38bdf8 25%, #a78bfa 50%, #f472b6 75%, #5eead4 100%)",
          backgroundSize: "200% 100%",
          animation: "phostide-sweep 12s linear infinite",
          maskImage:
            "linear-gradient(to top, black 0%, black 40%, transparent 100%)",
          WebkitMaskImage:
            "linear-gradient(to top, black 0%, black 40%, transparent 100%)",
        }}
      />

      <header className="mx-auto flex w-full max-w-5xl items-center justify-between px-6 pb-4 pt-8">
        <div className="font-[family-name:var(--font-display)] text-lg font-semibold tracking-tight">
          Phostide
        </div>
        <nav className="flex items-center gap-5 text-sm text-[var(--muted)]">
          <a
            className="transition-colors hover:text-[var(--ink)]"
            href="https://github.com/teamdoticca/Phostide"
          >
            GitHub
          </a>
          <a
            className="transition-colors hover:text-[var(--ink)]"
            href="#how"
          >
            How it works
          </a>
        </nav>
      </header>

      <main className="mx-auto flex w-full max-w-5xl flex-col px-6 pb-24 pt-10 sm:pt-16">
        <section className="flex min-h-[70vh] flex-col justify-center gap-8">
          <p className="font-[family-name:var(--font-mono)] text-xs uppercase tracking-[0.22em] text-[var(--accent)]">
            by doticca
          </p>
          <h1 className="max-w-3xl font-[family-name:var(--font-display)] text-5xl font-semibold leading-[1.05] tracking-tight sm:text-7xl">
            Phostide
          </h1>
          <p className="max-w-xl text-lg leading-relaxed text-[var(--muted)] sm:text-xl">
            OLED-safe animated themes on the{" "}
            <span className="text-[var(--ink)]">real</span> Windows taskbar —
            Explorer composition, not overlays.
          </p>
          <div className="flex flex-wrap items-center gap-4 pt-2">
            <LatestDownload />
            <a
              href="https://github.com/teamdoticca/Phostide"
              className="inline-flex h-12 items-center justify-center rounded-full border border-[var(--line)] px-6 text-sm font-medium text-[var(--ink)] transition hover:border-[var(--accent)] hover:text-[var(--accent)]"
            >
              View source
            </a>
          </div>
          <p className="font-[family-name:var(--font-mono)] text-xs text-[var(--muted)]">
            Windows 11 22H2+ · x64 · MIT
          </p>
        </section>

        <section id="how" className="mt-8 grid gap-10 border-t border-[var(--line)] pt-16 sm:grid-cols-2">
          <div>
            <h2 className="font-[family-name:var(--font-display)] text-2xl font-semibold tracking-tight">
              How it works
            </h2>
            <p className="mt-4 leading-relaxed text-[var(--muted)]">
              On Windows 11 the taskbar is XAML inside{" "}
              <code className="font-[family-name:var(--font-mono)] text-[var(--ink)]">
                explorer.exe
              </code>
              . Phostide injects a tiny TAP helper, finds{" "}
              <code className="font-[family-name:var(--font-mono)] text-[var(--ink)]">
                Rectangle#BackgroundFill
              </code>
              , and paints animated Composition surfaces into that brush —
              the actual taskbar pixels. Icons stay on top. When Phostide
              exits, brushes restore.
            </p>
          </div>
          <div>
            <h2 className="font-[family-name:var(--font-display)] text-2xl font-semibold tracking-tight">
              Why not autohide?
            </h2>
            <p className="mt-4 leading-relaxed text-[var(--muted)]">
              Autohide removes the bar. Pixel-shift moves windows. Overlays
              sit on top. Phostide keeps the bar usable and keeps its pixels
              moving — so your ROG OLED (and every other panel) ages more
              evenly.
            </p>
          </div>
        </section>

        <section className="mt-20 border-t border-[var(--line)] pt-16">
          <h2 className="font-[family-name:var(--font-display)] text-2xl font-semibold tracking-tight">
            Themes
          </h2>
          <p className="mt-3 max-w-2xl text-[var(--muted)]">
            Procedural effects tuned for low average luminance — motion first,
            spectacle second.
          </p>
          <ul className="mt-10 grid gap-6 sm:grid-cols-2">
            {themes.map((theme) => (
              <li
                key={theme.name}
                className="border-t border-[var(--line)] pt-5"
              >
                <h3 className="font-[family-name:var(--font-display)] text-lg font-medium">
                  {theme.name}
                </h3>
                <p className="mt-2 text-sm leading-relaxed text-[var(--muted)]">
                  {theme.blurb}
                </p>
              </li>
            ))}
          </ul>
        </section>
      </main>

      <footer className="mx-auto flex w-full max-w-5xl items-center justify-between border-t border-[var(--line)] px-6 py-8 text-sm text-[var(--muted)]">
        <span>© {new Date().getFullYear()} Doticca</span>
        <a
          className="hover:text-[var(--ink)]"
          href="https://www.doticca.com"
        >
          doticca.com
        </a>
      </footer>

      <style>{`
        @keyframes phostide-sweep {
          from { background-position: 0% 50%; }
          to { background-position: 200% 50%; }
        }
      `}</style>
    </div>
  );
}
