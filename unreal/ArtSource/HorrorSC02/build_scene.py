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
from mathutils import Euler, Matrix, Vector

ROOT = Path(__file__).resolve().parent
EXPORT = ROOT / "Export"
EXPORT.mkdir(exist_ok=True)

# Derived from M_SC01_Skin (.59, .62, .54; roughness .74; specular .26): lifted
# towards bloodless white with less green, lower roughness for a damp sheen.
MATERIAL_ROWS = [
    {"name": "M_SC02_PaleSkin", "base_color_linear": [.68, .70, .65, 1], "roughness": .56,
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
    (.915, -.012, 0, .040, .050, .060),
    (.95, -.012, 0, .064, .080, .130),
    (.985, -.012, 0, .070, .088, .150),
    (1.02, -.012, 0, .068, .088, .146),
    (1.06, -.014, 0, .062, .084, .128),
    (1.10, -.018, 0, .046, .080, .106),
    (1.16, -.018, 0, .044, .080, .100),
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
    return s


HEAD = [  # long narrow skull over a small receding jaw
    (1.752, .028, 0, .010, .014, .014),
    (1.765, .022, 0, .024, .030, .026),
    (1.787, .012, 0, .040, .048, .040),
    (1.815, .004, 0, .056, .066, .047),
    (1.845, .002, 0, .078, .080, .051),
    (1.875, -.006, 0, .088, .090, .060),
    (1.905, -.012, 0, .092, .094, .067),
    (1.935, -.016, 0, .089, .092, .068),
    (1.962, -.020, 0, .077, .082, .062),
    (1.984, -.024, 0, .053, .058, .045),
    (1.997, -.026, 0, .022, .024, .018),
]


def head_shells():
    s = Shells()
    # The neck sleeve is centred on the turn axis and reaches down into the
    # torso's shoulder slope, so any yaw keeps the junction closed.
    s.chain([Vector((0, 0, 1.625)), Vector((0, 0, 1.76)), Vector((-.006, 0, 1.80))], [.032, .030, .030])
    s.loft(HEAD)
    for sy in (1, -1):
        s.seg((.022, .024 * sy, 1.645), (-.004, .036 * sy, 1.79), .009, .008)  # tendons
        s.seg(on_surface(HEAD, 1.83, sy * math.radians(105), .006),
              on_surface(HEAD, 1.768, sy * math.radians(25), .004), .009, .007)  # jaw line
        s.ell(on_surface(HEAD, 1.86, sy * math.radians(48), .008), (.016, .013, .009))  # cheekbones
    s.chain([on_surface(HEAD, 1.912, math.radians(-38), .006), on_surface(HEAD, 1.916, 0, .006),
             on_surface(HEAD, 1.912, math.radians(38), .006)], [.007, .008, .007])  # faint brow
    return s


HEAD_CARVES = [  # (centre, radius, depth, pull-towards) pushes skin into the skull
    ((.086, .027, 1.885), .023, .023, (-.03, .02, 1.885)),            # eye sockets
    ((.086, -.027, 1.885), .023, .023, (-.03, -.02, 1.885)),
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


FINGERS = [  # base x, length, fan dx over length
    (.033, .112, .014), (.011, .124, .004), (-.011, .116, -.005), (-.031, .096, -.014)]
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
    s.ell((.045, .10, .026), (.085, .032, .022))                      # long narrow foot
    s.seg((0, .10, .062), (.13, .10, .026), .021, .014)               # instep
    # Long toes; the big toe is medial (towards -Y for the left foot).
    for dy, length, r in ((-.022, .074, .0100), (-.007, .066, .0074), (.006, .060, .0068),
                          (.018, .052, .0064), (.029, .044, .0060)):
        y = .10 + dy
        s.chain([Vector((.12, y, .02)), Vector((.12 + length * .55, y + dy * .1, .014)),
                 Vector((.12 + length, y + dy * .16, r + .0004))], [r * 1.1, r, r * .85])
    return s


BUILDERS = {
    "SM_SC02_PaleTorso": torso_shells, "SM_SC02_PaleHead": head_shells,
    "SM_SC02_PaleUpperArm": upper_arm_shells, "SM_SC02_PaleLowerArm": lower_arm_shells,
    "SM_SC02_PaleThigh": thigh_shells, "SM_SC02_PaleShin": shin_shells,
}
CARVES = {"SM_SC02_PaleHead": HEAD_CARVES}


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


def tint(name, p):
    """Linear vertex colour multiplier: dark sockets and bruised fingertips."""
    if name == "SM_SC02_PaleHead":
        value = 1.0
        for sy in (1, -1):
            d = (p - Vector((.070, .026 * sy, 1.885))).length
            value = min(value, .04 + .96 * min(1, max(0, (d - .013) / .020)) ** 1.5)
        return (value, value * .98, value * 1.0)
    if name == "SM_SC02_PaleLowerArm" and p.z < .90:
        f = min(1, (.90 - p.z) / .09)
        return (1 - .26 * f, 1 - .30 * f, 1 - .18 * f)
    if name == "SM_SC02_PaleShin" and p.x > .15:
        f = min(1, (p.x - .15) / .06)
        return (1 - .18 * f, 1 - .22 * f, 1 - .14 * f)
    return (1.0, 1.0, 1.0)


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
        "joint_offsets_cm_ue": {"hip_centre": [0, 0, 102], "neck": [0, 0, 170], "shoulder": [0, -21, 160],
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
        obj.location = PARTS[obj.name.replace("_LOD1", "")]["origin"]
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
