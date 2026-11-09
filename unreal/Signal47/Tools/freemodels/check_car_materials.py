"""Reads only (SHERIFF-CAR, sweep of 9 November 2026): what the town's cars were imported with, for the grey clay cars Tom saw in the
packed game. Nothing is changed or saved.

For every car mesh S47CarPaint dresses (/Game/Free/Vehicles, SARO's parked sedan and pickup in /Game/Free/ServiceFleet): Nanite and
LODs, and for each slot its material, the chain of parents up to the base material, whether each parent's package exists (the glTF
instances' parents are the engine plugin's /InterchangeAssets/gltf/...), and whether the usages the mesh needs are set (Nanite when
the mesh is Nanite, instanced static meshes as the free props are drawn): a cooked game cannot add a usage, and a slot without it is
drawn with the engine's default (grey with grid lines). With S47_CARMAT_STATS=1 each base material's statistics are asked for too,
which compiles its shaders in this editor (slow; still nothing saved).

Run with no editor of this project open, no other heavy job, through the reservation:
  UnrealEditor-Cmd Signal47.uproject -run=pythonscript -script=Tools/freemodels/check_car_materials.py -unattended -nullrhi -nosplash
Output: one line per slot starting S47CARMAT, and Saved/S47CarMaterials.json.
"""
import json
import os
from pathlib import Path
import sys
import unreal

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from material_usage import ensure_usage, required_usage  # noqa: E402  (apply=False below: reads only)

E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary
ROOTS = ['/Game/Free/Vehicles', '/Game/Free/ServiceFleet']
STATS = os.environ.get('S47_CARMAT_STATS') == '1'


def car_meshes():
    for root in ROOTS:
        for path in E.list_assets(root, recursive=True, include_folder=False):
            name = path.split('.')[-1]
            if root.endswith('ServiceFleet') and not name.startswith('SM_SaroPark_'):
                continue
            if '/StaticMeshes/' not in path and not name.startswith('SM_SaroPark_'):
                continue
            asset = E.load_asset(path)
            if isinstance(asset, unreal.StaticMesh):
                yield asset


def chain(material):
    rows = []
    m = material
    while m is not None and isinstance(m, unreal.MaterialInstance):
        parent = m.get_editor_property('parent')
        package = parent.get_path_name().split('.')[0] if parent else ''
        rows.append({'parent': parent.get_path_name() if parent else None,
                     'exists': bool(parent) and E.does_asset_exist(package)})
        m = parent
    return rows


def main():
    out = []
    for mesh in car_meshes():
        nanite = mesh.get_editor_property('nanite_settings').enabled
        lods = mesh.get_num_lods()
        for i, slot in enumerate(mesh.get_editor_property('static_materials')):
            mat = slot.material_interface
            row = {'mesh': mesh.get_path_name(), 'nanite': nanite, 'lods': lods, 'slot': i, 'slot_name': str(slot.material_slot_name),
                   'material': mat.get_path_name() if mat else None, 'class': mat.get_class().get_name() if mat else None}
            if mat:
                row['parents'] = chain(mat)
                base = mat.get_base_material()
                row['base'] = base.get_path_name() if base else None
                usage = []
                for instanced in (False, True):
                    usage += ensure_usage(mat, required_usage(mesh, instanced), apply=False)
                row['usage'] = {r['usage']: r['before'] for r in usage}
                if STATS and base:
                    s = M.get_statistics(base)
                    row['stats'] = {'vs': s.num_vertex_shader_instructions, 'ps': s.num_pixel_shader_instructions}
            missing = [p['parent'] for p in row.get('parents', []) if not p['exists']]
            unset = [k for k, v in row.get('usage', {}).items() if not v]
            unreal.log('S47CARMAT %s slot %d %s: %s; base %s; parents missing %s; usage not set %s%s' % (
                mesh.get_name(), i, row['slot_name'], row['material'], row.get('base'), missing or 'none', unset or 'none',
                ('; stats %s' % row['stats']) if 'stats' in row else ''))
            out.append(row)
    target = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / 'S47CarMaterials.json'
    target.write_text(json.dumps(out, indent=1))
    unreal.log('S47CARMAT %d slots on %d meshes; %s' % (len(out), len({r['mesh'] for r in out}), target))


main()
