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
    "Compare Windows 11 OLED taskbar approaches — auto-hide, dimmer overlays, fade mods, and Phostide painting the real Explorer BackgroundFill.",
  path: "/docs/compare",
});

const approaches = [
  {
    name: "Auto-hide taskbar",
    how: "Windows hides the bar until you hit the screen edge.",
    burnIn: "Good while hidden — zero static strip.",
    usability: "You lose always-on Start, pins, and clock.",
    fit: "Fine if you already live with auto-hide.",
  },
  {
    name: "Dim / black overlay",
    how: "A semi-transparent window sits on top of the taskbar.",
    burnIn: "Lowers brightness of the strip (and often icons).",
    usability: "Can fight z-order, fullscreen games, and multi-monitor edge cases.",
    fit: "Common AHK / small-tool approach.",
  },
  {
    name: "Idle fade / hide mods",
    how: "Third-party mods dim or hide the bar after idle (e.g. Windhawk-style).",
    burnIn: "Strong when idle; static again while you work on the bar.",
    usability: "Keeps a normal bar when awake; depends on the mod ecosystem.",
    fit: "Great if you want idle protection more than themes.",
  },
  {
    name: "Phostide",
    how: "Paints motion into Explorer’s real taskbar BackgroundFill — not a floating window.",
    burnIn: "Moves the fill so the background isn’t one fixed bright block. Icons/tray stay Windows-drawn.",
    usability:
      "Bar stays visible; themes switch from the tray; per-monitor on/off and appearance; Exit restores stock brushes.",
    fit: "When you want the bar usable and themed, not hidden or covered.",
  },
];

const rows: { label: string; values: string[] }[] = [
  {
    label: "Mechanism",
    values: [
      "Hide bar",
      "Overlay window",
      "Idle opacity / hide",
      "Explorer BackgroundFill",
    ],
  },
  {
    label: "Bar stays visible",
    values: ["No", "Yes", "While awake", "Yes"],
  },
  {
    label: "Icons stay on top",
    values: ["N/A", "Under overlay", "Yes", "Yes"],
  },
  {
    label: "Animated themes",
    values: ["No", "Rare", "Usually no", "Five built-in"],
  },
  {
    label: "Per-monitor control",
    values: ["OS only", "Often fragile", "Depends on mod", "On/off + theme/intensity"],
  },
  {
    label: "Restores on quit",
    values: ["N/A", "Close overlay", "Disable mod", "Tray › Exit"],
  },
];

export default function ComparePage() {
  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <SiteHeader active="/docs/oled" />

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
          People search for ways to protect an OLED from a static taskbar —
          auto‑hide, dimmer overlays, idle‑fade mods, or painting the real fill.
          Here is an honest map of those approaches and where Phostide sits.
        </p>

        <section className="mt-14">
          <h2 className="text-2xl font-semibold">The problem in one line</h2>
          <p className="mt-3 max-w-2xl text-soft">
            OLED pixels age with light. The Windows taskbar is a bright, mostly
            static strip for hours. Tools either hide it, cover it, fade it when
            idle, or — like Phostide — keep it usable and move the{" "}
            <strong className="font-medium text-zinc-100">background</strong>{" "}
            so that fill is not one locked pattern.
          </p>
        </section>

        <section className="mt-14 grid gap-5 md:grid-cols-2">
          {approaches.map((item) => (
            <article
              key={item.name}
              className={`rounded-xl border p-7 ${
                item.name === "Phostide"
                  ? "border-ember/50 bg-emberdim/20"
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
                  <dt className="text-zinc-100">Burn‑in angle</dt>
                  <dd className="mt-1">{item.burnIn}</dd>
                </div>
                <div>
                  <dt className="text-zinc-100">Usability</dt>
                  <dd className="mt-1">{item.usability}</dd>
                </div>
                <div>
                  <dt className="text-zinc-100">Best fit</dt>
                  <dd className="mt-1">{item.fit}</dd>
                </div>
              </dl>
            </article>
          ))}
        </section>

        <section className="mt-16">
          <h2 className="text-2xl font-semibold">Side‑by‑side</h2>
          <p className="mt-3 max-w-2xl text-soft">
            Columns follow the same order: auto‑hide, overlay dimmer, idle fade
            mods, Phostide.
          </p>
          <div className="mt-6 overflow-x-auto rounded-xl border border-line">
            <table className="min-w-[40rem] w-full border-collapse text-left text-sm">
              <thead className="bg-panel text-soft">
                <tr>
                  <th className="px-4 py-3 font-medium"> </th>
                  <th className="px-4 py-3 font-medium">Auto‑hide</th>
                  <th className="px-4 py-3 font-medium">Overlay</th>
                  <th className="px-4 py-3 font-medium">Idle fade</th>
                  <th className="px-4 py-3 font-medium text-zinc-100">
                    Phostide
                  </th>
                </tr>
              </thead>
              <tbody>
                {rows.map((row) => (
                  <tr key={row.label} className="border-t border-line">
                    <th className="px-4 py-3 font-medium text-zinc-100">
                      {row.label}
                    </th>
                    {row.values.map((value, i) => (
                      <td
                        key={`${row.label}-${i}`}
                        className={`px-4 py-3 text-soft ${
                          i === 3 ? "text-zinc-100" : ""
                        }`}
                      >
                        {value}
                      </td>
                    ))}
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </section>

        <section className="mt-16 grid gap-5 md:grid-cols-3">
          <div className="rounded-xl border border-line bg-panel p-6">
            <IconLayers className="h-5 w-5 text-ember" />
            <h3 className="mt-4 font-semibold">Not an overlay</h3>
            <p className="mt-2 text-sm text-soft">
              Phostide injects a helper into Explorer and replaces the taskbar
              fill brush. Icons, tray, and clock stay Windows chrome on top.
            </p>
          </div>
          <div className="rounded-xl border border-line bg-panel p-6">
            <IconDisplay className="h-5 w-5 text-ember" />
            <h3 className="mt-4 font-semibold">OLED‑aware, not a warranty</h3>
            <p className="mt-2 text-sm text-soft">
              Motion on a dark base reduces how long the same background pixels
              stay locked bright. Static icons can still age — see{" "}
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
              Tray › Exit puts the original brushes back. No system‑file patch.
              Local settings only — details on{" "}
              <Link href="/docs/privacy" className="text-zinc-100 underline-offset-2 hover:underline">
                Privacy
              </Link>
              .
            </p>
          </div>
        </section>

        <section className="mt-16 rounded-xl border border-ember/50 bg-emberdim/30 p-8">
          <p className="text-xs text-soft">Try it</p>
          <h2 className="mt-1 text-2xl font-semibold">
            Prefer a usable bar with moving fill?
          </h2>
          <p className="mt-3 max-w-xl text-soft">
            Install the signed Setup.exe (per‑user, no admin), open Settings from
            the tray, pick a theme (All monitors or one display), Apply. Use the
            Monitors tab to leave a panel stock. Start with Space or Aurora at
            modest intensity if you want the quietest OLED look.
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
