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
R.REPORT = Path(os.environ.get("S47_SC01_IMPORT_REPORT", str(Path.home() / "scratch-bm/Signal47-sc01-20261026/native-import.json")))
R.report = {"status": "RUNNING", "engine": unreal.SystemLibrary.get_engine_version(),
            "source": str(SOURCE), "destination": DEST, "checks": [], "meshes": []}
E, S = R.E, R.S


def hashes():
    return {str(p.relative_to(SOURCE)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(SOURCE.rglob("*")) if p.is_file() and p.suffix in (".blend", ".fbx", ".json")}


try:
    R.flush()
    original = hashes()
    contract = json.loads((SOURCE / "model_contract.json").read_text())
    R.report["source_sha256"] = original
    mats = {row["name"]: R.material(row, DEST + "/Materials", SOURCE)
            for row in json.loads((SOURCE / "materials.json").read_text())}
    for name, row in contract["parts"].items():
        path = SOURCE / "Export" / (name + ".fbx")
        R.check(name + " exact original FBX", hashlib.sha256(path.read_bytes()).hexdigest() == contract["fbx_sha256"][path.name])
        mesh = R.import_mesh(path, DEST)
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            matname = str(slot.material_slot_name)
            R.check(name + " authored slot " + matname, matname in mats)
            mesh.set_material(index, mats[matname])
        settings = mesh.get_editor_property("nanite_settings")
        settings.enabled = False
        mesh.set_editor_property("nanite_settings", settings)
        mesh.set_editor_property("support_ray_tracing", True)
        mesh.set_editor_property("allow_cpu_access", True)
        body = mesh.get_editor_property("body_setup")
        if body:
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        for lod in range(S.get_lod_count(mesh)):
            build = S.get_lod_build_settings(mesh, lod)
            build.set_editor_property("use_full_precision_u_vs", True)
            S.set_lod_build_settings(mesh, lod, build)
        R.check(name + " saved original", E.save_loaded_asset(mesh))
        R.check(name + " exact authored triangle count", mesh.get_num_triangles(0) == row["triangles"], mesh.get_num_triangles(0))
        bounds = mesh.get_bounds()
        center = [bounds.origin.x, bounds.origin.y, bounds.origin.z]
        size = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
        low, high = row["bounds_local_m"]
        expected_center = [(low[0]+high[0])*50, -(low[1]+high[1])*50, (low[2]+high[2])*50]
        expected_size = [(high[k]-low[k])*100 for k in range(3)]
        R.check(name + " metric FBX center and axes", max(abs(a-b) for a,b in zip(center,expected_center)) < .35, center)
        R.check(name + " metric FBX dimensions", max(abs(a-b) for a,b in zip(size,expected_size)) < .35, size)
        R.report["meshes"].append({"name": name, "asset": mesh.get_path_name(), "triangles": mesh.get_num_triangles(0), "center_cm": center, "size_cm": size})
    R.check("source geometry unchanged", hashes() == original)
    R.report["status"] = "PASS"
    R.flush()
    unreal.log("S47 SC01 ORIGINAL NATIVE IMPORT PASS")
except Exception:
    R.report["status"] = "FAIL"
    R.report["error"] = traceback.format_exc()
    R.flush()
    raise
