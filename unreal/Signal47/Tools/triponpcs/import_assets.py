"""Import prepared private static mannequins through the existing measured FBX recipe."""
from pathlib import Path
import json
import os
import sys

import unreal


ROOT = Path(os.environ.get('S47_TRIPO_STATIC_DIR', str(Path.home() / 'Documents/ChatGPT/Signal 47/modeller/tripo-npc-2026-10-29/unreal-static-2026-10-30')))
DEST = '/Game/Licensed/TripoNPC'
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'fieldintegration'))
import import_recipe as R


def require(name, condition, detail=None):
    if not condition:
        raise RuntimeError(str((name, detail)))


def main():
    R.check = require
    data = json.loads((ROOT / 'manifest.json').read_text())
    selected = set(os.environ['S47_TRIPO_ONLY'].split(',')) if os.environ.get('S47_TRIPO_ONLY') else None
    result = []
    for row in data['assets']:
        if selected and row['role'] not in selected:
            continue
        material = R.material(row['material'], DEST + '/Materials', ROOT)
        if row['material'].get('unlit_black'):
            material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
            black = R.expression(material, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(0, 0, 0))
            R.M.connect_material_property(black, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
            require('shadow unlit material compiles', not list(R.M.recompile_material(material)))
            require('shadow material saved', R.E.save_loaded_asset(material))
        mesh = R.import_mesh(ROOT / row['fbx'], DEST)
        mesh.set_material(0, material)
        mesh.set_editor_property('allow_cpu_access', False)
        mesh.set_editor_property('support_ray_tracing', True)
        body = mesh.get_editor_property('body_setup')
        if body:
            instance = body.get_editor_property('default_instance')
            instance.set_editor_property('collision_profile_name', 'NoCollision')
            body.set_editor_property('default_instance', instance)
        options = unreal.StaticMeshReductionOptions()
        options.set_editor_property('auto_compute_lod_screen_size', False)
        levels = []
        for ratio, screen in ((1., 1.), (.45, .25), (.15, .07)):
            setting = unreal.StaticMeshReductionSettings()
            setting.set_editor_property('percent_triangles', ratio)
            setting.set_editor_property('screen_size', screen)
            levels.append(setting)
        options.set_editor_property('reduction_settings', levels)
        require(row['role'] + ' LODs', R.S.set_lods(mesh, options))
        bounds = mesh.get_bounding_box()
        require(row['role'] + ' expected height', abs(bounds.max.z - bounds.min.z - row['height_m'] * 100) < .5)
        require(row['role'] + ' saved', R.E.save_loaded_asset(mesh, only_if_is_dirty=False))
        result.append({'role': row['role'], 'mesh': mesh.get_path_name(), 'pose': row['pose'],
                       'height_m': row['height_m'], 'seat_z_m': row['seat_z_m']})
        unreal.log('TRIPO STATIC IMPORTED ' + row['role'] + ' ' + mesh.get_path_name())
    (ROOT / 'imported.json').write_text(json.dumps({'static_only': True, 'assets': result}, indent=2) + '\n')
    unreal.log('TRIPO STATIC IMPORT COMPLETE ' + str(len(result)))


if __name__ == '__main__':
    main()
