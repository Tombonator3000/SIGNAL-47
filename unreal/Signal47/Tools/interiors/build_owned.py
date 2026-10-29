"""Private selected-to-active PBR bake of the four reviewed Vintage originals.

Run only in a reserved Blender process, with factory startup and -Y. A validated
owned_sources.py plan is required. No engine/API/process launcher is included.
The source object's transforms, UV layers and shader graphs remain unchanged;
only evaluated and independently materialled copies are used for baking/export.
"""
import argparse
import json
from pathlib import Path
import sys
import traceback

import bpy
from mathutils import Matrix, Vector

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import owned_sources as O

NAMES = {
    '24_Cardboard Archive Box': 'ArchiveBox',
    '57_Wooden Bookcase': 'Bookcase',
    '13_Coaxial Signal Cable': 'SignalCable',
    '20_Audio Cassette Tape': 'AudioCassette',
}
CHANNELS = ('base_color', 'roughness', 'metallic', 'normal')


def active(obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def image_inputs(tree, seen=None):
    seen = set() if seen is None else seen
    if tree.as_pointer() in seen:
        return
    seen.add(tree.as_pointer())
    for node in tree.nodes:
        if node.type == 'TEX_IMAGE':
            O.require(node.image is not None, 'Unmapped image texture node: ' + node.name)
            yield node.image
        elif node.type == 'GROUP' and node.node_tree:
            yield from image_inputs(node.node_tree, seen)


def validate_materials(obj, blend):
    O.require(bool(obj.data.uv_layers), 'No original UV layers: ' + obj.name)
    uv_names = {layer.name for layer in obj.data.uv_layers}
    seen_trees = set()

    def validate_tree(tree):
        if tree.as_pointer() in seen_trees:
            return
        seen_trees.add(tree.as_pointer())
        for node in tree.nodes:
            if node.type == 'UVMAP' and node.uv_map:
                O.require(node.uv_map in uv_names,
                          'Missing original UV map ' + node.uv_map + ' in ' + obj.name)
            if node.type == 'GROUP' and node.node_tree:
                validate_tree(node.node_tree)

    for slot in obj.material_slots:
        mat = slot.material
        O.require(mat is not None and mat.use_nodes, 'Missing node material on ' + obj.name)
        validate_tree(mat.node_tree)
        for image in image_inputs(mat.node_tree):
            O.require(image.source in ('FILE', 'GENERATED'),
                      'Unsupported animated/tiled texture: ' + image.name)
            if image.source == 'FILE' and not image.packed_file:
                path = Path(bpy.path.abspath(image.filepath, library=image.library))
                if image.filepath.startswith('//'):
                    path = blend.parent / image.filepath[2:]
                O.require(path.is_file(), 'Missing source texture: ' + str(path))
            O.require(image.size[0] > 0 and image.size[1] > 0,
                      'Source texture has no decoded pixels: ' + image.name)


def output_node(tree, kind):
    rows = [node for node in tree.nodes if node.type == kind]
    selected = [node for node in rows if getattr(node, 'is_active_output', False)]
    O.require(bool(rows) and (len(rows) == 1 or len(selected) == 1),
              'Missing or ambiguous active shader output')
    return selected[0] if selected else rows[0]


def channel_material(original, channel):
    """Route actual Principled inputs through emission in a private shader copy.

    Mix/Add shader weights and nested shader-group inputs remain connected. An
    unsupported surface, transparency or transmission stops the build explicitly.
    The normal pass uses the unmodified original shader, including its bump graph.
    """
    copy = original.copy()
    copy.name = '_S47Bake_' + channel + '_' + original.name
    memo = {}
    copied_groups = set()

    def linked(socket):
        O.require(socket.is_linked, 'Unmapped shader socket: ' + socket.name)
        return socket.links[0].from_socket

    def interface_socket(sockets, identifier, label):
        matches = [item for item in sockets if item.identifier == identifier]
        O.require(len(matches) == 1, 'Unmapped or ambiguous group interface ' + label + ': ' + identifier)
        return matches[0]

    def route(tree, socket, stack, groups=()):
        key = (tree.as_pointer(), socket.as_pointer())
        O.require(key not in stack, 'Cyclic shader mapping in ' + original.name)
        if key in memo:
            return memo[key]
        node = socket.node
        next_stack = stack | {key}
        if node.type == 'REROUTE':
            result = route(tree, linked(node.inputs[0]), next_stack, groups)
        elif node.type == 'BSDF_PRINCIPLED':
            alpha = node.inputs.get('Alpha')
            transmission = node.inputs.get('Transmission Weight')
            O.require(alpha is not None and not alpha.is_linked and alpha.default_value >= .9999,
                      'Alpha requires a separate reviewed material mapping: ' + original.name)
            O.require(transmission is not None and not transmission.is_linked and transmission.default_value <= .0001,
                      'Transmission requires a separate reviewed material mapping: ' + original.name)
            source = node.inputs[{'base_color': 'Base Color', 'roughness': 'Roughness', 'metallic': 'Metallic'}[channel]]
            emission = tree.nodes.new('ShaderNodeEmission')
            emission.inputs['Strength'].default_value = 1
            if source.is_linked:
                tree.links.new(linked(source), emission.inputs['Color'])
            else:
                value = source.default_value
                emission.inputs['Color'].default_value = tuple(value) if channel == 'base_color' else (value, value, value, 1)
            result = emission.outputs[0]
        elif node.type in ('MIX_SHADER', 'ADD_SHADER'):
            for branch in node.inputs:
                if branch.type == 'SHADER':
                    tree.links.new(route(tree, linked(branch), next_stack, groups), branch)
            result = socket
        elif node.type == 'GROUP_INPUT':
            # Shader groups can wrap a caller-supplied BSDF (the pack's dust
            # group does this). Convert that caller branch in its own tree;
            # never connect sockets across trees or replace the group's mix.
            O.require(bool(groups), 'Shader group input has no caller: ' + original.name)
            outer, instance, inner = groups[-1]
            O.require(inner == tree, 'Shader group caller context differs: ' + original.name)
            caller_input = interface_socket(instance.inputs, socket.identifier, instance.name)
            O.require(caller_input.type == 'SHADER' and socket.type == 'SHADER',
                      'Expected a shader group input: ' + original.name)
            outer.links.new(route(outer, linked(caller_input), next_stack, groups[:-1]), caller_input)
            result = socket
        elif node.type == 'GROUP' and node.node_tree:
            identifier = socket.identifier
            if node.as_pointer() not in copied_groups:
                node.node_tree = node.node_tree.copy()
                copied_groups.add(node.as_pointer())
            inner = node.node_tree
            group_out = output_node(inner, 'GROUP_OUTPUT')
            destination = interface_socket(group_out.inputs, identifier, node.name)
            inner.links.new(route(inner, linked(destination), next_stack,
                                  groups + ((tree, node, inner),)), destination)
            result = interface_socket(node.outputs, identifier, node.name)
        else:
            raise ValueError('Unsupported surface mapping ' + node.bl_idname + ' in ' + original.name)
        memo[key] = result
        return result

    tree = copy.node_tree
    out = output_node(tree, 'OUTPUT_MATERIAL')
    O.require(not out.inputs['Volume'].is_linked, 'Volume material needs separate review: ' + original.name)
    tree.links.new(route(tree, linked(out.inputs['Surface']), set()), out.inputs['Surface'])
    return copy


def append_source(blend, name):
    with bpy.data.libraries.load(str(blend), link=False) as (source, target):
        O.require(name in source.objects, 'Object missing from actual original blend: ' + name)
        target.objects = [name]
    obj = target.objects[0]
    O.require(obj is not None and obj.type == 'MESH', 'Selected original is not a mesh: ' + name)
    bpy.context.scene.collection.objects.link(obj)
    bpy.context.view_layer.update()
    validate_materials(obj, blend)
    # Evaluated copy keeps every original UV and the same object/world coordinates.
    depsgraph = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(obj.evaluated_get(depsgraph),
                                         preserve_all_data_layers=True, depsgraph=depsgraph)
    high = bpy.data.objects.new('_High_' + name, mesh)
    bpy.context.scene.collection.objects.link(high)
    high.matrix_world = obj.matrix_world.copy()
    O.require(len(high.data.materials) == len(obj.material_slots), 'Evaluated material map changed: ' + name)
    obj.hide_render = True
    return obj, high


def atlas_target(high, suffix, size):
    low = bpy.data.objects.new('SM_InteriorOwned_' + suffix, high.data.copy())
    bpy.context.scene.collection.objects.link(low)
    low.matrix_world = high.matrix_world.copy()
    active(low)
    triangulate = low.modifiers.new('Delivery_triangles', 'TRIANGULATE')
    bpy.ops.object.modifier_apply(modifier=triangulate.name)
    # Only the target gets new UVs/materials. Source UVMap/Overlay stay untouched.
    for layer in list(low.data.uv_layers):
        low.data.uv_layers.remove(layer)
    low.data.uv_layers.new(name='S47_Atlas')
    low.data.uv_layers.active_index = 0
    low.data.uv_layers[0].active_render = True
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.1519173, island_margin=16 / size)
    bpy.ops.object.mode_set(mode='OBJECT')
    material = bpy.data.materials.new('M_InteriorOwned_' + suffix)
    material.use_nodes = True
    low.data.materials.clear()
    low.data.materials.append(material)
    for poly in low.data.polygons:
        poly.material_index = 0
    target = material.node_tree.nodes.new('ShaderNodeTexImage')
    target.name = 'S47_BakeTarget'
    material.node_tree.nodes.active = target
    return low, material, target


def bake(high, low, target_node, original_materials, channel, path, size, samples):
    scene = bpy.context.scene
    scene.cycles.samples = samples
    temporary = []
    high.data.materials.clear()
    for original in original_materials:
        material = original if channel == 'normal' else channel_material(original, channel)
        if material != original:
            temporary.append(material)
        high.data.materials.append(material)
    image = bpy.data.images.new('_S47_' + channel, width=size, height=size,
                                alpha=False, float_buffer=True)
    image.colorspace_settings.name = 'sRGB' if channel == 'base_color' else 'Non-Color'
    target_node.image = image
    active(low)
    high.select_set(True)
    settings = scene.render.bake
    settings.target = 'IMAGE_TEXTURES'
    settings.use_selected_to_active = True
    settings.use_cage = False
    settings.cage_extrusion = .003
    settings.max_ray_distance = .009
    settings.margin = 16
    settings.margin_type = 'EXTEND'
    settings.use_clear = True
    settings.normal_space = 'TANGENT'
    settings.normal_r = 'POS_X'
    settings.normal_g = 'NEG_Y'
    settings.normal_b = 'POS_Z'
    bpy.ops.object.bake(type='NORMAL' if channel == 'normal' else 'EMIT')
    image.filepath_raw = str(path)
    image.file_format = 'PNG'
    image.save()
    O.require(path.is_file() and path.stat().st_size > 0, 'Bake did not save ' + str(path))
    target_node.image = None
    bpy.data.images.remove(image)
    high.data.materials.clear()
    for original in original_materials:
        high.data.materials.append(original)
    for material in temporary:
        bpy.data.materials.remove(material)


def export(low, output):
    active(low)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    vertices = low.data.vertices
    lo = Vector([min(vertex.co[i] for vertex in vertices) for i in range(3)])
    hi = Vector([max(vertex.co[i] for vertex in vertices) for i in range(3)])
    pivot = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, lo.z))
    low.data.transform(Matrix.Translation(-pivot))
    low.data.update()
    path = output / 'Export' / (low.name + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH'},
                            apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
                            axis_forward='-Y', axis_up='Z', bake_anim=False,
                            mesh_smooth_type='FACE', use_tspace=True)
    O.require(path.is_file(), 'FBX export missing')
    bounds = [[float(fn(vertex.co[i] for vertex in vertices)) for i in range(3)] for fn in (min, max)]
    return {'triangles': sum(len(face.vertices) - 2 for face in low.data.polygons),
            'bounds_local_m': bounds, 'materials': [mat.name for mat in low.data.materials],
            'original_components': 1, 'fbx_sha256': O.digest(path),
            'pivot': 'bottom centre, Blender Z up; source front retained',
            'physical_scale_review': 'UNVERIFIED; original metric geometry retained'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan', type=Path, default=O.HOME / 'scratch-bm/Signal47-interiors-20261029/owned-sources/stage-plan.json')
    parser.add_argument('--output', type=Path, default=O.HOME / 'scratch-bm/Signal47-interiors-20261029/owned-bake')
    parser.add_argument('--size', type=int, choices=(1024, 2048, 4096), default=2048)
    parser.add_argument('--samples', type=int, default=16)
    parser.add_argument('--device', choices=('CPU', 'GPU'), default='CPU')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    O.require('-Y' in sys.argv or '--disable-autoexec' in sys.argv,
              'Start Blender with -Y / --disable-autoexec before loading licensed files')
    O.require(args.samples > 0, 'Sample count must be positive')
    bpy.context.preferences.filepaths.use_scripts_auto_execute = False
    config = json.loads((HERE / 'owned-selection.json').read_text())
    plan = json.loads(args.plan.read_text())
    O.require(plan['status'] in ('DRY_RUN_VALIDATED', 'STAGED_PRIVATE_SOURCES'), 'Source plan has not passed validation')
    O.require(plan['selection_sha256'] == O.digest(HERE / 'owned-selection.json'), 'Source plan is stale')
    O.require(O.digest(Path(plan['license_evidence'])) == config['license_evidence_sha256'], 'License evidence changed')
    planned = {Path(row['source']).name: row for row in plan['sources']}
    inputs = []
    for item in config['sources']:
        row = planned[item['blend']]
        blend = O.checked_file(Path(row['source']).parent, item['blend'], item['source_sha256'])
        inventory = O.checked_file(Path(row['inventory']).parent, item['inventory'], item['inventory_sha256'])
        O.require(json.loads(inventory.read_text())['file'] == item['blend'], 'Inventory source differs')
        for obj in item['objects']:
            O.require(obj['name'] in NAMES, 'Unreviewed object in selection')
            inputs.append((blend, obj['name'], item['source_sha256']))
    O.require({name for _, name, _ in inputs} == set(NAMES), 'Expected exactly four reviewed objects')
    output = O.private_work(args.output, inputs[0][0].parent)
    output.mkdir(parents=True, exist_ok=True)
    O.private_work(output, inputs[0][0].parent)
    O.require(not any(output.iterdir()), 'Bake output must be empty; choose a new candidate directory')
    (output / 'Export').mkdir()
    (output / 'textures').mkdir()
    report = {'status': 'RUNNING', 'blender': bpy.app.version_string,
              'source_plan_sha256': O.digest(args.plan), 'recipe_sha256': O.digest(Path(__file__)),
              'parts': {}, 'native_import': 'UNVERIFIED', 'runtime_appearance': 'UNVERIFIED',
              'required_native_features': ['textures.metallic'],
              'material_limitation': 'Stops on unsupported shader, alpha or transmission; no silent flattening.'}
    materials = []
    report_path = output / 'build-report.json'
    try:
        for blend, name, expected_hash in inputs:
            bpy.ops.wm.read_factory_settings(use_empty=True)
            bpy.context.preferences.filepaths.use_scripts_auto_execute = False
            scene = bpy.context.scene
            scene.unit_settings.system = 'METRIC'
            scene.unit_settings.scale_length = 1
            scene.render.engine = 'CYCLES'
            scene.cycles.device = args.device
            scene.render.image_settings.file_format = 'PNG'
            scene.render.image_settings.color_mode = 'RGB'
            scene.render.image_settings.color_depth = '16'
            original, high = append_source(blend, name)
            original_materials = list(high.data.materials)
            O.require(original_materials and all(original_materials), 'Missing evaluated material slots')
            source_uvs = [layer.name for layer in original.data.uv_layers]
            low, material, target = atlas_target(high, NAMES[name], args.size)
            textures = {}
            for channel in CHANNELS:
                relative = Path('textures') / (material.name + '_' + channel + '.png')
                bake(high, low, target, original_materials, channel, output / relative, args.size, args.samples)
                textures[channel] = str(relative)
            O.require(source_uvs == [layer.name for layer in original.data.uv_layers], 'Original UV layers changed')
            part = export(low, output)
            part.update({'source': name, 'source_blend': blend.name, 'source_sha256': expected_hash,
                         'source_uv_layers': source_uvs, 'material_source_names': [mat.name for mat in original_materials],
                         'texture_sha256': {role: O.digest(output / path) for role, path in textures.items()}})
            O.require(O.digest(blend) == expected_hash, 'Original blend changed during build')
            report['parts'][low.name] = part
            materials.append({'name': material.name, 'base_color_linear': [1, 1, 1, 1],
                              'roughness': 1, 'metallic': 0, 'specular': .5,
                              'normal_strength': 1, 'textures': textures,
                              'required_native_features': ['textures.metallic']})
            report_path.write_text(json.dumps(report, indent=2) + '\n')
        report['status'] = 'PASS_PRIVATE_BAKE_AND_FBX_EXPORT'
        contract = {'schema': 1, 'parts': report['parts'],
                    'required_native_features': report['required_native_features'],
                    'fbx_recipe': 'FBX -Y forward/Z up, metric; fieldintegration native import_rotation yaw90',
                    'native_import': 'UNVERIFIED', 'runtime_appearance': 'UNVERIFIED'}
        (output / 'model_contract.json').write_text(json.dumps(contract, indent=2) + '\n')
        (output / 'materials.json').write_text(json.dumps(materials, indent=2) + '\n')
    except Exception:
        report['status'] = 'FAIL'
        report['error'] = traceback.format_exc()
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n')
    print('S47 PRIVATE INTERIORS BAKE AND FBX EXPORT PASS: ' + str(output))


if __name__ == '__main__':
    main()
