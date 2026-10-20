# One source for where the 1947 lines A, B and C cross the ground (story/lines.ts), and a log of
# what the night did and whether it was seen (story/seen.ts; FORSLAG.md, 20 October, "Forslag til
# rekkefoelge" step 1 and "Hull").
#   - the lines: A from the transit at STATION 01 through Caliche's payphone and over the main
#     street; B from the transit to B-12's vane in SARO's yard, across the highway at SARO's
#     turn-off (between SARO and the motel), on through the field; C on the station's ground and
#     at the old road's crossing; the marks' places on the lines the map has always had them on
#   - the time away asks the module: B out on SARO's ground and in the field (waking further
#     along B), the highway's shoulder, A in town, C at the station
#   - the three lights pace the truck where B runs west of the highway, and turn off onto it,
#     south-west towards STATION 01; with three marks on B the line is drawn in on the map, and the
#     note says where it is: across the highway at the turn-off to SARO, through STATION 01
#   - the log: a glimpse looked at and one missed, a sound in the dark and the player turning to
#     it, the steps behind, the lights, the town's and the night's events by their keys, the time
#     away got away from and taken, the one over you on waking (missed, then looked at), the
#     dishes that follow (looked at), the steps on the roof (looked up at), the face on the film
#     (seen when the print is opened large), the one in the SARO jacket that stands where the
#     player was taken (when it is first looked at from further off); each once, with the clock, the place and the line;
#     nothing in the prologue and nothing written down as a save is put back; saved with the case
#     and back after Continue; on the ?debug overlay
#   - desktop 1280x720 and a phone 412x892 with touch; no console errors or warnings
# Usage (from web/): python3 tools/f_abc_lines.py OUTDIR
import asyncio, sys, json, os, math, base64
from playwright.async_api import async_playwright

OUT = sys.argv[1]
BASE = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
URL = BASE + ('&' if '?' in BASE else '?') + 'debug'
ARGS = ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required']
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

A = "S47.abduction()"
HELPERS = """(() => {
  const V = S47.three.Vector3;
  window.T = {
    // stand at (x, z) and look at a point, turned off it by off degrees (the eye is the camera's after a tick)
    aim(x, z, tx, ty, tz, off = 0) {
      const pl = S47.player; pl.place(x, z, pl.yaw); S47.tick(0.05);
      return T.look(tx, ty, tz, off) && pl.walkable(x, z);
    },
    look(tx, ty, tz, off = 0) {
      const pl = S47.player, c = S47.camera.position;
      pl.yaw = Math.atan2(-(tx - c.x), -(tz - c.z)) + off * Math.PI / 180;
      pl.pitch = Math.atan2(ty - c.y, Math.hypot(tx - c.x, tz - c.z)); S47.tick(0.05);
      return true;
    },
    log: () => S47.seen.map((e) => ({ ...e })),
    of: (kind, what) => S47.seen.filter((e) => e.kind === kind && (what === undefined || e.what === what)).map((e) => ({ ...e })),
  };
  return true;
})()"""

async def page(b, w, h, touch, errs):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
    except Exception: pass
    await pg.evaluate("localStorage.clear()"); await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    if touch: await pg.tap('button[data-a=start]')
    else: await pg.click('button[data-a=start]')
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS)
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    return ctx, pg

async def main():
    os.makedirs(OUT, exist_ok=True)
    errs = []
    async with async_playwright() as p:
        b = await p.chromium.launch(args=ARGS)
        # ======================= desktop =======================
        ctx, pg = await page(b, 1280, 720, False, errs)
        ev = pg.evaluate
        async def tick(s=0.1, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        log = lambda: ev("T.log()")
        of = lambda kind, what=None: ev(f"T.of({json.dumps(kind)}{', ' + json.dumps(what) if what else ''})")

        # ---------- the prologue: nothing ----------
        # (the night's one curve, story/curve.ts, stands aside in this test: each thing on its own timer; tools/f_curve.py has them together)
        await ev("S47.hold = true; S47.curve.free = true"); await tick(1, 5)
        ph = await ev("S47.game.phase")
        check(await ev("S47.seen.length") == 0 and await ev("Array.isArray(S47.seen)"), f'nothing in the log in the prologue, five seconds in ({ph})')

        # ---------- the lines (story/lines.ts) ----------
        L = await ev("""(() => { const l = S47.lines, T = l.TRANSIT, B = l.B12, P = l.PLACES, deg = (d) => (Math.atan2(d[0], -d[1]) * 180 / Math.PI + 360) % 360;
          const vane = l.toMap('station01', -7.4, -10), tr = l.toMap('station01', 3, -4), vd = [vane[0] - tr[0], vane[1] - tr[1]];
          const a = (d1, d2) => { const x = Math.abs(deg(d1) - deg(d2)) % 180; return Math.min(x, 180 - x); };
          return { transit: T, b12: B, dA: l.LINES.A.dir, dB: l.LINES.B.dir, bA: deg(l.LINES.A.dir), bB: deg(l.LINES.B.dir),
            B_transit: l.distTo('B', ...T), B_b12: l.distTo('B', ...B), A_transit: l.distTo('A', ...T), A_phone: l.distTo('A', ...P.phone),
            Bm: l.CROSSINGS.B.highwayM, Az: l.CROSSINGS.A.highway, town: l.CROSSINGS.A.town, field: l.CROSSINGS.B.field, dishes: l.CROSSINGS.B.dishes,
            vaneOff: a(vd, l.LINES.B.dir), lights: l.distTo('B', ...P.lights),
            marks: Object.fromEntries(['lights', 'cows', 'tv', 'phone', 'phone2', 'repeat', 'lamps', 'boy', 'regular', 'cutout', 'car', 'away?'].map((k) => [k, l.lineOfMark(k)])),
            here: { yard: l.lineAt('saro', 9.9, -12), field: l.lineAt('saro', -1, -62), shoulder: l.lineAt('saro', -24, 300), motel: l.lineAt('saro', -40, 690),
              herd: l.lineAt('road', P.cows[0], P.cows[1]), between: l.lineAt('road', 0, 950), town: l.lineAt('diner', 0, 2150), diner: l.lineAt('road', -20, 2000),
              cutout: l.lineAt('road', ...P.cutout), station: l.lineAt('station01', 0, 0), pad: l.lineAt('station01', -19, 31),
              oldC: l.lineAt('roswell', 120, 40), oldFar: l.lineAt('roswell', -400, -2000), room6: l.lineAt('room6', 0, 0) } }; })()""")
        print('   lines:', json.dumps({k: L[k] for k in ('transit', 'b12', 'bA', 'bB', 'Bm', 'Az', 'vaneOff', 'dishes')}), flush=True)
        check(L['B_transit'] < 0.01 and L['B_b12'] < 0.01 and 30 < L['bB'] < 50, f"B: from the transit at STATION 01 to B-12's vane at SARO, bearing {L['bB']:.1f}")
        check(0 < L['Bm'] < 640, f"B crosses the highway between SARO and the motel: at SARO's turn-off, {L['Bm']:.0f} m south of the site")
        check(L['lights'] < 60, f"where the three lights turn off, B is {L['lights']:.0f} m away")
        check(L['field'] is not None and len(L['dishes']) >= 3, f"B runs out through the antenna field, past {', '.join(L['dishes'])}")
        check(L['vaneOff'] < 12, f"at the station B leaves the transit {L['vaneOff']:.1f} degrees off the old comparison vane (the station is in its own terms)")
        check(L['A_transit'] < 0.01 and L['A_phone'] < 0.01 and 2016 < L['Az'] < 2300 and L['town'] is not None, f"A: from the transit through Caliche's payphone, over the main street at road z {L['Az']:.0f}")
        mk = L['marks']
        check(all(mk[k] == 'B' for k in ('lights', 'cows', 'tv')) and all(mk[k] == 'A' for k in ('phone', 'phone2', 'repeat', 'lamps', 'boy', 'regular', 'cutout', 'car', 'away?')),
              f'the marks on the lines the map has always had them on ({mk})')
        h = L['here']
        on = lambda k: h[k]['line'] if h[k] else None
        check(on('yard') == 'B' and on('field') == 'B' and on('shoulder') == 'B' and on('motel') == 'B' and on('herd') == 'B' and on('room6') == 'B',
              "B is SARO's line: the yard, the field, the shoulder, the motel and room 6, the herd")
        check(on('town') == 'A' and on('diner') == 'A' and on('cutout') == 'A', 'A is the town\'s: the street, the diner, the cutout on the highway north of it')
        check(on('station') == 'C' and on('pad') == 'C' and on('oldC') == 'C' and on('oldFar') is None, 'C at STATION 01 and on the old road at the crossing, not far from it')
        check(on('between') is None, 'the highway half way to the diner is on no line')

        # ---------- chapter two: Ward, the records room, the steps on the roof ----------
        await ev("S47.jump('chapter2'); S47.tick(1.5); S47.nightEvents['wait'] = 1e9")
        await ev("S47.game.d.inter.get('phone').use()"); await tick(1, 12)
        check(await ev("S47.ch2.s.stage") == 'records', 'chapter two: Ward has called, the records are next')
        n0 = await ev("S47.seen.length")
        await ev("T.aim(-3.65, 7.75, -3.65, 0.8, 8.6); S47.game.d.ui.close(); S47.tick(0.2)")
        started = await ev("S47.roofSteps.now(S47.roofIn())")
        await tick(0.1, 3)
        roof = await of('roof')
        check(started and len(roof) == 1 and roof[0]['line'] == 'B' and roof[0]['place'] == 'Records room' and roof[0]['looking'] is False,
              f'the steps on the roof: written down once, on B, in the records room, not looked up at yet ({roof})')
        # look up at where they make for, right overhead
        await ev("(() => { const o = S47.roofSteps.overhead; T.look(o.x, o.y, o.z); })()"); await tick(0.1, 3)
        roof = await of('roof')
        check(roof[0]['looking'] is True, 'looked up at the ceiling where they stop: looked')
        for _ in range(80):
            if not await ev("S47.roofSteps.busy"): break
            await tick(0.5)
        check(not await ev("S47.roofSteps.busy") and len(await of('roof')) == 1, 'the steps over and gone: still one')

        # ---------- out in the field (chapter two, about three) ----------
        await ev("S47.game.clock = 2 * 3600 + 58 * 60; S47.world.enter('saro'); S47.flashlight.have = true; S47.flashlight.on = true; S47.input.locked = true; S47.tick(0.2)")
        ok = await ev("T.aim(-1, -62, -1, 4, -300)")
        ab = await ev("(() => { const s = S47.abductIn(); return [s.line, s.along.x, s.along.z]; })()")
        dB = L['dB']
        check(ok and ab[0] == 'B' and abs(ab[1] - dB[0]) < 1e-6 and abs(ab[2] - dB[1]) < 1e-6, f'the time away asks the lines: out in the field, on B, and the way further along is B\'s ({ab})')
        lh = await ev("S47.lineHere()")
        check(lh['player'] and lh['player']['line'] == 'B' and lh['truck'] and lh['truck']['line'] == 'B', f"which line: the player in the field and the truck on SARO's pad, both on B")
        # a glimpse looked at: behind S-11, then turned to
        await ev("T.look(-12, 4, -208, 34)")
        put = await ev("S47.glimpses.now(S47.glimpseIn(), 'S-11')")
        await tick(0.1, 2)
        g = await of('glimpse', 'S-11')
        check(put and len(g) == 1 and g[0]['line'] == 'B' and g[0]['looking'] is False and g[0]['at'] is not None, f'a glimpse behind S-11, at the edge of the view: written down, on B, not looked at ({g})')
        await ev("T.look(-12, 0.6, -208, 0)"); await tick(0.1, 3)
        g = await of('glimpse', 'S-11')
        check(not await ev("S47.glimpses.up") and g[0]['looking'] is True, 'turned to it: it is not there, and it was looked at')
        # one missed: stays off the middle of the view until it goes by itself
        await ev("S47.glimpses['t'] = 0; S47.glimpses.count = 0")
        await ev("T.look(-42, 4, -308, -36)")
        put = await ev("S47.glimpses.now(S47.glimpseIn(), 'S-19')")
        for _ in range(45):
            await tick(1)
            if not await ev("S47.glimpses.up"): break
        g = await of('glimpse', 'S-19')
        check(put and len(g) == 1 and g[0]['looking'] is False and not await ev("S47.glimpses.up"), f'a glimpse behind S-19 never looked at, gone after its 40 s: missed ({g})')
        # a sound out in the dark, behind; then the player turns to it
        await ev("T.look(-1, 1.6, -300)")
        await ev("S47.calls.now()"); await tick(0.1, 25)
        c = await of('call')
        check(len(c) == 1 and c[0]['looking'] is False and c[0]['line'] == 'B', f'a sound out in the dark, behind the player: written down, on B, not turned to ({c})')
        await ev("(() => { const a = S47.calls.last.at; T.look(a.x, a.y, a.z); })()"); await tick(0.1, 3)
        check((await of('call'))[0]['looking'] is True, 'the player turns to where it came from: looked')
        # the steps behind (a night event, by its key)
        await ev("S47.player.place(-1, -62, Math.PI); S47.tick(0.1)")
        st = await ev("S47.nightEvents.now('steps', S47.nightIn())")
        for _ in range(60):
            if await ev("S47.townSeen.has('ne:steps1')"): break
            await tick(0.1)
        s1 = await of('event', 'steps')
        check(st and len(s1) == 1 and s1[0]['looking'] is False and s1[0]['line'] == 'B', f'the steps behind, by their key: written down once, nobody looking behind ({s1})')
        await ev("(() => { const a = S47.nightEvents.stepsAt; T.look(a.x, a.y + 1, a.z); })()"); await tick(0.1, 3)
        check((await of('event', 'steps'))[0]['looking'] is True, 'turned round to them: looked')

        # ---------- the dishes, in a warning in the field ----------
        await ev("S47.nightEvents['wait'] = 1e9; S47.glimpses['wait'] = 1e9; S47.calls.next = 1e9")
        await ev("T.aim(-1, -62, -16, 6, -52, 70)")
        await ev(f"{A}.data.last = -1e9; {A}.pressure = 5; S47.tick(0.05)")
        if await ev(f"{A}.stage") == 'warning': await ev(f"{A}['warnLen'] = 30")
        check(await ev(f"{A}.stage") == 'warning', 'a warning in the field')
        w = await of('abduction')
        check(len(w) == 1 and w[0]['what'] == 'warning' and w[0]['line'] == 'B' and w[0]['looking'] is None and 'end' not in w[0], f'the warning: written down, on B ({w})')
        first = await ev("S47.follow.constructor.firstAt(30)")
        await tick(0.1, int(first * 10) + 8)
        d = await of('dishes')
        check(await ev("S47.follow.mode") == 'follow' and len(d) == 1 and d[0]['line'] == 'B', f'the dishes come down out of 026: written down once, on B ({d})')
        await ev("(() => { const d = S47.ext.dishes.find((x) => x.id === 'S-02'); T.look(d.x, 5, d.z); })()"); await tick(0.1, 3)
        check((await of('dishes'))[0]['looking'] is True, 'looked at the dish that came round: looked')
        await ev("S47.input.keys.clear(); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
        w = await of('abduction')
        check(await ev(f"{A}.stage") == 'calm' and w[0].get('end') == 'away', f'indoors before the light: it passed, written down as got away ({w[0].get("end")})')
        for _ in range(60):
            if await ev("S47.follow.mode") == 'idle': break
            await tick(0.5)
        await pg.wait_for_timeout(3000)   # the outlines' pictures (they are asked for when a warning starts)

        # ---------- taken: the one over you, missed; the next time looked at ----------
        async def taken(count):
            for _ in range(3):
                await ev("S47.game.d.ui.close(); S47.roadside.reset(); S47.world.enter('saro'); S47.player.place(-24, 200, Math.PI); S47.tick(0.3)")
                await ev(f"{A}.data.count = {count}; {A}.data.last = -1e9; S47.game.clock = 2 * 3600 + 58 * 60")
                await ev(f"{A}.pressure = 5; S47.tick(0.1)")
                if await ev(f"{A}.stage") != 'warning': continue
                await ev(f"{A}['warnLen'] = 0.5; {A}['st'] = 0.5")
                for _ in range(300):
                    if await ev(f"{A}.stage") in ('back', 'calm'): break
                    await ev("S47.tick(0.1)")
                # a frame at a time from the waking, so the moment it comes is known
                for _ in range(150):
                    if await ev(f"{A}.overPhase") == 'there': return True
                    if await ev(f"{A}.stage") == 'calm': return False
                    await ev("S47.tick(1 / 30)")
            return False
        there = await taken(0)
        await ev("S47.tick(1 / 30)")
        w = await of('abduction')
        check(there and len(w) == 2 and w[1]['end'] == 'taken' and w[1]['line'] == 'B', f'out on the shoulder south of SARO: taken, written down so ({[x.get("end") for x in w]})')
        o = await of('over')
        check(len(o) == 1 and o[0]['what'] == 'first' and o[0]['line'] == 'B', f'the first time one of them is over you: written down ({o})')
        for _ in range(90):
            if await ev(f"{A}.overPhase") is None: break
            await ev("S47.tick(1 / 30)")
        await ev("S47.tick(1 / 30)")   # (the log takes it up at the start of the next step)
        o = await of('over')
        check(o[0]['looking'] is False, 'the eyes never went up to it: missed')
        for _ in range(240):
            if await ev(f"{A}.stage") == 'calm': break
            await tick(0.1)
        there = await taken(1)
        # the eyes up to it, through the game's own look (a mouse moved, a finger drawn)
        await ev("S47.input.lookDY = -0.3; S47.tick(1 / 30)")
        for _ in range(90):
            if await ev(f"{A}.overPhase") is None: break
            await ev("S47.tick(1 / 30)")
        await ev("S47.tick(1 / 30)")   # (the log takes it up at the start of the next step)
        o = await of('over')
        check(there and len(o) == 2 and o[1]['what'] == 'second' and o[1]['looking'] is True, f'the second time the eyes go up to it: looked ({[x["looking"] for x in o]})')
        for _ in range(240):
            if await ev(f"{A}.stage") == 'calm': break
            await tick(0.1)
        mk = await ev("S47.mapMarks().filter((m) => m.kind === 'away').map((m) => m.line)")
        check(mk == ['B', 'B'], f'the times away on the map, on B where they were taken ({mk})')
        # the one in the SARO jacket that stands where the player was taken (abduction.ts, the
        # recordings): written down the first time it is looked at from further off, once
        rec0 = len([e for e in await log() if e['kind'] == 'scare' and e['what'].startswith('record ')])
        await ev("S47.player.place(-24, 330, Math.PI); S47.player.pitch = 0; S47.tick(0.3)")
        rec1 = len([e for e in await log() if e['kind'] == 'scare' and e['what'].startswith('record ')])
        await ev("(() => { const t = S47.abduction().data.spots[0].take; T.look(t[0], t[1] + 1.2, t[2]); })()"); await tick(0.2, 2)
        rec = [e for e in await log() if e['kind'] == 'scare' and e['what'].startswith('record ')]
        await tick(0.5, 2)
        rec2 = [e for e in await log() if e['kind'] == 'scare' and e['what'].startswith('record ')]
        check(rec0 == 0 and rec1 == 0 and len(rec) == 2 and all(e['looking'] is True and e['line'] == 'B' for e in rec) and len(rec2) == 2,
              f'the one in the SARO jacket where they were taken: nothing until it is looked at, from 130 m; then each of the two, once ({[e["what"] for e in rec]})')

        # ---------- the three lights, where B runs west of the highway ----------
        await ev("S47.setAbductions(false); S47.game.clock = 3 * 3600 + 5 * 60; S47.player.place(-30.5, 150, Math.PI / 2); S47.player.pitch = 0.05; S47.tick(0.2)")
        await ev("""(() => { const L = S47.lineLights, orig = L.update.bind(L); window.__truck = new S47.three.Vector3(-25.8, 0, 100); L.__orig = orig;
          L.update = (dt, s) => orig(dt, { ...s, ok: true, truck: window.__truck }); })()""")
        n_lights = len(await of('event', 'lights'))
        # north first: nothing
        await ev("window.__truck.z = 240; S47.tick(0.1); for (let i = 0; i < 40; i++) { window.__truck.z -= 1.77; S47.tick(0.1); }")
        check(await ev("S47.lineLights.state") == 'none' and len(await of('event', 'lights')) == n_lights, 'driving north: no lights, nothing written down')
        track = await ev("""(() => { const out = []; window.__truck.z = 100; S47.tick(0.1);
          for (let i = 0; i < 260; i++) { window.__truck.z += 1.77; S47.tick(0.1); const at = S47.lineLights.at; out.push([S47.lineLights.state, at ? at.x : null, at ? at.z : null]); }
          return out; })()""")
        await ev("S47.lineLights.update = S47.lineLights.__orig; 0")
        off = [r for r in track if r[0] == 'off' and r[1] is not None]
        pace = [r for r in track if r[0] == 'pace']
        dist = lambda r: ev(f"S47.lines.distTo('B', ...S47.lines.toMap('saro', {r[1]}, {r[2]}))")
        d0, d1 = (await dist(off[0]), await dist(off[-1])) if off else (0, 0)
        check(len(pace) > 100 and len(off) > 20 and off[-1][1] < off[0][1] - 100 and off[-1][2] > off[0][2] and d1 < d0 * 0.5,
              f'the lights keep pace, then turn off onto B and along it, south-west towards STATION 01 ({d0:.0f} m off B to {d1:.0f})')
        lt = await of('event', 'lights')
        check(len(lt) == 1 and lt[0]['line'] == 'B' and lt[0]['looking'] is True, f'written down once when they came, on B, looked at: the player stood looking west ({lt})')
        await ev("S47.game.d.ui.close(); S47.tick(0.1)")
        # three marks on B: the line drawn in, and the note says where
        n_ev = len(await of('event'))
        await ev("S47.townSeen.add('mark:cows@' + Math.round(S47.game.clock - 600)); S47.townSeen.add('mark:tv@' + Math.round(S47.game.clock - 300)); S47.tick(1.5)")
        ct = await ev("T.of('event').slice(-2).map((e) => [e.what, e.line])")
        check(ct == [['cows', 'B'], ['tv', 'B']], f'the herd and the motel\'s television, by their marks: written down, on B ({ct})')
        nn = await ev("S47.game.notes.filter((n) => n.includes('Three on one line'))")
        check(len(nn) == 1 and 'turn-off to SARO' in nn[0] and 'south-west, through STATION 01' in nn[0] and 'south of the motel' not in nn[0],
              f'three on B: drawn in on the map, and the note says where it runs ({nn})')
        m = await ev("S47.game.notebookExtras().map")
        open(f'{OUT}/d_map_lines.png', 'wb').write(base64.b64decode(m.split(',', 1)[1]))
        await ev("S47.game.openNotebook()"); await tick(0.1)
        await pg.click('.notebook [data-t=map]'); await pg.wait_for_timeout(500); await tick(0.1)
        await shot('d01_notebook_map_B')
        await ev("S47.game.d.ui.close()"); await tick(0.1)

        # ---------- chapter four: the face on the film ----------
        await ev("S47.jump('chapter4'); S47.tick(1.0); S47.album.restore(null); S47.game.d.ui.close(); S47.tick(0.2)")
        await ev("S47.player.place(-30.5, 200, Math.PI); S47.player.pitch = -0.02; S47.tick(0.2)")
        await ev("S47.album.forceFace = true; S47.album.expose(); S47.tick(0.1)")
        f = await of('face')
        check(len(f) == 1 and f[0]['line'] == 'B' and f[0]['looking'] is False, f'the face on a free frame: written down where it was taken, on B, not seen yet ({f})')
        await ev("S47.game.clock += 91; S47.tick(0.3); S47.album.looked(S47.album.data.face.id); S47.tick(0.1)")
        f = await of('face')
        check(len(f) == 1 and f[0]['looking'] is True, 'the print opened large, come up: seen')

        # ---------- the log as it stands, on the ?debug overlay ----------
        full = await log()
        kinds = sorted({e['kind'] for e in full})
        check(all(k in kinds for k in ('roof', 'glimpse', 'call', 'event', 'abduction', 'dishes', 'over', 'face')), f'every kind is in the log ({kinds}, {len(full)} entries)')
        check(all(isinstance(e['clock'], int) and e['place'] and e['area'] for e in full), 'each with the clock, the place and the area')
        json.dump(full, open(f'{OUT}/seen_desktop.json', 'w'), indent=1)
        await ev("S47.hold = false"); await pg.wait_for_timeout(1500); await ev("S47.hold = true")
        hud = await ev("(() => { const e = document.querySelector('.debughud'); return e ? e.textContent : null; })()")
        check(hud is not None and 'seen ' in hud and 'face' in hud and 'looked' in hud, f'the ?debug overlay lists them ({(hud or "").splitlines()[-3:]})')
        await shot('d02_debug_overlay')

        # ---------- saved with the case; nothing doubled after Continue ----------
        await ev("S47.game.d.ui.close(); S47.tick(0.2)")
        await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(800)
        before = await log()
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev(HELPERS); await ev("S47.hold = true; S47.curve.free = true; S47.tick(0.5)")
        after = await log()
        check(after == before and len(after) > 10, f'after Continue the log is the one saved, and nothing was written down as the night was put back ({len(before)} and {len(after)} entries)')
        # the lights do not come again, and nothing new is written down for them
        await ev("""(() => { const L = S47.lineLights, orig = L.update.bind(L); window.__truck = new S47.three.Vector3(-25.8, 0, 100); L.__orig = orig;
          L.update = (dt, s) => orig(dt, { ...s, ok: true, truck: window.__truck }); })()""")
        await ev("window.__truck.z = 100; S47.tick(0.1); for (let i = 0; i < 160; i++) { window.__truck.z += 1.77; S47.tick(0.1); }")
        await ev("S47.lineLights.update = S47.lineLights.__orig; 0")
        check(await ev("S47.lineLights.done") and len(await of('event', 'lights')) == 1, 'after Continue the lights never come again, and the log has them once')

        # ======================= phone =======================
        ctx2, pp = await page(b, 412, 892, True, errs)
        e2 = pp.evaluate
        await e2("S47.hold = true; S47.curve.free = true; S47.jump('chapter2'); S47.tick(1.5); S47.nightEvents['wait'] = 1e9; S47.game.d.ui.close()")
        await e2("S47.game.clock = 2 * 3600 + 58 * 60; S47.setAbductions(false); S47.world.enter('saro'); S47.flashlight.have = true; S47.flashlight.on = true; S47.tick(0.2)")
        await e2("T.aim(-1, -62, -1, 4, -300)")
        # on a phone held upright the sides are hardly there: the edge of the view is above or below
        # the middle; a glimpse near the top of the view, then turned to, is looked at
        fov = await e2("S47.camera.fov")
        up = max(26.5, min(0.8 * fov / 2, 40))
        await e2(f"T.look(-12, 0.6, -208, 0); S47.player.pitch -= {math.radians(up)}; S47.tick(0.05)")
        put = await e2("S47.glimpses.now(S47.glimpseIn(), 'S-11')"); await e2("S47.tick(0.2)")
        g0 = await e2("T.of('glimpse', 'S-11')")
        print(f'   phone: vertical field of view {fov:.0f} degrees, the glimpse {up:.1f} degrees above the middle', flush=True)
        await e2("T.look(-12, 0.6, -208, 0)"); await e2("S47.tick(0.3)")
        g1 = await e2("T.of('glimpse', 'S-11')")
        check(put and len(g1) == 1 and g0[0]['looking'] is False and g1[0]['looking'] is True, f'phone: a glimpse at the edge, then turned to: looked ({g1})')
        await e2("S47.calls.now()"); await e2("for (let i = 0; i < 25; i++) S47.tick(0.1)")
        check(len(await e2("T.of('call')")) == 1, 'phone: a sound in the dark, written down')
        await e2("for (const k of ['lights@11000', 'cows@11400', 'tv@11800']) S47.townSeen.add('mark:' + k); S47.tick(1.5)")
        nb = await e2("S47.game.notes.filter((n) => n.includes('Three on one line'))")
        check(len(nb) == 1 and 'turn-off to SARO' in nb[0], 'phone: three on B, drawn in')
        await e2("S47.hold = false"); await pp.wait_for_timeout(1500); await e2("S47.hold = true")
        box = await e2("(() => { const e = document.querySelector('.debughud'); if (!e) return null; const r = e.getBoundingClientRect(); return { text: e.textContent, right: r.right, w: innerWidth }; })()")
        lines_seen = box['text'].split('seen ', 1)[1].splitlines() if box and 'seen ' in box['text'] else []
        # (each of the log's lines fits beside the touch controls: 72 px in, 13 px monospace)
        check(box is not None and any('glimpse' in l for l in lines_seen) and all(len(l) <= 40 for l in lines_seen[1:]),
              f"phone: the overlay lists them, each line short enough for the screen ({max((len(l) for l in lines_seen[1:]), default=0)} characters at most; the overlay's own lines reach {box and round(box['right'])} of {box and box['w']} px)")
        await pp.screenshot(path=f'{OUT}/p01_debug_overlay.png')
        await e2("S47.game.openNotebook(); S47.tick(0.1)")
        await pp.tap('.notebook [data-t=map]'); await pp.wait_for_timeout(500); await e2("S47.tick(0.1)")
        await pp.screenshot(path=f'{OUT}/p02_notebook_map_B.png')
        await e2("S47.game.d.ui.close(); S47.tick(0.1)")
        json.dump(await e2("T.log()"), open(f'{OUT}/seen_phone.json', 'w'), indent=1)

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:6])))
        json.dump({'checks': checks}, open(f'{OUT}/f_abc_lines.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
