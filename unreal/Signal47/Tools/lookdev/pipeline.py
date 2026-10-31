"""Blender look development for an explicit selection, preserving editable sources.

Run with Blender --background --factory-startup -Y --python-exit-code 1
--python pipeline.py -- --demo --config example.json --output /private/new-folder.
The script never saves over its input, installs assets, or changes the game.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))


def load_config(path):
    data = json.loads(path.read_text())
    if type(data.get('schema_version')) is not int or data.get('schema_version') != 1 or not isinstance(data.get('materials'), list):
        raise ValueError('Expected schema_version 1 and explicit materials list')
    if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_]{0,47}', data.get('name', '')):
        raise ValueError('name must be an asset-safe identifier (1 to 48 characters)')
    targets = [(x.get('object'), x.get('slot', 0)) for x in data['materials']]
    if len(targets) != len(set(targets)):
        raise ValueError('Each object/material slot may be targeted only once')
    return data


def output_directory(path, source=None):
    path = path.expanduser().absolute()
    if path != path.resolve():
        raise ValueError('Output may not contain symlinks or ..')
    if source and (path == source.parent or path == source):
        raise ValueError('Use a separate output directory, away from the original asset')
    existing = path
    while not existing.exists():
        existing = existing.parent
    check = subprocess.run(['git', '-C', str(existing), 'rev-parse', '--show-toplevel'],
                           capture_output=True, check=False)
    if check.returncode == 0:
        raise ValueError('Generated/private assets must be outside any Git working tree')
    if path.exists():
        raise ValueError('Use a new output directory; existing results are never overwritten')
    path.mkdir(parents=True)
    return path


def geometry_state(objects):
    """Geometry/UV/rig fingerprint; material bindings are intentionally editable."""
    import bpy
    bpy.context.view_layer.update()
    rows = []
    for obj in sorted(objects, key=lambda x: x.name):
        row = {'name': obj.name, 'type': obj.type,
               'matrix': [list(x) for x in obj.matrix_world],
               'parent': obj.parent.name if obj.parent else None}
        if obj.type == 'MESH':
            mesh = obj.data
            row.update(vertices=[tuple(v.co) for v in mesh.vertices],
                       faces=[(tuple(p.vertices), p.material_index, p.use_smooth) for p in mesh.polygons],
                       uv=[(u.name, [tuple(v.uv) for v in u.data]) for u in mesh.uv_layers],
                       weights=[[(g.group, g.weight) for g in v.groups] for v in mesh.vertices],
                       normals=[tuple(n.vector) for n in mesh.corner_normals],
                       modifiers=[(m.name, m.type, m.show_viewport, m.show_render) for m in obj.modifiers])
        if obj.type == 'ARMATURE':
            row['bones'] = [(b.name, tuple(b.head_local), tuple(b.tail_local),
                             b.parent.name if b.parent else None) for b in obj.data.bones]
        rows.append(row)
    return hashlib.sha256(json.dumps(rows, sort_keys=True).encode()).hexdigest()


def inventory(scene):
    return [{'object': o.name, 'type': o.type,
             'dimensions_m': [float(x) * scene.unit_settings.scale_length for x in o.dimensions],
             'slots': [{'slot': i, 'material': s.material.name if s.material else None}
                       for i, s in enumerate(o.material_slots)],
             'uv_layers': [u.name for u in o.data.uv_layers] if o.type == 'MESH' else []}
            for o in scene.objects]


def build_demo():
    """Small original test fixture, not replacement production/game assets."""
    import bpy
    from mathutils import Vector
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1

    def surface(name, color, roughness=.4, metallic=0):
        m = bpy.data.materials.new(name)
        m.use_nodes = True
        bsdf = m.node_tree.nodes.get('Principled BSDF')
        bsdf.inputs['Base Color'].default_value = (*color, 1)
        bsdf.inputs['Roughness'].default_value = roughness
        bsdf.inputs['Metallic'].default_value = metallic
        return m

    cream = surface('Source_CreamEnamel', (.42, .33, .19), .24)
    wall = surface('Source_Plaster', (.29, .28, .23), .86)
    metal = surface('Source_Metal', (.3, .32, .34), .23, .85)
    glass = surface('Source_Glass', (.82, .95, .86), .15)
    p = glass.node_tree.nodes.get('Principled BSDF')
    p.inputs['Transmission Weight'].default_value = 1
    p.inputs['IOR'].default_value = 1.45

    # A linked source texture and normal graph exercise non-destructive refinement.
    image = bpy.data.images.new('DemoSourceColor', width=32, height=32)
    image.pixels[:] = [v for y in range(32) for x in range(32)
                      for v in ((.45, .34, .19, 1) if (x // 4 + y // 4) % 2 else (.25, .2, .12, 1))]
    image.pack()
    tree = cream.node_tree
    tex = tree.nodes.new('ShaderNodeTexImage'); tex.image = image
    tree.links.new(tex.outputs['Color'], tree.nodes.get('Principled BSDF').inputs['Base Color'])
    noise = tree.nodes.new('ShaderNodeTexNoise'); noise.inputs['Scale'].default_value = 55
    bump = tree.nodes.new('ShaderNodeBump'); bump.inputs['Distance'].default_value = .0002
    tree.links.new(noise.outputs['Fac'], bump.inputs['Height'])
    tree.links.new(bump.outputs['Normal'], tree.nodes.get('Principled BSDF').inputs['Normal'])

    def box(name, at, size, mat):
        bpy.ops.mesh.primitive_cube_add(size=1, location=at)
        obj = bpy.context.object; obj.name = name
        obj.dimensions = size
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        obj.data.materials.append(mat)
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.smart_project(island_margin=.04)
        bpy.ops.object.mode_set(mode='OBJECT')
        return obj

    box('DemoFloor', (0, 0, -.05), (5, 4, .1), wall)
    box('DemoBackWall', (0, 1.95, 1.25), (5, .1, 2.5), wall)
    box('DemoTable', (0, .2, .7), (2.5, 1, .12), metal)
    box('DemoPaint', (-.65, .15, .99), (.6, .55, .46), cream)
    box('DemoMetal', (.1, .3, .95), (.48, .4, .35), metal)
    box('DemoGlassPane', (.8, .3, 1.15), (.62, .006, .78), glass)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=.22, location=(.1, -.15, 1.35))
    obj = bpy.context.object; obj.name = 'DemoSolidGlass'; obj.data.materials.append(glass)
    for face in obj.data.polygons: face.use_smooth = True
    bpy.ops.object.camera_add(location=(3.8, -5.5, 2.8))
    camera = bpy.context.object; camera.name = 'DemoCamera'
    camera.rotation_euler = (Vector((0, .3, 1)) - camera.location).to_track_quat('-Z', 'Y').to_euler()
    camera.data.lens = 48
    scene.camera = camera
    return scene


def preview(path, width, samples):
    import bpy
    scene = bpy.context.scene
    if not scene.camera:
        raise ValueError('Preview needs an existing scene camera; no production camera is moved')
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = samples
    scene.cycles.use_denoising = True
    scene.render.threads_mode = 'FIXED'; scene.render.threads = 4
    scene.render.resolution_x = width
    scene.render.resolution_y = round(width * 9 / 16)
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)


def export_fbx(objects, rows, path):
    """Export disposable copies with baked maps; never apply transforms to source."""
    import bpy
    from baking import export_material
    selected = list(bpy.context.selected_objects)
    active = bpy.context.view_layer.objects.active
    temporary, made_materials, mapping = [], [], {}
    try:
        for obj in objects:
            if obj.type not in {'MESH', 'ARMATURE', 'EMPTY'}:
                continue
            copy = obj.copy()
            if obj.type == 'MESH': copy.data = obj.data.copy()
            bpy.context.scene.collection.objects.link(copy)
            copy['s47_source_object'] = obj.name
            temporary.append(copy)
            mapping[obj] = copy
        for original, obj in mapping.items():
            if original.parent in mapping:
                matrix = obj.matrix_world.copy(); obj.parent = mapping[original.parent]; obj.matrix_world = matrix
            for mod in obj.modifiers:
                if mod.type == 'ARMATURE' and mod.object in mapping: mod.object = mapping[mod.object]
            if obj.type != 'MESH': continue
            for row in rows:
                if row['object'] != original.name: continue
                material = export_material(row)
                made_materials.append(material)
                obj.material_slots[row['slot']].link = 'OBJECT'
                obj.material_slots[row['slot']].material = material
        bpy.ops.object.select_all(action='DESELECT')
        for obj in temporary: obj.select_set(True)
        bpy.context.view_layer.objects.active = temporary[0]
        bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True,
                                object_types={'MESH', 'ARMATURE', 'EMPTY'},
                                apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
                                axis_forward='-Y', axis_up='Z', bake_anim=False,
                                add_leaf_bones=False, use_tspace=True, use_custom_props=True,
                                path_mode='RELATIVE')
        return {original.name: copy.name for original, copy in mapping.items()}
    finally:
        for obj in temporary:
            mesh = obj.data if obj.type == 'MESH' else None
            bpy.data.objects.remove(obj, do_unlink=True)
            if mesh and mesh.users == 0: bpy.data.meshes.remove(mesh)
        for mat in made_materials:
            if mat.users == 0: bpy.data.materials.remove(mat)
        for obj in selected: obj.select_set(True)
        bpy.context.view_layer.objects.active = active


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--input', type=Path)
    source.add_argument('--demo', action='store_true')
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--profile', choices=['day', 'night'], default='day')
    parser.add_argument('--inventory-only', action='store_true')
    parser.add_argument('--bake', action='store_true')
    parser.add_argument('--bake-size', type=int, default=1024)
    parser.add_argument('--export-fbx', action='store_true')
    parser.add_argument('--preview', action='store_true')
    parser.add_argument('--before-after', action='store_true')
    parser.add_argument('--preview-width', type=int, default=640)
    parser.add_argument('--samples', type=int, default=16)
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    if not 64 <= args.bake_size <= 4096 or not 160 <= args.preview_width <= 1920 or not 1 <= args.samples <= 128:
        parser.error('Bake 64..4096, preview width 160..1920, samples 1..128')
    if args.export_fbx and not args.bake:
        parser.error('--export-fbx requires --bake so procedural shaders are not silently lost')
    data = load_config(args.config.resolve(strict=True))
    source_path = args.input.expanduser().resolve(strict=True) if args.input else None
    import bpy
    if source_path:
        if source_path.suffix.lower() == '.blend':
            bpy.ops.wm.open_mainfile(filepath=str(source_path), load_ui=False)
        elif source_path.suffix.lower() in {'.glb', '.gltf'}:
            bpy.ops.wm.read_factory_settings(use_empty=True)
            bpy.ops.import_scene.gltf(filepath=str(source_path))
        else:
            parser.error('Supported inputs: .blend, .glb, .gltf (Tripo GLB recommended)')
    else:
        build_demo()
    scene = bpy.context.scene
    if not math.isclose(scene.unit_settings.scale_length, 1):
        raise ValueError('Recipe uses meters: first normalize units in a separate prepared source copy')
    output = output_directory(args.output, source_path)
    (output / 'inventory.json').write_text(json.dumps(inventory(scene), indent=2))
    if args.inventory_only:
        print('LOOKDEV_INVENTORY', output / 'inventory.json'); return
    objects = list(scene.objects)
    before = geometry_state(objects)
    from materials import apply_materials
    from lighting import apply_lighting
    light_report = apply_lighting(data, args.profile)
    if args.before_after:
        preview(output / 'before.png', args.preview_width, args.samples)
    rows = apply_materials(data['materials'], args.config.parent.resolve())
    for row in rows: row.setdefault('textures', {})
    if geometry_state(objects) != before:
        raise RuntimeError('Material/lighting edit changed source geometry, transforms, UV or rig')
    if args.self_test:
        first = [(r['object'], r['slot'], r['material_name']) for r in rows]
        count = len(bpy.data.materials)
        repeat = apply_materials(data['materials'], args.config.parent.resolve())
        assert first == [(r['object'], r['slot'], r['material_name']) for r in repeat]
        assert len(bpy.data.materials) == count, 'Repeat created extra material copies'
        other = 'night' if args.profile == 'day' else 'day'
        apply_lighting(data, other)
        apply_lighting(data, args.profile)
        assert before == geometry_state(objects), 'Profile switching changed source geometry'
    if args.bake:
        from baking import bake_materials
        bake_materials(rows, output, args.bake_size, min(args.samples, 8))
    if args.preview or args.before_after:
        preview(output / 'after.png', args.preview_width, args.samples)
    blend = output / (data['name'] + '_' + args.profile + '.blend')
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    fbx_objects = {}
    if args.export_fbx:
        # Include the asset's full rig/mesh hierarchy; exclude the owned preview rig.
        fbx_objects = export_fbx(objects, rows, output / (data['name'] + '.fbx'))
    for row in rows:
        row['textures'] = {k: str(Path(v).relative_to(output)) for k, v in row['textures'].items()}
    manifest = {'schema_version': 1, 'name': data['name'], 'source': str(source_path) if source_path else 'original_demo',
                'blender_version': bpy.app.version_string, 'coordinate_system': 'Blender_Z_UP_meters',
                'profile': args.profile, 'materials': rows, 'lighting': data['lighting'],
                'fbx_objects': fbx_objects,
                'geometry_preserved': before == geometry_state(objects),
                'lighting_report': light_report, 'blend': blend.name,
                'unreal_status': 'NOT_APPLIED',
                'export_notes': ['Blender nodes/volumes/world are not FBX shaders.',
                                 'Opaque maps are baked; glass needs native Unreal material.',
                                 'FBX uses -Y forward / Z up like existing interior exports; use consistent legacy LOD import.']}
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2))
    print('LOOKDEV_COMPLETE', output / 'manifest.json')


if __name__ == '__main__':
    main()
