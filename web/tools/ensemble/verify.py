#!/usr/bin/env python3
"""CPU file/PCM audit. ASR is a separate explicitly serial GPU operation."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
from audio_common import atomic_json, metrics, tokens
from asr import selection
from generate import SETTINGS, spoken_text

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'production/ensemble'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_bank(bank):
    production = BASE if bank == 'ensemble' else BASE / 'serling'
    assets = ROOT / 'src/assets/audio' / bank
    corpus = json.loads((production / 'corpus.json').read_text())
    catalog = json.loads((assets / 'catalog.json').read_text())
    generation = json.loads((production / 'generation.json').read_text())
    profiles = json.loads((BASE / 'profiles.json').read_text())['roles']
    overrides_file=production/'request-overrides.json'
    overrides=json.loads(overrides_file.read_text()) if overrides_file.exists() else {}
    expected = {c['id']: c for c in corpus['clips']}
    actual = {c['id']: c for c in catalog['clips']}
    errors, rows = [], []
    if len(actual) != len(catalog['clips']): errors.append('Duplicate catalog IDs')
    if set(actual) != set(expected): errors.append('Catalog IDs differ from corpus')
    if set(generation['clips']) != set(expected): errors.append('Generation IDs differ from corpus')
    if len(expected) != {'ensemble': 100, 'serling': 1402}[bank]: errors.append('Unexpected frozen corpus count')
    if bank=='serling':
        pronunciation=json.loads((production/'pronunciation-audit.json').read_text())
        if pronunciation['status']!='PASS' or pronunciation['corpus_sha256']!=sha(production/'corpus.json') or pronunciation['generator_sha256']!=sha(ROOT/'tools/ensemble/generate.py'): errors.append('Pronunciation audit is stale or failed')
        if pronunciation['pending_existing_audio_corrections']: errors.append('Unfinished consolidated pronunciation corrections')
        if {r['id'] for r in pronunciation['requests']}!=set(expected): errors.append('Pronunciation audit ID coverage differs')
    wanted = {c['id'] + '.mp3' for c in corpus['clips']} | {'catalog.json'}
    extra = sorted(str(p.relative_to(assets)) for p in assets.rglob('*') if p.is_file() and str(p.relative_to(assets)) not in wanted)
    if extra: errors.append({'unexpected_runtime_files': extra})
    dropped_zero = 0
    for cid, c in actual.items():
        try:
            role = c.get('voice', 'serling')
            original = expected[cid]
            normalized = ' '.join(c['text'].split())
            key = role + '\0' + normalized if bank == 'ensemble' else normalized
            if cid != hashlib.sha256(key.encode()).hexdigest()[:16]: raise ValueError('Unstable text ID')
            if c['text'] != original['text'] or (bank == 'ensemble' and c['voice'] != original['voice']): raise ValueError('Corpus text/role mismatch')
            if c['file'] != cid + '.mp3': raise ValueError('Unexpected filename')
            if set(c) != ({'id','text','duration','file','voice'} if bank == 'ensemble' else {'id','text','duration','file'}): raise ValueError('Runtime catalog contains unexpected metadata')
            file = assets / c['file']
            generated = generation['clips'][cid]
            requested={**SETTINGS,'profile_id':profiles[role]['profile_id'],'text':spoken_text(original,bank,corpus)}
            requested.update({k:v for k,v in overrides.get(cid,{}).items() if k in ('seed','text')})
            if generated['request']!=requested: raise ValueError('Final request is not reproducible from current settings/pronunciation overrides')
            override=overrides.get(cid,{})
            if generated.get('carrier_trim_seconds')!=override.get('trim_seconds'): raise ValueError('Final carrier trim differs from its reproducible override')
            if override.get('trim_seconds') and generated.get('rendered_text')!=override.get('rendered_text'): raise ValueError('Carrier-trim output text differs')
            if override.get('trim_seconds'):
                carrier=json.loads((production/'carrier-word-controls.json').read_text())['clips'][cid]
                if generated.get('uncut_wav_sha256')!=carrier['uncut_wav_sha256']: raise ValueError('Uncut carrier WAV hash differs from boundary evidence')
                if generated['carrier_trim_seconds']!=carrier['trim_seconds']: raise ValueError('Carrier boundary differs from no-prompt word-time evidence')
                cached=Path.home()/'.cache/signal47-ensemble'/bank/generated['cache_identity']/(cid+'.wav')
                if not cached.exists() or sha(cached)!=generated['uncut_wav_sha256']: raise ValueError('Original uncut carrier cache missing or changed')
            if generated['response'].get('seed')!=requested['seed']: raise ValueError('Returned generation seed differs')
            if sha(file) != generated['audio']['sha256']: raise ValueError('MP3 differs from generation hash')
            if generated['request']['profile_id'] != profiles[role]['profile_id']: raise ValueError('Wrong actual profile in request')
            if generated['reference_sha256'] != profiles[role]['profile_reference_sha256']: raise ValueError('Wrong pinned profile reference')
            if generated['response'].get('type') != 'done' or generated['response'].get('dropped_chunks') != 0: raise ValueError('Generation not complete with zero dropped chunks')
            dropped_zero += 1
            m = metrics(file)
            probe = json.loads(subprocess.check_output(['ffprobe','-v','error','-show_entries','stream=codec_name,sample_rate,channels,bit_rate','-of','json',str(file)]))['streams'][0]
            if (probe['codec_name'],probe['sample_rate'],probe['channels'],probe['bit_rate']) != ('mp3','24000',1,'64000'): raise ValueError('Wrong MP3 format')
            if c['duration'] <= 0 or m['duration_seconds'] <= 0 or abs(c['duration']-m['duration_seconds']) > .001: raise ValueError('Nonpositive or inconsistent duration')
            if m['peak_dbfs'] is None or m['peak_dbfs'] > -2 or m['nonsilent_seconds'] < .03 or m['samples_at_or_above_full_scale']: raise ValueError('Peak/silence/clipping check failed')
            rows.append({'id':cid,'voice':role,'bytes':file.stat().st_size,'sha256':sha(file),**m})
        except Exception as exc:
            errors.append({'id':cid,'error':str(exc)})
    asr_file = production / 'asr.json'
    asr = json.loads(asr_file.read_text()) if asr_file.exists() else {'clips':{}}
    asr_rows = asr.get('clips',{})
    for cid,r in asr_rows.items():
        if cid not in expected or r['audio_sha256'] != sha(assets/(cid+'.mp3')): errors.append({'id':cid,'error':'Stale or unknown ASR audio'})
        if r.get('status') not in ('PASS','REVIEWED_DIFFERENCE'): errors.append({'id':cid,'error':'ASR needs review','status':r.get('status')})
        if not tokens(r.get('recognized','')): errors.append({'id':cid,'error':'Empty recognized speech'})
        if r.get('status')=='REVIEWED_DIFFERENCE' and not r.get('note'): errors.append({'id':cid,'error':'ASR difference has no review note'})
        need=len(re.findall(r'(?<![\w])[-−]\d|\bminus\b',r['expected'],re.I))
        signs=len(re.findall(r'\b(?:minus|negative)\b|(?<![\w])[-−]\d',r['recognized'],re.I))
        if need>signs: errors.append({'id':cid,'error':'Required negative sign absent from ASR'})
    if bank == 'ensemble' and set(asr_rows) != set(expected): errors.append('ASR must cover all 100 ensemble clips')
    if bank == 'serling' and not asr.get('selection_complete'): errors.append('Stratified Serling ASR incomplete')
    selected_ids={c['id'] for c in selection(corpus,bank)}
    if not selected_ids.issubset(asr_rows): errors.append('Actual stratified/chapter selection is missing ASR rows')
    if set(asr.get('selected_ids',[]))!=selected_ids: errors.append('Recorded ASR selected_ids differ from the current deterministic selection')
    if bank == 'serling' and not asr.get('compositions_complete'): errors.append('Actual dynamic/number/identifier composition ASR incomplete')
    if bank=='serling':
        town_ids={c['id'] for c in corpus['clips'] if any(s.get('trigger')=='town-examine' for s in c['sources'])}
        if set(asr.get('additional_town_ids',[]))!=town_ids or not town_ids.issubset(asr_rows): errors.append('All finite town-examine turns require current ASR')
        addendum=json.loads((production/'town-addendum.json').read_text())
        changed_prior=[]
        for old_bank,files in addendum['before_mp3_sha256'].items():
            for name,h in files.items():
                file=ROOT/'src/assets/audio'/old_bank/name
                if not file.exists() or sha(file)!=h:changed_prior.append(old_bank+'/'+name)
        if changed_prior:errors.append({'previously_selected_mp3_changed':changed_prior})
        planned=json.loads((BASE/'qa/asr-composition-plan.json').read_text())['samples']
        planned_ids={r['id'] for r in planned}
        planned_ids|={group+'-'+str(i) for group,words in [('numbers',corpus['numberWords']),('identifiers',corpus['identifierWords'])] for i in range(0,len(words),9)}
        if {r['id'] for r in asr.get('compositions',[])}!=planned_ids: errors.append('Composition plan ID coverage differs')
        actual_compositions={r['id']:r for r in asr.get('compositions',[])}
        for sample in planned:
            if sample['id'] in actual_compositions and [c['id'] for c in sample['clips']]!=actual_compositions[sample['id']]['source_clip_ids']: errors.append({'composition':sample['id'],'error':'Composition clip order differs from runtime plan'})
    for row in asr.get('compositions',[]):
        if row.get('status') not in ('PASS','REVIEWED_DIFFERENCE'): errors.append({'composition':row['id'],'error':'ASR composition needs review'})
        if not tokens(row.get('recognized','')): errors.append({'composition':row['id'],'error':'Empty composition ASR'})
        if row.get('status')=='REVIEWED_DIFFERENCE' and not row.get('note'): errors.append({'composition':row['id'],'error':'ASR composition difference lacks review note'})
        need=len(re.findall(r'(?<![\w])[-−]\d|\bminus\b',row['expected'],re.I))
        signs=len(re.findall(r'\b(?:minus|negative)\b|(?<![\w])[-−]\d',row['recognized'],re.I))
        if need>signs: errors.append({'composition':row['id'],'error':'Required negative sign absent from composition ASR'})
        hashes=[sha(assets/(cid+'.mp3')) for cid in row['source_clip_ids']]
        if hashes!=row['source_audio_sha256']: errors.append({'composition':row['id'],'error':'Stale composition audio'})
    context_file=production/'context-asr.json'
    contexts=json.loads(context_file.read_text()).get('samples',{}) if context_file.exists() else {}
    for cid,row in contexts.items():
        if row.get('status') not in ('PASS','REVIEWED_DIFFERENCE'): errors.append({'context':cid,'error':'Unreviewed context ASR'})
        if row.get('status')=='REVIEWED_DIFFERENCE' and not row.get('note'): errors.append({'context':cid,'error':'Context difference lacks review note'})
        if not tokens(row.get('recognized','')): errors.append({'context':cid,'error':'Empty context ASR'})
        if [sha(assets/(i+'.mp3')) for i in row['source_clip_ids']]!=row['source_audio_sha256']: errors.append({'context':cid,'error':'Stale context source audio'})
    ambiguities=[cid for cid,r in asr_rows.items() if r.get('unresolved_asr_ambiguity')]
    composition_ambiguities=[r['id'] for r in asr.get('compositions',[]) if r.get('unresolved_asr_ambiguity')]
    context_ambiguities=[cid for cid,r in contexts.items() if r.get('unresolved_asr_ambiguity')]
    result = {'status':'PASS' if not errors else 'FAIL','errors':errors,'clips':len(rows),'bytes':sum(r['bytes'] for r in rows),'duration_seconds':sum(r['duration_seconds'] for r in rows),'audio':rows,'catalog_ids_match_corpus':set(actual)==set(expected),'orphan_or_unexpected_files':extra,'dropped_chunks':{'confirmed_zero':dropped_zero},'asr_clip_count':len(asr_rows),'asr_composition_count':len(asr.get('compositions',[])),'word_check_status':('CHECKED_WITH_UNRESOLVED_ASR_AMBIGUITY' if ambiguities else 'CHECKED_WITH_RECORDED_DIFFERENCES') if asr_rows else 'NOT_RUN','unresolved_asr_ambiguities':ambiguities,'corpus_sha256':sha(production/'corpus.json'),'catalog_sha256':sha(assets/'catalog.json'),'human_listened':False,'speaker_similarity':'UNVERIFIED','emotional_delivery':'UNVERIFIED'}
    result['context_asr_count']=len(contexts)
    if bank=='serling':
        result['current_stratified_asr_count']=len(selected_ids)
        result['additional_town_asr_count']=len(town_ids)
        result['previously_selected_mp3_preservation']={'checked':sum(len(files) for files in addendum['before_mp3_sha256'].values()),'changed':changed_prior,'status':'PASS' if not changed_prior else 'FAIL'}
    result['unresolved_composition_asr_ambiguities']=composition_ambiguities
    result['unresolved_context_asr_ambiguities']=context_ambiguities
    if composition_ambiguities or context_ambiguities: result['word_check_status']='CHECKED_WITH_UNRESOLVED_ASR_AMBIGUITY'
    result['context_asr_sha256']=sha(context_file) if context_file.exists() else None
    atomic_json(production / 'bank-verification.json',result)
    return result


def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--bank',choices=['ensemble','serling','all'],default='all'); args=parser.parse_args()
    reports={bank:verify_bank(bank) for bank in (['ensemble','serling'] if args.bank=='all' else [args.bank])}
    if args.bank=='all':
        report={'status':'PASS' if all(r['status']=='PASS' for r in reports.values()) else 'FAIL','errors':[{bank:r['errors']} for bank,r in reports.items() if r['errors']],'banks':reports,'clips':sum(r['clips'] for r in reports.values()),'bytes':sum(r['bytes'] for r in reports.values()),'duration_seconds':sum(r['duration_seconds'] for r in reports.values()),'word_check_status':'CHECKED_WITH_UNRESOLVED_ASR_AMBIGUITY' if any(r['word_check_status']=='CHECKED_WITH_UNRESOLVED_ASR_AMBIGUITY' for r in reports.values()) else 'CHECKED_WITH_RECORDED_DIFFERENCES','human_listened':False,'speaker_similarity':'UNVERIFIED','emotional_delivery':'UNVERIFIED'}
        atomic_json(BASE/'verification.json',report)
    else: report=reports[args.bank]
    print(json.dumps({k:report[k] for k in ['status','clips','bytes','duration_seconds','errors']},ensure_ascii=False),flush=True)
    if report['status']!='PASS': raise SystemExit(1)


if __name__=='__main__': main()
