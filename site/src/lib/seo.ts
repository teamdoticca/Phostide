import type { Metadata } from "next";

export const SITE_URL = "https://phostide.doticca.com";
export const SITE_NAME = "Phostide";
export const SITE_TAGLINE = "OLED burn-in care for the Windows 11 taskbar";
/** Home `<title>`: leads with the search phrase owners actually use. */
export const HOME_TITLE =
  "Phostide: OLED taskbar burn-in care for Windows 11 (free)";
export const SITE_DESCRIPTION =
  "Keep your Windows 11 taskbar visible and spare your OLED. Phostide slowly moves the taskbar background while you work, dims it when you step away and pauses it for fullscreen games. Free, open source, signed installer.";
export const SITE_OG_DESCRIPTION =
  "Keep your taskbar. Spare your OLED. A slowly moving taskbar background, idle dimming and per-monitor control for Windows 11. Free and open source.";

export const OG_IMAGE = {
  url: "/og.png",
  width: 1200,
  height: 630,
  alt: "Phostide — Keep your taskbar. Spare your OLED. OLED burn-in care for Windows 11.",
} as const;

export const GITHUB_REPO = "https://github.com/teamdoticca/Phostide";
export const ORG_NAME = "Doticca";
export const ORG_URL = "https://doticca.com";

/** Absolute URL helper. Paths should start with `/`. */
export function absoluteUrl(path = "/"): string {
  if (path === "/") return SITE_URL;
  return `${SITE_URL}${path.startsWith("/") ? path : `/${path}`}`;
}

type PageMetaInput = {
  title: string;
  description: string;
  path: string;
  /** When true, title is used as-is (no `%s · Phostide` template). */
  absoluteTitle?: boolean;
};

/** Shared page metadata: canonical + Open Graph + Twitter. */
export function pageMetadata({
  title,
  description,
  path,
  absoluteTitle = false,
}: PageMetaInput): Metadata {
  const url = absoluteUrl(path);
  const ogTitle = absoluteTitle ? title : `${title} · ${SITE_NAME}`;

  return {
    title: absoluteTitle ? { absolute: title } : title,
    description,
    alternates: { canonical: path },
    openGraph: {
      title: ogTitle,
      description,
      url,
      siteName: SITE_NAME,
      type: "website",
      locale: "en_US",
      images: [OG_IMAGE],
    },
    twitter: {
      card: "summary_large_image",
      title: ogTitle,
      description,
      images: [OG_IMAGE.url],
    },
  };
}

export const SITE_ROUTES = [
  { path: "/", priority: 1, changeFrequency: "weekly" as const },
  { path: "/docs", priority: 0.9, changeFrequency: "monthly" as const },
  { path: "/docs/install", priority: 0.8, changeFrequency: "monthly" as const },
  { path: "/docs/faq", priority: 0.85, changeFrequency: "monthly" as const },
  { path: "/docs/themes", priority: 0.8, changeFrequency: "monthly" as const },
  { path: "/docs/oled", priority: 0.85, changeFrequency: "monthly" as const },
  { path: "/docs/compare", priority: 0.8, changeFrequency: "monthly" as const },
  {
    path: "/docs/troubleshooting",
    priority: 0.7,
    changeFrequency: "monthly" as const,
  },
  { path: "/docs/privacy", priority: 0.5, changeFrequency: "yearly" as const },
  { path: "/llms.txt", priority: 0.6, changeFrequency: "monthly" as const },
  {
    path: "/llms-full.txt",
    priority: 0.55,
    changeFrequency: "monthly" as const,
  },
] as const;
