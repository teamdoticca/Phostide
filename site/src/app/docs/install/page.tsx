import type { Metadata } from "next";
import Link from "next/link";
import { IconArrowLeft, IconArrowRight, IconDownload } from "@/components/Icons";
import { LatestDownload } from "@/components/LatestDownload";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { resolveReleaseAssets } from "@/lib/releases";

export const metadata: Metadata = {
  title: "Install & run",
  description:
    "Install Phostide with the signed Setup.exe or run the portable zip. No admin required.",
};

const toc = [
  { href: "#setup", label: "1 · Setup.exe" },
  { href: "#files", label: "2 · Where it goes" },
  { href: "#run", label: "3 · Run from tray" },
  { href: "#portable", label: "Portable zip" },
  { href: "#settings", label: "Settings file" },
  { href: "#uninstall", label: "Uninstall" },
];

export default async function InstallPage() {
  const { tag } = await resolveReleaseAssets();
  const releaseTag = tag ?? "v0.1.1";

  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <SiteHeader active="/docs/install" />

      <main className="mx-auto grid max-w-site gap-14 px-6 py-16 sm:px-10 sm:py-20 lg:grid-cols-12">
        <aside className="hidden lg:col-span-3 lg:block">
          <div className="sticky top-28 space-y-3 text-sm text-soft">
            <Link href="/docs" className="inline-flex items-center gap-2 hover:text-white">
              <IconArrowLeft />
              Docs
            </Link>
            <div className="space-y-3 border-t border-line pt-4">
              {toc.map((item, i) => (
                <a
                  key={item.href}
                  href={item.href}
                  className={`block hover:text-white ${i === 0 ? "text-white" : ""}`}
                >
                  {item.label}
                </a>
              ))}
            </div>
          </div>
        </aside>

        <div className="max-w-3xl lg:col-span-9">
          <p className="eyebrow">Install &amp; run · {releaseTag}</p>
          <h1 className="mt-3 text-4xl font-semibold tracking-tight sm:text-5xl">
            Install Phostide
          </h1>
          <p className="mt-5 text-lg text-soft">
            Windows 11 22H2 or newer, x64. No admin rights needed.
          </p>

          <section
            id="setup"
            className="mt-14 rounded-xl border border-ember/50 bg-emberdim/30 p-8"
          >
            <div className="flex flex-wrap items-start justify-between gap-3">
              <div>
                <p className="text-xs text-soft">Step 1 · Recommended</p>
                <h2 className="mt-1 text-2xl font-semibold">
                  Run the signed Setup.exe
                </h2>
              </div>
              <span className="rounded bg-ember px-2 py-1 text-xs font-semibold text-black">
                Signed · Doticca SRL
              </span>
            </div>
            <p className="mt-3 text-soft">
              Download the latest Setup.exe from GitHub Releases and run it. It
              installs just for your user account.
            </p>
            <LatestDownload className="btn-ember mt-6">
              <span className="inline-flex items-center gap-2">
                <IconDownload className="h-4 w-4" />
                Download Setup.exe
              </span>
            </LatestDownload>
          </section>

          <section id="files" className="mt-6 rounded-xl border border-line bg-panel p-8">
            <p className="text-xs text-soft">Step 2</p>
            <h2 className="mt-1 text-2xl font-semibold">Where it goes</h2>
            <div className="mt-5 space-y-1 rounded-lg border border-line bg-black p-5 font-mono text-sm">
              <p className="text-soft">%LOCALAPPDATA%\Programs\Phostide\</p>
              <p className="pl-5">Phostide.exe</p>
              <p className="pl-5">ExplorerTAP.dll</p>
            </div>
            <p className="mt-4 text-sm text-soft">
              Phostide.exe is the tray app. ExplorerTAP.dll is what lets it paint
              inside the taskbar.
            </p>
          </section>

          <section id="run" className="mt-6 rounded-xl border border-line bg-panel p-8">
            <p className="text-xs text-soft">Step 3</p>
            <h2 className="mt-1 text-2xl font-semibold">Run it from the tray</h2>
            <p className="mt-3 text-soft">
              Open Phostide from the Start menu. An icon appears in the system
              tray — right‑click it to pick a theme or quit. Quitting brings the
              normal taskbar back.
            </p>
          </section>

          <section id="portable" className="mt-14">
            <h2 className="text-2xl font-semibold">
              Prefer not to install? Use the portable zip.
            </h2>
            <p className="mt-3 text-soft">
              Grab the zip from the same release, unzip it anywhere, and run
              Phostide.exe. Keep ExplorerTAP.dll in the same folder. To remove
              it, quit from the tray and delete the folder.
            </p>
            <LatestDownload kind="zip" className="btn-ghost mt-5">
              Download zip
            </LatestDownload>
          </section>

          <section id="settings" className="mt-14">
            <h2 className="text-2xl font-semibold">Settings file</h2>
            <p className="mt-3 text-soft">
              Your theme choice is saved here. Delete it to go back to defaults.
            </p>
            <div className="mt-4 rounded-lg border border-line bg-panel px-5 py-4 font-mono text-sm">
              %APPDATA%\Phostide\settings.json
            </div>
          </section>

          <section id="uninstall" className="mt-14">
            <h2 className="text-2xl font-semibold">Uninstall</h2>
            <ol className="mt-4 space-y-3 text-soft">
              <li>
                <span className="mr-2 font-medium text-white">1.</span>
                Quit Phostide from the tray icon.
              </li>
              <li>
                <span className="mr-2 font-medium text-white">2.</span>
                Open Settings › Apps › Installed apps, find Phostide, choose
                Uninstall.
              </li>
              <li>
                <span className="mr-2 font-medium text-white">3.</span>
                Optional: delete %APPDATA%\Phostide to remove your settings.
              </li>
            </ol>
            <Link
              href="/docs/themes"
              className="mt-12 inline-flex items-center gap-2 text-sm text-ember"
            >
              Next: Themes <IconArrowRight />
            </Link>
          </section>
        </div>
      </main>

      <SiteFooter />
    </div>
  );
}
