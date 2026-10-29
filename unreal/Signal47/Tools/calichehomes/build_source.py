"""Author original Caliche house sources. Run only in root's guarded DCC slot.

No rendering, Unreal, import or package launch. All output is a separate source
package; the existing TownBuildings sources and texture pixels are read only.
"""
import argparse
import json
import math
import sys
from pathlib import Path

import bpy

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
from design import CONTRACT, CONTRACT_PATH, HOUSES, PACKAGE, PARTS, sha
from check_source import bounds, evaluate
import home_geometry
from materials import material_rows
from source_data import (indexed_triangles, portable_materials, source_inputs,
                         texture_sources, topology_digest, write_json)

ROWS = {r['name']: r for r in material_rows()}


def empty_scene(name):
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for collection in list(bpy.data.collections):
        bpy.data.collections.remove(collection)
    for mat in list(bpy.data.materials):
        bpy.data.materials.remove(mat)
    for image in list(bpy.data.images):
        bpy.data.images.remove(image)
    for mesh in list(bpy.data.meshes):
        bpy.data.meshes.remove(mesh)
    for scene in list(bpy.data.scenes)[1:]:
        bpy.data.scenes.remove(scene)
    scene = bpy.data.scenes[0]
    scene.name = name
    bpy.context.window.scene = scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1
    return scene


def material(name):
    if name in bpy.data.materials:
        return bpy.data.materials[name]
    row = ROWS[name]
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.use_backface_culling = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    p = nodes.get('Principled BSDF')
    for key, value in [('Base Color', row['base_color_linear']), ('Metallic', row['metallic']),
                       ('Roughness', row['roughness']), ('Specular IOR Level', row.get('specular', .28))]:
        p.inputs[key].default_value = value
    for role, path in row.get('textures', {}).items():
        texture = nodes.new('ShaderNodeTexImage')
        texture.image = bpy.data.images.load(str(path), check_existing=True)
        texture.image.colorspace_settings.name = 'sRGB' if role == 'base_color' else 'Non-Color'
        if role == 'base_color':
            tint = nodes.new('ShaderNodeMixRGB')
            tint.blend_type = 'MULTIPLY'
            tint.inputs[0].default_value = 1
            tint.inputs[2].default_value = row['texture_tint_linear']
            links.new(texture.outputs['Color'], tint.inputs[1])
            links.new(tint.outputs[0], p.inputs['Base Color'])
        elif role == 'roughness':
            links.new(texture.outputs['Color'], p.inputs['Roughness'])
        elif role == 'normal':
            # The shared originals are DirectX normal maps. Only the Blender
            # preview graph reverses green; native metadata keeps DirectX.
            split = nodes.new('ShaderNodeSeparateXYZ')
            invert = nodes.new('ShaderNodeMath')
            invert.operation = 'SUBTRACT'
            invert.inputs[0].default_value = 1
            combine = nodes.new('ShaderNodeCombineXYZ')
            normal = nodes.new('ShaderNodeNormalMap')
            normal.inputs['Strength'].default_value = row['normal_strength']
            links.new(texture.outputs['Color'], split.inputs[0])
            links.new(split.outputs['X'], combine.inputs['X'])
            links.new(split.outputs['Y'], invert.inputs[1])
            links.new(invert.outputs[0], combine.inputs['Y'])
            links.new(split.outputs['Z'], combine.inputs['Z'])
            links.new(combine.outputs[0], normal.inputs['Color'])
            links.new(normal.outputs[0], p.inputs['Normal'])
        else:
            raise ValueError(role)
    return mat


def selected(obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def export_fbx(path):
    # Measured TownBuildings/Mesa Diner metre/axis recipe, not Interchange's
    # default coordinate guess. Native must also check the asymmetric probe.
    result = bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH'},
                                    apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
                                    axis_forward='-Y', axis_up='Z', bake_anim=False,
                                    add_leaf_bones=False, mesh_smooth_type='FACE')
    assert result == {'FINISHED'} and path.is_file(), path


def export_glb(path):
    result = bpy.ops.export_scene.gltf(filepath=str(path), export_format='GLB', use_selection=True,
                                     use_active_scene=True, export_materials='EXPORT', export_extras=True)
    assert result == {'FINISHED'} and path.is_file(), path


def create_object(name, shape, scene):
    positions, faces, coords, material_names, smoothing, tags = indexed_triangles(shape)
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(positions, [], faces)
    mesh.update()
    used = [n for n in ROWS if n in set(material_names)]
    for n in used:
        mesh.materials.append(material(n))
    uv = mesh.uv_layers.new(name='UVMap')
    for polygon, texcoords, mat, smooth in zip(mesh.polygons, coords, material_names, smoothing):
        polygon.material_index = used.index(mat)
        polygon.use_smooth = smooth
        for li, value in zip(polygon.loop_indices, texcoords):
            uv.data[li].uv = value
    obj = bpy.data.objects.new(name, mesh)
    scene.collection.objects.link(obj)
    by_tag = {}
    for face, tag in zip(faces, tags):
        by_tag.setdefault(tag, set()).update(face)
    for tag, vertices in by_tag.items():
        obj.vertex_groups.new(name=tag[:63]).add(sorted(vertices), 1, 'REPLACE')
    mesh.calc_loop_triangles()
    assert len(mesh.loop_triangles) == len(faces)
    assert min(t.area for t in mesh.loop_triangles) >= 5e-9, name
    assert all(p.normal.length > .99 for p in mesh.polygons), name
    minimum_uv = math.inf
    for triangle in mesh.loop_triangles:
        a, b, c = [uv.data[i].uv for i in triangle.loops]
        minimum_uv = min(minimum_uv, abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x)) / 2)
    assert minimum_uv > 1e-12, (name, minimum_uv)
    return obj, used, minimum_uv


def build(output):
    output = output.resolve()
    # Keep the authored authority unmodified and avoid an unexpected output
    # directory silently picking another contract with different house levels.
    assert output == PACKAGE.resolve(), 'Output must be the dedicated ArtSource/CalicheHomes package'
    output.mkdir(parents=True, exist_ok=True)
    exports = output / 'exports'
    exports.mkdir(exist_ok=True)
    inputs = source_inputs()
    preflight, _ = evaluate()
    write_json(output / 'materials.json', portable_materials(output))
    parts, houses, lods = {}, {}, {str(i): {} for i in range(3)}
    for home, h in HOUSES.items():
        scene = empty_scene(home)
        original = bpy.data.collections.new('EDITABLE_ORIGINAL_' + home)
        scene.collection.children.link(original)
        original.hide_viewport = original.hide_render = True
        names = []
        for lod in range(3):
            for name, shape in home_geometry.build(lod).items():
                if PARTS[name]['house'] != home:
                    continue
                suffix = '' if lod == 0 else '_LOD' + str(lod)
                obj, used, minimum_uv = create_object(name + suffix, shape, scene)
                spec = PARTS[name]
                row = dict(spec, origin=[0, 0, 0], site_local_offset_m=spec['offset'],
                           bounds_local_m=bounds([v.co for v in obj.data.vertices]),
                           vertices=len(obj.data.vertices), triangles=len(obj.data.polygons),
                           materials=used, minimum_uv_area=minimum_uv,
                           closed_solids=len(shape.closed_solids),
                           minimum_signed_volume_m3=min(v['signed_volume_m3'] for v in shape.closed_solids),
                           fbx='exports/' + name + suffix + '.fbx',
                           source_mesh_sha256=topology_digest(obj.data),
                           native_policy=CONTRACT['native_policy'])
                lods[str(lod)][name] = row
                if lod == 0:
                    parts[name] = row
                    names.append(name)
                    obj['source_revision'] = CONTRACT['source_revision']
                    obj['world_mapping'] = CONTRACT['mapping']
                    obj['site_world_three_m'] = spec['site_world_three_m']
                    obj['collision_authority'] = CONTRACT['native_policy']['collision']
                    obj['no_new_admission'] = True
                    obj['component_inventory'] = json.dumps(sorted(set(shape.tags)))
                    source = obj.copy()
                    source.data = obj.data.copy()
                    source.name = 'SRC_' + name
                    original.objects.link(source)
                selected(obj)
                export_fbx(output / row['fbx'])
                if lod == 0:
                    export_glb(exports / (name + '.glb'))
                else:
                    mesh = obj.data
                    bpy.data.objects.remove(obj, do_unlink=True)
                    bpy.data.meshes.remove(mesh)
        bpy.ops.object.select_all(action='DESELECT')
        for name in names:
            bpy.data.objects[name].select_set(True)
        export_glb(output / (home + '.glb'))
        for image in bpy.data.images:
            if image.source == 'FILE':
                image.pack()
                image.filepath = bpy.path.relpath(image.filepath, start=str(output))
        scene['source_revision'] = CONTRACT['source_revision']
        scene['native_visual_status'] = 'UNVERIFIED'
        scene['retained_runtime'] = json.dumps(CONTRACT['retained_runtime'])
        blend = output / (home + '.blend')
        bpy.ops.wm.save_as_mainfile(filepath=str(blend), compress=True)
        houses[home] = dict(blend=blend.name, parts=names, scene=home,
                            editable_collection=original.name,
                            site_world_three_m=h['root_world_three_m'], style=h['style'])
        print('CALICHE_HOME_SOURCE_READY', home, sum(parts[n]['triangles'] for n in names))
    # Separate probe, after saved house scenes, excluded from all house files.
    empty_scene('AxisProbe')
    bpy.ops.mesh.primitive_cube_add(size=1, location=(2, 4, 6))
    probe = bpy.context.object
    probe.dimensions = (.2, .4, .6)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    export_fbx(exports / 'axis_probe.fbx')
    assert source_inputs() == inputs, 'A read-only source or original texture changed during authoring'
    for name, row in parts.items():
        row['lod_triangles'] = [lods[str(i)][name]['triangles'] for i in range(3)]
        row['lods'] = [{key: lods[str(i)][name][key]
                        for key in ('triangles', 'vertices', 'bounds_local_m', 'materials', 'fbx')}
                       for i in range(3)]
        row['glb'] = 'exports/' + name + '.glb'
    contract = dict(status='SOURCE_GENERATED_NATIVE_UNVERIFIED',
                    source_revision=CONTRACT['source_revision'],
                    source_contract_sha256=sha(CONTRACT_PATH),
                    materials_manifest_sha256=sha(output / 'materials.json'),
                    native_import_policy=CONTRACT['native_policy'], units='metres',
                    mapping=CONTRACT['mapping'], parts=parts, lods=lods, houses=houses,
                    materials=portable_materials(output), texture_sources=texture_sources(output),
                    source_inputs_sha256=inputs,
                    lod_triangle_totals={k: sum(r['triangles'] for r in d.values()) for k, d in lods.items()},
                    axis_probe_source_center_m=[2, 4, 6], axis_probe_source_dimensions_m=[.2, .4, .6],
                    axis_probe_expected_UE_center_cm=[200, -400, 600],
                    axis_probe_expected_UE_dimensions_cm=[20, 40, 60],
                    retained_runtime=CONTRACT['retained_runtime'],
                    retained_geometry=CONTRACT['retained_geometry'], no_new_admission=True)
    assert len(parts) == 12 and len(houses) == 4
    assert contract['lod_triangle_totals'] == preflight['lod_triangle_totals']
    generated = [*exports.glob('*.fbx'), *exports.glob('*.glb'),
                 *[output / (h + ext) for h in HOUSES for ext in ('.blend', '.glb')], output / 'materials.json']
    contract['generated_sha256'] = {str(p.relative_to(output)): sha(p) for p in sorted(generated)}
    write_json(output / 'manifest.json', contract)
    # Earlier runtime coordination used this path. Keep the alias byte-identical
    # so there is only one set of measured source data for either reader.
    (output / 'model_contract.json').write_bytes((output / 'manifest.json').read_bytes())
    print('CALICHEHOMES_BUILD_PASS', json.dumps(dict(parts=12, houses=4, lods=3,
          triangles=contract['lod_triangle_totals'], scope='Generated source files; verification/native/visual still required')))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=PACKAGE)
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    build(args.out)
