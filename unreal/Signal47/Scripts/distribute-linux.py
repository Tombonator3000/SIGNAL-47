from pathlib import Path
import hashlib, json, shutil, stat, subprocess, sys, zipfile

import argparse
parser = argparse.ArgumentParser(description="Package a verified Linux runtime without recooking it.")
parser.add_argument("source", type=Path, help="Verified package's Linux directory")
parser.add_argument("output", type=Path, help="New output directory, with BUILD.json and README_NO.md prepared")
parser.add_argument("--date", required=True)
args = parser.parse_args()
WORK = args.output.resolve()
ARCHIVES = WORK / 'archives'
ARCHIVES.mkdir(exist_ok=True)
subprocess.run([sys.executable, str(Path(__file__).with_name('unreal_guard.py')), 'require-idle'], check=True)
# ZIP output plus one extracted verification install. Never spend the disk reserve.
assert shutil.disk_usage(WORK).free >= 100_000_000_000 + 14_000_000_000, 'Need 114 GB free before distribution'
SOURCE = args.source.resolve()
ROOT = Path(__file__).resolve().parents[3]
LAUNCH = Path(__file__).resolve().parents[1] / 'Build/Linux/launch'
TAG = 'playtest-linux-' + args.date
BASE_URL = f'https://github.com/Tombonator3000/SIGNAL-47/releases/download/{TAG}'
NAMES = [f'Signal47-Linux-{args.date}{suffix}.zip' for suffix in ('', '-Data1', '-Data2')]
UCAS = 'Linux/Signal47/Content/Paks/Signal47-Linux.ucas'
EXCLUDE = {'Manifest_DebugFiles_Linux.txt', 'Signal47/Binaries/Linux/Signal47.debug', 'Signal47/Binaries/Linux/Signal47.sym'}

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda: f.read(4 * 1024 * 1024), b''):
            h.update(b)
    return h.hexdigest()

def save(name, data):
    (WORK / name).write_text(json.dumps(data, indent=2, ensure_ascii=False) + '\n')

files = {}
excluded = []
for p in sorted(SOURCE.rglob('*')):
    rel = p.relative_to(SOURCE).as_posix()
    assert not p.is_symlink(), ('unexpected symlink', rel)
    if p.is_dir():
        assert p.name not in {'Saved', 'Intermediate', 'DerivedDataCache'}, rel
        continue
    if rel in EXCLUDE:
        excluded.append(rel)
        continue
    # Allow only the staged Linux runtime, never arbitrary files next to a build.
    top_level = {'Signal47.sh', 'Check-Linux.sh', 'LINUX-README.md', 'CREDITS.txt', 'NOTICES.txt',
                 'Manifest_NonUFSFiles_Linux.txt', 'Manifest_UFSFiles_Linux.txt'}
    engine_types = {'.ini', '.html', '.bat', '.sh', '.so', '.bin', '.tps', '.ttf', '.json'}
    allowed = (rel in top_level or rel == 'Signal47/Binaries/Linux/Signal47' or
               (rel.startswith('Signal47/Content/Paks/') and p.suffix in {'.pak', '.ucas', '.utoc'}) or
               (rel.startswith('Engine/') and (p.suffix in engine_types or '.so.' in p.name)))
    assert allowed, ('non-runtime file rejected', rel)
    if rel == 'LINUX-README.md':
        p = LAUNCH / 'README.md'
    actual = digest(p)
    files['Linux/' + rel] = {'path': p, 'sha256': actual, 'bytes': p.stat().st_size}

readme = (WORK / 'README_NO.md').read_text()
ucas_sha = files[UCAS]['sha256']
play = (LAUNCH / 'Play.sh').read_text()
low = (LAUNCH / 'Play-Low.sh').read_text()
terms = '''SIGNAL / 47 playtest: included third-party content

The included licensed content may be used only as incorporated in this game,
in object-code form and as an inseparable part of the game, to play and test it.
No right to extract or redistribute proprietary third-party assets separately
is granted. Separately identified open-license works retain their own licenses.
All representations, warranties, conditions and liabilities related to the
licensed content are expressly disclaimed to the extent permitted by law.

Credits and attribution are provided in Linux/CREDITS.txt, Linux/NOTICES.txt
and the in-game credits. This notice does not replace rights under applicable
mandatory law or the separate licenses of open-license components.
'''
build = json.loads((WORK / 'BUILD.json').read_text())
assert build['runtime_source_commit'] and build['release_source_commit']
assert build['full_matrix']['status'] == 'PASS', 'Release requires the full matrix'
assert build['night_contract']['status'] == 'PASS', 'Release requires the night sequence contract'
build['distribution'] = {'archives': NAMES, 'ucas_sha256': ucas_sha}
build['payload_sha256'] = {name: info['sha256'] for name, info in files.items()}
save('BUILD.json', build)
extra = {'README_NO.md': (readme.encode(), 0o644), 'BUILD.json': ((WORK / 'BUILD.json').read_bytes(), 0o644),
         'Play.sh': (play.encode(), 0o755), 'Play-Low.sh': (low.encode(), 0o755),
         'Install-Shortcut.sh': ((LAUNCH / 'Install-Shortcut.sh').read_bytes(), 0o755),
         'icon.png': ((ROOT / 'web/src/assets/art/brand/icon_512.png').read_bytes(), 0o644),
         'UCAS.sha256': ((ucas_sha + '  ' + UCAS + '\n').encode(), 0o644),
         'THIRD_PARTY_TERMS.txt': (terms.encode(), 0o644)}
expected = {name: d['sha256'] for name, d in files.items()}
expected.update({name: hashlib.sha256(b).hexdigest() for name, (b, mode) in extra.items()})
checksums = ''.join(f'{sha}  {name}\n' for name, sha in sorted(expected.items())).encode()
extra['FILES.sha256'] = (checksums, 0o644)
expected['FILES.sha256'] = hashlib.sha256(checksums).hexdigest()
ucas_path = files[UCAS]['path']
ucas_size = files[UCAS]['bytes']
half = ucas_size // 2
parts = {UCAS + '.part1': (0, half), UCAS + '.part2': (half, ucas_size - half)}

def write_slice(z, arcname, path, offset, length):
    info = zipfile.ZipInfo(arcname)
    info.create_system = 3
    info.external_attr = (stat.S_IFREG | 0o644) << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    with path.open('rb') as f, z.open(info, 'w', force_zip64=True) as dst:
        f.seek(offset)
        left = length
        while left:
            b = f.read(min(left, 4 * 1024 * 1024))
            assert b
            dst.write(b)
            left -= len(b)

for i, archive_name in enumerate(NAMES):
    out = ARCHIVES / archive_name
    assert not out.exists(), ('archive already exists', str(out))
    print('Packing', archive_name, flush=True)
    with zipfile.ZipFile(out, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as z:
        if i == 0:
            for name, entry in files.items():
                if name != UCAS:
                    z.write(entry['path'], 'Signal47-Linux/' + name)
            for name, (b, mode) in extra.items():
                info = zipfile.ZipInfo('Signal47-Linux/' + name)
                info.create_system = 3
                info.external_attr = (stat.S_IFREG | mode) << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                z.writestr(info, b)
        else:
            name = UCAS + '.part' + str(i)
            offset, length = parts[name]
            write_slice(z, 'Signal47-Linux/' + name, ucas_path, offset, length)
    assert out.stat().st_size < 2 * 1024**3, ('GitHub 2 GiB limit', archive_name, out.stat().st_size)
    print('Packed', archive_name, out.stat().st_size, 'bytes', flush=True)

archive_expected = {k: v for k, v in expected.items() if k != UCAS}
joined = hashlib.sha256()
seen = set()
archives = []
for name in NAMES:
    path = ARCHIVES / name
    with zipfile.ZipFile(path) as z:
        for info in z.infolist():
            rel = info.filename.removeprefix('Signal47-Linux/')
            assert info.filename.startswith('Signal47-Linux/') and '..' not in Path(rel).parts
            assert rel not in seen, rel
            h = hashlib.sha256()
            size = 0
            with z.open(info) as f:
                for b in iter(lambda: f.read(4 * 1024 * 1024), b''):
                    h.update(b)
                    size += len(b)
                    if rel in parts:
                        joined.update(b)
            if rel in parts:
                assert size == parts[rel][1], ('part size', rel, size)
            else:
                assert rel in archive_expected, rel
                assert h.hexdigest() == archive_expected[rel], ('archive mismatch', rel)
            if rel in {'Play.sh', 'Play-Low.sh', 'Install-Shortcut.sh', 'Linux/Signal47.sh', 'Linux/Signal47/Binaries/Linux/Signal47'}:
                assert (info.external_attr >> 16) & 0o111, ('execute mode missing', rel)
            seen.add(rel)
    archives.append({'name': name, 'bytes': path.stat().st_size, 'sha256': digest(path), 'url': BASE_URL + '/' + name})
assert seen == set(archive_expected) | set(parts), sorted(set(archive_expected) ^ seen)
assert joined.hexdigest() == expected[UCAS], 'joined ucas mismatch'
(WORK / 'SHA256SUMS.txt').write_text(''.join(f"{a['sha256']}  {a['name']}\n" for a in archives))
save('distribution-validation.json', {'status': 'PASS', 'archives': archives, 'verified_files': len(seen),
    'crc_and_decompression': 'PASS', 'all_file_hashes': 'PASS', 'launch_permissions': 'PASS',
    'split_ucas': {'parts': {k: v[1] for k, v in parts.items()}, 'joined_sha256': expected[UCAS], 'joined_verified': True},
    'runtime_source': str(SOURCE), 'personal_data_present': False,
    'excluded': excluded, 'runtime_launch': 'See BUILD.json for tests of this exact runtime.',
    'remote_machine_tested': False})
print('PASS:', len(seen), 'archive entries verified; split ucas joins to the tested hash; no new engine run.', flush=True)
