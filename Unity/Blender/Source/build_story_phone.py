"""A separate editable story-phone source; leaves recovered OBJ/FBX families intact."""
import bpy,math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Assets/Signal47/Art/Authored'
OUT.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system='METRIC'
def finish(o,name,bevel=.01):
 o.name=name
 if bevel:
  b=o.modifiers.new('Manufactured radius','BEVEL');b.width=bevel;b.segments=3
  o.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
 return o
def box(name,loc,scale,bevel=.01):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.scale=scale;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);return finish(o,name,bevel)
def tube(name,points,radius=.01,smooth=False):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.resolution_u=8;c.bevel_depth=radius;c.bevel_resolution=2
 s=c.splines.new('BEZIER' if smooth else 'POLY')
 if smooth:
  s.bezier_points.add(len(points)-1)
  for p,v in zip(s.bezier_points,points):p.co=v;p.handle_left_type='AUTO';p.handle_right_type='AUTO'
 else:
  s.points.add(len(points)-1)
  for p,v in zip(s.points,points):p.co=(*v,1)
 o=bpy.data.objects.new(name,c);bpy.context.collection.objects.link(o);bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.convert(target='MESH');o.select_set(False);return o
box('PhoneDarkBase',(0,0,.035),(.65,.55,.07),.025)
verts=[(-.34,-.28,.065),(.34,-.28,.065),(.34,.28,.065),(-.34,.28,.065),(-.34,-.28,.16),(.34,-.28,.16),(.34,.28,.30),(-.34,.28,.30)]
mesh=bpy.data.meshes.new('PhoneShell');mesh.from_pydata(verts,[],[(0,3,2,1),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7),(4,5,6,7)]);mesh.update();o=bpy.data.objects.new('PhoneCreamShell',mesh);bpy.context.collection.objects.link(o);finish(o,o.name,.025)
# Key tops follow the sloping surface. Legends remain editable text in .blend, meshes in FBX.
for row in range(4):
 for col in range(3):
  x=(col-1)*.075;y=-.20+row*.072;z=.16+(y+.28)*.25
  key=box(f'Phone_Key_{row}{col}',(x,y,z+.015),(.06,.06,.028),.006);key.rotation_euler.x=math.atan(.25)
  bpy.ops.object.text_add(location=(x,y-.012,z+.033));t=bpy.context.object;t.name=f'PhoneLegend_{row}{col}';t.data.body=['*0#','789','456','123'][row][col];t.data.align_x='CENTER';t.data.size=.033;t.rotation_euler.x=math.atan(.25)
for x in (-.21,.21):box('PhoneCreamCradle',(x,.155,.315),(.045,.11,.055),.01)
tube('PhoneHandsetCream',[(-.29,.15,.32),(-.22,.15,.405),(0,.15,.43),(.22,.15,.405),(.29,.15,.32)],.046,True)
for x in (-.27,.27):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=10,location=(x,.15,.325));o=bpy.context.object;o.scale=(.108,.085,.05);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);finish(o,'PhoneHandsetCreamEar',0)
points=[(-.33,.15,.325)]
for i in range(321):
 t=i/320;theta=t*math.tau*20;points.append((-.395+.027*math.cos(theta),.14-t*.32,.13+.027*math.sin(theta)))
points.append((-.335,-.18,.13));tube('PhoneCable',points,.007)
# A shallow seam gives a readable housing even in the low-light room.
box('PhoneDarkSeam',(0,-.283,.08),(.59,.007,.015),.002)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Blender/SIGNAL47_story_phone.blend'))
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.fbx(filepath=str(OUT/'SM_StoryPhone.fbx'),use_selection=True,object_types={'MESH','OTHER'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True,use_mesh_modifiers=True)
print('STORY_PHONE_EXPORT',OUT/'SM_StoryPhone.fbx')
