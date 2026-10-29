#!/usr/bin/env python3
"""Privat ClothesR2: to nye kildeassets, separat bind/lagret leser, ingen UE-start.

Root kjører --prepare/--binding som CPU-steg og editor_entry i hver sin guardjobb.
Ingen gamle R10/R11-rapporter eller eksisterende assets kan overskrives.
"""
from __future__ import annotations
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import types

ROOT = Path('/home/tom/scratch-bm/Signal47-mannequins-20261029')
SOURCE_ROOT = '/Game/MetaHumans/Mannequins/RadioWoman/ClothesR2'
CLOTHES = SOURCE_ROOT + '/SKM_RadioWoman_Clothes'
SKELETON = SOURCE_ROOT + '/SKEL_RadioWoman_Clothes'
R10 = '/Game/MetaHumans/Mannequins/RadioWoman/BakeR10'
R11 = '/Game/MetaHumans/Mannequins/RadioWoman/BakeR11/SM_RadioWoman_Seated'


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def load(path):
    return json.loads(Path(path).read_text())


def new_json(path, value):
    with Path(path).open('x', encoding='utf-8') as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2)
        stream.write('\n')


def digest(path, algorithm='sha256'):
    h = hashlib.new(algorithm)
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(8 << 20), b''):
            h.update(block)
    return h.hexdigest()


def row(path):
    path = Path(path)
    need(path.is_absolute() and path.is_file() and not path.is_symlink(), 'Vanlig absolutt faktisk fil kreves')
    p = path.resolve(strict=True)
    need(p == path and '..' not in path.parts, 'Ingen indirekte/normalisert filbane')
    return {'path': str(p), 'bytes': p.stat().st_size, 'sha256': digest(p), 'sha1': digest(p, 'sha1')}


def pin(value):
    need(isinstance(value, dict) and set(value) == {'path', 'bytes', 'sha1', 'sha256'}, 'Faktiske dualhash/bytes må fylles før kjøring')
    need(value == row(value['path']), 'Endret pinnet fil: ' + str(value.get('path')))
    return Path(value['path'])


def module(path, name):
    obj = types.ModuleType(name)
    obj.__file__ = str(path)
    exec(compile(Path(path).read_text(), str(path), 'exec'), obj.__dict__)
    return obj


def legacy_helpers(path, unreal):
    # Gjenbruk bare de beviste read-only-getterne. main/import_private kjøres ikke.
    names = {'sources_available', 'cloth_materials'}
    definitions = [n for n in ast.parse(Path(path).read_text()).body
                   if isinstance(n, ast.FunctionDef) and n.name in names]
    need({n.name for n in definitions} == names, 'Eksakte eksisterende getterfunksjoner')
    scope = {'unreal': unreal, 'require': need}
    exec(compile(ast.Module(body=definitions, type_ignores=[]), str(path), 'exec'), scope)
    return scope


def asset_file(project, asset):
    package = asset.split('.', 1)[0]
    need(package.startswith('/Game/') and '..' not in package, 'Eksakt nativepakke kreves')
    return Path(project) / 'Content' / (package.removeprefix('/Game/') + '.uasset')


def tree(path):
    result = {}
    for p in sorted(Path(path).rglob('*')):
        need(not p.is_symlink(), 'Symlink i beskyttet kildetre')
        if p.is_file():
            result[p.relative_to(path).as_posix()] = row(p)
    return result


def validate_fresh_source(report_path, fresh_path):
    source=load(report_path); fresh=load(fresh_path); yoke=source['coat_yoke']
    need(source['status']=='REST_CLOTHES_EXPORTED_NOT_ACCEPTED' and source['blend_reopened'] is True
         and source['pid']>0 and fresh['pid']>0 and source['pid']!=fresh['pid']
         and fresh['producer_pid']==source['pid']
         and fresh['status']=='FRESH_SOURCE_SAMPLED_PASS_NOT_NATIVE_OR_APPEARANCE'
         and fresh['mode']=='verify-source' and fresh['source_report']==row(report_path)
         and fresh['source_mutated'] is False and fresh['asset_save_called'] is False
         and fresh['native_accepted'] is False and fresh['appearance_accepted'] is False,
         'Separate actual source production and fresh saved-source reader required')
    need(source['source_body_payload_preserved'] is True
         and source['rest_skeleton_sha256']=='9b1eb0e830b3041e7d931692f06845d9c433b7265bce9b8c5ed09e2463eea113'
         and len(source['rest_skeleton']['bones'])==341 and source['rest_skeleton']['object_name']=='root'
         and source['source_reference_actual_before_gate']['bone_count']==341
         and source['source_reference_actual_before_gate']['separate_native_bones']==342
         and source['source_reference_actual_before_gate']['rest_skeleton']==source['rest_skeleton']
         and fresh['source_reference_sha256']==source['rest_skeleton_sha256']
         and fresh['garment_payload_snapshot']==source['garment_payload_snapshot']
         and fresh['source_part_identity_sha256']==hashlib.sha256(json.dumps(source['source_part_identity'],sort_keys=True,separators=(',',':')).encode()).hexdigest(),
         'Exact actual341 source/native342 separation and actual saved mesh payload required')
    need(yoke['policy']=='actual-combined-skin-local-coat-yoke-v1'
         and yoke['status']=='REST_YOKE_MEASURED_NOT_SEATED_OR_NATIVE_ACCEPTED'
         and yoke['mutable_part']=='Coat_body_open_lower_front' and yoke['preserved_original_parts']==16
         and len(yoke['preserved_parts'])==16 and 'Coat_body_open_lower_front' not in yoke['preserved_parts']
         and yoke['source_body_faces_copied'] is False and yoke['source_body_mutated'] is False
         and yoke['native_accepted'] is False and yoke['seated_fit_accepted'] is False
         and fresh['source_files']==yoke['source_files'], 'Bounded upper ownCoat+16 preserved parts, never old2caps/17')
    for value in source['producer_files'].values():pin(value)
    for value in yoke['source_files'].values():pin(value)
    source_inputs=load(pin(yoke['inputs']))
    need(source_inputs['status']=='PINNED_ACTUAL_SKIN_AND_OWN_SURFACE_ONLY'
         and source_inputs['files']==yoke['source_files'], 'Exact immutable skin/source chain')
    frame=load(yoke['source_files']['frame_confirmation']['path'])
    need(frame['status']=='PASS_COMPLETE_NATIVE_SKIN_FRAME_ONLY_NOT_GARMENT_FIT'
         and all(v is True for v in frame['checks'].values())
         and frame['native_report']==yoke['source_files']['native_rest']
         and frame['converted_skin']==yoke['source_files']['converted_skin'], 'Measured native→FBX frame and complete visible skin required')
    need(source['triangles']==23368 and source['triangles']<=24000,'Actual bounded new garment triangle budget')
    for name,value in yoke['preserved_parts'].items():
        need(value==source['existing_parts_before_yoke'][name]
             and value==source['existing_parts_preserved_before_weight_transfer'][name],
             'One of16 actual original parts changed')
    for fit in (yoke['fit'],source['saved_reopened_fit'],fresh['actual_fit']):
        need(fit['status']=='REST_YOKE_SAMPLED_NOT_SEATED_OR_NATIVE_ACCEPTED'
             and fit['minimum_clearance_m']==.006 and fit['outer_target_m']==.012
             and fit['continuous_surface_proof'] is False and fit['upper_facet_samples']==10752
             and fit['upper_facet_minimum_m']>=.006 and fit['required_skin_rays']>0
             and fit['required_skin_minimum_m']>=.006 and fit['actual_inner_minimum_m']>0,
             'Actual finite face/skin samples and unchanged6mm outer clearance required')
        upper=[q for q in fit['facets'] if q['band']>=20]
        need(len(upper)==10752,'Actual finite upper facet footprint')
        for q in upper:
            a=q['actual_skin']; distance=sum((x-y)**2 for x,y in zip(q['point'],a['point']))**.5
            need(a['classification_consistent'] is True and a['inside'] is False
                 and all(n%2==0 for n in a['ray_counts'])
                 and abs(distance-a['signed_distance_m'])<=1e-8 and distance>=.006,
                 'Actual skin clearance must reconstruct from raw measured points/parity')
        required=[q for q in fit['coverage'] if q['required']]
        need(len(required)==fit['required_skin_rays'],'Required skin sample count mismatch')
        for q in required:
            hit=q['selected']
            need(hit is not None and hit in q['all_hits'] and hit['normal_dot']>.05,'Actual external garment hit required')
            measured=sum((hit['point'][i]-q['skin'][i]-q['normal'][i]*1e-6)*q['normal'][i] for i in range(3))
            need(abs(measured-hit['distance'])<1e-7 and measured>=.006,'Actual cloth clearance must reconstruct from raw ray')
    for path,sha in source['output_files_sha256'].items():
        measured=row(path);need(measured['sha256']==sha and fresh['output_files'][path]==measured,'Actual source output bytes changed')
    blends=[Path(p) for p in source['output_files_sha256'] if p.endswith('.blend')]
    fbxs=[Path(p) for p in source['output_files_sha256'] if p.endswith('.fbx')]
    need(len(blends)==len(fbxs)==1,'Exact actual blend/FBX source pair')
    return fbxs[0],blends[0],source


def yoke_source(report, fresh_report, p):
    # Common original FBX/material/ref rules remain in the actual prepare helper.
    fbx,source=p.clothing_check(report,fresh_source_report=fresh_report)
    checked,blend,full=validate_fresh_source(report,fresh_report)
    need(fbx==checked and source==full,'Same actual source report/FBX')
    return fbx,blend,source


def inputs_checked(path):
    inputs = load(path)
    need(inputs['schema_version'] == 1 and inputs['status'] == 'ROOT_COMPLETED_CLOTHES_R2_INPUTS', 'Root må fylle faktiske nye produksjons-/byggpinner')
    required = {'helper', 'prepare', 'run_editor', 'native_cpp', 'editor_module', 'build_receipt',
                'r10_job', 'r11_job', 'r11_bake', 'r11_native', 'source_report',
                'source_build', 'source_geometry', 'source_shoulder_helper', 'source_contract', 'source_reader', 'source_surface_helper', 'source_fresh_report'}
    need(set(inputs['pins']) == required, 'Eksakt komplett kilde-/bygg-/nativeinventar kreves')
    for value in inputs['pins'].values():
        pin(value)
    need(inputs['pins']['helper'] == row(__file__), 'Denne fryste ClothesR2-helper kreves')
    proof = load(inputs['pins']['build_receipt']['path'])
    need(re.fullmatch('[0-9a-f]{40}', inputs['source_commit']) is not None, 'Eksakt faktisk bygget sourcecommit kreves')
    need(proof['schema_version'] == 1 and proof['status'] == 'PASS'
         and proof['source_commit'] == inputs['source_commit'], 'Faktisk kompilert ny modulkvittering kreves')
    for key in ('source_freeze', 'editor_module', 'build_guard', 'build_log'):
        # Root-formatet har SHA256-triplet. Match faktiske bytes uavhengig av SHA1-tillegget.
        measured = row(proof[key]['path'])
        need(proof[key] == {k: measured[k] for k in ('path', 'bytes', 'sha256')}, 'Faktisk buildbinding endret')
    need(proof['editor_module'] == {k: inputs['pins']['editor_module'][k] for k in ('path', 'bytes', 'sha256')}, 'Annen bygget modul')
    guard = load(proof['build_guard']['path'])
    need(guard.get('returncode') == 0 and guard.get('semantic_success') is True
         and guard.get('contention') == [] and guard.get('stop_reason') is None
         and guard.get('performance_status') == 'UNCONTENDED', 'Faktisk fullført editorbygg kreves, ikke bare selvrapportert PASS')
    project = Path(inputs['project'])
    for key, relative in {'prepare':'Tools/metahuman_mannequins/prepare.py','run_editor':'Tools/metahuman_mannequins/run_editor.py',
                          'native_cpp':'Source/Signal47/Private/S47MannequinBakeEditor.cpp','editor_module':'Binaries/Linux/libUnrealEditor-Signal47.so'}.items():
        need(Path(inputs['pins'][key]['path']) == project/relative, 'Eksakt kilde/modul i det faktiske prosjektet kreves')
    frozen = load(proof['source_freeze']['path'])
    need(frozen['schema_version'] == 1 and frozen['status'] == 'FROZEN_SOURCE_AND_MEASURED_NATIVE'
         and Path(frozen['worktree']) == project.parents[1], 'Faktisk frosset prosjektkilde kreves')
    for frozen_row in frozen['files']:
        measured = row(frozen_row['path'])
        need(frozen_row == {k:measured[k] for k in ('path','bytes','sha256')}, 'Faktisk bygget kildeinventar endret')
    frozen_paths = {x['path'] for x in frozen['files']}
    required_paths = {str(q) for folder in ('Source','Config') for q in (project/folder).rglob('*') if q.is_file()}
    required_paths.add(str(project/'Signal47.uproject'))
    need(required_paths <= frozen_paths, 'Fullt faktisk Source/Config/uproject-inventar må være med i byggets kildesnitt')
    need(frozen['source_commit'] == inputs['source_commit']
         and {k: inputs['pins']['native_cpp'][k] for k in ('path', 'bytes', 'sha256')} in frozen['files'], 'Faktisk CPP må være i byggets kildefrys')
    p = module(inputs['pins']['prepare']['path'], 's47_r12_pinned_prepare')
    base = load(inputs['pins']['r10_job']['path']); cfg = load(base['contract'])
    need(base['asset_root'] == R10 and p.sha(base['contract']) == base['contract_sha256'], 'Uendret R10 material-/Body-/Facekontrakt')
    p.imported_receipt(base, cfg)
    source_inventory = load(base['source_snapshot'])
    for pinned_original in source_inventory['pins'].values():
        need(p.file_row(pinned_original['path']) == {k:pinned_original[k] for k in ('bytes','sha256')}, 'Opprinnelig Creator/FBX/nativekilde er endret')
    need(p.sha(base['source_snapshot']) == base['source_snapshot_sha256'], 'Uendret originalfilinventar')
    for relative, expected in source_inventory['files'].items():
        source = Path(source_inventory['source_root']) / relative
        backup = Path(source_inventory['backup_root']) / relative
        need(p.file_row(source) == p.file_row(backup) == expected
             and (source.stat().st_dev, source.stat().st_ino) != (backup.stat().st_dev, backup.stat().st_ino),
             'Separate, byteidentiske originalkopier kreves: ' + relative)
    r11job, r11bake = load(inputs['pins']['r11_job']['path']), load(inputs['pins']['r11_bake']['path'])
    need(r11job['output_asset'] == R11 and r11bake['status'] == 'PASS' and r11bake['mode'] == 'bake'
         and r11bake['output']['asset'] == R11 + '.SM_RadioWoman_Seated'
         and asset_file(base['project'], R11).resolve() == pin(inputs['pins']['r11_native']), 'Faktisk bevart R11-referanse')
    fbx, blend, source = yoke_source(inputs['pins']['source_report']['path'], inputs['pins']['source_fresh_report']['path'], p)
    for key, field in [('source_build','recipe_sha256'), ('source_geometry','geometry_sha256'),
                       ('source_shoulder_helper','shoulder_recipe_sha256'), ('source_contract','clothing_contract_sha256')]:
        need(inputs['pins'][key]['sha256'] == source[field], 'Garmentrapport må binde faktisk kjørte nye oppskrift')
    need(inputs['pins']['source_reader']==source['producer_files']['verify_source.py']
         and inputs['pins']['source_surface_helper']==source['producer_files']['surface_cpu.py'], 'Exact actual fresh-reader/fitting code')
    need(Path(base['project']).resolve() == Path(inputs['project']).resolve(), 'Eksakt faktisk prosjekt')
    return inputs, p, base, cfg, fbx, blend


def protection(plan):
    before = load(pin(plan['protection']))
    need(tree(ROOT/'private') == before['private'], 'En eksisterende privat kilde/backup/R10fil er endret')
    need(tree(ROOT/'r11-runs') == before['r11_runs'], 'En eksisterende R11 forfatterkvittering er endret')
    expected = before['content']
    actual = tree(Path(plan['project']) / 'Content')
    need(all(actual.get(key) == value for key, value in expected.items()), 'En tidligere Contentfil er endret/slettet')
    extra = set(actual) - set(expected)
    allowed = {str(asset_file(plan['project'], x).relative_to(Path(plan['project']) / 'Content')) for x in (CLOTHES, SKELETON)}
    need(extra <= allowed, 'Kun de to nye ClothingR2-kildene kan skapes')
    return {'all_preexisting_content_preserved': True, 'all_private_inventory_preserved':True,
            'all_r11_authoring_receipts_preserved':True, 'preserved_files': len(expected),
            'preserved_private_files':len(before['private']), 'preserved_r11_receipts':len(before['r11_runs']), 'new_files': sorted(extra)}


def prepare(path):
    inputs, p, base, cfg, fbx, blend = inputs_checked(path)
    run = Path(inputs['run_root'])
    need(run.is_absolute() and run.parent == ROOT / 'r12-garment-source-runs' and not run.exists(), 'Ny privat source-runmappe')
    need(not asset_file(base['project'], CLOTHES).parent.exists(), 'Hele ClothesR2-nativepakken må være ny')
    need(set(inputs['profiles']) == {'garment-import', 'garment-verify'} and len(set(inputs['profiles'].values())) == 2, 'To ulike isolerte prosesser')
    for profile in inputs['profiles'].values():
        need(str(profile).startswith('/tmp/S47-') and Path(profile).is_absolute() and '..' not in Path(profile).parts and not Path(profile).exists(), 'Fersk separat profil')
    before = {'content':tree(Path(base['project'])/'Content'),'private':tree(ROOT/'private'),'r11_runs':tree(ROOT/'r11-runs')}
    run.mkdir(parents=True)
    new_json(run / 'inputs.json', inputs); new_json(run / 'before.json', before)
    plan = {'schema_version': 1, 'status': 'PREPARED_NOT_RUN', 'project': base['project'],
            'run_root': str(run), 'inputs': row(run/'inputs.json'), 'protection': row(run/'before.json'), 'jobs': {}}
    for mode in ('garment-import', 'garment-verify'):
        native_mode = 'bind-clothes' if mode == 'garment-import' else 'verify-clothes-bind'
        native = dict(base, run_id='ClothesR2', mode=native_mode, clothes_asset=CLOTHES, private_skeleton=SKELETON,
                      output_asset=SOURCE_ROOT+'/SM_UnwrittenBindTarget',
                      clothing_report=inputs['pins']['source_report']['path'], clothing_report_sha256=inputs['pins']['source_report']['sha256'],
                      clothing_fbx=str(fbx), clothing_fbx_sha256=digest(fbx),
                      clothes_bind_report=str(run/'bind-native.json'), clothes_bind_verify_report=str(run/'bind-verify-native.json'),
                      python_report=str(run/(mode+'.json')),
                      native_report=str(run/('bind-native.json' if native_mode == 'bind-clothes' else 'bind-verify-native.json')))
        target = run / ('job-' + mode + '.json'); new_json(target, native); plan['jobs'][mode] = row(target)
    new_json(run/'plan.json', plan)
    print(json.dumps({'status':'PREPARED_NOT_RUN','plan':row(run/'plan.json'),'engine_started':False}))


def editor_entry():
    import unreal
    plan_path = Path(os.environ['S47_R12_GARMENT_PLAN'])
    plan = load(plan_path); pin(plan['inputs']); pin(plan['protection'])
    inputs, p, base, cfg, fbx, blend = inputs_checked(plan['inputs']['path'])
    run = Path(plan['run_root']); mode = os.environ['S47_R12_GARMENT_MODE']
    need(mode in ('garment-import', 'garment-verify') and plan_path == run/'plan.json', 'Eksakt kildeplan og fase')
    native_path = pin(plan['jobs'][mode]); job = load(native_path)
    output = run / (mode + '.json'); need(not output.exists(), 'Ingen rapport erstattes')
    report = {'schema_version':1,'status':'STARTED_NOT_ACCEPTED','mode':mode,'pid':os.getpid(),
              'plan':row(plan_path),'job':row(native_path),'blender_source_verify':inputs['pins']['source_fresh_report']}
    try:
        need(unreal.SystemLibrary.get_engine_version() == inputs['engine_version'], 'Faktisk korrekt UE5.8.4')
        need(Path(unreal.Paths.project_dir()).resolve() == Path(base['project']).resolve(), 'Faktisk isolert prosjekt')
        args = shlex.split(unreal.SystemLibrary.get_command_line())
        need('-nullrhi' in [x.lower() for x in args], 'Garmentimport/bindleser er eksplisitt NullRHI, aldri GPUaksept')
        need([x.split('=',1)[1] for x in args if x.lower().startswith('-userdir=')] == [inputs['profiles'][mode]], 'Eksakt isolert UserDir')
        need(Path(unreal.Paths.project_saved_dir()).resolve().is_relative_to(Path(inputs['profiles'][mode])), 'Saved under isolert profil')
        report.update(protection(plan))
        h = legacy_helpers(inputs['pins']['run_editor']['path'], unreal)
        sources = h['sources_available'](cfg)
        old_materials = h['cloth_materials'](base, cfg)
        if mode == 'garment-import':
            need(not any(asset_file(base['project'], x).exists() or unreal.EditorAssetLibrary.does_asset_exist(x) for x in (CLOTHES,SKELETON)), 'Kun fersk privat import')
            unreal.EditorAssetLibrary.make_directory(SOURCE_ROOT)
            skel = unreal.EditorAssetLibrary.duplicate_asset(cfg['sources']['skeleton'], SKELETON)
            need(isinstance(skel, unreal.Skeleton) and skel != sources['skeleton'], 'Eksakt privat skeletonkopi')
            need(unreal.EditorAssetLibrary.save_loaded_asset(skel), 'Lagre bare ny privat skeletonkopi')
            task = unreal.AssetImportTask()
            task.filename = str(fbx)
            task.destination_path, task.destination_name = CLOTHES.rsplit('/',1)
            task.automated, task.replace_existing, task.save = True, False, True
            options = unreal.FbxImportUI()
            options.import_as_skeletal = True
            options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
            options.original_import_type = unreal.FBXImportType.FBXIT_SKELETAL_MESH
            options.skeleton = skel
            options.import_materials, options.import_textures, options.import_animations = False,False,False
            options.create_physics_asset = False
            data = options.skeletal_mesh_import_data
            for name, value in {'import_content_type':unreal.FBXImportContentType.FBXICT_ALL,
                'update_skeleton_reference_pose':False,'use_t0_as_ref_pose':False,'preserve_smoothing_groups':True,
                'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS,
                'import_uniform_scale':1.,'import_translation':unreal.Vector(0,0,0),'import_rotation':unreal.Rotator(0,0,0),
                'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False}.items():
                data.set_editor_property(name,value)
            task.options = options
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            cloth = unreal.load_asset(CLOTHES)
            need(isinstance(cloth,unreal.SkeletalMesh) and cloth.get_editor_property('skeleton') == skel, 'Faktisk klær mot egen skeleton')
            slots = list(cloth.get_editor_property('materials'))
            names = [str(s.get_editor_property('material_slot_name')) for s in slots]
            old_cloth = unreal.load_asset(base['clothes_asset'])
            need(names == [str(s.get_editor_property('material_slot_name')) for s in old_cloth.get_editor_property('materials')]
                 and len(names) == 5, 'Bevar eksakt målt slotrekkefølge')
            for slot in slots:
                slot.set_editor_property('material_interface', unreal.load_asset(base['asset_root']+'/'+str(slot.get_editor_property('material_slot_name'))))
            cloth.set_editor_property('materials',slots)
            need(unreal.EditorAssetLibrary.save_loaded_asset(cloth), 'Lagre bare ny klærmesh')
            need(unreal.EditorAssetLibrary.save_loaded_asset(skel), 'Lagre bare ny import-skeleton')
            report['skeleton_before_bind'] = row(asset_file(base['project'],SKELETON))
        else:
            imported = load(run/'garment-import.json')
            need(imported['status'] == 'PASS' and imported['pid'] != os.getpid(), 'Uavhengig fersk leser etter faktisk sourceimport')
            for value in imported['saved_assets'].values(): pin(value)
        need(not Path(job['native_report']).exists(), 'Fersk nativereceipt')
        previous = os.environ.get('S47_MANNEQUIN_JOB')
        try:
            os.environ['S47_MANNEQUIN_JOB'] = str(native_path)
            unreal.SystemLibrary.execute_console_command(None,'S47.MannequinBake')
        finally:
            if previous is None: os.environ.pop('S47_MANNEQUIN_JOB',None)
            else: os.environ['S47_MANNEQUIN_JOB'] = previous
        view = dict(job,asset_root=SOURCE_ROOT)
        bound = p.clothes_bind_receipt(view,cfg)
        if mode == 'garment-verify':
            view['clothes_bind_verify_report'] = job['native_report']
            verified = p.clothes_bind_verification(view,cfg)
            need(verified['pid'] == os.getpid(), 'Faktisk samme leser-PID')
            report['bind_verify_file'] = row(job['native_report'])
            need(imported['plan'] == row(plan_path), 'Uendret kildeplan')
        else:
            need(bound['pid'] == os.getpid(), 'Faktisk samme import/bind-PID')
            need(report['skeleton_before_bind'] == row(asset_file(base['project'],SKELETON)), 'Native bind må ikke lagre skeleton')
        report['bind_file'] = row(job['clothes_bind_report'])
        report['saved_assets'] = {x:row(asset_file(base['project'],x)) for x in (CLOTHES,SKELETON)}
        if mode == 'garment-verify': need(report['saved_assets'] == imported['saved_assets'], 'Fersk leser bevarer begge sourcefiler')
        report['actual_materials'] = h['cloth_materials'](job,cfg)
        need(report['actual_materials'] == old_materials, 'Nye klær bruker de fem uendrede faktiske R10 MICene')
        p.imported_receipt(base,cfg)
        report.update(protection(plan)); report['status']='PASS'
        report['limits']='Kun privat sourceimport/canonical bind/fersk leser. Ingen R12 pose, statisk bake, skulderutseende eller runtimeaksept.'
    except Exception as error:
        report.update(status='FAIL',error=repr(error))
        try: report['preservation_after_failure']=protection(plan)
        except Exception as changed: report['preservation_error']=repr(changed)
        new_json(output,report)
        unreal.log_error('S47 R12 GARMENT '+mode+' FAIL '+repr(error)); raise
    new_json(output,report)
    unreal.log('S47 R12 GARMENT '+mode+' PASS')


def validate_binding_value(binding):
    need(binding['schema_version']==1 and binding['status']=='READY_FOR_AUTHORING_ONLY'
         and binding['policy']=='R12-local-coat-yoke16-only-r11-pose'
         and binding['runtime_accepted'] is False and binding['appearance_accepted'] is False, 'Eksplisitt lukket R12-kildepolicy')
    for key,value in binding.items():
        if key.endswith('_file'): pin(value)
    for value in binding['new_native_assets'].values(): pin(value)
    for value in binding['unchanged_native_assets'].values(): pin(value)
    imported, verified = load(binding['import_receipt_file']['path']),load(binding['source_verify_file']['path'])
    bound, bindverified = load(binding['bind_receipt_file']['path']),load(binding['bind_verify_file']['path'])
    need(imported['status']==verified['status']==bound['status']==bindverified['status']=='PASS'
         and imported['mode']=='garment-import' and verified['mode']=='garment-verify'
         and bound['mode']=='bind-clothes' and bindverified['mode']=='verify-clothes-bind'
         and imported['plan']==verified['plan']==binding['source_plan_file']
         and imported['pid']==bound['pid'] and verified['pid']==bindverified['pid'] and imported['pid']!=verified['pid']
         and imported['saved_assets']==verified['saved_assets']==binding['new_native_assets']
         and bound['clothes_bind']==bindverified['clothes_bind']
         and imported['all_preexisting_content_preserved'] is True and verified['all_preexisting_content_preserved'] is True
         and imported['all_private_inventory_preserved'] is True and verified['all_private_inventory_preserved'] is True
         and imported['all_r11_authoring_receipts_preserved'] is True and verified['all_r11_authoring_receipts_preserved'] is True,
         'Faktisk separat-PID source/bind/reopen og uendrede sourcefiler')
    validate_fresh_source(binding['source_report_file']['path'],binding['blender_source_verify_file']['path'])
    need(imported['blender_source_verify']==verified['blender_source_verify']==binding['blender_source_verify_file'], 'Same actual independent Blender source reader')
    state = bound['clothes_bind']
    actual_cloth_file = binding['new_native_assets'][CLOTHES]
    need(set(binding['new_native_assets']) == {CLOTHES,SKELETON}
         and state['asset'] == CLOTHES and state['private_skeleton'] == SKELETON
         and state['policy'] == 'canonical-native-body'
         and state['reference_before_sha1'] == '2144e1ffc0504791556fd705e21b37add599da6f'
         and state['reference_after_sha1'] == state['body_reference_sha1'] == '93838fa5259ddca476c5e523526483746f038133'
         and state['inverse_reference_after_sha1'] == state['body_inverse_reference_sha1']
         and state['preserved_payload'] is True and state['geometry_before'] == state['geometry_after']
         and state['payload_before_sha1'] == state['payload_after_sha1']
         and state['file_after'] == {k:actual_cloth_file[k] for k in ('path','bytes','sha1')}
         and bindverified['bind_receipt'] == {k:binding['bind_receipt_file'][k] for k in ('path','bytes','sha1')},
         'Eksakt actual ClothesR2 fil/ref/InvRef/helepayload og original bindreceipt kreves')
    need(binding['new_job_fields']=={'clothes_asset':CLOTHES,'private_skeleton':SKELETON,
                                    'clothes_bind_report':binding['bind_receipt_file']['path']}, 'Eksakt tre nye felt')
    return binding


def validate_binding(path):
    return validate_binding_value(load(pin(row(path))))


def binding_from_plan(plan_path, output):
    plan=load(plan_path); inputs,p,base,cfg,fbx,blend=inputs_checked(plan['inputs']['path'])
    run=Path(plan['run_root']); protection(plan)
    imported,verified=load(run/'garment-import.json'),load(run/'garment-verify.json')
    source_assets=set(cfg['sources'].values()) | {x['asset'] for x in load(Path(base['python_report']).parent/'import-python.json')['cloth_materials']['instances'].values()}
    source_assets |= {base[k] for k in ('cloth_master','brow_material','clothes_asset','private_skeleton')}
    # Match native GetPathName objectpaths, while file conversion strips the object suffix.
    canonical=lambda s:s if '.' in s else s+'.'+s.rsplit('/',1)[1]
    value={'schema_version':1,'status':'READY_FOR_AUTHORING_ONLY','policy':'R12-local-coat-yoke16-only-r11-pose',
           'new_job_fields':{'clothes_asset':CLOTHES,'private_skeleton':SKELETON,'clothes_bind_report':str(run/'bind-native.json')},
           'source_plan_file':row(plan_path),
           'source_report_file':inputs['pins']['source_report'],'blender_source_verify_file':inputs['pins']['source_fresh_report'],'source_fbx_file':row(fbx),'source_blend_file':row(blend),
           'import_receipt_file':row(run/'garment-import.json'),'source_verify_file':row(run/'garment-verify.json'),
           'bind_receipt_file':row(run/'bind-native.json'),'bind_verify_file':row(run/'bind-verify-native.json'),
           'reference_r11_job_file':inputs['pins']['r11_job'],'reference_r11_bake_file':inputs['pins']['r11_bake'],
           'reference_r11_native_file':inputs['pins']['r11_native'],
           'new_native_assets':verified['saved_assets'],
           'unchanged_native_assets':{canonical(x):row(asset_file(base['project'],x)) for x in sorted(source_assets)},
           'runtime_accepted':False,'appearance_accepted':False}
    need(imported['plan']==verified['plan']==row(plan_path),'Faktisk uendret kildeplan')
    output=Path(output)
    need(output.is_absolute() and output.parent.is_relative_to(ROOT) and output.parent.is_dir() and not output.exists(), 'Ny separat immutable kildebinding')
    validate_binding_value(value)  # No READY file is emitted before all checks pass.
    new_json(output,value)
    print(json.dumps({'status':'READY_FOR_AUTHORING_ONLY','binding':row(output),'runtime_accepted':False,'appearance_accepted':False}))


def main():
    if 'S47_R12_GARMENT_PLAN' in os.environ:
        editor_entry(); return
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare',type=Path);parser.add_argument('--binding',type=Path);parser.add_argument('--out',type=Path)
    args=parser.parse_args()
    need(bool(args.prepare)!=bool(args.binding),'Velg prepare eller binding; ingen motor startes')
    if args.prepare:
        need(args.out is None,'prepare bruker bare den eksplisitte nye run_root');prepare(args.prepare)
    else:
        need(args.out is not None,'Ny utfil kreves');binding_from_plan(args.binding,args.out)


if __name__=='__main__':main()
