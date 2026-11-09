import Image from "next/image";
import type { ThemeInfo } from "@/lib/themes";

type ThemeStripProps = {
  theme: ThemeInfo;
  className?: string;
  rounded?: boolean;
};

/** Honest PaintThemePixels capture — full-width strip, no fake chrome. */
export function ThemeStrip({
  theme,
  className = "",
  rounded = false,
}: ThemeStripProps) {
  return (
    <div className={`strip overflow-hidden ${rounded ? "rounded" : ""} ${className}`.trim()}>
      <Image
        src={theme.previewSrc}
        alt={`${theme.name} taskbar theme`}
        width={1920}
        height={48}
        className="h-auto w-full"
        unoptimized
      />
    </div>
  );
}
