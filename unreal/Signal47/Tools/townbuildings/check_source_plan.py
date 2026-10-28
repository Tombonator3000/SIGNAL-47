"""Light system-Python modelling gates; no Blender, render, array-library or native launch."""
import sys,json,math
from pathlib import Path
R=Path(__file__).resolve().parent;sys.path.insert(0,str(R))
import town_geometry,motel_geometry,importlib.util
TB2={}
for family in ['Sheriff','Laundry','Feed','Cinema','Church']:
 spec=importlib.util.spec_from_file_location('tb2_'+family.lower(),R/'tb2'/(family.lower()+'.py'));TB2[family]=importlib.util.module_from_spec(spec);spec.loader.exec_module(TB2[family])
from design import PARTS,BUILDINGS,REVISION
from materials import material_rows
checks=[];stats={};materials={r['name']for r in material_rows()}|{r['name']for p in sorted((R/'tb2').glob('materials_*.json'))for r in json.loads(p.read_text())}
for lod in range(3):
 out={**{k:v for f in TB2.values()for k,v in f.build(lod).items()},**motel_geometry.build(lod)};assert set(out)==set(PARTS);stats[str(lod)]={}
 for name,m in out.items():
  minimum=math.inf;minuv=math.inf;tris=0
  for f,uv in zip(m.f,m.uv):
   for k in range(1,len(f)-1):
    a,b,c=[m.v[f[v]]for v in [0,k,k+1]];u,v,w=[uv[v]for v in [0,k,k+1]];cross=town_geometry.cross(town_geometry.sub(b,a),town_geometry.sub(c,a));area=sum(v*v for v in cross)**.5*.5;ua=abs((v[0]-u[0])*(w[1]-u[1])-(v[1]-u[1])*(w[0]-u[0]))*.5
    assert area>=5e-9,(name,lod,'unstable area',area);assert ua>1e-12,(name,lod,'collapsed UV',ua);minimum=min(minimum,area);minuv=min(minuv,ua);tris+=1
  assert set(m.mat)<=materials,(name,set(m.mat)-materials)
  stats[str(lod)][name]={'planned_triangles':tris,'minimum_raw_triangle_area':minimum,'minimum_raw_UV_area':minuv,'raw_bounds_local_m':[[fn(p[k]for p in m.v)for k in range(3)]for fn in [min,max]],'materials':sorted(set(m.mat)),**PARTS[name]}
report={'status':'SOURCE_PLAN_PASS_EXPORT_UNVERIFIED','source_revision':REVISION,'scope':'Light mathematical geometry/UV/material and coordinate gates only; Blender triangulation/weld/export/native are separate','parts':stats['0'],'lods':stats,'lod_totals':{l:sum(v['planned_triangles']for v in d.values())for l,d in stats.items()},'building_contract':BUILDINGS}
(R/'source_plan.json').write_text(json.dumps(report,indent=2)+'\n');print('TOWN_SOURCE_PLAN_PASS',report['lod_totals'])
