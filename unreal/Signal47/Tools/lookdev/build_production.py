"""Build scoped native SARO/motel material variants from their real source graphs.

Run only in the root-reserved UE5.8.4 editor/commandlet. This script does not
modify a mesh, map, source material, source texture, gameplay MID or Config file.
It writes only /Game/Signal47/Art/LookdevProduction and a private Saved report.

Original BaseColor/Roughness texture branches are wrapped, not reconstructed
from constant metadata. Normal/AO/opacity/emissive branches remain intact for
opaque surfaces. A tiny deterministic linear mask costs one texture sample;
there is no runtime Noise expression, WPO or extra material permutation switch.
Motel's mixed office atlas keeps BaseColor exactly, preserving printed labels.
The only translucent exception is its explicitly reviewed native glass assembly;
Room6's live original window, emissive atlases, CRTs, UI and text materials are
excluded. Thin-pane glass has Fresnel opacity and no screen-warping refraction.

MICs, if selected sources acquire them later, are duplicated with their complete
parent chain. Original parameter identities/values and static switches are
checked after parenting, following finalesky/update_ground_material.py.
No video author's source scripts are claimed or copied.
"""
from __future__ import annotations

import hashlib
import json
import math
import os
from pathlib import Path
import random
import struct
import zlib

import unreal

PROJECT = Path(__file__).resolve().parents[2]
DEST = '/Game/Signal47/Art/LookdevProduction'
TAG = 'S47LookdevProduction:'
OWNER = 'S47LookdevProductionOwner'
SOURCE_TAG = 'S47LookdevProductionSource'
SOURCE_SHA = 'S47LookdevProductionSourceSHA'
VERSION = '1'
E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary
PARAM_ARRAYS = ('scalar_parameter_values', 'vector_parameter_values', 'double_vector_parameter_values',
                'texture_parameter_values', 'texture_collection_parameter_values', 'parameter_collection_parameter_values',
                'runtime_virtual_texture_parameter_values', 'sparse_volume_texture_parameter_values', 'font_parameter_values')
SARO_PREFIXES = ['SM_Office_', 'SM_Saro_CorridorFinishes', 'SM_Saro_ControlRoomFinishes']
# Several Office graphs share this previously local normal asset, now included
# with the source assets so regeneration also works in a clean checkout.
# Own only this exact dependency where a selected clone actually references it.
LOCAL_NORMAL_DEPENDENCY = {
    'source': '/Game/Signal47/Art/Office/Materials/Textures/T_M_Office_CreamEnamel_normal',
    'replacement': DEST + '/Textures/T_LDP_Office_CreamEnamel_Normal',
    'reason': 'Selected Office graphs share the CreamEnamel normal; '
              'production clones keep one exact owned dependency while the original remains unchanged.',
}
PROFILES = {
    # strength is a small blend towards the dirt finish, not replacement color.
    'paint': {'color_amount': .075, 'roughness_amount': .14, 'roughness_target': .97,
              'tint': [.72, .70, .66], 'uv_scale': 4.0},
    'wood': {'color_amount': .055, 'roughness_amount': .16, 'roughness_target': .82,
             'tint': [.75, .70, .63], 'uv_scale': 5.0},
    'metal': {'color_amount': .045, 'roughness_amount': .13, 'roughness_target': .66,
              'tint': [.76, .75, .72], 'uv_scale': 5.0, 'metallic_target': .32, 'metallic_amount': .045},
    'floor': {'color_amount': .045, 'roughness_amount': .16, 'roughness_target': .88,
              'tint': [.78, .77, .73], 'uv_scale': 7.0},
    'motel_atlas': {'color_amount': 0.0, 'roughness_amount': .12, 'roughness_target': .85,
                    'tint': [1.0, 1.0, 1.0], 'uv_scale': 8.0},
    'motel_shell': {'color_amount': .07, 'roughness_amount': .12, 'roughness_target': .95,
                    'tint': [.73, .70, .64], 'uv_scale': 6.0},
}


def require(condition, message):
    if not condition:
        raise RuntimeError('S47 LOOKDEV PRODUCTION: ' + message)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def package_file(asset):
    path = asset.get_path_name().split('.')[0]
    require(path.startswith('/Game/'), 'only project-owned sources may be selected: ' + path)
    file = PROJECT / 'Content' / (path[len('/Game/'):] + '.uasset')
    require(file.is_file(), 'source package missing: ' + str(file))
    return file


def plan():
    rows = []
    def add(scope, root, names, profile, prefixes):
        for name in names:
            rows.append({'scope': scope, 'source': root + '/' + name,
                         'profile': profile, 'mesh_prefixes': [
                             ('/Game/MotelV2/' if scope == 'motel' else
                              '/Game/Signal47/Art/Office/' if prefix.startswith('SM_Office_') else
                              '/Game/Signal47/Art/SaroInteriors/') + prefix for prefix in prefixes]})
    room = '/Game/Signal47/Art/SaroInteriors/Materials'
    office = '/Game/Signal47/Art/Office/Materials'
    motel = '/Game/MotelV2/Materials'
    add('saro', room, ['M_Saro_Control_UpperPaint', 'M_Saro_Control_LowerPaint',
                       'M_Saro_Control_RackSteel'], 'paint', ['SM_Saro_ControlRoomFinishes'])
    add('saro', room, ['M_Saro_Corridor_UpperPaint'], 'paint', ['SM_Saro_CorridorFinishes'])
    add('saro', room, ['M_Saro_Corridor_VCT', 'M_Saro_Corridor_VCTFaded',
                       'M_Saro_Corridor_VCTRepair'], 'floor', ['SM_Saro_CorridorFinishes'])
    add('saro', office, ['M_Office_SageWall', 'M_Office_CreamEnamel',
                         'M_Office_TankerGreen', 'M_Office_CabinetEnamel'], 'paint', SARO_PREFIXES)
    add('saro', office, ['M_Office_Walnut'], 'wood', SARO_PREFIXES)
    add('saro', office, ['M_Office_BrushedMetal'], 'metal', SARO_PREFIXES)
    add('saro', office, ['M_Office_FloorLinoleum'], 'floor', SARO_PREFIXES)
    add('motel', motel, ['M_MotelV2_Office'], 'motel_atlas', ['SM_Sierra_Office'])
    add('motel', motel, ['M_MotelV2_RoofShell'], 'motel_shell', ['SM_Sierra_RoofShell'])
    add('motel', motel, ['M_MotelV2_Glass'], 'pane', ['SM_Sierra_Glass'])
    return rows


def chain(source):
    result = []
    seen = set()
    while isinstance(source, unreal.MaterialInstanceConstant):
        require(source.get_path_name() not in seen, 'cyclic material instance parents')
        seen.add(source.get_path_name())
        result.append(source)
        source = source.get_editor_property('parent')
    require(isinstance(source, unreal.Material), 'source must resolve to a native base material')
    return source, result


def switches(instance):
    return {str(name): bool(M.get_material_instance_static_switch_parameter_value(instance, name))
            for name in M.get_static_switch_parameter_names(instance)}


def parameter_arrays(instance):
    # ExpressionGUIDs can legitimately rebind after changing a duplicated parent.
    def value(item):
        if isinstance(item, unreal.Object):
            return item.get_path_name()
        if isinstance(item, unreal.StructBase):
            return item.export_text()
        return item
    result = {}
    for name in PARAM_ARRAYS:
        rows = []
        for item in instance.get_editor_property(name):
            identity = item.get_editor_property('parameter_info').export_text()
            if name == 'font_parameter_values':
                payload = (value(item.get_editor_property('font_value')), item.get_editor_property('font_page'))
            else:
                payload = value(item.get_editor_property('parameter_value'))
            rows.append((identity, payload))
        result[name] = rows
    result['base_property_overrides'] = instance.get_editor_property('base_property_overrides').export_text()
    return result


def source_port(material, prop):
    node = M.get_material_property_input_node(material, prop)
    return node, str(M.get_material_property_input_node_output_name(material, prop)) if node else ''


def zero_emissive(base, interface):
    """Accept only the project's proven zero-default emissive recipe.

    import_recipe always connects EmissiveFactor (RGB=0) and may multiply it
    by the color texture. Preserve that branch and its established parameter
    name. Unknown parameters/functions/drivers are not treated as zero, and a
    selected MIC with a nonzero effective override is rejected.
    """
    root, output = source_port(base, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if root is None:
        return True

    def rgb_zero(color, pin):
        channels = {'R': [color.r], 'G': [color.g], 'B': [color.b], 'A': [color.a]}
        values = channels.get(pin, [color.r, color.g, color.b])
        return all(abs(float(value)) <= 1e-8 for value in values)

    def prove(node, pin, seen):
        if node is None or node.get_path_name() in seen:
            return False
        seen = seen | {node.get_path_name()}
        if isinstance(node, unreal.MaterialExpressionConstant):
            return abs(float(node.get_editor_property('r'))) <= 1e-8
        if isinstance(node, (unreal.MaterialExpressionConstant3Vector, unreal.MaterialExpressionConstant4Vector)):
            return rgb_zero(node.get_editor_property('constant'), pin)
        if isinstance(node, unreal.MaterialExpressionVectorParameter):
            if str(node.get_editor_property('parameter_name')) != 'EmissiveFactor':
                return False
            if node.get_editor_property('use_custom_primitive_data'):
                return False
            value = (M.get_material_instance_vector_parameter_value(interface, 'EmissiveFactor')
                     if isinstance(interface, unreal.MaterialInstanceConstant)
                     else node.get_editor_property('default_value'))
            return rgb_zero(value, pin)
        if isinstance(node, unreal.MaterialExpressionMultiply):
            inputs = list(M.get_inputs_for_material_expression(base, node))
            if len(inputs) != 2:
                return False
            for index, item in enumerate(inputs):
                if item is None:
                    if abs(float(node.get_editor_property('const_a' if index == 0 else 'const_b'))) <= 1e-8:
                        return True
                else:
                    channel = M.get_input_node_output_name_for_material_expression(node, item)
                    if channel is not None and prove(item, str(channel), seen):
                        return True
        return False

    return prove(root, output, set())


def texture_bindings(material):
    return sorted((n.get_name(), n.get_editor_property('texture').get_path_name())
                  for n in M.get_material_expressions(material)
                  if isinstance(n, unreal.MaterialExpressionTextureSample)
                  and n.get_editor_property('texture') is not None
                  and not str(n.get_editor_property('desc')).startswith(TAG))


def original_port(source, target, prop, required=True):
    node, output = source_port(source, prop)
    if node is None:
        require(not required, 'source property is unconnected: ' + str(prop))
        return None, ''
    matches = [n for n in M.get_material_expressions(target)
               if n.get_name() == node.get_name() and n.get_class() == node.get_class()]
    require(len(matches) == 1 and matches[0].get_outer() == target,
            'duplicate must retain its actual original property node: ' + str(prop))
    return matches[0], output


def own_copy(source, destination):
    require(destination.startswith(DEST + '/'), 'write outside own package namespace')
    existed = E.does_asset_exist(destination)
    target = E.load_asset(destination) if existed else E.duplicate_asset(source.get_path_name(), destination)
    require(target is not None and target.get_class() == source.get_class(), 'could not duplicate source: ' + destination)
    require(target != source, 'candidate aliases source')
    sha = digest(package_file(source))
    if existed:
        require(E.get_metadata_tag(target, OWNER) == VERSION, 'existing asset is not owned by this recipe: ' + destination)
        require(E.get_metadata_tag(target, SOURCE_TAG) == source.get_path_name(), 'existing candidate source differs')
        require(E.get_metadata_tag(target, SOURCE_SHA) == sha,
                'source package changed; review/rebuild only the private candidate before continuing: ' + destination)
    E.set_metadata_tag(target, OWNER, VERSION)
    E.set_metadata_tag(target, SOURCE_TAG, source.get_path_name())
    E.set_metadata_tag(target, SOURCE_SHA, sha)
    return target


def local_normal_dependency(spec, target, restore=False):
    """Redirect one reviewed clone dependency, never its source or other graphs.

    Before graph verification, a previously redirected sample is temporarily
    restored to its exact source reference. After verification it is rebound to
    the private native duplicate again. This makes repeat builds idempotent and
    keeps the original branch-comparison strict, rather than broadly aliasing
    texture identities or weakening checks for the rest of the source graphs.
    """
    require(target.get_path_name().startswith(DEST + '/Materials/'), 'normal redirect must target our clone')
    original = E.load_asset(LOCAL_NORMAL_DEPENDENCY['source'])
    require(isinstance(original, unreal.Texture2D), 'reviewed source normal texture is missing')
    require(not original.get_editor_property('srgb')
            and original.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_NORMALMAP,
            'reviewed normal texture must already have valid linear/normal-map settings')
    replacement = (E.load_asset(LOCAL_NORMAL_DEPENDENCY['replacement'])
                   if E.does_asset_exist(LOCAL_NORMAL_DEPENDENCY['replacement']) else None)
    matching = [sample for sample in M.get_material_expressions(target)
                if isinstance(sample, unreal.MaterialExpressionTextureSample)
                and (sample.get_editor_property('texture') == original
                     or (replacement is not None and sample.get_editor_property('texture') == replacement))]
    if not matching:
        return []
    if restore:
        if replacement is not None:
            require(E.get_metadata_tag(replacement, OWNER) == VERSION, 'private normal destination has foreign ownership')
            for sample in matching:
                if sample.get_editor_property('texture') == replacement:
                    sample.set_editor_property('texture', original)
        return []
    replacement = own_copy(original, LOCAL_NORMAL_DEPENDENCY['replacement'])
    for field in ('srgb', 'compression_settings', 'flip_green_channel', 'mip_gen_settings',
                  'compression_no_alpha', 'virtual_texture_streaming'):
        require(replacement.get_editor_property(field) == original.get_editor_property(field),
                'normal dependency duplicate changed texture property: ' + field)
    dimensions = [original.blueprint_get_size_x(), original.blueprint_get_size_y()]
    require([replacement.blueprint_get_size_x(), replacement.blueprint_get_size_y()] == dimensions,
            'normal dependency duplicate changed texture dimensions')
    redirected = []
    for sample in matching:
        if sample.get_editor_property('texture') == original:
            require(sample.get_editor_property('sampler_type') == unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
                    'only an existing normal sampler may be redirected')
            sample.set_editor_property('texture', replacement)
            redirected.append(sample.get_name())
    require(bool(redirected), 'reviewed CreamEnamel normal sampler was not found in the clone')
    require(E.save_loaded_asset(replacement), 'could not save private normal dependency')
    return [{'source_material_path': spec['source'],
             'original_texture_path': original.get_path_name(),
             'copied_texture_path': replacement.get_path_name(),
             'reason': LOCAL_NORMAL_DEPENDENCY['reason'],
             'copy_method': 'native asset duplicate; no reimport, resampling, color conversion or pixel edit',
             'source_package_sha256': digest(package_file(original)),
             'dimensions_px': dimensions, 'srgb': False, 'compression': 'TC_NORMALMAP',
             'sample_nodes': redirected}]


def mask_pixels(size=128):
    """Seamless deterministic value noise; this is surface mottling, not edges."""
    rng = random.Random(470031)
    layers = []
    for count in (4, 16, 32):
        layers.append((count, [[rng.random() for _ in range(count)] for _ in range(count)]))
    def sample(layer, x, y):
        count, grid = layer
        x = x * count / size; y = y * count / size
        ix, iy = math.floor(x), math.floor(y)
        fx, fy = x - ix, y - iy
        fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy)
        a = grid[iy % count][ix % count] * (1 - fx) + grid[iy % count][(ix + 1) % count] * fx
        b = grid[(iy + 1) % count][ix % count] * (1 - fx) + grid[(iy + 1) % count][(ix + 1) % count] * fx
        return a * (1 - fy) + b * fy
    raw = bytearray()
    for y in range(size):
        raw.append(0)  # PNG filter type None
        for x in range(size):
            a, b, c = [sample(layer, x, y) for layer in layers]
            red = min(1, max(0, ((a * .55 + b * .3 + c * .15) - .28) / .5))
            green = min(1, max(0, (b - .35) / .45))
            raw.extend((round(red * 255), round(green * 255), round(c * 255), 255))
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', size, size, 8, 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(bytes(raw), 9)) + chunk(b'IEND', b''))


def ensure_mask(work):
    work.mkdir(parents=True, exist_ok=True)
    path = work / 's47-wear-mask.png'
    payload = mask_pixels()
    if not path.exists() or path.read_bytes() != payload:
        path.write_bytes(payload)
    sha = hashlib.sha256(payload).hexdigest()
    destination = DEST + '/Textures/T_S47LD_WearMask'
    existing = E.load_asset(destination) if E.does_asset_exist(destination) else None
    if existing:
        require(E.get_metadata_tag(existing, OWNER) == VERSION, 'wear mask destination is not owned')
    if not existing or E.get_metadata_tag(existing, SOURCE_SHA) != sha:
        task = unreal.AssetImportTask()
        task.filename = str(path); task.destination_path = DEST + '/Textures'
        task.destination_name = 'T_S47LD_WearMask'
        task.replace_existing = True; task.automated = True; task.save = False
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        existing = E.load_asset(destination)
    require(isinstance(existing, unreal.Texture2D), 'could not import private wear mask')
    existing.set_editor_property('srgb', False)
    existing.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
    E.set_metadata_tag(existing, OWNER, VERSION)
    E.set_metadata_tag(existing, SOURCE_SHA, sha)
    require(E.save_loaded_asset(existing), 'could not save private mask')
    return existing


class Graph:
    def __init__(self, material):
        self.material = material
        self.expressions = list(M.get_material_expressions(material))

    def node(self, cls, label, **properties):
        matches = [n for n in self.expressions if str(n.get_editor_property('desc')) == TAG + label]
        require(len(matches) <= 1, 'duplicate owned node: ' + label)
        n = matches[0] if matches else M.create_material_expression(self.material, cls)
        require(isinstance(n, cls) and n.get_outer() == self.material, 'wrong owned node type: ' + label)
        if not matches:
            n.set_editor_property('desc', TAG + label)
            n.set_editor_property('material_expression_editor_x', -500 + len(self.expressions) * 14)
            n.set_editor_property('material_expression_editor_y', 500)
            self.expressions.append(n)
        for key, value in properties.items():
            n.set_editor_property(key, value)
        return n

    def scalar(self, name, value):
        return self.node(unreal.MaterialExpressionScalarParameter, name,
                         parameter_name='S47LD_' + name, default_value=float(value))

    def wire(self, source, output, target, pin):
        require(M.connect_material_expressions(source, output, target, pin), 'failed node connection: ' + pin)

    def finish(self, source, output, prop):
        require(M.connect_material_property(source, output, prop), 'failed material connection: ' + str(prop))


def build_surface(source, target, profile, texture):
    graph = Graph(target)
    normal = original_port(source, target, unreal.MaterialProperty.MP_NORMAL, required=False)
    original_base = original_port(source, target, unreal.MaterialProperty.MP_BASE_COLOR)
    original_rough = original_port(source, target, unreal.MaterialProperty.MP_ROUGHNESS)
    uv = graph.node(unreal.MaterialExpressionTextureCoordinate, 'WearUV',
                    coordinate_index=0, u_tiling=profile['uv_scale'], v_tiling=profile['uv_scale'])
    noise = graph.node(unreal.MaterialExpressionTextureSample, 'WearMask', texture=texture,
                       sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    graph.wire(uv, '', noise, 'UVs')
    if profile['color_amount']:
        amount = graph.node(unreal.MaterialExpressionMultiply, 'ColorMask', const_b=profile['color_amount'])
        graph.wire(noise, 'R', amount, 'A')
        tint = graph.node(unreal.MaterialExpressionVectorParameter, 'WearTint',
                          parameter_name='S47LD_WearTint', default_value=unreal.LinearColor(*profile['tint'], 1))
        tinted = graph.node(unreal.MaterialExpressionMultiply, 'TintedSource')
        graph.wire(*original_base, tinted, 'A'); graph.wire(tint, 'RGB', tinted, 'B')
        blend = graph.node(unreal.MaterialExpressionLinearInterpolate, 'BaseColorBlend')
        graph.wire(*original_base, blend, 'A'); graph.wire(tinted, '', blend, 'B')
        graph.wire(amount, '', blend, 'Alpha'); graph.finish(blend, '', unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        graph.finish(*original_base, unreal.MaterialProperty.MP_BASE_COLOR)
    rough_amount = graph.node(unreal.MaterialExpressionMultiply, 'RoughnessMask', const_b=profile['roughness_amount'])
    graph.wire(noise, 'R', rough_amount, 'A')
    rough = graph.node(unreal.MaterialExpressionLinearInterpolate, 'RoughnessBlend', const_b=profile['roughness_target'])
    graph.wire(*original_rough, rough, 'A'); graph.wire(rough_amount, '', rough, 'Alpha')
    graph.finish(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    if 'metallic_target' in profile:
        original_metal = original_port(source, target, unreal.MaterialProperty.MP_METALLIC)
        metal_amount = graph.node(unreal.MaterialExpressionMultiply, 'MetallicMask', const_b=profile['metallic_amount'])
        graph.wire(noise, 'G', metal_amount, 'A')
        metal = graph.node(unreal.MaterialExpressionLinearInterpolate, 'MetallicBlend', const_b=profile['metallic_target'])
        graph.wire(*original_metal, metal, 'A'); graph.wire(metal_amount, '', metal, 'Alpha')
        graph.finish(metal, '', unreal.MaterialProperty.MP_METALLIC)
    require(source_port(target, unreal.MaterialProperty.MP_NORMAL) == normal, 'source normal branch changed')
    require(texture_bindings(target) == texture_bindings(source), 'source texture bindings changed')


def build_pane(source, target):
    # Only the reviewed opaque-infrastructure-independent native glass assembly.
    graph = Graph(target)
    original_opacity = original_port(source, target, unreal.MaterialProperty.MP_OPACITY)
    normal = original_port(source, target, unreal.MaterialProperty.MP_NORMAL, required=False)
    face = graph.node(unreal.MaterialExpressionMultiply, 'PaneFaceOpacity', const_b=.65)
    graph.wire(*original_opacity, face, 'A')
    fresnel = graph.node(unreal.MaterialExpressionFresnel, 'PaneFresnel', exponent=5.0, base_reflect_fraction=.04)
    opacity = graph.node(unreal.MaterialExpressionLinearInterpolate, 'PaneOpacity', const_b=.46)
    graph.wire(face, '', opacity, 'A'); graph.wire(fresnel, '', opacity, 'Alpha')
    graph.finish(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    graph.finish(graph.scalar('PaneIOR', 1.0), '', unreal.MaterialProperty.MP_REFRACTION)
    target.set_editor_property('refraction_method', unreal.RefractionMode.RM_INDEX_OF_REFRACTION)
    target.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    require(source_port(target, unreal.MaterialProperty.MP_NORMAL) == normal, 'pane changed source normal')
    require(texture_bindings(target) == texture_bindings(source), 'pane changed source texture bindings')
    require(original_port(source, target, unreal.MaterialProperty.MP_BASE_COLOR) == source_port(target, unreal.MaterialProperty.MP_BASE_COLOR),
            'pane changed source glass tint')


def validate_source(spec):
    source = E.load_asset(spec['source'])
    require(isinstance(source, unreal.MaterialInterface), 'missing explicit native source ' + spec['source'])
    base, instances = chain(source)
    require(base.get_editor_property('material_domain') == unreal.MaterialDomain.MD_SURFACE, 'non-surface source')
    require(not base.get_editor_property('use_material_attributes'), 'material-attributes source needs explicit review')
    require(zero_emissive(base, source),
            'nonzero or unproven emissive source is protected: ' + spec['source'])
    if spec['profile'] == 'pane':
        require(spec['source'] == '/Game/MotelV2/Materials/M_MotelV2_Glass'
                and spec['mesh_prefixes'] == ['/Game/MotelV2/SM_Sierra_Glass'], 'unreviewed transparent material')
        require(base.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_TRANSLUCENT, 'reviewed pane must remain translucent')
        require(source_port(base, unreal.MaterialProperty.MP_OPACITY)[0] is not None, 'pane opacity source missing')
    else:
        require(base.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_OPAQUE, 'non-opaque surface needs separate review')
        for prop in (unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialProperty.MP_ROUGHNESS):
            require(source_port(base, prop)[0] is not None, 'native source channel missing: ' + str(prop))
    require(spec['scope'] in ('saro', 'motel'), 'invalid binding scope')
    require(not any('Room06' in p or 'Room6' in p or 'Glow' in p for p in spec['mesh_prefixes']), 'protected mesh prefix')
    return source, base, instances


def build(output=None):
    work = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / 'LookdevProduction'
    output = Path(output or os.environ.get('S47_LOOKDEV_PRODUCTION_REPORT', work / 'build-report.json'))
    output.parent.mkdir(parents=True, exist_ok=True)
    specs = plan()
    prepared = [(spec, *validate_source(spec)) for spec in specs]
    protected = {}
    for _, source, base, instances in prepared:
        for asset in [source, base] + instances:
            file = package_file(asset); protected[str(file)] = digest(file)
    dependency = E.load_asset(LOCAL_NORMAL_DEPENDENCY['source'])
    require(isinstance(dependency, unreal.Texture2D), 'reviewed local normal dependency is missing')
    dependency_file = package_file(dependency)
    protected[str(dependency_file)] = digest(dependency_file)
    texture = ensure_mask(work / 'Sources')
    rows = []
    for spec, source, base, instances in prepared:
        key = source.get_name()
        base_path = DEST + '/Materials/M_LDP_' + key + ('_Base' if instances else '')
        target_base = own_copy(base, base_path)
        for name in ('material_domain', 'blend_mode', 'two_sided', 'tangent_space_normal', 'shading_model'):
            require(target_base.get_editor_property(name) == base.get_editor_property(name), 'source material property changed: ' + name)
        if instances:
            local_paths = {LOCAL_NORMAL_DEPENDENCY['source'], LOCAL_NORMAL_DEPENDENCY['replacement']}
            references_local = any(isinstance(sample, unreal.MaterialExpressionTextureSample)
                                   and sample.get_editor_property('texture') is not None
                                   and sample.get_editor_property('texture').get_path_name().split('.')[0] in local_paths
                                   for sample in M.get_material_expressions(target_base))
            require(not references_local, 'review texture-parameter overrides before redirecting this dependency on an MIC')
        local_normal_dependency(spec, target_base, restore=True)
        if spec['profile'] == 'pane':
            build_pane(base, target_base)
        else:
            build_surface(base, target_base, PROFILES[spec['profile']], texture)
        for prop in (unreal.MaterialProperty.MP_EMISSIVE_COLOR, unreal.MaterialProperty.MP_AMBIENT_OCCLUSION,
                     unreal.MaterialProperty.MP_OPACITY_MASK):
            require(source_port(target_base, prop) == original_port(base, target_base, prop, required=False),
                    'protected source branch changed: ' + str(prop))
        dependency_copies = local_normal_dependency(spec, target_base)
        errors = list(M.recompile_material(target_base))
        require(not errors, 'material compile failed ' + base_path + ': ' + str(errors))
        require(E.save_loaded_asset(target_base), 'could not save production material')
        replacement = target_base
        for index, instance in enumerate(reversed(instances)):
            arrays = parameter_arrays(instance); expected_switches = switches(instance)
            target_path = DEST + '/Materials/MI_LDP_' + key + ('_Parent' + str(index) if index < len(instances) - 1 else '')
            copied = own_copy(instance, target_path)
            M.set_material_instance_parent(copied, replacement); M.update_material_instance(copied)
            require(parameter_arrays(copied) == arrays, 'MIC parameter identities/values changed')
            require(switches(copied) == expected_switches, 'MIC static switch values changed')
            require(E.save_loaded_asset(copied), 'could not save production MIC')
            replacement = copied
        rows.append({'source_material_path': source.get_path_name(),
                     'replacement_material_path': replacement.get_path_name(),
                     'scope': spec['scope'], 'mesh_prefixes': spec['mesh_prefixes'],
                     'profile': spec['profile'], 'compile_errors': errors,
                     'source_texture_normal_graph_preserved': True,
                     'base_color_preserved_exactly': spec['profile'] in ('motel_atlas', 'pane'),
                     'base_material_path': target_base.get_path_name(),
                     'material_instance_chain_length': len(instances),
                     'extra_wear_texture_samples': 0 if spec['profile'] == 'pane' else 1,
                     'local_dependency_copies': dependency_copies})
    require(all(digest(file) == sha for file, sha in protected.items()), 'a protected source package changed')
    report = {'schema_version': 1, 'status': 'PASS', 'destination': DEST, 'bindings': rows,
              'wear_mask': texture.get_path_name(), 'source_packages_unchanged': len(protected),
              'local_dependency_copies': [copy for row in rows for copy in row['local_dependency_copies']],
              'protected': ['Room6 live glass', 'glow/emissive atlases', 'masked corridor wear overlay',
                            'CRT/UI/text materials', 'all source assets', 'all meshes, LODs and collision'],
              'notes': ['Motel office atlas retains original BaseColor; added variation affects roughness only.',
                        'Noise mask is UV-space mottled wear, not geometric edge detection.',
                        'No binding/map/runtime edit is performed; runtime consumes approved explicit rows.']}
    output.write_text(json.dumps(report, indent=2) + '\n')
    unreal.log('S47 LOOKDEV PRODUCTION PASS ' + str(len(rows)) + ' bindings; report=' + str(output))
    return report


if __name__ == '__main__':
    build()
