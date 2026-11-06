"""Rask kontroll uten Blender: trekanter per LOD, mål, materialspor og budsjett."""
import importlib
import sys

from fbkit import triangles, mats_used

from spec import SPEC

BUDGET = {k: v['tris'] for k, v in SPEC.items()}


def load(name):
    if name.startswith('shed'):
        mod = importlib.import_module('sheds')
        return lambda lod: getattr(mod, 'build_' + name)(lod)
    return importlib.import_module(name).build


def main(names):
    ok = True
    for name in names:
        rows = []
        slots = None
        for lod in range(3):
            k = load(name)(lod)
            xs, ys, zs = zip(*k.v)
            tri = triangles(k)
            rows.append(tri)
            ms = mats_used(k)
            if slots is None:
                slots = ms
            if ms != slots:
                ok = False
                print('SLOTFEIL', name, lod, sorted(set(slots) ^ set(ms)))
            if tri > BUDGET[name][lod]:
                ok = False
                print('OVER BUDSJETT', name, lod, tri, BUDGET[name][lod])
            lim = SPEC[name]['max']
            if min(xs) < lim[0] or max(xs) > lim[1] or min(ys) < lim[2] or max(ys) > lim[3] or max(zs) > lim[4]:
                ok = False
                print('UTENFOR MÅL', name, lod, lim)
            print(f'{name} LOD{lod} tris={tri} x {min(xs):.2f}..{max(xs):.2f} y {min(ys):.2f}..{max(ys):.2f} z {min(zs):.2f}..{max(zs):.2f}')
        print(' slots', [m.replace('M_Farm_', '') for m in slots])
    print('PASS' if ok else 'FAIL')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:] or list(BUDGET)))
