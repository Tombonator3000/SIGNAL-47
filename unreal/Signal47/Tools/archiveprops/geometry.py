"""Own geometry; read-only reuse of the project's original metre mesh primitives.

Authoring helpers take Three (x,y-up,z). Export coordinates are Blender
(x,-z,y), exactly the existing FBX import contract, without an extra axis bake.
"""
from pathlib import Path
import importlib.util, math
_p=Path(__file__).resolve().parent.parent/'substation'/'meshkit.py'
_s=importlib.util.spec_from_file_location('s47_archive_original_primitives',_p)
base=importlib.util.module_from_spec(_s);_s.loader.exec_module(base)
T=lambda p:(p[0],-p[2],p[1])
class Mesh(base.Mesh):
 def face(self,ps,mat,uv=None,smooth=False):
  super().face(ps,mat,uv,smooth);self.mat[-1]=self.mat[-1].replace('M_Substation_','M_ArchiveProps_',1)
 def b(self,name,at,size,mat):self.box(name,T(at),(size[0],size[2],size[1]),mat)
 def rb(self,name,at,size,radius,mat,lod):self.rounded_box(name,T(at),(size[0],size[2],size[1]),radius,mat,[4,2,1][lod])
 def rod(self,name,a,b,r,mat,lod):self.tube(name,T(a),T(b),r,mat,[12,8,6][lod])
 def print(self,name,at,w,h,cell,normal=(1,0,0)):
  # 4x4 atlas with generous gutters, fixed physical label dimensions.
  self.tag=name;n=base.unit(T(normal));right=base.unit(base.cross((0,0,1),n));up=base.cross(n,right)
  ps=[base.add(T(at),base.add(base.mul(right,x*w/2),base.mul(up,y*h/2)))for x,y in [(-1,-1),(1,-1),(1,1),(-1,1)]]
  col,row=cell%4,cell//4;u0,u1=(col+.04)/4,(col+.96)/4;v0,v1=1-(row+.96)/4,1-(row+.04)/4
  self.face(ps,'PrintAtlas',[(u0,v0),(u1,v0),(u1,v1),(u0,v1)])
 def topprint(self,name,at,w,d,cell):
  self.tag=name;cx,cy,cz=T(at);ps=[(cx-w/2,cy-d/2,cz),(cx+w/2,cy-d/2,cz),(cx+w/2,cy+d/2,cz),(cx-w/2,cy+d/2,cz)]
  col,row=cell%4,cell//4;u0,u1=(col+.04)/4,(col+.96)/4;v0,v1=1-(row+.96)/4,1-(row+.04)/4
  self.face(ps,'PrintAtlas',[(u0,v0),(u1,v0),(u1,v1),(u0,v1)])

def carton(m,at,size,lod,label=1):
 x,y,z=at;w,h,d=size
 # Closed archive cartons are deliberately separate from the story's falling box.
 m.rb('Kraft_archive_carton',(x,y+h/2,z),(w,h,d),.006,'Kraft',lod)
 m.b('Overhanging_card_lid',(x,y+h+.005,z),(w+.014,.018,d+.014),'KraftEdge')
 m.b('Lid_wrapped_fold',(x+w/2+.006,y+h-.012,z),(.009,.035,d+.012),'KraftEdge')
 m.print('Archive_carton_index',(x+w/2+.011,y+h*.49,z),min(d*.67,.22),min(h*.43,.10),label)
 if lod<2:
  # Recessed hand-slot, not a reflective or unlit floating rectangle.
  m.b('Handslot_dark_recess',(x+w/2+.007,y+h-.052,z),(.008,.018,.065),'Rubber')
  for dz in [-.038,.038]:m.b('Handslot_reinforced_corner',(x+w/2+.012,y+h-.052,z+dz),(.004,.024,.012),'KraftEdge')

def shelving(lod):
 m=Mesh('SM_ArchiveProps_ShelvingStorage')
 for z in [-1.5,0,1.5]:
  for x in [-.212,.212]:
   m.b('Folded_L_section_upright',(x,1.15,z),(.035,2.30,.035),'SteelGreen')
   m.b('Upright_return_flange',(x+.02,1.15,z+.014),(.018,2.30,.018),'SteelGreen')
   m.b('Rack_floor_foot',(x,.019,z),(.09,.038,.10),'DarkSteel')
   if lod==0:
    for y in [.25,.75,1.25,1.75,2.15]:m.rod('Rack_round_fixing',(x+.025,y,z-.026),(x+.025,y,z-.036),.007,'Metal',lod)
 for y in [.12,.62,1.12,1.62,2.12]:
  m.b('Folded_sheet_shelf',(0,y,0),(.45,.022,3),'SteelGreen')
  for x in [-.213,.213]:m.b('Shelf_folded_stiffening_edge',(x,y-.019,0),(.02,.038,3),'SteelGreen')
 if lod<2:
  for z0,z1 in [(-1.48,-.02),(.02,1.48)]:
   m.rod('Back_cross_brace',(-.225,.15,z0),(-.225,2.12,z1),.008,'DarkSteel',lod)
   m.rod('Back_cross_brace',(-.225,.15,z1),(-.225,2.12,z0),.008,'DarkSteel',lod)
 for row,y in enumerate([.135,.635,1.135,1.635]):
  for col,z in enumerate([-1.18,-.76,-.34,.08,.50,.92,1.30]):
   # STATION 01 and the hidden fire drawer need a real clear bay at z=-.05.
   if row==2 and abs(z+.05)<.32:continue
   h=.26+((row*7+col)%3)*.026;carton(m,(-.03,y,z),(.34,h,.32 if col<6 else .22),lod,1+((row+col)%3))
 m.print('Rack_inventory_plate',(.246,2.245,-1.12),.43,.095,4)
 return m

def filing(lod):
 m=Mesh('SM_ArchiveProps_FilingCabinets')
 for idx,x in enumerate([-.54,0,.54]):
  m.rb('Rolled_steel_cabinet',(x,.66,0),(.50,1.32,.62),.018,'SteelGreen',lod)
  m.b('Recessed_kick_base',(x,.05,0),(.46,.10,.58),'DarkSteel')
  for row in range(4):
   h=.2+row*.32;out=.13 if idx==1 and row==1 else 0
   m.rb('Drawer_with_pressed_edge',(x,h,-.319-out),(.448,.272,.025),.006,'SteelOlive',lod)
   if out:m.b('Open_drawer_tray',(x,h-.11,-.25-out/2),(.42,.018,.26),'DarkSteel')
   m.print('Drawer_typed_index',(x,h+.09,-.335-out),.10,.046,5,normal=(0,0,-1))
   m.rod('Chrome_cabinet_pull',(x-.067,h+.018,-.353-out),(x+.067,h+.018,-.353-out),.009,'Metal',lod)
   for dx in [-.065,.065]:m.b('Pull_standoff',(x+dx,h+.018,-.34-out),(.018,.018,.027),'Metal')
   if lod==0:m.rod('Cabinet_lock_cylinder',(x+.15,h+.082,-.334-out),(x+.15,h+.082,-.345-out),.008,'Metal',lod)
  if lod<2:carton(m,(x,1.325,0),(.39,.25,.44),lod,2)
 return m

def cardindex(lod):
 m=Mesh('SM_ArchiveProps_CardIndex')
 m.rb('Index_cabinet_shell',(0,.525,0),(.5,1.05,.9),.018,'SteelOlive',lod)
 m.b('Index_kick_base',(-.015,.04,0),(.46,.08,.86),'DarkSteel')
 for row in range(4):
  for col,z in enumerate([-.29,0,.29]):
   y=.18+row*.24;opened=row==2 and col==1;front=-.44 if opened else -.263
   m.rb('Pressed_card_drawer',(front,y,z),(.025,.205,.254),.005,'SteelGreen',lod)
   m.print('Maintenance_drawer_index',(front-.014,y+.047,z),.082,.037,6,normal=(-1,0,0))
   m.rod('Index_label_pull',(front-.036,y-.014,z-.047),(front-.036,y-.014,z+.047),.007,'Metal',lod)
   if opened:
    m.b('Open_index_tray',(-.34,y-.08,z),(.23,.014,.23),'DarkSteel')
    for i in range([10,6,3][lod]):m.b('Standing_filing_card',(-.42+i*.019,y-.01,z),(.003,.16,.21),'Paper')
 m.b('Index_top_overhang',(0,1.064,0),(.53,.028,.93),'SteelOlive')
 # Physical wall-mounted title plate, clear of the cabinet top, at the old anchor.
 m.b('Lineage_title_plate',(.241,1.30,0),(.008,.18,.50),'BeigeEnamel')
 m.print('Reference_lineage_title',(.236,1.30,0),.48,.16,7,normal=(-1,0,0))
 return m

def bookcase(lod):
 m=Mesh('SM_ArchiveProps_Bookcase');w,d,h=1.90,.36,.90
 for x in [-w/2+.02,w/2-.02]:m.b('Walnut_bookcase_side',(x,h/2,0),(.04,h,d),'Walnut')
 for y in [.035,.43,.88]:m.b('Walnut_fixed_shelf',(0,y,0),(w,.04,d),'Walnut')
 m.b('Hardboard_back',(0,h/2,.17),(w-.05,h-.05,.02),'Walnut')
 for i,x in enumerate([-.85,-.75,-.65,-.55,-.45,-.35,.40,.50,.60,.70,.80,.90]):
  m.rb('Cloth_bound_service_volume',(x,1.05,0),(.073,.3,.25),.004,'BookBlue' if i%2==0 else 'BookBrown',lod)
  for y in [.95,1.16]:m.b('Gold_spine_rule',(x,y,-.128),(.053,.005,.003),'Brass')
  if lod<2:m.print('Binder_spine_index',(x,1.055,-.129),.046,.11,8,normal=(0,0,-1))
 for i in range([9,6,3][lod]):
  x=-.73+i*.16;m.b('Stored_maintenance_volume',(x,.22,0),(.13,.31,.25),'BookBrown'if i%2 else'BookBlue')
 return m

def table(lod):
 m=Mesh('SM_ArchiveProps_WorkTable');m.rb('Laminated_work_table',(0,.745,0),(1.7,.05,.85),.018,'Walnut',lod)
 for x in [-.80,.80]:
  for z in [-.38,.38]:
   m.b('Square_tubular_leg',(x,.36,z),(.05,.72,.05),'Metal')
   m.b('Rubber_table_foot',(x,.013,z),(.058,.026,.058),'Rubber')
 for z in [-.34,.34]:m.b('Table_apron_beam',(0,.66,z),(1.62,.055,.025),'DarkSteel')
 return m

def fieldbox(lod):
 m=Mesh('SM_ArchiveProps_FieldBox');carton(m,(0,0,0),(.38,.28,.34),lod,0)
 # Separate folded lid seam and edge scuffs retain silhouette even at low LOD.
 for z in [-.17,.17]:m.b('Box_folded_corner',(.191,.135,z),(.004,.25,.008),'KraftEdge')
 return m

def drawer(lod):
 m=Mesh('SM_ArchiveProps_FireDrawer')
 m.rb('Fire_drawer_front',(-.007,-.035,0),(.014,.11,.27),.004,'Metal',lod)
 m.b('Fire_drawer_bottom',(-.157,-.088,0),(.286,.008,.25),'DarkSteel')
 for z in [-.122,.122]:m.b('Rolled_tray_side',(-.157,-.035,z),(.286,.11,.006),'Metal')
 m.b('Tray_back',(-.297,-.016,0),(.006,.15,.25),'DarkSteel')
 m.rod('Drawer_handle',( .023,-.012,-.052),(.023,-.012,.052),.008,'Metal',lod)
 for z in [-.051,.051]:m.b('Handle_standoff',(.012,-.012,z),(.019,.016,.016),'Metal')
 m.print('Uncatalogued_drawer_label',(.001,-.06,.086),.062,.028,9)
 return m

def reel(lod):
 m=Mesh('SM_ArchiveProps_Reel047Box');m.rb('Grey_fibreboard_reel_box',(0,.095,0),(.032,.19,.19),.005,'GreyBoard',lod)
 m.print('Reel_047_typed_label',(.017,.095,0),.18,.18,10)
 for z in [-.09,.09]:m.b('Reel_lid_overlap',(.018,.095,z),(.003,.185,.004),'Paper')
 return m

def switchboard(lod):
 m=Mesh('SM_ArchiveProps_Switchboard')
 m.rb('KSU_steel_enclosure',(0,0,0),(.10,.46,.34),.015,'BeigeEnamel',lod)
 m.print('KSU_extension_directory',(-.051,0,0),.335,.455,11,normal=(-1,0,0))
 for z in [-.144,.144]:
  for y in [-.204,.204]:m.rod('KSU_captive_screw',(-.055,y,z),(-.062,y,z),.005,'Metal',lod)
 for i in range(4):
  z=-.17+(22+i*56+18)/256*.34
  # Open rectangular metal bezel. Native lenses 391/392 occupy the first two voids.
  for zz in [z-.021,z+.021]:m.b('Lamp_bezel_side',(-.055,.112,zz),(.012,.032,.005),'DarkSteel')
  for yy in [.096,.128]:m.b('Lamp_bezel_rail',(-.055,yy,z),(.012,.005,.047),'DarkSteel')
  if i>=2:m.b('Inactive_INT_HOLD_lens',(-.055,.112,z),(.014,.026,.036),'Rubber')
 if lod<2:
  for k in range(7):m.b('KSU_vent_slot',(-.052,-.202,.075-k*.024),(.003,.008,.014),'DarkSteel')
 m.b('Cable_trunking',(.02,.68,.12),(.03,.9,.03),'DarkSteel')
 return m

def recorder(lod):
 m=Mesh('SM_ArchiveProps_CallRecorder')
 m.b('Steel_recorder_bracket',(0,-.008,0),(.24,.016,.40),'DarkSteel')
 for z in [-.16,.16]:m.b('Bracket_wall_return',(.10,.05,z),(.025,.13,.025),'DarkSteel')
 m.rb('Mechanical_call_recorder',(.01,.045,0),(.20,.09,.24),.007,'BeigeEnamel',lod)
 # Paper emerges at x=-.098/y=.07. Housing and slot never cross that plane.
 m.b('Paper_exit_dark_slot',(-.091,.070,0),(.006,.012,.16),'Rubber')
 m.b('Printhead_inspection_window',(-.092,.030,.080),(.006,.018,.05),'Rubber')
 m.rod('Original_paper_roll',(.03,.115,-.08),(.03,.115,.08),.035,'Paper',lod)
 for z in [-.084,.084]:m.rod('Paper_spindle_support',(.03,.090,z),(.03,.14,z),.006,'Metal',lod)
 m.print('Recorder_identity',(-.092,.034,-.050),.072,.029,12,normal=(-1,0,0))
 for z in [-.10,.10]:m.rod('Recorder_front_screw',(-.092,.020,z),(-.097,.020,z),.003,'Metal',lod)
 return m

def binder(lod):
 m=Mesh('SM_ArchiveProps_ServiceBinder');m.rb('Service_folder_cloth',(0,.03,0),(.30,.06,.34),.005,'BookBlue',lod)
 m.b('Compressed_paper_edge',(.006,.030,0),(.274,.043,.326),'Paper')
 for y in [.004,.058]:m.b('Hard_cloth_cover',(0,y,0),(.307,.008,.348),'BookBlue')
 m.topprint('Service_copies_cover',(0,.062,0),.235,.20,13)
 m.b('Folded_bound_spine',(-.150,.031,0),(.014,.06,.344),'BookBlue')
 return m

def build(lod):
 return {m.name:m for m in [shelving(lod),filing(lod),cardindex(lod),bookcase(lod),table(lod),fieldbox(lod),drawer(lod),reel(lod),switchboard(lod),recorder(lod),binder(lod)]}
