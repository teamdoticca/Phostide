/**
 * Home-page FAQ. Plain answers to the questions that stop a download; every
 * answer must match AGENTS.md / README. Also emitted as FAQPage JSON-LD.
 */
export const HOME_FAQ: { q: string; a: string }[] = [
  {
    q: "Will it slow down my games?",
    a: "By default, no paint runs on a display while a fullscreen game covers it, so your GPU stays on the game. Other displays keep moving. You can turn this off under OLED Care › Pause themes in fullscreen.",
  },
  {
    q: "Does it work with my monitor’s own OLED Care or Taskbar Detection?",
    a: "Yes, you can run both. Your monitor’s features work on the panel; Phostide works on the taskbar’s background inside Windows. Phostide adds motion while you work, dimming when you step away and a pause for games.",
  },
  {
    q: "Does it work on OLED laptops?",
    a: "Yes. It works on any display connected to a Windows 11 22H2 or later x64 PC, laptop screens included.",
  },
  {
    q: "Does it hide my taskbar?",
    a: "No. The taskbar stays where it is, with Start, your icons, the tray and the clock on top. Phostide changes only its background.",
  },
  {
    q: "What happens after a Windows update?",
    a: "Quitting Phostide always brings the normal taskbar back. If an update or an Explorer restart drops the theme, exit Phostide from the tray and start it again. The experimental icon and clock dimming can stop working if Windows renames parts of the taskbar.",
  },
  {
    q: "Is it really free?",
    a: "Yes. Phostide is open source under the MIT License, with no account and no paid tier.",
  },
];
