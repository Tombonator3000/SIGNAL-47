"""EASTWING-01 extras (Claude): sink, glass cabinet, posters, calendar, wall clock, first-aid box.
Fills the gaps the first prop delivery left open (Props/README.md). Original geometry; printed
surfaces come from make_prints.py. Run under run_guarded.py (production lease, require-idle).
Coordinates: metres, Z up, front -Y, base-centred origin.
"""
from pathlib import Path
import math
import sys
import bpy

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from prop_common import (material, box, cyl, rod, text, start_asset, finish_asset, render_catalog)

OUT = HERE
PRINTS = HERE / 'textures'
SOURCE = 'unreal/ArtSource/EastWing/Props/Extras/build_extras.py'

def pbr(name, maps=(('BaseColor', 'Base Color'), ('Roughness', 'Roughness'), ('NormalGL', 'Normal')), span=None):
    """Material from authored PBR images in Extras/textures (BaseColor sRGB; Roughness, NormalGL linear)."""
    m = bpy.data.materials.get(name)
    if m:
        return m
    m = bpy.data.materials.new(name); m.use_nodes = True
    if span: m['x_span'] = span  # world-planar UVs, metres per texture (see world_uv)
    bs = m.node_tree.nodes.get('Principled BSDF'); bs.inputs['Metallic'].default_value = 0
    for suffix, socket in maps:
        im = bpy.data.images.load(str(PRINTS / (name + '_' + suffix + '.png')), check_existing=True)
        im.colorspace_settings.name = 'sRGB' if suffix == 'BaseColor' else 'Non-Color'
        n = m.node_tree.nodes.new('ShaderNodeTexImage'); n.image = im
        if suffix == 'NormalGL':
            nm = m.node_tree.nodes.new('ShaderNodeNormalMap'); m.node_tree.links.new(n.outputs['Color'], nm.inputs['Color'])
            m.node_tree.links.new(nm.outputs['Normal'], bs.inputs['Normal'])
        else:
            m.node_tree.links.new(n.outputs['Color'], bs.inputs[socket])
    return m


import prop_common as PC
_authored_finish = finish_asset


def world_uv():
    """Photo pass: polygons whose material carries x_span get box-projected world-metre UVs,
    so wood grain and paint wear keep physical scale on every scaled primitive."""
    for o in bpy.context.scene.objects:
        if o.as_pointer() in PC.BEFORE or o.type != 'MESH':
            continue
        spans = [m.get('x_span') if m else None for m in o.data.materials]
        if not any(spans):
            continue
        if not o.data.uv_layers:
            o.data.uv_layers.new(name='UVMap')
        uv = o.data.uv_layers.active.data; mw = o.matrix_world; rot = mw.to_3x3()
        for poly in o.data.polygons:
            span = spans[poly.material_index] if poly.material_index < len(spans) else None
            if not span:
                continue
            n = rot @ poly.normal; axis = max(range(3), key=lambda k: abs(n[k])); a, b = [k for k in range(3) if k != axis]
            for li in poly.loop_indices:
                co = mw @ o.data.vertices[o.data.loops[li].vertex_index].co
                uv[li].uv = (co[a] / span, co[b] / span)


def finish_asset(name, outdir, description, *args, **kw):
    world_uv()
    return _authored_finish(name, outdir, description, *args, **kw)


green = pbr('EW_XEnamelGreen', span=.5)  # Codex ImageGen grey-green enamel (reference casework)
steel = pbr('EW_XStainless', span=.5)
steel.node_tree.nodes.get('Principled BSDF').inputs['Metallic'].default_value = .9
chrome = material('EW_X_Chrome', (.7, .73, .74), .17, 1)
dark = material('EW_X_Bakelite', (.017, .02, .019), .39)
rubber = pbr('EW_XRubber', span=.5)
cream = material('EW_X_CreamEnamel', (.69, .66, .53), .3)
white = pbr('EW_XEnamelWhite', span=.5)
red = material('EW_X_Red', (.38, .025, .012), .38)
paper = material('EW_X_LabelPaper', (.81, .78, .63), .8)
ink = material('EW_X_LabelInk', (.025, .032, .028), .7)
card = material('EW_X_GreyCard', (.36, .34, .3), .85)
brass = material('EW_X_Brass', (.55, .42, .17), .3, 1)
glass = material('EW_X_Glass', (.80, .86, .84), .06)
amber = material('EW_X_AmberGlass', (.24, .08, .016), .18)
for m, t in ((glass, .92), (amber, .5)):
    bs = m.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Transmission Weight'].default_value = t
    bs.inputs['IOR'].default_value = 1.47


def printed(name):
    """A matte print whose BaseColor is an authored image (finish_asset records the map)."""
    m = bpy.data.materials.get(name)
    if m:
        return m
    m = bpy.data.materials.new(name); m.use_nodes = True
    bs = m.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Roughness'].default_value = .82; bs.inputs['Metallic'].default_value = 0
    im = bpy.data.images.load(str(PRINTS / (name + '_BaseColor.png')), check_existing=True)
    im.colorspace_settings.name = 'sRGB'
    n = m.node_tree.nodes.new('ShaderNodeTexImage'); n.image = im
    m.node_tree.links.new(n.outputs['Color'], bs.inputs['Base Color'])
    m.diffuse_color = (.8, .78, .7, 1)
    return m


def sheet(name, centre, width, height, mat):
    """A flat sheet facing -Y with 0..1 UVs, image upright."""
    bpy.ops.mesh.primitive_plane_add(size=1, location=centre, rotation=(math.pi / 2, 0, 0))
    o = bpy.context.object; o.scale = (width, height, 1)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    o.name = name; o.data.materials.append(mat)
    return o


def disc(name, centre, radius, mat, segments=64):
    """A front-facing disc with a circular UV that maps the whole square image."""
    cx, cy, cz = centre
    verts = [(cx, cy, cz)] + [(cx + radius * math.cos(2 * math.pi * i / segments), cy, cz + radius * math.sin(2 * math.pi * i / segments)) for i in range(segments)]
    faces = [(0, 1 + i, 1 + (i + 1) % segments) for i in range(segments)]  # front (-Y) facing
    me = bpy.data.meshes.new(name); me.from_pydata(verts, [], faces); me.update()
    uv = me.uv_layers.new(name='UVMap')
    for poly in me.polygons:
        for li in poly.loop_indices:
            x, _, z = me.vertices[me.loops[li].vertex_index].co
            uv.data[li].uv = (.5 + (x - cx) / (2 * radius), .5 + (z - cz) / (2 * radius))
    o = bpy.data.objects.new(name, me); bpy.context.collection.objects.link(o)
    me.materials.append(mat)
    return o


def cone(name, loc, r1, r2, depth, mat, vertices=40):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=r1, radius2=r2, depth=depth, location=loc)
    o = bpy.context.object; o.name = name; o.data.materials.append(mat)
    for p in o.data.polygons:
        p.use_smooth = len(p.vertices) == 4
    return o


def label(name, body, x, y, z, w, h, size=None):
    box(name + '_paper', (x, y, z), (w, .0012, h), paper, .0006)
    text(name + '_ink', body, (x, y - .0011, z), size or h * .42, ink)


def bottle(name, x, y, z, r, h, mat, cap=dark, tag=None):
    cyl(name + '_body', (x, y, z + h * .36), r, h * .72, mat, vertices=28)
    cone(name + '_shoulder', (x, y, z + h * .78), r, r * .38, h * .12, mat, vertices=28)
    cyl(name + '_neck', (x, y, z + h * .89), r * .36, h * .1, mat, vertices=20)
    cyl(name + '_cap', (x, y, z + h * .97), r * .42, h * .06, cap, vertices=20)
    if tag:
        label(name + '_tag', tag, x, y - r - .0008, z + h * .38, r * 1.3, h * .26, h * .06)


# 1. Lab sink: green steel base cabinet, stainless top with an inset bowl, gooseneck faucet.
def lab_sink():
    name = 'SM_EW_LabSink'; start_asset(name)
    W, D, H = 1.20, .62, .90
    box('plinth', (0, .02, .05), (W - .04, D - .08, .10), rubber, .004)
    box('carcass', (0, 0, .10 + (H - .14) / 2), (W, D - .02, H - .14), green, .006)
    for s in (-1, 1):
        x = s * W / 4
        box(f'door{s}', (x, -D / 2 + .004, .47), (W / 2 - .03, .012, .60), green, .004)
        box(f'door{s}_seam', (x, -D / 2 - .001, .47), (W / 2 - .07, .0015, .56), dark, 0)
        box(f'door{s}_face', (x, -D / 2 - .002, .47), (W / 2 - .08, .0015, .55), green, .002)
        rod(f'handle{s}', (x - s * .2, -D / 2 - .032, .62), (x - s * .2, -D / 2 - .032, .72), .007, chrome)
        for zz in (.62, .72):
            rod(f'handle{s}_post{zz}', (x - s * .2, -D / 2 - .002, zz), (x - s * .2, -D / 2 - .032, zz), .005, chrome)
    # Top with a 0.50 x 0.38 opening; the bowl hangs 0.20 below it.
    T, BX, BY, BD = .035, .50, .38, .20
    z = H - T / 2
    box('top_back', (0, (BY / 2 + D / 2) / 2 + .01, z), (W + .02, D / 2 - BY / 2 + .02, T), steel, .004)
    box('top_front', (0, -(BY / 2 + D / 2) / 2 - .01, z), (W + .02, D / 2 - BY / 2 + .02, T), steel, .004)
    for s in (-1, 1):
        box(f'top_side{s}', (s * (BX / 2 + (W / 2 - BX / 2) / 2 + .005), 0, z), (W / 2 - BX / 2 + .01, BY, T), steel, .004)
    box('bowl_floor', (0, 0, H - BD), (BX, BY, .008), steel, .003)
    for s in (-1, 1):
        box(f'bowl_wall_x{s}', (s * BX / 2, 0, H - BD / 2), (.008, BY, BD), steel, .002)
        box(f'bowl_wall_y{s}', (0, s * BY / 2, H - BD / 2), (BX, .008, BD), steel, .002)
    cyl('drain', (0, .04, H - BD + .006), .035, .006, chrome)
    for i in range(6):
        box(f'drain_slot{i}', (-.02 + i * .008, .04, H - BD + .0095), (.003, .045, .0012), dark, 0)
    box('splash', (0, D / 2 - .005, H + .10), (W + .02, .012, .20), steel, .004)
    # Gooseneck faucet and two cross taps.
    fx, fy = 0, D / 2 - .07
    cyl('faucet_base', (fx, fy, H + .01), .028, .02, chrome)
    rod('faucet_riser', (fx, fy, H + .01), (fx, fy, H + .30), .011, chrome)
    prev = (fx, fy, H + .30)
    for i in range(1, 9):
        a = math.pi * i / 8
        p = (fx, fy - .11 * (1 - math.cos(a)), H + .30 + .11 * math.sin(a))
        rod(f'faucet_neck{i}', prev, p, .011, chrome); prev = p
    cyl('faucet_tip', (fx, prev[1], prev[2] - .025), .012, .05, chrome)
    for s in (-1, 1):
        tx = s * .16
        cyl(f'tap{s}_body', (tx, fy, H + .03), .018, .05, chrome)
        for a in (0, math.pi / 2):
            rod(f'tap{s}_arm{a}', (tx - .035 * math.cos(a), fy - .035 * math.sin(a), H + .062), (tx + .035 * math.cos(a), fy + .035 * math.sin(a), H + .062), .006, chrome)
        cyl(f'tap{s}_index', (tx, fy, H + .066), .008, .006, red if s < 0 else material('EW_X_Blue', (.03, .08, .3), .35))
    label('notice', 'NO SOLVENTS\nOR MERCURY\nIN THIS DRAIN', .42, D / 2 - .012, H + .12, .16, .11, .016)
    return finish_asset(name, OUT, 'Green steel lab sink cabinet, 1.20 m, stainless top with inset bowl, gooseneck faucet and cross taps.')


# 2. Glass-front chemical/glassware cabinet.
def glass_cabinet():
    name = 'SM_EW_GlassCabinet'; start_asset(name)
    W, D, H = .90, .42, 1.95
    box('plinth', (0, .01, .04), (W - .03, D - .05, .08), dark, .003)
    box('back', (0, D / 2 - .01, H / 2 + .04), (W, .02, H - .08), green, .004)
    for s in (-1, 1):
        box(f'side{s}', (s * (W / 2 - .01), 0, H / 2 + .04), (.02, D, H - .08), green, .004)
    box('top', (0, 0, H - .02), (W + .02, D + .02, .04), green, .006)
    box('bottom', (0, 0, .09), (W, D, .02), green, .004)
    box('cornice', (0, -D / 2 - .004, H - .07), (W, .01, .06), green, .003)
    shelves = [.42, .80, 1.18, 1.54]
    for i, z in enumerate(shelves):
        box(f'shelf{i}', (0, .01, z), (W - .05, D - .07, .012), steel, .002)
    # Contents, deterministic: bottles, amber reagents, flasks and beakers on each shelf.
    contents = [
        [('b', 'HCl 1N'), ('a', 'KOH'), ('b', 'H2O DIST'), ('a', 'IPA'), ('b', 'FLUX REM')],
        [('f', None), ('k', None), ('f', None), ('k', None), ('f', None)],
        [('a', 'ACETONE'), ('a', 'ETHANOL'), ('b', 'SILVER'), ('box', 'FILTER\nPAPER')],
        [('k', None), ('k', None), ('box', 'SAMPLE\nVIALS'), ('box', 'SLIDES')],
        [('b', 'BUFFER'), ('a', 'DEVELOPER'), ('a', 'FIXER'), ('b', 'STOP')],
    ]
    floors = [.10] + [z + .006 for z in shelves]
    for s, items in enumerate(contents):
        n = len(items); z0 = floors[s]
        for i, (kind, tag) in enumerate(items):
            x = -W / 2 + .1 + i * (W - .2) / max(1, n - 1)
            y = .02 + (i % 2) * .05
            nm = f'c{s}_{i}'
            if kind == 'b':
                bottle(nm, x, y, z0, .038, .21, glass, dark, tag)
            elif kind == 'a':
                bottle(nm, x, y, z0, .034, .19, amber, dark, tag)
            elif kind == 'f':
                cone(nm + '_flask', (x, y, z0 + .07), .055, .016, .14, glass)
                cyl(nm + '_neck', (x, y, z0 + .17), .016, .06, glass, vertices=20)
            elif kind == 'k':
                cyl(nm + '_beaker', (x, y, z0 + .06), .04, .12, glass, vertices=28)
                cyl(nm + '_water', (x, y, z0 + .035), .036, .06, material('EW_X_Liquid', (.4, .5, .45), .05), vertices=28)
            else:
                box(nm + '_carton', (x, y, z0 + .07), (.15, .11, .14), card, .002)
                label(nm + '_lbl', tag, x, y - .056, z0 + .08, .11, .06, .012)
    # Two glazed doors in steel frames, a chrome pull each and a cylinder lock.
    for s in (-1, 1):
        cx = s * (W / 4 - .002); fw, fh, fz = W / 2 - .03, H - .25, (H - .25) / 2 + .13
        y = -D / 2 - .008
        for dz in (-1, 1):
            box(f'door{s}_rail{dz}', (cx, y, fz + dz * (fh / 2 - .025)), (fw, .02, .05), green, .003)
        for dx in (-1, 1):
            box(f'door{s}_stile{dx}', (cx + dx * (fw / 2 - .025), y, fz), (.05, .02, fh), green, .003)
        box(f'door{s}_glass', (cx, y, fz), (fw - .05, .005, fh - .05), glass, 0)
        rod(f'door{s}_pull', (cx - s * (fw / 2 - .05), y - .03, fz - .08), (cx - s * (fw / 2 - .05), y - .03, fz + .08), .006, chrome)
        for zz in (fz - .08, fz + .08):
            rod(f'door{s}_pull_post{zz}', (cx - s * (fw / 2 - .05), y - .01, zz), (cx - s * (fw / 2 - .05), y - .03, zz), .004, chrome)
    cyl('lock', (.03, -D / 2 - .02, 1.0), .011, .01, brass, rotation=(math.pi / 2, 0, 0))
    label('class', 'GLASSWARE & REAGENTS\nKEEP DOORS CLOSED', 0, -D / 2 - .01, H - .07, .36, .045, .013)
    return finish_asset(name, OUT, 'Green steel glazed cabinet, 1.95 m, glassware and labelled reagent bottles on four shelves.')


def poster(asset, print_name, w, h, pins=red):
    start_asset(asset)
    box('backing', (0, .0015, h / 2), (w + .006, .002, h + .006), card, 0)
    sheet('print', (0, -.0002, h / 2), w, h, printed(print_name))
    for sx in (-1, 1):
        for sz in (0, 1):
            x, z = sx * (w / 2 - .02), .02 + sz * (h - .04)
            cyl(f'pin{sx}{sz}', (x, -.004, z), .006, .006, pins, vertices=16, rotation=(math.pi / 2, 0, 0))
    return finish_asset(asset, OUT, f'Wall print {print_name} ({w:.2f} x {h:.2f} m) on grey card with four push pins.')


def calendar_1986():
    asset = 'SM_EW_Calendar1986'; start_asset(asset)
    w, h = .30, .45
    sheet('pages', (0, -.0004, h / 2), w, h, printed('EW_Print_Calendar1986'))
    for i in range(1, 4):
        box(f'under{i}', (0, .0006 * i, h / 2 - .002 * i), (w - .002 * i, .0006, h - .004 * i), paper, 0)
    box('binding', (0, -.003, h - .01), (w + .004, .008, .02), dark, .002)
    for i in range(12):
        cyl(f'coil{i}', (-w / 2 + .02 + i * (w - .04) / 11, -.003, h + .002), .004, .012, chrome, vertices=12)
    rod('string_l', (-.06, -.002, h + .004), (0, -.002, h + .06), .0012, ink)
    rod('string_r', (.06, -.002, h + .004), (0, -.002, h + .06), .0012, ink)
    cyl('nail', (0, -.006, h + .062), .004, .014, steel, vertices=12, rotation=(math.pi / 2, 0, 0))
    return finish_asset(asset, OUT, 'Hanging feed-store wall calendar, November 1986, with shift notes in ballpoint.')


def wall_clock():
    asset = 'SM_EW_WallClock'; start_asset(asset)
    r, cz = .16, .17
    cyl('housing', (0, .012, cz), r, .045, dark, vertices=64, rotation=(math.pi / 2, 0, 0))
    disc('face', (0, -.0112, cz), r - .012, printed('EW_Print_ClockFace'))
    bpy.ops.mesh.primitive_torus_add(major_radius=r - .002, minor_radius=.009, major_segments=64, minor_segments=12, location=(0, -.012, cz), rotation=(math.pi / 2, 0, 0))
    bz = bpy.context.object; bz.name = 'bezel'; bz.data.materials.append(chrome)
    for p in bz.data.polygons: p.use_smooth = True
    disc('crystal', (0, -.019, cz), r - .008, glass)
    # Hands at 03:14, sweep second at 40 s; slight stand-off from the face.
    for hn, ang, ln, wd, dy in (('hour', 3.23 * 30, .075, .012, -.013), ('minute', 14 * 6, .115, .008, -.0145), ('second', 40 * 6, .125, .0025, -.016)):
        a = math.radians(ang)
        hand = box('hand_' + hn, (0, dy, cz), (wd, .0012, ln), red if hn == 'second' else dark, 0)
        hand.location = (math.sin(a) * ln * .42, dy, cz + math.cos(a) * ln * .42); hand.rotation_euler = (0, a, 0)
    cyl('hub', (0, -.017, cz), .008, .004, dark, vertices=16, rotation=(math.pi / 2, 0, 0))
    # The bounding origin is the clock's lowest point so it mounts like the other wall items.
    return finish_asset(asset, OUT, 'Black bakelite 32 cm wall clock, chrome bezel, printed SARO standard-time dial.')


def first_aid():
    asset = 'SM_EW_FirstAidBox'; start_asset(asset)
    w, d, h = .36, .13, .30
    box('body', (0, 0, h / 2), (w, d, h), white, .008)
    box('lid_seam', (0, -d / 2 - .0005, h / 2), (w - .03, .001, h - .03), dark, 0)
    box('lid', (0, -d / 2 - .002, h / 2), (w - .036, .002, h - .036), white, .002)
    box('cross_v', (0, -d / 2 - .004, h * .55), (.05, .002, .15), red, .001)
    for sx in (-1, 1):
        box(f'cross_h{sx}', (sx * .05, -d / 2 - .004, h * .55), (.05, .002, .05), red, .001)
    label('title', 'FIRST AID', 0, -d / 2 - .0035, h * .2, .16, .035, .02)
    cyl('latch', (w / 2 - .035, -d / 2 - .008, h / 2), .009, .012, chrome, vertices=16, rotation=(math.pi / 2, 0, 0))
    for z in (.06, h - .06):
        cyl(f'hinge{z}', (-w / 2 + .01, -d / 2 - .003, z), .006, .035, chrome, vertices=12)
    return finish_asset(asset, OUT, 'Wall-mounted white enamel steel first-aid cabinet with red cross.')


# 8. Rooftop evaporative cooler: galvanized cabinet, louvered pad sides, down-duct and stand.
def swamp_cooler():
    asset = 'SM_EW_SwampCooler'; start_asset(asset)
    W, D, H, z0 = 1.0, .95, .85, .25
    for sx in (-1, 1):
        for sy in (-1, 1):
            box(f'leg{sx}{sy}', (sx * (W / 2 - .05), sy * (D / 2 - .05), z0 / 2), (.05, .05, z0), steel, .003)
    box('base_pan', (0, 0, z0 + .03), (W, D, .06), steel, .004)
    box('lid', (0, 0, z0 + H - .02), (W + .04, D + .04, .05), steel, .006)
    box('lid_ridge', (0, 0, z0 + H + .015), (W - .2, D - .2, .02), steel, .004)
    for side, (ax, length) in enumerate([('x', W), ('y', D), ('x', W), ('y', D)]):
        sgn = -1 if side < 2 else 1
        for k in range(14):
            z = z0 + .12 + k * (H - .2) / 14
            if ax == 'x':
                o = box(f'louver{side}_{k}', (0, sgn * D / 2, z), (length - .08, .045, .012), steel, .001)
                o.rotation_euler = (math.radians(35 * sgn), 0, 0)
            else:
                o = box(f'louver{side}_{k}', (sgn * W / 2, 0, z), (.045, length - .08, .012), steel, .001)
                o.rotation_euler = (0, math.radians(-35 * sgn), 0)
        if ax == 'x':
            box(f'pad{side}', (0, sgn * (D / 2 - .03), z0 + H / 2), (length - .1, .03, H - .14), material('EW_X_AspenPad', (.42, .33, .2), .95), 0)
        else:
            box(f'pad{side}', (sgn * (W / 2 - .03), 0, z0 + H / 2), (.03, length - .1, H - .14), material('EW_X_AspenPad', (.42, .33, .2), .95), 0)
    for sx in (-1, 1):
        for sy in (-1, 1):
            box(f'corner{sx}{sy}', (sx * W / 2, sy * D / 2, z0 + H / 2), (.04, .04, H), steel, .003)
    box('duct', (0, D / 2 + .25, .18), (.5, .5, .36), steel, .004)
    box('duct_flange', (0, D / 2 + .25, .02), (.6, .6, .04), steel, .003)
    box('rust_streak', (.2, -D / 2 - .024, z0 + .2), (.12, .002, .3), material('EW_X_Rust', (.28, .12, .05), .9), 0)
    label('plate', 'SARO B-2\nAIR COOLER', -.3, -D / 2 - .03, z0 + .08, .14, .05, .011)
    return finish_asset(asset, OUT, 'Rooftop evaporative (swamp) cooler, galvanized, louvered aspen-pad sides, curb duct and stand.')


# 9. (Not built: the east wall already carries the retained meter and disconnect.)
# Exterior electrical service: meter base with glass dome, fused disconnect, conduit to the roof.
def electrical_service():
    asset = 'SM_EW_ElectricalService'; start_asset(asset)
    grey = material('EW_X_GreyEnamel', (.32, .34, .33), .45)
    box('backboard', (0, .02, 1.25), (.75, .025, .9), material('EW_X_Plywood', (.38, .3, .2), .8), .003)
    box('meter_base', (-.18, -.03, 1.35), (.28, .09, .38), grey, .006)
    cyl('meter_ring', (-.18, -.08, 1.38), .095, .02, chrome, rotation=(math.pi / 2, 0, 0))
    cyl('meter_dome', (-.18, -.13, 1.38), .085, .09, glass, rotation=(math.pi / 2, 0, 0))
    cyl('meter_face', (-.18, -.095, 1.38), .075, .004, white, rotation=(math.pi / 2, 0, 0))
    for i in range(5):
        cyl(f'dial{i}', (-.24 + i * .03, -.098, 1.40), .011, .002, ink, vertices=16, rotation=(math.pi / 2, 0, 0))
    box('disconnect', (.17, -.04, 1.2), (.26, .1, .42), grey, .006)
    box('handle', (.31, -.07, 1.22), (.03, .04, .14), red, .003)
    label('warn', 'DANGER\n480V', .17, -.091, 1.28, .14, .07, .014)
    rod('conduit_up', (-.18, -.03, 1.54), (-.18, -.03, 3.3), .02, steel)
    rod('conduit_dc', (.17, -.04, 1.41), (.17, -.04, 1.6), .016, steel)
    rod('conduit_link', (.17, -.04, 1.6), (-.16, -.04, 1.6), .016, steel)
    cyl('weatherhead', (-.18, -.03, 3.34), .035, .08, grey)
    rod('ground', (.17, -.04, .99), (.17, -.04, .05), .009, material('EW_X_Copper', (.55, .25, .12), .35, 1))
    cyl('ground_rod', (.17, -.04, .03), .012, .06, material('EW_X_Copper', (.55, .25, .12), .35, 1))
    for z in (.4, 2.1, 2.9):
        box(f'strap{z}', (-.18, -.05, z if z > 1 else 2.5), (.06, .012, .02), steel, .001)
    return finish_asset(asset, OUT, 'Wall-mounted electrical service: meter with glass dome, fused 480 V disconnect, conduit to roof weatherhead, copper ground.')


# 10. Wall hose reel with hose bib.
def hose_reel():
    asset = 'SM_EW_HoseReel'; start_asset(asset)
    hose = material('EW_X_GreenHose', (.08, .22, .1), .55)
    box('bracket', (0, .01, .8), (.3, .02, .32), steel, .003)
    cyl('drum', (0, -.12, .8), .17, .14, steel, rotation=(math.pi / 2, 0, 0))
    for i in range(7):
        cyl(f'coil{i}', (0, -.12, .8), .19 - i * .002, .018, hose, rotation=(math.pi / 2, 0, 0)).location.y = -.07 - i * .018
    rod('crank', (.14, -.21, .8), (.14, -.27, .8), .008, steel)
    cyl('bib', (-.24, -.04, .55), .02, .07, brass, rotation=(math.pi / 2, 0, 0))
    cyl('bib_wheel', (-.24, -.08, .58), .025, .01, red, rotation=(math.pi / 2, 0, 0))
    rod('hose_drop', (0, -.12, .61), (.05, -.2, .02), .014, hose)
    rod('hose_ground', (.05, -.2, .02), (.45, -.32, .02), .014, hose)
    cyl('nozzle', (.48, -.33, .02), .016, .08, brass, rotation=(0, math.pi / 2, 0))
    return finish_asset(asset, OUT, 'Wall hose reel with brass hose bib and a run of green garden hose on the ground.')


wood = pbr('EW_XWood', span=.5)
iron = material('EW_X_CastIron', (.11, .16, .2), .5, .55)
redenamel = pbr('EW_XEnamelRed', span=.5)
olive = pbr('EW_XOliveDrab', span=.5)
oil = material('EW_X_OilFilm', (.035, .031, .025), .5)  # soaked-in, not a wet puddle


# 11. Mechanic's bench: steel frame, laminated timber top, bench vise, drawer and shelf of cans.
def mechanic_bench():
    asset = 'SM_EW_MechanicBench'; start_asset(asset)
    W, D, H = 2.0, .75, .88
    for sx in (-1, 1):
        for sy in (-1, 1):
            box(f'leg{sx}{sy}', (sx * (W / 2 - .05), sy * (D / 2 - .05), (H - .06) / 2), (.06, .06, H - .06), green, .004)
        box(f'side_rail{sx}', (sx * (W / 2 - .05), 0, .18), (.04, D - .1, .04), green, .003)
    box('shelf', (0, .02, .18), (W - .1, D - .12, .025), green, .003)
    for k in range(9):
        box(f'top_lam{k}', (0, -D / 2 + D * (k + .5) / 9, H - .03), (W, D / 9 - .002, .06), wood, .003)
    box('apron', (0, -D / 2 + .03, H - .1), (W - .1, .02, .08), green, .003)
    box('drawer', (.55, -D / 2 + .02, H - .14), (.5, .03, .12), green, .003)
    rod('drawer_pull', (.45, -D / 2 - .01, H - .14), (.65, -D / 2 - .01, H - .14), .008, chrome)
    # Bench vise near the left front corner.
    vx, vy, vz = -.72, -D / 2 + .1, H
    box('vise_base', (vx, vy + .05, vz + .02), (.16, .2, .04), iron, .006)
    box('vise_body', (vx, vy + .02, vz + .09), (.12, .26, .1), iron, .008)
    box('vise_fixed_jaw', (vx, vy + .1, vz + .17), (.16, .05, .08), iron, .004)
    box('vise_moving_jaw', (vx, vy - .07, vz + .17), (.16, .05, .08), iron, .004)
    for y in (vy + .075, vy - .045):
        box(f'jaw_plate{y}', (vx, y, vz + .19), (.155, .006, .04), steel, .001)
    cyl('vise_screw', (vx, vy - .16, vz + .12), .012, .2, steel, rotation=(math.pi / 2, 0, 0))
    rod('vise_handle', (vx - .12, vy - .23, vz + .12), (vx + .12, vy - .23, vz + .12), .008, steel)
    cyl('vise_anvil', (vx, vy + .15, vz + .15), .03, .04, iron)
    # Shelf contents: oil cans, a grease gun and a rag.
    for i, x in enumerate((-.75, -.6, -.45)):
        cyl(f'oilcan{i}', (x, .05, .25), .05, .12, material('EW_X_CanYellow', (.55, .4, .06), .4, .3), vertices=24)
    cyl('grease_gun', (.1, .05, .25), .025, .32, redenamel, vertices=20, rotation=(0, math.pi / 2, 0))
    box('rag', (.5, .02, .2), (.3, .22, .03), material('EW_X_Rag', (.42, .36, .3), .95), .01)
    box('top_toolbox', (.55, .12, H + .09), (.46, .2, .18), redenamel, .006)
    rod('toolbox_handle', (.4, .12, H + .2), (.7, .12, H + .2), .008, chrome)
    return finish_asset(asset, OUT, 'Mechanic\'s 2.0 m steel bench with laminated timber top, cast-iron vise, tool box, oil cans and rag.')


# 12. Upright air compressor on wheels.
def air_compressor():
    asset = 'SM_EW_AirCompressor'; start_asset(asset)
    cyl('tank', (0, 0, .55), .2, .9, redenamel, rotation=(0, 0, 0))
    for z in (.1, 1.0):
        cyl(f'tank_cap{z}', (0, 0, z), .19, .02, redenamel)
    box('motor_plate', (0, 0, 1.06), (.36, .36, .03), iron, .003)
    cyl('motor', (-.06, -.03, 1.17), .085, .2, iron, rotation=(0, math.pi / 2, 0))
    cyl('pump', (.1, .06, 1.17), .06, .16, iron)
    box('belt_guard', (.02, .14, 1.17), (.32, .04, .16), redenamel, .01)
    cyl('gauge', (.12, -.19, .9), .03, .02, chrome, rotation=(math.pi / 2, 0, 0))
    cyl('gauge_face', (.12, -.201, .9), .025, .002, white, rotation=(math.pi / 2, 0, 0))
    for sx in (-1, 1):
        cyl(f'wheel{sx}', (sx * .17, .1, .07), .07, .04, rubber, rotation=(0, math.pi / 2, 0))
    rod('foot', (0, -.18, .1), (0, -.25, .02), .015, iron)
    rod('handle_l', (-.12, .2, .6), (-.12, .25, 1.05), .012, steel)
    rod('handle_r', (.12, .2, .6), (.12, .25, 1.05), .012, steel)
    rod('handle_bar', (-.12, .25, 1.05), (.12, .25, 1.05), .012, steel)
    for i in range(6):
        cyl(f'hose_coil{i}', (-.22, 0, .55 + i * .012), .13, .012, material('EW_X_YellowHose', (.6, .45, .05), .5), vertices=32).rotation_euler = (0, math.pi / 2, 0)
    label('rating', '125 PSI\nDRAIN DAILY', 0, -.205, .45, .14, .07, .016)
    return finish_asset(asset, OUT, 'Upright 1.2 m air compressor: red tank, cast motor and pump, belt guard, gauge, wheels, coiled air hose.')


# 13. Hydraulic floor jack.
def floor_jack():
    asset = 'SM_EW_FloorJack'; start_asset(asset)
    box('frame', (0, 0, .07), (.32, .62, .1), redenamel, .01)
    for sx in (-1, 1):
        cyl(f'front_wheel{sx}', (sx * .13, -.25, .04), .04, .03, steel, rotation=(0, math.pi / 2, 0))
        cyl(f'rear_caster{sx}', (sx * .14, .26, .035), .035, .03, steel, rotation=(0, math.pi / 2, 0))
    box('lift_arm', (0, -.12, .16), (.12, .4, .05), redenamel, .006)
    cyl('saddle', (0, -.3, .2), .05, .03, iron)
    cyl('ram', (0, .1, .16), .035, .14, chrome, rotation=(math.pi / 3, 0, 0))
    rod('handle', (0, .3, .1), (0, .95, .7), .016, redenamel)
    cyl('grip', (0, .95, .7), .02, .16, rubber, rotation=(0, math.pi / 2, 0))
    return finish_asset(asset, OUT, 'Red hydraulic trolley floor jack with long handle.')


# 14. Three steel jerry cans, one with a spout.
def jerry_cans():
    asset = 'SM_EW_JerryCans'; start_asset(asset)
    for i, (x, rz) in enumerate(((-.2, 0), (.0, .05), (.2, -.04))):
        o = box(f'can{i}', (x, 0, .235), (.165, .35, .47), olive, .012)
        o.rotation_euler = (0, 0, rz)
        for k in (-1, 1):
            box(f'can{i}_rib{k}', (x - .0835, k * .09, .2), (.004, .1, .3), olive, .002)
            box(f'can{i}_ribR{k}', (x + .0835, k * .09, .2), (.004, .1, .3), olive, .002)
        for h in (-.07, 0, .07):
            rod(f'can{i}_handle{h}', (x, h - .02, .47), (x, h + .02, .5), .01, olive)
        cyl(f'can{i}_cap', (x, -.13, .48), .025, .03, olive, rotation=(math.pi / 5, 0, 0))
    rod('spout', (.2, -.13, .5), (.2, -.28, .66), .014, olive)
    label('fuel', 'GASOLINE', -.2, -.176, .28, .1, .04, .01)
    return finish_asset(asset, OUT, 'Three olive steel jerry cans for the backup generator, one fitted with a pouring spout.')


# 15. Irregular oil stains: flat glossy polygons, no alpha; lay on the garage floor.
def oil_stains():
    asset = 'SM_EW_OilStains'; start_asset(asset)
    import random
    rnd = random.Random(1986)
    for k, (cx, cy, r) in enumerate(((0, 0, .26), (-.3, .7, .13), (.3, -.6, .17), (.1, 1.2, .07), (-.36, -.22, .06))):
        n = 28
        rad = [r * (1 + .28 * math.sin(3 * i * 2 * math.pi / n + k) + .16 * rnd.uniform(-1, 1)) for i in range(n)]
        # Concentric rings: decimation collapses the flat interior first and keeps the outline (LOD bounds).
        rings = (.25, .5, .75, 1.)
        verts = [(cx, cy, .0015)]
        for f in rings:
            verts += [(cx + f * rad[i] * math.cos(2 * math.pi * i / n), cy + f * rad[i] * .8 * math.sin(2 * math.pi * i / n), .0015) for i in range(n)]
        faces = [(0, 1 + i, 1 + (i + 1) % n) for i in range(n)]
        for r_ in range(len(rings) - 1):
            a0, b0 = 1 + r_ * n, 1 + (r_ + 1) * n
            faces += [(a0 + i, b0 + i, b0 + (i + 1) % n, a0 + (i + 1) % n) for i in range(n)]
        me = bpy.data.meshes.new(f'stain{k}'); me.from_pydata(verts, [], faces); me.update()
        me.uv_layers.new(name='UVMap')
        ob = bpy.data.objects.new(f'stain{k}', me); bpy.context.collection.objects.link(ob); me.materials.append(oil)
    return finish_asset(asset, OUT, 'Old engine-oil stains for the garage floor; flat glossy polygons, 1.5 mm above the slab.')


def plaque(asset, print_name):
    start_asset(asset)
    w, h = .32, .10
    box('plate', (0, .003, h / 2), (w, .006, h), dark, .0015)
    sheet('face', (0, -.0002, h / 2), w - .004, h - .004, printed(print_name))
    for sx in (-1, 1):
        cyl(f'screw{sx}', (sx * (w / 2 - .012), -.001, h / 2), .004, .003, chrome, vertices=12, rotation=(math.pi / 2, 0, 0))
    return finish_asset(asset, OUT, f'Engraved black phenolic door plaque {print_name}, 0.32 x 0.10 m.')


def notice_board():
    asset = 'SM_EW_NoticeBoard'; start_asset(asset)
    w, h = 1.20, .90
    box('backing', (0, .008, h / 2), (w, .016, h), wood, .003)
    sheet('cork', (0, -.0002, h / 2), w - .04, h - .04, printed('EW_Print_NoticeBoard'))
    for z in (.01, h - .01):
        box(f'frame_h{z}', (0, -.004, z), (w, .012, .03), wood, .002)
    for x in (-w / 2 + .01, w / 2 - .01):
        box(f'frame_v{x}', (x, -.004, h / 2), (.03, .012, h), wood, .002)
    return finish_asset(asset, OUT, 'Lab cork notice board, 1.2 x 0.9 m, with pinned memos (custody rule, generator fuel, room guide, night log).')


def lab_stool():
    asset = 'SM_EW_LabStool'; start_asset(asset)
    cyl('seat', (0, 0, .62), .17, .05, material('EW_X_Vinyl', (.06, .07, .065), .55), vertices=40)
    cyl('seat_pan', (0, 0, .59), .15, .02, chrome, vertices=32)
    cyl('column', (0, 0, .4), .022, .38, chrome, vertices=20)
    bpy.ops.mesh.primitive_torus_add(major_radius=.17, minor_radius=.009, major_segments=40, minor_segments=8, location=(0, 0, .26))
    ring = bpy.context.object; ring.name = 'foot_ring'; ring.data.materials.append(chrome)
    for i in range(4):
        a = i * math.pi / 2 + math.pi / 4
        rod(f'leg{i}', (0, 0, .22), (.24 * math.cos(a), .24 * math.sin(a), .02), .012, chrome)
        cyl(f'glide{i}', (.24 * math.cos(a), .24 * math.sin(a), .01), .016, .02, rubber, vertices=12)
    return finish_asset(asset, OUT, 'Chrome lab stool with black vinyl seat and foot ring.')


def wall_shelf():
    asset = 'SM_EW_ReagentShelf'; start_asset(asset)
    W, D = .90, .22
    box('board', (0, -D / 2 + .01, .02), (W, D, .025), green, .003)
    for x in (-W / 2 + .08, W / 2 - .08):
        box(f'bracket{x}', (x, -.06, -.06), (.02, .12, .16), steel, .002)
    box('lip', (0, -D + .015, .05), (W, .006, .04), steel, .001)
    items = [('a', 'ETHANOL'), ('b', 'H2O DIST'), ('a', 'ACETONE'), ('b', 'BUFFER'), ('a', 'IPA'), ('k', None)]
    for i, (kind, tag) in enumerate(items):
        x = -W / 2 + .09 + i * (W - .18) / (len(items) - 1)
        if kind == 'k':
            cyl(f'beaker{i}', (x, -.11, .1), .04, .14, glass, vertices=28)
        else:
            bottle(f'bottle{i}', x, -.11, .033, .034, .19, amber if kind == 'a' else glass, dark, tag)
    return finish_asset(asset, OUT, 'Green steel wall shelf with labelled reagent bottles and a beaker.')


def relay_tester():
    """Freeform calibration bench test set. The live status lamp (a separate runtime box,
    0.28 x 0.11 m) sits in the bezel window at 0.20 m; keep that window and height."""
    asset = 'SM_EW_RelayTester'; start_asset(asset)
    W, D, H = .42, .28, .27
    grey = pbr('EW_XInstrumentGrey', span=.5)
    box('case', (0, 0, H / 2), (W, D, H), grey, .008)
    box('panel', (0, -D / 2 - .002, H / 2), (W - .02, .004, H - .02), material('EW_X_PanelAlu', (.55, .56, .54), .35, .8), .002)
    for z, hgt in ((.255, .01), (.145, .01)):
        box(f'bezel_h{z}', (0, -D / 2 - .006, z), (.30, .006, hgt), dark, .001)
    for x in (-.15, .15):
        box(f'bezel_v{x}', (x, -D / 2 - .006, .20), (.01, .006, .12), dark, .001)
    box('window_back', (0, -D / 2 - .003, .20), (.28, .002, .11), dark, 0)
    for i, x in enumerate((-.15, -.07, .01)):
        cyl(f'rotary{i}', (x, -D / 2 - .016, .075), .018, .022, dark, vertices=24, rotation=(math.pi / 2, 0, 0))
        box(f'rotary{i}_mark', (x, -D / 2 - .028, .086), (.003, .002, .012), white, 0)
    for i, (x, c) in enumerate(((.09, red), (.12, dark), (.15, red), (.18, dark))):
        cyl(f'jack{i}', (x, -D / 2 - .008, .06), .007, .012, c, vertices=16, rotation=(math.pi / 2, 0, 0))
    rod('toggle', (.135, -D / 2 - .004, .11), (.135, -D / 2 - .03, .125), .003, chrome)
    label('maker', 'SARO  RT-12  RELAY TEST SET', -.05, -D / 2 - .0045, .028, .2, .02, .009)
    # Relay socket with a plugged octal relay on top; carry handle.
    cyl('socket', (.1, .02, H + .01), .03, .02, dark, vertices=24)
    cyl('relay_can', (.1, .02, H + .05), .022, .06, material('EW_X_RelayClear', (.6, .62, .58), .2, .3), vertices=24)
    for x in (-.17, .17):
        rod(f'handle_post{x}', (x, -.02, H), (x, -.02, H + .05), .006, chrome)
    rod('handle', (-.17, -.02, H + .05), (.17, -.02, H + .05), .008, chrome)
    rod('test_lead_r', (.09, -D / 2 - .014, .06), (.15, -D / 2 - .12, -.0), .003, red)
    rod('test_lead_b', (.12, -D / 2 - .014, .06), (.2, -D / 2 - .1, -.0), .003, dark)
    return finish_asset(asset, OUT, 'SARO RT-12 relay test set for the Freeform calibration bench; status window for the live lamp.')


def vinyl_floor():
    """Photo pass: 12-inch VCT over the lab, storage and calibration rooms (x20.85..31.525, z+-5.25).
    Origin at the area centre; UVs in world metres / 1.2192 (one texture = 4 x 4 tiles)."""
    asset = 'SM_EW_VinylFloor'; start_asset(asset)
    W, D, tile = 10.675, 10.5, 2.0  # reference pass: polished concrete, one texture = 2 m
    verts = [(-W / 2, -D / 2, .004), (W / 2, -D / 2, .004), (W / 2, D / 2, .004), (-W / 2, D / 2, .004)]
    me = bpy.data.meshes.new('vct'); me.from_pydata(verts, [], [(0, 1, 2, 3)]); me.update()
    uv = me.uv_layers.new(name='UVMap')
    for li, loop in enumerate(me.loops):
        x, y, _ = me.vertices[loop.vertex_index].co
        uv.data[li].uv = (x / tile, y / tile)
    ob = bpy.data.objects.new('vct', me); bpy.context.collection.objects.link(ob)
    me.materials.append(pbr('EW_PolishedConcrete'))
    return finish_asset(asset, OUT, 'Waxed 12-inch vinyl composition tile floor for the east wing rooms (garage excluded).')



def garage_floor():
    """Photo pass 3: unique garage slab over x31.675..38.35, z-5.25..5.25 (make_garage_floor.py).
    Origin at the area centre; four quads leave the drain grate (x35.15..35.65, z.05..35) open.
    UV u = (x-31.675)/6.675, v = (z+5.25)/10.5 so the painted lane lines land on site positions."""
    asset = 'SM_EW_GarageSlab'; start_asset(asset)
    X0, X1, Z0, Z1, cx = 31.675, 38.35, -5.25, 5.25, 35.0125
    rects = [(X0, X1, Z0, .05), (X0, X1, .35, Z1), (X0, 35.15, .05, .35), (35.65, X1, .05, .35)]
    verts, faces, uvs = [], [], []
    G = 8  # grid per rectangle: LOD decimation collapses interior edges first and keeps the outline
    for xa, xb, za, zb in rects:
        k = len(verts)
        for j in range(G + 1):
            for i in range(G + 1):
                x, z = xa + (xb - xa) * i / G, za + (zb - za) * j / G
                verts.append((x - cx, -z, .004)); uvs.append(((x - X0) / (X1 - X0), (z - Z0) / (Z1 - Z0)))
        for j in range(G):
            for i in range(G):
                q = k + j * (G + 1) + i
                faces.append((q, q + G + 1, q + G + 2, q + 1))   # winding so the face normal points up (+Z)
    me = bpy.data.meshes.new('slab'); me.from_pydata(verts, [], faces); me.update()
    uv = me.uv_layers.new(name='UVMap')
    for li, loop in enumerate(me.loops):
        uv.data[li].uv = uvs[loop.vertex_index]
    ob = bpy.data.objects.new('slab', me); bpy.context.collection.objects.link(ob)
    me.materials.append(pbr('EW_GarageSlab'))
    return finish_asset(asset, OUT, 'Unique 6.7 x 10.5 m garage slab: concrete, saw cuts, worn lane lines, tyre tracks, oil, blown sand.')


def steel_joist():
    """Reference pass: exposed W-shape steel joist under the deck, 10.4 m along X, 0.20 m deep.
    Origin at the bottom centre; place with yaw 90 to span north-south."""
    asset = 'SM_EW_SteelJoist'; start_asset(asset)
    L, Hd, Fw, Tf, Tw = 10.4, .20, .10, .012, .007
    paint = pbr('EW_XInstrumentGrey', span=.5)
    box('bottom_flange', (0, 0, Tf / 2), (L, Fw, Tf), paint, .002)
    box('top_flange', (0, 0, Hd - Tf / 2), (L, Fw, Tf), paint, .002)
    box('web', (0, 0, Hd / 2), (L, Tw, Hd - 2 * Tf), paint, .001)
    for x in [-L / 2 + .6 + i * 1.2 for i in range(9)]:
        box(f'stiffener{x:.1f}', (x, 0, Hd / 2), (.008, Fw - .01, Hd - 2 * Tf), paint, .001)
    return finish_asset(asset, OUT, 'Exposed painted W-shape steel joist, 10.4 m, under the east wing roof deck.')


def ceiling_conduit():
    """Reference pass: pair of 1-inch EMT runs with strut hangers, 5 m along X (repeat end to end)."""
    asset = 'SM_EW_CeilingConduit'; start_asset(asset)
    L = 5.0
    for y in (-.06, .06):
        rod(f'emt{y}', (-L / 2, y, .05), (L / 2, y, .05), .0125, steel)
    for x in (-L / 2 + .4, 0., L / 2 - .4):
        box(f'strut{x}', (x, 0, .09), (.04, .22, .02), steel, .002)
        for y in (-.06, .06):
            rod(f'clamp{x}{y}', (x, y, .05), (x, y, .09), .004, steel)
    box(f'jbox', (1.2, .06, .05), (.10, .10, .06), steel, .003)
    return finish_asset(asset, OUT, 'Two exposed EMT conduit runs on strut hangers with a junction box, 5 m.')

lab_sink()
glass_cabinet()
poster('SM_EW_PosterRF', 'EW_Print_PosterRF', .56, .79)
poster('SM_EW_PosterGlasses', 'EW_Print_PosterGlasses', .56, .79, chrome)
calendar_1986()
wall_clock()
first_aid()
swamp_cooler()
hose_reel()
mechanic_bench()
air_compressor()
floor_jack()
jerry_cans()
oil_stains()
for key in ('Lab', 'Storage', 'Calibration', 'Garage'):
    plaque('SM_EW_Plaque' + key, 'EW_Print_Plaque' + key)
notice_board()
lab_stool()
wall_shelf()
relay_tester()
vinyl_floor()
steel_joist()
ceiling_conduit()
garage_floor()
render_catalog(OUT, 'EASTWING / EXTRAS (CLAUDE)')
print('EASTWING EXTRAS EXPORT COMPLETE', flush=True)
