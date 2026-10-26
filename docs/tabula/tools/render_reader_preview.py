"""Render 1-bit, 528x792 previews of a Tabula reading page on the Xteink X3.

Matches CrossPoint reader defaults (CrossPointSettings.h): built-in Noto Sans,
justified (18 pt preview: left-aligned), NORMAL line spacing (0.95 x line height), extra paragraph spacing,
status bar with battery, chapter title and page count.

  python docs/tabula/tools/render_reader_preview.py <out_dir>
"""

import os
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SRC = os.path.join(ROOT, "lib", "EpdFont", "builtinFonts", "source")
W, H = 528, 792
MARGIN_X, MARGIN_TOP = 16, 14
STATUS_H = 30


def font(name, pt):
    return ImageFont.truetype(os.path.join(SRC, name), round(pt * 150 / 72))


# (style, text). Introit of Christ the King, 1962 Missal; English is the
# public-domain Douay-style translation.
PAGE = [
    ("h", "Introitus"),
    ("r", "Apoc. 5:12; 1:6"),
    ("la", "Dignus est Agnus, qui occísus est, accípere virtútem, et divinitátem, et sapiéntiam, "
           "et fortitúdinem, et honórem. Ipsi glória et impérium in sǽcula sæculórum."),
    ("la", "Ps. 71:1. Deus, judícium tuum Regi da: et justítiam tuam Fílio Regis."),
    ("en", "The Lamb that was slain is worthy to receive power, and divinity, and wisdom, and "
           "strength, and honour. To Him be glory and empire for ever and ever."),
    ("en", "Ps. Give to the King Thy judgment, O God: and to the King's Son Thy justice."),
    ("r", "Glory be to the Father. The Introit is repeated."),
    ("h", "Oratio"),
    ("la", "Omnípotens sempitérne Deus, qui in dilécto Fílio tuo, universórum Rege, ómnia "
           "instauráre voluísti: concéde propítius; ut cunctæ famíliæ géntium, peccáti vúlnere "
           "disgregátæ, eius suavíssimo subdántur império."),
]


def wrap(draw, text, f, width):
    words, lines, cur = text.split(), [], []
    for w in words:
        trial = " ".join(cur + [w])
        if cur and draw.textlength(trial, font=f) > width:
            lines.append(cur)
            cur = [w]
        else:
            cur.append(w)
    if cur:
        lines.append(cur)
    return lines


def draw_justified(draw, words, f, x, y, width, last):
    if last or len(words) == 1:
        draw.text((x, y), " ".join(words), font=f, fill=0)
        return
    total = sum(draw.textlength(w, font=f) for w in words)
    gap = (width - total) / (len(words) - 1)
    cx = x
    for w in words:
        draw.text((cx, y), w, font=f, fill=0)
        cx += draw.textlength(w, font=f) + gap


def render(pt, path, justify=True):
    reg = font("NotoSans/NotoSans-Regular.ttf", pt)
    bold = font("NotoSans/NotoSans-Bold.ttf", pt)
    ital = font("NotoSans/NotoSans-Italic.ttf", pt)
    small = font("Ubuntu/Ubuntu-Regular.ttf", 8)
    img = Image.new("L", (W, H), 255)
    d = ImageDraw.Draw(img)
    asc, desc = reg.getmetrics()
    line_h = round((asc + desc) * 0.95)
    para_gap = line_h // 2
    width = W - 2 * MARGIN_X
    y = MARGIN_TOP
    bottom = H - STATUS_H - 8
    styles = {"h": bold, "la": reg, "en": ital, "r": ital}
    for style, text in PAGE:
        f = styles[style]
        lines = wrap(d, text, f, width)
        for i, words in enumerate(lines):
            if y + line_h > bottom:
                break
            if style == "h":
                d.text((MARGIN_X, y), " ".join(words), font=f, fill=0)
            else:
                draw_justified(d, words, f, MARGIN_X, y, width, not justify or i == len(lines) - 1)
            y += line_h
        y += para_gap
        if y + line_h > bottom:
            break
    # Status bar: battery, chapter title, page count (reader defaults).
    sy = H - STATUS_H + 6
    d.rectangle((MARGIN_X, sy + 3, MARGIN_X + 18, sy + 11), outline=0, width=1)
    d.rectangle((MARGIN_X + 2, sy + 5, MARGIN_X + 14, sy + 9), fill=0)
    d.text((MARGIN_X + 24, sy), "86%", font=small, fill=0)
    d.text((W / 2, sy), "CHRIST THE KING", font=small, fill=0, anchor="ma")
    d.text((W - MARGIN_X, sy), "1 / 6", font=small, fill=0, anchor="ra")
    img.point(lambda v: 0 if v < 128 else 255).convert("1").save(path)


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    render(14, os.path.join(out, "reader-14pt.png"))
    render(18, os.path.join(out, "reader-18pt.png"), justify=False)


if __name__ == "__main__":
    main()
