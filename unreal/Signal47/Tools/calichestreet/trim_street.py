"""Compose the existing gas/radio cuts with the authored souvenir shop.

Original exported geometry is read only. All three cuts are made from the same
source so updating a shared town mesh cannot restore either earlier building.
"""
from __future__ import annotations
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
_spec = importlib.util.spec_from_file_location('s47_trim_web', ROOT / 'Tools/freemodels/trim_web.py')
_helpers = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_helpers)

RADIO_BOX = [19.04, 30.08, 0.0, 8.142166614532471, 2029.90, 2042.10]
SOUVENIR_BOX = [18.20, 29.60, 0.0, 10.0, 2057.70, 2070.30]


def inside(p, boxes):
    return any(b[0] <= p[0] <= b[1] and b[2] <= p[1] <= b[3] and b[4] <= p[2] <= b[5] for b in boxes)


def trim(export: Path, out: Path):
    manifest_bytes = (export / 'manifest.json').read_bytes()
    man = json.loads(manifest_bytes)
    handles = man['handles']
    group = handles['world.town.st']['node']
    gas = json.loads((ROOT / 'Tools/freemodels/models.json').read_text())['gas_station']['trim']
    gas_keep = {handles[k]['mat'] for k in gas.get('keep', []) if k in handles}
    candidates = {'g%d' % n['geo']: n for n in man['nodes'] if n.get('p') == group and n.get('geo') is not None}
    report = {}
    out.mkdir(parents=True, exist_ok=True)
    for old in out.glob('trim_*.glb'):
        old.unlink()
    for source in man['geoFiles']:
        if not any(source['from'] <= n['geo'] < source['to'] for n in candidates.values()):
            continue
        src = export / 'geo' / source['file']
        j, binary = _helpers._load(src)
        binary = bytearray(binary)
        chosen = []
        for mi, mesh in enumerate(j['meshes']):
            name = mesh.get('name')
            if name not in candidates:
                continue
            n = candidates[name]
            preserve_gas = bool(set(n.get('mats', [])) & gas_keep)
            prim_data = []
            original = gas_removed = radio_removed = souvenir_removed = 0
            for primitive in mesh['primitives']:
                pos = _helpers._values(j, binary, primitive['attributes']['POSITION'])
                indices = [v[0] for v in _helpers._values(j, binary, primitive['indices'])]
                kept = []
                original += len(indices) // 3
                for k in range(0, len(indices), 3):
                    tri = indices[k:k + 3]
                    if not preserve_gas and all(inside(pos[v], gas['boxes']) for v in tri):
                        gas_removed += 1
                    elif not preserve_gas and all(inside(pos[v], [RADIO_BOX]) for v in tri):
                        radio_removed += 1
                    elif not preserve_gas and all(inside(pos[v], [SOUVENIR_BOX]) for v in tri):
                        souvenir_removed += 1
                    else:
                        kept.extend(tri)
                prim_data.append((primitive, kept))
            # Unchanged radio/souvenir geometry stays with the existing exporter/Props pipeline.
            if not (radio_removed or souvenir_removed):
                continue
            kept_primitives = []
            for primitive, kept in prim_data:
                if not kept:
                    continue
                while len(binary) % 4:
                    binary += b'\0'
                offset = len(binary)
                binary += struct.pack('<' + 'I' * len(kept), *kept)
                j['bufferViews'].append({'buffer': 0, 'byteOffset': offset, 'byteLength': len(kept) * 4, 'target': 34963})
                a = copy.deepcopy(j['accessors'][primitive['indices']])
                a.update(bufferView=len(j['bufferViews']) - 1, byteOffset=0, componentType=5125,
                         count=len(kept), min=[min(kept)], max=[max(kept)])
                j['accessors'].append(a)
                primitive['indices'] = len(j['accessors']) - 1
                kept_primitives.append(primitive)
            mesh['primitives'] = kept_primitives
            hidden = not kept_primitives
            report[name] = {'tris': original, 'gasRemoved': gas_removed,
                            'radioRemoved': radio_removed, 'souvenirRemoved': souvenir_removed,
                            'resultTris': original - gas_removed - radio_removed - souvenir_removed,
                            'hidden': hidden,
                            'node': n['i'],
                            'asset': '' if hidden else f'/Game/CalicheStreet/Trim/StaticMeshes/{name}.{name}'}
            if not hidden:
                chosen.append(mi)
        if chosen:
            _helpers._write_part(j, binary, chosen, str(out), source['file'])
    if not report or sum(v['souvenirRemoved'] for v in report.values()) < 30:
        raise RuntimeError('Street trim did not find the expected shell; refusing to export stale geometry')
    payload = {'version': 1, 'group': 'world.town.st', 'souvenirBox': SOUVENIR_BOX, 'radioBox': RADIO_BOX,
               'manifestSha256': hashlib.sha256(manifest_bytes).hexdigest(),
               'meshes': report,
               'files': sorted(p.name for p in out.glob('trim_*.glb')),
               'preserved': ['studio', 'onAir', 'amSign', 'amDoor', 'mast', 'souvenir proxies', 'physical A02 figure', 'neighboring buildings'],
               'composedGasTrim': gas}
    (out / 'street-patches.json').write_text(json.dumps(payload, indent=2) + '\n')
    return payload


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('export', type=Path)
    parser.add_argument('out', type=Path)
    args = parser.parse_args()
    result = trim(args.export, args.out)
    print(json.dumps(result, indent=2))
