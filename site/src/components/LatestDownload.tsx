const RELEASES_API =
  "https://api.github.com/repos/teamdoticca/Phostide/releases/latest";
const FALLBACK_RELEASES =
  "https://github.com/teamdoticca/Phostide/releases/latest";

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
    const assets = data.assets ?? [];
    const setup =
      assets.find((a) => /Phostide-Setup.*\.exe$/i.test(a.name)) ??
      assets.find((a) => /Setup.*\.exe$/i.test(a.name));
    const portable =
      assets.find((a) => /Phostide.*win.*\.zip$/i.test(a.name)) ??
      assets.find((a) => /\.zip$/i.test(a.name));
    const asset = setup ?? portable ?? assets.find((a) => /\.(msi|exe)$/i.test(a.name));

    if (asset) {
      const isSetup = /\.exe$/i.test(asset.name);
      return {
        href: asset.browser_download_url,
        label: data.tag_name
          ? isSetup
            ? `Download installer ${data.tag_name}`
            : `Download ${data.tag_name}`
          : isSetup
            ? "Download installer for Windows"
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

export async function LatestDownload({
  className = "btn-primary",
}: {
  className?: string;
}) {
  const { href, label } = await resolveDownloadUrl();
  return (
    <a href={href} className={className}>
      {label}
    </a>
  );
}
