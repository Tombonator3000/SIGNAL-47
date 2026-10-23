"""Blender FBX roundtrip checks; does not require Unreal or rendering."""
import bpy
import json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
manifest=json.loads((ROOT/'verification.json').read_text())
report={'status':'PASS','checks':[]}
for name,expected in manifest['meshes'].items():
    bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.fbx(filepath=str(ROOT/'exports'/(name+'.fbx')),use_custom_normals=True)
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    assert len(meshes)==1,(name,len(meshes))
    ob=meshes[0]; ob.data.calc_loop_triangles()
    assert len(ob.data.loop_triangles)==expected['triangles'],name
    degenerate=sum(1 for t in ob.data.loop_triangles
                   if (ob.data.vertices[t.vertices[1]].co-ob.data.vertices[t.vertices[0]].co).cross(
                       ob.data.vertices[t.vertices[2]].co-ob.data.vertices[t.vertices[0]].co).length_squared<1e-20)
    assert degenerate==0,(name,degenerate)
    assert ob.location.length<1e-5,(name,list(ob.location))
    coords=[ob.matrix_world@v.co for v in ob.data.vertices]
    bounds=[[min(v[i] for v in coords) for i in range(3)],[max(v[i] for v in coords) for i in range(3)]]
    error=max(abs(bounds[j][i]-expected['bounds_local_m'][j][i]) for j in range(2) for i in range(3))
    assert error<.0001,(name,error,bounds,expected['bounds_local_m'])
    assert ob.scale.to_tuple()==(1,1,1),(name,list(ob.scale))
    material_names={m.name.split('.')[0] for m in ob.data.materials}
    assert len(material_names)==len(manifest['materials']),(name,material_names)
    report['checks'].append({'mesh':name,'triangles':expected['triangles'],'zero_area_triangles':degenerate,'bounds_max_error_m':error,'pivot_error_m':ob.location.length,'scale':list(ob.scale),'material_slots':len(ob.data.materials)})
(ROOT/'export_roundtrip.json').write_text(json.dumps(report,indent=2)+'\n')
print('ARRAY_ROUNDTRIP_PASS',len(report['checks']))
