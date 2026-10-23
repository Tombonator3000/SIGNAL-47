"""UE Python: original Blender sources to /Game/RadioArray. No licensed or shared asset is modified.

Run with legacy FBX importer enabled, unattended/nullrhi. S47_RADIOARRAY_ONLY=dish|station
selects a completed source set. Sidecars retain the Blender material values without FBX shading loss.
"""
import json
import os
from pathlib import Path
import shutil
import traceback
import unreal

ROOT = Path(__file__).resolve().parent
CONTENT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
REPORT = Path(os.environ.get('S47_RADIOARRAY_REPORT', str(ROOT / 'unreal-import.json')))
report = {'engine': unreal.SystemLibrary.get_engine_version(), 'checks': [], 'meshes': []}


def check(name, condition, detail=None):
    report['checks'].append({'name': name, 'pass': bool(condition), 'detail': detail})
    REPORT.write_text(json.dumps(report, indent=2))
    assert condition, (name, detail)


def expression(mat, cls, **values):
    node = M.create_material_expression(mat, cls)
    for key, value in values.items():
        node.set_editor_property(key, value)
    return node


def material(row, dest, src):
    name = row['name']
    mat = E.load_asset(dest + '/' + name) if E.does_asset_exist(dest + '/' + name) else T.create_asset(name, dest, unreal.Material, unreal.MaterialFactoryNew())
    M.delete_all_material_expressions(mat)
    col = row.get('base_color_linear', row.get('base_color', [.5, .5, .5, 1]))
    color = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name='Color', default_value=unreal.LinearColor(*col[:3], 1))
    M.connect_material_property(color, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    for key, prop in [('metallic', unreal.MaterialProperty.MP_METALLIC), ('roughness', unreal.MaterialProperty.MP_ROUGHNESS)]:
        n = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name=key.title(), default_value=float(row.get(key, .5)))
        M.connect_material_property(n, '', prop)
    strength = float(row.get('emission_strength', 0))
    em = row.get('emission', [0, 0, 0, 1])
    # Props::AddLit/ApplyLit uses this existing glTF-compatible parameter to gate studio power.
    n = expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name='EmissiveFactor', default_value=unreal.LinearColor(*[float(v) * strength for v in em[:3]], 1))
    M.connect_material_property(n, 'RGB', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    for role, filename in row.get('textures', {}).items():
        if role not in ('base_color', 'normal', 'roughness'):
            continue
        path = Path(filename)
        if not path.is_absolute():
            path = src / path
        check(name + ' texture source ' + role, path.is_file(), str(path))
        task = unreal.AssetImportTask()
        task.filename = str(path)
        task.destination_path = dest + '/Textures'
        task.destination_name = 'T_' + name + '_' + role
        task.replace_existing = True
        task.automated = True
        task.save = True
        T.import_asset_tasks([task])
        tex = E.load_asset(task.destination_path + '/' + task.destination_name)
        check(name + ' texture ' + role, isinstance(tex, unreal.Texture2D))
        tex.set_editor_property('srgb', role == 'base_color')
        if role == 'normal':
            tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
            # Source normals are DirectX tangent-space, exported explicitly by the generator.
            tex.set_editor_property('flip_green_channel', False)
        E.save_loaded_asset(tex)
        sample = expression(mat, unreal.MaterialExpressionTextureSample, texture=tex,
                            sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role == 'normal' else
                            unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if role == 'base_color' else
                            unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        scale = float(row.get('uv_scale', 1))
        if scale != 1:
            uv = expression(mat, unreal.MaterialExpressionTextureCoordinate, u_tiling=scale, v_tiling=scale)
            check(name+' UV scale connection', M.connect_material_expressions(uv, '', sample, 'UVs'))
        prop = {'base_color': unreal.MaterialProperty.MP_BASE_COLOR, 'normal': unreal.MaterialProperty.MP_NORMAL,
                'roughness': unreal.MaterialProperty.MP_ROUGHNESS}[role]
        if role == 'normal':
            # Decode first, then blend towards the neutral tangent normal; avoid embossed stucco.
            neutral = expression(mat, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(0,0,1))
            strength = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name='NormalStrength', default_value=float(row.get('normal_strength', 1)))
            blend = expression(mat, unreal.MaterialExpressionLinearInterpolate)
            check(name+' normal A connection', M.connect_material_expressions(neutral, '', blend, 'A'))
            check(name+' normal B connection', M.connect_material_expressions(sample, 'RGB', blend, 'B'))
            check(name+' normal Alpha connection', M.connect_material_expressions(strength, '', blend, 'Alpha'))
            normalized = expression(mat, unreal.MaterialExpressionNormalize)
            check(name+' normal VectorInput connection', M.connect_material_expressions(blend, '', normalized, 'VectorInput'))
            M.connect_material_property(normalized, '', prop)
        else:
            M.connect_material_property(sample, 'RGB' if role != 'roughness' else 'R', prop)
    if float(row.get('alpha', 1)) < 1:
        mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property('two_sided', True)
        n = expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name='Opacity', default_value=float(row['alpha']))
        M.connect_material_property(n, '', unreal.MaterialProperty.MP_OPACITY)
    M.recompile_material(mat)
    E.save_loaded_asset(mat)
    return mat


def import_mesh(path, dest):
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
    # The asymmetric probe measured FBX (x,y,z) -> UE (-y,-x,z); bake +90 degrees to (x,-y,z).
    data.import_rotation = unreal.Rotator(pitch=0, yaw=90, roll=0)
    data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    task = unreal.AssetImportTask()
    task.filename = str(path)
    task.destination_path = dest
    task.destination_name = path.stem
    task.replace_existing = True
    task.replace_existing_settings = True
    task.automated = True
    task.save = True
    task.options = options
    task.factory = unreal.FbxFactory()
    T.import_asset_tasks([task])
    mesh = E.load_asset(dest + '/' + path.stem)
    check(path.stem + ' imported', isinstance(mesh, unreal.StaticMesh))
    return mesh


def import_set(label):
    src = ROOT / label
    dest = '/Game/RadioArray/' + label.title()
    rows = json.loads((src / 'materials.json').read_text())
    if isinstance(rows, dict):
        rows = [dict(row, name=name) for name, row in rows.items()]
    mats = {row['name']: material(row, dest + '/Materials', src) for row in rows}
    paths = sorted((src / 'exports').glob('SM_*.fbx'))
    paths = [p for p in paths if '_LOD' not in p.stem]
    check(label + ' sources', bool(paths))
    for path in paths:
        mesh = import_mesh(path, dest)
        slots = list(mesh.get_editor_property('static_materials'))
        for i, slot in enumerate(slots):
            name = str(slot.get_editor_property('material_slot_name'))
            check(path.stem + ' material ' + name, name in mats, list(mats))
            mesh.set_material(i, mats[name])
        for lod in (1, 2):
            lodfile = path.with_stem(path.stem + '_LOD' + str(lod))
            if lodfile.exists():
                check(path.stem + ' LOD' + str(lod), S.import_lod(mesh, lod, str(lodfile)) == lod)
        settings = mesh.get_editor_property('nanite_settings')
        settings.enabled = False
        mesh.set_editor_property('nanite_settings', settings)
        body = mesh.get_editor_property('body_setup')
        if body:
            body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        E.save_loaded_asset(mesh)
        b = mesh.get_bounds()
        row = {'name': path.stem, 'asset': mesh.get_path_name(), 'lod_count': S.get_lod_count(mesh),
               'triangles': [mesh.get_num_triangles(i) for i in range(S.get_lod_count(mesh))],
               'center_cm': [b.origin.x, b.origin.y, b.origin.z],
               'size_cm': [2*b.box_extent.x, 2*b.box_extent.y, 2*b.box_extent.z], 'materials': len(slots)}
        report['meshes'].append(row)
        if label == 'dish':
            check(path.stem + ' three LODs', row['lod_count'] == 3)
            check(path.stem + ' metres converted to cm', max(row['size_cm']) > 100)

def import_probe():
    # A tiny asymmetric source is a deterministic FBX coordinate contract, before world placement.
    probe = ROOT / 'station/exports/axis_probe.fbx'
    if E.does_asset_exist('/Game/RadioArray/Station/Verification/axis_probe'):
        E.delete_asset('/Game/RadioArray/Station/Verification/axis_probe')
    p = import_mesh(probe, '/Game/RadioArray/Station/Verification')
    b = p.get_bounds()
    report['axis_probe_center_cm'] = [b.origin.x, b.origin.y, b.origin.z]
    report['axis_probe_size_cm'] = [2*b.box_extent.x, 2*b.box_extent.y, 2*b.box_extent.z]
    check('FBX probe height centimetres', abs(b.origin.z-600) < .1)
    check('FBX axes: Blender X to Unreal X, Blender Y to negative Unreal Y',
          max(abs(a-b) for a, b in zip(report['axis_probe_center_cm'], [200, -400, 600])) < .1,
          report['axis_probe_center_cm'])


def import_trim():
    src = Path(os.environ.get('S47_RADIOARRAY_TRIM', str(ROOT / 'trim')))
    metadata = src / 'station-patches.json'
    if not metadata.exists():
        return
    files = sorted(src.glob('trim_*.glb'))
    check('station trimmed geometry sources', bool(files))
    dest = '/Game/RadioArray/Station/Trim'
    if E.does_directory_exist(dest):
        E.delete_directory(dest)
    for path in files:
        task = unreal.AssetImportTask()
        task.filename = str(path)
        task.destination_path = dest
        task.replace_existing = True
        task.automated = True
        task.save = True
        T.import_asset_tasks([task])
    E.make_directory(dest + '/StaticMeshes')
    for path in E.list_assets(dest, recursive=True):
        mesh = E.load_asset(path)
        if isinstance(mesh, unreal.StaticMesh):
            for slot in range(len(mesh.get_editor_property('static_materials'))):
                mesh.set_material(slot, E.load_asset('/Engine/EngineMaterials/DefaultMaterial'))
            body = mesh.get_editor_property('body_setup')
            if body:
                body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            settings = mesh.get_editor_property('nanite_settings')
            settings.enabled = False
            mesh.set_editor_property('nanite_settings', settings)
            E.save_loaded_asset(mesh)
            # Default glTF import has a scene subfolder; normalize only our six derived meshes.
            wanted = dest + '/StaticMeshes/' + mesh.get_name()
            if mesh.get_path_name().split('.')[0] != wanted:
                check(mesh.get_name() + ' normalized runtime path', E.rename_asset(path, wanted))
    for name, row in json.loads(metadata.read_text())['meshes'].items():
        if row['hidden']:
            continue
        mesh = E.load_asset(row['asset'])
        # Interchange's render-data compilation may finish during shutdown. Exact triangle counts
        # are checked by verify_unreal.py in a fresh process, after the saved meshes are complete.
        check(name + ' composed trim at runtime path', isinstance(mesh, unreal.StaticMesh), row['asset'])
    out = CONTENT / 'RadioArray/Station'
    out.mkdir(parents=True, exist_ok=True)
    shutil.copy2(metadata, out / metadata.name)


try:
    labels = os.environ.get('S47_RADIOARRAY_ONLY', 'dish,station').split(',')
    import_probe()
    for label in labels:
        if label not in ('probe', 'trim'):
            import_set(label)
    if 'station' in labels or 'trim' in labels:
        import_trim()
    report['status'] = 'PASS'
    REPORT.write_text(json.dumps(report, indent=2))
    unreal.log('RADIOARRAY IMPORT PASS')
except Exception:
    report['status'] = 'FAIL'
    report['error'] = traceback.format_exc()
    REPORT.write_text(json.dumps(report, indent=2))
    raise
