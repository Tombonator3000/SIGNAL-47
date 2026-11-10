"""D45-29 (Tom, 10 November): the alien bobblehead he downloaded, for the dashboard of the truck.

Pure Python, no Unreal and no Blender (the system python3 runs it): reads Tom's own download
(default ~/Desktop/alienbubblehead.glb, a Tripo figure: one skinned mesh, 80 bones, no animation,
one 4096 px base colour JPEG) and writes two plain static glTF binaries beside a small manifest:

  alienbubblehead_body.glb   the pedestal, legs, body and neck; origin at the middle of its base
  alienbubblehead_head.glb   the big head; origin at the neck pivot, so turning it is the bobble
  alienbubblehead.json       scale, sizes, the pivot, the counts and the source's sha256

The skin is dropped (its rest pose is its bind pose, checked here), the figure is baked into the
three.js / glTF frame the game uses (metres, +Y up) at 11 cm tall, facing +Z. A triangle goes with
the head when its corners are more than half weighted to the head and neck bones. The texture is
copied as it is (Unreal limits it on import). The output is Tom's private derivative: it never
goes into Git, the repo or the patch (DECISIONS: bought or downloaded assets stay out of Git).

Usage: python3 -I prepare.py [source.glb] [output folder]
       defaults: ~/Desktop/alienbubblehead.glb and
       ~/Documents/ChatGPT/Signal 47/modeller/alienbubblehead-20261110/prepared
Then: import_assets.sh (Unreal editor commandlet, see there).
"""
import hashlib
import json
import math
import os
import struct
import sys

SRC = sys.argv[1] if len(sys.argv) > 1 else os.environ.get('S47_BOBBLE_SRC', os.path.expanduser('~/Desktop/alienbubblehead.glb'))
OUT = sys.argv[2] if len(sys.argv) > 2 else os.environ.get('S47_BOBBLE_OUT', os.path.expanduser('~/Documents/ChatGPT/Signal 47/modeller/alienbubblehead-20261110/prepared'))
# How tall the figure stands on the dash, metres (a dashboard bobblehead: 10 to 12 cm).
HEIGHT = 0.11
# The bones that carry the head: what is weighted to them more than half moves with it.
HEAD_BONES = {'head', 'neck_01', 'neck_02'}
# The pivot between the two neck bones: the spring a bobblehead's head sits on.
PIVOT_BONES = ('neck_01', 'neck_02')
# Must equal Source/Signal47/Private/S47Bobblehead.cpp (HEAD_PIVOT, FIGURE_HEIGHT): checked by import_assets.py too.
EXPECT_PIVOT = (0.0, 0.0436, -0.0017)
EXPECT_TOL = 0.0015


def fail(why):
    raise SystemExit('alienbubblehead prepare FAIL: ' + why)


def read_glb(path):
    b = open(path, 'rb').read()
    magic, version, length = struct.unpack_from('<III', b, 0)
    if magic != 0x46546C67 or version != 2 or length != len(b):
        fail('not a glTF 2 binary: ' + path)
    off, chunks = 12, []
    while off < len(b):
        n, kind = struct.unpack_from('<II', b, off)
        chunks.append((kind, b[off + 8:off + 8 + n]))
        off += 8 + n
    if not chunks or chunks[0][0] != 0x4E4F534A:
        fail('no JSON chunk')
    return json.loads(chunks[0][1]), (chunks[1][1] if len(chunks) > 1 else b''), b


def accessor(g, binary, i):
    a = g['accessors'][i]
    view = g['bufferViews'][a['bufferView']]
    width = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}[a['type']]
    fmt = {5126: 'f', 5125: 'I', 5123: 'H', 5121: 'B'}[a['componentType']]
    size = struct.calcsize(fmt)
    stride = view.get('byteStride', size * width)
    base = view.get('byteOffset', 0) + a.get('byteOffset', 0)
    rows = [struct.unpack_from('<' + fmt * width, binary, base + k * stride) for k in range(a['count'])]
    if a.get('normalized') and fmt in 'HB':
        top = 65535.0 if fmt == 'H' else 255.0
        rows = [tuple(v / top for v in r) for r in rows]
    return rows


IDENTITY = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]


def mul(a, b):  # column-major 4x4
    return [sum(a[r + 4 * k] * b[k + 4 * c] for k in range(4)) for c in range(4) for r in range(4)]


def node_local(n):
    if 'matrix' in n:
        return list(n['matrix'])
    t = n.get('translation', [0, 0, 0])
    x, y, z, w = n.get('rotation', [0, 0, 0, 1])
    s = n.get('scale', [1, 1, 1])
    r = [1 - 2 * (y * y + z * z), 2 * (x * y + z * w), 2 * (x * z - y * w),
         2 * (x * y - z * w), 1 - 2 * (x * x + z * z), 2 * (y * z + x * w),
         2 * (x * z + y * w), 2 * (y * z - x * w), 1 - 2 * (x * x + y * y)]
    return [r[0] * s[0], r[1] * s[0], r[2] * s[0], 0, r[3] * s[1], r[4] * s[1], r[5] * s[1], 0,
            r[6] * s[2], r[7] * s[2], r[8] * s[2], 0, t[0], t[1], t[2], 1]


def point(m, v):
    return tuple(m[r] * v[0] + m[4 + r] * v[1] + m[8 + r] * v[2] + m[12 + r] for r in range(3))


def direction(m, v):
    d = tuple(m[r] * v[0] + m[4 + r] * v[1] + m[8 + r] * v[2] for r in range(3))
    n = math.sqrt(sum(c * c for c in d)) or 1.0
    return tuple(c / n for c in d)


def box(points):
    return [min(p[k] for p in points) for k in range(3)], [max(p[k] for p in points) for k in range(3)]


def write_glb(path, name, positions, normals, uvs, indices, image, image_mime, sampler):
    """One node, one mesh, one material with the base colour picture: nothing else."""
    blobs, views, accessors = [], [], []

    def add(data, target=None):
        while sum(len(x) for x in blobs) % 4:
            blobs.append(b'\0')
        offset = sum(len(x) for x in blobs)
        blobs.append(data)
        v = {'buffer': 0, 'byteOffset': offset, 'byteLength': len(data)}
        if target:
            v['target'] = target
        views.append(v)
        return len(views) - 1

    lo, hi = box(positions)
    accessors.append({'bufferView': add(b''.join(struct.pack('<3f', *p) for p in positions), 34962), 'componentType': 5126,
                      'count': len(positions), 'type': 'VEC3', 'min': lo, 'max': hi})
    accessors.append({'bufferView': add(b''.join(struct.pack('<3f', *n) for n in normals), 34962), 'componentType': 5126,
                      'count': len(normals), 'type': 'VEC3'})
    accessors.append({'bufferView': add(b''.join(struct.pack('<2f', *t) for t in uvs), 34962), 'componentType': 5126,
                      'count': len(uvs), 'type': 'VEC2'})
    accessors.append({'bufferView': add(b''.join(struct.pack('<I', i) for i in indices), 34963), 'componentType': 5125,
                      'count': len(indices), 'type': 'SCALAR'})
    picture = add(image)
    g = {'asset': {'version': '2.0', 'generator': 'SIGNAL47 Tools/alienbubblehead/prepare.py'},
         'scene': 0, 'scenes': [{'nodes': [0]}],
         'nodes': [{'name': name, 'mesh': 0}],
         'meshes': [{'name': name, 'primitives': [{'attributes': {'POSITION': 0, 'NORMAL': 1, 'TEXCOORD_0': 2}, 'indices': 3, 'material': 0, 'mode': 4}]}],
         # (one material name and one picture name for both parts: the second import lands on the first's assets)
         'materials': [{'name': 'alienbubblehead', 'pbrMetallicRoughness': {'baseColorTexture': {'index': 0}, 'metallicFactor': 0.0, 'roughnessFactor': 0.9}}],
         'textures': [{'source': 0, 'sampler': 0}], 'samplers': [sampler],
         'images': [{'name': 'alienbubblehead_basecolor', 'mimeType': image_mime, 'bufferView': picture}],
         'accessors': accessors, 'bufferViews': views}
    binary = b''.join(blobs)
    while len(binary) % 4:
        binary += b'\0'
    g['buffers'] = [{'byteLength': len(binary)}]
    text = json.dumps(g, separators=(',', ':')).encode()
    while len(text) % 4:
        text += b' '
    total = 12 + 8 + len(text) + 8 + len(binary)
    with open(path, 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, total))
        f.write(struct.pack('<II', len(text), 0x4E4F534A) + text)
        f.write(struct.pack('<II', len(binary), 0x004E4942) + binary)


def main():
    g, binary, whole = read_glb(SRC)
    if len(g.get('meshes', [])) != 1 or len(g['meshes'][0]['primitives']) != 1:
        fail('expected one mesh with one primitive (the Tripo figure)')
    parents = {}
    for i, n in enumerate(g['nodes']):
        for c in n.get('children', []):
            parents[c] = i

    def world(i):
        m = node_local(g['nodes'][i])
        while i in parents:
            i = parents[i]
            m = mul(node_local(g['nodes'][i]), m)
        return m

    mesh_node = next(i for i, n in enumerate(g['nodes']) if n.get('mesh') == 0)
    mesh_world = world(mesh_node)
    prim = g['meshes'][0]['primitives'][0]
    pos = accessor(g, binary, prim['attributes']['POSITION'])
    nrm = accessor(g, binary, prim['attributes']['NORMAL'])
    uv = accessor(g, binary, prim['attributes']['TEXCOORD_0'])
    idx = [r[0] for r in accessor(g, binary, prim['indices'])]
    joints = accessor(g, binary, prim['attributes']['JOINTS_0'])
    weights = accessor(g, binary, prim['attributes']['WEIGHTS_0'])
    skin = g['skins'][g['nodes'][mesh_node]['skin']]
    names = [g['nodes'][k].get('name') for k in skin['joints']]
    # The skin's rest pose must be its bind pose: each joint's world times its inverse bind is the mesh node's world.
    ibm = accessor(g, binary, skin['inverseBindMatrices'])
    worst = max(max(abs(a - b) for a, b in zip(mul(world(k), list(ibm[j])), mesh_world)) for j, k in enumerate(skin['joints']))
    if worst > 1e-3:
        fail('the rest pose is not the bind pose (%.4f): the figure would not be the one in the file' % worst)
    P = [point(mesh_world, p) for p in pos]
    N = [direction(mesh_world, n) for n in nrm]
    lo, hi = box(P)
    tall = hi[1] - lo[1]
    if tall <= 0:
        fail('no height')
    scale = HEIGHT / tall
    # the middle of the base: its pedestal, the lowest 2 % of the figure
    base_pts = [p for p in P if p[1] <= lo[1] + 0.02 * tall]
    blo, bhi = box(base_pts)
    origin = ((blo[0] + bhi[0]) / 2, lo[1], (blo[2] + bhi[2]) / 2)
    # facing +Z: the toes stand out in front of the feet
    joint_at = {nm: point(world(k), (0, 0, 0)) for nm, k in zip(names, skin['joints'])}
    if 'ball_l' in joint_at and 'foot_l' in joint_at and joint_at['ball_l'][2] <= joint_at['foot_l'][2]:
        fail('the figure does not face +Z (toes behind the feet)')
    pivot_src = tuple(sum(joint_at[b][k] for b in PIVOT_BONES) / len(PIVOT_BONES) for k in range(3))

    def place(p):
        return tuple((p[k] - origin[k]) * scale for k in range(3))

    pivot = place(pivot_src)
    share = []
    for js, ws in zip(joints, weights):
        total = sum(ws) or 1.0
        share.append(sum(w for j, w in zip(js, ws) if names[j] in HEAD_BONES) / total)
    parts = {'head': [], 'body': []}
    for t in range(0, len(idx), 3):
        tri = idx[t:t + 3]
        parts['head' if sum(share[v] for v in tri) / 3 > 0.5 else 'body'].append(tri)
    image = g['images'][0]
    view = g['bufferViews'][image['bufferView']]
    picture = binary[view.get('byteOffset', 0):view.get('byteOffset', 0) + view['byteLength']]
    sampler = g['samplers'][g['textures'][0].get('sampler', 0)] if g.get('samplers') else {}
    os.makedirs(OUT, exist_ok=True)
    report = {'_doc': 'Made by unreal/Signal47/Tools/alienbubblehead/prepare.py (D45-29). Three.js metres, +Y up, the figure facing +Z. Private derivative of Tom\'s download: never in Git.',
              'source': os.path.abspath(SRC), 'source_sha256': hashlib.sha256(whole).hexdigest(), 'source_bytes': len(whole),
              'source_nodes': [n.get('name') for n in g['nodes'] if n.get('name')][:12], 'bones': len(names), 'animations': len(g.get('animations', [])),
              'source_triangles': len(idx) // 3, 'source_height_units': tall, 'scale': scale, 'height_m': HEIGHT,
              'pivot_m': list(pivot), 'pivot_bones': list(PIVOT_BONES), 'head_bones': sorted(HEAD_BONES), 'parts': {}}
    for part, tris in parts.items():
        remap, positions, normals, uvs, indices = {}, [], [], [], []
        shift = pivot if part == 'head' else (0.0, 0.0, 0.0)
        for tri in tris:
            for v in tri:
                if v not in remap:
                    remap[v] = len(positions)
                    q = place(P[v])
                    positions.append(tuple(q[k] - shift[k] for k in range(3)))
                    normals.append(N[v])
                    uvs.append(uv[v])
                indices.append(remap[v])
        name = 'alienbubblehead_' + part
        write_glb(os.path.join(OUT, name + '.glb'), name, positions, normals, uvs, indices, picture, image.get('mimeType', 'image/jpeg'), sampler)
        plo, phi = box(positions)
        report['parts'][part] = {'file': name + '.glb', 'triangles': len(tris), 'vertices': len(positions),
                                 'min_m': [round(c, 5) for c in plo], 'max_m': [round(c, 5) for c in phi]}
    report['parts']['head']['min_in_figure_m'] = [round(report['parts']['head']['min_m'][k] + pivot[k], 5) for k in range(3)]
    report['parts']['head']['max_in_figure_m'] = [round(report['parts']['head']['max_m'][k] + pivot[k], 5) for k in range(3)]
    json.dump(report, open(os.path.join(OUT, 'alienbubblehead.json'), 'w'), indent=1)
    off = max(abs(pivot[k] - EXPECT_PIVOT[k]) for k in range(3))
    print('alienbubblehead prepare: %d bones, %d triangles -> head %d, body %d; scale %.5f; %.1f cm tall; pivot %s m (game constant %s, off %.4f)'
          % (len(names), len(idx) // 3, len(parts['head']), len(parts['body']), scale, HEIGHT * 100, [round(c, 4) for c in pivot], EXPECT_PIVOT, off))
    print('  body', report['parts']['body']['min_m'], report['parts']['body']['max_m'])
    print('  head (from its pivot)', report['parts']['head']['min_m'], report['parts']['head']['max_m'])
    print('  wrote', OUT)
    if off > EXPECT_TOL:
        fail('the pivot moved from the game constant: change HEAD_PIVOT in S47Bobblehead.cpp and EXPECT_PIVOT here together')


if __name__ == '__main__':
    main()
