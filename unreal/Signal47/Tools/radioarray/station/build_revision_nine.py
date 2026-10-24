"""Owned R9 build with before/after preservation proof; never renders."""
import bpy,hashlib,json,runpy
from pathlib import Path
ROOT=Path(__file__).resolve().parent

def digest(value):return hashlib.sha256(json.dumps(value,separators=(',',':')).encode()).hexdigest()
def snapshot():
 result={}
 for o in bpy.data.objects:
  if o.type!='MESH' or not o.name.startswith(('SRC_','SM_1420_')):continue
  mesh=o.data
  result[o.name]={
   'geometry':digest({'vertices':[list(v.co) for v in mesh.vertices],'faces':[list(p.vertices) for p in mesh.polygons],'matrix':[list(row) for row in o.matrix_basis]}),
   'uv':digest([[list(v.uv) for v in uv.data] for uv in mesh.uv_layers]),
   'slots':digest({'names':[m.name for m in mesh.materials],'faces':[p.material_index for p in mesh.polygons]})}
 return result
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'1420_AM_station.blend'))
before=snapshot();old_contract=json.loads((ROOT/'model_contract.json').read_text())
# Retain the original primitive/bevel UV values exactly, avoiding modifier UV drift.
uv_archive=ROOT/'r9_original_uv.json'
if not uv_archive.exists():
 uv_rows={o.name[4:]:[[list(v.uv) for v in layer.data] for layer in o.data.uv_layers] for o in bpy.data.objects if o.type=='MESH' and o.name.startswith('SRC_')}
 uv_archive.write_text(json.dumps(uv_rows,separators=(',',':')))

# The baseline contains a hidden editable collection; reset it before regeneration.
bpy.ops.wm.read_factory_settings(use_empty=False)
runpy.run_path(str(ROOT/'build_station.py'),run_name='__main__')
after=snapshot();new_contract=json.loads((ROOT/'model_contract.json').read_text())
changes={k:[field for field in before[k] if before[k][field]!=after.get(k,{}).get(field)] for k in before}
changes={k:v for k,v in changes.items() if v}
wall_prefix=('SRC_Walnut_side_lining','SRC_Walnut_back_lining','SRC_Studio_corridor_partition','SRC_Lobby_back_wall')
def allowed(k,fields):
 if k=='SM_1420_Interior':return True
 if k.startswith(wall_prefix):return fields==['uv']
 if k=='SRC_Lobby_notice' or k.startswith('SRC_Lobby_notice.'):return set(fields)<= {'uv','slots'}
 return False
invalid={k:v for k,v in changes.items() if not allowed(k,v)}
preserved_contract={k:all(old_contract['parts'][n][k]==new_contract['parts'][n][k] for n in old_contract['parts']) for k in ['bounds_local_m','origin']}
slot_prefix_preserved=new_contract['parts']['SM_1420_Interior']['materials'][:17]==old_contract['parts']['SM_1420_Interior']['materials'][:17]
report={'status':'PASS' if not invalid and all(preserved_contract.values()) and slot_prefix_preserved else 'FAIL','changed_existing':changes,'unexpected_changes':invalid,'new_source_objects':sorted(set(after)-set(before)),'contract_preserved':preserved_contract,'existing_17_material_slots_preserved':slot_prefix_preserved,'before':before,'after':after,'triangle_diff':{n:[old_contract['parts'][n]['triangles'],new_contract['parts'][n]['triangles']] for n in old_contract['parts']}}
(ROOT/'r9_preservation.json').write_text(json.dumps(report,indent=2))
print('R9_PRESERVATION',json.dumps({k:v for k,v in report.items() if k not in ('before','after')}))
assert report['status']=='PASS', 'R9 unexpected preservation failure; see r9_preservation.json'
