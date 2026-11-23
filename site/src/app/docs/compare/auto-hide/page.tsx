import type { Metadata } from "next";
import { DocArticle } from "@/components/DocArticle";
import { pageMetadata } from "@/lib/seo";

export const metadata: Metadata = pageMetadata({
  title: "Auto-hide taskbar vs Phostide on an OLED",
  description:
    "Auto-hiding the Windows 11 taskbar is the usual OLED advice. What it protects, what it costs, and when keeping a visible bar with Phostide fits better.",
  path: "/docs/compare/auto-hide",
});

export default function AutoHidePage() {
  return (
    <DocArticle
      active="/docs/compare"
      back={{ href: "/docs/compare", label: "Compare" }}
      title="Auto-hide or keep the taskbar? An OLED guide"
      lede={
        <p>
          “Auto-hide your taskbar” is the first advice most OLED owners hear,
          and it works: a hidden bar can’t burn in. The question is whether you
          want to live without it.
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
          body: "The full checklist.",
        },
      ]}
      prev={{ href: "/docs/compare/monitor-taskbar-detection", label: "With your monitor’s OLED Care" }}
      next={{ href: "/docs/install", label: "Install Phostide" }}
    >
      <h2>How to turn on auto-hide</h2>
      <p>
        Settings › Personalization › Taskbar › Taskbar behaviors › tick{" "}
        <strong>Automatically hide the taskbar</strong>. Move the pointer to
        the bottom edge to bring it back.
      </p>

      <h2>What it costs</h2>
      <ul>
        <li>The clock, tray icons and notifications aren’t visible at a glance.</li>
        <li>You lose the row of open apps you can see and click at any time.</li>
        <li>
          The bar slides in and out on every visit, and some owners find the
          animation laggy on high-refresh OLED panels.
        </li>
        <li>
          On several monitors, each bar hides and shows on its own edge, which
          can get fiddly.
        </li>
      </ul>

      <h2>When auto-hide is the right call</h2>
      <p>
        If you rarely look at the taskbar and you’re happy reaching for it,
        auto-hide gives the strongest protection for free. You don’t need
        Phostide for the taskbar.
      </p>

      <h2>When a visible bar fits better</h2>
      <p>
        If you use the clock, tray and open-apps row all day, Phostide lets you
        keep them and still cut how long the same pixels sit bright: the bar’s
        background moves while you work, OLED Care dims it when you step away,
        and it pauses on displays covered by a fullscreen game. Icons stay on
        top, and nothing covers the bar.
      </p>

      <h2>Honest limits</h2>
      <p>
        A hidden bar emits no light at all; a visible one, however well cared
        for, still does. Phostide reduces the wear on a visible taskbar. It
        doesn’t make it equal to hiding it.
      </p>
    </DocArticle>
  );
}
