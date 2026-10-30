"""Privat observatør ved tilsiktet klesavslag. Ingen fitting eller kildesave.

Lagrer bare den nye factory/import-scenen i den ferske diagnostikkutmappen.
Rå worldmesh er den samme uevaluerte restgeometrien som R4s BVH bruker.
"""
import gzip
import hashlib
import json
from pathlib import Path


def signature(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def file_row(path):
    path = Path(path)
    data = path.read_bytes()
    return {'path': str(path), 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
            'sha1': hashlib.sha1(data).hexdigest()}


def matrix(value):
    return [[float(x) for x in row] for row in value]


def mesh_payload(obj):
    md = obj.data
    md.calc_loop_triangles()  # Read cache only; preserve actual polygons/winding too.
    normal_matrix = obj.matrix_world.to_3x3().inverted().transposed()
    groups = {g.index: g.name for g in obj.vertex_groups}
    return {'object_name': obj.name, 'mesh_name': md.name, 'matrix_world': matrix(obj.matrix_world),
            'parent': obj.parent.name if obj.parent else None,
            'parent_type': obj.parent_type, 'parent_bone': obj.parent_bone,
            'matrix_parent_inverse': matrix(obj.matrix_parent_inverse),
            'vertices_local': [list(v.co) for v in md.vertices],
            'vertices_world_m': [list(obj.matrix_world @ v.co) for v in md.vertices],
            'polygons': [{'vertices': list(p.vertices), 'loop_indices': list(p.loop_indices),
                          'material_index': p.material_index, 'smooth': bool(p.use_smooth),
                          'normal_local': list(p.normal)} for p in md.polygons],
            'triangles': [{'vertices': list(t.vertices), 'polygon_index': t.polygon_index}
                          for t in md.loop_triangles],
            'loops_vertex_index': [loop.vertex_index for loop in md.loops],
            'corner_normals_local': [list(n.vector) for n in md.corner_normals],
            'corner_normals_world': [list((normal_matrix @ n.vector).normalized()) for n in md.corner_normals],
            'normal_matrix_inverse_transpose_world': matrix(normal_matrix),
            'uv_layers': {uv.name: [list(loop.uv) for loop in uv.data] for uv in md.uv_layers},
            'materials': [m.name if m else None for m in md.materials],
            'weights': [[[groups[g.group], float(g.weight)] for g in v.groups] for v in md.vertices],
            'armature_modifiers': [{'name': m.name, 'object': m.object.name if m.object else None,
                                    'preserve_volume': bool(m.use_deform_preserve_volume)}
                                   for m in obj.modifiers if m.type == 'ARMATURE'],
            'scope': 'Actual raw/rest mesh and actual object world matrix, not an evaluated posed surface'}


def capture(bpy, output, report, snapshot, skeleton, caller):
    output = Path(output)
    partial = output / 'partial-failure.blend'
    geometry = output / 'actual-world-geometry.json.gz'
    receipt = output / 'failure-scene-receipt.json'
    if any(p.exists() for p in (partial, geometry, receipt)):
        raise RuntimeError('Tidligere diagnostikkfiler kan aldri erstattes')
    meshes = sorted((o for o in bpy.context.scene.objects if o.type == 'MESH'), key=lambda o: o.name)
    arms = sorted((o for o in bpy.context.scene.objects if o.type == 'ARMATURE'), key=lambda o: o.name)
    before_snapshots = {o.name: snapshot(o) for o in meshes}
    before_refs = {o.name: skeleton(o) for o in arms}
    # Preserve the failure scene first, even if later JSON extraction rejects
    # an unexpected data type. This is a fresh private path, never a source save.
    bpy.ops.wm.save_as_mainfile(filepath=str(partial))
    unchanged_after_save = ({o.name: snapshot(o) for o in meshes} == before_snapshots
                            and {o.name: skeleton(o) for o in arms} == before_refs)
    full = {'schema_version': 1, 'status': 'FAILED_R4_SCENE_ONLY_NOT_ACCEPTED',
            'original_failure': report.get('error'),
            'objects': [{'name': o.name, 'type': o.type, 'matrix_world': matrix(o.matrix_world),
                         'parent': o.parent.name if o.parent else None} for o in bpy.context.scene.objects],
            'meshes': {o.name: mesh_payload(o) for o in meshes}, 'armatures': before_refs,
            'source_reference_actual_before_gate': report.get('source_reference_actual_before_gate'),
            'r4_measured_queries': report.get('coat_yoke'),
            'pre_cap_snapshots': {'body': caller.get('body_snapshot'), 'parts': caller.get('preserved_parts')},
            'limits': ['Same failed R4 geometry only. No query/fitting/tolerance changed.',
                       'Body/MetaHuman derived mesh stays private. No successful source fit, native or appearance acceptance.']}
    # These are observer checks against already-existing per-run snapshots,
    # not setters or a fallback that changes the failed R4 seam decision.
    body = caller.get('body')
    body_unchanged = body is not None and caller.get('body_snapshot') == before_snapshots.get(body.name)
    previous_parts = caller.get('preserved_parts', {})
    parts_unchanged = bool(previous_parts) and all(before_snapshots.get(name) == value for name, value in previous_parts.items())
    full['observed_body_unchanged_since_pre_cap'] = body_unchanged
    full['observed_original_parts_unchanged_since_pre_cap'] = parts_unchanged
    full['actual_cap_mesh_objects'] = [o.name for o in meshes if o.name.startswith('Coat_measured_shoulder_cap_')]
    text = json.dumps(full, ensure_ascii=False, sort_keys=True, separators=(',', ':')).encode()
    with geometry.open('xb') as stream:
        stream.write(gzip.compress(text, mtime=0))
    # Saving this new private scene must not mutate any live source geometry/ref.
    unchanged = (unchanged_after_save
                 and {o.name: snapshot(o) for o in meshes} == before_snapshots
                 and {o.name: skeleton(o) for o in arms} == before_refs)
    state = {'schema_version': 1, 'status': 'CAPTURED_FAILURE_ONLY_NOT_ACCEPTED',
             'original_failure': report.get('error'), 'partial_blend': file_row(partial),
             'world_geometry': file_row(geometry), 'geometry_uncompressed_sha256': hashlib.sha256(text).hexdigest(),
             'actual_meshes': [{'name': o.name, 'vertices': len(o.data.vertices),
                               'triangles': len(o.data.loop_triangles)} for o in meshes],
             'actual_armatures': [{'name': o.name, 'bones': len(o.data.bones),
                                   'reference_sha256': signature(before_refs[o.name])} for o in arms],
             'capture_preserved_actual_geometry_and_ref': unchanged,
             'observed_body_unchanged_since_pre_cap': body_unchanged,
             'observed_original_parts_unchanged_since_pre_cap': parts_unchanged,
             'actual_cap_mesh_objects': full['actual_cap_mesh_objects'],
             'source_accepted': False, 'fit_accepted': False, 'native_accepted': False}
    if not unchanged:
        state['status'] = 'CAPTURED_FAILURE_WITH_OBSERVER_PRESERVATION_MISMATCH'
    with receipt.open('x', encoding='utf-8') as stream:
        json.dump(state, stream, ensure_ascii=False, indent=2)
        stream.write('\n')
    state['receipt'] = file_row(receipt)
    return state
