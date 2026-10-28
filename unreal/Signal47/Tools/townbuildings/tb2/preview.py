"""Felles forhåndsvisning for TB2. Bygger én bygning i Blender og rendrer faste kameravinkler.

blender -b -t 3 --python-exit-code 1 --python tb2/preview.py -- <Bygning> <tb1|tb2> <utmappe> [lod] [visninger]

tb1 bruker town_geometry.<bygning>(lod), tb2 bruker tb2/<bygning>.py sin build(lod).
Visninger: street, oblique, door, roof, dusk (standard: alle). Lagrer også <utmappe>/<Bygning>_<kilde>.blend.
"""
import bpy, sys, json, math, importlib.util
from pathlib import Path
from mathutils import Vector

R = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(R))
argv = sys.argv[sys.argv.index('--') + 1:]
NAME, SOURCE, OUT = argv[0], argv[1], Path(argv[2])
LOD = int(argv[3]) if len(argv) > 3 else 0
VIEWS = argv[4].split(',') if len(argv) > 4 else ['street', 'oblique', 'door', 'roof', 'dusk']
OUT.mkdir(parents=True, exist_ok=True)

from design import BUILDINGS
B = BUILDINGS[NAME]
ROWS = json.loads((R / 'materials.json').read_text())
for extra in sorted((R / 'tb2').glob('materials_*.json')):
    ROWS += json.loads(extra.read_text())
ROW = {r['name']: r for r in ROWS}

if SOURCE == 'tb1':
    import town_geometry
    data = getattr(town_geometry, NAME.lower())(LOD)
else:
    spec = importlib.util.spec_from_file_location('tb2_' + NAME.lower(), R / 'tb2' / (NAME.lower() + '.py'))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    data = mod.build(LOD)

for ob in list(bpy.data.objects):
    bpy.data.objects.remove(ob, do_unlink=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'


def material(name):
    if name in bpy.data.materials:
        return bpy.data.materials[name]
    r = ROW[name]
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    p = nt.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = r['base_color_linear']
    p.inputs['Metallic'].default_value = r['metallic']
    p.inputs['Roughness'].default_value = r['roughness']
    p.inputs['Specular IOR Level'].default_value = r.get('specular', .28)
    p.inputs['Emission Color'].default_value = r.get('emission', [0, 0, 0, 1])
    p.inputs['Emission Strength'].default_value = r.get('emission_strength', 0)
    if r.get('alpha', 1) < 1:
        p.inputs['Alpha'].default_value = r['alpha']
    for role, path in r.get('textures', {}).items():
        t = nt.nodes.new('ShaderNodeTexImage')
        t.image = bpy.data.images.load(str(R / path), check_existing=True)
        t.image.colorspace_settings.name = 'sRGB' if role == 'base_color' else 'Non-Color'
        if role == 'normal':
            sep = nt.nodes.new('ShaderNodeSeparateXYZ'); inv = nt.nodes.new('ShaderNodeMath'); inv.operation = 'SUBTRACT'; inv.inputs[0].default_value = 1
            comb = nt.nodes.new('ShaderNodeCombineXYZ'); nm = nt.nodes.new('ShaderNodeNormalMap'); nm.inputs['Strength'].default_value = r.get('normal_strength', .2)
            nt.links.new(t.outputs['Color'], sep.inputs[0]); nt.links.new(sep.outputs['X'], comb.inputs['X']); nt.links.new(sep.outputs['Y'], inv.inputs[1])
            nt.links.new(inv.outputs[0], comb.inputs['Y']); nt.links.new(sep.outputs['Z'], comb.inputs['Z']); nt.links.new(comb.outputs[0], nm.inputs['Color']); nt.links.new(nm.outputs[0], p.inputs['Normal'])
        elif role == 'base_color':
            mix = nt.nodes.new('ShaderNodeMixRGB'); mix.blend_type = 'MULTIPLY'; mix.inputs[0].default_value = 1
            mix.inputs[2].default_value = r.get('texture_tint_linear', [1, 1, 1, 1])
            nt.links.new(t.outputs['Color'], mix.inputs[1]); nt.links.new(mix.outputs[0], p.inputs['Base Color'])
        else:
            nt.links.new(t.outputs['Color'], p.inputs['Roughness'])
    return m


stats = {}
for name, shape in data.items():
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(shape.v, [], shape.f)
    mesh.update()
    names = sorted(set(shape.mat))
    for mn in names:
        mesh.materials.append(material(mn))
    idx = {mn: i for i, mn in enumerate(names)}
    uvl = mesh.uv_layers.new(name='UVMap')
    for poly, mn, uv, sm in zip(mesh.polygons, shape.mat, shape.uv, shape.smooth):
        poly.material_index = idx[mn]
        poly.use_smooth = bool(sm)
        for li, t in zip(poly.loop_indices, uv):
            uvl.data[li].uv = t
    ob = bpy.data.objects.new(name, mesh)
    scene.collection.objects.link(ob)
    stats[name] = sum(len(f) - 2 for f in shape.f)

# Bakke, himmel og sol: sen ettermiddag i New Mexico.
x0, x1, y0, y1 = B['footprint']; h = B['height']; s = B['front']
cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
fx = x1 if s > 0 else x0
bpy.ops.mesh.primitive_plane_add(size=200, location=(cx, cy, -0.002))
ground = bpy.context.object; ground.name = 'Preview_ground'
gm = bpy.data.materials.new('Preview_caliche'); gm.use_nodes = True
gp = gm.node_tree.nodes.get('Principled BSDF'); gp.inputs['Base Color'].default_value = (.42, .33, .23, 1); gp.inputs['Roughness'].default_value = .95
ground.data.materials.append(gm)

world = bpy.data.worlds.new('Preview_world'); scene.world = world; world.use_nodes = True
sky = world.node_tree.nodes.new('ShaderNodeTexSky')
if hasattr(sky, 'sun_disc'):
    sky.sun_disc = False
bg = world.node_tree.nodes.get('Background')
world.node_tree.links.new(sky.outputs['Color'], bg.inputs['Color'])
sun_data = bpy.data.lights.new('Preview_sun', 'SUN'); sun = bpy.data.objects.new('Preview_sun', sun_data); scene.collection.objects.link(sun)

scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 48
scene.cycles.use_denoising = True
scene.render.resolution_x, scene.render.resolution_y = 1280, 720
scene.view_settings.view_transform = 'AgX'
scene.view_settings.exposure = -.6

cam_data = bpy.data.cameras.new('Preview_cam'); cam = bpy.data.objects.new('Preview_cam', cam_data); scene.collection.objects.link(cam); scene.camera = cam


def look(at_from, at_to, lens=28):
    cam.location = at_from
    d = Vector(at_to) - Vector(at_from)
    cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    cam_data.lens = lens


def lighting(dusk):
    if dusk:
        sun_data.energy = .6; sun.rotation_euler = (math.radians(86), 0, math.radians(250))
        if hasattr(sky, 'sun_elevation'):
            sky.sun_elevation = math.radians(2)
        bg.inputs['Strength'].default_value = .25
    else:
        if hasattr(sky, 'sun_elevation'):
            sky.sun_elevation = math.radians(32)
        sun_data.energy = 3.2; sun.rotation_euler = (math.radians(58), 0, math.radians(215))
        bg.inputs['Strength'].default_value = .45


views = {
    'street': ((fx + s * 13, cy - 4, 1.7), (fx, cy, h * .55), 26),
    'oblique': ((fx + s * 16, y1 + 11 if s > 0 else y0 - 11, 2.4), (cx, cy, h * .5), 24),
    'door': ((fx + s * 4.2, B['door'][0] - 1.3, 1.65), (fx, B['door'][0], 1.6), 24),
    'roof': ((fx + s * 18, cy - 14, h + 12), (cx, cy, h * .4), 24),
    'dusk': ((fx + s * 13, cy + 3, 1.7), (fx, cy, h * .55), 26),
}
for v in VIEWS:
    a, b, lens = views[v]
    look(a, b, lens)
    lighting(v == 'dusk')
    scene.render.filepath = str(OUT / f'{NAME}_{SOURCE}_lod{LOD}_{v}.png')
    bpy.ops.render.render(write_still=True)

(OUT / f'{NAME}_{SOURCE}_lod{LOD}_tris.json').write_text(json.dumps(stats, indent=1))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / f'{NAME}_{SOURCE}_lod{LOD}.blend'), compress=True)
print('PREVIEW_OK', NAME, SOURCE, LOD, stats)
