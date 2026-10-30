"""Import only the two original door frames, reusing current native Office materials.

Use the project's guarded, idle UE 5.8.4 commandlet and legacy FBX flag.
Add -S47DoorFramesVerifyOnly for a read-only check of already saved assets.
No material or texture is created, recompiled, or reimported by this script.
"""
from pathlib import Path
import hashlib
import json
import sys
import traceback
import unreal

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]
SOURCE = PROJECT.parent / "ArtSource/DoorSet"
DEST = "/Game/Signal47/Art/Doors"
OFFICE = "/Game/Signal47/Art/Office/Materials"
NAMES = {"SM_DoorFrame_Service", "SM_DoorFrame_Archive"}
VERIFY_ONLY = ("--verify-only" in sys.argv or
               "S47DoorFramesVerifyOnly" in unreal.SystemLibrary.get_command_line())
sys.path.insert(0, str(HERE.parent / "fieldintegration"))
import import_recipe as R

R.REPORT = Path(unreal.Paths.project_saved_dir()) / "doorframes-native-import.json"
R.report = {"status": "RUNNING", "mode": "verify_saved_only" if VERIFY_ONLY else "import_and_verify",
            "engine": unreal.SystemLibrary.get_engine_version(), "source": str(SOURCE),
            "asset_root": DEST, "office_material_root": OFFICE, "checks": [], "meshes": []}
R.flush()


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


try:
    contract_path = SOURCE / "model_contract.json"
    contract = json.loads(contract_path.read_text())
    R.check("Exact two authored frame assets", set(contract["parts"]) == NAMES)
    R.check("Current Office material destination", contract["office_material_root"] == OFFICE)
    R.check("Existing Office leaf is reused", contract["leaf_asset"] ==
            "/Game/Signal47/Art/Office/SM_Office_Door.SM_Office_Door")
    R.report["contract_sha256"] = sha(contract_path)
    # Resolve every dependency before touching either frame. A missing Office
    # material must never trigger placeholder creation or texture duplication.
    names = {name for row in contract["parts"].values() for name in row["materials"]}
    materials = {}
    for name in sorted(names):
        path = OFFICE + "/" + name
        material = R.E.load_asset(path)
        R.check(name + " existing Office material", isinstance(material, unreal.MaterialInterface), path)
        materials[name] = material
    leaf = R.E.load_asset(contract["leaf_asset"].split(".")[0])
    R.check("Existing authored door leaf available", isinstance(leaf, unreal.StaticMesh))
    for name, row in contract["parts"].items():
        path = SOURCE / "Export" / (name + ".fbx")
        R.check(name + " source export exists", path.is_file(), str(path))
        profile = contract["frames"][name]
        R.check(name + " actual leaf perimeter stop clearance",
                abs(profile["stop_contact_z_m"] -
                    profile["scaled_leaf_perimeter_rear_m"] - .002) < 1e-8)
        R.check(name + " separate floor/leaf threshold",
                profile["leaf_bottom_m"] > profile["threshold_top_m"] and
                profile["side_and_head_clearance_m"] > 0)
    for name, row in contract["parts"].items():
        mesh = R.E.load_asset(DEST + "/" + name) if VERIFY_ONLY else R.import_mesh(
            SOURCE / "Export" / (name + ".fbx"), DEST)
        R.check(name + " native StaticMesh", isinstance(mesh, unreal.StaticMesh))
        if not VERIFY_ONLY:
            for index, slot in enumerate(mesh.get_editor_property("static_materials")):
                slot_name = str(slot.material_slot_name)
                R.check(name + " original material slot " + slot_name, slot_name in materials)
                mesh.set_material(index, materials[slot_name])
            settings = []
            for ratio, screen in ((1., 1.), (.55, .25), (.25, .08)):
                setting = unreal.StaticMeshReductionSettings()
                setting.set_editor_property("percent_triangles", ratio)
                setting.set_editor_property("screen_size", screen)
                settings.append(setting)
            options = unreal.StaticMeshReductionOptions()
            options.set_editor_property("auto_compute_lod_screen_size", False)
            options.set_editor_property("reduction_settings", settings)
            R.check(name + " three native LODs", R.S.set_lods(mesh, options) == 3)
            nanite = mesh.get_editor_property("nanite_settings")
            nanite.enabled = False
            mesh.set_editor_property("nanite_settings", nanite)
            mesh.set_editor_property("support_ray_tracing", True)
            R.S.remove_collisions(mesh)
            body = mesh.get_editor_property("body_setup")
            R.check(name + " body setup", body is not None)
            default = body.get_editor_property("default_instance")
            default.set_editor_property("collision_profile_name", "NoCollision")
            default.set_editor_property("collision_enabled", unreal.CollisionEnabled.NO_COLLISION)
            body.set_editor_property("default_instance", default)
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
            for lod in range(3):
                build = R.S.get_lod_build_settings(mesh, lod)
                build.set_editor_property("use_full_precision_u_vs", True)
                build.set_editor_property("remove_degenerates", False)
                R.S.set_lod_build_settings(mesh, lod, build)
            R.check(name + " saved", R.E.save_loaded_asset(mesh))
        slots = mesh.get_editor_property("static_materials")
        slot_names = [str(slot.material_slot_name) for slot in slots]
        R.check(name + " exact current Office material bindings",
                set(slot_names) == set(row["materials"]) and all(
                    slot.material_interface and
                    slot.material_interface.get_path_name() == materials[str(slot.material_slot_name)].get_path_name()
                    for slot in slots))
        bounds = mesh.get_bounds()
        lo, hi = row["bounds_local_m"]
        center = [bounds.origin.x, bounds.origin.y, bounds.origin.z]
        size = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
        expected = [(lo[0]+hi[0])*50, -(lo[1]+hi[1])*50, (lo[2]+hi[2])*50]
        error = max([abs(a-b) for a, b in zip(center, expected)] +
                    [abs(size[k]-(hi[k]-lo[k])*100) for k in range(3)])
        R.check(name + " metre-to-centimetre origin and dimensions", error < .2,
                {"center_cm": center, "size_cm": size, "max_error_cm": error})
        triangles = [mesh.get_num_triangles(i) for i in range(mesh.get_num_lods())]
        R.check(name + " authored LOD0 and reduced LODs", len(triangles) == 3 and
                triangles[0] == row["triangles"] and triangles[0] > triangles[1] > triangles[2] > 0,
                triangles)
        default = mesh.get_editor_property("body_setup").get_editor_property("default_instance")
        R.check(name + " decorative collision disabled",
                default.get_editor_property("collision_enabled") == unreal.CollisionEnabled.NO_COLLISION and
                str(default.get_editor_property("collision_profile_name")) == "NoCollision" and
                R.S.get_simple_collision_count(mesh) == 0)
        R.check(name + " existing RT policy with Nanite off",
                mesh.get_editor_property("support_ray_tracing") and
                not mesh.get_editor_property("nanite_settings").enabled)
        R.report["meshes"].append({"asset": mesh.get_path_name(), "triangles": triangles,
            "materials": slot_names, "center_cm": center, "size_cm": size,
            "fbx_sha256": sha(SOURCE / "Export" / (name+".fbx"))})
    R.report["status"] = "PASS"
    R.flush()
    unreal.log("S47 DOOR FRAMES NATIVE IMPORT PASS")
except Exception:
    R.report["status"] = "FAIL"
    R.report["error"] = traceback.format_exc()
    R.flush()
    raise
