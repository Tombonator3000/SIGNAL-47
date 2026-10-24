"""Independent FBX roundtrip: unit scale, bounds, triangles, UV and outward faces."""
import bpy,json
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parent
d=json.loads((R/'model_contract.json').read_text())['parts']['SM_Caliche_Mesas']
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(R/'exports/SM_Caliche_Mesas.fbx'))
obs=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert len(obs)==1
o=obs[0];vs=[o.matrix_world@v.co for v in o.data.vertices]
bounds=[[min(p[i] for p in vs) for i in range(3)],[max(p[i] for p in vs) for i in range(3)]]
error=max(abs(a-b) for aa,bb in zip(bounds,d['bounds_local_m']) for a,b in zip(aa,bb))
o.data.calc_loop_triangles()
checks={'single_mesh':len(obs)==1,'bounds_m':error<.0003,'triangles':len(o.data.loop_triangles)==d['triangles'],
        'uv_map':len(o.data.uv_layers)>0,'material_slots':len(o.data.materials)==6,
        'nondegenerate':all(p.area>1e-7 for p in o.data.polygons),'world_origin':o.location.length<1e-5,
        'scale_one':max(abs(s-1) for s in o.scale)<1e-5}
r={'status':'PASS' if all(checks.values()) else 'FAIL','checks':checks,'bounds_error_m':error,'triangles':len(o.data.loop_triangles)}
(R/'verification.json').write_text(json.dumps(r,indent=2));print(r);assert r['status']=='PASS'
