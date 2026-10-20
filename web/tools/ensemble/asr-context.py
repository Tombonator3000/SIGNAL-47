#!/usr/bin/env python3
"""Check ambiguous short actor lines in existing dialogue context, no new TTS."""
from pathlib import Path
import hashlib
import json
import subprocess
import tempfile
from audio_common import atomic_json
from asr import checked

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'production/ensemble'
GROUPS={
 'stanton-rig-context':['22010a044849c8f3','79a44e7e5fd1e5bf','d56bdd61a80ce2fb','080a38b87176fc6c'],
 'stanton-till-context':['ea2ae90b1d91d117','625190e94de15be0','71fd7972fd78795e'],
 'stack-pin-context':['bcd48344aa27b453','39891e2464e2e381','7f7d4cb8653d87ad'],
 'stack-town-context':['c2931e01331160a6','a3d657e1a59ed461','4080f29637b3f907','16d01f724af485f2'],
 'stack-calls-context':['16b8aad01575b86b','bcd48344aa27b453'],
}

def main():
    assets=ROOT/'src/assets/audio/ensemble'
    corpus=json.loads((BASE/'corpus.json').read_text()); by_id={c['id']:c for c in corpus['clips']}
    out=BASE/'context-asr.json'; report=json.loads(out.read_text()) if out.exists() else {'model':'Systran/faster-whisper-small','human_listened':False,'samples':{}}
    for cid,ids in GROUPS.items():
        text=' '.join(by_id[i]['text'] for i in ids)
        with tempfile.TemporaryDirectory(prefix='signal47-ensemble-asr-') as temp:
            file=Path(temp)/'context.wav'
            pcm=b''.join(subprocess.check_output(['ffmpeg','-v','error','-i',str(assets/(i+'.mp3')),'-ac','1','-ar','24000','-f','f32le','-']) for i in ids)
            subprocess.run(['ffmpeg','-v','error','-y','-f','f32le','-ar','24000','-ac','1','-i','-','-c:a','pcm_s16le',str(file)],input=pcm,check=True)
            row=checked('http://127.0.0.1:3900',file,cid,text,'existing_dialogue_context',report['samples'].get(cid))
            row['source_clip_ids']=ids; row['source_audio_sha256']=[hashlib.sha256((assets/(i+'.mp3')).read_bytes()).hexdigest() for i in ids]
            report['samples'][cid]=row; atomic_json(out,report)
            print(json.dumps({'id':cid,'expected':text,'recognized':row['recognized'],'status':row['status']},ensure_ascii=False),flush=True)

if __name__=='__main__': main()
