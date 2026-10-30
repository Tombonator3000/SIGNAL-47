"""Original 1986 RX bank 3 detail, on the existing live rack.

python3 rx3.py --textures       # light, Pillow only
blender -b -t 2 --python rx3.py # editable .blend + measured native FBX

The 19 inch faceplates are 0.4826m wide, on a 42U / 44.45mm grid.
Game coordinates use (x, height, z); Blender stores (x, -z, height).
The old rack, LEDs, power lever and service-record interaction stay in place.
All artwork and geometry are original; the HP 8640B manual is a period reference,
not a texture/geometry source or a claim that this fictional rack is an HP model.
"""
from pathlib import Path
import argparse
import json
import math
import random
import sys

ROOT = Path(__file__).resolve().parent
NAME = "SM_Saro1986_RX3Detail"
PREFIX = "M_Saro1986_RX3_"
U = .04445
WIDTH = .4826
BOTTOM = .04
TOP = BOTTOM + 42 * U
PANEL_X = 5.3475  # front5.3455, behind existing LED front5.3445
CENTRE_Z = -2.4
MODULES = (("Identity", 2), ("RF_receiver", 5), ("Front_end_status", 2),
           ("Patch_distribution", 3), ("Reference_status", 2),
           ("Crystal_reference", 3), ("IF_status", 2), ("Supply_monitor", 3),
           ("Local_status", 2), ("Service_bay", 8), ("Regulated_supply", 6),
           ("Cooling", 4))


def panels():
    top = TOP
    for name, units in MODULES:
        low = top - units * U
        yield {"name": name, "top": top, "bottom": low, "height": units * U, "u": units}
        top = low
    assert abs(top - BOTTOM) < 1e-8


def rows():
    def row(key, colour, roughness=.5, metallic=0, **extra):
        return {"name": PREFIX + key, "base_color_linear": colour,
                "roughness": roughness, "metallic": metallic, "specular": .5, **extra}
    return [row("Panel", [.33, .35, .31], .5, .45, textures={
                "base_color": "textures/rx3_panel_base.png",
                "roughness": "textures/rx3_panel_rough.png",
                "normal": "textures/rx3_panel_normal_dx.png"}, normal_strength=.18),
            row("DarkSteel", [.035, .044, .040], .49, .55),
            row("Aluminium", [.35, .37, .35], .32, .82),
            row("Bakelite", [.012, .015, .012], .38),
            row("Cream", [.63, .58, .44], .66),
            row("MeterScale", [.75, .72, .61], .22, textures={"base_color": "textures/rx3_meter_scale.png"}),
            row("CableBlack", [.008, .010, .008], .65),
            row("CableBlue", [.025, .055, .075], .61),
            row("Red", [.27, .035, .025], .53)]


def textures(root=ROOT):
    """Draw original silkscreen, channel legends and an analogue meter scale."""
    from PIL import Image, ImageDraw, ImageFont
    root = Path(root)
    out = root / "textures"
    out.mkdir(parents=True, exist_ok=True)
    w, h = 1024, 4096
    image = Image.new("RGB", (w, h), (125, 131, 120))
    draw = ImageDraw.Draw(image)
    mono = Path("/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf")
    bold = Path("/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf")
    if not mono.exists():
        mono = Path("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf")
    if not bold.exists():
        bold = Path("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf")
    fonts = {}

    def point(u, y):
        return ((u / WIDTH + .5) * w, (TOP-y) / (TOP-BOTTOM) * h)

    def label(text, u, y, size=.005, colour=(224, 220, 198), heavy=False, anchor="mm"):
        key = (max(7, round(size * h / (TOP-BOTTOM))), heavy)
        if key not in fonts:
            fonts[key] = ImageFont.truetype(str(bold if heavy else mono), key[0])
        draw.text(point(u, y), text, font=fonts[key], fill=colour, anchor=anchor)

    rng = random.Random(47031986)
    for panel in panels():
        name, top, bottom = panel["name"], panel["top"], panel["bottom"]
        colour = ((54, 68, 67) if name == "Identity" else
                  (151, 153, 136) if name in ("RF_receiver", "Crystal_reference", "Supply_monitor") else
                  (94, 108, 103) if "status" in name else (99, 109, 103))
        y0, y1 = point(0, top-.001)[1], point(0, bottom+.001)[1]
        draw.rectangle((0, y0, w-1, y1), fill=colour)
        draw.line((0, y0+2, w-1, y0+2), fill=tuple(min(255, c+17) for c in colour), width=2)
        draw.line((0, y1-2, w-1, y1-2), fill=tuple(max(0, c-30) for c in colour), width=3)
        # Millimetre wear at module edges and rack screw locations, not a noisy bump.
        for _ in range(60):
            x = rng.randrange(w)
            yy = rng.choice((int(y0)+rng.randrange(2, 8), int(y1)-rng.randrange(2, 8)))
            draw.line((x, yy, x+rng.randrange(1, 9), yy), fill=tuple(max(0, c-18) for c in colour))
    label("SARO  /  RECEIVING SYSTEMS", 0, 1.889, .008, heavy=True)
    label("RX BANK 3", -.025, 1.853, .022, heavy=True)
    label("ANALOG IF RECEIVER   /   TYPE 3B", 0, 1.829, .005)
    dark = (38, 45, 39)
    label("RF / IF RECEIVER", -.193, 1.804, .009, dark, True, "lm")
    label("LEVEL", -.117, 1.675, .0055, dark)
    label("COARSE TUNE", .045, 1.675, .0055, dark)
    label("FINE", .144, 1.677, .005, dark)
    label("FREQUENCY RANGE  /  VERNIER", .047, 1.781, .005, dark)
    label("CAL", -.157, 1.609, .0045, dark)
    label("ATTN dB", -.067, 1.609, .0045, dark)
    label("0   10   20   30", .060, 1.622, .004, dark)
    for y, title, first in ((1.55, "FRONT END", 1), (1.33, "REFERENCE / LOCK", 7),
                            (1.11, "IF AMPLIFIERS", 13), (.89, "LOCAL / REMOTE", 19)):
        label(title, -.192, y+.030, .0054, heavy=True, anchor="lm")
        for k in range(6):
            u = -.22+k*.068
            label("%02d" % (first+k), u, y-.024, .0044)
    label("REFERENCE / PATCH DISTRIBUTION", -.193, 1.490, .0062, heavy=True, anchor="lm")
    for u, text in zip((-.16, -.065, .035, .135), ("REF IN", "IF OUT", "MONITOR", "TERM")):
        label(text, u, 1.397, .0048)
    label("CRYSTAL REFERENCE", -.192, 1.269, .007, dark, True, "lm")
    label("PHASE", -.127, 1.166, .005, dark)
    label("DELAY", -.032, 1.166, .005, dark)
    label("SUPPLY MONITOR", -.193, 1.047, .0065, dark, True, "lm")
    label("DC LEVEL", -.125, .950, .005, dark)
    label("METER", -.020, .950, .005, dark)
    label("FIELD SERVICE", .119, .791, .0056, heavy=True)
    label("ISOLATE", .119, .764, .005)
    label("BEFORE", .119, .748, .005)
    label("REMOVING", .119, .732, .005)
    label("MODULES", .119, .716, .005)
    label("REGULATED SUPPLY", -.19, .466, .008, heavy=True, anchor="lm")
    label("DC OUTPUT", -.126, .348, .005)
    label("FUSES", .117, .424, .005)
    label("AUX", .080, .335, .0045)
    label("MAIN", .161, .335, .0045)
    label("CAUTION  /  LINE VOLTAGE", 0, .247, .0055)
    label("AIR INTAKE   /   KEEP CLEAR", 0, .067, .0056, heavy=True)
    label("PROPERTY OF SARO   3B-07164", 0, .091, .0043)
    image.save(out / "rx3_panel_base.png")
    rough = Image.new("RGB", (w, h), (145, 145, 145))
    rd = ImageDraw.Draw(rough)
    for _ in range(1800):
        x, y = rng.randrange(w), rng.randrange(h)
        c = rng.randrange(132, 159)
        rd.line((x, y, min(w-1, x+rng.randrange(3, 24)), y), fill=(c, c, c))
    rough.save(out / "rx3_panel_rough.png")
    Image.new("RGB", (256, 1024), (128, 128, 255)).save(out / "rx3_panel_normal_dx.png")

    meter = Image.new("RGB", (1024, 512), (213, 207, 180))
    md = ImageDraw.Draw(meter)
    centre, radius = (512, 460), 363
    for k in range(41):
        a = math.radians(148 - k*2.9)
        inner = radius - (40 if k % 10 == 0 else 25 if k % 5 == 0 else 14)
        p0 = (centre[0]+math.cos(a)*inner, centre[1]-math.sin(a)*inner)
        p1 = (centre[0]+math.cos(a)*radius, centre[1]-math.sin(a)*radius)
        md.line((*p0, *p1), fill=(42, 45, 37), width=4 if k % 5 == 0 else 2)
    f = ImageFont.truetype(str(mono), 38)
    for text, degrees in (("-40", 148), ("-20", 119), ("-10", 90), ("0", 61), ("+3", 32)):
        a = math.radians(degrees)
        md.text((512+math.cos(a)*285, 460-math.sin(a)*285), text, fill=(45, 47, 38), font=f, anchor="mm")
    md.arc((149, 97, 875, 823), 312, 328, fill=(139, 58, 40), width=6)
    md.text((512, 353), "LEVEL  /  dB", fill=(46, 47, 39), font=f, anchor="mm")
    meter.save(out / "rx3_meter_scale.png")
    (root / "rx3-materials.json").write_text(json.dumps(rows(), indent=2)+"\n")
    return out


def build(output_root=None):
    import bpy
    from mathutils import Vector
    root = Path(output_root) if output_root else ROOT
    (root / "Export").mkdir(parents=True, exist_ok=True)
    if not (root / "textures/rx3_panel_base.png").is_file():
        raise RuntimeError("Run ordinary Python rx3.py --textures before Blender")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1
    edit = bpy.data.collections.new("01_EDITABLE_RX3_MODULES")
    export = bpy.data.collections.new("02_NATIVE_RX3_EXPORT")
    scene.collection.children.link(edit)
    scene.collection.children.link(export)
    mats, objects = {}, []

    def active(obj):
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj

    for row in rows():
        mat = bpy.data.materials.new(row["name"])
        mat.use_nodes = True
        shader = mat.node_tree.nodes.get("Principled BSDF")
        shader.inputs["Base Color"].default_value = (*row["base_color_linear"], 1)
        shader.inputs["Roughness"].default_value = row["roughness"]
        shader.inputs["Metallic"].default_value = row["metallic"]
        for role, path in row.get("textures", {}).items():
            image = bpy.data.images.load(str(root/path), check_existing=True)
            image.colorspace_settings.name = "sRGB" if role == "base_color" else "Non-Color"
            node = mat.node_tree.nodes.new("ShaderNodeTexImage"); node.image = image
            if role == "base_color":
                mat.node_tree.links.new(node.outputs["Color"], shader.inputs["Base Color"])
            elif role == "roughness":
                mat.node_tree.links.new(node.outputs["Color"], shader.inputs["Roughness"])
        mats[row["name"]] = mat

    def xyz(p):
        return Vector((p[0], -p[2], p[1]))

    def keep(obj, key):
        for collection in list(obj.users_collection):
            collection.objects.unlink(obj)
        edit.objects.link(obj)
        obj.data.materials.append(mats[PREFIX+key])
        objects.append(obj)
        return obj

    def box(name, at, size, key="DarkSteel", bevel=.0008):
        bpy.ops.mesh.primitive_cube_add(size=1, location=xyz(at))
        obj = bpy.context.object; obj.name = name
        obj.dimensions = (size[0], size[2], size[1])
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        if bevel:
            mod = obj.modifiers.new("Machined_edge", "BEVEL")
            mod.width = min(bevel, min(size)*.24); mod.segments = 2
            bpy.ops.object.modifier_apply(modifier=mod.name)
            mod = obj.modifiers.new("Face_normals", "WEIGHTED_NORMAL"); mod.keep_sharp = True
            bpy.ops.object.modifier_apply(modifier=mod.name)
        return keep(obj, key)

    def cylinder(name, at, radius, depth, key="Aluminium", sides=24):
        bpy.ops.mesh.primitive_cylinder_add(vertices=sides, radius=radius, depth=depth, location=xyz(at))
        obj = bpy.context.object; obj.name = name
        obj.rotation_euler = Vector((-1, 0, 0)).to_track_quat("Z", "Y").to_euler()
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
        return keep(obj, key)

    def front_plane(name, x, y, u, width, height, key, full_uv=False):
        z = CENTRE_Z+u
        points = [(x, y-height/2, z-width/2), (x, y-height/2, z+width/2),
                  (x, y+height/2, z+width/2), (x, y+height/2, z-width/2)]
        data = bpy.data.meshes.new(name)
        data.from_pydata([xyz(p) for p in points], [], [(0, 1, 2, 3)])
        data.update(); obj = bpy.data.objects.new(name, data); edit.objects.link(obj)
        uv = data.uv_layers.new(name="UVMap")
        for loop, coord in zip(data.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
            uv.data[loop.index].uv = coord
        obj.data.materials.append(mats[PREFIX+key]); objects.append(obj)
        return obj

    def curve(name, points, radius=.0025, key="CableBlack"):
        data = bpy.data.curves.new(name, "CURVE"); data.dimensions = "3D"; data.resolution_u = 10
        data.bevel_depth = radius; data.bevel_resolution = 2
        spline = data.splines.new("BEZIER"); spline.bezier_points.add(len(points)-1)
        for vertex, p in zip(spline.bezier_points, points):
            vertex.co = xyz(p); vertex.handle_left_type = "AUTO"; vertex.handle_right_type = "AUTO"
        obj = bpy.data.objects.new(name, data); edit.objects.link(obj)
        active(obj); bpy.ops.object.convert(target="MESH")
        obj = bpy.context.object; obj.data.materials.append(mats[PREFIX+key]); objects.append(obj)
        return obj

    def screw(name, y, u, x=5.3435):
        z = CENTRE_Z+u
        cylinder(name+"_head", (x, y, z), .0033, .0024, sides=12)
        box(name+"_slot", (x-.0013, y, z), (.0003, .0007, .0045), "Bakelite", 0)

    def knob(name, y, u, radius=.023, depth=.027):
        z = CENTRE_Z+u
        cylinder(name+"_washer", (5.3435, y, z), radius+.003, .0025)
        # Alternating radii form real axial ribs rather than a smooth cylinder
        # with a painted knurl. The face has a separate aluminium skirt and index.
        vertices, faces = [], []
        count = 72
        for x in (5.343, 5.343-depth):
            for k in range(count):
                a = k*math.tau/count; r = radius*(1 if k % 2 else .91)
                vertices.append(xyz((x, y+math.sin(a)*r, z+math.cos(a)*r)))
        for k in range(count):
            n = (k+1) % count; faces.append((k, n, count+n, count+k))
        faces += [tuple(range(count-1, -1, -1)), tuple(range(count, 2*count))]
        data = bpy.data.meshes.new(name); data.from_pydata(vertices, [], faces); data.update()
        obj = bpy.data.objects.new(name, data); edit.objects.link(obj)
        obj.data.materials.append(mats[PREFIX+"Bakelite"]); objects.append(obj)
        cylinder(name+"_cap", (5.342-depth, y, z), radius*.72, .0016, "DarkSteel", 32)
        box(name+"_index", (5.3407-depth, y+radius*.48, z), (.0009, radius*.34, .0016), "Cream", .0002)

    def toggle(name, y, u):
        z = CENTRE_Z+u
        cylinder(name+"_locknut", (5.341, y, z), .0055, .005, sides=6)
        curve(name+"_arm", [(5.340, y, z), (5.328, y+.006, z)], .0017, "Aluminium")
        cylinder(name+"_cap", (5.326, y+.007, z), .0031, .005, "Bakelite", 12)

    def jack(name, y, u, plugged=False):
        z = CENTRE_Z+u
        cylinder(name+"_nut", (5.3425, y, z), .008, .004, sides=6)
        cylinder(name+"_barrel", (5.337, y, z), .0055, .008, sides=20)
        cylinder(name+"_dielectric", (5.3325, y, z), .0037, .0015, "Bakelite", 16)
        if plugged:
            cylinder(name+"_plug", (5.325, y, z), .0057, .014, sides=20)
            for dx in (5.328, 5.324, 5.320):
                cylinder(name+"_grip", (dx, y, z), .0061, .001, sides=20)

    def meter(name, y, u):
        z = CENTRE_Z+u
        box(name+"_case", (5.339, y, z), (.013, .079, .122), "Bakelite", .004)
        box(name+"_chrome_bezel", (5.3315, y, z), (.003, .072, .114), "Aluminium", .003)
        front_plane(name+"_reflective_scale", 5.3295, y, u, .103, .060, "MeterScale", True)
        # Parked at the mechanical left stop; these are passive analogue scales,
        # not additional powered status indicators or a fabricated live reading.
        curve(name+"_needle", [(5.3287, y-.025, z+.002), (5.3287, y+.017, z-.037)], .00045, "Bakelite")
        cylinder(name+"_pivot", (5.3283, y-.025, z+.002), .0034, .0016, "Bakelite", 12)
        for du in (-.051, .051):
            screw(name+"_bezel_%s" % du, y-.030, u+du, 5.329)

    for panel in panels():
        name, high, low = panel["name"], panel["top"], panel["bottom"]
        obj = box("RX3_19in_%s_%dU" % (name, panel["u"]), (PANEL_X, (high+low)/2, CENTRE_Z),
                  (.004, high-low-.002, WIDTH), "Panel", .0006)
        # One physical atlas spans all modules; their actual front face UVs
        # follow height and rack width, so silkscreen labels align with controls.
        uv = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
        for loop in obj.data.loops:
            p = obj.matrix_world @ obj.data.vertices[loop.vertex_index].co
            uv.data[loop.index].uv = ((-p.y-CENTRE_Z)/WIDTH+.5, (p.z-BOTTOM)/(TOP-BOTTOM))
        for side in (-1, 1):
            for y in (low+.011, high-.011):
                screw(name+"_mount_%d_%.3f" % (side, y), y, side*.223)
    # Front rack ears and embossed 19 inch rail lips, outside all LED centres.
    for u in (-.252, .252):
        box("RX3_recessed_rail_%s" % u, (5.345, .974, CENTRE_Z+u), (.008, 1.878, .014), "DarkSteel", .001)
        for y in (.10, .34, .58, .82, 1.06, 1.30, 1.54, 1.78):
            screw("RX3_rail_%s_%.2f" % (u, y), y, u, 5.339)

    meter("RF_level", 1.727, -.117)
    knob("Coarse_tuning", 1.726, .045, .031, .029)
    knob("Fine_tuning", 1.721, .144, .017, .024)
    toggle("Calibration", 1.639, -.157)
    knob("Attenuator", 1.649, -.067, .013, .020)
    for u in (.025, .075, .125):
        toggle("RF_range_%s" % u, 1.646, u)
    for u in (-.160, -.065, .035, .135):
        jack("Distribution_%s" % u, 1.438, u, plugged=u in (-.160, -.065, .035))
    knob("Reference_phase", 1.218, -.127, .026, .027)
    knob("Reference_delay", 1.218, -.032, .015, .022)
    jack("Reference_test", 1.218, -.198, True)
    meter("Supply_monitor", .997, -.125)
    knob("Meter_selector", .996, -.020, .014, .022)
    # Removable handle bars sit at the RF module's margins, not across the lamps.
    for u in (-.195, .195):
        curve("RF_module_handle_%s" % u, [(5.342, 1.776, CENTRE_Z+u), (5.310, 1.770, CENTRE_Z+u),
            (5.310, 1.661, CENTRE_Z+u), (5.342, 1.653, CENTRE_Z+u)], .0038, "Aluminium")

    for row in range(4):
        y = 1.55-row*.22
        for k in range(6):
            z = -2.62+k*.068
            # The live original16mm square LED remains inside a19mm clear hole.
            for dz in (-.011, .011):
                box("LED_%d_%d_vertical_bezel_%s" % (row, k, dz), (5.342, y, z+dz), (.005, .025, .003), "Bakelite", .0006)
            for dy in (-.011, .011):
                box("LED_%d_%d_horizontal_bezel_%s" % (row, k, dy), (5.342, y+dy, z), (.005, .003, .019), "Bakelite", .0006)

    # Coax patch leads stay in the spare left rail or between status rows.
    curve("Reference_patch_black", [(5.316, 1.438, -2.56), (5.307, 1.398, -2.63),
          (5.307, 1.335, -2.664), (5.310, 1.249, -2.651), (5.316, 1.218, -2.598)])
    curve("IF_monitor_patch_blue", [(5.316, 1.438, -2.365), (5.306, 1.408, -2.389),
          (5.306, 1.380, -2.440), (5.316, 1.438, -2.465)], .0023, "CableBlue")
    # Keep the service sheet's complete .26x.34m interaction envelope free.
    # The unobtrusive vent column lies to its right, away from the power lever.
    for k in range(14):
        box("Service_side_vent_%02d" % k, (5.3445, .544+k*.014, -2.235), (.0016, .005, .085), "Bakelite", .001)
    meter("Regulator_output", .403, -.126)
    for u in (.080, .161):
        cylinder("Fuse_carrier_%s" % u, (5.337, .381, CENTRE_Z+u), .014, .016, "Bakelite", 24)
        box("Fuse_slot_%s" % u, (5.328, .381, CENTRE_Z+u), (.001, .003, .019), "Aluminium", .0004)
    toggle("Aux_supply", .300, .080)
    toggle("Meter_test", .300, .161)
    for y in (.13, .151, .172, .193):
        box("Lower_air_intake_%.3f" % y, (5.3443, y, CENTRE_Z), (.002, .0075, .332), "Bakelite", .0013)
        box("Lower_air_louvre_%.3f" % y, (5.341, y+.0045, CENTRE_Z), (.005, .0025, .332), "DarkSteel", .0006)

    # Cabinet depth is the existing .62m housing, with new folded steel skin
    # and pressed side louvers; no enlarged collision or hidden original body.
    box("RX3_folded_top", (5.66, 1.954, CENTRE_Z), (.625, .007, .626), "DarkSteel", .0013)
    for side in (-1, 1):
        z = CENTRE_Z+side*.312
        box("RX3_side_skin_%d" % side, (5.668, .982, z), (.597, 1.89, .003), "DarkSteel", .0006)
        for y in (.11, 1.84):
            for x in (5.40, 5.92):
                box("RX3_side_fastener_%d_%.2f_%.2f" % (side, x, y), (x, y, z+side*.002), (.007, .007, .001), "Aluminium", .0005)
        for k in range(12):
            y = 1.53+k*.021
            box("RX3_side_vent_recess_%d_%02d" % (side, k), (5.714, y, z+side*.0017), (.34, .008, .001), "Bakelite", .001)
            box("RX3_side_louver_%d_%02d" % (side, k), (5.714, y+.005, z+side*.003), (.34, .0025, .007), "DarkSteel", .0007)

    # Give all untextured hard-surface parts useful UVs before the native join.
    for obj in objects:
        active(obj)
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        if not obj.data.uv_layers:
            bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.select_all(action="SELECT")
            bpy.ops.uv.cube_project(cube_size=1)
            bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.duplicate(); copies = list(bpy.context.selected_objects)
    bpy.context.view_layer.objects.active = copies[0]; bpy.ops.object.join()
    mesh = bpy.context.object; mesh.name = NAME
    for collection in list(mesh.users_collection):
        collection.objects.unlink(mesh)
    export.objects.link(mesh); active(mesh)
    tri = mesh.modifiers.new("Native_triangles", "TRIANGULATE"); bpy.ops.object.modifier_apply(modifier=tri.name)
    triangles = len(mesh.data.polygons)
    bounds = [[float(fn(v.co[k] for v in mesh.data.vertices)) for k in range(3)] for fn in (min, max)]
    bpy.ops.export_scene.fbx(filepath=str(root/"Export"/(NAME+".fbx")),
        use_selection=True, object_types={"MESH"}, apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y", axis_up="Z",
        bake_anim=False, mesh_smooth_type="FACE")
    contract = {"asset": NAME, "triangles": triangles, "bounds_local_m": bounds,
        "materials": [m.name for m in mesh.data.materials], "reuse_materials": {},
        "units": "metres", "mapping": "Blender(x,-z,y) -> UE(100x,100z,100y)",
        "source": "rx3.py", "editable_components": len(objects), "rack_width_m": WIDTH,
        "rack_unit_m": U, "rack_units": 42, "collision": "NoCollision; original RX3 gameplay retained",
        "protected": {"live_leds": "24 existing16mm squares; no replacement or new emissive status",
            "led_rows_y_m": [1.55, 1.33, 1.11, .89], "led_z_m": [-2.62+k*.068 for k in range(6)],
            "led_front_x_m": 5.3445, "new_front_x_m": 5.3455,
            "lever_free_yz_m": [[.91, -2.32], [1.25, -2.14]],
            "service_paper_free_yz_m": [[.51, -2.60], [.86, -2.34]]},
        "passive_meters": "needles parked at mechanical left stop, no new powered reading",
        "reference": "HP8640B manufacturer manual; original fictional artwork, no copied branding/assets",
        "reference_url": "https://www.keysight.com/zz/en/assets/9018-05715/user-manuals/9018-05715.pdf"}
    (root/"rx3-contract.json").write_text(json.dumps(contract, indent=2)+"\n")
    (root/"rx3-materials.json").write_text(json.dumps(rows(), indent=2)+"\n")
    mesh.hide_set(True); mesh.hide_render = True
    for image in bpy.data.images:
        if image.source == "FILE":
            image.pack()
    bpy.ops.file.make_paths_relative()
    bpy.ops.wm.save_as_mainfile(filepath=str(root/"Signal47_RX3Detail.blend"))
    print("RX3 DETAIL READY", json.dumps({"triangles": triangles, "components": len(objects)}))
    return contract


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--textures", action="store_true")
    parser.add_argument("--output", type=Path, default=ROOT)
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [] if "bpy" in sys.modules else None)
    if args.textures:
        print(textures(args.output))
    else:
        build(args.output)


if __name__ == "__main__":
    main()
