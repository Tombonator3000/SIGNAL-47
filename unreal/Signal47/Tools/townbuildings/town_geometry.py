"""Original architectural modules for five Caliche landmarks. No Blender dependency."""
import math
from geometry import *
from design import BUILDINGS,PARTS

def basic_building(name,lod,frontmat='Stucco',floor='FloorTile'):
 b=BUILDINGS[name];x0,x1,y0,y1=b['footprint'];h=b['height'];s=b['front'];fx=x1 if s>0 else x0;rear=x0 if s>0 else x1
 shell=Mesh('SM_Town_'+name+'_Shell');glass=Mesh('SM_Town_'+name+'_Glass');inside=Mesh('SM_Town_'+name+'_Interior');fixtures=Mesh('SM_Town_'+name+'_Fixtures')
 door,w,dh=b['door'];holes=[(door-w/2,door+w/2,0,dh)]+[(y-w/2,y+w/2,z0,z1)for y,w,z0,z1 in b['windows']]
 front_wall(shell,fx,y0,y1,h,holes,frontmat)
 shell.box('Rear_mineral_wall',(rear,(y0+y1)/2,h/2),(.22,y1-y0,h),frontmat)
 for y in [y0,y1]:shell.box('Side_mineral_wall',((x0+x1)/2,y,h/2),(x1-x0,.22,h),frontmat)
 shell.box('Deep_foundation',((x0+x1)/2,(y0+y1)/2,-.35),(x1-x0+.12,y1-y0+.12,.70),'Concrete')
 shell.box('Flat_roof_deck',((x0+x1)/2,(y0+y1)/2,h+.035),(x1-x0,y1-y0,.07),'DarkMetal')
 for y in [y0,y1]:shell.box('Parapet_return',((x0+x1)/2,y,h+.16),(x1-x0+.10,.25,.32),frontmat)
 for x in [rear,fx]:shell.box('Parapet_head',(x,(y0+y1)/2,h+.16),(.25,y1-y0+.1,.32),frontmat)
 inside.box('Finished_floor_authority',((x0+x1)/2,(y0+y1)/2,-.055),(x1-x0-.20,y1-y0-.20,.11),floor)
 inside.box('Real_acoustic_ceiling',((x0+x1)/2,(y0+y1)/2,h-.09),(x1-x0-.22,y1-y0-.22,.10),'Ceiling')
 for y,ww,z0,z1 in b['windows']:
  frame(shell,fx+s*.015,y,(z0+z1)/2,ww,z1-z0,lod,'Timber');pane(glass,fx+s*.035,y,(z0+z1)/2,ww,z1-z0,normal=s)
 frame(shell,fx+s*.02,door,dh/2,w,dh,lod,'DarkWood')
 # Fixed closed door; existing proxy/collision and absence of admission stay canonical.
 shell.box('Closed_door_lower_leaf',(fx+s*.01,door,.55),(.075,w-.05,1.10),'DarkWood')
 frame(shell,fx+s*.02,door,1.60,w-.18,.80,lod,'Timber',.10);pane(glass,fx+s*.04,door,1.60,w-.18,.80,normal=s)
 shell.box('Door_upper_rail',(fx,door,dh-.08),(.08,w,.16),'DarkWood')
 fixtures.tube('Brass_door_pull',(fx+s*.09,door-w*.32,.90),(fx+s*.09,door-w*.32,1.18),.018,'Metal',seg(lod)//2)
 shell.box('Doorhead_small_canopy',(fx+s*.36,door,dh+.24),(.95,w+.65,.15),frontmat)
 pendant(fixtures,fx+s*.29,door,dh+.12,lod)
 if lod<2:roofplant(shell,rear+s*2,(y0+y1)/2,h+.1,lod)
 return shell,glass,inside,fixtures

def sheet_stack(m,x,y,z,lod=0):
 for k in range([5,3,1][lod]):m.box('Loose_office_paper',(x+.005*(k%2),y,z+.0025+k*.003),(.25,.19,.003),'Paper')
def typewriter(m,x,y,z,lod):
 n=seg(lod);m.rounded_box('Typewriter_cast_base',(x,y,z+.08),(.43,.37,.16),.025,'OliveEnamel',2)
 m.rounded_box('Typewriter_key_slope',(x+.16,y,z+.07),(.20,.36,.09),.025,'DarkMetal',2)
 m.tube('Typewriter_rubber_platen',(x-.13,y-.245,z+.23),(x-.13,y+.245,z+.23),.035,'Rubber',n)
 for a in [-1,1]:m.lathe('Platen_turn_knob',(x-.13,y+a*.275,z+.23),[(0,.035),(.025,.035)],'DarkMetal',n,axis=(0,1,0))
 m.box('Paper_in_typewriter',(x-.15,y,z+.35),(.006,.28,.29),'Paper')
 if lod<2:
  for a in range(4):
   for k in range(10):m.lathe('Typewriter_round_key',(x+.09+a*.041,y-.15+k*.033,z+.13-a*.009),[(0,.012),(.012,.012)],'Ceramic',8)
 m.box('Typewriter_spacebar',(x+.285,y,z+.10),(.025,.27,.016),'Rubber')
def rotary_phone(m,x,y,z,lod):
 m.rounded_box('Rotary_telephone_body',(x,y,z+.07),(.25,.23,.14),.045,'Rubber',3)
 m.torus('Telephone_rotary_ring',(x+.04,y,z+.15),.061,.012,'Metal',seg(lod),5)
 for k in range(10 if lod<2 else 5):
  a=k*math.tau/10;m.lathe('Dial_finger_hole',(x+.04+.045*math.cos(a),y+.045*math.sin(a),z+.154),[(0,.009),(.003,.009)],'DarkMetal',6)
 m.tube('Telephone_handset_bridge',(x-.09,y-.16,z+.21),(x-.09,y+.16,z+.21),.030,'Rubber',seg(lod)//2)
 for yy in [y-.145,y+.145]:m.lathe('Handset_earpiece',(x-.09,yy,z+.15),[(0,.055),(.05,.049),(.065,.032)],'Rubber',seg(lod)//2)
 if lod==0:
  pts=[(x-.10+.018*math.cos(k*.9),y+.17+.016*math.sin(k*.9),z+.13-k*.005)for k in range(28)]
  for a,b in zip(pts,pts[1:]):m.tube('Coiled_telephone_cord',a,b,.004,'Rubber',4)
def sheriff(lod):
 s,g,i,f=basic_building('Sheriff',lod);n=seg(lod)
 s.panel('Caliche_sheriff_sign',(.135,.6,2.98),3.5,.62,0)
 # Recessed barred window and an interior across the whole open sight cone.
 for k in range(7):s.tube('Forged_window_security_bar',(.15,-3.05+k*.58,.78),(.15,-3.05+k*.58,2.41),.016,'DarkMetal',max(5,n//2))
 for z in [1.05,2.12]:s.tube('Horizontal_window_security_bar',(.15,-3.10,z),(.15,.50,z),.015,'DarkMetal',max(5,n//2))
 table(i,(-3.45,-1.25),(1.65,.90),.79);chair(i,-4.40,-1.25,lod,yaw=-math.pi/2);chair(i,-2.20,-1.25,lod,True,math.pi/2)
 typewriter(i,-3.30,-1.3,.79,lod);rotary_phone(i,-3.65,-.96,.79,lod);sheet_stack(i,-3.6,-1.55,.79,lod)
 for y in [-2.9,-2.1,2.9,3.7]:cabinet(i,-7.6,y,.70,.62,1.45,4,lod)
 # County map board, paper edges and physical colored pins (neutral original cartography).
 i.box('Oak_map_frame',(-6.35,-1.3,1.75),(.13,3.0,1.85),'DarkWood');i.panel('County_map_with_pins',(-6.275,-1.3,1.75),2.83,1.68,10)
 for y,z in [(-2.1,1.8),(-1.2,1.6),(-.5,2.0)]:i.tube('Map_notice_pin',(-6.265,y,z),(-6.22,y,z),.012,'CinemaRedGlow',6)
 # Reception counter and a passage to the records/break room behind a glazed partition.
 i.box('Reception_oak_counter',(-3.3,3.5,.48),(1.05,2.3,.96),'Timber');i.box('Reception_counter_cap',(-3.3,3.5,.99),(1.16,2.43,.06),'DarkWood')
 sheet_stack(i,-3.15,3.5,1.02,lod);i.box('Records_partition',(-7.1,0,1.45),(.14,4.1,2.9),'Stucco')
 i.box('Records_backroom_door_header',(-7.1,4.65,2.6),(.18,1.25,.35),'Timber')
 for y in [2.8,5.7]:i.box('Archive_shelves',(-9.35,y,1.1),(.48,1.0,2.2),'OliveEnamel')
 for y in [2.5,2.72,2.94,3.16,5.4,5.62,5.84]:
  for z in [.25,.83,1.42]:book(i,-9.05,y,z,.16,.26)
 tube_light(f,-3.6,-1.3,3.08,1.2);tube_light(f,-8.15,4.3,2.9,.9)
 # Desk fan has a real curved guard, stem and five blades, visible from the window.
 i.lathe('Desk_fan_base',(-7.25,-2.1,1.45),[(0,.14),(.035,.15),(.08,.07),(.26,.025)],'DarkMetal',n)
 for r in [.12,.20]:i.torus('Desk_fan_wire_guard',(-7.20,-2.1,1.86),r,.006,'Metal',n,4,axis=(1,0,0))
 for k in range(5):
  a=k*math.tau/5;ps=[(-7.18,-2.1+rr*math.cos(aa),1.86+rr*math.sin(aa))for rr,aa in [(.035,a),(.17,a+.18),(.18,a+.72),(.07,a+.62)]];i.tag='Original_fan_blade';i.face(ps,'OliveEnamel')
 return {m.name:m for m in [s,g,i,f]}

def washer(m,x,y,lod,dryer=False):
 n=seg(lod);h=1.65 if dryer else 1.04;rad=.29
 m.rounded_box('Stacked_dryer'if dryer else'Coin_washing_machine',(x,y,h/2),(.80,.77,h),.035,'CreamEnamel',2)
 front=x+.414;cz=.91 if dryer else.47
 m.torus('Chrome_drum_door',(front,y,cz),rad,.038,'Metal',n,max(4,n//4),axis=(1,0,0))
 m.lathe('Recessed_black_drum',(front-.035,y,cz),[(0,rad-.025),(.01,rad-.025)],'Rubber',n,axis=(1,0,0))
 m.torus('Glass_door_bevel',(front+.009,y,cz),rad-.055,.013,'Metal',n,4,axis=(1,0,0))
 m.lathe('Drum_window',(front+.012,y,cz),[(0,rad-.065),(.004,rad-.065)],'WasherGlass',n,axis=(1,0,0))
 m.box('Coin_control_panel',(front,y,h-.13),(.055,.68,.19),'Metal')
 for yy in [y-.20,y+.19]:m.lathe('Machine_rotary_selector',(front+.035,yy,h-.13),[(0,.033),(.025,.033)],'Rubber',n//2,axis=(1,0,0))
 m.box('Coin_slot',(front+.037,y+.05,h-.10),(.01,.006,.041),'DarkMetal')
 if lod<2:
  for k in range(9):m.box('Vent_slots',(front+.018,y-.23+k*.055,.08),(.012,.031,.011),'DarkMetal')
  m.box('Machine_service_panel',(front+.021,y,.20),(.013,.67,.10),'CreamEnamel')
def laundry(lod):
 s,g,i,f=basic_building('Laundry',lod,'Stucco');n=seg(lod)
 # Dado is below both window sills and split at the existing door.
 for a,b in [(-7,-4.95),(-3.85,7)]:s.box('Faded_blue_lower_dado',(.125,(a+b)/2,.29),(.022,b-a,.58),'BluePaint')
 s.panel('Laundromat_painted_sign',(.14,2,2.96),4.5,.67,1)
 for y in [-5.65,-4.6,-3.55,-2.5,-1.45,-.4,.65,1.7,2.75,3.8,4.85,5.9]:washer(i,-4.7,y,lod)
 for y in [-5,-3.9,-2.8]:washer(i,-8.7,y,lod,True)
 table(i,(-2.1,1.0),(1.2,2.3),.88,'CreamEnamel')
 for y in [-5.0,-3.9,4.6,5.7]:chair(i,-1.5,y,lod,True,math.pi/2)
 # Tubular wire laundry carts with open baskets and visible wheels, not opaque cubes.
 for x,y in [(-2.8,-1.6),(-7.3,3.8)]:
  for z in [.45,.85]:
   for a,b in [((x-.3,y-.23,z),(x+.3,y-.23,z)),((x+.3,y-.23,z),(x+.3,y+.23,z)),((x+.3,y+.23,z),(x-.3,y+.23,z)),((x-.3,y+.23,z),(x-.3,y-.23,z))]:i.tube('Wire_basket_rim',a,b,.012,'Metal',5)
  for k in range([9,6,4][lod]):
   xx=x-.29+k*.58/([9,6,4][lod]-1)
   for yy in [y-.23,y+.23]:i.tube('Cart_open_wire',(xx,yy,.45),(xx,yy,.85),.006,'Metal',4)
  for dx in [-.24,.24]:
   for dy in [-.19,.19]:
    i.tube('Cart_frame_leg',(x+dx,y+dy,.08),(x+dx,y+dy,.45),.012,'Metal',5);i.torus('Rubber_cart_wheel',(x+dx,y+dy,.08),.046,.018,'Rubber',8,4,axis=(1,0,0))
 i.box('Service_workbench',(-8.6,4.55,.87),(1.0,2.6,.07),'Timber')
 for yy in [3.5,5.6]:cabinet(i,-8.8,yy,.65,.85,.82,2,lod)
 i.panel('Laundry_price_notice',(-8.95,.4,1.70),.60,.74,9)
 for yy in [-.3,.05,.4,.75,1.1]:i.lathe('Detergent_bottle',(-8.3,yy,1.9),[(0,.048),(.23,.048),(.28,.022),(.33,.022)],'CreamEnamel',n//2)
 i.box('Detergent_wall_shelf',(-8.4,.4,1.88),(.45,2.1,.045),'Timber')
 for y in [-4,0,4]:tube_light(f,-3,y,3.07,1.5)
 return {m.name:m for m in [s,g,i,f]}

def sack(m,x,y,z,lod=0,yaw=0):
 # Bulging sewn textile sack, broad ends and asymmetric shoulder, not a pallet cube.
 n=[12,8,6][lod];rings=[]
 for zz,sx,sy in [(0,.25,.34),(.07,.31,.39),(.21,.33,.40),(.34,.27,.34),(.38,.22,.28)]:
  rings.append([(x+sx*math.cos(a),y+sy*math.sin(a),z+zz)for a in [k*math.tau/n+yaw for k in range(n)]])
 m.tag='Bulging_sewn_feed_sack';m.face(rings[0][::-1],'Burlap');m.face(rings[-1],'Burlap')
 for a,b in zip(rings,rings[1:]):
  for k in range(n):j=(k+1)%n;m.face([a[k],a[j],b[j],b[k]],'Burlap',smooth=True)
 if lod<2:m.tube('Feed_sack_stitched_seam',(x-.21,y,z+.39),(x+.21,y,z+.39),.007,'DarkWood',4)
def barrel(m,x,y,z,lod=0):
 m.lathe('Galvanized_stock_bin',(x,y,z),[(0,.28),(.05,.30),(.72,.30),(.75,.32),(.78,.32)],'Metal',seg(lod),caps=False)
 for zz in [.07,.67,.77]:m.torus('Stock_bin_rolled_rim',(x,y,z+zz),.304,.014,'Metal',seg(lod),4)
 m.lathe('Seed_visible_in_bin',(x,y,z+.71),[(0,.28),(.005,.28)],'Burlap',seg(lod))
def feed(lod):
 s,g,i,f=basic_building('Feed',lod,'Stucco','Timber');n=seg(lod)
 # Retail and warehouse share the dock datum. Neither creates new player admission.
 for k,p in enumerate(i.v):i.v[k]=(p[0],p[1],p[2]+.95 if p[2]<.1 else p[2])
 s.panel('Feed_seed_painted_fascia',(.14,0,4.20),6.7,1.1,2)
 s.box('Existing_loading_dock',(1.3,1,.34),(2.6,10,1.22),'Concrete')
 for y in [-3.9,5.9]:s.box('Timber_awning_post',(1.72,y,2.15),(.14,.14,4.3),'Timber')
 s.box('Awning_header',(1.72,1,3.97),(.18,10.1,.20),'DarkWood')
 # Thin corrugated metal awning, with actual raised transverse folds.
 count=[44,28,14][lod]
 for k in range(count):
  a=-4.1+k*10.2/count;b=-4.1+(k+1)*10.2/count
  s.tag='Folded_corrugated_tin_awning';points=[(-.15,a,4.33),(2.30,a,3.98),(2.30,b,3.98),(-.15,b,4.33)];s.face(points[::-1],'RustTin')
  if lod<2:s.tube('Corrugation_crown',(-.15,(a+b)/2,4.35),(2.30,(a+b)/2,4.0),.025,'RustTin',4)
 for y in [-11.8,11.8]:
  for x in [-14,-11,-8,-5,-2]:s.tube('Warehouse_rib',(x,y,.1),(x,y,5.15),.045,'RustTin',5)
 # Small door under loading aperture; display depth still visible to either side.
 for y in [-1.4,2.4]:s.box('Loading_oak_door_stile',(.10,y,2.3),(.12,.09,2.65),'DarkWood')
 s.box('Retail_door_bottom',(.10,.5,1.48),(.10,1.2,1.05),'Timber')
 pane(g,.17,.5,2.62,1.1,1.16)
 # Three aisles of stocks at shelf-level, and a clear central circulation strip.
 for x in [-12.5,-8.5,-3.8]:
  for y in [-8.4,8.3]:
   i.box('Warehouse_shelving_upright',(x,y,2.0),(.12,.12,2.1),'DarkWood')
  for z in [1.15,2.05,2.95]:i.box('Stock_shelf',(x,0,z),(.75,16.8,.09),'Timber')
  if lod<2:
   for y in [-7,-5,-3,1,3,5,7]:
    for z in [1.2,2.1]:
     if int(abs(y))%3:sack(i,x,y,z,lod,y*.07)
     else:barrel(i,x,y,z,lod)
 for x,y in [(-2.3,-6),(-2.3,-8.3),(-6.2,4.0)]:barrel(i,x,y,.95,lod)
 table(i,(-2.5,4.1),(1.1,3.3),1.91);i.rounded_box('Antique_cash_register',(-2.3,4.1,2.15),(.45,.39,.45),.035,'DarkMetal',2)
 if lod<2:
  for k in range(12):i.lathe('Register_round_key',(-2.07,3.95+(k%4)*.07,2.07+(k//4)*.05),[(0,.017),(.012,.017)],'Ceramic',6)
 # Back warehouse threshold, loose sacks on pallet beside the unchanged loading proxy.
 for a in [-.42,.42]:s.box('Dock_pallet_runner',(1.4+a,-2.4,1.035),(.10,1.2,.17),'Timber')
 for k in range(6):s.box('Dock_pallet_deck',(1.4,-2.95+k*.22,1.125),(1.2,.12,.045),'Timber')
 for k in range([7,5,3][lod]):sack(s,1.35+(k%2)*.06,-2.40+(k%2)*.15,1.16+(k//2)*.28,lod)
 # Real loops of horse tack on back/side wall and long-handled tools.
 for y in [-6,-4,-2,2,4,6]:
  i.torus('Hanging_leather_tack',(-15.75,y,2.45),.32,.022,'DarkWood',n,4,axis=(1,0,0))
 for y in [-9.5,-8.9,-8.3]:
  i.tube('Tool_wooden_handle',(-14.6,y,.97),(-14.7,y,2.65),.018,'Timber',6);i.box('Rake_tine_head',(-14.70,y,2.70),(.12,.42,.08),'DarkMetal')
 for x,y in [(-3,-4),(-3,4),(-9,0),(-14,0)]:pendant(f,x,y,4.5,lod)
 return {m.name:m for m in [s,g,i,f]}

def cinema(lod):
 s,g,i,f=basic_building('Cinema',lod,'PinkPlaster','Terrazzo');n=seg(lod)
 # Unequal Art-Deco pylons and stepped mineral parapet produce the photographed silhouette.
 for y,w,hh in [(-7,.70,6.5),(-4.5,.65,7.0),(0,1.3,7.6),(4.5,.65,7.0),(7,.70,6.5)]:
  s.box('Stepped_Art_Deco_fascia',(.07,y,hh/2),(.38,w,hh),'PinkPlaster');s.box('Deco_recessed_flute',(.275,y,4.8),(.022,.14,3.9),'Stucco')
 s.box('Marquee_real_volume',(1.25,0,3.55),(2.5,9.8,1.15),'DarkMetal')
 for z in [2.96,4.18]:s.box('Marquee_rolled_metal_trim',(1.25,0,z),(2.59,9.9,.12),'Metal')
 s.panel('Marquee_CLOSED',(2.552,0,3.55),9.6,.99,4)
 for y in [-4.91,4.91]:s.panel('Side_marquee_CLOSED',(1.25,y,3.55),2.42,.99,4,normal=(0,1 if y>0 else -1,0))
 for k in range(10 if lod<2 else 5):s.box('Physical_marquee_letter_groove',(2.562,0,3.08+k*.10),(.014,9.6,.013),'DarkMetal')
 s.rounded_box('PLAZA_blade_volume',(.48,0,6.1),(.9,.28,3.4),.12,'DarkWood',3)
 for y in [-.15,.15]:s.panel('PLAZA_blade_lettering',(.48,y,6.10),.78,3.21,3,normal=(0,1 if y>0 else -1,0))
 # Neon outline (its runtime power remains owned by the original marquee state).
 for y in [-.17,.17]:
  for x in [.08,.88]:f.tube('Blade_red_neon_edge',(x,y,4.55),(x,y,7.65),.014,'CinemaRedGlow',6)
 for k in range([22,14,7][lod]):
  y=-4.65+k*9.3/([22,14,7][lod]-1);f.lathe('Marquee_incandescent_bulb',(2.25,y,2.89),[(0,.035),(.05,.035),(.07,.023)],'WarmGlow',8)
 # Polygonal ticket booth, actual closed shutter and layered oak framing.
 booth=Mesh('TicketBooth');booth.box('Ticket_booth_oak_base',(.48,-.8,.63),(.95,1.4,1.26),'DarkWood')
 for yy in [-1.5,-.1]:booth.box('Ticket_booth_corner_post',(.95,yy,1.75),(.09,.09,2.15),'Timber')
 booth.box('Ticket_booth_header',(.96,-.8,2.55),(.12,1.55,.16),'Timber')
 for k in range([25,16,8][lod]):booth.box('Blind_lowered_metal_slat',(1.01,-.8,1.31+k*1.14/[25,16,8][lod]),(.022,1.28,.034),'Metal')
 booth.torus('Ticket_speaking_port',(1.033,-.8,1.70),.105,.015,'DarkMetal',n,5,axis=(1,0,0));s.merge(booth)
 s.panel('Ticket_booth_closed',(1.035,-.8,1.1),.50,.27,4)
 # Paired poster vitrines, independent depth, glass, cornice and warm hood.
 for y,cell in [(-7.0,11),(7.0,12)]:
  s.box('Poster_case_back',(.16,y,1.50),(.18,1.28,1.95),'DarkWood');s.panel('Original_fictional_film_poster',(.265,y,1.50),1.13,1.77,cell)
  pane(g,.285,y,1.50,1.15,1.79);frame(s,.27,y,1.50,1.2,1.85,lod,'Metal',.13)
 # Foyer only: no asset budget spent on an unused full auditorium.
 i.box('Lobby_rear_partition',(-5.6,0,1.75),(.18,19.6,3.5),'Stucco')
 for y in [-2.1,2.1]:
  i.box('Auditorium_closed_velvet_curtain',(-5.47,y,1.45),(.16,2.2,2.9),'Upholstery')
  if lod<2:
   for k in range(15):i.tube('Velvet_curtain_fold',(-5.33,y-1.05+k*.15,.08),(-5.33,y-1.05+k*.15,2.85),.034,'Upholstery',5)
 for y in [-6.9,6.9]:
  for x in [-1.6,-3.6]:i.panel('Framed_foyer_poster',(x,y,1.7),1.15,1.7,11 if x<-2 else 12,normal=(0,-1 if y>0 else 1,0))
 i.box('Concession_counter',(-3.15,7.9,.48),(3.9,.75,.96),'DarkWood');i.box('Concession_marble_top',(-3.15,7.9,.99),(4.05,.85,.075),'Terrazzo')
 i.box('Popcorn_machine_lower',(-3.5,7.85,1.12),(.76,.57,.19),'OliveEnamel')
 for dx in [-.36,.36]:
  for dy in [-.26,.26]:i.tube('Popcorn_machine_corner',(-3.5+dx,7.85+dy,1.20),(-3.5+dx,7.85+dy,1.86),.014,'Metal',6)
 i.box('Popcorn_machine_roof',(-3.5,7.85,1.91),(.84,.65,.13),'OliveEnamel')
 pane(g,-3.11,7.85,1.53,.53,.62)
 for k in range([18,10,4][lod]):i.lathe('Popcorn_in_glazed_cabinet',(-3.7+(k%4)*.13,7.65+(k//4)*.08,1.25),[(0,.04),(.045,.045)],'Ceramic',6)
 for x,y in [(-2.5,-5),(-2.5,4)]:pendant(f,x,y,3.8,lod)
 return {m.name:m for m in [s,g,i,f]}

def arch_window(shell,glass,at,width,bottom,shoulder,top,lod,normal=(0,-1,0)):
 # Pointed five-sided aperture: tracery/glass carry actual space, no room picture.
 right=unit(cross((0,0,1),normal));up=(0,0,1);cx,cy=at
 pts=[add((cx,cy,0),add(mul(right,x),mul(up,z)))for x,z in [(-width/2,bottom),(width/2,bottom),(width/2,shoulder),(0,top),(-width/2,shoulder)]]
 glass.tag='Pointed_church_glass';glass.face(pts,'Glass',[(0,0),(1,0),(1,.70),(.5,1),(0,.70)])
 for a,b in zip(pts,pts[1:]+pts[:1]):shell.tube('Pointed_timber_window_casing',a,b,.045,'Clapboard',6)
 shell.tube('Church_window_center_mullion',(cx,cy,bottom),(cx,cy,top-.05),.019,'Timber',5)
 shell.tube('Church_window_cross_rail',add((cx,cy,(bottom+shoulder)/2),mul(right,-width/2)),add((cx,cy,(bottom+shoulder)/2),mul(right,width/2)),.018,'Timber',5)
def church(lod):
 s=Mesh('SM_Town_Church_Shell');g=Mesh('SM_Town_Church_Glass');i=Mesh('SM_Town_Church_Interior');f=Mesh('SM_Town_Church_Fixtures');n=seg(lod)
 # Real hollow nave, gabled roof, original entrance and independent sacristy.
 front_wall(s,0,-4.5,4.5,4.25,[(-1.1,1.1,0,2.9),(-3.175,-2.625,2.45,3.95),(2.625,3.175,2.45,3.95)],'Clapboard')
 s.box('Rear_clapboard_wall',(14,0,2.1),(.20,9.0,4.2),'Clapboard')
 for yy in [-4.5,4.5]:
  q=Mesh('Side_clapboard');front_wall(q,0,0,14,4.2,[(x-.65,x+.65,1.25,3.75)for x in [3,7,11]],'Clapboard',.20);s.merge(q,(0,yy,0),-math.pi/2)
  for x in [3,7,11]:arch_window(s,g,(x,yy-.015 if yy<0 else yy+.015),1.3,1.25,3.10,3.75,lod,(0,-1 if yy<0 else 1,0))
  # Lapped siding is horizontal around the nave; keep clear actual window openings.
  if lod<2:
   for zz in [k*.18+.12 for k in range(23)]:
    for a,b in [(0,2.32),(3.68,6.32),(7.68,10.32),(11.68,14)] if 1.2<zz<3.8 else[(0,14)]:s.box('Individual_lapped_timber_siding',((a+b)/2,yy,zz),(b-a,.035,.035),'Clapboard')
 s.box('Church_raised_foundation',(7,0,-.28),(14.22,9.22,.56),'Concrete');i.box('Nave_timber_floor',(7,0,-.05),(13.8,8.8,.10),'Timber')
 # Gable triangular walls and two independently thick roof slopes.
 for x in [0,14]:
  s.tag='Gable_end_timber';ps=[(x,-4.5,4.2),(x,4.5,4.2),(x,0,6.4)];s.face(ps if x>0 else ps[::-1],'Clapboard')
 for a,b in [(-4.85,0),(0,4.85)]:
  za=6.4-abs(a)*2.2/4.5;zb=6.4-abs(b)*2.2/4.5;s.tag='Pitched_shingle_roof';s.face([(-.3,a,za),(14.3,a,za),(14.3,b,zb),(-.3,b,zb)],'RoofShingle')
  s.face([(-.3,b,zb-.12),(14.3,b,zb-.12),(14.3,a,za-.12),(-.3,a,za-.12)],'Clapboard')
 for x in [1.2,4.1,7,9.9,12.8]:
  i.tube('Nave_exposed_tie_beam',(x,-4.35,4.12),(x,4.35,4.12),.09,'Clapboard',4)
  for yy in [-4.35,4.35]:i.tube('Nave_pitched_rafter',(x,yy,4.15),(x,0,6.26),.075,'Clapboard',4)
 # Double panelled front door, fixed closed and fitted to the unchanged proxy.
 for yy in [-.55,.55]:
  s.box('Oak_church_door',(0,yy,1.42),(.10,1.08,2.84),'DarkWood')
  for dy in [-.24,.24]:
   for z in [.63,1.38,2.15]:s.box('Raised_oak_door_panel',(-.064,yy+dy,z),(.04,.40,.57),'Timber')
  f.tube('Church_brass_pull',(-.10,yy-(.38 if yy>0 else-.38),1.05),(-.10,yy-(.38 if yy>0 else-.38),1.35),.018,'Metal',6)
 for yy in [-2.9,2.9]:arch_window(s,g,(-.015,yy),.55,2.45,3.62,3.95,lod,(-1,0,0))
 # Covered entry porch within the existing stoop, with actual visible roof underside.
 s.box('Entry_porch_floor',(-.45,0,-.05),(.9,3.0,.10),'Concrete')
 for yy in [-1.39,1.39]:s.box('Porch_turned_post',(-.88,yy,1.7),(.10,.10,3.4),'Clapboard')
 s.box('Porch_lintel',(-.88,0,3.37),(.12,2.95,.16),'Clapboard')
 for a,b in [(-1.75,0),(0,1.75)]:
  za=4.15-abs(a)*.47;zb=4.15-abs(b)*.47;s.tag='Entry_pitched_roof';s.face([(-1.2,a,za),(.17,a,za),(.17,b,zb),(-1.2,b,zb)],'RoofShingle')
 # Open belfry, real small bell with hollow profile, shingled spire and timber cross.
 for x in [.65,2.15]:
  for y in [-.75,.75]:s.box('Open_belfry_corner_post',(x,y,6.55),(.13,.13,2.1),'Clapboard')
 s.box('Belfry_lower_plinth',(1.4,0,5.58),(1.7,1.7,.20),'Clapboard')
 for y in [-.83,.83]:s.box('Belfry_header',(1.4,y,7.58),(1.8,.13,.16),'Clapboard')
 for x in [.5,2.3]:s.box('Belfry_header',(x,0,7.58),(.13,1.8,.16),'Clapboard')
 s.lathe('Original_hollow_bronze_bell',(1.4,0,6.25),[(0,.36),(.08,.38),(.16,.29),(.45,.19),(.56,.12)],'Metal',n,caps=False)
 s.tube('Bell_clapper',(1.4,0,6.16),(1.4,0,6.7),.035,'DarkMetal',8)
 corners=[(.30,-1.1,7.65),(2.50,-1.1,7.65),(2.50,1.1,7.65),(.30,1.1,7.65)];apex=(1.4,0,8.90)
 for a,b in zip(corners,corners[1:]+corners[:1]):s.tag='Belfry_shingled_spire';s.face([a,b,apex],'RoofShingle')
 s.box('Belfry_cross_upright',(1.4,0,9.32),(.085,.085,.90),'DarkWood');s.box('Belfry_cross_arm',(1.4,0,9.46),(.085,.58,.085),'DarkWood')
 # Eight pairs of proper pews, shaped slab sides and inclined backs.
 for x in [2.3,3.5,4.7,5.9,7.1,8.3,9.5,10.7]:
  for y in [-2.25,2.25]:
   i.box('Pew_seat',(x,y,.46),(.52,2.4,.08),'DarkWood')
   i.box('Pew_back',(x-.22,y,.79),(.085,2.4,.65),'Timber')
   for yy in [y-1.23,y+1.23]:i.rounded_box('Pew_shaped_end',(x,yy,.46),(.65,.09,.92),.075,'Timber',2)
   if lod<2:i.box('Pew_hymnal_rack',(x-.30,y,.65),(.12,2.10,.13),'DarkWood')
 i.box('Altar_dais',(12.9,0,.12),(1.6,7.8,.24),'Timber');table(i,(13.15,0),(1.0,2.3),1.15,'DarkWood')
 i.box('Altar_hanging_linen',(12.61,0,.79),(.025,.75,.65),'LampFabric')
 i.box('Altar_cross_upright',(13.86,0,2.85),(.10,.10,1.6),'DarkWood');i.box('Altar_cross_arm',(13.86,0,3.10),(.10,.95,.10),'DarkWood')
 # Sacristy is an actual enclosed corner room with an open passage and shelves.
 front_wall(i,11.6,2.9,4.35,2.5,[(3.05,3.85,0,2.1)],'Stucco',.12)
 cabinet(i,13.55,3.75,.50,.75,1.65,3,lod,'DarkWood');chair(i,12.65,3.6,lod)
 for x,y in [(3,-2.1),(3,2.1),(7,-2.1),(7,2.1),(11,0)]:pendant(f,x,y,4.4,lod)
 return {m.name:m for m in [s,g,i,f]}

def build(lod):
 out={}
 for fn in [sheriff,laundry,feed,cinema,church]:out.update(fn(lod))
 return out
