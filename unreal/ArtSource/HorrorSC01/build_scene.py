"""Original SC01 hand, glass traces and retracting steel shutter.

Blender CPU authoring only. Exports metric FBX meshes for the existing native
import recipe; no library mesh, purchased asset or reference image is used.
"""
from pathlib import Path
import hashlib
import json
import math
import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
EXPORT = ROOT / "Export"
EXPORT.mkdir(exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = "METRIC"
bpy.context.scene.unit_settings.scale_length = 1.0

MATERIAL_ROWS = [
    {"name": "M_SC01_Skin", "base_color_linear": [.59, .62, .54, 1], "roughness": .74, "specular": .26, "metallic": 0},
    {"name": "M_SC01_Nail", "base_color_linear": [.29, .31, .25, 1], "roughness": .48, "specular": .31, "metallic": 0},
    {"name": "M_SC01_Steel", "base_color_linear": [.18, .23, .22, 1], "roughness": .48, "specular": .42, "metallic": .64},
    {"name": "M_SC01_Grease", "base_color_linear": [.36, .39, .32, 1], "roughness": .27, "specular": .58, "metallic": 0, "two_sided": True},
]
MATS = {}
for row in MATERIAL_ROWS:
    mat = bpy.data.materials.new(row["name"])
    mat.diffuse_color = row["base_color_linear"]
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = row["base_color_linear"]
    bsdf.inputs["Roughness"].default_value = row["roughness"]
    bsdf.inputs["Metallic"].default_value = row["metallic"]
    MATS[row["name"]] = mat


def ellipsoid(name, at, size, material, segments=20, rings=12):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, location=at)
    obj = bpy.context.object
    obj.name = name
    obj.scale = size
    obj.data.materials.append(MATS[material])
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    for p in obj.data.polygons:
        p.use_smooth = True
    return obj


def bone(name, a, b, radius, mat):
    direction = Vector(b) - Vector(a)
    bpy.ops.mesh.primitive_cone_add(vertices=20, radius1=radius * 1.06,
                                  radius2=radius * .88, depth=direction.length,
                                  location=(Vector(a) + Vector(b)) / 2)
    obj = bpy.context.object
    obj.name = name
    obj.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()
    obj.data.materials.append(MATS[mat])
    for p in obj.data.polygons:
        p.use_smooth = True
    return obj


def box(name, at, size, mat, bevel=.003):
    bpy.ops.mesh.primitive_cube_add(size=1, location=at)
    obj = bpy.context.object
    obj.name = name
    obj.scale = size
    obj.data.materials.append(MATS[mat])
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    mod = obj.modifiers.new("Manufactured rounded edges", "BEVEL")
    mod.width = bevel
    mod.segments = 2
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj


def join(parts, name, smooth=False):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    bpy.context.scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    if smooth:
        # Fuse flesh intersections into one authored solid, preserving nails as
        # independent material geometry added after this operation.
        mod = obj.modifiers.new("Continuous skin", "REMESH")
        mod.mode = "VOXEL"
        mod.voxel_size = .006
        mod.use_smooth_shade = True
        bpy.ops.object.modifier_apply(modifier=mod.name)
        mod = obj.modifiers.new("Skin relaxation", "SMOOTH")
        mod.factor = .36
        mod.iterations = 3
        bpy.ops.object.modifier_apply(modifier=mod.name)
        mod = obj.modifiers.new("Measured silhouette mesh", "DECIMATE")
        mod.ratio = .48
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj


skin = "M_SC01_Skin"
parts = [ellipsoid("Palm", (0, 0, .015), (.102, .043, .14), skin),
         ellipsoid("Heel", (-.01, .006, -.075), (.078, .041, .074), skin),
         bone("Forearm", (-.015, .022, -.83), (0, 0, -.12), .047, skin),
         ellipsoid("Wrist", (0, 0, -.14), (.047, .036, .065), skin)]
nails = []
# Long fingers bend slightly toward the pane; each joint remains distinguishable
# after the union, with a gradual taper and individually placed nail plates.
for i, (x, length, spread) in enumerate([(-.074, .34, -.030), (-.026, .43, -.006), (.027, .405, .010), (.073, .325, .038)]):
    a = Vector((x, 0, .10))
    b = Vector((x + spread * .28, -.011, .10 + length * .44))
    c = Vector((x + spread * .70, .004, .10 + length * .78))
    d = Vector((x + spread, .030, .10 + length))
    for j, (start, end, radius) in enumerate([(a, b, .021), (b, c, .0175), (c, d, .0138)]):
        parts.append(bone(f"Finger{i}_segment{j}", start, end, radius, skin))
        parts.append(ellipsoid(f"Finger{i}_joint{j}", end, (radius, radius * .86, radius * 1.10), skin))
    nails.append(ellipsoid(f"Nail{i}", (d.x, d.y + .012, d.z - .015), (.011, .003, .020), "M_SC01_Nail", 16, 8))
thumb = [Vector((-.075, .006, -.024)), Vector((-.156, .002, .060)), Vector((-.195, .026, .148))]
for i, (a, b) in enumerate(zip(thumb, thumb[1:])):
    parts.append(bone("Thumb" + str(i), a, b, .026 - i * .006, skin))
    parts.append(ellipsoid("ThumbKnuckle" + str(i), b, (.024 - i * .005, .021, .026), skin))
nails.append(ellipsoid("ThumbNail", (-.196, .043, .14), (.015, .004, .023), "M_SC01_Nail"))
hand = join(parts, "SM_SC01_Hand", True)
hand = join([hand] + nails, "SM_SC01_Hand")

slat = join([box("SlatFace", (0, 0, 0), (1.73, .021, .073), "M_SC01_Steel"),
             box("SlatUpperRib", (0, -.012, .027), (1.72, .013, .009), "M_SC01_Steel", .002),
             box("SlatLowerRib", (0, -.012, -.027), (1.72, .013, .009), "M_SC01_Steel", .002)],
            "SM_SC01_ShutterSlat")
marks = []
for i, (x, z, length) in enumerate([(-.073, .07, .15), (-.026, .12, .20), (.025, .105, .185), (.071, .065, .14), (-.14, -.035, .095)]):
    mark = ellipsoid("FingerGrease" + str(i), (x, 0, z), (.0085, .0018, length / 2), "M_SC01_Grease", 16, 8)
    mark.rotation_euler[1] = -.12 + i * .055
    marks.append(mark)
marks.append(ellipsoid("PalmGrease", (-.005, .0003, -.035), (.065, .0012, .052), "M_SC01_Grease", 20, 10))
trace = join(marks, "SM_SC01_GlassTraces")

contract = {"authorship": "Original procedural Blender geometry for SIGNAL / 47, no external model sources.",
            "axes": "Blender metric Z-up; native UE import x,-y,z in centimetres.", "parts": {}}
for obj in (hand, slat, trace):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    triangulate = obj.modifiers.new("Native export triangles", "TRIANGULATE")
    bpy.ops.object.modifier_apply(modifier=triangulate.name)
    vertices = [obj.matrix_world @ v.co for v in obj.data.vertices]
    contract["parts"][obj.name] = {
        "vertices": len(obj.data.vertices), "triangles": len(obj.data.polygons),
        "materials": [slot.name for slot in obj.data.materials],
        "bounds_local_m": [[min(v[k] for v in vertices) for k in range(3)], [max(v[k] for v in vertices) for k in range(3)]]}
    bpy.ops.export_scene.fbx(filepath=str(EXPORT / (obj.name + ".fbx")), use_selection=True,
                             object_types={"MESH"}, apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
                             axis_forward="-Y", axis_up="Z", bake_space_transform=False,
                             use_mesh_modifiers=True, add_leaf_bones=False, path_mode="AUTO")

bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / "SC01_PhysicalScene.blend"))
(ROOT / "materials.json").write_text(json.dumps(MATERIAL_ROWS, indent=2) + "\n")
contract["fbx_sha256"] = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in EXPORT.glob("*.fbx")}
(ROOT / "model_contract.json").write_text(json.dumps(contract, indent=2) + "\n")
print("SC01 ORIGINAL GEOMETRY EXPORTED", json.dumps(contract["parts"]))
