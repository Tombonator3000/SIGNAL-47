"""LAND-PLACES-20261114: brings a land place's meshes, baked maps and labels into /Game/LandPlaces/<Place>.

Sources (outside Git, made again by <place>/build_<place>.py and <place>/make_labels.py):
  S47_LP_WORK/Export, /textures, /contract.json     (default ~/scratch-bm/land-places-20261114/<place>/work)
  S47_LP_WORK/../tex                                 the labels
Run through the queue with no editor open:  python3 Tools/landplaces/run.py import <place>
Uses the project's measured FBX recipe (Tools/fieldintegration/import_recipe.py): Blender (x, y, z) m -> Unreal (x, -y, z) cm.

Each atlas is three maps: T_LP_<Atlas>_BC (colour), _N (normal, DirectX) and _ORM (occlusion, roughness, metal in R, G, B). The
meshes carry their own detail levels (LOD1..4, built from the same primitives on the same texels) at screen sizes worked out
from each mesh's own size for 30, 90, 220 and 550 m, so every part of a place changes level at the same distance. No Nanite (thin
lattice does badly in it, and the project's props do not use it). Traces go against a low level's own triangles
(lod_for_collision, complex as simple): what stops a look is exactly what is drawn, a little coarser.
"""
import json
import os
import sys
import traceback
from pathlib import Path

import unreal

HERE = Path(__file__).resolve().parent
PLACE = os.environ.get('S47_LP_PLACE', 'windmill')
WORK = Path(os.path.expanduser(os.environ.get('S47_LP_WORK', '~/scratch-bm/land-places-20261114/%s/work' % PLACE))).resolve()
LABELS = WORK.parent / 'tex'
FOLDER = {'windmill': 'Windmill', 'line_shack': 'LineShack', 'mine': 'Mine', 'cairn': 'Cairn'}[PLACE]
PREFIX = {'windmill': 'SM_LP_Mill_'}[PLACE]
DEST = '/Game/LandPlaces/' + FOLDER
sys.path.insert(0, str(HERE.parent / 'fieldintegration'))
import import_recipe as R

R.REPORT = WORK / 'import-report.json'
R.report = {'status': 'RUNNING', 'engine': unreal.SystemLibrary.get_engine_version(), 'source': str(WORK), 'destination': DEST, 'checks': [], 'meshes': [], 'textures': []}
E, S, M, T = R.E, R.S, R.M, R.T
ST = unreal.MaterialSamplerType
MP = unreal.MaterialProperty
DIST_M = (30.0, 90.0, 220.0, 550.0)    # where LOD1, 2, 3 and 4 begin
SCREEN = 1.428                         # screen size = SCREEN x radius / distance: 1 / tan(35), the walker's 70 degree vertical view


def texture(name, path, kind, size=0):
    """kind: color (sRGB), normal (DirectX), masks (linear R, G, B)."""
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
    elif kind == 'masks':
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
    else:
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
    if size:
        tex.set_editor_property('max_texture_size', size)
    E.save_loaded_asset(tex)
    R.report['textures'].append(name)
    return tex


def fresh(name):
    path = DEST + '/Materials/' + name
    mat = E.load_asset(path) if E.does_asset_exist(path) else T.create_asset(name, DEST + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
    for previous in list(M.get_material_expressions(mat)):
        M.delete_material_expression(mat, previous)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    return mat


def done(mat, name):
    errors = list(M.recompile_material(mat))
    R.check(name + ' compiles', not errors, errors)
    E.save_loaded_asset(mat)
    unreal.log('S47: LP material ' + name)
    return mat


def atlas_material(name, key, orm_size=0):
    d = WORK / 'textures'
    bc = texture('T_LP_%s_BC' % key, d / ('T_LP_%s_BC.png' % key), 'color')
    nm = texture('T_LP_%s_N' % key, d / ('T_LP_%s_N.png' % key), 'normal')
    orm = texture('T_LP_%s_ORM' % key, d / ('T_LP_%s_ORM.png' % key), 'masks', orm_size)
    mat = fresh(name)
    b = R.expression(mat, unreal.MaterialExpressionTextureSample, texture=bc, sampler_type=ST.SAMPLERTYPE_COLOR)
    n = R.expression(mat, unreal.MaterialExpressionTextureSample, texture=nm, sampler_type=ST.SAMPLERTYPE_NORMAL)
    o = R.expression(mat, unreal.MaterialExpressionTextureSample, texture=orm, sampler_type=ST.SAMPLERTYPE_MASKS)
    tint = R.expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name='Tint', default_value=unreal.LinearColor(1, 1, 1, 1))
    aoc = R.expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name='OcclusionInColour', default_value=0.5)
    one = R.expression(mat, unreal.MaterialExpressionConstant, r=1.0)
    lerp = R.expression(mat, unreal.MaterialExpressionLinearInterpolate)
    R.check(name + ' ao a', M.connect_material_expressions(one, '', lerp, 'A'))
    R.check(name + ' ao b', M.connect_material_expressions(o, 'R', lerp, 'B'))
    R.check(name + ' ao k', M.connect_material_expressions(aoc, '', lerp, 'Alpha'))
    m1 = R.expression(mat, unreal.MaterialExpressionMultiply)
    R.check(name + ' colour a', M.connect_material_expressions(b, 'RGB', m1, 'A'))
    R.check(name + ' colour b', M.connect_material_expressions(lerp, '', m1, 'B'))
    m2 = R.expression(mat, unreal.MaterialExpressionMultiply)
    R.check(name + ' tint a', M.connect_material_expressions(m1, '', m2, 'A'))
    R.check(name + ' tint b', M.connect_material_expressions(tint, 'RGB', m2, 'B'))
    R.check(name + ' base colour', M.connect_material_property(m2, '', MP.MP_BASE_COLOR))
    R.check(name + ' normal', M.connect_material_property(n, 'RGB', MP.MP_NORMAL))
    R.check(name + ' occlusion', M.connect_material_property(o, 'R', MP.MP_AMBIENT_OCCLUSION))
    R.check(name + ' roughness', M.connect_material_property(o, 'G', MP.MP_ROUGHNESS))
    R.check(name + ' metallic', M.connect_material_property(o, 'B', MP.MP_METALLIC))
    return done(mat, name)


def water_material(name):
    """Still, dark water that takes the sky: no texture, a smooth dielectric."""
    mat = fresh(name)
    col = R.expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name='Color', default_value=unreal.LinearColor(0.012, 0.017, 0.015, 1))
    R.check(name + ' colour', M.connect_material_property(col, 'RGB', MP.MP_BASE_COLOR))
    for key, prop, value in (('Roughness', MP.MP_ROUGHNESS, 0.035), ('Specular', MP.MP_SPECULAR, 0.5), ('Metallic', MP.MP_METALLIC, 0.0)):
        p = R.expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name=key, default_value=value)
        R.check(name + ' ' + key, M.connect_material_property(p, '', prop))
    return done(mat, name)


def windmill():
    contract = json.loads((WORK / 'contract.json').read_text())
    mats = {'M_LP_MillTower': atlas_material('M_LP_MillTower', 'MillTower', 2048), 'M_LP_MillTop': atlas_material('M_LP_MillTop', 'MillTop', 1024),
            'M_LP_MillBase': atlas_material('M_LP_MillBase', 'MillBase', 2048), 'M_LP_MillWater': water_material('M_LP_MillWater')}
    mats['M_LP_MillWire'] = R.material({'name': 'M_LP_MillWire', 'base_color': [0.70, 0.72, 0.73, 1], 'metallic': 1.0, 'roughness': 0.32, 'specular': 0.5}, DEST + '/Materials', WORK)
    mats['M_LP_MillPage'] = R.material({'name': 'M_LP_MillPage', 'base_color': [1, 1, 1, 1], 'metallic': 0.0, 'roughness': 0.28, 'specular': 0.5, 'two_sided': True,
                                        'textures': {'base_color': str(LABELS / 'T_LP_MillPage.png')}}, DEST + '/Materials', WORK)
    mats['M_LP_MillVaneLabel'] = R.material({'name': 'M_LP_MillVaneLabel', 'base_color': [1, 1, 1, 1], 'metallic': 0.0, 'roughness': 0.8, 'specular': 0.2,
                                             'opacity_from_base_color': True, 'alpha_mode': 'dithered_masked', 'textures': {'base_color': str(LABELS / 'T_LP_MillVaneLabel.png')}}, DEST + '/Materials', WORK)
    for part, row in contract['parts'].items():
        name = PREFIX + part
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
        R.check(name + ' one slot', len(slots) == 1, slots)
        lods = contract['lods'][part]
        for lod in range(1, lods):
            R.check(name + ' LOD' + str(lod), R.import_lod_same_frame(mesh, lod, WORK / 'Export' / ('%s_LOD%d.fbx' % (name, lod)), DEST) == lod)
        for index, slot in enumerate(mesh.get_editor_property('static_materials')):
            material = str(slot.get_editor_property('material_slot_name'))
            if material in mats and slot.get_editor_property('material_interface') != mats[material]:
                mesh.set_material(index, mats[material])
        if lods > 1:
            # every level in LOD0's own frame: a lower level may leave small pieces out (its box inside LOD0's), never stand outside it
            # (the last level of the tower is one slim pylon, well inside)
            base = R.lod_box(mesh, 0)
            for lod in range(1, mesh.get_num_lods()):
                box = R.lod_box(mesh, lod)
                out = max(max(base[0][k] - box[0][k], box[1][k] - base[1][k]) for k in range(3))
                gone = max(max(box[0][k] - base[0][k], base[1][k] - box[1][k]) for k in range(3))
                R.check('%s LOD%d in LOD0 frame' % (name, lod), out <= 3.0 and (gone <= 60.0 or lod >= 4), {'lod0': base, 'lod': box, 'outside_cm': out, 'left_out_cm': gone})
            radius = mesh.get_bounds().sphere_radius / 100.0
            sizes = [1.0] + [SCREEN * radius / d for d in DIST_M[:lods - 1]]
            R.check(name + ' screen sizes', S.set_lod_screen_sizes(mesh, sizes), sizes)
        settings = mesh.get_editor_property('nanite_settings')
        settings.enabled = False
        mesh.set_editor_property('nanite_settings', settings)
        mesh.set_editor_property('support_ray_tracing', True)
        mesh.set_editor_property('lod_for_collision', min(2, lods - 1) if part in ('Tower', 'Wheel', 'Tank', 'Head', 'Housing') else 0)
        body = mesh.get_editor_property('body_setup')
        if body:
            body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        for lod in range(S.get_lod_count(mesh)):
            build = S.get_lod_build_settings(mesh, lod)
            build.set_editor_property('use_full_precision_u_vs', True)
            S.set_lod_build_settings(mesh, lod, build)
        R.check(name + ' saved', E.save_loaded_asset(mesh, only_if_is_dirty=False))
        triangles = [mesh.get_num_triangles(lod) for lod in range(S.get_lod_count(mesh))]
        R.check(name + ' levels', len(triangles) == lods and triangles == row['triangles'][:lods], {'engine': triangles, 'source': row['triangles']})
        box = mesh.get_bounding_box()
        pv = row['pivot_m']
        lo = [(row['min_m'][0] - pv[0]) * 100, -(row['max_m'][1] - pv[1]) * 100, (row['min_m'][2] - pv[2]) * 100]
        hi = [(row['max_m'][0] - pv[0]) * 100, -(row['min_m'][1] - pv[1]) * 100, (row['max_m'][2] - pv[2]) * 100]
        got = [box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z]
        worst = max(abs(a - b) for a, b in zip(got, lo + hi))
        R.check(name + ' bounds as built (cm)', worst < 0.6, {'engine': [round(v, 2) for v in got], 'source': [round(v, 2) for v in lo + hi]})
        R.report['meshes'].append({'name': name, 'triangles': triangles, 'slots': slots, 'min_cm': [round(v, 2) for v in got[:3]], 'max_cm': [round(v, 2) for v in got[3:]]})
        unreal.log('S47: LP mesh %s %s tris %s' % (name, triangles, slots))


try:
    R.flush()
    {'windmill': windmill}[PLACE]()
    R.report['status'] = 'PASS'
    R.flush()
    unreal.log('S47: LP IMPORT PASS ' + PLACE)
except Exception:
    R.report['status'] = 'FAIL'
    R.report['error'] = traceback.format_exc()
    R.flush()
    unreal.log_error('S47: LP IMPORT FAIL ' + traceback.format_exc())
    raise
