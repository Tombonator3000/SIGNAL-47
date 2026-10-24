# Imports the web game's exported scene (Tools/webexport/export.py) into /Game/Web:
#   /Game/Web/Tex/T<id>          textures, with three's colour space, wrapping and filtering
#   /Game/Web/Geo/geo_NNN/...    one static mesh "g<id>" per geometry, collision from the mesh itself
#   /Game/Web/Base/M_Web_*       master materials, one per kind (lit or unlit, opaque, masked,
#                                translucent or additive, one or two sided)
#   /Game/Web/Mat/MI_W<id>       one material instance per web material, with its values
# /Game/Web is ignored by Git (it is made from the web build and holds the bought models too).
# Run: UnrealEditor-Cmd Signal47.uproject -run=pythonscript -script=Tools/webexport/import_web.py -unattended -nullrhi
# with S47_WEB=<export folder> (default ~/scratch-bm/Signal47-web).
# S47_ONLY=materials rebuilds Base and Mat only, without touching shared Tex or WebData.
# S47_ONLY=masters updates Base graphs in place, retaining all material instances and parents.
# S47_ONLY=picture updates only MPC_Web and M_WebTone; scene materials/assets stay in place.
import json
import os
import sys
import shutil
import struct
import zlib
import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from surface_profiles import PROFILES, material_labels, texture_labels, profile_for, glass_parameters, is_control_crt, write_detail_tile

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
SRC = os.environ.get('S47_WEB', os.path.expanduser('~/scratch-bm/Signal47-web'))
man = json.load(open(os.path.join(SRC, 'manifest.json')))
only = os.environ.get('S47_ONLY', 'tex,geo,mat').split(',')
masters_only = only == ['masters']
materials_only = masters_only or only == ['materials']
if materials_only:
    only = ['mat']


def log(s):
    unreal.log_warning('S47WEB ' + s)


# ---------- ui/Board.ts art: loose pictures loaded by S47Image::Art ----------
# Keep the web filenames under a board_ prefix so they cannot collide with core/art.ts names.
# Resolve from this checkout, not the user's main web working copy.
def copy_board_art():
    source = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '../../web/src/assets/art/board'))
    dest = os.path.join(unreal.Paths.project_content_dir(), 'WebData', 'art')
    os.makedirs(dest, exist_ok=True)
    count = 0
    if os.path.isdir(source):
        for name in sorted(os.listdir(source)):
            stem, ext = os.path.splitext(name)
            if ext.lower() not in ('.png', '.jpg', '.webp'):
                continue
            src = os.path.join(source, name)
            dst = os.path.join(dest, 'board_' + stem + ('.png' if ext.lower() == '.webp' else ext.lower()))
            if ext.lower() == '.webp':
                from PIL import Image
                with Image.open(src) as image:
                    image.save(dst, format='PNG')
            else:
                shutil.copyfile(src, dst)
            count += 1
    log('board art copied %d' % count)


if not materials_only and only != ['picture']:
    copy_board_art()


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


if only != ['picture']:
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

    # One authored, tileable data texture.  The opt-in static switch removes both
    # samples on paper, people, flora, emissive and already mapped PBR surfaces.
    write_detail_tile(os.path.join(base_dir, 'T_SurfaceDetail.png'))
    tools.import_asset_tasks([task(os.path.join(base_dir, 'T_SurfaceDetail.png'), '/Game/Web/Base')])
    surface_detail = eal.load_asset('/Game/Web/Base/T_SurfaceDetail')
    surface_detail.set_editor_property('srgb', False)
    surface_detail.set_editor_property('address_x', unreal.TextureAddress.TA_WRAP)
    surface_detail.set_editor_property('address_y', unreal.TextureAddress.TA_WRAP)
    eal.save_loaded_asset(surface_detail)

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
            a.set_editor_property('srgb', bool(t['srgb']) and not ('data' in u and 'color' not in u))
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
        if masters_only or only == ['picture']:
            # Preserve the UMaterial object: deleting it would sever existing MI parents.
            material = eal.load_asset(path)
            if not isinstance(material, unreal.Material):
                raise RuntimeError('S47WEB expected material at ' + path)
            mel.delete_all_material_expressions(material)
            return material
        eal.delete_asset(path)
    folder, name = path.rsplit('/', 1)
    return tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())


FLOOD = """if (Native > 0.5) return 0;
float3 p = float3(WP.x, WP.z, WP.y) * 0.01;
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

# world/flora.ts floraMaterial (back): a lamp behind the blades, the camera looking towards it
# through them, shows through their edges, the tips most (the floods' term with fBk fb^3 added)
FLOOD_FLORA = FLOOD.replace('float3 acc = 0;', """float3 cam = float3(Cam.x, Cam.z, Cam.y) * 0.01;
float3 fV = normalize(p - cam); float fBk = Back * 0.6 * (0.3 + 0.7 * Sway);
float3 acc = 0;""").replace('float ndl = abs(dot(n, L)) * 0.75 + 0.25;', """float fb = max(dot(fV, L), 0.0);
  float ndl = abs(dot(n, L)) * 0.75 + 0.25 + fBk * fb * fb * fb;""")

# world/flora.ts floraMaterial's vertex shader, as a world position offset: the copy shrinks into the
# ground with distance (the small ones much nearer), one of a succulent's two plants folds away
# (Mirror: the copy is mirrored, a cholla), and the top sways in the wind (wind.ts, the MPC's
# WindTime and WindAmp; WindDir the way it blows in Unreal's x, y, turned with the patch). Reach is
# this kind's (Low draws it nearer). IO and IX: the copy's origin and its x axis in the world.
FLORA_WPO = """float isc = length(IX);
float3 iw = float3(IO.x, IO.z, IO.y) * 0.01, cw = float3(Cam.x, Cam.z, Cam.y) * 0.01;
float reach = Reach * clamp(0.05 + 0.75 * isc, 0.3, 1.0);
float fade = 1.0 - smoothstep(reach * 0.72, reach, distance(iw.xz, cw.xz));
fade *= Parts > 0.5 ? step(abs(SP.y - Mirror), 0.5) : 1.0;
float ph = WindT * 1.9 + dot(iw.xz, float2(0.21, 0.17));
float gust = sin(ph) * 0.55 + sin(ph * 2.7 + iw.x * 0.5) * 0.25 + 0.35;
float3 wind = float3(WindDir.x, WindDir.y, 0.0) * (SP.x * WindA * gust * isc * fade * 100.0);
return (WP - IO) * (fade - 1.0) + wind;"""

# three.js flatShading (the stones): the face's own normal, from the position's derivatives, towards the eye
FLAT = """float3 n = normalize(cross(ddy(WP), ddx(WP)));
return dot(n, V) < 0 ? -n : n;"""

# World-plane projection keeps the grain's physical size on stretched export UVs.
# Finite slopes are baked in RG; no derivatives, texture loops or procedural noise
# are needed in the pixel/ray-hit shader.  Only the opted-in static permutation
# evaluates the two samples (micro-surface and broad independent macro variation).
SURFACE_UV = """float3 a = abs(N);
float2 p = a.z >= max(a.x, a.y) ? WP.xy : (a.x >= a.y ? WP.yz : WP.xz);
return p / max(Size, 1.0);"""
SURFACE_NORMAL = """float3 n = normalize(N);
float2 slope = (RG.xy - 0.5) * 2.0 * Strength;
float3 a = abs(n);
float3 gradient = a.z >= max(a.x, a.y) ? float3(slope, 0.0) :
  (a.x >= a.y ? float3(0.0, slope) : float3(slope.x, 0.0, slope.y));
gradient -= n * dot(n, gradient);
return normalize(n - gradient);"""
BUMP_NORMAL = """// Bump maps are height, unlike normal maps.  WP and height are centimetres.
#if PIXELSHADER
float3 n = normalize(N), dx = ddx(WP), dy = ddy(WP);
float3 rx = cross(dy, n), ry = cross(n, dx);
float determinant = dot(dx, rx);
float3 gradient = sign(determinant) * (ddx(H) * rx + ddy(H) * ry) * Scale;
return normalize(max(abs(determinant), 1e-10) * n - gradient);
#else
return normalize(N);
#endif
"""


# three.js ACESFilmicToneMapping undone: a colour three draws without tone mapping comes out
# of AS47Web's tone mapping (M_WebTone, the same curve) as it is. K is exposure / 0.6.
UNTONE = """if (all(C <= 0.0)) return float3(0, 0, 0);
float3 y = saturate(C);
float3 x = max(float3(dot(float3(0.6430382, 0.3111868, 0.0457755), y), dot(float3(0.0592687, 0.9314365, 0.0092949), y), dot(float3(0.0059619, 0.063929, 0.9301184), y)), 0);
float3 A = 1.0 - 0.983729 * x, B = 0.0245786 - 0.432951 * x, Q = -(0.000090537 + 0.238081 * x);
float3 v = max((-B + sqrt(max(B * B - 4 * A * Q, 0))) / (2 * A), 0);
float3 c = float3(dot(float3(1.764741, -0.6757777, -0.0889633), v), dot(float3(-0.1470279, 1.1602515, -0.0132237), v), dot(float3(-0.0363368, -0.1624364, 1.1987733), v));
return max(c, 0) / max(K, 1e-4);"""

# Kept as a separate source file so every look uses exactly the same scene/ACES curve.
with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'film_tone.hlsl'), encoding='utf-8') as tone_file:
    TONE = tone_file.read()

# ---------- the world's own life: the sky, the light in the air, the windows' light (S47WorldLife) ----------
# The sky group is drawn ten times as far off as three has it (S47Sky: beyond the farthest land, as
# three puts it at the far plane): the dome at 30 km. The tone pass's fog stops short of it.
SKY_CUT = 25000.0

# A direction of the view in the sky's own terms: V is the camera vector (towards the eye), Yaw how the
# sky group is turned in this area (World.skyYaw), three's axes.
SKY_DIR = """float3 u = -normalize(V);
float3 t = float3(u.x, u.z, u.y);
float cy = cos(Yaw), sy = sin(Yaw);
float3 d = float3(t.x * cy - t.z * sy, t.y, t.x * sy + t.z * cy);"""

# world/Sky.ts, the dome's fragment shader (its uniforms as parameters; uGlow as Glow0..2)
SKY_DOME = SKY_DIR + """
struct N {
  float hash(float3 p) { p = frac(p * 0.3183099 + 0.1); p *= 17.0; return frac(p.x * p.y * p.z * (p.x + p.y + p.z)); }
  float noise(float3 x) {
    float3 i = floor(x), f = frac(x); f = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(lerp(hash(i), hash(i + float3(1,0,0)), f.x), lerp(hash(i + float3(0,1,0)), hash(i + float3(1,1,0)), f.x), f.y),
                lerp(lerp(hash(i + float3(0,0,1)), hash(i + float3(1,0,1)), f.x), lerp(hash(i + float3(0,1,1)), hash(i + float3(1,1,1)), f.x), f.y), f.z);
  }
  float fbm(float3 p) { float a = 0.5, s = 0.0; for (int i = 0; i < 5; i++) { s += a * noise(p); p *= 2.07; a *= 0.5; } return s; }
};
N nz;
float3 bandN = float3(0.5146141883532817, -0.6086243722280211, -0.6039442115589323);   // (BAND_N and CORE as the export has them)
float3 core = float3(-0.25052665955947706, 0.42088478805992147, -0.8718327752669802);
float4 G[3] = { Glow0, Glow1, Glow2 };
float h = d.y;
float3 col = lerp(float3(0.030, 0.034, 0.060), float3(0.006, 0.009, 0.024), smoothstep(-0.02, 0.55, h));
float glow = 0.15;
float2 hd = normalize(d.xz + float2(1e-5, 0.0));
for (int i = 0; i < 3; i++) {
  float tt = acos(clamp(dot(hd, G[i].xy), -1.0, 1.0)) / G[i].w;
  glow += G[i].z * exp(-tt * tt) * 2.6;
}
col += float3(0.30, 0.15, 0.06) * exp(-max(h, 0.0) * 22.0) * 0.30 * glow;
float b = dot(d, bandN);
float band = exp(-b * b / 0.022);
float cr = exp(-b * b / 0.0045);
float along = dot(d, core);
float bulge = 0.35 + 1.4 * smoothstep(0.2, 1.0, along);
float c1 = nz.fbm(d * 3.5 + 2.0);
float c2 = nz.fbm(d * 11.0 - 4.0);
float dust = smoothstep(0.48, 0.72, nz.fbm(d * 6.0 + float3(7.0, 1.0, 3.0)));
float3 mwCol = lerp(float3(0.30, 0.27, 0.42), float3(0.62, 0.46, 0.40), c2 * bulge * 0.6);
float lum = (band * c1 * 0.65 + cr * c2 * 0.9) * bulge;
lum *= 1.0 - dust * cr * 0.9;
float hole = Hole * (1.0 - smoothstep(HoleR.x, HoleR.y, length(d - HoleDir)));
lum *= 1.0 - hole;
col += mwCol * lum * 0.20 * smoothstep(-0.02, 0.18, h);
float2 skyUV = float2(atan2(d.z, d.x) / 6.2831853 + 0.5, asin(clamp(d.y, -1.0, 1.0)) / 3.1415927 + 0.5);
float3 painted = Pano.SampleLevel(PanoSampler, skyUV, 0).rgb;
painted = lerp(painted, min(painted, HoleSky * 1.2), hole);
col = lerp(col, painted * 2.0, 0.72 * smoothstep(-0.03, 0.14, h));
float fa = max(dot(d, FlashDir), 0.0);
col += float3(0.55, 0.6, 0.85) * Flash * pow(fa, 18.0) * smoothstep(0.35, 0.0, h) * 1.4;
col += float3(0.25, 0.28, 0.4) * Flash * pow(fa, 4.0) * 0.12;
float sa = dot(normalize(float3(d.x, 0.0, d.z)), normalize(float3(SunDir.x, 0.0, SunDir.z)));
float east = 0.5 + 0.5 * sa;
float3 zenith = float3(0.055, 0.085, 0.17), rim = lerp(float3(0.20, 0.23, 0.30), float3(0.62, 0.42, 0.26), pow(east, 3.0));
float3 dawn = lerp(rim, zenith, smoothstep(-0.02, 0.45, h));
dawn += float3(0.55, 0.30, 0.12) * pow(east, 8.0) * exp(-max(h, 0.0) * 9.0) * 0.8;
col = lerp(col * (1.0 - 0.85 * smoothstep(0.1, 0.7, Dawn)), dawn, smoothstep(0.0, 1.0, Dawn) * 0.92);
float sd = dot(d, normalize(SunDir));
col += float3(1.0, 0.62, 0.30) * Sun * (pow(max(sd, 0.0), 60.0) * 0.9 + pow(max(sd, 0.0), 900.0) * 3.0);
col += float3(4.0, 3.2, 2.2) * Sun * smoothstep(0.99996, 0.99999, sd);
if (h < 0.0) col *= lerp(0.5, 1.0 - 0.5 * smoothstep(-0.015, -0.12, h), smoothstep(0.0, 1.0, Dawn));
return col;"""

# world/backdrop.ts, the painted horizon's fragment shader on its ring (U: the ring's u, the
# azimuth). The pictures carry their alpha premultiplied, as three uploads them (export.js writes them
# so). Returns the colour premultiplied and the cover (alpha composite).
EL_TOP, EL_BOT, FOOT = 8.4375, -2.8125, 0.32
SKY_BAND = SKY_DIR + """
float elTop = %(top).4f, elBot = %(bot).4f;
float el = degrees(asin(clamp(d.y, -1.0, 1.0))) + Dip;
float az = U * 360.0;
float2 uv = float2(U, clamp((el - elBot) / (elTop - elBot), 0.0, 1.0));
// Explicit LOD is also legal in the packaged Vulkan ray-hit permutations.
float4 n = Night.SampleLevel(NightSampler, uv, 0);
float ds = (az - 182.0 + 540.0) - 360.0 * floor((az - 182.0 + 540.0) / 360.0) - 180.0;
float sink = 3.50 * smoothstep(10.5, 13.5, abs(ds));
float2 uvS = float2((279.0 - ds * 1.15) / 360.0, clamp(((el + sink) / 0.62 - elBot) / (elTop - elBot), 0.0, 1.0));
float4 nS = Night.SampleLevel(NightSampler, uvS, 0);
float s = (1.0 - step(13.5, abs(ds))) * step(n.a + 0.002, nS.a);
float2 uvF = lerp(uv, uvS, s), gx = ddx(uv), gy = ddy(uv);
n = lerp(n, nS, s);
float4 dd = Day.SampleGrad(DaySampler, uvF, gx, gy);
float a0 = el < elBot ? 1.0 : n.a;
float w = clamp(fwidth(a0) * 0.7, 0.002, 0.5);
float a = smoothstep(0.5 - w, 0.5 + w, a0);
float dm = abs((az - 81.0 + 540.0) - 360.0 * floor((az - 81.0 + 540.0) / 360.0) - 180.0);
float top = 0.60 * (1.0 - smoothstep(9.0, 10.8, dm)) * (0.97 + 0.03 * sin(az * 2.7));
float ew = max(fwidth(el), 1e-4) * 0.7;
float sm = top > 0.0 ? 1.0 - smoothstep(top - ew, top + ew, el) : 0.0;
float onSun = sm * (1.0 - smoothstep(0.3, 0.7, a0));
a = max(a, sm);
float3 hz = normalize(float3(d.x, 0.0, d.z) + float3(1e-5, 0.0, 0.0));
float east = 0.5 + 0.5 * dot(hz, normalize(float3(SunDir.x, 0.0, SunDir.z) + float3(1e-5, 0.0, 0.0)));
float3 cn = n.rgb / max(n.a, 1e-4), cd = dd.rgb / max(dd.a, 1e-4);
float4 mm = Day.SampleGrad(DaySampler, uvF, gx * 11.3, gy * 11.3);
float3 mean = mm.rgb / max(mm.a, 1e-4), flt = lerp(mean, cd, 0.6);
float3 Lw = float3(0.2126, 0.7152, 0.0722);
float3 navy = lerp(dot(flt, Lw), dot(mean, Lw), 0.6) * float3(0.66, 0.8, 1.27);
float3 pre = lerp(flt, navy, 0.85 * (1.0 - east * east));
float above = Night.SampleGrad(NightSampler, uvF + float2(0.0, 2.0 * Texel), gx, gy).a;
pre += float3(0.05, 0.022, 0.026) * n.a * (1.0 - above) * smoothstep(0.5, 0.9, Dawn) * (1.0 - east);
float up = smoothstep(-0.002, 0.012, SunDir.y) * Sun;
float low = lerp(0.84 + 0.16 * smoothstep(%(foot).2f, 1.6, el), 1.0, up);
float3 col = lerp(cn, lerp(pre, cd, up), smoothstep(0.05, 0.75, Dawn)) * Gain * low;
col = lerp(col, Mesa, onSun);
float3 rim = lerp(float3(0.20, 0.23, 0.30), float3(0.62, 0.42, 0.26), pow(east, 3.0)) + float3(0.55, 0.30, 0.12) * pow(east, 8.0) * 0.8;
float3 air = lerp(Fog, lerp(Fog, rim * 0.92, smoothstep(0.0, 1.0, Dawn)), Rim);
col = lerp(col, air, BodyK * (1.0 - 0.85 * pow(east, 5.0) * smoothstep(0.1, 0.6, Dawn)));
float3 edge = lerp(Land, air, EdgeFog);
edge = max(edge, lerp(edge, rim * 0.62, pow(east, 8.0) * smoothstep(0.1, 0.8, Dawn) * smoothstep(-0.06, 0.0, el) * Rim));
col = lerp(col, edge, 1.0 - smoothstep(0.0, %(foot).2f + 2.0 * Dip, el));
float4 G[3] = { Glow0, Glow1, Glow2 };
float g = 0.0;
for (int i = 0; i < 3; i++) {
  float tt = acos(clamp(dot(hz.xz, G[i].xy), -1.0, 1.0)) / G[i].w;
  g += G[i].z * exp(-tt * tt) * exp(-max(el, 0.0) / 2.2);
}
col += float3(0.30, 0.15, 0.06) * g * 0.12;
return float4(col * a, a);""" % {'top': EL_TOP, 'bot': EL_BOT, 'foot': FOOT}

# world/Sky.ts makeStars: a star is a point a pixel or so across (gl_PointSize max(1, aSize uPR)); here
# a quad the copy's place, turned to the eye and sized in pixels (IO the copy's origin; Size its aSize)
STAR_WPO = """float px = max(1.0, Size * PR);
float D = length(IO - Cam);
float k = 2.0 * px * D * Tan / max(VSize, 1.0);
float2 q = UV - 0.5;
return IO + (q.x * Right - q.y * Up) * k - WP;"""
# its vertex shader's colour and the point's fragment shader (D its direction in the sky's terms,
# C its colour, Ph its phase)
STAR_COL = """float up = Dir.y;
float amp = lerp(0.3, 0.08, smoothstep(0.0, 0.35, up));
float tw = 1.0 - amp * (0.5 + 0.5 * sin(Time * (1.7 + frac(Ph) * 3.0) + Ph));
float horizon = smoothstep(0.0, 0.12, up);
float3 c = Col * tw * (0.3 + 0.7 * horizon);
c *= clamp(1.0 - Dawn * (2.2 - 1.2 * min(length(Col), 1.0)), 0.0, 1.0);
c *= 1.0 - Hole * (1.0 - smoothstep(HoleR.x, HoleR.y, length(normalize(Dir) - HoleDir)));
float2 q = UV - 0.5;
float a = Size > 1.2 ? exp(-dot(q, q) * 9.0) * 2.4 : 1.0;
return c * a;"""

# world/Sky.ts makeMeteor / meteorStep: the strip along a great circle at 2600 m, laid out from its
# corners' aT (along, 0 the tail) and aS (across) and the streak's start, way, head and tail
# (Scale: centimetres a sky metre is here)
METEOR_WPO = """float k = UV.x, sgn = UV.y;
float a = Back + (Head - Back) * k;
float3 md = Start * cos(a) + Way * sin(a);
float3 mt = Way * cos(a) - Start * sin(a);
float3 mw = normalize(cross(md, mt)) * (2600.0 * 0.0011 * (0.35 + 0.65 * k));
float3 p = md * 2600.0 + mw * sgn;
float cy = cos(Yaw), sy = sin(Yaw);
float3 w = float3(p.x * cy + p.z * sy, p.y, -p.x * sy + p.z * cy);
return Cam + float3(w.x, w.z, w.y) * Scale - WP;"""
METEOR_COL = """float t = UV.x, sv = UV.y;
float across = 1.0 - sv * sv;
float al = across * (pow(max(t, 0.0), 1.5) + 2.5 * smoothstep(0.86, 1.0, t));
float a = Back + (Head - Back) * t;
float3 d = normalize(Start * cos(a) + Way * sin(a));
float hole = Hole * (1.0 - smoothstep(HoleR.x, HoleR.y, length(d - HoleDir)));
return float3(0.86, 0.92, 1.0) * al * I * (1.0 - hole);"""

# Light added to the picture (a cone of lamplight, a window's light on the ground) fades with the
# scene's own fog by its own depth (three); the tone pass (M_WebTone) fogs the whole picture by what
# is behind it. This undoes the second: 1 / (1 - the fog behind). Behind it may be what three draws
# without fog (the custom depth and stencil, as M_WebTone reads them): none there, or the dawn's haze.
DEFOG = """float d = SD * 0.01;
float f = d > %.1f ? 0.0 : 1.0 - exp(-(Dens * d) * (Dens * d));
if (CS > 0.5 && abs(CD * 0.01 - d) <= max(0.5, d * 0.002)) f *= (CS < 1.5 ? HazeK : 0.0);
return 1.0 / max(1.0 - f, 1e-3);""" % SKY_CUT

# world/airlight.ts LightCones (CONE_VS, CONE_FS): VA the corner's place down the cone (aV) and its
# lamp (aLamp); K the lamps' strengths (uK) in a row of a small texture, Lamps how many
AIR_CONE = """if (Native > 0.5) return 0;
float3 vView = (Cam - WP) * 0.01;
float dist = length(vView);
float face = abs(dot(normalize(Nw), vView / max(dist, 1e-3)));
float along = smoothstep(0.0, 0.1, VA.x) * pow(max(1.0 - VA.x, 0.0), 1.8);
float near = smoothstep(0.8, 6.0, dist);
float k = K.SampleLevel(KSampler, float2((floor(VA.y + 0.5) + 0.5) / max(Lamps, 1.0), 0.5), 0).r;
float a = face * face * along * near * Gain;
float fd = Depth * 0.01;
a *= exp(-Dens * Dens * fd * fd);
return Col * k * a;"""

# world/airlight.ts GroundHaze (HAZE_FS): Layer the sheet (aLayer); the four nearest lamps
AIR_HAZE = """if (Native > 0.5) return 0;
float3 W = float3(WP.x, WP.z, WP.y) * 0.01, C = float3(Cam.x, Cam.z, Cam.y) * 0.01;
float3 d = W - C;
float dist = length(d);
float sinA = abs(d.y) / max(dist, 1e-3);
float a = 1.0 - exp(-0.012 / max(sinA, 0.03));
float2 q = W.xz * 0.045 + float2(Time * 0.011, Time * -0.007) + Layer * 3.1;
float n = 0.62 + 0.22 * sin(q.x * 1.7 + sin(q.y * 1.3)) * sin(q.y * 1.1 + 0.6 * sin(q.x * 0.9)) + 0.16 * sin(q.x * 3.1 - q.y * 2.3);
a *= n * (1.0 - smoothstep(R * 0.55, R, length(d.xz))) * smoothstep(5.0, 30.0, dist);
float3 col = Col;
float lit = 0.0;
float4 Lp[4] = { L0, L1, L2, L3 };
float3 Lc[4] = { C0, C1, C2, C3 };
for (int i = 0; i < 4; i++) {
  float2 l = W.xz - Lp[i].xz;
  float w = exp(-dot(l, l) / max(Lp[i].w, 1e-3));
  col += Lc[i] * w; lit += w * dot(Lc[i], float3(0.33, 0.33, 0.33));
}
a = a * K * (1.0 + 2.5 * min(lit, 1.0));
float fd = Depth * 0.01;
a *= exp(-Dens * Dens * fd * fd * 0.5);
return float4(col, clamp(a, 0.0, 0.12));"""

# world/windowLight.ts: the corner's colour (aCol) is a texel of Cols by its index (UV channel 2's
# y), read in the vertex shader; drawn a little nearer the eye than it lies (VS)
WIN_COL = """float W = max(ColsW, 1.0), H = max(ColsH, 1.0);
float i = floor(I + 0.5);
float2 at = float2((fmod(i, W) + 0.5) / W, (floor(i / W) + 0.5) / H);
return Cols.SampleLevel(ColsSampler, at, 0).rgb;"""
WIN_WPO = """float3 v = Cam - WP;
float l = length(v) * 0.01;
return v * min(0.12 / max(l, 0.5), 0.2);"""
# FS: the picture through the glass, blurred by the lamp's size seen from the ground
WIN_FS = """if (Native > 0.5) return 0;
float2 uv = Q0 / Q1.x;
float q = Q1.x;
float2 blur = float2(Q1.y, Q2.x);
float2 b = blur * max(q - 1.0, 0.0) / q;
float bs = min(b.x, b.y), bl = max(b.x, b.y);
float2 px = uv * MapSize.xy, dx = ddx(px), dy = ddy(px);
float lod = max(0.5 * log2(max(max(dot(dx, dx), dot(dy, dy)), 1e-8)), log2(max(max(bs, 0.33 * bl), 1.0)));
float2 e = (b - bs) / (3.0 * MapSize.xy);
float3 c = (Map.SampleLevel(MapSampler, uv - e, lod).rgb + Map.SampleLevel(MapSampler, uv, lod).rgb + Map.SampleLevel(MapSampler, uv + e, lod).rgb) * (VCol / 3.0);
float fd = Depth * 0.01;
c *= exp(-Dens * Dens * fd * fd);
return c;"""

# world/nightlife.ts: a point of the moths' and bats' cloud, or of the dust in a beam, is a quad here
# turned to the eye and sized in the picture's pixels (gl_PointSize): the vertex shader's work in
# three's terms (Cam, Right, Up, Fwd: the camera's place and axes; IO the copy's origin; Lines the
# picture's lines, 720 under the tape), the point's corner from the quad's UV (gl_PointCoord).
# Which value of the vertex shader a node gives: Out 0 the world position offset, 1 (colour, alpha),
# 2 (kind, flap, direction), 3 (the streak's size, length, half width).
VIEW3 = """float3 cam = float3(Cam.x, Cam.z, Cam.y) * 0.01, right = float3(Right.x, Right.z, Right.y), up = float3(Up.x, Up.z, Up.y), fwd = float3(Fwd.x, Fwd.z, Fwd.y);
float px = Lines / max(2.0 * Tan, 1e-4);"""
MOTH_VS = VIEW3 + """
struct MM {
  float3 c, wall3; float4 seed, wall; float kind;
  float3 at(float t) {
    float porch = wall.w, wide = step(0.86, seed.y) * step(kind, 2.5);
    float tt = t * (0.8 + 0.5 * seed.x);
    float r0 = lerp(0.12 + 0.38 * seed.y * seed.y, 0.6 + 0.8 * seed.z, wide) * (kind > 2.5 ? 0.4 : 1.0);
    float r = r0 * (0.75 + 0.25 * sin(tt * 1.3 + seed.z * 6.28));
    float a1 = tt * (4.0 + 4.0 * seed.z) / (0.5 + 2.5 * r0) * (seed.w < 0.5 ? 1.0 : -1.0) + seed.w * 6.28;
    float y = lerp(-0.16 + 0.14 * sin(tt * 2.1 + seed.x * 9.0), 0.1 + 0.12 * sin(tt * 2.3 + seed.x * 9.0), porch) * (1.0 + 2.0 * wide);
    float3 o = float3(cos(a1) * r, y + 0.06 * sin(tt * 7.3 + seed.y * 7.0), sin(a1 * 1.17 + seed.y * 3.0) * r);
    o += 0.04 * float3(sin(tt * 17.0 + seed.w * 30.0), sin(tt * 19.0 + seed.z * 20.0), cos(tt * 15.0 + seed.x * 25.0));
    float on = dot(o, wall3);
    o += wall3 * (abs(on) - on + 0.05 * porch);
    return c + o;
  }
};
MM mm; mm.c = float3(IO.x, IO.z, IO.y) * 0.01; mm.seed = Seed; mm.wall = Wall; mm.wall3 = Wall.xyz; mm.kind = Kind;
float3 c = mm.c, p = c;
float size = 0.0, a = 0.0, len = 0.0, vFlap = 0.0, vKind = 0.0;
float2 dir = float2(1.0, 0.0);
float3 vCol = 0, vS = 0;
float3 d0 = p - cam, m = float3(dot(d0, right), dot(d0, up), -dot(d0, fwd));
if (Kind < 0.5 || Kind > 2.5) {
  p = mm.at(Time);
  d0 = p - cam; m = float3(dot(d0, right), dot(d0, up), -dot(d0, fwd));
  float3 q0 = mm.at(Time - Shut) - cam, mq = float3(dot(q0, right), dot(q0, up), -dot(q0, fwd));
  float2 sd = m.xy / max(-m.z, 0.05) * px - mq.xy / max(-mq.z, 0.05) * px;
  len = length(sd);
  if (len > 1e-3) dir = sd / len;
  float dl = length(p - c);
  float lit = 0.25 + 1.0 / (1.0 + dl * dl * 3.0);
  float3 l0 = c - cam, lv = float3(dot(l0, right), dot(l0, up), -dot(l0, fwd));
  float perp = length(m.xy / -m.z - lv.xy / -lv.z) * -lv.z;
  float R = Kind > 2.5 ? 0.3 : 0.16;
  float dark = (1.0 - smoothstep(R * 0.5, R, perp)) * smoothstep(0.0, 0.06, m.z - lv.z);
  vFlap = 0.5 + 0.5 * abs(sin(Time * (31.0 + 17.0 * Seed.y) + Seed.w * 6.28));
  vCol = lerp(lerp(Col, float3(1.0, 0.86, 0.66), 0.15) * lit * vFlap, float3(0.03, 0.021, 0.013), dark);
  size = 0.055;
  a = K * step(Seed.x, MothK) * lerp(1.0, 0.75, dark);
  vKind = 0.0;
} else if (Kind < 1.5) {
  float t = Time * (0.85 + 0.3 * Seed.x) + Seed.w * 40.0;
  float R = (2.5 + 4.5 * (0.5 + 0.5 * sin(t * 0.23 + Seed.y * 6.28))) * Wall.w;
  float a1 = t * (6.0 / R) * (Seed.z < 0.5 ? 1.0 : -1.0);
  p = c + float3(cos(a1) * R + 1.2 * sin(t * 2.3 + Seed.y * 9.0), (-1.4 + 1.3 * sin(t * 0.9 + Seed.z * 5.0) + 0.4 * sin(t * 3.1)) * Wall.w, sin(a1 * 0.83 + Seed.x * 4.0) * R);
  d0 = p - cam; m = float3(dot(d0, right), dot(d0, up), -dot(d0, fwd));
  float dd = length(p - c);
  vCol = Col / (1.0 + dd * dd * 0.07) * (0.25 + 0.75 * K);
  size = 0.3;
  a = BatK * smoothstep(-0.3, 0.2, sin(t * 0.045 + Seed.y * 6.28)) * (1.0 - Low);
  vFlap = sin(Time * (13.0 + 3.0 * Seed.x) + Seed.w * 6.28);
  vKind = 1.0;
} else {
  float per = 14.0 + 22.0 * Seed.y, ph = Time / per + Seed.z, n = floor(ph);
  p = c + float3(0.0, 0.018 * (frac(sin(n * 12.9898 + Seed.w * 78.2) * 43758.5) - 0.5), 0.0);
  d0 = p - cam; m = float3(dot(d0, right), dot(d0, up), -dot(d0, fwd));
  vFlap = 1.0 + 0.6 * step(frac(ph), 0.012) * abs(sin(Time * 40.0));
  vCol = Col * (0.05 + 0.1 * K) + 0.003;
  size = 0.042; a = 0.92; vKind = 2.0;
  dir = float2(cos(Seed.x * 6.28), sin(Seed.x * 6.28));
}
a *= 1.0 - Low * High;
float dist = -m.z;
float pxs = size * px / max(dist, 0.1);
float w = max(pxs, 1.5), ps = 1.0;
float3 shift = 0;
if (vKind < 0.5) {
  float L = clamp(len, 2.5, 6.0);
  ps = L + w + 1.0;
  vS = float3(ps, L, 0.5 * w);
  shift = -(dir.x * right + dir.y * up) * (0.5 * L) / px * dist;
} else if (vKind < 1.5) {
  ps = clamp(pxs, 1.0, 64.0);
} else {
  ps = w * 1.4 + 1.0;
  vS = float3(ps, 0.0, 0.5 * w);
}
float vA = a * clamp(pxs / 0.9, 0.45, 1.0) * (1.0 - smoothstep(26.0, 48.0, dist)) * smoothstep(0.4, 1.0, dist);
if (vKind > 0.5 && vKind < 1.5) vA = a * min(pxs, 1.0) * (1.0 - smoothstep(32.0, 55.0, dist)) * smoothstep(0.5, 1.5, dist);
if (Out < 0.5) {
  float s = vA < 0.002 ? 0.0 : ps / px * dist;
  float3 at = p + shift + (Q.x - 0.5) * right * s - (Q.y - 0.5) * up * s;
  return float4(float3(at.x, at.z, at.y) * 100.0 - WP, 0.0);
}
if (Out < 1.5) return float4(vCol, vA);
if (Out < 2.5) return float4(vKind, vFlap, dir.x, dir.y);
return float4(vS, 0.0);"""
MOTH_FS = """float2 q = Q - 0.5;
float vA = I1.w, vKind = I2.x, vFlap = I2.y;
float3 vCol = I1.xyz, vS = I3.xyz;
float2 vDir = I2.zw;
float a; float3 col;
if (vKind < 0.5) {
  float2 s = float2(q.x, -q.y) * vS.x;
  float h = clamp(dot(s, vDir), -0.5 * vS.y, 0.5 * vS.y);
  float e = length(s - vDir * h);
  a = (1.0 - smoothstep(vS.z - 0.5, vS.z + 0.5, e)) * (0.55 + 0.45 * (h / max(vS.y, 1e-3) + 0.5)) * vA;
  col = vCol * Gain;
} else if (vKind < 1.5) {
  q.y = -q.y;
  float x = abs(q.x) * 2.1;
  float top = vFlap * 0.28 * x + 0.05 - 0.03 * x;
  float bot = top - (0.15 * (1.0 - x) + 0.025) + 0.035 * abs(sin(x * 9.4));
  float wing = step(bot, q.y) * step(q.y, top) * step(x, 1.0);
  float body = step(length(q / float2(0.075, 0.17)), 1.0);
  a = max(wing, body) * vA;
  col = vCol * float3(0.1, 0.075, 0.055) + 0.0015;
} else {
  float2 s = float2(q.x, -q.y) * vS.x;
  float2 u = float2(dot(s, vDir), dot(s, float2(-vDir.y, vDir.x))) / max(vS.z * 2.0, 1e-3);
  float wing = step(-0.55, u.x) * step(u.x, 0.5) * step(abs(u.y), (0.55 - u.x) * 0.55 * vFlap);
  a = max(wing, 1.0 - smoothstep(0.35, 0.6, length(u))) * vA;
  col = vCol;
}
if (a < 0.01) a = 0.0;
return float4(col, a);"""
# the dust: P the mote's place in its box (0 to 1), Seed its four numbers
MOTE_VS = VIEW3 + """
float3 Pw = P * Box.xyz + Drift.xyz + Wobble * float3(sin(Time * 0.7 + Seed.x * 40.0), sin(Time * 0.5 + Seed.y * 30.0), cos(Time * 0.6 + Seed.z * 35.0));
float3 base = Center.xyz - 0.5 * Box.xyz;
float3 rel = Pw - base;
float3 l = rel - Box.xyz * floor(rel / Box.xyz);
float3 wp = base + l;
float3 e = min(l, Box.xyz - l) / (0.16 * Box.xyz);
float edge = clamp(min(min(e.x, e.y), e.z), 0.0, 1.0);
float puff = step(Puff, Seed.w);
float light = 0.0, L0 = 0.0;
float3 BP[2] = { BeamPos0.xyz, BeamPos1.xyz };
float3 BD[2] = { BeamDir0.xyz, BeamDir1.xyz };
float BKs[2] = { BeamK.x, BeamK.y };
for (int i = 0; i < 2; i++) {
  float3 v = wp - BP[i];
  float L = length(v);
  float b = BKs[i] * smoothstep(Cos.x, Cos.y, dot(v, BD[i]) / max(L, 1e-3)) / (1.0 + L * L * Fall);
  b *= lerp(1.0, 1.0 - smoothstep(-0.12, 0.1, v.y - (0.15 - 0.012 * L)), Head);
  light += b;
  if (i == 0) L0 = L;
}
light *= lerp(1.0, smoothstep(2.5, 4.0, L0) * (1.0 - smoothstep(8.0, 14.0, L0)), puff * Head);
float3 d0 = wp - cam;
float dist = -(-dot(d0, fwd));
float pxs = lerp(Size * (0.6 + 0.8 * Seed.w), PuffSize * (0.6 + 0.8 * frac(Seed.w * 7.0)), puff) * px / max(dist, 0.05);
float ps = clamp(pxs, 1.0, lerp(16.0, 64.0, puff));
float blur = puff > 0.5 ? 1.0 : min(1.0, 16.0 / max(pxs * pxs, 16.0));
float soft = puff > 0.5 ? 1.0 : smoothstep(4.0, 9.0, pxs);
float fw = lerp(0.3 + 0.7 * pow(max(dot(normalize(wp - cam), BeamDir0.xyz), 0.0), 3.0), 1.0, puff);
float glint = 0.55 + 0.45 * sin(Time * (2.0 + 5.0 * Seed.y) + Seed.z * 6.28);
float vA = step(Seed.x, Density) * edge * light * fw * lerp(glint, PuffA, puff) * min(pxs, 1.0) * blur
  * smoothstep(lerp(Near, 2.0, puff), lerp(2.0 * Near, 5.0, puff), dist);
if (Out < 0.5) {
  float s = vA < 0.002 ? 0.0 : ps / px * dist;
  float3 at = wp + (Q.x - 0.5) * right * s - (Q.y - 0.5) * up * s;
  return float4(float3(at.x, at.z, at.y) * 100.0 - WP, 0.0);
}
return float4(vA, puff, soft, 0.0);"""
MOTE_FS = """float d = length(Q - 0.5), s = smoothstep(0.5, 0.0, d);
float a = lerp(lerp(smoothstep(0.5, 0.1, d), s, I.z), s * s, I.y) * I.x;
if (a < 0.004) a = 0.0;
return Col.xyz * a;"""

MPC_PATH = '/Game/Web/Base/MPC_Web'
mpc = eal.load_asset(MPC_PATH) if eal.does_asset_exist(MPC_PATH) else None
if any(step in only for step in ('mat', 'life', 'sky', 'picture')):
    if mpc is None:
        mpc = tools.create_asset('MPC_Web', '/Game/Web/Base', unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    # parameters added where missing; the ones there keep their ids, so nothing referring to them breaks
    sp = list(mpc.get_editor_property('scalar_parameters'))
    have = {str(p.get_editor_property('parameter_name')) for p in sp}
    # (WindTime, WindAmp: world/wind.ts, set each frame by S47Wind; HorizonHazeK: the dawn's haze on
    # the near mesas, world/horizon.ts, set each frame by S47WorldLife)
    # NativeLighting stays zero in generated assets for historical checkouts sharing content.
    # AS47Web sets it to one for the Unreal renderer; no player setting switches it back.
    for nm, v in (('NativeLighting', 0.0), ('ToneK', 1.05 / 0.6), ('FogDensity', 0.0021), ('Vhs', 1.0), ('PictureMode', 1.0), ('PictureStrength', 1.0), ('PictureCalm', 0.0), ('WindTime', 0.0), ('WindAmp', 0.07), ('HorizonHazeK', 0.0)):
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


def build_master(name, lit, blend, two_sided, flora=None, control_crt=False):
    # flora: None, or which of world/flora.ts's meshes it draws ('card': the grass, the sage, the
    # creosote, the stalks; 'succulent'; 'stones'; 'stain'), with its vertex shader as a world
    # position offset (FLORA_WPO), the sky less inside a plant, the backlight through the cards'
    # edges, both sides of a card lit as its top, and the stones' faces flat
    m = fresh('/Game/Web/Base/' + name)
    m.set_editor_property('two_sided', two_sided)
    m.set_editor_property('blend_mode', blend)
    if flora == 'stones':
        m.set_editor_property('tangent_space_normal', False)
    m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_DEFAULT_LIT if lit or control_crt else unreal.MaterialShadingModel.MSM_UNLIT)
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

    def switched(n, yes, no, yes_pin='', no_pin=''):
        e = node(unreal.MaterialExpressionStaticSwitchParameter, -250)
        e.set_editor_property('parameter_name', n)
        e.set_editor_property('default_value', False)
        # UMaterialExpressionStaticSwitchParameter::GetInputName exposes True/False,
        # although its stored FExpressionInputs and compiler errors call them A/B.
        for source, output, pin in ((yes, yes_pin, 'True'), (no, no_pin, 'False')):
            if not mel.connect_material_expressions(source, output, e, pin):
                raise RuntimeError('S47WEB failed switch %s.%s (output=%r; inputs=%r)' %
                                   (n, pin, output, list(mel.get_material_expression_input_names(e))))
        return e

    def world_to_tangent(source):
        e = node(unreal.MaterialExpressionTransform, -400)
        e.set_editor_property('transform_source_type', unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD)
        e.set_editor_property('transform_type', unreal.MaterialVectorCoordTransform.TRANSFORM_TANGENT)
        mel.connect_material_expressions(source, '', e, '')
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
    # (the vertex colour's outputs have no names: '' is its RGB; a name it has not, as 'RGB', leaves
    # the pin unconnected, white)
    vcm = lerp(one, vc, scalar('UseVertexColor', 0))
    # (and its alpha, which no output of a name gives: read in the shader)
    vca = custom('return Parameters.VertexColor.a + 0 * D.x;', ['D'], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    mel.connect_material_expressions(vc, '', vca, 'D')
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
    vam = lerp(one, vca, scalar('UseVertexColor', 0))
    alpha = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, scalar('Opacity', 1), tex_a), am), vam)
    if lit:
        detail_normal = None
        rough = op(unreal.MaterialExpressionMultiply, scalar('Roughness', 1), lerp(one, texp('RoughnessMap', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, white_lin, xuv('Rough')), scalar('UseRoughnessMap', 0), '', 'G'))
        if not flora:
            world_pos = node(unreal.MaterialExpressionWorldPosition)
            world_normal = node(unreal.MaterialExpressionVertexNormalWS)
            duv = custom(SURFACE_UV, ['WP', 'N', 'Size'])
            mel.connect_material_expressions(world_pos, '', duv, 'WP')
            mel.connect_material_expressions(world_normal, '', duv, 'N')
            mel.connect_material_expressions(scalar('DetailWorldSize', 50), '', duv, 'Size')
            detail = texp('SurfaceDetailTex', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, surface_detail, duv)
            macro_uv = custom('return WP.xy / max(Size, 1.0) + float2(0.173, 0.619);', ['WP', 'Size'])
            mel.connect_material_expressions(world_pos, '', macro_uv, 'WP')
            mel.connect_material_expressions(scalar('MacroWorldSize', 2400), '', macro_uv, 'Size')
            macro = texp('SurfaceMacroTex', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, surface_detail, macro_uv)
            var_base = custom('return B * max(0.0, 1.0 + (M * 2.0 - 1.0) * Amount);', ['B', 'M', 'Amount'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            mel.connect_material_expressions(base, '', var_base, 'B')
            mel.connect_material_expressions(macro, 'A', var_base, 'M')
            mel.connect_material_expressions(scalar('MacroColorVariation', 0), '', var_base, 'Amount')
            base = switched('SurfaceDetail', var_base, base)
            var_rough = custom('''float dirt = 1.0 - saturate(dot(VC, float3(0.2126, 0.7152, 0.0722)));
// Native specular replaces roadSurface.ts's emissive wet streak.  Pale road
// paint and gravel retain the roughness; broad dark tar catches a faint sheen.
float darkTar = 1.0 - smoothstep(0.025, 0.18, dot(Albedo, float3(0.2126, 0.7152, 0.0722)));
float damp = saturate(darkTar * Sheen * smoothstep(0.25, 0.72, M));
float r = lerp(R, min(R, 0.22), damp);
return clamp(r + (D * 2.0 - 1.0) * Variation + (M * 2.0 - 1.0) * MacroVariation + dirt * Dirt, 0.04, 1.0);''',
                               ['R', 'D', 'M', 'Variation', 'MacroVariation', 'VC', 'Dirt', 'Albedo', 'Sheen'], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
            for pin, source, output in (('R', rough, ''), ('D', detail, 'B'), ('M', macro, 'A'), ('VC', vcm, '')):
                mel.connect_material_expressions(source, output, var_rough, pin)
            for pin, name in (('Variation', 'RoughnessVariation'), ('MacroVariation', 'MacroRoughnessVariation'), ('Dirt', 'VertexDirtRoughness')):
                mel.connect_material_expressions(scalar(name, 0), '', var_rough, pin)
            mel.connect_material_expressions(tex_rgb, '', var_rough, 'Albedo')
            mel.connect_material_expressions(scalar('DampSheen', 0), '', var_rough, 'Sheen')
            rough = switched('SurfaceDetail', var_rough, rough)
            detail_normal = custom(SURFACE_NORMAL, ['N', 'RG', 'Strength'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            mel.connect_material_expressions(world_normal, '', detail_normal, 'N')
            # A float3 input is safe: only the packed red/green slopes are read.
            mel.connect_material_expressions(detail, 'RGB', detail_normal, 'RG')
            mel.connect_material_expressions(scalar('DetailNormalStrength', 0), '', detail_normal, 'Strength')
            detail_normal = world_to_tangent(detail_normal)
        mel.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
        mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
        mel.connect_material_property(scalar('Specular', .5), '', unreal.MaterialProperty.MP_SPECULAR)
        if not flora:
            aomap = texp('AOMap', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, white_lin, xuv('AO'))
            ao = lerp(one, aomap, scalar('AOIntensity', 1), '', 'R')
            ao = switched('HasAOMap', ao, one)
            mel.connect_material_property(ao, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
        metal = op(unreal.MaterialExpressionMultiply, scalar('Metallic', 0), lerp(one, texp('MetalnessMap', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, white_lin, xuv('Metal')), scalar('UseMetalnessMap', 0), '', 'B'))
        mel.connect_material_property(metal, '', unreal.MaterialProperty.MP_METALLIC)
        euv = xuv('Em')
        emap_c = texp('EmissiveMap', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, white, euv)
        emis = op(unreal.MaterialExpressionMultiply, vector('Emissive', (0, 0, 0, 1)), lerp(one, emap_c, scalar('UseEmissiveMap', 0), '', 'RGB'))
        # Native indicator/power gating keeps authored emissive maps intact. Brightness
        # belongs to the unlit picture path and cannot dim a lit console-panel material.
        emis = op(unreal.MaterialExpressionMultiply, emis, scalar('EmissivePower', 1))
        sway = None
        if flora:
            # the shape's sway and which of two plants a corner is (UV channel 1: export.js), the
            # copy's origin, x axis and mirroring (custom data 3), the wind, the kind's reach
            t1 = node(unreal.MaterialExpressionTextureCoordinate)
            t1.set_editor_property('coordinate_index', 1)
            sway = node(unreal.MaterialExpressionComponentMask)
            sway.set_editor_property('r', True); sway.set_editor_property('g', False); sway.set_editor_property('b', False); sway.set_editor_property('a', False)
            mel.connect_material_expressions(t1, '', sway, '')
            wpo = custom(FLORA_WPO, ['WP', 'IO', 'IX', 'SP', 'Mirror', 'Cam', 'Reach', 'Parts', 'WindT', 'WindA', 'WindDir'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            mel.connect_material_expressions(node(unreal.MaterialExpressionWorldPosition), '', wpo, 'WP')
            zero = node(unreal.MaterialExpressionConstant3Vector)
            io = node(unreal.MaterialExpressionTransformPosition)
            io.set_editor_property('transform_source_type', unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_INSTANCE)
            io.set_editor_property('transform_type', unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
            mel.connect_material_expressions(zero, '', io, '')
            mel.connect_material_expressions(io, '', wpo, 'IO')
            xa = node(unreal.MaterialExpressionConstant3Vector)
            xa.set_editor_property('constant', unreal.LinearColor(1, 0, 0, 0))
            ix = node(unreal.MaterialExpressionTransform)
            ix.set_editor_property('transform_source_type', unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_INSTANCE)
            ix.set_editor_property('transform_type', unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
            mel.connect_material_expressions(xa, '', ix, '')
            mel.connect_material_expressions(ix, '', wpo, 'IX')
            mel.connect_material_expressions(t1, '', wpo, 'SP')
            mir = node(unreal.MaterialExpressionPerInstanceCustomData)
            mir.set_editor_property('data_index', 3)
            mir.set_editor_property('const_default_value', 0.0)
            mel.connect_material_expressions(mir, '', wpo, 'Mirror')
            mel.connect_material_expressions(node(unreal.MaterialExpressionCameraPositionWS), '', wpo, 'Cam')
            mel.connect_material_expressions(scalar('Reach', 65), '', wpo, 'Reach')
            mel.connect_material_expressions(scalar('Parts', 0), '', wpo, 'Parts')
            mel.connect_material_expressions(coll('WindTime'), '', wpo, 'WindT')
            mel.connect_material_expressions(coll('WindAmp'), '', wpo, 'WindA')
            mel.connect_material_expressions(vector('WindDir', (0.8, 0.6, 0, 0)), '', wpo, 'WindDir')
            mel.connect_material_property(wpo, '', unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
        # the normal the lights take: the stones' faces flat (three's flatShading)
        if flora == 'stones':
            nrm = custom(FLAT, ['WP', 'V'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            rel = node(unreal.MaterialExpressionWorldPosition)
            rel.set_editor_property('world_position_shader_offset', unreal.WorldPositionIncludedOffsets.WPT_CAMERA_RELATIVE)
            mel.connect_material_expressions(rel, '', nrm, 'WP')
            mel.connect_material_expressions(node(unreal.MaterialExpressionCameraVectorWS), '', nrm, 'V')
        else:
            nrm = node(unreal.MaterialExpressionVertexNormalWS)
        # Legacy exports keep their surface-lighting terms until AS47Web enables native lights.
        # True material emissive remains untouched so CRTs, signs and lamps can illuminate Lumen.
        if flora:
            fc = custom(FLOOD_FLORA, ['Native', 'WP', 'NW', 'Tex', 'Count', 'Fall', 'Cam', 'Sway', 'Back'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            mel.connect_material_expressions(node(unreal.MaterialExpressionCameraPositionWS), '', fc, 'Cam')
            mel.connect_material_expressions(sway, '', fc, 'Sway')
            mel.connect_material_expressions(scalar('Back', 0), '', fc, 'Back')
        else:
            fc = custom(FLOOD, ['Native', 'WP', 'NW', 'Tex', 'Count', 'Fall'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        mel.connect_material_expressions(coll('NativeLighting'), '', fc, 'Native')
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
        # drive/herd.ts beamLit: the cows and frost also see the headlight cone itself. Its
        # level defaults to zero, so existing lit materials keep their look; S47Herd sets it.
        bc = custom('''if (Native > 0.5) return 0;
float3 p = float3(WP.x, WP.z, WP.y) * 0.01;
float3 n = float3(NW.x, NW.z, NW.y);
float3 bL = BP - p; float bD = length(bL); bL /= max(bD, 1e-3);
float bCone = smoothstep(0.68, 0.96, dot(-bL, BD));
float bNdl = lerp(max(dot(normalize(n), bL), 0.0) * 0.8 + 0.2, 1.0, Flat);
return B * Level * bCone * bNdl * 1.4 / (1.0 + bD * bD / 2500.0);''',
                    ['Native', 'WP', 'NW', 'BP', 'BD', 'Level', 'Flat', 'B'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        mel.connect_material_expressions(coll('NativeLighting'), '', bc, 'Native')
        mel.connect_material_expressions(node(unreal.MaterialExpressionWorldPosition), '', bc, 'WP')
        mel.connect_material_expressions(node(unreal.MaterialExpressionVertexNormalWS), '', bc, 'NW')
        bp = vector('BeamPos', (0, -999, 0, 0))
        mel.connect_material_expressions(bp, 'RGB', bc, 'BP')
        mel.connect_material_expressions(bp, 'A', bc, 'Level')
        mel.connect_material_expressions(vector('BeamDir', (0, 0, -1, 0)), 'RGB', bc, 'BD')
        mel.connect_material_expressions(scalar('BeamFlat', 0), '', bc, 'Flat')
        mel.connect_material_expressions(base, '', bc, 'B')
        emis = op(unreal.MaterialExpressionAdd, emis, bc)
        # HemisphereLight: mix(ground, sky, n.y / 2 + 1/2) * diffuse / pi, as three's indirect diffuse
        if flora:
            # (flora.ts shade: the sky in a tuft, a third at the root, three quarters at the tips)
            hc = custom('if (Native > 0.5) return 0; float k = saturate(N.z * 0.5 + 0.5); float fSky = lerp(1.0, lerp(0.35, 0.75, min(1.0, Sw * 1.6)), Shade); return lerp(G, S, k) * B * (1 - saturate(M)) / 3.14159265 * fSky;',
                        ['Native', 'N', 'G', 'S', 'B', 'M', 'Sw', 'Shade'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            mel.connect_material_expressions(sway, '', hc, 'Sw')
            mel.connect_material_expressions(scalar('Shade', 0), '', hc, 'Shade')
        else:
            hc = custom('if (Native > 0.5) return 0; float k = saturate(N.z * 0.5 + 0.5); return lerp(G, S, k) * B * (1 - saturate(M)) / 3.14159265;', ['Native', 'N', 'G', 'S', 'B', 'M'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        mel.connect_material_expressions(coll('NativeLighting'), '', hc, 'Native')
        mel.connect_material_expressions(nrm, '', hc, 'N')
        mel.connect_material_expressions(coll('HemiGround'), '', hc, 'G')
        mel.connect_material_expressions(coll('HemiSky'), '', hc, 'S')
        mel.connect_material_expressions(base, '', hc, 'B')
        mel.connect_material_expressions(metal, '', hc, 'M')
        emis = op(unreal.MaterialExpressionAdd, emis, hc)
        mel.connect_material_property(emis, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        if flora == 'stones':
            mel.connect_material_property(nrm, '', unreal.MaterialProperty.MP_NORMAL)
        elif flora in ('card', 'succulent'):
            # (flora.ts: both sides of a blade take the light as its top does: Unreal turns a back
            # face's normal round, the two-sided sign turns it back)
            tsn = custom('return float3(0, 0, S);', ['S'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            mel.connect_material_expressions(node(unreal.MaterialExpressionTwoSidedSign), '', tsn, 'S')
            mel.connect_material_property(tsn, '', unreal.MaterialProperty.MP_NORMAL)
        else:
            nmap = texp('NormalMap', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, flat, xuv('Normal'))
            normal = custom('float2 xy = N.xy * Scale.xy; return normalize(float3(xy, N.z));', ['N', 'Scale'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            mel.connect_material_expressions(nmap, 'RGB', normal, 'N')
            mel.connect_material_expressions(vector('NormalScale', (1, 1, 0, 0)), 'RGB', normal, 'Scale')
            bump = texp('BumpMap', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, white_lin, xuv('Bump'))
            bumped = custom(BUMP_NORMAL, ['WP', 'N', 'H', 'Scale'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            mel.connect_material_expressions(node(unreal.MaterialExpressionWorldPosition), '', bumped, 'WP')
            mel.connect_material_expressions(node(unreal.MaterialExpressionVertexNormalWS), '', bumped, 'N')
            mel.connect_material_expressions(bump, 'R', bumped, 'H')
            # three's bumpScale is in metres, the imported world is centimetres.
            mel.connect_material_expressions(scalar('BumpScale', 100), '', bumped, 'Scale')
            normal = switched('HasBumpMap', world_to_tangent(bumped), normal)
            if detail_normal:
                normal = switched('SurfaceDetail', detail_normal, normal)
            mel.connect_material_property(normal, '', unreal.MaterialProperty.MP_NORMAL)
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
        if control_crt:
            # Unlike the web's MeshBasicMaterial, a CRT's front glass reflects the
            # room. Its image remains exactly the existing emissive path above:
            # Brightness=0 removes phosphor, while the dark glass stays passive.
            mel.connect_material_property(vector('GlassColor', (.012, .014, .013, 1)), 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
            mel.connect_material_property(scalar('Roughness', .22), '', unreal.MaterialProperty.MP_ROUGHNESS)
            mel.connect_material_property(scalar('Specular', .5), '', unreal.MaterialProperty.MP_SPECULAR)
            mel.connect_material_property(scalar('Metallic', 0), '', unreal.MaterialProperty.MP_METALLIC)
            # Inverse ACES maps black to its toe cutoff, a small positive value.
            # A powered-off CRT must not send that value into Lumen in Clean mode.
            off = custom('return Level <= 0.0 ? float3(0, 0, 0) : C;', ['Level', 'C'], unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            mel.connect_material_expressions(scalar('Brightness', 1), '', off, 'Level')
            mel.connect_material_expressions(boost, '', off, 'C')
            boost = off
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
    crt_master = build_master('M_WebCRT', False, BLENDS['Opaque'], False, control_crt=True)
    # world/flora.ts: the ground cover's own (export.js marks its materials with the mesh they are on)
    flora_masters = {
        'card': build_master('M_WebFlora_Card', True, BLENDS['Masked'], True, 'card'),
        'succulent': build_master('M_WebFlora_Succulent', True, BLENDS['Opaque'], True, 'succulent'),
        'stones': build_master('M_WebFlora_Stones', True, BLENDS['Opaque'], False, 'stones'),
        'stain': build_master('M_WebFlora_Stain', True, BLENDS['Trans'], False, 'stain'),
    }
    FLORA_KIND = {'grass+sage': 'card', 'creosote+stalks': 'card', 'succulent': 'succulent', 'stones': 'stones', 'stain': 'stain'}
    log('masters %d, flora %d' % (len(masters), len(flora_masters)))

    # ---------- one instance per web material ----------
    tex_srgb = {t['id']: t['srgb'] for t in man['textures']}
    tex_names = texture_labels(man)
    mat_labels = material_labels(man)
    detail_counts = {}
    glass_count = 0
    crt_count = 0

    def T(i):
        return eal.load_asset('/Game/Web/Tex/T%d' % i) if i is not None and i >= 0 else None

    def rows(uvr):
        return unreal.LinearColor(uvr[0], uvr[1], uvr[2], 0), unreal.LinearColor(uvr[3], uvr[4], uvr[5], 0)

    made = 0
    for d in (() if masters_only else man['materials']):
        lit = bool(d.get('lit'))
        labels = mat_labels.get(d['id'], set())
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
        if is_control_crt(d, labels):
            parent = crt_master
            crt_count += 1
        fk = FLORA_KIND.get(d.get('flora', ''))
        if fk:
            parent = flora_masters[fk]
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
                scale = d.get('normalScale', [1, 1])
                vv(mi, 'NormalScale', unreal.LinearColor(float(scale[0]), float(scale[1]), 0, 0))
            # Source normal maps take precedence over bump maps, as in three.
            if not fk and 'normalMap' not in d and 'bumpMap' in d and T(d['bumpMap']):
                tv(mi, 'BumpMap', T(d['bumpMap']))
                r0, r1 = rows(d['bumpMapUV']); vv(mi, 'BumpU', r0); vv(mi, 'BumpV', r1)
                sv(mi, 'BumpScale', float(d.get('bumpScale', 1)) * 100.)
                mel.set_material_instance_static_switch_parameter_value(mi, 'HasBumpMap', True, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER, False)
            if not fk and 'aoMap' in d and T(d['aoMap']):
                tv(mi, 'AOMap', T(d['aoMap']))
                r0, r1 = rows(d['aoMapUV']); vv(mi, 'AOU', r0); vv(mi, 'AOV', r1)
                sv(mi, 'AOIntensity', float(d.get('aoMapIntensity', 1)))
                mel.set_material_instance_static_switch_parameter_value(mi, 'HasAOMap', True, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER, False)
            profile = profile_for(d, tex_names, labels)
            if profile:
                for parameter, value in PROFILES[profile].items():
                    sv(mi, parameter, value)
                mel.set_material_instance_static_switch_parameter_value(mi, 'SurfaceDetail', True, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER, False)
                detail_counts[profile] = detail_counts.get(profile, 0) + 1
                log('surface detail MI_W%d %s %s' % (d['id'], profile, ','.join(sorted(labels))))
            glass = glass_parameters(d, labels)
            if glass:
                for parameter, value in glass.items():
                    sv(mi, parameter, value)
                glass_count += 1
        if not lit and d.get('toneMapped') is False:
            sv(mi, 'Untonemapped', 1.0)
        if bk == 'Masked':
            ov = mi.get_editor_property('base_property_overrides')
            ov.set_editor_property('override_opacity_mask_clip_value', True)
            ov.set_editor_property('opacity_mask_clip_value', float(d.get('alphaTest', 0.5)))
            mi.set_editor_property('base_property_overrides', ov)
        if fk:
            # flora.ts floraMaterial(o, set, falloff, reach, parts, shade, back): the plants take the
            # sky less inside (shade), the cards let a lamp through their edges (back), the
            # succulents fold one of their two plants away (parts); the reach is set at run time
            sv(mi, 'Shade', 1.0 if fk in ('card', 'succulent') else 0.0)
            sv(mi, 'Back', 1.0 if fk == 'card' else 0.0)
            sv(mi, 'Parts', 1.0 if fk == 'succulent' else 0.0)
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
    log('surface detail profiles %s; dielectric glass %d; control CRT glass %d' % (json.dumps(detail_counts, sort_keys=True), glass_count, crt_count))
    if masters_only:
        log('masters only: existing material instances and parent bindings preserved')
class Graph:
    """A material made here node by node: parameters, the MPC's values, Unreal's built-in inputs and
    custom HLSL, laid out down the left."""

    def __init__(self, path, blend, two_sided=True, ism=False, sky=False):
        self.m = m = fresh(path)
        m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
        m.set_editor_property('blend_mode', blend)
        m.set_editor_property('two_sided', two_sided)
        if ism:
            m.set_editor_property('used_with_instanced_static_meshes', True)
        if sky:
            try:
                m.set_editor_property('is_sky', True)
            except Exception:
                pass
        self.y = 0

    def node(self, cls, x=-1400):
        self.y += 110
        return mel.create_material_expression(self.m, cls, x, self.y)

    def scalar(self, n, v):
        e = self.node(unreal.MaterialExpressionScalarParameter)
        e.set_editor_property('parameter_name', n); e.set_editor_property('default_value', v)
        return e

    def vector(self, n, v):
        e = self.node(unreal.MaterialExpressionVectorParameter)
        e.set_editor_property('parameter_name', n); e.set_editor_property('default_value', unreal.LinearColor(*v))
        return e

    def vector4(self, n, v):
        """a vector parameter with its fourth value (its main output has three)"""
        e = self.vector(n, v)
        return self.custom('return float4(C, A);', [('C', e), ('A', e, 'A')], 4)

    def coll(self, n):
        e = self.node(unreal.MaterialExpressionCollectionParameter)
        e.set_editor_property('collection', mpc); e.set_editor_property('parameter_name', n)
        return e

    def tex(self, n, default=None, linear=False):
        e = self.node(unreal.MaterialExpressionTextureObjectParameter)
        e.set_editor_property('parameter_name', n)
        e.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR if linear else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        e.set_editor_property('texture', default or (white_lin if linear else white))
        return e

    def uv(self, i=0):
        e = self.node(unreal.MaterialExpressionTextureCoordinate)
        e.set_editor_property('coordinate_index', i)
        return e

    def cdata(self, i, default=0.0):
        e = self.node(unreal.MaterialExpressionPerInstanceCustomData)
        e.set_editor_property('data_index', i); e.set_editor_property('const_default_value', default)
        return e

    def cdata3(self, i):
        c = self.custom('return float3(X, Y, Z);', [('X', self.cdata(i)), ('Y', self.cdata(i + 1)), ('Z', self.cdata(i + 2))], 3)
        return c

    def cdata4(self, i):
        return self.custom('return float4(X, Y, Z, W);', [('X', self.cdata(i)), ('Y', self.cdata(i + 1)), ('Z', self.cdata(i + 2)), ('W', self.cdata(i + 3))], 4)

    def const(self, v):
        e = self.node(unreal.MaterialExpressionConstant)
        e.set_editor_property('r', v)
        return e

    def view_axis(self, v):
        c = self.node(unreal.MaterialExpressionConstant3Vector)
        c.set_editor_property('constant', unreal.LinearColor(v[0], v[1], v[2], 0))
        e = self.node(unreal.MaterialExpressionTransform)
        e.set_editor_property('transform_source_type', unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_VIEW)
        e.set_editor_property('transform_type', unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
        mel.connect_material_expressions(c, '', e, '')
        return e

    def instance_origin(self):
        z = self.node(unreal.MaterialExpressionConstant3Vector)
        e = self.node(unreal.MaterialExpressionTransformPosition)
        e.set_editor_property('transform_source_type', unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_INSTANCE)
        e.set_editor_property('transform_type', unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
        mel.connect_material_expressions(z, '', e, '')
        return e

    def view_prop(self, prop, channel):
        e = self.node(unreal.MaterialExpressionViewProperty)
        e.set_editor_property('property', prop)
        mk = self.node(unreal.MaterialExpressionComponentMask)
        for ch in 'rgba':
            mk.set_editor_property(ch, ch == channel)
        mel.connect_material_expressions(e, '', mk, '')
        return mk

    def mask(self, src, chans, output=''):
        mk = self.node(unreal.MaterialExpressionComponentMask)
        for ch in 'rgba':
            mk.set_editor_property(ch, ch in chans)
        mel.connect_material_expressions(src, output, mk, '')
        return mk

    def custom(self, code, inputs, out=3):
        """inputs: (name, node) or (name, node, output name)"""
        e = self.node(unreal.MaterialExpressionCustom, -800)
        e.set_editor_property('code', code)
        e.set_editor_property('output_type', {1: unreal.CustomMaterialOutputType.CMOT_FLOAT1, 2: unreal.CustomMaterialOutputType.CMOT_FLOAT2,
                                              3: unreal.CustomMaterialOutputType.CMOT_FLOAT3, 4: unreal.CustomMaterialOutputType.CMOT_FLOAT4}[out])
        ins = []
        for it in inputs:
            ci = unreal.CustomInput(); ci.set_editor_property('input_name', it[0]); ins.append(ci)
        e.set_editor_property('inputs', ins)
        for it in inputs:
            mel.connect_material_expressions(it[1], it[2] if len(it) > 2 else '', e, it[0])
        return e

    def interp(self, src):
        """a vertex shader value carried to the pixel shader"""
        e = self.node(unreal.MaterialExpressionVertexInterpolator, -600)
        mel.connect_material_expressions(src, '', e, 'VS')
        return e

    def builtin(self, cls, **props):
        e = self.node(cls)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    def untoned(self, c):
        """toneMapped false (three draws it without the tone curve, under the tape with it): as the unlit masters"""
        un = self.custom(UNTONE, [('C', c), ('K', self.coll('ToneK'))])
        return self.custom('return lerp(U, C, V);', [('U', un), ('C', c), ('V', self.coll('Vhs'))])

    def defogged(self, c):
        sd = self.builtin(unreal.MaterialExpressionSceneDepth)
        cd = self.builtin(unreal.MaterialExpressionSceneTexture, scene_texture_id=unreal.SceneTextureId.PPI_CUSTOM_DEPTH)
        cs = self.builtin(unreal.MaterialExpressionSceneTexture, scene_texture_id=unreal.SceneTextureId.PPI_CUSTOM_STENCIL)
        k = self.custom(DEFOG, [('SD', sd), ('Dens', self.coll('FogDensity')), ('CD', self.mask(cd, 'r', 'Color')), ('CS', self.mask(cs, 'r', 'Color')),
                                ('HazeK', self.coll('HorizonHazeK'))], 1)
        return self.custom('return C * K;', [('C', c), ('K', k)])

    def out(self, src, prop, output=''):
        mel.connect_material_property(src, output, prop)

    def save(self):
        mel.recompile_material(self.m)
        eal.save_loaded_asset(self.m)
        return self.m


def build_life():
    """The world's own life (S47WorldLife): the live sky (Sky.ts's dome, its stars, the shooting star),
    the painted horizon (backdrop.ts), the light in the air (airlight.ts) and the windows' light on the
    ground (windowLight.ts), each with its three.js shader as HLSL and its uniforms as parameters."""
    MP = unreal.MaterialProperty
    B = unreal.BlendMode
    VP = unreal.MaterialExposedViewProperty
    TAN = getattr(VP, 'MEVP_TAN_HALF_FIELD_OF_VIEW', None) or getattr(VP, 'TAN_HALF_FIELD_OF_VIEW')
    VSIZE = getattr(VP, 'MEVP_VIEW_SIZE', None) or getattr(VP, 'VIEW_SIZE')
    WP = lambda g: g.builtin(unreal.MaterialExpressionWorldPosition)
    CAM = lambda g: g.builtin(unreal.MaterialExpressionCameraPositionWS)
    CV = lambda g: g.builtin(unreal.MaterialExpressionCameraVectorWS)
    sky_ins = lambda g: [('Dawn', g.scalar('Dawn', 0)), ('Sun', g.scalar('Sun', 0)), ('SunDir', g.vector('SunDir', (0.98, -0.02, -0.19, 0))),
                         ('Glow0', g.vector4('Glow0', (1, 0, 0, 1))), ('Glow1', g.vector4('Glow1', (1, 0, 0, 1))), ('Glow2', g.vector4('Glow2', (1, 0, 0, 1)))]
    hole_ins = lambda g: [('HoleDir', g.vector('HoleDir', (0.3718, 0.5299, -0.7622, 0))), ('HoleR', g.vector('HoleR', (0.0593, 0.0803, 0, 0))), ('Hole', g.scalar('Hole', 0))]
    # the dome
    g = Graph('/Game/Web/Base/M_WebSkyDome', B.BLEND_OPAQUE, sky=True)
    col = g.custom(SKY_DOME, [('V', CV(g)), ('Yaw', g.scalar('Yaw', 0)), ('Flash', g.scalar('Flash', 0)), ('FlashDir', g.vector('FlashDir', (0.957, 0.048, -0.287, 0))),
                               ('HoleSky', g.vector('HoleSky', (0.0044, 0.0066, 0.016, 0))), ('Pano', g.tex('Pano'))] + sky_ins(g) + hole_ins(g))
    g.out(col, MP.MP_EMISSIVE_COLOR)
    g.save()
    # the painted horizon on its ring, over the stars, the comet and the shooting star
    g = Graph('/Game/Web/Base/M_WebSkyBand', B.BLEND_ALPHA_COMPOSITE)
    band = g.custom(SKY_BAND, [('V', CV(g)), ('Yaw', g.scalar('Yaw', 0)), ('U', g.mask(g.uv(0), 'r')), ('Night', g.tex('Night')), ('Day', g.tex('Day')),
                                ('Gain', g.scalar('Gain', 1)), ('Rim', g.scalar('Rim', 0)), ('BodyK', g.scalar('BodyK', 0.2)), ('Dip', g.scalar('Dip', 0)),
                                ('Texel', g.scalar('Texel', 1 / 128)), ('Fog', g.vector('Fog', (0.0155, 0.0212, 0.0353, 0))), ('Mesa', g.vector('Mesa', (0.0045, 0.0055, 0.0085, 0))),
                                ('Land', g.vector('Land', (0.009, 0.009, 0.012, 0))), ('EdgeFog', g.scalar('EdgeFog', 1))] + sky_ins(g), 4)
    g.out(g.mask(band, 'rgb'), MP.MP_EMISSIVE_COLOR)
    g.out(g.mask(band, 'a'), MP.MP_OPACITY)
    g.save()
    # the stars: a quad per star turned to the eye (custom data: direction 0-2, colour 3-5, size 6, phase 7)
    g = Graph('/Game/Web/Base/M_WebStars', B.BLEND_ADDITIVE, ism=True)
    tan = g.view_prop(TAN, 'g')
    vsz = g.view_prop(VSIZE, 'g')
    wpo = g.custom(STAR_WPO, [('PR', g.scalar('PR', 1)), ('Size', g.cdata(6, 1)), ('IO', g.instance_origin()), ('Cam', CAM(g)), ('Tan', tan), ('VSize', vsz),
                              ('UV', g.uv(0)), ('Right', g.view_axis((1, 0, 0))), ('Up', g.view_axis((0, 1, 0))), ('WP', WP(g))])
    g.out(wpo, MP.MP_WORLD_POSITION_OFFSET)
    c = g.custom(STAR_COL, [('Dir', g.cdata3(0)), ('Col', g.cdata3(3)), ('Time', g.builtin(unreal.MaterialExpressionTime)), ('Ph', g.cdata(7)), ('Dawn', g.scalar('Dawn', 0)),
                            ('UV', g.uv(0)), ('Size', g.cdata(6, 1))] + hole_ins(g))
    g.out(g.untoned(c), MP.MP_EMISSIVE_COLOR)
    g.save()
    # the shooting star's strip
    g = Graph('/Game/Web/Base/M_WebMeteor', B.BLEND_ADDITIVE)
    streak = lambda g: [('Back', g.scalar('Back', 0)), ('Head', g.scalar('Head', 0)), ('Start', g.vector('Start', (0, 1, 0, 0))), ('Way', g.vector('Way', (0, 0, -1, 0)))]
    wpo = g.custom(METEOR_WPO, [('UV', g.uv(0)), ('Yaw', g.scalar('Yaw', 0)), ('Cam', CAM(g)), ('Scale', g.scalar('Scale', 1000)), ('WP', WP(g))] + streak(g))
    g.out(wpo, MP.MP_WORLD_POSITION_OFFSET)
    c = g.custom(METEOR_COL, [('UV', g.uv(0)), ('I', g.scalar('I', 0))] + streak(g) + hole_ins(g))
    g.out(g.untoned(c), MP.MP_EMISSIVE_COLOR)
    g.save()
    # the cones of lamplight (aCol is the vertex colour; the lamps' strengths a row of K)
    g = Graph('/Game/Web/Base/M_WebAirCone', B.BLEND_ADDITIVE)
    c = g.custom(AIR_CONE, [('Cam', CAM(g)), ('WP', WP(g)), ('Nw', g.builtin(unreal.MaterialExpressionVertexNormalWS)), ('VA', g.uv(0)), ('K', g.tex('K', linear=True)),
                            ('Lamps', g.scalar('Lamps', 1)), ('Gain', g.scalar('Gain', 0)), ('Depth', g.builtin(unreal.MaterialExpressionPixelDepth)),
                            ('Dens', g.coll('FogDensity')), ('Col', g.builtin(unreal.MaterialExpressionVertexColor)), ('Native', g.coll('NativeLighting'))])
    g.out(g.defogged(c), MP.MP_EMISSIVE_COLOR)
    g.save()
    # the haze on the ground
    g = Graph('/Game/Web/Base/M_WebHaze', B.BLEND_TRANSLUCENT)
    lamps = [('L%d' % i, g.vector4('Lamp%d' % i, (0, -999, 0, 1))) for i in range(4)] + [('C%d' % i, g.vector('LampCol%d' % i, (0, 0, 0, 0))) for i in range(4)]
    hz = g.custom(AIR_HAZE, [('WP', WP(g)), ('Cam', CAM(g)), ('Time', g.scalar('Time', 0)), ('Layer', g.mask(g.uv(0), 'r')), ('R', g.scalar('R', 320)),
                             ('Col', g.vector('Col', (0, 0, 0, 0))), ('K', g.scalar('K', 0)), ('Depth', g.builtin(unreal.MaterialExpressionPixelDepth)),
                             ('Dens', g.coll('FogDensity')), ('Native', g.coll('NativeLighting'))] + lamps, 4)
    g.out(g.mask(hz, 'rgb'), MP.MP_EMISSIVE_COLOR)
    g.out(g.mask(hz, 'a'), MP.MP_OPACITY)
    g.save()
    # the windows' light on the ground (UV 0: u q, v q; 1: q, the blur across; 2: the blur up, the corner's index)
    g = Graph('/Game/Web/Base/M_WebWindowLight', B.BLEND_ADDITIVE)
    vcol = g.custom(WIN_COL, [('I', g.mask(g.uv(2), 'g')), ('Cols', g.tex('Cols', linear=True)), ('ColsW', g.scalar('ColsW', 1)), ('ColsH', g.scalar('ColsH', 1))])
    wpo = g.custom(WIN_WPO, [('Cam', CAM(g)), ('WP', WP(g))])
    g.out(wpo, MP.MP_WORLD_POSITION_OFFSET)
    c = g.custom(WIN_FS, [('Q0', g.uv(0)), ('Q1', g.uv(1)), ('Q2', g.uv(2)), ('MapSize', g.vector('MapSize', (384, 384, 0, 0))), ('Map', g.tex('Map')),
                          ('VCol', g.interp(vcol)), ('Depth', g.builtin(unreal.MaterialExpressionPixelDepth)), ('Dens', g.coll('FogDensity')), ('Native', g.coll('NativeLighting'))])
    g.out(g.defogged(c), MP.MP_EMISSIVE_COLOR)
    g.save()
    # the life in the night: the moths and bats round the lamps (custom data: seed 0-3, lamp 4, kind 5,
    # colour 6-8, wall 9-12, drawn on High only 13; the lamps' strengths a row of K)
    view = lambda g: [('Cam', CAM(g)), ('Right', g.view_axis((1, 0, 0))), ('Up', g.view_axis((0, 1, 0))), ('Fwd', g.view_axis((0, 0, 1))),
                      ('Tan', g.view_prop(TAN, 'g')), ('Lines', g.scalar('Lines', 720)), ('Q', g.uv(0)), ('WP', WP(g))]
    g = Graph('/Game/Web/Base/M_WebMoths', B.BLEND_TRANSLUCENT, ism=True)
    lamp = g.cdata(4)
    k = g.custom('return K.SampleLevel(KSampler, float2((floor(L + 0.5) + 0.5) / max(N, 1.0), 0.5), 0).r;', [('K', g.tex('K', linear=True)), ('L', lamp), ('N', g.scalar('Lamps', 1))], 1)
    ins = view(g) + [('IO', g.instance_origin()), ('Seed', g.cdata4(0)), ('Kind', g.cdata(5)), ('Col', g.cdata3(6)), ('Wall', g.cdata4(9)), ('High', g.cdata(13)),
                     ('K', k), ('Time', g.scalar('Time', 0)), ('Shut', g.scalar('Shut', 1 / 30)), ('MothK', g.scalar('MothK', 1)), ('BatK', g.scalar('BatK', 0)), ('Low', g.scalar('Low', 0))]
    outs = [g.custom(MOTH_VS, ins + [('Out', g.const(o))], 4) for o in range(4)]
    g.out(g.mask(outs[0], 'rgb'), MP.MP_WORLD_POSITION_OFFSET)
    fs = g.custom(MOTH_FS, [('Q', g.uv(0)), ('I1', g.interp(outs[1])), ('I2', g.interp(outs[2])), ('I3', g.interp(outs[3])), ('Gain', g.scalar('Gain', 1.2))], 4)
    g.out(g.mask(fs, 'rgb'), MP.MP_EMISSIVE_COLOR)
    g.out(g.mask(fs, 'a'), MP.MP_OPACITY)
    g.save()
    # the dust in a beam (custom data: the place in its box 0-2, seed 3-6)
    g = Graph('/Game/Web/Base/M_WebMotes', B.BLEND_ADDITIVE, ism=True)
    ins = view(g) + [('P', g.cdata3(0)), ('Seed', g.cdata4(3)), ('Time', g.scalar('Time', 0)), ('Size', g.scalar('Size', 0.01)), ('Density', g.scalar('Density', 1)),
                     ('Wobble', g.scalar('Wobble', 0.05)), ('Fall', g.scalar('Fall', 0.1)), ('Puff', g.scalar('Puff', 2)), ('PuffSize', g.scalar('PuffSize', 1.4)),
                     ('PuffA', g.scalar('PuffA', 0.05)), ('Head', g.scalar('Head', 0)), ('Near', g.scalar('Near', 0.8)), ('Center', g.vector('Center', (0, 0, 0, 0))),
                     ('Box', g.vector('Box', (1, 1, 1, 0))), ('Drift', g.vector('Drift', (0, 0, 0, 0))), ('BeamPos0', g.vector('BeamPos0', (0, 0, 0, 0))), ('BeamPos1', g.vector('BeamPos1', (0, 0, 0, 0))),
                     ('BeamDir0', g.vector('BeamDir0', (0, 0, -1, 0))), ('BeamDir1', g.vector('BeamDir1', (0, 0, -1, 0))), ('BeamK', g.vector('BeamK', (0, 0, 0, 0))), ('Cos', g.vector('Cos', (0.92, 0.98, 0, 0)))]
    outs = [g.custom(MOTE_VS, ins + [('Out', g.const(o))], 4) for o in range(2)]
    g.out(g.mask(outs[0], 'rgb'), MP.MP_WORLD_POSITION_OFFSET)
    c = g.custom(MOTE_FS, [('Q', g.uv(0)), ('I', g.interp(outs[1])), ('Col', g.vector('Col', (1, 0.9, 0.75, 0)))])
    g.out(c, MP.MP_EMISSIVE_COLOR)
    g.save()
    log('the world\'s own life: sky dome, band, stars, meteor, cones, haze, window light, moths, motes')


def build_picture():
    t = fresh('/Game/Web/Base/M_WebTone')
    t.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)
    t.set_editor_property('blendable_location', unreal.BlendableLocation.BL_REPLACING_TONEMAPPER)
    tc = mel.create_material_expression(t, unreal.MaterialExpressionCustom, -400, 0)
    tc.set_editor_property('code', TONE); tc.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ins = []
    for nm in ('UV', 'K', 'Dens', 'FogCol', 'Time', 'Mode', 'Vhs', 'Strength', 'Calm', 'HazeK', 'Native', 'Dummy'):
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
    # (and the custom depth and stencil: the unfogged)
    cdn = mel.create_material_expression(t, unreal.MaterialExpressionSceneTexture, -800, 340)
    cdn.set_editor_property('scene_texture_id', unreal.SceneTextureId.PPI_CUSTOM_DEPTH)
    csn = mel.create_material_expression(t, unreal.MaterialExpressionSceneTexture, -800, 380)
    csn.set_editor_property('scene_texture_id', unreal.SceneTextureId.PPI_CUSTOM_STENCIL)
    dc = mel.create_material_expression(t, unreal.MaterialExpressionAdd, -600, 360)
    mel.connect_material_expressions(cdn, 'Color', dc, 'A'); mel.connect_material_expressions(csn, 'Color', dc, 'B')
    dm2 = mel.create_material_expression(t, unreal.MaterialExpressionAdd, -500, 300)
    mel.connect_material_expressions(dm, '', dm2, 'A'); mel.connect_material_expressions(dc, '', dm2, 'B')
    mel.connect_material_expressions(dm2, '', tc, 'Dummy')
    for k, (nm, pin) in enumerate((('ToneK', 'K'), ('FogDensity', 'Dens'), ('FogColor', 'FogCol'), ('PictureMode', 'Mode'), ('Vhs', 'Vhs'), ('PictureStrength', 'Strength'), ('PictureCalm', 'Calm'), ('HorizonHazeK', 'HazeK'), ('NativeLighting', 'Native'))):
        e = mel.create_material_expression(t, unreal.MaterialExpressionCollectionParameter, -800, 400 + k * 150)
        e.set_editor_property('collection', mpc); e.set_editor_property('parameter_name', nm)
        mel.connect_material_expressions(e, '', tc, pin)
    mel.connect_material_property(tc, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(t); eal.save_loaded_asset(t)
    log('film picture: Clean / VHS / Carpenter 35 mm / Documentary 16 mm / Super 8')


if only == ['picture']:
    build_picture()


# ---------- the sky pictures and the two special materials ----------
if only == ['life']:
    # (only the world's own life's materials, made again)
    build_life()
    log('done')
if 'sky' in only or 'mat' in only:
    import shutil
    tasks = [task(os.path.join(SRC, 'tex', v['file']), '/Game/Web/Tex', 'Sky_' + k) for k, v in (man.get('skies') or {}).items()]
    if tasks and not materials_only:
        tools.import_asset_tasks(tasks)
    for k in (() if materials_only else (man.get('skies') or {})):
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
    build_picture()
    log('sky, glow and tone materials')
    build_life()
    # the manifest goes with the build as a plain file
    if not materials_only:
        dst = os.path.join(unreal.Paths.project_content_dir(), 'WebData')
        os.makedirs(dst, exist_ok=True)
        shutil.copy(os.path.join(SRC, 'manifest.json'), os.path.join(dst, 'manifest.json'))
        log('manifest copied')
log('done')
