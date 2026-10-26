"""Read-only source/FBX degeneracy inventory; does not save or export model assets."""
import bpy,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'1420_AM_station.blend'))
rows=[]
for o in bpy.data.objects:
 if o.type!='MESH' or not o.name.startswith('SRC_'):continue
 o.data.calc_loop_triangles();tri=[]
 for t in o.data.loop_triangles:
  p=[o.matrix_basis@o.data.vertices[i].co for i in t.vertices]
  area=(p[1]-p[0]).cross(p[2]-p[0]).length/2
  edge=min((p[i]-p[(i+1)%3]).length for i in range(3))
  if area<1e-7 or edge<2e-4:tri.append({'area':area,'short_edge_m':edge,'vertices':[list(q) for q in p]})
 if tri:rows.append({'object':o.name,'count':len(tri),'exact_zero':sum(t['area']==0 for t in tri),'triangles':tri})
o=bpy.data.objects['SM_1420_Interior'];o.data.calc_loop_triangles();small=[]
for t in o.data.loop_triangles:
 p=[o.data.vertices[i].co for i in t.vertices];area=(p[1]-p[0]).cross(p[2]-p[0]).length/2;edge=min((p[i]-p[(i+1)%3]).length for i in range(3))
 if area<1e-7 or edge<2e-4:small.append({'index':t.index,'area':area,'short_edge_m':edge,'material':o.data.materials[t.material_index].name,'vertices':[list(q) for q in p]})
report={'source_objects':rows,'export_suspect_triangles':small}
(ROOT/'r3_degenerate_diagnosis.json').write_text(json.dumps(report,indent=2))
print('DIAGNOSIS',json.dumps({'source_objects':[(r['object'],r['count'],r['exact_zero']) for r in rows],'export_count':len(small),'smallest_export':sorted(small,key=lambda x:x['area'])[:12]}))
