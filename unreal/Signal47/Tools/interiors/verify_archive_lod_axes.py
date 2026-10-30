"""Read saved archive render positions and explicit source descriptions only.

Root runs this commandlet with -EnablePlugins=GeometryScripting. GeometryScript
copies RENDER_DATA into a transient DynamicMesh; it never edits the asset.
Default scope is FilingCabinets + WorkTable, three LODs each. No axis search,
source transforms, imports, saves or config writes occur. Output is diagnostic
Saved/archive-lod-axes.json, FAIL unless every actual render bound agrees.
"""
from pathlib import Path
import hashlib
import json
import math
import os
import traceback

import unreal


HERE = Path(__file__).resolve().parent
TOLERANCE_CM = .35  # Existing native archive metric acceptance, no new slack.


def require(ok, message, detail=None):
    if not ok:
        raise AssertionError(message, detail)


def expected_bounds(row):
    low, high = row['bounds_local_m']
    return [[100 * low[0], -100 * high[1], 100 * low[2]],
            [100 * high[0], -100 * low[1], 100 * high[2]]]


def require_render_api():
    required = ('GeometryScript_AssetUtils', 'GeometryScript_MeshQueries',
                'GeometryScriptCopyMeshFromAssetOptions', 'GeometryScriptMeshReadLOD',
                'GeometryScriptLODType', 'GeometryScriptOutcomePins', 'DynamicMesh')
    require(all(hasattr(unreal, name) for name in required),
            'GeometryScripting must be loaded using -EnablePlugins=GeometryScripting')


def measure_render_lod(mesh, lod, row):
    """Read the actual selected render buffer, not aggregate LOD0 asset bounds."""
    require_render_api()
    require(0 <= lod < mesh.get_num_lods(), 'Requested actual render LOD must exist', lod)
    options = unreal.GeometryScriptCopyMeshFromAssetOptions()
    # RenderData is already built. Never apply source/build transforms again.
    options.set_editor_property('apply_build_settings', False)
    options.set_editor_property('use_build_scale', False)
    options.set_editor_property('request_tangents', False)
    requested = unreal.GeometryScriptMeshReadLOD()
    requested.set_editor_property('lod_type', unreal.GeometryScriptLODType.RENDER_DATA)
    requested.set_editor_property('lod_index', lod)
    transient = unreal.DynamicMesh()
    result = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh_v2(
        mesh, transient, options, requested, True)
    require(isinstance(result, tuple) and len(result) == 2,
            'Expected UE5.8 reflected DynamicMesh and Outcome result', str(type(result)))
    dynamic, outcome = result
    require(dynamic == transient and outcome == unreal.GeometryScriptOutcomePins.SUCCESS,
            'Actual render buffer copy failed', str(outcome))
    bounds = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(dynamic)
    actual = [[float(getattr(v, key)) for key in ('x', 'y', 'z')] for v in (bounds.min, bounds.max)]
    expected = expected_bounds(row)
    finite = all(math.isfinite(value) for point in actual for value in point)
    error = max(abs(a - b) for aa, bb in zip(actual, expected) for a, b in zip(aa, bb))
    triangles = dynamic.get_triangle_count()
    result = {'lod': lod, 'read_mode': 'RENDER_DATA', 'actual_bounds_cm': actual,
              'expected_bounds_cm': expected, 'maximum_bounds_error_cm': error,
              'render_triangles': mesh.get_num_triangles(lod), 'copied_render_triangles': triangles,
              'expected_triangles': row['triangles'], 'render_uv_channels': mesh.get_num_tex_coords(lod),
              'pass': finite and error < TOLERANCE_CM and triangles == row['triangles'] ==
                      mesh.get_num_triangles(lod) and mesh.get_num_tex_coords(lod) >= 1}
    # Return failed measurements too so raw incorrect axes remain diagnosable.
    return result


def description_dump(mesh, lod):
    description = mesh.get_static_mesh_description(lod)
    if description is None:
        return {'is_none': True, 'triangles': None, 'faces': []}
    count, faces = description.get_triangle_count(), []
    for index in range(count * 4 + 64):
        triangle = unreal.TriangleID()
        require(triangle.import_text('(IDValue=' + str(index) + ')'), 'Reflected TriangleID parse failed')
        if not description.is_triangle_valid(triangle):
            continue
        corners = []
        for corner in range(3):
            instance = description.get_triangle_vertex_instance(triangle, corner)
            vertex = description.get_vertex_instance_vertex(instance)
            position = description.get_vertex_position(vertex)
            uv = description.get_vertex_instance_uv(instance, 0)
            corners.append([position.x, position.y, position.z, uv.x, uv.y])
        require(all(math.isfinite(v) for point in corners for v in point),
                'Nonfinite actual MeshDescription corner', index)
        faces.append({'id': index, 'corners_cm_uv0': corners})
        if len(faces) == count:
            break
    require(len(faces) == count and count > 0, 'Incomplete actual MeshDescription enumeration', count)
    points = [point for face in faces for point in face['corners_cm_uv0']]
    bounds = [[fn(point[k] for point in points) for k in range(3)] for fn in (min, max)]
    return {'is_none': False, 'triangles': count, 'actual_bounds_cm': bounds, 'faces': faces}


def main():
    project = Path(unreal.Paths.project_dir()).resolve()
    source = Path(os.environ.get('S47_ARCHIVE_SOURCE', str(HERE.parent / 'archiveprops'))).resolve()
    path = Path(unreal.Paths.project_saved_dir()) / 'archive-lod-axes.json'
    contract_path = source / 'model_contract.json'
    report = {'status': 'RUNNING', 'mode': 'READ_ONLY_SAVED_RENDER_DATA_AND_MESH_DESCRIPTIONS',
              'contract': str(contract_path), 'meshes': [], 'asset_mutation': False,
              'assembly_before_sha256': hashlib.sha256((project / 'Content/FieldModels/assembly.json').read_bytes()).hexdigest()}
    def save():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(report) + '\n')
    save()
    try:
        require_render_api()
        contract = json.loads(contract_path.read_text())
        report['contract_sha256'] = hashlib.sha256(contract_path.read_bytes()).hexdigest()
        for suffix in ('FilingCabinets', 'WorkTable'):
            name = 'SM_ArchiveProps_' + suffix
            mesh = unreal.load_asset('/Game/ArchiveProps/' + name)
            require(isinstance(mesh, unreal.StaticMesh), 'Actual saved StaticMesh missing', name)
            require(mesh.get_num_lods() == 3, 'Expected exactly three actual LODs', name)
            measured = {'name': name, 'asset': mesh.get_path_name(), 'lods': []}
            report['meshes'].append(measured)
            for lod in range(3):
                entry = measure_render_lod(mesh, lod, contract['lods'][str(lod)][name])
                entry['mesh_description'] = description_dump(mesh, lod)
                measured['lods'].append(entry)
            save()
        require(hashlib.sha256((project / 'Content/FieldModels/assembly.json').read_bytes()).hexdigest() ==
                report['assembly_before_sha256'], 'Shared assembly changed during read-only diagnosis')
        require(all(row['pass'] for mesh in report['meshes'] for row in mesh['lods']),
                'Actual render LOD axes/bounds/triangles differ from the existing source contract')
        report['status'] = 'PASS_SAVED_RENDER_LOD_AXES'
        save()
        unreal.log('S47 archive actual render LOD axes PASS: six buffers, no asset mutation')
    except Exception:
        report['status'] = 'FAIL'
        report['error'] = traceback.format_exc()
        save()
        raise


if __name__ == '__main__':
    main()
