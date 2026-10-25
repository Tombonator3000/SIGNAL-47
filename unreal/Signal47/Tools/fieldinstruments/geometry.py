"""Original detailed 1947 instruments still in field service in 1986.
Every component is real geometry; flat artwork is confined to printed surfaces.
No dependency on Blender, downloads, licensed assets or archived web authoring.
"""
import math
from shapes import Mesh,add,sub,mul,cross,unit
from design import PARTS,ATLAS

def build(lod=0):
 n=[32,16,10][lod];small=[12,8,6][lod];detail=[4,2,1][lod]
 out={key:Mesh(key) for key in PARTS}
 def get(s):return out['SM_FieldKit_'+s]
 def pipe(m,name,pts,r=.004,mat='Rubber',segments=None):
  for i,(a,b) in enumerate(zip(pts,pts[1:])):m.tube(name+'_'+str(i),a,b,r,mat,segments or small)
 def bolts(m,name,coords,axis=(0,-1,0),radius=.0035):
  if lod==2:return
  for i,p in enumerate(coords):m.tube(name+str(i),p,add(p,mul(axis,.003)),radius,'Metal',6)
 def plate(m,name,p,w,h,cell,normal=(0,-1,0)):
  m.panel(name,p,w,h,ATLAS[cell],normal)
 def circle_ticks(m,name,center,r,axis,steps,width=.0016):
  if lod==2:return
  a=unit(axis);u=unit(cross(a,(0,0,1) if abs(a[2])<.9 else (0,1,0)));v=cross(a,u)
  for j in range(steps):
   angle=j*math.tau/steps;direction=add(mul(u,math.cos(angle)),mul(v,math.sin(angle)))
   m.tube(name+str(j),add(center,mul(direction,r-.012 if j%5==0 else r-.005)),add(center,mul(direction,r)),width,'DarkMetal',4)
 def coil(m,name,center,axis,r,length,turns=12,wire=.0025):
  axis=unit(axis);u=unit(cross(axis,(0,0,1) if abs(axis[2])<.9 else (0,1,0)));v=cross(axis,u)
  count=turns*[12,8,6][lod];pts=[]
  for i in range(count+1):
   t=i/count;a=math.tau*turns*t
   pts.append(add(add(center,mul(axis,length*t)),add(mul(u,r*math.cos(a)),mul(v,r*math.sin(a)))))
  pipe(m,name,pts,wire,'Rubber',4 if lod else 6)

 # Real tapered pier retains the same top elevation and sight line as E09A.
 m=get('Transit');m.tag='Tapered_concrete_pier'
 lower=[(-.45,-.45,-.25),(.45,-.45,-.25),(.45,.45,-.25),(-.45,.45,-.25)]
 upper=[(-.26,-.26,1.05),(.26,-.26,1.05),(.26,.26,1.05),(-.26,.26,1.05)]
 m.face(lower[::-1],'Concrete');m.face(upper,'Concrete')
 for i in range(4):j=(i+1)%4;m.face([lower[i],lower[j],upper[j],upper[i]],'Concrete')
 m.box('Pier_cap_shadow',(0,0,1.047),(.525,.525,.012),'DarkMetal')
 m.box('Pier_cap_stone',(0,0,1.061),(.54,.54,.018),'Concrete')
 m.tube('Mounting_disc',(0,0,1.07),(0,0,1.086),.13,'Brass',n)
 for i in range(3):
  a=i*math.tau/3+.3;x,y=.077*math.cos(a),.077*math.sin(a)
  m.tube('Levelling_screw_stem',(x,y,1.08),(x,y,1.14),.007,'Metal',small)
  m.lathe('Knurled_levelling_wheel',(x,y,1.103),[(0,.021),(.006,.024),(.016,.024),(.022,.020)],'Brass',n//2)
  if lod==0:
   for j in range(12):
    t=j*math.tau/12;m.tube('Wheel_knurl',(x+.024*math.cos(t),y+.024*math.sin(t),1.111),(x+.024*math.cos(t),y+.024*math.sin(t),1.123),.0018,'DarkMetal',4)
 m.lathe('Horizontal_circle',(0,0,1.14),[(0,.09),(.01,.094),(.018,.094),(.025,.077),(.039,.077),(.045,.091),(.053,.091)],'Brass',n)
 circle_ticks(m,'Horizontal_degree_',(0,0,1.194),.085,(0,0,1),72 if lod==0 else 36)
 m.tube('Alidade',(0,0,1.193),(0,0,1.216),.067,'DarkMetal',n)
 # Sight axis matches the original A footing; the telescope tips down 0.125 rad.
 optical=unit((.70710678,.70710678,-.125));trunnion=unit((-.70710678,.70710678,0));center=(0,0,1.372)
 for side in [-1,1]:
  p=mul(trunnion,side*.065)
  m.box('A_frame_upright',(p[0],p[1],1.289),(.025,.025,.17),'DarkMetal')
  m.tube('Trunnion_hub',add(center,mul(trunnion,side*.048)),add(center,mul(trunnion,side*.088)),.018,'Brass',small)
 m.tube('Trunnion_axle',add(center,mul(trunnion,-.090)),add(center,mul(trunnion,.090)),.008,'Metal',small)
 m.lathe('Telescope_barrel',add(center,mul(optical,-.125)),[(0,.023),(.022,.026),(.20,.026),(.23,.029),(.28,.029),(.289,.031)],'DarkMetal',n,axis=optical)
 for t,r in [(-.145,.013),(-.122,.022),(.09,.029),(.15,.033)]:
  m.tube('Brass_optical_band',add(center,mul(optical,t)),add(center,mul(optical,t+.009)),r,'Brass',n)
 m.tube('Objective_lens',add(center,mul(optical,.166)),add(center,mul(optical,.169)),.027,'Glass',n)
 m.tube('Eyepiece_lens',add(center,mul(optical,-.151)),add(center,mul(optical,-.149)),.009,'Glass',small)
 vcenter=add(center,mul(trunnion,.091))
 m.torus('Vertical_circle',vcenter,.055,.007,'Brass',n,6,axis=trunnion)
 circle_ticks(m,'Vertical_degree_',add(vcenter,mul(trunnion,.008)),.052,trunnion,48 if lod==0 else 24)
 m.tube('Focus_knob_spindle',add(center,mul(optical,-.03)),add(add(center,mul(optical,-.03)),mul(trunnion,-.062)),.007,'Metal',small)
 m.tube('Focus_knob',add(add(center,mul(optical,-.03)),mul(trunnion,-.062)),add(add(center,mul(optical,-.03)),mul(trunnion,-.078)),.016,'DarkMetal',small)
 m.tube('Bubble_level_tube',(-.045,.055,1.229),(.045,.055,1.229),.009,'Brass',small)
 m.box('Level_sight_window',(0,.055,1.236),(.048,.009,.004),'Glass')
 m.box('Record_box_back',(-.322,0,.72),(.026,.27,.33),'DarkMetal')
 plate(m,'Record_card',(-.338,0,.72),.24,.29,'transit_card',(-1,0,0))
 for y in [-.145,.145]:m.box('Record_box_vertical_frame',(-.345,y,.72),(.013,.014,.35),'Metal')
 for z in [.548,.892]:m.box('Record_box_horizontal_frame',(-.345,0,z),(.013,.30,.014),'Metal')
 bolts(m,'Record_frame_screw',[(-.355,y,z) for y in [-.137,.137] for z in [.557,.883]],(-1,0,0))
 plate(m,'Survey_inventory',(.005,-.276,.90),.17,.075,'serial')

 m=get('LampStand')
 for i in range(3):
  a=i*math.tau/3+.5;foot=(math.cos(a)*.40,math.sin(a)*.40,-.15)
  m.tube('Tripod_leg',(0,0,1.0),foot,.014,'DarkMetal',small)
  m.tube('Tripod_cross_link',(0,0,.52),(foot[0]*.62,foot[1]*.62,.29),.006,'Metal',small)
  m.box('Tripod_foot',(foot[0],foot[1],-.139),(.08,.055,.022),'Rubber')
 m.lathe('Mast_locking_collar',(0,0,.965),[(0,.035),(.025,.038),(.055,.038),(.075,.029)],'DarkMetal',small)
 m.tube('Telescopic_mast',(0,0,1.0),(0,0,1.80),.016,'Metal',small)
 m.tube('Mast_thumbscrew',(.025,0,1.02),(.078,0,1.02),.006,'Brass',small)
 m.box('Battery_base',(-.32,-.36,-.125),(.34,.22,.045),'DarkMetal')
 m.rounded_box('Lead_acid_case',(-.32,-.36,-.015),(.32,.20,.20),.016,'Rubber',detail)
 m.box('Battery_top',(-.32,-.36,.094),(.33,.21,.025),'DarkMetal')
 for i in range(6):m.tube('Battery_cell_cap',(-.44+i*.048,-.36,.107),(-.44+i*.048,-.36,.119),.012,'Rubber',small)
 for x in [-.44,-.20]:m.tube('Battery_post',(x,-.29,.11),(x,-.29,.14),.008,'Metal',small)
 pipe(m,'Battery_lead',[(-.20,-.29,.14),(-.17,-.20,.16),(-.05,-.05,.6),(-.02,-.02,1.7)],.005,'Rubber')
 plate(m,'Lamp_safety',(0,-.033,1.50),.065,.09,'safety')

 # Head and blackout hood must remain independent local-pivot meshes.
 m=get('LampHead');ax=(0,-1,0)
 m.lathe('Reflector_shell',(0,.105,0),[(0,.081),(.035,.085),(.10,.113),(.19,.133),(.205,.133)],'OlivePaint',n,axis=ax,caps=False)
 m.lathe('Aluminium_reflector',(0,.09,0),[(0,.075),(.03,.077),(.10,.105),(.185,.127)],'Metal',n,axis=ax,caps=False)
 m.tube('Lamp_back_cover',(0,.106,0),(0,.120,0),.083,'DarkMetal',n)
 m.torus('Front_rim',(0,-.102,0),.129,.006,'Metal',n,6,axis=ax)
 for x in [-.14,.14]:m.tube('Head_yoke',(x,0,0),(x*.15,0,-.12),.010,'DarkMetal',small)
 for i in range(5 if lod<2 else 2):
  y=.035+i*.015;m.torus('Cooling_fin',(0,y,0),.09,.004,'DarkMetal',n//2,4,axis=ax)
 m.tube('Tin_hood_hinge',(-.036,-.105,.135),(.036,-.105,.135),.008,'Metal',small)
 bolts(m,'Head_mount_screw',[(-.146,0,0),(.146,0,0)],(1,0,0),.006)
 m=get('LampBulb');m.tube('Separate_switchable_lens',(0,-.098,0),(0,-.103,0),.123,'FieldBulb',n)
 m=get('LampHood');m.tube('Opaque_blackout_lid',(0,-.005,-.135),(0,-.050,-.135),.142,'Metal',n)
 m.torus('Rolled_lid_rim',(0,-.051,-.135),.139,.0045,'Metal',n,6,axis=ax)
 m.box('Hinge_leaf',(0,0,-.006),(.062,.016,.025),'DarkMetal')
 m.box('Lid_latch',(0,-.054,-.266),(.028,.018,.045),'DarkMetal')
 for x in [-.022,.022]:m.tube('Hinge_rivet',(x,-.012,-.011),(x,-.017,-.011),.0035,'Metal',6)

 m=get('Receiver')
 m.rounded_box('Crinkle_enamel_case',(0,0,.149),(.52,.34,.28),.014,'OlivePaint',detail)
 m.rounded_box('Front_panel',(0,-.177,.154),(.54,.018,.30),.008,'DarkMetal',detail)
 # Metal bezel stands proud of the separate stateful printed dial.
 for x in [-.104,.224]:m.box('Dial_vertical_bezel',(x,-.189,.205),(.012,.010,.098),'Metal')
 for z in [.157,.253]:m.box('Dial_horizontal_bezel',(.06,-.189,z),(.34,.010,.009),'Metal')
 m.box('Signal_meter_bezel',(-.17,-.190,.205),(.084,.012,.088),'Metal')
 # Aperture infill behind the lens is black and physically recessed.
 m.box('Dial_recess',(.06,-.186,.205),(.314,.004,.086),'Rubber')
 for i in range(5):
  x=-.20+i*.10;r=.024 if i in [1,3] else .020
  m.tube('Control_shaft',(x,-.186,.08),(x,-.213,.08),.006,'Metal',small)
  m.lathe('Ribbed_Bakelite_control',(x,-.204,.08),[(0,r*.87),(.005,r),(.025,r),(.029,r*.9)],'Rubber',small,axis=(0,-1,0))
  m.box('Control_index',(x,-.236,.087),(.003,.002,.012),'Paper')
  if lod==0:
   for j in range(10):
    a=j*math.tau/10;m.tube('Knob_ridge',(x+r*math.cos(a),-.210,.08+r*math.sin(a)),(x+r*math.cos(a),-.226,.08+r*math.sin(a)),.0018,'DarkMetal',4)
 for x in [-.24,.24]:
  pipe(m,'Front_handle',[(x,-.19,.245),(x,-.224,.245),(x,-.224,.280),(x,-.19,.280)],.005,'Metal')
 for x in [-.205,.205]:
  for y in [-.12,.12]:m.tube('Case_rubber_foot',(x,y,-.005),(x,y,.01),.018,'Rubber',small)
 for i in range([15,9,5][lod]):m.box('Top_vent_slot',(-.21+i*.42/max(1,[15,9,5][lod]-1),.015,.292),(.008,.20,.002),'Rubber')
 bolts(m,'Panel_screw',[(x,-.189,z) for x in [-.253,.253] for z in [.025,.28]])
 plate(m,'Manufacturer_plate',(0,-.191,.127),.13,.026,'receiver_plate')
 # External speaker has real ribs and separate recessed cone.
 m.rounded_box('Separate_speaker',(-.44,.02,.126),(.24,.18,.24),.012,'OlivePaint',detail)
 m.tube('Speaker_cone',(-.44,-.075,.126),(-.44,-.079,.126),.087,'Rubber',n)
 m.torus('Speaker_surround',(-.44,-.080,.126),.078,.006,'DarkMetal',n,6,axis=(0,-1,0))
 for i in range(7):m.box('Speaker_grille_rib',(-.44,-.086,.054+i*.024),(.18,.005,.0035),'Metal')
 # Headband is an actual arc lying on the bench, not a torus solid across the opening.
 headband=[]
 for i in range(17 if lod==0 else 9):
  a=i*math.pi/(16 if lod==0 else 8);headband.append((.32+.075*math.cos(a),-.25+.075*math.sin(a),.018))
 pipe(m,'Headphone_headband',headband,.007,'Rubber')
 for x in [.245,.395]:
  m.tube('Headphone_earcup',(x,-.25,.009),(x,-.25,.032),.031,'Rubber',small)
  m.torus('Headphone_earpad',(x,-.25,.032),.024,.005,'Cloth',small,6)
 coil(m,'Headphone_coiled_lead',(.46,-.24,.022),(0,1,0),.009,.15,8,.0018)
 pipe(m,'Speaker_cable',[(-.32,.03,.09),(-.28,.08,.035),(-.23,.12,.035),(-.22,.15,.12)],.0035,'Rubber')
 m=get('ReceiverDial');plate(m,'Backlit_slide_rule',(.06,-.196,.205),.30,.075,'receiver_dial')
 plate(m,'Backlit_S_meter',(-.17,-.198,.205),.068,.070,'receiver_meter')
 # The black needle is on the receiver body so the complete dial can be dimmed separately.
 get('Receiver').tube('S_meter_pointer',(-.17,-.201,.180),(-.186,-.201,.221),.0014,'Rubber',4)

 m=get('FieldPhone')
 m.rounded_box('Oak_phone_box',(0,0,0),(.16,.26,.44),.012,'Wood',detail)
 m.box('Oak_front_door',(.085,0,0),(.019,.239,.413),'Wood')
 for y in [-.11,.11]:m.box('Raised_oak_door_stile',(.100,y,0),(.012,.014,.42),'Wood')
 for z in [-.195,.195]:m.box('Raised_oak_door_rail',(.100,0,z),(.012,.23,.014),'Wood')
 for y in [-.12,.12]:
  for z in [-.22,.22]:m.tube('Wall_mounting_ear',(-.083,y,z-.015),(-.083,y,z+.015),.013,'DarkMetal',small)
 m.box('Oak_lower_shelf',(.03,0,-.26),(.20,.30,.025),'Wood')
 m.box('Phone_shelf_brace',(.01,0,-.235),(.03,.30,.03),'Wood')
 for y in [-.06,.06]:
  m.lathe('Nickel_gong',(.083,y,.17),[(0,.017),(.01,.033),(.026,.043),(.042,.046),(.049,.044)],'Metal',n//2,axis=(1,0,0))
  m.tube('Gong_screw',(.13,y,.17),(.14,y,.17),.004,'Brass',6)
 m.tube('Bell_striker_stem',(.117,0,.117),(.12,0,.186),.0035,'DarkMetal',small)
 m.tube('Bell_striker_head',(.12,-.017,.185),(.12,.017,.185),.007,'Brass',small)
 m.tube('Speaking_horn_neck',(.080,0,.02),(.20,0,.04),.012,'Rubber',small)
 m.lathe('Speaking_horn',(.19,0,.04),[(0,.013),(.012,.016),(.045,.030),(.060,.040)],'Rubber',n//2,axis=(1,0,0),caps=False)
 m.tube('Speaking_horn_recess',(.232,0,.04),(.234,0,.04),.028,'DarkMetal',small)
 pipe(m,'Magneto_crank',[(.08,.14,0),(.12,.19,0),(.12,.19,-.08)],.008,'Metal')
 m.tube('Crank_grip',(.12,.175,-.09),(.12,.211,-.09),.014,'Rubber',small)
 plate(m,'Field_phone_plaque',(.114,0,-.107),.15,.085,'telephone',(1,0,0))
 bolts(m,'Front_panel_screw',[(.116,y,z) for y in [-.102,.102] for z in [-.182,.182]],(1,0,0))
 pipe(m,'Field_line',[(.005,0,.22),(.005,0,.69),(.01,.05,.71)],.004,'Rubber')
 coil(m,'Receiver_cord',(.10,-.15,-.14),(0,0,-1),.009,.17,11,.002)
 m=get('Handset')
 m.lathe('Bakelite_earpiece',(0,0,-.158),[(0,.027),(.007,.033),(.028,.031),(.043,.023),(.140,.022),(.158,.019)],'Rubber',small)
 m.tube('Earpiece_cap',(0,0,-.156),(0,0,-.160),.027,'DarkMetal',small)
 if lod<2:
  for x,y in [(-.008,0),(0,0),(.008,0),(0,-.008),(0,.008)]:m.tube('Ear_cap_aperture',(x,y,-.161),(x,y,-.162),.0016,'Rubber',4)
 m.box('Receiver_hook',(0,.01,0),(.03,.04,.02),'Metal')

 m=get('TimingBench')
 # Top dimensions preserve the drawer's independent 27 cm travel towards the player.
 for j in range(5):m.box('Oak_top_plank_'+str(j),(0,-.288+j*.144,.775),(3.0,.141,.05),'Wood')
 for x in [-1.39,1.39]:
  for y in [-.30,.30]:
   m.box('Bench_leg',(x,y,.366),(.065,.065,.732),'Wood')
   m.box('Iron_foot_shoe',(x,y,.036),(.072,.072,.055),'DarkMetal')
   m.tube('Adjustable_level_foot',(x,y,-.012),(x,y,.014),.018,'Metal',small)
 for y in [-.325,.325]:
  # The front apron is interrupted at the animated drawer's exact opening.
  if y<0:
   for lo,hi in [(-1.50,.77),(1.23,1.50)]:m.box('Front_apron_clear_drawer',((lo+hi)/2,y,.704),(hi-lo,.032,.100),'Wood')
  else:m.box('Back_apron',(0,y,.704),(3,.032,.10),'Wood')
 for x in [-1.41,1.41]:m.box('End_apron',(x,0,.704),(.032,.65,.10),'Wood')
 m.box('Lower_shelf',(0,0,.22),(2.88,.60,.03),'Wood')
 for x in [-1.37,1.37]:m.box('Shelf_crossrail',(x,0,.192),(.055,.62,.035),'Wood')
 # Drawer outer cavity is dark, with no fixed front or floor blocking the moving drawer.
 m.box('Drawer_cavity_back',(1,.305,.69),(.48,.025,.12),'DarkMetal')
 for x in [.765,1.235]:m.box('Drawer_guide_rail',(x,.02,.682),(.018,.57,.025),'DarkMetal')
 for i in range(4):
  m.box('Spare_valve_carton',(-1.13+i*.28,.03,.313),(.19,.23,.15),'Paper')
  plate(m,'Spare_valve_label',(-1.13+i*.28,-.087,.313),.15,.10,'spares')
 m.torus('Spare_wire_coil',(.70,0,.264),.12,.018,'Rubber',n//2,6)
 if lod==0:
  for x in [-1.38,1.38]:
   for y in [-.30,.30]:m.tube('Countersunk_top_screw',(x,y,.800),(x,y,.802),.0035,'DarkMetal',6)
 m=get('TimingLog')
 m.box('Cloth_ledger_cover',(0,0,.010),(.47,.33,.020),'Cloth')
 for side in [-1,1]:
  m.box('Paper_text_block',(side*.114,0,.022),(.220,.313,.013),'Paper')
  for j in range(5 if lod==0 else 2):m.box('Visible_page_edge',(side*.114,-.157,.018+j*.002),(.22,.0012,.0007),'DarkMetal')
 plate(m,'Open_1947_record',(0,0,.031),.445,.307,'timing_log',(0,0,1))
 m.box('Ledger_spine_shadow',(0,0,.033),(.0035,.31,.002),'Cloth')
 m.tube('Pencil',(.06,-.10,.038),(.20,-.04,.038),.0038,'Wood',6)
 m.tube('Pencil_graphite',(.20,-.04,.038),(.213,-.034,.038),.0018,'DarkMetal',6)

 m=get('Diagram')
 m.box('Arrangement_backboard',(0,.008,0),(.68,.02,.33),'DarkMetal')
 plate(m,'Original_1947_arrangement',(0,-.008,0),.60,.2625,'diagram')
 for x in [-.33,.33]:m.box('Oak_diagram_vertical',(x,-.010,0),(.04,.03,.37),'Wood')
 for z in [-.165,.165]:m.box('Oak_diagram_horizontal',(0,-.010,z),(.70,.03,.04),'Wood')
 bolts(m,'Frame_corner_pin',[(x,-.027,z) for x in [-.33,.33] for z in [-.165,.165]],radius=.0022)
 # No opaque pane in front of the print; glazing specular is represented by a narrow edge only.
 m.box('Glazing_bottom_edge',(0,-.010,-.13),(.59,.0012,.002),'Glass')

 m=get('EvidenceProps')
 m.tube('Film_can_body',(-.03,0,0),(-.03,0,.052),.031,'Metal',n//2)
 m.tube('Film_can_lid',(-.03,0,.052),(-.03,0,.061),.033,'DarkMetal',n//2)
 plate(m,'Film_can_label',(-.03,-.031,.031),.047,.036,'film')
 m.box('Evidence_sleeve',(.045,.035,.003),(.105,.15,.006),'Paper')
 plate(m,'Evidence_sleeve_print',(.045,.035,.0065),.094,.134,'evidence',(0,0,1))
 # Cassette body has twin real reel wells and a recessed tape window.
 m.rounded_box('Compact_cassette',(.045,-.11,.008),(.100,.064,.014),.004,'Rubber',detail)
 m.box('Cassette_label_recess',(.045,-.11,.016),(.087,.048,.0015),'Paper')
 plate(m,'Recorded_cassette_label',(.045,-.109,.0175),.082,.044,'cassette',(0,0,1))
 for x in [.023,.067]:
  m.tube('Cassette_reel_well',(x,-.11,.018),(x,-.11,.020),.009,'DarkMetal',small)
  m.torus('Cassette_reel_hub',(x,-.11,.020),.0055,.0014,'Paper',small,4)
 m.box('Tape_window',(.045,-.107,.020),(.020,.010,.0015),'Film')
 for x in [.005,.085]:
  for y in [-.132,-.086]:m.tube('Cassette_screw',(x,y,.015),(x,y,.017),.0018,'Metal',6)
 return out
