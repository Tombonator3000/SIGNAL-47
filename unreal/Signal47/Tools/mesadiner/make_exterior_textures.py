"""Original Mesa1986 exterior PBR, authored without downloaded maps.

Run only in parent's exclusive heavy slot, using bundled NumPy/Pillow.
main() writes our five texturefamilies only; the parent merges material rows.
Textures are unlit albedo, DirectX tangent normals, scalar linear roughness.
"""
from pathlib import Path
import math
import shutil
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
R=Path(__file__).resolve().parent
N=2048


def noise(rng,n):
 g=np.pad(rng.random((n,n),dtype=np.float32),((0,1),(0,1)),mode='wrap')
 t=np.arange(N,dtype=np.float32)*n/N;i=t.astype(int);f=t-i;f=f*f*(3-2*f)
 return ((g[i[:,None],i[None,:]]*(1-f)[None,:]+g[i[:,None],i[None,:]+1]*f[None,:])*(1-f)[:,None]+(g[i[:,None]+1,i[None,:]]*(1-f)[None,:]+g[i[:,None]+1,i[None,:]+1]*f[None,:])*f[:,None])


def save(label,color,height,rough,strength=1):
 out=R/'textures';out.mkdir(parents=True,exist_ok=True)
 Image.fromarray(np.clip(color,0,255).astype('uint8'),'RGBA' if color.shape[2]==4 else 'RGB').save(out/(label+'_base_color.png'))
 # Periodic central differences keep the tangent normal tile edge seamless.
 h=np.asarray(Image.fromarray(np.asarray(height,dtype='float32'),'F').resize((1024,1024),Image.Resampling.BILINEAR))
 dx=(np.roll(h,-1,axis=1)-np.roll(h,1,axis=1))*.5*strength
 dy=(np.roll(h,-1,axis=0)-np.roll(h,1,axis=0))*.5*strength
 normal=np.stack([-dx,dy,np.ones_like(h)],axis=2);normal/=np.linalg.norm(normal,axis=2)[:,:,None]
 Image.fromarray(np.clip((normal*.5+.5)*255,0,255).astype('uint8'),'RGB').save(out/(label+'_normal.png'))
 Image.fromarray(np.clip(rough*255,0,255).astype('uint8'),'L').resize((1024,1024),Image.Resampling.LANCZOS).save(out/(label+'_roughness.png'))


def material_rows_extra():
 rows=[]
 for name,metal,rough,normal in [('Asphalt',0,.95,.40),('Concrete',0,.90,.30),('Gravel',0,.98,.44),('TruckPaint',.16,.62,.15),('SignTeal',.30,.62,.18)]:
  rows.append({'name':'M_Mesa_'+name,'base_color_linear':[1,1,1,1],'metallic':metal,'roughness':rough,'specular':.30,'normal_strength':normal,'normal_convention':'DirectX','uv_scale':1,'emission':[0,0,0,1],'emission_strength':0,'textures':{role:'textures/'+name+'_'+role+'.png' for role in ['base_color','normal','roughness']}})
 for row in rows:
  if row['name']=='M_Mesa_Asphalt':row.update(alpha_mode='dithered_masked',opacity_from_base_color=True,alpha_cutoff=.333)
 for name,prefix,masked in [('DryFoliage','botanical',True),('PlantBark','bark',False)]:
  rows.append({'name':'M_Mesa_'+name,'base_color_linear':[1,1,1,1],'metallic':0,'roughness':.84,'specular':.22,'normal_strength':.2,'normal_convention':'DirectX','uv_scale':1,'alpha':1,'alpha_mode':'masked' if masked else 'opaque','two_sided':masked,'opacity_from_base_color':masked,'alpha_cutoff':.333,'mip_alpha_coverage':masked,'emission':[0,0,0,1],'emission_strength':0,'textures':{'base_color':'textures/'+prefix+'_color.png','roughness':'textures/'+prefix+'_roughness.png','normal':'textures/'+prefix+'_normal_dx.png'}})
 return rows


def crack_mask(rng,count,width=2):
 im=Image.new('L',(N,N),0);d=ImageDraw.Draw(im)
 for k in range(count):
  x=float(rng.integers(0,N));y=float(rng.integers(0,N));angle=float(rng.random()*math.tau);points=[(x,y)]
  for j in range(int(rng.integers(4,9))):
   angle+=float(rng.uniform(-.60,.60));step=float(rng.uniform(16,65));x+=math.cos(angle)*step;y+=math.sin(angle)*step;points.append((x,y))
  # Wrapped copies author a tiling feature without erasing the ragged line ends.
  for ox in [-N,0,N]:
   for oy in [-N,0,N]:d.line([(xx+ox,yy+oy) for xx,yy in points],fill=255,width=width)
 return np.asarray(im,dtype=np.float32)/255


def main():
 rng=np.random.default_rng(19861024)
 a=noise(rng,6);b=noise(rng,52);c=noise(rng,410);grain=rng.random((N,N),dtype=np.float32)
 cracks=crack_mask(rng,11,2);aggregate=np.clip((c-.49)*1.9,0,.7)
 # Unique whole-lot UV: physical dust transition40-70cm, not a repeated dark mat.
 yy,xx=np.indices((N,N),dtype=np.float32);sx=2.8+(xx+.5)*13.9/N;sy=16-(yy+.5)*32/N
 inset=np.minimum(np.minimum(sx-3.65,16.0-sx),np.minimum(sy+15.25,15.20-sy))
 cover=np.clip((inset+(b-.5)*.80)/(.48+.22*a),0,1)
 pockets=np.exp(-(((sx-5.9)/1.6)**2+((sy+12.7)/1.9)**2))* .62
 pockets+=np.exp(-(((sx-14.7)/1.05)**2+((sy-6.3)/2.4)**2))* .60
 cover*=1-np.clip(pockets*(.65+.6*b),0,.9)
 dust=np.clip(.15+.20*a+.18*(b-.5)+(1-cover)*.63+pockets*.32,0,.95)
 color=np.array([66,64,57],dtype=np.float32)[None,None,:]*(.91+.18*a+.12*(b-.5))[:,:,None]
 color+=aggregate[:,:,None]*np.array([34,32,24])[None,None,:]+(grain-.5)[:,:,None]*9
 color=color*(1-cracks[:,:,None]*.45)
 gravel=np.array([132,119,96],dtype=np.float32)[None,None,:]*(.93+.10*a+.12*(c-.5))[:,:,None]
 color=color*(1-dust[:,:,None])+gravel*dust[:,:,None]
 color=np.concatenate([color,(cover*255)[:,:,None]],axis=2)
 save('Asphalt',color,c*.36+grain*.12-cracks*.23,.88+.08*b+cracks*.03,4.5)
 # Concrete: restrained clouded cement, random pores, small aggregate, no sine bands.
 a=noise(rng,8);b=noise(rng,108);c=noise(rng,430);grain=rng.random((N,N),dtype=np.float32)
 pores=np.clip((grain-.985)*55,0,1);craze=crack_mask(rng,4,1)
 color=np.array([151,143,125],dtype=np.float32)[None,None,:]*(.93+.12*a+.08*(b-.5))[:,:,None]-pores[:,:,None]*18-craze[:,:,None]*23
 save('Concrete',color,b*.08+c*.12-pores*.08-craze*.12,.85+.10*a,4)
 # Angular caliche-gravel grains sit in darker compacted fines, not rounded polka dots.
 a=noise(rng,8);b=noise(rng,125);c=noise(rng,390);grain=rng.random((N,N),dtype=np.float32)
 stones=np.clip((b-.47)*2.3,0,.85);tone=.90+.12*a+.24*(b-.5)+.12*(c-.5)
 color=np.array([132,119,96],dtype=np.float32)[None,None,:]*tone[:,:,None]+stones[:,:,None]*np.array([31,29,24])[None,None,:]
 save('Gravel',color,b*.37+stones*.38+c*.14+grain*.035,.91+.07*a,5.2)
 # Offwhite rolled enamel: large coherent sun fade + isolated small edge scratches.
 a=noise(rng,5);b=noise(rng,95);c=noise(rng,310);scratch=crack_mask(rng,17,1)
 wear=np.clip((.23-a)*1.5,0,.22);tone=.95+.07*a+.035*(b-.5)
 color=np.array([201,197,174],dtype=np.float32)[None,None,:]*tone[:,:,None]
 color=color*(1-wear[:,:,None])+np.array([142,132,107])[None,None,:]*wear[:,:,None]
 color-=scratch[:,:,None]*34
 save('TruckPaint',color,b*.018+c*.014+scratch*.025,.49+.19*a+scratch*.12,3)
 # Muted teal sign enamel, subtle faded patches and dark sparse finish losses.
 a=noise(rng,6);b=noise(rng,86);c=noise(rng,260);scratch=crack_mask(rng,12,2)
 wear=np.clip((.28-a)*1.4,0,.30);tone=.88+.18*a+.045*(b-.5)
 color=np.array([47,96,91],dtype=np.float32)[None,None,:]*tone[:,:,None]
 color=color*(1-wear[:,:,None])+np.array([113,119,98])[None,None,:]*wear[:,:,None]
 color=color*(1-scratch[:,:,None]*.45)
 save('SignTeal',color,b*.028+c*.017-scratch*.04,.53+.22*a,3)
 # Exact byte copies retain original CN3 masked atlas/UV compatibility.
 source=R.parent/'calichenature/vegetation/textures'
 for prefix in ['botanical','bark']:
  for suffix in ['color','roughness','normal_dx']:
   name=prefix+'_'+suffix+'.png';shutil.copyfile(source/name,R/'textures'/name)
 print('MESA_EXTERIOR_ORIGINAL_PBR_READY 5families 15maps plus6originalCalicheatlasCopies')


if __name__=='__main__':main()
