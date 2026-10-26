"""Small, deterministic geometry/UV/anchor gates; no Blender or native launch."""
import json
import math
from pathlib import Path
import geometry
from design import APERTURES,COVERAGE,EXCLUDED,HUT,PARTS,PROTECTED_RUNTIME,REVISION,ROOT_THREE
from make_sources import rows

R=Path(__file__).resolve().parent
known={r['name'] for r in rows()}
stats={};checks=0
for lod in range(3):
    shapes=geometry.build(lod);assert set(shapes)==set(PARTS)
    stats[str(lod)]={}
    for name,m in shapes.items():
        tris=0;minimum=math.inf;minuv=math.inf
        for face,uv in zip(m.f,m.uv):
            for k in range(1,len(face)-1):
                a,b,c=[m.v[face[i]] for i in [0,k,k+1]]
                u,v,w=[uv[i] for i in [0,k,k+1]]
                cross=geometry.cross(geometry.sub(b,a),geometry.sub(c,a))
                area=sum(t*t for t in cross)**.5*.5
                ua=abs((v[0]-u[0])*(w[1]-u[1])-(v[1]-u[1])*(w[0]-u[0]))*.5
                assert area>=5e-9,(name,lod,m.tags[m.f.index(face)],'small triangle',area)
                assert ua>1e-12,(name,lod,'collapsed UV',ua)
                minimum=min(minimum,area);minuv=min(minuv,ua);tris+=1
        assert set(m.mat)<=known,(name,set(m.mat)-known)
        checks+=5
        stats[str(lod)][name]={'planned_triangles':tris,'bounds_local_m':[[fn(p[k] for p in m.v) for k in range(3)] for fn in [min,max]],'materials':sorted(set(m.mat)),'minimum_raw_triangle_area':minimum,'minimum_raw_UV_area':minuv,**PARTS[name]}
assert APERTURES['south_door']['x']==[-10.5,-9.4]
assert HUT['height']==2.6
for name in PARTS:
    assert stats['0'][name]['planned_triangles']>=stats['1'][name]['planned_triangles']>=stats['2'][name]['planned_triangles'],name
    checks+=1
totals={lod:sum(v['planned_triangles'] for v in data.values()) for lod,data in stats.items()}
assert totals['0']<45000,totals
report={'status':'SOURCE_PLAN_PASS_EXPORT_UNVERIFIED','source_revision':REVISION,'checks':checks,'parts':stats['0'],'lods':stats,'lod_totals':totals,'site_root_three_m':ROOT_THREE,'apertures':APERTURES,'coverage':COVERAGE,'excluded_runtime_objects':EXCLUDED,'protected_runtime':PROTECTED_RUNTIME,'scope':'Pure-Python geometric area, UV, materials, LOD and metre-coordinate checks. Actual Blender export and native visuals are separate.'}
(R/'source_plan.json').write_text(json.dumps(report,indent=2)+'\n')
print('FIELDSTATION_SOURCE_PLAN_PASS',checks,totals)
