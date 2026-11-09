#!/usr/bin/env python3
"""Paper atlas, 2048 square: nothing legible, so nothing can read as story. Scribbles, ruled grids, dot-matrix grey. System python with PIL."""
import math,random,sys
from pathlib import Path
from PIL import Image,ImageDraw,ImageFilter
OUT=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).parent/'textures'
OUT.mkdir(parents=True,exist_ok=True)
R=random.Random(1986)
S=2048;im=Image.new('RGB',(S,S),(214,206,184));d=ImageDraw.Draw(im,'RGBA')
def scribble(x,y,w,h=9,ink=(32,36,70,235),density=1.,bold=1):
    # cursive that looks like a word and is none
    t=x
    while t<x+w:
        wordlen=R.uniform(.06,.3)*w if w>160 else R.uniform(.25,.7)*w
        pts=[];xx=t;ph=R.random()*6.28
        while xx<min(t+wordlen,x+w):
            pts.append((xx,y+math.sin(ph)*h*R.uniform(.25,.95)+R.uniform(-1,1)))
            xx+=R.uniform(1.4,3.4)/density;ph+=R.uniform(1.,2.1)
        if len(pts)>1:d.line(pts,fill=ink,width=bold,joint='curve')
        if R.random()<.25:d.line([(t+R.uniform(0,wordlen),y-h),(t+R.uniform(0,wordlen),y+h*.8)],fill=ink,width=1)
        t+=wordlen+R.uniform(5,12)
def age(box,tone=(170,140,80),amount=26):
    x0,y0,x1,y1=box
    for _ in range(int((x1-x0)*(y1-y0)/3000)):
        cx,cy=R.uniform(x0,x1),R.uniform(y0,y1);r=R.uniform(5,40)
        d.ellipse((max(x0,cx-r),max(y0,cy-r),min(x1,cx+r),min(y1,cy+r)),fill=(*tone,R.randint(2,amount//3)))
    d.rectangle((x0,y0,x1,y1),outline=(100,92,70,120))
# Cell 0: duty roster, 0..820 x 0..1024: printed grid, handwritten initials, blacked header (no words).
b=(0,0,820,1024);d.rectangle(b,fill=(226,218,196))
d.rectangle((36,34,784,86),fill=(46,46,50))
rows=22;cols=8
for r in range(rows+1):d.line((36,110+r*40,784,110+r*40),fill=(60,70,80,190),width=2)
for c in range(cols+1):
    x=36+c*93.5;d.line((x,110,x,110+rows*40),fill=(60,70,80,190),width=2)
for r in range(rows):
    scribble(44,130+r*40,74,8,(24,28,60,240))
    for c in range(1,cols):
        if R.random()<.72:scribble(36+c*93.5+14,130+r*40,50,8,(R.choice([(24,28,60,235),(24,28,60,235),(150,30,24,225),(30,70,40,220)])),.8)
        if R.random()<.06:d.line((36+c*93.5+4,126+r*40,36+(c+1)*93.5-4,134+r*40),fill=(150,30,24,230),width=3)
age(b);d.ellipse((520,840,640,960),outline=(110,70,30,70),width=6)
# Cells: little notes. (x0,y0,x1,y1,colour)
notes=[(830,0,1230,300,(232,214,120)),(1240,0,1640,300,(235,228,205)),(1650,0,2048,300,(226,176,170)),
 (830,310,1230,610,(226,220,198)),(1240,310,1640,610,(232,214,120)),(1650,310,2048,610,(214,222,206)),
 (830,620,1230,1024,(235,228,205)),(1240,620,1640,1024,(232,214,120))]
for x0,y0,x1,y1,c in notes:
    d.rectangle((x0,y0,x1,y1),fill=c)
    y=y0+40
    ruled=R.random()<.5
    while y<y1-22:
        if ruled:d.line((x0+6,y+11,x1-6,y+11),fill=(90,110,150,70),width=1)
        if R.random()<.84:scribble(x0+24,y,(x1-x0-48)*R.uniform(.45,1),9,R.choice([(24,28,60,240),(24,24,24,235),(140,30,24,225)]),.9,2)
        y+=R.randint(28,44)
    age((x0,y0,x1,y1),amount=18)
# Green-bar printout, 0..1024 x 1024..2048: bars and grey dots in columns. Reads as a printout, says nothing.
b=(0,1024,1024,2048);d.rectangle(b,fill=(230,228,214))
for r in range(0,34):
    y=1024+r*30
    if r%2==0:d.rectangle((40,y,984,y+30),fill=(186,214,186))
for r in range(1,68):
    y=1024+r*15
    x=60
    if R.random()<.12:continue
    while x<960:
        L=R.randint(10,70)
        d.line((x,y,x+L,y),fill=(38,38,50,165),width=4)
        x+=L+R.randint(12,50)
for y in range(1040,2048,40):
    for x in (14,1006):d.ellipse((x-7,y-7,x+7,y+7),fill=(40,40,40))
age(b,amount=12)
# Labels for reels and cans, 1024..2048 x 1024..1280: each has a hand-written line and a coloured bar.
for i in range(8):
    x=1024+i*128;c=R.choice([(220,214,190),(232,214,120),(226,220,198)])
    d.rectangle((x+2,1026,x+126,1278),fill=c);d.rectangle((x+2,1026,x+126,1060),fill=R.choice([(170,44,36),(50,90,140),(60,110,70),(30,30,30)]))
    for j in range(5):scribble(x+10,1084+j*38,106,8,(24,28,60,240))
# Masking tape, 1024..2048 x 1290..1400: creamy, a bit translucent looking, edges torn.
d.rectangle((1024,1290,2048,1400),fill=(205,186,136))
for i in range(900):
    x=R.uniform(1024,2048);d.line((x,1290,x+R.uniform(-3,3),1400),fill=(180,160,110,R.randint(6,30)),width=1)
# Patch panel, 1024..2048 x 1410..1640: two rows of black sockets and blank label strips.
d.rectangle((1024,1410,2048,1640),fill=(44,48,50))
for row in range(2):
    y=1440+row*100;d.rectangle((1040,y+44,2032,y+68),fill=(206,200,176))
    for c in range(24):
        x=1052+c*41;d.ellipse((x,y,x+30,y+30),fill=(10,10,12),outline=(130,130,130,160),width=2)
        if R.random()<.55:scribble(x,y+56,28,5,(24,28,60,240),1.2)
# Tape-transport front, 1024..2048 x 1650..2048: grey panel, seams, blank legend plates.
d.rectangle((1024,1650,2048,2048),fill=(150,152,146))
d.rectangle((1040,1666,2032,1720),fill=(58,60,62))
for i in range(6):
    x=1060+i*160;d.rectangle((x,1740,x+130,1790),fill=(206,200,176));scribble(x+8,1765,110,6,(30,30,30,220))
for i in range(40):
    x=R.uniform(1030,2040);y=R.uniform(1660,2040);d.line((x,y,x+R.uniform(-30,30),y+R.uniform(-3,3)),fill=(70,70,66,50))
im=im.filter(ImageFilter.GaussianBlur(.35))
im.save(OUT/'T_Lived_Paper_BC.png')
print('paper ok')
