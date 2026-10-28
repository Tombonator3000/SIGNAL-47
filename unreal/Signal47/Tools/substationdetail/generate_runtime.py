#!/usr/bin/env python3
"""Generate the native bounds/LOD contract and exact shoulder walk triangles.

Run after build_detail.py. --check compares bytes without changing the runtime
file; ordinary invocation writes it. No Blender or Unreal process is required.
"""
import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent
CONTRACT = ROOT / 'model_contract.json'
OUTPUT = ROOT.parents[1] / 'Source/Signal47/Private/S47SubstationDetailData.inl'


def generate(source):
    contract = json.loads(source)
    lines = [
        '// Generated from Tools/substationdetail/model_contract.json; do not hand-edit.',
        '// SHA256 ' + hashlib.sha256(source).hexdigest(),
        'const FPart Parts[] = {',
    ]
    # JSON part order is the authored native installation order. Native local
    # coordinates are (B.x, -B.y, B.z) in centimetres, with site placement later.
    for name, part in contract['parts'].items():
        low, high = part['bounds_local_m']
        center = [(low[i] + high[i]) * 50 for i in range(3)]
        center[1] = -center[1]
        extent = [(high[i] - low[i]) * 50 for i in range(3)]
        vector = lambda values: 'FVector(' + ', '.join(format(v, '.12g') for v in values) + ')'
        lods = ', '.join(str(v) for v in part['lod_triangles'])
        materials = ', '.join('TEXT("' + v + '")' for v in part['materials'])
        shoulder = 'true' if name == 'SM_SubstationDetail_GroundShoulder' else 'false'
        lines.append(
            ' { TEXT("' + name + '"), { ' + lods + ' }, { ' + materials + ' }, '
            + str(len(part['materials'])) + ', ' + vector(center) + ', '
            + vector(extent) + ', ' + shoulder + ' },'
        )
    lines.extend(['};', 'const FFloorTriangle ShoulderTriangles[] = {'])
    # Preserve exact float source values as round-trippable double literals;
    # these remain in SITE B metres for the barycentric surface lookup.
    for triangle in contract['shoulder_floor_triangles_b']:
        vectors = ['FVector(' + ', '.join(format(v, '.17g') for v in point) + ')'
                   for point in triangle]
        lines.append(' { ' + ', '.join(vectors) + ' },')
    lines.append('};')
    return ('\n'.join(lines) + '\n').encode('utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Compare bytes; never write output')
    args = parser.parse_args()
    expected = generate(CONTRACT.read_bytes())
    if args.check:
        if not OUTPUT.is_file() or OUTPUT.read_bytes() != expected:
            raise SystemExit('FAIL: runtime data differs from the authored contract: ' + str(OUTPUT))
        print('PASS: runtime data matches contract byte-for-byte; ' + str(len(expected)) + ' bytes')
    else:
        OUTPUT.write_bytes(expected)
        print('Wrote ' + str(OUTPUT))


if __name__ == '__main__':
    main()
