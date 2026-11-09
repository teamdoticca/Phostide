import type { ReactNode } from "react";
import { resolveReleaseAssets } from "@/lib/releases";

export async function LatestDownload({
  className = "btn-ember",
  children,
  kind = "setup",
}: {
  className?: string;
  children?: ReactNode;
  kind?: "setup" | "zip" | "page";
}) {
  const assets = await resolveReleaseAssets();
  const href =
    kind === "zip"
      ? assets.zipUrl
      : kind === "page"
        ? assets.pageUrl
        : assets.setupUrl;
  const label =
    children ??
    (kind === "zip"
      ? "Download zip"
      : assets.tag
        ? `Download installer ${assets.tag}`
        : "Download Setup.exe");

  return (
    <a href={href} className={className}>
      {label}
    </a>
  );
}

export async function ReleaseTag() {
  const { tag } = await resolveReleaseAssets();
  return <>{tag ?? "latest"}</>;
}
