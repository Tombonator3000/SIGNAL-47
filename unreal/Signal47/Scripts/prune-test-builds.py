#!/usr/bin/env python3
"""Retire generated Signal47 package payloads; dry-run unless --apply is explicit.

Only known package containers and their game executable/debug symbols are removed.
Saved, evidence, source/assets, Engine files and package directory shells remain.
"""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import stat
import sys


def inside(path, root):
    return path == root or root in path.parents


def safe_file(path, root):
    """Reject symlink traversal, not just symlink leaves."""
    if not inside(path, root):
        return False
    for part in (path, *path.parents):
        if part.is_symlink():
            return False
        if part == root:
            break
    return path.is_file()


def payloads(root):
    found = []
    if not root.is_dir():
        return found
    # Only Signal47 work folders, never unrelated projects or installed games.
    for project in sorted(root.glob('Signal47*')):
        if not project.is_dir() or project.is_symlink():
            continue
        for directory, children, files in os.walk(project, followlinks=False):
            here = Path(directory)
            children[:] = [name for name in children if name not in
                           {'Saved', '.git', 'Content', 'Intermediate', 'DerivedDataCache', 'node_modules'}
                           and not (here / name).is_symlink()]
            if len(here.relative_to(project).parts) >= 7:
                children[:] = []
            if (here.parts[-3:] != ('Signal47', 'Binaries', 'Linux') or
                    'Signal47' not in files):
                continue
            package = here.parents[2]
            exe = here / 'Signal47'
            paks = package / 'Signal47/Content/Paks'
            containers = sorted(path for path in paks.glob('*') if path.suffix in
                                {'.pak', '.ucas', '.utoc'} and safe_file(path, root))
            if not safe_file(exe, root) or not containers:
                continue
            paths = [exe, *containers]
            debug = here / 'Signal47.debug'
            if safe_file(debug, root):
                paths.append(debug)
            found.append({'path': str(package), 'mtime': max(p.stat().st_mtime for p in paths),
                          'bytes': sum(p.stat().st_size for p in paths),
                          'files': [{'path': str(p), 'bytes': p.stat().st_size,
                                     'mtime_ns': p.stat().st_mtime_ns, 'inode': p.stat().st_ino}
                                    for p in paths]})
    return sorted(found, key=lambda row: (row['mtime'], row['path']), reverse=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path.home() / 'scratch-bm')
    parser.add_argument('--current', type=Path, required=True,
                        help='Current verified package root, e.g. candidate-A1/Linux')
    parser.add_argument('--protect', type=Path, action='append', default=[],
                        help='Active candidate/package or containing test folder; repeatable')
    parser.add_argument('--apply', action='store_true', help='Remove only the files in the reported retire list')
    args = parser.parse_args()
    root = args.root.expanduser().absolute()
    current = args.current.expanduser().absolute()
    if root.is_symlink() or root.resolve() != root or not root.is_dir():
        parser.error('--root must be an existing canonical directory, without symlink traversal')
    if not (current / 'Signal47/Binaries/Linux/Signal47').is_file():
        parser.error('--current must contain the current verified packaged game executable')
    protected = [current.resolve(), (Path.home() / 'Games/Signal47').resolve(),
                 *(p.expanduser().resolve() for p in args.protect)]
    rows = payloads(root)
    old = []
    for row in rows:
        path = Path(row['path'])
        row['action'] = 'protected' if any(inside(path, p) or inside(p, path) for p in protected) else 'old'
        if row['action'] == 'old':
            old.append(row)
    for index, row in enumerate(old):
        row['action'] = 'keep_old' if index < 2 else 'retire_payload'
    retire = [row for row in rows if row['action'] == 'retire_payload']
    report = {'version': 1, 'at': datetime.now(timezone.utc).isoformat(), 'mode': 'apply' if args.apply else 'dry_run',
              'root': str(root), 'current': str(current), 'keep_old': 2,
              'protected': [str(p) for p in protected], 'packages': rows,
              'retire_count': len(retire), 'retire_logical_bytes': sum(row['bytes'] for row in retire),
              'preserved': 'Saved, source/assets, logs, screenshots, Engine files and package shells',
              'space_note': 'Logical bytes are not promised free space when reflinks/hardlinks share extents'}
    # Print the complete concrete plan even when an active job blocks application.
    print(json.dumps(report, indent=2))
    if not args.apply:
        return
    from unreal_guard import heavy_jobs
    if heavy_jobs():
        raise RuntimeError('Unreal/game/package job active; no files removed. Retry after the active test/build ends.')
    # Validate the whole plan before the first unlink, then revalidate every file at removal.
    def validate(row):
        p = Path(row['path'])
        if not safe_file(p, root):
            raise RuntimeError(f'Payload path changed or symlink appeared: {p}')
        info = p.stat()
        if (info.st_size, info.st_mtime_ns, info.st_ino) != (row['bytes'], row['mtime_ns'], row['inode']):
            raise RuntimeError(f'Payload changed after planning: {p}')
        if not stat.S_ISREG(info.st_mode):
            raise RuntimeError(f'Not a regular file: {p}')
        return p
    for package in retire:
        for row in package['files']:
            validate(row)
    removed = []
    for package in retire:
        if heavy_jobs():
            raise RuntimeError('New Unreal/game/package job appeared; stopping before the next package.')
        for row in package['files']:
            p = validate(row)
            p.unlink()
            removed.append(str(p))
    print(json.dumps({'removed_files': removed, 'retired_packages': len(retire)}, indent=2))


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError) as exc:
        print(f'prune-test-builds: {exc}', file=sys.stderr)
        sys.exit(1)
