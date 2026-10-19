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

def jpeg(png_path, quality=88):
    """The mask as a JPEG without chroma subsampling (4:4:4): its three channels are three
    different masks, and 4:2:0 would smear them into each other at half resolution."""
    from PIL import Image
    out = io.BytesIO()
    Image.open(png_path).convert('RGB').save(out, 'JPEG', quality=quality, subsampling=0, optimize=True)
    return out.getvalue()

def occlusion(path, data, materials, name):
    """Puts the baked mask (R occlusion, G dirt, B worn edges; JPEG bytes) into the .glb as the
    named materials' occlusion texture on their lightmap's set (three.js: aoMap and its channel)."""
    j, binc = read(path)
    binc += b'\0' * (-len(binc) % 4)
    j.setdefault('bufferViews', []).append({'buffer': 0, 'byteOffset': len(binc), 'byteLength': len(data)})
    binc += data
    j['buffers'][0]['byteLength'] = len(binc)
    j.setdefault('images', []).append({'bufferView': len(j['bufferViews']) - 1, 'mimeType': 'image/jpeg', 'name': name})
    j.setdefault('samplers', []).append({'magFilter': 9729, 'minFilter': 9987, 'wrapS': 33071, 'wrapT': 33071})
    j.setdefault('textures', []).append({'source': len(j['images']) - 1, 'sampler': len(j['samplers']) - 1})
    # on the meshes' last UV set: the lightmap (the first set where it is the only one)
    sets = {}
    for me in j['meshes']:
        for p in me['primitives']:
            if 'material' in p: sets[p['material']] = 1 if 'TEXCOORD_1' in p['attributes'] else 0
    for i, m in enumerate(j.get('materials', [])):
        if m['name'] in materials: m['occlusionTexture'] = {'index': len(j['textures']) - 1, 'texCoord': sets.get(i, 1)}
    write(path, j, binc)
    return j

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
