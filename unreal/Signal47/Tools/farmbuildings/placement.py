"""Plassering i spillets koordinater og hindringer for gange/kjøring. Ingen Blender.

  python3 placement.py <stats-mappe>   skriver placement.json der

Blender (x, y, z) med y nord. Three/road-lokal: X = ax + x cos t - y sin t, Z = az - x sin t - y cos t,
Y = ay + z, der t er three-rotasjonen rundt Y i radianer (samme 0,12 som kildens shed()).
Spillets vegger er aksejusterte bokser (FS47Obstacle::Box), så skrå vegger blir korte trappetrinn.
"""
import json
import math
import sys
from pathlib import Path

# anchor_x, anchor_z er road-lokale meter i Freeform. Story er +310 på x for huset og låven.
SITES = {
    'barn': {'mesh': 'SM_FarmBarn', 'anchor': (136.0, 170.0), 'yaw': .12, 'story_anchor': (446.0, 170.0),
             'replaces': 'g912 boks 12 tris (låvevegg) og g911 to takbokser 24 tris (bounds i kilde x 437,14..454,86 z 163,58..176,42)',
             'old_obstacle': 'Box(126,146,164,177)'},
    # Huset er flyttet 2 m nord fra dagens 192, så portalen ikke står i veien (veikant z 195,5).
    'house': {'mesh': 'SM_FarmHouse', 'anchor': (108.0, 190.0), 'yaw': .12, 'story_anchor': (418.0, 192.0),
              'replaces': 'g910 boks 12 tris (husvegg) og g911 to takbokser 24 tris (bounds i kilde x 411,28..424,72 z 187,31..196,69)',
              'old_obstacle': 'Box(101,115,187,197)'},
    'shed_a': {'mesh': 'SM_FarmShedA', 'anchor': (116.5, 170.0), 'yaw': .12, 'story_anchor': None, 'replaces': 'ingenting, nytt forslag',
               'old_obstacle': None},
    'shed_b': {'mesh': 'SM_FarmShedB', 'anchor': (84.5, 206.5), 'yaw': 0.0, 'story_anchor': None, 'replaces': 'ingenting, nytt forslag',
               'old_obstacle': None},
}


def to_road(site, x, y):
    ax, az = site['anchor']
    t = site['yaw']
    return ax + x * math.cos(t) - y * math.sin(t), az - x * math.sin(t) - y * math.cos(t)


def chain(site, rect, step=1.0):
    """Lokalt rektangel som aksejusterte road-bokser. Lange skrå vegger deles i biter."""
    x0, x1, y0, y1 = rect
    out = []
    if not site['yaw']:
        a, b = to_road(site, x0, y0), to_road(site, x1, y1)
        return [[round(min(a[0], b[0]), 3), round(max(a[0], b[0]), 3), round(min(a[1], b[1]), 3), round(max(a[1], b[1]), 3)]]
    long_x = x1 - x0 >= y1 - y0
    length = max(x1 - x0, y1 - y0)
    n = max(1, math.ceil(length / step))
    for i in range(n):
        if long_x:
            r = (x0 + (x1 - x0) * i / n, x0 + (x1 - x0) * (i + 1) / n, y0, y1)
        else:
            r = (x0, x1, y0 + (y1 - y0) * i / n, y0 + (y1 - y0) * (i + 1) / n)
        pts = [to_road(site, px, py) for px in r[:2] for py in r[2:]]
        out.append([round(min(p[0] for p in pts), 3), round(max(p[0] for p in pts), 3),
                    round(min(p[1] for p in pts), 3), round(max(p[1] for p in pts), 3)])
    return out


def main(root):
    out = {}
    for name, site in SITES.items():
        stats_path = root / f'stats_{name}.json'
        stats = json.loads(stats_path.read_text()) if stats_path.exists() else None
        row = dict(site)
        row['ue_rotation_yaw_deg'] = round(-math.degrees(site['yaw']), 3)
        row['road_three_position'] = {'x': site['anchor'][0], 'y': f"S47RoadTerrain::SurfaceY({site['anchor'][0]},{site['anchor'][1]})", 'z': site['anchor'][1]}
        if stats:
            lo, hi = stats['lods'][0]['bounds']
            corners = [to_road(site, x, y) for x in (lo[0], hi[0]) for y in (lo[1], hi[1])]
            row['road_bounds_incl_overhang'] = [round(min(c[0] for c in corners), 2), round(max(c[0] for c in corners), 2),
                                               round(min(c[1] for c in corners), 2), round(max(c[1] for c in corners), 2)]
            row['triangles'] = [l['triangles'] for l in stats['lods']]
            row['ucx'] = len(stats['ucx'])
            row['obstacles_road_local'] = [b for r in stats['solids_local'] for b in chain(site, r)]
        out[name] = row
    (root / 'placement.json').write_text(json.dumps(out, indent=1, ensure_ascii=False) + '\n')
    return out


def cpp_rows():
    """Radene til S47FarmBuildings.cpp (MinX, MaxX, MinZ, MaxZ), fra placement.json ved siden av dette skriptet."""
    data = json.loads((Path(__file__).resolve().parent / 'placement.json').read_text())
    for name, row in data.items():
        print(f' // {name}: {len(row["obstacles_road_local"])}')
        boxes = row['obstacles_road_local']
        for i in range(0, len(boxes), 4):
            print(' ' + ''.join('{%s,%s,%s,%s},' % tuple(b) for b in boxes[i:i + 4]))


if __name__ == '__main__':
    if sys.argv[1:] == ['--cpp']:
        cpp_rows()
        sys.exit()
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.home() / 'Documents/ChatGPT/Signal 47/modeller/farmbuildings-2026-11-07'
    for k, v in main(root).items():
        print(k, v.get('triangles'), v.get('ucx'), v.get('road_bounds_incl_overhang'), len(v.get('obstacles_road_local', [])))
