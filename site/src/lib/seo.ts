import type { Metadata } from "next";

export const SITE_URL = "https://phostide.doticca.com";
export const SITE_NAME = "Phostide";
export const SITE_TAGLINE = "Animated themes for the Windows taskbar";
export const SITE_DESCRIPTION =
  "Phostide paints OLED-safe motion directly into the Windows taskbar background — GPU when available, CPU otherwise. Free, MIT, signed installer for Windows 11.";
export const SITE_OG_DESCRIPTION =
  "Animated themes, painted into your real Windows taskbar (GPU or CPU). Free · MIT · Windows 11.";

export const OG_IMAGE = {
  url: "/og.png",
  width: 1200,
  height: 630,
  alt: "Phostide — animated themes for the Windows taskbar",
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
  { path: "/docs/themes", priority: 0.8, changeFrequency: "monthly" as const },
  { path: "/docs/oled", priority: 0.7, changeFrequency: "monthly" as const },
  {
    path: "/docs/troubleshooting",
    priority: 0.7,
    changeFrequency: "monthly" as const,
  },
  { path: "/docs/privacy", priority: 0.5, changeFrequency: "yearly" as const },
] as const;
