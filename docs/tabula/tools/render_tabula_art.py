#!/usr/bin/env python3
"""Draw Tabula's per-day artwork as 1-bit BMPs for the X3 (528 x 792).

  Tabula/Today/YYYY-MM-DD.head.bmp   Today screen header (528 x 300): rubric bar,
                                     lancet with trefoil, date, feast.
  Tabula/Sleep/YYYY-MM-DD.bmp        Sleep screensaver, Inscriptional (the chosen back
                                     sticker on paper). The band below ARC_TOP is left
                                     for the Hour arc the firmware draws; the firmware
                                     inverts the screen for the dark setting.

Design source: docs/tabula/previews/screens.html (Direction section). Nothing may
touch anything else, so text is fitted (smaller size, more lines) rather than
allowed to run into an ornament.

Needs Pillow and the fonts in build/host-fonts (Cinzel-Regular/Bold, EBGaramond-
Regular/Italic). Usage:
  .venv/bin/python docs/tabula/tools/render_tabula_art.py --mass mass.json [--out build/tabula-sd] [--only 2026-10-28]
"""

import argparse
import datetime as dt
import json
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

REPO = Path(__file__).resolve().parents[3]
FONTS = REPO / "build/host-fonts"
MARK = REPO / "docs/tabula/previews/img/ai-mark-light.png"
W, H = 528, 792
HEAD_H = 300
S = 3  # supersampling factor; drawn large, reduced, then thresholded

FERIA_LA = ["FERIA II", "FERIA III", "FERIA IV", "FERIA V", "FERIA VI", "SABBATO", "DOMINICA"]


def date_short(d):
    return f"{d:%b} {d.day}, {d.year}"


def font(name, px):
    return ImageFont.truetype(str(FONTS / f"{name}.ttf"), int(round(px * S)))


class Canvas:
    """Drawing in device pixels on a supersampled grayscale image."""

    def __init__(self, w, h, bg):
        self.w, self.h = w, h
        self.img = Image.new("L", (w * S, h * S), bg)
        self.d = ImageDraw.Draw(self.img)

    def p(self, *v):
        return [x * S for x in v]

    def line(self, x0, y0, x1, y1, ink, width=1):
        self.d.line(self.p(x0, y0, x1, y1), fill=ink, width=max(1, int(width * S)))

    def rect(self, x0, y0, x1, y1, ink):
        self.d.rectangle(self.p(x0, y0, x1, y1), fill=ink)

    def rrect(self, x0, y0, x1, y1, r, ink, width):
        self.d.rounded_rectangle(self.p(x0, y0, x1, y1), radius=r * S, outline=ink, width=int(width * S))

    def circle(self, cx, cy, r, ink, width):
        self.d.ellipse(self.p(cx - r, cy - r, cx + r, cy + r), outline=ink, width=max(1, int(width * S)))

    def poly(self, pts, ink, width=None, fill=None):
        flat = [c * S for xy in pts for c in xy]
        if fill is not None:
            self.d.polygon(flat, fill=fill)
        if width:
            self.d.line(flat + flat[:2], fill=ink, width=int(width * S), joint="curve")

    def text_w(self, text, f, tracking=0.0):
        if not tracking:
            return self.d.textlength(text, font=f) / S
        return (sum(self.d.textlength(c, font=f) for c in text) + tracking * f.size * (len(text) - 1)) / S

    def text_c(self, y, text, f, ink, tracking=0.0, cx=None):
        """Centered text whose glyph tops sit at y. Returns the bottom of the ink."""
        cx = self.w / 2 if cx is None else cx
        x = cx - self.text_w(text, f, tracking) / 2
        top = f.getbbox("ÁHg")[1] / S
        if not tracking:
            self.d.text((x * S, (y - top) * S), text, font=f, fill=ink)
        else:
            for c in text:
                self.d.text((x * S, (y - top) * S), c, font=f, fill=ink)
                x += (self.d.textlength(c, font=f) + tracking * f.size) / S
        return y + (f.getbbox("ÁHg")[3] - f.getbbox("ÁHg")[1]) / S

    def bitmap(self, light_text_bands=()):
        small = self.img.resize((self.w, self.h), Image.LANCZOS)
        # A dark-leaning threshold keeps thin capitals whole on paper; bands
        # carrying white text on black get the opposite so it stays whole too.
        out = small.point(lambda v: 255 if v >= 168 else 0)
        for y0, y1 in light_text_bands:
            band = small.crop((0, y0, self.w, y1)).point(lambda v: 255 if v >= 72 else 0)
            out.paste(band, (0, y0))
        return out.convert("1", dither=Image.Dither.NONE)


def arc_points(x0, x1, spring, apex_y=None, steps=48):
    """Points along a semicircular head from (x0, spring) over to (x1, spring)."""
    cx, r = (x0 + x1) / 2, (x1 - x0) / 2
    return [(cx - r * math.cos(math.pi * i / steps), spring - r * math.sin(math.pi * i / steps)) for i in range(steps + 1)]


def lancet_points(x0, x1, yb, ytop, steps=24):
    """An equilateral pointed arch: two arcs of radius (x1-x0) meeting at the apex."""
    w = x1 - x0
    spring = ytop + w * 0.866
    left = [(x1 - w * math.cos(a), spring - w * math.sin(a)) for a in
            [i * (math.pi / 3) / steps for i in range(steps + 1)]]
    right = [(x0 + w * math.cos(a), spring - w * math.sin(a)) for a in
             [math.pi / 3 - i * (math.pi / 3) / steps for i in range(steps + 1)]]
    return [(x0, yb)] + left + right[1:] + [(x1, yb)]


def quatrefoil(c, cx, cy, r, ink, width=1.4):
    o = r * 0.5
    for dx, dy in [(0, -o), (o, 0), (0, o), (-o, 0)]:
        c.circle(cx + dx, cy + dy, r * 0.52, ink, width)


def trefoil(c, cx, cy, r, ink):
    for i in range(3):
        a = -math.pi / 2 + i * 2 * math.pi / 3
        c.circle(cx + r * 0.45 * math.cos(a), cy + r * 0.45 * math.sin(a), r * 0.45, ink, 1.4)


def latin_cross(c, cx, top, h, ink):
    t = h * 0.12
    c.rect(cx - t / 2, top, cx + t / 2, top + h, ink)
    c.rect(cx - h * 0.3, top + h * 0.26, cx + h * 0.3, top + h * 0.26 + t, ink)


def wrap(c, text, f, max_w):
    words, lines, cur = text.split(), [], ""
    for wd in words:
        trial = (cur + " " + wd).strip()
        if c.text_w(trial, f) <= max_w or not cur:
            cur = trial
        else:
            lines.append(cur)
            cur = wd
    if cur:
        lines.append(cur)
    return lines


def fit_lines(c, text, sizes, max_w, max_h, leading=1.12, max_lines=3):
    """Largest size whose wrapped lines fit both width and height."""
    for px in sizes:
        f = font("EBGaramond-Regular", px)
        lines = wrap(c, text, f, max_w)
        if len(lines) <= max_lines and all(c.text_w(l, f) <= max_w for l in lines) and len(lines) * px * leading <= max_h:
            return f, px, lines
    f = font("EBGaramond-Regular", sizes[-1])
    return f, sizes[-1], wrap(c, text, f, max_w)[:max_lines]


def feast_parts(title):
    name, _, rest = title.partition(" ~ ")
    cls = rest.split(" · ")[0].strip()
    return name.strip(), cls


def rubric_bar(c, text):
    """H3: a black band with the day in white capitals and a cross at each end."""
    c.rect(0, 0, W, 38, 0)
    for cx in (26, W - 26):
        latin_cross(c, cx, 10, 18, 255)
    for px in (14, 13, 12, 11):
        f = font("Cinzel-Bold", px)
        if c.text_w(text, f, tracking=0.2) <= W - 2 * 46:
            break
    top = 19 - (f.getbbox("H")[3] - f.getbbox("H")[1]) / S / 2
    c.text_c(top, text, f, 255, tracking=0.2)


def draw_header(d, title):
    ink, paper = 0, 255
    c = Canvas(W, HEAD_H, paper)
    rubric_bar(c, "HODIE · " + FERIA_LA[d.weekday()])
    c.poly(lancet_points(210, 318, 178, 58), ink, width=2.4)
    c.poly(lancet_points(220, 308, 178, 72), ink, width=1)
    trefoil(c, 264, 128, 22, ink)
    c.rect(196, 177, 332, 179, ink)
    y = c.text_c(192, date_short(d), font("Cinzel-Bold", 13), ink, tracking=0.16)
    name, _ = feast_parts(title)
    f, px, lines = fit_lines(c, name, [29, 26, 23, 21], W - 60, HEAD_H - 6 - (y + 10))
    y += 10
    for line in lines:
        c.text_c(y, line, f, ink)
        y += px * 1.12
    assert y <= HEAD_H, (d, name)
    return c.bitmap(light_text_bands=[(0, 38)])


ARC_TOP = 552  # the firmware's Hour arc (cy 600, ry 30, sun r 13) starts below this


def draw_sleep(d, title):
    """Inscriptional: the chosen back sticker on paper. Dark mode is the
    firmware inverting this whole screen, so no light/dark fields are mixed."""
    ink, paper = 0, 255
    c = Canvas(W, H, paper)
    c.rrect(14, 14, W - 14, H - 14, 30, ink, 2.5)
    c.rrect(23, 23, W - 23, H - 23, 22, ink, 1.2)
    latin_cross(c, 264, 92, 68, ink)
    y = c.text_c(196, "TABULA", font("Cinzel-Bold", 62), ink, tracking=0.09)
    c.rect(204, y + 22, 324, y + 24, ink)
    y = c.text_c(y + 44, "MISSALE · BREVIARIVM", font("Cinzel-Bold", 16), ink, tracking=0.22)
    name, _ = feast_parts(title)
    f, px, lines = fit_lines(c, name, [28, 26, 24, 22], 400, ARC_TOP - 40 - (y + 36))
    y += 36
    for line in lines:
        c.text_c(y, line, f, ink)
        y += px * 1.12
    y = c.text_c(y + 12, date_short(d), font("Cinzel-Bold", 13), ink, tracking=0.2)
    assert y <= ARC_TOP - 8, (d, name, y)
    if MARK.exists():
        mark = Image.open(MARK).convert("RGBA")
        mw = 38 * S
        mark = mark.resize((mw, int(mark.height * mw / mark.width)), Image.LANCZOS)
        tag = font("EBGaramond-Italic", 19)
        tw = c.text_w("Acutis Ignatius", tag)
        x0 = (W - (38 + 8 + tw)) / 2
        my = 712
        # The light mark is ivory on transparent; print it as ink on paper.
        c.img.paste(Image.new("L", mark.size, ink), (int(x0 * S), int((my - mark.height / S / 2) * S)), mark.split()[3])
        c.d.text(((x0 + 46) * S, (my - 13) * S), "Acutis Ignatius", font=tag, fill=ink)
    return c.bitmap()


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--mass", type=Path, required=True)
    p.add_argument("--out", type=Path, default=REPO / "build/tabula-sd")
    p.add_argument("--only", default=None)
    a = p.parse_args()
    days = json.loads(a.mass.read_text())["days"]
    (a.out / "Tabula/Today").mkdir(parents=True, exist_ok=True)
    (a.out / "Tabula/Sleep").mkdir(parents=True, exist_ok=True)
    n = 0
    for ds, day in days.items():
        if a.only and ds != a.only:
            continue
        d = dt.date.fromisoformat(ds)
        draw_header(d, day["title"]).save(a.out / f"Tabula/Today/{ds}.head.bmp")
        draw_sleep(d, day["title"]).save(a.out / f"Tabula/Sleep/{ds}.bmp")
        n += 1
    print(f"{n} days of artwork -> {a.out}/Tabula/{{Today,Sleep}}")


if __name__ == "__main__":
    main()
