"use client";

import { useEffect, useRef, useState } from "react";
import type { ThemeInfo } from "@/lib/themes";

type HeroDisplayProps = {
  themes: ThemeInfo[];
  initialThemeId?: ThemeInfo["id"];
};

/**
 * Hero visual: a black OLED desktop whose taskbar plays the real Native-48px
 * theme capture. Theme buttons swap the capture; the Idle switch dims the fill
 * the way OLED Care does while the icons and clock stay put.
 */
export function HeroDisplay({ themes, initialThemeId = "aurora" }: HeroDisplayProps) {
  const [themeId, setThemeId] = useState(initialThemeId);
  const [idle, setIdle] = useState(false);
  const [reducedMotion, setReducedMotion] = useState(false);
  const videoRef = useRef<HTMLVideoElement>(null);
  const theme = themes.find((t) => t.id === themeId) ?? themes[0];

  useEffect(() => {
    const mq = window.matchMedia("(prefers-reduced-motion: reduce)");
    const sync = () => setReducedMotion(mq.matches);
    sync();
    mq.addEventListener("change", sync);
    return () => mq.removeEventListener("change", sync);
  }, []);

  useEffect(() => {
    const el = videoRef.current;
    if (!el) return;
    if (reducedMotion) {
      el.pause();
    } else {
      void el.play().catch(() => {
        /* autoplay refused: the poster still shows the theme */
      });
    }
  }, [reducedMotion, themeId]);

  return (
    <div className="w-full">
      <div
        className="hero-display relative overflow-hidden rounded-2xl border border-line bg-black"
        role="img"
        aria-label={`Windows 11 taskbar painted with the ${theme.name} theme${
          idle ? ", dimmed by OLED Care while idle" : ""
        }`}
      >
        {/* Desktop: one quiet app window on true black, as an OLED shows it. */}
        <div className="absolute inset-x-[8%] top-[12%] bottom-[22%] rounded-xl border border-white/[0.07] bg-white/[0.015]">
          <div className="flex h-8 items-center gap-2 border-b border-white/[0.06] px-4">
            <span className="h-2 w-16 rounded-full bg-white/[0.08]" />
          </div>
          <div className="space-y-3 p-5">
            <span className="block h-2 w-2/3 rounded-full bg-white/[0.06]" />
            <span className="block h-2 w-1/2 rounded-full bg-white/[0.05]" />
            <span className="block h-2 w-3/5 rounded-full bg-white/[0.05]" />
          </div>
        </div>

        {/* Taskbar: the real capture fills the bar; items sit on top of it. */}
        <div className="hero-taskbar absolute inset-x-0 bottom-0">
          <video
            key={theme.id}
            ref={videoRef}
            className={`absolute inset-0 h-full w-full object-cover transition-opacity duration-700 ${
              idle ? "opacity-[0.12]" : "opacity-100"
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
            14:20
          </span>
        </div>
      </div>

      <div className="mt-4 flex flex-col gap-3 lg:flex-row lg:items-center lg:justify-between">
        <div className="flex flex-wrap gap-2" role="group" aria-label="Taskbar theme">
          {themes.map((t) => {
            const active = t.id === themeId;
            return (
              <button
                key={t.id}
                type="button"
                aria-pressed={active}
                onClick={() => setThemeId(t.id)}
                className={`rounded-lg border px-3.5 py-2 text-sm transition-[color,border-color,transform] duration-150 hover:-translate-y-px active:translate-y-0 ${
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
        <button
          type="button"
          role="switch"
          aria-checked={idle}
          onClick={() => setIdle((v) => !v)}
          className="group inline-flex items-center gap-3 self-start whitespace-nowrap rounded-lg border border-line px-3.5 py-2 text-sm text-zinc-200 transition-[border-color,transform] duration-150 hover:-translate-y-px hover:border-zinc-600 active:translate-y-0 lg:self-auto"
        >
          <span
            className={`relative inline-block h-5 w-9 shrink-0 rounded-full transition-colors ${
              idle ? "bg-ember" : "bg-zinc-700"
            }`}
          >
            <span
              className={`absolute left-0 top-0.5 h-4 w-4 rounded-full bg-white transition-transform ${
                idle ? "translate-x-[18px]" : "translate-x-0.5"
              }`}
            />
          </span>
          Idle: OLED Care dims the bar
        </button>
      </div>
    </div>
  );
}
