(() => {
'use strict';

// ---------- DOM ----------
const $ = id => document.getElementById(id);
const canvas = $('gl');
const startScreen = $('startScreen');
const startBtn = $('startBtn');
const reticle = $('reticle');
const interactEl = $('interact');
const hint = $('hint');
const toast = $('toast');
const paperOverlay = $('paperOverlay');
const paper = $('paper');
const notebookOverlay = $('notebookOverlay');
const notesEl = $('notes');
const pauseOverlay = $('pauseOverlay');
const resumeBtn = $('resumeBtn');
const restartPauseBtn = $('restartPauseBtn');
const crtOverlay = $('crtOverlay');
const spectrumCanvas = $('spectrum');
const sctx = spectrumCanvas.getContext('2d');
const freqInput = $('freq'), gainInput = $('gain'), bwInput = $('bw'), azInput = $('az');
const freqVal = $('freqVal'), gainVal = $('gainVal'), bwVal = $('bwVal'), azVal = $('azVal');
const manualEl = $('manual'), crtStatus = $('crtStatus'), readoutEl = $('readout');
const consoleAction = $('consoleAction'), consoleExit = $('consoleExit');
const titleScreen = $('titleScreen');
const restartBtn = $('restartBtn');
const signalAcquiredEl = $('signalAcquired');

// ---------- Graphics ----------
let graphics;
try { graphics = window.Signal47Graphics(canvas); }
catch (error) {
  startBtn.disabled = true;
  const warning=document.createElement('p'); warning.className='graphicsError';
  warning.textContent=error.message; document.getElementById('startCard').appendChild(warning);
  console.error(error); return;
}
const pendingTimers = new Set();
function later(fn, ms) { const id = setTimeout(() => { pendingTimers.delete(id); fn(); }, ms); pendingTimers.add(id); return id; }

// ---------- World/game state ----------
const player={x:0,y:1.68,z:6.7,yaw:0,pitch:0}; // yaw 0 looks toward the control room (-Z)
const keys={};
let gameStarted=false, modalOpen=false, currentInteract=null;
let phoneRingTimer=null, futureDeadline=0, phoneSequenceStart=0, mugDropStart=0, mugBreakTime=0, finalStart=0;
let dishFinal=0;
let notebook=[];
let toastTimer=null;
let audio=null, master=null;
let state={};

const interactables={
  clipboard:{pos:[-1.7,1.3,5.2],label:'SHIFT CLIPBOARD'},
  receiver:{pos:[-6.3,1.3,1.60],label:'RECEIVER BANK'},
  console:{pos:[0,1.50,-2.32],label:'RX CONTROL CONSOLE'},
  printer:{pos:[3.1,1.2,-1.98],label:'DOT-MATRIX PRINTER'},
  phone:{pos:[5.1,1.28,.5],label:'DESK PHONE'}
};
const obstacles=[
  {x1:-7.3,x2:-5.3,z1:-.2,z2:2.1},
  {x1:-3.2,x2:3.8,z1:-3.25,z2:-1.35},
  {x1:3.4,x2:6.2,z1:-.5,z2:1.5},
  {x1:-3.2,x2:-2.6,z1:3.3,z2:7.6},
  {x1:2.6,x2:3.2,z1:3.3,z2:7.6},
  {x1:-9.28,x2:-7.82,z1:-5.16,z2:-4.04},
  {x1:7.17,x2:8.63,z1:-4.76,z2:-3.64},
  {x1:-3.19,x2:-2.41,z1:-.35,z2:.45}
];

function resetState(){
  for(const id of pendingTimers)clearTimeout(id); pendingTimers.clear();
  toast.classList.remove('show');toast.textContent='';
  [paperOverlay,notebookOverlay,pauseOverlay,crtOverlay,interactEl].forEach(el=>el.classList.add('hidden'));
  for(const key of Object.keys(keys))keys[key]=false;
  modalOpen=false;currentInteract=null;
  state={receiver:false,consolePhase:'calibration',printout:false,phoneRinging:false,phoneAnswered:false,mugBroken:false,ending:false,title:false,signalAcquired:false,subtleA:false,subtleB:false};
  player.x=0;player.z=6.7;player.yaw=0;player.pitch=0;
  notebook=[]; updateNotes();
  futureDeadline=0; phoneSequenceStart=0; mugDropStart=0; mugBreakTime=0; finalStart=0; dishFinal=0;
  if(phoneRingTimer){clearInterval(phoneRingTimer);phoneRingTimer=null;}
  signalAcquiredEl.classList.add('hidden'); titleScreen.classList.remove('visible');
  readoutEl.classList.add('hidden'); readoutEl.textContent='';
  freqInput.value='1419.620'; gainInput.value='27'; bwInput.value='82'; azInput.value='18';
  setConsolePhase('calibration',true);
}
resetState();

function addNote(text){if(!notebook.includes(text)){notebook.push(text);updateNotes();}}
function updateNotes(){notesEl.innerHTML=notebook.length?notebook.map((n,i)=>`<div>${String(i+1).padStart(2,'0')} // ${n}</div>`).join(''):'<div class="empty">No observations recorded.</div>';}
function showToast(msg,ms=1900){toast.textContent=msg;toast.classList.add('show');clearTimeout(toastTimer);toastTimer=later(()=>toast.classList.remove('show'),ms);}

// ---------- Audio ----------
function ensureAudio(){
  if(audio) return;
  audio=new (window.AudioContext||window.webkitAudioContext)();
  master=audio.createGain(); master.gain.value=.6; master.connect(audio.destination);
  const hum=audio.createOscillator(),hg=audio.createGain();hum.frequency.value=58;hum.type='sine';hg.gain.value=.018;hum.connect(hg).connect(master);hum.start();
  const hum2=audio.createOscillator(),h2g=audio.createGain();hum2.frequency.value=116;hum2.type='sine';h2g.gain.value=.006;hum2.connect(h2g).connect(master);hum2.start();
  const crt=audio.createOscillator(),cg=audio.createGain();crt.frequency.value=7450;crt.type='sine';cg.gain.value=.0017;crt.connect(cg).connect(master);crt.start();
}
function tone(freq,dur=.12,gain=.05,type='sine',delay=0){if(!audio)return;const o=audio.createOscillator(),g=audio.createGain();o.type=type;o.frequency.value=freq;g.gain.setValueAtTime(0,audio.currentTime+delay);g.gain.linearRampToValueAtTime(gain,audio.currentTime+delay+.008);g.gain.exponentialRampToValueAtTime(.0001,audio.currentTime+delay+dur);o.connect(g).connect(master);o.start(audio.currentTime+delay);o.stop(audio.currentTime+delay+dur+.03);}
function noiseBurst(dur=.2,gain=.05,delay=0,cut=1800){if(!audio)return;const len=Math.max(1,Math.floor(audio.sampleRate*dur)),buf=audio.createBuffer(1,len,audio.sampleRate),a=buf.getChannelData(0);for(let i=0;i<len;i++)a[i]=(Math.random()*2-1)*Math.pow(1-i/len,.25);const src=audio.createBufferSource(),f=audio.createBiquadFilter(),g=audio.createGain();src.buffer=buf;f.type='lowpass';f.frequency.value=cut;g.gain.value=gain;src.connect(f).connect(g).connect(master);src.start(audio.currentTime+delay);}
function clickSfx(){tone(900,.035,.025,'square');}
function powerSfx(){tone(90,.4,.08,'sine');tone(420,.08,.03,'square',.34);}
function printerSfx(){for(let i=0;i<15;i++){noiseBurst(.05,.025,i*.07,3500);tone(1700+((i%3)*180),.025,.008,'square',i*.07);}tone(140,.1,.035,'square',1.08);}
function phoneRing(){tone(440,.35,.055,'sine');tone(480,.35,.045,'sine');tone(440,.35,.055,'sine',.48);tone(480,.35,.045,'sine',.48);}
function boom(delay=0){tone(42,1.25,.16,'sine',delay);noiseBurst(.9,.12,delay,260);}
function smash(delay=0){for(let i=0;i<9;i++){tone(1450+((i*1987+471)%3500),.09+(i%5)*.028,.025+(i%4)*.008,'triangle',delay+i*.018);noiseBurst(.08,.025,delay+i*.02,6500);}}
function futureCallAudio(){noiseBurst(4.2,.018,0,5000);tone(63,1.2,.018,'sine',.35);noiseBurst(.45,.028,.95,800);boom(1.75);smash(2.48);tone(310,.08,.025,'square',4.05);}
function startPhoneRinging(){if(state.phoneRinging||state.phoneAnswered)return;state.phoneRinging=true;phoneRing();phoneRingTimer=setInterval(phoneRing,1800);showToast('A telephone rings somewhere in the control room.',2600);}
function stopPhoneRinging(){if(phoneRingTimer){clearInterval(phoneRingTimer);phoneRingTimer=null;}state.phoneRinging=false;}

// ---------- Console minigame ----------
function values(){return {f:+freqInput.value,g:+gainInput.value,b:+bwInput.value,a:+azInput.value};}
function updateVals(){const v=values();freqVal.textContent=v.f.toFixed(3);gainVal.textContent=v.g.toFixed(0);bwVal.textContent=v.b.toFixed(0);azVal.textContent=String(v.a.toFixed(0)).padStart(3,'0')+'°';}
[freqInput,gainInput,bwInput,azInput].forEach(el=>el.addEventListener('input',()=>{updateVals();clickSfx();}));

function setConsolePhase(phase,silent=false){
  state.consolePhase=phase; readoutEl.classList.add('hidden'); readoutEl.textContent='';
  if(phase==='calibration'){
    crtStatus.textContent='CALIBRATION REQUIRED';consoleAction.textContent='LOG CALIBRATION';
    manualEl.textContent='TAPED REF CARD // CAL 1419.900 MHz · AZ 042° · GAIN 55 ±10 · BW 48 ±14';
    if(!silent){freqInput.value='1419.620';gainInput.value='27';bwInput.value='82';azInput.value='18';}
  } else if(phase==='interference'){
    crtStatus.textContent='LOCAL CARRIER DRIFT';consoleAction.textContent='NOTCH INTERFERENCE';
    manualEl.textContent='LOCAL SPIKE // center carrier near 1420.110 MHz. Narrow BW below 22 kHz while retaining nominal gain.';
  } else if(phase==='anomaly'){
    crtStatus.textContent='RESIDUAL CARRIER';consoleAction.textContent='ISOLATE PATTERN';
    manualEl.textContent='UNLOGGED RESIDUAL // weak peak near 1420.4 MHz. Increase gain, narrow bandwidth, then search azimuth manually.';
  } else if(phase==='isolated'){
    crtStatus.textContent='PATTERN LOCK';consoleAction.textContent='DIRECTION SOLVE';
    manualEl.textContent='PULSE TRAIN STABLE // grouping repeats 4 / 7. Direction solver now available.';
  } else if(phase==='solved'){
    crtStatus.textContent='OUTPUT ROUTED TO PRINTER';consoleAction.textContent='PRINT COMPLETE';consoleAction.disabled=true;
    manualEl.textContent='Solver returned a non-physical propagation solution.';
    readoutEl.classList.remove('hidden');
    readoutEl.textContent='SOURCE: UNKNOWN\nFREQ: 1420.405 MHz\nRA: 05h 17m 32s\nDEC: -05° 23\' 14"\nS/N: 4.71\nDISTANCE SOLVE: -39 LY';
  }
  if(phase!=='solved')consoleAction.disabled=false;
  updateVals();
}
function consolePass(){
  const v=values();
  if(state.consolePhase==='calibration') return Math.abs(v.f-1419.900)<=.025 && v.g>=45&&v.g<=65 && v.b>=34&&v.b<=62 && Math.abs(v.a-42)<=7;
  if(state.consolePhase==='interference') return Math.abs(v.f-1420.110)<=.03 && v.b<=22 && v.g>=40&&v.g<=72;
  if(state.consolePhase==='anomaly') return Math.abs(v.f-1420.405)<=.008 && v.g>=78 && v.b<=14 && Math.abs(v.a-83)<=4;
  return true;
}
function partialLock(){
  const v=values(); let n=0,total=4;
  if(state.consolePhase==='calibration'){n+=(Math.abs(v.f-1419.9)<=.025);n+=(v.g>=45&&v.g<=65);n+=(v.b>=34&&v.b<=62);n+=(Math.abs(v.a-42)<=7);}
  else if(state.consolePhase==='interference'){total=3;n+=(Math.abs(v.f-1420.110)<=.03);n+=(v.b<=22);n+=(v.g>=40&&v.g<=72);}
  else if(state.consolePhase==='anomaly'){n+=(Math.abs(v.f-1420.405)<=.008);n+=(v.g>=78);n+=(v.b<=14);n+=(Math.abs(v.a-83)<=4);}
  return `${n}/${total}`;
}
consoleAction.addEventListener('click',()=>{
  ensureAudio(); clickSfx();
  if(state.consolePhase==='solved')return;
  if(state.consolePhase==='isolated'){
    setConsolePhase('solved');state.printout=true;printerSfx();addNote('1420.405 MHz — repeating 4 / 7 pulse grouping.');addNote('Direction solve returned -39 LY.');
    later(startPhoneRinging,3600);return;
  }
  if(!consolePass()){
    crtStatus.textContent=`NO LOCK // ${partialLock()} PARAMETERS`;tone(140,.12,.035,'square');return;
  }
  tone(950,.08,.035,'square');tone(1250,.08,.025,'square',.09);
  if(state.consolePhase==='calibration'){
    addNote('Receiver calibration completed at 23:4x local.');setConsolePhase('interference');
    freqInput.value='1419.98';bwInput.value='58';gainInput.value='55';azInput.value='42';updateVals();
  } else if(state.consolePhase==='interference'){
    addNote('Local carrier interference rejected.');setConsolePhase('anomaly');
    freqInput.value='1420.18';gainInput.value='53';bwInput.value='28';azInput.value='48';updateVals();
    later(()=>tone(473,.08,.018,'sine'),700);
  } else if(state.consolePhase==='anomaly'){
    setConsolePhase('isolated');addNote('Unlogged narrowband carrier isolated near hydrogen line.');
  }
});

function drawSpectrum(){
  if(crtOverlay.classList.contains('hidden'))return;
  const w=spectrumCanvas.width,h=spectrumCanvas.height,v=values(),phase=state.consolePhase;
  sctx.fillStyle='#020704';sctx.fillRect(0,0,w,h);
  sctx.strokeStyle='rgba(86,206,111,.14)';sctx.lineWidth=1;
  for(let x=0;x<w;x+=75){sctx.beginPath();sctx.moveTo(x,0);sctx.lineTo(x,h);sctx.stroke();}
  for(let y=0;y<h;y+=35){sctx.beginPath();sctx.moveTo(0,y);sctx.lineTo(w,y);sctx.stroke();}
  const mapF=f=>((f-1419.5)/(1420.7-1419.5))*w;
  const bwPix=Math.max(4,(v.b/100)*190); const fx=mapF(v.f);
  sctx.fillStyle='rgba(80,255,115,.055)';sctx.fillRect(fx-bwPix/2,0,bwPix,h);
  const baseY=h*.72;
  sctx.strokeStyle='#7dff9a';sctx.lineWidth=1.4;sctx.beginPath();
  for(let x=0;x<w;x++){
    let y=baseY+(Math.sin(x*.17+performance.now()*.004)*2)+(Math.sin(x*.043)*3)+(Math.random()-0.5)*7;
    const peaks=[];
    if(phase==='calibration')peaks.push([1419.900,46]);
    if(phase==='interference'||phase==='anomaly'||phase==='isolated'||phase==='solved')peaks.push([1420.110,58]);
    if(phase==='anomaly'||phase==='isolated'||phase==='solved')peaks.push([1420.405, phase==='anomaly'?25:48]);
    peaks.forEach(([pf,amp])=>{const px=mapF(pf);const dx=(x-px)/16;y-=amp*Math.exp(-dx*dx);});
    if(x===0)sctx.moveTo(x,y);else sctx.lineTo(x,y);
  }
  sctx.stroke();
  if(phase==='isolated'||phase==='solved'){
    sctx.fillStyle='rgba(122,255,153,.8)';
    const yy=30; const unit=7; let xx=24;
    [4,7,4,7].forEach((count,idx)=>{for(let i=0;i<count;i++){sctx.fillRect(xx,yy,3,22);xx+=unit;}xx+=idx%2?24:14;});
    sctx.font='14px Courier New';sctx.fillText('PULSE GROUP // 4   7   4   7',24,72);
  }
  sctx.fillStyle='#75c986';sctx.font='12px Courier New';
  sctx.fillText('1419.500',8,h-8);sctx.fillText('1420.100',w/2-38,h-8);sctx.fillText('1420.700',w-75,h-8);
}

function openConsole(){
  if(!state.receiver){showToast('No carrier. Receiver bank has no power.');tone(120,.1,.025,'square');return;}
  modalOpen=true;crtOverlay.classList.remove('hidden');document.exitPointerLock?.();updateVals();drawSpectrum();
}
function closeConsole(){crtOverlay.classList.add('hidden');modalOpen=false;showToast('Click the room to resume mouse-look.',1400);}
consoleExit.addEventListener('click',closeConsole);

// ---------- Paper/notebook/pause ----------
function openPaper(kind){
  modalOpen=true;document.exitPointerLock?.();paperOverlay.classList.remove('hidden');
  if(kind==='clipboard')paper.innerHTML=`<div class="paperhead">SARO // NIGHT SHIFT LOG</div>SHIFT: 23:30 — 07:30\nLOCAL: 23:41\n\n[ ] POWER RECEIVER BANK 3\n[ ] CALIBRATE ORION OBSERVATION\n[ ] IDENTIFY / NOTCH LOCAL INTERFERENCE\n\n WEATHER: dry / distant electrical activity\n ARRAY: scheduled track nominal\n\n\n“Night’s yours. Don’t break anything. —R.”<div class="paperclose">E / ESC — close</div>`;
  else paper.innerHTML=`<div class="paperhead">SARO // DIRECTION SOLVE OUTPUT</div>SOURCE: UNKNOWN\nFREQ: 1420.405 MHz\nRA: 05h 17m 32s\nDEC: -05° 23' 14"\nS/N: 4.71\n\nDISTANCE SOLVE:  -39 LY\n\n*** RANGE SIGN ERROR ***\n*** RE-RUN REQUIRED ***<div class="paperclose">E / ESC — close</div>`;
}
function closePaper(){paperOverlay.classList.add('hidden');modalOpen=false;}
function toggleNotebook(force){
  if(!gameStarted)return;
  const open=force!==undefined?force:notebookOverlay.classList.contains('hidden');
  if(open){modalOpen=true;document.exitPointerLock?.();notebookOverlay.classList.remove('hidden');updateNotes();}
  else {notebookOverlay.classList.add('hidden');modalOpen=false;}
}
paperOverlay.addEventListener('click',e=>{if(e.target===paperOverlay)closePaper();});
notebookOverlay.addEventListener('click',e=>{if(e.target===notebookOverlay)toggleNotebook(false);});

function showPause(){if(!gameStarted||modalOpen||state.title)return;modalOpen=true;pauseOverlay.classList.remove('hidden');}
function closePause(){pauseOverlay.classList.add('hidden');modalOpen=false;requestLook();}
resumeBtn.addEventListener('click',closePause);
restartPauseBtn.addEventListener('click',()=>{pauseOverlay.classList.add('hidden');modalOpen=false;restartGame();});

// ---------- Interaction ----------
function interact(id){
  ensureAudio();
  if(id==='clipboard'){openPaper('clipboard');return;}
  if(id==='receiver'){
    if(!state.receiver){state.receiver=true;powerSfx();addNote('Receiver bank 3 powered on.');showToast('RECEIVER BANK 3 // ONLINE');}
    else showToast('Receiver bank already online.');
  } else if(id==='console'){openConsole();}
  else if(id==='printer'){
    if(state.printout)openPaper('printout'); else showToast('Printer idle. No queued output.');
  } else if(id==='phone'){
    if(state.phoneRinging){answerPhone();} else if(state.phoneAnswered){showToast('No dial tone.');noiseBurst(.18,.018,0,2500);} else showToast('Internal line. No call.');
  }
}
function answerPhone(){
  stopPhoneRinging();state.phoneAnswered=true;addNote('Desk phone received an unlogged incoming call.');
  showToast('The handset carries only room tone and static…',2800);futureCallAudio();phoneSequenceStart=performance.now();
  later(()=>{
    tone(260,.05,.025,'square');futureDeadline=performance.now()+47000;addNote('Call audio contained a heavy impact and ceramic break.');
    showToast('The line goes dead.',1500);
  },4300);
}

// ---------- Input ----------
canvas.addEventListener('click',()=>{if(gameStarted&&!modalOpen&&!state.title)requestLook();});
document.addEventListener('pointerlockchange',()=>{
  const locked=document.pointerLockElement===canvas;
  reticle.classList.toggle('hidden',!locked);hint.classList.toggle('hidden',!gameStarted||modalOpen);
});
document.addEventListener('mousemove',e=>{
  if(document.pointerLockElement!==canvas||modalOpen)return;
  player.yaw-=e.movementX*.00235;player.pitch-=e.movementY*.0022;player.pitch=Math.max(-1.15,Math.min(1.15,player.pitch));
});
document.addEventListener('keydown',e=>{
  if(e.code==='Tab'){e.preventDefault(); if(!modalOpen)toggleNotebook(true); else if(!notebookOverlay.classList.contains('hidden'))toggleNotebook(false);return;}
  if(e.code==='Escape'){
    if(!paperOverlay.classList.contains('hidden')){closePaper();return;}
    if(!notebookOverlay.classList.contains('hidden')){toggleNotebook(false);return;}
    if(!crtOverlay.classList.contains('hidden')){closeConsole();return;}
    if(!pauseOverlay.classList.contains('hidden')){closePause();return;}
    later(showPause,0);return;
  }
  if(e.code==='KeyE'){
    if(!paperOverlay.classList.contains('hidden')){closePaper();return;}
    if(!crtOverlay.classList.contains('hidden')){closeConsole();return;}
    if(currentInteract&&!modalOpen)interact(currentInteract);
  }
  keys[e.code]=true;
});
document.addEventListener('keyup',e=>keys[e.code]=false);

// ---------- Start/restart ----------
startBtn.addEventListener('click',()=>{
  ensureAudio(); if(audio.state==='suspended')audio.resume();
  startScreen.classList.add('fadeout');later(()=>startScreen.classList.add('hidden'),900);
  gameStarted=true;modalOpen=false;hint.classList.remove('hidden');
  showToast('SHIFT LOG // 23:41 LOCAL',2000);later(()=>requestLook(),300);
});
restartBtn.addEventListener('click',restartGame);
function restartGame(){
  resetState();gameStarted=true;modalOpen=false;titleScreen.classList.remove('visible');
  later(()=>{requestLook();showToast('SHIFT LOG // 23:41 LOCAL',1600);},250);
}

// ---------- Movement/collision ----------
function canMove(x,z){
  const r=.34;if(x<-9.35+r||x>9.35-r||z<-7.45+r||z>7.55-r)return false;
  for(const o of obstacles)if(x>o.x1-r&&x<o.x2+r&&z>o.z1-r&&z<o.z2+r)return false;
  return true;
}
function updateMovement(dt){
  if(document.pointerLockElement!==canvas||modalOpen||state.title)return;
  const fw=(keys.KeyW?1:0)-(keys.KeyS?1:0), rt=(keys.KeyD?1:0)-(keys.KeyA?1:0); if(!fw&&!rt)return;
  const len=Math.hypot(fw,rt)||1, speed=3.15; const f=fw/len,r=rt/len;
  const fx=-Math.sin(player.yaw),fz=-Math.cos(player.yaw); // yaw 0 => -Z
  const rx=Math.cos(player.yaw),rz=-Math.sin(player.yaw);
  const dx=(fx*f+rx*r)*speed*dt,dz=(fz*f+rz*r)*speed*dt;
  if(canMove(player.x+dx,player.z))player.x+=dx;if(canMove(player.x,player.z+dz))player.z+=dz;
}
function updateInteraction(){
  if(!gameStarted||modalOpen||document.pointerLockElement!==canvas){currentInteract=null;interactEl.classList.add('hidden');return;}
  const fwd=[-Math.sin(player.yaw)*Math.cos(player.pitch),Math.sin(player.pitch),-Math.cos(player.yaw)*Math.cos(player.pitch)];
  let best=null,bscore=.88;
  for(const [id,o] of Object.entries(interactables)){
    const dx=o.pos[0]-player.x,dy=o.pos[1]-player.y,dz=o.pos[2]-player.z,dist=Math.hypot(dx,dy,dz);if(dist>2.65)continue;
    const dot=(dx*fwd[0]+dy*fwd[1]+dz*fwd[2])/dist;const score=dot-(dist*.015);
    if(score>bscore){bscore=score;best=id;}
  }
  currentInteract=best;
  if(best){interactEl.textContent=`E — ${interactables[best].label}`;interactEl.classList.remove('hidden');}else interactEl.classList.add('hidden');
}

// ---------- Script timing ----------
function updateScript(now){
  if(futureDeadline && !state.mugBroken){
    const elapsed=47000-(futureDeadline-now);
    if(elapsed>16000&&!state.subtleA){state.subtleA=true;tone(73,.7,.012,'sine');}
    if(elapsed>31000&&!state.subtleB){state.subtleB=true;noiseBurst(.35,.012,0,900);}
    if(now>=futureDeadline && !mugDropStart){boom();mugDropStart=now;showToast('A heavy impact rolls across the desert.',1700);}
    if(mugDropStart && now-mugDropStart>=700 && !state.mugBroken){state.mugBroken=true;mugBreakTime=now;smash();addNote('Coffee mug broke immediately after a distant impact.');}
  }
  if(state.mugBroken && !state.ending && now-mugBreakTime>3000){state.ending=true;finalStart=now;tone(61,2.2,.035,'sine');}
  if(state.ending && !state.title){
    const t=(now-finalStart)/3200;dishFinal=Math.min(1,Math.max(0,t));
    if(t>.42&&!state.signalAcquired){state.signalAcquired=true;signalAcquiredEl.classList.remove('hidden');tone(520,.1,.025,'square');tone(780,.1,.02,'square',.13);}
    if(t>1.35){state.title=true;document.exitPointerLock?.();later(()=>titleScreen.classList.add('visible'),500);}
  }
}

// ---------- Main loop ----------
let last=performance.now();
function frame(now){
  const dt=Math.min(.035,(now-last)/1000); last=now;
  updateMovement(dt); updateInteraction(); updateScript(now); drawSpectrum();
  graphics.render(now,player,state,{dishFinal,mugDropStart});
  requestAnimationFrame(frame);
}
requestAnimationFrame(frame);
const exposureInput=document.getElementById('exposure');
exposureInput.addEventListener('input',()=>graphics.setExposure(+exposureInput.value));
const reducedInput=document.getElementById('reducedEffects');
reducedInput.addEventListener('change',()=>{ graphics.setReduced(reducedInput.checked); document.body.classList.toggle('reduced',reducedInput.checked); });
function requestLook(){ if(!canvas.requestPointerLock)return; try { const p=canvas.requestPointerLock(); if(p&&p.catch)p.catch(()=>showToast('Click the room to enable mouse-look.')); } catch (_) { showToast('Click the room to enable mouse-look.'); } }
window.addEventListener('blur',()=>{for(const k of Object.keys(keys))keys[k]=false;});

// A tiny clock update on the CRT; game-time is intentionally not synchronized to the hidden 47s event.
setInterval(()=>{const s=Math.floor(performance.now()/1000)%60;$('crtClock').textContent=`23:41:${String(s).padStart(2,'0')}`;},1000);

})();
