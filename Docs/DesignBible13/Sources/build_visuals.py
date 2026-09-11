"""Editable vector design sketches. These are not Unity screens or surveyed maps."""
from pathlib import Path
from reportlab.graphics.shapes import Drawing,Rect,Line,String,Polygon,Circle
from reportlab.graphics import renderSVG,renderPDF
from reportlab.lib.colors import HexColor
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
import json,math
ROOT=Path(__file__).resolve().parents[1]
for name,file in [('DV','DejaVuSans.ttf'),('DVB','DejaVuSans-Bold.ttf')]:
 if name not in pdfmetrics.getRegisteredFontNames():pdfmetrics.registerFont(TTFont(name,'/usr/share/fonts/truetype/dejavu/'+file))
C={'bg':'#101b1b','panel':'#1a2a28','line':'#50645d','ink':'#eeeade','muted':'#b7c5bd','gold':'#efb56e','paper':'#e1dccb','dark':'#18251f','red':'#f7a195','green':'#9ac5a5'}
class Board:
 def __init__(self,title,subtitle,kind='DESIGN SKETCH',w=1280,h=800):
  self.d=Drawing(w,h);self.w=w;self.h=h;self.bounds=[];self.rect(0,0,w,h,'bg');self.text(48,48,'SIGNAL / 47',22,'gold',True);self.text(48,97,title,38,'ink',True);self.text(48,132,subtitle,19,'muted');self.line(48,153,w-48,153)
  self.line(48,h-60,w-48,h-60);self.text(48,h-29,kind+' / v0.1 / PROPOSED - NOT GAMEPLAY',17,'muted');self.text(w-48,h-29,'1986',18,'gold',align='right')
 def col(self,c):return HexColor(C.get(c,c)) if c else None
 def rect(self,x,y,w,h,fill='panel',stroke=None,r=0):self.d.add(Rect(x,self.h-y-h,w,h,rx=r,ry=r,fillColor=self.col(fill),strokeColor=self.col(stroke),strokeWidth=1.3))
 def line(self,x,y,xx,yy,c='line',width=1.3):self.d.add(Line(x,self.h-y,xx,self.h-yy,strokeColor=self.col(c),strokeWidth=width))
 def text(self,x,y,s,size=24,c='ink',bold=False,align='left'):
  font='DVB' if bold else 'DV';w=pdfmetrics.stringWidth(s,font,size);left=x if align=='left' else x-w if align=='right' else x-w/2
  self.bounds.append({'text':s,'x':round(left,2),'right':round(left+w,2),'baseline_y':y,'size':size})
  self.d.add(String(x,self.h-y,s,fontName=font,fontSize=size,fillColor=self.col(c),textAnchor={'left':'start','right':'end','center':'middle'}[align]))
 def wrap(self,x,y,s,width,size=22,c='muted',bold=False,leading=None):
  words=s.split();line='';leading=leading or size*1.45
  for word in words:
   test=(line+' '+word).strip()
   if pdfmetrics.stringWidth(test,'DVB' if bold else 'DV',size)>width and line:self.text(x,y,line,size,c,bold);y+=leading;line=word
   else:line=test
  if line:self.text(x,y,line,size,c,bold);y+=leading
  return y
 def arrow(self,x,y,xx,yy,c='gold',width=2):
  self.line(x,y,xx,yy,c,width);a=math.atan2(yy-y,xx-x);pts=[]
  for px,py in [(xx,yy),(xx-10*math.cos(a)+5*math.sin(a),yy-10*math.sin(a)-5*math.cos(a)),(xx-10*math.cos(a)-5*math.sin(a),yy-10*math.sin(a)+5*math.cos(a))]:pts.extend([px,self.h-py])
  self.d.add(Polygon(pts,fillColor=self.col(c),strokeColor=None))
 def button(self,x,y,w,label,active=False):
  assert pdfmetrics.stringWidth(label,'DVB' if active else 'DV',22)<=w-34, ('button text overflow',label,w)
  self.rect(x,y,w,48,'gold' if active else 'panel','gold' if active else 'line',3);self.text(x+17,y+31,label,22,'dark' if active else 'ink',active)
 def tag(self,x,y,label,c='gold'):
  w=pdfmetrics.stringWidth(label,'DVB',16)+22;self.rect(x,y,w,29,c);self.text(x+11,y+20,label,16,'dark',True)
 def node(self,x,y,w,h,head,body,new=False):
  self.rect(x,y,w,h,'panel','gold' if new else 'line',5);self.text(x+18,y+34,head,24,'gold' if new else 'ink',True);self.wrap(x+18,y+66,body,w-36,19)

def make_all():
 boards={}
 b=Board('Tre steder. Én sammenhengende sak.','Verdenskart / topologi og hovedrute / ingen geografisk målestokk','PROPOSED MAP')
 b.node(48,218,360,285,'SARO','K1 / K2 / K5 / K6 / EPILOG\nKontrollrom, fotolab, arkiv og servicegård. Det kjente knutepunktet.',False)
 b.node(460,218,360,285,'MÅLESTASJONEN','K3 / Ett rom, transitflate, fastmerker og kabelbrudd. Rekonstruer forsøket.',True)
 b.node(872,218,360,285,'SIERRA MOTOR COURT','K4 / Kontor og rom 6. Møt Nora og prøv rapporten mot vitnet.',True)
 b.arrow(408,320,460,320);b.arrow(820,320,872,320);b.arrow(1052,525,1052,568);b.arrow(1052,568,228,568);b.arrow(228,568,228,525)
 b.text(640,609,'RETUR TIL SARO: NYE PRØVER / INFORMERT SLUTTVALG',23,'gold',True,'center')
 b.wrap(48,655,'Områdebytte via tjenestebil og oppdragskart. Kort hoppbar reiseintro; ingen kjørefysikk, drivstoff eller åpen verden.',1160,22)
 boards['map-01-world']=b
 b=Board('SARO / fungerende kjerne og foreslått utvidelse','Topologisk romplan / plasseringer skal tilpasses faktisk scene / 3 interiører','PROPOSED MAP')
 b.node(58,204,290,154,'FOTOLAB','Eksponering → framkalling → undersøkelse. Eksisterende.',False)
 b.node(400,204,335,154,'KONTROLLROM / C','Mottaker, telefon, utskrift og finaleprotokoll. Eksisterende.',False)
 b.node(792,204,360,154,'ARKIV','Nytt lite rom: to protokoller, referansekart og kontaktspor.',True)
 b.arrow(348,275,400,275);b.arrow(735,275,792,275)
 b.node(400,435,335,169,'SERVICEGÅRD','Eksisterende hovedganglinje og port. Reisepunkt ved bilen.',False)
 b.arrow(565,358,565,435);b.line(348,328,374,328);b.line(374,328,374,476);b.arrow(374,476,400,476)
 b.node(58,435,255,169,'S-03','Motorlogg og første eksponering.',False)
 b.node(837,435,315,169,'B-12 / B','Kontrollreferanse, svarprøve og observasjonspunkt.',False)
 b.arrow(400,562,313,562);b.arrow(735,510,837,510)
 b.tag(58,660,'EKSISTERENDE FORBINDELSE','green');b.tag(467,660,'FORESLÅTT NYTT ROM','gold')
 b.text(58,711,'A ligger ved målestasjonen. Kartet viser forbindelser, ikke oppmålte avstander eller nye azimutverdier.',19,'muted')
 boards['map-02-saro']=b
 b=Board('To små steder med hver sin oppgave','Målestasjon og motell / foreslåtte gangsløyfer / alle andre dører er kulisse','PROPOSED MAP')
 b.rect(48,192,565,470,'panel','line',4);b.rect(642,192,590,470,'panel','line',4)
 b.text(72,229,'MÅLESTASJON / K3',26,'gold',True);b.text(666,229,'SIERRA MOTOR COURT / K4',26,'gold',True)
 b.node(72,266,255,144,'Hytte','Benk, opptak og gammel protokoll.',True);b.node(366,266,220,144,'Transit / A','Sammenlign fastmerker.',True)
 b.node(72,474,255,122,'Kabelbrudd','Nærfoto og fysisk brudd.',True);b.node(366,474,220,122,'Fastmerker','Lokale nullprøver.',True)
 b.arrow(327,338,366,338);b.arrow(476,410,476,474);b.arrow(366,535,327,535);b.arrow(185,474,185,410)
 b.node(666,266,248,144,'Kontor','Beskjed, konvolutt, vei til Nora.',True);b.node(957,266,248,144,'Rom 6','Samtale og tre fysiske bevis.',True)
 b.arrow(914,338,957,338);b.rect(666,472,539,124,'bg','line');b.text(686,509,'Parkering / gangvei / returreise',22,'ink',True);b.wrap(686,546,'Basseng, biler og andre rom er miljø, ikke egne oppgaver.',494,20)
 b.arrow(791,472,791,410);b.arrow(1081,410,1081,472)
 b.wrap(48,700,'Historien bestemmer adgang. Ingen skjulte nøkler, ekstra hotellrom eller tom terrengvandring legges til for å fylle tid.',1170,20)
 boards['map-03-local-areas']=b
 b=Board('18 oppgaver med tydelige porter','Hovedrute / to lokale rekkefølgevalg / se registeret for bevis og feiltilbakemelding','DESIGN GRAPH')
 groups=[('K1 / 50 min',['P01 Mottaker','P02 S-03','P03 B-12']),('K2 / 45 min',['P04 Rapport','P05 Kart']),('K3 / 60 min',['P06 Fastmerke','P07 Nullprøve','P08 Kabel','P09 Tidslogg']),('K4 / 45 min',['P10 Vis bevis','P11 Motiv','P12 Stemme']),('K5 / 65 min',['P13 Svar','P14 Geometri','P15 Maskering']),('K6 / 55 min',['P16 Valg','P17 Utfør'])]
 for i,(title,tasks) in enumerate(groups):
  x=48+i*200;b.rect(x,200,182,385,'panel','line',5);b.text(x+14,235,title,21,'gold',True)
  for j,label in enumerate(tasks):
   y=270+j*70;b.rect(x+12,y,158,52,'bg','line',3);b.text(x+21,y+33,label,18,'ink')
   if j<len(tasks)-1:
    if (i,j) in [(2,0),(3,1)]: b.text(x+158,y+68,'↕',19,'gold')
    else:b.arrow(x+91,y+52,x+91,y+70,'line')
  if i<5:b.arrow(x+182,425,x+200,425)
 b.tag(48,615,'VALGFRI REKKEFØLGE');b.text(303,637,'P06 ↔ P07 og P11 ↔ P12. Begge delene kreves ved neste port.',20,'muted')
 b.text(48,694,'P18 / EPILOG / 10 min',23,'gold',True);b.text(385,694,'Ferdig rapport og konsekvens. Totalbudsjett: 330 minutter.',22,'ink')
 boards['map-04-evidence-flow']=b
 # Menu concepts
 b=Board('THE ERASED NIGHT','Main menu / example case data / returning player','UI SKETCH')
 b.text(75,225,'SIGNAL / 47',59,'ink',True)
 for i,t in enumerate(['CONTINUE','NEW CASE','LOAD CASE','SETTINGS','CREDITS','QUIT']):b.button(78,266+i*66,362,t,i==0)
 b.rect(535,204,662,470,'panel','line',5);b.tag(565,230,'CASE 01');b.text(565,313,'THE AMENDED PROTOCOL',29,'ink',True);b.text(565,355,'SARO / Archive',25,'muted');b.text(565,400,'PLAYTIME 01:28:12',22,'muted');b.text(565,438,'LAST SAVED: TODAY, 21:14',22,'muted')
 b.line(565,474,1166,474);b.wrap(565,519,'Continue from your latest checkpoint. Your original photographs stay with this case.',570,24);b.text(565,643,'ENTER  SELECT        ESC  BACK',19,'gold')
 boards['ui-01-start']=b
 b=Board('LOAD CASE','Select a case, then choose a checkpoint. Example data, not an actual save file.','UI SKETCH')
 for i,(title,sub) in enumerate([('CASE 01 / NIGHT SHIFT','SARO · 01:28:12'),('CASE 02 / EMPTY','Start a new investigation'),('CASE 03 / EMPTY','Start a new investigation')]):
  y=193+i*145;b.rect(48,y,335,122,'panel','gold' if i==0 else 'line',5);b.text(67,y+40,title,23,'gold' if i==0 else 'ink',True);b.wrap(67,y+79,sub,295,20)
 b.rect(418,193,814,398,'panel','line',5);b.text(444,235,'CHECKPOINTS / CASE 01',26,'ink',True)
 for i,(title,detail) in enumerate([('AUTO 03 / Archive entered','Today 21:14 · SARO · 01:28:12'),('MANUAL 01 / Local report filed','Today 21:06 · SARO · 01:20:04'),('AUTO 02 / B-12 print collected','Today 20:59 · SARO · 01:13:09')]):
  y=264+i*98;b.rect(440,y,768,84,'bg','gold' if i==0 else 'line');b.text(458,y+32,title,23,'gold' if i==0 else 'ink',True);b.text(458,y+62,detail,20,'muted')
 b.text(444,575,'Original photographs: available',20,'green');b.button(418,624,300,'LOAD CHECKPOINT',True);b.button(741,624,190,'BACK');b.wrap(964,641,'Future chapters stay hidden.',250,19)
 boards['ui-02-load']=b
 b=Board('PAUSED','Save / resume flow. Game time and timed story sequences remain paused.','UI SKETCH')
 for i,t in enumerate(['RESUME','SAVE CASE','LOAD CASE','SETTINGS','MAIN MENU','QUIT']):b.button(48,207+i*70,340,t,i==1)
 b.rect(438,198,794,445,'panel','line',5);b.tag(467,223,'MANUAL SAVE');b.text(467,300,'CASE 01 / NIGHT SHIFT',31,'ink',True);b.text(467,345,'SARO / Archive',25,'muted')
 b.text(467,393,'SAVE SLOT',18,'gold',True);b.button(467,411,230,'01 / REPORT',True);b.button(712,411,230,'02 / EMPTY');b.button(957,411,230,'03 / EMPTY')
 b.line(467,493,1198,493);b.text(467,535,'READY TO SAVE',23,'green',True);b.wrap(467,575,'Writes a new snapshot. Original exposures are preserved.',710,23)
 b.button(438,672,300,'SAVE CHECKPOINT',True);b.text(769,703,'ESC / Return to pause menu',21,'muted')
 boards['ui-03-save']=b
 b=Board('CASE FILE / PHOTOGRAPHS','Unmarked comparison view. Illustrative layout; in-game panels load original exposures.','UI SKETCH')
 tabs=['OBSERVATIONS','PHOTOGRAPHS','MAP','HYPOTHESES']
 for i,t in enumerate(tabs):b.button(48+i*296,179,276,t,i==1)
 for i,(title,meta) in enumerate([('E03 / S-03','EXPOSURE 01 · ORIGINAL'),('E04 / B-12','EXPOSURE 02 · CONTROL')]):
  x=48+i*600;b.rect(x,252,584,327,'#b9b9ae','line',3);b.rect(x+23,278,538,230,'#3a4542')
  # Schematic panel, deliberately not an invented evidentiary photograph.
  b.line(x+23,451,x+561,451,'#8a948e',2);b.line(x+211,458,x+303,319,'#b8c0b8',5);b.line(x+303,319,x+446,455,'#b8c0b8',5);b.line(x+190,374,x+407,399,'#b8c0b8',4)
  b.text(x+23,545,title,24,'dark',True);b.text(x+563,571,meta,16,'dark',align='right')
 b.button(48,611,220,'ZOOM');b.button(285,611,220,'MARK DETAIL');b.button(522,611,220,'RESET VIEW');b.button(760,611,220,'OBSERVATIONS');b.text(48,704,'Select a detail to add your observation. The game does not mark the answer for you.',22,'muted')
 boards['ui-04-evidence']=b
 b=Board('SETTINGS','Readable before starting a case. Example values; keyboard focus remains visible.','UI SKETCH')
 for i,t in enumerate(['ACCESSIBILITY','AUDIO','CONTROLS','DISPLAY']):b.button(48,196+i*72,300,t,i==0)
 b.rect(392,195,840,462,'panel','line',5)
 rows=[('TEXT SIZE','125%'),('SUBTITLES','ON / SPEAKER NAMES'),('SUBTITLE BACKGROUND','HIGH CONTRAST'),('CAMERA SHAKE','OFF'),('HEAD MOTION','OFF'),('SCREEN FLICKER','REDUCED')]
 for i,(k,v) in enumerate(rows):
  y=234+i*62;b.text(419,y,k,21,'ink',True);b.rect(806,y-28,394,43,'bg','line',2);b.text(824,y,v,20,'gold')
 b.button(392,675,262,'SAVE AND CLOSE',True);b.button(675,675,236,'RESET TAB');b.text(938,707,'ESC / Back',20,'muted')
 boards['ui-05-settings']=b
 b=Board('CHECKPOINT RECOVERY','Load failure state / preserve originals / no silent restart','UI SKETCH')
 b.rect(178,199,924,486,'panel','red',6);b.tag(214,228,'CHECKPOINT NEEDS RECOVERY','red');b.text(214,306,'The latest checkpoint could not be read.',31,'ink',True)
 b.wrap(214,355,'Your previous checkpoint and original photographs have been kept. A verified recovery copy is available.',829,25)
 b.rect(214,434,850,100,'bg','line',4);b.text(234,472,'RECOVERY COPY / CASE 01',23,'gold',True);b.text(234,510,'SARO · Today 21:06 · Playtime 01:20:04',22,'muted')
 b.button(214,576,296,'LOAD RECOVERY',True);b.button(532,576,233,'TRY AGAIN');b.button(787,576,277,'BACK TO CASES');b.text(214,664,'The unreadable file is preserved. Loading recovery may lose newer progress.',19,'muted')
 boards['ui-06-load-recovery']=b
 return boards

def export():
 boards=make_all();out=ROOT/'Visuals';out.mkdir(exist_ok=True);scratch=ROOT.parents[1]/'Artifacts/Design13/VectorPDF';scratch.mkdir(parents=True,exist_ok=True)
 checks=[]
 for name,b in boards.items():
  outside=[t for t in b.bounds if t['x']<0 or t['right']>1280 or t['baseline_y']<0 or t['baseline_y']>800]
  if outside:raise ValueError((name,outside))
  renderSVG.drawToFile(b.d,str(out/(name+'.svg')));renderPDF.drawToFile(b.d,str(scratch/(name+'.pdf')))
  checks.append({'file':name+'.svg','width':1280,'height':800,'text_items':len(b.bounds),'text_bounds':'PASS','scope':'Vector layout bounds, not a Unity UI interaction test'})
 (ROOT/'Sources/vector-layout-check.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2)+'\n')
 print(json.dumps({'boards':len(boards),'svg_directory':str(out),'pdf_previews':str(scratch)},indent=2))
 return boards
if __name__=='__main__':export()
