"""Light exact source-row monotonicity audit. No Blender, meshes or textures."""
from pathlib import Path
import math,json,importlib.util,datetime
import geology as current
R=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('rejected_geology',R/'evidence/cn3-uv2-rejected-folds/geology.py');old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
def audit(module):
    result=[]
    for b in module.BELTS:
        profile=module.split_profile(b['profile']);n=math.ceil((b['along'][1]-b['along'][0])/b['step']);minimum=math.inf;bad=0;worst=None
        for i in range(n+1):
            s=b['along'][0]+(b['along'][1]-b['along'][0])*i/n
            previous=None
            for j,(d,f,m) in enumerate(profile):
                p=module.surface_point(b,s,d,f);lateral=b['side']*(p[0] if b['axis']=='z' else p[1])
                if previous is not None:
                    width=lateral-previous
                    if width<minimum:minimum=width;worst={'row':i,'profile':j,'along_m':s,'setback_m':d,'fraction':f}
                    if width<=0:bad+=1
                previous=lateral
        result.append({'belt':b['id'],'minimum_projected_profile_width_m':minimum,'reversed_or_zero_intervals':bad,'worst':worst})
    return result
previous=audit(old);fixed=audit(current)
assert sum(r['reversed_or_zero_intervals'] for r in previous)>0,'Need reproduce actual source failure'
assert all(r['reversed_or_zero_intervals']==0 and r['minimum_projected_profile_width_m']>1e-4 for r in fixed),fixed
result={'status':'SOURCE_PROFILE_FOLD_CAUSE_REPRODUCED_AND_FIXED','utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'rejected_source':previous,'corrected_source':fixed,'scope':'Exact source rows and split-profile intervals only; no Blender or FBX performed','triangulation_implication':'Strict front-to-back ordering at each along row makes projected quads convex trapezoids; either Blender diagonal has strictly positive area after consistent winding.'}
(R/'profile_fold_fix.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
