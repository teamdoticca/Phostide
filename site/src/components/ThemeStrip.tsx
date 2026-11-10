import type { ThemeInfo } from "@/lib/themes";

type ThemeStripProps = {
  theme: ThemeInfo;
  className?: string;
  rounded?: boolean;
};

/** Honest PaintThemePixels capture — animated WebM with PNG poster fallback. */
export function ThemeStrip({
  theme,
  className = "",
  rounded = false,
}: ThemeStripProps) {
  return (
    <div className={`strip overflow-hidden ${rounded ? "rounded" : ""} ${className}`.trim()}>
      <video
        className="h-auto w-full"
        width={1920}
        height={48}
        autoPlay
        muted
        loop
        playsInline
        preload="metadata"
        poster={theme.previewSrc}
        aria-label={`${theme.name} taskbar theme`}
      >
        <source src={theme.previewVideo} type="video/webm" />
      </video>
    </div>
  );
}
