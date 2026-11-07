"""/Game/FreeformTerrain/M_S47_FreeformGround: Freeform's ground material. UnrealEditor-Cmd Python.

Does not touch import_ground.py or M_S47_DesertGround, which Story uses. Reuses their textures
(T_Ground{Sand,Crust,Gravel}_{D,H,N,R}), the same parameter names, and adds:

- UV1.x sediment: light sand in arroyo beds (above .55), gravel where a bed meets its banks and on fans (.12-.46)
- UV1.y exposed stone, and slope from VertexNormalWS (smoothstep 22-37 deg), rock projected from the sides
- every boundary by height blending against the layers' own height maps, so no hard edge anywhere
- one soil that changes on three scales: T_FF_Noise over 40-200 m and 3-15 m, T_FF_Patch_D (a real 20 m of dry ground) between them
- no count of tiles: each layer's offset jumps by a smooth index (T_FF_Noise.a), the colour crossfaded over the jump and the rest picked
- stones lying in the ground: T_FF_Stones_*, whole stones by rank, more on slopes, on fans and in gravel washes, standing up by displacement

All the new colour is a ratio of mean 1 (ground_textures: tools/make_ground_textures.py), so the middle tone stays where GroundTint sets it.
World-space normals as in M_S47_DesertGround. No light or engine state is read or changed.
"""
import os
from pathlib import Path

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
# (name, compression, sampler). Made by tools/make_ground_textures.py under S47_FF_ROOT/textures/out; KILDER.md beside them.
GROUND_TEXTURES = (('T_FF_Stones_D', 'TC_BC7'), ('T_FF_Stones_N', 'TC_BC7'), ('T_FF_Patch_D', 'TC_DEFAULT'), ('T_FF_Noise', 'TC_VECTOR_DISPLACEMENTMAP'))
RATIO = 2.5                  # what the ratio textures were divided by
PEBBLE_SPAN, PATCH_SPANS, NOISE_SPANS = (4.0, 1.63), (23.0, 71.0), (1100.0, 67.0)


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


def sample_grad(mat, texture, uv, dx, dy, x, y, sampler=None):
    """A sample whose UV jumps: the mip comes from the smooth UV's own derivatives, so no line of blur along a jump."""
    s = sample(mat, texture, uv, x, y, sampler)
    s.set_editor_property('mip_value_mode', unreal.TextureMipValueMode.TMVM_DERIVATIVE)
    wire(dx, '', s, 'DDX(UVs)')
    wire(dy, '', s, 'DDY(UVs)')
    return s


def derivatives(mat, uv, x, y):
    dx = node(mat, unreal.MaterialExpressionDDX, x, y)
    wire(uv, '', dx, 'Value')
    dy = node(mat, unreal.MaterialExpressionDDY, x, y + 60)
    wire(uv, '', dy, 'Value')
    return dx, dy


def channels(mat, source, x, y, **pick):
    m = node(mat, unreal.MaterialExpressionComponentMask, x, y, **dict(dict(r=False, g=False, b=False, a=False), **pick))
    wire(source, '', m, '')
    return m


def parallax_offset(mat, wp, height, pick, y):
    """World XY, fixed 8 steps, gone by 16 m, grazing clamp, as M_S47_DesertGround; centimetres to move all layers by. One march
    against the stones' height (its alpha), not one for each layer."""
    cam = node(mat, unreal.MaterialExpressionCameraPositionWS, -3600, y)
    depth = node(mat, unreal.MaterialExpressionScalarParameter, -3600, y + 80, parameter_name='MicroReliefCm', default_value=1.2)
    tex = node(mat, unreal.MaterialExpressionTextureObject, -3600, y + 160, texture=height)
    span = node(mat, unreal.MaterialExpressionConstant, -3600, y + 240, r=PEBBLE_SPAN[0] * 100.)
    code = """
float2 uv = World.xy / Span + Pick;
float3 delta = Camera - World;
float distanceCm = length(delta);
float fade = 1.0 - smoothstep(600.0,1600.0,distanceCm);
float3 v = normalize(delta + float3(0,0,0.00001));
float2 dx = ddx(uv), dy = ddy(uv);
if (fade <= 0.0 || Depth <= 0.0) return float2(0,0);
float2 ray = v.xy / max(abs(v.z),0.3) * clamp(Depth,0.0,2.0) / Span * fade;
float2 p = uv + ray * 0.5;
float layer = 1.0;
float previous = layer - Texture2DSampleGrad(Height,HeightSampler,p,dx,dy).a;
[unroll] for(int i=0;i<8;i++)
{
    float2 nextP = p - ray * 0.125;
    layer -= 0.125;
    float difference = layer - Texture2DSampleGrad(Height,HeightSampler,nextP,dx,dy).a;
    if(difference <= 0.0)
        return (lerp(p,nextP,saturate(previous / max(previous-difference,0.00001))) - uv) * Span;
    p = nextP;
    previous = difference;
}
return (p - uv) * Span;
"""
    return custom(mat, code, dict(World=wp, Camera=cam, Span=span, Depth=depth, Height=tex, Pick=pick), unreal.CustomMaterialOutputType.CMOT_FLOAT2, -3100, y)


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


# How many of the stones lie here, 0-1: a few everywhere, more in the gravel washes, on fans, where stone shows and on slopes, few in sand.
PRESENCE = """
float sedSand = smoothstep(0.55, 0.85, Sed.x);
float sedGravel = smoothstep(0.12, 0.34, Sed.x) * (1.0 - smoothstep(0.46, 0.62, Sed.x));
float deg = acos(saturate(normalize(N).z)) * 57.29578;
float wash = smoothstep(0.50, 0.74, N1.b * 0.8 + N2.g * 0.2);
float p = 0.07 + 0.60 * wash + 0.60 * smoothstep(5.0, 22.0, deg) + 0.65 * sedGravel + 0.55 * Sed.y;
p *= (0.5 + 0.59 * Gravel) * (0.7 + 0.6 * N2.r) * (1.0 - 0.85 * sedSand);
return min(p, 0.98);
"""


def build(mat, tex, stone, ff, report):
    wp = node(mat, unreal.MaterialExpressionWorldPosition, -3600, -1800)
    uv1 = node(mat, unreal.MaterialExpressionTextureCoordinate, -3600, -1700, coordinate_index=1)
    n_ws = node(mat, unreal.MaterialExpressionVertexNormalWS, -3600, -1600)
    color = dict(r=True, g=True, b=False, a=False)
    gray = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE
    normal = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
    linear = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
    f2, f3, f4, f1 = (getattr(unreal.CustomMaterialOutputType, 'CMOT_FLOAT%d' % i) for i in (2, 3, 4, 1))

    # the fields. The second is turned 37 degrees and pulled about by the first, so neither runs in lines
    n1 = sample(mat, ff['T_FF_Noise'], world_uv(mat, wp, color, NOISE_SPANS[0], -3300, -2300), -3100, -2300, linear)
    span2 = node(mat, unreal.MaterialExpressionConstant, -3300, -2150, r=NOISE_SPANS[1] * 100.)
    uv_n2 = custom(mat, 'return float2(World.x * 0.8 - World.y * 0.6, World.x * 0.6 + World.y * 0.8) / Span + (N1.rg - 0.5) * 0.16;',
                   dict(World=wp, Span=span2, N1=n1), f2, -3100, -2150)
    n2 = sample(mat, ff['T_FF_Noise'], uv_n2, -2900, -2150, linear)
    # the stochastic tiling: eight sets of offsets, the index smooth, so each set meets only the next
    offsets = custom(mat, 'float i = floor(Index * 8.0); return float4(sin(float2(3.0, 7.0) * i), sin(float2(3.0, 7.0) * (i + 1.0)));',
                     dict(Index=(n2, 'A')), f4, -2700, -2300)
    fade = custom(mat, 'float l = Index * 8.0; return smoothstep(0.33, 0.67, l - floor(l));', dict(Index=(n2, 'A')), f1, -2700, -2200)
    pick = custom(mat, 'return Fade < 0.5 ? O.xy : O.zw;', dict(O=offsets, Fade=fade), f2, -2700, -2100)

    relief = parallax_offset(mat, wp, ff['T_FF_Stones_N'], pick, -2000)

    def layer_uvs(metres, y):
        """(smooth UV, ddx, ddy, UV of the first set, of the second, of the picked one)."""
        base = world_uv(mat, wp, color, metres, -2800, y)
        dx, dy = derivatives(mat, base, -2650, y)
        span = node(mat, unreal.MaterialExpressionConstant, -2800, y + 60, r=metres * 100.)
        both = custom(mat, 'float2 p = (World.xy + Relief) / Span; return float4(p + O.xy, p + O.zw);', dict(World=wp, Relief=relief, Span=span, O=offsets), f4, -2650, y + 120)
        picked = custom(mat, 'return (World.xy + Relief) / Span + Pick;', dict(World=wp, Relief=relief, Span=span, Pick=pick), f2, -2650, y + 200)
        return base, dx, dy, channels(mat, both, -2500, y, r=True, g=True), channels(mat, both, -2500, y + 60, b=True, a=True), picked

    # albedo of the layers; the height, roughness and normal of the same layers from the picked set alone
    d, h_blend, r, n = {}, {}, {}, {}
    for i, layer in enumerate(('Sand', 'Crust', 'Gravel')):
        y = -1400 + i * 400
        _, dx, dy, ua, ub, up = layer_uvs(SPAN[layer], y)
        da = sample_grad(mat, tex[layer, 'D'], ua, dx, dy, -2400, y)
        db = sample_grad(mat, tex[layer, 'D'], ub, dx, dy, -2400, y + 60)
        d[layer] = lerp(mat, da, db, fade, -2200, y, 'RGB', 'RGB')
        h_blend[layer] = sample_grad(mat, tex[layer, 'H'], up, dx, dy, -2400, y + 120, gray)
        r[layer] = sample_grad(mat, tex[layer, 'R'], up, dx, dy, -2400, y + 180, gray)
        n[layer] = sample_grad(mat, tex[layer, 'N'], up, dx, dy, -2400, y + 240, normal)

    crust_amount = node(mat, unreal.MaterialExpressionScalarParameter, -2200, -2100, parameter_name='CrustAmount', default_value=.7)
    gravel_amount = node(mat, unreal.MaterialExpressionScalarParameter, -2200, -2000, parameter_name='GravelAmount', default_value=.6)
    # in the terms of masks_code: crust above .46, gravel below .42, mean of a field .5
    t150 = custom(mat, 'return N1.g * 0.7 + N2.r * 0.3 - 0.06;', dict(N1=n1, N2=n2), f1, -2000, -2100)
    t40 = custom(mat, 'return 1.0 - (N1.b * 0.75 + N2.g * 0.25);', dict(N1=n1, N2=n2), f1, -2000, -2000)

    # the stones: a big scale and a small one turned from it, each stone whole
    presence = custom(mat, PRESENCE, dict(Sed=uv1, N=n_ws, N1=n1, N2=n2, Gravel=gravel_amount), f1, -2000, -1900)
    _, bdx, bdy, _, _, ubig = layer_uvs(PEBBLE_SPAN[0], 1500)
    big_d = sample_grad(mat, ff['T_FF_Stones_D'], ubig, bdx, bdy, -2400, 1500, linear)
    big_n = sample_grad(mat, ff['T_FF_Stones_N'], ubig, bdx, bdy, -2400, 1560, linear)
    span_small = node(mat, unreal.MaterialExpressionConstant, -2800, 1700, r=PEBBLE_SPAN[1] * 100.)
    base_small = custom(mat, 'return float2(World.x * 0.8 + World.y * 0.6, World.y * 0.8 - World.x * 0.6) / Span;', dict(World=wp, Span=span_small), f2, -2650, 1700)
    sdx, sdy = derivatives(mat, base_small, -2500, 1700)
    usmall = custom(mat, 'return Base + Pick.yx;', dict(Base=base_small, Pick=pick), f2, -2500, 1800)
    small_d = sample_grad(mat, ff['T_FF_Stones_D'], usmall, sdx, sdy, -2400, 1800, linear)
    stones = custom(mat, """
float mb = smoothstep(1.0 - P, 1.06 - P, Big.a);
float ms = smoothstep(1.0 - P * 0.8, 1.06 - P * 0.8, Small.a);
float3 c = lerp(float3(1.0, 1.0, 1.0), Big.rgb * Ratio, mb);
c = lerp(c, Small.rgb * Ratio, ms);
return float4(c, mb);
""", dict(Big=(big_d, 'RGBA'), Small=(small_d, 'RGBA'), P=presence, Ratio=node(mat, unreal.MaterialExpressionConstant, -2000, 1400, r=RATIO)), f4, -1800, 1500)
    stones_rgb = channels(mat, stones, -1600, 1500, r=True, g=True, b=True)
    stones_big = channels(mat, stones, -1600, 1560, a=True)

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
    pins = dict(Sed=uv1, T150=t150, T40=t40, Crust=crust_amount, Gravel=gravel_amount, N=n_ws,
                Hs=(h_blend['Sand'], 'R'), Hc=(h_blend['Crust'], 'R'), Hg=(h_blend['Gravel'], 'R'), Hr=stone_h)
    masks = custom(mat, masks_code(True), pins, unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1400, -1500)
    mc, mg, mr = channels(mat, masks, -1200, -1700, r=True), channels(mat, masks, -1200, -1600, g=True), channels(mat, masks, -1200, -1500, b=True)

    # colour
    crust_tint = node(mat, unreal.MaterialExpressionVectorParameter, -1500, -800, parameter_name='CrustTint', default_value=unreal.LinearColor(.94, .94, .94, 1))
    gravel_tint = node(mat, unreal.MaterialExpressionVectorParameter, -1500, -700, parameter_name='GravelTint', default_value=unreal.LinearColor(.94, .94, .94, 1))
    crust = mul(mat, d['Crust'], crust_tint, -1300, -850)
    gravel = mul(mat, d['Gravel'], gravel_tint, -1300, -700)
    l1 = lerp(mat, d['Sand'], crust, mc, -1100, -900)
    l2 = lerp(mat, l1, gravel, mg, -950, -850)
    # stone in the land's own colour: the rock's pattern, normalised by its mean, over the soil beside it
    local_stone = custom(mat, 'return Soil * clamp(Rock / float3(0.183, 0.132, 0.109), 0.25, 1.7) * 0.72;', dict(Soil=l2, Rock=stone_d), unreal.CustomMaterialOutputType.CMOT_FLOAT3, -950, -760)
    l3 = lerp(mat, l2, local_stone, mr, -800, -800)
    # the soil's own detail, the cracks most of all on crust, gone under stone
    soil = custom(mat, 'return C * lerp(1.0, Detail * Ratio, lerp(lerp(0.45, 0.9, Crust), 0.25, Rock));',
                  dict(C=l3, Detail=(big_n, 'B'), Crust=mc, Rock=mr, Ratio=node(mat, unreal.MaterialExpressionConstant, -950, -700, r=RATIO)), f3, -700, -800)
    patch1 = sample(mat, ff['T_FF_Patch_D'], custom(mat, 'return float2(World.x * 0.28 - World.y * 0.96, World.x * 0.96 + World.y * 0.28) / Span;',
                                                     dict(World=wp, Span=node(mat, unreal.MaterialExpressionConstant, -1300, -560, r=PATCH_SPANS[0] * 100.)), f2, -1150, -560), -950, -560, linear)
    patch2 = sample(mat, ff['T_FF_Patch_D'], custom(mat, 'return float2(World.x * 0.6 + World.y * 0.8, World.y * 0.6 - World.x * 0.8) / Span + N1.gb * 0.02;',
                                                     dict(World=wp, N1=n1, Span=node(mat, unreal.MaterialExpressionConstant, -1300, -470, r=PATCH_SPANS[1] * 100.)), f2, -1150, -470), -950, -470, linear)
    macro = custom(mat, """
// mean 1 in every factor. Brightness +-10 % over 40-200 m and +-8 % over 3-15 m, the hue from a redder soil to a greyer one
float3 c = (1.0 + 0.33 * (N1.r - 0.5)) * (1.0 + 0.27 * (N2.r - 0.5));
c *= lerp(float3(1.035, 0.995, 0.96), float3(0.965, 1.0, 1.05), smoothstep(0.25, 0.75, N1.g));
// between them a real patch of dry ground, 1-20 m, kept to about +-12 % and 8 %
c *= clamp(lerp(float3(1.0, 1.0, 1.0), P1.rgb * Ratio, 0.34), 0.72, 1.35);
c *= clamp(lerp(float3(1.0, 1.0, 1.0), P2.rgb * Ratio, 0.22), 0.8, 1.25);
return c;
""", dict(N1=n1, N2=n2, P1=patch1, P2=patch2, Ratio=node(mat, unreal.MaterialExpressionConstant, -950, -380, r=RATIO)), f3, -950, -600)
    vc = node(mat, unreal.MaterialExpressionVertexColor, -800, -600)
    tint = node(mat, unreal.MaterialExpressionVectorParameter, -800, -500, parameter_name='GroundTint', default_value=DEFAULT_TINT)
    # (a VertexColor's first output has no name: it was asked for 'RGB' here, which left this Multiply's A open at 0)
    vt = mul(mat, vc, tint, -600, -550)
    tinted = mul(mat, soil, vt, -450, -700)
    with_macro = mul(mat, tinted, macro, -300, -700)
    base = mul(mat, with_macro, stones_rgb, -200, -700)

    # roughness
    rl1 = lerp(mat, r['Sand'], r['Crust'], mc, -1100, -300, 'R', 'R')
    rl2 = lerp(mat, rl1, r['Gravel'], mg, -950, -300, '', 'R')
    rl3 = lerp(mat, rl2, node(mat, unreal.MaterialExpressionConstant, -1100, -200, r=.93), mr, -800, -300)
    rough = lerp(mat, rl3, node(mat, unreal.MaterialExpressionConstant, -800, -200, r=.72), stones_big, -650, -300)

    # normal, as M_S47_DesertGround: world XY, reconstructed on the geometric slope; then stone over it
    nl1 = lerp(mat, n['Sand'], n['Crust'], mc, -1100, 0, 'RGB', 'RGB')
    nl2 = lerp(mat, nl1, n['Gravel'], mg, -950, 0, '', 'RGB')
    # the scan's normal over them: a stone that is not here leaves no bump
    detail_n = custom(mat, """
float off = step(0.002, A) * (1.0 - M);
float2 xy = (N.xy * 2.0 - 1.0) * (1.0 - off);
return float3(Detail.xy + xy, Detail.z);
""", dict(N=big_n, A=(big_d, 'A'), M=stones_big, Detail=nl2), f3, -950, 100)
    ground_n = custom(mat, 'float3 n = normalize(MacroN); float3 t = normalize(float3(1,0,0) - n * n.x + float3(0,0.00001,0)); float3 b = normalize(cross(n,t)); return normalize(t * Detail.x + b * Detail.y + n * Detail.z);',
                      dict(Detail=detail_n, MacroN=n_ws), unreal.CustomMaterialOutputType.CMOT_FLOAT3, -800, 0)
    world_n = custom(mat, 'return normalize(lerp(A, B, Alpha));', dict(A=ground_n, B=stone_n, Alpha=mr), unreal.CustomMaterialOutputType.CMOT_FLOAT3, -600, 0)

    # displacement: no parallax, camera or derivatives on this path, as M_S47_DesertGround. The same stones, by the same pick, stand up here
    disp_pins = dict(Sed=uv1, T150=t150, T40=t40, Crust=crust_amount, Gravel=gravel_amount, N=n_ws)
    disp_masks = custom(mat, masks_code(False), disp_pins, unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1300, 1100)
    uv_d = custom(mat, 'return World.xy / Span + Pick;', dict(World=wp, Pick=pick, Span=node(mat, unreal.MaterialExpressionConstant, -1700, 1100, r=PEBBLE_SPAN[0] * 100.)), f2, -1600, 1100)
    disp_d = sample(mat, ff['T_FF_Stones_D'], uv_d, -1500, 1100, linear)
    disp_h = sample(mat, ff['T_FF_Stones_N'], uv_d, -1500, 1180, linear)
    uv_g = custom(mat, 'return World.xy / Span + Pick;', dict(World=wp, Pick=pick, Span=node(mat, unreal.MaterialExpressionConstant, -1700, 1280, r=SPAN['Gravel'] * 100.)), f2, -1600, 1280)
    disp_g = sample(mat, tex['Gravel', 'H'], uv_g, -1500, 1280, gray)
    disp = custom(mat, """
float m = smoothstep(1.0 - P, 1.06 - P, A);
float off = step(0.002, A) * (1.0 - m);
float h = lerp(H, 0.5, off);
h = lerp(h, Hg, M.y);
return lerp(h, 0.5, M.z);
""", dict(H=(disp_h, 'A'), A=(disp_d, 'A'), P=presence, Hg=(disp_g, 'R'), M=disp_masks), unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1000, 1100)

    attrs = node(mat, unreal.MaterialExpressionMakeMaterialAttributes, -100, -100)
    for out, pin in ((base, 'BaseColor'), (rough, 'Roughness'), (world_n, 'Normal'), (disp, 'Displacement')):
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


def ground_textures(report):
    """The detail textures: imported again only if the PNG under S47_FF_ROOT has changed. Without that folder the assets stand."""
    root = os.environ.get('S47_FF_ROOT')
    out = report.setdefault('ground_textures', {})
    made = {}
    for name, compression in GROUND_TEXTURES:
        path = DEST + '/' + name
        png = Path(root) / 'textures/out' / (name + '.png') if root else None
        state = '%d:%d' % (png.stat().st_size, png.stat().st_mtime) if png and png.exists() else None
        texture = E.load_asset(path) if E.does_asset_exist(path) else None
        if state and (texture is None or E.get_metadata_tag(texture, 'S47Source') != state):
            task = unreal.AssetImportTask()
            task.set_editor_property('filename', str(png))
            task.set_editor_property('destination_path', DEST)
            task.set_editor_property('destination_name', name)
            task.set_editor_property('replace_existing', True)
            task.set_editor_property('automated', True)
            task.set_editor_property('save', False)
            T.import_asset_tasks([task])
            texture = E.load_asset(path)
            if texture is None:
                raise RuntimeError('Could not import ' + str(png))
            texture.set_editor_property('srgb', False)
            texture.set_editor_property('compression_settings', getattr(unreal.TextureCompressionSettings, compression))
            E.set_metadata_tag(texture, 'S47Source', state)
            E.set_metadata_tag(texture, 'S47Owner', OWNER)
            if not E.save_loaded_asset(texture):
                raise RuntimeError('Could not save ' + path)
            out[name] = 'imported ' + state
        else:
            out[name] = 'kept'
        if texture is None:
            raise RuntimeError('Missing ground texture ' + path)
        made[name] = texture
    return made


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
    ff = ground_textures(report)
    mat = E.load_asset(path) if E.does_asset_exist(path) else None
    if mat is not None and E.get_metadata_tag(mat, 'S47Owner') != OWNER:
        raise RuntimeError('not owned: ' + path)
    if mat is None:
        mat = T.create_asset(NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
    else:
        L.delete_all_material_expressions(mat)
    build(mat, tex, stone, ff, out)
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

