const RELEASES_API =
  "https://api.github.com/repos/teamdoticca/Phostide/releases/latest";
export const FALLBACK_RELEASES =
  "https://github.com/teamdoticca/Phostide/releases/latest";
export const GITHUB_REPO = "https://github.com/teamdoticca/Phostide";

export type ReleaseAssets = {
  tag: string | null;
  pageUrl: string;
  setupUrl: string;
  zipUrl: string;
};

export async function resolveReleaseAssets(): Promise<ReleaseAssets> {
  try {
    const res = await fetch(RELEASES_API, {
      next: { revalidate: 300 },
      headers: {
        Accept: "application/vnd.github+json",
        "User-Agent": "phostide-site",
      },
    });
    if (!res.ok) {
      return {
        tag: null,
        pageUrl: FALLBACK_RELEASES,
        setupUrl: FALLBACK_RELEASES,
        zipUrl: FALLBACK_RELEASES,
      };
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
      tag: data.tag_name ?? null,
      pageUrl,
      setupUrl: setup?.browser_download_url ?? pageUrl,
      zipUrl: zip?.browser_download_url ?? pageUrl,
    };
  } catch {
    return {
      tag: null,
      pageUrl: FALLBACK_RELEASES,
      setupUrl: FALLBACK_RELEASES,
      zipUrl: FALLBACK_RELEASES,
    };
  }
}
