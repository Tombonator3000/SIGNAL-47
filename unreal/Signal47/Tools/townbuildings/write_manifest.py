"""Light source inventory after root's completed generation/verification."""
from pathlib import Path
import json,hashlib
R=Path(__file__).resolve().parent
c=json.loads((R/'model_contract.json').read_text());v=json.loads((R/'verification.json').read_text());assert v['status']=='PASS'
files=[]
for p in sorted(R.rglob('*')):
 if not p.is_file()or'__pycache__'in p.parts or p.suffix in ['.blend1','.blend2']or p.name=='source_manifest.json':continue
 if p.suffix not in ['.py','.md','.json','.png','.fbx','.glb','.blend']:continue
 files.append({'path':str(p.relative_to(R)),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
r={'status':'SOURCE_VERIFIED_NATIVE_VISUAL_UNVERIFIED','source_revision':c['source_revision'],'parts':len(c['parts']),'families':len(c['families']),'LOD_triangles':{k:sum(p['triangles']for p in d.values())for k,d in c['lods'].items()},'actual_source_checks':len(v['checks']),'files':files}
(R/'source_manifest.json').write_text(json.dumps(r,indent=2)+'\n');print('TOWN_SOURCE_MANIFEST_READY',r['LOD_triangles'],len(files),'files')
