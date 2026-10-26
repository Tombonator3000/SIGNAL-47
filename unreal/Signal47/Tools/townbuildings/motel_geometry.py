"""Sierra court/reception and separate Room6/bathroom, preserving live props.
The court is authored about[-45.9,H,674.9], Room6 about[0,0,-8000].
"""
import math
from geometry import *
from town_geometry import sheet_stack,cabinet,book

def court(lod):
 s=Mesh('SM_Town_Motel_Shell');g=Mesh('SM_Town_Motel_Glass');i=Mesh('SM_Town_Motel_Office');f=Mesh('SM_Town_Motel_Fixtures');n=seg(lod)
 # Office exterior exactly preserves1.4m door and2.1m window voids.
 front_wall(s,2.30,-2.9,2.9,2.8,[(.1,1.5,0,2.2),(-2.35,-.25,.71,2.07)],'Stucco',.20)
 for y in [-2.8,2.8]:s.box('Office_side_wall',(0,y,1.4),(4.8,.20,2.8),'Stucco')
 s.box('Office_rear_wall',(-2.30,0,1.4),(.20,5.6,2.8),'Stucco')
 # Rear wing and genuine recessed unit facades; front glazing is never an image billboard.
 s.box('Court_rear_wing',(-4.5,-21.1,1.4),(4.2,48,2.8),'Stucco')
 s.box('Court_roof_deck',(-2.1,-21.1,2.90),(9.15,48.2,.22),'Concrete')
 for y in [-45.15,2.95]:s.box('Court_roof_teal_end',(-2.1,y,3.02),(9.2,.16,.18),'TealPaint')
 s.box('Court_long_front_fascia',(2.50,-21.1,3.02),(.12,48.2,.21),'TealPaint')
 # Guest units retain all original closed leaves; room6's animated leaf remains runtime owned.
 for room in range(1,9):
  y=24.9-(25+room*5);wy=y-2.03
  front_wall(s,2.28,y-3.52,y+1.46,2.8,[(y-.51,y+.51,0,2.10),(wy-.70,wy+.70,.78,1.98)],'Stucco',.15)
  frame(s,2.38,y,1.065,.96,2.10,lod,'TealPaint',.18)
  if room!=6:
   s.box('Independent_guest_room_door',(2.34,y,1.015),(.06,.92,2.03),'Timber')
   for z in [.45,1.05,1.65]:s.box('Recessed_guest_door_panel',(2.379,y,z),(.025,.66,.38),'DarkWood')
   s.lathe('Guest_door_brass_knob',(2.41,y-.32,.96),[(0,.025),(.035,.03)],'Metal',n//2,axis=(1,0,0))
  pane(g,2.38,wy,1.38,1.4,1.2)
  frame(s,2.37,wy,1.38,1.4,1.2,lod,'TealPaint',.19)
  # Curtains, airgap and a shallow real room return behind each guest window.
  s.box('Guest_room_recess_back',(1.20,wy,1.44),(.12,1.66,1.48),'Ceiling')
  for q in range([12,8,4][lod]):s.tube('Curtain_physical_fold',(1.80,wy-.63+q*1.26/([12,8,4][lod]-1),.80),(1.80,wy-.63+q*1.26/([12,8,4][lod]-1),1.96),.035,'LampFabric',5)
  s.box('Throughwall_AC_body',(2.40,wy,.38),(.31,.66,.39),'Metal')
  for k in range([7,5,3][lod]):s.box('AC_louvred_vent',(2.568,wy,.24+k*.044),(.013,.57,.012),'DarkMetal')
  s.box('Room_number_plaque',(2.42,y+.70,1.78),(.025,.22,.31),'TealPaint')
  if room==6:s.panel('Room6_number',(2.44,y+.70,1.78),.19,.27,6)
  # Visible numerals for other rooms use small raised geometric bars, original vector lettering.
  else:
   from lettering import text_mesh
   temp=Mesh('number');text_mesh(temp,str(room),(.0,0,0),.16,'Paper',normal=(1,0,0));s.merge(temp,(2.445,y+.70,1.78))
  s.box('Shielded_guest_door_sconce',(2.47,y,2.23),(.25,.29,.08),'DarkMetal')
  f.box('Guest_door_opal_diffuser',(2.53,y,2.17),(.18,.22,.035),'MotelOfficeGlow')
 # Continuous covered walk and columns match existing gait plane, not a raised new slab.
 # Exact original3slabs: edge-to-edge with the office and retained parking.
 for a,b,c,d in [(2.15,5.15,2.7,3.3),(2.2,5.15,-2.7,2.7),(2.15,5.15,-45.9,-2.7)]:s.box('Covered_court_walk',((a+b)/2,(c+d)/2,-.07),(b-a,d-c,.14),'Concrete')
 s.box('Covered_walk_roof',(3.45,-21.1,2.85),(2.9,49.0,.15),'TealPaint')
 for y in [2.9,-5.1,-10.1,-15.1,-20.1,-25.1,-30.1,-35.1,-40.1,-45.1]:s.box('Covered_walk_steel_post',(4.6,y,1.35),(.10,.10,2.70),'TealPaint')
 # Office glass shows a fully modelled reception, wall key board and original live media.
 pane(g,2.411,-1.30,1.39,2.04,1.36);frame(s,2.39,-1.3,1.39,2.04,1.36,lod,'TealPaint',.22)
 s.panel('Reception_OFFICE_sign',(2.425,.80,2.49),1.15,.29,7)
 s.panel('Motel_rate_card',(2.426,-2.61,1.67),.60,.44,8)
 # The canonical office door is already fixed open inward; retain an identical clear aperture.
 s.box('Open_office_door_leaf',(1.62,.05,1.05),(1.15,.055,2.10),'TealPaint')
 i.box('Reception_floor_authority',(0,0,-.055),(4.4,5.4,.11),'Terrazzo')
 i.box('Reception_acoustic_ceiling',(0,0,2.75),(4.4,5.4,.10),'Ceiling')
 # Counter footprint/source top1.020m: physical documents stay at1.027m.
 i.box('Oak_reception_counter',(-.80,-.35,.475),(1.0,3.2,.95),'DarkWood')
 i.rounded_box('Reception_counter_bullnose',(-.80,-.35,.985),(1.16,3.34,.07),.035,'Timber',3)
 for yy in [1.10-k*.22 for k in range(14)]:i.box('Counter_fluted_wood_apron',(-.28,yy,.45),(.020,.026,.73),'Timber')
 # Register printed on retained original surface is excluded; no double story documents.
 i.box('Keyboard_oak_back',(-2.1,1.67,1.67),(.075,1.55,.63),'DarkWood')
 for room in range(1,9):
  row=0 if room<=4 else 1;col=(room-1)%4;yy=2.22-col*.36;z=1.83-row*.3
  from lettering import text_mesh
  temp=Mesh('key_number');text_mesh(temp,str(room),(0,0,0),.07,'Paper',normal=(1,0,0));i.merge(temp,(-2.055,yy,z+.075))
  if room!=6:
   i.tube('Key_board_hook',(-2.055,yy,z-.04),(-2.005,yy,z-.065),.008,'Metal',5);i.box('Old_brass_room_key',(-1.99,yy,z-.12),(.012,.055,.12),'Metal')
 # Wall telephone base only; original handset at[.43,2.52,1.57] remains independent.
 i.box('Oak_wall_telephone_base',(.23,2.61,1.46),(.32,.16,.52),'Timber')
 i.box('Telephone_lower_shelf',(.23,2.56,1.17),(.37,.25,.04),'DarkWood')
 i.torus('Telephone_rotary_dial',(.23,2.514,1.48),.065,.013,'Metal',n,5,axis=(0,-1,0))
 i.tube('Telephone_mouthpiece_stem',(.23,2.51,1.73),(.23,2.42,1.69),.018,'DarkMetal',6)
 # Media shelf accommodates original Kelvora TV at[-1.87,-1.95,1.73].
 i.box('Reception_media_shelf',(-1.93,-1.95,1.48),(.55,1.06,.06),'Timber')
 for z in [.18,.57,1.02]:
  i.box('Reception_bookcase_shelf',(-1.93,-1.95,z),(.45,1.0,.05),'DarkWood')
  for y in [-2.25,-2.02,-1.80,-1.60]:book(i,-1.82,y,z+.025,.15,.25)
 chair(i,1.0,-2.2,lod,yaw=math.pi);tube_light(f,-.3,0,2.65,1.2,'MotelOfficeGlow')
 return {m.name:m for m in [s,g,i,f]}

def room6(lod):
 s=Mesh('SM_Town_Motel_Room6Shell');i=Mesh('SM_Town_Motel_Room6Interior');f=Mesh('SM_Town_Motel_Room6Fixtures');bath=Mesh('SM_Town_Motel_Bathroom');n=seg(lod)
 # South wall has the original entrance. North wall now contains a real bath aperture,
 # while original closed door / interaction remain runtime-owned.
 for x in [-2.33,2.33]:s.box('Room6_side_wall',(x,0,1.3),(.16,6.16,2.6),'Stucco')
 south=Mesh('South_wall');front_wall(south,0,-2.25,2.25,2.6,[(-1.98,-.82,0,2.15),(-.2,1.75,.9,2.1)],'Stucco',.16);s.merge(south,(0,-3.08,0),-math.pi/2)
 north=Mesh('North_wall');front_wall(north,0,-2.25,2.25,2.6,[(-1.43,-.51,0,2.14)],'Stucco',.16);s.merge(north,(0,3.08,0),-math.pi/2)
 s.box('Room6_ceiling',(0,0,2.69),(4.82,6.32,.18),'Ceiling');i.box('Room6_carpet_floor',(0,0,-.055),(4.5,6,.11),'Carpet')
 s.box('Room6_covered_walk',(0,-4.1,-.07),(5.4,2.2,.14),'Concrete');s.box('Room6_walk_canopy',(0,-4.05,2.72),(5.4,2.3,.14),'TealPaint')
 for x in [-2.55,2.55]:s.box('Room6_canopy_post',(x,-5.05,1.325),(.12,.12,2.65),'TealPaint')
 # Interior trims deliberately stop at entrance and bathroom voids.
 for x in [-2.2,2.2]:i.box('Room6_side_skirting',(x,0,.06),(.06,6,.12),'TealPaint')
 for a,b in [(-2.25,-1.43),(-.51,2.25)]:i.box('Bath_wall_split_skirting',((a+b)/2,2.97,.06),(b-a,.06,.12),'TealPaint')
 for a,b in [(-2.25,-1.98),(-.82,2.25)]:i.box('Entry_split_skirting',((a+b)/2,-2.97,.06),(b-a,.06,.12),'TealPaint')
 for x in [-1.98,-.82]:s.box('Room6_door_jamb',(x,-3.02,1.1),(.06,.20,2.2),'TealPaint')
 s.box('Room6_door_header',(-1.4,-3.02,2.17),(1.22,.20,.07),'TealPaint')
 s.panel('Room6_external_number',(-.61,-3.17,1.75),.18,.27,6,normal=(0,-1,0))
 # Existing room window/light canvas remains runtime owned; new solid deep sill and folds.
 for z in [.88,2.12]:i.box('Room6_window_rail',(.775,-2.96,z),(2.06,.14,.06),'TealPaint')
 for x in [-.21,1.65]:
  for k in range([5,4,2][lod]):i.tube('Room6_physical_curtain_fold',(x+k*.05,-2.80+(k%2)*.045,.77),(x+k*.05,-2.80+(k%2)*.045,2.15),.034,'Bedding',6)
 i.box('Room6_AC_casing',(.75,-2.85,.33),(1.05,.12,.42),'CreamEnamel')
 for k in range(9):i.box('Room6_AC_grille',(.75,-2.781,.2+k*.032),(.88,.018,.018),'DarkMetal')
 # Reuse original own bed through existing Props, and retain all live objects/Nora.
 i.box('Room6_nightstand',(-.26,2.5,.28),(.55,.60,.52),'Timber');i.box('Room6_nightstand_top',(-.26,2.5,.56),(.60,.66,.05),'DarkWood')
 i.box('Room6_nightstand_drawer',(.03,2.5,.32),(.025,.49,.24),'Timber')
 i.tube('Nightstand_handle',(.055,2.43,.32),(.055,2.57,.32),.012,'Metal',6)
 i.box('Room6_dresser_body',(-1.88,.8,.435),(.64,1.45,.79),'Timber');i.box('Room6_dresser_top',(-1.88,.8,.85),(.72,1.51,.055),'DarkWood')
 for k in range(3):
  i.box('Room6_drawer_face',(-1.54,.8,.22+k*.215),(.035,1.33,.19),'Timber');i.tube('Room6_drawer_pull',(-1.50,.70,.23+k*.215),(-1.50,.90,.23+k*.215),.013,'Metal',6)
 # Table top=.810 matches documents at.815; old proxy center=.775 unchanged.
 table(i,(.7,-1.45),(1.35,.95),.810,'Timber');chair(i,.75,-2.3,lod);chair(i,.75,-.56,lod,yaw=math.pi)
 # Coherent bathroom, original closed boundary atY3.0; no new walk/admission mechanic.
 bath.box('Bathroom_tile_floor',(-.67,4.25,-.055),(2.65,2.45,.11),'FloorTile')
 for x in [-2.0,.66]:bath.box('Bathroom_wall',(x,4.25,1.3),(.12,2.5,2.6),'Stucco')
 bath.box('Bathroom_rear_wall',(-.67,5.5,1.3),(2.78,.12,2.6),'Stucco');bath.box('Bathroom_ceiling',(-.67,4.25,2.65),(2.78,2.62,.10),'Ceiling')
 # Toilet has curved porcelain tank/bowl, lid and visible trap pedestal.
 bath.lathe('Porcelain_toilet_pedestal',(-1.35,4.85,0),[(0,.16),(.12,.13),(.30,.18),(.38,.25)],'Ceramic',n)
 bath.lathe('Porcelain_toilet_bowl',(-1.35,4.85,.30),[(0,.17),(.10,.27),(.15,.29),(.19,.28)],'Ceramic',n,caps=False)
 bath.torus('Toilet_seat',(-1.35,4.85,.49),.245,.030,'Ceramic',n,6)
 bath.rounded_box('Toilet_tank',(-1.35,5.19,.69),(.43,.20,.45),.055,'Ceramic',3)
 bath.box('Toilet_tank_lid',(-1.35,5.19,.93),(.46,.23,.035),'Ceramic')
 bath.lathe('Porcelain_sink_pedestal',(.15,4.85,0),[(0,.17),(.12,.09),(.74,.08)],'Ceramic',n)
 bath.lathe('Sink_basin',(.15,4.85,.73),[(0,.16),(.08,.28),(.11,.28)],'Ceramic',n,caps=False)
 bath.tube('Sink_faucet',(.15,5.02,.83),(.15,5.02,1.0),.017,'Metal',8);bath.tube('Sink_faucet_spout',(.15,5.02,1.0),(.15,4.88,1.0),.017,'Metal',8)
 bath.box('Medicine_cabinet',(.15,5.405,1.48),(.47,.10,.65),'Metal');bath.box('Polished_mirror_front',(.15,5.342,1.48),(.42,.015,.60),'Metal')
 bath.box('Shower_pan',(.08,3.80,.055),(1.02,1.20,.11),'Ceramic')
 for k in range([18,12,6][lod]):bath.tube('Shower_curtain_physical_fold',(-.40+k*.96/([18,12,6][lod]-1),4.36,.17),(-.40+k*.96/([18,12,6][lod]-1),4.36,2.20),.035,'LampFabric',6)
 f.tube('Shower_rod',(-.48,4.36,2.27),(.64,4.36,2.27),.018,'Metal',8)
 f.lathe('Bathroom_opal_ceiling_light',(-.67,4.25,2.55),[(0,.19),(.06,.18),(.10,.12)],'Room6Glow',n)
 return {m.name:m for m in [s,i,f,bath]}
def build(lod):return {**court(lod),**room6(lod)}
