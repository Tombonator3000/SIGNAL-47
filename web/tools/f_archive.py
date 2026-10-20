# Chapter two's peak in the archive: the phone from the future a second time (story/archivePhone.ts,
# story/archiveDishes.ts, story/reel047.ts; FORSLAG.md, 20 October, item 5 with Valg for Tom 1).
#   - not in chapter one, not before Ward's call, not before the steps on the roof (they are the build)
#   - lifting the wall phone while the steps are on the roof: the line is dead, and the steps go on
#   - after the steps, at the room's last quiet moment: the call recorder prints IN TRK1 EXT 201 by
#     itself, and the wall phone rings, and the control room's through the wall
#   - answered: the records room before it happens (the tube, a thump, a drawer, a box, paper), then
#     the line is dead; the note says it has been heard before; the console log loses WWVB again
#   - the 47 from the line's death: the corner clock in seconds, K3 through the wall at once, :16 the
#     beep, :31 the recorder prints the night's first call again FROM EXT 201, :40 the tired tube goes
#     out; held while a paper is open; no save, nothing else starts
#   - bank 3 put back in time: at :47 the printer prints the same solve with its old time (and the
#     log has it again at that time); left out (the phone run): a blank sheet
#   - :47: every dish rogue, down from 32 to the horizon along 026 together (el 2), the drawer out of
#     the wall behind the STATION 01 box, the box thrown on the floor (and in the way), the papers;
#     after, the dishes at rest, saving again, two notes, the curve's peak landed and ch2's 47 taken
#   - the strip, the sheet and the drawer: read, torn off, reel 047 into the kit
#   - kept with the case: after Continue the drawer, the box, the dishes tipped exactly, the reel, the
#     strip; the phone never rings again; lifting it now calls the key holder (and the strip has it)
#   - reel 047 on the deck: the pulse group, a crank, a cut, 47 s of nothing (not even the hiss), a
#     relay, -32 LY on the display; at the relay the array goes back up to exactly 026 / 32, given back
#   - the phone (412x892, touch): the handset lifted first (no dial tone), by the Use button; nobody at
#     SARO when the dishes are down: they go back unseen, and the first look at them after says so
#   - screenshots where there is something to see; no console errors or warnings
# Usage (from web/): python3 tools/f_archive.py OUTDIR   (S47_URL for another build)
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
ARGS = ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required']
IDB_GET = """((store, key) => new Promise((ok) => { const r = indexedDB.open('s47', 2);
  r.onsuccess = () => { const g = r.result.transaction(store).objectStore(store).get(key);
    g.onsuccess = () => { ok(g.result ?? null); r.result.close(); }; g.onerror = () => ok(null); }; r.onerror = () => ok(null); }))"""
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

HELPERS = """(() => {
  window.T = {
    aim(x, z, tx, ty, tz) {
      const pl = S47.player; pl.place(x, z, 0); S47.tick(0.05);
      const c = S47.camera.position;
      pl.place(x, z, Math.atan2(-(tx - c.x), -(tz - c.z)));
      pl.pitch = Math.atan2(ty - c.y, Math.hypot(tx - c.x, tz - c.z)); S47.tick(0.05);
      return pl.walkable(x, z);
    },
    toasts: [],
    hook() {
      const ui = S47.game.d.ui; if (ui.__hooked) return; ui.__hooked = true;
      const o = ui.toast.bind(ui); ui.toast = (t, s, c) => { T.toasts.push(t); return o(t, s, c); };
    },
    said() { return T.toasts.join(' | '); },
    arc() {
      const a = S47.archive, d = S47.ext.dishes, fb = S47.annex.objs.fieldBoxModel;
      return { phase: a.phase, t: a.t, how: a.how, pending: a.pending, busy: a.busy, racing: a.racing, beats: a.beats.map((b) => b.what),
        town: [...S47.townSeen].filter((k) => k.startsWith('arc')), lines: a.lines(), clock: a.clockShow,
        drawer: S47.annex.arc.drawer.position.x, papers: S47.annex.arc.papers.visible, reelShown: S47.annex.arc.reel.visible,
        box: [fb.position.x, fb.position.y, fb.position.z, fb.rotation.x, fb.rotation.order], tube: S47.annex.tubeMode,
        tip: S47.arrayTip.mode, humming: S47.arrayTip.humming,
        rogue: d.filter((x) => x.rogue).length, az: [Math.min(...d.map((x) => x.curAz)), Math.max(...d.map((x) => x.curAz))],
        el: [Math.min(...d.map((x) => x.curEl)), Math.max(...d.map((x) => x.curEl))], moving: d.filter((x) => x.moving).length,
        k3: S47.game.ns.k3Open, bankIn: S47.game.bankIn, cinematic: S47.game.cinematic };
    },
    log() { return S47.game.ns.log.map((l) => l[1]); },
    logAt(text) { return S47.game.ns.log.filter((l) => l[1] === text).map((l) => l[0]); },
    async until(fn, secs = 60, step = 0.1) { for (let t = 0; t < secs; t += step) { if (fn()) return true; S47.tick(step); } return fn(); },
    quiet() { S47.nightEvents['wait'] = 1e9; S47.glimpses['wait'] = 1e9; S47.calls.next = 1e9; S47.cutouts['wait'] = 1e9; S47.setAbductions(false); },
    hud() { const e = document.querySelector('.hud .examine'); return e && e.classList.contains('show') ? e.textContent : null; },
    corner() { const e = document.querySelector('.clockline'); return e ? e.textContent : null; },
  };
  // the ambience bus (the room's hum, the drives through the walls): K-weighted roughly, the median of
  // 50 ms readings (as tools/horror_roof.py)
  T.bed = async (secs = 1.5) => {
    const a = S47.audio, ctx = a.ctx;
    const inp = ctx.createGain(), hp = ctx.createBiquadFilter(), sh = ctx.createBiquadFilter(), an = ctx.createAnalyser(), z = ctx.createGain();
    hp.type = 'highpass'; hp.frequency.value = 38; sh.type = 'highshelf'; sh.frequency.value = 1700; sh.gain.value = 4;
    an.fftSize = 2048; z.gain.value = 0; inp.connect(hp); hp.connect(sh); sh.connect(an); an.connect(z); z.connect(ctx.destination);
    a.amb.connect(inp);
    const buf = new Float32Array(an.fftSize), l = []; const t0 = performance.now();
    while (performance.now() - t0 < secs * 1000) {
      await new Promise((r) => setTimeout(r, 15));
      an.getFloatTimeDomainData(buf); let s = 0; for (const v of buf) s += v * v; l.push(Math.sqrt(s / buf.length));
    }
    a.amb.disconnect(inp); inp.disconnect(); l.sort((x, y) => x - y); return l[l.length >> 1];
  };
  return true;
})()"""

async def page(b, w, h, touch, errs):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("try{localStorage.setItem('s47.serlingNarrator','false');localStorage.setItem('s47.characterVoices','false');}catch{}")
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
    except Exception: pass
    await pg.evaluate("localStorage.clear()")
    await pg.add_init_script("try{localStorage.setItem('s47.serlingNarrator','false');localStorage.setItem('s47.characterVoices','false');}catch{}")
    await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    if touch: await pg.tap('button[data-a=start]')
    else: await pg.click('button[data-a=start]')
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS)
    await pg.evaluate("T.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    return ctx, pg

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=ARGS)
        errs = []
        # ======================= desktop =======================
        ctx, pg = await page(b, 1280, 720, False, errs)
        ev = pg.evaluate
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        arc = lambda: ev("T.arc()")
        s2 = lambda k: ev(f"S47.ch2.s.{k}")
        notes = lambda: ev("S47.game.notes.slice()")
        async def clear():
            await ev("S47.game.d.ui.clearToasts(); T.toasts.length = 0"); await tick(0.05)
        await ev("S47.hold = true")

        # ---------- not in chapter one ----------
        await ev("S47.jump('chapter1'); S47.tick(1.0); T.quiet()")
        await ev("T.aim(-3.65, 7.75, -3.65, 0.8, 8.6)")
        await tick(1, 40)
        a = await arc()
        check(await ev("S47.game.phase") == 'ch1' and a['phase'] == 'idle' and not a['town'], 'chapter one: 40 s in the records room, nothing')

        # ---------- chapter two: the curve keeps its 47 for the archive ----------
        await ev("S47.jump('chapter2'); S47.tick(1.5); T.quiet()")
        check(await ev("S47.game.phase") == 'ch2' and await s2('stage') == 'ward-call' and (await arc())['pending'], 'chapter two begins with the supervisor line; the archive\'s phone is still to come')
        await ev("S47.curve.force('peak'); S47.player.place(-2, -24, 0); S47.tick(0.3)")
        check(await ev("S47.curve.phase") == 'peak' and not await ev("S47.curve.may('long')") and await ev("S47.curve.may('glimpse')"),
              'at a peak in chapter two, no other 47 may come: the chapter keeps its one for the archive (a glimpse may)')
        await ev("S47.curve.force('rest'); S47.curve.free = true")   # (the curve stands aside from here: each thing on its own)
        await ev("T.aim(-3.65, 7.75, -3.65, 0.8, 8.6)")
        await tick(1, 20)
        check((await arc())['phase'] == 'idle', 'not before Ward has called: 20 s in the records room, nothing')
        await ev("S47.game.d.inter.get('phone').use()"); await tick(1, 12)
        check(await s2('stage') == 'records', 'Ward called; the records are next')

        # ---------- not before the steps (they are the build) ----------
        await ev("T.aim(-5.05, 8.6, -5.55, 1.28, 8.6)")
        await ev("S47.game.d.inter.get('fieldBox').use()"); await tick(0.1)
        await pg.click('.docview [data-a=back]'); await tick(0.1)
        await ev("Object.assign(S47.ch2.s, { p04: true, p05: true, stage: 'call-nora' })")
        # the steps start by themselves at the last quiet moment; until then, nothing of the phone
        await ev("T.aim(-2.3, 8.6, -1.12, 1.42, 9.25)")
        await ev("T.until(() => S47.roofSteps.busy, 15, 0.1)")
        a = await arc()
        check(await ev("S47.roofSteps.busy") and a['phase'] == 'idle' and not a['town'], 'call-nora: the steps on the roof come first, and the phone is quiet while they go')
        # the handset lifted while something stands on the roof: a dead line, and the steps go on
        await ev("T.until(() => S47.roofSteps.phase === 'still', 20, 0.1)")
        await clear()
        await ev("T.aim(-1.75, 9.25, -1.12, 1.42, 9.25)")
        n0 = await ev("S47.roofSteps.log.length")
        await ev("S47.game.d.inter.get('recPhone').use()"); await tick(0.2)
        dead = 'line is dead' in await ev("T.said()") and not await s2('called') and (await arc())['phase'] == 'idle'
        await ev("T.until(() => S47.roofSteps.phase === 'done', 40, 0.1)")
        check(dead and await ev("S47.roofSteps.phase") == 'done' and await ev("S47.roofSteps.log.length") > n0,
              f'lifted while they stand on the roof: the line is dead, no call; the steps go on to the end ({await ev("S47.roofSteps.log.length") - n0} more)')
        # ---------- after them: the recorder, and the ring ----------
        await clear()
        hum0 = await ev("T.bed()")
        waited = 0.0
        while waited < 30 and (await arc())['phase'] == 'idle':
            await tick(0.25); waited += 0.25
        a = await arc()
        check(a['phase'] == 'ringing' and 10 <= waited <= 17, f'{waited:.1f} s after the steps were over (and the 12 s the archive waits for them), at the quiet: the phone rings')
        await tick(2.0)
        a = await arc()
        rings = await ev("S47.archive['ringHandles'].length")
        check(any('IN   TRK1  EXT 201' in l for l in a['lines']) and 'call recorder prints a line' in await ev("T.said()"), f'the call recorder put down IN TRK1 EXT 201 by itself ({a["lines"][-1]})')
        check(rings == 2 and 'control room\'s, on the same line' in await ev("T.said()"), 'the wall phone rings, and the control room\'s through the wall')
        check(await ev("S47.oneAtATime()") and await ev("S47.saveBlock()") and await ev("S47.game.d.inter.get('recPhone').label()") == 'Answer the phone',
              'while it rings: nothing else starts, no save, the wall phone offers the call')
        bed = None
        await ev("T.aim(-1.85, 9.0, -1.08, 1.5, 10.22)"); await tick(0.2); await tick(0.1)
        await shot('desk_01_ringing')

        # ---------- answered ----------
        await clear()
        await ev("T.aim(-1.75, 9.25, -1.12, 1.42, 9.25)")
        await ev("S47.game.d.inter.get('recPhone').use()"); await tick(0.1)
        a = await arc()
        check(a['phase'] == 'line' and a['cinematic'] and a['how'] == 'ring' and 'arc:call' in a['town'], 'answered: the line, the player held')
        k3_before = (await arc())['k3']
        lost0 = len(await ev("T.logAt('STATION CLOCK  WWVB REFERENCE LOST')"))
        await ev("T.until(() => S47.archive.phase === 'count', 12, 0.1)")
        said = await ev("T.said()")
        check(all(x in said for x in ['tube ticks', 'thump, through concrete', 'drawer rolls out', 'heavy falls', 'line is dead']),
              'on the line: the room, the tube, a thump through concrete, a drawer, a box falling, paper; then dead')
        nn = [n for n in await notes() if 'The wall phone in the records room' in n]
        check(len(nn) == 1 and 'heard a call like this once before tonight' in nn[0], f'the note: heard before ({nn[0][:90] if nn else None}...)')
        lost = await ev("T.logAt('STATION CLOCK  WWVB REFERENCE LOST')")
        check(len(lost) == lost0 + 1, f'the console log loses WWVB again ({len(lost)} times tonight)')
        await tick(0.6)
        a = await arc()
        check(a['k3'] and a['beats'][:1] == ['relay'] and ('relay' in await ev("T.said()")), f'K3 through the wall at once (it was {"open" if k3_before else "closed"}; bank 3 out)')
        corner = await ev("T.corner()")
        check(a['clock'] and corner and corner.count(':') == 2 and corner.startswith(a['clock']['hm']), f'the corner clock in seconds ({corner})')
        # a paper open holds the 47, as it holds the clock
        t0 = (await arc())['t']
        await ev("S47.game.openNotebook()"); await tick(0.5, 4)
        t1 = (await arc())['t']
        await ev("S47.game.d.ui.close()"); await tick(0.05)
        check(abs(t1 - t0) < 1e-6, f'the notebook open holds the 47 ({t0:.2f} -> {t1:.2f})')
        await clear()
        await ev("S47.game.d.inter.get('recPhone').use()"); await tick(0.1)
        check('line is dead' in await ev("T.said()") and not await s2('called'), 'the wall phone in the 47: dead')
        # bank 3 back in, at the rack: the race (the player walks through the corridor; here placed)
        await ev("T.aim(4.6, -2.24, 5.34, 1.05, -2.24)")
        await ev("S47.game.d.inter.get('rack').use()"); await tick(0.2)
        check(await ev("S47.game.bankIn"), 'bank 3 put back in on the rack before :47')
        await ev("T.aim(-4.4, 9.4, -5.6, 0.9, 8.6)")
        await ev("T.until(() => S47.archive.t >= 16.2, 30, 0.1)")
        await ev("T.until(() => S47.archive.t >= 31.6, 30, 0.1)"); await tick(1.3)
        a = await arc()
        check(a['beats'][:3] == ['relay', 'beep', 'print'] and any('FROM EXT 201' in l for l in a['lines']), f'the beep at :16; at :31 the recorder prints again by itself: {[l for l in a["lines"] if "FROM" in l][:1]}')
        check(a['lines'].index(next(l for l in a['lines'] if 'FROM EXT 201' in l)) == len(a['lines']) - 1 and a['lines'][-2].endswith('EXT 201'),
              'at the bottom of the strip, under the call that just came: the first call of the night again')
        await ev("T.until(() => S47.archive.t >= 40.3, 30, 0.1)")
        a = await arc()
        check(a['tube'] == 'out' and 'tube' in a['beats'], 'at :40 the tired tube goes out')
        await shot('desk_02_at40')
        await clear()
        await ev("T.until(() => S47.archive.phase === 'moment', 10, 0.05)")
        a = await arc()
        check(a['phase'] == 'moment' and 'arc:done' in a['town'] and a['tip'] == 'tipping' and a['rogue'] == 27 and await ev("S47.ext.dishes.every((d) => d.tgtEl === 2 && d.tgtAz === 26)"),
              f'at :47 every dish is taken, down to el 2 on 026 ({a["rogue"]} rogue, at {a["t"]:.2f} s)')
        await pg.wait_for_timeout(2000); await tick(0.05)   # (the drives' bed comes up on the sound's own clock)
        bed = await ev("S47.arrayTip['bed'] ? S47.arrayTip['bed'].g.gain.value : 0")
        check(a['humming'] and bed > 0.05 and await ev("S47.saveBlock()"), f'the drives run (their bed at {bed:.2f}), and no save while they move')
        hum1 = await ev("T.bed()")
        db = lambda x: 20 * math.log10(max(x, 1e-9))
        print(f'levels: the records room\'s hum {db(hum0):.1f} dB, with the drives through the walls {db(hum1):.1f} dB')
        check(3 <= db(hum1) - db(hum0) <= 16, f'the drives through the walls stand over the room\'s hum ({db(hum1) - db(hum0):+.1f} dB)')
        await tick(0.05, 40)
        a = await arc()
        fb = a['box']
        check(abs(a['drawer'] - (-5.694)) < 0.01 and a['papers'] and fb[1] < 0.3 and abs(fb[3] - math.pi / 2) < 0.12, f'the drawer out of the wall, the box on its side on the floor ({fb}), its papers spilled')
        check(await ev("S47.player.colliders.some((c) => c.minX === -5.55 && c.maxZ === 8.82)") and abs(await ev("S47.annex.objs.fieldBox.position.y") - 0.2) < 0.01, 'and in the way, its hit area on the floor with it')
        said = await ev("T.said()")
        check('Every drive in the array has started at once' in said and 'drawer has rolled out of the wall' in said, 'captioned: the drives, the drawer, the box')
        await ev("T.aim(-4.3, 9.5, -5.55, 0.75, 8.6)"); await tick(0.1); await tick(0.1)
        await pg.wait_for_timeout(2500); await tick(0.05)
        await shot('desk_03_drawer_box')
        await ev("T.until(() => S47.archive.phase === 'done', 40, 0.2)")
        a = await arc()
        check(a['phase'] == 'done' and a['tip'] == 'down' and a['el'] == [2, 2] and a['az'] == [26, 26] and a['moving'] == 0 and not a['racing'],
              f'the dishes come to rest on 026, el 2, all of them ({a["el"]}); the moment is over')
        check(not await ev("S47.saveBlock()") and not a['humming'], 'the motors stop; it can be saved again')
        nn = await notes()
        check(any('Forty-seven seconds after the line went dead' in n and 'behind the STATION 01 box' in n for n in nn), 'the note: forty-seven seconds after, the drives, the drawer from the phone, the box')
        cv = await ev("({ landed: S47.curve.landed && S47.curve.landed.what, long: [...S47.curve.long] })")
        check(cv['landed'] == 'archive' and 'ch2' in cv['long'] and not (await arc())['pending'], f'the curve: it landed the peak and took chapter two\'s 47 ({cv})')
        logs = await ev("T.log()")
        solve = await ev("S47.game.ns.solveAt")
        check(len(await ev("T.logAt('SOLVE  RANGE NEGATIVE')")) == 2 and set(await ev("T.logAt('SOLVE  RANGE NEGATIVE')")) == {solve} and 'ARRAY  27 DRIVES  AZ 026 EL 02  NO COMMAND' in logs,
              'the console log: the solve again at its old time, the array at 026 / 02')
        seen = await ev("S47.seen.filter((e) => e.what === 'archive').map((e) => [e.kind, e.end || null])")
        check(['event', 'answered'] in seen and any(k == 'dishes' for k, _ in seen), f'the night\'s log: the event, answered; the array tipped ({seen})')

        # ---------- out there: the control room's windows ----------
        await ev("T.aim(0, -3.1, 0, 9, -80)"); await tick(0.3); await tick(0.1)
        await clear()
        await shot('desk_04_array_tipped')
        check('arc:tipseen' in (await arc())['town'] and any('tipped down to the horizon' in n for n in await notes()), 'seen through the windows: written down once')
        # the printer: the same solve, its old time
        await ev("T.aim(4.6, -0.4, 5.5, 0.9, -0.4)"); await tick(0.1)
        check(await ev("S47.game.d.inter.get('printer').label()") == 'Tear off the printout' and await ev("S47.room.printerPaper.visible"), 'a sheet hangs out of the printer')
        await shot('desk_05_printer')
        await ev("S47.game.d.inter.get('printer').use()"); await tick(0.1)
        txt = await ev("document.querySelector('.docview').textContent")
        hm = await ev(f"(() => {{ const c = {solve}; const d = ((c % 86400) + 86400) % 86400; return [Math.floor(d / 3600), Math.floor(d % 3600 / 60), Math.floor(d % 60)].map((v) => String(v).padStart(2, '0')).join(':'); }})()")
        check('-39 LY' in txt and hm in txt and 'again' in txt, f'torn off: the same solve, with its old time ({hm})')
        await pg.click('.docview [data-a=back]'); await tick(0.1)
        check(any('Nobody ran a solve' in n for n in await notes()) and not await ev("S47.room.printerPaper.visible"), 'and written down')
        # the strip
        await ev("T.aim(-1.6, 9.7, -1.12, 1.03, 10.22)"); await tick(0.1)
        check(await ev("S47.game.d.inter.get('arc:recorder').label()") == 'Call recorder', 'the call recorder can be read')
        await shot('desk_06_recorder')
        await ev("S47.game.d.inter.get('arc:recorder').use()"); await tick(0.1)
        txt = await ev("document.querySelector('.docview').textContent")
        check('FROM EXT 201' in txt and 'EXT 201 is the control room' in txt, 'the strip: the first call again, from EXT 201 to EXT 201; the card says 201 is the control room')
        await shot('desk_07_strip')
        await pg.click('.docview [data-a=back]'); await tick(0.1)
        check(any('The control room called itself' in n for n in await notes()), 'written down: the control room called itself')
        # the drawer and the reel
        await ev("T.aim(-4.6, 8.6, -5.8, 1.22, 8.6)"); await tick(0.1)
        lab = await ev("(() => { const it = S47.game.d.inter.update(S47.camera); return it ? it.id + ':' + it.label() : null; })()")
        check(lab == 'arc:drawer:Take reel 047', f'the drawer offers the reel ({lab})')
        await ev("S47.input.zoomHeld = true"); await tick(0.8)
        hud = await ev("T.hud()")
        check(hud and 'fire drawer' in hud and '047' in hud, f'a closer look: {hud}')
        await ev("S47.input.zoomHeld = false"); await tick(0.4)
        await clear()
        await shot('desk_08_drawer_reel')
        await ev("S47.game.d.inter.get('arc:drawer').use()"); await tick(0.1)
        kit = await ev("S47.kitItems().map((k) => k.name + '|' + (k.state || ''))")
        check('Reel 047|10-AUG-79' in kit and not (await arc())['reelShown'] and any('NOT FOR CATALOGUE' in n for n in await notes()), f'reel 047 into the kit ({[k for k in kit if "Reel" in k]})')

        # ---------- kept with the case ----------
        await ev("S47.saveNow('manual', 0)")
        await pg.wait_for_timeout(600)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev(HELPERS); await ev("T.hook(); S47.hold = true; S47.curve.free = true; S47.tick(0.5); T.quiet()")
        await pg.add_style_tag(content='.lockhint{display:none !important}')
        a = await arc()
        check(a['phase'] == 'done' and abs(a['drawer'] + 5.694) < 0.01 and a['papers'] and abs(a['box'][3] - math.pi / 2) < 0.01 and not a['reelShown'],
              'after Continue: the drawer out, the box on the floor, the papers, the reel gone from the drawer')
        check(a['tip'] == 'down' and a['rogue'] == 27 and a['el'] == [2, 2] and a['az'] == [26, 26] and a['moving'] == 0, f'and the dishes exactly as they were saved, at rest on 026 / 02 ({a["el"]})')
        check(any('FROM EXT 201' in l for l in a['lines']) and 'Reel 047|10-AUG-79' in await ev("S47.kitItems().map((k) => k.name + '|' + (k.state || ''))"), 'the strip and the kit as they were')
        await ev("T.aim(-2.3, 8.6, -3.65, 0.8, 8.6)")
        await tick(1, 60)
        check((await arc())['phase'] == 'done' and len([n for n in await notes() if 'The wall phone in the records room' in n]) == 1, '60 s in the records room after Continue: it never rings again, one note')
        # one owner for the dishes: a warning out in the field while they are tipped does not take them
        # (story/dishesFollow.ts begins only when nothing else has them)
        A = "S47.abduction()"
        await ev(f"S47.setAbductions(true); S47.player.place(-1, -62, Math.PI); S47.player.pitch = 0.3; S47.tick(0.3); {A}.data.last = -1e9; {A}.pressure = 5; S47.tick(0.05)")
        warned = await ev(f"{A}.stage") == 'warning'
        if warned: await ev(f"{A}['warnLen'] = 47")
        await tick(0.5, 46)
        f = await ev("({ mode: S47.follow.mode, tip: S47.arrayTip.mode, el: [Math.min(...S47.ext.dishes.map((d) => d.curEl)), Math.max(...S47.ext.dishes.map((d) => d.curEl))] })")
        check(warned and f['mode'] == 'idle' and f['tip'] == 'down' and f['el'] == [2, 2], f'a warning in the field while the array is tipped: the warning\'s dishes do not take them ({f})')
        await ev("S47.input.keys.clear(); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
        await ev("T.until(() => S47.abduction().stage === 'calm', 20, 0.25)")
        await ev("T.quiet()")

        # ---------- reel 047 on the deck ----------
        await ev("T.aim(4.6, -1.55, 5.45, 1.2, -1.55)")
        check(await ev("S47.game.d.inter.get('decoder').label()") == 'Thread reel 047 on the deck', 'the signal processor offers reel 047')
        await clear()
        await ev("S47.game.d.inter.get('decoder').use()"); await tick(0.1)
        check(await ev("S47.reel047.playing") and await ev("S47.oneAtATime()") and await ev("S47.saveBlock()"), 'it plays: nothing else starts, no save while it runs')
        await ev("T.until(() => S47.reel047.t >= 4, 10, 0.1)")
        await ev("T.until(() => S47.reel047.t >= 19.6, 20, 0.1)")
        hole = await ev("({ state: S47.reel047.state, gate: S47.reel047.gate })")
        await ev("T.aim(4.75, -1.62, 5.466, 0.85, -1.627)"); await tick(0.2)
        await shot('desk_09_reel_nothing')
        said = await ev("T.said()")
        check(all(x in said for x in ['pulse group', 'field telephone', 'cutters', 'Not even the hiss']), 'on the reel: the pulse group, a crank, a cut, then nothing')
        check(hole['state'] == 'NO SIGNAL' and hole['gate'] == 0, f'in the hole the tape has nothing on it, not even the hiss ({hole})')
        r0 = await ev("S47.reel047.heard.length")
        await tick(1, 30)
        check(await ev("S47.reel047.heard.length") == r0 and (await arc())['tip'] == 'down', 'thirty seconds into the 47: still nothing on it, the dishes still down')
        await ev("T.until(() => S47.reel047.heard.includes('relay'), 30, 0.1)")
        hole_len = await ev("(() => { const p = S47.reel047; return p.t; })()")
        a = await arc()
        check(a['tip'] == 'raising' and 'relay, loud and close' in await ev("T.said()"), f'the relay on the tape at {hole_len:.1f} s, and out in the array every drive starts at once')
        await ev("T.aim(0, -3.1, 0, 9, -80)"); await tick(9); await tick(0.1)
        rise = await arc()
        await clear()
        await shot('desk_10_array_rising')
        check(rise['tip'] == 'raising' and 8 < rise['el'][0] < 30 and rise['el'][0] == rise['el'][1], f'all of them together on the way up ({rise["el"]})')
        await ev("T.aim(4.95, -1.627, 5.466, 0.95, -1.627)"); await tick(0.1)
        await ev("T.until(() => !S47.reel047.playing, 20, 0.1)"); await tick(0.3)
        check(await ev("S47.reel047.state") == 'RANGE' and '-32 LY' in await ev("S47.archive.shown.join(' ')"), 'the display: SOURCE DISTANCE -32 LY')
        await clear()
        await ev("S47.input.zoomHeld = true"); await tick(0.8); await tick(0.1)
        await shot('desk_11_display_32')
        await ev("S47.input.zoomHeld = false"); await tick(0.4)
        await ev("T.aim(0, -3.1, 0, 9, -80)")
        await ev("T.until(() => S47.arrayTip.mode === 'idle', 40, 0.2)")
        a = await arc()
        exact = await ev("S47.ext.dishes.every((d) => d.curAz === 26 && d.curEl === 32 && d.tgtAz === 26 && d.tgtEl === 32 && !d.rogue)")
        check(a['tip'] == 'idle' and exact and 'arc:up' in a['town'] and 'arc:upseen' in a['town'], 'back up, exactly 026 / 32, every one given back; seen going')
        nn = await notes()
        check(any('Reel 047, 10 August 1979' in n and 'went back up' in n for n in nn) and 'arc:heard' in a['town'], 'the note: what is on the reel, and the array going back up at the relay')
        check(len(await ev("S47.archive.books()")) == 1 and await ev("S47.game.extraBooks().some((b) => b.key === 'ch2' && b.o.cards().some((c) => c.id === 'r47'))"), 'reel 047 is on the case board')
        await ev("S47.game.d.ui.close()")
        # ---------- the key holder, at last ----------
        await ev("T.aim(-1.75, 9.25, -1.12, 1.42, 9.25)")
        check(await ev("S47.game.d.inter.get('recPhone').label()") == 'Call the key holder', 'the wall phone: call the key holder')
        await ev("S47.game.d.inter.get('recPhone').use()")
        await ev("T.until(() => S47.ch2.s.called, 40, 0.25)"); await tick(1.5)
        check(await s2('called') and any('OUT  TRK1  EXT 214  555 0119' in l for l in (await arc())['lines']), 'the call to N. Vega goes as before, and the recorder puts it down')
        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'desktop: no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:6])))
        await ctx.close()

        # ======================= phone, touch: the handset first, bank 3 left out, away unseen =======================
        errs2 = []
        ctx2, pp = await page(b, 412, 892, True, errs2)
        ev = pp.evaluate
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def shot(name): await pp.screenshot(path=f'{OUT}/{name}.png')
        arc = lambda: ev("T.arc()")
        await ev("S47.hold = true; S47.curve.free = true; S47.jump('chapter2'); S47.tick(1.5); T.quiet()")
        check(await ev("S47.input.touchMode"), 'phone: touch controls')
        # (and without the bought sounds: the game's own stand in, Audio.or)
        nlic = await ev("(() => { let n = 0; for (const k of Object.keys(S47.audio.buf)) if (k.startsWith('lic:')) { delete S47.audio.buf[k]; n++; } return n; })()")
        await ev("S47.game.d.inter.get('phone').use()"); await tick(1, 12)
        await ev("S47.game.ns.k3Open = true")   # (bank 3 out, and left out)
        await ev("Object.assign(S47.ch2.s, { read: { original: true, amended: true, lineage: true, index: true }, p04: true, p05: true, stage: 'call-nora' })")
        # straight to the phone, before the steps can come: tapped with the Use button. The line is dead,
        # and the steps come now: the build is always before the peak
        await ev("T.aim(-1.75, 9.25, -1.12, 1.42, 9.25)"); await tick(0.1)
        check(await ev("(() => { const it = S47.game.d.inter.update(S47.camera); return it && it.id; })()") == 'recPhone', 'phone: aimed at the wall phone')
        await ev("S47.game.d.ui.clearToasts(); T.toasts.length = 0"); await tick(0.05)
        await pp.tap('.tbtn.use'); await tick(0.1)
        a = await arc()
        check(a['phase'] == 'idle' and 'line is dead' in await ev("T.said()") and await ev("S47.roofSteps.busy") and not await ev("S47.ch2.s.called"),
              'phone: straight to the phone from the board, before the steps: the line is dead, and the steps come now')
        await ev("T.until(() => S47.roofSteps.phase === 'done', 45, 0.1)")
        await ev("S47.game.d.ui.clearToasts(); T.toasts.length = 0"); await tick(0.5)
        await pp.tap('.tbtn.use'); await tick(0.1)
        a = await arc()
        check(a['phase'] == 'line' and a['how'] == 'lift' and 'No dial tone' in await ev("T.said()") and any('IN   TRK1  EXT 201' in l for l in a['lines']),
              'phone: lifted again after them: no dial tone, the line already open (and the recorder has it)')
        await pp.wait_for_timeout(1200); await tick(1.2); await pp.wait_for_timeout(600); await tick(0.05)
        await shot('phone_01_line')
        await ev("T.until(() => S47.archive.phase === 'count', 14, 0.1)")
        check(not await ev("S47.roofSteps.busy"), 'phone: the steps do not come in the 47')
        await ev("T.until(() => S47.archive.t >= 46.5, 60, 0.25)")
        await ev("T.aim(-4.3, 9.5, -5.55, 0.75, 8.6)"); await tick(0.05)
        await ev("S47.game.d.ui.clearToasts(); T.toasts.length = 0")
        await ev("T.until(() => S47.archive.phase === 'moment', 5, 0.05)"); await tick(0.05, 40)
        await pp.wait_for_timeout(2500); await tick(0.05)
        await shot('phone_02_drawer_box')
        a = await arc()
        check(a['papers'] and a['box'][1] < 0.3 and 'arc:out' in a['town'], 'phone: the drawer, the box on the floor; bank 3 was out')
        await ev("T.until(() => S47.archive.phase === 'done', 40, 0.2)")
        # standing in the records room after it: the steps do not come again (they were its build)
        await ev("T.aim(-2.3, 8.6, -3.65, 0.8, 8.6)"); n0 = await ev("S47.roofSteps.log.length")
        await tick(0.5, 100)
        check(await ev("S47.roofSteps.phase") == 'done' and await ev("S47.roofSteps.log.length") == n0, 'phone: 50 s in the records room after it, nothing more on the roof')
        await ev("T.aim(4.6, -0.4, 5.5, 0.9, -0.4)")
        await ev("S47.game.d.inter.get('printer').use()"); await tick(0.1)
        txt = await ev("document.querySelector('.docview').textContent")
        check('blank sheet' in txt and 'Nothing is printed on it' in txt and len(await ev("T.logAt('SOLVE  RANGE NEGATIVE')")) == 1, 'phone: bank 3 left out: a blank sheet, and no solve again in the log')
        await shot('phone_03_blank')
        await pp.tap('.docview [data-a=back]'); await tick(0.1)
        await ev("T.aim(0, -3.1, 0, 9, -80)"); await tick(0.3); await tick(0.1)
        await ev("S47.game.d.ui.clearToasts()"); await tick(0.05)
        await shot('phone_04_array_tipped')
        # nobody at SARO with the dishes down: they go back unseen
        await ev("window.__p = S47.world.prepare('diner')")
        for _ in range(150):
            if await ev("!!S47.world.diner"): break
            await pp.wait_for_timeout(200)
        await ev("S47.world.enter('diner'); S47.tick(0.3)")
        a = await arc()
        exact = await ev("S47.ext.dishes.every((d) => d.curAz === 26 && d.curEl === 32 && !d.rogue)")
        check(a['tip'] == 'idle' and exact and 'arc:unseen' in a['town'], 'phone: away from SARO, the dishes go back to exactly 026 / 32, unseen')
        await ev("S47.world.enter('saro'); T.aim(0, -3.1, 0, 9, -80)"); await tick(0.5)
        check(any('I did not see them go' in n for n in await ev("S47.game.notes")), 'phone: back at SARO, the first look at them says so')
        noise = [e for e in errs2 if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, f'phone (without the {nlic} bought sounds): no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:6])))
        await ctx2.close()

        json.dump({'checks': checks}, open(f'{OUT}/f_archive.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
