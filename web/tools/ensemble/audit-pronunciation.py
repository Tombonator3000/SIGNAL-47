#!/usr/bin/env python3
"""CPU audit of the complete stable speech-request transformation, no audio."""
from pathlib import Path
import hashlib
import json
import re
from audio_common import atomic_json
from generate import spoken_text

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'production/ensemble/serling'

def main():
    corpus=json.loads((BASE/'corpus.json').read_text())
    generated=json.loads((BASE/'generation.json').read_text())['clips']
    override_file=BASE/'request-overrides.json'
    overrides=json.loads(override_file.read_text()) if override_file.exists() else {}
    rows=[]; errors=[]; changed=[]
    for clip in corpus['clips']:
        text=overrides.get(clip['id'],{}).get('text',spoken_text(clip,'serling',corpus))
        source=clip['text']
        required_minus=len(re.findall(r'(?<![\w])[-−]\d|\bminus\b',source,re.I))
        actual_minus=len(re.findall(r'\bminus\b',text,re.I))
        checks={'nonempty':bool(text.strip()),'no_double_slash':'//' not in text,'no_loose_start_punctuation':not re.match(r'^[,:;]|^\.(?!\d)',text),'no_double_period':not re.search(r'(?:\.\s*){2,}',text),'no_detached_period':not re.search(r'(?<=\s)\.(?=\s|$)',text),'negative_count_preserved':required_minus==actual_minus}
        if not all(checks.values()): errors.append({'id':clip['id'],'checks':checks,'source':source,'request':text})
        row={'id':clip['id'],'spoken_text_sha256':hashlib.sha256(text.encode()).hexdigest(),'required_negative_signs':required_minus}
        rows.append(row)
        previous=generated.get(clip['id'])
        if previous and previous['request']['text']!=text:
            changed.append({'id':clip['id'],'before_audio_sha256':previous['audio']['sha256'],'before_request_text':previous['request']['text'],'corrected_request_text':text,'reason':'One consolidated technical punctuation/sign/identifier request audit. Source text, ID and actual profile remain unchanged.'})
    # Explicitly distinguish hyphenated IDs and numeric ranges from signs.
    examples={raw:spoken_text({'text':raw},'serling',corpus) for raw in ['S-03','B-12','-39 LY','45-65','-00 LY','. A complete fragment.','First. . Second.','First // Second','.5']}
    if 'minus' in examples['S-03'].lower() or 'minus' in examples['B-12'].lower() or 'minus' in examples['45-65'].lower(): errors.append('Identifier/range hyphen became minus')
    if examples['-39 LY']!='minus thirty nine light years' or examples['-00 LY']!='minus zero zero light years' or examples['.5']!='.5': errors.append('Signed value or leading decimal changed')
    result={'status':'PASS' if not errors else 'FAIL','errors':errors,'clips':len(rows),'corpus_sha256':hashlib.sha256((BASE/'corpus.json').read_bytes()).hexdigest(),'generator_sha256':hashlib.sha256((ROOT/'tools/ensemble/generate.py').read_bytes()).hexdigest(),'scope':'CPU-only request-text structure and sign/ID preservation; no claim of listening or correct generated pronunciation.','examples':examples,'requests':rows,'pending_existing_audio_corrections':changed}
    atomic_json(BASE/'pronunciation-audit.json',result)
    history=json.loads((BASE/'corrections.json').read_text())
    existing={(r['id'],r['before_audio_sha256'],r['corrected_request_text']) for r in history['attempts']}
    history['attempts'] += [r for r in changed if (r['id'],r['before_audio_sha256'],r['corrected_request_text']) not in existing]
    atomic_json(BASE/'corrections.json',history)
    print(json.dumps({'status':result['status'],'clips':len(rows),'errors':errors,'queued_existing_corrections':len(changed)}))
    if errors: raise SystemExit(1)

if __name__=='__main__': main()
