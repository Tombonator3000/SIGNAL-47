#!/usr/bin/env python3
"""CPU-only preflight, separate reflink backup and job contract. Never starts UE/DCC."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
PIN = '292e178b06f0d172d3a43220bf3ac42270e500671a6a8eec5969465348bc3e92'
PRIVATE = Path.home() / 'scratch-bm/Signal47-mannequins-20261029/private'


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(4 * 1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def load(path):
    return json.loads(Path(path).read_text())


def write(path, value):
    Path(path).write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n')


def file_row(path):
    path = Path(path)
    return {'bytes': path.stat().st_size, 'sha256': sha(path)}


def source_check(inventory, project):
    """Pin the current runtime, not the older Creator staging import."""
    data = load(inventory)
    rows = {}
    for key, item in data['artifacts'].items():
        if key.startswith('evening_worktree_') or key in ('creator_character', 'body_fbx'):
            path = Path(item['path']).resolve(strict=True)
            if key.startswith('evening_worktree_') and not path.is_relative_to(project):
                raise RuntimeError('Inventory points to another runtime worktree: ' + key)
            actual = file_row(path)
            if actual['sha256'] != item['sha256'] or actual['bytes'] != item['bytes']:
                raise RuntimeError('Pinned source changed: ' + key)
            rows[key] = {'path': str(path), **actual}
    required = {'creator_character', 'body_fbx', 'evening_worktree_body', 'evening_worktree_face',
                'evening_worktree_skeleton', 'evening_worktree_hair_cards_lod1',
                'evening_worktree_hair_cards_lod3', 'evening_worktree_brow_cards_lod1',
                'evening_worktree_hair_card_material'}
    if not required.issubset(rows) or rows['body_fbx']['sha256'] != PIN:
        raise RuntimeError('Missing current female source pins')
    return rows


def clothing_check(report):
    data = load(report)
    if (data.get('status') != 'REST_CLOTHES_EXPORTED_NOT_ACCEPTED' or not data.get('blend_reopened')
            or data.get('input_body_sha256') != PIN or data.get('input_body_sha256_after') != PIN
            or data.get('source_body_geometry_exported') is not False):
        raise RuntimeError('No actual private rest-clothes export/reopen receipt')
    skeleton = data['rest_skeleton']
    actual = hashlib.sha256(json.dumps(skeleton, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != data['rest_skeleton_sha256'] or not skeleton['bones']:
        raise RuntimeError('Clothing skeleton receipt is inconsistent')
    expected = {'axis_forward': '-Z', 'axis_up': 'Y', 'global_scale': 1.0, 'apply_unit_scale': True,
                'bake_space_transform': False, 'automatic_bone_orientation_on_source_import': False,
                'add_leaf_bones': False, 'bake_anim': False, 'use_armature_deform_only': False,
                'body_geometry_exported': False, 'reference_pose_changed': False}
    if any(data['fbx_policy'].get(key) != value for key, value in expected.items()):
        raise RuntimeError('Rest-clothes FBX basis or skeleton contract changed')
    exported = data['output_files_sha256']
    for path, expected_sha in exported.items():
        if sha(path) != expected_sha:
            raise RuntimeError('Actual private clothes output changed: ' + path)
    paths = [Path(path).resolve(strict=True) for path in exported if Path(path).name == 'SKM_RadioWoman_Clothes.fbx']
    if len(paths) != 1 or data['triangles'] > 24000 or data['triangles'] <= 0:
        raise RuntimeError('Clothing export identity/count differs')
    weights = data['weights']
    if (weights['unweighted_vertices'] or weights['vertices_over_four_influences']
            or weights['unknown_bones'] or weights['maximum_weight_sum_error'] > 1e-5):
        raise RuntimeError('Clothing weight audit failed')
    recipe = load(HERE / 'radio_woman.json')['materials']
    if {item['name'] for item in data['materials']} != set(recipe):
        raise RuntimeError('Clothing material identity differs')
    for item in data['materials']:
        expected_mat = recipe[item['name']]
        if (item['base_color_linear'] != expected_mat['color'] or item['roughness'] != expected_mat['roughness']
                or item['metallic'] != 0):
            raise RuntimeError('Clothing palette differs')
    return paths[0], data


def preservation(job):
    """All pre-existing MetaHumans files must stay byte-identical, including the waitress."""
    if sha(job['source_snapshot']) != job['source_snapshot_sha256']:
        raise RuntimeError('Frozen original-file inventory changed')
    data = load(job['source_snapshot'])
    root = Path(data['source_root'])
    expected = data['files']
    allowed = Path('Mannequins/RadioWoman') / job['run_id']
    for name, pin in data['pins'].items():
        if file_row(pin['path']) != {key: pin[key] for key in ('bytes', 'sha256')}:
            raise RuntimeError('Pinned original/Creator/FBX changed: ' + name)
    for relative, row in expected.items():
        if file_row(root / relative) != row:
            raise RuntimeError('Original MetaHuman file changed: ' + relative)
    extras = []
    for path in root.rglob('*'):
        if path.is_symlink():
            raise RuntimeError('Source symlink is not allowed: ' + str(path))
        if path.is_file():
            relative = path.relative_to(root)
            if relative.as_posix() not in expected:
                if not relative.is_relative_to(allowed):
                    raise RuntimeError('Unexpected source-side write: ' + str(relative))
                extras.append({'path': relative.as_posix(), **file_row(path)})
    return {'status': 'PASS', 'original_files_unchanged': len(expected), 'new_private_files': extras}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, default=HERE.parents[1])
    parser.add_argument('--inventory', type=Path, required=True)
    parser.add_argument('--clothing-report', type=Path)
    parser.add_argument('--out', type=Path)
    parser.add_argument('--check-source', action='store_true')
    args = parser.parse_args()
    project = args.project.expanduser().resolve(strict=True)
    pins = source_check(args.inventory, project)
    if args.check_source:
        print(json.dumps({'status': 'PASS', 'pinned_sources': len(pins), 'engine_started': False}))
        return
    if args.out is None or args.clothing_report is None:
        parser.error('--out and --clothing-report are required for a new private authoring job')
    output = args.out.expanduser().resolve()
    if not output.is_relative_to(PRIVATE.resolve()) or output == PRIVATE.resolve() or output.exists():
        raise RuntimeError('Use a fresh child of ' + str(PRIVATE))
    if any((parent / '.git').exists() for parent in [output, *output.parents]):
        raise RuntimeError('Private backup cannot be placed inside a Git worktree')
    if not re.fullmatch('[A-Za-z][A-Za-z0-9_]{0,63}', output.name):
        raise RuntimeError('Output directory name must also be a safe native package leaf')
    fbx, cloth = clothing_check(args.clothing_report)
    source = project / 'Content/MetaHumans'
    paths = sorted(path for path in source.rglob('*') if path.is_file())
    if any(path.is_symlink() for path in source.rglob('*')):
        raise RuntimeError('Do not follow source-side symlinks')
    before = {path.relative_to(source).as_posix(): file_row(path) for path in paths}
    output.mkdir(parents=True)
    backup = output / 'original-MetaHumans'
    backup.mkdir()
    for path in paths:
        target = backup / path.relative_to(source)
        target.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(['cp', '--reflink=auto', '--preserve=mode,timestamps', '--', str(path), str(target)], check=True)
        if (path.stat().st_dev, path.stat().st_ino) == (target.stat().st_dev, target.stat().st_ino):
            raise RuntimeError('Backup shares a hardlink with source')
        if file_row(target) != before[path.relative_to(source).as_posix()]:
            raise RuntimeError('Backup was not byte-identical')
    write(output / 'source-files.json', {'schema_version': 1, 'source_root': str(source), 'backup_root': str(backup),
                                       'files': before, 'pins': pins, 'inventory_sha256': sha(args.inventory)})
    contract = HERE / 'radio_woman.json'
    (output / 'radio_woman.json').write_bytes(contract.read_bytes())
    (output / 'clothing-source-receipt.json').write_bytes(args.clothing_report.read_bytes())
    asset_root = '/Game/MetaHumans/Mannequins/RadioWoman/' + output.name
    job = {'schema_version': 1, 'run_id': output.name, 'project': str(project), 'mode': 'bake',
           'contract': str(output / 'radio_woman.json'), 'contract_sha256': sha(contract),
           'source_snapshot': str(output / 'source-files.json'), 'source_snapshot_sha256': sha(output / 'source-files.json'),
           'clothing_report': str(output / 'clothing-source-receipt.json'),
           'clothing_report_sha256': sha(args.clothing_report), 'clothing_fbx': str(fbx), 'clothing_fbx_sha256': sha(fbx),
           'asset_root': asset_root, 'clothes_asset': asset_root + '/SKM_RadioWoman_Clothes',
           'private_skeleton': asset_root + '/SK_RadioWoman_Source', 'brow_material': asset_root + '/M_RadioWoman_BrowCards',
           'output_asset': asset_root + '/SM_RadioWoman_Seated', 'native_report': str(output / 'bake-native.json'),
           'bake_report': str(output / 'bake-native.json'), 'python_report': str(output / 'bake-python.json')}
    preservation(job)
    write(output / 'job-bake.json', job)
    for mode in ('probe', 'verify'):
        variant = dict(job, mode=mode, native_report=str(output / (mode + '-native.json')),
                       python_report=str(output / (mode + '-python.json')))
        write(output / ('job-' + mode + '.json'), variant)
    print(json.dumps({'status': 'PREPARED_NOT_PRODUCED', 'backed_up_files': len(before), 'job': str(output / 'job-bake.json'),
                      'engine_started': False}, ensure_ascii=False))


if __name__ == '__main__':
    main()
