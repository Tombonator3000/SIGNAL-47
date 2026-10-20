# One pressure for the whole night (story/curve.ts; FORSLAG.md, 20 October, item 1; HENDELSER.md,
# section 4): plain night, the signs piling up, a peak, then a rest in which nothing can happen.
#   - the prologue: no curve; chapter one: ten minutes of plain night in which nothing comes
#     (the calls, the glimpses, the cutout, the night's events, the time away all asked to come)
#   - the build: the crickets thin out, a sign (the flashlight stutters), one call out in the
#     dark (and no second), the cutout turns up out of the view (one in a build), and goes again
#     unfound when the build is over (never at the peak's moment); the night's events from half way
#   - the peak: a glimpse that is missed does not count, and the peak goes on; one that is looked
#     at lands it, and the rest begins
#   - the rest: 8 to 12 minutes, really quiet (everything asked again: nothing), the crickets back;
#     the developer's "now" still works; then the next build, a little stronger
#   - the time away waits at the top of its pressure until the peak; Missing time off still turns
#     it off, and the glimpse is still a peak then
#   - 47 seconds: the silence lands a peak, the rest waits for it to be over, and no second extra
#     47 in that chapter (the next chapter may); the one-at-a-time rule
#   - held: a panel open, Nora's room; no sign of the curve while one of the night's things is going
#   - a save from the prologue still gets the plain ten minutes; an older case a short quiet
#   - the cutout's twist, once: driving in a build after the cutout has been met twice, the same
#     shape at the end of the headlights; when the lights reach it they stutter and it is a
#     reflector post, with the note; not again in the next build, nor after Continue; on a phone
#     held upright the lights reach it sooner, so the post is still in the picture after the blink
#   - the curve is kept with the case; on S47 and the ?debug overlay
#   - desktop 1280x720 and a phone 412x892 with touch; no console errors or warnings
# Usage (from web/): python3 tools/f_curve.py OUTDIR
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
BASE = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
URL = BASE + ('&' if '?' in BASE else '?') + 'debug'
ARGS = ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required']
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

A = "S47.abduction()"
C = "S47.curve"
HELPERS = """(() => {
  window.T = {
    look(tx, ty, tz, off = 0) {
      const pl = S47.player, c = S47.camera.position;
      pl.yaw = Math.atan2(-(tx - c.x), -(tz - c.z)) + off * Math.PI / 180;
      pl.pitch = Math.atan2(ty - c.y, Math.hypot(tx - c.x, tz - c.z)); S47.tick(0.05);
      return true;
    },
    of: (kind, what) => S47.seen.filter((e) => e.kind === kind && (what === undefined || e.what === what)).map((e) => ({ ...e })),
    may: () => Object.fromEntries(['sign', 'call', 'cutout', 'post', 'event', 'long', 'glimpse', 'abduct', 'scare'].map((k) => [k, S47.curve.may(k)])),
    // what has happened so far, of the things the curve holds back
    counts: () => ({ calls: S47.calls.count, glimpse: S47.glimpses.count, cutPlaced: S47.cutouts.placed, cutUp: S47.cutouts.up, ne: S47.nightEvents.last && S47.nightEvents.last.id,
      stage: S47.abduction().stage, seen: S47.seen.length, town: S47.townEvents.now, signs: S47.curveSigns.length }),
    // ask everything to come now, the way it would by itself (its own timer run out)
    askAll() {
      S47.calls.next = 0; S47.glimpses['wait'] = 0; S47.cutouts['wait'] = 0; S47.nightEvents['wait'] = 0;
      const a = S47.abduction(); a.data.last = -1e9; a.pressure = 5; S47.curve['signT'] = 0;
    },
    // and back to their own timers, far off (one thing at a time from here on)
    calm() {
      S47.calls.next = 1e9; S47.glimpses['wait'] = 1e9; S47.cutouts['wait'] = 1e9; S47.nightEvents['wait'] = 1e9;
      const a = S47.abduction(); a.pressure = 0; S47.curve['signT'] = 1e9;
    },
    // a few cardboard aliens met before, as the log has them (the wandering one; event 6)
    met(n) { for (let i = 0; i < n; i++) S47.seenLog.add({ kind: 'scare', what: 'cutout', clock: Math.round(S47.game.clock) - 600 + i, place: 'SARO', area: 'saro', line: null, looking: true }); },
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

async def drive_to_post(ev, tick, shot, tag):
    """In the truck down the highway with the autopilot, in a build: until the shape stands at the
    end of the headlights. Pictures as the truck comes up on it and just after the lights stutter."""
    # (the curve at rest until the truck is out on the highway: the shape belongs to a build)
    await ev(f"{C}.force('rest'); S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.tick(0.1); S47.world.driveToDiner(() => {{}})")
    for _ in range(240):
        await tick(0.25)
        if await ev("S47.world.driving"): break
    await ev("S47.world.autopilot = 13")
    # out past the turn-off and down the highway, then the build
    for _ in range(400):
        await tick(0.25)
        z = await ev("(() => { const t = S47.world.truckHere(); return t ? t.group.position.z : 0; })()")
        if await ev("S47.world.area") == 'road' or z > 330: break
    await ev(f"{C}.force('build', 0.4); {C}['signT'] = 1e9")
    placed, at = False, None
    for _ in range(300):
        await tick(0.1)
        if await ev("S47.cutouts.up") == 'post': placed = True; break
    d0 = await ev("(() => { const c = S47.cutouts.position, t = S47.world.truckHere().group.position; return Math.hypot(c.x - t.x, c.z - t.z); })()") if placed else None
    pics, near = {}, None
    trouble0 = await ev("S47.world.trouble.count")
    for _ in range(600):
        if not await ev("S47.world.driving"): break
        st = await ev("(() => { const c = S47.cutouts.position, t = S47.world.truckHere().group.position; return { up: S47.cutouts.up, d: Math.hypot(c.x - t.x, c.z - t.z), vis: S47.scene.getObjectByName('cutout:wander').visible }; })()")
        if st['up'] == 'post' and st['d'] < 75 and 'far' not in pics:
            pics['far'] = st['d']; await shot(f'{tag}_post_far')
        if st['up'] == 'post' and st['d'] < 32 and 'near' not in pics:
            pics['near'] = st['d']; await shot(f'{tag}_post_near')
        if st['up'] != 'post':
            near = st['d']; break
        await tick(1 / 30)
    # the lights come back after the stutter: what is there now
    await tick(1 / 30, 8)
    await shot(f'{tag}_post_after')
    post = await ev("S47.cutouts.post")
    # where the post stands in the picture now (the card stood in front of it): still in view
    side = await ev("(() => { const c = S47.cutouts.position.clone(); c.y += 0.9; const n = c.project(S47.camera); return n.z < 1 ? Math.max(Math.abs(n.x), Math.abs(n.y)) : 9; })()")
    if post is not None: post['side'] = round(side, 2)
    trouble1 = await ev("S47.world.trouble.count")
    return placed, d0, pics, post, trouble1 > trouble0

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
        of = lambda kind, what=None: ev(f"T.of({json.dumps(kind)}{', ' + json.dumps(what) if what else ''})")
        async def look_off(yaw=-0.75): await ev(f"S47.player.yaw = {yaw}; S47.player.pitch = 0; S47.tick(0.05)")
        async def at_fig():
            await ev("(() => { const f = S47.scene.getObjectByName('glimpse').position, p = S47.player; p.yaw = Math.atan2(-(f.x - p.pos.x), -(f.z - p.pos.z)); p.pitch = Math.atan2(f.y + .6 - (p.floorY + p.eye), Math.hypot(f.x - p.pos.x, f.z - p.pos.z)); S47.tick(0.05); })()")

        # ---------- the prologue: no curve ----------
        await ev("S47.hold = true"); await tick(1, 3)
        m = await ev("T.may()")
        check(await ev(f"{C}.phase") == 'off' and not any(m.values()), f'the prologue: the curve is off and lets nothing come ({await ev("S47.game.phase")})')

        # ---------- chapter one: ten minutes of plain night ----------
        await ev("S47.jump('chapter1'); S47.tick(1.0); S47.game.d.ui.close(); S47.tick(0.2)")
        await ev("S47.game.clock = 2 * 3600 + 20 * 60; S47.flashlight.have = true; S47.flashlight.on = true; S47.world.enter('saro')")
        # (a night in which SARO never went dark, Hondo's telex on the roll anyway: story/saroDark.ts keeps
        # chapter one's 47 for the dark until it has come or 02:40 is past; tools/f_relay_hondo.py has that)
        await ev("S47.game.ns.hondo = 2 * 3600 + 29 * 60")
        await ev("S47.player.place(-2, -24, 0); S47.tick(0.3)")
        await look_off()
        cv = await ev(f"({{ phase: {C}.phase, len: {C}.len, level: {C}.level }})")
        check(cv['phase'] == 'plain' and cv['len'] == 600 and cv['level'] == 0, f'chapter one begins: plain night, ten minutes of it ({cv})')
        check(not any((await ev("T.may()")).values()), 'nothing may come in it')
        n0 = await ev("T.counts()")
        for _ in range(20):
            await ev("T.askAll()"); await tick(0.5)
        n1 = await ev("T.counts()")
        thin = await ev("S47.audio.night ? S47.audio.night.thin : -1")
        check(n1['calls'] == n0['calls'] and n1['glimpse'] == n0['glimpse'] and n1['cutPlaced'] == n0['cutPlaced'] and n1['cutUp'] is None and n1['ne'] is None and n1['stage'] == 'calm' and n1['signs'] == 0,
              f'ten seconds of everything asked to come, outdoors with the flashlight on: nothing (no call, glimpse, cutout, event, warning or sign: {n1})')
        check(thin == 0 and await ev(f"{A}.pressure") <= await ev(f"{A}['top']") + 1e-6, f'the crickets are all there (thin {thin}); the pressure on a line waits at its top')
        await shot('d01_plain')
        await ev("T.calm()")

        # ---------- the build ----------
        await ev(f"{C}.advance(600)")
        cv = await ev(f"({{ phase: {C}.phase, len: {C}.len, level: {C}.level, builds: {C}.builds }})")
        check(cv['phase'] == 'build' and 320 < cv['len'] < 460 and cv['level'] < 0.05, f'after the ten minutes: the build begins, some six minutes of it at this hour ({cv})')
        m = await ev("T.may()")
        check(not m['call'] and not m['cutout'] and not m['glimpse'] and not m['abduct'], f'at its foot: not yet a call or the cutout, never a glimpse or the warning ({m})')
        await ev(f"{C}.force('build', 0.35); {C}['signT'] = 1e9"); await tick(0.1)
        m = await ev("T.may()")
        check(m['sign'] and m['call'] and m['cutout'] and not m['event'] and not m['glimpse'] and not m['abduct'], f'a third of the way up: signs, a call, the cutout; no events yet, no glimpse, no warning ({m})')
        await tick(0.5, 4)
        thin = await ev("S47.audio.night.thin")
        check(0.08 < thin < 0.5, f'the crickets thin out ({thin:.2f})')
        # a sign: the flashlight stutters
        await ev(f"{C}['signT'] = 0"); await tick(0.1)
        sg = await ev("S47.curveSigns.slice(-1)[0]")
        fl = await ev("S47.flashlight['flicks'].get('curve')")
        check(sg and sg['what'] == 'flashlight' and fl > 0.3, f'a sign: the flashlight stutters ({sg}, {fl})')
        await ev(f"{C}['signT'] = 1e9")
        # a call out in the dark, and no second in this build
        c0 = await ev("S47.calls.count")
        await ev("S47.calls.next = 0"); await tick(0.5, 6)
        c1 = await ev("S47.calls.count")
        await ev("S47.calls.next = 0"); await tick(0.5, 6)
        c2 = await ev("S47.calls.count")
        check(c1 == c0 + 1 and c2 == c1 and len(await of('call')) == 1, f'a call out in the dark, and not a second in this build ({c0}, {c1}, {c2})')
        # the cutout: put out of the view, found, gone; no second in this build
        async def put():
            for _ in range(30):
                await ev("S47.cutouts['wait'] = 0"); await tick(0.2)
                if await ev("S47.cutouts.up"): return True
            return False
        await put()
        up = await ev("S47.cutouts.up")
        vis = await ev("(() => { const v = S47.cutouts['view'](S47.camera, S47.cutouts.position); return v.inView; })()")
        check(up == 'foot' and not vis and await ev(f"{C}.took.cutout") == 1, f'the cutout turns up, out of the view ({up}, in view {vis})')
        await ev("(() => { const c = S47.cutouts.position, p = S47.player.pos, d = Math.hypot(c.x - p.x, c.z - p.z); if (d > 20) { const k = (d - 12) / d; S47.player.place(p.x + (c.x - p.x) * k, p.z + (c.z - p.z) * k, 0); } })()")
        await ev("(() => { const c = S47.cutouts.position, p = S47.player.pos; S47.player.yaw = Math.atan2(-(c.x - p.x), -(c.z - p.z)); S47.player.pitch = -0.1; })()")
        await tick(0.3, 3)
        await shot('d02_build_cutout')
        check(await ev("S47.cutouts.seen") == 1, 'turned to it: found')
        await ev("S47.player.yaw += Math.PI; S47.tick(0.2)"); await tick(0.5, 8)
        await ev("S47.cutouts['wait'] = 0"); await tick(0.5, 4)
        check(await ev("S47.cutouts.up") is None, 'looked away: gone, and no second one in this build')
        # one at a time: while one of the night's things is going, the curve gives no sign of its own
        # (a blink of the dash would make the 1947 bulletin not heard through)
        await ev(f"{C}.force('build', 0.5); {C}['signT'] = 1e9"); await tick(0.1)
        k0 = await ev("S47.curveSigns.length")
        st = await ev("S47.nightEvents.now('steps', S47.nightIn())")
        await ev(f"{C}['signT'] = 0"); await tick(0.1, 3)
        busy, k1 = await ev("S47.oneAtATime()"), await ev("S47.curveSigns.length")
        check(st and busy and k1 == k0, f'while the steps behind are going, no sign of the curve on top of them ({st}, {busy}, {k1 - k0})')
        for _ in range(30):
            if not await ev("S47.oneAtATime()"): break
            await tick(0.5)
        await ev(f"{C}['signT'] = 1e9"); await tick(0.1)
        # one put out and not found is gone again once the build is over (never at the peak's moment)
        await ev("S47.player.place(-2, -24, 0); S47.tick(0.2)"); await look_off()
        await ev(f"{C}.force('build', 0.5); {C}['signT'] = 1e9"); await put()
        u1 = await ev("S47.cutouts.up")
        m = await ev("T.may()")
        check(m['event'] and m['scare'] and not m['glimpse'], f'half way up: the night\'s events may come too ({m})')
        await ev(f"{C}.force('peak'); {C}['signT'] = 1e9"); await tick(0.2, 3)
        u2 = await ev("S47.cutouts.up")
        check(u1 == 'foot' and u2 is None, f'one not yet found when the peak comes: not there any more ({u1}, then {u2})')
        m = await ev("T.may()")
        thin = await ev("S47.audio.night.thin")
        check(m['glimpse'] and m['abduct'] and m['long'] and not m['cutout'] and thin >= 0.65, f'the peak: a glimpse, the warning, a 47 may come; not the cutout; the night at its thinnest ({thin:.2f}, {m})')

        # ---------- the peak: a glimpse missed does not count; one seen does ----------
        await look_off()
        await ev("S47.glimpses['wait'] = 0"); await tick(0.1, 2)
        g = await ev("S47.glimpses.up")
        await shot('d03_peak_glimpse_edge')
        await ev("S47.player.yaw += Math.PI; S47.tick(0.05)")
        for _ in range(46): await tick(1.0)
        miss = await of('glimpse')
        check(g in ('S-11', 'S-14') and await ev("S47.glimpses.up") is None and len(miss) == 1 and miss[0]['looking'] is False and await ev(f"{C}.phase") == 'peak',
              f'a glimpse behind one of the far dishes that is never looked at: gone, written down as missed, and the peak goes on ({g}, {[x["looking"] for x in miss]}, {await ev(f"{C}.phase")})')
        await look_off()
        await ev("S47.glimpses['wait'] = 0"); await tick(0.1, 2)
        g2 = await ev("S47.glimpses.up")
        await at_fig(); await tick(0.1, 2)
        seen = await of('glimpse')
        cv = await ev(f"({{ phase: {C}.phase, len: {C}.len, cycle: {C}.cycle, landed: {C}.landed }})")
        check(g2 and len(seen) == 2 and seen[1]['looking'] is True and cv['phase'] == 'rest' and cv['cycle'] == 1 and cv['landed']['kind'] == 'glimpse',
              f'the next one, turned to: looked at, and that is the peak: the rest begins ({cv})')
        check(480 <= cv['len'] <= 720, f"8 to 12 minutes of it ({cv['len'] / 60:.1f})")

        # ---------- the rest: really quiet ----------
        await tick(0.5, 30)
        thin = await ev("S47.audio.night.thin")
        check(not any((await ev("T.may()")).values()) and thin == 0, f'in the rest nothing may come, and the crickets are back ({thin})')
        await ev("S47.player.place(-1, -62, 0); S47.tick(0.2)")
        on_b = await ev("S47.abductIn().line")
        n0 = await ev("T.counts()")
        for _ in range(24):
            await ev("T.askAll()"); await tick(0.5)
        await look_off(); await tick(0.5, 4)
        n1 = await ev("T.counts()")
        check(on_b == 'B' and n1['calls'] == n0['calls'] and n1['glimpse'] == n0['glimpse'] and n1['cutPlaced'] == n0['cutPlaced'] and n1['ne'] == n0['ne'] and n1['stage'] == 'calm' and n1['signs'] == n0['signs'],
              f'out in the field on B, everything asked again for a quarter of a minute: nothing ({n1})')
        await shot('d04_rest')
        await ev("T.calm()")
        await ev("S47.calls.now()"); await tick(0.5, 6)
        check(await ev("S47.calls.count") == n1['calls'] + 1, "the developer's sound in the dark still comes when asked for")
        b0 = await ev(f"{C}.builds")
        await ev(f"{C}.advance({C}.len + 1)")
        cv = await ev(f"({{ phase: {C}.phase, builds: {C}.builds, len: {C}.len }})")
        check(cv['phase'] == 'build' and cv['builds'] == b0 + 1, f'after the rest: the next build ({cv})')

        # ---------- the time away: only at the peak; Missing time off still turns it off ----------
        await ev(f"{C}.force('build', 0.9); {C}['signT'] = 1e9; {A}.data.last = -1e9; {A}.pressure = 5"); await tick(0.5, 4)
        st1, pr = await ev(f"{A}.stage"), await ev(f"{A}.pressure <= {A}['top'] + 1e-6")
        await ev("S47.setAbductions(false)"); await ev(f"{C}.force('peak'); {A}.pressure = 5"); await tick(0.5, 4)
        st2 = await ev(f"{A}.stage")
        check(st1 == 'calm' and pr and st2 == 'calm' and await ev(f"{C}.may('glimpse')"), f'alone on B late in the build the pressure waits at its top ({st1}); with Missing time off, the peak brings no warning, and a glimpse may still be it ({st2})')
        await ev("S47.setAbductions(true)"); await ev(f"{A}.pressure = 5"); await tick(0.1, 2)
        st3 = await ev(f"{A}.stage")
        w = await of('abduction')
        cv = await ev(f"({{ phase: {C}.phase, landed: {C}.landed && {C}.landed.what }})")
        check(st3 == 'warning' and len(w) == 1 and cv['phase'] == 'rest' and cv['landed'] == 'warning', f'turned on again at the peak: the warning, and that is the peak ({st3}, {cv})')
        await ev("S47.input.keys.clear(); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
        check(await ev(f"{A}.stage") == 'calm', 'indoors before the light: it passes')
        for _ in range(80):
            if await ev("S47.follow.mode") == 'idle': break
            await tick(0.5)

        # ---------- 47 seconds: one extra in a chapter; the rest waits for it ----------
        await ev("S47.player.place(-2, -24, 0); S47.tick(0.3)")
        await ev(f"{C}.force('peak')"); await tick(0.1)
        ok = await ev("S47.nightEvents.now('silence', S47.nightIn())"); await tick(0.5, 2)
        cv = await ev(f"({{ phase: {C}.phase, t: {C}.t, long: [...{C}.long] }})")
        busy = await ev("S47.oneAtATime()")
        m = await ev("T.may()")
        check(ok and await ev("S47.roadside.silent") and cv['phase'] == 'rest' and cv['long'] == ['ch1'] and busy and not any(m.values()),
              f'the silence: the peak, the one extra 47 of chapter one, and while it lasts nothing else may start ({cv})')
        await tick(1.0, 10)
        t1 = await ev(f"{C}.t")
        check(t1 < 0.01, f'the rest counts from when it is over ({t1:.2f} s into it, ten seconds into the silence)')
        for _ in range(40):
            if not await ev("S47.roadside.silent"): break
            await tick(1.0)
        await tick(1.0, 3)
        check(await ev(f"{C}.t") > 1, 'the silence over: now the rest counts')
        await ev(f"{C}.force('peak')"); await tick(0.1)
        m1 = await ev(f"{C}.may('long')")
        check(not m1 and await ev(f"{C}.may('glimpse')"), 'the next peak in chapter one: no second extra 47, a glimpse yes')
        # held: a panel open
        await ev("S47.game.openNotebook(); S47.tick(0.1)")
        t0 = await ev(f"{C}.t"); await tick(0.5, 6); t1 = await ev(f"{C}.t")
        held = not any((await ev("T.may()")).values())
        await ev("S47.game.d.ui.close(); S47.tick(0.1)")
        check(held and abs(t1 - t0) < 1e-6, f'with the notebook open the curve is held: nothing may come, and its time stands ({t0:.1f}, {t1:.1f})')
        await ev("S47.jump('chapter2'); S47.tick(1.0); S47.game.d.ui.close(); S47.tick(0.2)")
        await ev(f"{C}.force('peak')"); await tick(0.1)
        await ev("S47.player.place(-2, -24, 0); S47.tick(0.3)")
        check(await ev(f"{C}.may('long')") and await ev(f"{C}.phase") == 'peak', 'chapter two: its own extra 47 may come at a peak')

        # ---------- on S47 and the ?debug overlay ----------
        await ev(f"{C}.force('build', 0.42); {C}['signT'] = 1e9"); await tick(0.1)
        await ev("S47.hold = false"); await pg.wait_for_timeout(1500); await ev("S47.hold = true")
        txt = await ev("(() => { const e = document.querySelector('.debughud'); return e ? e.textContent : ''; })()")
        check('curve build 0.4' in txt, f"the ?debug overlay has the curve's phase and level ({[l for l in txt.splitlines() if 'curve' in l][:1]})")
        await shot('d05_debug_overlay')

        # ---------- the twist: the same shape at the end of the headlights, a reflector post ----------
        await ev("T.met(2); S47.game.clock = 3 * 3600 + 10 * 60")
        n0 = len(await ev("S47.game.notes"))
        placed, d0, pics, post, blinked = await drive_to_post(ev, tick, shot, 'd06')
        nn = (await ev("S47.game.notes"))[n0:]
        pe = await of('scare', 'cutout, post')
        check(placed and d0 and 140 < d0 < 220, f'driving in a build, the cutout met twice: the same shape stands on the shoulder, far down the road ({d0 and round(d0)} m)')
        check(post and 10 < post['dist'] < 21 and post['looked'] and blinked, f'when the headlights reach it they stutter, and it is not there: the reflector post ({post}, blink {blinked}; pictures at {pics})')
        check(any('The same shape. There was nothing there when I got to it.' in n for n in nn) and await ev("S47.townSeen.has('cut:post')") and len(pe) == 1 and pe[0]['looking'] is True,
              f'the notebook: {[n for n in nn if "same shape" in n][:1]}')
        # once: the next build, still driving, nothing of the kind
        await ev(f"{C}.force('build', 0.4); {C}['signT'] = 1e9"); await tick(0.5, 30)
        check(await ev("S47.cutouts.up") != 'post' and len(await of('scare', 'cutout, post')) == 1, 'once in the night: not in the next build')
        # a sign in the truck: the radio slides under its static, or the lights blink
        await ev("S47.carRadio.set(true, 'caliche')"); await tick(0.5, 4)
        k0 = await ev("S47.world.trouble.count")
        await ev(f"{C}['signT'] = 0"); await tick(1 / 30, 20)
        sg = await ev("S47.curveSigns.slice(-1)[0]")
        intf = await ev("S47.carRadio.interference")
        check(sg and ((sg['what'] == 'radio' and intf > 0.3) or (sg['what'] == 'dash' and await ev("S47.world.trouble.count") > k0)), f'a sign in the truck: {sg} (the station under the static {intf:.2f})')
        await ev(f"{C}['signT'] = 1e9; S47.carRadio.set(false)")
        for _ in range(400):
            if not await ev("S47.world.driving"): break
            await tick(0.5)

        # ---------- kept with the case ----------
        await ev(f"{C}.force('rest')"); await tick(0.5, 4)
        before = await ev(f"({{ phase: {C}.phase, t: Math.round({C}.t), len: Math.round({C}.len), cycle: {C}.cycle, long: [...{C}.long] }})")
        await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(800)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev(HELPERS); await ev("S47.hold = true; S47.tick(0.3)")
        after = await ev(f"({{ phase: {C}.phase, t: Math.round({C}.t), len: Math.round({C}.len), cycle: {C}.cycle, long: [...{C}.long] }})")
        check(after['phase'] == before['phase'] and abs(after['t'] - before['t']) <= 2 and after['len'] == before['len'] and after['cycle'] == before['cycle'] and after['long'] == before['long'],
              f'after Continue the curve is where it was ({before} and {after})')
        check(await ev("S47.townSeen.has('cut:post')"), 'and the shape at the end of the headlights stays had')

        # a save from the prologue (02:13, no case yet, the curve off) still gets the plain ten
        # minutes when chapter one begins; an older case without a curve, a short quiet
        u = await ev(f"(() => {{ const c = new {C}.constructor(S47.seenLog); c.restore({{ phase: 'off', t: 0, len: 0, cycle: 0, long: [] }}, true); c.update(0.1, {{ chapter: 'ch1', held: false, exposed: true, busy: false, peaking: false, clock: 8200 }}); const a = [c.phase, Math.round(c.len)]; c.restore(undefined, true); c.update(0.1, {{ chapter: 'ch2', held: false, exposed: true, busy: false, peaking: false, clock: 11000 }}); return [a, [c.phase, Math.round(c.len)]]; }})()")
        check(u == [['plain', 600], ['rest', 150]], f'a save from the prologue: the plain ten minutes; an older case: a short quiet ({u})')

        # ---------- chapter six and Nora's room: no curve ----------
        u = await ev(f"(() => {{ const c = new {C}.constructor(S47.seenLog); c.update(0.1, {{ chapter: 'ch1', held: false, exposed: true, busy: false, peaking: false, clock: 9000 }}); const a = c.phase; c.force('peak'); c.update(0.1, {{ chapter: 'ch6', held: false, exposed: true, busy: false, peaking: false, clock: 19000 }}); return [a, c.phase, c.may('glimpse')]; }})()")
        check(u == ['plain', 'off', False], f'chapter six (THE EVENT has the night): the curve is off ({u})')
        r6 = await ev(f"(() => {{ const c = new {C}.constructor(S47.seenLog); c.update(0.1, {{ chapter: 'ch4', held: false, exposed: true, busy: false, peaking: false, clock: 16800 }}); c.force('peak'); c.update(0.1, {{ chapter: 'ch4', held: true, exposed: true, busy: false, peaking: false, clock: 16800 }}); return c.may('glimpse'); }})()")
        check(r6 is False, "held (Nora's room, the diner and its map, a call, a cinematic): nothing may come")

        # ======================= phone =======================
        ctx2, pp = await page(b, 412, 892, True, errs)
        e2 = pp.evaluate
        async def tick2(s=0.1, n=1):
            for _ in range(n): await e2(f"S47.tick({s})")
        async def shot2(name): await pp.screenshot(path=f'{OUT}/{name}.png')
        await e2("S47.hold = true; S47.jump('chapter2'); S47.tick(1.0); S47.game.d.ui.close(); S47.tick(0.2)")
        await e2("S47.game.clock = 3 * 3600 + 5 * 60; S47.setAbductions(false); T.met(2); S47.tick(0.1)")
        check(await e2(f"{C}.phase") == 'plain', 'phone: a night begun at chapter two starts with its plain stretch')
        placed, d0, pics, post, blinked = await drive_to_post(e2, tick2, shot2, 'p01')
        nn = await e2("S47.game.notes.filter((n) => n.includes('The same shape'))")
        check(placed and post and post['looked'] and len(nn) == 1, f'phone: the shape at the end of the headlights, then the post ({d0 and round(d0)} m, {post}, {pics})')
        check(post and 20 < post['dist'] < 45 and post['side'] < 0.95, f'phone held upright: the lights reach it while it is well in view, so the post is still in the picture after the blink ({post})')
        await e2(f"{C}.force('build', 0.42); {C}['signT'] = 1e9"); await tick2(0.1)
        await e2("S47.hold = false"); await pp.wait_for_timeout(1500); await e2("S47.hold = true")
        box = await e2("(() => { const e = document.querySelector('.debughud'); if (!e) return null; const r = e.getBoundingClientRect(); return { text: e.textContent, right: r.right, w: innerWidth }; })()")
        cl = [l for l in (box['text'] if box else '').splitlines() if l.startswith('curve')]
        check(cl and 'build' in cl[0] and len(cl[0]) <= 52, f'phone: the overlay has the curve, on one line ({cl[:1]})')
        await shot2('p02_debug_overlay')

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:6])))
        json.dump({'checks': checks}, open(f'{OUT}/f_curve.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
