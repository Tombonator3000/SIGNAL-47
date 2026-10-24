"""Original reusable industrial-site models. Placement awaits audited anchors.
No Blender, rendering, textures or native runtime is launched by this module.
"""
import math
from shapes import Mesh,add,sub,mul

def pole(lod=0,name='SM_SubstationSite_Pole'):
 m=Mesh(name);n=[20,12,8][lod]
 # Slight irregular taper belongs to the silhouette; weathering/knots are UV PBR.
 m.lathe('Weathered_treated_timber_pole',(0,0,0),[(-.35,.195),(0,.185),(1.2,.179),(4.0,.155),(7.2,.132),(8.52,.118)],'Wood',n)
 m.box('Timber_crossarm',(0,0,8.33),(3.18,.19,.22),'Wood')
 # Timber grain follows armX; endcaps need independentY/Z coordinates.
 for j in range(len(m.f)-6,len(m.f)):
  ps=[m.v[k] for k in m.f[j]]
  from shapes import cross
  normal=cross(sub(ps[1],ps[0]),sub(ps[2],ps[0]))
  if abs(normal[0])>max(abs(normal[1]),abs(normal[2])):m.uv[j]=[(p[1]*2,p[2]) for p in ps]
  elif abs(normal[2])>abs(normal[1]):m.uv[j]=[(p[1]*2,p[0]/2) for p in ps]
  else:m.uv[j]=[(p[2],p[0]/2) for p in ps]
 for x in [-1.05,1.05]:
  m.tube('Galvanized_arm_brace',(x,0,8.24),(0,-.015,7.29),.026,'GalvanizedSteel',max(6,n//2))
 for z in [7.26,8.25]:
  m.torus('Pole_steel_band',(0,0,z),.144 if z<8 else .125,.014,'GalvanizedSteel',n,6)
  m.box('Through_bolt_plate',(0,-.157,z),(.15,.035,.22),'GalvanizedSteel')
  m.tube('Through_bolt',(0,-.19,z),(0,.19,z),.020,'GalvanizedSteel',6)
 for phase,x in enumerate([-1.04,0,1.04]):
  m.tube('Insulator_pin_'+str(phase),(x,0,8.41),(x,0,8.60),.040,'GalvanizedSteel',max(8,n//2))
  profile=[(0,.058),(.06,.065)]
  for i in range(5):
   a=.075+i*.105;profile.extend([(a,.065),(a+.022,.135),(a+.042,.128),(a+.079,.064)])
  profile.extend([(.62,.062),(.66,.052)])
  m.lathe('Brown_porcelain_phase_'+str(phase),(x,0,8.53),profile,'PorcelainBrown',[20,12,8][lod])
  m.tube('Line_clamp_'+str(phase),(x,-.10,9.20),(x,.10,9.20),.029,'DarkSteel',8)
 for z in [1.1,1.9]:m.box('Pole_id_plate_back',(0,-.183,z),(.16,.025,.14),'GalvanizedSteel')
 m.panel('Pole_id_1986',(0,-.199,1.9),.17,.085,0)
 # Small grounded downlead follows the timber; no long guy wire is placed before audit.
 m.tube('Downlead_copper',(0.13,-.08,.05),(.13,-.08,2.85),.009,'DarkSteel',6)
 return m

def sheet(m,name,origin,width,height,normal,lod=0):
 """Physical folded galvanized siding. The profile has no alpha cards."""
 pitch=[.105,.15,.24][lod];steps=max(4,math.ceil(width/pitch)*4);ox,oy,oz=origin;right=(1,0,0) if abs(normal[1])>.5 else (0,1,0)
 for i in range(steps):
  u0=width*i/steps;u1=width*(i+1)/steps
  # Squared, subtly rounded wave makes broad sheet pans and narrow folds.
  def depth(u):return .019*math.cos(u*2*math.pi/pitch)
  a=add(origin,add(mul(right,u0),mul(normal,depth(u0))));b=add(origin,add(mul(right,u1),mul(normal,depth(u1))))
  ps=[a,b,add(b,(0,0,height)),add(a,(0,0,height))]
  # Face all panels outward, whichever edge/right direction the wall uses.
  from shapes import cross,dot
  if dot(cross(sub(ps[1],ps[0]),sub(ps[2],ps[0])),normal)<0:ps.reverse()
  m.tag=name;m.face(ps,'CorrugatedSteel',smooth=False)

def shed(lod=0,name='SM_SubstationSite_Shed'):
 m=Mesh(name);n=[16,10,8][lod]
 # Foundation, timber corners and thin metal skins remain separate topology.
 m.box('Small_concrete_sill',(0,0,.045),(3.12,2.52,.09),'Concrete')
 for x in [-1.43,1.43]:
  for y in [-1.13,1.13]:m.box('Shed_timber_corner',(x,y,1.35),(.105,.105,2.61),'Wood')
 sheet(m,'Back_corrugated_wall',(-1.5,1.2,.09),3,2.60,(0,1,0),lod)
 sheet(m,'Left_corrugated_wall',(-1.5,-1.2,.09),2.4,2.26,(-1,0,0),lod)
 sheet(m,'Right_corrugated_wall',(1.5,-1.2,.09),2.4,2.26,(1,0,0),lod)
 # Sloped side infill is opaque actual geometry under the mono-pitch roof.
 for x,normal in [(-1.5,(-1,0,0)),(1.5,(1,0,0))]:
  ps=[(x,-1.2,2.35),(x,1.2,2.35),(x,1.2,2.70)];m.tag='Sloped_siding_infill'
  if x<0:ps.reverse()
  m.face(ps,'CorrugatedSteel')
 # The closed door fills a real front opening; no sheet crosses its aperture.
 sheet(m,'Front_left_wall',(-1.5,-1.2,.09),.62,2.26,(0,-1,0),lod)
 sheet(m,'Front_right_wall',(.04,-1.2,.09),1.46,2.26,(0,-1,0),lod)
 sheet(m,'Front_door_lintel',(-.88,-1.2,2.05),.92,.30,(0,-1,0),lod)
 m.rounded_box('Locked_metal_door',(-.42,-1.237,1.065),(.87,.06,1.96),.02,'RoofRust',[3,2,1][lod])
 for x in [-.9,.06]:m.box('Door_frame',(x,-1.245,1.10),(.055,.07,2.06),'GalvanizedSteel')
 m.box('Door_frame_header',(-.42,-1.245,2.13),(1.02,.07,.06),'GalvanizedSteel')
 for z in [.40,1.05,1.79]:m.tube('Door_hinge',(-.865,-1.287,z-.07),(-.865,-1.287,z+.07),.019,'GalvanizedSteel',8)
 m.box('Padlock_hasp',(-.035,-1.292,1.02),(.11,.025,.05),'GalvanizedSteel');m.box('Weathered_padlock',(-.035,-1.322,.965),(.075,.04,.095),'DarkSteel')
 m.torus('Padlock_shackle',(-.035,-1.322,1.028),.035,.007,'GalvanizedSteel',n,6,axis=(0,1,0))
 m.panel('Maintenance_identity',(-.42,-1.271,1.65),.58,.29,1)
 m.box('Vent_dark_opening',(.88,-1.233,1.90),(.50,.035,.33),'DarkSteel')
 for z in [1.76,2.04]:m.box('Vent_frame',(.88,-1.265,z),(.54,.035,.028),'GalvanizedSteel')
 for k in range(6):m.box('Vent_louvre',(.88,-1.279,1.79+k*.043),(.46,.045,.012),'GalvanizedSteel')
 # Roof corrugation follows drainage directionY; true sheet folds overhang walls.
 width=3.18;pitch=[.105,.15,.24][lod];steps=math.ceil(width/pitch)*4
 for i in range(steps):
  x0=-width/2+width*i/steps;x1=-width/2+width*(i+1)/steps
  def height(x,y):return 2.35+(y+1.2)*(.35/2.4)+.025*math.cos(x*2*math.pi/pitch)
  m.tag='Rusty_corrugated_roof';m.face([(x0,-1.35,height(x0,-1.35)),(x1,-1.35,height(x1,-1.35)),(x1,1.35,height(x1,1.35)),(x0,1.35,height(x0,1.35))],'RoofRust')
 for y in [-1.37,1.37]:m.box('Folded_roof_edge',(0,y,2.35+(y+1.2)*(.35/2.4)),(3.18,.045,.045),'RoofRust')
 m.tube('Front_gutter',(-1.59,-1.35,2.29),(1.59,-1.35,2.29),.044,'GalvanizedSteel',n)
 m.tube('Gutter_downpipe',(-1.56,-1.35,.16),(-1.56,-1.35,2.29),.031,'GalvanizedSteel',n)
 if lod<2:
  for x in [-1.35,-.65,.15,.85,1.45]:
   for y in [-1.1,1.12]:
    z=2.35+(y+1.2)*(.35/2.4)+.03;m.tube('Roof_fastener',(x,y,z),(x,y,z+.012),.013,'GalvanizedSteel',6)
 m.v=[(x,y,z-.09) for x,y,z in m.v]
 return m

def props(lod=0,name='SM_SubstationSite_Equipment'):
 m=Mesh(name);n=[20,12,8][lod]
 for x,y,r,width in [(0,0,.43,.57),(.94,.20,.30,.43)]:
  z=r
  for xx in [x-width/2,x+width/2-.055]:m.lathe('Cable_drum_wood_flange',(xx,y,z),[(0,r*.97),(.015,r),(.045,r),(.055,r*.97)],'Wood',n,axis=(1,0,0))
  m.tube('Cable_drum_core',(x-width/2+.055,y,z),(x+width/2-.055,y,z),r*.44,'Wood',n)
  if r>.4:
   for k in range([7,5,3][lod]):m.torus('Wound_black_cable',(x-width*.36+k*width*.72/([7,5,3][lod]-1),y,z),r*.77,.031,'CableBlack',n,6,axis=(1,0,0))
  for a in [0,math.pi/2,math.pi,3*math.pi/2]:
   yy=y+r*.67*math.cos(a);zz=z+r*.67*math.sin(a);m.tube('Drum_tie_bolt',(x-width/2-.004,yy,zz),(x+width/2+.004,yy,zz),.012,'GalvanizedSteel',6)
 m.rounded_box('Old_steel_toolbox',(-1.02,.10,.23),(.70,.44,.44),.035,'RoofRust',[3,2,1][lod])
 m.box('Toolbox_lid_seam',(-1.02,.10,.40),(.73,.45,.023),'DarkSteel');m.tube('Toolbox_carry_handle',(-1.20,.1,.49),(-.84,.1,.49),.019,'GalvanizedSteel',8)
 for xx in [-1.25,-.79]:m.box('Toolbox_latch',(xx,-.13,.37),(.05,.024,.085),'GalvanizedSteel')
 return m

def lines(lod=0,name='SM_SubstationSite_Lines'):
 from design import POLES,SPANS,PHASE_X,PHASE_HEIGHT,BUS_ENDPOINTS,GUY_ANCHOR
 m=Mesh(name);segments=[16,8,4][lod];radial=[6,4,3][lod]
 for si,span in enumerate(SPANS):
  for phase,x in enumerate(PHASE_X):
   a=BUS_ENDPOINTS[phase] if span['from']=='bus' else add(POLES[span['from']],(x,0,PHASE_HEIGHT))
   b=add(POLES[span['to']],(x,0,PHASE_HEIGHT));points=[]
   for j in range(segments+1):
    t=j/segments;p=add(mul(a,1-t),mul(b,t));points.append((p[0],p[1],p[2]-4*span['sag_m']*t*(1-t)))
   for j,(u,v) in enumerate(zip(points,points[1:])):m.tube('Sagged_phase_%s_%s_segment%s'%(si,phase,j),u,v,.014,'DarkSteel',radial,caps=False)
 # Last pole is physically dead-ended and guyed to the audited ground anchor.
 top=add(POLES['C'],(0,0,7.75));bottom=add(GUY_ANCHOR,(0,0,.10));delta=sub(top,bottom)
 m.tube('Terminal_guy_steel',bottom,top,.011,'GalvanizedSteel',radial,caps=False)
 length=math.dist(top,bottom);guardtop=add(bottom,mul(delta,1.7/length));m.tube('Ochre_guy_guard',bottom,guardtop,.031,'SafetyOchre',max(6,radial))
 m.tube('Guy_buried_rod',add(GUY_ANCHOR,(0,0,-.32)),bottom,.034,'DarkSteel',8)
 m.torus('Guy_anchor_eye',bottom,.075,.014,'GalvanizedSteel',12,6,axis=(1,0,0))
 for x in PHASE_X:
  p=add(POLES['C'],(x,0,PHASE_HEIGHT));m.box('Deadend_bolted_clamp',p,(.10,.18,.075),'GalvanizedSteel');m.tube('Deadend_tension_bolt',add(p,(-.075,0,0)),add(p,(.075,0,0)),.018,'GalvanizedSteel',6)
 return m

def ground(lod=0,name='SM_SubstationSite_Ground'):
 import random
 from design import POLES,GUY_ANCHOR
 m=Mesh(name);rng=random.Random(198647)
 def stone(center,r):
  m.lathe('Loose_caliche_contact_stone',center,[(-.018,r*.85),(.012,r),(.05,r*.48),(.065,r*.20)],'Gravel',[6,5,4][lod])
 # Keep every perimeter island inside the known flat source gravel20x16m.
 for x,y in [(-9.65,-4),(-9.65,3),(9.65,-3),(9.65,3),(-6,7.65),(4,7.65),(-5,-7.65)]:
  for k in range([13,7,3][lod]):stone((x+rng.uniform(-.15,.15),y+rng.uniform(-.15,.15),-.08),rng.uniform(.025,.075))
 for p in list(POLES.values())+[GUY_ANCHOR]:
  for k in range([17,9,4][lod]):
   a=rng.uniform(0,2*math.pi);r=rng.uniform(.22,.40);stone((p[0]+r*math.cos(a),p[1]+r*math.sin(a),p[2]),rng.uniform(.025,.065))
 return m

def build(lod=0):
 from design import POLES,PARTS
 result={}
 for key in POLES:
  name='SM_SubstationSite_Pole_'+key;result[name]=pole(lod,name)
 result['SM_SubstationSite_Shed']=shed(lod)
 result['SM_SubstationSite_Equipment']=props(lod)
 result['SM_SubstationSite_Lines']=lines(lod)
 result['SM_SubstationSite_Ground']=ground(lod)
 assert set(result)==set(PARTS)
 return result
