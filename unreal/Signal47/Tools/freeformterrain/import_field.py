"""Import only T_FF_Field again (make_ground_textures.py field), as import_ground_relief.py's ground_textures() does it, with no
material or mesh touched. For a change to the flora's rules (S47Flora NoiseAt), so the soil's dark feet follow the bushes.

    UnrealEditor-Cmd Signal47.uproject -run=pythonscript -script=import_field.py   (env S47_FF_FIELD=<path to T_FF_Field.png>)
"""
import os
from pathlib import Path

import unreal

DEST, NAME, OWNER = '/Game/FreeformTerrain', 'T_FF_Field', 'S47FreeformGround_v1'
E = unreal.EditorAssetLibrary
png = Path(os.environ['S47_FF_FIELD'])
state = '%d:%d' % (png.stat().st_size, png.stat().st_mtime)
task = unreal.AssetImportTask()
task.set_editor_property('filename', str(png))
task.set_editor_property('destination_path', DEST)
task.set_editor_property('destination_name', NAME)
task.set_editor_property('replace_existing', True)
task.set_editor_property('automated', True)
task.set_editor_property('save', False)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = E.load_asset(DEST + '/' + NAME)
if texture is None:
    raise RuntimeError('Could not import ' + str(png))
texture.set_editor_property('srgb', False)
texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_BC7)
E.set_metadata_tag(texture, 'S47Source', state)
E.set_metadata_tag(texture, 'S47Owner', OWNER)
if not E.save_loaded_asset(texture):
    raise RuntimeError('Could not save ' + DEST + '/' + NAME)
unreal.log('S47: T_FF_Field imported from %s (%s)' % (png, state))
