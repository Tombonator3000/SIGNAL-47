#!/usr/bin/env python3
"""Snapshot or install the accepted cow candidate. Default installation is validation only.

Adapted from the measured field-model/T02 filesystem helpers: no Git mutation,
build, launch, process signalling, pruning, or Saved replacement. --write is
required for a full reflink rollback backup and actual payload installation.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import subprocess
import sys
import uuid

PROJECT = Path(__file__).resolve().parents[2]
TARGET = Path.home() / 'Games/Signal47/Linux'
BINARY = 'Signal47/Binaries/Linux/Signal47'
REQUIRED_CASES = {'selftest', 'pasture', 'pasturemissing', 'ninth'}
MESHES = {'SM_Cow_Alive_Body', 'SM_Cow_Alive_Head', 'SM_Cow_Abducted'}
ASSET_PREFIX = '/Game/Signal47/Art/Cows/'
ERRORS = re.compile(r'Critical error:|Fatal error:|Assertion failed:|VK_ERROR_DEVICE_LOST|Signal \d+ caught|Ensure condition failed|self test FAILED|smoke test FAILED|Default Material will be used in game')


def require(ok, text):
    if not ok:
        raise RuntimeError(text)


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def write(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2) + '\n')
    temporary.replace(path)


def snapshot(root, saved=False):
    """Hash all regular nodes, permissions, link targets and extended attributes.

    Include empty directories. Never follow symlinks, including Saved symlinks.
    Saved is selected by any complete path component, matching rsync exclusion.
    """
    root = Path(root)
    require(root.is_dir() and not root.is_symlink(), 'Expected real directory: ' + str(root))
    paths = [root]
    for current, dirs, files in os.walk(root, followlinks=False):
        paths.extend(Path(current) / name for name in sorted(dirs + files))
    records = {}
    for path in paths:
        rel = path.relative_to(root)
        if ('Saved' in rel.parts) != saved:
            continue
        info = path.lstat()
        row = {'mode': stat.S_IMODE(info.st_mode), 'uid': info.st_uid, 'gid': info.st_gid}
        row['xattrs'] = {name: hashlib.sha256(os.getxattr(path, name, follow_symlinks=False)).hexdigest()
                         for name in sorted(os.listxattr(path, follow_symlinks=False))}
        if stat.S_ISDIR(info.st_mode):
            row['directory'] = True
        elif stat.S_ISLNK(info.st_mode):
            row['link'] = os.readlink(path)
        elif stat.S_ISREG(info.st_mode):
            row.update(sha256=sha(path), bytes=info.st_size)
        else:
            raise RuntimeError('Unsupported filesystem node: ' + str(path))
        records[rel.as_posix()] = row
    return dict(sorted(records.items()))


def source_files(root):
    root = Path(root)
    require(root.is_dir() and any(root.glob('*.uproject')), 'Source root must contain the tested .uproject')
    files = list(root.glob('*.uproject'))
    for name in ('Source', 'Config', 'Content', 'Shaders'):
        files.extend(path for path in (root / name).rglob('*') if path.is_file())
    return {str(path.relative_to(root)): sha(path) for path in sorted(files)}


def idle():
    sys.path.insert(0, str(PROJECT / 'Scripts'))
    from unreal_guard import require_idle
    grace = os.environ.pop('S47_INSTALLED_GAME_GRACE', None)
    try:
        value = require_idle()
        require(not value.get('heavy_jobs') and not value.get('exempted_installed_game'), 'Strict idle required')
        return value
    finally:
        if grace is not None:
            os.environ['S47_INSTALLED_GAME_GRACE'] = grace


def reflink_backup(source, target, expected):
    """Copy only payload nodes. Every regular file must be a successful reflink."""
    target.mkdir()
    directories = [name for name, row in expected.items() if row.get('directory')]
    for name in sorted(directories, key=lambda item: len(PurePosixPath(item).parts)):
        (target / name).mkdir(exist_ok=True)
    for name, row in expected.items():
        if row.get('directory'):
            continue
        subprocess.run(['cp', '-a', '--reflink=always', '--', str(source / name), str(target / name)], check=True)
    for name in sorted(directories, key=lambda item: len(PurePosixPath(item).parts), reverse=True):
        info = (source / name).stat()
        os.chown(target / name, info.st_uid, info.st_gid)
        shutil.copystat(source / name, target / name, follow_symlinks=False)
    require(snapshot(target) == expected and not snapshot(target, saved=True), 'Full reflink backup verification failed')


def sync_payload(source, target):
    # This unanchored exclusion protects every receiver Saved node from --delete.
    subprocess.run(['rsync', '-aX', '--checksum', '--delete', '--exclude=Saved', str(source) + '/', str(target) + '/'], check=True)


def validate(proof_path, expected_sha):
    proofs = {}

    def bind(row):
        require(isinstance(row, dict) and set(row) >= {'path', 'sha256'}, 'Expected {path, sha256} proof reference')
        path = Path(row['path']).expanduser()
        path = path if path.is_absolute() else proof_path.parent / path
        path = path.resolve()
        require(path.is_file() and re.fullmatch('[a-f0-9]{64}', str(row['sha256'])) and sha(path) == row['sha256'], 'Evidence hash mismatch: ' + str(path))
        proofs[str(path)] = row['sha256']
        return path

    proof = json.loads(bind({'path': str(proof_path), 'sha256': expected_sha}).read_text())
    require(proof.get('schema') == 's47-cows-install-v1' and proof.get('status') == 'PASS_READY_TO_INSTALL', 'Cow installation proof is incomplete')
    frozen_path = bind(proof.get('snapshot'))
    frozen = json.loads(frozen_path.read_text())
    require(frozen.get('schema') == 's47-cows-snapshot-v1', 'Expected cow source/payload snapshot')
    candidate_path = Path(frozen['candidate'])
    require(candidate_path.is_absolute() and not candidate_path.is_symlink(), 'Expected explicit real candidate directory')
    candidate = candidate_path.resolve()
    source = Path(frozen['source_root']).resolve()
    require(candidate.is_dir() and not candidate.is_symlink() and TARGET.is_dir() and not TARGET.is_symlink(), 'Missing or symlinked candidate/target')
    require(not candidate.is_relative_to(TARGET.resolve()) and not TARGET.resolve().is_relative_to(candidate), 'Candidate/target overlap')
    require(snapshot(candidate) == frozen['payload'] and not snapshot(candidate, saved=True), 'Candidate payload changed or contains Saved nodes')
    require(source_files(source) == frozen['source_files'], 'Tested source bytes or inventory changed')
    package = frozen['payload'].get(BINARY, {}).get('sha256')
    require(package and os.access(candidate / BINARY, os.X_OK), 'Candidate executable missing')
    require(proof.get('package_sha256') == package, 'Proof used a different executable')
    native = {}
    for label in ('native_import', 'native_verify'):
        report_path = bind(proof.get(label))
        report = json.loads(report_path.read_text())
        require(report.get('status') == 'PASS' and report.get('checks') and all(row['pass'] for row in report['checks']), 'Native cow proof did not pass: ' + label)
        require({row.get('name') for row in report.get('meshes', [])} == MESHES and len(report['meshes']) == 3, 'Native cow family differs: ' + label)
        for row in report['meshes']:
            require(row.get('asset') == ASSET_PREFIX + row['name'] + '.' + row['name'], 'Unexpected native asset path')
        native[label] = report
    require(native['native_verify'].get('import_report_sha256') == proof['native_import']['sha256'], 'Fresh verification used another import report')
    for row in native['native_import']['meshes']:
        verified = next(item for item in native['native_verify']['meshes'] if item['name'] == row['name'])
        require(verified['triangles'] == row['triangles'] and verified['materials'] == row['materials'], 'Fresh native family metadata differs')
    cases = proof.get('runtime_cases', [])
    require(REQUIRED_CASES <= {case.get('name') for case in cases} and len({case.get('name') for case in cases}) == len(cases), 'Required distinct cow runtime cases missing')
    for case in cases:
        require(type(case.get('returncode')) is int and case['returncode'] == 0 and case.get('package_sha256') == package and case.get('snapshot_sha256') == proof['snapshot']['sha256'], 'Runtime package/result binding mismatch')
        command = case.get('command', [])
        require(command and Path(command[0]).resolve() in (candidate / BINARY, candidate / 'Signal47.sh'), 'Runtime used another candidate executable')
        raw = bind(case.get('log')).read_bytes()
        text = raw.decode('utf-16') if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else raw.decode('utf-8-sig', errors='replace')
        require(not ERRORS.search(text), 'Runtime error: ' + case['name'])
        markers = case.get('required', [])
        require(markers and all(isinstance(marker, str) and marker and marker in text for marker in markers), 'Runtime finish markers missing: ' + case['name'])
        if case['name'] in REQUIRED_CASES:
            require('smoke test OK' in text, 'Runtime lacks completed smoke test: ' + case['name'])
            require(all(ASSET_PREFIX + name + '.' + name in text for name in MESHES), 'Runtime lacks actual native cow asset paths')
            required_marker, required_argument = {
                'selftest': ('self test OK', '-S47SelfTest'),
                'pasture': ('pasture test still', '-S47Pasture=still'),
                'pasturemissing': ('pasture test short', '-S47Pasture=missing'),
                'ninth': ('ninth cow test ', '-S47Ninth'),
            }[case['name']]
            require(required_marker in text and any(argument == required_argument or argument.startswith(required_argument + '=') for argument in command[1:]), 'Wrong runtime case context: ' + case['name'])
    review_path = bind(proof.get('visual_review'))
    review = json.loads(review_path.read_text())
    require(review.get('status') == 'PASS' and review.get('package_sha256') == package, 'Visual review used another package or did not pass')
    images = review.get('images', [])
    require({'alive', 'abducted'} <= {row.get('subject') for row in images}, 'Review needs actual live and abducted cow images')
    for row in images:
        require(row.get('reviewed') is True and row.get('verdict') == 'PASS', 'Image lacks completed visual review')
        image = dict(row)
        image['path'] = str(review_path.parent / image['path']) if not Path(image['path']).is_absolute() else image['path']
        bind(image)
    require(all(not Path(name).is_relative_to(candidate) and not Path(name).is_relative_to(TARGET.resolve()) for name in proofs), 'Evidence may not be inside candidate or installed tree')
    return {'candidate': candidate, 'source': source, 'frozen': frozen, 'proofs': proofs, 'package_sha256': package}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='action', required=True)
    freeze = sub.add_parser('snapshot', help='Record the actual tested source and complete candidate payload; no installation')
    freeze.add_argument('--candidate', type=Path, required=True)
    freeze.add_argument('--source-root', type=Path, required=True)
    freeze.add_argument('--out', type=Path, required=True)
    install = sub.add_parser('install', help='Validate an accepted candidate; add --write for actual installation')
    install.add_argument('--proof', type=Path, required=True)
    install.add_argument('--proof-sha256', required=True)
    install.add_argument('--work', type=Path, required=True)
    install.add_argument('--write', action='store_true')
    args = parser.parse_args()
    if args.action == 'snapshot':
        require(not args.candidate.is_symlink(), 'Expected real candidate directory')
        candidate, source = args.candidate.resolve(), args.source_root.resolve()
        require(not snapshot(candidate, saved=True), 'Candidate may not contain Saved nodes')
        write(args.out, {'schema': 's47-cows-snapshot-v1', 'candidate': str(candidate), 'source_root': str(source),
            'payload': snapshot(candidate), 'source_files': source_files(source)})
        print('COW_SNAPSHOT', args.out.resolve(), sha(args.out))
        return
    proof = args.proof.expanduser().resolve()
    value = validate(proof, args.proof_sha256)
    guard = idle()
    if not args.write:
        print(json.dumps({'status': 'VALIDATED_NOT_INSTALLED', 'package_sha256': value['package_sha256'], 'payload_nodes': len(value['frozen']['payload']), 'source_files': len(value['frozen']['source_files'])}, indent=2))
        return
    stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ') + '-' + uuid.uuid4().hex[:8]
    evidence = args.work.expanduser().resolve() / ('installation-cows-' + stamp)
    require(not any(evidence.is_relative_to(path) for path in (TARGET.resolve(), value['candidate'], value['source'])), 'Backup/evidence cannot overlap payload or source')
    evidence.mkdir(parents=True)
    old, saved = snapshot(TARGET), snapshot(TARGET, saved=True)
    backup = evidence / 'payload-backup'
    result = {'status': 'RUNNING', 'candidate': str(value['candidate']), 'target': str(TARGET), 'backup': str(backup),
        'package_sha256': value['package_sha256'], 'initial_guard': guard, 'proof_hashes': value['proofs'], 'before_payload': old, 'before_saved': saved,
        'method': 'Full verified reflink payload backup; rsync -aX --checksum --delete --exclude=Saved; Saved never copied or restored'}
    write(evidence / 'result.json', result)
    try:
        reflink_backup(TARGET, backup, old)
        require(snapshot(TARGET) == old and snapshot(TARGET, saved=True) == saved, 'Installed state changed during backup')
        current = validate(proof, args.proof_sha256)
        require(current == value, 'Proof, source or candidate changed before writing')
        result['prewrite_guard'] = idle()
        require(snapshot(TARGET) == old and snapshot(TARGET, saved=True) == saved, 'Installed state changed before writing')
        try:
            sync_payload(value['candidate'], TARGET)
            require(snapshot(TARGET) == value['frozen']['payload'], 'Installed payload mismatch')
            require(snapshot(TARGET, saved=True) == saved, 'Saved changed during installation')
            require(source_files(value['source']) == value['frozen']['source_files'], 'Source changed during installation')
            after_saved = snapshot(TARGET, saved=True)
            require(after_saved == saved, 'Saved changed before final receipt')
        except BaseException as error:
            result['installation_error'] = repr(error)
            try:
                result['rollback_guard'] = idle()
                sync_payload(backup, TARGET)
                require(snapshot(TARGET) == old, 'Rollback payload mismatch')
                require(snapshot(TARGET, saved=True) == saved, 'Saved differs after rollback; installer never overwrites it')
                result['rollback'] = 'PASS'
            except BaseException as rollback_error:
                result['rollback'] = 'FAIL'
                result['rollback_error'] = repr(rollback_error)
            raise
        result.update(status='PASS', installed_at=datetime.now(timezone.utc).isoformat(), verified_payload_nodes=len(value['frozen']['payload']), saved_nodes_unchanged=len(saved), after_saved=after_saved)
    except BaseException as error:
        result.update(status='FAIL', error=repr(error))
        write(evidence / 'result.json', result)
        raise
    write(evidence / 'result.json', result)
    print('COW_INSTALLATION_PASS', evidence / 'result.json')


if __name__ == '__main__':
    main()
