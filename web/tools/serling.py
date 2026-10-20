"""Serling descriptions and every authored chapter card, against real MP3s.

All catalogue entries are checked by the runtime resolver; representative game
readers and all six card variants run on the real AudioContext. This intentionally
does not play the whole 1402-clip bank in real time. Root runs GPU serially.
"""
import asyncio
import hashlib
import json
import os
import sys
import tempfile
from pathlib import Path

from playwright.async_api import async_playwright
from readaloud import (CSP, no_horizontal_overflow, normalized, open_game_document,
                       quiet_background, sample_master, start_night, wait_for_js)
import descriptions
import ensemble

OUT=Path(sys.argv[1] if len(sys.argv)>1 else 'production/ensemble/qa/serling-single')
URL=os.environ.get('S47_URL') or Path('dist-single/index.html').resolve().as_uri()
PAGES=bool(os.environ.get('S47_URL'))
CHECKS=[]
HUD='button[data-a=read-description]'
PROBE=r"""() => {
  if(window.__serlingProbe)return;
  const n=S47.narrator,ui=S47.game.d.ui,a=S47.audio;
  const p=window.__serlingProbe={requests:[],cards:[],samples:[],late:null,examineAt:0,lastCard:null};
  const play=n.play.bind(n);n.play=(texts,onState,onTextIndex)=>{
    const phase=p.lastCard ? texts.includes(p.lastCard.closed)||texts.includes(p.lastCard.recap)?'A':'B':null;
    const r={texts:[...texts],segments:n.segments(texts),phase,at:a.ctx.currentTime,wall:performance.now(),done:null};
    r.textIndex=0;r.boundaries=[];p.requests.push(r);
    const result=play(texts,onState,index=>{r.textIndex=index;r.boundaries.push({index,at:a.ctx.currentTime});onTextIndex?.(index);});
    result.then(()=>r.done=a.ctx.currentTime);return result;};
  const card=ui.chapterCard.bind(ui);ui.chapterCard=o=>{
    p.lastCard=o;p.cards.push({value:o,at:a.ctx.currentTime,gt:S47.game.gt});return card(o);};
  const tick=__ensembleProbe.tick;__ensembleProbe.tick=()=>{const result=tick(),el=document.querySelector('.chapter-card:not(.out)'),now=a.ctx.currentTime;
    if(el)for(const r of __readingTrace.starts.filter(r=>r.serlingId)){
      if(r.cardSampled||now<r.at+Math.min(.35,r.duration/3)||now>=r.expectedEnd)continue;
      r.cardSampled=true;const A=el.querySelector('.cc-a'),B=el.querySelector('.cc-b'),
        visible=x=>{const s=getComputedStyle(x);return s.display!=='none'&&s.visibility!=='hidden'&&Number(s.opacity)>.01;};
      const request=p.requests.at(-1),phase=request?.texts.length===5?(request.textIndex<2?'A':'B'):request?.phase;
      p.samples.push({id:r.serlingId,at:now,phase,A:visible(A),B:visible(B),
        aOpacity:getComputedStyle(A).opacity,bOpacity:getComputedStyle(B).opacity,classes:el.className});
    }return result;};
  p.reset=()=>{n.stop();__ensembleProbe.reset();p.requests.length=p.cards.length=p.samples.length=0;p.lastCard=null;};
  p.snapshot=()=>({now:a.ctx.currentTime,gt:S47.game.gt,context:a.ctx.state,state:n.state,active:!!n.source,pending:!!n.pending,
    requests:p.requests,cards:p.cards,samples:p.samples,stage:ui.chapterStage,cinematic:S47.game.cinematic,
    visibleB:(()=>{const e=document.querySelector('.chapter-card:not(.out) .cc-b');return !!e&&getComputedStyle(e).display!=='none'&&getComputedStyle(e).visibility!=='hidden'&&Number(getComputedStyle(e).opacity)>.01;})(),
    visibleCard:!!document.querySelector('.chapter-card:not(.out)'),
    starts:__readingTrace.snapshot().filter(r=>r.serlingId||r.kinski)});
  p.delayNextLoad=()=>{const original=a.ctx.decodeAudioData;let release;const gate=new Promise(resolve=>release=resolve);
    n.buffers.clear();n.bufferBytes=0;p.lateSignal=null;p.late={ready:false,get aborted(){return !!p.lateSignal?.aborted;}};
    a.ctx.decodeAudioData=async function(...args){a.ctx.decodeAudioData=original;
      const buffer=await original.apply(this,args);p.lateSignal=n.pending?.signal;p.late.ready=true;
      await gate;return buffer;};p.releaseLate=()=>release();};
  p.delayNextUnlock=()=>{const original=a.unlock;let release;const gate=new Promise(resolve=>release=resolve);
    p.lateSignal=null;p.late={ready:false,get aborted(){return !!p.lateSignal?.aborted;}};
    a.unlock=function(...args){a.unlock=original;const genuine=original.apply(this,args);
      return genuine.then(async()=>{p.lateSignal=n.pending?.signal;p.late.ready=true;await gate;});};p.releaseLate=()=>release();};
}"""
INVENTORY=descriptions.INVENTORY.replace('reader = g.d.ui.readAloud','reader = S47.narrator')


def check(ok,message):
    CHECKS.append({'pass':bool(ok),'message':message});print(('PASS ' if ok else 'FAIL ')+message,flush=True)


def starts(state):return [r for r in state['starts'] if r.get('serlingId')]


async def snapshot(page):return await page.evaluate('__serlingProbe.snapshot()')


async def first_start(page,count=0):
    await ensemble.pump(page,until=f"__readingTrace.snapshot().filter(r=>r.serlingId).length>{count}")
    return await snapshot(page)


async def read_box(page,kind):
    box=page.locator(f'[data-reading-kind="{kind}"]')
    await box.wait_for()
    text=await box.evaluate('e=>JSON.parse(e.dataset.readingTexts)')
    button=box.locator('[data-a=narrate]')
    check(not await button.is_disabled(),kind+': exact current description has complete Serling coverage')
    before=len(starts(await snapshot(page)));await button.click();await first_start(page,before)
    state=await snapshot(page)
    check(state['requests'][-1]['texts']==text and all(ensemble.direct(r) for r in starts(state)[before:]),kind+': real Read description uses the exact current text and direct master route')
    return text,state


async def lifecycle(page,text,saved):
    evidence=[]
    for pending in (False,True):
        for action in ('close','replacement','phone','cinematic','Off','reset','jump','load','title'):
            if not await page.evaluate('S47.started()'):
                await start_night(page)
            await page.evaluate("S47.game.d.ui.close();S47.jump('chapter1');S47.tick(.2);S47.narrator.enabled=true;__serlingProbe.reset()")
            if pending:await page.evaluate('__serlingProbe.delayNextLoad()')
            await page.evaluate("t=>{S47.game.d.ui.toast(t);S47.game.d.ui.readLastDescription();}",text)
            if pending:await wait_for_js(page,'__serlingProbe.late.ready&&S47.narrator.loading')
            else:await first_start(page)
            before=await snapshot(page)
            if action=='close':await page.evaluate('S47.game.d.ui.close()')
            elif action=='replacement':await page.evaluate('S47.game.openNotebook()')
            elif action=='phone':await page.evaluate("S47.audio.phoneCall([{by:'N. VEGA',text:'Sierra Motor Court.'}])")
            elif action=='cinematic':await page.evaluate('S47.game.cinematic=true;S47.tick(.05,60)')
            elif action=='Off':await page.evaluate('S47.narrator.enabled=false')
            elif action=='reset':await page.evaluate('S47.game.reset()')
            elif action=='jump':await page.evaluate("S47.jump('chapter1')")
            elif action in ('load','title'):
                await ensemble.art.pause(page)
                await page.locator(f'.settings-panel [data-a={action}]').click()
                if action=='load':
                    await page.locator(f'.savemenu [data-load="{saved["id"]}"]').click()
                    await wait_for_js(page,'S47.started()&&!S47.game.cinematic')
                    await page.evaluate('S47.hold=true')
            if pending:await page.evaluate('__serlingProbe.releaseLate()')
            await ensemble.pump(page,.2);after=await snapshot(page)
            check(after['state']=='idle' and not after['active'] and not after['pending'] and
                  (not starts(after) if pending else ensemble.art.cancelled_or_completed(starts(after))) and
                  (not pending or await page.evaluate('__serlingProbe.late.aborted')),
                  f"{'pending decoded' if pending else 'active'} Serling {action}: cancellation cannot revive narration")
            evidence.append({'pending':pending,'action':action,'before':before,'after':after})
            await page.evaluate('S47.audio.stopPhoneCalls();S47.game.cinematic=false')
    return evidence


async def card_journey(page,label,setup,trigger,screens=False):
    await page.evaluate("S47.game.d.ui.close();S47.jump('chapter1');S47.tick(.2);S47.narrator.enabled=true;"+setup+"__serlingProbe.reset();"+trigger)
    await ensemble.pump(page,until='__serlingProbe.cards.some(c=>c.value!==null)')
    await first_start(page)
    opening=await snapshot(page);card=next(c for c in opening['cards'] if c['value'] is not None)
    texts=[card['value'][k] for k in ('closed','recap','next','title','clock')]
    covered=await page.evaluate('texts=>texts.every(t=>S47.narrator.available([t]))',texts)
    check(opening['requests'][0]['texts'] in (texts,texts[:2]) and covered,label+': all five actual card fields, headings and clock resolve')
    if screens:
        await ensemble.pump(page,.5);await page.screenshot(path=str(OUT/(label+'-phase-A.png')))
    # Observe beyond the former fixed 6.4-second cutoff using real audio time.
    await ensemble.pump(page,until=f"S47.audio.ctx.currentTime>={card['at']+6.5}")
    extended=await snapshot(page)
    check(extended['visibleCard'] and extended['cinematic'] and any(r['done'] is None for r in extended['requests']),label+': the real card remains on black after 6.4 seconds while narration still runs')
    if screens:
        await ensemble.pump(page,until="S47.game.d.ui.chapterStage==='b'&&S47.narrator.playing",timeout=180000)
        await ensemble.pump(page,.5);await page.screenshot(path=str(OUT/(label+'-phase-B.png')))
    await ensemble.pump(page,until="!S47.game.cinematic&&!document.querySelector('.chapter-card:not(.out)')&&S47.narrator.state==='idle'",timeout=180000)
    done=await snapshot(page);rows=starts(done);open_index=next(i for i,c in enumerate(done['cards']) if c['value'] is not None)
    close=next(c for c in done['cards'][open_index+1:] if c['value'] is None)
    requests=done['requests'];segments=[s for r in requests for s in r['segments']]
    check([t for r in requests for t in r['texts']]==texts,label+': displayed A then B fields are spoken exactly once in order')
    check(bool(done['samples']) and all(s['A'] and not s['B'] if s['phase']=='A' else s['B'] and not s['A'] for s in done['samples']) and
          {s['phase'] for s in done['samples']}=={'A','B'},label+': the currently spoken card phase is visible and the other phase stays hidden')
    check(ensemble.natural(rows) and all(ensemble.direct(r) for r in rows) and len(rows)==len(segments) and
          close['at']>=max(r['expectedEnd'] for r in rows)-.04 and close['at']>=card['at']+6.4-.05,
          label+': every real segment finishes naturally before the actual chapter transition closes its card')
    return {'opening':opening,'after_old_cutoff':extended,'finished':done}


async def main():
    global URL
    OUT.mkdir(parents=True,exist_ok=True)
    clips=json.loads(Path('src/assets/audio/serling/catalog.json').read_text())['clips']
    corpus=json.loads(Path('production/ensemble/serling/corpus.json').read_text())['clips']
    if len(clips)!=1402 or len(corpus)!=1402 or any(c['duration']<=0 for c in clips):raise AssertionError('Finish the complete 1402-recording Serling bank before browser QA')
    check({(c['id'],c['text']) for c in clips}=={(c['id'],c['text']) for c in corpus} and all(
          hashlib.sha256(normalized(c['text']).encode()).hexdigest()[:16]==c['id'] and Path('src/assets/audio/serling',c['file']).is_file() for c in clips),
          'the complete Serling catalogue equals the source corpus with exact text hashes and actual MP3 files')
    report={'pages':PAGES,'strict_csp':not PAGES,'bank_count':len(clips),'fixtures':'Game region/finish callbacks are explicit API fixtures; every card uses actual main chapterBreak and its real clock. Object Zoom/V/HUD, paper and notebook buttons are actual input. Full bank resolver coverage is separate from representative natural audio.'}
    errors,requests,bad=[],[],[];csp_dir=None
    if not PAGES:
        csp_dir=tempfile.TemporaryDirectory(prefix='s47-serling-csp-');target=Path(csp_dir.name,'index.html')
        target.write_text(Path('dist-single/index.html').read_text().replace('<head>','<head><meta http-equiv="Content-Security-Policy" '+f'content="{CSP}">',1));URL=target.as_uri()
    report['url']=URL
    try:
        async with async_playwright() as p:
            browser=await p.chromium.launch(args=['--use-angle=swiftshader','--enable-unsafe-swiftshader','--ignore-gpu-blocklist'])
            async def new_page(context):
                pg=await context.new_page();pg.set_default_timeout(120000)
                await pg.add_init_script(ensemble.art.REAL_AUDIO+'\n'+ensemble.GRAPH)
                pg.on('pageerror',lambda e:errors.append(str(e)))
                pg.on('console',lambda m:errors.append(m.type+': '+m.text) if m.type in ('error','warning') else None)
                pg.on('request',lambda r:requests.append(r.url) if r.url.startswith(('http:','https:')) and r.url.split('?',1)[0].endswith('.mp3') else None)
                pg.on('response',lambda r:bad.append((r.status,r.url)) if r.status>=400 and r.url.split('?',1)[0].endswith('.mp3') else None)
                await pg.goto(URL);await pg.evaluate('localStorage.clear()');await pg.reload();await start_night(pg)
                await pg.evaluate(ensemble.PROBE);await pg.evaluate(PROBE);await quiet_background(pg)
                return pg
            desktop=await browser.new_context(viewport={'width':1280,'height':800});page=await new_page(desktop)
            check(await page.evaluate('S47.narrator.enabled&&S47.game.d.ui.descriptionReader===S47.narrator&&!S47.game.d.ui.readAloud.enabled&&S47.npcDialogue.enabled'), 'Serling defaults On with Kinski Off and remains independent of the character setting')
            coverage=await page.evaluate("clips=>clips.map(c=>({id:c.id,available:S47.narrator.available([c.text]),segments:S47.narrator.segments([c.text])}))",clips)
            check(all(c['available'] and c['segments'] for c in coverage) and len(coverage)==len(clips),'every Serling catalogue text resolves completely in the actual runtime without playing the entire bank')
            report['resolver']=coverage
            # Actual right-button Zoom reaches main's raycast and the 650ms UI
            # timer. Pointer lock alone is declared as a headless input fixture.
            await page.evaluate("""() => {const it=S47.game.d.inter.get('look:labSign'),p=it.object.getWorldPosition(new S47.three.Vector3());
              const x=p.x-2.2,z=p.z;S47.player.place(x,z,Math.atan2(-(p.x-x),-(p.z-z)));
              S47.player.pitch=Math.atan2(p.y-(S47.player.floorY+S47.player.eye),Math.hypot(p.x-x,p.z-z));
              S47.input.locked=true;__serlingProbe.reset();S47.tick(.05,60);__serlingProbe.examineAt=performance.now();}""")
            await page.mouse.down(button='right');await page.evaluate('S47.tick(.2,60)')
            examined=await page.locator('.hud .examine').inner_text();await asyncio.sleep(.30)
            check(bool(examined) and await page.evaluate('__serlingProbe.requests.length')==0,'real Zoom exposes a description but does not speak before the stable 650ms delay')
            await first_start(page);zoom=await snapshot(page)
            check(zoom['requests'][0]['texts']==[examined] and zoom['requests'][0]['wall']-await page.evaluate('__serlingProbe.examineAt')>=620 and all(ensemble.direct(r) for r in starts(zoom)), 'stable Zoom automatically reads the exact raycast description using genuine Serling MP3s')
            await page.screenshot(path=str(OUT/'serling-desktop.png'))
            await page.mouse.up(button='right');await page.evaluate('S47.tick(.1);S47.input.locked=false')
            check(await page.evaluate("S47.narrator.state==='idle'"),'releasing Zoom cancels automatic Serling narration')
            await page.evaluate("__serlingProbe.reset();S47.game.d.inter.get('clock').use()")
            last_description=await page.evaluate('S47.game.d.ui.lastDescription')
            await page.keyboard.press('v');await first_start(page)
            check((await snapshot(page))['requests'][-1]['texts']==[last_description] and '23:41' in last_description,
                  'actual V reads the latest genuine wall-clock observation, including its numeric time, with Serling')
            await page.locator(HUD).click();check(await page.evaluate("S47.narrator.state==='idle'"),'actual HUD Stop reading cancels the same narrator')
            await page.locator(HUD).click();await first_start(page,1)
            audible=await sample_master(page);check(audible['peakRms']>1e-5,'genuine Serling PCM reaches master with unrelated buses silenced')
            await page.evaluate('S47.game.d.ui.close();__serlingProbe.reset()')
            item,_=await open_game_document(page,kind='description');text,doc=await read_box(page,'description')
            check(await page.locator('[data-reading-kind=document]').count()==0,'with Kinski Off the actual paper description uses Serling while its paper-text Kinski control stays unavailable')
            report['document']={'item':item,'text':text,'audio':doc};await page.evaluate('S47.game.d.ui.close()')
            report['regions']={}
            areas=[('chapter1','saro'),('chapter2','saro'),('chapter3','station01'),('chapter4','room6'),('chapter5','diner'),('chapter6','roswell')]
            for chapter,area in areas:
                await page.evaluate("c=>{S47.game.d.ui.close();S47.jump(c);S47.tick(.2,60);__serlingProbe.reset();}",chapter)
                if area=='roswell':
                    await ensemble.pump(page,until="S47.world.area==='roswell'&&!S47.game.cinematic",timeout=30000)
                else:
                    await page.evaluate("async a=>{await S47.world.prepare(a);S47.world.enter(a);S47.tick(.1,60);}",area)
                inventory=await page.evaluate(INVENTORY)
                check(bool(inventory) and all(e['available'] for e in inventory) and await page.evaluate('S47.world.area')==area,chapter+'/'+area+': current object/notebook/people/map descriptions resolve in the actual prepared area')
                report['regions'][chapter]={'area':area,'inventory':inventory}
                await page.evaluate('S47.game.openNotebook()');await page.locator('.notebook [data-t=tasks]').click()
                await read_box(page,'notebook-description');await page.evaluate('S47.game.d.ui.close()')
            await page.evaluate("S47.jump('chapter1');S47.tick(.2)");saved=await page.evaluate("S47.saveNow('manual',0)")
            report['lifecycle']=await lifecycle(page,examined,saved)
            await page.evaluate("S47.game.d.ui.close();S47.jump('chapter1');S47.tick(.1);S47.narrator.enabled=true;__serlingProbe.reset()")
            await page.evaluate("() => {void S47.narrator.play(['A totally unavailable sentence qxzv.']);}")
            await wait_for_js(page,"S47.narrator.state==='error'")
            check(not starts(await snapshot(page)),'unknown narrator words fail explicitly without partial speech')
            await page.evaluate("() => {S47.narrator.stop();window.__serlingLoad=S47.narrator.load;S47.narrator.load=async()=>{throw Error('intentional Serling decode-failure fixture');};}")
            await page.evaluate('t=>{void S47.narrator.play([t]);}',examined);await wait_for_js(page,"S47.narrator.state==='error'")
            check(not starts(await snapshot(page)),'decoder failure produces no stale Serling source')
            await page.evaluate('() => {S47.narrator.load=__serlingLoad;S47.narrator.stop();}')
            await ensemble.toggle(page,'serling',False);await ensemble.toggle(page,'kinski',True)
            await page.evaluate('t=>{__serlingProbe.reset();S47.game.d.ui.toast(t);}',examined);await page.locator(HUD).click()
            await ensemble.pump(page,until='__readingTrace.snapshot().some(r=>r.kinski)')
            check(await page.evaluate('S47.game.d.ui.descriptionReader===S47.game.d.ui.readAloud') and not starts(await snapshot(page)),'Serling Off preserves the real optional Kinski description fallback')
            await ensemble.toggle(page,'serling',True);await ensemble.toggle(page,'kinski',False)
            report['cards']={}
            cases=[('prologue','',"S47.game.onFinish()"),('chapter1_passive','',"S47.ch1.onEnd('passive')"),('chapter1_active','',"S47.ch1.onEnd('active')"),
                   ('chapter2','',"S47.ch2.onEnd()"),('chapter3','',"S47.ch3.onEnd()"),('chapter4','',"S47.ch4.onEnd()")]
            for label,setup,trigger in cases:report['cards'][label]=await card_journey(page,label,setup,trigger,screens=label=='prologue')
            # Keep a genuine decode result unresolved until the product's own
            # per-buffer watchdog has advanced the real chapter transition.
            # Releasing it first would only prove cancellation, not liveness.
            report['hanging_card']={}
            for held_step in ('decode','unlock'):
                delay='__serlingProbe.delayNextLoad()' if held_step=='decode' else '__serlingProbe.delayNextUnlock()'
                await page.evaluate("S47.game.d.ui.close();S47.jump('chapter1');S47.tick(.1);__serlingProbe.reset();"+delay+";S47.ch2.onEnd()")
                await ensemble.pump(page,until='__serlingProbe.late.ready&&S47.narrator.loading')
                hanging=await snapshot(page)
                check(hanging['visibleCard'] and bool(hanging['cards'][-1]['value']['recap']) and not starts(hanging),f'a held genuine {held_step} leaves the actual chapter card readable without partial speech')
                await ensemble.pump(page,until="S47.game.d.ui.chapterStage==='error-b'",timeout=25000)
                error_b=await snapshot(page)
                await ensemble.pump(page,.25);readable=await snapshot(page)
                check(readable['visibleB'] and readable['visibleCard'] and not starts(readable),f'{held_step} timeout displays readable B without starting later speech')
                await ensemble.pump(page,until="!S47.game.cinematic&&!document.querySelector('.chapter-card:not(.out)')",timeout=15000)
                timed_out=await snapshot(page);open_index=next(i for i,c in enumerate(timed_out['cards']) if c['value'] is not None)
                close=next(c for c in timed_out['cards'][open_index+1:] if c['value'] is None)
                check(timed_out['requests'][0]['done'] is not None and not starts(timed_out) and close['gt']>=error_b['gt']+3.1 and
                      await page.evaluate('__serlingProbe.late.aborted'),f'the product watchdog advances after readable B time while genuine {held_step} remains unresolved')
                await page.evaluate('__serlingProbe.releaseLate()');await ensemble.pump(page,.25)
                check(not starts(await snapshot(page)),f'a watchdog-expired {held_step} cannot revive after the chapter has advanced')
                report['hanging_card'][held_step]={'pending':hanging,'error_b':error_b,'advanced_without_release':timed_out,'after_release':await snapshot(page)}
            sample_card=next(c['value'] for c in report['cards']['prologue']['opening']['cards'] if c['value'] is not None)
            report['card_cancellation']=[]
            for action in ('card-null','reset','Off'):
                await page.evaluate("S47.game.d.ui.close();S47.jump('chapter1');S47.tick(.1);__serlingProbe.reset();__serlingProbe.delayNextLoad()")
                await page.evaluate('o=>{void S47.game.d.ui.chapterCard(o);}',sample_card)
                await wait_for_js(page,'__serlingProbe.late.ready&&S47.narrator.loading')
                await page.evaluate('S47.game.d.ui.chapterCard(null)' if action=='card-null' else 'S47.game.reset()' if action=='reset' else 'S47.narrator.enabled=false')
                cancelled=await snapshot(page)
                check(cancelled['state']=='idle' and not cancelled['pending'] and not cancelled['active'] and (action=='Off' or not cancelled['visibleCard']),action+': pending narrator cancels immediately')
                if action=='Off':
                    check(cancelled['stage']=='text-b' and cancelled['visibleB'],'same-card Serling Off shows the next title as readable B text without speaking it')
                await page.evaluate('__serlingProbe.releaseLate()');await ensemble.pump(page,.2)
                check(not starts(await snapshot(page)) and await page.evaluate('__serlingProbe.late.aborted&&S47.narrator.buffers.size===0'),action+': cancelled genuine decode cannot cache, restart or begin later B speech')
                report['card_cancellation'].append({'action':action,'cancelled':cancelled,'after_release':await snapshot(page)})
                await page.evaluate('S47.game.d.ui.chapterCard(null);S47.narrator.enabled=true')
            # Off keeps the original minimum-time chapter transition; it must not
            # create a narrator source or leave the cinematic waiting forever.
            await page.evaluate("S47.game.d.ui.close();S47.jump('chapter1');S47.tick(.1);S47.narrator.enabled=false;__serlingProbe.reset();S47.ch2.onEnd()")
            await ensemble.pump(page,until='__serlingProbe.cards.some(c=>c.value!==null)')
            await ensemble.pump(page,until="!S47.game.cinematic&&!document.querySelector('.chapter-card:not(.out)')",timeout=20000)
            off=await snapshot(page);check(not starts(off) and not off['requests'],'Serling Off retains the original chapter-card transition with no speech or hang')
            report['cards_off']=off
            mobile=await browser.new_context(viewport={'width':844,'height':390},is_mobile=True,has_touch=True);m=await new_page(mobile)
            await m.evaluate('t=>{__serlingProbe.reset();S47.game.d.ui.toast(t);}',examined)
            await m.locator(HUD).tap();await first_start(m)
            check(await descriptions.hud_clear(m),'844x390 genuine touch HUD narration leaves other controls clear')
            await m.screenshot(path=str(OUT/'serling-mobile.png'));await m.locator(HUD).tap()
            report['mobile_card']=await card_journey(m,'mobile-prologue','',"S47.game.onFinish()",screens=True)
            await ensemble.toggle(m,'serling',False,touch=True);await ensemble.toggle(m,'characters',False,touch=True);await ensemble.toggle(m,'kinski',True,touch=True)
            await m.reload();await start_night(m);await m.evaluate(ensemble.PROBE);await m.evaluate(PROBE)
            check(await m.evaluate("!S47.narrator.enabled&&!S47.npcDialogue.enabled&&S47.game.d.ui.readAloud.enabled&&localStorage.getItem('s47.serlingNarrator')==='false'&&localStorage.getItem('s47.characterVoices')==='false'"),'touch Serling and character preferences reload independently of Kinski')
            await mobile.close()
            check(not bad,'Serling and other recorded MP3 requests have no HTTP failures')
            if not PAGES:check(not requests,'strict-CSP single reads embedded Serling bytes with zero external MP3 requests')
            deliberate=[e for e in errors if 'intentional Serling decode-failure fixture' in e or 'Recording load timed out' in e]
            check(len(deliberate)==3 and sum('intentional Serling decode-failure fixture' in e for e in deliberate)==1 and
                  sum('Recording load timed out' in e for e in deliberate)==2,'only the three specific deliberate decoder-failure/watchdog warnings are observed')
            noise=[e for e in errors if e not in deliberate and not any(s in e for s in ('GPU stall','Automatic fallback to software WebGL','favicon'))]
            check(not noise,'no unexpected browser errors or warnings'+(': '+' | '.join(noise[:4]) if noise else ''))
            report.update(errors=noise,expected_decode_warnings=deliberate,requests=requests,bad_responses=bad);await browser.close()
    except Exception as e:report['fatal']=repr(e);raise
    finally:
        if csp_dir:csp_dir.cleanup()
        report['checks']=CHECKS;report['limits']='Full bank text-resolution and separate CPU PCM audit; representative genuine browser sources and all actual chapter-card variants. Human listening, resemblance, pronunciation, hardware output and device performance UNVERIFIED.'
        (OUT/'serling.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    passed=sum(c['pass'] for c in CHECKS);print(f'{passed} of {len(CHECKS)} PASS',flush=True);return 0 if passed==len(CHECKS) else 1


if __name__=='__main__':sys.exit(asyncio.run(main()))
