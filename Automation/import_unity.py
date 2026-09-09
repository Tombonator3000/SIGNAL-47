#!/usr/bin/env python3
"""Import an original SIGNAL / 47 ZIP without overwriting an existing project."""
import argparse
from pathlib import Path, PurePosixPath
import stat
import tempfile
import zipfile
import shutil


def import_project(archive: Path, repo: Path):
    destination = repo / 'Unity'
    if destination.exists():
        raise ValueError(f'Refusing to overwrite {destination}')
    with zipfile.ZipFile(archive) as z:
        items = z.infolist()
        if sum(i.file_size for i in items) > 2 * 1024**3:
            raise ValueError('Archive exceeds 2 GiB; inspect it before importing')
        names = set()
        for i in items:
            p = PurePosixPath(i.filename)
            if p.is_absolute() or '..' in p.parts or '\\' in i.filename or stat.S_ISLNK(i.external_attr >> 16):
                raise ValueError(f'Unsafe archive entry: {i.filename}')
            if p in names:
                raise ValueError(f'Duplicate archive entry: {p}')
            names.add(p)
        roots = [p.parent.parent for p in names if p.name == 'ProjectVersion.txt' and p.parent.name == 'ProjectSettings']
        if len(roots) != 1:
            raise ValueError('Expected exactly one Unity project')
        root = roots[0]
        version = z.read(str(root / 'ProjectSettings/ProjectVersion.txt')).decode()
        if 'm_EditorVersion: 6000.3.22f1' not in version.splitlines():
            raise ValueError(f'Unexpected Unity version: {version}')
        if root / 'Packages/manifest.json' not in names or not any(p.is_relative_to(root / 'Assets') for p in names):
            raise ValueError('Project is missing Packages/manifest.json or Assets')
        # Validate everything first. Commit only the complete extracted project.
        with tempfile.TemporaryDirectory(prefix='.signal47-import-', dir=repo) as temporary:
            target = Path(temporary) / 'Unity'
            target.mkdir()
            for i in items:
                p = PurePosixPath(i.filename)
                if not p.is_relative_to(root):
                    continue
                rel = p.relative_to(root)
                if rel.parts and rel.parts[0] in {'Library', 'Temp', 'Logs', 'Obj', '.git'}:
                    continue
                out = target.joinpath(*rel.parts)
                if i.is_dir():
                    out.mkdir(parents=True, exist_ok=True)
                else:
                    out.parent.mkdir(parents=True, exist_ok=True)
                    with z.open(i) as source, out.open('xb') as dest:
                        shutil.copyfileobj(source, dest)
                    out.chmod(0o755 if i.external_attr >> 16 & 0o111 else 0o644)
            target.rename(destination)
    print(f'Imported original project to {destination}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    args = parser.parse_args()
    import_project(args.archive, Path(__file__).resolve().parents[1])
