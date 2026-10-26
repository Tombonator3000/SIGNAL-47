"""Original Mesa Diner shell/interior, tightly fitted to audited legacy plan.
Metres, BlenderZup, FRONT+X. No render/native work at import.
"""
import math
from geometry import Mesh,add,sub,mul
ROOT=[7979.95,4.309006389843889,1999.95]
DOOR_HINGE=[1.9,-.58,0]
PARTS={
 'SM_Mesa_Shell':{'offset':[0,0,0],'role':'weathered stucco, tiled front, deep windows, cornice, roof plant'},
 'SM_Mesa_Glass':{'offset':[0,0,0],'role':'separate thin front glazing'},
 'SM_Mesa_Door':{'offset':DOOR_HINGE,'role':'hinged door normalised at original interaction pivot'},
 'SM_Mesa_Interior':{'offset':[0,0,0],'role':'checker floor, panelled walls, ceiling and public room finishes'},
 'SM_Mesa_Booths':{'offset':[0,0,0],'role':'four original padded booth/table settings'},
 'SM_Mesa_Counter':{'offset':[0,0,0],'role':'teal counter, seven chrome stools and original appliances'},
 'SM_Mesa_Kitchen':{'offset':[0,0,0],'role':'service hatch, cooking range, hood, racks and menu'},
 'SM_Mesa_Fixtures':{'offset':[0,0,0],'role':'pendant/fluorescent fixtures, wall fittings and blinds'},
}
def nseg(lod):return [20,12,8][lod]
def wall(m,name,x0,x1,y0,y1,z0,z1,mat):m.box(name,((x0+x1)/2,(y0+y1)/2,(z0+z1)/2),(x1-x0,y1-y0,z1-z0),mat)
def ringrect(m,name,x0,x1,y0,y1,z,r,mat,lod):
 for a,b in [((x0,y0,z),(x1,y0,z)),((x1,y0,z),(x1,y1,z)),((x1,y1,z),(x0,y1,z)),((x0,y1,z),(x0,y0,z))]:m.tube(name,a,b,r,mat,max(4,nseg(lod)//3))
def cup(m,x,y,z,lod=0):
 n=nseg(lod);m.lathe('Heavy_diner_cup',(x,y,z),[(0,.044),(.012,.053),(.105,.060),(.119,.060)],'Ceramic',n)
 m.lathe('Cup_coffee',(x,y,z+.116),[(0,.049),(.001,.049)],'Coffee',n)
 m.torus('Cup_handle',(x+.066,y,z+.065),.033,.009,'Ceramic',max(8,n//2),5,axis=(0,1,0))
def settings(m,x,y,z,lod):
 cup(m,x-.25,y+.10,z,lod);n=max(8,nseg(lod)//2)
 m.box('Napkin_stack',(x+.29,y+.13,z+.016),(.15,.16,.032),'Ceramic')
 m.rounded_box('Chrome_napkin_holder',(x+.33,y-.12,z+.108),(.115,.16,.205),.018,'Chrome',[3,2,1][lod])
 for dx in [-.06,.06]:
  m.lathe('Salt_pepper_shaker',(x+.1+dx,y-.14,z),[(0,.024),(.075,.027),(.09,.020)],'Ceramic',n)
  m.lathe('Shaker_cap',(x+.1+dx,y-.14,z+.09),[(0,.022),(.014,.022)],'Chrome',n)
def shell(lod):
 m=Mesh('SM_Mesa_Shell');n=nseg(lod)
 # Full-thickness shell with true door/window apertures, no hidden frontbox.
 for name,x0,x1,y0,y1 in [('Rear',-8,-7.8,-9,9),('South',-7.8,2,-9,-8.8),('North',-7.8,2,8.8,9)]:
  wall(m,name+'_stucco',x0,x1,y0,y1,0,3.18,'Stucco')
  wall(m,name+'_faded_red_band',x0-.006,x1+.006,y0-.006,y1+.006,2.82,3.02,'PaintRed')
 # Clipping pier is at originalBy−1.35; keep solid backing in place.
 wall(m,'Clipping_pier',1.8,2,-2.15,-.6,0,3.18,'Stucco')
 for y0,y1 in [(-9,-2.15),(.6,9)]:
  wall(m,'Tiled_window_base',1.8,2,y0,y1,0,.85,'TealTile')
  wall(m,'Front_stucco_fascia',1.8,2,y0,y1,2.58,3.18,'Stucco')
  wall(m,'Faded_front_red_stripe',2.002,2.017,y0,y1,2.85,3.03,'PaintRed')
  for z in [.84,2.58]:m.box('Aluminium_window_rail',(2.006,(y0+y1)/2,z),(.22,y1-y0,.056),'Chrome')
  y=y0+.045
  while y<y1:
   m.box('Deep_window_mullion',(1.991,y,1.71),(.17,.06,1.76),'Chrome');y+=2.05
  m.box('Window_end_mullion',(1.991,y1-.033,1.71),(.17,.06,1.76),'Chrome')
  m.box('Weathered_window_sill',(2.055,(y0+y1)/2,.815),(.34,y1-y0,.075),'Concrete')
 wall(m,'Entrance_header',1.8,2,-.6,.6,2.26,3.18,'Stucco')
 for y in [-.62,.62]:m.box('Entrance_surround',(1.96,y,1.15),(.22,.065,2.30),'Chrome')
 m.box('Threshold',(1.96,0,.018),(.38,1.2,.036),'Chrome')
 m.box('Facade_cornice',(2.035,0,3.21),(.28,18.17,.18),'Stucco')
 m.box('Flat_insulated_roof',(-3,0,3.3),(10.12,18.10,.20),'Concrete')
 for y in [-9.025,9.025]:m.box('Folded_roof_edge',(-3,y,3.42),(10.16,.07,.055),'Chrome')
 m.box('Rear_roof_edge',(-8.065,0,3.42),(.07,18.17,.055),'Chrome')
 # Rooftop equipment: rounded spun vents, seam-lined HVAC and duct.
 for x,y in [(-5,-5.4),(-5,5.6)]:
  m.lathe('Roof_exhaust_curb',(x,y,3.4),[(0,.32),(.18,.32),(.20,.46),(.30,.46),(.34,.36)],'Chrome',n)
  m.lathe('Roof_exhaust_cap',(x,y,3.74),[(0,.36),(.04,.40),(.07,.38)],'DarkMetal',n)
 m.box('HVAC_concrete_curb',(-5.7,0,3.50),(1.65,1.62,.20),'Concrete')
 m.rounded_box('HVAC_weathered_case',(-5.7,0,3.90),(1.48,1.45,.62),.05,'Chrome',[3,2,1][lod])
 for k in range([13,8,5][lod]):m.box('HVAC_louvre',(-4.945,-.61+k*1.22/([13,8,5][lod]-1),3.9),(.025,.026,.40),'DarkMetal')
 m.box('HVAC_duct',(-6.7,.0,3.63),(.68,.60,.32),'Chrome')
 # No random chips geometry: sparse purposeful sill cracks and plumbing only.
 for y in [-8.6,8.6]:
  m.tube('Rear_downpipe',(-8.07,y,.18),(-8.07,y,3.28),.045,'Chrome',max(6,n//2))
  for z in [.4,1.8,2.9]:m.box('Downpipe_clip',(-8.07,y,z),(.12,.13,.035),'DarkMetal')
 return m

def glass(lod):
 m=Mesh('SM_Mesa_Glass')
 for a,b in [(-8.94,-2.19),(.65,8.94)]:
  m.tag='Thin_front_glazing';m.face([(1.987,a,.9),(1.987,b,.9),(1.987,b,2.54),(1.987,a,2.54)],'WindowGlass')
 return m

def door(lod):
 m=Mesh('SM_Mesa_Door');n=nseg(lod)
 for y in [0,1.16]:m.box('Door_stile',(0,y,1.12),(.075,.064,2.24),'Chrome')
 for z in [.045,.77,2.21]:m.box('Door_rail',(0,.58,z),(.075,1.16,.075),'Chrome')
 m.box('Door_teal_kickplate',(0,.58,.40),(.046,1.07,.64),'TealPaint')
 m.tag='Door_glazing';m.face([(.004,.05,.82),(.004,1.11,.82),(.004,1.11,2.17),(.004,.05,2.17)],'WindowGlass')
 for z in [.25,1.15,2.0]:m.tube('Door_hinge',(0,0,z-.06),(0,0,z+.06),.025,'Chrome',max(6,n//2))
 for x in [-.066,.066]:
  m.tube('Pushbar',(x,.23,1.03),(x,.93,1.03),.021,'Chrome',max(6,n//2))
  for y in [.23,.93]:m.tube('Pushbar_mount',(0,y,1.03),(x,y,1.03),.018,'Chrome',6)
 m.panel('Push_lettering',(.044,.85,1.16),.22,.10,6,normal=(1,0,0))
 # Permanently open outward: no new game mechanic or blocked doorway.
 angle=math.radians(-105);co,si=math.cos(angle),math.sin(angle)
 m.v=[(x*co-y*si,x*si+y*co,z) for x,y,z in m.v]
 return m

def interior(lod):
 m=Mesh('SM_Mesa_Interior')
 m.tag='Actual_checker_floor';m.face([(-7.79,-8.79,.001),(1.79,-8.79,.001),(1.79,8.79,.001),(-7.79,8.79,.001)],'CheckerFloor',[(x/1.10,y/1.10) for x,y in [(-7.79,-8.79),(1.79,-8.79),(1.79,8.79),(-7.79,8.79)]])
 # Public walls face inward; half-height teal wainscot with real trim.
 for y in [-8.785,8.785]:
  m.box('Interior_wall_cream',(-1.8,y,1.91),(7.8,.024,2.24),'WallCream')
  m.box('Interior_teal_wainscot',(-1.8,y,.48),(7.8,.028,.96),'TealTile')
  m.box('Wall_chrome_dado',(-1.8,y*.998,.98),(7.8,.035,.032),'Chrome')
  m.box('Dark_floor_skirting',(-1.8,y*.995,.07),(7.8,.04,.14),'DarkMetal')
 for x in [-5.67,1.75]:m.box('Long_floor_skirting',(x,0,.07),(.04,17.6,.14),'DarkMetal')
 # Suspended acoustic ceiling on a physical grid, glass remains below it.
 m.box('Acoustic_ceiling',(-3,0,3.105),(9.6,17.6,.035),'Ceiling')
 for y in range(-8,9,2):m.box('Ceiling_grid_cross',(-3,y,3.077),(9.6,.018,.025),'Chrome')
 for x in [-7.2,-6,-4.8,-3.6,-2.4,-1.2,0,1.2]:m.box('Ceiling_grid_long',(x,0,3.077),(.018,17.6,.025),'Chrome')
 # Closed restroom doors remain background; don't introduce extra walkspace.
 for x in [-4.5,-2.8]:
  m.rounded_box('Closed_restroom_door',(x,-8.745,1.1),(1.03,.055,2.2),.015,'TealPaint',[2,1,1][lod])
  m.panel('Restroom_label',(x,-8.710,1.78),.73,.18,2,normal=(0,1,0))
  m.tube('Restroom_handle',(x+.34,-8.68,1.02),(x+.23,-8.68,1.02),.017,'Chrome',6)
 # Bulletin panels on end wall, away from game-owned story photos/phone.
 for x in [-3.7,-2.1]:
  m.box('Notice_frame',(x,8.73,1.72),(1.24,.05,.89),'Wood')
  m.panel('Community_notice',(x,8.695,1.72),1.10,.76,7,normal=(0,-1,0))
 return m

def booths(lod):
 m=Mesh('SM_Mesa_Booths');n=nseg(lod);seg=[4,2,1][lod]
 for y in [6.1,3.05,-3.05,-6.1]:
  m.rounded_box('Formica_booth_table',(.7,y,.765),(1.65,.76,.075),.10,'Formica',seg)
  ringrect(m,'Table_bullnose_chrome',-.10,1.50,y-.33,y+.33,.776,.018,'Chrome',lod)
  m.lathe('Table_pedestal',(.7,y,0),[(.015,.25),(.045,.27),(.10,.10),(.68,.055),(.72,.11)],'Chrome',n)
  for side in [-1,1]:
   sy=y+side*.93;back=sy+side*.21
   m.rounded_box('Booth_wood_plinth',(.7,sy,.19),(1.80,.48,.38),.035,'Wood',seg)
   m.rounded_box('Padded_vinyl_seat',(.7,sy,.475),(1.85,.49,.15),.08,'Vinyl',seg)
   m.rounded_box('Padded_vinyl_back',(.7,back,.875),(1.85,.16,.75),.07,'Vinyl',seg)
   m.rounded_box('Booth_crown_roll',(.7,back,1.245),(1.87,.19,.075),.08,'Vinyl',seg)
   # Chrome piping and shallow padded seam divisions give real silhouette.
   for x in [-.19,1.59]:m.tube('Booth_edge_piping',(x,back-side*.086,.55),(x,back-side*.086,1.22),.010,'Chrome',max(4,n//4))
   if lod<2:
    for x in [.24,.70,1.16]:m.tube('Upholstery_seam',(x,back-side*.083,.59),(x,back-side*.083,1.18),.0035,'PaintRed',4)
  settings(m,.7,y,.803,lod)
 return m

def counter(lod):
 m=Mesh('SM_Mesa_Counter');n=nseg(lod);seg=[4,2,1][lod]
 m.rounded_box('Teal_counter_cabinet',(-4.5,0,.46),(.58,10.8,.92),.04,'TealTile',seg)
 m.rounded_box('Formica_countertop',(-4.5,0,.97),(.60,10.95,.06),.045,'Formica',seg)
 m.tube('Counter_chrome_bullnose',(-4.181,-5.45,.963),(-4.181,5.45,.963),.025,'Chrome',max(6,n//2))
 m.tube('Counter_footrail',(-3.99,-5.4,.19),(-3.99,5.4,.19),.023,'Chrome',max(6,n//2))
 for y in [-4.8,-3.2,-1.6,0,1.6,3.2,4.8]:
  m.lathe('Stool_flared_base',(-3.4,y,0),[(.015,.225),(.035,.245),(.065,.21),(.12,.09),(.61,.046),(.65,.115)],'Chrome',n)
  m.lathe('Stool_chrome_rim',(-3.4,y,0),[(.66,.235),(.69,.267),(.71,.269)],'Chrome',n)
  m.lathe('Stool_padded_vinyl',(-3.4,y,0),[(.71,.25),(.75,.266),(.79,.245)],'Vinyl',n)
  if lod<2:m.torus('Stool_seat_piping',(-3.4,y,.766),.254,.008,'PaintRed',n,5)
 # Machine stays behind the game-owned reachable coffee pot (B−4.3,+1.15).
 m.rounded_box('Coffee_brewer',(-4.65,1.25,1.22),(.33,.47,.44),.025,'Chrome',seg)
 m.box('Brewer_front',(-4.468,1.25,1.24),(.025,.37,.32),'DarkMetal')
 m.box('Brewer_canopy',(-4.40,1.25,1.43),(.23,.48,.05),'Rubber')
 m.lathe('Warmer_plate',(-4.34,1.15,1.005),[(0,.13),(.022,.13)],'DarkMetal',n)
 for y in [-5,-3.5,4.2]:settings(m,-4.48,y,1.002,lod)
 m.rounded_box('Pastry_display_base',(-4.50,-4.5,1.045),(.49,1.0,.085),.035,'Chrome',seg)
 for y in [-4.95,-4.05]:m.box('Pastry_display_end',(-4.50,y,1.22),(.44,.025,.30),'Chrome')
 m.box('Pastry_display_cover',(-4.50,-4.5,1.39),(.45,.91,.025),'WindowGlass')
 for k in range(4):m.lathe('Pie_on_display',(-4.48,-4.80+k*.20,1.10),[(0,.075),(.035,.075),(.05,.060)],'Formica',max(8,n//2))
 return m

def kitchen(lod):
 m=Mesh('SM_Mesa_Kitchen');n=nseg(lod)
 for a,b in [(-8.8,1.2),(3.6,8.8)]:
  wall(m,'Kitchen_partition',-5.9,-5.7,a,b,0,3.12,'WallCream')
  wall(m,'Partition_teal_base',-5.695,-5.68,a,b,0,.97,'TealTile')
 wall(m,'Hatch_base',-5.9,-5.7,1.2,3.6,0,1.01,'TealTile')
 wall(m,'Hatch_header',-5.9,-5.7,1.2,3.6,2.24,3.12,'WallCream')
 m.box('Hatch_chrome_shelf',(-5.63,2.4,1.055),(.5,2.6,.09),'Chrome')
 m.panel('Period_menu_board',(-5.675,2.4,2.69),1.60,.80,0,normal=(1,0,0))
 m.box('Menu_frame',(-5.715,2.4,2.69),(.07,1.72,.90),'Wood')
 # Through real hatch: lipped range, dark hotplate circles, racks and hood.
 m.box('Kitchen_workbench',(-7.05,2.4,.48),(1.30,2.40,.94),'Chrome')
 m.box('Range_dark_top',(-7.05,2.4,.98),(1.24,2.32,.035),'DarkMetal')
 for x in [-7.35,-6.8]:
  for y in [1.65,2.25,2.85,3.35]:m.lathe('Range_burner',(x,y,1.0),[(0,.13),(.025,.13)],'DarkMetal',max(8,n//2))
 for y in [1.6,2.0,2.4,2.8,3.2]:m.lathe('Range_knob',(-6.385,y,.84),[(0,.038),(.028,.038)],'Rubber',max(6,n//2),axis=(1,0,0))
 m.box('Vent_hood_upper',(-7.0,2.4,2.71),(1.35,2.70,.40),'Chrome')
 m.tag='Sloped_stainless_hood'
 for ps in [[(-7.675,1.05,2.51),(-6.325,1.05,2.51),(-6.05,.9,2.20),(-7.75,.9,2.20)],[(-6.325,1.05,2.51),(-6.325,3.75,2.51),(-6.05,3.9,2.20),(-6.05,.9,2.20)],[(-6.325,3.75,2.51),(-7.675,3.75,2.51),(-7.75,3.9,2.20),(-6.05,3.9,2.20)]]:m.face(ps,'Chrome')
 m.box('Hood_dark_filters',(-6.75,2.4,2.205),(1.35,2.62,.025),'DarkMetal')
 for y in [1.35,3.45]:
  m.lathe('Kitchen_saucepan',(-6.8,y,1.04),[(0,.12),(.16,.14)],'Chrome',n)
  m.tube('Pan_handle',(-6.8,y,1.17),(-6.5,y,1.17),.020,'DarkMetal',max(6,n//2))
 for z in [1.42,1.86]:
  m.box('Service_plate_shelf',(-6.2,2.4,z),(.35,2.3,.035),'Chrome')
  for y in [1.7,2.45,3.1]:
   for k in range([4,2,1][lod]):m.lathe('Stacked_plates',(-6.2,y,z+.025+k*.025),[(0,.12),(.012,.15),(.021,.145)],'Ceramic',n)
 for y in [1.27,3.53]:m.tube('Plate_shelf_support',(-6.2,y,1.03),(-6.2,y,2.09),.018,'Chrome',6)
 # Other kitchen mass stays wholly behind original collision partition.
 m.rounded_box('Refrigerator',(-7.18,-4.9,1.07),(1.10,1.35,2.10),.055,'Chrome',[3,2,1][lod])
 for z in [.67,1.47]:m.box('Fridge_handle',(-6.58,-4.95,z),(.055,.43,.035),'DarkMetal')
 return m

def fixtures(lod):
 m=Mesh('SM_Mesa_Fixtures');n=nseg(lod)
 # Venetian blinds remain high enough to read the illuminated interior.
 for a,b in [(-8.92,-2.20),(.66,8.92)]:
  m.box('Blind_header',(1.77,(a+b)/2,2.49),(.12,b-a,.09),'WallCream')
  for k in range([9,6,4][lod]):
   z=2.44-k*.035;m.box('Angled_blind_slat',(1.735,(a+b)/2,z),(.11,b-a-.03,.012),'Wood')
  for y in [a+.22,b-.22]:m.tube('Blind_pull_cord',(1.71,y,2.5),(1.71,y,1.87),.0035,'WallCream',4)
 # Five warm shallow glass pendants, curved lampshade silhouette.
 for y in [-6.1,-3.05,0,3.05,6.1]:
  m.tube('Pendant_drop',(.65,y,2.49),(.65,y,3.07),.009,'DarkMetal',max(6,n//3))
  m.lathe('Pendant_brass_cap',(.65,y,2.44),[(0,.09),(.07,.08),(.10,.025)],'Chrome',n)
  m.lathe('Pendant_opal_shade',(.65,y,2.25),[(0,.17),(.045,.23),(.13,.24),(.18,.17),(.20,.09)],'PendantGlow',n)
 for y in [-5,0,5]:
  m.box('Fluorescent_steel_fixture',(-2,y,3.045),(.59,2.75,.085),'Chrome')
  for x in [-2.13,-1.87]:m.tube('Fluorescent_tube',(x,y-1.22,2.991),(x,y+1.22,2.991),.025,'Fluorescent',max(6,n//2))
 # Clock body and physically separate hands, facing+Y from southwall.
 m.lathe('Wall_clock_case',(-1.7,-8.70,2.38),[(0,.25),(.045,.25)],'DarkMetal',n,axis=(0,1,0))
 m.lathe('Wall_clock_face',(-1.7,-8.65,2.38),[(0,.224),(.002,.224)],'Ceramic',n,axis=(0,1,0))
 for i in range(12):
  a=i*math.pi/6;x=-1.7+math.sin(a)*.19;z=2.38+math.cos(a)*.19
  m.tube('Clock_hour_mark',(x,-8.643,z),(x-math.sin(a)*.025,-8.643,z-math.cos(a)*.025),.006,'DarkMetal',4)
 m.tube('Clock_minute_hand',(-1.7,-8.635,2.38),(-1.7,-8.635,2.56),.007,'DarkMetal',5)
 m.tube('Clock_hour_hand',(-1.7,-8.633,2.38),(-1.59,-8.633,2.38),.010,'DarkMetal',5)
 # Small decorative framed menus, no duplicated game-owned photo/clipping.
 m.box('Daily_special_frame',(-4.4,8.72,2.18),(1.22,.06,.69),'Wood')
 m.panel('Daily_special',(-4.4,8.682,2.18),1.10,.57,5,normal=(0,-1,0))
 # Original payphone base, with space for the preserved animated handset.
 m.rounded_box('Payphone_metal_housing',(.4,-8.66,1.44),(.64,.22,.87),.045,'Chrome',[3,2,1][lod])
 m.box('Payphone_black_face',(.4,-8.529,1.43),(.53,.035,.70),'Rubber')
 for row in range(4):
  for col in range(3):m.rounded_box('Payphone_key',(.48+col*.05,-8.503,1.48-row*.057),(.035,.022,.030),.005,'Chrome',1)
 m.box('Payphone_coin_slot',(.59,-8.49,1.62),(.135,.025,.018),'Chrome')
 m.box('Payphone_return',(.58,-8.487,1.12),(.15,.045,.12),'Chrome')
 m.box('Coin_return_hollow',(.58,-8.46,1.13),(.11,.008,.07),'DarkMetal')
 m.panel('Telephone_header',(.4,-8.515,2.01),.66,.125,4,normal=(0,1,0))
 # Cord ends below the dynamic handset. No duplicate handset or storypaper.
 for i in range([24,16,8][lod]):
  count=[24,16,8][lod]
  a=i/count*math.pi*12;b=(i+1)/count*math.pi*12
  p=(.22+.015*math.sin(a),-8.43+.015*math.cos(a),1.28-i/count*.31)
  q=(.22+.015*math.sin(b),-8.43+.015*math.cos(b),1.28-(i+1)/count*.31)
  m.tube('Payphone_coiled_cord',p,q,.005,'Rubber',4)
 m.tube('Phone_cord_tail',(.22,-8.415,.97),(.45,-8.59,1.02),.006,'Rubber',5)
 # Rounded jukebox arch sits on its original static footprint, front−Y.
 cx=-1.1;cy=8.25;steps=[16,10,6][lod]
 outline=[(cx-.45,0),(cx+.45,0),(cx+.45,1.20)]
 for k in range(1,steps+1):
  a=k/steps*math.pi;outline.append((cx+.45*math.cos(a),1.20+.45*math.sin(a)))
 front=[(x,cy-.32,z) for x,z in outline];back=[(x,cy+.32,z) for x,z in outline]
 m.tag='Arched_wood_jukebox';m.face(front,'Wood');m.face(back[::-1],'Wood')
 for i in range(len(front)):
  j=(i+1)%len(front);m.face([front[j],front[i],back[i],back[j]],'Wood')
 for i in range(steps):
  a=i/steps*math.pi;b=(i+1)/steps*math.pi
  m.tube('Jukebox_opal_arch',(cx+.395*math.cos(a),cy-.341,1.20+.395*math.sin(a)),(cx+.395*math.cos(b),cy-.341,1.20+.395*math.sin(b)),.035,'PendantGlow',max(5,n//3))
 for x in [cx-.395,cx+.395]:m.tube('Jukebox_opal_upright',(x,cy-.341,.20),(x,cy-.341,1.20),.035,'PendantGlow',max(5,n//3))
 m.box('Jukebox_speaker_cloth',(cx,cy-.335,.55),(.60,.02,.61),'DarkMetal')
 for k in range([10,7,4][lod]):m.box('Jukebox_chrome_grille',(cx,cy-.355,.28+k*.053),(.59,.025,.012),'Chrome')
 m.panel('Jukebox_selection_card',(cx,cy-.345,1.20),.57,.28,3,normal=(0,-1,0))
 for k in range(6):m.box('Jukebox_selection_button',(cx-.25+k*.10,cy-.365,.99),(.06,.025,.035),'Ceramic')
 return m

def build(lod=0):
 return {m.name:m for m in [shell(lod),glass(lod),door(lod),interior(lod),booths(lod),counter(lod),kitchen(lod),fixtures(lod)]}
