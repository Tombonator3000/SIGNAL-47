"""Verify saved native meshes in a fresh editor process after import compilation."""
import json
import os
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parent
OUT = Path(os.environ['S47_STREET_VERIFY'])
E = unreal.EditorAssetLibrary
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
checks = []


def check(name, value, detail=None):
    checks.append({'name':name, 'pass':bool(value), 'detail':detail})


for label in ('souvenir','ground','backdrop'):
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
        expected_materials=row.get('used_materials',row.get('materials',row.get('material_slots')))
        actual_materials=[mesh.get_material(S.get_lod_material_slot(mesh,0,i)).get_name() for i in range(mesh.get_num_sections(0))]
        check(name+' source materials used by real sections',set(actual_materials)==set(expected_materials),{'expected':expected_materials,'actual':actual_materials})
        check(name+' Nanite disabled',not mesh.get_editor_property('nanite_settings').enabled)
        if label=='ground':
            data=mesh.get_editor_property('asset_import_data')
            check(name+' authored vertex colors imported',data.get_editor_property('vertex_color_import_option')==unreal.VertexColorImportOption.REPLACE)
            if name in ('SM_Caliche_Surface','SM_Caliche_RoadWear'):
                check(name+' real perimeter vertex colors retained',S.has_vertex_colors(mesh))

patches=json.loads((Path(unreal.Paths.project_content_dir())/'CalicheStreet/street-patches.json').read_text())
for name,row in patches['meshes'].items():
    if row['hidden']:continue
    mesh=E.load_asset(row['asset'])
    check(name+' composed trim triangles',isinstance(mesh,unreal.StaticMesh) and mesh.get_num_triangles(0)==row['resultTris'])
check('souvenir geometry removed',sum(v['souvenirRemoved'] for v in patches['meshes'].values())>30)
M=unreal.MaterialEditingLibrary
for label in ('souvenir','ground','backdrop'):
    rows=json.loads((ROOT/label/'materials.json').read_text())
    if isinstance(rows,dict):rows=[dict(row,name=name) for name,row in rows.items()]
    for row in rows:
        mat=E.load_asset('/Game/CalicheStreet/'+label.title()+'/Materials/'+row['name'])
        if row.get('textures',{}).get('normal'):
            value=M.get_material_default_scalar_parameter_value(mat,'NormalStrength')
            check(row['name']+' controlled normal strength',abs(value-float(row.get('normal_strength',.2)))<.00001,value)
        check(row['name']+' sidedness',mat.get_editor_property('two_sided')==(bool(row.get('two_sided',False)) or float(row.get('alpha',1))<1))
        if 'specular' in row:
            check(row['name']+' authored specular',abs(M.get_material_default_scalar_parameter_value(mat,'Specular')-float(row['specular']))<.00001)
        if row.get('vertex_alpha_opacity') or row.get('alpha_mode')=='dithered_masked':
            expressions=M.get_material_expressions(mat)
            mask=M.get_material_property_input_node(mat,unreal.MaterialProperty.MP_OPACITY_MASK)
            check(row['name']+' masked perimeter',mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_MASKED)
            if row.get('vertex_alpha_opacity'):
                check(row['name']+' vertex opacity node',any(isinstance(n,unreal.MaterialExpressionVertexColor) for n in expressions))
            check(row['name']+' temporal perimeter mask',isinstance(mask,unreal.MaterialExpressionMaterialFunctionCall) and 'DitherTemporalAA' in str(mask.get_editor_property('material_function')))
radio_rows=json.loads((ROOT.parent/'radioarray/station/materials.json').read_text())
for name,row in radio_rows.items():
    if not row.get('textures',{}).get('normal'):continue
    mat=E.load_asset('/Game/RadioArray/Station/Materials/'+name)
    check(name+' corrected original finish',isinstance(mat,unreal.Material) and abs(M.get_material_default_scalar_parameter_value(mat,'NormalStrength')-float(row.get('normal_strength',1)))<.00001)
for name in ('M_Caliche_Sky','M_Caliche_Horizon','M_Caliche_Stars'):
    mat=E.load_asset('/Game/CalicheStreet/Atmosphere/'+name)
    check(name+' own sky asset',isinstance(mat,unreal.Material))
    check(name+' original sky outside Caliche',isinstance(mat,unreal.Material) and M.get_material_default_scalar_parameter_value(mat,'ConceptBlend')==0)
    if name=='M_Caliche_Horizon':
        check(name+' retain old horizon unless backdrop is ready',M.get_material_default_scalar_parameter_value(mat,'BackdropReady')==0)
result={'status':'PASS' if all(c['pass'] for c in checks) else 'FAIL','checks':checks,
        'engine':unreal.SystemLibrary.get_engine_version()}
OUT.write_text(json.dumps(result,indent=2))
assert result['status']=='PASS',[c for c in checks if not c['pass']]
unreal.log('CALICHE STREET VERIFY PASS '+str(len(checks)))
