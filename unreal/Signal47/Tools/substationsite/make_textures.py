"""Original weathered timber, oxidized sheet-metal and maintenance print atlas.
Only run after the explicit shared heavy-job grant. No external image assets.
"""
from pathlib import Path
import importlib.util,json,math
import numpy as np
from PIL import Image,ImageDraw
R=Path(__file__).resolve().parent
_spec=importlib.util.spec_from_file_location('s47_substation_texture_recipe',R.parent/'substation'/'make_textures.py');base=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(base);base.R=R
N=2048

def field(rng,nx,ny):
 g=np.pad(rng.random((ny,nx),dtype=np.float32),((0,1),(0,1)),mode='wrap');x=np.arange(N,dtype=np.float32)*nx/N;y=np.arange(N,dtype=np.float32)*ny/N;ix=x.astype(int);iy=y.astype(int);fx=x-ix;fy=y-iy;fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy)
 return ((g[iy[:,None],ix[None,:]]*(1-fx)[None,:]+g[iy[:,None],ix[None,:]+1]*fx[None,:])*(1-fy)[:,None]+(g[iy[:,None]+1,ix[None,:]]*(1-fx)[None,:]+g[iy[:,None]+1,ix[None,:]+1]*fx[None,:])*fy[:,None])
def print_atlas():
 im=Image.new('RGB',(2048,2048),(174,165,140));d=ImageDraw.Draw(im);ink=(36,39,33)
 panels=[('CJ  /  LINE','CALICHE UTILITIES','1986'),('MAINTENANCE','CALICHE UTILITIES','EQUIPMENT STORE'),('DISTRICT  /  CJ','CABLE AND TOOLS','PROPERTY OF CALICHE UTILITIES')]
 for i,lines in enumerate(panels):
  x=i%2*1024;y=i//2*512;d.rectangle((x+8,y+8,x+1016,y+504),fill=(181,172,145),outline=ink,width=8)
  for j,text in enumerate(lines):d.text((x+512,y+65+j*145),text,font=base.font([75,51,38][j],True),anchor='mt',fill=ink)
  for xx in [x+30,x+994]:
   for yy in [y+30,y+482]:d.ellipse((xx-7,yy-7,xx+7,yy+7),fill=(89,84,69))
 out=R/'textures';im.save(out/'PrintAtlas_base_color.png');Image.new('RGB',(1024,1024),(128,128,255)).save(out/'PrintAtlas_normal.png');Image.new('L',(1024,1024),210).save(out/'PrintAtlas_roughness.png')
def main():
 (R/'textures').mkdir(exist_ok=True);rng=np.random.default_rng(198612)
 grain=field(rng,210,5);broad=field(rng,17,3);fine=field(rng,510,23);knots=field(rng,9,8);checks=np.clip((.21-grain)*3.0,0,.5)
 tone=.80+.25*broad+.22*(grain-.5)+.055*(fine-.5)-checks*.25
 color=np.array([117,101,78])[None,None,:]*tone[:,:,None];gray=(knots-.5)*16;color+=gray[:,:,None]
 base.maps('WeatheredWood',color,grain*.15+fine*.035-checks*.12,.78+.16*broad)
 # Sparse connected corrosion follows sheet drainage; broadX3/Y1 fields
 # keep it elongated, while a weakX5/Y2 field breaks the edges organically.
 # Exact quantile controls visible coverage instead of a dense speckle carpet.
 broad=field(rng,3,1)*.85+field(rng,5,2)*.15;grain=field(rng,90,120)
 threshold=float(np.quantile(broad,.21));mask=np.clip((threshold+.035-broad)/.07,0,1);rust=mask*mask*(3-2*mask)
 steel=np.array([137,142,135])[None,None,:]*(.91+.09*broad+.025*(grain-.5))[:,:,None];oxide=np.array([117,81,53])[None,None,:]
 color=steel*(1-rust[:,:,None]*.90)+oxide*(rust[:,:,None]*.90)
 base.maps('CorrugatedSteel',color,grain*.025+rust*.065,.67+.17*rust)
 rust2=field(rng,3,1)*.78+field(rng,5,2)*.22;pitting=field(rng,95,130)
 threshold2=float(np.quantile(rust2,.24));mask=np.clip((threshold2+.035-rust2)/.07,0,1);weight=mask*mask*(3-2*mask)
 metal=np.array([132,138,130])[None,None,:]*(.92+.08*rust2+.02*(pitting-.5))[:,:,None]
 color=metal*(1-weight[:,:,None]*.92)+np.array([115,78,49])[None,None,:]*(weight[:,:,None]*.92)
 base.maps('RoofRust',color,pitting*.025+weight*.07,.69+.18*weight)
 (R/'texture_authoring.json').write_text(json.dumps({'revision':'R1b-sparse-gravity-rust','corrugated_visible_rust_fraction':float(np.mean(rust>.5)),'roof_visible_rust_fraction':float(np.mean(weight>.5)),'intended_coverage':[.15,.25],'macro_grid_main':[3,1],'edge_break_grid':[5,2],'unchanged_geometry':True,'native_visual_status':'UNVERIFIED'},indent=2)+'\n')
 print_atlas();old=json.loads((R.parent/'substation'/'materials.json').read_text());rows=[]
 for short in ['GalvanizedSteel','DarkSteel','PorcelainBrown','Concrete','Gravel']:
  row=dict(next(x for x in old if x['name']=='M_Substation_'+short));row['name']='M_SubstationSite_'+short
  if row.get('textures'):row['textures']={role:'../substation/'+path for role,path in row['textures'].items()}
  rows.append(row)
 for short,label,metal,rough,normal in [('Wood','WeatheredWood',0,.88,.36),('CorrugatedSteel','CorrugatedSteel',.62,.82,.16),('RoofRust','RoofRust',.51,.89,.18),('PrintAtlas','PrintAtlas',.1,.83,.03)]:
  rows.append({'name':'M_SubstationSite_'+short,'base_color_linear':[1,1,1,1],'metallic':metal,'roughness':rough,'specular':.28,'normal_strength':normal,'normal_convention':'DirectX','uv_scale':1,'two_sided':short in ['CorrugatedSteel','RoofRust'],'textures':{role:'textures/'+label+'_'+role+'.png' for role in ['base_color','normal','roughness']},'emission':[0,0,0,1],'emission_strength':0})
 for short,rgb,metal,rough in [('CableBlack',[.016,.021,.018],0,.91),('SafetyOchre',[.37,.24,.045],.1,.8)]:rows.append({'name':'M_SubstationSite_'+short,'base_color_linear':[*rgb,1],'metallic':metal,'roughness':rough,'specular':.25,'emission':[0,0,0,1],'emission_strength':0})
 (R/'materials.json').write_text(json.dumps(rows,indent=2)+'\n');print('SUBSTATION_SITE_ORIGINAL_PBR_READY',len(rows),'materials')
if __name__=='__main__':main()
