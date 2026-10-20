"""Night Line's recorded host: real MP3s, AM routing and pausable audio time.

Run from web/: python3 tools/artbell-radio.py OUTDIR
S47_URL selects a served Pages build (including its /SIGNAL-47/ subdirectory).
Without S47_URL, dist-single is tested under the same strict CSP as csptest.py.
No audio is substituted. Short, explicitly labelled manuscript fixtures exercise
the real Station planner without listening through every long programme. Existing
radio.py/tripradio.py cover the canonical event and trip selection rules.
"""
import asyncio
import hashlib
import json
import os
import sys
import tempfile
from pathlib import Path

from playwright.async_api import async_playwright

from readaloud import CSP, INSTRUMENT, no_horizontal_overflow, normalized, sample_master, start_night, wait_for_js


OUT = Path(sys.argv[1] if len(sys.argv) > 1 else "shots/artbell-radio")
PAGES = bool(os.environ.get("S47_URL"))
URL = os.environ.get("S47_URL") or Path("dist-single/index.html").resolve().as_uri()
CHECKS = []
# Time fragments can be shorter than the narration probe's SFX exclusion.
REAL_AUDIO = INSTRUMENT.replace("this.buffer.duration > 0.5", "this.buffer.duration > 0")

# Observe the actual graph, including disconnects, without adding audio edges.
# Buffer identity comes from the real decoded host cache, never from file names
# guessed from unrelated sound effects.
RADIO_GRAPH = r"""(() => {
  const edges = new WeakMap();
  const connect = AudioNode.prototype.connect, disconnect = AudioNode.prototype.disconnect;
  AudioNode.prototype.connect = function (destination, ...rest) {
    const result = connect.call(this, destination, ...rest);
    if (destination instanceof AudioNode) {
      let targets = edges.get(this); if (!targets) edges.set(this, targets = new Set());
      targets.add(destination);
    }
    return result;
  };
  AudioNode.prototype.disconnect = function (...args) {
    const result = disconnect.apply(this, args), targets = edges.get(this);
    if (args[0] instanceof AudioNode) targets?.delete(args[0]); else targets?.clear();
    return result;
  };
  function path(node, target, seen = new Set()) {
    if (node === target) return [node];
    if (!node || seen.has(node)) return null;
    seen.add(node);
    for (const next of edges.get(node) || []) {
      const rest = path(next, target, seen); if (rest) return [node, ...rest];
    }
    return null;
  }
  const start = AudioBufferSourceNode.prototype.start;
  AudioBufferSourceNode.prototype.start = function (...args) {
    const result = start.apply(this, args), s = window.S47, audio = s?.audio;
    const record = window.__readingTrace.starts.findLast(r => r.source === this);
    if (record && audio && s.radio) {
      record.hostId = [...s.radio.hostBank.buffers].find(([, b]) => b === this.buffer)?.[0] || null;
      record.radioPath = (path(this, audio.radioBus) || []).map(n => n instanceof BiquadFilterNode
        ? {kind: n.type, hz: n.frequency.value}
        : {kind: n === audio.radioBus ? 'radioBus' : n.constructor.name});
      record.radioMaster = !!path(this, audio.master);
      record.offset = args[1] || 0;
      record.rate = this.playbackRate.value;
    }
    return result;
  };
})();"""

RADIO_PROBE = r"""() => {
  if (window.__artProbe) return;
  const a = S47.audio, radio = S47.radio, ui = S47.game.d.ui;
  const probe = window.__artProbe = {captions: [], lines: [], clock: 0, remainder: 0, late: null};
  const onLine = radio.onLine;
  radio.onLine = (id, text, by, duration) => {
    probe.lines.push({id, text, by: by ?? null, duration: duration ?? null,
      at: a.ctx.currentTime, t: radio.stations[id].t});
    return onLine?.(id, text, by, duration);
  };
  const toast = ui.toast.bind(ui);
  ui.toast = (text, seconds, ...rest) => {
    if (text.startsWith('RADIO')) probe.captions.push({text, seconds, at: a.ctx.currentTime,
      audioClock: typeof rest[0] === 'function'});
    return toast(text, seconds, ...rest);
  };
  probe.reset = () => {
    __readingTrace.reset(); probe.captions.length = 0; probe.lines.length = 0;
    probe.clock = a.ctx.currentTime; probe.remainder = 0;
  };
  probe.tick = () => {
    const now = a.ctx.currentTime;
    probe.remainder += Math.max(0, now - probe.clock); probe.clock = now;
    const frames = Math.floor(probe.remainder * 60);
    if (frames) { probe.remainder -= frames / 60; S47.tick(frames / 60, 60); }
    return {now, t: radio.stations.caliche.t, context: a.ctx.state};
  };
  probe.snapshot = () => ({context: a.ctx.state, now: a.ctx.currentTime,
    state: radio.state(), captions: probe.captions, lines: probe.lines,
    starts: __readingTrace.snapshot().filter(r => r.hostId),
    sources: radio.stations.caliche.hostSources.length,
    pending: !!radio.stations.caliche.hostPending,
    spoken: radio.stations.caliche.spoken.map(l => ({text:l.text, who:l.line.who,
      secs:l.secs, ids:l.clips?.map(c=>c.id) ?? [], buffers:l.buffers?.length ?? 0})),
  });
  // Delay publication of a genuine load/decode result. All bytes still pass
  // through the real loader and decoder. This exercises stale async completion
  // in both Pages and data-URL builds without manufacturing an AudioBuffer.
  probe.delayNextLoad = () => {
    const bank = radio.hostBank, original = bank.load;
    let release;
    const gate = new Promise(resolve => { release = resolve; });
    probe.late = {ready:false, released:false, aborted:false};
    bank.buffers.clear(); bank.bytes = 0;
    bank.load = async function (...args) {
      bank.load = original;
      const result = await original.apply(this, args);
      probe.late.ready = true;
      await gate;
      probe.late.aborted = args[2].aborted;
      return result;
    };
    probe.releaseLate = () => { probe.late.released = true; release(); };
  };
}"""


def check(ok, message):
    CHECKS.append({"pass": bool(ok), "message": message})
    print(("PASS " if ok else "FAIL ") + message, flush=True)


def mp3(record):
    head = (record.get("decoded") or {}).get("head", [])
    return len(head) >= 3 and (head[:3] == [73, 68, 51] or head[0] == 255 and head[1] & 224 == 224)


def am_route(record):
    filters = [(n["kind"], n.get("hz")) for n in record.get("radioPath", [])
               if n["kind"] in ("bandpass", "lowpass", "highpass")]
    return (len(filters) >= 3 and filters[:2] == [("bandpass", 1250), ("lowpass", 3400)]
            and filters[2][0] == "lowpass" and record.get("radioMaster") and record.get("master")
            and record["radioPath"][-1]["kind"] == "radioBus")


def cancelled_or_completed(records):
    return bool(records) and all(
        r["stoppedAt"] is not None or r["endedAt"] is not None and r["endedAt"] >= r["expectedEnd"] - 0.08
        for r in records)


async def pump(page, seconds=0, until=None, timeout=60000):
    """Advance the game only as the real, pausable audio clock progresses."""
    begin = await page.evaluate("S47.audio.ctx.currentTime")
    deadline = asyncio.get_running_loop().time() + timeout / 1000
    while True:
        state = await page.evaluate("__artProbe.tick()")
        if until and await page.evaluate(until):
            return state
        if not until and state["now"] - begin >= seconds:
            return state
        if asyncio.get_running_loop().time() >= deadline:
            raise TimeoutError("Radio audio clock did not reach " + (until or str(seconds)))
        await asyncio.sleep(0.06)


async def pause(page, touch=False):
    if touch:
        await page.locator(".touch .pause").tap()
    else:
        await page.keyboard.press("Escape")
    await page.wait_for_selector(".settings-panel [data-a=resume]")
    await wait_for_js(page, "S47.audio.ctx.state === 'suspended'")


async def resume(page, touch=False):
    control = page.locator(".settings-panel [data-a=resume]")
    if touch:
        await control.tap()
    else:
        await control.click()
    await wait_for_js(page, "S47.audio.ctx.state === 'running'")


async def foreground_radio(page):
    await page.evaluate("""() => {
      const a = S47.audio;
      for (const bus of [a.sfx, a.amb, a.music]) {
        bus.gain.cancelScheduledValues(a.ctx.currentTime); bus.gain.value = 0;
      }
      a.setRadio(1); a.setVolume(0.8);
    }""")


async def sample_station(page, milliseconds=450):
    return await page.evaluate("""async ms => {
      const a = S47.audio, bus = S47.radio.stations.caliche.bus;
      const analyser = a.ctx.createAnalyser(); analyser.fftSize = 2048;
      bus.connect(analyser);
      const data = new Float32Array(analyser.fftSize), levels = [];
      try {
        for (let elapsed=0; elapsed<ms; elapsed+=40) {
          await new Promise(resolve=>setTimeout(resolve,40));
          analyser.getFloatTimeDomainData(data);
          levels.push(Math.sqrt(data.reduce((sum,x)=>sum+x*x,0)/data.length));
        }
      } finally { bus.disconnect(analyser); analyser.disconnect(); }
      return {peakRms:Math.max(...levels), levels};
    }""", milliseconds)


async def enter_truck(page):
    await page.evaluate("S47.game.d.ui.close(); S47.jump('chapter1'); S47.tick(1)")
    await wait_for_js(page, "!!S47.game.d.inter.get('truck')")
    await page.evaluate("S47.game.d.inter.get('truck').use()")
    for _ in range(100):
        await page.evaluate("S47.tick(0.25, 60)")
        if await page.evaluate("S47.world.driving && !S47.game.cinematic"):
            break
        await asyncio.sleep(0.03)
    await wait_for_js(page, "S47.world.driving && !S47.game.cinematic")
    await page.evaluate("S47.radio.reset(S47.game.clock); __artProbe.reset()")


async def fixture(page, text, *, lines=None, station="caliche", clock=None, reset=True, delay_load=False):
    """Use actual authored text with the real planner; not a story-event trigger."""
    await page.evaluate("""({text, lines, station, clock, reset, delayLoad}) => {
      if (clock !== null) S47.game.clock = clock;
      const radio = S47.radio, st = radio.stations[station];
      S47.carRadio.set(true, station); S47.tick(0.02,60);
      if (reset) __artProbe.reset();
      if (delayLoad) __artProbe.delayNextLoad();
      st.start([{kind:'talk', lines:lines || [{who:'host',text}]}, {kind:'dead'}], S47.game.clock);
    }""", {"text": text, "lines": lines, "station": station, "clock": clock, "reset": reset,
            "delayLoad": delay_load})


async def loaded(page):
    await pump(page, until="!S47.radio.stations.caliche.hostPending && S47.radio.stations.caliche.hostReady")
    await pump(page, 0.28)
    return await page.evaluate("__artProbe.snapshot()")


def render_time(corpus, template, hour, minute):
    hours = {x["value"]: x for x in corpus["timeBank"]["hours"]}
    minutes = {x["value"]: x for x in corpus["timeBank"]["minutes"]}
    time = hours[hour]["text"] + ("-" if 0 < minute < 10 else " ") + minutes[minute]["text"]
    if template["capitalize"]:
        time = time[0].upper() + time[1:]
    text = template["prefix"] + time + template["suffix"]
    ids = [template.get("before"), hours[hour]["id"], minutes[minute]["id"], template.get("after")]
    return text, [x for x in ids if x]


async def late_completion(page, text, action, saved):
    await page.evaluate("S47.radio.setHostVoice(true)")
    await fixture(page, text, delay_load=True)
    await wait_for_js(page, "__artProbe.late.ready && S47.radio.stations.caliche.hostPending !== null")
    before = await page.evaluate("__artProbe.snapshot()")
    check(not before["starts"] and before["pending"], f"{action}: real decoded data waits before any recorded source starts")
    if action == "Off":
        await pause(page)
        await page.locator(".settings-panel [data-a=artbell]").click()
        await resume(page)
    elif action == "station change":
        await page.keyboard.press("r")
        await page.evaluate("S47.tick(0.02,60)")
    elif action == "reset":
        await page.evaluate("S47.radio.reset(S47.game.clock)")
    elif action == "load case":
        await pause(page)
        await page.locator(".settings-panel [data-a=load]").click()
        await page.locator(f'.savemenu [data-load="{saved["id"]}"]').click()
        await wait_for_js(page, "S47.started() && !S47.world.driving && !S47.game.cinematic")
        await page.evaluate("S47.hold=true")
    else:
        raise ValueError(action)
    await page.evaluate("__artProbe.releaseLate()")
    await pump(page, 0.3)
    after = await page.evaluate("__artProbe.snapshot()")
    check(await page.evaluate("__artProbe.late.aborted") and not after["pending"] and
          not after["starts"] and after["sources"] == 0,
          f"{action}: a late genuine decode result cannot revive host audio or future sources")
    if action == "Off":
        await pause(page)
        await page.locator(".settings-panel [data-a=artbell]").click()
        await resume(page)
    return {"before": before, "after": after, "late": await page.evaluate("__artProbe.late")}


async def main():
    global URL
    OUT.mkdir(parents=True, exist_ok=True)
    corpus = json.loads(Path("production/artbell/corpus.json").read_text())
    catalog = json.loads(Path("src/assets/audio/artbell/catalog.json").read_text())
    clips = catalog.get("clips", [])
    if len(clips) != 115 or any(c.get("duration", 0) <= 0 for c in clips):
        raise AssertionError("Finish all 115 recorded Art Bell clips before running this browser check")
    by_id = {c["id"]: c for c in clips}
    check(set(by_id) == {c["id"] for c in corpus["clips"]} and all(
        hashlib.sha256(normalized(c["text"]).encode()).hexdigest()[:16] == c["id"] and
        Path("src/assets/audio/artbell", c["file"]).is_file() for c in clips),
        "all 115 host/time clips preserve the extracted text hashes and have real MP3 files")
    template_fields = ("id", "prefix", "suffix", "capitalize", "before", "after", "parts")
    template_content = lambda templates: [{k: t.get(k) for k in template_fields} for t in templates]
    check(template_content(catalog.get("templates", [])) == template_content(corpus["templates"]) and
          catalog.get("timeBank") == corpus["timeBank"],
          "the runtime catalogue contains all six exact time templates and both complete time banks")
    opening = corpus["staticLines"][0]
    short = min(corpus["staticLines"], key=lambda line: by_id[line["id"]]["duration"])
    signoff = next(x for x in corpus["staticLines"] if x["group"] == "SIGN_OFF_SEQ")
    boy = next(x for x in corpus["staticLines"] if x["group"] == "BOY_CALL" and x["text"] == "Is your dad up?")
    trip = next(x for x in corpus["staticLines"] if x["group"] == "PROGRAMME" and x["text"] == "Not the wind, then.")

    def host_url(url):
        name = url.split("?", 1)[0].rsplit("/", 1)[-1]
        return any(name == c["file"] or name.startswith(c["id"] + "-") and name.endswith(".mp3") for c in clips)

    csp_dir = None
    if not PAGES:
        csp_dir = tempfile.TemporaryDirectory(prefix="s47-artbell-csp-")
        target = Path(csp_dir.name) / "index.html"
        target.write_text(Path("dist-single/index.html").read_text().replace(
            "<head>", '<head><meta http-equiv="Content-Security-Policy" '
            f'content="{CSP}">', 1))
        URL = target.as_uri()
    errors, requests, bad_responses = [], [], []
    report = {"url": URL, "pages": PAGES, "strict_csp": not PAGES,
              "fixtures": "Exact extracted manuscript passed to the real Station planner. Canonical event/trip triggers are covered by radio.py and tripradio.py."}
    try:
        async with async_playwright() as playwright:
            browser = await playwright.chromium.launch(args=[
                "--use-angle=swiftshader", "--enable-unsafe-swiftshader", "--ignore-gpu-blocklist",
            ])

            async def new_page(context):
                page = await context.new_page()
                page.set_default_timeout(120000)
                # Legacy timing/voice baseline; new defaults are exercised in ensemble.py/serling.py.
                await page.add_init_script("try{localStorage.setItem('s47.serlingNarrator','false');localStorage.setItem('s47.characterVoices','false');}catch{}")
                await page.add_init_script(REAL_AUDIO + "\n" + RADIO_GRAPH)
                page.on("pageerror", lambda error: errors.append("PAGEERROR: " + str(error)))
                page.on("console", lambda message: errors.append(message.type + ": " + message.text)
                        if message.type in ("error", "warning") else None)
                page.on("request", lambda request: requests.append(request.url) if host_url(request.url) else None)
                page.on("response", lambda response: bad_responses.append((response.status, response.url))
                        if response.status >= 400 and host_url(response.url) else None)
                await page.goto(URL)
                await page.evaluate("localStorage.clear()")
                await page.reload()
                await start_night(page)
                await page.evaluate(RADIO_PROBE)
                return page

            context = await browser.new_context(viewport={"width": 1280, "height": 800})
            page = await new_page(context)
            check(await page.evaluate("S47.radio.hostVoice && !S47.carRadio.on && S47.radio.hostBank.bufferCount === 0"),
                  "Art Bell defaults On while the radio is Off and the host bank stays undecoded")
            # Keep this host-only suite's legacy caller assertions explicit.
            # radiocast-radio.py covers caller/news default On and all 28 MP3s.
            await page.evaluate("S47.radio.setCallerVoice(false)")
            await page.evaluate("S47.jump('chapter1'); S47.tick(1)")
            saved = await page.evaluate("S47.saveNow('manual',0)")
            await enter_truck(page)
            # Seed an authored opening while asleep, then use the real R handler.
            await page.evaluate("""text => {
              S47.radio.stations.caliche.start([{kind:'talk',lines:[{who:'host',text}]},{kind:'dead'}],S47.game.clock);
              __artProbe.reset();
            }""", opening["text"])
            await page.keyboard.press("r")
            await page.evaluate("S47.tick(0.02,60)")
            await foreground_radio(page)
            first = await loaded(page)
            check(await page.evaluate("S47.carRadio.on && S47.carRadio.station==='caliche' && S47.carRadio.live") and
                  first["state"]["caliche"]["awake"] and first["state"]["caliche"]["host"]["playing"],
                  "real R wakes AM 1420 and starts the recorded host on the truck's receiver")
            check([r["hostId"] for r in first["starts"]] == [opening["id"]] and
                  all(mp3(r) and am_route(r) and r["rate"] == 1 for r in first["starts"]),
                  "the exact opening MP3 is decoded and routed through AM filters, radioBus and master at its original rate")
            source = first["starts"][0]
            host = first["state"]["caliche"]["host"]
            captions = [c for c in first["captions"] if opening["text"] in c["text"]]
            check(host["text"] == opening["text"] and abs(host["duration"] - source["duration"]) < 0.002 and
                  len(captions) == 1 and captions[0]["text"] == f'RADIO: "{opening["text"]}"' and
                  captions[0]["audioClock"] and
                  abs(captions[0]["at"] - source["at"]) < 0.16 and
                  abs(captions[0]["seconds"] - max(2.4, source["duration"] + 0.2)) < 0.01,
                  "one exact caption begins with the recording and remains for the full decoded duration")
            station_sound = await sample_station(page)
            audible = await sample_master(page)
            check(station_sound["peakRms"] > 1e-4 and audible["peakRms"] > 1e-5,
                  "the host station produces real nonzero PCM samples and its receiver reaches master")
            await pump(page, 0.05)
            before = await page.evaluate("__artProbe.snapshot()")
            opening_caption = f'RADIO: "{opening["text"]}"'
            await page.evaluate("""text => {
              window.__artOpeningCaption=[...document.querySelectorAll('.toasts .toast')].find(el =>
                el.textContent===text && !el.classList.contains('out'));
            }""", opening_caption)
            await pause(page)
            frozen = await page.evaluate("__artProbe.snapshot()")
            # The old wall-time toast would disappear during this pause. Wait
            # longer than the entire original recording, with audio suspended.
            await asyncio.sleep(source["duration"] + 0.8)
            still = await page.evaluate("__artProbe.snapshot()")
            check(still["context"] == "suspended" and abs(still["now"] - frozen["now"]) < 0.01 and
                  abs(still["state"]["caliche"]["t"] - frozen["state"]["caliche"]["t"]) < 0.01 and
                  len(still["starts"]) == len(before["starts"]),
                  "pause holds both the real audio clock and station position without restarting speech")
            retained_caption = """text => {
              const el=window.__artOpeningCaption;
              return !!el?.isConnected && el.textContent===text && !el.classList.contains('out');
            }"""
            live_caption = """text => {
              const el=window.__artOpeningCaption;
              return !!el?.isConnected && el.textContent===text && !el.classList.contains('out') &&
                getComputedStyle(el).visibility!=='hidden' && el.getBoundingClientRect().height>0;
            }"""
            # The pause menu deliberately hides the whole HUD. Its original
            # caption must remain attached and live, then become visible again
            # when the menu closes.
            check(captions[0]["audioClock"] and await page.evaluate(retained_caption, opening_caption),
                  "the exact radio caption survives a real pause longer than the whole recording")
            check(await page.locator("[data-a=artbell]").get_attribute("aria-pressed") == "true" and
                  await page.locator("[data-a=kinski]").get_attribute("aria-pressed") == "false" and
                  await page.locator("[data-a=radiocast]").get_attribute("aria-pressed") == "false" and
                  await page.evaluate("S47.radio.hostVoice && S47.earlDialogue.enabled && !S47.radio.callerVoice && !S47.game.d.ui.readAloud.enabled") and
                  await page.locator(".settings-panel .reading-note").first.inner_text() ==
                  "Synthetic Art Bell parody for Earl on Night Line, AM 1420, and when you speak with him in Caliche.",
                  "desktop settings identify the full radio/Caliche parody scope and keep caller and Kinski controls independent")
            await page.locator("[data-a=artbell]").scroll_into_view_if_needed()
            await page.screenshot(path=str(OUT / "r01_desktop_settings.png"))
            check(await no_horizontal_overflow(page, ".settings-panel"), "desktop radio settings fit the viewport")
            await page.evaluate("""() => {
              const slider=document.querySelector('#rad'); slider.value='0'; slider.dispatchEvent(new Event('input'));
            }""")
            check(await page.evaluate("S47.audio.radioLevel===0 && S47.audio.radioBus.gain.value===0 && localStorage.getItem('s47.radioVol')==='0'"),
                  "the real Radio slider controls and persists the bus used by the recorded host")
            await resume(page)
            await pump(page, 0.12)
            check(await page.evaluate(live_caption, opening_caption),
                  "Resume retains the same live radio caption for the remaining speech")
            muted = await sample_master(page, 300)
            check(muted["peakRms"] < max(1e-5, audible["peakRms"] * 0.02),
                  "Radio volume zero silences master when unrelated buses are muted")
            await page.evaluate("S47.audio.setRadio(1)")
            await pump(page, 0.1)
            after_pause = await page.evaluate("__artProbe.snapshot()")
            check(len(after_pause["starts"]) == 1 and after_pause["starts"][0]["stoppedAt"] is None and
                  after_pause["state"]["caliche"]["t"] > frozen["state"]["caliche"]["t"],
                  "resume continues the same source and station position without double playback")
            # A second receiver subscribes to the same station bus. No second
            # source or caption may be made for its copy of the programme.
            old_count = len(after_pause["starts"])
            await page.evaluate("""() => {
              const r=window.__extraRadio=S47.radio.receiver({vol:0.15});
              r.pos=S47.carRadio.pos.clone(); r.set(true,'caliche');
            }""")
            await pump(page, 0.2)
            two = await page.evaluate("__artProbe.snapshot()")
            check(two["state"]["caliche"]["listeners"] == 2 and len(two["starts"]) == old_count and
                  len([x for x in two["captions"] if opening["text"] in x["text"]]) == 1,
                  "two receivers share one recorded station source and one caption")
            await page.evaluate("__extraRadio.reset(); S47.radio.receivers.splice(S47.radio.receivers.indexOf(__extraRadio),1)")
            report["opening"] = {"audio": two, "station_pcm": station_sound, "master": audible, "muted_master": muted}

            # Rejoin an already decoded line through the real four-step R dial.
            # Tuning notices displace the old toast, so the offset recording must
            # announce its exact caption again without waiting for another decode.
            await fixture(page, opening["text"])
            warm_before = await loaded(page)
            await pump(page, 0.2)
            await page.evaluate("__artProbe.captions.length=0")
            for _ in range(4):
                await page.keyboard.press("r")
                await page.evaluate("S47.tick(0.02,60)")
            await pump(page, 0.08)
            warm = await page.evaluate("__artProbe.snapshot()")
            warm_sources = warm["starts"]
            check(warm["state"]["caliche"]["host"]["playing"] and not warm["pending"] and
                  len(warm_sources) == len(warm_before["starts"]) + 1 and
                  warm_sources[-1]["hostId"] == opening["id"] and warm_sources[-1]["offset"] > 0.05 and
                  all(r["stoppedAt"] is not None for r in warm_sources[:-1]),
                  "real R retuning rejoins the cached host at its current offset without duplicate sources")
            check(len(warm["captions"]) == 1 and warm["captions"][0]["text"] == opening_caption and
                  await page.evaluate("""text => [...document.querySelectorAll('.toasts .toast')].some(el =>
                    el.textContent===text && !el.classList.contains('out') && el.isConnected)""", opening_caption),
                  "warm retuning restores one exact live caption for the recorded line")
            report["warm_retuning"] = warm

            await fixture(page, short["text"])
            natural = await loaded(page)
            check([r["hostId"] for r in natural["starts"]] == [short["id"]],
                  "the natural-end fixture uses a complete authored short host line")
            await pump(page, until="__readingTrace.snapshot().filter(r=>r.hostId).every(r=>r.endedAt!==null)",
                       timeout=int((by_id[short["id"]]["duration"] + 15) * 1000))
            natural = await page.evaluate("__artProbe.snapshot()")
            check(len(natural["starts"]) == 1 and all(r["endedAt"] >= r["expectedEnd"] - 0.08 and
                  (r["stoppedAt"] is None or r["stoppedAt"] >= r["expectedEnd"] - 0.08) for r in natural["starts"]),
                  "a complete real host buffer reaches its natural end without truncation")
            report["natural_end"] = natural

            # The first time template has short before/after fragments. Test
            # both the hyphenated early minute and an on-the-hour boundary.
            report["time_templates"] = []
            for minute in (3, 0):
                template = corpus["templates"][0]
                text, ids = render_time(corpus, template, 2, minute)
                await fixture(page, text, clock=2 * 3600 + minute * 60)
                dynamic = await loaded(page)
                starts = dynamic["starts"]
                duration = sum(r["duration"] for r in starts) + 0.035 * (len(starts) - 1)
                check([r["hostId"] for r in starts] == ids and all(mp3(r) and am_route(r) for r in starts),
                      f"02:{minute:02d} selects every exact time fragment and schedules real decoded MP3s in order")
                check(dynamic["state"]["caliche"]["host"]["text"] == text and
                      abs(dynamic["state"]["caliche"]["host"]["duration"] - duration) < 0.002 and all(
                          abs(b["at"] - a["expectedEnd"] - 0.035) < 0.025 for a, b in zip(starts, starts[1:])),
                      f"02:{minute:02d} retains the complete displayed sentence and derives its duration from all decoded fragments")
                report["time_templates"].append(dynamic)

            report["manuscript_fixtures"] = []
            for label, line in (("BOY_CALL host", boy), ("sign-off host", signoff), ("trip programme host", trip)):
                await fixture(page, line["text"])
                result = await loaded(page)
                check([r["hostId"] for r in result["starts"]] == [line["id"]] and
                      result["spoken"][0]["text"] == line["text"] and all(mp3(r) and am_route(r) for r in result["starts"]),
                      f"{label}: the extracted manuscript fixture uses its exact recording through the real station planner")
                report["manuscript_fixtures"].append({"label": label, "audio": result})

            # A caller can say a string that happens to exist in the host bank:
            # who and station still control voice selection, never text alone.
            await fixture(page, boy["text"], lines=[{"who": "boy", "by": "a boy", "text": boy["text"]}])
            await pump(page, 0.35)
            caller = await page.evaluate("__artProbe.snapshot()")
            caller_pcm = await sample_station(page)
            check(not caller["starts"] and caller["sources"] == 0 and not caller["spoken"][0]["ids"] and
                  caller_pcm["peakRms"] > 1e-5,
                  "with caller recordings Off, a caller remains procedural even when its words exist in the host bank")
            roswell_text = "Cattle at the sale barn steady, a dollar higher on calves."
            await fixture(page, roswell_text, station="roswell", clock=5 * 3600 + 10 * 60,
                          lines=[{"who": "roswell", "text": roswell_text}])
            await pump(page, 0.35)
            check(not (await page.evaluate("__artProbe.snapshot()"))["starts"] and
                  await page.evaluate("S47.radio.stations.roswell.spoken.every(l=>!l.clips && !l.buffers)"),
                  "Roswell 1230's morning host remains procedural while Art Bell is enabled")
            unknown = "This deliberately unknown radio fixture has no prerecorded host clip."
            await fixture(page, unknown, clock=2 * 3600 + 15 * 60)
            await pump(page, 0.35)
            fallback = await page.evaluate("__artProbe.snapshot()")
            check(not fallback["starts"] and not fallback["pending"] and not fallback["spoken"][0]["ids"] and
                  any(x["text"] == unknown and x["duration"] is None for x in fallback["lines"]),
                  "an unknown host sentence falls back to the complete original procedural line and caption")
            report["procedural"] = {"caller": caller, "caller_pcm": caller_pcm, "unknown": fallback}

            # A real decoder rejection is a deliberate failure fixture, not
            # replacement audio. Restore the decoder before subsequent checks.
            await page.evaluate("""() => {
              const bank=S47.radio.hostBank, ctx=S47.audio.ctx;
              bank.buffers.clear(); bank.bytes=0;
              window.__realRadioDecode=ctx.decodeAudioData;
              ctx.decodeAudioData=()=>Promise.reject(new DOMException('Intentional radio decode failure','EncodingError'));
            }""")
            try:
                await fixture(page, short["text"])
                await pump(page, until="!S47.radio.stations.caliche.hostPending && S47.radio.stations.caliche.hostReady")
                await pump(page, 0.32)
                failed = await page.evaluate("__artProbe.snapshot()")
            finally:
                await page.evaluate("() => { S47.audio.ctx.decodeAudioData=__realRadioDecode; }")
            check(not failed["starts"] and not failed["pending"] and not failed["spoken"][0]["ids"] and
                  any(x["text"] == short["text"] and x["duration"] is None for x in failed["lines"]),
                  "a failed real decoder returns to the complete procedural host and caption without a partial recorded start")
            report["decode_failure"] = failed

            report["late_completion"] = {}
            for action in ("Off", "station change", "reset", "load case"):
                report["late_completion"][action] = await late_completion(page, short["text"], action, saved)
            # Loading the saved chapter restored an on-foot scene. Re-enter
            # through its real interaction before testing active source stops.
            await enter_truck(page)
            await foreground_radio(page)
            report["active_cancellation"] = {}
            for action in ("Off", "station change", "reset", "load case", "title"):
                await page.evaluate("S47.radio.setHostVoice(true)")
                await fixture(page, opening["text"], lines=[
                    {"who": "host", "text": opening["text"]},
                    {"who": "host", "text": signoff["text"]},
                ])
                active = await loaded(page)
                if action == "Off":
                    await pause(page)
                    await page.locator("[data-a=artbell]").click()
                    await resume(page)
                elif action == "station change":
                    await page.keyboard.press("r")
                    await page.evaluate("S47.tick(0.02,60)")
                elif action == "reset":
                    await page.evaluate("S47.radio.reset(S47.game.clock)")
                elif action == "load case":
                    await pause(page)
                    await page.locator(".settings-panel [data-a=load]").click()
                    await page.locator(f'.savemenu [data-load="{saved["id"]}"]').click()
                    await wait_for_js(page, "S47.started() && !S47.world.driving && !S47.game.cinematic")
                    await page.evaluate("S47.hold=true")
                else:
                    await pause(page)
                    await page.locator("[data-a=title]").click()
                await asyncio.sleep(0.12)
                stopped = await page.evaluate("__artProbe.snapshot()")
                check(active["sources"] == 2 and len(active["starts"]) == 2 and
                      active["starts"][1]["at"] > active["now"] and
                      cancelled_or_completed(stopped["starts"]) and stopped["sources"] == 0,
                      f"{action}: all active and future recorded host sources are stopped")
                report["active_cancellation"][action] = stopped
                if action == "load case":
                    await enter_truck(page)
                    await foreground_radio(page)
            check(await page.evaluate("S47.radio.hostBank.bufferCount <= 24"),
                  "host buffer caching remains within the bounded 24-buffer bank")
            await context.close()

            # Touch settings are exercised in a separate fresh local storage
            # context, then reloaded to verify the persisted choice.
            mobile = await browser.new_context(viewport={"width": 844, "height": 390},
                                               is_mobile=True, has_touch=True, device_scale_factor=1)
            page = await new_page(mobile)
            await pause(page, touch=True)
            toggle = page.locator(".settings-panel [data-a=artbell]")
            await toggle.scroll_into_view_if_needed()
            check(await toggle.get_attribute("aria-pressed") == "true" and await no_horizontal_overflow(page, ".settings-panel"),
                  "844x390 touch settings show Art Bell On and fit the screen")
            await page.screenshot(path=str(OUT / "r02_mobile_settings_on.png"))
            await toggle.tap()
            check(await page.evaluate("!S47.radio.hostVoice && localStorage.getItem('s47.artBellRadioHost')==='false' && !S47.game.d.ui.readAloud.enabled"),
                  "a real touch turns only Art Bell Off and stores its separate s47 preference")
            await page.locator("[data-a=kinski]").tap()
            check(await page.evaluate("S47.game.d.ui.readAloud.enabled && !S47.radio.hostVoice && localStorage.getItem('s47.kinskiReadAloud')==='true'"),
                  "turning Kinski On by touch does not change the Art Bell preference")
            await page.screenshot(path=str(OUT / "r03_mobile_independent_preferences.png"))
            await resume(page, touch=True)
            await page.reload()
            await start_night(page)
            await pause(page, touch=True)
            check(await page.locator("[data-a=artbell]").get_attribute("aria-pressed") == "false" and
                  await page.locator("[data-a=kinski]").get_attribute("aria-pressed") == "true" and
                  not await page.evaluate("S47.radio.hostVoice"),
                  "reloading retains the independent Art Bell Off and Kinski On choices")
            await mobile.close()

            check(not bad_responses, f"recorded host MP3 requests have no HTTP failures ({len(requests)} requests)")
            if not PAGES:
                check(not requests, "strict-CSP single-file radio playback decodes inline MP3s without external host requests")
            ignored = ("GPU stall", "Automatic fallback to software WebGL", "favicon")
            noise = [e for e in errors if not any(term in e for term in ignored)]
            check(not noise, "no browser errors or warnings" + (": " + " | ".join(noise[:4]) if noise else ""))
            report.update({"requests": requests, "bad_responses": bad_responses, "errors": noise})
            await browser.close()
    finally:
        if csp_dir:
            csp_dir.cleanup()
        report["checks"] = CHECKS
        report["physical_speakers"] = "UNVERIFIED: real decode/PCM/graph/lifecycle evidence is not a hardware listening or voice-similarity test"
        (OUT / "artbell-radio.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    passed = sum(c["pass"] for c in CHECKS)
    print(f"{passed} of {len(CHECKS)} PASS", flush=True)
    return 0 if passed == len(CHECKS) else 1


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
