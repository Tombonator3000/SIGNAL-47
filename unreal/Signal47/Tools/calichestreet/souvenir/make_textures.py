"""Small original, tileable PBR surfaces; Python standard library only."""
import math, struct, zlib
from pathlib import Path

def png(path,w,h,rows):
 def chunk(kind,data): return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
 raw=b''.join(b'\0'+bytes(row) for row in rows)
 path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw,9))+chunk(b'IEND',b''))

def generate(folder):
 folder=Path(folder); folder.mkdir(parents=True,exist_ok=True); n=256
 specs={'stucco':((.60,.53,.41),.88,'stone'),'walnut':((.16,.065,.023),.58,'wood'),'desk_walnut':((.28,.13,.045),.46,'wood'),'concrete':((.30,.28,.24),.92,'stone')}
 for name,(base,rough,kind) in specs.items():
  height=[]
  for y in range(n):
   row=[]; v=y/n*math.tau
   for x in range(n):
    u=x/n*math.tau
    if kind=='wood': h=.50+.13*math.sin(u*36+1.5*math.sin(v*2))+.10*math.sin(u*79+2*math.sin(v))+ .045*math.sin(u*153+math.sin(v*3))
    else: h=.5+.11*math.sin(u*37+math.sin(v*19))+.10*math.sin(v*43+math.sin(u*7))+.06*math.sin(u*97+v*83)+.035*math.sin(u*119-v*103)
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
