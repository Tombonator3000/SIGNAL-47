"""Opt-in source previews only. Run after the parent grants a graphics-queue slot.
blender -b -t 2 --factory-startup --python render_previews.py
No geometry exports are rewritten and no Unreal process is launched.
"""
import bpy,json
from pathlib import Path
ROOT=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Saucer_Crash_1947.blend'))
scene=bpy.context.scene; scene.render.threads_mode='FIXED'; scene.render.threads=2
if 'PREVIEW_neutral_ground' not in bpy.data.objects:
 bpy.ops.mesh.primitive_plane_add(size=100,location=(0,0,-.47)); o=bpy.context.object; o.name='PREVIEW_neutral_ground'
 m=bpy.data.materials.new('PREVIEW_neutral_stage'); m.use_nodes=True
 p=m.node_tree.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value=(.12,.095,.068,1); p.inputs['Roughness'].default_value=1
 o.data.materials.append(m)
for name,cam in [('exterior','CAM_Exterior'),('interior','CAM_Interior'),('street_window','CAM_StreetWindow')]:
 scene.camera=bpy.data.objects[cam]; scene.render.filepath=str(ROOT/(name+'.png')); bpy.ops.render.render(write_still=True)
c=json.loads((ROOT/'model_contract.json').read_text()); c['preview_status']='Blender previews rendered; visual inspection and native UE street review are separate checks.'
(ROOT/'model_contract.json').write_text(json.dumps(c,indent=2))
