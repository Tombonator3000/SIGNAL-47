"""Fresh-editor verification of saved Caliche nature assets.

Never launches Unreal. Native RGB/alpha proof exports the saved *render* mesh,
not its source MeshDescription, then compares actual polygon-corner colors to
the authored FBX. The binary FBX reader also works in ordinary Python.
"""
import hashlib
import json
import math
import os
from pathlib import Path
import runpy
import re
import struct
import sys
import traceback
import zlib

ROOT = Path(__file__).resolve().parent


def nature_material_warnings(log):
    """Owned nature shader/instancing failures; unrelated Web/Free logs excluded."""
    bad = re.compile(r'\bSMU\b|please fix material usage|missing usage flag|default material|fallback|failed to compile|(?:warning|error).*?(?:usage flag|material usage|InstancedStaticMeshes)', re.I)
    return [line for line in log.splitlines() if 'calichestreet/nature' in line.lower() and bad.search(line)]


def nature_runtime_evidence(log, placement=None, required=True):
    """Pure read-only log audit; ready line alone cannot prove component setup."""
    ready_rows = re.findall(r'S47: Caliche nature ready: (\d+) authored instances in (\d+) HISM families, (\d+) LODs, no collision; road/story/photo exclusions preserved', log)
    components = [{'id': name, 'instances': int(count), 'cull_cm': [int(start), int(end)], 'world_transform_checks': state}
                  for name, count, start, end, state in re.findall(r'S47: nature component (\S+): instances=(\d+) cull_cm=(\d+),(\d+) world_transform_checks=(PASS|FAIL)', log)]
    cooked_prefix = 'S47: nature cooked LOD screens '
    cooked_records = re.findall(r'S47: nature cooked LOD screens (\S+): ([+-]?\d+\.\d{5}),([+-]?\d+\.\d{5}),([+-]?\d+\.\d{5})[ \t]*\r?$', log, re.M)
    cooked_screens = [{'id': name, 'screens': [float(a), float(b), float(c)], 'printed_values': [a, b, c]}
                      for name, a, b, c in cooked_records]
    has_native_evidence = bool(ready_rows or components or log.count(cooked_prefix))
    checks = []
    def check(name, value):
        checks.append({'name': name, 'pass': bool(value)})
    check('no runtime nature rejection or component mismatch', not any(marker in log for marker in ('nature rejected:', 'nature runtime component mismatch:')))
    material_warnings = nature_material_warnings(log)
    check('no Nature material usage or fallback diagnostics', not material_warnings)
    if required or has_native_evidence:
        check('one complete native ready record', len(ready_rows) == 1)
        check('twelve distinct actual HISM components', len(components) == 12 and len({row['id'] for row in components}) == 12)
        check('actual transforms counts culls and NoCollision passed native checks', bool(components) and all(row['world_transform_checks'] == 'PASS' for row in components))
        check('actual positive component counts', bool(components) and all(row['instances'] > 0 for row in components))
        check('twelve distinct exact-format cooked LOD screen records', log.count(cooked_prefix) == 12 and len(cooked_screens) == 12 and len({row['id'] for row in cooked_screens}) == 12)
        check('cooked LOD screen families match actual components', bool(cooked_screens) and {row['id'] for row in cooked_screens} == {row['id'] for row in components})
        for row in cooked_screens:
            check(row['id'] + ' actual cooked LOD screens are 1/.12/.025', row['printed_values'] == ['1.00000', '0.12000', '0.02500'])
        if len(ready_rows) == 1:
            count, families, lods = map(int, ready_rows[0])
            check('ready totals match actual component counts and three LODs', families == 12 and lods == 3 and count == sum(row['instances'] for row in components))
        if placement is not None:
            expected = placement['families']
            check('placement has twelve distinct families', len(expected) == 12 and len({row['id'] for row in expected}) == 12)
            observed = {row['id']: row for row in components}
            check('actual family IDs match authored placement', set(observed) == {row['id'] for row in expected})
            check('cooked LOD screen families match authored placement', {row['id'] for row in cooked_screens} == {row['id'] for row in expected})
            expected_total = sum(len(row['instances']) for row in expected)
            check('placement total is internally consistent', expected_total == placement['total_instances'])
            for row in expected:
                actual = observed.get(row['id'])
                end = math.floor(float(row['cull_metres']) * 100 + .5)
                start = math.floor(end * .82 + .5)
                check(row['id'] + ' actual count and native cull distances match placement', actual is not None and actual['instances'] == len(row['instances']) and actual['cull_cm'] == [start, end])
    status = 'FAIL' if not all(row['pass'] for row in checks) else 'PASS' if required or has_native_evidence else 'NOT_REQUESTED'
    return {'status': status,
            'required': required, 'checks': checks, 'ready': ready_rows, 'components': components, 'cooked_lod_screens': cooked_screens, 'nature_material_warnings': material_warnings,
            'limits': 'Native telemetry checks actual cooked RenderData.ScreenSize, count, cull distances, NoCollision and first/last world transforms after tree build. This verifies runtime thresholds separately from the protected editor flag. It does not prove visibility of every instance or frame rate.'}


def runtime_audit_cli():
    import argparse
    parser = argparse.ArgumentParser(description='Audit existing packaged nature log; no Unreal launch or import')
    parser.add_argument('--runtime-log', type=Path, required=True)
    parser.add_argument('--placement', type=Path, required=True)
    parser.add_argument('--runtime-report', type=Path, required=True)
    args = parser.parse_args()
    raw = args.runtime_log.read_bytes()
    placement_raw = args.placement.read_bytes()
    report = nature_runtime_evidence(raw.decode(errors='replace'), json.loads(placement_raw))
    report['runtime_log'] = {'path': str(args.runtime_log.resolve()), 'sha256': hashlib.sha256(raw).hexdigest()}
    report['placement'] = {'path': str(args.placement.resolve()), 'sha256': hashlib.sha256(placement_raw).hexdigest()}
    args.runtime_report.parent.mkdir(parents=True, exist_ok=True)
    with args.runtime_report.open('x') as stream:
        json.dump(report, stream, indent=2)
    print('NATURE RUNTIME AUDIT', report['status'], args.runtime_report)
    return 0 if report['status'] == 'PASS' else 1


def fbx_nodes(path):
    raw = Path(path).read_bytes()
    if not raw.startswith(b'Kaydara FBX Binary  \x00\x1a\x00'):
        raise ValueError('Expected binary FBX: ' + str(path))
    version = struct.unpack_from('<I', raw, 23)[0]
    wide = version >= 7500
    header = '<QQQB' if wide else '<IIIB'
    header_size = struct.calcsize(header)
    scalar = {'Y': '<h', 'C': '<?', 'I': '<i', 'F': '<f', 'D': '<d', 'L': '<q'}
    arrays = {'f': 'f', 'd': 'd', 'i': 'i', 'l': 'q', 'b': 'B', 'c': 'B'}

    def prop(offset):
        kind = chr(raw[offset])
        offset += 1
        if kind in scalar:
            fmt = scalar[kind]
            return struct.unpack_from(fmt, raw, offset)[0], offset + struct.calcsize(fmt)
        if kind in ('S', 'R'):
            length = struct.unpack_from('<I', raw, offset)[0]
            offset += 4
            end = offset + length
            if end > len(raw):
                raise ValueError('Truncated FBX string')
            value = raw[offset:end]
            return value.decode('utf-8', errors='replace') if kind == 'S' else value, end
        if kind in arrays:
            count, encoding, length = struct.unpack_from('<III', raw, offset)
            offset += 12
            if encoding not in (0, 1) or offset + length > len(raw):
                raise ValueError('Invalid FBX array storage')
            data = raw[offset:offset + length]
            if encoding:
                data = zlib.decompress(data)
            fmt = '<' + str(count) + arrays[kind]
            if len(data) != struct.calcsize(fmt):
                raise ValueError('FBX array byte count differs')
            return list(struct.unpack(fmt, data)), offset + length
        raise ValueError('Unknown FBX property type: ' + kind)

    def node(offset, parent_end):
        start = offset
        end, count, size, name_size = struct.unpack_from(header, raw, offset)
        offset += header_size
        if not end:
            return None, start + header_size
        if not start < end <= parent_end or count > 10000000:
            raise ValueError('Invalid FBX node boundary')
        name = raw[offset:offset + name_size].decode('utf-8', errors='replace')
        offset += name_size
        property_end = offset + size
        properties = []
        for _ in range(count):
            value, offset = prop(offset)
            properties.append(value)
        if offset != property_end or offset > end:
            raise ValueError('FBX property byte count differs')
        children = []
        while offset + header_size <= end:
            child, offset = node(offset, end)
            if child is None:
                break
            children.append(child)
        if offset > end:
            raise ValueError('FBX children exceed node')
        return {'name': name, 'properties': properties, 'children': children}, end

    result = []
    offset = 27
    while offset + header_size <= len(raw):
        item, offset = node(offset, len(raw))
        if item is None:
            break
        result.append(item)
    return result


def corner_colors(path):
    """RGBA per real polygon corner; supports Blender and UE binary exporters."""
    def children(item, name):
        return [child for child in item['children'] if child['name'] == name]

    def value(item, name, default=None):
        rows = children(item, name)
        return rows[0]['properties'][0] if rows else default

    roots = fbx_nodes(path)
    objects = next((row for row in roots if row['name'] == 'Objects'), None)
    if objects is None:
        raise ValueError('FBX has no Objects')
    rgba = []
    names = []
    for geom in children(objects, 'Geometry'):
        if geom['properties'][-1] != 'Mesh':
            continue
        layers = children(geom, 'LayerElementColor')
        polygon = value(geom, 'PolygonVertexIndex')
        if len(layers) != 1 or not polygon:
            raise ValueError('Expected one color layer and polygon corners per mesh: ' + str(geom['properties']))
        layer = layers[0]
        colors = value(layer, 'Colors')
        indices = value(layer, 'ColorIndex', [])
        mapping = value(layer, 'MappingInformationType')
        reference = value(layer, 'ReferenceInformationType')
        if not colors or len(colors) % 4 or mapping not in ('ByPolygonVertex', 'ByVertice', 'ByVertex', 'ByControlPoint', 'AllSame') or reference not in ('Direct', 'IndexToDirect', 'Index'):
            raise ValueError('Unsupported FBX color layer: ' + repr((mapping, reference)))
        for corner, position in enumerate(polygon):
            control = position if position >= 0 else -position - 1
            index = corner if mapping == 'ByPolygonVertex' else 0 if mapping == 'AllSame' else control
            if reference != 'Direct':
                index = indices[index]
            color = colors[index * 4:index * 4 + 4]
            if len(color) != 4 or any(not math.isfinite(channel) or not 0 <= channel <= 1.000001 for channel in color):
                raise ValueError('Invalid RGBA values')
            rgba.append(tuple(color))
        names.append(geom['properties'][1])
    if not rgba:
        raise ValueError('FBX contains no colored mesh corners')
    return rgba, names


def color_stats(rgba):
    return {'polygon_corners': len(rgba),
            'rgba_min': [min(row[i] for row in rgba) for i in range(4)],
            'rgba_max': [max(row[i] for row in rgba) for i in range(4)],
            'rgb_nonwhite_corners': sum(min(row[:3]) < .995 for row in rgba),
            'partial_alpha_corners': sum(.005 < row[3] < .995 for row in rgba),
            'unique_rgba_bytes': len({tuple(round(v * 255) for v in row) for row in rgba})}


def compare_colors(source, native):
    detail = {'source': color_stats(source), 'native_render': color_stats(native),
              'method': 'Sorted per-channel polygon-corner distributions; ignores native corner reorder, tolerates byte quantization',
              'tolerance': 2 / 255}
    if len(source) != len(native):
        return False, detail
    delta = [max(abs(a - b) for a, b in zip(sorted(row[i] for row in source), sorted(row[i] for row in native))) for i in range(4)]
    detail['max_sorted_channel_delta'] = delta
    return max(delta) <= detail['tolerance'], detail


def corner_uvs(path, channel=0):
    """UV per FBX polygon corner, not control-point/unique UV counts.

    Both source and native exporter use FBX's V convention, so no repeat,
    modulo, offset or tolerance-widening normalization is applied here.
    """
    def children(item, name):
        return [child for child in item['children'] if child['name'] == name]

    def value(item, name, default=None):
        rows = children(item, name)
        return rows[0]['properties'][0] if rows else default

    roots = fbx_nodes(path)
    objects = next((row for row in roots if row['name'] == 'Objects'), None)
    if objects is None:
        raise ValueError('FBX has no Objects')
    result, names = [], []
    for geom in children(objects, 'Geometry'):
        if geom['properties'][-1] != 'Mesh':
            continue
        layers = [row for row in children(geom, 'LayerElementUV') if row['properties'] and row['properties'][0] == channel]
        polygon = value(geom, 'PolygonVertexIndex')
        if len(layers) != 1 or not polygon:
            raise ValueError('Expected exactly one UV' + str(channel) + ' layer and polygon corners per mesh: ' + str(geom['properties']))
        layer = layers[0]
        coordinates, indices = value(layer, 'UV'), value(layer, 'UVIndex', [])
        mapping, reference = value(layer, 'MappingInformationType'), value(layer, 'ReferenceInformationType')
        if not coordinates or len(coordinates) % 2 or mapping not in ('ByPolygonVertex', 'ByVertice', 'ByVertex', 'ByControlPoint', 'AllSame') or reference not in ('Direct', 'IndexToDirect', 'Index'):
            raise ValueError('Unsupported FBX UV layer: ' + repr((mapping, reference)))
        for corner, position in enumerate(polygon):
            control = position if position >= 0 else -position - 1
            index = corner if mapping == 'ByPolygonVertex' else 0 if mapping == 'AllSame' else control
            if reference != 'Direct':
                if not 0 <= index < len(indices):
                    raise ValueError('UVIndex does not cover polygon corners/control points')
                index = indices[index]
            if not 0 <= index < len(coordinates) // 2:
                raise ValueError('FBX UV index is out of range')
            uv = tuple(coordinates[index * 2:index * 2 + 2])
            if any(not math.isfinite(number) for number in uv):
                raise ValueError('Nonfinite FBX UV coordinate')
            result.append(uv)
        names.append(geom['properties'][1])
    if not result:
        raise ValueError('FBX contains no mesh UV corners')
    return result, names


def compare_uvs(source, native):
    def stats(values):
        return {'polygon_corners': len(values), 'uv_min': [min(row[i] for row in values) for i in range(2)],
                'uv_max': [max(row[i] for row in values) for i in range(2)]}
    detail = {'source': stats(source), 'native_render': stats(native), 'tolerance_uv': 1e-4,
              'method': 'Sorted UV0 per-channel polygon-corner distributions, exact FBX UV coordinates; no wrap/offset normalization'}
    if len(source) != len(native):
        return False, detail
    delta = [max(abs(a - b) for a, b in zip(sorted(row[i] for row in source), sorted(row[i] for row in native))) for i in range(2)]
    detail['max_sorted_channel_delta_uv'] = delta
    return max(delta) <= detail['tolerance_uv'], detail


def masked_texture_evidence(M, unreal, mat, tex, row):
    """Saved native simple cutout graph and texture policy, without mutation."""
    cutoff = float(row.get('alpha_cutoff', .333))
    mask = M.get_material_property_input_node(mat, unreal.MaterialProperty.MP_OPACITY_MASK)
    output = M.get_material_property_input_node_output_name(mat, unreal.MaterialProperty.MP_OPACITY_MASK)
    sample = isinstance(mask, unreal.MaterialExpressionTextureSample)
    actual_texture = mask.get_editor_property('texture') if sample else None
    thresholds = tex.get_editor_property('alpha_coverage_thresholds')
    actual_thresholds = [float(thresholds.x), float(thresholds.y), float(thresholds.z), float(thresholds.w)]
    clip = float(mat.get_editor_property('opacity_mask_clip_value'))
    checks = [
        {'name': 'cutout uses masked blend, never translucent', 'pass': mat.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_MASKED},
        {'name': 'cutout mask directly samples correct source base texture', 'pass': sample and actual_texture == tex,
         'detail': {'expected': tex.get_path_name(), 'actual': actual_texture.get_path_name() if actual_texture else None}},
        {'name': 'cutout mask uses source texture alpha output', 'pass': output == 'A', 'detail': output},
        {'name': 'cutout clip threshold matches source', 'pass': math.isfinite(cutoff) and 0 <= cutoff <= 1 and abs(clip - cutoff) < 1e-5,
         'detail': {'expected': cutoff, 'actual': clip}},
        {'name': 'cutout compression retains alpha', 'pass': not tex.get_editor_property('compression_no_alpha')},
        {'name': 'cutout mip alpha coverage enabled', 'pass': bool(tex.get_editor_property('do_scale_mips_for_alpha_coverage'))},
        {'name': 'cutout mip coverage thresholds match source alpha', 'pass': all(abs(a-b) < 1e-5 for a,b in zip(actual_thresholds, [0,0,0,cutoff])),
         'detail': {'expected': [0,0,0,cutoff], 'actual': actual_thresholds}},
    ]
    return {'status': 'PASS' if all(item['pass'] for item in checks) else 'FAIL', 'checks': checks,
            'limits': 'Checks saved native graph and alpha/mip policy, not per-pixel residency or rendered leaf quality.'}


def auto_lod_flag_evidence(mesh):
    """Read when exposed; only UE5.8's exact protected-field refusal is waived."""
    try:
        actual = mesh.get_editor_property('bAutoComputeLODScreenSize')
    except Exception as error:
        expected = "StaticMesh: Property 'bAutoComputeLODScreenSize' for attribute 'bAutoComputeLODScreenSize' on 'StaticMesh' is protected and cannot be read"
        if str(error) != expected:
            raise
        return {'status': 'UNVERIFIED', 'native_property': 'bAutoComputeLODScreenSize',
                'reason': str(error),
                'limits': 'Protected editor-only flag is not read or inferred from screen values. Saved screen sizes are checked separately; cooked RenderData.ScreenSize requires the native runtime gate.'}
    return {'status': 'PASS' if actual is False else 'FAIL',
            'native_property': 'bAutoComputeLODScreenSize', 'actual': actual}


def verify_all():
    import unreal
    helpers = runpy.run_path(str(ROOT / 'import_assets.py'), run_name='nature_helpers')
    selected = helpers['labels']()
    output = Path(os.environ.get('S47_NATURE_VERIFY', str(ROOT / 'unreal-verification.json')))
    output.parent.mkdir(parents=True, exist_ok=True)
    render_dir = output.parent / (output.stem + '-render-fbx')
    render_dir.mkdir(parents=True, exist_ok=True)
    E, M = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary
    S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
    report = {'status': 'INCOMPLETE', 'checks': [], 'unverified': [], 'meshes': [], 'materials': [],
              'engine': unreal.SystemLibrary.get_engine_version(), 'sets': selected,
              'limits': 'Checks saved native assets, actual LOD0 render vertex colors, and ground FP32 policy/render UV0. Runtime HISM placement, cull distances, lighting and visual acceptance require packaged tests.'}

    def write():
        output.write_text(json.dumps(report, indent=2))

    def check(name, value, detail=None):
        report['checks'].append({'name': name, 'pass': bool(value), 'detail': detail})
        write()
        return bool(value)

    def ancestry(mat, root):
        result, pending, seen = [], [root] if root else [], set()
        while pending:
            node = pending.pop()
            key = node.get_path_name()
            if key in seen:
                continue
            seen.add(key)
            result.append(node)
            pending.extend(child for child in M.get_inputs_for_material_expression(mat, node) if child is not None)
        return result

    def render_export(mesh, filename):
        options = unreal.FbxExportOption()
        for key, value in {'ascii': False, 'vertex_color': True, 'level_of_detail': False,
                           'collision': False, 'export_source_mesh': False}.items():
            options.set_editor_property(key, value)
        task = unreal.AssetExportTask()
        task.object = mesh
        task.filename = str(filename)
        task.exporter = unreal.StaticMeshExporterFBX()
        task.options = options
        task.automated = True
        task.prompt = False
        task.replace_identical = True
        task.selected = False
        return unreal.Exporter.run_asset_export_task(task) and filename.is_file()

    try:
        sources = helpers['preflight'](selected)
        report['sources'] = {label: {key: value for key, value in data.items() if key in ('files', 'contract_sha256', 'materials_sha256')} for label, data in sources.items()}
        probe = E.load_asset('/Game/CalicheStreet/Verification/axis_probe')
        check('saved coordinate probe exists', isinstance(probe, unreal.StaticMesh))
        if isinstance(probe, unreal.StaticMesh):
            actual = helpers['native_bounds'](probe)
            check('saved asymmetric Blender to UE coordinate policy', max(abs(a - b) for a, b in zip(actual, [200, -400, 600, 20, 40, 60])) < .12, actual)
        for label, data in sources.items():
            src, contract, dest = data['src'], data['contract'], helpers['SETS'][label]
            rows = {row['name']: row for row in data['rows']}
            for name, row in rows.items():
                mat_path = dest + '/Materials/' + name
                mat = E.load_asset(mat_path)
                if not check(name + ' native material exists', isinstance(mat, unreal.Material), mat_path):
                    continue
                if label == 'vegetation':
                    usage = mat.get_editor_property('used_with_instanced_static_meshes')
                    check(name + ' saved InstancedStaticMeshes material usage enabled', usage is True, usage)
                base = M.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
                check(name + ' source material fingerprint', E.get_metadata_tag(mat, 'CN_SOURCE_MATERIALS_SHA256') == data['materials_sha256'])
                if row.get('vertex_rgb_tint'):
                    check(name + ' vertex RGB multiplication at base color', isinstance(base, unreal.MaterialExpressionMultiply) and base.get_editor_property('desc') == 'CN_BaseColor_times_VertexRGB')
                    inputs = list(M.get_inputs_for_material_expression(mat, base)) if base else []
                    check(name + ' base texture/tint and vertex RGB remain connected', len(inputs) == 2 and inputs[0] is not None and isinstance(inputs[1], unreal.MaterialExpressionVertexColor))
                else:
                    check(name + ' original opaque plant color path', base is not None)
                masked = bool(row.get('vertex_alpha_opacity') or row.get('alpha_mode') == 'dithered_masked')
                expected_blend = unreal.BlendMode.BLEND_TRANSLUCENT if float(row.get('alpha', 1)) < 1 else unreal.BlendMode.BLEND_MASKED if masked or (row.get('opacity_from_base_color') and row.get('alpha_mode', 'masked') == 'masked') else unreal.BlendMode.BLEND_TRANSLUCENT if row.get('opacity_from_base_color') else unreal.BlendMode.BLEND_OPAQUE
                check(name + ' expected blend mode', mat.get_editor_property('blend_mode') == expected_blend)
                check(name + ' sidedness', bool(mat.get_editor_property('two_sided')) == (bool(row.get('two_sided', False)) or float(row.get('alpha', 1)) < 1))
                if masked:
                    mask = M.get_material_property_input_node(mat, unreal.MaterialProperty.MP_OPACITY_MASK)
                    nodes = ancestry(mat, mask)
                    check(name + ' temporal perimeter function remains connected', isinstance(mask, unreal.MaterialExpressionMaterialFunctionCall) and 'DitherTemporalAA' in str(mask.get_editor_property('material_function')))
                    if row.get('vertex_alpha_opacity'):
                        check(name + ' native vertex alpha reaches perimeter mask', any(isinstance(node, unreal.MaterialExpressionVertexColor) for node in nodes))
                for key in ('metallic', 'specular'):
                    expected = float(row.get(key, .5))
                    check(name + ' saved ' + key, abs(M.get_material_default_scalar_parameter_value(mat, key.title()) - expected) < 1e-5)
                for role in row.get('textures', {}):
                    if role not in ('base_color', 'normal', 'roughness'):
                        continue
                    path = mat_path.rsplit('/', 1)[0] + '/Textures/T_' + name + '_' + role
                    tex = E.load_asset(path)
                    if not check(name + ' native ' + role + ' texture', isinstance(tex, unreal.Texture2D), path):
                        continue
                    check(name + ' ' + role + ' source texture fingerprint', E.get_metadata_tag(tex, 'CN_SOURCE_TEXTURE_SHA256') == helpers['sha'](src / row['textures'][role]))
                    check(name + ' ' + role + ' SRGB', bool(tex.get_editor_property('srgb')) == (role == 'base_color'))
                    if role == 'base_color' and row.get('opacity_from_base_color') and row.get('alpha_mode', 'masked') == 'masked' and not masked:
                        alpha_proof = masked_texture_evidence(M, unreal, mat, tex, row)
                        for item in alpha_proof['checks']:
                            check(name + ' ' + item['name'], item['pass'], item.get('detail'))
                    if role == 'normal':
                        check(name + ' DirectX normal texture settings', tex.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_NORMALMAP and not tex.get_editor_property('flip_green_channel'))
                        check(name + ' controlled normal strength', abs(M.get_material_default_scalar_parameter_value(mat, 'NormalStrength') - float(row.get('normal_strength', .2))) < 1e-5)
                    elif role == 'roughness':
                        check(name + ' linear roughness compression', tex.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_DEFAULT)
                    prop = {'base_color': unreal.MaterialProperty.MP_BASE_COLOR, 'normal': unreal.MaterialProperty.MP_NORMAL, 'roughness': unreal.MaterialProperty.MP_ROUGHNESS}[role]
                    reachable = ancestry(mat, M.get_material_property_input_node(mat, prop))
                    check(name + ' ' + role + ' texture reaches correct material property', any(isinstance(node, unreal.MaterialExpressionTextureSample) and node.get_editor_property('texture') == tex for node in reachable))
                report['materials'].append({'name': name, 'asset': mat.get_path_name()})
            for name, row in contract['parts'].items():
                path = dest + '/' + name
                mesh = E.load_asset(path)
                if not check(name + ' saved native mesh exists', isinstance(mesh, unreal.StaticMesh), path):
                    continue
                lods = helpers['levels'](name, row)
                count = S.get_lod_count(mesh)
                check(name + ' exact authored LOD count', count == len(lods), count)
                expected_bounds, actual_bounds = helpers['bounds_cm'](row), helpers['native_bounds'](mesh)
                check(name + ' scale orientation pivot bounds', max(abs(a - b) for a, b in zip(expected_bounds, actual_bounds)) < .15, {'expected': expected_bounds, 'actual': actual_bounds})
                expected_screens, actual_screens = helpers['screens'](contract, row, len(lods)), list(S.get_lod_screen_sizes(mesh))
                check(name + ' explicit saved LOD screen sizes', len(actual_screens) == len(expected_screens) and all(abs(a - b) < 1e-6 for a, b in zip(expected_screens, actual_screens)), {'expected': expected_screens, 'actual': actual_screens})
                auto_screen = auto_lod_flag_evidence(mesh)
                if auto_screen['status'] == 'UNVERIFIED':
                    report['unverified'].append(dict(auto_screen, name=name + ' automatic LOD screen computation disabled'))
                    write()
                else:
                    check(name + ' automatic LOD screen computation disabled', auto_screen['status'] == 'PASS', auto_screen)
                check(name + ' Nanite disabled for authored LODs', not mesh.get_editor_property('nanite_settings').enabled)
                check(name + ' authored vertex import policy', mesh.get_editor_property('asset_import_data').get_editor_property('vertex_color_import_option') == unreal.VertexColorImportOption.REPLACE)
                check(name + ' source contract fingerprint', E.get_metadata_tag(mesh, 'CN_SOURCE_CONTRACT_SHA256') == data['contract_sha256'])
                expected_fbx_hashes = [helpers['sha'](helpers['source_path'](src, name if index == 0 else name + '_LOD' + str(index), level)) for index, level in enumerate(lods)]
                check(name + ' authored LOD FBX fingerprints', E.get_metadata_tag(mesh, 'CN_SOURCE_LOD_FBX_SHA256') == json.dumps(expected_fbx_hashes))
                policy = helpers['cull_policy'](contract, row)
                if policy:
                    check(name + ' runtime cull policy metadata (not runtime proof)', E.get_metadata_tag(mesh, 'CN_RUNTIME_CULL_METERS') == json.dumps(policy))
                slots = list(mesh.get_editor_property('static_materials'))
                check(name + ' every slot bound to owned named material', all(slot.material_interface and slot.material_interface.get_path_name() == dest + '/Materials/' + str(slot.material_slot_name) + '.' + str(slot.material_slot_name) and str(slot.material_slot_name) in rows for slot in slots))
                for index, level in enumerate(lods[:count]):
                    triangles = mesh.get_num_triangles(index)
                    check(name + ' LOD' + str(index) + ' real triangle count', triangles == int(level['triangles']), {'expected': level['triangles'], 'actual': triangles})
                    bindings = []
                    for section in range(mesh.get_num_sections(index)):
                        material = mesh.get_material(S.get_lod_material_slot(mesh, index, section))
                        bindings.append(material.get_name() if material else None)
                    expected = helpers['used_materials'](row, level)
                    check(name + ' LOD' + str(index) + ' real section material bindings', set(bindings) == set(expected), {'expected': expected, 'actual': bindings})
                    check(name + ' LOD' + str(index) + ' authored UV channel retained', S.get_num_uv_channels(mesh, index) >= 1)
                    if label == 'ground':
                        full_precision = bool(S.get_lod_build_settings(mesh, index).get_editor_property('use_full_precision_u_vs'))
                        check(name + ' LOD' + str(index) + ' stored full-precision UV build policy', full_precision)
                needs_colors = any(rows[material].get('vertex_rgb_tint') or rows[material].get('vertex_alpha_opacity') for material in helpers['used_materials'](row))
                color_proof, uv_proof = None, None
                if needs_colors:
                    check(name + ' source mesh descriptions retain nonwhite colors', S.has_vertex_colors(mesh))
                if needs_colors or label == 'ground':
                    filename = render_dir / (name + '.fbx')
                    if check(name + ' actual native render FBX exported', render_export(mesh, filename), str(filename)):
                        source_path = helpers['source_path'](src, name, lods[0])
                        evidence = {'source_fbx': str(source_path), 'native_render_fbx': str(filename), 'source_sha256': helpers['sha'](source_path), 'native_sha256': helpers['sha'](filename)}
                        if needs_colors:
                            source_rgba, source_objects = corner_colors(source_path)
                            native_rgba, native_objects = corner_colors(filename)
                            ok, color_proof = compare_colors(source_rgba, native_rgba)
                            color_proof.update(dict(evidence, source_objects=source_objects, native_objects=native_objects))
                            check(name + ' actual render RGBA matches authored corners', ok, color_proof)
                            if any(rows[m].get('vertex_rgb_tint') for m in helpers['used_materials'](row)):
                                check(name + ' native render RGB is not uniform white', color_proof['native_render']['rgb_nonwhite_corners'] > 0)
                        if label == 'ground':
                            source_uv, source_objects = corner_uvs(source_path)
                            native_uv, native_objects = corner_uvs(filename)
                            ok, uv_proof = compare_uvs(source_uv, native_uv)
                            uv_proof.update(dict(evidence, source_objects=source_objects, native_objects=native_objects))
                            check(name + ' actual LOD0 render UV0 matches authored corners at 1e-4', ok, uv_proof)
                report['meshes'].append({'name': name, 'asset': mesh.get_path_name(), 'lod_triangles': [mesh.get_num_triangles(i) for i in range(count)], 'bounds_cm': actual_bounds, 'screens': actual_screens, 'vertex_color_proof': color_proof, 'uv0_proof': uv_proof})
                write()
        report['status'] = 'PASS' if all(row['pass'] for row in report['checks']) else 'FAIL'
        write()
        if report['status'] != 'PASS':
            raise AssertionError([row['name'] for row in report['checks'] if not row['pass']])
        unreal.log('CALICHE NATURE VERIFY PASS ' + str(len(report['checks'])) + ' readable checks; ' + str(len(report['unverified'])) + ' protected editor flags UNVERIFIED (see report)')
    except Exception:
        report['status'] = 'FAIL'
        report['error'] = traceback.format_exc()
        write()
        raise


if __name__ == '__main__':
    if '--runtime-log' in sys.argv:
        raise SystemExit(runtime_audit_cli())
    verify_all()
