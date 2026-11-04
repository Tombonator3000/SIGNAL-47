"""SARO desert ground: three-layer world-projected blend (loose sand / pale caliche crust / gravel).
Builds /Game/Backdrop/M_S47_DesertGround. Texture slots are parameters (SandTex, CrustTex, GravelTex)
that default to the exported sand T4; Codex photo textures replace them by rebuilding with
S47_GROUND_TEXTURES=1 (expects /Game/Backdrop/Textures/T_Ground{Sand,Crust,Gravel}_D).
Base = VertexColor (authored paths/shading) x layer blend; masks come from the sand map at 150 m and 40 m."""
import json, os, traceback
from pathlib import Path
import unreal
DEST = '/Game/Backdrop'; NAME = 'M_S47_DesertGround'; OWNER = 'S47DesertGround_v1'
E = unreal.EditorAssetLibrary; L = unreal.MaterialEditingLibrary; T = unreal.AssetToolsHelpers.get_asset_tools()

def node(mat, cls, x, y, **p):
    n = L.create_material_expression(mat, cls, x, y)
    for k, v in p.items(): n.set_editor_property(k, v)
    return n

def planar(mat, tex_param, metres, x, y):
    wp = node(mat, unreal.MaterialExpressionWorldPosition, x - 500, y)
    m = node(mat, unreal.MaterialExpressionComponentMask, x - 380, y, r=True, g=True, b=False, a=False)
    L.connect_material_expressions(wp, '', m, '')
    d = node(mat, unreal.MaterialExpressionDivide, x - 260, y, const_b=metres * 100.)
    L.connect_material_expressions(m, '', d, 'A')
    s = node(mat, unreal.MaterialExpressionTextureSample, x, y)
    s.set_editor_property('texture', tex_param[1])
    if tex_param[1] is None: raise RuntimeError('ground texture missing for ' + tex_param[0])
    L.connect_material_expressions(d, '', s, 'UVs')
    return s

def main():
    report = Path(os.environ['S47_GROUND_REPORT']); out = {'status': 'RUNNING'}
    try:
        sand = unreal.load_asset('/Game/Web/Tex/T4.T4')
        out['sand'] = str(sand)
        def tex(n):
            if os.environ.get('S47_GROUND_TEXTURES') == '1':
                t = unreal.load_asset('/Game/Backdrop/Textures/T_Ground%s_D' % n)
                if t: return t
            return sand
        path = DEST + '/' + NAME
        if E.does_asset_exist(path):
            if E.get_metadata_tag(E.load_asset(path), 'S47Owner') != OWNER: raise RuntimeError('not owned: ' + path)
            E.delete_asset(path)
        mat = T.create_asset(NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
        sandA = planar(mat, ('SandTex', tex('Sand')), 2.4, -900, -400)
        sandB = planar(mat, ('SandTex', tex('Sand')), 9.0, -900, -200)
        crust = planar(mat, ('CrustTex', tex('Crust')), 3.2, -900, 0)
        grav = planar(mat, ('GravelTex', tex('Gravel')), 1.6, -900, 200)
        mask1 = planar(mat, ('SandTex', tex('Sand')), 150., -900, 400)
        mask2 = planar(mat, ('SandTex', tex('Sand')), 40., -900, 600)
        sand_mix = node(mat, unreal.MaterialExpressionMultiply, -600, -300)
        L.connect_material_expressions(sandA, 'RGB', sand_mix, 'A'); L.connect_material_expressions(sandB, 'RGB', sand_mix, 'B')
        sand_gain = node(mat, unreal.MaterialExpressionMultiply, -500, -300, const_b=1.9)
        L.connect_material_expressions(sand_mix, '', sand_gain, 'A')
        crust_tint = node(mat, unreal.MaterialExpressionVectorParameter, -700, 80, parameter_name='CrustTint', default_value=unreal.LinearColor(1.12, 1.06, .98, 1))
        crust_c = node(mat, unreal.MaterialExpressionMultiply, -600, 0)
        L.connect_material_expressions(crust, 'RGB', crust_c, 'A'); L.connect_material_expressions(crust_tint, '', crust_c, 'B')
        grav_tint = node(mat, unreal.MaterialExpressionVectorParameter, -700, 280, parameter_name='GravelTint', default_value=unreal.LinearColor(.62, .56, .50, 1))
        grav_c = node(mat, unreal.MaterialExpressionMultiply, -600, 200)
        L.connect_material_expressions(grav, 'RGB', grav_c, 'A'); L.connect_material_expressions(grav_tint, '', grav_c, 'B')
        # Masks: big pale crust pans, gravel lanes where the mid map is dark.
        def remap(src, lo, hi, x, y, pin='RGB'):
            r = node(mat, unreal.MaterialExpressionComponentMask, x, y, r=True, g=False, b=False, a=False)
            L.connect_material_expressions(src, pin, r, '')
            sub = node(mat, unreal.MaterialExpressionSubtract, x + 110, y, const_b=lo)
            L.connect_material_expressions(r, '', sub, 'A')
            mul = node(mat, unreal.MaterialExpressionMultiply, x + 220, y, const_b=1. / (hi - lo))
            L.connect_material_expressions(sub, '', mul, 'A')
            sat = node(mat, unreal.MaterialExpressionSaturate, x + 330, y)
            L.connect_material_expressions(mul, '', sat, '')
            return sat
        crust_m = remap(mask1, .46, .62, -600, 420)
        grav_inv = node(mat, unreal.MaterialExpressionOneMinus, -700, 620)
        L.connect_material_expressions(mask2, 'RGB', grav_inv, '')
        grav_m = remap(grav_inv, .58, .74, -600, 620, '')
        amt_c = node(mat, unreal.MaterialExpressionScalarParameter, -300, 500, parameter_name='CrustAmount', default_value=.7)
        amt_g = node(mat, unreal.MaterialExpressionScalarParameter, -300, 700, parameter_name='GravelAmount', default_value=.6)
        mc = node(mat, unreal.MaterialExpressionMultiply, -180, 450); L.connect_material_expressions(crust_m, '', mc, 'A'); L.connect_material_expressions(amt_c, '', mc, 'B')
        mg = node(mat, unreal.MaterialExpressionMultiply, -180, 650); L.connect_material_expressions(grav_m, '', mg, 'A'); L.connect_material_expressions(amt_g, '', mg, 'B')
        l1 = node(mat, unreal.MaterialExpressionLinearInterpolate, -100, -100)
        L.connect_material_expressions(sand_gain, '', l1, 'A'); L.connect_material_expressions(crust_c, '', l1, 'B'); L.connect_material_expressions(mc, '', l1, 'Alpha')
        l2 = node(mat, unreal.MaterialExpressionLinearInterpolate, 0, 0)
        L.connect_material_expressions(l1, '', l2, 'A'); L.connect_material_expressions(grav_c, '', l2, 'B'); L.connect_material_expressions(mg, '', l2, 'Alpha')
        vc = node(mat, unreal.MaterialExpressionVertexColor, 0, -300)
        # The export colours the sand map with the material colour (0.81, 0.72, 0.58) as well as vertex colour.
        gt = node(mat, unreal.MaterialExpressionVectorParameter, 0, -450, parameter_name='GroundTint', default_value=unreal.LinearColor(.807, .716, .578, 1))
        vt = node(mat, unreal.MaterialExpressionMultiply, 100, -350)
        L.connect_material_expressions(vc, '', vt, 'A'); L.connect_material_expressions(gt, '', vt, 'B')
        base = node(mat, unreal.MaterialExpressionMultiply, 150, -100)
        L.connect_material_expressions(vt, '', base, 'A'); L.connect_material_expressions(l2, '', base, 'B')
        L.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
        rough = node(mat, unreal.MaterialExpressionLinearInterpolate, 150, 200, const_a=.97, const_b=.86)
        L.connect_material_expressions(mc, '', rough, 'Alpha')
        L.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
        L.recompile_material(mat); E.set_metadata_tag(mat, 'S47Owner', OWNER); E.save_loaded_asset(mat)
        out['status'] = 'PASS_GROUND_MATERIAL'; out['material'] = mat.get_path_name()
    except Exception:
        out['status'] = 'FAIL'; out['error'] = traceback.format_exc()
    report.write_text(json.dumps(out, indent=2) + '\n')

if os.environ.get('S47_GROUND_APPLY') == '1': main()
