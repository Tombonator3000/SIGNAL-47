"""Root-run optional actual model preview; one small CPU image, no source mutation."""
import bpy,sys,math,os
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,str(Path(__file__).resolve().parent));from design import WORK
bpy.ops.wm.open_mainfile(filepath=str(WORK/'SaroFleet.blend'))
for o in bpy.context.scene.objects:
 if o.name.startswith('SM_SaroPark_'):o.hide_render=True
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=12;scene.render.resolution_x=1024;scene.render.resolution_y=720;scene.render.resolution_percentage=100
scene.world.color=(.14,.14,.14)
for pos,power,size in [((3,2,6),900,5),((-4,-3,4),600,4)]:
 bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(-5,5.6,3.2));c=bpy.context.object;c.rotation_euler=(Vector((0,0,1))-c.location).to_track_quat('-Z','Y').to_euler();c.data.lens=48;scene.camera=c;scene.view_settings.view_transform='AgX';scene.render.filepath=os.environ.get('S47_FLEET_PREVIEW',str(WORK/'service-exterior.png'));bpy.ops.render.render(write_still=True)
