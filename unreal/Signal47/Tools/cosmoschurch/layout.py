#!/usr/bin/env python3
"""Write Config/Props/church_cosmos.json: where the bought grave markers and the bench stand in San Isidro's cemetery.

Site metres (S47TownSites "cemetery": x east, z south from road x 49, z 2095.5; the plot is x -11 to 11, z -11 to 10, the gravel
path along z 0, the footpath from the north gap along x 3.9). Six lines of graves north to south, each stone facing west to the gate
and the church; the same list every run (a fixed seed). The measures come from verification.json: no picture is read.

    python3 layout.py            (writes the file)
"""
import json
import random
from pathlib import Path

HERE = Path(__file__).resolve().parent
OUT = HERE.parents[1] / 'Config' / 'Props' / 'church_cosmos.json'
MESH = '/Game/Licensed/CosmosChurch/Village/Meshes/%s/%s.%s'
V = json.loads((HERE / 'verification.json').read_text())['meshes']
# kind: (mesh, how many at most, weight). The light meshes carry the rows; the heavy ones (45 000 triangles and up) are few.
KINDS = json.loads((HERE / 'kinds.json').read_text())['kinds']
COLUMNS = [-8.5, -5.3, -2.1, 1.1, 6.4, 9.0]
ROWS = [-9.2, -6.8, -4.4, -2.2, 2.2, 4.6, 7.0]
BENCHES = [('benchEast', 10.3, 5.8, 0), ('benchWest', -10.3, -6.0, 0)]
EMPTY = 9      # plots nobody lies in yet


def box(name):
    s = V[name]['size_cm']
    return [round(s[0] / 100, 2), round(s[1] / 100, 2), round(s[2] / 100, 2)]


def main():
    rnd = random.Random(1986)
    spots = [(x, z) for x in COLUMNS for z in ROWS if not any(abs(x - bx) < 2.0 and abs(z - bz) < 1.8 for _, bx, bz, _ in BENCHES)]
    for s in rnd.sample(spots, EMPTY):
        spots.remove(s)
    left = {k['mesh']: k['most'] for k in KINDS}
    pieces = []
    for i, (x, z) in enumerate(spots):
        pool = [k for k in KINDS if left[k['mesh']] > 0]
        k = rnd.choices(pool, [k['weight'] for k in pool])[0]
        left[k['mesh']] -= 1
        pieces.append({'id': 'grave%02d' % (i + 1), 'asset': MESH % ('Cemetery', k['mesh'], k['mesh']),
                       'at': [round(x + rnd.uniform(-.12, .12) + k.get('east', 0), 2), round(z + rnd.uniform(-.1, .1), 2)],
                       'yaw': round(k['yaw'] + rnd.uniform(-4, 4), 1), 'scale': round(k.get('scale', 1) * rnd.uniform(.94, 1.04), 2), 'collide': True, 'box': box(k['mesh'])})
    for name, x, z, yaw in BENCHES:
        pieces.append({'id': name, 'asset': MESH % ('Props', 'SM_bench', 'SM_bench'), 'at': [x, z], 'yaw': yaw, 'collide': True, 'box': box('SM_bench')})
    doc = ("CHURCH (Tom, 13 November): bought Cosmos grave markers and a bench in San Isidro's cemetery behind the Caliche church (S47TownSites "
           "FDress; the meshes by Tools/cosmoschurch, git-ignored under /Game/Licensed/CosmosChurch, loaded only if present; a grey box of the size "
           "in box [x, y, z metres] stands for each without them). at: the site's metres [x east, z south] from the cemetery's spot (road x 49, "
           "z 2095.5) where the bottom centre of the piece's turned and scaled bounds goes on the land; the plot is x -11 to 11, z -11 to 10, "
           "the gravel path along z 0, the gate at x -11. yaw: Unreal degrees on top of the site's. collide: a walk block from its bounds. "
           "Written by Tools/cosmoschurch/layout.py; hand edits are lost on the next run.")
    lines = ['{', ' "_doc": %s,' % json.dumps(doc), ' "pieces": [']
    lines += ['  ' + json.dumps(p) + (',' if i < len(pieces) - 1 else '') for i, p in enumerate(pieces)]
    lines += [' ]', '}']
    OUT.write_text('\n'.join(lines) + '\n')
    print('S47 COSMOS CHURCH LAYOUT', len(pieces), 'pieces', {k['mesh']: k['most'] - left[k['mesh']] for k in KINDS})


if __name__ == '__main__':
    main()
