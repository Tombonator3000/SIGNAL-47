"""Add exact-target ordinary native LODs without reimporting sources or changing materials.

UE 5.8.4 RayTracingGeometryManager.cpp:595 marks the last NumAlwaysResidentLODs
(default 1) in each mesh group as always resident. An ordinary reduced final LOD
therefore reduces the mandatory RT representation while preserving full LOD0.
This is separate from raster mesh streaming: Linux inherits its unsupported
platform default and r.MeshStreaming defaults to 0. No global setting changes.

Run only through UnrealEditor-Cmd when no other heavy engine/game job is active.
S47_FREE_RT_LOD_REPORT selects the measured native report. The canonical free
importer calls apply_configured_lods after importing the exact configured mesh.
"""
from pathlib import Path
import hashlib
import json
import os
import traceback
from urllib.parse import unquote, urlparse
import unreal

HERE = Path(__file__).resolve().parent
CONTENT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
CONFIG = HERE / 'models.json'
MANIFEST = CONTENT / 'Free/free_models.json'
E = unreal.EditorAssetLibrary
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
REPORT = Path(os.environ.get('S47_FREE_RT_LOD_REPORT', str(Path.home() / 'scratch-bm/Signal47-free/rt-lods-report.json')))


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write_report(report):
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2, allow_nan=False) + '\n')


def material_slots(mesh):
    return [{'slot': str(slot.material_slot_name), 'imported_slot': str(slot.get_editor_property('imported_material_slot_name')),
             'material': slot.material_interface.get_path_name() if slot.material_interface else None}
            for slot in mesh.get_editor_property('static_materials')]


def bounds(mesh):
    box = mesh.get_bounding_box()
    return {'min': [box.min.x, box.min.y, box.min.z], 'max': [box.max.x, box.max.y, box.max.z]}


def lod0_geometry(mesh):
    description = mesh.get_static_mesh_description(0)
    require(description is not None, 'Original LOD0 description is absent')
    positions = []
    for index in range(description.get_vertex_count()):
        vertex = unreal.VertexID(id_value=index)
        require(description.is_vertex_valid(vertex), 'Source LOD0 vertex IDs are not compact')
        point = description.get_vertex_position(vertex)
        positions.append([round(point.x, 5), round(point.y, 5), round(point.z, 5)])
    require(positions, 'Original LOD0 contains no vertices')
    return {'vertices': len(positions), 'triangles': description.get_triangle_count(),
            'positions_sha256': hashlib.sha256(json.dumps(sorted(positions), separators=(',', ':')).encode()).hexdigest(),
            'bounds_cm': {'min': [min(row[i] for row in positions) for i in range(3)],
                          'max': [max(row[i] for row in positions) for i in range(3)]}}


def per_platform(value):
    return {'default': value.get_editor_property('default'),
            'per_platform': {str(key): int(item) for key, item in value.get_editor_property('per_platform').items()}}


def platform_property(mesh, native_name, python_name):
    # Some non-editable UStaticMesh fields have no Pythonized editor property
    # in this build. Record that boundary instead of inventing a default value.
    for name in (native_name, python_name, python_name.replace('_lods', '_lo_ds')):
        try:
            return {'available': True, 'value': per_platform(mesh.get_editor_property(name))}
        except Exception:
            pass
    return {'available': False, 'native_property': native_name,
            'limit': 'Not exposed through this editor Python build; no direct setting write is performed.'}


def native_settings(mesh):
    body = mesh.get_editor_property('body_setup')
    return {'allow_cpu_access': bool(mesh.get_editor_property('allow_cpu_access')),
            'support_ray_tracing': bool(mesh.get_editor_property('support_ray_tracing')),
            'nanite_enabled': bool(mesh.get_editor_property('nanite_settings').get_editor_property('enabled')),
            'never_stream': bool(mesh.get_editor_property('never_stream')),
            'lod_streaming': str(mesh.get_editor_property('lod_streaming')),
            'num_streamed_lods': platform_property(mesh, 'NumStreamedLODs', 'num_streamed_lods'),
            'min_lod': platform_property(mesh, 'MinLOD', 'min_lod'),
            'lod_group': str(mesh.get_editor_property('lod_group')),
            'collision_lod': int(mesh.get_editor_property('lod_for_collision')),
            'collision_trace_flag': str(body.get_editor_property('collision_trace_flag')) if body else None}


def source_files(mesh):
    data = mesh.get_editor_property('asset_import_data')
    require(data is not None, 'Original asset import provenance is absent')
    files = {Path(value).resolve() for value in data.extract_filenames()}
    require(files, 'Original import source list is empty')
    for path in list(files):
        require(path.is_file(), 'Original import source is absent: ' + str(path))
        if path.suffix.lower() == '.gltf':
            gltf = json.loads(path.read_text())
            for row in gltf.get('buffers', []) + gltf.get('images', []):
                uri = row.get('uri', '')
                if not uri or uri.startswith('data:'):
                    continue
                parsed = urlparse(uri)
                require(not parsed.scheme or parsed.scheme == 'file', 'External non-file glTF URI')
                dependency = Path(unquote(parsed.path))
                files.add((dependency if dependency.is_absolute() else path.parent / dependency).resolve())
    require(all(path.is_file() for path in files), 'An original glTF buffer/texture dependency is absent')
    return {str(path): sha(path) for path in sorted(files)}


def related_assets(asset):
    model_root = CONTENT / 'Free/Vehicles/fairheaven_sw84'
    target = CONTENT / (asset.split('.')[0].removeprefix('/Game/') + '.uasset')
    require(target.is_file(), 'Original native target package is absent')
    return {str(path): sha(path) for path in sorted(model_root.rglob('*.uasset')) if path != target}


def apply_configured_lods(mesh, metadata):
    config = json.loads(CONFIG.read_text())
    name = metadata['name']
    profile = (config.get(name) or {}).get('runtime_lods')
    if not profile or mesh.get_name() != profile['asset_name']:
        return None
    asset = '/Game/Free/' + config[name]['category'] + '/' + name + '/StaticMeshes/' + profile['asset_name']
    require(mesh.get_path_name() == asset + '.' + profile['asset_name'], 'Configured exact asset path differs')
    report = {'status': 'RUNNING', 'engine': unreal.SystemLibrary.get_engine_version(), 'asset': mesh.get_path_name(),
              'profile': profile, 'metadata': metadata, 'checks': [],
              'basis': 'Ordinary native reduced final LOD changes the geometry group AlwaysResident member. Full LOD0 remains available. Raster mesh streaming is not assumed or enabled.',
              'limits': 'Native source/LOD/material/bounds verification only. Packaged RT residency, TLAS references and actual near/far images remain separate acceptance. No measured memory saving is claimed.'}
    def check(label, condition, detail=None):
        report['checks'].append({'name': label, 'pass': bool(condition), 'detail': detail})
        write_report(report)
        require(condition, label)
    try:
        check('original credited complete car', name == 'fairheaven_sw84' and metadata.get('licence') == 'CC BY 4.0'
              and metadata.get('author') == 'Daniel Zhabotinsky' and metadata.get('link') == 'https://sketchfab.com/3d-models/aa0becb6e854422596cac6b21bf79787')
        before = {'materials': material_slots(mesh), 'bounds_cm': bounds(mesh), 'lod0_geometry': lod0_geometry(mesh), 'native': native_settings(mesh),
                  'lod_triangles': [mesh.get_num_triangles(lod) for lod in range(S.get_lod_count(mesh))],
                  'sources_sha256': source_files(mesh), 'related_assets_sha256': related_assets(asset),
                  'manifest_sha256': sha(MANIFEST) if MANIFEST.is_file() else None}
        report['before'] = before
        check('exact original LOD0 topology', before['lod_triangles'][0] == profile['lod0_triangles'] == 19461)
        check('all eight original bound materials', len(before['materials']) == profile['material_count'] == 8 and all(row['material'] for row in before['materials']))
        check('translucent car remains non-Nanite with hardware RT', not before['native']['nanite_enabled'] and before['native']['support_ray_tracing'])
        check('original collision uses full LOD0', before['native']['collision_lod'] == 0)
        for edge in ('min', 'max'):
            check('original full LOD0 metric bounds ' + edge, max(abs(a-b) for a, b in zip(before['lod0_geometry']['bounds_cm'][edge], profile['bounds_cm'][edge])) < .2, before['lod0_geometry']['bounds_cm'][edge])
        options = unreal.StaticMeshReductionOptions()
        options.set_editor_property('auto_compute_lod_screen_size', False)
        reductions = []
        for percent, size in ((1.0, 1.0), (profile['lod1_percent_triangles'], profile['lod1_screen_size'])):
            setting = unreal.StaticMeshReductionSettings()
            setting.set_editor_property('percent_triangles', percent)
            setting.set_editor_property('screen_size', size)
            reductions.append(setting)
        options.set_editor_property('reduction_settings', reductions)
        check('two ordinary native rendering/RT LODs', S.set_lods(mesh, options) == 2)
        check('explicit native screen sizes', S.set_lod_screen_sizes(mesh, [1.0, profile['lod1_screen_size']]))
        check('native target saved', E.save_loaded_asset(mesh))
        after = {'materials': material_slots(mesh), 'bounds_cm': bounds(mesh), 'lod0_geometry': lod0_geometry(mesh), 'native': native_settings(mesh),
                 'lod_triangles': [mesh.get_num_triangles(lod) for lod in range(S.get_lod_count(mesh))],
                 'lod_screen_sizes': list(S.get_lod_screen_sizes(mesh))}
        report['after'] = after
        check('exact full LOD0 retained', len(after['lod_triangles']) == 2 and after['lod_triangles'][0] == 19461, after['lod_triangles'])
        actual_fraction = after['lod_triangles'][1] / after['lod_triangles'][0]
        check('actual native final LOD near requested triangle fraction', abs(actual_fraction - profile['lod1_percent_triangles']) < .04,
              {'actual_lod1_triangles': after['lod_triangles'][1], 'actual_fraction': actual_fraction, 'requested_fraction': profile['lod1_percent_triangles']})
        check('native screen transition preserved', len(after['lod_screen_sizes']) == 2 and abs(after['lod_screen_sizes'][1] - profile['lod1_screen_size']) < 1e-6, after['lod_screen_sizes'])
        check('all material names/order/bindings byte-independent identity preserved', before['materials'] == after['materials'])
        check('CPU access/collision/Nanite/RT and exposed streaming settings unchanged', before['native'] == after['native'], after['native'])
        check('full LOD0 positions/count/bounds retained', before['lod0_geometry'] == after['lod0_geometry'], after['lod0_geometry'])
        check('all-LOD culling bounds include only bounded far-LOD deviation',
              all(max(abs(a-b) for a, b in zip(profile['bounds_cm'][edge], after['bounds_cm'][edge])) < 2.5 for edge in ('min', 'max')),
              {'bounds_cm': after['bounds_cm'], 'limit_cm': 2.5, 'note': 'Overall culling bounds include reduced LOD1; exact LOD0 is checked separately.'})
        check('original glTF/texture/buffer files unchanged', source_files(mesh) == before['sources_sha256'])
        check('body/door/material/texture assets byte-preserved', related_assets(asset) == before['related_assets_sha256'])
        check('manifest and credit metadata untouched by LOD helper', (sha(MANIFEST) if MANIFEST.is_file() else None) == before['manifest_sha256'])
        report['status'] = 'PASS_NATIVE_LOD_ONLY'
        write_report(report)
        unreal.log('S47 FREE EXACT-TARGET NATIVE LODS PASS: ' + mesh.get_path_name())
        return report
    except Exception:
        report['status'] = 'FAIL'
        report['error'] = traceback.format_exc()
        write_report(report)
        raise


def main():
    manifest = json.loads(MANIFEST.read_text())
    config = json.loads(CONFIG.read_text())
    for name, row in config.items():
        if not isinstance(row, dict) or not row.get('runtime_lods'):
            continue
        asset_name = row['runtime_lods']['asset_name']
        asset = '/Game/Free/' + row['category'] + '/' + name + '/StaticMeshes/' + asset_name + '.' + asset_name
        matches = [item for item in manifest['models'] if item['asset'] == asset]
        require(len(matches) == 1, 'No unique original credited manifest row: ' + asset)
        mesh = E.load_asset(asset)
        require(isinstance(mesh, unreal.StaticMesh), 'Exact existing native target is absent: ' + asset)
        apply_configured_lods(mesh, matches[0])


if __name__ == '__main__':
    main()
