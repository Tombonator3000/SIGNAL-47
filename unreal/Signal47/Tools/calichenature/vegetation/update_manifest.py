"""Small source bookkeeping; records evidence honestly, never starts DCC."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parent

def update():
 contract=json.loads((ROOT/'model_contract.json').read_text()) if (ROOT/'model_contract.json').is_file() else {}
 report=json.loads((ROOT/'verification.json').read_text()) if (ROOT/'verification.json').is_file() else {}
 verified=report.get('source_revision')==contract.get('source_revision') and report.get('status')=='PASS' and report.get('passed')==report.get('total')
 preview=ROOT/'botanical_preview.png';blend=ROOT/'caliche_vegetation.blend';guard=ROOT/'verification'/'preview-guard.json'
 g=json.loads(guard.read_text()) if guard.is_file() else {}
 rendered=preview.is_file() and blend.is_file() and preview.stat().st_mtime>blend.stat().st_mtime and g.get('returncode')==0 and g.get('stop_reason') is None
 for pending in ROOT.glob('CN*_pending.json'):
  row=json.loads(pending.read_text())
  if row.get('revision')==contract.get('source_revision'):
   row['state']='ROUNDTRIP_PASS_RENDER_COMPLETED_VISUAL_REVIEW_PENDING' if verified and rendered else 'ROUNDTRIP_PASS_VISUAL_PENDING' if verified else 'EXPORTED_NOT_YET_ROUNDTRIP_VERIFIED';pending.write_text(json.dumps(row,indent=2))
 files={}
 for f in sorted(ROOT.rglob('*')):
  if not f.is_file() or f.name=='asset_manifest.json' or '__pycache__' in f.parts or f.suffix in {'.pyc','.blend1'}:continue
  data=f.read_bytes();files[str(f.relative_to(ROOT))]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
 out={'asset':'Original Caliche vegetation prototype kit','source_revision':contract.get('source_revision'),'technical_status':f"{report.get('passed')}/{report.get('total')} source/FBX/GLB checks PASS" if verified else 'CURRENT_SOURCE_NOT_YET_ROUNDTRIP_VERIFIED','preview_completed_for_current_blend':rendered,'visual_status':'UNVERIFIED pending explicit review of actual botanical image and native game frames','files':files}
 (ROOT/'asset_manifest.json').write_text(json.dumps(out,indent=2));print('ASSET_MANIFEST',len(files),'files',out['technical_status'],'preview_complete',rendered)
if __name__=='__main__':update()
