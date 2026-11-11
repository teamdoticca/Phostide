const RELEASES_API =
  "https://api.github.com/repos/teamdoticca/Phostide/releases/latest";
export const FALLBACK_RELEASES =
  "https://github.com/teamdoticca/Phostide/releases/latest";
export const GITHUB_REPO = "https://github.com/teamdoticca/Phostide";

/** Bump with each published GitHub Release (used when the API is unreachable). */
export const FALLBACK_TAG = "v0.1.12";
const FALLBACK_SETUP =
  "https://github.com/teamdoticca/Phostide/releases/download/v0.1.12/Phostide-Setup-0.1.12-win-x64.exe";
const FALLBACK_ZIP =
  "https://github.com/teamdoticca/Phostide/releases/download/v0.1.12/Phostide-0.1.12-win-x64.zip";

export type ReleaseAssets = {
  tag: string | null;
  pageUrl: string;
  setupUrl: string;
  zipUrl: string;
};

function fallbackAssets(tag: string | null = FALLBACK_TAG): ReleaseAssets {
  return {
    tag,
    pageUrl: FALLBACK_RELEASES,
    setupUrl: FALLBACK_SETUP,
    zipUrl: FALLBACK_ZIP,
  };
}

export async function resolveReleaseAssets(): Promise<ReleaseAssets> {
  try {
    const res = await fetch(RELEASES_API, {
      next: { revalidate: 60 },
      headers: {
        Accept: "application/vnd.github+json",
        "User-Agent": "phostide-site",
      },
    });
    if (!res.ok) {
      return fallbackAssets();
    }
    const data = (await res.json()) as {
      tag_name?: string;
      html_url?: string;
      assets?: Array<{ name: string; browser_download_url: string }>;
    };
    const assets = data.assets ?? [];
    const setup =
      assets.find((a) => /Phostide-Setup.*\.exe$/i.test(a.name)) ??
      assets.find((a) => /Setup.*\.exe$/i.test(a.name));
    const zip =
      assets.find((a) => /Phostide.*win.*\.zip$/i.test(a.name)) ??
      assets.find((a) => /\.zip$/i.test(a.name));
    const pageUrl = data.html_url ?? FALLBACK_RELEASES;

    return {
      tag: data.tag_name ?? FALLBACK_TAG,
      pageUrl,
      setupUrl: setup?.browser_download_url ?? FALLBACK_SETUP,
      zipUrl: zip?.browser_download_url ?? FALLBACK_ZIP,
    };
  } catch {
    return fallbackAssets();
  }
}
