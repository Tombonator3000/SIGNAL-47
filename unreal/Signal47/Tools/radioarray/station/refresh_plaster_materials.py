"""Material-only R5 packing; run only via owned guard after a granted DCC slot."""
import bpy,hashlib,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'1420_AM_station.blend'))
def geometry_hash():
 rows=[]
 for o in sorted(bpy.data.objects,key=lambda ob:ob.name):
  if o.type!='MESH':continue
  m=o.data
  rows.append((o.name,[list(row) for row in o.matrix_basis],[tuple(v.co) for v in m.vertices],
   [(tuple(p.vertices),p.material_index,p.use_smooth) for p in m.polygons],
   [(layer.name,[tuple(d.uv) for d in layer.data]) for layer in m.uv_layers],[s.name for s in m.materials]))
 return hashlib.sha256(json.dumps(rows,separators=(',',':')).encode()).hexdigest()
before=geometry_hash();rows=json.loads((ROOT/'materials.json').read_text())
for img in bpy.data.images:
 if img.source=='FILE':img.reload()
for name in ['M_1420_Stucco_cream','M_1420_WeatheredFacadePaint']:
 mat=bpy.data.materials[name]
 for node in mat.node_tree.nodes:
  if node.type=='NORMAL_MAP':node.inputs['Strength'].default_value=rows[name]['normal_strength']
parts=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.name.startswith('SM_')]
locations={o.name:o.location.copy() for o in parts};bpy.ops.object.select_all(action='DESELECT')
for o in parts:o.select_set(True);o.location-=Vector((0,5,0))
bpy.ops.export_scene.gltf(filepath=str(ROOT/'1420_AM_station.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
for o in parts:o.location=locations[o.name]
bpy.context.view_layer.update();after=geometry_hash();assert before==after,'Material refresh changed geometry/UV/slot/transform'
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'1420_AM_station.blend'))
pre=json.loads((ROOT/'r5_plaster_pre_refresh.json').read_text())
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==v for p,v in pre['fbx_hashes'].items()),'FBX geometry changed'
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==v for p,v in pre['untouched_timber_hashes'].items()),'Timber maps changed'
proof={'revision':'R5-macro-lime-plaster','geometry_hash_before':before,'geometry_hash_after':after,'geometry_identical':before==after,'fbx_byte_identical':True,'timber_byte_identical':True,'normal_strengths':{n:rows[n]['normal_strength'] for n in ['M_1420_Stucco_cream','M_1420_WeatheredFacadePaint']},'scope':'Two plaster PBR families, Blender material nodes and packed GLB only; no geometry rebuild or FBX export.'}
(ROOT/'r5_plaster_refresh.json').write_text(json.dumps(proof,indent=2));print('PLASTER_REFRESH',json.dumps(proof))
