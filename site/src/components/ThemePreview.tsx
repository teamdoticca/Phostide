import Image from "next/image";
import type { ThemeInfo } from "@/lib/themes";

type ThemePreviewProps = {
  theme: ThemeInfo;
  className?: string;
  sizes?: string;
};

export function ThemePreview({
  theme,
  className = "",
  sizes = "(max-width: 640px) 100vw, (max-width: 1024px) 50vw, 33vw",
}: ThemePreviewProps) {
  return (
    <div
      className={`theme-preview relative overflow-hidden ${className}`.trim()}
    >
      <Image
        src={theme.previewSrc}
        alt=""
        fill
        className="object-cover"
        sizes={sizes}
      />
      <span className="sr-only">{theme.name} theme preview</span>
    </div>
  );
}
