"""Private, bounded Coat_body yoke over actual immutable Body+Face skin.

Only own garment coordinates/topology are produced. The licensed skin is used
for measurements, never copied into garment faces or exported as clothing.
Finite face/ray samples are evidence, not a continuous surface proof.
"""
from pathlib import Path
import gzip
import hashlib
import json
import math
import os
import sys
sys.dont_write_bytecode = True
from surface_cpu import Surface, add, sub, mul, dot, cross, length, unit

HERE = Path(__file__).resolve().parent
ROOT = Path('/home/tom/scratch-bm/Signal47-mannequins-20261029')
POLICY = 'actual-combined-skin-local-coat-yoke-v1'
COAT = 'Coat_body_open_lower_front'
REF = '9b1eb0e830b3041e7d931692f06845d9c433b7265bce9b8c5ed09e2463eea113'
ATTR = ('RW_R4_Part', 'RW_R4_LocalVertex', 'RW_R4_LocalFace')
DIRECTIONS = [unit(x) for x in ((.317, 1, .233), (1, .413, .277), (-.421, .319, 1))]


def need(value, why):
    if not value:
        raise RuntimeError(why)


def signature(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def file_row(path):
    p = Path(path)
    need(p.is_absolute() and p.is_file() and not p.is_symlink() and p.resolve() == p,
         'Actual ordinary absolute immutable input required: '+str(p))
    data = p.read_bytes()
    return dict(path=str(p), bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                sha1=hashlib.sha1(data).hexdigest())


def pin(row):
    need(set(row) == {'path', 'bytes', 'sha256', 'sha1'} and row == file_row(row['path']),
         'Changed or incomplete input pin: '+str(row.get('path')))
    return Path(row['path'])


def load(path):
    path = Path(path)
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def write_new(path, value):
    with Path(path).open('x', encoding='utf-8') as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2)
        stream.write('\n')


def inputs_checked(cfg):
    spec = cfg['coat_yoke']
    need(spec['policy'] == POLICY and spec['minimum_clearance_m'] == .006
         and spec['outer_target_m'] == .012 and spec['source_skeleton_sha256'] == REF,
         'Explicit bounded R4 policy and unchanged fit thresholds required')
    inputs = load(pin(spec['inputs']))
    need(inputs['schema_version'] == 1 and inputs['status'] == 'PINNED_ACTUAL_SKIN_AND_OWN_SURFACE_ONLY'
         and inputs['policy'] == POLICY and inputs['source_fit_accepted'] is False
         and inputs['runtime_accepted'] is False, 'Measured inputs only; no self-declared garment acceptance')
    expected = {'recipe', 'native_rest', 'native_acceptance', 'frame_confirmation',
                'converted_skin', 'source_scene', 'capture_acceptance'}
    need(set(inputs['files']) == expected, 'Exact complete skin/source file inventory required')
    values = {key: load(pin(row)) for key, row in inputs['files'].items()}
    recipe, native, frame = (values[key] for key in ('recipe', 'native_rest', 'frame_confirmation'))
    native_accept, capture = values['native_acceptance'], values['capture_acceptance']
    need(native_accept['status'] == 'PASS_READONLY_REST_SURFACES_ONLY_NOT_FRAME_OR_GARMENT_FIT'
         and len(native_accept['checks']) == 12 and all(v is True for v in native_accept['checks'].values())
         and native_accept['native_report'] == inputs['files']['native_rest'], 'Actual independent native acceptance required')
    for key in ('native_report', 'runner_receipt', 'build_receipt', 'guard', 'log'):
        pin(native_accept[key])
    guard = load(native_accept['guard']['path'])
    need(guard['returncode'] == 0 and guard['semantic_success'] is True
         and guard['contention'] == [] and not guard.get('stop_reason'), 'Actual completed uncontended native reader required')
    need(capture['status'] == 'PASS_DIAGNOSTIC_CAPTURE_ONLY_SOURCE_STILL_FAIL'
         and len(capture['checks']) == 12 and all(x is True for x in capture['checks'].values()),
         'Actual unchanged failed-R3 source scene required; old failure stays a failure')
    need(frame['status'] == 'PASS_COMPLETE_NATIVE_SKIN_FRAME_ONLY_NOT_GARMENT_FIT'
         and all(x is True for x in frame['checks'].values()) and len(frame['checks']) == 6
         and frame['native_report'] == inputs['files']['native_rest']
         and frame['converted_skin'] == inputs['files']['converted_skin']
         and frame['source_scene'] == inputs['files']['source_scene'], 'Actual full mesh/frame/normal/face-skin parity required')
    need(native['status'] == 'PASS' and native['mode'] == 'rest-surface-diagnostic'
         and native['asset_save_called'] is False and native['runtime_accepted'] is False, 'Read-only native rest surfaces required')
    for row in native['rest_surface_provenance'].values():
        pin(row)
    body, face = native['rest_surfaces']['body'], native['rest_surfaces']['face']
    need(len(body['bones']) == 342 and len(face['bones']) == 875
         and body['reference_pose_sha1'] == body['copy_reference_pose_sha1'] == '93838fa5259ddca476c5e523526483746f038133'
         and face['reference_pose_sha1'] == 'cf72ce9fce569a34374d50c8359803b6a3a308a6'
         and len(body['positions_body_reference_copy_cm']) == 32334 and len(body['triangles']) == 60816,
         'Exact unchanged native references and actual geometry required')
    skin_sections = [s for s in face['sections'] if s['material'].endswith('/MI_Face_Skin_Baked_LOD1.MI_Face_Skin_Baked_LOD1')]
    need(len(skin_sections) == 1 and skin_sections[0]['section'] == 0
         and skin_sections[0]['native_material_index'] == 0 and skin_sections[0]['resolved_material_slot'] == 8
         and skin_sections[0]['triangles'] == 23860 and not skin_sections[0]['disabled'], 'Actual face skin section0/slot8; teeth are not skin')
    basis, trans = frame['frame']['measured_exact_unit_axis_frame'], frame['frame']['translation_m']
    def mapped(v):
        return [sum(r[k]*v[k] for k in range(3))+trans[j] for j, r in enumerate(basis)]
    full = values['converted_skin']
    need(full['native_report'] == inputs['files']['native_rest'] and full['frame'] == frame['frame'], 'Same measured native/body frame required')
    mapped_vertices = [mapped(v) for s in (body, face) for v in s['positions_body_reference_copy_cm']]
    need(len(mapped_vertices) == len(full['mesh']['vertices_world_m'])
         and max(length(sub(a,b)) for a,b in zip(mapped_vertices, full['mesh']['vertices_world_m'])) < 1e-12,
         'Cached fitting vertices must reconstruct the complete actual native copy')
    reverse = frame['body_mesh']['native_triangle_order_reversed_to_match_fbx']
    triangles = [dict(vertices=list(reversed(t)) if reverse else t, polygon_index=i) for i,t in enumerate(body['triangles'])]
    triangles += [dict(vertices=[v+32334 for v in (list(reversed(t)) if reverse else t)], polygon_index=i)
                  for i,t in enumerate(face['triangles']) if face['triangle_sections'][i] == 0]
    need(full['mesh']['triangles'] == triangles and len(triangles) == 84676,
         'Every actual body/visible-face triangle and original winding must match')
    scene = values['source_scene']
    need(scene['observed_body_unchanged_since_pre_cap'] is True
         and scene['observed_original_parts_unchanged_since_pre_cap'] is True
         and scene['actual_cap_mesh_objects'] == [], 'R3 failed scene must remain unchanged and without caps')
    need(recipe['policy'] == POLICY and recipe['status'] == 'CPU_OWN_SURFACE_RECIPE_NOT_BLENDER_FIT'
         and recipe['mutable_part'] == COAT and recipe['preserved_original_parts'] == 16
         and recipe['source_reference_sha256'] == REF and recipe['minimum_clearance_m'] == .006
         and len(recipe['positions_world_m']) == 2145 and len(recipe['faces']) == 2880
         and len(recipe['uvs']) == len(recipe['faces']) and recipe['expected_whole_garment_triangles'] == 23368,
         'Only the exact authored upper Coat_body topology may change')
    for row in recipe['cpu_proofs'].values():
        pin(row)
    return inputs, values


def body_source_checked(body, arm, cfg, values, snapshot, skeleton):
    scene = values['source_scene']
    reference = skeleton(arm)
    need(signature(reference) == REF and len(reference['bones']) == 341 and reference['object_name'] == 'root',
         'Exact actual341 Blender reference; separate native342 gate remains unchanged')
    need(signature(snapshot(body)) == signature(scene['pre_cap_snapshots']['body']),
         'Fresh actual Body payload must be identical to the measured FBX source')
    # Actual current ref heads are checked against all measured native correspondences.
    refbones = {b['name']:b for b in values['native_rest']['rest_surfaces']['body']['bones']}
    frame = values['frame_confirmation']['frame'];A=frame['measured_exact_unit_axis_frame'];T=frame['translation_m']
    errors=[]
    for b in arm.data.bones:
        native = refbones[b.name];v=native['component_matrix4x4'][3][:3]
        target=[sum(A[j][k]*v[k] for k in range(3))+T[j] for j in range(3)]
        actual=tuple(arm.matrix_world @ b.head_local)
        parent = refbones[b.parent.name]['index'] if b.parent else 0
        need(native['parent_index'] == parent, 'Changed current source/native parent: '+b.name)
        errors.append(dict(name=b.name,actual=actual,expected=target,error_m=length(sub(actual,target))))
    need(max(q['error_m'] for q in errors) < .00025, 'Actual native/source frame failed')
    return errors


def surface_payload(obj):
    md=obj.data;md.calc_loop_triangles()
    return dict(vertices_world_m=[list(obj.matrix_world @ v.co) for v in md.vertices],
                triangles=[dict(vertices=list(t.vertices),polygon_index=t.polygon_index) for t in md.loop_triangles])


def signed_skin(skin,p):
    near=skin.nearest(p);counts=[]
    for direction in DIRECTIONS:
        unique=[]
        for hit in skin.ray_hits(p,direction,3):
            if not unique or abs(hit['distance']-unique[-1])>1e-6:unique.append(hit['distance'])
        counts.append(len(unique))
    flags=[v%2==1 for v in counts]
    row=dict(**near,ray_counts=counts,inside=flags[0],classification_consistent=len(set(flags))==1,
             signed_distance_m=near['distance']*(-1 if flags[0] else 1))
    return row


def samples(vertices):
    a,b,c=vertices
    return [mul(add(add(a,b),c),1/3),a,b,c,mul(add(a,b),.5),mul(add(b,c),.5),mul(add(c,a),.5)]


def in_neckline(p,ring):
    n=0
    for a,b in zip(ring,ring[1:]+ring[:1]):
        if (a[1]>p[1])!=(b[1]>p[1]) and a[0]+(p[1]-a[1])*(b[0]-a[0])/(b[1]-a[1])>p[0]:n+=1
    return n%2==1


def fit_actual(parts, values, evidence):
    """Measure actual meshes. Attach every raw list before any possible assertion."""
    recipe=values['recipe'];skinmesh=values['converted_skin']['mesh'];skin=Surface(skinmesh)
    facets=[];join=[];coverage=[];inner=[];orientation=[]
    evidence.update(status='MEASURING_NOT_ACCEPTED',minimum_clearance_m=.006,outer_target_m=.012,
                    facets=facets,legacy_join_layers=join,coverage=coverage,inner_diagnostic=inner,
                    orientation=orientation,continuous_surface_proof=False)
    coat=parts[COAT];need(len(coat['vertices_world_m'])==4290,'Actual Solidify coat vertex count changed')
    nouter=len(recipe['faces']);outer=Surface(dict(vertices_world_m=coat['vertices_world_m'],
        triangles=[t for t in coat['triangles'] if t['polygon_index']<nouter]))
    vertices=[];triangles=[];names=[]
    for name,limit in ((COAT,nouter),('Coat_full_sleeve_l',480),('Coat_full_sleeve_r',480),('Coat_low_fold_collar',96)):
        part=parts[name];off=len(vertices);vertices.extend(part['vertices_world_m'])
        for tri in part['triangles']:
            if tri['polygon_index']>=limit:continue
            triangles.append(dict(vertices=[v+off for v in tri['vertices']],polygon_index=tri['polygon_index']));names.append(name)
    cloth=Surface(dict(vertices_world_m=vertices,triangles=triangles))
    def layer(p,n):
        hits=cloth.ray_hits(add(p,mul(n,1e-6)),n,.15)
        for q in hits:q['component']=names[q['triangle']]
        outward=[h for h in hits if h['normal_dot']>.05]
        return hits,outward[-1] if outward else None
    for ti,tri in enumerate(coat['triangles']):
        if tri['polygon_index']>=nouter:continue
        band=recipe['face_bands'][tri['polygon_index']]
        if band<19:continue
        ps=[coat['vertices_world_m'][v] for v in tri['vertices']];normal=unit(cross(sub(ps[1],ps[0]),sub(ps[2],ps[0])))
        orientation.append(dict(triangle=ti,polygon=tri['polygon_index'],band=band,normal=normal,
                                twice_area_m2=length(cross(sub(ps[1],ps[0]),sub(ps[2],ps[0])))))
        for si,p in enumerate(samples(ps)):
            actual=signed_skin(skin,p);row=dict(triangle=ti,polygon=tri['polygon_index'],band=band,sample=si,
                                              point=p,normal=normal,actual_skin=actual)
            facets.append(row)
            need(actual['classification_consistent'],'Full actual skin parity rays disagree at a garment sample')
            if band>=20:
                need(actual['signed_distance_m']>=.006,'Actual upper Coat_body facet is below unchanged6mm skin clearance')
            elif actual['signed_distance_m']<.006:
                hits,selected=layer(actual['point'],actual['normal'])
                q=dict(facet=row,all_hits=hits,selected=selected)
                join.append(q)
                need(selected is not None and selected['distance']>=.006,
                     'Legacy lower join intersects exposed skin; no actual external sleeve/coat cover')
    ring=coat['vertices_world_m'][2145-65:2145];z=recipe['neck_plane'][2]
    for ti,tri in enumerate(skinmesh['triangles']):
        ps=[skinmesh['vertices_world_m'][i] for i in tri['vertices']];n=cross(sub(ps[1],ps[0]),sub(ps[2],ps[0]))
        if length(n)<1e-12:continue
        n=unit(n);centre=mul(add(add(ps[0],ps[1]),ps[2]),1/3)
        if not(n[2]>=.15 and .055<=abs(centre[0])<=.245 and 1.29<=centre[2]<=1.405):continue
        for si,p in enumerate(samples(ps)):
            distance=(z-p[2])/n[2];at_neck=add(p,mul(n,distance))
            through=distance>=-1e-6 and in_neckline(at_neck,ring)
            above=p[2]>=z and in_neckline(p,ring)
            hits,selected=layer(p,n)
            row=dict(skin_triangle=ti,sample=si,skin=p,normal=n,all_hits=hits,selected=selected,
                     neck_plane_distance=distance,at_neck=at_neck,neck_aperture=through,
                     above_plane_inside_neckline=above,required=not(through or above),
                     role='intentionally_open_neck' if through or above else 'shoulder_must_have_outer_cloth')
            coverage.append(row)
            need(not row['required'] or selected is not None and selected['distance']>=.006,
                 'Actual required shoulder skin sample has no outside garment layer with6mm clearance')
    # Actual inner shell distance is diagnostic. The unchanged6mm gate above
    # is the existing OUTER surface/coverage contract, not an invented inner gate.
    # Check upper inner vertices/centroids for penetration as an extra concrete failure.
    inner_start=nouter;inner_end=2*nouter
    for ti,tri in enumerate(coat['triangles']):
        pi=tri['polygon_index']
        if not(inner_start<=pi<inner_end) or recipe['face_bands'][pi-inner_start]<20:continue
        ps=[coat['vertices_world_m'][v] for v in tri['vertices']]
        for si,p in enumerate(samples(ps)):
            actual=signed_skin(skin,p);q=dict(triangle=ti,polygon=pi,sample=si,point=p,actual_skin=actual)
            inner.append(q)
            need(actual['classification_consistent'] and actual['signed_distance_m']>0,
                 'Actual upper inner cloth shell penetrates skin')
    upper=[q for q in facets if q['band']>=20]
    evidence.update(status='REST_YOKE_SAMPLED_NOT_SEATED_OR_NATIVE_ACCEPTED',
        upper_facet_samples=len(upper),upper_facet_minimum_m=min(q['actual_skin']['signed_distance_m'] for q in upper),
        required_skin_rays=sum(q['required'] for q in coverage),intentional_neck_rays=sum(not q['required'] for q in coverage),
        required_skin_minimum_m=min(q['selected']['distance'] for q in coverage if q['required']),
        actual_inner_minimum_m=min(q['actual_skin']['signed_distance_m'] for q in inner),
        legacy_join_internal_samples=len(join),legacy_join_outer_layer_minimum_m=min(q['selected']['distance'] for q in join) if join else None)


def actual_shading_footprint(before,after,old_polygons,polygons,recipe,evidence):
    # Normal fans depend on both coordinates and oriented incident faces.
    # Derive this one-ring directly, never from a band/loop exception list.
    old_to_new={}
    for ri,r in enumerate(recipe['row_parameters']):
        if r!=int(r):continue
        for col in range(65):
            a=int(r)*65+col;b=ri*65+col
            old_to_new[a]=b;old_to_new[1625+a]=2145+b
    source_faces=[];actual_faces=[];retired=[];incident=[];unexpected=[];lower_moved=[]
    evidence.update(status='DERIVING_ACTUAL_ORIENTED_TOPOLOGY_DEPENDENCY_NOT_FIT',
        old_to_new_vertex=[[a,b] for a,b in sorted(old_to_new.items())],
        source_faces=source_faces,actual_faces=actual_faces,retired_refined_source_faces=retired,
        incident_changed_polygons=incident,unexpected_oriented_faces=unexpected,
        unexpected_preserved_vertex_movements=lower_moved,
        normal_outside_dependency_policy=dict(vector_limit=NORMAL_VECTOR_LIMIT,one_minus_dot_limit=NORMAL_ONE_MINUS_DOT_LIMIT),
        normal_dependency_definition='Vertices of actual faces incident to moved/added vertices or new oriented face cycles, plus mapped vertices of retired/refined source faces. One ring only; no band/loop whitelist.')
    need(len(old_to_new)==3250 and len(set(old_to_new.values()))==3250,
         'Complete unique actual original-to-refined row mapping required')
    need(len(before)==3250 and len(after)==4290 and len(old_polygons)==3248 and len(polygons)==5952,
         'Actual complete coat vertex/polygon inventory changed')
    need(set(old_to_new)==set(range(len(before))) and all(0<=v<len(after) for v in old_to_new.values()),
         'Actual source row mapping is incomplete or outside the new mesh')
    new_to_old={b:a for a,b in old_to_new.items()}
    added=set(range(len(after)))-set(new_to_old)
    changed={b for a,b in old_to_new.items() if before[a]!=after[b]}
    changed_or_added=changed|added
    evidence.update(added_actual_vertices=[dict(actual_vertex=b,after=after[b]) for b in sorted(added)],
        changed_mapped_actual_vertices=[dict(source_vertex=new_to_old[b],actual_vertex=b,
            before=before[new_to_old[b]],after=after[b],distance_m=length(sub(after[b],before[new_to_old[b]]))) for b in sorted(changed)],
        changed_or_added_actual_vertex_ids=sorted(changed_or_added))
    old_lookup={};new_lookup={}
    for index,vertices in enumerate(old_polygons):
        mapped=[old_to_new[v] for v in vertices]
        key=oriented_cycle(mapped)
        row=dict(source_polygon=index,source_vertices=vertices,mapped_actual_vertices=mapped,oriented_cycle=list(key))
        source_faces.append(row)
        need(len(vertices)>=3 and len(set(vertices))==len(vertices) and key not in old_lookup,
             'Ambiguous/degenerate original oriented coat face')
        old_lookup[key]=index
    for index,vertices in enumerate(polygons):
        key=oriented_cycle(vertices);trigger=set(vertices)&changed_or_added
        row=dict(actual_polygon=index,vertices=vertices,oriented_cycle=list(key),
                 source_polygon=old_lookup.get(key),changed_or_added_vertices=sorted(trigger),
                 new_or_refined_oriented_face=key not in old_lookup)
        actual_faces.append(row)
        need(len(vertices)>=3 and len(set(vertices))==len(vertices)
             and all(0<=v<len(after) for v in vertices) and key not in new_lookup,
             'Ambiguous/degenerate actual oriented coat face')
        new_lookup[key]=index
    # Bound this graph to the exact authored outer recipe and its ordinary
    # 2.5mm Solidify inner/rim topology. A reversed face must never become
    # a convenient extra shading exception.
    outer=recipe['faces'];nouter=len(recipe['positions_world_m']);edges={}
    need(nouter==2145 and len(outer)==2880,'Exact authored outer inventory required')
    for index,vertices in enumerate(outer):
        need(len(vertices)>=3 and len(set(vertices))==len(vertices)
             and all(0<=v<nouter for v in vertices),'Invalid authored oriented outer face')
        for a,b in zip(vertices,vertices[1:]+vertices[:1]):
            need((a,b) not in edges,'Duplicate authored oriented edge')
            edges[a,b]=index
    boundary=[(a,b) for a,b in edges if (b,a) not in edges]
    expected_faces=outer+[[v+nouter for v in reversed(f)] for f in outer]
    expected_faces+=[[b,a,a+nouter,b+nouter] for a,b in boundary]
    expected={oriented_cycle(f) for f in expected_faces}
    evidence.update(expected_solidify_oriented_face_cycles=[list(oriented_cycle(f)) for f in expected_faces],
                    actual_boundary_edges=[list(e) for e in boundary])
    for key,index in new_lookup.items():
        if key not in expected:unexpected.append(actual_faces[index])
    missing=sorted(expected-set(new_lookup))
    evidence['missing_expected_oriented_faces']=[list(v) for v in missing]
    need(len(expected)==len(expected_faces)==len(polygons) and not unexpected and not missing,
         'Actual coat topology differs from the exact authored oriented outer/inner/rim recipe')
    for a in list(range(1300))+list(range(1625,1625+1235)):
        b=old_to_new[a]
        if before[a]!=after[b]:lower_moved.append(dict(source_vertex=a,actual_vertex=b,before=before[a],after=after[b]))
    need(not lower_moved,'Actual untouched lower coat vertex moved outside authored position scope')
    shade=set();dependency={};retired_dependency={}
    for row in actual_faces:
        reasons=[]
        if row['changed_or_added_vertices']:reasons.append('incident_to_actual_moved_or_added_vertex')
        if row['new_or_refined_oriented_face']:reasons.append('new_or_refined_actual_oriented_face')
        row['change_reasons']=reasons
        if not reasons:continue
        incident.append(row);shade.update(row['vertices'])
        for vertex in row['vertices']:dependency.setdefault(vertex,[]).append(row['actual_polygon'])
    for key,index in old_lookup.items():
        source_faces[index]['retired_or_refined']=key not in new_lookup
        if key in new_lookup:continue
        row=source_faces[index];retired.append(row);shade.update(row['mapped_actual_vertices'])
        for vertex in row['mapped_actual_vertices']:retired_dependency.setdefault(vertex,[]).append(index)
    evidence.update(status='ACTUAL_ORIENTED_FACE_SHADING_DEPENDENCY_DERIVED_NOT_FIT',
        affected_shading_vertex_ids=sorted(shade),changed_or_added_vertices=len(changed_or_added),
        incident_changed_polygon_count=len(incident),affected_shading_vertices=len(shade),
        actual_new_or_refined_face_count=sum(r['new_or_refined_oriented_face'] for r in actual_faces),
        retired_refined_source_face_count=len(retired),
        retired_source_face_dependency_by_actual_vertex={str(v):ids for v,ids in sorted(retired_dependency.items())})
    return old_to_new,new_to_old,shade,dependency


def check_actual_normal_dependency(old_loops,old_corners,new_loops,new_corners,new_to_old,shade,dependency,evidence):
    # Keep all actual corner data, including the cause of a rejection. Added
    # vertices have no old normal; never invent one for a comparison.
    old_normals={}
    need(len(old_loops)==len(old_corners) and len(new_loops)==len(new_corners),
         'Actual normal/loop payload must be complete')
    for li,vertex in enumerate(old_loops):old_normals.setdefault(vertex,set()).add(tuple(old_corners[li]))
    queries=[];changed=[];affected=[];added=[];outside_ids=[]
    maximum=0.;maximum_dot=0.;inside_max=0.;inside_dot=0.;outside_exact=0
    evidence.update(actual_corner_queries=queries,actual_normal_changes=changed,
        affected_actual_loop_ids=affected,added_vertex_actual_loop_ids=added,
        outside_dependency_actual_loop_ids=outside_ids,normal_checks_completed=False)
    for li,vertex in enumerate(new_loops):
        source_vertex=new_to_old.get(vertex);inside=vertex in shade;actual=list(new_corners[li])
        before=old_normals[source_vertex] if source_vertex is not None else set()
        record=dict(actual_loop=li,actual_vertex=vertex,source_vertex=source_vertex,actual_normal=actual,
                    reference_normals=[list(x) for x in sorted(before)],inside_actual_shading_dependency=inside,
                    actual_changed_face_neighbours=dependency.get(vertex,[]),
                    retired_source_face_neighbours=evidence['retired_source_face_dependency_by_actual_vertex'].get(str(vertex),[]))
        queries.append(record);evidence['last_normal_query']=record
        (affected if inside else outside_ids).append(li)
        need(all(math.isfinite(x) for x in actual) and length(actual)>1e-12,'Invalid actual computed coat normal')
        if source_vertex is None:
            added.append(li);record['comparison']='NO_SOURCE_NORMAL_FOR_ADDED_VERTEX'
            need(inside,'Added vertex outside actual shading dependency')
            continue
        if tuple(actual) not in before:changed.append(record)
        measured=normal_comparison(actual,list(min(before,key=lambda n:length(sub(actual,n)))))
        record['actual_comparison']=measured
        if inside:
            inside_max=max(inside_max,measured['vector_delta']);inside_dot=max(inside_dot,measured['one_minus_dot'])
        else:
            need(len(before)==1,'Unchanged original smooth vertex has unexpected split normals')
            maximum=max(maximum,measured['vector_delta']);maximum_dot=max(maximum_dot,measured['one_minus_dot'])
            outside_exact+=tuple(actual) in before
            need(measured['valid'],'Normal changed outside actual oriented-face dependency and strict numeric bound')
    evidence.update(normal_checks_completed=True,outside_dependency_normal_corners=len(outside_ids),
        outside_dependency_exact_normal_corners=outside_exact,affected_normal_corner_count=len(affected),
        added_vertex_normal_corner_count=len(added),changed_mapped_normal_corner_count=len(changed),
        maximum_inside_dependency_normal_delta=inside_max,maximum_inside_dependency_one_minus_dot=inside_dot,
        maximum_outside_dependency_normal_delta=maximum,maximum_outside_dependency_one_minus_dot=maximum_dot)


def replace_coat(coat, recipe, thickness, evidence, recipe_input):
    import bpy
    before=surface_payload(coat);old=coat.data
    evidence.update(before=before,proposed_recipe=file_row(pin(recipe_input)))
    need(len(old.vertices)==3250 and len(old.polygons)==3248,'Actual original coat inventory changed')
    need([list(v.co) for v in old.vertices[:1300]]==recipe['positions_world_m'][:1300],
         'Exact lower source coat prefix changed')
    data=bpy.data.meshes.new(COAT+'_R4_yoke')
    data.from_pydata(recipe['positions_world_m'],[],recipe['faces']);data.update()
    need(not data.validate(verbose=True),'Blender repaired authored R4 garment topology')
    for mat in old.materials:data.materials.append(mat)
    uv=data.uv_layers.new(name='GarmentUV')
    for p,coords in zip(data.polygons,recipe['uvs']):
        p.use_smooth=True
        for li,coord in zip(p.loop_indices,coords):uv.data[li].uv=coord
    coat.data=data
    bpy.ops.object.select_all(action='DESELECT');coat.select_set(True);bpy.context.view_layer.objects.active=coat
    mod=coat.modifiers.new('R4 same measured fabric thickness','SOLIDIFY');mod.thickness=thickness;mod.offset=-1
    need(thickness==.0025,'Original fabric thickness must remain2.5mm')
    bpy.ops.object.modifier_apply(modifier=mod.name);coat.select_set(False)
    md=coat.data;md.update();md.calc_loop_triangles();after=surface_payload(coat)
    evidence.update(after=after,actual_vertices=len(md.vertices),actual_triangles=len(md.loop_triangles))
    footprint={};evidence['actual_shading_footprint']=footprint
    old_to_new,new_to_old,shade,dependency=actual_shading_footprint(
        before['vertices_world_m'],after['vertices_world_m'],[list(p.vertices) for p in old.polygons],
        [list(p.vertices) for p in md.polygons],recipe,footprint)
    check_actual_normal_dependency([loop.vertex_index for loop in old.loops],[list(n.vector) for n in old.corner_normals],
        [loop.vertex_index for loop in md.loops],[list(n.vector) for n in md.corner_normals],new_to_old,shade,dependency,footprint)
    need(len(md.vertices)==4290 and len(md.loop_triangles)==8576,'Actual generated coat exceeds bounded topology')
    need(all(list(md.polygons[i].vertices)==face for i,face in enumerate(recipe['faces'])),
         'Solidify did not retain the actual oriented outer face topology')
    # Explicit allowed positional/normal footprint. Lower inner points use
    # their original source indices; an upper new triangle must not be
    # allowed to alter the untouched lower garment.
    outer_changed=[];inner_changed=[];normal_changed=[]
    evidence.update(changed_original_outer_vertices=outer_changed,changed_original_inner_vertices=inner_changed,
                    changed_lower_boundary_normals=normal_changed,
                    numerical_normal_policy=dict(vector_limit=NORMAL_VECTOR_LIMIT,one_minus_dot_limit=NORMAL_ONE_MINUS_DOT_LIMIT))
    for i in range(1300):
        need(list(md.vertices[i].co)==list(old.vertices[i].co),'Untouched lower outer coat moved')
    for i in range(1235):
        need(list(md.vertices[2145+i].co)==list(old.vertices[1625+i].co),'Untouched lower inner coat moved')
    for ri,row in enumerate(recipe['row_parameters']):
        if row!=int(row):continue
        oi=int(row)*65;ni=ri*65
        for col in range(65):
            if list(md.vertices[ni+col].co)!=list(old.vertices[oi+col].co):outer_changed.append(dict(source_vertex=oi+col,new_vertex=ni+col,distance_m=length(sub(md.vertices[ni+col].co,old.vertices[oi+col].co))))
            if list(md.vertices[2145+ni+col].co)!=list(old.vertices[1625+oi+col].co):inner_changed.append(dict(source_vertex=1625+oi+col,new_vertex=2145+ni+col,distance_m=length(sub(md.vertices[2145+ni+col].co,old.vertices[1625+oi+col].co))))
    for pi in range(1216):
        a,b=md.polygons[pi],old.polygons[pi]
        need(list(a.vertices)==list(b.vertices) and a.material_index==b.material_index and a.use_smooth==b.use_smooth,
             'Original lower topology/material/smoothing changed')
        for la,lb in zip(a.loop_indices,b.loop_indices):
            need(list(md.uv_layers['GarmentUV'].data[la].uv)==list(old.uv_layers['GarmentUV'].data[lb].uv),'Original lower GarmentUV changed')
            na,nb=list(md.corner_normals[la].vector),list(old.corner_normals[lb].vector)
            measured=normal_comparison(na,nb)
            vertex=md.loops[la].vertex_index;actual_dependency=vertex in shade
            if na!=nb:normal_changed.append(dict(source_loop=lb,new_loop=la,source_normal=nb,new_normal=na,
                                                actual_comparison=measured,original_polygon=pi,
                                                inside_actual_shading_dependency=actual_dependency,
                                                actual_changed_face_neighbours=dependency.get(vertex,[])))
            if not actual_dependency:need(measured['valid'],'Normal outside actual incident-face dependency exceeds unchanged strict numeric bound')
    evidence.update(changed_original_outer_vertices=outer_changed,changed_original_inner_vertices=inner_changed,
                    changed_lower_boundary_normals=normal_changed,added_outer_vertices=520,
                    exact_preserved_outer_vertex_prefix=1300,exact_preserved_inner_vertex_prefix=1235,
                    exact_preserved_outer_polygon_prefix=1216,normal_dependency_scope='actual moved/added vertices and oriented face changes only',
                    maximum_original_outer_displacement_m=max(q['distance_m'] for q in outer_changed),
                    maximum_original_inner_displacement_m=max(q['distance_m'] for q in inner_changed))
    need(evidence['maximum_original_outer_displacement_m']<=.12,'Bounded local source displacement exceeded')


# Exact oriented source faces/corners; only measured float-normal arithmetic
# is compared numerically. No position, UV, material or weight tolerance.
NORMAL_VECTOR_LIMIT = 2e-7
NORMAL_ONE_MINUS_DOT_LIMIT = 1e-14


def normal_comparison(actual, expected):
    delta=length(sub(actual,expected))
    good=all(math.isfinite(v) for v in (*actual,*expected)) and min(length(actual),length(expected))>1e-12
    angular=max(0.,1-dot(unit(actual),unit(expected))) if good else float('inf')
    return dict(actual=actual,reference=expected,vector_delta=delta,one_minus_dot=angular,
                vector_limit=NORMAL_VECTOR_LIMIT,one_minus_dot_limit=NORMAL_ONE_MINUS_DOT_LIMIT,
                valid=good and delta<=NORMAL_VECTOR_LIMIT and angular<=NORMAL_ONE_MINUS_DOT_LIMIT)


def oriented_cycle(vertices):
    return min(tuple(vertices[i:]+vertices[:i]) for i in range(len(vertices)))


def compare_coat_reference(actual, expected, evidence):
    rows=[];position_diff=[]
    evidence.update(status='MEASURING_REFERENCE_NOT_FIT',oriented_polygon_corners=rows,
                    position_differences=position_diff,normal_vector_limit=NORMAL_VECTOR_LIMIT,
                    normal_one_minus_dot_limit=NORMAL_ONE_MINUS_DOT_LIMIT,
                    scope='Only cross-process Solidify rim ordering and measured normal arithmetic; exact positions/UV/material/smoothing/weights and original orientation')
    for key in ('object_name','matrix_world','parent','parent_type','parent_bone','matrix_parent_inverse',
                'materials','weights','armature_modifiers'):
        evidence.setdefault('metadata',{})[key]=dict(actual=actual[key],reference=expected[key],exact=actual[key]==expected[key])
        need(actual[key]==expected[key],'Original coat metadata/weights changed: '+key)
    for key in ('vertices_local','vertices_world_m'):
        av,ev=actual[key],expected[key]
        need(len(av)==len(ev)==3250,'Original coat position count changed')
        for i,(a,b) in enumerate(zip(av,ev)):
            if a!=b:position_diff.append(dict(field=key,vertex=i,actual=a,reference=b))
        need(not position_diff,'Original coat positions changed')
    ap,ep=actual['polygons'],expected['polygons']
    need(len(ap)==len(ep)==3248,'Original coat polygon inventory changed')
    need(set(actual['uv_layers'])==set(expected['uv_layers']), 'Original coat UV layers changed')
    lookup={}
    for i,face in enumerate(ep):
        need(len(set(face['vertices']))==len(face['vertices']), 'Repeated original source corner')
        key=oriented_cycle(face['vertices'])
        need(key not in lookup,'Ambiguous original oriented coat polygon')
        lookup[key]=i
    used=set();normal_max=0.;dot_max=0.;permuted=0;different=0
    for i,face in enumerate(ap):
        key=oriented_cycle(face['vertices']);match=lookup.get(key)
        row=dict(actual_polygon=i,reference_polygon=match,actual=face,corners=[])
        rows.append(row)  # Keep the exact cause even on the first rejection.
        need(match is not None and match not in used,'Missing/reversed/duplicate original coat polygon')
        used.add(match);source=ep[match];row['reference']=source
        need(face['material_index']==source['material_index'] and face['smooth']==source['smooth'],
             'Original oriented coat material/smoothing changed')
        permuted+=i!=match
        for vertex,li in zip(face['vertices'],face['loop_indices']):
            source_corner=source['vertices'].index(vertex);old_loop=source['loop_indices'][source_corner]
            actual_uv={n:v[li] for n,v in actual['uv_layers'].items()}
            source_uv={n:v[old_loop] for n,v in expected['uv_layers'].items()}
            normal=normal_comparison(actual['corner_normals_local'][li],expected['corner_normals_local'][old_loop])
            corner=dict(vertex=vertex,actual_loop=li,reference_loop=old_loop,actual_uv=actual_uv,
                        reference_uv=source_uv,normal=normal)
            row['corners'].append(corner)
            need(actual_uv==source_uv,'Original mapped GarmentUV corner changed')
            need(normal['valid'],'Original mapped coat normal exceeds measured numeric bound')
            normal_max=max(normal_max,normal['vector_delta']);dot_max=max(dot_max,normal['one_minus_dot'])
            different+=normal['actual']!=normal['reference']
    need(len(used)==3248,'Original coat polygon lost')
    evidence.update(status='ACTUAL_ORIENTED_COAT_REFERENCE_SEMANTICS_PASS_NOT_FIT',
                    vertices_exact=True,oriented_polygons_matched=len(used),source_polygon_permuted=permuted,
                    normal_corners_different=different,maximum_corner_normal_vector_delta=normal_max,
                    maximum_normal_one_minus_dot=dot_max,UV_material_smoothing_weights_exact=True)


def apply(body,arm,coat,parts,cfg,report,snapshot,skeleton):
    inputs,values=inputs_checked(cfg);scene=values['source_scene'];recipe=values['recipe']
    evidence=dict(policy=POLICY,mutable_part=COAT,preserved_original_parts=16,
                  source_body_faces_copied=False,source_body_mutated=False,native_accepted=False,seated_fit_accepted=False,
                  inputs=cfg['coat_yoke']['inputs'],source_files=inputs['files'],status='STARTED_NOT_ACCEPTED')
    report['coat_yoke']=evidence
    evidence['actual_source_heads']=body_source_checked(body,arm,cfg,values,snapshot,skeleton)
    originals={ob.name:snapshot(ob) for ob in parts}
    expected_parts=scene['pre_cap_snapshots']['parts']
    need(set(originals)==set(expected_parts) and len(originals)==17,
         'All17 original source parts must exist before bounded change')
    for name,value in originals.items():
        if name!=COAT:
            need(signature(value)==signature(expected_parts[name]),
                 'One of16 original source parts changed before the yoke: '+name)
    # The actual R4 rejection measured only170 cyclic-equivalent rim quads
    # and124 float-normal corners. Never replace this with a general hash
    # exception: compare all actual vertices, oriented faces and mapped data.
    evidence['original_coat_reference_comparison']={}
    import failure_scene
    compare_coat_reference(failure_scene.mesh_payload(coat),scene['meshes'][COAT],
                           evidence['original_coat_reference_comparison'])
    retained_snapshot_keys=set(expected_parts[COAT])-{'corner_normals_sha256','faces_slots_smoothing_sha256','uv_layers_sha256'}
    need(all(originals[COAT][key]==expected_parts[COAT][key] for key in retained_snapshot_keys),
         'Original coat changed beyond the explicit oriented-corner comparison')
    report['existing_parts_before_yoke']=originals
    partlist=list(parts);need(len(partlist)==17 and coat.name==COAT,'Exact original17 part inventory')
    evidence['coat_preservation']={}
    replace_coat(coat,recipe,cfg['fit']['fabric_thickness_m'],evidence['coat_preservation'],inputs['files']['recipe'])
    retained={ob.name:snapshot(ob) for ob in parts if ob!=coat}
    need(len(retained)==16 and all(signature(v)==signature(originals[n]) for n,v in retained.items()),'One of the16 preserved garment parts changed')
    need(signature(snapshot(body))==signature(scene['pre_cap_snapshots']['body']),'Original Body changed')
    evidence['preserved_parts']=retained
    evidence['fit']={}
    fit_actual({ob.name:surface_payload(ob) for ob in parts},values,evidence['fit'])
    evidence['status']='REST_YOKE_MEASURED_NOT_SEATED_OR_NATIVE_ACCEPTED'
    report['existing_parts_preserved_before_weight_transfer']=retained
    return values


def tag_parts(parts,report):
    """Source-only identity survives join/reopen; never a runtime interaction."""
    records={}
    for part_id,ob in enumerate(parts):
        md=ob.data
        need(all(md.attributes.get(n) is None for n in ATTR),'Unexpected own source identity attribute')
        pid=md.attributes.new(ATTR[0],'INT','FACE');vi=md.attributes.new(ATTR[1],'INT','POINT');fi=md.attributes.new(ATTR[2],'INT','FACE')
        for p in md.polygons:pid.data[p.index].value=part_id;fi.data[p.index].value=p.index
        for v in md.vertices:vi.data[v.index].value=v.index
        records[ob.name]=dict(part_id=part_id,payload=surface_payload(ob),vertices=len(md.vertices),faces=len(md.polygons),
                             face_vertex_lists=[list(p.vertices) for p in md.polygons])
    report['source_part_identity']=records


def recover_parts(clothes,report):
    md=clothes.data;md.calc_loop_triangles()
    attrs=[md.attributes.get(n) for n in ATTR]
    need(all(attrs) and attrs[0].domain=='FACE' and attrs[1].domain=='POINT' and attrs[2].domain=='FACE',
         'Actual joined source part identities missing')
    lookup={r['part_id']:n for n,r in report['source_part_identity'].items()};need(len(lookup)==17,'All17 source parts required')
    results={};vmap={};fmap={}
    for p in md.polygons:
        pid=attrs[0].data[p.index].value;name=lookup.get(pid)
        need(name is not None,'Unknown joined source part')
        results.setdefault(name,dict(vertices_world_m=[None]*report['source_part_identity'][name]['vertices'],triangles=[]))
        maps=vmap.setdefault(name,{});localface=attrs[2].data[p.index].value
        need(localface not in fmap.setdefault(name,{}),'Duplicate local source face')
        fmap[name][localface]=p.index
        localvertices=[]
        for index in p.vertices:
            local=attrs[1].data[index].value;localvertices.append(local)
            need(0<=local<len(results[name]['vertices_world_m']),'Source local vertex out of range')
            previous=maps.setdefault(local,index);need(previous==index,'Duplicate local source vertex')
            results[name]['vertices_world_m'][local]=list(clothes.matrix_world @ md.vertices[index].co)
        need(localvertices==report['source_part_identity'][name]['face_vertex_lists'][localface],
             'Actual joined source topology changed')
    for tri in md.loop_triangles:
        pid=attrs[0].data[tri.polygon_index].value;name=lookup[pid]
        results[name]['triangles'].append(dict(vertices=[attrs[1].data[v].value for v in tri.vertices],
                                             polygon_index=attrs[2].data[tri.polygon_index].value))
    for name,part in results.items():
        expected=report['source_part_identity'][name]
        need(len(fmap[name])==expected['faces'] and None not in part['vertices_world_m'],'Joined source part incomplete')
        # Joining under the same unchanged armature may change float storage
        # at the metre transform's last bit. Measure it, never rewrite data.
        error=max(length(sub(a,b)) for a,b in zip(part['vertices_world_m'],expected['payload']['vertices_world_m']))
        need(error<=1e-6,'Actual joined source moved relative to prejoin part')
        need(sorted((t['polygon_index'],t['vertices']) for t in part['triangles'])==sorted((t['polygon_index'],t['vertices']) for t in expected['payload']['triangles']),
             'Actual joined source triangulation changed')
    need(set(results)==set(report['source_part_identity']),'A preserved part was lost')
    return results
