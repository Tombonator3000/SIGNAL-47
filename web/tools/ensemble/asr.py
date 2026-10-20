#!/usr/bin/env python3
"""Serial cached word checks with the already installed Whisper Small model.

This is not a listening test. Differences require explicit review; the script
never silently accepts empty or materially different speech.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import tempfile
from audio_common import atomic_json, transcribe, tokens, edit_distance

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'production/ensemble'
ONES='zero one two three four five six seven eight nine ten eleven twelve thirteen fourteen fifteen sixteen seventeen eighteen nineteen'.split()
TENS='zero ten twenty thirty forty fifty sixty seventy eighty ninety'.split()

def cardinal(n):
    if n<20: return ONES[n]
    if n<100: return TENS[n//10]+(' '+ONES[n%10] if n%10 else '')
    if n<1000: return ONES[n//100]+' hundred'+(' '+cardinal(n%100) if n%100 else '')
    if n<1000000: return cardinal(n//1000)+' thousand'+(' '+cardinal(n%1000) if n%1000 else '')
    return ' '.join(ONES[int(x)] for x in str(n))

def equivalent_tokens(text):
    # Numeric spelling and apostrophe typography are explicit ASR presentation
    # differences. Abbreviations, missing words and names are not discarded.
    def number(m):
        raw=m.group(0)
        return ' '.join(ONES[int(c)] for c in raw) if len(raw)>1 and raw.startswith('0') else cardinal(int(raw))
    text=re.sub(r'(?<![\w])[-−](?=\d)','minus ',text.lower())
    text=re.sub(r'(?<![\w])\d+(?![\w])',number,text)
    return tokens(text.replace("'",'').replace('’',''))

def selection(corpus,bank):
    if bank=='ensemble': return corpus['clips']
    selected={}
    by_text={c['text']:c for c in corpus['clips']}
    for card in corpus['chapterCards']:
        for values in card['fields'].values():
            for value in values:
                c=by_text[' '.join(value.split())]; selected[c['id']]=c
    long=[c for c in corpus['clips'] if len(tokens(c['text']))>=25]
    for i in range(min(32,len(long))):
        c=long[round(i*(len(long)-1)/max(1,min(32,len(long))-1))]; selected[c['id']]=c
    for c in sorted(corpus['clips'],key=lambda c:len(tokens(c['text'])),reverse=True)[:12]: selected[c['id']]=c
    # Every finite technical return log must retain its date/time/frequency,
    # negative distance and any reclassification text, not just one example.
    for c in corpus['clips']:
        if c['text'].startswith('DATE ') and 'FREQ ' in c['text']: selected[c['id']]=c
    # One example of each source trigger across the actual static/dynamic prose.
    triggers=set()
    for c in corpus['clips']:
        for source in c['sources']:
            key=(source.get('kind'),source.get('trigger'))
            if key not in triggers and len(tokens(c['text']))>=6:
                selected[c['id']]=c; triggers.add(key)
    return list(selected.values())

def checked(endpoint,file,cid,expected,group,previous):
    sha=hashlib.sha256(file.read_bytes()).hexdigest()
    if previous and previous.get('audio_sha256')==sha and previous.get('expected')==expected: return previous
    result=transcribe(endpoint,file)
    recognized=result.get('text','')
    strict=edit_distance(tokens(expected),tokens(recognized))
    normalized=edit_distance(equivalent_tokens(expected),equivalent_tokens(recognized))
    required_signs=len(re.findall(r'(?<![\w])[-−]\d|\bminus\b',expected,re.I))
    recognized_signs=len(re.findall(r'\b(?:minus|negative)\b|(?<![\w])[-−]\d',recognized,re.I))
    status='PASS' if not strict else ('REVIEWED_DIFFERENCE' if not normalized else 'REVIEW_REQUIRED')
    if required_signs>recognized_signs: status='REVIEW_REQUIRED'
    if not tokens(recognized): status='FAIL_EMPTY'
    return {'id':cid,'group':group,'expected':expected,'recognized':recognized,'strict_word_edits':strict,'expected_words':len(tokens(expected)),'normalized_word_edits':normalized,'required_negative_signs':required_signs,'recognized_negative_signs':recognized_signs,'audio_sha256':sha,'status':status,'note':('Number spelling/apostrophe presentation only.' if strict and not normalized else ''),'response':result}

def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--bank',choices=['ensemble','serling'],required=True); parser.add_argument('--compositions',action='store_true'); parser.add_argument('--available-only',action='store_true',help='Early sample of finished files; never marks a missing final selection complete'); parser.add_argument('--endpoint',default='http://127.0.0.1:3900'); parser.add_argument('--limit',type=int,default=0); args=parser.parse_args()
    production=BASE if args.bank=='ensemble' else BASE/'serling'; assets=ROOT/'src/assets/audio'/args.bank
    corpus=json.loads((production/'corpus.json').read_text()); out=production/'asr.json'
    report=json.loads(out.read_text()) if out.exists() else {'model':'Systran/faster-whisper-small','mode':'accurate','refine':False,'clips':{},'compositions':[],'human_listened':False,'speaker_similarity':'UNVERIFIED','interpretation':'Automatic recognition checks words imperfectly; source identity, timbre, acting and lack of music are not human-verified.'}
    selected=selection(corpus,args.bank); count=0
    # Actual browser inventory exposed the Caliche registration wrapper. Check
    # every finite examine turn added by that sink, beyond stratified sampling.
    additional=[c for c in corpus['clips'] if args.bank=='serling' and any(s.get('trigger')=='town-examine' for s in c['sources'])]
    requested=list({c['id']:c for c in selected+additional}.values())
    for c in requested:
        old=report['clips'].get(c['id']); file=assets/(c['id']+'.mp3')
        if args.available_only and not file.exists(): continue
        if args.limit and count>=args.limit: break
        row=checked(args.endpoint,file,c['id'],c['text'],'all_ensemble' if args.bank=='ensemble' else 'stratified_serling',old)
        row['voice']=c.get('voice','serling'); report['clips'][c['id']]=row
        if row is not old: count+=1
        atomic_json(out,report)
        print(json.dumps({'asr':c['id'],'voice':row['voice'],'edits':row['strict_word_edits'],'normalized':row['normalized_word_edits'],'status':row['status'],'recognized':row['recognized']},ensure_ascii=False),flush=True)
    report['selected_ids']=[c['id'] for c in selected]
    report['selection_complete']=all(c['id'] in report['clips'] for c in selected)
    report['additional_town_ids']=[c['id'] for c in additional]
    report['additional_town_complete']=all(c['id'] in report['clips'] for c in additional)
    if args.compositions:
        if args.bank!='serling': raise ValueError('Compositions are Serling only')
        plan=json.loads((BASE/'qa/asr-composition-plan.json').read_text())['samples']
        by_text={c['text']:c for c in corpus['clips']}
        # Check every number and identifier lexeme in assembled context, in
        # addition to the ten exact runtime scenarios supplied by the engine.
        for group,words in [('numbers',corpus['numberWords']),('identifiers',corpus['identifierWords'])]:
            for start in range(0,len(words),9):
                values=words[start:start+9]
                plan.append({'id':group+'-'+str(start),'text':' '.join(values),'clips':[{'id':by_text[w]['id'],'file':by_text[w]['id']+'.mp3','text':w} for w in values]})
        previous={r['id']:r for r in report['compositions']}
        for sample in plan:
            with tempfile.TemporaryDirectory(prefix='signal47-serling-asr-') as temp:
                wav=Path(temp)/'composition.wav'
                # Decoded f32 PCM concatenation adds no pause or rate changes,
                # exactly matching the runtime bank order.
                pcm=b''.join(subprocess.check_output(['ffmpeg','-v','error','-i',str(assets/c['file']),'-ac','1','-ar','24000','-f','f32le','-']) for c in sample['clips'])
                subprocess.run(['ffmpeg','-v','error','-y','-f','f32le','-ar','24000','-ac','1','-i','-','-c:a','pcm_s16le',str(wav)],input=pcm,check=True)
                generated=json.loads((production/'generation.json').read_text())['clips']
                spoken=' '.join(generated[c['id']].get('rendered_text',generated[c['id']]['request']['text']) for c in sample['clips'])
                row=checked(args.endpoint,wav,sample['id'],spoken,'actual_runtime_composition',previous.get(sample['id']))
                row['runtime_text']=sample['text']; row['source_clip_ids']=[c['id'] for c in sample['clips']]
                row['source_audio_sha256']=[hashlib.sha256((assets/c['file']).read_bytes()).hexdigest() for c in sample['clips']]
                previous[sample['id']]=row; report['compositions']=list(previous.values()); atomic_json(out,report)
                print(json.dumps({'composition':sample['id'],'edits':row['strict_word_edits'],'status':row['status'],'recognized':row['recognized']},ensure_ascii=False),flush=True)
        report['compositions_complete']=True
    atomic_json(out,report)

if __name__=='__main__': main()
