// Parameter-bank, exact-text and UI routing checks without a browser or real audio.
const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),ts=require('typescript');
const base=require('./artbell-core.cjs');
let passed=0;const check=(ok,text)=>{assert.ok(ok,text);++passed;};
const flush=()=>new Promise(resolve=>setImmediate(resolve));
const raw=JSON.parse(fs.readFileSync('src/assets/audio/serling/catalog.json','utf8'));
const clips=raw.clips.map(c=>({...c,duration:99}));
const urls=Object.fromEntries(clips.map(c=>[c.file,'data:audio/mpeg;base64,AA==']));
const source=fs.readFileSync('src/core/readAloud.ts','utf8').replace(/const FILES = import\.meta\.glob[^\n]+;/,'const FILES = {};');
const readerModule={};
let deadlineTimers=null;
vm.runInNewContext(base.compile(source),{exports:readerModule,require:()=>({default:JSON.stringify({clips:[]})}),AbortController,atob,Uint8Array,Float32Array,console:{warn(){}},
 setTimeout:(fn,ms)=>deadlineTimers?deadlineTimers.set(fn,ms):setTimeout(fn,ms),clearTimeout:id=>id?.fake?deadlineTimers?.clear(id):clearTimeout(id)});
function fixture(bank={clips,urls,label:'Synthetic Rod Serling parody'}){
 const ctx=new base.Clock(),audio={ctx,master:ctx.createGain(),unlocks:0,unlock(){++this.unlocks;return Promise.resolve();}};
 const reader=new readerModule.ReadAloud(audio,bank);reader.enabled=true;return {reader,ctx,audio};
}
class Box{
 constructor(){this.dataset={};this.events={};this.children=[];this.classList={add(){}};}
 set innerHTML(value){this.html=value;this.button=new Box();this.status=new Box();}
 querySelector(sel){return sel==='button'?this.button:sel.includes('data-reading-status')?this.status:null;}
 setAttribute(key,value){this[key]=value;}
 addEventListener(key,fn){this.events[key]=fn;}
 appendChild(box){this.children.push(box);}
 insertBefore(box){this.children.push(box);}
 remove(){this.removed=true;}
}
const doc={createElement:()=>new Box(),body:new Box()};
const npcModule={};
vm.runInNewContext(base.compile(fs.readFileSync('src/core/npcDialogue.ts','utf8')),{exports:npcModule,
 require:name=>name==='./recordedDialogue'?{RecordedDialogue:class{}}:{}});
const uiModule={};
vm.runInNewContext(base.compile(fs.readFileSync('src/ui/UI.ts','utf8')),{exports:uiModule,require:name=>name.includes('npcDialogue')?npcModule:{},document:doc,setTimeout:()=>0});
function uiFixture(){const ui=Object.create(uiModule.UI.prototype);ui.narrator=null;ui.readAloud=null;ui.cardGeneration=0;ui.chapterStage='idle';ui.updateDescriptionButton=()=>{};return ui;}

(async()=>{
 let f=fixture();
 check(clips.length===1402&&clips.every(c=>f.reader.available([c.text])),'all 1402 authored Serling corpus entries resolve through the shared reader with synthetic URLs');
 check(f.ctx.decodes===0&&f.reader.buffers.size===0,'text coverage neither downloads nor decodes the narration bank');
 const ast=ts.createSourceFile('readaloud-core.cjs',fs.readFileSync('tools/readaloud-core.cjs','utf8'),ts.ScriptTarget.Latest,true);
 let samples=[];function find(node){if(ts.isVariableDeclaration(node)&&node.name.getText(ast)==='samples'&&node.initializer&&ts.isArrayLiteralExpression(node.initializer))samples=vm.runInNewContext(node.initializer.getText(ast));ts.forEachChild(node,find);}find(ast);
 const cases=samples.map(([id,text])=>({id,text,expected:true,segments:f.reader.segments([text])}));
 cases.push({id:'unknown-must-fail',text:'An invented unrecorded platypus description.',expected:false,segments:f.reader.segments(['An invented unrecorded platypus description.'])});
 const unknown=cases.filter(c=>(c.segments!==null)!==c.expected);
 check(samples.length===25&&unknown.length===0,'all existing dynamic descriptions, clocks, minus signs and identifiers retain full word coverage');
 check(!f.reader.available(['SOURCE DISTANCE: -39 LY extra invented words']),'a valid prefix never hides missing trailing words');
 check(f.reader.segments(['SOURCE DISTANCE: -39 LY']).some(text=>text==='minus'||text.includes('-39')),'the negative distance cannot lose its spoken minus');
 const first=clips[0].text;let play=f.reader.play([first]);check(f.audio.unlocks===1,'Serling resumes synchronously inside the triggering gesture');await flush();
 check(f.reader.playing&&f.reader.source.buffer.duration!==99,'Serling uses the actual decoded source length');
 const sourceNode=f.reader.source;f.ctx.state='suspended';await flush();
 check(f.reader.source===sourceNode&&!sourceNode.stopped,'pause preserves the same Serling source without wall-time advance');
 f.ctx.state='running';sourceNode.onended();await play;check(f.reader.state==='idle','natural Serling completion resolves the queue');
 let release;f=fixture();f.ctx.waitDecode=new Promise(resolve=>{release=resolve;});play=f.reader.play([first]);await flush();f.reader.stop();release();await play;
 check(f.reader.state==='idle'&&!f.reader.source&&!f.reader.buffers.size,'late Serling decode after close cannot start, cache or change UI');
 f=fixture();play=f.reader.play([first,clips[1].text]);await flush();const old=f.reader.source;
 const replacement=f.reader.play([clips[2].text]);await flush();await play;
 check(old.stopped&&old.disconnected&&f.reader.playing,'a new description cancels the old sentence and every queued fragment');f.reader.stop();await replacement;
 f=fixture();for(const c of clips.slice(0,7)){play=f.reader.play([c.text]);await flush();f.reader.source.onended();await play;}
 check(f.reader.buffers.size<=4&&f.reader.bufferBytes<=32*1024*1024,'Serling retains the original reader cache limits');

 const ui=uiFixture(), k=fixture({label:'Synthetic Kinski parody',clips:[{id:'paper',text:'A paper.',duration:1,file:'paper.mp3'}],urls:{'paper.mp3':'data:audio/mpeg;base64,AQ=='}});
 f=fixture();ui.narrator=f.reader;ui.readAloud=k.reader;
 check(ui.descriptionReader===f.reader,'Serling has description priority when both voices are On');
 f.reader.enabled=false;check(ui.descriptionReader===k.reader,'Serling Off restores Kinski descriptions');
 k.reader.enabled=false;check(ui.descriptionReader===null,'both choices Off leaves descriptions silent');
 f.reader.enabled=true;check(ui.descriptionReader===f.reader,'Serling works with Kinski Off');k.reader.enabled=true;
 const parent=new Box();let selected=null;ui.onReadingStart=reader=>{selected=reader;};
 ui.readingControls(parent,[first],'Read description','panel-description');parent.children[0].button.events.click();await flush();
 check(selected===f.reader&&f.reader.playing&&!k.reader.playing,'an actual description control starts only Serling');f.reader.stop();
 ui.readingControls(parent,['A paper.'],'Read document','document');parent.children[1].button.events.click();await flush();
 check(selected===k.reader&&k.reader.playing,'paper reading remains on Kinski even with Serling On');k.reader.stop();
 ui.readingControls(parent,['A paper.'],'Read aloud','conversation');parent.children[2].button.events.click();await flush();
 check(selected===k.reader&&k.reader.playing,'manual conversation narration preserves its Kinski choice');k.reader.stop();
 const actors=new Box();ui.npcDialogue={enabled:true,available:()=>true};
 ui.characterReadingControls(actors,'bacall',[{who:'N. VEGA',text:'A Nora line.'}]);
 check(actors.children[0].status.textContent==='Synthetic Lauren Bacall parody','a Nora-only reply retains its single-actor parody label');
 ui.characterReadingControls(actors,'bacall',[{who:'N. VEGA',text:'A Nora line.'},{who:'THE LINE',text:'The existing recording.'}]);
 check(actors.children[1].status.textContent==='Synthetic Lauren Bacall / Vincent Price parodies','the mixed Nora and field-recording queue labels both actual actors');
 ui.characterReadingControls(actors,'stanton',[{who:'THE ATTENDANT',text:'An attendant line.'},{who:'THE LINE',text:'An excluded recording.'}]);
 check(actors.children[2].status.textContent==='Synthetic Harry Dean Stanton parody','an excluded recording cannot add Price to another actor label');

 // Exercise the actual chapter-card promise with every displayed field, including a clock.
 const fields=['Closed case.','A complete recap.','CHAPTER TWO','THE AMENDED RECORD','02:47'];
 const bank={label:'Synthetic Rod Serling parody',clips:fields.map((text,i)=>({id:'card'+i,text,duration:99,file:'card'+i+'.mp3'})),urls:Object.fromEntries(fields.map((_text,i)=>['card'+i+'.mp3','data:audio/mpeg;base64,AA==']))};
 f=fixture(bank);ui.narrator=f.reader;ui.cardEl=null;let complete=false;
 play=ui.chapterCard({closed:fields[0],recap:fields[1],next:fields[2],title:fields[3],clock:fields[4]}).then(()=>{complete=true;});await flush();
 for(let i=0;i<fields.length;i++){
  check(!complete&&f.reader.source,'the chapter card waits for displayed field '+(i+1));
  check(ui.chapterStage===(i<2?'a':'b'),'the visible card phase matches spoken field '+(i+1));
  f.reader.source.onended();await flush();
 }
 await play;check(complete&&f.reader.state==='idle','chapter-card narration resolves only after every complete field');ui.chapterCard(null);
 for(const action of ['Off','null']){
  f=fixture(bank);ui.narrator=f.reader;play=ui.chapterCard({closed:fields[0],recap:fields[1],next:fields[2],title:fields[3],clock:fields[4]});await flush();
  if(action==='Off')f.reader.enabled=false;else ui.chapterCard(null);
  await play;
  check(!f.reader.source&&(action==='Off'?ui.chapterStage==='text-b'&&ui.cardEl.dataset.voicePhase==='b':ui.chapterStage==='idle'),
    action+': no later speech; only an Off choice on the same card preserves readable text-B');
 }
 {
  const timers=new Map();deadlineTimers={set(fn,ms){const id={fake:true};timers.set(id,{fn,ms});return id;},clear(id){timers.delete(id);}};
  f=fixture(bank);ui.narrator=f.reader;let release;
  f.ctx.waitDecode=new Promise(resolve=>{release=resolve;});
  play=ui.chapterCard({closed:fields[0],recap:fields[1],next:fields[2],title:fields[3],clock:fields[4]});await flush();
  check(ui.chapterStage==='a'&&timers.size===1,'a hanging first chapter buffer remains on its readable A phase');
  for(const timer of [...timers.values()])timer.fn();await play;
  check(ui.chapterStage==='error-b'&&ui.cardEl.dataset.voicePhase==='b'&&!f.reader.source,'A-load timeout reveals readable B without starting replacement speech');
  ui.chapterCard(null);release();await flush();
  check(ui.chapterStage==='idle'&&!f.reader.source&&!f.reader.buffers.size,'a timed-out decode finishing after card removal cannot revive either phase');deadlineTimers=null;
 }

 // Main's real priority callbacks are evaluated independently of the renderer.
 const mainAst=ts.createSourceFile('main.ts',fs.readFileSync('src/main.ts','utf8'),ts.ScriptTarget.Latest,true);let stop='',bindings=[],chapter='';
 function visit(node){
  if(ts.isVariableDeclaration(node)&&node.name.getText(mainAst)==='stopNarration')stop='const stopNarration = '+node.initializer.getText(mainAst)+';';
  if(ts.isBinaryExpression(node)&&['ui.onReadingStart','ui.onEarlStart','ui.onCharacterStart','audio.onPhoneCall'].includes(node.left.getText(mainAst)))bindings.push(node.getText(mainAst)+';');
  if(ts.isFunctionDeclaration(node)&&node.name?.getText(mainAst)==='chapterBreak')chapter=node.getText(mainAst);
  ts.forEachChild(node,visit);
 }visit(mainAst);
 const player=()=>({stops:0,stop(){++this.stops;}}),readAloud=player(),narrator=player(),earlDialogue=player(),npcDialogue=player(),cb=player();
 const priority={ui:{},audio:{phoneStops:0,stopPhoneCalls(){++this.phoneStops;}},readAloud,narrator,earlDialogue,npcDialogue,cb};
 vm.runInNewContext(base.compile(stop+bindings.join('\n')),priority);
 priority.ui.onCharacterStart();check(npcDialogue.stops===0&&[readAloud,narrator,earlDialogue,cb].every(p=>p.stops===1),'NPC playback stops every competing foreground voice and CB');
 priority.ui.onReadingStart(narrator);check(narrator.stops===1&&npcDialogue.stops===1&&readAloud.stops===2,'Serling start preserves itself and interrupts NPC/Kinski');
 priority.audio.onPhoneCall();check([readAloud,narrator,earlDialogue,npcDialogue].every(p=>p.stops>0),'a phone call stops every foreground narration player');
 for(const stage of ['error-b','text-b']){
  const timers=[];let resolveCard,closed=0;
  const ui={chapterStage:'a',close(){},viewfinder(){},fade(){},showHud(){},chapterCard(value){if(!value){++closed;return Promise.resolve();}return new Promise(resolve=>{resolveCard=resolve;});}};
  const context={ui,mode:'play',fcam:{raise(){}},game:{cinematic:false,clock:0,after(seconds,fn){timers.push({seconds,fn});}},clockText:()=>'',input:{enabled:false,touchMode:false}};
  vm.runInNewContext(base.compile(chapter+'\nglobalThis.run=chapterBreak;'),context);
  context.run({closed:'',recap:'',next:'',title:'',begin(){}});
  timers.shift().fn();timers.shift().fn();
  check(!closed&&timers.some(t=>t.seconds===.1),'main keeps the card open beyond its minimum while its loader is unresolved');
  ui.chapterStage=stage;resolveCard();await flush();timers.shift().fn();
  check(!closed&&timers.some(t=>t.seconds===3.2),stage+': a silent fallback still receives an independent 3.2-second B reading interval');
  const title=timers.find(t=>t.seconds===3.2),poll=timers.find(t=>t.seconds===.1);title.fn();poll.fn();
  check(closed===1&&!context.game.cinematic,stage+': main exits the readable fallback card only after its B interval completes');
 }
 const qa='production/ensemble/qa';fs.mkdirSync(qa,{recursive:true});
 fs.writeFileSync(qa+'/serling-resolver-coverage.json',JSON.stringify({evidence:'Text-only shared-resolver coverage using synthetic URLs; no real audio claim',corpusClips:clips.length,passed:cases.length-unknown.length,total:cases.length,unknown,cases},null,2)+'\n');
 console.log(`serling-core: ${passed}/${passed} PASS; text-only descriptions ${cases.length-unknown.length}/${cases.length}`);
})().catch(error=>{console.error(error);process.exitCode=1;});
