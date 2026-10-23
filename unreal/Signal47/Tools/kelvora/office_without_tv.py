"""Remove the old procedural TV from the office's merged, original geometry.

Run with Python 3, export folder as argv[1]. No web sources or original GLB are changed.
Only triangles whose three vertices are inside the old set's local bounds are removed.
The shelf's larger triangles and the rest of the office remain intact.
"""
import copy
import json
import math
from pathlib import Path
import struct
import sys

src = Path(sys.argv[1])
out = Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
raw = (src / 'geo/geo_004.glb').read_bytes()
size = struct.unpack_from('<I', raw, 12)[0]
j = json.loads(raw[20:20 + size])
binary = raw[28 + size:]

def values(index):
    a = j['accessors'][index]
    v = j['bufferViews'][a['bufferView']]
    fmt = {5121: 'B', 5123: 'H', 5125: 'I', 5126: 'f'}[a['componentType']]
    width = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
    stride = v.get('byteStride', struct.calcsize(fmt) * width)
    start = v.get('byteOffset', 0) + a.get('byteOffset', 0)
    return [struct.unpack_from('<' + fmt * width, binary, start + n * stride) for n in range(a['count'])]

def inside(p):
    x, y, z = p[0] + 45.27, p[1] - 1.5, p[2] - 26.85
    c, s = math.cos(2.4), math.sin(2.4)
    x, z = c * x - s * z, s * x + c * z
    return -.225 <= x <= .225 and -.006 <= y <= .68 and -.185 <= z <= .205

report = {}
for mesh in j['meshes']:
    removed = 0
    for prim in mesh['primitives']:
        pos = values(prim['attributes']['POSITION'])
        indices = [v[0] for v in values(prim['indices'])]
        keep = []
        for n in range(0, len(indices), 3):
            tri = indices[n:n + 3]
            if all(inside(pos[k]) for k in tri):
                removed += 1
            else:
                keep.extend(tri)
        if len(keep) == len(indices):
            continue
        if not keep:
            # The picture is its own complete mesh, hidden at runtime by material id.
            report[mesh['name']] = {'hidden': True, 'removed': removed}
            continue
        while len(binary) % 4:
            binary += b'\0'
        offset = len(binary)
        binary += struct.pack('<' + 'I' * len(keep), *keep)
        view = len(j['bufferViews'])
        j['bufferViews'].append({'buffer': 0, 'byteOffset': offset, 'byteLength': len(keep) * 4, 'target': 34963})
        acc = copy.deepcopy(j['accessors'][prim['indices']])
        acc.update(bufferView=view, byteOffset=0, componentType=5125, count=len(keep), min=[min(keep)], max=[max(keep)])
        prim['indices'] = len(j['accessors'])
        j['accessors'].append(acc)
    if removed and mesh['name'] not in report:
        report[mesh['name']] = {'hidden': False, 'removed': removed}

selected = [m for m in j['meshes'] if m['name'] in report and not report[m['name']]['hidden']]
assert selected and sum(v['removed'] for v in report.values()) > 100, report
j['meshes'] = selected
j['nodes'] = [{'name': m['name'], 'mesh': i} for i, m in enumerate(selected)]
j['scenes'] = [{'nodes': list(range(len(selected)))}]
j['scene'] = 0
j['buffers'][0]['byteLength'] = len(binary)
data = json.dumps(j, separators=(',', ':')).encode()
data += b' ' * (-len(data) % 4)
binary += b'\0' * (-len(binary) % 4)
glb = struct.pack('<III', 0x46546c67, 2, 28 + len(data) + len(binary))
glb += struct.pack('<II', len(data), 0x4e4f534a) + data
glb += struct.pack('<II', len(binary), 0x004e4942) + binary
(out / 'OfficeWithoutTV.glb').write_bytes(glb)
(out / 'office-patches.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report))
