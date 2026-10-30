"""Small native SARO 1986 imports, using the project's measured legacy FBX recipe.

Run Unreal Python with -S47Saro1986Kind=rx3|jacket|phone|fixtures|legacy.
Private phone/Tripo preparations never enter public Git. Existing Office assets
are read only; the optional fixture cleanup uses a separate CPU-retained copy.
"""
from pathlib import Path
import json
import os
import re
import sys
import unreal

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]
SOURCE = PROJECT.parent / 'ArtSource/Saro1986'
DEST = '/Game/Signal47/Art/Saro1986'
PRIVATE = Path(os.environ.get('S47_SARO_PREPARED', str(
    Path.home() / 'scratch-bm/Signal47-saro-1986-props-20261030/prepared')))
sys.path.insert(0, str(HERE.parent / 'fieldintegration'))
import import_recipe as R


def require(name, condition, detail=None):
    if not condition:
        raise RuntimeError(str((name, detail)))


R.check = require


def native_mesh(root, row, destination, materials, lods=False):
    name = row['asset']
    path = destination + '/' + name
    if R.E.does_asset_exist(path):
        require('replace only this pass asset', name.startswith('SM_Saro'))
        require('replace prepared mesh', R.E.delete_asset(path))
    fbx = root / row.get('fbx', 'Export/' + name + '.fbx')
    mesh = R.import_mesh(fbx, destination)
    for index, slot in enumerate(mesh.get_editor_property('static_materials')):
        key = str(slot.material_slot_name)
        require('known authored material ' + key, key in materials)
        mesh.set_material(index, materials[key])
    require('expected topology ' + name, mesh.get_num_triangles(0) == row['triangles'])
    if 'bounds_local_m' in row:
        lo, hi = row['bounds_local_m']
        expected = [[lo[0]*100, -hi[1]*100, lo[2]*100],
                    [hi[0]*100, -lo[1]*100, hi[2]*100]]
        box = mesh.get_bounding_box()
        actual = [[box.min.x, box.min.y, box.min.z], [box.max.x, box.max.y, box.max.z]]
        require('measured metric axes ' + name,
                max(abs(a-b) for aa, bb in zip(actual, expected) for a, b in zip(aa, bb)) < .35,
                actual)
    if lods:
        options = unreal.StaticMeshReductionOptions()
        options.auto_compute_lod_screen_size = False
        levels = []
        for ratio, screen in ((1., 1.), (.45, .25), (.15, .07)):
            level = unreal.StaticMeshReductionSettings()
            level.percent_triangles = ratio
            level.screen_size = screen
            levels.append(level)
        options.reduction_settings = levels
        require('prop LODs', R.S.set_lods(mesh, options))
    nanite = mesh.get_editor_property('nanite_settings')
    nanite.enabled = False
    mesh.set_editor_property('nanite_settings', nanite)
    mesh.set_editor_property('support_ray_tracing', True)
    mesh.set_editor_property('allow_cpu_access', False)
    body = mesh.get_editor_property('body_setup')
    if body:
        instance = body.get_editor_property('default_instance')
        instance.set_editor_property('collision_profile_name', 'NoCollision')
        body.set_editor_property('default_instance', instance)
    for lod in range(R.S.get_lod_count(mesh)):
        for section in range(mesh.get_num_sections(lod)):
            R.S.enable_section_collision(mesh, False, lod, section)
    require('saved native prop', R.E.save_loaded_asset(mesh, only_if_is_dirty=False))
    unreal.log('S47 SARO 1986 IMPORTED ' + mesh.get_path_name())


def own(kind):
    row = json.loads((SOURCE / (kind + '-contract.json')).read_text())
    materials = {item['name']: R.material(item, DEST + '/Materials', SOURCE)
                 for item in json.loads((SOURCE / (kind + '-materials.json')).read_text())}
    for key, path in row.get('reuse_materials', {}).items():
        materials[key] = R.E.load_asset(path)
        require('existing material ' + key, isinstance(materials[key], unreal.MaterialInterface))
    native_mesh(SOURCE, row, DEST, materials)


def phone():
    for row in json.loads((PRIVATE / 'phone-contract.json').read_text())['meshes']:
        materials = {}
        for slot in row['materials']:
            material = R.E.load_asset(slot['path'])
            require('existing private phone material', isinstance(material, unreal.MaterialInterface))
            materials[slot['slot']] = material
        native_mesh(PRIVATE, row, '/Game/Licensed/Saro1986', materials, lods=True)


def fixtures():
    root = Path(os.environ['S47_TRIPO_FIXTURE_PREPARED'])
    data = json.loads((root / 'fixtures-contract.json').read_text())
    materials = {item['name']: R.material(item, '/Game/Licensed/Saro1986/Materials', root)
                 for item in json.loads((root / 'fixtures-materials.json').read_text())}
    for row in data['meshes']:
        native_mesh(root, row, '/Game/Licensed/Saro1986', materials, lods=True)


def legacy():
    command = unreal.SystemLibrary.get_command_line()
    match = re.search(r'-S47Saro1986CpuGeos=([0-9,]+)', command)
    geos = [int(value) for value in match.group(1).split(',')] if match else [54, 59, 219, 376]
    for geo in geos:
        matches = list((PROJECT / 'Content/Web/Geo').glob(f'*/StaticMeshes/g{geo}.uasset'))
        require('one measured legacy mesh package', len(matches) == 1, geo)
        path = '/Game/' + str(matches[0].relative_to(PROJECT / 'Content').with_suffix(''))
        mesh = R.E.load_asset(path)
        require('explicit legacy CPU mesh', isinstance(mesh, unreal.StaticMesh), path)
        mesh.set_editor_property('allow_cpu_access', True)
        require('saved legacy CPU data', R.E.save_loaded_asset(mesh, only_if_is_dirty=False))
    original = '/Game/Signal47/Art/Office/SM_Office_Room'
    # Office's approved native package is never modified by this preparation.
    duplicate_path = DEST + '/SM_Saro1986_OfficeRoomCpu'
    if not R.E.does_asset_exist(duplicate_path):
        require('separate Office CPU copy', R.E.duplicate_asset(original, duplicate_path))
    duplicate = R.E.load_asset(duplicate_path)
    require('Office CPU copy exists', isinstance(duplicate, unreal.StaticMesh))
    duplicate.set_editor_property('allow_cpu_access', True)
    require('CPU copy saved', R.E.save_loaded_asset(duplicate, only_if_is_dirty=False))


match = re.search(r'-S47Saro1986Kind=([a-z]+)', unreal.SystemLibrary.get_command_line())
require('explicit import kind', match is not None)
kind = match.group(1)
require('known import kind', kind in ('rx3', 'jacket', 'phone', 'fixtures', 'legacy', 'props', 'console', 'refine'))
if kind == 'props':
    own('rx3')
    own('jacket')
    phone()
    legacy()
elif kind == 'refine':
    own('jacket')
    own('console-trim')
elif kind == 'console':
    own('console-trim')
elif kind in ('rx3', 'jacket'):
    own(kind)
else:
    globals()[kind]()
unreal.log('S47 SARO 1986 IMPORT PASS ' + kind)
