"""Write explicit, unexecuted own-source/import/runtime job plans. No process is launched."""
import argparse
import hashlib
import json
from pathlib import Path


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def dump(path,value):path.write_text(json.dumps(value,indent=2)+'\n')


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--project',type=Path,required=True,help='Built integrated Unreal project directory, not necessarily this thin worktree')
    ap.add_argument('--run-root',type=Path,required=True,help='Fresh private plan/evidence root')
    ap.add_argument('--departure',type=Path,help='Actual PASS SC12 sibling departure-checkpoint.disk.json')
    ap.add_argument('--blender',type=Path,default=Path.home()/'.local/bin/blender')
    ap.add_argument('--editor',type=Path,default=Path.home()/'.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd')
    a=ap.parse_args();project=a.project.resolve();out=a.run_root.resolve();assert not out.exists()
    assert(project/'Signal47.uproject').is_file();out.mkdir(parents=True)
    source=out/'source-r1';steps=[]
    def step(name,argv,timeout,expected,env=None):
        steps.append(dict(name=name,command=[str(x)for x in argv],cwd=str(project),timeout=timeout,expected=expected,env=env or {}))
    step('finale-source-r1',[a.blender,'--background','--factory-startup','--python',project/'Tools/finalesky/build_source.py','--','--output',source],120,'S47 FINALE SKY SOURCE REOPEN PASS')
    # Root builds the integrated editor with the own read-only assets command before these phases.
    import_report=out/'import-r1.json';fresh_report=out/'fresh-r1.json';binding=out/'binding-verified.json'
    for mode,report in [('apply',import_report),('verify',fresh_report)]:
        job=dict(schema=1,mode=mode,source=str(source),report=str(report),import_report=str(import_report),binding=str(binding))
        path=out/(mode+'-job.json');dump(path,job)
        args=[a.editor,project/'Signal47.uproject','-run=pythonscript','-script='+str(project/'Tools/finalesky/import_assets.py'),
              '-unattended','-nullrhi','-NoSound','-nosplash','-NoLiveCoding','-stdout','-FullStdOutLogOutput','-UTF8Output',
              '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0','-UserDir=/tmp/S47-finale-'+mode+'-'+out.name]
        step('finale-'+mode+'-r1',args,300,'S47 FINALE SKY '+mode.upper()+' PASS',{'S47_FINALE_JOB':str(path)})
    runtime=[]
    if a.departure:
        departure=a.departure.resolve();assert departure.is_file()and(departure.parent/'result.json').is_file()
        dump(out/'sc12-input-pins.json',{str(p):dict(bytes=p.stat().st_size,sha256=sha(p))for p in (departure,departure.parent/'result.json')})
        for mode,quality,comfort in [('full','high',False),('line','low',True),('legacy','high',False)]:
            args=[a.editor.with_name('UnrealEditor'),project/'Signal47.uproject','-game','-unattended','-nosplash','-windowed','-ResX=1280','-ResY=720',
                  '-S47Case=0','-S47EveningTimeline',*(['-S47Jump=chapter6:line']if mode=='legacy'else ['-S47Title']),'-S47SpineContract=sc13',
                  '-RenderOffscreen','-NoCrashDialog','-nocrashreports','-S47SmokeTest','-S47Seconds='+('1250'if mode=='full'else '400'),'-ExecCmds=t.MaxFPS 60',
                  '-S47SC13Mode='+mode,'-S47Quality='+quality,'-S47SC13Departure='+str(departure),'-S47SelfTest',
                  '-UserDir=/tmp/S47-sc13-'+mode+'-'+out.name,'-stdout','-FullStdOutLogOutput','-UTF8Output']
            if comfort:args+=['-S47SC13Comfort']
            runtime.append(dict(name='sc13-'+mode+'-'+quality,command=[str(v)for v in args],cwd=str(project),timeout=1320 if mode=='full'else 450,
                                expected='S47: spine contract sc13 PASS COMPLETE: '+mode,scope='complete13.1km road'if mode=='full'else 'declared last-line fixture only'))
    files=[p for p in(project/'Tools/finalesky').glob('*')if p.is_file()]
    files+=[project.parent/'ArtSource/FinaleSky/source-contract.json']
    dump(out/'unexecuted-plan.json',dict(schema=1,status='UNEXECUTED',source_import_steps=steps,runtime_steps=runtime,
        mandatory_before_native='Root builds integrated editor including S47.FinaleSkyAssets. These argv are not a build/guard receipt.',
        mandatory_before_runtime='Root requires outer0 + import/fresh PASS, then copies verified binding to Content/WebData/finalesky/binding.json. No runtime acceptance is inferred.',
        guard='One heavy job at a time, through Scripts/unreal_guard.py require-idle and root shared lease. This script never launches.',
        source_inputs={str(p):dict(bytes=p.stat().st_size,sha256=sha(p))for p in files}))
    print(out/'unexecuted-plan.json')


if __name__=='__main__':main()
