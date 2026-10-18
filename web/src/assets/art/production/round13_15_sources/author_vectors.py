"""Original vector assets for ART_BRIEF rounds 14 and 15. No game code changes.

Run with Pillow and CairoSVG installed. Each SVG is a standalone editable source;
PNG/JPEG exports carry the same sRGB ICC as the generated raster exports.
"""
from pathlib import Path
import html
import io
import math
import random
import json
import cairosvg
from PIL import Image, ImageCms

HERE = Path(__file__).resolve().parent
ART = HERE.parent.parent
PROFILE = HERE / 'srgb.icc'
if not PROFILE.exists():
    PROFILE.write_bytes(ImageCms.ImageCmsProfile(ImageCms.createProfile('sRGB')).tobytes())
ICC = PROFILE.read_bytes()
SPECS = []


def svg(w, h, content, defs=''):
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}"><defs>{defs}</defs>{content}</svg>'


def save(name, w, h, content, description, defs='', extra=None):
    source = HERE / (name.replace('/', '__').rsplit('.', 1)[0] + '.svg')
    source.write_text(svg(w, h, content, defs), encoding='utf-8')
    image = Image.open(io.BytesIO(cairosvg.svg2png(url=str(source)))).convert('RGBA')
    dest = ART / name
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.suffix == '.jpg':
        image.convert('RGB').save(dest, quality=85, subsampling=0, optimize=True, icc_profile=ICC)
    else:
        image.save(dest, optimize=True, icc_profile=ICC)
    SPECS.append(dict(file=name, size=[w, h], description=description,
                      source=source.name, method='original-svg', **(extra or {})))


def textline(line, x, y, size, color, family='Liberation Sans', weight='bold'):
    return f'<text x="{x}" y="{y}" text-anchor="middle" font-family="{family}" font-weight="{weight}" font-size="{size}" fill="{color}">{html.escape(line)}</text>'


def signs():
    configs = [
        ('sign_caliche', 1024, 512, '#235341', '#e5e6d7', [('CALICHE', 225, 150), ('POP. 412', 370, 79)], 'Bygrenseskilt, grønn refleksplate', 18),
        ('sign_gas', 1024, 512, '#dfd8bd', '#922f25', [('GAS', 235, 174), ('DIESEL', 391, 101)], 'Slitt hvitt lysskilt med rød kant', 24),
        ('sign_1420', 1024, 512, '#162126', '#e9dfb8', [('1420 AM', 231, 139), ('NIGHT LINE', 374, 84)], 'Mørkt takskilt for nattradioen', 16),
        ('sign_souvenir', 1024, 512, '#705541', '#e2c893', [('SAUCER CRASH 1947', 161, 68), ('TOURS', 300, 123), ('ASK INSIDE', 414, 67)], 'Håndmalt treskilt med eldre overmaling', 11),
        ('sign_sheriff', 1024, 256, '#d4cbb1', '#263b3a', [('COUNTY SHERIFF', 121, 80), ('SUBSTATION', 202, 48)], 'Metallskilt over sheriffens dør', 12),
        ('marquee_plaza', 2048, 256, '#ded8c0', '#262a28', [('CLOSED', 77, 55), ('THANK YOU CALICHE', 147, 55), ('FOR 31 YEA S', 217, 55)], 'Kinomarkise, bokstaven R har falt ut av YEARS', 14),
        ('board_church', 1024, 256, '#1b2426', '#e7dfc6', [('THE HEAVENS DECLARE THE GLORY OF GOD', 116, 34), ('PS 19:1', 201, 54)], 'Riflet bokstavtavle ved kirken', 12),
    ]
    for i, (stem, w, h, bg, ink, lines, desc, margin) in enumerate(configs):
        rng = random.Random(4700 + i)
        inner = f'<rect x="{margin}" y="{margin}" width="{w-2*margin}" height="{h-2*margin}" rx="{12 if h>256 else 4}" fill="{bg}" stroke="{ink}" stroke-width="7"/>'
        if stem == 'sign_souvenir':
            inner += '<rect x="40" y="42" width="944" height="426" fill="#354841" opacity=".17"/>'
            for y in [126, 247, 367]:
                inner += f'<path d="M26 {y} Q500 {y+4} 998 {y-2}" fill="none" stroke="#201e1c" stroke-width="3" opacity=".42"/>'
        if stem in ('board_church', 'marquee_plaza'):
            step = 10 if stem == 'board_church' else 69
            for y in range(24, h-14, step):
                inner += f'<path d="M24 {y} H{w-24}" stroke="{ink}" opacity=".13" stroke-width="2"/>'
        for _ in range(230):
            x, y = rng.uniform(27, w-27), rng.uniform(25, h-25)
            edge = min(x, w-x, y, h-y)
            if edge > 48 and rng.random() < .9:
                continue
            inner += f'<ellipse cx="{x:.1f}" cy="{y:.1f}" rx="{rng.uniform(.8,5):.1f}" ry="{rng.uniform(.5,2.7):.1f}" fill="{rng.choice(["#a88f65", "#403a30", "#dfd2af"])}" opacity=".35"/>'
        for x in [margin+15, w-margin-15]:
            for y in [margin+15, h-margin-15]:
                inner += f'<circle cx="{x}" cy="{y}" r="4" fill="#54544a"/><circle cx="{x-1}" cy="{y-1}" r="1" fill="#d8d4be"/>'
        save(f'town/{stem}_blank.png', w, h, inner, desc + ', uten tekst', extra={'sign_text': []})
        lettering = ''.join(textline(t, w/2, y, fs, ink) for t, y, fs in lines)
        save(f'town/{stem}.png', w, h, inner + lettering, desc, extra={'sign_text': [t for t, _, _ in lines]})


def board():
    wooddefs = '<linearGradient id="wood" x1="0" y1="0" x2=".25" y2="1"><stop stop-color="#816145"/><stop offset=".3" stop-color="#59432f"/><stop offset=".7" stop-color="#735338"/><stop offset="1" stop-color="#3a2e24"/></linearGradient><clipPath id="rail"><path fill-rule="evenodd" d="M8 8 H2040 V1272 H8 Z M64 64 V1216 H1984 V64 Z"/></clipPath>'
    content = '<path fill-rule="evenodd" d="M8 8 H2040 V1272 H8 Z M64 64 V1216 H1984 V64 Z" fill="url(#wood)"/>'
    rng = random.Random(4715)
    grain = ''
    for _ in range(165):
        y = rng.uniform(10, 1270)
        grain += f'<path d="M10 {y:.1f} C560 {y+3:.1f} 1500 {y-3:.1f} 2040 {y+1:.1f}" stroke="{rng.choice(["#c1996b", "#281e18"])}" opacity=".16" fill="none" stroke-width="{rng.uniform(.6,2):.2f}"/>'
    content += f'<g clip-path="url(#rail)">{grain}</g><path d="M12 12 H2036 V1268 H12 Z M61 61 H1987 V1219 H61 Z" fill="none" stroke="#b49365" opacity=".5" stroke-width="3"/><path d="M8 8 L64 64 M1984 64 L2040 8 M64 1216 L8 1272 M1984 1216 L2040 1272" stroke="#30241a" stroke-width="2"/><path d="M2013 8 L2040 8 L2040 30 L2030 23 L2020 26 Z" fill="#c6a473"/>'
    save('board/frame_wood.png', 2048, 1280, content, 'Smal slitt treramme, 1920×1152 åpent midtfelt ved 64,64', wooddefs, {'window': [64,64,1920,1152]})
    for color, dark, light, name in [('#b32f32','#6d1c22','#e48a77','red'),('#d2aa39','#82641c','#f0d675','yellow'),('#356e98','#203e65','#83b3c6','blue')]:
        defs=f'<radialGradient id="head" cx=".35" cy=".28"><stop stop-color="{light}"/><stop offset=".5" stop-color="{color}"/><stop offset="1" stop-color="{dark}"/></radialGradient>'
        c='<ellipse cx="74" cy="80" rx="32" ry="17" fill="#211b15" opacity=".17"/><path d="M63 61 L73 91" stroke="#7e7d72" stroke-width="5"/><ellipse cx="61" cy="55" rx="30" ry="25" fill="url(#head)"/><ellipse cx="52" cy="45" rx="11" ry="6" fill="#fff5de" opacity=".2"/>'
        save(f'board/pin_{name}.png',128,128,c,f'Tegnestift med {name} rundt plasthode',defs)
    c='<path d="M0 16 H512" stroke="#54272b" stroke-width="8"/><path d="M0 15 H512" stroke="#a43c3d" stroke-width="5"/>'
    for x in range(-16, 528, 8):
        c += f'<path d="M{x} 12 L{x+6} 19" stroke="#d17162" opacity=".65" stroke-width="1.2"/><path d="M{x+2} 10 l3 -2 M{x+6} 21 l2 2" stroke="#aa5050" stroke-width=".8" opacity=".5"/>'
    save('board/thread_red.png',512,32,c,'Rødt ullgarn, periode 8 px og sømløst på langs',extra={'tile_axes': 'x'})
    c='<path d="M14 16 L742 13 L755 37 L754 492 L14 495 Z" fill="#c2b79e" opacity=".3"/><path d="M12 12 L736 12 L752 31 L752 492 L12 492 Z" fill="#e9e1ca" stroke="#b7ab90" stroke-width="2"/><path d="M736 12 L735 32 L752 31 Z" fill="#d0c5ac"/>'
    c += '<path d="M28 91 H735" stroke="#ac6a66" stroke-width="2" opacity=".6"/>'
    for y in range(140, 480, 44): c += f'<path d="M29 {y} H735" stroke="#7b98a3" opacity=".48" stroke-width="1.4"/>'
    save('board/card_index_blank.png',768,512,c,'Linjert arkivkort, blå linjer og rød toppmarg, uten tekst')
    c='<path d="M12 15 L22 18 L18 25 L25 31 L18 38 L23 44 L16 51 L22 59 L17 67 L23 76 L12 80 L241 80 L236 72 L243 65 L238 58 L245 50 L238 43 L244 36 L238 29 L244 22 L238 14 Z" fill="#d6caa4" opacity=".75"/><path d="M28 21 L231 20 M29 72 L230 74" stroke="#f6efd3" opacity=".22" stroke-width="2"/>'
    save('board/tape_strip.png',256,96,c,'Revet maskeringstape med delvis gjennomsiktig papir')
    rng=random.Random(4747); c=''
    for i in range(170):
        angle=i*2*math.pi/170
        if .1 < angle < .4 or 3.5 < angle < 3.75: continue
        radius=175+math.sin(angle*7)*3+rng.uniform(-4,4)
        x=256+math.cos(angle)*radius; y=251+math.sin(angle)*radius*.89
        c+=f'<ellipse cx="{x:.2f}" cy="{y:.2f}" rx="{rng.uniform(3,7):.2f}" ry="{rng.uniform(2,5):.2f}" fill="#79502b" opacity="{rng.uniform(.02,.08):.3f}"/>'
    c+='<ellipse cx="408" cy="366" rx="11" ry="6" fill="#81542d" opacity=".09"/><ellipse cx="430" cy="389" rx="4" ry="3" fill="#81542d" opacity=".08"/>'
    save('board/stain_coffee.png',512,512,c,'Svak ujevn tørket kaffering')


def album():
    save('album/photo_corner.png',128,128,'<path d="M14 14 H114 L14 114 Z" fill="#141615" stroke="#353631" stroke-width="2"/><path d="M25 20 H100 L20 100 V25" fill="#20221f"/><path d="M14 114 L114 14" stroke="#42423b" stroke-width="2"/>','Én svart fotohjørnelomme sett ovenfra')
    c='<path fill-rule="evenodd" d="M0 0 H768 V900 H0 Z M48 48 V720 H720 V48 Z" fill="#e9e5d8"/><path d="M3 3 H765 V897 H3 Z" fill="none" stroke="#c2beaf" stroke-width="4"/><path d="M47 47 H721 V721 H47 Z" fill="none" stroke="#b7b5a9" stroke-width="1"/>'
    save('album/frame_instant_blank.png',768,900,c,'Instant-ramme, eksakt åpent vindu 672×672 ved 48,48',extra={'window':[48,48,672,672]})
    c='<rect width="1024" height="820" fill="#e8e3d5"/><path fill-rule="evenodd" d="M36 33 H988 V787 H36 Z M64 60 V760 H960 V60 Z" fill="#1b1d1b"/><path fill-rule="evenodd" d="M0 0 H1024 V820 H0 Z M64 60 V760 H960 V60 Z" fill="#e8e3d5"/><path fill-rule="evenodd" d="M36 33 H988 V787 H36 Z M64 60 V760 H960 V60 Z" fill="#1b1d1b"/><path d="M860 33 v10 h8 v-10 M878 33 v8 h8 v-8 M896 33 v10 h8 v-10" fill="#e8e3d5"/>'
    # Use even-odd geometry for the entire source. No matte is painted over a photo.
    c='<path fill-rule="evenodd" d="M0 0 H1024 V820 H0 Z M64 60 V760 H960 V60 Z" fill="#e8e3d5"/><path fill-rule="evenodd" d="M36 33 H988 V787 H36 Z M64 60 V760 H960 V60 Z" fill="#1b1d1b"/><path d="M860 33 v10 h8 v-10 M878 33 v8 h8 v-8 M896 33 v10 h8 v-10" fill="#e8e3d5"/>'
    save('album/frame_sheetfilm_blank.png',1024,820,c,'Planfilmramme, eksakt åpent vindu 896×700 ved 64,60',extra={'window':[64,60,896,700]})


def silhouettes():
    # The same body is used for all three distant faceless silhouettes.
    body='M240 227 C222 227 215 212 214 195 C192 180 183 151 184 120 C186 73 211 48 250 47 C291 47 321 71 322 114 C325 153 305 181 282 196 L279 228 C301 231 319 245 325 266 L355 535 L341 559 L328 550 L297 327 L290 527 L310 899 L297 948 L270 946 L252 627 L237 944 L209 944 L198 904 L217 526 L209 327 L180 548 L166 557 L154 538 L180 274 C184 246 210 232 240 227 Z'
    defs='<linearGradient id="rim"><stop stop-color="#111723"/><stop offset=".14" stop-color="#030405"/><stop offset=".8" stop-color="#000"/><stop offset="1" stop-color="#263043"/></linearGradient>'
    save('alien/silhouette_stand.png',512,1024,f'<path d="{body}" fill="url(#rim)"/>','Høy svart skikkelse med lange armer, uten ansiktsdetaljer',defs)
    # Narrower body, tilted oversized head. It remains the same character.
    c='<g transform="translate(93 0) scale(.66 1)"><path d="'+body+'" fill="url(#rim)"/></g><ellipse cx="257" cy="120" rx="49" ry="77" transform="rotate(15 257 120)" fill="#010203"/>'
    save('alien/silhouette_turned.png',512,1024,c,'Samme skikkelse halvt fra siden, hodet lyttende på skrå',defs)
    # Multiple very faint contours make a soft distant edge without face detail.
    c=''
    for dx,dy,opacity in [(-1,0,.11),(1,0,.11),(0,-1,.11),(0,1,.11),(0,0,.82)]:
        c+=f'<g transform="translate({dx} {dy}) scale(.5)"><path d="{body}" fill="#000" opacity="{opacity}"/></g>'
    save('alien/silhouette_far.png',256,512,c,'Samme skikkelse på avstand med myk kant')


def branding_and_testcard():
    c='<circle cx="256" cy="256" r="246" fill="#17324e"/><path d="M49 298 H139 L177 270 L201 306 L224 282 H463" fill="none" stroke="#ffb052" stroke-width="9" stroke-linejoin="round"/><path d="M156 155 Q235 325 389 236 L317 184 Z" fill="#04070d"/><path d="M204 247 L177 408 H338 L281 266 M224 301 L308 372 M204 373 L280 287" fill="none" stroke="#05080f" stroke-width="20" stroke-linejoin="round"/><path d="M156 155 L317 184 L389 236 M234 169 L317 184 L332 126" fill="none" stroke="#708796" stroke-width="10"/><path d="M156 155 Q260 243 389 236" fill="none" stroke="#b6c7cd" stroke-width="7"/>'
    save('brand/icon_512.png',512,512,c,'Radioparabol i mørkeblå sirkel med tynn oransje signallinje')
    colors=['#b9b9b9','#b6ad22','#25a9a5','#29984a','#a235a0','#a23432','#283e9d']
    c='<rect width="640" height="480" fill="#202322"/>'
    for i,color in enumerate(colors):c+=f'<rect x="{i*640/7:.4f}" y="0" width="{640/7+.1:.4f}" height="480" fill="{color}"/>'
    c+='<circle cx="320" cy="240" r="194" fill="#b3b4aa" stroke="#252925" stroke-width="4"/>'
    for p in range(-160,161,32):
        half=math.sqrt(190**2-p**2)
        c+=f'<path d="M{320+p} {240-half:.3f} V{240+half:.3f} M{320-half:.3f} {240+p} H{320+half:.3f}" stroke="#555c54" stroke-width="1.4"/>'
    c+='<path d="M127 240 H513 M320 47 V433" stroke="#202822" stroke-width="3"/>'
    for i,color in enumerate(['#161817','#3b3e3b','#62655f','#96998e','#c3c4b4','#ecebda']):c+=f'<rect x="{164+i*52}" y="300" width="52" height="49" fill="{color}"/>'
    c+='<rect x="172" y="186" width="296" height="89" fill="#2d332f"/>'
    for x in range(180,460,7): c+=f'<rect x="{x}" y="193" width="3" height="75" fill="#d6d8c7"/>'
    save('motel/tv_testcard_blank.jpg',640,480,c,'Generisk testbilde med fargestriper, sirkel, rutenett og gråtrapp')


if __name__ == '__main__':
    signs(); board(); album(); silhouettes(); branding_and_testcard()
    (HERE / 'vector-specs.json').write_text(json.dumps(SPECS, ensure_ascii=False, indent=2)+'\n',encoding='utf-8')
    print(f'Exported {len(SPECS)} original vector assets.')
