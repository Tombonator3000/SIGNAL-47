"""Activate only independently verified native assets by writing assembly.json.

Pure Python; no editor/build launch. Import and fresh read-only verification must
both PASS with matching current authored source fingerprints. Runtime layout is
the separate reviewed world-space contract, not inferred from mesh AABBs.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read(path):
    return json.loads(Path(path).read_text())


def rect(value):
    assert isinstance(value, list) and len(value) == 4
    assert all(isinstance(v, (int, float)) and math.isfinite(v) for v in value)
    assert value[0] < value[1] and value[2] < value[3]


def overlap_area(a, b):
    return max(0, min(a[1], b[1]) - max(a[0], b[0])) * max(0, min(a[3], b[3]) - max(a[2], b[2]))


def verify_grounding(source, manifest, trim, layout):
    """Bind repaired geometry, ground cuts and walking heights to one source."""
    path = source / 'grounding-contract.json'
    grounding = read(path)
    repair = manifest['grounding_repair']
    assert repair['id'] == grounding['repair_id'] == 'motel-grounded-20261029-v1'
    assert repair['contract_sha256'] == trim['grounding_contract_sha256'] == sha(path)
    origin = manifest['parts']['SM_Sierra_Office']['site_world_three_m']
    expected = []
    for surface in grounding['surface_rectangles_blender_xy_m'].values():
        x0, x1, y0, y1 = surface['rect']
        pieces = [[x0, x1, y0, y1]]
        if 'water_rect' in surface:
            a, b, c, d = surface['water_rect']
            assert x0 < a < b < x1 and y0 < c < d < y1
            pieces = [[x0, a, y0, y1], [b, x1, y0, y1],
                      [a, b, y0, c], [a, b, d, y1]]
        for a, b, c, d in pieces:
            expected.append(([origin[0] + a, origin[0] + b, origin[2] - d, origin[2] - c],
                             origin[1] + surface['top_z']))
    remaining = list(layout['floors'])
    for footprint, height in expected:
        matches = [row for row in remaining if len(row['rect']) == 4
                   and all(math.isclose(a, b, abs_tol=.0001) for a, b in zip(row['rect'], footprint))
                   and math.isclose(row['y'], height, abs_tol=.00001)]
        assert len(matches) == 1, ('runtime floor differs from repaired source', footprint, height)
        remaining.remove(matches[0])
    assert not remaining, ('runtime floors without authored surface', remaining)
    # FloorAt uses first match. Raised tiles/deck must precede their underlay.
    for index, earlier in enumerate(layout['floors']):
        for later in layout['floors'][index + 1:]:
            if overlap_area(earlier['rect'], later['rect']) > 1e-8:
                assert earlier['y'] >= later['y'] - .00001, 'lower floor shadows a raised visible surface'


def verify_asphalt(trim):
    """The actual original asphalt is partially replaced; its approach stays."""
    row = trim['meshes']['g456']
    assert (row['node'], row['geo'], row['parent'], row['source_tris'], row['result_tris']) == (671, 456, 663, 24, 30)
    assert row['accepted_previous_tris'] == [24] and row['original_material_indices'] == [0]
    assert row['source_material_ids'] == [320] and row['hidden'] is False
    assert row['asset'] == '/Game/MotelV2/Trim/g456.g456' and row['fbx'] == 'exports/g456.fbx'
    translation = row['source_parent_translation_three_m']
    assert translation == [-2.5, -2.098568384594018, 650]
    holes = [[a - translation[0], b - translation[0], c - translation[2], d - translation[2]]
             for a, b, c, d in trim['ground_openings_world_xz_m']]
    proof = trim['legacy_asphalt_verification']
    assert proof['status'] == 'PASS' and proof['original_buffer_prefix_identical'] is True
    assert (proof['node'], proof['geo'], proof['parent'], proof['source_material_ids']) == (671, 456, 663, [320])
    assert row['ground_holes_local_xz_m'] == proof['ground_holes_local_xz_m'] == holes
    assert proof['source_geometry_sha256'] == trim['source_geometry_sha256']['geo_003.glb']
    assert proof['source_geometry_sha256'] == 'fd60fa957655d289cd05909020f2f39f85e8a8e9692f3c402d9418f9b3d1a5f9'
    for key, expected in (('original_top_area_m2', 786.39), ('native_overlap_top_area_m2', 562.70),
                          ('retained_approach_top_area_m2', 223.69)):
        assert math.isclose(proof[key], expected, rel_tol=0., abs_tol=.0001), (key, proof[key])
    assert proof['cleared_inside_rays'] > 0 and proof['preserved_outside_rays'] > 0
    assert len(proof['waypoint11_footprint']) == 5
    assert all(sample['old_hit_y_m'] and sample['new_hit_y_m'] == [] for sample in proof['waypoint11_footprint'])


def verify_asphalt_native_delta(imported, project):
    """Every existing file and the single new asset are bound before staging."""
    project = project.resolve()
    asset = '/Game/MotelV2/Trim/g456.g456'
    filename = 'Content/MotelV2/Trim/g456.uasset'
    assert imported['changed_native_assets'] == [asset]
    changed = imported['changed_native_files_sha256']
    assert set(changed) == {filename}, 'asphalt delta may change only g456'
    expected = changed[filename]
    assert len(expected) == 64 and all(c in '0123456789abcdef' for c in expected)
    path = project / filename
    native = project / 'Content/MotelV2'
    assert path.resolve().is_relative_to(native.resolve()) and sha(path) == expected, 'new asphalt asset changed after import'
    actual = {str(p.relative_to(project)): sha(p) for p in sorted(native.rglob('*')) if p.is_file() and p != path}
    assert actual and imported['preserved_native_before_sha256'] == actual, (
        'complete preserved native inventory must match actual disk, excluding only new g456')


def stage(args):
    source, trims = args.source.resolve(), args.trims.resolve()
    contract, trim = read(source / 'manifest.json'), read(trims / 'trim-manifest.json')
    layout = read(args.layout)
    imported, verified = read(args.import_report), read(args.verify_report)
    assert imported['kind'] in ('native-import', 'oriented-floor-import', 'legacy-asphalt-trim-import') and verified['kind'] == 'saved-native-read-only'
    if imported['kind'] == 'legacy-asphalt-trim-import':
        verify_asphalt_native_delta(imported, args.project)
    if imported['kind'] == 'oriented-floor-import':
        # A source-audited delta imports eleven owners and re-verifies the full
        # inventory. Its additional hash gate must retain every unrelated asset.
        audit_sha = imported.get('source_orientation_audit_sha256', '')
        assert len(audit_sha) == 64 and all(c in '0123456789abcdef' for c in audit_sha)
        preserved = imported.get('preserved_native_before_sha256', {})
        assert preserved, 'oriented delta must bind its preserved native inventory'
        for filename, digest in preserved.items():
            path = args.project.resolve() / filename
            assert path.resolve().is_relative_to(args.project.resolve() / 'Content/MotelV2')
            assert sha(path) == digest, ('unrelated native changed after oriented import', filename)
    for proof in (imported, verified):
        assert proof['status'] == 'PASS' and proof['checks'] and all(c['pass'] for c in proof['checks'])
        assert proof['source_sha256']['manifest'] == sha(source / 'manifest.json')
        assert proof['source_sha256']['materials'] == sha(source / 'materials.json')
        assert proof['source_sha256']['trim_manifest'] == sha(trims / 'trim-manifest.json')
    assert imported['source_files_sha256'] == verified['source_files_sha256'], 'native proof source differs'
    for filename, digest in verified['source_files_sha256'].items():
        assert sha(filename) == digest, ('authored source changed after verification', filename)
    expected_meshes = set(contract['parts']) | {name for name, row in trim['meshes'].items() if not row['hidden']}
    for proof in (imported, verified):
        assert {r['name'] for r in proof['meshes']} == expected_meshes
    assert 0 < verified['resident_lod_budget']['actual_triangles'] <= 24000
    verify_grounding(source, contract, trim, layout)
    verify_asphalt(trim)
    assert layout['version'] == 1
    assert set(layout['retire_native']) == {'SM_Town_Motel_Shell', 'SM_Town_Motel_Glass', 'SM_Town_Motel_Office', 'SM_Town_Motel_Fixtures'}
    assert {(r['node'], r['geo'], r['parent']) for r in layout['retire_nodes']} == {(26, 19, 25), (27, 20, 25), (28, 21, 25)}
    assert set(layout['retire_props']) == {'motelBlade', 'motelNeon'}
    assert layout['retire_named_nodes'] == [
        {'node': 737, 'name': 'lic:lamp_marble', 'parent': 689}
    ], 'only the identified duplicate reception lamp group may be retired'
    moves = layout['move_interaction_nodes']
    assert len(moves) == 1
    move = moves[0]
    assert (move['handle'], move['node'], move['geo'], move['parent']) == (
        'world.motel.proxies.iceMachine', 742, 506, 662)
    assert len(move['expected_world_three_m']) == 3
    assert all(math.isclose(a, b, abs_tol=.000001) for a, b in zip(
        move['expected_world_three_m'], [-43.13, -1.398568384594018, 678.1]))
    for key in ('world_three_m', 'scale'):
        assert len(move[key]) == 3 and all(isinstance(v, (int, float)) and math.isfinite(v) for v in move[key])
    assert all(0 < v <= 2 for v in move['scale']), 'interaction proxy scale must remain bounded'
    assert set(layout['car_positions']) == {'motelCar1', 'motelCar2', 'motelCar3'}
    for value in layout['car_positions'].values():
        assert len(value) == 3 and all(math.isfinite(v) for v in value)
    for key in ('disable_colliders', 'add_colliders', 'add_zones'):
        assert layout[key]
        for value in layout[key]:
            rect(value)
    hole = [-61., -56.8, 669.8, 678.]
    assert any(all(abs(a - b) < .0001 for a, b in zip(value, hole)) for value in layout['add_colliders']), 'water needs collider'
    for floor in layout['floors']:
        rect(floor['rect'])
        assert math.isfinite(floor['y']) and overlap_area(floor['rect'], hole) < 1e-8, 'no floor across water'
    parts = {}
    saved_meshes = {row['name']: row for row in verified['meshes']}
    for name, row in contract['parts'].items():
        assert name.startswith('SM_') and '/' not in name
        parent = row.get('parent_handle', 'world.motel.group')
        site = row['site_world_three_m']
        assert parent in ('world.motel.group', 'world.room6.group')
        assert len(site) == 3 and all(isinstance(v, (int, float)) and math.isfinite(v) for v in site)
        assert len(row['lod_triangles']) == 3 and all(isinstance(n, int) and n > 0 for n in row['lod_triangles'])
        native_lods = saved_meshes[name]['lods']
        native_counts = [lod['triangles'] for lod in native_lods]
        assert len(native_counts) == 4 and native_counts[:3] == row['lod_triangles']
        assert native_counts[3] > 0 and native_lods[3]['generated_resident_lod'] is True
        assert saved_meshes[name]['ray_tracing_proxy_settings']['enabled'] is False
        parts[name] = {'asset': f'/Game/MotelV2/{name}.{name}', 'parent_handle': parent,
                       'site_world_three_m': site, 'lod_triangles': native_counts,
                       'bounds_local_m': row['bounds_local_m']}
    patches = trim['patches']
    assert len({r['node'] for r in patches}) == len(patches)
    assert {r['geo'] for r in patches} == {4, 456, 457, 459, 461, 463, 464, 467, 528, 529, 663}
    for row in patches:
        assert row['accepted_previous_tris'] and row['result_tris'] >= 0
        assert row['hidden'] == (row['result_tris'] == 0)
        if row['hidden']:
            assert not row['asset'] and not row['original_material_indices']
        else:
            assert row['asset'] == f"/Game/MotelV2/Trim/g{row['geo']}.g{row['geo']}"
    result = {key: layout[key] for key in ('version', 'retire_native', 'retire_nodes', 'retire_props',
              'retire_named_nodes', 'move_interaction_nodes',
              'disable_colliders', 'add_colliders', 'add_zones', 'floors', 'car_positions')}
    result.update(parts=parts, patches=patches,
                  provenance={'manifest_sha256': sha(source / 'manifest.json'),
                              'trim_manifest_sha256': sha(trims / 'trim-manifest.json'),
                              'runtime_layout_sha256': sha(args.layout),
                              'native_import_report_sha256': sha(args.import_report),
                              'saved_native_report_sha256': sha(args.verify_report)})
    project = args.project.resolve()
    assert project.is_dir() and (project / 'Signal47.uproject').is_file()
    destination = project / 'Content/MotelV2/assembly.json'
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        backup = args.report.resolve().parent / ('assembly.previous.' + sha(destination)[:16] + '.json')
        if not backup.exists():
            backup.write_bytes(destination.read_bytes())
    temporary = destination.with_suffix('.json.tmp')
    temporary.write_text(json.dumps(result, indent=2) + '\n')
    os.replace(temporary, destination)
    report = {'status': 'STAGED_NATIVE_VERIFIED_RUNTIME_UNVERIFIED', 'assembly': str(destination),
              'assembly_sha256': sha(destination), 'parts': len(parts), 'patches': len(patches),
              'provenance': result['provenance']}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--trims', type=Path, required=True)
    parser.add_argument('--layout', type=Path, default=Path(__file__).with_name('runtime_layout.json'))
    parser.add_argument('--import-report', type=Path, required=True)
    parser.add_argument('--verify-report', type=Path, required=True)
    parser.add_argument('--project', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    stage(parser.parse_args())
