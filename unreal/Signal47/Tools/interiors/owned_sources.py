"""Validate the small owned interior selection; optionally stage private source copies.

Ordinary Python only. Default is a dry run writing a private plan. This does not
start Blender/Unreal, export objects, bake materials or mutate the source pack.
Inventory numbers describe the historical Blender inspection, not fresh runtime.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
HOME = Path.home()
DEFAULT_SOURCE = HOME / 'Assets/vintage-liminal/VINTAGE LIMINAL PROPS PACK_v2/VINTAGE LIMINAL PROPS PACK/VINTAGE LIMINAL PROPS PACK'


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(4 * 1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def private_work(path, source):
    """Reject all Git trees, source directories and symlinked output roots."""
    raw = path.absolute()
    require(raw == raw.resolve(), 'Private work path must not contain symlinks or ..')
    require(not raw.is_relative_to(source) and not source.is_relative_to(raw),
            'Work directory must be separate from original source pack')
    existing = raw
    while not existing.exists():
        existing = existing.parent
    result = subprocess.run(['git', '-C', str(existing), 'rev-parse', '--show-toplevel'],
                            capture_output=True, text=True, check=False)
    require(result.returncode != 0, 'Refuse staging or writing a plan inside any Git working tree')
    return raw


def checked_file(root, name, expected):
    require(Path(name).name == name, 'Selection must use plain file names: ' + name)
    path = root / name
    require(path.is_file() and not path.is_symlink(), 'Missing or symlinked source: ' + str(path))
    require(digest(path) == expected, 'Source hash differs from reviewed selection: ' + str(path))
    return path


def stage(source, destination, expected):
    require(not destination.is_symlink(), 'Refuse symlinked staging target: ' + str(destination))
    if destination.exists():
        require(destination.is_file() and digest(destination) == expected,
                'Refuse overwriting changed staging file: ' + str(destination))
        return 'existing_identical'
    # An exclusive hard link publishes only the complete copy, never overwrites.
    fd, temporary = tempfile.mkstemp(prefix='.source-', dir=destination.parent)
    os.close(fd)
    try:
        shutil.copyfile(source, temporary)
        require(digest(Path(temporary)) == expected, 'Staged copy hash mismatch')
        os.link(temporary, destination)
    finally:
        Path(temporary).unlink(missing_ok=True)
    return 'copied'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=DEFAULT_SOURCE)
    parser.add_argument('--inventory-root', type=Path, default=HOME / 'scratch-models')
    parser.add_argument('--license-evidence', type=Path,
                        default=HOME / 'SIGNAL-47/web/src/assets/licensed/LICENSES.md')
    parser.add_argument('--work', type=Path,
                        default=HOME / 'scratch-bm/Signal47-interiors-20261029/owned-sources')
    parser.add_argument('--apply', action='store_true', help='Copy the three reviewed blends privately; no export')
    args = parser.parse_args()
    selection = json.loads((HERE / 'owned-selection.json').read_text())
    require(selection['schema'] == 1, 'Unsupported selection schema')
    source = args.source_root.resolve(strict=True)
    work = private_work(args.work, source)
    require(args.license_evidence.is_file(), 'Missing local license evidence')
    require(digest(args.license_evidence) == selection['license_evidence_sha256'],
            'License evidence changed; review it before updating the selection hash')
    license_text = args.license_evidence.read_text()
    require(all(word in license_text for word in ('Vintage Liminal Props Pack', 'Standard License', '19. oktober 2026')),
            'License evidence does not describe this owned pack')
    rows = []
    names = set()
    for item in selection['sources']:
        blend = checked_file(source, item['blend'], item['source_sha256'])
        inventory = checked_file(args.inventory_root, item['inventory'], item['inventory_sha256'])
        data = json.loads(inventory.read_text())
        require(data['file'] == blend.name, 'Inventory belongs to a different blend')
        objects = []
        for wanted in item['objects']:
            require(wanted['name'] not in names, 'Duplicate selected object: ' + wanted['name'])
            names.add(wanted['name'])
            matches = [obj for obj in data['assets'] if obj['name'] == wanted['name'] and obj['kind'] == 'object']
            require(len(matches) == 1, 'Object missing or ambiguous in reviewed inventory: ' + wanted['name'])
            obj = matches[0]
            require(obj['tris'] == wanted['triangles'], 'Historical object topology differs: ' + wanted['name'])
            require(bool(obj['mats']), 'Object has no inventoried materials: ' + wanted['name'])
            objects.append({**wanted, 'historical_dimensions_blender_m': obj['dims'],
                            'materials': obj['mats'], 'historical_images': obj['imgs']})
        rows.append({'source': str(blend), 'source_sha256': item['source_sha256'],
                     'bytes': blend.stat().st_size, 'inventory': str(inventory),
                     'inventory_sha256': item['inventory_sha256'], 'objects': objects})
    work.mkdir(parents=True, exist_ok=True)
    # Recheck after mkdir, before writing/copying any licensed information.
    private_work(work, source)
    for row in rows:
        row['staging'] = stage(Path(row['source']), work / Path(row['source']).name,
                               row['source_sha256']) if args.apply else 'not_copied_dry_run'
    report = {'status': 'STAGED_PRIVATE_SOURCES' if args.apply else 'DRY_RUN_VALIDATED',
              'pack': selection['pack'], 'license': selection['license'],
              'license_evidence': str(args.license_evidence.resolve()),
              'license_evidence_sha256': selection['license_evidence_sha256'],
              'selection_sha256': digest(HERE / 'owned-selection.json'),
              'inventory_scope': selection['inventory_scope'], 'sources': rows,
              'native_import': 'UNVERIFIED', 'runtime_appearance': 'UNVERIFIED',
              'next_steps': selection['next_steps']}
    report_path = work / ('stage-applied.json' if args.apply else 'stage-plan.json')
    require(not report_path.is_symlink(), 'Refuse symlinked report path')
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({'status': report['status'], 'objects': len(names),
                      'blend_files': len(rows), 'report': str(report_path)}))


if __name__ == '__main__':
    main()
