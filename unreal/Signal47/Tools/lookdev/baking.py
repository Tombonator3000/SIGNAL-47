"""Bake evaluated PBR channels onto existing UVs; retain editable Blender nodes.

Reuses the project's inspected shader traversal rather than guessing a texture
from a filename. Glass and volumes are exported as native-material instructions.
"""
from collections import defaultdict
import importlib.util
from pathlib import Path
import re
import sys


def _channel_mapper():
    folder = Path(__file__).resolve().parent.parent / 'interiors'
    sys.path.insert(0, str(folder))
    spec = importlib.util.spec_from_file_location('_s47_owned_shader_bake', folder / 'build_owned.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.channel_material


def _black_material():
    import bpy
    mat = bpy.data.materials.new('_S47LD_BakeExcludedGlass')
    mat.use_nodes = True
    tree = mat.node_tree; tree.nodes.clear()
    shader = tree.nodes.new('ShaderNodeEmission')
    shader.inputs['Color'].default_value = (0, 0, 0, 1)
    tree.links.new(shader.outputs[0], tree.nodes.new('ShaderNodeOutputMaterial').inputs['Surface'])
    return mat


def bake_materials(rows, output, size=1024, samples=4):
    import bpy
    channel_material = _channel_mapper()
    groups = defaultdict(list)
    for row in rows:
        if row['kind'] == 'surface': groups[row['object']].append(row)
    texture_dir = output / 'textures'; texture_dir.mkdir()
    scene = bpy.context.scene
    previous = (scene.render.engine, scene.cycles.device, scene.cycles.samples)
    selected = list(bpy.context.selected_objects)
    active = bpy.context.view_layer.objects.active
    scene.render.engine = 'CYCLES'; scene.cycles.device = 'CPU'; scene.cycles.samples = samples
    scene.render.threads_mode = 'FIXED'; scene.render.threads = 4
    try:
        for object_name, wanted in groups.items():
            obj = bpy.data.objects[object_name]
            if not obj.data.uv_layers:
                raise ValueError('Bake requires existing non-overlapping UVs: ' + object_name)
            if any(not -.0001 <= x <= 1.0001 for uv in obj.data.uv_layers.active.data for x in uv.uv):
                raise ValueError('UVs outside 0..1 need a separate atlas/UDIM plan: ' + object_name)
            original = [(slot.link, slot.material) for slot in obj.material_slots]
            if any(mat is None for _, mat in original):
                raise ValueError('Bake encountered empty material slot: ' + object_name)
            glass_slots = {r['slot'] for r in rows if r['object'] == object_name and r['kind'] == 'glass'}
            stem = re.sub(r'[^A-Za-z0-9_]', '_', object_name)
            # Include a stable suffix so distinct Unicode/punctuated names cannot collide.
            import hashlib
            stem += '_' + hashlib.sha256(object_name.encode()).hexdigest()[:8]
            textures = {}
            for channel in ('base_color', 'roughness', 'metallic', 'normal'):
                temporary, image = [], None
                try:
                    image = bpy.data.images.new('_S47LD_Bake_' + channel, width=size, height=size,
                                                alpha=False, float_buffer=True)
                    image.colorspace_settings.name = 'sRGB' if channel == 'base_color' else 'Non-Color'
                    for i, (_, source) in enumerate(original):
                        if i in glass_slots:
                            mat = _black_material()
                        else:
                            mat = source.copy() if channel == 'normal' else channel_material(source, channel)
                        temporary.append(mat)
                        node = mat.node_tree.nodes.new('ShaderNodeTexImage'); node.image = image
                        mat.node_tree.nodes.active = node
                        obj.material_slots[i].link = 'OBJECT'
                        obj.material_slots[i].material = mat
                    bpy.ops.object.select_all(action='DESELECT')
                    obj.select_set(True); bpy.context.view_layer.objects.active = obj
                    settings = scene.render.bake
                    settings.use_selected_to_active = False
                    settings.use_clear = True
                    settings.margin = max(2, min(16, size // 64))
                    settings.normal_space = 'TANGENT'
                    settings.normal_r = 'POS_X'; settings.normal_g = 'NEG_Y'; settings.normal_b = 'POS_Z'
                    bpy.ops.object.bake(type='NORMAL' if channel == 'normal' else 'EMIT')
                    path = texture_dir / (stem + '_' + channel + '.png')
                    image.filepath_raw = str(path); image.file_format = 'PNG'; image.save()
                    textures[channel] = str(path)
                finally:
                    for slot, (link, mat) in zip(obj.material_slots, original):
                        slot.link = 'OBJECT'; slot.material = mat; slot.link = link
                    for mat in temporary: bpy.data.materials.remove(mat)
                    if image is not None: bpy.data.images.remove(image)
            for row in wanted:
                row['textures'] = textures.copy()
                row['normal_convention'] = 'directx'
                row['bake_uv'] = obj.data.uv_layers.active.name
                row['uv_overlap_status'] = 'Uses source UV; arbitrary overlap not repaired or certified'
    finally:
        bpy.ops.object.select_all(action='DESELECT')
        for obj in selected: obj.select_set(True)
        bpy.context.view_layer.objects.active = active
        scene.render.engine, scene.cycles.device, scene.cycles.samples = previous


def export_material(row):
    """FBX-friendly opaque maps and glass scalar hints; UE adapter owns glass."""
    import bpy
    mat = bpy.data.materials.new('_S47LD_FBX_' + row['material_name'])
    mat.use_nodes = True
    tree = mat.node_tree; bsdf = tree.nodes.get('Principled BSDF')
    if row['kind'] == 'glass':
        glass = row['glass']
        bsdf.inputs['Base Color'].default_value = glass['tint']
        bsdf.inputs['Roughness'].default_value = glass['roughness']
        bsdf.inputs['IOR'].default_value = glass['ior']
        bsdf.inputs['Transmission Weight'].default_value = 1
        return mat
    if not row.get('textures'):
        raise ValueError('Opaque procedural material needs baked maps for FBX: ' + row['material_name'])
    for channel, socket in [('base_color', 'Base Color'), ('roughness', 'Roughness'), ('metallic', 'Metallic')]:
        texture = tree.nodes.new('ShaderNodeTexImage')
        texture.image = bpy.data.images.load(row['textures'][channel], check_existing=False)
        texture.image.colorspace_settings.name = 'sRGB' if channel == 'base_color' else 'Non-Color'
        tree.links.new(texture.outputs['Color'], bsdf.inputs[socket])
    # Normal textures remain explicit in the manifest. Blender expects OpenGL
    # tangent normals, whereas our existing Unreal export uses DirectX Y-minus.
    return mat
