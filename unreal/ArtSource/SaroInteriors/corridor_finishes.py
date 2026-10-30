"""Small, independent SARO corridor finish layer; never rebuild approved Office.

Blender: blender -b -t 2 --python corridor_finishes.py
Layout only: python3 corridor_finishes.py --check
Three metres (x, height, z) become Blender (x, -z, height). The original walls,
floor, ceiling, signs, lamps, doors and gameplay collision remain in place.
"""
from pathlib import Path
import copy
import json
import math
import sys

ROOT = Path(__file__).resolve().parent
OFFICE = ROOT.parent / "Office"
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
NAME = "SM_Saro_CorridorFinishes"
FLOOR_TOP = .008  # The old native tile top is .005: a three-millimetre finish.
WALL_NORTH = 4.835
WALL_SOUTH = 6.579
REUSE = {
    name: "/Game/Signal47/Art/Office/Materials/" + name
    for name in ("M_Office_CreamEnamel", "M_Office_BrushedMetal", "M_Office_BlackBakelite", "M_Office_CabinetEnamel")
}


def material_rows():
    """New corridor material names, read-only references to approved Office maps."""
    originals = {r["name"]: r for r in json.loads((OFFICE / "materials.json").read_text())}
    choices = (
        ("M_Saro_Corridor_UpperPaint", "M_Office_SageWall", (.395, .405, .355), (.255, .282, .250)),
        ("M_Saro_Corridor_VCT", "M_Office_FloorLinoleum", (.310, .330, .295), (.235, .255, .232)),
        ("M_Saro_Corridor_VCTFaded", "M_Office_FloorLinoleum", (.321, .336, .297), (.235, .255, .232)),
        ("M_Saro_Corridor_VCTRepair", "M_Office_FloorLinoleum", (.280, .303, .278), (.235, .255, .232)),
    )
    result = []
    for name, source, color, original in choices:
        row = copy.deepcopy(originals[source])
        row["name"] = name
        row["texture_tint_linear"] = [round(a/b, 7) for a, b in zip(color, original)]
        row["textures"] = {role: "../Office/" + path for role, path in row["textures"].items()}
        row["source_material"] = source
        row["source_use"] = "read-only approved Office PBR maps; separate corridor material"
        result.append(row)
    from corridor_wear import material_rows as wear_material_rows
    return result + wear_material_rows()


def subtract(rect, hole):
    """Split a two-dimensional rectangle around a protected wall opening."""
    a, b, lo, hi = rect
    c, d, bot, top = hole
    c, d, bot, top = max(a, c), min(b, d), max(lo, bot), min(hi, top)
    if c >= d or bot >= top:
        return [rect]
    return [r for r in ((a, c, lo, hi), (d, b, lo, hi),
                         (c, d, lo, bot), (c, d, top, hi))
            if r[1]-r[0] > .003 and r[3]-r[2] > .003]


def layout():
    parts = []

    def quad(name, points, material):
        parts.append({"name": name, "points_three_m": points, "material": material})

    def box(name, at, size, material, bevel=.0007):
        parts.append({"name": name, "at_three_m": at, "size_three_m": size,
                      "material": material, "bevel_m": bevel})

    def wall(name, spans, z, north, holes=()):
        for i, rect in enumerate(spans):
            pieces = [rect]
            for hole in holes:
                pieces = [p for piece in pieces for p in subtract(piece, hole)]
            for j, (a, b, lo, hi) in enumerate(pieces):
                points = [(a, lo, z), (b, lo, z), (b, hi, z), (a, hi, z)]
                quad(name+"_%d_%d" % (i, j), points if north else list(reversed(points)),
                     "M_Saro_Corridor_UpperPaint")

    def notice_hole(x, y, width, height, tilt=0):
        # Include the tilted paper and both top tape pieces, then a 3mm margin.
        points = []
        for u in (-width/2, width/2):
            for v in (-height/2, height/2):
                points.append((u*math.cos(tilt)-v*math.sin(tilt),
                               u*math.sin(tilt)+v*math.cos(tilt)))
        for u in (-width/2+.01, width/2-.01):
            v = height/2-.01
            center = (u*math.cos(tilt)-v*math.sin(tilt),
                      u*math.sin(tilt)+v*math.cos(tilt))
            angle = tilt+.6
            for a in (-.0175, .0175):
                for b in (-.008, .008):
                    points.append((center[0]+a*math.cos(angle)-b*math.sin(angle),
                                   center[1]+a*math.sin(angle)+b*math.cos(angle)))
        return (x+min(p[0] for p in points)-.003, x+max(p[0] for p in points)+.003,
                y+min(p[1] for p in points)-.003, y+max(p[1] for p in points)+.003)

    # The 1.20m dado and all three door surrounds belong to the existing layer.
    # Preserve the complete north noticeboard, including its physical back.
    wall("North_upper", [(-5.96, -2.94, 1.218, 2.752),
                         (-1.86, 1.96, 1.218, 2.752),
                         (-2.94, -1.86, 2.205, 2.752)], WALL_NORTH, True,
         holes=((-1.18, -.02, 1.20, 2.01),
                notice_hole(-4.4, 1.55, .4, .55, .01),
                notice_hole(-5.25, 1.45, .14, .19),
                (-4.033, -3.467, 1.4005, 1.7995),  # .03m aerial frame plus 3mm.
                (-3.118, -3.042, 1.1895, 1.3105)))  # Original switch plate.
    wall("South_upper", [(-5.96, -4.45, 1.218, 2.752),
                         (-3.35, -.96, 1.218, 2.752),
                         (-4.45, -3.35, 2.185, 2.752)], WALL_SOUTH, False,
         holes=(notice_hole(-2.3, 1.45, .36, .5, -.015),))

    # Short end-wall returns. A hole protects the original battery EXIT sign;
    # the native FIRE EXIT enamel plate is already in front of the finish skin.
    for name, x, west, head in (("West", -5.981, True, 2.215),
                                ("East", 1.978, False, 2.215)):
        spans = [(4.84, 5.165, 1.218, 2.752),
                 (6.235, 6.57, 1.218, 2.752),
                 (5.165, 6.235, head, 2.752)]
        for i, rect in enumerate(spans):
            pieces = subtract(rect, (5.44, 5.96, 2.20, 2.44)) if west else [rect]
            for j, (a, b, lo, hi) in enumerate(pieces):
                points = [(x, lo, b), (x, lo, a), (x, hi, a), (x, hi, b)]
                quad(name+"_upper_%d_%d" % (i, j), points if west else list(reversed(points)),
                     "M_Saro_Corridor_UpperPaint")

    # Retain the existing 30cm grid. Small scattered replacement/faded tiles
    # avoid a perfect alternating checkerboard and retain the old recessed joints.
    for ix in range(27):
        x0 = max(-5.96, -6+ix*.3+.001)
        x1 = min(1.96, -6+(ix+1)*.3-.001)
        if x1 <= x0:
            continue
        for iz in range(6):
            z0, z1 = max(4.84, 4.8+iz*.3+.001), min(6.57, 4.8+(iz+1)*.3-.001)
            if z1 <= z0:
                continue
            key = (ix*47+iz*83+ix*iz*13) % 101
            material = "M_Saro_Corridor_VCTRepair" if key < 4 else (
                "M_Saro_Corridor_VCTFaded" if key < 22 else "M_Saro_Corridor_VCT")
            quad("VCT_%02d_%02d" % (ix, iz),
                 [(x0, FLOOR_TOP, z1), (x1, FLOOR_TOP, z1),
                  (x1, FLOOR_TOP, z0), (x0, FLOOR_TOP, z0)], material)

    # Thin painted cove under the suspended ceiling. Only these north/south
    # faces move into the room: they stop before the z4.84..6.57 clear zone.
    for name, spans, z in (("North", ((-5.96, 1.96),), 4.8355),
                            ("South", ((-5.96, -.96),), 6.5785)):
        for i, (a, b) in enumerate(spans):
            box(name+"_ceiling_cove_%d" % i, ((a+b)/2, 2.731, z),
                (b-a, .025, .005), "M_Office_CreamEnamel", .001)

    # Flat socket-cover fasteners in the existing low raceway envelope. No
    # new junction boxes, props, light sources or gameplay surfaces are added.
    for side, spans, z in (("North", ((-5.85, -3.04), (-1.76, 1.83)), 4.833),
                            ("South", ((-5.85, -4.52), (-3.26, -1.04)), 6.577)):
        for a, b in spans:
            for i in range(math.ceil((b-a)/1.2)+1):
                x = min(b, a+i*1.2)
                box(side+"_skirt_fastener_%.3f" % x, (x, .091, z),
                    (.006, .006, .001), "M_Office_BrushedMetal", .0008)
                box(side+"_screw_slot_%.3f" % x, (x, .091, z+(.0007 if side=="North" else -.0007)),
                    (.0036, .0006, .0004), "M_Office_BlackBakelite", .0001)
    from corridor_service import service_parts
    from corridor_wear import wear_parts
    parts.extend(service_parts())
    parts.extend(wear_parts())
    return parts


def build():
    import bpy
    from mathutils import Vector
    ROOT.mkdir(parents=True, exist_ok=True)
    (ROOT / "Export").mkdir(exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1
    edit = bpy.data.collections.new("01_EDITABLE_CORRIDOR_FINISHES")
    export = bpy.data.collections.new("02_NATIVE_EXPORT")
    bpy.context.scene.collection.children.link(edit)
    bpy.context.scene.collection.children.link(export)
    from corridor_wear import generate_maps
    generate_maps(ROOT)
    originals = {r["name"]: r for r in json.loads((OFFICE / "materials.json").read_text())}
    rows = material_rows()
    mats = {}

    def material(row, source_root=ROOT):
        m = bpy.data.materials.new(row["name"])
        m.use_nodes = True
        nodes, links = m.node_tree.nodes, m.node_tree.links
        p = nodes.get("Principled BSDF")
        p.inputs["Roughness"].default_value = row["roughness"]
        p.inputs["Metallic"].default_value = row.get("metallic", 0)
        for role, path in row["textures"].items():
            if role not in ("base_color", "roughness", "normal"):
                continue
            path = Path(path)
            if not path.is_absolute():
                path = source_root/path
            image = bpy.data.images.load(str(path), check_existing=True)
            image.colorspace_settings.name = "sRGB" if role == "base_color" else "Non-Color"
            sample = nodes.new("ShaderNodeTexImage")
            sample.image = image
            if role == "base_color":
                mix = nodes.new("ShaderNodeMixRGB")
                mix.blend_type = "MULTIPLY"
                mix.inputs[0].default_value = 1
                mix.inputs[2].default_value = (*row.get("texture_tint_linear", [1, 1, 1]), 1)
                links.new(sample.outputs["Color"], mix.inputs[1])
                links.new(mix.outputs[0], p.inputs["Base Color"])
                if row.get("opacity_from_base_color"):
                    links.new(sample.outputs["Alpha"], p.inputs["Alpha"])
                    m.surface_render_method = "DITHERED"
            elif role == "roughness":
                links.new(sample.outputs["Color"], p.inputs["Roughness"])
            else:
                # Office maps are DirectX: invert green only in Blender's preview.
                sep = nodes.new("ShaderNodeSeparateColor")
                inv = nodes.new("ShaderNodeMath"); inv.operation = "SUBTRACT"; inv.inputs[0].default_value = 1
                comb = nodes.new("ShaderNodeCombineColor")
                normal = nodes.new("ShaderNodeNormalMap")
                links.new(sample.outputs["Color"], sep.inputs["Color"])
                links.new(sep.outputs["Green"], inv.inputs[1])
                links.new(sep.outputs["Red"], comb.inputs["Red"])
                links.new(inv.outputs[0], comb.inputs["Green"])
                links.new(sep.outputs["Blue"], comb.inputs["Blue"])
                links.new(comb.outputs[0], normal.inputs["Color"])
                links.new(normal.outputs["Normal"], p.inputs["Normal"])
        mats[row["name"]] = m

    for row in rows:
        material(row)
    for name in REUSE:
        material(originals[name], OFFICE)

    def xyz(point):
        return Vector((point[0], -point[2], point[1]))

    def active(obj):
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj

    objects = []
    parts = layout()
    for part in parts:
        if "points_three_m" in part:
            mesh = bpy.data.meshes.new(part["name"])
            mesh.from_pydata([xyz(p) for p in part["points_three_m"]], [], [(0, 1, 2, 3)])
            mesh.update()
            if "uv" in part:
                layer = mesh.uv_layers.new(name="UVMap")
                for polygon in mesh.polygons:
                    for loop_index in polygon.loop_indices:
                        vertex = mesh.loops[loop_index].vertex_index
                        layer.data[loop_index].uv = part["uv"][vertex]
            obj = bpy.data.objects.new(part["name"], mesh)
            edit.objects.link(obj)
        else:
            bpy.ops.mesh.primitive_cube_add(size=1, location=xyz(part["at_three_m"]))
            obj = bpy.context.object
            obj.name = part["name"]
            size = part["size_three_m"]
            obj.dimensions = (size[0], size[2], size[1])
            active(obj)
            bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
            for collection in list(obj.users_collection):
                collection.objects.unlink(obj)
            edit.objects.link(obj)
            radius = min(part["bevel_m"], min(size)*.24)
            if radius:
                bevel = obj.modifiers.new("Small_real_edge", "BEVEL")
                bevel.width = radius
                bevel.segments = 2
                bpy.ops.object.modifier_apply(modifier=bevel.name)
                normals = obj.modifiers.new("Weighted_normals", "WEIGHTED_NORMAL")
                normals.keep_sharp = True
                bpy.ops.object.modifier_apply(modifier=normals.name)
        obj.data.materials.append(mats[part["material"]])
        active(obj)
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        objects.append(obj)

    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.duplicate()
    copies = list(bpy.context.selected_objects)
    bpy.context.view_layer.objects.active = copies[0]
    bpy.ops.object.join()
    mesh = bpy.context.object
    mesh.name = NAME
    for collection in list(mesh.users_collection):
        collection.objects.unlink(mesh)
    export.objects.link(mesh)
    active(mesh)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="DESELECT")
    bpy.ops.mesh.select_mode(type="FACE")
    bpy.ops.object.mode_set(mode="OBJECT")
    # Wear uses original atlas UVs. Only the existing finish/service faces use
    # the measured one-metre cube projection; joining must not overwrite masks.
    for polygon in mesh.data.polygons:
        polygon.select = mesh.data.materials[polygon.material_index].name != "M_Saro_CorridorWear"
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.uv.cube_project(cube_size=1)
    bpy.ops.object.mode_set(mode="OBJECT")
    tri = mesh.modifiers.new("Native_triangles", "TRIANGULATE")
    bpy.ops.object.modifier_apply(modifier=tri.name)
    triangles = len(mesh.data.polygons)
    assert triangles < 15000, triangles
    bpy.ops.export_scene.fbx(filepath=str(ROOT/"Export"/(NAME+".fbx")),
        use_selection=True, object_types={"MESH"}, apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y", axis_up="Z",
        bake_anim=False, mesh_smooth_type="FACE")
    bounds = [[float(fn(v.co[k] for v in mesh.data.vertices)) for k in range(3)] for fn in (min, max)]
    (ROOT/"corridor-materials.json").write_text(json.dumps(rows, indent=2)+"\n")
    contract = {"units": "metres", "mapping": "Blender (x,-z,y) to UE (100x,100z,100y)",
        "asset": NAME, "triangles": triangles, "bounds_local_m": bounds,
        "materials": [m.name for m in mesh.data.materials], "reuse_materials": REUSE,
        "source": "corridor_finishes.py", "editable_components": len(objects),
        "floor_top_m": FLOOR_TOP, "collision": "NoCollision; original gameplay unchanged",
        "protected": "noticeboard, five papers, signs, all door apertures, lamps and maintenance hatch",
        "office": "read-only source textures and native material reuse; no Office rebuild"}
    (ROOT/"corridor-contract.json").write_text(json.dumps(contract, indent=2)+"\n")
    mesh.hide_set(True)
    mesh.hide_render = True
    for image in bpy.data.images:
        if image.source == "FILE":
            image.pack()
    bpy.ops.file.make_paths_relative()
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/"Signal47_CorridorFinishes.blend"))
    print("CORRIDOR FINISHES READY", json.dumps({"triangles": triangles, "components": len(objects)}))


if __name__ == "__main__":
    if "--check" in sys.argv:
        parts = layout()
        assert WALL_NORTH < 4.84 and WALL_SOUTH > 6.57
        assert len({p["name"] for p in parts}) == len(parts)
        assert all(min(p["size_three_m"]) > 0 for p in parts if "size_three_m" in p)
        assert all((ROOT/path).is_file() for row in material_rows() if row["name"] != "M_Saro_CorridorWear" for path in row["textures"].values())
        assert all(len(p["uv"]) == len(p["points_three_m"]) for p in parts if "uv" in p)
        print("Corridor layout:", len(parts), "components; original Office files only read; own wear maps generated by Blender.")
    else:
        build()
