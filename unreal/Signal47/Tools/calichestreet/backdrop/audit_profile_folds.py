"""Light exact source-row monotonicity audit. No Blender, meshes or textures."""
from pathlib import Path
import math,json,importlib.util,datetime,argparse
import geology as current
R=Path(__file__).resolve().parent
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
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('--compare',type=Path,help='Optional explicit rejected geology.py; historical archives are not a production dependency')
ap.add_argument('--out',type=Path,help='Optional new JSON report; existing reports are preserved')
args=ap.parse_args();fixed=audit(current)
assert all(r['reversed_or_zero_intervals']==0 and r['minimum_projected_profile_width_m']>1e-4 for r in fixed),fixed
result={'status':'PASS','utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'current_source':fixed,'scope':'Exact source rows and split-profile intervals only; no Blender, FBX or runtime performed'}
if args.compare:
    spec=importlib.util.spec_from_file_location('rejected_geology',args.compare);old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
    result['comparison_source']=str(args.compare);result['comparison_rows']=audit(old)
if args.out:
    with args.out.open('x') as stream:stream.write(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
