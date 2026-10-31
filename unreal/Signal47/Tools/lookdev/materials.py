"""Editable, explicitly scoped Blender material refinement for owned/Tripo assets.

API: ``apply_materials(rules, base_dir) -> reports``. Rules target one exact
``object`` and zero-based ``slot``. No mesh, UV, transform, normal or rig data is
edited. Target slots become object-linked so shared meshes remain independent.
Only a direct Principled BSDF (optionally through reroutes) is supported; mixed
or nested surfaces fail rather than guessing which branch to overwrite.

``refine`` accepts roughness/metallic plus optional mask_image/channel (R/G/B/A).
``wear`` accepts amount (blend strength), scale (noise frequency per metre),
roughness, color (linear RGBA), mask_image/channel, bump_distance (metres) and
bump_strength. This is mottled surface wear, not curvature/edge detection.
``glass`` accepts mode (solid/pane), tint (linear RGBA), ior, roughness,
thickness (metres, export metadata only) and absorption (density per metre).
Solid requires existing closed geometry; pane is a thin Fresnel surface without
refraction/absorption. Neither mode creates or thickens geometry.

The original input graphs are preserved in owned backup materials, with a fake
user so a saved .blend retains the baseline. Reapplying an unchanged rule reuses
the result; changed rules rebuild from that baseline, not the refined graph.
Procedural nodes require baking for Unreal; glass needs a native Unreal shader.
"""
from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path

import bpy

OWNER = "s47_lookdev_owner"
SOURCE = "s47_lookdev_source"
RULE_HASH = "s47_lookdev_rule_hash"
REPORT = "s47_lookdev_report"
VERSION = 1


def _require(condition, message):
    if not condition:
        raise ValueError(message)


def _number(value, label, low, high):
    _require(isinstance(value, (int, float)) and not isinstance(value, bool)
             and math.isfinite(value) and low <= value <= high,
             f"{label} must be a finite number in [{low}, {high}]")
    return float(value)


def _color(value, label):
    _require(isinstance(value, (list, tuple)) and len(value) in (3, 4),
             label + " must be RGB or RGBA")
    c = tuple(_number(v, label, 0, 1) for v in value)
    return c + (1.0,) if len(c) == 3 else c


def _options(options, allowed, label):
    _require(isinstance(options, dict), label + " must be an object")
    _require(not (set(options) - set(allowed)),
             label + " contains unknown settings: " + ", ".join(sorted(set(options) - set(allowed))))


def _mask_path(options, base_dir):
    if "mask_image" not in options:
        _require("channel" not in options, "channel requires mask_image")
        return None
    raw = options["mask_image"]
    _require(isinstance(raw, str) and raw, "mask_image must be a path")
    p = Path(raw).expanduser()
    p = (base_dir / p if not p.is_absolute() else p).resolve()
    _require(p.is_file(), "Missing material mask: " + str(p))
    _require(options.get("channel", "R") in ("R", "G", "B", "A"),
             "Mask channel must be R, G, B or A")
    return p


def _principled(material):
    if not material.use_nodes:
        return None
    outputs = [n for n in material.node_tree.nodes if n.type == "OUTPUT_MATERIAL"]
    active = [n for n in outputs if n.is_active_output]
    _require(len(active) == 1 or len(outputs) == 1,
             "Ambiguous material output: " + material.name)
    output = active[0] if active else outputs[0]
    _require(not output.inputs["Volume"].is_linked,
             "Existing volume needs a separate reviewed mapping: " + material.name)
    socket = output.inputs["Surface"]
    seen = set()
    while socket.is_linked:
        node = socket.links[0].from_node
        _require(node.as_pointer() not in seen, "Cyclic shader graph: " + material.name)
        seen.add(node.as_pointer())
        if node.type == "REROUTE":
            socket = node.inputs[0]
            continue
        _require(node.type == "BSDF_PRINCIPLED",
                 "Refinement requires a direct Principled surface: " + material.name)
        for name in ("Base Color", "Metallic", "Roughness", "Normal", "Alpha", "IOR", "Transmission Weight"):
            _require(node.inputs.get(name) is not None, "Principled v2 input missing: " + name)
        return node
    raise ValueError("Unconnected material surface: " + material.name)


def _preflight(rule, base_dir, unit):
    _options(rule, ("object", "slot", "kind", "name", "refine", "wear", "glass"), "material rule")
    _require(isinstance(rule.get("object"), str) and rule["object"], "Exact object name required")
    obj = bpy.data.objects.get(rule["object"])
    _require(obj is not None and obj.type == "MESH", "Missing mesh object: " + rule["object"])
    slot = rule.get("slot", 0)
    _require(isinstance(slot, int) and not isinstance(slot, bool) and 0 <= slot < len(obj.material_slots),
             "Invalid material slot on " + obj.name)
    _require(obj.library is None and obj.override_library is None, "Use a local editable object: " + obj.name)
    current = obj.material_slots[slot].material
    _require(current is not None, "Empty material slot on " + obj.name)
    owner = hashlib.sha256((obj.name + "\0" + str(slot)).encode()).hexdigest()[:16]
    source = current
    if current.get(OWNER) == owner and current.get(SOURCE):
        source = bpy.data.materials.get(current[SOURCE])
        _require(source is not None, "Preserved source material is missing for " + obj.name)
    _principled(source)
    kind = rule.get("kind", "surface")
    _require(kind in ("surface", "glass"), "kind must be surface or glass")
    _require(not (kind == "glass" and ("refine" in rule or "wear" in rule)),
             "Glass uses glass settings, not surface refinement/wear")
    _require(not (kind == "surface" and "glass" in rule), "Surface rule cannot contain glass settings")
    masks = {}
    if "name" in rule:
        _require(isinstance(rule["name"], str) and rule["name"], "name must be non-empty")
    if kind == "surface":
        refine = rule.get("refine", {})
        wear = rule.get("wear", {})
        _options(refine, ("roughness", "metallic", "mask_image", "channel"), "refine")
        _options(wear, ("amount", "scale", "roughness", "color", "mask_image", "channel", "bump_distance", "bump_strength"), "wear")
        for key in ("roughness", "metallic"):
            if key in refine:
                _number(refine[key], "refine." + key, 0, 1)
        if "mask_image" in refine:
            _require(any(k in refine for k in ("roughness", "metallic")), "Refine mask has no target channel")
        if wear:
            _number(wear.get("amount", .08), "wear.amount", 0, 1)
            _number(wear.get("scale", 24), "wear.scale", .001, 100000)
            _number(wear.get("roughness", .8), "wear.roughness", 0, 1)
            _color(wear.get("color", (.16, .12, .08, 1)), "wear.color")
            _number(wear.get("bump_distance", .0004), "wear.bump_distance", 0, .02)
            _number(wear.get("bump_strength", .15), "wear.bump_strength", 0, 1)
        masks = {k: _mask_path(v, base_dir) for k, v in (("refine", refine), ("wear", wear))}
        if any(masks.values()):
            _require(bool(obj.data.uv_layers), "Image masks need an existing UV map: " + obj.name)
    else:
        glass = rule.get("glass", {})
        _options(glass, ("mode", "tint", "ior", "roughness", "thickness", "absorption"), "glass")
        _require(glass.get("mode", "solid") in ("solid", "pane"), "glass.mode must be solid or pane")
        _color(glass.get("tint", (.96, .985, 1, 1)), "glass.tint")
        _number(glass.get("ior", 1.45), "glass.ior", 1, 3)
        _number(glass.get("roughness", .06), "glass.roughness", 0, 1)
        _number(glass.get("thickness", .004), "glass.thickness", .00001, 10)
        _number(glass.get("absorption", 0), "glass.absorption", 0, 10000)
        _require(glass.get("mode", "solid") != "pane" or glass.get("absorption", 0) == 0,
                 "Pane mode has no volume; set absorption to 0")
        if glass.get("mode", "solid") == "solid":
            # A closed edge manifold is a minimum, not proof of a sound watertight model.
            edge_users = {}
            for poly in obj.data.polygons:
                for edge in poly.edge_keys:
                    edge_users[edge] = edge_users.get(edge, 0) + 1
            _require(bool(edge_users) and all(n == 2 for n in edge_users.values()),
                     "Solid glass requires closed existing mesh geometry: " + obj.name)
    file_identity = {k: (str(p), p.stat().st_size, p.stat().st_mtime_ns) for k, p in masks.items() if p}
    signature = hashlib.sha256(json.dumps({"version": VERSION, "rule": rule, "masks": file_identity,
                                          "units": unit, "scale": list(obj.matrix_world.to_scale())},
                                         sort_keys=True).encode()).hexdigest()
    return obj, slot, current, source, owner, masks, signature


def _node(tree, kind, label, x=0, y=0):
    n = tree.nodes.new(kind)
    n.name = "S47LD_" + label
    n.label = label
    n.location = (x, y)
    return n


def _input_value(tree, socket):
    if socket.is_linked:
        return socket.links[0].from_socket, None
    value = socket.default_value
    return None, tuple(value) if hasattr(value, "__len__") else float(value)


def _replace_input(tree, socket, value):
    for link in list(socket.links):
        tree.links.remove(link)
    socket.default_value = value


def _mix_input(tree, socket, factor, target, label):
    source, default = _input_value(tree, socket)
    n = _node(tree, "ShaderNodeMixRGB", label, -180, 100)
    n.blend_type = "MIX"
    tree.links.new(factor, n.inputs[0])
    if source:
        tree.links.new(source, n.inputs[1])
    else:
        n.inputs[1].default_value = default if isinstance(default, tuple) else (default, default, default, 1)
    n.inputs[2].default_value = target if isinstance(target, tuple) else (target, target, target, 1)
    tree.links.new(n.outputs[0], socket)


def _mask(tree, path, channel, owner, label, made_images):
    # Never mutate a source image's color space; masks have a private image ID.
    image = bpy.data.images.load(str(path), check_existing=False)
    image.name = "S47LD_" + owner + "_" + label
    image[OWNER] = owner
    image.colorspace_settings.name = "Non-Color"
    made_images.append(image)
    uv = _node(tree, "ShaderNodeTexCoord", label + " UV", -950, -300)
    tex = _node(tree, "ShaderNodeTexImage", label + " mask (Non-Color)", -740, -300)
    tex.image = image
    tex.interpolation = "Linear"
    tex.extension = "EXTEND"
    tree.links.new(uv.outputs["UV"], tex.inputs["Vector"])
    if channel == "A":
        return tex.outputs["Alpha"]
    split = _node(tree, "ShaderNodeSeparateColor", label + " channel " + channel, -500, -300)
    split.mode = "RGB"
    tree.links.new(tex.outputs["Color"], split.inputs["Color"])
    return split.outputs[{"R": "Red", "G": "Green", "B": "Blue"}[channel]]


def _surface(material, bsdf, rule, masks, owner, obj, unit, made_images):
    tree = material.node_tree
    refine = rule.get("refine", {})
    mask = _mask(tree, masks["refine"], refine.get("channel", "R"), owner, "refine", made_images) if masks.get("refine") else None
    for key, name in (("roughness", "Roughness"), ("metallic", "Metallic")):
        if key not in refine:
            continue
        if mask:
            _mix_input(tree, bsdf.inputs[name], mask, float(refine[key]), "Selective " + name)
        else:
            _replace_input(tree, bsdf.inputs[name], float(refine[key]))
    wear = rule.get("wear", {})
    if not wear or wear.get("amount", .08) == 0:
        return
    coords = _node(tree, "ShaderNodeTexCoord", "Object wear coordinates", -1100, 300)
    scale = _node(tree, "ShaderNodeVectorMath", "Object coordinates in metres", -900, 300)
    scale.operation = "MULTIPLY"
    scale.inputs[1].default_value = tuple(abs(v) * unit for v in obj.matrix_world.to_scale())
    tree.links.new(coords.outputs["Object"], scale.inputs[0])
    noise = _node(tree, "ShaderNodeTexNoise", "Mottled wear (not edge curvature)", -680, 300)
    noise.noise_dimensions = "3D"
    noise.inputs["Scale"].default_value = float(wear.get("scale", 24))
    noise.inputs["Detail"].default_value = 3
    noise.inputs["Roughness"].default_value = .7
    tree.links.new(scale.outputs["Vector"], noise.inputs["Vector"])
    ramp = _node(tree, "ShaderNodeValToRGB", "Sparse wear distribution", -450, 300)
    ramp.color_ramp.elements[0].position = .50
    ramp.color_ramp.elements[1].position = .66
    tree.links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
    strength = _node(tree, "ShaderNodeMath", "Wear blend strength", -220, 300)
    strength.operation = "MULTIPLY"
    strength.inputs[1].default_value = float(wear.get("amount", .08))
    tree.links.new(ramp.outputs["Color"], strength.inputs[0])
    factor = strength.outputs[0]
    if masks.get("wear"):
        user_mask = _mask(tree, masks["wear"], wear.get("channel", "R"), owner, "wear", made_images)
        masked = _node(tree, "ShaderNodeMath", "Wear limited to selected surface", -10, 300)
        masked.operation = "MULTIPLY"
        tree.links.new(factor, masked.inputs[0]); tree.links.new(user_mask, masked.inputs[1])
        factor = masked.outputs[0]
    _mix_input(tree, bsdf.inputs["Base Color"], factor,
               _color(wear.get("color", (.16, .12, .08, 1)), "wear.color"), "Surface wear tint")
    _mix_input(tree, bsdf.inputs["Roughness"], factor, float(wear.get("roughness", .8)), "Surface wear roughness")
    if wear.get("bump_distance", .0004) and wear.get("bump_strength", .15):
        previous, _ = _input_value(tree, bsdf.inputs["Normal"])
        bump = _node(tree, "ShaderNodeBump", "Wear over original normal", 10, -150)
        bump.inputs["Distance"].default_value = float(wear.get("bump_distance", .0004)) / unit
        bump.inputs["Strength"].default_value = float(wear.get("bump_strength", .15))
        tree.links.new(factor, bump.inputs["Height"])
        if previous:
            tree.links.new(previous, bump.inputs["Normal"])
        tree.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])


def _glass(material, bsdf, settings, unit):
    tree = material.node_tree
    outputs = [n for n in tree.nodes if n.type == "OUTPUT_MATERIAL"]
    out = next((n for n in outputs if n.is_active_output), outputs[0])
    tint = _color(settings.get("tint", (.96, .985, 1, 1)), "glass.tint")
    ior = float(settings.get("ior", 1.45)); rough = float(settings.get("roughness", .06))
    if settings.get("mode", "solid") == "pane":
        # Thin pane approximation: Fresnel reflection without a solid-lens ray.
        normal, _ = _input_value(tree, bsdf.inputs["Normal"])
        transparent = _node(tree, "ShaderNodeBsdfTransparent", "Thin pane transmission", 0, 200)
        transparent.inputs["Color"].default_value = tint
        reflection = _node(tree, "ShaderNodeBsdfAnisotropic", "Thin pane reflection", 0, 50)
        reflection.inputs["Color"].default_value = (1, 1, 1, 1)
        reflection.inputs["Roughness"].default_value = rough
        fresnel = _node(tree, "ShaderNodeFresnel", "Glass Fresnel", -220, -70)
        fresnel.inputs["IOR"].default_value = ior
        if normal:
            tree.links.new(normal, reflection.inputs["Normal"])
            tree.links.new(normal, fresnel.inputs["Normal"])
        mix = _node(tree, "ShaderNodeMixShader", "Thin pane glass", 220, 170)
        tree.links.new(fresnel.outputs["Fac"], mix.inputs[0])
        tree.links.new(transparent.outputs[0], mix.inputs[1]); tree.links.new(reflection.outputs[0], mix.inputs[2])
        tree.links.new(mix.outputs[0], out.inputs["Surface"])
    else:
        for name, value in (("Base Color", tint), ("IOR", ior), ("Roughness", rough),
                            ("Metallic", 0.0), ("Transmission Weight", 1.0), ("Alpha", 1.0)):
            _replace_input(tree, bsdf.inputs[name], value)
        if settings.get("absorption", 0):
            absorb = _node(tree, "ShaderNodeVolumeAbsorption", "Glass absorption per metre", 120, -200)
            absorb.inputs["Color"].default_value = tint
            absorb.inputs["Density"].default_value = float(settings["absorption"]) * unit
            tree.links.new(absorb.outputs[0], out.inputs["Volume"])
    material.diffuse_color = tint
    material["s47_glass_mode"] = settings.get("mode", "solid")
    material["s47_glass_thickness_m"] = float(settings.get("thickness", .004))


def apply_materials(rules: list[dict], base_dir: Path) -> list[dict]:
    """Apply explicit rules atomically with respect to object-slot assignment.

    Returned records contain object, slot, kind, material, source_material,
    source_backup, rule_hash, bake_supported, export_requires and glass metadata.
    Source opacity/transmission is retained for surfaces; opaque-channel baking
    must reject unsupported source alpha/transmission as build_owned already does.
    """
    _require(bpy.app.version >= (4, 5, 0), "Lookdev materials require Blender 4.5+")
    _require(isinstance(rules, list), "materials must be a list")
    base_dir = Path(base_dir).resolve()
    unit = float(bpy.context.scene.unit_settings.scale_length)
    _number(unit, "scene.unit_settings.scale_length", .000001, 1000000)
    plans = [_preflight(rule, base_dir, unit) for rule in rules]
    targets = [(p[0].name, p[1]) for p in plans]
    _require(len(targets) == len(set(targets)), "One material rule per object/slot is required")
    staged = []
    made_materials = []
    made_images = []
    try:
        for rule, plan in zip(rules, plans):
            obj, slot, current, source, owner, masks, signature = plan
            if current.get(OWNER) == owner and current.get(RULE_HASH) == signature:
                staged.append((obj, slot, current, current, json.loads(current[REPORT])))
                continue
            if current.get(OWNER) == owner and current.get(SOURCE):
                baseline = source
            else:
                baseline = source.copy(); made_materials.append(baseline)
                baseline.name = "_S47LD_Source_" + owner + "_" + source.name
                baseline.use_fake_user = True
                baseline[OWNER] = owner
                baseline["s47_source_name"] = source.name
            material = baseline.copy(); made_materials.append(material)
            material.use_fake_user = False
            material.name = rule.get("name", "S47LD_" + baseline.get("s47_source_name", source.name) + "_" + owner[:6])
            material[OWNER] = owner; material[SOURCE] = baseline.name; material[RULE_HASH] = signature
            material.use_nodes = True
            bsdf = _principled(material)
            if rule.get("kind", "surface") == "glass":
                _glass(material, bsdf, rule.get("glass", {}), unit)
            else:
                _surface(material, bsdf, rule, masks, owner, obj, unit, made_images)
            kind = rule.get("kind", "surface")
            opaque = (not bsdf.inputs["Alpha"].is_linked and bsdf.inputs["Alpha"].default_value >= .9999
                      and not bsdf.inputs["Transmission Weight"].is_linked
                      and bsdf.inputs["Transmission Weight"].default_value <= .0001)
            report = {"object": obj.name, "slot": slot, "kind": kind,
                      "material": material.name, "material_name": material.name, "source_material": baseline.get("s47_source_name", source.name),
                      "source_backup": baseline.name, "rule_hash": signature,
                      "bake_supported": kind == "surface" and opaque,
                      "export_requires": "baked_pbr_textures" if kind == "surface" else "native_unreal_glass"}
            if kind == "surface":
                # These are unlinked/default fallback values. Linked/procedural
                # channel truth comes from the actual texture bake, never these.
                report["surface"] = {
                    "base_color": list(bsdf.inputs["Base Color"].default_value),
                    "roughness": float(bsdf.inputs["Roughness"].default_value),
                    "metallic": float(bsdf.inputs["Metallic"].default_value),
                }
                report["surface_values_are_fallbacks"] = True
            if kind == "glass":
                report["glass"] = {"mode": rule.get("glass", {}).get("mode", "solid"),
                                   "tint": list(_color(rule.get("glass", {}).get("tint", (.96, .985, 1, 1)), "tint")),
                                   "ior": float(rule.get("glass", {}).get("ior", 1.45)),
                                   "roughness": float(rule.get("glass", {}).get("roughness", .06)),
                                   "thickness_m": float(rule.get("glass", {}).get("thickness", .004)),
                                   "absorption_per_m": float(rule.get("glass", {}).get("absorption", 0))}
            material[REPORT] = json.dumps(report, sort_keys=True)
            staged.append((obj, slot, current, material, report))
    except Exception:
        for material in reversed(made_materials):
            bpy.data.materials.remove(material, do_unlink=True)
        for image in made_images:
            if image.users == 0:
                bpy.data.images.remove(image)
        raise
    for obj, slot, old, material, report in staged:
        obj.material_slots[slot].link = "OBJECT"
        obj.material_slots[slot].material = material
        if old != material and old.get(OWNER) == material.get(OWNER) and old.get(SOURCE) and old.users == 0:
            orphan_images = [n.image for n in old.node_tree.nodes if n.type == "TEX_IMAGE" and n.image and n.image.get(OWNER) == old[OWNER]]
            bpy.data.materials.remove(old)
            for image in set(orphan_images):
                if image.users == 0:
                    bpy.data.images.remove(image)
    return [row[4] for row in staged]
