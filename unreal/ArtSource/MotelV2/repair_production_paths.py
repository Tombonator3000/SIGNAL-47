"""Repair only packed-image paths in the completed clean production .blend.

Root runs sequentially after native import:
blender --background --threads 4 --python-exit-code 1 --python repair_production_paths.py -- \
  --package /path/to/output --report /path/to/path-repair-report.json

No rendering, baking, FBX import/export, image reload, unpack or repack. Native
FBX/textures/manifests remain untouched. Geometry and packed bytes are hashed
before and after save/reopen. An unexpected post-save failure restores the
original blend bytes; the independent production verifier must then be run.
"""
import argparse
from array import array
import datetime
import hashlib
import json
from pathlib import Path
import sys

import bpy

parser = argparse.ArgumentParser()
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--report', type=Path, required=True)
args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
package = args.package.resolve()
blend = package / 'Sierra_Production.blend'
report_path = args.report.resolve()
assert report_path.suffix == '.json' and report_path not in {package / n for n in ['manifest.json', 'materials.json', 'selection.json']}
report = {'schema': 1, 'status': 'FAIL', 'scope': 'Packed image path repair only', 'blend': str(blend), 'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'path_changes': [], 'errors': [], 'restored_original_after_failure': False}
original_bytes = None
saved = False


def digest(data):
    return hashlib.sha256(data).hexdigest()


def buffer_hash(hasher, elements, prop, count, dtype):
    values = array(dtype, [0]) * count
    elements.foreach_get(prop, values)
    hasher.update(values.tobytes())


def snapshot():
    result = {'meshes': {}, 'images': {}, 'materials': sorted(m.name for m in bpy.data.materials), 'scenes': {s.name: sorted(o.name for o in s.objects) for s in bpy.data.scenes}}
    for ob in sorted(bpy.data.objects, key=lambda o: o.name):
        assert ob.type == 'MESH', ('Unexpected nonmesh', ob.name)
        mesh = ob.data
        h = hashlib.sha256()
        h.update(json.dumps({'name': ob.name, 'matrix': [list(row) for row in ob.matrix_world], 'slots': [m.name if m else None for m in mesh.materials], 'uv_layers': [u.name for u in mesh.uv_layers], 'modifiers': [m.type for m in ob.modifiers]}, sort_keys=True).encode())
        buffer_hash(h, mesh.vertices, 'co', len(mesh.vertices) * 3, 'f')
        buffer_hash(h, mesh.loops, 'vertex_index', len(mesh.loops), 'i')
        buffer_hash(h, mesh.polygons, 'loop_start', len(mesh.polygons), 'i')
        buffer_hash(h, mesh.polygons, 'loop_total', len(mesh.polygons), 'i')
        buffer_hash(h, mesh.polygons, 'material_index', len(mesh.polygons), 'i')
        h.update(bytes(p.use_smooth for p in mesh.polygons))
        for layer in mesh.uv_layers:
            buffer_hash(h, layer.data, 'uv', len(layer.data) * 2, 'f')
        result['meshes'][ob.name] = {'sha256': h.hexdigest(), 'vertices': len(mesh.vertices), 'polygons': len(mesh.polygons), 'loops': len(mesh.loops)}
    for im in bpy.data.images:
        assert im.packed_file and im.packed_file.size > 0, ('Unpacked image', im.name)
        result['images'][im.name] = {'packed_sha256': digest(bytes(im.packed_file.data)), 'packed_bytes': im.packed_file.size, 'color_space': im.colorspace_settings.name, 'dimensions': list(im.size)}
    return result


try:
    original_bytes = blend.read_bytes()
    report['blend_sha256_before'] = digest(original_bytes)
    manifest = json.loads((package / 'manifest.json').read_text())
    materials = json.loads((package / 'materials.json').read_text())
    paths = {p for m in materials for p in m.get('textures', {}).values()}
    by_basename = {Path(p).name: p for p in paths}
    assert len(paths) == len(by_basename) == 67, 'Expected 67 unique texture filenames'
    assert all(p.startswith('textures/') and '..' not in Path(p).parts and (package / p).is_file() for p in paths), 'Texture input missing or nonportable'
    bpy.ops.wm.open_mainfile(filepath=str(blend))
    assert {o.name for o in bpy.data.objects} == set(manifest['parts']) and len(bpy.data.objects) == 24
    assert len(bpy.data.images) == 67
    before = snapshot()
    for im in bpy.data.images:
        basename = Path(im.filepath.replace('\\', '/')).name
        assert basename in by_basename, ('Unexpected packed image path', im.name, im.filepath)
        target = '//' + by_basename[basename]
        report['path_changes'].append({'image': im.name, 'before': im.filepath, 'after': target})
        im.filepath = target
    assert snapshot() == before, 'Path assignment unexpectedly changed geometry or packed bytes'
    # Relative paths are already correct for this package; automatic remapping
    # would resolve them against a prior authoring filepath and break portability.
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=str(blend), compress=True, relative_remap=False, check_existing=False)
    saved = True
    bpy.ops.wm.open_mainfile(filepath=str(blend))
    after = snapshot()
    assert after == before, 'Saved/reopened geometry, materials, scenes or packed bytes changed'
    assert {im.filepath for im in bpy.data.images} == {'//' + p for p in paths}, 'Saved image paths are not the exact package-relative paths'
    report.update(status='PASS', blend_sha256_after=digest(blend.read_bytes()), geometry_and_packed_bytes_unchanged=True, native_files_modified=False, before_snapshot=before, after_snapshot=after)
except Exception as exc:
    report['errors'].append({'type': type(exc).__name__, 'detail': str(exc)})
    if saved and original_bytes is not None:
        blend.write_bytes(original_bytes)
        report['restored_original_after_failure'] = True
finally:
    report['completed_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    print('SIERRA_PATH_REPAIR', json.dumps({'status': report['status'], 'images': len(report['path_changes']), 'report': str(report_path)}), flush=True)

if report['status'] != 'PASS':
    raise SystemExit(1)
