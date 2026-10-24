"""Small original metre-based mesh authoring helpers. No Blender dependency."""
import math
from collections import defaultdict

def add(a,b):return tuple(x+y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def mul(a,s):return tuple(x*s for x in a)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def unit(a):return mul(a,1/math.sqrt(dot(a,a)))

class Mesh:
 def __init__(self,name):self.name=name;self.v=[];self.f=[];self.uv=[];self.mat=[];self.smooth=[];self.tags=[];self.tag='Assembly'
 def face(self,ps,mat,uv=None,smooth=False):
  ps=list(ps);n=cross(sub(ps[1],ps[0]),sub(ps[2],ps[0]));assert dot(n,n)>1e-16,(self.tag,'degenerate face')
  if uv is None:
   axis=max(range(3),key=lambda k:abs(n[k]));axes=([1,2] if axis==0 else [0,2] if axis==1 else [0,1]);scale=2 if mat in ['OlivePaint','Concrete'] else 1
   uv=[(p[axes[0]]/scale,p[axes[1]]/scale) for p in ps];off=[math.floor(min(t[k] for t in uv)) for k in [0,1]];uv=[(t[0]-off[0],t[1]-off[1]) for t in uv]
  start=len(self.v);self.v.extend(ps);self.f.append(list(range(start,start+len(ps))));self.uv.append(uv);self.mat.append('M_Substation_'+mat);self.smooth.append(smooth);self.tags.append(self.tag)
 def box(self,name,center,size,mat):
  self.tag=name;x,y,z=center;a,b,c=[s/2 for s in size]
  v=[(x-a,y-b,z-c),(x+a,y-b,z-c),(x+a,y+b,z-c),(x-a,y+b,z-c),(x-a,y-b,z+c),(x+a,y-b,z+c),(x+a,y+b,z+c),(x-a,y+b,z+c)]
  for ids in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]:self.face([v[i] for i in ids],mat)
 def rounded_box(self,name,center,size,radius,mat,segments=3):
  self.tag=name;cx,cy,cz=center;sx,sy,sz=size;radius=min(radius,.45*sx,.45*sy);outline=[]
  for x,y,start in [(sx/2-radius,sy/2-radius,0),(-sx/2+radius,sy/2-radius,90),(-sx/2+radius,-sy/2+radius,180),(sx/2-radius,-sy/2+radius,270)]:
   for i in range(segments+1):
    a=math.radians(start+i*90/segments);outline.append((cx+x+radius*math.cos(a),cy+y+radius*math.sin(a)))
  low=[(x,y,cz-sz/2) for x,y in outline];high=[(x,y,cz+sz/2) for x,y in outline]
  self.face(low[::-1],mat);self.face(high,mat)
  for i in range(len(low)):
   j=(i+1)%len(low);self.face([low[i],low[j],high[j],high[i]],mat,smooth=False)
 def tube(self,name,a,b,r,mat,n=12,caps=True):
  self.tag=name;axis=unit(sub(b,a));right=unit(cross(axis,(0,0,1) if abs(axis[2])<.9 else (0,1,0)));up=cross(axis,right)
  rings=[]
  for c in [a,b]:rings.append([add(c,add(mul(right,r*math.cos(2*math.pi*i/n)),mul(up,r*math.sin(2*math.pi*i/n)))) for i in range(n)])
  for i in range(n):
   j=(i+1)%n;self.face([rings[0][i],rings[0][j],rings[1][j],rings[1][i]],mat,smooth=True)
  if caps:self.face(rings[0][::-1],mat);self.face(rings[1],mat)
 def lathe(self,name,origin,profile,mat,n=24,axis=(0,0,1),caps=True):
  self.tag=name;axis=unit(axis);right=unit(cross(axis,(0,0,1) if abs(axis[2])<.9 else (0,1,0)));up=cross(axis,right)
  rings=[[add(add(origin,mul(axis,z)),add(mul(right,r*math.cos(2*math.pi*i/n)),mul(up,r*math.sin(2*math.pi*i/n)))) for i in range(n)] for z,r in profile]
  for k in range(len(rings)-1):
   for i in range(n):j=(i+1)%n;self.face([rings[k][i],rings[k][j],rings[k+1][j],rings[k+1][i]],mat,smooth=True)
  if caps:self.face(rings[0][::-1],mat);self.face(rings[-1],mat)
 def torus(self,name,center,r,wire,mat,n=20,k=6,axis=(0,0,1)):
  self.tag=name;axis=unit(axis);right=unit(cross(axis,(0,0,1) if abs(axis[2])<.9 else (0,1,0)));up=cross(axis,right);rings=[]
  for i in range(n):
   a=2*math.pi*i/n;out=add(mul(right,math.cos(a)),mul(up,math.sin(a)));c=add(center,mul(out,r));rings.append([add(c,add(mul(out,wire*math.cos(2*math.pi*j/k)),mul(axis,wire*math.sin(2*math.pi*j/k)))) for j in range(k)])
  for i in range(n):
   for j in range(k):self.face([rings[i][j],rings[(i+1)%n][j],rings[(i+1)%n][(j+1)%k],rings[i][(j+1)%k]],mat,smooth=True)
 def panel(self,name,center,width,height,cell,normal=(0,-1,0)):
  self.tag=name;normal=unit(normal);right=unit(cross((0,0,1),normal));up=cross(normal,right);ps=[add(center,add(mul(right,x*width/2),mul(up,y*height/2))) for x,y in [(-1,-1),(1,-1),(1,1),(-1,1)]]
  col=cell%2;row=cell//2;u0=(col+.025)/2;u1=(col+.975)/2;v0=1-(row+.975)/4;v1=1-(row+.025)/4
  self.face(ps,'PrintAtlas',[(u0,v0),(u1,v0),(u1,v1),(u0,v1)])
