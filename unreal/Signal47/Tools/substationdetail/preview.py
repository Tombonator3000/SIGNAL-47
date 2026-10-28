"""Authoring preview with existing original kit as context. Not game lighting/terrain."""
import bpy,json,math
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(R/'Substation_Detail.blend'))
scene=bpy.context.scene
context=bpy.data.collections.new('PREVIEW_EXISTING_ORIGINAL_KIT');scene.collection.children.link(context)
for folder,filename in [('substation','Caliche_Junction_Substation.blend'),('substationsite','Substation_Site.blend')]:
 src=R.parent/folder/filename
 contract=json.loads((R.parent/folder/'model_contract.json').read_text())
 with bpy.data.libraries.load(str(src),link=False) as (a,b):b.objects=[n for n in contract['parts'] if n in a.objects]
 for ob in b.objects:
  if ob:context.objects.link(ob);ob.hide_render=False;ob.hide_set(False)
# Context plane is explicitly a studio background, never exported or used for placement.
bpy.ops.mesh.primitive_plane_add(size=250,location=(0,0,-.72));floor=bpy.context.object;floor.name='PREVIEW_STUDIO_FLOOR'
mat=bpy.data.materials.new('PREVIEW_DUST_BACKGROUND');mat.diffuse_color=(.19,.14,.095,1);mat.use_nodes=True;mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.19,.14,.095,1);mat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.93;floor.data.materials.append(mat)
scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.50,.60,.72,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.45
bpy.ops.object.light_add(type='SUN',location=(3,-7,15));sun=bpy.context.object;sun.rotation_euler=(math.radians(27),math.radians(-18),math.radians(-28));sun.data.energy=2.8;sun.data.angle=.08
bpy.ops.object.camera_add();camera=bpy.context.object;scene.camera=camera
scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=True;scene.render.resolution_x=1400;scene.render.resolution_y=900;scene.render.resolution_percentage=100
out=R/'Preview';out.mkdir(exist_ok=True)
for name,at,target,lens in [('service',(7.4,-12.1,2.8),(4.90,-8.60,1.10),55),('site',(18,-25,12),(0,0,1.2),48)]:
 camera.location=at;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.lens=lens;scene.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
print('SUBSTATION_DETAIL_PREVIEW_COMPLETE')
