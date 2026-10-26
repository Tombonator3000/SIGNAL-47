#!/usr/bin/env python3
"""Validate and import measured SC01 takes into the isolated horror voice bank.

The committed catalog contains no audio. --import-raw changes only ignored
Content/Audio/Raw and preserves every unrelated bank structurally unchanged.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import os
import shutil
import tempfile
import wave

HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('signal47_sc01_catalog_generation',HERE/'generate.py')
generate=importlib.util.module_from_spec(spec);spec.loader.exec_module(generate)


def catalog(data):
    takes=json.loads((HERE/'takes.json').read_text());report=json.loads((HERE/'verification.json').read_text())
    if report.get('technical_status')!='PASS':raise ValueError('Technical recording validation has not passed')
    required=set(generate.CUE_IDS)
    if set(takes['takes'])!=required:raise ValueError('Partial or unexpected SC01 timing set')
    clips=[];lookup={}
    for cue in generate.cues():
        cue_id=cue['line_id'];meta=takes['takes'][cue_id];path=data/'takes'/f'{cue_id}.wav'
        with wave.open(str(path),'rb') as wav:
            if (wav.getnchannels(),wav.getsampwidth(),wav.getframerate(),wav.getcomptype())!=(1,2,48000,'NONE'):
                raise ValueError(f'Invalid recording format: {cue_id}')
            frames=wav.getnframes()
            if len(wav.readframes(frames))!=frames*2:raise ValueError(f'Truncated recording: {cue_id}')
        digest=hashlib.sha256(path.read_bytes()).hexdigest();duration=frames/48000
        if digest!=meta['sha256'] or abs(meta['duration_seconds']-duration)>1/48000:
            raise ValueError(f'Stale measured recording: {cue_id}')
        start,end=meta['speech_start_seconds'],meta['speech_end_seconds']
        if not 0<=start<end<=duration:raise ValueError(f'Invalid timing: {cue_id}')
        key=(cue['voice_profile'],' '.join(cue['text'].split()))
        if key in lookup:raise ValueError(f'Role/text lookup collision: {cue_id}')
        lookup[key]=cue_id
        clips.append({'id':cue_id,'text':cue['text'],'voice':cue['voice_profile'],
                      'file':f'{cue_id}.wav','asset':f'v_{cue_id}',
                      'raw':f'voice/horror/{cue_id}.wav','duration':duration,'frames':frames,'sha256':digest,
                      'speech_start_seconds':start,'speech_end_seconds':end,'reuse_of':None})
    return {'horror':{'folder':'/Game/Audio/Voice/Horror','catalog':{'clips':clips},
          'validation':{'pack_id':'signal47_horror_story_h1','scenario':'SC01','cue_count':5,
                        'sample_rate_hz':48000,'sample_format':'PCM16','channels':1,
                        'technical_status':report['technical_status'],'word_status':report['word_status'],
                        'human_listened':False,'scope':'Five measured SC01 takes only; later story cues remain pending.'}}}


def import_raw(entry,data,target):
    index=target/'voices.json';before=json.loads(index.read_text()) if index.exists() else {}
    for clip in entry['horror']['catalog']['clips']:
        destination=target/clip['raw'];destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(data/'takes'/clip['file'],destination)
        if hashlib.sha256(destination.read_bytes()).hexdigest()!=clip['sha256']:
            raise ValueError(f'Imported recording hash mismatch: {clip["id"]}')
    after={**before,**entry};target.mkdir(parents=True,exist_ok=True)
    with tempfile.NamedTemporaryFile(mode='w',encoding='utf-8',dir=target,prefix='.voices-horror-',delete=False) as stream:
        json.dump(after,stream,ensure_ascii=False,indent=1,allow_nan=False);stream.write('\n');temporary=Path(stream.name)
    os.replace(temporary,index)
    actual=json.loads(index.read_text())
    if any(actual[k]!=v for k,v in before.items() if k!='horror'):
        raise ValueError('Unrelated voice bank changed')
    return len(entry['horror']['catalog']['clips'])


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--data',type=Path,default=generate.HOME_DATA)
    p.add_argument('--output',type=Path,default=HERE/'catalog.json');p.add_argument('--check',action='store_true')
    p.add_argument('--import-raw',type=Path);a=p.parse_args();entry=catalog(a.data)
    text=json.dumps(entry,ensure_ascii=False,indent=2,allow_nan=False)+'\n'
    if a.check:
        if a.output.read_text()!=text:raise ValueError('Catalog differs from measured inputs')
    else:a.output.write_text(text)
    if a.import_raw:print('Imported recordings:',import_raw(entry,a.data,a.import_raw))
    print('PASS: five exact SC01 cue identities, PCM frames, hashes and speech timings')
