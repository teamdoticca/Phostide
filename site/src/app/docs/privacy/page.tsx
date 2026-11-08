import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "Privacy & permissions",
  description:
    "What Phostide runs locally, what injection means, and what never leaves your PC.",
};

export default function PrivacyPage() {
  return (
    <div className="prose-docs">
      <h1 className="font-[family-name:var(--font-display)] text-4xl font-semibold tracking-tight">
        Privacy & permissions
      </h1>
      <p className="mt-4 max-w-2xl text-lg">
        Phostide is a local Windows utility. There is no account, no telemetry
        cloud, and no network requirement for themes to run.
      </p>

      <h2>What runs on your machine</h2>
      <ul>
        <li>
          <code>Phostide.exe</code> — tray host that manages settings and
          injection.
        </li>
        <li>
          <code>ExplorerTAP.dll</code> — loaded into Explorer to paint the
          taskbar fill.
        </li>
      </ul>

      <h2>Why injection exists</h2>
      <p>
        The Windows 11 taskbar background is owned by Explorer. To animate that
        fill without an overlay window, Phostide uses a supported-style XAML
        Diagnostics TAP path plus a window hook to load the helper. That is a
        privileged local technique: it can see and modify Explorer UI state for
        the taskbar brush. It is not a browser extension and it does not
        request cloud OAuth scopes.
      </p>

      <h2>Data on disk</h2>
      <ul>
        <li>
          Settings: <code>%APPDATA%\Phostide\settings.json</code>
        </li>
        <li>
          Optional diagnostics: <code>%TEMP%\Phostide\*.log</code>
        </li>
      </ul>
      <p>Those files stay local unless you choose to share them.</p>

      <h2>What Phostide does not do</h2>
      <ul>
        <li>No analytics beacons from the native host.</li>
        <li>No modification of system files under <code>Windows\System32</code>.</li>
        <li>No persistence that survives a clean uninstall of the unpack folder + AppData settings (aside from optional Windows tray / startup choices you enable yourself).</li>
      </ul>

      <h2>Website note</h2>
      <p>
        The marketing site at{" "}
        <a href="https://phostide.doticca.com">phostide.doticca.com</a> may
        fetch the public GitHub Releases API to resolve the download button.
        That call is separate from the desktop app.
      </p>

      <h2>License</h2>
      <p>
        Phostide is MIT-licensed. Source:{" "}
        <a href="https://github.com/teamdoticca/Phostide">
          github.com/teamdoticca/Phostide
        </a>
        .
      </p>
    </div>
  );
}
