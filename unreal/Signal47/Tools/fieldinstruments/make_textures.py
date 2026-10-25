"""Original deterministic surface maps and English 1986 artwork, no external assets.
Run with ordinary Python/Pillow before Blender. Text is authored as source, not photos.
"""
from pathlib import Path
import json, math, random
from PIL import Image, ImageDraw, ImageFont, ImageFilter

R=Path(__file__).resolve().parent
OUT=R/'textures'

def font(n,bold=False):
 for root in [Path('/usr/share/fonts/truetype/dejavu'),Path('/usr/share/fonts/dejavu')]:
  p=root/('DejaVuSans-Bold.ttf' if bold else 'DejaVuSans.ttf')
  if p.exists():return ImageFont.truetype(str(p),n)
 raise RuntimeError('DejaVu font required for original readable artwork')

def surface(name,color,rough,seed,wood=False):
 rng=random.Random(seed);n=1024;im=Image.new('RGB',(n,n));px=im.load()
 for y in range(n):
  for x in range(n):
   grain=(math.sin(y*.5+math.sin(x*.013)*3)+.5*math.sin(y*.13+x*.003))*3 if wood else 0
   k=rng.uniform(-4,4)+grain+math.sin(x*.016)*1.6+math.cos(y*.023)*1.5
   px[x,y]=tuple(max(0,min(255,int(c+k))) for c in color)
 d=ImageDraw.Draw(im)
 for _ in range(220):
  x=rng.randrange(n);y=rng.randrange(n);v=rng.randint(40,90)
  d.line((x,y,x+rng.randrange(3,35),y+rng.choice([-1,0,1])),fill=tuple(max(0,c-v) for c in color),width=1)
 im.save(OUT/(name+'_base_color.png'))
 Image.new('L',(512,512),int(rough*255)).save(OUT/(name+'_roughness.png'))
 # Tangent normal convention is OpenGL (+Y). Its tiny original stipple remains restrained.
 nm=Image.new('RGB',(512,512));np=nm.load()
 for y in range(512):
  for x in range(512):np[x,y]=(128+rng.randrange(-2,3),128+rng.randrange(-2,3),254)
 nm.save(OUT/(name+'_normal.png'))

def atlas():
 im=Image.new('RGB',(2048,2048),(198,189,157));d=ImageDraw.Draw(im);ink=(27,32,28)
 def panel(i,lines,bg=(210,199,164)):
  x=i%4*512;y=i//4*512;d.rectangle((x+2,y+2,x+510,y+510),fill=bg,outline=(61,63,48),width=4)
  for text,yy,size in lines:d.text((x+256,y+yy),text,font=font(size,True),fill=ink,anchor='mt')
  return x,y
 x,y=panel(0,[('FIELD RECEIVER',20,35),('MHz',345,28),('AM   /   CW     BAND 3',438,24)],(214,178,96))
 for j in range(36):
  xx=x+25+j*13;d.line((xx,y+125,xx,y+125+(43 if j%5==0 else 21)),fill=ink,width=3)
  if j%5==0:d.text((xx,y+192),str(10+j//5),font=font(27),fill=ink,anchor='mt')
 d.line((x+24,y+280,x+489,y+280),fill=ink,width=3)
 for j in range(31):
  xx=x+28+j*15;d.line((xx,y+280,xx,y+280+(28 if j%5==0 else 13)),fill=ink,width=2)
 d.text((x+255,y+389),'CALIBRATION   100 kHz',font=font(22),fill=ink,anchor='mt')
 x,y=panel(1,[('SIGNAL',33,43),('S  1  3  5  7  9  +20',330,27)],(208,181,112))
 for j in range(25):
  a=math.radians(160-j*5);r=185;d.line((x+256+math.cos(a)*(r-20),y+291-math.sin(a)*(r-20),x+256+math.cos(a)*r,y+291-math.sin(a)*r),fill=ink,width=3)
 panel(2,[('SARO',38,70),('FIELD RECEIVER  R-47',155,32),('VALVE COMMUNICATIONS',240,26),('115 V AC  /  60 CYCLE',325,25),('SERVICE FILE   01-1947',410,23)],(155,160,139))
 x,y=panel(3,[('STATION 01  /  1947',14,23),('FIELD TIMING RECORD',49,26)],(212,203,172))
 d.line((x+256,y+92,x+256,y+496),fill=(104,95,75),width=5)
 chronology=[('01:40','transit set on A'),('01:52','B vane checked'),('02:05','carrier on'),('02:11','lamp on'),('02:14','reference holding'),('02:16','T. at reference'),('02:17:00','carrier ceased'),('02:17:47','relay impact')]
 for j,(tm,txt) in enumerate(chronology):
  xx=x+15+(j//4)*256;yy=y+112+(j%4)*90
  d.text((xx,yy),tm,font=font(19,True),fill=ink);d.text((xx,yy+27),txt,font=font(14),fill=ink)
  d.line((xx,yy+63,xx+224,yy+63),fill=(154,150,123),width=1)
 x,y=panel(4,[('STATION 01',14,35),('ORIGINAL ARRANGEMENT / 1947',61,19)],(213,205,174))
 pts={'T':(x+251,y+350),'A':(x+405,y+148),'B':(x+78,y+182),'C':(x+434,y+356)}
 for key in ['A','B','C']:
  d.line((*pts['T'],*pts[key]),fill=ink,width=4)
  xx,yy=pts[key];d.ellipse((xx-8,yy-8,xx+8,yy+8),fill=ink);d.text((xx-8,yy-42),key,font=font(29,True),fill=ink)
 d.rectangle((pts['T'][0]-13,pts['T'][1]-13,pts['T'][0]+13,pts['T'][1]+13),outline=ink,width=4)
 d.text((x+179,y+393),'TRANSIT',font=font(26,True),fill=ink)
 d.polygon((x+394,y+170,x+383,y+192,x+405,y+192),outline=ink,width=3);d.line((x+381,y+201,x+407,y+201),fill=ink,width=3)
 d.text((x+35,y+463),'A / FIXED FOOTING     B / WEST    C / CLOSING',font=font(14),fill=ink)
 panel(5,[('FIELD TRANSIT',31,47),('STATION 01',119,47),('SURVEY RECORD  E09A',230,27),('FIXED POINT A',314,34),('1947 / ORIGINAL FOOTING',414,22)])
 panel(6,[('SARO',30,67),('FIELD LINE',142,42),('RING CENTRAL',259,30),('CRANK BEFORE SPEAKING',363,20),('STATION 01',433,25)],(163,151,105))
 panel(7,[('SARO  /  FIELD ROLL',32,31),('EXPOSED FILM',139,45),('STATION 01',242,41),('KEEP CLOSED',350,32),('RECORD IN FIELD LOG',435,22)])
 panel(8,[('SARO',26,71),('35 mm',144,68),('FIELD PHOTOGRAPHY',260,27),('PANCHROMATIC',352,24),('36 EXPOSURES',431,28)],(202,173,86))
 panel(9,[('SARO FIELD RECORDING',38,29),('STATION 01',143,43),('SIDE A',255,57),('60 MIN / NORMAL BIAS',377,24),('DATE: __ / __ / 1986',446,20)],(202,193,165))
 panel(10,[('SURVEY OFFICE',48,37),('INSTRUMENT  01-T',164,34),('ORIGINAL 1947',284,39),('RETAIN IN FIELD',412,26)],(154,157,125))
 panel(11,[('FIELD LAMP',50,45),('HOT SURFACE',170,39),('COVER FOR NULL TEST',297,25),('CLOSE TIN HOOD',409,29)],(202,182,122))
 panel(12,[('SPARE VALVES',56,40),('SARO STORES',192,40),('FRAGILE / KEEP DRY',342,26)])
 panel(13,[('STATION 01',110,62),('FIELD SERVICE',281,37)])
 panel(14,[('INSTRUMENT INSPECTION',56,28),('CHECKED:  09 APR 1986',183,25),('RETURN TO STORES',335,28)])
 panel(15,[],(211,203,170))
 im.save(OUT/'PrintAtlas_base_color.png');Image.new('L',(512,512),207).save(OUT/'PrintAtlas_roughness.png');Image.new('RGB',(512,512),(128,128,255)).save(OUT/'PrintAtlas_normal.png')

def main():
 OUT.mkdir(exist_ok=True);surface('OlivePaint',(83,91,72),.68,47);surface('Wood',(110,79,43),.72,1947,True);surface('Concrete',(154,146,127),.94,86);surface('Metal',(148,153,143),.42,1420);atlas()
 rows=[]
 def row(name,rgb,metal=0,rough=.7,tex=None,**kw):
  r={'name':'M_FieldKit_'+name,'base_color_linear':[*rgb,1], 'metallic':metal,'roughness':rough,'specular':.3,'normal_strength':.17,'normal_convention':'OpenGL','emission':[0,0,0,1],'emission_strength':0,**kw}
  if tex:r['textures']={role:'textures/'+tex+'_'+role+'.png' for role in ['base_color','roughness','normal']}
  rows.append(r)
 row('OlivePaint',[1,1,1],.2,.68,'OlivePaint');row('Wood',[1,1,1],0,.72,'Wood');row('Concrete',[1,1,1],0,.94,'Concrete')
 row('Metal',[1,1,1],.83,.42,'Metal');row('DarkMetal',[.035,.042,.031],.7,.57);row('Brass',[.36,.225,.075],.84,.39)
 row('Rubber',[.012,.014,.011],0,.83);row('Paper',[.67,.62,.47],0,.88);row('Glass',[.021,.037,.037],.22,.13)
 row('PrintAtlas',[1,1,1],0,.75,'PrintAtlas');row('Cloth',[.075,.10,.074],0,.9);row('Film',[.29,.115,.025],.3,.6)
 # Runtime must bind these two separate parts to their existing stateful materials.
 row('FieldBulb',[.86,.66,.35],0,.28,emission=[1,.73,.36,1],emission_strength=.7)
 (R/'materials.json').write_text(json.dumps(rows,indent=2)+'\n')
 print('FIELD_INSTRUMENTS_ORIGINAL_TEXTURES_READY',len(rows),'materials')
if __name__=='__main__':main()
