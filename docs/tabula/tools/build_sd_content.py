#!/usr/bin/env python3
"""Build the Tabula SD card content (1962 Missal + Roman Breviary) as EPUBs.

Inputs are the Divinum Officium packs made by backmuse's tools/office:
  build_mass.py  --output mass.json     (Rubrics 1960 - 1960, 1962 Missal)
  build_daily.py --tradition roman1962 --output office.json

Output (under --out, default build/tabula-sd):
  Tabula/Today/YYYY-MM-DD.propers.epub   the day's propers, one chapter per ordo.json step id
  Tabula/Today/YYYY-MM-DD.full.epub      the whole Mass in order, same chapter ids
  Tabula/Missal/Ordo-Missae.epub         the Ordinary, same chapter ids
  Tabula/Missal/ordo.json                copied from docs/tabula/sd
  Tabula/Hours/YYYY-MM-DD.hours.epub     the eight Hours, chapters matins..compline
  LICENSES.txt                           Divinum Officium credit (MIT)

Glyphs the device fonts lack are replaced: ℣ ℟ -> bold V. R., ✠ -> †.
"""

import argparse
import datetime as dt
import html
import json
import re
import shutil
import zipfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
ORDO = REPO / "docs/tabula/sd/Tabula/Missal/ordo.json"

# Divinum Officium section heading -> ordo.json step id.
HEADING_ID = {
    "Incipit": "foot", "Introitus": "introit", "Kyrie": "kyrie", "Gloria": "gloria",
    "Oratio": "collect", "Lectio": "epistle", "Epistola": "epistle", "Graduale": "gradual",
    "Evangelium": "gospel", "Credo": "credo", "Offertorium": "offertory", "Secreta": "secret",
    "Præfatio": "preface", "Canon": "canon", "Preparatio Communionis": "pater",
    "Communio": "commant", "Postcommunio": "postcomm", "Conclusio": "dismissal",
    "Orationes Leonis XIII": "leonine", "Allelúia.": "gradual",
}
# A block starting a new Ordinary step inside a DO section: (section id, text, new id).
INNER_SPLITS = [
    ("offertory", "Lavábo inter innocéntes", "lavabo"),
    ("preface", "Sanctus, Sanctus, Sanctus", "sanctus"),
    ("pater", "Agnus Dei, qui tollis", "agnus"),
    ("agnus", "Dómine, non sum dignus", "communion"),
    ("dismissal", "sancti Evangélii secúndum Ioánnem", "lastgospel"),
]
PROPER_IDS = ["introit", "collect", "epistle", "gradual", "gospel", "offertory", "secret", "preface",
              "commant", "postcomm"]
# Short cue lines between propers, pointing to the Ordinary.
CUES = {
    "introit": "Kyrie · Gloria — see the Order of Mass",
    "gospel": "Credo — see the Order of Mass",
    "offertory": "Lavabo · Orate fratres — see the Order of Mass",
    "preface": "Sanctus · Canon · Pater noster · Agnus Dei · Communion — see the Order of Mass",
    "postcomm": "Ite, missa est · Blessing · Last Gospel — see the Order of Mass",
}
HOURS = [("matins", "Matins", "Ad Matutinum"), ("lauds", "Lauds", "Ad Laudes"), ("prime", "Prime", "Ad Primam"),
         ("terce", "Terce", "Ad Tertiam"), ("sext", "Sext", "Ad Sextam"), ("none", "None", "Ad Nonam"),
         ("vespers", "Vespers", "Ad Vesperas"), ("compline", "Compline", "Ad Completorium")]
FERIA_LA = ["Feria II", "Feria III", "Feria IV", "Feria V", "Feria VI", "Sabbato", "Dominica"]

CSS = """body{margin:0 0.4em}
h1{text-align:center;font-size:1.4em;margin:1.2em 0 0.3em}
h2{text-align:center;font-size:1.15em;margin:0.6em 0 0.5em}
p{margin:0.35em 0;text-indent:0}
p.pair{margin:0.45em 0}
p.en1{margin:0.35em 0}
p.ref{text-align:center;font-size:0.85em;margin:0.6em 0 0.2em}
p.cue{text-align:center;font-style:italic;margin:1.2em 0}
p.c{text-align:center}
"""



# Latin/English pairing, from backmuse tools/office/build_firmware_pack.py:
# lines and psalm pause marks divide a prayer where both languages have the
# same number; inside those, clauses are matched in order by length; short
# neighbours are joined again while the Latin still fits a line.
CERTAIN = (r"\n", r"(?<=[*\u2020\u2021])\s+")
CLAUSE = r"(?<=[,.;:?!])\s+|\n"
STRONG = ".?!"
LINE = 32  # Latin letters and spaces on one line at the default size
SPAN = 4


def _letters(text):
    return sum(ch.isalpha() for ch in text) or 1


def _clauses(text):
    out, lead = [], False
    for part in (piece.strip() for piece in re.split(CLAUSE, text)):
        if not part:
            continue
        count = sum(ch.isalpha() for ch in part)
        if out and (lead or count == 0):
            out[-1] += " " + part
        else:
            out.append(part)
        lead = " " not in out[-1] and 0 < sum(ch.isalpha() for ch in out[-1]) <= 3 or \
            (count == 0 and len(out) == 1 and out[-1] == part)
    return out


def _match(latin, english):
    if len(latin) < 2 or len(english) < 2:
        return [(" ".join(latin), " ".join(english))]
    ratio = sum(map(_letters, english)) / sum(map(_letters, latin))
    a = [0]
    for part in latin:
        a.append(a[-1] + _letters(part))
    b = [0]
    for part in english:
        b.append(b[-1] + _letters(part))
    best = {(0, 0): (0.0, None)}
    for i in range(len(latin) + 1):
        for j in range(len(english) + 1):
            if (i, j) not in best:
                continue
            for di in range(1, SPAN + 1):
                for dj in range(1, SPAN + 1):
                    if i + di > len(latin) or j + dj > len(english) or (di > 1 and dj > 1 and di + dj > 4):
                        continue
                    la, en = a[i + di] - a[i], b[j + dj] - b[j]
                    cost = (en - la * ratio) ** 2 / (la * ratio + en) + 2.0 * (di + dj - 2)
                    ends = (latin[i + di - 1][-1] in STRONG, english[j + dj - 1][-1] in STRONG)
                    cost += 1.5 if ends[0] != ends[1] else 0.0
                    total = best[(i, j)][0] + cost
                    key = (i + di, j + dj)
                    if key not in best or total < best[key][0]:
                        best[key] = (total, (i, j))
    key, pairs = (len(latin), len(english)), []
    if key not in best:
        return [(" ".join(latin), " ".join(english))]
    while best[key][1] is not None:
        prev = best[key][1]
        pairs.append((" ".join(latin[prev[0]:key[0]]), " ".join(english[prev[1]:key[1]])))
        key = prev
    return pairs[::-1]


def _join_short(pairs):
    out = []
    for la, en in pairs:
        if out and len(out[-1][0]) + 1 + len(la) <= LINE and out[-1][0][-1] not in STRONG:
            out[-1] = (out[-1][0] + " " + la, out[-1][1] + " " + en)
        else:
            out.append((la, en))
    return out


def phrases(latin, english):
    parts = [(latin, english)]
    for pattern in CERTAIN:
        finer = []
        for la, en in parts:
            x = [part.strip() for part in re.split(pattern, la) if part.strip()]
            y = [part.strip() for part in re.split(pattern, en) if part.strip()]
            finer += zip(x, y) if len(x) == len(y) > 1 else [(la, en)]
        parts = finer
    out = []
    for la, en in parts:
        out += _join_short(_match(_clauses(la), _clauses(en)))
    return out


# Edition being written: "" Latin & English line by line, ".la" Latin, ".en" English.
EDITION = ""
EDITIONS = ("", ".la", ".en")

def date_short(d):
    return f"{d:%b} {d.day}, {d.year}"


def book_title(kind, d, feast):
    return f"{kind} · {date_short(d)} · {feast.replace(' ~ ', ' · ')}"


SPEAKER = re.compile(r"^(S|M|P|O|C|D|V|R)\. ")


def fmt(text):
    t = html.escape(text.replace("✠", "†").replace(" + ", " † "), quote=False)
    t = t.replace("℣.", "<b>V.</b>").replace("℣", "<b>V.</b>").replace("℟.", "<b>R.</b>").replace("℟", "<b>R.</b>")
    return SPEAKER.sub(lambda m: f"<b>{m.group(1)}.</b> ", t)


def is_ref(b):
    la = b.get("latin", "").strip()
    return len(la) < 40 and bool(re.match(r"^(Ps|[1-4]? ?[A-Z][a-z]+\.?) ?\d", la)) and ":" in la


def block_html(b):
    la, en = b.get("latin", "").strip(), b.get("english", "").strip()
    if is_ref(b):
        return f'<p class="ref">{fmt(la)}</p>'
    if not en or en == la or EDITION == ".la":
        return f'<p class="la">{fmt(la)}</p>' if la else ""
    if EDITION == ".en" or not la:
        return f'<p class="en1">{fmt(en)}</p>'
    # One Latin line, then its English, phrase by phrase.
    # One paragraph per pair, so the reader's paragraph gap falls between pairs only.
    return "".join(f'<p class="pair">{fmt(x)}<br/><i>{fmt(y)}</i></p>' for x, y in phrases(la, en))


def slug(s):
    return re.sub(r"[^a-z0-9]+", "-", s.lower()).strip("-")[:40] or "part"


def mass_chapters(day):
    """Split a DO day into [(id, latin title, english title, blocks)] using ordo.json ids."""
    chapters, cur = [], None
    for b in day["blocks"]:
        if b["kind"] == "heading":
            cid = HEADING_ID.get(b["latin"]) or slug(b["latin"])
            cur = [cid, b["latin"], b.get("english", ""), []]
            chapters.append(cur)
            continue
        if cur is None:
            cur = ["foot", "Incipit", "Beginning", []]
            chapters.append(cur)
        for sec, marker, new in INNER_SPLITS:
            if cur[0] == sec and marker in b.get("latin", ""):
                cur = [new, ORDO_LA.get(new, new), ORDO_EN.get(new, ""), []]
                chapters.append(cur)
                break
        if b.get("latin", "").strip() == cur[1]:
            continue  # DO repeats the section name as a line
        cur[3].append(b)
    # Orate fratres opens DO's Secreta; it belongs to the Lavabo step.
    for i, c in enumerate(chapters):
        if c[0] == "secret" and i > 0 and chapters[i - 1][0] == "lavabo":
            for j, b in enumerate(c[3][:5]):
                if b.get("latin", "").rstrip().endswith("Amen.") and "Oráte" in c[3][0].get("latin", ""):
                    chapters[i - 1][3] += c[3][: j + 1]
                    c[3] = c[3][j + 1:]
                    break
    # Keep ids unique (special days repeat headings).
    seen = {}
    for c in chapters:
        n = seen.get(c[0], 0)
        seen[c[0]] = n + 1
        if n:
            c[0] = f"{c[0]}-{n + 1}"
    return chapters


def propers_only(cid, blocks):
    def cut_from(pred):
        for i, b in enumerate(blocks):
            if pred(b.get("latin", "")):
                return blocks[i:]
        return blocks

    def cut_before(pred):
        for i, b in enumerate(blocks):
            if pred(b.get("latin", "")):
                return blocks[:i]
        return blocks

    if cid == "gospel":
        return cut_from(lambda t: "sancti Evangélii" in t or "Evangélii" in t and t.startswith(("Sequéntia", "Inítium")))
    if cid == "offertory":
        return cut_before(lambda t: t.startswith("Súscipe, sancte Pater"))
    if cid == "preface":
        return cut_before(lambda t: t.startswith("Sanctus, Sanctus"))
    return blocks


def xhtml(title, body):
    return ('<?xml version="1.0" encoding="utf-8"?>\n<!DOCTYPE html>\n'
            '<html xmlns="http://www.w3.org/1999/xhtml" xmlns:epub="http://www.idpf.org/2007/ops" lang="la">'
            f'<head><meta charset="utf-8"/><title>{html.escape(title)}</title>'
            '<link rel="stylesheet" type="text/css" href="style.css"/></head>'
            f"<body>{body}</body></html>")


def write_epub(path, ident, title, chapters):
    """chapters: [(file id, toc title, xhtml body)]"""
    path.parent.mkdir(parents=True, exist_ok=True)
    items = "".join(f'<item id="c{i}" href="{c[0]}.xhtml" media-type="application/xhtml+xml"/>' for i, c in enumerate(chapters))
    spine = "".join(f'<itemref idref="c{i}"/>' for i in range(len(chapters)))
    opf = ('<?xml version="1.0" encoding="utf-8"?>\n'
           '<package xmlns="http://www.idpf.org/2007/opf" version="3.0" unique-identifier="uid">'
           '<metadata xmlns:dc="http://purl.org/dc/elements/1.1/">'
           f'<dc:identifier id="uid">{ident}</dc:identifier><dc:title>{html.escape(title)}</dc:title>'
           '<dc:language>la</dc:language><dc:creator>Divinum Officium</dc:creator>'
           '<dc:publisher>Tabula · Acutis Ignatius</dc:publisher>'
           f'<meta property="dcterms:modified">{dt.datetime.now(dt.timezone.utc):%Y-%m-%dT%H:%M:%SZ}</meta></metadata>'
           '<manifest><item id="nav" href="nav.xhtml" media-type="application/xhtml+xml" properties="nav"/>'
           '<item id="ncx" href="toc.ncx" media-type="application/x-dtbncx+xml"/>'
           f'<item id="css" href="style.css" media-type="text/css"/>{items}</manifest>'
           f'<spine toc="ncx">{spine}</spine></package>')
    nav = xhtml(title, '<nav epub:type="toc"><ol>' + "".join(
        f'<li><a href="{c[0]}.xhtml">{html.escape(c[1])}</a></li>' for c in chapters) + "</ol></nav>")
    ncx = ('<?xml version="1.0" encoding="utf-8"?>\n<ncx xmlns="http://www.daisy.org/z3986/2005/ncx/" version="2005-1">'
           f'<head><meta name="dtb:uid" content="{ident}"/></head><docTitle><text>{html.escape(title)}</text></docTitle><navMap>'
           + "".join(f'<navPoint id="n{i}" playOrder="{i + 1}"><navLabel><text>{html.escape(c[1])}</text></navLabel>'
                     f'<content src="{c[0]}.xhtml"/></navPoint>' for i, c in enumerate(chapters))
           + "</navMap></ncx>")
    with zipfile.ZipFile(path, "w") as z:
        z.writestr(zipfile.ZipInfo("mimetype"), "application/epub+zip", compress_type=zipfile.ZIP_STORED)
        z.writestr("META-INF/container.xml", '<?xml version="1.0"?><container version="1.0" '
                   'xmlns="urn:oasis:names:tc:opendocument:xmlns:container"><rootfiles>'
                   '<rootfile full-path="OEBPS/content.opf" media-type="application/oebps-package+xml"/>'
                   '</rootfiles></container>', compress_type=zipfile.ZIP_DEFLATED)
        for name, data in [("content.opf", opf), ("nav.xhtml", nav), ("toc.ncx", ncx), ("style.css", CSS)]:
            z.writestr(f"OEBPS/{name}", data, compress_type=zipfile.ZIP_DEFLATED)
        for fid, toc, body in chapters:
            z.writestr(f"OEBPS/{fid}.xhtml", xhtml(toc, body), compress_type=zipfile.ZIP_DEFLATED)


def section(la, en, blocks, cue=None):
    body = f"<h2>{fmt(la)}" + (f"<br/><i>{fmt(en)}</i>" if en and en != la else "") + "</h2>"
    body += "".join(block_html(b) for b in blocks)
    if cue:
        body += f'<p class="cue">{fmt(cue)}</p>'
    return body


def title_page(kind, feast, d):
    return (f'<h1>{fmt(kind)}</h1><p class="c">{FERIA_LA[d.weekday()]}</p>'
            f'<p class="c"><i>{d:%A}, {date_short(d)}</i></p><h2>{fmt(feast.replace(" ~ ", " · "))}</h2>')


def build_mass_day(out, d, day):
    chapters = mass_chapters(day)
    feast = day["title"]
    by_id = {c[0]: c for c in chapters}
    tp = ("title", "Titulus", title_page("Missa", feast, d))
    full = [tp] + [(c[0], f"{c[1]} · {c[2]}" if c[2] and c[2] != c[1] else c[1], section(c[1], c[2], c[3])) for c in chapters]
    write_epub(out / f"Tabula/Today/{d}.full{EDITION}.epub", f"tabula-mass-full{EDITION}-{d}", book_title("Missa", d, feast), full)
    props = [tp]
    for pid in PROPER_IDS:
        c = by_id.get(pid)
        if not c:
            continue
        blocks = propers_only(pid, c[3])
        if blocks:
            props.append((pid, f"{c[1]} · {c[2]}", section(c[1], c[2], blocks, CUES.get(pid))))
    write_epub(out / f"Tabula/Today/{d}.propers{EDITION}.epub", f"tabula-mass-propers{EDITION}-{d}", book_title("Propria", d, feast), props)


def build_ordinary(out, day):
    chapters = mass_chapters(day)
    by_id = {c[0]: c for c in chapters}
    parts = [("title", "Ordo Missæ", '<h1>Ordo Missæ</h1><p class="c"><i>The Order of Mass · Missale Romanum 1962</i></p>')]
    for step in ORDO_STEPS:
        sid = step.get("id")
        if not sid:
            continue
        c = by_id.get(sid)
        if step.get("kind") == "proper" or not c:
            body = (f"<h2>{fmt(step['la'])}<br/><i>{fmt(step['en'])}</i></h2>"
                    '<p class="cue">Proper of the day: see today’s Mass.</p>')
        else:
            body = section(c[1], c[2], c[3])
        parts.append((sid, f"{step['la']} · {step['en']}", body))
    write_epub(out / f"Tabula/Missal/Ordo-Missae{EDITION}.epub", f"tabula-ordo-missae{EDITION}", "Ordo Missæ", parts)


def build_hours_day(out, d, hours, texts):
    feast = next(iter(hours.values()))["title"] if hours else ""
    parts = [("title", "Titulus", title_page("Breviarium Romanum", feast, d))]
    for hid, en, la in HOURS:
        h = hours.get(en)
        if not h:
            continue
        body = f"<h1>{fmt(la)}</h1><p class=\"c\"><i>{en}</i></p>"
        for ref in h["blocks"]:
            b = texts.get(ref, ref) if isinstance(ref, str) else ref
            if not isinstance(b, dict):
                continue
            if b.get("kind") == "heading":
                body += f"<h2>{fmt(b.get('latin', ''))}" + (
                    f"<br/><i>{fmt(b['english'])}</i>" if b.get("english") and b["english"] != b.get("latin") else "") + "</h2>"
            else:
                body += block_html(b)
        parts.append((hid, f"{la} · {en}", body))
    write_epub(out / f"Tabula/Hours/{d}.hours{EDITION}.epub", f"tabula-hours{EDITION}-{d}", book_title("Horæ", d, feast), parts)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--mass", type=Path, required=True)
    p.add_argument("--office", type=Path, required=True)
    p.add_argument("--out", type=Path, default=REPO / "build/tabula-sd")
    p.add_argument("--ordinary-day", default=None, help="a day with Gloria and Credo for Ordo-Missae.epub")
    a = p.parse_args()

    global ORDO_STEPS, ORDO_LA, ORDO_EN
    ordo = json.loads(ORDO.read_text())
    ORDO_STEPS = ordo["steps"]
    ORDO_LA = {s["id"]: s["la"] for s in ORDO_STEPS if "id" in s}
    ORDO_EN = {s["id"]: s["en"] for s in ORDO_STEPS if "id" in s}

    mass = json.loads(a.mass.read_text())
    office = json.loads(a.office.read_text())
    for sub in ("Tabula/Today", "Tabula/Hours", "Tabula/Missal"):
        for f in (a.out / sub).glob("*.epub") if (a.out / sub).exists() else []:
            f.unlink()

    global EDITION
    days = mass["days"]
    ord_day = a.ordinary_day or next(ds for ds, day in days.items()
                                      if dt.date.fromisoformat(ds).weekday() == 6
                                      and {"Gloria", "Credo"} <= {b["latin"] for b in day["blocks"] if b["kind"] == "heading"})
    for EDITION in EDITIONS:
        for ds, day in days.items():
            build_mass_day(a.out, dt.date.fromisoformat(ds), day)
        build_ordinary(a.out, days[ord_day])
        for ds, trad in office["days"].items():
            hours = trad.get("roman1962")
            if hours:
                build_hours_day(a.out, dt.date.fromisoformat(ds), hours, office["texts"])

    (a.out / "Tabula/Missal").mkdir(parents=True, exist_ok=True)
    shutil.copy(ORDO, a.out / "Tabula/Missal/ordo.json")
    src = mass["source"]
    (a.out / "LICENSES.txt").write_text(
        "Tabula content\n\nMass and Office texts: Divinum Officium (https://www.divinumofficium.com), MIT License.\n"
        f"Revision {src['revision']}, {src['rite']}; {src['calendar']}.\n"
        "Latin with Divinum Officium traditional English. Not an FSSP-published or endorsed edition;\n"
        "local and FSSP-specific propers are not applied.\n")
    n = sum(1 for _ in a.out.rglob("*.epub"))
    size = sum(f.stat().st_size for f in a.out.rglob("*") if f.is_file())
    print(f"{n} EPUBs, {size / 1e6:.1f} MB -> {a.out}  (Ordinary from {ord_day})")


if __name__ == "__main__":
    main()
