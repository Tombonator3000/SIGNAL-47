"""Read-only native sky capture graph verification. No compile, save or mutation.
Root runs a fresh guarded Unreal commandlet and sets S47_SKY_CAPTURE_VERIFY_REPORT.
PASS is graph/source agreement only, not GPU reliability or visual acceptance.
"""
from pathlib import Path
import json
import os
import sys
import unreal

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from sky_capture import CAPTURE_SHA256, FINITE_SHA256, BRANCH, sha256

ASSET = '/Game/CalicheStreet/Atmosphere/M_Caliche_Sky'
path = Path(os.environ['S47_SKY_CAPTURE_VERIFY_REPORT'])
assert not path.exists(), f'Refusing evidence overwrite: {path}'
report = {'status': 'RUNNING', 'asset': ASSET, 'checks': [],
          'scope': 'Read-only source/graph proof; GPU and visual acceptance are separate.'}


def check(label, ok):
    report['checks'].append({'check': label, 'status': 'PASS' if ok else 'FAIL'})
    assert ok, label


def sources_with_names(mat, expression):
    M = unreal.MaterialEditingLibrary
    names = list(M.get_material_expression_input_names(expression))
    sources = list(M.get_inputs_for_material_expression(mat, expression))
    assert len(names) == len(sources), 'Input names and sources differ'
    return dict(zip(names, sources))


try:
    M = unreal.MaterialEditingLibrary
    mat = unreal.EditorAssetLibrary.load_asset(ASSET)
    check('native_material_present', bool(mat))
    nodes = list(M.get_material_expressions(mat))
    expressions = [n for n in nodes if isinstance(n, unreal.MaterialExpressionCustom)
                   and sha256(str(n.get_editor_property('code'))) == CAPTURE_SHA256]
    check('exact_capture_custom_code_sha', len(expressions) == 1)
    expression = expressions[0]
    code = str(expression.get_editor_property('code'))
    check('finite_main_body_preserved', code.count(BRANCH) == 1 and sha256(code.replace(BRANCH, '', 1)) == FINITE_SHA256)
    inputs = sources_with_names(mat, expression)
    check('all_custom_inputs_connected', all(value is not None for value in inputs.values()))
    switch = inputs.get('IsCapture')
    check('IsCapture_uses_official_switch', isinstance(switch, unreal.MaterialExpressionReflectionCapturePassSwitch))
    switch_inputs = sources_with_names(mat, switch)
    check('switch_has_Default_Reflection_inputs', set(switch_inputs) == {'Default', 'Reflection'})
    for pin, expected in [('Default', 0.), ('Reflection', 1.)]:
        value = switch_inputs[pin]
        check(pin + '_constant_type', isinstance(value, unreal.MaterialExpressionConstant))
        check(pin + '_constant_value', float(value.get_editor_property('r')) == expected)
    check('single_capture_switch', sum(isinstance(n, unreal.MaterialExpressionReflectionCapturePassSwitch) for n in nodes) == 1)
    check('material_still_sky', bool(mat.get_editor_property('is_sky')))
    check('material_still_opaque', mat.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_OPAQUE)
    check('material_still_unlit', mat.get_editor_property('shading_model') == unreal.MaterialShadingModel.MSM_UNLIT)
    report.update(status='PASS', custom_code_sha256=sha256(code), node_count=len(nodes),
                  custom_input_count=len(inputs), expression=expression.get_path_name(),
                  switch=switch.get_path_name(), saved_assets=[], modified_assets=[])
    unreal.log('S47 SKY CAPTURE VERIFY PASS ' + json.dumps(report))
except BaseException as error:
    report.update(status='FAIL', error=repr(error))
    raise
finally:
    path.write_text(json.dumps(report, indent=2) + '\n')
