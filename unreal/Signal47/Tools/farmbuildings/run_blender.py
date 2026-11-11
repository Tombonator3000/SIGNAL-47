"""Blender-driver: bygger ett gårdsbygg, eksporterer og rendrer.

blender -b -t 4 --python-exit-code 1 --python run_blender.py -- <utrot> <bygg> <clay|mat|back|export> [visninger] [lod]

Akser og FBX-valg følger townbuildings (meter, Z opp, axis_forward -Y, axis_up Z, FBX_SCALE_UNITS).
Alle LOD-er deler nullpunkt og koordinatramme. UCX ligger i LOD0-filen. Tekstur leses fra <utrot>/textures.
"""
import importlib
import json
import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Vector

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.dont_write_bytecode = True
import fbkit  # noqa: E402

argv = sys.argv[sys.argv.index('--') + 1:]
ROOT, BUILD, MODE = Path(argv[0]), argv[1], argv[2]
VIEWS = argv[3].split(',') if len(argv) > 3 and argv[3] != 'all' else None
LOD_VIEW = int(argv[4]) if len(argv) > 4 else 0
TEX = ROOT / 'textures'
RENDER_DIR = ROOT / 'renders' / MODE / BUILD
FBX_DIR = ROOT / 'fbx'
BLEND_DIR = ROOT / 'blend'
for d in (RENDER_DIR, FBX_DIR, BLEND_DIR):
    d.mkdir(parents=True, exist_ok=True)

NAMES = {'barn': 'SM_FarmBarn', 'house': 'SM_FarmHouse', 'shed_a': 'SM_FarmShedA', 'shed_b': 'SM_FarmShedB', 'shed_door': 'SM_FarmShedDoor'}
CAMS = {
    'barn': {'hero': ((-19, -21, 1.6), (0, 0, 3.0), 30), 'rear': ((20, 19, 1.6), (0, 0, 3.0), 30),
             'front': ((1.5, -27, 1.6), (0, 0, 3.4), 35), 'door': ((1.0, -11.5, 1.6), (0, -5.5, 2.1), 30),
             'inside': ((1.8, -4.9, 1.6), (1.2, 5, 2.2), 20), 'roof': ((-24, -27, 15), (0, 0, 2.5), 32)},
    'house': {'hero': ((-13, -16, 1.6), (0, 0, 2.0), 30), 'rear': ((14, 15, 1.6), (0, 0, 2.0), 30),
              'front': ((1.5, -19, 1.6), (0, 0, 2.2), 35), 'door': ((-3.6, -8.2, 1.6), (-.6, -2.6, 1.5), 28),
              'east': ((15, -4, 1.6), (5.5, 0, 2.0), 32), 'roof': ((-17, -20, 12), (0, 0, 1.8), 32)},
    'shed_a': {'hero': ((-9, -12, 1.6), (0, 0, 1.6), 30), 'rear': ((10, 11, 1.6), (0, 0, 1.6), 30),
               'front': ((.5, -14, 1.6), (0, 0, 1.7), 32), 'roof': ((-11, -13, 8), (0, 0, 1), 32)},
    'shed_b': {'hero': ((-6, -8, 1.6), (0, 0, 1.4), 30), 'rear': ((6.5, 7.5, 1.6), (0, 0, 1.4), 30),
               'door': ((-.8, -4.8, 1.6), (-.4, -1.6, 1.2), 26), 'roof': ((-7, -8, 6), (0, 0, 1), 32)},
    'shed_door': {'hero': ((-1.1, -3.4, 1.6), (.48, 0, 1.0), 30), 'back': ((1.7, 2.9, 1.6), (.48, 0, 1.0), 30)},
}


def load(name):
    if name.startswith('shed'):
        mod = importlib.import_module('sheds')
        return (lambda lod: getattr(mod, 'build_' + name)(lod)), getattr(mod, 'collision_' + name)
    mod = importlib.import_module(name)
    return mod.build, mod.collision


def material(name):
    if name in bpy.data.materials:
        return bpy.data.materials[name]
    short = name.replace('M_Farm_', '')
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    p = nt.nodes.get('Principled BSDF')
    bc = nt.nodes.new('ShaderNodeTexImage')
    bc.image = bpy.data.images.load(str(TEX / f'T_Farm_{short}_BC.png'), check_existing=True)
    vc = nt.nodes.new('ShaderNodeVertexColor')
    vc.layer_name = 'Col'
    mul = nt.nodes.new('ShaderNodeMixRGB')
    mul.blend_type = 'MULTIPLY'
    mul.inputs[0].default_value = 1
    nt.links.new(bc.outputs['Color'], mul.inputs[1])
    nt.links.new(vc.outputs['Color'], mul.inputs[2])
    # Samme graf som import_assets.py: faktor per materiale, og jord nederst der vertex alpha sier det.
    fac = nt.nodes.new('ShaderNodeMixRGB')
    fac.blend_type = 'MULTIPLY'
    fac.inputs[0].default_value = 1
    fac.inputs[2].default_value = (*fbkit.ALBEDO[short], 1)
    nt.links.new(mul.outputs[0], fac.inputs[1])
    inv_a = nt.nodes.new('ShaderNodeMath')
    inv_a.operation = 'SUBTRACT'
    inv_a.inputs[0].default_value = 1
    nt.links.new(vc.outputs['Alpha'], inv_a.inputs[1])
    soil = nt.nodes.new('ShaderNodeMixRGB')
    soil.blend_type = 'MIX'
    soil.inputs[2].default_value = (*fbkit.EARTH_COLOR, 1)
    nt.links.new(inv_a.outputs[0], soil.inputs[0])
    nt.links.new(fac.outputs[0], soil.inputs[1])
    nt.links.new(soil.outputs[0], p.inputs['Base Color'])
    r = nt.nodes.new('ShaderNodeTexImage')
    r.image = bpy.data.images.load(str(TEX / f'T_Farm_{short}_R.png'), check_existing=True)
    r.image.colorspace_settings.name = 'Non-Color'
    nt.links.new(r.outputs['Color'], p.inputs['Roughness'])
    n = nt.nodes.new('ShaderNodeTexImage')
    n.image = bpy.data.images.load(str(TEX / f'T_Farm_{short}_N.png'), check_existing=True)
    n.image.colorspace_settings.name = 'Non-Color'
    sep = nt.nodes.new('ShaderNodeSeparateColor')
    inv = nt.nodes.new('ShaderNodeMath')
    inv.operation = 'SUBTRACT'
    inv.inputs[0].default_value = 1
    com = nt.nodes.new('ShaderNodeCombineColor')
    nm = nt.nodes.new('ShaderNodeNormalMap')
    nt.links.new(n.outputs['Color'], sep.inputs[0])
    nt.links.new(sep.outputs[1], inv.inputs[1])  # DirectX-kart: grønn vendes for Blender.
    nt.links.new(sep.outputs[0], com.inputs[0])
    nt.links.new(inv.outputs[0], com.inputs[1])
    nt.links.new(sep.outputs[2], com.inputs[2])
    nt.links.new(com.outputs[0], nm.inputs['Color'])
    nt.links.new(nm.outputs[0], p.inputs['Normal'])
    p.inputs['Specular IOR Level'].default_value = .35
    if short == 'Steel':
        p.inputs['Metallic'].default_value = .55
    if short == 'Tin':
        p.inputs['Metallic'].default_value = .12
    if short == 'Board':
        p.inputs['Specular IOR Level'].default_value = .2
    if short == 'Glass':
        p.inputs['Specular IOR Level'].default_value = .6
    if short == 'WindowLit':
        e = nt.nodes.new('ShaderNodeTexImage')
        e.image = bpy.data.images.load(str(TEX / f'T_Farm_{short}_E.png'), check_existing=True)
        nt.links.new(e.outputs['Color'], p.inputs['Emission Color'])
        p.inputs['Emission Strength'].default_value = 2.5
    return m


def to_mesh(k, name, order):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(k.v, [], k.f)
    mesh.update()
    for m in order:
        mesh.materials.append(material(m))
    uv = mesh.uv_layers.new(name='UVMap')
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    for i, (p, mat, coords, smooth, tint) in enumerate(zip(mesh.polygons, k.mat, k.uv, k.smooth, k.tint)):
        fx = k.fx.get(i)
        p.material_index = order.index(mat)
        p.use_smooth = smooth
        for li, t in zip(p.loop_indices, coords):
            uv.data[li].uv = t
            v = k.v[mesh.loops[li].vertex_index]
            g = fbkit.grime(mat, v)
            e = fbkit.earth(g)
            w = fbkit.fx_tint(fx, v)
            c = (min(1, tint[0] * e[0] * w[0]), min(1, tint[1] * e[1] * w[1]), min(1, tint[2] * e[2] * w[2]), 1 - fbkit.earth_mix(mat, v))
            col.data[li].color_srgb = c
    mesh.validate(clean_customdata=False)
    ob = bpy.data.objects.new(name, mesh)
    return ob


def ucx_object(name, verts):
    bm = bmesh.new()
    for v in verts:
        bm.verts.new(v)
    bmesh.ops.convex_hull(bm, input=bm.verts, use_existing_faces=False)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    return bpy.data.objects.new(name, mesh)


def tri_count(ob):
    ob.data.calc_loop_triangles()
    return len(ob.data.loop_triangles)


def bounds(ob):
    vs = [v.co for v in ob.data.vertices]
    return [[min(v[i] for v in vs) for i in range(3)], [max(v[i] for v in vs) for i in range(3)]]


def export(path, obs):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in obs:
        ob.hide_set(False)
        ob.select_set(True)
    bpy.context.view_layer.objects.active = obs[0]
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH'}, apply_unit_scale=True,
                             apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Y', axis_up='Z', bake_anim=False,
                             add_leaf_bones=False, mesh_smooth_type='FACE', use_mesh_modifiers=True, colors_type='SRGB',
                             path_mode='RELATIVE')


def verify():
    """Les den eksporterte FBX-en inn på nytt i tom scene og mål. Skriver reimport_<bygg>.json."""
    from spec import SPEC
    spec = SPEC[BUILD]
    stats = json.loads((ROOT / f'stats_{BUILD}.json').read_text())
    name = NAMES[BUILD]
    out = {'name': name, 'spec': spec, 'files': [], 'ok': True}
    for lod, suffix in enumerate(('', '_LOD1', '_LOD2')):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        path = FBX_DIR / f'{name}{suffix}.fbx'
        bpy.ops.import_scene.fbx(filepath=str(path))
        obs = list(bpy.context.scene.objects)
        render = [o for o in obs if not o.name.startswith('UCX_')]
        ucx = [o for o in obs if o.name.startswith('UCX_')]
        kinds = sorted({o.type for o in obs})
        pts = [o.matrix_world @ v.co for o in render for v in o.data.vertices]
        lo = [min(p[i] for p in pts) for i in range(3)]
        hi = [max(p[i] for p in pts) for i in range(3)]
        tris = 0
        for o in render:
            o.data.calc_loop_triangles()
            tris += len(o.data.loop_triangles)
        names = [o.name for o in obs] + [m.name for m in bpy.data.materials]
        dotted = [n for n in names if '.0' in n]
        want = stats['lods'][lod]
        d = max(abs(a - b) for a, b in zip(lo + hi, want['bounds'][0] + want['bounds'][1]))
        row = {'file': path.name, 'objects': len(render), 'ucx': len(ucx), 'types': kinds, 'tris': tris, 'bounds_min': [round(v, 3) for v in lo],
               'bounds_max': [round(v, 3) for v in hi], 'delta_vs_build_m': round(d, 4), 'dotted_names': dotted,
               'slots': [s.material.name if s.material else None for s in render[0].material_slots],
               'has_uv': bool(render[0].data.uv_layers), 'has_col': bool(render[0].data.color_attributes)}
        if render[0].data.color_attributes:
            alpha = [c.color[3] for c in render[0].data.color_attributes[0].data]
            row['earth_alpha'] = [round(min(alpha), 3), round(max(alpha), 3)]
        lim = spec['max']
        good = (kinds == ['MESH'] and tris == want['triangles'] and d < .012 and not dotted and tris <= spec['tris'][lod]
                and lo[0] >= lim[0] and hi[0] <= lim[1] and lo[1] >= lim[2] and hi[1] <= lim[3] and hi[2] <= lim[4])
        row['ok'] = good
        out['ok'] &= good
        if lod == 0:
            good_ucx = len(ucx) == len(stats['ucx'])
            row['ucx_ok'] = good_ucx
            out['ok'] &= good_ucx
        out['files'].append(row)
        print('REIMPORT', json.dumps(row), flush=True)
    (ROOT / f'reimport_{BUILD}.json').write_text(json.dumps(out, indent=1) + '\n')
    print('VERIFY', BUILD, 'PASS' if out['ok'] else 'FAIL', flush=True)
    if not out['ok']:
        raise SystemExit(1)


def main():
    if MODE == 'verify':
        return verify()
    for ob in list(bpy.data.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1
    builder, collide = load(BUILD)
    name = NAMES[BUILD]
    kits = [builder(lod) for lod in range(3)]
    order = fbkit.mats_used(kits[0])
    src = bpy.data.collections.new('EDITABLE_FARM_' + BUILD.upper())
    scene.collection.children.link(src)
    meshes = []
    for lod, k in enumerate(kits):
        assert fbkit.mats_used(k) == order, ('material slots differ', lod, fbkit.mats_used(k), order)
        ob = to_mesh(k, name + ('' if lod == 0 else f'_LOD{lod}'), order)
        src.objects.link(ob)
        meshes.append(ob)
    ucx = []
    for i, verts in enumerate(collide()):
        ob = ucx_object(f'UCX_{name}_{i + 1:02d}', verts)
        src.objects.link(ob)
        ob.display_type = 'WIRE'
        ob.hide_render = True
        ucx.append(ob)
    for lod, ob in enumerate(meshes):
        ob.hide_render = lod != LOD_VIEW
    stats = {'name': name, 'lods': [{'triangles': tri_count(ob), 'bounds': bounds(ob)} for ob in meshes],
             'materials': order, 'ucx': [{'name': o.name, 'bounds': bounds(o), 'triangles': tri_count(o)} for o in ucx],
             'solids_local': kits[0].solids}
    (ROOT / f'stats_{BUILD}.json').write_text(json.dumps(stats, indent=1) + '\n')
    print('STATS', [l['triangles'] for l in stats['lods']], 'ucx', len(ucx), flush=True)

    export(FBX_DIR / f'{name}.fbx', [meshes[0]] + ucx)
    export(FBX_DIR / f'{name}_LOD1.fbx', [meshes[1]])
    export(FBX_DIR / f'{name}_LOD2.fbx', [meshes[2]])
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_DIR / f'{name}.blend'), compress=True)
    if MODE == 'export':
        return

    # Scene til forhåndsvisning: bakke, sol, himmel. Ingenting av dette går i FBX.
    gm = bpy.data.materials.new('Ground')
    gm.use_nodes = True
    nt = gm.node_tree
    p = nt.nodes.get('Principled BSDF')
    tc = nt.nodes.new('ShaderNodeTexCoord')
    noise = nt.nodes.new('ShaderNodeTexNoise')
    noise.inputs['Scale'].default_value = .25
    noise.inputs['Detail'].default_value = 12
    ramp = nt.nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].position, ramp.color_ramp.elements[0].color = .35, (.26, .19, .12, 1)
    ramp.color_ramp.elements[1].position, ramp.color_ramp.elements[1].color = .65, (.40, .30, .20, 1)
    nt.links.new(tc.outputs['Object'], noise.inputs['Vector'])
    nt.links.new(noise.outputs[0], ramp.inputs[0])
    nt.links.new(ramp.outputs[0], p.inputs['Base Color'])
    p.inputs['Roughness'].default_value = .95
    gmesh = bpy.data.meshes.new('Ground')
    gmesh.from_pydata([(-300, -300, 0), (300, -300, 0), (300, 300, 0), (-300, 300, 0)], [], [(0, 1, 2, 3)])
    gmesh.materials.append(gm)
    gob = bpy.data.objects.new('Ground', gmesh)
    scene.collection.objects.link(gob)
    sun = bpy.data.lights.new('LGT_Key_Sun', 'SUN')
    sun.energy = 4.2
    sun.angle = math.radians(1.2)
    sun.use_temperature = True
    sun.temperature = 5400
    sob = bpy.data.objects.new('LGT_Key_Sun', sun)
    sob.rotation_mode = 'QUATERNION'
    # Lav ettermiddagssol fra sørvest.
    sob.rotation_quaternion = Vector((.78, .36, -.5)).to_track_quat('-Z', 'Y')
    scene.collection.objects.link(sob)
    w = bpy.data.worlds.new('Sky')
    w.use_nodes = True
    bg = w.node_tree.nodes['Background']
    grad = w.node_tree.nodes.new('ShaderNodeTexGradient')
    tc = w.node_tree.nodes.new('ShaderNodeTexCoord')
    mapn = w.node_tree.nodes.new('ShaderNodeMapping')
    mapn.inputs['Rotation'].default_value = (0, math.radians(90), 0)
    rr = w.node_tree.nodes.new('ShaderNodeValToRGB')
    rr.color_ramp.elements[0].color = (.62, .68, .74, 1)
    rr.color_ramp.elements[1].color = (.20, .34, .62, 1)
    rr.color_ramp.elements[1].position = .55
    w.node_tree.links.new(tc.outputs['Generated'], mapn.inputs['Vector'])
    w.node_tree.links.new(mapn.outputs[0], grad.inputs[0])
    w.node_tree.links.new(grad.outputs[0], rr.inputs[0])
    w.node_tree.links.new(rr.outputs[0], bg.inputs[0])
    bg.inputs[1].default_value = 1.0
    scene.world = w

    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 24
    scene.cycles.use_denoising = True
    scene.cycles.max_bounces = 5
    scene.render.resolution_x, scene.render.resolution_y = 1024, 640
    scene.render.image_settings.file_format = 'PNG'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'None'
    scene.cycles.use_adaptive_sampling = True
    scene.cycles.adaptive_threshold = .03
    scene.view_settings.exposure = -.1 if MODE == 'mat' else 0

    if MODE == 'clay':
        cm = bpy.data.materials.new('Clay')
        cm.use_nodes = True
        cp = cm.node_tree.nodes.get('Principled BSDF')
        cp.inputs['Base Color'].default_value = (.5, .5, .49, 1)
        cp.inputs['Roughness'].default_value = .85
        scene.view_layers[0].material_override = cm
    if MODE == 'back':
        cm = bpy.data.materials.new('Back')
        cm.use_nodes = True
        t = cm.node_tree
        for nd in list(t.nodes):
            t.nodes.remove(nd)
        out = t.nodes.new('ShaderNodeOutputMaterial')
        geo = t.nodes.new('ShaderNodeNewGeometry')
        mix = t.nodes.new('ShaderNodeMixRGB')
        mix.inputs[1].default_value = (.5, .5, .5, 1)
        mix.inputs[2].default_value = (1, 0, 0, 1)
        diff = t.nodes.new('ShaderNodeBsdfDiffuse')
        t.links.new(geo.outputs['Backfacing'], mix.inputs[0])
        t.links.new(mix.outputs[0], diff.inputs[0])
        t.links.new(diff.outputs[0], out.inputs[0])
        scene.view_layers[0].material_override = cm
        scene.cycles.samples = 8

    if MODE == 'wire':
        cm = bpy.data.materials.new('Wire')
        cm.use_nodes = True
        t = cm.node_tree
        for nd in list(t.nodes):
            t.nodes.remove(nd)
        out = t.nodes.new('ShaderNodeOutputMaterial')
        wf = t.nodes.new('ShaderNodeWireframe')
        wf.use_pixel_size = True
        wf.inputs[0].default_value = .8
        mix = t.nodes.new('ShaderNodeMixRGB')
        mix.inputs[1].default_value = (.62, .62, .6, 1)
        mix.inputs[2].default_value = (.03, .05, .09, 1)
        diff = t.nodes.new('ShaderNodeBsdfDiffuse')
        t.links.new(wf.outputs[0], mix.inputs[0])
        t.links.new(mix.outputs[0], diff.inputs[0])
        t.links.new(diff.outputs[0], out.inputs[0])
        scene.view_layers[0].material_override = cm
        scene.cycles.samples = 12
        scene.cycles.use_denoising = False
        gob.hide_render = True

    # Bare forhåndsvisning av innsiden: en naken pære, så rommet kan leses. Lys settes i Unreal.
    bulb = bpy.data.lights.new('LGT_Preview_Bulb', 'POINT')
    bulb.energy = 260
    bulb.shadow_soft_size = .08
    bulb.use_temperature = True
    bulb.temperature = 3000
    bob = bpy.data.objects.new('LGT_Preview_Bulb', bulb)
    bob.location = (1.8, -1.5, 3.2)
    scene.collection.objects.link(bob)

    cam = bpy.data.cameras.new('Cam')
    cob = bpy.data.objects.new('Cam', cam)
    scene.collection.objects.link(cob)
    scene.camera = cob
    tag = f'_lod{LOD_VIEW}' if LOD_VIEW else ''
    for view, (loc, tgt, lens) in CAMS[BUILD].items():
        if VIEWS and view not in VIEWS:
            continue
        cam.lens = lens
        cam.clip_start = .05
        cob.location = loc
        cob.rotation_mode = 'QUATERNION'
        cob.rotation_quaternion = (Vector(tgt) - Vector(loc)).to_track_quat('-Z', 'Y')
        bob.hide_render = view != 'inside'
        scene.view_settings.exposure = (1.2 if view == 'inside' else -.1) if MODE == 'mat' else 0
        scene.render.filepath = str(RENDER_DIR / f'{view}{tag}.png')
        print('RENDER', view, flush=True)
        bpy.ops.render.render(write_still=True)


main()
