"""PROPS-REUSE-20261113: split the gas station's low-poly coffee brewer and night chair out of gas_station_Station into parts of
their own (Brewer, NightChair), so the game hides them behind the real models (gas_cosmos.json "standin") and shows them only
without the bought packs. Run once with python3 -I on Tom's model source, then rebuild and reimport:
  python3 -I gas_station_standins_patch.py /home/tom/Assets/signal47-free/own/gas_station/src
  blender -b -P src/gas_station.py   (when nobody plays)
  bash unreal/Signal47/Tools/freemodels/import_free.sh   (S47_FREE_ONLY=gas_station)
Idempotent: refuses if already applied."""
import os
import sys

src = sys.argv[1]


def rep(s, a, b, n=1):
    assert s.count(a) == n, (a[:90], s.count(a))
    return s.replace(a, b)


p = os.path.join(src, 'gs_interior.py')
s = open(p).read()
assert "Mesh('Brewer')" not in s, 'already applied'
s = rep(s, "IT = Mesh('InsideTubes')\n",
        "IT = Mesh('InsideTubes')\n"
        "BR = Mesh('Brewer')       # PROPS-REUSE-20261113: the old brewer, its own part: the game shows it only without the bought coffee machine\n"
        "CH = Mesh('NightChair')   # PROPS-REUSE-20261113: the old swivel chair, likewise (the real office chair stands in its place)\n")
# the chair
old_chair = '''    y = FLO
    S.box(x - 0.24, x + 0.24, y + 0.42, y + 0.48, z - 0.24, z + 0.24, 'M_Plastic', ('cube', 0.5))
    S.box(x - 0.3, x - 0.25, y + 0.5, y + 1.0, z - 0.22, z + 0.22, 'M_Plastic', ('cube', 0.5))
    S.tube((x, y + 0.06, z), (x, y + 0.42, z), 0.025, 'M_Chrome', seg=8)
    for k in range(5):
        a = 2 * math.pi * k / 5 + 0.3
        S.tube('''
s = rep(s, old_chair, old_chair.replace('    S.', '    CH.').replace('        S.tube(', '        CH.tube('))
# the brewer: its steel body and head, the pot, the coffee in it, the warmer's handle (the cups, sugar, honor box and sign stay)
for line in ("S.box(-21.95, -21.55, ty, ty + 0.45, cz0 + 0.12, IZ1 - 0.02, 'M_Steel', ('cube', 0.5))\n",
             "S.box(-21.95, -21.55, ty + 0.3, ty + 0.45, cz0 + 0.02, cz0 + 0.12, 'M_Steel', ('cube', 0.5))\n",
             "S.tube((-21.75, ty, cz0 + 0.12), (-21.75, ty + 0.18, cz0 + 0.12), 0.075, 'M_GlassClear', seg=14, r1=0.06)\n",
             "S.tube((-21.75, ty, cz0 + 0.12), (-21.75, ty + 0.08, cz0 + 0.12), 0.072, 'M_Coffee', seg=14)\n",
             "S.box(-21.83, -21.67, ty + 0.17, ty + 0.2, cz0 + 0.05, cz0 + 0.2, 'M_Black', ('cube', 0.5))\n"):
    s = rep(s, line, 'BR.' + line[2:])
open(p, 'w').write(s)

p = os.path.join(src, 'gas_station.py')
s = open(p).read()
s = rep(s, "IT.build(MATS)]", "IT.build(MATS), BR.build(MATS), CH.build(MATS)]")
open(p, 'w').write(s)
print('applied: Brewer and NightChair are parts of their own')
