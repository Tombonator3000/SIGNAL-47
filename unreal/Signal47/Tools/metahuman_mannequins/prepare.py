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


def write_new(path, value):
    """Immutable jobs/receipts: never replace a previous attempt, including failures."""
    with Path(path).open('x', encoding='utf-8') as stream:
        stream.write(json.dumps(value, ensure_ascii=False, indent=2) + '\n')


def file_row(path):
    path = Path(path)
    return {'bytes': path.stat().st_size, 'sha256': sha(path)}


def receipt_file(path):
    return {'path': str(Path(path).resolve(strict=True)), **file_row(path)}


def native_file_row(path):
    """FSHA1 is available in the Linux editor; Python additionally binds SHA256."""
    path = Path(path).resolve(strict=True)
    digest = hashlib.sha1()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(4 * 1024 * 1024), b''):
            digest.update(block)
    return {'path': str(path), 'bytes': path.stat().st_size, 'sha1': digest.hexdigest()}


def private_asset_file(job, asset):
    if not asset.startswith(job['asset_root'] + '/') or '..' in asset:
        raise RuntimeError('Asset is outside this fresh private run: ' + asset)
    return Path(job['project']) / 'Content' / (asset.removeprefix('/Game/') + '.uasset')


def import_assets(job, cfg):
    assets = [job['private_skeleton'], job['clothes_asset'], job['brow_material'], job['cloth_master']]
    assets += [job['asset_root'] + '/' + name for name in cfg['materials']]
    if len(assets) != len(set(assets)) or len(assets) != 9:
        raise RuntimeError('Exactly nine private import assets are required')
    return assets


def clothes_bind_receipt(job, cfg):
    path = Path(job['clothes_bind_report'])
    data = load(path)
    state = data['clothes_bind']
    policy = cfg['clothes_bind']
    expected_file = private_asset_file(job, job['clothes_asset']).resolve(strict=True)
    expected_object = job['clothes_asset'] + '.' + job['clothes_asset'].rsplit('/', 1)[1]
    if (data.get('schema_version') != 1 or data.get('status') != 'PASS' or data.get('mode') != 'bind-clothes'
            or type(data.get('pid')) is not int or data['pid'] <= 0
            or state['asset'] != job['clothes_asset'] or state['object'] != expected_object
            or state['private_skeleton'] != job['private_skeleton']
            or policy['policy'] != 'canonical-native-body' or state['policy'] != policy['policy']
            or state['imported_reference_pose_sha1'] != policy['imported_reference_pose_sha1']
            or state['canonical_reference_pose_sha1'] != policy['canonical_reference_pose_sha1']
            or state['reference_before_sha1'] != policy['imported_reference_pose_sha1']
            or state['reference_after_sha1'] != policy['canonical_reference_pose_sha1']
            or state['body_reference_sha1'] != policy['canonical_reference_pose_sha1']
            or state['preserved_payload'] is not True
            or not state['payload_before_sha1'] or state['payload_before_sha1'] != state['payload_after_sha1']
            or not state['inverse_reference_after_sha1'] or state['inverse_reference_after_sha1'] != state['body_inverse_reference_sha1']
            or state['file_after'] != native_file_row(expected_file)):
        raise RuntimeError('Actual saved clothes rebind receipt/file/reference/payload differs')
    return data


def clothes_bind_verification(job, cfg, imported=None):
    bound = clothes_bind_receipt(job, cfg)
    verified = load(job['clothes_bind_verify_report'])
    if (verified.get('schema_version') != 1 or verified.get('status') != 'PASS'
            or verified.get('mode') != 'verify-clothes-bind' or type(verified.get('pid')) is not int
            or verified['pid'] <= 0 or verified['pid'] == bound['pid']
            or verified.get('bind_receipt') != native_file_row(job['clothes_bind_report'])
            or verified.get('clothes_bind') != bound['clothes_bind']
            or (imported is not None and verified['pid'] == imported['pid'])):
        raise RuntimeError('Fresh-process read-only clothes-bind verification differs')
    return verified


def imported_receipt(job, cfg):
    root = Path(job['python_report']).parent
    imported = load(root / 'import-python.json')
    saved = imported['saved_assets']
    before = imported['assets_before_bind_sha256']
    expected = import_assets(job, cfg)
    bound = clothes_bind_receipt(job, cfg)
    import_job = load(root / 'job-import.json')
    bind_job = root / 'job-import-bind-clothes.json'
    if (imported.get('status') != 'PASS' or imported.get('mode') != 'import'
            or type(imported.get('pid')) is not int or imported['pid'] != bound['pid']
            or imported.get('job_sha256') != sha(root / 'job-import.json')
            or import_job != operation_jobs(job, root)['import']
            or imported['clothing_fbx_sha256'] != job['clothing_fbx_sha256']
            or len(saved) != 9 or {row['asset'] for row in saved} != set(expected)
            or set(before) != set(expected)
            or any(row['file'] != before[row['asset']] for row in saved if row['asset'] != job['clothes_asset'])
            or imported.get('clothes_bind_native_report') != receipt_file(job['clothes_bind_report'])
            or imported.get('clothes_bind_job') != receipt_file(bind_job)
            or load(bind_job) != dict(import_job, mode='bind-clothes', native_report=job['clothes_bind_report'])
            or imported.get('clothes_bind_state') != bound['clothes_bind']):
        raise RuntimeError('Final nine-asset import receipt must follow the exact accepted native bind')
    for row in saved:
        if file_row(private_asset_file(job, row['asset'])) != row['file']:
            raise RuntimeError('Private import asset changed after final bind receipt: ' + row['asset'])
    return imported


def operation_jobs(job, output):
    """Import/bind is a separate process; each later read-only bind check has its own file."""
    result = {}
    for mode in ('import', 'probe', 'bake', 'verify'):
        result[mode] = dict(job, mode=mode,
                            clothes_bind_report=str(output / 'clothes-bind-native.json'),
                            clothes_bind_verify_report=str(output / (mode + '-clothes-bind-verify-native.json')),
                            native_report=str(output / ('clothes-bind-native.json' if mode == 'import' else mode + '-native.json')),
                            python_report=str(output / ('import-operation-python.json' if mode == 'import' else mode + '-python.json')))
    return result


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


def clothing_check(report, fresh_source_report=None):
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
    if 'coat_yoke' in data:
        # R4 is not a new nine-asset import shortcut. Only the dedicated
        # two-source-asset helper supplies this independently measured reader.
        if fresh_source_report is None:
            raise RuntimeError('R4 upper-Coat source requires its separate actual fresh source reader')
        fresh = load(fresh_source_report)
        yoke = data['coat_yoke']
        actual_row = dict(path=str(Path(report).resolve(strict=True)), **file_row(report),
                          sha1=native_file_row(report)['sha1'])
        if (yoke.get('policy') != 'actual-combined-skin-local-coat-yoke-v1'
                or yoke.get('status') != 'REST_YOKE_MEASURED_NOT_SEATED_OR_NATIVE_ACCEPTED'
                or yoke.get('mutable_part') != 'Coat_body_open_lower_front'
                or yoke.get('preserved_original_parts') != 16
                or len(yoke.get('preserved_parts', {})) != 16
                or data.get('pid', 0) <= 0
                or fresh.get('status') != 'FRESH_SOURCE_SAMPLED_PASS_NOT_NATIVE_OR_APPEARANCE'
                or fresh.get('pid', 0) <= 0 or fresh.get('pid') == data['pid']
                or fresh.get('producer_pid') != data['pid']
                or fresh.get('source_report') != actual_row
                or fresh.get('asset_save_called') is not False
                or fresh.get('source_mutated') is not False
                or fresh.get('source_reference_sha256') != data['rest_skeleton_sha256']
                or fresh.get('garment_payload_snapshot') != data['garment_payload_snapshot']):
            raise RuntimeError('R4 requires actual upper-Coat+16-preserved and separate saved-source proof')
        for fit in (yoke['fit'], data['saved_reopened_fit'], fresh['actual_fit']):
            if (fit.get('status') != 'REST_YOKE_SAMPLED_NOT_SEATED_OR_NATIVE_ACCEPTED'
                    or fit.get('minimum_clearance_m') != .006
                    or fit.get('upper_facet_samples') != 10752
                    or fit.get('upper_facet_minimum_m', -1) < .006
                    or fit.get('required_skin_rays', 0) <= 0
                    or fit.get('required_skin_minimum_m', -1) < .006
                    or fit.get('continuous_surface_proof') is not False):
                raise RuntimeError('R4 sampled source proof/unchanged6mm gate is missing')
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
           'cloth_master': asset_root + '/' + load(contract)['cloth_surface']['master_name'],
           'output_asset': asset_root + '/SM_RadioWoman_Seated', 'native_report': str(output / 'bake-native.json'),
           'bake_report': str(output / 'bake-native.json'), 'python_report': str(output / 'bake-python.json')}
    preservation(job)
    for mode, variant in operation_jobs(job, output).items():
        write_new(output / ('job-' + mode + '.json'), variant)
    print(json.dumps({'status': 'PREPARED_NOT_PRODUCED', 'backed_up_files': len(before), 'job': str(output / 'job-bake.json'),
                      'first_job': str(output / 'job-import.json'), 'engine_started': False}, ensure_ascii=False))


if __name__ == '__main__':
    main()
