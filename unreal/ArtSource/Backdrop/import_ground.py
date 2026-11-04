"""SARO world-projected sand/caliche/gravel blend at /Game/Backdrop/M_S47_DesertGround.
S47_GROUND_TEXTURES=1 imports twelve D/N/R/H maps from S47_GROUND_SOURCE. Otherwise
all colour layers retain exported T4. T4 also supplies stable 150 m / 40 m masks.
Generated normal maps are OpenGL; texture imports convert them to Unreal Y-.
"""
import json, os, struct, traceback
from pathlib import Path
import unreal
DEST = '/Game/Backdrop'; NAME = 'M_S47_DesertGround'; OWNER = 'S47DesertGround_v1'
E = unreal.EditorAssetLibrary; L = unreal.MaterialEditingLibrary; T = unreal.AssetToolsHelpers.get_asset_tools()

def node(mat, cls, x, y, **p):
    n = L.create_material_expression(mat, cls, x, y)
    for k, v in p.items(): n.set_editor_property(k, v)
    return n

def planar(mat, tex_param, metres, x, y, sampler=None, uv=None):
    wp = node(mat, unreal.MaterialExpressionWorldPosition, x - 500, y)
    m = node(mat, unreal.MaterialExpressionComponentMask, x - 380, y, r=True, g=True, b=False, a=False)
    L.connect_material_expressions(wp, '', m, '')
    d = node(mat, unreal.MaterialExpressionDivide, x - 260, y, const_b=metres * 100.)
    L.connect_material_expressions(m, '', d, 'A')
    s = node(mat, unreal.MaterialExpressionTextureSample, x, y)
    s.set_editor_property('texture', tex_param[1])
    if tex_param[1] is None: raise RuntimeError('ground texture missing for ' + tex_param[0])
    if sampler is not None: s.set_editor_property('sampler_type', sampler)
    L.connect_material_expressions(uv if uv is not None else d, '', s, 'UVs')
    return s


def import_textures(source):
    """Preflight every file/owner before replacing any texture or material."""
    inputs = []
    for layer in ('Sand', 'Crust', 'Gravel'):
        for role in ('D', 'N', 'R', 'H'):
            name = 'T_Ground%s_%s' % (layer, role)
            filename = source / (name + '.png')
            with filename.open('rb') as stream:
                header = stream.read(33)
            if (len(header) < 33 or header[:8] != b'\x89PNG\r\n\x1a\n'
                    or header[12:16] != b'IHDR'
                    or struct.unpack('>II', header[16:24]) != (2048, 2048)):
                raise RuntimeError('Expected a 2048 x 2048 PNG: ' + str(filename))
            path = DEST + '/Textures/' + name
            if E.does_asset_exist(path) and E.get_metadata_tag(E.load_asset(path), 'S47Owner') != OWNER:
                raise RuntimeError('Refusing unowned texture: ' + path)
            if role == 'H' and (header[24] != 16 or header[25] != 0):
                raise RuntimeError('Expected linear 16-bit grayscale height: ' + str(filename))
            inputs.append((layer, role, name, filename, path))
    result = {}
    for layer, role, name, filename, path in inputs:
        task = unreal.AssetImportTask()
        task.filename = str(filename)
        task.destination_path = DEST + '/Textures'
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = False
        T.import_asset_tasks([task])
        texture = E.load_asset(path)
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError('Texture import failed: ' + path)
        texture.set_editor_property('srgb', role == 'D')
        texture.set_editor_property('compression_settings', {
            'D': unreal.TextureCompressionSettings.TC_DEFAULT,
            'N': unreal.TextureCompressionSettings.TC_NORMALMAP,
            'R': unreal.TextureCompressionSettings.TC_GRAYSCALE,
            # Displacementmap samples alpha; our G16 PNG carries height in red.
            'H': unreal.TextureCompressionSettings.TC_GRAYSCALE,
        }[role])
        texture.set_editor_property('flip_green_channel', role == 'N')
        texture.set_editor_property('address_x', unreal.TextureAddress.TA_WRAP)
        texture.set_editor_property('address_y', unreal.TextureAddress.TA_WRAP)
        if (texture.get_editor_property('srgb') != (role == 'D')
                or texture.get_editor_property('flip_green_channel') != (role == 'N')):
            raise RuntimeError('Texture color space/normal convention mismatch: ' + path)
        E.set_metadata_tag(texture, 'S47Owner', OWNER)
        if not E.save_loaded_asset(texture):
            raise RuntimeError('Texture save failed: ' + path)
        result[(layer, role)] = texture
    return result


def blend(mat, a, b, alpha, x, y, pin=''):
    result = node(mat, unreal.MaterialExpressionLinearInterpolate, x, y)
    L.connect_material_expressions(a, pin, result, 'A')
    L.connect_material_expressions(b, pin, result, 'B')
    L.connect_material_expressions(alpha, '', result, 'Alpha')
    return result


def custom(mat, code, inputs, output, x, y):
    n = node(mat, unreal.MaterialExpressionCustom, x, y)
    n.set_editor_property('code', code)
    n.set_editor_property('output_type', output)
    pins = []
    for name, expression in inputs.items():
        pin = unreal.CustomInput()
        pin.set_editor_property('input_name', name)
        pins.append(pin)
    n.set_editor_property('inputs', pins)
    for name, expression in inputs.items():
        L.connect_material_expressions(expression, '', n, name)
    return n


def parallax_uv(mat, height, metres, y):
    # World XY projection: texture detail must remain aligned across separate meshes.
    wp = node(mat, unreal.MaterialExpressionWorldPosition, -2200, y)
    cam = node(mat, unreal.MaterialExpressionCameraPositionWS, -2200, y + 80)
    span = node(mat, unreal.MaterialExpressionConstant, -2200, y + 160, r=metres * 100.)
    depth = node(mat, unreal.MaterialExpressionScalarParameter, -2200, y + 240,
                 parameter_name='MicroReliefCm', default_value=1.2)
    tex = node(mat, unreal.MaterialExpressionTextureObject, -2200, y + 320, texture=height)
    # Fixed 8-step near-field POM; explicit gradients avoid divergent derivative issues.
    # Flat0.5 is unchanged. Fade at6–16m and grazing clamp prevent horizon swimming.
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
    return custom(mat, code, dict(World=wp, Camera=cam, Span=span, Depth=depth, Height=tex),
                  unreal.CustomMaterialOutputType.CMOT_FLOAT2, -1700, y)


def displacement(mat, textures, crust_mask, gravel_mask):
    # No camera-dependent parallax or pixel derivatives on the displacement path.
    samples = [planar(mat, (layer+'H', textures[(layer,'H')]), scale, -900, 2600+i*180,
                      unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
               for i,(layer,scale) in enumerate((('Sand',2.4),('Crust',3.2),('Gravel',1.6)))]
    first = blend(mat, samples[0], samples[1], crust_mask, -300, 2600, 'R')
    final = node(mat, unreal.MaterialExpressionLinearInterpolate, 0, 2600)
    L.connect_material_expressions(first, '', final, 'A')
    L.connect_material_expressions(samples[2], 'R', final, 'B')
    L.connect_material_expressions(gravel_mask, '', final, 'Alpha')
    # MP_Displacement is hidden from Python in UE5.8. The public attributes
    # expression exposes the same native Displacement input without enum hacks.
    attrs = node(mat, unreal.MaterialExpressionMakeMaterialAttributes, 600, 2600)
    for prop, pin in ((unreal.MaterialProperty.MP_BASE_COLOR, 'BaseColor'),
                      (unreal.MaterialProperty.MP_NORMAL, 'Normal'),
                      (unreal.MaterialProperty.MP_ROUGHNESS, 'Roughness')):
        expression = L.get_material_property_input_node(mat, prop)
        output = L.get_material_property_input_node_output_name(mat, prop)
        if not L.connect_material_expressions(expression, output, attrs, pin):
            raise RuntimeError('Could not connect ground attribute: ' + pin)
    if not L.connect_material_expressions(final, '', attrs, 'Displacement'):
        raise RuntimeError('Could not connect ground displacement')
    mat.set_editor_property('use_material_attributes', True)
    L.connect_material_property(attrs, '', unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    mat.set_editor_property('enable_tessellation', True)
    mat.set_editor_property('used_with_nanite', True)
    scaling = mat.get_editor_property('displacement_scaling')
    scaling.set_editor_property('magnitude', 7.0)
    scaling.set_editor_property('center', 0.5)
    mat.set_editor_property('displacement_scaling', scaling)


def texture_details(mat, textures, crust_mask, gravel_mask, relief_uvs):
    for role, sampler, prop in (
            ('R', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE, unreal.MaterialProperty.MP_ROUGHNESS),
            ('N', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, unreal.MaterialProperty.MP_NORMAL)):
        y = 1000 if role == 'R' else 1800
        samples = [planar(mat, (layer + role, textures[(layer, role)]), scale, -900, y + i * 180, sampler, relief_uvs[layer])
                   for i, (layer, scale) in enumerate((('Sand', 2.4), ('Crust', 3.2), ('Gravel', 1.6)))]
        pin = 'R' if role == 'R' else 'RGB'
        first = blend(mat, samples[0], samples[1], crust_mask, -300, y, pin)
        final = node(mat, unreal.MaterialExpressionLinearInterpolate, 0, y)
        L.connect_material_expressions(first, '', final, 'A')
        L.connect_material_expressions(samples[2], pin, final, 'B')
        L.connect_material_expressions(gravel_mask, '', final, 'Alpha')
        if role == 'N':
            # UVs use world XY, so the mesh's authored tangent frame must not rotate
            # the normal detail. Reconstruct that world frame on the geometric slope.
            mat.set_editor_property('tangent_space_normal', False)
            macro = node(mat, unreal.MaterialExpressionVertexNormalWS, 0, y + 200)
            frame = node(mat, unreal.MaterialExpressionCustom, 250, y)
            frame.set_editor_property('code', 'float3 n = normalize(MacroN); float3 t = normalize(float3(1,0,0) - n * n.x + float3(0,0.00001,0)); float3 b = normalize(cross(n,t)); return normalize(t * Detail.x + b * Detail.y + n * Detail.z);')
            frame.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            pins = []
            for name in ('Detail', 'MacroN'):
                item = unreal.CustomInput()
                item.set_editor_property('input_name', name)
                pins.append(item)
            frame.set_editor_property('inputs', pins)
            L.connect_material_expressions(final, '', frame, 'Detail')
            L.connect_material_expressions(macro, '', frame, 'MacroN')
            final = frame
        L.connect_material_property(final, '', prop)

def main():
    report = Path(os.environ['S47_GROUND_REPORT']); out = {'status': 'RUNNING'}
    try:
        sand = unreal.load_asset('/Game/Web/Tex/T4.T4')
        out['sand'] = str(sand)
        enabled = os.environ.get('S47_GROUND_TEXTURES') == '1'
        path = DEST + '/' + NAME
        mat = E.load_asset(path) if E.does_asset_exist(path) else None
        if mat and E.get_metadata_tag(mat, 'S47Owner') != OWNER:
            raise RuntimeError('not owned: ' + path)
        if sand is None:
            raise RuntimeError('Exported T4 is required for the existing macro masks')
        textures = import_textures(Path(os.environ['S47_GROUND_SOURCE'])) if enabled else {}
        out['textures'] = {layer + '_' + role: value.get_path_name() for (layer, role), value in textures.items()}
        def tex(n):
            return textures[(n, 'D')] if enabled else sand
        # Preserve the material object and references. Clear only after all inputs validate.
        if mat:
            L.delete_all_material_expressions(mat)
        else:
            mat = T.create_asset(NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
        mat.set_editor_property('tangent_space_normal', True)
        mat.set_editor_property('use_material_attributes', False)
        mat.set_editor_property('enable_tessellation', False)
        relief_uvs = {layer: parallax_uv(mat, textures[(layer, 'H')], scale, i * 350) for i, (layer, scale) in enumerate((('Sand', 2.4), ('Crust', 3.2), ('Gravel', 1.6)))} if enabled else {}
        sandA = planar(mat, ('SandTex', tex('Sand')), 2.4, -900, -400, uv=relief_uvs.get('Sand'))
        sandB = planar(mat, ('SandTex', tex('Sand')), 9.0, -900, -200)
        crust = planar(mat, ('CrustTex', tex('Crust')), 3.2, -900, 0, uv=relief_uvs.get('Crust'))
        grav = planar(mat, ('GravelTex', tex('Gravel')), 1.6, -900, 200, uv=relief_uvs.get('Gravel'))
        mask1 = planar(mat, ('MacroSand', sand), 150., -900, 400)
        mask2 = planar(mat, ('MacroSand', sand), 40., -900, 600)
        # The generated albedo already carries its colour; multiplying it by itself
        # exaggerates saturation. Mix scales gently, keeping the original fallback.
        sand_mix = node(mat, unreal.MaterialExpressionLinearInterpolate, -600, -300, const_alpha=.2) if enabled else node(mat, unreal.MaterialExpressionMultiply, -600, -300)
        L.connect_material_expressions(sandA, 'RGB', sand_mix, 'A'); L.connect_material_expressions(sandB, 'RGB', sand_mix, 'B')
        sand_gain = node(mat, unreal.MaterialExpressionMultiply, -500, -300, const_b=1.0 if enabled else 1.9)
        L.connect_material_expressions(sand_mix, '', sand_gain, 'A')
        crust_tint = node(mat, unreal.MaterialExpressionVectorParameter, -700, 80, parameter_name='CrustTint', default_value=unreal.LinearColor(.94, .94, .94, 1) if enabled else unreal.LinearColor(1.12, 1.06, .98, 1))
        crust_c = node(mat, unreal.MaterialExpressionMultiply, -600, 0)
        L.connect_material_expressions(crust, 'RGB', crust_c, 'A'); L.connect_material_expressions(crust_tint, '', crust_c, 'B')
        grav_tint = node(mat, unreal.MaterialExpressionVectorParameter, -700, 280, parameter_name='GravelTint', default_value=unreal.LinearColor(.94, .94, .94, 1) if enabled else unreal.LinearColor(.62, .56, .50, 1))
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
        gt = node(mat, unreal.MaterialExpressionVectorParameter, 0, -450, parameter_name='GroundTint', default_value=unreal.LinearColor(.65, .65, .65, 1) if enabled else unreal.LinearColor(.807, .716, .578, 1))
        vt = node(mat, unreal.MaterialExpressionMultiply, 100, -350)
        L.connect_material_expressions(vc, '', vt, 'A'); L.connect_material_expressions(gt, '', vt, 'B')
        base = node(mat, unreal.MaterialExpressionMultiply, 150, -100)
        L.connect_material_expressions(vt, '', base, 'A'); L.connect_material_expressions(l2, '', base, 'B')
        L.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
        if enabled:
            texture_details(mat, textures, mc, mg, relief_uvs)
            displacement(mat, textures, mc, mg)
        else:
            rough = node(mat, unreal.MaterialExpressionLinearInterpolate, 150, 200, const_a=.97, const_b=.86)
            L.connect_material_expressions(mc, '', rough, 'Alpha')
            L.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
        errors = [str(error) for error in L.recompile_material(mat)]
        out['compile_errors'] = errors
        if errors:
            raise RuntimeError('Ground material compilation failed: ' + '; '.join(errors))
        E.set_metadata_tag(mat, 'S47Owner', OWNER)
        if not E.save_loaded_asset(mat):
            raise RuntimeError('Ground material save failed')
        if enabled:
            import sys
            sys.path.insert(0, str(Path(__file__).resolve().parent))
            from ground_nanite import configure_ground_nanite
            out['ground_nanite'] = configure_ground_nanite()
            out['relief'] = {'micro_cm': 1.2, 'displacement_peak_to_trough_cm': 7.0, 'center': 0.5,
                             'nanite_tessellation': unreal.SystemLibrary.get_console_variable_int_value('r.Nanite.Tessellation')}
            if out['relief']['nanite_tessellation'] != 1:
                raise RuntimeError('Nanite tessellation is disabled')
        out['status'] = 'PASS_GROUND_MATERIAL'; out['material'] = mat.get_path_name()
    except Exception:
        out['status'] = 'FAIL'; out['error'] = traceback.format_exc()
    report.write_text(json.dumps(out, indent=2) + '\n')

if os.environ.get('S47_GROUND_APPLY') == '1': main()
