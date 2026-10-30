"""Import the small SARO room finish layers with the measured native FBX/PBR recipe.

Default: corridor. Pass -S47SaroImportControl for the later control-room layer.
Existing Office materials are loaded for reuse and never regenerated here.
"""
from pathlib import Path
import json
import sys
import unreal

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]
SOURCE = PROJECT.parent / 'ArtSource/SaroInteriors'
DEST = '/Game/Signal47/Art/SaroInteriors'
sys.path.insert(0, str(HERE.parent / 'fieldintegration'))
import import_recipe as R

kind = 'control' if 'S47SaroImportControl' in unreal.SystemLibrary.get_command_line() else 'corridor'
R.REPORT = Path(unreal.Paths.project_saved_dir()) / ('saro-' + kind + '-import.json')
R.report = {'status': 'RUNNING', 'kind': kind, 'checks': [], 'meshes': []}
R.flush()
contract = json.loads((SOURCE / (kind + '-contract.json')).read_text())
materials = {}
for row in json.loads((SOURCE / (kind + '-materials.json')).read_text()):
    materials[row['name']] = R.material(row, DEST + '/Materials', SOURCE)
for name, path in contract.get('reuse_materials', {}).items():
    material = R.E.load_asset(path)
    R.check('existing material ' + name, isinstance(material, unreal.MaterialInterface))
    materials[name] = material

name = contract['asset']
R.check('owned room asset name', name.startswith('SM_Saro_'))
path = DEST + '/' + name
if R.E.does_asset_exist(path):
    R.check('replace only new room finish mesh', R.E.delete_asset(path))
mesh = R.import_mesh(SOURCE / 'Export' / (name + '.fbx'), DEST)
bound = []
for index, slot in enumerate(mesh.get_editor_property('static_materials')):
    material_name = str(slot.material_slot_name)
    R.check('authored material ' + material_name, material_name in materials)
    mesh.set_material(index, materials[material_name])
    bound.append(material_name)
R.check('complete finish materials', set(bound) == set(contract['materials']))
R.check('authored topology', mesh.get_num_triangles(0) == contract['triangles'])
low, high = contract['bounds_local_m']
box = mesh.get_bounding_box()
expected_low = [low[0] * 100, -high[1] * 100, low[2] * 100]
expected_high = [high[0] * 100, -low[1] * 100, high[2] * 100]
actual_low = [box.min.x, box.min.y, box.min.z]
actual_high = [box.max.x, box.max.y, box.max.z]
R.check('metric shared origin', max(abs(a-b) for actual, expected in
        ((actual_low, expected_low), (actual_high, expected_high))
        for a, b in zip(actual, expected)) < .35,
        {'min_cm': actual_low, 'max_cm': actual_high})
# These small architectural skins keep every portal edge at every distance.
# They have only a few thousand triangles, so decimation would discard useful
# door/wall quads for no meaningful saving.
nanite = mesh.get_editor_property('nanite_settings')
nanite.enabled = False
mesh.set_editor_property('nanite_settings', nanite)
mesh.set_editor_property('support_ray_tracing', True)
body = mesh.get_editor_property('body_setup')
if body:
    instance = body.get_editor_property('default_instance')
    instance.set_editor_property('collision_profile_name', 'NoCollision')
    body.set_editor_property('default_instance', instance)
for lod in range(R.S.get_lod_count(mesh)):
    for section in range(mesh.get_num_sections(lod)):
        R.S.enable_section_collision(mesh, False, lod, section)
    build = R.S.get_lod_build_settings(mesh, lod)
    build.set_editor_property('use_full_precision_u_vs', True)
    R.S.set_lod_build_settings(mesh, lod, build)
R.check('saved native finish', R.E.save_loaded_asset(mesh))
R.report['meshes'].append({'asset': mesh.get_path_name(), 'triangles': contract['triangles']})
R.report['status'] = 'PASS'
R.flush()
unreal.log('S47 SARO INTERIOR IMPORT PASS ' + kind)
