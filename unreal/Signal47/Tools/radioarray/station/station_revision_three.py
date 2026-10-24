"""Street-facing analog composition and finer original physical detail."""
import bpy,math,random
from mathutils import Vector,Matrix
from station_revision_two import textured

def apply(groups,mat,cube,cyl,torus,text,namespace):
 namespace['current']='Interior';r=random.Random(19861420);rows={}
 dark=bpy.data.materials['M_1420_Black_enamel'];metal=bpy.data.materials['M_1420_Brushed_aluminium']
 wood=bpy.data.materials['M_1420_Walnut'];ivory=bpy.data.materials['M_1420_Ivory_plastic'];paper=bpy.data.materials['M_1420_Paper'];ink=bpy.data.materials['M_1420_Ink']
 red=bpy.data.materials['M_1420_Brick_red'];green=bpy.data.materials['M_1420_Vinyl_green']
 rows[green.name]=textured(green,'aged_vinyl',(.025,.042,.028),1,.28,folder='revision3')
 panel=mat('M_1420_ProgramPanelPrint',(.15,.16,.13),rough=.63)
 rows[panel.name]=textured(panel,'program_panel',(.15,.16,.13),1,.05,folder='revision3')

 def tube(name,points,rad,m):
  curve=bpy.data.curves.new(name,'CURVE');curve.dimensions='3D';curve.resolution_u=2;curve.bevel_depth=rad;curve.bevel_resolution=2
  sp=curve.splines.new('POLY');sp.points.add(len(points)-1)
  for p,co in zip(sp.points,points):p.co=(*co,1)
  o=bpy.data.objects.new(name,curve);bpy.context.scene.collection.objects.link(o);bpy.ops.object.select_all(action='DESELECT');bpy.context.view_layer.objects.active=o;o.select_set(True)
  bpy.ops.object.convert(target='MESH');o=bpy.context.object;o.data.materials.append(m);groups['Interior'].append(o);o.select_set(False);return o

 def face_atlas(name,x,y,z,w,h,m):
  mesh=bpy.data.meshes.new(name);mesh.from_pydata([(x-w/2,y,z-h/2),(x+w/2,y,z-h/2),(x+w/2,y,z+h/2),(x-w/2,y,z+h/2)],[],[(0,1,2,3)]);mesh.update()
  uv=mesh.uv_layers.new(name='UVMap')
  for p,q in zip(uv.data,[(0,0),(1,0),(1,1),(0,1)]):p.uv=q
  o=bpy.data.objects.new(name,mesh);bpy.context.scene.collection.objects.link(o);o.data.materials.append(m);groups['Interior'].append(o);return o

 # The real tape transport now presents its face to the street through the opening.
 target=Vector((.14,2.30,0));old=Vector((-1.8,1.3,0));rot=Matrix.Rotation(math.pi,4,'Z')
 tape=[]
 for o in groups['Interior']:
  if o.name.startswith(('Reel_machine','Tape_','Exposed_tape')):
   o.matrix_world=Matrix.Translation(target)@rot@Matrix.Translation(-old)@o.matrix_world;tape.append(o.name)
 # Rear meter bridge is a physical, legible station program monitor instead of a blank slab.
 face_atlas('ProgramMonitorStreetFace',1.2,1.083,1.185,2.61,.245,panel)
 for x in [1.40,1.65,1.90,2.15]:
  cyl('MonitorKnob',(x,1.080,1.19),(x,1.053,1.19),.017,dark,20)
  cube('MonitorKnobIndex',(x,1.050,1.197),(.002,.002,.015),paper)
 for x in [-.20,2.60]:cube('ConsoleWoodCheek',(x,1.54,1.04),(.085,.86,.19),wood,.025)
 # Broadcast connectors, vents and cable boots give the actual rear chassis a plausible structure.
 for i in range(14):
  x=-.02+i*.187
  cube('ConsoleBackVent',(x,1.096,1.022),(.105,.004,.009),dark,.002)
  if i%2==0:
   cyl('RearXLRSocket',(x,1.095,.972),(x,1.070,.972),.019,dark,16)
   for a in [0,2.094,4.188]:cyl('XLRContact',(x+.008*math.cos(a),1.068,.972+.008*math.sin(a)),(x+.008*math.cos(a),1.066,.972+.008*math.sin(a)),.0024,metal,6)
 # Small cart machine on the right is visibly turned toward the window, not another closed box.
 cube('WindowCartTransport',(2.62,2.37,1.32),(.68,.46,.78),wood,.035)
 cube('CartBrushedFace',(2.62,2.132,1.32),(.61,.018,.70),metal,.012)
 for z in [1.18,1.42]:
  cube('CartThroat',(2.59,2.117,z),(.37,.012,.095),dark,.006)
  cube('CartInserted',(2.59,2.087,z),(.30,.07,.064),ivory,.009)
  text('CartHandLabel','PSA / 06' if z<1.3 else 'WEATHER',(2.59,2.045,z-.012),.022,ink)
  cyl('CartPlayButton',(2.85,2.113,z),(2.85,2.093,z),.024,red,16)
 for x in [2.40,2.83]:cyl('CartFastener',(x,2.118,1.59),(x,2.113,1.59),.008,dark,12)
 # Clear title strips on the tape deck and a visible tape loop connecting both reels.
 text('TapeTransportID','1420 / MASTER',(.14,2.051,1.04),.043,ivory)
 tube('TapeTransportLoop',[(-.09,2.045,1.60),(-.03,2.041,1.39),(.14,2.041,1.35),(.31,2.041,1.39),(.37,2.045,1.60)],.0035,ink)

 # Round, upholstered cushions and real piped seams, retaining the original seat position.
 for o in list(groups['Interior']):
  if o.name.startswith(('Chair_seat','Chair_back','Chair_armrest')):
   groups['Interior'].remove(o);bpy.data.objects.remove(o,do_unlink=True)
 def cushion(name,loc,size):
  o=cube(name,loc,size,green);mod=o.modifiers.new('SoftUpholstery','BEVEL');mod.width=.055;mod.segments=6;mod.affect='EDGES';mod.use_clamp_overlap=True
  bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
  for p in o.data.polygons:p.use_smooth=True
  return o
 seat=cushion('Chair_seat_soft',(1.55,2.8,.51),(.67,.62,.15))
 back=cushion('Chair_back_soft',(1.55,3.07,.92),(.68,.16,.82));back.rotation_euler.x=math.radians(-8)
 for x in [1.14,1.96]:cushion('Chair_armrest_soft',(x,2.75,.76),(.10,.47,.07))
 seam=mat('M_1420_LeatherStitch',(.11,.12,.075),rough=.9)
 def rounded_loop(cx,cy,z,w,h,rad):
  pts=[]
  for ax,ay,a0 in [(1,1,0),(-1,1,90),(-1,-1,180),(1,-1,270)]:
   for j in range(7):
    a=math.radians(a0+j*15);pts.append((cx+ax*(w/2-rad)+math.cos(a)*rad,cy+ay*(h/2-rad)+math.sin(a)*rad,z))
  return pts+[pts[0]]
 tube('SeatPiping',rounded_loop(1.55,2.8,.566,.61,.56,.06),.003,green)
 pts=rounded_loop(1.55,.92,0,.61,.75,.06)
 # Convert the 2D loop into the tilted, street-facing backrest seam.
 points=[(p[0],3.07-.086*math.cos(math.radians(-8))-(p[1]-.92)*math.sin(math.radians(-8)),.92-.086*math.sin(math.radians(-8))+(p[1]-.92)*math.cos(math.radians(-8))) for p in pts]
 tube('ChairBackPiping',points,.0033,green)
 for i in range(19):
  z=.60+i*.033;y=2.987+(z-.92)*.14
  cyl('LeatherStitchLeft',(1.27,y,z),(1.27,y,z+.011),.0012,seam,5)
  cyl('LeatherStitchRight',(1.83,y,z),(1.83,y,z+.011),.0012,seam,5)
 for x in [1.38,1.72]:
  for z in [.79,1.09]:cyl('ChairButton',(x,2.970+(z-.92)*.14,z),(x,2.962+(z-.92)*.14,z),.012,green,16)

 # Retain main microphone coordinates. Add its actual suspension, mesh and adjustable joints.
 for x,y,z in [(3.15,1.85,.96),(3.15,1.85,1.64),(2.28,1.45,1.96),(1.72,1.68,1.54)]:
  cyl('BoomPivotPlate',(x,y-.026,z),(x,y+.026,z),.043,dark,24)
  cyl('BoomPivotRivet',(x,y-.033,z),(x,y-.026,z),.015,metal,20)
 for z in [1.32,1.46]:torus('MicShockMount',(1.72,1.68,z),.092,.006,dark)
 for i in range(8):
  a=i*math.tau/8;b=a+.5
  tube('MicElasticSuspension',[(1.72+.09*math.cos(a),1.68+.09*math.sin(a),1.32),(1.72+.067*math.cos(b),1.68+.067*math.sin(b),1.39),(1.72+.09*math.cos(a),1.68+.09*math.sin(a),1.46)],.002,dark)
 # Fine woven microphone grille, not thick silver rings alone.
 for i in range(32):
  a=i*math.tau/32
  pts=[(1.72+.0665*math.cos(a+j*.02),1.68+.0665*math.sin(a+j*.02),1.275+j*.008) for j in range(28)]
  tube('MicWovenGrille',pts,.00135,metal)
 tube('HangingMicCable',[(1.72,1.70,1.54),(1.77,1.76,1.67),(1.90,1.78,1.72),(2.20,1.50,1.98),(2.60,1.62,1.88),(3.18,1.90,1.65),(3.20,1.92,1.08),(3.24,1.94,.88),(3.31,1.90,.83)],.0048,dark)
 spring=[(2.38+i*.014,1.49+.017*math.cos(i*.9),1.92-i*.004+.017*math.sin(i*.9)) for i in range(39)]
 tube('BoomTensionSpring',spring,.0024,metal)
 # Scripts, cassette case and cue marks occupy the visible return rather than hidden rear room.
 cube('CassetteCaseOnReturn',(.27,.76,.885),(.12,.08,.018),dark,.005)
 for x in [.24,.30]:cyl('CassetteReelWindow',(x,.76,.895),(x,.76,.898),.016,ivory,16)
 text('CueCardVisible','1420  /  18:30',(.98,.55,.899),.031,ink,rot=(0,0,0))
 # Recess the practical emitter into its shade; no exposed horizontal glowing rim.
 for o in groups['Interior']:
  if o.name.startswith('Desk_lamp_bulb'):
   o.location.z+=.045;o.scale.x=.64;o.scale.y=.64
 namespace['current']='Mast'
 return rows,{'moved_tape_parts':tape,'console_anchor_unchanged':[1.2,1.53,1.0],'microphone_body_unchanged':[1.72,1.68,1.405],
  'practical_emitter':'Recessed under shade at source (4,1.25,1.505), diameter0.141m. No free sphere in source. Native orb requires runtime light/glass isolation.'}
