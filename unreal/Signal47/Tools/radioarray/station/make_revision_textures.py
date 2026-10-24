"""Original R2 radio-station PBR, from seeded multiscale fields and painted wear.
Uses Pillow through system Python, never external image/model generation.
"""
from pathlib import Path
import math, random, json
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageChops
ROOT=Path(__file__).resolve().parent; OUT=ROOT/'textures'/'revision2'
FONT='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'

def field(size,cells,seed):
 """Periodic random values interpolated from a wrapped grid, no trigonometric bands."""
 w,h=size; cx,cy=cells; r=random.Random(seed)
 tile=Image.frombytes('L',(cx,cy),r.randbytes(cx*cy))
 wrap=Image.new('L',(cx*3,cy*3))
 for y in range(3):
  for x in range(3): wrap.paste(tile,(x*cx,y*cy))
 return wrap.resize((w*3,h*3),Image.Resampling.BICUBIC).crop((w,h,w*2,h*2))

def srgb(v): return round(255*(12.92*v if v<=.0031308 else 1.055*v**(1/2.4)-.055))

def normals(height,stem,amount=.65):
 w,h=height.size; p=height.load(); gl=[]; dx=[]
 for y in range(h):
  for x in range(w):
   a=(p[(x+1)%w,y]-p[(x-1)%w,y])/255*amount
   b=(p[x,(y+1)%h]-p[x,(y-1)%h])/255*amount
   length=math.sqrt(1+a*a+b*b)
   q=(round(127.5-127.5*a/length),round(127.5-127.5*b/length),round(127.5+127.5/length))
   gl.append(q);dx.append((q[0],255-q[1],q[2]))
 im=Image.new('RGB',(w,h)); im.putdata(gl); im.save(OUT/(stem+'_normal_gl.png'))
 im.putdata(dx); im.save(OUT/(stem+'_normal_dx.png'))

def plaster(size=(1024,1024),facade=False):
 w,h=size; coarse=field(size,(5,4),51); middle=field(size,(31,27),52); grain=field(size,(w//3,h//3),53)
 height=Image.blend(Image.blend(grain,middle,.24),coarse,.08)
 rgb=Image.new('RGB',size); rough=Image.new('L',size)
 cp=coarse.load();mp=middle.load();gp=grain.load(); px=rgb.load();rp=rough.load()
 for y in range(h):
  for x in range(w):
   k=1+(cp[x,y]-128)/255*.10+(mp[x,y]-128)/255*.045+(gp[x,y]-128)/255*.022
   px[x,y]=tuple(srgb(max(0,min(1,b*k))) for b in (.60,.525,.405))
   rp[x,y]=max(208,min(246,round(230+(gp[x,y]-128)*.055)))
 if facade:
  d=ImageDraw.Draw(rgb);hd=ImageDraw.Draw(height);rd=ImageDraw.Draw(rough); r=random.Random(2036)
  def pt(x,z): return ((x+6)/12*w,(3.65-z)/3.65*h)
  # Ground splash, softened rain stains and dark accumulated dust under the coping.
  stain=Image.new('RGBA',size);sd=ImageDraw.Draw(stain)
  for i in range(95):
   x=r.uniform(-6,6);z=r.uniform(.02,.38); ww=r.uniform(.08,.38)
   a,b=pt(x-ww,z+.1),pt(x+ww,-.05); sd.ellipse([a,b],fill=(82,65,43,r.randint(8,38)))
  for i in range(55):
   x=r.uniform(-6,6);zz=r.uniform(3.35,3.62);length=r.uniform(.06,.40)
   sd.line([pt(x,zz),pt(x+r.uniform(-.025,.025),zz-length)],fill=(66,55,37,r.randint(12,36)),width=r.randint(2,9))
  rgb=Image.alpha_composite(rgb.convert('RGBA'),stain.filter(ImageFilter.GaussianBlur(5))).convert('RGB');d=ImageDraw.Draw(rgb)
  # Edges peel in selected dry clusters, not uniform dirt scattered over every surface.
  centres=[(-5.85,.40),(-5.72,3.44),(-3.95,.23),(-3.35,2.50),(-.83,2.42),(3.30,.48),(5.80,.18),(5.78,3.42),(1.3,.17)]
  for cx,cz in centres:
   for j in range(r.randint(4,12)):
    x=cx+r.gauss(0,.12);z=cz+r.gauss(0,.13);rr=r.uniform(.012,.065)
    poly=[pt(x+math.cos(a)*rr*r.uniform(.65,1.4),z+math.sin(a)*rr*r.uniform(.65,1.4)) for a in [i*math.tau/7 for i in range(7)]]
    d.polygon(poly,fill=r.choice([(161,143,111),(174,155,121),(185,168,136)]));hd.polygon(poly,fill=89);rd.polygon(poly,fill=247)
  # Dry hairline cracks lead away from lintel corners and old filled repairs.
  for x,z,endx,endz in [(-.77,2.48,-1.1,3.14),(3.2,2.48,3.45,3.13),(-4.03,2.2,-3.7,2.62),(5.8,1.6,5.48,.75),(-5.8,3.42,-5.25,3.15)]:
   points=[]
   for i in range(13):
    t=i/12; points.append(pt(x+(endx-x)*t+r.uniform(-.018,.018),z+(endz-z)*t+r.uniform(-.014,.014)))
   d.line(points,fill=(136,123,101),width=2);hd.line(points,fill=102,width=2);rd.line(points,fill=246,width=3)
  # Painted, slightly abraded facade identity. Letter positions are architectural metres.
  mask=Image.new('L',size);md=ImageDraw.Draw(mask)
  font=ImageFont.truetype(FONT,round(h*.19)); label='1420 AM'; box=md.textbbox((0,0),label,font=font)
  textlayer=Image.new('L',(box[2]-box[0],box[3]-box[1]));ImageDraw.Draw(textlayer).text((-box[0],-box[1]),label,font=font,fill=255)
  tw=round(w*3.85/12);th=round(h*.57/3.65);textlayer=textlayer.resize((tw,th),Image.Resampling.LANCZOS)
  left,top=pt(1.2-3.85/2,3.12);mask.paste(textlayer,(round(left),round(top)))
  # Small paint losses expose the original stucco instead of alpha-translucent glyphs.
  wear=field(size,(720,360),91);wp=wear.load();lp=mask.load()
  for y in range(max(0,int(top)),min(h,int(top)+th)):
   for x in range(max(0,int(left)),min(w,int(left)+tw)):
    if wp[x,y]>250: lp[x,y]=round(lp[x,y]*.10)
  paint=Image.new('RGB',size,(37,35,27));rgb.paste(paint,(0,0),mask)
  stem='facade_1420_weathered'
 else: stem='stucco_layers'
 rgb.save(OUT/(stem+'_color.png'));rough.convert('RGB').save(OUT/(stem+'_roughness.png'));normals(height,stem,.62)
 return stem

def timber(stem,base,seed,olive=False):
 size=(1024,1024); w,h=size
 long=field(size,(80,5) if olive else (5,96),seed);shade=field(size,(4,3),seed+1);small=field(size,(240,30),seed+2)
 rgb=Image.new('RGB',size);height=Image.blend(long,small,.22);rough=Image.new('L',size)
 lp=long.load();sp=shade.load();fine=small.load();p=rgb.load();rp=rough.load()
 for y in range(h):
  for x in range(w):
   k=.92+(sp[x,y]-128)/255*.35+(lp[x,y]-128)/255*.30+(fine[x,y]-128)/255*.05
   b=base
   if olive and lp[x,y]>210 and fine[x,y]>180:b=(.22,.14,.065)
   p[x,y]=tuple(srgb(max(0,min(1,v*k))) for v in b);rp[x,y]=int(158+(fine[x,y]-128)*.13)
 rgb.save(OUT/(stem+'_color.png'));rough.convert('RGB').save(OUT/(stem+'_roughness.png'));normals(height,stem,.18)

def generate():
 OUT.mkdir(parents=True,exist_ok=True)
 plaster();plaster((2048,1024),True)
 timber('walnut_worked',(.105,.048,.021),123)
 timber('desk_worn',(.23,.105,.038),127)
 timber('olive_timber',(.062,.079,.041),131,True)
 print('R2_PBR_READY',OUT)

if __name__=='__main__': generate()
