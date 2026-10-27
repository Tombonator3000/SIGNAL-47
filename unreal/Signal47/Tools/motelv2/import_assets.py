"""Explicit UE commandlet import into /Game/MotelV2 only; launches no process.

S47_MOTEL_SOURCE: authored export containing manifest.json/materials.json/exports.
S47_MOTEL_TRIMS: optional prepared-and-FBX-converted old geometry cuts.
S47_MOTEL_REPORT: fresh report JSON path. Assembly activation is a separate step.
"""
import json
import os
import sys
import traceback
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import native as N


def main(root, trims, report):
    contract, rows = N.load_contract(root, report)
    # Probe first; a changed engine import convention cannot silently corrupt the site.
    probe = N.import_mesh(root / 'exports/axis_probe.fbx', N.DEST + '/Verification', 'axis_probe', report)
    N.finish_mesh(probe, report)
    N.verify_probe(report)
    materials = {row['name']: N.make_material(row, root, report) for row in rows}
    for name, row in contract['parts'].items():
        path = root / 'exports' / (name + '.fbx')
        mesh = N.import_mesh(path, N.DEST, name, report)
        slots = list(mesh.get_editor_property('static_materials'))
        actual = [str(slot.get_editor_property('material_slot_name')) for slot in slots]
        report.check(name + ' FBX material order', actual == row['materials'], {'actual': actual, 'expected': row['materials']})
        for i, material_name in enumerate(actual):
            report.check(name + ' material defined ' + material_name, material_name in materials)
            mesh.set_material(i, materials[material_name])
        for lod in range(1, len(row['lods'])):
            source = root / 'exports' / (name + '_LOD' + str(lod) + '.fbx')
            report.check(name + ' authored LOD source ' + str(lod), source.is_file())
            report.check(name + ' imported LOD ' + str(lod), N.S.import_lod(mesh, lod, str(source)) == lod)
        N.finish_mesh(mesh, report)
    if trims:
        data = json.loads((trims / 'trim-manifest.json').read_text())
        converted = json.loads((trims / 'trim-fbx.json').read_text())
        report.check('converted trim source exact', converted['status'] == 'PASS' and
                     converted['trim_manifest_sha256'] == N.sha(trims / 'trim-manifest.json'))
        files = {r['name']: r for r in converted['meshes']}
        report.check('all nonempty trims converted', set(files) == {name for name, row in data['meshes'].items() if not row['hidden']})
        fallback = N.E.load_asset('/Engine/EngineMaterials/DefaultMaterial')
        for name, row in data['meshes'].items():
            if row['hidden']:
                continue
            source = trims / row['fbx']
            report.check(name + ' trim FBX unchanged', N.sha(source) == files[name]['fbx_sha256'])
            mesh = N.import_mesh(source, N.DEST + '/Trim', name, report)
            slots = list(mesh.get_editor_property('static_materials'))
            report.check(name + ' original material mapping count', len(slots) == len(row['original_material_indices']))
            # Runtime restores original live material pointers through the ordinal map.
            for i in range(len(slots)):
                mesh.set_material(i, fallback)
            N.finish_mesh(mesh, report)
    # Re-read saved native data, not just return values from import tasks.
    N.verify_all(root, trims, report)
    report.data['status'] = 'PASS'
    report.data['activation'] = 'NOT_STAGED: runtime assembly.json must be staged only after independent saved verification'
    report.flush()
    N.unreal.log('MOTEL_V2_NATIVE_IMPORT_PASS')


if __name__ == '__main__':
    report = N.Report(os.environ['S47_MOTEL_REPORT'], 'native-import')
    try:
        main(Path(os.environ['S47_MOTEL_SOURCE']).resolve(),
             Path(os.environ['S47_MOTEL_TRIMS']).resolve() if os.environ.get('S47_MOTEL_TRIMS') else None, report)
    except Exception:
        report.data.update(status='FAIL', error=traceback.format_exc())
        report.flush()
        raise
