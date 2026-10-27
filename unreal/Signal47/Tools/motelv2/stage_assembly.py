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


def stage(args):
    source, trims = args.source.resolve(), args.trims.resolve()
    contract, trim = read(source / 'manifest.json'), read(trims / 'trim-manifest.json')
    layout = read(args.layout)
    imported, verified = read(args.import_report), read(args.verify_report)
    assert imported['kind'] == 'native-import' and verified['kind'] == 'saved-native-read-only'
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
    assert layout['version'] == 1
    assert set(layout['retire_native']) == {'SM_Town_Motel_Shell', 'SM_Town_Motel_Glass', 'SM_Town_Motel_Office', 'SM_Town_Motel_Fixtures'}
    assert {(r['node'], r['geo'], r['parent']) for r in layout['retire_nodes']} == {(26, 19, 25), (27, 20, 25), (28, 21, 25)}
    assert set(layout['retire_props']) == {'motelBlade', 'motelNeon'}
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
    for name, row in contract['parts'].items():
        assert name.startswith('SM_') and '/' not in name
        parent = row.get('parent_handle', 'world.motel.group')
        site = row['site_world_three_m']
        assert parent in ('world.motel.group', 'world.room6.group')
        assert len(site) == 3 and all(isinstance(v, (int, float)) and math.isfinite(v) for v in site)
        assert len(row['lod_triangles']) == 3 and all(isinstance(n, int) and n > 0 for n in row['lod_triangles'])
        parts[name] = {'asset': f'/Game/MotelV2/{name}.{name}', 'parent_handle': parent,
                       'site_world_three_m': site, 'lod_triangles': row['lod_triangles'],
                       'bounds_local_m': row['bounds_local_m']}
    patches = trim['patches']
    assert len({r['node'] for r in patches}) == len(patches)
    assert {r['geo'] for r in patches} == {4, 457, 461, 463, 464, 467, 528, 529, 663}
    for row in patches:
        assert row['accepted_previous_tris'] and row['result_tris'] >= 0
        assert row['hidden'] == (row['result_tris'] == 0)
        if row['hidden']:
            assert not row['asset'] and not row['original_material_indices']
        else:
            assert row['asset'] == f"/Game/MotelV2/Trim/g{row['geo']}.g{row['geo']}"
    result = {key: layout[key] for key in ('version', 'retire_native', 'retire_nodes', 'retire_props',
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
