"""Aktiver ENV02 først etter separat import og lagret, lesende native-verifikasjon.

Ren Python. Alle kilder, nitten native mesh og materialbindinger kontrolleres
før assembly.json byttes atomisk. Ingen importer, motor eller TB2-filer endres.
"""
import argparse
import json
import math
import os
from pathlib import Path

import schema as S


def read(path):
    return json.loads(Path(path).read_text())


def equal_bounds(actual, blender):
    lo, hi = blender
    expected = [[lo[0]*100, -hi[1]*100, lo[2]*100], [hi[0]*100, -lo[1]*100, hi[2]*100]]
    return len(actual) == 2 and all(len(a) == 3 for a in actual) and max(abs(a-b) for p, q in zip(actual, expected) for a, b in zip(p, q)) <= .03


def validate_trim(trim):
    assert trim['version'] == S.VERSION and trim['source_revision'] == S.REVISION
    assert trim['manifest_sha256'] == S.MANIFEST_SHA256
    assert trim['source_geometry_sha256'] == {'geo_008.glb': S.GEO_SHA256}
    assert trim['source_verification']['status'] == 'PASS'
    assert trim['removed_house_total'] == 528 and trim['removed_sign_total'] == 76
    assert trim['removed_houses'] == {name: 132 for name in S.HOUSES}
    assert trim['collider_moves'] == S.collider_moves()
    assert trim['protected_handles'] == S.PROTECTED_HANDLES
    assert {int(k[1:]) for k in trim['meshes']} == set(S.PATCHES)
    assert 'g1007' not in trim['meshes'], 'H02 glass retained'
    for geo, (node, material, source, previous, removed, final) in S.PATCHES.items():
        row = trim['meshes']['g' + str(geo)]
        assert (row['node'], row['geo'], row['parent'], row['source_tris']) == (node, geo, S.PARENT, source)
        assert row['accepted_previous_tris'] == [previous] and row['result_tris'] == final
        assert row['removed_houses'] + row['removed_signs'] == removed
        assert row['hidden'] == (final == 0) and row['source_material_ids'] == [material]
        assert row['original_material_indices'] == ([0] if final else [])
        assert row['materials'] == ([f'M_Homes_Trim_g{geo}_slot0'] if final else [])
        assert row['asset'] == (f'{S.DEST}/Trim/g{geo}.g{geo}' if final else '')
    assert set(trim['signs']) == {'SM_Homes_Sign' + k for k in S.SIGNS}
    for label, sign in S.SIGNS.items():
        name, row = 'SM_Homes_Sign' + label, trim['signs']['SM_Homes_Sign' + label]
        assert row['asset'] == f'{S.DEST}/{name}.{name}' and row['triangles'] == 38
        assert row['lod_triangles'] == [38] and row['materials'] == S.sign_materials(label)
        assert row['source_nodes'] == [1448, 1449, 1450] and row['source_material_ids'] == [638, 641, 642]
        assert row['original_material_indices'] == [0, 0, 0]
        assert row['site_world_three_m'] == [8000 + sign['x'], S.terrain_low(sign['x'], sign['z']), sign['z']]
        assert len(row['ground_samples']) == 3
        for sample, dx in zip(row['ground_samples'], (0., -1.4, 1.4)):
            assert sample == {'x': sign['x'] + dx, 'z': sign['z'], 'y': S.terrain_low(sign['x'] + dx, sign['z'])}


def stage(args):
    source, trims, project = args.source.resolve(), args.trims.resolve(), args.project.resolve()
    manifest, trim = read(source / 'manifest.json'), read(trims / 'trim-manifest.json')
    contract, layout = read(source / 'source-contract.json'), read(args.layout)
    imported, verified = read(args.import_report), read(args.verify_report)
    S.validate_source_contract(contract)
    validate_trim(trim)
    assert (source / 'manifest.json').read_bytes() == (source / 'model_contract.json').read_bytes()
    assert manifest['source_revision'] == S.REVISION and manifest['no_new_admission'] is True
    assert manifest['source_contract_sha256'] == trim['source_contract_sha256'] == S.sha(source / 'source-contract.json')
    assert manifest['materials_manifest_sha256'] == S.sha(source / 'materials.json')
    assert set(manifest['parts']) == S.part_names()
    assert layout == {'version': 1, 'source_revision': S.REVISION, 'required_town_native_parts': 28,
                      'houses': list(S.HOUSES), 'part_roles': ['Shell', 'Roof', 'Fixtures'],
                      'removed_house_triangles': 528, 'removed_sign_triangles': 76,
                      'preserve_stoops': True, 'preserve_h02_glass_geo': 1007,
                      'sign_z': {'North': 1895, 'South': 2315}, 'admission': 'unchanged'}
    assert S.sha(project / 'Content/WebData/manifest.json') == S.MANIFEST_SHA256
    assert S.sha(project / 'Content/TownBuildings/town-patches.json') == trim['town_patches_sha256']
    converted = read(trims / 'trim-fbx.json')
    assert converted['status'] == 'PASS' and converted['trim_manifest_sha256'] == S.sha(trims / 'trim-manifest.json')
    converted_rows = {r['name']: r for r in converted['meshes']}
    original_rows = {name: row for name, row in trim['meshes'].items() if not row['hidden']}
    original_rows.update(trim['signs'])
    assert len(converted_rows) == len(converted['meshes']) == 7 and set(converted_rows) == set(original_rows)
    for name, row in converted_rows.items():
        expected = original_rows[name]
        assert row['triangles'] == expected['result_tris'] and row['materials'] == row['fbx_material_names'] == expected['materials']
        assert row['bounds_local_m'] == row['bounds_blender_m']
        assert max(abs(a-b) for p, q in zip(row['bounds_local_m'], expected['bounds_local_m']) for a, b in zip(p, q)) < .00002
        assert row['fbx'] == expected['fbx'] and S.sha(trims / row['fbx']) == row['fbx_sha256']
    expected_names = S.expected_meshes(trim)
    assert len(expected_names) == 19
    expected_native = {f'Content/CalicheHomes/{name}.uasset' for name in S.part_names() | set(trim['signs'])}
    expected_native |= {f'Content/CalicheHomes/Trim/{name}.uasset' for name in original_rows if name.startswith('g')}
    expected_native |= {'Content/CalicheHomes/Materials/' + name.replace('M_Homes_', 'MI_Homes_', 1) + '.uasset'
                        for part in manifest['parts'].values() for name in part['materials']}
    expected_hashes = {'manifest': S.sha(source / 'manifest.json'), 'materials': S.sha(source / 'materials.json'),
                       'source_contract': S.sha(source / 'source-contract.json'), 'trim_manifest': S.sha(trims / 'trim-manifest.json')}
    assert imported['kind'] == 'native-import' and verified['kind'] == 'saved-native-read-only'
    for proof in (imported, verified):
        assert proof['status'] == 'PASS' and proof['checks'] and all(c['pass'] is True for c in proof['checks'])
        assert proof['source_sha256'] == expected_hashes
        assert len(proof['meshes']) == 19 and {r['name'] for r in proof['meshes']} == expected_names
        for filename, digest in proof['source_files_sha256'].items():
            assert S.sha(filename) == digest, ('source changed', filename)
        assert proof['native_files_sha256'], 'saved native file inventory required'
        assert expected_native <= set(proof['native_files_sha256']), 'all nineteen meshes and their material instances need saved fingerprints'
        for filename, digest in proof['native_files_sha256'].items():
            path = (project / filename).resolve()
            assert path.is_relative_to(project / 'Content/CalicheHomes')
            assert S.sha(path) == digest, ('saved native changed', filename)
    assert imported['source_files_sha256'] == verified['source_files_sha256']
    assert imported['native_files_sha256'] == verified['native_files_sha256']
    assert imported['meshes'] == verified['meshes'], 'native geometry/materials differ between sessions'
    saved = {r['name']: r for r in verified['meshes']}
    parts = {}
    for name, row in manifest['parts'].items():
        home = name.split('_')[2]
        assert row['house'] == home and row['chunk'] == name.split('_')[3]
        assert row['site_world_three_m'] == S.HOUSES[home]['root'] and row['offset'] == [0, 0, 0]
        assert row['parent_handle'] == 'world.town.group'
        counts = row['lod_triangles']
        assert len(counts) == 3 and all(isinstance(n, int) and n > 0 for n in counts)
        assert len(row['lods']) == 3 and [r['triangles'] for r in row['lods']] == counts
        native = saved[name]
        assert native['asset'] == f'{S.DEST}/{name}.{name}' and native['materials'] == row['materials']
        expected_materials = [f"{S.DEST}/Materials/{m.replace('M_Homes_', 'MI_Homes_', 1)}.{m.replace('M_Homes_', 'MI_Homes_', 1)}" for m in row['materials']]
        assert native['material_assets'] == expected_materials
        assert len(native['lods']) == 3 and len(native['source_positions_sha256']) == 3
        for actual, expected in zip(native['lods'], row['lods']):
            assert actual['triangles'] == expected['triangles'] and equal_bounds(actual['bounds_cm'], expected['bounds_local_m'])
        parts[name] = {'asset': native['asset'], 'parent_handle': row['parent_handle'], 'site_world_three_m': row['site_world_three_m'],
                       'lod_triangles': counts, 'bounds_local_m': row['bounds_local_m'], 'materials': row['materials'],
                       'material_assets': native['material_assets']}
    for name, row in original_rows.items():
        native = saved[name]
        assert native['asset'] == row['asset'] and native['materials'] == row['materials']
        assert len(native['lods']) == len(native['source_positions_sha256']) == 1
        assert native['lods'][0]['triangles'] == row['result_tris'] and equal_bounds(native['lods'][0]['bounds_cm'], row['bounds_local_m'])
    required_files = {str((source / name).resolve()) for name in ('manifest.json', 'materials.json', 'source-contract.json')}
    required_files |= {str((source / row['fbx']).resolve()) for part in manifest['parts'].values() for row in part['lods']}
    required_files |= {str((trims / row['fbx']).resolve()) for row in converted_rows.values()}
    required_files |= {str((trims / name).resolve()) for name in ('trim-manifest.json', 'trim-fbx.json')}
    assert required_files <= set(verified['source_files_sha256'])
    result = {key: trim[key] for key in ('version', 'source_revision', 'manifest_sha256', 'manifest_sha1', 'town_patches_sha256',
                                        'town_patches_sha1', 'meshes', 'signs', 'collider_moves', 'protected_nodes', 'protected_handles')}
    result.update(status='STAGED_NATIVE_VERIFIED_RUNTIME_UNVERIFIED', parts=parts, required_town_native_parts=28,
                  removed_house_total=528, removed_sign_total=76,
                  provenance={**expected_hashes, 'layout': S.sha(args.layout), 'native_import_report': S.sha(args.import_report),
                              'saved_native_report': S.sha(args.verify_report)})
    destination = project / 'Content/CalicheHomes/assembly.json'
    assert (project / 'Signal47.uproject').is_file()
    destination.parent.mkdir(parents=True, exist_ok=True)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        backup = args.report.parent / ('assembly.previous.' + S.sha(destination)[:16] + '.json')
        if not backup.exists():
            backup.write_bytes(destination.read_bytes())
    temporary = destination.with_suffix('.json.tmp')
    temporary.write_text(json.dumps(result, indent=2) + '\n')
    os.replace(temporary, destination)
    report = {'status': result['status'], 'assembly': str(destination), 'assembly_sha256': S.sha(destination),
              'parts': 12, 'patches': 8, 'signs': 2, 'native_meshes': 19, 'provenance': result['provenance']}
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


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
