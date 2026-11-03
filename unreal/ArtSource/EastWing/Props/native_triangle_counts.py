"""Predict UE's unconditional removal of repeated-corner FBX triangles, read-only."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import bpy

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('eastwing_import', ROOT / 'import_unreal.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
plan = module.make_plan()
rows = []

def f32(value):
    return struct.unpack('f', struct.pack('f', value))[0]

for model in plan['models']:
    for lod, item in enumerate(model['lods']):
        path = Path(model['source']).parent / item['file']
        before = hashlib.sha256(path.read_bytes()).hexdigest()
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(path), use_custom_normals=True)
        total = removed = 0
        for obj in bpy.context.scene.objects:
            if obj.type != 'MESH' or obj.name.startswith('UCX_'):
                continue
            obj.data.calc_loop_triangles()
            points = [tuple(f32(v * 100) for v in (obj.matrix_world @ p.co)) for p in obj.data.vertices]
            for triangle in obj.data.loop_triangles:
                total += 1
                a, b, c = (points[i] for i in triangle.vertices)
                removed += a == b or a == c or b == c
        assert total == item['triangles'], (path.name, total, item['triangles'])
        assert hashlib.sha256(path.read_bytes()).hexdigest() == before
        rows.append({'asset': model['row']['asset'], 'lod': lod, 'file': path.name,
                     'sha256': before, 'source_triangles': total,
                     'repeated_corner_triangles': removed, 'render_triangles': total - removed})
output = {'engine_rule': 'UE5.8.4 StaticMeshBuilder: repeated float32 corner positions are discarded even with remove_degenerates=False',
          'source_files_unchanged': True, 'lods': rows}
(ROOT / 'native-triangle-counts.json').write_text(json.dumps(output, indent=2) + '\n')
print('EASTWING NATIVE TRIANGLE PREDICTION', len(rows), 'FBX files', sum(r['repeated_corner_triangles'] for r in rows), 'zero-edge triangles')
