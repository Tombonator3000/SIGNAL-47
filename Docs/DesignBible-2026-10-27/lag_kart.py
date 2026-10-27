from pathlib import Path
import json,html,math,argparse
D=Path(__file__).resolve().parent
repo=D.parents[1]
parser=argparse.ArgumentParser(description='Generer nye kildekart uten å overskrive de arkiverte SVG-ene.')
parser.add_argument('--manifest',type=Path,default=repo/'unreal/Signal47/Content/WebData/manifest.json')
parser.add_argument('--output-dir',type=Path,default=D/'build/kart')
args=parser.parse_args()
if args.output_dir.resolve()==D.resolve():
 parser.error('Arkivkartene skal bevares; velg en annen --output-dir.')
D=args.output_dir.resolve();D.mkdir(parents=True,exist_ok=True)
if not args.manifest.is_file():
 parser.error('Sceneeksport mangler. Oppgi eksisterende manifest med --manifest; dette scriptet starter ingen Unreal-jobb.')
j=json.loads(args.manifest.read_text())
# Planar coordinates from the exported scene, column-major parent transforms.
nodes=j['nodes']; cache={}
def matrix(i):
 if i in cache:return cache[i]
 n=nodes[i]; m=n['m']; parent=n['p']; a=matrix(parent) if parent>=0 else [1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]
 cache[i]=[sum(a[k*4+r]*m[c*4+k] for k in range(4)) for c in range(4) for r in range(4)];return cache[i]
def pos(h):
 m=matrix(j['handles'][h]['node']);return m[12],m[14]
def esc(s):return html.escape(str(s))
def start(title,subtitle):
 return [f'<svg xmlns="http://www.w3.org/2000/svg" width="1600" height="800" viewBox="0 0 1600 800"><rect width="1600" height="800" fill="#ede8dc"/><text x="64" y="70" font-family="sans-serif" font-size="37" font-weight="700" fill="#193d3d">{esc(title)}</text><text x="64" y="105" font-family="sans-serif" font-size="17" fill="#555d54">{esc(subtitle)}</text>']
def txt(x,y,s,size=15,fill='#263e3d',anchor='start',weight='normal'):return f'<text x="{x:.1f}" y="{y:.1f}" font-family="sans-serif" font-size="{size}" text-anchor="{anchor}" font-weight="{weight}" fill="{fill}">{esc(s)}</text>'
def dot(x,y,n,color='#a14d34'):return f'<circle cx="{x:.1f}" cy="{y:.1f}" r="15" fill="{color}" stroke="#fff" stroke-width="2"/>'+txt(x,y+5,n,14,'white','middle','bold')
# Caliche map: Z along horizontal, X on vertical, equal metres per pixel.
a=start('CALICHE / DAGENS PLASSERINGER','Kildeavlest plan · prikker er ankerpunkter, ikke bygningsomriss · orientering etter kildeaksene')
s=4.0; ox=135; oy=380
P=lambda x,z:(ox+(z-1980)*s,oy+(x-8000)*s)
a.append('<rect x="110" y="170" width="1350" height="430" rx="20" fill="#dcd4bf"/>')
y1=P(7995,1980)[1];y2=P(8005,1980)[1]
a.append(f'<rect x="{P(8000,1980)[0]}" y="{y1}" width="{(2300-1980)*s}" height="{y2-y1}" fill="#656c67"/>')
a.append(f'<line x1="{P(8000,1980)[0]}" x2="{P(8000,2300)[0]}" y1="{oy}" y2="{oy}" stroke="#e6d8a8" stroke-width="3" stroke-dasharray="20 18"/>')
items=[('Mesa Diner','world.diner.group'),('1420 AM','world.town.proxies.studio'),('Bensinstasjon','world.town.proxies.gasDoor'),('Suvenirbutikk','world.town.proxies.souvenirDoor'),('Kirke','world.town.proxies.churchDoor'),('Sheriff','world.town.proxies.sheriffDoor'),('Vaskeri','world.town.proxies.laundromat'),('Kino','world.town.proxies.cinema'),('Fôrbutikk','world.town.proxies.feed')]
for i,(name,h) in enumerate(items,1):
 x,z=pos(h);px,py=P(x,z);a.append(dot(px,py,str(i)));a.append(f'<line x1="{px:.0f}" y1="{py:.0f}" x2="{px:.0f}" y2="{py+(-44 if x<8000 else 44):.0f}" stroke="#836554"/>')
 a.append(txt(px,py+(-53 if x<8000 else 62),str(i)+' '+name,15,'#263e3d','middle','bold'))
a.append(txt(130,660,'Lokal akse: Z øker mot høyre; X øker nedover. Byens gruppe har X = 8000.',17))
a.append(txt(130,690,'Veibanen er forenklet; terreng, dører, tomtegrenser og bygningers omriss er ikke oppmålt her.',17))
a.append('<line x1="1170" y1="725" x2="1370" y2="725" stroke="#193d3d" stroke-width="5"/>');a.append(txt(1270,750,'50 m på begge akser',16,anchor='middle'));a.append('</svg>');(D/'01-caliche-na.svg').write_text(''.join(a))
# SARO map, source site-local X/Z, same scale.
a=start('SARO / DAGENS ANKERPUNKTER','Kildeavlest situasjonskart · X mot høyre, Z nedover · ikke ferdig plantegning')
s=1.5; ox=590; oy=625
P=lambda x,z:(ox+x*s,oy+z*s)
a.append('<rect x="110" y="145" width="1040" height="565" rx="20" fill="#dcd4bf"/>')
for dish in j['dishes']:
 x,y=P(dish['x'],dish['z']);a.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="8" fill="#6b817d" stroke="#fff" stroke-width="1"/>')
 if dish['id'] in ['S-01','S-02','S-03','S-04','S-05','S-11']:a.append(txt(x+11,y-7,dish['id'],13))
for name,x,z,num in [('Kontrollrom, origo',0,0,'1'),('B-12 kontroll',8.35,-16.9,'2'),('Besøksparkering',-13,15.5,'3'),('Inngang / hengsel',-6.27,5.22,'4'),('Servicebil',8.9,8,'5'),('SARO-fasade, senter',29.6,0,'6')]:
 px,py=P(x,z); a.append(f'<circle cx="{px:.1f}" cy="{py:.1f}" r="4" fill="#a14d34" stroke="white" stroke-width="1"/>'); lx=715+(int(num)%2)*85; ly=380+int(num)*31; a.append(f'<path d="M {px:.1f} {py:.1f} L {lx} {ly}" stroke="#a14d34" stroke-width="1" fill="none"/>'); a.append(dot(lx,ly,num));a.append(txt(1190,220+int(num)*42,num+'  '+name,18,weight='bold'))
a.append(txt(1180,520,'• 27 antenner fra manifestet',18));a.append(txt(1180,550,'• Sirklene viser antennesentre',18));a.append(txt(1180,580,'• Ingen romvegger er oppmålt',18));a.append(txt(1180,610,'• Ny inngang er fortsatt under arbeid',18));a.append('<line x1="130" y1="680" x2="280" y2="680" stroke="#193d3d" stroke-width="5"/>');a.append(txt(205,706,'100 m',16,anchor='middle'));a.append('</svg>');(D/'02-saro-na.svg').write_text(''.join(a))
# Conceptual future sketch, deliberately not claiming coordinates.
a=start('CALICHE OG SARO / MULIG MER NATURLIG STRUKTUR','Forslag til senere Unreal-arbeid · skjematisk · ikke målestokk og ingen bygninger er flyttet')
a.append('<path d="M85 462 C340 458 375 400 600 410 S1000 450 1510 417" fill="none" stroke="#676e69" stroke-width="58"/><path d="M85 462 C340 458 375 400 600 410 S1000 450 1510 417" fill="none" stroke="#eadcb2" stroke-width="3" stroke-dasharray="18 17"/>')
a.append('<path d="M320 440 C300 350 265 300 230 250 M1130 430 C1140 510 1200 570 1290 605" fill="none" stroke="#aca797" stroke-width="23"/>')
boxes=[(120,165,235,80,'SARO','Avkjørsel, parkering, service'),(405,270,200,70,'MOTELL','Lav bebyggelse, rolig tun'),(765,255,210,70,'DINER + STASJON','Kryss, parkering og liv'),(1000,255,210,70,'BYKJERNE','Sheriff, butikk, kino'),(1190,595,265,70,'BAKRE SERVICE','Levering, gårdsrom, jordveier')]
for x,y,w,h,t,sub in boxes:
 a.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="10" fill="#f7f3e9" stroke="#617d75" stroke-width="2"/>');a.append(txt(x+w/2,y+31,t,18,anchor='middle',weight='bold'));a.append(txt(x+w/2,y+54,sub,13,anchor='middle'))
a.append(txt(610,590,'Konsentrer handel ved knutepunktet; la mellomrom, terreng og gårdsveier',20));a.append(txt(610,620,'forklare avstanden til observatoriet. Bevar eksisterende oppdragsruter.',20));a.append(txt(90,740,'Dette er et arbeidsforslag som må testes mot faktisk kjøring, kollisjon, dialoger og siktlinjer.',16));a.append('</svg>');(D/'03-layoutforslag.svg').write_text(''.join(a))
print('Created three SVG maps; town anchors',len(items),'dishes',len(j['dishes']))
