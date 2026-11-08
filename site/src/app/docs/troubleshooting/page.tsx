import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "Troubleshooting",
  description:
    "Fix Phostide inject and bind issues without killing Explorer for normal use.",
};

export default function TroubleshootingPage() {
  return (
    <div className="prose-docs">
      <h1 className="font-[family-name:var(--font-display)] text-4xl font-semibold tracking-tight">
        Troubleshooting
      </h1>
      <p className="mt-4 max-w-2xl text-lg">
        Most users only need to launch <code>Phostide.exe</code>. This page
        covers inject/bind edge cases and clarifies the difference between
        product start and developer reinject rituals.
      </p>

      <h2>Do not kill Explorer for normal use</h2>
      <p>
        Product start injects the TAP helper and waits for readiness. It does{" "}
        <strong className="text-[var(--ink)]">not</strong> restart{" "}
        <code>explorer.exe</code>. Restarting Explorer is a{" "}
        <em>development</em> ritual used when iterating on the DLL so XAML
        Diagnostics can rebind cleanly — not something the tray app expects
        users to do.
      </p>

      <h2>Quick checks</h2>
      <ul>
        <li>Confirm Windows 11 22H2+ with the XAML taskbar.</li>
        <li>
          Run the x64 build; ARM64 / x86 packages are not the shipping target.
        </li>
        <li>
          Quit any older Phostide instance from the tray before launching a new
          unpack.
        </li>
        <li>
          After quit, the original taskbar brush should return. If a theme
          appears stuck, exit Phostide fully and sign out / back in as a last
          resort.
        </li>
      </ul>

      <h2>Log locations</h2>
      <p>
        Diagnostic logs live under <code>%TEMP%\Phostide\</code> (not a bare{" "}
        <code>%TEMP%\tap.log</code>):
      </p>
      <ul>
        <li>
          <code>tap.log</code> — TAP / bind lines (Unicode)
        </li>
        <li>
          <code>inject.log</code> — host copy/hook/ready wait
        </li>
      </ul>
      <p>
        Healthy product bind often includes <code>ixde succeeded</code> and a{" "}
        <code>Bound fill WxH</code> with a real size such as{" "}
        <code>2560x48</code>. A later tiny <code>8x8</code> line can appear
        after a good bind; if the Bound event stays set, treat the large bind
        as success.
      </p>

      <h2>Settings sanity</h2>
      <pre>{`Get-Content "$env:APPDATA\\Phostide\\settings.json"`}</pre>
      <p>
        Ensure <code>enabled</code> is true and <code>theme</code> is{" "}
        <code>0–4</code> (Rainbow → Aurora).
      </p>

      <h2>Still stuck?</h2>
      <p>
        Open an issue on{" "}
        <a href="https://github.com/teamdoticca/Phostide/issues">
          GitHub Issues
        </a>{" "}
        with your Windows build, Phostide version, and a redacted snip of{" "}
        <code>tap.log</code> / <code>inject.log</code>.
      </p>
    </div>
  );
}
