#!/usr/bin/env python3
"""Isolated fault injection from a genuinely completed case; never journey evidence."""
import argparse,hashlib,json,shutil
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('completed_profile');p.add_argument('destination');a=p.parse_args()
source=Path(a.completed_profile).resolve();destination=Path(a.destination).resolve()
original=json.loads((source/'case.json').read_text());snapshot=json.loads(original['payload'])
assert json.loads(snapshot['chapter'])['complete'],'Seed must come from a completed input journey'
for name in ('CorruptPrimary','InvalidBoth','DuplicateFrame','MissingSecond','ReadOnly'):
 root=destination/name;root.mkdir(parents=True,exist_ok=False);photos=root/'FieldPhotos';photos.mkdir()
 payload=json.loads(original['payload']);camera=json.loads(payload['camera'])
 for frame in camera['frames']:
  old=Path(frame['path']);new=photos/old.name;shutil.copy2(old,new);frame['path']=str(new)
  assert hashlib.sha256(new.read_bytes()).hexdigest()==frame['sha256']
 if name=='MissingSecond':Path(camera['frames'][1]['path']).unlink()
 if name=='DuplicateFrame':camera['frames'][1]=dict(camera['frames'][0])
 payload['camera']=json.dumps(camera,separators=(',',':'));encoded=json.dumps(payload,separators=(',',':'))
 envelope=dict(original,payload=encoded,sha256=hashlib.sha256(encoded.encode()).hexdigest())
 valid=json.dumps(envelope,indent=2)
 (root/'case.json').write_text(valid)
 if name=='CorruptPrimary':
  (root/'case.backup.json').write_text(valid);(root/'case.json').write_text('{partial interrupted write')
 if name=='InvalidBoth':
  (root/'case.json').write_text('{partial interrupted write');(root/'case.backup.json').write_text('invalid recovery copy')
 (root/'fixture.json').write_text(json.dumps({'fault':name,'seed':str(source),'scope':'Offline fault-injection fixture, excluded from normal new-game-to-ending evidence. Photo copies are confined to this task profile.'},indent=2))
 if name=='ReadOnly':root.chmod(0o555)
 print(root)
