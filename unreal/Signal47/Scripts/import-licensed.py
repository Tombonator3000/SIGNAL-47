# Imports the bought models (Vintage Liminal Props Pack, reduced to .glb by web/src/assets/licensed,
# a clone of the private repo SIGNAL-47-assets) into /Game/Licensed/Models. That folder is
# ignored by Git: the bought files never go into the public repo. Without the clone the script
# does nothing and the game falls back to the archive's models.
#   UnrealEditor-Cmd Signal47.uproject -run=pythonscript -script=Scripts/import-licensed.py -unattended -nullrhi
import os
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
root = os.path.normpath(os.path.join(unreal.Paths.project_dir(), '..', '..'))
src = os.path.join(root, 'web', 'src', 'assets', 'licensed', 'models')
if not os.path.isdir(src):
    unreal.log_warning('S47: no licensed models at ' + src + ' (clone SIGNAL-47-assets there first)')
else:
    count = 0
    for name in sorted(os.listdir(src)):
        if not name.endswith('.glb'):
            continue
        asset = os.path.splitext(name)[0]
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', os.path.join(src, name))
        task.set_editor_property('destination_path', '/Game/Licensed/Models')
        task.set_editor_property('destination_name', asset)
        task.set_editor_property('replace_existing', True)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', True)
        # glTF goes through Interchange, which keeps the baked colour maps and emissive maps.
        tools.import_asset_tasks([task])
        mesh = eal.load_asset('/Game/Licensed/Models/' + asset)
        if mesh:
            b = mesh.get_bounding_box()
            s = b.max - b.min
            unreal.log('S47: licensed %s  size %.0f x %.0f x %.0f cm' % (asset, s.x, s.y, s.z))
            count += 1
        else:
            # Interchange may import a scene as several assets; look for the first static mesh under the folder.
            found = [a for a in eal.list_assets('/Game/Licensed/Models/' + asset, recursive=True) if 'StaticMesh' in str(unreal.EditorAssetLibrary.find_asset_data(a).asset_class_path.asset_name)]
            unreal.log_warning('S47: licensed %s imported as %d assets under its own folder' % (asset, len(found)))
            count += 1
    unreal.log('S47: licensed models imported: %d' % count)
