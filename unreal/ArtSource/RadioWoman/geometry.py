"""Egen ring-/panelgeometri uten Blender eller lisensierte kroppspunkter i kilden."""
import math


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def mul(a, value):
    return tuple(x * value for x in a)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def length(a):
    return math.sqrt(dot(a, a))


def unit(a):
    size = length(a)
    if size < 1e-9:
        raise ValueError('Retning uten lengde')
    return mul(a, 1/size)


def signed_power(value, power):
    return math.copysign(abs(value)**power, value)


def loft(rows, samples=48, closed=True, cap=False, exponent=.82):
    """Rader følger aksen u×v; radius består av rx, front og back.

    Åpen front starter/slutter rundt -v. Gap kan variere per rad, slik at
    frakken får en faktisk åpning under de nederste knappene. UV ligger per
    løkke, også over sømmen i en lukket ring.
    """
    vertices, faces, uvs = [], [], []
    count = samples if closed else samples+1
    if len(rows) < 2 or samples < 8:
        raise ValueError('Loft krever minst to rader og åtte omkretspunkter')
    for a, b in zip(rows, rows[1:]):
        direction = sub(b['centre'], a['centre'])
        for row in (a, b):
            if dot(cross(row.get('u', (1, 0, 0)), row.get('v', (0, 1, 0))), direction) <= 0:
                raise ValueError('Radbasis snur geometrinormalene innover')
    for row in rows:
        centre, u, v = row['centre'], row.get('u', (1, 0, 0)), row.get('v', (0, 1, 0))
        if min(row['rx'], row['front'], row['back']) <= 0:
            raise ValueError('Ringradius må være positiv')
        gap = 0 if closed else row.get('gap', .04)
        for i in range(count):
            a = -math.pi/2+gap + (math.tau-2*gap)*i/samples
            c, s = math.cos(a), math.sin(a)
            x = signed_power(c, exponent)*row['rx']
            y = signed_power(s, exponent)*(row['front'] if s < 0 else row['back'])
            vertices.append(add(centre, add(mul(u, x), mul(v, y))))
    for j in range(len(rows)-1):
        for i in range(samples):
            k = (i+1) % count
            faces.append((j*count+i, j*count+k, (j+1)*count+k, (j+1)*count+i))
            uvs.append(((i/samples, j/(len(rows)-1)), ((i+1)/samples, j/(len(rows)-1)),
                        ((i+1)/samples, (j+1)/(len(rows)-1)), (i/samples, (j+1)/(len(rows)-1))))
    if cap:
        if not closed:
            raise ValueError('Frontåpen flate kan ikke ringlukkes')
        faces += [tuple(reversed(range(count))), tuple((len(rows)-1)*count+i for i in range(count))]
        for face in faces[-2:]:
            uvs.append(tuple((.5+.45*math.cos(math.tau*(i % count)/count),
                              .5+.45*math.sin(math.tau*(i % count)/count)) for i in face))
    return {'vertices': vertices, 'faces': faces, 'uvs': uvs}


def panel(vertices, rows, columns):
    faces, uvs = [], []
    for j in range(rows-1):
        for i in range(columns-1):
            faces.append((j*columns+i, j*columns+i+1, (j+1)*columns+i+1, (j+1)*columns+i))
            uvs.append(((i/(columns-1), j/(rows-1)), ((i+1)/(columns-1), j/(rows-1)),
                        ((i+1)/(columns-1), (j+1)/(rows-1)), (i/(columns-1), (j+1)/(rows-1))))
    return {'vertices': vertices, 'faces': faces, 'uvs': uvs}


def audit(data):
    vertices, faces, uvs = data['vertices'], data['faces'], data['uvs']
    if not vertices or not faces or len(faces) != len(uvs):
        raise ValueError('Tom geometri eller avvikende UV-inventar')
    if not all(len(p) == 3 and all(math.isfinite(c) for c in p) for p in vertices):
        raise ValueError('Ugyldige posisjoner')
    for face, uv in zip(faces, uvs):
        if len(face) < 3 or len(set(face)) != len(face) or len(face) != len(uv):
            raise ValueError('Ugyldig flate/løkke')
        if any(i < 0 or i >= len(vertices) for i in face):
            raise ValueError('Indeks utenfor geometri')
        if any(not all(math.isfinite(v) for v in point) for point in uv):
            raise ValueError('Ugyldig UV')
        area = sum(length(cross(sub(vertices[face[i]], vertices[face[0]]),
                                sub(vertices[face[i+1]], vertices[face[0]])))
                   for i in range(1, len(face)-1))
        if area < 1e-12:
            raise ValueError('Degenerert flate')
    return {'vertices': len(vertices), 'triangles': sum(len(f)-2 for f in faces)}


def check_geometry():
    rows = [{'centre': (0, 0, z), 'rx': .2, 'front': .15, 'back': .14} for z in (0., .4, 1.)]
    cylinder = loft(rows, 32, cap=True)
    audit(cylinder)
    volume = 0
    edges = {}
    for face in cylinder['faces']:
        for i in range(1, len(face)-1):
            a, b, c = (cylinder['vertices'][face[k]] for k in (0, i, i+1))
            volume += dot(a, cross(b, c))/6
        for a, b in zip(face, face[1:]+face[:1]):
            key = tuple(sorted((a, b)))
            edges[key] = edges.get(key, 0)+1
    if volume <= 0 or set(edges.values()) != {2}:
        raise ValueError('Lukket loft har feil winding eller åpne kanter')
    # UV-løkker på hver side av samme posisjon har 0/1, ikke en stripe over atlaset.
    if cylinder['uvs'][0][0][0] != 0 or cylinder['uvs'][31][1][0] != 1:
        raise ValueError('UV-søm mistet egne løkkekoordinater')
    # En faktisk åpen frakkfront skal ikke få en flate over frontgapet.
    opened = loft([dict(r, gap=.1) for r in rows], 32, closed=False)
    audit(opened)
    if any(0 in f and 32 in f for f in opened['faces']):
        raise ValueError('Frakkfront ble lukket')
    bad = dict(cylinder, faces=[(0, 0, 1)], uvs=[((0, 0),)*3])
    try:
        audit(bad)
    except ValueError:
        pass
    else:
        raise ValueError('Ugyldig geometri ble godtatt')
    try:
        loft(list(reversed(rows)), 32)
    except ValueError:
        return {'closed_volume_positive': True, 'closed_edges_two_faces': True,
                'uv_seam_loop_coordinates': True, 'open_front_preserved': True,
                'degenerate_face_rejected': True, 'reversed_basis_rejected': True}
    raise ValueError('Innovervendt radbasis ble godtatt')
