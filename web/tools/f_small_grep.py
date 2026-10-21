# Small changes 5 to 8 (FORSLAG.md, 20 October, "Små grep"; Tom: "Spenning og skummelt slår faste regler").
#   5  the morning holds its breath (story/morningHolds.ts): chapter five, Mesa Diner, 05:13: not before,
#      not outside on the lot, not while a panel is open or another of the night's things is going (it
#      waits); then at once the cooler behind the counter, the radio and the birds stop (the sound
#      measured on the master bus: silence), nothing else may start (oneAtATime), the music keeps out;
#      a caption; 8 to 12 s later the waitress, text only: "There. Some mornings about now." (never
#      "every morning"); then back one at a time: the cooler, the radio, a bird; the night's log, one
#      note; once, kept after Continue, not after 05:20; the birds come in through the diner's glass
#   6  the card ends on a ring (main.ts chapterBreak, story/Chapter2.ts holdRing): CHAPTER TWO's card
#      is silent until its title is up; then the phone rings under it (heard on the sfx bus); the card
#      cuts away on a ring, not before the title has had its time; the ringing goes on in the game, it
#      is the supervisor's line in front of the player, and answering stops it; the summaries are as
#      they were; with the read-aloud voice on (the phone), the ring waits for the voice to reach the
#      title and the cut for the voice to finish
#   7  the lines meet (ui/areaMap.ts): C on the notebook's map only once the maps at the diner have
#      found it (P13); with A and B drawn in as well, all three drawn on past where they are known and a
#      small circle where they cross, at the transit; nothing of it with only two; the first time the
#      map is turned to with it, one note in Reyes's words, and the line under the map
#   8  a thread you do not remember (story/sameThread.ts): after a time away, one red thread on the case
#      board the player did not tie, from the photograph they did not take to a card they have (never a
#      question, never a pin an open question rests on), "same" in pencil on it; the first time the
#      board is laid out with it, the board opens on it and says "I don't remember drawing that.", one
#      note; once per time away, a second time away gets its own to another card; kept after Continue
# Desktop 1280x720 and a phone 412x892 with touch; no console errors or warnings.
# Usage: python3 tools/f_small_grep.py OUTDIR
import asyncio, sys, json, os, math
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
LINE = 'There. Some mornings about now.'
SAY = "I don't remember drawing that."
MEET = 'All three cross at the transit. I stood at it tonight.'
T513 = 5 * 3600 + 13 * 60
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

HELPERS = """(() => {
  window.T = {
    toasts: [],
    hook() {
      const ui = S47.game.d.ui; if (ui.__hooked) return; ui.__hooked = true;
      const o = ui.toast.bind(ui); ui.toast = (t, s, c) => { T.toasts.push([t, S47.game.gt]); return o(t, s, c); };
    },
    // the level on the game's buses, measured while the night runs in real time (ticks of 0.05 s)
    meter() {
      if (T.an) return;
      const a = S47.audio, ctx = a.ctx, mk = (n) => { const an = ctx.createAnalyser(); an.fftSize = 2048; n.connect(an); return an; };
      T.an = { all: mk(a.master), sfx: mk(a.sfx), amb: mk(a.amb), radio: mk(a.radioBus), music: mk(a.music) };
    },
    rms(an) { const d = new Float32Array(an.fftSize); an.getFloatTimeDomainData(d); let s = 0; for (const v of d) s += v * v; return Math.sqrt(s / d.length); },
    async run(secs, step = 0.05) {
      T.meter(); const m = {}, peak = {}; let n = 0; const t0 = performance.now();
      while (performance.now() - t0 < secs * 1000) {
        S47.tick(step); await new Promise((r) => setTimeout(r, step * 1000));
        for (const k in T.an) { const v = T.rms(T.an[k]); m[k] = (m[k] || 0) + v * v; peak[k] = Math.max(peak[k] || 0, v); } n++;
      }
      const out = {}; for (const k in m) out[k] = +Math.sqrt(m[k] / n).toFixed(5); out.peak = peak; return out;
    },
    // real-time ticks until a condition holds (or secs pass)
    async until(fn, secs, step = 0.1) { const t0 = performance.now(); while (performance.now() - t0 < secs * 1000) { if (fn()) return true; S47.tick(step); await new Promise((r) => setTimeout(r, step * 1000)); } return fn(); },
    notes(s) { return S47.game.notes.filter((n) => n.includes(s)); },
    said(s) { return T.toasts.filter(([t]) => t.includes(s)); },
    // pixels of the notebook's map darker than the paper, round a ring or along a stretch
    async mapDark(pts) {
      const src = document.querySelector('.nb-map img').src, img = new Image(); img.src = src; await img.decode();
      const c = document.createElement('canvas'); c.width = img.width; c.height = img.height; const g = c.getContext('2d', { willReadFrequently: true }); g.drawImage(img, 0, 0);
      let n = 0; for (const [x, y] of pts) { const d = g.getImageData(Math.round(x) - 1, Math.round(y) - 1, 3, 3).data; let dark = false; for (let i = 0; i < d.length; i += 4) if (d[i] < 170) dark = true; if (dark) n++; }
      return n;
    },
  };
  return true;
})()"""
RING = [[170 + 31 * math.cos(a * math.pi / 36), 372 + 31 * math.sin(a * math.pi / 36)] for a in range(72)]

async def page(b, w, h, touch, voice=False):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
    except Exception: pass
    await pg.evaluate("localStorage.clear()")
    if not voice: await pg.add_init_script("try{localStorage.setItem('s47.serlingNarrator','false');localStorage.setItem('s47.characterVoices','false');}catch{}")
    await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    if touch: await pg.tap('button[data-a=start]'); await begin_opening(pg, touch=True)
    else: await pg.click('button[data-a=start]'); await begin_opening(pg)
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS)
    await pg.evaluate("T.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    await pg.evaluate("S47.hold = true; S47.curve.free = true; S47.input.locked = true")
    return ctx, pg, errs

async def continue_case(pg, tag):
    ev = pg.evaluate
    await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(600)
    await pg.reload()
    await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
    await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
    await ev(HELPERS); await ev("T.hook()")
    await ev("S47.hold = true; S47.curve.free = true; S47.input.locked = true; S47.tick(0.5)")

# ======================= 6: the card ends on a ring =======================
async def card(pg, tag, voiced):
    ev = pg.evaluate
    await ev("S47.jump('chapter1'); S47.tick(1.0); S47.game.d.ui.close(true); S47.setAbductions(false)")
    await ev("T.meter()")
    await ev("S47.ch1.onEnd('passive')")
    # on the black, then the card: until the ring, nothing of the phone is heard
    ok = await ev("T.until(() => !!document.querySelector('.chapter-card'), 6)")
    check(ok and await ev("S47.game.phase") == 'ch2' and await ev("S47.ch2.ringing && S47.ch2.holdRing && !S47.audio['loops'].has('ring2')"),
          f'{tag}: chapter two has begun behind the card, its phone ringing but held back')
    text = await ev("document.querySelector('.chapter-card').textContent")
    check('The shielded lamp did not explain it.' in text and 'THE AMENDED RECORD' in text and 'CHAPTER TWO' in text, f'{tag}: the card and its summary as they were')
    before = await ev("T.run(1.5)")
    check(not await ev("S47.audio['loops'].has('ring2')") and before['peak']['sfx'] < 0.006, f'{tag}: under the summary, no ring (sfx peak {before["peak"]["sfx"]:.4f})')
    await ev("T.until(() => S47.cardRing() && S47.cardRing().start !== null, 40)")
    cr = await ev("S47.cardRing()")
    stage = await ev("S47.game.d.ui.chapterStage")
    if voiced:
        check(cr['voiced'] and cr['start'] is not None and stage != 'a', f'{tag}: with the voice reading the card, the ring waits for it to reach the title (at {cr["start"]:.1f} s, stage {stage})')
    else:
        check(not cr['voiced'] and cr['start'] is not None and 3.0 <= cr['start'] <= 3.6, f'{tag}: without the voice, the ring as the summary goes and the title comes (at {cr["start"]:.1f} s)')
    await pg.wait_for_timeout(700 if not voiced else 100)
    await ev("T.run(1.0)")
    check(await ev("S47.audio['loops'].has('ring2')") and await ev("S47.game.cinematic && !!document.querySelector('.chapter-card')"), f'{tag}: the phone rings under the card, still on the black')
    rang = await ev("T.run(1.2)")
    await pg.screenshot(path=f'{OUT}/{tag}_card_ring.png')
    check(rang['peak']['sfx'] > max(0.01, 3 * before['peak']['sfx']), f'{tag}: and is heard (sfx peak {rang["peak"]["sfx"]:.3f}, {before["peak"]["sfx"]:.4f} before it)')
    await ev("T.until(() => !S47.game.cinematic, 20, 0.05)")
    cr = await ev("S47.cardRing()")
    rings = sorted(cr['rings']); last = rings[-1] if rings else -1
    gap = cr['cut'] - (rings[-2] if len(rings) > 1 else cr['start'])
    check(cr['cut'] is not None and abs(cr['cut'] - last) < 0.15 and 4.5 <= gap <= cr['period'] + 0.15,
          f'{tag}: the card cuts away on a ring ({cr["cut"]:.2f} s; rings at {[round(r, 2) for r in rings]}, {gap:.2f} s after the one before)')
    if not voiced: check(cr['cut'] >= 6.4, f'{tag}: not before the title has had its time ({cr["cut"]:.1f} s)')
    await pg.wait_for_timeout(150)
    await pg.screenshot(path=f'{OUT}/{tag}_card_cut.png')
    check(await ev("S47.audio['loops'].has('ring2') && S47.ch2.ringing") and 'SUPERVISOR LINE IS RINGING' in await ev("document.querySelector('.objective').textContent"),
          f'{tag}: and it goes on ringing in the game: the supervisor\'s line in front of the player')
    await ev("T.until(() => T.said('The phone again').length > 0, 3)")
    said = await ev("T.said('The phone again')")
    check(len(said) == 1, f'{tag}: "The phone again. The supervisor\'s line." once, after the cut')
    await ev("S47.game.d.inter.get('phone').use(); S47.tick(0.1)")
    check(not await ev("S47.audio['loops'].has('ring2')") and await ev("S47.ch2.s.stage") in ('ward-call', 'records'), f'{tag}: answered, the ringing stops (Ward on the line)')
    await ev("T.until(() => !S47.game.cinematic, 30, 0.2)")

# ======================= 5: the morning holds its breath =======================
async def to_diner(pg):
    ev = pg.evaluate
    await ev("S47.jump('chapter5'); S47.tick(1.0); S47.game.d.ui.close(true); S47.truckB.off = true")
    await ev("async () => { await S47.world.prepare('diner'); S47.world.placeTruck('diner'); S47.world.enter('diner'); S47.world.placeAtDiner('inside'); S47.ch5.arrivedDiner(); S47.tick(0.3); }")
    await ev("S47.game.d.ui.close(true); S47.game.d.ui.clearToasts(); S47.game.d.ui.showHud(true, S47.input.touchMode); S47.tick(0.2)")

async def morning(pg, tag, full):
    ev = pg.evaluate
    await to_diner(pg)
    m = lambda: ev("S47.morning.phase")
    check(await ev("S47.world.area === 'diner' && S47.space() === 'room' && S47.morningIn().here"), f'{tag}: in Mesa Diner in chapter five, inside')
    # the birds through the glass at dawn
    await ev("S47.game.clock = 5 * 3600 + 8 * 60")
    b0 = await ev("S47.audio.night.birds")
    await ev("T.run(2.0, 0.25)")
    for _ in range(40): await ev("S47.tick(1)")
    b1 = await ev("S47.audio.night.birds")
    check(b1 - b0 >= 3 and await ev("S47.audio.night.window") == 1, f'{tag}: before it, birds outside, heard in through the glass ({b1 - b0} in 40 s)')
    check(await m() == 'idle', f'{tag}: 05:08: nothing yet')
    if full:
        # not outside on the lot
        await ev("S47.world.placeAtDiner('arrive'); S47.tick(0.2)")
        await ev(f"S47.game.clock = {T513 + 5}")
        for _ in range(4): await ev("S47.tick(0.5)")
        check(await m() == 'idle' and not await ev("S47.morningIn().here"), f'{tag}: 05:13 out on the lot: nothing')
        # not while a panel is open: it waits
        await ev("S47.game.clock = 5 * 3600 + 12 * 50; S47.world.placeAtDiner('inside'); S47.tick(0.2)")
        await ev("S47.game.d.inter.get('diner:waitress').use(); S47.tick(0.2)")
        await ev(f"S47.game.clock = {T513 + 5}")
        for _ in range(4): await ev("S47.tick(0.5)")
        check(await m() == 'idle' and await ev("!!S47.game.d.ui.modal"), f'{tag}: talking to the waitress: it waits')
        await ev("S47.game.d.ui.close(true); S47.input.locked = true; S47.game.d.ui.showHud(true, false); S47.ninth.quiet = 3; S47.tick(0.1)")
        check(await m() == 'idle' and await ev("S47.oneAtATime()"), f'{tag}: another of the night\'s things going: it waits')
        await ev("T.until(() => S47.ninth.quiet <= 0, 8, 0.2)")
    else:
        await ev(f"S47.game.clock = {T513 + 2}")
    await ev("T.until(() => S47.morning.phase !== 'idle', 20, 0.1)")
    before = await ev("T.run(0.01)")   # (the meter running)
    check(await m() == 'held' and await ev("S47.townSeen.has('morning:held')"), f'{tag}: then it holds ({await m()})')
    await ev("T.run(1.2)")
    st = await ev("({ still: S47.audio.night.isStill, tone: S47.audio.roomToneHeld, radio: S47.dinerRadio.power, cooler: S47.dinerCooler.running, one: S47.oneAtATime(), music: S47.musicHeld() })")
    quiet = await ev("T.run(2.0)")
    check(st['still'] and st['tone'] < 0.05 and st['radio'] == 0 and not st['cooler'], f'{tag}: at once the cooler, the radio, the birds and the room\'s hum stop ({st})')
    check(quiet['all'] < 2e-4 and quiet['peak']['all'] < 1e-3, f'{tag}: and it is silent (master {quiet["all"]}, peak {quiet["peak"]["all"]:.5f}; {quiet})')
    check(st['one'] and st['music'], f'{tag}: nothing else may start meanwhile, and the music keeps out')
    await ev("T.until(() => T.said('The cooler behind the counter cuts out').length > 0, 3)")
    check(len(await ev("T.said('The cooler behind the counter cuts out')")) == 1, f'{tag}: a caption for it')
    await ev(f"T.until(() => T.said('{LINE}').length > 0, 12, 0.05)")
    said = await ev(f"T.said('{LINE}')")
    tm = await ev("S47.morning.t")
    await pg.wait_for_timeout(250)
    await pg.screenshot(path=f'{OUT}/{tag}_morning_line.png')
    check(len(said) == 1 and said[0][0] == f'THE WAITRESS: "{LINE}"' and 8 <= tm <= 12.5, f'{tag}: {tm:.1f} s of nothing, then the waitress, as text: {said[0][0] if said else None}')
    check(not any('every morning' in t.lower() for t, _ in await ev("T.toasts")) and not any('every morning' in n.lower() for n in await ev("S47.game.notes")), f'{tag}: never "every morning"')
    still_quiet = await ev("T.run(1.5)")
    check(still_quiet['all'] < 2e-4, f'{tag}: still nothing under her line (master {still_quiet["all"]})')
    order = []
    async def mark():
        s = await ev("({ cooler: S47.dinerCooler.running, radio: S47.dinerRadio.power, still: S47.audio.night.isStill, t: S47.morning.t })")
        for k, on in (('cooler', s['cooler']), ('radio', s['radio'] > 0.5), ('birds', not s['still'])):
            if on and k not in [o[0] for o in order]: order.append((k, round(s['t'], 1)))
    for _ in range(60):
        await ev("T.run(0.3)"); await mark()
        if await m() == 'done': break
    check([o[0] for o in order] == ['cooler', 'radio', 'birds'] and order[1][1] - order[0][1] >= 3 and order[2][1] - order[1][1] >= 3,
          f'{tag}: then back one at a time: {order}')
    after = await ev("T.run(1.5)")
    check(await m() == 'done' and not await ev("S47.oneAtATime()") and after['all'] > 0.005 and await ev("S47.audio.roomToneHeld") > 0.9, f'{tag}: and the room as it was (master {after["all"]})')
    log = await ev("S47.seen.filter((e) => e.what === 'the morning holds').map((e) => [e.kind, e.area])")
    notes = await ev(f"T.notes('{LINE}')")
    check(log == [['event', 'diner']] and len(notes) == 1, f'{tag}: the night\'s log has it, the notebook once ({log}, {len(notes)})')
    await ev(f"S47.game.clock = {T513 + 30}")
    for _ in range(6): await ev("S47.tick(0.5)")
    check(await m() == 'done' and len(await ev(f"T.notes('{LINE}')")) == 1, f'{tag}: once: not again')
    if full:
        await continue_case(pg, tag)
        s = await ev("({ ph: S47.morning.phase, had: S47.morning.had, still: S47.audio.night.isStill, tone: S47.audio.roomToneHeld, radio: S47.dinerRadio.power })")
        for _ in range(6): await ev("S47.tick(0.5)")
        check(s['had'] and s['ph'] == 'done' and await ev("S47.morning.phase") == 'done' and not s['still'] and s['radio'] == 1, f'{tag}: after Continue, had: not again, and the sounds as they are ({s})')
        # walked in after 05:13: not the moment the door shuts, the room heard first; and a panel opened
        # in the stillness holds it, her line with it, until it is closed
        await ev("S47.townSeen.delete('morning:held'); S47.morning.reset(); S47.game.clock = 5 * 3600 + 15 * 60")
        await ev("S47.world.placeAtDiner('arrive'); S47.tick(0.5); S47.world.placeAtDiner('inside'); S47.tick(0.1)")
        await ev("T.run(4.0, 0.1)")
        check(await m() == 'idle', f'{tag}: walked in at 05:15: not the moment the door shuts')
        await ev("T.until(() => S47.morning.phase !== 'idle', 20, 0.1)")
        check(await m() == 'held', f'{tag}: but once the room has been heard ({await m()})')
        n0 = len(await ev(f"T.said('{LINE}')"))
        await ev("T.run(1.0)")
        await ev("S47.game.d.inter.get('diner:booth').use(); S47.tick(0.1)")
        await ev("T.run(14.0)")
        held = await ev("({ t: S47.morning.t, modal: !!S47.game.d.ui.modal, still: S47.audio.night.isStill, cooler: S47.dinerCooler.running })")
        check(held['modal'] and held['t'] < 10.6 and held['still'] and not held['cooler'] and len(await ev(f"T.said('{LINE}')")) == n0,
              f'{tag}: the maps spread on the table in the stillness: it waits under them, her line not said under the panel ({held})')
        await ev("S47.game.d.ui.close(true); S47.input.locked = true; S47.tick(0.05)")
        await ev(f"T.until(() => T.said('{LINE}').length > {n0}, 12, 0.05)")
        check(len(await ev(f"T.said('{LINE}')")) == n0 + 1, f'{tag}: closed, and then she says it')
        await ev("T.until(() => S47.morning.phase === 'done', 30, 0.2)")
        check(await m() == 'done' and not await ev("S47.audio.night.isStill") and await ev("S47.dinerCooler.running"), f'{tag}: and the room back')
        # not after 05:20
        await ev("S47.townSeen.delete('morning:held'); S47.morning.reset(); S47.game.clock = 5 * 3600 + 20 * 60 + 5")
        await ev("S47.world.enter('diner'); S47.world.placeAtDiner('inside'); S47.tick(0.2)")
        for _ in range(6): await ev("S47.tick(0.5)")
        check(await ev("S47.morning.phase") == 'idle', f'{tag}: a night that had not had it, after 05:20: nothing')

# ======================= 7: the lines meet =======================
async def lines_meet(pg, tag):
    ev = pg.evaluate
    await ev("S47.jump('chapter5'); S47.tick(1.0); S47.game.d.ui.close(true)")
    for k in ['mark:phone@11000', 'mark:lamps@12000', 'mark:boy@12500', 'mark:lights@13000', 'mark:cows@14000']:
        await ev(f"S47.townSeen.add('{k}')")
    async def view_map():
        await ev("S47.game.d.ui.close(true); S47.game.d.ui.notebookAt('tasks'); S47.game.openNotebook()")
        await ev("document.querySelector('.notebook [data-t=map]').click()"); await pg.wait_for_timeout(250)
        cmid = [[170 + (1005 - 170) * k, 372 + (410 - 372) * k] for k in [0.35 + 0.01 * i for i in range(30)]]
        return {'ring': await ev(f"T.mapDark({json.dumps(RING)})"), 'c': await ev(f"T.mapDark({json.dumps(cmid)})"),
                'note': await ev("(document.querySelector('.nb-mapnote') || {}).textContent || ''"), 'meet': await ev("S47.townSeen.has('lines:meet')")}
    # A three, B two, C not found
    v0 = await view_map()
    check(v0['ring'] < 20 and v0['c'] <= 4 and not v0['meet'] and not v0['note'], f'{tag}: A drawn, B not yet, C not found: no C, no circle ({v0})')
    await ev("S47.townSeen.add('mark:tv@15000')")
    v1 = await view_map()
    check(v1['ring'] < 20 and v1['c'] <= 4 and not v1['meet'], f'{tag}: A and B drawn, C not found: still nothing more ({v1})')
    await pg.screenshot(path=f'{OUT}/{tag}_map_two.png')
    n0 = len(await ev("T.notes('They cross in one point')"))
    await ev("S47.ch5.s.p13 = true")
    v2 = await view_map()
    check(v2['c'] >= 12 and v2['ring'] >= 50 and v2['meet'] and v2['note'] == MEET, f'{tag}: with C found at the diner: C drawn, all three on to where they cross, a small circle there ({v2})')
    notes = await ev("T.notes('They cross in one point')")
    check(n0 == 0 and len(notes) == 1 and 'transit at STATION 01' in notes[0], f'{tag}: the first time the map is turned to with it, one note: {notes[0] if notes else None}')
    await pg.screenshot(path=f'{OUT}/{tag}_map_meet.png')
    await view_map()
    check(len(await ev("T.notes('They cross in one point')")) == 1, f'{tag}: turned to again: no second note')
    await ev("S47.game.d.ui.close(true)")
    # only C found, A and B not drawn: C, no circle (a fresh look: the marks gone)
    for k in ['mark:phone@11000', 'mark:lamps@12000', 'mark:boy@12500', 'mark:lights@13000', 'mark:cows@14000', 'mark:tv@15000']:
        await ev(f"S47.townSeen.delete('{k}')")
    v3 = await view_map()
    check(v3['c'] >= 12 and v3['ring'] < 20 and not v3['note'], f'{tag}: C found but A and B not drawn: C alone, no circle ({v3})')
    await ev("S47.game.d.ui.close(true)")

# ======================= 8: a thread you do not remember =======================
async def taken(pg, x=-24, z=200):
    """On the shoulder south of SARO, on B: the warning at once, the light, and back (tools/abduct.py)."""
    ev = pg.evaluate
    A = "S47.abduction()"
    n = await ev(f"{A}.data.count")
    await ev(f"S47.world.enter('saro'); S47.player.place({x}, {z}, Math.PI); S47.tick(0.3)")
    await ev(f"{A}.pressure = 5; S47.tick(0.5)")
    for _ in range(400):
        await ev("S47.tick(0.5)")
        if await ev(f"{A}.stage") == 'calm' and await ev(f"{A}.data.count") > n: break

async def thread(pg, tag, full):
    ev = pg.evaluate
    A = "S47.abduction()"
    await ev("S47.jump('chapter2'); S47.tick(1.0); S47.setAbductions(true)")
    await ev("S47.game.d.ui.close(); S47.game.clock = 2 * 3600 + 58 * 60; S47.tick(0.2)")
    check(await ev("S47.sameThread.threads.length") == 0, f'{tag}: before any time away, no thread')
    await ev("S47.game.openBoard(); S47.tick(0.1)"); await pg.wait_for_timeout(400)
    check(await ev("!document.querySelector('.bnote.hand')") and await ev("document.querySelector('.board-say').textContent") != SAY, f'{tag}: and nothing on the board')
    await ev("S47.game.d.ui.close(true)")
    await taken(pg)
    th = await ev("S47.sameThread.threads")
    pins = await ev("S47.game.board.pins()")
    qs = await ev("S47.game.board['books']().flatMap((b) => b.o.questions.map((q) => q.id))")
    ok = len(th) == 1 and th[0]['a'] == 'away1' and any(p['pin'] == th[0]['b'] for p in pins) and th[0]['b'] not in qs
    check(ok and await ev(f"!S47.game.board.rests('{th[0]['b'] if th else ''}')"), f'{tag}: after the time away, one thread: from the photograph not taken to a card on the board, no question, nothing open resting on it ({th})')
    check(await ev(f"S47.townSeen.has('same:1=' + '{th[0]['a']}|{th[0]['b']}')") if th else False, f'{tag}: kept with the case')
    n0 = len(await ev(f"T.notes(\"{SAY}\")"))
    await ev("S47.game.openBoard(); S47.tick(0.1)"); await pg.wait_for_timeout(1600)
    hand = await ev("[...document.querySelectorAll('.bnote.hand')].map((e) => [e.textContent.trim(), getComputedStyle(e).fontFamily])")
    say = await ev("document.querySelector('.board-say').textContent")
    paths = await ev("document.querySelectorAll('.board-threads path.thread:not(.thin):not(.auto):not(.live)').length")
    # the word hangs on the yarn: the middle of its scrap near the thread's curve
    near = await ev("""(() => { const n = document.querySelector('.bnote.hand'); if (!n) return 1e9; const x = n.offsetLeft + n.offsetWidth / 2, y = n.offsetTop + n.offsetHeight * 0.3;
      let best = 1e9; for (const p of document.querySelectorAll('.board-threads path.thread:not(.live)')) { if (!p.getAttribute('d')) continue; const L = p.getTotalLength(); for (let i = 0; i <= 60; i++) { const q = p.getPointAtLength(L * i / 60); best = Math.min(best, Math.hypot(q.x - x, q.y - y)); } } return best; })()""")
    await pg.screenshot(path=f'{OUT}/{tag}_board_same.png')
    check(len(hand) == 1 and hand[0][0] == 'same' and 'Reenie' in hand[0][1] and paths >= 1, f'{tag}: on the board, the thread, "same" in pencil in the player\'s hand ({hand}, {paths} full threads)')
    check(near < 40, f'{tag}: the word hangs on the thread ({near:.0f} px from it)')
    notes = await ev(f"T.notes(\"{SAY}\")")
    check(say == SAY and n0 == 0 and len(notes) == 1 and await ev("S47.sameThread.noticed"), f'{tag}: the board opens on it and says it; one note: {notes[0] if notes else None}')
    check(await ev("S47.seen.filter((e) => e.what === 'same thread').length") == 1, f'{tag}: the night\'s log has it')
    a, b2 = th[0]['a'], th[0]['b']
    r = await ev(f"S47.game.board.view.link('{a}', '{b2}')")
    check(r['ok'] and 'already connected' in r['text'], f'{tag}: tying the same two again: "{r["text"]}"')
    await ev("S47.game.d.ui.close(true); S47.game.openBoard(); S47.tick(0.1)"); await pg.wait_for_timeout(1500)
    check(await ev("document.querySelector('.board-say').textContent") != SAY and len(await ev(f"T.notes(\"{SAY}\")")) == 1, f'{tag}: laid out again: not said again')
    await ev("S47.game.d.ui.close(true)")
    if not full: return
    await continue_case(pg, tag)
    th2 = await ev("S47.sameThread.threads")
    await ev("S47.game.openBoard(); S47.tick(0.1)"); await pg.wait_for_timeout(1500)
    check(th2 == th and await ev("document.querySelectorAll('.bnote.hand').length") == 1 and await ev("document.querySelector('.board-say').textContent") != SAY,
          f'{tag}: after Continue: the thread kept, one, not said again ({th2})')
    await ev("S47.game.d.ui.close(true)")
    # a second time away: its own thread, to another card
    last = await ev(f"{A}.data.last")
    await ev(f"{A}.data.last = -1e9; S47.game.clock = 3 * 3600 + 5 * 60; S47.tick(0.1)")
    await taken(pg)
    th3 = await ev("S47.sameThread.threads")
    cnt = await ev(f"{A}.data.count")
    check(cnt == 2 and len(th3) == 2 and th3[1]['a'] == 'away2' and th3[1]['b'] not in (th3[0]['a'], th3[0]['b']), f'{tag}: a second time away, its own thread, to another card ({th3})')
    await ev("S47.game.openBoard(); S47.tick(0.1)"); await pg.wait_for_timeout(800)
    check(await ev("document.querySelectorAll('.bnote.hand').length") == 2 and len(await ev(f"T.notes(\"{SAY}\")")) == 1, f'{tag}: two on the board; the line only the first time')
    await pg.screenshot(path=f'{OUT}/{tag}_board_second.png')
    await ev("S47.game.d.ui.close(true)")

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        # ======================= desktop =======================
        ctx, pg, errs = await page(b, 1280, 720, False)
        await card(pg, 'desk', False)
        await morning(pg, 'desk', True)
        await lines_meet(pg, 'desk')
        await thread(pg, 'desk', True)
        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'desktop: no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        await ctx.close()
        # ======================= phone =======================
        # (with the read-aloud voice on, as a new player has it: the card waits for the voice)
        ctx, pg, errs = await page(b, 412, 892, True, voice=True)
        await card(pg, 'phone', True)
        await ev_off_voice(pg)
        await morning(pg, 'phone', False)
        await lines_meet(pg, 'phone')
        await thread(pg, 'phone', False)
        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'phone: no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        await ctx.close()
        json.dump({'checks': checks}, open(f'{OUT}/f_small_grep.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

async def ev_off_voice(pg):
    # the rest without the voices reading over it
    await pg.evaluate("S47.game.d.ui.narrator && (S47.game.d.ui.narrator.enabled = false)")

asyncio.run(main())
