"""Description coverage through real game UI and genuinely decoded narration.

Run from web/: python3 tools/descriptions.py OUTDIR
S47_URL selects the Pages build under its deployment subdirectory. Otherwise the
single HTML is copied to a clean directory with the existing strict CSP. Analyser
samples prove Web Audio output, not perceived likeness or physical speakers.
"""
import asyncio
import json
import os
import sys
import tempfile
from pathlib import Path

from playwright.async_api import async_playwright

from readaloud import (CSP, INSTRUMENT, READER, no_horizontal_overflow, normalized,
                       quiet_background, sample_master, start_night, wait_for_js)


OUT = Path(sys.argv[1] if len(sys.argv) > 1 else "shots/descriptions")
PAGES = bool(os.environ.get("S47_URL"))
URL = os.environ.get("S47_URL") or Path("dist-single/index.html").resolve().as_uri()
CHECKS = []
HUD = "button[data-a=read-description]"
# Spoken-number clips can be shorter than half a second. Keep every real MP3
# decode in the inherited probe, while synthesized noise buffers stay filtered.
DESCRIPTION_INSTRUMENT = INSTRUMENT.replace(
    "this.buffer.duration > 0.5", "(this.buffer.duration > 0.5 || decoded.get(this.buffer))")

PROBE = r"""() => {
  const reader = S47.game.d.ui.readAloud;
  if (window.__descriptionTrace) return;
  const trace = window.__descriptionTrace = {requests: [], toasts: []};
  const play = reader.play.bind(reader), toast = S47.game.d.ui.toast.bind(S47.game.d.ui);
  reader.play = (texts, callback) => {
    trace.requests.push({texts: [...texts], segments: reader.segments(texts), at: S47.audio.ctx.currentTime});
    return play(texts, callback);
  };
  S47.game.d.ui.toast = (text, ...rest) => {
    trace.toasts.push(text); return toast(text, ...rest);
  };
  trace.reset = () => { trace.requests.length=0; trace.toasts.length=0; __readingTrace.reset(); };
  trace.snapshot = () => ({requests: trace.requests, toasts: trace.toasts,
    sources: __readingTrace.snapshot().filter(r => r.narration)});
}"""

# Keep the existing real MP3 decode probe; tag only the source actually owned by
# ReadAloud so an unrelated door, telephone or wind sound cannot pass this check.
TAG_READING = r"""(() => {
  const start = AudioBufferSourceNode.prototype.start;
  AudioBufferSourceNode.prototype.start = function (...args) {
    const result = start.apply(this, args);
    const record = __readingTrace.starts.findLast(r => r.source === this);
    if (record) record.narration = window.S47?.game?.d?.ui?.readAloud?.source === this;
    return result;
  };
})();"""

INVENTORY = r"""() => {
  const g = S47.game, reader = g.d.ui.readAloud, inter = g.d.inter;
  const entries = [];
  const add = (source, text) => {
    if (text && text.trim()) entries.push({source, text, available: reader.available([text])});
  };
  for (const [id, look] of Object.entries(inter.looks)) add('looks:' + id, look.say());
  for (const item of inter.items) if (item.examine) add('examine:' + item.id, item.examine());
  const extra = g.notebookExtras();
  for (const [i, text] of (extra.mapDescription || []).entries()) add('map:' + i, text);
  for (const item of extra.kit || []) {
    add('kit:' + item.name + ':name', item.name); add('kit:' + item.name + ':note', item.note);
    add('kit:' + item.name + ':state', item.state);
  }
  for (const p of extra.people || []) {
    add('person:' + p.name + ':name', p.name); add('person:' + p.name + ':role', p.role); add('person:' + p.name + ':note', p.note);
  }
  for (const text of g.notes) add('note', text);
  for (const task of g.tasks()) add('task', task.text);
  return entries;
}"""


def check(ok, message):
    CHECKS.append({"pass": bool(ok), "message": message})
    print(("PASS " if ok else "FAIL ") + message, flush=True)


def box(kind):
    return f'.read-aloud[data-reading-kind="{kind}"]'


def genuine_mp3(record):
    head = (record.get("decoded") or {}).get("head", [])
    return record.get("master") and len(head) >= 3 and (
        head[:3] == [73, 68, 51] or head[0] == 255 and head[1] & 224 == 224)


async def texts(page, kind):
    return await page.locator(box(kind)).evaluate("el => JSON.parse(el.dataset.readingTexts)")


async def hud_clear(page):
    return await page.evaluate("""() => {
      const r=document.querySelector('[data-a=read-description]').getBoundingClientRect();
      return ['.clockline','.objective','.touch .notes','.touch .pause','.touch .camera','.touch .light','.touch .zoom']
        .map(s=>document.querySelector(s)).filter(e=>e && e.getClientRects().length && !e.closest('[hidden]'))
        .every(e=>{const q=e.getBoundingClientRect();return !(Math.max(r.left,q.left)<Math.min(r.right,q.right) && Math.max(r.top,q.top)<Math.min(r.bottom,q.bottom));});
    }""")


async def ready(page, kind):
    await page.wait_for_selector(box(kind))
    await wait_for_js(page, f"!!document.querySelector({json.dumps(box(kind))})?.dataset.readingTexts")
    content = await texts(page, kind)
    enabled = not await page.locator(box(kind) + " [data-a=narrate]").is_disabled()
    check(bool(content) and enabled, f"{kind} has complete recording coverage for its current visible descriptions")
    if not enabled:
        missing = await page.evaluate("texts=>texts.filter(t=>!S47.game.d.ui.readAloud.available([t]))", content)
        print("MISSING " + json.dumps(missing, ensure_ascii=False), flush=True)
        await page.screenshot(path=str(OUT / ("missing-" + kind + ".png")))
        raise AssertionError("Missing recorded description: " + repr(missing))
    return content


async def play_box(page, kind):
    content = await ready(page, kind)
    await page.evaluate("__descriptionTrace.reset()")
    await page.locator(box(kind) + " [data-a=narrate]").click()
    await wait_for_js(page, f"{READER}.playing")
    await wait_for_js(page, "__descriptionTrace.snapshot().sources.length > 0")
    probe = await page.evaluate("__descriptionTrace.snapshot()")
    check(probe["requests"][-1]["texts"] == content,
          f"{kind} plays exactly the narrative text advertised by its real control")
    check(any(genuine_mp3(r) for r in probe["sources"]),
          f"{kind} starts genuinely decoded MP3 narration connected to master")
    return content, probe


async def stop_box(page, kind):
    await page.locator(box(kind) + " [data-a=narrate]").click()
    await wait_for_js(page, f"{READER}.state === 'idle'")


async def notebook(page, tab):
    if not await page.locator(".notebook").count():
        await page.evaluate("S47.game.openNotebook(); S47.tick(0.05)")
    await page.locator(f'.notebook [data-t="{tab}"]').click()
    await wait_for_js(page, f"document.querySelector('.nb-page.on')?.dataset.p === {json.dumps(tab)}")
    # The observer intentionally runs at a microtask boundary after tab mutation.
    await page.evaluate("new Promise(resolve => queueMicrotask(resolve))")


async def inventory(page, label, report):
    entries = await page.evaluate(INVENTORY)
    # The herd's ear tags (PR #132, the ninth cow) are new text without a recording; the game
    # shows them as text with the read-aloud button disabled ("No recording for this text.").
    unrecorded = [e for e in entries if not e["available"] and e["source"].startswith("examine:look:cow:")]
    missing = [e for e in entries if not e["available"] and e not in unrecorded]
    check(len(entries) >= 100 and not missing,
          f"{label}: all {len(entries)} sampled authored look/map/kit/people/task/note descriptions resolve" +
          (f" ({len(unrecorded)} cow tags unrecorded, by design)" if unrecorded else "") +
          (": " + " | ".join(e["source"] + "=" + e["text"] for e in missing[:4]) if missing else ""))
    report[label] = {"entries": entries, "missing": missing, "unrecorded": unrecorded}


async def enable(page):
    await page.keyboard.press("Escape")
    toggle = page.locator(".settings-panel [data-a=kinski]")
    await toggle.wait_for()
    if await toggle.get_attribute("aria-pressed") != "true":
        await toggle.click()
    await page.locator(".settings-panel [data-a=resume]").click()
    await wait_for_js(page, "S47.audio.ctx.state === 'running'")


async def main():
    global URL
    OUT.mkdir(parents=True, exist_ok=True)
    clips = json.loads(Path("src/assets/audio/kinski/catalog.json").read_text())["clips"]
    if len(clips) <= 337:
        raise AssertionError("Generate and build the expanded description corpus before this browser check")
    csp_dir = None
    if not PAGES:
        csp_dir = tempfile.TemporaryDirectory(prefix="s47-descriptions-csp-")
        target = Path(csp_dir.name) / "index.html"
        target.write_text(Path("dist-single/index.html").read_text().replace(
            "<head>", '<head><meta http-equiv="Content-Security-Policy" '
            f'content="{CSP}">', 1))
        URL = target.as_uri()
    ids = {c["id"] for c in clips}
    errors, requests, bad_responses = [], [], []
    report = {"url": URL, "pages": PAGES, "strict_csp": not PAGES, "catalogue_clips": len(clips)}

    def reading_url(url):
        name = url.split("?", 1)[0].rsplit("/", 1)[-1]
        return name.endswith(".mp3") and name.split("-", 1)[0].removesuffix(".mp3") in ids

    try:
        async with async_playwright() as playwright:
            browser = await playwright.chromium.launch(args=[
                "--use-angle=swiftshader", "--enable-unsafe-swiftshader", "--ignore-gpu-blocklist",
            ])
            context = await browser.new_context(viewport={"width": 1280, "height": 800})
            async def new_page(ctx):
                pg = await ctx.new_page()
                pg.set_default_timeout(60000)
                # Legacy timing/voice baseline; new defaults are exercised in ensemble.py/serling.py.
                await pg.add_init_script("try{localStorage.setItem('s47.serlingNarrator','false');localStorage.setItem('s47.characterVoices','false');}catch{}")
                await pg.add_init_script(DESCRIPTION_INSTRUMENT + TAG_READING)
                pg.on("pageerror", lambda e: errors.append("PAGEERROR: " + str(e)))
                pg.on("console", lambda m: errors.append(m.type + ": " + m.text)
                      if m.type in ("error", "warning") else None)
                pg.on("request", lambda r: requests.append(r.url) if reading_url(r.url) else None)
                pg.on("response", lambda r: bad_responses.append((r.status, r.url))
                      if reading_url(r.url) and r.status >= 400 else None)
                return pg

            page = await new_page(context)
            await page.goto(URL)
            await wait_for_js(page, "!!window.S47")
            # Await complete ordinary, telephone, character and optional licensed
            # preloads; a partial total cannot prove any individual bank ready.
            await page.evaluate("() => S47.audio.preload()")
            await page.evaluate("localStorage.clear()")
            await page.reload()
            await start_night(page)
            await page.evaluate(PROBE)
            check(await page.evaluate(f"!{READER}.enabled"), "descriptions still default to Off on a fresh device")
            await page.evaluate("S47.game.openNotebook(); S47.tick(0.05)")
            check(await page.locator(box("notebook-description")).count() == 0,
                  "the default notebook has no narration control until Kinski is enabled")
            await page.locator(".notebook .close").click()
            await enable(page)
            await quiet_background(page)
            await inventory(page, "initial_inventory", report)

            # This is the real phone's quiet-state use handler, not a made-up toast.
            await page.evaluate("__descriptionTrace.reset(); S47.game.d.ui.clearToasts(); S47.game.d.inter.get('phone').use()")
            environmental = await page.evaluate("__descriptionTrace.toasts.at(-1)")
            await page.wait_for_selector(HUD + ":not([hidden])")
            await asyncio.sleep(0.8)
            check(await page.evaluate("__descriptionTrace.requests.length") == 0,
                  "an ordinary environmental toast does not autoplay or interrupt the night")
            await page.keyboard.press("v")
            await wait_for_js(page, f"{READER}.playing")
            hud = await page.evaluate("__descriptionTrace.snapshot()")
            check(hud["requests"][-1]["texts"] == [environmental] and any(genuine_mp3(r) for r in hud["sources"]),
                  "V reads the actual last environmental description using a decoded MP3")
            output = await sample_master(page, 350)
            check(output["peakRms"] > 0.00001, "description narration produces nonzero samples at master")
            await page.keyboard.press("v")
            await wait_for_js(page, f"{READER}.state === 'idle'")
            await page.evaluate("""() => {
              const ui=S47.game.d.ui;
              ui.toast('RADIO: "The lights are back over the mesa."', 0.1);
              ui.toast('CB, EARL: "Anybody out there?"', 0.1);
              ui.toast('WARD: "Check the reference record."', 0.1);
              ui.toast('N. VEGA: "Sierra Motor Court."', 0.1);
              ui.toast('THE LINE: "Reference west. No. East. Hold the last reading."', 0.1);
            }""")
            await page.locator(HUD).click()
            await wait_for_js(page, f"{READER}.playing")
            check((await page.evaluate("__descriptionTrace.snapshot()"))["requests"][-1]["texts"] == [environmental],
                  "radio, CB and actual telephone speaker captions preserve the last environmental description")
            await page.locator(HUD).click()

            await page.locator(HUD).click()
            await wait_for_js(page, f"{READER}.playing")
            before_phone = await page.evaluate("__descriptionTrace.requests.length")
            await page.evaluate("""() => {
              S47.audio.phoneCall([{by:'WARD',text:'That is enough to hold the morning series. It is not enough to tell us why it happened.'}]);
              S47.game.d.ui.examine(S47.game.d.inter.looks.phone.say());
            }""")
            await wait_for_js(page, f"{READER}.state === 'idle'")
            await asyncio.sleep(0.8)
            check(await page.evaluate("S47.audio.phonePlaying") and
                  await page.evaluate("__descriptionTrace.requests.length") == before_phone,
                  "a real telephone queue stops narration and blocks automatic Zoom descriptions")
            await page.evaluate("S47.audio.stopPhoneCalls(); S47.game.d.ui.examine(null)")

            await page.evaluate("S47.game.d.ui.toast('Internal line and one outside line. Nobody calls out here after midnight.')")
            await page.locator(HUD).click()
            await wait_for_js(page, f"{READER}.playing")
            await page.evaluate("S47.game.cinematic = true; S47.tick(0.05)")
            check(await page.evaluate(f"{READER}.state === 'idle'") and not await page.locator(HUD).is_visible(),
                  "a cinematic interrupts manual HUD narration even when no Zoom description was active")
            await page.evaluate("S47.game.cinematic = false; S47.tick(0.05)")

            # Exercise the actual desktop Zoom input and main's raycast-to-examine
            # path. Only native pointer lock is stubbed, as in the existing suite.
            await page.evaluate("""() => {
              const it=S47.game.d.inter.get('look:labSign'), p=it.object.getWorldPosition(new S47.three.Vector3());
              const x=p.x-2.2, z=p.z;
              S47.player.place(x,z,Math.atan2(-(p.x-x),-(p.z-z)));
              S47.player.pitch=Math.atan2(p.y-1.62,Math.hypot(p.x-x,p.z-z));
              S47.input.locked=true; __descriptionTrace.reset(); S47.tick(0.05);
            }""")
            await page.mouse.down(button="right")
            await page.evaluate("S47.tick(0.4)")
            examined = await page.locator(".hud .examine").inner_text()
            check(bool(examined) and await page.evaluate("t => S47.game.d.ui.readAloud.available([t])", examined),
                  "holding real desktop Zoom exposes a recordable raycast description")
            await wait_for_js(page, "__descriptionTrace.requests.length === 1")
            await wait_for_js(page, f"{READER}.playing")
            await page.screenshot(path=str(OUT / 'd03_examine_desktop.png'))
            check(await hud_clear(page), 'the desktop description button leaves the clock and other HUD controls clear')
            check((await page.evaluate("__descriptionTrace.snapshot()"))["requests"][0]["texts"] == [examined],
                  "650 ms of stable Zoom automatically reads the exact examined object")
            await page.evaluate("S47.tick(0.3)")
            await asyncio.sleep(0.75)
            check(await page.evaluate("__descriptionTrace.requests.length") == 1,
                  "repeated frame updates on the same object do not restart automatic narration")
            await page.mouse.up(button="right")
            await page.evaluate("S47.tick(0.3); S47.input.locked=false")
            check(await page.evaluate(f"{READER}.state === 'idle'"), "releasing Zoom clears and stops its automatic description")

            await page.evaluate("__descriptionTrace.reset(); S47.game.d.inter.get('clock').use()")
            await page.locator(HUD).click()
            await wait_for_js(page, "__descriptionTrace.requests.length > 0")
            clock_request = (await page.evaluate("__descriptionTrace.snapshot()"))["requests"][-1]
            check(clock_request["texts"][0].startswith("23:41.") and
                  clock_request["segments"][:4] == ["twenty", "three", "forty", "one"],
                  "the actual wall clock description retains 23:41 as four complete spoken-number segments")
            await wait_for_js(page, f"__descriptionTrace.snapshot().sources.length >= {len(clock_request['segments'])}", timeout=20000)
            clock_audio = await page.evaluate("__descriptionTrace.snapshot()")
            check(len(clock_audio["sources"]) == len(clock_request["segments"]) and
                  all(genuine_mp3(r) for r in clock_audio["sources"]) and all(
                      b["at"] >= a["expectedEnd"] - 0.05
                      for a, b in zip(clock_audio["sources"], clock_audio["sources"][1:])),
                  "every clock-number segment starts a real decoded MP3 in order without overlap")
            await page.locator(HUD).click()
            report["clock_description"] = clock_audio

            # One real receiver panel, refreshed through its five authored card
            # states. These fixtures do not claim to solve the calibration puzzle.
            await page.evaluate("S47.game.d.inter.get('rack').use(); S47.game.d.inter.get('crtCenter').use()")
            receiver_cards = [
                (0, False, "TAPED REF CARD / CAL 1419.900 MHz / AZ 042 / GAIN 55 +-10 / BW 48 +-14"),
                (1, False, "LOCAL SPIKE / centre carrier near 1420.110 MHz. Narrow BW below 22 kHz while keeping nominal gain."),
                (2, True, "UNLOGGED RESIDUAL / weak peak near 1420.4 MHz. Raise gain, narrow bandwidth, then search array azimuth by hand."),
                (2, False, "Survey runs unattended. Nothing scheduled until 06:00."),
                (3, True, "Pattern repeats every 5.2 s: four pulses, a gap, seven pulses. Run a direction solve on the tracking terminal."),
            ]
            receiver_results = []
            for stage, visible, expected in receiver_cards:
                await page.evaluate("""({stage,visible}) => {
                  S47.game.rx.stage=stage; S47.game.rxc.residualVisible=visible; S47.game.rxc.sync(true);
                }""", {"stage": stage, "visible": visible})
                await wait_for_js(page, f"{READER}.state === 'idle'")
                content, receiver_probe = await play_box(page, "receiver-description")
                check(content == [expected] and normalized(await page.locator(".crt .refcard").inner_text()) == expected,
                      f"receiver stage {stage}, residual={visible}: the exact current reference card is read")
                receiver_results.append({"stage": stage, "residual_visible": visible, "audio": receiver_probe})
            await page.screenshot(path=str(OUT / 'd04_receiver_desktop.png'))
            await page.locator(".crt [data-a=exit]").click()
            check(await page.evaluate(f"{READER}.state === 'idle'"), "closing the receiver stops its reference-card narration")
            report["receiver_cards"] = receiver_results

            # The work order creates a genuine notebook observation when put back.
            await page.evaluate("S47.game.d.inter.get('workOrder').use()")
            await page.locator(".docview [data-a=back]").click()
            await notebook(page, "tasks")
            task_texts = await ready(page, "notebook-description")
            check(task_texts == await page.evaluate("S47.game.tasks().map(t=>t.text)"),
                  "Tasks reads authored task text without the x/dash status markers")
            await play_box(page, "notebook-description")
            await page.locator(".notebook [data-t=notes]").click()
            await wait_for_js(page, f"{READER}.state === 'idle'")
            note_texts = await ready(page, "notebook-description")
            check(note_texts == await page.evaluate("S47.game.notes"), "Notes reads actual observations and stops the old tab's queue")
            await notebook(page, "kit")
            kit_texts, _ = await play_box(page, "notebook-description")
            check("Notebook" in kit_texts and any("Tab opens it." in t for t in kit_texts),
                  "Kit exposes item names and narrative notes")
            await notebook(page, "people")
            people = await ready(page, "notebook-description")
            check("Dale" in people and any("Left the shift log" in t for t in people),
                  "People exposes the actual name, role and character description")
            await notebook(page, "map")
            early_map = await ready(page, "notebook-description")
            check(early_map == await page.evaluate("S47.game.notebookExtras().mapDescription") and
                  "STATION 01. 1947." not in early_map and "The old road is closed." in early_map,
                  "the map describes only its current known places and closed road")
            await play_box(page, "notebook-description")
            await page.screenshot(path=str(OUT / "d01_map_desktop.png"))
            await page.locator(".notebook .close").click()
            check(await page.evaluate(f"{READER}.state === 'idle'"), "closing the notebook stops description playback")

            # Real field-lamp code supplies both the instructions and changing hint.
            await page.evaluate("__descriptionTrace.reset(); S47.ch3['openLamp']()")
            initial_panel, _ = await play_box(page, "panel-description")
            await page.locator(".labpanel [data-a=hint]").click()
            await wait_for_js(page, f"{READER}.state === 'idle'")
            changed = await ready(page, "panel-description")
            hint = await page.locator(".labpanel [data-say]").inner_text()
            check(bool(hint) and hint in changed and changed != initial_panel,
                  "a real puzzle Hint updates the readable description and cancels the former queue")
            check(await page.evaluate("__descriptionTrace.requests.length") == 1,
                  "changing a puzzle description does not start another reading without a gesture")
            await page.evaluate("""() => {
              const el=document.createElement('p'); el.className='lp-text'; el.hidden=true;
              el.textContent='UNRECORDED HIDDEN TEST WORDS'; document.querySelector('.labpanel').append(el);
            }""")
            await page.evaluate("new Promise(resolve=>queueMicrotask(resolve))")
            check(await texts(page, "panel-description") == changed, "hidden panel prose and playback button labels are excluded from narration")
            await play_box(page, "panel-description")
            await page.evaluate("document.querySelector('.labpanel [data-say]').textContent='Unrecorded browser test phrase quuxxyz.'")
            await wait_for_js(page, f"{READER}.state === 'idle'")
            await wait_for_js(page, f"document.querySelector({json.dumps(box('panel-description') + ' [data-a=narrate]')})?.disabled")
            check("No recording" in await page.locator(box("panel-description") + " [data-reading-status]").inner_text(),
                  "an unsupported changed description disables playback without silently dropping its unknown words")
            await page.locator(".labpanel [data-a=hint]").click()
            await ready(page, "panel-description")
            await play_box(page, "panel-description")
            await page.locator(".labpanel [data-a=close]").click()
            check(await page.evaluate(f"{READER}.state === 'idle'"), "closing a puzzle cancels its current description and remaining segments")

            # Later chapter fixtures supply the board, expanded people and known map.
            await page.evaluate("S47.jump('chapter5'); S47.tick(1)")
            await page.evaluate("['attendant','deputy','host','radio_woman','souvenir_man'].forEach(id=>S47.townSeen.add('met:'+id))")
            await inventory(page, "later_inventory", report)
            await notebook(page, "people")
            town_people = await ready(page, "notebook-description")
            check(all(name in town_people for name in ["The attendant", "The deputy", "Earl", "The woman on the bench", "The souvenir man"]),
                  "all five met Caliche NPC names, roles and notes join the readable People tab")
            await notebook(page, "map")
            later_map = await ready(page, "notebook-description")
            check("STATION 01. 1947." in later_map and "The old road to Roswell." in later_map,
                  "the later map describes STATION 01 and the known old road")
            await page.locator(".notebook .close").click()

            await page.evaluate("S47.ch5.s.p13=false; S47.ch5.s.tracing=[300,690]; S47.ch5['openMap']()")
            map_texts, _ = await play_box(page, "panel-description")
            check("Drag the tracing until the transit lies on STATION 01." in map_texts,
                  "the service-map overlay includes its actual unaligned drag description")
            canvas = page.locator(".mapov canvas")
            await canvas.scroll_into_view_if_needed()
            rect = await canvas.bounding_box()
            scale = rect["width"] / 1200
            sx, sy = rect["x"] + 330 * scale, rect["y"] + 690 * scale
            ex, ey = rect["x"] + 504 * scale, rect["y"] + 363 * scale
            await page.mouse.move(sx, sy)
            await page.mouse.down()
            for n in range(1, 13):
                await page.mouse.move(sx + (ex - sx) * n / 12, sy + (ey - sy) * n / 12)
            await page.mouse.up()
            await wait_for_js(page, f"{READER}.state === 'idle'")
            aligned_texts = await ready(page, "panel-description")
            aligned = "The tracing's transit sits on STATION 01, north to north. C runs east across the map."
            check(await page.evaluate("JSON.stringify(S47.ch5.s.tracing) === '[470,360]'") and aligned in aligned_texts,
                  "a real tracing drag snaps to STATION 01 and replaces narration with the aligned map description")
            await play_box(page, "panel-description")
            await page.locator(".labpanel [data-a=close]").click()
            check(await page.evaluate(f"{READER}.state === 'idle'"), "closing the map overlay cancels its updated description")
            await page.evaluate("S47.ch5.s.arrived=true; S47.ch5.s.stage='diner'; S47.game.openBoard()")
            board_texts = await ready(page, "board-description")
            summaries = await page.locator(".bcard:not(.open) .bcard-sum[data-reading-text]").evaluate_all(
                "nodes=>nodes.map(n=>({visible:n.textContent,reading:n.dataset.readingText}))")
            check(bool(summaries) and all(normalized(s["reading"]) in board_texts for s in summaries),
                  "board excerpts use their complete authored source paragraphs rather than arbitrary truncated suffixes")
            await play_box(page, "board-description")
            await page.screenshot(path=str(OUT / 'd05_board_desktop.png'))
            more = page.locator(".bcard [data-more]").first
            await more.evaluate("button=>button.click()")
            await wait_for_js(page, f"{READER}.state === 'idle'")
            expanded = await ready(page, "board-description")
            full = await page.locator(".bcard.open .bcard-full").first.inner_text()
            check(expanded != board_texts and any(normalized(t) in normalized(full) for t in expanded),
                  "opening actual board TEXT replaces the excerpt with readable full source text")
            hint_button = page.locator(".board-wrap [data-hint]").first
            if await hint_button.count():
                await hint_button.evaluate("button=>button.click()")
                await page.evaluate("new Promise(resolve=>queueMicrotask(resolve))")
                board_hint = await page.locator(".board-say").inner_text()
                check(bool(board_hint) and board_hint in await texts(page, "board-description"),
                      "the board's actual Hint becomes part of its readable current description")
            await page.locator(".board-wrap [data-a=close]").click()

            # Use real camera exposures, retaining their actual generated captions.
            await page.evaluate("S47.jump('chapter1'); S47.tick(0.4); S47.game.d.inter.get('fieldCamera').use(); S47.tick(0.2)")
            await page.evaluate("""() => {
              const p=S47.game.d.inter.get('phone').object.getWorldPosition(new S47.three.Vector3());
              const pl=S47.player; pl.place(p.x,p.z+1.5,0); S47.tick(0.1);
              const c=S47.camera.position;
              pl.place(p.x,p.z+1.5,Math.atan2(-(p.x-c.x),-(p.z-c.z)));
              pl.pitch=Math.atan2(p.y-c.y,Math.hypot(p.x-c.x,p.z-c.z)); S47.tick(0.1);
            }""")
            await page.keyboard.press("c")
            await page.evaluate("S47.tick(0.2)")
            await page.keyboard.press("Space")
            await page.evaluate("S47.tick(0.3)")
            await page.keyboard.press("Space")
            await page.evaluate("S47.tick(0.2)")
            await page.keyboard.press("c")
            await page.evaluate("S47.tick(0.2)")
            check(await page.evaluate("S47.album.shots.length >= 2"), "the real field camera supplies two free photographs for caption testing")
            await notebook(page, "photos")
            await page.locator(".nb-album [data-f]").first.click()
            photo_texts, _ = await play_box(page, "photo-description")
            actual_caption = await page.locator(".al-view .al-cap b").inner_text()
            actual_meta = await page.locator(".al-view .al-cap small").inner_text()
            report['photo_description'] = {'texts': photo_texts, 'caption': actual_caption, 'meta': actual_meta}
            await page.screenshot(path=str(OUT / 'd06_photo_desktop.png'))
            check(normalized(actual_caption) in photo_texts and normalized(actual_meta) in photo_texts,
                  "a free photo reads its actual generated caption and place/time metadata")
            photo_button = page.locator(box('photo-description') + ' [data-a=narrate]')
            await photo_button.focus()
            await page.keyboard.press('Enter')
            await wait_for_js(page, f"{READER}.state === 'idle'")
            await page.keyboard.press('Space')
            await wait_for_js(page, f"{READER}.playing")
            check(await page.locator('.al-view').count() == 1, 'Enter and Space activate the focused photo reading button without closing the photograph')
            await page.locator(".al-view [data-v=next]").click()
            await wait_for_js(page, f"{READER}.state === 'idle'")
            await ready(page, "photo-description")
            check(await page.locator(box("photo-description")).count() == 1,
                  "Next photo cancels the old reading and rebinds exactly one caption control")
            await play_box(page, "photo-description")
            await page.locator(".al-view [data-v=back]").click()
            check(await page.evaluate(f"{READER}.state === 'idle'"), "Back to album stops the nested photo reading without closing the notebook")
            await page.locator(".notebook .close").click()

            # Close the desktop context before opening the touch page, keeping
            # this suite sequential even while the shared machine has other work.
            await context.close()
            mobile_context = await browser.new_context(viewport={"width": 844, "height": 390},
                                                       is_mobile=True, has_touch=True, device_scale_factor=1)
            mobile = await new_page(mobile_context)
            await mobile.goto(URL)
            await mobile.locator("button[data-a=set]").tap()
            await mobile.locator("button[data-a=kinski]").tap()
            await mobile.locator("[data-a=resume]").tap()
            await start_night(mobile)
            await mobile.evaluate(PROBE)
            await quiet_background(mobile)
            await mobile.evaluate("S47.game.d.inter.get('phone').use()")
            await mobile.wait_for_selector(HUD + ':not([hidden])')
            check(await hud_clear(mobile), 'the touch description button leaves the clock, Notes and other touch controls clear')
            await mobile.locator(HUD).tap()
            await wait_for_js(mobile, f'{READER}.playing')
            await mobile.screenshot(path=str(OUT / 'd07_hud_mobile.png'))
            check(any(genuine_mp3(r) for r in (await mobile.evaluate('__descriptionTrace.snapshot()'))['sources']),
                  'a real touch on the HUD description button starts its decoded recording')
            await mobile.locator(HUD).tap()
            await notebook(mobile, "kit")
            await ready(mobile, "notebook-description")
            read_button = mobile.locator(box("notebook-description") + " [data-a=narrate]")
            await read_button.scroll_into_view_if_needed()
            await mobile.screenshot(path=str(OUT / "d02_kit_mobile.png"))
            check(await no_horizontal_overflow(mobile, ".notebook"),
                  "the 844×390 touch notebook description control has no horizontal overflow")
            await mobile.evaluate("__descriptionTrace.reset()")
            await read_button.tap()
            await wait_for_js(mobile, f"{READER}.playing")
            await wait_for_js(mobile, "__descriptionTrace.snapshot().sources.length > 0")
            check(any(genuine_mp3(r) for r in (await mobile.evaluate("__descriptionTrace.snapshot()"))["sources"]),
                  "a real mobile tap starts decoded notebook description audio")
            await read_button.tap()
            await mobile.locator(".notebook .close").tap()
            check(await mobile.evaluate(f"{READER}.state === 'idle'"), "mobile Stop and close leave no narration playing")
            await mobile_context.close()
            check(not bad_responses, f"description MP3 requests have no HTTP failures ({len(requests)} requests)")
            if not PAGES:
                check(not requests, "strict-CSP single-file descriptions require no external MP3 requests")
            ignored = ("GPU stall", "Automatic fallback to software WebGL", "favicon")
            noise = [e for e in errors if not any(term in e for term in ignored)]
            check(not noise, "no browser errors or warnings" + (": " + " | ".join(noise[:4]) if noise else ""))
            report.update({"requests": requests, "bad_responses": bad_responses, "errors": noise,
                           "master_samples": output, "early_map": early_map, "later_map": later_map})
            await browser.close()
    finally:
        if csp_dir:
            csp_dir.cleanup()
        report["checks"] = CHECKS
        report["physical_speakers"] = "UNVERIFIED: analyser samples are not a hardware listening test"
        (OUT / "descriptions.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    passed = sum(c["pass"] for c in CHECKS)
    print(f"{passed} of {len(CHECKS)} PASS", flush=True)
    return 0 if passed == len(CHECKS) else 1


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
