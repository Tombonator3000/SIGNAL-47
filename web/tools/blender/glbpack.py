# The .glb as a file: read, write, and the baked mask put in as the occlusion texture. Plain
# Python (tools/blender/build.py runs it after Blender), no Blender needed.
import io, json, struct

def read(path):
    b = open(path, 'rb').read()
    magic, version, total = struct.unpack('<III', b[:12])
    assert magic == 0x46546C67 and version == 2, f'{path} is not a glTF 2 binary'
    n = struct.unpack('<I', b[12:16])[0]
    j = json.loads(b[20:20 + n])
    rest = b[20 + n:]
    binc = rest[8:8 + struct.unpack('<I', rest[:4])[0]] if rest else b''
    return j, bytearray(binc)

def write(path, j, binc):
    js = json.dumps(j, separators=(',', ':')).encode()
    js += b' ' * (-len(js) % 4)
    binc = bytes(binc) + b'\0' * (-len(binc) % 4)
    with open(path, 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(binc)))
        f.write(struct.pack('<II', len(js), 0x4E4F534A)); f.write(js)
        f.write(struct.pack('<II', len(binc), 0x004E4942)); f.write(binc)

def jpeg(png_path, quality=88, mask=True):
    """A picture as a JPEG; a mask without chroma subsampling (4:4:4): its three channels are
    three different masks, and 4:2:0 would smear them into each other at half resolution."""
    from PIL import Image
    out = io.BytesIO()
    Image.open(png_path).convert('RGB').save(out, 'JPEG', quality=quality, subsampling=0 if mask else 2, optimize=True)
    return out.getvalue()

def lightmap_sets(j):
    """Each material's lightmap set: the last of its meshes' UV sets (the first where it is the only one)."""
    sets = {}
    for me in j['meshes']:
        for p in me['primitives']:
            if 'material' in p: sets[p['material']] = 1 if 'TEXCOORD_1' in p['attributes'] else 0
    return sets

def texture(path, data, materials, name, slot='occlusionTexture', tex_coord=None):
    """Puts a picture (JPEG bytes) into the .glb as the named materials' texture in `slot`
    (occlusionTexture, emissiveTexture, or baseColorTexture), on the UV set tex_coord (None: the
    lightmap's). three.js: aoMap, emissiveMap, map, each with its channel."""
    j, binc = read(path)
    binc += b'\0' * (-len(binc) % 4)
    j.setdefault('bufferViews', []).append({'buffer': 0, 'byteOffset': len(binc), 'byteLength': len(data)})
    binc += data
    j['buffers'][0]['byteLength'] = len(binc)
    j.setdefault('images', []).append({'bufferView': len(j['bufferViews']) - 1, 'mimeType': 'image/jpeg', 'name': name})
    j.setdefault('samplers', []).append({'magFilter': 9729, 'minFilter': 9987, 'wrapS': 33071, 'wrapT': 33071})
    j.setdefault('textures', []).append({'source': len(j['images']) - 1, 'sampler': len(j['samplers']) - 1})
    sets = lightmap_sets(j)
    for i, m in enumerate(j.get('materials', [])):
        if m['name'] not in materials: continue
        ref = {'index': len(j['textures']) - 1, 'texCoord': sets.get(i, 1) if tex_coord is None else tex_coord}
        if slot == 'baseColorTexture': m.setdefault('pbrMetallicRoughness', {})[slot] = ref
        else: m[slot] = ref
        if slot == 'emissiveTexture': m['emissiveFactor'] = [1.0, 1.0, 1.0]
    write(path, j, binc)
    return j

def occlusion(path, data, materials, name):
    """The baked mask (R occlusion, G dirt, B worn edges) as the materials' occlusion texture."""
    return texture(path, data, materials, name)

def extras(path, materials, data):
    """What the game needs to know about the bake (the lamps of the house's light) in the named
    materials' extras (three.js: the material's userData)."""
    j, binc = read(path)
    for m in j.get('materials', []):
        if m['name'] in materials: m['extras'] = data
    write(path, j, binc)

def check(path, big=0.25, tol=0.5):
    """The flat faces' shading and their lightmap, read back from the .glb as the game gets it.
    Triangles that lie in one plane and share edges make a flat region; in each region of at
    least `big` square metres and 12 cm across every vertex normal must be within `tol` degrees of the plane's
    (a bent one draws a seam across the wall), and on a wall the lightmap must not be cut inside
    it (one face on two islands). Returns the problems found, as text."""
    import math
    from array import array
    j, binc = read(path)
    TC = {5126: 'f', 5125: 'I', 5123: 'H', 5122: 'h', 5121: 'B', 5120: 'b'}
    NC = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}
    def acc(i):
        a = j['accessors'][i]; bv = j['bufferViews'][a['bufferView']]
        n, t = NC[a['type']], TC[a['componentType']]
        sz = array(t).itemsize; stride = bv.get('byteStride') or n * sz
        start = bv.get('byteOffset', 0) + a.get('byteOffset', 0)
        out = []
        for k in range(a['count']):
            v = array(t); v.frombytes(bytes(binc[start + k * stride:start + k * stride + n * sz]))
            out.append(list(v) if n > 1 else v[0])
        return out
    sets = lightmap_sets(j)
    cos_tol = math.cos(math.radians(tol)); cos_plane = math.cos(math.radians(0.05))
    problems = []
    for me in j['meshes']:
        for p in me['primitives']:
            if 'indices' not in p: continue
            P, N = acc(p['attributes']['POSITION']), acc(p['attributes']['NORMAL'])
            mi = p.get('material'); mname = j['materials'][mi]['name'] if mi is not None else me['name']
            uvk = f"TEXCOORD_{sets.get(mi, 0)}"
            UV = acc(p['attributes'][uvk]) if uvk in p['attributes'] else None
            I = acc(p['indices'])
            key = [tuple(round(c, 4) for c in q) for q in P]
            tris, fn = [], []
            for t in range(0, len(I), 3):
                a, b, c = I[t:t + 3]
                u = [P[b][k] - P[a][k] for k in range(3)]; w = [P[c][k] - P[a][k] for k in range(3)]
                n = [u[1] * w[2] - u[2] * w[1], u[2] * w[0] - u[0] * w[2], u[0] * w[1] - u[1] * w[0]]
                l = math.sqrt(sum(x * x for x in n))
                tris.append((a, b, c)); fn.append(([x / l for x in n], l / 2) if l > 1e-9 else None)
            parent = list(range(len(tris)))
            def find(x):
                while parent[x] != x: parent[x] = parent[parent[x]]; x = parent[x]
                return x
            edges = {}
            for t, (a, b, c) in enumerate(tris):
                if fn[t] is None: continue
                for x, y in ((a, b), (b, c), (c, a)):
                    edges.setdefault(tuple(sorted((key[x], key[y]))), []).append((t, x, y))
            same = lambda s, t: sum(fn[s][0][k] * fn[t][0][k] for k in range(3)) > cos_plane and \
                abs(sum(fn[s][0][k] * (P[tris[s][0]][k] - P[tris[t][0]][k]) for k in range(3))) < 2e-4
            inner = []
            for e, lst in edges.items():
                if len(lst) == 2 and same(lst[0][0], lst[1][0]):
                    parent[find(lst[0][0])] = find(lst[1][0]); inner.append(lst)
            area, big_n, lo, hi = {}, {}, {}, {}
            for t in range(len(tris)):
                if fn[t] is None: continue
                r = find(t); area[r] = area.get(r, 0) + fn[t][1]
                if r not in big_n or fn[t][1] > big_n[r][1]: big_n[r] = fn[t]
                for v in tris[t]:
                    lo[r] = [min(a, b) for a, b in zip(lo.get(r, P[v]), P[v])]; hi[r] = [max(a, b) for a, b in zip(hi.get(r, P[v]), P[v])]
            # a face, not a strip: a bevel's segment or a ladder's rail is long and narrow, and its
            # normals are rounded on purpose (its width: its area over its length)
            for r in list(area):
                if area[r] / max(max(b - a for a, b in zip(lo[r], hi[r])), 1e-6) < 0.12: area[r] = 0.0
                if big_n[r][0][1] < -0.7: area[r] = 0.0      # (what looks down: the undersides, never seen)
            bent, cut = {}, {}
            for t, (a, b, c) in enumerate(tris):
                if fn[t] is None or area[find(t)] < big: continue
                n0 = big_n[find(t)][0]
                for v in (a, b, c):
                    nv = N[v]; l = math.sqrt(sum(x * x for x in nv)) or 1
                    if sum(nv[k] * n0[k] for k in range(3)) / l < cos_tol: bent.setdefault(find(t), P[v])
            if UV is not None:
                for (s, x1, y1), (t, x2, y2) in inner:
                    # (the walls: a ring lying flat, a plinth's top or the skirt, is cut at its
                    # corners on purpose, so the atlas's room inside it is not lost)
                    if area[find(s)] < big or abs(big_n[find(s)][0][1]) > 0.5: continue
                    # the same edge in both triangles: its two ends' coordinates must agree
                    pair = {key[x1]: x1, key[y1]: y1}
                    for x in (x2, y2):
                        o = pair.get(key[x])
                        if o is not None and o != x and max(abs(UV[o][k] - UV[x][k]) for k in range(2)) > 1e-5: cut.setdefault(find(s), P[x])
            for r, at in bent.items():
                problems.append(f"{me['name']}/{mname}: a flat face of {area[r]:.2f} m2 has bent normals (at {[round(c, 3) for c in at]})")
            for r, at in cut.items():
                problems.append(f"{me['name']}/{mname}: a flat face of {area[r]:.2f} m2 lies on two islands of the lightmap (at {[round(c, 3) for c in at]})")
    return problems

def summary(path):
    j, binc = read(path)
    tris = 0
    for m in j['meshes']:
        for p in m['primitives']:
            a = j['accessors'][p['indices']] if 'indices' in p else j['accessors'][p['attributes']['POSITION']]
            tris += a['count'] // 3
    verts = sum(j['accessors'][p['attributes']['POSITION']]['count'] for m in j['meshes'] for p in m['primitives'])
    imgs = sum(j['bufferViews'][i['bufferView']]['byteLength'] for i in j.get('images', []))
    return {'meshes': [m['name'] for m in j['meshes']], 'triangles': tris, 'vertices': verts, 'bytes': len(binc), 'image_bytes': imgs,
            'materials': [m['name'] for m in j.get('materials', [])]}

def quantize(path):
    """KHR_mesh_quantization, which three's GLTFLoader reads without a decoder: the normals as
    bytes (padded to four) and the texture coordinates that stay within 0..1 (the lightmaps) as
    unsigned shorts, both normalised; positions, the tiling coordinates and the rest as they were.
    The binary chunk is laid out again, one view per accessor and per image. Returns the bytes saved."""
    from array import array
    j, binc = read(path)
    views = j['bufferViews']
    TC = {5126: 'f', 5125: 'I', 5123: 'H', 5122: 'h', 5121: 'B', 5120: 'b'}
    NC = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}
    role = {}
    for m in j['meshes']:
        for p in m['primitives']:
            for k, a in p['attributes'].items(): role[a] = k
            if 'indices' in p: role[p['indices']] = 'INDEX'
    out = bytearray(); new_views = []
    def put(b, target=None, stride=None):
        out.extend(b'\0' * (-len(out) % 4))
        v = {'buffer': 0, 'byteOffset': len(out), 'byteLength': len(b)}
        if target: v['target'] = target
        if stride: v['byteStride'] = stride
        out.extend(b); new_views.append(v)
        return len(new_views) - 1
    saved = 0
    for i, acc in enumerate(j['accessors']):
        bv = views[acc['bufferView']]
        n, tc = NC[acc['type']], TC[acc['componentType']]
        a = array(tc)
        assert not bv.get('byteStride') or bv['byteStride'] == n * a.itemsize, 'interleaved data'
        start = bv.get('byteOffset', 0) + acc.get('byteOffset', 0)
        raw = bytes(binc[start:start + acc['count'] * n * a.itemsize])
        a.frombytes(raw)
        r = role.get(i, '')
        acc.pop('byteOffset', None)
        if r == 'NORMAL' and tc == 'f' and n == 3:
            q = array('b', [0]) * (acc['count'] * 4)
            for k in range(acc['count']):
                for c in range(3): q[k * 4 + c] = int(round(max(-1.0, min(1.0, a[k * 3 + c])) * 127))
            acc['bufferView'] = put(q.tobytes(), 34962, 4); acc['componentType'] = 5120; acc['normalized'] = True
        elif r.startswith('TEXCOORD') and tc == 'f' and len(a) and min(a) >= 0 and max(a) <= 1:
            q = array('H', (int(round(v * 65535)) for v in a))
            acc['bufferView'] = put(q.tobytes(), 34962); acc['componentType'] = 5123; acc['normalized'] = True
        else:
            acc['bufferView'] = put(raw, 34963 if r == 'INDEX' else 34962 if r else None)
            continue
        acc.pop('min', None); acc.pop('max', None)
        saved += len(raw) - new_views[-1]['byteLength']
    for im in j.get('images', []):
        bv = views[im['bufferView']]
        im['bufferView'] = put(bytes(binc[bv.get('byteOffset', 0):bv.get('byteOffset', 0) + bv['byteLength']]))
    j['bufferViews'] = new_views
    j['buffers'][0]['byteLength'] = len(out)
    for k in ('extensionsUsed', 'extensionsRequired'):
        j[k] = sorted(set(j.get(k, [])) | {'KHR_mesh_quantization'})
    write(path, j, out)
    return saved
