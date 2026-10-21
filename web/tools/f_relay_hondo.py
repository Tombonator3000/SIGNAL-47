# The relay goes first, the yard holds its breath, and 02:29: SARO goes dark and Hondo writes
# (FORSLAG.md, 20 October: small changes 1 and 2, item 4; KAPITLER.md, The Second Exposure, new 1).
#   - the relay goes first (story/Prologue.ts, relayFirst): with the survey left to run, K3 trips by
#     itself with its clack, the toast says so; forty seconds later (on the log's clock) the lights
#     dip for most of a second, the note says the service record said so; then the skip to 02:13.
#     The lever puts the bank back; left out, the skip throws it back on (logged)
#   - the yard holds its breath (core/ambience.ts, breath): the first time out in chapter one, no
#     crickets and no wind, then the crickets one at a time over 47 s; a caption; the night's curve
#     runs its plain ten minutes through it (nothing waits); never again (not the second time out)
#   - 02:29, SARO dark (story/saroDark.ts):
#       not before 02:29, not from the control room before 02:36, not after 02:40, not in chapter two;
#       not on top of another of the night's things (it waits for it); the curve keeps chapter one's
#       47 for it until then (story/curve.ts)
#       from the lab: K3 trips in the empty control room (or, already open, the log writes it again),
#       the corner clock in seconds, a caption; at :31 a sag; at :40 every light that is SARO's out
#       (the lab's safelights and their glass, the yard's and the array's lamps, the red lamps on all
#       27 antennas, the control room), the hum and the room tone gone, the night quiet a breath later;
#       the highway's lamps and the motel's sign still lit; no saving, nothing else starts; the wet
#       bench and the B-12 control wait for the light; the curve lands on it as chapter one's 47
#       the figure: out of the middle of the view where the eyes can reach (the windows side, from
#       the walk or the control room's windows), stays while looked at, gone with a closer look, and
#       not there when the light comes back; written in the night's log
#       (critic, 20 October) the unlit paint (the stencils, the fence, the lettering) dims with the
#       lamps and the S-03 motor bus's glass goes dark, all exactly back after; in the lab the
#       doorway is captioned, and from it the figure stands up on the control room's roof against
#       the stars; the note says only what the lab saw; a chapter two before 02:29 gets no telex yet
#       the light: everything back at once, the sodium lamps dim and up in half a minute, then
#       exactly as before; the telex runs, Hondo Valley's message on the roll (the words of
#       KAPITLER.md), the console log lines, the notes; K3 stays open; once; saved; after Continue
#       it is on the roll and never comes again
#   - desktop 1280x720 and phone 412x892 with touch; no console errors or warnings
# Usage (from web/): python3 tools/f_relay_hondo.py OUTDIR
import asyncio, sys, os, json, math
from intro_flow import begin_opening
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

D = "S47.saroDark"
HELPERS = """(() => {
  window.T = {
    // stand at (x, z) and look at a point (the eye is the camera's after a tick)
    aim(x, z, tx, ty, tz) {
      const pl = S47.player; pl.place(x, z, 0); S47.tick(0.05);
      const c = S47.camera.position;
      pl.place(x, z, Math.atan2(-(tx - c.x), -(tz - c.z)));
      pl.pitch = Math.atan2(ty - c.y, Math.hypot(tx - c.x, tz - c.z)); S47.tick(0.05);
      return pl.walkable(x, z);
    },
    look(tx, ty, tz) {
      const pl = S47.player, c = S47.camera.position;
      pl.yaw = Math.atan2(-(tx - c.x), -(tz - c.z)); pl.pitch = Math.atan2(ty - c.y, Math.hypot(tx - c.x, tz - c.z)); S47.tick(0.02);
      return true;
    },
    toasts: [],
    hook() {
      const ui = S47.game.d.ui; if (ui.__hooked) return; ui.__hooked = true;
      const o = ui.toast.bind(ui); ui.toast = (t, s, c) => { T.toasts.push(t); return o(t, s, c); };
    },
    said: (s) => T.toasts.some((t) => t.includes(s)),
    log: () => S47.game.ns.log.map((l) => l[1]),
    clock: () => { const e = document.querySelector('.clockline'); return e ? { text: e.textContent, secs: e.classList.contains('secs') } : null; },
    dark: () => ({ phase: S47.saroDark.phase, t: S47.saroDark.t, out: S47.saroPower.out, inner: S47.saroPower.inner, sodium: S47.saroPower.sodium,
      wasOpen: S47.saroDark.wasOpen, fig: S47.saroDark.figure && { ...S47.saroDark.figure }, up: !!S47.saroDark.figureAt, adapt: S47.saroDark.adapt }),
    // the lamps: SARO's slots in the site's floods (as the cut took them), the highway's, the lab's
    lamps() {
      const site = S47.floodSets.find((s) => s.key === 'site'), snap = S47.saroPower['snap'];
      const hw = []; for (let i = 0; i < site.count; i++) { const p = site.pos[i]; if (Math.abs(p.x + 20) < 1.3 && p.z < 300 && p.y > 0) hw.push(p.w); }
      const beacons = S47.ext.dishArray.beaconGlow.points.geometry.getAttribute('aCol');
      let bmax = 0; for (let i = 0; i < beacons.count; i++) bmax = Math.max(bmax, beacons.getX(i));
      return { saro: snap ? snap.slots.map((s) => site.pos[s.i].w) : null, base: snap ? snap.slots.map((s) => s.w) : null, slots: snap ? snap.slots.length : 0,
        highway: hw, lab: S47.yard.labFlood.scale.value, red: Math.max(...S47.yard.redGlass.map((m) => m.color.r)), beacons: beacons.count, bmax,
        ceiling: S47.room.lights.ceiling.map((l) => l.intensity), tubeOn: S47.room.tubes.filter((t) => t.material.type === 'MeshBasicMaterial').length,
        bus: S47.yard.screens[0].color.r, sight: S47.yard.paint[1].color.r, fence: S47.ext.paint.map((m) => +m.color.r.toFixed(4)), vend: S47.annex.screens[0].color.r };
    },
    // the yard's own lamp posts (the floods at their heads): the w of each, from the site set
    yardLamps() { const site = S47.floodSets.find((s) => s.key === 'site'); const out = []; for (let i = 0; i < site.count; i++) { const p = site.pos[i]; if (p.x > 6 && p.x < 13 && p.z > -10 && p.z < 12 && p.y > 3) out.push(+p.w.toFixed(3)); } return out; },
    seenOf: (kind, what) => S47.seen.filter((e) => e.kind === kind && (what === undefined || e.what === what)).map((e) => ({ ...e })),
  };
  // the loudest 50 ms of what one call plays on the sfx bus, K-weighted roughly (as tools/horror_roof.py)
  const a = S47.audio, ctx = a.ctx;
  T.peak = async (fn, secs = 0.7) => {
    const inp = ctx.createGain(), hp = ctx.createBiquadFilter(), sh = ctx.createBiquadFilter(), an = ctx.createAnalyser(), z = ctx.createGain();
    hp.type = 'highpass'; hp.frequency.value = 38; sh.type = 'highshelf'; sh.frequency.value = 1700; sh.gain.value = 4;
    an.fftSize = 2048; z.gain.value = 0; inp.connect(hp); hp.connect(sh); sh.connect(an); an.connect(z); z.connect(ctx.destination);
    const keep = a.sfx; a.sfx = inp; try { fn(); } finally { a.sfx = keep; }
    const buf = new Float32Array(an.fftSize); let best = 0; const t0 = performance.now();
    while (performance.now() - t0 < secs * 1000) {
      await new Promise((r) => setTimeout(r, 15));
      an.getFloatTimeDomainData(buf); let s = 0; for (const v of buf) s += v * v; best = Math.max(best, Math.sqrt(s / buf.length));
    }
    inp.disconnect(); return best;
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
    if touch: await pg.tap('button[data-a=start]'); await begin_opening(pg, touch=True)
    else: await pg.click('button[data-a=start]'); await begin_opening(pg)
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS)
    await pg.evaluate("T.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    return ctx, pg

LAB = (15.2, -3.6, 15.0, 1.6, -5.6)            # at the wet bench, under the safelight
ROOM = (0.4, -2.6, 0.0, 1.2, -9.0)             # at the console, facing the windows

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=ARGS)
        # ======================= desktop =======================
        errs = []
        ctx, pg = await page(b, 1280, 720, False, errs)
        ev = pg.evaluate
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def shot(name):
            await pg.wait_for_timeout(120)
            await pg.screenshot(path=f'{OUT}/{name}.png')
        async def until(js, secs, step=0.25):
            t = 0.0
            while t < secs:
                if await ev(js): return t
                await tick(step); t += step
            return None
        aim = lambda v: ev(f"T.aim({v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]})")
        await ev("S47.hold = true; S47.nightEvents['wait'] = 1e9")

        # ---------- the relay goes first (the prologue, before the skip) ----------
        async def to_survey(clock):
            await ev(f"""(() => {{ S47.jump('shift'); S47.tick(0.2); const g = S47.game; g.d.ui.close();
              g.powerUp(true); g.rx.advance(); g.rx.advance(); g.phase = 'survey'; g.clock = {clock}; T.toasts.length = 0;
              S47.player.place(4.6, -1.4, Math.PI / 2); g['consoleClosed'](); return true; }})()""")
        await to_survey(23 * 3600 + 51 * 60 + 30)
        log0 = await ev("T.log()")
        t1 = await until("S47.game.ns.k3Open", 12, 0.1)
        log1 = await ev("T.log()")
        check(t1 is not None and 6.5 <= t1 <= 7.5 and log1.count('RX BANK 3  K3 OPEN') == log0.count('RX BANK 3  K3 OPEN') + 1 and await ev("S47.game.phase") == 'survey',
              f'the survey left to run: {t1} s later K3 trips by itself, in the log')
        await tick(0.6)
        check(await ev("T.said('A clack from the rack. Bank 3 has dropped out again.')") and await ev("S47.game.d.inter.get('rack').label()") == 'Reset RX bank 3',
              'the clack is captioned, and the rack offers the lever')
        await ev("S47.game.d.inter.get('rack').use()"); await tick(0.5)
        check(not await ev("S47.game.ns.k3Open") and 'RX BANK 3  K3 CLOSED' in (await ev("T.log()"))[-2:], 'the lever puts bank 3 back on')
        base = await ev("S47.room.lights.ceiling.map((l) => l.intensity)")
        t2 = await until("S47.game.ns.log.some((l) => l[1].startsWith('LINE POWER  DIP'))", 45, 0.1)
        await tick(0.15)
        dipped = await ev("S47.room.lights.ceiling.map((l) => l.intensity)")
        await shot('desk_a1_dip')
        lines = await ev("S47.game.ns.log.map((l) => [l[0], l[1]])")
        k3at = [c for c, t in lines if t == 'RX BANK 3  K3 OPEN'][-1]
        dipat = [c for c, t in lines if t.startswith('LINE POWER  DIP')][-1]
        check(t2 is not None and dipat - k3at == 40 and all(d < 0.4 * b0 for d, b0 in zip(dipped, base)),
              f'forty seconds after the trip (log {k3at} to {dipat}) the lights dip ({[round(x, 2) for x in dipped]} of {[round(x, 2) for x in base]})')
        await tick(0.5, 3)
        after = await ev("S47.room.lights.ceiling.map((l) => l.intensity)")
        notes = await ev("S47.game.notes.slice()")
        check(all(abs(a - b0) < 1e-6 for a, b0 in zip(after, base)) and any('Forty seconds later the lights dipped' in n and 'it trips before, not after' in n for n in notes)
              and await ev("T.said('The lights dip. Forty seconds after the relay.')"), 'and come back; the note and the caption tie it to the service record')
        t3 = await until("S47.game.phase === 'residual'", 12, 0.25)
        check(t3 is not None and abs(await ev("S47.game.clock % 86400") - (2 * 3600 + 13 * 60 + 41)) < 6, 'then the night skips to 02:13 as before')
        # left out: the skip throws it back on
        await to_survey(23 * 3600 + 52 * 60)
        await until("S47.game.ns.k3Open", 12, 0.2)
        await until("S47.game.phase === 'residual'", 60, 0.5)
        lines = await ev("S47.game.ns.log.map((l) => l[1])")
        check(not await ev("S47.game.ns.k3Open") and lines.index('RX BANK 3  K3 CLOSED', lines.index('LINE POWER  DIP 0.7 S')) > 0 and await ev("S47.room.crtCenter.powered"),
              'left out, bank 3 is back on after the skip (and logged)')

        # ---------- the yard holds its breath (chapter one, first time out) ----------
        await ev("S47.jump('chapter1'); S47.tick(1.0); S47.nightEvents['wait'] = 1e9; S47.game.d.ui.close(); T.toasts.length = 0")
        check(await ev("S47.game.phase") == 'ch1' and not await ev("S47.game.ns.jacket") and await ev("S47.audio.night.breathing") is None, 'chapter one: still in the control room, the night as it is')
        c0 = await ev("({ phase: S47.curve.phase, t: S47.curve.t })")
        await ev("T.aim(8.2, 1.7, 9.0, 1.2, -12)"); await tick(0.1)
        br = await ev("S47.audio.night.breathing")
        check(br and br['back'] == 0 and await ev("S47.audio.night.cricketsNow") == 0 and await ev("S47.game.ns.jacket"), f'first time out: the yard holds its breath, not a cricket ({br})')
        await tick(0.5, 7)
        check(await ev("T.said('Not a sound out here. Not even the crickets.')"), 'captioned, for a phone with the sound down')
        backs = []
        for k in range(10):
            await tick(0.5, 10)
            backs.append(await ev("S47.audio.night.breathing ? S47.audio.night.breathing.back : -1"))
        c1 = await ev("({ phase: S47.curve.phase, t: S47.curve.t })")
        steps = [x for x in backs if x >= 0]
        check(steps == sorted(steps) and len(set(steps)) >= 3 and backs[-1] == -1 and await ev("S47.audio.night.cricketsNow") >= 2,
              f'the crickets come back one at a time over 47 s ({backs})')
        check(c0['phase'] == 'plain' and c1['phase'] == 'plain' and c1['t'] - c0['t'] > 45 and not await ev("S47.oneAtATime()"),
              f'the plain night of the curve runs through it: not an event, nothing waits ({c0} to {c1})')
        await ev("T.aim(3.4, 1.4, 0, 1.2, -3)"); await tick(0.5, 4)
        await ev("T.aim(8.2, 1.7, 9.0, 1.2, -12)"); await tick(0.5)
        check(await ev("S47.audio.night.breathing") is None, 'out again: not again')

        # ---------- 02:29: not after 02:40, not yet, not from the control room; the curve keeps the 47 ----------
        await ev("S47.game.clock = 2 * 3600 + 40 * 60 + 5")
        await aim(LAB)
        await tick(0.5, 30)
        check(await ev(f"{D}.phase") == 'idle' and not await ev("T.seenOf('event', 'blackout').length"), 'in the lab after 02:40: it does not come')
        fb = await ev("({ hondo: S47.game.ns.hondo, log: S47.game.ns.log.filter((l) => l[1] === 'TELEX RECEIVED  HONDO VALLEY'), roll: S47.room['telexMore'] })")
        check(fb['hondo'] == 2 * 3600 + 29 * 60 and len(fb['log']) == 1 and fb['log'][0][0] == 2 * 3600 + 29 * 60 + 14 and 'FM HONDO VALLEY RADIO STN' in fb['roll'],
              'a night that never went dark still has Hondo Valley\'s telex on the roll, printed at 02:29 with nobody there')
        # (back to a night where it may still come)
        await ev("""(() => { const g = S47.game; g.ns.hondo = undefined; g.ns.log = g.ns.log.filter((l) => l[1] !== 'TELEX RECEIVED  HONDO VALLEY'); S47.room.setTelexRoll([]); return true; })()""")
        await ev("S47.game.clock = 2 * 3600 + 28 * 60 + 35")
        await aim(LAB)
        await tick(0.5, 30)
        check(await ev(f"{D}.phase") == 'idle' and await ev("S47.darkWhere()") == 'lab', 'in the lab before 02:29: nothing')
        await ev("S47.curve.force('build', 0.7)")
        may = await ev("({ long: S47.curve.may('long'), event: S47.curve.may('event') })")
        check(not may['long'] and may['event'], f'the curve keeps chapter one\'s 47 for it meanwhile ({may})')
        await ev("S47.curve.force('plain')")
        # heard in the lab through the wall: the clack about as loud as a step of one's own, the
        # teleprinter from the yard by the east door, quieter but there
        own = [await ev("T.peak(() => S47.audio.step('tile'))") for _ in range(3)]
        clack = [await ev("T.peak(() => S47.audio.relayClack(S47.game.rackAt, true))") for _ in range(2)]
        await ev("T.aim(7.4, 1.6, 0, 1.2, 1.6)")
        bar = [await ev("T.peak(() => S47.audio.typeBar(S47.game.telexAt))") for _ in range(3)]
        own2 = [await ev("T.peak(() => S47.audio.step('concrete'))") for _ in range(3)]
        await aim(LAB)
        dbv = lambda v: 20 * math.log10(max(sum(v) / len(v), 1e-9))
        c_db, o_db, b_db, o2_db = dbv(clack), dbv(own), dbv(bar), dbv(own2)
        print(f'levels: clack in the lab {c_db:.1f} dB, own step there {o_db:.1f} dB; a telex strike from the east door {b_db:.1f} dB, own step there {o2_db:.1f} dB')
        check(c_db - o_db > -4 and b_db - o2_db > -20, f'the clack through the wall is about as loud as a step ({c_db - o_db:+.1f} dB); the teleprinter carries out of the door ({b_db - o2_db:+.1f} dB)')
        await aim(ROOM)
        await tick(0.5, 40)
        check(await ev(f"{D}.phase") == 'idle' and await ev("S47.darkWhere()") == 'room' and await ev("S47.game.clock % 86400") > 2 * 3600 + 29 * 60,
              '02:29 in the control room itself: nothing (the room must stand empty)')
        # one at a time: something of the night's is going (steps behind, begun in the room), it waits for it
        ok = await ev("S47.nightEvents.now('steps', S47.nightIn())")
        await aim(LAB)
        await tick(0.25)
        busy = await ev("S47.oneAtATime()")
        await tick(0.25, 4)
        waited = await ev(f"{D}.phase") == 'idle'
        t4 = await until(f"{D}.phase !== 'idle'", 8, 0.1)
        check(ok and busy and waited and t4 is not None and not await ev("S47.nightEvents.running"), f'another of the night\'s things going (steps behind): it waits until that is over (then {t4} s)')

        # ---------- the relay, from the lab ----------
        st = await ev("T.dark()")
        clack_clock = await ev(f"{D}.at")
        hhmm = await ev(f"(() => {{ const c = {D}.at % 86400; return String(Math.floor(c / 3600)).padStart(2, '0') + String(Math.floor(c % 3600 / 60)).padStart(2, '0'); }})()")
        lines = await ev("T.log()")
        await tick(0.6)
        ck = await ev("T.clock()")
        seen = await ev("T.seenOf('event', 'blackout')")
        check(st['phase'] == 'relay' and not st['wasOpen'] and await ev("S47.game.ns.k3Open") and lines[-1] == 'RX BANK 3  K3 OPEN',
              'in the lab after 02:29: K3 trips in the empty control room, in the log')
        check(await ev("T.said('Through the wall, in the empty control room: the relay.')") and ck and ck['secs'] and ck['text'].replace(':', '').startswith(hhmm) and len(ck['text']) == 8,
              f"captioned; the corner clock goes over to seconds ({ck})")
        check(len(seen) == 1 and seen[0]['place'] == 'Photo lab' and await ev("S47.curve.landed && S47.curve.landed.what") == 'blackout' and await ev("S47.curve.long.has('ch1')"),
              'written in the night\'s log; the curve lands on it as chapter one\'s 47')
        check(await ev("S47.saveBlock()") == 'Not right now. Let the moment finish first.' and await ev("S47.oneAtATime()"), 'no saving, and nothing else starts while it runs')
        await shot('desk_c1_relay_lab')
        pre = await ev("T.lamps()")
        await until(f"{D}.t >= 31.0", 40, 0.25)
        sag = await ev("({ out: S47.saroPower.out, inner: S47.saroPower.inner, hum: S47.audio.humHeld })")
        await tick(0.5)
        check(sag['out'] and sag['inner'] < 0.5 and sag['hum'] < 0.5 and not await ev("S47.saroPower.out") and await ev(f"{D}.phase") == 'relay', f'at :31 everything sags for a moment ({sag})')
        await until(f"{D}.phase === 'dark'", 12, 0.1)
        await tick(0.1)
        L = await ev("T.lamps()")
        check(L['slots'] >= 12 and all(w == 0 for w in L['saro']) and all(w > 0 for w in L['base'][:4]) and L['lab'] == 0 and L['red'] < 0.02 and L['beacons'] == 27 and L['bmax'] == 0
              and all(c == 0 for c in L['ceiling']) and L['tubeOn'] == 0,
              f"at :40 every light that is SARO's goes out: {L['slots']} lamps outside, the lab's, the safelights' glass, the 27 red lamps, the control room")
        check(len(L['highway']) >= 3 and all(w > 0 for w in L['highway']), f"the highway's lamps stay lit ({L['highway']})")
        check(L['bus'] < 0.05 * pre['bus'] and L['vend'] < 0.05 * pre['vend'] and abs(L['sight'] - 0.36 * pre['sight']) < 0.01 and all(abs(a - 0.36 * b0) < 0.01 for a, b0 in zip(L['fence'], pre['fence'])),
              f"the motor bus's glass and the vending machine go dark, the stencils and the fence dim ({L['bus']:.3f}, {L['sight']:.3f} of {pre['sight']:.3f})")
        await tick(0.5, 4); await pg.wait_for_timeout(500)
        snd = await ev("({ hum: S47.audio.humHeld, tone: S47.audio.roomToneHeld })")
        check(snd['hum'] == 0 and snd['tone'] < 0.1 and 'LINE POWER  FAIL' in await ev("T.log()") and await ev("T.said('The safelight dies, and the hum with it. Black.')"),
              f'the hum and the room tone die with it, the log has it, the caption ({snd})')
        await shot('desk_c2_dark_lab')
        # the wet bench and the B-12 control wait for the light
        dev = await ev("S47.ch1.s.dev1")
        await ev("S47.game.d.inter.get('wetBench').use()"); await tick(0.1)
        bench = await ev("T.said('Too dark to work at the bench.') && !S47.game.d.ui.modal")
        await ev("S47.game.d.inter.get('b12Control').use()"); await tick(0.1)
        check(bench and await ev("T.said('The B-12 control is dead. No power.')") and await ev("S47.ch1.s.dev1") == dev, 'the wet bench and the B-12 control wait for the light (nothing changed)')
        await ev("S47.game.d.inter.get('rack').use()"); await tick(0.1)
        check(await ev("S47.game.ns.k3Open") and await ev("T.said('The rack is dead.')"), 'and the rack is dead (K3 cannot be put back in the dark)')

        # ---------- the figure, from the walk ----------
        await ev("T.aim(9.4, -9.0, 30, 1.2, -9)"); await tick(0.25, 4)
        # (out on the ground only from 12 s into the dark: a roof first if one comes into view)
        await until(f"!!{D}.figureAt", 14, 0.25)
        f = await ev("T.dark()")
        check(f['fig'] and f['fig']['id'] == 'roof' and f['up'] and not f['fig']['looked'], f'out on the walk, looking away: it stands up on the control room\'s roof behind you ({f["fig"]})')
        await shot('desk_c3_walk_away')
        # (turned to it, the eye a little beside it: the dot not on its head)
        await ev("T.look(6.3, 4.3, -4.7)"); await tick(0.25, 4)
        f = await ev("T.dark()")
        check(f['up'] and f['fig']['looked'], 'turned to it, it is there, and it stays while it is looked at')
        await shot('desk_c4_figure')
        await tick(0.5, 6)
        seenf = await ev("T.seenOf('scare', 'figure, dark')")
        check(await ev(f"!!{D}.figureAt") and len(seenf) == 1 and seenf[0]['looking'], 'still there 3 s on; in the night\'s log as seen')
        # ---------- the light ----------
        await until(f"{D}.phase === 'back'", 40, 0.25)
        await tick(0.6)
        L = await ev("T.lamps()"); f = await ev("T.dark()")
        check(not f['up'] and f['fig']['gone'] == 'light' and f['inner'] == 1 and f['sodium'] < 0.25 and all(c > 0 for c in L['ceiling']) and L['bmax'] > 0,
              f"after 47 s the light: the room, the lab and the red lamps at once, the sodium lamps dim ({f['sodium']:.2f}); it is not there")
        check('LINE POWER  RESTORED  00:00:47' in await ev("T.log()") and not (await ev("T.clock()"))['secs'] and await ev("T.said('The lights come back.')")
              and await ev("S47.audio.humHeld") == 1, 'the log, the clock back to the night\'s, the hum back')
        await shot('desk_c5_back')
        tx = await until("S47.game.ns.hondo !== undefined", 30, 0.25)
        lines = await ev("T.log()")
        check(tx is not None and 10 < tx < 26 and await ev("S47.game.ns.hondo") == clack_clock and lines[-1] == 'TELEX RECEIVED  HONDO VALLEY' and await ev("T.said('In the control room, the telex starts up.')"),
              f'the telex runs for {tx} s and Hondo Valley\'s message is on the roll (filed {clack_clock})')
        notes = await ev("S47.game.notes.slice()")
        check(any('In the empty control room K3 tripped. Forty seconds later every light at SARO went out' in n for n in notes)
              and any('up on the roof of the control room' in n and 'someone small, standing quite still' in n and 'Not there when the lights came back' in n for n in notes),
              'two notes: the relay and the dark, and the figure that was looked at')
        check(await ev(f"{D}.phase") == 'done' and await ev("S47.game.ns.k3Open") and await ev("S47.game.d.inter.get('rack').label()") == 'Reset RX bank 3' and not await ev("S47.oneAtATime()"),
              'done; K3 stays open, the lever on the rack')
        y0 = await ev("T.yardLamps()")
        # (the game changes a lamp itself while they warm up: the B-12 work lamp shielded; that is kept)
        await ev("S47.yard.setWorkLamp(0)")
        await ev("S47.game.d.inter.get('rack').use()"); await tick(0.5)
        k3back = await ev("!S47.game.ns.k3Open && S47.room.crtCenter.powered")
        await tick(0.5, 60)
        check(k3back and await ev("!S47.game.ns.k3Open && S47.room.crtCenter.powered"), 'the lever thrown while the lamps warm up: bank 3 back, and it stays back')
        y1 = await ev("T.yardLamps()")
        work = await ev("({ w: S47.floodSets.find((s) => s.key === 'site').pos[S47.yard.workLamp].w, g: S47.yard.glow.points.geometry.getAttribute('aCol').getX(S47.yard['workGlow']) })")
        check(not await ev("S47.saroPower.out") and all(b0 > 0 for b0 in y1) and y1 != y0, f'the sodium lamps warm up and everything is as before ({y0} to {y1})')
        L = await ev("T.lamps()")
        check(all(abs(L[k] - pre[k]) < 1e-6 for k in ('bus', 'sight', 'vend')) and L['fence'] == pre['fence'], 'the glass, the stencils and the fence exactly as they were')
        check(work['w'] == 0 and work['g'] == 0, f'a lamp the game shielded while they warmed up stays shielded ({work})')
        await ev("S47.yard.setWorkLamp(1)")
        await shot('desk_c6_after')
        # the telex roll and the log on the terminal
        await ev("T.aim(-4.2, 3.1, -4.2, 1.0, 4.18)")
        await ev("S47.game.d.inter.get('telex').use()"); await tick(0.2)
        page_t = await ev("document.querySelector('.docview') ? document.querySelector('.docview').textContent : ''")
        check(all(s in page_t for s in (f'ZCZC HVR{hhmm}', 'SARO OPS', 'FM HONDO VALLEY RADIO STN', f'{hhmm} MST 14 APR 86', 'RANGE SOLUTION NEGATIVE. REQUEST PROCEDURE.', 'NNNN', 'NET STATUS 2300 MST')),
              'the telex roll: last evening\'s traffic, and Hondo Valley\'s message as KAPITLER.md has it')
        await shot('desk_c7_telex')
        await ev("S47.game.d.ui.close()"); await tick(0.2)
        check(any(n.endswith('Somebody else got one tonight, and they are asking us what to do.') and 'Hondo Valley' in n for n in await ev("S47.game.notes.slice()")), 'read, it is written down')
        await ev("T.aim(-5.0, 2.45, -5.6, 1.0, 2.45)")
        await ev("S47.game.d.inter.get('opsTerminal').use()"); await tick(0.1)
        await pg.click('.term-cmds button[data-c="SHOW LOG"]'); await tick(0.05)
        out = await ev("document.querySelector('.term-out').textContent")
        i = [out.find(s) for s in ('LINE POWER  FAIL', 'LINE POWER  RESTORED  00:00:47', 'TELEX RECEIVED  HONDO VALLEY')]
        check(all(x > 0 for x in i) and i == sorted(i), 'SHOW LOG on the operations terminal has the failure, the 47 s and the telex, in order')
        await pg.click('.term-foot [data-a=close]'); await tick(0.1)
        # once
        await aim(LAB)
        await tick(1, 40)
        check(await ev(f"{D}.phase") == 'done' and len(await ev("T.seenOf('event', 'blackout')")) == 1, 'once: 40 s more in the lab inside the window, nothing again')
        # saved, and Continue
        rec = None
        for _ in range(60):
            meta = await ev("S47.saves.list(S47.caseId())[0] || null")
            rec = await ev(IDB_GET + f"('saves', '{meta['id']}')") if meta else None
            if rec and rec['state']['case'] and rec['state']['case']['ns'].get('hondo') == clack_clock: break
            await pg.wait_for_timeout(300); await tick(0.05)
        check(rec and rec['state']['case']['ns'].get('hondo') == clack_clock, 'autosaved with the telex on the roll')
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev(HELPERS); await ev("T.hook(); S47.hold = true; S47.tick(0.5); S47.nightEvents['wait'] = 1e9")
        roll = await ev("S47.room['telexMore']")
        check(await ev(f"{D}.phase") == 'done' and await ev("S47.game.ns.hondo") == clack_clock and 'FM HONDO VALLEY RADIO STN' in roll and 'FM HONDO VALLEY' in await ev("S47.game.telexRoll().join('\\n')"),
              'after Continue: done, the message on the roll and on the paper')
        await aim(LAB)
        await ev(f"S47.game.clock = Math.max(S47.game.clock, 2 * 3600 + 30 * 60)")
        await tick(1, 20)
        check(await ev(f"{D}.phase") == 'done' and sum(1 for n in await ev("S47.game.notes.slice()") if 'every light at SARO went out' in n) == 1, 'and it never comes again; the note not doubled')
        desk_errs = list(errs)
        await ctx.close()

        # ======================= phone =======================
        errs = []
        ctx, pg = await page(b, 412, 892, True, errs)
        ev = pg.evaluate
        await ev("S47.hold = true; S47.nightEvents['wait'] = 1e9")
        # as most nights have it: K3 left open since the impact
        await ev("S47.jump('chapter1'); S47.tick(1.0); S47.nightEvents['wait'] = 1e9; S47.game.d.ui.close(); S47.game['tripK3'](false); S47.game.clock = 2 * 3600 + 29 * 60 + 2; T.toasts.length = 0")
        check(await ev("S47.game.ns.k3Open"), 'phone: K3 open since the impact, as a real night leaves it')
        k3n = (await ev("T.log()")).count('RX BANK 3  K3 OPEN')
        await aim(LAB)
        await until(f"{D}.phase === 'relay'", 6, 0.1)
        lines = await ev("T.log()")
        check(await ev(f"{D}.wasOpen") and lines.count('RX BANK 3  K3 OPEN') == k3n + 1 and lines[-1] == 'RX BANK 3  K3 OPEN',
              'K3 had nothing left to trip: the same clack, and the log writes the trip again')
        await tick(0.5, 3)
        await shot('phone_c1_relay')
        await until(f"{D}.phase === 'dark'", 45, 0.25); await tick(0.5)
        await shot('phone_c2_dark_lab')
        # into the control room (the windows), facing away; then the figure out of the windows
        await ev("T.aim(1.2, -2.6, 6, 1.2, 0)"); await tick(0.25, 8)
        await until(f"!!{D}.figureAt", 14, 0.25)
        f = await ev("T.dark()")
        await ev("T.look(-6.5, 0.5, -12.5)"); await tick(0.25, 2)
        f = await ev("T.dark()")
        check(f['fig'] and f['fig']['id'] == 'grounds' and f['up'] and f['fig']['looked'], f'phone: from the control room, through the windows ({f["fig"]})')
        await shot('phone_c3_figure_window')
        await ev("S47.input.zoomHeld = true"); await tick(0.1, 6)
        f = await ev("T.dark()")
        await ev("S47.input.zoomHeld = false"); await tick(0.25, 2)
        check(not f['up'] and f['fig']['gone'] == 'zoom', 'a closer look: it was not there')
        await shot('phone_c4_gone')
        await until(f"{D}.phase === 'back'", 40, 0.25); await tick(0.5)
        await shot('phone_c5_back')
        await until(f"{D}.phase === 'done'", 30, 0.25)
        notes = await ev("S47.game.notes.slice()")
        check(any('K3, with nothing left to trip. It tripped anyway.' in n for n in notes) and any('someone small' in n and n.endswith('Then not there.') for n in notes),
              'the notes: the relay with nothing to trip, and the figure, then not there')
        await ev("T.aim(-4.2, 3.1, -4.2, 1.0, 4.18)")
        await ev("S47.game.d.inter.get('telex').use()"); await tick(0.2)
        await shot('phone_c6_telex')
        check('RANGE SOLUTION NEGATIVE. REQUEST PROCEDURE.' in await ev("document.querySelector('.docview').textContent"), 'phone: the telex reads on a phone')
        await ev("S47.game.d.ui.close()"); await tick(0.2)
        # ---------- a night put back to before it: from the lab, the doorway, the roof against the stars ----------
        await ev("""(() => { const g = S47.game; S47.saroDark.halt(); g.ns.hondo = undefined; g.ns.log = g.ns.log.filter((l) => l[1] !== 'TELEX RECEIVED  HONDO VALLEY');
          S47.room.setTelexRoll([]); S47.saroDark.restore(false); S47.curve.long.delete('ch1'); g.clock = 2 * 3600 + 33 * 60; T.toasts.length = 0;
          if (!S47.doors.isOpen('lab')) g.d.inter.get('labDoor').use(); return true; })()""")
        await tick(0.5, 3)
        await aim(LAB)
        t5 = await until(f"{D}.phase === 'dark'", 50, 0.25)
        await tick(0.5, 14)
        check(t5 is not None and await ev("T.said('Only the doorway, grey with starlight.')") and not await ev(f"{D}.figureAt"), 'phone: black in the lab, and a few seconds on the doorway is the one thing left (nothing stands where it cannot be seen)')
        await ev("T.aim(12.4, -1.2, 12.4, 1.6, -20)"); await tick(0.25, 4)
        f = await ev("T.dark()")
        check(f['fig'] and f['fig']['id'] == 'roof' and f['up'], f"phone: from the doorway, looking out along the walk, it stands up on the control room's roof ({f['fig']})")
        await ev("T.look(5.3, 4.5, -3.9)"); await tick(0.25, 4)
        f = await ev("T.dark()")
        await shot('phone_c7_roof')
        check(f['up'] and f['fig']['looked'], 'phone: turned to it, black against the stars, it stays')
        await until(f"{D}.phase === 'done'", 80, 0.25)
        notes = await ev("S47.game.notes.slice()")
        check(any('the power went: the safelight, the hum, everything' in n for n in notes) and any('up on the roof of the control room' in n and 'Not there when the lights came back' in n for n in notes),
              'phone: the notes say what the lab saw, and the figure on the roof, gone with the light')
        # not in chapter two, even as if it had not come (and there the curve has its 47 again)
        await ev("S47.jump('chapter2'); S47.tick(1.0); S47.nightEvents['wait'] = 1e9; S47.game.d.ui.close(); S47.saroDark.restore(false); S47.game.ns.hondo = undefined; S47.game.clock = 2 * 3600 + 22 * 60")
        await tick(0.5, 4)
        early = await ev("S47.game.ns.hondo")
        await ev("S47.game.clock = 2 * 3600 + 30 * 60"); await tick(0.5, 2)
        check(early is None and await ev("S47.game.ns.hondo") == 2 * 3600 + 29 * 60, 'chapter two before 02:29: no telex yet; from 02:29 it is on the roll, printed with nobody there')
        await aim(LAB); await tick(1, 15)
        await ev("S47.curve.force('build', 0.7)")
        # (chapter two keeps its 47 for the archive's phone, story/archivePhone.ts: stood aside, the dark keeps nothing there)
        await ev("S47.archive.off = true"); await tick(0.1, 1)
        check(await ev("S47.game.phase") == 'ch2' and await ev(f"{D}.phase") == 'idle' and await ev("S47.curve.may('long')"), 'not in chapter two (and there, without the archive\'s phone, the curve has its 47 again)')
        phone_errs = list(errs)
        await ctx.close()
        await b.close()

    bad = [e for e in desk_errs + phone_errs]
    check(not bad, f'no console errors or warnings ({bad[:6]})')
    n_fail = sum(1 for c in checks if c[0] == 'FAIL')
    print(f'{len(checks) - n_fail}/{len(checks)} PASS', flush=True)
    with open(f'{OUT}/results.json', 'w') as fh: json.dump(checks, fh, indent=1)
    sys.exit(1 if n_fail else 0)

asyncio.run(main())
