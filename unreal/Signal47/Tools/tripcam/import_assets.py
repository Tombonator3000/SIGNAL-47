"""D46-10 (Tom, 10 November): the trip camera's model, into the project.

Imports the model made in Blender (build_tripcam.py, kept with the blend, glb and renders under
~/Documents/ChatGPT/Signal 47/modeller/tripcam-20261110/: original geometry, no downloads, no Cosmos) through
Interchange as Tools/alienbubblehead/import_assets.py imports glTF: one static mesh, its parts combined, no
skeleton, no animation, and NO Nanite (the dresser's lesson, D44: Interchange's glTF material instances stand on
the engine's own glTF base material, which is not marked for Nanite, so a Nanite mesh with it draws grey in the
packaged game). A box collision is added so the use trace (ECC_GameTraceChannel1) finds it: S47FieldLife.cpp's
TripModel() refuses a mesh without collision and draws the old box instead.

  /Game/Licensed/Models/tripcam/StaticMeshes/tripcam     pivot: the middle of the stake's foot, on the ground;
                                                         the lens looks down -Y in Unreal (three's -Z: yaw 0)
  /Game/Licensed/Models/tripcam/Materials                its dozen flat PBR materials (no textures)

/Game/Licensed is ignored by Git (unreal/.gitignore): it stays on this machine. Without it the trip camera is the
box it was.

Run (no editor of this project open; the editor target built):
  unreal/Signal47/Tools/tripcam/import_assets.sh
or by hand:
  S47_TRIPCAM_SRC=<folder with tripcam.glb and tripcam.json> UnrealEditor-Cmd Signal47.uproject -run=pythonscript \
    -script=Tools/tripcam/import_assets.py -unattended -nullrhi -nosplash
Writes Saved/tripcam-import.json and logs S47TRIPCAM lines (OK / FAIL).
"""
import json
import os

import unreal

SRC = os.environ.get('S47_TRIPCAM_SRC', os.path.expanduser('~/Documents/ChatGPT/Signal 47/modeller/tripcam-20261110'))
DEST = '/Game/Licensed/Models/tripcam'
NAME = 'tripcam'
# Must match S47FieldLife.cpp (TripModel): the package it loads.
WANT = '%s/StaticMeshes/%s' % (DEST, NAME)

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
checks = []


def log(s):
    unreal.log_warning('S47TRIPCAM ' + s)


def check(name, ok, detail=''):
    checks.append({'check': name, 'pass': bool(ok), 'detail': str(detail)})
    log(('OK   ' if ok else 'FAIL ') + name + ((': ' + str(detail)) if detail != '' else ''))
    return ok


def pipelines():
    """The project's glTF pipelines, changed in memory only (as import_free.py and the bobblehead)."""
    assets = unreal.load_asset('/Interchange/Pipelines/DefaultGLTFAssetsPipeline')
    gltf = unreal.load_asset('/Interchange/Pipelines/DefaultGLTFPipeline')
    assets.set_editor_property('scene_name_sub_folder', False)
    assets.set_editor_property('asset_type_sub_folders', True)
    assets.set_editor_property('asset_name', NAME)
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


def import_glb(assets, gltf, path):
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


def collision(mesh):
    """A box around it for the use trace; failing that, its triangles as the collision."""
    try:
        sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        sub.remove_collisions(mesh)
        n = sub.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.BOX)
        if n > 0:
            return 'box'
    except Exception as e:  # (the subsystem is missing in some commandlets)
        log('note: no box collision through the subsystem (%s)' % e)
    try:
        body = mesh.get_editor_property('body_setup')
        body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        mesh.set_editor_property('body_setup', body)
        return 'complex as simple'
    except Exception as e:
        log('note: no complex-as-simple either (%s)' % e)
    return 'none'


def main():
    glb = os.path.join(SRC, NAME + '.glb')
    manifest_path = os.path.join(SRC, NAME + '.json')
    if not check('tripcam.glb and tripcam.json present (build_tripcam.py made them)', os.path.isfile(glb) and os.path.isfile(manifest_path), SRC):
        return
    manifest = json.load(open(manifest_path))
    check('model 25 to 35 cm tall as built', 0.25 <= manifest['height_m'] <= 0.35, manifest['height_m'])
    check('under 8000 triangles as built', manifest['triangles'] < 8000, manifest['triangles'])
    # afresh: the folder holds only this model
    if eal.does_directory_exist(DEST):
        eal.delete_directory(DEST)
    assets, gltf = pipelines()
    paths = import_glb(assets, gltf, glb)
    sm = [a for a in (eal.load_asset(p) for p in paths) if isinstance(a, unreal.StaticMesh)]
    if not check('imported as one static mesh', len(sm) == 1, paths):
        return
    mesh = sm[0]
    if mesh.get_path_name().split('.')[0] != WANT:
        eal.rename_asset(mesh.get_path_name().split('.')[0], WANT)
        mesh = eal.load_asset(WANT)
    nanite = mesh.get_editor_property('nanite_settings')
    if nanite.get_editor_property('enabled'):
        nanite.set_editor_property('enabled', False)
        mesh.set_editor_property('nanite_settings', nanite)
    how = collision(mesh)
    eal.save_loaded_asset(mesh, only_if_is_dirty=False)
    box = mesh.get_bounding_box()
    got_min, got_max = [box.min.x, box.min.y, box.min.z], [box.max.x, box.max.y, box.max.z]
    # three / glTF (x, y, z) metres are Unreal (x, z, y) centimetres: the same rule as S47Three::ToUE and the bobblehead
    want_min = [manifest['min_m'][0] * 100, manifest['min_m'][2] * 100, manifest['min_m'][1] * 100]
    want_max = [manifest['max_m'][0] * 100, manifest['max_m'][2] * 100, manifest['max_m'][1] * 100]
    err = max(max(abs(a - b) for a, b in zip(got_min, want_min)), max(abs(a - b) for a, b in zip(got_max, want_max)))
    check('size and pivot as built (cm, within 0.1)', err <= 0.1, 'min %s max %s, off %.3f' % ([round(v, 2) for v in got_min], [round(v, 2) for v in got_max], err))
    check('stands on its foot (min z 0) and its lens is on the -Y side', abs(got_min[2]) <= 0.1 and got_min[1] < -5 and got_max[1] < 7, '%s' % [round(v, 2) for v in got_min])
    check('Nanite off', not mesh.get_editor_property('nanite_settings').get_editor_property('enabled'))
    check('collision for the use trace', how != 'none', how)
    slots = mesh.get_editor_property('static_materials')
    unbound = [i for i, s in enumerate(slots) if s.material_interface is None]
    check('every material slot bound', len(slots) > 0 and not unbound, '%d slots, unbound %s' % (len(slots), unbound))
    check('at the path S47FieldLife.cpp loads', mesh.get_path_name().split('.')[0] == WANT, mesh.get_path_name())
    report = {'_doc': 'Made by Tools/tripcam/import_assets.py (D46-10). Unreal centimetres.', 'source': glb, 'asset': mesh.get_path_name(),
              'tris': mesh.get_num_triangles(0), 'min_cm': [round(v, 3) for v in got_min], 'max_cm': [round(v, 3) for v in got_max],
              'materials': [s.material_interface.get_path_name() if s.material_interface else '' for s in slots], 'collision': how,
              'checks': checks, 'pass': all(c['pass'] for c in checks)}
    out = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), 'tripcam-import.json')
    json.dump(report, open(out, 'w'), indent=1)
    log('%s: %d checks, wrote %s' % ('PASS' if report['pass'] else 'FAIL', len(checks), out))


main()
