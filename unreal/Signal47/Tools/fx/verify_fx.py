"""Read back the generated arc sparks and lock the C++/asset contract (fx_contract.json).

Called at the end of import_fx.py, or alone through UnrealEditor-Cmd -run=pythonscript
with S47_FX_REPORT set. Does not regenerate/edit graphs. Forces a save before
compile diagnostics so UNiagaraSystem::PreSave drains fresh-load compilation.
"""
import json
import os
import sys
import traceback
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
C = json.loads((ROOT / 'fx_contract.json').read_text())
TS = 'NiagaraToolsets.NiagaraToolset_System'
from arc_trajectory import assignments, kill_expression
EXPECT_SPAWN = ['InitializeParticle', 'ShapeLocation', 'AddVelocityInCone']


def verify_materials(report, check):
    E = unreal.EditorAssetLibrary
    M = unreal.MaterialEditingLibrary
    folder = C['folder']
    mat_path = folder + '/' + C['material']
    mat = unreal.load_asset(mat_path)
    check('material exists', mat is not None, mat_path)
    check('material is additive and unlit', mat.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_ADDITIVE and
          mat.get_editor_property('shading_model') == unreal.MaterialShadingModel.MSM_UNLIT)
    check('material is used with Niagara sprites', M.has_material_usage(mat, unreal.MaterialUsage.MATUSAGE_NIAGARA_SPRITES))
    tex = [e for e in M.get_material_expressions(mat) if isinstance(e, unreal.MaterialExpressionTextureBase)]
    check('material samples no texture', not tex, [t.get_name() for t in tex])
    arc_path = folder + '/' + C['arcMaterial']
    arc = unreal.load_asset(arc_path)
    check('arc mesh material exists', arc is not None, arc_path)
    check('arc mesh material additive unlit and ISM-compatible',
          arc.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_ADDITIVE and
          arc.get_editor_property('shading_model') == unreal.MaterialShadingModel.MSM_UNLIT and
          M.has_material_usage(arc, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES))
    expressions = M.get_material_expressions(arc)
    classes = [e.get_class().get_name() for e in expressions]
    check('arc material has no texture, particle or time dependency',
          not any('Texture' in n or 'Particle' in n or 'Time' in n for n in classes), classes)
    scalars = [str(n) for n in M.get_scalar_parameter_names(arc)]
    vectors = [str(n) for n in M.get_vector_parameter_names(arc)]
    check('arc runtime parameters exist', 'ArcIntensity' in scalars and 'ArcColor' in vectors,
          {'scalar': scalars, 'vector': vectors})
    spark_expressions = M.get_material_expressions(mat)
    bad_masks = [e.get_name() for e in spark_expressions
                 if isinstance(e, unreal.MaterialExpressionComponentMask) and e.get_editor_property('a')]
    check('spark graph has no invalid alpha mask on RGB', not bad_masks, bad_masks)
    report['materialValidationScope'] = 'Graph/property readback only; Vulkan shader compilation is checked by package/cook.'


def verify(report, check, tool, ref):
    verify_materials(report, check)
    E = unreal.EditorAssetLibrary
    folder = C['folder']
    mat_path = folder + '/' + C['material']
    sys_path = folder + '/' + C['system']
    system = unreal.load_asset(sys_path)
    check('system exists', isinstance(system, unreal.NiagaraSystem), sys_path)
    sysref = {'refPath': system.get_path_name()}
    summary = tool('GetSystemSummary', {'system': sysref})
    report['summary'] = summary
    ems = summary['emitters']
    check('one enabled CPU emitter', len(ems) == 1 and ems[0]['emitterName'] == C['emitter'] and ems[0]['bEnabled'] and ems[0]['simTarget'] == C['simTarget'], ems)
    check('sprite renderer only', [r['refPath'] for r in ems[0]['rendererClasses']] == ['/Script/Niagara.NiagaraSpriteRendererProperties'], ems[0]['rendererClasses'])
    users = tool('GetUserVariables', {'system': sysref})
    actual_users = {u['name']: u for u in users['userVariables']}
    check('user parameters match the C++ contract', set(actual_users) == {'User.' + n for n in C['user']}, users)
    ed = json.loads(tool('GetEmitterData', {'emitterRef': ref(C['emitter'])})['propertyValues'])
    check('emitter deterministic, local space, fixed bounds', ed['bDeterminism'] and ed['bLocalSpace'] and ed['CalculateBoundsMode'] == 'Fixed' and ed['RandomSeed'] == C['randomSeed'],
          {k: ed.get(k) for k in ('bDeterminism', 'bLocalSpace', 'CalculateBoundsMode', 'RandomSeed', 'FixedBounds')})
    fixed = ed['FixedBounds']
    check('stored fixed bounds match centimetre contract', all(
        abs(fixed[side][axis] - C['bounds'][side][index]) < .001
        for side in ('min', 'max') for index, axis in enumerate('xyz')), fixed)
    sd = json.loads(tool('GetSystemData', {'system': sysref})['propertyValues'])
    check('system has fixed bounds and casts no shadow', sd['bFixedBounds'] and not sd['bCastShadow'], {k: sd.get(k) for k in ('bFixedBounds', 'FixedBounds', 'bCastShadow')})
    topo = tool('GetEmitterTopology', {'emitterRef': ref(C['emitter'])})
    mods = lambda key: [m['moduleName'] for m in topo[key]['modules'] if m['enabled']]
    report['modules'] = {k: mods(k) for k in ('emitterUpdateScript', 'particleSpawnScript', 'particleUpdateScript')}
    check('emitter update has one burst', 'SpawnBurst_Instantaneous' in mods('emitterUpdateScript'), report['modules'])
    check('particle spawn modules', all(m in mods('particleSpawnScript') for m in EXPECT_SPAWN), report['modules'])
    up = mods('particleUpdateScript')
    check('one analytic bounce with no solver or collision module',
          up.count('KillParticles') == 1 and not any('Collision' in n or 'SolveForces' in n or n in ('GravityForce', 'Drag') for n in up), up)
    for module, name, expected in (
        ('InitializeParticle', 'Lifetime Min', C['lifetime'][0]),
        ('InitializeParticle', 'Lifetime Max', C['lifetime'][1]),
        ('ShapeLocation', 'Sphere Radius', C['spawnRadius']),
        ('AddVelocityInCone', 'Cone Angle', C['coneAngle']),
    ):
        stored = tool('GetStackInputData', {'stackInputRef': ref(C['emitter'], 'ParticleSpawnScript', module, (name,))})
        check('stored scalar ' + module + '/' + name,
              abs(stored['value']['value'] - expected) < .0001, stored)
    # Check stored assignment expressions, not merely the existence of similarly named modules.
    # Assignments have generated module names; identify each by its exact parameter input.
    script_keys = {'ParticleSpawnScript': 'particleSpawnScript', 'ParticleUpdateScript': 'particleUpdateScript'}
    order = {}
    for script, parameter, kind, expr in assignments(C):
        matches = []
        for index, module in enumerate(topo[script_keys[script]]['modules']):
            if not module['enabled']:
                continue
            module_ref = ref(C['emitter'], script, module['moduleName'])
            detail = tool('GetModuleTopology', {'moduleRef': module_ref})
            for inp in detail['inputs']:
                if inp['name'] in (parameter, parameter.split('.')[-1]):
                    matches.append((index, module['moduleName'], inp['name']))
        check('unique stored assignment ' + parameter, len(matches) == 1, matches)
        index, module, input_name = matches[0]
        actual = tool('GetStackInputData', {'stackInputRef': ref(C['emitter'], script, module, (input_name,))})
        check('stored trajectory ' + parameter, actual.get('value', {}).get('hlslExpression') == expr, actual)
        order[parameter] = index
    check('spawn state captured after velocity initialization',
          mods('particleSpawnScript').index('AddVelocityInCone') < order['Particles.ArcStart'] < order['Particles.ArcInitialVelocity'] < order['Particles.ArcHitTime'] < order['Particles.ArcReboundSpeed'], order)
    check('update position and alignment before second-contact kill',
          order['Particles.Position'] < order['Particles.Velocity'] < up.index('KillParticles'), order)
    kill = tool('GetStackInputData', {'stackInputRef': ref(C['emitter'], 'ParticleUpdateScript', 'KillParticles', ('Kill Particles',))})
    check('stored kill occurs on SECOND gravel contact', kill.get('value', {}).get('hlslExpression') == kill_expression(C), kill)
    burst = tool('GetStackInputData', {'stackInputRef': ref(C['emitter'], 'EmitterUpdateScript', 'SpawnBurst_Instantaneous', ('Spawn Count',))})
    check('burst count linked to runtime Low/High control', burst.get('value', {}).get('linkedVariable', {}).get('name') == 'User.SparkCount', burst)
    rd = {k.lower(): v for k, v in json.loads(tool('GetRendererData', {'rendererRef': ref(C['emitter'], renderer=0)})['propertyValues']).items()}
    mref = (rd.get('material') or {}).get('refPath', '')
    check('renderer uses the original spark material, velocity aligned, no shadows', mref.startswith(mat_path) and rd.get('alignment') == 'VelocityAligned' and not rd.get('bcastshadows'),
          {k: rd.get(k) for k in ('material', 'alignment', 'facingmode', 'bcastshadows')})
    deps = tool('GetSystemDependencies', {'system': sysref})
    report['dependencies'] = deps
    blob = json.dumps(deps)
    check('only Niagara engine modules and our own assets are referenced', all(p.startswith(('/Niagara/', '/Script/', folder)) for p in
          [x for x in (s.split('"')[0] for s in blob.split('"refPath": "')[1:]) if x]), deps)
    # Fresh asset load may enqueue compilation even when the package is not dirty.
    # Force PreSave/WaitForCompilationComplete before the async diagnostics wrapper
    # is called, including direct verify_fx.py and verify-only importer invocations.
    # Do not create a pending tool result/ticker callback before this wait: a commandlet
    # does not tick it to completion before shutdown. No graph is regenerated here.
    check('existing system saved after synchronous compile wait',
          E.save_loaded_asset(system, only_if_is_dirty=False), sys_path)
    # Never accept a pending, stale, unknown or failed result as a successful check.
    state = tool('GetSystemCompileState', {'system': sysref})
    report['compileState'] = state
    clean = {'UpToDate', 'UpToDateWithWarnings'}
    check('system compile completed without errors', isinstance(state, dict) and
          state.get('aggregateStatus') in clean and state.get('bIsCompiling') is False and
          state.get('bIsStale') is False and state.get('bHasErrors') is False, state)
    scripts = state.get('scripts', [])
    # UE reports qualified ENiagaraScriptUsage names; interpolated spawn is the
    # compiled spawn role of Minimal's CPU emitter. EmitterSpawn/Update are
    # inlined into the two system scripts and have no independent compile result.
    roles = []
    for script in scripts:
        role = script['scriptName'].removeprefix('ENiagaraScriptUsage::')
        if role == 'ParticleSpawnScriptInterpolated':
            role = 'ParticleSpawnScript'
        owner = script['emitterName']
        roles.append((owner, role))
    expected_roles = {
        ('None', 'SystemSpawnScript'), ('None', 'SystemUpdateScript'),
        (C['emitter'], 'ParticleSpawnScript'), (C['emitter'], 'ParticleUpdateScript'),
    }
    check('all CPU script results are present and up to date',
          set(roles) == expected_roles and len(roles) == len(expected_roles) and
          all(s['lastCompileStatus'] in clean and not s.get('errorSummary') and
              not any(e.get('severity') == 'Error' for e in s.get('compileEvents', [])) for s in scripts), scripts)


if __name__ == '__main__':
    report_path = Path(os.environ.get('S47_FX_REPORT', '/tmp/s47-fx-verify.json'))
    report = {'engine': unreal.SystemLibrary.get_engine_version(), 'checks': []}

    def save():
        report_path.write_text(json.dumps(report, indent=1, default=str) + '\n')

    def check(name, condition, detail=None):
        report['checks'].append({'name': name, 'pass': bool(condition), 'detail': detail})
        save()
        if not condition:
            raise AssertionError((name, detail))

    def tool(name, payload):
        r = unreal.ToolsetRegistry.execute_tool(TS, name, json.dumps(payload))
        if not r.get_editor_property('is_complete'):
            raise RuntimeError(name + ' did not complete synchronously')
        if r.get_editor_property('error'):
            raise RuntimeError(name + ': ' + r.get_editor_property('error'))
        v = r.get_editor_property('value')
        return json.loads(v).get('returnValue') if v else None

    obj = C['folder'] + '/' + C['system'] + '.' + C['system']

    def ref(emitter='', script='', module='', inputs=(), renderer=-1):
        return {'system': {'refPath': obj}, 'emitterName': emitter, 'scriptName': script, 'moduleName': module, 'rendererIndex': renderer, 'inputNameStack': list(inputs)}
    try:
        verify(report, check, tool, ref)
        report['status'] = 'PASS'
    except Exception:
        report['status'] = 'FAIL'
        report['error'] = traceback.format_exc()
    save()
