# The link preview (index.html, og:image and twitter:image): round 15's og_cover.jpg with the
# title written in its dark left third, as the brief has it (ART_BRIEF.md, round 15, brand/), in
# the title screen's own type: SIGNAL / 47 in Oswald, the slash in neon red, the line under it in
# VT323. The words are set here, not painted, so they are always right. Writes
# src/assets/art/runtime/og_card.jpg (1200x630). Run from web/ when the cover or the words change:
#   python3 tools/brand_card.py
import asyncio, base64, io
from pathlib import Path
from playwright.async_api import async_playwright
from PIL import Image

ART = Path('src/assets/art')
OUT = ART / 'runtime/og_card.jpg'
b64 = lambda p: base64.b64encode(p.read_bytes()).decode()
PAGE = f'''<!doctype html><html><head><style>
@font-face {{ font-family: Oswald; src: url(data:font/woff2;base64,{b64(Path('src/assets/fonts/oswald.woff2'))}) format('woff2'); font-weight: 200 700; }}
@font-face {{ font-family: VT323; src: url(data:font/woff2;base64,{b64(Path('src/assets/fonts/vt323.woff2'))}) format('woff2'); }}
html, body {{ margin: 0; width: 1200px; height: 630px; overflow: hidden; background: #05070d; }}
.card {{ position: relative; width: 1200px; height: 630px; background: url(data:image/jpeg;base64,{b64(ART / 'brand/og_cover.jpg')}) 0 0 / 1200px 630px; }}
.card::before {{ content: ''; position: absolute; inset: 0; background: linear-gradient(90deg, rgba(3,4,8,.55) 0%, rgba(3,4,8,.25) 30%, transparent 45%); }}
.words {{ position: absolute; left: 64px; bottom: 92px; }}
h1 {{ margin: 0; font-family: Oswald; font-weight: 500; font-size: 118px; line-height: .86; letter-spacing: .02em; color: #f3efe4; text-shadow: 0 0 40px rgba(255,140,60,.25); white-space: nowrap; }}
h1 span {{ color: #ff5a45; font-weight: 300; margin: 0 .12em; }}
p {{ margin: 18px 0 0; font-family: VT323; font-size: 34px; color: #ffb052; }}
</style></head><body><div class="card"><div class="words"><h1>SIGNAL<span>/</span>47</h1><p>New Mexico, 1986. Night shift at SARO.</p></div></div></body></html>'''

async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch()
        pg = await b.new_page(viewport={'width': 1200, 'height': 630})
        await pg.set_content(PAGE)
        await pg.evaluate('document.fonts.ready')
        ok = await pg.evaluate("document.fonts.check('500 118px Oswald') && document.fonts.check('34px VT323')")
        png = await pg.screenshot(type='png')
        await b.close()
    if not ok: raise SystemExit('the fonts did not load')
    im = Image.open(io.BytesIO(png)).convert('RGB')
    assert im.size == (1200, 630), im.size
    im.save(OUT, 'JPEG', quality=86, optimize=True, progressive=True)
    print(f'{OUT}: {im.size[0]}x{im.size[1]}, {OUT.stat().st_size:,} bytes')

asyncio.run(main())
