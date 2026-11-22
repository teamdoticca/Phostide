"use client";

import { useEffect, useRef, useState } from "react";
import type { ThemeInfo } from "@/lib/themes";

type HeroDisplayProps = {
  themes: ThemeInfo[];
  initialThemeId?: ThemeInfo["id"];
};

type Moment = "work" | "away" | "game";

const MOMENTS: {
  id: Moment;
  label: string;
  caption: string;
  /** Wall clock shown on the taskbar for this moment. */
  clock: string;
  /** Autoplay dwell, ms. */
  dwell: number;
}[] = [
  {
    id: "work",
    label: "While you work",
    caption:
      "The background drifts slowly, so no pixel of the bar sits bright and still.",
    clock: "14:20",
    dwell: 7000,
  },
  {
    id: "away",
    label: "When you step away",
    caption:
      "OLED Care dims the bar after 5 idle minutes. A screen you’re not using can dim on its own.",
    clock: "14:26",
    dwell: 5000,
  },
  {
    id: "game",
    label: "When you game",
    caption:
      "Fullscreen pauses the paint on that display only, so your GPU stays on the game.",
    clock: "14:40",
    dwell: 5000,
  },
];

/* Syntax colours, muted for a black OLED editor. */
const K = "text-[#b9a3e3]"; // keyword
const F = "text-[#8fb3e8]"; // function
const S = "text-[#9fcf9f]"; // string
const T = "text-[#e6c88f]"; // type
const P = "text-white/45"; // punctuation
const C = "text-white/30"; // comment
const N = "text-white/75"; // name

type Token = [cls: string, text: string];

const CODE: Token[][] = [
  [[C, "// One customer's orders, without voided ones."]],
  [],
  [
    [K, "export async function "],
    [F, "loadOrders"],
    [P, "("],
    [N, "userId"],
    [P, ": "],
    [T, "string"],
    [P, ") {"],
  ],
  [
    [K, "  const "],
    [N, "res "],
    [P, "= "],
    [K, "await "],
    [F, "fetch"],
    [P, "("],
    [S, "`/api/orders?user=${userId}`"],
    [P, ");"],
  ],
  [
    [K, "  if "],
    [P, "(!"],
    [N, "res"],
    [P, "."],
    [N, "ok"],
    [P, ") "],
    [K, "throw new "],
    [F, "Error"],
    [P, "("],
    [S, '"Orders failed"'],
    [P, ");"],
  ],
  [
    [K, "  const "],
    [N, "orders "],
    [P, "= "],
    [K, "await "],
    [N, "res"],
    [P, "."],
    [F, "json"],
    [P, "();"],
  ],
];

/** Typed in live while you work: exactly 49 characters (see .hero-typing). */
const TYPED: Token[] = [
  [K, "return "],
  [N, "orders"],
  [P, "."],
  [F, "filter"],
  [P, "(("],
  [N, "o"],
  [P, ") => "],
  [N, "o"],
  [P, "."],
  [N, "status "],
  [P, "!== "],
  [S, '"void"'],
  [P, ");"],
];

function Tokens({ line }: { line: Token[] }) {
  return (
    <>
      {line.map(([cls, text], i) => (
        <span key={i} className={cls}>
          {text}
        </span>
      ))}
    </>
  );
}

/** A plain code editor on true black: what the screen shows while you work. */
function EditorWindow({
  moment,
  typingKey,
  animate,
}: {
  moment: Moment;
  typingKey: number;
  animate: boolean;
}) {
  const typing = moment === "work" && animate;
  const files = ["db.ts", "orders.ts", "users.ts", "orders.test.ts"];
  return (
    <div className="hero-window absolute inset-x-[5%] top-[7%] flex flex-col overflow-hidden rounded-[0.7em] border border-white/[0.08] bg-[#08080a] font-mono leading-[1.6] text-white/60">
      {/* Title bar, Windows-style controls on the right. */}
      <div className="flex h-[2.4em] shrink-0 items-center justify-between border-b border-white/[0.06] pl-[1.2em]">
        <span className="font-sans text-white/50">orders.ts · shop-api</span>
        <span className="flex h-full items-center text-white/35" aria-hidden="true">
          <span className="px-[1.1em]">─</span>
          <span className="px-[1.1em]">▢</span>
          <span className="px-[1.1em]">✕</span>
        </span>
      </div>

      <div className="flex min-h-0 flex-1">
        <div className="hero-sidebar w-[13em] shrink-0 border-r border-white/[0.06] py-[0.8em] font-sans">
          <p className="px-[1.2em] pb-[0.4em] text-[0.85em] uppercase tracking-[0.1em] text-white/30">
            src / api
          </p>
          {files.map((f) => (
            <p
              key={f}
              className={`px-[1.2em] ${
                f === "orders.ts" ? "bg-white/[0.05] text-white/80" : "text-white/40"
              }`}
            >
              {f}
            </p>
          ))}
        </div>

        <div className="min-w-0 flex-1">
          <div className="flex h-[2.2em] items-end border-b border-white/[0.06] font-sans">
            <span className="border-r border-white/[0.06] border-t border-t-ember/70 bg-white/[0.03] px-[1.2em] py-[0.25em] text-white/75">
              orders.ts
            </span>
          </div>
          <div className="whitespace-pre py-[0.6em]">
            {[...CODE, "typed" as const, [[P, "}"]] as Token[]].map((line, i) => (
              <div key={i} className="flex">
                <span className="w-[3.2em] shrink-0 pr-[1em] text-right text-white/20">
                  {i + 1}
                </span>
                {line === "typed" ? (
                  <span className="flex-1 bg-white/[0.03]">
                    {"  "}
                    <span
                      key={typingKey}
                      className={typing ? "hero-typing" : undefined}
                    >
                      <Tokens line={TYPED} />
                    </span>
                    <span
                      className={`ml-px inline-block h-[1.15em] w-[0.12em] translate-y-[0.2em] bg-white/80 ${
                        moment === "work" ? (animate ? "hero-caret" : "") : "opacity-0"
                      }`}
                      aria-hidden="true"
                    />
                  </span>
                ) : (
                  <span>
                    <Tokens line={line} />
                  </span>
                )}
              </div>
            ))}
          </div>
        </div>

        <div className="hero-terminal w-[24em] shrink-0 border-l border-white/[0.06] font-sans">
          <div className="flex h-[2.2em] items-end border-b border-white/[0.06] px-[1.2em] pb-[0.3em] text-[0.85em] uppercase tracking-[0.1em] text-white/30">
            Terminal
          </div>
          <div className="space-y-[0.15em] whitespace-pre p-[1em] font-mono">
            <p className="text-white/45">&gt; npm test orders</p>
            <p className="text-[#9fcf9f]">✓ 24 passed</p>
            <p className="text-white/30">  Ran in 1.8s</p>
            <p className="pt-[0.6em] text-white/45">&gt; npm run dev</p>
            <p className="text-white/40">  Ready on :3000</p>
          </div>
        </div>
      </div>

      <div className="flex h-[1.9em] shrink-0 items-center justify-between border-t border-white/[0.06] px-[1.2em] font-sans text-[0.9em] text-white/35">
        <span>main</span>
        <span>Ln 7, Col 51 · TypeScript</span>
      </div>
    </div>
  );
}

/** An original fullscreen game scene: dusk ridge, road and a light HUD. */
function GameScene() {
  return (
    <>
      <svg
        className="absolute inset-0 h-full w-full"
        viewBox="0 0 1600 600"
        preserveAspectRatio="xMidYMid slice"
        aria-hidden="true"
      >
        <defs>
          <linearGradient id="hero-sky" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0" stopColor="#070b16" />
            <stop offset="0.5" stopColor="#1d2442" />
            <stop offset="0.72" stopColor="#7a3f45" />
            <stop offset="0.82" stopColor="#d9794a" />
          </linearGradient>
          <radialGradient id="hero-sun" cx="0.5" cy="0.5" r="0.5">
            <stop offset="0" stopColor="#ffd19a" />
            <stop offset="0.35" stopColor="#f59a5b" stopOpacity="0.85" />
            <stop offset="1" stopColor="#f59a5b" stopOpacity="0" />
          </radialGradient>
        </defs>
        <rect width="1600" height="600" fill="url(#hero-sky)" />
        <circle cx="1060" cy="420" r="210" fill="url(#hero-sun)" />
        <circle cx="1060" cy="420" r="46" fill="#ffd9a8" />
        <path
          d="M0 400 L160 330 L300 372 L470 286 L640 360 L790 300 L960 380 L1130 310 L1290 366 L1440 296 L1600 350 V600 H0 Z"
          fill="#1a1f36"
        />
        <path
          d="M0 452 L210 392 L380 438 L560 380 L720 446 L900 398 L1080 452 L1260 404 L1430 448 L1600 410 V600 H0 Z"
          fill="#0f1224"
        />
        <path
          d="M0 500 Q400 450 800 476 T1600 470 V600 H0 Z"
          fill="#07080f"
        />
        <path d="M720 600 L792 476 L808 476 L880 600 Z" fill="#12141f" />
        <path
          d="M800 482 V492 M800 506 V522 M800 540 V566 M800 584 V600"
          stroke="#d9794a"
          strokeOpacity="0.55"
          strokeWidth="3"
        />
        {[
          [90, 488, 44],
          [190, 474, 60],
          [300, 484, 38],
          [1290, 470, 58],
          [1400, 482, 42],
          [1500, 468, 64],
        ].map(([x, y, h]) => (
          <path
            key={x}
            d={`M${x} ${y - h} L${x + h * 0.32} ${y} H${x - h * 0.32} Z`}
            fill="#040509"
          />
        ))}
      </svg>

      {/* HUD */}
      <div className="absolute left-[3%] top-[7%] font-mono text-[clamp(8px,0.9cqw,13px)] text-white/70">
        <div className="flex items-center gap-[0.6em]">
          <span>HP</span>
          <span className="block h-[0.7em] w-[14cqw] max-w-[220px] bg-white/15">
            <span className="block h-full w-[72%] bg-[#e5594f]" />
          </span>
        </div>
        <p className="mt-[0.6em] font-sans text-white/50">Reach the ridge before dark</p>
      </div>
      <div className="absolute right-[3%] top-[7%] aspect-square w-[8cqw] min-w-[44px] rounded-full border border-white/30 bg-black/30">
        <span className="absolute left-1/2 top-1/2 h-[10%] w-[10%] -translate-x-1/2 -translate-y-1/2 rounded-full bg-[#f5c46b]" />
        <span className="absolute left-[30%] top-[28%] h-[7%] w-[7%] rounded-full bg-[#e5594f]" />
      </div>
      <div className="absolute left-1/2 top-[52%] h-[2.6cqw] w-[2.6cqw] min-h-[14px] min-w-[14px] -translate-x-1/2 -translate-y-1/2" aria-hidden="true">
        <span className="absolute left-1/2 top-0 h-[32%] w-px bg-white/70" />
        <span className="absolute bottom-0 left-1/2 h-[32%] w-px bg-white/70" />
        <span className="absolute left-0 top-1/2 h-px w-[32%] bg-white/70" />
        <span className="absolute right-0 top-1/2 h-px w-[32%] bg-white/70" />
      </div>
      <p className="absolute bottom-[6%] right-[3%] font-mono text-[clamp(11px,1.8cqw,26px)] tabular-nums text-white/80">
        24<span className="text-white/40"> / 90</span>
      </p>
    </>
  );
}

/**
 * Hero visual: a black OLED desktop whose taskbar plays the real Native-48px
 * theme capture at its true 1920×48 ratio. It walks through a day — coding,
 * away (OLED Care dims the bar), a fullscreen game (paint pauses on that
 * display) — and stops cycling as soon as the visitor picks a moment or theme.
 */
export function HeroDisplay({ themes, initialThemeId = "aurora" }: HeroDisplayProps) {
  const [themeId, setThemeId] = useState(initialThemeId);
  const [moment, setMoment] = useState<Moment>("work");
  const [auto, setAuto] = useState(true);
  const [reducedMotion, setReducedMotion] = useState(false);
  const [typingKey, setTypingKey] = useState(0);
  const barVideo = useRef<HTMLVideoElement>(null);
  const theme = themes.find((t) => t.id === themeId) ?? themes[0];
  const current = MOMENTS.find((m) => m.id === moment) ?? MOMENTS[0];

  useEffect(() => {
    const mq = window.matchMedia("(prefers-reduced-motion: reduce)");
    const sync = () => setReducedMotion(mq.matches);
    sync();
    mq.addEventListener("change", sync);
    return () => mq.removeEventListener("change", sync);
  }, []);

  const go = (id: Moment) => {
    if (id === "work") setTypingKey((k) => k + 1);
    setMoment(id);
  };

  // Walk the day on a timer until the visitor takes over.
  const cycling = auto && !reducedMotion;
  useEffect(() => {
    if (!cycling) return;
    const t = window.setTimeout(() => {
      const i = MOMENTS.findIndex((m) => m.id === moment);
      const next = MOMENTS[(i + 1) % MOMENTS.length].id;
      if (next === "work") setTypingKey((k) => k + 1);
      setMoment(next);
    }, current.dwell);
    return () => window.clearTimeout(t);
  }, [cycling, moment, current.dwell]);

  // Paint is frozen in a fullscreen game and under reduced motion.
  const frozen = reducedMotion || moment === "game";
  useEffect(() => {
    const el = barVideo.current;
    if (!el) return;
    if (frozen) {
      el.pause();
    } else {
      void el.play().catch(() => {
        /* autoplay refused: the poster still shows the theme */
      });
    }
  }, [frozen, themeId]);

  const pick = (id: Moment) => {
    setAuto(false);
    go(id);
  };

  const game = moment === "game";

  return (
    <div className="w-full">
      <div
        className="hero-display relative overflow-hidden rounded-2xl border border-line bg-black"
        role="img"
        aria-label={`Windows 11 desktop on an OLED screen. The taskbar is painted with the ${theme.name} theme. ${current.label}: ${current.caption}`}
      >
        <div className={`transition-opacity duration-500 ${game ? "opacity-0" : "opacity-100"}`}>
          <EditorWindow moment={moment} typingKey={typingKey} animate={!reducedMotion} />
        </div>

        {/* Taskbar at true scale. */}
        <div className="hero-taskbar absolute inset-x-0 bottom-0">
          <video
            key={`bar-${theme.id}`}
            ref={barVideo}
            className={`absolute inset-0 h-full w-full object-cover transition-opacity duration-[1200ms] ease-out ${
              moment === "away" ? "opacity-[0.12]" : "opacity-100"
            }`}
            width={1920}
            height={48}
            muted
            loop
            playsInline
            autoPlay={!reducedMotion}
            preload="auto"
            poster={theme.previewSrc}
            aria-hidden="true"
          >
            <source src={theme.previewVideo} type="video/webm" />
          </video>
          <div className="relative flex h-full items-center justify-center gap-[0.9%]">
            {Array.from({ length: 7 }, (_, i) => (
              <span
                key={i}
                className={`aspect-square h-[58%] rounded-[22%] ${
                  i === 0 ? "bg-white/30" : "bg-white/[0.16]"
                }`}
              />
            ))}
          </div>
          <span className="absolute right-[1.6%] top-1/2 -translate-y-1/2 font-mono text-[clamp(8px,0.9cqw,13px)] tabular-nums text-white/75">
            {current.clock}
          </span>
        </div>

        {/* Fullscreen game covers the whole display, taskbar included. */}
        <div
          className={`absolute inset-0 transition-opacity duration-500 ${
            game ? "opacity-100" : "pointer-events-none opacity-0"
          }`}
          aria-hidden="true"
        >
          <GameScene />
        </div>
      </div>

      {/* The day, as three moments. */}
      <div className="mt-4 grid gap-4 xl:grid-cols-[minmax(0,1fr)_auto] xl:items-start">
        <div>
          <div
            className="grid grid-cols-3 divide-x divide-line overflow-hidden rounded-lg border border-line"
            role="group"
            aria-label="Show a moment"
          >
            {MOMENTS.map((m) => {
              const active = m.id === moment;
              return (
                <button
                  key={m.id}
                  type="button"
                  aria-pressed={active}
                  onClick={() => pick(m.id)}
                  className={`relative px-2 py-3 text-[13px] transition-colors duration-150 sm:px-4 sm:text-sm ${
                    active
                      ? "bg-white/[0.04] text-zinc-50"
                      : "text-soft hover:bg-white/[0.02] hover:text-zinc-200"
                  }`}
                >
                  {m.label}
                  <span
                    aria-hidden="true"
                    className="absolute inset-x-0 bottom-0 h-0.5 bg-white/[0.06]"
                  >
                    {active && (
                      <span
                        key={`${m.id}-${cycling}`}
                        className={`block h-full bg-ember ${
                          cycling ? "hero-progress" : "w-full"
                        }`}
                        style={
                          cycling
                            ? ({ animationDuration: `${m.dwell}ms` } as React.CSSProperties)
                            : undefined
                        }
                      />
                    )}
                  </span>
                </button>
              );
            })}
          </div>
          <p className="mt-3 min-h-[3em] max-w-2xl text-[15px] leading-relaxed text-zinc-300" aria-live="polite">
            {current.caption}
          </p>
        </div>

        <div className="flex flex-wrap gap-2" role="group" aria-label="Taskbar theme">
          {themes.map((t) => {
            const active = t.id === themeId;
            return (
              <button
                key={t.id}
                type="button"
                aria-pressed={active}
                onClick={() => {
                  setThemeId(t.id);
                  if (game) pick("work");
                }}
                className={`rounded-lg border px-3.5 py-2 text-sm transition-[color,border-color] duration-150 ${
                  active
                    ? "border-ember/70 bg-emberdim/40 text-zinc-50"
                    : "border-line text-soft hover:border-zinc-600 hover:text-zinc-200"
                }`}
              >
                {t.name}
              </button>
            );
          })}
        </div>
      </div>
    </div>
  );
}
