"""Verify saved native meshes in a fresh editor process after import compilation."""
import json
import os
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parent
OUT = Path(os.environ['S47_STREET_VERIFY'])
E = unreal.EditorAssetLibrary
checks = []


def check(name, value, detail=None):
    checks.append({'name':name, 'pass':bool(value), 'detail':detail})


for label in ('souvenir','ground'):
    parts = json.loads((ROOT/label/'model_contract.json').read_text())['parts']
    for name,row in parts.items():
        mesh = E.load_asset('/Game/CalicheStreet/'+label.title()+'/'+name)
        check(name+' imported', isinstance(mesh,unreal.StaticMesh))
        if not isinstance(mesh,unreal.StaticMesh): continue
        bounds = row['bounds_local_m']
        center = [(bounds[0][i]+bounds[1][i])*50 for i in range(3)]
        center[1] *= -1
        size = [(bounds[1][i]-bounds[0][i])*100 for i in range(3)]
        b = mesh.get_bounds()
        actual = [b.origin.x,b.origin.y,b.origin.z,2*b.box_extent.x,2*b.box_extent.y,2*b.box_extent.z]
        check(name+' scale, orientation, pivot',max(abs(a-b) for a,b in zip(actual,center+size))<.12,
              {'expected':center+size,'actual':actual})
        check(name+' triangle count',mesh.get_num_triangles(0)==row['triangles'],
              {'expected':row['triangles'],'actual':mesh.get_num_triangles(0)})
        check(name+' material slots',all(x.material_interface and x.material_interface.get_path_name().startswith('/Game/CalicheStreet/') for x in mesh.get_editor_property('static_materials')))
        check(name+' Nanite disabled',not mesh.get_editor_property('nanite_settings').enabled)

patches=json.loads((Path(unreal.Paths.project_content_dir())/'CalicheStreet/street-patches.json').read_text())
for name,row in patches['meshes'].items():
    if row['hidden']:continue
    mesh=E.load_asset(row['asset'])
    check(name+' composed trim triangles',isinstance(mesh,unreal.StaticMesh) and mesh.get_num_triangles(0)==row['resultTris'])
check('souvenir geometry removed',sum(v['souvenirRemoved'] for v in patches['meshes'].values())>30)
M=unreal.MaterialEditingLibrary
for label in ('souvenir','ground'):
    rows=json.loads((ROOT/label/'materials.json').read_text())
    if isinstance(rows,dict):rows=[dict(row,name=name) for name,row in rows.items()]
    for row in rows:
        mat=E.load_asset('/Game/CalicheStreet/'+label.title()+'/Materials/'+row['name'])
        if row.get('textures',{}).get('normal'):
            value=M.get_material_default_scalar_parameter_value(mat,'NormalStrength')
            check(row['name']+' controlled normal strength',abs(value-float(row.get('normal_strength',.2)))<.00001,value)
        check(row['name']+' sidedness',mat.get_editor_property('two_sided')==bool(row.get('two_sided',False)))
for name in ('M_1420_Stucco_cream','M_1420_Concrete'):
    mat=E.load_asset('/Game/RadioArray/Station/Materials/'+name)
    check(name+' corrected original finish',isinstance(mat,unreal.Material) and abs(M.get_material_default_scalar_parameter_value(mat,'NormalStrength')-.18)<.00001)
result={'status':'PASS' if all(c['pass'] for c in checks) else 'FAIL','checks':checks,
        'engine':unreal.SystemLibrary.get_engine_version()}
OUT.write_text(json.dumps(result,indent=2))
assert result['status']=='PASS',[c for c in checks if not c['pass']]
unreal.log('CALICHE STREET VERIFY PASS '+str(len(checks)))
