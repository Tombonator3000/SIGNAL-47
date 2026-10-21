"""New night opening: original text, real Serling MP3s and actual menu input.

Root runs this serially through slot.sh --alone after the five clips are frozen.
S47_URL selects the Pages build under /SIGNAL-47/. Without it, load the single
HTML from an otherwise empty directory under the existing strict CSP. No fake
PCM or forced DOM/state transition is used. Save seeding, context suspension and
late/failing genuine decode are explicitly labelled fixtures.
"""
import asyncio
import hashlib
import json
import os
import sys
import tempfile
from pathlib import Path

from playwright.async_api import async_playwright
from readaloud import CSP, no_horizontal_overflow, normalized, sample_master, wait_for_js
import ensemble

OUT = Path(sys.argv[1] if len(sys.argv) > 1 else 'production/intro/qa/intro-single')
URL = os.environ.get('S47_URL') or Path('dist-single/index.html').resolve().as_uri()
PAGES = bool(os.environ.get('S47_URL'))
CHECKS = []
BEGIN = '.opening-card [data-a=begin-night]'
BACK = '.opening-card [data-a=back-title]'
PARAGRAPHS = [
    'New Mexico, 1986. Twenty-seven antennas listen to a sky that has never answered. On the night shift, silence is a measurement. It can be filed, signed, and forgotten.',
    'Tonight, the instruments will record something the sky has not sent. A telephone will ring in an empty room. A photograph will remember what no one saw.',
    'Your name is Reyes. Your shift ends at dawn. Between now and then, forty-seven seconds will refuse to belong to any clock.',
    'The record calls it interference.',
    'The record is incomplete.',
]
PROBE = r"""() => {
  if (window.__introProbe) return;
  const a=S47.audio,n=S47.narrator,g=S47.game,s=S47.saves,o=S47.opening;
  const p=window.__introProbe={requests:[],starts:[],removes:[],saves:[],opens:[],samples:[],gate:null};
  const play=n.play.bind(n);n.play=(texts,...rest)=>{
    const r={texts:[...texts],at:a.ctx?.currentTime??null,wall:performance.now(),done:null};p.requests.push(r);
    const result=play(texts,...rest);result.then(()=>r.done=a.ctx?.currentTime??null);return result;};
  const unlock=a.unlock.bind(a);a.unlock=(...args)=>{
    __introGestureCalls.push({kind:'unlock',action:window.__introGestureAction,active:navigator.userActivation.isActive});return unlock(...args);};
  const start=g.start.bind(g);g.start=(...args)=>{p.starts.push({checkpoint:args[0]??null,wall:performance.now()});return start(...args);};
  const remove=s.removeCase.bind(s);s.removeCase=(id)=>{p.removes.push({id,wall:performance.now()});return remove(id);};
  const save=s.save.bind(s);s.save=(...args)=>{p.saves.push({caseId:args[0],kind:args[1],wall:performance.now()});return save(...args);};
  const open=o.open.bind(o);o.open=(choice)=>{p.opens.push({...choice});return open(choice);};
  p.reset=()=>{__readingTrace.reset();p.requests.length=p.starts.length=p.removes.length=p.saves.length=p.opens.length=p.samples.length=0;
    if(window.__introGestureCalls)__introGestureCalls.length=0;
    if(window.__introBusTargets)__introBusTargets.length=0;};
  p.snapshot=()=>({opening:o.status,reader:n.state,pending:!!n.pending,source:!!n.source,
    now:a.ctx?.currentTime??null,context:a.ctx?.state??null,started:S47.started(),clock:g.clock,gt:g.gt,
    phase:g.phase,playtime:S47.playtime(),caseId:S47.caseId(),activeCase:s.active,saves:s.list(),
    roomTone:!!a.roomTone,music:a.loops.has('music'),requests:p.requests,gameStarts:p.starts,removes:p.removes,saveCalls:p.saves,
    opens:p.opens,gate:p.gate?{ready:p.gate.ready,aborted:!!p.gate.signal?.aborted,released:p.gate.released}:null,
    gestures:window.__introGestureCalls||[],busTargets:window.__introBusTargets||[],
    busValues:a.ctx?{sfx:a.sfx.gain.value,amb:a.amb.gain.value}:null,
    paragraphs:[...document.querySelectorAll('.opening-copy p')].map(e=>({text:e.textContent,
      visible:getComputedStyle(e).display!=='none'&&getComputedStyle(e).visibility!=='hidden'&&Number(getComputedStyle(e).opacity)>.01})),
    buttonEnabled:!!document.querySelector('[data-a=begin-night]')&&!document.querySelector('[data-a=begin-night]').disabled,
    domState:document.querySelector('.opening-card')?.dataset.openingState??null,
    status:document.querySelector('[data-opening-status]')?.textContent??null,
    audio:__readingTrace.snapshot().filter(r=>r.serlingId)});
  // Gate only the real-time narrator decode, never AudioSys's offline SFX preload.
  // Let native decoding complete first; no replacement buffer is manufactured.
  p.decodeGate=(mode='late')=>{
    n.buffers.clear();n.bufferBytes=0;const original=BaseAudioContext.prototype.decodeAudioData;let release;
    const gate=new Promise(resolve=>release=resolve);p.gate={ready:false,released:false,signal:null};
    BaseAudioContext.prototype.decodeAudioData=function(...args){
      if(this!==a.ctx||!n.loading||!window.__introMp3ByteSizes.includes(args[0].byteLength))return original.apply(this,args);
      BaseAudioContext.prototype.decodeAudioData=original;
      return original.apply(this,args).then(async buffer=>{
        p.gate.signal=n.pending?.signal;p.gate.ready=true;
        if(mode==='fail')throw new Error('intro deliberate decode-failure fixture');
        await gate;return buffer;});};
    p.release=()=>{p.gate.released=true;release();};
  };
  p.persistence=async()=>{await s.queue;const active=s.active,metas=s.list(),rows=[];
    for(const m of metas)rows.push({meta:m,state:await s.load(m.id)});
    if(active)s.setActive(active);return {active,rows};};
  return true;
}"""
GESTURES = r"""(() => {
  window.__introGestureCalls=[];window.__introGestureAction=null;window.__introBusTargets=[];
  const target=AudioParam.prototype.setTargetAtTime;
  AudioParam.prototype.setTargetAtTime=function(value,at,constant){
    const a=window.S47?.audio,bus=this===a?.sfx?.gain?'sfx':this===a?.amb?.gain?'amb':null;
    if(bus)__introBusTargets.push({bus,value,at,constant});
    return target.call(this,value,at,constant);};
  for(const type of ['pointerdown','keydown','click'])addEventListener(type,e=>{
    window.__introGestureAction=e.target instanceof Element?e.target.closest('[data-a]')?.dataset.a??null:null;
  },true);
  const lock=HTMLElement.prototype.requestPointerLock;
  HTMLElement.prototype.requestPointerLock=function(...args){
    __introGestureCalls.push({kind:'pointerLock',action:window.__introGestureAction,active:navigator.userActivation.isActive});return lock.apply(this,args);};
  const full=Element.prototype.requestFullscreen;
  if(full)Element.prototype.requestFullscreen=function(...args){
    __introGestureCalls.push({kind:'fullscreen',action:window.__introGestureAction,active:navigator.userActivation.isActive});return full.apply(this,args);};
})();"""


def check(ok, message):
    CHECKS.append({'pass': bool(ok), 'message': message})
    print(('PASS ' if ok else 'FAIL ') + message, flush=True)


def audio(state):
    return state['audio']


def cancelled(rows):
    return all(r.get('stoppedAt') is not None or r.get('endedAt') is not None for r in rows)


def unchanged(before, after):
    return all(before[k] == after[k] for k in ('gt', 'clock', 'phase', 'playtime', 'caseId', 'activeCase', 'saves'))


async def snapshot(page):
    return await page.evaluate('__introProbe.snapshot()')


async def click(page, selector, touch=False):
    button = page.locator(selector)
    await button.wait_for(state='visible')
    await button.scroll_into_view_if_needed()
    if touch:
        await button.tap()
    else:
        await button.click()


async def new_night(page, touch=False):
    await click(page, '.title-screen [data-a=start]', touch)
    await page.locator(BEGIN).wait_for(state='visible')


async def title(page):
    await page.keyboard.press('Escape')
    await click(page, '.settings-panel [data-a=title]')
    await page.locator('.title-screen [data-a=start]').wait_for(state='visible')
    await page.evaluate('async()=>{await S47.saves.queue;__introProbe.reset();}')


async def begun(page):
    await wait_for_js(page, 'S47.started() && !S47.opening.active && S47.audio.ready')
    await page.evaluate('S47.hold=true')
    return await snapshot(page)


async def first_audio(page):
    await wait_for_js(page, '__introProbe.snapshot().audio.length>0 && S47.narrator.playing')
    return await snapshot(page)


async def readable(page, name):
    data = await page.evaluate("""() => {
      const e=document.querySelector('.opening-card'),ps=[...e.querySelectorAll('.opening-copy p')];
      const r=e.getBoundingClientRect();return {width:innerWidth,height:innerHeight,scrollHeight:e.scrollHeight,
        clientHeight:e.clientHeight,paragraphs:ps.map(p=>({text:p.textContent,font:parseFloat(getComputedStyle(p).fontSize),
          contrast:[getComputedStyle(p).color,getComputedStyle(e).backgroundColor],height:p.getBoundingClientRect().height})),
        bounds:{left:r.left,right:r.right,top:r.top,bottom:r.bottom}};
    }""")
    check(await no_horizontal_overflow(page, '.opening-card') and
          [p['text'] for p in data['paragraphs']] == PARAGRAPHS and
          all(p['font'] >= 14 and p['height'] > 0 for p in data['paragraphs']),
          name + ': all five original paragraphs remain readable without horizontal overflow')
    # Scrolling the real card must expose both controls even on the short phone.
    await page.locator(BEGIN).scroll_into_view_if_needed()
    check(await page.locator(BEGIN).is_visible() and await page.locator(BACK).is_visible(),
          name + ': Begin night and Back to title are reachable')
    await page.evaluate("document.querySelector('.opening-card').scrollTop=0")
    await page.screenshot(path=str(OUT / (name + '.png')))
    return data


async def main():
    global URL
    OUT.mkdir(parents=True, exist_ok=True)
    catalog = json.loads(Path('src/assets/audio/serling-intro/catalog.json').read_text())
    clips = catalog['clips']
    check(len(clips) == 5 and [c['text'] for c in clips] == PARAGRAPHS and all(
          c['id'] == hashlib.sha256(normalized(c['text']).encode()).hexdigest()[:16] and c['duration'] > 0 and
          Path('src/assets/audio/serling-intro', c['file']).is_file() for c in clips),
          'the separate five-clip intro bank exactly matches the frozen original manuscript and actual MP3 files')
    if not CHECKS[-1]['pass']:
        raise AssertionError('Finish the exact five-clip intro bank before browser QA')
    ids = [c['id'] for c in clips]
    report = {'pages': PAGES, 'strict_csp': not PAGES, 'clips': clips,
              'fixtures': 'Real title/New night/Begin/Back/settings/Continue/Load/DEV input. Explicit API fixtures only seed full save slots, suspend the context, and hold or reject a genuine native decode. No source or PCM substitution.'}
    errors, mp3_requests, bad_responses = [], [], []
    csp_dir = None
    if not PAGES:
        csp_dir = tempfile.TemporaryDirectory(prefix='s47-intro-csp-')
        target = Path(csp_dir.name, 'index.html')
        target.write_text(Path('dist-single/index.html').read_text().replace(
            '<head>', '<head><meta http-equiv="Content-Security-Policy" ' + f'content="{CSP}">', 1))
        URL = target.as_uri()
    report['url'] = URL
    try:
        async with async_playwright() as p:
            browser = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'])
            async def page_in(ctx, *, voice=None, real_gate=False, dev=False):
                pg = await ctx.new_page(); pg.set_default_timeout(60000)
                await pg.add_init_script(ensemble.art.REAL_AUDIO + '\n' + ensemble.GRAPH + '\n' + GESTURES +
                                         '\nwindow.__introMp3ByteSizes=' + json.dumps([Path('src/assets/audio/serling-intro',c['file']).stat().st_size for c in clips]) + ';')
                if real_gate:
                    await pg.add_init_script("Object.defineProperty(Navigator.prototype,'webdriver',{get:()=>false});")
                if voice is not None:
                    await pg.add_init_script("localStorage.setItem('s47.serlingNarrator'," + ("'true'" if voice else "'false'") + ");")
                pg.on('pageerror', lambda e: errors.append('PAGEERROR: ' + str(e)))
                pg.on('console', lambda m: errors.append(m.type + ': ' + m.text) if m.type in ('error', 'warning') else None)
                pg.on('request', lambda r: mp3_requests.append(r.url) if r.url.startswith(('http:', 'https:')) and r.url.split('?', 1)[0].endswith('.mp3') else None)
                pg.on('response', lambda r: bad_responses.append([r.status, r.url]) if r.status >= 400 and r.url.split('?', 1)[0].endswith('.mp3') else None)
                await pg.goto(URL + ('?dev' if dev else ''))
                await wait_for_js(pg, '!!window.S47?.opening')
                await pg.evaluate(PROBE)
                return pg

            desktop = await browser.new_context(viewport={'width': 1280, 'height': 800})
            page = await page_in(desktop, real_gate=True)
            check(await page.locator('.title-screen').evaluate("e=>e.classList.contains('gated')"),
                  'the production first-gesture title gate is retained')
            await click(page, '.title-screen .gate')
            await wait_for_js(page, "!document.querySelector('.title-screen').classList.contains('gated')")
            check(not await page.evaluate('S47.opening.active||S47.started()'),
                  'the initial real gesture reveals the menu without starting an intro or a night')
            check(await page.evaluate('S47.narrator.enabled && !S47.game.d.ui.readAloud.enabled'),
                  'Serling defaults On with the independent Kinski option Off')
            await page.evaluate('__introProbe.reset()')
            baseline = await snapshot(page)
            await new_night(page)
            await first_audio(page)
            opening = await snapshot(page)
            check(opening['opening']['active'] and not opening['started'] and opening['requests'][0]['texts'] == PARAGRAPHS and
                  all(p['visible'] for p in opening['paragraphs']) and opening['buttonEnabled'],
                  'actual New night shows all five paragraphs and starts the exact narration while Begin remains available')
            check(unchanged(baseline, opening) and not opening['saveCalls'] and not opening['gameStarts'] and
                  not opening['roomTone'] and not opening['music'],
                  'intro leaves clock, playtime, progress, cases, autosaves and the playable prologue untouched and silences menu music')
            check(await page.evaluate('S47.narrator.available(' + json.dumps(PARAGRAPHS) + ')'),
                  'the real narrator resolver covers all five exact intro paragraphs')
            # Keyboard navigation uses native input, not a synthetic click or forced focus.
            await page.keyboard.press('Tab')
            check(await page.locator(BACK).evaluate('e=>e===document.activeElement'),
                  'Tab moves from Begin night to Back to title')
            await page.keyboard.press('Shift+Tab')
            check(await page.locator(BEGIN).evaluate('e=>e===document.activeElement'),
                  'Shift+Tab moves back to Begin night')
            report['desktop_layout'] = await readable(page, 'intro-desktop')
            level = await sample_master(page,1000)
            check(level['peakRms'] > 1e-5, 'genuine intro PCM reaches master while unrelated title/game audio is stopped')
            report['master_samples'] = level
            await page.evaluate('async()=>{await S47.audio.ctx.suspend();}')
            held = await snapshot(page); await asyncio.sleep(.35); paused = await snapshot(page)
            check(held['context'] == paused['context'] == 'suspended' and held['now'] == paused['now'] and
                  len(audio(held)) == len(audio(paused)) and paused['opening']['active'],
                  'suspending the real AudioContext freezes the current intro queue without starting the night')
            await page.evaluate('async()=>{await S47.audio.ctx.resume();}')
            await wait_for_js(page, "S47.opening.state==='ready' && S47.narrator.state==='idle'", timeout=120000)
            done = await snapshot(page); rows = audio(done)
            check([r['serlingId'] for r in rows] == ids and ensemble.natural(rows) and all(ensemble.direct(r) for r in rows),
                  'all five genuine MP3 buffers play once in paragraph order at rate 1 directly to master and finish naturally')
            check(done['opening']['active'] and unchanged(baseline, done) and not done['gameStarts'] and not done['saveCalls'],
                  'natural narration completion keeps the readable card open until the player chooses Begin night')
            report['natural'] = {'before': baseline, 'opening': opening, 'paused': paused, 'finished': done}
            await click(page, BEGIN)
            started = await begun(page)
            check(len(started['gameStarts']) == 1 and started['phase'] == 'intro' and abs(started['clock'] - (23 * 3600 + 41 * 60)) < 2 and
                  started['reader'] == 'idle' and not started['pending'] and not started['source'],
                  'real Begin night enters the unchanged 23:41 playable prologue exactly once after narration')
            check(any(g['kind'] == 'pointerLock' and g['action']=='begin-night' and g['active'] for g in started['gestures']),
                  'Begin preserves a fresh real user gesture for the existing pointer-lock request')
            # Use one real save as a seed for the menu/overwrite fixtures below.
            await page.evaluate("S47.jump('chapter1');S47.tick(.5)")
            saved = await page.evaluate("S47.saveNow('manual',0)")
            if not saved:
                raise AssertionError('The real save fixture could not be written')
            await title(page)
            before = await snapshot(page)
            await click(page, '.title-screen [data-a=cont]'); await begun(page)
            after = await snapshot(page)
            check(not after['opens'] and len(after['gameStarts']) == 1 and after['phase'] == 'ch1' and not audio(after),
                  'real Continue restores the saved chapter directly with no opening card or intro voice')
            await title(page)
            await click(page, '.title-screen [data-a=load]')
            await click(page, '.savemenu [data-load="' + saved['id'] + '"]'); await begun(page)
            after = await snapshot(page)
            check(not after['opens'] and len(after['gameStarts']) == 1 and after['phase'] == 'ch1' and not audio(after),
                  'real Load case restores the selected save directly without opening narration')
            report['bypass'] = {'continue': True, 'load': after}
            await desktop.close()

            # Independent fresh contexts keep New night on the free-case route.
            report['cancellation'] = []
            for pending in (False, True):
                for action in ('begin', 'back', 'escape'):
                    ctx = await browser.new_context(viewport={'width':1280,'height':800})
                    pg = await page_in(ctx)
                    if pending:
                        await pg.evaluate("__introProbe.decodeGate('late')")
                    before = await snapshot(pg); await new_night(pg)
                    if pending:
                        await wait_for_js(pg, '__introProbe.gate.ready && S47.narrator.loading')
                    else:
                        await first_audio(pg)
                    active = await snapshot(pg)
                    if action == 'begin': await click(pg, BEGIN)
                    elif action == 'back': await click(pg, BACK)
                    else: await pg.keyboard.press('Escape')
                    if pending: await pg.evaluate('__introProbe.release()')
                    if action == 'begin': await begun(pg)
                    else: await page_focus_title(pg)
                    await asyncio.sleep(.2); after = await snapshot(pg)
                    check(not after['opening']['active'] and after['reader']=='idle' and not after['source'] and not after['pending'] and
                          (not audio(after) if pending else cancelled(audio(after))) and (not pending or after['gate']['aborted']),
                          ('loading' if pending else 'playing') + ' ' + action + ': the real control cancels the source and late native decode cannot revive it')
                    check((after['started'] and len(after['gameStarts'])==1) if action=='begin' else
                          (not after['started'] and unchanged(before,after) and not after['removes'] and not after['saveCalls']),
                          ('loading' if pending else 'playing') + ' ' + action + ': starts one night only for Begin; cancellation preserves the untouched case')
                    report['cancellation'].append({'pending':pending,'action':action,'before':before,'active':active,'after':after})
                    await ctx.close()

            # A real external-modal replacement used to dismiss the card while
            # leaving the title's music stopped and SFX/ambience targets at zero.
            # Observe actual AudioParam scheduling, not a guessed ramp delay.
            report['dev_dismissal'] = []
            for pending in (False, True):
                ctx = await browser.new_context(viewport={'width':1280,'height':800})
                pg = await page_in(ctx,dev=True)
                if pending: await pg.evaluate("__introProbe.decodeGate('late')")
                before = await snapshot(pg); await new_night(pg)
                if pending: await wait_for_js(pg,'__introProbe.gate.ready && S47.narrator.loading')
                else: await first_audio(pg)
                active = await snapshot(pg)
                await pg.keyboard.press('F2')
                await pg.locator('.devmenu').wait_for(state='visible')
                replaced = await snapshot(pg)
                check(not replaced['opening']['active'] and replaced['reader']=='idle' and
                      not replaced['pending'] and not replaced['source'] and not await pg.locator('.opening-card').count() and
                      (not audio(replaced) if pending else cancelled(audio(replaced))),
                      ('loading' if pending else 'playing')+' intro → real F2: the DEV modal dismisses narration and removes its card')
                await click(pg,'.devmenu [data-a=close]')
                await pg.locator('.title-screen [data-a=start]').wait_for(state='visible')
                if pending: await pg.evaluate('__introProbe.release()')
                await wait_for_js(pg,"""() => {const s=__introProbe.snapshot();return s.music && s.context==='running' &&
                  ['sfx','amb'].every(bus=>s.busTargets.findLast(t=>t.bus===bus)?.value===1);}""",timeout=10000)
                await asyncio.sleep(.2); after = await snapshot(pg)
                check(not after['started'] and unchanged(before,after) and not after['gameStarts'] and not after['saveCalls'] and
                      not after['opening']['active'] and after['reader']=='idle' and not after['source'] and not after['pending'] and
                      (not audio(after) if pending else cancelled(audio(after))) and (not pending or after['gate']['aborted']),
                      ('loading' if pending else 'playing')+' intro → F2 → Close: no late voice, night, case change or autosave is revived')
                check(after['music'] and after['context']=='running' and all(
                      next((t['value'] for t in reversed(after['busTargets']) if t['bus']==bus),None)==1 for bus in ('sfx','amb')),
                      ('loading' if pending else 'playing')+' intro → F2 → Close: title music runs and both actual bus gain targets return to 1')
                report['dev_dismissal'].append({'pending':pending,'before':before,'active':active,'replaced':replaced,'after':after})
                await ctx.close()

            ctx = await browser.new_context(viewport={'width':1280,'height':800})
            pg = await page_in(ctx)
            await new_night(pg)
            await pg.locator(BEGIN).dblclick()
            double = await begun(pg); await asyncio.sleep(.2)
            check(len((await snapshot(pg))['gameStarts'])==1 and len(double['opens'])==1,
                  'a real double-click on Begin night starts exactly one run')
            report['double_click'] = await snapshot(pg); await ctx.close()

            report['failures'] = []
            for mode in ('fail', 'never'):
                ctx = await browser.new_context(viewport={'width':1280,'height':800})
                pg = await page_in(ctx); await pg.evaluate('mode=>__introProbe.decodeGate(mode)', mode)
                await new_night(pg); await wait_for_js(pg, '__introProbe.gate.ready')
                await wait_for_js(pg, "S47.opening.state==='error'", timeout=20000)
                failed = await snapshot(pg)
                check(failed['opening']['active'] and not failed['started'] and not audio(failed) and failed['buttonEnabled'] and
                      all(p['visible'] for p in failed['paragraphs']) and 'unavailable' in failed['status'].lower(),
                      mode + ': narration failure leaves every paragraph and Begin available without entering the prologue')
                await click(pg, BEGIN); await begun(pg)
                # Keep the never-settling decoder unresolved until after the watchdog
                # and actual Begin have already succeeded, then prove no stale start.
                if mode=='never': await pg.evaluate('__introProbe.release()')
                await asyncio.sleep(.2); after = await snapshot(pg)
                check(len(after['gameStarts'])==1 and not audio(after) and after['reader']=='idle',
                      mode + ': the real Begin works after failure/watchdog and a later decode cannot start stale intro speech')
                report['failures'].append({'mode':mode,'failed':failed,'after':after}); await ctx.close()

            # Persisted Off is chosen through the actual title settings, without
            # changing the character/Kinski/radio options behind the UI.
            ctx = await browser.new_context(viewport={'width':1280,'height':800})
            pg = await page_in(ctx)
            await click(pg,'.title-screen [data-a=set]')
            await click(pg,'.settings-panel [data-a=serling]')
            check(await pg.locator('.settings-panel [data-a=serling]').get_attribute('aria-pressed')=='false',
                  'the actual title Serling setting can be switched Off')
            await click(pg,'.settings-panel [data-a=resume]')
            await pg.reload(); await wait_for_js(pg,'!!window.S47?.opening'); await pg.evaluate(PROBE)
            await pg.evaluate('async()=>{await S47.audio.preload();}')
            before_requests = len(mp3_requests)
            await new_night(pg); await asyncio.sleep(.3); off = await snapshot(pg)
            check(not await pg.evaluate('S47.narrator.enabled') and off['opening']['state']=='ready' and not off['requests'] and
                  not audio(off) and len(mp3_requests)==before_requests and len(off['paragraphs'])==5,
                  'Serling Off survives reload and shows the full opening without loading or starting any intro recording')
            await pg.keyboard.press('Tab'); await pg.keyboard.press('Shift+Tab'); await pg.keyboard.press('Space')
            await begun(pg)
            check(len((await snapshot(pg))['gameStarts'])==1,
                  'native Space activates the focused Begin night button with Serling Off')
            report['voice_off'] = off; await ctx.close()

            # All three occupied case slots are a save API fixture, made from a
            # genuine playable save. Every destructive choice below uses its UI.
            ctx = await browser.new_context(viewport={'width':1280,'height':800})
            pg = await page_in(ctx, voice=False)
            await new_night(pg); await click(pg,BEGIN); await begun(pg)
            await pg.evaluate("S47.jump('chapter1');S47.tick(.5)")
            seeded = await pg.evaluate("""async()=>{const m=await S47.saveNow('manual',0),state=await S47.saves.load(m.id);
              for(const c of [2,3])await S47.saves.save(c,'manual',0,state,m);S47.saves.setActive(1);return m;}""")
            await title(pg); original = await pg.evaluate('__introProbe.persistence()')
            await click(pg,'.title-screen [data-a=start]')
            await click(pg,'.savemenu [data-new="1"]'); await click(pg,'.savemenu [data-no]')
            check(await pg.evaluate('__introProbe.persistence()')==original and not await pg.locator('.opening-card').count(),
                  'full-case replacement confirmation Cancel preserves all saves and never opens an intro')
            await click(pg,'.savemenu [data-a=back]')
            await click(pg,'.title-screen [data-a=start]'); await click(pg,'.savemenu [data-new="1"]'); await click(pg,'.savemenu [data-yes]')
            await pg.locator(BEGIN).wait_for()
            check(await pg.evaluate('__introProbe.persistence()')==original and not (await snapshot(pg))['removes'],
                  'confirming an occupied case only opens the card; old save content remains intact before Begin')
            await click(pg,BACK); await page_focus_title(pg)
            check(await pg.evaluate('__introProbe.persistence()')==original,
                  'Back from the selected replacement intro preserves every original save')
            await click(pg,'.title-screen [data-a=start]'); await click(pg,'.savemenu [data-new="1"]'); await click(pg,'.savemenu [data-yes]')
            await pg.locator(BEGIN).wait_for(); await pg.keyboard.press('Escape'); await page_focus_title(pg)
            check(await pg.evaluate('__introProbe.persistence()')==original,
                  'Escape from a selected replacement also preserves every original save')
            await click(pg,'.title-screen [data-a=start]'); await click(pg,'.savemenu [data-new="1"]'); await click(pg,'.savemenu [data-yes]')
            await pg.locator(BEGIN).wait_for(); await pg.locator(BEGIN).dblclick(); await begun(pg)
            replacement = await pg.evaluate('__introProbe.persistence()'); final = await snapshot(pg)
            other_before = [r for r in original['rows'] if r['meta']['caseId']!=1]
            other_after = [r for r in replacement['rows'] if r['meta']['caseId']!=1]
            check(len(final['removes'])==1 and final['removes'][0]['id']==1 and len(final['gameStarts'])==1 and
                  other_after==other_before and not any(r['meta']['id']==seeded['id'] for r in replacement['rows']),
                  'only the final genuine Begin deletes the chosen old case once and preserves the other two cases byte-for-byte as JSON')
            report['full_cases'] = {'before':original,'after':replacement,'state':final}; await ctx.close()

            ctx = await browser.new_context(viewport={'width':1280,'height':800})
            pg = await page_in(ctx,dev=True)
            await click(pg,'.devbtn'); await click(pg,'.devmenu button.dev:text-is("3 The Survey Station")')
            await wait_for_js(pg,"S47.game.phase==='ch3' && !document.querySelector('.fade.on')")
            dev = await snapshot(pg)
            check(not dev['opens'] and not audio(dev) and len(dev['gameStarts'])==1 and not dev['saves'],
                  'the actual DEV chapter button bypasses the opening and keeps its unsaved run contract')
            report['dev'] = dev; await ctx.close()

            report['mobile'] = []
            for width,height in ((844,390),(390,844)):
                ctx = await browser.new_context(viewport={'width':width,'height':height},is_mobile=True,has_touch=True)
                pg = await page_in(ctx)
                await new_night(pg,touch=True); await first_audio(pg)
                name=f'intro-mobile-{width}x{height}'
                layout = await readable(pg,name)
                active = await snapshot(pg)
                check(active['requests'][0]['texts']==PARAGRAPHS and active['opening']['active'] and not active['started'],
                      name+': genuine New night tap starts the same exact narrator queue before gameplay')
                await click(pg,BACK,touch=True); await page_focus_title(pg)
                await pg.evaluate('__introProbe.reset()'); await new_night(pg,touch=True); await click(pg,BEGIN,touch=True)
                mobile = await begun(pg)
                check(len(mobile['gameStarts'])==1 and mobile['reader']=='idle' and cancelled(audio(mobile)) and
                      any(g['kind']=='unlock' and g['action']=='begin-night' and g['active'] for g in mobile['gestures']),
                      name+': actual Back and Begin taps cancel narration and preserve the new-start gesture')
                report['mobile'].append({'layout':layout,'active':active,'started':mobile}); await ctx.close()

            check(not bad_responses,'all requested recorded MP3 files load without an HTTP failure')
            if not PAGES:
                check(not mp3_requests,'strict-CSP offline HTML reads embedded MP3 bytes with zero external MP3 requests')
            else:
                check('/SIGNAL-47/' in URL and all('/SIGNAL-47/' in u for u in mp3_requests),
                      'the ordinary build resolves all real MP3 requests under the Pages subdirectory')
            expected = ('intro deliberate decode-failure fixture','Recording load timed out')
            controlled = [e for e in errors if any(t in e for t in expected)]
            noise = [e for e in errors if not any(t in e for t in (*expected,'GPU stall','Automatic fallback to software WebGL','favicon'))]
            check(not noise,'no unexpected browser errors, warnings or CSP violations'+(': '+' | '.join(noise[:3]) if noise else ''))
            report.update(errors=noise,controlled_failure_warnings=controlled,mp3_requests=mp3_requests,bad_responses=bad_responses)
            await browser.close()
    except Exception as e:
        report['fatal'] = repr(e)
        raise
    finally:
        if csp_dir:csp_dir.cleanup()
        report['checks'] = CHECKS
        report['limits'] = 'Real native MP3 decode/start/end, graph routing, PCM and actual menu keyboard/touch input. Pointer-lock acquisition is a declared headless fixture, but the Begin gesture is observed. Save seeding and decoder gates are explicit fixtures. Human listening, Serling identity/pronunciation and physical-device output/FPS UNVERIFIED.'
        (OUT/'intro.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    count = sum(c['pass'] for c in CHECKS)
    print(f'{count} of {len(CHECKS)} PASS',flush=True)
    return 0 if count==len(CHECKS) else 1


async def page_focus_title(page):
    await page.locator('.title-screen [data-a=start]').wait_for(state='visible')
    check(await page.locator('.title-screen [data-a=start]').evaluate('e=>e===document.activeElement'),
          'Back/Escape returns keyboard focus to the real New night button')


if __name__=='__main__':
    sys.exit(asyncio.run(main()))
