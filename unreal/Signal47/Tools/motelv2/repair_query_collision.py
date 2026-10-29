"""Explicit migration of owned, already imported meshes; no reimport or geometry edits.

Uses S47_MOTEL_SOURCE/TRIMS/REPORT, like verify_assets.py. Records the observed
section flags before mutation and preserves the actual saved screen sizes.
The independent verify_assets.py must follow in a fresh editor process.
"""
import json
import os
import sys
import traceback
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import native as N


def main(report):
    root = Path(os.environ['S47_MOTEL_SOURCE']).resolve()
    trims = Path(os.environ['S47_MOTEL_TRIMS']).resolve()
    contract, _ = N.load_contract(root, report)
    paths = [N.DEST + '/' + name for name in contract['parts']]
    trim_contract = json.loads((trims / 'trim-manifest.json').read_text())
    paths += [row['asset'] for row in trim_contract['meshes'].values() if not row['hidden']]
    report.check('unique contracted migration meshes', len(paths) == len(set(paths)) and len(paths) == 31)
    for path in paths:
        mesh = N.E.load_asset(N.owned(path))
        report.check(path + ' existing native mesh', isinstance(mesh, N.unreal.StaticMesh))
        screens = list(N.S.get_lod_screen_sizes(mesh))
        report.check(path + ' actual saved screens before migration', bool(screens) and
                     all(N.math.isfinite(value) and value > 0 for value in screens), screens)
        N.configure_query_collision(mesh, report)
        report.check(path + ' restore actual screen policy', N.S.set_lod_screen_sizes(mesh, screens))
        report.check(path + ' saved migrated mesh', N.E.save_loaded_asset(mesh))
        N.verify_query_collision(mesh, report)
        after = list(N.S.get_lod_screen_sizes(mesh))
        report.check(path + ' screen policy unchanged', len(after) == len(screens) and
                     all(abs(a-b) < 1e-6 for a, b in zip(after, screens)), after)
    N.verify_all(root, trims, report)
    report.data['status'] = 'PASS'
    report.data['limits'] = 'Native query sections only. Fresh reader and actual runtime rays required separately.'
    report.flush()
    N.unreal.log('MOTEL_V2_QUERY_COLLISION_REPAIR_PASS')


if __name__ == '__main__':
    report = N.Report(os.environ['S47_MOTEL_REPORT'], 'query-collision-migration')
    try:
        main(report)
    except Exception:
        report.data.update(status='FAIL', error=traceback.format_exc())
        report.flush()
        raise
