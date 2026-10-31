"""Build the isolated finale ground A/B; run only in the root-owned editor job.

Source Web assets are read only. The candidate retains the real ground MIC's
static switches and parameter values; runtime owns both private MIDs and restore.
FinalePlanarNormal defaults to zero. All original normal branches remain intact;
one final world-space rotation levels their macro normal without changing detail.
"""
from pathlib import Path
import unreal


HERE = Path(__file__).resolve().parent
SOURCE = '/Game/Web/Base/M_Web_Lit_Opaque'
SOURCE_MI = '/Game/Web/Mat/MI_W774'
TARGET = '/Game/FinaleSky/M_S47_FinalGround'
TARGET_MI = '/Game/FinaleSky/MI_S47_FinalGround'
PARAMETER = 'FinalePlanarNormal'
PARAM_ARRAYS = ('scalar_parameter_values', 'vector_parameter_values', 'double_vector_parameter_values',
                'texture_parameter_values', 'texture_collection_parameter_values', 'parameter_collection_parameter_values',
                'runtime_virtual_texture_parameter_values', 'sparse_volume_texture_parameter_values', 'font_parameter_values')
ROTATE_INPUTS = ['MacroN', 'NormalWS', PARAMETER]
SELECT_INPUTS = ['Original', 'Corrected', 'MacroN', PARAMETER]
SELECT_CODE = '''// Preserve the original tangent output exactly when correction is inactive.
if (FinalePlanarNormal <= 0.0 || normalize(MacroN).z <= 0.9975640503)
    return Original;
return Corrected;'''
TAG = 'S47FinaleGround:'
E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary


def require(condition, message):
    if not condition:
        raise RuntimeError('S47 FINALE GROUND: ' + message)


def switches(instance):
    return {str(name): bool(M.get_material_instance_static_switch_parameter_value(instance, name))
            for name in M.get_static_switch_parameter_names(instance)}


def parameter_arrays(instance):
    # Parent updates legitimately rebind ExpressionGUIDs to the duplicated graph.
    # Compare the real parameter identity/value, not that editor bookkeeping.
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
    return result


def own_copy(source_path, target_path, cls):
    asset = E.load_asset(target_path) if E.does_asset_exist(target_path) else E.duplicate_asset(source_path, target_path)
    require(isinstance(asset, cls), 'missing or wrong candidate type: ' + target_path)
    require(asset.get_path_name() == target_path + '.' + target_path.rsplit('/', 1)[1],
            'candidate path differs: ' + target_path)
    return asset


def main():
    source = E.load_asset(SOURCE)
    source_mi = E.load_asset(SOURCE_MI)
    require(isinstance(source, unreal.Material), 'source opaque Web master missing')
    require(isinstance(source_mi, unreal.MaterialInstanceConstant), 'source ground MIC missing')
    require(source_mi.get_editor_property('parent') == source, 'source MIC has an unexpected parent')
    require(source.get_editor_property('tangent_space_normal'), 'expected world-to-tangent normal graph')
    expected_switches = switches(source_mi)
    expected_arrays = parameter_arrays(source_mi)
    hlsl = (HERE / 'ground_normal.hlsl').read_text()
    source_normal = M.get_material_property_input_node(source, unreal.MaterialProperty.MP_NORMAL)
    require(source_normal is not None and source_normal.get_outer() == source,
            'source must own its actual connected MP_Normal node')
    source_output = M.get_material_property_input_node_output_name(source, unreal.MaterialProperty.MP_NORMAL)
    unreal.log('S47 FINALE GROUND SOURCE ' + source_mi.get_path_name() + '; normal='
               + source_normal.get_path_name() + '; output=' + source_output + '; switches=' + str(expected_switches))

    mat = own_copy(SOURCE, TARGET, unreal.Material)
    require(mat != source, 'candidate is the shared source material')
    for name in ('material_domain', 'blend_mode', 'two_sided', 'tangent_space_normal', 'shading_model'):
        require(mat.get_editor_property(name) == source.get_editor_property(name),
                'candidate changed source material property: ' + name)
    expressions = list(M.get_material_expressions(mat))
    require(all(node.get_outer() == mat for node in expressions), 'duplicate must own every expression')
    current = M.get_material_property_input_node(mat, unreal.MaterialProperty.MP_NORMAL)
    if current and str(current.get_editor_property('desc')) == TAG + 'Select':
        require(list(M.get_material_expression_input_names(current)) == SELECT_INPUTS,
                'existing candidate has an unexpected final selector')
        inputs = list(M.get_inputs_for_material_expression(mat, current))
        require(len(inputs) == 4 and inputs[0] is not None, 'existing candidate lost its original normal')
        original = inputs[0]
    else:
        original = current
    require(original is not None and original.get_outer() == mat
            and original.get_class() == source_normal.get_class()
            and original.get_name() == source_normal.get_name(),
            'candidate must retain the actual source normal node and its type')
    # A stale candidate from the earlier SurfaceDetail-only recipe is not a clean
    # baseline. Do not silently keep its modified branch in this common-output A/B.
    require(not any(isinstance(node, unreal.MaterialExpressionCustom)
                    and PARAMETER in M.get_material_expression_input_names(node)
                    and not str(node.get_editor_property('desc')).startswith(TAG)
                    for node in expressions), 'candidate contains an older modified normal branch; recreate only the own candidate')

    def node(cls, label):
        matches = [item for item in expressions if str(item.get_editor_property('desc')) == TAG + label]
        require(len(matches) <= 1, 'duplicate candidate node: ' + label)
        item = matches[0] if matches else M.create_material_expression(mat, cls)
        require(isinstance(item, cls) and item.get_outer() == mat, 'candidate must own node: ' + label)
        if not matches:
            item.set_editor_property('desc', TAG + label)
            expressions.append(item)
        return item

    def connect(output, target, pin):
        # Preserve the exact original output channel (RGB for the live NormalMap),
        # rather than assuming a static-switch output or a particular source graph.
        name = source_output if output == original else ''
        require(M.connect_material_expressions(output, name, target, pin), 'cannot connect candidate pin: ' + pin)

    def custom(label, names, code):
        item = node(unreal.MaterialExpressionCustom, label)
        rows = []
        for name in names:
            pin = unreal.CustomInput()
            pin.set_editor_property('input_name', name)
            rows.append(pin)
        item.set_editor_property('inputs', rows)
        item.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        item.set_editor_property('code', code)
        return item

    planar = node(unreal.MaterialExpressionScalarParameter, 'Parameter')
    planar.set_editor_property('parameter_name', PARAMETER)
    planar.set_editor_property('default_value', 0.0)
    macro = node(unreal.MaterialExpressionVertexNormalWS, 'Macro')
    to_world = node(unreal.MaterialExpressionTransform, 'ToWorld')
    to_world.set_editor_property('transform_source_type', unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_TANGENT)
    to_world.set_editor_property('transform_type', unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    connect(original, to_world, '')
    rotated = custom('Rotate', ROTATE_INPUTS, hlsl)
    connect(macro, rotated, 'MacroN')
    connect(to_world, rotated, 'NormalWS')
    connect(planar, rotated, PARAMETER)
    to_tangent = node(unreal.MaterialExpressionTransform, 'ToTangent')
    to_tangent.set_editor_property('transform_source_type', unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD)
    to_tangent.set_editor_property('transform_type', unreal.MaterialVectorCoordTransform.TRANSFORM_TANGENT)
    connect(rotated, to_tangent, '')
    selected = custom('Select', SELECT_INPUTS, SELECT_CODE)
    for output, pin in ((original, 'Original'), (to_tangent, 'Corrected'), (macro, 'MacroN'), (planar, PARAMETER)):
        connect(output, selected, pin)
    require(M.connect_material_property(selected, '', unreal.MaterialProperty.MP_NORMAL), 'cannot connect final normal')
    require(M.get_material_property_input_node(mat, unreal.MaterialProperty.MP_NORMAL) == selected,
            'candidate final normal was not installed')
    require(list(M.get_inputs_for_material_expression(mat, selected)) == [original, to_tangent, macro, planar],
            'candidate final selector connections differ')

    mi = own_copy(SOURCE_MI, TARGET_MI, unreal.MaterialInstanceConstant)
    require(mi != source_mi, 'candidate is the shared source MIC')
    require(mi.get_editor_property('parent') in (source, mat), 'candidate MIC has an unexpected parent')
    errors = list(M.recompile_material(mat))
    require(not errors, 'candidate shader compile failed: ' + '; '.join(errors))
    M.set_material_instance_parent(mi, mat)
    M.update_material_instance(mi)
    require(switches(mi) == expected_switches, 'static-switch permutation changed after parent update')
    actual_arrays = parameter_arrays(mi)
    require(actual_arrays == expected_arrays, 'source MIC parameter values changed after parent update: '
            + str({name: (expected_arrays[name], actual_arrays[name]) for name in PARAM_ARRAYS
                   if expected_arrays[name] != actual_arrays[name]}))
    require(switches(source_mi) == expected_switches and parameter_arrays(source_mi) == expected_arrays,
            'shared source MIC changed')
    require(mi.get_editor_property('parent') == mat, 'candidate MIC parent was not retained')
    require(E.save_loaded_asset(mat, False) and E.save_loaded_asset(mi, False), 'candidate asset save failed')
    unreal.log('S47 FINALE GROUND COMPLETE; own master/MIC; FinalePlanarNormal=0; '
               'common final-normal Rodrigues rotation; exact original TS at 0 or >=4deg; '
               '2..4deg slope gate; static switches and source MIC parameter identities/values preserved: '
               + str(expected_switches))


if __name__ == '__main__':
    main()
