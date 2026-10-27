"""Explicit UE5.8 commandlet: append distant resident LOD3 to 24 motel parts.

Same S47_MOTEL_SOURCE/TRIMS/REPORT environment as import_assets.py. This does not
select authored LOD1 or rely on ineffective non-Nanite proxy settings. It appends
an ordinary native LOD3 at 2% LOD0 and leaves all three authored raster LODs,
collision, material bindings, trims, probe and global policy intact. Tiny planes
copy their already negligible LOD2 instead of risking zero-face reduction.
No process launch. Packaged GPU residency and visual acceptance remain separate.
"""
import hashlib
import os
import struct
import sys
import traceback
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import native as N


def raster_snapshot(mesh):
    materials = [{'slot': str(slot.get_editor_property('material_slot_name')),
                  'imported_slot': str(slot.get_editor_property('imported_material_slot_name')),
                  'material': slot.get_editor_property('material_interface').get_path_name()
                  if slot.get_editor_property('material_interface') else None}
                 for slot in mesh.get_editor_property('static_materials')]
    lods = []
    for lod in range(N.S.get_lod_count(mesh)):
        description = mesh.get_static_mesh_description(lod)
        points = N.vertices(description)
        digest = hashlib.sha256()
        for point in points:
            digest.update(struct.pack('<ddd', *point))
        lods.append({'triangles': mesh.get_num_triangles(lod), 'source_triangles': description.get_triangle_count(),
                     'vertices': len(points), 'positions_sha256': digest.hexdigest(),
                     'bounds_cm': [[op(p[a] for p in points) for a in range(3)] for op in (min, max)]})
    body = mesh.get_editor_property('body_setup')
    return {'materials': materials, 'lods': lods, 'screen_sizes': list(N.S.get_lod_screen_sizes(mesh)),
            'collision_lod': int(mesh.get_editor_property('lod_for_collision')),
            'collision_trace_flag': str(body.get_editor_property('collision_trace_flag')) if body else None,
            'nanite_enabled': bool(mesh.get_editor_property('nanite_settings').get_editor_property('enabled')),
            'support_ray_tracing': bool(mesh.get_editor_property('support_ray_tracing')),
            'allow_cpu_access': bool(mesh.get_editor_property('allow_cpu_access')),
            'never_stream': bool(mesh.get_editor_property('never_stream'))}


def native_files():
    content = Path(N.unreal.Paths.convert_relative_path_to_full(N.unreal.Paths.project_content_dir()))
    return {str(path.resolve()): N.sha(path) for path in (content / 'MotelV2').rglob('*.uasset')}


def build_state(mesh):
    # Do not pretend NullRHI/editor source data is a GPU-residency measurement.
    query = getattr(mesh, 'is_compiling', None)
    return {'is_compiling': bool(query()) if callable(query) else None,
            'ordinary_render_lod_triangles': [mesh.get_num_triangles(i) for i in range(N.S.get_lod_count(mesh))],
            'gpu_rt_allocation': 'UNVERIFIED until actual packaged RayTracingGeometry CSV and allocation dump'}


def authored_snapshot(snapshot):
    return {**snapshot, 'lods': snapshot['lods'][:3], 'screen_sizes': snapshot['screen_sizes'][:3]}


def main(root, trims, report):
    contract, _ = N.load_contract(root, report)
    report.check('exact authored motel part set', len(contract['parts']) == 24 and
                 all(name.startswith('SM_Sierra_') for name in contract['parts']))
    report.data['basis'] = {
        'operation': 'Append ordinary native LOD3 at 2% LOD0, screen .005; tiny pieces copy LOD2. Disable ineffective dedicated proxy.',
        'engine_static_mesh': 'SetLodFromStaticMesh appends only source model3; SetLodReductionSettings changes only model3',
        'engine_evidence': 'StaticMeshBuilder.cpp builds dedicated proxies only inside bBuildNanite. Actual R2 dump Room6Lining5420 confirms ordinaryLOD2. GeometryManager always-resident uses final ordinaryLOD.',
        'raster': 'All three authored LODs and original screens retained with exact per-LOD position hashes',
        'global_policy': 'No changes; RT budget400MiB/warning20% remain authoritative',
        'limits': 'Native settings/source verification only. Packaged RT memory and actual screenshots required.'}
    before_files = native_files()
    source_before = dict(report.data['source_files_sha256'])
    targets = set()
    report.data['updates'] = []
    for name in contract['parts']:
        mesh = N.E.load_asset(N.DEST + '/' + name)
        report.check(name + ' existing native mesh', isinstance(mesh, N.unreal.StaticMesh))
        before = raster_snapshot(mesh)
        before_settings = N.rt_proxy_settings(mesh)
        N.set_resident_lod(mesh, report)
        report.check(name + ' saved appended ordinary resident LOD', N.E.save_loaded_asset(mesh))
        after = raster_snapshot(mesh)
        report.check(name + ' authored LOD0..2 materials collision screens unchanged', authored_snapshot(before) == authored_snapshot(after))
        report.check(name + ' exactly one appended LOD3', len(after['lods']) == 4)
        content = Path(N.unreal.Paths.convert_relative_path_to_full(N.unreal.Paths.project_content_dir()))
        targets.add(str((content / 'MotelV2' / (name + '.uasset')).resolve()))
        report.data['updates'].append({'name': name, 'before_settings': before_settings,
                                       'after_settings': N.rt_proxy_settings(mesh), 'preserved_raster': after,
                                       'build_state': build_state(mesh)})
        report.flush()
    after_files = native_files()
    report.check('no owned asset created or deleted', set(before_files) == set(after_files))
    report.check('trims probe materials textures byte-identical',
                 all(after_files[path] == digest for path, digest in before_files.items() if path not in targets))
    report.check('authored FBX material texture source bytes unchanged',
                 all(N.sha(path) == digest for path, digest in source_before.items()))
    N.verify_all(root, trims, report)
    report.data['status'] = 'PASS_NATIVE_RESIDENT_LOD_ONLY'
    report.data['final_lod_triangles_total'] = sum(r['lods'][-1]['triangles'] for r in report.data['meshes'] if r['name'].startswith('SM_Sierra_'))
    report.flush()
    N.unreal.log('MOTEL_V2_RESIDENT_LOD_NATIVE_PASS: 24 ordinary final LODs, authored LOD0..2 unchanged; packaged memory unverified')


if __name__ == '__main__':
    report = N.Report(os.environ['S47_MOTEL_REPORT'], 'native-resident-lod-update')
    try:
        main(Path(os.environ['S47_MOTEL_SOURCE']).resolve(),
             Path(os.environ['S47_MOTEL_TRIMS']).resolve() if os.environ.get('S47_MOTEL_TRIMS') else None, report)
    except Exception:
        report.data.update(status='FAIL', error=traceback.format_exc())
        report.flush()
        raise
