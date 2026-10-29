#!/usr/bin/env python3
"""Generate S47SubstationFieldData.inl (part LOD triangle counts, retired T02 cube names).
--check compares bytes. No Blender or Unreal process is required."""
import argparse, hashlib, json
from pathlib import Path
ROOT = Path(__file__).resolve().parent
CONTRACT = ROOT / 'model_contract.json'
OUTPUT = ROOT.parents[1] / 'Source/Signal47/Private/S47SubstationFieldData.inl'
# Earlier engine-cube T02 meshes now represented by the kit. They stay as hidden proxies.
RETIRED = ['T02CulvertTop', 'T02CulvertLeft', 'T02CulvertRight', 'T02CulvertInterior', 'T02BandReferencePost',
           'T02PoleWhiteBand1', 'T02PoleWhiteBand2', 'T02ReferenceFootMark', 'T02ReferenceMarker', 'T02EnclosedFixture']


def generate(source):
    contract = json.loads(source)
    lines = ['// Generated from Tools/substationfield/model_contract.json; do not hand-edit.',
             '// SHA256 ' + hashlib.sha256(source).hexdigest(), 'const FFieldPart FieldPartsData[] = {']
    for name, part in contract['parts'].items():
        lines.append(' { TEXT("' + name + '"), { ' + ', '.join(str(v) for v in part['lod_triangles']) + ' } },')
    lines += ['};', 'const TCHAR* const FieldRetired[] = { ' + ', '.join('TEXT("' + n + '")' for n in RETIRED) + ' };']
    return ('\n'.join(lines) + '\n').encode()


def main():
    a = argparse.ArgumentParser(description=__doc__); a.add_argument('--check', action='store_true'); args = a.parse_args()
    data = generate(CONTRACT.read_bytes())
    if args.check:
        if not OUTPUT.is_file() or OUTPUT.read_bytes() != data: raise SystemExit('FAIL: runtime data differs from contract')
        print('PASS: runtime data matches contract')
    else:
        OUTPUT.write_bytes(data); print('Wrote', OUTPUT)


if __name__ == '__main__':
    main()
