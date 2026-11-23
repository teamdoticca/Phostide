import type { Metadata } from "next";
import Link from "next/link";
import { DocArticle } from "@/components/DocArticle";
import { pageMetadata } from "@/lib/seo";

export const metadata: Metadata = pageMetadata({
  title: "OLED laptops: taskbar burn-in care on Windows 11",
  description:
    "OLED laptop screens rarely have a monitor's care menus. How to protect the Windows 11 taskbar on an OLED laptop, with Windows settings, your maker's app and Phostide.",
  path: "/docs/oled-laptops",
});

export default function OledLaptopsPage() {
  return (
    <DocArticle
      active="/docs/oled"
      back={{ href: "/docs/oled", label: "OLED notes" }}
      title="OLED laptops: taskbar care without monitor menus"
      lede={
        <p>
          An OLED laptop screen shows the same Windows taskbar for every hour
          you work, often closer to your eyes and at higher brightness than a
          desk monitor. Unlike most OLED monitors, it usually has no on-screen
          menu with taskbar detection or care settings of its own.
        </p>
      }
      related={[
        {
          href: "/docs/oled",
          label: "How to reduce OLED burn-in on Windows 11",
          body: "The full checklist, laptop or desktop.",
        },
        {
          href: "/docs/install",
          label: "Install & run",
          body: "Signed Setup.exe, per-user, no admin rights.",
        },
      ]}
      prev={{ href: "/docs/oled", label: "OLED notes" }}
      next={{ href: "/docs/compare", label: "Compare approaches" }}
    >
      <h2>Windows settings that matter most on a laptop</h2>
      <ol>
        <li>
          <strong>Short screen timeouts, on battery and plugged in.</strong>{" "}
          Settings › System › Power &amp; battery › Screen and sleep. Laptops
          spend a lot of time plugged in on a desk, so set the plugged-in
          timeout too.
        </li>
        <li>
          <strong>Dark mode</strong> (Settings › Personalization › Colors) and a
          moderate brightness.
        </li>
        <li>
          <strong>Seconds on the clock</strong> (Settings › Personalization ›
          Taskbar › Taskbar behaviors), so its digits don’t stay frozen.
        </li>
      </ol>

      <h2>Check your maker’s app</h2>
      <p>
        Some laptop makers ship their own OLED care utility. On ASUS OLED
        laptops, for example, MyASUS has OLED care options such as pixel shift
        and a translucent or auto-hidden taskbar. Look in your maker’s control
        app and turn on what it offers. Phostide doesn’t replace it.
      </p>

      <h2>What Phostide adds on a laptop</h2>
      <ul>
        <li>
          <strong>A visible taskbar that keeps moving.</strong> The bar’s
          background slowly drifts while you work, with your icons on top.
          Aurora at the fresh-install 35% intensity is the quietest place to
          start.
        </li>
        <li>
          <strong>Dimming when you step away.</strong> OLED Care dims the bar
          after 5 idle minutes by default, before Windows turns the screen off.
        </li>
        <li>
          <strong>A pause for fullscreen.</strong> Games and fullscreen video
          pause the painting on that screen.
        </li>
        <li>
          <strong>Docked to an LCD?</strong> If you plug the laptop into an
          ordinary monitor, turn Phostide off for that display on the Monitors
          tab and keep it on for the laptop’s OLED. It works the other way round
          too: an OLED monitor next to an LCD laptop screen.
        </li>
      </ul>
      <p>
        Requirements are the same as on a desktop: Windows 11 22H2 or later on
        an x64 PC. It is free, open source and installs per-user without admin
        rights. <Link href="/docs/install">Install &amp; run</Link>.
      </p>

      <h2>Honest limits</h2>
      <p>
        Phostide reduces how long the taskbar’s pixels stay bright and still.
        It can’t make burn-in impossible, and it doesn’t replace the screen
        timeout, which does more for an idle laptop than anything else.
      </p>
    </DocArticle>
  );
}
