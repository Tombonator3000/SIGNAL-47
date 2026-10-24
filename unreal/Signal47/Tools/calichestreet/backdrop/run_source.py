"""Run the owned source sequence ONLY after explicit shared DCC-slot handoff.

The repository Unreal guard monitors contention and terminates only the owned
process session. External Blender scheduling is coordinated by the parent.
No rendering or Unreal execution is performed by this script.
"""
from pathlib import Path
import json,os,sys
R=Path(__file__).resolve().parent
sys.path.insert(0,str(R/'../../../Scripts'))
from unreal_guard import run_owned

def main():
    runtime=Path.home()/'.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3'
    env=dict(os.environ,OMP_NUM_THREADS='2',OPENBLAS_NUM_THREADS='2')
    jobs=[('texture',[str(runtime),'texture_geology.py']),
          ('build',['blender','-b','--threads','2','--python-exit-code','1','--python','build_backdrop.py']),
          ('verify',['blender','-b','--threads','2','--python-exit-code','1','--python','verify_backdrop.py'])]
    for name,command in jobs:
        result=run_owned(command,R/(name+'.log'),timeout=300,cwd=str(R),env=env)
        (R/('dcc-'+name+'-guard.json')).write_text(json.dumps(result,indent=2)+'\n')
        if result['returncode']!=0:raise RuntimeError(name+' failed; inspect '+name+'.log')
    print('CALICHE_GEOLOGY_GENERATION_EXPORT_VERIFICATION_PASS')
if __name__=='__main__':main()
