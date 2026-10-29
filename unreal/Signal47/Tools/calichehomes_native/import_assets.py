"""Eksplisitt editorimport, bare til /Game/CalicheHomes.

S47_HOMES_SOURCE/TRIMS/REPORT er obligatoriske miljøvariabler. Starter ingen
prosess. Aktivering skjer separat etter en fersk verify_assets-kjøring.
"""
import os
from pathlib import Path
import sys
import traceback

sys.path.insert(0, str(Path(__file__).resolve().parent))
import native as N


def main(root, trims, report):
    sources = N.source_inputs(root, trims, report)
    manifest, rows, trim, _ = sources
    protected = N.protect_dependencies(report)
    try:
        # Hele kilde-/foreldregrafsettet skal være kjent før første mutasjon.
        for row in rows:
            N.parent_material(row, root, report)
        materials = {row['name']: N.make_instance(row, root, report) for row in rows}
        for name, row in manifest['parts'].items():
            mesh = N.FBX.import_mesh(N.source_path(root, row['lods'][0]['fbx']), N.DEST, name, report)
            slots = list(mesh.get_editor_property('static_materials'))
            actual = [str(slot.get_editor_property('material_slot_name')) for slot in slots]
            report.check(name + ' imported FBX material order', actual == row['materials'], actual)
            for index, material_name in enumerate(actual):
                report.check(name + ' named owned instance ' + material_name, material_name in materials)
                mesh.set_material(index, materials[material_name])
            for lod in (1, 2):
                source = N.source_path(root, row['lods'][lod]['fbx'])
                report.check(name + ' authored LOD import ' + str(lod), N.S.import_lod(mesh, lod, str(source)) == lod)
            N.finish(mesh, report, 3)
        fallback = N.E.load_asset('/Engine/EngineMaterials/DefaultMaterial')
        report.check('existing trim fallback material', fallback is not None)
        for group in ('meshes', 'signs'):
            for name, row in trim[group].items():
                if row.get('hidden', False):
                    continue
                destination = N.DEST + ('/Trim' if group == 'meshes' else '')
                expected_asset = destination + '/' + name + '.' + name
                report.check(name + ' exact owned destination', row['asset'] == expected_asset)
                mesh = N.FBX.import_mesh(N.source_path(trims, row['fbx']), destination, name, report)
                slots = list(mesh.get_editor_property('static_materials'))
                actual = [str(slot.get_editor_property('material_slot_name')) for slot in slots]
                report.check(name + ' original ordinal/FBX placeholders', actual == row['materials']
                             and len(slots) == len(row['original_material_indices']), actual)
                # Runtime gjenoppretter kildekomponentenes eksisterende materialer.
                for slot in range(len(slots)):
                    mesh.set_material(slot, fallback)
                N.finish(mesh, report, 1)
        N.verify_all(root, trims, report, sources)
        N.check_preserved(protected, report)
    except Exception:
        report.data['town_dependencies_after_failure'] = N.town_snapshot()
        report.data['town_unchanged_after_failure'] = report.data['town_dependencies_after_failure'] == protected
        raise
    report.data.update(status='PASS', activation='NOT_STAGED: fresh saved-native-read-only proof required')
    report.flush()
    N.unreal.log('HOMES_NATIVE_IMPORT_PASS')


if __name__ == '__main__':
    root, trims = Path(os.environ['S47_HOMES_SOURCE']).resolve(), Path(os.environ['S47_HOMES_TRIMS']).resolve()
    output = Path(os.environ['S47_HOMES_REPORT']).resolve()
    assert not any(output.is_relative_to(p) for p in (root, trims, N.PROJECT / 'Content')), 'Bevisrapporten må ligge utenfor kilde-/assetmappene'
    report = N.Report(output, 'native-import')
    try:
        main(root, trims, report)
    except Exception:
        report.data.update(status='FAIL', error=traceback.format_exc())
        report.flush()
        raise
