"""Original restrained 1986 office PBR, paper and English label atlas.

No downloaded model, texture or photograph. Run only in root's heavy-job slot.
"""
from pathlib import Path
import importlib.util, json
import numpy as np
from PIL import Image, ImageDraw
R=Path(__file__).resolve().parent
_s=importlib.util.spec_from_file_location('s47_archive_font_reference',R.parent/'substation'/'make_textures.py')
base=importlib.util.module_from_spec(_s);_s.loader.exec_module(base)
N=1024

def field(rng,nx,ny):
 g=np.pad(rng.random((ny,nx),dtype=np.float32),((0,1),(0,1)),mode='wrap')
 x=np.arange(N,dtype=np.float32)*nx/N;y=np.arange(N,dtype=np.float32)*ny/N
 ix=x.astype(int);iy=y.astype(int);fx=x-ix;fy=y-iy;fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy)
 return (g[iy[:,None],ix[None,:]]*(1-fx)[None,:]+g[iy[:,None],ix[None,:]+1]*fx[None,:])*(1-fy)[:,None]+(g[iy[:,None]+1,ix[None,:]]*(1-fx)[None,:]+g[iy[:,None]+1,ix[None,:]+1]*fx[None,:])*fy[:,None]

def maps(name,color,height,rough):
 out=R/'textures';Image.fromarray(np.clip(color,0,255).astype('uint8'),'RGB').save(out/(name+'_base_color.png'))
 h=np.asarray(Image.fromarray(height.astype('float32'),'F').resize((512,512),Image.Resampling.BILINEAR))
 dx=np.gradient(h,axis=1)*4;dy=np.gradient(h,axis=0)*4;n=np.stack([-dx,dy,np.ones_like(h)],axis=2);n/=np.linalg.norm(n,axis=2)[:,:,None]
 Image.fromarray(np.clip((n*.5+.5)*255,0,255).astype('uint8'),'RGB').save(out/(name+'_normal.png'))
 Image.fromarray(np.clip(rough*255,0,255).astype('uint8'),'L').resize((512,512),Image.Resampling.LANCZOS).save(out/(name+'_roughness.png'))

def atlas():
 im=Image.new('RGB',(2048,2048),(196,190,167));d=ImageDraw.Draw(im);ink=(42,43,38)
 labels=[
  ['STATION 01','FIELD RECORDS','1947'],['SARO / RECORDS','MAINTENANCE','1978 - 1985'],
  ['SARO / RECORDS','FIELD SURVEYS','REFERENCE COPY'],['SARO / RECORDS','INSTRUMENTS','SERVICE LOGS'],
  ['SARO / ARCHIVE','FIXED-POINT SURVEYS','RETURN TO RECORDS'],['SARO','FIELD FILES','INDEX 01 - 12'],
  ['MAINTENANCE','REFERENCE CARDS','01 - 47'],['MAINTENANCE','REFERENCE LINEAGE','RECORDS INDEX'],
  ['SARO','SERVICE','COPY'],['ARCHIVE',''],['047','10-AUG-79','NOT FOR CATALOGUE'],
  ['KSU-12','KEY SERVICE UNIT'],['SARO','CALL RECORDER','CR-12'],
  ['SERVICE COPIES','REFERENCE RECORDS','SARO'],['FIELD RECORD','CONTROL COPY','1986'],['SARO','RECORDS ROOM','NO SMOKING'],
 ]
 for cell,lines in enumerate(labels):
  x=(cell%4)*512;y=(cell//4)*512
  d.rectangle((x+10,y+10,x+502,y+502),fill=(207,199,173),outline=ink,width=4)
  if cell==11:
   # Native lamp centres sit above the extension card. Printed labels register
   # at the same 4 x 56px positions as archive047.ts, without painted fake lights.
   d.text((x+256,y+39),'KSU-12',font=base.font(50,True),anchor='mt',fill=ink)
   d.text((x+256,y+99),'KEY SERVICE UNIT',font=base.font(23,True),anchor='mt',fill=ink)
   for j,txt in enumerate(['TRK 1','TRK 2','INT','HOLD']):d.text((x+80+j*112,y+140),txt,font=base.font(20,True),anchor='mt',fill=ink)
   # Blank register strip y165..225 allows native lenses to remain the only lights.
   for j,txt in enumerate(['EXT 201  CONTROL ROOM','EXT 214  RECORDS','EXT 220  OPERATIONS','EXT 230  PHOTO LAB','TRK 1    OUTSIDE LINE','TRK 2    SUPERVISOR']):d.text((x+40,y+265+j*29),txt,font=base.font(21),fill=ink)
   continue
  for j,txt in enumerate(lines):
   size=[47,31,29][min(j,2)]
   while base.font(size,True).getbbox(txt)[2]>448:size-=1
   d.text((x+256,y+83+j*128),txt,font=base.font(size,j==0),anchor='mt',fill=ink)
  if cell not in [8,9]:d.line((x+42,y+442,x+470,y+442),fill=(111,105,87),width=2)
 # Wear stays away from the characters; no dense noisy fake lettering.
 rng=np.random.default_rng(1986047)
 for _ in range(130):
  x=int(rng.integers(0,2048));y=int(rng.integers(0,2048));d.line((x,y,x+int(rng.integers(2,12)),y),fill=(169,160,137),width=1)
 out=R/'textures';im.save(out/'PrintAtlas_base_color.png');Image.new('RGB',(512,512),(128,128,255)).save(out/'PrintAtlas_normal.png');Image.new('L',(512,512),219).save(out/'PrintAtlas_roughness.png')

def main():
 (R/'textures').mkdir(exist_ok=True);rng=np.random.default_rng(1986047)
 for name,rgb,metal in [('SteelGreen',[90,106,91],.43),('SteelOlive',[101,111,93],.35),('BeigeEnamel',[174,166,145],.25),('Kraft',[133,111,79],0)]:
  a=field(rng,11,7);b=field(rng,93,76);fine=rng.random((N,N),dtype=np.float32)
  tone=.94+.08*(a-.5)+.025*(b-.5);color=np.array(rgb)[None,None,:]*tone[:,:,None]
  maps(name,color,b*.018+fine*.006,np.clip(.64+.10*a+(.12 if name=='Kraft'else 0),0,1))
 a=field(rng,3,37);b=field(rng,9,210);color=np.array([83,65,45])[None,None,:]*(.91+.16*a+.05*(b-.5))[:,:,None]
 maps('Walnut',color,a*.045+b*.008,.65+.14*a)
 atlas();rows=[]
 def row(name,rgb,metal,rough,texture=None,normal=.12):
  r={'name':'M_ArchiveProps_'+name,'base_color_linear':[*rgb,1],'metallic':metal,'roughness':rough,'specular':.27,'normal_strength':normal,'normal_convention':'DirectX','emission':[0,0,0,1],'emission_strength':0,'two_sided':False}
  if texture:r['textures']={role:'textures/'+texture+'_'+role+'.png'for role in ['base_color','normal','roughness']}
  rows.append(r)
 for name,metal,rough in [('SteelGreen',.43,.72),('SteelOlive',.35,.73),('BeigeEnamel',.25,.68),('Kraft',0,.88),('Walnut',0,.75),('PrintAtlas',0,.86)]:row(name,[1,1,1],metal,rough,name,.10 if name!='PrintAtlas'else .025)
 for name,rgb,metal,rough in [('DarkSteel',[.045,.055,.050],.64,.71),('Metal',[.34,.38,.36],.85,.40),('Brass',[.25,.16,.055],.72,.57),('Rubber',[.012,.015,.013],0,.92),('Paper',[.66,.62,.49],0,.94),('KraftEdge',[.32,.25,.15],0,.91),('GreyBoard',[.27,.29,.26],0,.90),('BookBlue',[.026,.061,.084],0,.84),('BookBrown',[.10,.040,.021],0,.88)]:row(name,rgb,metal,rough)
 (R/'materials.json').write_text(json.dumps(rows,indent=2)+'\n')
 (R/'texture_authoring.json').write_text(json.dumps({'source':'R1-original-SARO-records-room-1986','base_color_px':1024,'atlas_px':2048,'normal_roughness_px':512,'normal_convention':'DirectX','assets':'Original deterministic paint/wood/cardboard and fictional English labels','external_assets':False,'native_visual_status':'UNVERIFIED'},indent=2)+'\n')
 print('ARCHIVEPROPS_TEXTURES_READY',len(rows))
if __name__=='__main__':main()
