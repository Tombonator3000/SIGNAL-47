"""Root-run small PBR source/LOD previews. Never save over a model source."""
from pathlib import Path
import argparse
import json
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import prepare


def main():
    import bpy
    import numpy as np
    from mathutils import Vector

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sources', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--stage', choices=('raw', 'prepared'), default='raw')
    parser.add_argument('--only')
    parser.add_argument('--views', default='overview,front,rear,side,top')
    parser.add_argument('--resolution', type=int, default=480)
    options = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    only = set(options.only.split(',')) if options.only else None
    root, models = prepare.read_sources(options.sources.expanduser().resolve(), only)
    views = options.views.split(',')
    directions = {'overview': (1, -1.6, .85), 'front': (0, -1, .12),
                  'rear': (0, 1, .12), 'side': (1, 0, .12), 'top': (0, -.001, 1)}
    if options.stage == 'prepared':
        directions.update(overview=(-1, 1.6, .85), front=(0, 1, .12), rear=(0, -1, .12), driver=(0, 1, -.08))
    prepare.require(set(views) <= set(directions), 'Unknown preview view')
    prepare.require(128 <= options.resolution <= 1024, 'Preview resolution must be128..1024')
    output = options.output.expanduser().resolve()
    prepare.require(not output.is_relative_to(prepare.REPO), 'Previews remain private')
    output.mkdir(parents=True, exist_ok=True)
    report = {'stage': options.stage, 'purpose': 'PBR asset shape/axis inspection; not game runtime', 'models': []}

    def aim(obj, at):
        obj.rotation_euler = (Vector(at) - obj.location).to_track_quat('-Z', 'Y').to_euler()

    for row in models:
        if options.stage == 'raw':
            bpy.ops.wm.read_factory_settings(use_empty=True)
            bpy.ops.import_scene.gltf(filepath=row['source_glb'], import_pack_images=True)
        else:
            bpy.ops.wm.open_mainfile(filepath=str(root / row['id'] / 'prepared.blend'))
        scene = bpy.context.scene
        for obj in list(scene.objects):
            if obj.type in {'LIGHT', 'CAMERA'}:
                bpy.data.objects.remove(obj, do_unlink=True)
        meshes = [o for o in scene.objects if o.type == 'MESH' and not o.hide_render]
        points = prepare.xyz(meshes)
        low, high = points.min(axis=0), points.max(axis=0)
        center = Vector((low + high) / 2)
        span = float(max(high - low))
        prepare.require(span > 0 and np.isfinite(points).all(), 'Invalid preview bounds')
        scene.render.engine = 'CYCLES'
        scene.cycles.device = 'CPU'
        scene.cycles.samples = 8
        scene.cycles.use_denoising = True
        scene.cycles.max_bounces = 3
        scene.cycles.diffuse_bounces = 1
        scene.cycles.glossy_bounces = 2
        scene.render.threads_mode = 'FIXED'
        scene.render.threads = 2
        scene.render.resolution_x = scene.render.resolution_y = options.resolution
        scene.render.resolution_percentage = 100
        scene.render.image_settings.file_format = 'PNG'
        scene.world = bpy.data.worlds.new('Tripo preview world')
        scene.world.use_nodes = True
        background = scene.world.node_tree.nodes.get('Background')
        background.inputs['Color'].default_value = (.12, .14, .18, 1)
        background.inputs['Strength'].default_value = .35
        scene.view_settings.view_transform = 'AgX'
        scene.view_settings.exposure = 0
        for label, offset, energy, size in [('Key', (1, -1.2, 1.5), 650, 1.3),
                                           ('Fill', (-1.5, -.2, .7), 400, 1.8),
                                           ('Rim', (.3, 1.3, 1), 700, 1.1)]:
            light = bpy.data.lights.new('Preview' + label, 'AREA')
            light.energy = energy * span * span
            light.shape = 'DISK'
            light.size = size * span
            obj = bpy.data.objects.new(light.name, light)
            scene.collection.objects.link(obj)
            obj.location = center + Vector(offset) * span
            aim(obj, center)
        camera = bpy.data.cameras.new('Tripo inspection camera')
        camera.type = 'ORTHO'
        camera.ortho_scale = span * 1.35
        camera.clip_start = max(.001, span * .001)
        camera.clip_end = span * 20
        obj = bpy.data.objects.new(camera.name, camera)
        scene.collection.objects.link(obj)
        scene.camera = obj
        images = []
        for view in views:
            if view == 'driver':
                eye = row.get('driver_eye_three_m')
                prepare.require(eye is not None, 'driver view requires an inspected driver_eye_three_m')
                camera.type = 'PERSP';camera.lens = 24
                obj.location = Vector((eye[0], -eye[2], eye[1]))
                aim(obj, obj.location + Vector((0, 3, -.24)))
            else:
                camera.type = 'ORTHO'
                obj.location = center + Vector(directions[view]).normalized() * span * 3
                aim(obj, center)
            path = output / (row['id'] + '-' + options.stage + '-' + view + '.png')
            scene.render.filepath = str(path)
            bpy.ops.render.render(write_still=True)
            images.append({'view': view, 'path': str(path)})
        report['models'].append({'id': row['id'], 'source_glb': row['source_glb'],
                                 'bounds_blender': [low.tolist(), high.tolist()], 'images': images})
        prepare.write_json(output / 'preview.json', report)
        print('S47_TRIPO_PREVIEW', row['id'], [v['view'] for v in images], flush=True)


if __name__ == '__main__':
    main()
