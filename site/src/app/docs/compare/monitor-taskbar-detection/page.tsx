import type { Metadata } from "next";
import { DocArticle } from "@/components/DocArticle";
import { pageMetadata } from "@/lib/seo";

export const metadata: Metadata = pageMetadata({
  title: "Using Phostide with your monitor's OLED Care and Taskbar Detection",
  description:
    "ASUS, MSI and Philips OLED monitors can dim the taskbar area on their own. How that differs from Phostide, and how to run both on Windows 11.",
  path: "/docs/compare/monitor-taskbar-detection",
});

export default function MonitorTaskbarDetectionPage() {
  return (
    <DocArticle
      active="/docs/compare"
      back={{ href: "/docs/compare", label: "Compare" }}
      title="Phostide and your monitor’s OLED Care"
      lede={
        <p>
          Some newer OLED monitors can spot the Windows taskbar and lower its
          brightness: ASUS, MSI and Philips all have a version of it, under
          names like Taskbar Detection or taskbar dimming. If yours has it, keep
          using it. Phostide works on a different layer, and the two can run
          together.
        </p>
      }
      related={[
        {
          href: "/docs/compare",
          label: "Compare every approach",
          body: "Auto-hide, firmware, overlays, idle mods and Phostide side by side.",
        },
        {
          href: "/docs/oled",
          label: "How to reduce OLED burn-in on Windows 11",
          body: "Windows settings, monitor care and where Phostide fits.",
        },
      ]}
      prev={{ href: "/docs/compare", label: "Compare" }}
      next={{ href: "/docs/compare/auto-hide", label: "Auto-hide vs Phostide" }}
    >
      <h2>What each one can see</h2>
      <p>
        <strong>The monitor</strong> only sees the picture it is sent. It can
        find a static area at the bottom of the screen and turn its brightness
        down, all the time, whatever you are doing.
      </p>
      <p>
        <strong>Phostide</strong> runs inside Windows, so it knows things the
        monitor can’t: whether you are at the keyboard, which display your
        cursor is on, whether a fullscreen game covers a screen, and which
        displays you want painted at all.
      </p>

      <h2>What Phostide adds</h2>
      <ul>
        <li>
          <strong>Motion while you work.</strong> The bar’s background slowly
          moves instead of sitting at one level.
        </li>
        <li>
          <strong>A deeper dim when you step away,</strong> after the idle time
          you choose.
        </li>
        <li>
          <strong>Rest for screens you aren’t using.</strong> While you work on
          one display, another can dim on its own.
        </li>
        <li>
          <strong>A pause for fullscreen games</strong> on the covered display
          only.
        </li>
        <li>
          <strong>Coverage where firmware has none:</strong> older OLED
          monitors, OLED TVs used as monitors and laptop screens.
        </li>
      </ul>

      <h2>Running both</h2>
      <ol>
        <li>
          Keep the monitor’s pixel refresh and pixel shift on. They do work
          Phostide can’t.
        </li>
        <li>
          Taskbar Detection is your call. With it on, the monitor also dims
          whatever Phostide paints in that strip, so themes look darker. If the
          bar looks too dim, raise Phostide’s intensity a little or turn
          detection off and let Phostide handle the bar.
        </li>
        <li>
          In Phostide, start with Aurora at a modest intensity and leave OLED
          Care at its 5-minute default.
        </li>
      </ol>

      <h2>Honest limits</h2>
      <p>
        Neither approach makes burn-in impossible. Both cut how long the same
        taskbar pixels sit bright. Exact firmware behaviour varies by model and
        firmware version, so check your monitor’s manual for what its version
        does.
      </p>
    </DocArticle>
  );
}
