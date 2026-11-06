"""Original Niagara lightning bolt and light orb for the substation and the SARO orbs.

Runs through import.sh like import_fx.py: bash Tools/fx/import.sh import_night.py
Everything is authored here from Niagara's Minimal emitter and engine modules by reference.

NS_S47_Bolt: one jagged ribbon from User.Start to User.End in component-local centimetres.
  C++ moves the ends and advances User.Phase, so the arc pauses with the game and restrikes
  at User.Rate per second. No randomness outside the seeded expressions.
NS_S47_Orb: hot core and wide halo in local space, plus drifting motes left in world space.
"""
import json
import math
import os
import traceback
from pathlib import Path

import unreal

E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
REPORT = Path(os.environ.get('S47_FX_REPORT', '/tmp/s47-fx-night.json'))
TS = 'NiagaraToolsets.NiagaraToolset_System'
FOLDER = '/Game/Signal47/FX'
report = {'engine': unreal.SystemLibrary.get_engine_version(), 'checks': [], 'steps': []}
TYPE = {'float': '/Script/Niagara.NiagaraFloat', 'int': '/Script/Niagara.NiagaraInt32', 'bool': '/Script/Niagara.NiagaraBool',
        'position': '/Script/Niagara.NiagaraPosition', 'vec3': '/Script/CoreUObject.Vector3f', 'vec2': '/Script/CoreUObject.Vector2f',
        'color': '/Script/CoreUObject.LinearColor'}
BOLT_LINKS = 28
SYS = ''
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


def assign(script, name, kind, expr):
    default = {'vec2': [0., 0.], 'vec3': [0., 0., 0.], 'position': [0., 0., 0.], 'color': [0., 0., 0., 0.]}.get(kind, 0.)
    topo = tool('AddSetParametersModule', {
        'moduleLocationRef': ref(EM, script),
        'parameters': [{'variable': {'name': name, 'type': {'classStructOrEnum': {'refPath': TYPE[kind]}}},
                        'defaultValue': val(kind, default)}]})
    set_input(script, topo['moduleName'], [name, name.split('.')[-1]], hlsl(expr))


def expression(mat, cls, x, y, **values):
    node = M.create_material_expression(mat, cls, x, y)
    for key, value in values.items():
        node.set_editor_property(key, value)
    return node


def base_material(name):
    path = FOLDER + '/' + name
    mat = unreal.load_asset(path) if E.does_asset_exist(path) else T.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    M.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    return mat, path


def finish_material(mat, path, shape, usages):
    """Emissive = shape * ParticleColor.rgb * ParticleColor.a. HDR particle colour drives the bloom."""
    X = unreal.MaterialExpressionMultiply
    pc = expression(mat, unreal.MaterialExpressionParticleColor, -500, 200)
    rgb = expression(mat, X, -300, 100)
    M.connect_material_expressions(shape, '', rgb, 'A')
    M.connect_material_expressions(pc, '', rgb, 'B')
    em = expression(mat, X, -120, 100)
    M.connect_material_expressions(rgb, '', em, 'A')
    check(path + ' alpha connected', M.connect_material_expressions(pc, 'A', em, 'B'))
    M.connect_material_property(em, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    for usage in usages:
        M.set_base_material_usage(mat, usage, True)
    M.recompile_material(mat)
    check(path + ' saved', E.save_loaded_asset(mat), path)


def make_bolt_material():
    """Across the ribbon: a thin white-hot core inside a wide soft sheath. No texture."""
    mat, path = base_material('M_S47_FxBolt')
    uv = expression(mat, unreal.MaterialExpressionTextureCoordinate, -1500, 0)
    v = expression(mat, unreal.MaterialExpressionComponentMask, -1350, 0, r=False, g=True, b=False, a=False)
    M.connect_material_expressions(uv, '', v, '')
    twice = expression(mat, unreal.MaterialExpressionMultiply, -1200, 0, const_b=2.0)
    M.connect_material_expressions(v, '', twice, 'A')
    centred = expression(mat, unreal.MaterialExpressionSubtract, -1050, 0, const_b=1.0)
    M.connect_material_expressions(twice, '', centred, 'A')
    dist = expression(mat, unreal.MaterialExpressionAbs, -900, 0)
    M.connect_material_expressions(centred, '', dist, '')
    edge = expression(mat, unreal.MaterialExpressionOneMinus, -780, 0)
    M.connect_material_expressions(dist, '', edge, '')
    core = expression(mat, unreal.MaterialExpressionPower, -640, -80, const_exponent=26.0)
    M.connect_material_expressions(edge, '', core, 'Base')
    sheath = expression(mat, unreal.MaterialExpressionPower, -640, 80, const_exponent=2.4)
    M.connect_material_expressions(edge, '', sheath, 'Base')
    soft = expression(mat, unreal.MaterialExpressionMultiply, -500, 80, const_b=.10)
    M.connect_material_expressions(sheath, '', soft, 'A')
    shape = expression(mat, unreal.MaterialExpressionAdd, -420, 0)
    M.connect_material_expressions(core, '', shape, 'A')
    M.connect_material_expressions(soft, '', shape, 'B')
    finish_material(mat, path, shape, [unreal.MaterialUsage.MATUSAGE_NIAGARA_RIBBONS])
    return path + '.M_S47_FxBolt'


def make_orb_material():
    """Radial falloff with a long tail, so a bright sprite reads as light in the air, not as a disc."""
    mat, path = base_material('M_S47_FxOrb')
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
    shape = expression(mat, unreal.MaterialExpressionPower, -640, 40, const_exponent=3.0)
    M.connect_material_expressions(clamp, '', shape, 'Base')
    finish_material(mat, path, shape, [unreal.MaterialUsage.MATUSAGE_NIAGARA_SPRITES])
    return path + '.M_S47_FxOrb'


def make_system(name, users):
    global SYS
    path = FOLDER + '/' + name
    SYS = path + '.' + name
    if E.does_asset_exist(path):
        E.delete_asset(path)
    made = tool('CreateNiagaraSystem', {'assetName': name, 'assetPath': FOLDER, 'templateSystem': None})
    check(name + ' created', made and made.get('refPath') == SYS, made)
    sysref = {'refPath': SYS}
    tool('AddUserVariables', {'system': sysref, 'variablesToAdd': [
        {'name': 'User.' + n, 'type': {'classStructOrEnum': {'refPath': TYPE[k]}}, 'defaultValue': val(k, d), 'description': 'S47 night fx'}
        for n, k, d in users]})
    return path, sysref


def bounds(half):
    return {'min': dict(zip('xyz', (-half, -half, -half))), 'max': dict(zip('xyz', (half, half, half))), 'isValid': True}


def add_emitter(sysref, name, half, local=True, persistent=False):
    global EM
    EM = name
    tool('AddEmitter', {'system': sysref, 'templateEmitter': {'refPath': '/Niagara/DefaultAssets/Templates/Emitters/Minimal.Minimal'}, 'emitterName': name})
    ed = json.loads(tool('GetEmitterData', {'emitterRef': ref(name)})['propertyValues'])
    ed.update({'SimTarget': 'CPUSim', 'bLocalSpace': local, 'bDeterminism': True, 'RandomSeed': 4747,
               'CalculateBoundsMode': 'Fixed', 'FixedBounds': bounds(half), 'bRequiresPersistentIDs': persistent})
    tool('SetEmitterData', {'emitter': ref(name), 'emitterData': {'propertyValues': json.dumps(ed)}})


def finish_system(path, sysref, half):
    sd = json.loads(tool('GetSystemData', {'system': sysref})['propertyValues'])
    sd.update({'bFixedBounds': True, 'FixedBounds': bounds(half), 'bDeterminism': True, 'RandomSeed': 4747, 'bCastShadow': False, 'bOverrideCastShadow': True})
    tool('SetSystemData', {'system': sysref, 'systemData': {'propertyValues': json.dumps(sd)}})
    system = unreal.load_asset(path)
    check(path + ' saved', E.save_loaded_asset(system, only_if_is_dirty=False), path)
    state = tool('GetSystemCompileState', {'system': sysref})
    report.setdefault('compile', {})[path] = state
    save()
    check(path + ' compiled without errors', isinstance(state, dict) and state.get('aggregateStatus') == 'UpToDate' and not state.get('bHasErrors') and not state.get('bIsCompiling'), state)


def renderer_set(index, want):
    r = ref(EM, renderer=index)
    rd = json.loads(tool('GetRendererData', {'rendererRef': r})['propertyValues'])
    report.setdefault('rendererKeys', {})[EM] = sorted(rd)
    keys = {k.lower(): k for k in rd}
    for k, v in want.items():
        rd[keys.get(k.lower(), k)] = v
    tool('SetRendererData', {'renderer': r, 'rendererData': {'propertyValues': json.dumps(rd)}})


def forever(script='EmitterUpdateScript'):
    set_enum(script, 'EmitterState', ['Life Cycle Mode'], 'Self')
    set_enum(script, 'EmitterState', ['Loop Behavior'], 'Once')
    set_input(script, 'EmitterState', ['Loop Duration'], val('float', 100000.))


def make_bolt(material):
    path, sysref = make_system('NS_S47_Bolt', [
        ('Start', 'vec3', (0, 0, 0)), ('End', 'vec3', (0, 0, 200)), ('Jitter', 'float', 24.), ('Width', 'float', 22.),
        ('Phase', 'float', 0.), ('Rate', 'float', 18.), ('Seed', 'float', 1.), ('BoltColor', 'color', (.36, .64, 1., 1.))])
    add_emitter(sysref, 'Bolt', 2500, persistent=True)
    forever()
    burst = add_module('EmitterUpdateScript', '/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous')
    set_input('EmitterUpdateScript', burst, ['Spawn Count'], val('int', BOLT_LINKS))
    set_enum('ParticleSpawnScript', 'InitializeParticle', ['Lifetime Mode'], 'Direct Set')
    set_input('ParticleSpawnScript', 'InitializeParticle', ['Lifetime'], val('float', 100000.))
    # Modulo: a re-activated component keeps counting IDs, and the ribbon must stay in order.
    u = '((float(Particles.UniqueID) - %.1f * floor(float(Particles.UniqueID) / %.1f)) / %.1f)' % (BOLT_LINKS, BOLT_LINKS, BOLT_LINKS - 1)
    assign('ParticleSpawnScript', 'Particles.BoltU', 'float', u)
    assign('ParticleSpawnScript', 'Particles.RibbonLinkOrder', 'float', 'Particles.BoltU')
    # Every restrike (T) draws a new path; ends stay pinned, the middle wanders most.
    tick = 'floor(User.Phase * User.Rate)'
    assign('ParticleUpdateScript', 'Particles.BoltT', 'float', tick)
    assign('ParticleUpdateScript', 'Particles.BoltA', 'vec3', 'normalize(cross(normalize(User.End - User.Start), float3(0.37, 0.59, 0.71)))')
    assign('ParticleUpdateScript', 'Particles.BoltB', 'vec3', 'cross(normalize(User.End - User.Start), Particles.BoltA)')
    u = 'Particles.BoltU'
    t = 'Particles.BoltT'
    hash1 = 'frac(sin(%s * 91.7 + %s * 12.9898 + User.Seed * 37.7) * 43758.5453) - 0.5' % (u, t)
    hash2 = 'frac(sin(%s * 47.3 + %s * 78.233 + User.Seed * 11.1) * 24634.6345) - 0.5' % (u, t)
    wander1 = 'sin(%s * 9.42 + %s * 1.7 + User.Seed)' % (u, t)
    wander2 = 'sin(%s * 22.0 + %s * 2.9 + User.Seed * 1.3)' % (u, t)
    assign('ParticleUpdateScript', 'Particles.BoltEnv', 'float', 'pow(sin(%s * 3.14159265), 0.6)' % u)
    assign('ParticleUpdateScript', 'Particles.Position', 'position',
           'lerp(User.Start, User.End, %s) + (Particles.BoltA * (%s * 0.8 + %s * 0.3 + (%s) * 0.42) + Particles.BoltB * (%s * 0.35 + (%s) * 0.42)) * User.Jitter * Particles.BoltEnv'
           % (u, wander1, wander2, hash1, wander2, hash2))
    assign('ParticleUpdateScript', 'Particles.RibbonWidth', 'float', 'User.Width * (0.55 + 0.45 * Particles.BoltEnv)')
    assign('ParticleUpdateScript', 'Particles.Color', 'color', 'User.BoltColor')
    tool('RemoveRenderer', {'rendererToRemove': ref(EM, renderer=0)})
    added = tool('AddRenderer', {'newRendererLocation': ref(EM), 'rendererClass': '/Script/Niagara.NiagaraRibbonRendererProperties'})
    step('ribbon renderer', added)
    renderer_set(0, {'material': {'refPath': material}, 'TessellationMode': 'Disabled'})
    finish_system(path, sysref, 2500)


def make_orb(material):
    path, sysref = make_system('NS_S47_Orb', [
        ('OrbColor', 'color', (1.0, .78, .46, 1.)), ('Size', 'float', 90.), ('Brightness', 'float', 1.), ('Phase', 'float', 0.)])
    # The core and halo sit on the component; only their pulse is simulated.
    pulse = '(0.86 + 0.14 * sin(User.Phase * 3.1) * sin(User.Phase * 1.3 + 0.7))'
    for name, scale, hot in (('Core', 1.0, 26.), ('Halo', 6.5, .55)):
        add_emitter(sysref, name, 1200)
        forever()
        burst = add_module('EmitterUpdateScript', '/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous')
        set_input('EmitterUpdateScript', burst, ['Spawn Count'], val('int', 1))
        set_enum('ParticleSpawnScript', 'InitializeParticle', ['Lifetime Mode'], 'Direct Set')
        set_input('ParticleSpawnScript', 'InitializeParticle', ['Lifetime'], val('float', 100000.))
        assign('ParticleUpdateScript', 'Particles.Position', 'position', 'float3(0.0, 0.0, 0.0)')
        assign('ParticleUpdateScript', 'Particles.SpriteSize', 'vec2', 'float2(1.0, 1.0) * User.Size * %.2f * %s' % (scale, pulse))
        assign('ParticleUpdateScript', 'Particles.Color', 'color', 'User.OrbColor * float4(%.2f, %.2f, %.2f, 1.0) * User.Brightness * %s' % (hot, hot, hot, pulse))
        renderer_set(0, {'material': {'refPath': material}, 'sortMode': 'None', 'bCastShadows': False})
    # Motes stay behind in the world, so a moving orb leaves a faint wake.
    add_emitter(sysref, 'Motes', 6000, local=False)
    set_enum('EmitterUpdateScript', 'EmitterState', ['Life Cycle Mode'], 'Self')
    set_enum('EmitterUpdateScript', 'EmitterState', ['Loop Behavior'], 'Infinite')
    rate = add_module('EmitterUpdateScript', '/Niagara/Modules/Emitter/SpawnRate.SpawnRate')
    set_input('EmitterUpdateScript', rate, ['SpawnRate'], val('float', 16.))
    set_enum('ParticleSpawnScript', 'InitializeParticle', ['Lifetime Mode'], 'Direct Set')
    set_input('ParticleSpawnScript', 'InitializeParticle', ['Lifetime'], val('float', 1.8))
    shape = add_module('ParticleSpawnScript', '/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation')
    set_input('ParticleSpawnScript', shape, ['Sphere Radius'], val('float', 42.))
    assign('ParticleSpawnScript', 'Particles.Velocity', 'vec3', '(frac(sin(float3(Particles.Position.x, Particles.Position.y, Particles.Position.z) * 0.37) * 4375.85) - 0.5) * 30.0')
    add_module('ParticleUpdateScript', '/Niagara/Modules/Solvers/SolveForcesAndVelocity.SolveForcesAndVelocity')
    assign('ParticleUpdateScript', 'Particles.SpriteSize', 'vec2', 'float2(1.0, 1.0) * User.Size * 0.14 * (1.0 - Particles.NormalizedAge)')
    assign('ParticleUpdateScript', 'Particles.Color', 'color', 'User.OrbColor * float4(9.0, 9.0, 9.0, 1.0) * User.Brightness * (1.0 - Particles.NormalizedAge)')
    renderer_set(0, {'material': {'refPath': material}, 'sortMode': 'None', 'bCastShadows': False})
    finish_system(path, sysref, 6000)


def main():
    check('toolset registry available', unreal.ToolsetRegistry.is_available())
    check('niagara system toolset registered', unreal.ToolsetRegistry.is_toolset_registered(TS))
    make_bolt(make_bolt_material())
    make_orb(make_orb_material())
    report['status'] = 'PASS'
    save()


try:
    main()
except Exception:
    report['status'] = 'FAIL'
    report['error'] = traceback.format_exc()
    save()
    unreal.log_error('S47 FX night FAIL: ' + report['error'])
    raise
