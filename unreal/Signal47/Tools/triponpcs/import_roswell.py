"""Import only the private Roswell skin, its PBR materials and authored LODs.

Run in UnrealEditor-Cmd through the project guard after prepare_roswell.py.
No imported asset or source model belongs in the public repository.
"""
from pathlib import Path
import json
import math
import os
import sys

import unreal


DEFAULT_ROOT = Path.home() / 'Documents/ChatGPT/Signal 47/modeller/roswell-glimpse-20261031/prepared'
ROOT = Path(os.environ.get('S47_ROSWELL_DIR', str(DEFAULT_ROOT))).expanduser().resolve()
DEST = '/Game/Licensed/TripoNPC/Roswell'
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'fieldintegration'))
import import_recipe as R


def require(name, condition, detail=None):
    if not condition:
        raise RuntimeError(str((name, detail)))


def configure_fbx_data(settings):
    settings.import_uniform_scale = 1.
    settings.convert_scene = True
    settings.convert_scene_unit = True
    settings.force_front_x_axis = True
    settings.set_editor_property('coordinate_system_policy', unreal.CoordinateSystemPolicy.MATCH_UP_FORWARD_AXES)
    settings.import_rotation = unreal.Rotator(pitch=0, yaw=90, roll=0)
    settings.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS


def require_fbx_data(settings):
    require('Roswell legacy source import data', isinstance(settings, unreal.FbxSkeletalMeshImportData))
    require('Roswell metre-to-centimetre conversion retained', settings.convert_scene_unit and
            settings.convert_scene and abs(float(settings.import_uniform_scale) - 1.) < 1e-6)
    require('Roswell saved import frame retained', settings.force_front_x_axis and
            settings.get_editor_property('coordinate_system_policy') == unreal.CoordinateSystemPolicy.MATCH_UP_FORWARD_AXES and
            abs(float(settings.import_rotation.yaw) - 90) < 1e-5)


def verify_render_lod(mesh, row):
    required = ('GeometryScript_AssetUtils', 'GeometryScript_MeshQueries', 'GeometryScript_List')
    require('GeometryScripting enabled for actual LOD render-position validation',
            all(hasattr(unreal, name) for name in required), 'Use -EnablePlugins=GeometryScripting')
    options = unreal.GeometryScriptCopyMeshFromAssetOptions()
    options.set_editor_property('apply_build_settings', False)
    options.set_editor_property('use_build_scale', False)
    options.set_editor_property('request_tangents', False)
    requested = unreal.GeometryScriptMeshReadLOD()
    requested.set_editor_property('lod_type', unreal.GeometryScriptLODType.RENDER_DATA)
    requested.set_editor_property('lod_index', row['level'])
    transient = unreal.DynamicMesh()
    dynamic, outcome = unreal.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(
        mesh, transient, options, requested)
    require('actual Roswell render LOD copied', dynamic == transient and
            outcome == unreal.GeometryScriptOutcomePins.SUCCESS, row['level'])
    _, vector_list, _ = unreal.GeometryScript_MeshQueries.get_all_vertex_positions(dynamic, True)
    vectors = unreal.GeometryScript_List.convert_vector_list_to_array(vector_list)
    positions = [(float(point.x), float(point.y), float(point.z)) for point in vectors]
    require('actual Roswell render vertices finite', positions and
            all(math.isfinite(value) for point in positions for value in point), row['level'])
    actual = [[fn(point[axis] for point in positions) for axis in range(3)] for fn in (min, max)]
    low, high = row['bounds_blender_m']
    expected = [[100 * low[0], -100 * high[1], 100 * low[2]],
                [100 * high[0], -100 * low[1], 100 * high[2]]]
    bound_error = max(abs(a - b) for aa, bb in zip(actual, expected) for a, b in zip(aa, bb))
    require('actual Roswell render LOD frame and scale', bound_error < .1, (row['level'], actual, expected))
    probes = row.get('probe_points_blender_m', [])
    require('asymmetric Blender probes available; rerun prepare_roswell', len(probes) == 9)
    errors = []
    for x, y, z in probes:
        expected_point = (100 * x, -100 * y, 100 * z)
        errors.append(math.sqrt(min(sum((p[k] - expected_point[k]) ** 2 for k in range(3)) for p in positions)))
    require('actual Roswell render LOD asymmetric geometry/facing', max(errors) < .1, (row['level'], errors))
    triangles = dynamic.get_triangle_count()
    require('actual Roswell render triangle count', triangles == row['triangles'], (row['level'], triangles))
    return {'level': row['level'], 'triangles': triangles, 'bounds_cm': actual,
            'maximum_bounds_error_cm': bound_error, 'maximum_probe_error_cm': max(errors),
            'probe_count': len(probes), 'read_mode': 'RENDER_DATA'}


def main():
    # FbxFactory only chooses LOD0. ImportLOD otherwise silently selects Interchange
    # via CanTranslateSourceData, changing units/frame for the lower LODs.
    flag = 'Interchange.FeatureFlags.Import.FBX'
    unreal.SystemLibrary.execute_console_command(None, flag + ' 0')
    require('legacy FBX translator selected for every LOD',
            unreal.SystemLibrary.get_console_variable_int_value(flag) == 0)
    data = json.loads((ROOT / 'manifest.json').read_text())
    require('real Roswell skeletal source', data['name'] == 'SK_Tripo_Roswell' and data.get('skeletal'))
    require('three authored consistent LODs', [row['level'] for row in data['lods']] == [0, 1, 2])
    R.check = require
    material_map = {}
    for row in data['materials']:
        material = R.material(row, DEST + '/Materials', ROOT)
        material.set_editor_property('used_with_skeletal_mesh', True)
        require(row['name'] + ' skeletal material compiles', not list(R.M.recompile_material(material)))
        require(row['name'] + ' material saved', R.E.save_loaded_asset(material, only_if_is_dirty=False))
        material_map[row['name']] = material
    task = unreal.AssetImportTask()
    task.filename = str(ROOT / data['lods'][0]['fbx'])
    task.destination_path = DEST
    task.destination_name = data['name']
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = True
    options.import_animations = False
    options.import_materials = False
    options.import_textures = False
    options.create_physics_asset = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    options.automated_import_should_detect_type = False
    # The project's measured FBX mapping: Blender (x,y,z)m -> UE (100x,-100y,100z)cm.
    # Preserve it in the saved base import data, reused by legacy ImportLOD.
    configure_fbx_data(options.skeletal_mesh_import_data)
    existing = unreal.load_asset(DEST + '/' + data['name'])
    if existing:
        require('existing Roswell target is skeletal', isinstance(existing, unreal.SkeletalMesh))
        saved_settings = existing.get_editor_property('asset_import_data')
        require('existing Roswell target has legacy FBX data', isinstance(saved_settings, unreal.FbxSkeletalMeshImportData))
        # UE's atomic skeletal reimport replaces task.options.SkeletalMeshImportData
        # with this saved object (EditorFactories.cpp). Configure it too; otherwise
        # a failed first import silently retains its old units/frame on every retry.
        configure_fbx_data(saved_settings)
        require_fbx_data(saved_settings)
    task.options = options
    task.factory = unreal.FbxFactory()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.load_asset(DEST + '/' + data['name'])
    require('Roswell skeletal mesh imported', isinstance(mesh, unreal.SkeletalMesh), task.imported_object_paths)
    require_fbx_data(mesh.get_editor_property('asset_import_data'))
    editor = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    require('skeletal editor available', editor is not None)
    for row in data['lods'][1:]:
        require_fbx_data(mesh.get_editor_property('asset_import_data'))
        imported = editor.import_lod(mesh, row['level'], str(ROOT / row['fbx']))
        require('Roswell LOD' + str(row['level']) + ' imported', imported == row['level'], imported)
    require('all three native LODs', editor.get_lod_count(mesh) == 3)
    render_lods = [verify_render_lod(mesh, row) for row in data['lods']]
    entries = mesh.get_editor_property('materials')
    for entry in entries:
        slot = str(entry.get_editor_property('imported_material_slot_name'))
        if slot not in material_map:
            slot = str(entry.get_editor_property('material_slot_name'))
        require('source material slot recognized', slot in material_map, slot)
        entry.material_interface = material_map[slot]
    mesh.set_editor_property('materials', entries)
    bounds = mesh.get_imported_bounds()
    height = float(bounds.box_extent.z) * 2
    low = float(bounds.origin.z) - float(bounds.box_extent.z)
    require('Roswell native height cm', abs(height - data['height_m'] * 100) < .5, height)
    require('Roswell native foot pivot', abs(low) < .5, low)
    vertices = [editor.get_num_verts(mesh, level) for level in range(3)]
    require('nonempty native LODs', all(count > 0 for count in vertices), vertices)
    require('reduced native LODs', vertices[0] > vertices[1] > vertices[2], vertices)
    require('Roswell skeletal asset saved', R.E.save_loaded_asset(mesh, only_if_is_dirty=False))
    skeleton = mesh.get_editor_property('skeleton')
    require('Roswell source skeleton retained', skeleton is not None)
    require('Roswell skeleton saved', R.E.save_loaded_asset(skeleton, only_if_is_dirty=False))
    result = {'mesh': mesh.get_path_name(), 'skeleton': skeleton.get_path_name(),
              'height_cm': height, 'foot_z_cm': low, 'lod_vertices': vertices,
              'render_lods': render_lods,
              'source_sha256': data['source_sha256'],
              'materials': [material.get_path_name() for material in material_map.values()]}
    (ROOT / 'imported.json').write_text(json.dumps(result, indent=2) + '\n')
    unreal.log('ROSWELL IMPORT COMPLETE ' + json.dumps(result))


if __name__ == '__main__':
    main()
