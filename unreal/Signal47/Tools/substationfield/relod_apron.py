"""Run once on 9 November 2026 (PASS): ApronGrade's LOD1/2 had come in turned 90 degrees through S.import_lod.
Replace only SM_SubstationField_ApronGrade's LOD1 and LOD2 with the authored LOD files in LOD0's frame. Nothing else is imported."""
import json, traceback
from pathlib import Path
import unreal
E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
S = (unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem))
ROOT = Path(__file__).resolve().parent
OUT = Path(unreal.Paths.project_saved_dir()) / 'substationfield-relod-apron.json'
report = {'checks': []}
def check(name, ok, detail=None):
    report['checks'].append({'name': name, 'pass': bool(ok), 'detail': detail}); OUT.write_text(json.dumps(report, indent=1))
    assert ok, (name, detail)

def import_mesh(path, dest):  # Tools/substationfield/import_assets.py's own recipe, unchanged
    options = unreal.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.import_as_skeletal = False
    options.import_mesh = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.import_materials = False
    options.import_textures = False
    data = options.static_mesh_import_data
    data.combine_meshes = True
    data.auto_generate_collision = False
    data.generate_lightmap_u_vs = True
    data.convert_scene = True
    data.convert_scene_unit = True
    data.force_front_x_axis = True
    data.set_editor_property('coordinate_system_policy', unreal.CoordinateSystemPolicy.MATCH_UP_FORWARD_AXES)
    data.import_rotation = unreal.Rotator(pitch=0, yaw=90, roll=0)
    data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    task = unreal.AssetImportTask()
    task.filename = str(path); task.destination_path = dest; task.destination_name = path.stem
    task.replace_existing = True; task.replace_existing_settings = True; task.automated = True; task.save = True
    task.options = options; task.factory = unreal.FbxFactory()
    T.import_asset_tasks([task])
    mesh = E.load_asset(dest + '/' + path.stem)
    check(path.stem + ' imported', isinstance(mesh, unreal.StaticMesh))
    return mesh

def box(mesh, lod):
    lo, hi = [1e9] * 3, [-1e9] * 3
    for s in range(mesh.get_num_sections(lod)):
        for v in unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh, lod, s)[0]:
            for k, x in enumerate((v.x, v.y, v.z)):
                lo[k] = min(lo[k], x); hi[k] = max(hi[k], x)
    return lo, hi

try:
    name = 'SM_SubstationField_ApronGrade'
    mesh = E.load_asset('/Game/SubstationField/' + name)
    check('existing mesh', isinstance(mesh, unreal.StaticMesh))
    contract = json.loads((ROOT / 'model_contract.json').read_text())['parts'][name]
    report['before'] = {'tris': [mesh.get_num_triangles(i) for i in range(mesh.get_num_lods())], 'boxes': [box(mesh, i) for i in range(mesh.get_num_lods())]}
    for lod in (1, 2):
        src = ROOT / 'exports' / (name + '_LOD%d.fbx' % lod)
        check(src.name + ' exists', src.is_file())
        temp = '/Game/SubstationField/_LodTemp'
        source = import_mesh(src, temp)
        got = S.set_lod_from_static_mesh(mesh, lod, source, 0, True)
        E.delete_asset(temp + '/' + src.stem); E.delete_directory(temp)
        check(name + ' LOD%d copied' % lod, got == lod, got)
    b0 = box(mesh, 0)
    for lod in (1, 2):
        b = box(mesh, lod); worst = max(abs(a - c) for p, q in zip(b0, b) for a, c in zip(p, q))
        # a reduced LOD may lose a sliver at its edge (LOD2: 12 cm on a 13 m apron); a turned one is off by metres
        tol = max(5.0, .015 * max(h - l for l, h in zip(*b0)))
        check(name + ' LOD%d in LOD0 frame' % lod, worst <= tol, {'lod0': b0, 'lod': b, 'worst_cm': worst, 'tolerance_cm': tol})
    tris = [mesh.get_num_triangles(i) for i in range(mesh.get_num_lods())]
    check(name + ' authored LOD triangles', tris == contract['lod_triangles'], {'actual': tris, 'contract': contract['lod_triangles']})
    check(name + ' saved', E.save_loaded_asset(mesh, only_if_is_dirty=False))
    check('no temp folder left', not E.does_directory_exist('/Game/SubstationField/_LodTemp'))
    report['status'] = 'PASS'; OUT.write_text(json.dumps(report, indent=1)); unreal.log('S47 APRON RELOD PASS')
except Exception:
    report['status'] = 'FAIL'; report['error'] = traceback.format_exc(); OUT.write_text(json.dumps(report, indent=1)); raise
