"""Sequential source generation, only after root grants the single heavy slot."""
from pathlib import Path
import sys,json,os
R=Path(__file__).resolve().parent;sys.path.insert(0,str(R/'../../Scripts'))
from unreal_guard import run_owned
runtime=Path.home()/'.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3'
def main():
 env=dict(os.environ,OMP_NUM_THREADS='2',OPENBLAS_NUM_THREADS='2')
 jobs=[('textures',[str(runtime),'make_textures.py']),('build',['blender','-b','--threads','2','--python-exit-code','1','--python','build_substation.py']),('verify',['blender','-b','--threads','2','--python-exit-code','1','--python','verify_substation.py'])]
 for label,command in jobs:
  result=run_owned(command,R/(label+'.log'),timeout=300,cwd=str(R),env=env);(R/('dcc-'+label+'-guard.json')).write_text(json.dumps(result,indent=2)+'\n')
  assert result['returncode']==0,(label,result)
 print('SUBSTATION_SOURCE_EXPORT_VERIFICATION_PASS')
if __name__=='__main__':main()
