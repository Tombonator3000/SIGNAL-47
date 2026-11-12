"""ANIM-TRY-20261112 (-> M4, M1): retarget Free100 walk/idle/jog/sit from the UE5 mannequin onto our NPC skeletons.

Runs in Signal47.uproject (it needs our target meshes), editor only, after import.sh has put the pack in /Game/Licensed/Free100:

    UnrealEditor-Cmd Signal47.uproject -run=pythonscript -script=Tools/animtry/retarget.py -unattended -nullrhi -nosplash -NoSound

It writes ONLY under /Game/Licensed/Free100 (git-ignored): IK Rigs and IK Retargeters in Rigs/, clips in Retargeted/<role>/ named the
way FS47ClipBank reads them (A_F100_<role>_<clip>, ClipFile in S47CreatureClips.cpp). Before saving it refuses if any other package is
dirty. Nothing in TripoNPC, MetaHumans or anywhere else is saved; today's Tripo clips stay as they are.

Steps, per target (selection.json "targets"):
  1. IK Rig for the source (SKM_Manny_Simple) and the target, with the same hand-made chain names on both (Spine, Neck, Head,
     Left/RightClavicle, Left/RightArm, Left/RightLeg; pelvis is the retarget root), foot and hand goals, and a full-body IK solver
     when the engine gives one. The Tripo townsfolk have 3 spine bones and one neck bone (23 bones), the mannequin and MetaHuman 5 and 2:
     the chain names stay the same, the bones differ (rig "tripo23" / "mannequin").
  2. IK Retargeter: both rigs and preview meshes, the default op stack (pelvis motion, FK chains, IK chains, IK solve, root motion),
     chains mapped by exact name, target retarget pose auto-aligned to the source's (T-pose and A-pose meet).
  3. One batch retarget per clip, renamed to our name.
  4. Gaits (walk, run): the clip player plays a walk by the distance the root really goes and reads that travel off the PELVIS (the
     skin's root bone is held at rest, S47CreatureClips.cpp). So any travel in the root track is moved into the pelvis and the root is
     held at rest; and if the clip walks on the spot (the template's are in-place), the travel is put back from the feet: the planted
     foot's backward speed is the walk's speed. Afterwards a planted foot stands still in the clip's own space, as ReadGait expects.
  5. Report (S47_ANIMTRY_RETARGET_REPORT, default Tools/animtry/retarget-report.json): names and numbers only (length, frames,
     pelvis travel, cycle cm, speed m/s, lowest foot, stance share per foot, sit hip drop), may go into Git.

Environment:
  S47_ANIMTRY_TARGETS  comma list of selection.json targets (default: those with "first": true, the deputy)
  S47_ANIMTRY_CLIPS    comma list of our clip names (default walk,idle,run,sit; a target's "clips_only" narrows it)
  S47_ANIMTRY_EXTRAS   1: also the extras (gestures), under A_F100_<role>_x_<name>
  S47_ANIMTRY_FORCE    1: an earlier rig/retargeter/clip of the same name under /Game/Licensed/Free100 is deleted and made again
  S47_ANIMTRY_MODE     retarget (default) or verify (only step 5, in a fresh process)
"""
import json
import os
import traceback
from pathlib import Path

import unreal

HERE = Path(__file__).resolve().parent
SEL = json.loads((HERE / 'selection.json').read_text())
DEST = SEL['dest']
RIGS = DEST + '/Rigs'
OUT = DEST + '/Retargeted'
MODE = os.environ.get('S47_ANIMTRY_MODE', 'retarget')
REPORT = Path(os.environ.get('S47_ANIMTRY_RETARGET_REPORT', str(HERE / 'retarget-report.json')))
FORCE = os.environ.get('S47_ANIMTRY_FORCE') == '1'
EXTRAS = os.environ.get('S47_ANIMTRY_EXTRAS') == '1'
E = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
report = {'status': 'RUNNING', 'mode': MODE, 'engine': unreal.SystemLibrary.get_engine_version(), 'checks': [], 'targets': {}}

# Same chain names on both sides; (start, end, goal bone or None).
CHAINS = {
    'mannequin': {'Spine': ('spine_01', 'spine_05', None), 'Neck': ('neck_01', 'neck_02', None), 'Head': ('head', 'head', None)},
    'tripo23': {'Spine': ('spine_01', 'spine_03', None), 'Neck': ('neck_01', 'neck_01', None), 'Head': ('head', 'head', None)},
}
for kind in CHAINS:
    for side, s in (('Left', 'l'), ('Right', 'r')):
        CHAINS[kind][side + 'Clavicle'] = ('clavicle_' + s, 'clavicle_' + s, None)
        CHAINS[kind][side + 'Arm'] = ('upperarm_' + s, 'hand_' + s, 'hand_' + s)
        CHAINS[kind][side + 'Leg'] = ('thigh_' + s, 'ball_' + s, 'foot_' + s)
FBIK = ('/Script/IKRig.IKRigFullBodyIKSolver', '/Script/IKRig.IKRigFBIKSolver', '/Script/IKRig.FullBodyIKSolver')


def flush():
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=1, sort_keys=True, default=str))


def check(name, ok, detail=None):
    report['checks'].append({'name': name, 'pass': bool(ok), 'detail': detail})
    flush()
    if not ok:
        raise RuntimeError('CHECK FAILED: ' + name + ' ' + json.dumps(detail, default=str))


def enum(owner, *names):
    cls = getattr(unreal, owner)
    for n in names:
        if hasattr(cls, n):
            return getattr(cls, n)
    raise RuntimeError('no %s among %s' % (owner, names))


SOURCE = enum('RetargetSourceOrTarget', 'SOURCE')
TARGET = enum('RetargetSourceOrTarget', 'TARGET')


def fresh(path):
    """Ours to make: under /Game/Licensed/Free100, and not there (or FORCE)."""
    check('writes only under ' + DEST + ': ' + path, path.startswith(DEST + '/'), path)
    if E.does_asset_exist(path):
        if not FORCE:
            raise RuntimeError(path + ' is there already; S47_ANIMTRY_FORCE=1 to make it again')
        check('delete earlier ' + path, E.delete_asset(path))


def make_rig(name, mesh, kind):
    path = RIGS + '/IK_' + name
    fresh(path)
    rig = TOOLS.create_asset('IK_' + name, RIGS, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(rig)
    check(name + ': rig takes the mesh', c.set_skeletal_mesh(mesh), mesh.get_path_name())
    check(name + ': pelvis is the retarget root', c.set_retarget_root('pelvis'))
    facts = {'kind': kind, 'chains': {}, 'solver': None}
    goals = []
    for chain, (start, end, goal_bone) in CHAINS[kind].items():
        goal = ''
        if goal_bone:
            goal = str(c.add_new_goal(chain + 'Goal', goal_bone))
            goals.append(goal)
        got = str(c.add_retarget_chain(chain, start, end, goal))
        check(name + ': chain ' + chain, got == chain, {'got': got, 'start': start, 'end': end})
        facts['chains'][chain] = [start, end, goal_bone]
    # Full-body IK so the feet can land where the source's land (the IK Chains op); without a solver the retarget is FK only.
    for solver_type in FBIK:
        try:
            index = c.add_solver(solver_type)
        except Exception:
            index = -1
        if index is not None and index >= 0:
            c.set_start_bone('pelvis', index)
            for goal in goals:
                c.connect_goal_to_solver(goal, index)
            facts['solver'] = solver_type
            break
    E.save_loaded_asset(rig, only_if_is_dirty=False)
    return rig, facts


def make_retargeter(name, src_rig, src_mesh, tgt_rig, tgt_mesh):
    path = RIGS + '/RTG_' + name
    fresh(path)
    rtg = TOOLS.create_asset('RTG_' + name, RIGS, unreal.IKRetargeter, unreal.IKRetargetFactory())
    c = unreal.IKRetargeterController.get_controller(rtg)
    c.set_ik_rig(SOURCE, src_rig)
    c.set_ik_rig(TARGET, tgt_rig)
    c.set_preview_mesh(SOURCE, src_mesh)
    c.set_preview_mesh(TARGET, tgt_mesh)
    c.add_default_ops()
    c.assign_ik_rig_to_all_ops(SOURCE, src_rig)
    c.assign_ik_rig_to_all_ops(TARGET, tgt_rig)
    c.auto_map_chains(enum('AutoMapChainType', 'EXACT'), True)
    mapped = {chain: str(c.get_source_chain(chain)) for chain in CHAINS['mannequin']}
    check(name + ': every chain mapped by name', all(mapped[k] == k for k in mapped), mapped)
    c.auto_align_all_bones(TARGET, enum('RetargetAutoAlignMethod', 'CHAIN_TO_CHAIN'))
    E.save_loaded_asset(rtg, only_if_is_dirty=False)
    return rtg, {'ops': c.get_num_retarget_ops(), 'mapped': mapped}


def retarget(src_path, new_name, folder, src_mesh, tgt_mesh, rtg):
    fresh(folder + '/' + new_name)
    inputs = unreal.IKRetargetBatchOperationInputs()
    inputs.set_editor_property('assets_to_retarget', [E.find_asset_data(src_path)])
    inputs.set_editor_property('source_mesh', src_mesh)
    inputs.set_editor_property('target_mesh', tgt_mesh)
    inputs.set_editor_property('ik_retarget_asset', rtg)
    inputs.set_editor_property('search', src_path.split('/')[-1])
    inputs.set_editor_property('replace', new_name)
    inputs.set_editor_property('target_path', folder)
    inputs.set_editor_property('use_source_path', False)
    inputs.set_editor_property('include_referenced_assets', False)
    inputs.set_editor_property('overwrite_existing_files', True)
    made = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs) or []
    paths = [str(d.package_name) for d in made]
    check('retarget ' + new_name, folder + '/' + new_name in paths, paths)
    return E.load_asset(folder + '/' + new_name)


# ---------- reading and fixing a clip ----------

def opts():
    o = unreal.AnimPoseEvaluationOptions()
    o.set_editor_property('extract_root_motion', False)
    o.set_editor_property('incorporate_root_motion_into_pose', True)
    return o


def keys(seq):
    return int(unreal.AnimationLibrary.get_num_keys(seq))


def poses(seq):
    return [unreal.AnimPoseExtensions.get_anim_pose_at_frame(seq, i, opts()) for i in range(keys(seq))]


def at(pose, bone, world=True):
    return unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD if world else unreal.AnimPoseSpaces.LOCAL)


def write_track(seq, bone, transforms):
    ctl = seq.get_editor_property('controller')
    ok = ctl.set_bone_track_keys(bone, [t.translation for t in transforms], [t.rotation for t in transforms],
                                 [t.scale3d for t in transforms])
    check('%s: write %s track' % (seq.get_name(), bone), ok)


def fix_gait(seq):
    """Travel into the pelvis, root at rest; from the feet if the clip walks on the spot. Returns what was done."""
    done = []
    frames = poses(seq)
    root = [at(p, 'root', False) for p in frames]
    moved = abs(root[-1].translation.x - root[0].translation.x) + abs(root[-1].translation.y - root[0].translation.y)
    if moved > 1.0:
        # pelvis in component space (root * pelvis) becomes the pelvis' local transform under a root held at its rest pose
        rest = unreal.AnimPoseExtensions.get_ref_bone_pose(frames[0], 'root', unreal.AnimPoseSpaces.LOCAL)
        inv = rest.inverse()
        write_track(seq, 'pelvis', [unreal.MathLibrary.compose_transforms(at(p, 'pelvis', True), inv) for p in frames])
        write_track(seq, 'root', [rest for _ in frames])
        done.append('root travel %.0f cm moved into the pelvis' % moved)
        frames = poses(seq)
    pelvis = [at(p, 'pelvis', True).translation for p in frames]
    travel = ((pelvis[-1].x - pelvis[0].x) ** 2 + (pelvis[-1].y - pelvis[0].y) ** 2) ** .5
    if travel < 5.0:
        rate = (len(frames) - 1) / max(.01, float(seq.get_play_length()))
        vel = []
        for foot in ('foot_l', 'foot_r'):
            p = [at(f, foot, True).translation for f in frames]
            low = min(v.z for v in p)
            for i in range(len(p) - 1):
                if p[i].z < low + 3.0 and p[i + 1].z < low + 3.0:
                    vel.append(((p[i + 1].x - p[i].x) * rate, (p[i + 1].y - p[i].y) * rate))
        check(seq.get_name() + ': planted frames found for an on-the-spot walk', len(vel) >= 4, len(vel))
        vx = sorted(v[0] for v in vel)[len(vel) // 2]
        vy = sorted(v[1] for v in vel)[len(vel) // 2]
        # the ground goes back under a planted foot at the walk's speed: the body goes the other way. The speed is in component
        # space; the pelvis track is local to the root, whose rest pose may hold the file's unit and turn (Tripo): bring it over.
        rest = unreal.AnimPoseExtensions.get_ref_bone_pose(frames[0], 'root', unreal.AnimPoseSpaces.LOCAL)
        far = unreal.MathLibrary.inverse_transform_location(rest, unreal.Vector(vx, vy, 0))
        near = unreal.MathLibrary.inverse_transform_location(rest, unreal.Vector(0, 0, 0))
        lv = unreal.Vector(far.x - near.x, far.y - near.y, far.z - near.z)
        local = [at(f, 'pelvis', False) for f in frames]
        out = []
        for i, t in enumerate(local):
            s = i / rate
            out.append(unreal.Transform(unreal.Vector(t.translation.x - lv.x * s, t.translation.y - lv.y * s, t.translation.z - lv.z * s),
                                        t.rotation.rotator(), t.scale3d))
        write_track(seq, 'pelvis', out)
        done.append('on-the-spot: %.2f m/s put back from the planted feet' % ((vx * vx + vy * vy) ** .5 / 100.))
    return done


def measure(seq, clip):
    frames = poses(seq)
    n = len(frames)
    length = float(seq.get_play_length())
    pelvis = [at(p, 'pelvis', True).translation for p in frames]
    travel = ((pelvis[-1].x - pelvis[0].x) ** 2 + (pelvis[-1].y - pelvis[0].y) ** 2) ** .5
    out = {'path': seq.get_path_name().split('.')[0], 'skeleton': seq.get_skeleton().get_path_name(), 'length_s': round(length, 3),
           'keys': n, 'pelvis_travel_cm': round(travel, 1), 'speed_mps': round(travel / max(.01, length) / 100., 3),
           'pelvis_z_cm': [round(min(v.z for v in pelvis), 1), round(max(v.z for v in pelvis), 1)]}
    for foot in ('foot_l', 'foot_r', 'ball_l', 'ball_r'):
        z = [at(p, foot, True).translation.z for p in frames]
        out[foot + '_z_cm'] = [round(min(z), 1), round(max(z), 1)]
    if clip in SEL['gaits']:
        for foot in ('foot_l', 'foot_r'):
            p = [at(f, foot, True).translation for f in frames]
            low = min(v.z for v in p)
            speed = travel / max(.01, length)
            planted = 0
            for i in range(1, n - 1):
                v = (((p[i + 1].x - p[i - 1].x) ** 2 + (p[i + 1].y - p[i - 1].y) ** 2) ** .5) * (n - 1) / max(.01, length) / 2.
                planted += v < speed * .3 and p[i].z < low + travel * .04
            out[foot + '_stance_share'] = round(planted / max(1, n - 2), 2)
    return out


def targets():
    wanted = os.environ.get('S47_ANIMTRY_TARGETS')
    names = [t for t in wanted.split(',') if t] if wanted else [k for k, v in SEL['targets'].items() if v.get('first')]
    for t in names:
        check('known target ' + t, t in SEL['targets'], sorted(SEL['targets']))
    return names


def clips_for(name):
    spec = SEL['targets'][name]
    wanted = [c for c in os.environ.get('S47_ANIMTRY_CLIPS', ','.join(SEL['clips'])).split(',') if c]
    wanted = [c for c in wanted if c in spec.get('clips_only', wanted)]
    jobs = [(c, SEL['clips'][c], 'A_F100_%s_%s' % (name, c)) for c in wanted]
    if EXTRAS:
        jobs += [(x, SEL['extras'][x], 'A_F100_%s_x_%s' % (name, x)) for x in spec.get('extras_only', SEL['extras'])]
    return jobs


def run():
    check('runs in Signal47 (needs our target meshes)', unreal.Paths.get_project_file_path().endswith('Signal47.uproject'),
          unreal.Paths.get_project_file_path())
    src_mesh = E.load_asset(SEL['source_mesh'])
    check('source mesh under Free100 (run import.sh first)', isinstance(src_mesh, unreal.SkeletalMesh), SEL['source_mesh'])
    src_rig, src_facts = make_rig('Free100_Mannequin', src_mesh, 'mannequin')
    report['source'] = dict(src_facts, mesh=SEL['source_mesh'])
    for name in targets():
        spec = SEL['targets'][name]
        tgt_mesh = E.load_asset(spec['mesh'])
        check(name + ': target mesh loads', isinstance(tgt_mesh, unreal.SkeletalMesh), spec['mesh'])
        rig, rig_facts = make_rig('F100_' + name, tgt_mesh, spec['rig'])
        rtg, rtg_facts = make_retargeter('Free100_to_' + name, src_rig, src_mesh, rig, tgt_mesh)
        folder = OUT + '/' + name
        entry = report['targets'][name] = {'mesh': spec['mesh'], 'rig': rig_facts, 'retargeter': rtg_facts, 'clips': {}}
        for clip, src, new in clips_for(name):
            seq = retarget(src, new, folder, src_mesh, tgt_mesh, rtg)
            fixes = fix_gait(seq) if clip in SEL['gaits'] else []
            entry['clips'][clip] = dict(measure(seq, clip), fixes=fixes)
            flush()
    dirty = [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
    outside = [p for p in dirty if not p.startswith(DEST + '/')]
    check('nothing outside ' + DEST + ' is changed', not outside, outside)
    check('save ' + DEST, E.save_directory(DEST, only_if_is_dirty=True, recursive=True))


def verify():
    for name in targets():
        spec = SEL['targets'][name]
        tgt = E.load_asset(spec['mesh'])
        entry = report['targets'][name] = {'mesh': spec['mesh'], 'clips': {}}
        for clip, _src, new in clips_for(name):
            path = OUT + '/' + name + '/' + new
            seq = E.load_asset(path) if E.does_asset_exist(path) else None
            check(path + ' loads on the target skeleton', isinstance(seq, unreal.AnimSequence) and seq.get_skeleton() == tgt.skeleton, path)
            entry['clips'][clip] = measure(seq, clip)
            flush()
        c = entry['clips']
        if 'walk' in c:
            check(name + ': walk travels (0.6-2.2 m/s)', .6 <= c['walk']['speed_mps'] <= 2.2, c['walk']['speed_mps'])
            check(name + ': walk plants each foot (stance share 0.25-0.75)',
                  all(.25 <= c['walk'][f + '_stance_share'] <= .75 for f in ('foot_l', 'foot_r')), c['walk'])
        if 'idle' in c:
            check(name + ': idle stays on the spot (< 5 cm)', c['idle']['pelvis_travel_cm'] < 5, c['idle']['pelvis_travel_cm'])
        if 'sit' in c and 'idle' in c:
            drop = c['idle']['pelvis_z_cm'][0] - c['sit']['pelvis_z_cm'][0]
            c['sit']['hip_drop_cm'] = round(drop, 1)
            check(name + ': sit lowers the hips 25-70 cm', 25 <= drop <= 70, drop)


try:
    flush()
    if MODE == 'retarget':
        run()
        verify()
    elif MODE == 'verify':
        verify()
    else:
        raise RuntimeError('S47_ANIMTRY_MODE must be retarget or verify, not ' + MODE)
    report['status'] = 'PASS'
    flush()
    unreal.log('S47 ANIMTRY RETARGET ' + MODE.upper() + ' PASS ' + str(REPORT))
except Exception as error:
    report['status'] = 'FAIL'
    report['error'] = str(error)
    report['traceback'] = traceback.format_exc()
    flush()
    unreal.log_error('S47 ANIMTRY RETARGET ' + MODE.upper() + ' FAIL ' + str(error))
