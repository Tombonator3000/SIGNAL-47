"""One CPU-only source inspection; does not modify authored blend or exports."""
import bpy
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Caliche_Ground.blend'))
s=bpy.context.scene;s.render.engine='CYCLES';s.cycles.device='CPU';s.cycles.samples=20;s.cycles.use_denoising=True
s.render.resolution_x=1280;s.render.resolution_y=800;s.render.resolution_percentage=100
s.view_settings.view_transform='AgX'
s.world.use_nodes=True
bg=s.world.node_tree.nodes.get('Background');bg.inputs['Color'].default_value=(.23,.28,.35,1);bg.inputs['Strength'].default_value=.65
d=bpy.data.lights.new('PreviewSun','SUN');d.energy=3;d.angle=.05
sun=bpy.data.objects.new('PreviewSun',d);bpy.context.collection.objects.link(sun);sun.rotation_euler=(.43,-.5,-.8)
d=bpy.data.cameras.new('GroundInspection');cam=bpy.data.objects.new('GroundInspection',d);bpy.context.collection.objects.link(cam);s.camera=cam;d.lens=31
cam.location=(17,20,6.1);cam.rotation_euler=(Vector((23,10,4.8))-cam.location).to_track_quat('-Z','Y').to_euler()
s.render.filepath=str(ROOT/'ground_frontage_v3.png');bpy.ops.render.render(write_still=True)
print('GROUND_SOURCE_PREVIEW_COMPLETE')
