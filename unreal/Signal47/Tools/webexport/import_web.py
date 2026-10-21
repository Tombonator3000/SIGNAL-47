# Imports the web game's exported scene (Tools/webexport/export.py) into /Game/Web:
#   /Game/Web/Tex/T<id>          textures, with three's colour space, wrapping and filtering
#   /Game/Web/Geo/geo_NNN/...    one static mesh "g<id>" per geometry, collision from the mesh itself
#   /Game/Web/Base/M_Web_*       master materials, one per kind (lit or unlit, opaque, masked,
#                                translucent or additive, one or two sided)
#   /Game/Web/Mat/MI_W<id>       one material instance per web material, with its values
# /Game/Web is ignored by Git (it is made from the web build and holds the bought models too).
# Run: UnrealEditor-Cmd Signal47.uproject -run=pythonscript -script=Tools/webexport/import_web.py -unattended -nullrhi
# with S47_WEB=<export folder> (default ~/scratch-bm/Signal47-web).
import json
import os
import struct
import zlib
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
SRC = os.environ.get('S47_WEB', os.path.expanduser('~/scratch-bm/Signal47-web'))
man = json.load(open(os.path.join(SRC, 'manifest.json')))
only = os.environ.get('S47_ONLY', 'tex,geo,mat').split(',')


def log(s):
    unreal.log_warning('S47WEB ' + s)


def task(filename, dest, name=None):
    t = unreal.AssetImportTask()
    t.set_editor_property('filename', filename)
    t.set_editor_property('destination_path', dest)
    if name:
        t.set_editor_property('destination_name', name)
    t.set_editor_property('replace_existing', True)
    t.set_editor_property('automated', True)
    t.set_editor_property('save', True)
    return t


# ---------- default textures (white, sRGB and linear; flat normal) ----------
def png(path, rgba, w=4, h=4):
    raw = b''.join(b'\x00' + bytes(rgba) * w for _ in range(h))
    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')
    open(path, 'wb').write(data)


base_dir = os.path.join(SRC, 'base')
os.makedirs(base_dir, exist_ok=True)
png(os.path.join(base_dir, 'T_White.png'), (255, 255, 255, 255))
png(os.path.join(base_dir, 'T_WhiteLinear.png'), (255, 255, 255, 255))
png(os.path.join(base_dir, 'T_FlatNormal.png'), (128, 128, 255, 255))
tools.import_asset_tasks([task(os.path.join(base_dir, n), '/Game/Web/Base') for n in ('T_White.png', 'T_WhiteLinear.png', 'T_FlatNormal.png')])
white = eal.load_asset('/Game/Web/Base/T_White')
white_lin = eal.load_asset('/Game/Web/Base/T_WhiteLinear')
white_lin.set_editor_property('srgb', False)
eal.save_loaded_asset(white_lin)
flat = eal.load_asset('/Game/Web/Base/T_FlatNormal')
flat.set_editor_property('srgb', False)
flat.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
eal.save_loaded_asset(flat)

# which textures are used how (colour, data, normal)
use = {}
for m in man['materials']:
    for k in ('map', 'emissiveMap'):
        if k in m:
            use.setdefault(m[k], set()).add('color')
    for k in ('alphaMap', 'roughnessMap', 'metalnessMap', 'aoMap', 'bumpMap'):
        if k in m:
            use.setdefault(m[k], set()).add('data')
    if 'normalMap' in m:
        use.setdefault(m['normalMap'], set()).add('normal')
    for v in (m.get('uniforms') or {}).values():
        if isinstance(v, dict) and 'tex' in v:
            use.setdefault(v['tex'], set()).add('color')

# ---------- textures ----------
if 'tex' in only:
    tasks = []
    for t in man['textures']:
        if t.get('missing'):
            continue
        tasks.append(task(os.path.join(SRC, 'tex', 't%03d.png' % t['id']), '/Game/Web/Tex', 'T%d' % t['id']))
    for k in range(0, len(tasks), 40):
        tools.import_asset_tasks(tasks[k:k + 40])
    addr = {'repeat': unreal.TextureAddress.TA_WRAP, 'clamp': unreal.TextureAddress.TA_CLAMP, 'mirror': unreal.TextureAddress.TA_MIRROR}
    n = 0
    for t in man['textures']:
        a = eal.load_asset('/Game/Web/Tex/T%d' % t['id'])
        if not a:
            continue
        u = use.get(t['id'], {'color'})
        if 'normal' in u:
            a.set_editor_property('srgb', False)
            a.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
        else:
            a.set_editor_property('srgb', bool(t['srgb']))
        a.set_editor_property('address_x', addr[t['wrapS']])
        a.set_editor_property('address_y', addr[t['wrapT']])
        if t.get('nearest'):
            a.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)
        eal.save_loaded_asset(a)
        n += 1
    log('textures %d' % n)

# ---------- geometry ----------
if 'geo' in only:
    # afresh: the scene's order can change between exports (the flora is laid out as it loads),
    # and a mesh left from an earlier export would stand under the wrong node
    if eal.does_directory_exist('/Game/Web/Geo'):
        eal.delete_directory('/Game/Web/Geo')
    for f in man['geoFiles']:
        tools.import_asset_tasks([task(os.path.join(SRC, 'geo', f['file']), '/Game/Web/Geo')])
    count = 0
    for a in eal.list_assets('/Game/Web/Geo', recursive=True):
        if '/StaticMeshes/' not in a:
            continue
        sm = eal.load_asset(a)
        if not isinstance(sm, unreal.StaticMesh):
            continue
        ns = sm.get_editor_property('nanite_settings')
        if ns.get_editor_property('enabled'):
            ns.set_editor_property('enabled', False)
            sm.set_editor_property('nanite_settings', ns)
        bs = sm.get_editor_property('body_setup')
        if bs:
            bs.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        eal.save_loaded_asset(sm)
        count += 1
    log('meshes %d' % count)

# ---------- master materials ----------
# The UV transform is three's texture matrix as two rows (a b c / d e f): uv' = (a u + b v + c, d u + e v + f).
UVX = 'return float2(dot(R0, float3(UV, 1.0)), dot(R1, float3(UV, 1.0)));'


def fresh(path):
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    folder, name = path.rsplit('/', 1)
    return tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())


FLOOD = """float3 p = float3(WP.x, WP.z, WP.y) * 0.01;
float3 n = normalize(float3(NW.x, NW.z, NW.y));
float3 acc = 0;
float cnt = max(Count, 1.0);
[loop] for (int i = 0; i < 32; i++) {
  if (i >= (int)Count) break;
  float u = (i + 0.5) / cnt;
  float4 P = Tex.SampleLevel(TexSampler, float2(u, 0.25), 0);
  float3 C = Tex.SampleLevel(TexSampler, float2(u, 0.75), 0).rgb;
  if (P.w <= 0) continue;
  float3 L = P.xyz - p; float d2 = dot(L, L); L *= rsqrt(max(d2, 1e-4));
  float ndl = abs(dot(n, L)) * 0.75 + 0.25;
  float win = saturate(1.0 - sqrt(d2) / (P.w * 1.6 + 8.0));
  acc += C * (P.w * ndl * win * win / (1.0 + d2 * Fall));
}
return acc;"""


# three.js ACESFilmicToneMapping undone: a colour three draws without tone mapping comes out
# of AS47Web's tone mapping (M_WebTone, the same curve) as it is. K is exposure / 0.6.
UNTONE = """float3 y = saturate(C);
float3 x = max(float3(dot(float3(0.6430382, 0.3111868, 0.0457755), y), dot(float3(0.0592687, 0.9314365, 0.0092949), y), dot(float3(0.0059619, 0.063929, 0.9301184), y)), 0);
float3 A = 1.0 - 0.983729 * x, B = 0.0245786 - 0.432951 * x, Q = -(0.000090537 + 0.238081 * x);
float3 v = max((-B + sqrt(max(B * B - 4 * A * Q, 0))) / (2 * A), 0);
float3 c = float3(dot(float3(1.764741, -0.6757777, -0.0889633), v), dot(float3(-0.1470279, 1.1602515, -0.0132237), v), dot(float3(-0.0363368, -0.1624364, 1.1987733), v));
return max(c, 0) / max(K, 1e-4);"""

# The web version's picture (core/vhs.ts, the default on every tier but Low): the scene with its
# FogExp2, ACESFilmicToneMapping and sRGB per tap, then the 1986 tape over it: chroma smeared
# sideways, red and blue parting at the edges, a glow round bright things, the grade, faint
# scanlines and noise, on 720 lines. Vhs 0 gives the plain picture.
TONE = """struct W {
  float K, Dens; float3 FogCol;
  float3 sc(float2 vuv) {
    float2 b = ViewportUVToBufferUV(saturate(vuv));
    float3 c = max(SceneTextureLookup(b, 14, true).rgb, 0);
    c = (any(isnan(c)) ? 0 : min(c, 10.0));
    float d = SceneTextureLookup(b, 1, false).r * 0.01;
    float f = d > 1400.0 ? 0.0 : 1.0 - exp(-(Dens * d) * (Dens * d));
    c = lerp(c, FogCol, f) * K;
    float3 v = float3(dot(float3(0.59719, 0.35458, 0.04823), c), dot(float3(0.07600, 0.90834, 0.01566), c), dot(float3(0.02840, 0.13383, 0.83777), c));
    v = (v * (v + 0.0245786) - 0.000090537) / (v * (0.983729 * v + 0.4329510) + 0.238081);
    float3 o = saturate(float3(dot(float3(1.60475, -0.53108, -0.07367), v), dot(float3(-0.10208, 1.10813, -0.00605), v), dot(float3(-0.00327, -0.07276, 1.07602), v)));
    float3 lo = o * 12.92, hi = 1.055 * pow(max(o, 1e-6), 1.0 / 2.4) - 0.055;
    return lerp(hi, lo, step(o, 0.0031308));
  }
  float hash(float2 p) { float3 q = frac(float3(p.xyx) * 0.1031); q += dot(q, q.yzx + 33.33); return frac((q.x + q.y) * q.z); }
  float3 yiq(float3 c) { return float3(dot(float3(0.299, 0.587, 0.114), c), dot(float3(0.596, -0.274, -0.322), c), dot(float3(0.211, -0.523, 0.312), c)); }
  float3 rgb(float3 c) { return float3(c.x + 0.956 * c.y + 0.621 * c.z, c.x - 0.272 * c.y - 0.647 * c.z, c.x - 1.106 * c.y + 1.703 * c.z); }
};
W w; w.K = K; w.Dens = Dens; w.FogCol = FogCol;
if (Vhs < 0.5) return w.sc(UV);
float2 vs = View.ViewSizeAndInvSize.xy;
float2 size = float2(720.0 * vs.x / max(vs.y, 1.0), 720.0);
float2 uv = float2(UV.x, 1.0 - UV.y);
float px = 1.0 / size.x;
float ln = floor(uv.y * size.y);
float t = Time;
float wobble = sin(uv.y * 220.0 + t * 7.0) * 0.18 + (w.hash(float2(ln, floor(t * 24.0) * 7.0)) - 0.5) * 0.6;
uv.x += wobble * 0.15 * px;
float2 r = uv - 0.5;
float fringe = dot(r, r) * 0.0024;
float2 at = float2(uv.x, 1.0 - uv.y);
float3 q = w.yiq(w.sc(at));
float3 avg = 0;
for (int i = -3; i <= 3; i++) avg += w.yiq(w.sc(at + float2((i * 1.6 + 1.5) * px, 0)));
avg /= 7.0;
q.yz = avg.yz;
float3 col = w.rgb(q);
float2 rr = float2(r.x, -r.y) * fringe;
col.r = lerp(col.r, w.sc(at + rr).r, 0.6);
col.b = lerp(col.b, w.sc(at - rr).b, 0.6);
float3 glow = 0;
for (int j = 0; j < 8; j++) {
  float a = j * 0.785398;
  float3 g = w.sc(at + float2(cos(a), -sin(a)) * px * 5.0 * float2(1.0, size.x / size.y));
  glow += max(g - 0.8, 0.0);
}
col += glow * 0.25 * 0.5;
float l = dot(col, float3(0.299, 0.587, 0.114));
col = lerp(l.xxx, col, 0.88);
col = lerp(col, col * float3(0.86, 1.02, 1.04), smoothstep(0.45, 0.0, l));
col = lerp(col, col * float3(1.06, 1.0, 0.9), smoothstep(0.5, 1.0, l));
col = col * 0.95 + float3(0.026, 0.031, 0.033);
col *= 1.0 - 0.02 * (0.5 + 0.5 * cos(uv.y * size.y * 3.14159));
float n = w.hash(floor(uv * size) + float2(floor(t * 24.0) * 17.0, floor(t * 24.0) * 31.0));
col += (n - 0.5) * 0.06 * 0.5;
return saturate(col);"""

MPC_PATH = '/Game/Web/Base/MPC_Web'
mpc = eal.load_asset(MPC_PATH) if eal.does_asset_exist(MPC_PATH) else None
if 'mat' in only:
    if mpc is None:
        mpc = tools.create_asset('MPC_Web', '/Game/Web/Base', unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    # parameters added where missing; the ones there keep their ids, so nothing referring to them breaks
    sp = list(mpc.get_editor_property('scalar_parameters'))
    have = {str(p.get_editor_property('parameter_name')) for p in sp}
    for nm, v in (('ToneK', 1.05 / 0.6), ('FogDensity', 0.0021), ('Vhs', 1.0)):
        if nm in have:
            continue
        p = unreal.CollectionScalarParameter(); p.set_editor_property('parameter_name', nm); p.set_editor_property('default_value', v); sp.append(p)
    vp = list(mpc.get_editor_property('vector_parameters'))
    have = {str(p.get_editor_property('parameter_name')) for p in vp}
    for nm, v in (('HemiSky', (0.0144, 0.0266, 0.0704, 0)), ('HemiGround', (0.0208, 0.0099, 0.0047, 0)), ('FogColor', (0.01548, 0.02124, 0.03528, 0))):
        if nm in have:
            continue
        p = unreal.CollectionVectorParameter(); p.set_editor_property('parameter_name', nm); p.set_editor_property('default_value', unreal.LinearColor(*v)); vp.append(p)
    mpc.set_editor_property('scalar_parameters', sp)
    mpc.set_editor_property('vector_parameters', vp)
    eal.save_loaded_asset(mpc)


def build_master(name, lit, blend, two_sided):
    m = fresh('/Game/Web/Base/' + name)
    m.set_editor_property('two_sided', two_sided)
    m.set_editor_property('blend_mode', blend)
    if not lit:
        m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    if blend == unreal.BlendMode.BLEND_TRANSLUCENT:
        m.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING if lit else unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_NON_DIRECTIONAL)
    m.set_editor_property('used_with_instanced_static_meshes', True)
    y = [0]

    def node(cls, x=-1400):
        y[0] += 120
        return mel.create_material_expression(m, cls, x, y[0])

    def scalar(n, v):
        e = node(unreal.MaterialExpressionScalarParameter)
        e.set_editor_property('parameter_name', n)
        e.set_editor_property('default_value', v)
        return e

    def vector(n, v):
        e = node(unreal.MaterialExpressionVectorParameter)
        e.set_editor_property('parameter_name', n)
        e.set_editor_property('default_value', unreal.LinearColor(*v))
        return e

    def coll(n):
        e = node(unreal.MaterialExpressionCollectionParameter)
        e.set_editor_property('collection', mpc)
        e.set_editor_property('parameter_name', n)
        return e

    def texp(n, sampler, default, uvs):
        e = node(unreal.MaterialExpressionTextureSampleParameter2D, -900)
        e.set_editor_property('parameter_name', n)
        e.set_editor_property('sampler_type', sampler)
        e.set_editor_property('texture', default)
        mel.connect_material_expressions(uvs, '', e, 'UVs')
        return e

    def custom(code, inputs, out=unreal.CustomMaterialOutputType.CMOT_FLOAT2):
        e = node(unreal.MaterialExpressionCustom, -1150)
        e.set_editor_property('code', code)
        e.set_editor_property('output_type', out)
        ins = []
        for nm in inputs:
            ci = unreal.CustomInput()
            ci.set_editor_property('input_name', nm)
            ins.append(ci)
        e.set_editor_property('inputs', ins)
        return e

    def op(cls, a, b, ain='', bin_=''):
        e = node(cls, -500)
        mel.connect_material_expressions(a, ain, e, 'A')
        mel.connect_material_expressions(b, bin_, e, 'B')
        return e

    def lerp(a, b, alpha, ain='', bin_='', alin=''):
        e = node(unreal.MaterialExpressionLinearInterpolate, -350)
        mel.connect_material_expressions(a, ain, e, 'A')
        mel.connect_material_expressions(b, bin_, e, 'B')
        mel.connect_material_expressions(alpha, alin, e, 'Alpha')
        return e

    uv = node(unreal.MaterialExpressionTextureCoordinate)

    def xuv(prefix):
        c = custom(UVX, ['UV', 'R0', 'R1'])
        mel.connect_material_expressions(uv, '', c, 'UV')
        mel.connect_material_expressions(vector(prefix + 'U', (1, 0, 0, 0)), '', c, 'R0')
        mel.connect_material_expressions(vector(prefix + 'V', (0, 1, 0, 0)), '', c, 'R1')
        return c

    one = node(unreal.MaterialExpressionConstant)
    one.set_editor_property('r', 1.0)
    # base colour: Color * map (sRGB or linear) * vertex colour * instance colour
    muv = xuv('Map')
    map_c = texp('Map', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, white, muv)
    map_l = texp('MapLinear', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, white_lin, muv)
    map_rgb = lerp(map_c, map_l, scalar('MapIsLinear', 0), 'RGB', 'RGB')
    map_a = lerp(map_c, map_l, scalar('MapIsLinear', 0), 'A', 'A')
    use_map = scalar('UseMap', 0)
    tex_rgb = lerp(one, map_rgb, use_map)
    tex_a = lerp(one, map_a, use_map)
    color = vector('Color', (1, 1, 1, 1))
    vc = node(unreal.MaterialExpressionVertexColor)
    vcm = lerp(one, vc, scalar('UseVertexColor', 0), '', 'RGB')
    icd = custom('return float3(R, G, B);', ['R', 'G', 'B'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    for k, nm in enumerate(('R', 'G', 'B')):
        pc = node(unreal.MaterialExpressionPerInstanceCustomData)
        pc.set_editor_property('data_index', k)
        pc.set_editor_property('const_default_value', 1.0)
        mel.connect_material_expressions(pc, '', icd, nm)
    icm = lerp(one, icd, scalar('UseInstanceColor', 0))
    base = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, color, tex_rgb, 'RGB'), op(unreal.MaterialExpressionMultiply, vcm, icm))
    # alpha: Opacity * map alpha * alpha map (green, as three) * vertex alpha
    auv = xuv('AlphaMap')
    amap = texp('AlphaMapTex', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, white_lin, auv)
    am = lerp(one, amap, scalar('UseAlphaMap', 0), '', 'G')
    vam = lerp(one, vc, scalar('UseVertexColor', 0), '', 'A')
    alpha = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, scalar('Opacity', 1), tex_a), am), vam)
    if lit:
        mel.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
        rough = op(unreal.MaterialExpressionMultiply, scalar('Roughness', 1), lerp(one, texp('RoughnessMap', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, white_lin, xuv('Rough')), scalar('UseRoughnessMap', 0), '', 'G'))
        mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
        metal = op(unreal.MaterialExpressionMultiply, scalar('Metallic', 0), lerp(one, texp('MetalnessMap', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, white_lin, xuv('Metal')), scalar('UseMetalnessMap', 0), '', 'B'))
        mel.connect_material_property(metal, '', unreal.MaterialProperty.MP_METALLIC)
        euv = xuv('Em')
        emap_c = texp('EmissiveMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, white, euv)
        emis = op(unreal.MaterialExpressionMultiply, vector('Emissive', (0, 0, 0, 1)), lerp(one, emap_c, scalar('UseEmissiveMap', 0), '', 'RGB'))
        # kit.ts floodlit: the fake floodlight pools, added as light the surface gives off
        fc = custom(FLOOD, ['WP', 'NW', 'Tex', 'Count', 'Fall'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        mel.connect_material_expressions(node(unreal.MaterialExpressionWorldPosition), '', fc, 'WP')
        mel.connect_material_expressions(node(unreal.MaterialExpressionVertexNormalWS), '', fc, 'NW')
        ft = node(unreal.MaterialExpressionTextureObjectParameter)
        ft.set_editor_property('parameter_name', 'FloodTex')
        ft.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        ft.set_editor_property('texture', white_lin)
        mel.connect_material_expressions(ft, '', fc, 'Tex')
        mel.connect_material_expressions(scalar('FloodCount', 0), '', fc, 'Count')
        mel.connect_material_expressions(scalar('FloodFall', 0.012), '', fc, 'Fall')
        fl = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, fc, base), scalar('FloodScale', 0))
        emis = op(unreal.MaterialExpressionAdd, emis, fl)
        # HemisphereLight: mix(ground, sky, n.y / 2 + 1/2) * diffuse / pi, as three's indirect diffuse
        hc = custom('float k = saturate(N.z * 0.5 + 0.5); return lerp(G, S, k) * B * (1 - saturate(M)) / 3.14159265;', ['N', 'G', 'S', 'B', 'M'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        mel.connect_material_expressions(node(unreal.MaterialExpressionVertexNormalWS), '', hc, 'N')
        mel.connect_material_expressions(coll('HemiGround'), '', hc, 'G')
        mel.connect_material_expressions(coll('HemiSky'), '', hc, 'S')
        mel.connect_material_expressions(base, '', hc, 'B')
        mel.connect_material_expressions(metal, '', hc, 'M')
        emis = op(unreal.MaterialExpressionAdd, emis, hc)
        mel.connect_material_property(emis, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        nmap = texp('NormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, flat, xuv('Normal'))
        mel.connect_material_property(nmap, 'RGB', unreal.MaterialProperty.MP_NORMAL)
    else:
        boost = op(unreal.MaterialExpressionMultiply, base, scalar('Brightness', 1))
        # toneMapped: false in three: undo the tone mapping that follows
        un = custom(UNTONE, ['C', 'K'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        mel.connect_material_expressions(boost, '', un, 'C')
        mel.connect_material_expressions(coll('ToneK'), '', un, 'K')
        # (the tape, core/vhs.ts, tone maps the whole picture, these too: only without it)
        unk = custom('return U * (1 - V);', ['U', 'V'], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
        mel.connect_material_expressions(scalar('Untonemapped', 0), '', unk, 'U')
        mel.connect_material_expressions(coll('Vhs'), '', unk, 'V')
        boost = lerp(boost, un, unk)
        mel.connect_material_property(boost, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if blend == unreal.BlendMode.BLEND_MASKED:
        mel.connect_material_property(alpha, '', unreal.MaterialProperty.MP_OPACITY_MASK)
    elif blend in (unreal.BlendMode.BLEND_TRANSLUCENT, unreal.BlendMode.BLEND_ADDITIVE):
        mel.connect_material_property(alpha, '', unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


BLENDS = {'Opaque': unreal.BlendMode.BLEND_OPAQUE, 'Masked': unreal.BlendMode.BLEND_MASKED, 'Trans': unreal.BlendMode.BLEND_TRANSLUCENT, 'Add': unreal.BlendMode.BLEND_ADDITIVE}
masters = {}
if 'mat' in only:
    for lit in (True, False):
        for bk, bv in BLENDS.items():
            if lit and bk == 'Add':
                continue
            for two in (False, True):
                nm = 'M_Web_%s_%s%s' % ('Lit' if lit else 'Unlit', bk, '_2S' if two else '')
                masters[(lit, bk, two)] = build_master(nm, lit, bv, two)
    log('masters %d' % len(masters))

    # ---------- one instance per web material ----------
    tex_srgb = {t['id']: t['srgb'] for t in man['textures']}

    def T(i):
        return eal.load_asset('/Game/Web/Tex/T%d' % i) if i is not None and i >= 0 else None

    def rows(uvr):
        return unreal.LinearColor(uvr[0], uvr[1], uvr[2], 0), unreal.LinearColor(uvr[3], uvr[4], uvr[5], 0)

    made = 0
    for d in man['materials']:
        lit = bool(d.get('lit'))
        uses_alpha = 'alphaMap' in d or d.get('opacity', 1) < 0.999
        if d.get('additive'):
            bk, lit = 'Add', False
        elif d.get('transparent') and (uses_alpha or 'map' in d or d.get('vertexColors')):
            bk = 'Trans'
        elif d.get('alphaTest', 0) > 0:
            bk = 'Masked'
        else:
            bk = 'Opaque'
        two = d.get('side') != 'front'
        parent = masters[(lit, bk, two)]
        path = '/Game/Web/Mat/MI_W%d' % d['id']
        if eal.does_asset_exist(path):
            eal.delete_asset(path)
        mi = tools.create_asset('MI_W%d' % d['id'], '/Game/Web/Mat', unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property('parent', parent)
        c = d.get('color', [1, 1, 1])
        sv = mel.set_material_instance_scalar_parameter_value
        vv = mel.set_material_instance_vector_parameter_value
        tv = mel.set_material_instance_texture_parameter_value
        vv(mi, 'Color', unreal.LinearColor(c[0], c[1], c[2], 1))
        sv(mi, 'Opacity', float(d.get('opacity', 1)))
        if d.get('vertexColors'):
            sv(mi, 'UseVertexColor', 1.0)
        if 'map' in d and T(d['map']):
            sv(mi, 'UseMap', 1.0)
            if tex_srgb.get(d['map'], True):
                tv(mi, 'Map', T(d['map']))
            else:
                tv(mi, 'MapLinear', T(d['map']))
                sv(mi, 'MapIsLinear', 1.0)
            r0, r1 = rows(d['mapUV'])
            vv(mi, 'MapU', r0); vv(mi, 'MapV', r1)
        if 'alphaMap' in d and T(d['alphaMap']):
            sv(mi, 'UseAlphaMap', 1.0)
            tv(mi, 'AlphaMapTex', T(d['alphaMap']))
            r0, r1 = rows(d['alphaMapUV'])
            vv(mi, 'AlphaMapU', r0); vv(mi, 'AlphaMapV', r1)
        if lit:
            sv(mi, 'Roughness', float(d.get('roughness', 1)))
            sv(mi, 'Metallic', float(d.get('metalness', 0)))
            e = d.get('emissive', [0, 0, 0]); k = float(d.get('emissiveIntensity', 1))
            vv(mi, 'Emissive', unreal.LinearColor(e[0] * k, e[1] * k, e[2] * k, 1))
            if 'emissiveMap' in d and T(d['emissiveMap']):
                sv(mi, 'UseEmissiveMap', 1.0)
                tv(mi, 'EmissiveMap', T(d['emissiveMap']))
                r0, r1 = rows(d['emissiveMapUV'])
                vv(mi, 'EmU', r0); vv(mi, 'EmV', r1)
            if 'roughnessMap' in d and T(d['roughnessMap']):
                sv(mi, 'UseRoughnessMap', 1.0); tv(mi, 'RoughnessMap', T(d['roughnessMap']))
                r0, r1 = rows(d['roughnessMapUV']); vv(mi, 'RoughU', r0); vv(mi, 'RoughV', r1)
            if 'metalnessMap' in d and T(d['metalnessMap']):
                sv(mi, 'UseMetalnessMap', 1.0); tv(mi, 'MetalnessMap', T(d['metalnessMap']))
                r0, r1 = rows(d['metalnessMapUV']); vv(mi, 'MetalU', r0); vv(mi, 'MetalV', r1)
            if 'normalMap' in d and T(d['normalMap']):
                tv(mi, 'NormalMap', T(d['normalMap']))
                r0, r1 = rows(d['normalMapUV']); vv(mi, 'NormalU', r0); vv(mi, 'NormalV', r1)
        if not lit and d.get('toneMapped') is False:
            sv(mi, 'Untonemapped', 1.0)
        if bk == 'Masked':
            ov = mi.get_editor_property('base_property_overrides')
            ov.set_editor_property('override_opacity_mask_clip_value', True)
            ov.set_editor_property('opacity_mask_clip_value', float(d.get('alphaTest', 0.5)))
            mi.set_editor_property('base_property_overrides', ov)
        if d.get('shader'):
            # a shader material: its colour-like uniforms give the nearest plain look
            u = d.get('uniforms') or {}
            for key in ('uColor', 'color', 'uCol', 'uTint', 'tint'):
                if isinstance(u.get(key), list) and len(u[key]) == 3:
                    vv(mi, 'Color', unreal.LinearColor(u[key][0], u[key][1], u[key][2], 1))
                    break
            for key in ('uMap', 'map', 'tDiffuse', 'uTex'):
                if isinstance(u.get(key), dict) and T(u[key]['tex']):
                    sv(mi, 'UseMap', 1.0); tv(mi, 'Map', T(u[key]['tex']))
                    break
        mel.update_material_instance(mi)
        eal.save_loaded_asset(mi)
        made += 1
    log('material instances %d' % made)
# ---------- the sky pictures and the two special materials ----------
if 'sky' in only or 'mat' in only:
    import shutil
    tasks = [task(os.path.join(SRC, 'tex', v['file']), '/Game/Web/Tex', 'Sky_' + k) for k, v in (man.get('skies') or {}).items()]
    if tasks:
        tools.import_asset_tasks(tasks)
    for k in (man.get('skies') or {}):
        a = eal.load_asset('/Game/Web/Tex/Sky_' + k)
        if a:
            a.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            a.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_SKYBOX)
            a.set_editor_property('srgb', True)
            a.set_editor_property('address_x', unreal.TextureAddress.TA_WRAP)
            a.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
            eal.save_loaded_asset(a)
            log('sky %s %dx%d' % (k, a.blueprint_get_size_x(), a.blueprint_get_size_y()))
    # the sky: the equirectangular picture by view direction (longitude from north, to the east)
    m = fresh('/Game/Web/Base/M_WebSky')
    m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property('two_sided', True)
    try:
        m.set_editor_property('is_sky', True)
    except Exception:
        pass
    cam = mel.create_material_expression(m, unreal.MaterialExpressionCameraVectorWS, -900, 0)
    c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -650, 0)
    c.set_editor_property('code', 'float3 d = -normalize(V); float3 t = float3(d.x, d.z, d.y); float lon = atan2(t.x, -t.z); if (lon < 0) lon += 6.2831853; float lat = asin(clamp(t.y, -1.0, 1.0)); return float2(lon / 6.2831853, 0.5 - lat / 3.14159265);')
    c.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    ci = unreal.CustomInput(); ci.set_editor_property('input_name', 'V'); c.set_editor_property('inputs', [ci])
    mel.connect_material_expressions(cam, '', c, 'V')
    sk = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -400, 0)
    sk.set_editor_property('parameter_name', 'Sky')
    sk.set_editor_property('texture', white)
    mel.connect_material_expressions(c, '', sk, 'UVs')
    br = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -400, 200)
    br.set_editor_property('parameter_name', 'Brightness'); br.set_editor_property('default_value', 1.0)
    mu = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -200, 50)
    mel.connect_material_expressions(sk, 'RGB', mu, 'A'); mel.connect_material_expressions(br, '', mu, 'B')
    us = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -100, 50)
    us.set_editor_property('code', UNTONE); us.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ins = []
    for nm in ('C', 'K'):
        ci = unreal.CustomInput(); ci.set_editor_property('input_name', nm); ins.append(ci)
    us.set_editor_property('inputs', ins)
    kk = mel.create_material_expression(m, unreal.MaterialExpressionCollectionParameter, -300, 250)
    kk.set_editor_property('collection', mpc); kk.set_editor_property('parameter_name', 'ToneK')
    mel.connect_material_expressions(mu, '', us, 'C'); mel.connect_material_expressions(kk, '', us, 'K')
    # the dome is tone mapped on screen, the stars only under the tape; with the tape (the default)
    # the whole sky goes through the tone curve, so the picture is light as it is
    vh = mel.create_material_expression(m, unreal.MaterialExpressionCollectionParameter, -300, 400)
    vh.set_editor_property('collection', mpc); vh.set_editor_property('parameter_name', 'Vhs')
    sl = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, 0, 50)
    mel.connect_material_expressions(us, '', sl, 'A'); mel.connect_material_expressions(mu, '', sl, 'B'); mel.connect_material_expressions(vh, '', sl, 'Alpha')
    mel.connect_material_property(sl, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m); eal.save_loaded_asset(m)
    # a glow point: additive, a soft round spot, the point's colour
    g = fresh('/Game/Web/Base/M_WebGlow')
    g.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    g.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    g.set_editor_property('two_sided', True)
    uvg = mel.create_material_expression(g, unreal.MaterialExpressionTextureCoordinate, -900, 0)
    cg = mel.create_material_expression(g, unreal.MaterialExpressionCustom, -650, 0)
    # textures.ts glowSprite: alpha 1 at the centre, 0.55 a quarter out, 0 at the rim
    cg.set_editor_property('code', 'float r = saturate(length(UV - 0.5) * 2.0); return r < 0.25 ? 1.0 - 1.8 * r : 0.55 * (1.0 - r) / 0.75;')
    cg.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    ci = unreal.CustomInput(); ci.set_editor_property('input_name', 'UV'); cg.set_editor_property('inputs', [ci])
    mel.connect_material_expressions(uvg, '', cg, 'UV')
    colg = mel.create_material_expression(g, unreal.MaterialExpressionVectorParameter, -650, 200)
    colg.set_editor_property('parameter_name', 'Color'); colg.set_editor_property('default_value', unreal.LinearColor(1, 0.7, 0.4, 1))
    brg = mel.create_material_expression(g, unreal.MaterialExpressionScalarParameter, -650, 350)
    brg.set_editor_property('parameter_name', 'Brightness'); brg.set_editor_property('default_value', 1.0)
    m1 = mel.create_material_expression(g, unreal.MaterialExpressionMultiply, -400, 100)
    mel.connect_material_expressions(cg, '', m1, 'A'); mel.connect_material_expressions(colg, '', m1, 'B')
    m2 = mel.create_material_expression(g, unreal.MaterialExpressionMultiply, -250, 100)
    mel.connect_material_expressions(m1, '', m2, 'A'); mel.connect_material_expressions(brg, '', m2, 'B')
    mel.connect_material_property(m2, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(g); eal.save_loaded_asset(g)
    t = fresh('/Game/Web/Base/M_WebTone')
    t.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)
    t.set_editor_property('blendable_location', unreal.BlendableLocation.BL_REPLACING_TONEMAPPER)
    tc = mel.create_material_expression(t, unreal.MaterialExpressionCustom, -400, 0)
    tc.set_editor_property('code', TONE); tc.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ins = []
    for nm in ('UV', 'K', 'Dens', 'FogCol', 'Time', 'Vhs', 'Dummy'):
        ci = unreal.CustomInput(); ci.set_editor_property('input_name', nm); ins.append(ci)
    tc.set_editor_property('inputs', ins)
    sp = mel.create_material_expression(t, unreal.MaterialExpressionScreenPosition, -800, 0)
    mel.connect_material_expressions(sp, 'ViewportUV', tc, 'UV')
    tm = mel.create_material_expression(t, unreal.MaterialExpressionTime, -800, 150)
    mel.connect_material_expressions(tm, '', tc, 'Time')
    # a scene texture node, so the material reads the scene (SceneTextureLookup needs it)
    st = mel.create_material_expression(t, unreal.MaterialExpressionSceneTexture, -800, 250)
    st.set_editor_property('scene_texture_id', unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    sd = mel.create_material_expression(t, unreal.MaterialExpressionSceneTexture, -800, 300)
    sd.set_editor_property('scene_texture_id', unreal.SceneTextureId.PPI_SCENE_DEPTH)
    dm = mel.create_material_expression(t, unreal.MaterialExpressionAdd, -600, 280)
    mel.connect_material_expressions(st, 'Color', dm, 'A'); mel.connect_material_expressions(sd, 'Color', dm, 'B')
    mel.connect_material_expressions(dm, '', tc, 'Dummy')
    for k, (nm, pin) in enumerate((('ToneK', 'K'), ('FogDensity', 'Dens'), ('FogColor', 'FogCol'), ('Vhs', 'Vhs'))):
        e = mel.create_material_expression(t, unreal.MaterialExpressionCollectionParameter, -800, 400 + k * 150)
        e.set_editor_property('collection', mpc); e.set_editor_property('parameter_name', nm)
        mel.connect_material_expressions(e, '', tc, pin)
    mel.connect_material_property(tc, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(t); eal.save_loaded_asset(t)
    log('sky, glow and tone materials')
    # the manifest goes with the build as a plain file
    dst = os.path.join(unreal.Paths.project_content_dir(), 'WebData')
    os.makedirs(dst, exist_ok=True)
    shutil.copy(os.path.join(SRC, 'manifest.json'), os.path.join(dst, 'manifest.json'))
    log('manifest copied')
log('done')
