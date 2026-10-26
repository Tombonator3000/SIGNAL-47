"""Import only SC01's original authored geometry with the validated FBX recipe.

Run by the integration owner in UnrealEditor-Cmd, never concurrently with build,
editor, voice GPU work or a game test. Writes /Game/Signal47/HorrorSC01 only.
"""
from pathlib import Path
import hashlib
import json
import os
import sys
import traceback
import unreal

SOURCE = Path(__file__).resolve().parent
PROJECT = SOURCE.parents[1] / "Signal47"
sys.path.insert(0, str(PROJECT / "Tools/fieldintegration"))
import import_recipe as R

DEST = "/Game/Signal47/HorrorSC01"
RT_ONLY = os.environ.get("S47_SC01_RT_ONLY") == "1"
HAND = "SM_SC01_Hand"
R.REPORT = Path(os.environ.get("S47_SC01_IMPORT_REPORT", str(Path.home() / "scratch-bm/Signal47-sc01-20261026/native-import.json")))
R.report = {"status": "RUNNING", "engine": unreal.SystemLibrary.get_engine_version(),
            "source": str(SOURCE), "destination": DEST,
            "mode": "hand_rt_only" if RT_ONLY else "full_import", "checks": [], "meshes": []}
E, S = R.E, R.S


def hashes():
    return {str(p.relative_to(SOURCE)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(SOURCE.rglob("*")) if p.is_file() and p.suffix in (".blend", ".fbx", ".json")}


def unchanged_asset_hashes():
    root = PROJECT / "Content/Signal47/HorrorSC01"
    return {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(root.rglob("*.uasset")) if path.stem != HAND}


def hand_ray_tracing(mesh):
    # UE 5.8.4 UStaticMesh has no RayTracingMinLOD (that property is skeletal).
    # StaticMeshBuilder only builds a dedicated RT proxy for a Nanite-enabled
    # mesh. Keep the complete rendering/collision fallback and reduce only the
    # separately built ray_tracing_proxy_settings representation.
    options = unreal.StaticMeshReductionOptions()
    options.set_editor_property("auto_compute_lod_screen_size", False)
    lod0 = unreal.StaticMeshReductionSettings()
    lod0.set_editor_property("percent_triangles", 1.0)
    lod0.set_editor_property("screen_size", 1.0)
    options.set_editor_property("reduction_settings", [lod0])
    R.check(HAND + " exactly one unreduced rendering LOD", S.set_lods(mesh, options) == 1)

    target = unreal.NaniteFallbackTarget.PERCENT_TRIANGLES
    nanite = S.get_nanite_settings(mesh)
    nanite.set_editor_property("enabled", True)
    nanite.set_editor_property("keep_percent_triangles", 1.0)
    nanite.set_editor_property("trim_relative_error", 0.0)
    nanite.set_editor_property("generate_fallback", unreal.NaniteGenerateFallback.ENABLED)
    nanite.set_editor_property("fallback_target", target)
    nanite.set_editor_property("fallback_percent_triangles", 1.0)
    nanite.set_editor_property("fallback_relative_error", 0.0)
    proxy = mesh.get_editor_property("ray_tracing_proxy_settings")
    proxy.set_editor_property("enabled", True)
    proxy.set_editor_property("fallback_target", target)
    proxy.set_editor_property("fallback_percent_triangles", .25)
    proxy.set_editor_property("fallback_relative_error", 0.0)
    mesh.set_editor_property("ray_tracing_proxy_settings", proxy)
    mesh.set_editor_property("lod_for_collision", 0)
    S.set_nanite_settings(mesh, nanite, True)


def verify_hand_ray_tracing(mesh):
    nanite = S.get_nanite_settings(mesh)
    proxy = mesh.get_editor_property("ray_tracing_proxy_settings")
    target = unreal.NaniteFallbackTarget.PERCENT_TRIANGLES
    R.check(HAND + " unreduced Nanite source and rendering fallback",
            nanite.get_editor_property("enabled")
            and nanite.get_editor_property("keep_percent_triangles") == 1.0
            and nanite.get_editor_property("trim_relative_error") == 0.0
            and nanite.get_editor_property("generate_fallback") == unreal.NaniteGenerateFallback.ENABLED
            and nanite.get_editor_property("fallback_target") == target
            and nanite.get_editor_property("fallback_percent_triangles") == 1.0
            and nanite.get_editor_property("fallback_relative_error") == 0.0)
    R.check(HAND + " separate quarter-triangle hardware RT proxy",
            proxy.get_editor_property("enabled")
            and proxy.get_editor_property("fallback_target") == target
            and proxy.get_editor_property("fallback_percent_triangles") == .25
            and proxy.get_editor_property("fallback_relative_error") == 0.0)
    R.check(HAND + " full-detail collision LOD0", mesh.get_editor_property("lod_for_collision") == 0)
    R.check(HAND + " complete CPU geometry retained", mesh.get_editor_property("allow_cpu_access"))
    return {"nanite_enabled": True, "rendering_fallback_percent_triangles": 1.0,
            "rendering_lod_count": S.get_lod_count(mesh), "collision_lod": 0,
            "separate_rt_proxy_enabled": True, "rt_proxy_percent_triangles": .25,
            "rt_proxy_relative_error": 0.0,
            "rt_proxy_triangle_count": None,
            "measurement_limit": "Dedicated RT proxy triangle count is not exposed by editor Python. Native settings and full LOD0 are checked here; actual RT residency and pixels require the packaged run."}


try:
    R.flush()
    original = hashes()
    contract = json.loads((SOURCE / "model_contract.json").read_text())
    R.report["source_sha256"] = original
    preserved = unchanged_asset_hashes() if RT_ONLY else None
    if RT_ONLY:
        R.check("RT-only existing SC01 assets", bool(preserved))
        R.report["unchanged_asset_sha256"] = preserved
    mats = {}
    for row in json.loads((SOURCE / "materials.json").read_text()):
        mat = E.load_asset(DEST + "/Materials/" + row["name"]) if RT_ONLY else R.material(row, DEST + "/Materials", SOURCE)
        R.check(row["name"] + " existing original material" if RT_ONLY else row["name"] + " original material", mat is not None)
        mats[row["name"]] = mat
    for name, row in contract["parts"].items():
        path = SOURCE / "Export" / (name + ".fbx")
        R.check(name + " exact original FBX", hashlib.sha256(path.read_bytes()).hexdigest() == contract["fbx_sha256"][path.name])
        mutate = not RT_ONLY or name == HAND
        mesh = R.import_mesh(path, DEST) if mutate else E.load_asset(DEST + "/" + name)
        R.check(name + " native original mesh", isinstance(mesh, unreal.StaticMesh))
        slots = []
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            matname = str(slot.material_slot_name)
            R.check(name + " authored slot " + matname, matname in mats)
            slots.append(matname)
            if mutate:
                mesh.set_material(index, mats[matname])
        R.check(name + " exact original material slots", slots == row["materials"], slots)
        if mutate:
            mesh.set_editor_property("support_ray_tracing", True)
            mesh.set_editor_property("allow_cpu_access", True)
            if name == HAND:
                hand_ray_tracing(mesh)
            else:
                settings = mesh.get_editor_property("nanite_settings")
                settings.enabled = False
                mesh.set_editor_property("nanite_settings", settings)
        body = mesh.get_editor_property("body_setup")
        if body and mutate:
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        R.check(name + " full-detail complex collision", body is not None and body.get_editor_property("collision_trace_flag") == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        if mutate:
            for lod in range(S.get_lod_count(mesh)):
                build = S.get_lod_build_settings(mesh, lod)
                build.set_editor_property("use_full_precision_u_vs", True)
                S.set_lod_build_settings(mesh, lod, build)
            R.check(name + " saved original", E.save_loaded_asset(mesh))
        R.check(name + " hardware RT retained", mesh.get_editor_property("support_ray_tracing"))
        triangles = [mesh.get_num_triangles(lod) for lod in range(S.get_lod_count(mesh))]
        R.check(name + " single full-detail rendering LOD", len(triangles) == 1, triangles)
        R.check(name + " exact authored triangle count", mesh.get_num_triangles(0) == row["triangles"], mesh.get_num_triangles(0))
        bounds = mesh.get_bounds()
        center = [bounds.origin.x, bounds.origin.y, bounds.origin.z]
        size = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
        low, high = row["bounds_local_m"]
        expected_center = [(low[0]+high[0])*50, -(low[1]+high[1])*50, (low[2]+high[2])*50]
        expected_size = [(high[k]-low[k])*100 for k in range(3)]
        R.check(name + " metric FBX center and axes", max(abs(a-b) for a,b in zip(center,expected_center)) < .35, center)
        R.check(name + " metric FBX dimensions", max(abs(a-b) for a,b in zip(size,expected_size)) < .35, size)
        item = {"name": name, "asset": mesh.get_path_name(), "triangles": mesh.get_num_triangles(0),
                "rendering_lod_triangles": triangles, "material_slots": slots,
                "center_cm": center, "size_cm": size, "mutated": mutate}
        if name == HAND:
            item["native_rt_profile"] = verify_hand_ray_tracing(mesh)
        R.report["meshes"].append(item)
    if RT_ONLY:
        R.check("other meshes and all original materials byte-preserved", unchanged_asset_hashes() == preserved)
    R.check("source geometry unchanged", hashes() == original)
    R.report["status"] = "PASS"
    R.flush()
    unreal.log("S47 SC01 ORIGINAL NATIVE IMPORT PASS")
except Exception:
    R.report["status"] = "FAIL"
    R.report["error"] = traceback.format_exc()
    R.flush()
    raise
