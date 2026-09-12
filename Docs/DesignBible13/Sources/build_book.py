"""Render the editable design proposal and vector sketches to one review PDF."""
from pathlib import Path
import re,html,json,sys
from reportlab.platypus import BaseDocTemplate,PageTemplate,Frame,Paragraph,Spacer,PageBreak,NextPageTemplate,Table,TableStyle,Image,Flowable
from reportlab.platypus.tableofcontents import TableOfContents
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.pagesizes import A4,landscape
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.graphics import renderPDF
from PIL import Image as PILImage
import build_visuals
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'output/pdf/SIGNAL47-Designbibel-v0.1.pdf';OUT.parent.mkdir(parents=True,exist_ok=True)
W,H=A4;LW,LH=landscape(A4);M=43;BW=W-2*M
pdfmetrics.registerFontFamily('DV',normal='DV',bold='DVB',italic='DV',boldItalic='DVB')
INK=colors.HexColor('#1a2c26');ACC=colors.HexColor('#ab6530');MUT=colors.HexColor('#53685e');PAPER=colors.HexColor('#faf8f2')
S={
 'body':ParagraphStyle('Body',fontName='DV',fontSize=10.2,leading=15.2,textColor=INK,spaceAfter=8,splitLongWords=True),
 'h1':ParagraphStyle('H1',fontName='DVB',fontSize=22,leading=27,textColor=INK,spaceBefore=19,spaceAfter=12,keepWithNext=True),
 'h2':ParagraphStyle('H2',fontName='DVB',fontSize=13.5,leading=18,textColor=ACC,spaceBefore=13,spaceAfter=7,keepWithNext=True),
 'h3':ParagraphStyle('H3',fontName='DVB',fontSize=11,leading=16,textColor=MUT,spaceBefore=8,spaceAfter=6,keepWithNext=True),
 'cell':ParagraphStyle('Cell',fontName='DV',fontSize=9,leading=12.2,textColor=INK,spaceAfter=0,splitLongWords=True),
 'th':ParagraphStyle('TH',fontName='DVB',fontSize=9,leading=12.2,textColor=colors.white,spaceAfter=0),
 'caption':ParagraphStyle('Caption',fontName='DV',fontSize=10,leading=14,textColor=MUT,spaceAfter=6),
 'toc':ParagraphStyle('TOC0',fontName='DV',fontSize=9.3,leading=13.5,textColor=INK,leftIndent=0,firstLineIndent=0,rightIndent=24,spaceBefore=1),
}
def inline(s):
 s=s.translate(str.maketrans({'–':'-','—':'-','−':'-','‑':'-'}))
 s=html.escape(s)
 s=re.sub(r'\*\*(.+?)\*\*',r'<b>\1</b>',s)
 s=re.sub(r'`(.+?)`',r'<font color="#465e52">\1</font>',s)
 return s

def header(c,doc):
 w,h=c._pagesize;c.saveState();c.setFillColor(PAPER);c.rect(0,0,w,h,fill=1,stroke=0);c.setFillColor(MUT);c.setFont('DV',8)
 c.drawString(M,h-26,'SIGNAL / 47  |  DEN SLETTEDE NATTEN');c.drawRightString(w-M,h-26,'DESIGNFORSLAG v0.1 / INNEHOLDER AVSLØRINGER')
 c.setStrokeColor(colors.HexColor('#c7cabe'));c.line(M,h-35,w-M,h-35);c.line(M,37,w-M,37);c.drawString(M,23,'11.09.2026  /  NY HISTORIE ER FORSLAG  /  INGEN SPILLTEST I DETTE PASSET');c.drawRightString(w-M,23,str(doc.page));c.restoreState()

def cover(c,doc):
 c.saveState();c.setFillColor(colors.HexColor('#101b1b'));c.rect(0,0,W,H,fill=1,stroke=0)
 c.setFillColor(colors.HexColor('#efb56e'));c.setFont('DVB',11);c.drawString(M,H-65,'DESIGNBIBEL  /  v0.1  /  11 SEPTEMBER 2026')
 c.setFont('DVB',41);c.setFillColor(colors.HexColor('#eeeade'));c.drawString(M,H-143,'SIGNAL / 47')
 c.setFont('DV',23);c.drawString(M,H-185,'Den slettede natten')
 c.setFont('DV',11);c.setFillColor(colors.HexColor('#b7c5bd'));c.drawString(M,H-224,'En hel historie. Et avgrenset spill. En konkret byggeplan.')
 image=ROOT/'Visuals/concept-01-survey-station.png';iw,ih=PILImage.open(image).size;hh=BW*ih/iw;c.drawImage(str(image),M,290,width=BW,height=hh,mask='auto')
 c.setFillColor(colors.HexColor('#efb56e'));c.setFont('DVB',18);c.drawString(M,246,'3 STEDER    18 OPPGAVER    330 MINUTTER')
 c.setFillColor(colors.HexColor('#b7c5bd'));c.setFont('DV',10)
 for i,t in enumerate(['Historie, regler, kapitler og begge avslutninger.','Kart, konseptbilder, UI, save/load og produksjonsporter.','Spilletiden er et designmål og må prøves på førstegangsspillere.']):c.drawString(M,208-i*19,t)
 c.setStrokeColor(colors.HexColor('#50645d'));c.line(M,107,W-M,107);c.setFont('DVB',9);c.setFillColor(colors.HexColor('#efb56e'));c.drawString(M,82,'DESIGNFORSLAG / IKKE ET FERDIG SPILL / INNEHOLDER HELE AVSLØRINGEN')
 c.restoreState()

class Book(BaseDocTemplate):
 def __init__(self,*a,**kw):
  super().__init__(*a,**kw);self.heading_records=[]
 def beforeDocument(self):self.heading_records=[]
 def afterFlowable(self,f):
  if isinstance(f,Paragraph) and f.style.name=='H1' and f.getPlainText()!='Lesekart':
   title=f.getPlainText();key='section-'+re.sub('[^a-z0-9]+','-',title.lower()).strip('-')
   self.canv.bookmarkPage(key);self.canv.addOutlineEntry(title,key,0,False);self.notify('TOCEntry',(0,title,self.page,key));self.heading_records.append({'title':title,'page':self.page})

class BoardFlow(Flowable):
 def __init__(self,d):super().__init__();self.d=d;self.scale=(LW-60)/1280;self.width=1280*self.scale;self.height=800*self.scale
 def draw(self):self.canv.saveState();self.canv.scale(self.scale,self.scale);renderPDF.draw(self.d,self.canv,0,0);self.canv.restoreState()

def table_widths(head):
 n=len(head)
 if n==3:return [BW*.20,BW*.4,BW*.4]
 if n==4:
  if any('minutter' in x for x in head):return [BW*.26,BW*.40,BW*.11,BW*.23]
  if head[0]=='Milepæl':return [BW*.17,BW*.30,BW*.18,BW*.35]
  return [BW*.18,BW*.28,BW*.20,BW*.34]
 return [BW/n]*n

def parse_md(path,skip_titles=False):
 lines=path.read_text().splitlines();out=[];i=0
 if skip_titles:
  while i<len(lines) and (lines[i].startswith('#') or not lines[i].strip()):i+=1
  out.append(Paragraph('00 / Ramme og status',S['h1']))
 while i<len(lines):
  l=lines[i].strip()
  if not l or l=='---':i+=1;continue
  if l.startswith('|'):
   rows=[]
   while i<len(lines) and lines[i].strip().startswith('|'):
    cells=[x.strip() for x in lines[i].strip().strip('|').split('|')]
    if not all(re.fullmatch(r':?-+:?',x.replace(' ','')) for x in cells):rows.append(cells)
    i+=1
   n=len(rows[0]);assert all(len(r)==n for r in rows),(path,rows)
   data=[[Paragraph(inline(x),S['th'] if j==0 else S['cell']) for x in r] for j,r in enumerate(rows)]
   t=Table(data,colWidths=table_widths(rows[0]),repeatRows=1,hAlign='LEFT');t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),INK),('ROWBACKGROUNDS',(0,1),(-1,-1),[colors.HexColor('#edf0e8'),colors.HexColor('#f8f6ef')]),('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),7),('RIGHTPADDING',(0,0),(-1,-1),7),('TOPPADDING',(0,0),(-1,-1),6),('BOTTOMPADDING',(0,0),(-1,-1),6),('LINEBELOW',(0,0),(-1,0),.7,INK),('LINEBELOW',(0,1),(-1,-1),.3,colors.HexColor('#d0d5c9'))]));out.extend([t,Spacer(1,10)]);continue
  m=re.match(r'^(#{1,3}) (.*)',l)
  if m:
   lev=len(m[1]);title=m[2]
   if lev==1 and (title.startswith(('03 /','06 /','11 /','15 /','17 /')) or title.startswith('Vedlegg')):out.append(PageBreak())
   out.append(Paragraph(inline(title),S['h'+str(lev)]));i+=1;continue
  buf=[l];i+=1
  while i<len(lines) and lines[i].strip() and not lines[i].startswith(('#','|')) and lines[i].strip()!='---':buf.append(lines[i].strip());i+=1
  out.append(Paragraph(inline(' '.join(buf)),S['body']))
 return out

def main():
 doc=Book(str(OUT),pagesize=A4,title='SIGNAL / 47 - Designbibel v0.1',author='SIGNAL / 47 project',subject='Complete proposed narrative, scope, roadmap, maps and UI sketches; not gameplay verification')
 doc.addPageTemplates([PageTemplate(id='Cover',frames=[Frame(M,50,BW,H-100)],onPage=cover),PageTemplate(id='Body',frames=[Frame(M,50,BW,H-101,leftPadding=0,rightPadding=0,topPadding=0,bottomPadding=0)],onPage=header),PageTemplate(id='Landscape',pagesize=landscape(A4),frames=[Frame(30,48,LW-60,LH-93,leftPadding=0,rightPadding=0,topPadding=0,bottomPadding=0)],onPage=header)])
 flow=[Spacer(1,1),NextPageTemplate('Body'),PageBreak(),Paragraph('Lesekart',S['h1']),Paragraph('Start med ramme og hele historien. Oppgaveregister, lagring og roadmap brukes under produksjon. De visuelle platene ligger samlet bakerst, i liggende format og med tydelig status.',S['body'])]
 toc=TableOfContents();toc.levelStyles=[S['toc']];flow.extend([toc,PageBreak()]);flow.extend(parse_md(ROOT/'design-bible.md',True));flow.extend(parse_md(ROOT/'story-material.md'))
 flow.extend([Paragraph('Visuelle plater / kart, konsepter og UI',S['h1']),Paragraph('De neste sidene er redigerbare kart-/UI-skisser og tre genererte konseptbilder. Kartene er topologiske forslag, ikke oppmålinger. UI-data er eksempler. Ingen av platene er et bevis på at ny historiefunksjonalitet er bygget i Unity.',S['body']),Paragraph('Platerekkefølge: verdensrute, SARO, to lokale områder, oppgaveflyt, målestasjon, Nora, finale, startmeny, last sak, pause/lagre, fotoarbeid, innstillinger og gjenoppretting.',S['body']),NextPageTemplate('Landscape'),PageBreak()])
 boards=build_visuals.make_all()
 ordered=list(boards.keys())[:4]+['concept-01-survey-station','concept-02-nora-motel','concept-03-final-reading']+list(boards.keys())[4:]
 captions={
 'concept-01-survey-station':'KONSEPT 01 / Målestasjonen. Én liten hytte, fysisk frakobling og en lesbar siktelinje til fastmerket. Tekst, merker og nøyaktig geometri må produseres fra den vedtatte oppgaven, ikke fra bildeillustrasjonen.',
 'concept-02-nora-motel':'KONSEPT 02 / Rom 6. Et menneskelig møte rundt to rapporter og et bilde. Figurens fotorealistiske detaljnivå er et stemningsmål; den foreslåtte spillfiguren har begrenset animasjon og må prøves som egen produksjonsoppgave.',
 'concept-03-final-reading':'KONSEPT 03 / Siste avlesning. Kjent SARO, fysisk velger, analogt kamera og kort vei tilbake. En uønsket antennestråle i første utkast ble fjernet; himmelen skal ha et svakt, fjernt referansespor, ikke en laser eller portal.'}
 for idx,name in enumerate(ordered):
  if idx:flow.append(PageBreak())
  if name in boards:flow.append(BoardFlow(boards[name].d))
  else:
   p=ROOT/'Visuals'/(name+'.png');iw,ih=PILImage.open(p).size;ww=LW-60;hh=ww*ih/iw;flow.append(Image(str(p),width=ww,height=hh));flow.append(Spacer(1,12));flow.append(Paragraph(inline(captions[name]),S['caption']))
 doc.multiBuild(flow,maxPasses=4)
 (ROOT/'Sources/pdf-sections.json').write_text(json.dumps(doc.heading_records,ensure_ascii=False,indent=2)+'\n')
 print(json.dumps({'pdf':str(OUT),'pages':doc.page,'sections':len(doc.heading_records),'visual_plates':len(ordered)},indent=2))
if __name__=='__main__':main()
