"""Own house material names, reusing the original Town PBR files read-only."""
import copy
import importlib.util
from pathlib import Path

TOWN = Path(__file__).resolve().parents[1] / 'townbuildings'
SPEC = importlib.util.spec_from_file_location('s47_homes_town_materials', TOWN / 'materials.py')
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)


def material_rows():
    originals = {row['name'].removeprefix('M_Town_'): row for row in BASE.material_rows()}
    result = []

    def take(label, source, rgb=None, roughness=None):
        row = copy.deepcopy(originals[source])
        row['name'] = 'M_Homes_' + label
        row['source_material'] = 'M_Town_' + source
        if rgb is not None:
            tint = [*BASE.linear(rgb), 1]
            row['base_color_linear'] = tint
            if row.get('textures'):
                row['texture_tint_linear'] = tint
        if roughness is not None:
            row['roughness'] = roughness
        row['textures'] = {role: str((TOWN / name).resolve()) for role, name in row.get('textures', {}).items()}
        result.append(row)

    # Original Town.ts tan/cream sRGB values are preserved exactly on the walls.
    take('TanStucco', 'Stucco', [185, 143, 104], 1.)
    take('CreamStucco', 'Stucco', [220, 201, 166], .95)
    take('TanWear', 'Stucco', [169, 132, 100], 1.)
    take('CreamWear', 'Stucco', [198, 182, 151], 1.)
    take('DoorMasonry', 'Stucco', [203, 188, 158], .95)
    take('Concrete', 'Concrete')
    take('WarmTimber', 'Timber')
    take('DarkTimber', 'DarkWood')
    take('SagePaint', 'TealPaint', [123, 145, 124], .82)
    take('BluePaint', 'BluePaint', [116, 138, 149], .83)
    take('RoofTar', 'Concrete', [54, 51, 45], .98)
    take('Galvanized', 'Metal')
    take('DarkMetal', 'DarkMetal')
    take('RustTin', 'RustTin')
    take('RecessShadow', 'DarkWood', [36, 33, 28], 1.)
    return result
