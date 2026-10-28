from pathlib import Path
import json, html, hashlib, shutil
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent
pairs = [
    ('Startmeny', 'title', '01-startmeny', 'title', 'Mer rom rundt valgene, roligere typografi og større, mørkere antennesilhuetter.'),
    ('Options', 'settings-picture', '02-options', 'settings', 'Grupperte innstillinger, bildeforhåndsvisning og tydeligere valgmarkering.'),
    ('Gameplay HUD', 'room', '03-gameplay-hud', 'hud', 'Mindre tekst over verdenen, tydelig interaksjon og en samlet oppgavepåminnelse.'),
    ('Notatbok', 'notebook-findings', '04-notatbok', 'notebook', 'Fotobevis ved siden av hverandre, horisontale faner og korte, lesbare funn.'),
    ('Pause', 'pause', '05-pause', 'pause', 'Roligere venstremeny og et fysisk notatkort i arbeidsmiljøet.'),
]
def fig(src, label):
    return f'<figure><a href="{src}" target="_blank"><img loading="lazy" src="{src}" alt="{html.escape(label)}"></a><figcaption>{html.escape(label)}</figcaption></figure>'

sections = []
for i, (title, before, after, old, note) in enumerate(pairs):
    a = f'skjermbilder/{before}.png'; b = f'nye/{after}.png'
    sections.append(f'''<section id="screen-{i}" class="comparison"><div class="section-head"><h2>{title}</h2><p>{note}</p></div>
    <div class="compare" style="--split:50%"><img src="{b}" alt="Nytt konsept: {title}"><div class="before"><img src="{a}" alt="Faktisk Unreal-bilde: {title}"></div><div class="divider"></div><span class="tag left">Faktisk Unreal</span><span class="tag right">Nytt konsept</span></div>
    <label class="slider">Dra for å sammenligne <input type="range" min="0" max="100" value="50" aria-label="Sammenligning av {title}" oninput="this.closest('section').querySelector('.compare').style.setProperty('--split',this.value+'%')"></label>
    <div class="links"><a href="{a}" target="_blank">Åpne førbildet</a><a href="{b}" target="_blank">Åpne nytt konsept</a><a href="tidligere/{old}.png" target="_blank">Tidligere utkast</a></div></section>''')
new = ''.join(fig(f'nye/{a}.png',t) for t,_,a,_,_ in pairs)
new += fig('nye/06-npc-dialog.png', 'NPC-samtale / nattevakten i Caliche (konsept uten eget førbilde)')
old_names = [('title','Startmeny'),('settings','Settings'),('pause','Pause'),('load','Lasting'),('hud','Gameplay HUD'),('notebook','Notatbok / kit'),('document','Dokumentvisning'),('08-evidence-ui','Eldre evidence UI-konsept')]
old = ''.join(fig(f'tidligere/{n}.png',t) for n,t in old_names)
captures = json.loads((ROOT/'capture-manifest.json').read_text())
before = ''.join(fig(x['file'],Path(x['file']).stem) for x in captures)
nav = ''.join(f'<a href="#screen-{i}">{t}</a>' for i,(t,*_) in enumerate(pairs))
page = '''<!doctype html><html lang="no"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>SIGNAL / 47 · UI-konsepter</title><style>
:root{color-scheme:dark;--ink:#eee6d2;--amber:#dca768;--sage:#9cab95}*{box-sizing:border-box}body{margin:0;background:#101512;color:var(--ink);font:16px/1.6 system-ui,sans-serif}header,main,footer{max-width:1500px;margin:auto;padding:30px}header{padding-top:55px}h1{font-size:clamp(30px,5vw,66px);line-height:1.1;letter-spacing:-2px;margin:10px 0 18px}h1 span{color:#c35036}h2{font-size:27px;margin:0}p{max-width:850px;color:#bebfaf}small,.eyebrow{font:12px/1.5 monospace;letter-spacing:2px;color:var(--sage)}a{color:var(--amber);text-underline-offset:4px}nav{display:flex;gap:12px;flex-wrap:wrap;padding:18px 0}nav a{padding:8px 14px;border:1px solid #465044;text-decoration:none}section{scroll-margin-top:24px;margin:0 0 65px}.section-head{display:flex;justify-content:space-between;gap:24px;align-items:baseline}.section-head p{max-width:700px}.compare{position:relative;aspect-ratio:1672/941;background:#080b09;overflow:hidden;border:1px solid #465044}.compare>img,.before img{position:absolute;width:100%;height:100%;object-fit:contain;inset:0}.before{position:absolute;inset:0;clip-path:inset(0 calc(100% - var(--split)) 0 0)}.divider{position:absolute;left:var(--split);top:0;bottom:0;width:2px;background:var(--amber);pointer-events:none}.tag{position:absolute;top:12px;background:#0a100eeb;padding:5px 10px;font:12px monospace;border:1px solid #5a6455}.left{left:12px}.right{right:12px}.slider{display:flex;align-items:center;gap:18px;font:13px monospace;padding:18px 0;color:var(--sage)}input{flex:1;accent-color:var(--amber);cursor:ew-resize}.links{display:flex;gap:22px;flex-wrap:wrap;font-size:14px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:22px;margin-top:24px}figure{margin:0;background:#19211b;border:1px solid #354236}figure img{display:block;width:100%;aspect-ratio:16/9;object-fit:contain;background:#070a08}figcaption{padding:10px 15px;color:#c9c8b9;font-size:14px}.notice{border-left:2px solid var(--amber);padding:2px 18px;margin:22px 0 30px}footer{border-top:1px solid #354236;font-size:13px;color:#aab09e}@media(max-width:800px){header,main,footer{padding:20px}.section-head{display:block}.slider{display:block}input{width:100%;margin-top:12px}.grid{grid-template-columns:1fr}}
</style></head><body><header><div class="eyebrow">NEW MEXICO / 1986 / 24. OKTOBER 2026</div><h1>SIGNAL <span>/</span> 47</h1><h2>UI og menyer</h2><p>Seks nye konsepter med en mørkere, analog retning: ravlys, sotgrønt metall, papir og tomme arbeidsrom. Sammenlign dem med faktiske Unreal-bilder og de tidligere utkastene.</p><div class="notice"><p>De 21 førbildene er ferske runtime-opptak fra den pågående cloud-completion-testen 24. oktober, samlet fra eksisterende testkjøring uten å starte et konkurrerende spill. De viser testpakken, ikke en ny opptaksøkt fra installasjonen. Nye bilder er genererte konsepter og er ikke implementert i spillet.</p></div><nav>''' + nav + '''<a href="#new">Alle nye</a><a href="#old">Tidligere utkast</a><a href="#captures">Alle førbilder</a></nav></header><main>''' + ''.join(sections) + f'''<section id="new"><h2>Seks nye konsepter</h2><div class="grid">{new}</div></section><section id="old"><h2>Gjenfunne tidligere utkast</h2><p>Sju opprinnelige UI-/menybilder gjenopprettet fra historiske bildeutdata i Claudes samtalearkiv. Det åttende er det tidligere evidence UI-konseptet fra Concept11. Gamle datoer og tilstander er bevart som historisk referanse.</p><div class="grid">{old}</div></section><section id="captures"><h2>21 faktiske Unreal-bilder</h2><p>Startmeny, alle seks options-faner, HUD, pause, lagring/lasting, notatbok, konsoller, brett og dokument. Alle tilhørende opptakslogger inneholder «smoke test OK».</p><div class="grid">{before}</div></section></main><footer>Innebygd image_gen. Originale prompter: <a href="prompts.json">prompts.json</a>. Opptakskilder og SHA-256: <a href="capture-manifest.json">capture-manifest.json</a>. Arkivgjenoppretting: <a href="tidligere/recovery.json">recovery.json</a>. Bildene er inspirasjon til et senere UI-pass. Eksempelverdier, nye forhåndsvisninger, notatkort og knappetekster er konseptforslag. Ingen spillkode eller installasjon er endret.</footer></body></html>'''
(ROOT/'GALLERI.html').write_text(page)

# A quick visual index of the delivered concepts.
sheet=Image.new('RGB',(1500,3*445),'#101512');draw=ImageDraw.Draw(sheet)
for i,(title,_,name,_,_) in enumerate(pairs):
    im=Image.open(ROOT/'nye'/f'{name}.png').convert('RGB');im.thumbnail((730,411))
    x=(i%2)*750;y=(i//2)*445;sheet.paste(im,(x,y));draw.text((x+12,y+417),title,fill='#eee6d2')
sheet.save(ROOT/'NYE-KONSEPTER.jpg',quality=92)

for rel in __import__('re').findall(r'(?:src|href)="([^"#]+)"',page):
    if not (ROOT/rel).exists():raise RuntimeError(f'Missing gallery target {rel}')
for item in captures:
    assert hashlib.sha256((ROOT/item['file']).read_bytes()).hexdigest()==item['sha256']
assert all(x['smoke_ok'] for x in captures)
print('Gallery verified:',len(pairs),'concepts,',len(old_names),'earlier drafts,',len(captures),'runtime captures')
