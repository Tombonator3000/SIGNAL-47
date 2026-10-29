#!/usr/bin/env python3
"""Privat R12 output-only over uendret faktisk R11 pose og ny ClothesR2. Starter aldri en editor eller annen prosess.

Et faktisk bygget R12-CPP og produsert ClothesR2 må være bundet først.
--prepare skriver bare nye private jobber/inventarer etter roots fulle input.
Inne i roots allerede startede editor leses en av disse jobbene fra miljøet.
"""
from __future__ import annotations

import argparse
import ast
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shlex
import types

ROOT = Path('/home/tom/scratch-bm/Signal47-mannequins-20261029')
R10 = '/Game/MetaHumans/Mannequins/RadioWoman/BakeR10'
R12 = '/Game/MetaHumans/Mannequins/RadioWoman/BakeR12/SM_RadioWoman_Seated'
OUTPUT_REL = R12.removeprefix('/Game/') + '.uasset'
MODES = ('probe', 'bake', 'verify')
PIN_NAMES = {'runner', 'prepare', 'run_editor', 'native_cpp', 'editor_module', 'build_receipt',
             'r10_job', 'postrepair_job_field', 'garment_source_binding', 'garment_source_helper'}


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def load(path):
    return json.loads(Path(path).read_text())


def new_json(path, value):
    with Path(path).open('x', encoding='utf-8') as f:
        json.dump(value, f, ensure_ascii=False, indent=2)
        f.write('\n')


def digest(path, algorithm='sha256'):
    h = hashlib.new(algorithm)
    with Path(path).open('rb') as f:
        for chunk in iter(lambda: f.read(8 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def file_row(path, sha1=False):
    p = Path(path).resolve(strict=True)
    need(p.is_file() and not Path(path).is_symlink(), 'Vanlig fil kreves: ' + str(path))
    result = {'path': str(p), 'bytes': p.stat().st_size, 'sha256': digest(p)}
    if sha1:
        result['sha1'] = digest(p, 'sha1')
    return result


def pinned(row):
    need(isinstance(row, dict) and set(row) in ({'path', 'bytes', 'sha256'},
         {'path', 'bytes', 'sha256', 'sha1'}), 'Komplett eksplisitt filidentitet kreves')
    need(type(row['bytes']) is int and row['bytes'] > 0
         and isinstance(row['sha256'], str) and re.fullmatch('[0-9a-f]{64}', row['sha256']),
         'Input har uutfylt/ugyldig byte-/SHA256-binding')
    need(file_row(row['path'], 'sha1' in row) == row, 'Endret pinnet fil: ' + str(row['path']))
    return Path(row['path'])


def garment_helper(inputs):
    return prepare_module(inputs['pins']['garment_source_helper']['path'])


def input_pins(inputs):
    need(set(inputs['pins']) == PIN_NAMES, 'Eksakt komplett R12 kilde-/bygg-/garmentbinding kreves')
    for value in inputs['pins'].values(): pinned(value)
    need(inputs['pins']['runner'] == file_row(__file__), 'Denne fryste R12-runner kreves')
    helper = garment_helper(inputs)
    binding = helper.validate_binding(inputs['pins']['garment_source_binding']['path'])
    plan = helper.load(helper.pin(binding['source_plan_file']))
    source_inputs, *_ = helper.inputs_checked(helper.pin(plan['inputs']))
    need(source_inputs['project'] == inputs['project'] and source_inputs['source_commit'] == inputs['native_cpp_revision']
         and source_inputs['engine_version'] == inputs['engine_version'], 'Samme faktisk bygde modul/prosjekt som sourceimport')
    for key in ('prepare','run_editor','native_cpp','editor_module','build_receipt','r10_job'):
        need(source_inputs['pins'][key] == inputs['pins'][key], 'R12 kilde/modul avviker fra faktisk ClothesR2-produksjon: '+key)
    return binding


def tree(root):
    root = Path(root)
    need(root.is_dir() and not root.is_symlink(), 'Eksisterende vanlig trerot kreves')
    result = {}
    for p in sorted(root.rglob('*')):
        need(not p.is_symlink(), 'Symlink i beskyttet tre: ' + str(p))
        if p.is_file():
            row = file_row(p)
            result[p.relative_to(root).as_posix()] = {k: row[k] for k in ('bytes', 'sha256')}
    return result


def content_delta(before, actual, output_required):
    need(all(actual.get(name) == row for name, row in before.items()),
         'En eksisterende Content-fil er slettet/endret')
    extra = set(actual) - set(before)
    need(extra <= {OUTPUT_REL}, 'Andre Content-filer enn eksakt R12-output er opprettet')
    if output_required is not None:
        need(extra == ({OUTPUT_REL} if output_required else set()),
             'Faktisk R12-outputtilstand stemmer ikke med fasen')
    return {'preserved_files': len(before), 'new_files': sorted(extra),
            'output': actual.get(OUTPUT_REL)}


def prepare_module(path):
    # Compile den pinnede lokale teksten i minnet, uten __pycache__ i verktøymappen.
    module = types.ModuleType('s47_r12_pinned_prepare')
    module.__file__ = str(path)
    exec(compile(Path(path).read_text(), str(path), 'exec'), module.__dict__)
    return module


def readonly_editor_helpers(path, unreal):
    # run_editor.py har ubetinget main(). Bare disse to pinnede getterfunksjonene
    # gjenbrukes; main/import_private/native_suboperation eller setters lastes ikke.
    parsed = ast.parse(Path(path).read_text())
    names = {'sources_available', 'cloth_materials'}
    definitions = [n for n in parsed.body if isinstance(n, ast.FunctionDef) and n.name in names]
    need({n.name for n in definitions} == names, 'Eksakte eksisterende read-only-hjelpere kreves')
    namespace = {'unreal': unreal, 'require': need}
    exec(compile(ast.Module(body=definitions, type_ignores=[]), str(path), 'exec'), namespace)
    return namespace


def original_sources(job, p):
    need(p.sha(job['source_snapshot']) == job['source_snapshot_sha256'], 'R10 source_snapshot er endret')
    original = p.load(job['source_snapshot'])
    need(len(original['files']) == 405, 'Nøyaktig opprinnelig 405-filssnitt kreves')
    need(Path(original['source_root']).resolve() == Path(job['project']).resolve() / 'Content/MetaHumans',
         'Opprinnelig inventar peker på annet nativeprosjekt')
    for row in original['pins'].values():
        need(p.file_row(row['path']) == {k: row[k] for k in ('bytes', 'sha256')},
             'Pinnet original/Creator/FBX er endret')
    for relative, row in original['files'].items():
        need(p.file_row(Path(original['source_root']) / relative) == row,
             'Opprinnelig MetaHuman-fil er endret: ' + relative)
    return len(original['files'])


def r10_evidence(inputs, p):
    input_pins(inputs)
    job = load(inputs['pins']['r10_job']['path'])
    need(job['mode'] == 'bake' and job['asset_root'] == R10
         and job['output_asset'] == R10 + '/SM_RadioWoman_Seated', 'Eksakt faktisk R10-kildejobb kreves')
    cfg = load(job['contract'])
    need(p.sha(job['contract']) == job['contract_sha256'], 'Uendret faktisk R10-contract kreves')
    need(p.sha(job['clothing_report']) == job['clothing_report_sha256'], 'Klærnes opprinnelige kildekvittering kreves')
    p.clothing_check(job['clothing_report'])
    imported = p.imported_receipt(job, cfg)  # Uendret originaljobb/mappe, ingen R12-alias her.
    op = load(Path(job['python_report']).parent / 'import-operation-python.json')
    need(op['status'] == 'PASS' and op['mode'] == 'import' and op['pid'] == imported['pid']
         and op['private_import'] == imported
         and op['import_receipt'] == p.receipt_file(Path(job['python_report']).parent / 'import-python.json'),
         'Fullført faktisk R10-importoperasjon kreves')
    need(inputs['engine_version'] == op['engine_version'], 'Faktisk original UE5.8.4-versjon kreves')
    old_bake = load(Path(job['python_report']).parent / 'bake-python.json')
    old_verify = load(Path(job['python_report']).parent / 'verify-python.json')
    need(old_bake['status'] == old_verify['status'] == 'PASS'
         and old_bake['pid'] != old_verify['pid'] and old_bake['output_asset'] == old_verify['output_asset'],
         'Akseptert separat-PID R10 bake/leser kreves')
    old_output = Path(job['project']) / 'Content' / (job['output_asset'].removeprefix('/Game/') + '.uasset')
    need(p.file_row(old_output) == {k: old_bake['output_asset'][k] for k in ('bytes', 'sha256')},
         'R10s eksisterende statiske output er endret')
    original_sources(job, p)
    field = load(inputs['pins']['postrepair_job_field']['path'])
    need(set(field) == {'postrepair_binding'}, 'Eksakt Natt-jobbfelt kreves, ingen normalisering')
    binding = load(pinned(field['postrepair_binding']))
    need(binding['reference_job_file'] == file_row(inputs['pins']['r10_job']['path'], True)
         and binding['reference_contract_file'] == file_row(job['contract'], True),
         'Natt-bindingen peker på annen jobb/contract enn faktisk R10')
    binding = input_pins(inputs)
    need(binding['reference_r11_job_file']['path'] != inputs['pins']['r10_job']['path'], 'Akseptert separat R11 posegrunnlag kreves')
    field = dict(field, garment_source_binding=inputs['pins']['garment_source_binding'])
    return job, cfg, imported, field


def make_jobs(base, field, run):
    binding = load(pinned(field['garment_source_binding']))
    source = binding['new_job_fields']
    source_fields = dict(source, clothing_report=binding['source_report_file']['path'],
                         clothing_report_sha256=binding['source_report_file']['sha256'],
                         clothing_fbx=binding['source_fbx_file']['path'], clothing_fbx_sha256=binding['source_fbx_file']['sha256'])
    jobs = {}
    for mode in MODES:
        report = run / (mode + '-native.json')
        cloth_report = run / (mode + '-clothes-verify-native.json')
        actual = dict(base, **source_fields)
        actual.update(run_id='BakeR12', mode=mode, output_asset=R12, native_report=str(report),
                      bake_report=str(run/'bake-native.json'), python_report=str(run/(mode+'-python.json')),
                      clothes_bind_verify_report=str(cloth_report), **field)
        # Only the verifier's strict package-root identity uses the new SOURCE root.
        # No dummy static output is written in verify-clothes-bind mode.
        cloth = dict(actual, mode='verify-clothes-bind', output_asset='/Game/MetaHumans/Mannequins/RadioWoman/ClothesR2/SM_UnwrittenBindTarget',
                     native_report=str(cloth_report), python_report=str(run/(mode+'-clothes-unused-python.json')))
        jobs[mode] = {'actual':actual,'clothes':cloth}
    return jobs


def prepare_run(input_path):
    inputs = load(input_path)
    need(inputs['schema_version'] == 1 and inputs['status'] == 'ROOT_COMPLETED_R12_INPUTS',
         'Root må fullføre inputplanen etter faktisk C++-frys/bygg')
    input_pins(inputs)
    p = prepare_module(inputs['pins']['prepare']['path'])
    base, cfg, imported, field = r10_evidence(inputs, p)
    project = Path(inputs['project']).resolve(strict=True)
    need(project == Path(base['project']).resolve(strict=True), 'Samme faktiske R10-prosjekt kreves')
    run = Path(inputs['run_root']).resolve()
    need(run.parent == ROOT / 'r12-runs' and re.fullmatch('[A-Za-z][A-Za-z0-9_-]{0,63}', run.name)
         and not run.exists(), 'Ferskt direkte barn under private r12-runs kreves')
    need(not (project / 'Content' / OUTPUT_REL).exists()
         and not (project / 'Content' / OUTPUT_REL).parent.exists(), 'Hele nye BakeR12-destinasjonen må være fraværende')
    need(set(inputs['profiles']) == set(MODES) and len(set(inputs['profiles'].values())) == 3,
         'Tre forskjellige eksplisitte isolerte profiler kreves')
    for profile in inputs['profiles'].values():
        q = Path(profile)
        need(q.is_absolute() and str(q).startswith('/tmp/S47-') and '..' not in q.parts
             and not q.exists(), 'Fersk isolert /tmp/S47-profil kreves')
    # Kun root kjører denne CPU-forberedelsen. Ingen GPU-/editorprosess startes.
    protection = {'content': tree(project / 'Content'), 'r10_private': tree(Path(base['python_report']).parent),
                  'all_private':tree(ROOT/'private'),'r11_runs':tree(ROOT/'r11-runs'),
                  'garment_source_run':tree(Path(load(inputs['pins']['garment_source_binding']['path'])['source_plan_file']['path']).parent)}
    need(OUTPUT_REL not in protection['content'], 'R12 finnes allerede i førinventaret')
    run.mkdir(parents=True)
    new_json(run / 'protection-before.json', protection)
    new_json(run / 'inputs.json', inputs)
    rows = {}
    for mode, pair in make_jobs(base, field, run).items():
        rows[mode] = {}
        for kind, job in pair.items():
            path = run / ('job-' + mode + '-' + kind + '.json')
            new_json(path, job)
            rows[mode][kind] = file_row(path)
    plan = {'schema_version': 1, 'status': 'PREPARED_NOT_RUN', 'run_root': str(run),
            'inputs': file_row(run / 'inputs.json'), 'protection': file_row(run / 'protection-before.json'),
            'jobs': rows, 'source_r10_import_pid': imported['pid'], 'output_asset': R12,
            'runtime_accepted': False, 'contact_accepted': False, 'adoption_accepted': False}
    new_json(run / 'plan.json', plan)
    print(json.dumps({'status': 'PREPARED_NOT_RUN', 'plan': file_row(run / 'plan.json'),
                      'protected_content_files': len(protection['content']), 'engine_started': False}))


def native_evidence(native, job, binding):
    need(native.get('schema_version') == 1 and native.get('status') == 'PASS'
         and native.get('mode') == job['mode'] and native.get('pid') == os.getpid(), 'Fersk samme-PID native PASS kreves')
    need(native.get('postrepair_binding_version') == 1
         and native.get('postrepair_binding_file') == job['postrepair_binding']
         and native.get('postrepair_binding') == binding, 'Gammel/ubundet binær eller feil R12-binding')
    targets = native.get('postrepair_derived_foot_targets', {})
    need(set(targets) == {'foot_l', 'foot_r'}, 'Begge faktiske utledede fotmål kreves')
    for row in targets.values():
        need(row.get('actual_rays') == 48 and row.get('output_side') in ('negative_Y', 'positive_Y'),
             'Faktiske 48 R2-prøver per navngitt output-side kreves')
        for key in ('minimum_gap_cm', 'maximum_gap_cm', 'reference_vertical_delta_cm', 'vertical_lift_cm'):
            need(isinstance(row.get(key), (int, float)) and math.isfinite(row[key]), 'Endelig målt/utledet scalar kreves')
        for key in ('reference_actual_foot_output_cm', 'reference_seed_foot_output_cm', 'target_output_cm'):
            v = row.get(key)
            need(isinstance(v, list) and len(v) == 3 and all(type(x) in (int, float) and math.isfinite(x) for x in v),
                 'Komplett faktisk fotvektor kreves')
    need({x['output_side'] for x in targets.values()} == {'negative_Y', 'positive_Y'}, 'Ulike faktiske output-sider kreves')
    packages = native.get('postrepair_native_packages_actual')
    need(isinstance(packages, list) and len(packages) == 4 and len({x['asset'] for x in packages}) == 4,
         'Fire distinkte faktisk leste native pakker kreves')
    expected = load(pinned(binding['acceptance_file']))['native_assets']
    need({x['asset'] for x in packages} == set(expected), 'Faktiske native pakker avviker fra R2-bindingen')
    for row in packages:
        need(row['file'] == expected[row['asset']]['file'], 'Native pakkefilrad avviker fra frosset R2')
        q = pinned(row['file'])
        need(q.is_relative_to(Path(job['project']) / 'Content'), 'Native pakkefil ligger utenfor prosjektet')
        need(isinstance(row.get('actual_lod_triangles'), list) and row['actual_lod_triangles']
             and all(type(x) in (int, float) and math.isfinite(x) and x > 0 and int(x) == x
                     for x in row['actual_lod_triangles']) and row.get('lod_scope'),
             'Faktiske native LOD-tall/scope kreves')

    garment = load(pinned(job['garment_source_binding']))
    need(native.get('garment_source_binding_version') == 1
         and native.get('garment_source_binding_file') == job['garment_source_binding']
         and native.get('garment_source_binding') == garment
         and native.get('garment_reference_pose_unchanged') is True,
         'Faktisk ny R12-binær må bekrefte kildebinding og eksakt R11 pose')
    bound = load(garment['bind_receipt_file']['path'])
    need(native.get('garment_source_geometry_actual') == bound['clothes_bind']['geometry_after'],
         'Faktisk komplett ClothesR2 import/render/Description-payload må matche akseptert bind')


def editor_entry():
    import unreal
    plan_path = Path(os.environ['S47_R12_RUN_PLAN']).resolve(strict=True)
    plan = load(plan_path)
    need(plan['status'] == 'PREPARED_NOT_RUN', 'Bare fryst forberedt privat plan kan kjøres')
    run = Path(plan['run_root'])
    need(plan_path == run / 'plan.json' and run.parent == ROOT / 'r12-runs', 'Eksakt privat planbane kreves')
    pinned(plan['inputs']); pinned(plan['protection'])
    inputs = load(plan['inputs']['path'])
    input_pins(inputs)
    job_path = Path(os.environ['S47_MANNEQUIN_JOB']).resolve(strict=True)
    matches = [mode for mode, pair in plan['jobs'].items() if pair['actual']['path'] == str(job_path)]
    need(len(matches) == 1, 'Eksakt forberedt actual-jobb kreves')
    mode = matches[0]
    pair = plan['jobs'][mode]
    pinned(pair['actual']); pinned(pair['clothes'])
    job, cloth_job = load(pair['actual']['path']), load(pair['clothes']['path'])
    report_path = Path(job['python_report'])
    need(not report_path.exists(), 'Tidligere PASS/FAIL kan aldri erstattes')
    report = {'status': 'STARTED_NOT_ACCEPTED', 'mode': mode, 'pid': os.getpid(),
              'plan': file_row(plan_path), 'job': pair['actual'], 'runtime_accepted': False,
              'contact_accepted': False, 'adoption_accepted': False}
    protection = load(plan['protection']['path'])
    p = prepare_module(inputs['pins']['prepare']['path'])
    base = None

    def protect(required):
        need(tree(Path(base['python_report']).parent) == protection['r10_private'], 'R10s private filer er endret')
        need(tree(ROOT/'private') == protection['all_private'], 'Private kilder/backup/R10filer er endret')
        need(tree(ROOT/'r11-runs') == protection['r11_runs'], 'R11-forfatterfiler er endret')
        binding = load(inputs['pins']['garment_source_binding']['path'])
        need(tree(Path(binding['source_plan_file']['path']).parent) == protection['garment_source_run'], 'Akseptert ClothesR2-sourceimport/bind/leser er endret')
        original_sources(base, p)
        return content_delta(protection['content'], tree(Path(job['project']) / 'Content'), required)

    def command(path):
        data = load(path)
        need(Path(data['native_report']).parent == run and not Path(data['native_report']).exists(), 'Ny egen native rapport kreves')
        previous = os.environ.get('S47_MANNEQUIN_JOB')
        try:
            os.environ['S47_MANNEQUIN_JOB'] = str(path)
            unreal.SystemLibrary.execute_console_command(None, 'S47.MannequinBake')
        finally:
            if previous is None:
                os.environ.pop('S47_MANNEQUIN_JOB', None)
            else:
                os.environ['S47_MANNEQUIN_JOB'] = previous
        need(Path(data['native_report']).is_file(), 'Bygget native kommando skrev ingen rapport')
        return load(data['native_report'])

    try:
        need(unreal.SystemLibrary.get_engine_version() == inputs['engine_version'], 'Feil faktisk motorversjon')
        need(Path(unreal.Paths.project_dir()).resolve() == Path(job['project']).resolve(), 'Feil faktisk editorprosjekt')
        tokens = shlex.split(unreal.SystemLibrary.get_command_line())
        profiles = [x.split('=', 1)[1] for x in tokens if x.lower().startswith('-userdir=')]
        need(profiles == [inputs['profiles'][mode]], 'Feil/duplisert eksplisitt isolert UserDir')
        need(Path(unreal.Paths.project_saved_dir()).resolve().is_relative_to(Path(profiles[0])), 'Faktisk Saved ligger utenfor privat profil')
        need(mode == 'probe' or '-nullrhi' not in [x.lower() for x in tokens], 'Bake/verify krever faktisk RHI')
        base, cfg, imported, field = r10_evidence(inputs, p)
        need(job == make_jobs(base, field, run)[mode]['actual']
             and cloth_job == make_jobs(base, field, run)[mode]['clothes'], 'Jobbfelt er endret, ingen skjult normalisering tillates')
        need(imported['pid'] != os.getpid(), 'Ny prosess etter original import/bind kreves')
        for prior in MODES[:MODES.index(mode)]:
            old = load(run / (prior + '-python.json'))
            need(old['status'] == 'PASS' and old['mode'] == prior and type(old['pid']) is int and old['pid'] > 0
                 and old['pid'] != os.getpid() and old['plan'] == file_row(plan_path)
                 and old['job'] == plan['jobs'][prior]['actual'], 'Tidligere vellykket fase i forskjellig PID kreves')
            pinned(old['native_report']); pinned(old['clothes_verify'])
        report['preservation_before'] = protect(mode == 'verify')
        h = readonly_editor_helpers(inputs['pins']['run_editor']['path'], unreal)
        h['sources_available'](cfg)
        material_state = h['cloth_materials'](job, cfg)
        need(material_state == imported['cloth_materials'], 'R10 materialer/parent/verdier er endret')
        report['cloth_materials'] = material_state
        actual_cloth = command(pair['clothes']['path'])
        view = dict(cloth_job, asset_root='/Game/MetaHumans/Mannequins/RadioWoman/ClothesR2', clothes_bind_verify_report=cloth_job['native_report'])
        verified = p.clothes_bind_verification(view, cfg, imported)
        need(actual_cloth == verified and verified['pid'] == os.getpid(), 'Faktisk fersk READONLY ClothesR2-binding kreves')
        report['clothes_job'] = pair['clothes']
        report['clothes_verify'] = file_row(cloth_job['native_report'])
        p.imported_receipt(base, cfg)
        need(material_state == h['cloth_materials'](job, cfg), 'Read-only kleskontroll endret materialbindinger')
        protect(mode == 'verify')
        if mode == 'verify':
            baked = load(run / 'bake-python.json')
            need(file_row(Path(job['project']) / 'Content' / OUTPUT_REL) == baked['output_file'], 'Bakt R12-fil endret før fersk reload')
        if mode == 'bake':
            folder = (Path(job['project']) / 'Content' / OUTPUT_REL).parent
            need(not folder.exists(), 'Ny R12-outputmappe må fortsatt være fraværende før bake')
            folder.mkdir()  # Bare eget nye pakkested, aldri R10 eller andre assetmapper.
        native = command(pair['actual']['path'])  # Actual-jobben er gjenopprettet i finally.
        binding = load(pinned(job['postrepair_binding']))
        native_evidence(native, job, binding)
        report['native_report'] = file_row(job['native_report'])
        if mode != 'probe':
            report['output_file'] = file_row(Path(job['project']) / 'Content' / OUTPUT_REL)
            need(native['output']['asset'] == R12 + '.SM_RadioWoman_Seated', 'Faktisk output peker ikke på R12')
        if mode == 'verify':
            need(report['output_file'] == baked['output_file']
                 and native['output'] == load(run / 'bake-native.json')['output'], 'Fersk full R12-outputaudit avviker fra faktisk bake')
        need(material_state == h['cloth_materials'](job, cfg), 'Native fase endret R10s materialbindinger')
        p.imported_receipt(base, cfg)
        report['preservation_after'] = protect(mode != 'probe')
        report['engine_version'] = unreal.SystemLibrary.get_engine_version()
        report['status'] = 'PASS'
        report['limits'] = 'Kun privat R12 nativeforfatting. Ingen runtime-, kontakt-, utseende-, input-, installasjons- eller releaseaksept.'
    except Exception as error:
        report.update(status='FAIL', error=repr(error))
        if base:
            try:
                report['preservation_after'] = protect(None)
            except Exception as changed:
                report['preservation_error'] = repr(changed)
        new_json(report_path, report)
        unreal.log_error('S47 R12 OUTPUT-ONLY ' + mode.upper() + ' FAIL: ' + repr(error))
        raise
    new_json(report_path, report)
    unreal.log('S47 R12 OUTPUT-ONLY ' + mode.upper() + ' PASS')


def main():
    if 'S47_R12_RUN_PLAN' in os.environ:
        editor_entry()
        return
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare', required=True, type=Path, help='Faktisk source-/buildbundet privat R12-input, ingen editorstart')
    args = parser.parse_args()
    prepare_run(args.prepare)



if __name__ == '__main__':
    main()
