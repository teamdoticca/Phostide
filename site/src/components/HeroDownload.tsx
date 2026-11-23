"use client";

import { useState, useSyncExternalStore } from "react";
import { NextSteps } from "@/components/NextSteps";

type HeroDownloadProps = {
  setupUrl: string;
  zipUrl: string;
  pageUrl: string;
};

const noopSubscribe = () => () => {};

/** True only when we can tell the visitor is not on a Windows PC. */
function isNotWindows(): boolean {
  const nav = navigator as Navigator & {
    userAgentData?: { platform?: string; mobile?: boolean };
  };
  const platform = nav.userAgentData?.platform;
  if (platform) return !/windows/i.test(platform);
  const ua = navigator.userAgent;
  if (/windows/i.test(ua)) return false;
  return /android|iphone|ipad|ipod|macintosh|mac os x|cros|linux/i.test(ua);
}

/**
 * Hero download actions. Windows visitors (and anyone we can't identify) get
 * the signed installer. Phones, Macs and Linux get a way to carry the link to
 * their PC instead of a .exe they can't run.
 */
export function HeroDownload({ setupUrl, zipUrl, pageUrl }: HeroDownloadProps) {
  // Platform never changes mid-visit; the server renders the Windows CTA.
  const elsewhere = useSyncExternalStore(noopSubscribe, isNotWindows, () => false);
  const [copied, setCopied] = useState(false);
  const [started, setStarted] = useState(false);

  const carry = async () => {
    const data = {
      title: "Phostide for Windows 11",
      text: "OLED taskbar care for Windows 11. Open on your PC to install.",
      url: pageUrl,
    };
    if (typeof navigator.share === "function") {
      try {
        await navigator.share(data);
        return;
      } catch {
        /* dismissed or blocked: fall through to copy */
      }
    }
    try {
      await navigator.clipboard.writeText(pageUrl);
      setCopied(true);
      window.setTimeout(() => setCopied(false), 2400);
    } catch {
      /* clipboard blocked: the visible URL below is still selectable */
    }
  };

  if (elsewhere) {
    return (
      <div>
        <p className="text-[15px] leading-relaxed text-zinc-200">
          Phostide is a Windows 11 app. Send the link to your PC and install it
          there.
        </p>
        <div className="mt-4 flex flex-col gap-3 sm:flex-row sm:flex-wrap sm:items-center">
          <button type="button" onClick={carry} className="btn-ember">
            {copied ? "Link copied" : "Send link to my PC"}
          </button>
          <a href={setupUrl} className="btn-ghost">
            Download Setup.exe anyway
          </a>
        </div>
        <p className="mt-3 select-all font-mono text-[13px] text-soft">
          {pageUrl.replace(/^https?:\/\//, "")}
        </p>
      </div>
    );
  }

  return (
    <div>
      <div className="flex flex-col gap-3 sm:flex-row sm:flex-wrap sm:items-center">
        <a href={setupUrl} className="btn-ember" onClick={() => setStarted(true)}>
          Download for Windows 11
        </a>
        <a href={zipUrl} className="btn-ghost">
          Portable zip
        </a>
      </div>
      {started ? (
        <div className="mt-5 rounded-xl border border-line bg-panel p-5" role="status">
          <p className="text-sm font-medium text-zinc-100">Downloading. Next:</p>
          <NextSteps stacked className="mt-3" />
        </div>
      ) : (
        <p className="mt-3 text-[13px] text-soft">
          Signed installer · per-user, no admin rights · no account
        </p>
      )}
    </div>
  );
}
