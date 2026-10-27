from pathlib import Path
import re,html,ctypes,os,hashlib,argparse
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, PageBreak, Image, KeepTogether, HRFlowable, Preformatted
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.utils import ImageReader
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from PIL import Image as PILImage
# Arkiv-PDF-en er uforandret. Nye bygg går som standard til build/.
b=Path(__file__).resolve().parents[1]
repo=b.parents[1]
parser=argparse.ArgumentParser(description='Bygg en ny designbibel fra arkiverte kilder; original-PDF bevares.')
parser.add_argument('--output', type=Path, default=b/'build/SIGNAL-47-Designbibel.pdf')
args=parser.parse_args()
out=args.output.resolve()
if out == (b/'SIGNAL-47-Designbibel.pdf').resolve():
 parser.error('Arkiv-PDF-en skal bevares; velg en annen --output.')
out.parent.mkdir(parents=True,exist_ok=True)
cache_dir=b/'build/cache';cache_dir.mkdir(parents=True,exist_ok=True)
root=b/'assets'
v=b/'visuell-retning' 
pdfmetrics.registerFont(TTFont('DejaVu','/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'));pdfmetrics.registerFont(TTFont('DejaVuBold','/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'));pdfmetrics.registerFontFamily('DejaVu',normal='DejaVu',bold='DejaVuBold',italic='DejaVu',boldItalic='DejaVuBold')
styles=getSampleStyleSheet();styles.add(ParagraphStyle(name='TitleS',fontName='DejaVuBold',fontSize=25,leading=30,textColor=colors.HexColor('#294640'),spaceAfter=15));styles.add(ParagraphStyle(name='SubS',fontName='DejaVu',fontSize=11,leading=16,textColor=colors.HexColor('#58685f'),spaceAfter=12));styles.add(ParagraphStyle(name='H1S',fontName='DejaVuBold',fontSize=17,leading=22,textColor=colors.HexColor('#294640'),spaceBefore=12,spaceAfter=7,keepWithNext=True));styles.add(ParagraphStyle(name='H2S',fontName='DejaVuBold',fontSize=12,leading=16,textColor=colors.HexColor('#9b5139'),spaceBefore=10,spaceAfter=5,keepWithNext=True));styles.add(ParagraphStyle(name='BodyS',fontName='DejaVu',fontSize=9,leading=13.8,spaceAfter=6,splitLongWords=True));styles.add(ParagraphStyle(name='SmallS',fontName='DejaVu',fontSize=8,leading=11.5,textColor=colors.HexColor('#56615a'),spaceAfter=5));styles.add(ParagraphStyle(name='BulletS',parent=styles['BodyS'],leftIndent=13,firstLineIndent=-9));styles.add(ParagraphStyle(name='MonoS',fontName='DejaVu',fontSize=7.5,leading=10,leftIndent=10,spaceAfter=4))
story=[]
def P(t,style='BodyS'):return Paragraph(t,styles[style])
def clean(s):
 s=s.replace('—','-').replace('–','-').replace('\u2011','-')
 s=re.sub(r'!\[([^]]*)\]\([^)]*\)',r'\1',s);s=re.sub(r'\[([^]]+)\]\([^)]*\)',r'\1',s)
 s=html.escape(s);s=re.sub(r'\*\*(.+?)\*\*',r'<b>\1</b>',s);s=re.sub(r'`([^`]+)`',r'\1',s);return s

def svg_png(src,dest):
 r=ctypes.CDLL('librsvg-2.so.2'); c=ctypes.CDLL('libcairo.so.2');g=ctypes.CDLL('libgobject-2.0.so.0')
 r.rsvg_handle_new_from_file.argtypes=[ctypes.c_char_p,ctypes.c_void_p];r.rsvg_handle_new_from_file.restype=ctypes.c_void_p
 r.rsvg_handle_render_cairo.argtypes=[ctypes.c_void_p,ctypes.c_void_p];r.rsvg_handle_render_cairo.restype=ctypes.c_int
 c.cairo_image_surface_create.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_int];c.cairo_image_surface_create.restype=ctypes.c_void_p;c.cairo_create.argtypes=[ctypes.c_void_p];c.cairo_create.restype=ctypes.c_void_p;c.cairo_surface_write_to_png.argtypes=[ctypes.c_void_p,ctypes.c_char_p];c.cairo_destroy.argtypes=[ctypes.c_void_p];c.cairo_surface_destroy.argtypes=[ctypes.c_void_p];g.g_object_unref.argtypes=[ctypes.c_void_p]
 handle=r.rsvg_handle_new_from_file(str(src).encode(),None);assert handle
 w,h=(1600,760) if src.name=='fargepalett.svg' else (1600,800)
 surf=c.cairo_image_surface_create(0,w,h);ctx=c.cairo_create(surf);assert r.rsvg_handle_render_cairo(handle,ctx);assert c.cairo_surface_write_to_png(surf,str(dest).encode())==0
 c.cairo_destroy(ctx);c.cairo_surface_destroy(surf);g.g_object_unref(handle)

def add_image(path,title,caption):
 if not path.exists():raise FileNotFoundError(path)
 with PILImage.open(path) as im:
  im=im.convert('RGB'); im.thumbnail((2000,2000),PILImage.Resampling.LANCZOS);w,h=im.size
  cache=cache_dir/('image-'+hashlib.sha256(str(path).encode()).hexdigest()[:16]+'.jpg');im.save(cache,format='JPEG',quality=86,optimize=True)
 path=cache
 maxw=176*mm;maxh=170*mm;k=min(maxw/w,maxh/h);img=Image(str(path),width=w*k,height=h*k,hAlign='CENTER')
 story.append(KeepTogether([P(clean(title),'H2S'),img,Spacer(1,5),P(clean(caption),'SmallS'),Spacer(1,10)]))
def markdown(path):
 lines=path.read_text().splitlines();i=0;in_code=False;mermaid=False
 while i<len(lines):
  line=lines[i].strip();i+=1
  if line.startswith('```'):
   if not in_code:in_code=True;mermaid='mermaid' in line
   else:in_code=False;mermaid=False
   continue
  if in_code:
   if not mermaid and line:story.append(P(clean(line),'MonoS'))
   continue
  if not line or line.startswith('!['):continue
  if line.startswith('|'):
   rows=[line]
   while i<len(lines) and lines[i].strip().startswith('|'):rows.append(lines[i].strip());i+=1
   cells=lambda r:[c.strip() for c in r.strip('|').split('|')]
   hdr=cells(rows[0]); data=rows[1:]
   if data and all(re.fullmatch(r':?-+:?',x.replace(' ','')) for x in cells(data[0])):data=data[1:]
   for row in data:
    vals=cells(row); pairs=[]
    for k,x in enumerate(vals):
     if not x:continue
     if k==0:pairs.append('<b>'+clean(x)+'</b>')
     else:pairs.append((clean(hdr[k])+': ' if k<len(hdr) else '')+clean(x))
    story.append(P(' | '.join(pairs),'BodyS'))
   continue
  if line.startswith('#'):
   h=len(line)-len(line.lstrip('#'));title=line[h:].strip();story.append(P(clean(title),'H1S' if h<=2 else 'H2S'));continue
  if line.startswith('- '):story.append(P('• '+clean(line[2:]),'BulletS'));continue
  if re.match(r'^\d+\. ',line):story.append(P(clean(line),'BulletS'));continue
  story.append(P(clean(line),'BodyS'))

def footer(c,d):
 c.setStrokeColor(colors.HexColor('#d9d3c4'));c.line(18*mm,17*mm,192*mm,17*mm);c.setFont('DejaVu',7);c.setFillColor(colors.HexColor('#69756b'));c.drawString(18*mm,12*mm,'SIGNAL / 47  ·  DESIGNBIBEL  ·  KONSEPT OG PRODUKSJON');c.drawRightString(192*mm,12*mm,str(d.page))

story.append(Spacer(1,20*mm));story.append(P('SIGNAL / 47','TitleS'));story.append(P('DESIGNBIBEL · VERDEN, HISTORIE OG VISUELL RETNING','SubS'));story.append(P('Arbeidsversjon 27. oktober 2026 · inneholder hele historien og spoilere','BodyS'))
add_image(v/'caliche-kveld.png','CALICHE / NEW MEXICO, 1986','Visuelt konsept. Skilt, kjøretøy og bygningsrekkefølge er ikke fasit for dagens spill.');story.append(PageBreak())
story.append(P('Slik brukes boken','H1S'));story.append(P('Boken samler designbibelen, den nyere oppdragsplanen, kildebaserte kart og visuelle referanser. Kildene skiller eksisterende spill, historiske planer og forslag for senere produksjon. Konseptbilder viser ønsket uttrykk, ikke ferdig geometri eller bevis for implementering.','BodyS'));story.append(P('Del 1  Visuelle referanser og stedskart','H2S'));story.append(P('Del 2  Hele designbibelen, historien og de 32 oppdragskortene','H2S'));story.append(P('Del 3  Visuell retning, farger og produksjonsprinsipper','H2S'));story.append(PageBreak());story.append(P('DEL 1 / VERDEN OG VISUELLE REFERANSER','TitleS'))
for title,path,cap in [
 ('Caliche - samlet by',v/'caliche-luftperspektiv.png','Konsept: naturligere tomter og terreng. Plasseringer må avstemmes mot kildekart.'),
 ('SARO - observatoriet',v/'saro-luftperspektiv.png','Konsept: ankomst og miljø. Antenner og bygninger følger ikke eksakt spillgeometri.'),
 ('SARO - kontrollrom, mørkerom, arkiv og Dale-kontor',v/'tillegg/saro-interior.png','Fire nye interiørkonsepter; inventar og romforbindelser må kontrolleres i spillet.'),
 ('Sierra Motor Court - eksteriør',root/'konseptbilder-2026-10-27/01-sierra-motor-court-eksterior.png','Eksisterende konsept for resepsjon, romfløy og parkering.'),
 ('Motellet - rom 6',v/'tillegg/motell-rom6.png','Ny referanse: rom 6 og gangvei. Arkitektur og oppdragsrekvisitter må tilpasses.'),
 ('STATION 01 - feltstasjon',v/'tillegg/station01.png','Nytt konsept for eksteriør og interiør. Trekledningen avviker fra dagens stucco-referanse; oppstilling og bevis må følge spillkildene.'),
 ('Bensinstasjon, servicested og gammel vei',v/'tillegg/veier-og-service.png','Fire forslag. Geometri og veivalg må avstemmes mot faktiske spillområder.'),
 ('Mesa Diner - nytt interiørmål',root/'konseptbilder-2026-10-27/02-mesa-diner-interior.png','Konsept for båser, disk, materialer og lys; bevar funksjonell gangakse.'),
 ('Besøksparkering - eksisterende modellrender',root/'resultater/ankomst-og-kontor/besoksparkering.png','Modellrender, ikke konsept eller dokumentasjon på gjeldende spillbygg.'),
 ('Dale-kontor - eksisterende modellrender',root/'resultater/ankomst-og-kontor/dales-kontor.png','Modellrender, ikke dokumentasjon på gjeldende spillbygg.'),
 ('Ku og gårdsmiljø - levende dyr',repo/'unreal/ArtSource/Cows/Concepts/ku-levende.png','Figurkonsept; ikke en oppmålt gårdsplan.'),
 ('Ku - etter bortføring',repo/'unreal/ArtSource/Cows/Concepts/ku-dod-etter-bortforing.png','Figurkonsept til miljøfortelling.'),
 ('Notatbok og undersøkelse',root/'konseptbilder/2026-10-24-ui/nye/04-notatbok.png','UI-konsept; ikke bevis for navigasjon i spillet.'),
]:add_image(path,title,cap)
for fn,title in [('01-mesa-diner.png','Mesa Diner'),('02-sheriffkontor.png','Sheriffkontoret'),('03-radiostudio-1420-am.png','Radiostudio 1420 AM'),('04-suvenirbutikk.png','Suvenirbutikk'),('05-vaskeri.png','Vaskeri'),('06-plaza-kino.png','Plaza-kino'),('07-forbutikk.png','Fôrbutikk'),('08-kirke.png','Kirke')]:
 add_image(repo/'unreal/Concepts/Caliche-2026-10-23'/fn,title,'Eksisterende konseptark: eksteriør og foreslått interiør. Interiøret er ikke nødvendigvis tilgjengelig i spillet.')
for fn,title in [('control-room-v1.png','SARO kontrollrom - tidligere mål'),('Concept11/04-photolab.png','Fotolab - tidligere mål'),('Concept11/05-old-survey-station.png','Gammel målestasjon - tidligere mål'),('Concept11/06-motel-witness.png','Motell og vitne - tidligere mål'),('Concept11/02-light-event.png','Lysfenomen - skrekkretning')]:add_image(repo/'Docs/VisualTargets'/fn,title,'Tidligere visuell referanse; ikke runtimebevis.')
for svg,title in [('01-caliche-na.svg','Kart: Caliche i dagens kilder'),('02-saro-na.svg','Kart: SARO i dagens kilder'),('03-layoutforslag.svg','Skisse: mulige senere justeringer'),('visuell-retning/fargepalett.svg','Arbeidspalett')]:
 src=b/svg; dest=cache_dir/(src.stem+'.png');svg_png(src,dest);add_image(dest,title,'Kildekart eller forslag: se tilhørende beskrivelse. Ikke et oppmålt bygg eller bevis på endringer i Unreal.')
story.append(PageBreak());story.append(P('DEL 2 / DESIGNBIBEL OG OPPDRAG','TitleS'));markdown(b/'DESIGNBIBEL.md')
story.append(PageBreak());story.append(P('DEL 3 / VISUELL RETNING','TitleS'));markdown(v/'VISUELL-BIBEL.md')
story.append(PageBreak());story.append(P('DEL 4 / KART OG VIDERE ARBEID','TitleS'));markdown(b/'KART.md')
doc=SimpleDocTemplate(str(out),pagesize=A4,rightMargin=18*mm,leftMargin=18*mm,topMargin=17*mm,bottomMargin=23*mm,title='SIGNAL / 47 - Designbibel',author='Signal 47');doc.build(story,onFirstPage=footer,onLaterPages=footer);print(out)
