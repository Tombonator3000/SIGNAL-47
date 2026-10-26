"""Triangulate export meshes exactly as Blender evaluates them; remove area=0 only.

UVs, material slot order, original vertices, smooth flags and corner normals remain.
The editable source collection is intentionally independent of these export meshes.
"""
import bpy, hashlib, json
from collections import Counter

def surface_hash(mesh):
 mesh.calc_loop_triangles()
 visible=[]
 for tri in mesh.loop_triangles:
  if tri.area==0: continue
  visible.append((tri.material_index,mesh.polygons[tri.polygon_index].use_smooth,
                  [tuple(mesh.vertices[i].co) for i in tri.vertices],
                  [[tuple(layer.data[i].uv) for i in tri.loops] for layer in mesh.uv_layers]))
 return hashlib.sha256(json.dumps(visible,separators=(',',':')).encode()).hexdigest()

def clean_triangulated_mesh(obj):
 old=obj.data; old.calc_loop_triangles()
 before_count=len(old.loop_triangles); before_hash=surface_hash(old)
 bad=[t for t in old.loop_triangles if t.area==0]
 removed_materials=Counter(old.materials[t.material_index].name for t in bad)
 triangles=[t for t in old.loop_triangles if t.area>0]
 corners=[[tuple(old.corner_normals[i].vector) for i in t.loops] for t in triangles]
 uv_data={layer.name:[[tuple(layer.data[i].uv) for i in t.loops] for t in triangles] for layer in old.uv_layers}
 vertices=[tuple(v.co) for v in old.vertices]
 faces=[tuple(t.vertices) for t in triangles]
 materials=list(old.materials)
 face_slots=[t.material_index for t in triangles]
 smooth=[old.polygons[t.polygon_index].use_smooth for t in triangles]
 new=bpy.data.meshes.new(old.name+'_CleanTriangles')
 new.from_pydata(vertices,[],faces)
 for material in materials: new.materials.append(material)
 for p,slot,flag in zip(new.polygons,face_slots,smooth): p.material_index=slot; p.use_smooth=flag
 for name,rows in uv_data.items():
  layer=new.uv_layers.new(name=name)
  for poly,row in zip(new.polygons,rows):
   for loop,uv in zip(poly.loop_indices,row): layer.data[loop].uv=uv
 new.normals_split_custom_set([normal for tri in corners for normal in tri])
 new.update()
 obj.data=new
 new.calc_loop_triangles()
 after_normals=[tuple(new.corner_normals[i].vector) for tri in new.loop_triangles for i in tri.loops]
 normal_error=max(abs(a[i]-b[i]) for a,b in zip([n for row in corners for n in row],after_normals) for i in range(3))
 after_hash=surface_hash(new)
 proof={'before_triangles':before_count,'removed_exact_zero_area':len(bad),
        'removed_by_material':dict(removed_materials),'after_triangles':len(new.loop_triangles),
        'visible_surface_hash_before':before_hash,'visible_surface_hash_after':after_hash,
        'visible_surface_identical':before_hash==after_hash,
        'vertices_retained':len(vertices)==len(new.vertices),'material_slots_identical':[m.name for m in materials]==[m.name for m in new.materials],
        'corner_normal_max_component_delta':normal_error,
        'all_faces_triangles':all(len(p.vertices)==3 for p in new.polygons),
        'remaining_zero_area':sum(t.area==0 for t in new.loop_triangles)}
 if not proof['visible_surface_identical'] or proof['remaining_zero_area'] or not proof['material_slots_identical'] or normal_error>.001:
  raise RuntimeError('Export triangulation changed authored surface: '+obj.name)
 if old.users==0: bpy.data.meshes.remove(old)
 return proof
