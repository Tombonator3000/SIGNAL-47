"""Fitted lookdev derivative of Tom's existing Jeep, root-run Blender CLI only.

Original GLB/Blends/textures remain untouched. Seven native parts, measured axes,
working cabin and lamp vertex masks remain. New material tiles are Blender-baked
from our procedural shaders, never pixel edits of the source image atlas.
"""
from pathlib import Path
import argparse
import copy
import json
import math
import shutil
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import prepare as P

PRIVATE = Path.home() / 'Documents/ChatGPT/Signal 47/modeller/tripo-fleet-20261031'
SOURCE = PRIVATE / 'interior-window-service-20261101/classic_jeep'
OUTPUT = Path.home() / 'scratch-bm/Signal47-jeep-lookdev-20261102/candidate'
BODY = 'SM_TripoJeep_InteriorBody'
PARTS = [BODY] + ['SM_TripoJeep_' + s for s in ('Interior', 'Steering', 'SpeedFace', 'FuelFace', 'TempFace', 'RadioFace')]
FACE_PARTS = set(PARTS[3:])


def read_json(path):
    return json.loads(path.read_text())


def fit_wheels(obj):
    """Fit real sidewall circles, not guessed nominal vehicle axle coordinates."""
    import numpy as np
    points = P.xyz([obj]); result = []
    for side in (-1, 1):
        for end in (-1, 1):
            cloud = points[(side * points[:, 0] > .60) & (end * points[:, 1] > .60) & (points[:, 2] < .79)]
            P.require(len(cloud) > 300, 'Inspected wheel region missing')
            # The most outboard sidewall points exclude axles and floor rails.
            outer = cloud[side * cloud[:, 0] > np.quantile(side * cloud[:, 0], .80)]
            contact=cloud[cloud[:,2]<.12]
            P.require(len(contact)>10,'Actual low tire contact patch missing')
            contact_y=float(np.median(contact[:,1]))
            yz = outer[:, 1:]; rng = np.random.default_rng(4702 + side * 11 + end)
            best = None
            for _ in range(192):
                a = yz[rng.choice(len(yz), 3, replace=False)]
                matrix = np.column_stack((2 * a[:, 0], 2 * a[:, 1], np.ones(3)))
                try: cy, cz, k = np.linalg.solve(matrix, np.sum(a * a, axis=1))
                except np.linalg.LinAlgError: continue
                rad2 = k + cy * cy + cz * cz
                if not (.39 < cz < .50 and abs(cy-contact_y)<.055 and .32**2 < rad2 < .41**2): continue
                radius = math.sqrt(rad2); residual = np.abs(np.linalg.norm(yz - (cy, cz), axis=1) - radius)
                hits = residual < .009
                score = int(hits.sum())
                if best is None or score > best[0]: best = (score, hits, cy, cz, radius)
            P.require(best is not None and best[0] >= 28, 'No stable source sidewall circle')
            yz = yz[best[1]]
            matrix = np.column_stack((2 * yz[:, 0], 2 * yz[:, 1], np.ones(len(yz))))
            cy, cz, k = np.linalg.lstsq(matrix, np.sum(yz * yz, axis=1), rcond=None)[0]
            radius = math.sqrt(k + cy * cy + cz * cz)
            residual = float(np.median(np.abs(np.linalg.norm(yz - (cy, cz), axis=1) - radius)))
            P.require(residual < .012 and .30 < cz < .55, 'Uncertain source wheel fit')
            # Real outer sidewall surface, with a recessed steel rim face.
            x = side * (float(np.quantile(side * outer[:, 0], .92)) - .020)
            result.append(dict(side=side, end=end, center=[x, float(cy), float(cz)],
                               sidewall_fit_radius_m=radius, residual_m=residual, inliers=len(yz), contact_patch_y_m=contact_y,
                               rim_radius_m=.225, tire_radius_m=float(cz)))
    return result


def sample_source(obj):
    """Read a small in-memory copy solely to identify existing material regions."""
    import bpy
    import numpy as np
    mat = obj.data.materials[0]
    shader = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    source = shader.inputs['Base Color'].links[0].from_node.image
    thumbnail = source.copy(); thumbnail.scale(512, 512)
    pixels = np.empty(512 * 512 * 4, dtype=np.float32); thumbnail.pixels.foreach_get(pixels)
    pixels = pixels.reshape((512, 512, 4)); uv = obj.data.uv_layers.active.data
    colors = []
    for face in obj.data.polygons:
        u, v = (sum(uv[i].uv[j] for i in face.loop_indices) / len(face.loop_indices) for j in range(2))
        colors.append(pixels[min(511, max(0, int(v * 512))), min(511, max(0, int(u * 512))), :3].copy())
    bpy.data.images.remove(thumbnail)
    return np.asarray(colors)


def fit_seats(obj, colors):
    import numpy as np
    centers = np.asarray([tuple(f.center) for f in obj.data.polygons])
    normals = np.asarray([tuple(f.normal) for f in obj.data.polygons])
    brown = (colors[:, 0] > colors[:, 1] * 1.35) & (colors[:, 1] > colors[:, 2] * 1.18) & (colors[:, 0] > .035)
    seats = []
    for side in (-1, 1):
        region = brown & (side * centers[:, 0] > .10) & (side * centers[:, 0] < .72) & (centers[:, 1] > -1.10) & (centers[:, 1] < .12) & (centers[:, 2] > .70) & (centers[:, 2] < 1.68)
        pts = centers[region]
        P.require(len(pts) > 20, 'Original front seat material region missing')
        low, high = np.quantile(pts, .015, axis=0), np.quantile(pts, .985, axis=0)
        horizontal = centers[region & (normals[:, 2] > .45) & (centers[:, 2] < 1.10)]
        P.require(len(horizontal) > 5, 'Cannot locate actual cushion surface')
        cx = float((low[0] + high[0]) / 2)
        cushion_top = float(np.quantile(horizontal[:, 2], .65))
        front = float(np.quantile(horizontal[:, 1], .95)); back = float(np.quantile(pts[:, 1], .05))
        width = float(min(.59, max(.47, high[0] - low[0])))
        depth = float(min(.53, max(.42, front - back)))
        y = front - depth / 2
        high_region = brown & (side * centers[:, 0] > .10) & (side * centers[:, 0] < .72) & (centers[:, 1] > -1.18) & (centers[:, 1] < .12) & (centers[:, 2] > 1.65) & (centers[:, 2] < 1.98)
        high_points = centers[high_region]
        P.require(len(high_points) > 10, 'Original front headrest region missing')
        head_lo, head_hi = high_points.min(axis=0), high_points.max(axis=0)
        # The original high headrest was excluded by the cushion fit's 1.68 m
        # ceiling. Retire its shell independently; no change to the new seat fit.
        # Cab-back rays find the dark rear shell up to 7 cm behind the brown
        # front seed. Include its measured thickness, without widening toward
        # the roll bar or changing the fitted cushion/seat-back location.
        legacy_back = [[max(float(low[0]-.02), cx-width*.49), float(min(low[1],head_lo[1])-.09), cushion_top-.10],
                       [min(float(high[0]+.02), cx+width*.49), float(back+.19), float(head_hi[2]+.04)]]
        seats.append(dict(kind='front', side=side, center=[cx, y, cushion_top - .04],
                          legacy_back_bounds=legacy_back,
                          width=width, depth=depth, back_y=back + .025,
                          back_top=float(min(1.62, max(1.40, high[2]))),
                          source_bounds=[(low - (.02, .03, .025)).tolist(), (high + (.02, .03, .025)).tolist()]))
    region = brown & (np.abs(centers[:, 0] - .05) < .72) & (centers[:, 1] < -1.10) & (centers[:, 1] > -1.95) & (centers[:, 2] > .73)
    pts = centers[region]
    if len(pts) > 20:
        low, high = np.quantile(pts, .015, axis=0), np.quantile(pts, .985, axis=0)
        # Brown pixels identify the rear backrest; the cushion itself is black
        # in this source. Locate its real horizontal surface independently.
        surface=(np.abs(centers[:,0]-.05)<.57)&(centers[:,1]>-1.60)&(centers[:,1]<-1.06)&(centers[:,2]>.85)&(centers[:,2]<1.18)&(normals[:,2]>.5)
        horizontal=centers[surface]
        P.require(len(horizontal)>8,'Actual rear cushion support surface missing')
        top=float(np.quantile(horizontal[:,2],.70))
        back_y=float(low[1]+.035)
        seats.append(dict(kind='rear', center=[float((low[0] + high[0]) / 2), back_y+.23, top - .04],
                          width=float(min(1.20, max(.95, high[0] - low[0]))), depth=.44,
                          back_y=float(low[1] + .035), back_top=float(min(1.48, max(1.22, high[2]))),
                          source_bounds=[[float(low[0]-.04),back_y-.075,top-.11],[float(high[0]+.04),back_y+.49,float(high[2]+.045)]]))
    return seats



def retire_source_box(bm, source_layer, lo, hi):
    """Subtract a measured cabin volume from source faces, preserving its outside.

    Bisect interpolates the existing UV/colour corner layers. Centroid-only
    deletion left long triangle tips; whole-face deletion would cut the tub.
    No authored additions are in this BMesh, and lamp faces have source_layer=0.
    """
    import bmesh
    from mathutils import Vector
    eps = 1e-6
    def touches(face):
        return face[source_layer] and all(
            max(v.co[k] for v in face.verts) >= lo[k] - eps and
            min(v.co[k] for v in face.verts) <= hi[k] + eps for k in range(3))
    # A selected face can share an edge with an unselected lamp face. Isolate
    # the edit region before bisecting so protected neighbours keep their exact
    # vertices/corners, rather than acquiring interpolated extra lamp corners.
    initial = [f for f in bm.faces if touches(f)]
    if not initial: return 0
    bmesh.ops.split(bm, geom=initial, use_only_faces=True)
    for axis in range(3):
        normal = Vector((0, 0, 0)); normal[axis] = 1
        for boundary in (lo[axis], hi[axis]):
            faces = [f for f in bm.faces if touches(f)]
            if not faces: return 0
            geom = set(faces)
            for face in faces: geom.update(face.edges); geom.update(face.verts)
            point = Vector((0, 0, 0)); point[axis] = boundary
            bmesh.ops.bisect_plane(bm, geom=list(geom), dist=eps,
                                  plane_co=point, plane_no=normal,
                                  clear_inner=False, clear_outer=False)
    inside = [f for f in bm.faces if f[source_layer] and
              all(lo[k]-eps <= f.calc_center_median()[k] <= hi[k]+eps for k in range(3))]
    bmesh.ops.delete(bm, geom=inside, context='FACES')
    return len(inside)


def triangulate_derivative(obj):
    """Triangulate only the generated mesh, copying each original loop corner.

    Bisected polygons can have more than four vertices. Explicit loop triangles
    retain the evaluated custom normals/UV/lamp mask at every new corner and
    avoid FBX's unsupported-ngon tangent path. Original source meshes are never
    modified; even the derivative's vertex coordinates/order remain unchanged.
    """
    import bpy
    mesh = obj.data
    if all(len(p.vertices) == 3 for p in mesh.polygons): return
    mesh.calc_loop_triangles()
    triangles = list(mesh.loop_triangles)
    corners = [i for tri in triangles for i in tri.loops]
    normals = [tuple(mesh.corner_normals[i].vector) for i in corners]
    result = bpy.data.meshes.new(mesh.name + ' triangles')
    result.from_pydata([tuple(v.co) for v in mesh.vertices], [], [tuple(t.vertices) for t in triangles])
    for mat in mesh.materials: result.materials.append(mat)
    for face, tri in zip(result.polygons, triangles):
        face.material_index = mesh.polygons[tri.polygon_index].material_index
        # Explicit copied corner normals remain authoritative, including flat
        # clipped source faces whose triangulated geometric normals can differ.
        face.use_smooth = True
    for layer in mesh.uv_layers:
        new = result.uv_layers.new(name=layer.name)
        new.data.foreach_set('uv', [v for i in corners for v in layer.data[i].uv])
        new.active_render = layer.active_render
    if mesh.uv_layers: result.uv_layers.active_index = mesh.uv_layers.active_index
    for layer in mesh.color_attributes:
        P.require(layer.domain in ('CORNER', 'POINT'), 'Unexpected mesh colour domain')
        new = result.color_attributes.new(name=layer.name, type=layer.data_type, domain=layer.domain)
        indices = corners if layer.domain == 'CORNER' else range(len(mesh.vertices))
        new.data.foreach_set('color', [v for i in indices for v in layer.data[i].color])
    if mesh.color_attributes:
        result.color_attributes.active_color_index = mesh.color_attributes.active_color_index
        result.color_attributes.render_color_index = mesh.color_attributes.render_color_index
    # A custom normal belongs to its original smooth fan. from_pydata loses
    # the split edges and flat-polygon boundaries; without both, Blender blends
    # nearby supplied normals across those boundaries (measured 0.0158 error).
    # New triangulation diagonals stay smooth within the original polygon.
    sharp_edges = {tuple(sorted(e.vertices)) for e in mesh.edges if e.use_edge_sharp}
    sharp_edges.update(tuple(sorted(edge)) for face in mesh.polygons if not face.use_smooth for edge in face.edge_keys)
    for edge in result.edges: edge.use_edge_sharp = tuple(sorted(edge.vertices)) in sharp_edges
    result.update()
    result.normals_split_custom_set(normals)
    normal_error = max((math.sqrt(sum((a-b)**2 for a,b in zip(n.vector,wanted))) for n,wanted in zip(result.corner_normals,normals)), default=0.)
    P.require(normal_error < .001, 'Derivative triangulation changed corner normals: '+str(normal_error))
    P.require(len(result.vertices)==len(mesh.vertices) and all(len(p.vertices)==3 for p in result.polygons), 'Derivative triangulation changed vertices or left ngons')
    obj.data = result
    if mesh.users == 0: bpy.data.meshes.remove(mesh)

def smooth_paint_normals(obj, paint_slot, iterations=30, first_excluded=None, keep=None):
    """Shade lumpy scan panels smoothly without moving one render vertex.

    A private proxy copy is welded and Laplacian-smoothed; only its corner
    normals are transferred back onto painted faces (vertex-group masked).
    Split source seams receive identical normals, so no crack can open.
    """
    import bpy
    import bmesh
    limit=len(obj.data.polygons) if first_excluded is None else first_excluded
    chosen={p.index for p in obj.data.polygons[:limit] if p.material_index==paint_slot and (keep is None or keep(p.center))}
    paint_verts={v for i in chosen for v in obj.data.polygons[i].vertices}
    if not paint_verts:return []
    group=obj.vertex_groups.new(name='S47PaintNormals');group.add(sorted(paint_verts),1.,'REPLACE')
    proxy=obj.copy();proxy.data=obj.data.copy();proxy.name='S47PaintNormalProxy'
    bpy.context.scene.collection.objects.link(proxy)
    bm=bmesh.new();bm.from_mesh(proxy.data)
    bm.faces.ensure_lookup_table()
    bmesh.ops.delete(bm,geom=[f for f in bm.faces if f.index not in chosen],context='FACES')
    bmesh.ops.remove_doubles(bm,verts=bm.verts,dist=.0025)
    for _ in range(iterations):
        bmesh.ops.smooth_vert(bm,verts=bm.verts,factor=.5,use_axis_x=True,use_axis_y=True,use_axis_z=True)
    bm.to_mesh(proxy.data);bm.free()
    if proxy.data.has_custom_normals:
        P.select([proxy]);bpy.ops.mesh.customdata_custom_splitnormals_clear()
    proxy.data.shade_smooth()
    for i in chosen:obj.data.polygons[i].use_smooth=True
    mod=obj.modifiers.new('S47 paint normals','DATA_TRANSFER');mod.object=proxy
    mod.use_loop_data=True;mod.data_types_loops={'CUSTOM_NORMAL'};mod.loop_mapping='POLYINTERP_NEAREST'
    mod.vertex_group=group.name;mod.mix_mode='REPLACE';mod.mix_factor=.9
    before=[tuple(v.co) for v in obj.data.vertices]
    P.select([obj]);bpy.ops.object.modifier_apply(modifier=mod.name)
    P.require(len(before)==len(obj.data.vertices) and all(tuple(v.co)==b for v,b in zip(obj.data.vertices,before)),'Normal transfer moved a vertex')
    left=obj.vertex_groups.get('S47PaintNormals')
    if left:obj.vertex_groups.remove(left)
    bpy.data.objects.remove(proxy,do_unlink=True)
    return sorted(paint_verts)


MIRROR_EYE = (-.41, -.65, 1.70)  # driver eye, Blender metres (anchors driver_eye_three_m)


def add_mirrors(materials, anchors):
    """Live-mirror glass: two CJ side mirrors and one interior rear-view mirror.

    Codex's rigid side-mirror heads are kept, but swung about their upright
    bracket so the glass faces the bisector of driver eye and straight back.
    Each glass is one flat quad with its own slot and full 0..1 UV; runtime
    replaces the slot with a render target. Anchors are in native UEcm.
    """
    import bpy
    import bmesh
    from mathutils import Vector, Matrix
    eye = Vector(MIRROR_EYE); back = Vector((0, -1, 0)); up_z = Vector((0, 0, 1))
    mats = {}
    for key in ('L', 'R', 'Rear'):
        name = 'MI_JeepMirrorGlass_' + key
        m = bpy.data.materials.get(name) or bpy.data.materials.new(name); m.use_nodes = True
        sh = m.node_tree.nodes.get('Principled BSDF')
        sh.inputs['Base Color'].default_value = (.58, .61, .62, 1); sh.inputs['Metallic'].default_value = .95; sh.inputs['Roughness'].default_value = .05
        mats[key] = m
        materials.append(dict(name=name, base_color_linear=[.58, .61, .62, 1], roughness=.05, metallic=.95, textures={},
                              normal_format='OpenGL', runtime_render_target_mirror=key))

    def frame(n):
        d = -n; r = d.cross(up_z).normalized(); u = r.cross(d).normalized(); return r, u

    def ue(v, point=True):
        k = 100. if point else 1.
        return [round(v.x * k, 4), round(-v.y * k, 4), round(v.z * k, 4)]

    specs = {}
    for key, side in (('L', -1), ('R', 1)):
        pivot = Vector((side * .89, .17, 1.665)); center0 = Vector((side * .89, .1205, 1.665))
        n = ((eye - center0).normalized() + back); n.z = 0; n.normalize()
        theta = math.atan2(n.x, -n.y)
        specs[key] = dict(side=side, pivot=pivot, theta=theta, rot=Matrix.Rotation(theta, 4, 'Z'),
                          center=pivot + Matrix.Rotation(theta, 3, 'Z') @ (center0 - pivot), normal=n, size=(.146, .205))
    rc = Vector((0., -.175, 1.915))
    rn = ((eye - rc).normalized() + back).normalized()
    specs['Rear'] = dict(center=rc, normal=rn, size=(.161, .0435))  # 25 % below the first .215 x .058
    report = {}
    for lod in range(3):
        obj = bpy.data.objects['SM_TripoJeep_Interior' + ('' if lod == 0 else '_LOD' + str(lod))]
        names = [m.name for m in obj.data.materials]
        silver = names.index('M_JeepInterior_MirrorSilver'); plastic = names.index('M_JeepInterior_Plastic')
        slot = {}
        for key, m in mats.items(): obj.data.materials.append(m); slot[key] = len(obj.data.materials) - 1
        bm = bmesh.new(); bm.from_mesh(obj.data); bm.faces.ensure_lookup_table()
        old = [f for f in bm.faces if f.material_index == silver and abs(f.calc_center_median().x) > .80
               and .10 < f.calc_center_median().y < .14 and 1.54 < f.calc_center_median().z < 1.79]
        P.require(len(old) >= 2, 'Codex side-mirror face not found in Interior LOD' + str(lod))
        bmesh.ops.delete(bm, geom=old, context='FACES')
        swung = 0
        for v in bm.verts:
            for spec in (specs['L'], specs['R']):
                sx = spec['side'] * v.co.x
                if .78 < sx < 1.0 and .105 < v.co.y < .185 and 1.53 < v.co.z < 1.80:
                    v.co = spec['pivot'] + spec['rot'].to_3x3() @ (v.co - spec['pivot']); swung += 1
        uv_layers = list(bm.loops.layers.uv.values())

        def quad(corners, mat_index, uvs=((0, 0), (1, 0), (1, 1), (0, 1))):
            vs = [bm.verts.new(c) for c in corners]; f = bm.faces.new(vs); f.material_index = mat_index; f.smooth = False
            for loop, uv in zip(f.loops, uvs):
                for layer in uv_layers: loop[layer].uv = uv
            return f

        def oriented_box(center, r, u, n, w, h, d, mat_index):
            pts = {}
            for i in (-1, 1):
                for j in (-1, 1):
                    for k in (-1, 1): pts[i, j, k] = center + r * (i * w / 2) + u * (j * h / 2) + n * (k * d / 2)
            for a, b, c, e in [((-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)), ((1,-1,-1),(-1,-1,-1),(-1,1,-1),(1,1,-1)),
                               ((-1,-1,-1),(1,-1,-1),(1,-1,1),(-1,-1,1)), ((-1,1,1),(1,1,1),(1,1,-1),(-1,1,-1)),
                               ((1,-1,1),(1,-1,-1),(1,1,-1),(1,1,1)), ((-1,-1,-1),(-1,-1,1),(-1,1,1),(-1,1,-1))]:
                quad([pts[a], pts[b], pts[c], pts[e]], mat_index)

        for key, spec in specs.items():
            n = spec['normal']; r, u = frame(n); w, h = spec['size']; c = spec['center']
            if key == 'Rear':
                # Small black rectangular housing on a short arm from the top rail.
                oriented_box(c - n * .01175, r, u, n, .184, .060, .0225, plastic)
                top = Vector((0., -.112, 1.995)); foot = c - n * .03 + u * .02
                arm_n = (foot - top).normalized(); arm_r = arm_n.cross(Vector((1, 0, 0))).normalized(); arm_u = arm_r.cross(arm_n)
                # Chrome arm/mount keep the existing MirrorSilver slot in use, so
                # native reimport retains slot names and appends the glass slots.
                oriented_box((top + foot) / 2, Vector((1, 0, 0)), arm_n, arm_r, .014, (foot - top).length, .012, silver)
                oriented_box(top + Vector((0, 0, .004)), Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1)), .05, .035, .012, silver)
            gc = c + n * .0012
            quad([gc - r * (w / 2) - u * (h / 2), gc + r * (w / 2) - u * (h / 2), gc + r * (w / 2) + u * (h / 2), gc - r * (w / 2) + u * (h / 2)], slot[key])
            if lod == 0:
                report[key] = dict(part='SM_TripoJeep_Interior', material_slot='MI_JeepMirrorGlass_' + key, slot_index_lod0=slot[key],
                                   center_uecm=ue(gc), normal_uecm=ue(n, False), up_uecm=ue(u, False), u_axis_uecm=ue(r, False),
                                   size_cm=[round(w * 100, 2), round(h * 100, 2)],
                                   yaw_from_straight_back_deg=round(math.degrees(spec.get('theta', math.atan2(n.x, -n.y))), 3))
        bm.to_mesh(obj.data); bm.free(); obj.data.update()
        P.require(swung > 20, 'Side-mirror heads not found for swing in LOD' + str(lod))
    anchors['mirrors'] = dict(
        frame='native UEcm in the shared vehicle frame of SM_TripoJeep_Interior/InteriorBody: UEcm=(Bx,-By,Bz)*100; vectors use the same sign map without scale',
        uv='Unreal UV (0,0) = top-left as seen by a viewer facing the glass; U along u_axis_uecm, V down (opposite up_uecm). Blender UV (0,0) bottom-left before FBX V flip.',
        normal='glass front normal: away from the glass toward the driver/rear side', driver_eye_uecm=ue(eye),
        glass=report)
    return report


def run(source, output, inspect_only=False, preview=False, export=True, style='hybrid', smooth_normals=False):
    import bpy
    import bmesh
    import numpy as np
    from mathutils import Vector
    source, output = source.resolve(), output.resolve()
    P.require(source.is_dir() and not output.is_relative_to(P.REPO) and not output.is_relative_to(source) and not source.is_relative_to(output), 'Separate private source/output required')
    contract = read_json(source / 'model_contract.json'); anchors = read_json(source / 'anchors.json')
    protected = {str(p): P.sha(p) for p in (source / 'jeep-interior.blend', source / 'model_contract.json', source / 'materials.json', source / 'anchors.json', Path(contract['source_glb']))}
    bpy.ops.wm.open_mainfile(filepath=str(source / 'jeep-interior.blend'))
    body0 = bpy.data.objects[BODY]; body0.data.update()
    colors = sample_source(body0); wheels = fit_wheels(body0); seats = fit_seats(body0, colors)
    layout = dict(wheels=wheels, seats=seats)
    output.mkdir(parents=True, exist_ok=True)
    P.write_json(output / 'fitted-layout.json', layout)
    print('S47_JEEP_FITTED_LAYOUT', json.dumps(layout), flush=True)
    if inspect_only: return
    model = output / 'classic_jeep'; (model / 'exports').mkdir(parents=True, exist_ok=True)
    (model / 'textures').mkdir(exist_ok=True)
    materials = copy.deepcopy(read_json(source / 'materials.json'))
    for row in materials:
        for file in list(row.get('textures', {}).values()) + [row.get('packed_metallic_roughness'), row.get('occlusion')]:
            if not file: continue
            src, dst = (source / file).resolve(), (model / file).resolve()
            P.require(src.is_relative_to(source) and dst.is_relative_to(model), 'Texture path escapes model')
            dst.parent.mkdir(parents=True, exist_ok=True); shutil.copyfile(src, dst)
    scene = bpy.context.scene
    for obj in list(scene.objects):
        if obj.type in ('CAMERA', 'LIGHT'): bpy.data.objects.remove(obj, do_unlink=True)
    scene.render.engine = 'CYCLES'; scene.cycles.device = 'CPU'; scene.cycles.samples = 1
    scene.render.threads_mode = 'FIXED'; scene.render.threads = 2
    mats = {}

    def material(key, color, rough, metal=0., grain=.025, bump_strength=.16, two_sided=False):
        name = 'M_JeepInterior_CJ' + key
        mat = bpy.data.materials.get(name) or bpy.data.materials.new(name); mat.use_nodes = True
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        nodes.clear(); out = nodes.new('ShaderNodeOutputMaterial'); shader = nodes.new('ShaderNodeBsdfPrincipled')
        links.new(shader.outputs[0], out.inputs['Surface'])
        uv = nodes.new('ShaderNodeTexCoord'); noise = nodes.new('ShaderNodeTexNoise')
        noise.inputs['Scale'].default_value = 90.; noise.inputs['Detail'].default_value = 2.
        links.new(uv.outputs['UV'], noise.inputs['Vector'])
        mix = nodes.new('ShaderNodeMixRGB'); mix.blend_type = 'MULTIPLY'; mix.inputs[0].default_value = grain
        mix.inputs[1].default_value = (*color, 1); links.new(noise.outputs['Color'], mix.inputs[2])
        links.new(mix.outputs[0], shader.inputs['Base Color'])
        shader.inputs['Roughness'].default_value = rough; shader.inputs['Metallic'].default_value = metal
        bump = nodes.new('ShaderNodeBump'); bump.inputs['Strength'].default_value = bump_strength
        bump.inputs['Distance'].default_value = .00025; links.new(noise.outputs['Fac'], bump.inputs['Height']); links.new(bump.outputs[0], shader.inputs['Normal'])
        # New tiny tiling maps are rendered from these authored shaders in Blender.
        # The original Tripo image data, UV atlas and normal texture are not edited.
        bpy.ops.mesh.primitive_plane_add(size=2, location=(0, 0, -20)); plane = bpy.context.object
        plane.name = 'CJ material bake only'; plane.data.materials.append(mat)
        row = dict(name=name, base_color_linear=[1, 1, 1, 1], roughness=1., metallic=metal,
                   normal_strength=1., normal_format='OpenGL', textures={}, two_sided=two_sided)
        for role in ('base_color', 'roughness', 'normal'):
            image = bpy.data.images.new(name + '_' + role, 256, 256, alpha=False, is_data=role != 'base_color')
            image.colorspace_settings.name = 'sRGB' if role == 'base_color' else 'Non-Color'
            target = nodes.new('ShaderNodeTexImage'); target.image = image; nodes.active = target
            P.select([plane]); scene.render.bake.use_clear = True; scene.render.bake.margin = 2; scene.render.bake.use_selected_to_active = False
            emission = None
            if role != 'normal':
                emission = nodes.new('ShaderNodeEmission')
                if role == 'base_color': links.new(mix.outputs[0], emission.inputs['Color'])
                else: emission.inputs['Color'].default_value = (rough, rough, rough, 1)
                links.new(emission.outputs[0], out.inputs['Surface'])
            bpy.ops.object.bake(type='NORMAL' if role == 'normal' else 'EMIT')
            if emission: nodes.remove(emission)
            links.new(shader.outputs[0], out.inputs['Surface'])
            filename = 'textures/' + name + '_' + role + '.png'
            image.filepath_raw = str(model / filename); image.file_format = 'PNG'; image.save(); image.pack()
            row['textures'][role] = filename; nodes.remove(target)
        bpy.data.objects.remove(plane, do_unlink=True)
        mats[key] = mat; materials.append(row)
        return mat

    # Clean factory white enamel (1986 CJ-7), black textured trim/flares,
    # bright styled-steel rims and tan vinyl. Seats are two-sided so any
    # culled source remnant behind them can never show through in Unreal.
    # style 'source' (r15 comparison): the original Tripo atlas keeps paint,
    # Renegade decals, stripes, flares, seats and wheels; only the cabin
    # closure (floor pan, toe board, twin faces) is new. style 'cj' is the
    # earlier repaint with new seats and rims, kept for comparison.
    # style 'hybrid' (default): source exterior plus clean fitted seats in
    # the original orange-brown (nutmeg) vinyl.
    cj = style == 'cj'; new_seats = style in ('cj', 'hybrid')
    if cj: material('IvoryPaint', (.70, .68, .61), .55, .0, .010, .04)
    material('TireRubber', (.014, .017, .016), .86, 0., .065)
    material('Chassis', (.022, .024, .024), .70, .0, .035)
    if cj: material('WheelSteel', (.56, .57, .55), .30, .90, .012, .05)
    if cj:
        material('SeatVinyl', (.115, .080, .045), .62, 0., .050, .16, True)
        material('SeatSeam', (.045, .032, .018), .80, 0., .02, .16, True)
    elif new_seats:
        material('SeatVinyl', (.30, .085, .025), .42, 0., .040, .14, True)
        material('SeatSeam', (.11, .032, .010), .70, 0., .02, .16, True)
    # Procedural Blender preview uses the same tiled UV convention as native maps.
    created = []

    def mesh_object(name, verts, faces, key):
        data = bpy.data.meshes.new(name); data.from_pydata(verts, [], faces); data.materials.append(mats[key])
        uv = data.uv_layers.new(name='UVMap')
        for poly in data.polygons:
            poly.use_smooth = True
            for li in poly.loop_indices:
                v = data.vertices[data.loops[li].vertex_index].co
                uv.data[li].uv = (v.x * 2, (v.y + v.z) * 2)
        obj = bpy.data.objects.new(name, data); scene.collection.objects.link(obj)
        # Explicit black RG on every new corner prevents whole seats/rims lighting.
        color = data.color_attributes.new(name='S47VehicleLamps', type='FLOAT_COLOR', domain='CORNER')
        for item in color.data: item.color = (0, 0, 0, 1)
        created.append(obj); return obj

    def rod(name, a, b, radius, key='Chassis', segments=12):
        a, b = Vector(a), Vector(b)
        bpy.ops.mesh.primitive_cylinder_add(vertices=segments, radius=radius, depth=(b-a).length, location=(a+b)/2)
        obj = bpy.context.object; obj.name = name; obj.rotation_mode = 'QUATERNION'; obj.rotation_quaternion = (b-a).to_track_quat('Z', 'Y')
        obj.data.materials.append(mats[key]); colors_ = obj.data.color_attributes.new(name='S47VehicleLamps', type='FLOAT_COLOR', domain='CORNER')
        for item in colors_.data: item.color = (0, 0, 0, 1)
        created.append(obj); return obj

    def pillow(name, center, width, depth, height, back=False):
        # Four rounded superellipse rings form real piping, raised side bolsters
        # and a shallow sitting depression, rather than a bevelled cube overlay.
        verts = []; faces = []; n = 48
        rings = [(.20, .020), (.55, .014), (.83, .032), (1., .018), (1., -.030), (.86, -height)]
        def place(x, y, z):
            if back: return (center[0]+x, center[1]+z-y*.16, center[2]+y)
            return (center[0]+x, center[1]+y, center[2]+z + .045*(abs(x)/(width/2))**4)
        for scale, z in rings:
            for i in range(n):
                a = 2*math.pi*i/n; c, s = math.cos(a), math.sin(a)
                x = math.copysign(abs(c)**.5, c)*width/2*scale; y = math.copysign(abs(s)**.5, s)*depth/2*scale
                verts.append(place(x, y, z))
        verts.append(place(0, 0, .012)); top = len(verts)-1
        verts.append(place(0, 0, -height)); bottom = len(verts)-1
        for i in range(n):
            j = (i+1)%n; faces.append((top, i, j))
            for r in range(len(rings)-1): faces.append((r*n+i,(r+1)*n+i,(r+1)*n+j,r*n+j))
            faces.append((bottom,(len(rings)-1)*n+j,(len(rings)-1)*n+i))
        if back: faces=[tuple(reversed(face))for face in faces]
        obj = mesh_object(name, verts, faces, 'SeatVinyl')
        # Follow the actual cushion surface. Straight ribs at a fixed height
        # crossed the raised/depressed rings and appeared as pointed black shards.
        from mathutils.bvhtree import BVHTree
        surface = BVHTree.FromPolygons(verts, faces)
        outward = Vector((0, 1, 0) if back else (0, 0, 1))
        for u in (-.66, -.33, 0, .33, .66):
            x = width/2*u
            points = []
            for step in range(21):
                y = depth * (-.35 + .70 * step / 20)
                hit, normal, _, _ = surface.ray_cast(Vector(place(x, y, .20)), -outward, .40)
                P.require(hit is not None, 'Seat stitch missed its authored cushion')
                points.append(hit + outward * .0009)
            for a, b in zip(points, points[1:]):
                rod(name+' pressed stitch', a, b, .0006, 'SeatSeam', 6)
        return obj

    def box(name, lo, hi, key):
        (x0,y0,z0),(x1,y1,z1)=lo,hi
        verts=[(x0,y0,z0),(x1,y0,z0),(x1,y1,z0),(x0,y1,z0),(x0,y0,z1),(x1,y0,z1),(x1,y1,z1),(x0,y1,z1)]
        faces=[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]
        obj = mesh_object(name,verts,faces,key)
        # Folded sheet metal and rubber mats have planar faces, not interpolated
        # corner normals that make the broad floor read as a sagging bowl.
        for poly in obj.data.polygons: poly.use_smooth = False
        return obj

    # The fused Tripo seats had no floor beneath them; the smaller real
    # seats opened a view down to the frame. A measured, closed floor pan
    # with ribbed rubber mat and seat risers closes the cabin.
    centers0=np.asarray([tuple(f.center) for f in body0.data.polygons]);normals0=np.asarray([tuple(f.normal) for f in body0.data.polygons])
    # The large flat rear cargo floor is the one unambiguous floor surface;
    # the authored pedals and gear boots (interior.py) stand on the same level.
    # (Its Tripo normals point down, like much of the cabin shell.)
    pan=(np.abs(normals0[:,2])>.7)&(np.abs(centers0[:,0]-.05)<.45)&(centers0[:,1]>-1.7)&(centers0[:,1]<-1.0)&(centers0[:,2]>.55)&(centers0[:,2]<.70)
    P.require(pan.sum()>20,'Rear cargo floor not found')
    areas0=np.asarray([f.area for f in body0.data.polygons])[pan];zs=centers0[pan,2];order=np.argsort(zs)
    # Area-weighted median: the broad cargo floor, not small chassis bits.
    floor_z=float(zs[order][np.searchsorted(np.cumsum(areas0[order]),areas0.sum()/2)])+.006
    print('S47_JEEP_FLOOR_Z',floor_z,flush=True)
    P.require(.57<floor_z<.68,'Unexpected measured cabin floor height')
    layout['floor_z_m']=floor_z
    box('CJ front floor pan',(.05-.68,-.98,floor_z-.03),(.05+.68,.30,floor_z+.006),'TireRubber')
    box('CJ rear floor pan',(.05-.50,-1.80,floor_z-.03),(.05+.50,-.96,floor_z+.006),'TireRubber')
    # Sloped toe board behind the pedals closes the source gap to the firewall.
    toe=[(.05-.68,.28,floor_z+.004),(.05+.68,.28,floor_z+.004),(.05+.68,.42,1.06),(.05-.68,.42,1.06)]
    mesh_object('CJ toe board',toe+[(x,y+.008,z-.004) for x,y,z in toe],[(0,1,2,3),(7,6,5,4)],'Chassis')
    for i in range(9):
        y=-.90+i*.105
        for side in (-1,1):
            x0=.05+side*.14 if side>0 else .05-.60
            box('CJ mat rib',(x0,y,floor_z+.006),(x0+.46,y+.035,floor_z+.012),'TireRubber')

    # New padding follows measured original seat positions and leaves cabin eye,
    # pedals, wheel, doorway and luggage space at their original anchors.
    for i, seat in enumerate(seats if new_seats else []):
        cx, cy, cz = seat['center']; width, depth = seat['width'], seat['depth']
        if cz-.065-floor_z>.02:
            box('CJ seat riser',(cx-width*.40,cy-depth*.40,floor_z),(cx+width*.40,cy+depth*.40,cz-.05),'Chassis')
        pillow('CJ fitted cushion '+str(i),(cx,cy,cz),width,depth,.065)
        bottom = cz + .10; top = seat['back_top']; back_height = max(.34, top-bottom)
        pillow('CJ fitted backrest '+str(i),(cx,seat['back_y'],(top+bottom)/2),width*.96,back_height,.045,True)
        for side in (-1,1):
            rod('CJ seat support',(cx+side*width*.32,cy-.15,cz-.08),(cx+side*width*.32,cy+.14,cz-.08),.012,'Chassis')

    def steel_wheel(wheel):
        side = wheel['side']; x, cy, cz = wheel['center']; n = 80
        # Pressed five-lug steel rim, with real open ventilation sectors.
        profile = [(.048,-.020),(.078,-.015),(.125,-.036),(.173,-.032),(.205,-.010),(.222,.006),(.228,.005)]
        verts = [(x+side*d,cy+r*math.cos(2*math.pi*i/n),cz+r*math.sin(2*math.pi*i/n)) for r,d in profile for i in range(n)]
        faces=[]
        for ring in range(len(profile)-1):
            for i in range(n):
                # Ten cut-out slots in the dished steel, not painted black spots.
                if ring==2 and i%8 in (2,3,4,5): continue
                a,b=ring*n+i,ring*n+(i+1)%n;c,d=(ring+1)*n+i,(ring+1)*n+(i+1)%n
                faces.append((a,c,d,b) if side>0 else (a,b,d,c))
        mesh_object('CJ pressed steel rim',verts,faces,'WheelSteel')
        rod('CJ dark hub',(x-side*.025,cy,cz),(x+side*.018,cy,cz),.048,'Chassis',32)
        for i in range(5):
            a=2*math.pi*i/5;y,z=cy+.089*math.cos(a),cz+.089*math.sin(a)
            rod('CJ five lug nut',(x-side*.018,y,z),(x+side*.009,y,z),.011,'WheelSteel',6)
        # Thin bead rings emphasise the real source tire shape, without replacing
        # its silhouette or inventing rotating wheel parts.
        for radius in (.249,):
            ring=[]
            for i in range(48):
                a=2*math.pi*i/48;ring.append((x-side*.006,cy+radius*math.cos(a),cz+radius*math.sin(a)))
            for i in range(48): rod('CJ tire sidewall bead',ring[i],ring[(i+1)%48],.0022,'TireRubber',6)

    for wheel in (wheels if cj else []): steel_wheel(wheel)
    P.select(created); bpy.ops.object.join(); additions = bpy.context.object
    additions.name = 'CJ authored fitted surfaces'
    scene.cursor.location = (0,0,0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR'); bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    tri = additions.modifiers.new('CJ export triangles','TRIANGULATE'); bpy.ops.object.modifier_apply(modifier=tri.name)
    # New pieces are independent from original LODs and carry black lamp masks.
    total_added = len(additions.data.polygons)
    report=[]
    for lod in range(3):
        suffix='' if lod==0 else '_LOD'+str(lod); obj=bpy.data.objects[BODY+suffix]
        original_colors = colors if lod==0 else sample_source(obj)
        slots={key:len(obj.data.materials)+i for i,key in enumerate(mats)}
        for mat in mats.values(): obj.data.materials.append(mat)
        lamp=obj.data.color_attributes.get('S47VehicleLamps'); P.require(lamp is not None,'Source lamp mask missing')
        old_lamps=sum(1 for d in lamp.data if d.color[0]>.001 or d.color[1]>.001)
        bm=bmesh.new();bm.from_mesh(obj.data);bm.faces.ensure_lookup_table()
        source_layer=bm.faces.layers.int.new('S47RetireCabinSource')
        remove=[];old_floor_faces=0
        for face in bm.faces:
            source_face=obj.data.polygons[face.index]; c=face.calc_center_median()
            protected_lamp=any(lamp.data[j].color[0]>.001 or lamp.data[j].color[1]>.001 for j in source_face.loop_indices)
            if protected_lamp or source_face.material_index!=0: continue
            face[source_layer]=1
            removed=False
            # The source-only cab diagnostic exposes scan sheets above the new
            # measured floor. Retire only triangles wholly inside that front
            # pan's footprint and below the dashboard. Requiring every vertex
            # inside preserves the outer tub/thresholds and underside, even
            # where a long scan triangle crosses the replacement boundary.
            if new_seats and all(-.63 < v.co.x < .73 and -.98 < v.co.y < .30 and
                   floor_z + .003 < v.co.z < 1.10 for v in face.verts):
                remove.append(face); old_floor_faces += 1; continue
            if -.10<c.x<.15 and -.90<c.y<0. and .98<c.z<1.70:
                # Jagged single-sided Tripo sheet standing between the front
                # seats (seen from the driver eye); nothing real is there.
                remove.append(face);continue
            if not new_seats: continue  # original atlas, seats and wheels stay untouched
            sr,sg,sb=original_colors[source_face.index]
            seat_brown=sr>sg*1.35 and sg>sb*1.18 and sr>.035
            for seat in seats:
                lo,hi=seat['source_bounds']
                pad=(.07,.09,.0) if seat_brown else (0,0,0)
                if all(lo[k]-pad[k]<c[k]<hi[k]+pad[k]+(.05 if seat_brown and k==2 else 0) for k in range(3)):
                    remove.append(face);removed=True;break
            if removed: continue
            if not cj:
                # Source exterior stays; only seat-coloured cabin remnants
                # outside the fitted cushions take the matching vinyl.
                if seat_brown and abs(c.x-.05)<.70 and -1.90<c.y<.40 and .50<c.z<1.42:face.material_index=slots['SeatVinyl']
                continue
            wheel=None
            for candidate in wheels:
                x,y,z=candidate['center'];radius=math.hypot(c.y-y,c.z-z)
                if candidate['side']*c.x>.60 and abs(c.y-y)<.51 and c.z<.96:
                    wheel=candidate
                    if radius<.232 and candidate['side']*(c.x-x)>-.065:
                        remove.append(face);removed=True
                    else:face.material_index=slots['WheelSteel' if radius<.227 else 'TireRubber']
                    break
            if removed or wheel:continue
            r,g,b=original_colors[source_face.index]; lum=(r+g+b)/3
            # Protect real orange indicator/rear marker regions. Scan graphics
            # on the hood are removed by selecting geometry, not image editing.
            if r>g*1.5 and g>b*1.15 and (c.y>1.5 or c.y<-1.7): continue
            brown=r>g*1.35 and g>b*1.18 and r>.035
            cab=abs(c.x-.05)<.70 and -1.90<c.y<.40 and .50<c.z<1.42
            if cab and brown:
                # Seat-coloured source remnants outside the replaced cushions
                # read as seat frame/vinyl, never as white paint shards.
                face.material_index=slots['SeatVinyl'];continue
            hood=c.y>.20 and c.z>1.31
            side_hood=abs(c.x-.05)>.40 and .25<c.y<1.62 and 1.19<c.z<1.53
            rollbar=c.z>1.48 and c.y<-.15
            windshield_frame=c.z>1.48 and c.y>=-.15
            side_band=abs(c.x-.05)>.50 and -.78<c.y<.95 and .80<c.z<1.30
            tailgate=abs(c.x-.05)<.56 and c.y<-1.75 and c.z>.87
            grille=abs(c.x-.05)<.29 and c.y>1.65 and .93<c.z<1.43
            # Garbled Tripo decals on hood/cowl/tailgate become clean paint.
            # Everywhere else the source luminance decides: CJ-7 fender flares,
            # rocker stripe, bumpers and grille slots stay black trim.
            # Sampled atlas values are display-referred; white enamel is
            # above .55 even in its baked shade, black plastic below .38.
            flare=False
            for wh in wheels:
                wx,wy,wz=wh['center'];rad=math.hypot(c.y-wy,c.z-wz)
                if wh['side']*c.x>.60 and rad<wh['tire_radius_m']+.17 and c.z>wz-.12 and abs(c.y-wy)<.66:flare=True
            key='IvoryPaint' if hood or side_hood or windshield_frame or tailgate or side_band or lum>.38 else 'Chassis'
            if flare and lum<.62 and not hood:key='Chassis'
            # Black padded roll bar and black rubber floor/tunnel, as on the
            # 1986 Renegade; this also hides the lumpy scan tube and floor.
            footwell=abs(c.x-.05)<.72 and -.05<c.y<.45 and .50<c.z<1.20
            if rollbar or footwell or (cab and c.z<.98 and abs(c.x-.05)<.62):key='Chassis'
            if grille and lum<.09:key='Chassis'
            face.material_index=slots[key]
        P.require((not new_seats and old_floor_faces == 0) or 0 < old_floor_faces < 6000, 'Unexpected retired interior floor region: '+str(old_floor_faces))
        removed=len(remove);bmesh.ops.delete(bm,geom=remove,context='FACES')
        clipped_floor = clipped_back = 0
        if new_seats:
            # Existing pan and toe board close these precise regions already.
            clipped_floor += retire_source_box(bm,source_layer,(-.63,-.98,floor_z+.003),(.73,.30,1.10))
            clipped_floor += retire_source_box(bm,source_layer,(-.63,.30,floor_z+.003),(.73,.42,1.10))
            for seat in seats:
                if seat['kind']=='front':
                    clipped_back += retire_source_box(bm,source_layer,*seat['legacy_back_bounds'])
            P.require(clipped_floor < 6000 and 0 < clipped_back < 4000, 'Unexpected cabin boundary/headrest cut')
        bm.faces.layers.int.remove(source_layer)
        # Tripo cabin shells are single sided and many face away from the
        # occupants (floor, tub sides, seat frames, roll bar). Unreal culls
        # them, so the road showed through as pale shards. Give the inspected
        # cabin volume a reversed twin surface; source faces stay unchanged.
        bm.faces.ensure_lookup_table()
        # Runtime rejects a body LOD0 above 100k triangles, so only faces that
        # turn their back to an occupant eye and lie above the new floor pan
        # get a twin; wheels and hidden chassis underside do not.
        eyes=[Vector(MIRROR_EYE),Vector((.45,-.65,1.70)),Vector((.05,-1.35,1.65))]
        skip={slots[k] for k in ('TireRubber','WheelSteel') if k in slots}
        cabin=[]
        # split/bisect may replace BMFace identities. Protect the actual corner
        # mask after clipping, not the stale set of faces from before clipping.
        lamp_layer = bm.loops.layers.float_color.get('S47VehicleLamps') or bm.loops.layers.color.get('S47VehicleLamps')
        P.require(lamp_layer is not None, 'Clipped source lamp mask missing')
        for f in bm.faces:
            c=f.calc_center_median()
            if any(lp[lamp_layer][0]>.001 or lp[lamp_layer][1]>.001 for lp in f.loops): continue
            if f.material_index in skip or not(abs(c.x-.05)<.78 and -1.95<c.y<.40 and floor_z+.008<c.z<2.15):continue
            if any(wh['side']*c.x>.55 and math.hypot(c.y-wh['center'][1],c.z-wh['center'][2])<wh['tire_radius_m']+.06 for wh in wheels):continue
            if not(obj.data.materials[f.material_index].name.startswith('M_JeepInterior_CJ') or f.material_index==0):continue
            if any(f.normal.dot(e-c)<0 for e in eyes):cabin.append(f)
        first_twin=len(bm.faces)
        twins=[g for g in bmesh.ops.duplicate(bm,geom=cabin)['geom'] if isinstance(g,bmesh.types.BMFace)]
        bmesh.ops.reverse_faces(bm,faces=twins)
        # Keep all surviving source coordinates exactly as authored. Split scan
        # topology is unsuitable for local smoothing; preserve its silhouette
        # and fine dents rather than opening geometric seams.
        bm.to_mesh(obj.data);bm.free();P.clean_unused(obj)
        mesh=obj.data
        normals=[tuple(n.vector) for n in mesh.corner_normals]
        for poly in mesh.polygons[first_twin:]:
            for li in poly.loop_indices:normals[li]=tuple(poly.normal)
        mesh.normals_split_custom_set(normals)
        smooth=[]
        if cj or smooth_normals:
            outside=lambda c: not(abs(c.x-.05)<.72 and -1.95<c.y<.40 and c.z<1.48)
            smooth=smooth_paint_normals(obj,slots['IvoryPaint'] if cj else 0,first_excluded=first_twin,keep=None if cj else outside)
        extra=additions.copy();extra.data=additions.data.copy();scene.collection.objects.link(extra)
        if lod:P.reduce_mesh(extra,(.60,.32)[lod-1],'fitted seat/rim distance detail')
        added_count=len(extra.data.polygons)
        P.select([extra,obj]);bpy.context.view_layer.objects.active=obj;bpy.ops.object.join()
        P.clean_unused(obj);obj.data.update()
        triangulate_derivative(obj)
        new_lamp=obj.data.color_attributes.get('S47VehicleLamps')
        after_lamps=sum(1 for d in new_lamp.data if d.color[0]>.001 or d.color[1]>.001)
        P.require(after_lamps==old_lamps,'New authored geometry changed actual source lamp corners: '+str(old_lamps)+' -> '+str(after_lamps))
        obj.hide_render=lod>0;obj.hide_set(lod>0)
        report.append(dict(lod=lod,retired_inner_floor_faces=old_floor_faces,clipped_floor_faces=clipped_floor,retired_legacy_back_faces=clipped_back,cabin_twin_faces=len(twins),removed_source_triangles=removed,added_triangles=added_count,locally_smoothed_vertices=len(smooth),max_smoothing_displacement_m=0.))
    bpy.data.objects.remove(additions,do_unlink=True)
    mirror_report=add_mirrors(materials,anchors)
    print('S47_JEEP_MIRRORS',json.dumps(mirror_report),flush=True)
    # Preserve the exact existing seven-part import contract and live UV convention.
    for name,part in contract['parts'].items():
        measurements=[]
        for lod,row in enumerate(part['lods']):
            obj=bpy.data.objects[name+('' if lod==0 else '_LOD'+str(lod))]
            original_location=obj.location.copy()
            if name.endswith('Steering'):obj.location=(0,0,0)
            obj.hide_set(False);bpy.context.view_layer.update();measured=P.measurements(obj)
            relative='exports/'+obj.name+'.fbx'
            saved=[]
            if name in FACE_PARTS:
                saved=[(layer,[tuple(loop.uv)for loop in layer.data])for layer in obj.data.uv_layers]
                for layer,values in saved:
                    for loop,uv in zip(layer.data,values):loop.uv=(uv[0],1-uv[1])
            try:
                if export:P.export_fbx(obj,model/relative)
            finally:
                for layer,values in saved:
                    for loop,uv in zip(layer.data,values):loop.uv=uv
                obj.location=original_location;obj.hide_set(lod>0)
            measurements.append(dict(measured,lod=lod,fbx=relative));obj.hide_render=lod>0
        # Same limit as S47TripoFleet.cpp Asset(): above it runtime silently
        # falls back to the original Jeep.
        P.require(measurements[0]['triangles']<=96000,name+' LOD0 exceeds runtime 100k triangle budget: '+str(measurements[0]['triangles']))
        part.update({k:v for k,v in measurements[0].items()if k not in ('lod','fbx')});part['lods']=measurements
    methods = {
        'cj': 'geometry-selected new PBR + fitted cushions/backrests/steel rims; clipped legacy inner floor and seat shells',
        'hybrid': 'original source atlas exterior/wheels + fitted cushions/backrests; clipped legacy inner floor and seat shells',
        'source': 'original source atlas exterior/seats/wheels',
    }
    contract['lookdev']={'source_blend':str(source/'jeep-interior.blend'),'style':style,'smooth_exterior_normals':bool(smooth_normals),'method':methods[style]+'; closed two-sided cabin, floor pan, toe board; source images unchanged','layout':layout,'edits':report,'limit':'Original fused tire silhouette retained; no separate animated road wheels.'}
    P.write_json(model/'materials.json',materials);P.write_json(model/'anchors.json',anchors);P.write_json(model/'model_contract.json',contract)
    manifest=read_json(source.parent/'manifest.json');manifest['models'][0].update(output_dir=str(model),contract='classic_jeep/model_contract.json',materials='classic_jeep/materials.json')
    if export:P.write_json(output/'manifest.json',manifest)
    bpy.ops.wm.save_as_mainfile(filepath=str(model/'jeep-lookdev.blend'))
    P.require(all(P.sha(Path(p))==h for p,h in protected.items()),'An original source changed')
    if preview:render_preview(model)
    print('S47_JEEP_LOOKDEV_READY',str(output/'manifest.json'),json.dumps(report),flush=True)


def render_preview(output):
    import bpy
    from mathutils import Vector
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=8;scene.cycles.max_bounces=3;scene.cycles.use_denoising=True
    scene.render.resolution_x=640;scene.render.resolution_y=480;scene.render.resolution_percentage=100
    scene.world=bpy.data.worlds.new('CJ inspection');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.16,.19,.22,1);scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.4
    for at,power,size in [((-3,4,5),700,4),((3,-2,4),450,3)]:
        d=bpy.data.lights.new('CJ softbox','AREA');d.energy=power;d.size=size;o=bpy.data.objects.new(d.name,d);scene.collection.objects.link(o);o.location=at;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
    d=bpy.data.cameras.new('CJ inspection');d.lens=48;o=bpy.data.objects.new(d.name,d);scene.collection.objects.link(o);scene.camera=o
    for name,pos,target in [('front',(-5,6,3.7),(0,0,1)),('side',(6,0,2.8),(0,0,1)),('rear',(-4,-5,3.4),(0,0,1))]:
        o.location=pos;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(output/('after-'+name+'.png'));bpy.ops.render.render(write_still=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--source',type=Path,default=SOURCE);parser.add_argument('--output',type=Path,default=OUTPUT);parser.add_argument('--inspect-only',action='store_true');parser.add_argument('--preview',action='store_true');parser.add_argument('--no-export',action='store_true');parser.add_argument('--style',choices=('hybrid','source','cj'),default='hybrid');parser.add_argument('--smooth-normals',action='store_true')
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:]if'--'in sys.argv else sys.argv[1:])
    run(args.source,args.output,args.inspect_only,args.preview,not args.no_export,args.style,args.smooth_normals)


if __name__=='__main__':main()
