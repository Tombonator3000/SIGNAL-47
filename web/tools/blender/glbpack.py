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
    named materials' occlusion texture on TEXCOORD_1 (three.js: aoMap, channel 1)."""
    j, binc = read(path)
    binc += b'\0' * (-len(binc) % 4)
    j.setdefault('bufferViews', []).append({'buffer': 0, 'byteOffset': len(binc), 'byteLength': len(data)})
    binc += data
    j['buffers'][0]['byteLength'] = len(binc)
    j.setdefault('images', []).append({'bufferView': len(j['bufferViews']) - 1, 'mimeType': 'image/jpeg', 'name': name})
    j.setdefault('samplers', []).append({'magFilter': 9729, 'minFilter': 9987, 'wrapS': 33071, 'wrapT': 33071})
    j.setdefault('textures', []).append({'source': len(j['images']) - 1, 'sampler': len(j['samplers']) - 1})
    for m in j.get('materials', []):
        if m['name'] in materials: m['occlusionTexture'] = {'index': len(j['textures']) - 1, 'texCoord': 1}
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
