"""Separate existing Jeep road-wheel faces without changing the source.

Run through the root-owned production lease. Default mode writes measured
JSON and SVG projections. --export writes a separate five-part derivative,
partitioning source faces exactly once and retaining original corner data on
the source's calculated triangle view. The source mesh is never triangulated.
Inspect the diagnostic before export, then verify turning in native Unreal.
No generated replacement tyres, source overwrite or automatic native import.

blender -b -t 2 --python split_jeep_wheels.py -- \
    --source /private/latest-cabin/classic_jeep --output /private/wheel-probe
"""
from pathlib import Path
import argparse
import collections
import hashlib
import json
import math
import shutil
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import prepare as P

BODY = 'SM_TripoJeep_InteriorBody'
ORDER = (('FL', -1, 1), ('FR', 1, 1), ('RL', -1, -1), ('RR', 1, -1))


def components(mesh, face_ids, weld_digits=None, include_faces=False):
    """Exact topology: deliberately do not weld UV seams or alter the mesh."""
    parent = {}
    def root(i):
        parent.setdefault(i, i)
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    faces = [mesh.polygons[i] for i in face_ids]
    canonical = {}
    if weld_digits is not None:
        positions = {}
        for f in faces:
            for v in f.vertices:
                key = tuple(round(c, weld_digits) for c in mesh.vertices[v].co)
                canonical[v] = positions.setdefault(key, v)
    def vertex(v): return canonical.get(v, v)
    for f in faces:
        a = root(vertex(f.vertices[0]))
        for v in f.vertices[1:]:
            b = root(vertex(v))
            if a != b: parent[b] = a
    groups = collections.defaultdict(list)
    for f in faces: groups[root(vertex(f.vertices[0]))].append(f.index)
    result = []
    for ids in groups.values():
        verts = {v for i in ids for v in mesh.polygons[i].vertices}
        points = [mesh.vertices[v].co for v in verts]
        result.append({'faces': len(ids), 'vertices': len(verts),
                       'bounds': [[min(p[k] for p in points) for k in range(3)],
                                  [max(p[k] for p in points) for k in range(3)]]})
        if include_faces: result[-1]['face_ids'] = ids
    return sorted(result, key=lambda r: r['faces'], reverse=True)


def write_projection(path, mesh, faces, selected, wheel, axes):
    """Flat technical projection, not a render or evidence of correct cutting."""
    x, cy, cz = wheel['center']; side = wheel['side']
    bounds = ((cy - .65, cy + .65), (-.05, 1.07)) if axes == (1, 2) else ((.35, 1.08), (cy - .65, cy + .65))
    def project(v):
        a = v[axes[0]] * (side if axes[0] == 0 else 1)
        b = v[axes[1]]
        return (24 + (a-bounds[0][0])/(bounds[0][1]-bounds[0][0])*752,
                576 - (b-bounds[1][0])/(bounds[1][1]-bounds[1][0])*552)
    lines = ['<svg xmlns="http://www.w3.org/2000/svg" width="800" height="600" viewBox="0 0 800 600">',
             '<rect width="800" height="600" fill="#fafafa"/>',
             '<text x="12" y="18" font-size="13">Candidate red; retained context grey. Projection only, cut NOT approved.</text>']
    for selected_pass in (False, True):
        for i in faces:
            if (i in selected) != selected_pass: continue
            f = mesh.polygons[i]
            pts = ' '.join('%.2f,%.2f' % project(mesh.vertices[j].co) for j in f.vertices)
            lines.append('<polygon points="%s" fill="%s" fill-opacity=".25" stroke="%s" stroke-width=".20"/>' %
                         (pts, '#df442a' if selected_pass else '#a0adb5', '#9d2b17' if selected_pass else '#66727a'))
    center = project((x, cy, cz))
    lines.append('<circle cx="%.2f" cy="%.2f" r="5" fill="none" stroke="blue" stroke-width="2"/>' % center)
    lines.append('</svg>'); path.write_text('\n'.join(lines))


def reference_wheels(mesh, fits):
    """Select complete source LOD1 wheels, never manufacture a round tyre."""
    from mathutils.bvhtree import BVHTree
    groups = components(mesh, range(len(mesh.polygons)), 5, True)
    result = {}; used = set()
    points = [v.co.copy() for v in mesh.vertices]
    for name, side, end in ORDER:
        matches = []
        for group in groups:
            lo, hi = group['bounds']; dim = [hi[i]-lo[i] for i in range(3)]
            if (group['faces'] > 500 and .30 < dim[0] < .42 and .85 < dim[1] < .96
                    and .83 < dim[2] < .94 and -.025 < lo[2] < .04
                    and side * (lo[0]+hi[0]) / 2 > .60
                    and end * (lo[1]+hi[1]) / 2 > 1.): matches.append(group)
        P.require(len(matches) == 1, f'{name}: need exactly one complete reference wheel, got {len(matches)}')
        group = matches[0]; ids = set(group['face_ids'])
        lo, hi = group['bounds']; pivot = [(lo[i]+hi[i])/2 for i in range(3)]
        # The scanned hubs/spokes partly meet the axle. Keep the axle inside
        # the measured tyre's inner sidewall on the body; include only source
        # faces wholly inside the existing wheel's central metal region.
        hub_ids = set()
        for f in mesh.polygons:
            vertices = [mesh.vertices[v].co for v in f.vertices]
            if all(lo[0]-.012 <= p.x <= hi[0]+.012 and
                   math.hypot(p.y-pivot[1], p.z-pivot[2]) < .29 for p in vertices): hub_ids.add(f.index)
        ids.update(hub_ids)
        P.require(not used.intersection(ids), 'Reference wheel sets overlap'); used.update(ids)
        result[name] = {'ids': ids, 'bounds': group['bounds'],
                        'pivot': pivot, 'hub_faces': len(hub_ids-set(group['face_ids'])),
                        'bvh': BVHTree.FromPolygons(points, [list(mesh.polygons[i].vertices) for i in ids], all_triangles=False)}
    body_ids = set(range(len(mesh.polygons))) - used
    # Cabin clipping may leave valid n-gons. BVH's temporary polygon support
    # must not triangulate or otherwise alter the retained source mesh.
    body = BVHTree.FromPolygons(points, [list(mesh.polygons[i].vertices) for i in body_ids], all_triangles=False)
    return result, body


def reference_selection(mesh, region, reference, body_reference):
    result = set(); max_distance = 0.
    for i in region:
        face = mesh.polygons[i]
        wheel_near = reference['bvh'].find_nearest(face.center)
        body_near = body_reference.find_nearest(face.center)
        if wheel_near[0] is not None and body_near[0] is not None:
            distance, other = wheel_near[3], body_near[3]
            if distance < .065 and distance < other:
                result.add(i); max_distance = max(max_distance, distance)
    return result, max_distance


def split_mesh(source, face_ids, name, pivot):
    """Copy the nonmutating source triangle view with exact corner provenance.

    Cabin clipping leaves n-gons. Each derivative triangle takes its vertex,
    UV, colour and normal from source.loop_triangles.loops; no FBX-side
    triangulation or new smoothing decision is left to the exporter.
    """
    import bpy
    from mathutils import Vector
    source.calc_loop_triangles()
    triangles = [t for t in source.loop_triangles if t.polygon_index in face_ids]
    faces = [source.polygons[t.polygon_index] for t in triangles]
    P.require({t.polygon_index for t in triangles} == set(face_ids), 'Source face has no triangle representation')
    vertices = sorted({v for t in triangles for v in t.vertices})
    remap = {old: new for new, old in enumerate(vertices)}
    loops = [i for t in triangles for i in t.loops]
    shift = Vector(pivot)
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([source.vertices[i].co-shift for i in vertices], [],
                     [[remap[i] for i in t.vertices] for t in triangles])
    for mat in source.materials: mesh.materials.append(mat)
    for old, new in zip(faces, mesh.polygons):
        new.material_index = old.material_index
        # Explicit split corner normals preserve both flat and smooth source
        # surfaces. Smooth flags let those supplied normals remain authoritative
        # on nonplanar n-gon triangles instead of recalculating face normals.
        new.use_smooth = True
    # Preserve original split-normal fans. Flat source polygons also form fan
    # boundaries, even if their boundary edges were not explicitly sharp.
    # New diagonals inside those n-gons remain smooth and receive the original
    # corner normals. Making every edge sharp is NOT equivalent: the readonly
    # r18c probe showed larger normal error for that shortcut.
    sharp = {tuple(sorted(e.vertices)) for e in source.edges if e.use_edge_sharp}
    sharp.update(tuple(sorted(e)) for f in source.polygons if not f.use_smooth for e in f.edge_keys)
    for edge in mesh.edges:
        edge.use_edge_sharp = tuple(sorted(vertices[i] for i in edge.vertices)) in sharp
    for old in source.uv_layers:
        layer = mesh.uv_layers.new(name=old.name)
        layer.data.foreach_set('uv', [x for i in loops for x in old.data[i].uv])
        layer.active_render = old.active_render
    for old in source.color_attributes:
        P.require(old.domain in ('POINT','CORNER'), 'Unsupported source colour domain')
        layer = mesh.color_attributes.new(name=old.name, type=old.data_type, domain=old.domain)
        indices = vertices if old.domain == 'POINT' else loops
        layer.data.foreach_set('color', [x for i in indices for x in old.data[i].color])
    if source.color_attributes:
        mesh.color_attributes.active_color_index = source.color_attributes.active_color_index
        mesh.color_attributes.render_color_index = source.color_attributes.render_color_index
    mesh.update()
    normals = [tuple(source.corner_normals[i].vector) for i in loops]
    mesh.normals_split_custom_set(normals)
    obj = bpy.data.objects.new(name, mesh); bpy.context.scene.collection.objects.link(obj)
    position_error = max((mesh.vertices[new].co+shift-source.vertices[old].co).length for old, new in remap.items())
    uv_error = max((mesh.uv_layers[layer.name].data[new].uv-layer.data[old].uv).length
                   for layer in source.uv_layers for new, old in enumerate(loops))
    normal_error = max((mesh.corner_normals[new].vector-source.corner_normals[old].vector).length
                       for new, old in enumerate(loops))
    color_error = 0.
    for layer in source.color_attributes:
        indices = vertices if layer.domain == 'POINT' else loops
        copied = mesh.color_attributes[layer.name]
        color_error = max(color_error, max(abs(a-b) for new, old in enumerate(indices)
                                           for a,b in zip(copied.data[new].color,layer.data[old].color)))
    P.require(position_error < .000001 and uv_error < .000001 and normal_error < .001 and color_error < .000001,
              f'{name}: source corner data changed: {position_error}, {uv_error}, {normal_error}, {color_error}')
    P.require(all(len(f.vertices) == 3 for f in mesh.polygons), 'Derivative must be triangles before FBX')
    proof = {'source_faces': len(face_ids), 'derivative_triangles': len(triangles),
        'source_triangle_corner_ids_sha256': hashlib.sha256(','.join(map(str, loops)).encode()).hexdigest(),
        'source_face_ids_sha256': hashlib.sha256(
        ','.join(map(str, sorted(face_ids))).encode()).hexdigest(), 'position_error_m': position_error,
        'uv_error': uv_error, 'normal_error': normal_error, 'colour_error': color_error,
        'no_new_surface': True, 'triangulation': 'source.loop_triangles with original loop-corner data',
        'material_identity_preserved': True,
        'source_colour_layers': [a.name for a in source.color_attributes]}
    return obj, proof


def copy_private_file(source, dest):
    """Reflink small required PBR files; no asset-bank or source-project copy."""
    import fcntl
    dest.parent.mkdir(parents=True, exist_ok=True)
    with source.open('rb') as src, dest.open('wb') as dst:
        try: fcntl.ioctl(dst.fileno(), 0x40049409, src.fileno())
        except OSError: shutil.copyfileobj(src, dst)
    P.require(P.sha(source) == P.sha(dest), 'Private material copy differs')


def compact_part_materials(rows):
    """Remove only slots unused by every LOD, preserving source-name identity."""
    original_names = [m.name for m in rows[0]['object'].data.materials]
    P.require(len(original_names) == len(set(original_names)), 'Ambiguous duplicate source material names')
    used = set()
    for row in rows:
        mesh = row['object'].data
        P.require([m.name for m in mesh.materials] == original_names, 'Source LOD material order differs')
        used.update(f.material_index for f in mesh.polygons)
    retained = [i for i in range(len(original_names)) if i in used]
    P.require(retained, 'Part has no used source materials')
    remap = {old: new for new, old in enumerate(retained)}
    provenance = [{'source_slot': i, 'material': name, 'derivative_slot': remap.get(i)}
                  for i, name in enumerate(original_names)]
    for row in rows:
        mesh = row['object'].data
        old_slots = [f.material_index for f in mesh.polygons]
        original_materials = list(mesh.materials)
        mesh.materials.clear()
        for index in retained: mesh.materials.append(original_materials[index])
        for face, old in zip(mesh.polygons, old_slots): face.material_index = remap[old]
        P.require(all(mesh.materials[f.material_index].name == original_names[old]
                      for f, old in zip(mesh.polygons, old_slots)), 'Compaction changed a source face material')
        row['proof']['material_slot_remap'] = provenance
        row['proof']['material_identity_preserved'] = True
        row['proof']['material_slots_compacted_by_all_lods'] = True


def export_split(source, output):
    import bpy
    source = source.resolve(); output = output.resolve()
    P.require(not output.is_relative_to(P.REPO), 'Output must stay private')
    P.require(source != output and not output.is_relative_to(source), 'Output must not touch source candidate')
    blend = source/'jeep-lookdev.blend'; protected = P.sha(blend)
    original = json.loads((source/'model_contract.json').read_text())
    fits = original['lookdev']['layout']['wheels']
    bpy.ops.wm.open_mainfile(filepath=str(blend))
    references, body_ref = reference_wheels(bpy.data.objects[BODY+'_LOD1'].data, fits)
    model = output/'classic_jeep'; (model/'exports').mkdir(parents=True, exist_ok=True)
    report = {'status': 'PREPARED_NATIVE_UNVERIFIED', 'source_blend': str(blend),
              'source_blend_sha256': protected, 'method': 'Original LOD1 wheel surfaces and hub faces classify source faces; no manufactured geometry',
              'source_body': BODY, 'body': 'SM_TripoJeep_WheelBody', 'wheels': [], 'lods': []}
    for name, _, _ in ORDER:
        reference = references[name]; lo, hi = reference['bounds']; pivot = reference['pivot']
        report['wheels'].append({'name': name, 'part': 'SM_TripoJeep_Wheel'+name,
            'pivot_blender_m': pivot, 'pivot_three_m': [pivot[0],pivot[2],-pivot[1]],
            'rolling_radius_m': (hi[2]-lo[2])/2, 'reference_bounds_blender_m': reference['bounds'],
            'mesh_pivot_m': [0,0,0], 'spin_axis_blender': [1,0,0], 'reference_hub_faces': reference['hub_faces']})
    parts = {}; built = {}
    for lod in range(3):
        suffix = '' if lod == 0 else '_LOD'+str(lod)
        obj = bpy.data.objects[BODY+suffix]; mesh = obj.data; mesh.update()
        selections = {}; taken = set(); distances = {}
        for name, side, _ in ORDER:
            reference = references[name]; cy = reference['pivot'][1]
            region = [f.index for f in mesh.polygons if side*f.center.x > .35 and
                      abs(f.center.y-cy) < .65 and f.center.z < 1.07]
            if lod == 1: ids, distance = reference['ids'], 0.
            else: ids, distance = reference_selection(mesh, region, reference, body_ref)
            P.require(len(ids) > 150 and distance < .035, f'{name} LOD{lod}: reference mismatch')
            P.require(not taken.intersection(ids), 'Wheel selections overlap')
            taken.update(ids); selections['SM_TripoJeep_Wheel'+name] = (ids,reference['pivot'])
            distances[name] = distance
        selections['SM_TripoJeep_WheelBody'] = (set(range(len(mesh.polygons)))-taken, [0,0,0])
        P.require(sum(len(ids) for ids, _ in selections.values()) == len(mesh.polygons), 'Incomplete source face partition')
        checks = []
        for name, (ids, pivot) in selections.items():
            new, proof = split_mesh(mesh, ids, name+suffix, pivot)
            proof = dict(part=name, **proof); checks.append(proof)
            built.setdefault(name, []).append({'object': new, 'lod': lod, 'pivot': pivot, 'proof': proof})
        obj.hide_render = True; obj.hide_set(True)
        report['lods'].append({'lod': lod, 'source_faces': len(mesh.polygons), 'partition_faces': sum(c['source_faces'] for c in checks),
                               'maximum_reference_distance_m': distances, 'checks': checks})
    # Native FBX imports discard unused slots. Establish a stable union from
    # all three actual LODs before measuring or exporting any part.
    for name, rows in built.items():
        compact_part_materials(rows)
        for row in rows:
            new = row['object']; lod = row['lod']
            measured = P.measurements(new); relative = 'exports/'+new.name+'.fbx'
            P.export_fbx(new, model/relative)
            parts.setdefault(name, dict(measured, asset_folder='/Game/Licensed/TripoFleet/classic_jeep', lods=[]))['lods'].append(
                dict(measured, lod=lod, fbx=relative))
            new.location = row['pivot']; new.hide_render = lod > 0; new.hide_set(lod > 0)
    materials = json.loads((source/'materials.json').read_text())
    needed = {m for part in parts.values() for m in part['materials']}
    materials = [m for m in materials if m['name'] in needed]
    P.require({m['name'] for m in materials} == needed, 'Missing original material records')
    copied = set()
    for row in materials:
        files = list(row.get('textures',{}).values())+[row[k] for k in ('packed_metallic_roughness','occlusion') if row.get(k)]
        for relative in files:
            if relative in copied: continue
            src = (source/relative).resolve(); dest = (model/relative).resolve()
            P.require(src.is_relative_to(source) and dest.is_relative_to(model), 'Texture path escapes derivative')
            copy_private_file(src,dest); copied.add(relative)
    contract = {'version': 1, 'id': 'classic_jeep', 'name': 'SM_TripoJeep_WheelBody',
                'coordinates': P.COORDINATES, 'parts': parts, 'wheel_split': report}
    manifest = {'version': 1, 'coordinates': P.COORDINATES, 'models': [{
        'id': 'classic_jeep', 'name': 'SM_TripoJeep_WheelBody', 'output_dir': str(model),
        'contract': 'classic_jeep/model_contract.json', 'materials': 'classic_jeep/materials.json',
        'status': 'PREPARED_NATIVE_UNVERIFIED'}]}
    P.write_json(model/'materials.json', materials); P.write_json(model/'model_contract.json', contract)
    P.write_json(model/'wheel_anchors.json', report); P.write_json(output/'manifest.json', manifest)
    bpy.ops.wm.save_as_mainfile(filepath=str(model/'jeep-wheels.blend'))
    P.require(P.sha(blend) == protected, 'Original lookdev blend changed')
    print('S47_JEEP_WHEELS_PREPARED', str(output/'manifest.json'), flush=True)


def diagnose(source, output, strategy='volume'):
    import bpy
    source = source.resolve(); output = output.resolve()
    P.require(not output.is_relative_to(P.REPO), 'Diagnostic output must be private')
    P.require(source != output and not output.is_relative_to(source), 'Never write inside the source candidate')
    blend = source / 'jeep-lookdev.blend'
    P.require(blend.is_file(), 'Expected a prepared lookdev blend')
    protected = P.sha(blend)
    contract = json.loads((source / 'model_contract.json').read_text())
    fits = contract['lookdev']['layout']['wheels']
    output.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.open_mainfile(filepath=str(blend))
    references, body_reference = reference_wheels(bpy.data.objects[BODY+'_LOD1'].data, fits) if strategy == 'reference' else ({}, None)
    report = {'source': str(blend), 'source_sha256': protected, 'status': 'DIAGNOSTIC_ONLY',
              'coordinates': P.COORDINATES, 'runtime_order': [n for n, _, _ in ORDER], 'strategy': strategy,
              'references': {n: {'bounds': r['bounds'], 'pivot_blender_m': r['pivot'], 'faces': len(r['ids'])} for n, r in references.items()},
              'warning': 'Source fit X is outer sidewall, not axle pivot. Face masks require visual approval.', 'lods': []}
    for lod in range(3):
        obj = bpy.data.objects[BODY + ('' if lod == 0 else '_LOD' + str(lod))]
        mesh = obj.data; mesh.update()
        row = {'lod': lod, 'object': obj.name, 'triangles': len(mesh.polygons),
               'all_components': components(mesh, range(len(mesh.polygons)))[:12],
               'geometric_components_10um': components(mesh, range(len(mesh.polygons)), 5)[:20], 'wheels': []}
        for name, side, end in ORDER:
            wheel = next(w for w in fits if w['side'] == side and w['end'] == end)
            _, cy, cz = wheel['center']
            region = [f.index for f in mesh.polygons if side * f.center.x > .35 and
                      abs(f.center.y-cy) < .65 and f.center.z < 1.07]
            stats = []
            selected = set()
            for inner in (.55, .60, .65, .70):
                for margin in (-.02, 0., .02):
                    radius = cz + margin
                    ids = []
                    for i in region:
                        points = [mesh.vertices[v].co for v in mesh.polygons[i].vertices]
                        if all(side * p.x >= inner and math.hypot(p.y-cy, p.z-cz) <= radius for p in points): ids.append(i)
                    stats.append({'inner_side_x_m': inner, 'radius_m': radius, 'faces': len(ids),
                                  'components': components(mesh, ids)[:4]})
                    if inner == .60 and margin == 0.: selected = set(ids)
            reference_distance = None
            if strategy == 'reference':
                if lod == 1: selected = references[name]['ids']; reference_distance = 0.
                else: selected, reference_distance = reference_selection(mesh, region, references[name], body_reference)
            touching = collections.Counter()
            for f in mesh.polygons:
                for e in f.edge_keys: touching[tuple(sorted(e))] += int(f.index in selected)
            boundary = [e for e, n in touching.items() if n == 1]
            selected_vertices = {v for i in selected for v in mesh.polygons[i].vertices}
            points = [mesh.vertices[v].co for v in selected_vertices]
            wheel_row = {'name': name, 'fit': wheel, 'region_faces': len(region),
                         'candidate_faces': len(selected), 'candidate_boundary_edges': len(boundary),
                         'reference_max_distance_m': reference_distance,
                         'region_geometric_components_10um': components(mesh, region, 5)[:12],
                         'candidate_bounds': [[min(p[k] for p in points) for k in range(3)],
                                              [max(p[k] for p in points) for k in range(3)]] if points else None,
                         'threshold_probes': stats}
            row['wheels'].append(wheel_row)
            for axes, label in [((1, 2), 'side'), ((0, 1), 'plan')]:
                write_projection(output / f'{name}-LOD{lod}-{label}.svg', mesh, region, selected, wheel, axes)
        report['lods'].append(row)
    P.require(P.sha(blend) == protected, 'Source changed during diagnostic')
    P.write_json(output / 'diagnostic.json', report)
    print('S47_JEEP_WHEEL_DIAGNOSTIC', str(output / 'diagnostic.json'), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--strategy', choices=('volume','reference'), default='reference')
    parser.add_argument('--export', action='store_true', help='Export the validated source-face partition, no native import')
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else sys.argv[1:])
    if args.export:
        P.require(args.strategy == 'reference', 'Volume-mask export is deliberately unsupported')
        export_split(args.source, args.output)
    else: diagnose(args.source, args.output, args.strategy)


if __name__ == '__main__': main()
