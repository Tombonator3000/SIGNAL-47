"""Original low-poly fittings using the project's original metric mesh helper."""
import importlib.util,math,random,json
from pathlib import Path
from design import PARTS
_spec=importlib.util.spec_from_file_location('substation_detail_meshkit',Path(__file__).resolve().parent.parent/'substation/meshkit.py')
_k=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_k)
class Mesh(_k.Mesh):
 def face(self,ps,mat,uv=None,smooth=False):
  super().face(ps,mat,uv,smooth);self.mat[-1]=self.mat[-1].replace('M_Substation_','M_SubstationDetail_',1)

def terrain_height(x,y):
 data=json.loads((Path(__file__).resolve().parent/'terrain_contact.json').read_text());hits=[]
 for t in data['triangles']:
  a,b,c=t['site_vertices_m'];den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
  if abs(den)<1e-10:continue
  u=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1]))/den;v=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1]))/den;w=1-u-v
  if min(u,v,w)>=-1e-7:hits.append(u*a[2]+v*b[2]+w*c[2])
 if not hits:raise ValueError(('No measured terrain triangle',x,y))
 return max(hits)

def shoulder_sections(lod=0):
 # Ordered outer-facing strips, with a broad safe omission at the gameplay approach.
 strips=[((-9.90,-7.8),(-9.90,7.90),(-1,0)),((-9.90,7.90),(9.90,7.90),(0,1)),((9.90,7.90),(9.90,-7.10),(1,0)),((-9.90,-7.90),(-2.8,-7.90),(0,-1))]
 out=[]
 for si,(a,b,d) in enumerate(strips):
  steps=math.ceil(math.dist(a,b)/[.55,1.1,1.8][lod]);rows=[]
  for i in range(steps+1):
   t=i/steps;x=a[0]+(b[0]-a[0])*t;y=a[1]+(b[1]-a[1])*t
   width=.78+.13*math.sin(t*17+si)+.07*math.sin(t*47+si*1.7)
   ox=x+d[0]*width;oy=y+d[1]*width;oz=terrain_height(ox,oy)+.006
   rows.append(((x,y,-.075),(ox,oy,oz)))
  out.append(rows)
 return out

def build(lod=0):
 n=[24,12,8][lod];meshes={k:Mesh(k) for k in PARTS}
 def m(short):return meshes['SM_SubstationDetail_'+short]
 def pipe(mesh,tag,points,r,mat='GalvanizedSteel',segments=None):
  for i,(a,b) in enumerate(zip(points,points[1:])):mesh.tube(tag+'_'+str(i),a,b,r,mat,segments or [12,8,6][lod])
 def bolt(mesh,tag,x,y,z,axis='Z'):
  a=(x,y,z);b=(x,y,z+.018) if axis=='Z' else (x,y-.018,z)
  mesh.tube(tag,a,b,.020,'GalvanizedSteel',6)
 def rock(mesh,tag,x,y,z,r,h,seed):
  rng=random.Random(seed);sides=[7,6,5][lod];rings=[]
  for zz,scale in [(z-.012,.86),(z+h*.32,1),(z+h,.46)]:
   rings.append([(x+math.cos(2*math.pi*j/sides)*r*scale*rng.uniform(.8,1.08),y+math.sin(2*math.pi*j/sides)*r*scale*rng.uniform(.8,1.08),zz+rng.uniform(-.004,.004)) for j in range(sides)])
  mesh.tag=tag
  for a,b in zip(rings,rings[1:]):
   for j in range(sides):k=(j+1)%sides;mesh.face([a[j],a[k],b[k],b[j]],'Gravel')
  mesh.face(rings[-1],'Gravel');mesh.face(rings[0][::-1],'Gravel')
 mesh=m('InspectionFittings');x,y,z=4.62,-8.925,1.70
 # Annular torus has no end cap across the retained amber lens.
 mesh.torus('Open_lamp_bezel',(x,-8.952,z),.077,.008,'GalvanizedSteel',n,6 if lod==0 else 4,axis=(0,-1,0))
 # Small sheet-metal brow is entirely above the optical aperture. Front/down rays remain clear.
 mesh.box('Rain_brow',(x,-8.960,1.799),(.215,.150,.012),'OlivePaint')
 mesh.box('Left_rain_cheek',(x-.106,-8.960,1.766),(.012,.150,.055),'OlivePaint')
 mesh.box('Right_rain_cheek',(x+.106,-8.960,1.766),(.012,.150,.055),'OlivePaint')
 for xx in [x-.125,x+.125]:
  mesh.box('Lamp_mount_ear',(xx,-8.943,1.757),(.035,.014,.080),'DarkSteel')
  bolt(mesh,'Mount_ear_screw',xx,-8.952,1.773,'Y')
 # Deliberately below/away from the printed panel and existing hinge at x4.43.
 mesh=m('ServiceConduit')
 mesh.rounded_box('Side_junction_box',(4.342,-8.34,1.52),(.075,.22,.28),.02,'OlivePaint',[3,2,1][lod])
 mesh.box('Junction_lid',(4.296,-8.34,1.52),(.018,.235,.295),'GalvanizedSteel')
 pipe(mesh,'Cabinet_side_conduit',[(4.32,-8.34,1.38),(4.32,-8.34,.30),(4.32,-8.12,.12),(4.76,-8.12,.12),(5.5,-8.12,.12),(5.5,-7.4,.10)],.021)
 for zz in [.48,.95,1.30]:
  mesh.box('Conduit_strap',(4.32,-8.34,zz),(.070,.055,.025),'GalvanizedSteel')
  mesh.box('Strap_foot',(4.36,-8.34,zz),(.035,.090,.045),'DarkSteel')
 for yy in [-8.405,-8.275]:
  for zz in [1.43,1.61]:mesh.tube('Side_lid_screw',(4.286,yy,zz),(4.276,yy,zz),.013,'DarkSteel',6)
 for zz in [1.36,.31]:mesh.tube('Conduit_coupling',(4.32,-8.34,zz-.025),(4.32,-8.34,zz+.025),.030,'DarkSteel',n//2)
 mesh=m('FoundationContact')
 for xx in [4.48,5.52]:
  for yy in [-8.75,-8.25]:
   mesh.box('Skid_baseplate',(xx,yy,.012),(.25,.19,.024),'GalvanizedSteel')
   for dx in [-.083,.083]:bolt(mesh,'Foundation_anchor',xx+dx,yy,.025)
 # A narrow folded rain lip remains on the plinth, away from the walking ramp.
 mesh.box('Plinth_rear_flashing',(5,-7.79,.014),(1.62,.075,.026),'GalvanizedSteel')
 rng=random.Random(47028)
 for i in range([28,16,8][lod]):
  # Small deposits at the apron edges, not a new curb or step.
  x=3.22+rng.uniform(.025,.12) if i%2 else 6.78-rng.uniform(.025,.12);y=rng.uniform(-13.55,-9.42)
  z=-.25+(y+14)/4.75*.25
  rock(mesh,'Apron_edge_grit',x,y,z+.004,rng.uniform(.022,.05),rng.uniform(.009,.026),3100+i)
 mesh=m('GravelContact');rng=random.Random(861947)
 # Nonuniform small clusters wholly inside the existing bed. No borrowed terrain height.
 clusters=[(-9.58,-6.1),(-9.58,-2.8),(-9.58,2.1),(-9.42,6.8),(-5.7,7.55),(-2.2,7.56),(2.0,7.56),(6.2,7.5),(9.5,5.8),(9.5,2.4),(9.5,-1.4),(9.50,-5.3),(-7.5,-7.57),(-4.0,-7.57),(8.2,-7.52)]
 for ci,(cx,cy) in enumerate(clusters):
  for j in range([7,4,2][lod]):
   x=max(-9.76,min(9.76,cx+rng.uniform(-.32,.32)));y=max(-7.76,min(7.76,cy+rng.uniform(-.30,.30)))
   r=rng.uniform(.055,.16);h=rng.uniform(.025,.085)
   rock(mesh,'Caliche_edge_cluster_%02d'%ci,x,y,-.085,r,h,ci*30+j)
 mesh=m('GroundShoulder')
 for si,rows in enumerate(shoulder_sections(lod)):
  mesh.tag='Measured_feathered_shoulder_'+str(si)
  for a,b in zip(rows,rows[1:]):
   # Reverse normals when needed; the shoulder is a thin visible grading surface.
   ps=[a[0],a[1],b[1],b[0]]
   nx=_k.cross(_k.sub(ps[1],ps[0]),_k.sub(ps[2],ps[0]))
   if nx[2]<0:ps.reverse()
   mesh.face(ps,'Gravel')
 return meshes
