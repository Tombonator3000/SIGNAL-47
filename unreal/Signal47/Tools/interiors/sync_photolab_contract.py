"""Bind the runtime's ten mesh checks to the actually reopened PhotoLab export."""
from pathlib import Path
import hashlib
import json
import re

PROJECT = Path(__file__).resolve().parents[2]
SOURCE = PROJECT.parent / 'ArtSource/PhotoLab'
CPP = PROJECT / 'Source/Signal47/Private/S47PhotoLabDetail.cpp'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    contract_path = SOURCE / 'model_contract.json'
    contract = json.loads(contract_path.read_text())
    proof = json.loads((SOURCE / 'source-verification.json').read_text())
    assert proof['status'] == 'PASS' and proof['contract_sha256'] == digest(contract_path)
    assert proof['source_sha256'] == digest(SOURCE / 'Signal47_PhotoLab_Detail.blend')
    assert len(contract['parts']) == 10 and sum(p['triangles'] for p in contract['parts'].values()) < 80000
    text = CPP.read_text()
    pattern = r'( const FPart Parts\[\] = \{\n)(.*?)(\n \};)'
    matches = list(re.finditer(pattern, text, flags=re.S))
    assert len(matches) == 1
    old = matches[0].group(2)
    assert set(re.findall(r'TEXT\("(SM_PhotoLab_[^"]+)"\)', old)) == set(contract['parts'])
    lines = []
    for name, row in contract['parts'].items():
        lo, hi = row['bounds_local_m']
        center = [(lo[0]+hi[0])*50, -(lo[1]+hi[1])*50, (lo[2]+hi[2])*50]
        extent = [(hi[k]-lo[k])*50 for k in range(3)]
        vec = lambda values: 'FVector(' + ', '.join(f'{v:.6f}' for v in values) + ')'
        lines.append(f'  {{ TEXT("{name}"), {row["triangles"]}, {len(row["materials"])}, {vec(center)}, {vec(extent)} }}')
    updated = text[:matches[0].start(2)] + ',\n'.join(lines) + text[matches[0].end(2):]
    CPP.write_text(updated)
    print('S47 PHOTOLAB RUNTIME CONTRACT SYNCHRONIZED', digest(contract_path))


if __name__ == '__main__':
    main()
