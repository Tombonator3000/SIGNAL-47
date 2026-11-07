"""UnrealEditor-Cmd Python: SM_FF_Range, its two maps and M_S47_FreeformRange, in /Game/FreeformTerrain.

S47_FF_ROOT = the folder with mesh/SM_FF_Range.glb and range/*.png; S47_FF_REPORT = the JSON to write. Same import path as
the ground (a GLB by AssetImportTask). No collision. Nanite with a fallback of 4 %, no distance field, no ray tracing of its
own use beyond the fallback. Touches nothing else in the folder. Run by run_import_range.py under the production lease.

The material:
- the foot uses the ground material's own layers, world UVs, tint and macro colour (M_S47_FreeformGround), so that the land
  runs from one mesh to the other without a line. GroundTint, GravelAmount and CrustAmount come from S47MesaRock, as the
  ground's do. FootVertexColor stands in for the ground mesh's vertex colour.
- above the foot none of that: those layers tile every 1.6 to 150 m, and on a hillside a kilometre off the tiling is a grid of
  squares. There the soil is the same layers' mean colour (a fixed high mip, so tint and light stay what they were) times T_FF_Range_C
  (bake_range_color.py: USGS NAIP over the measured land, made colour over the made hills), varied by large turned samples of the macro map.
- bare rock where T_FF_Range_C.a says, in the beds of T_FF_Range_B: red-brown and grey-brown with pale ledges, by a tilted height that wanders, so they dip and carry on
  across a valley. Shrubs on the lower slopes (T_FF_Range_M: R shrubs, B cavity, A foot), and a dry scrub tone in a band above the foot
- no specular: at Specular above 0 UE's Fresnel goes to 1 at grazing light, and a low sun drew white stripes down the flanks
- the normal from T_FF_Range_N, in the mesh's own axes, so no triangle shows however large it is
- haze by distance from the camera, into the base colour only, with the normal levelling off the same way, so the
  far ranges go blue-grey and flat by day and dark with the rest of the land at night. Nothing here glows.
"""
import json
import os
from pathlib import Path
import sys
import traceback

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_ground_relief as G

DEST = '/Game/FreeformTerrain'
NAME = 'M_S47_FreeformRange'
MESH = 'SM_FF_Range'
OWNER = 'S47FreeformRange_v1'
E = unreal.EditorAssetLibrary
L = unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
FALLBACK_SHARE = .04
MEAN_MIP = 9    # of the ground's 2048 layers: 4 x 4 texels to a tile
BED_TILT, BED_SECTION_M = (5800., 3600.), 512.    # metres of bed height over the map's UV: about 16 degrees; bake_range_color.py SECTION_M
OLD_MESH = '/Game/CalicheStreet/Backdrop/SM_Caliche_Mesas.SM_Caliche_Mesas'


def owned_or_new(path):
    if E.does_asset_exist(path):
        if E.get_metadata_tag(E.load_asset(path), 'S47Owner') != OWNER:
            raise RuntimeError('not owned: ' + path)
        if not E.delete_asset(path):
            raise RuntimeError('could not replace ' + path)


def import_texture(root, name, srgb, compression, sampler_group, wrap=False):
    path = DEST + '/' + name
    owned_or_new(path)
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(root / 'range' / (name + '.png')))
    task.set_editor_property('destination_path', DEST)
    task.set_editor_property('destination_name', name)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', False)
    T.import_asset_tasks([task])
    tex = E.load_asset(path)
    if tex is None:
        raise RuntimeError('texture did not import: ' + name)
    tex.set_editor_property('srgb', srgb)
    tex.set_editor_property('compression_settings', compression)
    tex.set_editor_property('lod_group', sampler_group)
    tex.set_editor_property('address_x', unreal.TextureAddress.TA_WRAP if wrap else unreal.TextureAddress.TA_CLAMP)
    tex.set_editor_property('address_y', unreal.TextureAddress.TA_WRAP if wrap else unreal.TextureAddress.TA_CLAMP)
    tex.set_editor_property('virtual_texture_streaming', False)
    tex.set_editor_property('flip_green_channel', False)
    E.set_metadata_tag(tex, 'S47Owner', OWNER)
    if not E.save_loaded_asset(tex):
        raise RuntimeError('could not save ' + path)
    return tex


def build_material(mat, tex, stone, macro, normal_map, mask_map, fine_noise, color_map, beds_map, out):
    node, wire, lerp, mul, custom, sample, world_uv = G.node, G.wire, G.lerp, G.mul, G.custom, G.sample, G.world_uv
    F1, F3 = unreal.CustomMaterialOutputType.CMOT_FLOAT1, unreal.CustomMaterialOutputType.CMOT_FLOAT3
    F2 = unreal.CustomMaterialOutputType.CMOT_FLOAT2

    def turned(metres, degrees, x, y):
        """World metres to a macro map's UV at this size, turned, so that no two of them line up and no row runs along an axis."""
        code = 'float a = %f; float2 p = World.xy * 0.01; return float2(p.x * cos(a) - p.y * sin(a), p.x * sin(a) + p.y * cos(a)) / %f;' % (degrees * .0174533, metres)
        return custom(mat, code, dict(World=wp), F2, x, y)

    def mean_of(texture, x, y):
        """The texture's own mean: a fixed high mip, whatever the distance. Same colour as the full layers, none of their pattern."""
        s = sample(mat, texture, uv['Sand'], x, y)
        s.set_editor_property('mip_value_mode', unreal.TextureMipValueMode.TMVM_MIP_LEVEL)
        s.set_editor_property('const_mip_value', MEAN_MIP)
        return s
    gray = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE
    color = dict(r=True, g=True, b=False, a=False)
    wp = node(mat, unreal.MaterialExpressionWorldPosition, -3600, -1800)
    cam = node(mat, unreal.MaterialExpressionCameraPositionWS, -3600, -1720)
    uv0 = node(mat, unreal.MaterialExpressionTextureCoordinate, -3600, -1640, coordinate_index=0)

    # the two maps of the land itself
    n_map = node(mat, unreal.MaterialExpressionTextureSample, -3200, -1500)
    n_map.set_editor_property('texture', normal_map)
    n_map.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    G.shared(n_map)
    n_map.set_editor_property('sampler_source', unreal.SamplerSourceMode.SSM_CLAMP_WORLD_GROUP_SETTINGS)
    wire(uv0, '', n_map, 'UVs')
    m_map = node(mat, unreal.MaterialExpressionTextureSample, -3200, -1300)
    m_map.set_editor_property('texture', mask_map)
    m_map.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    m_map.set_editor_property('sampler_source', unreal.SamplerSourceMode.SSM_CLAMP_WORLD_GROUP_SETTINGS)
    wire(uv0, '', m_map, 'UVs')
    c_map = node(mat, unreal.MaterialExpressionTextureSample, -3200, -1100)
    c_map.set_editor_property('texture', color_map)
    c_map.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    c_map.set_editor_property('sampler_source', unreal.SamplerSourceMode.SSM_CLAMP_WORLD_GROUP_SETTINGS)
    wire(uv0, '', c_map, 'UVs')
    # object space to the world's (the shared landscape can be a quarter-turned)
    to_world = node(mat, unreal.MaterialExpressionTransform, -3000, -1500)
    to_world.set_editor_property('transform_source_type', unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
    to_world.set_editor_property('transform_type', unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    wire(n_map, 'RGB', to_world, '')

    haze_start = node(mat, unreal.MaterialExpressionScalarParameter, -3000, -1900, parameter_name='HazeStartM', default_value=900.)
    haze_scale = node(mat, unreal.MaterialExpressionScalarParameter, -3000, -1820, parameter_name='HazeScaleM', default_value=3800.)
    haze_most = node(mat, unreal.MaterialExpressionScalarParameter, -3000, -1740, parameter_name='HazeMost', default_value=.88)
    # (less at the foot, for the first few kilometres: the plain beside it has no haze of its own, and a hill's blue foot on an orange
    # plain is two layers, not one land)
    haze = custom(mat, 'float d = length(Camera - World) * 0.01; return Most * (1.0 - exp(-max(d - Start, 0.0) / Scale)) * lerp(1.0, 0.2, M.a * (1.0 - smoothstep(3000.0, 8000.0, d)));',
                  dict(Camera=cam, World=wp, Start=haze_start, Scale=haze_scale, Most=haze_most, M=(m_map, 'RGBA')), F1, -2700, -1850)

    # the layers of the ground's own material, at the same world UVs. No parallax out here.
    uv = {layer: world_uv(mat, wp, color, G.SPAN[layer], -2500, -1400 + i * 200) for i, layer in enumerate(('Sand', 'Crust', 'Gravel'))}
    d = {layer: sample(mat, tex[layer, 'D'], uv[layer], -2200, -1400 + i * 200) for i, layer in enumerate(('Sand', 'Crust', 'Gravel'))}
    h = {layer: sample(mat, tex[layer, 'H'], uv[layer], -2000, -1400 + i * 200, gray) for i, layer in enumerate(('Sand', 'Crust', 'Gravel'))}
    uv150 = world_uv(mat, wp, color, 150., -2500, -800)
    uv40 = world_uv(mat, wp, color, 40., -2500, -700)
    uv47 = world_uv(mat, wp, color, 47., -2500, -600)
    uv430 = world_uv(mat, wp, color, 430., -2500, -500)
    t150 = sample(mat, macro, uv150, -2200, -800)
    t40 = sample(mat, macro, uv40, -2200, -700)
    t47 = sample(mat, macro, uv47, -2200, -600)
    t430 = sample(mat, macro, uv430, -2200, -500)
    p1 = sample(mat, macro, turned(613., 23., -2500, -400), -2200, -400)
    p2 = sample(mat, macro, turned(1717., -37., -2500, -320), -2200, -320)
    p3 = sample(mat, macro, turned(211., 61., -2500, -240), -2200, -240)
    # the shrubs' dots, about 2 m: T_FF_Noise's fine channels at two sizes that share nothing, gone to an even tone with distance.
    # (at 300 and 413 m they ran together into camouflage: 15 m smears)
    gray_linear = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
    d1 = sample(mat, fine_noise, turned(97., 17., -2500, -60), -2200, 20, gray_linear)
    d2 = sample(mat, fine_noise, turned(131., -52., -2500, 20), -2200, 100, gray_linear)
    mean_sand = mean_of(tex['Sand', 'D'], -2200, -160)
    mean_gravel = mean_of(tex['Gravel', 'D'], -2200, -80)
    crust_amount = node(mat, unreal.MaterialExpressionScalarParameter, -2200, -300, parameter_name='CrustAmount', default_value=.75)
    gravel_amount = node(mat, unreal.MaterialExpressionScalarParameter, -2200, -200, parameter_name='GravelAmount', default_value=.85)

    # stone from the sides, as the ground's, but larger: from here it is outcrops, not grains
    uvx = world_uv(mat, wp, dict(r=False, g=True, b=True, a=False), 34., -2500, 0)
    uvy = world_uv(mat, wp, dict(r=True, g=False, b=True, a=False), 34., -2500, 100)
    uvz = world_uv(mat, wp, color, 34., -2500, 200)
    sx = sample(mat, stone['D'], uvx, -2200, 0)
    sy = sample(mat, stone['D'], uvy, -2200, 100)
    sz = sample(mat, stone['D'], uvz, -2200, 200)
    # the beds: a height that tilts about 16 degrees over the map, folds over 1.7 km and wanders over less
    bed_uv = custom(mat, 'float h = World.z * 0.01 + UV.x * %f + UV.y * %f + (P2 - 0.4) * 130.0 + (P1 - 0.4) * 46.0 + (P3 - 0.4) * 14.0; return float2(h / %f, 0.5);' % (BED_TILT[0], BED_TILT[1], BED_SECTION_M),
                    dict(World=wp, UV=uv0, P1=(p1, 'R'), P2=(p2, 'R'), P3=(p3, 'R')), F2, -2500, 280)
    beds = node(mat, unreal.MaterialExpressionTextureSample, -2200, 280)
    beds.set_editor_property('texture', beds_map)
    beds.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    G.shared(beds)
    wire(bed_uv, '', beds, 'UVs')
    weights = custom(mat, 'float3 w = pow(abs(normalize(N)), 5.0); return w / max(w.x + w.y + w.z, 0.0001);', dict(N=(to_world, '')), F3, -2000, 0)
    stone_d = custom(mat, 'return X * W.x + Y * W.y + Z * W.z;', dict(X=(sx, 'RGB'), Y=(sy, 'RGB'), Z=(sz, 'RGB'), W=weights), F3, -1800, 0)

    pins = dict(T150=(t150, 'R'), T40=(t40, 'R'), Crust=crust_amount, Gravel=gravel_amount, Hs=(h['Sand'], 'R'), Hc=(h['Crust'], 'R'), Hg=(h['Gravel'], 'R'),
                M=(m_map, 'RGBA'), N=(to_world, ''))
    # the ground's masks; then gravel comes up with the slope, and the shrubs' map and the bare rock decide the rest
    masks = custom(mat, """
float deg = acos(saturate(normalize(N).z)) * 57.29578;
float crust0 = saturate((T150 - 0.46) / 0.16) * Crust;
float gravel0 = saturate(((1.0 - T40) - 0.58) / 0.16) * Gravel;
gravel0 = max(gravel0, smoothstep(4.0, 14.0, deg) * 0.8 * (1.0 - M.a));
float k = 0.35;
float mc = smoothstep(0.0, 1.0, crust0 * (1.0 + 2.0 * k) - k + (Hc - Hs) * k);
float under = lerp(Hs, Hc, mc);
float mg = smoothstep(0.0, 1.0, gravel0 * (1.0 + 2.0 * k) - k + (Hg - under) * k);
return float3(mc, mg, deg);
""", pins, F3, -1700, -1200)
    pick = lambda name, y: _pick(mat, masks, name, y)
    mc, mg = pick('r', -1300), pick('g', -1200)

    crust_tint = node(mat, unreal.MaterialExpressionVectorParameter, -1500, -1000, parameter_name='CrustTint', default_value=unreal.LinearColor(.94, .94, .94, 1))
    gravel_tint = node(mat, unreal.MaterialExpressionVectorParameter, -1500, -900, parameter_name='GravelTint', default_value=unreal.LinearColor(.94, .94, .94, 1))
    crust = mul(mat, d['Crust'], crust_tint, -1300, -1000, 'RGB')
    gravel = mul(mat, d['Gravel'], gravel_tint, -1300, -900, 'RGB')
    l1 = lerp(mat, d['Sand'], crust, mc, -1100, -1100, 'RGB')
    l2 = lerp(mat, l1, gravel, mg, -950, -1000)

    macro_c = custom(mat, """
// as the ground's macro colour
float big = saturate((Z - 0.22) / 0.33);
float small = saturate((M - 0.22) / 0.33);
float3 caliche = float3(1.10, 1.09, 1.07);
float3 neutral = float3(1.00, 1.00, 1.00);
float3 greyish = float3(0.93, 0.95, 0.98);
float3 reddish = float3(1.03, 0.92, 0.83);
float3 c = big < 0.33 ? lerp(caliche, neutral, big / 0.33) : big < 0.66 ? lerp(neutral, greyish, (big - 0.33) / 0.33) : lerp(greyish, reddish, (big - 0.66) / 0.34);
return c * (0.88 + 0.24 * small);
""", dict(Z=(t430, 'R'), M=(t47, 'R')), F3, -950, -760)
    tint = node(mat, unreal.MaterialExpressionVectorParameter, -950, -650, parameter_name='GroundTint', default_value=G.DEFAULT_TINT)
    vc = node(mat, unreal.MaterialExpressionVectorParameter, -950, -560, parameter_name='FootVertexColor', default_value=unreal.LinearColor(.82, .80, .76, 1))
    rock_tint = node(mat, unreal.MaterialExpressionVectorParameter, -950, -470, parameter_name='RockTint', default_value=unreal.LinearColor(1., 1., 1., 1))
    scrub = node(mat, unreal.MaterialExpressionVectorParameter, -950, -380, parameter_name='ShrubColor', default_value=unreal.LinearColor(.050, .055, .030, 1))
    dry = node(mat, unreal.MaterialExpressionVectorParameter, -950, -200, parameter_name='DryScrub', default_value=unreal.LinearColor(.135, .110, .070, 1))
    haze_col = node(mat, unreal.MaterialExpressionVectorParameter, -950, -290, parameter_name='HazeColor', default_value=unreal.LinearColor(.12, .19, .34, 1))
    land = custom(mat, """
// the foot: the ground's own soil, its pattern going with distance, where the plain's own has gone to its mean as well
float foot = smoothstep(0.15, 0.85, M.a) * (1.0 - saturate((length(Camera - World) * 0.01 - 700.0) / 900.0));
float3 mean = lerp(MeanSand, MeanGravel, 0.55) * Tint * Vc;
// (what the land is made of, T_FF_Range_C: the photograph's colour or the made hill's, as a ratio to the plain's soil)
float3 soil = lerp(mean * Col.rgb * 2.5, Soil * Tint * Vc * Macro, foot);
float up = 1.0 - M.a;
float3 bedRatio = Bed.rgb * 2.5;
// large patches, so no slope is one colour
soil *= lerp(1.0, 0.92 + 0.16 * saturate((P3 - 0.22) / 0.33), up * (1.0 - saturate(Haze * 3.0)));
// the beds through the talus as well, faintly: a bench shows in the soil before the rock does
soil *= lerp(float3(1.0, 1.0, 1.0), bedRatio / float3(0.92, 0.76, 0.66), 0.22 * up);
// a dry scrub tone in a band above the foot
float band = M.a * (1.0 - M.a) * 4.0;
soil = lerp(soil, Dry, band * 0.2);
// bare rock, cut with the stone's pattern, in its beds. No rock on the fans or at the foot; the map's mask says where
float rock = saturate(Col.a * up);
rock *= smoothstep(0.15, 0.55, rock + (dot(Stone, float3(0.3, 0.6, 0.1)) * 3.0 - 0.45) * 0.5);
float3 stone = mean * RockTint * bedRatio * clamp(Stone / float3(0.183, 0.132, 0.109), 0.55, 1.4);
float3 c = lerp(soil, stone, rock);
// shrubs as a dark speckle
float near = saturate((length(Camera - World) * 0.01 - 120.0) / 200.0);
float dotsAt = D1 * 0.55 + D2 * 0.45;
float want = lerp(0.66, 0.52, saturate(M.r * 0.8 + 0.1));
float shrub = smoothstep(want, want + 0.07, dotsAt) * (1.0 - rock);
c = lerp(c, Shrub, shrub * near * lerp(0.62, 0.35, saturate(Haze * 2.0)));
// and where they crowd, in what is cut in, the ground under them darker too
c = lerp(c, Shrub, M.r * (1.0 - rock) * 0.18);
return c * lerp(0.48, 1.0, M.b);
""", dict(Soil=l2, Tint=tint, Vc=vc, Macro=macro_c, M=(m_map, 'RGBA'), Col=(c_map, 'RGBA'), Bed=(beds, 'RGBA'), Stone=stone_d, RockTint=rock_tint, P3=(p3, 'R'), MeanSand=(mean_sand, 'RGB'), MeanGravel=(mean_gravel, 'RGB'),
          Shrub=scrub, Dry=dry, Haze=haze, Camera=cam, World=wp, D1=(d1, 'A'), D2=(d2, 'B')), F3, -600, -700)
    base = lerp(mat, land, haze_col, haze, -300, -700)

    normal = custom(mat, """
float3 n = normalize(N);
// farther off the shading evens out: air, not a change in the land. The far side of a range still goes darker against a low sun.
float3 toEye = normalize(Camera - World);
float3 flat_ = normalize(float3(0, 0, 1) * 0.7 + toEye * 0.7);
return normalize(lerp(n, flat_, saturate(Haze * 0.45)));
""", dict(N=(to_world, ''), Camera=cam, World=wp, Haze=haze), F3, -300, -300)
    rough = node(mat, unreal.MaterialExpressionConstant, -300, -150, r=.92)
    spec = node(mat, unreal.MaterialExpressionConstant, -300, -60, r=0.)
    L.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
    L.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    L.connect_material_property(spec, '', unreal.MaterialProperty.MP_SPECULAR)
    L.connect_material_property(normal, '', unreal.MaterialProperty.MP_NORMAL)
    mat.set_editor_property('tangent_space_normal', False)
    mat.set_editor_property('used_with_nanite', True)
    out['emissive_connected'] = False


def _pick(mat, source, name, y):
    m = G.node(mat, unreal.MaterialExpressionComponentMask, -1500, y, r=name == 'r', g=name == 'g', b=name == 'b', a=False)
    G.wire(source, '', m, '')
    return m


def make_material(normal_map, mask_map, color_map, beds_map, report):
    out = report.setdefault('material', {'status': 'RUNNING'})
    tex = {}
    for layer in ('Sand', 'Crust', 'Gravel'):
        for role in ('D', 'H'):
            t = E.load_asset('/Game/Backdrop/Textures/T_Ground%s_%s' % (layer, role))
            if t is None:
                raise RuntimeError('Missing ground texture ' + layer + role)
            tex[layer, role] = t
    stone = {'D': E.load_asset(G.STONE + 'base_color')}
    if stone['D'] is None:
        raise RuntimeError('Missing stone: ' + G.STONE)
    macro = unreal.load_asset('/Game/Web/Tex/T4.T4')
    if macro is None:
        raise RuntimeError('T4 is required for the macro colour')
    path = DEST + '/' + NAME
    mat = E.load_asset(path) if E.does_asset_exist(path) else None
    if mat is not None and E.get_metadata_tag(mat, 'S47Owner') != OWNER:
        raise RuntimeError('not owned: ' + path)
    if mat is None:
        mat = T.create_asset(NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
    else:
        L.delete_all_material_expressions(mat)
    fine_noise = E.load_asset(DEST + '/T_FF_Noise')
    if fine_noise is None:
        raise RuntimeError('T_FF_Noise is required for the shrubs: run the ground material first')
    build_material(mat, tex, stone, macro, normal_map, mask_map, fine_noise, color_map, beds_map, out)
    errors = [str(e) for e in L.recompile_material(mat)]
    out['compile_errors'] = errors
    if errors:
        raise RuntimeError('Freeform range material failed: ' + '; '.join(errors))
    E.set_metadata_tag(mat, 'S47Owner', OWNER)
    if not E.save_loaded_asset(mat):
        raise RuntimeError('Freeform range material save failed')
    out['path'] = mat.get_path_name()
    out['status'] = 'PASS'
    return mat


def import_mesh(root, mat, out):
    path = DEST + '/' + MESH
    owned_or_new(path)
    work = DEST + '/_import_' + MESH
    if E.does_directory_exist(work):
        E.delete_directory(work)
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(root / 'mesh' / (MESH + '.glb')))
    task.set_editor_property('destination_path', work)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    T.import_asset_tasks([task])
    made = E.list_assets(work, recursive=True)
    source = None
    for a in made:
        if isinstance(E.load_asset(a), unreal.StaticMesh):
            source = a.split('.')[0]
    if source is None:
        raise RuntimeError('No static mesh: ' + str(made))
    mesh = E.load_asset(source)
    mesh.set_material(0, mat)
    if not E.rename_asset(source, path):
        raise RuntimeError('could not move mesh to ' + path)
    mesh = E.load_asset(path)
    E.delete_directory(work)
    b = S.get_lod_build_settings(mesh, 0)
    b.set_editor_property('generate_lightmap_u_vs', False)
    b.set_editor_property('use_full_precision_u_vs', True)
    b.set_editor_property('remove_degenerates', False)
    b.set_editor_property('recompute_normals', False)
    b.set_editor_property('distance_field_resolution_scale', 0.)
    S.set_lod_build_settings(mesh, 0, b)
    mesh.set_editor_property('light_map_coordinate_index', 0)
    n = mesh.get_editor_property('nanite_settings')
    n.set_editor_property('enabled', True)
    n.set_editor_property('generate_fallback', unreal.NaniteGenerateFallback.ENABLED)
    n.set_editor_property('fallback_target', unreal.NaniteFallbackTarget.PERCENT_TRIANGLES)
    n.set_editor_property('fallback_percent_triangles', FALLBACK_SHARE)
    n.set_editor_property('fallback_relative_error', 0.)
    mesh.set_editor_property('nanite_settings', n)
    mesh.set_editor_property('support_ray_tracing', False)
    # no collision of any kind: nothing stands on it and nothing may hit it
    setup = mesh.get_editor_property('body_setup')
    if setup:
        setup.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
        S.remove_collisions(mesh)
    E.set_metadata_tag(mesh, 'S47Owner', OWNER)
    if not E.save_loaded_asset(mesh):
        raise RuntimeError('could not save ' + path)
    box = mesh.get_bounding_box()
    desc = mesh.get_static_mesh_description(0)
    actual = mesh.get_editor_property('nanite_settings')
    out.update(source_triangles=desc.get_triangle_count(), source_vertices=desc.get_vertex_count(), fallback_triangles=mesh.get_num_triangles(0),
               nanite=actual.enabled, fallback_percent=actual.fallback_percent_triangles,
               bounds_cm={'min': [box.min.x, box.min.y, box.min.z], 'max': [box.max.x, box.max.y, box.max.z]},
               collision_primitives=S.get_simple_collision_count(mesh) if hasattr(S, 'get_simple_collision_count') else None)
    old = E.load_asset(OLD_MESH)
    if old is not None:
        o = old.get_bounding_box()
        out['old_mesa_bounds_cm'] = {'min': [o.min.x, o.min.y, o.min.z], 'max': [o.max.x, o.max.y, o.max.z]}
    v = desc.get_vertex_position(unreal.VertexID(0))
    out['first_vertex_cm'] = [v.x, v.y, v.z]


def main():
    root = Path(os.environ['S47_FF_ROOT'])
    path = Path(os.environ['S47_FF_REPORT'])
    report = {'status': 'RUNNING'}
    try:
        # (S47_FF_MATERIAL_ONLY=1: the colour maps and the material; the mesh and its normal and mask stand as they are)
        only = os.environ.get('S47_FF_MATERIAL_ONLY') == '1'
        if only:
            normal_map, mask_map = E.load_asset(DEST + '/T_FF_Range_N'), E.load_asset(DEST + '/T_FF_Range_M')
        else:
            # the normal is BC5: x and y, its z rebuilt; never below the horizon on this land
            normal_map = import_texture(root, 'T_FF_Range_N', False, unreal.TextureCompressionSettings.TC_NORMALMAP, unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
            mask_map = import_texture(root, 'T_FF_Range_M', False, unreal.TextureCompressionSettings.TC_MASKS, unreal.TextureGroup.TEXTUREGROUP_WORLD)
        color_map = import_texture(root, 'T_FF_Range_C', False, unreal.TextureCompressionSettings.TC_BC7, unreal.TextureGroup.TEXTUREGROUP_WORLD)
        beds_map = import_texture(root, 'T_FF_Range_B', False, unreal.TextureCompressionSettings.TC_BC7, unreal.TextureGroup.TEXTUREGROUP_WORLD, wrap=True)
        report['textures'] = [normal_map.get_path_name(), mask_map.get_path_name(), color_map.get_path_name(), beds_map.get_path_name()]
        mat = make_material(normal_map, mask_map, color_map, beds_map, report)
        report['mesh'] = {}
        if not only:
            import_mesh(root, mat, report['mesh'])
        report['status'] = 'PASS_FREEFORM_RANGE'
    except Exception:
        report['status'] = 'FAIL'
        report['error'] = traceback.format_exc()
    path.write_text(json.dumps(report, indent=1, default=str) + '\n')


if os.environ.get('S47_FF_APPLY') == '1':
    main()
