"""Import only SC02's original pale actor geometry with the validated FBX recipe.

Run by the integration owner in UnrealEditor-Cmd, never concurrently with build,
editor, voice GPU work or a game test. Writes /Game/Signal47/HorrorSC02 only.

Each of the six parts becomes one static mesh with two ordinary native LODs:
the authored LOD0 FBX, and the authored *_LOD1.fbx copied in as LOD1 with screen
size 0.04 (as SC01). No Nanite, no separate ray tracing proxy, no collision on
the mesh (gameplay owns a capsule). The temporary LOD1 import assets under
HorrorSC02/LODSource are deleted again after the copy.
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

DEST = "/Game/Signal47/HorrorSC02"
LOD_SOURCE = DEST + "/LODSource"
LOD_SCREEN_SIZES = (1.0, .04)
R.REPORT = Path(os.environ.get("S47_SC02_IMPORT_REPORT", str(Path.home() / "scratch-bm/Signal47-sc02/native-import.json")))
R.report = {"status": "RUNNING", "engine": unreal.SystemLibrary.get_engine_version(),
            "source": str(SOURCE), "destination": DEST, "checks": [], "meshes": []}
E, S, M = R.E, R.S, R.M


def hashes():
    return {str(p.relative_to(SOURCE)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(SOURCE.rglob("*")) if p.is_file() and p.suffix in (".blend", ".fbx", ".json")}


def pale_skin(row):
    """Recipe material, then base colour = Color parameter x vertex colour (dark eye sockets)."""
    mat = R.material(row, DEST + "/Materials", SOURCE)
    if not row.get("vertex_color_multiply"):
        return mat
    colors = [n for n in M.get_material_expressions(mat)
              if isinstance(n, unreal.MaterialExpressionVectorParameter)
              and str(n.get_editor_property("parameter_name")) == "Color"]
    R.check(row["name"] + " one authored Color parameter", len(colors) == 1, len(colors))
    vertex = R.expression(mat, unreal.MaterialExpressionVertexColor)
    multiply = R.expression(mat, unreal.MaterialExpressionMultiply)
    R.check(row["name"] + " colour into tint", M.connect_material_expressions(colors[0], "RGB", multiply, "A"))
    R.check(row["name"] + " vertex colour into tint", M.connect_material_expressions(vertex, "", multiply, "B"))
    R.check(row["name"] + " tinted base colour", M.connect_material_property(multiply, "", unreal.MaterialProperty.MP_BASE_COLOR))
    errors = list(M.recompile_material(mat))
    R.check(row["name"] + " tinted material compiles", not errors, errors)
    R.check(row["name"] + " tinted material saved", E.save_loaded_asset(mat))
    return mat


def two_lods(name, mesh, lod1_path, expected_lod1):
    # Ordinary rendering LOD0 without Nanite or a separate RT proxy, exactly as
    # the SC01 hand profile. set_lods writes the two screen sizes; the authored
    # reduced geometry then replaces the generated LOD1.
    nanite = S.get_nanite_settings(mesh)
    nanite.set_editor_property("enabled", False)
    proxy = mesh.get_editor_property("ray_tracing_proxy_settings")
    proxy.set_editor_property("enabled", False)
    mesh.set_editor_property("ray_tracing_proxy_settings", proxy)
    S.set_nanite_settings(mesh, nanite, True)
    options = unreal.StaticMeshReductionOptions()
    options.set_editor_property("auto_compute_lod_screen_size", False)
    settings = []
    for screen in LOD_SCREEN_SIZES:
        lod = unreal.StaticMeshReductionSettings()
        lod.set_editor_property("percent_triangles", 1.0)
        lod.set_editor_property("screen_size", screen)
        settings.append(lod)
    options.set_editor_property("reduction_settings", settings)
    R.check(name + " two LOD slots with requested screen sizes", S.set_lods(mesh, options) == 2)
    # set_lod_from_static_mesh crashes UE 5.8.4 here (signal 11); import_lod is the
    # project's proven path for authored LODs (cows, arrival entrance, field kit).
    R.check(name + " authored LOD1 imported", S.import_lod(mesh, 1, str(lod1_path)) == 1)
    R.check(name + " authored LOD1 triangles", mesh.get_num_triangles(1) == expected_lod1, mesh.get_num_triangles(1))


try:
    R.flush()
    original = hashes()
    contract = json.loads((SOURCE / "model_contract.json").read_text())
    R.report["source_sha256"] = original
    mats = {}
    for row in json.loads((SOURCE / "materials.json").read_text()):
        mat = pale_skin(row)
        R.check(row["name"] + " original material", mat is not None)
        mats[row["name"]] = mat
    for name, row in contract["parts"].items():
        path = SOURCE / "Export" / (name + ".fbx")
        lod1_path = SOURCE / "Export" / (name + "_LOD1.fbx")
        for fbx in (path, lod1_path):
            R.check(fbx.stem + " exact original FBX", hashlib.sha256(fbx.read_bytes()).hexdigest() == contract["fbx_sha256"][fbx.name])
        mesh = R.import_mesh(path, DEST)
        R.check(name + " native original mesh", isinstance(mesh, unreal.StaticMesh))
        slots = []
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            matname = str(slot.material_slot_name)
            R.check(name + " authored slot " + matname, matname in mats)
            slots.append(matname)
            mesh.set_material(index, mats[matname])
        R.check(name + " exact original material slots", slots == row["materials"], slots)
        mesh.set_editor_property("support_ray_tracing", True)
        two_lods(name, mesh, lod1_path, row["lod1_triangles"])
        body = mesh.get_editor_property("body_setup")
        if body:
            # No simple shapes are authored, so simple-as-complex leaves the mesh without collision.
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
        R.check(name + " no mesh collision", body is None or body.get_editor_property("collision_trace_flag") == unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
        for lod in range(S.get_lod_count(mesh)):
            build = S.get_lod_build_settings(mesh, lod)
            build.set_editor_property("use_full_precision_u_vs", True)
            S.set_lod_build_settings(mesh, lod, build)
        # Rebuilding after the build settings recomputes the sizes when the asset was
        # reimported over an existing one; pin them last.
        R.check(name + " explicit LOD screen sizes", S.set_lod_screen_sizes(mesh, list(LOD_SCREEN_SIZES)))
        R.check(name + " saved original", E.save_loaded_asset(mesh))
        nanite = S.get_nanite_settings(mesh)
        proxy = mesh.get_editor_property("ray_tracing_proxy_settings")
        R.check(name + " no Nanite and no separate RT proxy",
                not nanite.get_editor_property("enabled") and not proxy.get_editor_property("enabled"))
        R.check(name + " hardware RT retained", mesh.get_editor_property("support_ray_tracing"))
        triangles = [mesh.get_num_triangles(lod) for lod in range(S.get_lod_count(mesh))]
        R.check(name + " exact authored LOD triangle counts", triangles == [row["triangles"], row["lod1_triangles"]], triangles)
        screens = list(S.get_lod_screen_sizes(mesh))
        R.check(name + " LOD screen sizes 1.0 and 0.04",
                len(screens) == 2 and all(abs(a - b) < 1e-3 for a, b in zip(screens, LOD_SCREEN_SIZES)), screens)
        bounds = mesh.get_bounds()
        center = [bounds.origin.x, bounds.origin.y, bounds.origin.z]
        size = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
        low, high = row["bounds_local_m"]
        expected_center = [(low[0]+high[0])*50, -(low[1]+high[1])*50, (low[2]+high[2])*50]
        expected_size = [(high[k]-low[k])*100 for k in range(3)]
        R.check(name + " metric FBX center, joint origin and axes", max(abs(a-b) for a,b in zip(center,expected_center)) < .35, center)
        R.check(name + " metric FBX dimensions", max(abs(a-b) for a,b in zip(size,expected_size)) < .35, size)
        R.report["meshes"].append({"name": name, "asset": mesh.get_path_name(), "triangles": triangles[0],
                                   "rendering_lod_triangles": triangles, "lod_screen_sizes": screens,
                                   "material_slots": slots, "center_cm": center, "size_cm": size,
                                   "nanite_enabled": False, "separate_rt_proxy_enabled": False})
    R.check("no temporary LOD source assets left", not E.list_assets(LOD_SOURCE, True, False))
    if E.does_directory_exist(LOD_SOURCE):
        E.delete_directory(LOD_SOURCE)
    total = sum(m["triangles"] * (2 if contract["parts"][m["name"]]["mirrored_pair"] else 1) for m in R.report["meshes"])
    R.check("whole body LOD0 within 14000 triangles", total <= 14000, total)
    R.check("source geometry unchanged", hashes() == original)
    R.report["status"] = "PASS"
    R.flush()
    unreal.log("S47 SC02 ORIGINAL NATIVE IMPORT PASS")
except Exception:
    R.report["status"] = "FAIL"
    R.report["error"] = traceback.format_exc()
    R.flush()
    raise
