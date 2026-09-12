"""Original SARO antenna detail kit; Blender 4.5, metres, Unity -Z/Y FBX.
Existing 4.2m-radius parabola and animation pivots are retained. NASA/NRAO
are engineering references only; no third-party geometry enters these exports.
"""
import bpy,math,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Assets/Signal47/Art/Resources19';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system='METRIC'
bpy.context.preferences.filepaths.save_version=0
materials={}
for n,c in [('Steel',(.27,.31,.29,1)),('Enamel',(.60,.63,.57,1)),('Seam',(.29,.33,.30,1))]:
 m=bpy.data.materials.new('R19_'+n);m.diffuse_color=c;materials[n]=m
# Design in Unity coordinates; convert to Blender's Z-up coordinates.
def point(v):return Vector((v[0],-v[2],v[1]))
def beam(n,a,b,r=.035,mat='Steel',sides=8):
 a,b=point(a),point(b);delta=b-a
 bpy.ops.mesh.primitive_cylinder_add(vertices=sides,radius=r,depth=delta.length,location=(a+b)/2)
 o=bpy.context.object;o.name=n;o.rotation_euler=delta.to_track_quat('Z','Y').to_euler();o.data.materials.append(materials[mat]);return o

def box(n,p,s,mat='Steel'):
 bpy.ops.mesh.primitive_cube_add(size=1,location=point(p));o=bpy.context.object;o.name=n;o.scale=(s[0],s[2],s[1]);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(materials[mat]);return o

def ring(n,r,y,tube,mat='Steel',segments=64):
 for i in range(segments):
  a=math.tau*i/segments;b=math.tau*(i+1)/segments
  beam(n,(r*math.cos(a),y,r*math.sin(a)),(r*math.cos(b),y,r*math.sin(b)),tube,mat,6)

def radial(r,a,offset):return(r*math.cos(a),r*r/12+offset,r*math.sin(a))
models=[]
def export(name,objects):
 # Consolidate by material for few renderers; keep UVs and normals in FBX.
 meshes=[]
 groups=[[o for o in objects if o.data.materials[0]==m] for m in materials.values()]
 for group in groups:
  if not group:continue
  bpy.ops.object.select_all(action='DESELECT')
  for o in group:o.select_set(True)
  bpy.context.view_layer.objects.active=group[0];bpy.ops.object.join();o=bpy.context.object;o.name=name+'_'+o.data.materials[0].name
  bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
  bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.normals_make_consistent(inside=False);bpy.ops.uv.smart_project(island_margin=.02);bpy.ops.object.mode_set(mode='OBJECT')
  meshes.append(o)
 bpy.ops.object.select_all(action='DESELECT')
 for o in meshes:o.select_set(True)
 bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True)
 tris=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in meshes)
 assert tris<16000
 models.append({'name':name,'triangles':tris,'renderers':len(meshes)})
 return meshes

for i in range(16):
 a=math.tau*i/16
 for r0,r1 in zip([.55,1.4,2.3,3.2],[1.4,2.3,3.2,4.16]):
  beam('RadialBackRib',radial(r0,a,-.17),radial(r1,a,-.17),.040)
 # Tapered triangulated backing, visible along silhouette and from the rear.
 beam('BackTruss',radial(.65,a,-.48),radial(3.2,a,-.17),.035)
 beam('InnerBrace',radial(.65,a,-.48),radial(.65,a,-.17),.035)
 for r0,r1 in zip([.45,1.25,2.2,3.15],[1.25,2.2,3.15,4.16]):
  beam('PanelJoint',radial(r0,a,.009),radial(r1,a,.009),.008,'Seam',4)
ring('EdgeRim',4.2,1.47,.045,'Enamel')
for r in [1.25,2.2,3.15]:ring('PanelRing',r,r*r/12+.009,.008,'Seam',64)
ring('BackingRing',2.3,2.3**2/12-.17,.04)
bowl=export('R19_BowlDetail',list(bpy.context.scene.objects))
for o in bowl:o.hide_set(True)
start=set(bpy.context.scene.objects)
# Stationary maintenance fixtures fit the existing pedestal and base footprint.
for x in [-.22,.22]:beam('LadderRail',(x,.26,-.56),(x,2.05,-.56),.023)
for i in range(8):beam('LadderRung',(-.22,.29+i*.23,-.56),(.22,.29+i*.23,-.56),.020)
box('MotorCabinet',(.66,.78,0),(.42,.58,.46),'Enamel')
box('CabinetDoor',(.66,.78,.24),(.35,.47,.025))
for i in range(6):box('VentSlot',(.66,.64+i*.043,.257),(.24,.012,.008),'Seam')
beam('DriveConduit',(.65,.48,0),(.45,.32,0),.033)
# Bearing caps belong to the elevation/azimuth housing; no new collision geometry.
for x in [-.75,.75]:
 beam('BearingCap',(x-.07,1.65,0),(x+.07,1.65,0),.26,'Steel',32)
 for i in range(8):
  a=math.tau*i/8;y=1.65+.20*math.cos(a);z=.20*math.sin(a)
  beam('BearingBolt',(x-.082,y,z),(x+.082,y,z),.027,'Enamel',6)
base=export('R19_PedestalDetail',[o for o in bpy.context.scene.objects if o not in start])
for o in bowl:o.hide_set(False)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Blender/SIGNAL47_resources19_dish.blend'))
(OUT/'source-manifest.json').write_text(json.dumps({'original_geometry':True,'bowl_radius_m':4.2,'models':models},indent=2)+'\n')
print('RESOURCES19_EXPORT',json.dumps(models))
