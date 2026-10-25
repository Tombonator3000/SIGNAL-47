"""Original layered stucco/concrete PBR and painted institutional sign. No external images.
Run explicitly before build_site.py. The patterns combine mineral grain, repair patches,
hairline shrinkage and mottled paint, rather than a homogeneous random colour field.
"""
from pathlib import Path
import json,random,hashlib
from PIL import Image,ImageDraw,ImageFont,ImageFilter
import numpy as np
R=Path(__file__).resolve().parent;T=R/'textures';T.mkdir(exist_ok=True)
rng=random.Random(47025);size=1024
def surface(name,color):
 h=Image.new('L',(size,size),127);d=ImageDraw.Draw(h)
 for _ in range(350):
  x,y=rng.randrange(size),rng.randrange(size);w,hg=rng.randrange(7,110),rng.randrange(4,80)
  d.ellipse((x-w,y-hg,x+w,y+hg),fill=rng.randrange(100,155))
 h=h.filter(ImageFilter.GaussianBlur(12));d=ImageDraw.Draw(h)
 for _ in range(35000):
  x,y=rng.randrange(size),rng.randrange(size);rr=rng.choice([1,1,1,2,3]);d.ellipse((x,y,x+rr,y+rr),fill=rng.randrange(85,167))
 if name=='Concrete':
  for _ in range(14):
   x,y=rng.randrange(size),rng.randrange(size);points=[(x,y)]
   for k in range(rng.randrange(4,10)):x+=rng.randrange(-13,14);y+=rng.randrange(7,32);points.append((x,y))
   d.line(points,fill=84,width=1)
 a=np.asarray(h,dtype=np.float32);rgb=np.clip(np.array(color)[None,None,:]+(a[:,:,None]-127)*.20,0,255).astype('uint8')
 Image.fromarray(rgb).save(T/(name+'_base_color.png'))
 dy,dx=np.gradient(a/255);n=np.stack([-dx*1.2,dy*1.2,np.ones_like(dx)],2);n/=np.linalg.norm(n,axis=2,keepdims=True)
 Image.fromarray(np.uint8(np.clip(n*.5+.5,0,1)*255)).save(T/(name+'_normal.png'))
 Image.fromarray(np.uint8(np.clip(222+(a-127)*.12,190,240))).save(T/(name+'_roughness.png'))
surface('Stucco',(172,166,144));surface('Concrete',(139,136,119))
im=Image.new('RGB',(2048,480),(48,55,53));d=ImageDraw.Draw(im)
font='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf';bold='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
d.rounded_rectangle((12,12,2036,468),radius=8,outline=(147,146,125),width=4)
d.text((68,20),'SARO',font=ImageFont.truetype(bold,255),fill=(222,215,193),stroke_width=1)
text='SOUTHWEST ASTRONOMICAL RESEARCH OBSERVATORY'
f=ImageFont.truetype(font,53);d.text((68,350),text,font=f,fill=(219,211,187))
d.text((1430,105),'LABORATORIES',font=ImageFont.truetype(bold,40),fill=(196,191,171))
d.text((1430,163),'AND OFFICES',font=ImageFont.truetype(font,37),fill=(196,191,171))
for _ in range(1000):
 x,y=rng.randrange(2048),rng.randrange(480);d.point((x,y),fill=(69,72,62))
im.save(T/'SARO_sign_base_color.png')
rows=[]
def mat(name,color,rough=.8,metal=0,**extra):
 row={'name':'M_SaroSite_'+name,'base_color_linear':color+[1],'roughness':rough,'metallic':metal,'specular':.25,**extra};rows.append(row);return row
for name in ['Stucco','Concrete']:
 row=mat(name,[1,1,1],.93);row['normal_convention']='DirectX';row['normal_strength']=.25
 row['textures']={k:'textures/'+name+'_'+k+'.png' for k in ['base_color','normal','roughness']}
mat('Trim',[.31,.32,.29],.6,.1);mat('Roof',[.10,.105,.09],.95);mat('Interior',[.20,.19,.15],.94)
mat('Wood',[.11,.065,.029],.8);mat('Glass',[.18,.25,.26],.12,0,alpha=.17,two_sided=True)
mat('SignBack',[.034,.045,.04],.87);mat('Sign',[1,1,1],.88,textures={'base_color':'textures/SARO_sign_base_color.png'})
mat('Metal',[.27,.30,.27],.48,.65);mat('DarkMetal',[.045,.052,.047],.65,.65);mat('Ceramic',[.65,.66,.57],.22);mat('Label',[.64,.61,.49],.83)
(R/'materials.json').write_text(json.dumps(rows,indent=2)+'\n')
(R/'texture_provenance.json').write_text(json.dumps({'origin':'Original deterministic Pillow mineral/repair/grain layers and original typeset SARO lettering; no image generation or external photo pixels','ai_generated_bitmap_fields':[], 'font':'DejaVu Sans (installed open font; no font files redistributed)', 'files':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(T.glob('*.png'))}},indent=2)+'\n')
print('SARO_SITE_TEXTURES_READY',len(list(T.glob('*.png'))))
