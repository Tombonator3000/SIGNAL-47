#!/usr/bin/env python3
"""Chapter journey through native input. No game actions are injected via telemetry."""
import argparse, importlib.util, json, time, hashlib, os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("legacy_journey",ROOT/"Automation/gauntlet-user-journey.py")
legacy=importlib.util.module_from_spec(spec);spec.loader.exec_module(legacy)
if os.environ.get('SIGNAL47_OBSERVER_DIR'):legacy.OUT=Path(os.environ['SIGNAL47_OBSERVER_DIR'])
state,wait_for=legacy.state,legacy.wait_for

class PhysicalDesktop(legacy.Desktop):
 def move_pointer(self,x,y):
  # Unity can render 1280x800 into a larger fullscreen XWayland surface.
  # Convert GUI render pixels to actual native client pixels before injection.
  C=legacy.C;D=C.c_void_p;U=C.c_ulong;I=C.c_int;N=C.c_uint
  self.x.XGetGeometry.argtypes=[D,U,C.POINTER(U),C.POINTER(I),C.POINTER(I),C.POINTER(N),C.POINTER(N),C.POINTER(N),C.POINTER(N)]
  root=U();a=I();b=I();w=N();h=N();border=N();depth=N()
  assert self.x.XGetGeometry(self.d,self.w,C.byref(root),C.byref(a),C.byref(b),C.byref(w),C.byref(h),C.byref(border),C.byref(depth))
  s=state();super().move_pointer(x*w.value/s['width'],y*h.value/s['height'])

def chapter(s=None):return json.loads((s or state()).get("chapterState") or "{}")
def frames(s=None):return json.loads((s or state()).get("cameraState") or "{}").get("frames",[])

class ChapterJourney(legacy.Journey):
 def __init__(self,d,shots=True):super().__init__(d,shots,True,True)
 def exterior(self):
  s=state();self.d.click(s["width"]/2,s["height"]/2+66)
  wait_for(lambda s:s["yardActive"] and not s["title"]);self.mark("chapter-begins-after-prologue")
  self.walk(6.8,3.2);self.walk(8,3.5)
  self.interact(8.25,.965,4.95,"COLLECT FIELD CAMERA")
  wait_for(lambda s:s["cameraAcquired"]);self.mark("physical-camera-collected")
  self.d.tap("c");wait_for(lambda s:s["cameraRaised"]);before=state()["rejectedFrames"];self.d.tap("space")
  wait_for(lambda s:s["rejectedFrames"]>before);assert state()["frameCount"]==0
  self.d.tap("c");self.mark("premature-exposure-rejected")
  self.walk(8,3.2);self.interact(9.35,1.15,3.2,"OPEN SERVICE")
  wait_for(lambda s:s["doorOpen"]);self.walk(10.7,3.2);self.walk(10.7,-2);self.walk(10.7,-7.8);self.walk(10.7,-12.2)
  self.interact(11.9,1.1,-13.2,"BUS S-03")
  wait_for(lambda s:s["yardComplete"] and s["modal"]);self.mark("first-controller-record")
  self.d.tap("Escape");n=state()["evidence"]
  self.interact(11.9,1.1,-13.2,"BUS S-03");assert state()["evidence"]==n;self.d.tap("Escape")
  self.aim(10.7,.3,-13);self.d.tap("c");before=state()["rejectedFrames"];self.d.tap("space")
  wait_for(lambda s:s["rejectedFrames"]>before);assert state()["frameCount"]==0;self.mark("wrong-subject-rejected")
  self.aim(8,4.8,-34);wait_for(lambda s:s["frameProblem"]=="");self.mark("first-exposure-framed")
  self.d.tap("space");wait_for(lambda s:s["frameCount"]==1 and s["photoReady"],20)
  assert state()["developedFrames"]==0
  frame=frames()[0];assert Path(frame["path"]).is_file();self.mark("immutable-undeveloped-frame")
  wait_for(lambda s:s["saveExists"] and not s["saving"],20)
  self.mark("checkpoint-before-development")
 def return_to_lab(self):
  if -7.4 < state()["z"] < 7.6 and abs(state()["x"]) < 9.5:
   for x,z in [(3.2,2.1),(0,6.5),(0,9.5),(0,10.3)]:self.walk(x,z)
   return
  if state()['z']>7.6 and abs(state()['x'])<4:
   self.walk(0,10.3);return
  if state()['z'] < -15:self.walk(10.7,-15.3)
  for x,z in [(10.7,-12.2),(10.7,-7.8),(10.7,-2),(10.7,3.2),(8,3.2),(6,5.7),(0,6.5),(0,9.5),(0,10.3)]:self.walk(x,z)
  self.aim(-2,1,11.5);self.mark("walked-into-photolab")
 def click_panel(self,x,y):
  s=state();scale=min(s['width']/1280,s['height']/800)
  self.d.click((s['width']-1280*scale)/2+x*scale,(s['height']-800*scale)/2+y*scale)
 def close_panel(self):
  if state()['modal']:self.d.tap('Escape')
  wait_for(lambda s:not s['modal'])
 def develop(self,number,pause_test=False):
  self.interact(-2.06,1.08,11.5,'FILM');wait_for(lambda s:s['chapterPanel']=='lab');self.mark('physical-film-processing-'+str(number))
  self.click_panel(640,487);wait_for(lambda s:chapter(s)['labStep']==1)
  if pause_test:
   self.close_panel();self.d.tap('Escape');s=wait_for(lambda s:s['paused']);remaining=chapter(s)['labRemaining']
   time.sleep(1);assert abs(chapter()['labRemaining']-remaining)<.02,'Processing continued while paused'
   self.mark('pause-freezes-development');self.d.tap('Escape');wait_for(lambda s:not s['paused'])
   self.interact(-2.06,1.08,11.5,'PRINT');wait_for(lambda s:s['chapterPanel']=='lab')
  wait_for(lambda s:chapter(s)['labRemaining']<=0,8);self.click_panel(640,487)
  wait_for(lambda s:chapter(s)['labStep']==2);wait_for(lambda s:chapter(s)['labRemaining']<=0,8)
  self.click_panel(640,487);wait_for(lambda s:s['developedFrames']==number)
  assert frames()[number-1]['developed'];self.mark('developed-print-'+str(number))
 def mark_photo(self,number):
  frame=frames()[number-1];outer=(55,149,810,438) if number==1 else (650,153,566,337)
  x,y,w,h=outer;x+=9;y+=9;w-=18;h-=18;aspect=frame['width']/frame['height']
  if w/h>aspect:nw=h*aspect;x+=(w-nw)/2;w=nw
  else:nh=w/aspect;y+=(h-nh)/2;h=nh
  uv=frame['referenceViewport'];self.click_panel(x+uv['x']*w,y+(1-uv['y'])*h)
  key='clueMarked' if number==1 else 'secondClueMarked';wait_for(lambda s:chapter(s)[key]);self.mark('marked-visible-reference-'+str(number))
 def interpret(self,method):
  if not chapter()['clueMarked']:self.mark_photo(1)
  self.close_panel();self.walk(0.25,10.65)
  self.interact(2.06,1.10,11.5,'FIELD RECORDS');wait_for(lambda s:s['chapterPanel']=='archive');self.mark('physical-reference-sheet')
  self.click_panel(910,551);assert not chapter()['archiveMatched'];self.mark('wrong-reference-gives-recoverable-feedback')
  self.click_panel(342,551);wait_for(lambda s:chapter(s)['archiveMatched'])
  before=chapter()['wrongHypotheses'];self.click_panel(1002,542);wait_for(lambda s:chapter(s)['wrongHypotheses']>before);assert not chapter()['hypothesis'];self.mark('motor-command-hypothesis-rejected')
  self.click_panel(246 if method=='passive' else 624,542);wait_for(lambda s:bool(chapter(s)['hypothesis']));self.mark('testable-hypothesis-'+method);self.close_panel()
 def visit_control(self,method):
  for x,z in [(0,9.5),(0,6.5),(6,5.7),(8,3.2),(10.7,3.2),(10.7,-2),(10.7,-7.8),(10.7,-12.2),(10.7,-15.3),(10.75,-18.9)]:self.walk(x,z)
  self.aim(10,1.6,-22);self.mark('new-field-point-before-control')
  self.interact(11.66,1.02,-20.10,'B-12');wait_for(lambda s:s['chapterPanel']=='experiment')
  self.click_panel(345 if method=='passive' else 907,512);wait_for(lambda s:chapter(s)['experimentMethod']==method and not s['chapterModal']);time.sleep(2)
  self.aim(10,1.6,-22);self.mark('physical-control-changes-world-'+method)
  self.interact(11.66,1.02,-20.10,'OBSERVATION');wait_for(lambda s:s['chapterPanel']=='experiment');self.click_panel(640,612)
  wait_for(lambda s:chapter(s)['controlObserved'] and not s['chapterModal']);self.mark('direct-observation-filed')
  self.aim(10,1.6,-22);self.d.tap('c');wait_for(lambda s:s['cameraRaised'] and s['frameProblem']=='');self.mark('second-exposure-framed')
  self.d.tap('space');wait_for(lambda s:s['frameCount']==2 and s['photoReady'],20)
  assert state()['developedFrames']==1 and frames()[1]['method']==method
  wait_for(lambda s:s['saveExists'] and not s['saving'],20);self.mark('checkpoint-after-control-exposure')
 def conclude(self):
  self.mark_photo(2);before=chapter()['wrongConclusions'];self.click_panel(236,614);wait_for(lambda s:chapter(s)['wrongConclusions']>before)
  assert not chapter()['comparisonConfirmed'];self.mark('wrong-light-conclusion-recoverable')
  self.click_panel(620,614);wait_for(lambda s:chapter(s)['wrongConclusions']>before+1)
  self.click_panel(1020,614);wait_for(lambda s:chapter(s)['comparisonConfirmed']);self.mark('supported-photographic-comparison');self.close_panel()
  self.walk(.25,11.6);self.interact(2.06,1.09,12.52,'FILE LOCAL');wait_for(lambda s:s['chapterPanel']=='report');self.mark('physical-case-report')
  self.click_panel(640,610);wait_for(lambda s:s['chapterComplete'] and s['chapterPanel']=='ending');self.mark('chapter-completed')
  wait_for(lambda s:s['saveExists'] and not s['saving'],15)
 def quit(self):
  if not state()['started']:
   s=state();self.mark('normal-menu-quit');self.d.click(s['width']/2+75,s['height']/2+188);return
  self.close_panel()
  if not state()['paused']:self.d.tap('Escape')
  s=wait_for(lambda s:s['paused']);self.mark('normal-save-and-quit');self.d.click(s['width']/2+62,s['height']/2+168)
 def resume(self):
  self.d.focus();s=state();assert not s["started"]
  self.d.move_pointer(s["width"]/2,s["height"]/2+138);load_started=time.monotonic()
  self.d.click(s["width"]/2,s["height"]/2+138)
  wait_for(lambda s:s["started"] and not s["restoring"],30)
  load_seconds=time.monotonic()-load_started
  self.mark("continued-in-new-process")
  self.events[-1]['continueObservedSeconds']=load_seconds
  self.events[-1]['continueTimingScope']='Ready-player observation after native Continue click; includes 0.42s input settling and <=0.1s observation polling. Explicit scene reload, outside continuous gameplay frame measurement.'
 def inspect_visual10(self):
  self.close_panel();self.return_to_lab();self.walk(0,8.0);self.aim(0,1.60,13.4);self.mark('visual10-lab-entrance')
  self.walk(0,10.3);self.aim(-2.7,1.3,11.4);self.mark('visual10-lab-wet-bench')
  self.aim(2.7,1.3,11.3);self.mark('visual10-lab-archive')
  self.aim(0,1.0,13.4);self.mark('visual10-lab-sink')
  self.aim(-.6,.03,11.9);self.mark('visual10-floor-reflection-detail')
  for x,z in [(0,6.5),(6,5.7),(8,3.2),(10.7,3.2),(10.7,-7.8),(10.7,-12.2),(10.7,-18.7)]:self.walk(x,z)
  self.walk(11.35,-18.0);self.aim(10.8,1.10,-21.4);self.mark('visual10-b12-overview')
  self.aim(11.67,.85,-20.40);self.mark('visual10-b12-cabinet')
  self.walk(10.7,-19.35);self.aim(10,1.6,-22);self.mark('visual10-b12-reference')
 def inspect_rooms(self):
  self.return_to_lab();self.walk(0,9.5);self.aim(0,1.4,12.5);self.mark('photolab-overview')
  self.walk(0,10.3);self.aim(-2.7,1.3,11.4);self.mark('photolab-enlarger-and-trays')
  self.aim(2.7,1.3,11.3);self.mark('photolab-archive-workplace')
  for x,z in [(0,9.5),(0,6.5),(0,3.2),(0,2.1)]:self.walk(x,z)
  self.aim(0,1.5,-15);self.mark('control-room-overview-after-light-correction')
  self.walk(-4.2,4.3);self.walk(-4.2,2.8);self.aim(-6.3,1.35,1.6);self.mark('receiver-restored-powered-leds')
  for x,z in [(-4.2,4.3),(0,3.2),(3.3,1.85),(4.6,1.85)]:self.walk(x,z)
  self.aim(5.1,1.08,.5);self.mark('phone-desk-after-light-correction')
 def settings(self,restore=False,keep_fullscreen=False):
  s=state();assert not s['started']
  if restore:
   expected=json.loads(Path(s['savePath']).with_name('ui-test-settings-expected.json').read_text())
   assert abs(s['masterVolume']-expected['masterVolume'])<.00001 and abs(s['mouseSensitivity']-expected['mouseSensitivity'])<.00001 and s['fullscreen']==expected['fullscreen'],'Settings did not survive process restart'
   self.mark('audio-and-mouse-settings-survived-restart')
  if not s['settingsOpen']:self.d.click(s['width']/2-77,s['height']/2+188)
  wait_for(lambda s:s['settingsOpen']);self.mark('settings-open-without-starting-case')
  for key,target,lo,hi,offset in ([] if restore else [('masterVolume',.37,0,1,125),('mouseSensitivity',.14,.02,.25,205)]):
   desired=(target-lo)/(hi-lo)
   for _ in range(7):
    s=state();left=s['width']/2-220;y=s['height']/2-170+offset+5
    self.d.drag(left+5+(s[key]-lo)/(hi-lo)*430,y,left+5+max(0,min(1,desired))*430,y)
    if abs(state()[key]-target)<(hi-lo)*.04:break
    desired+=(target-state()[key])/(hi-lo)
   else:raise RuntimeError('Settings slider failed: '+key)
  assert not state()['started'],'Settings click started a new game underneath overlay'
  if not restore:
   s=state();window=(s['width'],s['height']);self.d.click(s['width']/2-210,s['height']/2+85);wait_for(lambda s:s['fullscreen'],8);time.sleep(.8);self.mark('fullscreen-applied')
   s=state();self.d.click(s['width']/2-210,s['height']/2+85);wait_for(lambda s:not s['fullscreen'] and (s['width'],s['height'])==window,8);self.mark('fullscreen-menu-click-returns-to-previous-window-size')
   if keep_fullscreen:
    s=state();self.d.click(s['width']/2-210,s['height']/2+85);wait_for(lambda s:s['fullscreen'],8);time.sleep(.8)
  if restore:
   s=state();self.d.click(s['width']/2-123,s['height']/2+163);wait_for(lambda s:s['masterVolume']==1 and abs(s['mouseSensitivity']-.085)<.00001 and not s['fullscreen'])
  s=state();self.d.click(s['width']/2+106,s['height']/2+163);wait_for(lambda s:not s['settingsOpen']);self.mark('settings-saved')
  settings=json.loads(Path(state()['savePath']).with_name('settings.json').read_text())
  assert abs(settings['masterVolume']-state()['masterVolume'])<.001 and abs(settings['mouseSensitivity']-state()['mouseSensitivity'])<.001
  if not restore:
   Path(state()['savePath']).with_name('ui-test-settings-expected.json').write_text(json.dumps({k:state()[k] for k in ('masterVolume','mouseSensitivity','fullscreen')}))
   s=state();self.d.click(s['width']/2,s['height']/2+81);wait_for(lambda s:s['started']);before=state()['yaw'];sensitivity=state()['mouseSensitivity'];self.d.look_delta(100,0);time.sleep(.2)
   moved=abs(legacy.angle(state()['yaw']-before));assert abs(moved-sensitivity*100)<.5,(moved,sensitivity);self.mark('saved-sensitivity-changes-real-mouse-look')
 def invalid_save(self):
  s=state();assert not s['started'] and s['saveExists'];self.d.click(s['width']/2,s['height']/2+138)
  time.sleep(1);assert not state()['started'] and not state()['restoring'];assert 'cannot' in state()['saveStatus'].lower() or 'failed' in state()['saveStatus'].lower() or 'unreadable' in state()['saveStatus'].lower()
  self.mark('invalid-save-rejected-with-readable-status')
 def new_case_preserves_exports(self):
  assert state()['chapterComplete'];p=Path(state()['savePath']).parent/'FieldPhotos';before={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in p.iterdir() if f.is_file()}
  self.d.tap('Escape');s=wait_for(lambda s:s['paused']);self.d.click(s['width']/2,s['height']/2+64)
  wait_for(lambda s:not s['started']);assert state()['frameCount']==0 and state()['evidence']==0 and not state()['chapterComplete'] and not state()['saveExists']
  after={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in p.iterdir() if f.is_file()};assert before==after
  self.mark('new-case-resets-progress-preserves-export-archive')
 def failed_save_exit(self):
  path=Path(state()['savePath']);before=hashlib.sha256(path.read_bytes()).hexdigest();assert state()['chapterComplete']
  self.quit();wait_for(lambda s:s['canQuitWithoutSaving'] and not s['quitPending'],8)
  assert state()['paused'] and state()['chapterComplete'];assert hashlib.sha256(path.read_bytes()).hexdigest()==before;self.mark('write-failure-keeps-live-case-and-old-checkpoint')
  s=state();self.d.click(s['width']/2,s['height']/2+300)
 def recover_second(self):
  assert state()['frameCount']==1 and state()['developedFrames']==1 and state()['chapterStage']=='second-exposure';self.mark('missing-photo-reports-recoverable-second-exposure')
  # Completed fixtures may resume at the inspected phone desk or inside the lab.
  # Follow the real aisle/door approach; a diagonal through the jamb is not a route.
  approach=[(0,9.5),(0,6.5)] if state()['z']>7.5 else [(3.3,1.85),(0,3.2),(0,6.5)]
  for x,z in approach+[(6,5.7),(8,3.2),(10.7,3.2),(10.7,-2),(10.7,-7.8),(10.7,-12.2),(10.7,-15.3),(10.75,-18.9)]:self.walk(x,z)
  self.aim(10,1.6,-22);self.d.tap('c');wait_for(lambda s:s['cameraRaised'] and not s['frameProblem']);self.d.tap('space')
  wait_for(lambda s:s['frameCount']==2 and s['photoReady'],20);assert len({f['id'] for f in frames()})==2;self.mark('missing-photo-retaken-with-stable-evidence-identity')
 def premature_control(self):
  self.walk(10.7,-15.3);self.walk(10.75,-18.9);self.interact(11.66,1.02,-20.10,'B-12');wait_for(lambda s:s['chapterPanel']=='experiment')
  self.click_panel(345,512);assert not chapter()['experimentMethod'] and not chapter()['controlObserved'] and state()['developedFrames']==0;self.mark('premature-control-requires-an-investigation-question');self.close_panel()
 def reopen_evidence(self):
  self.close_panel();wait_for(lambda s:not s['saving'],15)
  payload=json.loads(json.loads(Path(state()['savePath']).read_text())['payload'])
  evidence=json.loads(payload['notebook'])['evidence'];count=state()['evidence']
  before={f['id']:hashlib.sha256(Path(f['path']).read_bytes()).hexdigest() for f in frames()}
  for cycle in range(2):
   for identity,panel in [('s03-field-photograph','first'),('b12-control-photograph','comparison')]:
    index=next(i for i,e in enumerate(evidence) if e['Id']==identity)
    self.d.tap('Tab');wait_for(lambda s:s['modal']);s=state();self.d.click(s['width']/2,162+index*48)
    wait_for(lambda s:s['chapterPanel']==panel);self.mark('notebook-reopened-'+identity+'-'+str(cycle+1))
    if cycle==0 and panel=='first':
     self.click_panel(1177,170);self.mark('contact-print-loupe-enlarged')
     self.d.drag(1066,211,1155,211);self.mark('contact-print-loupe-panned')
    self.close_panel()
  assert state()['evidence']==count and state()['frameCount']==2
  after={f['id']:hashlib.sha256(Path(f['path']).read_bytes()).hexdigest() for f in frames()}
  assert before==after;self.mark('reopening-and-loupe-preserve-originals-without-duplicates')
 def package_start(self):
  s=state();assert not s['started'];self.mark('extracted-package-normal-start-menu')
  self.d.click(s['width']/2,s['height']/2+81);wait_for(lambda s:s['started']);self.mark('extracted-package-enters-night-shift')
  before=state()['z'];self.d.key('w',True);time.sleep(.5);self.d.key('w',False);time.sleep(.2);assert abs(state()['z']-before)>.1;self.mark('extracted-package-receives-native-movement')

if __name__=="__main__":
 parser=argparse.ArgumentParser();parser.add_argument("--resume",action="store_true");parser.add_argument("--lab",action="store_true");parser.add_argument("--no-shots",action="store_true");parser.add_argument('--current',action='store_true');parser.add_argument('--phase',choices=['first','control','finish','all','inspect','visual'],default='first');parser.add_argument('--method',choices=['passive','active'],default='passive');parser.add_argument('--quit',action='store_true');parser.add_argument('--measure',action='store_true')
 parser.add_argument('--check',choices=['settings-write','settings-fullscreen-write','settings-read','invalid-save','new-case','backup','failed-save-exit','missing-second','package-start']);parser.add_argument('--inspect-after',action='store_true');parser.add_argument('--out-of-order',action='store_true');parser.add_argument('--reopen',action='store_true')
 args=parser.parse_args();d=PhysicalDesktop();d.inject_setup();j=ChapterJourney(d,not args.no_shots)
 outcome="FAIL";error=""
 try:
  d.focus()
  if args.resume:j.resume()
  elif not args.current and not args.check:j.run()
  if args.check:
   if args.check.startswith('settings-'):j.settings(args.check=='settings-read',args.check=='settings-fullscreen-write')
   elif args.check=='invalid-save':j.invalid_save()
   elif args.check=='new-case':j.new_case_preserves_exports()
   elif args.check=='backup':
    assert state()['chapterComplete'] and 'recovery copy' in state()['saveStatus'];j.mark('corrupt-primary-restored-from-backup')
   elif args.check=='failed-save-exit':j.failed_save_exit()
   elif args.check=='missing-second':j.recover_second()
   elif args.check=='package-start':j.package_start()
  if args.out_of_order:j.premature_control()
  if args.measure:time.sleep(20);legacy.command('record');measurement_start=time.monotonic()
  if args.lab or args.phase in ('control','finish','all'):j.return_to_lab()
  if args.phase=='inspect':j.inspect_rooms()
  if args.phase=='visual':j.inspect_visual10()
  if args.phase in ('control','all'):
   if state()['developedFrames']<1:j.develop(1,not args.measure)
   j.interpret(args.method);j.visit_control(args.method)
  if args.phase in ('finish','all'):
   if args.phase=='all':j.return_to_lab()
   j.develop(2);j.conclude()
  if args.inspect_after:j.close_panel();j.inspect_rooms()
  if args.reopen:j.reopen_evidence()
  if args.measure:
   while time.monotonic()-measurement_start<121:time.sleep(1)
   legacy.command('finish');time.sleep(.4)
  if args.quit and args.check!='failed-save-exit':j.quit()
  outcome="PASS"
 except Exception as e:error=str(e);print(error,flush=True)
 finally:
  d.close();out=legacy.OUT/"chapter09-result.json"
  if args.measure:legacy.command('finish')
  out.write_text(json.dumps({"outcome":outcome,"error":error,"resume":args.resume,"phase":args.phase,"branch":args.method,"measured":args.measure,"method":"Native uinput keyboard/mouse, observation-only telemetry; no teleport/game method actions. Marker positions derived from read-only camera metadata; visual legibility requires separate human/critic image inspection.","events":j.events},indent=2))
 if outcome!="PASS":raise SystemExit(1)
