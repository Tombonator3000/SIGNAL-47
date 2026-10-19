"""Recorded callers and the 1947 bulletin, observed through real Web Audio.

Run from web/ through the project's serial GPU wrapper:
  python3 tools/radiocast-radio.py production/radiocast/qa/pages
S47_URL selects the served Pages build. Without it, dist-single is loaded with
the same strict CSP as readaloud.py. No sound is substituted. Manuscript fixtures
test each exact role/text recording; the bulletin and boy also use actual event
selection. A complete mixed exchange and the 47-second bulletin play naturally.
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


# Reuse the existing native decode, source, AM graph and audio-clock probes.
_spec = importlib.util.spec_from_file_location("artbell_radio_probe", Path(__file__).with_name("artbell-radio.py"))
art = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(art)
OUT = Path(sys.argv[1] if len(sys.argv) > 1 else "production/radiocast/qa/single")
PAGES = bool(os.environ.get("S47_URL"))
URL = os.environ.get("S47_URL") or Path("dist-single/index.html").resolve().as_uri()
CHECKS = []
GRAPH = art.RADIO_GRAPH.replace(
    "record.hostId = [...s.radio.hostBank.buffers].find(([, b]) => b === this.buffer)?.[0] || null;",
    """record.hostId = [...s.radio.hostBank.buffers].find(([, b]) => b === this.buffer)?.[0] || null;
      record.castId = [...s.radio.callerBank.buffers].find(([, b]) => b === this.buffer)?.[0] || null;""")
PROBE = art.RADIO_PROBE.replace(
    "starts: __readingTrace.snapshot().filter(r => r.hostId),",
    "starts: __readingTrace.snapshot().filter(r => r.hostId || r.castId),")
PROBE = PROBE.replace("const bank = radio.hostBank, original = bank.load;",
                      "const bank = radio.callerBank, original = bank.load;")
PROBE = PROBE.replace("secs:l.secs, ids:l.clips?.map(c=>c.id) ?? [], buffers:l.buffers?.length ?? 0})),",
                      "secs:l.secs, ids:l.clips?.map(c=>c.id) ?? [], buffers:l.buffers?.length ?? 0})),\n"
                      "    notes: S47.game.notes.filter(n=>n.includes('AM 1420, a news bulletin')),\n"
                      "    seen: [...S47.townSeen].filter(k=>k.startsWith('ne:bulletin') || k.startsWith('mark:boy') || k==='ev:boy'),")


def check(ok, message):
    CHECKS.append({"pass": bool(ok), "message": message})
    print(("PASS " if ok else "FAIL ") + message, flush=True)


def cast_starts(snapshot):
    return [r for r in snapshot["starts"] if r.get("castId")]


def caption_for(text, by="a caller"):
    return f'RADIO, {by}: "{text}"' if by else f'RADIO: "{text}"'


def exact_caption(snapshot, text, duration, by="a caller"):
    captions = [c for c in snapshot["captions"] if c["text"] == caption_for(text, by)]
    return (len(captions) == 1 and captions[0]["audioClock"] and
            abs(captions[0]["seconds"] - max(2.4, duration + 0.2)) < 0.015)


async def snapshot(page):
    return await page.evaluate("__artProbe.snapshot()")


async def fixture(page, lines, *, delay=False, clock=2 * 3600 + 40 * 60):
    await page.evaluate("S47.radio.setCallerVoice(true)")
    await art.fixture(page, lines[0]["text"], lines=lines, clock=clock, delay_load=delay)


async def loaded(page):
    await art.pump(page, until="!S47.radio.stations.caliche.hostPending && S47.radio.stations.caliche.hostReady")
    await art.pump(page, 0.29)
    return await snapshot(page)


async def recover_truck(page):
    if not await page.evaluate("S47.started()"):
        await start_night(page)
    if not await page.evaluate("S47.world.driving && !S47.game.cinematic"):
        await art.enter_truck(page)
    await page.evaluate("""() => {
      S47.setAbductions(false); S47.nightEvents.wait=1e9;
      S47.radio.boyCalled=true; S47.radio.setHostVoice(true); S47.radio.setCallerVoice(true);
    }""")
    await art.foreground_radio(page)


async def ordinary_programme(page):
    """Select a real ordinary loop segment, then let the real event director cut in."""
    await recover_truck(page)
    await page.evaluate("""() => {
      const st=S47.radio.stations.caliche;
      S47.game.clock=2*3600+40*60; S47.carRadio.set(true,'caliche'); S47.tick(.02,60);
      const seg=st.cfg.loop.find(s=>s.kind==='talk' && s.lines.some(l=>l.who==='host'));
      st.start([seg],S47.game.clock); st.prev=S47.game.clock; st.calm=1e9;
      S47.nightEvents.reset(); S47.nightEvents.wait=1e9;
      for(const k of [...S47.townSeen]) if(k.startsWith('ne:bulletin')) S47.townSeen.delete(k);
      S47.game.notes=S47.game.notes.filter(n=>!n.includes('AM 1420, a news bulletin'));
      __artProbe.reset();
    }""")
    result = await loaded(page)
    check(await page.evaluate("S47.nightIn().radio1420 && S47.radio.stations.caliche.free"),
          "the bulletin fixture is a real free Night Line segment on the live truck receiver")
    return result


async def action(page, label, saved):
    if label == "Off":
        await art.pause(page)
        await page.locator(".settings-panel [data-a=radiocast]").click()
        await art.resume(page)
    elif label == "tune":
        await page.keyboard.press("r")
        await page.evaluate("S47.tick(.02,60)")
    elif label == "reset":
        await page.evaluate("S47.radio.reset(S47.game.clock)")
    elif label == "load":
        await art.pause(page)
        await page.locator(".settings-panel [data-a=load]").click()
        await page.locator(f'.savemenu [data-load="{saved["id"]}"]').click()
        await wait_for_js(page, "S47.started() && !S47.world.driving && !S47.game.cinematic")
        await page.evaluate("S47.hold=true")
    elif label == "title":
        await art.pause(page)
        await page.locator(".settings-panel [data-a=title]").click()
    else:
        raise ValueError(label)


async def cancellation(page, line, saved, pending):
    results = {}
    for label in ("Off", "tune", "reset", "load", "title"):
        await recover_truck(page)
        await fixture(page, [line, line], delay=pending)
        if pending:
            await wait_for_js(page, "__artProbe.late.ready && !!S47.radio.stations.caliche.hostPending")
            before = await snapshot(page)
            check(before["pending"] and not cast_starts(before),
                  f"pending {label}: genuine decoded caller data is held before source start")
        else:
            before = await loaded(page)
        await action(page, label, saved)
        if pending:
            await page.evaluate("__artProbe.releaseLate()")
        await asyncio.sleep(0.15)
        after = await snapshot(page)
        if pending:
            check(await page.evaluate("__artProbe.late.aborted") and not after["pending"] and
                  not cast_starts(after) and after["sources"] == 0,
                  f"pending {label}: a late real decode cannot revive callers or future sources")
        else:
            starts = cast_starts(after)
            check(len(cast_starts(before)) == 2 and cast_starts(before)[1]["at"] > before["now"] and
                  art.cancelled_or_completed(starts) and after["sources"] == 0,
                  f"active {label}: both the current and future caller buffers stop")
        results[label] = {"before": before, "after": after}
    return results


async def bulletin(page, news):
    evidence = {}
    await ordinary_programme(page)
    # Genuine load/decode is gated after the old host has settled. This cannot
    # accidentally gate an earlier wake-up load, nor manufacture test audio.
    accepted = await page.evaluate("""() => {
      __artProbe.reset(); __artProbe.delayNextLoad();
      return S47.nightEvents.now('bulletin',S47.nightIn());
    }""")
    check(accepted, "the actual NightEvents bulletin trigger accepts the live free station")
    await wait_for_js(page, "__artProbe.late.ready && !!S47.radio.stations.caliche.hostPending")
    await page.evaluate("S47.tick(49,60)")
    cold = await snapshot(page)
    archive = cold["state"]["caliche"]["archive"]
    check(archive["state"] == "loading" and archive["elapsed"] < 0.05 and
          not cold["notes"] and "ne:bulletin1" not in cold["seen"] and not cast_starts(cold),
          "49 simulated seconds during cold genuine decode cannot advance or award the 47-second bulletin")
    await page.evaluate("__artProbe.releaseLate()")
    await loaded(page)
    await art.pump(page, 0.45)
    # Leaving the receiver during the real event invalidates its full-listen
    # award. The event must become retryable rather than stick in busy state.
    await page.keyboard.press("r")
    await page.evaluate("S47.tick(.02,60)")
    await art.pump(page, 0.12)
    interrupted = await snapshot(page)
    check(interrupted["state"]["caliche"]["archive"]["state"] == "cancelled" and
          not interrupted["notes"] and "ne:bulletin1" not in interrupted["seen"] and
          art.cancelled_or_completed(cast_starts(interrupted)) and
          not await page.evaluate("S47.nightEvents.busy"),
          "tuning away cancels the partial bulletin and awards no note or completion marker")
    evidence["cold_partial"] = {"loading": cold, "cancelled": interrupted}

    await ordinary_programme(page)
    await page.evaluate("""() => {
      const st=S47.radio.stations.caliche;
      window.__heldProgramme={seg:st.seg,t:st.t,spoken:st.spoken,
        buffers:st.spoken.flatMap(l=>l.buffers||[])};
      __artProbe.reset();
    }""")
    accepted = await page.evaluate("S47.nightEvents.now('bulletin',S47.nightIn())")
    check(accepted, "the unheard bulletin can be retried through its actual event trigger")
    playing = await loaded(page)
    archive_audio_at = await page.evaluate("S47.radio.stations.caliche.audioAt")
    playing["archive_audio_at"] = archive_audio_at
    await page.screenshot(path=str(OUT / "c03_bulletin_caption.png"))
    starts = cast_starts(playing)
    check([r["castId"] for r in starts] == [c["id"] for c in news] and
          all(art.mp3(r) and art.am_route(r) and r["rate"] == 1 for r in starts),
          "all six exact bulletin MP3s use their natural rate through AM, radioBus and master")
    durations = [r["duration"] for r in starts]
    check(len(starts) == 6 and starts[-1]["expectedEnd"] - starts[0]["at"] <= 47 and all(
        b["at"] >= a["expectedEnd"] - 0.002 for a, b in zip(starts, starts[1:])) and
        playing["state"]["caliche"]["archive"]["duration"] == 47,
        "the six complete buffers fit inside exactly 47 seconds with nonoverlapping pauses")
    await art.pause(page)
    frozen = await snapshot(page)
    await asyncio.sleep(1.3)
    paused = await snapshot(page)
    check(paused["context"] == "suspended" and abs(paused["now"] - frozen["now"]) < 0.01 and
          abs(paused["state"]["caliche"]["archive"]["elapsed"] -
              frozen["state"]["caliche"]["archive"]["elapsed"]) < 0.01 and not paused["notes"],
          "a real pause freezes the bulletin audio, elapsed time and pending event award")
    await art.resume(page)
    await art.pump(page, until="S47.radio.stations.caliche.archiveStatus.elapsed>=46.65", timeout=60000)
    almost = await snapshot(page)
    check(almost["state"]["caliche"]["archive"]["state"] == "playing" and
          not almost["notes"] and "ne:bulletin1" not in almost["seen"],
          "at 46.65 actual audio seconds the event still has no premature notebook award")
    await art.pump(page, until="S47.radio.stations.caliche.archiveStatus.state==='complete' && S47.townSeen.has('ne:bulletin1')")
    complete = await snapshot(page)
    completed = cast_starts(complete)
    check(len(completed) == 6 and all(r["endedAt"] is not None and
          r["endedAt"] >= r["expectedEnd"] - 0.08 and
          (r["stoppedAt"] is None or r["stoppedAt"] >= r["expectedEnd"] - 0.08) for r in completed),
          "every bulletin buffer reaches its natural end without truncation")
    captions = [x for x in complete["lines"] if x["by"] == "a bulletin"]
    check([x["text"] for x in captions] == [c["text"] for c in news] and all(
        abs(x["duration"] - duration) < 0.002 and exact_caption(complete, x["text"], duration, "a bulletin")
        for x, duration in zip(captions, durations)),
        "six exact news captions follow the complete decoded durations once each")
    resumed = await page.evaluate("""() => {
      const st=S47.radio.stations.caliche,h=__heldProgramme;
      return {sameSegment:st.seg===h.seg,samePlan:st.spoken===h.spoken,
        sameBuffers:st.spoken.flatMap(l=>l.buffers||[]).every((b,i)=>b===h.buffers[i]),
        before:h.t,after:st.t,heldOffset:Math.max(0,h.t-.2),
        sources:__readingTrace.snapshot().filter(r=>r.hostId),
        notes:S47.game.notes.filter(n=>n.includes('AM 1420, a news bulletin')),
        seen:[...S47.townSeen].filter(k=>k.startsWith('ne:bulletin'))};
    }""")
    # The held real opening also has future time-check fragments. The earliest
    # resumed source is the current intro; the final source is a future suffix.
    resumed_current = min(resumed["sources"], key=lambda r: r["at"]) if resumed["sources"] else None
    # A frame may notice completion slightly after the 47-second boundary, but
    # no ordinary-programme source played during that scheduling delay. Its
    # recording must therefore resume at the exact held sample, without seeking.
    resumed["resume_schedule_delay"] = resumed_current["at"] - archive_audio_at - 47 if resumed_current else None
    check(resumed["sameSegment"] and resumed["samePlan"] and resumed["sameBuffers"] and
          resumed_current and 0 <= resumed["after"] - resumed["before"] < 0.16 and
          abs(resumed_current["offset"] - resumed["heldOffset"]) < 0.003,
          "the interrupted ordinary programme resumes the same decoded buffer and sample offset")
    check(len(resumed["notes"]) == 1 and resumed["seen"] == ["ne:bulletin1"] and
          not await page.evaluate("S47.nightEvents.busy"),
          "only the full actual 47-second event writes one notebook note and persistent completion marker")
    saved = await page.evaluate("S47.saveNow('manual',1)")
    persisted = await page.evaluate("id=>S47.saves.load(id)", saved["id"])
    check("ne:bulletin1" in persisted["town"] and len([n for n in persisted["case"]["notes"]
          if "AM 1420, a news bulletin" in n]) == 1,
          "a genuine saved case contains the one full-listen news note and event marker")
    evidence["complete"] = {"start": playing, "paused": paused, "almost": almost,
                            "end": complete, "resumed": resumed, "saved": saved}
    return evidence


async def main():
    global URL
    OUT.mkdir(parents=True, exist_ok=True)
    corpus = json.loads(Path("production/radiocast/corpus.json").read_text())
    catalog = json.loads(Path("src/assets/audio/radiocast/catalog.json").read_text())
    clips = catalog.get("clips", [])
    expected = {"woman": 8, "man": 8, "old": 3, "boy": 3, "news": 6}
    if len(clips) != 28 or any(c.get("duration", 0) <= 0 for c in clips):
        raise AssertionError("Generate all 28 real caller/news MP3s before this browser test")
    check(Counter(c["voice"] for c in clips) == expected and
          {c["id"] for c in clips} == {c["id"] for c in corpus["clips"]} and all(
              hashlib.sha256((c["voice"] + "\0" + normalized(c["text"])).encode()).hexdigest()[:16] == c["id"] and
              Path("src/assets/audio/radiocast", c["file"]).is_file() for c in clips),
          "all 28 role/text hashes, five cast counts and real MP3 files match the extracted source")
    news = [c for c in clips if c["voice"] == "news"]
    callers = [c for c in clips if c["voice"] != "news"]
    shortest = min(callers, key=lambda c: c["duration"])
    longest = max(callers, key=lambda c: c["duration"])
    host_catalog = json.loads(Path("src/assets/audio/artbell/catalog.json").read_text())
    host = min((c for c in host_catalog["clips"] if c["text"] in ("Smart dog.", "Not the wind, then.", "Is your dad up?")),
               key=lambda c: c["duration"])
    line = lambda clip, by="a caller": {"text": clip["text"], "who": clip["voice"], "by": by}
    all_ids = {c["id"] for c in clips + host_catalog["clips"]}
    requests, all_mp3, bad, errors = [], [], [], []
    report = {"pages": PAGES, "strict_csp": not PAGES,
              "coverage": "22 caller manuscript starts plus one complete mixed exchange; actual 47s news event and boy selection; no physical-speaker or voice-identity claim",
              "preserved_tests": {"radio.py": "explicit procedural host+callers Off for accelerated route tests",
                                  "tripradio.py": "explicit procedural host+callers Off for accelerated slots/events",
                                  "radioarchive.ts": "runtime agent preserves the procedural 47s/restore baseline",
                                  "storyexpansion.py": "unchanged: witness/payphone/TV story contracts"}}
    csp_dir = None
    if not PAGES:
        csp_dir = tempfile.TemporaryDirectory(prefix="s47-radiocast-csp-")
        target = Path(csp_dir.name, "index.html")
        target.write_text(Path("dist-single/index.html").read_text().replace(
            "<head>", '<head><meta http-equiv="Content-Security-Policy" ' + f'content="{CSP}">', 1))
        URL = target.as_uri()
    report["url"] = URL
    try:
        async with async_playwright() as p:
            browser = await p.chromium.launch(args=["--use-angle=swiftshader", "--enable-unsafe-swiftshader", "--ignore-gpu-blocklist"])

            async def new_page(context):
                page = await context.new_page()
                page.set_default_timeout(120000)
                await page.add_init_script(art.REAL_AUDIO + "\n" + GRAPH)
                page.on("pageerror", lambda e: errors.append("PAGEERROR: " + str(e)))
                page.on("console", lambda m: errors.append(m.type + ": " + m.text) if m.type in ("error", "warning") else None)
                def observe(request):
                    name = request.url.split("?", 1)[0].rsplit("/", 1)[-1]
                    if name.endswith(".mp3") and request.url.startswith(("http:", "https:")):
                        all_mp3.append(request.url)
                        if any(name.startswith(i) for i in all_ids):
                            requests.append(request.url)
                page.on("request", observe)
                page.on("response", lambda r: bad.append((r.status, r.url)) if r.status >= 400 and r.url.split("?", 1)[0].endswith(".mp3") else None)
                await page.goto(URL)
                await page.evaluate("localStorage.clear()")
                await page.reload()
                await start_night(page)
                await page.evaluate(PROBE)
                await page.evaluate("S47.setAbductions(false); S47.nightEvents.wait=1e9")
                return page

            desktop = await browser.new_context(viewport={"width": 1280, "height": 800})
            page = await new_page(desktop)
            check(await page.evaluate("S47.radio.callerVoice && S47.radio.hostVoice && !S47.game.d.ui.readAloud.enabled && !S47.carRadio.on && S47.radio.callerBank.bufferCount===0"),
                  "callers/news default On independently of Art Bell and Kinski; radio Off loads no caller buffers")
            await page.evaluate("S47.jump('chapter1'); S47.tick(1)")
            saved = await page.evaluate("S47.saveNow('manual',0)")
            await art.enter_truck(page)
            await page.evaluate("""line => {
              S47.game.clock=2*3600+40*60; S47.radio.boyCalled=true;
              S47.radio.stations.caliche.start([{kind:'talk',lines:[line]},{kind:'dead'}],S47.game.clock);
              __artProbe.reset();
            }""", line(longest))
            await page.keyboard.press("r")
            await page.evaluate("S47.tick(.02,60)")
            await art.foreground_radio(page)
            first = await loaded(page)
            check(await page.evaluate("S47.carRadio.on && S47.carRadio.live && S47.carRadio.station==='caliche'") and
                  [r["castId"] for r in cast_starts(first)] == [longest["id"]] and
                  all(art.mp3(r) and art.am_route(r) for r in cast_starts(first)),
                  "real R wakes the truck receiver and plays its exact caller MP3 through AM/radioBus/master")
            pcm = await art.sample_station(page)
            master = await sample_master(page)
            check(pcm["peakRms"] > 1e-4 and master["peakRms"] > 1e-5,
                  "the recorded caller produces real PCM at its station bus and master")
            before_count = len(first["starts"])
            await page.evaluate("""() => {
              const r=window.__extraRadio=S47.radio.receiver({vol:.15});
              r.pos=S47.carRadio.pos.clone(); r.set(true,'caliche');
            }""")
            await art.pump(page, .13)
            two = await snapshot(page)
            check(two["state"]["caliche"]["listeners"] == 2 and len(two["starts"]) == before_count and
                  exact_caption(two, longest["text"], cast_starts(first)[0]["duration"]),
                  "two receivers share one caller source and one exact caption")
            await page.evaluate("__extraRadio.reset(); S47.radio.receivers.splice(S47.radio.receivers.indexOf(__extraRadio),1)")
            report["real_R"] = {"audio": two, "station_pcm": pcm, "master_pcm": master}

            # Each starts at its real first sample, rather than only being present
            # in the preload cache or scheduled far into a long queue.
            report["all_callers"] = []
            for clip in callers:
                await fixture(page, [line(clip)])
                result = await loaded(page)
                starts = cast_starts(result)
                check(len(starts) == 1 and starts[0]["castId"] == clip["id"] and
                      starts[0]["at"] <= result["now"] and starts[0]["rate"] == 1 and
                      art.mp3(starts[0]) and art.am_route(starts[0]) and
                      exact_caption(result, clip["text"], starts[0]["duration"]),
                      f'{clip["voice"]} {clip["id"]}: exact MP3, natural rate, actual start and complete caption')
                report["all_callers"].append(result)

            # Prove a real mixed host/caller queue and full natural completion.
            await fixture(page, [{"who": "host", "text": host["text"]}, line(shortest)])
            mixed = await loaded(page)
            starts = mixed["starts"]
            check(len(starts) == 2 and starts[0].get("hostId") == host["id"] and
                  starts[1].get("castId") == shortest["id"] and all(art.mp3(r) and art.am_route(r) for r in starts) and
                  abs(starts[1]["at"] - starts[0]["expectedEnd"] - .7) < .025,
                  "one Station schedules Art Bell and a separate caller with decoded lengths and a .7s gap")
            await art.pump(page, until="__readingTrace.snapshot().filter(r=>r.hostId||r.castId).every(r=>r.endedAt!==null)")
            mixed = await snapshot(page)
            check(len(mixed["starts"]) == 2 and all(r["endedAt"] >= r["expectedEnd"] - .08 and
                  (r["stoppedAt"] is None or r["stoppedAt"] >= r["expectedEnd"] - .08) for r in mixed["starts"]) and
                  exact_caption(mixed, host["text"], mixed["starts"][0]["duration"], None) and
                  exact_caption(mixed, shortest["text"], mixed["starts"][1]["duration"]),
                  "both mixed recordings finish without truncation and receive exact full-duration captions")
            report["mixed_natural"] = mixed

            report["bulletin"] = await bulletin(page, news)

            # Canonical boy selector and its existing notebook/map bookmark.
            await page.evaluate("""() => {
              S47.nightEvents.reset(); S47.nightEvents.wait=1e9;
              for(const k of [...S47.townSeen]) if(k==='ev:boy'||k.startsWith('mark:boy')) S47.townSeen.delete(k);
              S47.game.clock=2*3600+26*60; S47.radio.boyCalled=false;
              // Reset normally joins up to 20s into a programme. Choose its
              // zero-offset branch so all three canonical boy lines are visible.
              const random=Math.random; Math.random=()=>0;
              try { S47.radio.reset(S47.game.clock); } finally { Math.random=random; }
              S47.carRadio.set(true,'caliche');
              __artProbe.reset(); S47.tick(.02,60);
            }""")
            boy = await loaded(page)
            check({r["castId"] for r in cast_starts(boy)} == {c["id"] for c in callers if c["voice"] == "boy"} and
                  await page.evaluate("S47.townSeen.has('ev:boy') && [...S47.townSeen].some(k=>k.startsWith('mark:boy@')) && S47.game.notes.some(n=>n.includes(\"From his father's ranch. He said it was July.\"))"),
                  "the actual 02:26 boy-call selector schedules all three boy MP3s and writes its notebook/map bookmark")
            report["boy_event"] = boy

            # Unknown role/text and a decoder rejection must keep every word in
            # procedural fallback, with no partial recording or stuck load.
            unknown = "This unknown caller fixture has no recorded radio cast clip."
            await fixture(page, [{"who": "woman", "text": unknown, "by": "a caller"}])
            await art.pump(page, .32)
            fallback = await snapshot(page)
            check(not cast_starts(fallback) and not fallback["pending"] and
                  any(x["text"] == unknown and x["duration"] is None for x in fallback["lines"]),
                  "unknown caller text retains its whole procedural line and caption")
            await page.evaluate("""() => {
              const bank=S47.radio.callerBank,ctx=S47.audio.ctx;
              bank.buffers.clear(); bank.bytes=0; window.__castDecode=ctx.decodeAudioData;
              ctx.decodeAudioData=()=>Promise.reject(new DOMException('Intentional caller decode failure','EncodingError'));
            }""")
            try:
                await fixture(page, [line(shortest)])
                failed = await loaded(page)
            finally:
                await page.evaluate("() => { S47.audio.ctx.decodeAudioData=__castDecode; }")
            check(not cast_starts(failed) and not failed["pending"] and
                  any(x["text"] == shortest["text"] and x["duration"] is None for x in failed["lines"]),
                  "real decoder rejection falls back to the complete caller without a partial recorded source")
            report["fallback"] = {"unknown": fallback, "decode_failure": failed}
            report["pending_cancellation"] = await cancellation(page, line(longest), saved, True)
            report["active_cancellation"] = await cancellation(page, line(longest), saved, False)
            check(await page.evaluate("S47.radio.callerBank.bufferCount<=24 && S47.radio.hostBank.bufferCount<=24"),
                  "both independent decoded voice caches stay within their 24-buffer bounds")
            await desktop.close()

            mobile = await browser.new_context(viewport={"width": 844, "height": 390}, is_mobile=True, has_touch=True, device_scale_factor=1)
            page = await new_page(mobile)
            await art.pause(page, touch=True)
            toggle = page.locator(".settings-panel [data-a=radiocast]")
            await toggle.scroll_into_view_if_needed()
            check(await toggle.get_attribute("aria-pressed") == "true" and await no_horizontal_overflow(page, ".settings-panel"),
                  "844x390 touch settings expose callers/news On without horizontal overflow")
            await page.screenshot(path=str(OUT / "c01_mobile_on.png"))
            await toggle.tap()
            check(await page.evaluate("!S47.radio.callerVoice && S47.radio.hostVoice && !S47.game.d.ui.readAloud.enabled && localStorage.getItem('s47.radioCallerVoices')==='false'"),
                  "real touch turns only callers/news Off and persists its separate setting")
            await page.locator("[data-a=kinski]").scroll_into_view_if_needed()
            await page.locator("[data-a=kinski]").tap()
            await page.locator("[data-a=artbell]").scroll_into_view_if_needed()
            await page.locator("[data-a=artbell]").tap()
            check(await page.evaluate("!S47.radio.callerVoice && !S47.radio.hostVoice && S47.game.d.ui.readAloud.enabled"),
                  "Art Bell and Kinski touch changes remain independent of caller/news Off")
            await page.screenshot(path=str(OUT / "c02_mobile_independent.png"))
            await art.resume(page, touch=True)
            await page.reload()
            await start_night(page)
            await art.pause(page, touch=True)
            check(await page.locator("[data-a=radiocast]").get_attribute("aria-pressed") == "false" and
                  await page.locator("[data-a=artbell]").get_attribute("aria-pressed") == "false" and
                  await page.locator("[data-a=kinski]").get_attribute("aria-pressed") == "true",
                  "reload preserves all three independent voice choices")
            await page.locator("[data-a=radiocast]").scroll_into_view_if_needed()
            await page.locator("[data-a=radiocast]").tap()
            await art.resume(page, touch=True)
            await page.evaluate(PROBE)
            await art.enter_truck(page)
            # startTrip resolves asynchronously. Its promise can set driving
            # just after enter_truck's final tick, before radioStep has shown the
            # touch control. Normal rAF would do that next; this held harness
            # must pump the same real audio/frame progression before tapping.
            try:
                await art.pump(page, until="S47.world.driving && !S47.game.cinematic && !!S47.world.truckHere() && document.querySelector('.touch .radio')?.hidden===false", timeout=15000)
            except Exception:
                report["mobile_radio_ready_failure"] = await page.evaluate("""() => ({
                  driving:S47.world.driving,truckHere:!!S47.world.truckHere(),
                  cinematic:S47.game.cinematic,modal:!!S47.game.d.ui.modal,
                  context:S47.audio.ctx.state,radioHidden:document.querySelector('.touch .radio')?.hidden
                })""")
                print("MOBILE RADIO READY " + json.dumps(report["mobile_radio_ready_failure"]), flush=True)
                raise
            await page.evaluate("""line => {
              S47.game.clock=2*3600+40*60; S47.radio.boyCalled=true;
              S47.radio.stations.caliche.start([{kind:'talk',lines:[line]},{kind:'dead'}],S47.game.clock);
              __artProbe.reset();
            }""", line(longest))
            await page.locator(".touch .radio").tap()
            await page.evaluate("S47.tick(.02,60)")
            mobile_audio = await loaded(page)
            check(await page.evaluate("S47.carRadio.on && S47.carRadio.station==='caliche' && S47.radio.callerVoice && !S47.radio.hostVoice && S47.game.d.ui.readAloud.enabled") and
                  [r["castId"] for r in cast_starts(mobile_audio)] == [longest["id"]] and
                  all(art.mp3(r) and art.am_route(r) for r in cast_starts(mobile_audio)),
                  "the real touch Radio handler starts caller MP3 playback with Art Bell Off and Kinski On")
            report["mobile_touch_radio"] = mobile_audio
            await mobile.close()
            check(not bad, f"caller/host MP3 requests have no HTTP failures ({len(requests)} requests)")
            if not PAGES:
                check(not all_mp3, "strict-CSP single-file playback uses inline bytes with no external MP3 requests")
            noise = [e for e in errors if not any(s in e for s in ("GPU stall", "Automatic fallback to software WebGL", "favicon"))]
            check(not noise, "no browser errors or warnings" + (": " + " | ".join(noise[:4]) if noise else ""))
            report.update({"requests": requests, "all_external_mp3_requests": all_mp3, "bad_responses": bad, "errors": noise})
            await browser.close()
    except Exception as error:
        report["fatal"] = repr(error)
        raise
    finally:
        if csp_dir:
            csp_dir.cleanup()
        report["checks"] = CHECKS
        report["limits"] = "Real decoding, source start/end, PCM and graph evidence; no physical-speaker, pronunciation or perceived voice-similarity claim. Individual caller fixtures are intentionally interrupted after their actual start."
        (OUT / "radiocast-radio.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    passed = sum(c["pass"] for c in CHECKS)
    print(f"{passed} of {len(CHECKS)} PASS", flush=True)
    return 0 if passed == len(CHECKS) else 1


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
