"""Regenerate the legacy access derivatives and import them, under the shared lease and idle guard.
Blender writes FBX/manifest to a fresh private OUT; Unreal then runs import_legacy_access.py there.
"""
from pathlib import Path
import argparse
import json
import os
import shutil
import sys

HERE=Path(__file__).resolve().parent
PROJECT=HERE.parents[2]/'Signal47'
sys.path.insert(0,str(PROJECT/'Scripts'))
from production_lease import HeavyReservation
from unreal_guard import run_owned


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply',action='store_true')
    parser.add_argument('--output',type=Path,required=True,help='fresh private directory outside the repository')
    args=parser.parse_args()
    output=args.output.expanduser().resolve()
    source=output/'derivatives'
    engine=Path(os.environ.get('UE_ROOT',str(Path.home()/'.local/opt/UnrealEngine-5.8.4')))
    editor=engine/'Engine/Binaries/Linux/UnrealEditor-Cmd'
    blender=[str(Path.home()/'.local/bin/blender'),'-b','-t','4','--python-exit-code','1','--python',str(HERE/'build_legacy_access.py')]
    command=[str(editor),str(PROJECT/'Signal47.uproject'),'-run=pythonscript',
             '-script='+str(HERE/'import_legacy_access.py'),'-unattended','-nop4','-nosplash','-NullRHI','-NoSound',
             '-UserDir='+str(output/'profile'),'-EnablePlugins=GeometryScripting,ProceduralMeshComponent',
             '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0']
    print(json.dumps({'blender':blender,'command':command,'output':str(output),'apply':args.apply},indent=2))
    if not args.apply:return 0
    if output.is_relative_to(HERE.parents[2]):raise RuntimeError('Output must stay outside the repository')
    if output.exists():raise RuntimeError('Use a fresh output directory; do not overwrite an earlier result')
    if shutil.disk_usage(PROJECT).free<110_000_000_000:raise RuntimeError('Import must leave the 100 GB disk reserve')
    output.mkdir(parents=True)
    report=output/'import.json'
    with HeavyReservation('EASTWING-01 legacy access derivatives') as lease:
        os.environ.pop('S47_INSTALLED_GAME_GRACE',None)
        env=os.environ.copy();env.update(lease.environment());os.environ.update(lease.environment())
        env.update(S47_EASTWING_ACCESS_OUT=str(source))
        result=run_owned(blender,output/'blender.log',timeout=900,env=env,cwd=HERE)
        if result['returncode']!=0 or result['stop_reason']:return 1
        env.update(S47_EASTWING_ACCESS_APPLY='1',S47_EASTWING_ACCESS_REPORT=str(report))
        result=run_owned(command,output/'editor.log',timeout=1800,env=env,cwd=PROJECT)
        (output/'process.json').write_text(json.dumps(result,indent=2)+'\n')
    if result['returncode']!=0 or result['stop_reason']:return 1
    data=json.loads(report.read_text()) if report.exists() else {}
    print('ACCESS IMPORT',data.get('status'))
    return 0 if str(data.get('status','')).startswith('PASS') else 1


if __name__=='__main__':sys.exit(main())
