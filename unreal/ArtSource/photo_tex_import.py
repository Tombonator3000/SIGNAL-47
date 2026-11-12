"""Texture-only re-import of the surfaces photo_tex.py regenerates (TEXTURES-PHOTO-20261112): each PNG goes over the texture
asset its pipeline's importer made (same asset, so materials and meshes are untouched), keeping its sRGB / compression /
green-flip settings, with the surface's largest size. Runs in the editor's python:
  python3 run_photo_tex_import.py saro <dir with the Lived textures (lived_tex.py's output)>
Report: Saved/photo-tex-import.json."""
from pathlib import Path
import json, re
import unreal
ART = Path(__file__).resolve().parent
cl = unreal.SystemLibrary.get_command_line()
SET = re.search(r'-S47PhotoSet=(\w+)', cl).group(1)
LIVED = Path(re.search(r'-S47PhotoLived=(\S+)', cl).group(1))
EW = ART / 'EastWing/Props/Textures'
OFFICE = ART / 'Office/textures'
ROLES = ('base_color', 'roughness', 'normal')


def rows(dest, name, files, size):
    return [(dest + '/T_' + name + '_' + role, str(f), role, size) for role, f in zip(ROLES, files)]


def ew(key):
    return [EW / (key + '_' + c + '.png') for c in ('BaseColor', 'Roughness', 'NormalDX')]


def office(key):
    return [OFFICE / (key + s + '.png') for s in ('_base', '_rough', '_normal_dx')]


SETS = {'saro': (
    # walls and floors walked close to: 2048; small or far surfaces: 1024 (Tom: memory use sane)
    rows('/Game/Lived/Saro/Materials/Textures', 'M_Lived_BlockWing', [LIVED / ('T_Lived_Block_' + c + '.png') for c in ('BC', 'R', 'N')], 2048)
    + rows('/Game/EastWing/Props/Surfaces/Textures', 'M_EW_CinderblockCream', ew('EW_CinderblockCream'), 2048)
    + rows('/Game/EastWing/Props/Surfaces/Textures', 'M_EW_Concrete', ew('EW_Concrete'), 2048)
    + rows('/Game/EastWing/Props/Surfaces/Textures', 'M_EW_GreenSteel', ew('EW_GreenSteel'), 1024)
    + rows('/Game/EastWing/Props/Surfaces/Textures', 'M_EW_Galvanized', ew('EW_Galvanized'), 1024)
    + rows('/Game/EastWing/Props/Workshop/Materials/Textures', 'M_EW_GreenSteel', ew('EW_GreenSteel'), 1024)
    + rows('/Game/EastWing/Props/Workshop/Materials/Textures', 'M_EW_Galvanized', ew('EW_Galvanized'), 1024)
    + rows('/Game/Signal47/Art/Office/Materials/Textures', 'M_Office_SageWall', office('M_Office_SageWall'), 1024)
    + rows('/Game/Signal47/Art/Office/Materials/Textures', 'M_Office_CeilingTile', office('M_Office_CeilingTile'), 1024)
    + rows('/Game/Signal47/Art/Office/Materials/Textures', 'M_Office_FloorLinoleum', office('M_Office_FloorLinoleum'), 1024))}

E_ = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
report = {'status': 'RUNNING', 'set': SET, 'textures': []}
REPORT = Path(unreal.Paths.project_saved_dir()) / 'photo-tex-import.json'
KEEP = ('srgb', 'compression_settings', 'flip_green_channel', 'lod_group', 'mip_gen_settings')
bad = 0
for asset, src, role, size in SETS[SET]:
    row = {'asset': asset, 'source': src, 'max': size, 'pass': False}
    report['textures'].append(row)
    if not Path(src).is_file():
        row['why'] = 'no source file'; bad += 1; continue
    if not E_.does_asset_exist(asset):
        row['why'] = 'no such texture asset: nothing to replace (not made new here)'; bad += 1; continue
    old = E_.load_asset(asset)
    keep = {k: old.get_editor_property(k) for k in KEEP}
    task = unreal.AssetImportTask()
    task.filename = src; task.destination_path = asset.rsplit('/', 1)[0]; task.destination_name = asset.rsplit('/', 1)[1]
    task.replace_existing = True; task.automated = True; task.save = False
    T.import_asset_tasks([task])
    tex = E_.load_asset(asset)
    for k, v in keep.items():
        tex.set_editor_property(k, v)
    tex.set_editor_property('max_texture_size', size)
    E_.save_loaded_asset(tex)
    row.update({'pass': True, 'srgb': bool(keep['srgb']), 'compression': str(keep['compression_settings']),
                'source_size': [tex.blueprint_get_size_x(), tex.blueprint_get_size_y()]})
    unreal.log('S47 PHOTOTEX ' + asset + ' <- ' + Path(src).name + ' max %d' % size)
report['status'] = 'PASS' if bad == 0 else 'FAIL'
REPORT.write_text(json.dumps(report, indent=1))
unreal.log('S47 PHOTOTEX %s %d textures, %d failed' % (report['status'], len(report['textures']), bad))
