#!/usr/bin/env python3
"""Egne vinterklær på privat, uendret MetaHuman-restskjelett.

Bare --check-source kan kjøres med vanlig Python. Selve forfattingen kjøres av
root med Blender gjennom unreal_guard; alle mesh-/fitting-eksporter er private.
Ingen servitriseuniform eller kroppsoverflate kopieres som klesgeometri.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.dont_write_bytecode = True
sys.path.insert(0, str(HERE))
import geometry as G
import coat_yoke
import failure_scene

PRIVATE_ROOT = Path.home()/'scratch-bm/Signal47-mannequins-20261029/private'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def config():
    result = json.loads((HERE/'clothing.json').read_text())
    names = [m['name'] for m in result['materials']]
    if names != ['M_RW_Coat', 'M_RW_Trousers', 'M_RW_Shoe', 'M_RW_Trim', 'M_RW_Button']:
        raise ValueError('Materialrekkefølgen er del av FBX-kontrakten')
    if result['fbx']['axis_forward'] != '-Z' or result['fbx']['axis_up'] != 'Y':
        raise ValueError('Bevar uniform-v5 sin faktiske FBX-basis')
    fixed = {'global_scale': 1., 'apply_unit_scale': True, 'bake_space_transform': False,
             'apply_scale_options': 'FBX_SCALE_NONE', 'use_space_transform': True,
             'add_leaf_bones': False, 'bake_anim': False, 'use_armature_deform_only': False,
             'automatic_bone_orientation_on_source_import': False,
             'body_geometry_exported': False, 'reference_pose_changed': False}
    if any(result['fbx'].get(k) != value for k, value in fixed.items()):
        raise ValueError('FBX-kontrakten avviker fra den uendrede restposeeksporten')
    if result['fit']['maximum_vertex_influences'] != 4:
        raise ValueError('Bevar høyst fire beinvekter')
    return result


def skeleton(arm):
    return {'object_name': arm.name,
            'object_matrix_world': [float(x) for row in arm.matrix_world for x in row],
            'bones': [{'name': b.name, 'parent': b.parent.name if b.parent else None,
                       'use_deform': bool(b.use_deform),
                       'matrix_local': [float(x) for row in b.matrix_local for x in row]}
                      for b in arm.data.bones]}


def signature(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def bounds(points):
    return {'min': [min(p[i] for p in points) for i in range(3)],
            'max': [max(p[i] for p in points) for i in range(3)]}


def interpolate(rows, z, column):
    if z <= rows[0][0]:
        return rows[0][column]
    for a, b in zip(rows, rows[1:]):
        if z <= b[0]:
            t = (z-a[0])/(b[0]-a[0])
            return a[column]*(1-t)+b[column]*t
    return rows[-1][column]


def segment_distance(p, a, b):
    delta = G.sub(b, a)
    t = max(0., min(1., G.dot(G.sub(p, a), delta)/max(G.dot(delta, delta), 1e-12)))
    return G.length(G.sub(p, G.add(a, G.mul(delta, t))))


def private_paths(body_fbx, destination, cfg):
    source = body_fbx.expanduser().resolve(strict=True)
    output = destination.expanduser().resolve()
    if not output.is_relative_to(PRIVATE_ROOT.resolve()) or output == PRIVATE_ROOT.resolve():
        raise RuntimeError('Velg en ny undermappe av '+str(PRIVATE_ROOT))
    if any((parent/'.git').exists() for parent in [output, *output.parents]):
        raise RuntimeError('Private avledede assets kan ikke skrives inn i et Git-worktree')
    if output.exists() or source == output or output in source.parents:
        raise RuntimeError('Utmappe må være ny og atskilt fra inndata')
    before = sha(source)
    if before != cfg['source_body_fbx_sha256']:
        raise RuntimeError('Feil privat kroppskilde; ikke tilpass rigg eller skala ved gjetting')
    return source, output, before


def weight_audit(obj, arm):
    bones = {b.name for b in arm.data.bones}
    groups = {g.index: g.name for g in obj.vertex_groups}
    missing, over, foreign, error = 0, 0, set(), 0.
    for vertex in obj.data.vertices:
        weights = [(groups[g.group], g.weight) for g in vertex.groups if g.weight > 1e-8]
        missing += not weights
        over += len(weights) > 4
        foreign.update(name for name, _ in weights if name not in bones)
        error = max(error, abs(sum(w for _, w in weights)-1))
    if missing or over or foreign or error > 1e-5:
        raise RuntimeError(f'Ugyldige vekter: tomme={missing}, over4={over}, fremmede={foreign}, sumavvik={error}')
    return {'unweighted_vertices': missing, 'vertices_over_four_influences': over,
            'unknown_bones': sorted(foreign), 'maximum_weight_sum_error': error}


def clothes_snapshot(obj):
    groups = {g.index: g.name for g in obj.vertex_groups}
    return {'vertices': len(obj.data.vertices),
            'vertex_positions_sha256': signature([list(v.co) for v in obj.data.vertices]),
            'faces_slots_smoothing_sha256': signature([
                [list(p.vertices), p.material_index, bool(p.use_smooth)] for p in obj.data.polygons]),
            'uv_layers_sha256': {uv.name: signature([list(loop.uv) for loop in uv.data])
                                 for uv in obj.data.uv_layers},
            'corner_normals_sha256': signature([list(n.vector) for n in obj.data.corner_normals]),
            'bone_weights_sha256': signature([
                [[groups[g.group], float(g.weight)] for g in v.groups] for v in obj.data.vertices]),
            'materials': [m.name for m in obj.data.materials],
            'matrix_world': [float(x) for row in obj.matrix_world for x in row],
            'armature_modifiers': [(m.type, m.object.name if m.object else None,
                                    bool(m.use_deform_preserve_volume))
                                   for m in obj.modifiers if m.type == 'ARMATURE']}


def build(args, cfg):
    source, output, before = private_paths(args.body_fbx, args.output, cfg)
    import bpy

    output.parent.mkdir(parents=True, exist_ok=True)
    output.mkdir(exist_ok=False)
    report = {'status': 'STARTED_NOT_ACCEPTED', 'private_output': True,
              'input_body_fbx': str(source), 'input_body_sha256': before,
              'recipe_sha256': sha(__file__), 'geometry_sha256': sha(HERE/'geometry.py'),
              'clothing_contract_sha256': sha(HERE/'clothing.json'),
              'shoulder_recipe_sha256': sha(HERE/'coat_yoke.py'), 'pid': os.getpid(), 'parts': [],
              'producer_files': {n: coat_yoke.file_row(HERE/n) for n in ('build_clothes.py','geometry.py','clothing.json','coat_yoke.py','surface_cpu.py','failure_scene.py','verify_source.py','yoke-inputs.json')},
              'limits': ['Bare klær i kildens restpose; ingen sittende mannequinaksept',
                         'Nativebaker eier kropp/klær/ansikt og autoritativ sittepose',
                         'Ingen import-, native LOD-, proxy-, radio-, natt- eller diskkontroll her']}
    report_path = output/'clothing-report.json'
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n')
    try:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(source), automatic_bone_orientation=False,
                                 ignore_leaf_bones=False, use_anim=False)
        arms = [o for o in bpy.context.scene.objects if o.type == 'ARMATURE']
        if len(arms) != 1:
            raise RuntimeError('Kilden skal ha nøyaktig ett originalt armatur')
        arm = arms[0]
        body = max((o for o in bpy.context.scene.objects if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
        reference = skeleton(arm)
        reference_sha = signature(reference)
        # Log the actual full Blender reference before any possible rejection.
        # The armature object root has 341 bones. Native FBX/Body has 342;
        # that separate canonical gate remains in the unchanged native baker.
        report['source_reference_actual_before_gate'] = {
            'rest_skeleton': reference, 'rest_skeleton_sha256': reference_sha,
            'bone_count': len(reference['bones']), 'armature_object_name': arm.name,
            'expected_blender_bones': 341, 'separate_native_bones': 342,
            'scope': 'Actual Blender source reference, not the native import/Body reference'}
        if (reference_sha != cfg['coat_yoke']['source_skeleton_sha256']
                or len(reference['bones']) != 341 or reference['object_name'] != 'root'):
            raise RuntimeError('R12 krever eksakt målt Blender-restskjelett: 341 bein, object root, hash9b1eb; native342 er separat')
        arm.data.pose_position = 'REST'
        bpy.context.scene.unit_settings.system = 'METRIC'
        bpy.context.scene.unit_settings.scale_length = 1.
        bpy.context.view_layer.update()
        required = ['pelvis', 'spine_01', 'neck_01']+[n+'_'+s for s in 'lr'
                    for n in ['upperarm', 'lowerarm', 'hand', 'thigh', 'calf', 'foot', 'ball']]
        if any(n not in arm.data.bones for n in required):
            raise RuntimeError('Manglende originale MetaHuman-bein')
        bpy.ops.object.select_all(action='DESELECT')
        joint = lambda name: tuple(arm.matrix_world @ arm.data.bones[name].head_local)
        pelvis, neck = joint('pelvis'), joint('neck_01')
        waist = joint('spine_01')[2]
        shoulder = sum(joint('upperarm_'+s)[2] for s in 'lr')/2
        if not (.75 < pelvis[2] < 1.15 and 1.25 < neck[2] < 1.65):
            raise RuntimeError('Uventet FBX-skala eller basis; ingen automatisk kompensasjon')
        points = [tuple(body.matrix_world @ v.co) for v in body.data.vertices]
        segments = {'trunk': [(pelvis, neck)]}
        for side in 'lr':
            segments['arm_'+side] = [(joint('upperarm_'+side), joint('lowerarm_'+side)),
                                     (joint('lowerarm_'+side), joint('hand_'+side))]
            segments['leg_'+side] = [(joint('thigh_'+side), joint('calf_'+side)),
                                     (joint('calf_'+side), joint('foot_'+side))]
        owners = {name: [] for name in segments}
        for p in points:
            owner = min(segments, key=lambda key: min(segment_distance(p, a, b) for a, b in segments[key]))
            owners[owner].append(p)
        trunk = owners['trunk']+owners['leg_l']+owners['leg_r']
        centre_y = (min(p[1] for p in owners['trunk'])+max(p[1] for p in owners['trunk']))/2

        def section(z, clearance, band=.035):
            chosen = [p for p in trunk if abs(p[2]-z) < band]
            if not chosen:
                raise RuntimeError('Ingen målt kroppstverrsnitt ved '+str(z))
            return (max(abs(p[0]-pelvis[0]) for p in chosen)+clearance,
                    centre_y-min(p[1] for p in chosen)+clearance,
                    max(p[1] for p in chosen)-centre_y+clearance)

        materials = {}
        for row in cfg['materials']:
            mat = bpy.data.materials.new(row['name'])
            mat.diffuse_color = (*row['base_color_linear'], 1)
            mat.use_nodes = True
            bsdf = mat.node_tree.nodes.get('Principled BSDF')
            bsdf.inputs['Base Color'].default_value = mat.diffuse_color
            bsdf.inputs['Roughness'].default_value = row['roughness']
            bsdf.inputs['Metallic'].default_value = row['metallic']
            materials[row['name']] = mat
        parts, rigid = [], {}

        def mesh(name, data, material, thickness=0):
            counts = G.audit(data)
            md = bpy.data.meshes.new(name)
            md.from_pydata(data['vertices'], [], data['faces'])
            md.update()
            if md.validate(verbose=True):
                raise RuntimeError('Blender måtte reparere ny geometri: '+name)
            ob = bpy.data.objects.new(name, md)
            bpy.context.scene.collection.objects.link(ob)
            md.materials.append(materials[material])
            uv = md.uv_layers.new(name='GarmentUV')
            for polygon, coords in zip(md.polygons, data['uvs']):
                polygon.use_smooth = True
                for loop, value in zip(polygon.loop_indices, coords):
                    uv.data[loop].uv = value
            if thickness:
                mod = ob.modifiers.new('Egen stofftykkelse', 'SOLIDIFY')
                mod.thickness = thickness
                mod.offset = -1
                bpy.context.view_layer.objects.active = ob
                ob.select_set(True)
                bpy.ops.object.modifier_apply(modifier=mod.name)
                ob.select_set(False)
            parts.append(ob)
            report['parts'].append({'name': name, 'material': material, **counts,
                                    'constructed_surface_not_body_faces': True})
            return ob

        clear = cfg['fit']['coat_clearance_m']
        hem = pelvis[2]-.255
        profile = []
        for z in [hem, hem+.045, pelvis[2]-.15, pelvis[2]-.075, waist, waist+.07, shoulder-.15, shoulder-.055]:
            rx, front, back = section(z, clear)
            profile.append((z, max(rx, .20), front, back))
        top_width = max(section(shoulder-.055, clear)[0],
                        max(abs(joint('upperarm_'+s)[0]-pelvis[0]) for s in 'lr')+.026)
        front, back = profile[-1][2:4]
        profile += [(shoulder+.012, top_width, front*.93, back*.96),
                    (neck[2]-.018, .079, .078, .077)]
        profile.sort()
        rows = []
        for j in range(25):
            z = hem+(profile[-1][0]-hem)*j/24
            rows.append({'centre': (pelvis[0], centre_y, z),
                         'rx': interpolate(profile, z, 1), 'front': interpolate(profile, z, 2),
                         'back': interpolate(profile, z, 3),
                         'gap': .035+.10*max(0., min(1., (pelvis[2]-.06-z)/.20))})
        coat = mesh('Coat_body_open_lower_front', G.loft(rows, 64, closed=False), 'M_RW_Coat', cfg['fit']['fabric_thickness_m'])

        def coat_front(x, z, offset=.004):
            radius = interpolate(profile, z, 1)
            part = min(.99, abs(x/radius))
            curve = max(.01, 1-part**(2/.82))**(.82/2)
            return (pelvis[0]+x, centre_y-interpolate(profile, z, 2)*curve-offset, z)

        # Underfront dekker smal åpning over hoften; nedenfor er frakken faktisk åpen.
        verts = [coat_front(-.026+.052*i/4, pelvis[2]-.065+(shoulder-.12-pelvis[2]+.065)*j/9, -.006)
                 for j in range(10) for i in range(5)]
        mesh('Coat_underfront', G.panel(verts, 10, 5), 'M_RW_Trim', .0015)
        for sign in (-1, 1):
            verts = [coat_front(sign*.13-.045+.09*i/7, waist-.18+.075*j/5, .006+.001*j/5)
                     for j in range(6) for i in range(8)]
            mesh('Coat_patch_pocket_'+str(sign), G.panel(verts, 6, 8), 'M_RW_Coat', .0015)
        for z in [waist+.025, waist+.115, shoulder-.12]:
            p = coat_front(.018, z, .005)
            rows = [{'centre': (p[0], p[1]-depth, p[2]), 'u': (1, 0, 0), 'v': (0, 0, 1),
                     'rx': .0065, 'front': .0065, 'back': .0065} for depth in (0., .003)]
            mesh('Coat_horn_button_'+str(round(z, 3)), G.loft(rows, 16, cap=True, exponent=1), 'M_RW_Button')
        collar_rows = [{'centre': (pelvis[0], centre_y, neck[2]+dz), 'rx': rx, 'front': f, 'back': b, 'gap': .18}
                       for dz, rx, f, b in [(-.047, .108, .095, .095), (-.017, .084, .079, .078), (.008, .075, .073, .073)]]
        mesh('Coat_low_fold_collar', G.loft(collar_rows, 48, closed=False), 'M_RW_Trim', .002)

        sleeve_profiles = {}

        def limb(name, centres, source_points, clearance, material):
            rows = []
            for i, centre in enumerate(centres):
                axis = G.unit(G.sub(centres[min(i+1, len(centres)-1)], centres[max(0, i-1)]))
                u = G.unit(G.sub((0, -1, 0), G.mul(axis, G.dot((0, -1, 0), axis))))
                v = G.unit(G.cross(axis, u))
                chosen = [p for p in source_points if abs(G.dot(G.sub(p, centre), axis)) < .027]
                if not chosen:
                    chosen = sorted(source_points, key=lambda p: abs(G.dot(G.sub(p, centre), axis)))[:24]
                if not chosen:
                    raise RuntimeError('Tom kroppsregion: '+name)
                radial = [G.sub(p, centre) for p in chosen]
                rows.append({'centre': centre, 'u': u, 'v': v,
                             'rx': max(abs(G.dot(p, u)) for p in radial)+clearance,
                             'front': max(-G.dot(p, v) for p in radial)+clearance,
                             'back': max(G.dot(p, v) for p in radial)+clearance})
            sleeve_profiles[name] = rows
            return mesh(name, G.loft(rows, 40, exponent=1), material, .0018)

        for side in 'lr':
            a, b, c = (joint(n+'_'+side) for n in ['upperarm', 'lowerarm', 'hand'])
            centres = [G.add(a, G.mul(G.sub(b, a), t/6)) for t in range(7)]
            centres += [G.add(b, G.mul(G.sub(c, b), t/6)) for t in range(1, 7)]
            limb('Coat_full_sleeve_'+side, centres, owners['arm_'+side], cfg['fit']['sleeve_clearance_m'], 'M_RW_Coat')
            a, b, c = (joint(n+'_'+side) for n in ['thigh', 'calf', 'foot'])
            centres = [G.add(a, G.mul(G.sub(b, a), t/9)) for t in range(10)]
            centres += [G.add(b, G.mul(G.sub(c, b), t/9)) for t in range(1, 9)]
            limb('Trousers_leg_'+side, centres, owners['leg_'+side], cfg['fit']['trouser_clearance_m'], 'M_RW_Trousers')
        hip_rows = []
        for j in range(9):
            z = pelvis[2]-.13+(waist+.025-pelvis[2]+.13)*j/8
            rx, f, b = section(z, cfg['fit']['trouser_clearance_m'])
            hip_rows.append({'centre': (pelvis[0], centre_y, z), 'rx': rx, 'front': f, 'back': b})
        mesh('Trousers_waist_and_hips', G.loft(hip_rows, 56), 'M_RW_Trousers', .0018)

        for side in 'lr':
            foot, ball = joint('foot_'+side), joint('ball_'+side)
            forward = G.unit((ball[0]-foot[0], ball[1]-foot[1], 0))
            lateral = G.unit(G.cross((0, 0, 1), forward))
            v = G.cross((0, 0, 1), lateral)
            other = joint('foot_'+('r' if side == 'l' else 'l'))
            region = [p for p in points if p[2] < foot[2]+.13 and
                      math.hypot(p[0]-foot[0], p[1]-foot[1]) < math.hypot(p[0]-other[0], p[1]-other[1])]
            if not region:
                raise RuntimeError('Ingen fotpunkter')
            low = min(p[2] for p in region)

            def shoe_section(z, band=.026):
                chosen = [p for p in region if abs(p[2]-z) < band]
                if not chosen:
                    chosen = sorted(region, key=lambda p: abs(p[2]-z))[:30]
                lat, lon = [G.dot(p, lateral) for p in chosen], [G.dot(p, v) for p in chosen]
                lc, fc = (min(lat)+max(lat))/2, (min(lon)+max(lon))/2
                rx = (max(lat)-min(lat))/2+cfg['fit']['shoe_clearance_m']
                ry = (max(lon)-min(lon))/2+cfg['fit']['shoe_clearance_m']
                centre = G.add(G.mul(lateral, lc), G.mul(v, fc)); centre = (centre[0], centre[1], z)
                fit = max([1.]+[(abs(G.dot(G.sub(p, centre), lateral)/rx)**4+
                                 abs(G.dot(G.sub(p, centre), v)/ry)**4)**.25 for p in chosen])
                return {'centre': centre, 'u': lateral, 'v': v, 'rx': rx*fit, 'front': ry*fit, 'back': ry*fit}
            base_row = shoe_section(low+.025, .038)
            sole_rows = [dict(base_row, centre=(base_row['centre'][0], base_row['centre'][1], low+z)) for z in (-.004, .014)]
            sole = mesh('Shoe_sole_'+side, G.loft(sole_rows, 48, cap=True, exponent=.5), 'M_RW_Trim')
            upper_rows = [dict(base_row, centre=sole_rows[-1]['centre'])]+[shoe_section(low+z) for z in (.035, .060, .085, .110, .135)]
            upper = mesh('Shoe_upper_'+side, G.loft(upper_rows, 48, cap=True, exponent=.5), 'M_RW_Shoe')
            rigid[sole.name] = rigid[upper.name] = 'foot_'+side

        # R4 changes only own upper Coat_body. All actual licensed input,
        # lower coat outside its declared fan, and16 other parts stay fixed.
        body_snapshot = clothes_snapshot(body)
        preserved_parts = {ob.name: clothes_snapshot(ob) for ob in parts}
        yoke_values = coat_yoke.apply(body, arm, coat, parts, cfg, report, clothes_snapshot, skeleton)
        if clothes_snapshot(body) != body_snapshot or skeleton(arm) != reference:
            raise RuntimeError('Lokal frakkretting endret original Body/restskjelett')
        report['source_body_payload_preserved'] = True
        report['source_body_object_name'] = body.name
        for part in report['parts']:
            if part['name'] == coat.name:
                part['original_constructed_counts'] = {k:part[k] for k in ('vertices','triangles')}
                coat.data.calc_loop_triangles()
                part.update(vertices=len(coat.data.vertices), triangles=len(coat.data.loop_triangles))

        # Bare vekter overføres; armaturets navn, hierarki og referansematriser endres aldri.
        for ob in parts:
            bpy.context.view_layer.objects.active = ob
            ob.select_set(True)
            if ob.name in rigid:
                group = ob.vertex_groups.new(name=rigid[ob.name])
                group.add(list(range(len(ob.data.vertices))), 1., 'REPLACE')
            else:
                for group in body.vertex_groups:
                    ob.vertex_groups.new(name=group.name)
                mod = ob.modifiers.new('Eksakte kroppsvekter', 'DATA_TRANSFER')
                mod.object = body
                mod.use_vert_data = True
                mod.data_types_verts = {'VGROUP_WEIGHTS'}
                mod.vert_mapping = 'POLYINTERP_NEAREST'
                mod.layers_vgroup_select_src = 'ALL'
                mod.layers_vgroup_select_dst = 'NAME'
                bpy.ops.object.modifier_apply(modifier=mod.name)
                bpy.ops.object.vertex_group_limit_total(group_select_mode='ALL', limit=4)
                bpy.ops.object.vertex_group_normalize_all(lock_active=False)
            weight_audit(ob, arm)
            deform = ob.modifiers.new('Uendret MetaHuman-restskjelett', 'ARMATURE')
            deform.object = arm
            matrix = ob.matrix_world.copy()
            ob.parent = arm
            ob.matrix_world = matrix
            ob.select_set(False)
        coat_yoke.tag_parts(parts, report)
        bpy.ops.object.select_all(action='DESELECT')
        for ob in parts:
            ob.select_set(True)
        bpy.context.view_layer.objects.active = coat
        bpy.ops.object.join()
        clothes = bpy.context.view_layer.objects.active
        clothes.name = cfg['fbx']['mesh_name']
        old_names = [clothes.data.materials[p.material_index].name for p in clothes.data.polygons]
        clothes.data.materials.clear()
        for row in cfg['materials']:
            clothes.data.materials.append(materials[row['name']])
        ordered = [row['name'] for row in cfg['materials']]
        for polygon, name in zip(clothes.data.polygons, old_names):
            polygon.material_index = ordered.index(name)
        if set(old_names) != set(ordered):
            raise RuntimeError('Materialspor mangler eller uventet spor ble tilført')
        report['weights'] = weight_audit(clothes, arm)
        triangles = sum(len(p.vertices)-2 for p in clothes.data.polygons)
        if triangles > cfg['fit']['maximum_lod0_triangles']:
            raise RuntimeError('Klesgeometri over kontraktens øvre kildebudsjett')
        if skeleton(arm) != reference:
            raise RuntimeError('Originalt skjelett endret under forfatting')
        snapshot = clothes_snapshot(clothes)
        report.update(rest_skeleton=reference, rest_skeleton_sha256=reference_sha,
                      restpose_bounds_blender_m=bounds([tuple(clothes.matrix_world @ v.co) for v in clothes.data.vertices]),
                      vertices=len(clothes.data.vertices), triangles=triangles,
                      materials=cfg['materials'], measurements={'pelvis': pelvis, 'neck': neck, 'hem_z': hem},
                      fbx_policy=cfg['fbx'], source_body_geometry_exported=False,
                      garment_payload_snapshot=snapshot,
                      uv_origin='Egen GarmentUV per løkke fra konstruerte klesflater',
                      normals_origin='Klesgeometriens egne smooth-normaler; ingen normaloverføring fra kropp')
        blend = output/'radio_woman_clothes_rest.blend'
        bpy.ops.wm.save_as_mainfile(filepath=str(blend))
        bpy.ops.object.select_all(action='DESELECT')
        clothes.select_set(True)
        arm.select_set(True)
        bpy.context.view_layer.objects.active = clothes
        fbx = output/cfg['fbx']['filename']
        bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={'ARMATURE', 'MESH'},
                                 axis_forward='-Z', axis_up='Y', global_scale=1., apply_unit_scale=True,
                                 apply_scale_options='FBX_SCALE_NONE', use_space_transform=True,
                                 bake_space_transform=False, add_leaf_bones=False, bake_anim=False,
                                 mesh_smooth_type='FACE', use_armature_deform_only=False)
        if not fbx.is_file() or fbx.stat().st_size < 1024:
            raise RuntimeError('FBX-eksport mangler')
        bpy.ops.wm.open_mainfile(filepath=str(blend))
        reopened = bpy.data.objects[cfg['fbx']['mesh_name']]
        reopened_arm = bpy.data.objects[reference['object_name']]
        if skeleton(reopened_arm) != reference or weight_audit(reopened, reopened_arm) != report['weights']:
            raise RuntimeError('Lagring/reåpning endret rigg eller vekter')
        if [m.name for m in reopened.data.materials] != ordered:
            raise RuntimeError('Lagring/reåpning endret materialrekkefølgen')
        if clothes_snapshot(reopened) != snapshot:
            raise RuntimeError('Lagring/reåpning endret posisjoner, UV, vekter, flater eller deformasjonbinding')
        report['saved_reopened_fit'] = {}
        coat_yoke.fit_actual(coat_yoke.recover_parts(reopened, report), yoke_values, report['saved_reopened_fit'])
        if sha(source) != before:
            raise RuntimeError('Privat original-FBX ble endret')
        report.update(status='REST_CLOTHES_EXPORTED_NOT_ACCEPTED', blend_reopened=True,
                      input_body_sha256_after=sha(source),
                      output_files_sha256={str(blend): sha(blend), str(fbx): sha(fbx)})
        report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n')
        print('RADIO_WOMAN_CLOTHES_REST_EXPORTED', str(report_path))
    except Exception as error:
        report.update(status='FAIL', error=str(error), input_body_sha256_after=sha(source))
        try:
            report['failure_capture'] = failure_scene.capture(bpy, output, report, clothes_snapshot, skeleton, locals())
        except Exception as capture_error:
            report['failure_capture_error'] = repr(capture_error)
        report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n')
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('body_fbx', type=Path, nargs='?')
    parser.add_argument('output', type=Path, nargs='?')
    parser.add_argument('--check-source', action='store_true')
    parser.add_argument('--check-inputs', action='store_true')
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    cfg = config()
    if args.check_inputs:
        coat_yoke.inputs_checked(cfg)
        print('R4_ACTUAL_SKIN_INPUTS_PASS_SOURCE_FIT_UNVERIFIED')
        return
    if args.check_source:
        print('RADIO_WOMAN_CLOTHES_LIGHT_GEOMETRY_ONLY', json.dumps(G.check_geometry(), sort_keys=True))
        return
    if args.body_fbx is None or args.output is None:
        parser.error('Oppgi privat body_fbx og ny privat outputmappe')
    build(args, cfg)


if __name__ == '__main__':
    main()
