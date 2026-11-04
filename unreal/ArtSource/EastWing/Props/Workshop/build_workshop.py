"""EASTWING-01 original workshop props and explicitly documented owned reuse.
Run only by root under production lease + require-idle PASS.
Blender metres, Z up, front -Y. No world placement or Claude integration.
"""
from pathlib import Path
import sys, math, json, shutil, hashlib
import bpy
from mathutils import Vector, Matrix
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent))
from prop_common import material,box,cyl,sphere,rod,text,start_asset,finish_asset,render_catalog
REPO=HERE.parents[4]
P=math.pi
M={
 'green':material('M_EW_GreenSteel',(.17,.25,.20,1),.42,.0,texture='EW_GreenSteel'),
 'steel':material('M_EW_Galvanized',(.45,.48,.47,1),.35,.85,texture='EW_Galvanized'),
 'dark':material('M_EW_DarkSteel',(.045,.05,.048,1),.45,.7),
 'red':material('M_EW_RedEnamel',(.34,.025,.018,1),.35,.05),
 'cream':material('M_EW_CreamEnamel',(.65,.62,.49,1),.45,.0),
 'rubber':material('M_EW_Rubber',(.018,.022,.019,1),.82,0),
 'wood':material('M_EW_CrateWood',(.30,.21,.12,1),.82,0),
 'paper':material('M_EW_LabelPaper',(.76,.72,.57,1),.78,0),
 'black':material('M_EW_LabelInk',(.008,.01,.009,1),.7,0),
 'glass':material('M_EW_FluorescentGlass',(.82,.85,.76,1),.25,0),
 'peg':material('M_EW_Pegboard',(.31,.22,.12,1),.75,0,texture='EW_Pegboard')}
SRC_ARCHIVE='unreal/Signal47/Tools/archiveprops/geometry.py'
SRC_FLUOR='Unity/Blender/Source/visual10_lab_details.py'
def b(n,p,s,m='green',bev=.003):return box(n,p,s,M[m],bev)
def c(n,p,r,d,m='steel',rot=(0,0,0),v=32):return cyl(n,p,r,d,M[m],vertices=v,rotation=rot)
def r(n,a,z,rad=.006,m='steel'):return rod(n,a,z,rad,M[m])
def label(body,x,y,z,w=.15):
 b('Label_plate_'+body,(x,y+.001,z),(w,.003,.047),'paper',.001)
 text('Label_'+body,body,(x,y-.002,z),min(.023,w/max(1,len(body))*.9),M['black'],rotation=(P/2,0,0))
def screw(x,y,z):
 c('Slotted_screw',(x,y,z),.004,.003,'steel',(P/2,0,0),12)
 b('Screw_slot',(x,y-.002,z),(.005,.001,.0008),'dark',0)
def pull(x,y,z,w=.16):
 for xx in [x-w/2,x+w/2]:r('Handle_standoff',(xx,y+.025,z),(xx,y,z),.007)
 r('Handle_bar',(x-w/2,y,z),(x+w/2,y,z),.009)
def caster(x,y,z=.065):
 c('Caster_tyre',(x,y,z),.055,.028,'rubber',(P/2,0,0),32)
 c('Caster_hub',(x,y-.016,z),.020,.005,'steel',(P/2,0,0),20)
 b('Caster_fork',(x,y+.021,z+.022),(.025,.01,.07),'steel')
 c('Swivel_plate',(x,y,z+.08),.033,.008,'steel')
def complete(name,description,sources=None):
 # Ground manually assembled wall tools whose construction uses convenient hook heights.
 if name in {'SM_EW_WallTools','SM_EW_PartsCart','SM_EW_TyreStack','SM_EW_FireExtinguisher'}:
  import prop_common
  obs=[o for o in bpy.context.scene.objects if o.as_pointer() not in prop_common.BEFORE and o.type in {'MESH','CURVE','FONT'}]
  dg=bpy.context.evaluated_depsgraph_get()
  low=min((o.matrix_world@Vector(p)).z for o in obs for p in o.evaluated_get(dg).bound_box)
  for o in obs:o.location.z-=low
 finish_asset(name,HERE,description,sources=sources or [],license_id='LicenseRef-ProjectOwned')
def cabinet(name,w=.7,h=.9,d=.55,drawers=5,mat='green',wheels=False):
 start_asset(name)
 zbase=.16 if wheels else .06
 b('Pressed_steel_shell',(0,0,(h+zbase)/2),(w,d,h-zbase),mat,.012)
 b('Black_kick_recess',(0,0,zbase/2),(w-.055,d-.04,zbase),'dark')
 b('Top_rolled_lip',(0,0,h+.012),(w+.02,d+.02,.025),'steel')
 dh=(h-zbase-.04)/drawers
 for i in range(drawers):
  z=zbase+.02+dh*(i+.5)
  b('Inset_drawer_front',(0,-d/2-.006,z),(w-.04,.026,dh-.012),mat,.004)
  pull(0,-d/2-.045,z+.012,w*.56)
  label('%02d'%(i+1),w*.30,-d/2-.024,z+dh*.23,.08)
  for xx in [-w*.4,w*.4]:screw(xx,-d/2-.023,z)
 c('Cabinet_lock',(w*.36,-d/2-.027,h-.035),.008,.009,'steel',(P/2,0,0),16)
 if wheels:
  for x in [-w*.36,w*.36]:
   for y in [-d*.35,d*.35]:caster(x,y)
  pull(w/2+.04,0,h*.75,.26)
 complete(name,'Folded-sheet drawer cabinet; recessed individual drawer fronts, label holders, rolled top, lock, physical pulls and screws.',[SRC_ARCHIVE+' filing(): adapted owned folded-sheet/pull detailing'])

cabinet('SM_EW_DrawerCabinet')
cabinet('SM_EW_RedToolChest',.9,.98,.51,6,'red',True)

start_asset('SM_EW_GreenBench')
b('Worktop',(0,0,.915),(1.8,.70,.035),'steel',.008)
b('Back_splash',(0,.34,.97),(1.8,.025,.10),'steel')
for x in [-.84,.84]:
 for y in [-.28,.28]:
  b('Folded_steel_leg',(x,y,.45),(.055,.055,.90),'green')
  b('Adjustable_foot',(x,y,.018),(.075,.075,.036),'rubber')
b('Lower_shelf',(0,0,.20),(1.69,.57,.025),'green')
for x in [-.58,.58]:
 b('Drawer_housing',(x,0,.80),(.53,.59,.18),'green')
 b('Drawer_front',(x,-.31,.80),(.51,.023,.15),'green')
 pull(x,-.36,.81,.26)
complete('SM_EW_GreenBench','1.8m green lacquered steel bench with galvanized worktop, two drawers, backsplash, lower shelf and levelling feet.',[SRC_ARCHIVE+' worktable concept, independently dimensioned'])

start_asset('SM_EW_PartsCart')
for z in [.20,.56,.91]:
 b('Tray_bottom',(0,0,z),(.68,.43,.025),'green')
 for x in [-.34,.34]:b('Tray_side',(x,0,z+.045),(.018,.45,.09),'green')
 for y in [-.22,.22]:b('Tray_end',(0,y,z+.045),(.68,.018,.09),'green')
for x in [-.32,.32]:
 for y in [-.20,.20]:
  b('Cart_upright',(x,y,.55),(.027,.027,.83),'steel')
  caster(x,y)
r('Handle_L',(-.32,.2,.96),(-.32,.31,1.12),.014)
r('Handle_R',(.32,.2,.96),(.32,.31,1.12),.014)
r('Handle_cross',(-.32,.31,1.12),(.32,.31,1.12),.014)
label('PARTS CART',0,-.235,.95,.24)
complete('SM_EW_PartsCart','Three-lipped-tray steel service cart, four swivel casters and bent tubular push handle.')

for name,word in [('SM_EW_CrateFuses','FUSES'),('SM_EW_CrateRelays','RELAYS')]:
 start_asset(name)
 for x in [-.21,0,.21]:b('Base_plank',(x,0,.025),(.205,.43,.05),'wood')
 for z in [.10,.19,.28]:
  for y in [-.225,.225]:b('Side_board',(0,y,z),(.66,.028,.082),'wood')
  for x in [-.315,.315]:b('End_board',(x,0,z),(.028,.43,.082),'wood')
 for x in [-.29,.29]:
  for y in [-.19,.19]:b('Corner_batten',(x,y,.18),(.036,.04,.31),'wood')
 for x in [-.25,.25]:
  for z in [.10,.19,.28]:screw(x,-.242,z)
 label(word,0,-.244,.19,.30)
 for x in [-.18,0,.18]:b('Packed_component_box',(x,0,.17),(.15,.29,.20),'paper')
 complete(name,'Open wooden stock crate with genuine slatted walls, corner battens, fasteners, packed component cartons and readable '+word+' label.')

start_asset('SM_EW_CableDrum')
# Reel axis Y, base Z=0, flanges radius .34. Open wound cable body.
for y in [-.22,.22]:
 c('Plywood_flange',(0,y,.34),.34,.035,'wood',(P/2,0,0),64)
 c('Hub_cap',(0,y*1.10,.34),.09,.015,'steel',(P/2,0,0),32)
 for i in range(8):
  a=i*P/4;c('Flange_bolt',(.23*math.sin(a),y*1.1,.34+.23*math.cos(a)),.009,.012,'steel',(P/2,0,0),12)
c('Reel_core',(0,0,.34),.13,.43,'wood',(P/2,0,0),48)
for j in range(20):
 y=-.195+j*.020
 bpy.ops.mesh.primitive_torus_add(major_radius=.235,minor_radius=.009,major_segments=64,minor_segments=8,location=(0,y,.34),rotation=(P/2,0,0))
 o=bpy.context.object;o.name='Wound_cable';o.data.materials.append(M['rubber'])
complete('SM_EW_CableDrum','Wood cable reel with paired flanges, hubs, bolts and individually modeled wound cable loops.')

start_asset('SM_EW_ChemicalCabinet')
b('Cabinet_body',(0,0,.92),(.88,.48,1.84),'cream',.016)
b('Recessed_toe',(0,0,.05),(.81,.43,.10),'dark')
for x in [-.212,.212]:
 b('Inset_door',(x,-.253,.96),(.414,.028,1.65),'cream',.007)
 for z in [.29,1.62]:b('Door_hinge',(x+(-.19 if x<0 else .19),-.271,z),(.025,.025,.11),'steel')
 for z in [.27,.32,.37]:b('Vent_louvre',(x,-.271,z),(.29,.008,.012),'dark',.002)
pull(.04,-.30,1.02,.075)
c('Lock_cylinder',(.10,-.287,1.0),.014,.012,'steel',(P/2,0,0),20)
b('Lock_hasps',(0,-.29,.88),(.11,.019,.043),'steel')
b('Padlock_body',(0,-.31,.83),(.044,.025,.052),'dark')
for x in [-.014,.014]:r('Padlock_shackle',(x,-.31,.855),(x,-.31,.88),.004)
r('Padlock_shackle_top',(-.014,-.31,.88),(.014,-.31,.88),.004)
label('CHEMICAL',0,-.274,1.46,.30);label('LOCKED',0,-.274,1.39,.24)
complete('SM_EW_ChemicalCabinet','Lockable chemical cupboard with double doors, ventilation louvres, hinge barrels, padlock and labels.')

start_asset('SM_EW_TyreStack')
for k in range(3):
 z=.13+k*.24
 bpy.ops.mesh.primitive_torus_add(major_radius=.255,minor_radius=.105,major_segments=64,minor_segments=20,location=(0,0,z))
 o=bpy.context.object;o.name='Tyre_carcass';o.scale.z=1.12;o.data.materials.append(M['rubber'])
 for j in range(48):
  a=j*math.tau/48
  for dz in [-.055,.055]:
   ob=b('Raised_tread_block',(.351*math.cos(a),.351*math.sin(a),z+dz),(.042,.026,.041),'rubber',.004)
   ob.rotation_euler.z=a
complete('SM_EW_TyreStack','Three hollow tyres with rounded carcasses and physical staggered tread blocks.')

start_asset('SM_EW_FireExtinguisher')
c('Cylinder_body',(0,0,.28),.085,.42,'red',(0,0,0),64)
sphere('Rounded_shoulder',(0,0,.49),(.085,.085,.05),M['red'])
sphere('Rounded_base',(0,0,.075),(.085,.085,.035),M['red'])
c('Base_ring',(0,0,.045),.087,.025,'rubber')
c('Valve_neck',(0,0,.54),.025,.06,'steel')
b('Lever_fixed',(0,-.005,.578),(.14,.025,.015),'dark')
b('Lever_press',(0,.005,.603),(.14,.028,.012),'red')
c('Pressure_gauge',(-.035,-.037,.55),.023,.016,'steel',(P/2,0,0),32)
c('Gauge_face',(-.035,-.047,.55),.019,.003,'paper',(P/2,0,0),32)
r('Gauge_needle',(-.035,-.05,.55),(-.028,-.05,.562),.0015,'black')
for a,z in [((.025,.012,.55),(.11,.02,.52)),((.11,.02,.52),(.12,.02,.29)),((.12,.02,.29),(.10,.01,.15))]:r('Discharge_hose',a,z,.012,'rubber')
c('Hose_nozzle',(.10,.01,.17),.018,.09,'dark')
label('FIRE',0,-.086,.35,.13);label('ABC DRY',0,-.086,.29,.13)
b('Wall_bracket',(0,.085,.30),(.075,.025,.22),'steel')
complete('SM_EW_FireExtinguisher','Period dry-powder extinguisher with hose, squeeze levers, pressure gauge, label and wall cradle.')

start_asset('SM_EW_Pegboard')
b('Pegboard_sheet',(0,0,.61),(1.2192,.022,1.2192),'peg',.003)
for x in [-.59,.59]:b('Frame_vertical',(x,-.018,.61),(.03,.025,1.22),'wood')
for z in [.02,1.20]:b('Frame_horizontal',(0,-.018,z),(1.22,.025,.03),'wood')
complete('SM_EW_Pegboard','48 inch square workshop pegboard; 2K PBR hole field on supplied material, wood mounting frame.')

start_asset('SM_EW_WallTools')
b('Mounting_rail',(0,.015,.58),(1.12,.025,.035),'steel')
for i in range(7):
 x=-.48+i*.095;L=.21+i*.012
 r('Spanner_shank',(x,0,.55),(x,0,.55-L),.009)
 for zz in [.55,.55-L]:
  bpy.ops.mesh.primitive_torus_add(major_radius=.018,minor_radius=.006,major_segments=24,minor_segments=8,location=(x,0,zz),rotation=(P/2,0,0))
  bpy.context.object.name='Ring_spanner_end';bpy.context.object.data.materials.append(M['steel'])
 r('Tool_hook',(x,.025,.58),(x,-.015,.58),.003)
for i in range(3):
 x=.24+i*.10
 c('Screwdriver_handle',(x,0,.46),.016,.12,'red',v=24)
 c('Screwdriver_shaft',(x,0,.33),.004,.15,'steel',v=12)
 b('Flat_driver_tip',(x,0,.25),(.010,.004,.018),'steel')
complete('SM_EW_WallTools','Hanging ring-spanner size set and three screwdrivers with mounting hooks; open spaces remain between tools.')

start_asset('SM_EW_RollUpDoorSegment')
b('Segment_skin',(0,0,.125),(3.5,.025,.25),'steel',.004)
for z in [.02,.23]:b('Rolled_interlock_edge',(0,.018,z),(3.5,.035,.025),'steel')
for z in [.065,.125,.185]:b('Pressed_stiffener',(0,-.019,z),(3.45,.012,.012),'steel',.003)
for x in [-1.69,1.69]:
 b('End_bracket',(x,.035,.125),(.07,.03,.18),'steel')
 c('Guide_roller',(x,.07,.125),.027,.032,'rubber',(0,P/2,0),24)
 for z in [.06,.19]:screw(x,-.017,z)
complete('SM_EW_RollUpDoorSegment','3.5m wide, 0.25m high modular roll-up segment with rolled interlocks, ribs and end rollers. Repeat vertically; pivot at lower center. Runtime articulation belongs to Claude.')

start_asset('SM_EW_DrainGrate')
for x in [-.245,.245]:b('Frame_side',(x,0,.025),(.010,.30,.05),'steel')
for y in [-.145,.145]:b('Frame_end',(0,y,.025),(.50,.010,.05),'steel')
for i in range(20):b('Open_grate_bar',(-.225+i*.0237,0,.040),(.010,.278,.020),'steel',.002)
for x in [-.23,.23]:
 for y in [-.13,.13]:c('Grate_fastener',(x,y,.052),.005,.003,'steel',v=12)
complete('SM_EW_DrainGrate','0.5 x 0.3m modular metal floor drain with genuine open slots and fixing frame; place top flush to floor.')

# Actual geometry reuse of original project-owned fluorescent. Do not execute source's exporters.
start_asset('SM_EW_FluorescentFixture')
source=REPO/SRC_FLUOR
code=source.read_text()
chunk=code[code.index("# Lower tube surface"):code.index('# Open plastic measuring jug')]
def oldbox(name,loc,size,material,bevel=.006):return b(name,loc,size,{'Cream':'cream','Steel':'steel','WarmDiffuser':'glass'}[material],bevel)
def oldcyl(name,loc,radius,depth,material,vertices=24,rotation=None):return c(name,loc,radius,depth,{'Cream':'cream','Steel':'steel','WarmDiffuser':'glass'}[material],rotation or (0,0,0),vertices)
exec(compile(chunk,str(source),'exec'),{'box':oldbox,'cylinder':oldcyl,'math':math})
complete('SM_EW_FluorescentFixture','Original V10 twin fluorescent geometry reused verbatim: independent tubes, pin ferrules, angled reflector, porcelain holders and ballast cover. Local lower tube at Z=0; mounting plate +0.10m. Emission must follow runtime power.',[SRC_FLUOR+' V10_TwinFluorescent block, owned original geometry'])

# Reuse the existing authored shelving geometry (including ordinary cartons), normalize pivot.
name='SM_EW_ShelvingStocked';start_asset(name)
prior=set(bpy.data.objects)
path=REPO/'unreal/Signal47/Tools/archiveprops/exports/SM_ArchiveProps_ShelvingStorage.fbx'
bpy.ops.import_scene.fbx(filepath=str(path))
obs=[o for o in bpy.data.objects if o not in prior and o.type=='MESH']
# Copy only actual original texture dependencies, preserving byte content/resolution.
texdir=HERE/'ReusedTextures';texdir.mkdir(exist_ok=True)
for o in obs:
 for mat in o.data.materials:
  if not mat or not mat.use_nodes:continue
  for node in mat.node_tree.nodes:
   if node.type!='TEX_IMAGE' or not node.image:continue
   im=node.image;original=Path(bpy.path.abspath(im.filepath))
   if not original.exists():
    candidates=list((REPO/'unreal/Signal47/Tools/archiveprops/textures').glob(original.name))
    if len(candidates)!=1:raise FileNotFoundError(str(original))
    original=candidates[0]
   dest=texdir/original.name
   if original.resolve()!=dest.resolve():shutil.copy2(original,dest)
   im.filepath=str(dest)
# FBX roundtrip returns original metres under established export settings.
points=[o.matrix_world@v.co for o in obs for v in o.data.vertices]
lo=Vector([min(v[k] for v in points) for k in range(3)]);hi=Vector([max(v[k] for v in points) for k in range(3)])
shift=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
for o in obs:
 o.location-=shift
 o.matrix_world=Matrix.Rotation(-P/2,4,'Z')@o.matrix_world
 # Keep all original baked material slots and original UVs; source manifest records originals.
complete(name,'Reused five-tier original SARO shelving with carton dressing, braces and bolts. Original material UVs retained; rotated from source +X front to -Y front. Own prior assets, no third-party models.',[str(path.relative_to(REPO)),SRC_ARCHIVE])

start_asset('SM_EW_GalvanizedShelving')
for x in [-.58,.58]:
 for y in [-.24,.24]:
  b('Angle_upright',(x,y,1.0),(.035,.035,2.0),'steel')
  b('Foot_plate',(x,y,.016),(.09,.09,.032),'steel')
  for z in [.2,.6,1.,1.4,1.8]:screw(x,y-.02,z)
for z in [.12,.56,1.,1.44,1.88]:
 b('Folded_shelf',(0,0,z),(1.20,.50,.025),'steel')
 for y in [-.24,.24]:b('Shelf_lip',(0,y,z-.02),(1.20,.02,.05),'steel')
r('Cross_brace',(-.58,.25,.12),(.58,.25,1.9),.007)
r('Cross_brace',(.58,.25,.12),(-.58,.25,1.9),.007)
complete('SM_EW_GalvanizedShelving','Empty modular 1.2m five-tier galvanized shelf with folded lips, L-posts, footplates and diagonal braces.',[SRC_ARCHIVE+' shelving(): folded-sheet construction adapted'])

render_catalog(HERE,'EASTWING-01 / WORKSHOP PROPS')
print('EASTWING_WORKSHOP_DONE',flush=True)
