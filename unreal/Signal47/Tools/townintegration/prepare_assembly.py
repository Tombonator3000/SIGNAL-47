"""Stage the original model placement and precise leaf retirement contract. Pure Python."""
import argparse,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def prepare(export):
 source=export/'manifest.json';replacement=json.loads((ROOT/'Tools/townbuildings/runtime_replacement.json').read_text())
 assert hashlib.sha256(source.read_bytes()).hexdigest()==replacement['manifest_sha256']
 man=json.loads(source.read_text());counts={}
 for f in man['geoFiles']:
  raw=(export/'geo'/f['file']).read_bytes();size=struct.unpack_from('<I',raw,12)[0];g=json.loads(raw[20:20+size])
  for mesh in g['meshes']:counts[mesh['name']]=sum(g['accessors'][p['indices']]['count']//3 for p in mesh['primitives'])
 old=json.loads((ROOT/'Content/Kelvora/office-patches.json').read_text());retire=[]
 for row in replacement['whole_hide_nodes']:
  r=dict(row);r['tris']=counts['g'+str(r['geo'])];accepted={r['tris']};prior=old.get('g'+str(r['geo']))
  if prior and not prior['hidden']:accepted.add(r['tris']-prior['removed'])
  r['acceptedPreviousTris']=sorted(accepted);retire.append(r)
 c=json.loads((ROOT/'Tools/townbuildings/model_contract.json').read_text());out=ROOT/'Content/TownBuildings';out.mkdir(exist_ok=True)
 data={'source_revision':c['source_revision'],'manifestSha256':replacement['manifest_sha256'],'parts':c['parts'],'retire':retire,'retained':replacement['retain_subtrees'],'never_hide_parents':replacement['never_hide_parents']}
 (out/'assembly.json').write_text(json.dumps(data,indent=2)+'\n');return data
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('export',type=Path);a=parser.parse_args();r=prepare(a.export);print('ASSEMBLY_PREPARED',len(r['parts']),len(r['retire']))
