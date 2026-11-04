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

green = material('EW_X_GreenEnamel', (.16, .24, .20), .33, texture='EW_GreenSteel')
steel = material('EW_X_Stainless', (.48, .51, .5), .24, .85, texture='EW_Galvanized')
chrome = material('EW_X_Chrome', (.7, .73, .74), .17, 1)
dark = material('EW_X_Bakelite', (.017, .02, .019), .39)
rubber = material('EW_X_Rubber', (.022, .025, .023), .7)
cream = material('EW_X_CreamEnamel', (.69, .66, .53), .3)
white = material('EW_X_WhiteEnamel', (.74, .73, .68), .28)
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


lab_sink()
glass_cabinet()
poster('SM_EW_PosterRF', 'EW_Print_PosterRF', .56, .79)
poster('SM_EW_PosterGlasses', 'EW_Print_PosterGlasses', .56, .79, chrome)
calendar_1986()
wall_clock()
first_aid()
render_catalog(OUT, 'EASTWING / EXTRAS (CLAUDE)')
print('EASTWING EXTRAS EXPORT COMPLETE', flush=True)
