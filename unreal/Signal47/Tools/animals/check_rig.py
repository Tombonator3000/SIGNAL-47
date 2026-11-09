"""Read only (PT-FARM-GROUND-20261109): do the rigged animals' mesh, skeleton, clips and materials agree?

Tom saw something big and grey moving through the ground by the farm at night. The packaged log has the dog's and the coyote's
materials refused on a skeletal mesh (the game's M_Web_Lit_Opaque has no skeletal usage, so the engine's grey was drawn), and
the dog's mesh bounds reading 38936 units along the skeleton's up: about its length, not its height. This says, for each animal,
without changing or saving anything:

  - the mesh's bounds, and where the paws and the head of the reference pose stand in them along the skeleton's up (S47Animals
    shows a rig only when the paws are at the foot of the mesh and the head well up in it: RigFramesAgree)
  - how far the clips' first frame moves the root and the Body bone from the reference pose (a clip in another frame turns or
    scales the whole animal)
  - each material slot, its parent and whether that parent may be drawn on a skeletal mesh

Run in the editor's Python console or headless, one heavy job at a time and never while Tom plays:
  UnrealEditor-Cmd Signal47.uproject -run=pythonscript -script=Tools/animals/check_rig.py -unattended -nosplash -nullrhi
"""
import unreal

E = unreal.EditorAssetLibrary
PAWS = (('FF.L', 'FF.R', 'FFB.L', 'FFB.R'), ('FrontLowerLeg.L', 'FrontLowerLeg.R', 'BackLowerLeg.L', 'BackLowerLeg.R'))


def log(s):
    unreal.log_warning('S47RIGCHECK ' + s)


def vec(v):
    return '(%.1f %.1f %.1f)' % (v.x, v.y, v.z)


def bone_names(skeleton_pose):
    try:
        return [str(n) for n in unreal.AnimPoseExtensions.get_bone_names(skeleton_pose)]
    except Exception:
        return []


def find(names, bone):
    for spelt in (bone, bone.replace('.', '_'), bone.replace('_', '.')):
        if spelt in names:
            return spelt
    return None


def where(pose, bone):
    t = unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)
    return t.translation


def check(animal):
    path = '/Game/Free/Animals/%s/SK_%s' % (animal, animal)
    mesh = E.load_asset(path)
    if not mesh:
        log('%s: no mesh at %s' % (animal, path))
        return
    skeleton = mesh.get_editor_property('skeleton')
    bounds = mesh.get_bounds()
    log('%s bounds origin %s extent %s' % (animal, vec(bounds.origin), vec(bounds.box_extent)))
    ref = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
    names = bone_names(ref)
    log('%s %d bones, first %s' % (animal, len(names), names[:4]))
    paws = None
    for row in PAWS:
        found = [find(names, b) for b in row]
        if all(found):
            paws = found
            break
    head = find(names, 'Head')
    if not paws or not head:
        log('%s: no paws or head among the bones' % animal)
    else:
        p = [where(ref, b) for b in paws]
        h = where(ref, head)
        at = unreal.Vector((p[0].x + p[1].x + p[2].x + p[3].x) / 4, (p[0].y + p[1].y + p[2].y + p[3].y) / 4, (p[0].z + p[1].z + p[2].z + p[3].z) / 4)
        ahead = ((p[0] + p[1]) - (p[2] + p[3])).normal()
        across = ((p[0] + p[2]) - (p[1] + p[3])).normal()
        up = ahead.cross(across).normal()
        if up.dot(h - at) < 0:
            up = -up
        e = bounds.box_extent
        tall = 2 * (abs(up.x) * e.x + abs(up.y) * e.y + abs(up.z) * e.z)
        sole = bounds.origin.dot(up) - tall / 2
        pf, hf = (at.dot(up) - sole) / tall, (h.dot(up) - sole) / tall
        agree = -.05 < pf < .2 and .35 < hf < 1.05
        log('%s up %s ahead %s; bounds along up %.0f; paws %.0f %% and head %.0f %% up the mesh: %s' % (
            animal, vec(up), vec(ahead), tall, pf * 100, hf * 100, 'ONE FRAME' if agree else 'DIFFERENT FRAMES (S47Animals will not show it)'))
        log('%s paws %s head %s (component space, reference pose)' % (animal, vec(at), vec(h)))
    for clip in ('Idle', 'Idle2', 'Attack', 'HeadLow', 'Walk', 'Gallop'):
        seq = E.load_asset('/Game/Free/Animals/%s/A_%s_%s' % (animal, animal, clip))
        if not seq:
            continue
        try:
            pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, 0.0, unreal.AnimPoseEvaluationOptions())
        except Exception as err:
            log('%s %s: pose not read (%s)' % (animal, clip, err))
            continue
        for bone in names[:2] + ['Body']:
            if bone not in names:
                continue
            a = unreal.AnimPoseExtensions.get_bone_pose(ref, bone, unreal.AnimPoseSpaces.WORLD)
            b = unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)
            turn = a.rotation.quaternion().angular_distance(b.rotation.quaternion()) * 57.2958
            log('%s %s %s: moved %.1f, turned %.1f deg, scale %s against %s' % (animal, clip, bone, (a.translation - b.translation).length(), turn, vec(b.scale3d), vec(a.scale3d)))
    for i, slot in enumerate(mesh.get_editor_property('materials')):
        mat = slot.get_editor_property('material_interface')
        base = mat.get_base_material() if mat else None
        usage = base.get_editor_property('used_with_skeletal_mesh') if base else None
        log('%s slot %d %s: %s, parent %s, skeletal usage %s' % (animal, i, slot.get_editor_property('material_slot_name'),
                                                                 mat.get_path_name() if mat else None, base.get_path_name() if base else None, usage))
    lit = E.load_asset('/Game/Web/Base/M_Web_Lit_Opaque')
    if lit:
        log('M_Web_Lit_Opaque skeletal usage %s (the game used it on the rigs before 9 November)' % lit.get_editor_property('used_with_skeletal_mesh'))


for animal in ('Dog', 'Coyote'):
    try:
        check(animal)
    except Exception as err:
        log('%s: check stopped (%s)' % (animal, err))
log('DONE (nothing changed or saved)')
