# Design

<!-- impeccable:design-schema 1 -->

## World

OLED void + living cyan light. The page is deep black water/desk night; bioluminescent ribbons carry brand energy. Marketing Persuade surface for Phostide.

## Color

- Ground: `#020408` (`--bg0`), elevated `#070b10` (`--bg1`)
- Ink: `#f2f8fa`
- Muted: `#8a9eaa`
- Accent / wave: `#5eead4`, `#38bdf8`
- Line: white at ~10% on dark

Strategy: Committed dark ground; light is the accent field (wave imagery), not scattered chips.

## Type

- Display: Darker Grotesque (bold, tight tracking)
- Body: Karla
- Mono: IBM Plex Mono (code / meta only)

## Components

- Primary CTA: pill, ink fill → accent on hover, 2px lift
- Ghost: hairline border, accent on hover
- Theme plates: full-width light fields with name overlay (not equal icon cards)
- Header: absolute over hero; mark + wordmark; quiet nav

## Motion

- Hero wave slow drift (~32s alternate)
- Rise-in on hero copy (staggered)
- Theme plate hover lift + image scale
- Mark breathe on OLED section

## Landing composition

1. Full-bleed wave-only hero; single H1 Phostide; CTAs
2. Real taskbar pixels proof + three truths as divided list
3. Themes as stacked light plates
4. OLED section with mark
5. Download close

## Anti-patterns refused

- og-ai.jpg (baked wordmark) as hero background
- Equal theme cards with captions under images as page structure
- Kickers / eyebrows above headings
- Purple neon SaaS glow costume
