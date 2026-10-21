#!/usr/bin/env python3
"""Lag den etterspurte prøvefilen utenfor repoet, uten å endre bankklippene."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
OUT=Path(__file__).resolve().parent
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/ensemble'))
from audio_common import atomic_json,metrics

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    target=Path(sys.argv[1]);target.parent.mkdir(parents=True,exist_ok=True)
    assets=ROOT/'src/assets/audio/serling-intro'
    catalog=json.loads((assets/'catalog.json').read_text())
    before={c['id']:sha(assets/c['file']) for c in catalog['clips']}
    pcm=b''.join(subprocess.check_output(['ffmpeg','-v','error','-i',str(assets/c['file']),'-ac','1','-ar','24000','-f','f32le','-']) for c in catalog['clips'])
    subprocess.run(['ffmpeg','-v','error','-y','-f','f32le','-ar','24000','-ac','1','-i','-',
        '-c:a','libmp3lame','-b:a','64k','-map_metadata','-1',str(target)],input=pcm,check=True)
    assert before=={c['id']:sha(assets/c['file']) for c in catalog['clips']}
    audio=metrics(target)
    assert audio['samples_at_or_above_full_scale']==0 and audio['peak_dbfs']<=-2
    assert abs(audio['duration_seconds']-sum(c['duration'] for c in catalog['clips']))<.001
    report={'file':target.name,'sha256':sha(target),'bytes':target.stat().st_size,**audio,
        'source_clips':[{'id':c['id'],'sha256':before[c['id']]} for c in catalog['clips']],
        'source_bank_unchanged':True,'extra_pause_seconds':0,
        'method':'Exact decoded bank order, preserving existing natural pauses. One MP3 encode for listening only; no gain,tempo or pitch changes.',
        'human_listened':False,'speaker_similarity':'UNVERIFIED'}
    atomic_json(OUT/'preview.json',report)
    atomic_json(target.with_suffix('.json'),report)
    print(json.dumps(report),flush=True)

if __name__=='__main__':main()
