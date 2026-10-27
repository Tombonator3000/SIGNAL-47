"""SIGNAL / 47 MetaHuman staging: the Mesa Diner waitress.

Runs inside the MHStaging editor (commandlet with -AllowCommandletRendering, or a GUI editor
through tools/mh_remote.py). Every step logs one JSON line to logs/mh.jsonl so a later step,
or another agent, can see exactly what happened. Nothing here touches the Signal47 project.

Steps (call in this order; each is idempotent):
  probe()                    engine facts the later steps depend on
  studio()                   neutral studio level with diner-like light and capture rigs
  audition(names)            preview-assemble presets and capture face/body images
  make_waitress(preset)      the character asset, hair, skin, eyes, light makeup
  preview_capture(tag)       capture the current waitress preview
  export_preview_body()      body FBX for the Blender uniform (no cloud needed)
  import_takes()             the 43 waitress WAVs as SoundWaves
  speech2face(ids)           audio-driven face AnimSequences per take (local NNE)
  rig() / textures()         Epic cloud: needs one interactive Epic login
  build()                    UE Optimized (High) assembly under /Game/MetaHumans
"""
import json
import os
import time
import traceback

import unreal

BASE = '/home/tom/scratch-bm/Signal47-metahuman-20261026'
LOG = BASE + '/logs/mh.jsonl'
SHOTS = BASE + '/shots'
TAKES = '/home/tom/.cache/signal47-eveningshift/takes'
MH_ROOT = '/Game/MetaHumans'
CHAR_DIR = MH_ROOT + '/Waitress'
CHAR_NAME = 'MHC_Waitress'
CHAR_PATH = f'{CHAR_DIR}/{CHAR_NAME}'
AUDITION_DIR = '/Game/S47/Audition'
AUDIO_DIR = '/Game/S47/Audio/Waitress'
FACE_ANIM_DIR = CHAR_DIR + '/FaceAnims'   # runtime: AS_<take>, see S47Waitress.cpp
ANIM_DIR = CHAR_DIR + '/Anims'           # runtime: AS_Waitress_Idle, AS_Waitress_IdleFace
PERF_DIR = '/Game/S47/Performances/Waitress'
PRESETS = '/MetaHumanCharacter/Optional/Presets'
GROOMS = '/MetaHumanCharacter/Optional/Grooms/Bindings'
STUDIO = '/Game/S47/L_Studio'
PLUGIN_FACE_SKELETON = '/MetaHumanCharacter/Face/Face_Archetype_Skeleton.Face_Archetype_Skeleton'
COMMON_FACE_SKELETON = MH_ROOT + '/Common/Face/Face_Archetype_Skeleton.Face_Archetype_Skeleton'
# The idle pair captured together for MetaHuman Creator's preview: body loop and face curves.
IDLE_BODY = '/MetaHumanCharacter/Optional/Animation/TemplateAnimations/Technical_Loops/Idle/mhc_mh001_fmn_b_idle'
IDLE_FACE = '/MetaHumanCharacter/Optional/Animation/TemplateAnimations/Technical_Loops/Idle/mhc_mh001_fmn_f_idle'


def log(step, **data):
    data = {'t': time.strftime('%Y-%m-%dT%H:%M:%S'), 'step': step, **data}
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    with open(LOG, 'a') as f:
        f.write(json.dumps(data, default=str) + '\n')
    unreal.log(f'[S47MH] {step}: {json.dumps(data, default=str)[:1500]}')
    say(f"{step}: {', '.join(f'{k}={v}' for k, v in data.items() if k not in ('t', 'step') and len(str(v)) < 120)}"[:300])
    return data


def say(text):
    """One readable line for the terminal window Tom watches (logs/progress.log)."""
    with open(BASE + '/logs/progress.log', 'a') as f:
        f.write(time.strftime('%H:%M:%S ') + text + '\n')


def guarded(fn):
    def run(*a, **k):
        try:
            return fn(*a, **k)
        except Exception as e:
            log(fn.__name__ + ':error', error=repr(e), trace=traceback.format_exc())
            raise
    run.__name__ = fn.__name__
    return run


def sub():
    return unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)


def actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def assets(path, cls=None, recursive=True):
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    flt = unreal.ARFilter(package_paths=[path], recursive_paths=recursive,
                          class_paths=[cls.static_class().get_class_path_name()] if cls else [])
    return reg.get_assets(flt)


def enum_names(e):
    return [n for n in dir(e) if n.isupper()]


def props(obj):
    return sorted(n for n in dir(obj) if not n.startswith('_') and not callable(getattr(obj, n, None)))


# ---------------------------------------------------------------- probe

@guarded
def probe(load_preset=True):
    facts = {}
    opt = '/home/tom/.local/opt/UnrealEngine-5.8.4/Engine/Plugins/MetaHuman/MetaHumanCharacter/Content/Optional'
    facts['optional_dirs'] = sorted(os.listdir(opt)) if os.path.isdir(opt) else None
    facts['presets'] = sorted(str(a.asset_name) for a in assets(PRESETS))
    for slot in ('Hair', 'Eyebrows', 'Eyelashes', 'Peachfuzz'):
        facts['groom_' + slot] = sorted(str(a.asset_name) for a in assets(f'{GROOMS}/{slot}', unreal.MetaHumanWardrobeItem))
    for name in ('MetaHumanRigType', 'MetaHumanQualityLevel', 'MetaHumanDefaultPipelineType',
                 'AudioDrivenAnimationMood', 'PerformanceHeadMovementMode'):
        e = getattr(unreal, name, None)
        facts['enum_' + name] = enum_names(e) if e else None
    facts['character_props'] = props(unreal.MetaHumanCharacter)
    facts['build_params'] = props(unreal.MetaHumanCharacterEditorBuildParameters())
    facts['subsystem'] = sorted(n for n in dir(sub()) if not n.startswith('_'))
    facts['face_skeleton'] = bool(unreal.load_asset(PLUGIN_FACE_SKELETON))
    facts['idle_pair'] = [bool(unreal.EditorAssetLibrary.does_asset_exist(x)) for x in (IDLE_BODY, IDLE_FACE)]
    facts['rhi'] = unreal.SystemLibrary.get_rendering_detail_mode() if hasattr(unreal.SystemLibrary, 'get_rendering_detail_mode') else None
    ch = unreal.load_asset(f'{PRESETS}/Grace') if load_preset else None
    if ch:
        for p in ('skin_settings', 'eyes_settings', 'makeup_settings', 'head_model_settings'):
            v = getattr(ch, p, None)
            facts['grace_' + p] = props(v) if v is not None else None
        facts['grace_has_high_res'] = getattr(ch, 'has_high_resolution_textures', None)
    return log('probe', **facts)


# ---------------------------------------------------------------- studio

def _spawn(cls, loc, rot=(0, 0, 0), label=None):
    a = actors().spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(*rot))
    if label:
        a.set_actor_label(label)
    return a


@guarded
def studio():
    """A plain room lit like the diner counter: warm overhead tubes, a cool window side, dim fill."""
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(STUDIO):
        les.load_level(STUDIO)
        return log('studio', loaded=True)
    les.new_level(STUDIO)
    floor = _spawn(unreal.StaticMeshActor, (0, 0, 0), label='Floor')
    floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane'))
    floor.set_actor_scale3d(unreal.Vector(20, 20, 1))
    wall = _spawn(unreal.StaticMeshActor, (-250, 0, 150), (0, 90, 0), label='Backwall')
    wall.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane'))
    wall.set_actor_scale3d(unreal.Vector(6, 8, 1))
    wall.set_actor_rotation(unreal.Rotator(roll=0, pitch=-90, yaw=0), False)  # plane normal +Z to +X, facing the cameras
    over = _spawn(unreal.RectLight, (60, 0, 260), (0, -90, 0), label='CounterTubes')
    over.light_component.set_intensity(45)
    over.light_component.set_light_color(unreal.LinearColor(1.0, 0.86, 0.68))
    over.light_component.set_editor_property('source_width', 120)
    over.light_component.set_editor_property('source_height', 30)
    window = _spawn(unreal.RectLight, (120, 180, 150), (0, -15, -120), label='WindowDusk')
    window.light_component.set_intensity(12)
    window.light_component.set_light_color(unreal.LinearColor(0.55, 0.65, 1.0))
    sky = _spawn(unreal.SkyLight, (0, 0, 300), label='Fill')
    sky.light_component.set_intensity(0.35)
    ppv = _spawn(unreal.PostProcessVolume, (0, 0, 0), label='Exposure')
    ppv.set_editor_property('unbound', True)
    s = ppv.settings
    s.set_editor_property('override_auto_exposure_method', True)
    s.set_editor_property('auto_exposure_method', unreal.AutoExposureMethod.AEM_MANUAL)
    s.set_editor_property('override_auto_exposure_bias', True)
    s.set_editor_property('auto_exposure_bias', 10.0)
    ppv.set_editor_property('settings', s)
    for tag, loc, fov in (('face', (85, 0, 158), 26.0), ('body', (330, 0, 105), 34.0)):
        cam = _spawn(unreal.CameraActor, loc, (0, 0, 180), label='Cam_' + tag)
        cam.camera_component.set_field_of_view(fov)
        rig = _spawn(unreal.SceneCapture2D, loc, (0, 0, 180), label='Cap_' + tag)
        cap = rig.capture_component2d
        cap.set_editor_property('fov_angle', fov)
        cap.set_editor_property('capture_source', unreal.SceneCaptureSource.SCS_FINAL_TONE_CURVE_HDR)
    unreal.EditorLoadingAndSavingUtils.save_current_level()
    return log('studio', created=True)


def world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def _find(label):
    for a in actors().get_all_level_actors():
        if a.get_actor_label() == label:
            return a
    return None


def capture(tag, name, size=(1024, 1280)):
    """Scene-capture the labelled rig to SHOTS/name.png; works in a commandlet with rendering."""
    rig = _find('Cap_' + tag)
    if not rig:
        raise RuntimeError('studio() first: no capture rig ' + tag)
    rt = unreal.RenderingLibrary.create_render_target2d(world(), size[0], size[1],
                                                        unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB)
    cap = rig.capture_component2d
    cap.set_editor_property('texture_target', rt)
    for _ in range(3):  # streaming and grooms settle over a few captures
        cap.capture_scene()
    os.makedirs(SHOTS, exist_ok=True)
    unreal.RenderingLibrary.export_render_target(world(), rt, SHOTS, name + '.png')
    return SHOTS + '/' + name + '.png'


# ---------------------------------------------------------------- characters

def _tolerant(call, *a):
    """UE Python turns any error line logged during a call into an exception, even from unrelated
    assets (ARKit mapping, control rig link rebuilds). Run anyway; callers verify the result."""
    try:
        return call(*a)
    except RuntimeError as e:
        unreal.log_warning('[S47MH] tolerated: ' + str(e).splitlines()[0][:300])
        return None


def _duplicate(src, dst):
    if not unreal.EditorAssetLibrary.does_asset_exist(dst):
        _tolerant(unreal.EditorAssetLibrary.duplicate_asset, src, dst)
    a = unreal.load_asset(dst)
    if not a:
        raise RuntimeError(f'duplicate failed: {src} -> {dst}')
    return a


def _open(ch):
    if not sub().is_object_added_for_editing(ch):
        if not sub().try_add_object_to_edit(character=ch):
            raise RuntimeError('cannot open for edit: ' + ch.get_path_name())


def _show(ch, yaw=0.0):
    sub().assemble_for_preview(ch)
    actor = sub().spawn_meta_human_actor(ch, True)
    actor.set_actor_location(unreal.Vector(0, 0, 0), False, False)
    actor.set_actor_rotation(unreal.Rotator(0, 0, yaw), False)
    return actor


@guarded
def audition(names=('Grace', 'Celeste', 'Jelena', 'Vivian', 'Aera', 'Etta')):
    """Single-call variant for a commandlet; in a live editor prefer show/shoot/clear, which let
    the editor tick (texture streaming, groom binding) between spawning and capturing."""
    studio()
    done = []
    for n in names:
        show(n)
        done.append({'preset': n, 'shots': shoot(n)})
        clear()
    return log('audition', presets=done)


_shown = {}


@guarded
def show(name, yaw=0.0):
    """Spawn one preset (or 'Waitress') in the studio and leave it for the editor to tick."""
    clear()
    clear_all()
    path = CHAR_PATH if name == 'Waitress' else f'{AUDITION_DIR}/MHC_{name}'
    ch = unreal.load_asset(CHAR_PATH) if name == 'Waitress' else _duplicate(f'{PRESETS}/{name}', path)
    _open(ch)
    _shown['actor'], _shown['character'], _shown['name'] = _show(ch, yaw), ch, name
    return log('show', name=name, actor=_shown['actor'].get_name())


@guarded
def shoot(tag=None):
    tag = tag or _shown.get('name', 'shot')
    shots = [capture('face', f'{tag}_face'), capture('body', f'{tag}_body')]
    log('shoot', tag=tag, shots=shots)
    return shots


def clear():
    """Remove the shown actor; audition presets also leave edit mode (the waitress stays open)."""
    a, ch, name = _shown.pop('actor', None), _shown.pop('character', None), _shown.pop('name', None)
    if a:
        try:
            a.destroy_actor()
        except Exception:
            pass  # its level is already gone (level change, editor restart)
    if ch and name != 'Waitress' and sub().is_object_added_for_editing(ch):
        sub().remove_object_to_edit(ch)


def _wardrobe_item(slot, name):
    folder = {'Beard': 'Beards', 'Mustache': 'Mustaches'}.get(slot, slot)
    for a in assets(f'{GROOMS}/{folder}', unreal.MetaHumanWardrobeItem):
        if str(a.asset_name) == name:
            return a.get_asset()
    raise RuntimeError(f'no {slot} wardrobe item {name}')


def _select(ch, slot, item):
    key = ch.internal_collection.try_add_item_from_wardrobe_item(slot_name=slot, wardrobe_item=item)
    if key is None:
        raise RuntimeError(f'cannot add {slot} item')
    ch.internal_collection.default_instance.set_single_slot_selection(slot_name=slot, item_key=key)
    return key


# The canonical portrait: about 48, light warm sun-worn skin, brown shoulder-length hair half up
# with a pencil behind the ear, brown-hazel eyes, a working day's makeup in 1986.
WAITRESS = {
    'hair': 'WI_Hair_M_BobMessy',   # audition first; alternates: WI_Hair_S_LowPonytail, WI_Hair_M_Layered
    'eyebrows': None,            # keep the preset's
    'skin': (0.22, 0.62),        # lightness, redness (MetaHumanGenerator scale)
    'eyes': (0.75, 0.35),        # temperature, brightness: medium hazel-brown
    'body': {'Masculine/Feminine': 0.88, 'Fat': 0.42, 'Muscularity': 0.38, 'Height': 165.0},
}


def _set_body(ch, wanted):
    cons = sub().get_body_constraints(character=ch, scale_measurement_ranges_with_height=False)
    by = {str(c.name): c for c in cons}
    for name, value in wanted.items():
        c = by.get(name)
        if not c:
            continue
        lo, hi = float(c.min_measurement), float(c.max_measurement)
        c.is_active = True
        c.target_measurement = min(max(value, lo), hi) if name == 'Height' else lo + (hi - lo) * value
    sub().set_body_constraints(character=ch, body_constraints=list(by.values()))
    sub().commit_body_state(character=ch)
    if hasattr(unreal, 'MetaHumanGeneratorSubsystemWrapper'):
        unreal.MetaHumanGeneratorSubsystemWrapper.reset_neck_to_body(ch)


@guarded
def make_waitress(preset='Grace', cfg=None):
    cfg = {**WAITRESS, **(cfg or {})}
    ch = _duplicate(f'{PRESETS}/{preset}', CHAR_PATH)
    _open(ch)
    _set_body(ch, cfg['body'])
    skin = ch.skin_settings
    skin.skin.u, skin.skin.v = cfg['skin']
    sub().commit_skin_settings(character=ch, skin_settings=skin)
    eyes = ch.eyes_settings
    for eye in (eyes.eye_left, eyes.eye_right):
        eye.iris.primary_color_u, eye.iris.primary_color_v = cfg['eyes']
        eye.iris.secondary_color_u, eye.iris.secondary_color_v = cfg['eyes']
    sub().commit_eyes_settings(character=ch, eyes_settings=eyes)
    chosen = {'Hair': cfg['hair']}
    if cfg.get('eyebrows'):
        chosen['Eyebrows'] = cfg['eyebrows']
    for slot, name in chosen.items():
        _select(ch, slot, _wardrobe_item(slot, name))
    unreal.EditorAssetLibrary.save_loaded_asset(ch)
    return log('make_waitress', preset=preset, cfg=cfg, path=CHAR_PATH)


@guarded
def preview_capture(tag='waitress', yaw=0.0):
    studio()
    ch = unreal.load_asset(CHAR_PATH)
    _open(ch)
    actor = _show(ch, yaw)
    shots = [capture('face', f'{tag}_face'), capture('body', f'{tag}_body')]
    actor.destroy_actor()
    return log('preview_capture', shots=shots)


@guarded
def export_preview_body(path=BASE + '/blender/waitress_body.fbx'):
    """The previewed body (and its skeleton) as FBX, for modelling the uniform onto this exact body."""
    ch = unreal.load_asset(CHAR_PATH)
    _open(ch)
    actor = _show(ch)
    body = None
    for c in actor.get_components_by_class(unreal.SkeletalMeshComponent):
        mesh = c.get_skeletal_mesh_asset() if hasattr(c, 'get_skeletal_mesh_asset') else c.skeletal_mesh
        if mesh and 'body' in c.get_name().lower():
            body = mesh
    if not body:
        raise RuntimeError('no body mesh on preview actor')
    task = unreal.AssetExportTask()
    task.object = body
    task.filename = path
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.options = unreal.FbxExportOption()
    ok = unreal.Exporter.run_asset_export_task(task)
    actor.destroy_actor()
    return log('export_preview_body', ok=ok, mesh=body.get_path_name(), file=path)


# ---------------------------------------------------------------- voice and face animation

def waitress_takes():
    """Take ids whose speaker is WAITRESS, from the authored EveningShift modules."""
    root = '/home/tom/SIGNAL-47/unreal/Narrative/EveningShift/modules'
    ids = []

    def walk(o):
        if isinstance(o, dict):
            if o.get('speaker') == 'WAITRESS' and o.get('id'):
                ids.append(o['id'])
            for v in o.values():
                walk(v)
        elif isinstance(o, list):
            for v in o:
                walk(v)
    for f in sorted(os.listdir(root)):
        if f.endswith('.json'):
            walk(json.load(open(os.path.join(root, f))))
    return [i for i in ids if os.path.exists(f'{TAKES}/{i}.wav')]


@guarded
def import_takes():
    ids = waitress_takes()
    tasks = []
    for i in ids:
        t = unreal.AssetImportTask()
        t.filename = f'{TAKES}/{i}.wav'
        t.destination_path = AUDIO_DIR
        t.destination_name = i
        t.automated = True
        t.replace_existing = True
        t.save = True
        tasks.append(t)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    got = [i for i in ids if unreal.EditorAssetLibrary.does_asset_exist(f'{AUDIO_DIR}/{i}')]
    return log('import_takes', wanted=len(ids), imported=len(got), missing=sorted(set(ids) - set(got)))


@guarded
def speech2face(ids=None, mood='Auto', skeleton=None):
    """Local audio-driven animation: full face, generated blinks, head movement as curves."""
    ids = ids or waitress_takes()
    # After assembly the game-side copy of the face skeleton exists; before it, the plugin's.
    skeleton = skeleton or (COMMON_FACE_SKELETON if unreal.EditorAssetLibrary.does_asset_exist(COMMON_FACE_SKELETON.split('.')[0]) else PLUGIN_FACE_SKELETON)
    target = unreal.load_asset(skeleton)
    out = []
    for i in ids:
        anim_path = f'{FACE_ANIM_DIR}/AS_{i}'
        if unreal.EditorAssetLibrary.does_asset_exist(anim_path):
            out.append({'id': i, 'skipped': True})
            continue
        wave = unreal.load_asset(f'{AUDIO_DIR}/{i}')
        perf = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name=f'MHP_{i}', package_path=PERF_DIR, asset_class=unreal.MetaHumanPerformance,
            factory=unreal.MetaHumanPerformanceFactoryNew())
        perf.set_editor_property('input_type', unreal.DataInputType.AUDIO)
        perf.set_editor_property('audio', wave)
        ov = unreal.AudioDrivenAnimationSolveOverrides()
        ov.mood = getattr(unreal.AudioDrivenAnimationMood, {'Auto': 'AUTO_DETECT'}.get(mood, mood.upper()))
        ov.mood_intensity = 0.6  # a tired evening shift, not a performance
        perf.set_editor_property('audio_driven_animation_solve_overrides', ov)
        perf.set_editor_property('audio_driven_animation_output_controls', unreal.AudioDrivenAnimationOutputControls.FULL_FACE)
        perf.set_editor_property('head_movement_mode', unreal.PerformanceHeadMovementMode.TRANSFORM_TRACK)
        perf.set_blocking_processing(True)
        err = perf.start_pipeline()
        s = unreal.MetaHumanPerformanceExportAnimationSettings()
        s.show_export_dialog = False
        s.package_path = FACE_ANIM_DIR
        s.asset_name = f'AS_{i}'
        s.target_skeleton_or_skeletal_mesh = target
        s.enable_head_movement = True
        s.export_range = unreal.PerformanceExportRange.PROCESSING_RANGE
        anim = unreal.MetaHumanPerformanceExportUtils.export_animation_sequence(perf, s)
        out.append({'id': i, 'pipeline': str(err), 'anim': anim.get_path_name() if anim else None,
                    'length': anim.get_play_length() if anim else None})
        unreal.EditorAssetLibrary.save_directory(FACE_ANIM_DIR)
    return log('speech2face', count=len(out), skeleton=skeleton, results=out)


# ---------------------------------------------------------------- cloud and assembly

@guarded
def rig(kind='JOINTS_AND_BLEND_SHAPES'):
    ch = unreal.load_asset(CHAR_PATH)
    _open(ch)
    p = unreal.MetaHumanCharacterAutoRiggingRequestParams()
    p.blocking = True
    p.report_progress = True
    p.rig_type = getattr(unreal.MetaHumanRigType, kind)
    sub().request_auto_rigging(character=ch, params=p)
    unreal.EditorAssetLibrary.save_loaded_asset(ch)
    return log('rig', kind=kind, can_build=sub().can_build_meta_human(character=ch, log_error=True))


@guarded
def rig_async(kind='JOINTS_AND_BLEND_SHAPES'):
    """Non-blocking request: lets the editor tick, so Epic's login page can complete."""
    ch = unreal.load_asset(CHAR_PATH)
    _open(ch)
    p = unreal.MetaHumanCharacterAutoRiggingRequestParams()
    p.blocking = False
    p.report_progress = True
    p.rig_type = getattr(unreal.MetaHumanRigType, kind)
    sub().request_auto_rigging(character=ch, params=p)
    return log('rig_async', kind=kind, note='login may open in the browser; watch the editor notification')


@guarded
def textures():
    ch = unreal.load_asset(CHAR_PATH)
    _open(ch)
    p = unreal.MetaHumanCharacterTextureRequestParams()
    p.blocking = True
    p.report_progress = True
    sub().request_texture_sources(character=ch, params=p)
    unreal.EditorAssetLibrary.save_loaded_asset(ch)
    return log('textures', high_res=ch.has_high_resolution_textures,
               can_build=sub().can_build_meta_human(character=ch, log_error=True))


@guarded
def build(quality='HIGH'):
    ch = unreal.load_asset(CHAR_PATH)
    _open(ch)
    if not sub().can_build_meta_human(character=ch, log_error=True):
        return log('build', built=False, reason='not buildable; see the MetaHuman log line above')
    p = unreal.MetaHumanCharacterEditorBuildParameters()
    p.pipeline_type = unreal.MetaHumanDefaultPipelineType.OPTIMIZED
    p.pipeline_quality = getattr(unreal.MetaHumanQualityLevel, quality)
    p.absolute_build_path = MH_ROOT
    p.common_folder_path = MH_ROOT + '/Common'
    p.enable_wardrobe_item_validation = True
    sub().build_meta_human(character=ch, params=p)
    unreal.EditorAssetLibrary.save_directory(MH_ROOT)
    built = sorted(str(a.package_name) for a in assets(MH_ROOT) if 'Common' not in str(a.package_name))
    return log('build', built=True, quality=quality, assets=built[:200], count=len(built))


@guarded
def copy_idles():
    """The idle pair under the waitress, where the runtime looks for them."""
    done = {}
    for src, name in ((IDLE_BODY, 'AS_Waitress_Idle'), (IDLE_FACE, 'AS_Waitress_IdleFace')):
        dst = f'{ANIM_DIR}/{name}'
        a = _duplicate(src, dst)
        done[name] = {'skeleton': a.get_editor_property('skeleton').get_path_name() if a else None,
                      'length': a.get_play_length() if a else None}
    unreal.EditorAssetLibrary.save_directory(ANIM_DIR)
    return log('copy_idles', anims=done)


@guarded
def skin(face_index=None, u=None, v=None, roughness=None):
    """Change the shown character's skin; the preview actor follows after a few ticks."""
    ch = _shown['character']
    s = ch.skin_settings
    if face_index is not None:
        s.skin.face_texture_index = int(face_index)
    if u is not None:
        s.skin.u = u
    if v is not None:
        s.skin.v = v
    if roughness is not None:
        s.skin.roughness = roughness
    sub().commit_skin_settings(character=ch, skin_settings=s)
    return log('skin', face_index=s.skin.face_texture_index, u=s.skin.u, v=s.skin.v)


@guarded
def blend_face(weights):
    """Face shape as a weighted mix of presets' face model coefficients, e.g. {'Vivian': .7, 'Grace': .3}."""
    ch = _shown['character']
    total = None
    for name, w in weights.items():
        src = unreal.load_asset(f'{PRESETS}/{name}')
        _open(src)
        c = list(sub().get_face_model_coefficients(src))
        sub().remove_object_to_edit(src)
        total = [w * x for x in c] if total is None else [t + w * x for t, x in zip(total, c)]
    sub().set_face_model_coefficients(ch, total)
    sub().commit_face_state(character=ch)
    return log('blend_face', weights=weights, coefficients=len(total))


def clear_all():
    """Destroy every MetaHuman preview actor in the level, remembered or not."""
    n = 0
    for a in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.Actor):  # previews are transient
        if 'MetaHuman' in a.get_class().get_name():
            a.destroy_actor(); n += 1
    _shown.clear()
    return n


@guarded
def hair(name, slot='Hair'):
    """Select a groom on the shown character through its preview collection (Epic's own route)."""
    ch = _shown['character']
    col = sub().get_preview_collection(ch)
    item = unreal.load_asset(f'{GROOMS}/{slot}/{name}')
    key = col.try_add_item_from_wardrobe_item(slot_name=slot, wardrobe_item=item)
    col.default_instance.set_single_slot_selection(slot_name=slot, item_key=key)
    sub().on_edit_preview_collection(ch)
    sub().assemble_for_preview(character=ch)
    _shown['hair_key'] = key
    params = col.default_instance.get_instance_parameters(item_path=unreal.MetaHumanPaletteItemPath(item_key=key))
    return log('hair', name=name, params=[(str(p.name), str(p.type)) for p in params])


@guarded
def hair_params(values):
    """Set float/colour instance parameters on the selected hair, e.g. {'Melanin': .6}."""
    ch = _shown['character']
    col = sub().get_preview_collection(ch)
    params = col.default_instance.get_instance_parameters(item_path=unreal.MetaHumanPaletteItemPath(item_key=_shown['hair_key']))
    done = {}
    for p in params:
        n = str(p.name)
        if n in values:
            v = values[n]
            if p.type == unreal.MetaHumanCharacterInstanceParameterType.FLOAT:
                p.set_float(value=float(v))
            elif p.type == unreal.MetaHumanCharacterInstanceParameterType.BOOL:
                p.set_bool(value=bool(v))
            elif p.type == unreal.MetaHumanCharacterInstanceParameterType.COLOR:
                p.set_color(value=unreal.LinearColor(*v))
            done[n] = v
    sub().on_edit_preview_collection(ch)
    return log('hair_params', set=done)


@guarded
def rebuild_waitress(preset='Vivian', face_index=60, skin_uv=(0.33, 0.62), eyes=(0.8, 0.3)):
    """Fresh character asset from a preset with an aged face texture (any earlier rig is discarded)."""
    clear_all()
    if unreal.EditorAssetLibrary.does_asset_exist(CHAR_PATH):
        old = unreal.load_asset(CHAR_PATH)
        if sub().is_object_added_for_editing(old):
            sub().remove_object_to_edit(old)
        unreal.EditorAssetLibrary.delete_asset(CHAR_PATH)
    ch = unreal.EditorAssetLibrary.duplicate_asset(f'{PRESETS}/{preset}', CHAR_PATH)
    _open(ch)
    _set_body(ch, WAITRESS['body'])
    s = ch.skin_settings
    s.skin.face_texture_index = face_index
    s.skin.u, s.skin.v = skin_uv
    sub().commit_skin_settings(character=ch, skin_settings=s)
    e = ch.eyes_settings
    for eye in (e.eye_left, e.eye_right):
        eye.iris.primary_color_u, eye.iris.primary_color_v = eyes
        eye.iris.secondary_color_u, eye.iris.secondary_color_v = eyes
    sub().commit_eyes_settings(character=ch, eyes_settings=e)
    unreal.EditorAssetLibrary.save_loaded_asset(ch)
    return log('rebuild_waitress', preset=preset, face_index=face_index, skin=skin_uv, eyes=eyes)


@guarded
def makeup_1986():
    """A working day's makeup: faint warm-brown eyes, little blush, a muted lip."""
    ch = _shown['character']
    m = ch.makeup_settings
    m.eyes.opacity = 0.22
    m.eyes.primary_color = unreal.LinearColor(0.05, 0.03, 0.02, 1)
    m.blush.intensity = 0.12
    m.lips.type = unreal.MetaHumanCharacterLipsMakeupType.NATURAL if hasattr(unreal.MetaHumanCharacterLipsMakeupType, 'NATURAL') else m.lips.type
    m.lips.color = unreal.LinearColor(0.20, 0.06, 0.05, 1)
    m.lips.opacity = 0.25
    m.lips.metalness = 0.0
    m.lips.roughness = 0.55
    sub().commit_makeup_settings(character=ch, makeup_settings=m)
    return log('makeup_1986', lips=str(m.lips.type))


@guarded
def textures_async():
    ch = unreal.load_asset(CHAR_PATH)
    _open(ch)
    p = unreal.MetaHumanCharacterTextureRequestParams()
    p.blocking = False
    p.report_progress = True
    sub().request_texture_sources(character=ch, params=p)
    return log('textures_async', requested=True)


def status():
    ch = unreal.load_asset(CHAR_PATH)
    _open(ch)
    return {'high_res': ch.has_high_resolution_textures, 'can_build': sub().can_build_meta_human(character=ch)}


BODY_MESH = '/Game/MetaHumans/MHC_Waitress/Body/SKM_MHC_Waitress_BodyMesh'
RTG_MH = '/MetaHumanCharacter/Animation/Retargeting/RTG_MH_IKRig'
PLUGIN_BODY_SKEL = '/MetaHumanCharacter/Female/Medium/NormalWeight/Body/metahuman_base_skel'


def _plugin_body_mesh():
    """Any plugin skeletal mesh on the plugin body skeleton, as the retarget source."""
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    flt = unreal.ARFilter(package_paths=['/MetaHumanCharacter'], recursive_paths=True,
                          class_paths=[unreal.SkeletalMesh.static_class().get_class_path_name()])
    for a in reg.get_assets(flt):
        m = a.get_asset()
        if m and m.skeleton and m.skeleton.get_path_name().startswith(PLUGIN_BODY_SKEL):
            return m
    return None


@guarded
def idles_to_waitress():
    """Body idle retargeted onto her body (MetaHuman to MetaHuman); face idle curves copied onto the
    assembled face skeleton. Both end up under ANIM_DIR, owned by her own skeletons."""
    out = {}
    tmp = '/Game/S47/Tmp'
    src_body = _duplicate(IDLE_BODY, f'{tmp}/AS_IdleBodySrc')
    source = _plugin_body_mesh()
    target = unreal.load_asset(BODY_MESH)
    rtg = unreal.load_asset(RTG_MH)
    ad = unreal.AssetRegistryHelpers.get_asset_registry().get_asset_by_object_path(src_body.get_path_name())
    _tolerant(lambda: unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        [ad], source, target, rtg, search='', replace='', prefix='', suffix='_W', target_path=tmp,
        use_source_path=False, include_referenced_assets=False, overwrite_existing_files=True))
    paths = [p for p in (f'{tmp}/AS_IdleBodySrc_W',) if unreal.EditorAssetLibrary.does_asset_exist(p)]
    body = next((p for p in paths if p.endswith('_W')), None)
    dst = f'{ANIM_DIR}/AS_Waitress_Idle'
    if body:
        if unreal.EditorAssetLibrary.does_asset_exist(dst):
            unreal.EditorAssetLibrary.delete_asset(dst)
        unreal.EditorAssetLibrary.rename_asset(body, dst)
    a = unreal.load_asset(dst)
    out['body'] = {'source_mesh': source.get_path_name() if source else None, 'made': paths,
                   'skeleton': a.get_editor_property('skeleton').get_path_name() if a else None, 'length': a.get_play_length() if a else None}
    # Face: curves only, onto the assembled face skeleton.
    src = unreal.load_asset(IDLE_FACE)
    fdst = f'{ANIM_DIR}/AS_Waitress_IdleFace'
    if unreal.EditorAssetLibrary.does_asset_exist(fdst):
        unreal.EditorAssetLibrary.delete_asset(fdst)
    fac = unreal.AnimSequenceFactory()
    fac.set_editor_property('target_skeleton', unreal.load_asset(COMMON_FACE_SKELETON))
    face = unreal.AssetToolsHelpers.get_asset_tools().create_asset('AS_Waitress_IdleFace', ANIM_DIR, unreal.AnimSequence, fac)
    frames = unreal.AnimationLibrary.get_num_frames(src)
    length = unreal.AnimationLibrary.get_sequence_length(src)
    fps = round(frames / length) if length else 30
    ctrl = face.controller
    ctrl.open_bracket(unreal.Text('S47 face idle'), False)
    ctrl.set_frame_rate(unreal.FrameRate(fps, 1), False)
    ctrl.set_number_of_frames(unreal.FrameNumber(frames), False)
    names = unreal.AnimationLibrary.get_animation_curve_names(src, unreal.RawCurveTrackTypes.RCT_FLOAT)
    for n in names:
        times, values = unreal.AnimationLibrary.get_float_keys(src, n)
        unreal.AnimationLibrary.add_curve(face, n, unreal.RawCurveTrackTypes.RCT_FLOAT, False)
        unreal.AnimationLibrary.add_float_curve_keys(face, n, times, values)
    ctrl.close_bracket(False)
    out['face'] = {'curves': len(names), 'frames': frames, 'fps': fps, 'length': face.get_play_length()}
    unreal.EditorAssetLibrary.save_directory(ANIM_DIR)
    return log('idles_to_waitress', **out)
