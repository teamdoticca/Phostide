"use client";

import { useCallback, useRef, useState } from "react";
import type { ThemeInfo } from "@/lib/themes";

type ThemeStripProps = {
  theme: ThemeInfo;
  className?: string;
  rounded?: boolean;
};

/** Native-48px capture — plays on hover only; idle sheen invites the pointer. */
export function ThemeStrip({
  theme,
  className = "",
  rounded = false,
}: ThemeStripProps) {
  const videoRef = useRef<HTMLVideoElement>(null);
  const [playing, setPlaying] = useState(false);

  const play = useCallback(() => {
    const el = videoRef.current;
    if (!el) return;
    void el.play().then(() => setPlaying(true)).catch(() => setPlaying(false));
  }, []);

  const pause = useCallback(() => {
    const el = videoRef.current;
    if (!el) return;
    el.pause();
    // Settle on the first frame so idle reads as “waiting”, not a random freeze.
    try {
      el.currentTime = 0;
    } catch {
      /* ignore seek until metadata ready */
    }
    setPlaying(false);
  }, []);

  return (
    <div
      className={`strip group relative overflow-hidden ${rounded ? "rounded" : ""} ${className}`.trim()}
      onMouseEnter={play}
      onMouseLeave={pause}
      onFocus={play}
      onBlur={pause}
    >
      <video
        ref={videoRef}
        className="h-auto w-full"
        width={1920}
        height={48}
        muted
        loop
        playsInline
        preload="metadata"
        poster={theme.previewSrc}
        aria-label={`${theme.name} taskbar theme — hover to play`}
      >
        <source src={theme.previewVideo} type="video/webm" />
      </video>
      <div
        className={`strip-idle-sheen pointer-events-none absolute inset-0 ${playing ? "opacity-0" : "opacity-100"}`}
        aria-hidden
      />
    </div>
  );
}
