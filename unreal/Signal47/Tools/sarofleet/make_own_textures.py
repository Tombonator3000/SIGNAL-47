"""Original fleet paint and vinyl PBR. Run in root's granted source-preparation slot."""
from pathlib import Path
import sys,json
import numpy as np
from PIL import Image,ImageFilter
sys.path.insert(0,str(Path(__file__).resolve().parent));from design import WORK

def main():
 out=WORK/'textures';out.mkdir(parents=True,exist_ok=True);rng=np.random.default_rng(4707);n=1024
 def field(size):
  return np.asarray(Image.fromarray(rng.integers(0,256,(size,size),dtype=np.uint8),'L').resize((n,n),Image.Resampling.BICUBIC),dtype=float)/255-.5
 macro=field(12);mid=field(64);fine=field(420);grain=field(900)
 for name,color in [('Paint',(201,205,184)),('Vinyl',(91,103,94)),('Dash',(57,65,58))]:
  amount=macro*5+mid*3+fine*2
  base=np.clip(np.array(color)[None,None,:]+amount[:,:,None],0,255).astype('uint8')
  Image.fromarray(base).save(out/(name+'_base.png'))
  Image.fromarray(np.clip((.77+macro*.025+grain*.08)*255,0,255).astype('uint8')).save(out/(name+'_rough.png'))
  height=grain*.003+fine*.008;gy,gx=np.gradient(height)
  normal=np.stack((-gx*2,gy*2,np.ones_like(gx)),2);normal/=np.linalg.norm(normal,axis=2)[:,:,None]
  Image.fromarray(np.clip((normal*.5+.5)*255,0,255).astype('uint8')).save(out/(name+'_normalDX.png'))
 # Licensed derivative stays in ignored WORK. Repaint only blue fleet panels, preserve
 # source chrome, upholstery, rubber and UV detail; remove unrelated brand artwork.
 src=Path('/home/tom/scratch-bm/Signal47-free/src/pickup_c10_1966/textures/wire_006134006.001_baseColor.png')
 im=Image.open(src).convert('RGBA');a=np.array(im);rgb=a[:,:,:3].astype(float)
 mask=(rgb[:,:,2]>rgb[:,:,0]*1.12)&(rgb[:,:,1]>rgb[:,:,0]*1.10)&(rgb[:,:,0]<85)
 lum=rgb.mean(2);variation=np.clip((lum-54)*.22,-15,15)
 for c,base in enumerate([201,205,184]):a[:,:,c]=np.where(mask,np.clip(base+variation,0,255),a[:,:,c])
 h,w=a.shape[:2]
 for x0,y0,x1,y1 in [( .062,.010,.148,.094),(.231,.041,.270,.084)]:a[int(y0*h):int(y1*h),int(x0*w):int(x1*w),:3]=[201,205,184]
 Image.fromarray(a).save(out/'C10_fleet_repaint.png')
 print('S47_FLEET_ORIGINAL_TEXTURES_READY',out)
if __name__=='__main__':main()
