"""Import baked lookdev materials, optionally build an isolated Unreal preview map.

Normal Python can print a validated import plan without loading Unreal. In the
editor use S47_LOOKDEV_MANIFEST, S47_LOOKDEV_PROFILE and S47_LOOKDEV_APPLY=1.
S47_LOOKDEV_PREVIEW=1 additionally creates/loads a separate preview map. This
module never assigns materials to game meshes or changes project settings.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re


OWNER_TAG = "S47Lookdev_v1"
DESTINATION = "/Game/Signal47/Lookdev/"


def _number(value, label, minimum, maximum):
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{label}: expected a number")
    value = float(value)
    if not math.isfinite(value) or not minimum <= value <= maximum:
        raise ValueError(f"{label}: expected {minimum}..{maximum}")
    return value


def _vector(value, label, length=3, minimum=-1e9, maximum=1e9):
    if not isinstance(value, (list, tuple)) or len(value) != length:
        raise ValueError(f"{label}: expected {length} components")
    return [_number(item, label, minimum, maximum) for item in value]


def _color(value, label):
    if not isinstance(value, (list, tuple)) or len(value) not in (3, 4):
        raise ValueError(f"{label}: expected RGB or RGBA")
    return _vector(value[:3], label, minimum=0, maximum=1) + [1.0]


def _slug(value):
    if not isinstance(value, str) or not value.strip():
        raise ValueError("Empty lookdev/asset name")
    name = re.sub(r"[^A-Za-z0-9_]", "_", value).strip("_")
    if not name:
        raise ValueError("Name has no valid asset characters")
    return name[:70]


def blender_to_unreal_meters(value):
    """Blender +Y forward/+X right/+Z up to Unreal +X forward/+Y right/+Z up."""
    x, y, z = _vector(value, "location_m")
    return [y * 100, x * 100, z * 100]


def _path(value, base_dir):
    if not isinstance(value, str) or not value:
        raise ValueError("Texture path must be a nonempty string")
    path = Path(value).expanduser()
    if not path.is_absolute():
        path = base_dir / path
    path = path.resolve()
    if not path.is_file():
        raise ValueError(f"Missing baked texture: {path}")
    if path.suffix.lower() not in (".png", ".tga", ".exr", ".tif", ".tiff"):
        raise ValueError(f"Unsupported texture type: {path}")
    return str(path)


def generate_plan(manifest, profile="day", base_dir=None):
    """Validate without importing Unreal, starting a process or changing files."""
    if not isinstance(manifest, dict) or type(manifest.get("schema_version")) is not int or manifest.get("schema_version") != 1:
        raise ValueError("Expected lookdev manifest schema_version 1")
    if manifest.get("coordinate_system", "Blender_Z_UP_meters") != "Blender_Z_UP_meters":
        raise ValueError("Expected coordinate_system Blender_Z_UP_meters")
    if profile not in ("day", "night"):
        raise ValueError("Profile must be day or night")
    base_dir = Path(base_dir or ".").resolve()
    name = _slug(manifest.get("name", "Scene"))
    dest = DESTINATION + name
    plan = {"schema_version": 1, "name": name, "profile": profile,
            "destination": dest, "materials": [], "lighting": None,
            "warnings": [], "apply_requirements": [], "preview_requirements": []}
    rows = manifest.get("materials", [])
    if not isinstance(rows, list):
        raise ValueError("materials must be a list")
    identities = set()
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError("material row must be an object")
        source_object = row.get("object")
        if not isinstance(source_object, str) or not source_object:
            raise ValueError("material object must be named explicitly")
        slot = row.get("slot", 0)
        if isinstance(slot, bool) or not isinstance(slot, int) or slot < 0:
            raise ValueError("Material slot must be a nonnegative integer")
        identity = (source_object, slot)
        if identity in identities:
            raise ValueError(f"Duplicate material target: {identity}")
        identities.add(identity)
        kind = row.get("kind", "surface")
        if kind not in ("surface", "glass"):
            raise ValueError(f"Unsupported material kind: {kind}")
        unique = hashlib.sha256(f"{source_object}\0{slot}".encode()).hexdigest()[:8]
        asset_name = "M_" + _slug(row.get("material_name", row.get("material", source_object))) + "_" + unique
        surface = row.get("surface", {})
        if not isinstance(surface, dict):
            raise ValueError("surface fallback must be an object")
        item = {"object": source_object, "slot": slot, "kind": kind,
                "asset_name": asset_name, "asset_path": dest + "/Materials/" + asset_name,
                "textures": {}, "normal_convention": row.get("normal_convention", "opengl"),
                "base_color": _color(row.get("base_color", surface.get("base_color", [0.5, 0.5, 0.5])), "base_color"),
                "roughness": _number(row.get("roughness", surface.get("roughness", 0.5)), "roughness", 0, 1),
                "metallic": _number(row.get("metallic", surface.get("metallic", 0)), "metallic", 0, 1)}
        if item["normal_convention"] not in ("opengl", "directx"):
            raise ValueError("normal_convention must be opengl or directx")
        textures = row.get("textures", {})
        if not isinstance(textures, dict) or set(textures) - {"base_color", "roughness", "metallic", "normal"}:
            raise ValueError("Unknown baked texture role")
        for role, path in textures.items():
            item["textures"][role] = _path(path, base_dir)
        fallbacks = row.get("surface_values_are_fallbacks", False)
        if not isinstance(fallbacks, bool):
            raise ValueError("surface_values_are_fallbacks must be a boolean")
        if kind == "surface" and fallbacks:
            missing = sorted({"base_color", "roughness", "metallic", "normal"} - set(textures))
            if missing:
                plan["apply_requirements"].append(
                    f"{source_object}[{slot}]: bake the procedural surface before importing; missing " + ", ".join(missing))
        if kind == "glass":
            glass = row.get("glass", {})
            if not isinstance(glass, dict) or glass.get("mode", "pane") not in ("pane", "solid"):
                raise ValueError("glass.mode must be pane or solid")
            item["glass"] = {"mode": glass.get("mode", "pane"),
                             "tint": _color(glass.get("tint", [1, 1, 1]), "glass.tint"),
                             "ior": _number(glass.get("ior", 1.45), "glass.ior", 1, 3),
                             "roughness": _number(glass.get("roughness", 0.05), "glass.roughness", 0, 1),
                             "thickness_m": _number(glass.get("thickness_m", glass.get("thickness", 0.004)), "glass.thickness_m", 0, 10),
                             "absorption_per_m": _number(glass.get("absorption_per_m", glass.get("absorption", 0)), "glass.absorption_per_m", 0, 1e5)}
            plan["warnings"].append(f"{source_object}: native translucent glass is an approximation; Blender volume absorption/caustics are not exported.")
        elif not textures:
            plan["warnings"].append(f"{source_object}: no baked maps; only explicit scalar/color fallback will be imported.")
        plan["materials"].append(item)
    lighting_all = manifest.get("lighting", {})
    if not isinstance(lighting_all, dict):
        raise ValueError("lighting must be an object")
    if profile in lighting_all:
        spec = lighting_all[profile]
        if not isinstance(spec, dict) or not isinstance(spec.get("unreal", {}), dict):
            raise ValueError("lighting profile and its unreal overrides must be objects")
        native = spec.get("unreal", {})
        output = {"directionals": [], "practicals": [], "sky_intensity": None,
                  "exposure_bias": None, "fog": None}

        def setting(key, low, high):
            if key not in native:
                plan["preview_requirements"].append(f"lighting.{profile}.unreal.{key}")
                return None
            return _number(native[key], key, low, high)

        output["sky_intensity"] = setting("sky_intensity", 0, 1e4)
        output["exposure_bias"] = setting("exposure_bias", -20, 20)
        for role in ("sun", "moon"):
            if role not in spec:
                continue
            light = spec[role]
            if not isinstance(light, dict):
                raise ValueError(f"{role}: expected an object")
            output["directionals"].append({"role": role, "lux": setting(role + "_lux", 0, 1e7),
                "azimuth_deg": _number(light.get("azimuth_deg", 0), role + ".azimuth_deg", -36000, 36000),
                "elevation_deg": _number(light.get("elevation_deg", 30), role + ".elevation_deg", -90, 90),
                "angle_deg": _number(light.get("angle_deg", 0.53), role + ".angle_deg", 0, 180),
                "color": _color(light.get("color", [1, 1, 1]), role + ".color")})
        practicals = spec.get("practicals", [])
        lumens = native.get("practical_lumens", {})
        if not isinstance(practicals, list) or not isinstance(lumens, dict):
            raise ValueError("practicals must be a list; practical_lumens must be an object")
        practical_names = set()
        for light in practicals:
            if not isinstance(light, dict):
                raise ValueError("practical must be an object")
            light_name = light.get("name")
            if not isinstance(light_name, str) or not light_name or light_name in practical_names:
                raise ValueError("Practical names must be nonempty and unique")
            practical_names.add(light_name)
            kind = light.get("type", "POINT")
            if kind not in ("AREA", "POINT", "SPOT"):
                raise ValueError("Unknown practical light type")
            intensity = None
            if light_name in lumens:
                intensity = _number(lumens[light_name], light_name + " lumens", 0, 1e8)
            else:
                plan["preview_requirements"].append(f"lighting.{profile}.unreal.practical_lumens.{light_name}")
            position = blender_to_unreal_meters(light.get("location_m", [0, 0, 0]))
            target = blender_to_unreal_meters(light["target_m"]) if "target_m" in light else None
            if target is not None and target == position:
                raise ValueError("Practical target must differ from its position")
            output["practicals"].append({"name": light_name, "type": kind, "lumens": intensity,
                "location_cm": position, "target_cm": target,
                "color": _color(light.get("color", [1, 1, 1]), "practical.color"),
                "size_cm": 100 * _number(light.get("size_m", 0.2), "size_m", 0.001, 100),
                "spot_angle_deg": _number(light.get("spot_angle_deg", 60), "spot_angle_deg", 1, 178),
                "spot_blend": _number(light.get("spot_blend", 0.2), "spot_blend", 0, 1)})
        fog = spec.get("fog", {})
        if not isinstance(fog, dict):
            raise ValueError("fog must be an object")
        if fog.get("enabled", False):
            output["fog"] = {"density": setting("fog_density", 0, 1),
                "height_falloff": setting("fog_height_falloff", 0.001, 2),
                "extinction_scale": setting("fog_extinction_scale", 0, 100),
                "anisotropy": _number(fog.get("anisotropy", 0), "fog.anisotropy", -0.9, 0.9),
                "color": _color(fog.get("color", [1, 1, 1]), "fog.color"),
                "location_cm": blender_to_unreal_meters(fog.get("center_m", [0, 0, 0])),
                "view_distance_cm": _number(native.get("fog_view_distance_m", 80), "fog_view_distance_m", 1, 5000) * 100}
            plan["warnings"].append("Preview uses native exponential-height volumetric fog, not the Blender bounded-volume box or a density-per-meter conversion.")
        plan["warnings"].append("Sky preview uses Unreal SkyAtmosphere/captured SkyLight, not a pixel-identical Nishita/constant Blender world. Native intensity/exposure values are explicit overrides.")
        plan["lighting"] = output
    else:
        plan["preview_requirements"].append(f"lighting.{profile}")
    return plan


def _require(value, message):
    if not value:
        raise RuntimeError(message)


def apply_plan(plan, preview=False):
    """Editor-only mutation, restricted to our namespace and optional own map."""
    _require(plan["destination"].startswith(DESTINATION), "Refusing an external asset destination")
    _require(not plan.get("apply_requirements"), "Unbaked material data: " + "; ".join(plan["apply_requirements"]))
    if preview:
        _require(not plan["preview_requirements"], "Missing explicit Unreal preview settings: " + ", ".join(plan["preview_requirements"]))
    import unreal

    assets = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    library = unreal.MaterialEditingLibrary
    result = {"materials": [], "preview_map": None, "warnings": list(plan["warnings"]),
              "runtime_integrated": False}
    # Reject a collision before editing any existing material/texture.
    candidates = [item["asset_path"] for item in plan["materials"]]
    if preview:
        candidates.append(plan["destination"] + "/Maps/Preview_" + plan["profile"])
    for item in plan["materials"]:
        candidates.extend(plan["destination"] + "/Textures/T_" + item["asset_name"][2:] + "_" + role for role in item["textures"])
    for path in candidates:
        if assets.does_asset_exist(path):
            existing = assets.load_asset(path)
            _require(str(assets.get_metadata_tag(existing, "S47LookdevOwner")) == OWNER_TAG,
                     "Refusing to replace an unowned asset: " + path)

    def scalar(material, name, value):
        node = library.create_material_expression(material, unreal.MaterialExpressionScalarParameter)
        node.set_editor_property("parameter_name", name)
        node.set_editor_property("default_value", value)
        return node

    def vector(material, name, value):
        node = library.create_material_expression(material, unreal.MaterialExpressionVectorParameter)
        node.set_editor_property("parameter_name", name)
        node.set_editor_property("default_value", unreal.LinearColor(*value))
        return node

    def connect(node, output, prop):
        _require(library.connect_material_property(node, output, prop), "Material-property connection failed")

    def wire(a, output, b, socket):
        _require(library.connect_material_expressions(a, output, b, socket), "Material-node connection failed: " + socket)

    for item in plan["materials"]:
        path = item["asset_path"]
        material = assets.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset(
            item["asset_name"], path.rsplit("/", 1)[0], unreal.Material, unreal.MaterialFactoryNew())
        _require(isinstance(material, unreal.Material), "Cannot create lookdev material: " + path)
        assets.set_metadata_tag(material, "S47LookdevOwner", OWNER_TAG)
        for node in list(library.get_material_expressions(material)):
            library.delete_material_expression(material, node)
        _require(library.get_num_material_expressions(material) == 0, "Old material nodes remain")
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
        material.set_editor_property("two_sided", False)
        connect(vector(material, "BaseColor", item["base_color"]), "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
        connect(scalar(material, "Roughness", item["roughness"]), "", unreal.MaterialProperty.MP_ROUGHNESS)
        connect(scalar(material, "Metallic", item["metallic"]), "", unreal.MaterialProperty.MP_METALLIC)
        for role, filename in item["textures"].items():
            texture_name = "T_" + item["asset_name"][2:] + "_" + role
            task = unreal.AssetImportTask()
            task.set_editor_property("filename", filename)
            task.set_editor_property("destination_path", plan["destination"] + "/Textures")
            task.set_editor_property("destination_name", texture_name)
            task.set_editor_property("automated", True)
            task.set_editor_property("replace_existing", True)
            task.set_editor_property("save", False)
            tools.import_asset_tasks([task])
            texture = assets.load_asset(plan["destination"] + "/Textures/" + texture_name)
            _require(isinstance(texture, unreal.Texture2D), "Texture import failed: " + filename)
            assets.set_metadata_tag(texture, "S47LookdevOwner", OWNER_TAG)
            texture.set_editor_property("srgb", role == "base_color")
            texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP if role == "normal" else unreal.TextureCompressionSettings.TC_DEFAULT)
            if role == "normal":
                texture.set_editor_property("flip_green_channel", item["normal_convention"] == "opengl")
            node = library.create_material_expression(material, unreal.MaterialExpressionTextureSample)
            node.set_editor_property("texture", texture)
            node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role == "normal" else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if role == "base_color" else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
            connect(node, "R" if role in ("roughness", "metallic") else "RGB", {
                "base_color": unreal.MaterialProperty.MP_BASE_COLOR, "normal": unreal.MaterialProperty.MP_NORMAL,
                "roughness": unreal.MaterialProperty.MP_ROUGHNESS, "metallic": unreal.MaterialProperty.MP_METALLIC}[role])
            assets.save_loaded_asset(texture)
        if item["kind"] == "glass":
            glass = item["glass"]
            material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
            material.set_editor_property("two_sided", glass["mode"] == "pane")
            material.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
            material.set_editor_property("refraction_method", unreal.RefractionMode.RM_INDEX_OF_REFRACTION)
            # This preview uses a constant reference optical path. It does not
            # infer mesh thickness or reproduce Blender solid-glass transport.
            transmittance = math.exp(-glass["absorption_per_m"] * glass["thickness_m"])
            tint = [component * transmittance for component in glass["tint"][:3]] + [1.0]
            connect(vector(material, "GlassTint", tint), "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
            connect(scalar(material, "GlassRoughness", glass["roughness"]), "", unreal.MaterialProperty.MP_ROUGHNESS)
            connect(scalar(material, "GlassMetallic", 0), "", unreal.MaterialProperty.MP_METALLIC)
            f0 = ((glass["ior"] - 1) / (glass["ior"] + 1)) ** 2
            connect(scalar(material, "GlassSpecular", min(f0 / 0.08, 1)), "", unreal.MaterialProperty.MP_SPECULAR)
            fresnel = library.create_material_expression(material, unreal.MaterialExpressionFresnel)
            fresnel.set_editor_property("base_reflect_fraction", f0)
            opacity = library.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate)
            wire(scalar(material, "OpacityFace", 0.06), "", opacity, "A")
            wire(scalar(material, "OpacityEdge", 0.5), "", opacity, "B")
            wire(fresnel, "", opacity, "Alpha")
            connect(opacity, "", unreal.MaterialProperty.MP_OPACITY)
            # Thin panes avoid whole-screen IOR warping. Solid refraction is
            # still screen-space and needs visual review, not Blender parity.
            connect(scalar(material, "IOR", glass["ior"] if glass["mode"] == "solid" else 1), "", unreal.MaterialProperty.MP_REFRACTION)
        errors = list(library.recompile_material(material))
        _require(not errors, f"Material compile failed {path}: {errors}")
        assets.save_loaded_asset(material)
        result["materials"].append({"object": item["object"], "slot": item["slot"], "asset_path": path})
    if preview:
        result["preview_map"] = _preview(unreal, plan)
    return result


def _preview(unreal, plan):
    """Only create/update our own actors in an explicitly separate map."""
    assets = unreal.EditorAssetLibrary
    map_path = plan["destination"] + "/Maps/Preview_" + plan["profile"]
    maps = unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    _require(not maps, "Save or discard editor map changes before loading the lookdev preview")
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    loaded = levels.load_level(map_path) if assets.does_asset_exist(map_path) else levels.new_level(map_path)
    _require(loaded is not False, "Could not load/create isolated lookdev map")
    preview_world = assets.load_asset(map_path)
    _require(preview_world is not None, "Could not resolve isolated lookdev map asset")
    assets.set_metadata_tag(preview_world, "S47LookdevOwner", OWNER_TAG)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    existing = {actor.get_actor_label(): actor for actor in actors.get_all_level_actors()
                if OWNER_TAG in [str(tag) for tag in actor.tags]}
    wanted = set()

    def spawn(cls, role, position=(0, 0, 0), rotation=(0, 0, 0)):
        label = "S47_LOOKDEV_" + role
        wanted.add(label)
        actor = existing.get(label)
        if actor is not None and not isinstance(actor, cls):
            actors.destroy_actor(actor)
            actor = None
        if actor is None:
            actor = actors.spawn_actor_from_class(cls, unreal.Vector(*position), unreal.Rotator(*rotation))
        _require(actor is not None, "Could not spawn " + role)
        actor.set_actor_label(label)
        actor.set_actor_location(unreal.Vector(*position), False, False)
        actor.set_actor_rotation(unreal.Rotator(*rotation), False)
        actor.set_editor_property("tags", [unreal.Name(OWNER_TAG)])
        return actor

    light_spec = plan["lighting"]
    spawn(unreal.SkyAtmosphere, "Atmosphere")
    for index, row in enumerate(light_spec["directionals"]):
        # Light forward vector points down the incoming ray. Azimuth is
        # clockwise from Blender +Y north, which maps to Unreal +X.
        rotation = [-row["elevation_deg"], row["azimuth_deg"] + 180, 0]
        actor = spawn(unreal.DirectionalLight, row["role"], (0, 0, 500), rotation)
        component = actor.light_component
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_intensity(row["lux"])
        component.set_light_color(unreal.LinearColor(*row["color"]))
        component.set_atmosphere_sun_light(True)
        component.set_atmosphere_sun_light_index(index)
        component.set_light_source_angle(row["angle_deg"])
        component.set_volumetric_scattering_intensity(1)
    sky = spawn(unreal.SkyLight, "Sky", (0, 0, 300)).light_component
    sky.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.set_intensity(light_spec["sky_intensity"])
    sky.set_real_time_capture(True)
    for row in light_spec["practicals"]:
        rotation = [0, 0, 0]
        if row["target_cm"] is not None:
            offset = [b - a for a, b in zip(row["location_cm"], row["target_cm"])]
            rotation = [math.degrees(math.atan2(offset[2], math.hypot(offset[0], offset[1]))),
                        math.degrees(math.atan2(offset[1], offset[0])), 0]
        actor = spawn({"AREA": unreal.RectLight, "POINT": unreal.PointLight, "SPOT": unreal.SpotLight}[row["type"]],
                      "Practical_" + _slug(row["name"]), row["location_cm"], rotation)
        component = actor.light_component
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
        component.set_intensity(row["lumens"])
        component.set_light_color(unreal.LinearColor(*row["color"]))
        component.set_volumetric_scattering_intensity(1)
        if row["type"] == "AREA":
            component.set_editor_property("source_width", row["size_cm"])
            component.set_editor_property("source_height", row["size_cm"])
        if row["type"] == "SPOT":
            component.set_outer_cone_angle(row["spot_angle_deg"] / 2)
            component.set_inner_cone_angle(row["spot_angle_deg"] * (1 - row["spot_blend"]) / 2)
    if light_spec["fog"]:
        row = light_spec["fog"]
        fog_actor = spawn(unreal.ExponentialHeightFog, "Fog", row["location_cm"])
        fog = fog_actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
        fog.set_fog_density(row["density"])
        fog.set_fog_height_falloff(row["height_falloff"])
        fog.set_fog_inscattering_color(unreal.LinearColor(*row["color"]))
        fog.set_volumetric_fog(True)
        fog.set_volumetric_fog_scattering_distribution(row["anisotropy"])
        fog.set_volumetric_fog_extinction_scale(row["extinction_scale"])
        fog.set_volumetric_fog_distance(row["view_distance_cm"])
    exposure = spawn(unreal.PostProcessVolume, "Exposure")
    exposure.set_editor_property("unbound", True)
    settings = exposure.settings
    for key, value in (("override_auto_exposure_method", True),
                       ("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL),
                       ("override_auto_exposure_apply_physical_camera_exposure", True),
                       ("auto_exposure_apply_physical_camera_exposure", False),
                       ("override_auto_exposure_bias", True),
                       ("auto_exposure_bias", light_spec["exposure_bias"])):
        settings.set_editor_property(key, value)
    exposure.set_editor_property("settings", settings)
    for label, actor in existing.items():
        if label not in wanted:
            actors.destroy_actor(actor)
    unreal.EditorLoadingAndSavingUtils.save_current_level()
    return map_path


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", default=os.environ.get("S47_LOOKDEV_MANIFEST"))
    parser.add_argument("--profile", choices=("day", "night"), default=os.environ.get("S47_LOOKDEV_PROFILE", "day"))
    parser.add_argument("--apply", action="store_true", default=os.environ.get("S47_LOOKDEV_APPLY") == "1")
    parser.add_argument("--preview", action="store_true", default=os.environ.get("S47_LOOKDEV_PREVIEW") == "1")
    args = parser.parse_args(argv)
    if not args.manifest:
        parser.error("--manifest or S47_LOOKDEV_MANIFEST is required")
    path = Path(args.manifest).expanduser().resolve()
    plan = generate_plan(json.loads(path.read_text()), args.profile, path.parent)
    if args.preview and not args.apply:
        parser.error("--preview requires --apply")
    print(json.dumps(apply_plan(plan, args.preview) if args.apply else plan, indent=2))


if __name__ == "__main__":
    main()
