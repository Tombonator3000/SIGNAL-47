"""Original SC02 pale reactive actor: six rigid body parts with two LODs each.

Blender CPU authoring only. Every shape below is placed by hand from numbers in
this file; no library mesh, purchased asset, scan or reference image is used.
Runs unchanged in Blender 4.5 LTS (``blender -b -P build_scene.py``) and in the
bpy 5.1 module (``python build_scene.py``). Writes Export/*.fbx,
SC02_PaleActor.blend, materials.json and model_contract.json next to itself.
"""
from pathlib import Path
import hashlib
import json
import math
import bpy  # first: the bpy module provides bmesh and mathutils
import bmesh
from mathutils import Euler, Matrix, Vector, noise

ROOT = Path(__file__).resolve().parent
EXPORT = ROOT / "Export"
EXPORT.mkdir(exist_ok=True)

# Same base colour as M_SC01_Skin, so the body matches the SC01 hand the player
# has already seen; lower roughness for a damp sheen. Mottling and veins are in
# the vertex colour.
MATERIAL_ROWS = [
    {"name": "M_SC02_PaleSkin", "base_color_linear": [.59, .62, .54, 1], "roughness": .56,
     "specular": .34, "metallic": 0, "derived_from": "M_SC01_Skin",
     "vertex_color_multiply": True},
]
SKIN = MATERIAL_ROWS[0]["name"]

# Contract joints (Blender metres, Z up, facing +X, feet at z=0). Limbs are the
# left side (+Y); the game mirrors them with scale Y = -1 for the right side.
PARTS = {
    "SM_SC02_PaleTorso": {"origin": (0, 0, 1.02), "voxel": .0055, "lod0": 3600},
    "SM_SC02_PaleHead": {"origin": (0, 0, 1.70), "voxel": .0035, "lod0": 2300},
    "SM_SC02_PaleUpperArm": {"origin": (0, .21, 1.60), "voxel": .0045, "lod0": 520},
    "SM_SC02_PaleLowerArm": {"origin": (0, .21, 1.24), "voxel": .0021, "lod0": 1720},
    "SM_SC02_PaleThigh": {"origin": (0, .10, 1.00), "voxel": .0055, "lod0": 560},
    "SM_SC02_PaleShin": {"origin": (0, .10, .51), "voxel": .0030, "lod0": 1080},
}
MIRRORED = ("SM_SC02_PaleUpperArm", "SM_SC02_PaleLowerArm", "SM_SC02_PaleThigh", "SM_SC02_PaleShin")
LOD1_FRACTION = .30
LOD0_BODY_BUDGET = 14000


def reset_scene():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for block in (bpy.data.meshes, bpy.data.materials, bpy.data.cameras, bpy.data.lights):
        for item in list(block):
            block.remove(item)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0


def make_material(row):
    mat = bpy.data.materials.new(row["name"])
    mat.diffuse_color = row["base_color_linear"]
    if mat.node_tree is None:  # Blender 4.5; 5.x materials always own a node tree.
        mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Roughness"].default_value = row["roughness"]
    bsdf.inputs["Metallic"].default_value = row["metallic"]
    bsdf.inputs["Specular IOR Level"].default_value = row["specular"]
    # Same graph as the Unreal import: authored colour times vertex colour.
    attr = nodes.new("ShaderNodeAttribute")
    attr.attribute_name = "Col"
    tint = nodes.new("ShaderNodeVectorMath")
    tint.operation = "MULTIPLY"
    tint.inputs[1].default_value = row["base_color_linear"][:3]
    links.new(attr.outputs["Color"], tint.inputs[0])
    links.new(tint.outputs["Vector"], bsdf.inputs["Base Color"])
    return mat


# ---------------------------------------------------------------- shapes
class Shells:
    """Overlapping closed shells, fused later by the voxel remesher."""

    def __init__(self):
        self.bm = bmesh.new()

    def ell(self, c, r, rot=(0, 0, 0), seg=(24, 14)):
        m = Matrix.Translation(c) @ Euler(rot).to_matrix().to_4x4() @ Matrix.Diagonal((r[0], r[1], r[2], 1))
        bmesh.ops.create_uvsphere(self.bm, u_segments=seg[0], v_segments=seg[1], radius=1, matrix=m)

    def seg(self, a, b, ra, rb, caps=True):
        a, b = Vector(a), Vector(b)
        d = b - a
        rot = Vector((0, 0, 1)).rotation_difference(d).to_matrix().to_4x4()
        m = Matrix.Translation((a + b) / 2) @ rot
        bmesh.ops.create_cone(self.bm, cap_ends=True, cap_tris=False, segments=20,
                              radius1=ra, radius2=rb, depth=d.length, matrix=m)
        if caps:
            self.ell(a, (ra, ra, ra), seg=(20, 10))
            self.ell(b, (rb, rb, rb), seg=(20, 10))

    def chain(self, points, radii):
        for i in range(len(points) - 1):
            self.seg(points[i], points[i + 1], radii[i], radii[i + 1])

    def loft(self, rings, segments=32):
        """Closed tube through horizontal rings (z, cx, cy, front x radius, back x radius, y radius)."""
        loops = []
        for z, cx, cy, rxf, rxb, ry in sorted(rings):
            loop = []
            for k in range(segments):
                t = 2 * math.pi * k / segments
                c, s = math.cos(t), math.sin(t)
                loop.append(self.bm.verts.new((cx + (rxf if c >= 0 else rxb) * c, cy + ry * s, z)))
            loops.append(loop)
        for lo, hi in zip(loops, loops[1:]):
            for k in range(segments):
                self.bm.faces.new((lo[k], lo[(k + 1) % segments], hi[(k + 1) % segments], hi[k]))
        self.bm.faces.new(list(reversed(loops[0])))
        self.bm.faces.new(loops[-1])


def hunch(z):
    """Forward (+X) offset of the spine at height z: a starved, rounded upper back."""
    return .12 * max(0.0, (z - 1.20) / .50) ** 2


NECK_FORWARD = hunch(1.70)
SHOULDER_FORWARD = hunch(1.60)


def ring_at(rings, z):
    """Interpolated loft ring at height z."""
    rings = sorted(rings)
    z = min(max(z, rings[0][0]), rings[-1][0])
    for lo, hi in zip(rings, rings[1:]):
        if z <= hi[0]:
            f = (z - lo[0]) / (hi[0] - lo[0])
            return [a + (b - a) * f for a, b in zip(lo, hi)]
    return list(rings[-1])


def on_surface(rings, z, t, inset=0.0):
    """Point on a loft surface at height z and angle t (0 = front +X, pi/2 = +Y)."""
    _, cx, cy, rxf, rxb, ry = ring_at(rings, z)
    c, s = math.cos(t), math.sin(t)
    rx = rxf if c >= 0 else rxb
    return Vector((cx + (rx - inset) * c, cy + (ry - inset) * s, z))


# Long, starved trunk: deep ribcage over a caved-in waist, narrow hips.
TORSO = [  # z, cx, cy, rx front, rx back, ry
    (.915, -.012, 0, .034, .046, .044),
    (.95, -.012, 0, .050, .080, .092),
    (.985, -.012, 0, .056, .088, .124),
    (1.02, -.012, 0, .056, .088, .136),
    (1.06, -.014, 0, .052, .084, .130),
    (1.10, -.018, 0, .044, .080, .116),
    (1.16, -.018, 0, .042, .080, .104),
    (1.22, -.012, 0, .066, .088, .118),
    (1.28, -.004, 0, .094, .098, .138),
    (1.36, 0, 0, .096, .098, .142),
    (1.44, -.004, 0, .094, .100, .146),
    (1.50, -.010, 0, .092, .102, .150),
    (1.55, -.016, 0, .076, .092, .164),
    (1.59, -.020, 0, .058, .074, .176),
    (1.608, -.017, 0, .056, .062, .152),
    (1.628, -.013, 0, .052, .054, .112),
    (1.648, -.010, 0, .044, .046, .066),
]


def torso_shells():
    s = Shells()
    s.loft(TORSO)
    for sy in (1, -1):
        # Bony pelvis instead of a smooth mannequin hip: iliac crests curving round
        # the front, the top of each thigh bone at the side and a pubic ridge.
        s.chain([on_surface(TORSO, 1.085, sy * math.radians(a), .001) for a in (60, 80, 100, 120)],
                [.011, .013, .012, .010])
        s.ell(on_surface(TORSO, 1.05, sy * math.radians(30), .004), (.013, .016, .018))   # front hip spine
        s.ell((-.004, .145 * sy, .975), (.022, .020, .026))                               # greater trochanter
        s.seg(on_surface(TORSO, 1.06, sy * math.radians(32), .006),
              on_surface(TORSO, .945, sy * math.radians(8), .004), .008, .007)         # inguinal ridge
    s.ell(on_surface(TORSO, .935, 0, .006), (.014, .036, .012))                         # pubic ridge
    # Ribs stand proud of the cage on each side of a bare sternum.
    for i in range(8):
        zf = 1.24 + i * .033
        for sy in (1, -1):
            pts = []
            for k in range(11):
                t = math.radians(22 + k * 11) * sy
                z = zf + .08 * (1 - math.cos(t)) / 2
                pts.append(on_surface(TORSO, z, t, .002))
            s.chain(pts, [.0064 - .0012 * (i > 5)] * len(pts))
    for k in range(9):  # sternum
        s.ell(on_surface(TORSO, 1.33 + k * .028, 0, .006), (.012, .016, .02))
    for sy in (1, -1):
        # S-curved collarbones lying on the chest from sternum to shoulder.
        s.chain([on_surface(TORSO, 1.585, sy * math.radians(14), .002),
                 on_surface(TORSO, 1.595, sy * math.radians(48), .002),
                 Vector((-.004, .186 * sy, 1.612))], [.0095, .0085, .011])
        s.ell((-.002, .194 * sy, 1.606), (.036, .030, .028))           # acromion
        s.seg((-.022, .028 * sy, 1.668), (-.02, .16 * sy, 1.612), .022, .018)  # trapezius
        s.ell(on_surface(TORSO, 1.46, sy * math.radians(150), .011), (.016, .055, .08),
              (.22 * sy, 0, -.35 * sy))                                # scapulae
        s.seg(on_surface(TORSO, 1.588, sy * math.radians(8), .004),
              (.02, .028 * sy, 1.64), .010, .009)                      # neck tendons
        s.ell(on_surface(TORSO, 1.055, sy * math.radians(38), .008), (.014, .02, .014))  # hip bone points
    z = 1.04
    while z < 1.64:  # spine knuckles down the back
        s.ell(on_surface(TORSO, z, math.pi, .0065), (.010, .009, .010))
        z += .032
    s.seg((0, 0, 1.58), (0, 0, 1.69), .026, .025)                      # neck core, hidden in the head's neck
    for v in s.bm.verts:
        v.co.x += hunch(v.co.z)
    return s


HEAD = [  # narrow, too-long human skull with a real jaw and chin, not a bulb over a small jaw
    (1.748, .046, 0, .016, .014, .020),
    (1.760, .040, 0, .030, .030, .034),
    (1.780, .028, 0, .046, .050, .046),
    (1.808, .016, 0, .058, .064, .052),
    (1.840, .008, 0, .070, .076, .054),
    (1.872, -.002, 0, .078, .084, .058),
    (1.902, -.008, 0, .080, .086, .062),
    (1.932, -.012, 0, .076, .082, .060),
    (1.956, -.016, 0, .064, .070, .052),
    (1.974, -.020, 0, .040, .046, .034),
    (1.984, -.022, 0, .014, .016, .012),
]


def head_shells():
    s = Shells()
    # The neck sleeve is centred on the turn axis and reaches down into the
    # torso's shoulder slope, so any yaw keeps the junction closed.
    s.chain([Vector((-.012, 0, 1.625)), Vector((0, 0, 1.76)), Vector((-.006, 0, 1.80))], [.040, .036, .032])
    s.loft(HEAD)
    for sy in (1, -1):
        s.seg((.022, .024 * sy, 1.645), (-.004, .036 * sy, 1.79), .009, .008)  # tendons
        s.seg(on_surface(HEAD, 1.83, sy * math.radians(105), .006),
              on_surface(HEAD, 1.768, sy * math.radians(25), .004), .009, .007)  # jaw line
        s.ell(on_surface(HEAD, 1.86, sy * math.radians(48), .008), (.016, .013, .009))  # cheekbones
    s.chain([on_surface(HEAD, 1.906, math.radians(-40), .004), on_surface(HEAD, 1.910, 0, .004),
             on_surface(HEAD, 1.906, math.radians(40), .004)], [.010, .011, .010])  # heavy brow ridge
    for sy in (1, -1):
        s.ell(on_surface(HEAD, 1.79, sy * math.radians(80), .006), (.016, .012, .018))  # jaw angle
    s.ell(on_surface(HEAD, 1.756, 0, .004), (.016, .020, .012))                     # chin
    s.seg(on_surface(HEAD, 1.900, 0, .003), on_surface(HEAD, 1.852, 0, -.010), .007, .009)  # nose bridge
    return s


HEAD_CARVES = [  # (centre, radius, depth, pull-towards) pushes skin into the skull
    ((.082, .028, 1.884), .017, .026, (-.03, .02, 1.884)),            # small, deep eye sockets
    ((.082, -.028, 1.884), .017, .026, (-.03, -.02, 1.884)),
    ((.078, .010, 1.802), .012, .006, (0, 0, 1.802)),                 # lipless mouth line
    ((.078, -.010, 1.802), .012, .006, (0, 0, 1.802)),
    ((.080, 0, 1.802), .011, .006, (0, 0, 1.802)),
    ((.05, .046, 1.83), .024, .011, (0, 0, 1.83)),                    # hollow cheeks
    ((.05, -.046, 1.83), .024, .011, (0, 0, 1.83)),
    ((.04, .074, 1.905), .026, .007, (-.02, 0, 1.905)),               # sunken temples
    ((.04, -.074, 1.905), .026, .007, (-.02, 0, 1.905)),
]

UPPER_ARM = [
    (1.222, -.004, .21, .016, .020, .021),
    (1.24, -.004, .21, .022, .028, .029),
    (1.27, -.003, .21, .023, .026, .029),
    (1.33, 0, .21, .024, .024, .023),
    (1.42, .003, .21, .028, .025, .024),
    (1.50, 0, .21, .029, .029, .027),
    (1.55, 0, .212, .034, .034, .033),
    (1.60, 0, .21, .036, .036, .037),
]


def upper_arm_shells():
    s = Shells()
    s.loft(UPPER_ARM)
    s.ell((0, .206, 1.60), (.038, .038, .042))                        # rounded shoulder
    s.ell((-.004, .21, 1.245), (.026, .03, .026))                     # elbow end
    for dy in (.026, -.026):
        s.ell((-.006, .21 + dy, 1.25), (.011, .011, .011))            # epicondyles
    return s


FINGERS = [  # base x, length, fan dx over length: long enough to hang well down the thigh
    (.033, .140, .016), (.011, .156, .005), (-.011, .146, -.006), (-.031, .120, -.016)]
LOWER_ARM = [
    (.925, .004, .207, .040, .040, .0092),
    (.95, .006, .207, .043, .043, .0110),
    (.99, .007, .207, .036, .036, .0118),
    (1.015, .006, .208, .021, .021, .0115),
    (1.03, .006, .21, .019, .019, .0125),
    (1.07, .005, .21, .018, .018, .0150),
    (1.14, .003, .21, .023, .023, .0230),
    (1.20, 0, .21, .027, .028, .0270),
    (1.25, -.004, .21, .024, .030, .0290),
    (1.268, -.004, .21, .017, .021, .0210),
]


def lower_arm_shells():
    s = Shells()
    s.loft(LOWER_ARM)
    s.ell((-.002, .21, 1.24), (.027, .031, .028))                     # elbow ball
    s.ell((-.024, .21, 1.232), (.015, .016, .02))                     # olecranon
    s.ell((-.010, .214, 1.024), (.006, .006, .007))                   # ulna knob
    for x, length, fan in FINGERS:
        s.ell((x, .212, .928), (.0098, .0088, .010))                  # knuckle on the back
        s.seg((x * .55, .214, 1.005), (x, .213, .935), .0035, .0045)  # tendon on the back
        pts, radii = [], []
        for f, dy, r in ((0, 0, .0078), (.42, -.004, .0068), (.73, -.010, .0057), (1, -.018, .0044)):
            pts.append(Vector((x + fan * f, .206 + dy, .928 - length * f)))
            radii.append(r)
        s.chain(pts, radii)
        for p, r in zip(pts[1:-1], radii[1:-1]):
            s.ell(p, (r * 1.2, r * 1.15, r * 1.15))                   # bony finger joints
    s.chain([Vector((.030, .203, .998)), Vector((.054, .197, .958)), Vector((.066, .192, .922)),
             Vector((.072, .187, .893))], [.0115, .0090, .0070, .0052])  # thumb
    return s


THIGH = [
    (.495, .005, .10, .028, .030, .032),
    (.52, .005, .10, .038, .040, .044),
    (.58, .006, .10, .034, .034, .036),
    (.68, .012, .10, .038, .034, .036),
    (.80, .016, .10, .044, .040, .042),
    (.90, .010, .101, .050, .048, .049),
    (.98, .002, .103, .052, .056, .052),
]


def thigh_shells():
    s = Shells()
    s.loft(THIGH)
    s.ell((0, .10, 1.0), (.05, .05, .05))                             # hip ball, buried in the pelvis
    s.ell((.005, .10, .515), (.036, .040, .03))                       # knee end
    s.ell((.036, .10, .53), (.014, .019, .021))                       # kneecap
    return s


SHIN = [
    (.075, -.004, .10, .019, .022, .022),
    (.11, -.005, .10, .017, .020, .019),
    (.20, -.002, .10, .019, .024, .021),
    (.30, .002, .10, .022, .034, .026),
    (.40, .004, .10, .026, .040, .031),
    (.46, .008, .10, .034, .038, .040),
    (.51, .005, .10, .038, .040, .044),
    (.535, .005, .10, .029, .030, .033),
]


def shin_shells():
    s = Shells()
    s.loft(SHIN)
    s.ell((.003, .10, .51), (.036, .040, .036))                       # knee ball
    s.seg((.024, .10, .46), (.015, .10, .13), .008, .006)             # shin edge
    s.ell((-.004, .10, .068), (.026, .028, .028))                     # ankle
    for dy in (.025, -.025):
        s.ell((-.004, .10 + dy, .07), (.011, .011, .011))             # malleoli
    s.seg((-.026, .10, .20), (-.033, .10, .055), .010, .010)          # Achilles tendon
    s.ell((-.038, .10, .032), (.032, .026, .032))                     # heel
    s.ell((.030, .10, .026), (.064, .034, .022))                      # bony, shorter foot
    s.seg((0, .10, .062), (.095, .10, .024), .021, .015)              # instep
    for dy in (-.018, -.004, .010, .022):
        s.seg((.01, .10 + dy * .5, .05), (.09, .10 + dy, .026), .0045, .004)  # metatarsal ridges
    # Toes with knuckles; the big toe is medial (towards -Y for the left foot).
    for dy, length, r in ((-.022, .046, .0102), (-.007, .040, .0076), (.006, .037, .0070),
                          (.018, .032, .0066), (.029, .027, .0062)):
        y = .10 + dy
        pts = [Vector((.088, y, .02)), Vector((.088 + length * .5, y + dy * .1, .017)),
               Vector((.088 + length, y + dy * .16, r + .0004))]
        s.chain(pts, [r * 1.1, r, r * .85])
        s.ell(pts[1], (r * 1.25, r * 1.2, r * 1.3))
    return s


BUILDERS = {
    "SM_SC02_PaleTorso": torso_shells, "SM_SC02_PaleHead": head_shells,
    "SM_SC02_PaleUpperArm": upper_arm_shells, "SM_SC02_PaleLowerArm": lower_arm_shells,
    "SM_SC02_PaleThigh": thigh_shells, "SM_SC02_PaleShin": shin_shells,
}
TORSO_CARVES = [  # starved belly and groin hollows between the hip bones
    ((.07, .075, .955), .040, .016, (0, .02, .98)),
    ((.07, -.075, .955), .040, .016, (0, -.02, .98)),
    ((.07, 0, 1.03), .060, .012, (-.05, 0, 1.03)),
]
CARVES = {"SM_SC02_PaleHead": HEAD_CARVES, "SM_SC02_PaleTorso": TORSO_CARVES}


# ---------------------------------------------------------------- mesh steps
def apply_modifiers(obj):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(obj.evaluated_get(depsgraph))
    old = obj.data
    obj.modifiers.clear()
    obj.data = mesh
    bpy.data.meshes.remove(old)


def triangulate(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    bm.to_mesh(mesh)
    bm.free()


def carve(mesh, carves):
    for v in mesh.vertices:
        p = v.co.copy()
        for centre, radius, depth, pull in carves:
            d = (p - Vector(centre)).length
            if d < radius:
                w = (1 - (d / radius) ** 2) ** 2
                p += (Vector(pull) - p).normalized() * depth * w
        v.co = p


def skin(p):
    """Mottled, bloodless skin with faint blue veins. Deterministic noise in body space."""
    m = .90 + .10 * noise.noise(p * 9.0) + .05 * noise.noise(p * 31.0)
    vein = max(0.0, 1 - abs(noise.noise(p * 14.0 + Vector((3.1, 7.7, 1.3)))) / .06)
    return (m * (1 - .16 * vein), m * (1 - .10 * vein), m * (1 - .02 * vein))


def tint(name, p):
    """Linear vertex colour multiplier: mottled skin, dark sockets and bruised extremities."""
    r, g, b = skin(p)
    if name == "SM_SC02_PaleHead":
        value = 1.0
        for sy in (1, -1):
            d = (p - Vector((.066, .028 * sy, 1.884))).length
            value = min(value, .02 + .98 * min(1, max(0, (d - .010) / .016)) ** 1.5)
        mouth = (p - Vector((.074, 0, 1.802))).length
        value *= .55 + .45 * min(1, max(0, (mouth - .004) / .016))
        return (r * value, g * value * .98, b * value)
    if name == "SM_SC02_PaleLowerArm" and p.z < .90:
        f = min(1, (.90 - p.z) / .12)
        return (r * (1 - .26 * f), g * (1 - .30 * f), b * (1 - .18 * f))
    if name == "SM_SC02_PaleShin" and p.x > .08:
        f = min(1, (p.x - .08) / .05)
        return (r * (1 - .18 * f), g * (1 - .22 * f), b * (1 - .14 * f))
    return (r, g, b)


def finish_surface(obj, name, world_origin):
    """Vertex colour, box-projected UVs (tangents and lightmap layout) and smooth shading."""
    mesh = obj.data
    while mesh.color_attributes:
        mesh.color_attributes.remove(mesh.color_attributes[0])
    while mesh.uv_layers:
        mesh.uv_layers.remove(mesh.uv_layers[0])
    col = mesh.color_attributes.new("Col", "BYTE_COLOR", "CORNER")
    uv = mesh.uv_layers.new(name="UVMap")
    offset = Vector(world_origin)
    for poly in mesh.polygons:
        n = poly.normal
        axis = max(range(3), key=lambda k: abs(n[k]))
        a, b = [k for k in range(3) if k != axis]
        for li in poly.loop_indices:
            co = mesh.vertices[mesh.loops[li].vertex_index].co
            world = co + offset
            col.data[li].color = (*tint(name, world), 1.0)
            uv.data[li].uv = (co[a] * 2 + axis, co[b] * 2)
    mesh.polygons.foreach_set("use_smooth", [True] * len(mesh.polygons))
    mesh.update()


def build_part(name, spec, mat):
    shells = BUILDERS[name]()
    mesh = bpy.data.meshes.new(name)
    shells.bm.to_mesh(mesh)
    shells.bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    remesh = obj.modifiers.new("Fused original skin", "REMESH")
    remesh.mode = "VOXEL"
    remesh.voxel_size = spec["voxel"]
    remesh.adaptivity = 0
    smooth = obj.modifiers.new("Skin relaxation", "SMOOTH")
    smooth.factor = .5
    smooth.iterations = 4
    apply_modifiers(obj)
    if name in CARVES:
        carve(obj.data, CARVES[name])
        smooth = obj.modifiers.new("Carve relaxation", "SMOOTH")
        smooth.factor = .4
        smooth.iterations = 2
        apply_modifiers(obj)
    triangulate(obj.data)
    dec = obj.modifiers.new("Budgeted silhouette", "DECIMATE")
    dec.decimate_type = "COLLAPSE"
    dec.ratio = spec["lod0"] / len(obj.data.polygons)
    apply_modifiers(obj)
    triangulate(obj.data)
    if name == "SM_SC02_PaleShin":
        for v in obj.data.vertices:  # sole stands exactly on the floor
            if v.co.z < 0:
                v.co.z = 0
    # Joint origin: mesh data is moved so the proximal joint sits at (0, 0, 0).
    obj.data.transform(Matrix.Translation(-Vector(spec["origin"])))
    obj.data.materials.append(mat)
    finish_surface(obj, name, spec["origin"])

    lod1 = bpy.data.objects.new(name + "_LOD1", obj.data.copy())
    lod1.data.name = name + "_LOD1"
    bpy.context.scene.collection.objects.link(lod1)
    dec = lod1.modifiers.new("Distant silhouette", "DECIMATE")
    dec.decimate_type = "COLLAPSE"
    dec.ratio = LOD1_FRACTION
    apply_modifiers(lod1)
    triangulate(lod1.data)
    finish_surface(lod1, name, spec["origin"])
    return obj, lod1


def describe(obj):
    vs = [v.co for v in obj.data.vertices]
    return {"vertices": len(vs), "triangles": len(obj.data.polygons),
            "materials": [m.name for m in obj.data.materials],
            "bounds_local_m": [[round(min(v[k] for v in vs), 6) for k in range(3)],
                               [round(max(v[k] for v in vs), 6) for k in range(3)]]}


def export_fbx(obj, path):
    for other in bpy.context.scene.objects:
        other.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={"MESH"},
                             apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
                             axis_forward="-Y", axis_up="Z", bake_space_transform=False,
                             use_mesh_modifiers=True, add_leaf_bones=False, path_mode="AUTO",
                             colors_type="SRGB")


def main():
    reset_scene()
    mat = make_material(MATERIAL_ROWS[0])
    contract = {
        "authorship": "Original procedural Blender geometry for SIGNAL / 47, no external model sources.",
        "axes": "Blender metric Z-up, facing +X; native UE import x,-y,z in centimetres.",
        "blender_version": bpy.app.version_string,
        "origin": "Each mesh has its proximal joint at local (0,0,0). Limbs are the left side (+Y); the right side is the same mesh with scale Y = -1.",
        "joint_offsets_cm_ue": {"hip_centre": [0, 0, 102], "neck": [round(NECK_FORWARD * 100, 1), 0, 170],
                                "shoulder": [round(SHOULDER_FORWARD * 100, 1), -21, 160],
                                "elbow": "shoulder + (0,0,-36)", "hip": [0, -10, 100], "knee": "hip + (0,0,-49)",
                                "note": "UE y is negated Blender y; the left (+Y Blender) limb sits at UE y -21/-10."},
        "native_lod_profile": {"lod_count": 2, "lod1_source": "authored *_LOD1.fbx, Blender collapse decimate",
                               "lod1_fraction": LOD1_FRACTION, "lod_screen_sizes_requested": [1.0, .04],
                               "nanite_enabled": False, "separate_rt_proxy_enabled": False,
                               "collision": "none on the mesh; gameplay uses its own capsule"},
        "parts": {},
    }
    objects = []
    for name, spec in PARTS.items():
        lod0, lod1 = build_part(name, spec, mat)
        row = describe(lod0)
        low = describe(lod1)
        row.update({"origin_world_m": list(spec["origin"]), "mirrored_pair": name in MIRRORED,
                    "lod1_vertices": low["vertices"], "lod1_triangles": low["triangles"],
                    "lod1_bounds_local_m": low["bounds_local_m"]})
        contract["parts"][name] = row
        objects += [lod0, lod1]
    for f in EXPORT.glob("*.fbx"):
        f.unlink()
    for obj in objects:
        export_fbx(obj, EXPORT / (obj.name + ".fbx"))
    count = lambda key: sum(r[key] * (2 if n in MIRRORED else 1) for n, r in contract["parts"].items())
    contract["body_totals"] = {"lod0_triangles": count("triangles"), "lod1_triangles": count("lod1_triangles"),
                               "lod0_budget": LOD0_BODY_BUDGET,
                               "counting": "torso + head + two of each limb part"}
    assert contract["body_totals"]["lod0_triangles"] <= LOD0_BODY_BUDGET, contract["body_totals"]

    # The .blend shows the assembled rest pose: LOD0 at the joints, LOD1 hidden.
    lod1_col = bpy.data.collections.new("LOD1")
    bpy.context.scene.collection.children.link(lod1_col)
    for obj in objects:
        base = obj.name.replace("_LOD1", "")
        x, y, z = PARTS[base]["origin"]
        x += {"SM_SC02_PaleHead": NECK_FORWARD, "SM_SC02_PaleUpperArm": SHOULDER_FORWARD}.get(base, 0)
        # The elbow hangs below the forward shoulder.
        x += SHOULDER_FORWARD if base == "SM_SC02_PaleLowerArm" else 0
        obj.location = (x, y, z)
        if obj.name.endswith("_LOD1"):
            bpy.context.scene.collection.objects.unlink(obj)
            lod1_col.objects.link(obj)
            obj.hide_render = True
            obj.hide_set(True)
    bpy.context.preferences.filepaths.save_version = 0  # no .blend1 backups next to the source
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / "SC02_PaleActor.blend"))
    backup = ROOT / "SC02_PaleActor.blend1"
    if backup.exists():
        backup.unlink()
    (ROOT / "materials.json").write_text(json.dumps(MATERIAL_ROWS, indent=2) + "\n")
    contract["fbx_sha256"] = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(EXPORT.glob("*.fbx"))}
    (ROOT / "model_contract.json").write_text(json.dumps(contract, indent=2) + "\n")
    print("SC02 ORIGINAL GEOMETRY EXPORTED", json.dumps(contract["body_totals"]))
    for name, row in contract["parts"].items():
        print(name, row["triangles"], row["lod1_triangles"], row["bounds_local_m"])


main()
