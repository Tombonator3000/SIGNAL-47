"""Original compact1986 electrical yard, complete editable geometry recipes.
Dimensions author an evocative fictional prop, not installation instructions.
"""
import math,random
from meshkit import Mesh,add,sub,mul
from design import PARTS,ATLAS_CELLS

def build(lod=0):
 n=[24,12,8][lod];wire_n=[6,4,3][lod];detail=[3,2,1][lod];meshes={k:Mesh(k) for k in PARTS}
 def get(short):return meshes['SM_Substation_'+short]
 def pipe(m,name,points,r=.035,mat='GalvanizedSteel',segments=None):
  for i,(a,b) in enumerate(zip(points,points[1:])):m.tube(name+str(i),a,b,r,mat,segments or n)
 def bushing(m,name,x,y,z,height=2.15,radius=.235,sheds=13):
  m.tube(name+'_mount',(x,y,z-.12),(x,y,z+.07),radius*1.05,'GalvanizedSteel',n)
  core=radius*.43;profile=[(0,core*.92),(.075,core)]
  spacing=(height-.22)/sheds
  for i in range(sheds):
   a=.10+i*spacing;profile.extend([(a,core),(a+.025,radius*.82),(a+.05,radius),(a+.075,radius*.70),(a+spacing-.005,core)])
  profile.extend([(height-.11,core),(height,core*.8)])
  m.lathe(name+'_porcelain',(x,y,z),profile,'PorcelainBrown',n)
  m.tube(name+'_terminal',(x,y,z+height),(x,y,z+height+.16),core*.8,'Copper',max(8,n//2))
  m.box(name+'_terminal_clamp',(x,y,z+height+.105),(.28,.13,.07),'Copper')
 def bolts(m,name,center,size,z,count=10):
  for side in [-1,1]:
   for i in range(count):
    t=-.46+.92*i/(count-1);x=center[0]+size[0]*t;y=center[1]+side*size[1]*.48
    m.tube(name,(x,y,z),(x,y,z+.048),.032,'GalvanizedSteel',6)
 def gauge(m,name,center,radius,cell):
  x,y,z=center;m.tube(name+'_rim',(x,y+.05,z),(x,y,z),radius,'GalvanizedSteel',n)
  # Circular instrument plate uses a square atlas cell behind its physical rim.
  m.panel(name+'_print',(x,y-.002,z),radius*1.78,radius*1.30,cell)
  pipe(m,name+'_needle',[(x-.055,y-.008,z+.025),(x+.025,y-.008,z+.002)],.005,'DarkSteel',6)
 m=get('Transformer')
 m.box('Foundation_skid_A',(-1.05,0,.14),(.28,2.05,.28),'GalvanizedSteel');m.box('Foundation_skid_B',(1.05,0,.14),(.28,2.05,.28),'GalvanizedSteel')
 m.rounded_box('Tank_lower_sump',(0,0,.50),(3.30,1.96,.42),.18,'OlivePaint',detail)
 m.rounded_box('Oil_tank',(0,0,1.98),(3.30,1.96,2.65),.18,'OlivePaint',detail)
 m.rounded_box('Bolted_lid_flange',(0,0,3.34),(3.52,2.14,.12),.18,'DarkSteel',detail)
 m.rounded_box('Crowned_top_cover',(0,0,3.43),(3.46,2.08,.12),.16,'OlivePaint',detail)
 bolts(m,'Lid_bolts',(0,0),(3.40,2.05),3.50,12 if lod==0 else 7)
 for x in [-1.30,1.30]:
  m.box('Tank_lifting_lug',(x,.68,3.56),(.12,.25,.23),'OlivePaint');m.torus('Lifting_eye',(x,.68,3.74),.085,.024,'GalvanizedSteel',n,6,axis=(1,0,0))
 # Air gaps between the separate pressed-steel radiator panels remain real.
 fincount=[18,12,8][lod]
 for side in [-1,1]:
  for i in range(fincount):
   y=-.83+1.66*i/(fincount-1);x=side*2.15
   m.rounded_box('Separate_radiator_fin',(x,y,1.90),(.82,.048,2.14),.018,'OlivePaint',detail)
   if lod<2:
    for zz in [1.06,2.73]:m.box('Fin_pressed_ridge',(x+side*.415,y,zz),(.025,.065,.18),'DarkSteel')
  for zz in [.85,2.96]:
   m.tube('Radiator_header',(side*2.02,-1.03,zz),(side*2.02,1.03,zz),.095,'OlivePaint',n)
   for yy in [-.82,.82]:pipe(m,'Radiator_tank_pipe',[(side*1.64,yy,zz),(side*1.85,yy,zz),(side*2.02,yy,zz)],.065,'OlivePaint')
  for yy in [-1.0,1.0]:m.tube('Radiator_brace',(side*1.52,yy,.5),(side*2.52,yy,.70),.032,'GalvanizedSteel',max(8,n//2))
 for i,x in enumerate([-1.04,0,1.04]):
  bushing(m,'HV_phase_'+str(i+1),x,.05,3.57,height=2.24,radius=.23,sheds=13)
  bushing(m,'LV_phase_'+str(i+1),x,-.74,3.55,height=.68,radius=.145,sheds=4)
  pipe(m,'LV_rigid_lead',[(x,-.74,4.38),(x,-1.22,4.5),(x,-1.50,4.13)],.026,'Copper')
 # Horizontal conservator with dished ends, legs, breather and plumbing.
 m.lathe('Oil_conservator',(-1.62,1.44,4.26),[(0,.12),(.045,.30),(.12,.39),(.23,.42),(2.95,.42),(3.07,.39),(3.16,.30),(3.20,.12)],'OlivePaint',n,axis=(1,0,0))
 for x in [-1.08,1.08]:
  pipe(m,'Conservator_saddle',[(x,1.18,3.45),(x,1.18,4.13),(x,1.70,4.13),(x,1.70,3.48)],.045,'GalvanizedSteel')
 pipe(m,'Conservator_oil_pipe',[(.85,1.44,3.9),(.85,1.1,3.8),(.85,.9,3.53)],.055,'OlivePaint')
 pipe(m,'Breather_pipe',[(1.48,1.43,4.25),(1.76,1.43,4.25),(1.76,1.43,2.55)],.028,'GalvanizedSteel')
 m.lathe('Silica_breather',(1.76,1.43,2.12),[(0,.08),(.06,.12),(.36,.12),(.43,.08)],'PorcelainBrown',max(12,n//2))
 m.tube('Conservator_fill_cap',(0,1.44,4.64),(0,1.44,4.80),.085,'GalvanizedSteel',n)
 gauge(m,'Oil_temperature',(1.22,-1.03,2.64),.15,ATLAS_CELLS['temperature'])
 m.box('Tank_marshalling_cabinet',(-1.02,-1.10,1.75),(.72,.28,.95),'OlivePaint')
 m.box('Tank_cabinet_door',(-1.02,-1.25,1.75),(.65,.025,.86),'DarkSteel')
 m.panel('Transformer_nameplate',(.15,-1.005,2.10),.76,.38,ATLAS_CELLS['nameplate'])
 m.panel('Transformer_serial',(.15,-1.007,1.62),.36,.18,ATLAS_CELLS['serial'])
 pipe(m,'Drain_valve_pipe',[(1.35,-.97,.60),(1.35,-1.27,.60),(1.35,-1.43,.78)],.043,'Copper')
 m.torus('Drain_valve_wheel',(1.35,-1.47,.82),.115,.016,'DarkSteel',n,6,axis=(0,1,0))
 for a in [0,2.094,4.189]:m.tube('Valve_wheel_spoke',(1.35,-1.47,.82),(1.35+.11*math.cos(a),-1.47,.82+.11*math.sin(a)),.009,'DarkSteel',6)
 pipe(m,'Earth_copper_strap',[(-1.42,-1.0,1.1),(-1.42,-1.0,.26),(-1.8,-1.18,.08)],.014,'Copper',8)
 for x in [-1.25,1.25]:m.tube('Inspection_cover_handle',(x,-1.03,1.02),(x,-1.03,1.35),.019,'GalvanizedSteel',8)
 # Buswork is independent of the tank and can be placed/culled separately.
 m=get('Buswork')
 for x in [-3.45,3.45]:
  m.box('Bus_frame_foot',(x,3.25,.10),(.75,.9,.2),'GalvanizedSteel')
  m.box('Galvanized_portal_leg',(x,3.25,3.42),(.18,.22,6.65),'GalvanizedSteel')
  pipe(m,'Portal_diagonal',[(x,3.25,5.55),(x+(-.9 if x>0 else .9),3.25,6.75)],.04,'GalvanizedSteel',8)
 m.box('Portal_top_beam',(0,3.25,6.82),(7.28,.25,.24),'GalvanizedSteel')
 m.box('Portal_beam_flange',(0,3.25,6.69),(7.35,.42,.045),'GalvanizedSteel')
 for i,x in enumerate([-1.04,0,1.04]):
  bushing(m,'Bus_support_'+str(i),x,3.25,6.94,height=.75,radius=.16,sheds=5)
  points=[]
  for j in range(7):
   t=j/6;points.append((x,.05+3.20*t,5.97+(7.85-5.97)*t-.25*math.sin(math.pi*t)))
  pipe(m,'Flexible_phase_conductor',points,.025,'Copper',max(8,n//2))
  pipe(m,'North_bus_stub',[(x,3.25,7.85),(x,4.65,7.85)],.031,'Copper',max(8,n//2))
 # Control is outside the locked HV fence; physical labels are original English.
 m=get('Control');x,y=5,-8.5
 for xx in [4.55,5.45]:m.box('Control_skid',(xx,y,.11),(.15,.68,.22),'GalvanizedSteel')
 m.rounded_box('External_service_cabinet',(x,y,1.12),(1.25,.76,1.85),.055,'OlivePaint',detail)
 m.box('Control_door_shadow',(x,y-.388,1.13),(1.13,.018,1.69),'DarkSteel')
 m.rounded_box('Control_door',(x,y-.410,1.13),(1.10,.042,1.66),.035,'OlivePaint',detail)
 m.panel('Control_status_print',(x,y-.435,1.55),.89,.445,ATLAS_CELLS['panel'])
 m.panel('Dated_service_record',(4.83,y-.437,.77),.50,.25,ATLAS_CELLS['service'])
 for xx in [4.43,5.57]:
  for zz in [.48,1.7]:m.tube('Door_hinge',(xx,y-.42,zz-.08),(xx,y-.42,zz+.08),.027,'GalvanizedSteel',8)
 for xx in [4.6,4.8,5.0]:m.tube('Analog_control_bezel',(xx,y-.44,1.18),(xx,y-.48,1.18),.04,'GalvanizedSteel',n)
 for i in range(7):m.box('Cabinet_louvre',(5.35,y-.441,.47+i*.045),(.20,.016,.012),'DarkSteel')
 m.tube('Cabinet_lock',(5.45,y-.45,.95),(5.45,y-.50,.95),.025,'GalvanizedSteel',8)
 m.box('Lever_mount',(5.52,-8.94,1.15),(.13,.065,.20),'GalvanizedSteel')
 m.tube('Lamp_bezel',(4.62,-8.90,1.70),(4.62,-8.925,1.70),.060,'DarkSteel',n)
 pipe(m,'Service_conduit',[(5.5,-8.13,.37),(5.5,-8.13,.04),(5.5,-7.3,.04)],.026,'GalvanizedSteel',8)
 m=get('StatusLamp');m.lathe('Amber_status_lens',(4.62,-8.925,1.70),[(0,.043),(.012,.050),(.030,.048),(.039,.025)],'LampAmber',n,axis=(0,-1,0))
 m=get('Lever');m.tube('Pivoting_disconnect_handle',(5.52,-8.94,1.15),(5.52,-9.10,1.35),.022,'GalvanizedSteel',12);m.lathe('Insulated_handle_grip',(5.52,-9.10,1.35),[(0,.034),(.13,.034),(.15,.026)],'Rubber',12)
 # Fence has actual diamond-wire geometry, no transparent alpha plane.
 m=get('Fence')
 def fence_panel(m,name,a,b,height=2.35):
  length=math.dist(a,b);direction=mul(sub(b,a),1/length)
  for zz in [.14,height]:m.tube(name+'_rail',add(a,(0,0,zz)),add(b,(0,0,zz)),.026,'GalvanizedSteel',max(8,n//2))
  spacing=[.22,.31,.45][lod];bottom=.22;top=height-.09;H=top-bottom
  # Line x±z=c clipped exactly to rectangular panel bounds.
  for slope in [-1,1]:
   c=-H;index=0
   while c<=length+H:
    points=[]
    for u in [0,length]:
     zz=slope*(u-c)
     if 0<=zz<=H:points.append((u,zz))
    for zz in [0,H]:
     u=c+slope*zz
     if 0<=u<=length and not any(abs(u-p[0])+abs(zz-p[1])<1e-7 for p in points):points.append((u,zz))
    if len(points)==2 and math.dist(points[0],points[1])>.035:
     ps=[add(a,add(mul(direction,u),(0,0,bottom+zz))) for u,zz in points];m.tube(name+'_diamond_wire',*ps,.006,'GalvanizedSteel',wire_n,caps=False)
    c+=spacing;index+=1
 def fence_run(a,b):
  count=math.ceil(math.dist(a,b)/2.7)
  for i in range(count+1):
   p=add(a,mul(sub(b,a),i/count));m.tube('Fence_post',add(p,(0,0,-.15)),add(p,(0,0,2.48)),.048,'GalvanizedSteel',n)
  for i in range(count):fence_panel(m,'Fence',add(a,mul(sub(b,a),i/count)),add(a,mul(sub(b,a),(i+1)/count)))
 fence_run((-9.2,-7.2,0),(-2.2,-7.2,0));fence_run((2.2,-7.2,0),(9.2,-7.2,0));fence_run((-9.2,-7.2,0),(-9.2,7.2,0));fence_run((9.2,-7.2,0),(9.2,7.2,0));fence_run((-9.2,7.2,0),(9.2,7.2,0))
 m.box('Station_sign_back',(0,-7.24,3.25),(3.95,.060,1.15),'GalvanizedSteel');m.panel('Station_identity',(0,-7.276,3.25),3.85,1.05,ATLAS_CELLS['station'])
 for xx in [-2.45,2.45]:m.tube('Station_sign_post',(xx,-7.2,-.15),(xx,-7.2,3.95),.055,'GalvanizedSteel',n)
 for xx in [-8.2,8.1]:m.panel('Danger_fence_sign',(xx,-7.245,1.36),.85,.425,ATLAS_CELLS['danger'])
 for side,name in [(-1,'Gate_L'),(1,'Gate_R')]:
  m=get(name);a=(side*2.2,-7.2,0);b=(side*.045,-7.2,0);fence_panel(m,name,a,b)
  for p in [a,b]:m.tube('Gate_upright',add(p,(0,0,.12)),add(p,(0,0,2.35)),.032,'GalvanizedSteel',max(8,n//2))
  m.tube('Gate_diagonal',add(a,(0,0,.15)),add(b,(0,0,2.28)),.023,'GalvanizedSteel',max(8,n//2))
  for zz in [.48,1.82]:m.tube('Gate_hinge',add(a,(0,0,zz-.09)),add(a,(0,0,zz+.09)),.07,'GalvanizedSteel',n)
  if side==-1:m.panel('Gate_warning',(-1.05,-7.245,1.40),1.25,.625,ATLAS_CELLS['danger'])
 # Foundation exact top surfaces are authority for native floor zones.
 m=get('Pad');m.rounded_box('Transformer_concrete_foundation',(0,0,-.18),(8,6,.36),.11,'Concrete',detail)
 m.box('External_control_plinth',(5,-8.5,-.15),(1.8,1.5,.30),'Concrete')
 m.tag='Control_apron_ramp';x0,x1=3.2,6.8;y0,y1=-14,-9.25;z0,z1=-.25,0
 A=(x0,y0,z0);B=(x1,y0,z0);C=(x1,y1,z1);D=(x0,y1,z1)
 m.face([A,B,C,D],'Concrete');m.face([(x0,y0,z0-.25),(x0,y1,-.30),(x1,y1,-.30),(x1,y0,z0-.25)],'Concrete')
 m.face([A,(x0,y0,z0-.25),(x1,y0,z0-.25),B],'Concrete');m.face([D,C,(x1,y1,-.30),(x0,y1,-.30)],'Concrete');m.face([A,D,(x0,y1,-.30),(x0,y0,z0-.25)],'Concrete');m.face([B,(x1,y0,z0-.25),(x1,y1,-.30),C],'Concrete')
 for xx in [-2.7,0,2.7]:m.box('Concrete_expansion_joint',(xx,0,.001),(.012,5.7,.002),'DarkSteel')
 m=get('Gravel');m.box('Gravel_contact_skirt',(0,0,-.365),(20,16,.57),'Gravel')
 # Sparse original aggregate silhouettes outside pads and service paths.
 rng=random.Random(1947)
 for i in range([150,65,20][lod]):
  x=rng.uniform(-9.8,9.8);y=rng.uniform(-7.8,7.8)
  if abs(x)<4.2 and abs(y)<3.2:continue
  rad=rng.uniform(.035,.095);m.lathe('Loose_caliche_aggregate',(x,y,-.085),[(0,rad),(.035,rad*.80),(.065,rad*.28)],'Gravel',6)
 return meshes
