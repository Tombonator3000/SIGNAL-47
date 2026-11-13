"""DEV-WAVES: brings the invented carbine of the F10 wave shooter (a dev toy) into /Game/Signal47/Dev/Waves.

Source: ArtSource/DevWaves/build_carbine.py (Blender), which writes Export/*.fbx and textures/*.png to a work folder.
Run through the queue with no editor open:
  S47_DEVWAVES_SOURCE=~/scratch-bm/Signal47-devwaves UnrealEditor-Cmd Signal47.uproject -run=pythonscript \
    -script=$PWD/Tools/devwaves/import_assets.py -unattended -nullrhi -nosplash -NoSound \
    '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0'
Uses the project's measured FBX/PBR recipe (Tools/fieldintegration/import_recipe.py): Blender (x, y, z) m -> Unreal (x, -y, z) cm.
"""
import os
import sys
import traceback
from pathlib import Path

import unreal

HERE = Path(__file__).resolve().parent
SRC = Path(os.path.expanduser(os.environ.get('S47_DEVWAVES_SOURCE', '~/scratch-bm/Signal47-devwaves'))).resolve()
DEST = '/Game/Signal47/Dev/Waves'
sys.path.insert(0, str(HERE.parent / 'fieldintegration'))
import import_recipe as R

R.REPORT = SRC / 'import-report.json'
R.report = {'status': 'RUNNING', 'engine': unreal.SystemLibrary.get_engine_version(), 'source': str(SRC), 'destination': DEST, 'checks': [], 'meshes': []}
E, S = R.E, R.S
TEX = {'base_color': 'textures/carbine_basecolor.png', 'normal': 'textures/carbine_normal.png',
       'roughness': 'textures/carbine_roughness.png', 'metallic': 'textures/carbine_metallic.png'}
ROWS = [
    {'name': 'M_DevWaves_Carbine', 'base_color': [1, 1, 1, 1], 'metallic': 1.0, 'roughness': 0.5, 'specular': 0.5, 'normal_strength': 1.0, 'textures': TEX},
    {'name': 'M_DevWaves_Brass', 'base_color': [0.80, 0.56, 0.20, 1], 'metallic': 1.0, 'roughness': 0.30, 'specular': 0.5},
    {'name': 'M_DevWaves_Crate', 'base_color': [0.105, 0.125, 0.065, 1], 'metallic': 0.25, 'roughness': 0.58, 'specular': 0.4},
    {'name': 'M_DevWaves_CrateTrim', 'base_color': [0.05, 0.05, 0.052, 1], 'metallic': 0.9, 'roughness': 0.45, 'specular': 0.5},
]
NAMES = ('SM_DevWaves_Carbine', 'SM_DevWaves_Mag', 'SM_DevWaves_Bolt', 'SM_DevWaves_Charge', 'SM_DevWaves_Casing', 'SM_DevWaves_Crate')
try:
    R.flush()
    R.check('source folder', SRC.is_dir(), str(SRC))
    R.check('all FBX sources', all((SRC / 'Export' / (n + '.fbx')).is_file() for n in NAMES))
    mats = {}
    for row in ROWS:
        mats[row['name']] = R.material(row, DEST + '/Materials', SRC)
    for name in NAMES:
        asset = DEST + '/' + name
        if E.does_asset_exist(asset):
            R.check(name + ' replace', E.delete_asset(asset))
        mesh = R.import_mesh(SRC / 'Export' / (name + '.fbx'), DEST)
        slots = []
        for index, slot in enumerate(mesh.get_editor_property('static_materials')):
            material = str(slot.material_slot_name)
            R.check(name + ' slot ' + material, material in mats, list(mats))
            mesh.set_material(index, mats[material])
            slots.append(material)
        settings = mesh.get_editor_property('nanite_settings')
        settings.enabled = False
        mesh.set_editor_property('nanite_settings', settings)
        for lod in range(S.get_lod_count(mesh)):
            build = S.get_lod_build_settings(mesh, lod)
            build.set_editor_property('use_full_precision_u_vs', True)
            S.set_lod_build_settings(mesh, lod, build)
        R.check(name + ' saved', E.save_loaded_asset(mesh, only_if_is_dirty=False))
        box = mesh.get_bounding_box()
        R.report['meshes'].append({'name': name, 'triangles': mesh.get_num_triangles(0), 'slots': slots,
                                   'min_cm': [round(v, 2) for v in (box.min.x, box.min.y, box.min.z)],
                                   'max_cm': [round(v, 2) for v in (box.max.x, box.max.y, box.max.z)]})
    R.report['status'] = 'PASS'
    R.flush()
    unreal.log('S47: DEVWAVES IMPORT PASS')
except Exception:
    R.report['status'] = 'FAIL'
    R.report['error'] = traceback.format_exc()
    R.flush()
    unreal.log_error('S47: DEVWAVES IMPORT FAIL ' + traceback.format_exc())
    raise
