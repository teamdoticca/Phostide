import type { Metadata } from "next";
import Link from "next/link";
import { IconArrowLeft, IconArrowRight } from "@/components/Icons";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { absoluteUrl, pageMetadata, SITE_NAME } from "@/lib/seo";

export const metadata: Metadata = pageMetadata({
  title: "FAQ",
  description:
    "What Phostide is (OLED taskbar care), Settings tabs, dual-monitor recipe, Windhawk, themes, and honest limits.",
  path: "/docs/faq",
});

const faqs: { q: string; a: string }[] = [
  {
    q: "What does Phostide do?",
    a: "Phostide is an OLED taskbar solution for Windows 11: moving fill while you work, hybrid idle Care when you step away, and per-monitor control — painted into Explorer’s real BackgroundFill. Icons stay on top. It is not a floating overlay and not Windows auto-hide. Quit from the tray and the normal bar comes back.",
  },
  {
    q: "Does Phostide need Windhawk or other software?",
    a: "No. Phostide is standalone. It does not require Windhawk, TranslucentTB, RainbowTaskbar, or any separate taskbar overlay. Windows 11 22H2+ x64 is enough.",
  },
  {
    q: "Do I need administrator rights?",
    a: "No. The signed Setup.exe installs per-user under your LocalAppData Programs folder. A portable zip is also available — unzip and run Phostide.exe.",
  },
  {
    q: "Is there a theme editor?",
    a: "No. The shipped app has exactly five built-in themes: Rainbow, Sea Waves, Weather, Space, and Aurora. There is no end-user theme editor or custom theme import. (Developers building from source can use ThemeLab to iterate GPU shaders — that is not a product Settings feature.)",
  },
  {
    q: "How do Settings tabs work?",
    a: "Tray › Settings… has four tabs. Taskbar: Apply to All or a monitor number, theme and intensity, then Apply. OLED Care: hybrid idle fill dim and optional experimental chrome/clock care (everyday preset is care-only — it does not change your theme). Monitors: turn displays on or off (off keeps a stock Windows taskbar). System: shared Auto/GPU/CPU rendering, Start with Windows, and updates. Rendering mode is app-global; Care only affects monitors left on in Monitors.",
  },
  {
    q: "Can I theme only my OLED and leave the other monitor alone?",
    a: "Yes. Open Settings › Monitors and uncheck the non-OLED display. Then on the Taskbar tab, Apply to the OLED monitor number, pick Space or Aurora at a modest intensity, and Apply. You can also leave both on and set different themes per display. OLED Care follows the same on/off list.",
  },
  {
    q: "Which theme should I use on an OLED panel?",
    a: "Start with Space or Aurora — they stay darkest. Keep intensity modest (default is 45%). Weather and Sea Waves are busier; dial intensity down if you want calmer motion. OLED Care › OLED everyday preset sets idle care only (5m idle, low fill, chrome/clock care, night boost) — it does not change your Taskbar theme or intensity.",
  },
  {
    q: "What is OLED Care?",
    a: "A dedicated Settings tab. Idle is hybrid: walk away (no input for Idle after) and every enabled bar dims; while you keep working, leave the cursor on one display and the other can dim alone after Idle after (attendance follows the cursor). Optional experimental toggles dim icons/tray/clock (including Search) or micro-nudge the clock only on that idle bar — not on the one you’re using. Night boost shortens the idle timeout and darkens the fill in the evening. OLED everyday preset applies those care defaults only — it does not change Taskbar theme or intensity. Exit restores stock brushes and chrome.",
  },
  {
    q: "Does Phostide prevent OLED burn-in completely?",
    a: "No. Phostide reduces locked-bright time on the taskbar (moving fill + idle Care); it is not a burn-in warranty or pixel-shift replacement. Themes move the fill; residual Windows chrome stays static unless you enable experimental chrome dim / clock nudge (may no-op after Explorer XAML renames). Windows tips: show clock seconds; hide the date if you do not need it.",
  },
  {
    q: "Does it collect data or need an account?",
    a: "No account and no telemetry. Themes work offline. An optional GitHub update check is the only network use. Settings live in %APPDATA%\\Phostide\\settings.json.",
  },
  {
    q: "Where do I download it?",
    a: "https://phostide.doticca.com or GitHub Releases for teamdoticca/Phostide. Prefer the signed Setup.exe; portable zip is available on the same release.",
  },
];

function FaqJsonLd() {
  const data = {
    "@context": "https://schema.org",
    "@type": "FAQPage",
    mainEntity: faqs.map((item) => ({
      "@type": "Question",
      name: item.q,
      acceptedAnswer: {
        "@type": "Answer",
        text: item.a,
      },
    })),
    isPartOf: {
      "@type": "WebSite",
      name: SITE_NAME,
      url: absoluteUrl("/"),
    },
  };

  return (
    <script
      type="application/ld+json"
      dangerouslySetInnerHTML={{ __html: JSON.stringify(data) }}
    />
  );
}

export default function FaqPage() {
  return (
    <div className="min-h-screen bg-void text-zinc-100">
      <FaqJsonLd />
      <SiteHeader active="/docs" />

      <main className="mx-auto max-w-site px-6 py-16 sm:px-10 sm:py-20">
        <Link
          href="/docs"
          className="inline-flex items-center gap-2 text-sm text-soft hover:text-white"
        >
          <IconArrowLeft />
          Docs
        </Link>
        <p className="mt-6 eyebrow">FAQ</p>
        <h1 className="mt-3 max-w-3xl text-4xl font-semibold tracking-tight sm:text-5xl">
          Short answers agents (and humans) need.
        </h1>
        <p className="mt-5 max-w-2xl text-lg text-soft">
          Canonical facts about mechanism, Settings tabs, dual‑monitor OLED,
          and what Phostide is not. Full brief:{" "}
          <a href="/llms-full.txt" className="text-ember hover:underline">
            llms-full.txt
          </a>
          .
        </p>

        <div className="mt-14 space-y-4">
          {faqs.map((item) => (
            <section
              key={item.q}
              className="rounded-xl border border-line bg-panel p-6 sm:p-8"
            >
              <h2 className="text-lg font-semibold">{item.q}</h2>
              <p className="mt-3 text-soft leading-relaxed">{item.a}</p>
            </section>
          ))}
        </div>

        <div className="mt-12 flex flex-wrap justify-between gap-4 text-sm">
          <Link
            href="/docs/install"
            className="inline-flex items-center gap-2 text-soft hover:text-white"
          >
            <IconArrowLeft />
            Install &amp; run
          </Link>
          <Link
            href="/docs/compare"
            className="inline-flex items-center gap-2 text-ember"
          >
            Compare approaches <IconArrowRight />
          </Link>
        </div>
      </main>

      <SiteFooter />
    </div>
  );
}
