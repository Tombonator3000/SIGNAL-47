"""UnrealEditor-Cmd Python: the Freeform ground meshes and their material, /Game/FreeformTerrain.

S47_FF_ROOT = the folder with mesh/*.glb; S47_FF_REPORT = the JSON to write. Same import path as the
exported web geometry (a GLB by AssetImportTask), so the frame and pivot are those of g873 and g4.
Touches nothing outside /Game/FreeformTerrain. Run by run_import.py under the production lease.
"""
import json
import os
from pathlib import Path
import sys
import traceback

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_ground_relief as MAT

DEST = '/Game/FreeformTerrain'
E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
MESHES = {'SM_FF_Ground_Road': '/Game/Web/Geo/geo_007/StaticMeshes/g873.g873', 'SM_FF_Ground_Saro': '/Game/MotelV2/Trim/g4.g4'}
COLLISION = {'SM_FF_Ground_Road': 'SM_FF_Ground_Road_Col', 'SM_FF_Ground_Saro': 'SM_FF_Ground_Saro_Col'}
FALLBACK_SHARE = .04
SAMPLES = 2000


def bounds(mesh):
    b = mesh.get_bounding_box()
    return {'min': [b.min.x, b.min.y, b.min.z], 'max': [b.max.x, b.max.y, b.max.z]}


def import_one(root, name, mat, out):
    path = DEST + '/' + name
    if E.does_asset_exist(path):
        if E.get_metadata_tag(E.load_asset(path), 'S47Owner') != MAT.OWNER:
            raise RuntimeError('not owned: ' + path)
        if not E.delete_asset(path):
            raise RuntimeError('could not replace ' + path)
    work = DEST + '/_import_' + name
    if E.does_directory_exist(work):
        E.delete_directory(work)
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(root / 'mesh' / (name + '.glb')))
    task.set_editor_property('destination_path', work)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    T.import_asset_tasks([task])
    made = E.list_assets(work, recursive=True)
    out['imported_assets'] = [str(a) for a in made]
    source = None
    for a in made:
        loaded = E.load_asset(a)
        if isinstance(loaded, unreal.StaticMesh):
            source = a.split('.')[0]
    if source is None:
        raise RuntimeError('No static mesh from ' + name + ': ' + str(made))
    mesh = E.load_asset(source)
    # the slot is ours; the GLB's placeholder material goes with the import folder
    mesh.set_material(0, mat)
    if not E.rename_asset(source, path):
        raise RuntimeError('could not move mesh to ' + path)
    mesh = E.load_asset(path)
    E.delete_directory(work)
    return mesh


def configure_collision(mesh, out):
    """A plain mesh for traces only: never drawn, no Nanite, no ray tracing, no distance field."""
    b = S.get_lod_build_settings(mesh, 0)
    b.set_editor_property('generate_lightmap_u_vs', False)
    b.set_editor_property('remove_degenerates', False)
    b.set_editor_property('recompute_normals', False)
    b.set_editor_property('distance_field_resolution_scale', 0.)
    S.set_lod_build_settings(mesh, 0, b)
    mesh.set_editor_property('support_ray_tracing', False)
    n = mesh.get_editor_property('nanite_settings')
    n.set_editor_property('enabled', False)
    mesh.set_editor_property('nanite_settings', n)
    setup = mesh.get_editor_property('body_setup')
    if setup:
        setup.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    E.set_metadata_tag(mesh, 'S47Owner', MAT.OWNER)
    if not E.save_loaded_asset(mesh):
        raise RuntimeError('could not save ' + mesh.get_path_name())
    out['triangles_lod0'] = mesh.get_num_triangles(0)
    out['bounds_cm'] = bounds(mesh)


def configure(mesh, out, collision_mesh):
    # UV1 carries the sediment and stone: no generated lightmap UV may write over it
    b = S.get_lod_build_settings(mesh, 0)
    b.set_editor_property('generate_lightmap_u_vs', False)
    b.set_editor_property('use_full_precision_u_vs', True)
    b.set_editor_property('remove_degenerates', False)
    b.set_editor_property('recompute_normals', False)
    b.set_editor_property('distance_field_resolution_scale', .25)
    S.set_lod_build_settings(mesh, 0, b)
    mesh.set_editor_property('light_map_coordinate_index', 0)
    # Nanite, with a fallback of 4 %: ray tracing and complex collision use it (not 100 % as the old grounds)
    n = S.get_nanite_settings(mesh) if hasattr(S, 'get_nanite_settings') else mesh.get_editor_property('nanite_settings')
    n.set_editor_property('enabled', True)
    n.set_editor_property('generate_fallback', unreal.NaniteGenerateFallback.ENABLED)
    n.set_editor_property('fallback_target', unreal.NaniteFallbackTarget.PERCENT_TRIANGLES)
    n.set_editor_property('fallback_percent_triangles', FALLBACK_SHARE)
    n.set_editor_property('fallback_relative_error', 0.)
    mesh.set_editor_property('nanite_settings', n)
    mesh.set_editor_property('support_ray_tracing', True)
    # traces go to the plain mesh beside it: the 4 % fallback is metres out over 38 km2
    mesh.set_editor_property('complex_collision_mesh', collision_mesh)
    setup = mesh.get_editor_property('body_setup')
    if setup:
        setup.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    E.set_metadata_tag(mesh, 'S47Owner', MAT.OWNER)
    if not E.save_loaded_asset(mesh):
        raise RuntimeError('could not save ' + mesh.get_path_name())
    if mesh.get_editor_property('complex_collision_mesh') != collision_mesh:
        raise RuntimeError('complex collision mesh did not hold: ' + mesh.get_path_name())
    out['complex_collision_mesh'] = collision_mesh.get_path_name()
    actual = mesh.get_editor_property('nanite_settings')
    out['nanite'] = actual.enabled
    out['fallback_percent_triangles'] = actual.fallback_percent_triangles


def readback(mesh, out):
    out['uv_channels'] = S.get_num_uv_channels(mesh, 0) if hasattr(S, 'get_num_uv_channels') else None
    out['bounds_cm'] = bounds(mesh)
    out['slots'] = [str(s.get_editor_property('material_interface').get_path_name()) if s.get_editor_property('material_interface') else None for s in mesh.get_editor_property('static_materials')]
    out['lod_count'] = S.get_lod_count(mesh)
    out['fallback_triangles_lod0'] = mesh.get_num_triangles(0)
    b = mesh.get_lod_build_settings(0) if hasattr(mesh, 'get_lod_build_settings') else S.get_lod_build_settings(mesh, 0)
    out['generate_lightmap_uvs'] = b.get_editor_property('generate_lightmap_u_vs')
    desc = mesh.get_static_mesh_description(0)
    n = desc.get_vertex_count()
    t = desc.get_triangle_count()
    out['source_vertices'] = n
    out['source_triangles'] = t
    rows = []
    step = max(1, n // SAMPLES)
    for i in range(0, n, step):
        vid = unreal.VertexID(i)
        p = desc.get_vertex_position(vid)
        row = [p.x, p.y, p.z]
        try:
            instances = desc.get_vertex_vertex_instances(vid)
            if instances:
                row.append(list((desc.get_vertex_instance_uv(instances[0], 0).x, desc.get_vertex_instance_uv(instances[0], 0).y)))
                row.append(list((desc.get_vertex_instance_uv(instances[0], 1).x, desc.get_vertex_instance_uv(instances[0], 1).y)))
        except Exception as error:
            out['uv_read_error'] = str(error)
        rows.append(row)
    out['vertex_samples'] = rows
    out['has_vertex_colors'] = S.has_vertex_colors(mesh) if hasattr(S, 'has_vertex_colors') else None
    try:
        # the colours as the build holds them, by GeometryScript from the source model
        options = unreal.GeometryScriptCopyMeshFromAssetOptions()
        options.set_editor_property('apply_build_settings', False)
        options.set_editor_property('request_tangents', False)
        lod = unreal.GeometryScriptMeshReadLOD()
        lod.set_editor_property('lod_type', unreal.GeometryScriptLODType.SOURCE_MODEL)
        lod.set_editor_property('lod_index', 0)
        dynamic = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(mesh, unreal.DynamicMesh(), options, lod)[0]
        got = unreal.GeometryScript_VertexColors.get_mesh_per_vertex_colors(dynamic)
        colors = next(g for g in got if isinstance(g, unreal.GeometryScriptColorList))
        values = unreal.GeometryScript_List.convert_color_list_to_array(colors)
        step = max(1, len(values) // 1000)
        pick = [values[i] for i in range(0, len(values), step)]
        out['vertex_color_stats'] = {'n': len(values), 'mean': [sum(getattr(c, k) for c in pick) / len(pick) for k in 'rgb'], 'min': [min(getattr(c, k) for c in pick) for k in 'rgb'], 'max': [max(getattr(c, k) for c in pick) for k in 'rgb']}
    except Exception as error:
        out['vertex_color_read_error'] = str(error)


def main():
    root = Path(os.environ['S47_FF_ROOT'])
    path = Path(os.environ['S47_FF_REPORT'])
    report = {'status': 'RUNNING'}
    try:
        only = os.environ.get('S47_FF_ONLY', '')
        # (with S47_FF_ONLY the material stands as it is, and so does everything without that word in its name)
        mat = E.load_asset(MAT.DEST + '/' + MAT.NAME) if only else MAT.main(report)
        report['meshes'] = {}
        for name, old in MESHES.items():
            if only and only not in name:
                continue
            out = report['meshes'].setdefault(name, {})
            col_out = report['meshes'].setdefault(COLLISION[name], {})
            collision_mesh = import_one(root, COLLISION[name], mat, col_out)
            configure_collision(collision_mesh, col_out)
            mesh = import_one(root, name, mat, out)
            configure(mesh, out, collision_mesh)
            readback(mesh, out)
            ref = E.load_asset(old)
            if ref is not None:
                out['old_mesh'] = old
                out['old_bounds_cm'] = bounds(ref)
                out['old_triangles_lod0'] = ref.get_num_triangles(0)
            path.write_text(json.dumps(report, indent=1, default=str) + '\n')
        report['assets'] = [str(a) for a in E.list_assets(DEST, recursive=True)]
        report['status'] = 'PASS_FREEFORM_TERRAIN'
    except Exception:
        report['status'] = 'FAIL'
        report['error'] = traceback.format_exc()
    path.write_text(json.dumps(report, indent=1, default=str) + '\n')


if os.environ.get('S47_FF_APPLY') == '1':
    main()
