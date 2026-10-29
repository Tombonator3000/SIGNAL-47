#!/usr/bin/env python3
"""Measure and ASR-check the five SC01 takes; keep human listening separate."""
from pathlib import Path
import argparse
import datetime
import hashlib
import importlib.util
import json
import math
import sys
import wave

HERE=Path(__file__).resolve().parent
if str(HERE) not in sys.path:sys.path.insert(0,str(HERE))
import text_contract
spec=importlib.util.spec_from_file_location('signal47_sc01_generation',HERE/'generate.py')
generate=importlib.util.module_from_spec(spec);spec.loader.exec_module(generate)
spec=importlib.util.spec_from_file_location('signal47_existing_asr',HERE.parents[1]/'EveningShift/voice/verify.py')
asr_tools=importlib.util.module_from_spec(spec);spec.loader.exec_module(asr_tools)
transcribe=asr_tools.transcribe
tokens=asr_tools.tokens
edit_distance=asr_tools.edit_distance


def verify(data,endpoint,no_asr=False):
    generation=json.loads((HERE/'generation.json').read_text())
    overrides=text_contract.spoken_overrides(HERE)
    previous=json.loads((HERE/'verification.json').read_text()) if (HERE/'verification.json').exists() else {'takes':{}}
    rows={};takes={};technical_failures=[];word_differences=[]
    cues=generate.cues()
    for cue in cues:
        cue_id=cue['line_id'];path=data/'takes'/f'{cue_id}.wav';item=generation.get('takes',{}).get(cue_id,{})
        digest=hashlib.sha256(path.read_bytes()).hexdigest()
        problems=text_contract.generation_problems(cue,item,digest,overrides)
        current_generation=not problems
        with wave.open(str(path),'rb') as wav:
            fmt=(wav.getnchannels(),wav.getsampwidth(),wav.getframerate(),wav.getcomptype())
            if fmt!=(1,2,48000,'NONE'):problems.append(f'wrong PCM format: {fmt}')
            frames=wav.getnframes();pcm=wav.readframes(frames)
            if len(pcm)!=frames*2:problems.append('truncated PCM')
        metrics=generate.metrics(path)
        if metrics['peak_dbfs']>-2+1e-6 or metrics['samples_at_or_above_full_scale']:problems.append('peak cap/clipping')
        if abs(metrics['nonsilent_rms_dbfs']+20)>3:problems.append('active RMS outside target +/-3dB')
        if metrics['first_2ms_peak']>.01 or metrics['last_2ms_peak']>.01:problems.append('unfaded edge')
        start,end=metrics['speech_start_seconds'],metrics['speech_end_seconds']
        if start is None or end is None or not 0<start<end<=frames/48000:problems.append('invalid measured speech interval')
        elif start<.03 or frames/48000-end<.05:problems.append('speech reaches file edge')
        old=previous['takes'].get(cue_id,{})
        if not current_generation:
            asr=None
        elif no_asr:
            asr=text_contract.cached_asr(cue,old,digest)
        else:
            reply=asr_tools.transcribe(endpoint,path)
            text=reply.get('text') or ' '.join(s.get('text','') for s in reply.get('segments',[]))
            asr={'engine':'faster-whisper via existing VoiceStudio /transcribe, accurate/en',
                 'expected_text':cue['text'],'audio_sha256':digest,
                 'text':text.strip(),'strict_word_edits':asr_tools.edit_distance(asr_tools.strict_tokens(cue['text']),asr_tools.strict_tokens(text)),
                 'normalized_word_edits':asr_tools.edit_distance(asr_tools.tokens(cue['text']),asr_tools.tokens(text)),
                 'expected_tokens':len(asr_tools.tokens(cue['text']))}
        if not asr or asr['normalized_word_edits']:word_differences.append(cue_id)
        if problems:technical_failures.append(cue_id)
        duration=frames/48000
        rows[cue_id]={'actor':cue['actor'],'voice_profile':cue['voice_profile'],'text':cue['text'],
                     'sha256':digest,'seed':item.get('request',{}).get('seed'),'duration_seconds':duration,
                     'speech_start_seconds':start,'speech_end_seconds':end,
                     'peak_dbfs':metrics['peak_dbfs'],'nonsilent_rms_dbfs':metrics['nonsilent_rms_dbfs'],
                     'integrated_lufs':item.get('audio',{}).get('integrated_lufs'),'asr':asr,'technical_problems':problems}
        if not problems:
            takes[cue_id]={'file':f'HorrorStory/{cue_id}.wav','sha256':digest,'duration_seconds':duration,
                           'speech_start_seconds':start,'speech_end_seconds':end}
        print(json.dumps({'id':cue_id,'technical_ok':not problems,'asr':asr,'problems':problems}),flush=True)
    report={'version':1,'scenario':generate.SCENARIO,'completed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'status':'PASS' if not technical_failures and not word_differences else 'FAIL',
            'technical_status':'PASS' if not technical_failures else 'FAIL',
            'word_status':'CHECKED' if not word_differences else 'ASR_DIFFERENCES_UNRESOLVED',
            'required_takes':len(cues),'present_takes':len(rows),'technical_failures':technical_failures,
            'asr_differences':word_differences,'total_seconds':sum(r['duration_seconds'] for r in rows.values()),
            'human_listened':False,'speaker_similarity':'UNVERIFIED','emotional_delivery':'UNVERIFIED','takes':rows}
    generate.atomic_json(HERE/'verification.json',report)
    generate.atomic_json(HERE/'takes.json',{'version':1,
       'note_nb':f'Kun {generate.SCENARIO}: {len(takes)} faktisk kontrollerte WAV-er. Tider og SHA er målt fra filene. Øvrige HorrorStory-opptak er ikke produsert; menneskelig lytting står åpen.',
       'format':{'sample_rate_hz':48000,'sample_format':'PCM16','channels':1},'takes':takes})
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--endpoint',default='http://127.0.0.1:3900')
    p.add_argument('--data',type=Path,default=generate.HOME_DATA);p.add_argument('--no-asr',action='store_true')
    args=p.parse_args();report=verify(args.data,args.endpoint,args.no_asr)
    print(json.dumps({k:v for k,v in report.items() if k!='takes'},indent=2))
    raise SystemExit(0 if report['status']=='PASS' else 1)
