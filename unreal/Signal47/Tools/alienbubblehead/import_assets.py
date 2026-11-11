"""D45-29 (Tom, 10 November): the alien bobblehead for the truck's dashboard, into the project.

Imports the two parts prepare.py made (head and body, glTF binaries in metres, +Y up) through
Interchange, as Tools/freemodels/import_free.py imports glTF: static meshes only, no skeleton,
no animation, the parts' meshes combined, and NO Nanite (the dresser's lesson, D44: Interchange's
glTF material instances stand on the engine's own glTF base material, which is not marked for
Nanite and cannot be saved, so a Nanite mesh with it draws grey in the packaged game).

  /Game/Licensed/Models/alienbubblehead/StaticMeshes/alienbubblehead_body   pivot: the middle of its base
  /Game/Licensed/Models/alienbubblehead/StaticMeshes/alienbubblehead_head   pivot: the neck spring
  /Game/Licensed/Models/alienbubblehead/Materials, Textures                 the one material and picture

/Game/Licensed is ignored by Git (unreal/.gitignore): these stay on this machine. S47Bobblehead.cpp
loads them by these paths; without them the truck is as it was.

Run (no editor of this project open; the editor target built):
  unreal/Signal47/Tools/alienbubblehead/import_assets.sh
or by hand:
  S47_BOBBLE_PREPARED=<prepared folder> UnrealEditor-Cmd Signal47.uproject -run=pythonscript \
    -script=Tools/alienbubblehead/import_assets.py -unattended -nullrhi -nosplash
Writes Saved/alienbubblehead-import.json and logs S47BOBBLE lines (OK / FAIL).
"""
import json
import os

import unreal

PREPARED = os.environ.get('S47_BOBBLE_PREPARED', os.path.expanduser('~/Documents/ChatGPT/Signal 47/modeller/alienbubblehead-20261110/prepared'))
DEST = '/Game/Licensed/Models/alienbubblehead'
PARTS = ('alienbubblehead_body', 'alienbubblehead_head')
# Must equal S47Bobblehead.cpp (HEAD_PIVOT) and prepare.py (EXPECT_PIVOT), metres in three's frame.
PIVOT = (0.0, 0.0436, -0.0017)
MAX_TEX = 1024  # an 11 cm figure: the 4096 px source is cut down for the cook (the source file stays as it came)

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
checks = []


def log(s):
    unreal.log_warning('S47BOBBLE ' + s)


def check(name, ok, detail=''):
    checks.append({'check': name, 'pass': bool(ok), 'detail': str(detail)})
    log(('OK   ' if ok else 'FAIL ') + name + ((': ' + str(detail)) if detail != '' else ''))
    return ok


def pipelines():
    """The project's glTF pipelines, changed in memory only (as import_free.py)."""
    assets = unreal.load_asset('/Interchange/Pipelines/DefaultGLTFAssetsPipeline')
    gltf = unreal.load_asset('/Interchange/Pipelines/DefaultGLTFPipeline')
    assets.set_editor_property('scene_name_sub_folder', False)
    assets.set_editor_property('asset_type_sub_folders', True)
    common = assets.get_editor_property('common_meshes_properties')
    common.set_editor_property('force_all_mesh_as_type', unreal.InterchangeForceMeshType.IFMT_STATIC_MESH)
    common.set_editor_property('bake_meshes', True)
    mesh = assets.get_editor_property('mesh_pipeline')
    mesh.set_editor_property('combine_static_meshes_behavior', unreal.InterchangeCombineStaticMeshesBehavior.ALL)
    mesh.set_editor_property('build_nanite', False)
    mesh.set_editor_property('import_skeletal_meshes', False)
    anim = assets.get_editor_property('animation_pipeline')
    anim.set_editor_property('import_animations', False)
    return assets, gltf


def import_part(assets, gltf, path, name):
    assets.set_editor_property('asset_name', name)
    stack = unreal.InterchangePipelineStackOverride()
    stack.add_pipeline(assets)
    stack.add_pipeline(gltf)
    t = unreal.AssetImportTask()
    t.set_editor_property('filename', path)
    t.set_editor_property('destination_path', DEST)
    t.set_editor_property('replace_existing', True)
    t.set_editor_property('automated', True)
    t.set_editor_property('save', True)
    t.set_editor_property('options', stack)
    tools.import_asset_tasks([t])
    return list(t.get_editor_property('imported_object_paths') or [])


def main():
    manifest_path = os.path.join(PREPARED, 'alienbubblehead.json')
    if not check('prepared parts present (run prepare.py first)', os.path.isfile(manifest_path), PREPARED):
        return
    manifest = json.load(open(manifest_path))
    off = max(abs(manifest['pivot_m'][k] - PIVOT[k]) for k in range(3))
    check('prepared pivot matches the game constant', off <= 0.0015, '%s vs %s' % (manifest['pivot_m'], PIVOT))
    check('prepared figure about 11 cm tall', 0.10 <= manifest['height_m'] <= 0.12, manifest['height_m'])
    # afresh: the folder holds only this figure
    if eal.does_directory_exist(DEST):
        eal.delete_directory(DEST)
    assets, gltf = pipelines()
    meshes = {}
    for name in PARTS:
        paths = import_part(assets, gltf, os.path.join(PREPARED, name + '.glb'), name)
        found = [eal.load_asset(p) for p in paths]
        sm = [a for a in found if isinstance(a, unreal.StaticMesh)]
        if not check(name + ' imported as one static mesh', len(sm) == 1, paths):
            continue
        mesh = sm[0]
        want = '%s/StaticMeshes/%s' % (DEST, name)
        if mesh.get_path_name().split('.')[0] != want:
            eal.rename_asset(mesh.get_path_name().split('.')[0], want)
            mesh = eal.load_asset(want)
        nanite = mesh.get_editor_property('nanite_settings')
        if nanite.get_editor_property('enabled'):
            nanite.set_editor_property('enabled', False)
            mesh.set_editor_property('nanite_settings', nanite)
        # no collision: it is drawn only (S47Bobblehead also sets NoCollision on its components)
        try:
            unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).remove_collisions(mesh)
        except Exception as e:  # (the subsystem is missing in some commandlets: the component's NoCollision is enough)
            log('note: collision kept on the asset (%s); the game draws it without' % e)
        eal.save_loaded_asset(mesh, only_if_is_dirty=False)
        meshes[name] = mesh
    for path in eal.list_assets(DEST, recursive=True):
        tex = eal.load_asset(path)
        if isinstance(tex, unreal.Texture2D) and (tex.get_editor_property('max_texture_size') == 0 or tex.get_editor_property('max_texture_size') > MAX_TEX):
            tex.set_editor_property('max_texture_size', MAX_TEX)
            eal.save_loaded_asset(tex)
    rows = {}
    for name, mesh in meshes.items():
        box = mesh.get_bounding_box()
        size = [box.max.x - box.min.x, box.max.y - box.min.y, box.max.z - box.min.z]
        part = manifest['parts'][name.split('_')[-1]]
        # three (x, y, z) metres are Unreal (x, z, y) centimetres: the same rule as S47Three::ToUE and Interchange's glTF
        want_min = [part['min_m'][0] * 100, part['min_m'][2] * 100, part['min_m'][1] * 100]
        want_max = [part['max_m'][0] * 100, part['max_m'][2] * 100, part['max_m'][1] * 100]
        got_min, got_max = [box.min.x, box.min.y, box.min.z], [box.max.x, box.max.y, box.max.z]
        err = max(max(abs(a - b) for a, b in zip(got_min, want_min)), max(abs(a - b) for a, b in zip(got_max, want_max)))
        check(name + ' size and pivot as prepared (cm, within 0.05)', err <= 0.05, 'min %s max %s, off %.3f' % ([round(v, 2) for v in got_min], [round(v, 2) for v in got_max], err))
        check(name + ' Nanite off', not mesh.get_editor_property('nanite_settings').get_editor_property('enabled'))
        slots = mesh.get_editor_property('static_materials')
        check(name + ' one material bound', len(slots) == 1 and slots[0].material_interface is not None,
              slots[0].material_interface.get_path_name() if slots and slots[0].material_interface else 'none')
        rows[name] = {'asset': mesh.get_path_name(), 'tris': mesh.get_num_triangles(0), 'size_cm': [round(v, 2) for v in size],
                      'min_cm': [round(v, 3) for v in got_min], 'max_cm': [round(v, 3) for v in got_max],
                      'material': slots[0].material_interface.get_path_name() if slots and slots[0].material_interface else ''}
    body = rows.get('alienbubblehead_body')
    head = rows.get('alienbubblehead_head')
    if body and head:
        check('figure 10 to 12 cm tall with its head on', 10.0 <= PIVOT[1] * 100 + head['max_cm'][2] <= 12.0, PIVOT[1] * 100 + head['max_cm'][2])
        check('body stands on its base (min z 0)', abs(body['min_cm'][2]) <= 0.05, body['min_cm'][2])
    report = {'_doc': 'Made by Tools/alienbubblehead/import_assets.py (D45-29). Unreal centimetres.', 'prepared': PREPARED,
              'source_sha256': manifest.get('source_sha256'), 'pivot_m': PIVOT, 'meshes': rows, 'checks': checks,
              'pass': all(c['pass'] for c in checks)}
    out = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), 'alienbubblehead-import.json')
    json.dump(report, open(out, 'w'), indent=1)
    log('%s: %d checks, wrote %s' % ('PASS' if report['pass'] else 'FAIL', len(checks), out))


main()
