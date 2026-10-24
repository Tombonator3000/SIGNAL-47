"""Source look-dev only. Run through run_guarded.py render, never alongside Unreal."""
import bpy,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'1420_AM_station.blend'))
s=bpy.context.scene;s.render.threads_mode='FIXED';s.render.threads=2;s.cycles.samples=24
s.render.resolution_x=1280;s.render.resolution_y=960;s.render.resolution_percentage=100
ext=bpy.data.objects['CAM_Exterior'];ext.location=(-8,-15,2.8);ext.rotation_euler=(Vector((-.2,1,1.60))-ext.location).to_track_quat('-Z','Y').to_euler();ext.data.lens=35
inside=bpy.data.objects['CAM_Interior'];inside.location=(4.4,4.7,1.9);inside.rotation_euler=(Vector((1.0,1.0,1.12))-inside.location).to_track_quat('-Z','Y').to_euler();inside.data.lens=26
data=bpy.data.cameras.new('CAM_StreetWindow');data.lens=31;street=bpy.data.objects.new('CAM_StreetWindow',data);s.collection.objects.link(street)
street.location=(-.8,-4.1,1.65);street.rotation_euler=(Vector((1.1,1.1,1.3))-street.location).to_track_quat('-Z','Y').to_euler()
for name,cam in [('exterior',ext),('interior',inside),('street-window',street)]:
 s.camera=cam;s.render.filepath=str(ROOT/('r2-'+name+'.png'));bpy.ops.render.render(write_still=True)
print('R2_RENDER_COMPLETE')
