# Imports the WAV files staged by import-audio.sh (S47_AUDIO_STAGE) into /Game/Audio/<group> and
# /Game/Licensed/Sfx. Loops (names the web version loops) are marked looping.
import os
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
stage = os.environ.get('S47_AUDIO_STAGE', '')
LOOPS = {'engine_idle', 'engine_drive', 'crickets', 'wind', 'radio_static', 'tv_hum', 'pressure', 'line_hush', 'printer', 'phone_ring',
         'carrier_hum', 'room_tone', 'motors', 'static_noise'}
total = 0
for group in sorted(os.listdir(stage)):
    folder = os.path.join(stage, group)
    if not os.path.isdir(folder):
        continue
    dest = '/Game/Licensed/Sfx' if group == 'Licensed' else '/Game/Audio/' + group
    tasks = []
    for name in sorted(os.listdir(folder)):
        if not name.endswith('.wav'):
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', os.path.join(folder, name))
        task.set_editor_property('destination_path', dest)
        task.set_editor_property('destination_name', os.path.splitext(name)[0])
        task.set_editor_property('replace_existing', True)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', True)
        tasks.append(task)
    tools.import_asset_tasks(tasks)
    n = 0
    for t in tasks:
        asset = eal.load_asset(dest + '/' + t.get_editor_property('destination_name'))
        if asset:
            n += 1
            if t.get_editor_property('destination_name') in LOOPS:
                asset.set_editor_property('looping', True)
                eal.save_loaded_asset(asset)
    unreal.log_warning('S47: audio %s: %d of %d imported into %s' % (group, n, len(tasks), dest))
    total += n
unreal.log_warning('S47: audio imported: %d' % total)
