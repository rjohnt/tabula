"""Build docs/tabula/previews/sizing.html: sticker sizing on the X3 back, and screen text on the front.

Geometry (mm) is measured from docs/tabula/brand/reference/x3-back-amazon.png
(0.138 mm/px against the published 64 x 98 mm body); verify on a real unit
before ordering a cut die.

  python3 docs/tabula/tools/build_sizing_page.py
"""

import json
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
TAKES = os.path.join(ROOT, "docs", "tabula", "brand", "sticker-takes")
OUT = os.path.join(ROOT, "docs", "tabula", "previews", "sizing.html")
S = 6  # px per mm, same scale as the sticker takes
ASSETS = {
    "/_blob/abe730a29d5273cc5aa8ae7ef98f5b32": "img/ai-mark.png",
    "/_blob/ab518f775f96cba01eabb61e80beaf12": "img/ai-mark-light.png",
}
ROOT_SIZE = 'style="width: 384px; height: 588px;'

# Sticker options: x, y, w, h, corner radius (mm), notch (mm) or None.
OPTIONS = [
    ("A", "Above the pins (recommended)", 3, 2.5, 58, 86, 2, None,
     "58 × 86 mm. Ends 3.3 mm above the pins, so no cutout is needed. Covers the Xteink logo."),
    ("B", "Full panel with pin notch", 3, 2.5, 58, 93, 2, (26, 90.4, 12, 6),
     "58 × 93 mm with a 12 × 5 mm notch open at the bottom edge for the charger."),
    ("C", "2 × 3 in logo sticker", 6.6, 4, 50.8, 76.2, 3, None,
     "50.8 × 76.2 mm (a standard 2 × 3 in size). Cheapest; leaves a gray/black border."),
]


def mm(v):
    return round(v * S, 1)


def templates():
    keep = json.load(open(os.path.join(TAKES, "shortlist.json")))["keep"]
    canvas = json.load(open(os.path.join(TAKES, "canvas.json")))
    out, fonts = [], set()
    for name in keep:
        src = open(os.path.join(TAKES, name)).read()
        for line in src.splitlines():
            if line.startswith('<link rel="stylesheet"'):
                fonts.add(line.strip())
        body = src.split("</helmet>")[1].split("</x-dc>")[0].strip()
        for blob, local in ASSETS.items():
            body = body.replace(blob, local)
        assert ROOT_SIZE in body, name
        out.append({"id": name, "title": canvas["boards"][name]["title"], "html": body})
    return out, sorted(fonts)


def pins():
    # Four gold pins, 1.6 mm, centered; row center 5.4 mm above the bottom edge.
    cy = 98 - 5.4
    dots = "".join(
        f'<span class="pin" style="left:{mm(32 + dx - 0.8)}px;top:{mm(cy - 0.8)}px"></span>'
        for dx in (-3.35, -1.12, 1.12, 3.35)
    )
    pad = f'<span class="pinpad" style="left:{mm(32 - 4.6)}px;top:{mm(cy - 1.2)}px;width:{mm(9.2)}px;height:{mm(2.4)}px"></span>'
    return pad + dots


def back(option=None):
    parts = ['<div class="device back"><div class="panel"></div><div class="wordmark">XTEINK</div>']
    if option:
        key, _, x, y, w, h, r, notch, _ = option
        parts.append(
            f'<div class="sticker" data-w="{mm(w)}" data-h="{mm(h)}" '
            f'style="left:{mm(x)}px;top:{mm(y)}px;width:{mm(w)}px;height:{mm(h)}px;border-radius:{mm(r)}px"></div>'
        )
        if notch:
            nx, ny, nw, _ = notch
            parts.append(
                f'<div class="notch" style="left:{mm(nx)}px;top:{mm(ny)}px;width:{mm(nw)}px;height:{mm(98 - ny)}px"></div>'
            )
    parts.append(pins())
    parts.append("</div>")
    return "".join(parts)


def front(img, alt):
    # Screen: 528 x 792 px at ~259 ppi = 51.8 x 77.7 mm. Bezels approximate.
    return (
        '<div class="device front">'
        f'<div class="screen" style="left:{mm(6.1)}px;top:{mm(6)}px;width:{mm(51.8)}px;height:{mm(77.7)}px">'
        f'<img src="img/{img}" alt="{alt}"></div>'
        "</div>"
    )


def main():
    tpls, fonts = templates()
    backs = [("Bare back (reference)", back(), "64 × 98 mm body; flat panel about 60 × 95 mm; pins 4.6–6.2 mm above the bottom edge.")]
    backs += [(f"{o[0]} · {o[1]}", back(o), o[8]) for o in OPTIONS]
    fronts = [
        ("Reading, 14 pt (default)", front("reader-14pt.png", "Introit of Christ the King at 14 point")),
        ("Reading, 18 pt, left-aligned", front("reader-18pt.png", "Introit of Christ the King at 18 point")),
        ("Order of Mass", front("A-timeline.png", "Order of Mass timeline screen")),
    ]
    fig = lambda cap, el, note="": f'<figure>{el}<figcaption><b>{cap}</b>{"<br>" + note if note else ""}</figcaption></figure>'
    options = "".join(f'<option value="{i}">{t["title"]}</option>' for i, t in enumerate(tpls))
    page = f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Tabula Sizing</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
{chr(10).join(fonts)}
<style>
:root{{--bg:#f3f1ec;--ink:#1f1e1b;--muted:#5d5a52;--body:#d1d5d4;--panel:#ced4d4;--seam:#aeb4b3;--mark:#2b2e2e}}
@media (prefers-color-scheme: dark){{:root{{--bg:#1b1c1a;--ink:#ecebe6;--muted:#a8a69e}}}}
body.black{{--body:#26272a;--panel:#1d1e20;--seam:#3a3c40;--mark:#c9ccce}}
body{{margin:0;background:var(--bg);color:var(--ink);font-family:system-ui,-apple-system,sans-serif}}
main{{max-width:2200px;margin:0 auto;padding:32px 16px 64px}}
h1{{font-size:28px;margin:0 0 4px}} h2{{font-size:20px;margin:40px 0 16px}}
p.note{{color:var(--muted);margin:0 0 16px;max-width:75ch;line-height:1.45}}
.controls{{display:flex;flex-wrap:wrap;gap:16px 28px;align-items:center;margin:16px 0 8px}}
.controls label{{display:flex;gap:8px;align-items:center;font-size:15px}}
select{{font:inherit;padding:6px 8px}}
.seg button{{font:inherit;padding:6px 14px;border:1px solid var(--muted);background:transparent;color:var(--ink);cursor:pointer}}
.seg button:first-child{{border-radius:8px 0 0 8px}} .seg button:last-child{{border-radius:0 8px 8px 0;border-left:0}}
.seg button[aria-pressed=true]{{background:var(--ink);color:var(--bg)}}
.row{{display:flex;flex-wrap:wrap;gap:36px;align-items:flex-start}}
figure{{margin:0;width:{mm(64)}px}} figcaption{{color:var(--muted);font-size:14px;margin-top:12px;line-height:1.4}}
figcaption b{{color:var(--ink);font-weight:600}}
.scale{{transform-origin:top left}}
body.real .device{{zoom:{round(96 / 25.4 / S, 4)}}} body.real figure{{width:auto}}
.device{{position:relative;width:{mm(64)}px;height:{mm(98)}px;border-radius:{mm(3.3)}px;background:var(--body);box-shadow:0 3px 14px rgba(0,0,0,.22);overflow:hidden}}
.panel{{position:absolute;left:{mm(2)}px;top:{mm(1.5)}px;right:{mm(2)}px;bottom:{mm(1.5)}px;border-radius:{mm(2)}px;background:var(--panel);box-shadow:inset 0 0 0 1px var(--seam)}}
.wordmark{{position:absolute;left:0;right:0;top:{mm(22.2)}px;text-align:center;font:700 {mm(4.2)}px/1 system-ui,sans-serif;letter-spacing:.18em;color:var(--mark)}}
.sticker{{position:absolute;overflow:hidden;box-shadow:0 0 0 .5px rgba(0,0,0,.25)}}
.notch{{position:absolute;background:var(--panel);border-radius:{mm(1)}px {mm(1)}px 0 0}}
.pinpad{{position:absolute;border-radius:{mm(1.2)}px;background:#3a3b3d}}
.pin{{position:absolute;width:{mm(1.6)}px;height:{mm(1.6)}px;border-radius:50%;background:radial-gradient(circle at 35% 35%,#f6dc8c,#c79a2e 60%,#8f6a17)}}
.front .screen{{position:absolute;background:#e8e8e2;box-shadow:inset 0 0 0 1px rgba(0,0,0,.25)}}
.front .screen img{{display:block;width:100%;height:100%}}
.btn{{position:absolute;border-radius:{mm(1.6)}px;box-shadow:inset 0 0 0 1px var(--seam);background:var(--panel)}}
</style>
</head>
<body>
<main>
<h1>Tabula on the X3: sticker sizing and screen text</h1>
<p class="note">To scale at 6 px/mm. Body 64 × 98 mm (published). Back panel, pin and corner positions are measured from Xteink's product photo (±0.5 mm); confirm on a real unit before ordering a cut die. <a href="index.html">All previews</a></p>
<div class="controls">
<div class="seg" role="group" aria-label="Device color"><button type="button" data-color="mist" aria-pressed="true">Mist Gray</button><button type="button" data-color="black" aria-pressed="false">Black</button></div>
<label>Design <select id="design">{options}</select></label>
<label><input type="checkbox" id="real"> Approximate real size</label>
</div>
<h2>Back</h2>
<div class="row">{"".join(fig(c, e, n) for c, e, n in backs)}</div>
<h2>Front: text on the screen</h2>
<p class="note">1-bit renders at the panel's 528 × 792, using the firmware's built-in Noto Sans and default reader settings, shown at the screen's physical size (about 51.8 × 77.7 mm). Bezels are approximate. The four control buttons sit on the bottom edge (not the face), with two side buttons and Power.</p>
<div class="row">{"".join(fig(c, e) for c, e in fronts)}</div>
</main>
<script>
const TPLS = {json.dumps(tpls)};
const ROOT_SIZE = {json.dumps(ROOT_SIZE)};
function paint(i) {{
  document.querySelectorAll('.sticker').forEach(el => {{
    const w = el.dataset.w, h = el.dataset.h;
    el.innerHTML = TPLS[i].html.replace(ROOT_SIZE, `style="width: ${{w}}px; height: ${{h}}px;`);
  }});
  try {{ localStorage.setItem('tabula-design', i); }} catch (e) {{}}
}}
const sel = document.getElementById('design');
sel.addEventListener('change', () => paint(sel.value));
document.querySelectorAll('[data-color]').forEach(b => b.addEventListener('click', () => {{
  document.body.classList.toggle('black', b.dataset.color === 'black');
  document.querySelectorAll('[data-color]').forEach(o => o.setAttribute('aria-pressed', o === b));
}}));
document.getElementById('real').addEventListener('change', e => document.body.classList.toggle('real', e.target.checked));
let start = 0;
try {{ start = Number(localStorage.getItem('tabula-design')) || 0; }} catch (e) {{}}
if (!TPLS[start]) start = 0;
sel.value = start;
paint(start);
</script>
</body>
</html>
"""
    open(OUT, "w").write(page)
    print("wrote", OUT)


if __name__ == "__main__":
    main()
