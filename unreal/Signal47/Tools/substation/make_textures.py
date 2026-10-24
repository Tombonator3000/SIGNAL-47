"""Original industrial PBR and readable1986 print atlas. No downloaded assets.
Only run in the explicitly granted heavy-job slot with bundled NumPy/Pillow.
"""
from pathlib import Path
import json,math
import numpy as np
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parent;N=2048

def noise(rng,n):
 g=np.pad(rng.random((n,n),dtype=np.float32),((0,1),(0,1)),mode='wrap');t=np.arange(N,dtype=np.float32)*n/N;i=t.astype(int);f=t-i;f=f*f*(3-2*f)
 return ((g[i[:,None],i[None,:]]*(1-f)[None,:]+g[i[:,None],i[None,:]+1]*f[None,:])*(1-f)[:,None]+(g[i[:,None]+1,i[None,:]]*(1-f)[None,:]+g[i[:,None]+1,i[None,:]+1]*f[None,:])*f[:,None])
def maps(label,color,height,rough):
 out=R/'textures';out.mkdir(exist_ok=True)
 Image.fromarray(np.clip(color,0,255).astype('uint8'),'RGB').save(out/(label+'_base_color.png'))
 h=np.asarray(Image.fromarray(height.astype('float32'),'F').resize((1024,1024),Image.Resampling.BILINEAR));dx=np.gradient(h,axis=1)*5;dy=np.gradient(h,axis=0)*5
 n=np.stack([-dx,dy,np.ones_like(h)],axis=2);n/=np.linalg.norm(n,axis=2)[:,:,None]
 Image.fromarray(np.clip((n*.5+.5)*255,0,255).astype('uint8'),'RGB').save(out/(label+'_normal.png'))
 Image.fromarray(np.clip(rough*255,0,255).astype('uint8'),'L').resize((1024,1024),Image.Resampling.LANCZOS).save(out/(label+'_roughness.png'))
def font(size,bold=False):
 name='DejaVuSans-Bold.ttf' if bold else 'DejaVuSans.ttf'
 for base in [Path('/usr/share/fonts/truetype/dejavu'),Path('/usr/share/fonts/dejavu')]:
  if (base/name).exists():return ImageFont.truetype(str(base/name),size)
 raise RuntimeError('DejaVu font missing; readable original signage requires font')
def atlas():
 im=Image.new('RGB',(2048,2048),(180,172,147));d=ImageDraw.Draw(im)
 def panel(index,bg,lines):
  x=(index%2)*1024;y=(index//2)*512;d.rectangle((x+8,y+8,x+1016,y+504),fill=bg,outline=(47,46,39),width=9)
  for text,py,size,col in lines:d.text((x+512,y+py),text,font=font(size,True),fill=col,anchor='mt')
  for xx in [x+32,x+992]:
   for yy in [y+32,y+480]:d.ellipse((xx-8,yy-8,xx+8,yy+8),fill=(76,74,63))
 ink=(37,40,34);red=(144,35,23)
 panel(0,(194,188,163),[('CALICHE JUNCTION',70,83,ink),('SUBSTATION',172,105,ink),('CALICHE UTILITIES  /  EST. 1954',350,40,ink)])
 panel(1,(207,196,161),[('DANGER',43,133,red),('HIGH VOLTAGE',219,94,ink),('KEEP OUT',359,73,ink)])
 panel(2,(177,175,157),[('CALICHE UTILITIES',40,69,ink),('THREE PHASE  /  OIL IMMERSED',148,47,ink),('UNIT CJ-47     TYPE OA',250,59,ink),('SERIAL 84-0197     1984',362,49,ink)])
 panel(3,(194,186,152),[('INSPECTION RECORD',38,73,ink),('LAST CHECK   12 MAR 1986',151,49,ink),('R. SALAZAR     CJ-47',248,53,ink),('SERVICE FILE  /  CALICHE DISTRICT',380,38,ink)])
 panel(4,(90,102,84),[('CALICHE JUNCTION',36,65,(209,201,166)),('FEEDER  03',141,100,(217,210,176)),('STATUS      LOCAL      ALARM',301,43,(219,210,179)),('CONTROL  /  CJ-47',412,40,(215,207,171))])
 for index,title,units in [(5,'OIL TEMP','DEG C'),(6,'OIL LEVEL','NORMAL')]:
  x=index%2*1024;y=index//2*512;d.rectangle((x+8,y+8,x+1016,y+504),fill=(194,189,164));cx=x+512;cy=y+284;r=173
  d.ellipse((cx-r,cy-r,cx+r,cy+r),fill=(218,212,184),outline=ink,width=9)
  for k in range(21):
   a=math.radians(210-k*12);r1=r-15;r2=r1-(28 if k%5==0 else 13);d.line((cx+math.cos(a)*r1,cy-math.sin(a)*r1,cx+math.cos(a)*r2,cy-math.sin(a)*r2),fill=ink,width=5)
  d.text((cx,y+38),title,font=font(53,True),fill=ink,anchor='mt');d.text((cx,cy+88),units,font=font(29,True),fill=ink,anchor='mt')
 panel(7,(164,159,136),[('CJ  /  47',76,138,ink),('CALICHE DISTRICT',298,61,ink)])
 # Sparse age marks remain much smaller than letters.
 rng=np.random.default_rng(198647)
 for _ in range(260):
  x=int(rng.integers(0,2048));y=int(rng.integers(0,2048));d.line((x,y,x+int(rng.integers(2,17)),y+1),fill=(132,126,104),width=1)
 out=R/'textures';im.save(out/'PrintAtlas_base_color.png')
 Image.new('RGB',(1024,1024),(128,128,255)).save(out/'PrintAtlas_normal.png');Image.new('L',(1024,1024),203).save(out/'PrintAtlas_roughness.png')
def main():
 rng=np.random.default_rng(198647);a=noise(rng,11);b=noise(rng,72);c=rng.random((N,N),dtype=np.float32)
 wear=np.clip((.22-a)*2.4,0,.20);tone=.87+.20*a+.07*(b-.5)+.03*(c-.5)
 color=np.array([84,94,74])[None,None,:]*tone[:,:,None];color+=wear[:,:,None]*np.array([52,21,3])[None,None,:]
 maps('OlivePaint',color,b*.045+c*.024+wear*.12,.62+.15*a)
 a=noise(rng,83);b=noise(rng,240);c=noise(rng,13);tone=.88+.19*a+.08*(b-.5)
 maps('Galvanized',np.array([142,148,145])[None,None,:]*tone[:,:,None],a*.08+b*.04,.43+.19*c)
 a=noise(rng,13);b=noise(rng,125);c=rng.random((N,N),dtype=np.float32)
 maps('Concrete',np.array([146,139,125])[None,None,:]*(.88+.18*a+.12*(b-.5))[:,:,None],b*.13+c*.06,.86+.10*a)
 a=noise(rng,12);b=noise(rng,170);c=rng.random((N,N),dtype=np.float32)
 stones=np.clip((b-.56)*2.5,0,.8)
 maps('Gravel',np.array([129,117,99])[None,None,:]*(.85+.20*a+.27*(b-.5))[:,:,None]+stones[:,:,None]*24,b*.32+stones*.38+c*.05,.91+.07*a)
 atlas()
 rows=[]
 def row(name,rgb,metal,rough,label=None,normal=.2,**kw):
  d={'name':'M_Substation_'+name,'base_color_linear':[*rgb,1],'metallic':metal,'roughness':rough,'specular':.3,'normal_strength':normal,'normal_convention':'DirectX','uv_scale':1,'emission':[0,0,0,1],'emission_strength':0,**kw}
  if label:d['textures']={role:'textures/'+label+'_'+role+'.png' for role in ['base_color','normal','roughness']}
  rows.append(d)
 row('OlivePaint',[1,1,1],.16,.7,'OlivePaint',.22)
 row('GalvanizedSteel',[1,1,1],.86,.54,'Galvanized',.25)
 row('DarkSteel',[.055,.061,.052],.72,.68)
 row('PorcelainBrown',[.16,.075,.036],0,.25)
 row('Copper',[.32,.17,.069],.92,.54)
 row('Rubber',[.012,.016,.013],0,.83)
 row('Concrete',[1,1,1],0,.94,'Concrete',.33)
 row('Gravel',[1,1,1],0,.98,'Gravel',.48)
 row('PrintAtlas',[1,1,1],.1,.8,'PrintAtlas',.05)
 row('LampAmber',[1,.32,.028],0,.28,emission=[1,.22,.018,1],emission_strength=2)
 (R/'materials.json').write_text(json.dumps(rows,indent=2)+'\n')
 print('SUBSTATION_ORIGINAL_PBR_READY',len(rows),'materials,5original texture sets')
if __name__=='__main__':main()
