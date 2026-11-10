"""D48-20261110 (Tom, 10 November): the balloon find's model, into the project.

Imports the four pieces made in Blender (build_balloon.py, kept with the blend, glbs and renders under
~/Documents/ChatGPT/Signal 47/modeller/balloon-20261110/: original geometry, no downloads, no Cosmos) through Interchange as
Tools/tripcam/import_assets.py does: one static mesh a piece, its parts combined, no skeleton, no animation, and NO Nanite
(the dresser's lesson, D44: Interchange's glTF material instances stand on the engine's own glTF base material, which is not marked
for Nanite, so a Nanite mesh with it draws grey in the packaged game). A box collision is added to each so the use trace
(ECC_GameTraceChannel1) finds them: S47Balloon.cpp's Piece() refuses a mesh without collision and draws the basic shape instead.

  /Game/Licensed/Models/balloon/StaticMeshes/envelope      the burst envelope with the scrub bush under its corner
  /Game/Licensed/Models/balloon/StaticMeshes/parachute     the small orange parachute, its shroud lines and ring
  /Game/Licensed/Models/balloon/StaticMeshes/cord          the long white cord
  /Game/Licensed/Models/balloon/StaticMeshes/radiosonde    the instrument box, its label, antenna, battery pack and beacon lamp
  /Game/Licensed/Models/balloon/Materials                  their flat PBR materials (no textures)
Every pivot is on the ground under the piece's own middle; S47Balloon.cpp lays them out (metres: x east, y up, z south).

/Game/Licensed is ignored by Git (unreal/.gitignore): it stays on this machine. Without it the balloon is the basic shapes it was,
and the instrument box a pale cube; the beacon blinks either way.

Run (no editor of this project open; the editor target built):
  unreal/Signal47/Tools/balloon/import_assets.sh
or by hand:
  S47_BALLOON_SRC=<folder with the four glbs and balloon.json> UnrealEditor-Cmd Signal47.uproject -run=pythonscript \
    -script=Tools/balloon/import_assets.py -unattended -nullrhi -nosplash
Writes Saved/balloon-import.json and logs S47BALLOON lines (OK / FAIL).
"""
import json
import os

import unreal

SRC = os.environ.get('S47_BALLOON_SRC', os.path.expanduser('~/Documents/ChatGPT/Signal 47/modeller/balloon-20261110'))
DEST = '/Game/Licensed/Models/balloon'
# Must match S47Balloon.cpp (Layout and Folder): the packages it loads.
PIECES = ['envelope', 'parachute', 'cord', 'radiosonde']

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
checks = []


def log(s):
    unreal.log_warning('S47BALLOON ' + s)


def check(name, ok, detail=''):
    checks.append({'check': name, 'pass': bool(ok), 'detail': str(detail)})
    log(('OK   ' if ok else 'FAIL ') + name + ((': ' + str(detail)) if detail != '' else ''))
    return ok


def pipelines(name):
    """The project's glTF pipelines, changed in memory only (as import_free.py and the trip camera)."""
    assets = unreal.load_asset('/Interchange/Pipelines/DefaultGLTFAssetsPipeline')
    gltf = unreal.load_asset('/Interchange/Pipelines/DefaultGLTFPipeline')
    assets.set_editor_property('scene_name_sub_folder', False)
    assets.set_editor_property('asset_type_sub_folders', True)
    assets.set_editor_property('asset_name', name)
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


def game_frame(lo, hi):
    """Blender (x, y, z up) metres to the game's (x, y up, z south) centimetres: z is Blender's y with the sign turned."""
    want_min = [lo[0] * 100, lo[2] * 100, -hi[1] * 100]
    want_max = [hi[0] * 100, hi[2] * 100, -lo[1] * 100]
    return want_min, want_max


def main():
    manifest_path = os.path.join(SRC, 'balloon.json')
    present = all(os.path.isfile(os.path.join(SRC, p + '.glb')) for p in PIECES) and os.path.isfile(manifest_path)
    if not check('the four glbs and balloon.json present (build_balloon.py made them)', present, SRC):
        return
    manifest = json.load(open(manifest_path))
    check('under 15000 triangles in all as built', manifest['triangles'] < 15000, manifest['triangles'])
    # afresh: the folder holds only this model
    if eal.does_directory_exist(DEST):
        eal.delete_directory(DEST)
    report_pieces = {}
    for name in PIECES:
        assets, gltf = pipelines(name)
        paths = import_glb(assets, gltf, os.path.join(SRC, name + '.glb'))
        sm = [a for a in (eal.load_asset(p) for p in paths) if isinstance(a, unreal.StaticMesh)]
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
        how = collision(mesh)
        eal.save_loaded_asset(mesh, only_if_is_dirty=False)
        box = mesh.get_bounding_box()
        got_min, got_max = [box.min.x, box.min.y, box.min.z], [box.max.x, box.max.y, box.max.z]
        # Blender metres (x, y, z up) are Unreal (x, -y, z) centimetres: the frame S47Three::ToUE and the trip camera use
        piece = manifest['pieces'][name]
        want_min = [piece['min_m'][0] * 100, -piece['max_m'][1] * 100, piece['min_m'][2] * 100]
        want_max = [piece['max_m'][0] * 100, -piece['min_m'][1] * 100, piece['max_m'][2] * 100]
        err = max(max(abs(a - b) for a, b in zip(got_min, want_min)), max(abs(a - b) for a, b in zip(got_max, want_max)))
        check(name + ' size and pivot as built (cm, within 0.5)', err <= 0.5, 'min %s max %s, off %.3f' % ([round(v, 2) for v in got_min], [round(v, 2) for v in got_max], err))
        check(name + ' lies on the ground (min z about 0)', -2.5 <= got_min[2] <= 1.0, round(got_min[2], 2))
        check(name + ' Nanite off', not mesh.get_editor_property('nanite_settings').get_editor_property('enabled'))
        check(name + ' collision for the use trace', how != 'none', how)
        slots = mesh.get_editor_property('static_materials')
        unbound = [i for i, s in enumerate(slots) if s.material_interface is None]
        check(name + ' every material slot bound', len(slots) > 0 and not unbound, '%d slots, unbound %s' % (len(slots), unbound))
        check(name + ' at the path S47Balloon.cpp loads', mesh.get_path_name().split('.')[0] == want, mesh.get_path_name())
        report_pieces[name] = {'asset': mesh.get_path_name(), 'tris': mesh.get_num_triangles(0), 'min_cm': [round(v, 3) for v in got_min], 'max_cm': [round(v, 3) for v in got_max],
                               'materials': [s.material_interface.get_path_name() if s.material_interface else '' for s in slots], 'collision': how}
    beacon = manifest.get('beacon_m')
    check('the beacon sits on the radiosonde where S47Balloon.cpp has it (x .087, up .107, south .001, within 1 cm)',
          beacon and abs(beacon[0] - .087) < .01 and abs(beacon[2] - .107) < .01 and abs(-beacon[1] - .0011) < .01, beacon)
    report = {'_doc': 'Made by Tools/balloon/import_assets.py (D48-20261110). Unreal centimetres.', 'source': SRC, 'pieces': report_pieces,
              'checks': checks, 'pass': all(c['pass'] for c in checks) and len(report_pieces) == len(PIECES)}
    out = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), 'balloon-import.json')
    json.dump(report, open(out, 'w'), indent=1)
    log('%s: %d checks, wrote %s' % ('PASS' if report['pass'] else 'FAIL', len(checks), out))


main()
