// Actual Opening, ReadAloud, UI and main functions; controlled decoder/clock, no browser/GPU.
const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),crypto=require('node:crypto'),ts=require('typescript');
const base=require('./artbell-core.cjs');
let passed=0;const check=(ok,message)=>{assert.ok(ok,message);++passed;};
const flush=()=>new Promise(resolve=>setImmediate(resolve));
const load=(path,imports=()=>({}),globals={})=>{const exports={};vm.runInNewContext(base.compile(fs.readFileSync(path,'utf8')),{exports,require:imports,...globals});return exports;};
const {Opening,OPENING_PARAGRAPHS}=load('src/core/Opening.ts');
const catalog=JSON.parse(fs.readFileSync('src/assets/audio/serling-intro/catalog.json','utf8'));
const legacy=JSON.parse(fs.readFileSync('src/assets/audio/serling/catalog.json','utf8'));
const assetMap=(path,clips)=>Object.fromEntries(clips.map((clip,i)=>[path+clip.file,'data:audio/mpeg;base64,'+Buffer.from([i+1]).toString('base64')]));
let bankSource=fs.readFileSync('src/core/narrationBanks.ts','utf8')
 .replace(/const FILES = import\.meta\.glob[^\n]+;/,'const FILES = '+JSON.stringify(assetMap('../assets/audio/serling/',legacy.clips))+';')
 .replace(/const OPENING_FILES = import\.meta\.glob[^\n]+;/,'const OPENING_FILES = '+JSON.stringify(assetMap('../assets/audio/serling-intro/',catalog.clips))+';');
const banks={};vm.runInNewContext(base.compile(bankSource),{exports:banks,require:name=>({default:JSON.stringify(name.includes('serling-intro')?catalog:legacy)})});
const timers=new Map();let timed=false;
const readers={};vm.runInNewContext(base.compile(fs.readFileSync('src/core/readAloud.ts','utf8').replace(/const FILES = import\.meta\.glob[^\n]+;/,'const FILES = {};')),
 {exports:readers,require:()=>({default:JSON.stringify({clips:[]})}),AbortController,atob,Uint8Array,Float32Array,console:{warn(){}},
  setTimeout:(fn,ms)=>{if(!timed)return setTimeout(fn,ms);const token={fake:true};timers.set(token,{fn,ms});return token;},clearTimeout:id=>id?.fake?timers.delete(id):clearTimeout(id)});
function fixture(enabled=true){
 const ctx=new base.Clock(),audio={ctx,master:ctx.createGain(),unlock(){return Promise.resolve();}};
 const reader=new readers.ReadAloud(audio,banks.serlingBank);reader.enabled=enabled;
 let actions,removed=0,quiet=0,back=0,dismissed=0,begun=[];const states=[];
 const opening=new Opening(reader,{quiet(){++quiet;},show(next){actions=next;return {close(){++removed;actions.closed();},state(value){states.push(value);}};},begin(choice){begun.push(choice);},back(){++back;},dismissed(){++dismissed;}});
 return {ctx,audio,reader,opening,states,get actions(){return actions;},get removed(){return removed;},get quiet(){return quiet;},get back(){return back;},get dismissed(){return dismissed;},begun};
}
function mainFunction(name){const source=fs.readFileSync('src/main.ts','utf8'),ast=ts.createSourceFile('main.ts',source,ts.ScriptTarget.Latest,true);let result;function visit(node){if(ts.isFunctionDeclaration(node)&&node.name?.getText(ast)===name)result=node.getText(ast);ts.forEachChild(node,visit);}visit(ast);assert.ok(result,name);return result;}

(async()=>{
 check(catalog.clips.length===5&&JSON.stringify(catalog.clips.map(c=>c.text))===JSON.stringify(OPENING_PARAGRAPHS),'the separate bank contains exactly the five complete frozen paragraphs');
 check(catalog.clips.every(c=>c.id===crypto.createHash('sha256').update(c.text.replace(/\s+/g,' ').trim()).digest('hex').slice(0,16)),'intro clip identities retain every original word');
 check(banks.serlingBank.clips.length===legacy.clips.length+5&&catalog.clips.every(c=>banks.serlingBank.urls[c.file]),'the actual narrator bank merges the new URLs without replacing legacy descriptions');
 let f=fixture();check(OPENING_PARAGRAPHS.every(text=>f.reader.available([text])),'all five paragraphs resolve through the actual shared ReadAloud engine');
 check(f.opening.open({caseId:2,replace:true})&&!f.opening.open({caseId:1,replace:false})&&f.quiet===1,'duplicate New night cannot create another card or change its selected case');await flush();
 check(!f.begun.length&&f.opening.active&&f.reader.playing,'showing the card starts narration but never starts or replaces a night');
 for(let i=0;i<5;i++){
  check(f.reader.source&&f.reader.source.buffer.duration!==catalog.clips[i].duration,'paragraph '+(i+1)+' waits for actual decoded audio rather than manifest timing');
  f.reader.source.onended();await flush();
 }
 check(f.opening.active&&f.opening.state==='ready'&&!f.begun.length,'natural end leaves the whole card waiting for Begin night');
 check(f.opening.begin()&&!f.opening.begin()&&f.begun.length===1&&f.begun[0].caseId===2&&f.begun[0].replace,'Begin dispatches the selected replacement exactly once and synchronously');
 check(!f.opening.active&&f.removed===1&&!f.reader.playing,'Begin removes the card and stops all remaining narration');
 f=fixture(false);f.opening.open({caseId:1,replace:false});await flush();check(f.opening.active&&f.opening.state==='ready'&&!f.reader.source,'Serling Off keeps the full text and Begin available without speech');f.opening.cancel();
 check(!f.begun.length&&f.back===1,'Back does not start or replace a case');
 f=fixture(false);f.opening.open({caseId:1,replace:false});f.actions.closed();
 check(f.dismissed===1&&!f.opening.active&&!f.begun.length&&f.back===0,'an external modal closure restores title audio without beginning or choosing another case');
 f.actions.closed();f.opening.dismiss();check(f.dismissed===1,'repeated external closure cannot run title cleanup twice');
 f=fixture();f.opening.open({caseId:1,replace:false});await flush();const stopped=f.reader.source;f.reader.enabled=false;await flush();
 check(f.opening.active&&f.opening.state==='ready'&&stopped.stopped&&!f.reader.source,'turning Serling Off stops its queue and leaves a usable text card');f.opening.cancel();
 for(const action of ['cancel','dismiss','begin']){
  f=fixture();let release;f.ctx.waitDecode=new Promise(resolve=>{release=resolve;});f.opening.open({caseId:3,replace:true});await flush();
  f.opening[action]();await flush();release();await flush();
  check(!f.opening.active&&!f.reader.source&&!f.reader.buffers.size&&f.removed===1,action+': pending decode cannot revive audio, cache or a dead card');
  check(f.begun.length===(action==='begin'?1:0),action+': only Begin commits the selection');
 }
 timed=true;f=fixture();f.ctx.waitDecode=new Promise(()=>{});f.opening.open({caseId:1,replace:false});await flush();
 check(f.opening.state==='loading'&&timers.size===1,'a never-settling intro decoder is covered by the existing loading watchdog');
 for(const timer of [...timers.values()])timer.fn();await flush();
 check(f.opening.state==='error'&&f.opening.active&&!f.reader.source,'loader timeout reports an error without hiding text or blocking Begin');f.opening.begin();check(f.begun.length===1,'Begin still works after a permanent decode failure');timed=false;

 // Actual UI method, including keyboard handling, without a renderer.
 class Button{constructor(){this.events={};}addEventListener(key,fn){this.events[key]=fn;}focus(){doc.activeElement=this;}}
 class Box{constructor(){this.dataset={};this.events={};this.buttons={begin:new Button(),back:new Button()};this.status={};}setAttribute(){}addEventListener(key,fn){this.events[key]=fn;}querySelector(selector){return selector.includes('begin-night')?this.buttons.begin:selector.includes('back-title')?this.buttons.back:this.status;}}
 const doc={createElement:()=>new Box(),activeElement:null};
 const uiExports=load('src/ui/UI.ts',name=>name.includes('/Opening')?{OPENING_PARAGRAPHS}:{},{document:doc,HTMLButtonElement:Button});
 const ui=Object.create(uiExports.UI.prototype);let begin=0,back=0,closed=0;
 ui.open=(el,callback)=>{ui.modal=el;ui.callback=callback;};ui.close=()=>{ui.modal=null;ui.callback();};
 const view=ui.openingCard({begin(){++begin;},back(){++back;},closed(){++closed;}}),el=ui.modal;
 check((el.innerHTML.match(/<p>/g)||[]).length===5&&OPENING_PARAGRAPHS.every(p=>el.innerHTML.includes(p)),'the real card displays all five complete paragraphs');
 check(el.innerHTML.includes('Begin night')&&el.innerHTML.includes('Back to title')&&doc.activeElement===el.buttons.begin,'both real controls exist and initial focus is on Begin');
 let prevented=0,stoppedEvent=0;el.events.keydown({code:'Tab',preventDefault(){++prevented;},stopPropagation(){++stoppedEvent;}});
 check(doc.activeElement===el.buttons.back&&prevented===1&&stoppedEvent===1,'Tab reaches Back despite the global game Tab handler');
 el.events.keydown({code:'Tab',shiftKey:true,preventDefault(){++prevented;},stopPropagation(){++stoppedEvent;}});
 check(doc.activeElement===el.buttons.begin,'Shift-Tab stays within the two opening controls');
 prevented=0;el.events.keydown({code:'Space',target:el.buttons.begin,preventDefault(){++prevented;},stopPropagation(){++stoppedEvent;}});
 check(prevented===0&&stoppedEvent===3,'Space reaches native button activation while bypassing the global Space blocker');
 el.buttons.begin.events.click();el.buttons.back.events.click();view.state('error');view.close();
 check(begin===1&&back===1&&closed===1&&el.dataset.openingState==='error'&&el.status.textContent.includes('begin the night'),'the real card wires both actions, close lifecycle and visible failure feedback');

 // Actual main wrappers protect the loading/title boundaries.
 let opened=0;const requestContext={starting:false,mode:'title',opening:{active:false,open(choice){++opened;this.active=true;this.choice=choice;}}};
 vm.runInNewContext(base.compile(mainFunction('requestNewNight')+'\nglobalThis.request=requestNewNight;'),requestContext);
 requestContext.request(2,true);requestContext.request(1,false);check(opened===1&&requestContext.opening.choice.caseId===2,'the shared New-night wrapper retains one chosen slot without touching storage');
 let releaseUnlock,loops=0;const music={mode:'title',opening:{active:false},audio:{music:{},unlock:()=>new Promise(resolve=>{releaseUnlock=resolve;}),loop(){++loops;}}};
 vm.runInNewContext(base.compile(mainFunction('menuMusic')+'\nglobalThis.run=menuMusic;'),music);music.run();music.opening.active=true;releaseUnlock();await flush();check(loops===0,'a title-music unlock finishing after the intro opens cannot start music under it');
 const calls=[];let rejectRemove;const oldCase=2;
 const gameContext={starting:false,opening:{dismiss(){calls.push('dismiss');}},devRun:false,mode:'title',releasingLock:false,
  audio:{unlock(){calls.push('unlock');return Promise.resolve();},stop(){},silence(){}},input:{touchMode:true,locked:false,enabled:false,reset(){},requestLock(){calls.push('lock');}},
  document:{documentElement:{requestFullscreen(){calls.push('fullscreen');return Promise.resolve();}}},titleEl:{remove(){}},endEl:null,
  ui:{close(){},fade(){},toast(text){calls.push(text);}},saves:{ready:Promise.resolve(),active:oldCase,removeCase(){calls.push('remove');return new Promise((_resolve,reject)=>{rejectRemove=reject;});}},
  showTitleAgain(){calls.push('title');},setTimeout(){return 1;},clearTimeout(){}};
 vm.runInNewContext(base.compile(mainFunction('startGame')+'\nglobalThis.start=startGame;'),gameContext);
 const start=gameContext.start({caseId:3,replaceCase:true,playtime:0,state:null});
 check(calls.includes('fullscreen')&&calls.includes('lock')&&!calls.includes('remove'),'Begin requests fullscreen/pointer lock synchronously before awaiting replacement');await flush();
 const duplicate=gameContext.start({caseId:1,playtime:0,state:null});await duplicate;check(calls.filter(c=>c==='remove').length===1,'double Begin cannot invoke replacement or startup twice');rejectRemove(new Error('controlled storage failure'));await start;
 check(gameContext.mode==='title'&&!gameContext.starting&&gameContext.saves.active===oldCase&&calls.includes('title')&&calls.some(c=>c.includes('could not be replaced')),'replacement failure returns to a usable title without choosing a new active case or starting the night');
 console.log(`intro-core: ${passed}/${passed} PASS`);
})().catch(error=>{console.error(error);process.exitCode=1;});
