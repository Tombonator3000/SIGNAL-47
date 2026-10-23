"""FBX roundtrip and ground contract checks. Run in Blender after generation."""
import bpy
import json
from pathlib import Path
import sys
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT))
from terrain import clear_dressing, ANCHOR
contract=json.loads((ROOT/'model_contract.json').read_text())
report={'status':'PASS','parts':{},'checks':[]}
for name,row in contract['parts'].items():
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.fbx(filepath=str(ROOT/'exports'/(name+'.fbx')))
    objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
    assert len(objects)==1,(name,len(objects))
    ob=objects[0];me=ob.data;me.calc_loop_triangles()
    assert len(me.loop_triangles)==row['triangles'],name
    assert ob.location.length<1e-6 and (ob.scale-Vector((1,1,1))).length<1e-6,name
    assert len(me.uv_layers)==1 and len(me.materials)==len(row['material_slots']),name
    actual=[[min(v.co[i] for v in me.vertices) for i in range(3)],[max(v.co[i] for v in me.vertices) for i in range(3)]]
    error=max(abs(actual[j][i]-row['bounds_local_m'][j][i]) for i in range(3) for j in range(2))
    assert error<1e-4,(name,error)
    zero=sum((me.vertices[t.vertices[1]].co-me.vertices[t.vertices[0]].co).cross(me.vertices[t.vertices[2]].co-me.vertices[t.vertices[0]].co).length_squared<1e-20 for t in me.loop_triangles)
    assert zero==0,(name,zero)
    report['parts'][name]={'triangles':len(me.loop_triangles),'bounds_error_m':error,'zero_area_triangles':zero,'uv_layers':len(me.uv_layers),'material_slots':len(me.materials)}
for row in contract['dressing']:
    assert clear_dressing(*row['xz'],row['radius_m']),row
report['checks'].append({'name':'rocks_and_plants_clear_main_walk_and_protected_approaches','pass':True,'count':len(contract['dressing'])})
samples=json.loads((ROOT/'surface_samples.json').read_text())
assert all(0<=r['above_source_m']<=.014 for r in samples)
report['checks'].append({'name':'all_overlay_vertices_max_14mm_above_unchanged_source_surface','pass':True,'count':len(samples),'maximum_m':max(r['above_source_m'] for r in samples)})
for mat in json.loads((ROOT/'materials.json').read_text()):
    for filename in mat['textures'].values():assert (ROOT/filename).is_file(),filename
report['checks'].append({'name':'all_material_texture_files_present','pass':True})
(ROOT/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('CALICHE_GROUND_VERIFIED',json.dumps(report['parts']))
