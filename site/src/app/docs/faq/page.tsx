import type { Metadata } from "next";
import Link from "next/link";
import { IconArrowLeft, IconArrowRight } from "@/components/Icons";
import { SiteFooter } from "@/components/SiteFooter";
import { SiteHeader } from "@/components/SiteHeader";
import { absoluteUrl, pageMetadata, SITE_NAME } from "@/lib/seo";

export const metadata: Metadata = pageMetadata({
  title: "FAQ",
  description:
    "What Phostide is, how Settings tabs work, dual-monitor OLED setup, Windhawk, themes, and burn-in limits.",
  path: "/docs/faq",
});

const faqs: { q: string; a: string }[] = [
  {
    q: "What does Phostide do?",
    a: "Phostide paints OLED-safe animated themes into the real Windows 11 taskbar background (Explorer BackgroundFill). Icons stay on top. It is not a floating overlay and not Windows auto-hide. Quit from the tray and the normal bar comes back.",
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
    a: "Tray › Settings… has three tabs. Taskbar: choose Apply to All or a monitor number, set theme and intensity, then Apply. Monitors: turn individual displays on or off (off keeps a stock Windows taskbar). System: shared Auto/GPU/CPU rendering for every bar, Start with Windows, and updates. Rendering is not per-monitor.",
  },
  {
    q: "Can I theme only my OLED and leave the other monitor alone?",
    a: "Yes. Open Settings › Monitors and uncheck the non-OLED display. Then on the Taskbar tab, Apply to the OLED monitor number, pick Space or Aurora at a modest intensity, and Apply. You can also leave both on and set different themes per display.",
  },
  {
    q: "Which theme should I use on an OLED panel?",
    a: "Start with Space or Aurora — they stay darkest. Keep intensity modest (default is 45%). Weather and Sea Waves are busier; dial intensity down if you want calmer motion.",
  },
  {
    q: "Does Phostide prevent OLED burn-in completely?",
    a: "No. It reduces how long the background fill stays as one static bright strip. Icons, Start, the tray, and the clock remain Windows-drawn and static, so they can still contribute to burn-in. Phostide is not a burn-in warranty.",
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
