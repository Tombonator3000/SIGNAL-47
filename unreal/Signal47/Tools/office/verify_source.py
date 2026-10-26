"""Reopen the authored .blend and validate delivered topology and metric placement."""
from pathlib import Path
import bpy, json
ROOT=Path(__file__).resolve().parents[3]/'ArtSource/Office'
contract=json.loads((ROOT/'model_contract.json').read_text());checks=[]
for name,row in contract['parts'].items():
    ob=bpy.data.objects.get(name);assert ob and ob.type=='MESH',name
    assert len(ob.data.polygons)==row['triangles'],name
    assert all(len(p.vertices)==3 for p in ob.data.polygons),name
    assert all(p.area>1e-12 for p in ob.data.polygons),name
    assert len(ob.data.uv_layers)>0,name
    assert set(m.name for m in ob.data.materials)==set(row['materials']),name
    assert all(abs(v)<1e-6 for v in ob.location),name
    assert (ROOT/'Export'/(name+'.fbx')).stat().st_size>1000,name
    checks.append({'mesh':name,'triangles':len(ob.data.polygons),'components':row['original_components']})
assert len(checks)==8
assert all((ROOT/row['textures']['base_color']).is_file() for row in json.loads((ROOT/'materials.json').read_text()))
report={'status':'PASS','scope':'Blender source geometry, UVs, material and metric pivot contract','parts':checks}
(ROOT/'source-verification.json').write_text(json.dumps(report,indent=2)+'\n');print('OFFICE SOURCE VERIFY PASS',json.dumps(report))
