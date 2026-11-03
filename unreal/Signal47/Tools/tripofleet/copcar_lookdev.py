"""Selective cleanup derivative of Tom's Tripo patrol car, root-run Blender CLI only.

Reads the existing private prepared/police_car (body + split driver door, three
LODs). Original GLB, original.blend, prepared.blend, exports and textures stay
untouched (sha256 checked before and after). The derivative keeps the native
names SM_TripoFleet_PoliceCar and SM_TripoFleet_PoliceCarDoor, the inspected
door profile and hinge from split_patrol_door.py, the measured scale, axes,
ground pivot and coordinate frame, and three LODs under the 96k runtime cap.

Lessons from the Jeep lookdev (2 November) are built in: no global smoothing of
split Tripo panels, no normal transfer from a smoothed proxy, the source atlas
(decals, SHERIFF/STATE POLICE lettering, stripes, light bar) stays on the
paint. Material separation is by slots that all sample the original atlas, with
different roughness/metal; new geometry only for the wheels.
"""
from pathlib import Path
import argparse
import copy
import json
import math
import shutil
import sys

TOOLS = Path('/home/tom/scratch-bm/wt/saro-story-loop-20261031/unreal/Signal47/Tools/tripofleet')
HERE = Path(__file__).resolve().parent
for path in (HERE, TOOLS):
    if (path / 'prepare.py').is_file():
        sys.path.insert(0, str(path))
        break
import prepare as P
import split_patrol_door as D

PRIVATE = Path.home() / 'Documents/ChatGPT/Signal 47/modeller/tripo-fleet-20261031'
SOURCE = PRIVATE / 'prepared/police_car'
OUTPUT = Path.home() / 'scratch-bm/Signal47-copcar-20261103/candidate'
BODY = 'SM_TripoFleet_PoliceCar'
DOOR = D.NAME
ATLAS = 'M_TripoFleet_police_car_00'
LOD0_CAP = 96000


def read_json(path):
    return json.loads(path.read_text())


def protected_files(source):
    contract = read_json(source / 'model_contract.json')
    files = [Path(contract['source_glb']), source.parent / 'manifest.json']
    files += sorted(p for p in source.rglob('*') if p.is_file())
    return {str(p): P.sha(p) for p in files}


# ---------------------------------------------------------------- scene

def open_source(source):
    """Open prepared.blend and return world-space joined LOD0 working copy."""
    import bpy
    bpy.ops.wm.open_mainfile(filepath=str(source / 'prepared.blend'))
    scene = bpy.context.scene
    for obj in list(scene.objects):
        if obj.type in ('CAMERA', 'LIGHT'):
            bpy.data.objects.remove(obj, do_unlink=True)
    return scene


def joined_lod0(name='S47 copcar work'):
    """Body + door LOD0 as one world-space mesh (door is stored hinge-relative)."""
    import bpy
    body, door = bpy.data.objects[BODY], bpy.data.objects[DOOR]
    work = body.copy(); work.data = body.data.copy(); work.name = name
    bpy.context.scene.collection.objects.link(work)
    extra = door.copy(); extra.data = door.data.copy()
    bpy.context.scene.collection.objects.link(extra)
    extra.data.transform(extra.matrix_world); extra.matrix_world.identity()
    atlas = bpy.data.materials[ATLAS]
    for i, m in enumerate(extra.data.materials):
        if m and m.name.endswith('_Door'):
            extra.data.materials[i] = atlas
    P.select([work, extra]); bpy.context.view_layer.objects.active = work
    bpy.ops.object.join()
    # Join keeps duplicate slots; collapse to the single source atlas.
    mesh = work.data
    remap = {}
    for i, m in enumerate(mesh.materials):
        remap[i] = 0 if m is None or m.name in (ATLAS, ATLAS + '_Door') else i
    for poly in mesh.polygons:
        poly.material_index = remap[poly.material_index]
    while len(mesh.materials) > 1:
        mesh.materials.pop(index=len(mesh.materials) - 1)
    work.hide_render = False
    return work


def atlas_pixels(size=1024):
    import bpy
    import numpy as np
    mat = bpy.data.materials[ATLAS]
    shader = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    image = shader.inputs['Base Color'].links[0].from_node.image
    thumb = image.copy(); thumb.scale(size, size)
    pixels = np.empty(size * size * 4, dtype=np.float32); thumb.pixels.foreach_get(pixels)
    bpy.data.images.remove(thumb)
    return pixels.reshape((size, size, 4))[:, :, :3]


def face_colors(obj, pixels):
    """Median-ish atlas colour per face: average of centroid and corner samples."""
    import numpy as np
    mesh = obj.data; size = pixels.shape[0]
    uv = np.empty(len(mesh.loops) * 2, dtype=np.float32)
    mesh.uv_layers.active.data.foreach_get('uv', uv); uv = uv.reshape((-1, 2))
    out = np.zeros((len(mesh.polygons), 3), dtype=np.float32)
    for face in mesh.polygons:
        loops = list(face.loop_indices); c = uv[loops].mean(axis=0)
        pts = [c] + [c * .5 + uv[l] * .5 for l in loops]
        samples = [pixels[min(size - 1, max(0, int(p[1] * size))), min(size - 1, max(0, int(p[0] * size)))] for p in pts]
        out[face.index] = np.median(np.asarray(samples), axis=0)
    return out


def face_arrays(obj):
    import numpy as np
    mesh = obj.data
    centers = np.empty(len(mesh.polygons) * 3); mesh.polygons.foreach_get('center', centers)
    normals = np.empty(len(mesh.polygons) * 3); mesh.polygons.foreach_get('normal', normals)
    areas = np.empty(len(mesh.polygons)); mesh.polygons.foreach_get('area', areas)
    return centers.reshape((-1, 3)), normals.reshape((-1, 3)), areas


def components(obj, welded=True, eps=1e-5):
    """Connected face islands; welded=True links faces by vertex position."""
    import numpy as np
    mesh = obj.data
    co = np.empty(len(mesh.vertices) * 3); mesh.vertices.foreach_get('co', co); co = co.reshape((-1, 3))
    if welded:
        key = np.round(co / eps).astype(np.int64)
        _, ident = np.unique(key, axis=0, return_inverse=True); ident = ident.ravel()
    else:
        ident = np.arange(len(co))
    parent = list(range(len(mesh.polygons)))

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]; a = parent[a]
        return a
    owner = {}
    for face in mesh.polygons:
        for v in face.vertices:
            k = int(ident[v])
            if k in owner:
                ra, rb = find(owner[k]), find(face.index)
                if ra != rb: parent[ra] = rb
            else:
                owner[k] = face.index
    roots = np.asarray([find(i) for i in range(len(mesh.polygons))])
    return roots, ident


def ray_orientation(obj, centers, normals, eps=.004, far=20.):
    """For each face: does +normal escape, does -normal escape (BVH ray cast)."""
    import numpy as np
    from mathutils.bvhtree import BVHTree
    from mathutils import Vector
    import bmesh
    bm = bmesh.new(); bm.from_mesh(obj.data)
    tree = BVHTree.FromBMesh(bm); bm.free()
    out = np.zeros((len(centers), 2), dtype=bool)
    for i, (c, n) in enumerate(zip(centers, normals)):
        c, n = Vector(c), Vector(n)
        if n.length < .5: continue
        out[i, 0] = tree.ray_cast(c + n * eps, n, far)[0] is None
        out[i, 1] = tree.ray_cast(c - n * eps, -n, far)[0] is None
    return out


# ---------------------------------------------------------------- preview

def setup_render(scene, res=(480, 320), samples=8):
    import bpy
    from mathutils import Vector
    scene.render.engine = 'CYCLES'; scene.cycles.device = 'CPU'; scene.cycles.samples = samples
    scene.cycles.max_bounces = 3; scene.cycles.use_denoising = True
    scene.render.threads_mode = 'FIXED'; scene.render.threads = 2
    scene.render.resolution_x, scene.render.resolution_y = res; scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = 'AgX'
    world = bpy.data.worlds.new('copcar inspection'); scene.world = world; world.use_nodes = True
    bg = world.node_tree.nodes['Background']
    # Pale desert sky gradient makes glass/chrome reflections readable.
    sky = world.node_tree.nodes.new('ShaderNodeTexSky')
    if hasattr(sky, 'sky_type'):
        try: sky.sky_type = 'HOSEK_WILKIE'
        except TypeError: pass
    world.node_tree.links.new(sky.outputs[0], bg.inputs['Color']); bg.inputs['Strength'].default_value = .35
    for at, power, size in [((-4, 5, 6), 900, 4), ((4, -3, 4), 500, 3), ((0, 0, 9), 300, 6)]:
        d = bpy.data.lights.new('copcar softbox', 'AREA'); d.energy = power; d.size = size
        o = bpy.data.objects.new(d.name, d); scene.collection.objects.link(o); o.location = at
        o.rotation_euler = (Vector((0, 0, .7)) - o.location).to_track_quat('-Z', 'Y').to_euler()
    ground = bpy.data.meshes.new('copcar ground')
    ground.from_pydata([(-30, -30, 0), (30, -30, 0), (30, 30, 0), (-30, 30, 0)], [], [(0, 1, 2, 3)])
    g = bpy.data.objects.new('copcar ground', ground); scene.collection.objects.link(g)
    gm = bpy.data.materials.new('copcar caliche'); gm.use_nodes = True
    gb = next(n for n in gm.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    gb.inputs['Base Color'].default_value = (.42, .36, .29, 1); gb.inputs['Roughness'].default_value = .95
    ground.materials.append(gm)
    cam = bpy.data.cameras.new('copcar camera'); cam.lens = 50
    o = bpy.data.objects.new(cam.name, cam); scene.collection.objects.link(o); scene.camera = o
    return o


VIEWS = {
    'front': ((0, 8.4, 1.25), (0, 0, .72)),
    'side': ((-9.6, 0, 1.25), (0, 0, .72)),
    'rear': ((0, -8.4, 1.35), (0, 0, .72)),
    'threequarter': ((-5.8, 6.6, 2.6), (0, 0, .62)),
    'wheel': ((-3.2, 2.9, .75), (-1.0, 1.55, .38)),
    'panel': ((-4.2, -1.6, 1.05), (-.9, .2, .78)),
    'glass': ((-3.6, 1.8, 2.5), (-.2, -.1, 1.25)),
    'windshield': ((-1.6, 4.2, 2.3), (0, .65, 1.22)),
    'rearglass': ((1.6, -4.4, 2.3), (0, -1.3, 1.25)),
}


def render_views(scene, folder, prefix, views, res=(480, 320)):
    from mathutils import Vector
    cam = scene.camera; folder.mkdir(parents=True, exist_ok=True); out = []
    for name in views:
        pos, target = VIEWS[name]
        cam.location = pos; cam.rotation_euler = (Vector(target) - Vector(pos)).to_track_quat('-Z', 'Y').to_euler()
        scene.render.resolution_x, scene.render.resolution_y = res
        scene.render.filepath = str(folder / (prefix + name + '.png'))
        import bpy
        bpy.ops.render.render(write_still=True); out.append(scene.render.filepath)
    return out


def backface_diagnostic(materials):
    """Paint backfaces red, as a stand-in for UE culling/flipped normals."""
    for mat in materials:
        if not mat or not mat.use_nodes: continue
        nt = mat.node_tree
        out = next(n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL')
        link = out.inputs['Surface'].links[0]; src = link.from_socket
        geo = nt.nodes.new('ShaderNodeNewGeometry'); red = nt.nodes.new('ShaderNodeEmission')
        red.inputs['Color'].default_value = (1, 0, 0, 1); red.inputs['Strength'].default_value = 1.5
        mix = nt.nodes.new('ShaderNodeMixShader')
        nt.links.new(geo.outputs['Backfacing'], mix.inputs[0])
        nt.links.new(src, mix.inputs[1]); nt.links.new(red.outputs[0], mix.inputs[2])
        nt.links.new(mix.outputs[0], out.inputs['Surface'])


# ---------------------------------------------------------------- inspect

def inspect(source, output):
    import bpy
    import numpy as np
    output = output.resolve(); output.mkdir(parents=True, exist_ok=True)
    protected = protected_files(source)
    scene = open_source(source)
    report = {'objects': {}}
    for obj in scene.objects:
        if obj.type == 'MESH':
            report['objects'][obj.name] = dict(tris=sum(len(p.vertices) - 2 for p in obj.data.polygons),
                verts=len(obj.data.vertices), loc=list(obj.location), hide_render=obj.hide_render,
                materials=[m.name for m in obj.data.materials], custom_normals=obj.data.has_custom_normals,
                smooth=int(sum(p.use_smooth for p in obj.data.polygons)),
                uv=[u.name for u in obj.data.uv_layers], colors=[c.name for c in obj.data.color_attributes])
    work = joined_lod0()
    mesh = work.data
    centers, normals, areas = face_arrays(work)
    roots, ident = components(work, welded=True)
    rootsu, _ = components(work, welded=False)
    report['welded_unique_positions'] = int(ident.max() + 1); report['vertices'] = len(mesh.vertices)
    uniq, counts = np.unique(roots, return_counts=True)
    order = np.argsort(-counts)
    comps = []
    for k in order[:60]:
        sel = roots == uniq[k]
        comps.append(dict(faces=int(counts[k]), area=float(areas[sel].sum()),
                          lo=centers[sel].min(axis=0).round(3).tolist(), hi=centers[sel].max(axis=0).round(3).tolist()))
    report['welded_components'] = len(uniq); report['largest_components'] = comps
    report['unwelded_islands'] = int(len(np.unique(rootsu)))
    small = counts < 40
    report['small_component_faces'] = int(counts[small].sum()); report['small_components'] = int(small.sum())
    # Duplicate faces by rounded vertex positions.
    co = np.empty(len(mesh.vertices) * 3); mesh.vertices.foreach_get('co', co); co = co.reshape((-1, 3))
    keys = {}
    dup = 0
    for face in mesh.polygons:
        k = tuple(sorted(tuple(np.round(co[v] * 2000).astype(int)) for v in face.vertices))
        if k in keys: dup += 1
        else: keys[k] = face.index
    report['duplicate_faces'] = dup
    report['degenerate_faces'] = int((areas < 1e-9).sum())
    orient = ray_orientation(work, centers, normals)
    report['faces'] = len(mesh.polygons)
    report['out_escapes'] = int(orient[:, 0].sum()); report['in_escapes_only'] = int((~orient[:, 0] & orient[:, 1]).sum())
    report['enclosed_both_blocked'] = int((~orient[:, 0] & ~orient[:, 1]).sum())
    flipped = ~orient[:, 0] & orient[:, 1]
    report['flipped_area_m2'] = float(areas[flipped].sum()); report['total_area_m2'] = float(areas.sum())
    fl = centers[flipped]
    if len(fl):
        hist, edges = np.histogramdd(fl, bins=(4, 6, 4))
        report['flipped_hist'] = dict(edges=[e.round(2).tolist() for e in edges], counts=hist.astype(int).tolist())
    # Colour classes from atlas.
    pixels = atlas_pixels()
    col = face_colors(work, pixels)
    lum = col @ np.asarray([.2126, .7152, .0722])
    sat = col.max(axis=1) - col.min(axis=1)
    report['lum_hist'] = np.histogram(lum, bins=12, range=(0, 1))[0].tolist()
    report['sat_hist'] = np.histogram(sat, bins=10, range=(0, 1))[0].tolist()
    np.savez_compressed(output / 'inspect-faces.npz', centers=centers, normals=normals, areas=areas, colors=col,
                        roots=roots, orient=orient)
    # Slices for region reasoning: per z band, per class.
    bands = []
    for z0 in np.arange(0, 1.8, .1):
        sel = (centers[:, 2] >= z0) & (centers[:, 2] < z0 + .1)
        if sel.any():
            bands.append(dict(z=round(float(z0), 2), faces=int(sel.sum()), x=[float(centers[sel, 0].min()), float(centers[sel, 0].max())],
                              y=[float(centers[sel, 1].min()), float(centers[sel, 1].max())],
                              lum=float(np.median(lum[sel])), dark=float((lum[sel] < .08).mean())))
    report['bands'] = bands
    P.write_json(output / 'inspect.json', report)
    print('S47_COPCAR_INSPECT', json.dumps({k: v for k, v in report.items() if k not in ('largest_components', 'bands', 'objects')}), flush=True)
    # Before renders: LOD0 body + door as stored.
    bpy.data.objects.remove(work, do_unlink=True)
    for obj in scene.objects:
        if obj.type == 'MESH':
            obj.hide_render = obj.name not in (BODY, DOOR)
    setup_render(scene)
    render_views(scene, output, 'before-', ['front', 'side', 'rear', 'threequarter', 'wheel', 'glass', 'panel'])
    backface_diagnostic([m for m in bpy.data.materials if m.name.startswith('M_TripoFleet_police')])
    render_views(scene, output, 'before-backface-', ['threequarter', 'side', 'glass'])
    P.require(protected_files(source) == protected, 'A protected source changed during inspection')
    print('S47_COPCAR_INSPECT_DONE', str(output), flush=True)



# ---------------------------------------------------------------- build

# Slot plan. Every source-derived slot samples the unchanged source atlas and
# normal map with the source UVs, so SHERIFF/STATE POLICE lettering, stripes,
# badge and light-bar colours stay exactly as authored. Only gloss/metal differ.
SLOTS = {
    'Interior': dict(roughness=.9, metallic=0., normal=1.),            # source row, unchanged
    'Paint': dict(roughness=.34, metallic=0., normal=.55),
    'Chrome': dict(roughness=.22, metallic=.85, normal=.4),
    'Trim': dict(roughness=.72, metallic=0., normal=.8),
    'Lens': dict(roughness=.12, metallic=0., normal=.25),
    'Window': dict(roughness=.06, metallic=0., normal=0.),           # Tripo's painted rear glass
}
NEW = {
    'Glass': dict(color=(.018, .024, .026), roughness=.05, metallic=0., alpha=.52),
    'Tire': dict(color=(.018, .018, .019), roughness=.86, metallic=0.),
    'Steel': dict(color=(.016, .016, .017), roughness=.34, metallic=.25),
    'Hubcap': dict(color=(.80, .80, .80), roughness=.16, metallic=1.),
}
ORDER = ['Interior', 'Paint', 'Chrome', 'Trim', 'Lens', 'Window', 'Glass', 'Tire', 'Steel', 'Hubcap']


def slot_name(key):
    return ATLAS if key == 'Interior' else 'M_TripoFleet_police_car_' + key


def make_materials(rows, options):
    """Blender preview materials + sidecar rows, in ORDER."""
    import bpy
    atlas = bpy.data.materials[ATLAS]
    base = next(r for r in rows if r['name'] == ATLAS)
    mats, out = [], []
    for key in ORDER:
        name = slot_name(key)
        if key == 'Interior':
            mat = atlas; row = copy.deepcopy(base)
        elif key in SLOTS:
            spec = dict(SLOTS[key]); spec.update(options.get(key, {}))
            mat = atlas.copy(); mat.name = name
            sh = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
            for sock, val in (('Roughness', spec['roughness']), ('Metallic', spec['metallic'])):
                for link in list(sh.inputs[sock].links): mat.node_tree.links.remove(link)
                sh.inputs[sock].default_value = val
            for node in mat.node_tree.nodes:
                if node.type == 'NORMAL_MAP': node.inputs['Strength'].default_value = spec['normal']
            row = copy.deepcopy(base)
            row.update(name=name, roughness=spec['roughness'], metallic=spec['metallic'], normal_strength=spec['normal'],
                       source_name=base['source_name'] + ' / ' + key.lower() + ' faces, source atlas and UV unchanged')
        else:
            spec = NEW[key]
            mat = bpy.data.materials.new(name); mat.use_nodes = True
            sh = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
            sh.inputs['Base Color'].default_value = (*spec['color'], 1)
            sh.inputs['Roughness'].default_value = spec['roughness']; sh.inputs['Metallic'].default_value = spec['metallic']
            row = dict(name=name, source_name='authored ' + key.lower() + ' (copcar_lookdev)', base_color_linear=[*spec['color'], 1],
                       roughness=spec['roughness'], metallic=spec['metallic'], normal_strength=1, normal_format='OpenGL',
                       two_sided=True, alpha_mode='OPAQUE', alpha_cutoff=.5, alpha=1, emission=[0, 0, 0, 1],
                       emission_strength=1, occlusion_strength=1, textures={})
            if 'alpha' in spec:
                sh.inputs['Alpha'].default_value = spec['alpha']; mat.surface_render_method = 'BLENDED'
                row.update(alpha_mode='BLEND', base_color_linear=[*spec['color'], spec['alpha']],
                           glass_method='thin tinted alpha pane fitted to the measured opening; no refraction')
        mat.use_backface_culling = False
        mats.append(mat); out.append(row)
    return mats, out


def classify(obj, colors):
    """Source face -> slot key, by inspected region + atlas colour. Returns int array into ORDER."""
    import numpy as np
    c, n, _ = face_arrays(obj)
    x, y, z = c[:, 0], c[:, 1], c[:, 2]; ax = np.abs(x)
    lum = colors @ np.asarray([.2126, .7152, .0722]); sat = colors.max(axis=1) - colors.min(axis=1)
    red = (colors[:, 0] > colors[:, 1] * 1.5) & (sat > .12)
    key = np.full(len(c), ORDER.index('Paint'))
    cabin = (ax < .84) & (y > -1.32) & (y < .62) & (z > .45) & (z < 1.47)
    # Parcel shelf / rear seat back under the rear glass: keep source matte.
    cabin |= (ax < .80) & (y > -1.58) & (y <= -1.32) & (z > .45) & (z < 1.13)
    key[cabin] = ORDER.index('Interior')
    under = (n[:, 2] < -.6) & (z < .40)
    key[under] = ORDER.index('Trim')
    front, rear = y > 2.15, y < -2.15
    bumper = (front | rear) & (z < .63) & (lum > .42)
    key[bumper] = ORDER.index('Chrome')
    grill = (y > 2.22) & (ax < .52) & (z > .56) & (z < .95) & (n[:, 1] > .2)
    key[grill] = ORDER.index('Chrome')
    rub = rear & (z > .6) & (z < .72) & (lum < .16)
    pushbar = (y > 2.36) & (lum < .12) & (z < .95)
    key[rub | pushbar] = ORDER.index('Trim')
    head = front & (n[:, 1] > .3) & (ax > .5) & (ax < 1.05) & (z > .6) & (z < .92) & ((lum > .45) | (sat > .15))
    tail = rear & (n[:, 1] < -.3) & (ax > .42) & (z > .7) & (z < .96) & red
    bar = z > 1.60
    key[head | tail | bar] = ORDER.index('Lens')
    # Tripo's painted windshield and rear glass (a grey texture with a cut-out hole in the middle).
    # Tom, 3 November: the whole old pane goes; build() deletes these faces and fills each full
    # opening with one new tinted pane. Bright roof edge is kept (lum < .88).
    rn = np.asarray([0, -.634, .773]); wn = np.asarray([0, .634, .773])
    # Both sides of Tripo's thick painted pane: the outer face and the inner one facing the cabin.
    # Core box at .80; its trapezoid corners and lower edge only where the face is square to the
    # glass (> .92): hood and trunk lid meet the glass at about .77 and must stay.
    ar, aw = np.abs(n @ rn), np.abs(n @ wn)
    rearglass = ((ar > .80) & (y > -1.66) & (y < -1.02) & (z > 1.02) & (z < 1.52) & (ax < .74)) | \
                ((ar > .92) & (y > -1.80) & (y < -.98) & (z > .98) & (z < 1.54) & (ax < .92))
    windshield = ((aw > .80) & (y > .28) & (y < 1.02) & (z > 1.0) & (z < 1.50) & (ax < .74)) | \
                 ((aw > .92) & (y > .22) & (y < 1.10) & (z > .96) & (z < 1.54) & (ax < .92))
    key[rearglass | windshield] = ORDER.index('Window')
    return key


def fill_opening(tree, name, origin, d, u, v, ur, vr, step=.01, cell=.025, inset=.012, dilate=.022, min_area=.02, **_):
    """Measure one window opening by rays and return pane quads (world verts, faces) list."""
    import numpy as np
    from mathutils import Vector
    d, u, v, origin = Vector(d).normalized(), Vector(u).normalized(), Vector(v).normalized(), Vector(origin)
    us = np.arange(ur[0], ur[1], step); vs = np.arange(vr[0], vr[1], step)
    t = np.full((len(vs), len(us)), np.nan)
    for j, b in enumerate(vs):
        for i, a in enumerate(us):
            hit = tree.ray_cast(origin + u * a + v * b, d, 10)
            if hit[0] is not None: t[j, i] = hit[3]
    A, B = np.meshgrid(us, vs)
    basis = lambda a, b: np.stack([np.ones_like(a), a, b, a * a, a * b, b * b], axis=-1)
    finite = np.isfinite(t)
    body = finite & (t < np.nanquantile(t, .3) + .15)
    for _ in range(4):
        coef = np.linalg.lstsq(basis(A[body], B[body]), t[body], rcond=None)[0]
        res = np.where(finite, t - basis(A, B) @ coef, np.inf)
        body = finite & (res < .05)
    opening = ~finite | (res > .08)
    # Connected components of the opening; drop those touching the sampled border.
    label = -np.ones(opening.shape, dtype=int); comps = []
    for j0, i0 in zip(*np.nonzero(opening)):
        if label[j0, i0] >= 0: continue
        stack = [(j0, i0)]; label[j0, i0] = len(comps); cells = []; border = False
        while stack:
            j, i = stack.pop(); cells.append((j, i))
            if j in (0, len(vs) - 1) or i in (0, len(us) - 1): border = True
            for dj, di in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                jj, ii = j + dj, i + di
                if 0 <= jj < len(vs) and 0 <= ii < len(us) and opening[jj, ii] and label[jj, ii] < 0:
                    label[jj, ii] = len(comps); stack.append((jj, ii))
        comps.append((np.asarray(cells), border))
    panes = []
    r = int(round(dilate / step))
    for k, (cells, border) in enumerate(comps):
        area = len(cells) * step * step
        if border or area < min_area: continue
        mask = label == k
        grown = mask.copy()
        for _ in range(r):
            g = grown.copy(); g[1:] |= grown[:-1]; g[:-1] |= grown[1:]; g[:, 1:] |= grown[:, :-1]; g[:, :-1] |= grown[:, 1:]; grown = g
        ring = grown.copy()
        for _ in range(4):
            g = ring.copy(); g[1:] |= ring[:-1]; g[:-1] |= ring[1:]; g[:, 1:] |= ring[:, :-1]; g[:, :-1] |= ring[:, 1:]; ring = g
        ring &= ~mask & finite & (res < .05)
        if ring.sum() < 30: continue
        lc = np.linalg.lstsq(basis(A[ring], B[ring]), t[ring], rcond=None)[0]
        # Pane grid over the grown opening.
        a0, a1 = A[grown].min(), A[grown].max(); b0, b1 = B[grown].min(), B[grown].max()
        ga = np.arange(a0, a1 + cell, cell); gb = np.arange(b0, b1 + cell, cell)
        def inside(a, b):
            i = int(round((a - ur[0]) / step)); j = int(round((b - vr[0]) / step))
            return 0 <= j < len(vs) and 0 <= i < len(us) and grown[j, i]
        verts, index, faces = [], {}, []
        def vid(i, j):
            if (i, j) not in index:
                a, b = ga[i], gb[j]
                depth = float(basis(np.asarray(a), np.asarray(b)) @ lc) + inset
                p = origin + u * a + v * b + d * depth
                index[(i, j)] = len(verts); verts.append(tuple(p))
            return index[(i, j)]
        outward_wind = u.cross(v).dot(-d) > 0
        for j in range(len(gb) - 1):
            for i in range(len(ga) - 1):
                if inside((ga[i] + ga[i + 1]) / 2, (gb[j] + gb[j + 1]) / 2):
                    quad = (vid(i, j), vid(i + 1, j), vid(i + 1, j + 1), vid(i, j + 1))
                    faces.append(quad if outward_wind else quad[::-1])
        cu = float(A[mask].mean()); cv = float(B[mask].mean())
        panes.append(dict(name=name + str(len(panes)), verts=verts, faces=faces, area_m2=area,
                          center_uv=[cu, cv], rim_residual_m=float(np.median(np.abs(t[ring] - basis(A[ring], B[ring]) @ lc))),
                          outward=tuple(-d)))
    return panes


WINDOWS = [
    # name, ray origin, ray direction (into car), u, v, u range, v range
    ('side_left_', (-3, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1), (-1.75, 1.25), (.95, 1.62)),
    ('side_right_', (3, 0, 0), (-1, 0, 0), (0, 1, 0), (0, 0, 1), (-1.75, 1.25), (.95, 1.62)),
    # Windshield and rear glass: Tripo's painted panes are deleted first (classify -> Window), so
    # each ray grid sees the whole opening inside its frame and gets one full pane, square to the glass.
    ('windshield_', (0, 2.55, 3.57), (0, -.634, -.773), (1, 0, 0), (0, -.773, .634), (-1.05, 1.05), (-.66, .66), dict(dilate=.06)),
    ('rear_', (0, -2.62, 2.85), (0, .634, -.773), (1, 0, 0), (0, .773, .634), (-1.05, 1.05), (-.66, .66), dict(dilate=.09)),
]


def pane_objects(tree, scene, mats):
    import bpy
    report, door_panes, body_panes = [], [], []
    for spec in WINDOWS:
        kw = spec[7] if len(spec) > 7 else {}
        for pane in fill_opening(tree, *spec[:7], **kw):
            mesh = bpy.data.meshes.new('pane ' + pane['name'])
            mesh.from_pydata(pane['verts'], [], pane['faces'])
            # Wind every quad toward the outside (ray origin side).
            from mathutils import Vector
            out = Vector(pane['outward'])
            P.require(all(poly.normal.dot(out) > 0 for poly in mesh.polygons), 'Pane must face outward: ' + pane['name'])
            for poly in mesh.polygons: poly.use_smooth = False
            mesh.materials.append(mats[ORDER.index('Glass')])
            uv = mesh.uv_layers.new(name='UVMap')
            for poly in mesh.polygons:
                for li in poly.loop_indices:
                    co = mesh.vertices[mesh.loops[li].vertex_index].co
                    uv.data[li].uv = (co.x + co.y, co.z)
            obj = bpy.data.objects.new(mesh.name, mesh); scene.collection.objects.link(obj)
            cy, cz = (sum(p[1] for p in pane['verts']) / len(pane['verts']), sum(p[2] for p in pane['verts']) / len(pane['verts']))
            on_door = pane['name'].startswith('side_left_') and D.inside(cy, cz) and all(D.X_RANGE[0] <= p[0] <= D.X_RANGE[1] for p in pane['verts'])
            (door_panes if on_door else body_panes).append(obj)
            report.append(dict({k: v for k, v in pane.items() if k not in ('verts', 'faces')}, faces=len(pane['faces']), door=on_door,
                               centroid_yz=[cy, cz]))
            if pane['name'].startswith('rear_') and pane['area_m2'] > .3:
                body_panes.append(seal_object(tree, scene, mats, pane, spec))
    return body_panes, door_panes, report


def seal_object(tree, scene, mats, pane, spec, up=.035, over=.05, lift=.004, cell=.025):
    """Black rubber seal over the rear pane's lower edge, where Tripo's trunk-lid seam is jagged."""
    import bpy
    import numpy as np
    from mathutils import Vector
    origin, d, u, v = Vector(spec[1]), Vector(spec[2]).normalized(), Vector(spec[3]).normalized(), Vector(spec[4]).normalized()
    out = -d
    pts = np.asarray(pane['verts'])
    a = (pts - np.asarray(origin)) @ np.asarray(u); b = (pts - np.asarray(origin)) @ np.asarray(v)
    cols = {}
    for k, key in enumerate(np.round(a / cell).astype(int)):
        if key not in cols or b[k] < b[cols[key]]: cols[key] = k
    verts, faces, ring = [], [], []
    for key in sorted(cols):
        p0 = Vector(pts[cols[key]])
        top = p0 + v * (up + .022) + out * lift          # onto the glass, past the pane's tuck under the lid
        probe = p0 - v * over + Vector((0, 0, .4))
        hit = tree.ray_cast(probe, Vector((0, 0, -1)), 1.)
        low = (hit[0] + Vector((0, 0, lift))) if hit[0] is not None else p0 - v * over + out * lift
        ring.append((len(verts), len(verts) + 1)); verts += [tuple(top), tuple(low)]
    for (t0, l0), (t1, l1) in zip(ring, ring[1:]):
        faces.append((t0, l0, l1, t1))
    mesh = bpy.data.meshes.new('pane rear seal'); mesh.from_pydata(verts, [], faces)
    for poly in mesh.polygons:
        if poly.normal.z < 0: poly.flip()
        poly.use_smooth = False
    mesh.update(); mesh.materials.append(mats[ORDER.index('Tire')])
    uv = mesh.uv_layers.new(name='UVMap')
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            co = mesh.vertices[mesh.loops[li].vertex_index].co; uv.data[li].uv = (co.x, co.y + co.z)
    obj = bpy.data.objects.new(mesh.name, mesh); scene.collection.objects.link(obj)
    return obj


def wheel_object(scene, mats, wheel, segments, name):
    """Lathe a 1986 police steel wheel with dog-dish cap and P215/70R15-like tire.

    Profile (radius fraction of tire R, axial offset from outer face, slot).
    Fitted to the measured source wheel island; outer face at its measured x.
    """
    import bpy
    R = wheel['radius']; W = wheel['width']; rim = R * .56
    tire, steel, cap = ORDER.index('Tire'), ORDER.index('Steel'), ORDER.index('Hubcap')
    prof = [  # (radius, axial depth inward from outer face, slot of the band starting here)
        (rim * 1.02, W - .012, tire), (R * .74, W + .004, tire), (R * .90, W - .010, tire), (R * .975, W - .030, tire),
        (R, W - .055, tire), (R * .992, W * .70, tire), (R, W * .62, tire), (R, W * .38, tire), (R * .992, W * .30, tire),
        (R, .055, tire), (R * .975, .030, tire), (R * .90, .010, tire), (R * .74, -.004, tire), (rim * 1.04, .012, steel),
        (rim * 1.00, .020, steel), (rim * .97, .032, steel), (rim * .90, .060, steel), (rim * .80, .074, steel),
        (rim * .76, .066, steel), (rim * .70, .066, steel), (rim * .66, .074, steel),
        (rim * .50, .074, cap), (rim * .48, .066, cap), (rim * .42, .056, cap), (rim * .30, .049, cap), (rim * .15, .046, cap),
        (0., .045, cap),
    ]
    back = [(rim * 1.02, W - .012), (rim * .70, W - .030), (0., W - .040)]
    import math
    side = wheel['side']; cx, cy, cz = wheel['center']
    verts, faces, slots = [], [], []
    def ring(r, depth):
        start = len(verts)
        for s in range(segments):
            ang = 2 * math.pi * s / segments
            # depth measured inward from the outer face (toward car centre).
            verts.append((cx - side * depth, cy + r * math.cos(ang), cz + r * math.sin(ang)))
        return start
    rings = [ring(r, dep) for r, dep, _ in prof]
    for k in range(len(prof) - 1):
        a, b = rings[k], rings[k + 1]
        for s in range(segments):
            t = (s + 1) % segments
            if prof[k + 1][0] == 0. and False: pass
            faces.append((a + s, a + t, b + t, b + s)); slots.append(prof[k][2])
    brings = [rings[0]] + [ring(r, dep) for r, dep in back[1:]]
    for k in range(len(back) - 1):
        a, b = brings[k], brings[k + 1]
        for s in range(segments):
            t = (s + 1) % segments
            faces.append((a + s, b + s, b + t, a + t)); slots.append(steel)
    mesh = bpy.data.meshes.new(name); mesh.from_pydata(verts, [], faces)
    for m in mats: mesh.materials.append(m)
    for poly, slot in zip(mesh.polygons, slots): poly.material_index = slot; poly.use_smooth = True
    import bmesh
    bm = bmesh.new(); bm.from_mesh(mesh)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=1e-6)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    for e in bm.edges:
        if len(e.link_faces) == 2:
            f0, f1 = e.link_faces
            if f0.material_index != f1.material_index or f0.normal.angle(f1.normal, 0) > math.radians(38): e.smooth = False
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 3])
    bm.to_mesh(mesh); bm.free()
    uv = mesh.uv_layers.new(name='UVMap')
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            co = mesh.vertices[mesh.loops[li].vertex_index].co
            uv.data[li].uv = (co.y - cy + .5, co.z - cz + .5)
    obj = bpy.data.objects.new(name, mesh); scene.collection.objects.link(obj)
    return obj


def fit_wheels(obj, roots):
    """The four source wheels are separate welded islands; measure them."""
    import numpy as np
    c, _, _ = face_arrays(obj)
    co = np.empty(len(obj.data.vertices) * 3); obj.data.vertices.foreach_get('co', co); co = co.reshape((-1, 3))
    uniq, counts = np.unique(roots, return_counts=True)
    wheels = []
    for r in uniq[np.argsort(-counts)][1:5]:
        faces = np.nonzero(roots == r)[0]
        vids = np.unique(np.concatenate([list(obj.data.polygons[int(f)].vertices) for f in faces]))
        p = co[vids]; lo, hi = p.min(axis=0), p.max(axis=0)
        P.require(.25 < hi[0] - lo[0] < .34 and .66 < hi[1] - lo[1] < .76 and lo[2] < .02, 'Unexpected wheel island bounds')
        side = 1 if lo[0] > 0 else -1
        outer = hi[0] if side > 0 else lo[0]
        radius = float((hi[1] - lo[1] + hi[2] - lo[2]) / 4)
        wheels.append(dict(side=side, end=1 if lo[1] > 0 else -1, faces=faces, root=int(r),
                           center=[float(outer), float((lo[1] + hi[1]) / 2), radius], radius=radius,
                           width=float(min(.25, hi[0] - lo[0] - .01)), source_bounds=[lo.tolist(), hi.tolist()]))
    P.require(sorted((w['side'], w['end']) for w in wheels) == [(-1, -1), (-1, 1), (1, -1), (1, 1)], 'Four source wheels not found')
    return wheels


def cleanup(work, roots, wheels, report):
    """Delete source wheels, duplicate faces and detached specks; flip inverted shell faces."""
    import bmesh
    import numpy as np
    from mathutils.bvhtree import BVHTree
    from mathutils import Vector
    mesh = work.data
    c, n, areas = face_arrays(work)
    orient = ray_orientation(work, c, n)
    remove = set(int(f) for w in wheels for f in w['faces'])
    # Detached specks: small islands > 3 cm away from every other surface.
    uniq, counts = np.unique(roots, return_counts=True)
    wheel_roots = {w['root'] for w in wheels}
    co = np.empty(len(mesh.vertices) * 3); mesh.vertices.foreach_get('co', co); co = co.reshape((-1, 3))
    main = uniq[np.argmax(counts)]
    others = [f.index for f in mesh.polygons if roots[f.index] == main]
    bm = bmesh.new(); bm.from_mesh(mesh); bm.faces.ensure_lookup_table()
    keep = [bm.faces[i] for i in others]
    sub = bmesh.new()
    for f in keep:
        sub.faces.new([sub.verts.new(v.co) for v in f.verts]) if len(set(v.index for v in f.verts)) == len(f.verts) else None
    tree = BVHTree.FromBMesh(sub); sub.free()
    specks = []
    for r, cnt in zip(uniq, counts):
        if r == main or r in wheel_roots or cnt >= 60: continue
        faces = np.nonzero(roots == r)[0]
        if areas[faces].sum() > .02: continue
        vids = np.unique(np.concatenate([list(mesh.polygons[int(f)].vertices) for f in faces]))
        gap = min(tree.find_nearest(Vector(co[v]))[3] for v in vids)
        if gap > .03:
            specks.append(dict(faces=int(cnt), gap_m=float(gap), center=co[vids].mean(axis=0).round(3).tolist()))
            remove.update(int(f) for f in faces)
    # Duplicate faces (same rounded vertex positions).
    seen = set(); dups = 0
    for face in mesh.polygons:
        k = tuple(sorted(tuple(np.round(co[v] * 2000).astype(int)) for v in face.vertices))
        if k in seen: remove.add(face.index); dups += 1
        else: seen.add(k)
    # Inverted shell faces: the back escapes to open air, the front faces into the car.
    fhits = []
    tree2 = BVHTree.FromBMesh(bm)
    flip = set()
    for i in np.nonzero(~orient[:, 0] & orient[:, 1])[0]:
        if int(i) in remove: continue
        hit = tree2.ray_cast(Vector(c[i]) + Vector(n[i]) * .004, Vector(n[i]), 20)
        if hit[0] is not None and hit[3] > .15: flip.add(int(i))
    # Corner normals survive bmesh edits as an explicit attribute.
    normals = np.asarray([tuple(cn.vector) for cn in mesh.corner_normals], dtype=np.float32)
    bm.free()
    attr = mesh.attributes.new('S47N', 'FLOAT_VECTOR', 'CORNER'); attr.data.foreach_set('vector', normals.ravel())
    flag = mesh.attributes.new('S47Flip', 'INT8', 'FACE'); flag.data.foreach_set('value', [1 if i in flip else 0 for i in range(len(mesh.polygons))])
    bm = bmesh.new(); bm.from_mesh(mesh); bm.faces.ensure_lookup_table()
    bmesh.ops.delete(bm, geom=[bm.faces[i] for i in sorted(remove)], context='FACES')
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context='VERTS')
    bm.to_mesh(mesh); bm.free(); mesh.update()
    nrm = np.empty(len(mesh.loops) * 3, dtype=np.float32); mesh.attributes['S47N'].data.foreach_get('vector', nrm); nrm = nrm.reshape((-1, 3))
    flips = np.empty(len(mesh.polygons), dtype=np.int8); mesh.attributes['S47Flip'].data.foreach_get('value', flips)
    flipped_polys = [p for p in mesh.polygons if flips[p.index]]
    # Flip winding by reversing loops: rebuild via bmesh reverse keeps loop data with corners.
    bm = bmesh.new(); bm.from_mesh(mesh); bm.faces.ensure_lookup_table()
    bmesh.ops.reverse_faces(bm, faces=[bm.faces[p.index] for p in flipped_polys], flip_multires=False)
    bm.to_mesh(mesh); bm.free(); mesh.update()
    nrm = np.empty(len(mesh.loops) * 3, dtype=np.float32); mesh.attributes['S47N'].data.foreach_get('vector', nrm); nrm = nrm.reshape((-1, 3))
    flips = np.empty(len(mesh.polygons), dtype=np.int8); mesh.attributes['S47Flip'].data.foreach_get('value', flips)
    for p in mesh.polygons:
        if flips[p.index]:
            for li in p.loop_indices: nrm[li] = -nrm[li]
    lengths = np.linalg.norm(nrm, axis=1, keepdims=True); nrm = nrm / np.maximum(lengths, 1e-8)
    mesh.attributes.remove(mesh.attributes['S47N']); mesh.attributes.remove(mesh.attributes['S47Flip'])
    mesh.normals_split_custom_set([tuple(v) for v in nrm])
    report.update(removed_wheel_faces=sum(len(w['faces']) for w in wheels), removed_duplicate_faces=dups,
                  removed_specks=specks, flipped_inverted_faces=len(flip))


def smooth_panels(obj, keys, iterations=10, cap=.006, crease_deg=24., report=None):
    """Taubin smoothing of large paint panels on WELDED positions (no cracks).

    Seam-split duplicates move together. Fixed: open boundaries, slot borders,
    creases and panel lines (adjacent faces > crease_deg apart). Corner normals
    are recomputed only around moved positions, from the actual surface (no proxy).
    """
    import numpy as np
    mesh = obj.data
    paint = ORDER.index('Paint')
    old_normals = np.asarray([tuple(cn.vector) for cn in mesh.corner_normals])
    co = np.empty(len(mesh.vertices) * 3); mesh.vertices.foreach_get('co', co); co = co.reshape((-1, 3))
    _, ident = np.unique(np.round(co / 1e-5).astype(np.int64), axis=0, return_inverse=True); ident = ident.ravel()
    n_pos = ident.max() + 1
    pos = np.zeros((n_pos, 3)); pos[ident] = co
    _, fn, fa = face_arrays(obj)
    movable = np.ones(n_pos, dtype=bool)
    faces_of = [[] for _ in range(n_pos)]
    edge_faces = {}
    for f in mesh.polygons:
        ids = [int(ident[v]) for v in f.vertices]
        for k in ids:
            faces_of[k].append(f.index)
            if keys[f.index] != paint: movable[k] = False
        for a, b in zip(ids, ids[1:] + ids[:1]):
            edge_faces.setdefault((min(a, b), max(a, b)), []).append(f.index)
    nbr = [set() for _ in range(n_pos)]
    for (a, b), fl in edge_faces.items():
        nbr[a].add(b); nbr[b].add(a)
        if len(fl) != 2: movable[a] = movable[b] = False
    cos_c = math.cos(math.radians(crease_deg))
    for k in np.nonzero(movable)[0]:
        ns = fn[faces_of[k]]
        if (ns @ ns.T).min() < cos_c: movable[k] = False
    idx = np.nonzero(movable)[0]
    nb = [np.fromiter(nbr[k], dtype=np.int64) for k in idx]
    start = pos.copy(); cur = pos.copy()
    for it in range(iterations * 2):
        lam = .5 if it % 2 == 0 else -.53
        mean = np.asarray([cur[a].mean(axis=0) for a in nb])
        cur[idx] = cur[idx] + lam * (mean - cur[idx])
    disp = cur - start; length = np.linalg.norm(disp, axis=1)
    scale = np.minimum(1., cap / np.maximum(length, 1e-12)); cur = start + disp * scale[:, None]
    moved = np.nonzero(np.linalg.norm(cur - start, axis=1) > 1e-6)[0]
    mesh.vertices.foreach_set('co', cur[ident].ravel()); mesh.update()
    # Corner normals: recompute only around moved positions.
    _, fn2, fa2 = face_arrays(obj)
    moved_set = set(moved.tolist())
    normals = old_normals.copy()
    for f in mesh.polygons:
        for li, v in zip(f.loop_indices, f.vertices):
            k = int(ident[v])
            if k not in moved_set: continue
            ring = np.asarray(faces_of[k]); w = fa2[ring][:, None] * fn2[ring]
            sel = fn2[ring] @ fn2[f.index] > .76
            vec = w[sel].sum(axis=0); normals[li] = vec / max(np.linalg.norm(vec), 1e-12)
    mesh.normals_split_custom_set([tuple(v) for v in normals])
    if report is not None:
        report.update(smoothed_positions=int(len(moved)), movable_positions=int(len(idx)),
                      max_smoothing_displacement_m=float(np.linalg.norm(cur - start, axis=1).max()),
                      mean_smoothing_displacement_m=float(np.linalg.norm(cur - start, axis=1)[moved].mean()) if len(moved) else 0.)


def split_door(obj, door_name, glass_door, mats):
    """Exactly the split_patrol_door.py selection on source faces; door panes join the door."""
    import bpy
    from mathutils import Matrix, Vector
    door = obj.copy(); door.data = obj.data.copy(); door.name = door_name
    bpy.context.scene.collection.objects.link(door)
    D.subset(door.data, True); D.subset(obj.data, False)
    P.require(len(door.data.polygons) >= 10, 'Door profile missed panel')
    if glass_door:
        parts = []
        for g in glass_door:
            dup = g.copy(); dup.data = g.data.copy(); bpy.context.scene.collection.objects.link(dup); parts.append(dup)
        P.select([door] + parts); bpy.context.view_layer.objects.active = door; bpy.ops.object.join()
    door.data.transform(Matrix.Translation(-Vector(D.PIVOT)))
    return door


def door_materials(door, rows, cache):
    import bpy
    by = {r['name']: r for r in rows}
    for i, m in enumerate(list(door.data.materials)):
        name = m.name + '_Door'
        dm = bpy.data.materials.get(name)
        if dm is None:
            dm = m.copy(); dm.name = name; dm.use_backface_culling = False
        if name not in cache:
            row = copy.deepcopy(by[m.name]); row.update(name=name, two_sided=True, source_name=row['source_name'] + ' / source driver door panel')
            cache[name] = row
        door.data.materials[i] = dm


def build(source, output, rest):
    import bpy
    import bmesh
    import numpy as np
    from mathutils.bvhtree import BVHTree
    ap = argparse.ArgumentParser()
    ap.add_argument('--preview', action='store_true'); ap.add_argument('--no-export', action='store_true')
    ap.add_argument('--paint-normal', type=float); ap.add_argument('--paint-roughness', type=float)
    ap.add_argument('--label', default=''); ap.add_argument('--smooth-panels', action='store_true')
    opt = ap.parse_args(rest)
    options = {}
    if opt.paint_normal is not None: options.setdefault('Paint', {})['normal'] = opt.paint_normal
    if opt.paint_roughness is not None: options.setdefault('Paint', {})['roughness'] = opt.paint_roughness
    source, output = source.resolve(), output.resolve()
    P.require(not output.is_relative_to(P.REPO) and not output.is_relative_to(source.parent), 'Private output outside sources required')
    protected = protected_files(source)
    contract = read_json(source / 'model_contract.json')
    P.require(contract.get('source_panel_split', {}).get('hinge_blender_m') == D.PIVOT, 'Source door split differs from split_patrol_door.py')
    rows = read_json(source / 'materials.json')
    model = output / 'police_car'; (model / 'exports').mkdir(parents=True, exist_ok=True)
    shutil.copytree(source / 'textures', model / 'textures', dirs_exist_ok=True)
    scene = open_source(source)
    original_bounds = {}
    for lod in range(3):
        sfx = '' if lod == 0 else '_LOD' + str(lod)
        original_bounds[lod] = P.measurements(bpy.data.objects[BODY + sfx])['bounds_local_m']
    work = joined_lod0()
    report = {}
    roots, _ = components(work)
    wheels = fit_wheels(work, roots)
    cleanup(work, roots, wheels, report)
    mats, mat_rows = make_materials(rows, options)
    colors = face_colors(work, atlas_pixels())
    keys = classify(work, colors)
    mesh = work.data
    mesh.materials.clear()
    for m in mats: mesh.materials.append(m)
    mesh.polygons.foreach_set('material_index', keys.astype(np.int32))
    mesh.update()
    # Delete Tripo's painted windshield/rear glass (old texture with its cut-out); panes replace it whole.
    bm = bmesh.new(); bm.from_mesh(mesh); bm.faces.ensure_lookup_table()
    window = ORDER.index('Window')
    old_glass = [f for f in bm.faces if f.material_index == window]
    report['painted_glass_faces_removed'] = len(old_glass)
    bmesh.ops.delete(bm, geom=old_glass, context='FACES')
    bm.to_mesh(mesh); bm.free(); mesh.update()
    keys = np.empty(len(mesh.polygons), dtype=np.int32); mesh.polygons.foreach_get('material_index', keys); keys = keys.astype(np.int64)
    if opt.smooth_panels: smooth_panels(work, keys, report=report)
    report['source_slot_faces'] = {ORDER[k]: int((keys == k).sum()) for k in range(len(ORDER))}
    # Panes are measured against the cleaned source shell (wheels gone, no glass).
    bm = bmesh.new(); bm.from_mesh(mesh); tree = BVHTree.FromBMesh(bm); bm.free()
    body_panes, door_panes, pane_report = pane_objects(tree, scene, mats)
    report['panes'] = pane_report
    for w in wheels: w.pop('faces')
    report['wheels'] = wheels
    # Hide all source objects; build three LODs from the cleaned LOD0.
    source_tris = len(mesh.polygons)
    results = {BODY: [], DOOR: []}; door_rows = {}
    built = []
    for lod, (ratio, segs) in enumerate([(1., 48), (.5, 32), (.2, 20)]):
        sfx = '' if lod == 0 else '_LOD' + str(lod)
        for name in (BODY + sfx, DOOR + sfx):
            old = bpy.data.objects.get(name)
            if old: old.name = name + ' (source)'; old.hide_render = True; old.hide_set(True)
        lodsrc = work.copy(); lodsrc.data = work.data.copy(); scene.collection.objects.link(lodsrc)
        if lod: P.reduce_mesh(lodsrc, ratio, 'copcar LOD' + str(lod))
        lodsrc.name = BODY + sfx
        door = split_door(lodsrc, DOOR + sfx, door_panes, mats)
        extras = [wheel_object(scene, mats, w, segs, 'wheel%d%s' % (i, sfx)) for i, w in enumerate(wheels)]
        for g in body_panes:
            dup = g.copy(); dup.data = g.data.copy(); scene.collection.objects.link(dup); extras.append(dup)
        P.select([lodsrc] + extras); bpy.context.view_layer.objects.active = lodsrc; bpy.ops.object.join()
        body = lodsrc
        for obj in (body, door):
            obj.data.validate(clean_customdata=False); obj.data.update(); P.clean_unused(obj)
        built.append((body, door))
    # Same slot list in every LOD of a part: keep slots used by any LOD.
    for idx in (0, 1):
        objs = [b[idx] for b in built]
        used = sorted({m.name for o in objs for m in [o.data.materials[p.material_index] for p in o.data.polygons]},
                      key=lambda n: ORDER.index(n.replace('M_TripoFleet_police_car_', '').replace('00', 'Interior')) if n != ATLAS else 0)
        for o in objs:
            names = [m.name for m in o.data.materials]
            idxs = np.empty(len(o.data.polygons), dtype=np.int32); o.data.polygons.foreach_get('material_index', idxs)
            remap = np.asarray([used.index(n) if n in used else 0 for n in names], dtype=np.int32)
            keep = [bpy.data.materials[n] for n in used]
            o.data.materials.clear()
            for m in keep: o.data.materials.append(m)
            o.data.polygons.foreach_set('material_index', remap[idxs]); o.data.update()
    for lod, (body, door) in enumerate(built):
        door_materials(door, mat_rows, door_rows)
        for obj, dest in ((body, results[BODY]), (door, results[DOOR])):
            measured = P.measurements(obj)
            rel = 'exports/' + obj.name + '.fbx'
            if not opt.no_export: P.export_fbx(obj, model / rel)
            dest.append(dict(measured, lod=lod, fbx=rel))
        door.location = D.PIVOT
        for obj in (body, door): obj.hide_render = lod > 0; obj.hide_set(lod > 0)
    used_rows = {n for v in results.values() for n in v[0]['materials']}
    mat_rows = [r for r in mat_rows if r['name'] in used_rows]
    for g in body_panes + door_panes: bpy.data.objects.remove(g, do_unlink=True)
    bpy.data.objects.remove(work, do_unlink=True)
    mat_rows += [r for n, r in door_rows.items() if n in used_rows]
    # Contract checks: LOD0 cap, falling counts, unchanged slot frame, bounds vs source.
    for name, lods in results.items():
        tris = [l['triangles'] for l in lods]
        P.require(tris[0] <= LOD0_CAP, name + ' LOD0 over runtime cap: ' + str(tris[0]))
        P.require(tris[0] > tris[1] > tris[2] > 0, name + ' LOD counts must fall strictly: ' + str(tris))
        P.require(all(l['materials'] == lods[0]['materials'] for l in lods), name + ' slot frame differs between LODs')
    for lod in range(3):
        merged = np.asarray(results[BODY][lod]['bounds_local_m'])
        dlo, dhi = np.asarray(results[DOOR][lod]['bounds_local_m']) + np.asarray(D.PIVOT)
        lo = np.minimum(merged[0], dlo); hi = np.maximum(merged[1], dhi)
        orig = np.asarray(original_bounds[lod])
        report.setdefault('bounds_change_m', []).append(float(np.abs(np.asarray([lo, hi]) - orig).max()))
    new_contract = copy.deepcopy(contract)
    parts = new_contract['parts']
    parts[BODY] = dict({k: v for k, v in results[BODY][0].items() if k not in ('lod', 'fbx')}, asset_folder=parts[BODY]['asset_folder'], lods=results[BODY])
    parts[DOOR] = dict({k: v for k, v in results[DOOR][0].items() if k not in ('lod', 'fbx')}, asset_folder=parts[DOOR]['asset_folder'], lods=results[DOOR], hinge_blender_m=D.PIVOT)
    new_contract['lookdev'] = dict(script='copcar_lookdev.py', source_prepared_blend=str(source / 'prepared.blend'), label=opt.label,
                                   method='selective: source wheels replaced by fitted lathe wheels, measured tinted glass panes, '
                                          'atlas-sharing paint/chrome/trim/lens slots, specks/duplicates removed, inverted shell faces flipped; '
                                          'source atlas, UVs, decals and surviving vertex positions unchanged',
                                   slot_options=options, report=report)
    new_contract['status'] = 'PREPARED_NATIVE_UNVERIFIED'
    P.write_json(model / 'materials.json', mat_rows)
    P.write_json(model / 'model_contract.json', new_contract)
    manifest = read_json(source.parent / 'manifest.json')
    manifest['models'] = [dict(m, output_dir=str(model)) for m in manifest['models'] if m['id'] == 'police_car']
    if not opt.no_export: P.write_json(output / 'manifest.json', manifest)
    P.select([built[0][0]])
    bpy.ops.wm.save_as_mainfile(filepath=str(model / 'copcar-lookdev.blend'), compress=True)
    summary = dict(tris={k: [l['triangles'] for l in v] for k, v in results.items()},
                   **{k: v for k, v in report.items() if k not in ('panes', 'wheels')},
                   panes=[(p['name'], p['faces'], round(p['area_m2'], 3), p['door'], round(p['rim_residual_m'], 4)) for p in report['panes']])
    print('S47_COPCAR_BUILT', json.dumps(summary), flush=True)
    if opt.preview:
        setup_render(scene)
        render_views(scene, output / 'preview', 'after-', ['front', 'side', 'rear', 'threequarter', 'wheel', 'glass', 'panel', 'windshield', 'rearglass'])
        for lod in (1, 2):
            for o in built[0]: o.hide_render = True
            for o in built[lod]: o.hide_render = False
            render_views(scene, output / 'preview', 'lod%d-' % lod, ['threequarter'])
        for o in built[0]: o.hide_render = False
        for lod in (1, 2):
            for o in built[lod]: o.hide_render = True
        door0 = built[0][1]; door0.rotation_euler = (0, 0, math.radians(-58))
        render_views(scene, output / 'preview', 'dooropen-', ['threequarter', 'side'])
        door0.rotation_euler = (0, 0, 0)
        backface_diagnostic([m for m in bpy.data.materials if m.name.startswith('M_TripoFleet_police') and 'Glass' not in m.name])
        render_views(scene, output / 'preview', 'after-backface-', ['threequarter', 'side', 'glass'])
    P.require(protected_files(source) == protected, 'A protected source changed')
    print('S47_COPCAR_DONE', str(output), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=SOURCE)
    parser.add_argument('--output', type=Path, default=OUTPUT)
    parser.add_argument('--inspect', action='store_true')
    args, rest = parser.parse_known_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:])
    if args.inspect:
        return inspect(args.source, args.output)
    return build(args.source, args.output, rest)


if __name__ == '__main__':
    main()
