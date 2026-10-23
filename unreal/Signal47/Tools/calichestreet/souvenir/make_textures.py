"""Small original, tileable PBR surfaces; Python standard library only."""
import math, struct, zlib, random
from pathlib import Path

# Shader-only tiling, never a change to mesh UVs, positions or material slots.
# The legacy *_normal.png is OpenGL for Blender. UE uses *_normal_dx.png.
STONE_SETTINGS={'stucco':{'uv_scale':1.0,'normal_strength':.18,'resolution':512},
                'concrete':{'uv_scale':1.0,'normal_strength':.18,'resolution':512}}

def png(path,w,h,rows):
 def chunk(kind,data): return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
 raw=b''.join(b'\0'+bytes(row) for row in rows)
 path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw,9))+chunk(b'IEND',b''))

def _fade(t): return t*t*t*(t*(t*6-15)+10)

def _value_noise(x,y,cells,grid):
 gx=x*cells; gy=y*cells; ix=int(gx)%cells; iy=int(gy)%cells
 tx=_fade(gx-math.floor(gx)); ty=_fade(gy-math.floor(gy))
 a=grid[iy][ix]; b=grid[iy][(ix+1)%cells]
 c=grid[(iy+1)%cells][ix]; d=grid[(iy+1)%cells][(ix+1)%cells]
 return (a+(b-a)*tx)*(1-ty)+(c+(d-c)*tx)*ty

def _stone(folder,name,base,rough):
 """Periodic value noise plus fine aggregate. No directional sine patterns."""
 n=STONE_SETTINGS[name]['resolution']; rng=random.Random(1420 if name=='stucco' else 1947)
 layers=[]
 for cells,weight in [(8,.08),(16,.12),(32,.18),(64,.24),(128,.23),(256,.15)]:
  layers.append((cells,weight,[[rng.uniform(-1,1) for _ in range(cells)] for _ in range(cells)]))
 grain=[[rng.uniform(-1,1) for _ in range(n)] for _ in range(n)]
 h=[]; tone=[]
 for y in range(n):
  heights=[]; tones=[]
  for x in range(n):
   values=[_value_noise(x/n,y/n,cells,grid) for cells,_,grid in layers]
   aggregate=sum(weight*v for (_,weight,_),v in zip(layers,values))
   heights.append(.5+.16*aggregate+.035*grain[y][x])
   # Pigment variation is deliberately quiet and independent of grain normals.
   tones.append(.012*values[0]+.006*values[2]+.003*grain[y][x])
  h.append(heights); tone.append(tones)
 colors=[]; gl=[]; dx=[]; rr=[]
 for y in range(n):
  cr=[]; gr=[]; dr=[]; roughrow=[]
  for x in range(n):
   for b in base:
    linear=max(0,min(1,b*(1+tone[y][x])))
    srgb=12.92*linear if linear<=.0031308 else 1.055*linear**(1/2.4)-.055
    cr.append(round(255*srgb))
   sx=(h[y][(x+1)%n]-h[y][(x-1)%n])*.9
   sy=(h[(y+1)%n][x]-h[(y-1)%n][x])*.9
   length=math.sqrt(sx*sx+sy*sy+1)
   rgb=[round(255*(-sx/length*.5+.5)),round(255*(-sy/length*.5+.5)),round(255*(1/length*.5+.5))]
   gr.extend(rgb); dr.extend([rgb[0],255-rgb[1],rgb[2]])
   q=round(255*max(0,min(1,rough+(h[y][x]-.5)*.10))); roughrow.extend([q,q,q])
  colors.append(cr); gl.append(gr); dx.append(dr); rr.append(roughrow)
 png(folder/(name+'_color.png'),n,n,colors)
 png(folder/(name+'_normal.png'),n,n,gl)
 png(folder/(name+'_normal_gl.png'),n,n,gl)
 png(folder/(name+'_normal_dx.png'),n,n,dx)
 png(folder/(name+'_roughness.png'),n,n,rr)

def generate(folder,only=None):
 folder=Path(folder); folder.mkdir(parents=True,exist_ok=True); n=256
 specs={'stucco':((.60,.53,.41),.88,'stone'),'walnut':((.16,.065,.023),.58,'wood'),'desk_walnut':((.28,.13,.045),.46,'wood'),'concrete':((.30,.28,.24),.92,'stone')}
 for name,(base,rough,kind) in specs.items():
  if only is not None and name not in only: continue
  if kind=='stone':
   _stone(folder,name,base,rough); continue
  height=[]
  for y in range(n):
   row=[]; v=y/n*math.tau
   for x in range(n):
    u=x/n*math.tau
    h=.50+.13*math.sin(u*36+1.5*math.sin(v*2))+.10*math.sin(u*79+2*math.sin(v))+ .045*math.sin(u*153+math.sin(v*3))
    row.append(h)
   height.append(row)
  color=[]; normal=[]; roughrows=[]
  for y in range(n):
   c=[]; nr=[]; r=[]
   for x in range(n):
    h=height[y][x]; k=1+(h-.5)*(.55 if kind=='wood' else .26)
    for b in base: c.append(round(255*(max(0,min(1,b*k))**(1/2.2))))
    dx=(height[y][(x+1)%n]-height[y][(x-1)%n])*.8; dy=(height[(y+1)%n][x]-height[(y-1)%n][x])*.8
    l=math.sqrt(dx*dx+dy*dy+1); nr.extend([round(255*(-dx/l*.5+.5)),round(255*(-dy/l*.5+.5)),round(255*(1/l*.5+.5))])
    q=round(255*max(0,min(1,rough+(h-.5)*.12))); r.extend([q,q,q])
   color.append(c); normal.append(nr); roughrows.append(r)
  png(folder/(name+'_color.png'),n,n,color)
  png(folder/(name+'_normal_gl.png'),n,n,normal)
  dxrows=[[255-value if i%3==1 else value for i,value in enumerate(row)] for row in normal]
  png(folder/(name+'_normal_dx.png'),n,n,dxrows)
  png(folder/(name+'_roughness.png'),n,n,roughrows)

if __name__=='__main__': generate(Path(__file__).parent/'textures')
