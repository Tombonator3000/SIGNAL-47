"""UE 5.8 material usage for meshes drawn through the native prop/ISM path.

No graph, binding, Nanite setting or source mesh is changed. Usage is compiled
on the actual interface, including imported instances with usage overrides.
"""
import unreal

E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary


def required_usage(mesh, instanced=True):
    flags = []
    if instanced:
        flags.append(unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    if mesh.get_editor_property('nanite_settings').enabled:
        flags.append(unreal.MaterialUsage.MATUSAGE_NANITE)
    return flags


def ensure_usage(material, flags, apply=True):
    rows = []
    for flag in flags:
        before = M.has_material_usage(material, flag)
        if apply and not before:
            if isinstance(material, unreal.Material):
                M.set_base_material_usage(material, flag, True)
            elif isinstance(material, unreal.MaterialInstanceConstant):
                # Keep shared Interchange/Engine masters untouched. UE5.8 supports
                # usage permutations on the imported instance itself.
                M.set_material_usage_override(material, flag, True, True)
            else:
                raise TypeError('Unsupported material interface: ' + material.get_path_name())
            if not E.save_loaded_asset(material):
                raise RuntimeError('Could not save ' + material.get_path_name())
        rows.append({'material': material.get_path_name(), 'usage': str(flag),
                     'before': before, 'after': M.has_material_usage(material, flag)})
    return rows


def ensure_mesh_material_usage(mesh, instanced=True, apply=True):
    rows = []
    seen = set()
    for slot in mesh.get_editor_property('static_materials'):
        material = slot.material_interface
        if not material:
            raise RuntimeError('Unbound material on ' + mesh.get_path_name())
        if material.get_path_name() in seen:
            continue
        seen.add(material.get_path_name())
        rows.extend(ensure_usage(material, required_usage(mesh, instanced), apply))
    return rows
