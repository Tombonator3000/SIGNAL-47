"""Import the private normalized Tripo skeletal mesh, preserving its real skin."""
from pathlib import Path
import json
import os
import unreal


def main():
    root = Path(os.environ.get('S47_TRIPO_CREATURE_DIR', str(Path.home() / 'Documents/ChatGPT/Signal 47/modeller/tripo-npc-2026-10-29/unreal-creature-2026-10-31')))
    data = json.loads((root / 'manifest.json').read_text())
    task = unreal.AssetImportTask()
    task.filename = data['fbx']
    task.destination_path = '/Game/Licensed/TripoNPC/Animated'
    task.destination_name = data['name']
    task.automated = True
    task.replace_existing = True
    task.save = True
    opts = unreal.FbxImportUI()
    opts.import_mesh = True
    opts.import_as_skeletal = True
    opts.import_animations = False
    opts.import_materials = False
    opts.import_textures = False
    opts.create_physics_asset = False
    opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    opts.automated_import_should_detect_type = False
    opts.skeletal_mesh_import_data.import_uniform_scale = 1.0
    # Keep the importer default reference pose; this UE build does not expose
    # the optional bUseT0AsRefPose setter to Python.
    task.options = opts
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    skeletal = [unreal.load_asset(p) for p in task.imported_object_paths]
    mesh = next((a for a in skeletal if isinstance(a, unreal.SkeletalMesh)), None)
    if not mesh:
        raise RuntimeError('No skeletal mesh imported: ' + str(task.imported_object_paths))
    material = unreal.load_asset(data['material'])
    if not material:
        raise RuntimeError('Existing Tall One material missing: ' + data['material'])
    material.set_editor_property('used_with_skeletal_mesh', True)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    materials = mesh.get_editor_property('materials')
    for entry in materials:
        entry.material_interface = material
    mesh.set_editor_property('materials', materials)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False):
        raise RuntimeError('Could not save the skinned creature')
    data['imported_mesh'] = mesh.get_path_name()
    (root / 'imported.json').write_text(json.dumps(data, indent=2) + '\n')
    unreal.log('TRIPO SKINNED IMPORT COMPLETE ' + mesh.get_path_name())


if __name__ == '__main__':
    main()
