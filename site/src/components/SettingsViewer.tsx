"use client";

import { useState } from "react";
import { SettingsShot, type SettingsShotId } from "@/components/SettingsShot";

const TABS: { id: SettingsShotId; label: string; line: string }[] = [
  {
    id: "taskbar",
    label: "Taskbar",
    line: "Pick one of five themes and an intensity, for every display or just one.",
  },
  {
    id: "oled-care",
    label: "OLED Care",
    line: "Choose when an idle bar dims and how far, with pauses for a locked PC and fullscreen apps. The everyday preset changes care settings only, never your theme.",
  },
  {
    id: "monitors",
    label: "Monitors",
    line: "Turn each display on or off and see what it’s doing right now.",
  },
  {
    id: "system",
    label: "System",
    line: "Rendering mode, Start with Windows, one-click updates and Copy diagnostics for bug reports.",
  },
];

/** The real Settings window, one tab at a time, as the app shows it. */
export function SettingsViewer() {
  const [active, setActive] = useState<SettingsShotId>("taskbar");
  const current = TABS.find((t) => t.id === active) ?? TABS[0];

  return (
    <div className="grid gap-10 lg:grid-cols-[minmax(0,1fr)_minmax(0,24rem)] lg:items-start">
      <div>
        <div
          className="inline-grid grid-cols-2 gap-1 rounded-lg border border-line p-1 sm:grid-cols-4"
          role="tablist"
          aria-label="Settings tabs"
        >
          {TABS.map((t) => {
            const on = t.id === active;
            return (
              <button
                key={t.id}
                type="button"
                role="tab"
                id={`settings-tab-${t.id}`}
                aria-selected={on}
                aria-controls="settings-panel"
                onClick={() => setActive(t.id)}
                className={`rounded-md px-4 py-2 text-sm transition-colors duration-150 ${
                  on ? "bg-white/[0.08] text-zinc-50" : "text-soft hover:text-zinc-200"
                }`}
              >
                {t.label}
              </button>
            );
          })}
        </div>
        <p className="mt-6 min-h-[4.5em] max-w-xl text-lg leading-relaxed text-zinc-300" aria-live="polite">
          {current.line}
        </p>
        <div className="mt-8 border-t border-line pt-6">
          <p className="font-medium text-zinc-100">From the tray, without opening Settings</p>
          <ul className="mt-3 space-y-2 text-soft">
            <li>Left-click to pause or resume painting.</li>
            <li>Pause for 1 hour, or until the next restart, for a presentation or a stream.</li>
            <li>Switch the theme on every display from the Theme menu.</li>
          </ul>
        </div>
      </div>
      <div id="settings-panel" role="tabpanel" aria-labelledby={`settings-tab-${active}`}>
        <SettingsShot id={active} />
      </div>
    </div>
  );
}
