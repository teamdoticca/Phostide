import type { Metadata } from "next";
import Link from "next/link";
import {
  IconArrowLeft,
  IconArrowRight,
  IconDisplay,
  IconDownload,
  IconLayers,
  IconUndo,
} from "@/components/Icons";
import { LatestDownload } from "@/components/LatestDownload";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { pageMetadata } from "@/lib/seo";

export const metadata: Metadata = pageMetadata({
  title: "OLED taskbar burn-in: overlays vs real fill",
  description:
    "Compare ways to protect an OLED from the Windows 11 taskbar: auto-hide, monitor Taskbar Detection, overlay dimmers, idle-fade mods and Phostide.",
  path: "/docs/compare",
});

type Approach = {
  name: string;
  short: string;
  how: string;
  costs: string;
  fit: string;
};

const approaches: Approach[] = [
  {
    name: "Auto-hide taskbar",
    short: "Auto-hide",
    how: "Windows hides the bar until you move the pointer to the screen edge.",
    costs:
      "No static strip while it’s hidden, but you lose the clock, tray and open apps at a glance. Some owners find the slide animation laggy on OLED.",
    fit: "Fine if you already live with auto-hide.",
  },
  {
    name: "Monitor Taskbar Detection",
    short: "Monitor firmware",
    how: "Some newer OLED monitors (for example ASUS OLED Care, MSI OLED Care and Philips) detect the taskbar area and lower its brightness.",
    costs:
      "Built in and free where it exists. It dims a fixed area all day and doesn’t know when you’ve stepped away or which screen you’re using. Laptops and older panels usually don’t have it.",
    fit: "Worth turning on if your monitor has it. Phostide runs alongside it.",
  },
  {
    name: "Overlay dimmer apps",
    short: "Overlay apps",
    how: "A free script or a paid app places a dark, semi-transparent window over the taskbar.",
    costs:
      "Lowers the strip’s brightness, but it covers your icons too and can fight with notifications, fullscreen games and multi-monitor edges.",
    fit: "A quick fix if you don’t mind a dimmer, covered bar.",
  },
  {
    name: "Idle fade / hide mods",
    short: "Idle mods",
    how: "Third-party mods (for example through Windhawk) fade or hide the bar after a period without input.",
    costs:
      "Strong when you’re away; the bar is static again while you work. Needs the mod platform and its injection.",
    fit: "Good if idle protection matters more to you than the look.",
  },
  {
    name: "Phostide",
    short: "Phostide",
    how: "A standalone app that paints the taskbar’s own background in Explorer. No window on top, no Windhawk.",
    costs:
      "Reduces how long the same pixels stay bright: motion while you work, OLED Care when you step away, a pause for fullscreen games. Not a burn-in warranty.",
    fit: "When you want to keep a visible, usable taskbar on an OLED.",
  },
];

type Mark = "yes" | "no" | "part" | "na";

const MARK_LABEL: Record<Mark, string> = {
  yes: "Yes",
  no: "No",
  part: "Partly",
  na: "Not applicable",
};

/** Cells follow the `approaches` order. */
const matrix: { label: string; cells: [Mark, string?][] }[] = [
  {
    label: "Taskbar stays visible",
    cells: [["no"], ["yes", "Dimmed"], ["yes", "Covered"], ["part", "While you work"], ["yes"]],
  },
  {
    label: "Icons not covered",
    cells: [["na"], ["part", "Dimmed with the bar"], ["no"], ["yes"], ["yes"]],
  },
  {
    label: "Moves while you work",
    cells: [["no"], ["no"], ["no"], ["no"], ["yes"]],
  },
  {
    label: "Knows when you step away",
    cells: [["na"], ["no"], ["part", "Varies"], ["yes"], ["yes"]],
  },
  {
    label: "Rests screens you’re not using",
    cells: [["no"], ["no"], ["no"], ["part", "Varies"], ["yes"]],
  },
  {
    label: "Pauses for fullscreen games",
    cells: [["na"], ["na"], ["part", "Varies"], ["part", "Varies"], ["yes", "Covered display only"]],
  },
  {
    label: "Works on any OLED, laptops too",
    cells: [["yes"], ["no", "Some newer monitors"], ["yes"], ["yes"], ["yes"]],
  },
  {
    label: "Nothing extra to install",
    cells: [["yes"], ["yes"], ["no"], ["no", "Mod platform"], ["no", "One app"]],
  },
];

const PHOSTIDE_COL = approaches.length - 1;

function MarkGlyph({ mark }: { mark: Mark }) {
  const glyph = { yes: "✓", no: "✕", part: "◐", na: "–" }[mark];
  const tone = {
    yes: "text-emerald-300",
    no: "text-zinc-500",
    part: "text-amber-300",
    na: "text-zinc-600",
  }[mark];
  return (
    <span className={`inline-block w-4 shrink-0 text-center ${tone}`} aria-hidden="true">
      {glyph}
    </span>
  );
}

function Cell({ mark, note }: { mark: Mark; note?: string }) {
  return (
    <span className="inline-flex items-start gap-2">
      <MarkGlyph mark={mark} />
      <span>
        {note ? (
          <>
            <span className="sr-only">{MARK_LABEL[mark]}: </span>
            {note}
          </>
        ) : (
          MARK_LABEL[mark]
        )}
      </span>
    </span>
  );
}

export default function ComparePage() {
  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <SiteHeader active="/docs/compare" />

      <main className="mx-auto max-w-site px-6 py-16 sm:px-10 sm:py-20">
        <Link
          href="/docs"
          className="inline-flex items-center gap-2 text-sm text-soft hover:text-white"
        >
          <IconArrowLeft />
          Docs
        </Link>

        <p className="mt-6 eyebrow">Compare · OLED taskbar</p>
        <h1 className="mt-3 max-w-3xl text-4xl font-semibold leading-tight tracking-tight sm:text-5xl">
          Windows 11 OLED taskbar burn‑in: what actually changes the bar.
        </h1>
        <p className="mt-5 max-w-2xl text-lg text-soft">
          The taskbar is a bright strip that never moves, so OLED owners hide
          it, dim it, cover it or fade it when idle. Here is an honest map of
          those options, including the Taskbar Detection built into some
          monitors. Phostide keeps the bar visible and works alongside your
          monitor’s own care.
        </p>

        <section className="mt-16">
          <h2 className="text-2xl font-semibold">At a glance</h2>
          <p className="mt-3 max-w-2xl text-soft">
            ✓ yes · ◐ partly, or depends on the tool · ✕ no · – not applicable
          </p>

          {/* Wide screens: one matrix, Phostide column highlighted. */}
          <div className="mt-6 hidden overflow-hidden rounded-xl border border-line md:block">
            <table className="w-full border-collapse text-left text-sm">
              <thead>
                <tr className="bg-panel text-soft">
                  <th className="px-4 py-3 font-medium">
                    <span className="sr-only">Feature</span>
                  </th>
                  {approaches.map((a, i) => (
                    <th
                      key={a.short}
                      scope="col"
                      className={`px-4 py-3 font-medium ${
                        i === PHOSTIDE_COL ? "bg-emberdim/50 text-zinc-50" : ""
                      }`}
                    >
                      {a.short}
                    </th>
                  ))}
                </tr>
              </thead>
              <tbody>
                {matrix.map((row) => (
                  <tr key={row.label} className="border-t border-line">
                    <th scope="row" className="px-4 py-3 font-medium text-zinc-100">
                      {row.label}
                    </th>
                    {row.cells.map(([mark, note], i) => (
                      <td
                        key={i}
                        className={`px-4 py-3 align-top ${
                          i === PHOSTIDE_COL ? "bg-emberdim/25 text-zinc-100" : "text-soft"
                        }`}
                      >
                        <Cell mark={mark} note={note} />
                      </td>
                    ))}
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          {/* Phones: one card per approach, rows in the same order. */}
          <div className="mt-6 space-y-4 md:hidden">
            {approaches.map((a, col) => (
              <div
                key={a.short}
                className={`rounded-xl border p-5 ${
                  col === PHOSTIDE_COL
                    ? "border-ember/50 bg-emberdim/25"
                    : "border-line bg-panel"
                }`}
              >
                <h3 className="font-semibold">{a.name}</h3>
                <dl className="mt-3 space-y-2 text-sm">
                  {matrix.map((row) => {
                    const [mark, note] = row.cells[col];
                    return (
                      <div key={row.label} className="flex justify-between gap-4">
                        <dt className="text-soft">{row.label}</dt>
                        <dd className="shrink-0 text-right text-zinc-200">
                          <Cell mark={mark} note={note} />
                        </dd>
                      </div>
                    );
                  })}
                </dl>
              </div>
            ))}
          </div>
        </section>

        <section className="mt-20">
          <h2 className="text-2xl font-semibold">Each approach, honestly</h2>
          <div className="mt-6 grid gap-5 md:grid-cols-2">
            {approaches.map((item, i) => (
              <article
                key={item.name}
                className={`rounded-xl border p-7 ${
                  i === PHOSTIDE_COL
                    ? "border-ember/50 bg-emberdim/20 md:col-span-2"
                    : "border-line bg-panel"
                }`}
              >
                <h3 className="text-lg font-semibold">{item.name}</h3>
                <dl className="mt-4 space-y-3 text-sm text-soft">
                  <div>
                    <dt className="text-zinc-100">How it works</dt>
                    <dd className="mt-1">{item.how}</dd>
                  </div>
                  <div>
                    <dt className="text-zinc-100">What it costs you</dt>
                    <dd className="mt-1">{item.costs}</dd>
                  </div>
                  <div>
                    <dt className="text-zinc-100">Best fit</dt>
                    <dd className="mt-1">{item.fit}</dd>
                  </div>
                </dl>
              </article>
            ))}
          </div>
        </section>

        <section className="mt-16 grid gap-5 md:grid-cols-3">
          <div className="rounded-xl border border-line bg-panel p-6">
            <IconLayers className="h-5 w-5 text-ember" />
            <h3 className="mt-4 font-semibold">Not an overlay</h3>
            <p className="mt-2 text-sm text-soft">
              Phostide runs a small helper inside Explorer that replaces the
              taskbar’s background brush. Icons, tray and clock stay Windows UI
              on top.
            </p>
          </div>
          <div className="rounded-xl border border-line bg-panel p-6">
            <IconDisplay className="h-5 w-5 text-ember" />
            <h3 className="mt-4 font-semibold">OLED‑aware, not a warranty</h3>
            <p className="mt-2 text-sm text-soft">
              Motion plus idle OLED Care reduce the time the same pixels stay
              bright; they are not a burn‑in warranty. Icons and the clock can
              still age unless experimental chrome care is on (and working). See{" "}
              <Link href="/docs/oled" className="text-zinc-100 underline-offset-2 hover:underline">
                OLED notes
              </Link>
              .
            </p>
          </div>
          <div className="rounded-xl border border-line bg-panel p-6">
            <IconUndo className="h-5 w-5 text-ember" />
            <h3 className="mt-4 font-semibold">Exit restores stock</h3>
            <p className="mt-2 text-sm text-soft">
              Tray › Exit puts the original brushes back. No system files are
              patched. Settings stay local; details on{" "}
              <Link href="/docs/privacy" className="text-zinc-100 underline-offset-2 hover:underline">
                Privacy
              </Link>
              .
            </p>
          </div>
        </section>

        <section className="mt-16 rounded-xl border border-ember/50 bg-emberdim/30 p-8">
          <h2 className="text-2xl font-semibold">
            Want a usable OLED taskbar, not a hidden one?
          </h2>
          <p className="mt-3 max-w-xl text-soft">
            Install the signed Setup.exe (per‑user, no admin) and open Settings
            from the tray. Pick a theme on the Taskbar tab, set OLED Care (or
            use the everyday preset), and turn off any LCD on the Monitors tab.
            Space and Aurora at a modest intensity are the quietest everyday
            look.
          </p>
          <div className="mt-6 flex flex-wrap gap-3">
            <LatestDownload className="btn-ember">
              <span className="inline-flex items-center gap-2">
                <IconDownload className="h-4 w-4" />
                Download Setup.exe
              </span>
            </LatestDownload>
            <Link href="/docs/install" className="btn-ghost">
              Install guide
            </Link>
          </div>
        </section>

        <div className="mt-12 flex justify-between text-sm">
          <Link
            href="/docs/oled"
            className="inline-flex items-center gap-2 text-soft hover:text-white"
          >
            <IconArrowLeft />
            OLED notes
          </Link>
          <Link
            href="/docs/themes"
            className="inline-flex items-center gap-2 text-ember"
          >
            Themes <IconArrowRight />
          </Link>
        </div>
      </main>

      <SiteFooter />
    </div>
  );
}
