#!/usr/bin/env python3
"""Avgrenset, gjenopptakbar lokal lydproduksjon av fem frosne introavsnitt.

GPU-steg kjøres serielt under prosjektets eksisterende SLOT --alone. Originale
referanser ligger i den eksisterende profilen; ingen kopieres til spillbanken.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import sys
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
ASSETS = ROOT / 'src/assets/audio/serling-intro'
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / 'tools/ensemble'))
from audio_common import atomic_json, generate, encode, transcribe, metrics, tokens, edit_distance

TEXTS = [
    'New Mexico, 1986. Twenty-seven antennas listen to a sky that has never answered. On the night shift, silence is a measurement. It can be filed, signed, and forgotten.',
    'Tonight, the instruments will record something the sky has not sent. A telephone will ring in an empty room. A photograph will remember what no one saw.',
    'Your name is Reyes. Your shift ends at dawn. Between now and then, forty-seven seconds will refuse to belong to any clock.',
    'The record calls it interference.',
    'The record is incomplete.',
]
SETTINGS = {'language':'en','engine':'omnivoice','num_step':32,'seed':42,
            'speed':1,'denoise':True,'stream':True,'effect_preset':'broadcast'}
API = 'http://127.0.0.1:3900'
CACHE = Path.home() / '.cache/signal47-serling-intro'

def sha(file): return hashlib.sha256(file.read_bytes()).hexdigest()
def digest(value): return hashlib.sha256(json.dumps(value,sort_keys=True).encode()).hexdigest()
def get(path): return json.load(urllib.request.urlopen(API+path,timeout=30))
def profile_list():
    value=get('/profiles')
    return value.get('profiles',[]) if isinstance(value,dict) else value

def identity():
    manifest=ROOT/'production/ensemble/profiles.json'
    saved=json.loads(manifest.read_text())['roles']['serling']
    actual=next((p for p in profile_list() if p['id']==saved['profile_id']),None)
    if not actual or actual['kind']!='clone': raise ValueError('Existing Serling clone profile unavailable')
    if actual['name']!=saved['profile_name'] or actual['ref_text']!=saved['conditioning_text']:
        raise ValueError('Existing Serling metadata differs from source manifest')
    data=urllib.request.urlopen(API+'/profiles/'+actual['id']+'/audio',timeout=30).read()
    reference=hashlib.sha256(data).hexdigest()
    if reference!=saved['profile_reference_sha256']: raise ValueError('Existing Serling reference changed')
    return {'profile_id':actual['id'],'kind':actual['kind'],'profile_metadata_sha256':digest(actual),
            'reference_sha256':reference,'source_manifest':'production/ensemble/profiles.json',
            'source_manifest_sha256':sha(manifest),'source_role':'serling'}

def corpus():
    return [{'id':hashlib.sha256(text.encode()).hexdigest()[:16],'text':text,'order':i+1}
            for i,text in enumerate(TEXTS)]

def init():
    OUT.mkdir(parents=True,exist_ok=True); ASSETS.mkdir(parents=True,exist_ok=True)
    items=corpus(); previous={c['id']:c for c in json.loads((ASSETS/'catalog.json').read_text())['clips']} if (ASSETS/'catalog.json').exists() else {}
    atomic_json(OUT/'corpus.json',{'version':1,'source':'Five frozen original introduction paragraphs authorized on 2026-10-21','clips':items})
    atomic_json(ASSETS/'catalog.json',{'version':1,'voice':'Rod Serling synthetic parody narration',
        'clips':[{'id':c['id'],'text':c['text'],'file':c['id']+'.mp3','duration':previous.get(c['id'],{}).get('duration',0)} for c in items]})
    (OUT/'manus.md').write_text('# Frosset intromanus\n\nFem originale engelske avsnitt. SIGNAL / 47 er visbar merkevare og har ikke et eget lydklipp.\n\n'+'\n\n'.join(f"{c['order']}. {c['text']}" for c in items)+'\n')
    if not (OUT/'preservation-baseline.json').exists():
        names=subprocess.check_output(['git','ls-files','-z','--','src/assets/audio'],cwd=ROOT).decode().split('\0')
        files={name:sha(ROOT/name) for name in names if name and (ROOT/name).is_file()}
        profiles=profile_list()
        if len(profiles)!=18: raise ValueError('Expected the existing 18-profile baseline')
        rows={}
        for p in profiles:
            data=urllib.request.urlopen(API+'/profiles/'+p['id']+'/audio',timeout=30).read()
            rows[p['id']]={'metadata_sha256':digest(p),'reference_sha256':hashlib.sha256(data).hexdigest()}
        atomic_json(OUT/'preservation-baseline.json',{'base_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
            'audio_files_sha256':files,'profiles':rows,'profile_hash_recipe':'sha256(json.dumps(profile,sort_keys=True).encode()); Python defaults, complete live object'})
    atomic_json(OUT/'profile-binding.json',identity())
    print(json.dumps({'initialized':len(items),'profile':identity()['profile_id']}),flush=True)

def spoken(clip):
    return clip['text'].replace('New Mexico, 1986.','New Mexico, nineteen eighty-six.')

def requests(binding):
    overrides=json.loads((OUT/'request-overrides.json').read_text()) if (OUT/'request-overrides.json').exists() else {}
    return {c['id']:{**SETTINGS,'profile_id':binding['profile_id'],'text':spoken(c),
                     **{k:v for k,v in overrides.get(c['id'],{}).items() if k in ('text','seed')}} for c in corpus()}

def produce():
    binding=identity(); wanted=requests(binding)
    report=json.loads((OUT/'generation.json').read_text()) if (OUT/'generation.json').exists() else {'clips':{}}
    cat=json.loads((ASSETS/'catalog.json').read_text()); by_id={c['id']:c for c in cat['clips']}
    for clip in corpus():
        cid=clip['id']; request=wanted[cid]
        cache_key=digest({'request':request,'reference_sha256':binding['reference_sha256']})[:20]
        cache=CACHE/cache_key; cache.mkdir(parents=True,exist_ok=True)
        wav=cache/(cid+'.wav'); result_file=cache/(cid+'.json'); mp3=ASSETS/(cid+'.mp3')
        old=report['clips'].get(cid)
        if old and old['cache_identity']==cache_key and mp3.exists() and sha(mp3)==old['audio']['sha256']: continue
        if not wav.exists():
            result=generate(API,request)
            data=urllib.request.urlopen(API+'/audio/'+result['audio_path'],timeout=60).read()
            wav.write_bytes(data); atomic_json(result_file,result)
        else: result=json.loads(result_file.read_text())
        audio=encode(wav,mp3)
        report['clips'][cid]={'text':clip['text'],'spoken_text':request['text'],'request':request,'response':result,
            'cache_identity':cache_key,'uncut_wav_sha256':sha(wav),'reference_sha256':binding['reference_sha256'],'audio':audio}
        by_id[cid]['duration']=audio['duration_seconds']
        atomic_json(OUT/'generation.json',report);atomic_json(ASSETS/'catalog.json',cat)
        print(json.dumps({'generated':cid,'seconds':audio['duration_seconds']}),flush=True)

def normalized(text):
    # Exact same numeric values; no names or missing lexical words are erased.
    for value,words in [('1986','nineteen eighty six'),('27','twenty seven'),('47','forty seven')]:
        text=re.sub(r'\b'+value+r'\b',words,text)
    return tokens(text.replace('’',"'"))

def asr():
    report=json.loads((OUT/'asr.json').read_text()) if (OUT/'asr.json').exists() else {'model':'Systran/faster-whisper-small','mode':'accurate','refine':False,'human_listened':False,'clips':{}}
    for c in corpus():
        file=ASSETS/(c['id']+'.mp3');h=sha(file);old=report['clips'].get(c['id'])
        if old and old['audio_sha256']==h:continue
        result=transcribe(API,file);text=result.get('text','');edits=edit_distance(tokens(c['text']),tokens(text));same=edit_distance(normalized(c['text']),normalized(text))
        status='PASS' if edits==0 else 'REVIEWED_DIFFERENCE' if same==0 else 'REVIEW_REQUIRED'
        report['clips'][c['id']]={'expected':c['text'],'recognized':text,'strict_word_edits':edits,'same_value_word_edits':same,
            'status':status,'note':'Same-value number spelling only.' if edits and same==0 else '', 'audio_sha256':h,'response':result,'human_word_check':'UNVERIFIED'}
        atomic_json(OUT/'asr.json',report)
        print(json.dumps({'asr':c['id'],'status':status,'recognized':text}),flush=True)

def verify():
    binding=identity();wanted=requests(binding);baseline=json.loads((OUT/'preservation-baseline.json').read_text())
    report=json.loads((OUT/'generation.json').read_text());word=json.loads((OUT/'asr.json').read_text());catalog=json.loads((ASSETS/'catalog.json').read_text())
    current={p['id']:p for p in profile_list()};profile_rows={};changed=[]
    for pid,before in baseline['profiles'].items():
        data=urllib.request.urlopen(API+'/profiles/'+pid+'/audio',timeout=30).read()
        after={'metadata_sha256':digest(current[pid]),'reference_sha256':hashlib.sha256(data).hexdigest()}
        profile_rows[pid]={'before':before,'after':after,'unchanged':before==after}
    for name,h in baseline['audio_files_sha256'].items():
        file=ROOT/name
        if not file.exists() or sha(file)!=h: changed.append(name)
    preservation={'status':'PASS' if not changed and all(r['unchanged'] for r in profile_rows.values()) and set(current)==set(baseline['profiles']) else 'FAIL',
        'original_audio_files':len(baseline['audio_files_sha256']),'original_mp3':sum(n.endswith('.mp3') for n in baseline['audio_files_sha256']),
        'changed_audio_files':changed,'original_profiles':18,'current_profiles':len(current),'profiles':profile_rows,
        'baseline_file':'preservation-baseline.json','baseline_sha256':sha(OUT/'preservation-baseline.json')}
    atomic_json(OUT/'preservation.json',preservation)
    errors=[];rows=[];items=corpus();expected={c['id']:c for c in items}
    if {c['id'] for c in catalog['clips']}!=set(expected) or len(catalog['clips'])!=5:errors.append('Catalog coverage differs from five frozen paragraphs')
    extra=sorted(f.name for f in ASSETS.iterdir() if f.is_file() and f.name not in {'catalog.json',*(c['id']+'.mp3' for c in items)})
    if extra:errors.append({'unexpected_runtime_files':extra})
    for c in catalog['clips']:
        cid=c['id'];file=ASSETS/c['file'];g=report['clips'][cid];a=word['clips'][cid]
        if set(c)!={'id','text','file','duration'} or c['text']!=expected[cid]['text'] or c['file']!=cid+'.mp3':errors.append({'id':cid,'error':'Runtime text/schema mismatch'})
        if g['request']!=wanted[cid] or g['reference_sha256']!=binding['reference_sha256']:errors.append({'id':cid,'error':'Actual profile/request differs'})
        if g['response'].get('type')!='done' or g['response'].get('dropped_chunks')!=0:errors.append({'id':cid,'error':'Incomplete/dropped generation'})
        if g['response']['seed']!=g['request']['seed']:errors.append({'id':cid,'error':'Actual seed differs'})
        m=metrics(file);probe=json.loads(subprocess.check_output(['ffprobe','-v','error','-show_entries','stream=codec_name,sample_rate,channels,bit_rate','-of','json',str(file)]))['streams'][0]
        if (probe['codec_name'],probe['sample_rate'],probe['channels'],probe['bit_rate'])!=('mp3','24000',1,'64000'):errors.append({'id':cid,'error':'Wrong format'})
        if m['peak_dbfs'] is None or m['peak_dbfs']>-2 or m['nonsilent_seconds']<.03 or m['samples_at_or_above_full_scale'] or m['duration_seconds']<=0 or abs(m['duration_seconds']-c['duration'])>.001:errors.append({'id':cid,'error':'PCM/duration/peak failure'})
        if sha(file)!=g['audio']['sha256'] or a['audio_sha256']!=sha(file):errors.append({'id':cid,'error':'Stale generation/ASR hash'})
        if a['status'] not in ['PASS','REVIEWED_DIFFERENCE'] or not a['recognized'].strip() or a['status']=='REVIEWED_DIFFERENCE' and not a['note']:errors.append({'id':cid,'error':'Word check needs explicit review'})
        rows.append({'id':cid,'file':c['file'],'sha256':sha(file),'bytes':file.stat().st_size,**m})
    if preservation['status']!='PASS':errors.append('Original files/profiles changed')
    result={'status':'PASS' if not errors else 'FAIL','errors':errors,'clips':len(rows),'bytes':sum(r['bytes'] for r in rows),'duration_seconds':sum(r['duration_seconds'] for r in rows),'audio':rows,
        'asr_clips':len(word['clips']),'human_listened':False,'human_word_check':'UNVERIFIED','speaker_similarity':'UNVERIFIED','acting':'UNVERIFIED',
        'catalog_sha256':sha(ASSETS/'catalog.json'),'corpus_sha256':sha(OUT/'corpus.json'),'asr_sha256':sha(OUT/'asr.json'),'preservation_sha256':sha(OUT/'preservation.json')}
    atomic_json(OUT/'audio-verification.json',result)
    atomic_json(OUT/'audio-provenance.json',{'synthetic':True,'label':'Synthetic Rod Serling parody narration','profile':binding,'settings':SETTINGS,
        'source_provenance_reference':'production/ensemble/profiles.json and production/ensemble/provenance.json; no new source recording or original-source excerpt',
        'existing_provenance_sha256':sha(ROOT/'production/ensemble/provenance.json'),'corpus_sha256':sha(OUT/'corpus.json'),
        'same_value_pronunciation':'1986 is requested as nineteen eighty-six; original runtime text/hash is unchanged. Any bounded retake remains in request-overrides.json and generation.json.',
        'postprocessing':'One constant gain seeks -20 dBFS nonsilent RMS, decoded peak at most -2 dBFS; MP3 mono24kHz64kbps. No time stretch/pitch shift.',
        'rights':'Existing source/model caveats remain; no new consent,endorsement,verified-own-voice or actor-likeness claim.',
        'human_listened':False,'speaker_similarity':'UNVERIFIED','verification_file':'audio-verification.json','verification_status':result['status']})
    print(json.dumps({k:result[k] for k in ['status','errors','clips','bytes','duration_seconds']}),flush=True)
    if errors:raise SystemExit(1)
    atomic_json(OUT/'audio-ready.json',{'gpu_idle':True,'verification':'PASS','clips':5,'bytes':result['bytes'],'duration_seconds':result['duration_seconds'],
        'catalog_sha256':result['catalog_sha256'],'audio_verification_sha256':sha(OUT/'audio-verification.json'),'audio_provenance_sha256':sha(OUT/'audio-provenance.json')})

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('stage',choices=['init','generate','asr','verify','produce']);args=parser.parse_args()
    if args.stage=='init':init()
    elif args.stage=='generate':produce()
    elif args.stage=='asr':asr()
    elif args.stage=='verify':verify()
    else:produce();asr();verify()
