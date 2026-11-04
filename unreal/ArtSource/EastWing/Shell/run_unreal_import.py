"""Run the shell importer in the existing shared production lease and idle guard."""
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
    parser.add_argument('--source',type=Path,default=HERE/'exports')
    parser.add_argument('--output',type=Path,default=Path.home()/'scratch-bm/Signal47-eastwing-props-20261103/shell-native-import')
    args=parser.parse_args()
    source=args.source.expanduser().resolve()
    output=args.output.expanduser().resolve()
    engine=Path(os.environ.get('UE_ROOT',str(Path.home()/'.local/opt/UnrealEngine-5.8.4')))
    editor=engine/'Engine/Binaries/Linux/UnrealEditor-Cmd'
    command=[str(editor),str(PROJECT/'Signal47.uproject'),'-run=pythonscript',
             '-script='+str(HERE/'import_unreal.py'),'-unattended','-nop4','-nosplash','-NullRHI','-NoSound',
             '-UserDir='+str(output/'profile'),'-EnablePlugins=GeometryScripting,ProceduralMeshComponent',
             '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0']
    print(json.dumps({'command':command,'source':str(source),'report':str(output/'import.json'),'apply':args.apply},indent=2))
    if not args.apply:return 0
    if not editor.is_file() or not (source/'shell-manifest.json').is_file():
        raise RuntimeError('Editor or generated shell manifest is missing')
    if shutil.disk_usage(PROJECT).free<110_000_000_000:
        raise RuntimeError('Shell import must leave the 100 GB disk reserve plus its 10 GB allowance')
    report=output/'import.json'
    if report.exists():raise RuntimeError('Use a fresh output directory; do not overwrite an earlier import result')
    output.mkdir(parents=True,exist_ok=True)
    with HeavyReservation('EASTWING-01 native shell import') as lease:
        os.environ.pop('S47_INSTALLED_GAME_GRACE',None)
        env=os.environ.copy()
        env.update(lease.environment())
        os.environ.update(lease.environment())
        env.update(S47_EASTWING_SHELL_APPLY='1',S47_EASTWING_SHELL_OUT=str(source),S47_EASTWING_SHELL_REPORT=str(report))
        result=run_owned(command,output/'editor.log',timeout=1800,env=env,cwd=PROJECT)
        (output/'process.json').write_text(json.dumps(result,indent=2)+'\n')
    if result['returncode']!=0 or result['stop_reason']:return 1
    data=json.loads(report.read_text()) if report.exists() else {}
    return 0 if data.get('status')=='PASS_NATIVE_SHELL_IMPORT' else 1


if __name__=='__main__':sys.exit(main())
