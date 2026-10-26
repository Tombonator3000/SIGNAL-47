"""Read-only UE5.8 inspection of PRIVATE staged copies. No process launch or save.

Collect real source geometry/bounds, registry dependencies and graph wiring before
replacing vendor material graphs. The owner schedules this through unreal_guard.
"""
from pathlib import Path
import json
import os
import traceback
import unreal

HERE = Path(__file__).resolve().parent
WORK = Path(os.environ.get('S47_RN_WORK', str(Path.home() / 'scratch-bm/Signal47-terrain-20261026/assets')))
CONFIG = json.loads((HERE / 'selection.json').read_text())
E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary
REG = unreal.AssetRegistryHelpers.get_asset_registry()

def prop(obj, name):
    try:
        value = obj.get_editor_property(name)
        if hasattr(value, 'get_path_name'):
            return value.get_path_name()
        if isinstance(value, (bool, int, float, str)):
            return value
        return str(value)
    except Exception:
        return None

def graph(mat):
    nodes = list(M.get_material_expressions(mat))
    ids = {node.get_path_name(): i for i, node in enumerate(nodes)}
    rows = []
    for node in nodes:
        inputs = []
        for inp in M.get_inputs_for_material_expression(mat, node):
            if inp:
                inputs.append({'node': ids.get(inp.get_path_name(), inp.get_path_name()),
                               'output': str(M.get_input_node_output_name_for_material_expression(node, inp))})
        row = {'id': ids[node.get_path_name()], 'class': node.get_class().get_name(), 'inputs': inputs}
        for name in ('parameter_name', 'texture', 'material_function', 'r', 'g', 'b', 'a', 'default_value', 'constant'):
            value = prop(node, name)
            if value is not None:
                row[name] = value
        rows.append(row)
    outputs = {}
    for name in ('MP_BASE_COLOR', 'MP_ROUGHNESS', 'MP_NORMAL', 'MP_OPACITY_MASK', 'MP_MATERIAL_ATTRIBUTES'):
        try:
            node = M.get_material_property_input_node(mat, getattr(unreal.MaterialProperty, name))
            outputs[name] = {'node': ids.get(node.get_path_name(), node.get_path_name()),
                             'output': M.get_material_property_input_node_output_name(mat, getattr(unreal.MaterialProperty, name))} if node else None
        except Exception as error:
            outputs[name] = {'unavailable': str(error)}
    return {'nodes': rows, 'outputs': outputs}

def material_info(material):
    row = {'path': material.get_path_name(), 'class': material.get_class().get_name()}
    if isinstance(material, unreal.MaterialInstanceConstant):
        row['parent'] = prop(material, 'parent')
        row['textures'] = {str(name): M.get_material_instance_texture_parameter_value(material, name).get_path_name()
                           for name in M.get_texture_parameter_names(material)
                           if M.get_material_instance_texture_parameter_value(material, name)}
    elif isinstance(material, unreal.Material):
        row['graph'] = graph(material)
    return row

def main():
    REG.scan_paths_synchronous(['/Game/Northwood', '/Game/JustBStudios'], True)
    options = unreal.AssetRegistryDependencyOptions()
    options.set_editor_property('include_hard_package_references', True)
    options.set_editor_property('include_soft_package_references', True)
    report = {'status': 'RUNNING', 'meshes': [], 'materials': {}, 'dependencies': {},
              'engine': unreal.SystemLibrary.get_engine_version(), 'mutations': False}
    seen = set()
    queue = [row['source_package'] for row in CONFIG['meshes']]
    while queue:
        package = queue.pop()
        if package in seen:
            continue
        seen.add(package)
        deps = [str(x) for x in REG.get_dependencies(package, options)]
        report['dependencies'][package] = deps
        queue.extend(dep for dep in deps if dep.startswith('/Game/'))
    for row in CONFIG['meshes']:
        mesh = E.load_asset(row['source_package'])
        assert isinstance(mesh, unreal.StaticMesh), row['source_package']
        bounds = mesh.get_bounding_box()
        desc = mesh.get_static_mesh_description(0)
        data = {'name': row['name'], 'source': row['source_package'],
                'bounds_cm': [[bounds.min.x, bounds.min.y, bounds.min.z], [bounds.max.x, bounds.max.y, bounds.max.z]],
                'lod_triangles': [mesh.get_num_triangles(i) for i in range(mesh.get_num_lods())],
                'source_triangles': desc.get_triangle_count(), 'source_vertices': desc.get_vertex_count(),
                'nanite_enabled': mesh.get_editor_property('nanite_settings').enabled,
                'materials': []}
        for slot in mesh.get_editor_property('static_materials'):
            mat = slot.material_interface
            assert mat, (row['name'], 'Missing original material')
            data['materials'].append({'slot': str(slot.material_slot_name), 'material': mat.get_path_name()})
            current = mat
            while current and current.get_path_name() not in report['materials']:
                report['materials'][current.get_path_name()] = material_info(current)
                current = current.get_editor_property('parent') if isinstance(current, unreal.MaterialInstanceConstant) else None
        report['meshes'].append(data)
    report['status'] = 'PASS_READ_ONLY_NATIVE_INSPECTION'
    WORK.mkdir(parents=True, exist_ok=True)
    (WORK / 'native-source-inspection.json').write_text(json.dumps(report, indent=2) + '\n')
    print('S47 ROUTENATURE native source inspection PASS')

try:
    main()
except Exception:
    WORK.mkdir(parents=True, exist_ok=True)
    (WORK / 'native-source-inspection-error.txt').write_text(traceback.format_exc())
    raise
