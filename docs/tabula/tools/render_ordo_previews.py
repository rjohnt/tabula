"""Render 1-bit, 528x792 previews of Order of Mass treatments for the Xteink X3.

Uses the firmware's own TTF sources at the sizes fontconvert.py bakes them
(size_pt at 150 DPI), thresholded to 1-bit like the built-in fonts, and the
X3 button-hint geometry from BaseTheme::drawButtonHints.

  python docs/tabula/tools/render_ordo_previews.py <out_dir>
"""

import json
import os
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SRC = os.path.join(ROOT, "lib", "EpdFont", "builtinFonts", "source")
ORDO = os.path.join(ROOT, "docs", "tabula", "sd", "Tabula", "Missal", "ordo.json")
W, H = 528, 792
HINT_X = [38, 154, 268, 384]
HINT_W, HINT_H = 106, 40


def font(family, style, pt):
    names = {
        "ubuntu": {"regular": "Ubuntu/Ubuntu-Regular.ttf", "bold": "Ubuntu/Ubuntu-Bold.ttf"},
        "sans": {
            "regular": "NotoSans/NotoSans-Regular.ttf",
            "bold": "NotoSans/NotoSans-Bold.ttf",
            "italic": "NotoSans/NotoSans-Italic.ttf",
        },
        "serif": {
            "regular": "NotoSerif/NotoSerif-Regular.ttf",
            "bold": "NotoSerif/NotoSerif-Bold.ttf",
            "italic": "NotoSerif/NotoSerif-Italic.ttf",
        },
    }
    return ImageFont.truetype(os.path.join(SRC, names[family][style]), round(pt * 150 / 72))


UI8 = font("ubuntu", "regular", 8)
UI10 = font("ubuntu", "regular", 10)
UI10B = font("ubuntu", "bold", 10)
UI12B = font("ubuntu", "bold", 12)


class Screen:
    def __init__(self):
        self.img = Image.new("L", (W, H), 255)
        self.d = ImageDraw.Draw(self.img)

    def text(self, xy, s, f, fill=0, anchor="la"):
        self.d.text(xy, s, font=f, fill=fill, anchor=anchor)

    def width(self, s, f):
        return self.d.textlength(s, font=f)

    def header(self, title, right=None):
        self.text((20, 14), "100%", UI8)
        self.text((W - 20, 14), "10:42", UI8, anchor="ra")
        self.text((20, 40), title, UI12B)
        if right:
            self.text((W - 20, 46), right, UI8, anchor="ra")
        self.d.line((20, 80, W - 20, 80), fill=0, width=1)

    def hints(self, labels):
        for x, label in zip(HINT_X, labels):
            if not label:
                continue
            y = H - HINT_H
            self.d.rectangle((x, y, x + HINT_W - 1, y + HINT_H - 1), outline=0, width=1)
            self.text((x + HINT_W / 2, y + HINT_H / 2), label, UI10, anchor="mm")

    def dot(self, cx, cy, r, filled, fill=0, bg=255):
        self.d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=fill if filled else bg, outline=fill, width=2)

    def save(self, path):
        # Same 1-bit decision the built-in fonts get at conversion time.
        self.img.point(lambda v: 0 if v < 128 else 255).convert("1").save(path)


def load_steps():
    with open(ORDO) as f:
        return json.load(f)["steps"]


TODAY = "Christ the King"


def legend(s, y):
    s.dot(26, y + 8, 6, True)
    s.text((38, y), "today's proper", UI8)
    s.dot(176, y + 8, 6, False)
    s.text((188, y), "ordinary", UI8)
    s.text((W - 20, y), "page 1 of 2", UI8, anchor="ra")


def timeline(steps, fonts, path, selected="collect", start=0):
    """A: vertical rail, filled dot = proper, hollow = ordinary, posture at right."""
    la_f, en_f, sec_f, pos_f = fonts
    s = Screen()
    s.header("Order of Mass", TODAY)
    rail_x, y = 44, 96
    bottom = H - HINT_H - 40
    s.d.line((rail_x, y, rail_x, bottom - 8), fill=0, width=2)
    for st in steps[start:]:
        if y + (32 if "section" in st else 58) > bottom:
            break
        if "section" in st:
            s.d.rectangle((rail_x - 6, y + 6, rail_x + 6, y + 18), fill=0)
            s.text((rail_x + 20, y + 4), st["section"].upper(), sec_f)
            y += 32
            continue
        row_h = 58
        sel = st["id"] == selected
        if sel:
            s.d.rounded_rectangle((rail_x + 14, y - 4, W - 14, y + row_h - 4), radius=6, fill=0)
        fg = 255 if sel else 0
        s.dot(rail_x, y + 14, 8, st["kind"] == "proper")
        s.text((rail_x + 24, y + 2), st["la"], la_f, fill=fg)
        s.text((rail_x + 24, y + 26), st["en"], en_f, fill=fg)
        s.text((W - 26, y + 6), st["posture"].upper(), pos_f, fill=fg, anchor="ra")
        y += row_h
    legend(s, H - HINT_H - 30)
    s.hints(["« Back", "Open", "Up", "Down"])
    s.save(path)


def two_lanes(steps, path, selected="collect"):
    """B: Ordinary in the left lane, propers in the right lane, read top to bottom."""
    s = Screen()
    s.header("Order of Mass", TODAY)
    mid = W // 2
    s.text((mid - 12, 92), "ORDINARY", UI8, anchor="ra")
    s.text((mid + 12, 92), "TODAY'S PROPER", UI8)
    s.d.line((mid, 112, mid, H - HINT_H - 16), fill=0, width=1)
    y = 118
    for st in steps[:13]:
        if "section" in st:
            s.d.line((20, y + 10, W - 20, y + 10), fill=0, width=1)
            label = st["section"]
            tw = s.width(label, UI8)
            s.d.rectangle((mid - tw / 2 - 8, y, mid + tw / 2 + 8, y + 20), fill=255)
            s.text((mid, y + 10), label, UI8, anchor="mm")
            y += 30
            continue
        sel = st["id"] == selected
        proper = st["kind"] == "proper"
        box = (mid + 8, y, W - 16, y + 36) if proper else (16, y, mid - 8, y + 36)
        if sel:
            s.d.rounded_rectangle(box, radius=6, fill=0)
        elif proper:
            s.d.rounded_rectangle(box, radius=6, outline=0, width=2)
        fg = 255 if sel else 0
        s.dot(mid, y + 18, 5, proper)
        if proper:
            s.text((mid + 18, y + 18), st["la"], UI10B, fill=fg, anchor="lm")
        else:
            s.text((mid - 18, y + 18), st["la"], UI10, fill=fg, anchor="rm")
        y += 44
    s.hints(["« Back", "Open", "Up", "Down"])
    s.save(path)


def follow_along(steps, path, current="gospel"):
    """C: one step at a time for following along at Mass; progress ticks on top."""
    s = Screen()
    s.header("Order of Mass", TODAY)
    real = [st for st in steps if "section" not in st]
    idx = next(i for i, st in enumerate(real) if st["id"] == current)
    tick_w = (W - 40) / len(real)
    for i, st in enumerate(real):
        x0 = 20 + i * tick_w
        h = 22 if st["kind"] == "proper" else 12
        if i <= idx:
            s.d.rectangle((x0 + 2, 120 - h, x0 + tick_w - 3, 120), fill=0)
        else:
            s.d.rectangle((x0 + 2, 120 - h, x0 + tick_w - 3, 120), outline=0, width=1)
    s.text((20, 132), f"Step {idx + 1} of {len(real)}  ·  Mass of the Catechumens", UI8)

    st = real[idx]
    s.text((W / 2, 220), "TODAY'S PROPER" if st["kind"] == "proper" else "ORDINARY", UI10B, anchor="mm")
    s.text((W / 2, 290), st["la"], font("sans", "bold", 18), anchor="mm")
    s.text((W / 2, 340), st["en"], font("sans", "italic", 14), anchor="mm")
    s.d.rounded_rectangle((W / 2 - 90, 392, W / 2 + 90, 440), radius=24, fill=0)
    s.text((W / 2, 416), st["posture"].upper(), UI12B, fill=255, anchor="mm")

    prev_st, next_st = real[idx - 1], real[idx + 1]
    s.d.line((20, 520, W - 20, 520), fill=0, width=1)
    s.text((20, 540), "BEFORE", UI8)
    s.text((20, 562), prev_st["la"], UI10)
    s.text((W - 20, 540), "NEXT", UI8, anchor="ra")
    s.text((W - 20, 562), next_st["la"], UI10B, anchor="ra")
    s.text((W / 2, 660), "Open reads the text. Up and Down step through the Mass.", UI8, anchor="mm")
    s.hints(["« Back", "Open", "Up", "Down"])
    s.save(path)


def hints_two(s):
    """Two large labels on the outer buttons; side rocker cue on the right edge."""
    y = H - HINT_H - 6
    for x, label in ((HINT_X[0], "\u00ab Back"), (HINT_X[1], "Open")):
        s.d.rounded_rectangle((x, y, x + HINT_W - 1, H - 1), radius=6, fill=0)
        s.text((x + HINT_W / 2, y + (H - y) / 2), label, UI12B, fill=255, anchor="mm")
    s.text((W - 20, y + 4), "side buttons", UI8, anchor="ra")
    s.text((W - 20, y + 22), "move up / down", UI8, anchor="ra")


def hints_none(s):
    """No hint boxes; one plain instruction line."""
    s.text((W / 2, H - 26), "Side buttons move  \u00b7  Open reads  \u00b7  Back returns", UI10, anchor="mm")


def timeline_variant(steps, path, hint_fn):
    timeline(steps, (font("ubuntu", "bold", 12), UI10, UI8, UI8), path)
    img = Image.open(path).convert("L")
    s = Screen()
    s.img.paste(img)
    s.d = ImageDraw.Draw(s.img)
    s.d.rectangle((0, H - HINT_H - 8, W, H), fill=255)
    hint_fn(s)
    s.save(path)


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    steps = load_steps()
    timeline(
        steps,
        (font("ubuntu", "bold", 12), UI10, UI8, UI8),
        os.path.join(out, "A-timeline.png"),
    )
    two_lanes(steps, os.path.join(out, "B-two-lanes.png"))
    follow_along(steps, os.path.join(out, "C-follow-along.png"))
    timeline(
        steps,
        (font("serif", "bold", 12), font("serif", "regular", 10), UI8, UI8),
        os.path.join(out, "D-timeline-serif.png"),
    )
    timeline_variant(steps, os.path.join(out, "E-hints-two.png"), hints_two)
    timeline_variant(steps, os.path.join(out, "F-hints-none.png"), hints_none)


if __name__ == "__main__":
    main()
