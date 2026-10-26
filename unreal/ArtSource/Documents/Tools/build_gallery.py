#!/usr/bin/env python3
"""Build an offline production gallery and a labelled JPG contact sheet."""

from __future__ import annotations

import argparse
import html
import io
import json
from pathlib import Path

from PIL import Image, ImageCms, ImageDraw, ImageFont, ImageOps


ASSET_ROOT = Path(__file__).resolve().parent.parent
GROUP_NAMES = {
    "dale_office": "Dales mappe",
    "evening_documents": "Skift og feltarbeid",
    "dale_private_drafts": "Dales private samling",
    "archive_newspaper": "Gamle aviser",
    "survey_archive": "Målefeltets arkiv",
    "saro_technical_archive": "SAROs tekniske arkiv",
    "supplemental_archive": "Frivillige bakgrunnsrekvisitter",
}


def escape(value: object) -> str:
    return html.escape(str(value), quote=True)


def gallery_bytes(catalog: dict) -> bytes:
    cards = []
    groups = list(dict.fromkeys(asset["group"] for asset in catalog["assets"]))
    ready = sum("image" in asset for asset in catalog["assets"])
    for index, asset in enumerate(catalog["assets"], 1):
        title = escape(asset["title"])
        group = GROUP_NAMES.get(asset["group"], asset["group"])
        if "image" in asset:
            dimensions = asset["image"]
            picture = f'<a class="picture" href="{escape(asset["filename"])}" target="_blank" aria-label="Åpne {title} i full størrelse"><img src="{escape(asset["filename"])}" width="{dimensions["width"]}" height="{dimensions["height"]}" loading="lazy" alt="{title}"></a>'
        else:
            picture = '<div class="picture missing">Under arbeid</div>'
        cards.append(f'''<article data-group="{escape(asset["group"])}">
<div class="card-top"><span>{index:02d}</span><span>{escape(group)}</span></div>
{picture}
<h2>{title}</h2>
<p class="use">{escape(asset["intended_use_nb"])}</p>
<details><summary>Les dokumentteksten</summary><pre>{escape(asset["text"])}</pre></details>
</article>''')
    buttons = ['<button type="button" data-filter="all" aria-pressed="true">Alle</button>']
    buttons += [f'<button type="button" data-filter="{escape(group)}" aria-pressed="false">{escape(GROUP_NAMES.get(group, group))}</button>' for group in groups]
    document = '''<!doctype html>
<html lang="nb"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>SIGNAL / 47 · Dokumentarkiv</title>
<style>
:root{color-scheme:light;--paper:#eae2ce;--ink:#242923;--muted:#626451;--red:#8c392c}*{box-sizing:border-box}body{margin:0;background:#e3dcc9;color:var(--ink);font:16px/1.55 system-ui,sans-serif}header,main{max-width:1660px;margin:auto;padding:34px 32px}header{padding-bottom:16px}.eyebrow{font:13px/1.4 monospace;letter-spacing:3px;color:var(--red)}h1{font:clamp(34px,5vw,64px)/1.08 Georgia,serif;margin:16px 0}header p{max-width:720px;color:var(--muted)}nav{display:flex;flex-wrap:wrap;gap:9px;margin:25px 0 10px}button{font:14px system-ui,sans-serif;padding:9px 15px;background:transparent;border:1px solid #a5a18f;border-radius:3px;color:var(--ink);cursor:pointer}button[aria-pressed=true]{background:var(--ink);color:var(--paper);border-color:var(--ink)}button:focus-visible,a:focus-visible,summary:focus-visible{outline:3px solid var(--red);outline-offset:4px}#count{color:var(--muted);font:13px monospace}.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(290px,1fr));gap:24px}article{min-width:0;align-self:start;background:#f3ecdb;border:1px solid #c9c0a8;padding:18px;box-shadow:0 5px 15px #34301d0b}article[hidden]{display:none}.card-top{display:flex;justify-content:space-between;gap:14px;font:11px/1.4 monospace;color:var(--muted);margin:0 0 13px;text-transform:uppercase;letter-spacing:1px}.picture{display:flex;width:100%;height:365px;align-items:center;justify-content:center;background:#d2c9b2;overflow:hidden}.picture img{display:block;max-width:100%;max-height:100%;width:auto;height:auto;object-fit:contain}.missing{color:var(--muted);font-family:Georgia,serif}h2{font:22px/1.22 Georgia,serif;margin:18px 0 10px}.use{font-size:13px;color:var(--muted);margin:0 0 18px}details{border-top:1px solid #d0c5ac;padding-top:12px}summary{cursor:pointer;font-size:13px;font-weight:600}pre{font:13px/1.65 ui-monospace,monospace;white-space:pre-wrap;overflow-wrap:anywhere;margin:16px 0 0}footer{max-width:1660px;margin:0 auto;padding:0 32px 40px;font-size:13px;color:var(--muted)}@media(max-width:600px){header,main{padding:24px 18px}.picture{height:390px}nav{gap:7px}button{padding:8px 10px}footer{padding:0 18px 30px}}@media print{nav,details{display:none}article{break-inside:avoid;box-shadow:none}.picture{height:280px}}
</style></head><body>
<header><div class="eyebrow">SIGNAL / 47</div><h1>Dales dokumentarkiv</h1>
<p>Feltmapper, private notater og gamle arkivspor fra New Mexico. Åpne et bilde for å se dokumentet i full størrelse, eller brett ut teksten under.</p>
<nav aria-label="Filtrer dokumenter">BUTTONS</nav><p id="count" aria-live="polite">COUNT</p></header>
<main><div class="grid">CARDS</div></main>
<footer>Produksjonsgalleri. Aviser og dokumenter er fiktive rekvisitter til spillet.</footer>
<script>
const buttons=[...document.querySelectorAll('button[data-filter]')];
const cards=[...document.querySelectorAll('article[data-group]')];
for(const button of buttons){button.addEventListener('click',()=>{
for(const other of buttons)other.setAttribute('aria-pressed',String(other===button));
let shown=0;for(const card of cards){card.hidden=button.dataset.filter!=='all'&&card.dataset.group!==button.dataset.filter;if(!card.hidden)shown++;}
document.querySelector('#count').textContent=shown+' av '+cards.length+' dokumenter';
});}
</script></body></html>
'''
    document = document.replace("BUTTONS", "".join(buttons)).replace("CARDS", "\n".join(cards))
    document = document.replace("COUNT", f"{len(catalog['assets'])} dokumenter · {ready} bilder klare")
    return document.encode("utf-8")


def font(size: int, bold: bool = False) -> ImageFont.ImageFont:
    try:
        return ImageFont.truetype("DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf", size)
    except OSError:
        return ImageFont.load_default(size=size)


def wrapped_label(draw: ImageDraw.ImageDraw, value: str, width: int, face: ImageFont.ImageFont) -> list[str]:
    words = value.split()
    lines: list[str] = []
    for word in words:
        if lines and draw.textlength(lines[-1] + " " + word, font=face) <= width:
            lines[-1] += " " + word
        else:
            lines.append(word)
    if len(lines) > 2:
        lines = lines[:2]
        while draw.textlength(lines[1] + "…", font=face) > width:
            lines[1] = lines[1][:-1]
        lines[1] += "…"
    return lines


def overview_bytes(asset_root: Path, catalog: dict, pending: dict[str, bytes] | None = None) -> bytes:
    pending = pending or {}
    columns, card_w, card_h, gap, margin, header_h = (7 if len(catalog["assets"]) > 30 else 6), 236, 338, 16, 28, 100
    rows = (len(catalog["assets"]) + columns - 1) // columns
    sheet = Image.new("RGB", (margin * 2 + columns * card_w + (columns - 1) * gap,
                              margin * 2 + header_h + rows * card_h + (rows - 1) * gap), "#ded5bd")
    draw = ImageDraw.Draw(sheet)
    draw.text((margin, margin), "SIGNAL / 47", font=font(27, True), fill="#7f3428")
    draw.text((margin, margin + 42), f"DOKUMENTARKIV · {len(catalog['assets'])} REKVISITTER", font=font(18), fill="#31372e")
    label_font = font(13)
    for index, asset in enumerate(catalog["assets"]):
        x = margin + (index % columns) * (card_w + gap)
        y = margin + header_h + (index // columns) * (card_h + gap)
        draw.rectangle((x, y, x + card_w, y + card_h), fill="#f2ead7")
        thumb_box = (card_w - 16, card_h - 71)
        data = pending.get(asset["id"])
        path = asset_root / asset["filename"]
        if data is not None or path.is_file():
            with Image.open(io.BytesIO(data) if data is not None else path) as image:
                thumb = ImageOps.contain(image.convert("RGB"), thumb_box, Image.Resampling.LANCZOS)
                sheet.paste(thumb, (x + (card_w - thumb.width) // 2, y + 8 + (thumb_box[1] - thumb.height) // 2))
        else:
            draw.text((x + 30, y + 120), "UNDER ARBEID", font=font(16), fill="#777866")
        draw.text((x + 9, y + card_h - 59), f"{index + 1:02d}", font=font(13, True), fill="#8c392c")
        for line_no, line in enumerate(wrapped_label(draw, asset["title"], card_w - 45, label_font)):
            draw.text((x + 36, y + card_h - 60 + line_no * 18), line, font=label_font, fill="#30372e")
    stream = io.BytesIO()
    srgb = ImageCms.ImageCmsProfile(ImageCms.createProfile("sRGB")).tobytes()
    sheet.save(stream, "JPEG", quality=82, optimize=True, progressive=True, subsampling=0, icc_profile=srgb)
    return stream.getvalue()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asset-root", type=Path, default=ASSET_ROOT)
    args = parser.parse_args()
    catalog = json.loads((args.asset_root / "catalog.json").read_text(encoding="utf-8"))
    gallery = gallery_bytes(catalog)
    overview = overview_bytes(args.asset_root, catalog)
    (args.asset_root / "GALLERI.html").write_bytes(gallery)
    (args.asset_root / "OVERSIKT.jpg").write_bytes(overview)
    print("Wrote offline GALLERI.html and labelled OVERSIKT.jpg.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
