"""Where the black of each alien's eyes is, for S47EyeGlint (Tom, 6 and 9 November: a faint white dot in each big black eye).

blender -b -t 2 --python measure_eyes.py -- <prepared .blend> [--dark .12] [--yaw deg] [--render out.png]

Reads only. The body at its rest pose, the faces whose base colour is darker than --dark (.12 was used), near the head bone and on
the face's side of it; per side the largest connected patch (a nostril or the mouth is not counted with the eye) with the patches
the painted highlights split off it, their centre as seen from in front, and the surface there found by a ray from in front. The dot
sits 1.5 mm out from that surface. Prints one line EYE {json} with each side in Unreal component centimetres relative to the head
bone, forward +Y, up +Z, as S47EyeGlint's table holds them. --yaw: the face's own turn at rest when it does not look along -Y (the
old Pale: -40). --render writes a close view of the face with the two dots drawn over it, to check by eye that they sit in the black.
Static figures (no head bone) are not measured well yet: the head is guessed from the top of the mesh.

9 November, all with --dark .12: SK_Tripo_PaleOne, SK_Tripo_TallOne, SK_Tripo_Roswell2 and SK_Tripo_Roswell straight on,
SK_Tripo_Pale with --yaw -40; pictures checked by eye.

Coordinates as the Roswell recipe: Blender metres, Z up, forward -Y; Unreal centimetres, Z up, forward +Y.
"""
import json
import sys
from collections import defaultdict

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree


def options():
    a = sys.argv[sys.argv.index('--') + 1:]
    out = {'blend': a[0], 'render': None, 'dark': .06}
    if '--render' in a:
        out['render'] = a[a.index('--render') + 1]
    if '--dark' in a:
        out['dark'] = float(a[a.index('--dark') + 1])
    out['turned'] = '--turned' in a
    # --yaw degrees: the face's own turn at rest, when it does not look along -Y (positive turns it toward +X)
    out['yaw'] = float(a[a.index('--yaw') + 1]) if '--yaw' in a else None
    return out


def base_image(material):
    nodes = material.node_tree.nodes if material and material.node_tree else []
    bsdf = next((n for n in nodes if n.type == 'BSDF_PRINCIPLED'), None)
    seen = set()

    def walk(socket):
        if not socket or not socket.is_linked:
            return None
        node = socket.links[0].from_node
        if node.as_pointer() in seen:
            return None
        seen.add(node.as_pointer())
        if node.type == 'TEX_IMAGE' and node.image:
            return node.image
        for i in node.inputs:
            found = walk(i)
            if found:
                return found
        return None

    return walk(bsdf.inputs['Base Color']) if bsdf else None


def main():
    o = options()
    bpy.ops.wm.open_mainfile(filepath=o['blend'])
    arms = [ob for ob in bpy.data.objects if ob.type == 'ARMATURE']
    for arm in arms:
        arm.data.pose_position = 'REST'
    meshes = [ob for ob in bpy.data.objects if ob.type == 'MESH' and len(ob.data.vertices) > 1000 and not ob.hide_render]
    bpy.context.view_layer.update()
    deps = bpy.context.evaluated_depsgraph_get()
    if arms and arms[0].data.bones.get('head'):
        head = arms[0].matrix_world @ arms[0].data.bones['head'].head_local
        anchor = 'head bone'
    else:
        head = None
        anchor = 'mesh origin'
    verts, faces, uvs, mats, images = [], [], [], [], {}
    for ob in meshes:
        ev = ob.evaluated_get(deps)
        me = ev.to_mesh()
        base = len(verts)
        verts += [tuple(ob.matrix_world @ v.co) for v in me.vertices]
        uv = me.uv_layers.active.data
        for p in me.polygons:
            faces.append([base + i for i in p.vertices])
            uvs.append([tuple(uv[i].uv) for i in p.loop_indices])
            m = ob.material_slots[p.material_index].material if ob.material_slots else None
            mats.append(m.name if m else '')
            if m and m.name not in images:
                images[m.name] = base_image(m)
        ev.to_mesh_clear()
    V = np.array(verts)
    lo, hi = V.min(axis=0), V.max(axis=0)
    height = hi[2] - lo[2]
    if head is None:
        # A static figure: its head is the highest part of it, near the front.
        head = Vector((float((lo[0] + hi[0]) / 2), float(V[V[:, 2] > hi[2] - .05 * height][:, 1].mean()), float(hi[2] - .06 * height)))
    head = np.array(head)
    pix = {}
    for name, img in images.items():
        if img:
            w, h = img.size
            p = np.empty(w * h * img.channels, dtype=np.float32)
            img.pixels.foreach_get(p)
            pix[name] = (p.reshape((h, w, img.channels)), w, h)
    def measure(V, forward=True):
        radius = max(.06, .11 * height)
        dark = []
        lum_hist = []
        for fi, f in enumerate(faces):
            c = V[f].mean(axis=0)
            if np.linalg.norm(c - head) > radius or (forward and c[1] > head[1]) or c[2] < head[2] - .25 * radius:
                continue
            P = pix.get(mats[fi])
            if not P:
                continue
            p, w, h = P
            u = float(np.mean([t[0] for t in uvs[fi]]))
            v = float(np.mean([t[1] for t in uvs[fi]]))
            r, g, b = p[min(h - 1, max(0, int(v % 1 * h))), min(w - 1, max(0, int(u % 1 * w))), :3]
            lum = .2126 * r + .7152 * g + .0722 * b
            lum_hist.append(lum)
            if lum < o['dark']:
                dark.append(fi)
        # connected patches over shared vertices
        by_vert = defaultdict(list)
        for fi in dark:
            for vi in faces[fi]:
                by_vert[vi].append(fi)
        patch = {}
        for fi in dark:
            if fi in patch:
                continue
            stack = [fi]
            patch[fi] = fi
            while stack:
                cur = stack.pop()
                for vi in faces[cur]:
                    for nb in by_vert[vi]:
                        if nb not in patch:
                            patch[nb] = fi
                            stack.append(nb)

        def area_normal(f):
            a, b, c = V[f[0]], V[f[1]], V[f[2]]
            n = np.cross(b - a, c - a)
            ar = np.linalg.norm(n) / 2
            if len(f) == 4:
                n2 = np.cross(V[f[2]] - a, V[f[3]] - a)
                ar += np.linalg.norm(n2) / 2
                n = n + n2
            return ar, n / (np.linalg.norm(n) + 1e-12)

        groups = defaultdict(list)
        for fi, root in patch.items():
            groups[root].append(fi)
        tree = BVHTree.FromPolygons([tuple(v) for v in V], faces)
        sides = {}
        for side, sign in (('left', 1), ('right', -1)):
            best = None
            for root, fs in groups.items():
                cs = np.array([V[faces[f]].mean(axis=0) for f in fs])
                if np.mean(np.sign(cs[:, 0] - head[0])) * sign < .5:
                    continue
                an = [area_normal(faces[f]) for f in fs]
                area = sum(a for a, _ in an)
                if best is None or area > best[0]:
                    best = (area, fs, cs, an)
            if not best:
                continue
            area, fs, cs, an = best
            # The painted highlights split one eye into several patches: the others on this side whose centre is within the
            # largest one's reach count with it (a nostril or the mouth lies further off, and lower).
            core = (cs * np.array([a for a, _ in an])[:, None]).sum(axis=0) / area
            reach = max(.025, float(np.ptp(cs, axis=0).max()))
            for root, more in groups.items():
                if more is fs or more[0] in fs:
                    continue
                mc = np.array([V[faces[f]].mean(axis=0) for f in more])
                if np.mean(np.sign(mc[:, 0] - head[0])) * sign < .5 or np.linalg.norm(mc.mean(axis=0) - core) > reach:
                    continue
                fs = fs + more
                cs = np.vstack([cs, mc])
                an = an + [area_normal(faces[f]) for f in more]
            area = sum(a for a, _ in an)
            # The centre as seen from in front: each face by the area it shows forward (-Y), so the part of the eye that wraps round
            # the side of the head does not pull the dot out of the black.
            wts = np.array([a * max(0., -n[1]) for a, n in an]) + 1e-12
            centre = (cs * wts[:, None]).sum(axis=0) / wts.sum()
            normal = sum(n * a for a, n in an)
            normal = normal / np.linalg.norm(normal)
            hit, hn, _, _ = tree.ray_cast(Vector((centre[0], centre[1] - .1, centre[2])), Vector((0, 1, 0)), .3)
            surface = np.array(hit) if hit else centre
            dot = surface + (np.array(hn) if hit else normal) * .0015
            rel = dot - head
            ext = np.ptp(cs, axis=0)
            sides[side] = {'faces': len(fs), 'area_cm2': float(area * 1e4), 'extent_cm': [float(x * 100) for x in ext],
                           'dot_ue_cm': [float(100 * rel[0]), float(-100 * rel[1]), float(100 * rel[2])],
                           'normal_ue': [float(normal[0]), float(-normal[1]), float(normal[2])], 'hit': bool(hit),
                           'dot_world': [float(x) for x in dot]}
        return sides, groups, lum_hist, dark, area_normal

    yaw = np.radians(o['yaw']) if o['yaw'] is not None else 0.
    if o['turned'] and o['yaw'] is None:
        # A head turned at rest (the old Pale): the face's own forward from its two largest dark patches, and the body turned
        # about the head so that it looks along -Y; the dots are turned back after.
        _, groups0, _, _, an0 = measure(V, False)
        big = sorted(groups0.values(), key=lambda fs: -sum(an0(faces[f])[0] for f in fs))
        cents = [np.mean([V[faces[f]].mean(axis=0) for f in fs], axis=0) for fs in big[:6]]
        pair = next(((a, b) for i, a in enumerate(cents) for b in cents[i + 1:] if 0.04 < np.linalg.norm((a - b)[:2]) < .25), None)
        if pair:
            e = pair[0] - pair[1]
            fwd = np.array([-e[1], e[0]])
            if fwd[1] > 0:
                fwd = -fwd
            yaw = float(np.arctan2(fwd[0], -fwd[1]))
    c, sn = np.cos(yaw), -np.sin(yaw)  # turns the face's forward (sin yaw, -cos yaw) onto -Y
    R = np.array([[c, -sn, 0], [sn, c, 0], [0, 0, 1]])
    sides, groups, lum_hist, dark, _ = measure((V - head) @ R.T + head, True)
    for sd in sides.values():
        back = R.T @ (np.array(sd['dot_world']) - head)
        sd['dot_world'] = [float(x) for x in back + head]
        sd['dot_ue_cm'] = [float(100 * back[0]), float(-100 * back[1]), float(100 * back[2])]
    if yaw:
        print('TURNED', json.dumps({'face_yaw_deg': float(np.degrees(yaw))}))
    h = np.array(lum_hist) if lum_hist else np.zeros(1)
    print('HIST', json.dumps({'faces_near_head': len(lum_hist), 'dark': len(dark), 'patches': len(groups),
                              'lum_p': [float(np.percentile(h, q)) for q in (1, 5, 10, 25, 50)]}))
    print('EYE', json.dumps({'blend': o['blend'], 'anchor': anchor, 'height_m': float(height),
                             'head_ue_cm': [float(100 * head[0]), float(-100 * head[1]), float(100 * head[2])], 'sides': sides}))
    if o['render']:
        render(o['render'], head, height, [s['dot_world'] for s in sides.values()], [s["normal_ue"] for s in sides.values()], yaw)


def render(path, head, height, dots, normals, yaw=0.):
    scene = bpy.context.scene
    mat = bpy.data.materials.new('S47EyeDot')
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = (1, 0, 0, 1)
    for d in dots:
        bpy.ops.mesh.primitive_uv_sphere_add(radius=.0025 * max(1, height / 1.2), location=d, segments=12, ring_count=8)
        bpy.context.object.data.materials.append(mat)
    cam_data = bpy.data.cameras.new('S47EyeCam')
    cam_data.lens = 85
    cam = bpy.data.objects.new('S47EyeCam', cam_data)
    scene.collection.objects.link(cam)
    target = Vector(dots[0]) * .5 + Vector(dots[-1]) * .5 if dots else Vector(head)
    away = .36 + .12 * height
    cam.location = target + Vector((np.sin(yaw) * away, -np.cos(yaw) * away, 0))
    cam.rotation_euler = (Vector(target) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    scene.camera = cam
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.light = 'FLAT'
    scene.display.shading.color_type = 'TEXTURE'
    scene.render.resolution_x = scene.render.resolution_y = 640
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print('OUT', path)


main()
