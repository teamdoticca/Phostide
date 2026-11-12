import type { Metadata } from "next";
import Link from "next/link";
import { IconArrowLeft, IconArrowRight, IconDownload } from "@/components/Icons";
import { LatestDownload } from "@/components/LatestDownload";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { FALLBACK_TAG, resolveReleaseAssets } from "@/lib/releases";
import { absoluteUrl, pageMetadata, SITE_NAME } from "@/lib/seo";

export const metadata: Metadata = pageMetadata({
  title: "Install & run",
  description:
    "Install Phostide with the signed Setup.exe or portable zip. Multi-monitor Settings, no admin required.",
  path: "/docs/install",
});

const toc = [
  { href: "#setup", label: "1 · Setup.exe" },
  { href: "#files", label: "2 · Where it goes" },
  { href: "#run", label: "3 · Tray & Settings" },
  { href: "#monitors", label: "Multi-monitor" },
  { href: "#oled-dual", label: "Dual-monitor OLED" },
  { href: "#portable", label: "Portable zip" },
  { href: "#settings-file", label: "Settings file" },
  { href: "#uninstall", label: "Uninstall" },
];

function InstallHowToJsonLd() {
  const data = {
    "@context": "https://schema.org",
    "@type": "HowTo",
    name: `Install ${SITE_NAME} on Windows 11`,
    description:
      "Install the signed Setup.exe, open Settings from the tray, and Apply a theme to all monitors or one.",
    url: absoluteUrl("/docs/install"),
    totalTime: "PT5M",
    tool: [
      {
        "@type": "HowToTool",
        name: "Phostide Setup.exe (signed, per-user, no admin)",
      },
    ],
    step: [
      {
        "@type": "HowToStep",
        name: "Download Setup.exe",
        text: "Get the latest signed Setup.exe from phostide.doticca.com or GitHub Releases for teamdoticca/Phostide.",
        url: absoluteUrl("/docs/install#setup"),
      },
      {
        "@type": "HowToStep",
        name: "Run the installer",
        text: "Run Setup.exe. It installs for your user account only (no admin) and can launch Phostide when it finishes. A tray icon appears.",
      },
      {
        "@type": "HowToStep",
        name: "Open Settings and Apply a theme",
        text: "Right-click the tray icon › Settings… › Taskbar. Choose Apply to All or a monitor number, pick a theme and intensity, then click Apply.",
        url: absoluteUrl("/docs/install#run"),
      },
      {
        "@type": "HowToStep",
        name: "Optional multi-monitor OLED",
        text: "On Monitors, uncheck displays you want stock. On Taskbar, Apply Space or Aurora to the OLED monitor only. On System, leave RENDERING on Auto.",
        url: absoluteUrl("/docs/install#oled-dual"),
      },
    ],
  };

  return (
    <script
      type="application/ld+json"
      dangerouslySetInnerHTML={{ __html: JSON.stringify(data) }}
    />
  );
}

export default async function InstallPage() {
  const { tag } = await resolveReleaseAssets();
  const releaseTag = tag ?? FALLBACK_TAG;

  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <InstallHowToJsonLd />
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
            Windows 11 22H2 or newer, x64. Per‑user install — no admin rights.
            Standalone: no Windhawk or extra overlay software. Five built‑in
            themes only — no theme editor.
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
              Download the latest Setup.exe from GitHub Releases (or this site)
              and run it. It installs for your user account only and can launch
              Phostide when it finishes.
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
              <p className="pl-5">Themes.hlsl</p>
            </div>
            <p className="mt-4 text-sm text-soft">
              <code className="text-zinc-100">Phostide.exe</code> is the tray
              host. <code className="text-zinc-100">ExplorerTAP.dll</code> is
              injected into Explorer so themes paint the real taskbar
              background. <code className="text-zinc-100">Themes.hlsl</code> is
              the GPU shader pack — keep all three in the same folder. CPU
              painting still works if the HLSL file is missing.
            </p>
          </section>

          <section id="run" className="mt-6 rounded-xl border border-line bg-panel p-8">
            <p className="text-xs text-soft">Step 3</p>
            <h2 className="mt-1 text-2xl font-semibold">Tray menu &amp; Settings</h2>
            <p className="mt-3 text-soft">
              Open Phostide from the Start menu. A tray icon appears. Right‑click
              it for:
            </p>
            <ul className="mt-4 list-disc space-y-2 pl-5 text-sm text-soft">
              <li>
                <span className="text-zinc-100">Enable Phostide</span> — toggle
                themes on or off without quitting
              </li>
              <li>
                <span className="text-zinc-100">Settings…</span> — Taskbar
                (theme/intensity per display), Monitors (on/off), System
                (shared rendering, Start with Windows, updates)
              </li>
              <li>
                <span className="text-zinc-100">Exit</span> — quit and restore
                the normal Windows taskbar
              </li>
            </ul>
            <div className="mt-6 rounded-lg border border-line bg-black p-5 text-sm text-soft">
              <p className="font-medium text-zinc-100">In Settings</p>
              <ul className="mt-3 list-disc space-y-2 pl-5">
                <li>
                  <span className="text-zinc-100">Taskbar</span> tab — scope{" "}
                  <span className="text-zinc-100">All</span> or a specific
                  monitor, then set theme and intensity (0–100%). Live preview;
                  click <span className="text-zinc-100">Apply</span> to commit
                </li>
                <li>
                  <span className="text-zinc-100">Monitors</span> tab — turn
                  individual displays on or off (default: all on). Off displays
                  keep a stock Windows taskbar
                </li>
                <li>
                  <span className="text-zinc-100">System</span> tab —{" "}
                  <span className="text-zinc-100">RENDERING</span> (Auto / GPU /
                  CPU, shared by all bars), Start with Windows, check for updates
                </li>
              </ul>
              <p className="mt-3">
                Apply under All writes the draft theme/intensity to every
                attached display (and the fallback for newly plugged monitors).
                Apply under a monitor number writes only that display. Rendering
                is always app‑wide. Auto uses D3D11 GPU paint when it can and
                falls back to CPU. Closing without Apply keeps your previous
                look.
              </p>
            </div>
          </section>

          <section id="monitors" className="mt-14">
            <h2 className="text-2xl font-semibold">Multi-monitor</h2>
            <p className="mt-3 text-soft">
              Every attached display can look different — or stay stock.
            </p>
            <ul className="mt-4 list-disc space-y-2 pl-5 text-sm text-soft">
              <li>
                <span className="text-zinc-100">Taskbar › APPLY TO</span> —{" "}
                <span className="text-zinc-100">All</span> writes the current
                theme and intensity to every display (and the fallback for newly
                plugged monitors). A numbered chip edits one display only.
              </li>
              <li>
                <span className="text-zinc-100">Monitors</span> — uncheck a
                display to leave its taskbar alone. Grayed chips on the Taskbar
                tab mean that display is off here; you can still edit its look
                for when you turn it back on.
              </li>
              <li>
                <span className="text-zinc-100">System › RENDERING</span> is
                shared by every active bar (Auto / GPU / CPU). It is not
                per‑monitor.
              </li>
            </ul>
          </section>

          <section id="oled-dual" className="mt-14">
            <h2 className="text-2xl font-semibold">Dual-monitor OLED</h2>
            <p className="mt-3 text-soft">
              Example: OLED primary + a second LCD. Theme only the OLED and leave
              the LCD stock.
            </p>
            <ol className="mt-4 list-decimal space-y-3 pl-5 text-sm text-soft">
              <li>
                <span className="text-zinc-100">Monitors</span> — keep the OLED
                checked; uncheck the LCD (or any display you want stock).
              </li>
              <li>
                <span className="text-zinc-100">Taskbar</span> — Apply to the OLED
                monitor number › pick <span className="text-zinc-100">Space</span>{" "}
                or <span className="text-zinc-100">Aurora</span> › modest intensity
                › <span className="text-zinc-100">Apply</span>.
              </li>
              <li>
                <span className="text-zinc-100">System</span> — leave{" "}
                <span className="text-zinc-100">RENDERING</span> on{" "}
                <span className="text-zinc-100">Auto</span>; optionally enable Start
                with Windows › Apply.
              </li>
            </ol>
            <p className="mt-4 text-sm text-soft">
              Honest limit: only the background fill moves. Static icons, Start,
              tray, and clock can still contribute to OLED burn‑in. See{" "}
              <Link href="/docs/oled" className="text-ember hover:underline">
                OLED notes
              </Link>{" "}
              and the{" "}
              <Link href="/docs/faq" className="text-ember hover:underline">
                FAQ
              </Link>
              .
            </p>
          </section>

          <section id="portable" className="mt-14">
            <h2 className="text-2xl font-semibold">
              Prefer not to install? Use the portable zip.
            </h2>
            <p className="mt-3 text-soft">
              Grab the zip from the same release, unzip it anywhere, and run
              Phostide.exe. Keep ExplorerTAP.dll and Themes.hlsl in the same
              folder. To remove it, Exit from the tray and delete the folder.
              Settings still live under %APPDATA%\Phostide.
            </p>
            <LatestDownload kind="zip" className="btn-ghost mt-5">
              Download zip
            </LatestDownload>
          </section>

          <section id="settings-file" className="mt-14">
            <h2 className="text-2xl font-semibold">Settings file</h2>
            <p className="mt-3 text-soft">
              Fallback <code className="text-zinc-100">theme</code> /{" "}
              <code className="text-zinc-100">intensity</code>, app‑global{" "}
              <code className="text-zinc-100">renderMode</code> (Auto / GPU /
              CPU), <code className="text-zinc-100">monitorSettings</code>{" "}
              (per‑display theme/intensity),{" "}
              <code className="text-zinc-100">disabledMonitors</code>, Start with
              Windows, and a few runtime flags are saved here. Delete the file to
              reset to defaults (Rainbow, 45% intensity, Auto rendering, all
              monitors active).
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
                Exit Phostide from the tray icon (restores the normal taskbar).
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
            <div className="mt-12 flex flex-wrap justify-between gap-4 text-sm">
              <Link
                href="/docs/faq"
                className="inline-flex items-center gap-2 text-soft hover:text-white"
              >
                FAQ
              </Link>
              <Link
                href="/docs/themes"
                className="inline-flex items-center gap-2 text-ember"
              >
                Next: Themes <IconArrowRight />
              </Link>
            </div>
          </section>
        </div>
      </main>

      <SiteFooter />
    </div>
  );
}
