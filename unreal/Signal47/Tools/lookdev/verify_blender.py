"""Small direct Blender fixtures for the scoped lookdev material operations.

Run in a reserved Blender process, for example::

    blender --background --factory-startup --python verify_blender.py

No rendering, external source asset, installed game or Unreal session is used.
All fixture IDs and mask files are private temporary data and cleaned afterward.
"""
from __future__ import annotations

import copy
from pathlib import Path
import sys
import tempfile
import uuid

import bpy

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))
from materials import apply_materials

CHECKS = 0


def check(value, message):
    global CHECKS
    if not value:
        raise AssertionError(message)
    CHECKS += 1


def vector(value):
    return tuple(float(x) for x in value)


def material_snapshot(material):
    tree = material.node_tree
    nodes = []
    for n in tree.nodes:
        sockets = []
        for socket in n.inputs:
            if not hasattr(socket, "default_value"):
                continue
            value = socket.default_value
            if isinstance(value, (bool, int, float, str)):
                serialized = value
            elif hasattr(value, "__iter__"):
                serialized = tuple(value)
            else:
                serialized = str(value)
            sockets.append((socket.identifier, serialized))
        image = n.image if n.type == "TEX_IMAGE" else None
        nodes.append((n.name, n.bl_idname, tuple(sockets),
                      image.as_pointer() if image else None,
                      image.colorspace_settings.name if image else None))
    links = sorted((l.from_node.name, l.from_socket.identifier,
                    l.to_node.name, l.to_socket.identifier) for l in tree.links)
    return (tuple(sorted(nodes)), tuple(links), material.use_fake_user)


def geometry_snapshot(obj):
    bpy.context.view_layer.update()
    mesh = obj.data
    return (tuple(vector(v.co) for v in mesh.vertices),
            tuple((tuple(p.vertices), p.material_index, p.use_smooth) for p in mesh.polygons),
            tuple((layer.name, tuple(vector(v.uv) for v in layer.data)) for layer in mesh.uv_layers),
            tuple(vector(v.normal) for v in mesh.vertices),
            tuple(vector(row) for row in obj.matrix_world))


def fixture_mesh(prefix, name, closed=True):
    mesh = bpy.data.meshes.new(prefix + name)
    if closed:
        verts = [(-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1),
                 (-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)]
        faces = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4),
                 (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    else:
        verts = [(-1, -1, 0), (1, -1, 0), (1, 1, 0), (-1, 1, 0)]
        faces = [(0, 1, 2, 3)]
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    uv = mesh.uv_layers.new(name="UVMap")
    corners = ((0, 0), (1, 0), (1, 1), (0, 1))
    for poly in mesh.polygons:
        for index, loop in enumerate(poly.loop_indices):
            uv.data[loop].uv = corners[index]
    return mesh


def fixture_object(prefix, name, mesh):
    obj = bpy.data.objects.new(prefix + name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def fixture_source(prefix, mask_path):
    image = bpy.data.images.new(prefix + "SourceColor", width=2, height=2)
    image.colorspace_settings.name = "sRGB"
    image.pixels[:] = [0.2, 0.4, 0.6, 1.0, 0.8, 0.1, 0.3, 0.25,
                       0.5, 0.7, 0.2, 0.5, 0.9, 0.9, 0.9, 1.0]
    image.filepath_raw = str(mask_path)
    image.file_format = "PNG"
    image.save()
    material = bpy.data.materials.new(prefix + "SourceMaterial")
    material.use_nodes = True
    tree = material.node_tree
    bsdf = next(n for n in tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Roughness"].default_value = .61
    tex = tree.nodes.new("ShaderNodeTexImage")
    tex.name = "OriginalColorTexture"
    tex.image = image
    tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    normal = tree.nodes.new("ShaderNodeNormalMap")
    normal.name = "OriginalNormalMap"
    normal.inputs["Color"].default_value = (.5, .5, 1, 1)
    tree.links.new(normal.outputs["Normal"], bsdf.inputs["Normal"])
    return material, image


def expect_rejected(rules, base_dir, message):
    try:
        apply_materials(rules, base_dir)
    except ValueError:
        check(True, message)
    else:
        check(False, message)


def exercise(base_dir, prefix):
    source, image = fixture_source(prefix, base_dir / "shared-color-mask.png")
    second = bpy.data.materials.new(prefix + "SecondSlot")
    second.use_nodes = True
    mesh = fixture_mesh(prefix, "SharedCube")
    mesh.materials.append(source)
    mesh.materials.append(second)
    target = fixture_object(prefix, "Target", mesh)
    other = fixture_object(prefix, "Untouched", mesh)
    target.location = (2, 0, 0)
    other.location = (-2, 0, 0)
    original_geometry = geometry_snapshot(target)
    original_other_geometry = geometry_snapshot(other)
    original_material = material_snapshot(source)
    original_second = material_snapshot(second)

    # One explicit object/slot must not mutate another object sharing its mesh.
    first = {"object": target.name, "slot": 0, "refine": {"roughness": .41}}
    rows = apply_materials([first], base_dir)
    check(rows[0]["material_name"] == target.material_slots[0].material.name, "Report points to assigned material")
    check(target.material_slots[0].link == "OBJECT", "Target uses an independent object slot")
    check(other.material_slots[0].material == source, "Shared-mesh neighbour keeps original source")
    check(mesh.materials[0] == source, "Shared mesh material table is unchanged")
    check(target.material_slots[1].material == second, "Untargeted slot is untouched")
    check(material_snapshot(source) == original_material, "Source graph is unchanged after first application")
    check(material_snapshot(second) == original_second, "Untargeted second material graph is unchanged")
    check(geometry_snapshot(target) == original_geometry, "Target UVs, mesh, normals and transform are unchanged")
    check(geometry_snapshot(other) == original_other_geometry, "Neighbour geometry and transform are unchanged")

    # The same file can be color input and a data mask without changing its ID.
    masked = {"object": target.name, "slot": 0,
              "refine": {"roughness": .9, "metallic": .7,
                         "mask_image": "shared-color-mask.png", "channel": "R"},
              "wear": {"amount": .2, "scale": 32, "bump_distance": .0003,
                       "bump_strength": .12, "mask_image": "shared-color-mask.png", "channel": "A"}}
    rows = apply_materials([masked], base_dir)
    result = target.material_slots[0].material
    bsdf = next(n for n in result.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    check(image.colorspace_settings.name == "sRGB", "Source color image stays sRGB")
    mask_images = [n.image for n in result.node_tree.nodes if n.type == "TEX_IMAGE"
                   and n.image is not None and n.image.get("s47_lookdev_owner")]
    check(len(mask_images) == 2, "Refine and wear use their private image masks")
    check(all(i != image and i.colorspace_settings.name == "Non-Color" for i in mask_images),
          "All data masks use separate Non-Color image IDs")
    color_mix = bsdf.inputs["Base Color"].links[0].from_node
    check(color_mix.type == "MIX_RGB" and color_mix.inputs[1].is_linked,
          "Wear tint retains the original color branch")
    check(color_mix.inputs[1].links[0].from_node.image == image, "Original color texture remains connected")
    bump = bsdf.inputs["Normal"].links[0].from_node
    check(bump.type == "BUMP" and bump.inputs["Normal"].is_linked, "Wear bump chains over the original normal")
    check(bump.inputs["Normal"].links[0].from_node.name == "OriginalNormalMap", "Original normal map remains connected")
    check(material_snapshot(source) == original_material, "Masked refinement does not edit original material nodes")
    baseline = rows[0]["source_backup"]

    # Repeating is stable; a changed rule starts from the preserved source.
    counts = (len(bpy.data.materials), len(bpy.data.images))
    original_result = result
    again = apply_materials([copy.deepcopy(masked)], base_dir)
    check(target.material_slots[0].material == original_result, "Unchanged rule reuses the material ID")
    check(counts == (len(bpy.data.materials), len(bpy.data.images)), "Unchanged rule adds no materials or images")
    check(again == rows, "Unchanged rule returns the same report")
    changed = {"object": target.name, "slot": 0, "refine": {"roughness": .23}}
    new_rows = apply_materials([changed], base_dir)
    current = target.material_slots[0].material
    bsdf = next(n for n in current.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    check(abs(bsdf.inputs["Roughness"].default_value - .23) < 1e-6, "Changed refinement takes effect")
    check(bsdf.inputs["Base Color"].links[0].from_node.image == image, "Rebuild starts with original color branch")
    check(bsdf.inputs["Normal"].links[0].from_node.type == "NORMAL_MAP", "Rebuild does not stack old bump nodes")
    check(not any("Surface wear" in n.label for n in current.node_tree.nodes), "Removed wear nodes do not accumulate")
    check(new_rows[0]["source_backup"] == baseline, "Rebuild retains the same original backup")
    check(material_snapshot(source) == original_material, "Changed rule still preserves original material")

    # An invalid later rule must be rejected before the earlier rule is assigned.
    before = target.material_slots[0].material
    counts = (len(bpy.data.materials), len(bpy.data.images))
    expect_rejected([{"object": target.name, "refine": {"roughness": .72}},
                     {"object": prefix + "DoesNotExist", "refine": {"roughness": .5}}],
                    base_dir, "Malformed second rule is rejected")
    check(target.material_slots[0].material == before, "Malformed second rule causes zero target assignments")
    check(counts == (len(bpy.data.materials), len(bpy.data.images)), "Preflight rejection leaves no material/image copies")

    # An open window surface is not silently treated as a thick glass volume.
    plane_mesh = fixture_mesh(prefix, "OpenPane", closed=False)
    plane_mesh.materials.append(source)
    pane = fixture_object(prefix, "Pane", plane_mesh)
    original_pane_geometry = geometry_snapshot(pane)
    expect_rejected([{"object": pane.name, "kind": "glass", "glass": {"mode": "solid"}}],
                    base_dir, "Open mesh rejects solid glass")
    check(pane.material_slots[0].material == source, "Rejected solid glass does not replace the pane material")
    glass_rows = apply_materials([{"object": pane.name, "kind": "glass", "glass": {"mode": "pane"}}], base_dir)
    pane_material = pane.material_slots[0].material
    output = next(n for n in pane_material.node_tree.nodes if n.type == "OUTPUT_MATERIAL" and n.is_active_output)
    check(output.inputs["Surface"].is_linked and output.inputs["Surface"].links[0].from_node.type == "MIX_SHADER",
          "Pane uses an explicit thin-surface shader mix")
    check(not output.inputs["Volume"].is_linked, "Thin pane has no fake volume absorption")
    check(glass_rows[0]["glass"]["mode"] == "pane" and not glass_rows[0]["bake_supported"],
          "Pane export is marked for native Unreal glass")
    check(geometry_snapshot(pane) == original_pane_geometry, "Pane geometry is not extruded or otherwise changed")
    check(material_snapshot(source) == original_material, "Glass setup does not modify the shared source")


def main():
    prefix = "_S47LD_Test_" + uuid.uuid4().hex[:8] + "_"
    collections = (bpy.data.objects, bpy.data.materials, bpy.data.meshes, bpy.data.images)
    before = [set(blocks) for blocks in collections]
    try:
        with tempfile.TemporaryDirectory(prefix="s47-lookdev-material-tests-") as temp:
            exercise(Path(temp), prefix)
        print(f"S47_LOOKDEV_MATERIAL_TESTS PASS {CHECKS}")
    finally:
        # Remove only IDs allocated by these fixtures, in reference-safe order.
        for blocks, existing in zip(collections, before):
            for block in list(blocks):
                if block not in existing:
                    blocks.remove(block, do_unlink=True)


if __name__ == "__main__":
    main()
