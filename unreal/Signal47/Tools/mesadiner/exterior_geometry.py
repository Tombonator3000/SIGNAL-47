"""Original 1986 Mesa roadside sign, short tractor/reefer and worn parking.

All parts use the canonical diner origin, metres/Zup, FRONT+X. Authored
geometry only; importing this module does not start Blender or write files.
The original game still owns the parked driver's/rig interactions and cars.
"""
import math
import random
from pathlib import Path
import importlib.util
from meshkit import Mesh, add, sub, mul, dot, cross, unit

PARTS = {
 'SM_Mesa_Truck': {'offset':[0,0,0], 'role':'original short conventional tractor and refrigerated semitrailer; stationary'},
 'SM_Mesa_Sign': {'offset':[0,0,0], 'role':'weathered double-sided MESA/DINER neon sign at original primary anchor'},
 'SM_Mesa_Parking': {'offset':[0,0,0], 'role':'worn paved/gravel lot, retained takeaway bench, bay marks and wheel stops'},
}
RIG_CENTER = (9.28,14.15,0)
SIGN_ANCHOR = (2.65,-6.8,0)
BENCH_SEAT = (2.43,-5,.46)
DRIVE_CLEAR = (5,14,-14,4)


def seg(lod, high=24, mid=14, low=10): return [high,mid,low][lod]


def prism(m,name,profile,y0,y1,mat):
 """Closed convex XZ profile, CCW viewed in XZ, extruded along Y."""
 m.tag=name
 near=[(x,y0,z) for x,z in profile];far=[(x,y1,z) for x,z in profile]
 # XZ CCW gives normal-Y, the negative-Y end's outward normal.
 m.face(near,mat);m.face(far[::-1],mat)
 for i in range(len(profile)):
  j=(i+1)%len(profile);m.face([near[i],far[i],far[j],near[j]],mat)


def plate_yz(m,name,outline,x0,x1,mat):
 """Closed sign profile, CCW in YZ, normal+X at its front."""
 m.tag=name
 back=[(x0,y,z) for y,z in outline];front=[(x1,y,z) for y,z in outline]
 m.face(back[::-1],mat);m.face(front,mat)
 for i in range(len(outline)):
  j=(i+1)%len(outline);m.face([back[i],back[j],front[j],front[i]],mat)


def polyline(m,name,points,r,mat,lod=0,n=None):
 n=n or seg(lod,8,6,4)
 for a,b in zip(points,points[1:]):
  if dot(sub(b,a),sub(b,a))>1e-10:m.tube(name,a,b,r,mat,n)


def frame(m,name,corners,r,mat,lod):
 polyline(m,name,[*corners,corners[0]],r,mat,lod)


def tire(m,x,y,z,lod,dual=False):
 """Rounded truck casing, bead/rim recess and a real ten-lug steel wheel."""
 n=seg(lod,24,12,8);axis=(0,1,0)
 profile=[(-.155,.343),(-.139,.444),(-.100,.505),(.100,.505),(.139,.444),(.155,.343)]
 m.lathe('Rounded_tire_casing',(x,y,z),profile,'Rubber',n,axis=axis,caps=False)
 # Geometry is not a black capped cylinder: small radial recess inside rim.
 for side in [-1,1]:
  outside=y+side*.153
  m.lathe('Wheel_rim',(x,outside,z),[(0,.347),(.020,.348),(.036,.305),(.048,.288)],'Chrome',n,axis=(0,side,0))
  m.lathe('Recessed_steel_wheel',(x,outside+side*.048,z),[(0,.286),(.025,.265),(.030,.105)],'DarkMetal',n,axis=(0,side,0))
  m.lathe('Wheel_hub',(x,outside+side*.080,z),[(0,.101),(.055,.096),(.075,.076)],'Chrome',max(8,n//2),axis=(0,side,0))
  if lod<2:
   for k in range([10,5][lod]):
    a=2*math.pi*k/[10,5][lod];xx=x+math.cos(a)*.144;zz=z+math.sin(a)*.144
    m.tube('Wheel_lug',(xx,outside+side*.064,zz),(xx,outside+side*.084,zz),.016,'Chrome',6)
   for k in range([8,4][lod]):
    a=2*math.pi*k/[8,4][lod];xx=x+math.cos(a)*.235;zz=z+math.sin(a)*.235
    m.lathe('Wheel_vent_inset',(xx,outside+side*.073,zz),[(0,.033),(.002,.033)],'Rubber',6,axis=(0,side,0))
  if lod==0:
   m.torus('Raised_sidewall_ring',(x,outside+side*.004,z),.409,.006,'Rubber',n,4,axis=axis)
 # Three staggered rows of low rectangular tread ribs on the visible crown.
 if lod<2:
  for k in range(seg(lod,24,12,8)):
   a=2*math.pi*k/seg(lod,24,12,8)
   for dy in [-.064,.064]:
    aa=a+(dy/.064)*.022;r=.507;da=.027
    m.tag='Tire_tread_blocks'
    m.face([(x+math.cos(aa-da)*r,y+dy-.046,z+math.sin(aa-da)*r),
            (x+math.cos(aa+da)*r,y+dy-.046,z+math.sin(aa+da)*r),
            (x+math.cos(aa+da)*r,y+dy+.046,z+math.sin(aa+da)*r),
            (x+math.cos(aa-da)*r,y+dy+.046,z+math.sin(aa-da)*r)][::-1],'Rubber')


def arch_fender(m,x,y0,y1,z,r,lod,mat='TruckPaint'):
 """Real upper half wheel arch: curved inner face, top and rolled side lip."""
 count=seg(lod,16,10,6);m.tag='Curved_front_fender'
 for k in range(count):
  a=.04+math.pi*.92*k/count;b=.04+math.pi*.92*(k+1)/count
  def p(rad,y,t):return(x+rad*math.cos(t),y,z+rad*math.sin(t))
  # Inner/outer surfaces follow tyre silhouette, rather than hiding it in box.
  m.face([p(r,y0,a),p(r,y0,b),p(r,y1,b),p(r,y1,a)],mat,smooth=True)
  m.face([p(r+.075,y1,a),p(r+.075,y1,b),p(r+.075,y0,b),p(r+.075,y0,a)],mat,smooth=True)
  for yy,reverse in [(y0,False),(y1,True)]:
   ps=[p(r,yy,a),p(r+.075,yy,a),p(r+.075,yy,b),p(r,yy,b)]
   m.face(ps[::-1] if reverse else ps,mat)
 for a in [.04,.04+math.pi*.92]:m.face([p(r,y0,a),p(r,y1,a),p(r+.075,y1,a),p(r+.075,y0,a)],mat)


def rivets(m,name,points,lod,axis=(0,1,0)):
 if lod:return
 for p in points:m.lathe(name,p,[(0,.009),(.006,.010)],'Chrome',6,axis=axis)


def truck(lod):
 m=Mesh('Truck_local');n=seg(lod);small=seg(lod,10,6,4)
 # Original short 1980s conventional day-cab tractor, pointing+X. No branding.
 for y in [-.48,.48]:
  m.box('Tractor_chassis_rail',(1.38,y,.78),(4.98,.15,.23),'DarkMetal')
 for x in [-1,.1,1.4,2.8,3.65]:m.box('Tractor_frame_crossmember',(x,0,.78),(.14,1.08,.15),'DarkMetal')
 for x in [-.75,.25,4.20]:
  m.tube('Axle',(x,-1.12,.515),(x,1.12,.515),.092,'DarkMetal',small)
  if x<1:
   m.lathe('Axle_differential',(x,0,.515),[(-.17,.09),(-.1,.19),(.1,.19),(.17,.09)],'DarkMetal',small,axis=(0,1,0))
   for y in [-.5,.5]:
    for k in range(3 if lod<2 else 1):m.box('Leaf_spring_pack',(x,y,.66+k*.018),(1.08-.12*k,.09,.018),'DarkMetal')
   for side in [-1,1]:
    # Dual rubber casings are physically separated by a narrow groove.
    for yy in ([.79,1.075] if lod<2 else [1.075]):tire(m,x,side*yy,.515,lod,dual=True)
  else:
   for side in [-1,1]:tire(m,x,side*.99,.515,lod)
 m.tube('Tractor_driveshaft',(-.75,0,.63),(2.05,0,.68),.061,'DarkMetal',small)
 m.rounded_box('Gearbox',(1.8,0,.73),(1.2,.49,.35),.08,'DarkMetal',max(1,3-lod))
 m.lathe('Fifth_wheel_plate',(-.35,0,1.01),[(0,.55),(.075,.55)],'DarkMetal',n)
 m.box('Fifth_wheel_slot',(-.63,0,1.058),(.55,.10,.012),'Rubber')
 # Cab lower panels have true side/window openings, not an opaque box inside glass.
 prism(m,'Cab_lower_body',[(1.65,1.02),(3.53,1.02),(3.53,2.03),(1.65,2.03)],-1.065,1.065,'TruckPaint')
 m.rounded_box('Cab_rounded_roof',(2.57,0,2.91),(2.02,2.22,.20),.15,'TruckPaint',max(1,4-lod))
 m.box('Cab_rear_wall',(1.69,0,2.43),(.10,2.13,.90),'TruckPaint')
 # Tall A/B pillars and separate rear quarter pillars.
 for side in [-1,1]:
  y=side*1.064
  m.box('Cab_B_pillar',(1.86,y,2.42),(.12,.10,.86),'TruckPaint')
  m.tube('Cab_A_pillar',(3.48,y,2.03),(3.27,y,2.83),.052,'TruckPaint',small)
  m.box('Side_window_lower_rail',(2.62,y,2.055),(1.45,.10,.09),'Chrome')
  m.tag='Side_cab_glass'
  ps=[(1.93,y,2.10),(3.43,y,2.10),(3.23,y,2.81),(1.93,y,2.81)]
  # +Y side faces outward; profile winding in XZ gives normal-Y.
  m.face(ps[::-1] if side==1 else ps,'WindowGlass')
  m.tube('Quarter_light_bar',(3.04,y+side*.006,2.10),(2.99,y+side*.006,2.80),.015,'Chrome',small)
  # Raised door edge, contrasting teal band and mechanical handle.
  frame(m,'Door_panel_reveal',[(1.83,y+side*.01,1.17),(3.32,y+side*.01,1.17),(3.32,y+side*.01,1.98),(1.83,y+side*.01,1.98)],.008,'DarkMetal',lod,)
  m.box('Teal_cab_side_stripe',(2.62,y+side*.016,1.61),(1.52,.016,.23),'TealPaint')
  m.box('Door_handle_mount',(2.02,y+side*.034,1.88),(.20,.03,.055),'Chrome')
  m.tube('Door_pull',(1.94,y+side*.065,1.89),(2.11,y+side*.065,1.89),.014,'Chrome',small)
  m.lathe('Door_lock',(1.95,y+side*.052,1.80),[(0,.018),(side*.005,.018)],'Chrome',6,axis=(0,1,0))
  # Step stringer and perforated steps, below the cab and behind front fender.
  for z,xx in [(.51,2.25),(.78,2.33)]:
   m.box('Cab_steel_step',(xx,side*1.10,z),(.72,.28,.075),'Chrome')
   if lod<2:
    for k in range(5):m.box('Step_grip_groove',(xx-.28+k*.14,side*1.10,z+.039),(.035,.24,.006),'DarkMetal')
  # Round aluminium tank, straps and period external vertical exhaust.
  m.lathe('Fuel_tank',(.95,side*.79,.64),[(-.40,.23),(-.35,.27),(.35,.27),(.40,.23)],'Chrome',n,axis=(1,0,0))
  for xx in [.66,1.24]:m.torus('Fuel_tank_strap',(xx,side*.79,.64),.274,.016,'DarkMetal',n,5,axis=(1,0,0))
  m.lathe('Fuel_filler',(.95,side*.79,.895),[(0,.06),(.035,.06)],'Chrome',small)
  m.tube('Mirror_lower_arm',(2.91,y,1.99),(2.79,side*1.29,2.30),.018,'Chrome',small)
  m.tube('Mirror_upper_arm',(2.95,y,2.61),(2.79,side*1.29,2.59),.016,'Chrome',small)
  m.rounded_box('Tall_chrome_mirror',(2.79,side*1.29,2.45),(.12,.075,.48),.028,'Chrome',max(1,3-lod))
  m.box('Mirror_dark_silver_glass',(2.721,side*1.29,2.45),(.008,.055,.42),'WindowGlass')
 # Slanted split windscreen, isolated rubber gasket and wipers.
 for side in [-1,1]:
  y0,y1=sorted([side*.035,side*.982]);ps=[(3.495,y0,2.085),(3.495,y1,2.085),(3.286,y1,2.806),(3.286,y0,2.806)]
  m.tag='Split_windscreen';m.face(ps,'WindowGlass');frame(m,'Windscreen_rubber_seal',ps,.022,'Rubber',lod)
  m.tube('Windscreen_wiper',(3.502,side*.12,2.10),(3.437,side*.73,2.33),.009,'DarkMetal',small)
 m.tube('Windscreen_centre_post',(3.504,0,2.06),(3.286,0,2.83),.022,'TruckPaint',small)
 m.box('Cab_dashboard',(3.22,0,1.994),(.35,1.96,.14),'DarkMetal')
 for y in [-.57,.57]:
  m.rounded_box('Cab_bench_seat',(2.27,y,1.47),(.70,.68,.19),.07,'Rubber',max(1,3-lod))
  m.rounded_box('Cab_seat_back',(1.95,y,1.84),(.16,.68,.65),.06,'Rubber',max(1,3-lod))
 m.tube('Steering_column',(2.80,.57,1.52),(2.90,.57,1.95),.027,'DarkMetal',small)
 m.torus('Steering_wheel',(2.91,.57,1.99),.18,.018,'Rubber',n,6,axis=(1,0,.26))
 for k in range(3):
  a=2*math.pi*k/3;m.tube('Steering_spoke',(2.91,.57,1.99),(2.91,.57+.155*math.cos(a),1.99+.155*math.sin(a)),.009,'Chrome',small)
 # Long sculpted conventional hood, flared real arch fenders and front grille.
 prism(m,'Sloping_engine_hood',[(3.47,1.20),(5.15,1.20),(5.15,1.89),(3.47,2.04)],-.77,.77,'TruckPaint')
 for side in [-1,1]:
  arch_fender(m,4.20,side*.80 if side==1 else side*1.25,side*1.25 if side==1 else side*.80,.515,.588,lod)
  m.box('Hood_teal_side_stripe',(4.31,side*.783,1.65),(1.55,.022,.20),'TealPaint')
  for xx in [3.60,4.93]:m.box('Hood_latch',(xx,side*.803,1.38),(.045,.045,.16),'Chrome')
  m.rounded_box('Headlamp_housing',(5.167,side*.89,1.23),(.15,.39,.40),.055,'Chrome',max(1,3-lod))
  m.lathe('Round_headlamp',(5.257,side*.89,1.28),[(0,.132),(.021,.132),(.032,.120)],'Headlamp',n,axis=(1,0,0))
  m.box('Front_turn_signal',(5.256,side*.89,1.075),(.04,.22,.071),'AmberLens')
  if lod<2:
   for k in range(5):m.tube('Headlamp_lens_fluting',(5.294,side*.89-.075+k*.038,1.205),(5.294,side*.89-.075+k*.038,1.35),.0028,'Headlamp',4)
 m.rounded_box('Radiator_outer_frame',(5.175,0,1.48),(.16,1.40,.87),.06,'Chrome',max(1,3-lod))
 m.box('Recessed_radiator',(5.264,0,1.49),(.015,1.25,.72),'DarkMetal')
 for k in range([12,8,5][lod]):m.box('Chrome_grille_vertical',(5.278,-.56+k*1.12/([12,8,5][lod]-1),1.49),(.025,.025,.68),'Chrome')
 for z in [1.24,1.48,1.73]:m.box('Grille_horizontal_tie',(5.293,0,z),(.025,1.24,.018),'Chrome')
 m.rounded_box('Front_bumper',(5.275,0,.88),(.20,2.52,.23),.045,'Chrome',max(1,3-lod))
 for side in [-1,1]:
  m.box('Bumper_tow_slot',(5.382,side*.65,.89),(.008,.19,.047),'DarkMetal')
  m.box('Front_mudflap',(3.57,side*1.03,.42),(.035,.45,.43),'Rubber')
 for y in [-.76,-.38,0,.38,.76]:
  m.rounded_box('Cab_roof_marker',(2.86,y,3.035),(.13,.08,.057),.022,'AmberLens',max(1,3-lod))
 m.tube('Chrome_exhaust_stack',(1.72,-1.10,1.0),(1.72,-1.10,3.24),.065,'Chrome',n)
 m.lathe('Exhaust_dark_outlet',(1.72,-1.10,3.24),[(0,.055),(.002,.055)],'DarkMetal',n)
 if lod<2:
  for z in [1.35,1.80,2.25]:m.tube('Exhaust_heat_guard_band',(1.72,-1.10,z),(1.72,-1.10,z+.08),.081,'DarkMetal',small)
  # Curved air/electric connections visibly bridge tractor and trailer.
  for yy,zz in [(-.3,1.5),(.15,1.60)]:
   pts=[(1.66,yy,zz),(1.40,yy,zz+.18),(1.17,yy,zz+.26),(.99,yy,zz+.21)]
   polyline(m,'Trailer_air_hose',pts,.025,'Rubber',lod)
 # Separate semi-trailer; it has a visible coupling gap, landing gear, reefer unit.
 for y in [-.89,.89]:m.box('Trailer_chassis_rail',(-2.22,y,1.115),(6.30,.13,.22),'DarkMetal')
 m.box('Trailer_insulated_floor',(-2.20,0,1.295),(6.30,2.43,.16),'Chrome')
 # Side/rear/front panel shell, true closed refrigerated box.
 for side in [-1,1]:
  y=side*1.226
  m.box('Insulated_trailer_side',(-2.20,y,2.56),(6.30,.075,2.42),'TruckPaint')
  m.box('Trailer_teal_stripe',(-2.20,y+side*.041,1.96),(6.25,.012,.26),'TealPaint')
  for z in [1.36,3.78]:m.box('Trailer_side_edge_extrusion',(-2.20,y+side*.046,z),(6.38,.043,.065),'Chrome')
  for xx in [-5.36,.97]:m.box('Trailer_corner_post',(xx,y,2.56),(.065,.10,2.48),'Chrome')
  # Thin period folded panel seams; coarser LODs preserve vertical rhythm.
  for k in range([11,6,4][lod]):
   xx=-5.20+k*6.0/([11,6,4][lod]-1)
   m.box('Trailer_panel_seam',(xx,y+side*.042,2.57),(.014,.010,2.30),'Chrome')
   rivets(m,'Trailer_panel_rivet',[(xx,y+side*.050,z) for z in [1.47,2.02,2.58,3.15,3.66]],lod,axis=(0,side,0))
  for xx in [-5.0,-2.3,.6]:
   m.box('Trailer_amber_clearance',(xx,y+side*.065,1.40),(.15,.034,.055),'AmberLens')
  m.box('Trailer_rear_mudflap',(-4.95,side*.96,.31),(.035,.53,.48),'Rubber')
 m.rounded_box('Trailer_rolled_roof',(-2.20,0,3.79),(6.40,2.54,.13),.07,'TruckPaint',max(1,4-lod))
 m.box('Trailer_front_bulkhead',(.99,0,2.57),(.08,2.42,2.42),'TruckPaint')
 # Mechanical refrigeration unit on trailer front, distinctly separate from cab.
 m.rounded_box('Refrigeration_front_unit',(1.17,0,2.87),(.32,1.55,1.37),.09,'TruckPaint',max(1,4-lod))
 m.box('Reefer_dark_condenser',(1.341,0,2.86),(.025,1.32,1.01),'DarkMetal')
 for k in range([14,8,5][lod]):m.box('Refrigeration_louvre',(1.366,0,2.43+k*.82/([14,8,5][lod]-1)),(.036,1.30,.028),'Chrome')
 m.box('Reefer_service_lower',(1.35,0,2.23),(.035,1.24,.24),'TruckPaint')
 m.box('Reefer_service_handle',(1.377,.42,2.24),(.028,.15,.025),'DarkMetal')
 if lod<2:
  for yy in [-.39,.39]:m.lathe('Reefer_fan_hub',(1.386,yy,3.18),[(0,.07),(.024,.07)],'DarkMetal',small,axis=(1,0,0))
 # Rear twin doors, hinge straps and vertical locking rods.
 m.box('Trailer_rear_dark_aperture',(-5.385,0,2.56),(.04,2.39,2.35),'DarkMetal')
 for side in [-1,1]:
  yy=side*.598
  m.box('Trailer_rear_door',(-5.418,yy,2.57),(.045,1.17,2.30),'TruckPaint')
  m.tube('Rear_door_lock_rod',(-5.451,side*.38,1.52),(-5.451,side*.38,3.63),.018,'Chrome',small)
  for z in [1.60,2.56,3.49]:m.box('Rear_hinge_strap',(-5.456,side*1.075,z),(.055,.27,.052),'Chrome')
  m.box('Rear_lock_handle',(-5.49,side*.38,2.05),(.055,.32,.032),'DarkMetal')
  for z in [1.51,3.66]:m.box('Rear_red_clearance',(-5.474,side*1.02,z),(.028,.14,.065),'RedLens')
 m.box('Trailer_rear_bumper',(-5.45,0,.92),(.13,2.39,.15),'Chrome')
 for yy in [-.90,.90]:
  m.box('Rear_bumper_support',(-5.20,yy,1.09),(.13,.11,.42),'DarkMetal')
  m.box('Rear_tail_light',(-5.53,yy,1.13),(.025,.28,.115),'RedLens')
 for xx in [-4.45,-3.42]:
  m.tube('Trailer_axle',(xx,-1.13,.515),(xx,1.13,.515),.09,'DarkMetal',small)
  for side in [-1,1]:
   for yy in ([.79,1.075] if lod<2 else [1.075]):tire(m,xx,side*yy,.515,lod,dual=True)
   m.box('Trailer_suspension',(xx,side*.75,.82),(.83,.14,.18),'DarkMetal')
 for yy in [-.81,.81]:
  m.box('Raised_landing_leg',(-.55,yy,.98),(.10,.11,.50),'DarkMetal')
  m.box('Landing_foot',(-.55,yy,.72),(.30,.23,.045),'DarkMetal')
  m.tube('Landing_gear_brace',(-1.05,yy,1.20),(-.55,yy,.76),.035,'DarkMetal',small)
 # The canonical static prop is baked once into the original site coordinates.
 out=Mesh('SM_Mesa_Truck');out.merge(m,RIG_CENTER);return out


# Original centreline alphabet, shaped for physical tubing (no copied typeface).
LETTER={
 'M':[[(0,0),(0,1),(.50,.40),(1,1),(1,0)]],
 'E':[[(1,1),(0,1),(0,0),(1,0)],[(0,.51),(.79,.51)]],
 'S':[[(.95,.96),(.75,1),(.18,1),(0,.82),(.05,.62),(.23,.53),(.76,.47),(.98,.32),(.95,.12),(.78,0),(.15,0),(0,.08)]],
 'A':[[(0,0),(.39,1),(.61,1),(1,0)],[(.20,.40),(.80,.40)]],
 'D':[[(0,0),(0,1),(.58,1),(.91,.85),(1,.65),(1,.30),(.86,.10),(.60,0),(0,0)]],
 'I':[[(.14,1),(.86,1)],[(.50,1),(.50,0)],[(.14,0),(.86,0)]],
 'N':[[(0,0),(0,1),(1,0),(1,1)]],
 'R':[[(0,0),(0,1),(.67,1),(.96,.85),(.96,.65),(.69,.51),(0,.51)],[(.52,.51),(1,0)]],
}


def sign_word(m,text,x,center_y,z,width,height,lod,back=False):
 gap=.11;letterw=(width-gap*(len(text)-1))/len(text);start=center_y-width/2
 # Text reads left-right from the observing side; local front+X sees +Y right.
 direction=1 if not back else -1
 for i,letter in enumerate(text):
  for path in LETTER[letter]:
   pts=[(x,center_y+direction*(start-center_y+i*(letterw+gap)+px*letterw),z+pz*height) for px,pz in path]
   polyline(m,'Dark_neon_channel_'+text,[(p[0]-.030*direction,p[1],p[2]) for p in pts],.028,'DarkMetal',lod)
   polyline(m,'Original_neon_'+text,pts,.0135 if text=='MESA' else .014,'SignRedGlow' if text=='MESA' else 'SignCyanGlow',lod)


def sign(lod):
 m=Mesh('Sign_local');n=seg(lod)
 # Tapered steel support, bolted concrete shoe, side diagonals and wiring box.
 m.box('Sign_concrete_foot',(0,0,.105),(.75,.82,.21),'Concrete')
 m.box('Sign_steel_baseplate',(0,0,.235),(.52,.56,.055),'DarkMetal')
 for yy in [-.21,.21]:
  for xx in [-.19,.19]:m.lathe('Sign_anchor_bolt',(xx,yy,.265),[(0,.025),(.045,.025)],'Chrome',6)
 prism(m,'Tapered_teal_sign_post',[(-.105,.25),(.105,.25),(.070,3.62),(-.070,3.62)],-.12,.12,'SignTeal')
 for yy in [-1.20,1.20]:m.tube('Sign_diagonal_brace',(-.05,0,1.90),(-.05,yy,3.19),.035,'DarkMetal',max(6,n//2))
 m.box('Sign_electrical_case',(-.19,0,1.02),(.15,.33,.44),'SignTeal')
 m.tube('Sign_electrical_conduit',(-.27,0,.27),(-.27,0,2.70),.018,'DarkMetal',6)
 outline=[(-1.56,3.02),(1.56,2.96),(1.69,3.32),(1.39,5.28),(-1.48,5.48),(-1.75,5.20)]
 plate_yz(m,'Asymmetric_weathered_teal_backing',outline,-.092,.092,'SignTeal')
 # Side returns keep the board visibly thick at oblique player angles.
 frame(m,'Folded_sign_edge',[(.105,y,z) for y,z in outline],.022,'Chrome',lod)
 frame(m,'Back_sign_edge',[(-.105,y,z) for y,z in outline],.022,'Chrome',lod)
 # Red upper signface is deliberately canted, like the original concept art.
 upper=[(-1.41,4.23),(1.43,4.15),(1.25,5.20),(-1.45,5.36)]
 for side in [-1,1]:
  x0,x1=sorted([side*.103,side*.119]);plate_yz(m,'Faded_red_MESA_backplate',upper,x0,x1,'PaintRed')
  sign_word(m,'MESA',side*.151,0,4.40,2.47,.67,lod,back=side<0)
  sign_word(m,'DINER',side*.138,0,3.28,2.58,.68,lod,back=side<0)
 # Period bent arrow at top, with narrow irregular star and original neon edge.
 arrow=[(-.54,5.34),(-.54,5.67),(-1.20,5.89),(-.28,5.77),(.03,6.18),(.16,5.73),(.64,5.47),(.12,5.55),(-.12,5.24)]
 plate_yz(m,'Midcentury_arrow_star',arrow,-.065,.065,'SignTeal')
 for side in [-1,1]:
  frame(m,'Star_neon_edge',[(side*.088,y,z) for y,z in arrow],.009,'SignCyanGlow',lod)
 # Small real bolt heads on the board, never raster highlights.
 if lod<2:
  for yy,zz in [(-1.44,3.16),(1.42,3.14),(-1.38,5.14),(1.22,5.04)]:
   for side in [-1,1]:m.tube('Signface_screw',(side*.121,yy,zz),(side*.133,yy,zz),.012,'Chrome',6)
 out=Mesh('SM_Mesa_Sign');out.merge(m,SIGN_ANCHOR);return out


def bench(m,lod):
 """Seat height and clear seat point exactly preserve the takeaway anchor."""
 n=seg(lod,10,6,4);segments=max(1,3-lod)
 for y in [-5.73,-4.27]:
  # Cast iron side frames, subtly splayed feet, wooden slatted seat/back.
  polyline(m,'Bench_cast_side',[(2.10,y,.05),(2.22,y,.43),(2.69,y,.43),(2.77,y,.05)],.032,'DarkMetal',lod)
  polyline(m,'Bench_back_support',[(2.22,y,.23),(2.13,y,.65),(2.05,y,.99)],.025,'DarkMetal',lod)
  m.rounded_box('Bench_foot',(2.10,y,.043),(.16,.11,.046),.025,'DarkMetal',segments)
  m.rounded_box('Bench_foot',(2.77,y,.043),(.16,.11,.046),.025,'DarkMetal',segments)
 for x in [2.19,2.35,2.51,2.67]:m.rounded_box('Weathered_bench_seat_slat',(x,-5,.438),(.143,1.88,.044),.018,'Wood',segments)
 for z in [.65,.81,.97]:
  x=2.16-(z-.60)*.24
  m.rounded_box('Bench_back_slat',(x,-5,z),(.054,1.88,.133),.018,'Wood',segments)
  if lod==0:
   for yy in [-5.73,-4.27]:m.tube('Bench_slat_bolt',(x+.030,yy,z),(x+.041,yy,z),.010,'Chrome',6)
 m.tube('Bench_underseat_brace',(2.43,-5.74,.22),(2.43,-4.26,.22),.025,'DarkMetal',n)



def facade_foot(m,lod):
 """Sparse original kit plants/stone at the wall, clear of protected use areas."""
 # Read-only original CN3 source. A new mesh stores copied faces/UVs in this kit;
 # the delivered Blender/FBX/GLB never need to resolve this Python dependency.
 path=Path(__file__).resolve().parent.parent/'calichenature/vegetation/geometry.py'
 spec=importlib.util.spec_from_file_location('mesa_original_caliche_botany',path)
 source=importlib.util.module_from_spec(spec)
 import sys
 sys.modules[spec.name]=source;spec.loader.exec_module(source)
 plants=[('Grass_A',2.97,-7.75,.78,.7),('Grass_A',3.05,-3.05,.73,2.1),
         ('Grass_A',3.02,2.25,.62,.1),('Grass_B',3.00,4.35,.62,3.2),
         ('Grass_A',3.01,8.10,.82,4.8),('Sage_C',3.12,-8.65,.71,1.2),
         ('Sage_C',3.10,6.75,.82,2.6)]
 specs={s[0]:s for s in source.SPECS}
 for name,x,y,scale,yaw in plants:
  plant=source.build(specs[name],lod);co,si=math.cos(yaw),math.sin(yaw)
  m.tag='Original_Caliche_'+name
  for face,mat in zip(plant.faces,plant.material):
   points=[]
   for i in face:
    px,py,pz=plant.vertices[i];points.append((x+scale*(px*co-py*si),y+scale*(px*si+py*co),.006+scale*pz))
   m.face(points,'DryFoliage' if mat=='M_CN_Foliage' else 'PlantBark',[plant.uv[i] for i in face])
 # Irregular angular contact stones anchor sparse plants to the dusty base.
 rng=random.Random(471986)
 for k in range([24,15,10][lod]):
  y=rng.choice([-8.65,-7.75,-3.05,2.25,4.35,6.75,8.10])+rng.uniform(-.34,.34)
  x=rng.uniform(2.84,3.42);r=rng.uniform(.055,.15);h=rng.uniform(.05,.13)
  angle=rng.random()*math.tau
  ring=[(x+math.cos(angle+j*math.tau/5)*r*rng.uniform(.8,1.12),y+math.sin(angle+j*math.tau/5)*r*rng.uniform(.65,1.1),.006) for j in range(5)]
  top=(x+r*.13,y-r*.11,.006+h);m.tag='Facade_contact_caliche'
  for j in range(5):m.face([ring[j],ring[(j+1)%5],top],'Gravel')


def parking(lod):
 m=Mesh('SM_Mesa_Parking');rng=random.Random(1986)
 # Asphalt and gravel are distinct original PBR surfaces, flat where game drives.
 # Grid creates an irregular ragged edge, with a real .10m fall at the perimeter.
 nx,ny=18,40;x0,x1=2.80,16.70;y0,y1=-16,16
 grid=[]
 for i in range(nx+1):
  row=[]
  for j in range(ny+1):
   x=x0+(x1-x0)*i/nx;y=y0+(y1-y0)*j/ny
   edge=min(x-x0,x1-x,y-y0,y1-y)
   z=.006 if edge>=.52 else -.10+max(0,edge)/.52*.106
   row.append((x,y,z))
  grid.append(row)
 for i in range(nx):
  for j in range(ny):
   m.tag='Contacted_gravel_lot';ps=[grid[i][j],grid[i+1][j],grid[i+1][j+1],grid[i][j+1]]
   m.face(ps,'Gravel',[(p[0]/2.2,p[1]/2.2) for p in ps])
 # Worn blacktop over compacted gravel, ragged edge with varied breadth.
 # Unique lot UVs carry the authored40-70cm dusty fade; no repeated dark mat.
 loops=[]
 for j in range(42):
  yy=-15.70+j*31.40/41
  left=3.22+.20*math.sin(yy*.52)+rng.uniform(-.08,.08)
  right=16.20+.15*math.sin(yy*.61+.8)+rng.uniform(-.08,.08)
  loops.append([(left,yy,.014),(right,yy,.014)])
 for j in range(len(loops)-1):
  m.tag='Ragged_old_asphalt';ps=[loops[j][0],loops[j][1],loops[j+1][1],loops[j+1][0]]
  m.face(ps,'Asphalt',[((p[0]-2.8)/13.9,(p[1]+16)/32) for p in ps])
 # Small broken patches near the building, away from the drivable centre.
 if lod<2:
  for k in range(19):
   y=-14.5+k*1.6;x=3.75+rng.uniform(-.15,.35);r=rng.uniform(.14,.29)
   ps=[(x+math.cos(a)*r*rng.uniform(.7,1.2),y+math.sin(a)*r*rng.uniform(.7,1.2),.017) for a in [2*math.pi*i/7 for i in range(7)]]
   m.tag='Asphalt_gravel_spall';m.face(ps,'Gravel',[(p[0]/2.2,p[1]/2.2) for p in ps])
 # Thin front apron meets existing threshold and keeps the original door walk free.
 for j in range(12):
  ya=-9.2+j*18.4/12;yb=-9.2+(j+1)*18.4/12
  # The entrance gets its broad threshold; other bays have only a narrow worn walk.
  outer=3.29 if ya<1.4 and yb>-1.4 else 2.69
  m.tag='Concrete_front_apron';ps=[(2.005,ya,.020),(outer,ya,.020),(outer,yb,.020),(2.005,yb,.020)]
  m.face(ps,'Concrete',[(p[0]/1.3,p[1]/1.3) for p in ps])
  if j>0:m.box('Apron_expansion_joint',((2.005+outer)/2,ya,.021),(outer-2.02,.014,.003),'DarkMetal')
 # Fragmented paint rather than immaculate full-white modern parking lines.
 for yy in [5.05,8.05,11.05]:
  for k in range([15,10,7][lod]):
   if k in [2,8,12]:continue
   x=4.55+k*3.55/[15,10,7][lod]
   m.box('Faded_parking_bay_paint',(x,yy,.023),(.16 if lod==0 else .25,.071,.004),'YardMarking')
 for yy in [6.55,9.55]:
  # Original cars sit at x6.25; stops are west of the cars, outside DinerPark.
  m.rounded_box('Concrete_wheel_stop',(4.35,yy,.074),(.20,1.40,.14),.045,'Concrete',max(1,3-lod))
  for dy in [-.46,.46]:m.lathe('Wheel_stop_anchor',(4.35,yy+dy,.147),[(0,.014),(.009,.014)],'DarkMetal',6)
 # Cracks are thin dark mineral fills, never long raised collision strips.
 cracks=[[(4.0,-11.7),(4.25,-10.9),(4.17,-10.0),(4.7,-9.7)],[(12.9,-13.4),(13.5,-12.8),(13.3,-12.1),(14.0,-11.8)],[(8.8,11.4),(9.3,11.1),(9.5,10.5)],[(3.95,2.9),(4.2,2.2),(4.15,1.6)]]
 for path in cracks:
  for a,b in zip(path,path[1:]):
   d=unit((b[0]-a[0],b[1]-a[1],0));p=(-d[1]*.008,d[0]*.008,0)
   m.tag='Filled_asphalt_crack';aa=(*a,.024);bb=(*b,.024);m.face([sub(aa,p),sub(bb,p),add(bb,p),add(aa,p)],'DarkMetal')
 # Sparse low contact chips at edges. NO fixed object in drive/vending rectangle.
 for k in range([100,48,20][lod]):
  y=rng.uniform(-15.6,15.6);x=rng.choice([3.45,16.30])+rng.uniform(-.15,.10)
  if -1.4<y<1.4 and x<4:continue
  radius=rng.uniform(.028,.085);height=radius*rng.uniform(.28,.6)
  angle=rng.random()*math.tau;ps=[(x+math.cos(angle+i*math.tau/5)*radius,y+math.sin(angle+i*math.tau/5)*radius,.005) for i in range(5)]
  top=(x+radius*.18,y-radius*.08,.005+height);m.tag='Loose_contact_gravel'
  for i in range(5):m.face([ps[i],ps[(i+1)%5],top],'Gravel')
 facade_foot(m,lod)
 bench(m,lod)
 return m


def build(lod=0):
 assert lod in (0,1,2)
 return {m.name:m for m in [truck(lod),sign(lod),parking(lod)]}
