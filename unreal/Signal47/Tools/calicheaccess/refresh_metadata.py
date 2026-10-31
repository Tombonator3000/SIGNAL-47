"""Lightweight furniture collision refinement after import; does not touch generated meshes.
python refresh_metadata.py SOURCE_JSON CONTENT_ASSEMBLY_JSON
"""
import ast,importlib.util,json,sys
from pathlib import Path
R=Path(__file__).resolve().parent;TB=R.parent/'townbuildings';sys.path.insert(0,str(TB))
from design import BUILDINGS
parsed=ast.parse((R/'build_source.py').read_text());fn=next(n for n in parsed.body if isinstance(n,ast.FunctionDef)and n.name=='furniture_colliders');scope={};exec(compile(ast.Module(body=[fn],type_ignores=[]),str(R/'build_source.py'),'exec'),scope)
rows={}
for family in ['Sheriff','Laundry','Feed','Cinema','Church']:
 spec=importlib.util.spec_from_file_location('refresh_'+family,TB/'tb2'/(family.lower()+'.py'));module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);data=module.build(0);interior=next(m for n,m in data.items()if n.endswith('_Interior'));rows[family]=scope['furniture_colliders'](interior,BUILDINGS[family]['root'])
for filename in sys.argv[1:]:
 p=Path(filename);j=json.loads(p.read_text())
 for b in j['buildings']:
  if b['id']in rows:b['furniture']=rows[b['id']]
  if b['id']=='Laundry':b['interact']='laundromat'
 p.write_text(json.dumps(j,indent=2)+'\n')
print('ACCESS_METADATA_REFINED',{k:len(v)for k,v in rows.items()})
