"""Small Blender previews of the street lamp family (Cycles CPU, few samples). Not game images.

Usage: blender -b -t 2 -P preview.py [-- shot ...]. Ground, sky, sun and the wood pole are
preview context only and are not exported.
"""
from pathlib import Path
import sys, math
import bpy
from mathutils import Vector
HERE = Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(HERE / 'SIGNAL47_StreetLamps.blend'))
ARGS = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
sc = bpy.context.scene; sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = 14
sc.cycles.use_denoising = True; sc.render.threads_mode = 'FIXED'; sc.render.threads = 2
sc.render.resolution_x = 720; sc.render.resolution_y = 540; sc.render.resolution_percentage = 100
sc.view_settings.view_transform = 'AgX'; sc.render.film_transparent = False
for c in bpy.data.collections:
    if '_EDIT_' in c.name: c.hide_render = True
for o in bpy.data.objects:
    if o.type == 'MESH': o.hide_render = True
def xyz(p): return Vector((p[0], -p[2], p[1]))
def aim(o, p): o.rotation_euler = (xyz(p) - o.location).to_track_quat('-Z', 'Y').to_euler()
def mat(name, c, r=.9, m=0.):
    mt = bpy.data.materials.new(name); mt.use_nodes = True; p = mt.node_tree.nodes['Principled BSDF']
    p.inputs['Base Color'].default_value = (*c, 1); p.inputs['Roughness'].default_value = r; p.inputs['Metallic'].default_value = m
    return mt
clay = mat('PREVIEW_clay', (.45, .45, .44), .7)
asphalt = mat('PREVIEW_CONTEXT_ground', (.09, .085, .08), .95)
wood = mat('PREVIEW_CONTEXT_creosote_pole', (.10, .07, .045), .85)
world = bpy.data.worlds.new('PREVIEW_sky'); world.use_nodes = True; sc.world = world
bg = world.node_tree.nodes['Background']
def sky(col, s): bg.inputs['Color'].default_value = (*col, 1); bg.inputs['Strength'].default_value = s
bpy.ops.mesh.primitive_plane_add(size=60, location=(0, 0, 0)); ground = bpy.context.object; ground.name = 'PREVIEW_CONTEXT_ground'
ground.data.materials.append(asphalt)
bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=.13, depth=9, location=(-.13, 0, 4.5)); wp = bpy.context.object
wp.name = 'PREVIEW_CONTEXT_wood_pole'; wp.data.materials.append(wood); wp.hide_render = True
sun_d = bpy.data.lights.new('PREVIEW_sun', 'SUN'); sun_d.energy = 3.2; sun_d.angle = math.radians(2)
sun = bpy.data.objects.new('PREVIEW_sun', sun_d); sc.collection.objects.link(sun)
sun.rotation_euler = (math.radians(50), math.radians(10), math.radians(-35))
cd = bpy.data.cameras.new('PREVIEW_cam'); cam = bpy.data.objects.new('PREVIEW_cam', cd); sc.collection.objects.link(cam); sc.camera = cam
O = {n: bpy.data.objects[n] for n in ('SM_StreetLamp_Head', 'SM_StreetLamp_Arm', 'SM_StreetLamp_Pole', 'SM_StreetLamp_WoodArm')}
def show(*names, at=None):
    for o in O.values(): o.hide_render = True; o.location = (0, 0, 0)
    for n in names: O[n].hide_render = False; O[n].hide_set(False)
    for n, p in (at or {}).items(): O[n].location = xyz(p)
def shoot(fname, pos, target, lens=50, ortho=None, clay_on=False, ground_on=True):
    cd.type = 'ORTHO' if ortho else 'PERSP'
    if ortho: cd.ortho_scale = ortho
    cd.lens = lens; cd.clip_start = .01; cam.location = xyz(pos); aim(cam, target)
    sc.view_layers[0].material_override = clay if clay_on else None
    ground.hide_render = not ground_on
    out = HERE / 'Preview'; out.mkdir(exist_ok=True); sc.render.filepath = str(out / fname)
    bpy.ops.render.render(write_still=True); print('SHOT', fname)
HEAD_AT = (1.80, 8.05, 0)
ASM = {'SM_StreetLamp_Arm': (0, 7.60, 0), 'SM_StreetLamp_Head': HEAD_AT}
shots = {
 'head-clay-34.png': lambda: (show('SM_StreetLamp_Head'), shoot('head-clay-34.png', (1.25, .55, .95), (.38, 0, 0), 50, clay_on=True, ground_on=False)),
 'head-clay-side.png': lambda: (show('SM_StreetLamp_Head'), shoot('head-clay-side.png', (.40, .0, 3.0), (.40, .0, 0), ortho=.95, clay_on=True, ground_on=False)),
 'head-mat-below.png': lambda: (show('SM_StreetLamp_Head'), shoot('head-mat-below.png', (1.35, -.85, .85), (.40, -.06, 0), 45, ground_on=False)),
 'head-mat-34.png': lambda: (show('SM_StreetLamp_Head'), shoot('head-mat-34.png', (-.45, .55, 1.05), (.36, 0, 0), 45, ground_on=False)),
 'arm-clay.png': lambda: (show('SM_StreetLamp_Arm'), shoot('arm-clay.png', (.9, .25, 3.4), (.9, .22, 0), ortho=2.2, clay_on=True, ground_on=False)),
 'assembly-clay.png': lambda: (show('SM_StreetLamp_Pole', 'SM_StreetLamp_Arm', 'SM_StreetLamp_Head', at=ASM),
                               shoot('assembly-clay.png', (7.0, 1.6, 9.5), (.9, 4.3, 0), 30, clay_on=True)),
 'assembly-day.png': lambda: (show('SM_StreetLamp_Pole', 'SM_StreetLamp_Arm', 'SM_StreetLamp_Head', at=ASM),
                              shoot('assembly-day.png', (7.0, 1.6, 9.5), (.9, 4.3, 0), 30)),
 'assembly-top-detail.png': lambda: (show('SM_StreetLamp_Pole', 'SM_StreetLamp_Arm', 'SM_StreetLamp_Head', at=ASM),
                                     shoot('assembly-top-detail.png', (3.6, 6.6, 2.9), (1.1, 7.85, 0), 40)),
 'pole-foot.png': lambda: (show('SM_StreetLamp_Pole'), shoot('pole-foot.png', (-.9, .75, .7), (0, .2, 0), 40)),
 'woodarm-day.png': lambda: (show('SM_StreetLamp_WoodArm', 'SM_StreetLamp_Head', at={'SM_StreetLamp_WoodArm': (0, 7.0, 0), 'SM_StreetLamp_Head': (1.20, 7.0, 0)}),
                             setattr(wp, 'hide_render', False), shoot('woodarm-day.png', (2.4, 6.2, 2.6), (.55, 6.85, 0), 40), setattr(wp, 'hide_render', True)),
}
def dusk():
    show('SM_StreetLamp_Pole', 'SM_StreetLamp_Arm', 'SM_StreetLamp_Head', at=ASM)
    sky((.05, .07, .13), .35); sun_d.energy = .05
    lens = bpy.data.materials['M_StreetLamp_Lens'].node_tree.nodes['Principled BSDF']
    lens.inputs['Emission Color'].default_value = (1., .50, .16, 1); lens.inputs['Emission Strength'].default_value = 8.
    sd = bpy.data.lights.new('PREVIEW_HPS', 'SPOT'); sd.energy = 2600; sd.spot_size = math.radians(140); sd.spot_blend = .6
    sd.use_temperature = True if hasattr(sd, 'use_temperature') else False
    if hasattr(sd, 'temperature'): sd.temperature = 2100
    else: sd.color = (1., .55, .2)
    s = bpy.data.objects.new('PREVIEW_HPS', sd); sc.collection.objects.link(s)
    s.location = xyz((HEAD_AT[0] + .42, HEAD_AT[1] - .175, 0)); s.rotation_euler = (0, 0, 0)
    shoot('assembly-dusk-lit.png', (7.0, 1.6, 9.5), (.9, 4.3, 0), 30)
sky((.50, .60, .78), .9)
for k, f in shots.items():
    if not ARGS or k in ARGS: f()
if not ARGS or 'assembly-dusk-lit.png' in ARGS: dusk()
print('STREETLAMP PREVIEWS COMPLETE')
