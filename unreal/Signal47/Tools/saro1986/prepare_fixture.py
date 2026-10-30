"""Prepare Tom's private Tripo fluorescent fixture for the measured native recipe.

blender -b -t 2 --python prepare_fixture.py -- --source /private/model.glb \
  --target-dir /private/fixture/prepared --max-triangles 20000 [--flip]

Original files stay untouched. PCA maps long/middle/thin axes to Blender X/Y/Z;
the flatter broad side becomes the back. --flip reverses that choice for preview.
The housing is 1.30x.36x.08m, with its top-centre at the shared origin.
The default inserts are two straight T12 glass tubes, below the original housing.
Optional original-face and measured-skin modes preserve the source atlas UVs.
No housing becomes an emitting surface.
"""
from pathlib import Path
import argparse
import json
import math
import sys


def main():
    import bpy
    import bmesh
    import numpy as np
    from mathutils import Matrix, Vector
    from mathutils.bvhtree import BVHTree
    from mathutils.geometry import barycentric_transform

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--target-dir", required=True, type=Path)
    parser.add_argument("--flip", action="store_true")
    parser.add_argument("--max-triangles", type=int, default=20000)
    parser.add_argument("--lens-mode", choices=("auto", "original", "strips", "tubes"), default="tubes")
    parser.add_argument("--normal-y", choices=("opengl", "directx"), default="opengl")
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    source = args.source.resolve(strict=True)
    out = args.target_dir.resolve()
    if out == source.parent or source.is_relative_to(out):
        raise ValueError("Output must be a separate private preparation directory")
    if source.suffix.lower() not in (".fbx", ".glb", ".gltf"):
        raise ValueError("Use the actual private FBX or glTF export")
    out.mkdir(parents=True, exist_ok=True)
    (out/"textures").mkdir(exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1
    if source.suffix.lower() == ".fbx":
        bpy.ops.import_scene.fbx(filepath=str(source), use_anim=False)
    else:
        bpy.ops.import_scene.gltf(filepath=str(source))
    objects = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    if not objects or not all(o.data.uv_layers for o in objects):
        raise ValueError("The Tripo atlas UVs are missing")

    def active(obj):
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True); bpy.context.view_layer.objects.active = obj

    # Preserve the importer's real materials long enough to resolve embedded
    # glTF images as well as the separate maps in an FBX .fbm directory.
    originals = [m for o in objects for m in o.data.materials if m]
    images = []
    for mat in originals:
        if mat.use_nodes:
            for node in mat.node_tree.nodes:
                if node.type == "TEX_IMAGE" and node.image and node.image not in images:
                    images.append(node.image)
    paths = [p for p in source.parent.rglob("*") if p.is_file() and
             p.suffix.lower() in (".png", ".jpg", ".jpeg", ".tga") and not p.is_relative_to(out)]

    def role_name(name, role):
        name = name.lower().replace("-", "_")
        keys = {"base_color": ("basecolor", "base_color", "albedo", "diffuse"),
                "roughness": ("roughness", "rough"), "metallic": ("metallic", "metalness"),
                "normal": ("normal",), "packed": ("_rm", "metallicroughness", "metallic_roughness")}
        return any(key in name for key in keys[role])

    def image_for(role):
        found = [im for im in images if role_name(im.name, role)]
        if found:
            return found[0], False
        # Embedded glTF RM is the selected export's atlas. A neighbouring FBX
        # export can use a completely different UV layout, even for the same
        # generated object, so do not prefer that file over this packed map.
        if role in ("roughness", "metallic"):
            packed = [im for im in images if role_name(im.name, "packed")]
            if packed:
                return packed[0], True
        found = [p for p in paths if role_name(p.stem, role)]
        if found:
            return bpy.data.images.load(str(found[0]), check_existing=True), False
        if role in ("roughness", "metallic"):
            packed = [im for im in images if role_name(im.name, "packed")]
            if not packed:
                packed = [bpy.data.images.load(str(p), check_existing=True) for p in paths if role_name(p.stem, "packed")][:1]
            if packed:
                return packed[0], True
        return None, False

    prepared_textures, prepared_images = {}, {}
    for role in ("base_color", "roughness", "metallic", "normal"):
        original, packed = image_for(role)
        if original is None and role in ("base_color", "normal"):
            raise ValueError("Missing original Tripo " + role + " atlas")
        if original:
            image = original.copy()
            image.colorspace_settings.name = "sRGB" if role == "base_color" else "Non-Color"
            width, height = image.size
            # A2k game export is copied at its original resolution. Only larger
            # source atlases are downsampled, never written back to the source.
            if max(width, height) > 2048:
                scale = 2048/max(width, height)
                image.scale(max(1, round(width*scale)), max(1, round(height*scale)))
        else:
            image = bpy.data.images.new("Fixture_"+role, 256, 256, alpha=True)
            image.colorspace_settings.name = "Non-Color"
            value = .55 if role == "roughness" else 0
            image.generated_color = (value, value, value, 1)
        width, height = image.size
        if packed or role == "normal" and args.normal_y == "opengl":
            pixels = np.empty(width*height*4, dtype=np.float32)
            image.pixels.foreach_get(pixels); pixels = pixels.reshape(-1, 4)
            if packed:
                value = pixels[:, 1 if role == "roughness" else 2].copy()
                pixels[:, :3] = value[:, None]
            else:
                pixels[:, 1] = 1-pixels[:, 1]
            pixels[:, 3] = 1
            image.pixels.foreach_set(pixels.ravel())
        target = out/"textures"/("fixture_"+role+".png")
        image.filepath_raw = str(target); image.file_format = "PNG"; image.save()
        prepared_textures[role] = str(target.relative_to(out))
        prepared_images[role] = image

    # All reviewed exports have a single atlas. Reject multiple incompatible
    # atlases rather than silently rebinding another part to the wrong artwork.
    albedos = [im for im in images if role_name(im.name, "base_color")]
    if len(albedos) > 1 and len({im.name for im in albedos}) > 1:
        raise ValueError("Export contains multiple distinct albedo atlases; use the single-atlas game export")
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]; bpy.ops.object.join()
    original = bpy.context.object
    original.data.transform(original.matrix_world); original.matrix_world = Matrix.Identity(4)
    original.data.calc_loop_triangles(); source_triangles = len(original.data.loop_triangles)
    if args.max_triangles < 1000:
        raise ValueError("Fixture triangle target must be at least1000")
    if source_triangles > args.max_triangles:
        mod = original.modifiers.new("Private_game_density_UV_preserved", "DECIMATE")
        mod.decimate_type = "COLLAPSE"
        mod.ratio = args.max_triangles/source_triangles
        mod.use_collapse_triangulate = True
        bpy.ops.object.modifier_apply(modifier=mod.name)
    tri = original.modifiers.new("Delivery_triangles", "TRIANGULATE")
    bpy.ops.object.modifier_apply(modifier=tri.name)
    mesh = original.data
    mesh.uv_layers[0].name = "S47_Atlas"
    points = np.array([v.co[:] for v in mesh.vertices], dtype=np.float64)
    centre = points.mean(axis=0)
    values, axes = np.linalg.eigh(np.cov((points-centre).T))
    axes = axes[:, np.argsort(values)[::-1]]
    for i in (0, 1):
        if axes[np.argmax(np.abs(axes[:, i])), i] < 0:
            axes[:, i] *= -1
    axes[:, 2] = np.cross(axes[:, 0], axes[:, 1])
    aligned = (points-centre) @ axes
    indices = np.array([tuple(p.vertices) for p in mesh.polygons], dtype=np.int32)

    def face_data(vertices):
        corners = vertices[indices]
        cross = np.cross(corners[:, 1]-corners[:, 0], corners[:, 2]-corners[:, 0])
        lengths = np.linalg.norm(cross, axis=1)
        normals = cross/np.maximum(lengths[:, None], 1e-14)
        return corners.mean(axis=1), normals, lengths*.5

    centers, normals, areas = face_data(aligned)
    zlo, zhi = aligned[:, 2].min(), aligned[:, 2].max()
    middle = (zlo+zhi)/2
    horizontal = np.abs(normals[:, 2]) > .90
    bins = np.linspace(zlo, zhi+1e-10, 65)
    hist, _ = np.histogram(centers[horizontal, 2], bins=bins, weights=areas[horizontal])
    low_score, high_score = float(hist[:32].max(initial=0)), float(hist[32:].max(initial=0))
    auto_flip = low_score > high_score
    if bool(auto_flip) != bool(args.flip):
        aligned[:, 1:] *= -1
    lo, hi = aligned.min(axis=0), aligned.max(axis=0)
    aligned = (aligned-np.array([(lo[0]+hi[0])/2, (lo[1]+hi[1])/2, hi[2]])) * (np.array([1.30, .36, .08])/(hi-lo))
    for vertex, point in zip(mesh.vertices, aligned):
        vertex.co = point
    mesh.update()
    # Imported custom normals refer to the old unscaled source axes. Let the
    # preserved smoothing flags and final geometry define new tangent frames.
    if hasattr(bpy.ops.mesh, "customdata_custom_splitnormals_clear"):
        try:
            bpy.ops.mesh.customdata_custom_splitnormals_clear()
        except (RuntimeError, AttributeError):
            pass
    centers, normals, areas = face_data(aligned)
    uv = mesh.uv_layers[0]
    polygon_uv = np.array([[uv.data[i].uv[:] for i in p.loop_indices] for p in mesh.polygons])
    image = prepared_images["base_color"]
    width, height = image.size
    pixels = np.empty(width*height*4, dtype=np.float32); image.pixels.foreach_get(pixels)
    pixels = pixels.reshape(height, width, 4)
    uv_center = polygon_uv.mean(axis=1)
    xx = np.clip((uv_center[:, 0]*width).astype(int), 0, width-1)
    yy = np.clip((uv_center[:, 1]*height).astype(int), 0, height-1)
    colour = pixels[yy, xx, :3]
    brightness = colour @ np.array([.2126, .7152, .0722])
    # Only original underside faces can become the lamp lens. Top/back, enamel
    # sidewalls, fasteners and end clips remain on the non-emitting housing.
    underside = (centers[:, 2] < -.042) & (normals[:, 2] < -.28)
    inset = (np.abs(centers[:, 0]) < .612) & (np.abs(centers[:, 1]) < .170)
    candidate = underside & inset & (brightness > .12)
    projected = float(np.sum(areas[candidate]*np.abs(normals[candidate, 2])))
    projected_ratio = projected/(1.30*.36)
    use_original = args.lens_mode not in ("strips", "tubes") and .10 <= projected_ratio <= .45 and int(candidate.sum()) >= 16
    if args.lens_mode == "original" and not use_original:
        raise ValueError("Original lens selection ambiguous: projected coverage %.3f; preview --flip or use --lens-mode strips" % projected_ratio)

    def subset(name, selected):
        data = mesh.copy(); obj = bpy.data.objects.new(name, data); bpy.context.scene.collection.objects.link(obj)
        bm = bmesh.new(); bm.from_mesh(data); bm.faces.ensure_lookup_table()
        bmesh.ops.delete(bm, geom=[f for i, f in enumerate(bm.faces) if not selected[i]], context="FACES")
        bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
        bm.normal_update(); bm.to_mesh(data); bm.free(); data.update()
        return obj

    body = subset("SM_Saro_FluorescentBody", ~candidate if use_original else np.ones(len(indices), dtype=bool))
    tube_centres = []
    if args.lens_mode == "tubes":
        # Generated housings can carry baked dirt/shadows across the glass.
        # Straight T12 inserts keep those shadows out of the emitting surface.
        # The Tripo housing and its original atlas remain entirely untouched.
        verts, faces = [], []
        sides = 24
        tube_centres = [-.08, .08]
        for y in tube_centres:
            offset = len(verts)
            for x in (-.59, .59):
                for i in range(sides):
                    angle = 2*math.pi*i/sides
                    verts.append((x, y+.018*math.cos(angle), -.078+.018*math.sin(angle)))
            for i in range(sides):
                j = (i+1) % sides
                faces.append((offset+i, offset+j, offset+sides+j, offset+sides+i))
            faces.append(tuple(offset+i for i in reversed(range(sides))))
            faces.append(tuple(offset+sides+i for i in range(sides)))
        data = bpy.data.meshes.new("Straight_T12_glass_inserts")
        data.from_pydata(verts, [], faces); data.update()
        lens = bpy.data.objects.new("SM_Saro_FluorescentLens", data)
        bpy.context.scene.collection.objects.link(lens)
        for polygon in data.polygons:
            polygon.use_smooth = len(polygon.vertices) == 4
        active(lens)
        triangulate = lens.modifiers.new("Glass_delivery_triangles", "TRIANGULATE")
        bpy.ops.object.modifier_apply(modifier=triangulate.name)
        split_mode = "straight_T12_glass_inserts_original_housing"
    elif use_original:
        lens = subset("SM_Saro_FluorescentLens", candidate)
        split_mode = "original_underside_faces_UV_preserved"
    else:
        # Two emitting skins sit on actual original underside ray hits. The
        # lateral centres come from each half's measured deepest bright profile.
        eligible = underside & (np.abs(centers[:, 0]) < .50)
        for sign in (-1, 1):
            choose = eligible & (centers[:, 1]*sign > .012)
            if not np.any(choose):
                raise ValueError("No measured underside profile; preview the orientation using --flip")
            weights = areas[choose]*np.maximum(brightness[choose], .05)*np.maximum(-centers[choose, 2]-.035, .001)**2
            tube_centres.append(float(np.clip(np.average(centers[choose, 1], weights=weights), -.130, .130)))
        tree = BVHTree.FromPolygons([Vector(p) for p in aligned], [tuple(i) for i in indices], all_triangles=True)
        verts, texcoords, faces = [], [], []
        nx, ny = 32, 6
        for center_y in tube_centres:
            offset = len(verts)
            for ix in range(nx+1):
                x = -.59+1.18*ix/nx
                for iy in range(ny+1):
                    y = center_y-.019+.038*iy/ny
                    loc, normal, face, distance = tree.ray_cast(Vector((x, y, -.20)), Vector((0, 0, 1)), .30)
                    if loc is None or loc.z > -.035:
                        raise ValueError("Tube skin ray missed real underside; preview --flip")
                    a, b, c = [Vector(p) for p in aligned[indices[face]]]
                    ua, ub, uc = [Vector((p[0], p[1], 0)) for p in polygon_uv[face]]
                    tex = barycentric_transform(loc, a, b, c, ua, ub, uc)
                    verts.append((loc.x, loc.y, loc.z-.00025)); texcoords.append((tex.x, tex.y))
            for ix in range(nx):
                for iy in range(ny):
                    a = offset+ix*(ny+1)+iy; b = a+ny+1
                    faces.extend(((a, a+1, b+1), (a, b+1, b)))
        data = bpy.data.meshes.new("Measured_T12_emitting_skins"); data.from_pydata(verts, [], faces); data.update()
        layer = data.uv_layers.new(name="S47_Atlas")
        for loop in data.loops:
            layer.data[loop.index].uv = texcoords[loop.vertex_index]
        lens = bpy.data.objects.new("SM_Saro_FluorescentLens", data); bpy.context.scene.collection.objects.link(lens)
        split_mode = "two_measured_underside_T12_skins_original_UV"

    material_rows = []
    for obj, name, roughness in ((body, "M_Saro_FluorescentBody", .62), (lens, "M_Saro_FluorescentLens", .38)):
        clean_glass = obj == lens and args.lens_mode == "tubes"
        object_textures = {} if clean_glass else prepared_textures
        row = {"name": name, "base_color_linear": [.80, .79, .72, 1] if clean_glass else [1, 1, 1, 1], "roughness": roughness, "metallic": 0,
               "specular": .5, "normal_strength": 1, "normal_convention": "DirectX", "textures": object_textures}
        material_rows.append(row)
        mat = bpy.data.materials.new(name); mat.use_nodes = True
        shader = mat.node_tree.nodes.get("Principled BSDF")
        shader.inputs["Roughness"].default_value = roughness
        shader.inputs["Base Color"].default_value = row["base_color_linear"]
        for role, image in prepared_images.items():
            if clean_glass:
                continue
            node = mat.node_tree.nodes.new("ShaderNodeTexImage"); node.image = image
            if role == "base_color":
                mat.node_tree.links.new(node.outputs["Color"], shader.inputs["Base Color"])
            elif role in ("roughness", "metallic"):
                mat.node_tree.links.new(node.outputs["Color"], shader.inputs[role.title()])
            elif role == "normal":
                nodes, links = mat.node_tree.nodes, mat.node_tree.links
                separate = nodes.new("ShaderNodeSeparateColor")
                inverse = nodes.new("ShaderNodeMath"); inverse.operation = "SUBTRACT"; inverse.inputs[0].default_value = 1
                combine = nodes.new("ShaderNodeCombineColor"); normal = nodes.new("ShaderNodeNormalMap")
                links.new(node.outputs["Color"], separate.inputs["Color"])
                links.new(separate.outputs["Green"], inverse.inputs[1])
                links.new(separate.outputs["Red"], combine.inputs["Red"])
                links.new(inverse.outputs[0], combine.inputs["Green"])
                links.new(separate.outputs["Blue"], combine.inputs["Blue"])
                links.new(combine.outputs[0], normal.inputs["Color"])
                links.new(normal.outputs["Normal"], shader.inputs["Normal"])
        obj.data.materials.clear(); obj.data.materials.append(mat)
        for polygon in obj.data.polygons:
            polygon.material_index = 0
    records = []
    for obj in (body, lens):
        active(obj)
        bpy.ops.export_scene.fbx(filepath=str(out/(obj.name+".fbx")), use_selection=True,
            object_types={"MESH"}, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
            axis_forward="-Y", axis_up="Z", bake_anim=False, mesh_smooth_type="FACE", use_tspace=True)
        coords = [v.co[:] for v in obj.data.vertices]
        records.append({"asset": obj.name, "fbx": obj.name+".fbx", "triangles": len(obj.data.polygons),
                        "bounds_local_m": [[fn(v[i] for v in coords) for i in range(3)] for fn in (min, max)]})
    original.name = "Normalized_Tripo_source_reference"; original.hide_set(True); original.hide_render = True
    contract = {"meshes": records, "source": str(source), "source_triangles": source_triangles,
        "prepared_source_triangles": len(mesh.polygons), "max_triangles": args.max_triangles,
        "normalization": {"dimensions_blender_m": [1.30, .36, .08], "shared_origin": "top-centre, topZ0 bottomZ-.08",
                          "PCA_basis_columns": axes.tolist(), "automatic_side_flip": bool(auto_flip), "manual_flip": args.flip,
                          "flat_side_area_scores": {"lower": low_score, "upper": high_score}},
        "lens": {"mode": split_mode, "original_projected_area_ratio": projected_ratio,
                 "source_faces_selected": int(candidate.sum()), "tube_centres_blender_y_m": tube_centres},
        "collision": "NoCollision; original fixtures retain power/visibility/flicker drivers",
        "materials": [row["name"] for row in material_rows], "normal_y": "DirectX",
        "units": "metres", "mapping": "Blender(x,y,z) -> UE(100x,-100y,100z); shared ceiling anchor"}
    (out/"fixtures-contract.json").write_text(json.dumps(contract, indent=2)+"\n")
    (out/"fixtures-materials.json").write_text(json.dumps(material_rows, indent=2)+"\n")
    for image in prepared_images.values():
        image.pack()
    bpy.ops.file.make_paths_relative(); bpy.ops.wm.save_as_mainfile(filepath=str(out/"editable.blend"))
    print("TRIPO FIXTURE PREPARED", json.dumps({"meshes": records, "lens_mode": split_mode, "coverage": projected_ratio}), flush=True)


if __name__ == "__main__":
    main()
