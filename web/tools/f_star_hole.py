# No stars where the antennas point (world/Sky.ts STAR_HOLE, story/starHole.ts, story/Chapter6.ts
# LAST_LOOK; FORSLAG.md, 20 October, "Valg for Tom" 6; Tom: "Spenning og skummelt slår faste regler").
# From 02:15, when every dish stands at az 026, el 32 with nobody driving it, a round patch of the sky
# there has no stars in it: nothing is drawn over the sky, only what is beyond the air is missing.
#   - the sky (rendered alone, the sky's own group, through a narrow camera): before 02:15 there are
#     stars at az 026, el 32; it comes in over a minute or two, not at once; after it there are none
#     within 3 degrees of it and there are 10 degrees away; the sky behind it is the colour of the
#     sky round it, and the same as without it (no dark disc, no glow); an hour of the night later
#     it has not moved and not grown, and the stars round it are where they were; at dawn it is still
#     there; it lines up with the dishes (S-03 points at it); a shooting star sent through it is gone
#     inside it and comes out the other side; the comet is never near it
#   - noticed once: not before 02:15, not indoors, not while another of the night's things is going
#     (the silence), not 10 degrees off it, not after 2.5 s, not at dawn; looked at for 3 s: a toast
#     and a note in Reyes's words, once, the night's log ('scare', 'no stars', looked at), a card on
#     the board; looked at again, nothing more; a closer look (the zoom) says what is in it
#   - a free frame of it: the line under the print, its target 'starhole' where it is on the print,
#     the print on the board's card; the card's thread to S-03's heading holds and is kept
#   - Continue: it is there (by the clock), noticed, the thread kept, nothing written again
#   - the last picture of the pull-out (05:29:47 + 34 s) has it, low in the frame, and on a phone
#   - desktop 1280x720 and a phone 412x892 with touch: the phone notices it through the viewfinder
#     (Camera button, a tap for the shutter); no console errors or warnings
# Screenshots: looking up along the dishes at SARO, the closer look, from the highway, from Caliche,
# the board, the last picture of the pull-out; the same on a phone.
# Usage: python3 tools/f_star_hole.py OUTDIR
import asyncio, sys, json, os, math, base64
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
TOAST = 'There are no stars where the dishes point.'
PRINT = 'No stars where the dishes point.'
NOTE_BIT = 'There are no stars where the dishes point. Az 026, el 32'
H0 = 2 * 3600 + 15 * 60 + 22          # the night's impact, 02:15:12, and STAR_HOLE.after
MILE = 1609.344
s_at = lambda mi: (mi - 8.15) * MILE
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

HELPERS = """(() => {
  const THREE = S47.three, V = THREE.Vector3, D = Math.PI / 180;
  const dirAE = (az, el) => new V(Math.sin(az * D) * Math.cos(el * D), Math.sin(el * D), -Math.cos(az * D) * Math.cos(el * D));
  window.T = {
    toasts: [],
    hook() {
      const ui = S47.game.d.ui; if (ui.__hooked) return; ui.__hooked = true;
      const o = ui.toast.bind(ui); ui.toast = (t, s, c) => { T.toasts.push(t); return o(t, s, c); };
    },
    // a direction of the sky (azimuth, elevation in its own terms) in world terms here
    world(az, el) { return dirAE(az, el).applyAxisAngle(new V(0, 1, 0), S47.world.skyYaw); },
    // the eyes on (az, el) of the sky, from where the player stands
    look(az, el) { const d = T.world(az, el), p = S47.player; p.yaw = Math.atan2(-d.x, -d.z); p.pitch = Math.asin(d.y); S47.tick(0.02); },
    off() { const f = S47.camera.getWorldDirection(new V()); return f.angleTo(T.world(26, 32)) / D; },
    // The sky alone (its own group: the dome, the stars, the comet, the shooting star, the painted
    // horizon) through a camera fov degrees wide, N pixels, aimed at (az, el); as linear values.
    render(az, el, fov, N, only) {
      const sky = S47.sky, R = S47.renderer, g = sky.group;
      const cam = new THREE.PerspectiveCamera(fov, 1, 1, 10000);
      g.updateMatrixWorld(true);
      cam.position.copy(g.position); cam.lookAt(cam.position.clone().add(T.world(az, el))); cam.updateMatrixWorld();
      const keep = g.children.map((c) => c.visible);
      if (only) g.children.forEach((c) => { c.visible = only(c); });
      const rt = new THREE.WebGLRenderTarget(N, N, { type: THREE.FloatType });
      const cc = R.getClearColor(new THREE.Color()), ca = R.getClearAlpha();
      R.setRenderTarget(rt); R.setClearColor(0x000000, 1); R.clear(); R.render(g, cam); R.setRenderTarget(null); R.setClearColor(cc, ca);
      g.children.forEach((c, i) => { c.visible = keep[i]; });
      const px = new Float32Array(N * N * 4); R.readRenderTargetPixels(rt, 0, 0, N, N, px); rt.dispose();
      // pixels per degree at the middle, and the angle of each pixel from the middle
      const f = (N / 2) / Math.tan(fov * D / 2);
      const at = (x, y) => Math.atan(Math.hypot(x + 0.5 - N / 2, y + 0.5 - N / 2) / f) / D;
      return { px, N, at, lum: (i) => 0.2126 * px[i * 4] + 0.7152 * px[i * 4 + 1] + 0.0722 * px[i * 4 + 2] };
    },
    // the stars within r degrees of (az, el): pixels brighter than the sky round them
    stars(az, el, r, thr = 0.07) {
      const s = T.render(az, el, 12, 256), N = s.N;
      let bg = 0, nb = 0; const pts = [];
      for (let y = 0; y < N; y++) for (let x = 0; x < N; x++) { const a = s.at(x, y); if (a > r + 0.6 && a < r + 2) { bg += s.lum(y * N + x); nb++; } }
      bg /= Math.max(1, nb);
      let n = 0, max = 0;
      for (let y = 0; y < N; y++) for (let x = 0; x < N; x++) {
        if (s.at(x, y) > r) continue;
        const l = s.lum(y * N + x) - bg; if (l > max) max = l;
        if (l > thr) { n++; pts.push(x + ',' + y); }
      }
      return { n, max, bg, pts };
    },
    // the sky behind it, without the stars: its mean inside r, in a ring round it, in linear values
    air(az, el, r) {
      const pts = S47.sky.group.children.find((c) => c.isPoints);
      const s = T.render(az, el, 16, 256, (c) => c !== pts && c !== S47.sky.meteor), N = s.N;
      let a = 0, na = 0, b = 0, nb = 0;
      for (let y = 0; y < N; y++) for (let x = 0; x < N; x++) {
        const d = s.at(x, y), l = s.lum(y * N + x);
        if (d < r) { a += l; na++; } else if (d > 5.4 && d < 7.4) { b += l; nb++; }
      }
      return { inside: a / na, ring: b / nb };
    },
    // a shooting star sent along a great circle through it, 12 degrees long, 3 s; at each step the
    // light of the streak alone inside the hole and before and after it along the way
    meteor() {
      const sky = S47.sky, M = sky['met'], h = sky.holeDir.clone();
      const t = new V(0, 1, 0).cross(h).normalize(), a0 = 8 * D;
      M.t = -1; sky.meteorNow();
      M.start.copy(h).multiplyScalar(Math.cos(a0)).addScaledVector(t, -Math.sin(a0)).normalize();
      M.tan.copy(t).multiplyScalar(Math.cos(a0)).addScaledVector(h, Math.sin(a0)).normalize();
      M.len = 16 * D; M.dur = 3; M.bright = 2.6; M.t = 0;
      const out = [], g = sky.group;
      // (where "before" and "after" it are on the picture: the side of the hole along t)
      const tw = t.clone().applyAxisAngle(new V(0, 1, 0), S47.world.skyYaw);
      for (let k = 0; k < 14; k++) {
        sky['meteorStep'](0.25);
        if (!sky.meteor.visible) { out.push(null); continue; }
        const s = T.render(26, 32, 30, 512, (c) => c === sky.meteor), N = s.N;
        const cam = new THREE.PerspectiveCamera(30, 1, 1, 10000); cam.position.copy(g.position); cam.lookAt(cam.position.clone().add(T.world(26, 32))); cam.updateMatrixWorld();
        const side = cam.position.clone().add(T.world(26, 32)).addScaledVector(tw, 0.2).project(cam);
        const sx = side.x, sy = side.y;
        let inside = 0, before = 0, after = 0;
        for (let y = 0; y < N; y++) for (let x = 0; x < N; x++) {
          const l = s.lum(y * N + x); if (l <= 0.002) continue;
          const d = s.at(x, y);
          if (d < 3.1) inside += l;
          else if (d > 5.2) { const u = (x + 0.5) / N * 2 - 1, v = (y + 0.5) / N * 2 - 1; if (u * sx + v * sy > 0) after += l; else before += l; }
        }
        out.push({ head: (M.len * Math.min(1, M.t / M.dur)) / D, inside, before, after });
      }
      M.t = -1; sky.meteor.visible = false;
      return out;
    },
    has(k) { return S47.townSeen.has(k); },
    keys(re) { return [...S47.townSeen].filter((k) => new RegExp(re).test(k)); },
    count(s) { return S47.game.notes.filter((n) => n.includes(s)).length; },
    log() { return S47.seen.filter((e) => e.what === 'no stars').map((e) => [e.kind, e.looking]); },
    // where it is on the screen now (-1 to 1 from the middle), and in front
    ndc() { const c = S47.camera, p = c.position.clone().addScaledVector(T.world(26, 32), 2000).project(c); return [p.x, p.y, p.z < 1]; },
  };
  return true;
})()"""

async def page(b, w, h, touch):
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
    await pg.add_init_script("try{localStorage.setItem('s47.serlingNarrator','false');localStorage.setItem('s47.characterVoices','false');}catch{}")
    await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    if touch: await pg.tap('button[data-a=start]'); await begin_opening(pg, touch=True)
    else: await pg.click('button[data-a=start]'); await begin_opening(pg)
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS)
    await pg.evaluate("T.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    return ctx, pg, errs

async def pullout_end(pg, ev, tag):
    """Chapter six from the diner's lot to the line and THE EVENT, then the pull-out to its last
    picture: where the camera looks, where az 026, el 32 is in it, and the stars there in the sky."""
    async def tick(s): await ev(f"S47.tick({s})")
    await ev("S47.game.d.ui.close(); S47.jump('chapter6'); S47.tick(0.5)")
    for _ in range(80):
        await tick(0.25); await pg.wait_for_timeout(100)
        if await ev("S47.world.area === 'roswell' && S47.world.driving"): break
    await ev(f"""(() => {{ const r = S47.world.oldRoad, d = S47.world.oldDrive, s = {s_at(8.12)};
          const a = r.lane(s), b2 = r.lane(s + 4); d.place(a, Math.atan2(-(b2.x - a.x), -(b2.z - a.z))); }})()""")
    await tick(0.1)
    await ev("""((v) => { const w = S47.world, road = w.oldRoad, drv = w.oldDrive;
      w.testInput = () => { const s = road.where(drv.pos.x, drv.pos.z).s; const tg = road.lane(s + 18);
        let err = Math.atan2(-(tg.x - drv.pos.x), -(tg.z - drv.pos.z)) - drv.heading; err = Math.atan2(Math.sin(err), Math.cos(err));
        return { steer: Math.max(-1, Math.min(1, -err * 2.2)), throttle: Math.max(-1, Math.min(1, (v - drv.speed) * 0.5)) }; }; })(16)""")
    for _ in range(3000):
        await ev("S47.tick(1 / 30)")
        if await ev("S47.ch6.stage") == 'event': break
    for _ in range(200):
        await tick(0.5)
        if await ev("S47.ch6.stage") == 'pullout': break
    # to a moment before the black: the last picture
    for _ in range(200):
        if await ev("S47.ch6['pt']") >= 33.6: break
        await tick(0.25)
    await tick(0.2)
    r = await ev("""(() => { const c = S47.camera, f = c.getWorldDirection(new S47.three.Vector3()), D = Math.PI / 180;
      return { pt: S47.ch6['pt'], stage: S47.ch6.stage, area: S47.world.area, el: Math.asin(f.y) / D, az: ((Math.atan2(f.x, -f.z) / D) + 360) % 360,
        ndc: T.ndc(), hole: S47.sky.hole, dawn: S47.sky.uniforms.uDawn.value, fov: c.fov }; })()""")
    # the stars in the sky of that picture, at az 026, el 32 and 10 degrees away
    r['in'] = (await ev("T.stars(26, 32, 1.4)"))['n']
    r['away'] = (await ev("T.stars(26, 42, 1.4)"))['n']
    # (the radio's lines from THE EVENT are long gone by now in a night played in real time)
    await ev("S47.game.d.ui.clearToasts()"); await pg.wait_for_timeout(300)
    await pg.screenshot(path=f'{OUT}/{tag}_pullout_last.png')
    return r

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        # ======================= desktop =======================
        ctx, pg, errs = await page(b, 1280, 720, False)
        ev = pg.evaluate
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def shot(name, clear=True):
            if clear: await ev("S47.game.d.ui.clearToasts && S47.game.d.ui.clearToasts()")
            await pg.wait_for_timeout(450); await pg.screenshot(path=f'{OUT}/{name}.png')
        async def quiet(): await ev("S47.nightEvents['wait'] = 1e9")
        noticed = lambda: ev("S47.starHole.noticed")
        async def at(x, z, clock=None):
            c = f"S47.game.clock = {clock}; " if clock is not None else ''
            await ev(f"{c}S47.world.enter('saro'); S47.player.place({x}, {z}, 0); S47.tick(0.2)")
        async def zoom(secs=0.9):
            await ev("S47.input.zoomHeld = true"); await tick(secs)
            t = await ev("(() => { const e = document.querySelector('.hud .examine'); return e && e.classList.contains('show') ? e.textContent : null; })()")
            return t
        async def unzoom(): await ev("S47.input.zoomHeld = false"); await tick(0.4)
        await ev("S47.hold = true; S47.curve.free = true; S47.input.locked = true")
        await ev("S47.jump('chapter1'); S47.tick(1.0); S47.setAbductions(false); S47.game.d.ui.close(true)"); await quiet()
        # (no lightning on the horizon in the middle of the measurements)
        await ev("S47.sky['nextFlash'] = 1e9; S47.sky['flashT'] = 0; S47.sky.uniforms.uFlash.value = 0")
        X, Z = 9.5, -6.0     # out in the service yard
        await at(X, Z)
        check(await ev("S47.space()") == 'yard' and await ev("S47.world.area") == 'saro', f'out in the service yard at SARO ({await ev("S47.placeName()")})')

        # ---------- the sky ----------
        await ev(f"S47.game.clock = {H0 - 300}; S47.tick(0.1)")
        s0 = await ev("T.stars(26, 32, 1.4)")
        check(await ev("S47.sky.hole") == 0 and s0['n'] >= 3, f'02:10: stars at az 026, el 32 ({s0["n"]} star pixels within 1.4 degrees), no hole')
        air0 = await ev("T.air(26, 32, 3.0)")
        fade = []
        for k in range(16):
            await ev(f"S47.game.clock = {H0 - 20 + k * 10}; S47.tick(0.02)")
            fade.append(round(await ev("S47.sky.hole"), 3))
        jumps = max(b2 - a for a, b2 in zip(fade, fade[1:]))
        check(fade[0] == 0 and fade[1] == 0 and fade[2] == 0 and fade[-1] == 1 and all(b2 >= a for a, b2 in zip(fade, fade[1:])) and jumps < 0.2
              and sum(1 for v in fade if 0 < v < 1) >= 8, f'from 02:15:22, as the dishes come round to it, it comes in over 100 s, never in a step (every 10 s: {fade})')
        await ev(f"S47.game.clock = {H0 + 50}; S47.tick(0.02)")
        half = await ev("T.stars(26, 32, 1.4)")
        check(0 < await ev("S47.sky.hole") < 1 and half['max'] < s0['max'] * 0.9, f'half way: the stars there dimmer, not gone at once (brightest {half["max"]:.2f} against {s0["max"]:.2f})')
        await ev("S47.game.clock = 3 * 3600 + 20 * 60; S47.tick(0.1)")
        s1 = await ev("T.stars(26, 32, 3.0)")
        a1 = await ev("T.stars(26, 42, 1.4)")
        a2 = await ev("T.stars(38, 32, 1.4)")
        check(await ev("S47.sky.hole") == 1 and s1['n'] == 0 and s1['max'] < 0.02, f'03:20: no star within 3 degrees of az 026, el 32 (brightest pixel {s1["max"]:.3f} over the sky)')
        check(a1['n'] >= 2 and a2['n'] >= 2, f'and 10 degrees away there are (el 42: {a1["n"]}, az 038: {a2["n"]} star pixels)')
        air1 = await ev("T.air(26, 32, 3.0)")
        # (what goes from behind it is the faint light the painted sky had there, no more)
        check(abs(air1['inside'] / air1['ring'] - 1) < 0.04 and 0.9 < air1['inside'] / air0['inside'] <= 1.01 and abs(air1['ring'] / air0['ring'] - 1) < 0.02,
              f'nothing drawn over the sky: behind it the sky is the colour of the sky round it ({air1["inside"]:.4f} / {air1["ring"]:.4f}), no lighter than before and hardly darker ({air0["inside"]:.4f} / {air0["ring"]:.4f})')
        ring1 = await ev("T.stars(26, 32, 6)")
        await ev("S47.game.clock = 4 * 3600 + 20 * 60; S47.tick(0.1)")
        s2 = await ev("T.stars(26, 32, 1.4)")
        ring2 = await ev("T.stars(26, 32, 6)")
        same = len(set(ring1['pts']) & set(ring2['pts'])) / max(1, len(set(ring1['pts']) | set(ring2['pts'])))
        check(s2['n'] == 0 and await ev("S47.sky.holeDir.toArray().map((v) => +v.toFixed(5))") == await ev("T.world(26, 32).applyAxisAngle(new S47.three.Vector3(0, 1, 0), -S47.world.skyYaw).toArray().map((v) => +v.toFixed(5))")
              and same > 0.85 and abs(ring1['n'] - ring2['n']) <= max(3, ring1['n'] * 0.15),
              f'an hour later, 04:20: still none there, not moved, not grown; the stars within 6 degrees where they were ({ring1["n"]} and {ring2["n"]}, {same:.0%} the same pixels)')
        await ev("S47.game.clock = 5 * 3600 + 15 * 60; S47.tick(0.1)")
        dawn = await ev("[S47.sky.hole, S47.sky.uniforms.uDawn.value]")
        sd = await ev("T.stars(26, 32, 1.4, 0.03)"); ad = await ev("T.stars(26, 42, 4, 0.03)")
        check(dawn[0] == 1 and dawn[1] > 0.3 and sd['n'] == 0, f'05:15, the dawn coming ({dawn[1]:.2f}): still there while the stars last ({ad["n"]} still 10 degrees off)')
        # the dishes: S-03 points at it
        aim = await ev("""(() => { const d = S47.ext.dishes.find((x) => x.id === 'S-03') || S47.ext.dishes[0], D = Math.PI / 180;
          const v = new S47.three.Vector3(Math.sin(d.curAz * D) * Math.cos(d.curEl * D), Math.sin(d.curEl * D), -Math.cos(d.curAz * D) * Math.cos(d.curEl * D));
          return [d.id, d.curAz, d.curEl, v.angleTo(T.world(26, 32)) / D]; })()""")
        check(aim[3] < 0.05, f'it is where the dishes point: {aim[0]} at az {aim[1]:.1f}, el {aim[2]:.1f}, {aim[3]:.3f} degrees off it')
        await ev("S47.game.clock = 3 * 3600; S47.tick(0.1)")
        met = await ev("T.meteor()")
        seen_m = [m for m in met if m]
        before_side = any(m['before'] > 0.5 for m in seen_m); after_side = any(m['after'] > 0.5 for m in seen_m)
        through = [m for m in seen_m if 6.5 < m['head'] < 9.5]
        check(len(seen_m) >= 8 and max(m['inside'] for m in seen_m) < 0.05 and before_side and after_side and through,
              f'a shooting star sent through it: gone inside it ({max(m["inside"] for m in seen_m):.3f} of light there at most, its head at {[round(m["head"], 1) for m in through]} degrees), there before and after it')
        cm = await ev("(() => { const c = S47.sky.comet.position.clone().normalize(); return c.angleTo(S47.sky.holeDir) * 180 / Math.PI; })()")
        check(cm > 120, f'the comet, at az 200: {cm:.0f} degrees from it, never in it')
        # (nothing more to draw for it: the same view, before and after, the same draw calls)
        await at(X, Z, H0 - 300); await ev("T.look(26, 32)"); await tick(0.2)
        n_draw0 = await ev("S47.renderer.info.render.calls")
        await at(X, Z, 3 * 3600); await ev("T.look(26, 32)"); await tick(0.2)
        n_draw1 = await ev("S47.renderer.info.render.calls")
        check(n_draw0 == n_draw1, f'and it costs no draw call: {n_draw0} before it, {n_draw1} with it, the same view')

        # ---------- noticed: not when it should not be ----------
        await ev("S47.game.d.ui.clearToasts && S47.game.d.ui.clearToasts(); T.toasts.length = 0")
        await at(X, Z, H0 - 600); await ev("T.look(26, 32)"); await tick(0.5, 10)
        check(not await noticed() and await ev("S47.sky.hole") == 0, '02:05, looking up at az 026, el 32 for 5 s: nothing (it is not there yet)')
        await at(X, Z, 3 * 3600); await ev("T.look(38, 34)"); await tick(0.5, 10)
        check(not await noticed() and 9 < await ev("T.off()") < 11, f'03:00, 10 degrees off it for 5 s: not noticed')
        await ev("T.look(26, 32)"); await tick(0.5, 5)
        check(not await noticed() and await ev("T.off()") < 1, f'on it for 2.5 s: not yet ({await ev("S47.starHole.look"):.1f} s)')
        await ev("T.look(40, 32)"); await tick(1.0, 2)
        await ev("S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.1); T.look(26, 32)"); await tick(0.5, 10)
        check(not await noticed() and await ev("S47.space()") == 'room', 'in the control room, the eyes the same way for 5 s: not noticed')
        await at(X, Z, 3 * 3600)
        await ev("S47.roadside.silenceNow({ hush: (s, l) => S47.audio.night?.hush(s, l), flicker: () => {} })"); await tick(0.2)
        check(await ev("S47.oneAtATime()"), "the silence (47 s), one of the night's things, is going")
        await ev("T.look(26, 32)"); await tick(0.5, 10)
        z0 = await zoom(); await unzoom()
        check(not await noticed(), f'while it lasts, looked at for 5 s and closer: not noticed (one at a time)')
        for _ in range(60):
            if not await ev("S47.roadside.silent"): break
            await tick(1.0)
        await ev("T.look(26, 32)")
        await ev(f"S47.game.clock = 5 * 3600 + 20 * 60; S47.tick(0.05)")
        await tick(0.5, 10)
        check(not await noticed() and await ev("S47.sky.uniforms.uDawn.value") >= 0.3, '05:20, the dawn up: not noticed any more')

        # ---------- noticed, once ----------
        await ev("S47.game.d.ui.clearToasts()")
        await ev(f"S47.game.clock = 3 * 3600 + 5 * 60; S47.tick(0.05)")
        n0 = await ev("S47.game.notes.length"); await ev("T.toasts.length = 0")
        await ev("T.look(26, 32)"); await tick(0.5, 7)
        nn = await ev(f"S47.game.notes.slice({n0})")
        ts = await ev("T.toasts.slice()")
        note = [n for n in nn if NOTE_BIT in n]
        check(await noticed() and len(note) == 1 and ts.count(TOAST) == 1, f'03:05, looked at for 3 s: noticed, once ("{TOAST}")')
        check(note and note[0].startswith('03:05. ') and 'as if something were standing in front of them' in note[0] and 'Not a cloud' in note[0], f'the note: "{note[:1]}"')
        check(await ev("T.log()") == [['scare', True]] and await ev("T.has('sky:hole') && T.keys('^sky:hole@').length === 1 && T.has('sky:hole/ch1')"),
              f'the night\'s log has it once, looked at ({await ev("T.log()")}); kept with the case')
        await shot('desk_01_noticed', clear=False)
        await ev("T.look(40, 30)"); await tick(0.5, 2); await ev("T.look(26, 32)"); await tick(0.5, 10)
        check(await ev(f"T.count({json.dumps(NOTE_BIT)})") == 1 and (await ev("T.toasts.slice()")).count(TOAST) == 1 and len(await ev("T.log()")) == 1,
              'looked at again for 5 s: nothing more')
        ex = await zoom()
        await shot('desk_02_closer_look', clear=False)
        await unzoom()
        check(ex is not None and 'No stars in it' in ex, f'a closer look at it: "{ex}"')
        ex2 = None
        await ev("T.look(38, 34)"); ex2 = await zoom(); await unzoom()
        check(ex2 is None or 'No stars' not in ex2, f'a closer look 10 degrees off it: nothing about it ({ex2})')

        # ---------- looking up along the dishes ----------
        # (behind S-03 in the antenna field, the dishes in the bottom of the picture)
        await ev("S47.game.clock = 3 * 3600 + 10 * 60; S47.player.place(-6, -40, 0); S47.tick(0.1); T.look(26, 24)"); await tick(0.3)
        await shot('desk_03_saro_along_the_dishes')
        # out on the highway's shoulder south of SARO
        await ev("S47.player.place(-24, 200, 0); S47.tick(0.1); T.look(26, 26)"); await tick(0.3)
        check(await ev("S47.space()") == 'yard', f'on the shoulder of the highway ({await ev("S47.placeName()")})')
        await shot('desk_04_highway')

        # ---------- a free frame of it ----------
        await ev("S47.player.place(9.5, -6.0, 0); S47.tick(0.1); T.look(26, 32)"); await tick(0.2)
        await ev("S47.fcam.have = true")
        if not await ev("S47.fcam.raised"): await pg.keyboard.press('KeyC'); await tick(0.2)
        k = await ev("S47.album.shots.length")
        await pg.keyboard.press('Space'); await tick(0.3)
        sh = await ev("S47.album.shots.at(-1)") if await ev("S47.album.shots.length") > k else None
        if sh: open(f'{OUT}/desk_05_print.jpg', 'wb').write(base64.b64decode(sh['url'].split(',', 1)[1]))
        if await ev("S47.fcam.raised"): await pg.keyboard.press('KeyC'); await tick(0.2)
        tg = (sh or {}).get('targets', {}).get('starhole')
        check(sh is not None and PRINT in sh['text'] and sh.get('odd') and tg and 0.4 < tg[0] < 0.6 and 0.4 < tg[1] < 0.6,
              f'a free frame of it: "{sh and sh["text"]}", its target starhole at {tg}')
        check(await ev(f"T.keys('^sky:hole:print@')") == [f"sky:hole:print@{sh['id']}"] if sh else False, 'the print is kept for the board')
        await ev("T.look(200, 6)"); await tick(0.2)
        if not await ev("S47.fcam.raised"): await pg.keyboard.press('KeyC'); await tick(0.2)
        await pg.keyboard.press('Space'); await tick(0.3)
        sh2 = await ev("S47.album.shots.at(-1)")
        if await ev("S47.fcam.raised"): await pg.keyboard.press('KeyC'); await tick(0.2)
        check(PRINT not in sh2['text'] and 'starhole' not in (sh2.get('targets') or {}), f'one the other way, at the comet: not on it ("{sh2["text"]}")')

        # ---------- the board ----------
        await ev("S47.ch1.s.log = true; S47.game.openBoard('SARO', 'ch1')"); await tick(0.2)
        await pg.wait_for_selector('.board-wrap')
        card = await ev("(() => { const c = document.querySelector('[data-card=hole]'); return c ? [c.className, c.textContent.replace(/\\s+/g, ' ').trim(), !!c.querySelector('img.bcard-img'), c.querySelectorAll('.bline').length] : null; })()")
        check(card is not None and 'away' not in card[0] and 'NO STARS AT 026' in card[1] and card[2] and card[3] == 1, f'on the board: NO STARS AT 026, the print on it, one pin ({card and card[1][:90]})')
        bsay = lambda: ev("(document.querySelector('.board-wrap [data-say]')||{}).textContent || ''")
        async def thread(a, b2):
            for n in (a, b2):
                await ev(f"S47.game.board.view.show('{n}')"); await pg.wait_for_timeout(80)
                await pg.click(f'[data-node="{n}"]')
            return await bsay()
        have_s03 = await ev("!!document.querySelector('[data-node=\"s03log.heading\"]')")
        if have_s03:
            r1 = await thread('hole.sky', 's03log.heading')
            check('the one place in the sky with no stars in it' in r1 and await ev("T.keys('^sh:')") == ['sh:hole.sky|s03log.heading'], f'its pin to S-03\'s heading: it holds, and is kept ({r1[:80]})')
        else: check(False, "S-03's motor bus card is on the board with its heading")
        await ev("S47.game.board.view.show('hole')"); await pg.wait_for_timeout(200)
        await shot('desk_06_board')
        await pg.click('.board-wrap .lp-close'); await tick(0.2)

        # ---------- from Caliche ----------
        await ev("window.__p = S47.world.prepare('diner')")
        for _ in range(150):
            if await ev("!!S47.world.town && !!S47.world.diner"): break
            await pg.wait_for_timeout(200)
        await ev("S47.world.enter('diner'); S47.world.placeAtDiner('arrive'); S47.tick(0.3); T.look(26, 26)"); await tick(0.3)
        town = [await ev("S47.space()"), await ev("S47.placeName()"), await ev("S47.sky.hole")]
        tc = await ev("T.stars(26, 32, 1.4)")
        check(town[0] == 'yard' and town[2] == 1 and tc['n'] == 0, f'in Caliche ({town[1]}): the same, no stars there')
        await shot('desk_07_caliche')
        await ev("S47.world.enter('saro'); S47.player.place(9.5, -6.0, 0); S47.tick(0.3)")

        # ---------- Continue ----------
        await ev("S47.game.d.ui.close()"); await tick(0.3)
        await ev("S47.saveNow('manual', 0)")
        await pg.wait_for_timeout(800)
        nb = await ev("S47.game.notes.length")
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev(HELPERS); await ev("T.hook()"); await ev("S47.hold = true; S47.curve.free = true; S47.input.locked = true; S47.tick(0.5)"); await quiet()
        await pg.add_style_tag(content='.lockhint{display:none !important}')
        await ev("S47.setAbductions(false)")
        check(await noticed() and await ev("S47.sky.hole") == 1 and await ev("T.keys('^sh:')") == ['sh:hole.sky|s03log.heading'] and await ev("S47.game.notes.length") == nb,
              'Continue: it is there, noticed, the thread kept, nothing written again')
        await at(X, Z); await ev("T.look(26, 32)"); await tick(0.5, 10)
        sc = await ev("T.stars(26, 32, 1.4)")
        check(sc['n'] == 0 and await ev(f"T.count({json.dumps(NOTE_BIT)})") == 1 and len(await ev("T.log()")) == 1, 'looked at again after Continue: no stars there, nothing written again')

        # ---------- the last picture of the pull-out ----------
        r = await pullout_end(pg, ev, 'desk')
        x, y, front = r['ndc']
        check(r['stage'] == 'pullout' and r['pt'] > 33 and r['area'] == 'roswell', f'the pull-out, {r["pt"]:.1f} s in, the last picture (looking az {r["az"]:.0f}, el {r["el"]:.0f})')
        check(front and abs(x) < 0.5 and -0.7 < y < -0.2 and r['hole'] == 1, f'az 026, el 32 is in it, under the middle and a little left of it (at {x:.2f}, {y:.2f})')
        check(r['in'] == 0 and r['away'] >= 2 and r['dawn'] < 0.3, f'and up there the stars are back round it ({r["away"]} 10 degrees off) and none in it')
        print('\n'.join(errs[:30]) or 'no console errors/warnings (desktop)', flush=True)
        check(not errs, 'console clean (desktop)')
        await ctx.close()

        # ======================= a phone =======================
        ctx, pg, errs = await page(b, 412, 892, True)
        ev = pg.evaluate
        async def ptick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        await ev("S47.hold = true; S47.curve.free = true; S47.jump('chapter2'); S47.tick(1.0); S47.setAbductions(false); S47.game.d.ui.close(true)"); await ev("S47.nightEvents['wait'] = 1e9")
        await ev("S47.game.clock = 3 * 3600 + 40 * 60; S47.world.enter('saro'); S47.player.place(-6, -40, 0); S47.tick(0.2); T.look(26, 30)"); await ptick(0.3)
        check(await ev("S47.space()") == 'yard' and not await ev("S47.starHole.noticed"), f'phone, chapter two, out in the antenna field: not noticed yet')
        await pg.screenshot(path=f'{OUT}/phone_01_along_the_dishes.png')
        await ev("S47.player.place(9.5, -6.0, 0); S47.tick(0.1); T.look(26, 32); S47.fcam.have = true"); await ptick(0.1)
        await pg.tap('.touch .camera'); await ptick(0.1)
        check(await ev("S47.fcam.raised"), 'phone: the Camera button raises the camera')
        await ev("T.toasts.length = 0")
        await pg.touchscreen.tap(206, 446); await ptick(0.2)
        ps = await ev("S47.album.shots.at(-1)")
        if ps: open(f'{OUT}/phone_02_print.jpg', 'wb').write(base64.b64decode(ps['url'].split(',', 1)[1]))
        pt = await ev("T.toasts.slice()")
        check(ps and PRINT in ps['text'] and (ps.get('targets') or {}).get('starhole') and await ev("S47.starHole.noticed")
              and any('In the viewfinder, up where the dishes point: no stars.' in t for t in pt) and await ev(f"T.count({json.dumps(NOTE_BIT)})") == 1 and await ev("T.has('sky:hole/ch2')"),
              f'phone: a tap for the shutter, and through the viewfinder it is noticed, once ("{ps and ps["text"]}")')
        await pg.wait_for_timeout(500); await ptick(0.05)
        await pg.screenshot(path=f'{OUT}/phone_03_viewfinder.png')
        await pg.tap('.touch .camera'); await ptick(0.2)
        await ev("T.look(26, 32)"); await ptick(0.5, 8)
        check(await ev(f"T.count({json.dumps(NOTE_BIT)})") == 1 and len(await ev("T.log()")) == 1, 'phone: looked at for 4 s after it: nothing more')
        await ev("S47.game.openBoard('SARO', 'ch2')"); await ptick(0.2)
        await pg.wait_for_selector('.board-wrap')
        check(await ev("!!document.querySelector('[data-card=hole] img.bcard-img')"), 'phone: the card on the board, with the print, in chapter two\'s cork')
        await ev("S47.game.board.view.show('hole')"); await pg.wait_for_timeout(200)
        await pg.screenshot(path=f'{OUT}/phone_04_board.png')
        await pg.tap('.board-wrap .lp-close'); await ptick(0.2)
        r = await pullout_end(pg, ev, 'phone')
        x, y, front = r['ndc']
        check(r['stage'] == 'pullout' and front and abs(x) < 0.6 and -0.7 < y < -0.15 and r['in'] == 0 and r['away'] >= 2,
              f'phone: the last picture of the pull-out has it too (at {x:.2f}, {y:.2f}; fov {r["fov"]:.0f}), the stars round it')
        print('\n'.join(errs[:30]) or 'no console errors/warnings (phone)', flush=True)
        check(not errs, 'console clean (phone)')

        json.dump(checks, open(f'{OUT}/checks.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
