#!/usr/bin/env python3
"""One bounded carrier-word take for documented short Serling lexeme failures.

Uses the already installed Small model on CPU for word boundaries, without a
prompt. Full WAV and generation response remain in the external resumable cache.
The published evidence contains hashes and boundaries, never private paths.
"""
from pathlib import Path
import hashlib
import json
import string
import urllib.request
from faster_whisper import WhisperModel
from audio_common import atomic_json
from audio_common import generate as render
from generate import SETTINGS, spoken_text

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'production/ensemble/serling'
ENDPOINT='http://127.0.0.1:3900'
VALUES=list('AEILMNOPQR')+['seven','eight']

def main():
    corpus=json.loads((BASE/'corpus.json').read_text())
    by_text={c['text']:c for c in corpus['clips']}
    profiles=json.loads((BASE.parent/'profiles.json').read_text())['roles']
    profile=profiles['serling']
    base=hashlib.sha256(json.dumps({'settings':SETTINGS,'bank':'serling','role':'serling',
        'profile_id':profile['profile_id'],'reference_sha256':profile['profile_reference_sha256']},sort_keys=True).encode()).hexdigest()[:16]
    overrides=json.loads((BASE/'request-overrides.json').read_text())
    corrections=json.loads((BASE/'corrections.json').read_text())
    previous=json.loads((BASE/'generation.json').read_text())['clips']
    evidence_file=BASE/'carrier-word-controls.json'
    evidence=json.loads(evidence_file.read_text()) if evidence_file.exists() else {'human_listened':False,'method':'One carrier take per targeted lexeme; no-prompt cached Small CPU word timestamps. Extract only the final word with 15 ms start padding; no time stretching. Full source WAV retained in external cache.','clips':{}}
    model=WhisperModel('Systran/faster-whisper-small',device='cpu',compute_type='int8',cpu_threads=2,local_files_only=True)
    cache_root=Path.home()/'.cache/signal47-ensemble/serling'
    for value in VALUES:
        clip=by_text[value];cid=clip['id']
        if cid in evidence['clips'] and cid in overrides and overrides[cid].get('trim_seconds'):
            print(json.dumps({'id':cid,'cached_control':True}),flush=True);continue
        text=('The letter ' if value in string.ascii_uppercase else 'The number ')+value+'.'
        request={**SETTINGS,'profile_id':profile['profile_id'],'seed':44,'text':text}
        result=render(ENDPOINT,request)
        wav=urllib.request.urlopen(ENDPOINT+'/audio/'+result['audio_path']).read()
        work=Path.home()/'.cache/signal47-ensemble/carrier-controls';work.mkdir(parents=True,exist_ok=True)
        full=work/(cid+'.wav');full.write_bytes(wav)
        segments,_=model.transcribe(str(full),language='en',beam_size=5,word_timestamps=True)
        segments=list(segments)
        words=[{'word':w.word,'start':w.start,'end':w.end} for s in segments for w in s.words]
        recognized=' '.join(s.text.strip() for s in segments)
        lexical=[w['word'].strip().strip('.,!?;:').lower() for w in words]
        expected=['the','letter' if value in string.ascii_uppercase else 'number',value.lower()]
        numeric={'seven':'7','eight':'8'}
        match=lexical[-3:]==expected or lexical[-3:]==expected[:2]+[numeric.get(value,value.lower())]
        row={'id':cid,'request':request,'uncut_wav_sha256':hashlib.sha256(wav).hexdigest(),
             'uncut_duration_seconds':result['duration'],'recognized':recognized,'words':words,
             'status':'BOUNDARY_IDENTIFIED' if match else 'UNRESOLVED','human_word_check':'UNVERIFIED'}
        evidence['clips'][cid]=row;atomic_json(evidence_file,evidence)
        if not match:
            print(json.dumps({'id':cid,'status':'UNRESOLVED','recognized':recognized}),flush=True);continue
        trim=[round(max(0,words[-1]['start']-.015),4),round(result['duration'],4)]
        override={'seed':44,'text':text,'rendered_text':value+'.','trim_seconds':trim,
          'reason':'One bounded carrier take after short isolated-letter/digit ASR failure. Cached no-prompt word timestamps locate the final lexeme; only that lexeme remains in runtime. Same manuscript value, ID and profile.'}
        overrides[cid]=override
        identity=hashlib.sha256((base+'\0'+spoken_text(clip,'serling',corpus)+'\0'+json.dumps(override,sort_keys=True)).encode()).hexdigest()[:16]
        cache=cache_root/identity;cache.mkdir(parents=True,exist_ok=True)
        (cache/(cid+'.wav')).write_bytes(wav);atomic_json(cache/(cid+'.result.json'),result)
        corrections['attempts'].append({'id':cid,'before_audio_sha256':previous[cid]['audio']['sha256'],
          'before_request_text':previous[cid]['request']['text'],'corrected_request_text':text,
          'seed':44,'trim_seconds':trim,'rendered_text':value+'.','uncut_wav_sha256':row['uncut_wav_sha256'],
          'reason':override['reason']})
        row['trim_seconds']=trim;row['cache_identity']=identity
        atomic_json(evidence_file,evidence);atomic_json(BASE/'request-overrides.json',overrides);atomic_json(BASE/'corrections.json',corrections)
        print(json.dumps({'id':cid,'status':row['status'],'recognized':recognized,'trim':trim}),flush=True)

if __name__=='__main__':main()
