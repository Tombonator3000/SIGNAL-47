"""Face the one-sided CRT toward the viewer, including the editable Blender source."""
import bpy,bmesh,json,os
from pathlib import Path
R=Path(os.environ.get('S47_KELVORA_SOURCE', str(Path.home()/'Desktop/Retro TV og radio')))
bpy.ops.wm.open_mainfile(filepath=str(R/'Kelvora_Retro.blend'))
o=bpy.data.objects['SM_Kelvora_TV'];bm=bmesh.new();bm.from_mesh(o.data);faces=[f for f in bm.faces if f.material_index==1]
assert faces
bmesh.ops.reverse_faces(bm,faces=[f for f in faces if f.normal.y>0])
bm.to_mesh(o.data);bm.free();o.data.update()
assert max(p.normal.y for p in o.data.polygons if p.material_index==1)<-.3
# Original component, where retained in the editable collection.
for ob in bpy.data.objects:
 if ob.name.startswith('TV_Video_Surface') and ob.type=='MESH':
  bm=bmesh.new();bm.from_mesh(ob.data)
  bmesh.ops.reverse_faces(bm,faces=[f for f in bm.faces if f.normal.y>0])
  bm.to_mesh(ob.data);bm.free()
bpy.ops.wm.save_as_mainfile(filepath=str(R/'Kelvora_Retro.blend'))
loc=o.location.copy();o.location=(0,0,0);o.hide_set(False);bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
bpy.ops.export_scene.gltf(filepath=str(R/'SM_Kelvora_TV.glb'),export_format='GLB',use_selection=True,export_animations=False)
print('CRT OUTWARD NORMAL PASS')
