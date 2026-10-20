// Real role-bank, dialogue, phone and CB code; controlled decoder/clock, no GPU.
const assert = require('node:assert/strict'), fs = require('node:fs'), vm = require('node:vm'), crypto = require('node:crypto');
const base = require('./artbell-core.cjs');
let passed = 0;
const check = (ok, text) => { assert.ok(ok, text); ++passed; };
const near = (a, b, text) => check(Math.abs(a - b) < 1e-7, `${text}: ${a}/${b}`);
const flush = () => new Promise(resolve => setImmediate(resolve));
const clean = text => text.replace(/\s+/g, ' ').trim();
const catalog = JSON.parse(fs.readFileSync('src/assets/audio/ensemble/catalog.json', 'utf8'));
const fakeCatalog = { ...catalog, clips: catalog.clips.map((c, i) => ({ ...c, duration: 99, index: i })) };
const files = Object.fromEntries(fakeCatalog.clips.map(c => [`../assets/audio/ensemble/${c.file}`, `data:audio/mpeg;base64,${Buffer.from([128+c.index]).toString('base64')}`]));
const bank = base.bankModule({}, undefined, { clips: [] }, {}, { clips: [] }, {}, fakeCatalog, files);
function moduleAt(path, requireModule = () => ({}), globals = {}) {
  const result = {};
  vm.runInNewContext(base.compile(fs.readFileSync(path, 'utf8')), { exports: result, require: requireModule, AbortController, console, ...globals });
  return result;
}
const playback = moduleAt('src/core/recordedDialogue.ts');
const npcModule = moduleAt('src/core/npcDialogue.ts', name => name.includes('recordedDialogue') ? playback : bank);
const { NPCDialogue, CHARACTER_SPEAKERS } = npcModule;
const picked = role => catalog.clips.find(c => c.voice === role);
function fixture() {
  const ctx = new base.Clock(); ctx.sampleRate = 48000;
  const master = ctx.createGain(), sfx = ctx.createGain(), radioBus = ctx.createGain(); sfx.connect(master); radioBus.connect(master);
  const audio = { ctx, master, sfx, radioBus, kinskiCalls: false, unlocks: 0,
    unlock() { ++this.unlocks; return Promise.resolve(); },
    phoneLine() {
      const hp = ctx.createBiquadFilter(), lp = ctx.createBiquadFilter(), gain = ctx.createGain();
      hp.type='highpass'; hp.frequency.value=320; lp.type='lowpass'; lp.frequency.value=3200;
      hp.connect(lp); lp.connect(gain); gain.connect(sfx); return { input: hp, nodes: [hp,lp,gain] };
    } };
  const player = new NPCDialogue(audio); player.enabled = true;
  return { ctx, audio, player };
}
const input = role => ({ voice: role, lines: [{ who: role === 'price' ? 'THE LINE' : CHARACTER_SPEAKERS[role], text: picked(role).text }] });

(async () => {
  const counts = Object.fromEntries(['stanton','stack','lynch','bacall','price'].map(role => [role,catalog.clips.filter(c=>c.voice===role).length]));
  check(JSON.stringify(counts) === JSON.stringify({stanton:18,stack:19,lynch:13,bacall:49,price:1}), 'all 100 authored role clips are present');
  check(catalog.clips.every(c => c.id === crypto.createHash('sha256').update(c.voice+'\0'+clean(c.text)).digest('hex').slice(0,16)), 'every clip identity includes its role and exact normalized words');
  const resolver = new bank.RadioVoice('ensemble');
  check(resolver.bufferCount === 0, 'role-bank construction decodes nothing');
  for (const role of Object.keys(counts)) {
    check(resolver.resolve('  '+picked(role).text.replace(/ /g,'\n ')+'  ',0,role)?.[0].voice===role, `${role} resolves its exact words with whitespace normalization`);
    check(!resolver.resolve(picked(role).text,0,role==='price'?'bacall':'price'), `${role} cannot borrow another role for the same words`);
    const f=fixture(), data=input(role);
    check(f.player.available(data) && !f.player.available({voice:role,lines:[{who:'YOU',text:data.lines[0].text},{who:'',text:data.lines[0].text}]}), `${role} strictly excludes Reyes and stage directions`);
    const promise=f.player.play(data); check(f.audio.unlocks===1, `${role} unlock begins inside the synchronous gesture`); await flush();
    check(f.player.playing && f.player.source.buffer && !f.audio.kinskiCalls, `${role} speaks independently of Kinski`);
    check(f.player.source.playbackRate.value===1 && base.reaches(f.player.source,f.audio.master) && !base.reaches(f.player.source,f.audio.radioBus), `${role} uses natural speed and the correct foreground bus`);
    if(role==='price') check(f.ctx.nodes.some(n=>n.type==='highpass'&&n.frequency.value===320) && f.ctx.nodes.some(n=>n.type==='lowpass'&&n.frequency.value===3200), 'the field recording alone uses the phone-line filter');
    f.player.stop(); await promise;
  }
  let f=fixture();
  const price=picked('price'), nora=picked('bacall');
  let states=[];
  let play=f.player.play({voice:'bacall',lines:[{who:'YOU',text:'A question.'},{who:'N. VEGA',text:nora.text},{who:'THE LINE',text:price.text},{who:'',text:'She hangs up.'}]},s=>states.push(s));
  await flush(); const first=f.player.source;
  check(f.player.status.clips.map(c=>c.voice).join(',')==='bacall,price' && f.player.status.clips.every(c=>c.duration!==99), 'the mixed Nora exchange keeps both true actor identities and decoded lengths');
  f.ctx.state='suspended'; await flush(); check(f.player.source===first && !first.stopped, 'a suspended context never advances the foreground queue on wall time');
  f.ctx.state='running'; f.ctx.currentTime+=first.buffer.duration; first.onended(); await flush();
  check(f.player.status.text===price.text && first.disconnected && f.player.source!==first, 'Price starts only after Nora reaches its actual source end');
  near(f.player.source.startAt,f.ctx.currentTime+.22,'interline gap follows the audio clock');
  f.player.source.onended(); await play; check(f.player.state==='idle'&&states.at(-1)==='idle','the whole mixed exchange returns to idle');
  play=f.player.play(input('stanton')); await flush(); const replaced=f.player.source;
  const replacement=f.player.play(input('stack')); await flush(); await play;
  check(replaced.stopped&&replaced.disconnected&&f.player.status.text===picked('stack').text,'a new actor cancels current and unstarted old speech');
  f.player.enabled=false; await replacement; check(f.player.state==='idle'&&f.player.source===null,'Character voices Off stops the current queue');
  f=fixture(); let release;
  f.ctx.waitDecode=new Promise(resolve=>{release=resolve;}); play=f.player.play(input('stanton')); await flush(); f.player.stop(); release(); await play;
  check(!f.player.source&&f.player.bank.bufferCount===0&&f.player.state==='idle','a late decode after close cannot cache, play or update stale state');
  f=fixture(); f.player.bank.load=async()=>{throw new Error('controlled failure');}; await f.player.play(input('bacall'));
  check(f.player.state==='error'&&!f.player.source,'a load failure keeps the entire reply as visible text and starts no partial audio');
  f=fixture(); await f.player.bank.load(fakeCatalog.clips,f.ctx,new AbortController().signal);
  check(f.player.bank.bufferCount<=24&&f.player.bank.bytes<=24*1024*1024,'all 100 role clips retain both lazy-cache bounds');

  // Exercise actual AudioSys actor selection and its deliberately small preload.
  const scripts=moduleAt('src/core/phoneScripts.ts');
  const phoneCatalog={clips:scripts.PHONE_TEXTS.map((text,i)=>({id:'phone'+i,text,file:'phone'+i+'.mp3'}))};
  const phoneFiles=Object.fromEntries(phoneCatalog.clips.map((c,i)=>[`../assets/audio/kinski/${c.file}`,`data:audio/mpeg;base64,${Buffer.from([i]).toString('base64')}`]));
  let audioSource=fs.readFileSync('src/core/Audio.ts','utf8').replace(/const (STEP_FILES|ALIEN_FILES|PHONE_FILES|CHARACTER_FILES|LICENSED) = import\.meta\.glob[^\n]+;/g,(_m,name)=>`const ${name} = ${JSON.stringify(name==='PHONE_FILES'?phoneFiles:name==='CHARACTER_FILES'?files:{})};`);
  const audioExports={};
  vm.runInNewContext(base.compile(audioSource),{exports:audioExports,atob,console,window:{OfflineAudioContext:base.Clock},require(name){
    if(name==='./phoneScripts')return scripts;
    if(name==='./readAloud')return {normalizeReadingText:clean};
    if(name.endsWith('catalog.json?raw'))return {default:JSON.stringify(name.includes('/ensemble/')?fakeCatalog:phoneCatalog)};
    if(name.endsWith('.mp3'))return {default:'data:audio/mpeg;base64,AA=='};
    return {};
  }});
  const a=new audioExports.AudioSys(); await a.preload();
  check(Object.keys(a.buf).filter(k=>k.startsWith('character:')).length===11,'only seven Nora phones, three CB lines and one Price clip are preloaded');
  check(Object.keys(a.buf).filter(k=>k.startsWith('phone:')).length===24,'all 24 original Kinski phone buffers are preserved');
  f=fixture(); Object.assign(a,{ctx:f.ctx,master:f.audio.master,sfx:f.audio.sfx,radioBus:f.audio.radioBus,noise:f.ctx.createBuffer(1,48000,48000),characterVoices:true,kinskiCalls:false});
  const call=a.phoneCall(scripts.PHONE_CALLS.nora);
  check(call&&call.cues.length===4&&call.cues[1].aside&&call.cues[1].duration===3.4,'Nora phone works KinskiOff and preserves its exact silent aside');
  check(call.cues.filter(c=>!c.aside).every(c=>c.duration===a.characterBuffer('bacall',c.text).duration),'every Nora phone caption uses its selected actor buffer duration');
  a.stopPhoneCalls(); const pcall=a.phoneCall([{by:'THE LINE',text:price.text}],{lead:4});
  near(pcall.cues[0].at,4,'Price preserves the hut recording lead');
  check(pcall.cues[0].duration===a.characterBuffer('price',price.text).duration,'Price uses exactly the same natural recording on the handset');
  a.stopPhoneCalls(); check(!a.phoneCall([{by:'THE LINE',text:price.text+' Wrong words.'}]),'unknown field-line text cannot borrow Price or omit words');
  a.characterVoices=false; check(!a.phoneCall(scripts.PHONE_CALLS.nora),'both voice choices Off preserves the procedural phone path');
  a.kinskiCalls=true; check(a.phoneCall(scripts.PHONE_CALLS.nora),'Character voices Off preserves the existing Kinski phone option'); a.stopPhoneCalls();
  a.characterVoices=true;
  {
    a.kinskiCalls=false;
    const ch2Module=moduleAt('src/story/Chapter2.ts',name=>name==='three'?require('three'):name.includes('phoneScripts')?scripts:{});
    const chapter=Object.create(ch2Module.Chapter2.prototype),timers=[];let calls=0;
    a.play=()=>null;a.dial=()=>1;a.ringback=()=>2;
    const phone=a.phoneCall.bind(a);a.phoneCall=(...args)=>{++calls;return phone(...args);};
    Object.assign(chapter,{s:{stage:'call-nora',called:false},d:{audio:a,ui:{toast(){}},annex:{objs:{recPhone:{}}}},
      g:{cinematic:false,after(at,fn){timers.push({at,fn});},note(){}},pos:()=>({}),file(){},docMap:()=>({}),time:()=>'',save(){}});
    chapter.useRecPhone();
    check(calls===0&&timers.length===1,'the authored chapter-two Nora call still waits for dial and ringback');
    timers.shift().fn();
    check(calls===1&&a.phonePlaying&&!chapter.s.called,'Character On reaches the actual Bacall phone branch with Kinski Off, without premature progress');
    a.stopPhoneCalls();
  }
  const cbModule=moduleAt('src/core/cb.ts',name=>name==='three'?require('three'):name==='./radio'?base.runtime:{});
  const cb=new cbModule.CB(a), heard=[]; let ended=0;
  const cbLines=scripts.NORA_CB_TEXTS.map(text=>({text,by:'N. VEGA',who:'woman'}));
  const beforeNodes=f.ctx.nodes.length;
  const seconds=cb.call(cbLines,null,(line,duration)=>heard.push([line.text,duration]),()=>++ended);
  const nodes=f.ctx.nodes.slice(beforeNodes), speech=nodes.filter(n=>cbLines.some(l=>n.buffer===a.characterBuffer('bacall',l.text)));
  check(seconds>0&&speech.length===3&&speech.every(n=>base.reaches(n,a.radioBus)&&n.playbackRate.value===1),'CB uses all three Bacall buffers at natural speed through its original radio speaker');
  near(speech[1].startAt-speech[0].startAt,speech[0].buffer.duration+.55,'CB schedules the next sentence after its true decoded length');
  const markers=nodes.filter(n=>n.buffer?.length===1&&n.onended);
  check(markers.length===4&&heard.length===0&&!ended,'CB captions and completion use silent audio-clock markers instead of wall timers');
  markers.slice(0,3).forEach(marker=>marker.onended());
  check(heard.every(([text,duration],i)=>text===cbLines[i].text&&duration===speech[i].buffer.duration),'CB captions carry every exact authored word and natural duration');
  markers[3].onended(); check(!cb.busy&&ended===1&&speech.every(n=>n.stopped&&n.disconnected),'natural CB completion releases sources and fires its original completion callback once');
  cb.call(cbLines,null,()=>{},()=>++ended); cb.stop(); check(!cb.busy&&ended===1,'cancelled CB cannot publish a stale completion');
  console.log(`ensemble-core: ${passed}/${passed} PASS`);
})().catch(error=>{console.error(error);process.exitCode=1;});
