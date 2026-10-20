#!/usr/bin/env python3
"""Synchronize public production metadata after extraction/verification (CPU)."""
from pathlib import Path
import hashlib
import json
from audio_common import atomic_json
from generate import SETTINGS

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'production/ensemble'
def sha(file): return hashlib.sha256(file.read_bytes()).hexdigest()

def main():
    previous=json.loads((ROOT/'production/artbell/provenance.json').read_text())
    profiles=json.loads((BASE/'profiles.json').read_text())['roles']
    verification=json.loads((BASE/'verification.json').read_text()) if (BASE/'verification.json').exists() else {}
    preservation=json.loads((BASE/'preservation.json').read_text()) if (BASE/'preservation.json').exists() else {}
    manifest={'version':1,'banks':{}}
    counts={}
    for bank in ['ensemble','serling']:
        production=BASE if bank=='ensemble' else BASE/'serling'
        assets=ROOT/'src/assets/audio'/bank
        corpus=json.loads((production/'corpus.json').read_text())
        catalog=json.loads((assets/'catalog.json').read_text())
        ready=[c for c in catalog['clips'] if c['duration']>0 and (assets/c['file']).exists()]
        files={str((assets/c['file']).relative_to(ROOT)):sha(assets/c['file']) for c in ready}
        files[str((assets/'catalog.json').relative_to(ROOT))]=sha(assets/'catalog.json')
        counts[bank]={'corpus_clips':len(corpus['clips']),'generated_clips':len(ready),'bytes':sum((assets/c['file']).stat().st_size for c in ready),'duration_seconds':sum(c['duration'] for c in ready)}
        manifest['banks'][bank]={'corpus_sha256':sha(production/'corpus.json'),'catalog_sha256':sha(assets/'catalog.json'),'files_sha256':files,**counts[bank]}
        history_file=production/'corrections.json'
        if verification.get('status')=='PASS' and history_file.exists():
            history=json.loads(history_file.read_text())
            generated=json.loads((production/'generation.json').read_text())['clips']
            corrected_ids={r['id'] for r in history['attempts']} - set(history.get('excluded_ids',[]))
            history['final_selected']={cid:{'audio_sha256':generated[cid]['audio']['sha256'],'request':generated[cid]['request'],'profile_reference_sha256':generated[cid]['reference_sha256'],'cache_identity':generated[cid]['cache_identity']} for cid in sorted(corrected_ids)}
            history['interpretation']='Attempt entries preserve prior hashes/ASR and queued correction reasoning; final_selected is the actual reproducible selected rendering. Source wording, ID and profile are unchanged.'
            atomic_json(history_file,history)
    atomic_json(BASE/'audio-asset-manifest.json',manifest)
    provenance={'synthetic':True,'label':'Synthetic actor parody ensemble and Rod Serling narration','human_listened':False,'speaker_similarity':'UNVERIFIED','emotional_delivery':'UNVERIFIED','language':'en','voice_software':previous['voice_software'],'model':previous['model'],'library_reference':previous['library_reference'],'profiles_file':'profiles.json','profile_ids':{role:p['profile_id'] for role,p in profiles.items()},'reference_hashes_distinct':len({p['profile_reference_sha256'] for p in profiles.values()})==6,'source_excerpt_policy':'One conditioning_text per actor, at most 25 source words, only in profiles.json. Full original source ASR and recordings remain outside Git.','rights':'Public source access does not establish permission to clone or distribute the voice. No consent, endorsement or independently verified source licence is asserted. The Serling archive uploader public-domain label is unverified.','request_settings':SETTINGS,'postprocessing':{'codec':'MP3','channels':1,'sample_rate':24000,'bit_rate':64000,'gain':'One constant gain per clip seeks -20 dBFS nonsilent RMS and caps decoded peak at -2 dBFS. No time stretching or pitch shift.','natural_speed':1},'scope':{'ensemble':'100 existing actor speech clips: Stanton18, Bacall49 including phone/CB/replies, Stack19, Lynch13, Price1 reused at two tape sites. Player lines/asides excluded.','serling':'1402 existing prose/chapter-field/fragment clips including complete number and identifier banks. Every closed/recap/next/title branch is included; clocks use the shared complete numeric resolver. Existing Kinski corpus remains untouched.'},'segmentation':'Exact normalized text first, then complete static fragments, number/clock and strict identifier composition. Runtime wording and IDs are unchanged by explicit pronunciation requests. Unknown tokens fail availability rather than skip words.','pronunciation_overrides':'generate.py documents same-value letter names, initials, elevation, pauses, strict IDs and signed values. Targeted corrections are pinned by request-overrides.json with separate cache identities and corrections.json evidence.','counts':counts,'verification':{'file':'verification.json','status':verification.get('status','IN_PROGRESS'),'word_check_status':verification.get('word_check_status','IN_PROGRESS'),'asr_ensemble':'asr.json: every one of 100 actor clips; context-asr.json and word-diagnostics.json preserve ambiguous short-line checks','asr_serling':'serling/asr.json: all chapter fields plus stratified long/prose sources and actual no-gap runtime compositions covering clock/negative numbers/identifiers and every number/identifier lexeme','asr_model':'Systran/faster-whisper-small, already installed, accurate mode, refine false; automatic word check is not human listening','preservation_file':'preservation.json','preservation_status':preservation.get('status','IN_PROGRESS')},'corpus_sha256':{bank:manifest['banks'][bank]['corpus_sha256'] for bank in manifest['banks']},'runtime_excludes':['original source recordings','reference WAV files','full original source ASR','model weights','VoiceStudio API dependency','local cache paths'],'audio_asset_manifest_file':'audio-asset-manifest.json','audio_asset_manifest_sha256':sha(BASE/'audio-asset-manifest.json')}
    atomic_json(BASE/'provenance.json',provenance)
    asr_summary={}
    for bank in ['ensemble','serling']:
        production=BASE if bank=='ensemble' else BASE/'serling'
        word=json.loads((production/'asr.json').read_text())
        rows=word['clips']; compositions=word.get('compositions',[])
        asr_summary[bank]={'selected_clips':len(word.get('selected_ids',[])),'checked_clips':len(rows),'additional_town_clips':len(word.get('additional_town_ids',[])),'compositions':len(compositions),
            'strict_clip_pass':sum(r['status']=='PASS' for r in rows.values()),
            'reviewed_clip_differences':sum(r['status']=='REVIEWED_DIFFERENCE' for r in rows.values()),
            'unresolved_clip_ids':[cid for cid,r in rows.items() if r.get('unresolved_asr_ambiguity')],
            'unresolved_composition_ids':[r['id'] for r in compositions if r.get('unresolved_asr_ambiguity')],
            'human_word_check':'UNVERIFIED'}
    provenance['verification']['asr_summary']=asr_summary
    provenance['verification']['interpretation']='PASS is the technical file/PCM/hash/provenance check with all automatic word results reviewed. It is not exact-word listening approval. Listed unresolved word, identifier and clock ambiguities remain.'
    provenance['rejected_carrier_trial']={'file':'serling/carrier-word-controls.json','selected_for_runtime':False,'selected_trimmed_clips':0,'decision':'A bounded carrier-word method did not establish sufficiently clean final lexemes. Carrier-free selected takes remain; no carrier words or trims are in runtime.'}
    provenance['coverage_addendum']={'file':'serling/town-addendum.json','sha256':sha(BASE/'serling/town-addendum.json'),
        'added_clips':18,'source':'src/story/caliche.ts add(...,examine) argument four, all finite branches',
        'previously_selected_mp3':1484,'preservation':verification.get('banks',{}).get('serling',{}).get('previously_selected_mp3_preservation',{}),
        'word_check':'Every added town-examine clip is checked, in addition to the recomputed current stratified set and preserved valid ASR history.'}
    atomic_json(BASE/'provenance.json',provenance)
    readme=BASE/'README.md'
    if readme.exists():
        begin='<!-- production-counts:start -->'; end='<!-- production-counts:end -->'
        text=readme.read_text()
        if begin in text: text=text[:text.index(begin)].rstrip()+'\n'
        rows=['',begin,'','## Faktiske produksjonstall','',f"Teknisk kontrollstatus: {verification.get('status','IN_PROGRESS')}. Ordstatus: {verification.get('word_check_status','IN_PROGRESS')}. Tallene under gjelder filene i arbeidskopien.",'','| Bank | Ferdige klipp | Byte | Varighet i sekunder |','| --- | ---: | ---: | ---: |']
        for bank,c in counts.items(): rows.append(f"| {bank} | {c['generated_clips']} / {c['corpus_clips']} | {c['bytes']} | {c['duration_seconds']:.2f} |")
        rows+=['',end,'']; readme.write_text(text+'\n'.join(rows))
    print(json.dumps(counts,ensure_ascii=False))

if __name__=='__main__': main()
