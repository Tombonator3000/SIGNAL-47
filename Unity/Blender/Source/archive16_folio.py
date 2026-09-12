"""Original archive folio, metres; Blender CLI -> editable source -> FBX -> Unity."""
import bpy, json, math
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[2]; out=root/'Assets/Signal47/Art/Archive16';out.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system='METRIC'
colors={'Card':(.43,.32,.15,1),'Paper':(.83,.78,.61,1),'Tab':(.42,.16,.09,1),'Clip':(.22,.25,.23,1)}
mats={}
for name,col in colors.items():
 m=bpy.data.materials.new('A16_'+name);m.diffuse_color=col;m.use_nodes=True
 m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=col
 m.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.8
 mats[name]=m
parts=[]
def box(name,loc,size,mat):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.scale=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 o.data.materials.append(mats[mat]);b=o.modifiers.new('Soft card edge','BEVEL');b.width=.0008;b.segments=2
 bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=b.name)
 parts.append(o);return o
box('Back cover',(0,0,.002),(.46,.34,.004),'Card')
for i in range(4):box('Retained paper '+str(i),(.005-i*.001,0,.0045+i*.0012),(.424,.298,.0008),'Paper')
box('Cover',(0,0,.011),(.46,.34,.002),'Card')
box('Index tab',(.135,.182,.011),(.105,.035,.002),'Tab')
box('Label field',(-.055,-.02,.0125),(.27,.11,.0005),'Paper')
for x in [-.185,-.165]:box('Archive fastener',(x,.115,.014),(.012,.057,.004),'Clip')
source=root/'Blender/SIGNAL47_archive16_folio.blend';bpy.context.preferences.filepaths.save_version=0
bpy.ops.wm.save_as_mainfile(filepath=str(source))
bpy.ops.object.select_all(action='DESELECT')
for o in parts:o.select_set(True)
bpy.ops.export_scene.fbx(filepath=str(out/'A16_Folio.fbx'),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True)
coords=[o.matrix_world@Vector(c) for o in parts for c in o.bound_box];dims=[max(c[i] for c in coords)-min(c[i] for c in coords) for i in range(3)]
triangles=sum(len(p.vertices)-2 for o in parts for p in o.data.polygons)
for o in parts:o.select_set(False)
bpy.ops.import_scene.fbx(filepath=str(out/'A16_Folio.fbx'));copies=list(bpy.context.selected_objects)
coords2=[o.matrix_world@Vector(c) for o in copies for c in o.bound_box];dims2=[max(c[i] for c in coords2)-min(c[i] for c in coords2) for i in range(3)]
assert all(abs(a-b)<1e-5 for a,b in zip(dims,dims2))
(out/'source-manifest.json').write_text(json.dumps({'source':'Unity/Blender/SIGNAL47_archive16_folio.blend','recipe':'Unity/Blender/Source/archive16_folio.py','blender':bpy.app.version_string,'authored_original':True,'units':'metres','dimensions_blender_xyz':dims,'triangles':triangles,'fbx_roundtrip_bounds':'PASS'},indent=2)+'\n')
for o in copies:bpy.data.objects.remove(o,do_unlink=True)
bpy.ops.object.camera_add(location=(.52,-.6,.6));camera=bpy.context.object;camera.rotation_euler=(Vector((0,0,0))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=.7
scene=bpy.context.scene;scene.camera=camera;scene.world=bpy.data.worlds.new('Archive study world');scene.world.color=(.15,.15,.15)
bpy.ops.object.light_add(type='AREA',location=(.2,-.3,1));bpy.context.object.data.energy=65;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=1
scene.render.engine='CYCLES';scene.cycles.samples=16;scene.render.resolution_x=720;scene.render.resolution_y=500;scene.render.resolution_percentage=100
scene.render.filepath=str(root.parent/'Artifacts/Archive16/folio-authoring.png');bpy.ops.render.render(write_still=True)
