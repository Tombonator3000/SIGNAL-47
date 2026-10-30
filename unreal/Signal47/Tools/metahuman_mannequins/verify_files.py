#!/usr/bin/env python3
"""Read-only final file binding after the separate native saved-asset reader."""
import argparse
from pathlib import Path

from prepare import (clothes_bind_verification, clothing_check, file_row, imported_receipt,
                     load, operation_jobs, preservation, private_asset_file, receipt_file, sha, write_new)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--job', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    job = load(args.job)
    cfg = load(job['contract'])
    if args.out.exists():
        raise RuntimeError('Use a new receipt path')
    root = Path(job['python_report']).parent
    if job['mode'] != 'bake' or job != load(root / 'job-bake.json'):
        raise RuntimeError('Final verification requires the frozen bake job')
    if sha(job['contract']) != job['contract_sha256'] or sha(job['clothing_report']) != job['clothing_report_sha256']:
        raise RuntimeError('Private authoring source receipt changed')
    fbx, _ = clothing_check(job['clothing_report'])
    if str(fbx) != job['clothing_fbx'] or sha(fbx) != job['clothing_fbx_sha256']:
        raise RuntimeError('Actual private authoring FBX changed')
    imported = imported_receipt(job, cfg)
    material_state = imported['cloth_materials']
    receipts = {}
    operations = {}
    for mode, expected_job in operation_jobs(job, root).items():
        mode_job_path = root / ('job-' + mode + '.json')
        mode_job = load(mode_job_path)
        if mode_job != expected_job:
            raise RuntimeError('Operation job differs from the frozen common authoring contract: ' + mode)
        py = load(mode_job['python_report'])
        native = load(mode_job['native_report'])
        if (py.get('status') != 'PASS' or py.get('mode') != mode or type(py.get('pid')) is not int or py['pid'] <= 0
                or py.get('job_sha256') != sha(mode_job_path)
                or py.get('native_report') != receipt_file(mode_job['native_report'])
                or native.get('status') != 'PASS' or native.get('mode') != ('bind-clothes' if mode == 'import' else mode)
                or native.get('pid') != py['pid'] or py.get('cloth_materials') != material_state):
            raise RuntimeError('Actual operation/native/material receipt differs: ' + mode)
        operations[mode] = py
        for path in (mode_job_path, Path(mode_job['python_report']), Path(mode_job['native_report'])):
            receipts[path.name] = file_row(path)
        if mode == 'import':
            if (py['pid'] != imported['pid'] or py.get('import_receipt') != receipt_file(root / 'import-python.json')
                    or py.get('private_import') != imported):
                raise RuntimeError('Importer operation does not bind the final post-bind nine-asset receipt')
            receipts['job-import-bind-clothes.json'] = file_row(root / 'job-import-bind-clothes.json')
        else:
            verified_bind = clothes_bind_verification(mode_job, cfg, imported)
            verify_job = root / ('job-' + mode + '-verify-clothes-bind.json')
            if (verified_bind['pid'] != py['pid']
                    or py.get('clothes_bind_verify_report') != receipt_file(mode_job['clothes_bind_verify_report'])
                    or py.get('clothes_bind_verify_job') != receipt_file(verify_job)
                    or load(verify_job) != dict(mode_job, mode='verify-clothes-bind',
                                               native_report=mode_job['clothes_bind_verify_report'])
                    or py.get('clothes_bind_state') != verified_bind['clothes_bind']):
                raise RuntimeError('Fresh operation did not accept its own read-only saved clothes-bind proof: ' + mode)
            for path in (verify_job, Path(mode_job['clothes_bind_verify_report'])):
                receipts[path.name] = file_row(path)
    if len({row['pid'] for row in operations.values()}) != 4:
        raise RuntimeError('Import, probe, bake and saved reader must be four separate editor processes')
    baked, verified = operations['bake'], operations['verify']
    if baked['output_asset'] != verified['output_asset']:
        raise RuntimeError('Fresh reader did not preserve the accepted saved static output')
    original = preservation(job)
    if file_row(private_asset_file(job, job['output_asset'])) != {key: baked['output_asset'][key] for key in ('bytes', 'sha256')}:
        raise RuntimeError('Actual baked native payload changed')
    if (baked['render_context']['null_rhi'] or verified['render_context']['null_rhi']
            or set(baked['private_master_compile_errors']) != {job['cloth_master'], job['brow_material']}
            or any(baked['private_master_compile_errors'].values())):
        raise RuntimeError('Real-RHI bake/verify and both private masters without returned compile errors are required')
    receipts['import-python.json'] = file_row(root / 'import-python.json')
    write_new(args.out, {'status': 'PASS', 'kind': 'private_mannequin_authoring_files_only', 'job_sha256': sha(args.job),
                     'preservation': original, 'operation_receipts': receipts, 'output': baked['output_asset'],
                     'clothes_bind': imported['clothes_bind_state'], 'cloth_materials': material_state,
                     'private_import_assets': len(imported['saved_assets']),
                     'native_geometry': load(root / 'verify-native.json')['output'],
                     'limits': ['No runtime integration', 'No appearance/seat/contact acceptance', 'No human input or FPS',
                                'No full matrix, installation or release', 'MetaHuman derivatives remain private']})
    print('S47 MANNEQUIN FILES PASS ' + str(args.out))


if __name__ == '__main__':
    main()
