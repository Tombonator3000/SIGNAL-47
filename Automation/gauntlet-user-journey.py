#!/usr/bin/env python3
"""Exercise the Linux player with native Linux uinput keyboard/mouse events.
Reads observation-only Unity telemetry; never edits game state or calls interactions.
"""
import argparse,ctypes as C,json,math,time,os,fcntl,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'Artifacts/Gauntlet'
class Desktop:
 def __init__(self):
  self.x=C.CDLL('libX11.so.6');self.t=C.CDLL('libXtst.so.6');self.d=None;self.held=set();self.devices=[]
  def bind(lib,name,args,result=C.c_int):
   f=getattr(lib,name);f.argtypes=args;f.restype=result;return f
  D=C.c_void_p;U=C.c_ulong;I=C.c_int
  bind(self.x,'XOpenDisplay',[C.c_char_p],D);bind(self.x,'XDefaultRootWindow',[D],U)
  bind(self.x,'XQueryTree',[D,U,C.POINTER(U),C.POINTER(U),C.POINTER(C.POINTER(U)),C.POINTER(C.c_uint)])
  bind(self.x,'XFetchName',[D,U,C.POINTER(C.c_char_p)]);bind(self.x,'XFree',[D]);bind(self.x,'XFlush',[D])
  bind(self.x,'XWarpPointer',[D,U,U,I,I,C.c_uint,C.c_uint,I,I]);bind(self.x,'XStringToKeysym',[C.c_char_p],U);bind(self.x,'XKeysymToKeycode',[D,U],C.c_uint)
  bind(self.x,'XSetInputFocus',[D,U,I,U]);bind(self.x,'XGetInputFocus',[D,C.POINTER(U),C.POINTER(I)])
  bind(self.x,'XQueryPointer',[D,U,C.POINTER(U),C.POINTER(U),C.POINTER(I),C.POINTER(I),C.POINTER(I),C.POINTER(I),C.POINTER(C.c_uint)]);bind(self.x,'XRaiseWindow',[D,U]);bind(self.x,'XTranslateCoordinates',[D,U,U,I,I,C.POINTER(I),C.POINTER(I),C.POINTER(U)])
  bind(self.t,'XTestFakeKeyEvent',[D,C.c_uint,I,U]);bind(self.t,'XTestFakeButtonEvent',[D,C.c_uint,I,U])
  bind(self.t,'XTestFakeMotionEvent',[D,I,I,I,U]);bind(self.t,'XTestFakeRelativeMotionEvent',[D,I,I,U])
  self.d=self.x.XOpenDisplay(None);assert self.d,'X11 display unavailable'
  self.root=self.x.XDefaultRootWindow(self.d)
  windows=[]
  def walk(w,depth):
   name=C.c_char_p()
   if self.x.XFetchName(self.d,w,C.byref(name)) and name.value:
    title=name.value.decode(errors='replace');self.x.XFree(C.cast(name,D))
    if title=='SIGNAL 47':windows.append(w)
   if depth<=0:return
   root,parent=U(),U();children=C.POINTER(U)();n=C.c_uint()
   if self.x.XQueryTree(self.d,w,C.byref(root),C.byref(parent),C.byref(children),C.byref(n)):
    for i in range(n.value):walk(children[i],depth-1)
    if children:self.x.XFree(children)
  walk(self.root,3);assert len(windows)==1,f'Expected one SIGNAL 47 window, found {windows}'
  self.w=windows[0]
 def inject_setup(self):
  for name,keys,relative in [('Signal47 Gauntlet keyboard',[1,15,17,18,30,31,32,46,57],False),('Signal47 Gauntlet mouse',[272],True)]:
   fd=os.open('/dev/uinput',os.O_WRONLY|os.O_NONBLOCK);self.devices.append(fd)
   fcntl.ioctl(fd,0x40045564,1)
   for k in keys:fcntl.ioctl(fd,0x40045565,k)
   if relative:
    fcntl.ioctl(fd,0x40045564,2);fcntl.ioctl(fd,0x40045566,0);fcntl.ioctl(fd,0x40045566,1)
   fcntl.ioctl(fd,0x405c5503,struct.pack('HHHH80sI',3,0x1209,0x4747,1,name.encode(),0));fcntl.ioctl(fd,0x5501)
  time.sleep(1)
 def emit(self,device,events):
  fd=self.devices[device]
  for kind,code,value in [*events,(0,0,0)]:os.write(fd,struct.pack('llHHi',0,0,kind,code,int(value)))
 def close(self):
  self.release()
  for fd in self.devices:fcntl.ioctl(fd,0x5502);os.close(fd)
  self.devices=[]
 def focus(self):
  # Ask the Wayland window manager to activate this XWayland client.
  class Msg(C.Structure):_fields_=[('type',C.c_int),('serial',C.c_ulong),('send',C.c_int),('display',C.c_void_p),('window',C.c_ulong),('message_type',C.c_ulong),('format',C.c_int),('data',C.c_long*5)]
  class Event(C.Union):_fields_=[('message',Msg),('pad',C.c_long*24)]
  self.x.XInternAtom.argtypes=[C.c_void_p,C.c_char_p,C.c_int];self.x.XInternAtom.restype=C.c_ulong
  self.x.XSendEvent.argtypes=[C.c_void_p,C.c_ulong,C.c_int,C.c_long,C.POINTER(Event)]
  e=Event();e.message.type=33;e.message.display=self.d;e.message.window=self.w;e.message.message_type=self.x.XInternAtom(self.d,b'_NET_ACTIVE_WINDOW',0);e.message.format=32;e.message.data[0]=2
  self.x.XSendEvent(self.d,self.root,0,(1<<20)|(1<<19),C.byref(e));self.x.XFlush(self.d);time.sleep(.7)
 def guard(self):
  f=C.c_ulong();r=C.c_int();self.x.XGetInputFocus(self.d,C.byref(f),C.byref(r));assert f.value==self.w,'Game lost native input focus; stopped to avoid typing into another app'
 def key(self,k,down):
  if down:self.guard()
  code={'Escape':1,'Tab':15,'w':17,'e':18,'a':30,'s':31,'d':32,'c':46,'space':57}[k]
  self.emit(0,[(1,code,int(down))])
  if down:self.held.add(k)
  else:self.held.discard(k)
 def tap(self,k):self.key(k,True);time.sleep(.07);self.key(k,False);time.sleep(.25)
 def move_pointer(self,x,y):
  self.guard()
  for _ in range(40):
   rr,cc=C.c_ulong(),C.c_ulong();a,b,lx,ly=C.c_int(),C.c_int(),C.c_int(),C.c_int();mask=C.c_uint()
   self.x.XQueryPointer(self.d,self.w,C.byref(rr),C.byref(cc),C.byref(a),C.byref(b),C.byref(lx),C.byref(ly),C.byref(mask))
   dx,dy=round(x)-lx.value,round(y)-ly.value
   if abs(dx)<=2 and abs(dy)<=2:break
   # Small closed-loop motions tolerate the compositor's pointer acceleration.
   self.emit(1,[(2,0,max(-120,min(120,dx*.45)) or (1 if dx>0 else -1) if dx else 0),(2,1,max(-120,min(120,dy*.45)) or (1 if dy>0 else -1) if dy else 0)]);time.sleep(.08)
  else:raise RuntimeError('Pointer could not reach client coordinates')
 def click(self,x,y):
  self.move_pointer(x,y);self.guard();self.emit(1,[(1,272,1)]);time.sleep(.12);self.emit(1,[(1,272,0)]);time.sleep(.3)
 def drag(self,x1,y1,x2,y2):
  self.move_pointer(x1,y1);self.guard();self.emit(1,[(1,272,1)]);time.sleep(.12)
  try:self.move_pointer(x2,y2)
  finally:self.emit(1,[(1,272,0)])
  time.sleep(.3)
 def look_delta(self,x,y):
  self.guard();self.emit(1,[(2,0,round(x)),(2,1,round(y))]);time.sleep(.12)
 def release(self):
  for k in list(self.held):self.key(k,False)
def state():
 for _ in range(30):
  try:return json.loads((OUT/'state.json').read_text())
  except (OSError,ValueError):time.sleep(.02)
 raise RuntimeError('Missing or incomplete game telemetry')
def command(value):
 p=OUT/'command.txt'
 for _ in range(30):
  if not p.exists():break
  time.sleep(.1)
 p.write_text(value)
def wait_for(fn,timeout=8):
 end=time.monotonic()+timeout
 while time.monotonic()<end:
  s=state()
  if fn(s):return s
  time.sleep(.1)
 raise RuntimeError('State wait timed out: '+json.dumps(state()))
def angle(v):return (v+180)%360-180
class Journey:
 def __init__(self,desktop,shots,yard=False,camera=False):self.d=desktop;self.shots=shots;self.yard=yard;self.camera=camera;self.events=[]
 def mark(self,name):
  s=state();self.events.append({'checkpoint':name,'wall':time.time(),'state':s});print(name,flush=True)
  if self.shots:
   command('shot');time.sleep(.6)
   images=list(OUT.glob('journey-*.png'))
   if images:self.events[-1]['screenshot']=max(images,key=lambda p:p.stat().st_mtime).name
 def aim(self,x,y,z):
  for _ in range(100):
   s=state();dx,dz=x-s['x'],z-s['z'];yaw=math.degrees(math.atan2(dx,dz));pitch=math.degrees(math.atan2(s['y']+1.63-y,math.hypot(dx,dz)))
   a,b=angle(yaw-s['yaw']),pitch-s['pitch']
   if abs(a)<.6 and abs(b)<.6:return
   self.d.look_delta(max(-180,min(180,a/.085)),max(-150,min(150,b/.085)))
  raise RuntimeError('Mouse look did not reach target')
 def walk(self,x,z):
  end=time.monotonic()+15;previous=state();stuck=time.monotonic()
  try:
   while time.monotonic()<end:
    s=state();distance=math.hypot(x-s['x'],z-s['z'])
    if distance<.24:return
    self.aim(x,s['y']+1.63,z)
    self.d.key('w',True);time.sleep(min(.25,max(.045,(distance-.18)/3.15)));self.d.key('w',False);time.sleep(.13)
    if math.hypot(s['x']-previous['x'],s['z']-previous['z'])>.07:previous=s;stuck=time.monotonic()
    if time.monotonic()-stuck>3:raise RuntimeError(f'Walking blocked on route to {x,z}: {s}')
   raise RuntimeError('Walking deadline')
  finally:self.d.key('w',False)
 def interact(self,x,y,z,prompt):
  self.aim(x,y,z);wait_for(lambda s:prompt in s['prompt']);self.d.tap('e')
 def slider(self,name,value,row,lo,hi,tolerance):
  s=state();scale=min(s['width']/960,s['height']/760);left=(s['width']/scale-880)/2;top=(s['height']/scale-700)/2
  unit=(value-lo)/(hi-lo)
  for _ in range(4):
   current=(state()[name]-lo)/(hi-lo);y=(top+380+row*42+14)*scale
   self.d.drag((left+310+5+current*520)*scale,y,(left+310+5+unit*520)*scale,y)
   actual=state()[name]
   if abs(actual-value)<tolerance:return
   if name=='frequency' and abs(actual-value)<.035:
    for _ in range(40):
     actual=state()[name]
     if abs(actual-value)<.0006:return
     self.d.click((left+(455 if value>actual else 355))*scale,(top+567)*scale)
    raise RuntimeError('Frequency fine adjustment did not converge')
   unit+=(value-actual)/(hi-lo)
  raise RuntimeError(f'Slider {name}: requested {value}, observed {actual}')
 def tune(self,frequency,gain,bandwidth,azimuth,stage):
  self.slider('frequency',frequency,0,1419.5,1420.7,.003)
  self.slider('gain',gain,1,0,100,2)
  self.slider('bandwidth',bandwidth,2,4,100,1.9)
  self.slider('azimuth',azimuth,3,0,180,2)
  self.action();wait_for(lambda s:s['stages']==stage);self.mark('tuned-stage-'+str(stage))
 def action(self):
  s=state();scale=min(s['width']/960,s['height']/760);left=(s['width']/scale-880)/2;top=(s['height']/scale-700)/2
  self.d.click((left+158)*scale,(top+657)*scale)
 def exterior(self):
  s=state();before=s['evidence'];self.d.click(s['width']/2,s['height']/2+66)
  wait_for(lambda s:s['yardActive'] and not s['title']);self.mark('service-investigation-start')
  if self.camera:
   self.walk(6.8,3.2);self.walk(8.0,3.5);self.aim(8.25,.965,4.95);self.mark('camera-on-shelf');self.interact(8.25,.965,4.95,'COLLECT FIELD CAMERA');wait_for(lambda s:s['cameraAcquired']);self.mark('camera-collected')
   self.d.tap('c');wait_for(lambda s:s['cameraRaised']);self.d.tap('space');assert not state()['photoTaken'];self.d.tap('c');self.mark('camera-rejects-before-motor-log')
  self.walk(6.8,3.2);self.walk(8.0,3.2);self.interact(9.35,1.15,3.2,'OPEN SERVICE');wait_for(lambda s:s['doorOpen']);self.mark('service-door-open')
  self.walk(10.7,3.2);assert state()['x']>9.6;self.aim(11,1,-13.2);self.mark('walked-outside')
  self.walk(10.7,-2);self.walk(10.7,-7.8);self.aim(12.1,1.1,-13.2);self.mark('service-path')
  self.walk(10.7,-12.2);self.interact(11.90,1.1,-13.2,'BUS S-03');wait_for(lambda s:s['yardComplete'] and s['evidence']==before+1 and s['modal']);self.mark('motor-log-filed');self.d.tap('Escape')
  self.interact(11.90,1.1,-13.2,'BUS S-03');assert state()['evidence']==before+1;self.d.tap('Escape');self.mark('motor-log-deduplicated')
  self.d.tap('Tab');wait_for(lambda s:s['modal']);self.mark('motor-log-in-notebook');self.d.tap('Escape')
  if self.camera:self.photograph()
  self.walk(10.7,-7.8);self.walk(10.7,-2);self.walk(10.7,3.2);self.walk(8,3.2)
  wait_for(lambda s:s['yardReturned']);self.mark('returned-to-control-room')
  if self.camera:
   self.open_photo();self.d.click(362,686);wait_for(lambda s:s['photoCompared']);self.mark('photo-log-compared');count=state()['observations'];self.d.click(362,686);assert state()['observations']==count;self.d.tap('Escape');self.d.tap('Escape');self.open_photo();assert state()['photoCompared'];self.mark('photo-reopened-after-comparison');self.d.tap('Escape');self.d.tap('Escape')
  self.d.tap('Escape');wait_for(lambda s:s['paused']);s=state();self.d.click(s['width']/2,s['height']/2+64)
  wait_for(lambda s:not s['started']);assert state()['evidence']==0 and not state()['yardActive'] and not state()['doorOpen'];self.mark('restart-clears-yard')
  if self.camera:assert not state()['cameraAcquired'] and not state()['photoTaken'] and not state()['photoCompared'];self.mark('restart-clears-camera-state')
 def open_photo(self):
  self.d.tap('Tab');wait_for(lambda s:s['modal']);s=state();self.d.click(s['width']/2,354);wait_for(lambda s:s['photoOpen'])
 def photograph(self):
  self.aim(10.7,.3,-13);self.d.tap('c');wait_for(lambda s:s['cameraRaised']);before=state()['rejectedFrames'];self.d.tap('space');wait_for(lambda s:s['rejectedFrames']>before);assert not state()['photoTaken'];self.mark('camera-rejects-wrong-subject')
  self.aim(8,4.8,-34);wait_for(lambda s:s['frameProblem']=='');self.mark('camera-valid-viewfinder');self.d.tap('space');wait_for(lambda s:s['photoTaken'] and s['photoSave']=='SAVED',15);self.mark('photo-captured-and-saved')
  photo=Path(state()['photoPath']);assert photo.is_file() and photo.stat().st_size>10000 and photo.with_suffix('.json').is_file()
  self.d.tap('c');wait_for(lambda s:s['cameraRaised']);count=state()['evidence'];self.d.tap('space');assert state()['evidence']==count;self.d.tap('c');self.mark('photograph-deduplicated')
  self.open_photo();self.mark('photograph-in-notebook');self.d.click(362,686);assert not state()['photoCompared'];self.mark('comparison-requires-control-room');self.d.tap('Escape');self.d.tap('Escape')
 def run(self):
  self.d.focus();s=state();assert not s['started'],'Start from a fresh player launch'
  self.mark('start');self.d.click(s['width']/2,s['height']/2+81);wait_for(lambda s:s['started'])
  self.interact(-1.7,.9,5.2,'READ SHIFT');wait_for(lambda s:s['evidence']==1);self.mark('read-shift-log');self.d.tap('Escape')
  self.walk(-4.2,4.3);self.walk(-4.2,2.8);self.interact(-6.3,1.35,1.6,'RECEIVER');wait_for(lambda s:s['powered']);self.mark('receiver-powered')
  self.walk(-3.2,2);self.walk(0,.6);self.interact(0,1.42,-2.32,'RX CONTROL');wait_for(lambda s:s['console'])
  self.tune(1419.9,55,48,42,1);self.tune(1420.11,55,12,42,2);self.tune(1420.405,82,12,83,3);self.action();wait_for(lambda s:s['printed']);self.d.tap('Escape')
  self.walk(3.1,1.2);self.walk(3.1,-.4);self.interact(3.1,1.18,-1.98,'READ PRINTOUT');wait_for(lambda s:s['evidence']==2);self.mark('read-physical-printout');self.d.tap('Escape')
  self.walk(3.3,1.85);self.walk(4.6,1.85);self.aim(5.1,1.08,.5);self.mark('phone-before-answer');self.interact(5.1,1.08,.5,'ANSWER DESK PHONE');wait_for(lambda s:s['answered']);self.mark('answered-phone');wait_for(lambda s:s['lineDead'])
  self.d.tap('Tab');wait_for(lambda s:s['modal']);self.mark('notebook');self.d.tap('Escape')
  self.d.tap('Escape');s=wait_for(lambda s:s['paused']);before=s['gameTime'];time.sleep(1);assert abs(state()['gameTime']-before)<.001;self.mark('pause-freezes-clock');self.d.tap('Escape');wait_for(lambda s:not s['paused'])
  self.walk(3.2,2.1);self.walk(0,2.1);self.aim(0,1.5,-15);self.mark('array-before-impact')
  wait_for(lambda s:s['impact'],60);wait_for(lambda s:s['title'],15);self.mark('ending-title')
  if self.yard:self.exterior();return
  s=state();self.d.click(s['width']/2,s['height']/2+119);wait_for(lambda s:not s['started']);assert state()['evidence']==0;self.mark('restart-clears-evidence')
if __name__=='__main__':
 args=argparse.ArgumentParser();args.add_argument('--measure',action='store_true');args.add_argument('--inspect',action='store_true');args.add_argument('--yard',action='store_true');args.add_argument('--camera',action='store_true');opt=args.parse_args()
 d=Desktop();print('Verified SIGNAL 47 X11 window:',d.w,flush=True)
 if opt.inspect:print(json.dumps(state(),indent=2));raise SystemExit()
 d.inject_setup();j=Journey(d,not opt.measure,opt.yard,opt.camera);begin=None;outcome='FAIL';error=''
 try:
  d.focus()
  if opt.measure:time.sleep(20);command('record');begin=time.monotonic();time.sleep(.3)
  j.run()
  if begin:
   # Include additional walking/turning after restart until a full 120 seconds.
   s=state();d.click(s['width']/2,s['height']/2+81);wait_for(lambda s:s['started'])
   while time.monotonic()-begin<121:
    j.walk(2.5,4.2);j.walk(-2.5,4.2)
   command('finish');time.sleep(.5)
  outcome='PASS'
 except Exception as e:error=str(e);print(error,flush=True)
 finally:
  d.close();command('finish');(OUT/'journey-result.json').write_text(json.dumps({'outcome':outcome,'error':error,'method':'Native Linux uinput keyboard and mouse input, XWayland focus/position checks; observation-only game telemetry; no teleports or direct game method calls','measured':opt.measure,'service_yard':opt.yard,'field_camera':opt.camera,'events':j.events},indent=2))
 if outcome!='PASS':raise SystemExit(1)
