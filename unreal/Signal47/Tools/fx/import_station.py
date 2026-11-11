"""GAS-STATION-LIFT-01: insects and dust motes under the Caliche gas station's canopy tubes.

Runs through import.sh like the others: bash Tools/fx/import.sh import_station.py
S47_FX_VERIFY_ONLY=1 reads the existing assets back without regenerating them.
Everything is authored here from Niagara's Minimal emitter and engine modules by reference; no template effect.

NS_S47_Insects, both emitters CPU, local space, deterministic, fixed bounds. The component hangs just under the tubes with
the canopy's own rotation (S47StationInsects.cpp): local x across the canopy, y along it, z up, centimetres.
  Insects  moths and gnats orbiting fast and erratically round a home under the tube line; the moths keep rising to the tube.
           The path is analytic (seeded HLSL from the particle's id and its age), so no solver, no collision, no GPU.
  Motes    a few slow, larger, very faint specks carried through the light by User.Wind.
User parameters (C++ sets them each frame the swarm is on):
  User.Count   float, insects spawned per second (0..48; about 3.8 s each, so up to about 180 at once)
  User.Motes   float, motes spawned per second (0..3; about 7.5 s each)
  User.Extent  vec3, half-size of the box under the tubes (cm; default 225 x 810 x 150: the 4.5 x 16.2 m tube field)
  User.Wind    vec3, the wind in component-local cm/s (|v| <= 120)
  User.Tint    color, rgb the light the specks catch; it is their emission (low, warm), alpha unused by C++ (1)
The material M_S47_FxInsect is a soft round speck (unlit, additive, no texture), not the spark's hot core.
The system is never deleted and remade in the same run (README: that crashed the editor on 5 November). To rebuild it,
delete NS_S47_Insects first, then run again.
"""
import json
import os
import traceback
from pathlib import Path

import unreal

E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
REPORT = Path(os.environ.get('S47_FX_REPORT', '/tmp/s47-fx-station.json'))
VERIFY_ONLY = os.environ.get('S47_FX_VERIFY_ONLY') == '1'
TS = 'NiagaraToolsets.NiagaraToolset_System'
FOLDER = '/Game/Signal47/FX'
NAME = 'NS_S47_Insects'
MATERIAL = 'M_S47_FxInsect'
HALF = 1600.0  # fixed bounds, cm each way: the tube field plus nine seconds of the strongest wind C++ sends
SEED = 4747
report = {'engine': unreal.SystemLibrary.get_engine_version(), 'checks': [], 'steps': []}
TYPE = {'float': '/Script/Niagara.NiagaraFloat', 'int': '/Script/Niagara.NiagaraInt32', 'bool': '/Script/Niagara.NiagaraBool',
        'position': '/Script/Niagara.NiagaraPosition', 'vec3': '/Script/CoreUObject.Vector3f', 'vec2': '/Script/CoreUObject.Vector2f',
        'color': '/Script/CoreUObject.LinearColor'}
USERS = [('Count', 'float', 24.), ('Motes', 'float', .8), ('Extent', 'vec3', (225., 810., 150.)),
         ('Wind', 'vec3', (0., 0., 0.)), ('Tint', 'color', (1.0, .84, .62, 1.))]
SYS = FOLDER + '/' + NAME + '.' + NAME
EM = ''


def save():
    REPORT.write_text(json.dumps(report, indent=1, default=str) + '\n')


def check(name, condition, detail=None):
    report['checks'].append({'name': name, 'pass': bool(condition), 'detail': detail})
    save()
    if not condition:
        raise AssertionError((name, detail))


def step(text, detail=None):
    report['steps'].append({'step': text, 'detail': detail})
    unreal.log('S47 FX: ' + text)
    save()


def prop(obj, name):
    try:
        return getattr(obj, name)
    except Exception:
        return obj.get_editor_property(name)


def tool(name, payload):
    r = unreal.ToolsetRegistry.execute_tool(TS, name, json.dumps(payload))
    if not prop(r, 'is_complete'):
        raise RuntimeError(name + ' did not complete synchronously')
    err = prop(r, 'error')
    if err:
        raise RuntimeError(name + ': ' + str(err)[:1500])
    v = prop(r, 'value')
    return json.loads(v).get('returnValue') if v else None


def ref(emitter='', script='', module='', inputs=(), renderer=-1):
    return {'system': {'refPath': SYS}, 'emitterName': emitter, 'scriptName': script, 'moduleName': module,
            'rendererIndex': renderer, 'inputNameStack': list(inputs)}


def val(kind, v):
    t = TYPE[kind]
    if kind in ('float', 'int'):
        return {'struct': {'refPath': t}, 'value': {'value': float(v) if kind == 'float' else int(v)}}
    if kind == 'vec2':
        return {'struct': {'refPath': t}, 'value': {'x': float(v[0]), 'y': float(v[1])}}
    if kind in ('vec3', 'position'):
        return {'struct': {'refPath': t}, 'value': {'x': float(v[0]), 'y': float(v[1]), 'z': float(v[2])}}
    if kind == 'color':
        return {'struct': {'refPath': t}, 'value': dict(zip('rgba', map(float, v)))}
    raise ValueError(kind)


def link(name, kind):
    return {'struct': {'refPath': '/Script/NiagaraEditor.NiagaraExt_StackInputData_Linked'},
            'value': {'linkedVariable': {'name': name, 'type': {'classStructOrEnum': {'refPath': TYPE[kind]}}}}}


def hlsl(expr):
    return {'struct': {'refPath': '/Script/NiagaraEditor.NiagaraExt_StackInputData_HlslExpression'}, 'value': {'hlslExpression': expr}}


def pick(script, module, names):
    topo = tool('GetModuleTopology', {'moduleRef': ref(EM, script, module)})
    inputs = {i['name'] for i in topo['inputs']}
    for n in names:
        if n in inputs:
            return n
    raise KeyError('%s/%s has none of %s; inputs: %s' % (script, module, names, sorted(inputs)))


def set_input(script, module, names, data):
    name = pick(script, module, names)
    tool('SetStackInputData', {'stackInputRef': ref(EM, script, module, (name,)), 'inputData': data})
    return name


def set_enum(script, module, names, display):
    name = pick(script, module, names)
    r = ref(EM, script, module, (name,))
    current = tool('GetStackInputData', {'stackInputRef': r})
    if current['value']['displayName'] == display:
        return
    enum_path = current['value']['enum']['refPath']
    short = enum_path.split('.')[-1]
    seen = []
    for n in range(12):
        data = {'struct': {'refPath': '/Script/NiagaraEditor.NiagaraExt_StackInputData_Enum'},
                'value': {'enum': {'refPath': enum_path}, 'enumName': '%s::NewEnumerator%d' % (short, n), 'displayName': display}}
        try:
            tool('SetStackInputData', {'stackInputRef': r, 'inputData': data})
        except Exception as e:
            seen.append(str(e)[:60])
            continue
        got = tool('GetStackInputData', {'stackInputRef': r})['value']
        seen.append(got.get('displayName'))
        if got.get('displayName') == display:
            return
    raise KeyError('enum %s has no %r; saw %s' % (name, display, seen))


def add_module(script, path):
    return tool('AddModule', {'moduleLocationRef': ref(EM, script), 'moduleAsset': {'refPath': path}})['moduleName']


# every assignment written, (emitter, script, parameter, expression), read back after the save
WRITTEN = []


def assign(script, name, kind, expr):
    default = {'vec2': [0., 0.], 'vec3': [0., 0., 0.], 'position': [0., 0., 0.], 'color': [0., 0., 0., 0.]}.get(kind, 0.)
    topo = tool('AddSetParametersModule', {
        'moduleLocationRef': ref(EM, script),
        'parameters': [{'variable': {'name': name, 'type': {'classStructOrEnum': {'refPath': TYPE[kind]}}},
                        'defaultValue': val(kind, default)}]})
    set_input(script, topo['moduleName'], [name, name.split('.')[-1]], hlsl(expr))
    WRITTEN.append((EM, script, name, expr))


def expression(mat, cls, x, y, **values):
    node = M.create_material_expression(mat, cls, x, y)
    for key, value in values.items():
        node.set_editor_property(key, value)
    return node


def make_material():
    """A soft round speck: Emissive = saturate(1 - 2|uv - .5|)^1.6 * ParticleColor.rgb * ParticleColor.a. Unlit, additive,
    no texture. Softer than the spark's core and without its built-in heat, so a pale insect catches the light without glowing."""
    path = FOLDER + '/' + MATERIAL
    mat = unreal.load_asset(path) if E.does_asset_exist(path) else T.create_asset(MATERIAL, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    M.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    uv = expression(mat, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    centre = expression(mat, unreal.MaterialExpressionConstant2Vector, -1400, 120, r=.5, g=.5)
    dist = expression(mat, unreal.MaterialExpressionDistance, -1200, 40)
    M.connect_material_expressions(uv, '', dist, 'A')
    M.connect_material_expressions(centre, '', dist, 'B')
    radius = expression(mat, unreal.MaterialExpressionMultiply, -1050, 40, const_b=2.0)
    M.connect_material_expressions(dist, '', radius, 'A')
    edge = expression(mat, unreal.MaterialExpressionOneMinus, -900, 40)
    M.connect_material_expressions(radius, '', edge, '')
    clamp = expression(mat, unreal.MaterialExpressionSaturate, -780, 40)
    M.connect_material_expressions(edge, '', clamp, '')
    shape = expression(mat, unreal.MaterialExpressionPower, -640, 40, const_exponent=1.6)
    M.connect_material_expressions(clamp, '', shape, 'Base')
    X = unreal.MaterialExpressionMultiply
    pc = expression(mat, unreal.MaterialExpressionParticleColor, -500, 200)
    rgb = expression(mat, X, -300, 100)
    M.connect_material_expressions(shape, '', rgb, 'A')
    M.connect_material_expressions(pc, '', rgb, 'B')
    em = expression(mat, X, -120, 100)
    M.connect_material_expressions(rgb, '', em, 'A')
    check(path + ' alpha connected', M.connect_material_expressions(pc, 'A', em, 'B'))
    M.connect_material_property(em, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    M.set_base_material_usage(mat, unreal.MaterialUsage.MATUSAGE_NIAGARA_SPRITES, True)
    M.recompile_material(mat)
    check(path + ' saved', E.save_loaded_asset(mat), path)
    return path + '.' + MATERIAL


def bounds(half):
    return {'min': dict(zip('xyz', (-half, -half, -half))), 'max': dict(zip('xyz', (half, half, half))), 'isValid': True}


def add_emitter(sysref, name):
    global EM
    EM = name
    tool('AddEmitter', {'system': sysref, 'templateEmitter': {'refPath': '/Niagara/DefaultAssets/Templates/Emitters/Minimal.Minimal'}, 'emitterName': name})
    ed = json.loads(tool('GetEmitterData', {'emitterRef': ref(name)})['propertyValues'])
    # Persistent ids: each particle's seed is its id, so the swarm is the same with the same spawn history.
    ed.update({'SimTarget': 'CPUSim', 'bLocalSpace': True, 'bDeterminism': True, 'RandomSeed': SEED,
               'CalculateBoundsMode': 'Fixed', 'FixedBounds': bounds(HALF), 'bRequiresPersistentIDs': True})
    tool('SetEmitterData', {'emitter': ref(name), 'emitterData': {'propertyValues': json.dumps(ed)}})


def renderer_set(index, want):
    r = ref(EM, renderer=index)
    rd = json.loads(tool('GetRendererData', {'rendererRef': r})['propertyValues'])
    report.setdefault('rendererKeys', {})[EM] = sorted(rd)
    keys = {k.lower(): k for k in rd}
    for k, v in want.items():
        rd[keys.get(k.lower(), k)] = v
    tool('SetRendererData', {'renderer': r, 'rendererData': {'propertyValues': json.dumps(rd)}})


def looping(rate_user, life):
    """Spawned at a rate C++ sets (0: none, and the ones out die off within their lifetime), forever."""
    set_enum('EmitterUpdateScript', 'EmitterState', ['Life Cycle Mode'], 'Self')
    set_enum('EmitterUpdateScript', 'EmitterState', ['Loop Behavior'], 'Infinite')
    rate = add_module('EmitterUpdateScript', '/Niagara/Modules/Emitter/SpawnRate.SpawnRate')
    set_input('EmitterUpdateScript', rate, ['SpawnRate'], link(rate_user, 'float'))
    set_enum('ParticleSpawnScript', 'InitializeParticle', ['Lifetime Mode'], 'Random')
    set_input('ParticleSpawnScript', 'InitializeParticle', ['Lifetime Min'], val('float', life[0]))
    set_input('ParticleSpawnScript', 'InitializeParticle', ['Lifetime Max'], val('float', life[1]))


def seeds(prefix):
    """Three numbers in [0, 1) from the particle's id (modulo, so a long night keeps float precision)."""
    n = '(float(Particles.UniqueID) - 4096.0 * floor(float(Particles.UniqueID) / 4096.0))'
    assign('ParticleSpawnScript', 'Particles.%sSeed' % prefix, 'vec3',
           'frac(sin(float3(%s * 12.9898 + 1.7, %s * 78.233 + 4.1, %s * 37.719 + 9.3)) * 43758.5453)' % (n, n, n))


# soft in and out over the first and last sixth of a life: nothing pops on or off
FADE = 'saturate(min(Particles.NormalizedAge * 6.0, (1.0 - Particles.NormalizedAge) * 6.0))'


def make_insects():
    add_emitter({'refPath': SYS}, 'Insects')
    looping('User.Count', (2.4, 5.2))
    seeds('Insect')
    s = 'Particles.InsectSeed'
    # about one in four is a moth: larger, slower, wider loops, and it keeps going up to the tube
    assign('ParticleSpawnScript', 'Particles.InsectKind', 'float', 'step(0.72, frac(%s.x * 7.31 + %s.z * 3.17))' % (s, s))
    assign('ParticleSpawnScript', 'Particles.InsectHome', 'vec3',
           'float3((%s.x * 2.0 - 1.0) * User.Extent.x, (%s.y * 2.0 - 1.0) * User.Extent.y, -(0.08 + 0.92 * %s.z * %s.z) * User.Extent.z)' % (s, s, s, s))
    assign('ParticleSpawnScript', 'Particles.InsectW', 'float', 'lerp(7.5, 3.4, Particles.InsectKind) * (0.8 + 0.4 * %s.y)' % s)
    assign('ParticleSpawnScript', 'Particles.InsectR', 'float', 'lerp(12.0, 34.0, Particles.InsectKind) * (0.7 + 0.6 * %s.z)' % s)
    assign('ParticleSpawnScript', 'Particles.Position', 'position', 'Particles.InsectHome')
    a = 'Particles.Age'
    w = 'Particles.InsectW'
    p = '(%s * 6.2832)' % s
    orbit = ('float3(sin({a} * {w} + {p}.x) + 0.45 * sin({a} * {w} * 2.7 + {p}.y), '
             'cos({a} * {w} * 0.9 + {p}.y) + 0.45 * sin({a} * {w} * 3.1 + {p}.z), '
             '0.55 * sin({a} * {w} * 1.3 + {p}.z) + 0.25 * sin({a} * {w} * 4.3 + {p}.x)) * Particles.InsectR').format(a=a, w=w, p=p)
    # a moth's home climbs to the tube line and drops back, about every six seconds; a gnat stays in its cloud
    home = ('float3(Particles.InsectHome.x, Particles.InsectHome.y, Particles.InsectHome.z * '
            '(1.0 - Particles.InsectKind * (0.5 + 0.5 * sin({a} * 1.1 + {p}.z))))').format(a=a, p=p)
    assign('ParticleUpdateScript', 'Particles.InsectAt', 'vec3', '%s + %s + User.Wind * %s * 0.3' % (home, orbit, a))
    # never up into the canopy's ceiling
    assign('ParticleUpdateScript', 'Particles.Position', 'position',
           'float3(Particles.InsectAt.x, Particles.InsectAt.y, min(Particles.InsectAt.z, -3.0))')
    assign('ParticleUpdateScript', 'Particles.SpriteSize', 'vec2',
           'float2(1.0, 1.0) * lerp(1.4, 3.6, Particles.InsectKind) * (0.8 + 0.4 * %s.x)' % s)
    # gnats fainter; a moth's wings turn in the light a little (15 %, about three times a second: a shimmer, not a blink)
    assign('ParticleUpdateScript', 'Particles.Color', 'color',
           'User.Tint * float4(1.0, 1.0, 1.0, %s * lerp(0.55, 0.86 + 0.14 * sin(%s * 17.0 + %s.x), Particles.InsectKind))' % (FADE, a, p))
    renderer_set(0, {'material': {'refPath': MAT}, 'sortMode': 'None', 'bCastShadows': False})


def make_motes():
    add_emitter({'refPath': SYS}, 'Motes')
    looping('User.Motes', (6.0, 9.0))
    seeds('Mote')
    s = 'Particles.MoteSeed'
    # anywhere in the lit air, from the tubes down to well under them, set back upwind so they cross the light as they live
    assign('ParticleSpawnScript', 'Particles.MoteHome', 'vec3',
           'float3((%s.x * 2.0 - 1.0) * User.Extent.x, (%s.y * 2.0 - 1.0) * User.Extent.y, -(0.15 + 1.45 * %s.z) * User.Extent.z) - User.Wind * 3.5' % (s, s, s))
    assign('ParticleSpawnScript', 'Particles.Position', 'position', 'Particles.MoteHome')
    a = 'Particles.Age'
    p = '(%s * 6.2832)' % s
    assign('ParticleUpdateScript', 'Particles.Position', 'position',
           'Particles.MoteHome + User.Wind * {a} + float3(sin({a} * 0.7 + {p}.x), cos({a} * 0.6 + {p}.y), 0.6 * sin({a} * 0.5 + {p}.z)) * 9.0'.format(a=a, p=p))
    assign('ParticleUpdateScript', 'Particles.SpriteSize', 'vec2', 'float2(1.0, 1.0) * (5.0 + 4.0 * %s.x)' % s)
    assign('ParticleUpdateScript', 'Particles.Color', 'color', 'User.Tint * float4(1.0, 1.0, 1.0, 0.1 * %s)' % FADE)
    renderer_set(0, {'material': {'refPath': MAT}, 'sortMode': 'None', 'bCastShadows': False})


def finish_system(path, sysref):
    sd = json.loads(tool('GetSystemData', {'system': sysref})['propertyValues'])
    sd.update({'bFixedBounds': True, 'FixedBounds': bounds(HALF), 'bDeterminism': True, 'RandomSeed': SEED, 'bCastShadow': False, 'bOverrideCastShadow': True})
    tool('SetSystemData', {'system': sysref, 'systemData': {'propertyValues': json.dumps(sd)}})


def verify(path, sysref):
    """Read back what the C++ side relies on. A save first: UNiagaraSystem::PreSave waits for the compile (README)."""
    global EM
    system = unreal.load_asset(path)
    check(path + ' is a Niagara system', isinstance(system, unreal.NiagaraSystem), path)
    check(path + ' saved', E.save_loaded_asset(system, only_if_is_dirty=False), path)
    state = tool('GetSystemCompileState', {'system': sysref})
    report['compile'] = state
    check(path + ' compiled without errors', isinstance(state, dict) and state.get('aggregateStatus') == 'UpToDate' and not state.get('bHasErrors') and not state.get('bIsCompiling'), state)
    summary = tool('GetSystemSummary', {'system': sysref})
    report['summary'] = summary
    ems = {e['emitterName']: e for e in summary['emitters']}
    check('two enabled CPU emitters, Insects and Motes', sorted(ems) == ['Insects', 'Motes'] and all(e['bEnabled'] and e['simTarget'] == 'CPUSim' for e in ems.values()), summary['emitters'])
    for e in ems.values():
        check(e['emitterName'] + ' sprite renderer only', [r['refPath'] for r in e['rendererClasses']] == ['/Script/Niagara.NiagaraSpriteRendererProperties'], e['rendererClasses'])
    users = tool('GetUserVariables', {'system': sysref})
    names = {u['name'] for u in users['userVariables']}
    check('user parameters match the C++ contract', names == {'User.' + n for n, _, _ in USERS}, users)
    for name in ems:
        EM = name
        ed = json.loads(tool('GetEmitterData', {'emitterRef': ref(name)})['propertyValues'])
        check(name + ' deterministic, local space, fixed bounds, persistent ids',
              ed['bDeterminism'] and ed['bLocalSpace'] and ed['CalculateBoundsMode'] == 'Fixed' and ed['RandomSeed'] == SEED and ed['bRequiresPersistentIDs'],
              {k: ed.get(k) for k in ('bDeterminism', 'bLocalSpace', 'CalculateBoundsMode', 'RandomSeed', 'bRequiresPersistentIDs', 'FixedBounds')})
        rate = tool('GetStackInputData', {'stackInputRef': ref(name, 'EmitterUpdateScript', 'SpawnRate', ('SpawnRate',))})
        want = 'User.Count' if name == 'Insects' else 'User.Motes'
        check(name + ' spawn rate linked to ' + want, rate.get('value', {}).get('linkedVariable', {}).get('name') == want, rate)
        rd = {k.lower(): v for k, v in json.loads(tool('GetRendererData', {'rendererRef': ref(name, renderer=0)})['propertyValues']).items()}
        mref = (rd.get('material') or {}).get('refPath', '')
        check(name + ' renderer uses ' + MATERIAL + ', no shadows', mref.startswith(FOLDER + '/' + MATERIAL) and not rd.get('bcastshadows'), {k: rd.get(k) for k in ('material', 'bcastshadows', 'sortmode')})
        topo = tool('GetEmitterTopology', {'emitterRef': ref(name)})
        up = [m['moduleName'] for m in topo['particleUpdateScript']['modules'] if m['enabled']]
        check(name + ' has no solver, collision or force module', not any('Collision' in n or 'SolveForces' in n or 'Force' in n or n == 'Drag' for n in up), up)
    sd = json.loads(tool('GetSystemData', {'system': sysref})['propertyValues'])
    check('system has fixed bounds and casts no shadow', sd['bFixedBounds'] and not sd['bCastShadow'], {k: sd.get(k) for k in ('bFixedBounds', 'FixedBounds', 'bCastShadow')})
    # the expressions as stored, when this run wrote them
    for em, script, name, expr in WRITTEN:
        EM = em
        topo = tool('GetEmitterTopology', {'emitterRef': ref(em)})
        key = {'ParticleSpawnScript': 'particleSpawnScript', 'ParticleUpdateScript': 'particleUpdateScript'}[script]
        found = []
        for module in topo[key]['modules']:
            detail = tool('GetModuleTopology', {'moduleRef': ref(em, script, module['moduleName'])})
            for inp in detail['inputs']:
                if inp['name'] in (name, name.split('.')[-1]):
                    found.append(tool('GetStackInputData', {'stackInputRef': ref(em, script, module['moduleName'], (inp['name'],))}).get('value', {}).get('hlslExpression'))
        check('%s %s %s stored' % (em, script, name), expr in found, found)


MAT = FOLDER + '/' + MATERIAL + '.' + MATERIAL


def main():
    check('toolset registry available', unreal.ToolsetRegistry.is_available())
    check('niagara system toolset registered', unreal.ToolsetRegistry.is_toolset_registered(TS))
    path = FOLDER + '/' + NAME
    sysref = {'refPath': SYS}
    exists = E.does_asset_exist(path)
    if VERIFY_ONLY:
        check(path + ' exists for a verify-only run', exists, path)
        check(MATERIAL + ' exists', E.does_asset_exist(FOLDER + '/' + MATERIAL))
    else:
        # Delete-and-recreate in one run crashed the editor (README); a rebuild deletes the system first, in its own run.
        check(path + ' absent (delete it first to rebuild)', not exists, path)
        make_material()
        made = tool('CreateNiagaraSystem', {'assetName': NAME, 'assetPath': FOLDER, 'templateSystem': None})
        check(NAME + ' created', made and made.get('refPath') == SYS, made)
        tool('AddUserVariables', {'system': sysref, 'variablesToAdd': [
            {'name': 'User.' + n, 'type': {'classStructOrEnum': {'refPath': TYPE[k]}}, 'defaultValue': val(k, d), 'description': 'S47 station insects'}
            for n, k, d in USERS]})
        make_insects()
        make_motes()
        finish_system(path, sysref)
        step('system built', len(WRITTEN))
    verify(path, sysref)
    report['status'] = 'PASS'
    save()


try:
    main()
except Exception:
    report['status'] = 'FAIL'
    report['error'] = traceback.format_exc()
    save()
    unreal.log_error('S47 FX station FAIL: ' + report['error'])
    raise
