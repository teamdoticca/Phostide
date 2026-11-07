const RELEASES_API =
  "https://api.github.com/repos/teamdoticca/Phostide/releases/latest";
const FALLBACK_RELEASES =
  "https://github.com/teamdoticca/Phostide/releases";

async function resolveDownloadUrl(): Promise<{
  href: string;
  label: string;
}> {
  try {
    const res = await fetch(RELEASES_API, {
      next: { revalidate: 300 },
      headers: {
        Accept: "application/vnd.github+json",
        "User-Agent": "phostide-site",
      },
    });
    if (!res.ok) {
      return { href: FALLBACK_RELEASES, label: "Download for Windows" };
    }
    const data = (await res.json()) as {
      tag_name?: string;
      html_url?: string;
      assets?: Array<{ name: string; browser_download_url: string }>;
    };
    const asset =
      data.assets?.find((a) =>
        /Phostide.*win.*\.(zip|msi|exe)$/i.test(a.name)
      ) ??
      data.assets?.find((a) => /\.(zip|msi|exe)$/i.test(a.name));

    if (asset) {
      return {
        href: asset.browser_download_url,
        label: data.tag_name
          ? `Download ${data.tag_name}`
          : "Download for Windows",
      };
    }
    return {
      href: data.html_url ?? FALLBACK_RELEASES,
      label: "Download for Windows",
    };
  } catch {
    return { href: FALLBACK_RELEASES, label: "Download for Windows" };
  }
}

export async function LatestDownload() {
  const { href, label } = await resolveDownloadUrl();
  return (
    <a
      href={href}
      className="inline-flex h-12 items-center justify-center rounded-full bg-[var(--ink)] px-7 text-sm font-semibold text-[var(--bg0)] transition hover:bg-[var(--accent)]"
    >
      {label}
    </a>
  );
}
