"""Original editable Caliche prototype geometry. Pure Python; no DCC or dependencies.
Plant names describe visual families, not a verified species identification.
"""
import math,random,hashlib,json
from dataclasses import dataclass,field
PI=math.pi

def add(a,b):return tuple(x+y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def mul(a,s):return tuple(x*s for x in a)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def norm(a):return math.sqrt(dot(a,a))
def unit(a):return mul(a,1/max(norm(a),1e-20))
def mix(a,b,t):return add(mul(a,1-t),mul(b,t))
def frame(axis):
 axis=unit(axis);u=unit(cross(axis,(0,0,1) if abs(axis[2])<.95 else (0,1,0)));return u,unit(cross(axis,u))

@dataclass
class Mesh:
 name:str
 vertices:list=field(default_factory=list)
 faces:list=field(default_factory=list)
 uv:list=field(default_factory=list)
 material:list=field(default_factory=list)
 components:list=field(default_factory=list)
 def vert(self,p,uv=(0,0)):
  self.vertices.append(tuple(p));self.uv.append(tuple(uv));return len(self.vertices)-1
 def tri(self,a,b,c,material):
  self.faces.append((a,b,c));self.material.append(material)
 def area(self,face):
  a,b,c=(self.vertices[i] for i in face);return norm(cross(sub(b,a),sub(c,a)))*.5
 def bounds(self):return [[min(p[i] for p in self.vertices) for i in range(3)],[max(p[i] for p in self.vertices) for i in range(3)]]
 def stats(self):
  return {'triangles':len(self.faces),'vertices':len(self.vertices),'bounds_local_m':self.bounds(),'minimum_triangle_area_m2':min(map(self.area,self.faces)),'sha256_geometry':hashlib.sha256(json.dumps([self.vertices,self.faces,self.uv,self.material],separators=(',',':')).encode()).hexdigest(),'materials':sorted(set(self.material)),'component_count':len(self.components)}
 def start(self,name):self.components.append({'name':name,'vertex_start':len(self.vertices),'face_start':len(self.faces)})

def tile_uv(tile,u,v):
 # Shared opaque 4x2 botanical atlas; inset avoids bleeding from neighbouring families.
 return ((tile%4+ .035+.93*u)/4,1-(tile//4+.035+.93*v)/2)

def tube(m,points,radius,sides=5,material='M_CN_Bark'):
 m.start('woody_branch');rings=[]
 for i,p in enumerate(points):
  axis=sub(points[min(i+1,len(points)-1)],points[max(i-1,0)]);u,v=frame(axis)
  rad=radius*(1-.75*i/(len(points)-1));rings.append([m.vert(add(p,add(mul(u,rad*math.cos(j*2*PI/sides)),mul(v,rad*math.sin(j*2*PI/sides)))),(j/sides,i/(len(points)-1))) for j in range(sides)])
 for a,b in zip(rings,rings[1:]):
  for j in range(sides):k=(j+1)%sides;m.tri(a[j],b[k],b[j],material);m.tri(a[j],a[k],b[k],material)
 for ring,point,flip in [(rings[0],points[0],True),(rings[-1],points[-1],False)]:
  center=m.vert(point,(.5,.5))
  for j in range(sides):
   a,b=ring[j],ring[(j+1)%sides];m.tri(center,b,a,material) if flip else m.tri(center,a,b,material)

def leaf(m,base,axis,length,width,tile=0,roll=0,material='M_CN_Foliage'):
 m.start('folded_leaf');axis=unit(axis);u,v=frame(axis);side=add(mul(u,math.cos(roll)),mul(v,math.sin(roll)))
 points=[(0,0),(-.48,.36),(-.34,.81),(0,1),(.34,.81),(.48,.36)]
 ids=[m.vert(add(base,add(mul(axis,length*t),mul(side,width*s))),tile_uv(tile,s+.5,t)) for s,t in points]
 center=m.vert(add(base,add(mul(axis,length*.51),mul(cross(axis,side),width*.18))),tile_uv(tile,.5,.51))
 for i in range(6):m.tri(center,ids[i],ids[(i+1)%6],material)

def ribbon(m,base,heading,height,spread,width,segments,tile,rng,droop=.15):
 m.start('curved_folded_blade');direction=(math.cos(heading),math.sin(heading),0);side=(-direction[1],direction[0],0);sway=rng.uniform(-.07,.07);rows=[]
 for i in range(segments):
  t=i/segments;p=add(base,add(mul(direction,spread*t**1.15),add(mul(side,sway*math.sin(t*PI)),(0,0,height*(t-droop*t*t*t)))))
  w=width*(1-t)**.65;row=[]
  for s in [-1,0,1]:
   q=add(p,add(mul(side,s*w*.5),(0,0,w*.18 if s==0 else 0)));row.append(m.vert(q,tile_uv(tile,(s+1)/2,t)))
  rows.append(row)
 for a,b in zip(rows,rows[1:]):
  for j in range(2):m.tri(a[j],b[j],b[j+1],'M_CN_Foliage');m.tri(a[j],b[j+1],a[j+1],'M_CN_Foliage')
 p=add(base,add(mul(direction,spread),(0,0,height*(1-droop))));tip=m.vert(p,tile_uv(tile,.5,1))
 for j in range(2):m.tri(rows[-1][j],tip,rows[-1][j+1],'M_CN_Foliage')

def foliar_spray(m,base,axis,length,width,tile,roll,segments,rng):
 """Deeply folded, irregular lobed foliage, not flat crossed billboard cards."""
 m.start('three_dimensional_foliar_spray');axis=unit(axis);u,v=frame(axis)
 side=add(mul(u,math.cos(roll)),mul(v,math.sin(roll)));normal=unit(cross(axis,side));rows=[]
 for i in range(segments):
  t=i/segments
  # Uneven paired lobes along a bent central ridge keep each spray's edge organic.
  envelope=(.18+.85*math.sin(PI*t)**.6)*(1-.30*t)
  if i%2==0 and i:envelope*=.76
  spine=add(base,add(mul(axis,length*t),add(mul(side,length*.07*math.sin(PI*t)),mul(normal,length*.11*math.sin(PI*t)))))
  row=[]
  for sign in [-1,0,1]:
   offset=sign*width*.5*envelope*rng.uniform(.86,1.13) if sign else 0
   fold=width*.32*math.sin(PI*t) if not sign else 0
   row.append(m.vert(add(spine,add(mul(side,offset),mul(normal,fold))),tile_uv(tile,(sign+1)/2,t)))
  rows.append(row)
 for a,b in zip(rows,rows[1:]):
  for j in range(2):m.tri(a[j],b[j],b[j+1],'M_CN_Foliage');m.tri(a[j],b[j+1],a[j+1],'M_CN_Foliage')
 tip=m.vert(add(base,mul(axis,length)),tile_uv(tile,.5,1))
 for j in range(2):m.tri(rows[-1][j],tip,rows[-1][j+1],'M_CN_Foliage')

SPECS=[
 ('Sage_A','sage',1101,.45,1.0),('Sage_B','sage',1127,.62,1.25),('Sage_C','sage',1189,.34,.84),
 ('Grass_A','grass',2203,.43,.62),('Grass_B','grass',2221,.64,.88),
 ('Yucca_A','yucca',3307,.78,1.32),('Yucca_B','yucca',3343,1.05,1.64),
 ('Juniper_A','juniper',4409,1.65,2.35),('Juniper_B','juniper',4451,2.65,3.50),
 ('Caliche_A','stone',5501,.19,.64),('Caliche_B','stone',5521,.37,.88),('Sandstone_A','stone',5557,.52,1.20)]

def sage(m,r,lod,h,w):
 branches=[8,6,4][lod];twigs=[4,3,2][lod];sprays=[12,8,7][lod];segments=[4,3,2][lod]
 for b in range(branches):
  phi=2*PI*(b+.24*r.random())/branches;reach=w*r.uniform(.30,.47)
  end=(math.cos(phi)*reach,math.sin(phi)*reach,h*r.uniform(.30,.64));root=(r.uniform(-.045,.045),r.uniform(-.045,.045),.014)
  elbow=add(mix(root,end,.52),(r.uniform(-.04,.04),r.uniform(-.04,.04),h*.08));tube(m,[root,elbow,end],.006,[5,4,3][lod])
  for k in range(twigs):
   t=.12+.80*(k+.5)/twigs;base=mix(root,end,t);ang=phi+r.uniform(-1.5,1.5)
   tip=add(base,(math.cos(ang)*w*.13,math.sin(ang)*w*.13,h*r.uniform(.16,.33)));tube(m,[base,tip],.0025,[4,3,3][lod])
   for j in range(sprays):
    q=mix(base,tip,(j+.15)/sprays);heading=ang+(j%2*2-1)*r.uniform(.5,1.8)
    axis=(math.cos(heading),math.sin(heading),r.uniform(.15,1.05));size=(1.18 if w>1.1 else 1.0)*(1.12 if lod==2 else 1.0)
    length=[.17,.24,.36][lod]*1.10*size*r.uniform(.80,1.17);width=[.095,.15,.24][lod]*1.35*size*r.uniform(.85,1.12)
    foliar_spray(m,q,axis,length,width,0,r.random()*2*PI,segments,r)

def grass(m,r,lod,h,w):
 blades=[110,46,24][lod];segments=[5,3,2][lod]
 for j in range(blades):
  phi=r.random()*2*PI;rad=math.sqrt(r.random())*.10;base=(math.cos(phi)*rad,math.sin(phi)*rad,.012)
  ribbon(m,base,phi+r.uniform(-.5,.5),h*r.uniform(.58,1.15),w*r.uniform(.16,.50),r.uniform(.004,.009)*(1+lod*.35),segments,1 if j%5 else 5,r,r.uniform(.1,.38))
 if lod==0:
  for j in range(4):
   phi=r.random()*2*PI;tip=(math.cos(phi)*.11,math.sin(phi)*.11,h*1.08);tube(m,[(0,0,.03),tip],.0012,3,'M_CN_Foliage')
   for k in range(3):leaf(m,add(tip,(0,0,-k*.017)),(math.cos(phi),math.sin(phi),.4),.024,.009,5)

def yucca(m,r,lod,h,w):
 count=[39,23,12][lod];segments=[7,4,2][lod]
 tube(m,[(0,0,.005),(.025,-.008,.15)],.085,[8,6,4][lod])
 for j in range(count):
  phi=j*2.399963+r.uniform(-.12,.12);ring=(j%3)/2;dry=j%7==0
  height=h*(.48+.55*(1-ring))*r.uniform(.88,1.10);spread=w*(.15+.33*ring)*r.uniform(.85,1.08)
  ribbon(m,(r.uniform(-.02,.02),r.uniform(-.02,.02),.07),phi,height,spread,r.uniform(.025,.047),segments,5 if dry else 2,r,.64 if dry else r.uniform(.02,.14))

def juniper(m,r,lod,h,w):
 arms=[10,8,6][lod];sprigs=[6,4,3][lod];sprays=[12,8,7][lod];segments=[4,3,2][lod]
 tube(m,[(0,0,.005),(.04,-.03,h*.24),(.02,.02,h*.61)],.075,[8,6,4][lod])
 for b in range(arms):
  phi=b*2.399963+r.uniform(-.35,.35);z=h*(.12+.69*b/max(1,arms-1))
  reach=w*.44*math.sqrt(max(.20,1-((z/h-.48)/.57)**2))*r.uniform(.83,1.07)
  base=(.025,-.02,z*.36);end=(math.cos(phi)*reach,math.sin(phi)*reach,z);elbow=add(mix(base,end,.54),(0,0,-h*.025));tube(m,[base,elbow,end],.020,[5,4,3][lod])
  for j in range(sprigs):
   p=mix(base,end,.18+.75*(j+.25)/sprigs);angle=phi+r.uniform(-1.7,1.7);length=w*r.uniform(.08,.17)
   tip=add(p,(math.cos(angle)*length,math.sin(angle)*length,h*r.uniform(.05,.16)));tube(m,[p,tip],.0045,[4,3,3][lod])
   for k in range(sprays):
    q=mix(p,tip,(k+.15)/sprays);rot=k*2.399963+r.uniform(-.35,.35)
    axis=(math.cos(rot),math.sin(rot),r.uniform(.1,1.0))
    density=1.25 if w>3 else 1.0
    length=[.32,.46,.68][lod]*(1.12 if w>3 else 1.0)*density*r.uniform(.82,1.18);width=[.19,.30,.48][lod]*1.55*(1.08 if w>3 else 1.0)*density*r.uniform(.85,1.15)
    foliar_spray(m,q,axis,length,width,3 if k%5 else 4,r.random()*2*PI,segments,r)

def stone(m,r,lod,h,w,sandstone):
 # Unequal rings keep layered angular fracture faces, not rounded sphere polygons.
 n=[11,8,5][lod];rings=[]
 for layer,z in enumerate([0,.16,.67,1]):
  ring=[]
  for j in range(n):
   a=2*PI*j/n;rad=w*.5*([.68,1,.89,.50][layer])*r.uniform(.8,1.12);p=(math.cos(a)*rad,math.sin(a)*rad*.72,h*(z+r.uniform(-.07,.07)) if layer else 0)
   ring.append(m.vert(p,tile_uv(7 if sandstone else 6,j/n,z)))
  rings.append(ring)
 for a,b in zip(rings,rings[1:]):
  for j in range(n):k=(j+1)%n;m.tri(a[j],b[k],b[j],'M_CN_Stone');m.tri(a[j],a[k],b[k],'M_CN_Stone')
 for ring,z,flip in [(rings[0],0,True),(rings[-1],h*.98,False)]:
  c=m.vert((0,0,z),tile_uv(7 if sandstone else 6,.5,.5))
  for j in range(n):
   a,b=ring[j],ring[(j+1)%n];m.tri(c,b,a,'M_CN_Stone') if flip else m.tri(c,a,b,'M_CN_Stone')

def build(spec,lod=0):
 name,family,seed,h,w=spec;r=random.Random(seed+lod*10000);m=Mesh('SM_CN_'+name+(f'_LOD{lod}' if lod else ''))
 {'sage':sage,'grass':grass,'yucca':yucca,'juniper':juniper}.get(family,lambda m,r,lod,h,w:stone(m,r,lod,h,w,name.startswith('Sandstone')))(m,r,lod,h,w)
 # Scale the authored envelope to the design height; the root remains at ground Z=0.
 lo,hi=m.bounds();scale_z=h/(hi[2]-lo[2]);scale_xy=w/max(hi[0]-lo[0],hi[1]-lo[1])
 m.vertices=[(x*scale_xy,y*scale_xy,(z-lo[2])*scale_z) for x,y,z in m.vertices]
 return m

if __name__=='__main__':
 for spec in SPECS:
  print(spec[0],[(lod,build(spec,lod).stats()['triangles'],round(build(spec,lod).stats()['minimum_triangle_area_m2'],10)) for lod in range(3)])
