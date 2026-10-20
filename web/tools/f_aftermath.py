# The time away has consequences, chapters three to five (FORSLAG.md, 20 October, item 9;
# story/stationTaken.ts, story/recording.ts, story/abduction.ts, story/Chapter4.ts, story/caliche.ts).
#   A  at STATION 01 everyone goes: after N. Vega's call, a player not taken yet gets the warning
#      a step or two out of the hut, whatever the night's curve is doing (here in its rest);
#      the hut door swings shut behind them and latches, the light inside goes out, the hut cannot
#      be walked into and its door does not give, the truck has no keys, running about the station
#      changes nothing; at :40 the lamps outside go out with the flashlight; the light, the taking;
#      on waking the door open again, the light and the lamps on, the keys in the frost (a toast,
#      a note); the 47 s kept as a path, a quarter second a sample. Not with Missing time off, not
#      after 04:16 (no room for the lost time before 04:35), not when taken already, not twice,
#      not after Continue; Missing time turned off on the way lets it pass and puts the station back
#   B  the recording walks the last 47 seconds where the player was taken: never in a rest (the
#      curve hurries for it, then it comes), where the player is not looking, along the kept path
#      (where the player stood it stands, where they ran it runs), standing still at the end for
#      the light's 3 s, not there, and back at the start; one at a time while it walks; gone when
#      walked up to; written in the seen log, landing the peak once looked at; a note; once, also
#      after Continue. At the station (the real path) and in SARO's antenna field from the pad
#      (chapter four, a path laid in)
#   C  the mark: N. Vega at P10 after a time away (a chair, then her lamp: "You have been out on
#      the line." / "One side of your face." / "Tomás came back from it like that in June."), not
#      recorded and in an exchange of its own (the P10 lines keep their voice); once; not without
#      a time away; later when taken after P10. T. Vega's field book has the June night (it fits
#      the page). The deputy after half past four: dried blood under his nose, and he has not
#      noticed; told, he says what he always says
#   desktop 1280x720 and a phone 412x892 with touch; no console errors or warnings.
# Usage (from web/): python3 tools/f_aftermath.py OUTDIR
import asyncio, sys, json, os, math, io
from PIL import Image
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

HELPERS = """(() => {
  const V = S47.three.Vector3;
  window.T = {
    toasts: [],
    hook() { const ui = S47.game.d.ui; if (ui.__hooked) return; ui.__hooked = true;
      const o = ui.toast.bind(ui); ui.toast = (t, s, c) => { T.toasts.push(t); return o(t, s, c); }; },
    last() { return T.toasts[T.toasts.length - 1] || ''; },
    ab() { return S47.abduction(); },
    site() { return S47.world.site; },
    o() { return S47.world.site.group.position; },
    w(x, z) { const o = T.o(); return [x + o.x, z + o.z]; },
    at() { const o = T.o(), p = S47.player.pos; return [+(p.x - o.x).toFixed(2), +(p.z - o.z).toFixed(2)]; },
    // the player's own movement a frame at a time, towards a point in the station's terms (or the world's)
    walk(x, z, run = false, stop = 0.2, max = 900, world = false) {
      const p = S47.player, [tx, tz] = world ? [x, z] : T.w(x, z);
      for (let n = 0; n < max; n++) {
        if (Math.hypot(p.pos.x - tx, p.pos.z - tz) < stop) return true;
        p.yaw = Math.atan2(-(tx - p.pos.x), -(tz - p.pos.z)); p.update(1 / 30, 0, -1, run); S47.tick(1 / 30);
      }
      return false;
    },
    // stand where one is, frame by frame, turning to a yaw (radians a second at most)
    stand(secs, yaw = null) {
      const p = S47.player;
      for (let n = 0; n < Math.round(secs * 30); n++) {
        if (yaw !== null) p.yaw += Math.max(-0.1, Math.min(0.1, Math.atan2(Math.sin(yaw - p.yaw), Math.cos(yaw - p.yaw))));
        S47.tick(1 / 30);
      }
    },
    yawTo(x, z, world = false) { const p = S47.player.pos, [tx, tz] = world ? [x, z] : T.w(x, z); return Math.atan2(-(tx - p.x), -(tz - p.z)); },
    look(v) { const p = S47.player, c = S47.camera.position; p.yaw = Math.atan2(-(v[0] - p.pos.x), -(v[2] - p.pos.z)); S47.tick(1 / 30);
      p.pitch = Math.atan2(v[1] - c.y, Math.hypot(v[0] - c.x, v[2] - c.z)); S47.tick(1 / 30); },
    door() { const v = T.site().proxies.hutDoor.getWorldPosition(new V()); return [v.x, v.y, v.z]; },
    truck() { const v = T.site().proxies.truckSpot.getWorldPosition(new V()); return [v.x, v.y, v.z]; },
    aimed() { const it = S47.game.d.inter.update(S47.camera); return it ? it.id : null; },
    st() { const a = T.ab(), s = T.site(); return { stage: a.stage, forced: a.forced, t: a.warn && a.warn.t, count: a.data.count, station: a.data.station || null,
      due: S47.stationDue(), shut: S47.stationTaken.hutShut, keys: S47.stationTaken.keysGone, light: s && s.hutLightOn, lamps: s && s.outLampsOn,
      leaf: s && +s.objs.hutDoorLeaf.rotation.y.toFixed(3), inside: S47.world.indoors(S47.player.pos), torch: S47.flashlight.on }; },
    rec(n) { return T.ab().recordings()[n - 1]; },
    fig(n) { const g = T.rec(n).fig.group; return { x: g.position.x, z: g.position.z, y: g.position.y, vis: g.visible, yaw: g.rotation.y }; },
    head(n) { const g = T.rec(n).fig.group; return [g.position.x, g.position.y + 1.45, g.position.z]; },
    walkSt() { const w = S47.recWalk; return { phase: w.phase, n: w.n, loops: w.loops, looked: w.looked, waiting: w.waiting, ended: w.ended, shown: w.shown, t: w.t }; },
    seen(kind, what) { return S47.seen.filter((e) => e.kind === kind && (what === undefined || e.what === what)).map((e) => ({ what: e.what, end: e.end, looking: e.looking, line: e.line, area: e.area })); },
    notes(s) { return S47.game.notes.filter((n) => n.includes(s)); },
    // on the screen: where a world point is (pixels), and whether it is in the view
    px(v) { const p = new V(...v).project(S47.camera); return [(p.x + 1) / 2 * innerWidth, (1 - p.y) / 2 * innerHeight, p.z < 1 && Math.abs(p.x) < 1 && Math.abs(p.y) < 1]; },
  };
  return true;
})()"""

async def page(b, w, h, touch):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    init = "try{localStorage.setItem('s47.serlingNarrator','false');}catch{}"
    await pg.add_init_script(init)
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
    except Exception: pass
    await pg.evaluate("localStorage.clear()")
    await pg.add_init_script(init)
    await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    if touch: await pg.tap('button[data-a=start]')
    else: await pg.click('button[data-a=start]')
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS); await pg.evaluate("T.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    return ctx, pg, errs

class Run:
    def __init__(self, pg, touch): self.pg = pg; self.ev = pg.evaluate; self.touch = touch
    async def tick(self, s, n=1):
        for _ in range(n): await self.ev(f"S47.tick({s})")
    async def shot(self, name, clear=False):
        if clear: await self.ev("S47.game.d.ui.clearToasts()"); await self.pg.wait_for_timeout(250)
        await self.pg.screenshot(path=f'{OUT}/{name}.png')
    async def quiet(self): await self.ev("S47.nightEvents['wait'] = 1e9")
    async def st(self): return await self.ev("T.st()")
    async def station(self, clock):
        """Chapter three at STATION 01 after N. Vega's call, inside the hut, at this clock."""
        ev = self.ev
        await ev("S47.jump('chapter3'); S47.tick(1.0)"); await self.quiet()
        await ev("window.__p = S47.world.prepare('station01')")
        await self.pg.wait_for_function("!!S47.world.site", polling=300)
        await ev("S47.world.placeTruck('station01'); S47.world.enter('station01'); S47.world.placeAtStation(); S47.tick(0.3); S47.ch3.arrivedStation(); S47.tick(0.3)")
        await ev(f"S47.ch3.s.called = true; S47.ch3.s.stage = 'return'; S47.game.clock = {clock}; S47.flashlight.have = true; S47.flashlight.on = true")
        await ev("const [x, z] = T.w(-11, 3.2); S47.player.place(x, z, Math.PI); S47.tick(0.2); S47.game.d.ui.clearToasts()")
    async def use(self, iid):
        """Use the thing: E on the desktop, the Use button with a finger on the phone."""
        if self.touch:
            use = self.pg.locator('.touch .use')
            if await use.is_visible():
                bb = await use.bounding_box()
                await self.pg.touchscreen.tap(bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2); await self.tick(0.1)
                return True
            return False
        await self.ev(f"S47.game.d.inter.get('{iid}').use()"); await self.tick(0.1)
        return True
    async def lines(self): return await self.ev("[...document.querySelectorAll('.talk-line')].map(e => e.textContent).join(' | ')")
    async def choices(self): return await self.ev("[...document.querySelectorAll('.talk [data-c]')].map(b => b.dataset.c)")
    async def tap(self, sel):
        await self.pg.locator(sel).first.scroll_into_view_if_needed()
        if self.touch:
            bb = await self.pg.locator(sel).first.bounding_box()
            await self.pg.touchscreen.tap(bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2)
        else: await self.pg.click(sel)
        await self.tick(0.1)
    async def room6(self, count):
        """Chapter four, the time away count set, into room 6, the talk open."""
        ev = self.ev
        await ev("S47.game.d.ui.close(); S47.jump('chapter4'); S47.tick(1.0)"); await self.quiet()
        await ev(f"T.ab().data.count = {count}")
        await ev("S47.world.goRoom6()")
        await self.pg.wait_for_function("S47.world.area === 'room6' && !S47.world.busy", polling=200)
        await self.tick(0.3)
        await ev("S47.game.d.inter.get('r6:nora').use()"); await self.tick(0.2)
        await self.pg.wait_for_selector('.labpanel .talk')
    async def p10(self):
        await self.tap('.talk [data-chip=frame01]'); await self.tap('.talk [data-chip=frame02]'); await self.tap('.talk [data-chip=e07]')
        await self.tap('.talk [data-a=table]')
        await self.tap('.talk [data-c=lamp]')

BOX = """((n) => { const T_ = S47.three, c = S47.camera, g = T.rec(n).fig.group; c.updateMatrixWorld(true);
  const a = new T_.Vector3(g.position.x, g.position.y + 1.8, g.position.z).project(c), b = new T_.Vector3(g.position.x, g.position.y, g.position.z).project(c);
  const r = new T_.Vector3(1, 0, 0).applyQuaternion(c.quaternion).multiplyScalar(0.45), e = new T_.Vector3(g.position.x, g.position.y + 0.9, g.position.z).add(r).project(c);
  const m = new T_.Vector3(g.position.x, g.position.y + 0.9, g.position.z).project(c);
  const X = (v) => (v + 1) / 2 * innerWidth, Y = (v) => (1 - v) / 2 * innerHeight, w = Math.abs(X(e.x) - X(m.x));
  return [X(m.x) - w, Y(a.y), X(m.x) + w, Y(b.y)]; })"""
async def shows(R, n, tag):
    """The frame with the one in the jacket and the same frame without it: in the box round it, how
    many grey levels it changes at most and on average (it is seen, not only placed)."""
    ev, pg = R.ev, R.pg
    await ev("S47.game.d.ui.clearToasts()"); await pg.wait_for_timeout(250)
    box = await ev(f"{BOX}({n})")
    a = Image.open(io.BytesIO(await pg.screenshot(path=f'{OUT}/{tag}.png'))).convert('L')
    await ev(f"T.rec({n}).fig.group.scale.setScalar(1e-4); S47.tick(1 / 30)")
    b = Image.open(io.BytesIO(await pg.screenshot(path=f'{OUT}/{tag}_without.png'))).convert('L')
    await ev(f"T.rec({n}).fig.group.scale.setScalar(1); S47.tick(1 / 30)")
    bx = (max(0, int(box[0]) - 1), max(0, int(box[1]) - 1), min(a.size[0], int(box[2]) + 2), min(a.size[1], int(box[3]) + 2))
    pa, pb = a.crop(bx).tobytes(), b.crop(bx).tobytes()
    if not pa: return {'max': 0, 'part': 0, 'box': bx}
    d = [abs(x - y) for x, y in zip(pa, pb)]
    return {'max': max(d), 'part': round(sum(1 for v in d if v >= 6) / len(d), 2), 'box': bx}

async def june_page(R, tag, sizes):
    """T. Vega's field book from the drawer in the hut, at its June page: what it says, and whether the
    page holds it (at each viewport size given)."""
    ev, pg = R.ev, R.pg
    dr = await ev("(() => { const v = S47.world.site.proxies.drawer.getWorldPosition(new S47.three.Vector3()); return [v.x, v.y, v.z]; })()")
    await ev(f"S47.player.place({dr[0]}, {dr[2]} + 0.95, 0); S47.tick(0.1); T.look([{dr[0]}, {dr[1]}, {dr[2]}])")
    await ev("S47.game.d.inter.get('s1drawer').use(); S47.tick(0.6)"); await R.tick(0.6)
    if not await ev("!!document.querySelector('.fbook')"):
        await ev("S47.game.d.inter.get('s1drawer').use(); S47.tick(0.6)"); await R.tick(0.6)
    await pg.wait_for_selector('.fbook', timeout=20000)
    await R.tap('.fbook [data-a=next]'); await pg.wait_for_timeout(600)
    hand = await ev("document.querySelector('.fbook [data-hand]').textContent")
    check('Jun 27. Night sight out on C. Lost an hour. One side of my face burned.' in hand and 'Told N. it was the lamp.' in hand and 'Jun 20.' in hand,
          f'{tag}: the field book, June: the night out along C, an hour lost, one side of his face burned')
    for i, (w, h) in enumerate(sizes):
        fits = await ev("(() => { const p = document.querySelector('.fbook [data-page]').getBoundingClientRect(), h = document.querySelector('.fbook [data-hand]').getBoundingClientRect(); return [h.bottom, p.bottom]; })()")
        check(fits[0] <= fits[1] + 1, f'{tag} {w}x{h}: the June page holds all four entries ({fits[0]:.0f} <= {fits[1]:.0f})')
        await R.shot(f'{tag}_fieldbook_june_{w}x{h}')
    await R.tap('.fbook [data-a=back]'); await R.tick(0.2)

async def forced_run(R, shots, tag):
    """Out of the hut after the call; the warning there; running for the hut and the truck."""
    ev, pg = R.ev, R.pg
    trace = []
    await ev("T.walk(-9.95, 4.3)")
    await ev("T.walk(-9.95, 6.2)")
    s = await R.st()
    check(s['stage'] == 'calm' and s['due'] == 'door', f'{tag}: a step out of the door, not yet ({s["due"]})')
    await ev("T.walk(-9.95, 8.4)")
    s = await R.st()
    check(s['stage'] == 'warning' and s['forced'] and s['station'] == 'taken', f'{tag}: a step or two out of the hut, the warning, the one that cannot be got away from ({s})')
    check(await ev("S47.curve.phase") == 'rest', f'{tag}: it is the story\'s: it came with the night\'s curve in its rest')
    # the door shuts behind; the light inside goes out
    await ev("T.stand(2.4)")
    s = await R.st()
    check(s['shut'] and abs(s['leaf']) < 0.01 and not s['light'], f'{tag}: behind, the hut door has swung shut and the light inside is out ({s["leaf"]}, light {s["light"]})')
    check(any('the hut door swings shut by itself' in t for t in await ev("T.toasts")), f'{tag}: it says so')
    d = await ev("T.door()")
    await ev(f"T.look([{d[0]}, {d[1] + 0.3}, {d[2]}])")
    if shots: await R.shot(f'{tag}_01_door_shut', True)
    # the hut: cannot be walked into; the door does not give
    ok = await ev("T.walk(-9.95, 3.2, false, 0.15, 150)")
    s = await R.st()
    check(not ok and not s['inside'] and s['stage'] == 'warning', f'{tag}: back into the hut: the door is in the way, and it goes on ({await ev("T.at()")})')
    await ev(f"T.look([{d[0]}, {d[1]}, {d[2]}])")
    check(await ev("T.aimed()") == 's1hutDoor' and await ev("S47.game.d.inter.get('s1hutDoor').label()") == 'Hut door', f'{tag}: the hut door')
    await R.use('s1hutDoor')
    check('The latch gives. The door does not.' in await ev("T.last()"), f'{tag}: tried: the latch gives, the door does not ({await ev("T.last()")})')
    trace.append((await R.st())['t'])
    # out through the walk gate to the truck, at a run
    ok = await ev("T.walk(-12, 9, true) && T.walk(-22.3, 23.3, true) && T.walk(-22.3, 28.2, true) && T.walk(-21.4, 30.9, true)")
    check(ok, f'{tag}: ran through the walk gate to the truck ({await ev("T.at()")})')
    tr = await ev("T.truck()")
    await ev(f"T.look([{tr[0]}, {tr[1]}, {tr[2]}])")
    check(await ev("T.aimed()") == 's1truck' and await ev("S47.game.d.inter.get('s1truck').label()") == 'Drive back to SARO', f'{tag}: the truck says what it always says')
    await R.use('s1truck')
    s = await R.st()
    check('The keys are not in it. They are always in it.' in await ev("T.last()") and s['stage'] == 'warning' and not await ev("S47.world.driving"),
          f'{tag}: the keys are not in it; still out, still counting ({await ev("T.last()")}, t {s["t"]:.1f})')
    ex = await ev("S47.game.d.inter.examine(S47.camera)")
    check(bool(ex) and 'ignition is empty' in ex['text'], f'{tag}: looked at closely: the ignition is empty')
    if shots: await R.shot(f'{tag}_02_no_keys')
    await ev("T.stand(1.5)")
    # back in through the gate, a few steps, and back to the truck
    await ev("T.walk(-22.3, 27.6, true) && T.walk(-22.3, 22.5, true)")
    await ev("T.stand(1.2, T.yawTo(-10, 5))")
    await ev("T.walk(-22.3, 28.2, true) && T.walk(-21.4, 30.9, true)")
    s = await R.st()
    check(s['stage'] == 'warning' and s['t'] < 40, f'{tag}: at the truck again, {s["t"]:.1f} s in')
    await ev(f"T.stand({max(0.1, 40.4 - s['t'])}, T.yawTo(-10, 5))")
    s = await R.st()
    check(s['stage'] == 'warning' and not s['lamps'] and not s['torch'], f'{tag}: :40: the flashlight dies, and the lamps outside go out with it ({s["t"]:.1f} s)')
    if shots: await R.shot(f'{tag}_03_dark', True)
    # the rest of it, standing by the truck
    for _ in range(60):
        await ev("T.stand(0.25)")
        s = await R.st()
        trace.append(s['stage'])
        if s['stage'] != 'warning': break
    check(s['stage'] == 'light', f'{tag}: the light ({s["stage"]})')
    await ev("T.stand(1.5)")
    if shots: await R.shot(f'{tag}_04_light')
    for _ in range(400):
        await R.tick(0.1)
        s = await R.st()
        if s['stage'] == 'calm': break
    return s

async def desktop(b):
    ctx, pg, errs = await page(b, 1280, 720, False)
    R = Run(pg, False); ev = pg.evaluate
    await ev("S47.hold = true")

    # ======================= A: STATION 01 =======================
    # not with Missing time off (each case set up inside the hut, then out of it)
    await R.station(3 * 3600 + 58 * 60)
    await ev("S47.setAbductions(false)")
    await june_page(R, 'desk', [(1280, 720)])
    await ev("const [x, z] = T.w(-11, 3.2); S47.player.place(x, z, Math.PI); S47.tick(0.2); S47.game.d.ui.clearToasts()")
    await ev("S47.setAbductions(false); S47.curve.force('rest')")
    await ev("T.walk(-9.95, 4.3) && T.walk(-9.95, 9.5)"); await ev("T.stand(2)")
    s = await R.st()
    check(s['stage'] == 'calm' and s['due'] == 'off' and not s['shut'] and abs(s['leaf'] + 1.75) < 0.01, f'Missing time off: out of the hut after the call, nothing ({s["due"]})')
    async def back_in(): return await ev("T.walk(-9.95, 4.3) && T.walk(-11, 3.2)")
    # not after 04:16: no room for the lost time before 04:35
    await back_in(); await ev("S47.game.clock = 4 * 3600 + 17 * 60; S47.setAbductions(true)")
    await ev("T.walk(-9.95, 4.3) && T.walk(-9.95, 9.5)"); await ev("T.stand(2)")
    s = await R.st()
    check(s['stage'] == 'calm' and s['due'] == 'late', f'after 04:16 (the lost time would run past 04:35): let go ({s["due"]})')
    # not when taken already tonight
    await back_in(); await ev("S47.game.clock = 3 * 3600 + 58 * 60; T.ab().data.count = 1")
    await ev("T.walk(-9.95, 4.3) && T.walk(-9.95, 9.5)"); await ev("T.stand(2)")
    s = await R.st()
    check(s['stage'] == 'calm' and s['due'] == 'done', f'taken already tonight: nothing here ({s["due"]})')
    # not before the call
    await back_in(); await ev("T.ab().data.count = 0; S47.ch3.s.called = false; S47.ch3.s.stage = 'call'")
    await ev("T.walk(-9.95, 4.3) && T.walk(-9.95, 9.5)"); await ev("T.stand(2)")
    s = await R.st()
    check(s['stage'] == 'calm' and s['due'] == 'story', f'before N. Vega\'s call: nothing ({s["due"]})')
    await back_in(); await ev("S47.ch3.s.called = true; S47.ch3.s.stage = 'return'; T.toasts.length = 0")
    check(not await ev("T.seen('abduction').length"), 'none of these had a warning')

    # Missing time turned off on the way: it passes, and the station is put back; never again
    await ev("S47.curve.force('rest')")
    await ev("T.walk(-9.95, 4.3) && T.walk(-9.95, 8.4)"); await ev("T.stand(3)")
    s = await R.st()
    check(s['stage'] == 'warning' and s['shut'], 'the warning, the door shut')
    await ev("S47.setAbductions(false)"); await ev("T.stand(0.5)")
    s = await R.st()
    check(s['stage'] == 'calm' and not s['shut'] and s['light'] and s['lamps'] and not s['keys'] and abs(s['leaf'] + 1.75) < 0.01 and s['station'] == 'passed' and s['count'] == 0,
          f'Missing time turned off on the way: it passes, the door open, the light on, the keys back ({s})')
    await ev("S47.setAbductions(true); T.walk(-9.95, 4.3) && T.walk(-11, 3.2)")
    await ev("T.walk(-9.95, 4.3) && T.walk(-9.95, 9.5)"); await ev("T.stand(2)")
    s = await R.st()
    check(s['stage'] == 'calm' and s['due'] == 'done', f'and not again that night ({s["due"]})')

    # the whole of it
    await R.station(3 * 3600 + 58 * 60)
    await ev("T.ab().data.station = undefined; delete T.ab().data.station; T.ab().data.count = 0; T.ab().data.last = -1e9; S47.curve.force('rest'); T.toasts.length = 0")
    # her call as the game makes it, not only the flag: P09 recorded, the ring, the handset, her
    # lines, the line quiet (the recorded call plays in real time; the ticks here run ahead of it)
    await ev("S47.ch3.s.called = false; S47.ch3.s.stage = 'call'; S47.ch3.callT = S47.game.gt + 0.5")
    for _ in range(20):
        await R.tick(0.25)
        if await ev("S47.ch3.ringing"): break
    await ev("""(() => { const s = T.site(), p = S47.player, ph = s.proxies.fieldPhone.getWorldPosition(new S47.three.Vector3()), h = s.hutBounds;
      const cx = (h.minX + h.maxX) / 2, cz = (h.minZ + h.maxZ) / 2, d = Math.hypot(cx - ph.x, cz - ph.z), x = ph.x + (cx - ph.x) / d * 1.1, z = ph.z + (cz - ph.z) / d * 1.1;
      p.place(x, z, Math.atan2(-(ph.x - x), -(ph.z - z))); S47.tick(0.05); T.look([ph.x, ph.y, ph.z]); })()""")
    rang = await ev("S47.ch3.ringing") and await ev("T.aimed()") == 's1phone'
    await R.use('s1phone')
    for _ in range(80):
        await R.tick(0.5)
        if await ev("S47.ch3.s.called"): break
    await pg.wait_for_function("!S47.game.d.audio.phonePlaying", polling=200, timeout=60000)
    await R.tick(0.2)
    s = await R.st()
    check(rang and await ev("S47.ch3.s.stage") == 'return' and s['stage'] == 'calm' and s['due'] == 'inside' and any('The line goes quiet' in t for t in await ev("T.toasts")),
          f'N. Vega rings the field telephone and says her piece; in the hut after it, nothing yet ({s["due"]})')
    await ev("const [x, z] = T.w(-11, 3.2); S47.player.place(x, z, Math.PI); S47.tick(0.1)")
    s = await forced_run(R, True, 'a')
    check(s['stage'] == 'calm' and s['count'] == 1, f'taken, and up again ({s})')
    sp = await ev("(() => { const s = T.ab().data.spots.at(-1); return { station: s.station, n: s.path ? s.path.length / 3 : 0, take: s.take, back: s.back, scene: s.scene, p0: s.path && s.path.slice(0, 3), pn: s.path && s.path.slice(-3) }; })()")
    check(sp['station'] and sp['scene'] == 'station01' and 186 <= sp['n'] <= 190, f'kept with the case: the station\'s, and its 47 s as a path of {sp["n"]} quarter seconds')
    o = await ev("[T.o().x, T.o().z]")
    out = math.hypot(sp['p0'][0] - o[0] + 9.95, sp['p0'][1] - o[1] - 4.76)
    check(2.3 <= out <= 3.0 and math.hypot(sp['pn'][0] - sp['take'][0], sp['pn'][1] - sp['take'][2]) < 0.2,
          f'from a step out of the hut door to where the light came ({sp["p0"]}, {sp["pn"]})')
    check(s['light'] and s['lamps'] and not s['shut'] and not s['keys'] and abs(s['leaf'] + 1.75) < 0.01, f'the door open into the room again, the light on, the lamps on, the keys back ({s})')
    check(any('keys are lying in the frost by your hand' in t for t in await ev("T.toasts")), 'the keys in the frost by your hand (a toast)')
    nn = await ev("T.notes('on the way out from the hut after N. Vega')")
    check(len(nn) == 1 and 'swung shut behind me by itself' in nn[0] and 'keys were not in the truck' in nn[0], f'a note: the door, the light, the keys ({nn[0][:90] if nn else None})')
    w = await ev("T.seen('abduction', 'warning')")
    check([x['end'] for x in w] == ['away', 'taken'] and w[-1]['line'] == 'C', f'the seen log: the one let pass, and this one, taken, on C ({w})')
    ex = await ev("(() => { const t = T.truck(); T.look(t); const e = S47.game.d.inter.examine(S47.camera); return e && e.text; })()")
    await R.shot('a_05_up', True)

    # ======================= B: the recording at the station =======================
    # the curve is in its rest after the warning: from the far corner of the yard, nothing walks
    take = sp['take']
    await ev("T.walk(-22.3, 28.2, true) && T.walk(-22.3, 23.3, true) && T.walk(25, 0, true) && T.walk(28.4, -22.4)")
    d = await ev(f"Math.hypot(S47.player.pos.x - {take[0]}, S47.player.pos.z - {take[2]})")
    await ev(f"S47.player.yaw = T.yawTo(60, -60); S47.tick(0.1)")
    ph = await ev("S47.curve.phase")
    await ev("T.stand(4)")
    ws = await ev("T.walkSt()")
    check(ph == 'rest' and ws['phase'] == 'idle' and ws['waiting'] and await ev("S47.curve.pulled && S47.curve.pullKind === 'scare'"),
          f'in the curve\'s rest, {d:.0f} m from where I was taken: it does not walk, but it waits, and the curve hurries for it ({ph}, {ws})')
    # the rest keeps its first three minutes; then the hurried build, and it comes
    await ev("S47.curve.t = Math.max(S47.curve.t, 178)")
    began = None
    for i in range(140):
        await ev("T.stand(0.5)")
        ws = await ev("T.walkSt()")
        if ws['phase'] != 'idle': began = (i, await ev("S47.curve.phase"), await ev("S47.curve.level")); break
    check(began is not None and began[1] in ('build', 'peak') and (began[1] == 'peak' or began[2] >= 0.5), f'after the breath, the hurried build: it walks ({began})')
    check(await ev("S47.oneAtATime()") and not await ev("S47.curve.may('glimpse')") and not await ev("S47.curve.may('abduct')"), 'one at a time: while it walks nothing else of the night\'s may start')
    path = await ev("T.rec(1).path")
    plen = await ev(f"S47.recPath.len(T.rec(1).path)")
    f0, w0 = await ev("T.fig(1)"), await ev("T.walkSt()")
    at0 = await ev(f"S47.recPath.at(T.rec(1).path, {w0['shown']})")
    check(f0['vis'] and math.hypot(f0['x'] - at0[0], f0['z'] - at0[1]) < 0.05, f'it is there, where I was then ({w0["shown"]:.2f} s into the 47)')
    head = await ev("T.head(1)")
    check(not (await ev(f"T.px({json.dumps(head)})"))[2], 'it came where I was not looking')
    wl = await ev("T.seen('scare', 'walk 1')")
    check(len(wl) == 1 and wl[0]['looking'] is False and wl[0]['area'] == 'station01', f'the seen log: walk 1, not looked at yet ({wl})')
    # follow it with the eyes, along the path
    def seg(p, a, b):
        return math.hypot(p[b * 3] - p[a * 3], p[b * 3 + 1] - p[a * 3 + 1])
    moved = [seg(path, i, i + 4) for i in range(0, len(path) // 3 - 4)]
    still = [i for i in range(len(moved)) if moved[i] < 0.05]
    runs = [i for i in range(len(moved)) if moved[i] > 2.6]
    samples = []
    for k in range(320):
        await ev("(() => { const h = T.head(1); const p = S47.player, c = S47.camera.position; p.yaw = Math.atan2(-(h[0] - p.pos.x), -(h[2] - p.pos.z)); p.pitch = Math.atan2(h[1] - c.y, Math.hypot(h[0] - c.x, h[2] - c.z)); S47.tick(0.25); })()")
        ws = await ev("T.walkSt()")
        f = await ev("T.fig(1)")
        samples.append((ws['phase'], ws['shown'], f['x'], f['z'], f['vis'], ws['loops']))
        if k == 30:
            await ev("S47.player.pitch += 0.03; S47.tick(1 / 30)")
            sv = await shows(R, 1, 'b_01_station_walk')
        if k == 31:
            # the same moment, nearer: a narrower view, as the game's zoom has it
            await ev("T.fov0 = S47.camera.fov; S47.camera.fov = T.fov0 * 0.45; S47.camera.updateProjectionMatrix(); S47.game.d.ui.clearToasts(); S47.tick(1 / 30)"); await pg.wait_for_timeout(250)
            await pg.screenshot(path=f'{OUT}/b_01b_station_walk_zoom.png')
            await ev("S47.camera.fov = T.fov0; S47.camera.updateProjectionMatrix(); S47.tick(1 / 30)")
        if ws['phase'] == 'idle' or (ws['loops'] >= 1 and ws['phase'] == 'walk' and ws['shown'] > 1.0): break
    wl = await ev("T.seen('scare', 'walk 1')")
    check(sv['max'] >= 25 and sv['part'] >= 0.05, f'at the station it is seen: against the lit yard, from {await ev("Math.round(Math.hypot(S47.player.pos.x - T.fig(1).x, S47.player.pos.z - T.fig(1).z))")} m ({sv})')
    check(wl and wl[0]['looking'] is True and (await ev("S47.curve.landed") or {}).get('what') == 'walk 1' and await ev("S47.curve.phase") == 'rest', f'looked at: the log has it, and it lands the peak ({wl}, {await ev("S47.curve.landed")})')
    errs_pos = [math.hypot(x - a[0], z - a[1]) for (ph, sh, x, z, v, _), a in zip(samples, [await ev(f"S47.recPath.at(T.rec(1).path, {s[1]})") for s in samples]) if ph == 'walk']
    check(errs_pos and max(errs_pos) < 0.08, f'all the way it is where I was at that second (off by {max(errs_pos) if errs_pos else None:.3f} m at most)')
    def where(i): return (path[i * 3], path[i * 3 + 1])
    if still:
        i = still[len(still) // 2]; t = i * 0.25
        near = [s for s in samples if s[0] == 'walk' and t <= s[1] <= t + 1.0]
        mv = max((math.hypot(s[2] - where(i)[0], s[3] - where(i)[1]) for s in near), default=None)
        check(mv is not None and mv < 0.15, f'where I stood still, it stands still ({t:.1f} s: moved {mv})')
    else: check(False, 'where I stood still, it stands still (no stand on the path)')
    if runs:
        rs = [s for s in samples if s[0] == 'walk' and runs[0] * 0.25 + 0.2 <= s[1] <= runs[0] * 0.25 + 1.2]
        sp_ = (math.hypot(rs[-1][2] - rs[0][2], rs[-1][3] - rs[0][3]) / max(0.01, rs[-1][1] - rs[0][1])) if len(rs) >= 2 else 0
        check(2.4 <= sp_ <= 3.6, f'where I ran, it runs ({sp_:.2f} m/s)')
    else: check(False, 'where I ran, it runs (no run on the path)')
    ph_seq = []
    for s in samples:
        if not ph_seq or ph_seq[-1] != s[0]: ph_seq.append(s[0])
    gone_vis = [s[4] for s in samples if s[0] == 'gone']
    restart = [s for s in samples if s[5] >= 1 and s[0] == 'walk']
    check(ph_seq[:4] == ['walk', 'stand', 'gone', 'walk'] and gone_vis and not any(gone_vis), f'at the end it stands, is not there, and starts over ({ph_seq})')
    if restart:
        p0 = await ev("S47.recPath.at(T.rec(1).path, 0)")
        check(math.hypot(restart[0][2] - p0[0], restart[0][3] - p0[1]) < 1.5, 'back where I was when the crickets stopped, a step out of the hut')
    st_ = [s for s in samples if s[0] == 'stand']
    check(st_ and max(math.hypot(s[2] - take[0], s[3] - take[2]) for s in st_) < 0.2, 'it stands where the light came for me')
    # walked up to: it was not there
    f = await ev("T.fig(1)")
    ok, dd = False, None
    for _ in range(400):
        dd = await ev(f"Math.hypot(S47.player.pos.x - {f['x']}, S47.player.pos.z - {f['z']})")
        await ev(f"T.walk({f['x']}, {f['z']}, true, 0.3, 6, true)")
        ws = await ev("T.walkSt()")
        if ws['phase'] == 'idle': ok = True; break
        f = await ev("T.fig(1)")
    check(ok and ws['ended'] == 'near' and not (await ev("T.fig(1)"))['vis'] and dd is not None and dd <= 47, f'walked up to: it is not there ({ws["ended"]}, {dd:.1f} m)')
    wl = await ev("T.seen('scare', 'walk 1')")
    check(wl and wl[0]['end'] == 'near', 'the log: walked up to')
    nn = await ev("T.notes('Someone in a SARO jacket, out where I was taken')")
    check(len(nn) == 1 and 'stopped where I stopped, ran where I ran' in nn[0] and 'doing it again' in nn[0], f'a note ({nn[0][:100] if nn else None})')
    wk = await ev("T.ab().data.spots[0].walked")
    check(isinstance(wk, (int, float)), f'kept with the case: it has walked ({wk})')
    # once: not again, at the peak, from the far corner
    await ev("T.walk(28.4, -22.4)"); await ev("S47.player.yaw = T.yawTo(60, -60); S47.curve.force('peak'); T.stand(4)")
    ws = await ev("T.walkSt()")
    check(ws['phase'] == 'idle' and not ws['waiting'], f'once: at the next peak, from the same corner, it does not walk again ({ws})')
    # Continue
    await R.tick(1.0)
    await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(600)
    await pg.reload()
    await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
    await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
    await ev(HELPERS); await ev("T.hook(); S47.hold = true; S47.tick(0.5)"); await R.quiet()
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    s = await R.st()
    sp2 = await ev("T.ab().data.spots.map((x) => ({ walked: x.walked, n: x.path ? x.path.length / 3 : 0, station: x.station }))")
    check(await ev("S47.world.area") == 'station01' and s['count'] == 1 and s['station'] == 'taken' and sp2[0]['n'] == sp['n'] and sp2[0]['walked'] == wk and sp2[0]['station'],
          f'Continue: back at the station, the time away kept with its path, walked ({sp2})')
    await ev("const [x, z] = T.w(-11, 3.2); S47.player.place(x, z, Math.PI); S47.tick(0.2)")
    await ev("T.walk(-9.95, 4.3) && T.walk(-9.95, 9.5)"); await ev("T.stand(2)")
    s = await R.st()
    check(s['stage'] == 'calm' and not s['shut'] and s['due'] == 'done', f'Continue: out of the hut again, nothing ({s["due"]})')
    await ev("T.walk(25, 0) && T.walk(28.4, -22.4)"); await ev("S47.player.yaw = T.yawTo(60, -60); S47.curve.force('peak'); T.stand(4)")
    check((await ev("T.walkSt()"))['phase'] == 'idle', 'Continue: and the recording does not walk again')

    # ======================= B: the highway shoulder south of SARO, chapter four, on the way to room 6 =======================
    await ev("S47.game.d.ui.close(); S47.jump('chapter4'); S47.tick(1.0); S47.world.enter('saro'); S47.curve.force('rest'); S47.tick(0.2)"); await R.quiet()
    # a time away on the shoulder, with a path laid in at the player's own speeds: a walk south, a
    # stand turning round, a run back north, a stand, a few running steps off into the grass, still
    await ev("""(() => { const a = T.ab(), path = []; let x = -31, z = 200, yaw = Math.PI;
      for (let i = 0; i <= 188; i++) { const t = i * 0.25;
        if (t < 10) { z += 1.9 * 0.25; yaw = Math.PI; } else if (t < 14) { yaw = Math.PI * (1 - Math.min(1, (t - 10) / 2)); }
        else if (t < 25) { z -= 3.0 * 0.25; yaw = 0; } else if (t < 29) { yaw = 0.9; } else if (t < 32) { x -= 3.0 * 0.25; yaw = Math.PI / 2; }
        path.push(+x.toFixed(2), +z.toFixed(2), +yaw.toFixed(2)); }
      const y = S47.player.floor ? S47.player.floor(x, z) : 0;
      a.data.spots.push({ scene: 'saro', take: [x, y, z], back: [x, y, z + 150], clock: 3 * 3600 + 3 * 60, path });
      a.data.count = a.data.spots.length; a.restore(JSON.parse(JSON.stringify(a.data))); return true; })()""")
    await ev("S47.player.place(-28.5, 125.0, 0); S47.flashlight.have = true; S47.flashlight.on = true; S47.tick(0.2)")
    d = await ev("(() => { const t = T.ab().data.spots.at(-1).take; return Math.hypot(S47.player.pos.x - t[0], S47.player.pos.z - t[2]); })()")
    await ev("T.stand(3)")
    check((await ev("T.walkSt()"))['phase'] == 'idle' and await ev("S47.curve.phase") == 'rest', f'chapter four, on the shoulder {d:.0f} m from where I was taken: never in a rest')
    await ev("S47.setAbductions(false); S47.curve.force('peak'); T.stand(2)")
    check((await ev("T.walkSt()"))['phase'] == 'idle', 'with Missing time off: not at the peak either')
    await ev("S47.setAbductions(true); S47.curve.force('peak'); T.stand(1)")
    ws = await ev("T.walkSt()")
    check(ws['phase'] == 'walk' and ws['n'] == 2, f'at the peak: the second time away walks, behind me ({ws})')
    for k in range(80):
        await ev("(() => { const h = T.head(2); const p = S47.player, c = S47.camera.position; p.yaw = Math.atan2(-(h[0] - p.pos.x), -(h[2] - p.pos.z)); p.pitch = Math.atan2(h[1] - c.y, Math.hypot(h[0] - c.x, h[2] - c.z)); S47.tick(1 / 6); })()")
    band = await ev("T.rec(2).fig.band.color.r")
    check(band > 1.2, f'the flashlight on it: the reflective band gives the light back ({band:.2f})')
    # (the eye a hair above it, so the dot in the middle of the view is not on it)
    await ev("S47.player.pitch += 0.03; S47.tick(1 / 30)")
    v = await shows(R, 2, 'b_02_shoulder_walk')
    check(v['max'] >= 40 and v['part'] >= 0.05, f'and it is seen in the flashlight: the band bright in the dark ({v})')
    await ev("S47.flashlight.on = false; T.stand(0.2); S47.player.pitch += 0.03; S47.tick(1 / 30)")
    band0 = await ev("T.rec(2).fig.band.color.r")
    v0 = await shows(R, 2, 'b_03_shoulder_walk_dark')
    check(band0 < 0.8, f'the flashlight off: the band is only grey, a shape in the dark ({band0:.2f}, {v0})')
    await ev("S47.flashlight.on = true; T.stand(0.2)")
    f = await ev("T.fig(2)"); hp = await ev("T.px(T.head(2))")
    check(f['vis'] and hp[2], f'down the shoulder: someone in a SARO jacket, walking ({hp[0]:.0f}, {hp[1]:.0f} on the screen)')
    gl = await ev("T.rec(2).fig.glints.map((g) => g.visible)")
    check(all(gl) and await ev("T.rec(2).fig.glintMat.opacity") > 0.5, f'in the beam the tape glints: across the chest and at the sleeves ({gl})')
    # the flashlight off, out past 60 m: a dark shape on dark ground has not been seen, however the eyes are turned
    await ev("S47.flashlight.on = false; T.stand(0.2)")
    l0 = (await ev("T.walkSt()"))['looked']
    for k in range(12):
        await ev("(() => { const h = T.head(2); const p = S47.player, c = S47.camera.position; p.yaw = Math.atan2(-(h[0] - p.pos.x), -(h[2] - p.pos.z)); p.pitch = Math.atan2(h[1] - c.y, Math.hypot(h[0] - c.x, h[2] - c.z)); S47.tick(1 / 6); })()")
    l1 = (await ev("T.walkSt()"))['looked']
    dd = await ev("Math.hypot(S47.player.pos.x - T.fig(2).x, S47.player.pos.z - T.fig(2).z)")
    check(dd > 60 and l1 - l0 < 0.05 and await ev("S47.recWalk.at === null") and not any(await ev("T.rec(2).fig.glints.map((g) => g.visible)")),
          f'in the dark at {dd:.0f} m it is not counted as looked at, and nothing glints (+{l1 - l0:.2f} s)')
    await ev("S47.flashlight.on = true; T.stand(0.2)")
    # gone away from before it was ever made out: it walks another time the player is back (once more)
    await ev("S47.recWalk.reset(); delete T.ab().data.spots[1].walked; S47.flashlight.on = false; S47.player.place(-28.5, 125.0, 0); S47.tick(0.1)")
    await ev("S47.curve.force('peak'); T.stand(1)")
    w1 = await ev("T.walkSt()")
    await ev("S47.player.place(-28.5, 125.0 + 560, 0); T.stand(0.5)")
    w2 = await ev("T.walkSt()"); k1 = await ev("T.ab().data.spots[1].walked")
    await ev("S47.player.place(-28.5, 125.0, 0); S47.curve.force('peak'); T.stand(1)")
    w3 = await ev("T.walkSt()")
    await ev("S47.player.place(-28.5, 125.0 + 560, 0); T.stand(0.5)")
    k2 = await ev("T.ab().data.spots[1].walked")
    check(w1['phase'] == 'walk' and w2['ended'] == 'left' and k1 is None and w3['phase'] == 'walk' and isinstance(k2, (int, float)),
          f'left before it was made out: it walks again when I am back, once ({w1["phase"]}, {w2["ended"]}, {k1}, {w3["phase"]}, {k2})')
    await ev("S47.flashlight.on = true; S47.player.place(-28.5, 125.0, 0); S47.tick(0.1)")

    # ======================= C: the mark =======================
    # no time away: P10, and her table as it always was
    await R.room6(0)
    await R.p10()
    ch = await R.choices()
    check('sit' not in ch and 'tomas' in ch and not await ev("!!S47.ch4.s.mark"), f'not taken: P10, and on as always ({ch})')
    # taken since: on coming back to her table, she sees it
    await ev("S47.game.d.ui.close(); T.ab().data.count = 1"); await R.tick(0.2)
    await ev("S47.game.d.inter.get('r6:nora').use()"); await R.tick(0.2)
    ln = await R.lines()
    check('She looks up when you come to the table' in ln and 'You have been out on the line.' in ln and await ev("S47.ch4.s.mark"), f'taken after P10: back at her table she sees it ({ln[:80]})')
    # taken before: P10, the chair, then the lamp
    await R.room6(1)
    await R.p10()
    ln = await R.lines(); ch = await R.choices()
    check(ch == ['sit'] and 'A reference that stays on the film' in ln and 'pushes a chair out' in ln, f'taken tonight: P10, and a chair pushed out (only: Sit down) ({ch})')
    await pg.wait_for_timeout(300)
    voice = await ev("(() => { const b = document.querySelector('[data-reading-kind=character-dialogue] button'); return b ? !b.disabled : null; })()")
    check(voice is not False, f'the P10 lines keep their recorded voice ({voice})')
    await R.tap('.talk [data-c=sit]')
    ln = await R.lines()
    check(all(x in ln for x in ['You have been out on the line.', 'One side of your face.', 'Tomás came back from it like that in June.', 'hot and tight']) and await ev("S47.ch4.s.mark"),
          f'sat down: under her lamp, the side of the face ({ln[:120]})')
    await pg.wait_for_timeout(3600)
    st_txt = await ev("(() => { const e = document.querySelector('[data-reading-kind=character-dialogue] [data-reading-status]'); return e ? e.textContent : null; })()")
    check(st_txt is None or 'No recording' in st_txt, f'her new lines have no recording, and say so ({st_txt})')
    await R.shot('c_01_nora_mark')
    nn = await ev("T.notes('One side of your face')")
    check(len(nn) == 1 and 'Tomás came back from it like that in June' in nn[0], 'a note')
    ch = await R.choices()
    check('tomas' in ch and 'record' in ch, f'then on as always ({ch})')
    await R.tap('.talk [data-c=tomas]')
    await ev("S47.game.d.ui.close()"); await R.tick(0.2)
    await ev("S47.game.d.inter.get('r6:nora').use()"); await R.tick(0.2)
    check('out on the line' not in await R.lines() and len(await ev("T.notes('One side of your face')")) == 1 and await ev("S47.ch4.s.mark"), 'once: not again')
    await ev("S47.game.d.ui.close()")

    # the deputy: dried blood under his nose after half past four, not before
    async def deputy(clock):
        await ev("S47.game.d.ui.close()")
        await ev("async () => { await S47.world.prepare('diner'); S47.world.enter('diner'); }")
        await ev(f"S47.game.clock = {clock}; S47.world.town.update(S47.runTime(), S47.game.clock); S47.tick(0.05)")
        await ev("S47.game.d.inter.get('town:sheriffDoor').use()"); await R.tick(0.05)
        await pg.wait_for_selector('.labpanel .talk')
        return await R.lines(), await R.choices()
    ln, ch = await deputy(2 * 3600 + 30 * 60)
    check('dried blood' not in ln and 'nose' not in ch, f'the deputy at 02:30, before he goes out: nothing under his nose ({ch})')
    ln, ch = await deputy(4 * 3600 + 40 * 60)
    check('dried blood under his nose' in ln and 'does not seem to know' in ln and 'nose' in ch, f'the deputy at 04:40, back: dried blood under his nose, and he has not noticed ({ch})')
    await pg.wait_for_timeout(2600)
    await R.shot('c_02_deputy')
    await R.tap('.talk [data-c=nose]')
    ln = await R.lines()
    check('You have blood under your nose.' in ln and 'I drove the loop. I came straight back.' in ln and 'does not wipe it off' in ln, f'told: he touches it, and says what he always says ({ln[:100]})')
    check(len(await ev("T.notes('dried blood under his nose')")) == 1, 'a note')
    await ev("S47.game.d.ui.close()")

    print('\n'.join(errs[:30]) or 'desktop: no console errors/warnings')
    check(not errs, 'desktop: console clean')
    await ctx.close()

async def phone(b):
    ctx, pg, errs = await page(b, 412, 892, True)
    R = Run(pg, True); ev = pg.evaluate
    await ev("S47.hold = true")
    # T. Vega's field book: the June night on its page, and the page holds it (upright and on its side)
    await R.station(3 * 3600 + 58 * 60)
    await ev("S47.setAbductions(false)")
    await june_page(R, 'phone', [(412, 892)])
    await ev("S47.setAbductions(true)")
    await ev("S47.ch3.s.called = true; S47.ch3.s.stage = 'return'; const [x, z] = T.w(-11, 3.2); S47.player.place(x, z, Math.PI); S47.tick(0.2)")
    await ev("T.ab().data.count = 0; delete T.ab().data.station; S47.curve.force('rest'); T.toasts.length = 0")
    # A on the phone
    await ev("T.walk(-9.95, 4.3) && T.walk(-9.95, 8.4)"); await ev("T.stand(2.4)")
    s = await R.st()
    check(s['stage'] == 'warning' and s['forced'] and s['shut'], 'phone: out of the hut after the call, the warning, the door shut behind')
    d = await ev("T.door()")
    await ev(f"S47.player.yaw = T.yawTo({d[0]}, {d[2]}, true); S47.tick(0.05); T.walk({d[0]}, {d[2]} + 1.3, false, 0.15, 90, true); T.look([{d[0]}, {d[1]}, {d[2]}])")
    used = await R.use('s1hutDoor')
    check(used and 'The latch gives' in await ev("T.last()"), f'phone: Use on the hut door: it does not give ({await ev("T.last()")})')
    await R.shot('p_02_door_shut')
    await ev("T.walk(-12, 9, true) && T.walk(-22.3, 23.3, true) && T.walk(-22.3, 28.2, true) && T.walk(-21.4, 30.9, true)")
    tr = await ev("T.truck()")
    await ev(f"T.look([{tr[0]}, {tr[1]}, {tr[2]}])")
    used = await R.use('s1truck')
    check(used and 'The keys are not in it' in await ev("T.last()") and not await ev("S47.world.driving"), 'phone: Use on the truck: no keys')
    await R.shot('p_03_no_keys')
    s = await R.st()
    await ev(f"T.stand({max(0.1, 44 - s['t'])})")
    for _ in range(400):
        await R.tick(0.1)
        s = await R.st()
        if s['stage'] == 'calm': break
    check(s['stage'] == 'calm' and s['count'] == 1 and s['light'] and s['lamps'] and not s['keys'], f'phone: taken, and up; the station as always ({s["stage"]})')
    # B on the phone: from the far corner, at a peak
    await ev("T.walk(-22.3, 28.2, true) && T.walk(-22.3, 23.3, true) && T.walk(25, 0, true) && T.walk(28.4, -22.4)"); await ev("S47.player.yaw = T.yawTo(60, -60); S47.curve.force('peak'); T.stand(2)")
    ws = await ev("T.walkSt()")
    check(ws['phase'] == 'walk', f'phone: it walks ({ws["phase"]})')
    for k in range(40):
        await ev("(() => { const h = T.head(1); const p = S47.player, c = S47.camera.position; p.yaw = Math.atan2(-(h[0] - p.pos.x), -(h[2] - p.pos.z)); p.pitch = Math.atan2(h[1] - c.y, Math.hypot(h[0] - c.x, h[2] - c.z)); S47.tick(1 / 6); })()")
    hp = await ev("T.px(T.head(1))")
    await ev("S47.player.pitch += 0.03; S47.tick(1 / 30)")
    pv = await shows(R, 1, 'p_04_walk')
    check((await ev("T.fig(1)"))['vis'] and hp[2] and pv['max'] >= 25, f'phone: seen walking ({hp[0]:.0f}, {hp[1]:.0f}, {pv})')
    # C on the phone
    await R.room6(1)
    await R.p10()
    await R.tap('.talk [data-c=sit]')
    await pg.wait_for_timeout(3600)
    ln = await R.lines()
    check('One side of your face.' in ln, 'phone: N. Vega, under her lamp')
    await ev("(() => { const l = [...document.querySelectorAll('.talk-line')].pop(); if (l) l.scrollIntoView({ block: 'end' }); })()"); await pg.wait_for_timeout(300)
    await R.shot('p_05_nora_mark')
    await ev("S47.game.d.ui.close()")
    print('\n'.join(errs[:30]) or 'phone: no console errors/warnings')
    check(not errs, 'phone: console clean')
    await ctx.close()

async def landscape(b):
    """A phone on its side (844x390, touch): the June page of the field book holds its four entries."""
    ctx, pg, errs = await page(b, 844, 390, True)
    R = Run(pg, True); ev = pg.evaluate
    await ev("S47.hold = true")
    await R.station(3 * 3600 + 58 * 60)
    await ev("S47.setAbductions(false)")
    await june_page(R, 'side', [(844, 390)])
    print('\n'.join(errs[:30]) or 'side: no console errors/warnings')
    check(not errs, 'phone on its side: console clean')
    await ctx.close()

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        which = os.environ.get('F_AFTER', 'both')
        if which in ('both', 'desktop'): await desktop(b)
        if which in ('both', 'phone'): await phone(b)
        if which in ('both', 'phone', 'side'): await landscape(b)
        await b.close()
    json.dump(checks, open(f'{OUT}/checks-aftermath.json', 'w'), indent=1)
    fails = [c for c in checks if c[0] == 'FAIL']
    print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
    sys.exit(1 if fails else 0)

asyncio.run(main())
