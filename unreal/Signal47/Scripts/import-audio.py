# Imports the WAV files staged by import-audio.sh (S47_AUDIO_STAGE) into /Game/Audio/<group> (a group may
# be a folder of folders: Voice/Kinski goes to /Game/Audio/Voice/Kinski) and /Game/Licensed/Sfx. Loops
# (names the web version loops) are marked looping.
import os
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
stage = os.environ.get('S47_AUDIO_STAGE', '')
LOOPS = {'engine_idle', 'engine_drive', 'crickets', 'wind', 'radio_static', 'tv_hum', 'pressure', 'line_hush', 'printer', 'phone_ring',
         'carrier_hum', 'room_tone', 'motors', 'static_noise'}
BATCH = 200  # (the voice banks have some 1500 files a folder)
total = 0
groups = []
for dirpath, dirnames, filenames in os.walk(stage):
    dirnames.sort()
    if any(n.endswith('.wav') for n in filenames):
        groups.append(os.path.relpath(dirpath, stage))
for group in sorted(groups):
    folder = os.path.join(stage, group)
    dest = '/Game/Licensed/Sfx' if group == 'Licensed' else '/Game/Audio/' + group.replace(os.sep, '/')
    names = [n for n in sorted(os.listdir(folder)) if n.endswith('.wav')]
    n = 0
    for i in range(0, len(names), BATCH):
        tasks = []
        for name in names[i:i + BATCH]:
            task = unreal.AssetImportTask()
            task.set_editor_property('filename', os.path.join(folder, name))
            task.set_editor_property('destination_path', dest)
            task.set_editor_property('destination_name', os.path.splitext(name)[0])
            task.set_editor_property('replace_existing', True)
            task.set_editor_property('automated', True)
            task.set_editor_property('save', True)
            tasks.append(task)
        tools.import_asset_tasks(tasks)
        for t in tasks:
            asset = eal.load_asset(dest + '/' + t.get_editor_property('destination_name'))
            if asset:
                n += 1
                if t.get_editor_property('destination_name') in LOOPS:
                    asset.set_editor_property('looping', True)
                    eal.save_loaded_asset(asset)
    unreal.log_warning('S47: audio %s: %d of %d imported into %s' % (group, n, len(names), dest))
    total += n
unreal.log_warning('S47: audio imported: %d' % total)
