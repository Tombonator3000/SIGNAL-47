"""Character cast: real MP3 decoding, authored interactions and queue ownership.

Root runs this serially after TTS. S47_URL selects Pages under /SIGNAL-47/;
otherwise the offline HTML is copied into a clean, strict-CSP directory.
The full-bank case is an explicit manuscript fixture: it starts each real clip,
then cancels it. Authored journeys below separately prove natural completion.
"""
import asyncio
import hashlib
import importlib.util
import json
import os
import sys
import tempfile
from collections import Counter
from pathlib import Path

from playwright.async_api import async_playwright
from readaloud import (CSP, no_horizontal_overflow, normalized, quiet_background,
                       sample_master, start_night, wait_for_js)

spec = importlib.util.spec_from_file_location("ensemble_art_probe", Path(__file__).with_name("artbell-radio.py"))
art = importlib.util.module_from_spec(spec)
spec.loader.exec_module(art)
OUT = Path(sys.argv[1] if len(sys.argv) > 1 else "production/ensemble/qa/ensemble-single")
URL = os.environ.get("S47_URL") or Path("dist-single/index.html").resolve().as_uri()
PAGES = bool(os.environ.get("S47_URL"))
CHECKS = []
SPEAKERS = {"stanton": "THE ATTENDANT", "stack": "THE DEPUTY", "lynch": "THE SOUVENIR MAN",
            "bacall": "N. VEGA", "price": "THE LINE"}
CONTROL = '[data-reading-kind="character-dialogue"] [data-a=character]'

# Observe graph edges and real buffer identity without altering sound or timing.
GRAPH = art.RADIO_GRAPH.replace(
    "record.hostId = [...s.radio.hostBank.buffers].find(([, b]) => b === this.buffer)?.[0] || null;",
    """record.hostId = [...s.radio.hostBank.buffers].find(([, b]) => b === this.buffer)?.[0] || null;
      record.castId = [...(s.npcDialogue?.bank.buffers||[])].find(([,b])=>b===this.buffer)?.[0] ||
        Object.keys(audio.buf).find(k=>k.startsWith('character:')&&audio.buf[k]===this.buffer)?.slice(10)||null;
      record.earlId = [...(s.earlDialogue?.bank.buffers||[])].find(([,b])=>b===this.buffer)?.[0]||null;
      record.serlingId = [...(s.narrator?.buffers||[])].find(([,b])=>b===this.buffer)?.[0]||null;
      record.kinski = s.game.d.ui.readAloud.source===this;
      record.masterPath = (path(this,audio.master)||[]).map(n=>n instanceof BiquadFilterNode
        ? {kind:n.type,hz:n.frequency.value}: {kind:n===audio.master?'master':n===audio.radioBus?'radioBus':n===audio.sfx?'sfx':n.constructor.name});""")
PROBE = r"""() => {
  if(window.__ensembleProbe)return;
  const a=S47.audio,e=S47.npcDialogue,ui=S47.game.d.ui;
  const p=window.__ensembleProbe={clock:a.ctx.currentTime,remainder:0,calls:[],phones:[],captions:[],cb:[],late:null};
  window.__artProbe=p;
  const play=e.play.bind(e);e.play=(input,...rest)=>{
    p.calls.push({voice:input.voice,lines:input.lines.map(l=>({...l})),at:a.ctx.currentTime});return play(input,...rest);};
  const phone=a.phoneCall.bind(a);a.phoneCall=(lines,options)=>{
    const at=a.ctx.currentTime,gt=S47.game.gt,timeline=phone(lines,options);
    p.phones.push({at,gt,lines:lines.map(l=>({...l})),options,timeline});return timeline;};
  const toast=ui.toast.bind(ui);ui.toast=(text,seconds,...rest)=>{
    p.captions.push({text,seconds,at:a.ctx.currentTime,gt:S47.game.gt,audioClock:typeof rest[0]==='function'});
    return toast(text,seconds,...rest);};
  const cb=S47.cb.call.bind(S47.cb);S47.cb.call=(lines,at,onLine,onEnd)=>{
    const record={lines,at:a.ctx.currentTime,callbacks:[],ended:null};p.cb.push(record);
    const duration=cb(lines,at,(l,...rest)=>{record.callbacks.push({line:l,at:a.ctx.currentTime,rest});onLine(l,...rest);},
      ()=>{record.ended=a.ctx.currentTime;onEnd?.();});record.duration=duration;return duration;};
  p.tick=()=>{const now=a.ctx.currentTime;p.remainder+=Math.max(0,now-p.clock);p.clock=now;
    const n=Math.floor(p.remainder*60);if(n){p.remainder-=n/60;S47.tick(n/60,60);}return {now,context:a.ctx.state};};
  p.reset=()=>{__readingTrace.reset();p.calls.length=p.phones.length=p.captions.length=p.cb.length=0;
    p.clock=a.ctx.currentTime;p.remainder=0;};
  p.snapshot=()=>({now:a.ctx.currentTime,gt:S47.game.gt,context:a.ctx.state,status:e.status,
    pending:!!e.pending,active:!!e.source,calls:p.calls,phones:p.phones,captions:p.captions,cb:p.cb,
    starts:__readingTrace.snapshot().filter(r=>r.castId||r.kinski||r.earlId||r.serlingId),
    lines:[...document.querySelectorAll('.talk-line')].map(el=>{const who=el.querySelector('.who')?.textContent||'',
      copy=el.cloneNode(true);copy.querySelector('.who')?.remove();return {who,text:copy.textContent.trim()};}),
    choices:[...document.querySelectorAll('.talk [data-c]')].map(b=>b.dataset.c)});
  p.delayNextLoad=()=>{const bank=e.bank,original=bank.load;let release;
    const gate=new Promise(resolve=>release=resolve);p.late={ready:false,aborted:false};bank.buffers.clear();bank.bytes=0;
    bank.load=async function(...args){bank.load=original;const result=await original.apply(this,args);
      p.late.ready=true;await gate;p.late.aborted=args[2].aborted;return result;};p.releaseLate=()=>release();};
}"""


def check(ok, message):
    CHECKS.append({"pass": bool(ok), "message": message})
    print(("PASS " if ok else "FAIL ") + message, flush=True)


def cast_starts(state):
    return [r for r in state["starts"] if r.get("castId")]


def natural(records):
    return bool(records) and all(r.get("endedAt") is not None and
        r["endedAt"] >= r["expectedEnd"] - .08 and
        (r.get("stoppedAt") is None or r["stoppedAt"] >= r["expectedEnd"] - .08) for r in records)


def direct(record):
    path = [n["kind"] for n in record.get("masterPath", [])]
    return art.mp3(record) and record.get("master") and record.get("rate") == 1 and path[-1:] == ["master"] and not any(
        n in path for n in ("radioBus", "sfx", "bandpass", "lowpass", "highpass"))


def telephone(record):
    path = record.get("masterPath", [])
    return art.mp3(record) and record.get("master") and record.get("rate") == 1 and any(
        n["kind"] == "highpass" and abs(n["hz"] - 320) < 1 for n in path) and any(
        n["kind"] == "lowpass" and abs(n["hz"] - 3200) < 1 for n in path) and any(n["kind"] == "sfx" for n in path)


async def snapshot(page):
    return await page.evaluate("__ensembleProbe.snapshot()")


async def pump(page, seconds=0, until=None, timeout=120000):
    return await art.pump(page, seconds=seconds, until=until, timeout=timeout)


async def first_start(page, count=0):
    await pump(page, until=f"__readingTrace.snapshot().filter(r=>r.castId).length>{count}")
    return await snapshot(page)


async def toggle(page, name, value, touch=False):
    await page.evaluate("S47.game.d.ui.close()")
    await art.pause(page, touch=touch)
    b = page.locator(f'.settings-panel [data-a="{name}"]')
    await b.scroll_into_view_if_needed()
    if (await b.get_attribute("aria-pressed") == "true") != value:
        await (b.tap() if touch else b.click())
    await art.resume(page, touch=touch)


async def prepare_town(page, fresh=True):
    if not await page.evaluate("S47.started()"):
        await start_night(page); await page.evaluate(PROBE)
    await page.evaluate("""fresh=>{S47.game.d.ui.close();S47.jump('chapter1');S47.tick(.3,60);
      S47.setAbductions(false);S47.nightEvents.wait=1e9;
      if(fresh)for(const k of [...S47.townSeen])if(k.startsWith('met:')||k.startsWith('ask:'))S47.townSeen.delete(k);
    }""", fresh)
    await page.evaluate("async()=>{await S47.world.prepare('diner');S47.world.enter('diner');}")
    await quiet_background(page)
    await page.evaluate("S47.carRadio.set(false);S47.dinerRadio.set(false)")


async def aim(page, proxy):
    await page.evaluate("""key=>{S47.game.d.ui.close();S47.game.clock=2*3600+35*60;
      S47.world.town.update(S47.runTime(),S47.game.clock);
      const at=S47.world.town.proxies[key].getWorldPosition(S47.camera.position.clone()),p=S47.player;
      // The gas station and sheriff face east onto the street; the smoker's
      // stoop faces west. Stand outside the facade and aim at the real proxy.
      const side=['gasWindow','sheriffDoor'].includes(key)?1:-1;
      p.place(at.x+side*1.7,at.z,side*Math.PI/2);p.pitch=Math.atan2(at.y-(p.floorY+p.eye),1.7);S47.tick(.05,60);
    }""", proxy)
    try:
        await pump(page, until=f"!S47.game.cinematic && S47.game.d.inter.update(S47.camera)?.id==='town:{proxy}'", timeout=15000)
    except TimeoutError as error:
        state=await page.evaluate("""key=>({proxy:key,at:S47.world.town.proxies[key].getWorldPosition(S47.camera.position.clone()).toArray(),
          player:S47.player.pos.toArray(),camera:S47.camera.position.toArray(),yaw:S47.player.yaw,pitch:S47.player.pitch,
          current:S47.game.d.inter.update(S47.camera)?.id,cinematic:S47.game.cinematic,mode:S47.game.mode,area:S47.world.area})""",proxy)
        raise AssertionError('Real town target was not acquired: '+json.dumps(state)) from error


async def open_person(page, proxy, voice, touch=False):
    await aim(page, proxy)
    if touch:
        await pump(page, until="document.querySelector('.touch .use')?.classList.contains('ready')", timeout=15000)
        await page.locator(".touch .use").tap()
    else:
        await page.keyboard.press("e")
    await page.wait_for_selector(f'[data-reading-kind="character-dialogue"][data-voice="{voice}"]')
    await first_start(page)
    check(await page.locator(CONTROL).count() == 1,
          f"actual {'touch Use' if touch else 'aimed E'} opens the authored {voice} conversation")


async def finish_exchange(page, label, before=0, expected=None):
    await pump(page, until="S47.npcDialogue.state==='idle'&&!S47.npcDialogue.pending&&!S47.npcDialogue.source")
    state = await snapshot(page); starts = cast_starts(state)[before:]
    if expected is None:
        expected = await page.locator('[data-reading-kind="character-dialogue"]').evaluate("e=>JSON.parse(e.dataset.readingTexts)")
    planned = state["status"]["clips"]
    rendered = [l["text"] for l in state["lines"] if l["who"] in SPEAKERS.values()]
    check(natural(starts) and all(direct(r) for r in starts), label + ": real nearby dialogue buffers finish naturally through master")
    check([r["castId"] for r in starts] == [c["id"] for c in planned] and rendered == expected and
          all(abs(r["duration"]-c["duration"]) < .02 for r,c in zip(starts,planned)) and
          all(b["at"] >= a["expectedEnd"] + .20 for a,b in zip(starts,starts[1:])),
          label + ": every current authored word plays once, with exact buffer IDs, no overlap or truncation")
    return state


async def full_bank(page, clips):
    evidence = []
    for clip in clips:
        await page.evaluate("__ensembleProbe.reset()")
        own = {"who": SPEAKERS[clip["voice"]], "text": clip["text"]}
        forbidden = [{"who": "YOU", "text": clip["text"]}, {"who": "", "text": "An unvoiced stage direction."},
                     {"who": "EARL", "text": clip["text"]}] + [
                         {"who":who,"text":clip["text"]} for who in SPEAKERS.values()
                         if who!=own['who'] and not (clip['voice']=='bacall' and who=='THE LINE')]
        await page.evaluate("input=>{void S47.npcDialogue.play(input);}", {"voice": clip["voice"], "lines": forbidden + [own]})
        state = await first_start(page); rows = cast_starts(state)
        good_route = telephone(rows[0]) if clip["voice"] == "price" else direct(rows[0])
        check(len(rows) == 1 and rows[0]["castId"] == clip["id"] and good_route and
              abs(rows[0]["duration"] - clip["duration"]) < .02 and
              [c["id"] for c in state["status"]["clips"]] == [clip["id"]],
              f"manuscript {clip['voice']} {clip['id']}: exact real MP3, duration and role; YOU/asides/Earl excluded")
        evidence.append({"id": clip["id"], "voice": clip["voice"], "source": rows[0]})
        await page.evaluate("S47.npcDialogue.stop()")
    return evidence


async def lifecycle(page, saved, clips):
    evidence = []
    dialogue_ids = {clip['id'] for clip in clips if clip['voice'] == 'stanton'}
    actions = ("close", "Escape", "notebook", "phone", "reset", "jump", "load", "title", "Off")
    for pending in (False, True):
        for action in actions:
            await prepare_town(page)
            await page.evaluate("S47.npcDialogue.enabled=true;__ensembleProbe.reset()")
            if pending: await page.evaluate("__ensembleProbe.delayNextLoad()")
            await aim(page, "gasWindow")
            await page.keyboard.press("e")
            if pending: await wait_for_js(page, "__ensembleProbe.late.ready&&S47.npcDialogue.loading")
            else: await first_start(page)
            before = await snapshot(page)
            if action == "close": await page.locator('.labpanel [data-a=close]').click()
            elif action == "Escape": await page.keyboard.press("Escape")
            elif action == "notebook": await page.evaluate("S47.game.openNotebook()")
            elif action == "phone":
                await page.evaluate("S47.audio.phoneCall([{by:'N. VEGA',text:'Sierra Motor Court.'}])")
            elif action == "reset": await page.evaluate("S47.game.reset()")
            elif action == "jump": await page.evaluate("S47.jump('chapter1')")
            elif action == "Off": await page.evaluate("S47.npcDialogue.enabled=false")
            else:
                await page.keyboard.press("Escape"); await art.pause(page)
                await page.locator(f'.settings-panel [data-a={action}]').click()
                if action == "load":
                    await page.locator(f'.savemenu [data-load="{saved["id"]}"]').click()
                    await wait_for_js(page, "S47.started()&&!S47.game.cinematic")
                    await page.evaluate("S47.hold=true")
            if pending: await page.evaluate("__ensembleProbe.releaseLate()")
            await pump(page, .20)
            after = await snapshot(page)
            old = [source for source in cast_starts(after) if source['castId'] in dialogue_ids]
            # The new handset queue is intentionally alive after it interrupts
            # Stanton. Its Bacall buffer must not count as the old nearby queue.
            phone_started = action != 'phone' or (after['phones'][-1]['timeline'] is not None and any(
                telephone(source) for source in cast_starts(after) if source['castId'] not in dialogue_ids))
            check(after["status"]["state"] == "idle" and not after["active"] and not after["pending"] and
                  (not old if pending else art.cancelled_or_completed(old)) and
                  (not pending or await page.evaluate("__ensembleProbe.late.aborted")) and phone_started,
                  f"{'pending genuine decode' if pending else 'active'} {action}: old character audio cannot survive or revive")
            evidence.append({"pending": pending, "action": action, "before": before, "after": after})
    return evidence


async def main():
    global URL
    OUT.mkdir(parents=True, exist_ok=True)
    clips = json.loads(Path("src/assets/audio/ensemble/catalog.json").read_text())["clips"]
    corpus = json.loads(Path("production/ensemble/corpus.json").read_text())["clips"]
    expected = {"stanton":18,"stack":19,"lynch":13,"bacall":49,"price":1}
    if Counter(c["voice"] for c in clips) != expected or any(c["duration"] <= 0 for c in clips):
        raise AssertionError("Finish the full 100-recording cast bank before browser QA")
    check({(c["id"],c["text"],c["voice"]) for c in clips} == {(c["id"],c["text"],c["voice"]) for c in corpus} and all(
          hashlib.sha256((c["voice"]+'\0'+normalized(c["text"])).encode()).hexdigest()[:16] == c["id"] and
          Path("src/assets/audio/ensemble",c["file"]).is_file() for c in clips), "all 100 role-specific manuscript hashes and MP3 files match the corpus")
    report = {"pages":PAGES,"strict_csp":not PAGES,"fixtures":"Full-bank manuscript starts are cancelled after identity checks. Physical NPC E/Use and topic buttons are separate real input journeys; lifecycle reset/jump and role-negative inputs are labelled API fixtures."}
    errors, requests, bad = [], [], []; csp_dir = None
    if not PAGES:
        csp_dir=tempfile.TemporaryDirectory(prefix="s47-ensemble-csp-")
        target=Path(csp_dir.name,"index.html");target.write_text(Path("dist-single/index.html").read_text().replace("<head>",'<head><meta http-equiv="Content-Security-Policy" '+f'content="{CSP}">',1));URL=target.as_uri()
    report["url"]=URL
    try:
        async with async_playwright() as p:
            browser=await p.chromium.launch(args=["--use-angle=swiftshader","--enable-unsafe-swiftshader","--ignore-gpu-blocklist"])
            async def new_page(context):
                page=await context.new_page();page.set_default_timeout(120000)
                await page.add_init_script(art.REAL_AUDIO+'\n'+GRAPH)
                page.on("pageerror",lambda e:errors.append(str(e)))
                page.on("console",lambda m:errors.append(m.type+': '+m.text) if m.type in ('error','warning') else None)
                page.on("request",lambda r:requests.append(r.url) if r.url.startswith(('http:','https:')) and r.url.split('?',1)[0].endswith('.mp3') else None)
                page.on("response",lambda r:bad.append((r.status,r.url)) if r.status>=400 and r.url.split('?',1)[0].endswith('.mp3') else None)
                await page.goto(URL);await page.evaluate("localStorage.clear()");await page.reload();await start_night(page);await page.evaluate(PROBE)
                return page
            desktop=await browser.new_context(viewport={"width":1280,"height":800});page=await new_page(desktop)
            check(await page.evaluate("S47.npcDialogue.enabled&&S47.audio.characterVoices&&S47.narrator.enabled&&!S47.game.d.ui.readAloud.enabled&&S47.earlDialogue.enabled&&S47.radio.hostVoice&&S47.radio.callerVoice"),
                  "new character and Serling choices default On with Kinski Off; prior Art Bell and radio callers remain On")
            report["bank"]=await full_bank(page,clips)
            report["town"]={}
            for proxy,voice,topic in [('gasWindow','stanton','road'),('sheriffDoor','stack','lights'),('smoker','lynch','crash')]:
                await prepare_town(page);await page.evaluate("__ensembleProbe.reset()")
                await open_person(page,proxy,voice)
                opening=await finish_exchange(page,voice+' opening')
                before=len(cast_starts(opening))
                old_replay=None
                if voice=='stanton':
                    await page.locator(CONTROL).click();replay=await first_start(page,before);old_replay=cast_starts(replay)[-1]
                    audible=await sample_master(page)
                    await page.evaluate('S47.audio.setVolume(0)');await asyncio.sleep(.12)
                    muted=await sample_master(page,320);await page.evaluate('S47.audio.setVolume(.8)')
                    check(audible['peakRms']>1e-5 and muted['peakRms']<1e-5,'genuine nearby character PCM reaches master and the actual master-volume mute silences it after the FFT window settles')
                    await page.evaluate('S47.audio.suspend(true)');await wait_for_js(page,"S47.audio.ctx.state==='suspended'")
                    frozen=await snapshot(page);await asyncio.sleep(.6);held=await snapshot(page)
                    check(abs(held['now']-frozen['now'])<.01 and len(cast_starts(held))==len(cast_starts(frozen)),'nearby dialogue holds the same genuine buffer while the AudioContext is suspended')
                    await page.evaluate('S47.audio.suspend(false)');await wait_for_js(page,"S47.audio.ctx.state==='running'")
                    before=len(cast_starts(await snapshot(page)))
                choices=await page.locator('.talk [data-c]').evaluate_all("es=>es.map(e=>e.dataset.c)")
                chosen=topic if topic in choices else next(c for c in choices if c!='bye')
                await page.locator(f'.talk [data-c="{chosen}"]').click();await first_start(page,before)
                report['town'][voice]=await finish_exchange(page,voice+' actual topic',before)
                if old_replay:
                    replay_end=next(r for r in cast_starts(report['town'][voice]) if r['castId']==old_replay['castId'] and r['at']==old_replay['at'])
                    check(replay_end['stoppedAt'] is not None and replay_end['stoppedAt']<replay_end['expectedEnd'] and
                          cast_starts(report['town'][voice])[before]['at']>=replay_end['stoppedAt']-.02,
                          'an actual new topic stops the old replay before the fresh reply without overlap')
                check(all(c['voice']==voice for c in report['town'][voice]['status']['clips']),voice+': topic reply keeps its authored actor')
                if voice=='stanton':
                    await page.locator(CONTROL).click();await first_start(page,before+len(cast_starts(report['town'][voice])[before:]))
                    await page.locator(CONTROL).click();check(await page.evaluate("S47.npcDialogue.state==='idle'"),'real Replay/Stop voice controls cancel the current queue')
                    await page.screenshot(path=str(OUT/'ensemble-desktop.png'))
            saved=await page.evaluate("S47.saveNow('manual',0)")
            report['lifecycle']=await lifecycle(page,saved,clips)
            # The final lifecycle fixture deliberately disables this player.
            # Exercise unknown/load failure with it enabled, as in real use.
            await prepare_town(page);await page.evaluate("S47.npcDialogue.enabled=true;__ensembleProbe.reset()")
            await page.evaluate("input=>{void S47.npcDialogue.play(input);}",{'voice':'stanton','lines':[{'who':'THE ATTENDANT','text':'This sentence has no authored recording.'}]})
            await wait_for_js(page,"S47.npcDialogue.state==='error'")
            check(not cast_starts(await snapshot(page)), 'unknown dialogue text produces an explicit failure without substituting words or another actor')
            await page.evaluate("() => {S47.npcDialogue.stop();const bank=S47.npcDialogue.bank;window.__ensembleLoad=bank.load;bank.load=async()=>{throw Error('intentional decode-failure fixture');};}")
            await page.evaluate("c=>{void S47.npcDialogue.play({voice:c.voice,lines:[{who:'THE ATTENDANT',text:c.text}]});}",next(c for c in clips if c['voice']=='stanton'))
            await wait_for_js(page,"S47.npcDialogue.state==='error'")
            check(not cast_starts(await snapshot(page)),'loader failure leaves no partial or stale character source')
            await page.evaluate("() => {S47.npcDialogue.bank.load=__ensembleLoad;S47.npcDialogue.stop();}")
            await toggle(page,'characters',False);await toggle(page,'kinski',True)
            await prepare_town(page);await page.evaluate("__ensembleProbe.reset()")
            await aim(page,'gasWindow');await page.keyboard.press('e');await page.wait_for_selector('.talk')
            check(not await page.locator(CONTROL).count() and not cast_starts(await snapshot(page)) and await page.locator('[data-reading-kind=conversation] [data-a=narrate]').count()==1,
                  'characters Off prevents autoplay and preserves the optional legacy Kinski conversation control')
            await page.locator('[data-reading-kind=conversation] [data-a=narrate]').click()
            await pump(page,until="__readingTrace.snapshot().some(r=>r.kinski)")
            check(any(r.get('kinski') and art.mp3(r) for r in (await snapshot(page))['starts']),'legacy Kinski still decodes its own real conversation recording')
            await toggle(page,'characters',True);await toggle(page,'kinski',False)
            # Nora/telephone/CB and Price authored branches are checked below;
            # no arbitrary P.talk construction can pass those route assertions.
            report['authored_branches']=await authored_branches(page,clips)
            mobile=await browser.new_context(viewport={"width":844,"height":390},is_mobile=True,has_touch=True);m=await new_page(mobile)
            await prepare_town(m);await m.evaluate("__ensembleProbe.reset()")
            await open_person(m,'gasWindow','stanton',touch=True)
            check(await no_horizontal_overflow(m,'.labpanel'),'844x390 actual touch character conversation fits the screen')
            await m.locator(CONTROL).tap();await m.screenshot(path=str(OUT/'ensemble-mobile.png'))
            for name,value in [('characters',False),('serling',False),('artbell',False),('radiocast',True),('kinski',True)]:await toggle(m,name,value,touch=True)
            await m.reload();await start_night(m);await m.evaluate(PROBE)
            check(await m.evaluate("!S47.npcDialogue.enabled&&!S47.audio.characterVoices&&!S47.narrator.enabled&&!S47.radio.hostVoice&&!S47.earlDialogue.enabled&&S47.radio.callerVoice&&S47.game.d.ui.readAloud.enabled&&localStorage.getItem('s47.characterVoices')==='false'&&localStorage.getItem('s47.serlingNarrator')==='false'"),
                  'touch choices reload independently for characters, Serling, Art Bell, radio callers and Kinski')
            await mobile.close()
            check(not bad,'recorded MP3 requests have no HTTP failures')
            if not PAGES:check(not requests,'strict-CSP single uses inline bytes with zero external MP3 requests')
            noise=[e for e in errors if not any(s in e for s in ('GPU stall','Automatic fallback to software WebGL','favicon'))]
            check(not noise,'no browser errors or warnings'+(': '+' | '.join(noise[:4]) if noise else ''))
            report.update(errors=noise,requests=requests,bad_responses=bad);await browser.close()
    except Exception as e:
        report['fatal']=repr(e);raise
    finally:
        if csp_dir:csp_dir.cleanup()
        report['checks']=CHECKS;report['limits']='Genuine decode/PCM/graph/source lifecycle and real input. Manuscript full-bank starts intentionally cancelled; selected authored exchanges complete naturally. Human listening, identity, pronunciation and physical-device output/performance UNVERIFIED.'
        (OUT/'ensemble.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    passed=sum(c['pass'] for c in CHECKS);print(f'{passed} of {len(CHECKS)} PASS',flush=True)
    return 0 if passed==len(CHECKS) else 1


async def authored_branches(page, clips):
    """Authored phone/CB callbacks and actual talk buttons; no synthetic PCM."""
    evidence={}
    by_text={(c['voice'],normalized(c['text'])):c for c in clips}
    await page.evaluate('S47.audio.sfx.gain.value=1;S47.audio.radioBus.gain.value=1')
    # Scene setup is an API fixture; replies and topic selection use the real UI.
    await page.evaluate("S47.game.d.ui.close();S47.jump('chapter4');S47.tick(.3,60);__ensembleProbe.reset();S47.ch4.s.greeted=false;S47.ch4['openTalk']()")
    await page.wait_for_selector('[data-reading-kind=character-dialogue][data-voice=bacall]')
    await first_start(page);evidence['nora_open']=await finish_exchange(page,'Nora room-six greeting')
    await page.evaluate('__ensembleProbe.reset()');await page.locator('.talk [data-a=table]').click();await first_start(page)
    hint=await page.locator('.talk [data-character-quote]').inner_text()
    spoken=hint.split('"',1)[1].rsplit('"',1)[0]
    await pump(page,until="S47.npcDialogue.state==='idle'&&!S47.npcDialogue.pending")
    hint_state=await snapshot(page);hint_sources=cast_starts(hint_state)
    narration=await page.locator('[data-reading-kind=panel-description]').evaluate_all("es=>es.flatMap(e=>JSON.parse(e.dataset.readingTexts))")
    check([r['castId'] for r in hint_sources]==[by_text[('bacall',normalized(spoken))]['id']] and natural(hint_sources) and all(direct(r) for r in hint_sources) and
          all(spoken not in t for t in narration) and not any(r.get('serlingId') for r in hint_state['starts']),
          'the actual rejected evidence-table action speaks only its fresh Bacall correction and excludes it from Serling description controls')
    evidence['nora_hint']=hint_state
    # The menu fixture exposes existing topic gates, not a fabricated dialogue.
    await page.evaluate("S47.game.d.ui.close();S47.ch4['mode']='menu';S47.ch4['lines']=[];S47.ch4.s.p10=true;S47.ch4.s.line=false;S47.ch4['openTalk']();__ensembleProbe.reset()")
    await page.locator('.talk [data-c=tomas]').click();await first_start(page)
    evidence['nora_tomas']=await finish_exchange(page,'actual Nora Tomás choice')
    await page.locator('.talk [data-c=line]').click()
    await pump(page,until="__readingTrace.snapshot().some(r=>r.castId)")
    await pump(page,until="!S47.audio.phonePlaying&&!S47.npcDialogue.playing&&!S47.npcDialogue.loading")
    state=await snapshot(page);rows=cast_starts(state);price=next(c for c in clips if c['voice']=='price')
    check(any(r['castId']==price['id'] and telephone(r) for r in rows), 'the actual room-six Line choice uses the Price recording through the handset filter')
    check(natural(rows),'room-six current Nora/Price line finishes every scheduled real buffer naturally')
    evidence['nora_line']=state
    preloaded=await page.evaluate("Object.keys(S47.audio.buf).filter(k=>k.startsWith('character:')).map(k=>k.slice(10))")
    check(len(preloaded)==11 and set(preloaded)<=set(c['id'] for c in clips),'exactly the eleven small phone/CB/Price buffers are preloaded; nearby dialogue stays lazy')
    # Seed only authored branch prerequisites. Dial, ringback, captions, gameplay
    # timers and natural audio completion continue through the genuine handlers.
    branches=[('chapter2_nora',"S47.jump('chapter2');S47.audio.stop('ring2',.01);S47.ch2.ringing=false;S47.ch2.s.answered=true;S47.ch2.s.stage='call-nora';", "S47.ch2['useRecPhone']()",3,"S47.ch2.s.called"),
              ('field_nora',"S47.jump('chapter3');S47.ch3.ringing=true;S47.ch3.s.called=false;", "S47.ch3['usePhone']()",4,"S47.ch3.s.called"),
              ('hut_price',"S47.jump('chapter3');S47.ch3.s.heard=false;", "S47.ch3['useReceiver']()",1,"S47.ch3.s.heard")]
    for label,setup,action,count,complete in branches:
        await page.evaluate(setup+"S47.tick(.1);__ensembleProbe.reset();"+action)
        await pump(page,until="__ensembleProbe.phones.length>0",timeout=25000)
        before=await snapshot(page);call=before['phones'][-1];timeline=call['timeline'];sources=cast_starts(before)
        voiced=[c for c in timeline['cues'] if not c.get('aside')]
        expected_ids=[by_text[('price' if c.get('by')=='THE LINE' else 'bacall',normalized(c['text']))]['id'] for c in voiced]
        check(len(sources)==count and [r['castId'] for r in sources]==expected_ids and all(telephone(r) for r in sources),label+': authored handler schedules every exact actor buffer through the handset, excluding asides')
        check(all(abs(r['at']-call['at']-c['at'])<.035 and abs(r['duration']-c['duration'])<.002 for r,c in zip(sources,voiced)),label+': caption cue times and durations follow the real decoded buffers')
        if label=='hut_price':
            check(sources[0]['castId']==price['id'] and abs(sources[0]['at']-call['at']-4)<.04,'hut Price keeps the genuine four-second carrier lead and the same room-six recording')
        await pump(page,until=f"({complete})&&!S47.game.cinematic",timeout=90000)
        finished=await snapshot(page);played=cast_starts(finished)
        check(natural(played) and finished['now']>=max(r['expectedEnd'] for r in played),label+': scene completion follows every full natural buffer without truncation')
        if label!='hut_price':
            check(all(any(cue['text'] in cap['text'] and cap['at']>=call['at']+cue['at']-.08 for cap in finished['captions']) for cue in voiced),label+': actual spoken quotations reach their authored caption callbacks')
        evidence[label]={'before':before,'finished':finished}
    await page.evaluate("S47.game.d.ui.close();S47.jump('chapter4');S47.tick(.2);S47.ch4.s.cb=false;__ensembleProbe.reset();S47.world.driveToMotel(()=>{})")
    await pump(page,until="S47.world.driving&&!!S47.world.trip",timeout=30000)
    # Existing trip-clock fixture triggers Chapter4.update, including main's real
    # caption/onEnd callback. It does not claim a complete drive to the motel.
    await page.evaluate("S47.world.trip.secs=9.1;S47.tick(.05,60)")
    await pump(page,until="S47.cb.busy&&__ensembleProbe.cb.length>0")
    cb0=await snapshot(page);rows=cast_starts(cb0);call=cb0['cb'][-1]
    ids=[by_text[('bacall',normalized(l['text']))]['id'] for l in call['lines']]
    check(len(rows)==3 and [r['castId'] for r in rows]==ids and all(art.mp3(r) and r['rate']==1 and
          any(n['kind']=='bandpass' and abs(n['hz']-1500)<1 for n in r['radioPath']) and any(n['kind']=='lowpass' and abs(n['hz']-2900)<1 for n in r['radioPath']) and
          any(n['kind']=='radioBus' for n in r['radioPath']) for r in rows),'the actual Chapter4 CB trigger schedules exactly three Bacall recordings through its original narrow speaker and radio bus')
    await page.evaluate("S47.audio.suspend(true)");await wait_for_js(page,"S47.audio.ctx.state==='suspended'")
    frozen=await snapshot(page);await asyncio.sleep(.6);held=await snapshot(page)
    check(abs(held['now']-frozen['now'])<.01 and len(held['cb'][-1]['callbacks'])==len(frozen['cb'][-1]['callbacks']) and held['cb'][-1]['ended'] is None,'CB pause freezes source time, captions and completion callbacks')
    await page.evaluate("S47.audio.suspend(false)");await wait_for_js(page,"S47.audio.ctx.state==='running'")
    await pump(page,until="!S47.cb.busy",timeout=90000)
    cb1=await snapshot(page);rows=cast_starts(cb1);call=cb1['cb'][-1]
    check(natural(rows) and len(call['callbacks'])==3 and call['ended']>=max(r['expectedEnd'] for r in rows) and all(
          abs(c['at']-r['at'])<.10 and c['rest'] and abs(c['rest'][0]-r['duration'])<.002 for c,r in zip(call['callbacks'],rows)),
          'all three CB buffers end naturally; captions and the completion callback follow actual durations')
    check(any('CB, N. VEGA:' in c['text'] and c['audioClock'] for c in cb1['captions']),'main receives genuine CB captions on the pausable AudioContext clock')
    evidence['cb']={'scheduled':cb0,'finished':cb1};await page.evaluate("S47.world.stopDriving();S47.game.d.ui.close()")
    return evidence


if __name__=='__main__':sys.exit(asyncio.run(main()))
