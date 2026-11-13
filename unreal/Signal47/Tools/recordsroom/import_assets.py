"""RECORDS-ROOM-LIFT-20261113: brings the records room's textures, materials and models into /Game/RecordsRoom.

Sources (outside Git, made again by the scripts beside this one):
  S47_REC_SRC/tex            make_surfaces.py's and make_labels.py's PNGs (default ~/scratch-bm/records-room-20261113)
  S47_REC_WORK/Export, /textures   build_records.py's FBX files and baked atlases (default S47_REC_SRC/work)
Run through the queue with no editor open:  python3 Tools/recordsroom/run.py import [what ...]
what: surfaces, labels, or a model key of MODELS below (none: everything whose source is there).
Uses the project's measured FBX/PBR recipe (Tools/fieldintegration/import_recipe.py): Blender (x, y, z) m -> Unreal (x, -y, z) cm.

M_Rec_Block and M_Rec_Floor are laid on by world position (the web export's wall and floor meshes keep their geometry and UVs):
40 x 20 cm blocks from a 1.6 m tile, 30 cm tiles from a 1.2 m tile, and one macro map each for the room's own wear
(make_surfaces.py's header has the mapping). Their normals are built in world space, so `tangent_space_normal` is off.
"""
import json
import os
import sys
import traceback
from pathlib import Path

import unreal

HERE = Path(__file__).resolve().parent
SRC = Path(os.path.expanduser(os.environ.get('S47_REC_SRC', '~/scratch-bm/records-room-20261113'))).resolve()
WORK = Path(os.path.expanduser(os.environ.get('S47_REC_WORK', str(SRC / 'work')))).resolve()
TEX = SRC / 'tex'
DEST = '/Game/RecordsRoom'
WHAT = [w for w in os.environ.get('S47_REC_WHAT', '').split(',') if w]
sys.path.insert(0, str(HERE.parent / 'fieldintegration'))
import import_recipe as R

R.REPORT = WORK / 'import-report.json'
R.report = {'status': 'RUNNING', 'engine': unreal.SystemLibrary.get_engine_version(), 'source': str(SRC), 'destination': DEST, 'checks': [], 'meshes': [], 'textures': []}
E, S, M, T = R.E, R.S, R.M, R.T
ST = unreal.MaterialSamplerType
OUT = unreal.CustomMaterialOutputType


def texture(name, path, kind, size=0):
    """kind: color (sRGB), linear (grey data), normal (DirectX), rgba (linear with alpha)."""
    R.check(name + ' source', Path(path).is_file(), str(path))
    task = unreal.AssetImportTask()
    task.filename = str(path)
    task.destination_path = DEST + '/Textures'
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = True
    T.import_asset_tasks([task])
    tex = E.load_asset(DEST + '/Textures/' + name)
    R.check(name + ' imported', isinstance(tex, unreal.Texture2D))
    tex.set_editor_property('srgb', kind == 'color')
    if kind == 'normal':
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property('flip_green_channel', False)
    else:
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
    if size:
        tex.set_editor_property('max_texture_size', size)
    E.save_loaded_asset(tex)
    R.report['textures'].append(name)
    return tex


def custom(mat, tag, code, out, inputs):
    n = R.expression(mat, unreal.MaterialExpressionCustom)
    n.set_editor_property('description', tag)
    n.set_editor_property('code', code)
    n.set_editor_property('output_type', out)
    pins = []
    for name, _, _ in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property('input_name', name)
        pins.append(pin)
    n.set_editor_property('inputs', pins)
    for name, src, pin in inputs:
        R.check(tag + ' input ' + name, M.connect_material_expressions(src, pin, n, name))
    return n


# Unreal centimetres to SARO metres: p.x = x, p.y = z (south), p.z = height.
CODE = {
    'wall': {
        'uv': 'float3 p = WP * 0.01; float u = (abs(N.x) > 0.5) ? p.y : p.x; return float2(u / 1.6, -p.z / 1.6);',
        'muv': ('float3 p = WP * 0.01; float2 c; float u;'
                ' if (abs(N.x) > 0.5) { u = (p.y - 6.8) / 3.7; c = (N.x > 0) ? float2(0.0, 0.5) : float2(0.5, 0.5); }'
                ' else { u = (p.x + 6.0) / 5.0; c = (N.y > 0) ? float2(0.0, 0.0) : float2(0.5, 0.0); }'
                ' float v = 1.0 - p.z / 2.8;'
                ' return c + float2(clamp(u, 0.002, 0.998), clamp(v, 0.004, 0.996)) * 0.5;'),
        'mask': 'float3 p = WP * 0.01; return (p.x > -6.02 && p.x < -0.98 && p.y > 6.78 && p.y < 10.52 && p.z < 2.85) ? 1.0 : 0.0;',
        'normal': ('float3 n = normalize(N); float3 t = (abs(n.x) > 0.5) ? float3(0, 1, 0) : float3(1, 0, 0);'
                   ' return normalize(t * (Nm.x * S) + float3(0, 0, 1) * (Nm.y * S) + n * Nm.z);'),
    },
    'floor': {
        'uv': 'float3 p = WP * 0.01; return float2(p.x / 1.2, p.y / 1.2);',
        'muv': 'float3 p = WP * 0.01; return float2(clamp((p.x + 6.0) / 5.0, 0.001, 0.999), clamp((p.y - 6.8) / 3.7, 0.001, 0.999));',
        'mask': 'float3 p = WP * 0.01; return (p.x > -6.02 && p.x < -0.98 && p.y > 6.78 && p.y < 10.52) ? 1.0 : 0.0;',
        'normal': 'return normalize(float3(Nm.x * S, -Nm.y * S, Nm.z));',
    },
}


def world_material(name, kind, tex, macro, tint, specular):
    path = DEST + '/Materials/' + name
    mat = E.load_asset(path) if E.does_asset_exist(path) else T.create_asset(name, DEST + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
    for previous in list(M.get_material_expressions(mat)):
        M.delete_material_expression(mat, previous)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('tangent_space_normal', False)
    c = CODE[kind]
    wp = R.expression(mat, unreal.MaterialExpressionWorldPosition)
    vn = R.expression(mat, unreal.MaterialExpressionVertexNormalWS)
    uv = custom(mat, name + ' uv', c['uv'], OUT.CMOT_FLOAT2, [('WP', wp, ''), ('N', vn, '')])
    muv = custom(mat, name + ' macro uv', c['muv'], OUT.CMOT_FLOAT2, [('WP', wp, ''), ('N', vn, '')])
    mask = custom(mat, name + ' mask', c['mask'], OUT.CMOT_FLOAT1, [('WP', wp, '')])

    def sample(texture_asset, coords, sampler, tag):
        s = R.expression(mat, unreal.MaterialExpressionTextureSample, texture=texture_asset, sampler_type=sampler)
        R.check(name + ' ' + tag + ' uv', M.connect_material_expressions(coords, '', s, 'UVs'))
        return s
    b = sample(tex['base'], uv, ST.SAMPLERTYPE_COLOR, 'base')
    r = sample(tex['rough'], uv, ST.SAMPLERTYPE_LINEAR_COLOR, 'rough')
    n = sample(tex['normal'], uv, ST.SAMPLERTYPE_NORMAL, 'normal')
    mc = sample(macro, muv, ST.SAMPLERTYPE_LINEAR_COLOR, 'macro')
    tintp = R.expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name='Tint', default_value=unreal.LinearColor(tint[0], tint[1], tint[2], 1))
    ms = R.expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name='MacroStrength', default_value=1.0)
    ra = R.expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name='RoughAdd', default_value=0.0)
    ns = R.expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name='NormalStrength', default_value=1.0)
    sp = R.expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name='Specular', default_value=specular)
    col = custom(mat, name + ' colour', 'float3 m = lerp(float3(1, 1, 1), Mc * 2.0, saturate(Mask * Ms)); return B * T * m;', OUT.CMOT_FLOAT3,
                 [('B', b, 'RGB'), ('Mc', mc, 'RGB'), ('Mask', mask, ''), ('Ms', ms, ''), ('T', tintp, 'RGB')])
    rough = custom(mat, name + ' roughness', 'return saturate(R + (Ma - 0.5) * Mask * Ms + Ra);', OUT.CMOT_FLOAT1,
                   [('R', r, 'R'), ('Ma', mc, 'A'), ('Mask', mask, ''), ('Ms', ms, ''), ('Ra', ra, '')])
    nrm = custom(mat, name + ' normal', c['normal'], OUT.CMOT_FLOAT3, [('Nm', n, 'RGB'), ('N', vn, ''), ('S', ns, '')])
    R.check(name + ' base colour', M.connect_material_property(col, '', unreal.MaterialProperty.MP_BASE_COLOR))
    R.check(name + ' roughness', M.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS))
    R.check(name + ' normal', M.connect_material_property(nrm, '', unreal.MaterialProperty.MP_NORMAL))
    R.check(name + ' specular', M.connect_material_property(sp, '', unreal.MaterialProperty.MP_SPECULAR))
    errors = list(M.recompile_material(mat))
    R.check(name + ' compiles', not errors, errors)
    E.save_loaded_asset(mat)
    unreal.log('S47: REC material ' + name)
    return mat


def surfaces():
    sets = {}
    for key, size in (('Block', 2048), ('Floor', 2048)):
        sets[key] = {'base': texture('T_Rec_%s_base' % key, TEX / ('T_Rec_%s_base.png' % key), 'color', size),
                     'rough': texture('T_Rec_%s_rough' % key, TEX / ('T_Rec_%s_rough.png' % key), 'linear', size),
                     'normal': texture('T_Rec_%s_normal' % key, TEX / ('T_Rec_%s_normal.png' % key), 'normal', size)}
    wall_macro = texture('T_Rec_WallMacro', TEX / 'T_Rec_WallMacro.png', 'rgba', 2048)
    floor_macro = texture('T_Rec_FloorMacro', TEX / 'T_Rec_FloorMacro.png', 'rgba', 2048)
    world_material('M_Rec_Block', 'wall', sets['Block'], wall_macro, (1, 1, 1), 0.35)
    world_material('M_Rec_Floor', 'floor', sets['Floor'], floor_macro, (0.80, 0.78, 0.73), 0.45)   # a floor that has been walked on since 1968: a shade down from the picture


def tx(name):
    return {'base_color': str(TEX / ('T_Rec_%s_base.png' % name)), 'normal': str(TEX / ('T_Rec_%s_normal.png' % name)), 'roughness': str(TEX / ('T_Rec_%s_rough.png' % name))}


def baked(key):
    d = WORK / 'textures'
    return {'name': 'M_Rec_' + key, 'base_color': [1, 1, 1, 1], 'metallic': 1.0, 'roughness': 0.5, 'specular': 0.5, 'normal_strength': 1.0,
            'textures': {'base_color': str(d / (key + '_basecolor.png')), 'normal': str(d / (key + '_normal.png')),
                         'roughness': str(d / (key + '_roughness.png')), 'metallic': str(d / (key + '_metallic.png'))}}


LENS = {'base_color': str(TEX / 'T_Rec_Lens_base.png'), 'normal': str(TEX / 'T_Rec_Lens_normal.png'), 'roughness': str(TEX / 'T_Rec_Lens_rough.png')}
# key -> (meshes, material rows). A row without textures is a plain colour.
MODELS = {
    'ceiling': (['SM_Rec_Ceiling'], [
        {'name': 'M_Rec_CeilTile', 'base_color': [1, 1, 1, 1], 'metallic': 0, 'roughness': 0.95, 'specular': 0.2, 'normal_strength': 1.0, 'textures': tx('Ceiling')},
        {'name': 'M_Rec_TBar', 'base_color': [0.36, 0.345, 0.29, 1], 'metallic': 0, 'roughness': 0.62, 'specular': 0.35},
        {'name': 'M_Rec_CeilVoid', 'base_color': [0.004, 0.004, 0.004, 1], 'metallic': 0, 'roughness': 1.0, 'specular': 0.0},
        {'name': 'M_Rec_TrofferBody', 'base_color': [0.50, 0.49, 0.43, 1], 'metallic': 0, 'roughness': 0.5, 'specular': 0.4},
        {'name': 'M_Rec_TrofferLensA', 'base_color': [1, 1, 1, 1], 'metallic': 0, 'roughness': 0.35, 'specular': 0.5, 'normal_strength': 0.6,
         'texture_tint_linear': [0.42, 0.42, 0.39], 'emission': [1, 1, 1, 1], 'emission_strength': 1.0, 'textures': LENS},
        {'name': 'M_Rec_TrofferLensB', 'base_color': [1, 1, 1, 1], 'metallic': 0, 'roughness': 0.35, 'specular': 0.5, 'normal_strength': 0.6,
         'texture_tint_linear': [0.42, 0.42, 0.39], 'emission': [1, 1, 1, 1], 'emission_strength': 1.0, 'textures': LENS},
    ]),
    'fittings': (['SM_Rec_Fittings', 'SM_Rec_ExtBracket'], [baked('Fittings')]),
    'table': (['SM_Rec_WorkTable'], [baked('WorkTable')]),
    'lamp': (['SM_Rec_BankerLamp'], [baked('BankerLamp'),
        {'name': 'M_Rec_LampGlass', 'base_color': [0.012, 0.16, 0.06, 1], 'metallic': 0, 'roughness': 0.10, 'specular': 0.6, 'emission': [0, 0, 0, 1], 'emission_strength': 0},
        {'name': 'M_Rec_LampInner', 'base_color': [0.80, 0.79, 0.72, 1], 'metallic': 0, 'roughness': 0.45, 'specular': 0.4, 'emission': [0, 0, 0, 1], 'emission_strength': 0, 'two_sided': True}]),
    'typewriter': (['SM_Rec_Typewriter'], [baked('Typewriter')]),
    'shelving': (['SM_Rec_ShelfBay', 'SM_Rec_ShelfEnd'], [baked('Shelving')]),
    'bay2': (['SM_Rec_ShelfBay2'], [baked('ShelfBay2')]),
    'cabinetA': (['SM_Rec_CabinetA'], [baked('CabinetA')]),
    'cabinetB': (['SM_Rec_CabinetB'], [baked('CabinetB')]),
    'cabinetC': (['SM_Rec_CabinetC'], [baked('CabinetC')]),
    'archive': (['SM_Rec_BoxA', 'SM_Rec_BoxB', 'SM_Rec_BoxC', 'SM_Rec_BoxD', 'SM_Rec_BoxE', 'SM_Rec_BoxF', 'SM_Rec_BoxG', 'SM_Rec_BoxH',
                 'SM_Rec_BinderA', 'SM_Rec_BinderB', 'SM_Rec_BinderC', 'SM_Rec_Folders', 'SM_Rec_Papers'], [baked('Archive')]),
}


def model(key):
    names, rows = MODELS[key]
    mats = {}
    for row in rows:
        mats[row['name']] = R.material(row, DEST + '/Materials', WORK)
        # roughness and metallic at half the atlas's size: they hold no fine print (memory)
        for role in ('roughness', 'metallic'):
            t = DEST + '/Materials/Textures/T_' + row['name'] + '_' + role
            if row.get('textures', {}).get(role) and E.does_asset_exist(t):
                tex = E.load_asset(t)
                tex.set_editor_property('max_texture_size', max(512, tex.blueprint_get_size_x() // 2))
                E.save_loaded_asset(tex)
    for name in names:
        src = WORK / 'Export' / (name + '.fbx')
        R.check(name + ' source', src.is_file(), str(src))
        asset = DEST + '/' + name
        if E.does_asset_exist(asset):
            R.check(name + ' replace', E.delete_asset(asset))
        mesh = R.import_mesh(src, DEST)
        slots = []
        for index, slot in enumerate(mesh.get_editor_property('static_materials')):
            material = str(slot.material_slot_name)
            R.check(name + ' slot ' + material, material in mats, list(mats))
            mesh.set_material(index, mats[material])
            slots.append(material)
        settings = mesh.get_editor_property('nanite_settings')
        settings.enabled = False
        mesh.set_editor_property('nanite_settings', settings)
        for lod in range(S.get_lod_count(mesh)):
            build = S.get_lod_build_settings(mesh, lod)
            build.set_editor_property('use_full_precision_u_vs', True)
            S.set_lod_build_settings(mesh, lod, build)
        R.check(name + ' saved', E.save_loaded_asset(mesh, only_if_is_dirty=False))
        box = mesh.get_bounding_box()
        R.report['meshes'].append({'name': name, 'triangles': mesh.get_num_triangles(0), 'slots': slots,
                                   'min_cm': [round(v, 2) for v in (box.min.x, box.min.y, box.min.z)],
                                   'max_cm': [round(v, 2) for v in (box.max.x, box.max.y, box.max.z)]})
        unreal.log('S47: REC mesh %s %d tris %s' % (name, mesh.get_num_triangles(0), slots))


# Tom's two single Cosmos props (Tools/recordsroom/cosmos_props.py copies them to /Game/Rocket/<dir>, git-ignored). Their zips
# name a master material and an instance that are not in them: each mesh gets a plain PBR material of the pack's three textures.
COSMOS = {'RPS_DeskFan': (['SM_RPS_DeskFan01_c1_LOD1', 'SM_RPS_DeskFan02_c1_LOD1'], 'T_RPS_DeskFan_2048'),
          'RPS_FireExtinguisher': (['SM_RPS_FireExtinguisher_02_c1_LOD1'], 'T_RPS_FireExtinguisher_2048')}


def cosmos():
    for folder, (names, tex) in COSMOS.items():
        root = '/Game/Rocket/' + folder
        if not all(E.does_asset_exist(root + '/' + n) for n in names):
            unreal.log('S47: REC cosmos ' + folder + ' not here: the room keeps its old piece')
            continue
        name = 'M_Rec_' + folder
        mat = E.load_asset(root + '/' + name) if E.does_asset_exist(root + '/' + name) else T.create_asset(name, root, unreal.Material, unreal.MaterialFactoryNew())
        for previous in list(M.get_material_expressions(mat)):
            M.delete_material_expression(mat, previous)

        def sample(suffix):
            t = E.load_asset(root + '/' + tex + suffix)
            R.check(name + ' texture ' + suffix, isinstance(t, unreal.Texture2D))
            comp = t.get_editor_property('compression_settings')
            kind = (ST.SAMPLERTYPE_NORMAL if comp == unreal.TextureCompressionSettings.TC_NORMALMAP else
                    ST.SAMPLERTYPE_MASKS if comp == unreal.TextureCompressionSettings.TC_MASKS else
                    ST.SAMPLERTYPE_COLOR if t.get_editor_property('srgb') else ST.SAMPLERTYPE_LINEAR_COLOR)
            return R.expression(mat, unreal.MaterialExpressionTextureSample, texture=t, sampler_type=kind)
        b, n, orm = sample('_B'), sample('_N'), sample('_ORM')
        R.check(name + ' base', M.connect_material_property(b, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR))
        R.check(name + ' normal', M.connect_material_property(n, 'RGB', unreal.MaterialProperty.MP_NORMAL))
        R.check(name + ' ao', M.connect_material_property(orm, 'R', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION))
        R.check(name + ' rough', M.connect_material_property(orm, 'G', unreal.MaterialProperty.MP_ROUGHNESS))
        R.check(name + ' metal', M.connect_material_property(orm, 'B', unreal.MaterialProperty.MP_METALLIC))
        errors = list(M.recompile_material(mat))
        R.check(name + ' compiles', not errors, errors)
        E.save_loaded_asset(mat)
        for n_ in names:
            mesh = E.load_asset(root + '/' + n_)
            R.check(n_ + ' loads', isinstance(mesh, unreal.StaticMesh))
            for index in range(len(mesh.get_editor_property('static_materials'))):
                mesh.set_material(index, mat)
            settings = mesh.get_editor_property('nanite_settings')
            settings.enabled = False
            mesh.set_editor_property('nanite_settings', settings)
            R.check(n_ + ' saved', E.save_loaded_asset(mesh, only_if_is_dirty=False))
            box = mesh.get_bounding_box()
            R.report['meshes'].append({'name': n_, 'triangles': mesh.get_num_triangles(0), 'slots': len(mesh.get_editor_property('static_materials')),
                                       'min_cm': [round(v, 2) for v in (box.min.x, box.min.y, box.min.z)], 'max_cm': [round(v, 2) for v in (box.max.x, box.max.y, box.max.z)]})
            unreal.log('S47: REC cosmos mesh %s %d tris' % (n_, mesh.get_num_triangles(0)))


try:
    R.flush()
    todo = WHAT or (['surfaces', 'labels', 'cosmos'] + [k for k, (names, _) in MODELS.items() if all((WORK / 'Export' / (n + '.fbx')).is_file() for n in names)])
    for what in todo:
        if what == 'surfaces':
            surfaces()
        elif what == 'labels':
            texture('T_Rec_Labels', TEX / 'T_Rec_Labels.png', 'color', 2048)
        elif what == 'cosmos':
            cosmos()
        else:
            model(what)
    R.report['status'] = 'PASS'
    R.flush()
    unreal.log('S47: REC IMPORT PASS ' + ','.join(todo))
except Exception:
    R.report['status'] = 'FAIL'
    R.report['error'] = traceback.format_exc()
    R.flush()
    unreal.log_error('S47: REC IMPORT FAIL ' + traceback.format_exc())
    raise
