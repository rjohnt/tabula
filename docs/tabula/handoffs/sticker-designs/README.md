# Sticker design handoffs (from Grok Bot)

Drop spot for Grok Bot's work on the Tabula back-sticker designs.
Branch: `grok/sticker-designs` in https://github.com/rjohnt/tabula

## What to leave here

| Path | What |
|---|---|
| `HANDOFF.md` | Required. What you did, what you recommend, open questions. Newest entry at the top, each dated. |
| `takes/` | New or revised designs: HTML (same 384 × 588 px = 6 px/mm canvas as `../../brand/sticker-takes/`), SVG, PNG or PDF. One file per take, with a short name. |
| `sources.md` | Links for anything you looked up (printers, materials, fonts, reference images) and their licences. |

## Context to read first

- `docs/tabula/brand/STICKER.md`: the chosen design (2b, Roman inscriptional with the URL) and print notes.
- `docs/tabula/brand/sticker-takes/`: every take; `shortlist.json` lists the six kept.
- `docs/tabula/previews/index.html` and `sizing.html`: open in a browser to see all takes and the sticker on the device.
- `docs/tabula/research/branding-prices.md`: your earlier pricing research.

The device's sleep screensaver is being designed to match the sticker (same rounded double frame, Latin cross, TABULA, MISSALE · BREVIARIVM, Acutis mark). Changes to the sticker's visual language should say whether they would carry over to a 528 × 792 black-and-white e-ink screen.

## Rules

- Only add or change files in this folder. Don't edit firmware, `brand/`, or other docs.
- No purchases, accounts, checkouts, or submitted quote forms.
- Commit message: `handoff: sticker designs (Grok Bot)`. Push to `grok/sticker-designs`.
- If you can't push, reply with the full contents of `HANDOFF.md` and Ryan will paste it to Claude.
