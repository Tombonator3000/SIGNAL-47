#!/usr/bin/env python3
"""Take only the packages the chosen graveyard pieces need out of a Cosmos pack's zip.

The zips are bought and untrusted: nothing in them is run, every name is checked to stay inside the output folder, and only the
packages the chosen meshes reach (their materials, master materials, functions and textures) are written. Which packages a package
needs is read from the /Game/<pack>/... paths in its name table (plain bytes, no Unreal), so a 2 GB pack gives a few hundred MB.
No picture is opened or decoded here: the Cosmos licence allows no use as reference for AI tools.

Run with python3 -I (an isolated interpreter):
  python3 -I extract.py --zip ~/Desktop/DemonicVillage.zip --pack demonic --out <new empty folder>
The meshes come from selection.json next to this script (its packs.<pack>.root and .meshes). The output holds Content/<root>/...
as the pack had it, ready to copy into the throwaway project (import.sh).
"""
import argparse
import json
import re
import sys
import zipfile
from pathlib import Path, PurePosixPath

HERE = Path(__file__).resolve().parent
EXTS = ('.uasset', '.uexp', '.ubulk', '.uptnl')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--zip', required=True)
    ap.add_argument('--pack', required=True, help='a key of selection.json packs')
    ap.add_argument('--out', required=True, help='a new, empty folder')
    a = ap.parse_args()
    sel = json.loads((HERE / 'selection.json').read_text())['packs'][a.pack]
    root = sel['root']                      # e.g. Demonic_Village: the pack's /Game/<root>
    out = Path(a.out).resolve()
    if out.exists() and any(out.iterdir()):
        sys.exit('S47 COSMOS CHURCH EXTRACT: %s is not empty; use a new folder' % out)
    out.mkdir(parents=True, exist_ok=True)
    pattern = re.compile(rb'/Game/' + re.escape(root.encode()) + rb'/[A-Za-z0-9_\-/ .]+')
    with zipfile.ZipFile(a.zip) as z:
        # where the pack's Content/<root>/ lies in the zip (some zips have a top folder)
        members = {}
        marker = 'Content/' + root + '/'
        for info in z.infolist():
            name = info.filename
            k = name.find(marker)
            if info.is_dir() or k < 0 or (k > 0 and name[k - 1] != '/'):
                continue
            rel = name[k + len('Content/'):]                 # <root>/Meshes/SM_x.uasset
            members.setdefault(str(PurePosixPath(rel).with_suffix('')), []).append(info)
        todo = ['%s/%s' % (root, m) if '/' in m else None for m in sel['meshes']]
        if None in todo:
            sys.exit('S47 COSMOS CHURCH EXTRACT: meshes are named by their path under the root (Meshes/SM_x)')
        seen, missing = set(), []
        while todo:
            pkg = todo.pop()
            if pkg in seen:
                continue
            seen.add(pkg)
            infos = members.get(pkg)
            if not infos:
                missing.append(pkg)
                continue
            for info in infos:
                if not info.filename.endswith(EXTS):
                    continue
                data = z.read(info)
                dest = (out / 'Content' / (pkg + PurePosixPath(info.filename).suffix)).resolve()
                if out not in dest.parents:
                    sys.exit('S47 COSMOS CHURCH EXTRACT: %s would land outside %s' % (info.filename, out))
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(data)
                if info.filename.endswith('.uasset'):
                    for hit in pattern.findall(data):
                        ref = hit.decode('ascii', 'replace').split('.')[0].rstrip('/ ')[len('/Game/'):]
                        if ref and ref not in seen:
                            todo.append(ref)
    size = sum(p.stat().st_size for p in out.rglob('*') if p.is_file())
    report = {'zip': Path(a.zip).name, 'pack': a.pack, 'root': root, 'packages': len(seen) - len(missing),
              'bytes': size, 'missing': sorted(missing)}
    (out / 'extract.json').write_text(json.dumps(report, indent=1))
    print('S47 COSMOS CHURCH EXTRACT', a.pack, report['packages'], 'packages', round(size / 1e6, 1), 'MB;',
          len(missing), 'referenced but not in the zip:', missing[:12])


if __name__ == '__main__':
    main()
