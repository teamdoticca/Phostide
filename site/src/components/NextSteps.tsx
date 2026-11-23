import Link from "next/link";

const STEPS = [
  {
    title: "Run the installer",
    body: (
      <>
        If SmartScreen appears, check the publisher reads Doticca SRL, then
        choose More info › Run anyway.{" "}
        <Link href="/docs/troubleshooting#fix-smartscreen" className="text-zinc-200 underline decoration-line underline-offset-4 hover:decoration-zinc-400">
          Why
        </Link>
      </>
    ),
  },
  {
    title: "Launch Phostide",
    body: "Leave Launch Phostide ticked on the last Setup screen. Its icon sits in the tray, next to the clock.",
  },
  {
    title: "Tell it which displays are OLED",
    body: "A one-time Welcome asks, then you’re done. It starts with Aurora at 35% and OLED Care after 5 idle minutes.",
  },
];

/** What to do after the download starts: three steps to a painted taskbar. */
export function NextSteps({
  className = "",
  stacked = false,
}: {
  className?: string;
  stacked?: boolean;
}) {
  return (
    <ol className={`grid ${stacked ? "gap-4" : "gap-6 sm:grid-cols-3"} ${className}`}>
      {STEPS.map((s, i) => (
        <li key={s.title} className="border-t border-line pt-4">
          <span className="text-sm tabular-nums text-ember">{i + 1}</span>
          <p className="mt-1 font-medium text-zinc-100">{s.title}</p>
          <p className="mt-1 text-sm leading-relaxed text-soft">{s.body}</p>
        </li>
      ))}
    </ol>
  );
}
