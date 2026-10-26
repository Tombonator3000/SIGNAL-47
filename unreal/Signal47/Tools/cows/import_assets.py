"""Import the authored Hereford cow family with the project's measured FBX/PBR recipe.

Run through import.sh. Only /Game/Signal47/Art/Cows is written. Blender sources
live in unreal/ArtSource/Cows; no Web/Unity source or existing scene is edited.
"""
import hashlib
import json
import os
from pathlib import Path
import sys
import traceback

import unreal

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]
SOURCE = Path(os.environ.get('S47_COW_SOURCE', str(PROJECT.parent / 'ArtSource/Cows'))).resolve()
WORK = Path(os.environ.get('S47_COW_WORK', str(Path.home() / 'scratch-bm/Signal47-cows-20261025'))).resolve()
WORK.mkdir(parents=True, exist_ok=True)
DEST = '/Game/Signal47/Art/Cows'
NAMES = ('SM_Cow_Alive_Body', 'SM_Cow_Alive_Head', 'SM_Cow_Abducted')
sys.path.insert(0, str(HERE.parent / 'fieldintegration'))
import import_recipe as R

R.REPORT = Path(os.environ.get('S47_COW_REPORT', str(WORK / 'native-import.json')))
R.REPORT.parent.mkdir(parents=True, exist_ok=True)
R.report = {'status': 'RUNNING', 'engine': unreal.SystemLibrary.get_engine_version(),
            'source': str(SOURCE), 'destination': DEST, 'checks': [], 'meshes': []}
E, S, M = R.E, R.S, R.M


def hashes():
    files = [path for path in SOURCE.rglob('*') if path.is_file()
             and path.suffix.lower() in ('.fbx', '.png', '.jpg', '.jpeg', '.json', '.blend')]
    return {str(path.relative_to(SOURCE)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(files)}


def lods(mesh, path):
    authored = [path.with_stem(path.stem + '_LOD' + str(lod)) for lod in (1, 2)]
    R.check(path.stem + ' complete authored LOD pair',
            all(item.is_file() for item in authored) or not any(item.is_file() for item in authored))
    if all(item.is_file() for item in authored):
        for lod, lodfile in enumerate(authored, 1):
            R.check(path.stem + ' authored LOD' + str(lod), S.import_lod(mesh, lod, str(lodfile)) == lod)
        return 'authored'
    # UE 5.8.4 StaticMeshEditorSubsystemHelpers.h exposes these native structs.
    options = unreal.StaticMeshReductionOptions()
    options.set_editor_property('auto_compute_lod_screen_size', False)
    settings = []
    for percent, size in ((1.0, 1.0), (.50, .30), (.20, .10)):
        row = unreal.StaticMeshReductionSettings()
        row.set_editor_property('percent_triangles', percent)
        row.set_editor_property('screen_size', size)
        settings.append(row)
    options.set_editor_property('reduction_settings', settings)
    result = S.set_lods(mesh, options)
    R.check(path.stem + ' native reduced LODs', result == 3, result)
    return 'native_reduction'


try:
    R.flush()
    R.check('explicit source directory', SOURCE.is_dir(), str(SOURCE))
    R.check('three base FBX sources', all((SOURCE / 'Export' / (name + '.fbx')).is_file() for name in NAMES))
    material_file = SOURCE / 'materials.json'
    R.check('authored PBR sidecar', material_file.is_file())
    rows = json.loads(material_file.read_text())
    if isinstance(rows, dict):
        rows = [dict(row, name=name) for name, row in rows.items()]
    R.check('unique authored material names', len(rows) > 0 and len({row['name'] for row in rows}) == len(rows))
    contract_file = SOURCE / 'model_contract.json'
    contract = json.loads(contract_file.read_text()).get('parts', {}) if contract_file.is_file() else {}
    R.report['source_sha256'] = hashes()
    mats = {}
    for row in rows:
        mat = R.material(row, DEST + '/Materials', SOURCE)
        # Both live cow submeshes are instanced; this must be baked before cook.
        mat.set_editor_property('used_with_instanced_static_meshes', True)
        R.check(row['name'] + ' instancing usage compiles', not list(M.recompile_material(mat)))
        if row.get('normal_convention', 'DirectX').lower() == 'opengl' and row.get('textures', {}).get('normal'):
            normal_file = Path(row['textures']['normal'])
            if not normal_file.is_absolute():
                normal_file = SOURCE / normal_file
            # The shared recipe deduplicates identical source maps across materials.
            normal = R.TEXTURE_CACHE.get((hashlib.sha256(normal_file.read_bytes()).hexdigest(), 'normal'))
            R.check(row['name'] + ' normal texture', isinstance(normal, unreal.Texture2D))
            normal.set_editor_property('flip_green_channel', True)
            E.save_loaded_asset(normal)
        R.check(row['name'] + ' material saved', E.save_loaded_asset(mat))
        mats[row['name']] = mat
    for name in NAMES:
        asset = DEST + '/' + name
        if E.does_asset_exist(asset):
            R.check(name + ' replace owned asset', E.delete_asset(asset))
        path = SOURCE / 'Export' / (name + '.fbx')
        mesh = R.import_mesh(path, DEST)
        slots = []
        for index, slot in enumerate(mesh.get_editor_property('static_materials')):
            material = str(slot.material_slot_name)
            R.check(name + ' authored slot ' + material, material in mats, list(mats))
            mesh.set_material(index, mats[material])
            slots.append(material)
        R.check(name + ' nonempty PBR slots', bool(slots))
        row = contract.get(name, {})
        if 'materials' in row:
            R.check(name + ' source material set', set(slots) == set(row['materials']), {'native': slots, 'source': row['materials']})
        lod_source = lods(mesh, path)
        settings = mesh.get_editor_property('nanite_settings')
        settings.enabled = False
        mesh.set_editor_property('nanite_settings', settings)
        # Preserve solid-geometry RT fidelity. Residency is measured in the packaged
        # game; the editor also loads about 4.1 MiB of unrelated editor-only geometry.
        mesh.set_editor_property('support_ray_tracing', True)
        R.check(name + ' solid animal supports hardware RT geometry', mesh.get_editor_property('support_ray_tracing'))
        body = mesh.get_editor_property('body_setup')
        if body:
            body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        for lod in range(S.get_lod_count(mesh)):
            build = S.get_lod_build_settings(mesh, lod)
            build.set_editor_property('use_full_precision_u_vs', True)
            S.set_lod_build_settings(mesh, lod, build)
        R.check(name + ' saved', E.save_loaded_asset(mesh))
        triangles = [mesh.get_num_triangles(lod) for lod in range(S.get_lod_count(mesh))]
        R.check(name + ' three actual LODs', len(triangles) == 3, triangles)
        R.check(name + ' strictly reduced topology', triangles[0] > triangles[1] > triangles[2] > 0, triangles)
        if row.get('lod_triangles'):
            R.check(name + ' authored triangle counts', triangles == row['lod_triangles'], {'actual': triangles, 'source': row['lod_triangles']})
        bounds = mesh.get_bounds()
        center = [bounds.origin.x, bounds.origin.y, bounds.origin.z]
        size = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
        if row.get('bounds_local_m'):
            low, high = row['bounds_local_m']
            expected_center = [(low[0] + high[0]) * 50, -(low[1] + high[1]) * 50, (low[2] + high[2]) * 50]
            expected_size = [(high[k] - low[k]) * 100 for k in range(3)]
            R.check(name + ' FBX metric center and axes', max(abs(a-b) for a,b in zip(center, expected_center)) < .35,
                    {'actual': center, 'source': expected_center})
            R.check(name + ' FBX metric size', max(abs(a-b) for a,b in zip(size, expected_size)) < .35,
                    {'actual': size, 'source': expected_size})
        R.report['meshes'].append({'name': name, 'asset': mesh.get_path_name(), 'lod_source': lod_source,
            'triangles': triangles, 'materials': slots, 'center_cm': center, 'size_cm': size})
    R.check('source files unchanged by import', hashes() == R.report['source_sha256'])
    R.report['status'] = 'PASS'
    R.flush()
    unreal.log('S47 COWS NATIVE IMPORT PASS')
except Exception:
    R.report['status'] = 'FAIL'
    R.report['error'] = traceback.format_exc()
    R.flush()
    raise
