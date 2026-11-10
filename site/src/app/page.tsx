import Link from "next/link";
import {
  IconDownload,
  IconGrid,
  IconLayers,
  IconUndo,
} from "@/components/Icons";
import { LatestDownload } from "@/components/LatestDownload";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { ThemeStrip } from "@/components/ThemeStrip";
import { resolveReleaseAssets } from "@/lib/releases";
import { themes } from "@/lib/themes";

export default async function Home() {
  const release = await resolveReleaseAssets();
  const tag = release.tag ?? "v0.1.4";

  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <SiteHeader />

      <section className="border-b border-line">
        <div className="mx-auto grid w-full max-w-site gap-10 px-6 py-16 sm:px-10 sm:py-24 lg:grid-cols-12">
          <div className="lg:col-span-7">
            <span className="inline-flex items-center gap-2 rounded-full border border-line px-3 py-1 text-xs text-soft">
              <span className="h-1.5 w-1.5 rounded-full bg-ember" />
              {tag} · Free · MIT · Windows 11
            </span>
            <h1 className="mt-7 text-4xl font-semibold leading-[1.05] tracking-tight sm:text-5xl lg:text-6xl">
              Animated themes,
              <br />
              painted into your real taskbar.
            </h1>
            <p className="mt-6 max-w-xl text-base leading-relaxed text-soft sm:text-lg">
              Phostide draws OLED‑safe motion directly into the Windows taskbar
              background inside Explorer. Your icons stay on top. Quit, and the
              normal bar comes back.
            </p>
            <div className="mt-9 flex flex-wrap items-center gap-3">
              <LatestDownload className="btn-ember">
                Download Setup.exe
              </LatestDownload>
              <LatestDownload kind="zip" className="btn-ghost">
                Portable zip
              </LatestDownload>
            </div>
            <p className="mt-4 text-xs text-soft">
              Signed per‑user installer · no admin, no account, no telemetry
            </p>
          </div>
          <div className="lg:col-span-5 lg:pt-14">
            <div className="divide-y divide-line rounded-2xl border border-line bg-panel text-sm">
              {[
                ["What it is", "Taskbar background fill"],
                ["What it isn't", "An overlay window"],
                ["Themes", "5, switch from tray"],
                ["Data", "Stays on your PC"],
                ["Made by", "Doticca"],
              ].map(([k, v]) => (
                <div
                  key={k}
                  className="flex justify-between gap-4 px-6 py-4"
                >
                  <span className="text-soft">{k}</span>
                  <span className="text-right">{v}</span>
                </div>
              ))}
            </div>
          </div>
        </div>
      </section>

      <section id="themes" className="mx-auto max-w-site px-6 py-20 sm:px-10 sm:py-28">
        <div className="mb-12 flex flex-col gap-6 sm:flex-row sm:items-end sm:justify-between">
          <div>
            <p className="eyebrow">Themes</p>
            <h2 className="mt-2 text-3xl font-semibold tracking-tight sm:text-4xl">
              Five looks. One click in the tray.
            </h2>
          </div>
          <p className="max-w-sm text-sm leading-relaxed text-soft">
            Every strip below is a real capture of the taskbar running Phostide.
            Dark by default, so OLED panels stay comfortable.
          </p>
        </div>
        <div className="space-y-4">
          {themes.map((theme, i) => (
            <div
              key={theme.id}
              id={`theme-${theme.id}`}
              className="rounded-xl border border-line bg-panel p-5"
            >
              <div className="mb-4 flex justify-between text-sm">
                <span className="font-medium">{theme.name}</span>
                <span className="text-soft">{String(i + 1).padStart(2, "0")}</span>
              </div>
              <ThemeStrip theme={theme} rounded />
            </div>
          ))}
        </div>
      </section>

      <section className="border-y border-line bg-panel">
        <div className="mx-auto grid max-w-site gap-6 px-6 py-20 sm:px-10 sm:py-24 lg:grid-cols-3">
          <div className="mb-2 lg:col-span-3">
            <p className="eyebrow">How it works</p>
            <h2 className="mt-2 text-3xl font-semibold tracking-tight sm:text-4xl">
              Inside the bar, not on top of it.
            </h2>
          </div>
          {[
            {
              icon: IconLayers,
              title: "Real background fill",
              body: "Phostide paints the taskbar's own background inside Explorer. No floating window covering anything.",
            },
            {
              icon: IconGrid,
              title: "Icons stay on top",
              body: "Start, pinned apps, tray and clock keep working exactly as before — the motion lives behind them.",
            },
            {
              icon: IconUndo,
              title: "Quit restores it",
              body: "Close Phostide from the tray and Windows' normal taskbar returns. Nothing left behind.",
            },
          ].map((step) => (
            <div
              key={step.title}
              className="rounded-xl border border-line bg-black p-7"
            >
              <step.icon className="h-5 w-5 text-ember" />
              <h3 className="mt-5 text-lg font-semibold">{step.title}</h3>
              <p className="mt-2 text-sm leading-relaxed text-soft">{step.body}</p>
            </div>
          ))}
        </div>
      </section>

      <section className="mx-auto grid max-w-site items-center gap-12 px-6 py-20 sm:px-10 sm:py-24 lg:grid-cols-2 lg:gap-16">
        <div>
          <p className="eyebrow">Privacy</p>
          <h2 className="mt-2 text-3xl font-semibold tracking-tight sm:text-4xl">
            Local only. That&apos;s the whole policy.
          </h2>
          <p className="mt-5 max-w-lg leading-relaxed text-soft">
            No account to create, no telemetry, no network calls needed to run.
            The source is open under MIT, so you can read every line.
          </p>
        </div>
        <ul className="grid grid-cols-2 gap-4 text-sm">
          {["No account", "No telemetry", "MIT open source", "Signed installer"].map(
            (label) => (
              <li key={label} className="rounded-xl border border-line p-5">
                <p className="font-medium">{label}</p>
              </li>
            )
          )}
        </ul>
      </section>

      <section id="download" className="mx-auto max-w-site px-6 pb-20 sm:px-10 sm:pb-28">
        <div className="rounded-2xl border border-line bg-panel p-8 sm:p-12">
          <div className="mb-10 flex flex-col gap-4 sm:flex-row sm:items-end sm:justify-between">
            <div>
              <p className="eyebrow">Download · {tag}</p>
              <h2 className="mt-2 text-3xl font-semibold tracking-tight sm:text-4xl">
                Get Phostide for Windows 11
              </h2>
            </div>
            <a
              href="https://github.com/teamdoticca/Phostide"
              className="text-sm text-soft hover:text-white"
              rel="noreferrer"
            >
              github.com/teamdoticca/Phostide ↗
            </a>
          </div>
          <div className="grid gap-5 lg:grid-cols-2">
            <div className="flex flex-col rounded-xl border border-ember/50 bg-emberdim/40 p-7">
              <div className="flex flex-wrap items-center justify-between gap-2">
                <span className="text-lg font-semibold">Setup.exe</span>
                <span className="rounded bg-ember px-2 py-1 text-xs font-semibold text-black">
                  Recommended
                </span>
              </div>
              <p className="mt-2 text-sm text-soft">
                Signed, per‑user install. Adds a tray icon and Start menu entry.
                No admin rights.
              </p>
              <LatestDownload className="btn-ember mt-8 self-start">
                <span className="inline-flex items-center gap-2">
                  <IconDownload className="h-4 w-4" />
                  Download installer
                </span>
              </LatestDownload>
            </div>
            <div className="flex flex-col rounded-xl border border-line bg-black p-7">
              <span className="text-lg font-semibold">Portable zip</span>
              <p className="mt-2 text-sm text-soft">
                Unzip anywhere and run. Nothing installed — delete the folder to
                remove.
              </p>
              <LatestDownload kind="zip" className="btn-ghost mt-8 self-start">
                Download zip
              </LatestDownload>
            </div>
          </div>
          <p className="mt-6 text-xs text-soft">
            Both are published on GitHub Releases. Free forever under the MIT
            License.{" "}
            <Link href="/docs" className="text-ember hover:underline">
              Read the docs
            </Link>
          </p>
        </div>
      </section>

      <SiteFooter />
    </div>
  );
}
