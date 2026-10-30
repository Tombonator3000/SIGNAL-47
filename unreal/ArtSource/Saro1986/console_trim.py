"""Small original walnut console trim, retaining all live control-room surfaces.

blender -b -t 2 --python console_trim.py
python3 console_trim.py --check

No desktop skin: the original top at y.780, beige CRTs, keyboards, papers and
displays remain untouched. All details stay inside the console's old collider.
Approved Office materials are read-only native reuse, never regenerated.
"""
from pathlib import Path
import json
import math
import sys

ROOT = Path(__file__).resolve().parent
OFFICE = ROOT.parent / "Office"
NAME = "SM_Saro1986_ConsoleTrim"
WOOD, METAL, DARK = "M_Office_Walnut", "M_Office_BrushedMetal", "M_Office_BlackBakelite"
REUSE = {name: "/Game/Signal47/Art/Office/Materials/"+name for name in (WOOD, METAL, DARK)}


def layout():
    parts = []

    def box(name, at, size, material, bevel=.001):
        parts.append({"name": name, "at_three_m": at, "size_three_m": size,
                      "material": material, "bevel_m": bevel})

    # The bullnose is a real rounded strip below the existing .780m tabletop.
    # It covers the earlier thin metal edge without touching any desk contents.
    box("Walnut_console_bullnose", (0, .749, -3.443), (6.90, .046, .028), WOOD, .005)
    for x in (-3.451, 3.451):
        box("Walnut_console_end_%s" % x, (x, .3975, -3.90), (.008, .725, .88), WOOD, .0015)
    for bank, x in enumerate((-3.2, -1.1, 1.1, 3.2)):
        # The backing lies just in front of the old flat handle strips, so no
        # original strip is left floating across the new physical drawer gaps.
        box("Drawer_bank_%d_dark_reveals" % bank, (x, .380, -3.514), (.452, .694, .004), DARK, .0007)
        for drawer, y in enumerate((.180, .400, .620)):
            box("Drawer_%d_%d_walnut_front" % (bank, drawer), (x, y, -3.507), (.436, .215, .008), WOOD, .002)
            parts.append({"name": "Drawer_%d_%d_bent_metal_pull" % (bank, drawer),
                          "material": METAL, "radius_m": .0032,
                          "curve_three_m": [(x-.065, y+.025, -3.501),
                                            (x-.065, y+.025, -3.480),
                                            (x-.057, y+.025, -3.473),
                                            (x+.057, y+.025, -3.473),
                                            (x+.065, y+.025, -3.480),
                                            (x+.065, y+.025, -3.501)]})
    return parts


def check():
    parts = layout()
    assert len(parts) == 31
    assert set(p["material"] for p in parts) == set(REUSE)
    for part in parts:
        if "curve_three_m" in part:
            radius = part["radius_m"]
            points = part["curve_three_m"]
            lo = [min(p[k] for p in points)-radius for k in range(3)]
            hi = [max(p[k] for p in points)+radius for k in range(3)]
        else:
            lo = [a-s/2 for a, s in zip(part["at_three_m"], part["size_three_m"])]
            hi = [a+s/2 for a, s in zip(part["at_three_m"], part["size_three_m"])]
        assert -3.5 <= lo[0] <= hi[0] <= 3.5, (part["name"], lo, hi)
        assert -4.5 <= lo[2] <= hi[2] < -3.42, (part["name"], lo, hi)
        assert lo[1] > 0 and hi[1] < .780, (part["name"], lo, hi)
    return parts


def build():
    import bpy
    from mathutils import Vector
    parts = check()
    (ROOT/"Export").mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"; scene.unit_settings.scale_length = 1
    editable = bpy.data.collections.new("01_EDITABLE_CONSOLE_TRIM")
    export = bpy.data.collections.new("02_NATIVE_CONSOLE_TRIM")
    scene.collection.children.link(editable); scene.collection.children.link(export)
    rows = {r["name"]: r for r in json.loads((OFFICE/"materials.json").read_text())}
    mats = {}
    for name in REUSE:
        row = rows[name]
        mat = bpy.data.materials.new(name); mat.use_nodes = True
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        shader = nodes.get("Principled BSDF")
        shader.inputs["Roughness"].default_value = row["roughness"]
        shader.inputs["Metallic"].default_value = row.get("metallic", 0)
        for role, path in row.get("textures", {}).items():
            image = bpy.data.images.load(str(OFFICE/path), check_existing=True)
            image.colorspace_settings.name = "sRGB" if role == "base_color" else "Non-Color"
            tex = nodes.new("ShaderNodeTexImage"); tex.image = image
            if role == "base_color":
                links.new(tex.outputs["Color"], shader.inputs["Base Color"])
            elif role == "roughness":
                links.new(tex.outputs["Color"], shader.inputs["Roughness"])
            elif role == "normal":
                # The shared native maps are DirectX; Blender's preview uses +Y.
                separate = nodes.new("ShaderNodeSeparateColor")
                inverse = nodes.new("ShaderNodeMath"); inverse.operation = "SUBTRACT"; inverse.inputs[0].default_value = 1
                combine = nodes.new("ShaderNodeCombineColor"); normal = nodes.new("ShaderNodeNormalMap")
                links.new(tex.outputs["Color"], separate.inputs["Color"])
                links.new(separate.outputs["Green"], inverse.inputs[1])
                links.new(separate.outputs["Red"], combine.inputs["Red"])
                links.new(inverse.outputs[0], combine.inputs["Green"])
                links.new(separate.outputs["Blue"], combine.inputs["Blue"])
                links.new(combine.outputs[0], normal.inputs["Color"])
                links.new(normal.outputs["Normal"], shader.inputs["Normal"])
        mats[name] = mat

    def xyz(p):
        return Vector((p[0], -p[2], p[1]))

    def active(obj):
        bpy.ops.object.select_all(action="DESELECT"); obj.select_set(True)
        bpy.context.view_layer.objects.active = obj

    objects = []
    for part in parts:
        if "curve_three_m" in part:
            data = bpy.data.curves.new(part["name"], "CURVE"); data.dimensions = "3D"
            data.bevel_depth = part["radius_m"]; data.bevel_resolution = 2; data.use_fill_caps = True
            spline = data.splines.new("POLY"); spline.points.add(len(part["curve_three_m"])-1)
            for vertex, point in zip(spline.points, part["curve_three_m"]):
                vertex.co = (*xyz(point), 1)
            obj = bpy.data.objects.new(part["name"], data); editable.objects.link(obj)
            active(obj); bpy.ops.object.convert(target="MESH"); obj = bpy.context.object
        else:
            bpy.ops.mesh.primitive_cube_add(size=1, location=xyz(part["at_three_m"]))
            obj = bpy.context.object; obj.name = part["name"]
            size = part["size_three_m"]; obj.dimensions = (size[0], size[2], size[1])
            active(obj); bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
            for collection in list(obj.users_collection):
                collection.objects.unlink(obj)
            editable.objects.link(obj)
            bevel = obj.modifiers.new("Rounded_furniture_edge", "BEVEL")
            bevel.width = min(part["bevel_m"], min(size)*.24); bevel.segments = 3
            bpy.ops.object.modifier_apply(modifier=bevel.name)
            normal = obj.modifiers.new("Flat_panel_normals", "WEIGHTED_NORMAL"); normal.keep_sharp = True
            bpy.ops.object.modifier_apply(modifier=normal.name)
        obj.data.materials.append(mats[part["material"]]); active(obj)
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        # The Walnut texture's grain runs along V. Front drawer grain is horizontal;
        # end-panel grain hangs vertically, with no per-face rescaling/stretching.
        uv = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
        for polygon in obj.data.polygons:
            for index in polygon.loop_indices:
                p = obj.data.vertices[obj.data.loops[index].vertex_index].co
                if abs(polygon.normal.x) > .7:
                    uv.data[index].uv = (p.y*1.7, p.z*1.7)
                elif abs(polygon.normal.z) > .7:
                    uv.data[index].uv = (p.y*1.7, p.x*1.7)
                else:
                    uv.data[index].uv = (p.z*1.7, p.x*1.7)
        objects.append(obj)
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
    bpy.ops.export_scene.fbx(filepath=str(ROOT/"Export"/(NAME+".fbx")),
        use_selection=True, object_types={"MESH"}, apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y", axis_up="Z",
        bake_anim=False, mesh_smooth_type="FACE")
    bounds = [[float(fn(v.co[k] for v in mesh.data.vertices)) for k in range(3)] for fn in (min, max)]
    contract = {"asset": NAME, "triangles": len(mesh.data.polygons), "bounds_local_m": bounds,
        "materials": [mat.name for mat in mesh.data.materials], "reuse_materials": REUSE,
        "units": "metres", "mapping": "Blender(x,-z,y) -> UE(100x,100z,100y)",
        "source": "console_trim.py", "editable_components": len(objects),
        "collision": "NoCollision; within original console collider x[-3.5,3.5],z[-4.5,-3.42]",
        "protected": "original y.780 desktop, all CRTs/displays/keys/knobs/papers, floor and shoulder space"}
    (ROOT/"console-trim-contract.json").write_text(json.dumps(contract, indent=2)+"\n")
    (ROOT/"console-trim-materials.json").write_text("[]\n")
    mesh.hide_set(True); mesh.hide_render = True
    for image in bpy.data.images:
        if image.source == "FILE":
            image.pack()
    bpy.ops.file.make_paths_relative()
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/"Signal47_ConsoleTrim.blend"))
    print("CONSOLE TRIM READY", json.dumps({"triangles": contract["triangles"], "parts": len(objects)}))


if __name__ == "__main__":
    if "--check" in sys.argv:
        print(json.dumps({"parts": len(check()), "reuse_materials": REUSE}))
    else:
        build()
