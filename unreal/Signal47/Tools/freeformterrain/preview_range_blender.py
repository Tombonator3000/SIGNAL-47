"""Blender (Cycles CPU): SM_FF_Range and SM_FF_Ground_Road from the game's own files, at eye height. Run by itself under the lease:

    python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 preview_range_blender.py --run <freeformterrain> <out dir>

The land's own normal map and masks, a soil colour near the game's, a sun of 11:00 or 18:30. It shows shape, foot and haze;
the light of the game is the game's, and only the game can say the material is right.
"""
import json
import math
import os
from pathlib import Path
import subprocess
import sys

VIEWS = [('array', 24, -130, 250), ('farm', 150, 300, 235), ('road_w', 2, 1100, 270), ('road_e', -2, 1100, 90), ('road_n', 0, -1500, 340), ('foot_w', -300, 1400, 280)]
MESA_Z = 2060.0
EYE_Y = {'array': 1.7, 'farm': 1.7}


def inside_blender(root, out):
    import bpy
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 48
    scene.cycles.use_denoising = True
    scene.render.resolution_x, scene.render.resolution_y = 1600, 900
    scene.view_settings.view_transform = 'AgX'
    scene.world = bpy.data.worlds.new('sky')
    scene.world.use_nodes = True
    bg = scene.world.node_tree.nodes['Background']
    bg.inputs[0].default_value = (.55, .68, .9, 1)
    bg.inputs[1].default_value = 1.1

    def load(name):
        bpy.ops.import_scene.gltf(filepath=str(root / 'mesh' / (name + '.glb')))
        return bpy.context.selected_objects[0]
    rng = load('SM_FF_Range')
    ground = load('SM_FF_Ground_Road')
    # glTF to Blender: x, -z, y. The range stands 2060 m along its node's z.
    rng.location = (0, -MESA_Z, 0)

    # ground: flat soil
    m = bpy.data.materials.new('soil')
    m.use_nodes = True
    p = m.node_tree.nodes['Principled BSDF']
    p.inputs['Base Color'].default_value = (.40, .27, .19, 1)
    p.inputs['Roughness'].default_value = .95
    ground.data.materials[0] = m

    m = bpy.data.materials.new('land')
    m.use_nodes = True
    nt = m.node_tree
    p = nt.nodes['Principled BSDF']
    p.inputs['Roughness'].default_value = .92
    uv = nt.nodes.new('ShaderNodeUVMap')
    uv.uv_map = rng.data.uv_layers[0].name
    # glTF import flips v: back to rows of z
    sep = nt.nodes.new('ShaderNodeSeparateXYZ')
    nt.links.new(uv.outputs[0], sep.inputs[0])
    com = nt.nodes.new('ShaderNodeCombineXYZ')
    nt.links.new(sep.outputs[0], com.inputs[0])
    inv = nt.nodes.new('ShaderNodeMath')
    inv.operation = 'SUBTRACT'
    inv.inputs[0].default_value = 1
    nt.links.new(sep.outputs[1], inv.inputs[1])
    nt.links.new(inv.outputs[0], com.inputs[1])
    tex = nt.nodes.new('ShaderNodeTexImage')
    tex.image = bpy.data.images.load(str(root / 'range/T_FF_Range_M.png'))
    tex.image.colorspace_settings.name = 'Non-Color'
    tex.image.alpha_mode = 'CHANNEL_PACKED'
    tex.extension = 'EXTEND'
    nt.links.new(com.outputs[0], tex.inputs[0])
    sp = nt.nodes.new('ShaderNodeSeparateColor')
    nt.links.new(tex.outputs[0], sp.inputs[0])
    # (M: R shrubs, G rock, B cavity; alpha foot)
    grp = nt.nodes.new('ShaderNodeMix')
    grp.data_type = 'RGBA'
    grp.inputs['A'].default_value = (.40, .27, .19, 1)
    grp.inputs['B'].default_value = (.27, .22, .19, 1)
    nt.links.new(sp.outputs[1], grp.inputs['Factor'])
    shr = nt.nodes.new('ShaderNodeMix')
    shr.data_type = 'RGBA'
    shr.inputs['B'].default_value = (.05, .055, .03, 1)
    nt.links.new(grp.outputs['Result'], shr.inputs['A'])
    k = nt.nodes.new('ShaderNodeMath')
    k.operation = 'MULTIPLY'
    k.inputs[1].default_value = .8
    nt.links.new(sp.outputs[0], k.inputs[0])
    nt.links.new(k.outputs[0], shr.inputs['Factor'])
    ao = nt.nodes.new('ShaderNodeMix')
    ao.data_type = 'RGBA'
    ao.blend_type = 'MULTIPLY'
    ao.inputs['Factor'].default_value = 1
    nt.links.new(shr.outputs['Result'], ao.inputs['A'])
    cav = nt.nodes.new('ShaderNodeMapRange')
    cav.inputs['To Min'].default_value = .62
    nt.links.new(sp.outputs[2], cav.inputs['Value'])
    nt.links.new(cav.outputs[0], ao.inputs['B'])
    # haze: as the material's, by distance from the camera
    geo = nt.nodes.new('ShaderNodeNewGeometry')
    length = nt.nodes.new('ShaderNodeVectorMath')
    length.operation = 'DISTANCE'
    cam_at = nt.nodes.new('ShaderNodeCombineXYZ')
    nt.links.new(geo.outputs['Position'], length.inputs[0])
    nt.links.new(cam_at.outputs[0], length.inputs[1])
    a = nt.nodes.new('ShaderNodeMath')
    a.operation = 'SUBTRACT'
    a.inputs[1].default_value = 1500
    nt.links.new(length.outputs['Value'], a.inputs[0])
    b = nt.nodes.new('ShaderNodeMath')
    b.operation = 'DIVIDE'
    b.use_clamp = False
    b.inputs[1].default_value = -7000
    nt.links.new(a.outputs[0], b.inputs[0])
    c = nt.nodes.new('ShaderNodeMath')
    c.operation = 'EXPONENT'
    nt.links.new(b.outputs[0], c.inputs[0])
    d = nt.nodes.new('ShaderNodeMath')
    d.operation = 'SUBTRACT'
    d.inputs[0].default_value = 1
    d.use_clamp = True
    nt.links.new(c.outputs[0], d.inputs[1])
    e = nt.nodes.new('ShaderNodeMath')
    e.operation = 'MULTIPLY'
    e.inputs[1].default_value = .86
    nt.links.new(d.outputs[0], e.inputs[0])
    hz = nt.nodes.new('ShaderNodeMix')
    hz.data_type = 'RGBA'
    hz.inputs['B'].default_value = (.20, .25, .34, 1)
    nt.links.new(ao.outputs['Result'], hz.inputs['A'])
    nt.links.new(e.outputs[0], hz.inputs['Factor'])
    nt.links.new(hz.outputs['Result'], p.inputs['Base Color'])
    rng.data.materials[0] = m

    sun_o = bpy.data.objects.new('sun', bpy.data.lights.new('sun', 'SUN'))
    scene.collection.objects.link(sun_o)
    cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam.data.sensor_width = 36
    cam.data.lens = 36 / (2 * math.tan(math.radians(68) / 2))
    cam.data.clip_end = 40000
    # road height: from the cached ground, by the build's own numbers
    import numpy as np
    G = np.load(root / 'cache/range_G.npy', mmap_mode='r')
    cell, x0, z0 = 24000 / 4096, -13100., -12200.
    for (when, az, alt, power) in (('dag', 150, 58, 3.6), ('skumring', 262, 5, 2.2)):
        sun_o.data.energy = power
        sun_o.rotation_euler = (math.radians(90 - alt), 0, math.radians(180 - az))
        for name, x, z, yaw in VIEWS:
            if when == 'skumring' and name not in ('array', 'road_w'):
                continue
            y = float(G[int((z - z0) / cell), int((x - x0) / cell)]) + 1.7
            cam.location = (x, -z, y)
            cam.rotation_euler = (math.radians(90), 0, math.radians(-yaw))
            cam.data.shift_y = .10
            cam_at.inputs[0].default_value, cam_at.inputs[1].default_value, cam_at.inputs[2].default_value = x, -z, y
            scene.render.filepath = str(out / ('%s_%s.png' % (name, when)))
            bpy.ops.render.render(write_still=True)
            print('rendered', scene.render.filepath, flush=True)


def outside(root, out):
    project = Path.home() / 'SIGNAL-47/unreal/Signal47'
    sys.path.insert(0, str(project / 'Scripts'))
    from production_lease import HeavyReservation
    out.mkdir(parents=True, exist_ok=True)
    with HeavyReservation('FREEFORM range preview') as lease:
        env = os.environ.copy()
        env.update(lease.environment())
        r = subprocess.run(['blender', '-b', '--factory-startup', '-t', '16', '--python', __file__, '--', str(root), str(out)], env=env, capture_output=True, text=True, timeout=2400)
    print(r.stdout[-1500:])
    print(r.stderr[-800:])
    return r.returncode


if __name__ == '__main__':
    if '--run' in sys.argv:
        i = sys.argv.index('--run')
        sys.exit(outside(Path(sys.argv[i + 1]), Path(sys.argv[i + 2])))
    i = sys.argv.index('--')
    inside_blender(Path(sys.argv[i + 1]), Path(sys.argv[i + 2]))
