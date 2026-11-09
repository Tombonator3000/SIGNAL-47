"""Import the authored cobra-head street lamp family with the project's measured FBX/PBR recipe.

Run under unreal_guard against /home/tom/SIGNAL-47/unreal/Signal47/Signal47.uproject. The source
folder is explicit: S47_STREETLAMP_SOURCE, or a '--source=<dir>' argument, or by default the
ArtSource/StreetLamps folder next to this script's checkout (absolute, so a worktree copy of this
script imports from that worktree). Source geometry and maps are never modified.
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
ARG = next((a.split('=', 1)[1] for a in sys.argv[1:] if a.startswith('--source=')), None)
SOURCE = Path(ARG or os.environ.get('S47_STREETLAMP_SOURCE', str(PROJECT.parent / 'ArtSource/StreetLamps'))).resolve()
WORK = Path(os.environ.get('S47_STREETLAMP_WORK', str(Path.home() / 'scratch-bm/Signal47-streetlamps'))).resolve()
WORK.mkdir(parents=True, exist_ok=True)
DEST = '/Game/Signal47/Art/StreetLamps'
NAMES = ('SM_StreetLamp_Head', 'SM_StreetLamp_Arm', 'SM_StreetLamp_Pole', 'SM_StreetLamp_WoodArm')
sys.path.insert(0, str(HERE.parent / 'fieldintegration'))
import import_recipe as R

R.REPORT = Path(os.environ.get('S47_STREETLAMP_REPORT', str(WORK / 'streetlamps-native-import.json')))
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
    R.check(path.stem + ' complete authored LOD pair', all(item.is_file() for item in authored))
    # Not S.import_lod: it drops import_mesh's import_rotation, and LOD1/2 came in turned 90 degrees (9 November).
    for lod, lodfile in enumerate(authored, 1):
        R.check(path.stem + ' authored LOD' + str(lod), R.import_lod_same_frame(mesh, lod, lodfile, DEST) == lod)
    R.check_lod_frames(mesh, path.stem)
    return 'authored'


try:
    R.flush()
    R.check('explicit source directory', SOURCE.is_dir(), str(SOURCE))
    R.check('four base FBX sources', all((SOURCE / 'Export' / (name + '.fbx')).is_file() for name in NAMES))
    rows = json.loads((SOURCE / 'materials.json').read_text())
    R.check('unique authored material names', len(rows) > 0 and len({row['name'] for row in rows}) == len(rows))
    contract_doc = json.loads((SOURCE / 'model_contract.json').read_text())
    contract = contract_doc.get('parts', {})
    R.check('lens slot declared', contract_doc.get('lens_material') == 'M_StreetLamp_Lens')
    R.report['source_sha256'] = hashes()
    mats = {}
    for row in rows:
        mat = R.material(row, DEST + '/Materials', SOURCE)
        # Street lamps repeat along roads; keep instanced placement support.
        mat.set_editor_property('used_with_instanced_static_meshes', True)
        R.check(row['name'] + ' instancing usage compiles', not list(M.recompile_material(mat)))
        R.check(row['name'] + ' material saved', E.save_loaded_asset(mat))
        mats[row['name']] = mat
    # import_recipe.material gives every row the EmissiveFactor vector parameter (base texture x factor).
    R.check('separate lens material', 'M_StreetLamp_Lens' in mats, list(mats))
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
        row = contract.get(name, {})
        R.check(name + ' source material set', set(slots) == set(row.get('materials', [])),
                {'native': slots, 'source': row.get('materials')})
        lod_source = lods(mesh, path)
        settings = mesh.get_editor_property('nanite_settings')
        settings.enabled = False
        mesh.set_editor_property('nanite_settings', settings)
        mesh.set_editor_property('support_ray_tracing', True)
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
        R.check(name + ' authored triangle counts', triangles == row.get('lod_triangles'),
                {'actual': triangles, 'source': row.get('lod_triangles')})
        bounds = mesh.get_bounds()
        center = [bounds.origin.x, bounds.origin.y, bounds.origin.z]
        size = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
        low, high = row['bounds_local_m']
        expected_center = [(low[0] + high[0]) * 50, -(low[1] + high[1]) * 50, (low[2] + high[2]) * 50]
        expected_size = [(high[k] - low[k]) * 100 for k in range(3)]
        R.check(name + ' FBX metric center and axes', max(abs(a - b) for a, b in zip(center, expected_center)) < .35,
                {'actual': center, 'source': expected_center})
        R.check(name + ' FBX metric size', max(abs(a - b) for a, b in zip(size, expected_size)) < .35,
                {'actual': size, 'source': expected_size})
        R.report['meshes'].append({'name': name, 'asset': mesh.get_path_name(), 'lod_source': lod_source,
            'triangles': triangles, 'materials': slots, 'center_cm': center, 'size_cm': size})
    R.check('source files unchanged by import', hashes() == R.report['source_sha256'])
    R.report['status'] = 'PASS'
    R.flush()
    unreal.log('S47 STREETLAMPS IMPORT PASS')
except Exception:
    R.report['status'] = 'FAIL'
    R.report['error'] = traceback.format_exc()
    R.flush()
    raise
