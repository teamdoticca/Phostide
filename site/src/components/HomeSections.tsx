import Link from "next/link";
import { IconArrowRight } from "@/components/Icons";
import { SettingsShot } from "@/components/SettingsShot";
import { HOME_FAQ } from "@/lib/homeFaq";
import { SITE_NAME, absoluteUrl } from "@/lib/seo";

const REPO = "https://github.com/teamdoticca/Phostide";

/* Shared rhythm: one idea per section, big headline, quiet body. */
const SECTION = "border-b border-line";
const INNER = "mx-auto max-w-site px-6 py-24 sm:px-10 sm:py-32";
const H2 =
  "max-w-3xl text-[2rem] font-semibold leading-[1.08] tracking-[-0.03em] sm:text-5xl";
const LEDE = "mt-6 max-w-2xl text-lg leading-relaxed text-soft";

/* ------------------------------------------------------------------ */
/* S2 · The problem                                                     */
/* ------------------------------------------------------------------ */

/** Illustration of where static light piles up on a desktop over a year. */
function StaticHeatMap() {
  return (
    <figure>
      <div
        className="relative aspect-[16/10] overflow-hidden rounded-2xl border border-line bg-black"
        role="img"
        aria-label="Illustration: windows move around and their light spreads out over a year, while the taskbar glows hot in the same strip."
      >
        {/* Windows come and go: their light spreads out and stays cool. */}
        <div className="absolute left-[8%] top-[10%] h-[52%] w-[46%] rounded-[3%] bg-[#1b2a5a]/40 blur-[18px]" />
        <div className="absolute right-[7%] top-[18%] h-[46%] w-[40%] rounded-[3%] bg-[#24346b]/35 blur-[22px]" />
        <div className="absolute left-[28%] top-[34%] h-[44%] w-[48%] rounded-[3%] bg-[#2b2160]/30 blur-[26px]" />
        {/* The taskbar never moves: same strip, same light, every hour. */}
        <div className="absolute inset-x-0 bottom-0 h-[6%] bg-gradient-to-r from-[#ff7a2e] via-[#ffd27a] to-[#ff7a2e] shadow-[0_0_40px_8px_rgba(255,138,61,0.45)]" />
        <div className="absolute inset-x-0 bottom-[6%] h-[14%] bg-gradient-to-t from-[#ff8a3d]/25 to-transparent" />
      </div>
      <figcaption className="mt-3 flex items-center justify-between gap-4 text-[13px] text-soft">
        <span>Illustration: where static light piles up over a year.</span>
        <span className="flex shrink-0 items-center gap-2" aria-hidden="true">
          <span>cool</span>
          <span className="h-1.5 w-16 rounded-full bg-gradient-to-r from-[#1b2a5a] via-[#ff7a2e] to-[#ffd27a]" />
          <span>hot</span>
        </span>
      </figcaption>
    </figure>
  );
}

export function ProblemSection() {
  return (
    <section className={SECTION}>
      <div className={`${INNER} grid gap-14 lg:grid-cols-2 lg:items-center lg:gap-20`}>
        <div>
          <h2 className={H2}>The one bright thing on your OLED that never moves.</h2>
          <p className={LEDE}>
            Windows, tabs and videos come and go, so their light spreads across
            the panel. The taskbar stays in the same strip, at the same
            brightness, every hour the PC is on.
          </p>
          <p className="mt-10 flex items-baseline gap-4">
            <span className="text-5xl font-semibold tracking-[-0.03em] text-zinc-50 sm:text-6xl">
              ~2,900
            </span>
            <span className="max-w-[16rem] text-soft">
              hours a year of the same pixels, at 8 hours a day.
            </span>
          </p>
          <p className="mt-10 max-w-xl text-sm leading-relaxed text-soft">
            OLED pixels wear with the light they give off, and static content
            wears them unevenly. Monitors Unboxed’s two-year, 6,000-hour test of
            OLED gaming monitors shows burn-in building up over time.{" "}
            <a
              href="https://www.pcgamer.com/hardware/gaming-monitors/after-two-years-and-over-6-000-hours-monitors-unboxeds-long-term-oled-gaming-monitor-test-shows-increasing-burn-in"
              className="text-zinc-200 underline decoration-line underline-offset-4 hover:decoration-zinc-400"
              rel="noreferrer"
            >
              Read the report on PC Gamer
            </a>
            .
          </p>
        </div>
        <StaticHeatMap />
      </div>
    </section>
  );
}

/* ------------------------------------------------------------------ */
/* S3 · Today's trade-off                                               */
/* ------------------------------------------------------------------ */

const TRADE_OFFS = [
  {
    verb: "Hide it",
    who: "Windows auto-hide",
    cost: "The bar is gone until you reach for it, and so are the clock, the tray and your open apps at a glance.",
  },
  {
    verb: "Dim it",
    who: "Your monitor’s Taskbar Detection",
    cost: "Good where it exists, on some newer monitors. It dims a fixed area of the screen and can’t tell when you’ve stepped away or which screen you’re using.",
  },
  {
    verb: "Cover it",
    who: "Overlay dimmer apps",
    cost: "A dark window sits on top of the bar and your icons, and can get in the way of notifications and fullscreen apps.",
  },
];

export function TradeOffSection() {
  return (
    <section className={SECTION}>
      <div className={INNER}>
        <h2 className={H2}>Today you hide it, dim it, or cover it.</h2>
        <p className={LEDE}>
          Each of the usual fixes protects the panel by taking something away
          from the taskbar.
        </p>

        <div className="mt-16 grid gap-x-10 gap-y-12 md:grid-cols-3">
          {TRADE_OFFS.map((t) => (
            <div key={t.verb} className="border-t border-line pt-6">
              <h3 className="text-2xl font-semibold tracking-tight">{t.verb}</h3>
              <p className="mt-1 text-sm text-zinc-300">{t.who}</p>
              <p className="mt-4 leading-relaxed text-soft">{t.cost}</p>
            </div>
          ))}
        </div>

        <div className="mt-16 grid gap-8 rounded-2xl border border-ember/40 bg-emberdim/25 p-8 sm:p-10 lg:grid-cols-[minmax(0,1fr)_minmax(0,1.4fr)] lg:items-center">
          <div>
            <h3 className="text-2xl font-semibold tracking-tight sm:text-3xl">
              Phostide: keep it, and keep it moving.
            </h3>
          </div>
          <div>
            <p className="leading-relaxed text-zinc-300">
              The bar stays visible with your icons on top. Its background moves
              while you work, dims when you step away and pauses for games. It
              runs alongside your monitor’s own OLED Care, and on laptops and
              panels that don’t have it.
            </p>
            <Link
              href="/docs/compare"
              className="mt-5 inline-flex items-center gap-2 text-sm text-ember hover:underline"
            >
              Full comparison <IconArrowRight />
            </Link>
          </div>
        </div>
      </div>
    </section>
  );
}

/* ------------------------------------------------------------------ */
/* S5 · Multi-monitor                                                   */
/* ------------------------------------------------------------------ */

type ScreenKind = "active" | "idle" | "stock";

function Screen({
  kind,
  label,
  status,
}: {
  kind: ScreenKind;
  label: string;
  status: string;
}) {
  return (
    <figure className="min-w-0">
      <div className="relative aspect-[16/10] overflow-hidden rounded-xl border border-line bg-black">
        {kind === "active" && (
          <div className="absolute left-[12%] top-[14%] h-[52%] w-[62%] rounded-md border border-white/[0.07] bg-white/[0.02]">
            <div className="h-[14%] border-b border-white/[0.06]" />
            <div className="space-y-[6%] p-[6%]">
              <span className="block h-1 w-2/3 rounded-full bg-white/[0.07]" />
              <span className="block h-1 w-1/2 rounded-full bg-white/[0.05]" />
            </div>
          </div>
        )}
        {kind === "idle" && (
          <div className="absolute left-[22%] top-[22%] h-[42%] w-[52%] rounded-md border border-white/[0.06] bg-white/[0.015]" />
        )}
        {kind === "stock" && (
          <div className="absolute left-[14%] top-[16%] h-[50%] w-[64%] rounded-md bg-[#1d1f24]" />
        )}

        {kind === "active" && (
          <svg
            className="absolute left-[58%] top-[56%] h-[12%] w-auto"
            viewBox="0 0 12 18"
            aria-hidden="true"
          >
            <path d="M1 1 L1 15 L4.6 11.6 L7 17 L9.2 16 L6.9 10.8 L11.5 10.8 Z" fill="#fff" stroke="#000" strokeWidth="1" />
          </svg>
        )}

        {/* Taskbar, drawn a little taller than true scale so it reads. */}
        <div className="absolute inset-x-0 bottom-0 h-[7%] min-h-[8px]">
          {kind === "stock" ? (
            <div className="h-full bg-[#202227]" />
          ) : (
            <>
              {/* eslint-disable-next-line @next/next/no-img-element */}
              <img
                src="/themes/aurora.png"
                alt=""
                className={`absolute inset-0 h-full w-full object-cover ${
                  kind === "idle" ? "opacity-[0.12]" : "motion-safe:hidden"
                }`}
              />
              {kind === "active" && (
                <video
                  className="absolute inset-0 hidden h-full w-full object-cover motion-safe:block"
                  src="/themes/aurora.webm"
                  muted
                  loop
                  autoPlay
                  playsInline
                  preload="metadata"
                  aria-hidden="true"
                />
              )}
            </>
          )}
          <div className="relative flex h-full items-center justify-center gap-[1.2%]">
            {Array.from({ length: 5 }, (_, i) => (
              <span
                key={i}
                className={`aspect-square h-[56%] rounded-[22%] ${
                  kind === "stock" ? "bg-white/[0.22]" : "bg-white/[0.18]"
                }`}
              />
            ))}
          </div>
        </div>
      </div>
      <figcaption className="mt-3">
        <span className="block text-sm font-medium text-zinc-100">{label}</span>
        <span className="block text-sm text-soft">{status}</span>
      </figcaption>
    </figure>
  );
}

export function MultiMonitorSection() {
  return (
    <section className={SECTION}>
      <div className={INNER}>
        <h2 className={H2}>Theme only your OLED. Let idle screens rest.</h2>
        <p className={LEDE}>
          Turn Phostide on or off for each display. While you work on one
          screen, a screen you’re not using dims on its own after the idle time,
          and a fullscreen game pauses only the display it covers.
        </p>

        <div
          className="mt-16 grid gap-6 sm:grid-cols-3"
          aria-label="Three displays on one desk"
        >
          <Screen kind="active" label="OLED · cursor here" status="Moving" />
          <Screen kind="idle" label="OLED · not in use" status="Dimmed by OLED Care" />
          <Screen kind="stock" label="LCD · turned off" status="Plain Windows taskbar" />
        </div>
        <p className="mt-4 text-[13px] text-soft">
          Illustration. Phostide follows the display your cursor is on.
        </p>

        <div className="mt-20 grid gap-12 lg:grid-cols-[minmax(0,1fr)_minmax(0,21rem)] lg:items-center">
          <div>
            <h3 className="text-2xl font-semibold tracking-tight">
              Every display, one Monitors tab.
            </h3>
            <p className="mt-4 max-w-xl leading-relaxed text-soft">
              Tick the screens Phostide should paint and see what each one is
              doing right now. A different theme and intensity per display is one
              Apply away on the Taskbar tab.
            </p>
            <dl className="mt-8 grid max-w-xl grid-cols-[auto_minmax(0,1fr)] gap-x-6 gap-y-3 text-sm">
              {[
                ["painting", "The background is moving."],
                ["fullscreen", "Paused under a fullscreen app."],
                ["locked", "Paused while Windows is locked."],
                ["off", "Left as a plain Windows taskbar."],
                ["paused", "Paused from the tray."],
              ].map(([term, desc]) => (
                <div key={term} className="contents">
                  <dt className="font-mono text-zinc-200">{term}</dt>
                  <dd className="text-soft">{desc}</dd>
                </div>
              ))}
            </dl>
          </div>
          <SettingsShot id="monitors" />
        </div>
      </div>
    </section>
  );
}

/* ------------------------------------------------------------------ */
/* S8 · Safe to install  ·  S9 · Honest limits                          */
/* ------------------------------------------------------------------ */

const TRUST: { title: string; body: React.ReactNode }[] = [
  {
    title: "Not an overlay. No screen capture.",
    body: "Phostide changes one thing: the taskbar’s background. No window sits on top of the bar, and nothing on your screen is read or recorded.",
  },
  {
    title: "Quit and it’s gone.",
    body: "Exit from the tray and Windows’ own taskbar comes straight back. No system files are changed, so nothing is left behind.",
  },
  {
    title: "Signed by Doticca SRL.",
    body: "Per-user install, no admin rights. Update now installs a new release only when it carries the same signature.",
  },
  {
    title: "No account. No telemetry.",
    body: "The app works offline. Its only network use is the optional update check against GitHub Releases.",
  },
  {
    title: "Open source, MIT.",
    body: (
      <>
        Read every line, including{" "}
        <a
          href={`${REPO}/tree/main/src/ExplorerTAP`}
          className="text-zinc-200 underline decoration-line underline-offset-4 hover:decoration-zinc-400"
          rel="noreferrer"
        >
          the small helper
        </a>{" "}
        that runs inside Windows Explorer to paint the bar’s background.
      </>
    ),
  },
  {
    title: "Stays out of your games.",
    body: "Phostide runs inside Explorer, not inside games, and stops painting on any display a fullscreen game covers.",
  },
];

const LIMITS = [
  "It isn’t a burn-in warranty. It cuts the time the same pixels sit bright; it can’t make burn-in impossible.",
  "Icons, the tray and the clock are still Windows. Optional experimental care can dim them while a screen is idle.",
  "Windows 11 22H2 or later, x64 only.",
];

export function TrustSection() {
  return (
    <section className={SECTION}>
      <div className={INNER}>
        <h2 className={H2}>Safe to install. Easy to remove.</h2>
        <p className={LEDE}>
          Phostide works inside Windows Explorer, so it should earn your trust
          before you run it. Here is exactly what it does and doesn’t do.
        </p>

        <dl className="mt-16 grid gap-x-12 gap-y-10 sm:grid-cols-2 lg:grid-cols-3">
          {TRUST.map((item) => (
            <div key={item.title} className="border-t border-line pt-6">
              <dt className="text-lg font-semibold">{item.title}</dt>
              <dd className="mt-2 leading-relaxed text-soft">{item.body}</dd>
            </div>
          ))}
        </dl>

        <p className="mt-12 max-w-2xl text-sm leading-relaxed text-soft">
          A new release can show a SmartScreen prompt until it builds
          reputation. Check that the publisher reads Doticca SRL, then choose
          More info › Run anyway.{" "}
          <Link href="/docs/troubleshooting#fix-smartscreen" className="text-ember hover:underline">
            SmartScreen help
          </Link>{" "}
          ·{" "}
          <Link href="/docs/privacy" className="text-ember hover:underline">
            Privacy
          </Link>
        </p>

        <div className="mt-20 rounded-2xl border border-line p-8 sm:p-10">
          <h3 className="text-2xl font-semibold tracking-tight">What Phostide won’t do</h3>
          <ul className="mt-6 space-y-3">
            {LIMITS.map((line) => (
              <li key={line} className="flex gap-3 leading-relaxed text-soft">
                <span aria-hidden="true" className="mt-[0.7em] h-px w-3 shrink-0 bg-zinc-500" />
                {line}
              </li>
            ))}
          </ul>
        </div>
      </div>
    </section>
  );
}

/* ------------------------------------------------------------------ */
/* S10 · FAQ                                                            */
/* ------------------------------------------------------------------ */

export function HomeFaqSection() {
  const data = {
    "@context": "https://schema.org",
    "@type": "FAQPage",
    mainEntity: HOME_FAQ.map((item) => ({
      "@type": "Question",
      name: item.q,
      acceptedAnswer: { "@type": "Answer", text: item.a },
    })),
    isPartOf: { "@type": "WebSite", name: SITE_NAME, url: absoluteUrl("/") },
  };

  return (
    <section className={SECTION}>
      <script
        type="application/ld+json"
        dangerouslySetInnerHTML={{ __html: JSON.stringify(data) }}
      />
      <div className={`${INNER} grid gap-12 lg:grid-cols-[minmax(0,1fr)_minmax(0,1.6fr)] lg:gap-20`}>
        <div>
          <h2 className={H2}>Questions people ask first.</h2>
          <Link
            href="/docs/faq"
            className="mt-6 inline-flex items-center gap-2 text-sm text-ember hover:underline"
          >
            All questions <IconArrowRight />
          </Link>
        </div>
        <div className="divide-y divide-line border-y border-line">
          {HOME_FAQ.map((item) => (
            <details key={item.q} className="group py-5">
              <summary className="flex cursor-pointer list-none items-start justify-between gap-6 text-lg font-medium text-zinc-100 [&::-webkit-details-marker]:hidden">
                {item.q}
                <span
                  aria-hidden="true"
                  className="mt-1 text-xl leading-none text-soft transition-transform duration-200 group-open:rotate-45"
                >
                  +
                </span>
              </summary>
              <p className="mt-3 max-w-2xl leading-relaxed text-soft">{item.a}</p>
            </details>
          ))}
        </div>
      </div>
    </section>
  );
}
