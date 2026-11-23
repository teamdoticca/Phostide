import type { Metadata } from "next";
import Link from "next/link";
import { DocArticle } from "@/components/DocArticle";
import { SettingsShot } from "@/components/SettingsShot";
import { pageMetadata } from "@/lib/seo";

export const metadata: Metadata = pageMetadata({
  title: "How to reduce OLED burn-in on Windows 11 (taskbar, icons, clock)",
  description:
    "Practical, tool-agnostic ways to reduce OLED burn-in on Windows 11: dark mode, brightness, screen timeouts, the taskbar, clock and icons, your monitor's own care, and where Phostide fits.",
  path: "/docs/oled",
});

export default function OledPage() {
  return (
    <DocArticle
      active="/docs/oled"
      title="How to reduce OLED burn-in on Windows 11"
      lede={
        <p>
          OLED pixels make their own light, and they wear a little with every
          hour of it. Content that changes spreads that wear evenly. Content
          that sits still, bright, in the same place for months wears one spot
          faster, and that is what burn-in is. Windows has a few of those
          spots. Here is what helps, starting with what costs nothing.
        </p>
      }
      related={[
        {
          href: "/docs/oled-laptops",
          label: "OLED laptops",
          body: "Taskbar care on a laptop screen, where monitor features don’t exist.",
        },
        {
          href: "/docs/compare/monitor-taskbar-detection",
          label: "Phostide with your monitor’s OLED Care",
          body: "How Taskbar Detection and Phostide work together.",
        },
        {
          href: "/docs/compare/auto-hide",
          label: "Auto-hide vs Phostide",
          body: "When hiding the taskbar is the right call, and what it costs.",
        },
        {
          href: "/docs/compare",
          label: "Compare every approach",
          body: "Auto-hide, firmware, overlays, idle mods and Phostide side by side.",
        },
      ]}
      prev={{ href: "/docs/themes", label: "Themes" }}
      next={{ href: "/docs/compare", label: "Next: Compare" }}
    >
      <h2>Where Windows keeps pixels still</h2>
      <ul>
        <li>
          <strong>The taskbar.</strong> A full-width strip at the bottom of the
          screen, on every hour the PC is on. It is the biggest static shape on
          most desktops.
        </li>
        <li>
          <strong>The clock, tray and pinned icons.</strong> Small, but bright
          and in exactly the same place.
        </li>
        <li>
          <strong>Desktop icons and wallpaper,</strong> whenever nothing covers
          them.
        </li>
        <li>
          <strong>App chrome you keep open all day:</strong> a sidebar, a
          toolbar, a chat window docked to one side.
        </li>
      </ul>

      <h2>Start with Windows itself</h2>
      <ol>
        <li>
          <strong>Use dark mode.</strong> Settings › Personalization › Colors ›
          Choose your mode › Dark. Dark pixels emit less light, so they wear
          less.
        </li>
        <li>
          <strong>Keep desktop brightness moderate.</strong> Wear grows with
          light output. Save peak brightness for HDR content that changes.
        </li>
        <li>
          <strong>Turn the screen off when you leave.</strong> Settings ›
          System › Power (Power &amp; battery on a laptop) › Screen and sleep,
          then pick a short “turn my screen
          off” time. It is the single biggest saving for an idle desk.
        </li>
        <li>
          <strong>Quiet the clock.</strong> Settings › Personalization ›
          Taskbar › Taskbar behaviors › Show seconds in system tray clock keeps
          the digits changing. Hide the date if you don’t need it.
        </li>
        <li>
          <strong>Hide desktop icons</strong> (right-click the desktop › View ›
          Show desktop icons) and use a dark or slideshow wallpaper.
        </li>
        <li>
          <strong>Consider auto-hide.</strong> Settings › Personalization ›
          Taskbar › Taskbar behaviors › Automatically hide the taskbar removes
          the strip completely, at the cost of the clock, tray and open apps at
          a glance.{" "}
          <Link href="/docs/compare/auto-hide">Auto-hide vs Phostide</Link>.
        </li>
      </ol>

      <h2>Let your monitor do its part</h2>
      <p>
        Most OLED monitors have care features of their own. Read your
        monitor’s manual for the exact names. The common ones:
      </p>
      <ul>
        <li>
          <strong>Pixel refresh or panel care</strong> runs in standby after
          hours of use. Let the monitor go to standby normally rather than
          cutting power at the wall, so it can run.
        </li>
        <li>
          <strong>Pixel shift</strong> moves the whole picture by a few pixels
          over time.
        </li>
        <li>
          <strong>Taskbar or logo detection</strong> on some newer models dims
          the area where the taskbar sits.{" "}
          <Link href="/docs/compare/monitor-taskbar-detection">
            Using it with Phostide
          </Link>
          .
        </li>
      </ul>
      <p>
        Laptop OLED screens rarely have these menus.{" "}
        <Link href="/docs/oled-laptops">What to do on a laptop</Link>.
      </p>

      <h2>Where Phostide fits</h2>
      <p>
        The steps above deal with an idle screen and with brightness. The
        taskbar stays a problem while you work, because hiding it is the only
        built-in fix. Phostide keeps it visible and changes how long its pixels
        sit still:
      </p>
      <ul>
        <li>
          <strong>While you work,</strong> the taskbar’s background slowly
          moves (five built-in themes). Your icons stay on top; it is not an
          overlay.
        </li>
        <li>
          <strong>When you step away,</strong> OLED Care dims the bar after the
          idle time you pick (1, 5, 10 or 30 minutes; 5 by default). While you
          work on one display, a display you’re not using can dim on its own.
        </li>
        <li>
          <strong>When you game,</strong> painting pauses on the display a
          fullscreen app covers. Other displays keep moving.
        </li>
        <li>
          <strong>On mixed desks,</strong> turn Phostide off for an LCD on the
          Monitors tab and it stays a plain Windows taskbar.
        </li>
      </ul>
      <p>
        Optional, experimental: OLED Care can also dim the icons, tray and
        clock, or nudge the clock by a pixel or two, on an idle bar. Night boost
        dims sooner and darker in the evening. The OLED everyday preset turns
        these care settings on together; it never changes your theme.
      </p>
      <div className="grid gap-4 sm:grid-cols-2">
        <SettingsShot id="oled-care" />
        <SettingsShot id="monitors" />
      </div>

      <h2>Honest limits</h2>
      <ul>
        <li>
          Nothing on Windows makes burn-in impossible. Phostide reduces how long
          the same taskbar pixels stay bright; it is not a burn-in warranty or a
          replacement for your monitor’s pixel refresh.
        </li>
        <li>
          Themes move the taskbar’s background. Icons, Start, Search, the tray
          and the clock are still Windows UI, unless you turn on the
          experimental chrome care, which can stop working if a Windows update
          renames parts of the taskbar.
        </li>
        <li>
          Quit from the tray and the stock taskbar comes back. Phostide does
          not change system files.
        </li>
      </ul>
    </DocArticle>
  );
}
