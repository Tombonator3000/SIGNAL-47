"""Build the original Caliche substation arc sparks from fx_contract.json.

Runs inside UnrealEditor-Cmd -run=pythonscript (see import.sh). Everything is made here:
the unlit additive spark material through MaterialEditingLibrary and the Niagara system
through UE 5.8.4's experimental NiagaraToolsets, called with ToolsetRegistry.execute_tool.
No Epic template system is copied. The emitter starts from Niagara's Minimal emitter and
uses Niagara's own modules by reference with our values. Writes only /Game/Signal47/FX.
"""
import json
import os
import traceback
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parent
C = json.loads((ROOT / 'fx_contract.json').read_text())
E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
REPORT = Path(os.environ.get('S47_FX_REPORT', '/tmp/s47-fx-import.json'))
TS = 'NiagaraToolsets.NiagaraToolset_System'
FOLDER = C['folder']
MAT_PATH = FOLDER + '/' + C['material']
SYS_PATH = FOLDER + '/' + C['system']
SYS_OBJ = SYS_PATH + '.' + C['system']
EM = C['emitter']
report = {'engine': unreal.SystemLibrary.get_engine_version(), 'contract': C, 'checks': [], 'steps': []}

TYPE = {'float': '/Script/Niagara.NiagaraFloat', 'int': '/Script/Niagara.NiagaraInt32', 'bool': '/Script/Niagara.NiagaraBool',
        'position': '/Script/Niagara.NiagaraPosition', 'vec3': '/Script/CoreUObject.Vector3f', 'vec2': '/Script/CoreUObject.Vector2f', 'color': '/Script/CoreUObject.LinearColor'}


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
    val = prop(r, 'value')
    return json.loads(val).get('returnValue') if val else None


def ref(emitter='', script='', module='', inputs=(), renderer=-1):
    return {'system': {'refPath': SYS_OBJ}, 'emitterName': emitter, 'scriptName': script, 'moduleName': module,
            'rendererIndex': renderer, 'inputNameStack': list(inputs)}


def val(kind, v):
    t = TYPE[kind]
    if kind == 'float':
        return {'struct': {'refPath': t}, 'value': {'value': float(v)}}
    if kind == 'int':
        return {'struct': {'refPath': t}, 'value': {'value': int(v)}}
    if kind == 'bool':
        return {'struct': {'refPath': t}, 'value': {'value': -1 if v else 0}}
    if kind == 'vec2':
        return {'struct': {'refPath': t}, 'value': {'x': float(v[0]), 'y': float(v[1])}}
    if kind in ('vec3', 'position'):
        return {'struct': {'refPath': t}, 'value': {'x': float(v[0]), 'y': float(v[1]), 'z': float(v[2])}}
    raise ValueError(kind)


def link(name, kind):
    return {'struct': {'refPath': '/Script/NiagaraEditor.NiagaraExt_StackInputData_Linked'},
            'value': {'linkedVariable': {'name': name, 'type': {'classStructOrEnum': {'refPath': TYPE[kind]}}}}}


def hlsl(expr):
    return {'struct': {'refPath': '/Script/NiagaraEditor.NiagaraExt_StackInputData_HlslExpression'}, 'value': {'hlslExpression': expr}}


def dynamic(asset):
    return {'struct': {'refPath': '/Script/NiagaraEditor.NiagaraExt_StackInputData_DynamicInput'},
            'value': {'dynamicInputAsset': {'refPath': asset}}}


def module_inputs(script, module):
    topo = tool('GetModuleTopology', {'moduleRef': ref(EM, script, module)})
    return {i['name']: i for i in topo['inputs']}


def pick(script, module, names):
    inputs = module_inputs(script, module)
    for n in names:
        if n in inputs:
            return n
    raise KeyError('%s/%s has none of %s; inputs: %s' % (script, module, names, sorted(inputs)))


def set_input(script, module, names, data, stack=()):
    name = pick(script, module, names) if not stack else names[0]
    out = tool('SetStackInputData', {'stackInputRef': ref(EM, script, module, (name,) + tuple(stack)), 'inputData': data})
    step('set %s/%s/%s' % (script, module, '/'.join((name,) + tuple(stack))), out)
    return name


def find_enum(node, display):
    """Searches a stack-input schema for the enum entry whose display text matches."""
    hits = []
    def walk(o):
        if isinstance(o, dict):
            texts = [v for v in o.values() if isinstance(v, str)]
            if any(t.lower() == display.lower() for t in texts):
                names = [t for t in texts if '::' in t]
                if names:
                    hits.append(names[0])
            for v in o.values():
                walk(v)
        elif isinstance(o, list):
            for v in o:
                walk(v)
    walk(node)
    return hits[0] if hits else None


def set_enum(script, module, names, display, emitter=EM):
    """Niagara's switch enums are user-defined (Name::NewEnumeratorN) and the input schema does not
    list their options, so each entry is set in turn and its display text read back."""
    name = pick(script, module, names)
    r = ref(emitter, script, module, (name,))
    current = tool('GetStackInputData', {'stackInputRef': r})
    enum_path = current['value']['enum']['refPath']
    if current['value']['displayName'] == display:
        step('enum %s/%s/%s already %s' % (script, module, name, display))
        return
    short = enum_path.split('.')[-1]
    seen = []
    for n in range(12):
        data = {'struct': {'refPath': '/Script/NiagaraEditor.NiagaraExt_StackInputData_Enum'},
                'value': {'enum': {'refPath': enum_path}, 'enumName': '%s::NewEnumerator%d' % (short, n), 'displayName': display}}
        try:
            tool('SetStackInputData', {'stackInputRef': r, 'inputData': data})
        except Exception as e:
            seen.append('%d: %s' % (n, str(e)[:80]))
            continue
        got = tool('GetStackInputData', {'stackInputRef': r})['value']
        seen.append('%d: %s' % (n, got.get('displayName')))
        if got.get('displayName') == display:
            step('enum %s/%s/%s = %s (NewEnumerator%d)' % (script, module, name, display, n))
            return
    raise KeyError('enum %s has no display %r; saw %s' % (name, display, seen))


def add_module(script, path):
    topo = tool('AddModule', {'moduleLocationRef': ref(EM, script), 'moduleAsset': {'refPath': path}})
    step('module %s in %s' % (topo['moduleName'], script))
    return topo['moduleName']


def expression(mat, cls, x, y, **values):
    node = M.create_material_expression(mat, cls, x, y)
    for key, value in values.items():
        node.set_editor_property(key, value)
    return node


def make_material():
    mat = unreal.load_asset(MAT_PATH) if E.does_asset_exist(MAT_PATH) else T.create_asset(C['material'], FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    # Rebuild in place to preserve the Niagara renderer's existing material reference.
    M.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    X = unreal.MaterialExpressionMultiply
    # Round soft mask from UV distance: no texture, nothing imported.
    uv = expression(mat, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    centre = expression(mat, unreal.MaterialExpressionConstant2Vector, -1400, 120, r=.5, g=.5)
    dist = expression(mat, unreal.MaterialExpressionDistance, -1200, 40)
    M.connect_material_expressions(uv, '', dist, 'A'); M.connect_material_expressions(centre, '', dist, 'B')
    radius = expression(mat, X, -1050, 40, const_b=2.0)
    M.connect_material_expressions(dist, '', radius, 'A')
    edge = expression(mat, unreal.MaterialExpressionOneMinus, -900, 40)
    M.connect_material_expressions(radius, '', edge, '')
    edge_s = expression(mat, unreal.MaterialExpressionSaturate, -780, 40)
    M.connect_material_expressions(edge, '', edge_s, '')
    mask = expression(mat, unreal.MaterialExpressionPower, -650, 40, const_exponent=2.0)
    M.connect_material_expressions(edge_s, '', mask, 'Base')
    # Normalized age from the Niagara sprite renderer: hot white first, then the particle colour fades.
    age = expression(mat, unreal.MaterialExpressionParticleRelativeTime, -1400, 300)
    life = expression(mat, unreal.MaterialExpressionOneMinus, -1200, 300)
    M.connect_material_expressions(age, '', life, '')
    fade = expression(mat, unreal.MaterialExpressionPower, -1050, 260, const_exponent=1.5)
    M.connect_material_expressions(life, '', fade, 'Base')
    hot = expression(mat, unreal.MaterialExpressionPower, -1050, 380, const_exponent=6.0)
    M.connect_material_expressions(life, '', hot, 'Base')
    white = expression(mat, X, -900, 380, const_b=.6)
    M.connect_material_expressions(hot, '', white, 'A')
    pc = expression(mat, unreal.MaterialExpressionParticleColor, -1400, 520)
    rgb = expression(mat, unreal.MaterialExpressionComponentMask, -1200, 520, r=True, g=True, b=True, a=False)
    M.connect_material_expressions(pc, '', rgb, '')
    col = expression(mat, X, -900, 520)
    M.connect_material_expressions(rgb, '', col, 'A'); M.connect_material_expressions(fade, '', col, 'B')
    hotcol = expression(mat, unreal.MaterialExpressionAdd, -760, 460)
    M.connect_material_expressions(col, '', hotcol, 'A'); M.connect_material_expressions(white, '', hotcol, 'B')
    masked = expression(mat, X, -600, 300)
    M.connect_material_expressions(hotcol, '', masked, 'A'); M.connect_material_expressions(mask, '', masked, 'B')
    weighted = expression(mat, X, -450, 300)
    M.connect_material_expressions(masked, '', weighted, 'A')
    # ParticleColor default output is RGB (float3). Its explicit A output is scalar.
    check('particle alpha connected through explicit A output',
          M.connect_material_expressions(pc, 'A', weighted, 'B'))
    intensity = expression(mat, unreal.MaterialExpressionScalarParameter, -600, 480, parameter_name='Intensity', default_value=float(C['materialIntensity']))
    em = expression(mat, X, -300, 300)
    M.connect_material_expressions(weighted, '', em, 'A'); M.connect_material_expressions(intensity, '', em, 'B')
    M.connect_material_property(em, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    M.set_base_material_usage(mat, unreal.MaterialUsage.MATUSAGE_NIAGARA_SPRITES, True)
    M.recompile_material(mat)
    check('material saved', E.save_loaded_asset(mat), MAT_PATH)
    return mat


def make_arc_material():
    """Own mesh material: the runtime supplies the arc's geometry and time envelope."""
    path = FOLDER + '/' + C['arcMaterial']
    mat = unreal.load_asset(path) if E.does_asset_exist(path) else T.create_asset(C['arcMaterial'], FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    M.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    color = expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0,
                       parameter_name='ArcColor', default_value=unreal.LinearColor(*C['arcColor']))
    strength = expression(mat, unreal.MaterialExpressionScalarParameter, -400, 160,
                          parameter_name='ArcIntensity', default_value=C['arcIntensity'])
    mul = expression(mat, unreal.MaterialExpressionMultiply, -180, 0)
    M.connect_material_expressions(color, '', mul, 'A')
    M.connect_material_expressions(strength, '', mul, 'B')
    M.connect_material_property(mul, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    opacity = expression(mat, unreal.MaterialExpressionConstant, -180, 160, r=1.0)
    M.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    M.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    M.recompile_material(mat)
    check('arc material saved', E.save_loaded_asset(mat), path)


def bounds(b):
    return {'min': dict(zip('xyz', b['min'])), 'max': dict(zip('xyz', b['max'])), 'isValid': True}


def make_system():
    if E.does_asset_exist(SYS_PATH):
        E.delete_asset(SYS_PATH)
    made = tool('CreateNiagaraSystem', {'assetName': C['system'], 'assetPath': FOLDER, 'templateSystem': None})
    check('system created without template', made and made.get('refPath') == SYS_OBJ, made)
    sysref = {'refPath': SYS_OBJ}
    users = []
    for name, u in C['user'].items():
        kind, d = u['type'], u['default']
        if kind == 'color':
            dv = {'struct': {'refPath': '/Script/CoreUObject.LinearColor'}, 'value': dict(zip('rgba', d))}
        elif kind == 'vec3':
            dv = {'struct': {'refPath': '/Script/CoreUObject.Vector'}, 'value': dict(zip('xyz', d))}
        else:
            dv = val(kind, d)
        users.append({'name': 'User.' + name, 'type': {'classStructOrEnum': {'refPath': TYPE[kind]}}, 'defaultValue': dv, 'description': 'S47 arc contract'})
    tool('AddUserVariables', {'system': sysref, 'variablesToAdd': users})
    step('user variables', tool('GetUserVariables', {'system': sysref}))
    tool('AddEmitter', {'system': sysref, 'templateEmitter': {'refPath': '/Niagara/DefaultAssets/Templates/Emitters/Minimal.Minimal'}, 'emitterName': EM})
    # Emitter-local coordinates avoid a world-origin/LWC offset in the analytic ground plane.
    # Runtime holds the component stationary with identity world rotation until the tail dies.
    ed = json.loads(tool('GetEmitterData', {'emitterRef': ref(EM)})['propertyValues'])
    ed.update({'SimTarget': C['simTarget'], 'bLocalSpace': True, 'bDeterminism': True, 'RandomSeed': C['randomSeed'],
               'CalculateBoundsMode': 'Fixed', 'FixedBounds': bounds(C['bounds'])})
    tool('SetEmitterData', {'emitter': ref(EM), 'emitterData': {'propertyValues': json.dumps(ed)}})
    sd = json.loads(tool('GetSystemData', {'system': sysref})['propertyValues'])
    sd.update({'bFixedBounds': True, 'FixedBounds': bounds(C['bounds']), 'bDeterminism': True, 'RandomSeed': C['randomSeed'], 'bCastShadow': False, 'bOverrideCastShadow': True})
    tool('SetSystemData', {'system': sysref, 'systemData': {'propertyValues': json.dumps(sd)}})
    step('emitter and system data')
    # One burst, then the emitter completes; C++ re-activates it for each arc.
    set_enum('EmitterUpdateScript', 'EmitterState', ['Life Cycle Mode'], 'Self')
    set_enum('EmitterUpdateScript', 'EmitterState', ['Loop Behavior'], 'Once')
    set_input('EmitterUpdateScript', 'EmitterState', ['Loop Duration'], val('float', 1.6))
    burst = add_module('EmitterUpdateScript', '/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous')
    set_input('EmitterUpdateScript', burst, ['Spawn Count'], link('User.SparkCount', 'int'))
    # Particle spawn: random life, colour from C++, thin random sprites for velocity-aligned streaks.
    init = 'InitializeParticle'
    set_enum('ParticleSpawnScript', init, ['Lifetime Mode'], 'Random')
    set_input('ParticleSpawnScript', init, ['Lifetime Min'], val('float', C['lifetime'][0]))
    set_input('ParticleSpawnScript', init, ['Lifetime Max'], val('float', C['lifetime'][1]))
    set_enum('ParticleSpawnScript', init, ['Color Mode'], 'Direct Set')
    set_input('ParticleSpawnScript', init, ['Color'], link('User.SparkColor', 'color'))
    set_enum('ParticleSpawnScript', init, ['Sprite Size Mode'], 'Random Non-Uniform')
    set_input('ParticleSpawnScript', init, ['Sprite Size Min', 'Non-Uniform Sprite Size Min'], val('vec2', C['spriteSizeMin']))
    set_input('ParticleSpawnScript', init, ['Sprite Size Max', 'Non-Uniform Sprite Size Max'], val('vec2', C['spriteSizeMax']))
    shape = add_module('ParticleSpawnScript', '/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation')
    set_input('ParticleSpawnScript', shape, ['Sphere Radius'], val('float', C['spawnRadius']))
    cone = add_module('ParticleSpawnScript', '/Niagara/Modules/Spawn/Velocity/AddVelocityInCone.AddVelocityInCone')
    set_input('ParticleSpawnScript', cone, ['Cone Axis'], link('User.Direction', 'vec3'))
    set_input('ParticleSpawnScript', cone, ['Cone Angle'], val('float', C['coneAngle']))
    vs = set_input('ParticleSpawnScript', cone, ['Velocity Strength'], dynamic('/Niagara/DynamicInputs/UniformRange/UniformRangedFloat.UniformRangedFloat'))
    chain = tool('GetModuleInputValues', {'moduleRef': ref(EM, 'ParticleSpawnScript', cone)})
    report['coneValues'] = chain
    lo = hi = None
    for cand in (('Minimum', 'Maximum'), ('Min', 'Max'), ('Minimum Value', 'Maximum Value')):
        try:
            set_input('ParticleSpawnScript', cone, [vs], val('float', C['speed'][0]), stack=(cand[0],))
            set_input('ParticleSpawnScript', cone, [vs], val('float', C['speed'][1]), stack=(cand[1],))
            lo, hi = cand
            break
        except Exception as e:
            step('speed range names %s rejected' % (cand,), str(e)[:300])
    check('spark speed is a random range', lo is not None, chain)
    # Store spawn state, then evaluate an exact ballistic trajectory with ONE gravel bounce.
    # This uses no collision queries, force solver, render-depth collision or world-position offset.
    # Separate assignment modules guarantee later expressions see the preceding output.
    from arc_trajectory import assignments, kill_expression
    for script, name, kind, expr in assignments(C):
        default = [0., 0., 0.] if kind in ('vec3', 'position') else 0.
        topo = tool('AddSetParametersModule', {
            'moduleLocationRef': ref(EM, script),
            'parameters': [{'variable': {'name': name, 'type': {'classStructOrEnum': {'refPath': TYPE[kind]}}},
                            'defaultValue': val(kind, default)}]})
        module = topo['moduleName']
        set_input(script, module, [name, name.split('.')[-1]], hlsl(expr))
        step('analytic assignment ' + name, {'script': script, 'module': module, 'expression': expr})
    kill = add_module('ParticleUpdateScript', '/Niagara/Modules/Update/Lifetime/KillParticles.KillParticles')
    set_input('ParticleUpdateScript', kill, ['Kill Particles'], hlsl(kill_expression(C)))
    # Renderer 0 of Minimal is a sprite renderer: our material, velocity aligned, no shadows.
    rr = ref(EM, renderer=0)
    rd = json.loads(tool('GetRendererData', {'rendererRef': rr})['propertyValues'])
    report['rendererKeys'] = sorted(rd)
    want = {'material': {'refPath': MAT_PATH + '.' + C['material']}, 'alignment': 'VelocityAligned', 'facingMode': 'FaceCamera',
            'sortMode': 'None', 'bCastShadows': False}
    # The data blob uses C++ property names; the schema prints them camel-cased.
    keys = {k.lower(): k for k in rd}
    for k, v in want.items():
        rd[keys.get(k.lower(), k)] = v
    tool('SetRendererData', {'renderer': rr, 'rendererData': {'propertyValues': json.dumps(rd)}})
    step('sprite renderer', {k: rd[keys.get(k.lower(), k)] for k in want})
    system = unreal.load_asset(SYS_PATH)
    check('system saved', E.save_loaded_asset(system), SYS_PATH)
    return system


def main():
    import verify_fx
    materials_only = os.environ.get('S47_FX_MATERIALS_ONLY') == '1'
    verify_only = os.environ.get('S47_FX_VERIFY_ONLY') == '1'
    check('authoring modes are mutually exclusive', not (materials_only and verify_only))
    if materials_only:
        report['mode'] = 'materials-only'
        make_material()
        make_arc_material()
        verify_fx.verify_materials(report, check)
    else:
        check('toolset registry available', unreal.ToolsetRegistry.is_available())
        check('niagara system toolset registered', unreal.ToolsetRegistry.is_toolset_registered(TS))
        report['mode'] = 'verify-only' if verify_only else 'author-and-verify'
        if not verify_only:
            make_material()
            make_arc_material()
            make_system()
        else:
            step('read existing assets without regeneration')
        verify_fx.verify(report, check, tool, ref)
    report['status'] = 'PASS'
    save()


try:
    import sys
    sys.path.insert(0, str(ROOT))
    main()
except Exception:
    report['status'] = 'FAIL'
    report['error'] = traceback.format_exc()
    save()
    unreal.log_error('S47 FX import FAIL: ' + report['error'])
    raise
