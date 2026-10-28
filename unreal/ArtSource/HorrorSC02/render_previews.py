"""Preview renders of the assembled SC02 actor from the saved .blend (evidence only)."""
import sys, math
import bpy
from mathutils import Vector, Euler, Matrix

ARGS = sys.argv[sys.argv.index("--"):] if "--" in sys.argv else sys.argv  # blender -b -P ... -- or bpy module
BLEND, OUT = ARGS[1], ARGS[2]
ENGINE = ARGS[3] if len(ARGS) > 3 else "EEVEE"
VIEWS = ARGS[4].split(",") if len(ARGS) > 4 else ["front", "side", "three_quarter", "head60_armup", "back"]
bpy.ops.wm.open_mainfile(filepath=BLEND)
scene = bpy.context.scene
LOD1 = {}
for obj in list(scene.objects):
    if obj.name.endswith("_LOD1"):
        LOD1[obj.name[:-5]] = obj.data
        bpy.data.objects.remove(obj, do_unlink=True)
USE_LOD1 = [False]
P = {o.name: o for o in scene.objects}
# Neck and shoulders sit forward of the hunched spine, as in the game (model_contract.json).
J = {"torso": Vector((0, 0, 1.02)), "neck": Vector((.12, 0, 1.70)), "sh": Vector((.077, .21, 1.60)),
     "hip": Vector((0, .10, 1.00))}

def place(name, side, matrix):
    src = P[name]
    if side > 0:
        obj = src
    else:
        obj = scene.objects.get(name + "_R") or bpy.data.objects.new(name + "_R", src.data)
        if obj.name not in scene.collection.objects:
            scene.collection.objects.link(obj)
    if side < 0:
        obj.data = src.data
    obj.matrix_world = Matrix.Diagonal((1, side, 1, 1)) @ matrix
    return obj

def pose(head_yaw=0.0, arm_pitch=0.0, elbow=0.0):
    place("SM_SC02_PaleTorso", 1, Matrix.Translation(J["torso"]))
    place("SM_SC02_PaleHead", 1, Matrix.Translation(J["neck"]) @ Matrix.Rotation(head_yaw, 4, "Z"))
    for side in (1, -1):
        p = arm_pitch if side > 0 else 0
        e = elbow if side > 0 else 0
        up = Matrix.Translation(J["sh"]) @ Matrix.Rotation(-p, 4, "Y")
        place("SM_SC02_PaleUpperArm", side, up)
        place("SM_SC02_PaleLowerArm", side, up @ Matrix.Translation((0, 0, -.36)) @ Matrix.Rotation(-e, 4, "Y"))
        th = Matrix.Translation(J["hip"])
        place("SM_SC02_PaleThigh", side, th)
        place("SM_SC02_PaleShin", side, th @ Matrix.Translation((0, 0, -.49)))

# Stage: dark floor, cold key from above-front, faint rim, almost black world.
bpy.ops.mesh.primitive_plane_add(size=8, location=(0, 0, 0))
floor = bpy.context.object
fm = bpy.data.materials.new("Floor")
fm.diffuse_color = (.04, .045, .045, 1)
if fm.node_tree is None:
    fm.use_nodes = True
b = next(n for n in fm.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
b.inputs["Base Color"].default_value = (.035, .04, .04, 1)
b.inputs["Roughness"].default_value = .8
floor.data.materials.append(fm)
world = scene.world or bpy.data.worlds.new("W")
scene.world = world
if world.node_tree is None:
    world.use_nodes = True
bg = next(n for n in world.node_tree.nodes if n.type == "BACKGROUND")
bg.inputs["Color"].default_value = (.012, .014, .016, 1)
bg.inputs["Strength"].default_value = 1
def light(kind, loc, energy, color, size=1.0):
    data = bpy.data.lights.new(kind + str(loc), kind)
    data.energy = energy
    data.color = color
    if hasattr(data, "size"):
        data.size = size
    obj = bpy.data.objects.new(data.name, data)
    scene.collection.objects.link(obj)
    obj.location = loc
    obj.rotation_euler = (Vector((0, 0, 1.3)) - Vector(loc)).to_track_quat("-Z", "Y").to_euler()
    return obj
light("AREA", (2.2, -1.4, 3.0), 110, (.86, .92, 1.0), 1.2)
light("AREA", (-1.8, 1.6, 2.4), 90, (.75, .85, 1.0), .6)
light("AREA", (1.5, 2.0, 1.0), 15, (1, .95, .9), 2)

cam_data = bpy.data.cameras.new("Cam")
cam_data.lens = 70
cam = bpy.data.objects.new("Cam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam
scene.render.resolution_x, scene.render.resolution_y = 720, 1080
if ENGINE == "EEVEE":
    scene.render.engine = "BLENDER_EEVEE" if "BLENDER_EEVEE" in [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties["engine"].enum_items] else "BLENDER_EEVEE_NEXT"
    scene.eevee.taa_render_samples = 32
elif ENGINE == "CYCLES":
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 48
    scene.cycles.use_denoising = True
else:
    scene.render.engine = "BLENDER_WORKBENCH"
scene.view_settings.view_transform = "AgX" if "AgX" in [i.identifier for i in scene.view_settings.bl_rna.properties["view_transform"].enum_items] else "Filmic"
scene.view_settings.look = "None"

def shoot(tag, cam_pos, target=(0, 0, 1.0), lens=70):
    cam.location = cam_pos
    cam_data.lens = lens
    cam.rotation_euler = (Vector(target) - Vector(cam_pos)).to_track_quat("-Z", "Y").to_euler()
    scene.render.filepath = f"{OUT}/sc02_{tag}.png"
    bpy.ops.render.render(write_still=True)

D = 6.2
shots = {
    "front": (dict(), (D, 0, 1.05)),
    "side": (dict(), (0, -D, 1.05)),
    "three_quarter": (dict(), (D * .72, -D * .69, 1.15)),
    "back": (dict(), (-D, 0, 1.05)),
    "head60_armup": (dict(head_yaw=math.radians(60), arm_pitch=math.radians(95), elbow=math.radians(12)), (D * .72, -D * .69, 1.15)),
    "face": (dict(), (1.3, -.35, 1.86)),
    "joints_close": (dict(head_yaw=math.radians(60), arm_pitch=math.radians(95), elbow=math.radians(12)), (1.1, -1.2, 1.75)),
    "hand": (dict(), (.55, .95, 1.0)),
}
ORIG = {n: P[n].data for n in LOD1}
shots["lod1_front"] = (dict(), (D, 0, 1.05))
for v in VIEWS:
    USE_LOD1[0] = v.startswith("lod1")
    for n in ORIG:
        P[n].data = LOD1[n] if USE_LOD1[0] else ORIG[n]
    kw, pos = shots[v]
    pose(**kw)
    if v == "face":
        shoot(v, pos, (0.0, 0.0, 1.87), 85)
    elif v == "joints_close":
        shoot(v, pos, (0.0, 0.08, 1.58), 50)
    elif v == "hand":
        shoot(v, pos, (0.0, 0.21, 1.0), 85)
    else:
        shoot(v, pos)
