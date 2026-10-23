"""Original Saucer Crash 1947 shop for SIGNAL / 47. Blender 5.2, metres, Z up.

Source generation/export only by default. Add -- --render only when the shared
graphics queue is free. Runtime/import/collision remain owned by the parent task.
"""
import bpy, math, random, json, sys, hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent; sys.path.insert(0,str(ROOT))
from make_textures import generate
from mesh_cleanup import clean_triangulated_mesh
ROOT.joinpath('exports').mkdir(exist_ok=True); generate(ROOT/'textures')
random.seed(1947)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
for m in list(bpy.data.materials): bpy.data.materials.remove(m)
scene=bpy.context.scene; scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1
groups={k:[] for k in ['Shell','Interior','Glass']}; current='Shell'

def material(name,col,metal=0,rough=.7,em=0):
 m=bpy.data.materials.new('M_Souvenir_'+name); m.use_nodes=True; m.diffuse_color=(*col,1)
 p=m.node_tree.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value=(*col,1); p.inputs['Metallic'].default_value=metal; p.inputs['Roughness'].default_value=rough
 if em: p.inputs['Emission Color'].default_value=(*col,1); p.inputs['Emission Strength'].default_value=em
 return m
cream=material('CreamStucco',(.60,.53,.41),rough=.9)
walnut=material('Walnut',(.16,.065,.023),rough=.6)
counter=material('CounterWalnut',(.28,.13,.045),rough=.5)
olive=material('OlivePaint',(.14,.17,.12),rough=.67)
concrete=material('Concrete',(.30,.28,.24),rough=.92)
iron=material('BlackIron',(.018,.023,.020),metal=.35,rough=.48)
steel=material('Galvanized',(.32,.35,.32),metal=.75,rough=.45)
brass=material('WornBrass',(.38,.24,.075),metal=.72,rough=.4)
ivory=material('CeramicIvory',(.68,.59,.41),rough=.3)
teal=material('CeramicTeal',(.055,.18,.19),rough=.32)
red=material('FadedRed',(.37,.065,.035),rough=.65)
paper=material('CreamPaper',(.66,.60,.43),rough=.88)
ink=material('PrintInk',(.025,.032,.027),rough=.8)
mesa=material('MesaPrint',(.40,.19,.075),rough=.9)
sky=material('SkyPrint',(.14,.28,.34),rough=.9)
lamp=material('WarmLamp',(1,.62,.24),rough=.3,em=1.8)
glass=material('ClearGlass',(.22,.26,.23),rough=.10)
glass.node_tree.nodes['Principled BSDF'].inputs['Transmission Weight'].default_value=.96
glass.node_tree.nodes['Principled BSDF'].inputs['IOR'].default_value=1.45
textured={cream:'stucco',walnut:'walnut',counter:'desk_walnut',concrete:'concrete'}
for m,prefix in textured.items():
 n=m.node_tree.nodes; links=m.node_tree.links; p=n.get('Principled BSDF')
 for suffix,socket in [('color','Base Color'),('roughness','Roughness'),('normal_gl','Normal')]:
  tex=n.new('ShaderNodeTexImage'); tex.image=bpy.data.images.load(str(ROOT/'textures'/(prefix+'_'+suffix+'.png')))
  if suffix!='color': tex.image.colorspace_settings.name='Non-Color'
  if suffix=='normal_gl':
   norm=n.new('ShaderNodeNormalMap'); norm.inputs['Strength'].default_value=.40; links.new(tex.outputs['Color'],norm.inputs['Color']); links.new(norm.outputs['Normal'],p.inputs[socket])
  else: links.new(tex.outputs['Color'],p.inputs[socket])

def add(o,name,mat):
 o.name=name; o.data.materials.append(mat); groups[current].append(o); return o
def box(name,loc,size,mat,bevel=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc); o=bpy.context.object; o.dimensions=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  mod=o.modifiers.new('SoftWornEdges','BEVEL'); mod.width=min(bevel,min(size)*.4); mod.segments=2
  bpy.ops.object.modifier_apply(modifier=mod.name)
 # Repeat the small PBR surfaces in metric object coordinates, not once per wall.
 if mat in textured:
  uv=o.data.uv_layers.active
  for poly in o.data.polygons:
   axis=max(range(3),key=lambda i:abs(poly.normal[i])); axes={0:(1,2),1:(0,2),2:(0,1)}[axis]
   for loop in poly.loop_indices:
    co=o.data.vertices[o.data.loops[loop].vertex_index].co; uv.data[loop].uv=(co[axes[0]],co[axes[1]])
 return add(o,name,mat)
def rod(name,a,b,r,mat,segments=10):
 a=Vector(a); b=Vector(b); delta=b-a
 bpy.ops.mesh.primitive_cylinder_add(vertices=segments,radius=r,depth=delta.length,location=(a+b)/2)
 o=bpy.context.object; o.rotation_euler=delta.to_track_quat('Z','Y').to_euler(); return add(o,name,mat)
def ring(name,loc,r,t,mat,rot=(0,0,0)):
 bpy.ops.mesh.primitive_torus_add(major_radius=r,minor_radius=t,major_segments=20,minor_segments=6,location=loc,rotation=rot)
 return add(bpy.context.object,name,mat)
def sphere(name,loc,scale,mat):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=16,ring_count=8,radius=1,location=loc)
 o=bpy.context.object; o.scale=scale; bpy.ops.object.transform_apply(location=False,rotation=False,scale=True); return add(o,name,mat)
def lettering(name,body,loc,size,mat,rot=(math.pi/2,0,0),align='CENTER'):
 bpy.ops.object.text_add(location=loc,rotation=rot); o=bpy.context.object
 o.data.body=body; o.data.align_x=align; o.data.size=size; o.data.extrude=.0005; o.data.resolution_u=3
 bpy.ops.object.convert(target='MESH'); return add(bpy.context.object,name,mat)
def flat_mesh(name,vertices,faces,mat):
 mesh=bpy.data.meshes.new(name); mesh.from_pydata(vertices,[],faces); mesh.update()
 o=bpy.data.objects.new(name,mesh); scene.collection.objects.link(o); return add(o,name,mat)

# Reference facade plane: world X8020.5, floor4.952807474, worldZ2058..2070.
box('ShopFoundation',(0,0,-.23),(12.16,9.16,.46),concrete)
box('FloorboardsSubfloor',(0,0,-.045),(11.6,8.6,.09),walnut)
for i in range(42): box('TimberFloorBoard',(-5.65+i*.273,0,.012),(.265,8.55,.024),counter)
box('LeftStuccoWall',(-5.87,0,1.85),(.26,9,3.7),cream)
box('RightStuccoWall',(5.87,0,1.85),(.26,9,3.7),cream)
box('BackStuccoWall',(0,4.37,1.85),(12,.26,3.7),cream)
# Window: X[-2.8,-.6], Z[.6,2.3]. Door:X[2.3,3.3], Z[.025,2.175].
for x0,x1 in [(-6,-2.8),(-.6,2.3),(3.3,6)]: box('FrontStuccoPier',((x0+x1)/2,-4.37,1.85),(x1-x0,.26,3.7),cream)
box('WindowSillWall',(-1.7,-4.37,.30),(2.2,.26,.60),cream)
box('WindowLintel',(-1.7,-4.37,3.0),(2.2,.26,1.4),cream)
box('DoorLintel',(2.8,-4.37,2.9375),(1,.26,1.525),cream)
box('FlatRoof',(0,0,3.67),(12,9,.14),concrete)
for y in [-4.375,4.375]: box('RoofParapet',(0,y,3.875),(12.06,.25,.35),cream)
for x in [-5.875,5.875]: box('RoofParapet',(x,0,3.875),(.25,9,.35),cream)
for x in [-5.82,5.82]: rod('RainDownpipe',(x,-4.55,.02),(x,-4.55,3.65),.035,steel)
# Deep timber reveal projects glazing towards street, in front of original A02.
for x in [-2.855,-.545]: box('DisplayFrameVertical',(x,-4.63,1.45),(.11,.40,1.92),walnut,.015)
for z in [.545,2.355]: box('DisplayFrameHorizontal',(-1.7,-4.63,z),(2.2,.40,.11),walnut,.015)
box('DisplayStoneSill',(-1.7,-4.72,.51),(2.48,.63,.10),concrete,.025)
box('DisplayInnerSill',(-1.7,-4.27,.58),(2.4,.23,.08),counter,.018)
# Existing closed door remains static. No new interaction or walkable room.
box('ClosedDoorLeaf',(2.8,-4.49,1.10),(.94,.09,2.10),olive,.015)
for x in [2.25,3.35]: box('DoorJamb',(x,-4.53,1.12),(.10,.18,2.24),walnut,.012)
box('DoorHeader',(2.8,-4.53,2.26),(1.2,.18,.10),walnut,.012)
box('DoorUpperGlass',(2.8,-4.55,1.57),(.64,.02,.65),iron)
for x in [2.45,3.15]: box('DoorRaisedMoulding',(x,-4.555,1.10),(.04,.025,1.89),counter,.006)
for z in [.25,.88,1.92]: box('DoorRaisedMoulding',(2.8,-4.555,z),(.68,.025,.04),counter,.006)
rod('DoorBrassPull',(3.11,-4.64,.94),(3.11,-4.64,1.15),.018,brass)
box('DoorFadedHours',(2.8,-4.57,1.51),(.42,.015,.20),paper,.008)
lettering('ClosedDoorCard','CLOSED',(2.8,-4.581,1.48),.068,ink)
box('DoorStoop',(2.8,-4.95,-.07),(1.4,.9,.14),concrete,.02)
# Original porch spans xworld18.3..20.5 and three peeled timber posts.
box('PorchSlab',(0,-5.42,-.14),(12.4,2.4,.20),concrete,.025)
for x in [-5.6,0,5.6]:
 box('PeeledPorchPost',(x,-6.4,1.31),(.17,.17,2.62),walnut,.025)
 box('PorchPostFoot',(x,-6.4,.06),(.25,.25,.12),concrete,.015)
 rod('PorchDiagonalBrace',(x,-6.4,2.0),(x+(.45 if x<0 else -.45),-6.4,2.59),.037,walnut)
box('PorchFrontBeam',(0,-6.4,2.68),(12.4,.18,.18),counter,.02)
box('PorchWallLedger',(0,-4.42,2.85),(12.2,.14,.16),walnut,.015)
for x in [-5.8+i*1.45 for i in range(9)]: rod('PorchRafter',(x,-6.55,2.66),(x,-4.32,2.87),.055,walnut)
# Corrugated metal canopy, a few rust patches in the joints rather than random clutter.
roof=box('CorrugatedCanopy',(0,-5.44,2.82),(12.5,2.4,.055),steel)
roof.rotation_euler.x=math.radians(5)
for i in range(78):
 x=-6.15+i*.16; rod('CorrugatedRoofRib',(x,-6.64,2.725),(x,-4.24,2.935),.018,steel,6)
for x in [-5.7,-2.5,1.4,5.6]: box('CanopyWeatheredSeam',(x,-5.45,2.86),(.035,2.35,.006),red)
# A facade ghost sign and standing tour board preserve the existing name and proxy.
lettering('FadedFacadeName','SAUCER CRASH 1947',(0,-4.515,3.18),.56,red)
for x in [-1,1.2]: box('TourBoardSupport',(x,-6.2,3.19),(.075,.075,1.12),walnut)
box('StandingTourBoard',(.1,-6.3,3.60),(2.6,.13,1.30),counter,.02)
box('TourBoardPaint',(.1,-6.375,3.60),(2.5,.02,1.19),paper)
lettering('TourBoardTitle','SAUCER CRASH 1947',(.1,-6.39,3.94),.16,red)
lettering('TourBoardTours','TOURS',(.28,-6.39,3.48),.35,olive)
lettering('TourBoardAsk','ASK INSIDE',(.1,-6.39,3.18),.17,ink)
# Painted saucer pictogram is a toy-like disc, never a second alien/figure.
sphere('TourSignSaucer',(-.85,-6.405,3.58),(.28,.01,.065),steel)
sphere('TourSignDome',(-.85,-6.42,3.64),(.12,.012,.08),teal)
# Exterior practical lamp geometry. Actual runtime lights are owned by the parent.
for x in [-1.7,2.8]:
 rod('PorchLampStem',(x,-4.61,2.57),(x,-4.88,2.57),.016,iron)
 bpy.ops.mesh.primitive_cone_add(vertices=16,radius1=.13,radius2=.055,depth=.10,location=(x,-4.88,2.56)); add(bpy.context.object,'PorchLampShade',iron)
 rod('PorchLampDiffuser',(x,-4.88,2.505),(x,-4.88,2.511),.095,lamp,16)
# Modest bench and ash urn, clear of the figure/proxy and closed doorway.
for x in [-4.9,-3.4]:
 for y in [-5.3,-5.7]: box('BenchLeg',(x,y,.22),(.07,.07,.44),iron,.008)
for i in range(4): box('BenchSeatSlat',(-4.15,-5.3-i*.12,.46),(1.8,.095,.055),walnut,.008)
rod('AshUrnStem',(4.5,-5.45,0),(4.5,-5.45,.74),.035,iron)
rod('AshUrnBase',(4.5,-5.45,.01),(4.5,-5.45,.04),.15,iron,16)
ring('AshUrnRim',(4.5,-5.45,.76),.13,.025,steel)

current='Interior'
# Only the first five metres are dressed; opaque storage partition terminates view.
box('RearStorePartition',(0,.75,1.75),(11.5,.13,3.45),walnut)
box('InnerCeiling',(0,-1.9,3.52),(11.5,5.15,.06),paper)
for x in [-5.45,-3.65,-1.85,-.05,1.75,3.55,5.35]: box('CeilingTimber',(x,-1.9,3.44),(.075,5.15,.12),counter,.012)
for x in [-5.69,5.69]: box('ShopTimberLining',(x,-1.85,1.7),(.045,5.1,3.34),walnut)
for i in range(26):
 x=-5.55+i*.44; box('RearWallPanelBatten',(x,.66,1.72),(.035,.055,3.35),counter,.005)
# Counter to window's right; the sight cylinder around A02 remains empty.
box('CashCounterCase',(1.80,-2.72,.55),(3.1,.86,1.10),walnut,.03)
box('CashCounterTop',(1.80,-2.72,1.14),(3.25,1.0,.12),counter,.025)
box('CounterPlinth',(1.8,-2.72,.08),(3.2,.94,.16),olive,.014)
for x in [.48,1.36,2.24,3.12]:
 box('CounterPanel',(x,-3.168,.56),(.78,.022,.80),counter,.018)
 box('CounterPanelInset',(x,-3.185,.56),(.62,.013,.63),walnut,.012)
for x in [.25,3.35]: box('CounterCornerPost',(x,-2.72,.59),(.12,.98,1.1),counter,.018)
# Mechanical cash register, exposed round keys, receipt roll, drawer and amount display.
box('RegisterBase',(1.55,-2.72,1.26),(.72,.57,.12),iron,.035)
box('RegisterDrawer',(1.55,-3.02,1.25),(.62,.035,.08),brass,.008)
rod('RegisterDrawerPull',(1.45,-3.055,1.25),(1.65,-3.055,1.25),.012,iron)
box('RegisterMainBody',(1.55,-2.64,1.46),(.62,.36,.35),iron,.07)
box('RegisterKeybed',(1.55,-2.93,1.38),(.65,.29,.08),iron,.022)
for row in range(4):
 for col in range(9):
  x=1.30+col*.063; y=-3.04+row*.058
  rod('RegisterRoundKey',(x,y,1.416),(x,y,1.443),.020,ivory if row%2 else brass,10)
box('RegisterAmountHousing',(1.55,-2.54,1.71),(.56,.13,.22),brass,.018)
box('RegisterAmountGlass',(1.55,-2.614,1.71),(.49,.02,.14),iron,.008)
lettering('RegisterAmount','0 0 0 0',(1.55,-2.628,1.67),.085,paper)
rod('RegisterReceiptRoll',(1.20,-2.67,1.73),(1.20,-2.47,1.73),.061,paper,16)
box('RegisterReceipt',(1.20,-2.74,1.60),(.095,.17,.004),paper)
# Brass brackets, stock labels and real depth in each bookcase.
def bookcase(name,x,y,width,depth,height,front=True):
 for xx in [x-width/2,x+width/2]: box(name+'_Side',(xx,y,height/2),( .07,depth,height),walnut,.01)
 box(name+'_Back',(x,y+depth/2-.025,height/2),(width,.05,height),walnut)
 levels=[.15,.67,1.19,1.71,2.23]
 for z in levels: box(name+'_Shelf',(x,y,z),(width,depth,.055),counter,.007)
 for z in levels[:-1]:
  for xx in [x-width*.36,x+width*.36]:
   rod(name+'_Bracket',(xx,y+depth/2-.06,z-.16),(xx,y-depth/2+.08,z-.03),.012,brass,6)
 return levels
for x in [-4.2,-1.9,.4,2.7,5.0]: bookcase('RearSouvenirBookcase',x,.20,2.08,.70,2.53)
# Side bookcase is visible obliquely through the left display reveal.
bookcase('LeftSideCabinet',-4.55,-2.12,1.7,.76,2.40)

def mug(name,x,y,z,mat=ivory):
 # Lathed open mug with thick rim, inner wall and a real handle.
 profile=[(.065,0),(.074,.015),(.074,.145),(.068,.16),(.057,.155),(.057,.025),(0,.025),(0,0)]
 verts=[]; seg=20
 for r,h in profile:
  for i in range(seg):
   a=i*math.tau/seg; verts.append((x+r*math.cos(a),y+r*math.sin(a),z+h))
 faces=[]
 for j in range(len(profile)-1):
  for i in range(seg): faces.append((j*seg+i,j*seg+(i+1)%seg,(j+1)*seg+(i+1)%seg,(j+1)*seg+i))
 flat_mesh(name+'_Cup',verts,faces,mat)
 ring(name+'_Handle',(x+.083,y,z+.09),.042,.012,mat,rot=(math.pi/2,0,0))
 # A miniature disc graphic is raised ceramic, not an alien face.
 sphere(name+'_SaucerPrint',(x,y-.075,z+.085),(.035,.002,.012),teal if mat==ivory else ivory)
def ufo(name,x,y,z):
 sphere(name+'_Disc',(x,y,z+.055),(.14,.14,.035),steel)
 sphere(name+'_Dome',(x,y,z+.089),(.066,.066,.045),teal)
 ring(name+'_Rim',(x,y,z+.055),.13,.008,brass)
 for a in [0,2.094,4.188]: rod(name+'_ToyLeg',(x+.055*math.cos(a),y+.055*math.sin(a),z+.05),(x+.08*math.cos(a),y+.08*math.sin(a),z),.007,iron,6)
for bay,x in enumerate([-4.2,-1.9,.4,2.7,5.0]):
 for row,z in enumerate([.71,1.23,1.75]):
  for col in range(4):
   xx=x-.72+col*.46
   if row==1: ufo('ShelfToyUFO',xx,.02,z)
   elif row==0: mug('ShelfMug',xx,.01,z,ivory if (col+bay)%2 else teal)
   else:
    box('FoldedMapBundle',(xx,.03,z+.10),(.28,.34,.19),paper,.01)
    box('MapBundleWrapper',(xx,-.147,z+.10),(.09,.005,.16),olive)
  box('ShelfPriceCard',(x,-.174,z-.035),(.27,.012,.10),paper,.004)
  lettering('ShelfPriceInk','$1.00  UP',(x,-.183,z-.058),.036,ink)
# Forward low display table supports stock only behind the A02 silhouette.
box('WindowDisplayTable',(-1.7,-3.83,.48),(2.00,.55,.07),counter,.012)
for x in [-2.5,-.9]:
 for y in [-4.02,-3.65]: box('WindowTableLeg',(x,y,.23),(.045,.045,.46),walnut)
for i,x in enumerate([-2.45,-.95]):
 ufo('WindowToySaucer',x,-3.80,.52)
 mug('WindowMug',x,-3.70,.72,ivory) if i==0 else None

def postcard(name,x,y,z,w=.23,h=.33):
 box(name+'_Card',(x,y,z),(w,.009,h),paper)
 box(name+'_Sky',(x,y-.006,z+.025),(w*.86,.003,h*.55),sky)
 # Original printed desert mesa polygon on each postcard.
 flat_mesh(name+'_Mesa',[(x-w*.43,y-.009,z-.07),(x-w*.43,y-.009,z-.01),(x-w*.2,y-.009,z+.045),(x+w*.07,y-.009,z+.045),(x+w*.2,y-.009,z-.025),(x+w*.43,y-.009,z-.025),(x+w*.43,y-.009,z-.07)],[(0,1,2,3,4,5,6)],mesa)
 sphere(name+'_Sun',(x+w*.27,y-.012,z+.095),(w*.08,.002,w*.08),ivory)
 lettering(name+'_NewMexico','NEW MEXICO',(x,y-.011,z-h*.43),w*.10,ink)
# Postcard spinner rack left of the counter, clear of the window centre.
rod('PostcardRackSpine',(-3.48,-2.72,.1),(-3.48,-2.72,2.18),.022,iron)
rod('PostcardRackBase',(-3.48,-2.72,.035),(-3.48,-2.72,.075),.30,iron,20)
for row in range(4):
 z=.47+row*.39
 for col in range(3):
  x=-3.77+col*.29; postcard('RackPostcard',x,-2.83,z,.25,.34)
  for zz in [z-.14,z+.10]: rod('PostcardPocketWire',(x-.14,-2.88,zz),(x+.14,-2.88,zz),.006,steel,6)
  for xx in [x-.14,x+.14]: rod('PostcardPocketWire',(xx,-2.88,z-.14),(xx,-2.88,z+.10),.006,steel,6)
lettering('PostcardRackTop','POSTCARDS',(-3.48,-2.86,2.03),.09,paper)
# Local map poster behind counter: route network is hand drawn geometry, 1986 paper.
box('RoadMapBacking',(2.7,.116,2.95),(1.60,.025,.83),paper,.008)
for i in range(7):
 x=2.07+i*.18
 rod('MapRoadLine',(x,.096,2.62),(x+.17,.096,3.21),.004,mesa,6)
for a,b in [((2.0,.09,2.87),(3.4,.09,2.96)),((2.7,.09,2.63),(2.45,.09,3.2))]: rod('MapHighway',a,b,.009,red,6)
lettering('MapHeading','NEW MEXICO  /  ROAD MAP',(2.7,.08,3.26),.07,ink)
for i,x in enumerate([-4.7,-2.3,-.1,4.9]): postcard('WallPostcard',x,.13,2.94,.60,.73)
# Small counter merchandise and paper bags create a believable working shop.
for i in range(4):
 box('PaperShoppingBag',(2.77+i*.11,-2.61,1.37),(.085,.21,.33),paper,.004)
 rod('PaperBagHandle',(2.77+i*.11,-2.61,1.53),(2.77+i*.11,-2.61,1.60),.006,walnut,6)
box('CounterTourLeaflets',(.50,-2.80,1.218),(.32,.38,.025),paper,.004)
lettering('TourLeafletInk','SAUCER\nTOURS',(.5,-2.8,1.234),.06,red,rot=(0,0,0))
box('CounterGenuineDebris',(.70,-2.47,1.31),(.25,.26,.22),counter,.015)
for i in range(6): sphere('FoilDebrisBag',(.61+(i%3)*.08,-2.55+(i//3)*.09,1.43),(.035,.04,.055),steel)
lettering('DebrisCrateInk','GENUINE DEBRIS',(.70,-2.614,1.28),.025,ink)
# Ceiling fixtures are export geometry, with mild emissive surfaces.
for x in [-1.7,2.0]:
 for y in [-2.7,-.7]: rod('FluorescentHangWire',(x,y,3.30),(x,y,3.50),.005,iron,6)
 box('FluorescentHousing',(x,-1.7,3.30),(.23,2.2,.07),steel,.015)
 for dx in [-.065,.065]: rod('FluorescentTube',(x+dx,-2.72,3.24),(x+dx,-.68,3.24),.022,lamp,12)

# Keep the counter visibly inside the narrow original window from the pavement.
for o in groups['Interior']:
 if o.name.startswith(('CashCounter','Counter','Register','PaperShoppingBag','PaperBagHandle','FoilDebrisBag','DebrisCrateInk','TourLeafletInk')): o.location.x-=.9

current='Glass'
# Separate no-collision runtime glass is streetward of the existing unique figure.
box('DisplayGlazing',(-1.7,-4.76,1.45),(2.18,.012,1.68),glass)

# Named originals remain editable and independent from the compact runtime pieces.
source=bpy.data.collections.new('EDITABLE_SOURCE_enable_to_edit'); scene.collection.children.link(source)
for objects in groups.values():
 for original in objects:
  clone=original.copy(); clone.data=original.data.copy(); clone.name='SRC_'+original.name; source.objects.link(clone)
source.hide_render=True; source.hide_viewport=True
parts=[]
for name,objects in groups.items():
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects: o.select_set(True)
 bpy.context.view_layer.objects.active=objects[0]; bpy.ops.object.join(); o=bpy.context.object; o.name='SM_Souvenir_'+name
 scene.cursor.location=(0,0,0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR'); parts.append(o)
proof={o.name:clean_triangulated_mesh(o) for o in parts}
(ROOT/'mesh_cleanup.json').write_text(json.dumps(proof,indent=2))
for o in parts:
 bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active=o
 bpy.ops.export_scene.fbx(filepath=str(ROOT/'exports'/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
bpy.ops.object.select_all(action='DESELECT')
for o in parts: o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(ROOT/'Saucer_Crash_1947.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)

stats={}
for o in parts:
 m=o.data; m.calc_loop_triangles()
 stats[o.name]={'vertices':len(m.vertices),'triangles':len(m.loop_triangles),'materials':[a.name for a in m.materials],
  'bounds_local_m':[[min(v.co[i] for v in m.vertices) for i in range(3)],[max(v.co[i] for v in m.vertices) for i in range(3)]],
  'origin':list(o.location),'triangulated':True,'zero_area_triangles':sum(t.area==0 for t in m.loop_triangles),
  'uv_layers':[a.name for a in m.uv_layers],'uv_loops':{a.name:len(a.data) for a in m.uv_layers}}
contract={'asset':'Saucer Crash 1947 / Caliche souvenir shop','world_three_pivot_m':[8025,4.952807474136353,2064],
 'mapping':'worldThree=(8025+localY,floor+localZ,2064+localX)','blender_axes':'metres Z up; FBX -Y forward, Z up; UE runtime yaw=-90 same as radio station',
 'building_dimensions_m':[12,9,4.05],'front':'local -Y; structural facade Y=-4.5',
 'window_center_local_m':[-1.7,-4.5,1.45],'window_opening_m':[2.2,1.7],'glass_center_local_m':[-1.7,-4.76,1.45],
 'door_center_local_m':[2.8,-4.5,1.1],'door_opening_m':[1,2.15],'sign_center_local_m':[.1,-6.3,3.60],
 'figure_reserved_world_three_m':[8020.31,5.782807474136353,2062.3],
 'figure_policy':'No alien geometry is authored here. Preserve the one existing A02 cutout, price and split edge. Its proxy/logic remains unchanged.',
 'room_back_wall_local_y_m':.75,'visible_room_depth_m':5.25,'closed_door':True,'parts':stats,
 'notes':['Original display backing may be hidden by runtime to reveal the real shop; preserve its hit proxy.',
 'Canopy follows the three legacy post anchors at localX -5.6,0,5.6 / localY -6.4.',
 'Floor, stoop and porch are authored level. Shared street terrain is a separate parent asset.',
 'All export pieces use identical neutral origin and explicit nondegenerate triangles. Named originals are retained in a hidden editable source collection.',
 'No collision, interaction, new alien, UE import or runtime code is authored by this generator.']}
contract['practical_light_anchors_local_m']={'porch_window': [-1.7, -4.88, 2.505], 'porch_door': [2.8, -4.88, 2.505], 'ceiling_left': [-1.7, -1.7, 3.24], 'ceiling_right': [2.0, -1.7, 3.24]}
contract['source_editable_collection']='EDITABLE_SOURCE_enable_to_edit'
contract['preview_status']='NOT_RENDERED unless opt-in --render was requested'
(ROOT/'model_contract.json').write_text(json.dumps(contract,indent=2))
materials={}
for m in bpy.data.materials:
 p=m.node_tree.nodes.get('Principled BSDF')
 row={'base_color_linear':list(p.inputs['Base Color'].default_value),'metallic':p.inputs['Metallic'].default_value,'roughness':p.inputs['Roughness'].default_value,
      'emission':list(p.inputs['Emission Color'].default_value),'emission_strength':p.inputs['Emission Strength'].default_value,'alpha':.10 if m==glass else 1.0,'textures':{}}
 if m in textured:
  prefix=textured[m]; row['textures']={'base_color':'textures/'+prefix+'_color.png','normal':'textures/'+prefix+'_normal_dx.png','roughness':'textures/'+prefix+'_roughness.png'}
  row['normal_convention']='DirectX tangent space; Blender preview uses normal_gl image'
 materials[m.name]=row
(ROOT/'materials.json').write_text(json.dumps(materials,indent=2))

# Preview is deliberately separate from game assets and is opt-in for rendering.
def area(name,loc,power,color,size,target):
 data=bpy.data.lights.new(name,'AREA'); data.energy=power; data.color=color; data.shape='DISK'; data.size=size
 o=bpy.data.objects.new(name,data); scene.collection.objects.link(o); o.location=loc; o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
 o.visible_camera=False; o.visible_glossy=False; o.visible_transmission=False; data.specular_factor=0
 return o
area('PREVIEW_Sky',(-7,-13,9),1500,(.55,.68,1),12,(0,-2,1))
area('PREVIEW_Interior',(-1.1,-1.6,3.10),170,(1,.67,.34),3,(-1.1,-2,0))
area('PREVIEW_Porch',(-.4,-5.4,2.5),40,(1,.68,.36),2,(0,-5,0))
def camera(name,loc,target,lens):
 data=bpy.data.cameras.new(name); data.lens=lens; o=bpy.data.objects.new(name,data); scene.collection.objects.link(o); o.location=loc; o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler(); return o
outside=camera('CAM_Exterior',(-10,-19,4.7),(0,-2,1.65),36)
inside=camera('CAM_Interior',(-2.8,-4.10,1.70),(1.5,-1.2,1.25),22)
street=camera('CAM_StreetWindow',(-3.1,-8.2,1.8),(.3,-1.9,1.48),30)
scene.world.color=(.055,.065,.08); scene.render.engine='CYCLES'; scene.cycles.samples=32; scene.cycles.use_denoising=True
scene.render.threads_mode='FIXED'; scene.render.threads=2
scene.render.resolution_x=1440; scene.render.resolution_y=1080; scene.render.resolution_percentage=100; scene.view_settings.view_transform='AgX'
scene.camera=outside
for img in bpy.data.images:
 if img.source=='FILE': img.filepath=bpy.path.relpath(img.filepath,start=str(ROOT))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Saucer_Crash_1947.blend'))
if '--render' in sys.argv:
 for name,cam in [('exterior',outside),('interior',inside),('street_window',street)]:
  scene.camera=cam; scene.render.filepath=str(ROOT/(name+'.png')); bpy.ops.render.render(write_still=True)
print('SOUVENIR_EXPORTED',json.dumps({n:d['triangles'] for n,d in stats.items()}))
