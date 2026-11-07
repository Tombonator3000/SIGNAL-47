"""/Game/FreeformTerrain/M_S47_FreeformGround: Freeform's ground material. UnrealEditor-Cmd Python.

Does not touch import_ground.py or M_S47_DesertGround, which Story uses. Reuses their textures
(T_Ground{Sand,Crust,Gravel}_{D,H,N,R}), the same parameter names, and adds:

- UV1.x sediment: light sand in arroyo beds (above .55), gravel where a bed meets its banks and on fans (.12-.46)
- UV1.y exposed stone, and slope from VertexNormalWS (smoothstep 22-37 deg), rock projected from the sides
- every boundary by height blending against the layers' own height maps, so no hard edge anywhere
- macro colour: soils in zones of a few hundred metres, brightness of +-12 % over 20-60 m

World-space normals as in M_S47_DesertGround. No light or engine state is read or changed.
"""
import unreal

DEST = '/Game/FreeformTerrain'
NAME = 'M_S47_FreeformGround'
OWNER = 'S47FreeformGround_v1'
E = unreal.EditorAssetLibrary
L = unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
STONE = '/Game/CalicheStreet/Backdrop/Materials/Textures/T_M_Caliche_Mesa_1_'
# Material colour that makes the average bare ground sRGB (165, 130, 105) as albedo, with the textures'
# measured means and the mesh's vertex colour. Freeform's own daylight settles the final value via MID.
# Without tint, with the default amounts: linear (.482, .304, .208), sRGB 184, 150, 126.
DEFAULT_TINT = unreal.LinearColor(.78, .734, .68, 1)
SPAN = {'Sand': 2.4, 'Crust': 3.2, 'Gravel': 1.6}
STONE_SPAN = 7.0


def node(mat, cls, x, y, **p):
    n = L.create_material_expression(mat, cls, x, y)
    for k, v in p.items():
        n.set_editor_property(k, v)
    return n


def shared(n):
    n.set_editor_property('sampler_source', unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
    return n


def sample(mat, texture, uv, x, y, sampler=None):
    if texture is None:
        raise RuntimeError('Missing texture')
    s = node(mat, unreal.MaterialExpressionTextureSample, x, y)
    s.set_editor_property('texture', texture)
    if sampler is not None:
        s.set_editor_property('sampler_type', sampler)
    shared(s)
    wire(uv, '', s, 'UVs')
    return s


def world_uv(mat, wp, channels, metres, x, y):
    m = node(mat, unreal.MaterialExpressionComponentMask, x - 250, y, **channels)
    wire(wp, '', m, '')
    d = node(mat, unreal.MaterialExpressionDivide, x - 120, y, const_b=metres * 100.)
    wire(m, '', d, 'A')
    return d


def custom(mat, code, inputs, output, x, y):
    """inputs: name -> expression or (expression, output pin)."""
    n = node(mat, unreal.MaterialExpressionCustom, x, y)
    n.set_editor_property('code', code)
    n.set_editor_property('output_type', output)
    pins = []
    for name in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property('input_name', name)
        pins.append(pin)
    n.set_editor_property('inputs', pins)
    for name, value in inputs.items():
        expression, out = value if isinstance(value, tuple) else (value, '')
        if not L.connect_material_expressions(expression, out, n, name):
            raise RuntimeError('Could not connect custom input ' + name)
    return n


def wire(a, out, b, pin):
    # A pin that does not exist fails quietly, and an open Multiply is 0: the whole ground black. Never again quietly.
    if not L.connect_material_expressions(a, out, b, pin):
        raise RuntimeError('No such pin: %s output %r to %s input %r' % (a.get_name(), out, b.get_name(), pin))


def lerp(mat, a, b, alpha, x, y, pin_a='', pin_b='', pin_alpha=''):
    r = node(mat, unreal.MaterialExpressionLinearInterpolate, x, y)
    wire(a, pin_a, r, 'A')
    wire(b, pin_b, r, 'B')
    wire(alpha, pin_alpha, r, 'Alpha')
    return r


def mul(mat, a, b, x, y, pin_a='', pin_b=''):
    r = node(mat, unreal.MaterialExpressionMultiply, x, y)
    wire(a, pin_a, r, 'A')
    wire(b, pin_b, r, 'B')
    return r


def parallax_uv(mat, height, metres, y):
    """As in M_S47_DesertGround: world XY, fixed 8 steps, gone by 16 m, grazing clamp."""
    wp = node(mat, unreal.MaterialExpressionWorldPosition, -3600, y)
    cam = node(mat, unreal.MaterialExpressionCameraPositionWS, -3600, y + 80)
    span = node(mat, unreal.MaterialExpressionConstant, -3600, y + 160, r=metres * 100.)
    depth = node(mat, unreal.MaterialExpressionScalarParameter, -3600, y + 240, parameter_name='MicroReliefCm', default_value=1.2)
    tex = node(mat, unreal.MaterialExpressionTextureObject, -3600, y + 320, texture=height)
    code = """
float2 uv = World.xy / Span;
float3 delta = Camera - World;
float distanceCm = length(delta);
float fade = 1.0 - smoothstep(600.0,1600.0,distanceCm);
float3 v = normalize(delta + float3(0,0,0.00001));
float2 dx = ddx(uv), dy = ddy(uv);
if (fade <= 0.0 || Depth <= 0.0) return uv;
float2 ray = v.xy / max(abs(v.z),0.3) * clamp(Depth,0.0,2.0) / Span * fade;
float2 p = uv + ray * 0.5;
float layer = 1.0;
float previous = layer - Texture2DSampleGrad(Height,HeightSampler,p,dx,dy).r;
[unroll] for(int i=0;i<8;i++)
{
    float2 nextP = p - ray * 0.125;
    layer -= 0.125;
    float difference = layer - Texture2DSampleGrad(Height,HeightSampler,nextP,dx,dy).r;
    if(difference <= 0.0)
        return lerp(p,nextP,saturate(previous / max(previous-difference,0.00001)));
    p = nextP;
    previous = difference;
}
return p;
"""
    return custom(mat, code, dict(World=wp, Camera=cam, Span=span, Depth=depth, Height=tex), unreal.CustomMaterialOutputType.CMOT_FLOAT2, -3100, y)


def masks_code(heights):
    # Height blend: m = 0 gives exactly 0 and m = 1 exactly 1 whatever the heights are, and between
    # them the top layer takes the high grains first.
    h = ('Hs', 'Hc', 'Hg', 'Hr') if heights else ('0.5', '0.5', '0.5', '0.5')
    return """
float sedSand = smoothstep(0.55, 0.85, Sed.x);
float sedGravel = smoothstep(0.12, 0.34, Sed.x) * (1.0 - smoothstep(0.46, 0.62, Sed.x));
float crust0 = saturate((T150 - 0.46) / 0.16) * Crust * (1.0 - sedSand) * (1.0 - 0.7 * sedGravel);
float gravel0 = max(saturate(((1.0 - T40) - 0.58) / 0.16) * Gravel, 0.95 * sedGravel) * (1.0 - sedSand);
float deg = acos(saturate(normalize(N).z)) * 57.29578;
// the baked stone only where the ground has a slope to show it on: never on flat land
float rock0 = max(smoothstep(22.0, 37.0, deg), Sed.y * smoothstep(8.0, 16.0, deg));
float k = 0.35;
float hs = %s, hc = %s, hg = %s, hr = %s;
float mc = smoothstep(0.0, 1.0, crust0 * (1.0 + 2.0 * k) - k + (hc - hs) * k);
float under = lerp(hs, hc, mc);
float mg = smoothstep(0.0, 1.0, gravel0 * (1.0 + 2.0 * k) - k + (hg - under) * k);
under = lerp(under, hg, mg);
float mr = smoothstep(0.0, 1.0, rock0 * (1.0 + 2.0 * k) - k + (hr - under) * k);
return float3(mc, mg, mr);
""" % h


def build(mat, tex, stone, sand_macro, report):
    wp = node(mat, unreal.MaterialExpressionWorldPosition, -3600, -1800)
    uv1 = node(mat, unreal.MaterialExpressionTextureCoordinate, -3600, -1700, coordinate_index=1)
    n_ws = node(mat, unreal.MaterialExpressionVertexNormalWS, -3600, -1600)
    color = dict(r=True, g=True, b=False, a=False)
    gray = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE
    normal = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL

    relief = {layer: parallax_uv(mat, tex[layer, 'H'], SPAN[layer], -1400 + i * 400) for i, layer in enumerate(('Sand', 'Crust', 'Gravel'))}
    # macro masks: the old material's own pattern sources, from T4
    uv150 = world_uv(mat, wp, color, 150., -2500, -1800)
    uv40 = world_uv(mat, wp, color, 40., -2500, -1700)
    uv47 = world_uv(mat, wp, color, 47., -2500, -1600)
    uv430 = world_uv(mat, wp, color, 430., -2500, -1500)
    t150 = sample(mat, sand_macro, uv150, -2200, -1800)
    t40 = sample(mat, sand_macro, uv40, -2200, -1700)
    t47 = sample(mat, sand_macro, uv47, -2200, -1600)
    t430 = sample(mat, sand_macro, uv430, -2200, -1500)
    crust_amount = node(mat, unreal.MaterialExpressionScalarParameter, -2200, -1400, parameter_name='CrustAmount', default_value=.7)
    gravel_amount = node(mat, unreal.MaterialExpressionScalarParameter, -2200, -1300, parameter_name='GravelAmount', default_value=.6)

    # albedo of the layers
    d = {layer: sample(mat, tex[layer, 'D'], relief[layer], -2400, -1400 + i * 400) for i, layer in enumerate(('Sand', 'Crust', 'Gravel'))}
    h_blend = {layer: sample(mat, tex[layer, 'H'], relief[layer], -2400, -1200 + i * 400, gray) for i, layer in enumerate(('Sand', 'Crust', 'Gravel'))}
    r = {layer: sample(mat, tex[layer, 'R'], relief[layer], -2400, -1100 + i * 400, gray) for i, layer in enumerate(('Sand', 'Crust', 'Gravel'))}
    n = {layer: sample(mat, tex[layer, 'N'], relief[layer], -2400, -1000 + i * 400, normal) for i, layer in enumerate(('Sand', 'Crust', 'Gravel'))}
    uv9 = world_uv(mat, wp, color, 9., -2500, 100)
    sand_b = sample(mat, tex['Sand', 'D'], uv9, -2400, 100)

    # stone from the sides: three projections (UVs per the whiteout blend below: zy, xz, xy)
    uvx = world_uv(mat, wp, dict(r=False, g=True, b=True, a=False), STONE_SPAN, -2500, 300)
    uvy = world_uv(mat, wp, dict(r=True, g=False, b=True, a=False), STONE_SPAN, -2500, 500)
    uvz = world_uv(mat, wp, color, STONE_SPAN, -2500, 700)
    sx = sample(mat, stone['D'], uvx, -2400, 300)
    sy = sample(mat, stone['D'], uvy, -2400, 500)
    sz = sample(mat, stone['D'], uvz, -2400, 700)
    nx = sample(mat, stone['N'], uvx, -2200, 300, normal)
    ny = sample(mat, stone['N'], uvy, -2200, 500, normal)
    nz = sample(mat, stone['N'], uvz, -2200, 700, normal)
    weights = custom(mat, 'float3 w = pow(abs(normalize(N)), 5.0); return w / max(w.x + w.y + w.z, 0.0001);',
                     dict(N=n_ws), unreal.CustomMaterialOutputType.CMOT_FLOAT3, -2000, 300)
    stone_d = custom(mat, 'return X * W.x + Y * W.y + Z * W.z;', dict(X=(sx, 'RGB'), Y=(sy, 'RGB'), Z=(sz, 'RGB'), W=weights),
                     unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1800, 300)
    stone_h = custom(mat, 'return saturate(dot(C, float3(0.3, 0.6, 0.1)) * 2.4);', dict(C=stone_d), unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1600, 300)
    stone_n = custom(mat, """
float3 n = normalize(N);
float3 w = W;
float3 tx = float3(X.xy + n.zy, abs(X.z) * n.x);
float3 ty = float3(Y.xy + n.xz, abs(Y.z) * n.y);
float3 tz = float3(Z.xy + n.xy, abs(Z.z) * n.z);
return normalize(tx.zyx * w.x + ty.xzy * w.y + tz.xyz * w.z);
""", dict(X=(nx, 'RGB'), Y=(ny, 'RGB'), Z=(nz, 'RGB'), N=n_ws, W=weights), unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1600, 500)

    # what lies where
    pins = dict(Sed=uv1, T150=(t150, 'R'), T40=(t40, 'R'), Crust=crust_amount, Gravel=gravel_amount, N=n_ws,
                Hs=(h_blend['Sand'], 'R'), Hc=(h_blend['Crust'], 'R'), Hg=(h_blend['Gravel'], 'R'), Hr=stone_h)
    masks = custom(mat, masks_code(True), pins, unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1400, -1500)
    def pick(name, y):
        m = node(mat, unreal.MaterialExpressionComponentMask, -1200, y, r=name == 'r', g=name == 'g', b=name == 'b', a=False)
        wire(masks, '', m, '')
        return m
    mc, mg, mr = pick('r', -1700), pick('g', -1600), pick('b', -1500)

    # colour
    crust_tint = node(mat, unreal.MaterialExpressionVectorParameter, -1500, -800, parameter_name='CrustTint', default_value=unreal.LinearColor(.94, .94, .94, 1))
    gravel_tint = node(mat, unreal.MaterialExpressionVectorParameter, -1500, -700, parameter_name='GravelTint', default_value=unreal.LinearColor(.94, .94, .94, 1))
    sand = lerp(mat, d['Sand'], sand_b, node(mat, unreal.MaterialExpressionConstant, -1500, -950, r=.2), -1300, -1000, 'RGB', 'RGB')
    crust = mul(mat, d['Crust'], crust_tint, -1300, -850, 'RGB')
    gravel = mul(mat, d['Gravel'], gravel_tint, -1300, -700, 'RGB')
    l1 = lerp(mat, sand, crust, mc, -1100, -900)
    l2 = lerp(mat, l1, gravel, mg, -950, -850)
    # stone in the land's own colour: the rock's pattern, normalised by its mean, over the soil beside it
    local_stone = custom(mat, 'return Soil * clamp(Rock / float3(0.183, 0.132, 0.109), 0.25, 1.7) * 0.72;', dict(Soil=l2, Rock=stone_d), unreal.CustomMaterialOutputType.CMOT_FLOAT3, -950, -760)
    l3 = lerp(mat, l2, local_stone, mr, -800, -800)
    macro = custom(mat, """
// T4's red in linear runs about 0.22-0.55 (p5-p95)
float big = saturate((Z - 0.22) / 0.33);
float small = saturate((M - 0.22) / 0.33);
float3 caliche = float3(1.10, 1.09, 1.07);
float3 neutral = float3(1.00, 1.00, 1.00);
float3 greyish = float3(0.93, 0.95, 0.98);
float3 reddish = float3(1.03, 0.92, 0.83);
float3 c = big < 0.33 ? lerp(caliche, neutral, big / 0.33) : big < 0.66 ? lerp(neutral, greyish, (big - 0.33) / 0.33) : lerp(greyish, reddish, (big - 0.66) / 0.34);
return c * (0.88 + 0.24 * small);
""", dict(Z=(t430, 'R'), M=(t47, 'R')), unreal.CustomMaterialOutputType.CMOT_FLOAT3, -950, -600)
    vc = node(mat, unreal.MaterialExpressionVertexColor, -800, -600)
    tint = node(mat, unreal.MaterialExpressionVectorParameter, -800, -500, parameter_name='GroundTint', default_value=DEFAULT_TINT)
    # (a VertexColor's first output has no name: it was asked for 'RGB' here, which left this Multiply's A open at 0)
    vt = mul(mat, vc, tint, -600, -550)
    tinted = mul(mat, l3, vt, -450, -700)
    base = mul(mat, tinted, macro, -300, -700)

    # roughness
    rl1 = lerp(mat, r['Sand'], r['Crust'], mc, -1100, -300, 'R', 'R')
    rl2 = lerp(mat, rl1, r['Gravel'], mg, -950, -300, '', 'R')
    rl3 = lerp(mat, rl2, node(mat, unreal.MaterialExpressionConstant, -1100, -200, r=.93), mr, -800, -300)

    # normal, as M_S47_DesertGround: world XY, reconstructed on the geometric slope; then stone over it
    nl1 = lerp(mat, n['Sand'], n['Crust'], mc, -1100, 0, 'RGB', 'RGB')
    nl2 = lerp(mat, nl1, n['Gravel'], mg, -950, 0, '', 'RGB')
    ground_n = custom(mat, 'float3 n = normalize(MacroN); float3 t = normalize(float3(1,0,0) - n * n.x + float3(0,0.00001,0)); float3 b = normalize(cross(n,t)); return normalize(t * Detail.x + b * Detail.y + n * Detail.z);',
                      dict(Detail=nl2, MacroN=n_ws), unreal.CustomMaterialOutputType.CMOT_FLOAT3, -800, 0)
    world_n = custom(mat, 'return normalize(lerp(A, B, Alpha));', dict(A=ground_n, B=stone_n, Alpha=mr), unreal.CustomMaterialOutputType.CMOT_FLOAT3, -600, 0)

    # displacement: no parallax or camera on this path, as M_S47_DesertGround
    hds = {layer: sample(mat, tex[layer, 'H'], world_uv(mat, wp, color, SPAN[layer], -1700, 1100 + i * 180), -1500, 1100 + i * 180, gray) for i, layer in enumerate(('Sand', 'Crust', 'Gravel'))}
    disp_pins = dict(Sed=uv1, T150=(t150, 'R'), T40=(t40, 'R'), Crust=crust_amount, Gravel=gravel_amount, N=n_ws)
    disp_masks = custom(mat, masks_code(False), disp_pins, unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1300, 1100)
    disp = custom(mat, """
float h = lerp(lerp(Hs, Hc, M.x), Hg, M.y);
return lerp(h, 0.5, M.z);
""", dict(Hs=(hds['Sand'], 'R'), Hc=(hds['Crust'], 'R'), Hg=(hds['Gravel'], 'R'), M=disp_masks), unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1000, 1100)

    attrs = node(mat, unreal.MaterialExpressionMakeMaterialAttributes, -100, -100)
    for out, pin in ((base, 'BaseColor'), (rl3, 'Roughness'), (world_n, 'Normal'), (disp, 'Displacement')):
        if not L.connect_material_expressions(out, '', attrs, pin):
            raise RuntimeError('Could not connect ground attribute ' + pin)
    mat.set_editor_property('tangent_space_normal', False)
    mat.set_editor_property('use_material_attributes', True)
    L.connect_material_property(attrs, '', unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    mat.set_editor_property('enable_tessellation', True)
    mat.set_editor_property('used_with_nanite', True)
    scaling = mat.get_editor_property('displacement_scaling')
    scaling.set_editor_property('magnitude', 7.0)
    scaling.set_editor_property('center', .5)
    mat.set_editor_property('displacement_scaling', scaling)


def main(report):
    path = DEST + '/' + NAME
    out = report.setdefault('material', {'status': 'RUNNING'})
    tex = {}
    for layer in ('Sand', 'Crust', 'Gravel'):
        for role in ('D', 'H', 'N', 'R'):
            t = E.load_asset('/Game/Backdrop/Textures/T_Ground%s_%s' % (layer, role))
            if t is None:
                raise RuntimeError('Missing ground texture ' + layer + role)
            tex[layer, role] = t
    stone = {'D': E.load_asset(STONE + 'base_color'), 'N': E.load_asset(STONE + 'normal')}
    if stone['D'] is None or stone['N'] is None:
        raise RuntimeError('Missing stone textures: ' + STONE)
    out['stone'] = {k: v.get_path_name() for k, v in stone.items()}
    out['stone_normal_compression'] = str(stone['N'].get_editor_property('compression_settings'))
    macro = unreal.load_asset('/Game/Web/Tex/T4.T4')
    if macro is None:
        raise RuntimeError('T4 is required for the macro masks')
    mat = E.load_asset(path) if E.does_asset_exist(path) else None
    if mat is not None and E.get_metadata_tag(mat, 'S47Owner') != OWNER:
        raise RuntimeError('not owned: ' + path)
    if mat is None:
        mat = T.create_asset(NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
    else:
        L.delete_all_material_expressions(mat)
    build(mat, tex, stone, macro, out)
    errors = [str(e) for e in L.recompile_material(mat)]
    out['compile_errors'] = errors
    if errors:
        raise RuntimeError('Freeform ground material failed: ' + '; '.join(errors))
    E.set_metadata_tag(mat, 'S47Owner', OWNER)
    if not E.save_loaded_asset(mat):
        raise RuntimeError('Freeform ground material save failed')
    out['path'] = mat.get_path_name()
    out['status'] = 'PASS'
    return mat

