"""Discover the four actual CRT instances and clip a dedicated copy of their parent.

UnrealEditor-Cmd <project> -ExecutePythonScript=<this> -unattended -nullrhi
S47_CRT_CLIP_REPORT optionally selects the small JSON report.
The exporter imports the same SDF builder for regeneration stability.
"""
import json
import os
import hashlib
from pathlib import Path
import traceback

# SS47ScreenView::PicW/PicH and PictureContains. Idle 512x384 CRT targets
# have the same aspect ratio, so they use precisely the same UV aperture.
WIDTH, HEIGHT, INSET, RADIUS = 1280, 960, .035, .06
SOURCE_TAG = 'S47_CRT_ALPHA_SOURCE'
MASK_TAG = 'S47_CRT_APERTURE_MASK'
OUTPUT_TAG = 'S47_CRT_APERTURE_ALPHA'
APERTURE_CODE = '''float2 extent = float2(1280.0, 960.0);
float radius = 960.0 * 0.06;
float2 q = abs(UV * extent - extent * 0.5) - (extent * (0.5 - 0.035) - radius);
float distance = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
return distance <= 0.0 ? 1.0 : 0.0;'''


def aperture(material, alpha, uv):
    """Retain the original alpha path; append one idempotently tagged SDF."""
    import unreal
    M = unreal.MaterialEditingLibrary
    nodes = list(M.get_material_expressions(material))
    def tagged(tag):
        rows = [n for n in nodes if str(n.get_editor_property('desc')) == tag]
        assert len(rows) <= 1, ('Duplicate CRT clip node', tag)
        return rows[0] if rows else None
    alpha.set_editor_property('desc', SOURCE_TAG)
    mask = tagged(MASK_TAG) or M.create_material_expression(material, unreal.MaterialExpressionCustom, -650, 1250)
    mask.set_editor_property('desc', MASK_TAG)
    mask.set_editor_property('code', APERTURE_CODE)
    mask.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    pin = unreal.CustomInput()
    pin.set_editor_property('input_name', 'UV')
    mask.set_editor_property('inputs', [pin])
    assert M.connect_material_expressions(uv, '', mask, 'UV')
    output = tagged(OUTPUT_TAG) or M.create_material_expression(material, unreal.MaterialExpressionMultiply, -300, 1250)
    output.set_editor_property('desc', OUTPUT_TAG)
    assert M.connect_material_expressions(alpha, '', output, 'A')
    assert M.connect_material_expressions(mask, '', output, 'B')
    return output


def main():
    import unreal
    E, M = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary
    content = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
    manifest = json.loads((content / 'WebData/manifest.json').read_text())
    screens = ('crtLeft', 'crtCenter', 'crtRight', 'crtOps')
    instances = []
    for screen in screens:
        entry = manifest['handles'].get('room.' + screen + '.tex', {})
        if 'mat' not in entry:entry = manifest['handles'].get('room.' + screen + '.mat', {})
        assert isinstance(entry.get('mat'), int), ('CRT material handle missing', screen)
        instance = E.load_asset('/Game/Web/Mat/MI_W%d' % entry['mat'])
        assert isinstance(instance, unreal.MaterialInstanceConstant), screen
        instances.append(instance)
    assert len({m.get_path_name() for m in instances}) == 4, 'Expected four distinct CRT material instances'
    parents = {m.get_editor_property('parent').get_path_name() for m in instances}
    assert len(parents) == 1, ('CRT parents need individual inspection', parents)
    source = instances[0].get_editor_property('parent')
    assert isinstance(source, unreal.Material), 'An actual shared base material is required'
    source_path = source.get_path_name()
    destination = '/Game/Web/Base/M_WebCRT'
    source_file = content / (source_path.split('.')[0].removeprefix('/Game/') + '.uasset')
    assert source_file.is_file(), 'Cannot protect actual source material bytes'
    source_hash = hashlib.sha256(source_file.read_bytes()).hexdigest()
    def parameters(instance):
        result = {}
        for prop in ('scalar_parameter_values', 'vector_parameter_values', 'texture_parameter_values'):
            rows = []
            for row in instance.get_editor_property(prop):
                info = row.get_editor_property('parameter_info')
                value = row.get_editor_property('parameter_value')
                if isinstance(value, unreal.LinearColor):value = [value.r, value.g, value.b, value.a]
                elif isinstance(value, unreal.Object):value = value.get_path_name()
                rows.append([str(info.get_editor_property('name')), str(info.get_editor_property('association')),
                             info.get_editor_property('index'), value])
            result[prop] = rows
        overrides = instance.get_editor_property('base_property_overrides')
        assert not overrides.get_editor_property('override_blend_mode'), 'Explicit instance blend override needs review'
        return result
    saved_parameters = {m.get_path_name(): parameters(m) for m in instances}
    created = False
    if source_path.split('.')[0] == destination:
        material = source
    elif E.does_asset_exist(destination):
        material = E.load_asset(destination)
        assert E.get_metadata_tag(material, 'S47CRTSourceParent') == source_path, 'Refuse to overwrite unrelated CRT asset'
    else:
        material = E.duplicate_asset(source_path.split('.')[0], destination)
        assert isinstance(material, unreal.Material), 'Could not copy actual CRT parent'
        E.set_metadata_tag(material, 'S47CRTSourceParent', source_path)
        assert E.save_loaded_asset(material, False), 'Could not retain dedicated-copy provenance for retry'
        created = True
    properties = ('shading_model', 'two_sided', 'tangent_space_normal', 'opacity_mask_clip_value')
    before = {k: str(material.get_editor_property(k)) for k in properties}
    connections = (unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialProperty.MP_ROUGHNESS,
                   unreal.MaterialProperty.MP_SPECULAR, unreal.MaterialProperty.MP_METALLIC,
                   unreal.MaterialProperty.MP_NORMAL, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    def wires():
        result = {}
        for prop in connections:
            node = M.get_material_property_input_node(material, prop)
            result[str(prop)] = [node.get_path_name() if node else None,
                                 M.get_material_property_input_node_output_name(material, prop)]
        return result
    before_wires = wires()
    nodes = list(M.get_material_expressions(material))
    tagged_source = [n for n in nodes if str(n.get_editor_property('desc')) == SOURCE_TAG]
    assert len(tagged_source) <= 1
    alpha = tagged_source[0] if tagged_source else M.get_material_property_input_node(material, unreal.MaterialProperty.MP_OPACITY_MASK)
    if alpha is None:
        # Opaque exports still retain their complete unconnected alpha branch.
        # Identify that branch by all three original dependencies, never by a
        # fragile auto-generated node number or a substitute constant alpha.
        def dependencies(node, seen=None):
            seen = set() if seen is None else seen
            if not node or node.get_path_name() in seen:return []
            seen.add(node.get_path_name())
            result = [node]
            for item in M.get_inputs_for_material_expression(material, node):
                result.extend(dependencies(item, seen))
            return result
        candidates = []
        for node in nodes:
            if not isinstance(node, unreal.MaterialExpressionMultiply):continue
            deps = dependencies(node)
            names = {str(n.get_editor_property('parameter_name')) for n in deps
                     if isinstance(n, (unreal.MaterialExpressionScalarParameter, unreal.MaterialExpressionTextureSampleParameter2D))}
            if {'Opacity', 'AlphaMapTex', 'UseVertexColor'} <= names:candidates.append(node)
        assert len(candidates) == 1, ('Cannot uniquely retain the original CRT alpha branch', len(candidates))
        alpha = candidates[0]
    uv_nodes = [n for n in nodes if isinstance(n, unreal.MaterialExpressionTextureCoordinate)
                and n.get_editor_property('coordinate_index') == 0
                and n.get_editor_property('u_tiling') == 1 and n.get_editor_property('v_tiling') == 1]
    assert uv_nodes, 'Existing unmodified UV0 coordinate required'
    output = aperture(material, alpha, uv_nodes[0])
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    assert M.connect_material_property(output, '', unreal.MaterialProperty.MP_OPACITY_MASK)
    errors = list(M.recompile_material(material))
    assert not errors, errors
    after = {k: str(material.get_editor_property(k)) for k in properties}
    assert before == after, ('CRT glass properties changed', before, after)
    assert before_wires == wires(), 'CRT lighting, normal or phosphor graph connection changed'
    assert E.save_loaded_asset(material, False), 'Could not persist CRT aperture'
    instance_rows = []
    for instance in instances:
        instance.set_editor_property('parent', material)
        M.update_material_instance(instance)
        assert parameters(instance) == saved_parameters[instance.get_path_name()], 'CRT instance parameters changed'
        assert instance.get_editor_property('parent') == material
        assert E.save_loaded_asset(instance, False), instance.get_path_name()
        instance_rows.append({'instance': instance.get_path_name(), 'parent': material.get_path_name(),
                              'parameters_preserved': True})
    if source_path.split('.')[0] != destination:
        assert hashlib.sha256(source_file.read_bytes()).hexdigest() == source_hash, 'Shared source parent changed'
    report = {'status': 'PASS', 'material': material.get_path_name(),
              'source_parent': source_path, 'dedicated_copy_created': created,
              'shared_source_parent_bytes_preserved': source_path.split('.')[0] != destination,
              'crt_instances': instance_rows,
              'mask': {'width': WIDTH, 'height': HEIGHT, 'inset': INSET, 'radius_of_short_side': RADIUS},
              'preserved_properties': after, 'preserved_surface_connections': before_wires,
              'limits': 'Saved material graph only. Native four-screen render and pointer alignment remain required.'}
    return report


if __name__ == '__main__':
    filename = Path(os.environ.get('S47_CRT_CLIP_REPORT', '/tmp/s47-crt-clip.json'))
    success = filename.with_suffix('.success.json')
    filename.parent.mkdir(parents=True, exist_ok=True)
    for old in (filename, success):old.unlink(missing_ok=True)
    try:
        report = main()
        filename.write_text(json.dumps(report, indent=2) + '\n')
        success.write_text(json.dumps({'status': 'PASS', 'report': str(filename),
                                       'material': report['material'], 'crt_instances': report['crt_instances']}, indent=2) + '\n')
        import unreal
        unreal.log('S47 CRT APERTURE PATCH PASS ' + str(success))
    except Exception:
        filename.write_text(json.dumps({'status': 'FAIL', 'traceback': traceback.format_exc()}, indent=2) + '\n')
        raise
