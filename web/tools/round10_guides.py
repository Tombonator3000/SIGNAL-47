"""Technical composition diagrams for imagegen, never runtime art."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'src/assets/art/production/round10_qa/sources'
OUT.mkdir(parents=True, exist_ok=True)

im = Image.new('RGB', (2400, 1600), '#323232')
d = ImageDraw.Draw(im)
d.rectangle((0, 1060, 2399, 1599), fill='#686868')
d.polygon([(180,1060),(300,1000),(320,980),(1480,980),(1510,1010),(1670,1060)], fill='#151515')
d.ellipse((810,330,990,510), fill='#747474')
d.ellipse((880,400,920,440), fill='#f8f8f8')
d.line((1320,730,1380,840), fill='#7c7c7c', width=8)
d.ellipse((1375,835,1385,845), fill='#dddddd')
for x in (280,500):
    d.ellipse((x-25,1160,x+25,1210), fill='#111111')
    d.rectangle((x-42,1178,x+42,1192), fill='#111111')
    d.polygon([(x-25,1207),(x+25,1207),(x+44,1410),(x-45,1410)], fill='#111111')
    d.line((x-19,1390,x-35,1595), fill='#111111', width=29)
    d.line((x+19,1390,x+35,1595), fill='#111111', width=29)
d.line((0,1435,710,1435), fill='#222222', width=4)
for x in (120,690): d.rectangle((x,1380,x+12,1599),fill='#222222')
for x in (2010,2150,2290):
    d.ellipse((x-14,980,x+14,1010),fill='#151515')
    d.rectangle((x-22,1010,x+22,1100),fill='#151515')
    d.line((x-12,1090,x-20,1180),fill='#151515',width=12)
    d.line((x+12,1090,x+20,1180),fill='#151515',width=12)
d.rectangle((2130,988,2170,997),fill='#151515')
d.line((2210,960,2210,1180),fill='#222222',width=5)
d.rectangle((2267,1040,2313,1049),fill='#eeeeee')
d.rectangle((1968,1030,1998,1045),fill='#222222')
for x in (1960,2005): d.line((1983,1045,x,1180),fill='#222222',width=5)
im.save(OUT/'photo_composition_guide.png')

im = Image.new('RGB',(1024,1536),'#050b18')
d = ImageDraw.Draw(im)
d.polygon([(420,345),(510,355),(680,640),(656,633)],fill='#578097')
d.ellipse((668,628,692,652),fill='#eeeecc')
d.polygon([(0,970),(110,960),(150,925),(470,925),(520,960),(810,960),(860,935),(1023,935),(1023,999),(0,999)],fill='#01030a')
d.rectangle((70,1010,953,1289), fill='#ebe3cf', outline='#212735',width=2)
d.line((85,1240,938,1240),fill='#a9a594',width=1)
im.save(OUT/'poster_composition_guide.png')

im = Image.new('RGB',(1024,1186),'#f7f7ef')
d = ImageDraw.Draw(im)
for y in range(42,1186,72): d.rectangle((55,y,969,min(y+35,1185)),fill='#eef4e8')
for x in (27,997):
    for y in range(26,1161,54): d.ellipse((x-8,y-8,x+8,y+8),fill='#727272')
for x in (54,970):
    for y in range(0,1186,8): d.line((x,y,x,y+3),fill='#aaaaa0',width=1)
for y in (8,1178):
    for x in range(0,1024,8): d.line((x,y,x+3,y),fill='#aaaaa0',width=1)
im.save(OUT/'fanfold_geometry_guide.png')
print('3 technical composition guides saved')
