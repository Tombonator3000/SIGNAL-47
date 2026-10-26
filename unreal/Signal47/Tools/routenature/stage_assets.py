"""Stage only selected, already-owned package dependencies into ignored local Content.

Ordinary Python, no engine launch. Dry run by default. Source packs are immutable.
The length-prefixed package-name closure is conservative; native import reports the
actual AssetRegistry closure. No maps, vendor config or plugin installation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]
REPO = PROJECT.parents[1]
WORK = Path(os.environ.get('S47_RN_WORK', str(Path.home() / 'scratch-bm/Signal47-terrain-20261026/assets')))
DEFAULTS = {
    'desert': Path.home() / 'scratch-bm/Signal47-arrival-office-20261026/fab-plugin/FabStaging/Content',
    'oldwest': Path.home() / '.local/share/com.epicgames.eglv2/FabLibrary/Old West Learning-785da98f/Content',
}

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(4 * 1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()

def package_names(data):
    # Exclude arbitrary strings in AssetImportData, including /Game/Game/TGA paths.
    return sorted({m.group(0)[:-1].decode() for m in re.finditer(rb'/Game/[A-Za-z0-9_/]+\x00', data)
                   if m.start() >= 4 and int.from_bytes(data[m.start()-4:m.start()], 'little', signed=True) == len(m.group(0))})

def ignored(path):
    result = subprocess.run(['git', 'check-ignore', '-q', str(path)], cwd=REPO, check=False)
    if result.returncode != 0:
        raise RuntimeError('Private content must be gitignored before staging: ' + str(path))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    config = json.loads((HERE / 'selection.json').read_text())
    roots = {k: Path(os.environ.get(v['content_env'], str(DEFAULTS[k]))).resolve()
             for k, v in config['source_packs'].items()}
    queue = [(row['source_pack'], row['source_package']) for row in config['meshes']]
    rows = {}
    while queue:
        pack, package = queue.pop()
        if package in rows:
            continue
        if not package.startswith(('/Game/Northwood/', '/Game/JustBStudios/')):
            raise RuntimeError('Unexpected dependency outside approved private vendor roots: ' + package)
        relative = Path(package.removeprefix('/Game/') + '.uasset')
        source = roots[pack] / relative
        if not source.is_file():
            raise FileNotFoundError(source)
        blob = source.read_bytes()
        deps = [name for name in package_names(blob) if name != package]
        files = [source] + [p for p in source.parent.glob(source.stem + '.*')
                            if p.suffix in ('.ubulk', '.uexp', '.uptnl')]
        entries = []
        for file in files:
            rel = file.relative_to(roots[pack])
            dest = PROJECT / 'Content' / rel
            ignored(dest)
            entries.append({'relative_file': str(rel), 'sha256': digest(file), 'bytes': file.stat().st_size})
        rows[package] = {'package': package, 'source_pack': pack, 'dependencies': deps, 'files': entries}
        queue.extend((pack, dep) for dep in deps)
    for selected in config['meshes']:
        assert rows[selected['source_package']]['files'][0]['sha256'] == selected['source_sha256'], selected['name']
    report = {'status': 'DRY_RUN', 'method': 'Conservative serialized package-name closure; native registry checks follow.',
              'source_roots': {k: str(v) for k, v in roots.items()}, 'destination': str(PROJECT / 'Content'),
              'packages': list(sorted(rows.values(), key=lambda row: row['package'])), 'copied': [], 'existing_identical': []}
    if args.apply:
        for row in report['packages']:
            for item in row['files']:
                source = roots[row['source_pack']] / item['relative_file']
                dest = PROJECT / 'Content' / item['relative_file']
                if dest.exists():
                    assert digest(dest) == item['sha256'], ('Refuse overwriting existing changed staging package', str(dest))
                    report['existing_identical'].append(str(dest))
                else:
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(source, dest)
                    assert digest(dest) == item['sha256'], str(dest)
                    report['copied'].append(str(dest))
                assert digest(source) == item['sha256'], ('Original source changed', str(source))
        report['status'] = 'STAGED_PRIVATE_SOURCE_UNCHANGED'
    WORK.mkdir(parents=True, exist_ok=True)
    report['files'] = sum(len(row['files']) for row in rows.values())
    report['bytes'] = sum(f['bytes'] for row in rows.values() for f in row['files'])
    (WORK / ('stage-applied.json' if args.apply else 'stage-plan.json')).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('status', 'files', 'bytes')}))

if __name__ == '__main__':
    main()
