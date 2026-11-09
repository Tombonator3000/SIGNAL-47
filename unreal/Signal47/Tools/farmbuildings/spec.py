"""Bestillingen i tall, fastsatt før bygging. Meter, Blender-akser, nullpunkt midt i fotavtrykket på bakken.

walls: ytre veggmål x, y. max: største tillatte bounds med takutstikk (xmin, xmax, ymin, ymax, zmax).
tris: øvre grense per LOD. Mottaker er Unreal-importen som beskrevet i README. Stil: New Mexico 1986, bebodd.
"""
SPEC = {
    'barn': {'mesh': 'SM_FarmBarn', 'walls': (16.0, 11.0), 'height': 6.8, 'max': (-8.6, 8.6, -6.6, 6.2, 7.0), 'tris': (32000, 13000, 3500)},
    'house': {'mesh': 'SM_FarmHouse', 'walls': (12.0, 8.0), 'height': 4.9, 'max': (-6.8, 6.6, -4.7, 5.1, 6.9), 'tris': (24000, 10000, 3000)},
    'shed_a': {'mesh': 'SM_FarmShedA', 'walls': (9.0, 5.5), 'height': 3.7, 'max': (-5.2, 5.2, -3.5, 3.4, 3.9), 'tris': (8000, 3500, 1000)},
    'shed_b': {'mesh': 'SM_FarmShedB', 'walls': (5.0, 4.0), 'height': 3.15, 'max': (-3.15, 3.15, -3.0, 2.7, 3.3), 'tris': (8000, 3500, 1000)},
    # WEEKLY-CASE-20261109: skjul B sin dør som egen del, nullpunkt i hengselet (sheds.build_shed_door).
    'shed_door': {'mesh': 'SM_FarmShedDoor', 'walls': (.96, .04), 'height': 1.95, 'max': (-.02, .98, -.1, .03, 1.96), 'tris': (600, 250, 60)},
}
