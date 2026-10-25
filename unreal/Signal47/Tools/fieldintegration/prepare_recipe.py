"""Compose the three measured replacement plans. Does not launch Blender or Unreal."""
import argparse, hashlib, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
def prepare(export,out):
 mb=(export/'manifest.json').read_bytes();m=json.loads(mb);nodes={n['i']:n for n in m['nodes']};rows={}
 def add(node,whole=False,cuts=(),protect=()):
  n=nodes[node];assert n.get('geo') is not None
  r=rows.setdefault(n['geo'],{'node':node,'parent':n['p'],'geo':n['geo'],'source_material_ids':n['mats'],'whole':False,'cut_boxes':[],'protect_boxes':[]})
  assert r['node']==node,('Two components use the same replacement geometry',r,node)
  r['whole']|=whole;r['cut_boxes'].extend(b for b in cuts if b not in r['cut_boxes']);r['protect_boxes'].extend(b for b in protect if b not in r['protect_boxes'])
 def descendants(root):
  for n in nodes.values():
   i=n['i']
   while i>=0:
    if i==root:yield n;break
    i=nodes[i]['p']
 def boxes(values):
  return [[b['min'][0],b['max'][0],b['min'][1],b['max'][1],b['min'][2],b['max'][2]]for b in values]
 sources=[]
 hut=json.loads((ROOT/'Tools/fieldstation/runtime_replacement.json').read_text());sources.append(hut)
 for r in hut['whole_hide_nodes']:add(r['node'],True)
 for r in hut['partial_mesh_cuts']:add(r['node'],cuts=r['remove_whole_triangles_inside'])
 instruments=json.loads((ROOT/'Tools/fieldinstruments/retire_candidates.json').read_text());sources.append(instruments)
 for volume in instruments['volumes']:
  for r in volume['candidates']:add(r['node'],cuts=boxes(r['cut_boxes']),protect=boxes(r['protect_boxes']))
 for handle in instruments['hide_original_mesh_descendants_only']:
  for n in descendants(m['handles'][handle]['node']):
   if n.get('geo')is not None:add(n['i'],True)
 for r in instruments['additional_retire_named_meshes']:
  assert nodes[r['node']]['name']=='lic:car_battery'
  for n in descendants(r['node']):
   if n.get('geo')is not None:add(n['i'],True)
 arc=json.loads((ROOT/'Tools/archiveprops/integration_contract.json').read_text());sources.append(arc)
 for group in arc['retire_geometry_expected_nodes'].values():
  for n in group:add(n['i'],True)
 for n in arc['retire_named_nodes_in_scope']:add(n['i'],True)
 for r in arc['mixed_geometry'].values():
  if r.get('cuts_three_mesh_local_xxyyzz'):add(r['node']['i'],cuts=r['cuts_three_mesh_local_xxyyzz'])
 digest=hashlib.sha256(mb).hexdigest();assert all(s['manifest_sha256']==digest for s in sources)
 result={'version':1,'manifest_sha256':digest,'policy':'Only measured leaf meshes or whole triangles inside authored volumes; no parent/proxy/collider edits','meshes':sorted(rows.values(),key=lambda r:r['node'])}
 out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(result,indent=2)+'\n');return result
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('export',type=Path);p.add_argument('out',type=Path);a=p.parse_args();r=prepare(a.export,a.out);print('FIELD_RECIPE',len(r['meshes']),'precise leaf replacements')
