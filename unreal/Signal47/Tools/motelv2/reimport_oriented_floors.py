"""Reimport only the eleven source-verified R6 floor owners, then verify all.

S47_MOTEL_SOURCE/TRIMS/REPORT select the same inputs as import_assets.py.
S47_MOTEL_SOURCE_AUDIT must be the fresh successful read-only R6 audit.
Materials, textures, unrelated meshes and original source packages stay intact.
"""
import hashlib
import json
import os
import sys
import traceback
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import native as N

TARGETS = {'SM_Sierra_Office', 'SM_Sierra_CourtSurface', 'SM_Sierra_PoolDeck'} | {
    'SM_Sierra_Room' + str(i).zfill(2) for i in range(1, 9)}
PROJECT = Path(__file__).resolve().parents[2]


def fingerprints():
    return {str(path.relative_to(PROJECT)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted((PROJECT / 'Content/MotelV2').rglob('*'))
            if path.is_file() and not (path.parent == PROJECT / 'Content/MotelV2' and
                                      path.suffix == '.uasset' and path.stem in TARGETS)}


def main(report):
    root = Path(os.environ['S47_MOTEL_SOURCE']).resolve()
    trims = Path(os.environ['S47_MOTEL_TRIMS']).resolve()
    audit_path = Path(os.environ['S47_MOTEL_SOURCE_AUDIT']).resolve()
    audit = json.loads(audit_path.read_text())
    report.check('fresh source orientation audit', audit['status'] == 'READ_ONLY_AUDIT_REOPEN_BVH_PASS_NATIVE_UNVERIFIED')
    report.check('audited manifest exact', audit['output_manifest_sha256'] == N.sha(root / 'manifest.json'))
    report.check('exact eleven floor owners', set(audit['target_parts']) == TARGETS)
    report.data['source_orientation_audit_sha256'] = N.sha(audit_path)
    contract, _ = N.load_contract(root, report)
    preserved = fingerprints()
    report.data['preserved_native_before_sha256'] = preserved
    report.flush()
    for name in sorted(TARGETS):
        row = contract['parts'][name]
        mesh = N.import_mesh(root / 'exports' / (name + '.fbx'), N.DEST, name, report)
        actual = [str(slot.get_editor_property('material_slot_name')) for slot in mesh.get_editor_property('static_materials')]
        report.check(name + ' unchanged source material order', actual == row['materials'], actual)
        for i, material in enumerate(actual):
            parent = N.E.load_asset(N.DEST + '/Materials/' + material)
            report.check(name + ' retained material ' + material, isinstance(parent, N.unreal.Material))
            mesh.set_material(i, parent)
        for lod in range(1, 3):
            source = root / 'exports' / (name + '_LOD' + str(lod) + '.fbx')
            report.check(name + ' oriented authored LOD' + str(lod), N.S.import_lod(mesh, lod, str(source)) == lod)
        N.finish_mesh(mesh, report, N.AUTHORED_SCREEN_SIZES)
    report.check('all unrelated native bytes preserved', fingerprints() == preserved)
    N.verify_all(root, trims, report)
    report.data['status'] = 'PASS'
    report.data['activation'] = 'NOT_STAGED: fresh saved reader and actual surface rays required'
    report.flush()
    N.unreal.log('MOTEL_V2_ORIENTED_FLOOR_IMPORT_PASS')


if __name__ == '__main__':
    report = N.Report(os.environ['S47_MOTEL_REPORT'], 'oriented-floor-import')
    try:
        main(report)
    except Exception:
        report.data.update(status='FAIL', error=traceback.format_exc())
        report.flush()
        raise
