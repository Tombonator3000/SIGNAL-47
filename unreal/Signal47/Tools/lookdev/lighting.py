"""Editable, bounded Blender lighting rigs for material/Tripo look development.

No game world is changed. The caller saves a new .blend rather than its input.
Coordinates are metres, Z up; azimuth is clockwise from +Y (north). Use a metre
scene (unit_settings.scale_length=1). Blender SUN energy is W/m² and local light energy is W; neither value is an Unreal lux value.
"""
from __future__ import annotations

import copy
import math

import bpy
from mathutils import Vector


OWNER = "s47_lookdev_lighting"
COLLECTION = "S47_LOOKDEV_LIGHTING"


def _number(value, label, low=None, high=None):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise ValueError(f"{label} must be a finite number")
    value = float(value)
    if low is not None and value < low or high is not None and value > high:
        raise ValueError(f"{label} outside [{low}, {high}]")
    return value


def _vector(value, label, *, positive=False, color=False):
    if not isinstance(value, (list, tuple)) or len(value) != 3:
        raise ValueError(f"{label} must have three numbers")
    result = [_number(v, label, 0 if color else None, 1 if color else None) for v in value]
    if positive and any(v <= 0 for v in result):
        raise ValueError(f"{label} dimensions must be positive")
    return result


def sun_direction(azimuth_deg, elevation_deg):
    """Unit vector from surface toward the light, for both sky and SUN lamp."""
    azimuth, elevation = math.radians(azimuth_deg), math.radians(elevation_deg)
    return Vector((math.sin(azimuth) * math.cos(elevation),
                   math.cos(azimuth) * math.cos(elevation), math.sin(elevation)))


def _directional(raw, label):
    if not isinstance(raw, dict):
        raise ValueError(f"{label} must be an object")
    return dict(azimuth_deg=_number(raw.get("azimuth_deg", 135), label + ".azimuth_deg", 0, 360),
                elevation_deg=_number(raw.get("elevation_deg", 35), label + ".elevation_deg", -90, 90),
                energy_w_m2=_number(raw.get("energy_w_m2", 2), label + ".energy_w_m2", 0, 1000),
                color=_vector(raw.get("color", [1, 1, 1]), label + ".color", color=True),
                angle_deg=_number(raw.get("angle_deg", .526), label + ".angle_deg", .01, 180))


def validate_lighting(spec, profile):
    """Return a normalized profile; no datablocks are modified on invalid input."""
    if profile not in ("day", "night"):
        raise ValueError("Lighting profile must be day or night")
    if not isinstance(spec, dict):
        raise ValueError("Lighting specification must be an object")
    profiles = spec.get("lighting")
    if not isinstance(profiles, dict) or not isinstance(profiles.get(profile), dict):
        raise ValueError(f"Missing lighting.{profile} object")
    raw = copy.deepcopy(profiles[profile])
    world = raw.get("world", {})
    if not isinstance(world, dict):
        raise ValueError("world must be an object")
    sky = world.get("sky", "nishita" if profile == "day" else "constant")
    if sky not in ("constant", "nishita"):
        raise ValueError("world.sky must be constant or nishita")
    if profile == "night" and sky != "constant":
        raise ValueError("Night uses a constant dim world, not a daylight sky model")
    result = {"profile": profile, "world": {
        "sky": sky,
        "strength": _number(world.get("strength", .2 if profile == "day" else .025), "world.strength", 0, 100),
        "color": _vector(world.get("color", [.025, .04, .08]), "world.color", color=True),
        "air_density": _number(world.get("air_density", 1), "world.air_density", 0, 10),
        "dust_density": _number(world.get("dust_density", 1), "world.dust_density", 0, 10),
        "ozone_density": _number(world.get("ozone_density", 1), "world.ozone_density", 0, 10),
    }, "exposure": _number(raw.get("exposure", 0), "exposure", -16, 16)}
    for key in ("sun", "moon"):
        if key in raw:
            result[key] = _directional(raw[key], key)
    if sky == "nishita" and "sun" not in result:
        raise ValueError("Day Nishita sky requires an explicit sun direction")
    mute = raw.get("mute_existing_lights", False)
    if not isinstance(mute, bool):
        raise ValueError("mute_existing_lights must be boolean")
    result["mute_existing_lights"] = mute
    practicals = raw.get("practicals", [])
    if not isinstance(practicals, list):
        raise ValueError("practicals must be a list")
    result["practicals"] = []
    names = set()
    for i, light in enumerate(practicals):
        label = f"practicals[{i}]"
        if not isinstance(light, dict):
            raise ValueError(f"{label} must be an object")
        name = light.get("name")
        if (not isinstance(name, str) or not name.strip() or len(name) > 50 or name in names
                or name in {"Sun", "Moon", "Fog"}):
            raise ValueError("Practical names must be nonempty, unique, <=50 characters and not Sun/Moon/Fog")
        names.add(name)
        kind = light.get("type", "AREA")
        if kind not in ("AREA", "POINT", "SPOT"):
            raise ValueError(f"{label}.type must be AREA, POINT or SPOT")
        location = _vector(light.get("location_m"), label + ".location_m")
        target = light.get("target_m")
        if target is not None:
            target = _vector(target, label + ".target_m")
            if (Vector(target) - Vector(location)).length < .0001:
                raise ValueError(f"{label}.target_m coincides with light location")
        result["practicals"].append(dict(
            name=name, type=kind, location_m=location, target_m=target,
            energy_w=_number(light.get("energy_w", 40), label + ".energy_w", 0, 100000),
            color=_vector(light.get("color", [1, .65, .35]), label + ".color", color=True),
            size_m=_number(light.get("size_m", .3), label + ".size_m", .001, 100),
            spot_angle_deg=_number(light.get("spot_angle_deg", 55), label + ".spot_angle_deg", 1, 179),
            spot_blend=_number(light.get("spot_blend", .5), label + ".spot_blend", 0, 1)))
    fog = raw.get("fog", {"enabled": False})
    if not isinstance(fog, dict) or not isinstance(fog.get("enabled", False), bool):
        raise ValueError("fog must be an object with a boolean enabled field")
    result["fog"] = dict(enabled=fog.get("enabled", False))
    if result["fog"]["enabled"]:
        result["fog"].update(
            center_m=_vector(fog.get("center_m", [0, 0, 1.5]), "fog.center_m"),
            size_m=_vector(fog.get("size_m", [6, 6, 3]), "fog.size_m", positive=True),
            density_per_m=_number(fog.get("density_per_m", .01), "fog.density_per_m", 0, 2),
            absorption_per_m=_number(fog.get("absorption_per_m", 0), "fog.absorption_per_m", 0, 2),
            anisotropy=_number(fog.get("anisotropy", .25), "fog.anisotropy", -.95, .95),
            color=_vector(fog.get("color", [.8, .85, .9]), "fog.color", color=True))
    return result


def _owned(datablock):
    return datablock.get(OWNER, False)


def _collection(scene):
    collection = bpy.data.collections.get(COLLECTION)
    if collection is not None and not _owned(collection):
        raise ValueError(f"Reserved collection {COLLECTION} already belongs to the input")
    if collection is None:
        collection = bpy.data.collections.new(COLLECTION)
        collection[OWNER] = True
    if collection.name not in scene.collection.children:
        scene.collection.children.link(collection)
    return collection


def _world(profile, data):
    name = "S47_LOOKDEV_" + profile.upper() + "_WORLD"
    world = bpy.data.worlds.get(name)
    if world is not None and not _owned(world):
        raise ValueError(f"Reserved world {name} already belongs to the input")
    if world is None:
        world = bpy.data.worlds.new(name)
    world[OWNER] = True
    world.use_nodes = True
    tree = world.node_tree
    tree.nodes.clear()
    out = tree.nodes.new("ShaderNodeOutputWorld")
    out.name = "S47 World Output"
    out.location = (420, 0)
    background = tree.nodes.new("ShaderNodeBackground")
    background.name = "S47 Sky Illumination"
    background.location = (120, 0)
    background.inputs["Strength"].default_value = data["world"]["strength"]
    background.inputs["Color"].default_value = (*data["world"]["color"], 1)
    tree.links.new(background.outputs["Background"], out.inputs["Surface"])
    if data["world"]["sky"] == "nishita":
        sky = tree.nodes.new("ShaderNodeTexSky")
        sky.name = "S47 Physical Day Sky"
        sky.location = (-220, 0)
        sky_types = sky.bl_rna.properties["sky_type"].enum_items.keys()
        preferred = next((kind for kind in ("NISHITA", "MULTIPLE_SCATTERING", "SINGLE_SCATTERING")
                          if kind in sky_types), None)
        if preferred is None:
            raise RuntimeError("This Blender has no supported Nishita sky; choose world.sky=constant")
        sky.sky_type = preferred
        for key, value in (("sun_elevation", math.radians(data["sun"]["elevation_deg"])),
                           # Blender reverses this internally; a positive rotation is clockwise from +Y.
                           ("sun_rotation", math.radians(data["sun"]["azimuth_deg"])),
                           ("air_density", data["world"]["air_density"]),
                           ("aerosol_density" if hasattr(sky, "aerosol_density") else "dust_density", data["world"]["dust_density"]),
                           ("ozone_density", data["world"]["ozone_density"])):
            if not hasattr(sky, key):
                raise RuntimeError(f"Sky model lacks {key}; use supported Blender or a constant world")
            setattr(sky, key, value)
        # The separate SUN supplies the direct light once; no double-strength sun disc.
        if hasattr(sky, "sun_disc"):
            sky.sun_disc = False
        tree.links.new(sky.outputs["Color"], background.inputs["Color"])
    return world


def _light(collection, profile, name, kind):
    fullname = f"S47_LOOKDEV_{profile.upper()}_{name}"
    obj = bpy.data.objects.get(fullname)
    if obj is not None:
        if not _owned(obj) or obj.type != "LIGHT":
            raise ValueError(f"Reserved object {fullname} already belongs to the input")
        if obj.data.type != kind:
            old = obj.data
            obj.data = bpy.data.lights.new(fullname, kind)
            if old.users == 0:
                bpy.data.lights.remove(old)
    else:
        obj = bpy.data.objects.new(fullname, bpy.data.lights.new(fullname, kind))
        collection.objects.link(obj)
    if obj.data.users > 1:
        obj.data = obj.data.copy()
    obj.data[OWNER] = True
    obj[OWNER] = True
    obj["s47_lookdev_profile"] = profile
    obj["s47_lookdev_export"] = False
    obj.hide_render = False
    obj.hide_viewport = False
    obj.data.use_shadow = True
    return obj


def _fog(collection, profile, data, metres_per_unit):
    name = f"S47_LOOKDEV_{profile.upper()}_Fog"
    obj = bpy.data.objects.get(name)
    if obj is not None and (not _owned(obj) or obj.type != "MESH"):
        raise ValueError(f"Reserved object {name} already belongs to the input")
    if obj is None:
        # Make a closed box directly: no selection changes or default cube side effects.
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata([(-.5, -.5, -.5), (.5, -.5, -.5), (.5, .5, -.5), (-.5, .5, -.5),
                          (-.5, -.5, .5), (.5, -.5, .5), (.5, .5, .5), (-.5, .5, .5)], [],
                         [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5),
                          (2, 3, 7, 6), (3, 0, 4, 7)])
        mesh.update()
        obj = bpy.data.objects.new(name, mesh)
        collection.objects.link(obj)
    obj[OWNER] = True
    obj["s47_lookdev_profile"] = profile
    obj["s47_lookdev_export"] = False
    obj.location = Vector(data["center_m"]) / metres_per_unit
    obj.scale = Vector(data["size_m"]) / metres_per_unit
    mat_name = name + "_Volume"
    material = bpy.data.materials.get(mat_name)
    if material is not None and not _owned(material):
        raise ValueError(f"Reserved volume material {mat_name} belongs to the input")
    if material is None:
        material = bpy.data.materials.new(mat_name)
    material[OWNER] = True
    material.use_nodes = True
    nodes, links = material.node_tree.nodes, material.node_tree.links
    nodes.clear()
    out = nodes.new("ShaderNodeOutputMaterial")
    out.location = (500, 40)
    scatter = nodes.new("ShaderNodeVolumeScatter")
    scatter.name = "S47 Scattering Per Metre"
    scatter.location = (0, 80)
    scatter.inputs["Color"].default_value = (*data["color"], 1)
    # Blender volume coefficients are per scene unit. Convert sigma [/m] * [m/unit].
    scatter.inputs["Density"].default_value = data["density_per_m"] * metres_per_unit
    scatter.inputs["Anisotropy"].default_value = data["anisotropy"]
    if data["absorption_per_m"] > 0:
        absorption = nodes.new("ShaderNodeVolumeAbsorption")
        absorption.name = "S47 Absorption Per Metre"
        absorption.location = (0, -160)
        # Blender coefficient is (1 - color) * density: black gives neutral absorption.
        absorption.inputs["Color"].default_value = (0, 0, 0, 1)
        absorption.inputs["Density"].default_value = data["absorption_per_m"] * metres_per_unit
        add = nodes.new("ShaderNodeAddShader")
        add.location = (280, 40)
        links.new(scatter.outputs[0], add.inputs[0])
        links.new(absorption.outputs[0], add.inputs[1])
        links.new(add.outputs[0], out.inputs["Volume"])
    else:
        links.new(scatter.outputs[0], out.inputs["Volume"])
    # Deliberately no Surface link: this box must not become an opaque room-sized mesh.
    if obj.data.users > 1:
        obj.data = obj.data.copy()
    obj.data.materials.clear()
    obj.data.materials.append(material)
    obj.display_type = "WIRE"
    obj.hide_render = False
    obj.hide_viewport = False
    return obj


def apply_lighting(spec: dict, profile: str) -> dict:
    """Apply one independent day/night rig; never delete input geometry or lights.

    Generated rigs stay editable. Switching profile hides the other managed rig.
    Existing input lights are muted only on explicit request and their flags are
    restored when the option is turned off. Exposure/world change on the output
    scene; the caller is responsible for saving separately from the source file.
    """
    data = validate_lighting(spec, profile)
    scene = bpy.context.scene
    metres_per_unit = float(scene.unit_settings.scale_length)
    if not math.isfinite(metres_per_unit) or not math.isclose(metres_per_unit, 1.0, rel_tol=1e-6):
        raise ValueError("Lookdev lighting requires metre scene units (scale_length=1); convert a copy explicitly first")
    # Refuse name collisions before changing the scene or hiding the old rig.
    reserved = [COLLECTION, f"S47_LOOKDEV_{profile.upper()}_WORLD"]
    for collection_name in reserved[:1]:
        item = bpy.data.collections.get(collection_name)
        if item is not None and not _owned(item):
            raise ValueError(f"Reserved collection {collection_name} belongs to input")
    world = bpy.data.worlds.get(reserved[1])
    if world is not None and not _owned(world):
        raise ValueError(f"Reserved world {reserved[1]} belongs to input")
    names = [(f"S47_LOOKDEV_{profile.upper()}_{key.title()}", "LIGHT")
             for key in ("sun", "moon") if key in data]
    names.extend((f"S47_LOOKDEV_{profile.upper()}_{item['name']}", "LIGHT")
                 for item in data["practicals"])
    if data["fog"]["enabled"]:
        names.append((f"S47_LOOKDEV_{profile.upper()}_Fog", "MESH"))
        material_name = f"S47_LOOKDEV_{profile.upper()}_Fog_Volume"
        material = bpy.data.materials.get(material_name)
        if material is not None and not _owned(material):
            raise ValueError(f"Reserved volume material {material_name} belongs to input")
    for name, object_type in names:
        obj = bpy.data.objects.get(name)
        if obj is not None and (not _owned(obj) or obj.type != object_type):
            raise ValueError(f"Reserved object {name} belongs to input or has the wrong type")
    collection = _collection(scene)
    if "s47_lookdev_original_world" not in scene:
        scene["s47_lookdev_original_world"] = scene.world.name if scene.world else ""
        scene["s47_lookdev_original_exposure"] = float(scene.view_settings.exposure)
    scene.world = _world(profile, data)
    for obj in collection.objects:
        if _owned(obj):
            obj.hide_render = True
            obj.hide_viewport = True
    managed = []
    for role in ("sun", "moon"):
        if role not in data:
            continue
        directional = data[role]
        obj = _light(collection, profile, role.title(), "SUN")
        obj.rotation_euler = (-sun_direction(directional["azimuth_deg"], directional["elevation_deg"])).to_track_quat("-Z", "Y").to_euler()
        obj.data.energy = directional["energy_w_m2"]
        obj.data.color = directional["color"]
        obj.data.angle = math.radians(directional["angle_deg"])
        managed.append(obj.name)
    for practical in data["practicals"]:
        obj = _light(collection, profile, practical["name"], practical["type"])
        obj.location = Vector(practical["location_m"]) / metres_per_unit
        obj.data.energy = practical["energy_w"]
        obj.data.color = practical["color"]
        if practical["target_m"] is not None:
            direction = Vector(practical["target_m"]) / metres_per_unit - obj.location
            obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
        else:
            obj.rotation_euler = (0, 0, 0)
        if practical["type"] == "AREA":
            obj.data.shape = "DISK"
            obj.data.size = practical["size_m"] / metres_per_unit
        else:
            obj.data.shadow_soft_size = practical["size_m"] / metres_per_unit
        if practical["type"] == "SPOT":
            obj.data.spot_size = math.radians(practical["spot_angle_deg"])
            obj.data.spot_blend = practical["spot_blend"]
        managed.append(obj.name)
    if data["fog"]["enabled"]:
        managed.append(_fog(collection, profile, data["fog"], metres_per_unit).name)
    for obj in scene.objects:
        if obj.type != "LIGHT" or _owned(obj):
            continue
        if data["mute_existing_lights"]:
            if "s47_lookdev_previous_hide_render" not in obj:
                obj["s47_lookdev_previous_hide_render"] = bool(obj.hide_render)
                obj["s47_lookdev_previous_hide_viewport"] = bool(obj.hide_viewport)
            obj.hide_render = True
            obj.hide_viewport = True
        elif "s47_lookdev_previous_hide_render" in obj:
            obj.hide_render = obj["s47_lookdev_previous_hide_render"]
            obj.hide_viewport = obj["s47_lookdev_previous_hide_viewport"]
            del obj["s47_lookdev_previous_hide_render"]
            del obj["s47_lookdev_previous_hide_viewport"]
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.exposure = data["exposure"]
    scene["s47_lookdev_profile"] = profile
    bpy.context.view_layer.update()
    return dict(profile=profile, normalized_settings=data, managed_objects=managed,
                world=scene.world.name, metres_per_unit=metres_per_unit,
                input_lights_muted=data["mute_existing_lights"],
                export_note="Lighting, world and volume nodes require native Unreal setup; do not export fog as static geometry.")
