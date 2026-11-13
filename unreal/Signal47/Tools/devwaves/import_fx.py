"""DEV-WAVES: the two small effect materials of the F10 wave shooter (a dev toy), made here from nothing (no texture, no bought asset).

  M_DevWaves_Soft   unlit translucent soft round sprite: Color x Brightness, Opacity; for smoke, dust and the dark burst
  M_DevWaves_Decal  deferred decal, a soft round mark: Color, Opacity; for the short-lived bullet marks

Run through the queue with no editor open:
  UnrealEditor-Cmd Signal47.uproject -run=pythonscript -script=$PWD/Tools/devwaves/import_fx.py -unattended -nullrhi -nosplash -NoSound
"""
import unreal

DEST = '/Game/Signal47/Dev/Waves/Materials'
E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty


def fresh(name):
    path = DEST + '/' + name
    mat = E.load_asset(path) if E.does_asset_exist(path) else T.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    for previous in list(M.get_material_expressions(mat)):
        M.delete_material_expression(mat, previous)
    return mat


def node(mat, cls, **values):
    n = M.create_material_expression(mat, cls)
    for key, value in values.items():
        n.set_editor_property(key, value)
    return n


def link(a, out, b, pin):
    assert M.connect_material_expressions(a, out, b, pin), (a.get_name(), out, b.get_name(), pin)


def falloff(mat):
    """1 at the middle of the quad to 0 at its rim, soft: max(0, 1 - 4 r^2) squared."""
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate)
    off = node(mat, unreal.MaterialExpressionSubtract, const_b=0.5)
    link(uv, '', off, 'A')
    dot = node(mat, unreal.MaterialExpressionDotProduct)
    link(off, '', dot, 'A')
    link(off, '', dot, 'B')
    far = node(mat, unreal.MaterialExpressionMultiply, const_b=4.0)
    link(dot, '', far, 'A')
    inv = node(mat, unreal.MaterialExpressionSubtract, const_a=1.0)
    link(far, '', inv, 'B')
    sat = node(mat, unreal.MaterialExpressionMax, const_b=0.0)
    link(inv, '', sat, 'A')
    sq = node(mat, unreal.MaterialExpressionMultiply)
    link(sat, '', sq, 'A')
    link(sat, '', sq, 'B')
    return sq


def finish(mat, name):
    errors = list(M.recompile_material(mat))
    assert not errors, (name, errors)
    assert E.save_loaded_asset(mat, only_if_is_dirty=False), name
    unreal.log('S47: DEVWAVES FX ' + name + ' saved')


soft = fresh('M_DevWaves_Soft')
soft.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
soft.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
soft.set_editor_property('two_sided', True)
col = node(soft, unreal.MaterialExpressionVectorParameter, parameter_name='Color', default_value=unreal.LinearColor(0.2, 0.18, 0.15, 1))
bright = node(soft, unreal.MaterialExpressionScalarParameter, parameter_name='Brightness', default_value=1.0)
lit = node(soft, unreal.MaterialExpressionMultiply)
link(col, '', lit, 'A')
link(bright, '', lit, 'B')
assert M.connect_material_property(lit, '', MP.MP_EMISSIVE_COLOR)
op = node(soft, unreal.MaterialExpressionScalarParameter, parameter_name='Opacity', default_value=0.5)
alpha = node(soft, unreal.MaterialExpressionMultiply)
link(falloff(soft), '', alpha, 'A')
link(op, '', alpha, 'B')
assert M.connect_material_property(alpha, '', MP.MP_OPACITY)
finish(soft, 'M_DevWaves_Soft')

decal = fresh('M_DevWaves_Decal')
decal.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
decal.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
col = node(decal, unreal.MaterialExpressionVectorParameter, parameter_name='Color', default_value=unreal.LinearColor(0.012, 0.011, 0.010, 1))
assert M.connect_material_property(col, '', MP.MP_BASE_COLOR)
rough = node(decal, unreal.MaterialExpressionConstant, r=0.9)
assert M.connect_material_property(rough, '', MP.MP_ROUGHNESS)
op = node(decal, unreal.MaterialExpressionScalarParameter, parameter_name='Opacity', default_value=0.85)
alpha = node(decal, unreal.MaterialExpressionMultiply)
link(falloff(decal), '', alpha, 'A')
link(op, '', alpha, 'B')
assert M.connect_material_property(alpha, '', MP.MP_OPACITY)
finish(decal, 'M_DevWaves_Decal')
unreal.log('S47: DEVWAVES FX PASS')
