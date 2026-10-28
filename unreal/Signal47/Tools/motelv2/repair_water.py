"""Rett bare det eksisterende bassengvannmaterialet; ingen full import.

Bruk S47_MOTEL_SOURCE og S47_MOTEL_REPORT som i import_assets.py.
Ingen prosess startes her. Rotoppgaven kjører skriptet sekvensielt i Unreal.
PASS gjelder lagret materialpolicy og bevarte filer, ikke visuell godkjenning.
"""
import json
import os
from pathlib import Path
import sys
import traceback

sys.path.insert(0, str(Path(__file__).resolve().parent))
import native as N


def native_files():
    content = Path(N.unreal.Paths.convert_relative_path_to_full(N.unreal.Paths.project_content_dir()))
    return {str(path.resolve()): N.sha(path) for path in (content / 'MotelV2').rglob('*')
            if path.is_file() and path.suffix in {'.uasset', '.uexp', '.ubulk'}}


def main(root, report):
    source_path = root / 'materials.json'
    source_before = N.sha(source_path)
    rows = [row for row in json.loads(source_path.read_text()) if row.get('water_surface')]
    report.check('exact one authored pool water material', len(rows) == 1 and rows[0]['name'] == 'M_MotelV2_PoolWater')
    row = rows[0]
    path = N.DEST + '/Materials/' + row['name']
    mat = N.E.load_asset(path)
    report.check('existing owned pool water material', isinstance(mat, N.unreal.Material))
    before = native_files()
    report.data['before_native_sha256'] = before
    report.data['water_policy'] = {'base_color': 'Existing pale transmission atlas multiplied by authored water tint',
                                  'tint_linear': row['base_color_linear'][:3],
                                  'two_sided': False, 'alpha': row['alpha'],
                                  'roughness': 'Original ORM G, approximately0.07',
                                  'normal': 'Original DirectX normal map retained',
                                  'blend': 'TRANSLUCENT', 'lighting': 'SURFACE_PER_PIXEL_LIGHTING',
                                  'visual_acceptance': 'UNVERIFIED: fresh in-game pool screenshot required'}
    N.apply_water_policy(mat, row, report)
    errors = list(N.M.recompile_material(mat))
    report.check('water material compilation', not errors, errors)
    report.check('water material saved', N.E.save_loaded_asset(mat))
    N.verify_materials(rows, report)
    after = native_files()
    target = str((Path(N.unreal.Paths.convert_relative_path_to_full(N.unreal.Paths.project_content_dir())) /
                  'MotelV2' / 'Materials' / (row['name'] + '.uasset')).resolve())
    report.check('owned asset inventory unchanged', set(before) == set(after))
    report.check('only water material bytes may change', all(after[p] == value for p, value in before.items() if p != target))
    report.check('source material contract unchanged', N.sha(source_path) == source_before)
    report.data.update(status='PASS_NATIVE_WATER_POLICY_ONLY', after_native_sha256=after,
                       changed_native_files=[p for p in before if before[p] != after[p]],
                       source_materials_sha256=source_before)
    report.flush()
    N.unreal.log('MOTEL_V2_WATER_POLICY_PASS: material only; visual acceptance remains unverified')


if __name__ == '__main__':
    report = N.Report(os.environ['S47_MOTEL_REPORT'], 'native-water-material-repair')
    try:
        main(Path(os.environ['S47_MOTEL_SOURCE']).resolve(), report)
    except Exception:
        report.data.update(status='FAIL', error=traceback.format_exc())
        report.flush()
        raise
