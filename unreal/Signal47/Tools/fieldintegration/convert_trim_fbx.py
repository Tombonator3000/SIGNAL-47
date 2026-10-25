"""Convert the verified immutable-index GLBs through our measured FBX axis recipe.
Interchange's glTF build collapses thin retained cable triangles; the legacy FBX path
is checked against the exact retained triangle counts before use.
"""
import bpy,json,os
from pathlib import Path
WORK=Path(os.environ['S47_FIELD_WORK']);source=WORK/'trim';out=source/'fbx';out.mkdir(exist_ok=True)
data=json.loads((source/'field-patches.json').read_text());report=[]
for filename in data['files']:
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 bpy.ops.import_scene.gltf(filepath=str(source/filename),merge_vertices=False)
 for ob in list(bpy.context.scene.objects):
  if ob.type!='MESH':continue
  name=ob.name;assert name in data['meshes'],name
  bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
  bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
  tris=sum(len(p.vertices)-2 for p in ob.data.polygons)
  assert tris==data['meshes'][name]['resultTris'],(name,tris,data['meshes'][name]['resultTris'])
  bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
  bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
  report.append({'name':name,'triangles':tris,'materials':[m.name for m in ob.data.materials]})
(source/'fbx-conversion.json').write_text(json.dumps({'status':'PASS','scope':'Actual GLB import and FBX export, native acceptance separate','meshes':report},indent=2)+'\n')
print('FIELD_TRIM_FBX_PASS',len(report))
