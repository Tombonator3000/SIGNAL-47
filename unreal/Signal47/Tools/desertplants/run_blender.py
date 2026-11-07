"""Blender-driver: bygger plantene, eksporterer FBX, rendrer og leser tilbake.

blender -b -t 4 --python-exit-code 1 --python run_blender.py -- <utrot> <modus> [plante,plante] [visning,visning]

Moduser: clay (grå med atlasets alpha), mat (materialer), wire, lods (LOD 0/1/2 side om side),
wind (full vind i FLORA_WPO, ved siden av ro), export, verify. Plante: artsnavn eller cholla_a; 'all' er alle.
Akser og FBX-valg følger farmbuildings/townbuildings (meter, Z opp, -Y forward, FBX_SCALE_UNITS).
UV-kanalene heter UVChannel_1 og UVChannel_2, for da beholder Unreals FBX-import rekkefølgen.
"""
import importlib
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.dont_write_bytecode = True
from spec import ATLAS, BUDGET, MATERIAL, SPEC  # noqa: E402

argv = sys.argv[sys.argv.index('--') + 1:]
ROOT, MODE = Path(argv[0]), argv[1]
WHAT = argv[2] if len(argv) > 2 else 'all'
VIEWS = argv[3].split(',') if len(argv) > 3 else None
TEX = ROOT / 'textures'
FBX = ROOT / 'fbx'
BLEND = ROOT / 'blend'
OUT = ROOT / 'renders' / MODE
for d in (OUT, FBX, BLEND):
    d.mkdir(parents=True, exist_ok=True)
NAMES = [n for n in SPEC if WHAT == 'all' or n == WHAT or SPEC[n]['species'] in WHAT.split(',') or n in WHAT.split(',')]
LOD_SUFFIX = ('', '_LOD1', '_LOD2')


def kit(name, lod):
    species, variant = name.split('_')
    return importlib.import_module(species).build(variant, lod)


# ---------- materialer ----------

def material(species, clay):
    name = MATERIAL[species] + ('_Clay' if clay else '')
    if name in bpy.data.materials:
        return bpy.data.materials[name]
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    p = nt.nodes.get('Principled BSDF')
    short = species.capitalize()
    bc = nt.nodes.new('ShaderNodeTexImage')
    bc.image = bpy.data.images.load(str(TEX / f'T_DP_{short}_BC.png'), check_existing=True)
    vc = nt.nodes.new('ShaderNodeVertexColor')
    vc.layer_name = 'Col'
    if clay:
        p.inputs['Base Color'].default_value = (.42, .42, .42, 1)
        p.inputs['Roughness'].default_value = .85
    else:
        mult = nt.nodes.new('ShaderNodeMixRGB')
        mult.blend_type = 'MULTIPLY'
        mult.inputs[0].default_value = 1
        nt.links.new(bc.outputs['Color'], mult.inputs[1])
        nt.links.new(vc.outputs['Color'], mult.inputs[2])
        nt.links.new(mult.outputs[0], p.inputs['Base Color'])
        r = nt.nodes.new('ShaderNodeTexImage')
        r.image = bpy.data.images.load(str(TEX / f'T_DP_{short}_R.png'), check_existing=True)
        r.image.colorspace_settings.name = 'Non-Color'
        sep = nt.nodes.new('ShaderNodeSeparateColor')
        nt.links.new(r.outputs['Color'], sep.inputs[0])
        nt.links.new(sep.outputs[1], p.inputs['Roughness'])
    p.inputs['Specular IOR Level'].default_value = .3
    # Som i M_WebFlora_*: en bakside tar lyset som framsiden. Cycles snur normalen på baksider, her snus den tilbake.
    geo = nt.nodes.new('ShaderNodeNewGeometry')
    sign = nt.nodes.new('ShaderNodeMath')
    sign.operation = 'MULTIPLY_ADD'
    sign.inputs[1].default_value = -2
    sign.inputs[2].default_value = 1
    nt.links.new(geo.outputs['Backfacing'], sign.inputs[0])
    vm = nt.nodes.new('ShaderNodeVectorMath')
    vm.operation = 'SCALE'
    nt.links.new(geo.outputs['Normal'], vm.inputs[0])
    nt.links.new(sign.outputs[0], vm.inputs[3])
    nt.links.new(vm.outputs[0], p.inputs['Normal'])
    if ATLAS[species]['masked']:
        # Samme klipp som M_WebFlora_Card (alphaTest 0,42).
        gt = nt.nodes.new('ShaderNodeMath')
        gt.operation = 'GREATER_THAN'
        gt.inputs[1].default_value = .42
        nt.links.new(bc.outputs['Alpha'], gt.inputs[0])
        nt.links.new(gt.outputs[0], p.inputs['Alpha'])
    return m


def to_object(k, name, species, clay=False):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(k.v, [], k.f)
    mesh.update()
    mesh.materials.append(material(species, clay))
    uv0 = mesh.uv_layers.new(name='UVChannel_1')
    uv1 = mesh.uv_layers.new(name='UVChannel_2')
    col = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    normals = []
    for li, loop in enumerate(mesh.loops):
        i = loop.vertex_index
        uv0.data[li].uv = k.uv[i]
        uv1.data[li].uv = (k.s[i], 0.0)
        c = k.c[i]
        col.data[li].color = (min(1, c[0]), min(1, c[1]), min(1, c[2]), 1)
        normals.append(k.n[i])
    for p in mesh.polygons:
        p.use_smooth = True
    mesh.normals_split_custom_set(normals)
    uv0.active_render = True
    mesh.uv_layers.active_index = 0
    mesh.validate(clean_customdata=False)
    return bpy.data.objects.new(name, mesh)


# ---------- scene ----------

def new_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    s = bpy.context.scene
    s.render.engine = 'CYCLES'
    s.cycles.device = 'CPU'
    s.cycles.samples = 96
    s.cycles.use_denoising = False  # Denoiseren smører tynne kvister og pigger til skyer.
    s.cycles.transparent_max_bounces = 24
    s.cycles.max_bounces = 4
    s.cycles.sample_clamp_direct = 3  # Ingen hvite prikker fra de flippede normalene.
    s.cycles.sample_clamp_indirect = 1.5
    s.view_settings.view_transform = 'AgX'
    s.view_settings.look = 'None'
    s.render.film_transparent = False
    world = bpy.data.worlds.new('sky')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs[0].default_value = (.62, .68, .78, 1)
    world.node_tree.nodes['Background'].inputs[1].default_value = .7
    s.world = world
    sun = bpy.data.objects.new('sun', bpy.data.lights.new('sun', 'SUN'))
    sun.data.energy = 3.4
    sun.data.angle = math.radians(1.5)
    sun.data.color = (1, .94, .84)
    sun.rotation_euler = (math.radians(55), 0, math.radians(35))
    s.collection.objects.link(sun)
    # Rød-brun caliche-bakke, slik referansene viser den.
    bpy.ops.mesh.primitive_plane_add(size=120)
    g = bpy.context.object
    g.name = 'bakke'
    gm = bpy.data.materials.new('bakke')
    gm.use_nodes = True
    gp = gm.node_tree.nodes['Principled BSDF']
    gp.inputs['Roughness'].default_value = .95
    n = gm.node_tree.nodes.new('ShaderNodeTexNoise')
    n.inputs['Scale'].default_value = 8
    n.inputs['Detail'].default_value = 8
    ramp = gm.node_tree.nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].color = (.32, .19, .12, 1)
    ramp.color_ramp.elements[1].color = (.52, .35, .24, 1)
    ramp.color_ramp.elements[0].position = .3
    ramp.color_ramp.elements[1].position = .7
    gm.node_tree.links.new(n.outputs[0], ramp.inputs[0])
    gm.node_tree.links.new(ramp.outputs[0], gp.inputs['Base Color'])
    g.data.materials.append(gm)
    return s


def person(s, x, y):
    """Målestokk: 1,75 m høy grå stolpe med hode."""
    bpy.ops.mesh.primitive_cylinder_add(radius=.2, depth=1.5, location=(x, y, .75), vertices=12)
    a = bpy.context.object
    bpy.ops.mesh.primitive_uv_sphere_add(radius=.125, location=(x, y, 1.625), segments=12, ring_count=8)
    b = bpy.context.object
    m = bpy.data.materials.new('mal')
    m.use_nodes = True
    m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (.08, .09, .12, 1)
    a.data.materials.append(m)
    b.data.materials.append(m)


def camera(s, loc, tgt, lens, res=(1200, 900)):
    if 'cam' in bpy.data.objects:
        cam = bpy.data.objects['cam']
    else:
        cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
        s.collection.objects.link(cam)
    cam.location = loc
    cam.rotation_euler = (Vector(tgt) - Vector(loc)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens = lens
    cam.data.sensor_width = 36
    s.camera = cam
    s.render.resolution_x, s.render.resolution_y = res


def shoot(s, path):
    s.render.filepath = str(path)
    s.render.image_settings.file_format = 'PNG'
    bpy.ops.render.render(write_still=True)
    print('RENDER', path, flush=True)


def lineup_views(names, spread=3.4):
    """Plasser plantene på rekke. Returnerer x-posisjoner."""
    return {n: i * spread - (len(names) - 1) * spread / 2 for i, n in enumerate(names)}


def render_set(clay=False, wire=False):
    species = sorted({SPEC[n]['species'] for n in NAMES})
    for sp in species:
        names = [n for n in NAMES if SPEC[n]['species'] == sp]
        s = new_scene()
        if wire:
            s.cycles.samples = 12
        pos = lineup_views(names)
        for n in names:
            ob = to_object(kit(n, 0), SPEC[n]['mesh'], sp, clay)
            ob.location = (pos[n], 0, 0)
            s.collection.objects.link(ob)
            if wire:
                _wire_mat(ob)
        person(s, max(pos.values()) + 2.4, .2)
        tall = max(SPEC[n]['height'][1] for n in names)
        views = VIEWS or ['line', 'close', 'far']
        if 'line' in views:
            w = len(names) * 3.4 + 2.5
            camera(s, (0, -w * 1.25, 1.6), (0, 0, tall * .45), 50, (1500, 800))
            shoot(s, OUT / f'{sp}_line.png')
        if 'close' in views or 'rear' in views:
            for n in names:
                if 'close' in views:
                    camera(s, (pos[n] - 2.5, -4.3, 1.6), (pos[n], 0, SPEC[n]['height'][1] * .46), 50, (1000, 1000))
                    shoot(s, OUT / f'{n}_close.png')
                if 'rear' in views:
                    camera(s, (pos[n] + 3.2, 4.0, 1.1), (pos[n], 0, SPEC[n]['height'][1] * .46), 50, (1000, 1000))
                    shoot(s, OUT / f'{n}_rear.png')
        if 'macro' in views:
            n = names[0]
            camera(s, (pos[n] - .9, -1.25, 1.4), (pos[n], 0, SPEC[n]['height'][1] * .62), 50, (1000, 1000))
            shoot(s, OUT / f'{n}_macro.png')
        if 'far' in views:
            camera(s, (0, -30, 1.6), (0, 0, 1), 85, (1500, 500))
            shoot(s, OUT / f'{sp}_far.png')


def _wire_mat(ob):
    m = ob.data.materials[0].copy()
    nt = m.node_tree
    p = nt.nodes.get('Principled BSDF')
    w = nt.nodes.new('ShaderNodeWireframe')
    w.use_pixel_size = True
    w.inputs[0].default_value = 1.0
    mix = nt.nodes.new('ShaderNodeMixRGB')
    mix.blend_type = 'MIX'
    mix.inputs[1].default_value = (.55, .55, .55, 1)
    mix.inputs[2].default_value = (0, 0, 0, 1)
    nt.links.new(w.outputs[0], mix.inputs[0])
    nt.links.new(mix.outputs[0], p.inputs['Base Color'])
    ob.data.materials[0] = m


def render_lods():
    for n in NAMES:
        sp = SPEC[n]['species']
        s = new_scene()
        s.cycles.samples = 32
        for lod in range(3):
            ob = to_object(kit(n, lod), f'{SPEC[n]["mesh"]}{LOD_SUFFIX[lod]}', sp)
            ob.location = ((lod - 1) * 3.4, 0, 0)
            s.collection.objects.link(ob)
        camera(s, (0, -11, 1.6), (0, 0, SPEC[n]['height'][1] * .46), 50, (1500, 700))
        shoot(s, OUT / f'{n}_lods.png')
        camera(s, (0, -60, 1.6), (0, 0, 1), 85, (1500, 300))
        shoot(s, OUT / f'{n}_lods_far.png')


def render_wind():
    """Rene hjørneflytt lik FLORA_WPO: wind = dir * (sway * amp). Viser om noe løsner."""
    for n in NAMES:
        sp = SPEC[n]['species']
        s = new_scene()
        s.cycles.samples = 32
        k = kit(n, 0)
        ob = to_object(k, SPEC[n]['mesh'], sp)
        ob.location = (-1.8, 0, 0)
        s.collection.objects.link(ob)
        k2 = kit(n, 0)
        amp = .5  # Langt over spillets 0,07-0,1 m, for å se at ingenting løsner.
        k2.v = [(v[0] + sw * amp, v[1], v[2]) for v, sw in zip(k2.v, k2.s)]
        ob2 = to_object(k2, SPEC[n]['mesh'] + '_wind', sp)
        ob2.location = (1.8, 0, 0)
        s.collection.objects.link(ob2)
        camera(s, (0, -8, 1.6), (0, 0, SPEC[n]['height'][1] * .46), 50, (1400, 800))
        shoot(s, OUT / f'{n}_wind.png')


# ---------- eksport ----------

def export():
    report = {}
    for n in NAMES:
        sp = SPEC[n]['species']
        rows = []
        bpy.ops.wm.read_factory_settings(use_empty=True)
        s = bpy.context.scene
        for lod in range(3):
            k = kit(n, lod)
            name = SPEC[n]['mesh'] + LOD_SUFFIX[lod]
            ob = to_object(k, name, sp)
            s.collection.objects.link(ob)
            ob.select_set(True)
            bpy.context.view_layer.objects.active = ob
            path = FBX / f'{name}.fbx'
            bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH'}, apply_unit_scale=True,
                                     apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Y', axis_up='Z', bake_anim=False,
                                     add_leaf_bones=False, mesh_smooth_type='FACE', use_mesh_modifiers=True, colors_type='LINEAR',
                                     path_mode='RELATIVE')
            ob.select_set(False)
            lo, hi = k.bounds()
            rows.append({'lod_index': lod, 'name': name, 'fbx': f'fbx/{name}.fbx', 'triangles': k.tris(),
                         'materials': [MATERIAL[sp]], 'bounds_local_m': [list(lo), list(hi)], 'sway': [min(k.s), max(k.s)]})
            if k.tris() >= BUDGET[lod]:
                raise RuntimeError(f'{name}: {k.tris()} trekanter, budsjett {BUDGET[lod]}')
        report[SPEC[n]['mesh']] = {'species': sp, 'lods': rows}
        bpy.ops.wm.save_as_mainfile(filepath=str(BLEND / f'{SPEC[n]["mesh"]}.blend'), compress=True)
    path = ROOT / 'model_contract.json'
    old = json.loads(path.read_text()) if path.exists() else {'parts': {}}
    old['parts'].update(report)
    old['coordinates'] = 'Blender meter (x,y,z) til Unreal cm (x,-y,z)*100; rot i (0,0,0), røtter under bakken.'
    path.write_text(json.dumps(old, indent=1))
    mats = {MATERIAL[sp]: {'base_color': f'textures/T_DP_{sp.capitalize()}_BC.png', 'roughness': f'textures/T_DP_{sp.capitalize()}_R.png',
                           'masked': ATLAS[sp]['masked'], 'alpha_clip': .42, 'master': 'M_WebFlora_Card' if ATLAS[sp]['masked'] else 'M_WebFlora_Succulent'} for sp in {SPEC[n]['species'] for n in SPEC}}
    (ROOT / 'materials.json').write_text(json.dumps(mats, indent=1))


def verify():
    """Importer hver FBX på nytt i tom scene og mål. Skriver reimport.json."""
    out = {}
    all_ok = True
    for n in NAMES:
        spec = SPEC[n]
        for lod in range(3):
            name = spec['mesh'] + LOD_SUFFIX[lod]
            bpy.ops.wm.read_factory_settings(use_empty=True)
            bpy.ops.import_scene.fbx(filepath=str(FBX / f'{name}.fbx'), colors_type='LINEAR')
            obs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
            ob = obs[0]
            mesh = ob.data
            mesh.calc_loop_triangles()
            tris = len(mesh.loop_triangles)
            vs = [ob.matrix_world @ v.co for v in mesh.vertices]
            lo = [min(v[i] for v in vs) for i in range(3)]
            hi = [max(v[i] for v in vs) for i in range(3)]
            uvs = [u.name for u in mesh.uv_layers]
            sway = [d.uv[0] for d in mesh.uv_layers[1].data] if len(uvs) > 1 else []
            cols = mesh.color_attributes[0].data if mesh.color_attributes else []
            alphas = [c.color[3] for c in cols]
            # Vind skal være 0 i roten og størst høyt oppe: sammenheng med høyde over bakken.
            lowest = [d.uv[0] for d, lp in zip(mesh.uv_layers[1].data, mesh.loops) if (ob.matrix_world @ mesh.vertices[lp.vertex_index].co)[2] < .03] if sway else []
            row = {'objects': [o.name for o in obs], 'tris': tris, 'bounds_m': [lo, hi], 'size_m': [hi[i] - lo[i] for i in range(3)],
                   'uv': uvs, 'sway': [min(sway), max(sway)] if sway else None, 'sway_near_ground_max': max(lowest) if lowest else None,
                   'colors': bool(mesh.color_attributes), 'alpha_min': min(alphas) if alphas else None,
                   'materials': [m.name for m in mesh.materials], 'normals_custom': mesh.has_custom_normals}
            ok = (len(obs) == 1 and uvs[:2] == ['UVChannel_1', 'UVChannel_2'] and tris < BUDGET[lod] and row['colors']
                  and row['sway'] and 0 <= row['sway'][0] and row['sway'][1] <= 1.0 and len(row['materials']) == 1)
            if lod == 0:
                ok = ok and spec['height'][0] <= hi[2] <= spec['height'][1]
                ok = ok and max(hi[0] - lo[0], hi[1] - lo[1]) <= spec['spread']
            row['ok'] = bool(ok)
            all_ok = all_ok and row['ok']
            out[name] = row
            print(name, 'OK' if row['ok'] else 'FEIL', tris, [round(x, 3) for x in row['size_m']], row['sway'], flush=True)
    path = ROOT / 'reimport.json'
    old = json.loads(path.read_text()) if path.exists() else {}
    old.update(out)
    path.write_text(json.dumps(old, indent=1))
    if not all_ok:
        raise RuntimeError('tilbakelesing fant feil')


if MODE == 'clay':
    render_set(clay=True)
elif MODE == 'mat':
    render_set()
elif MODE == 'wire':
    render_set(clay=True, wire=True)
elif MODE == 'lods':
    render_lods()
elif MODE == 'wind':
    render_wind()
elif MODE == 'export':
    export()
elif MODE == 'verify':
    verify()
else:
    raise SystemExit('ukjent modus ' + MODE)
