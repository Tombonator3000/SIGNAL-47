"""Actual native render-LOD positions against the measured raw Blender FBX frame.

Imported by explicit editor jobs. No asset writes or engine process on import.
The binary FBX reader is the project's existing pure-Python implementation.
"""
import math
from pathlib import Path
import runpy

READ_FBX = runpy.run_path(str(Path(__file__).resolve().parents[1] /
                              'calichenature/verify_unreal.py'))['fbx_nodes']


def verify_lod(mesh, lod, filename, triangles, exact_render_sections=False):
    import unreal
    objects = next(n for n in READ_FBX(Path(filename)) if n['name'] == 'Objects')
    geometry = [n for n in objects['children'] if n['name'] == 'Geometry' and n['properties'][-1] == 'Mesh']
    assert len(geometry) == 1, (filename, 'one authored mesh')
    raw = next(n for n in geometry[0]['children'] if n['name'] == 'Vertices')['properties'][0]
    expected = [(raw[i]*100, -raw[i+1]*100, raw[i+2]*100) for i in range(0, len(raw), 3)]
    native_count = mesh.get_num_triangles(lod)
    assert native_count == triangles, (mesh.get_name(), lod, 'native render triangles',
                                       'actual', native_count, 'expected', triangles)
    if exact_render_sections:
        # UE's StaticMeshLODResourcesToDynamicMesh discards DuplicateTriangleID.
        # This reader uses the original render index buffer, retaining g663's
        # coincident source faces. Keep the same bounds/probes below.
        assert hasattr(unreal, 'ProceduralMeshLibrary'), 'Enable ProceduralMeshComponent for render readback'
        actual, index_count = [], 0
        for section in range(mesh.get_num_sections(lod)):
            vertices, indices, _, _, _ = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh,lod,section)
            actual.extend((p.x,p.y,p.z) for p in vertices)
            index_count += len(indices)
        assert index_count == triangles*3, (mesh.get_name(), lod, 'native render section indices',
                                            'actual',index_count//3,'expected',triangles)
        read_mode = 'RENDER_SECTION_INDEX_BUFFER'
    else:
        options = unreal.GeometryScriptCopyMeshFromAssetOptions()
        options.set_editor_property('apply_build_settings', False)
        options.set_editor_property('use_build_scale', False)
        options.set_editor_property('request_tangents', False)
        requested = unreal.GeometryScriptMeshReadLOD()
        requested.set_editor_property('lod_type', unreal.GeometryScriptLODType.RENDER_DATA)
        requested.set_editor_property('lod_index', lod)
        transient = unreal.DynamicMesh()
        dynamic, result = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(mesh, transient, options, requested)
        assert dynamic == transient and result == unreal.GeometryScriptOutcomePins.SUCCESS, (mesh.get_name(), lod)
        _, values, _ = unreal.GeometryScript_MeshQueries.get_all_vertex_positions(dynamic, True)
        actual = [(p.x, p.y, p.z) for p in unreal.GeometryScript_List.convert_vector_list_to_array(values)]
        dynamic_count = dynamic.get_triangle_count()
        assert dynamic_count == triangles, (mesh.get_name(), lod, 'DynamicMesh converted triangles',
                                            'actual',dynamic_count,'native',native_count,'expected',triangles)
        read_mode = 'RENDER_DATA'
    assert actual and all(math.isfinite(v) for p in actual for v in p)
    box = lambda points: [[fn(p[k] for p in points) for k in range(3)] for fn in (min, max)]
    bound_error = max(abs(a-b) for aa, bb in zip(box(actual), box(expected)) for a, b in zip(aa, bb))
    assert bound_error < .10, (mesh.get_name(), lod, 'native render bounds/axes', box(actual), box(expected))
    directions = [(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1),
                  (.37,.71,.59),(-.61,.23,.77),(.23,-.83,.41),(-.71,-.29,-.31)]
    errors = []
    for direction in directions:
        probe = max(expected, key=lambda p: sum(a*b for a,b in zip(p,direction)))
        errors.append(math.sqrt(min(sum((p[k]-probe[k])**2 for k in range(3)) for p in actual)))
    assert max(errors) < .10, (mesh.get_name(), lod, 'asymmetric actual-render probes', errors)
    return {'lod': lod, 'bounds_cm': box(actual), 'bounds_error_cm': bound_error,
            'maximum_probe_error_cm': max(errors), 'triangles': native_count, 'read_mode': read_mode}
