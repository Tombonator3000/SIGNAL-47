#!/usr/bin/env python3
"""Read-only final file binding after the separate native saved-asset reader."""
import argparse
from pathlib import Path

from prepare import file_row, load, preservation, sha, write


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--job', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    job = load(args.job)
    if args.out.exists():
        raise RuntimeError('Use a new receipt path')
    root = Path(job['python_report']).parent
    baked, verified = load(root / 'bake-python.json'), load(root / 'verify-python.json')
    if (baked['status'] != 'PASS' or verified['status'] != 'PASS' or baked['pid'] == verified['pid']
            or baked['output_asset'] != verified['output_asset']):
        raise RuntimeError('Actual separate-process native bake/reader receipts are required')
    receipts = {}
    for name in ('import-python.json', 'bake-python.json', 'bake-native.json', 'verify-python.json', 'verify-native.json'):
        path = root / name
        if load(path)['status'] != 'PASS':
            raise RuntimeError('Rejected actual authoring operation: ' + name)
        receipts[name] = file_row(path)
    for name in ('bake', 'verify'):
        py = load(root / (name + '-python.json'))
        native = root / (name + '-native.json')
        if file_row(native) != {key: py['native_report'][key] for key in ('bytes', 'sha256')}:
            raise RuntimeError('Native receipt changed after Python acceptance')
    original = preservation(job)
    asset = Path(job['project']) / 'Content' / (job['output_asset'].removeprefix('/Game/') + '.uasset')
    if file_row(asset) != {key: baked['output_asset'][key] for key in ('bytes', 'sha256')}:
        raise RuntimeError('Actual baked native payload changed')
    saved = load(root / 'import-python.json')['saved_assets']
    for item in saved:
        path = Path(job['project']) / 'Content' / (item['asset'].removeprefix('/Game/') + '.uasset')
        if file_row(path) != item['file']:
            raise RuntimeError('Private import/material changed since authoring')
    write(args.out, {'status': 'PASS', 'kind': 'private_mannequin_authoring_files_only', 'job_sha256': sha(args.job),
                     'preservation': original, 'operation_receipts': receipts, 'output': baked['output_asset'],
                     'native_geometry': load(root / 'verify-native.json')['output'],
                     'limits': ['No runtime integration', 'No appearance/seat/contact acceptance', 'No human input or FPS',
                                'No full matrix, installation or release', 'MetaHuman derivatives remain private']})
    print('S47 MANNEQUIN FILES PASS ' + str(args.out))


if __name__ == '__main__':
    main()
