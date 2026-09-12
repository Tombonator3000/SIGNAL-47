"""Run with Blender --background --factory-startup --python this.py -- GLB JSON."""
import json
from pathlib import Path
import sys

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

source, output = map(Path, sys.argv[sys.argv.index('--')+1:])
if output.exists():
    raise RuntimeError('Use a fresh report path')
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source.resolve()))
objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
vertices, faces = [], []
for obj in objects:
    offset = len(vertices)
    vertices.extend(obj.matrix_world @ v.co for v in obj.data.vertices)
    obj.data.calc_loop_triangles()
    faces.extend(tuple(offset+i for i in t.vertices) for t in obj.data.loop_triangles)
if not vertices or not faces:
    raise RuntimeError('Collision import has no mesh geometry')
bvh = BVHTree.FromPolygons(vertices, faces, all_triangles=True)
heights = {}
for label, x, y in [('box', 0, 0), ('floor', .75, 0)]:
    hit = bvh.ray_cast(Vector((x, y, 2)), Vector((0, 0, -1)), 3)[0]
    heights[label] = hit.z if hit is not None else None
report = {'blender': bpy.app.version_string, 'mesh_objects': len(objects),
          'vertices': len(vertices), 'triangles': len(faces),
          'bounds_min': [min(v[i] for v in vertices) for i in range(3)],
          'bounds_max': [max(v[i] for v in vertices) for i in range(3)],
          'axis': 'Blender Z-up after glTF import', 'downward_ray_heights': heights}
report['checks'] = {
    'box_above_floor': heights['box'] is not None and heights['floor'] is not None
                      and heights['box'] - heights['floor'] > .5,
    'fixture_scale': .7 < max(v.z for v in vertices) < 1.1
                    and 1.8 < max(v.x for v in vertices)-min(v.x for v in vertices) < 2.5,
}
output.write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report))
if not all(report['checks'].values()):
    raise RuntimeError('Collision fixture spatial checks failed')
