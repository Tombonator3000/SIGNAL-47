#!/usr/bin/env python3
import hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Artifacts/GauntletLinux'
def digest(paths,relative_to):
 h=hashlib.sha256()
 for p in paths:
  if p.is_file():h.update(str(p.relative_to(relative_to)).encode()+b'\0');h.update(p.read_bytes())
 return h.hexdigest()
source=digest((p for base in ('Unity/Assets','Unity/Packages','Unity/ProjectSettings') for p in sorted((root/base).rglob('*'))),root)
payload=digest((p for p in sorted(out.rglob('*')) if p.name not in ('build-manifest.json','build-id.txt')),out)
revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
dirty=bool(subprocess.check_output(['git','status','--porcelain'],cwd=root,text=True))
info={'base_revision':revision,'working_tree_changes':dirty,'unity_source_sha256':source,'player_sha256':hashlib.sha256((out/'Signal47.x86_64').read_bytes()).hexdigest(),'build_payload_sha256':payload,'payload_hash_method':'Sorted relative file paths, NUL, file bytes; excludes build-manifest.json and build-id.txt. player_sha256 covers the native launcher only.'}
(out/'build-manifest.json').write_text(json.dumps(info,indent=2)+'\n')
(out/'build-id.txt').write_text(f"{revision} {'working-tree' if dirty else 'clean'} source-sha256:{source}\n")
print(json.dumps(info))
