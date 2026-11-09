import Image from "next/image";
import type { ThemeInfo } from "@/lib/themes";

type ThemePreviewProps = {
  theme: ThemeInfo;
  className?: string;
  /** Larger chrome for landing rows; compact for docs. */
  size?: "row" | "docs";
};

/** Honest preview: real PaintThemePixels strip inside fake taskbar chrome. */
export function ThemePreview({
  theme,
  className = "",
  size = "row",
}: ThemePreviewProps) {
  const barH = size === "docs" ? "h-11 sm:h-12" : "h-12 sm:h-14";
  return (
    <div
      className={`taskbar-chrome ${className}`.trim()}
      aria-label={`${theme.name} taskbar preview`}
    >
      <div className={`taskbar-chrome__bar relative ${barH}`}>
        <Image
          src={theme.previewSrc}
          alt=""
          fill
          className="object-cover object-left"
          sizes="(max-width: 1152px) 100vw, 72rem"
          unoptimized
        />
        <div className="taskbar-chrome__icons" aria-hidden>
          <span className="taskbar-chrome__start" />
          <span className="taskbar-chrome__pill" />
          <span className="taskbar-chrome__pill taskbar-chrome__pill--dim" />
          <span className="taskbar-chrome__pill taskbar-chrome__pill--dim" />
          <span className="taskbar-chrome__spacer" />
          <span className="taskbar-chrome__tray" />
          <span className="taskbar-chrome__tray" />
          <span className="taskbar-chrome__clock" />
        </div>
      </div>
      <span className="sr-only">
        {theme.name} — captured from the real taskbar theme painter
      </span>
    </div>
  );
}
