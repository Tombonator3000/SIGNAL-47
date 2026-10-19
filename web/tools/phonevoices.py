"""Real telephone interactions, decoded MP3s and pausable story/audio clocks.

Run from web/: python3 tools/phonevoices.py OUTDIR
S47_URL selects the ordinary Pages build. Otherwise test dist-single under the
same strict CSP as readaloud.py. No audio is substituted and no speaker listening
claim is made: buffer lifecycle, graph routing and analyser samples are observed.
"""
import asyncio
import json
import os
import sys
import tempfile
from pathlib import Path

from playwright.async_api import async_playwright

from readaloud import CSP, INSTRUMENT, normalized, sample_master, start_night, wait_for_js


OUT = Path(sys.argv[1] if len(sys.argv) > 1 else "shots/phonevoices")
PAGES = bool(os.environ.get("S47_URL"))
URL = os.environ.get("S47_URL") or Path("dist-single/index.html").resolve().as_uri()
CHECKS = []
WARD = [
    "That is enough to hold the morning series. It is not enough to tell us why it happened.",
    "Check the reference record. If the instrument inherited the fault, the record may tell us from where.",
]
TOMAS = "Reference west. No. East. Hold the last reading."
CH4 = [
    "Where does that phone go?",
    "To the office. At night the office line is patched through to this room, and the other end of it is the field hut. It has been since 1947.",
    "You left the receiver on out there. Listen.",
    "Tomás. Where did you leave the spare key?", TOMAS,
    "That is what he said then. Not an answer to what I asked now.",
]
CH5_OPEN = ["Ward.", "It is Reyes. I am at the diner on the old road.",
            "I read the telex. All of it.", "Hondo called. So did Site 11."]
CH5_WHAT = ["What did they say?", "Enough. I am not doing this on a pay phone."]
CH5_ROAD = ["I am checking the old road first.",
            "Then check it, and come back. I want you on the console before the morning series.",
            "Six o'clock, Reyes."]
NORA = [
    "Sierra Motor Court.",
    "Station One. Nobody has asked me for that key in a long time.",
    "Don't send me copies. Go out to the station first and look at the cable, where it was cut. Then bring me the originals.",
]
FIELD_NORA = [
    "So you went out there.",
    "This line still rings in the motel office. It has since 1947. Nobody ever took it down.",
    "You have seen the cable now. It did not break by itself.",
    "Room 6. Bring the original record and your film, developed. Then I will tell you the rest.",
]
PHONE_TEXTS = WARD + NORA + FIELD_NORA + CH4 + CH5_OPEN + CH5_WHAT + CH5_ROAD

# Extend readaloud's real decode/start/end probe with buffer identity and the
# actual path to the SFX bus. It records connections without changing routing.
PHONE_GRAPH = r"""(() => {
  const edges = new WeakMap(), ids = new WeakMap(); let serial = 0;
  const connect = AudioNode.prototype.connect;
  AudioNode.prototype.connect = function (destination, ...rest) {
    const result = connect.call(this, destination, ...rest);
    if (destination instanceof AudioNode) {
      let targets = edges.get(this); if (!targets) edges.set(this, targets = new Set());
      targets.add(destination);
    }
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
    const result = start.apply(this, args), audio = window.S47?.audio;
    const record = window.__readingTrace.starts.findLast(r => r.source === this);
    if (record && audio) {
      if (!ids.has(this.buffer)) ids.set(this.buffer, ++serial);
      record.bufferId = ids.get(this.buffer);
      record.phoneKey = Object.keys(audio.buf).find(k => k.startsWith('phone:') && audio.buf[k] === this.buffer) || null;
      record.sfxPath = (path(this, audio.sfx) || []).map(n => n instanceof BiquadFilterNode
        ? {kind: n.type, hz: n.frequency.value} : {kind: n === audio.sfx ? 'sfx' : n.constructor.name});
    }
    return result;
  };
})();"""

PHONE_PROBE = r"""() => {
  const a = S47.audio, ui = S47.game.d.ui;
  if (window.__phoneProbe) return;
  const probe = window.__phoneProbe = {calls: [], captions: [], dial: [], ringbacks: [], clock: 0, remainder: 0};
  const call = a.phoneCall.bind(a);
  a.phoneCall = (lines, options) => {
    const at = a.ctx.currentTime, gt = S47.game.gt;
    const timeline = call(lines, options);
    probe.calls.push({at, gt, lines: lines.map(l => ({...l})), timeline});
    return timeline;
  };
  const toast = ui.toast.bind(ui);
  ui.toast = (text, seconds) => {
    probe.captions.push({text, at: a.ctx.currentTime, gt: S47.game.gt});
    return toast(text, seconds);
  };
  for (const [method, records] of [['dial', probe.dial], ['ringback', probe.ringbacks]]) {
    const original = a[method].bind(a);
    a[method] = (...args) => {
      const at = a.ctx.currentTime, duration = original(...args);
      records.push({at, duration}); return duration;
    };
  }
  probe.reset = () => {
    __readingTrace.reset(); probe.calls.length = 0; probe.captions.length = 0;
    probe.dial.length = 0; probe.ringbacks.length = 0;
    probe.clock = a.ctx.currentTime; probe.remainder = 0;
  };
  probe.tick = () => {
    const now = a.ctx.currentTime;
    probe.remainder += Math.max(0, now - probe.clock); probe.clock = now;
    const frames = Math.floor(probe.remainder * 60);
    if (frames) { probe.remainder -= frames / 60; S47.tick(frames / 60, 60); }
    return {now, gt: S47.game.gt};
  };
  probe.snapshot = () => ({calls: probe.calls, captions: probe.captions, dial: probe.dial, ringbacks: probe.ringbacks,
    starts: __readingTrace.snapshot().filter(r => r.phoneKey)});
}"""


def check(ok, message):
    CHECKS.append({"pass": bool(ok), "message": message})
    print(("PASS " if ok else "FAIL ") + message, flush=True)


def speech_texts(call):
    return [line["text"] for line in call["lines"] if not line.get("aside")]


def mp3(record):
    head = (record.get("decoded") or {}).get("head", [])
    return len(head) >= 3 and (head[:3] == [73, 68, 51] or head[0] == 255 and head[1] & 224 == 224)


def telephone_route(record):
    filters = [(n["kind"], n.get("hz")) for n in record.get("sfxPath", [])
               if n["kind"] in ("highpass", "lowpass", "bandpass")]
    return filters == [("highpass", 320), ("lowpass", 3200)] and record.get("master")


def stopped(records):
    # A short first line may already have ended before the user changes topic.
    # Every remaining source must stop, including sources scheduled in the future.
    def cancelled(r):
        return r["stoppedAt"] is not None and r["stoppedAt"] < r["expectedEnd"] - 0.1

    def completed(r):
        return r["endedAt"] is not None and r["endedAt"] >= r["expectedEnd"] - 0.08

    return bool(records) and any(cancelled(r) for r in records) and all(
        cancelled(r) or completed(r) for r in records)


async def pump(page, seconds=0, until=None, timeout=60000):
    """Advance game timers only as the real, pausable AudioContext progresses."""
    begin = await page.evaluate("S47.audio.ctx.currentTime")
    deadline = asyncio.get_running_loop().time() + timeout / 1000
    while True:
        state = await page.evaluate("__phoneProbe.tick()")
        if until and await page.evaluate(until):
            return state
        if not until and state["now"] - begin >= seconds:
            return state
        if asyncio.get_running_loop().time() >= deadline:
            raise TimeoutError("Telephone audio/story clock did not reach " + (until or str(seconds)))
        await asyncio.sleep(0.06)


async def foreground_phone(page):
    # The handset reaches master through SFX, so SFX must remain audible here.
    await page.evaluate("""() => {
      const a = S47.audio;
      for (const bus of [a.amb, a.music, a.radioBus]) {
        bus.gain.cancelScheduledValues(a.ctx.currentTime); bus.gain.value = 0;
      }
      a.sfx.gain.cancelScheduledValues(a.ctx.currentTime); a.sfx.gain.value = 1;
      a.setVolume(0.8);
    }""")


async def ward_call(page):
    await page.evaluate("S47.game.d.ui.close(); S47.jump('chapter2')")
    await wait_for_js(page, "S47.game.phase === 'ch2' && !!S47.game.d.inter.get('phone')")
    await page.evaluate("__phoneProbe.reset(); S47.game.d.inter.get('phone').use()")
    return await page.evaluate("__phoneProbe.snapshot()")


async def pause(page):
    await page.keyboard.press("Escape")
    await page.wait_for_selector(".settings-panel [data-a=resume]")
    await wait_for_js(page, "S47.audio.ctx.state === 'suspended'")


async def resume(page):
    await page.locator(".settings-panel [data-a=resume]").click()
    await wait_for_js(page, "S47.audio.ctx.state === 'running'")


async def short_call_branch(page, texts, by_text, label):
    """Verify real branch scheduling, then abort; Ward covers natural completion."""
    snapshot = await page.evaluate("__phoneProbe.snapshot()")
    call, sources = snapshot["calls"][-1], snapshot["starts"]
    check(speech_texts(call) == texts and call["timeline"] is not None,
          f"{label} selects every exact quotation and retains stage directions as unvoiced cues")
    keys = ["phone:" + by_text[normalized(t)]["id"] for t in texts]
    check([r["phoneKey"] for r in sources] == keys and all(mp3(r) and telephone_route(r) for r in sources),
          f"{label} schedules its {len(texts)} actual decoded MP3 identities through the handset path")
    timeline = call["timeline"]
    timers = await page.evaluate("S47.game.timers.map(t => t.at)")
    completion = next((t for t in timers if abs(t - call["gt"] - timeline["duration"] - 0.1) < 0.03), None)
    end = max(c["at"] + c["duration"] for c in timeline["cues"])
    voiced = [c for c in timeline["cues"] if not c.get("aside")]
    check(completion is not None and call["gt"] + end <= completion and len(voiced) == len(sources) and all(
        abs(r["duration"] - cue["duration"]) < 0.002 and abs(r["at"] - call["at"] - cue["at"]) < 0.03
        for r, cue in zip(sources, voiced)),
        f"{label} schedules scene completion after the last full clip, with no voiced aside or history")
    await pump(page, 0.4)
    check(await page.evaluate("S47.audio.ctx.currentTime") > sources[0]["at"] and
          (await page.evaluate("__phoneProbe.snapshot()"))["starts"][0]["stoppedAt"] is None,
          f"{label}'s first scheduled MP3 reaches its real AudioContext start")
    snapshot = await page.evaluate("__phoneProbe.snapshot()")
    await page.evaluate("S47.game.reset()")
    check(stopped((await page.evaluate("__phoneProbe.snapshot()"))["starts"]),
          f"{label}'s short branch check cancels all remaining scheduled speech on reset")
    return {"audio": snapshot, "completion_gt": completion,
            "limitation": "First start and complete scheduling verified; this branch was deliberately reset before full playback. No hardware listening test."}


async def main():
    global URL
    OUT.mkdir(parents=True, exist_ok=True)
    clips = json.loads(Path("src/assets/audio/kinski/catalog.json").read_text())["clips"]
    by_text = {normalized(c["text"]): c for c in clips}
    expected_keys = {"phone:" + by_text[normalized(t)]["id"] for t in PHONE_TEXTS}
    check(len(PHONE_TEXTS) == len(expected_keys) == 24, "all 24 authored telephone recordings exist in the catalogue")
    phone_ids = {key.removeprefix("phone:") for key in expected_keys}
    csp_dir = None
    if not PAGES:
        csp_dir = tempfile.TemporaryDirectory(prefix="s47-phonevoices-csp-")
        target = Path(csp_dir.name) / "index.html"
        target.write_text(Path("dist-single/index.html").read_text().replace(
            "<head>", '<head><meta http-equiv="Content-Security-Policy" '
            f'content="{CSP}">', 1))
        URL = target.as_uri()
    errors, requests, bad_responses = [], [], []
    report = {"url": URL, "pages": PAGES, "strict_csp": not PAGES}

    def phone_url(url):
        name = url.split("?", 1)[0].rsplit("/", 1)[-1]
        return any(name == ident + ".mp3" or name.startswith(ident + "-") and name.endswith(".mp3")
                   for ident in phone_ids)

    try:
        async with async_playwright() as playwright:
            browser = await playwright.chromium.launch(args=[
                "--use-angle=swiftshader", "--enable-unsafe-swiftshader", "--ignore-gpu-blocklist",
            ])
            context = await browser.new_context(viewport={"width": 1280, "height": 800})
            page = await context.new_page()
            page.set_default_timeout(60000)
            await page.add_init_script(INSTRUMENT + PHONE_GRAPH)
            page.on("pageerror", lambda e: errors.append("PAGEERROR: " + str(e)))
            page.on("console", lambda m: errors.append(m.type + ": " + m.text)
                    if m.type in ("error", "warning") else None)
            page.on("request", lambda r: requests.append(r.url) if phone_url(r.url) else None)
            page.on("response", lambda r: bad_responses.append((r.status, r.url))
                    if phone_url(r.url) and r.status >= 400 else None)
            await page.goto(URL)
            await wait_for_js(page, "!!window.S47")
            await page.evaluate("localStorage.clear()")
            await page.reload()
            await start_night(page)
            check(await page.evaluate("!S47.audio.kinskiCalls"), "telephone voices default to Off on a fresh device")
            await pause(page)
            await page.locator("button[data-a=kinski]").click()
            check(await page.evaluate("S47.audio.kinskiCalls && localStorage.getItem('s47.kinskiReadAloud') === 'true'"),
                  "the real Kinski setting enables telephone voices and persists it")
            await resume(page)
            await page.reload()
            await start_night(page)
            await page.evaluate(PHONE_PROBE)
            await foreground_phone(page)
            check(await page.evaluate("S47.audio.kinskiCalls"), "telephone voices remain enabled after reload")
            loaded = await page.evaluate("Object.keys(S47.audio.buf).filter(k => k.startsWith('phone:'))")
            check(set(loaded) == expected_keys, "AudioSys preloads exactly the 24 telephone MP3 buffers")

            first = await ward_call(page)
            call = first["calls"][0]
            check(speech_texts(call) == WARD and call["timeline"] is not None,
                  "the real Chapter 2 phone interaction selects both authored Ward lines")
            sources = first["starts"]
            check(len(sources) == 2 and all(mp3(r) for r in sources),
                  "both Ward sources use genuinely decoded MP3s")
            check(all(telephone_route(r) for r in sources),
                  "Ward speech follows high-pass 320 Hz → low-pass 3200 Hz → SFX → master")
            timeline = call["timeline"]
            check(len(timeline["cues"]) == 2 and all(
                abs(r["duration"] - c["duration"]) < 0.002 and
                abs(r["at"] - call["at"] - c["at"]) < 0.03
                for r, c in zip(sources, timeline["cues"])),
                "caption cues and scheduled speech use each real decoded buffer's full duration")
            await pump(page, 0.65)
            audible = await sample_master(page, 400)
            check(audible["peakRms"] > 0.0001, "the live handset path produces nonzero master analyser samples")
            await page.evaluate("S47.audio.setVolume(0)")
            # Drain the analyser's existing 2048-sample block before measuring
            # steady mute, rather than counting the pre-mute transition as sound.
            await pump(page, 0.15)
            silent = await sample_master(page, 350)
            check(silent["peakRms"] < 0.00001, "master volume zero silences handset analyser samples")
            await page.evaluate("S47.audio.setVolume(0.8)")
            await pump(page, 0.1)
            await pause(page)
            before = await page.evaluate("({at:S47.audio.ctx.currentTime,gt:S47.game.gt})")
            await asyncio.sleep(0.35)
            await page.evaluate("S47.tick(0.4)")
            after = await page.evaluate("({at:S47.audio.ctx.currentTime,gt:S47.game.gt})")
            check(abs(after["at"] - before["at"]) < 0.03 and abs(after["gt"] - before["gt"]) < 0.002,
                  "pause holds both the AudioContext clock and story timers")
            check(not await page.evaluate("S47.ch2.s.answered"), "pause does not prematurely finish the Ward call")
            await page.screenshot(path=str(OUT / "p01_ward_paused.png"))
            await resume(page)
            await pump(page, until="S47.ch2.s.answered", timeout=int((timeline["duration"] + 15) * 1000))
            await asyncio.sleep(0.15)
            full = await page.evaluate("__phoneProbe.snapshot()")
            check(len(full["starts"]) == 2 and all(
                r["endedAt"] is not None and r["endedAt"] >= r["expectedEnd"] - 0.08 and
                (r["stoppedAt"] is None or r["stoppedAt"] >= r["expectedEnd"] - 0.08)
                for r in full["starts"]), "both Ward buffers reach their natural ends without truncation")
            captions = [c for c in full["captions"] if c["text"].startswith('WARD: "')]
            check([c["text"] for c in captions] == [f'WARD: "{t}"' for t in WARD] and all(
                abs(c["gt"] - call["gt"] - cue["at"]) < 0.12
                for c, cue in zip(captions, timeline["cues"])),
                "both exact Ward captions follow the audio-derived game-time cues")
            check(await page.evaluate("S47.ch2.s.stage === 'records' && !S47.game.cinematic && S47.room.handset.visible"),
                  "the handset and chapter advance only after the complete spoken call")
            report["ward"] = {"call": call, "audio": full, "master": audible, "muted_master": silent}

            # A safe completed-call save; loading it must not preserve a later call.
            saved = await page.evaluate("S47.saveNow('manual', 0)")
            await ward_call(page)
            await pump(page, 0.4)
            await pause(page)
            await page.locator(".settings-panel [data-a=load]").click()
            await page.locator(f'.savemenu [data-load="{saved["id"]}"]').click()
            await wait_for_js(page, "S47.started() && S47.ch2.s.answered && !S47.game.cinematic")
            check(stopped((await page.evaluate("__phoneProbe.snapshot()"))["starts"]),
                  "loading a real saved case stops both current and future telephone sources")
            await foreground_phone(page)
            await ward_call(page)
            await pump(page, 0.4)
            await page.evaluate("S47.game.reset()")
            check(stopped((await page.evaluate("__phoneProbe.snapshot()"))["starts"]),
                  "reset cancels the old telephone session, including its queued second line")

            # Seed only the two authored branch prerequisites. Keep dial/ringback
            # and the real telephone handlers intact; do not skip their audio clock.
            await page.evaluate("""() => {
              S47.jump('chapter2'); S47.audio.stop('ring2', 0.01); S47.ch2.ringing=false;
              S47.ch2.s.answered=true; S47.ch2.s.stage='call-nora';
              __phoneProbe.reset(); S47.ch2['useRecPhone']();
            }""")
            await pump(page, until="__phoneProbe.calls.length > 0", timeout=25000)
            outgoing = await page.evaluate("__phoneProbe.snapshot()")
            ringback = outgoing["ringbacks"][0]
            check(len(outgoing["dial"]) == 1 and outgoing["calls"][0]["at"] >= ringback["at"] + ringback["duration"] + 0.1,
                  "Chapter 2 Nora speech starts only after the genuine dial and ringback interval")
            report["chapter2_nora"] = await short_call_branch(page, NORA, by_text, "Chapter 2 Nora")
            await page.evaluate("""() => {
              __phoneProbe.reset(); S47.ch3.ringing=true; S47.ch3.s.called=false;
              S47.ch3['usePhone']();
            }""")
            report["chapter3_field_nora"] = await short_call_branch(page, FIELD_NORA, by_text, "Chapter 3 field Nora")

            # The receiver and room-6 repeat must share the same preloaded buffer.
            await page.evaluate("__phoneProbe.reset(); S47.ch3.s.heard=false; S47.ch3['useReceiver']()")
            receiver = await page.evaluate("__phoneProbe.snapshot()")
            check(speech_texts(receiver["calls"][-1]) == [TOMAS] and len(receiver["starts"]) == 1,
                  "the Chapter 3 receiver selects the authored Tomás fragment")
            receiver_buffer = receiver["starts"][0]["bufferId"]
            check(receiver["starts"][0]["phoneKey"] == "phone:d111f9a951311961",
                  "the receiver uses the existing d111f9a951311961 Tomás recording")
            await page.evaluate("""() => {
              const c = S47.ch4; c.s.greeted=true; c.s.line=false; c.mode='menu'; c.openTalk();
            }""")
            await page.locator(".talk [data-c=line]").click()
            room6 = await page.evaluate("__phoneProbe.snapshot()")
            room_call = room6["calls"][-1]
            room_sources = room6["starts"][-6:]
            check(speech_texts(room_call) == CH4 and len(room_sources) == 6,
                  "the room-6 phone choice automatically voices all six quotations, without asides")
            repeated = [r for r in room_sources if r["phoneKey"] == "phone:d111f9a951311961"]
            check(len(repeated) == 1 and repeated[0]["bufferId"] == receiver_buffer,
                  "room 6 repeats the identical decoded Tomás buffer from the Chapter 3 receiver")
            check(all(telephone_route(r) for r in room_sources), "all room-6 quotations retain the handset filter path")
            await page.locator(".labpanel [data-a=close]").click()
            check(stopped((await page.evaluate("__phoneProbe.snapshot()"))["starts"][-6:]),
                  "closing the room-6 conversation cancels all six queued telephone sources")

            await page.evaluate("__phoneProbe.reset(); S47.ch5.s.ward=false; S47.ch5['openTalk']('ward')")
            opening = await page.evaluate("__phoneProbe.snapshot()")
            check(speech_texts(opening["calls"][-1]) == CH5_OPEN and len(opening["starts"]) == 4,
                  "the payphone automatically speaks its four opening quotations, excluding the stage direction")
            await pump(page, 0.4)
            await page.locator(".talk [data-c=what]").click()
            question = await page.evaluate("__phoneProbe.snapshot()")
            check(speech_texts(question["calls"][-1]) == CH5_WHAT and len(question["starts"]) == 6,
                  "Ask what they said speaks only the two new lines, without repeating retained history")
            check(stopped(question["starts"][:4]), "a new payphone choice stops the previous queue before its replacement")
            count = len(question["calls"])
            await page.evaluate("S47.ch5.panel.refresh()")
            check(await page.evaluate("__phoneProbe.calls.length") == count,
                  "refreshing unchanged payphone text does not replay the phone audio")
            await page.locator(".talk [data-c=road]").click()
            road = await page.evaluate("__phoneProbe.snapshot()")
            check(speech_texts(road["calls"][-1]) == CH5_ROAD and len(road["starts"]) == 9,
                  "the road choice speaks only its three new quotations, excluding She hangs up")
            check(stopped(road["starts"][4:6]), "the final payphone queue replaces the previous two lines without overlap")
            await page.screenshot(path=str(OUT / "p02_payphone.png"))
            await page.locator(".labpanel [data-a=close]").click()
            check(stopped((await page.evaluate("__phoneProbe.snapshot()"))["starts"][-3:]),
                  "closing the payphone panel cancels the final queued lines")
            report["payphone"] = road

            await ward_call(page)
            await pump(page, 0.4)
            await pause(page)
            await page.locator(".settings-panel [data-a=title]").click()
            check(stopped((await page.evaluate("__phoneProbe.snapshot()"))["starts"]),
                  "quitting to title stops a current call and its future second line")
            await start_night(page)
            check(await page.evaluate("!S47.game.cinematic && __readingTrace.snapshot().filter(r=>r.phoneKey && r.stoppedAt===null).length===0"),
                  "a real New night starts without old telephone voices")
            await foreground_phone(page)
            await ward_call(page)
            await pump(page, 0.4)
            await pause(page)
            await page.locator("button[data-a=kinski]").click()
            check(await page.evaluate("!S47.audio.kinskiCalls && localStorage.getItem('s47.kinskiReadAloud') === 'false'") and
                  stopped((await page.evaluate("__phoneProbe.snapshot()"))["starts"]),
                  "turning Kinski Off immediately stops the whole active phone queue")
            await resume(page)
            baseline = await ward_call(page)
            check(baseline["calls"][0]["timeline"] is None and not baseline["starts"],
                  "with Kinski Off the same real phone interaction uses the original unvoiced call")
            await page.evaluate("S47.tick(10.5)")
            baseline = await page.evaluate("__phoneProbe.snapshot()")
            check(await page.evaluate("S47.ch2.s.answered && !S47.game.cinematic") and
                  [c["text"] for c in baseline["captions"] if c["text"].startswith('WARD: "')] == [f'WARD: "{t}"' for t in WARD],
                  "the original Off timing, exact captions and chapter progression still work")
            check(not bad_responses, f"telephone MP3 requests have no HTTP failures ({len(requests)} requests)")
            if not PAGES:
                check(not requests, "strict-CSP single-file playback needs no external telephone MP3 request")
            ignored = ("GPU stall", "Automatic fallback to software WebGL", "favicon")
            noise = [e for e in errors if not any(term in e for term in ignored)]
            check(not noise, "no browser errors or warnings" + (": " + " | ".join(noise[:4]) if noise else ""))
            report.update({"requests": requests, "bad_responses": bad_responses, "errors": noise})
            await browser.close()
    finally:
        if csp_dir:
            csp_dir.cleanup()
        report["checks"] = CHECKS
        report["physical_speakers"] = "UNVERIFIED: analyser samples are not a hardware listening test"
        (OUT / "phonevoices.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    passed = sum(c["pass"] for c in CHECKS)
    print(f"{passed} of {len(CHECKS)} PASS", flush=True)
    return 0 if passed == len(CHECKS) else 1


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
