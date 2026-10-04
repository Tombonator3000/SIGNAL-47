#!/usr/bin/env python3
"""Oppdater SARO som redigerbar SVG og PNG uten eksterne bilder.

Kjør fra vilkårlig mappe: python3 render_maps.py
STATION 01 og motell er historiske forslag og regenereres ikke.
PNG krever Node med sharp. Sett MAP_NODE og MAP_SHARP ved behov.
SVG-filene består av navngitte grupper, former og redigerbar tekst.
"""
from pathlib import Path
from html import escape
import hashlib
import json
import os
import subprocess
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
W, H = 1800, 1280
C = dict(paper='#ebe3cf', ink='#152b36', muted='#637275', amber='#a8641f',
         plan='#f0d5a9', built='#d4ddd4', bg='#d9d5c8', line='#b1b4a9', white='#f6f0df')


class Map:
    def __init__(self, key, title, sub, tag):
        self.key, self.parts = key, []
        self.parts.append(f'''<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" role="img" aria-labelledby="title desc">
<title id="title">{escape(title)}: redigerbart nærkart</title>
<desc id="desc">{escape(sub)}. Konseptkart, ikke målestokk og ikke spillbilde.</desc>
<defs><pattern id="background-hatch" width="16" height="16" patternUnits="userSpaceOnUse"><path d="M-4 4L4-4M0 16L16 0M12 20L20 12" stroke="#b7b9b0" stroke-width="1"/></pattern></defs>
<style>text{{font-family:"DejaVu Sans",Arial,sans-serif;}}</style>''')
        self.rect(0, 0, W, H, C['paper'], 'none')
        self.text(64, 54, 'SIGNAL / 47     •     PRODUKSJONSKART', 20, weight=700, spacing=2)
        self.text(64, 123, title, 52, weight=700)
        self.text(66, 166, sub, 22, color=C['muted'])
        self.line(64, 191, 1736, 191, C['ink'], 2)
        self.text(1736, 54, tag, 18, weight=700, anchor='end', color=C['amber'])
        self.rect(64, 220, 1052, 910, C['white'], C['line'], 1, radius=10)
        self.line(1150, 220, 1150, 1130, C['line'], 1)
        self.text(64, 1239, 'KONSEPTKART   •   IKKE MÅLESTOKK   •   IKKE SPILLBILDE', 20, weight=700, spacing=1)
        self.text(1736, 1239, 'REDIGERBAR SVG  /  ' + key.upper(), 17, anchor='end', color=C['muted'])

    def raw(self, value): self.parts.append(value)
    def group(self, name): self.raw(f'<g id="{name}">')
    def end(self): self.raw('</g>')
    def rect(self, x, y, w, h, fill, stroke=C['ink'], sw=2, radius=0, dash=None):
        ds = f' stroke-dasharray="{dash}"' if dash else ''
        self.raw(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"{ds}/>')
    def line(self, x1, y1, x2, y2, col=C['ink'], width=2, dash=None):
        ds = f' stroke-dasharray="{dash}"' if dash else ''
        self.raw(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{col}" stroke-width="{width}"{ds}/>')
    def path(self, d, col=C['ink'], width=2, fill='none', dash=None):
        ds = f' stroke-dasharray="{dash}"' if dash else ''
        self.raw(f'<path d="{d}" fill="{fill}" stroke="{col}" stroke-width="{width}" stroke-linecap="round" stroke-linejoin="round"{ds}/>')
    def circle(self, x, y, r, fill=C['paper'], stroke=C['ink'], sw=2):
        self.raw(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>')
    def text(self, x, y, value, size=24, color=None, anchor='start', weight=400, spacing=None):
        lines = value if isinstance(value, list) else [value]
        sp = f' letter-spacing="{spacing}"' if spacing is not None else ''
        col = color or C['ink']
        self.raw(f'<text x="{x}" y="{y}" font-size="{size}" font-weight="{weight}" text-anchor="{anchor}" style="fill:{col}"{sp}>')
        for i, t in enumerate(lines): self.raw(f'<tspan x="{x}" dy="{0 if i == 0 else size*1.38}">{escape(t)}</tspan>')
        self.raw('</text>')
    def number(self, x, y, n, planned=False):
        color = C['amber'] if planned else C['ink']
        self.circle(x, y, 19, color, color)
        self.text(x, y+7, str(n), 20, C['white'], 'middle', 700)
    def arrow(self, x, y, caption='NORD', note=None):
        self.line(x, y+65, x, y+15, C['ink'], 3)
        self.path(f'M{x-10} {y+31}L{x} {y+10}L{x+10} {y+31}', width=3)
        self.text(x, y-4, caption, 19, anchor='middle', weight=700)
        if note: self.text(x, y+91, note, 15, C['muted'], 'middle')
    def background(self, x,y,w,h):
        self.rect(x,y,w,h,C['bg'],C['line'],2)
        self.rect(x,y,w,h,'url(#background-hatch)','none')
    def note(self, y, n, title, lines):
        self.number(1210,y-8,n,self.key!='saro')
        self.text(1245,y,title,24,weight=700)
        self.text(1245,y+37,lines,21,color=C['muted'])
    def legend(self, entries):
        y=1163
        for x, name, kind in entries:
            if kind=='route': self.line(x,y-5,x+40,y-5,C['amber'] if self.key!='saro' else C['ink'],7)
            elif kind=='sight': self.line(x,y-5,x+40,y-5,C['muted'] if self.key=='station01' else C['amber'],3,'6 6')
            elif kind=='cable': self.line(x,y-5,x+40,y-5,C['ink'],3)
            elif kind=='photo': self.rect(x,y-18,32,26,'none',C['amber'],2,dash='6 4')
            elif kind=='door': self.circle(x+16,y-6,8,C['paper'],C['ink'],3)
            elif kind=='plan': self.rect(x,y-18,32,26,C['plan'],C['amber'],2,dash='6 4')
            elif kind=='bg': self.background(x,y-18,32,26)
            else: self.rect(x,y-18,32,26,C['built'],C['ink'],2)
            self.text(x+52,y+1,name,20)
    def save(self):
        self.raw('</svg>')
        path=HERE/f'{self.key if self.key != "station01" else "station01"}-plan.svg'
        path.write_text('\n'.join(self.parts)+'\n')
        return path


def saro():
    m=Map('saro','SARO / FELT, LAB OG ARKIV','Kildeavledet fra main 3264fab. Nord er opp; sørdøra knytter arkivet til kontrollrommet.','THREE.JS / KAPITTEL 1 + 2')
    X=lambda x:500+20*x
    Y=lambda z:850+20*z
    m.group('orientation-and-background')
    m.arrow(150,280,note='-Z i spillet')
    # The distant dish is displaced only after the explicit drawing break.
    m.circle(X(9.5),320,41,C['bg'],C['muted'],3)
    m.path(f'M{X(9.5)-34} 305Q{X(9.5)} 353 {X(9.5)+34} 305',C['muted'],3)
    m.line(X(9.5),320,X(9.5)+20,292,C['muted'],3)
    m.text(780,307,'S-03 / ANTENNE',24,weight=700)
    m.text(780,343,['Bakgrunn, ikke gangbar.','Lenger nord enn utsnittet.'],18,color=C['muted'])
    m.text(280,273,['27 antenner i scenen.','Kun S-03 er vist her.'],19,color=C['muted'])
    m.line(X(9.9),Y(-18.35),X(9.5),363,C['amber'],3,'6 7')
    m.path('M654 408L671 400L688 408L705 400L722 408',C['muted'],2)
    m.text(646,418,'UTSNITTSBRUDD',15,color=C['muted'],anchor='end')
    m.line(284,445,1057,445,C['line'],4,'3 8')
    m.text(292,434,'GJERDE / FELTGRENSE',17,color=C['muted'])
    m.end()
    m.group('existing-physical-footprints')
    for x0,x1,z0,z1 in [(6.3,11,-.6,3.4),(8,11,-19.6,-.6),(11,12.6,-13.4,-8.6),(11,12.6,-1.9,-.6)]:
        m.rect(X(x0),Y(z0),(x1-x0)*20,(z1-z0)*20,C['built'],C['ink'],2)
    m.rect(X(-6.3),Y(-4.8),12.6*20,9.6*20,C['built'],C['ink'],4)
    m.rect(X(12.6),Y(-6),6.6*20,7*20,C['built'],C['ink'],4)
    m.rect(X(-5.6),Y(-4.52),10.9*20,6,'#91b3b4','none')
    m.line(X(6.3),Y(1.12),X(6.3),Y(2.08),C['built'],8)
    m.line(X(12.6),Y(-1.7),X(12.6),Y(-.7),C['built'],8)
    m.text(500,805,'KONTROLLROM',23,anchor='middle',weight=700)
    m.text(500,837,['Mottaker, telefon','og skriver'],18,anchor='middle')
    m.text(818,758,'FOTOLAB',21,anchor='middle',weight=700)
    m.text(818,785,['Foto','og rapport'],17,anchor='middle')
    m.text(672,934,'Østdør',18,anchor='middle')
    m.text(892,914,'Vestdør til fotolab',18,anchor='middle',color=C['muted'])
    m.line(795,894,X(12.6),Y(-1.2),C['muted'],1.5)
    m.end()
    m.group('built-south-annex')
    # Floors and wall centerlines are from Annex.ts. The closed east part is
    # shaded separately; no extra walkable rectangle or exit is invented.
    m.rect(X(-6),Y(4.8),8*20,1.8*20,C['built'],C['ink'],2)
    m.rect(X(-6),Y(6.8),5*20,3.7*20,C['built'],C['ink'],2)
    m.background(X(-1),Y(6.8),3*20,3.7*20)
    m.rect(X(-6.3),Y(4.8),8.6*20,6*20,'none',C['ink'],4)
    m.line(X(-6),Y(6.7),X(2),Y(6.7),C['ink'],3)
    m.line(X(-.9),Y(6.8),X(-.9),Y(10.5),C['ink'],3)
    # Actual openings: control-room south wall and records partition.
    m.line(X(-2.9),Y(4.8),X(-1.9),Y(4.8),C['built'],9)
    m.line(X(-4.4),Y(6.7),X(-3.4),Y(6.7),C['built'],9)
    m.text(304,917,'Sørdør',20,anchor='end')
    m.line(318,921,X(-2.9),Y(4.65),C['muted'],1.5)
    m.text(290,975,'SØRKORRIDOR',20,anchor='end',weight=700)
    m.line(304,969,X(-6),Y(5.7),C['muted'],1.5)
    m.text(300,1060,'ARKIV',23,anchor='end',weight=700)
    m.line(315,1052,X(-6),Y(9.9),C['muted'],1.5)
    m.text(625,1025,['LUKKET DEL','Ingen romtilgang'],19,color=C['muted'])
    m.line(607,1019,X(.9),Y(8.5),C['muted'],1.5)
    m.end()
    m.group('existing-route-and-field-points')
    m.path(f'M{X(0)} {Y(1.6)}H{X(9.4)}V{Y(-18.0)}',C['ink'],7)
    m.path(f'M{X(9.4)} {Y(-1.2)}H{X(13.5)}',C['ink'],7)
    m.path(f'M{X(0)} {Y(1.6)}H{X(-2.4)}V{Y(5.7)}H{X(-3.9)}V{Y(7.9)}',C['ink'],7)
    for x,z in [(6.15,1.6),(12.6,-1.2),(-2.4,4.65),(-3.9,6.7)]:
        m.circle(X(x),Y(z),7,C['paper'],C['ink'],3)
    m.rect(X(8),Y(-15.2),4.6*20,6.6*20,'none',C['amber'],3,dash='8 5')
    m.number(X(9.9),Y(-18.35),3)
    m.text(810,491,'B-12 / REFERANSE',23,weight=700)
    m.text(810,526,'Nord for motorskapet',18,color=C['muted'])
    m.line(X(9.9)+25,Y(-18.35),788,Y(-18.35),C['ink'],2)
    m.rect(X(8.35)-7,Y(-16.9)-7,14,14,C['ink'],C['ink'])
    m.line(X(8.35)-13,Y(-16.9),555,Y(-16.9),C['muted'],1.5)
    m.text(539,Y(-16.9)+7,'B-12-kontroll',19,anchor='end')
    m.number(X(12.3),Y(-11),2)
    m.line(X(12.3)+24,Y(-11),790,Y(-11),C['ink'],2)
    m.text(810,615,['S-03','MOTORSKAP'],23,weight=700)
    m.text(625,596,['FOTOFLATE','S-03'],22,anchor='end',weight=700,color=C['amber'])
    m.line(571,651,X(8)-10,651,C['amber'],2)
    m.text(625,709,'Gangvei',20,anchor='end')
    m.number(X(3.3),Y(3.2),1)
    m.number(857,847,4)
    m.number(513,Y(5.7),5)
    m.number(431,Y(9.45),6)
    m.end()
    m.group('reading-notes')
    m.text(1185,256,'FELTARBEID OG ARKIV I SAMME ANLEGG',21,weight=700)
    m.note(313,1,'Kontrollrommet',['Start ved pulten. Feltkameraet','hentes ved østdøra.'])
    m.note(443,2,'Servicegård og S-03',['Gangvei og oppstillingsflate','leder til skapet, ikke antennen.'])
    m.note(573,3,'B-12 lenger nord',['Referansen står ved gjerdet.','Siktlinjen fortsetter mot S-03.'])
    m.note(703,4,'Fotolab øst for gangveien',['Vestdør gir en kort avstikker.','Foto og rapport behandles her.'])
    m.note(833,5,'Sørkorridor i kapittel 2',['Gjennom sørdøra i kontrollrommet.','Drift, WC og nødutgang er stengt.'])
    m.note(963,6,'Arkivet',['Arkivdøra ligger vest i korridoren.','Bord, mapper og telefon er i bruk.'])
    m.text(1185,1091,'Spillfasen styrer når sørdøra åpnes.',20,color=C['muted'])
    m.end()
    m.legend([(80,'Eksisterende flate','built'),(396,'Bakgrunn','bg'),(652,'Fotoflate','photo'),(902,'Døråpning','door'),(1184,'Ganglinje','route'),(1480,'Siktlinje','sight')])
    return m.save()


def station():
    m=Map('station01','STATION 01 / DEN GAMLE MÅLESTASJONEN','Forslag til en kompakt three.js-feltsekvens. Unity-feltet finnes fra før.','PLANFORSLAG TIL THREE.JS')
    m.group('background-and-orientation')
    m.arrow(150,280,note='Skjematisk')
    for x,y,w,h in [(286,274,184,58),(776,259,177,44),(979,936,78,111)]:
        m.background(x,y,w,h)
    m.text(650,249,'FJERNE RYGGER OG BYGG ER BAKGRUNN',17,anchor='middle',color=C['muted'])
    m.end()
    m.group('suggested-short-walk')
    m.path('M265 1000L265 874L360 874L360 618L553 482L724 490L872 596L943 743L923 948L517 1053L265 1000',C['amber'],10)
    m.path('M337 670L349 647L374 650',C['amber'],4)
    m.path('M826 575L851 575L856 552',C['amber'],4)
    m.path('M601 1032L578 1057L603 1070',C['amber'],4)
    m.end()
    m.group('field-building')
    m.rect(205,656,307,202,C['plan'],C['amber'],3)
    m.line(324,858,390,858,C['plan'],8)
    m.line(334,656,387,656,C['plan'],8)
    m.rect(230,678,255,34,C['paper'],C['amber'],2)
    m.text(358,759,'FELTBYGNING',26,anchor='middle',weight=700)
    m.text(358,802,'Benk, logg og protokoll',20,anchor='middle')
    m.number(205,656,1,True)
    m.end()
    m.group('transit-and-reference-points')
    m.circle(724,490,59,C['plan'],C['amber'],3)
    m.path('M701 510L724 467L747 510M724 467V520M707 478H743',C['ink'],3)
    m.text(646,589,'TRANSITFLATE',22,anchor='end',weight=700)
    m.number(676,451,2,True)
    for x,y,n,tx,ty,anchor in [(506,403,1,465,366,'end'),(929,374,2,956,344,'middle'),(943,743,3,895,806,'end')]:
        m.path(f'M{x} {y-18}L{x-19} {y+15}H{x+19}Z',C['ink'],3,C['paper'])
        m.line(x-14,y+23,x+14,y+23,C['ink'],3)
        m.text(tx,ty,f'FASTMERKE {n}',20,anchor=anchor,weight=700)
    m.line(724,490,506,403,C['muted'],2,'6 7')
    m.line(724,490,929,374,C['muted'],2,'6 7')
    m.line(724,490,943,743,C['muted'],2,'6 7')
    m.text(757,667,['Feltmerking og vinkler','fastlegges ved blokkering.'],18,color=C['muted'],anchor='middle')
    m.end()
    m.group('cable-and-arrival')
    m.path('M512 785L585 830L620 932L742 932',C['ink'],4)
    m.path('M778 932L990 892L1045 716L998 670L983 447L929 374',C['ink'],4)
    m.path('M742 922L742 941M778 922L778 941',C['ink'],3)
    m.circle(760,932,36,'none',C['amber'],2)
    m.text(751,875,'KABELBRUDD',22,anchor='middle',weight=700)
    m.text(1046,598,'KABEL',17,anchor='middle')
    m.number(799,932,3,True)
    m.rect(211,1000,108,50,C['paper'],C['ink'],3,9)
    m.rect(238,1007,49,36,C['bg'],C['ink'],1,5)
    m.text(200,1092,'ANKOMST / RETUR',22,weight=700)
    m.end()
    m.group('proposal-notes')
    m.text(1185,256,'ETT ROM OG EN KORT FELTSLØYFE',21,weight=700)
    m.note(324,1,'En liten feltbygning',['Benk, historisk logg og protokoll.','Ingen ny fløy eller byggeklynge.'])
    m.note(474,2,'Transit og tre faste merker',['Kort siktavstand fra samme flate.','Terrengmerkene får tydelig form.'])
    m.note(624,3,'Kabel og retur',['Følg en synlig, avgrenset kabel.','Dokumenter bruddet og returner.'])
    m.rect(1185,788,551,307,C['paper'],C['amber'],2,dash='10 7')
    m.text(1210,828,'FØR BLOKKERING',23,weight=700,color=C['amber'])
    m.text(1210,873,['Dette er en lokal romlig skisse,','ikke en oppmåling av Unity-scenen.','','Fastmerkene viser ikke oppgavens fasit.','Historisk foto og bevis må komme fra','den endelige fysiske oppstillingen.'],21,color=C['muted'])
    m.end()
    m.legend([(80,'Planlagt spillflate','plan'),(410,'Kort gangsløyfe','route'),(763,'Bakgrunn','bg'),(1058,'Foreslåtte siktlinjer','sight'),(1480,'Kabel','cable')])
    return m.save()


def motel():
    m=Map('motel','SIERRA MOTOR COURT / KONTOR OG NORAS ROM','Forslag til et avgrenset motellområde i three.js. Bare to interiører åpnes.','PLANFORSLAG TIL THREE.JS')
    m.group('orientation-and-background')
    m.arrow(150,280,note='Skjematisk')
    m.background(298,352,606,196)
    for x in [399,500,601,702,803]: m.line(x,352,x,548,C['line'],2)
    m.text(600,430,'ØVRIGE ROM',26,anchor='middle',weight=700,color=C['muted'])
    m.text(600,468,'Bakgrunn uten tilgjengelige interiører',20,anchor='middle',color=C['muted'])
    m.background(633,738,249,172)
    m.rect(652,757,210,135,'none',C['muted'],2,22)
    m.text(757,809,'BASSENG',24,anchor='middle',weight=700,color=C['muted'])
    m.text(757,847,'Kun bakgrunn',19,anchor='middle',color=C['muted'])
    m.end()
    m.group('two-planned-interiors')
    m.rect(906,352,171,196,C['plan'],C['amber'],3)
    m.text(991,412,['NORAS','ROM'],25,anchor='middle',weight=700)
    m.text(991,501,'Samtale og bevis',17,anchor='middle')
    m.line(967,548,1017,548,C['plan'],8)
    m.number(1077,352,3,True)
    m.rect(162,784,257,188,C['plan'],C['amber'],3)
    m.rect(185,804,207,27,C['paper'],C['amber'],2)
    m.text(290,876,'KONTOR',28,anchor='middle',weight=700)
    m.text(290,918,'Beskjed og konvolutt',18,anchor='middle')
    m.line(419,869,419,924,C['plan'],8)
    m.number(162,784,2,True)
    m.end()
    m.group('parking-sign-and-walk')
    m.rect(450,986,580,108,'none',C['line'],2,12)
    for x in [565,680,795,910]: m.line(x,1008,x,1093,C['line'],2)
    m.rect(477,1008,63,66,C['paper'],C['ink'],2,9)
    m.text(910,974,'PARKERING',20,anchor='end',color=C['muted'])
    m.path('M507 1040H449V899H360',C['amber'],9)
    m.path('M449 899V577H991V524',C['amber'],9)
    m.path('M467 665L449 645L432 665',C['amber'],4)
    m.path('M837 560L856 577L837 594',C['amber'],4)
    m.text(711,624,'TYDELIG GANGVEI',21,anchor='middle',weight=700)
    m.number(457,1040,1,True)
    m.rect(165,522,132,124,C['paper'],C['amber'],2,8)
    m.text(231,563,['SIERRA','MOTOR','COURT'],20,anchor='middle',weight=700)
    m.line(214,646,214,701,C['muted'],3)
    m.line(247,646,247,701,C['muted'],3)
    m.text(231,732,'Skilt som orientering',17,anchor='middle',color=C['muted'])
    m.text(192,1096,'ANKOMST / RETUR',20,weight=700)
    m.end()
    m.group('proposal-notes')
    m.text(1185,256,'EN KORT VEI TIL SAMTALEN',21,weight=700,spacing=1)
    m.note(324,1,'Parkering og ankomst',['Synlig motellskilt og oversiktlig','ganglinje til kontoret.'])
    m.note(474,2,'Kontoret etablerer stedet',['En beskjed leder direkte til Nora.','Ingen leting gjennom romrekken.'])
    m.note(624,3,'Noras rom',['Plass til Nora, bord og papirbevis.','Begge interiører inngår i samme tur.'])
    m.rect(1185,788,551,307,C['paper'],C['amber'],2,dash='10 7')
    m.text(1210,828,'AVGRENSNING OG ÅPNE VALG',23,weight=700,color=C['amber'])
    m.text(1210,874,['Romnummer er ikke bestemt.','Derfor brukes bare NORAS ROM.','','Basseng, andre rom og veien rundt','er bakgrunn. Ingen ekstra oppgaver','eller flere tilgjengelige dører.'],21,color=C['muted'])
    m.end()
    m.legend([(80,'Planlagt tilgjengelig interiør','plan'),(559,'Kort ganglinje','route'),(944,'Bakgrunn uten romtilgang','bg')])
    return m.save()


def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    # The two proposal maps are retained as historical snapshots. Regenerating
    # SARO must not alter either their bytes or their manifest/source records.
    manifest_path=HERE/'map-spec.json'
    manifest=json.loads(manifest_path.read_text())
    preserved={name:digest(HERE/name) for name in ['station01-plan.svg','station01-plan.png','motel-plan.svg','motel-plan.png']}
    snapshot='3264fabc84ec36b5fa2b165a41b2b4f6b8f1e6dc'
    paths=['web/src/world/Annex.ts','web/src/world/ControlRoom.ts','web/src/world/ServiceYard.ts','web/src/world/Exterior.ts','web/src/story/Chapter2.ts']
    sources=[]
    for path in paths:
        data=subprocess.check_output(['git','show',snapshot+':'+path],cwd=ROOT)
        sources.append({'path':path,'sha256':hashlib.sha256(data).hexdigest()})
    path=saro()
    svg=ET.parse(path).getroot()
    assert (int(svg.attrib['width']),int(svg.attrib['height']))==(W,H)
    png=path.with_suffix('.png')
    runtime=Path.home()/'.cache/codex-runtimes/codex-primary-runtime/dependencies'
    node=os.environ.get('MAP_NODE',str(runtime/'node/bin/node'))
    sharp=os.environ.get('MAP_SHARP',str(runtime/'node/node_modules/sharp'))
    code='const sharp=require(process.argv[1]);sharp(process.argv[2]).png().toFile(process.argv[3]).catch(e=>{console.error(e);process.exit(1)});'
    subprocess.run([node,'-e',code,sharp,str(path),str(png)],check=True,capture_output=True,text=True)
    manifest['schema']=2
    manifest['sourceSnapshotAppliesTo']=['station01','motel']
    manifest['snapshotNote']='Rotens kildehash og snapshot 6f2bea0 bevarer grunnlaget for de historiske planforslagene STATION 01 og motell. SARO har eget nyere snapshot og kildehash under saro. Motellkartets uavklarte romnummer er historikk; gjeldende romnummer er 6.'
    manifest['status_legend']['amber_dashed_unfilled']='Eksisterende fotoavgrensning på SARO. Planmarkeringer i de historiske forslagene leses etter lokal tekst.'
    manifest['saro'].update({
        'status':'Kodeavledet fra main 3264fab, inkludert bygget sørkorridor og arkiv i kapittel 2. Ikke oppmåling eller kjørbar navigasjon.',
        'updated_utc':'2026-10-04','sourceSnapshotCommit':snapshot,'sources':sources,
        'main_projection':'SVG X = 500 + 20 * world x; SVG Y = 850 + 20 * world z. Antennen er flyttet til øvre bakgrunnsfelt med tydelig utsnittsbrudd.',
        'south_door_xz':[-2.4,4.65],
        'south_door_wall_opening_x':[-2.9,-1.9],
        'archive':{
            'status':'Bygget Three.js-område, ikke separat planinnsett.',
            'outer_shell_bounds_xz':[-6.3,2.3,4.8,10.8],
            'corridor_floor_bounds_xz':[-6,2,4.8,6.6],
            'records_floor_bounds_xz':[-6,-1,6.8,10.5],
            'records_door_center_xz':[-3.9,6.7],
            'records_door_wall_opening_x':[-4.4,-3.4],
            'walkable_zones_bounds_xz':{
                'southDoor':[-2.88,-1.92,3.6,5.8],
                'corridor':[-5.75,1.75,5.05,6.35],
                'recordsDoor':[-4.38,-3.42,5.6,7.7],
                'records':[-5.75,-1.25,7.05,10.25]},
            'route_world_xz':[[0,1.6],[-2.4,1.6],[-2.4,5.7],[-3.9,5.7],[-3.9,7.9]],
            'route_note':'Skjematisk ganglinje går gjennom begge faktiske åpninger og stopper før arkivbordet. Ikke en full kollisjonstest.',
            'closed_doors':['Driftskontor','WC','Nødutgang'],
            'closed_region':'Den østlige delen bak arkivskilleveggen er skravert. Ingen spillbar romdeling eller ny inngang er funnet på.'},
        'validation':{'svg':'XML kontrollert','dimensions':[W,H],'render':'PNG skal visuelt kontrolleres etter hver endring','preserved_other_outputs':preserved}
    })
    entry={'svg':path.name,'svg_sha256':digest(path),'svg_bytes':path.stat().st_size,'png_rendered':True,'png':png.name,'png_sha256':digest(png),'png_bytes':png.stat().st_size}
    manifest['outputs']=[entry if item['svg']=='saro-plan.svg' else item for item in manifest['outputs']]
    for name,expected in preserved.items():
        if digest(HERE/name)!=expected: raise RuntimeError('Historisk kart er endret: '+name)
    manifest_path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    print('SARO rendret. STATION 01 og motell bevart byte-for-byte.')


if __name__=='__main__': main()
