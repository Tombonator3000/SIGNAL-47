"""Caliche's photo textures into the town's materials (TEXTURES-PHOTO-20261112). Reads town-photo.json and the PNG sets that
`photo_tex.py town <dir>` wrote, imports each set as /Game/TownBuildings/Materials/Textures/T_Town_Photo<Set>_<role>, and
rebuilds the graph of every material named there around its picture (same asset, same parameter names: Color, Metallic,
Roughness, Specular, EmissiveFactor, TextureTint, NormalStrength; meshes and slots untouched).

The picture is mapped in the mesh's own space, not over the 1 m UVs: along the wall and up from the mesh's floor on upright
faces, flat on roofs and floors, with the same axes as the UVs the tangents were made from, so the normal map still fits.
band: the picture is not repeated upwards. Its dust band stays at the ground; above one picture's height the wall takes the
upper 55 % of the picture, mirrored back and forth. phase: metres up from the floor where the picture's top edge lies.
MI_Homes_* (instances, which cannot swap a texture) are given the new parent the file names and their tint.
Runs in the editor's python: python3 run_photo_tex_import.py town <dir>. Report: Saved/photo-tex-import.json."""
from pathlib import Path
import json, re
import unreal
cl = unreal.SystemLibrary.get_command_line()
SRC = Path(re.search(r'-S47PhotoLived=(\S+)', cl).group(1))
E_ = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty
TOWN = '/Game/TownBuildings/Materials'
HOMES = '/Game/CalicheHomes/Materials'
REPORT = Path(unreal.Paths.project_saved_dir()) / 'photo-tex-import.json'
report = {'status': 'RUNNING', 'set': 'town', 'textures': [], 'materials': []}
man = json.loads((SRC / 'town-photo.json').read_text())
bad = 0


def node(mat, cls, **values):
    n = M.create_material_expression(mat, cls)
    for k, v in values.items():
        n.set_editor_property(k, v)
    return n


def wire(a, out, b, inp):
    if not M.connect_material_expressions(a, out, b, inp):
        raise RuntimeError('no connection %s.%s -> %s.%s' % (a.get_name(), out, b.get_name(), inp))
    return b


def op(mat, cls, src, b=None, **values):
    """A two-input node: b a node, or a constant through const_b."""
    n = node(mat, cls, **({} if b is None or not isinstance(b, (int, float)) else {'const_b': float(b)}), **values)
    wire(src, '', n, 'A')
    if b is not None and not isinstance(b, (int, float)):
        wire(b, '', n, 'B')
    return n


def one(mat, cls, src, **values):
    return wire(src, '', node(mat, cls, **values), '')


def mask(mat, src, ch):
    return one(mat, unreal.MaterialExpressionComponentMask, src, r=ch == 'r', g=ch == 'g', b=ch == 'b', a=False)


def pick(mat, a, b, alpha):
    n = node(mat, unreal.MaterialExpressionLinearInterpolate)
    wire(a, '', n, 'A'); wire(b, '', n, 'B'); wire(alpha, '', n, 'Alpha')
    return n


def step(mat, a, b):
    """1 where a > b, else 0 (b a node or a number)."""
    return one(mat, unreal.MaterialExpressionSaturate, op(mat, unreal.MaterialExpressionMultiply, op(mat, unreal.MaterialExpressionSubtract, a, b), 4096.0))


def uv_graph(mat, row):
    su, sv = float(row['span'][0]) * 100, float(row['span'][1]) * 100
    wp = node(mat, unreal.MaterialExpressionWorldPosition)
    lp = one(mat, unreal.MaterialExpressionTransformPosition, wp, transform_source_type=unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD,
             transform_type=unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
    vn = node(mat, unreal.MaterialExpressionVertexNormalWS)
    ln = one(mat, unreal.MaterialExpressionAbs, one(mat, unreal.MaterialExpressionTransform, vn, transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD,
                                                     transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_LOCAL))
    px, py, pz = (mask(mat, lp, c) for c in 'rgb')
    nx, ny, nz = (mask(mat, ln, c) for c in 'rgb')
    # the UVs' axes: faces that look along x run u = -y (the source's +y), faces that look along y run u = x; v falls as z rises
    u_wall = pick(mat, px, op(mat, unreal.MaterialExpressionMultiply, py, -1.0), step(mat, nx, ny))
    up = op(mat, unreal.MaterialExpressionDivide, op(mat, unreal.MaterialExpressionSubtract, pz, float(row.get('phase', 0)) * 100), sv)   # pictures up from the floor
    if row.get('band'):
        over = op(mat, unreal.MaterialExpressionMax, op(mat, unreal.MaterialExpressionSubtract, up, 1.0), 0.0)
        tri = op(mat, unreal.MaterialExpressionAdd, op(mat, unreal.MaterialExpressionMultiply, over, 0.5 / 0.55), 0.5)
        tri = op(mat, unreal.MaterialExpressionSubtract, op(mat, unreal.MaterialExpressionMultiply, one(mat, unreal.MaterialExpressionFrac, tri), 2.0), 1.0)
        tri = op(mat, unreal.MaterialExpressionMultiply, one(mat, unreal.MaterialExpressionAbs, tri), 0.55)
        low = op(mat, unreal.MaterialExpressionMultiply, op(mat, unreal.MaterialExpressionSubtract, up, 1.0), -1.0)       # 1 - up
        v_wall = one(mat, unreal.MaterialExpressionClamp, pick(mat, low, tri, step(mat, up, 1.0)), min_default=0.004, max_default=0.996)
    else:
        v_wall = op(mat, unreal.MaterialExpressionMultiply, up, -1.0)
    wall = node(mat, unreal.MaterialExpressionAppendVector)
    wire(op(mat, unreal.MaterialExpressionDivide, u_wall, su), '', wall, 'A'); wire(v_wall, '', wall, 'B')
    flat = node(mat, unreal.MaterialExpressionAppendVector)
    wire(op(mat, unreal.MaterialExpressionDivide, px, su), '', flat, 'A'); wire(op(mat, unreal.MaterialExpressionDivide, py, sv), '', flat, 'B')
    return pick(mat, wall, flat, step(mat, nz, 0.7))


def texture(set_name, role, path, size):
    name = 'T_Town_Photo' + set_name.removeprefix('Town_') + '_' + role
    row = {'asset': TOWN + '/Textures/' + name, 'source': path, 'max': size, 'pass': False}
    report['textures'].append(row)
    task = unreal.AssetImportTask()
    task.filename = path; task.destination_path = TOWN + '/Textures'; task.destination_name = name
    task.replace_existing = True; task.automated = True; task.save = False
    T.import_asset_tasks([task])
    tex = E_.load_asset(row['asset'])
    if not isinstance(tex, unreal.Texture2D):
        raise RuntimeError('no texture ' + row['asset'])
    tex.set_editor_property('srgb', role == 'base_color')
    tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP if role == 'normal' else unreal.TextureCompressionSettings.TC_DEFAULT)
    tex.set_editor_property('flip_green_channel', False)
    tex.set_editor_property('max_texture_size', size)
    E_.save_loaded_asset(tex)
    row['pass'] = True
    return tex


def material(row, tex):
    path = TOWN + '/' + row['name']
    if E_.does_asset_exist(path):
        mat = E_.load_asset(path)
        if row.get('new') is None and not isinstance(mat, unreal.Material):
            raise RuntimeError(path + ' is not a material')
    elif row.get('new'):
        mat = T.create_asset(row['name'], TOWN, unreal.Material, unreal.MaterialFactoryNew())
    else:
        raise RuntimeError('no such material ' + path)
    emissive = unreal.LinearColor(0, 0, 0, 1)
    for old in list(M.get_material_expressions(mat)):
        if isinstance(old, unreal.MaterialExpressionVectorParameter) and str(old.get_editor_property('parameter_name')) == 'EmissiveFactor':
            emissive = old.get_editor_property('default_value')
        M.delete_material_expression(mat, old)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    uv = uv_graph(mat, row)
    tint = [float(x) for x in row['tint']]
    node(mat, unreal.MaterialExpressionVectorParameter, parameter_name='Color', default_value=unreal.LinearColor(*tint, 1))   # kept for instances that set it; the picture carries the colour
    for key, prop in (('metallic', MP.MP_METALLIC), ('specular', MP.MP_SPECULAR)):
        M.connect_material_property(node(mat, unreal.MaterialExpressionScalarParameter, parameter_name=key.title(), default_value=float(row[key])), '', prop)
    node(mat, unreal.MaterialExpressionScalarParameter, parameter_name='Roughness', default_value=float(row['roughness']))
    M.connect_material_property(node(mat, unreal.MaterialExpressionVectorParameter, parameter_name='EmissiveFactor', default_value=emissive), 'RGB', MP.MP_EMISSIVE_COLOR)
    samples = {}
    for role, st in (('base_color', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR), ('roughness', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR), ('normal', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)):
        samples[role] = node(mat, unreal.MaterialExpressionTextureSample, texture=tex[role], sampler_type=st)
        wire(uv, '', samples[role], 'UVs')
    mul = node(mat, unreal.MaterialExpressionMultiply)
    wire(samples['base_color'], 'RGB', mul, 'A')
    wire(node(mat, unreal.MaterialExpressionVectorParameter, parameter_name='TextureTint', default_value=unreal.LinearColor(*tint, 1)), 'RGB', mul, 'B')
    M.connect_material_property(mul, '', MP.MP_BASE_COLOR)
    M.connect_material_property(samples['roughness'], 'R', MP.MP_ROUGHNESS)
    blend = node(mat, unreal.MaterialExpressionLinearInterpolate)
    wire(node(mat, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(0, 0, 1)), '', blend, 'A')
    wire(samples['normal'], 'RGB', blend, 'B')
    wire(node(mat, unreal.MaterialExpressionScalarParameter, parameter_name='NormalStrength', default_value=1.0), '', blend, 'Alpha')
    M.connect_material_property(wire(blend, '', node(mat, unreal.MaterialExpressionNormalize), 'VectorInput'), '', MP.MP_NORMAL)
    M.recompile_material(mat)
    E_.save_loaded_asset(mat)
    return mat


try:
    textures = {}
    for t in man['textures']:
        textures[t['set']] = {role: texture(t['set'], role, f, t['size']) for role, f in t['files'].items()}
    for row in man['materials']:
        out = {'name': row['name'], 'kind': row['kind'], 'pass': False}
        report['materials'].append(out)
        try:
            if row['kind'] == 'material':
                material(row, textures[row['set']])
            else:
                mi = E_.load_asset(HOMES + '/' + row['name']); parent = E_.load_asset(TOWN + '/' + row['parent'])
                if not isinstance(mi, unreal.MaterialInstanceConstant) or not isinstance(parent, unreal.Material):
                    raise RuntimeError('instance or parent missing')
                M.set_material_instance_parent(mi, parent)
                M.set_material_instance_vector_parameter_value(mi, 'TextureTint', unreal.LinearColor(*[float(x) for x in row['tint']], 1))
                M.update_material_instance(mi)
                E_.save_loaded_asset(mi)
            out['pass'] = True
            unreal.log('S47 PHOTOTEX ' + row['name'] + ' <- ' + row.get('set', row.get('parent', '')))
        except Exception as e:
            bad += 1; out['why'] = str(e); unreal.log_error('S47 PHOTOTEX FAIL ' + row['name'] + ': ' + str(e))
    report['status'] = 'PASS' if bad == 0 else 'FAIL'
except Exception as e:
    report['status'] = 'FAIL'; report['error'] = str(e); unreal.log_error('S47 PHOTOTEX FAIL ' + str(e))
REPORT.write_text(json.dumps(report, indent=1))
unreal.log('S47 PHOTOTEX %s %d textures, %d materials, %d failed' % (report['status'], len(report['textures']), len(report['materials']), bad))
