"""Earl at the studio door: real controls, MP3s and cancellation.

Run from web/: python3 tools/artbell-town.py OUTDIR
S47_URL selects a Pages build served under /SIGNAL-47/. Otherwise load the single
HTML with readaloud.py's strict CSP. Root runs this in the serial GPU queue.
The town/observation fixtures do not claim a full walk or boy-broadcast journey.
Actual E/Use, topic, replay, stop, settings and case-menu controls are exercised.
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
from readaloud import CSP, no_horizontal_overflow, normalized, sample_master, start_night, wait_for_js
from phonevoices import PHONE_GRAPH, WARD


spec = importlib.util.spec_from_file_location("artbell_radio_probe", Path(__file__).with_name("artbell-radio.py"))
art = importlib.util.module_from_spec(spec)
spec.loader.exec_module(art)
OUT = Path(sys.argv[1] if len(sys.argv) > 1 else "production/artbell-town/qa/single")
PAGES = bool(os.environ.get("S47_URL"))
URL = os.environ.get("S47_URL") or Path("dist-single/index.html").resolve().as_uri()
CHECKS = []
CONTROL = '[data-reading-kind="earl-dialogue"] [data-a=earl]'
GRAPH = art.RADIO_GRAPH.replace(
    "record.hostId = [...s.radio.hostBank.buffers].find(([, b]) => b === this.buffer)?.[0] || null;",
    """record.hostId = [...s.radio.hostBank.buffers].find(([, b]) => b === this.buffer)?.[0] || null;
      record.earlId = [...s.earlDialogue.bank.buffers].find(([, b]) => b === this.buffer)?.[0] || null;
      record.narration = s.game.d.ui.readAloud.source === this;
      record.earlPath = (path(this,audio.master)||[]).map(n=>n===audio.master?'master':
        n===audio.radioBus?'radioBus':n===audio.sfx?'sfx':n instanceof BiquadFilterNode?n.type:n.constructor.name);""")
PROBE = r"""() => {
  if(window.__townProbe) return;
  const a=S47.audio,e=S47.earlDialogue;
  const p=window.__townProbe={clock:a.ctx.currentTime,remainder:0,calls:[],late:null};
  window.__artProbe=p;
  const play=e.play.bind(e);
  e.play=(lines,...rest)=>{p.calls.push({lines:lines.map(l=>({...l})),at:a.ctx.currentTime});return play(lines,...rest);};
  p.tick=()=>{
    const now=a.ctx.currentTime; p.remainder+=Math.max(0,now-p.clock);p.clock=now;
    const n=Math.floor(p.remainder*60);
    if(n){p.remainder-=n/60;S47.tick(n/60,60);}
    return {now,context:a.ctx.state};
  };
  p.reset=()=>{__readingTrace.reset();p.calls.length=0;p.clock=a.ctx.currentTime;p.remainder=0;};
  p.snapshot=()=>({now:a.ctx.currentTime,context:a.ctx.state,status:e.status,pending:!!e.pending,
    active:!!e.source,calls:p.calls,starts:__readingTrace.snapshot().filter(r=>r.earlId||r.narration||r.phoneKey),
    lines:[...document.querySelectorAll('.talk-line')].map(el=>{
      const who=el.querySelector('.who')?.textContent||'',copy=el.cloneNode(true);
      copy.querySelector('.who')?.remove();return {who,text:copy.textContent.trim()};}),
    choices:[...document.querySelectorAll('.talk [data-c]')].map(b=>b.dataset.c),
    host:S47.radio.hostVoice,callers:S47.radio.callerVoice,kinski:S47.game.d.ui.readAloud.enabled});
  p.delayNextLoad=()=>{
    const bank=e.bank,original=bank.load; let release;
    const gate=new Promise(resolve=>release=resolve);
    p.late={ready:false,aborted:false}; bank.buffers.clear();bank.bytes=0;
    bank.load=async function(...args){
      bank.load=original;const buffers=await original.apply(this,args);
      p.late.ready=true;await gate;p.late.aborted=args[2].aborted;return buffers;
    };
    p.releaseLate=()=>release();
  };
}"""


def check(ok, text):
    CHECKS.append({"pass": bool(ok), "message": text})
    print(("PASS " if ok else "FAIL ") + text, flush=True)


def earl_starts(state):
    return [r for r in state["starts"] if r.get("earlId")]


def direct(record):
    path = record.get("earlPath", [])
    return (art.mp3(record) and record.get("master") and record.get("rate") == 1 and
            record.get("offset") == 0 and path[-1:] == ["master"] and
            not any(n in path for n in ("radioBus", "bandpass", "lowpass", "highpass")))


async def snapshot(page):
    return await page.evaluate("__townProbe.snapshot()")


async def first_start(page, count=0):
    await art.pump(page, until=f"__readingTrace.snapshot().filter(r=>r.earlId).length>{count} && S47.earlDialogue.playing")
    return await snapshot(page)


async def full_exchange(page, before, expected, label):
    await art.pump(page, until="S47.earlDialogue.state==='idle' && !S47.earlDialogue.pending && !S47.earlDialogue.source")
    state = await snapshot(page)
    starts = earl_starts(state)[before:]
    check([r["earlId"] for r in starts] == [c["id"] for c in expected] and all(direct(r) for r in starts),
          label + ": exact Earl MP3 IDs reach master at rate 1 without the AM/radio-volume route")
    check(len(starts) == len(expected) and all(r["endedAt"] is not None and
          r["endedAt"] >= r["expectedEnd"] - .08 and
          (r["stoppedAt"] is None or r["stoppedAt"] >= r["expectedEnd"] - .08) and
          abs(r["duration"] - c["duration"]) < .015 for r, c in zip(starts, expected)) and all(
              b["at"] >= a["expectedEnd"] + .20 and
              abs(b["at"] - a["endedAt"] - .22) < .08 for a, b in zip(starts, starts[1:])),
          label + ": every real buffer finishes naturally, with decoded durations and audio-clock gaps")
    rendered = [l["text"] for l in state["lines"] if l["who"] == "EARL"]
    control_texts = json.loads(await page.locator('[data-reading-kind="earl-dialogue"]').get_attribute("data-reading-texts"))
    check(rendered == control_texts == [c["text"] for c in expected] and await page.locator(CONTROL).get_attribute("aria-pressed") == "false",
          label + ": spoken words match the actual Earl dialogue and Replay returns after completion")
    return state


async def prepare_town(page, fresh=False):
    if not await page.evaluate("S47.started()"):
        await start_night(page)
        await page.evaluate(PROBE)
    await page.evaluate("""fresh=>{
      S47.game.d.ui.close();S47.jump('chapter1');S47.tick(.3,60);
      S47.setAbductions(false);S47.nightEvents.wait=1e9;
      if(fresh) for(const k of [...S47.townSeen])
        if(k==='met:host'||k.startsWith('ask:host:')||k.startsWith('note:host:')||k==='ev:boy') S47.townSeen.delete(k);
    }""", fresh)
    # Await the real asynchronous town/model preparation before aiming or Use.
    await page.evaluate("async()=>{await S47.world.prepare('diner');S47.world.enter('diner');}")
    await page.evaluate("""() => {
      const a=S47.audio; for(const bus of [a.sfx,a.amb,a.music,a.radioBus]){
        bus.gain.cancelScheduledValues(a.ctx.currentTime);bus.gain.value=0;}
      a.setVolume(.8);S47.carRadio.set(false);S47.dinerRadio.set(false);
    }""")
    await aim_door(page)


async def aim_door(page):
    await page.evaluate("""() => {
      S47.game.d.ui.close();const p=S47.player;
      S47.game.clock=2*3600+35*60;S47.world.town.update(S47.runTime(),S47.game.clock);
      const at=S47.world.town.proxies.amDoor.getWorldPosition(S47.camera.position.clone());
      p.place(at.x-1.7,at.z, -Math.PI/2);
      p.pitch=Math.atan2(at.y-(p.floorY+p.eye),1.7);S47.tick(.05,60);
    }""")
    await art.pump(page, until="!S47.game.cinematic && !S47.game.d.ui.modal && S47.game.d.inter.update(S47.camera)?.id==='town:amDoor'", timeout=15000)


async def open_door(page, touch=False):
    if touch:
        await art.pump(page, until="document.querySelector('.touch .use')?.classList.contains('ready') && S47.game.d.inter.current?.id==='town:amDoor'", timeout=15000)
        await page.locator(".touch .use").tap()
    else:
        await page.keyboard.press("e")
    await page.wait_for_selector(".labpanel .talk")
    check("1420 AM / EARL" in await page.locator(".labpanel h3").inner_text(),
          "real touch Use opens Earl at the studio door" if touch else "real aimed E opens Earl at the studio door")


async def choose(page, topic, touch=False):
    button = page.locator(f'.talk [data-c="{topic}"]')
    if touch:
        await button.tap()
    else:
        await button.click()


async def toggle(page, name, value, touch=False):
    await page.evaluate("S47.game.d.ui.close()")
    await art.pause(page, touch=touch)
    button = page.locator(f'.settings-panel [data-a="{name}"]')
    await button.scroll_into_view_if_needed()
    if (await button.get_attribute("aria-pressed") == "true") != value:
        if touch:
            await button.tap()
        else:
            await button.click()
    await art.resume(page, touch=touch)


async def action(page, label, saved):
    if label == "choice":
        await choose(page, "calls")
    elif label == "Off (API)":
        await page.evaluate("() => {S47.radio.setHostVoice(false);S47.earlDialogue.enabled=false;}")
    elif label == "close":
        await page.locator(".labpanel [data-a=close]").click()
    elif label == "Escape":
        await page.keyboard.press("Escape")
    elif label == "UI replacement (API)":
        await page.evaluate("S47.game.openNotebook()")
    elif label == "phone (API)":
        await page.evaluate("lines=>S47.audio.phoneCall(lines)", [{"text": t, "by": "WARD"} for t in WARD])
    elif label == "reset (API)":
        await page.evaluate("S47.game.reset()")
    elif label == "jump (API)":
        await page.evaluate("S47.jump('chapter1')")
    elif label in ("load", "title"):
        # A player first leaves the modal conversation, then opens Settings.
        # The old voice must stay cancelled across the complete real menu flow.
        await page.keyboard.press("Escape")
        await art.pause(page)
        if label == "load":
            await page.locator(".settings-panel [data-a=load]").click()
            await page.locator(f'.savemenu [data-load="{saved["id"]}"]').click()
            await wait_for_js(page, "S47.started() && !S47.game.cinematic")
            await page.evaluate("S47.hold=true")
        else:
            await page.locator(".settings-panel [data-a=title]").click()
    else:
        raise ValueError(label)


async def lifecycle(page, saved, pending, calls):
    evidence = {}
    actions = ("choice", "Off (API)", "close", "Escape", "UI replacement (API)",
               "phone (API)", "reset (API)", "jump (API)", "load", "title")
    for label in actions:
        await prepare_town(page, fresh=True)
        await page.evaluate("() => {S47.radio.setHostVoice(true);S47.earlDialogue.enabled=true;__townProbe.reset();}")
        if pending:
            await page.evaluate("__townProbe.delayNextLoad()")
        await open_door(page)
        if pending:
            await wait_for_js(page, "__townProbe.late.ready && S47.earlDialogue.loading")
            before = await snapshot(page)
            check(not earl_starts(before), f"pending {label}: genuine decoded data waits before any Earl source")
        else:
            before = await first_start(page)
        await action(page, label, saved)
        if pending:
            await page.evaluate("__townProbe.releaseLate()")
        if label == "choice":
            await first_start(page, 0 if pending else 1)
        await art.pump(page, .28)
        after = await snapshot(page)
        if label == "choice":
            old_ids = {r["earlId"] for r in earl_starts(before)}
            check((not pending or await page.evaluate("__townProbe.late.aborted")) and
                  earl_starts(after)[-1]["earlId"] == calls[0]["id"] and
                  not any(r["earlId"] in old_ids and r["at"] > before["now"] for r in earl_starts(after)) and
                  all(r["stoppedAt"] is not None or r["endedAt"] is not None for r in earl_starts(after)[:len(old_ids)]),
                  f'{"pending" if pending else "active"} choice: fresh reply cancels the previous opening without replaying it')
        else:
            check((not pending or await page.evaluate("__townProbe.late.aborted")) and
                  after["status"]["state"] == "idle" and not after["pending"] and not after["active"] and
                  (not earl_starts(after) if pending else art.cancelled_or_completed(earl_starts(after))),
                  f'{"pending" if pending else "active"} {label}: old Earl audio cannot survive or revive')
        if label == "phone (API)":
            check(len([r for r in after["starts"] if r.get("phoneKey")]) == 2,
                  "a genuine two-buffer Ward telephone fixture takes priority over Earl")
        evidence[label] = {"before": before, "after": after}
    return evidence


async def main():
    global URL
    OUT.mkdir(parents=True, exist_ok=True)
    corpus = json.loads(Path("production/artbell-town/corpus.json").read_text())
    clips = json.loads(Path("src/assets/audio/artbell-town/catalog.json").read_text())["clips"]
    if len(clips) != 16 or any(c["duration"] <= 0 for c in clips):
        raise AssertionError("Finish all 16 real Earl MP3s before this browser test")
    by_id = {c["id"]: c for c in clips}
    groups = {topic: [by_id[c["id"]] for c in corpus["clips"]
                      if any(s["topic"] == topic for s in c["sources"])] for topic in ("open", "calls", "visitors", "tape", "boy")}
    check(Counter({t: len(rows) for t, rows in groups.items()}) == {"open": 2, "calls": 4, "visitors": 2, "tape": 4, "boy": 4} and all(
          hashlib.sha256(normalized(c["text"]).encode()).hexdigest()[:16] == c["id"] and
          Path("src/assets/audio/artbell-town", c["file"]).is_file() for c in clips),
          "all 16 exact Earl manuscript hashes, topic counts and real MP3 files are present")
    report = {"pages": PAGES, "strict_csp": not PAGES,
              "fixtures": "Prepared diner and aimed studio-door position; actual E/Use and talk buttons. ev:boy is an observation-key fixture, not a replay of that radio journey. Lifecycle API cases are labelled explicitly."}
    errors, requests, bad = [], [], []
    csp_dir = None
    if not PAGES:
        csp_dir = tempfile.TemporaryDirectory(prefix="s47-earl-csp-")
        target = Path(csp_dir.name, "index.html")
        target.write_text(Path("dist-single/index.html").read_text().replace(
            "<head>", '<head><meta http-equiv="Content-Security-Policy" ' + f'content="{CSP}">', 1))
        URL = target.as_uri()
    report["url"] = URL
    try:
        async with async_playwright() as p:
            browser = await p.chromium.launch(args=["--use-angle=swiftshader", "--enable-unsafe-swiftshader", "--ignore-gpu-blocklist"])

            async def new_page(context):
                page = await context.new_page(); page.set_default_timeout(120000)
                await page.add_init_script(art.REAL_AUDIO + "\n" + PHONE_GRAPH + "\n" + GRAPH)
                page.on("pageerror", lambda e: errors.append("PAGEERROR: " + str(e)))
                page.on("console", lambda m: errors.append(m.type + ": " + m.text) if m.type in ("error", "warning") else None)
                page.on("request", lambda r: requests.append(r.url) if r.url.startswith(("http:", "https:")) and r.url.split("?", 1)[0].endswith(".mp3") else None)
                page.on("response", lambda r: bad.append((r.status, r.url)) if r.status >= 400 and r.url.split("?", 1)[0].endswith(".mp3") else None)
                await page.goto(URL); await page.evaluate("localStorage.clear()"); await page.reload()
                await start_night(page); await page.evaluate(PROBE)
                return page

            desktop = await browser.new_context(viewport={"width": 1280, "height": 800})
            page = await new_page(desktop)
            check(await page.evaluate("S47.earlDialogue.enabled && S47.radio.hostVoice && S47.radio.callerVoice && !S47.game.d.ui.readAloud.enabled && S47.earlDialogue.bank.bufferCount===0"),
                  "Art Bell enables radio and Earl by default, independently of caller voices and Kinski, without preloading Earl")
            await prepare_town(page, fresh=True)
            saved = await page.evaluate("S47.saveNow('manual',0)")
            await page.evaluate("__townProbe.reset()")
            await open_door(page)
            active = await first_start(page)
            check(active["status"]["text"] == groups["open"][0]["text"] and not active["kinski"] and
                  "calls" in active["choices"] and "visitors" in active["choices"] and
                  "tape" not in active["choices"] and "boy" not in active["choices"],
                  "the first automatic reply works with Kinski Off and preserves the original topic gates")
            audible = await sample_master(page)
            check(audible["peakRms"] > 1e-5 and all(direct(r) for r in earl_starts(active)),
                  "genuine Earl PCM reaches ordinary master while AM/radio and SFX buses are muted")
            # API pause fixture: a modal talk is closed by Escape, so an audio
            # suspension is deliberately distinct from a real pause-menu journey.
            await page.evaluate("S47.audio.suspend(true)")
            await wait_for_js(page, "S47.audio.ctx.state==='suspended'")
            frozen = await snapshot(page); await asyncio.sleep(.8)
            held = await snapshot(page)
            check(abs(held["now"] - frozen["now"]) < .01 and len(earl_starts(held)) == len(earl_starts(frozen)),
                  "an actual AudioContext suspension holds the same Earl source without wall-clock queue advance (API fixture)")
            await page.evaluate("S47.audio.suspend(false)")
            await wait_for_js(page, "S47.audio.ctx.state==='running'")
            report["opening"] = await full_exchange(page, 0, [groups["open"][0]], "First opening")
            await page.screenshot(path=str(OUT / "e01_desktop_earl.png"))
            await page.locator(CONTROL).click()
            replay = await first_start(page, 1)
            check(earl_starts(replay)[-1]["earlId"] == groups["open"][0]["id"] and await page.locator(CONTROL).get_attribute("aria-pressed") == "true",
                  "Replay Earl starts the same exact recording through its real button")
            before = len(earl_starts(replay)); await choose(page, "calls")
            report["calls"] = await full_exchange(page, before, groups["calls"], "Calls topic")
            check(earl_starts(report["calls"])[before - 1]["stoppedAt"] is not None and
                  "tape" in report["calls"]["choices"] and "boy" not in report["calls"]["choices"],
                  "choosing Calls stops the replay before the fresh reply and unlocks only the proper next topic")
            for topic in ("visitors", "tape"):
                before = len(earl_starts(await snapshot(page)))
                await choose(page, topic)
                report[topic] = await full_exchange(page, before, groups[topic], topic.title() + " topic")
            check("boy" not in report["tape"]["choices"], "the boy follow-up still requires the observed radio broadcast")
            await page.evaluate("S47.townSeen.add('ev:boy')")
            await page.keyboard.press("Escape"); await aim_door(page)
            before = len(earl_starts(await snapshot(page))); await open_door(page)
            report["repeat_opening"] = await full_exchange(page, before, [groups["open"][1]], "Repeat opening")
            check("boy" in report["repeat_opening"]["choices"], "the observed-boy fixture and prior tape conversation unlock Boy together")
            before = len(earl_starts(await snapshot(page))); await choose(page, "boy")
            report["boy"] = await full_exchange(page, before, groups["boy"], "Boy follow-up")
            check("boy" not in report["boy"]["choices"] and await page.evaluate("S47.game.docs.some(d=>d.id==='witness-boy') && S47.game.notes.some(n=>n.includes('programme sheet'))"),
                  "the existing boy witness paper and notebook note are filed once by the genuine topic button")
            all_audio = earl_starts(await snapshot(page))
            check({r["earlId"] for r in all_audio} == set(by_id) and all(direct(r) for r in all_audio),
                  "all 16 Earl recordings actually start; player replies, asides and witness-paper prose never enter this bank")
            await page.locator(CONTROL).click(); await first_start(page, len(all_audio))
            await page.locator(CONTROL).click()
            stopped = await snapshot(page)
            check(stopped["status"]["state"] == "idle" and not stopped["active"] and earl_starts(stopped)[-1]["stoppedAt"] is not None,
                  "Stop Earl cancels the active reply and returns its control to Replay")

            # The real common preference affects both modes. Kinski remains an
            # explicit legacy narration path when Art Bell is disabled.
            await toggle(page, "artbell", False); await aim_door(page); await page.evaluate("__townProbe.reset()")
            await open_door(page); await art.pump(page, .25)
            off = await snapshot(page)
            check(not off["host"] and not off["status"]["enabled"] and not earl_starts(off) and not await page.locator(CONTROL).count(),
                  "the real Art Bell Off preference silences Earl and the radio host together")
            await toggle(page, "kinski", True); await aim_door(page); await page.evaluate("__townProbe.reset()")
            await open_door(page)
            narrator = page.locator('[data-reading-kind="conversation"] [data-a=narrate]')
            await narrator.click(); await art.pump(page, until="S47.game.d.ui.readAloud.playing")
            legacy = await snapshot(page)
            check(not earl_starts(legacy) and any(r.get("narration") and art.mp3(r) and r["master"] for r in legacy["starts"]),
                  "Art Bell Off retains genuine manual Kinski reading without automatic Earl or overlap")
            await toggle(page, "artbell", True); await prepare_town(page, fresh=True); await open_door(page)
            await first_start(page)
            await choose(page, "calls")
            texts = await page.locator('[data-reading-kind="conversation"]').get_attribute("data-reading-texts")
            check(json.loads(texts) == ["What do people call in about?"],
                  "Art Bell On excludes Earl lines from the separate Kinski narration control")
            await page.locator('[data-reading-kind="conversation"] [data-a=narrate]').click()
            await art.pump(page, until="S47.game.d.ui.readAloud.playing")
            check(await page.evaluate("S47.earlDialogue.state==='idle' && !S47.earlDialogue.source"),
                  "manual Kinski reading takes priority and stops Earl immediately")

            report["pending_cancellation"] = await lifecycle(page, saved, True, groups["calls"])
            report["active_cancellation"] = await lifecycle(page, saved, False, groups["calls"])
            check(await page.evaluate("S47.earlDialogue.bank.bufferCount<=24"), "Earl's lazy cache stays within its 24-buffer bound")
            await desktop.close()

            mobile = await browser.new_context(viewport={"width": 844, "height": 390}, is_mobile=True, has_touch=True, device_scale_factor=1)
            page = await new_page(mobile)
            await prepare_town(page, fresh=True); await page.evaluate("__townProbe.reset()")
            await open_door(page, touch=True); await first_start(page)
            check(await no_horizontal_overflow(page, ".labpanel"), "844x390 genuine touch Earl conversation fits the screen")
            await page.screenshot(path=str(OUT / "e02_mobile_earl.png"))
            await choose(page, "visitors", touch=True)
            await first_start(page, 1)
            await page.locator(CONTROL).tap()
            check(await page.evaluate("S47.earlDialogue.state==='idle'"), "a real mobile topic tap replaces the reply and Stop Earl cancels it")
            await toggle(page, "artbell", False, touch=True)
            await toggle(page, "kinski", True, touch=True)
            await toggle(page, "radiocast", False, touch=True)
            check(await page.evaluate("!S47.radio.hostVoice && !S47.earlDialogue.enabled && !S47.radio.callerVoice && S47.game.d.ui.readAloud.enabled && localStorage.getItem('s47.artBellRadioHost')==='false'"),
                  "touch settings share one persisted Art Bell choice while caller and Kinski choices remain separate")
            await page.reload(); await start_night(page); await page.evaluate(PROBE)
            check(await page.evaluate("!S47.radio.hostVoice && !S47.earlDialogue.enabled && !S47.radio.callerVoice && S47.game.d.ui.readAloud.enabled"),
                  "reload retains the independent Art Bell Off, callers Off and Kinski On choices")
            await toggle(page, "artbell", True, touch=True)
            await prepare_town(page); await open_door(page, touch=True); await first_start(page)
            check(await page.evaluate("S47.radio.hostVoice && S47.earlDialogue.enabled && !S47.radio.callerVoice && S47.game.d.ui.readAloud.enabled"),
                  "the shared Art Bell choice re-enables actual touch dialogue without changing caller or Kinski preferences")
            await mobile.close()

            check(not bad, "recorded MP3 requests have no HTTP failures")
            if not PAGES:
                check(not requests, "strict-CSP single playback uses inline MP3 bytes with no external MP3 requests")
            noise = [e for e in errors if not any(s in e for s in ("GPU stall", "Automatic fallback to software WebGL", "favicon"))]
            check(not noise, "no browser errors or warnings" + (": " + " | ".join(noise[:4]) if noise else ""))
            report.update({"errors": noise, "requests": requests, "bad_responses": bad})
            await browser.close()
    except Exception as error:
        report["fatal"] = repr(error); raise
    finally:
        if csp_dir: csp_dir.cleanup()
        report["checks"] = CHECKS
        report["limits"] = "Real decoding/PCM/source lifecycle and controls; human listening, pronunciation, voice likeness and physical device performance UNVERIFIED. API lifecycle and observed-boy fixtures are labelled."
        (OUT / "artbell-town.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    passed = sum(c["pass"] for c in CHECKS)
    print(f"{passed} of {len(CHECKS)} PASS", flush=True)
    return 0 if passed == len(CHECKS) else 1


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
