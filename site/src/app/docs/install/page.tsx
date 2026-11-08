import type { Metadata } from "next";
import { LatestDownload } from "@/components/LatestDownload";

export const metadata: Metadata = {
  title: "Install & run",
  description: "Download and run Phostide on Windows 11 x64.",
};

export default function InstallPage() {
  return (
    <div className="prose-docs">
      <h1 className="font-[family-name:var(--font-display)] text-4xl font-semibold tracking-tight">
        Install & run
      </h1>
      <p className="mt-4 max-w-2xl text-lg">
        Phostide ships as a portable Windows x64 zip. No installer required.
      </p>

      <div className="mt-8">
        <LatestDownload />
      </div>

      <h2>Requirements</h2>
      <ul>
        <li>Windows 11 22H2 or later (XAML taskbar)</li>
        <li>x64 (AMD64)</li>
      </ul>

      <h2>Steps</h2>
      <ol className="mt-3 list-decimal space-y-2 pl-5 text-[var(--muted)]">
        <li>
          Download the latest{" "}
          <code>Phostide-*-win-x64.zip</code> from{" "}
          <a href="https://github.com/teamdoticca/Phostide/releases">
            GitHub Releases
          </a>
          .
        </li>
        <li>Unpack somewhere durable (for example <code>%LOCALAPPDATA%\Phostide</code>).</li>
        <li>
          Run <code>Phostide.exe</code>. A tray icon appears; the host injects{" "}
          <code>ExplorerTAP.dll</code> into Explorer.
        </li>
        <li>Open the tray menu, pick a theme, and optionally set intensity.</li>
      </ol>

      <h2>What normal start does</h2>
      <p>
        Product launch finds <code>Shell_TrayWnd</code>, loads the TAP via a
        window hook, and waits for <code>Local\Phostide.TAP.Ready</code>. It
        does <strong className="text-[var(--ink)]">not</strong> kill or restart
        Explorer.
      </p>

      <h2>Settings</h2>
      <p>
        Preferences live at{" "}
        <code>%APPDATA%\Phostide\settings.json</code> (
        <code>theme</code>, <code>intensity</code>, <code>enabled</code>, …).
      </p>

      <h2>Uninstall</h2>
      <p>
        Quit Phostide from the tray (brushes restore), then delete the unpack
        folder and optionally remove{" "}
        <code>%APPDATA%\Phostide</code>.
      </p>

      <h2>Build from source</h2>
      <p>
        Native build needs Visual Studio 2022 with C++ desktop workload and a
        recent Windows SDK. See the repo README and{" "}
        <code>docs/development.md</code> for the MSBuild invocation.
      </p>
      <pre>{`$msbuild src\\Phostide.sln /p:Configuration=Release /p:Platform=x64
# outputs: out\\Release\\Phostide.exe , ExplorerTAP.dll`}</pre>
    </div>
  );
}
