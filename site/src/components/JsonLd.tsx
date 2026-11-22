import {
  GITHUB_REPO,
  HOME_TITLE,
  OG_IMAGE,
  ORG_NAME,
  ORG_URL,
  SITE_DESCRIPTION,
  SITE_NAME,
  SITE_URL,
  absoluteUrl,
} from "@/lib/seo";

type JsonLdProps = {
  /** Latest release tag when known (e.g. v0.1.7). */
  softwareVersion?: string | null;
};

export function JsonLd({ softwareVersion }: JsonLdProps) {
  const org = {
    "@type": "Organization",
    "@id": `${ORG_URL}/#organization`,
    name: ORG_NAME,
    url: ORG_URL,
  };

  const website = {
    "@type": "WebSite",
    "@id": `${SITE_URL}/#website`,
    url: SITE_URL,
    name: SITE_NAME,
    description: SITE_DESCRIPTION,
    publisher: { "@id": `${ORG_URL}/#organization` },
    inLanguage: "en",
  };

  const software = {
    "@type": "SoftwareApplication",
    "@id": `${SITE_URL}/#software`,
    name: SITE_NAME,
    description: SITE_DESCRIPTION,
    url: SITE_URL,
    applicationCategory: "UtilitiesApplication",
    applicationSubCategory: "Desktop Enhancement",
    operatingSystem: "Windows 11",
    softwareVersion: softwareVersion?.replace(/^v/, "") ?? undefined,
    downloadUrl: `${GITHUB_REPO}/releases/latest`,
    installUrl: `${GITHUB_REPO}/releases/latest`,
    softwareHelp: absoluteUrl("/docs"),
    releaseNotes: `${GITHUB_REPO}/releases`,
    license: "https://opensource.org/licenses/MIT",
    isAccessibleForFree: true,
    offers: {
      "@type": "Offer",
      price: "0",
      priceCurrency: "USD",
      availability: "https://schema.org/InStock",
    },
    image: absoluteUrl(OG_IMAGE.url),
    screenshot: [
      absoluteUrl("/images/settings/settings-oled-care.png"),
      absoluteUrl("/images/settings/settings-monitors.png"),
    ],
    author: { "@id": `${ORG_URL}/#organization` },
    publisher: { "@id": `${ORG_URL}/#organization` },
    sameAs: [GITHUB_REPO],
    featureList: [
      "Keeps the Windows 11 taskbar visible: no auto-hide, no overlay window",
      "Slowly moving taskbar background while you work",
      "OLED Care: dims the taskbar when you step away, and dims screens you are not using",
      "Pauses on a fullscreen game's display so the GPU stays on the game",
      "Per-monitor control: theme only the OLED, leave other displays stock",
      "Five built-in themes: Rainbow, Sea Waves, Weather, Space, Aurora (no theme editor)",
      "Quit restores the normal taskbar; no system files modified",
      "Signed per-user installer, no admin rights",
      "No account, no telemetry",
      "MIT open source; standalone, no Windhawk required",
    ],
  };

  const graph = {
    "@context": "https://schema.org",
    "@graph": [
      org,
      website,
      software,
      {
        "@type": "WebPage",
        "@id": `${SITE_URL}/#webpage`,
        url: SITE_URL,
        name: HOME_TITLE,
        description: SITE_DESCRIPTION,
        isPartOf: { "@id": `${SITE_URL}/#website` },
        about: { "@id": `${SITE_URL}/#software` },
        primaryImageOfPage: {
          "@type": "ImageObject",
          url: absoluteUrl(OG_IMAGE.url),
          width: OG_IMAGE.width,
          height: OG_IMAGE.height,
        },
      },
    ],
  };

  return (
    <script
      type="application/ld+json"
      dangerouslySetInnerHTML={{ __html: JSON.stringify(graph) }}
    />
  );
}
