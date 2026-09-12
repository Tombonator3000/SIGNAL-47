#!/usr/bin/env python3
"""Download the five reviewed sources and verify their exact hashes; no Unity writes."""
import hashlib,json,urllib.request,zipfile
from pathlib import Path,PurePosixPath
ROOT=Path(__file__).resolve().parents[1];dest=ROOT/'Artifacts/Resources19/Incoming';dest.mkdir(parents=True,exist_ok=True)
for item in json.loads((ROOT/'Docs/Resources19/downloads.json').read_text()):
 p=dest/item['file']
 if p.exists():data=p.read_bytes()
 else:
  with urllib.request.urlopen(item['url'],timeout=60) as r:data=r.read(item['bytes']+1)
 if len(data)!=item['bytes'] or hashlib.sha256(data).hexdigest()!=item['sha256']:raise RuntimeError('Source changed: '+item['file'])
 if not p.exists():p.write_bytes(data)
with zipfile.ZipFile(dest/'ambient-machinery.zip') as z:
 for entry in z.infolist():
  name=PurePosixPath(entry.filename)
  if name.is_absolute() or '..' in name.parts:raise RuntimeError('Unsafe archive path')
  if entry.is_dir():continue
  p=dest/'machinery'/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(z.read(entry))
print('Five sources verified; machinery extracted. Run prepare-resources19-audio.py next.')
