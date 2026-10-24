"""Reload the finer facade paint and repack GLB without touching any geometry."""
import bpy, hashlib, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'1420_AM_station.blend'))

def geometry_hash():
 rows=[]
 for o in sorted(bpy.data.objects,key=lambda ob:ob.name):
  if o.type!='MESH': continue
  m=o.data
  rows.append((o.name,[list(row) for row in o.matrix_world],[tuple(v.co) for v in m.vertices],
   [(tuple(p.vertices),p.material_index,p.use_smooth) for p in m.polygons],
   [(layer.name,[tuple(d.uv) for d in layer.data]) for layer in m.uv_layers],
   [slot.name for slot in m.materials]))
 return hashlib.sha256(json.dumps(rows,separators=(',',':')).encode()).hexdigest()

bpy.context.view_layer.update();before=geometry_hash()
for img in bpy.data.images:
 if img.source=='FILE':img.reload()
parts=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.name.startswith('SM_')]
locations={o.name:o.location.copy() for o in parts}
bpy.ops.object.select_all(action='DESELECT')
for o in parts:o.select_set(True);o.location-=Vector((0,5,0))
bpy.ops.export_scene.gltf(filepath=str(ROOT/'1420_AM_station.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
for o in parts:o.location=locations[o.name]
bpy.context.view_layer.update();after=geometry_hash()
assert before==after,'Texture refinement changed geometry, UVs, materials or pivots'
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'1420_AM_station.blend'))
proof={'source_revision':'R2-concept-match-2026-10-24','geometry_hash_before':before,'geometry_hash_after':after,'geometry_unchanged':before==after,'scope':'Sparse small paint chips replace coarse letter losses. All image data reloaded and GLB repacked. FBX unchanged.'}
(ROOT/'r2_material_refinement.json').write_text(json.dumps(proof,indent=2))
print('R2_MATERIAL_REFINEMENT',json.dumps(proof))
