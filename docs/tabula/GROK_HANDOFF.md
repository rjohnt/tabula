# Grok Bot handoff: Tabula back-sticker pricing

**Repo:** https://github.com/rjohnt/tabula, branch `grok/sticker-pricing`
**From:** Claude Code (working with Ryan), Oct 26, 2026

## Context
Tabula is a Catholic e-reader built on the Xteink X3 (CrossPoint firmware fork). 25–50 units will be sold at cost through a parish for charity, branded "by Acutis Ignatius". We want a printed sticker or skin on the device back.

## What's in this branch
- `docs/tabula/brand/sticker-takes/`: sticker designs (HTML at 6 px/mm of the 64 × 98 mm device back). `shortlist.json` lists the 6 designs Ryan kept. **Chosen: 2b, Roman inscriptional with `ACUTISIGNATIUS.COM/TABULA` under the attribution** (`InscriptionalURL.dc.html`); see `docs/tabula/brand/STICKER.md`. Price that design first.
- `docs/tabula/previews/sizing.html`: the sticker on the device at real proportions (proposed 58 × 86 mm, no pin cutout).
- `docs/tabula/previews/index.html`: open it in a browser to see every design (the shortlist comes first). Images are in `previews/img/`.
- `docs/tabula/device-branding.md`: device facts, material notes, recommended approach.
- `docs/tabula/research/grok-sticker-prompt.md`: **your task** (full pricing brief).

## Your task
Do the pricing research in `docs/tabula/research/grok-sticker-prompt.md`. Hard rules: no purchases, no accounts, no checkouts, no submitted quote forms.

## Returning results
- If you can push to this repo: add `docs/tabula/research/branding-prices.md` on branch `grok/sticker-pricing` and push. Commit message: `research: sticker pricing (Grok Bot)`.
- If you can't push: reply with the full markdown of that file, and Ryan will paste it back to Claude.
