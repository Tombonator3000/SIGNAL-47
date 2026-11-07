"""Rask kontroll uten Blender: trekanter, mål, vind. Feiler med kode 1 ved brudd på spec.py."""
import importlib
import sys

from spec import BUDGET, SPEC

sys.dont_write_bytecode = True


def build(name, lod):
    species, variant = name.split('_')
    return importlib.import_module(species).build(variant, lod)


def main(names):
    ok = True
    for name in names:
        spec = SPEC[name]
        rows = []
        for lod in range(3):
            k = build(name, lod)
            lo, hi = k.bounds()
            sx, sy, sz = hi[0] - lo[0], hi[1] - lo[1], hi[2]
            rows.append(f"LOD{lod} {k.tris():5d} tris  {sx:.2f} x {sy:.2f} x {sz:.2f} m  z0={lo[2]:.2f}  vind {min(k.s):.2f}..{max(k.s):.2f}")
            if k.tris() >= BUDGET[lod]:
                ok = False
                rows.append(f'  OVER BUDSJETT {k.tris()} >= {BUDGET[lod]}')
            lim = spec['height']
            if lod == 0 and not lim[0] <= sz <= lim[1]:
                ok = False
                rows.append(f'  HØYDE {sz:.2f} utenfor {lim}')
            if lod == 0 and max(sx, sy) > spec['spread']:
                ok = False
                rows.append(f"  BREDDE {max(sx, sy):.2f} over {spec['spread']}")
            if lod == 0 and 'tuft' in spec:
                # Tuen måles på levende blad: alt unntatt stengel er under 1,3 m.
                tz = max(v[2] for v, u in zip(k.v, k.uv) if u[0] < .3)
                rows.append(f'  tue {tz:.2f} m')
                if not spec['tuft'][0] <= tz <= spec['tuft'][1]:
                    ok = False
                    rows.append(f"  TUE {tz:.2f} utenfor {spec['tuft']}")
            if min(k.s) < 0 or max(k.s) > 1.0:
                ok = False
                rows.append('  VIND utenfor 0..1')
        print(name, spec['mesh'])
        for row in rows:
            print('  ', row)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:] or list(SPEC)))
