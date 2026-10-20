#!/usr/bin/env python3
"""Read-only asset/profile audit, with an auditable JSON result (no GPU)."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import urllib.request
from audio_common import atomic_json

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'production/ensemble'

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def metadata_sha(value): return hashlib.sha256(json.dumps(value,sort_keys=True).encode('utf-8')).hexdigest()

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--baseline',type=Path,required=True,help='External setup/ensemble folder containing original asset/profile snapshots')
    parser.add_argument('--endpoint',default='http://127.0.0.1:3900')
    args=parser.parse_args()
    baseline=json.loads((args.baseline/'assets-before.json').read_text())
    old_profiles=json.loads((args.baseline/'profiles-before.json').read_text())
    if isinstance(old_profiles,dict): old_profiles=old_profiles.get('profiles',old_profiles)
    current=json.load(urllib.request.urlopen(args.endpoint+'/profiles'))
    if isinstance(current,dict): current=current.get('profiles',current)
    after={p['id']:p for p in current}
    profiles=json.loads((BASE/'profiles.json').read_text())['roles']
    changed=[name for name,h in baseline.items() if not (ROOT/name).exists() or sha(ROOT/name)!=h]
    profile_rows=[]
    for p in old_profiles:
        cid=p['id']; before=metadata_sha(p); final=metadata_sha(after[cid]) if cid in after else None
        profile_rows.append({'id':cid,'before_sha256':before,'after_sha256':final,'unchanged':before==final})
    profile_changed=[p['id'] for p in profile_rows if not p['unchanged']]
    reference_rows={}
    for role,p in profiles.items():
        audio=urllib.request.urlopen(args.endpoint+'/profiles/'+p['profile_id']+'/audio').read()
        h=hashlib.sha256(audio).hexdigest()
        reference_rows[role]={'profile_id':p['profile_id'],'expected_sha256':p['profile_reference_sha256'],'actual_sha256':h,'matches':h==p['profile_reference_sha256'],'kind':after[p['profile_id']]['kind']}
        reference_rows[role]['name_matches']=after[p['profile_id']]['name']==p['profile_name']
        reference_rows[role]['conditioning_text_matches']=after[p['profile_id']]['ref_text']==p['conditioning_text']
        reference_rows[role]['profile_metadata_sha256']=metadata_sha(after[p['profile_id']])
        reference_rows[role]['verified_own_voice']=after[p['profile_id']].get('verified_own_voice')
    shared=[]; identical=[]
    for file in (ROOT/'src/assets/audio/serling').glob('*.mp3'):
        old=ROOT/'src/assets/audio/kinski'/file.name
        if old.exists():
            shared.append(file.stem)
            if sha(file)==sha(old): identical.append(file.stem)
    errors=[]
    kinski_proof=json.loads((BASE/'kinski-corpus-preservation.json').read_text())
    kinski_file=ROOT/kinski_proof['path']
    kinski_bytes=kinski_file.read_bytes()
    kinski_blob=hashlib.sha1(b'blob '+str(len(kinski_bytes)).encode()+b'\0'+kinski_bytes).hexdigest()
    kinski_unchanged=sha(kinski_file)==kinski_proof['sha256'] and kinski_blob==kinski_proof['git_blob_sha1']
    kinski_tools_changed=subprocess.check_output(['git','diff','--name-only','5c25e05','--','tools/kinski'],cwd=ROOT,text=True).splitlines()
    if not kinski_unchanged or kinski_tools_changed: errors.append('Existing Kinski corpus/tools changed')
    if len(baseline)!=1718 or sum(n.endswith('.mp3') for n in baseline)!=1714: errors.append('Unexpected old bank baseline count')
    if changed: errors.append('Old runtime assets changed')
    if len(old_profiles)!=12 or profile_changed: errors.append('Old profiles changed or incorrect baseline count')
    required_ids={p['id'] for p in old_profiles}|{p['profile_id'] for p in profiles.values()}
    unrelated_ids=sorted(set(after)-required_ids)
    if not required_ids.issubset(after) or len(profiles)!=6: errors.append('Missing existing or required new profiles')
    if len({p['profile_reference_sha256'] for p in profiles.values()})!=6: errors.append('New reference hashes not distinct')
    if len({p['profile_id'] for p in profiles.values()})!=6: errors.append('New actual profile IDs not distinct')
    if any(not r['matches'] or r['kind']!='clone' or not r['name_matches'] or not r['conditioning_text_matches'] or r['verified_own_voice'] for r in reference_rows.values()): errors.append('Saved reference/profile name/conditioning text differs, is not a clone, or asserts own voice')
    if identical: errors.append('Serling contains byte-identical Kinski audio')
    # Source audio/models are never part of either runtime bank.
    extra=[]
    for bank in ['ensemble','serling']:
        for file in (ROOT/'src/assets/audio'/bank).rglob('*'):
            if file.is_file() and file.name!='catalog.json' and file.suffix!='.mp3': extra.append(str(file.relative_to(ROOT)))
    if extra: errors.append('Unexpected runtime source/reference/model file')
    report={'status':'PASS' if not errors else 'FAIL','errors':errors,'base_commit':'5c25e05','assets':{'old_mp3':1714,'old_catalogs':4,'checked':len(baseline),'changed':changed,'files_sha256':baseline},'profiles':{'before':len(old_profiles),'after':len(current),'preserved':sum(p['unchanged'] for p in profile_rows),'changed':profile_changed,'new_ids':sorted(p['profile_id'] for p in profiles.values()),'unrelated_extra_ids':unrelated_ids,'metadata_hash_recipe':'sha256(json.dumps(profile, sort_keys=True).encode("utf-8")); Python defaults ensure_ascii=True and spaced separators, entire object','rows':profile_rows},'references':{'distinct':len({p['profile_reference_sha256'] for p in profiles.values()}),'profile_ids_by_role':{r:p['profile_id'] for r,p in profiles.items()},'rows':reference_rows},'serling_vs_kinski':{'shared_ids':len(shared),'byte_identical_ids':identical,'interpretation':'Different bytes identify a separate rendering, not verified actor similarity.'},'unexpected_runtime_files':extra}
    report['kinski_corpus']={'path':kinski_proof['path'],'sha256':sha(kinski_file),'git_blob_sha1':kinski_blob,'unchanged':kinski_unchanged}
    report['kinski_tools_changed']=kinski_tools_changed
    atomic_json(BASE/'preservation.json',report)
    print(json.dumps({k:report[k] for k in ['status','errors']}) )
    if errors: raise SystemExit(1)

if __name__=='__main__': main()
