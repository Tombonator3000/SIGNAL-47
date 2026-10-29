"""To korte, målte frakkskuldrer. Ingen kroppssletting eller rigg-/poseendring.

Kun root kjører build() inne i Blender. Import/ren geometri krever ikke bpy.
Alle BVH-punkter og alle avviste kandidater blir private diagnosedata.
"""
import math

import geometry as G


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def ellipse(row, index):
    angle = -math.pi / 2 + math.tau * index / 40
    c, s = math.cos(angle), math.sin(angle)
    return G.add(G.mul(row['u'], c * row['rx']),
                 G.mul(row['v'], s * (row['front'] if s < 0 else row['back'])))


def bvh(obj):
    from mathutils.bvhtree import BVHTree
    # Faktiske, allerede forfattede flater med world-meterskala. Ingen evaluated
    # pose, modifierkjøring eller annen mutasjon av originalkroppen.
    return BVHTree.FromPolygons([obj.matrix_world @ v.co for v in obj.data.vertices],
                               [tuple(p.vertices) for p in obj.data.polygons],
                               all_triangles=False, epsilon=0.)


def outside(tree, centre, direction, bounds, label, capture=None):
    """Finn en faktisk utvendig side på en avgrenset gjennomgående stråle.

    Senteret kan ligge utenfor den smale frakken. Start derfor bak senteret,
    mål alle treff og krev utvendig normal; ingen nærmeste-punkt-fallback.
    """
    from mathutils import Vector
    c, d = Vector(centre), Vector(G.unit(direction))
    start = c - d * .30
    travelled, hits, chosen = 0., [], None
    result = capture if capture is not None else {}
    result.update(label=label, centre_m=list(c), direction=list(d), all_hits=hits, selected=None)
    for _ in range(64):
        hit, normal, face, distance = tree.ray_cast(start, d, .60 - travelled)
        if hit is None:
            break
        radius = float((hit - c).dot(d))
        row = {'point_m': list(hit), 'normal': list(normal), 'face': int(face),
               'radius_m': radius, 'normal_dot_ray': float(normal.dot(d))}
        eligible = bounds[0] <= radius <= bounds[1] and row['normal_dot_ray'] > .10
        row['eligible_outward'] = eligible
        hits.append(row)
        if eligible and (chosen is None or radius > chosen['radius_m']):
            chosen = row
            result['selected'] = row
        advance = float(distance) + 1e-5
        travelled += advance
        if travelled >= .60:
            break
        start = hit + d * 1e-5
    else:
        raise RuntimeError(label + ': over 64 flater på avgrenset skulderstråle')
    return result


def oriented(data, centres):
    """Bevar faktisk utvendig winding også for den speilte høyre skulderen."""
    columns = len(data['vertices']) // len(centres)
    signs = []
    for face in data['faces']:
        a, b, c = (data['vertices'][i] for i in face[:3])
        mid = G.mul(tuple(sum(data['vertices'][i][k] for i in face) for k in range(3)), 1 / len(face))
        ring = face[0] // columns
        axis_mid = G.mul(G.add(centres[ring], centres[ring + 1]), .5)
        signs.append(G.dot(G.cross(G.sub(b, a), G.sub(c, a)), G.sub(mid, axis_mid)))
    need(all(abs(x) > 1e-12 for x in signs), 'Degenerert/tangentiell skulderflate')
    need(all(x > 0 for x in signs) or all(x < 0 for x in signs),
         'Foldet skulderkappe med blandet winding')
    if signs[0] < 0:
        data = dict(data, faces=[tuple(reversed(f)) for f in data['faces']],
                    uvs=[tuple(reversed(uv)) for uv in data['uvs']])
    G.audit(data)
    return data


def build(body, coat, sleeve_profiles, existing_parts, joint, mesh, cfg, report):
    """Eksisterende frakk/ermer/rigg endres ikke. Bare to nye cap-mesh legges til."""
    policy = cfg['shoulder_caps']
    need(policy['policy'] == 'measured-upper-shoulder-overlap-v1', 'Eksplisitt skulderpolicy kreves')
    need(policy['ring_vertices'] == 40 and policy['upper_arc_intervals'] == 24 and policy['rows'] == 5,
         'Kun lokal øvre 216-graders skulderkappe, ikke hel overkropp')
    need(policy['source_skeleton_sha256'] == '9b1eb0e830b3041e7d931692f06845d9c433b7265bce9b8c5ed09e2463eea113',
         'Eksakt original 342-beins Blender-restskjelett kreves')
    body_tree, coat_tree = bvh(body), bvh(coat)
    evidence = {'policy': policy['policy'], 'status': 'MEASURING_NOT_ACCEPTED', 'sides': [],
                'source_body_faces_copied': False, 'source_body_mutated': False,
                'seated_fit_accepted': False, 'native_accepted': False}
    report['shoulder_caps'] = evidence
    surface_gap, seam_lift = policy['skin_clearance_m'], policy['seam_lift_m']
    need(surface_gap == .025 and seam_lift == .0015 and cfg['fit']['fabric_thickness_m'] == .0025,
         'Bevar målt ermeklaring og eksisterende stofftykkelse')
    for side in 'lr':
        start = joint('upperarm_' + side)
        axis = G.unit(G.sub(joint('lowerarm_' + side), start))
        rows = sleeve_profiles['Coat_full_sleeve_' + side]
        outer = rows[1]  # Faktisk første komplette ring ned langs eksisterende erme.
        end = outer['centre']
        sleeve = next(ob for ob in existing_parts if ob.name == 'Coat_full_sleeve_' + side)
        measured_ring = []
        side_evidence = {'side': side, 'upperarm_m': start, 'axis': axis, 'outer_ring': outer,
                         'actual_outer_ring_vertices': measured_ring,
                         'basis_v_dot_world_up': outer['v'][2],
                         'candidates': [], 'accepted_candidate': None,
                         'actual_skin_samples': [], 'cap_centres_m': [], 'proposed_vertices_m': []}
        evidence['sides'].append(side_evidence)
        unique_vertices = set()
        # Match the actual outer vertices to the unchanged authored ring. No
        # assumption about Solidify's postmodifier vertex order or mirror sign.
        for index in range(40):
            expected = G.add(end, ellipse(outer, index))
            actual_index, actual = min(enumerate(sleeve.data.vertices), key=lambda item:
                G.length(G.sub(tuple(sleeve.matrix_world @ item[1].co), expected)))
            point = tuple(sleeve.matrix_world @ actual.co)
            measured_ring.append({'ring_index': index, 'vertex_index': actual_index,
                                  'world_m': point, 'expected_unmodified_ring_m': expected,
                                  'actual_match_error_m': G.length(G.sub(point, expected))})
            need(G.length(G.sub(point, expected)) <= 1e-6 and actual_index not in unique_vertices,
                 'Original ermering kan ikke identifiseres entydig i faktisk Solidify-mesh')
            unique_vertices.add(actual_index)
        top = max(range(40), key=lambda i: measured_ring[i]['world_m'][2])
        arc_indices = [(top + offset) % 40 for offset in range(-12, 13)]
        need(len(set(arc_indices)) == 25, 'Øvre kappe må ha 24 sammenhengende originale ringintervaller')
        side_evidence.update(actual_max_z_ring_index=top, selected_upper_arc_indices=arc_indices)
        selected = None
        for extension in policy['proximal_extensions_m']:
            centre = G.sub(start, G.mul(axis, extension))
            candidate = {'extension_m': extension, 'centre_m': centre, 'samples': [], 'valid': True}
            side_evidence['candidates'].append(candidate)
            inner_points, directions, radii = [], [], []
            for index in arc_indices:
                radial = G.sub(measured_ring[index]['world_m'], end)
                direction = G.unit(radial)
                raw = {'arc_index': index, 'coat': {}, 'skin': {}}
                candidate['samples'].append(raw)  # Preserve even a ray-loop exception.
                cloth = outside(coat_tree, centre, direction, (.001, .23), 'coat-seam', raw['coat'])
                skin = outside(body_tree, centre, direction, (.001, .23), 'body-seam', raw['skin'])
                ch, bh = cloth['selected'], skin['selected']
                valid = (ch is not None and bh is not None
                         and ch['radius_m'] - bh['radius_m'] >= .006
                         and (ch['point_m'][0] - start[0]) * (1 if side == 'l' else -1) > -.10
                         and abs(ch['point_m'][2] - start[2]) <= .13)
                raw['measured_seam_valid'] = valid
                candidate['valid'] &= valid
                if valid:
                    inner_points.append(G.add(ch['point_m'], G.mul(direction, seam_lift)))
                    directions.append(direction)
                    radii.append(ch['radius_m'] + seam_lift)
            if candidate['valid']:
                selected = (centre, inner_points, directions, radii)
                side_evidence['accepted_candidate'] = len(side_evidence['candidates']) - 1
                break
        need(selected is not None, 'Ingen faktisk målbar kroppsfri frakksøm ved skulder ' + side)
        inner_centre, inner_points, directions, inner_radii = selected
        vertices = side_evidence['proposed_vertices_m']
        centres = side_evidence['cap_centres_m']
        actual_samples = side_evidence['actual_skin_samples']
        for row_index in range(5):
            t = row_index / 4
            centre = G.add(G.mul(inner_centre, 1 - t), G.mul(end, t))
            centres.append(centre)
            for column, (index, direction) in enumerate(zip(arc_indices, directions)):
                outer_point = measured_ring[index]['world_m']
                outer_radius = G.length(G.sub(outer_point, end)) + seam_lift
                radius = inner_radii[column] * (1 - t) + outer_radius * t
                raw = {'row': row_index, 'arc_index': index, 'actual_skin': {},
                       'source_outer_vertex_m': outer_point, 'proposed_radius_before_skin_m': radius}
                actual_samples.append(raw)  # Raw input survives every following need().
                skin = outside(body_tree, centre, direction, (.001, .23), 'body-cap', raw['actual_skin'])
                need(skin['selected'] is not None, 'Faktisk hudtreff mangler under skulderkappen')
                measured_skin_radius = skin['selected']['radius_m']
                if 0 < row_index < 4:
                    radius = max(radius, measured_skin_radius + surface_gap)
                point = G.add(centre, G.mul(direction, radius))
                clearance = radius - measured_skin_radius
                raw.update(point_m=point, skin_gap_m=clearance, proposed_radius_m=radius)
                need(clearance >= (.006 if row_index == 0 else .018), 'Skulderkappen bryter målt hudklaring')
                need(radius <= .23 and abs(point[2] - start[2]) <= .18,
                     'Ny geometri gikk utenfor lokal skulderregion')
                vertices.append(point)
        data = oriented(G.panel(vertices, 5, 25), centres)
        ob = mesh('Coat_measured_shoulder_cap_' + side, data, 'M_RW_Coat', cfg['fit']['fabric_thickness_m'])
        side_evidence.update(constructed_surface=G.audit(data), actual_skin_samples=actual_samples,
                             object=ob.name, winding='outward; corner UV permuted with faces if mirrored',
                             inner_seam='actual outward coat hit + 1.5mm, inside unchanged cloth thickness',
                             outer_seam='unchanged measured sleeve ring1 + 1.5mm')
    evidence['status'] = 'REST_CAPS_MEASURED_NOT_SEATED_OR_NATIVE_ACCEPTED'
    return evidence
