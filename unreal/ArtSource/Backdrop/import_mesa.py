"""Distant New Mexico mesas: world-aligned photo rock material for the exported mesa meshes.
Run inside a leased editor (run_mesa_import.py). Builds /Game/Licensed/Backdrop/M_S47_MesaRock, which
uses the locally licensed 8K canyon sandstone (kept out of Git with the rest of /Game/Licensed).
Base colour = mesh vertex colour (the authored strata palette) x macro x detail triplanar rock, so the
painted bands survive and tiling is broken at two scales. Runtime falls back to the original material
when this asset is absent."""
import json, os, traceback
from pathlib import Path
import unreal
DEST = '/Game/Licensed/Backdrop'
NAME = 'M_S47_MesaRock'
ROCK = '/Game/Licensed/RouteNature/Textures/T_Huge_Canyon_Sandstone_Cliff_vcepbapga_8K_D'
ROCK_N = '/Game/Licensed/RouteNature/Textures/T_Huge_Canyon_Sandstone_Cliff_vcepbapga_8K_N'
WAT = '/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture'
E = unreal.EditorAssetLibrary; L = unreal.MaterialEditingLibrary; T = unreal.AssetToolsHelpers.get_asset_tools()
OWNER = 'S47MesaRock_v1'  # same owner across revisions

def node(mat, cls, x, y, **props):
    n = L.create_material_expression(mat, cls, x, y)
    for k, v in props.items(): n.set_editor_property(k, v)
    return n

def triplanar(mat, tex, size, x, y, normal=False):
    f = node(mat, unreal.MaterialExpressionMaterialFunctionCall, x, y)
    f.set_material_function(unreal.load_asset(WAT))
    t = node(mat, unreal.MaterialExpressionTextureObject, x - 300, y, texture=tex)
    s = node(mat, unreal.MaterialExpressionConstant3Vector, x - 300, y + 120, constant=unreal.LinearColor(size, size, size, 1))
    L.connect_material_expressions(t, '', f, 'TextureObject')
    L.connect_material_expressions(s, '', f, 'TextureSize')
    return f

def main():
    report = Path(os.environ['S47_MESA_REPORT']); out = {'status': 'RUNNING'}
    try:
        rock = unreal.load_asset(ROCK); rock_n = unreal.load_asset(ROCK_N)
        if not rock: raise RuntimeError('licensed sandstone texture missing: ' + ROCK)
        path = DEST + '/' + NAME
        if E.does_asset_exist(path):
            if E.get_metadata_tag(E.load_asset(path), 'S47Owner') != OWNER: raise RuntimeError('not owned: ' + path)
            E.delete_asset(path)
        mat = T.create_asset(NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
        # Per-slot tint (MID 'Tint'), optionally times mesh vertex colour ('UseVertexColor' 0..1).
        vc0 = node(mat, unreal.MaterialExpressionVertexColor, -1200, -300)
        one3 = node(mat, unreal.MaterialExpressionConstant3Vector, -1200, -150, constant=unreal.LinearColor(1, 1, 1, 1))
        usev = node(mat, unreal.MaterialExpressionScalarParameter, -1200, -50, parameter_name='UseVertexColor', default_value=0.0)
        vl = node(mat, unreal.MaterialExpressionLinearInterpolate, -1000, -200)
        L.connect_material_expressions(one3, '', vl, 'A'); L.connect_material_expressions(vc0, '', vl, 'B'); L.connect_material_expressions(usev, '', vl, 'Alpha')
        tint = node(mat, unreal.MaterialExpressionVectorParameter, -1000, -350, parameter_name='Tint', default_value=unreal.LinearColor(1, 1, 1, 1))
        vc = node(mat, unreal.MaterialExpressionMultiply, -900, -250)
        L.connect_material_expressions(tint, '', vc, 'A'); L.connect_material_expressions(vl, '', vc, 'B')
        macro = triplanar(mat, rock, 24000., -900, 0)
        detail = triplanar(mat, rock, 4000., -900, 300)
        # Detail carries the photo colour; macro only modulates brightness to break the repeat.
        md = node(mat, unreal.MaterialExpressionComponentMask, -650, 0, r=True, g=False, b=False, a=False)
        L.connect_material_expressions(macro, 'XYZ Texture', md, '')
        mm = node(mat, unreal.MaterialExpressionMultiply, -560, 0)
        two = node(mat, unreal.MaterialExpressionConstant, -650, 80, r=2.0)
        L.connect_material_expressions(md, '', mm, 'A'); L.connect_material_expressions(two, '', mm, 'B')
        mul = node(mat, unreal.MaterialExpressionMultiply, -500, 100)
        L.connect_material_expressions(detail, 'XYZ Texture', mul, 'A'); L.connect_material_expressions(mm, '', mul, 'B')
        gain = node(mat, unreal.MaterialExpressionScalarParameter, -500, 300, parameter_name='Gain', default_value=2.6)
        gmul = node(mat, unreal.MaterialExpressionMultiply, -350, 150)
        L.connect_material_expressions(mul, '', gmul, 'A'); L.connect_material_expressions(gain, '', gmul, 'B')
        amount = node(mat, unreal.MaterialExpressionScalarParameter, -500, 420, parameter_name='RockAmount', default_value=1.0)
        one = node(mat, unreal.MaterialExpressionConstant, -350, 0, r=1.0)
        lerp = node(mat, unreal.MaterialExpressionLinearInterpolate, -200, 100)
        L.connect_material_expressions(one, '', lerp, 'A'); L.connect_material_expressions(gmul, '', lerp, 'B'); L.connect_material_expressions(amount, '', lerp, 'Alpha')
        base = node(mat, unreal.MaterialExpressionMultiply, -50, 0)
        L.connect_material_expressions(vc, '', base, 'A'); L.connect_material_expressions(lerp, '', base, 'B')
        L.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
        if rock_n:
            nm = triplanar(mat, rock_n, 9000., -900, 650)
            L.connect_material_property(nm, 'XYZ Texture', unreal.MaterialProperty.MP_NORMAL)
        r = node(mat, unreal.MaterialExpressionConstant, -200, 500, r=.92)
        L.connect_material_property(r, '', unreal.MaterialProperty.MP_ROUGHNESS)
        L.recompile_material(mat); E.set_metadata_tag(mat, 'S47Owner', OWNER); E.save_loaded_asset(mat)
        out['status'] = 'PASS_MESA_MATERIAL'; out['material'] = mat.get_path_name()
    except Exception:
        out['status'] = 'FAIL'; out['error'] = traceback.format_exc()
    report.write_text(json.dumps(out, indent=2) + '\n')

if os.environ.get('S47_MESA_APPLY') == '1': main()
