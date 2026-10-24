"""Original low-contrast masked microfoliage PBR, Pillow only; no installs or external bitmaps."""
from pathlib import Path
import random
from PIL import Image,ImageDraw,ImageFilter,ImageOps,ImageChops
ROOT=Path(__file__).resolve().parent/'textures'

def noise(n,seed,scales=(8,24,75)):
 rng=random.Random(seed);out=Image.new('L',(n,n),128)
 for k,weight in zip(scales,(.55,.30,.15)):
  field=Image.frombytes('L',(k,k),rng.randbytes(k*k)).resize((n,n),Image.Resampling.BICUBIC)
  out=Image.blend(out,field,weight)
 return out

def save_pbr(stem,color,height,rough):
 ROOT.mkdir(exist_ok=True);color.save(ROOT/(stem+'_color.png'));rough.convert('RGB').save(ROOT/(stem+'_roughness.png'))
 # Central differences in bitmap space; GL green follows upward UV, DX is explicit inverse.
 xp=ImageChops.offset(height,-1,0);xm=ImageChops.offset(height,1,0)
 yp=ImageChops.offset(height,0,-1);ym=ImageChops.offset(height,0,1)
 dx=ImageChops.subtract(xm,xp,scale=2,offset=128);dy=ImageChops.subtract(yp,ym,scale=2,offset=128)
 blue=Image.new('L',height.size,255)
 Image.merge('RGB',(dx,dy,blue)).save(ROOT/(stem+'_normal_gl.png'))
 Image.merge('RGB',(dx,ImageOps.invert(dy),blue)).save(ROOT/(stem+'_normal_dx.png'))

def generate(tile_size=512):
 n=tile_size;atlas=Image.new('RGB',(n*4,n*2));height=Image.new('L',atlas.size,128);rough=Image.new('L',atlas.size,212)
 palette=[(139,145,123),(153,126,79),(83,99,59),(46,61,35),(69,79,45),(135,111,73),(166,151,126),(156,116,82)]
 for tile,color in enumerate(palette):
  field=noise(n,7400+tile)
  col=ImageOps.colorize(field,tuple(max(0,v-12) for v in color),tuple(min(255,v+12) for v in color))
  h=Image.blend(Image.new('L',(n,n),128),field,.25)
  if tile<6:
   vein=Image.new('L',(n,n));draw=ImageDraw.Draw(vein);draw.line([(n//2,0),(n//2,n)],fill=45,width=5);vein=vein.filter(ImageFilter.GaussianBlur(1.8))
   h=ImageChops.add(h,vein)
   col=Image.composite(Image.new('RGB',(n,n),tuple(min(255,v+6) for v in color)),col,vein)
  offset=((tile%4)*n,(tile//4)*n);atlas.paste(col,offset);height.paste(h,offset)
  rough.paste(ImageOps.colorize(field,(196,196,196),(220,220,220)).convert('L'),offset)
 save_pbr('botanical',atlas,height,rough)
 field=noise(n,14201986);rng=random.Random(1420)
 lines=Image.frombytes('L',(31,1),rng.randbytes(31)).resize((n,n),Image.Resampling.BICUBIC)
 h=Image.blend(field,lines,.24);bark=ImageOps.colorize(h,(69,59,41),(88,76,55))
 save_pbr('bark',bark,h,ImageOps.colorize(field,(218,218,218),(233,233,233)).convert('L'))
 from masked_atlas import apply_to_atlas
 apply_to_atlas(ROOT/'botanical_color.png',ROOT.parent/'masked_atlas_stats.json')
 print('ORIGINAL_PBR_READY',str(ROOT))
if __name__=='__main__':generate()
