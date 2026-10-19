"""Optional prerecorded Kinski reading, through real UI and decoded MP3 audio.

Run from web/: python3 tools/readaloud.py OUTDIR
Use S47_URL=http://127.0.0.1:PORT/SIGNAL-47/ for the ordinary Pages build.
Without S47_URL the self-contained build is loaded under the existing strict CSP.
The analyser observes Web Audio samples, not physical speakers or perceived quality.
"""
import asyncio
import json
import os
import re
import sys
import tempfile
from pathlib import Path

from playwright.async_api import async_playwright


OUT = Path(sys.argv[1] if len(sys.argv) > 1 else "shots/readaloud")
PAGES = bool(os.environ.get("S47_URL"))
URL = os.environ.get("S47_URL") or Path("dist-single/index.html").resolve().as_uri()
CHECKS = []
CSP = ("default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; "
       "img-src data: blob:; media-src data: blob:; font-src https://fonts.gstatic.com")

# No sounds are faked: retain each real decode, start, natural end and explicit stop.
# The graph is observed only to verify that a decoded clip reaches AudioSys.master.
INSTRUMENT = r"""(() => {
  const edges = new WeakMap(), decoded = new WeakMap();
  const trace = window.__readingTrace = { starts: [], decodes: [] };
  const connect = AudioNode.prototype.connect;
  AudioNode.prototype.connect = function (destination, ...rest) {
    const result = connect.call(this, destination, ...rest);
    if (destination instanceof AudioNode) {
      let targets = edges.get(this); if (!targets) edges.set(this, targets = new Set());
      targets.add(destination);
    }
    return result;
  };
  function reaches(node, target, seen = new Set()) {
    if (node === target) return true;
    if (!node || seen.has(node)) return false;
    seen.add(node);
    return [...(edges.get(node) || [])].some(next => reaches(next, target, seen));
  }
  const decode = BaseAudioContext.prototype.decodeAudioData;
  BaseAudioContext.prototype.decodeAudioData = function (data, ...rest) {
    const bytes = data.byteLength;
    const head = [...new Uint8Array(data, 0, Math.min(12, bytes))];
    const result = decode.call(this, data, ...rest);
    if (result && typeof result.then === 'function') return result.then(buffer => {
      const item = { bytes, head, duration: buffer.duration, rate: buffer.sampleRate,
        channels: buffer.numberOfChannels };
      decoded.set(buffer, item); trace.decodes.push(item); return buffer;
    });
    return result;
  };
  const start = AudioBufferSourceNode.prototype.start;
  AudioBufferSourceNode.prototype.start = function (...args) {
    if (this.buffer && !this.loop && this.buffer.duration > 0.5) {
      const now = this.context.currentTime, when = args[0] || now;
      const offset = args[1] || 0, rate = this.playbackRate.value;
      const record = { source: this, at: Math.max(now, when), duration: this.buffer.duration,
        expectedEnd: Math.max(now, when) + (this.buffer.duration - offset) / rate,
        decoded: decoded.get(this.buffer) || null, endedAt: null, stoppedAt: null,
        master: reaches(this, window.S47?.audio?.master) };
      trace.starts.push(record);
      this.addEventListener('ended', () => { record.endedAt = this.context.currentTime; }, { once: true });
    }
    return start.apply(this, args);
  };
  const stop = AudioBufferSourceNode.prototype.stop;
  AudioBufferSourceNode.prototype.stop = function (...args) {
    const record = trace.starts.findLast(item => item.source === this);
    if (record) record.stoppedAt = args[0] || this.context.currentTime;
    return stop.apply(this, args);
  };
  trace.reset = () => { trace.starts.length = 0; trace.decodes.length = 0; };
  trace.snapshot = () => trace.starts.map(({ source, ...item }) => item);
  HTMLElement.prototype.requestPointerLock = function () { return Promise.resolve(); };
})();"""

READER = "S47.game.d.ui.readAloud"
STATE = f"({{enabled:{READER}.enabled,state:{READER}.state,playing:{READER}.playing}})"
SNAPSHOT = "window.__readingTrace.snapshot()"
DESCRIPTION = ".docview .read-aloud[data-reading-kind=description]"
DOCUMENT = ".docview .read-aloud[data-reading-kind=document]"


def check(ok, message):
    CHECKS.append({"pass": bool(ok), "message": message})
    print(("PASS " if ok else "FAIL ") + message, flush=True)


def normalized(text):
    return " ".join(text.split())


async def wait_for_js(page, expression, timeout=60000):
    """Poll through the browser protocol without Playwright's string evaluator.

    wait_for_function constructs a Function inside the page, which requires
    unsafe-eval. Direct evaluate is also used by the existing strict-CSP check;
    keeping the timer in Python preserves that policy and still tests real state.
    """
    deadline = asyncio.get_running_loop().time() + timeout / 1000
    while True:
        value = await page.evaluate(expression)
        if value:
            return value
        if asyncio.get_running_loop().time() >= deadline:
            raise TimeoutError(f"Timed out after {timeout} ms waiting for {expression}")
        await asyncio.sleep(0.05)


async def start_night(page):
    await page.wait_for_selector("button[data-a=start]")
    await page.click("button[data-a=start]")
    # Repeated test reloads may have filled all case slots in this isolated context.
    if await page.locator(".savemenu [data-new]").count():
        await page.locator(".savemenu [data-new]").last.click()
        if await page.locator(".savemenu [data-yes]").count():
            await page.locator(".savemenu [data-yes]").click()
    await wait_for_js(page, "S47.started() && !!S47.game.d.ui.readAloud")
    await page.evaluate("S47.hold = true; S47.tick(0.2); S47.game.d.ui.close()")
    # Observe the genuine DocSpec passed by game interactions. This avoids
    # mistaking the descriptive transcript for the clue written on the paper.
    await page.evaluate("""() => {
      const ui = S47.game.d.ui;
      if (ui.__readingDocumentProbe) return;
      const open = ui.document.bind(ui);
      ui.document = (doc, close) => {
        window.__readingLastDocument = { id: doc.id, page: doc.page, transcript: doc.transcript };
        return open(doc, close);
      };
      ui.__readingDocumentProbe = true;
    }""")


async def quiet_background(page):
    # Only the new narration feeds master directly; silence the existing buses so
    # the analyser cannot mistake wind or title music for successful narration.
    await page.evaluate("""() => {
      const a = S47.audio;
      for (const bus of [a.sfx, a.amb, a.music, a.radioBus]) {
        bus.gain.cancelScheduledValues(a.ctx.currentTime); bus.gain.value = 0;
      }
      a.setVolume(0.8);
    }""")


async def open_game_document(page, supported=True, kind="description"):
    for item in ("logbook", "workOrder", "halley", "rfiCard", "serviceRecord"):
        exists = await page.evaluate("id => !!S47.game.d.inter.get(id)", item)
        if not exists:
            continue
        await page.evaluate("id => S47.game.d.inter.get(id).use()", item)
        await page.wait_for_selector(".docview")
        doc = await page.evaluate("window.__readingLastDocument")
        text = doc["page"] if kind == "document" else doc["transcript"]
        texts = [part for part in re.split(r"\n\s*\n", text) if part.strip()]
        button = page.locator(f".docview .read-aloud[data-reading-kind={kind}] [data-a=narrate]")
        available = await button.count() == 1 and not await button.is_disabled()
        if not supported or available:
            return item, texts
        await page.locator(".docview [data-a=back]").click()
    raise AssertionError("No supported real game document found in the generated clip catalogue")


async def open_text_document(page, text, title="Reading check"):
    await page.evaluate("""({text, title}) => S47.game.d.ui.document({
      id: 'readaloud-test', kind: 'typed', title, page: text, transcript: text,
    }, () => { window.__readingClosed = (window.__readingClosed || 0) + 1; })""",
                        {"text": text, "title": title})
    await page.wait_for_selector(".docview .read-aloud")


async def sample_master(page, milliseconds=550):
    return await page.evaluate("""async ms => {
      const a = S47.audio, analyser = a.ctx.createAnalyser(); analyser.fftSize = 2048;
      a.master.connect(analyser);
      const data = new Float32Array(analyser.fftSize), levels = [];
      try {
        for (let elapsed = 0; elapsed < ms; elapsed += 40) {
          await new Promise(resolve => setTimeout(resolve, 40));
          analyser.getFloatTimeDomainData(data);
          levels.push(Math.sqrt(data.reduce((sum, x) => sum + x * x, 0) / data.length));
        }
      } finally { a.master.disconnect(analyser); analyser.disconnect(); }
      return {peakRms: Math.max(...levels), levels};
    }""", milliseconds)


async def no_horizontal_overflow(page, selector):
    return await page.evaluate("""selector => {
      const el = document.querySelector(selector); if (!el) return false;
      const rect = el.getBoundingClientRect();
      return document.documentElement.scrollWidth <= innerWidth + 2 &&
        rect.left >= -2 && rect.right <= innerWidth + 2 &&
        el.scrollWidth <= el.clientWidth + 2;
    }""", selector)


async def main():
    global URL
    OUT.mkdir(parents=True, exist_ok=True)
    catalogue = json.loads(Path("src/assets/audio/kinski/catalog.json").read_text())
    clips = catalogue.get("clips", [])
    if not clips:
        raise AssertionError("Generate the prerecorded clip catalogue before running this browser check")
    # The expanded bank includes sub-second number/identifier pieces. Use a
    # complete line above the trace's 0.5 s sound-effect exclusion for this
    # natural-end queue check; short pieces are covered by descriptions.py.
    shortest = min((clip for clip in clips if clip["duration"] >= 0.8), key=lambda clip: clip["duration"])
    stems = {Path(clip["file"]).stem for clip in clips}

    def reading_url(url):
        name = url.split("?", 1)[0].rsplit("/", 1)[-1]
        return any(name == stem + ".mp3" or name.startswith(stem + "-") and name.endswith(".mp3")
                   for stem in stems)

    # Exercise the same no-connect-src policy as tools/csptest.py, including a real
    # Read click. Plain fetch(data:...) would fail here even with media-src data:.
    csp_dir = None
    if not PAGES:
        csp_dir = tempfile.TemporaryDirectory(prefix="s47-readaloud-csp-")
        html = Path("dist-single/index.html").read_text()
        target = Path(csp_dir.name) / "index.html"
        target.write_text(html.replace("<head>", '<head><meta http-equiv="Content-Security-Policy" '
                                        f'content="{CSP}">', 1))
        URL = target.as_uri()

    errors, requests, bad_responses = [], [], []
    report = {"url": URL, "pages": PAGES, "strict_csp": not PAGES}
    try:
        async with async_playwright() as playwright:
            browser = await playwright.chromium.launch(args=[
                "--use-angle=swiftshader", "--enable-unsafe-swiftshader", "--ignore-gpu-blocklist",
            ])
            context = await browser.new_context(viewport={"width": 1280, "height": 800})

            async def new_page(ctx):
                page = await ctx.new_page()
                page.set_default_timeout(60000)
                await page.add_init_script(INSTRUMENT)
                page.on("pageerror", lambda error: errors.append("PAGEERROR: " + str(error)))
                page.on("console", lambda message: errors.append(message.type + ": " + message.text)
                        if message.type in ("error", "warning") else None)
                page.on("request", lambda request: requests.append(request.url) if reading_url(request.url) else None)
                page.on("response", lambda response: bad_responses.append((response.status, response.url))
                        if reading_url(response.url) and response.status >= 400 else None)
                return page

            page = await new_page(context)
            await page.goto(URL)
            await wait_for_js(page, "!!window.S47")
            await page.evaluate("localStorage.clear()")
            await page.reload()
            await start_night(page)
            check(not (await page.evaluate(STATE))["enabled"], "Kinski reading is off on a fresh device")
            await open_game_document(page, supported=False)
            check(await page.locator(".docview .read-aloud").count() == 0,
                  "opening a document with reading off adds no playback controls")
            await page.locator(".docview [data-a=back]").click()
            await page.keyboard.press("Escape")
            toggle = page.locator("button[data-a=kinski]")
            await toggle.wait_for()
            check(await toggle.get_attribute("aria-pressed") == "false", "pause setting visibly defaults to Off")
            await toggle.click()
            check(await toggle.get_attribute("aria-pressed") == "true" and
                  await page.evaluate("localStorage.getItem('s47.kinskiReadAloud')") == "true",
                  "the Kinski setting switches on and is stored on this device")
            await page.locator("[data-a=resume]").click()
            await page.reload()
            await start_night(page)
            check((await page.evaluate(STATE))["enabled"], "reading stays enabled after a full page reload")
            await quiet_background(page)

            doc_id, doc_texts = await open_game_document(page)
            check(bool(doc_texts), f"a real supported game document opens ({doc_id})")
            await asyncio.sleep(0.2)  # let the page-turn sound finish before resetting the probe
            await page.evaluate("window.__readingTrace.reset()")
            await page.screenshot(path=str(OUT / "r01_document_desktop.png"))
            check(await no_horizontal_overflow(page, ".docview"), "desktop document and reading controls fit the viewport")
            description_button = page.locator(DESCRIPTION + " [data-a=narrate]")
            check(await description_button.inner_text() == "Read description",
                  "description reading is explicitly labelled separately from the paper's actual text")
            await description_button.click()
            await wait_for_js(page, f"{READER}.playing")
            await wait_for_js(page, "window.__readingTrace.starts.length > 0")
            started = await page.evaluate(SNAPSHOT)
            record = started[0]
            decoded = record.get("decoded") or {}
            head = decoded.get("head", [])
            mp3_header = head[:3] == [73, 68, 51] or len(head) >= 2 and head[0] == 255 and head[1] & 224 == 224
            check(record["master"] and decoded.get("bytes", 0) > 1000 and mp3_header and record["duration"] > 0.5,
                  f"Read decodes a real MP3 and starts its buffer through master ({record['duration']:.2f}s)")
            check(await description_button.inner_text() == "Stop reading",
                  "the live Read button becomes Stop reading")
            audible = await sample_master(page)
            await page.evaluate("S47.audio.setVolume(0)")
            muted = await sample_master(page, 350)
            await page.evaluate("S47.audio.setVolume(0.8)")
            check(audible["peakRms"] > 0.00001 and muted["peakRms"] < 0.000001,
                  f"real narration samples reach master and obey master volume ({audible['peakRms']:.5f}, muted {muted['peakRms']:.5f})")
            report["master_samples"] = {"reading": audible, "muted": muted}
            await description_button.click()
            check((await page.evaluate(STATE))["state"] == "idle", "Stop reading immediately cancels the current clip and queue")
            check(await description_button.inner_text() == "Read description",
                  "stopping restores the specific Read description label")
            await page.locator(".docview [data-a=back]").click()
            check(await page.evaluate("S47.game.logRead"), "document close still runs the game's original filing callback")

            page_id, page_texts = await open_game_document(page, kind="document")
            source_doc = await page.evaluate("window.__readingLastDocument")
            document_button = page.locator(DOCUMENT + " [data-a=narrate]")
            check(await document_button.inner_text() == "Read document" and
                  normalized(source_doc["page"]) != normalized(source_doc["transcript"]),
                  f"Read document selects the actual clue on a real paper, separately from its description ({page_id})")
            await asyncio.sleep(0.2)
            await page.evaluate("window.__readingTrace.reset()")
            await document_button.click()
            await wait_for_js(page, f"{READER}.playing")
            page_sources = await page.evaluate(SNAPSHOT)
            check(bool(page_sources) and bool(page_sources[0]["decoded"]) and page_sources[0]["master"],
                  "the paper's actual text also starts a genuinely decoded prerecorded clip")
            report["document"] = {"id": page_id, "paragraphs": page_texts, "sources": page_sources}
            await document_button.click()
            check((await page.evaluate(STATE))["state"] == "idle" and await document_button.inner_text() == "Read document",
                  "Stop on document reading clears playback and restores Read document")
            await page.locator(".docview [data-a=back]").click()

            # Queue two actual copies of the shortest shipped line; wait real audio
            # time for the first natural end instead of advancing only game time.
            await page.evaluate("window.__readingTrace.reset(); window.__readingStates = []")
            await page.evaluate("""text => { void S47.game.d.ui.readAloud.play([text, text],
              state => window.__readingStates.push(state)); }""", shortest["text"])
            await wait_for_js(page, "window.__readingTrace.starts.length >= 1")
            await asyncio.sleep(0.2)
            check(len(await page.evaluate(SNAPSHOT)) == 1, "a two-line queue starts only its first recording")
            await wait_for_js(page, "window.__readingTrace.starts.length >= 2",
                              timeout=int(shortest["duration"] * 1000 + 15000))
            queued = await page.evaluate(SNAPSHOT)
            check(len(queued) == 2 and queued[0]["endedAt"] is not None and
                  queued[1]["at"] >= queued[0]["endedAt"] - 0.08,
                  "the second real recording starts after the first ends, with no overlap")
            check(len(await page.evaluate("window.__readingTrace.decodes")) <= 1,
                  "repeating one recording reuses its decoded buffer")
            report["queued_sources"] = queued
            await page.evaluate(f"{READER}.stop()")

            missing = "No prerecorded clip exists for this unique read-aloud check 8f904e6a."
            await open_text_document(page, missing, "Missing recording")
            check(await page.locator(DESCRIPTION + " [data-a=narrate]").is_disabled() and
                  bool(await page.locator(DESCRIPTION + " [data-reading-status]").inner_text()),
                  "a missing recording has a disabled Read button and visible explanation")
            await page.locator(".docview [data-a=back]").click()

            # Open the real Nora conversation through its existing game method;
            # this exercises Panels.talk without loading another world or faking UI.
            await page.evaluate("S47.ch4['openTalk']()")
            await page.wait_for_selector(".talk [data-reading-text]")
            line_texts = await page.evaluate("""() => [...document.querySelectorAll('.talk [data-reading-text]')]
              .map(line => [...line.childNodes].filter(node => node.nodeType === Node.TEXT_NODE)
                .map(node => node.textContent).join(''))""")
            supported = await page.evaluate("texts => S47.game.d.ui.readAloud.available(texts)", line_texts)
            button = page.locator(".labpanel .read-aloud[data-reading-kind=conversation] [data-a=narrate]")
            check(bool(line_texts) and await button.count() == 1 and await button.is_disabled() == (not supported),
                  "the real dialogue has one Read control matching its recording coverage")
            if supported:
                await page.evaluate("window.__readingTrace.reset()")
                await button.click()
                await wait_for_js(page, f"{READER}.playing")
                before = len(await page.evaluate(SNAPSHOT))
                # This real choice updates the reply, with unchanged spoken lines.
                await page.locator(".talk [data-a=table]").click()
                await asyncio.sleep(0.15)
                check((await page.evaluate(STATE))["playing"] and len(await page.evaluate(SNAPSHOT)) == before,
                      "a dialogue redraw with unchanged lines does not restart or overlap narration")
            await page.screenshot(path=str(OUT / "r02_dialogue_desktop.png"))
            await page.locator(".labpanel [data-a=close]").click()
            check((await page.evaluate(STATE))["state"] == "idle", "closing a dialogue cancels narration")

            # Pause and actual save loading must not resurrect an earlier queue.
            await page.evaluate("S47.hold = true; S47.tick(0.2)")
            saved = await page.evaluate("S47.saveNow('manual', 0)")
            check(bool(saved), "the test night can be saved before exercising Load case")
            await page.evaluate("text => { void S47.game.d.ui.readAloud.play([text, text]); }", shortest["text"])
            await wait_for_js(page, f"{READER}.playing")
            await page.keyboard.press("Escape")
            await page.wait_for_selector("button[data-a=kinski]")
            check((await page.evaluate(STATE))["state"] == "idle" and
                  await page.evaluate("S47.audio.ctx.state") == "suspended",
                  "pausing cancels narration and suspends the shared AudioContext")
            await page.locator("[data-a=load]").click()
            await page.locator(".savemenu [data-load]").first.click()
            await wait_for_js(page, "S47.started()")
            await page.evaluate("S47.hold = true")
            await asyncio.sleep(0.25)
            check((await page.evaluate(STATE))["state"] == "idle" and (await page.evaluate(STATE))["enabled"],
                  "loading a real save keeps the device preference but starts no stale narration")

            # A fresh module instance has no decoded narration cache. Delay the
            # real Pages request, or the real single-file decode, and close the UI.
            await page.reload()
            await start_night(page)
            await quiet_background(page)
            await open_text_document(page, shortest["text"], "Close while loading")
            await asyncio.sleep(0.2)
            await page.evaluate("window.__readingTrace.reset()")
            gate, seen = asyncio.Event(), asyncio.Event()

            async def delayed(route):
                if reading_url(route.request.url):
                    seen.set()
                    await gate.wait()
                try:
                    await route.continue_()
                except Exception:
                    # stop() aborts the real fetch while its route is deliberately held.
                    if not gate.is_set():
                        raise

            if PAGES:
                await page.route("**/*", delayed)
            else:
                await page.evaluate("""() => {
                  const ctx = S47.audio.ctx, decode = ctx.decodeAudioData.bind(ctx);
                  window.__readingDecodeHeld = false;
                  ctx.decodeAudioData = async data => {
                    const buffer = await decode(data); window.__readingDecodeHeld = true;
                    await new Promise(resolve => { window.__readingReleaseDecode = resolve; });
                    return buffer;
                  };
                }""")
            await page.locator(DESCRIPTION + " [data-a=narrate]").click()
            if PAGES:
                await asyncio.wait_for(seen.wait(), timeout=15)
            else:
                await wait_for_js(page, "window.__readingDecodeHeld")
            check((await page.evaluate(STATE))["state"] == "loading", "the pending real recording is visibly loading")
            await page.locator(".docview [data-a=back]").click()
            if PAGES:
                gate.set()
                await page.unroute("**/*", delayed)
            else:
                await page.evaluate("window.__readingReleaseDecode()")
            await asyncio.sleep(0.7)
            check((await page.evaluate(STATE))["state"] == "idle" and not await page.evaluate(SNAPSHOT),
                  "closing during pending fetch/decode prevents any delayed audio start")
            check(await page.evaluate("window.__readingClosed || 0") == 1,
                  "the original close callback still runs exactly once during loading")

            # Mobile: use the real setting and document controls with touch input.
            mobile_context = await browser.new_context(viewport={"width": 844, "height": 390},
                                                       is_mobile=True, has_touch=True, device_scale_factor=1)
            mobile = await new_page(mobile_context)
            await mobile.goto(URL)
            await mobile.wait_for_selector("button[data-a=set]")
            await mobile.locator("button[data-a=set]").tap()
            toggle = mobile.locator("button[data-a=kinski]")
            await toggle.tap()
            check(await toggle.get_attribute("aria-pressed") == "true", "mobile Settings can turn Kinski reading on")
            await toggle.scroll_into_view_if_needed()
            await mobile.screenshot(path=str(OUT / "r03_settings_mobile.png"))
            check(await no_horizontal_overflow(mobile, ".settings-panel"),
                  "the mobile settings panel scrolls without horizontal overflow at 844x390")
            await mobile.locator("[data-a=resume]").tap()
            await start_night(mobile)
            await quiet_background(mobile)
            await open_game_document(mobile)
            read_button = mobile.locator(DESCRIPTION + " [data-a=narrate]")
            await read_button.scroll_into_view_if_needed()
            await mobile.screenshot(path=str(OUT / "r04_document_mobile.png"))
            check(await no_horizontal_overflow(mobile, ".docview") and await read_button.is_visible(),
                  "the mobile document exposes its reading control without horizontal overflow")
            await read_button.tap()
            await wait_for_js(mobile, f"{READER}.playing")
            check((await mobile.evaluate(STATE))["playing"], "a real touch on Read starts decoded audio on mobile Chromium")
            await read_button.tap()
            await mobile.locator(".docview [data-a=back]").tap()
            check((await mobile.evaluate(STATE))["state"] == "idle", "mobile Stop and close leave no narration playing")
            await mobile_context.close()

            check(not bad_responses, f"all requested prerecorded files load without HTTP errors ({len(requests)} requests)")
            if not PAGES:
                check(not requests, "the strict-CSP single-file build asks for no external narration files")
            ignored = ("GPU stall", "Automatic fallback to software WebGL", "favicon")
            noise = [error for error in errors if not any(term in error for term in ignored)]
            check(not noise, "no browser errors or warnings" + (": " + " | ".join(noise[:4]) if noise else ""))
            report.update({"requests": requests, "bad_responses": bad_responses, "errors": noise})
            await browser.close()
    finally:
        if csp_dir:
            csp_dir.cleanup()
        report["checks"] = CHECKS
        report["physical_speakers"] = "UNVERIFIED: analyser samples are not a hardware listening test"
        (OUT / "readaloud.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    passed = sum(check["pass"] for check in CHECKS)
    print(f"{passed} of {len(CHECKS)} PASS", flush=True)
    return 0 if passed == len(CHECKS) else 1


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
