"""Original 1986 instrument printing and fine aged vinyl, authored with Pillow."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont,ImageFilter
import math,random
import make_revision_textures as base
ROOT=Path(__file__).resolve().parent;OUT=ROOT/'textures'/'revision3'
FONT='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
def generate():
 OUT.mkdir(parents=True,exist_ok=True)
 # A long, finely printed equipment face, with two actual readable analog scales.
 im=Image.new('RGB',(1536,384),(64,68,58));d=ImageDraw.Draw(im)
 f=ImageFont.truetype(FONT,20);small=ImageFont.truetype(FONT,14)
 d.text((32,20),'1420 AM   /   PROGRAM MONITOR',font=f,fill=(214,208,174))
 for cx in [225,600]:
  d.rounded_rectangle((cx-90,76,cx+90,270),radius=12,fill=(23,27,22),outline=(157,157,135),width=4)
  d.rounded_rectangle((cx-80,91,cx+80,253),radius=7,fill=(216,200,151))
  for i in range(25):
   a=math.radians(210+i*5);rr=113
   x=cx+math.cos(a)*rr*.58;y=257+math.sin(a)*rr
   d.line((x,y,cx+math.cos(a)*(rr-9)*.58,257+math.sin(a)*(rr-9)),fill=(64,54,39),width=2)
  d.line((cx,250,cx+42,151),fill=(118,41,24),width=4)
  d.text((cx-16,225),'VU',font=small,fill=(52,49,34))
 for i,name in enumerate(['AUX','LINE','RETURN','CUE']):
  x=885+i*145;d.text((x-20,81),name,font=small,fill=(217,209,177))
  for a in range(13):
   angle=math.radians(35+a*25);d.line((x+35*math.sin(angle),166+35*math.cos(angle),x+42*math.sin(angle),166+42*math.cos(angle)),fill=(202,200,171),width=2)
 for y in [303,312,321]:
  for x in range(50,1460,20):d.line((x,y,x+10,y),fill=(31,35,29),width=3)
 for x,y in [(16,16),(1518,16),(16,366),(1518,366)]:
  d.ellipse((x-5,y-5,x+5,y+5),fill=(158,158,142));d.line((x-3,y,x+3,y),fill=(42,45,38),width=2)
 im.save(OUT/'program_panel_color.png')
 Image.new('RGB',im.size,(163,163,163)).save(OUT/'program_panel_roughness.png')
 Image.new('RGB',im.size,(128,128,255)).save(OUT/'program_panel_normal_gl.png');Image.new('RGB',im.size,(128,127,255)).save(OUT/'program_panel_normal_dx.png')
 size=(1024,1024);grain=base.field(size,(340,340),401);coarse=base.field(size,(7,10),402)
 image=Image.new('RGB',size);rough=Image.new('RGB',size);p=image.load();g=grain.load();c=coarse.load();rp=rough.load()
 for y in range(1024):
  for x in range(1024):
   k=.84+(g[x,y]-128)/255*.24+(c[x,y]-128)/255*.32
   p[x,y]=tuple(base.srgb(v*k) for v in (.025,.042,.028));q=round(153+(g[x,y]-128)*.13);rp[x,y]=(q,q,q)
 image.save(OUT/'aged_vinyl_color.png');rough.save(OUT/'aged_vinyl_roughness.png')
 old=base.OUT;base.OUT=OUT;base.normals(grain,'aged_vinyl',.26);base.OUT=old
 print('R3_PBR_READY',OUT)
if __name__=='__main__':generate()
