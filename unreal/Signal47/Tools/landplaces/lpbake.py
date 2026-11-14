"""LAND-PLACES kit, Blender side (5.2): meshes from lpkit's parts, weathered surfaces made of nodes over photo textures, the
bake of each atlas (base colour, normal, and one map of occlusion / roughness / metal), FBX export and look pictures.

The surfaces read where a point really is from the mesh attribute `rest` (metres, the model's own frame), so a part can be
moved away from the others for the bake (occlusion from its own shapes only: it will move in the game) and still take the
same streaks and dirt. `stain` and `tone` are per point: a mark the model itself lays down, and each piece's own age.
"""
import bpy, math, os
from pathlib import Path
import numpy as np

scn = None
SOCK = {}      # material -> its sockets for the bake
TEX = {}       # name -> path of a photo texture
SAMPLES = int(os.environ.get('S47_LP_SAMPLES', '48'))


def reset():
    global scn
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scn = bpy.context.scene
    SOCK.clear()
    return scn


def only(ob):
    bpy.context.view_layer.update()
    for o in bpy.context.view_layer.objects:
        if o:
            o.select_set(False)
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob


# ---------------------------------------------------------------- meshes
def mesh_object(name, geos, mats, offset_of=None):
    """One object from several (geo, atlas, pivot, group) rows: points less the pivot, plus offset_of(group) for a bake."""
    V, T, UV, MI, A, REST, sharp = [], [], [], [], [], [], []
    names = []
    for geo, atlas, pivot, group in geos:
        base = len(V)
        off = offset_of(group) if offset_of else (0.0, 0.0, 0.0)
        for v in geo.V:
            V.append((v[0] - pivot[0] + off[0], v[1] - pivot[1] + off[1], v[2] - pivot[2] + off[2]))
            REST.append(v)
        A.extend(geo.A)
        for t, uv, m in zip(geo.T, geo.UV, geo.M):
            T.append((t[0] + base, t[1] + base, t[2] + base))
            for ch, s, tt in uv:
                UV.extend(atlas.uv(ch, s, tt))
            if m not in names:
                names.append(m)
            MI.append(names.index(m))
        sharp.extend((a + base, b + base) for a, b in geo.sharp)
    me = bpy.data.meshes.new(name)
    me.from_pydata(V, [], T)
    me.update(calc_edges=True)
    me.uv_layers.new(name='UVMap').data.foreach_set('uv', UV)
    for m in names:
        me.materials.append(mats[m])
    me.polygons.foreach_set('material_index', MI)
    me.polygons.foreach_set('use_smooth', [True] * len(T))
    ev = np.zeros(len(me.edges) * 2, dtype=np.int64)
    me.edges.foreach_get('vertices', ev)
    ev = ev.reshape(-1, 2)
    nv = len(V)
    keys = np.minimum(ev[:, 0], ev[:, 1]) * nv + np.maximum(ev[:, 0], ev[:, 1])
    want = np.array([a * nv + b for a, b in sharp], dtype=np.int64)
    flags = np.isin(keys, want)
    me.attributes.new('sharp_edge', 'BOOLEAN', 'EDGE').data.foreach_set('value', flags.tolist())
    me.attributes.new('rest', 'FLOAT_VECTOR', 'POINT').data.foreach_set('vector', [c for v in REST for c in v])
    me.attributes.new('stain', 'FLOAT', 'POINT').data.foreach_set('value', [a[0] for a in A])
    me.attributes.new('tone', 'FLOAT', 'POINT').data.foreach_set('value', [a[1] for a in A])
    me.validate()
    me.update()
    ob = bpy.data.objects.new(name, me)
    scn.collection.objects.link(ob)
    return ob


# ---------------------------------------------------------------- nodes
def sock(node, ident, out=False):
    for s in (node.outputs if out else node.inputs):
        if s.identifier == ident:
            return s
    raise KeyError(ident + ' on ' + node.bl_idname + ': ' + str([s.identifier for s in (node.outputs if out else node.inputs)]))


def put(nt, value, target):
    if hasattr(value, 'is_output'):
        nt.links.new(value, target)
    elif isinstance(value, (tuple, list)):
        target.default_value = (value[0], value[1], value[2], 1.0) if len(target.default_value) == 4 else (value[0], value[1], value[2])
    else:
        target.default_value = value


def fmath(nt, op, a, b=None, clamp=False):
    n = nt.nodes.new('ShaderNodeMath')
    n.operation = op
    n.use_clamp = clamp
    put(nt, a, n.inputs[0])
    if b is not None:
        put(nt, b, n.inputs[1])
    return n.outputs[0]


def ramp(nt, value, lo, hi):
    n = nt.nodes.new('ShaderNodeMapRange')
    n.clamp = True
    put(nt, value, n.inputs[0])
    n.inputs[1].default_value, n.inputs[2].default_value, n.inputs[3].default_value, n.inputs[4].default_value = lo, hi, 0.0, 1.0
    return n.outputs[0]


def mixc(nt, fac, a, b, blend='MIX'):
    n = nt.nodes.new('ShaderNodeMix')
    n.data_type = 'RGBA'
    n.blend_type = blend
    n.clamp_factor = True
    put(nt, fac, sock(n, 'Factor_Float'))
    put(nt, a, sock(n, 'A_Color'))
    put(nt, b, sock(n, 'B_Color'))
    return sock(n, 'Result_Color', True)


def mixf(nt, fac, a, b):
    n = nt.nodes.new('ShaderNodeMix')
    n.data_type = 'FLOAT'
    n.clamp_factor = True
    put(nt, fac, sock(n, 'Factor_Float'))
    put(nt, a, sock(n, 'A_Float'))
    put(nt, b, sock(n, 'B_Float'))
    return sock(n, 'Result_Float', True)


def mapped(nt, coord, scale=(1, 1, 1), rot=(0, 0, 0)):
    m = nt.nodes.new('ShaderNodeMapping')
    m.inputs['Scale'].default_value = scale
    m.inputs['Rotation'].default_value = rot
    nt.links.new(coord, m.inputs['Vector'])
    return m.outputs[0]


def noise(nt, coord, scale, detail=5.0, rough=0.6, stretch=None):
    n = nt.nodes.new('ShaderNodeTexNoise')
    n.inputs['Scale'].default_value = scale
    n.inputs['Detail'].default_value = detail
    n.inputs['Roughness'].default_value = rough
    nt.links.new(mapped(nt, coord, stretch) if stretch else coord, n.inputs['Vector'])
    return n.outputs[0]


def image(nt, coord, path, size=1.0, color=True, turn=(0, 0, 0)):
    """A picture laid on from three sides by the point's own metres; size metres a repeat."""
    t = nt.nodes.new('ShaderNodeTexImage')
    t.image = bpy.data.images.load(str(path), check_existing=True)
    t.image.colorspace_settings.name = 'sRGB' if color else 'Non-Color'
    t.projection = 'BOX'
    t.projection_blend = 0.25
    nt.links.new(mapped(nt, coord, (1.0 / size,) * 3, turn), t.inputs['Vector'])
    return t.outputs['Color']


def bw(nt, col):
    n = nt.nodes.new('ShaderNodeRGBToBW')
    nt.links.new(col, n.inputs[0])
    return n.outputs[0]


def attr(nt, name, out='Fac'):
    n = nt.nodes.new('ShaderNodeAttribute')
    n.attribute_type = 'GEOMETRY'
    n.attribute_name = name
    return n.outputs[out]


def occl(nt, dist, samples=8, normal=None):
    n = nt.nodes.new('ShaderNodeAmbientOcclusion')
    n.samples = samples
    n.inputs['Distance'].default_value = dist
    if normal is not None:
        nt.links.new(normal, n.inputs['Normal'])
    return n.outputs['AO']


# ---------------------------------------------------------------- surfaces
def surface(name, kind, rust=0.5, tint=(1, 1, 1), ground=0.0, inner=None):
    """kind: galv (galvanized sheet and angle), cast (cast iron), wood, conc. rust: how far it has gone. ground: the height the
    dust of the ground reaches. inner: (x, y, radius, waterline) of a tank whose inside wall takes scale at the water."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    co = attr(nt, 'rest', 'Vector')
    tone = attr(nt, 'tone')
    stain = attr(nt, 'stain')
    geo = nt.nodes.new('ShaderNodeNewGeometry')
    nz = nt.nodes.new('ShaderNodeSeparateXYZ')
    nt.links.new(geo.outputs['Normal'], nz.inputs[0])
    pz = nt.nodes.new('ShaderNodeSeparateXYZ')
    nt.links.new(co, pz.inputs[0])
    up = nt.nodes.new('ShaderNodeVectorMath')
    up.operation = 'ADD'
    nt.links.new(geo.outputs['Normal'], up.inputs[0])
    up.inputs[1].default_value = (0, 0, 1.7)
    crev = ramp(nt, occl(nt, 0.022), 0.90, 0.38)                       # crevices and contacts
    under = ramp(nt, occl(nt, 0.16, 8, up.outputs[0]), 0.92, 0.30)     # what stands under something: where the rain runs off it
    streak = noise(nt, co, 1.0, 4.0, 0.55, (55.0, 55.0, 1.6))           # running down
    patch = noise(nt, co, 3.2, 5.0, 0.62)
    fine = noise(nt, co, 90.0, 3.0, 0.6)
    topface = ramp(nt, nz.outputs[2], 0.55, 0.95)
    dustk = fmath(nt, 'MAXIMUM', fmath(nt, 'MULTIPLY', topface, 0.55), ramp(nt, pz.outputs[2], ground, 0.02) if ground > 0 else 0.0)
    dustk = fmath(nt, 'MULTIPLY', dustk, fmath(nt, 'ADD', fmath(nt, 'MULTIPLY', patch, 0.6), 0.55), True)
    dust_col = (0.36, 0.29, 0.21)
    if kind in ('galv', 'cast'):
        g = image(nt, co, TEX['galv'], 0.42)
        lum = bw(nt, g)
        rpic = image(nt, co, TEX['rust'], 0.55)
        rlum = bw(nt, rpic)
        if kind == 'galv':
            base = mixc(nt, 0.30, g, lum)                                              # the zinc gone dull in the sun
            base = mixc(nt, 1.0, base, (0.80 * tint[0], 0.81 * tint[1], 0.82 * tint[2]), 'MULTIPLY')
            bloom = ramp(nt, noise(nt, co, 7.0, 5.0, 0.7), 0.52, 0.74)                 # white rust
            base = mixc(nt, fmath(nt, 'MULTIPLY', bloom, 0.45), base, (0.50, 0.51, 0.50))
            r0 = fmath(nt, 'ADD', fmath(nt, 'SUBTRACT', 0.58, fmath(nt, 'MULTIPLY', lum, 0.30)), fmath(nt, 'MULTIPLY', bloom, 0.2))
            m0 = fmath(nt, 'SUBTRACT', 0.82, fmath(nt, 'MULTIPLY', bloom, 0.35))
        else:
            cpic = image(nt, co, TEX['rust2'], 0.5)
            base = mixc(nt, 1.0, cpic, (0.30 * tint[0], 0.245 * tint[1], 0.215 * tint[2]), 'MULTIPLY')
            base = mixc(nt, ramp(nt, patch, 0.35, 0.7), base, mixc(nt, 1.0, g, (0.30, 0.31, 0.32), 'MULTIPLY'))   # what is left of its grey paint
            r0, m0 = 0.72, 0.35
        sore = ramp(nt, patch, 0.60, 0.80)
        run = fmath(nt, 'MULTIPLY', under, ramp(nt, streak, 0.38, 0.62))
        rm = fmath(nt, 'ADD', fmath(nt, 'ADD', crev, fmath(nt, 'MULTIPLY', run, 0.9)), fmath(nt, 'MULTIPLY', sore, 0.55))
        rm = fmath(nt, 'ADD', rm, fmath(nt, 'MULTIPLY', tone, 0.45))
        rm = fmath(nt, 'MULTIPLY', rm, ramp(nt, rlum, 0.62, 0.18))
        rm = fmath(nt, 'MULTIPLY', rm, 1.8 * rust, True)
        halo = fmath(nt, 'MULTIPLY', ramp(nt, fmath(nt, 'ADD', fmath(nt, 'ADD', crev, run), fmath(nt, 'MULTIPLY', sore, 0.6)), 0.05, 0.9), min(1.0, 0.55 * rust + 0.1), True)
        col = mixc(nt, halo, base, mixc(nt, 1.0, base, (0.86, 0.60, 0.40), 'MULTIPLY'))
        col = mixc(nt, rm, col, mixc(nt, 1.0, mixc(nt, 0.25, rpic, rlum), (0.38, 0.235, 0.16), 'MULTIPLY'))
        r = mixf(nt, rm, r0, 0.92)
        m = mixf(nt, rm, m0, 0.0)
        # what the model itself lays down: lime and rust under an overflow (0..1); the inside wall of a tank (2)
        inside = ramp(nt, stain, 1.4, 1.6)
        lime = fmath(nt, 'MULTIPLY', fmath(nt, 'MULTIPLY', ramp(nt, stain, 0.0, 1.0), fmath(nt, 'SUBTRACT', 1.0, inside)), ramp(nt, streak, 0.30, 0.60), True)
        col = mixc(nt, fmath(nt, 'MULTIPLY', lime, 0.85), col, mixc(nt, ramp(nt, noise(nt, co, 1.0, 3.0, 0.5, (90.0, 90.0, 2.5)), 0.45, 0.62), (0.80, 0.78, 0.71), (0.62, 0.36, 0.17)))
        r = mixf(nt, lime, r, 0.85)
        m = mixf(nt, lime, m, 0.05)
        if inner:
            band = fmath(nt, 'MULTIPLY', inside, ramp(nt, fmath(nt, 'ABSOLUTE', fmath(nt, 'SUBTRACT', pz.outputs[2], inner[3] + 0.02)), 0.07, 0.01), True)
            wet = fmath(nt, 'MULTIPLY', inside, ramp(nt, pz.outputs[2], inner[3] + 0.015, inner[3] - 0.01), True)
            col = mixc(nt, fmath(nt, 'MULTIPLY', band, 0.8), col, (0.78, 0.77, 0.70))
            col = mixc(nt, fmath(nt, 'MULTIPLY', wet, 0.7), col, (0.10, 0.12, 0.09))
            r = mixf(nt, wet, mixf(nt, band, r, 0.9), 0.25)
            m = mixf(nt, fmath(nt, 'MAXIMUM', band, wet), m, 0.1)
        height = fmath(nt, 'ADD', fmath(nt, 'MULTIPLY', lum, 0.55), fmath(nt, 'MULTIPLY', fmath(nt, 'MULTIPLY', rm, fine), 1.6))
        bump_d = 0.0006
    elif kind == 'wood':
        w = image(nt, co, TEX['wood'], 0.9, True, (0, 0, math.radians(90)))
        lum = bw(nt, w)
        col = mixc(nt, 1.0, mixc(nt, 0.45, w, lum), (1.10 * tint[0], 1.05 * tint[1], 0.95 * tint[2]), 'MULTIPLY')     # bleached grey by the sun
        col = mixc(nt, fmath(nt, 'MULTIPLY', crev, 0.7), col, (0.05, 0.04, 0.03))
        col = mixc(nt, fmath(nt, 'MULTIPLY', under, 0.18), col, (0.30, 0.17, 0.09))     # rust run down from its bolts
        r, m = fmath(nt, 'SUBTRACT', 0.95, fmath(nt, 'MULTIPLY', lum, 0.15)), 0.0
        height = fmath(nt, 'ADD', fmath(nt, 'MULTIPLY', lum, 1.3), fmath(nt, 'MULTIPLY', noise(nt, co, 14.0, 4.0, 0.6, (1.0, 30.0, 30.0)), 0.5))
        bump_d = 0.002
    else:   # conc
        c = image(nt, co, TEX['conc'], 0.8)
        lum = bw(nt, c)
        col = mixc(nt, 0.35, c, lum)
        col = mixc(nt, 1.0, col, (0.82 * tint[0], 0.77 * tint[1], 0.69 * tint[2]), 'MULTIPLY')
        col = mixc(nt, fmath(nt, 'MULTIPLY', ramp(nt, bw(nt, image(nt, co, TEX['conc2'], 1.1)), 0.55, 0.2), 0.5), col, (0.20, 0.17, 0.14))
        col = mixc(nt, fmath(nt, 'MULTIPLY', fmath(nt, 'MULTIPLY', under, ramp(nt, streak, 0.3, 0.6)), 0.75), col, (0.42, 0.24, 0.11))   # rust weeping from the foot plate
        r, m = fmath(nt, 'SUBTRACT', 0.97, fmath(nt, 'MULTIPLY', lum, 0.12)), 0.0
        height = fmath(nt, 'ADD', fmath(nt, 'MULTIPLY', lum, 1.6), fmath(nt, 'MULTIPLY', fine, 0.5))
        bump_d = 0.0025
    col = mixc(nt, fmath(nt, 'MULTIPLY', dustk, 0.75), col, dust_col)
    r = mixf(nt, dustk, r, 0.95)
    m = fmath(nt, 'MULTIPLY', m, fmath(nt, 'SUBTRACT', 1.0, fmath(nt, 'MULTIPLY', dustk, 0.85)), True)
    tonev = fmath(nt, 'ADD', fmath(nt, 'MULTIPLY', noise(nt, co, 1.3, 3.0, 0.5), 0.22), fmath(nt, 'SUBTRACT', 0.93, fmath(nt, 'MULTIPLY', tone, 0.12)))
    col = mixc(nt, 1.0, col, tonev, 'MULTIPLY')
    ao = ramp(nt, occl(nt, 0.45, 16), 0.0, 1.0)
    bump = nt.nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = 0.6
    bump.inputs['Distance'].default_value = bump_d
    nt.links.new(height, bump.inputs['Height'])
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    nt.links.new(col, bsdf.inputs['Base Color'])
    put(nt, r, bsdf.inputs['Roughness'])
    put(nt, m, bsdf.inputs['Metallic'])
    nt.links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    emit = nt.nodes.new('ShaderNodeEmission')
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    nt.links.new(bsdf.outputs[0], out.inputs['Surface'])
    img = nt.nodes.new('ShaderNodeTexImage')
    SOCK[mat] = {'basecolor': col, 'roughness': r, 'metallic': m, 'ao': ao, 'emit': emit, 'bsdf': bsdf, 'out': out, 'img': img, 'nt': nt}
    return mat


def plain(name, color, rough=0.5, metal=0.0, alpha=1.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    b = next(n for n in mat.node_tree.nodes if n.bl_idname == 'ShaderNodeBsdfPrincipled')
    b.inputs['Base Color'].default_value = (color[0], color[1], color[2], 1)
    b.inputs['Roughness'].default_value = rough
    b.inputs['Metallic'].default_value = metal
    b.inputs['Alpha'].default_value = alpha
    return mat


def pictured(name, path, rough=0.6, alpha=False):
    """A material that shows one picture by the mesh's own UVs (labels)."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    b = next(n for n in nt.nodes if n.bl_idname == 'ShaderNodeBsdfPrincipled')
    t = nt.nodes.new('ShaderNodeTexImage')
    t.image = bpy.data.images.load(str(path), check_existing=True)
    nt.links.new(t.outputs['Color'], b.inputs['Base Color'])
    if alpha:
        nt.links.new(t.outputs['Alpha'], b.inputs['Alpha'])
    b.inputs['Roughness'].default_value = rough
    return mat


# ---------------------------------------------------------------- bake
def cycles(samples=None):
    scn.render.engine = 'CYCLES'
    scn.cycles.samples = samples or SAMPLES
    try:
        prefs = bpy.context.preferences.addons['cycles'].preferences
        for kind in ('OPTIX', 'CUDA'):
            try:
                prefs.compute_device_type = kind
                prefs.get_devices()
                if any(d.type == kind for d in prefs.devices):
                    for d in prefs.devices:
                        d.use = d.type == kind
                    scn.cycles.device = 'GPU'
                    return kind
            except Exception as exc:
                print('S47 LP: no', kind, exc)
    except Exception as exc:
        print('S47 LP: cycles on the CPU:', exc)
    return 'CPU'


def pixels(im):
    a = np.empty(im.size[0] * im.size[1] * 4, dtype=np.float32)
    im.pixels.foreach_get(a)
    return a.reshape(im.size[1], im.size[0], 4)


def save_png(name, arr, path, space='Non-Color'):
    h, w = arr.shape[:2]
    im = bpy.data.images.new(name, w, h, alpha=False)
    im.colorspace_settings.name = space
    im.pixels.foreach_set(arr.astype(np.float32).ravel())
    im.filepath_raw = str(path)
    im.file_format = 'PNG'
    im.save()
    return im


def bake(ob, key, size, outdir, final_name):
    """ob's surfaces into <outdir>/T_LP_<key>_BC.png, _N.png and _ORM.png (occlusion, roughness, metal); returns the material that
    shows them."""
    print('S47 LP: bake', key, size, cycles(), flush=True)
    scn.render.bake.margin = max(4, size // 512)
    scn.render.bake.use_clear = True
    scn.render.bake.use_selected_to_active = False
    only(ob)
    done = {}
    for k in ('basecolor', 'roughness', 'metallic', 'ao', 'normal'):
        im = bpy.data.images.new(key + '_' + k, size, size, alpha=False, float_buffer=(k != 'basecolor' and k != 'normal'))
        im.colorspace_settings.name = 'sRGB' if k == 'basecolor' else 'Non-Color'
        for mat, row in SOCK.items():
            nt = row['nt']
            row['img'].image = im
            for n in nt.nodes:
                n.select = False
            row['img'].select = True
            nt.nodes.active = row['img']
            for link in list(row['out'].inputs['Surface'].links):
                nt.links.remove(link)
            if k == 'normal':
                nt.links.new(row['bsdf'].outputs[0], row['out'].inputs['Surface'])
            else:
                for link in list(row['emit'].inputs['Color'].links):
                    nt.links.remove(link)
                put(nt, row[k], row['emit'].inputs['Color'])
                nt.links.new(row['emit'].outputs[0], row['out'].inputs['Surface'])
        scn.cycles.samples = SAMPLES if k == 'ao' else max(4, SAMPLES // 6)
        if k == 'normal':
            bpy.ops.object.bake(type='NORMAL', normal_space='TANGENT', normal_r='POS_X', normal_g='NEG_Y', normal_b='POS_Z')   # DirectX green
        else:
            bpy.ops.object.bake(type='EMIT')
        done[k] = im
        print('S47 LP: baked', key, k, flush=True)
    outdir = Path(outdir)
    files = {}
    for k, suffix in (('basecolor', 'BC'), ('normal', 'N')):
        im = done[k]
        im.filepath_raw = str(outdir / ('T_LP_%s_%s.png' % (key, suffix)))
        im.file_format = 'PNG'
        im.save()
        files[suffix] = im
    orm = np.ones((size, size, 4), dtype=np.float32)
    orm[:, :, 0] = pixels(done['ao'])[:, :, 0]
    orm[:, :, 1] = pixels(done['roughness'])[:, :, 0]
    orm[:, :, 2] = pixels(done['metallic'])[:, :, 0]
    files['ORM'] = save_png(key + '_orm', np.clip(orm, 0, 1), outdir / ('T_LP_%s_ORM.png' % key))
    for mat, row in SOCK.items():                      # back as it was, for look pictures of the surfaces themselves
        nt = row['nt']
        for link in list(row['out'].inputs['Surface'].links):
            nt.links.remove(link)
        nt.links.new(row['bsdf'].outputs[0], row['out'].inputs['Surface'])
    return baked_material(final_name, files)


def baked_material(name, files):
    final = bpy.data.materials.new(name)
    final.use_nodes = True
    nt = final.node_tree
    bsdf = next(n for n in nt.nodes if n.bl_idname == 'ShaderNodeBsdfPrincipled')
    t = nt.nodes.new('ShaderNodeTexImage')
    t.image = files['BC']
    o = nt.nodes.new('ShaderNodeTexImage')
    o.image = files['ORM']
    sep = nt.nodes.new('ShaderNodeSeparateColor')
    nt.links.new(o.outputs['Color'], sep.inputs[0])
    nt.links.new(mixc(nt, 1.0, t.outputs['Color'], sep.outputs[0], 'MULTIPLY'), bsdf.inputs['Base Color'])
    nt.links.new(sep.outputs[1], bsdf.inputs['Roughness'])
    nt.links.new(sep.outputs[2], bsdf.inputs['Metallic'])
    n = nt.nodes.new('ShaderNodeTexImage')
    n.image = files['N']
    s2 = nt.nodes.new('ShaderNodeSeparateColor')
    back = nt.nodes.new('ShaderNodeCombineColor')
    nt.links.new(n.outputs['Color'], s2.inputs[0])
    nt.links.new(s2.outputs[0], back.inputs[0])
    nt.links.new(fmath(nt, 'SUBTRACT', 1.0, s2.outputs[1]), back.inputs[1])
    nt.links.new(s2.outputs[2], back.inputs[2])
    nm = nt.nodes.new('ShaderNodeNormalMap')
    nt.links.new(back.outputs[0], nm.inputs['Color'])
    nt.links.new(nm.outputs['Normal'], bsdf.inputs['Normal'])
    return final


# ---------------------------------------------------------------- export and pictures
def export(ob, path):
    only(ob)
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH'}, apply_unit_scale=True,
                             apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Y', axis_up='Z', bake_anim=False, add_leaf_bones=False,
                             mesh_smooth_type='FACE', use_tspace=True)


def camera(ortho=None):
    cd = bpy.data.cameras.new('c')
    cam = bpy.data.objects.new('c', cd)
    scn.collection.objects.link(cam)
    scn.camera = cam
    if ortho:
        cd.type = 'ORTHO'
        cd.ortho_scale = ortho
    cd.clip_start, cd.clip_end = 0.05, 4000
    return cam


def look(cam, eye, at, lens=None):
    from mathutils import Vector
    cam.location = eye
    d = Vector(at) - Vector(eye)
    cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    if lens:
        cam.data.lens = lens


def daylight(sun_deg=42, turn_deg=215, strength=3.2, sky=0.9):
    w = bpy.data.worlds.new('w')
    w.use_nodes = True
    w.node_tree.nodes['Background'].inputs[0].default_value = (0.55, 0.66, 0.85, 1)
    w.node_tree.nodes['Background'].inputs[1].default_value = sky
    scn.world = w
    sd = bpy.data.lights.new('sun', 'SUN')
    sd.energy = strength
    sd.angle = math.radians(1.0)
    so = bpy.data.objects.new('sun', sd)
    scn.collection.objects.link(so)
    so.rotation_euler = (math.radians(90 - sun_deg), 0, math.radians(turn_deg))
    return so


def render(path, res=(1600, 900), samples=48, clay=None):
    cycles(samples)
    scn.render.resolution_x, scn.render.resolution_y = res
    scn.render.image_settings.file_format = 'PNG'
    scn.render.film_transparent = False
    try:
        scn.view_settings.view_transform = 'AgX'
    except Exception:
        pass
    bpy.context.view_layer.material_override = clay
    scn.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    bpy.context.view_layer.material_override = None
