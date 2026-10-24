"""Original CN3 leaf/needle sprays with real holes; Pillow only, no external bitmap.
RGBA keeps colour bleed in transparent pixels to avoid dark filtering fringes.
"""
from pathlib import Path
import math,random,json
from PIL import Image,ImageDraw,ImageFilter
PALETTE={0:(139,145,123),3:(46,61,35),4:(69,79,45)}

def rotate(p,center,angle):
 x,y=p;c,s=math.cos(angle),math.sin(angle);return (center[0]+c*x-s*y,center[1]+s*x+c*y)

def spray_tile(tile,size=512):
 # Native draw at2x gives a narrow antialiased transition, not translucent whole plants.
 aa=2;n=size*aa;r=random.Random(310100+tile);base=PALETTE[tile]
 rgb=Image.new('RGB',(n,n),base);alpha=Image.new('L',(n,n));d=ImageDraw.Draw(rgb);a=ImageDraw.Draw(alpha)
 def line(points,width,color):
  d.line(points,fill=color,width=max(1,int(width)),joint='curve');a.line(points,fill=255,width=max(1,int(width)),joint='curve')
 def blade(center,length,width,angle):
  shade=r.randint(-12,12);color=tuple(max(0,min(255,v+shade)) for v in base)
  if tile==0:
   # Small narrow, mildly lobed sage leaves; no broad opaque plate is painted.
   shape=[(0,-.5),(.30,-.32),(.40,-.04),(.23,.08),(.35,.28),(.12,.5),(-.12,.5),(-.35,.28),(-.23,.08),(-.40,-.04),(-.30,-.32)]
  else:
   shape=[(0,-.5),(.42,-.20),(.36,.30),(0,.5),(-.36,.30),(-.42,-.20)]
  pts=[rotate((x*width,y*length),center,angle) for x,y in shape]
  d.polygon(pts,fill=color);a.polygon(pts,fill=255)
  # Restrained albedo variation and veins describe the leaf itself, not a baked light.
  if tile==0:
   p0=rotate((0,-length*.35),center,angle);p1=rotate((0,length*.36),center,angle)
   d.line([p0,p1],fill=tuple(min(255,v+4) for v in color),width=max(1,int(n*.001)))
 stem=(103,101,76) if tile==0 else (69,67,43)
 axis=[(n*(.50+.025*math.sin(t*2.1)),n*t) for t in [0,.25,.50,.75,1]];line(axis,n*.006,stem)
 rows=15 if tile==0 else 19
 for j in range(rows):
  y=(.07+(j+r.uniform(-.35,.35))*.84/(rows-1))*n
  for side in [-1,1]:
   reach=n*r.uniform(.24,.43)*(.55+.45*math.sin(math.pi*y/n));end=(n*.5+side*reach,y+n*r.uniform(-.09,.13));start=(n*(.5+r.uniform(-.025,.025)),y-n*.01)
   line([start,end],n*(.0032 if tile==0 else .0025),stem)
   count=20 if tile==0 else 27
   for k in range(count):
    t=(k+r.uniform(.05,.95))/count;x=start[0]+(end[0]-start[0])*t;yy=start[1]+(end[1]-start[1])*t
    branch_angle=math.atan2(end[1]-start[1],end[0]-start[0]);normal=(-math.sin(branch_angle),math.cos(branch_angle))
    for flank in [-1,1]:
     bend=branch_angle-math.pi/2+flank*r.uniform(.45,1.20)
     length=n*r.uniform(.038,.067) if tile==0 else n*r.uniform(.030,.052)
     width=n*r.uniform(.017,.027) if tile==0 else n*r.uniform(.006,.013)
     center=(x+normal[0]*flank*length*.27+n*r.uniform(-.012,.012),yy+normal[1]*flank*length*.27+n*r.uniform(-.012,.012))
     blade(center,length,width,bend)
    if tile!=0:
     blade((x,yy+n*.014),n*r.uniform(.03,.055),n*r.uniform(.007,.013),branch_angle-math.pi/2+r.uniform(-.9,.9))
 # Additional small connected basal shoots prevent a bare stripe through each spray.
 for j in range(70 if tile==0 else 100):
  t=r.uniform(.05,.95);x=n*(.5+r.uniform(-.09,.09));y=n*t
  blade((x,y),n*r.uniform(.035,.060),n*(r.uniform(.016,.025) if tile==0 else r.uniform(.006,.013)),r.uniform(-1.2,1.2))
 rgb=rgb.resize((size,size),Image.Resampling.LANCZOS);alpha=alpha.resize((size,size),Image.Resampling.LANCZOS)
 out=rgb.convert('RGBA');out.putalpha(alpha);return out

def apply_to_atlas(path,stats_path=None):
 path=Path(path);atlas=Image.open(path).convert('RGBA');tile=atlas.width//4;assert atlas.size==(tile*4,tile*2)
 report={'revision':'CN3-original-masked-microfoliage','alpha_cutoff':.333,'note':'Texel statistics only; density/appearance is not accepted before actual render.','tiles':{}}
 for index in range(8):
  x,y=(index%4)*tile,(index//4)*tile
  if index in PALETTE:cell=spray_tile(index,tile);atlas.paste(cell,(x,y))
  cell=atlas.crop((x,y,x+tile,y+tile));values=list(cell.getchannel('A').getdata())
  report['tiles'][str(index)]={'masked':index in PALETTE,'opaque_at_cutoff_fraction':sum(v>=85 for v in values)/len(values),'alpha_min':min(values),'alpha_max':max(values)}
 atlas.save(path)
 if stats_path:Path(stats_path).write_text(json.dumps(report,indent=2))
 return report
