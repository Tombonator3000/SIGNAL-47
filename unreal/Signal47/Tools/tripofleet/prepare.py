"""Prepare existing textured Tripo GLBs privately; never generate vehicle geometry.

Blender CLI (only after the player has stopped):
  blender -b -t 2 --python prepare.py -- --sources /private/sources.json
Lightweight file/schema preflight, with no Blender import:
  python3 prepare.py --sources /private/sources.json --validate-only
"""
from pathlib import Path
import argparse
import base64
import hashlib
import json
import math
import re
import struct
import sys


REPO = Path(__file__).resolve().parents[4]
AXES = {'+X': (1, 0, 0), '-X': (-1, 0, 0), '+Y': (0, 1, 0),
        '-Y': (0, -1, 0), '+Z': (0, 0, 1), '-Z': (0, 0, -1)}
COORDINATES = {'blender': 'metres; X width, Y length, Z up; front +Y',
               'native_mapping': 'UEcm=(Bx,-By,Bz)*100',
               'fbx_forward': '-Y', 'fbx_up': 'Z', 'native_import_yaw_deg': 90}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def finite(value, label, low=None, high=None):
    require(isinstance(value, (float, int)) and not isinstance(value, bool)
            and math.isfinite(value), label + ' must be finite')
    require(low is None or value >= low, label + ' below minimum')
    require(high is None or value <= high, label + ' above maximum')
    return float(value)


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n')
    temporary.replace(path)


def glb_document(path):
    """Read metadata and original binary image bytes, without converting pixels."""
    raw = path.read_bytes()
    require(len(raw) >= 20, 'Truncated GLB: ' + str(path))
    magic, version, length = struct.unpack_from('<4sII', raw)
    require(magic == b'glTF' and version == 2 and length == len(raw),
            'Expected complete glTF2 binary: ' + str(path))
    chunks, offset = {}, 12
    first_type = None
    while offset < len(raw):
        require(offset + 8 <= len(raw), 'Truncated GLB chunk header')
        size, kind = struct.unpack_from('<II', raw, offset)
        offset += 8
        require(size % 4 == 0 and offset + size <= len(raw), 'Invalid GLB chunk length')
        require(kind not in chunks, 'Duplicate GLB chunk type')
        if first_type is None:
            first_type = kind
        chunks[kind] = raw[offset:offset + size]
        offset += size
    require(first_type == 0x4E4F534A, 'GLB JSON must be the first chunk')
    doc = json.loads(chunks[0x4E4F534A].decode('utf-8').rstrip(' \t\r\n\x00'))
    require(str(doc.get('asset', {}).get('version')) == '2.0', 'Expected glTF 2.0')
    require(len(doc.get('buffers', [])) == 1 and 'uri' not in doc['buffers'][0],
            'Expected one embedded GLB buffer')
    binary = chunks.get(0x004E4942, b'')
    require(0 <= doc['buffers'][0]['byteLength'] <= len(binary), 'GLB binary buffer missing')
    return doc, binary


def image_bytes(doc, binary, index, source):
    entry = doc['images'][index]
    mime = entry.get('mimeType')
    if 'bufferView' in entry:
        view = doc['bufferViews'][entry['bufferView']]
        require(view.get('buffer', 0) == 0, 'Nonembedded image buffer')
        start, size = view.get('byteOffset', 0), view['byteLength']
        require(start >= 0 and size > 0 and start + size <= len(binary), 'Invalid image range')
        data = binary[start:start + size]
    else:
        uri = entry.get('uri', '')
        require(uri.startswith('data:') and ';base64,' in uri,
                'GLB must embed its textures; external image URI needs explicit review')
        header, payload = uri.split(',', 1)
        mime = mime or header[5:].split(';')[0]
        data = base64.b64decode(payload, validate=True)
    suffix = {'image/png': '.png', 'image/jpeg': '.jpg'}.get(mime)
    require(suffix is not None and data, 'Only embedded PNG/JPEG PBR maps supported: ' + str(source))
    require((suffix == '.png' and data.startswith(b'\x89PNG\r\n\x1a\n'))
            or (suffix == '.jpg' and data.startswith(b'\xff\xd8')), 'Image signature/MIME mismatch')
    return data, suffix


def preflight_materials(doc, binary, source):
    require(not doc.get('skins'), 'Static fleet preparation will not strip a source rig')
    require(not doc.get('animations'), 'Animated source requires a dedicated retained-rig preparation')
    rows = doc.get('materials', [])
    require(rows, 'Missing original PBR materials: ' + str(source))
    names = [m.get('name', 'Material_' + str(i)) for i, m in enumerate(rows)]
    require(all(isinstance(n, str) and n for n in names) and len(set(names)) == len(names),
            'Source material names must be unique for an unambiguous original-slot map')
    variants = {}
    for mesh in doc.get('meshes', []):
        for primitive in mesh.get('primitives', []):
            require(primitive.get('mode', 4) == 4, 'Only triangle primitives supported')
            require('material' in primitive, 'Unassigned source primitive material needs review')
            require(not primitive.get('targets'), 'Morph targets require a dedicated retained source path')
            attrs = primitive.get('attributes', {})
            require('TEXCOORD_0' in attrs, 'Original UV0 is required; no automatic reproject')
            require(not any(k.startswith('COLOR_') for k in attrs),
                    'Vertex-colour multiplied PBR needs explicit native material support')
            variants.setdefault(primitive['material'], set()).add('plain')
    require(variants, 'No source triangle mesh')
    for index in variants:
        require(0 <= index < len(rows), 'Invalid primitive material index')
    for material in rows:
        unsupported = set(material.get('extensions', {})) - {'KHR_materials_emissive_strength'}
        require(not unsupported, 'PBR extensions need native review: ' + str(sorted(unsupported)))
        pbr = material.get('pbrMetallicRoughness', {})
        color = pbr.get('baseColorFactor', [1, 1, 1, 1])
        require(isinstance(color, list) and len(color) == 4, 'Invalid glTF baseColorFactor')
        for value in color:
            finite(value, 'baseColorFactor', 0, 1)
        finite(pbr.get('roughnessFactor', 1), 'roughnessFactor', 0, 1)
        finite(pbr.get('metallicFactor', 1), 'metallicFactor', 0, 1)
        finite(material.get('normalTexture', {}).get('scale', 1), 'normal scale', 0)
        finite(material.get('occlusionTexture', {}).get('strength', 1), 'occlusion strength', 0, 1)
        emission = material.get('emissiveFactor', [0, 0, 0])
        require(isinstance(emission, list) and len(emission) == 3, 'Invalid emissiveFactor')
        for value in emission:
            finite(value, 'emissiveFactor', 0, 1)
        finite(material.get('extensions', {}).get('KHR_materials_emissive_strength', {}).get(
            'emissiveStrength', 1), 'emissiveStrength', 0)
        finite(material.get('alphaCutoff', .5), 'alphaCutoff', 0)
        infos = [pbr.get('baseColorTexture'), pbr.get('metallicRoughnessTexture'),
                 material.get('normalTexture'), material.get('occlusionTexture'),
                 material.get('emissiveTexture')]
        for info in filter(None, infos):
            require(info.get('texCoord', 0) == 0 and not info.get('extensions'),
                    'UV transform/alternate UV set needs explicit native support')
            texture = doc['textures'][info['index']]
            require('source' in texture and not texture.get('extensions'),
                    'Compressed/extension texture needs explicit conversion')
            sampler = doc.get('samplers', [])[texture['sampler']] if 'sampler' in texture else {}
            require(sampler.get('wrapS', 10497) == 10497 and sampler.get('wrapT', 10497) == 10497,
                    'Clamped/mirrored samplers need explicit native support')
            image_bytes(doc, binary, texture['source'], source)
        require(material.get('alphaMode', 'OPAQUE') in ('OPAQUE', 'MASK', 'BLEND'), 'Unknown alpha mode')
    return names


def read_sources(path, only=None):
    require(path.is_file(), 'Missing private sources.json: ' + str(path))
    require(not path.is_relative_to(REPO), 'Keep private sources.json outside public checkout')
    data = json.loads(path.read_text())
    require(data.get('version') == 1 and isinstance(data.get('models'), list), 'Expected sources version1/models')
    output = Path(data['output_root']).expanduser()
    require(output.is_absolute(), 'output_root must be absolute')
    output = output.resolve()
    require(not output.is_relative_to(REPO), 'Generated sources/derivatives must remain outside public Git')
    selected, identifiers, asset_names = [], set(), set()
    for value in data['models']:
        row = dict(value)
        ident = row.get('id', '')
        require(re.fullmatch(r'[a-z][a-z0-9_]{0,39}', ident) is not None, 'Invalid model id (maximum40 ASCII characters)')
        require(ident not in identifiers, 'Duplicate model id ' + ident)
        identifiers.add(ident)
        name = row.get('name', 'SM_TripoFleet_' + ident)
        require(re.fullmatch(r'SM_[A-Za-z0-9_]{1,60}', name) is not None, 'Invalid mesh asset name (maximum63 ASCII characters)')
        require(name not in asset_names, 'Duplicate mesh name ' + name)
        asset_names.add(name)
        if only and ident not in only:
            continue
        source = Path(row['source_glb']).expanduser()
        require(source.is_absolute(), ident + ': source_glb must be absolute')
        source = source.resolve()
        require(source.is_file() and source.suffix.lower() == '.glb', ident + ': GLB source missing')
        require(not source.is_relative_to(output), 'Raw source must be outside generated output_root')
        forward, up = row['source_forward'], row['source_up']
        require(forward in AXES and up in AXES and forward[-1] != up[-1],
                ident + ': explicit orthogonal source_forward/source_up needed (after Blender glTF import)')
        dimensions = row['target_dimensions_m']
        require(isinstance(dimensions, list) and len(dimensions) == 3, 'Expected [width,length,height] metres')
        row['target_dimensions_m'] = [finite(d, ident + ' target dimension', .001, 10000) for d in dimensions]
        require(row.get('fit', 'uniform') in ('uniform', 'exact'), 'fit must be uniform or exact')
        require(row.get('fit_axis', 'length') in ('width', 'length', 'height', 'longest'), 'Invalid fit_axis')
        require(row.get('pivot', 'ground') in ('ground', 'center'), 'Invalid pivot')
        ratios = row.get('lod_ratios', [1, .5, .2])
        require(isinstance(ratios, list) and len(ratios) == 3, 'Exactly three LOD ratios required')
        ratios = [finite(x, 'LOD ratio', .001, 1) for x in ratios]
        require(ratios[0] == 1 and 1 > ratios[1] > ratios[2], 'Expected 1 > LOD1 > LOD2 > 0')
        budget = row.get('lod0_max_triangles')
        require(budget is None or (isinstance(budget, int) and not isinstance(budget, bool) and budget >= 4),
                'lod0_max_triangles must be an integer >=4 or omitted')
        for region in row.get('lamp_regions', []) + row.get('glass_regions', []):
            require(region.get('kind') in ('head', 'tail', 'glass'), 'region kind must be head, tail or glass')
            box = region.get('bounds_blender_m')
            require(isinstance(box, list) and len(box) == 2 and all(isinstance(v, list) and len(v) == 3 for v in box),
                    'lamp_regions bounds_blender_m must be [[xmin,ymin,zmin],[xmax,ymax,zmax]] after normalization')
            for side in box:
                for component in side:
                    finite(component, 'lamp region bound')
            require(all(a < b for a, b in zip(*box)), 'Empty lamp region bounds')
        row.update(source_glb=str(source), name=name, lod_ratios=ratios,
                   lod_bounds_tolerance=finite(row.get('lod_bounds_tolerance', .08), 'LOD bounds tolerance', 0, .25))
        doc, binary = glb_document(source)
        preflight_materials(doc, binary, source)
        row['source_sha256'] = sha(source)
        destination = output / ident
        stamp = destination / 'original-source.json'
        if (destination / 'original.blend').exists():
            require(stamp.exists() and json.loads(stamp.read_text()).get('sha256') == row['source_sha256'],
                    ident + ': original.blend belongs to another source; use a new private output root')
        selected.append(row)
    require(selected, 'No selected models')
    require(not only or only <= identifiers, 'Unknown --only model id: ' + str(only - identifiers if only else ''))
    return output, selected


def texture_rows(doc, binary, source, output, ident):
    rows, image_paths = [], {}

    def image(info):
        if not info:
            return None
        index = doc['textures'][info['index']]['source']
        if index not in image_paths:
            raw, suffix = image_bytes(doc, binary, index, source)
            relative = 'textures/T_' + ident + '_' + str(index).zfill(2) + suffix
            (output / relative).write_bytes(raw)
            image_paths[index] = relative
        return image_paths[index]

    for index, source_material in enumerate(doc['materials']):
        pbr = source_material.get('pbrMetallicRoughness', {})
        alpha_mode = source_material.get('alphaMode', 'OPAQUE')
        color = pbr.get('baseColorFactor', [1, 1, 1, 1])
        row = {'name': 'M_TripoFleet_' + ident + '_' + str(index).zfill(2),
               'source_name': source_material.get('name', 'Material_' + str(index)), 'source_index': index,
               'base_color_linear': color, 'roughness': pbr.get('roughnessFactor', 1),
               'metallic': pbr.get('metallicFactor', 1),
               'normal_strength': source_material.get('normalTexture', {}).get('scale', 1),
               'normal_format': 'OpenGL', 'two_sided': source_material.get('doubleSided', False),
               'alpha_mode': alpha_mode, 'alpha_cutoff': source_material.get('alphaCutoff', .5),
               'alpha': color[3] if alpha_mode != 'OPAQUE' else 1,
               'emission': source_material.get('emissiveFactor', [0, 0, 0]) + [1],
               'emission_strength': source_material.get('extensions', {}).get(
                   'KHR_materials_emissive_strength', {}).get('emissiveStrength', 1),
               'occlusion_strength': source_material.get('occlusionTexture', {}).get('strength', 1),
               'textures': {}}
        for role, info in [('base_color', pbr.get('baseColorTexture')),
                           ('normal', source_material.get('normalTexture')),
                           ('emission', source_material.get('emissiveTexture'))]:
            relative = image(info)
            if relative:
                row['textures'][role] = relative
        packed = image(pbr.get('metallicRoughnessTexture'))
        if packed:
            row['packed_metallic_roughness'] = packed
        occlusion = image(source_material.get('occlusionTexture'))
        if occlusion:
            row['occlusion'] = occlusion
        rows.append(row)
    return rows


def select(objects):
    import bpy
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:
        obj.hide_set(False)
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]


def xyz(objects):
    import numpy as np
    return np.asarray([tuple(o.matrix_world @ v.co) for o in objects for v in o.data.vertices], dtype=float)


def clean_unused(obj):
    """Delete only loose working-copy vertices/edges, keeping every surface face."""
    import bpy
    used = {loop.vertex_index for loop in obj.data.loops}
    before = len(obj.data.vertices)
    if len(used) == before:
        return 0
    polygons = len(obj.data.polygons)
    select([obj])
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.delete_loose(use_verts=True, use_edges=True, use_faces=False)
    bpy.ops.object.mode_set(mode='OBJECT')
    obj.data.update()
    require(len(obj.data.polygons) == polygons, 'Loose vertex cleanup must not remove source faces')
    return before - len(obj.data.vertices)


def measurements(obj):
    import numpy as np
    mesh = obj.data
    mesh.calc_loop_triangles()
    loop_vertices = np.empty(len(mesh.loops), dtype=np.int32)
    mesh.loops.foreach_get('vertex_index', loop_vertices)
    # An FBX render mesh contains surface vertices. Loose control points cannot
    # serve as asymmetric render probes, even if Blender stores them in data.
    points = xyz([obj])[np.unique(loop_vertices)]
    require(len(points) > 3 and np.isfinite(points).all(), 'Invalid prepared mesh positions')
    low, high = points.min(axis=0), points.max(axis=0)
    directions = np.asarray([(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0),
                             (0, 0, 1), (0, 0, -1), (1, 2, 3), (-3, 2, 1), (2, -3, 1),
                             (3, 1, -2), (-1, -3, -2), (2, 3, -1)], dtype=float)
    probes = [points[int(np.argmax(points @ direction))].tolist() for direction in directions]
    used = sorted({face.material_index for face in mesh.polygons})
    require(all(0 <= i < len(mesh.materials) for i in used), 'Out-of-range material polygon index')
    return {'bounds_local_m': [low.tolist(), high.tolist()], 'dimensions_m': (high - low).tolist(),
            'triangles': len(mesh.loop_triangles), 'vertices': len(mesh.vertices),
            'materials': [m.name for m in mesh.materials], 'uv_layers': [u.name for u in mesh.uv_layers],
            'used_materials': [mesh.materials[i].name for i in used],
            'geometry_probes_m': probes, 'matrix_world': [list(row) for row in obj.matrix_world]}


def export_fbx(obj, path):
    import bpy
    select([obj])
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True,
        object_types={'MESH'}, apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
        axis_forward='-Y', axis_up='Z', bake_anim=False, use_mesh_modifiers=True,
        mesh_smooth_type='FACE', use_tspace=True, path_mode='AUTO', embed_textures=False)
    require(path.is_file() and path.stat().st_size > 0, 'FBX export missing')


def reduce_mesh(obj, ratio, label):
    """Reduce only the prepared working mesh, retaining its material and UV layers."""
    import bpy
    import bmesh
    import numpy as np
    protected = None
    layer = obj.data.color_attributes.get('S47VehicleLamps')
    if layer:
        rgba = np.empty(len(layer.data) * 4, dtype=np.float32)
        layer.data.foreach_get('color', rgba)
        mask = np.any(rgba.reshape((-1, 4))[:, :2] > .001, axis=1)
        keep = {p.index for p in obj.data.polygons if bool(mask[list(p.loop_indices)].any())}
        if keep:
            # LOD0 already contains a small real lens patch. Retain those exact
            # faces at lower LODs so collapse cannot erase a functioning lamp.
            # No new lens geometry, UV or material is manufactured.
            total = len(obj.data.polygons)
            target = int(total * ratio)
            require(len(keep) < target, 'Inspected protected lenses exceed this LOD budget')
            protected = obj.copy()
            protected.data = obj.data.copy()
            protected.name = 'S47ProtectedSourceLenses'
            bpy.context.scene.collection.objects.link(protected)
            for target_obj, keep_lenses in [(protected, True), (obj, False)]:
                bm = bmesh.new()
                bm.from_mesh(target_obj.data)
                bm.faces.ensure_lookup_table()
                remove = [face for face in bm.faces if (face.index in keep) != keep_lenses]
                bmesh.ops.delete(bm, geom=remove, context='FACES')
                loose = [v for v in bm.verts if not v.link_faces]
                if loose:
                    bmesh.ops.delete(bm, geom=loose, context='VERTS')
                bm.to_mesh(target_obj.data)
                bm.free()
                target_obj.data.update()
            ratio = (target - len(keep)) / (total - len(keep))
            obj['S47_retained_lens_triangles'] = len(keep)
    select([obj])
    decimate = obj.modifiers.new(label, 'DECIMATE')
    decimate.ratio = ratio
    decimate.use_collapse_triangulate = True
    bpy.ops.object.modifier_apply(modifier=decimate.name)
    obj.data.validate(clean_customdata=False)
    obj.data.update()
    if protected:
        select([obj, protected])
        bpy.ops.object.join()
        obj.data.update()
    clean_unused(obj)


def lamp_mask(obj, regions):
    """Mark inspected source lamp geometry only; R=head, G=tail, no colour tint."""
    import numpy as np
    if not regions:
        return None
    mesh = obj.data
    coordinates = np.empty(len(mesh.vertices) * 3, dtype=np.float32)
    mesh.vertices.foreach_get('co', coordinates)
    coordinates = coordinates.reshape((-1, 3))
    masks = np.zeros((len(mesh.vertices), 2), dtype=np.float32)
    for region in regions:
        low, high = np.asarray(region['bounds_blender_m'])
        hits = np.all((coordinates >= low) & (coordinates <= high), axis=1)
        if region.get('ellipse_xz'):
            cx, cz, rx, rz = region['ellipse_xz']
            require(rx > 0 and rz > 0, 'Lamp ellipse radii must be positive')
            hits &= ((coordinates[:, 0] - cx) / rx) ** 2 + ((coordinates[:, 2] - cz) / rz) ** 2 <= 1
        require(bool(hits.any()), 'Inspected lamp region has no vertices in ' + obj.name)
        masks[hits, 0 if region['kind'] == 'head' else 1] = 1
    indices = np.empty(len(mesh.loops), dtype=np.int32)
    mesh.loops.foreach_get('vertex_index', indices)
    rgba = np.zeros((len(mesh.loops), 4), dtype=np.float32)
    rgba[:, :2] = masks[indices]
    rgba[:, 3] = 1
    layer = mesh.color_attributes.get('S47VehicleLamps') or mesh.color_attributes.new(
        name='S47VehicleLamps', type='BYTE_COLOR', domain='CORNER')
    layer.data.foreach_set('color', rgba.ravel())
    mesh.color_attributes.active_color_index = list(mesh.color_attributes).index(layer)
    mesh.color_attributes.render_color_index = list(mesh.color_attributes).index(layer)
    return {'head_corners': int(np.count_nonzero(rgba[:, 0])),
            'tail_corners': int(np.count_nonzero(rgba[:, 1])), 'attribute': layer.name}


def glass_slots(obj, regions, materials, ident):
    """Assign only complete inspected pane triangles to a separate thin glass slot."""
    import bpy
    import numpy as np
    if not regions:
        return []
    mesh = obj.data
    require(all(len(p.vertices) == 3 for p in mesh.polygons), 'Pane selection requires source triangles')
    points = np.empty(len(mesh.vertices) * 3, dtype=np.float32)
    mesh.vertices.foreach_get('co', points)
    points = points.reshape((-1, 3))
    indices = np.empty(len(mesh.polygons) * 3, dtype=np.int32)
    mesh.polygons.foreach_get('vertices', indices)
    triangle_points = points[indices.reshape((-1, 3))]
    selected = np.zeros(len(mesh.polygons), dtype=bool)
    for region in regions:
        low, high = np.asarray(region['bounds_blender_m'])
        inside = np.all((triangle_points >= low) & (triangle_points <= high), axis=(1, 2))
        require(bool(inside.any()), 'Inspected pane box has no complete source triangles')
        selected |= inside
    mapping, report = {}, []
    originals = np.asarray([p.material_index for p in mesh.polygons], dtype=np.int32)
    by_name = {r['name']: r for r in materials}
    for old_index in sorted(set(originals[selected].tolist())):
        source = mesh.materials[old_index]
        pane = source.copy()
        pane.name = 'M_TripoFleet_' + ident + '_Glass' + str(old_index)
        shader = next(n for n in pane.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
        for key, value in [('Metallic', 0.), ('Roughness', .07), ('Alpha', .12), ('Transmission Weight', 0.)]:
            socket = shader.inputs.get(key)
            if socket:
                for link in list(socket.links):
                    pane.node_tree.links.remove(link)
                socket.default_value = value
        for link in list(shader.inputs['Normal'].links):
            pane.node_tree.links.remove(link)
        pane.surface_render_method = 'BLENDED'
        pane.use_backface_culling = False
        row = dict(by_name[source.name])
        row.update(name=pane.name, source_name=row['source_name'] + ' / inspected windshield triangles',
                   roughness=.07, metallic=0., alpha_mode='BLEND', alpha=.12, two_sided=True,
                   base_color_linear=[.8, .9, .96, .12], normal_strength=1.,
                   glass_method='thin alpha pane; no screen-space refraction')
        row['textures'] = {k: v for k, v in row['textures'].items() if k == 'base_color'}
        row.pop('packed_metallic_roughness', None)
        row.pop('occlusion', None)
        materials.append(row)
        mapping[old_index] = len(mesh.materials)
        mesh.materials.append(pane)
        report.append({'source_material': source.name, 'pane_material': pane.name,
                       'triangles': int(np.count_nonzero(selected & (originals == old_index)))})
    for index in np.flatnonzero(selected):
        mesh.polygons[int(index)].material_index = mapping[int(originals[index])]
    mesh.update()
    return report


def prepare(row, output):
    import bpy
    import numpy as np
    from mathutils import Matrix, Vector

    source = Path(row['source_glb'])
    doc, binary = glb_document(source)
    names = preflight_materials(doc, binary, source)
    output.mkdir(parents=True, exist_ok=True)
    (output / 'exports').mkdir(exist_ok=True)
    (output / 'textures').mkdir(exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = 1
    bpy.ops.import_scene.gltf(filepath=str(source), import_pack_images=True)
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    require(meshes and not any(o.type == 'ARMATURE' for o in bpy.context.scene.objects), 'Expected static imported meshes')
    require(all(o.data.uv_layers for o in meshes), 'Source UV missing; no automatic reproject')
    original_objects = [{'name': o.name, 'vertices': len(o.data.vertices),
                         'material_slots': [s.material.name if s.material else None for s in o.material_slots]}
                        for o in meshes]
    if not (output / 'original.blend').exists():
        for image in bpy.data.images:
            if image.source == 'FILE' and not image.packed_file:
                image.pack()
        bpy.ops.wm.save_as_mainfile(filepath=str(output / 'original.blend'))
        write_json(output / 'original-source.json', {'source_glb': str(source), 'sha256': row['source_sha256'],
                                                     'blender': bpy.app.version_string, 'objects': original_objects})

    materials = texture_rows(doc, binary, source, output, row['id'])
    material_by_source = {m['source_name']: m for m in materials}
    # A fresh factory scene gives the importer each original unique glTF material name.
    for material in list(bpy.data.materials):
        if material.users == 0:
            continue
        require(material.name in material_by_source,
                'Ambiguous imported material name ' + material.name + '; inspect original.blend')
        material.name = material_by_source[material.name]['name']

    up = Vector(AXES[row['source_up']])
    tilt = up.rotation_difference(Vector((0, 0, 1))).to_matrix().to_4x4()
    direction = tilt.to_3x3() @ Vector(AXES[row['source_forward']])
    # Existing native FBX conversion maps Blender Y to UE -Y, while the truck
    # controller's Three -Z is UE -Y. Therefore the prepared nose must be +Y.
    rotation = Matrix.Rotation(math.pi / 2 - math.atan2(direction.y, direction.x), 4, 'Z') @ tilt
    graph = bpy.context.evaluated_depsgraph_get()
    baked = []
    for original in meshes:
        evaluated = original.evaluated_get(graph)
        mesh = bpy.data.meshes.new_from_object(evaluated, preserve_all_data_layers=True, depsgraph=graph)
        transform = rotation @ original.matrix_world
        mesh.transform(transform)
        if transform.to_3x3().determinant() < 0:
            mesh.flip_normals()
        obj = bpy.data.objects.new('FleetWorkingPart', mesh)
        bpy.context.scene.collection.objects.link(obj)
        clean_unused(obj)
        baked.append(obj)
    points = xyz(baked)
    low, high = points.min(axis=0), points.max(axis=0)
    spans = high - low
    require(np.isfinite(points).all() and min(spans) > 1e-6, 'Degenerate source dimensions')
    target = np.asarray(row['target_dimensions_m'])
    if row.get('fit', 'uniform') == 'exact':
        factors = target / spans
    else:
        axis = {'width': 0, 'length': 1, 'height': 2}.get(row.get('fit_axis', 'length'), int(np.argmax(spans)))
        factors = np.full(3, target[axis] / spans[axis])
    scaled_low, scaled_high = low * factors, high * factors
    center = (scaled_low + scaled_high) / 2
    if row.get('pivot', 'ground') == 'ground':
        center[2] = scaled_low[2]
    normalization = Matrix.Translation(-Vector(center)) @ Matrix.Diagonal((*factors.tolist(), 1))
    for obj in baked:
        obj.data.transform(normalization)
    select(baked)
    bpy.ops.object.join()
    base = bpy.context.object
    base.name = row['name']
    for obj in list(bpy.context.scene.objects):
        if obj != base:
            bpy.data.objects.remove(obj, do_unlink=True)
    require(all(m for m in base.data.materials), 'Unassigned prepared material slot')
    select([base])
    triangulate = base.modifiers.new('Original triangle topology', 'TRIANGULATE')
    bpy.ops.object.modifier_apply(modifier=triangulate.name)
    pane_selection = glass_slots(base, row.get('glass_regions', []), materials, row['id'])
    base.data.validate(clean_customdata=False)
    base.data.update()
    clean_unused(base)
    source_measurement = measurements(base)
    budget = row.get('lod0_max_triangles')
    reductions = []
    if budget is not None:
        current = source_measurement['triangles']
        # Decimate's requested ratio is approximate. Measure actual triangles and
        # allow one bounded correction, never claim the ratio itself proves a cap.
        for attempt in range(2):
            if current <= budget:
                break
            ratio = .999 * budget / current
            reduce_mesh(base, ratio, 'Explicit LOD0 triangle budget')
            base.data.calc_loop_triangles()
            after = len(base.data.loop_triangles)
            reductions.append({'input_triangles': current, 'ratio': ratio, 'output_triangles': after})
            require(0 < after < current, 'LOD0 budget reduction failed; inspect source topology')
            current = after
        require(current <= budget, 'LOD0 remains above the explicit triangle budget; inspect source topology')
    baseline = measurements(base)
    require(baseline['materials'] == source_measurement['materials'], 'LOD0 budget changed material slots')
    require(baseline['uv_layers'] == source_measurement['uv_layers'], 'LOD0 budget lost source UV layers')
    source_delta = np.abs(np.asarray(baseline['bounds_local_m']) - np.asarray(source_measurement['bounds_local_m']))
    source_tolerance = np.maximum(.01, np.asarray(source_measurement['dimensions_m']) * row['lod_bounds_tolerance'])
    require(np.all(source_delta <= source_tolerance), 'LOD0 budget changed silhouette bounds excessively; raise budget')
    lods, lod_objects = [], []
    for level, ratio in enumerate(row['lod_ratios']):
        obj = base if level == 0 else base.copy()
        if level:
            obj.data = base.data.copy()
            bpy.context.scene.collection.objects.link(obj)
            obj.name = row['name'] + '_LOD' + str(level)
            reduce_mesh(obj, ratio, 'Source silhouette reduction')
        measured = measurements(obj)
        lamp_counts = lamp_mask(obj, row.get('lamp_regions', []))
        require(measured['materials'] == baseline['materials'], 'LOD material order drift')
        require(measured['uv_layers'] == baseline['uv_layers'], 'LOD UV layers lost')
        delta = np.abs(np.asarray(measured['bounds_local_m']) - np.asarray(baseline['bounds_local_m']))
        tolerance = np.maximum(.01, np.asarray(baseline['dimensions_m']) * row['lod_bounds_tolerance'])
        require(np.all(delta <= tolerance), 'LOD silhouette bounds changed excessively; increase ratio/inspect model')
        require(measured['triangles'] > 0 and (not lods or measured['triangles'] < lods[-1]['triangles']),
                'LOD reduction produced empty/nondecreasing geometry; inspect ratio/source')
        relative = 'exports/' + row['name'] + ('' if level == 0 else '_LOD' + str(level)) + '.fbx'
        export_fbx(obj, output / relative)
        lods.append(dict(measured, lod=level, fbx=relative, requested_ratio=ratio,
                         actual_ratio=measured['triangles'] / baseline['triangles'], lamp_mask=lamp_counts))
        lod_objects.append(obj)
    for level, obj in enumerate(lod_objects):
        obj.hide_render = level > 0
        obj.hide_set(level > 0)
    base.hide_set(False)
    select([base])
    bpy.context.scene['S47_source_glb'] = str(source)
    bpy.context.scene['S47_source_sha256'] = row['source_sha256']
    bpy.context.scene['S47_coordinates'] = json.dumps(COORDINATES)
    bpy.ops.wm.save_as_mainfile(filepath=str(output / 'prepared.blend'))
    require(sha(source) == row['source_sha256'], 'Raw source changed during preparation')
    material_names = set(baseline['materials'])
    materials = [m for m in materials if m['name'] in material_names]
    if row.get('lamp_regions'):
        for material in materials:
            if not material.get('glass_method'):
                material['vehicle_lamps'] = True
    write_json(output / 'materials.json', materials)
    contract = {'version': 1, 'id': row['id'], 'name': row['name'],
                'label': row.get('label', row['id']), 'source_glb': str(source),
                'source_sha256': row['source_sha256'], 'source_objects': original_objects,
                'blender': bpy.app.version_string, 'coordinates': COORDINATES,
                'lod0_preparation': {'max_triangles': budget,
                                     'source_triangles': source_measurement['triangles'],
                                     'prepared_triangles': baseline['triangles'],
                                     'source_bounds_local_m': source_measurement['bounds_local_m'],
                                     'actual_ratio': baseline['triangles'] / source_measurement['triangles'],
                                     'reductions': reductions},
                'normalization': {'source_forward': row['source_forward'], 'source_up': row['source_up'],
                                  'target_dimensions_m': row['target_dimensions_m'],
                                  'fit': row.get('fit', 'uniform'), 'fit_axis': row.get('fit_axis', 'length'),
                                  'scale_xyz': factors.tolist(), 'pivot': row.get('pivot', 'ground'),
                                  'source_oriented_dimensions': spans.tolist()},
                'lamp_regions': row.get('lamp_regions', []),
                'glass_regions': row.get('glass_regions', []), 'pane_selection': pane_selection,
                'parts': {row['name']: dict(baseline, asset_folder='/Game/Licensed/TripoFleet/' + row['id'], lods=lods)},
                'original_blend': 'original.blend', 'prepared_blend': 'prepared.blend',
                'materials': 'materials.json', 'status': 'PREPARED_NATIVE_UNVERIFIED',
                'limitations': ['No invented cabin, wheels, steering, collision, lights, animation or gameplay anchors.',
                                'Geometric LOD probes are measured; native roundtrip and visual silhouette review remain.']}
    write_json(output / 'model_contract.json', contract)
    print('S47_TRIPO_FLEET_PREPARED', row['id'], [lod['triangles'] for lod in lods], flush=True)
    return {'id': row['id'], 'name': row['name'], 'output_dir': str(output),
            'contract': row['id'] + '/model_contract.json', 'materials': row['id'] + '/materials.json',
            'source_glb': str(source), 'source_sha256': row['source_sha256'],
            'status': contract['status']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sources', required=True, type=Path)
    parser.add_argument('--only', help='Comma-separated configured model ids')
    parser.add_argument('--validate-only', action='store_true', help='No Blender import, export or output changes')
    options = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:])
    only = set(filter(None, options.only.split(','))) if options.only else None
    output, models = read_sources(options.sources.expanduser().resolve(), only)
    if options.validate_only:
        print('S47_TRIPO_FLEET_PREFLIGHT', json.dumps({'models': [m['id'] for m in models], 'output': str(output)}))
        return
    manifest_path = output / 'manifest.json'
    manifest = json.loads(manifest_path.read_text()) if manifest_path.exists() else {'version': 1, 'models': []}
    entries = {m['id']: m for m in manifest['models']}
    manifest.update(coordinates=COORDINATES)
    for row in models:
        # An interrupted generation cannot leave its previous version marked ready.
        entries.pop(row['id'], None)
        manifest['models'] = list(entries.values())
        write_json(manifest_path, manifest)
        entries[row['id']] = prepare(row, output / row['id'])
        manifest['models'] = list(entries.values())
        write_json(manifest_path, manifest)
    print('S47_TRIPO_FLEET_READY', str(manifest_path), len(entries), flush=True)


if __name__ == '__main__':
    main()
