"""Import only the owned cloud atlases and three native density volumes.

Root runs Unreal through unreal_guard; S47_CLOUD_ATLAS names atlas.json.
No process is launched here. No material, shared asset or directory-wide save.
"""
import json
import math
import os
from pathlib import Path
import struct

import unreal


DEST = '/Game/FinaleSky/Clouds'
AUTHORSHIP = 'Original SIGNAL / 47 Mantaflow density'
LETTERS = 'ABC'


def log(message):
    unreal.log('S47 CLOUD VOLUMES ' + message)
    print('S47 CLOUD VOLUMES ' + message, flush=True)


def source_manifest(path):
    data = json.loads(path.read_text())
    assert data.get('schema') == 1 and data.get('authorship') == AUTHORSHIP, 'Own schema-1 atlas required'
    dimensions = data.get('dimensions_xyz',[])
    assert len(dimensions) == 3 and dimensions[0] in (128,256) and dimensions == [dimensions[0]]*3
    size,bit_depth = dimensions[0],data.get('bit_depth')
    assert data.get('tiles_xy') == [16,size//16]
    assert data.get('png_size_xy') == [16*size,(size//16)*size]
    assert data.get('linear') is True and data.get('channels') == 1 and bit_depth in (8,16)
    scale = float(data['density_scale'])
    assert math.isfinite(scale) and scale > 0
    frames = data['frames']
    assert 1 <= len(frames) <= 3, 'Expected one to three selected density frames'
    for row in frames:
        png = (path.parent/row['png']).resolve()
        assert png.parent == path.parent and png.is_file(), 'Expected a PNG beside atlas.json'
        with png.open('rb') as stream:
            header = stream.read(33)
        assert header[:8] == b'\x89PNG\r\n\x1a\n' and header[12:16] == b'IHDR'
        width,height,depth,color,compression,filtering,interlace = struct.unpack('>IIBBBBB',header[16:29])
        assert (width,height,depth,color,compression,filtering,interlace) == (16*size,(size//16)*size,bit_depth,0,0,0,0)
        row['_png'] = png
    return data


def settings(texture, volume=False, bit_depth=8):
    compression = (unreal.TextureCompressionSettings.TC_HALF_FLOAT if bit_depth == 16
                   else unreal.TextureCompressionSettings.TC_GRAYSCALE)
    required = dict(srgb=False, compression_settings=compression,
                    mip_gen_settings=unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS,
                    lod_bias=0, max_texture_size=0, num_cinematic_mip_levels=0,
                    filter=unreal.TextureFilter.TF_BILINEAR,
                    power_of_two_mode=unreal.TexturePowerOfTwoSetting.NONE)
    if volume:
        # UVolumeTexture::AddressMode controls X, Y and Z together in UE5.8.
        required['address_mode'] = unreal.TextureAddress.TA_CLAMP
    else:
        required.update(address_x=unreal.TextureAddress.TA_CLAMP, address_y=unreal.TextureAddress.TA_CLAMP)
    for key,value in required.items():
        texture.set_editor_property(key,value)
        assert texture.get_editor_property(key) == value, (texture.get_name(),key)
    for key,value in (('never_stream',True),('virtual_texture_streaming',False),('defer_compression',False)):
        try:
            texture.get_editor_property(key)
        except Exception:
            log('%s optional property unavailable: %s' % (texture.get_name(),key))
        else:
            texture.set_editor_property(key,value)
            assert texture.get_editor_property(key) == value


def save_and_size(asset, expected, expected_format='G8'):
    assert asset.get_path_name().startswith(DEST+'/'), 'Refusing unowned save'
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False), 'Asset save failed: '+asset.get_path_name()
    # In editor this reports the configured target build size, not the live GPU resource.
    size = asset.blueprint_get_built_texture_size()
    actual = [int(size.x),int(size.y),int(size.z)]
    assert actual == expected, (asset.get_path_name(),'actual built dimensions',actual,expected)
    tags = {str(k):str(v) for k,v in unreal.EditorAssetLibrary.get_tag_values(asset.get_path_name()).items()}
    reported_format = tags.get('Format','')
    pixel_format = reported_format.strip()
    if pixel_format.casefold() in ('','none','unknown','pf_unknown','unavailable'):
        pixel_format = 'unavailable (registry=%r; expected configuration=%s; RHI unverified)' % (reported_format,expected_format)
    # Registry tags can still describe an intermediate/default resource after reimport.
    # The existing runtime readback checks TextureRHI format/size on the rendered frame.
    log('saved=1 asset=%s configured_size=%s registry_format=%s requested_format=%s; RHI unverified' %
        (asset.get_path_name(),actual,pixel_format,expected_format))
    return actual


def main():
    path = Path(os.environ['S47_CLOUD_ATLAS']).expanduser().resolve()
    data = source_manifest(path)
    size,bit_depth = data['dimensions_xyz'][0],data['bit_depth']
    expected_format = 'R16F' if bit_depth == 16 else 'G8'
    editor,tools = unreal.EditorAssetLibrary,unreal.AssetToolsHelpers.get_asset_tools()
    imported = []
    for index,letter in enumerate(LETTERS):
        # The one-frame pilot also fills all three shader slots; production has
        # three distinct frames. Two-frame sources hold their last frame in C.
        row = data['frames'][min(index,len(data['frames'])-1)]
        atlas_name,volume_name = 'T_S47_CloudDensityAtlas'+letter,'V_S47_CloudDensity'+letter
        atlas_path,volume_path = DEST+'/'+atlas_name,DEST+'/'+volume_name
        existing = editor.load_asset(atlas_path) if editor.does_asset_exist(atlas_path) else None
        assert existing is None or isinstance(existing,unreal.Texture2D), 'Wrong class at owned atlas path'
        task = unreal.AssetImportTask()
        task.filename = str(row['_png'])
        task.destination_path,task.destination_name = DEST,atlas_name
        task.automated,task.replace_existing,task.replace_existing_settings,task.save = True,True,True,False
        task.factory = unreal.TextureFactory()
        tools.import_asset_tasks([task])
        atlas = editor.load_asset(atlas_path)
        assert isinstance(atlas,unreal.Texture2D), 'PNG did not import as Texture2D'
        settings(atlas,bit_depth=bit_depth)
        save_and_size(atlas,data['png_size_xy']+[1],expected_format)

        volume = editor.load_asset(volume_path) if editor.does_asset_exist(volume_path) else None
        if volume is None:
            volume = tools.create_asset(volume_name,DEST,unreal.VolumeTexture,unreal.VolumeTextureFactory())
        assert isinstance(volume,unreal.VolumeTexture), 'Wrong class at owned volume path'
        settings(volume,volume=True,bit_depth=bit_depth)
        # Setting Source2DTexture invokes SetDefaultSource2DTileSize, whose square
        # atlas guess is wrong for 16x8. Always set sizes AFTER the source.
        volume.set_editor_property('source2d_texture',atlas)
        volume.set_editor_property('source2d_tile_size_x',size)
        volume.set_editor_property('source2d_tile_size_y',size)
        assert volume.get_editor_property('source2d_texture') == atlas
        assert volume.get_editor_property('source2d_tile_size_x') == size
        assert volume.get_editor_property('source2d_tile_size_y') == size
        assert volume.get_editor_property('srgb') is False
        editor.set_metadata_tag(volume, 'S47DensityScale', str(data['density_scale']))
        dims = save_and_size(volume,[size,size,size],expected_format)
        imported.append(dict(volume=volume.get_path_name(),frame=row['frame'],dimensions=dims))
    log('IMPORT COMPLETE density_scale=%.9g frames=%s; saved source/native assets only, RHI unverified' %
        (data['density_scale'],json.dumps(imported,separators=(',',':'))))


if __name__ == '__main__':
    main()
