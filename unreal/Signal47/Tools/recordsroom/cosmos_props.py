#!/usr/bin/env python3
"""RECORDS-ROOM-LIFT-20261113: Tom's two single Cosmos props for the records room (a desk fan, a fire extinguisher) to their own
package paths under Content/Rocket (git-ignored, as the Cosmos smoke pack lies: Tools/cosmossmoke).

  python3 -I cosmos_props.py [zip dir] [content dir]
Each zip is unsafe data: only the .uasset members under <pack>/Unreal/Rocket/<dir>/ are written, each name checked, into a
new folder; nothing in them is run and no picture is opened (Cosmos licence: never fed to an AI tool, never in Git).
The editor step (import_assets.py cosmos) then gives the meshes a material of their own from the pack's three textures: the
zips name a master material and an instance that are not in them.
"""
import sys, zipfile
from pathlib import Path
ZIPS = Path(sys.argv[1] if len(sys.argv) > 1 else '/home/tom/Desktop/SIGNAL 47/Content from Cosmos/Cosmos assets')
DEST = Path(sys.argv[2] if len(sys.argv) > 2 else '/home/tom/SIGNAL-47/unreal/Signal47/Content/Rocket')
PACKS = {'RPS_DeskFan_2f42aced.zip': ('RPS_DeskFan/Unreal/Rocket/', 'RPS_DeskFan'), 'RPS_FireExtinguisher02_7d9c1907.zip': ('RPS_FireExtinguisher02/Unreal/Rocket/', 'RPS_FireExtinguisher')}
for name, (prefix, folder) in PACKS.items():
    z = ZIPS / name
    if not z.is_file():
        print('absent', z)
        continue
    out = DEST / folder
    if out.exists():
        print('there already, left as it is:', out)
        continue
    out.mkdir(parents=True)
    n = 0
    with zipfile.ZipFile(z) as f:
        for i in f.infolist():
            rel = i.filename[len(prefix):] if i.filename.startswith(prefix) else ''
            parts = rel.split('/')
            if i.is_dir() or len(parts) != 2 or parts[0] != folder or not parts[1].endswith('.uasset') or '..' in parts[1] or '/' in parts[1] or '\\' in parts[1]:
                continue
            if i.file_size > 64 * 1024 * 1024:
                raise SystemExit('too large: ' + i.filename)
            (out / parts[1]).write_bytes(f.read(i))
            n += 1
    print(folder, n, 'files ->', out)
