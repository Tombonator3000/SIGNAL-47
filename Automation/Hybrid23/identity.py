#!/usr/bin/env python3
"""Stamp an actually successful isolated build with source and player file identity."""
import hashlib
import json
from pathlib import Path
import sys

def digest(path):
    with path.open('rb') as f: return hashlib.file_digest(f,'sha256').hexdigest()

def files(root, directories):
    return {str(p.relative_to(root)):digest(p) for directory in directories
            for p in sorted((root/directory).rglob('*')) if p.is_file()}

def aggregate(items):
    return hashlib.sha256(json.dumps(items,sort_keys=True,separators=(',',':')).encode()).hexdigest()

if __name__ == '__main__':
    project, output = [Path(p).resolve() for p in sys.argv[1:]]
    if output.exists(): raise SystemExit('Refusing to replace an existing build stamp')
    build = json.loads((project.parent/'build-result.json').read_text())
    if build['result'] != 'Succeeded' or build['errors'] != 0: raise SystemExit('Build report is not successful')
    source = files(project,['Assets','ProjectSettings','Packages'])
    player_root = project.parent/'Player'
    player = files(player_root,['.'])
    if 'Hybrid23.x86_64' not in player: raise SystemExit('Standalone player missing')
    result = {'build':build,'source_sha256':aggregate(source),'source_files':source,
              'player_sha256':aggregate(player),'player_files':player,
              'preparation':json.loads((project/'preparation.json').read_text()),
              'scope':'Isolated Hybrid23 experiment; no production promotion implied.'}
    output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['source_sha256','player_sha256']}))
