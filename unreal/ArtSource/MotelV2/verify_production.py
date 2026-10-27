"""Read-only independent audit of the clean motel production .blend.

Run under the project's exclusive DCC guard, never alongside import/bake jobs:
blender --background --threads 4 --python-exit-code 1 --python verify_production.py -- \
  --package /path/to/output --report /path/to/fresh-production-audit.json

Does not save the Blender file, change export assets, render, bake or import FBX.
Writes only the explicitly selected JSON report. A PASS is a source package
integrity result; Unreal/gameplay acceptance remains a separate verification.
"""
import argparse
import datetime
import hashlib
import json
import math
import re
import sys
from pathlib import Path, PurePosixPath

import bpy

HERE = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--package', type=Path, default=HERE / 'output')
parser.add_argument('--blend', type=Path, help='Optional explicit clean blend; defaults to package/Sierra_Production.blend')
parser.add_argument('--report', type=Path, required=True)
args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
package = args.package.resolve()
blend = (args.blend or package / 'Sierra_Production.blend').resolve()
report_path = args.report.resolve()
assert report_path != blend and report_path.suffix == '.json', 'Report must be a distinct JSON path'
assert report_path not in {package / 'manifest.json', package / 'materials.json', package / 'selection.json'}, 'Never overwrite production manifests'

errors = []
report = {
    'schema': 1,
    'status': 'FAIL',
    'scope': 'Independent read-only Blender production package integrity; not native Unreal acceptance',
    'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'blender_version': bpy.app.version_string,
    'package': str(package),
    'blend': str(blend),
    'report': str(report_path),
    'errors': errors,
    'checks': {},
    'parts': {},
    'images': [],
}


def check(key, condition, detail=None):
    report['checks'][key] = {'pass': bool(condition), 'detail': detail}
    if not condition:
        errors.append({'check': key, 'detail': detail})


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(4 * 1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def portable_texture_path(raw):
    """Return a package-relative texture name or None for a nonportable path."""
    name = raw.replace('\\', '/')
    if name.startswith('//'):
        name = name[2:]
    elif name.startswith('/') or re.match(r'^[A-Za-z]:', name):
        return None
    parts = PurePosixPath(name).parts
    if not parts or parts[0] != 'textures' or any(p in {'..', '.'} for p in parts):
        return None
    return str(PurePosixPath(name))


try:
    before_hash = sha(blend)
    manifest_path, materials_path = package / 'manifest.json', package / 'materials.json'
    manifest = json.loads(manifest_path.read_text())
    material_rows = json.loads(materials_path.read_text())
    expected_parts = manifest['parts']
    expected_materials = {m['name']: m for m in material_rows}
    expected_textures = {p for m in material_rows for p in m.get('textures', {}).values()}
    check('manifest_24_parts', len(expected_parts) == 24, len(expected_parts))
    check('manifest_24_materials', len(material_rows) == len(expected_materials) == 24, len(material_rows))
    check('manifest_67_textures', len(expected_textures) == 67, len(expected_textures))
    check('manifest_directx_normals', manifest.get('normal_convention') == 'DIRECTX')
    report['input_sha256'] = {'blend': before_hash, 'manifest': sha(manifest_path), 'materials': sha(materials_path)}

    bpy.ops.wm.open_mainfile(filepath=str(blend))
    objects = {o.name: o for o in bpy.data.objects}
    materials = {m.name: m for m in bpy.data.materials}
    check('object_names_exact', set(objects) == set(expected_parts), {'missing': sorted(set(expected_parts) - set(objects)), 'extra': sorted(set(objects) - set(expected_parts))})
    check('only_mesh_objects', all(o.type == 'MESH' for o in objects.values()), {o.name: o.type for o in objects.values() if o.type != 'MESH'})
    check('exact_24_mesh_data', len(bpy.data.meshes) == 24, len(bpy.data.meshes))
    check('material_names_exact', set(materials) == set(expected_materials), {'missing': sorted(set(expected_materials) - set(materials)), 'extra': sorted(set(materials) - set(expected_materials))})
    check('portable_material_prefixes', all(n.startswith('M_MotelV2_') for n in materials))
    check('no_camera_data', len(bpy.data.cameras) == 0, len(bpy.data.cameras))
    check('no_light_data', len(bpy.data.lights) == 0, len(bpy.data.lights))
    check('no_world_data', len(bpy.data.worlds) == 0, len(bpy.data.worlds))
    check('no_curve_data', len(bpy.data.curves) == 0, len(bpy.data.curves))
    check('no_embedded_authoring_scripts', len(bpy.data.texts) == 0, len(bpy.data.texts))
    check('no_linked_libraries', len(bpy.data.libraries) == 0, len(bpy.data.libraries))
    forbidden = re.compile(r'fairheaven|negotiator|zhabotinsky|kelvora|puresky|kloppenheim|retained original', re.I)
    check('no_excluded_asset_names', not any(forbidden.search(n) for n in list(objects) + list(materials) + [im.name for im in bpy.data.images]))

    scene_members = {s.name: sorted(o.name for o in s.objects) for s in bpy.data.scenes}
    report['scenes'] = scene_members
    check('exact_two_scenes', set(scene_members) == {'Sierra production exports', 'Room6 production lining'}, sorted(scene_members))
    check('room6_scene_isolated', scene_members.get('Room6 production lining') == ['SM_Sierra_Room6Lining'])
    check('court_scene_contains_other_23', set(scene_members.get('Sierra production exports', [])) == set(expected_parts) - {'SM_Sierra_Room6Lining'})
    check('all_scenes_metre_units', all(s.unit_settings.system == 'METRIC' and abs(s.unit_settings.scale_length - 1) < 1e-7 for s in bpy.data.scenes))

    for name, spec in sorted(expected_parts.items()):
        ob = objects.get(name)
        if ob is None or ob.type != 'MESH':
            continue
        mesh = ob.data
        bounds = [[fn(v.co[k] for v in mesh.vertices) for k in range(3)] for fn in [min, max]]
        slots = [m.name if m else None for m in mesh.materials]
        max_bound_error = max(abs(bounds[i][j] - spec['bounds_local_m'][i][j]) for i in range(2) for j in range(3))
        identity_error = max(abs(ob.matrix_world[i][j] - int(i == j)) for i in range(4) for j in range(4))
        row = {'triangles': len(mesh.polygons), 'vertices': len(mesh.vertices), 'bounds_local_m': bounds, 'materials': slots, 'maximum_bound_error_m': max_bound_error, 'identity_transform_error': identity_error}
        report['parts'][name] = row
        check(name + ':triangles', len(mesh.polygons) == spec['triangles'] and all(len(p.vertices) == 3 for p in mesh.polygons), row['triangles'])
        check(name + ':vertices', len(mesh.vertices) == spec['vertices'], row['vertices'])
        check(name + ':bounds', max_bound_error <= 0.0001, max_bound_error)
        check(name + ':identity_transform', identity_error <= 1e-6, identity_error)
        check(name + ':material_slots', slots == spec['materials'] and len(slots) == 1 and all(p.material_index == 0 for p in mesh.polygons), slots)
        check(name + ':uvs', len(mesh.uv_layers) >= 1 and len(mesh.uv_layers.active.data) == len(mesh.loops))
        check(name + ':finite_vertices', all(math.isfinite(c) for v in mesh.vertices for c in v.co))
        check(name + ':no_live_modifiers', len(ob.modifiers) == 0, [m.name for m in ob.modifiers])
        check(name + ':unlinked', ob.library is None and mesh.library is None)

    actual_texture_paths = []
    for im in bpy.data.images:
        rel = portable_texture_path(im.filepath)
        packed = im.packed_file
        row = {'name': im.name, 'filepath': im.filepath, 'relative_path': rel, 'source': im.source, 'color_space': im.colorspace_settings.name, 'dimensions': list(im.size), 'packed_bytes': packed.size if packed else 0}
        report['images'].append(row)
        check(im.name + ':portable_path', rel is not None, im.filepath)
        check(im.name + ':expected_texture', rel in expected_textures, rel)
        check(im.name + ':packed', bool(packed and packed.size > 0), row['packed_bytes'])
        check(im.name + ':unlinked', im.library is None)
        if rel is not None:
            actual_texture_paths.append(rel)
            path = package / rel
            check(im.name + ':disk_texture_exists', path.is_file(), str(path))
            if path.is_file(): row['disk_sha256'] = sha(path)
        if packed:
            row['packed_sha256'] = hashlib.sha256(bytes(packed.data)).hexdigest()
    check('exact_67_packed_images', len(bpy.data.images) == 67, len(bpy.data.images))
    check('texture_paths_exact_and_unique', set(actual_texture_paths) == expected_textures and len(actual_texture_paths) == len(expected_textures), {'missing': sorted(expected_textures - set(actual_texture_paths)), 'extra': sorted(set(actual_texture_paths) - expected_textures)})

    for name, spec in expected_materials.items():
        mat = materials.get(name)
        if mat is None:
            continue
        check(name + ':portable_nodes', mat.use_nodes and not any(n.type == 'GROUP' for n in mat.node_tree.nodes))
        image_nodes = [n for n in mat.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image]
        actual_paths = {portable_texture_path(n.image.filepath) for n in image_nodes}
        check(name + ':texture_bindings', actual_paths == set(spec.get('textures', {}).values()), sorted(p for p in actual_paths if p))
        for role, path in spec.get('textures', {}).items():
            images = [n.image for n in image_nodes if portable_texture_path(n.image.filepath) == path]
            expected_space = 'sRGB' if role == 'base_color' else 'Non-Color'
            check(name + ':' + role + ':color_space', len(images) == 1 and images[0].colorspace_settings.name == expected_space, [im.colorspace_settings.name for im in images])
    after_hash = sha(blend)
    check('source_blend_unchanged', before_hash == after_hash, after_hash)
    report['status'] = 'PASS' if not errors else 'FAIL'
except Exception as exc:
    errors.append({'check': 'unhandled_verification_exception', 'type': type(exc).__name__, 'detail': str(exc)})
finally:
    report['completed_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    print('SIERRA_PRODUCTION_AUDIT', json.dumps({'status': report['status'], 'errors': len(errors), 'report': str(report_path)}), flush=True)

if report['status'] != 'PASS':
    raise SystemExit(1)
