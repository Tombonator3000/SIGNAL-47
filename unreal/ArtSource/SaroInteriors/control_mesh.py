"""Small Blender export helper for the independent Control finish layer only."""
import json
from pathlib import Path


def build(root, office, name, parts, rows, reuse):
    import bpy
    from mathutils import Vector
    root.mkdir(parents=True, exist_ok=True)
    (root/"Export").mkdir(exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1
    edit = bpy.data.collections.new("01_EDITABLE_CONTROL_FINISHES")
    export = bpy.data.collections.new("02_NATIVE_EXPORT")
    bpy.context.scene.collection.children.link(edit)
    bpy.context.scene.collection.children.link(export)
    originals = {r["name"]: r for r in json.loads((office/"materials.json").read_text())}
    mats = {}

    def material(row, source_root=root):
        mat = bpy.data.materials.new(row["name"])
        mat.use_nodes = True
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        shader = nodes.get("Principled BSDF")
        shader.inputs["Roughness"].default_value = row["roughness"]
        shader.inputs["Metallic"].default_value = row.get("metallic", 0)
        for role, path in row["textures"].items():
            if role not in ("base_color", "roughness", "normal"):
                continue
            path = Path(path)
            image = bpy.data.images.load(str(path if path.is_absolute() else source_root/path), check_existing=True)
            image.colorspace_settings.name = "sRGB" if role == "base_color" else "Non-Color"
            sample = nodes.new("ShaderNodeTexImage")
            sample.image = image
            if role == "base_color":
                mix = nodes.new("ShaderNodeMixRGB")
                mix.blend_type = "MULTIPLY"
                mix.inputs[0].default_value = 1
                mix.inputs[2].default_value = (*row.get("texture_tint_linear", [1, 1, 1]), 1)
                links.new(sample.outputs["Color"], mix.inputs[1])
                links.new(mix.outputs[0], shader.inputs["Base Color"])
            elif role == "roughness":
                links.new(sample.outputs["Color"], shader.inputs["Roughness"])
            else:
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
                links.new(normal.outputs["Normal"], shader.inputs["Normal"])
        mats[row["name"]] = mat

    for row in rows:
        material(row)
    for key in reuse:
        material(originals[key], office)

    def xyz(point):
        return Vector((point[0], -point[2], point[1]))

    def active(obj):
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj

    objects = []
    for part in parts:
        if "points_three_m" in part:
            data = bpy.data.meshes.new(part["name"])
            data.from_pydata([xyz(p) for p in part["points_three_m"]], [], [(0, 1, 2, 3)])
            data.update()
            obj = bpy.data.objects.new(part["name"], data)
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
                normal = obj.modifiers.new("Weighted_normals", "WEIGHTED_NORMAL")
                normal.keep_sharp = True
                bpy.ops.object.modifier_apply(modifier=normal.name)
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
    mesh.name = name
    for collection in list(mesh.users_collection):
        collection.objects.unlink(mesh)
    export.objects.link(mesh)
    active(mesh)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.cube_project(cube_size=1)
    bpy.ops.object.mode_set(mode="OBJECT")
    tri = mesh.modifiers.new("Native_triangles", "TRIANGULATE")
    bpy.ops.object.modifier_apply(modifier=tri.name)
    triangles = len(mesh.data.polygons)
    assert triangles <= 20000, triangles
    bpy.ops.export_scene.fbx(filepath=str(root/"Export"/(name+".fbx")),
        use_selection=True, object_types={"MESH"}, apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y", axis_up="Z",
        bake_anim=False, mesh_smooth_type="FACE")
    bounds = [[float(fn(v.co[k] for v in mesh.data.vertices)) for k in range(3)] for fn in (min, max)]
    (root/"control-materials.json").write_text(json.dumps(rows, indent=2)+"\n")
    contract = {"units": "metres", "mapping": "Blender (x,-z,y) to UE (100x,100z,100y)",
        "asset": name, "triangles": triangles, "bounds_local_m": bounds,
        "materials": [m.name for m in mesh.data.materials], "reuse_materials": reuse,
        "source": "control_finishes.py", "editable_components": len(objects),
        "floor_top_m": 0, "collision": "NoCollision; original gameplay unchanged",
        "protected": "hex floor, window, doors, posters, screens, printer, DSP, telex, phones and rack controls",
        "office": "read-only approved maps/native reuse; no Office rebuild"}
    (root/"control-contract.json").write_text(json.dumps(contract, indent=2)+"\n")
    mesh.hide_set(True)
    mesh.hide_render = True
    for image in bpy.data.images:
        if image.source == "FILE":
            image.pack()
    bpy.ops.file.make_paths_relative()
    bpy.ops.wm.save_as_mainfile(filepath=str(root/"Signal47_ControlRoomFinishes.blend"))
    print("CONTROL FINISHES READY", json.dumps({"triangles": triangles, "components": len(objects)}))
