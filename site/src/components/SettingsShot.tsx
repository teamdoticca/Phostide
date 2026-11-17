import Image from "next/image";

export type SettingsShotId =
  | "taskbar"
  | "oled-care"
  | "monitors"
  | "system";

const SHOTS: Record<
  SettingsShotId,
  { src: string; alt: string; defaultCaption: string }
> = {
  taskbar: {
    src: "/images/settings/settings-taskbar.png",
    alt: "Phostide Settings — Taskbar tab with theme and intensity",
    defaultCaption: "Taskbar — theme, intensity, Apply to All or one monitor",
  },
  "oled-care": {
    src: "/images/settings/settings-oled-care.png",
    alt: "Phostide Settings — OLED Care tab with pause toggles",
    defaultCaption:
      "OLED Care — idle dim, pause while locked, pause in fullscreen (defaults on)",
  },
  monitors: {
    src: "/images/settings/settings-monitors.png",
    alt: "Phostide Settings — Monitors tab with live paint status",
    defaultCaption:
      "Monitors — on/off plus live status (painting / fullscreen / locked / off)",
  },
  system: {
    src: "/images/settings/settings-system.png",
    alt: "Phostide Settings — System tab with rendering and updates",
    defaultCaption: "System — Auto / GPU / CPU rendering, Start with Windows, updates",
  },
};

type SettingsShotProps = {
  id: SettingsShotId;
  caption?: string;
  className?: string;
  priority?: boolean;
};

/** Cropped Settings client capture — bordered panel, caption under the shot. */
export function SettingsShot({
  id,
  caption,
  className = "",
  priority = false,
}: SettingsShotProps) {
  const shot = SHOTS[id];
  return (
    <figure className={`overflow-hidden rounded-xl border border-line bg-panel ${className}`}>
      <Image
        src={shot.src}
        alt={shot.alt}
        width={720}
        height={752}
        className="h-auto w-full"
        sizes="(max-width: 768px) 100vw, 720px"
        priority={priority}
      />
      <figcaption className="border-t border-line px-4 py-3 text-sm text-soft">
        {caption ?? shot.defaultCaption}
      </figcaption>
    </figure>
  );
}
