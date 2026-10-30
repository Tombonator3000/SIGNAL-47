"""Private selected-to-active PBR bake of the four reviewed Vintage originals.

Run only in a reserved Blender process, with factory startup and -Y. A validated
owned_sources.py plan is required. No engine/API/process launcher is included.
The source object's transforms, UV layers and shader graphs remain unchanged;
only evaluated and independently materialled copies are used for baking/export.
"""
import argparse
from array import array
from collections import Counter
import hashlib
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


def indices_hash(indices):
    return hashlib.sha256(json.dumps(indices, separators=(',', ':')).encode()).hexdigest()


def check_source_indices(high, frozen):
    current = tuple(poly.material_index for poly in high.data.polygons)
    O.require(current == frozen, 'Source polygon material indices changed: ' + high.name)
    O.require(all(0 <= index < len(high.data.materials) for index in frozen),
              'Source polygon has an unmapped material slot: ' + high.name)


def replace_source_materials(high, materials, frozen):
    """IDMaterials.clear() resets polygon indices; restore the frozen mapping.

    Never recapture indices after clear(), or the all-zero reset would become the
    new baseline. Only the separate target atlas is deliberately single-material.
    """
    O.require(len(high.data.polygons) == len(frozen), 'Source polygon population changed')
    O.require(bool(materials) and all(materials) and
              all(0 <= index < len(materials) for index in frozen), 'Replacement material map is incomplete')
    high.data.materials.clear()
    for material in materials:
        high.data.materials.append(material)
    high.data.polygons.foreach_set('material_index', frozen)
    high.data.update()
    check_source_indices(high, frozen)


def source_histogram(high, materials, frozen):
    check_source_indices(high, frozen)
    counts = Counter(frozen)
    triangles = Counter()
    for poly, index in zip(high.data.polygons, frozen):
        triangles[index] += len(poly.vertices) - 2
    return [{'slot': index, 'material': material.name, 'polygons': counts[index],
             'triangles': triangles[index]} for index, material in enumerate(materials)]


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
    frozen = tuple(poly.material_index for poly in high.data.polygons)
    O.require(bool(frozen), 'Source object has no evaluated polygons: ' + name)
    check_source_indices(high, frozen)
    obj.hide_render = True
    return obj, high, frozen


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


def bake(high, low, target_node, original_materials, frozen, channel, path, size, samples):
    scene = bpy.context.scene
    scene.cycles.samples = samples
    temporary = []
    image = None
    result = {'source_material_indices_preserved': False,
              'source_material_indices_sha256': indices_hash(frozen)}
    check_source_indices(high, frozen)
    try:
        mapped = []
        for original in original_materials:
            material = original if channel == 'normal' else channel_material(original, channel)
            if material != original:
                temporary.append(material)
            mapped.append(material)
        replace_source_materials(high, mapped, frozen)
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
        check_source_indices(high, frozen)
        image.filepath_raw = str(path)
        image.file_format = 'PNG'
        image.save()
        O.require(path.is_file() and path.stat().st_size > 0, 'Bake did not save ' + str(path))
        if channel == 'metallic':
            pixels = array('f', [0.]) * len(image.pixels)
            image.pixels.foreach_get(pixels)
            red = pixels[0::4]
            stats = {'minimum': float(min(red)), 'maximum': float(max(red)),
                     'nonzero_texels_over_002': sum(value > .02 for value in red)}
            used = set(frozen)
            chrome = [mat.name for index, mat in enumerate(original_materials)
                      if index in used and 'chrome' in mat.name.lower()]
            result['metallic_pixels'] = stats
            result['used_chrome_source_materials'] = chrome
            O.require(not chrome or stats['nonzero_texels_over_002'] > 0,
                      'Used chrome source slots produced an all-zero metallic atlas; '
                      'do not accept this bake without investigating its source graph: ' + str(stats))
    finally:
        target_node.image = None
        if image is not None:
            bpy.data.images.remove(image)
        replace_source_materials(high, original_materials, frozen)
        for material in temporary:
            bpy.data.materials.remove(material)
    result['source_material_indices_preserved'] = True
    return result


def export(low, output, recenter=True):
    active(low)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    vertices = low.data.vertices
    lo = Vector([min(vertex.co[i] for vertex in vertices) for i in range(3)])
    hi = Vector([max(vertex.co[i] for vertex in vertices) for i in range(3)])
    if recenter:
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
            'pivot': ('bottom centre, Blender Z up; source front retained' if recenter else
                      'unchanged closed ArchiveBox bottom-centre origin; subset NOT recentered'),
            'physical_scale_review': 'UNVERIFIED; original metric geometry retained'}


def box_face_partition(mesh):
    """Partition the actual evaluated mesh: one 515-face body, two lid islands.

    The original editable object has five islands; evaluation joins its body.
    Keep the measured 515/120/25 topology and the historical min-Z > .2 rule.
    """
    O.require(len(mesh.polygons) == 660 and all(len(face.vertices) == 3 for face in mesh.polygons),
              'ArchiveBox must remain the exact triangulated 660-face source')
    parents = list(range(len(mesh.vertices)))

    def root(index):
        while parents[index] != index:
            parents[index] = parents[parents[index]]
            index = parents[index]
        return index

    for face in mesh.polygons:
        first = root(face.vertices[0])
        for vertex in face.vertices[1:]:
            parents[root(vertex)] = first
    components = {}
    for face in mesh.polygons:
        components.setdefault(root(face.vertices[0]), []).append(face.index)
    lid, info = set(), []
    for faces in components.values():
        vertices = {vertex for index in faces for vertex in mesh.polygons[index].vertices}
        minimum = min(mesh.vertices[index].co.z for index in vertices)
        is_lid = minimum > .2
        if is_lid:
            lid.update(faces)
        info.append({'minimum_z_m': float(minimum), 'triangles': len(faces),
                     'subset': 'lid' if is_lid else 'body'})
    body = set(range(660)) - lid
    O.require(sorted((row['subset'],row['triangles']) for row in info) ==
              [('body',515),('lid',25),('lid',120)] and len(body) == 515 and len(lid) == 145,
              'Evaluated ArchiveBox 515/120/25 component split no longer matches: ' + str(info))
    return body, lid, info


def mesh_snapshot(mesh):
    uv_name = mesh.uv_layers.active.name
    mesh.calc_tangents(uvmap=uv_name)
    snapshot = {'vertices': [tuple(vertex.co) for vertex in mesh.vertices],
                'faces': [list(face.loop_indices) for face in mesh.polygons],
                'corner_vertices': [loop.vertex_index for loop in mesh.loops],
                'normals': [tuple(item.vector) for item in mesh.corner_normals],
                'tangents': [tuple(loop.tangent) for loop in mesh.loops],
                'bitangent_signs': [float(loop.bitangent_sign) for loop in mesh.loops],
                'uv_layers': {layer.name: [tuple(item.uv) for item in layer.data] for layer in mesh.uv_layers},
                'active_uv': uv_name}
    snapshot['sha256'] = hashlib.sha256(json.dumps(snapshot, sort_keys=True, separators=(',', ':'),
                                                  allow_nan=False).encode()).hexdigest()
    return snapshot


def verify_box_subset(obj, snapshot, expected_faces):
    """Stop if native Separate lost corner data; do not synthesize new normals."""
    mesh = obj.data
    ids = {}
    for suffix, domain in (('Vertex', 'POINT'), ('Face', 'FACE'), ('Corner', 'CORNER')):
        attribute = mesh.attributes.get('S47_Source' + suffix)
        O.require(attribute is not None and attribute.domain == domain and attribute.data_type == 'INT',
                  'Separate lost exact provenance ' + suffix)
        ids[suffix] = [item.value for item in attribute.data]
    O.require(len(ids['Face']) == len(expected_faces) and set(ids['Face']) == expected_faces,
              'Separate changed the exact ArchiveBox face subset')
    O.require(mesh.uv_layers.active.name == snapshot['active_uv'] and
              {layer.name for layer in mesh.uv_layers} == set(snapshot['uv_layers']), 'Separate changed atlas UV layers')
    for vertex, index in zip(mesh.vertices, ids['Vertex']):
        O.require(tuple(vertex.co) == snapshot['vertices'][index], 'Subset vertex position changed')
    for face, original_index in zip(mesh.polygons, ids['Face']):
        corners = [ids['Corner'][index] for index in face.loop_indices]
        expected = snapshot['faces'][original_index]
        O.require(len(corners) == 3 and any(corners == expected[n:] + expected[:n] for n in range(3)),
                  'Separate changed source triangle winding or corners')
    for loop, corner in zip(mesh.loops, ids['Corner']):
        O.require(ids['Vertex'][loop.vertex_index] == snapshot['corner_vertices'][corner],
                  'Separate changed a source corner/vertex binding')
        for layer in mesh.uv_layers:
            O.require(tuple(layer.data[loop.index].uv) == snapshot['uv_layers'][layer.name][corner],
                      'Separate changed an atlas UV value')
    mesh.calc_tangents(uvmap=snapshot['active_uv'])
    normals = [tuple(item.vector) for item in mesh.corner_normals]
    normal_error = max(abs(value - snapshot['normals'][corner][axis]) for index, corner in enumerate(ids['Corner'])
                       for axis, value in enumerate(normals[index]))
    tangent_error = max(abs(value - snapshot['tangents'][corner][axis]) for loop, corner in zip(mesh.loops, ids['Corner'])
                        for axis, value in enumerate(loop.tangent))
    O.require(normal_error <= 1e-6 and tangent_error <= 1e-6,
              'Separate did not preserve source normal/tangent vectors: ' + str((normal_error, tangent_error)))
    O.require(all(loop.bitangent_sign == snapshot['bitangent_signs'][corner]
                  for loop, corner in zip(mesh.loops, ids['Corner'])), 'Separate changed tangent handedness')
    return {'exact_source_faces': True, 'vertex_coordinates_equal': True, 'uv_values_equal': True,
            'triangle_winding_equal': True, 'normal_max_error': normal_error,
            'tangent_max_error': tangent_error, 'vector_tolerance': 1e-6, 'bitangent_signs_equal': True}


def archive_box_derivatives(closed, base_part, output):
    """Separate existing body/lid only; no cap planes, retopology or new bake."""
    body_faces, lid_faces, components = box_face_partition(closed.data)
    snapshot = mesh_snapshot(closed.data)
    work = closed.copy()
    work.data = closed.data.copy()
    bpy.context.scene.collection.objects.link(work)
    work.name = 'SM_InteriorOwned_ArchiveBoxBody'
    for suffix, domain, count in (('Vertex', 'POINT', len(work.data.vertices)),
                                   ('Face', 'FACE', len(work.data.polygons)),
                                   ('Corner', 'CORNER', len(work.data.loops))):
        attribute = work.data.attributes.new('S47_Source' + suffix, 'INT', domain)
        attribute.data.foreach_set('value', list(range(count)))
    active(work)
    bpy.context.tool_settings.mesh_select_mode = (False, False, True)
    for vertex in work.data.vertices:
        vertex.select = False
    for edge in work.data.edges:
        edge.select = False
    for face in work.data.polygons:
        face.select = face.index in lid_faces
    before = {obj.as_pointer() for obj in bpy.data.objects}
    bpy.ops.object.mode_set(mode='EDIT')
    try:
        bpy.ops.mesh.separate(type='SELECTED')
    finally:
        bpy.ops.object.mode_set(mode='OBJECT')
    new = [obj for obj in bpy.data.objects if obj.as_pointer() not in before]
    O.require(len(new) == 1 and new[0].type == 'MESH', 'Separate did not produce exactly one lid mesh')
    lid = new[0]
    lid.name = 'SM_InteriorOwned_ArchiveBoxLid'
    parts = {}
    for obj, faces in ((work, body_faces), (lid, lid_faces)):
        O.require(obj.matrix_world == closed.matrix_world, 'Subset no longer shares closed-box origin')
        verify_box_subset(obj, snapshot, faces)
        part = export(obj, output, recenter=False)
        # FBX tangent calculation must also leave every source attribute intact.
        proof = verify_box_subset(obj, snapshot, faces)
        part.update({'derived_from': closed.name, 'geometry_only_derivative': True,
                     'source_material_proof_base': closed.name, 'shared_atlas_source': closed.name,
                     'texture_sha256': base_part['texture_sha256'],
                     'source_base_fbx_sha256': base_part['fbx_sha256'],
                     'source_geometry_snapshot_sha256': snapshot['sha256'],
                     'source_triangle_indices': sorted(faces), 'geometry_subset_proof': proof,
                     'shared_origin': closed.name + ' bottom centre',
                     'hollow_interior_appearance': 'UNVERIFIED; no opening/cap geometry added'})
        O.require(part['triangles'] == len(faces) and part['materials'] == base_part['materials'],
                  'Subset FBX no longer uses exact source triangles and shared material')
        parts[obj.name] = part
    O.require(mesh_snapshot(closed.data)['sha256'] == snapshot['sha256'], 'Closed ArchiveBox source geometry changed')
    O.require(not body_faces.intersection(lid_faces) and body_faces.union(lid_faces) == set(range(660)),
              'Body/lid do not reconstruct the exact original triangle population')
    union = {'base': closed.name, 'base_fbx_sha256': base_part['fbx_sha256'],
             'source_geometry_snapshot_sha256': snapshot['sha256'], 'split_rule': 'disconnected island minimum Z > 0.2m is lid',
             'source_components': components, 'body_triangles': 515, 'lid_triangles': 145,
             'union_triangles': 660, 'source_faces_disjoint': True, 'source_faces_complete': True,
             'closed_geometry_unchanged': True, 'shared_origin': True, 'shared_baked_atlas': True,
             'hollow_interior_appearance': 'UNVERIFIED'}
    return parts, union


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan', type=Path, default=O.HOME / 'scratch-bm/Signal47-interiors-20261029/owned-sources/stage-plan.json')
    parser.add_argument('--output', type=Path, default=O.HOME / 'scratch-bm/Signal47-interiors-20261029/owned-bake')
    parser.add_argument('--size', type=int, choices=(1024, 2048, 4096), default=2048)
    parser.add_argument('--samples', type=int, default=16)
    parser.add_argument('--device', choices=('CPU', 'GPU'), default='CPU')
    parser.add_argument('--asset', action='append', choices=tuple(NAMES),
                        help='Bake only these reviewed originals; omitted assets remain unaccepted')
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
              'source_material_indices_preserved': False,
              'material_limitation': 'Stops on unsupported shader, alpha or transmission; no silent flattening.'}
    if args.asset:
        inputs = [row for row in inputs if row[1] in args.asset]
        O.require(bool(inputs), 'No reviewed source matched the requested assets')
    report['selected_assets'] = [row[1] for row in inputs]
    report['omitted_assets'] = [name for name in NAMES if name not in report['selected_assets']]
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
            original, high, frozen = append_source(blend, name)
            original_materials = list(high.data.materials)
            O.require(original_materials and all(original_materials), 'Missing evaluated material slots')
            histogram = source_histogram(high, original_materials, frozen)
            source_uvs = [layer.name for layer in original.data.uv_layers]
            low, material, target = atlas_target(high, NAMES[name], args.size)
            textures = {}
            passes = {}
            for channel in CHANNELS:
                relative = Path('textures') / (material.name + '_' + channel + '.png')
                passes[channel] = bake(high, low, target, original_materials, frozen, channel,
                                       output / relative, args.size, args.samples)
                textures[channel] = str(relative)
            check_source_indices(high, frozen)
            O.require(source_uvs == [layer.name for layer in original.data.uv_layers], 'Original UV layers changed')
            part = export(low, output)
            part.update({'source': name, 'source_blend': blend.name, 'source_sha256': expected_hash,
                         'source_uv_layers': source_uvs, 'material_source_names': [mat.name for mat in original_materials],
                         'source_material_indices_preserved': all(value['source_material_indices_preserved'] for value in passes.values()),
                         'source_material_indices_sha256': indices_hash(frozen),
                         'source_material_index_histogram': histogram,
                         'bake_passes': passes,
                         'texture_sha256': {role: O.digest(output / path) for role, path in textures.items()}})
            O.require(O.digest(blend) == expected_hash, 'Original blend changed during build')
            report['parts'][low.name] = part
            if name == '24_Cardboard Archive Box':
                derivatives, partition = archive_box_derivatives(low, part, output)
                report['parts'].update(derivatives)
                report.setdefault('derived_partitions', {})[low.name] = partition
            materials.append({'name': material.name, 'base_color_linear': [1, 1, 1, 1],
                              'roughness': 1, 'metallic': 0, 'specular': .5,
                              'normal_strength': 1, 'textures': textures,
                              'required_native_features': ['textures.metallic']})
            report_path.write_text(json.dumps(report, indent=2) + '\n')
        bases = [part for part in report['parts'].values() if not part.get('derived_from')]
        report['source_material_indices_preserved'] = (len(bases) == len(inputs) and
                all(part['source_material_indices_preserved'] for part in bases))
        report['source_material_index_scope'] = 'Baked originals; named geometry-only subsets inherit their base proof'
        O.require(report['source_material_indices_preserved'], 'Incomplete source material preservation proof')
        report['status'] = 'PASS_PRIVATE_BAKE_AND_FBX_EXPORT'
        contract = {'schema': 1, 'parts': report['parts'],
                    'derived_partitions': report.get('derived_partitions', {}),
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
