"""Build docs/tabula/previews/index.html from the sticker-take sources and Order of Mass renders.

  python3 docs/tabula/tools/build_previews_site.py
  python3 -m http.server 8766 --bind 127.0.0.1 --directory docs/tabula/previews
"""

import html
import json
import os
import re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
TAKES = os.path.join(ROOT, "docs", "tabula", "brand", "sticker-takes")
OUT = os.path.join(ROOT, "docs", "tabula", "previews", "index.html")
# Canvas asset ids -> files served beside index.html.
ASSETS = {
    "/_blob/abe730a29d5273cc5aa8ae7ef98f5b32": "img/ai-mark.png",
    "/_blob/ab518f775f96cba01eabb61e80beaf12": "img/ai-mark-light.png",
}
ORDO = [
    ("A-timeline", "A · Timeline (built into firmware)"),
    ("B-two-lanes", "B · Two lanes"),
    ("C-follow-along", "C · Follow along"),
    ("D-timeline-serif", "D · Timeline, Noto Serif (needs +flash)"),
    ("E-hints-two", "Controls: two large labels + side-button cue"),
    ("F-hints-none", "Controls: no boxes, one instruction line"),
]


def take_cards():
    canvas = json.load(open(os.path.join(TAKES, "canvas.json")))
    meta = json.load(open(os.path.join(TAKES, "shortlist.json")))
    keep, chosen = meta["keep"], meta.get("chosen")
    variants = meta.get("chosen_variants", [])
    fonts, rows, shortlist, picked = set(), {}, [], []
    for name in canvas["order"]:
        src = open(os.path.join(TAKES, name)).read()
        fonts.update(re.findall(r'<link rel="stylesheet" href="([^"]+)">', src))
        body = src.split("</helmet>")[1].split("</x-dc>")[0].strip()
        for blob, local in ASSETS.items():
            body = body.replace(blob, local)
        board = canvas["boards"][name]
        card = f'<figure class="sticker"><div class="skin">{body}</div><figcaption>{html.escape(board["title"])}</figcaption></figure>'
        rows.setdefault(board["y"], []).append((board["x"], card))
        if name in keep:
            shortlist.append((keep.index(name), card))
        if name == chosen:
            picked.insert(0, card)
        elif name in variants:
            picked.append(card)
    ordered = [[c for _, c in sorted(cards)] for _, cards in sorted(rows.items())]
    return fonts, ordered, [c for _, c in sorted(shortlist)], picked


def main():
    fonts, rows, shortlist, picked = take_cards()
    links = "\n".join(f'<link rel="stylesheet" href="{u}">' for u in sorted(fonts))
    ordo = "".join(
        f'<figure class="screen"><img src="img/{n}.png" width="528" height="792" alt="{html.escape(t)}">'
        f"<figcaption>{html.escape(t)}</figcaption></figure>"
        for n, t in ORDO
    )
    row_titles = ["Back sticker takes, round 1", "Round 2: more like 1 and 2"]
    # Newest round first.
    takes = (f'<h2>Chosen</h2><div class="row">{"".join(picked)}</div>' if picked else "") + f'<h2>Shortlist</h2><div class="row">{"".join(shortlist)}</div>' + "".join(
        f'<h2>{row_titles[i] if i < len(row_titles) else f"Round {i + 1}"}</h2><div class="row">{"".join(r)}</div>'
        for i, r in reversed(list(enumerate(rows)))
    )
    page = f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Tabula Previews</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
{links}
<style>
:root{{--bg:#f3f1ec;--ink:#1f1e1b;--muted:#5d5a52}}
@media (prefers-color-scheme: dark){{:root{{--bg:#1b1c1a;--ink:#ecebe6;--muted:#a8a69e}}}}
body{{margin:0;background:var(--bg);color:var(--ink);font-family:system-ui,-apple-system,sans-serif}}
main{{max-width:2800px;margin:0 auto;padding:32px 16px 64px}}
h1{{font-size:28px;margin:0 0 4px}} h2{{font-size:20px;margin:48px 0 16px}}
p.note{{color:var(--muted);margin:0 0 20px;max-width:70ch;line-height:1.45}}
.row{{display:flex;flex-wrap:wrap;gap:32px;align-items:flex-start}}
figure{{margin:0}} figcaption{{color:var(--muted);font-size:14px;margin-top:10px}}
.screen img{{display:block;width:528px;max-width:100%;height:auto;image-rendering:pixelated;background:#fff;border:10px solid #d9d6cf;border-radius:14px}}
.skin{{width:384px;height:588px;border-radius:36px;overflow:hidden;box-shadow:0 2px 10px rgba(0,0,0,.18);transform-origin:top left}}
@media (max-width:440px){{.skin{{transform:scale(.8);margin-bottom:-118px}}}}
</style>
</head>
<body>
<main>
<h1>Tabula previews</h1>
<p class="note">Drafts. Sticker takes are the X3 back (64 × 98 mm) at 6 px/mm; see <a href="sizing.html">sizing on the device</a> for placement, device colors, and screen text. Order of Mass screens are 1-bit renders at the X3's native 528 × 792 with the firmware's built-in fonts.</p>
{takes}
<h2>Order of Mass on the X3</h2>
<p class="note">Filled dot = today's proper. Hollow dot = the Ordinary. Posture at right.</p>
<div class="row">{ordo}</div>
</main>
</body>
</html>
"""
    open(OUT, "w").write(page)
    print("wrote", OUT)


if __name__ == "__main__":
    main()
