"""Paired real-time cab audio cost, with Art Bell On/Off/On and unchanged gameplay.

Run through the GPU slot wrapper. Uses the existing built candidate, never builds.
The per-frame categories explain work; this test does not raise soundcheck's limits.
"""
import asyncio
import hashlib
import json
import os
import statistics
import sys
from pathlib import Path

from playwright.async_api import async_playwright
from readaloud import start_night, wait_for_js

OUT = Path(sys.argv[1] if len(sys.argv) > 1 else 'production/artbell/qa/audio-cost')
SECONDS = float(sys.argv[2]) if len(sys.argv) > 2 else 6
URL = os.environ.get('S47_URL') or Path('dist-single/index.html').resolve().as_uri()
CHECKS = []

COUNT = r"""(() => {
  const s = window.__cost = {nodes:0, params:0, starts:0, decodes:0, frames:0,
    sync:0, tag:'other', categories:{}, active:new Set(), levels:[], samples:[]};
  for (const key of Object.getOwnPropertyNames(BaseAudioContext.prototype)) {
    if (!key.startsWith('create')) continue;
    const fn = BaseAudioContext.prototype[key];
    if (typeof fn === 'function')
      BaseAudioContext.prototype[key] = function(...args) { ++s.nodes; return fn.apply(this,args); };
  }
  for (const key of ['setValueAtTime','linearRampToValueAtTime','exponentialRampToValueAtTime','setTargetAtTime','setValueCurveAtTime']) {
    const fn = AudioParam.prototype[key];
    AudioParam.prototype[key] = function(...args) {
      ++s.params; s.categories[s.tag] = (s.categories[s.tag] || 0) + 1;
      return fn.apply(this,args);
    };
  }
  const observed = new WeakSet();
  for (const prototype of [AudioScheduledSourceNode.prototype,AudioBufferSourceNode.prototype]) {
    const start = prototype.start;
    prototype.start = function(...args) {
      if (!observed.has(this)) {
        observed.add(this); ++s.starts; s.active.add(this);
        this.addEventListener('ended',()=>s.active.delete(this),{once:true});
      }
      return start.apply(this,args);
    };
  }
  const decode = BaseAudioContext.prototype.decodeAudioData;
  BaseAudioContext.prototype.decodeAudioData = function(...args) { ++s.decodes; return decode.apply(this,args); };
  s.installed=true;
})();"""

INSTALL = r"""() => {
  const s = __cost, a = S47.audio, radio = S47.radio, engine = S47.world.engine;
  const wrap = (object,key,tag,frame=false) => {
    const fn = object[key];
    object[key] = function(...args) {
      const previous=s.tag; s.tag=tag; if(frame) ++s.frames;
      if(tag==='station') ++s.sync;
      try { return fn.apply(this,args); } finally { s.tag=previous; }
    };
  };
  wrap(a,'listener','listener',true);
  wrap(engine,'set','engine'); wrap(engine,'tyres','tyres');
  for(const receiver of radio.receivers) wrap(receiver,'update','receiver');
  for(const station of Object.values(radio.stations)) wrap(station,'sync','station');
  const analyser=a.ctx.createAnalyser(); analyser.fftSize=2048; a.master.connect(analyser);
  const buffer=new Float32Array(analyser.fftSize);
  s.capture=()=>({now:performance.now(),audio:a.ctx.currentTime,nodes:s.nodes,params:s.params,
    starts:s.starts,decodes:s.decodes,frames:s.frames,sync:s.sync,
    active:s.active.size,categories:{...s.categories},hostSources:radio.stations.caliche.hostSources.length,
    segment:radio.stations.caliche.seg.kind,stationTime:radio.stations.caliche.t,
    driving:S47.world.driving,metres:S47.world.tripNow?.metres ?? null});
  s.timer=setInterval(()=>{
    analyser.getFloatTimeDomainData(buffer);
    s.levels.push(Math.sqrt(buffer.reduce((sum,v)=>sum+v*v,0)/buffer.length));
    s.samples.push(s.capture());
  },100);
  s.close=()=>{clearInterval(s.timer);a.master.disconnect(analyser);analyser.disconnect();};
}"""


def check(ok, message):
    CHECKS.append({'pass': bool(ok), 'message': message})
    print(('PASS ' if ok else 'FAIL ') + message, flush=True)


def dropouts(levels):
    found, run = [], 0
    for i in range(20, len(levels)):
        before = statistics.median(levels[i-20:i])
        if before > .003 and levels[i] < before * .25:
            run += 1
        else:
            if run >= 3:
                found.append({'at': round((i-run)/10, 1), 'seconds': run/10})
            run = 0
    if run >= 3:
        found.append({'at': round((len(levels)-run)/10, 1), 'seconds': run/10})
    return found


def delta(a, b):
    frames = b['frames'] - a['frames']
    seconds = (b['now'] - a['now']) / 1000
    return {'seconds': seconds, 'frames': frames, 'fps': frames/seconds,
            'per_second': {k: (b[k]-a[k])/seconds for k in ('params','nodes','starts','decodes','sync')},
            'params_per_frame': (b['params']-a['params'])/max(1, frames),
            'by_category_per_frame': {key: (b['categories'].get(key,0)-a['categories'].get(key,0))/max(1,frames)
                                      for key in set(a['categories']) | set(b['categories'])}}


async def main():
    OUT.mkdir(parents=True, exist_ok=True)
    corpus = json.loads(Path('production/artbell/corpus.json').read_text())
    catalog = json.loads(Path('src/assets/audio/artbell/catalog.json').read_text())
    by_id = {c['id']: c for c in catalog['clips']}
    # Both modes stay inside this same complete line for the whole measurement.
    clip = max(corpus['staticLines'], key=lambda item: by_id[item['id']]['duration'])
    check(SECONDS <= min(7.5, by_id[clip['id']]['duration'])-.5,
          'measurement fits inside both the real clip and the procedural line')
    report = {'url': URL, 'seconds_requested': SECONDS, 'fixture': clip,
              'recorded_duration': by_id[clip['id']]['duration'],
              'built_html_sha256': hashlib.sha256(Path('dist-single/index.html').read_bytes()).hexdigest(),
              'scope': 'Same real standing cab and authored host line, normal rAF, On/Off/On. Instrumentation observes without replacing audio.'}
    errors = []
    try:
        async with async_playwright() as playwright:
            browser = await playwright.chromium.launch(args=['--use-angle=swiftshader','--enable-unsafe-swiftshader',
                '--ignore-gpu-blocklist','--autoplay-policy=no-user-gesture-required'])
            page = await browser.new_page(viewport={'width':960,'height':600})
            page.set_default_timeout(120000)
            await page.add_init_script(COUNT + 'HTMLElement.prototype.requestPointerLock=function(){return Promise.resolve();};')
            page.on('pageerror', lambda error: errors.append(str(error)))
            await page.goto(URL)
            await page.evaluate('localStorage.clear()'); await page.reload()
            await start_night(page)
            await page.evaluate("S47.jump('chapter1'); S47.tick(1); S47.setAbductions(false); S47.nightEvents.wait=1e9")
            await wait_for_js(page, "!!S47.game.d.inter.get('truck')")
            await page.evaluate("S47.game.d.inter.get('truck').use()")
            for _ in range(100):
                await page.evaluate('S47.tick(.25,60)')
                if await page.evaluate('S47.world.driving && !S47.game.cinematic'):
                    break
                await asyncio.sleep(.03)
            await wait_for_js(page, 'S47.world.driving && !S47.game.cinematic && !!S47.world.engine')
            await page.evaluate("""() => {
              S47.world.autopilot=null; S47.world.testInput={steer:0,throttle:0};
              S47.radio.reset(2*3600+15*60); S47.game.clock=2*3600+15*60; S47.radio.boyCalled=true;
              S47.carRadio.set(true,'caliche'); S47.tick(.02,60); S47.hold=false;
            }""")
            await asyncio.sleep(2.7)
            await page.evaluate('S47.hold=true')
            await page.evaluate(INSTALL)
            check(await page.evaluate('__cost.installed === true && __cost.params > 0 && __cost.starts > 0'),
                  'native AudioParam and both oscillator/buffer-source counters are installed and active')
            report['engine_recorded'] = await page.evaluate('S47.world.engine.recorded')
            report['runs'] = []
            for label, enabled in (('On-1',True),('Off',False),('On-2',True)):
                await page.evaluate("""({text,enabled}) => {
                  S47.hold=true; S47.radio.setHostVoice(enabled);
                  const st=S47.radio.stations.caliche;
                  st.start([{kind:'talk',lines:[{who:'host',text},{who:'host',text}]},{kind:'dead'}],S47.game.clock);
                }""", {'text':clip['text'],'enabled':enabled})
                await wait_for_js(page, '!S47.radio.stations.caliche.hostPending && S47.radio.stations.caliche.hostReady')
                begin = await page.evaluate('__cost.capture()')
                await page.evaluate('__cost.levels=[]; __cost.samples=[]; S47.hold=false')
                await asyncio.sleep(SECONDS)
                await page.evaluate('S47.hold=true')
                end = await page.evaluate('__cost.capture()')
                observations = await page.evaluate('({levels:__cost.levels,samples:__cost.samples})')
                result = {'label':label,'enabled':enabled,'begin':begin,'end':end,**delta(begin,end),**observations}
                result['dropouts'] = dropouts(result['levels'])
                result['median_pcm'] = statistics.median(result['levels'])
                samples = result['samples']
                if len(samples)>8:
                    mid=len(samples)//2
                    result['early'] = delta(samples[0],samples[mid])
                    result['late'] = delta(samples[mid],samples[-1])
                report['runs'].append(result)
                print(f"{label}: {result['fps']:.2f}fps, {result['per_second']['params']:.2f}params/s, "
                      f"{result['params_per_frame']:.4f}params/frame; categories {result['by_category_per_frame']}; "
                      f"nodes/s {result['per_second']['nodes']:.2f}, sync {end['sync']-begin['sync']}, "
                      f"new sources {end['starts']-begin['starts']}, active {begin['active']}→{end['active']}, "
                      f"host {begin['hostSources']}→{end['hostSources']}, dropouts {len(result['dropouts'])}", flush=True)
                check(begin['driving'] and end['driving'] and abs((end['metres'] or 0)-(begin['metres'] or 0))<.1,
                      f'{label}: the same real cab remains standing throughout normal rAF')
                check(end['sync']==begin['sync'] and end['decodes']==begin['decodes'] and
                      end['hostSources']==begin['hostSources']==(2 if enabled else 0),
                      f'{label}: no per-frame station resync, re-decode or host source multiplication')
                check(not result['dropouts'] and result['per_second']['nodes']<60 and end['active']<=begin['active']+4,
                      f'{label}: no measured master dropouts or runaway node/source growth')
                check(abs(result['by_category_per_frame'].get('listener',0)-9)<.01 and
                      abs(result['by_category_per_frame'].get('engine',0)-8)<.01 and
                      abs(result['by_category_per_frame'].get('tyres',0)-5)<.01 and
                      abs(result['by_category_per_frame'].get('receiver',0)-4)<.01,
                      f'{label}: the existing listener/engine/tyres/receiver contribute exactly 26 calls per frame')
            on1, off, on2 = report['runs']
            check(all(abs(run['params_per_frame']-off['params_per_frame'])<1 for run in (on1,on2)),
                  'Art Bell On/Off/On has equivalent steady parameter work per frame, without raising the 1500/s threshold')
            check(all(abs(run['early']['params_per_frame']-run['late']['params_per_frame'])<1 for run in report['runs']),
                  'parameter work per frame stays stable between early and late measurement windows')
            check(not errors, 'no browser runtime errors')
            await page.evaluate('__cost.close()')
            await browser.close()
    finally:
        report.update({'checks':CHECKS,'errors':errors,'physical_speakers':'UNVERIFIED: PCM/counters are not a hardware listening test'})
        (OUT/'artbell-cost.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
    print(f"{sum(c['pass'] for c in CHECKS)}/{len(CHECKS)} PASS",flush=True)
    return 0 if all(c['pass'] for c in CHECKS) else 1


if __name__ == '__main__':
    sys.exit(asyncio.run(main()))
