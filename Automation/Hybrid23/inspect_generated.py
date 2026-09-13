"""Blender CLI: preserve and inspect an image-generated candidate, without claiming reconstruction."""
import bpy
import json
import sys
from pathlib import Path
from mathutils import Vector

args = sys.argv[sys.argv.index('--')+1:]
source, out = map(lambda p: Path(p).resolve(), args)
out.mkdir(parents=True, exist_ok=False)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source))
meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
points = [o.matrix_world @ Vector(p) for o in meshes for p in o.bound_box]
low = Vector(tuple(min(p[i] for p in points) for i in range(3)))
high = Vector(tuple(max(p[i] for p in points) for i in range(3)))
audit = {'source': source.name, 'bounds_min': list(low), 'bounds_max': list(high),
         'dimensions': list(high-low), 'objects': [
             {'name': o.name, 'vertices': len(o.data.vertices), 'polygons': len(o.data.polygons),
              'materials': [m.name for m in o.data.materials]} for o in meshes],
         'images': [{'name': i.name, 'width': i.size[0], 'height': i.size[1]} for i in bpy.data.images]}
(out/'audit.json').write_text(json.dumps(audit, indent=2)+'\n')
scene = bpy.context.scene
scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 16
scene.cycles.use_denoising = True
scene.render.threads_mode = 'FIXED'
scene.render.threads = 4
scene.render.resolution_x = 960
scene.render.resolution_y = 720
scene.render.resolution_percentage = 100
scene.world = bpy.data.worlds.new('Neutral inspection world')
scene.world.use_nodes = True
scene.world.node_tree.nodes['Background'].inputs[0].default_value = (.4,.4,.4,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value = .7
sun = bpy.data.lights.new('Inspection fill','SUN')
sun.energy = 2
light = bpy.data.objects.new('Inspection fill',sun)
scene.collection.objects.link(light)
light.rotation_euler = (.45,-.6,-.4)
camera = bpy.data.objects.new('Inspection camera',bpy.data.cameras.new('Inspection camera'))
scene.collection.objects.link(camera)
scene.camera = camera
camera.data.type = 'ORTHO'
camera.data.ortho_scale = max(high-low)*1.65
center = (low+high)/2
for name, offset in [('front',(9,-12,7)),('rear',(-9,12,7))]:
    camera.location = center+Vector(offset)
    camera.rotation_euler = (center-camera.location).to_track_quat('-Z','Y').to_euler()
    scene.render.filepath = str(out/(name+'.png'))
    bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'inspected.blend'))
print(json.dumps(audit))
