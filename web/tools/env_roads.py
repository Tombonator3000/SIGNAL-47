# Roads and paved surfaces at night (drive/roadSurface.ts, roadTextures.ts, roadProps.ts and the
# lots in the area files). Shoots the same eight views on High every time: the highway south of
# SARO from the shoulder walk, the service road and the truck pad, the motel's lot, Caliche's main
# street, the diner's lot and the old road's mouth across the highway, the highway and the survey
# track in the truck's headlights, and the old Roswell road at dawn. Prints the draw calls of each,
# and checks them against what they were before this pass (at most 15 more per area), that Low
# draws no more than High and has the wear decals and the wet sheen off, that the wear decals and
# the sheen are there on High, that the lots have their 1024 picture (8 m) on High and Low alike,
# that the dry grass is in the ground and Low draws a third of it, that the sheen lays a column
# under SARO's lamps and none from the truck's own headlights, that the new things by the road
# stand clear of the lanes and the
# track, and that the player still walks the service road, the shoulder to the motel, the motel's
# lot and Caliche's street with the real movement code.
# Usage: python3 tools/env_roads.py OUTDIR [WxH] [--shots]   (--shots: the views only, no checks)
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

args = [a for a in sys.argv[1:] if not a.startswith('--')]
SHOTS_ONLY = '--shots' in sys.argv
OUT = args[0]
W, H = (int(v) for v in (args[1] if len(args) > 1 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
X = 8000                                    # the road area's origin (World.ROAD_ORIGIN)
SX, SZ = 24, -300                           # road-local = SARO + (24, 0, -300) (geo.ts)
NIGHT = 1 * 3600 + 20 * 60                  # 01:20, the dead of the night
DAWN = 5 * 3600 + 20 * 60                   # 05:20, the old road as chapter six has it
# draw calls on High at each view before this pass (19 October, RTX 3080, 1280x800)
BEFORE = {'r1_highway_from_the_shoulder': 326, 'r2_service_road_and_pad': 293, 'r3_motel_lot': 369, 'r4_caliche_main_street': 56,
          'r5_diner_lot_from_the_highway': 92, 'r6_ranch_road_in_the_headlights': 337, 'r7_survey_track_in_the_headlights': 50, 'r8_old_road_at_dawn': 50}
checks = []
def check(ok, what): checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

# (name, area, x, z, yaw, pitch, clock): on foot in the area's own terms; area 'truck' and
# 'track' drive the road's truck (x, z road-local, yaw its heading), 'oldroad' the old road's
VIEWS = [
    ('r1_highway_from_the_shoulder', 'saro', -28.5, 92.0, 0.06, -0.07, NIGHT),
    ('r2_service_road_and_pad', 'saro', 1.5, 26.5, -0.6, -0.12, NIGHT),
    ('r3_motel_lot', 'saro', -31.6, 696.0, 0.62, -0.12, NIGHT),
    ('r4_caliche_main_street', 'diner', X - 1.5, 2036.0, math.pi, -0.06, NIGHT),
    ('r5_diner_lot_from_the_highway', 'diner', X + 3.0, 2013.0, 0.85, -0.1, NIGHT),
    ('r6_ranch_road_in_the_headlights', 'truck', 2.6, 207.0, -0.5, 0.0, NIGHT),
    ('r7_survey_track_in_the_headlights', 'track', -262.0, 495.6, math.pi / 2 + 0.03, 0.0, NIGHT),
    ('r8_old_road_at_dawn', 'oldroad', 0, 0, 0, 0.0, DAWN),
]
# with --extra, a few more to look at (no budgets kept for them)
EXTRA = [
    ('x1_track_mouth_in_the_headlights', 'truck', -1.8, 500.0, 2.57, 0.0, NIGHT),
    ('x2_highway_in_the_headlights', 'truck', -1.8, 1640.0, math.pi, 0.0, NIGHT),
    ('x3_stop_line', 'saro', -12.0, 27.0, 1.2, -0.4, NIGHT),
    ('x4_gas_station_apron', 'diner', X - 8.0, 2066.0, 0.1, -0.45, NIGHT),
    ('x5_motel_stalls', 'saro', -31.0, 690.0, 0.89, -0.3, NIGHT),
]
if '--extra' in sys.argv: VIEWS = VIEWS + EXTRA

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': W, 'height': H})).new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        # the views without the toasts and the pointer hint over them
        await pg.add_init_script("addEventListener('DOMContentLoaded', () => { const s = document.createElement('style'); s.textContent = '.toasts, .lockhint { display: none !important; }'; document.head.appendChild(s); });")
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(400)
        ev = pg.evaluate
        await ev("S47.hold = true; S47.setAbductions(false); S47.jump('chapter1'); S47.tick(1.0)")
        await ev("S47.setPicture('off'); S47.setQuality('high'); S47.tick(0.1)")
        await ev("S47.world.prepare('diner')")
        for _ in range(150):
            if await ev("!!S47.world.town && !!S47.world.diner && !!S47.world.road"): break
            await pg.wait_for_timeout(200)

        async def calls(): return await ev("S47.renderer.info.render.calls")
        async def until(js, n=120):
            for _ in range(n):
                if await ev(js): return True
                await ev("S47.tick(0.25)")
                await pg.wait_for_timeout(50)
            return False
        async def stop_driving():
            if await ev("S47.world.free !== null && S47.world.driving"):
                await ev("S47.world.endFreeDrive()"); await ev("S47.tick(0.2)")

        async def go(v):
            name, area, x, z, yaw, pitch, clock = v
            await ev(f"S47.game.clock = {clock}")
            if area in ('saro', 'diner'):
                await stop_driving()
                await ev(f"S47.world.enter('{area}'); S47.player.place({x}, {z}, {yaw}); S47.player.pitch = {pitch}; S47.tick(0.5)")
            elif area in ('truck', 'track'):
                if not await ev("S47.world.free === 'road' && S47.world.driving"):
                    await stop_driving()
                    await ev("S47.world.freeDrive('road')")
                    await until("S47.world.driving && S47.world.free === 'road'")
                await ev(f"(() => {{ const w = S47.world, L = w.legs.road; L.drive.place(new S47.three.Vector3({X + x}, L.area.height({X + x}, {z}), {z}), {yaw}); w['leg'] = 'road'; w.enter('road'); }})()")
                await ev("S47.tick(0.6)")
            else:
                await stop_driving()
                await ev("S47.world.freeDrive('oldroad')")
                await until("S47.world.driving && S47.world.free === 'oldroad'", 240)
                await ev("S47.tick(0.6)")

        async def shoot(v, tag=''):
            await go(v)
            await pg.screenshot(path=f'{OUT}/{v[0]}{tag}.png')
            return await calls()

        high = {}
        for v in VIEWS:
            high[v[0]] = await shoot(v)
            print(f'{v[0]}: {high[v[0]]} draw calls on High', flush=True)
        # the wear decals shown, and the programs of the wet materials with the sheen in them
        SHEEN = """(() => { const gl = S47.renderer.getContext(), out = { std: 0, stdWet: 0, lam: 0, lamWet: 0 };
          for (const p of S47.renderer.info.programs) { if (!p.cacheKey.includes('wet')) continue;
            const src = gl.getShaderSource(p.fragmentShader) || '', lam = src.includes('#define LAMBERT'), w = src.includes('wetness(');
            if (lam) { out.lam++; if (w) out.lamWet++; } else { out.std++; if (w) out.stdWet++; } }
          let n = 0, shown = 0; S47.scene.traverse((o) => { if (o.userData.lowOff) { n++; let v = true; for (let q = o; q; q = q.parent) v = v && q.visible; if (o.visible) shown++; } });
          out.decals = n; out.shown = shown;
          // the lots' asphalt (its picture's width) and the grounds with dry grass in them (how much is drawn)
          out.lot = []; out.grass = [];
          S47.scene.traverse((o) => { const m = o.material;
            if (m && m.map && m.map.name === 'lotAsphalt' && !out.lot.includes(m.map.image.width)) out.lot.push(m.map.image.width);
            if (o.userData.lowRange !== undefined) out.grass.push([o.name, o.userData.lowRange, o.geometry.index.count, o.geometry.drawRange.count === Infinity ? -1 : o.geometry.drawRange.count]); });
          return out; })()"""
        hi_sheen = await ev(SHEEN)
        await ev("S47.setQuality('low'); S47.tick(0.1)")
        low = {}
        for v in VIEWS:
            low[v[0]] = await shoot(v, '_low')
            print(f'{v[0]}: {low[v[0]]} draw calls on Low', flush=True)
        lo_sheen = await ev(SHEEN)
        await stop_driving()
        json.dump({'high': high, 'low': low}, open(f'{OUT}/calls.json', 'w'), indent=1)
        if SHOTS_ONLY:
            print('\n'.join(errs[:20]) or 'no console errors/warnings')
            await b.close(); return

        # ---------- draw calls ----------
        over = {k: (v, BEFORE[k]) for k, v in high.items() if k in BEFORE and v > BEFORE[k] + 15}
        check(not over and all(k in high for k in BEFORE), f'at most 15 more draw calls than before at every view on High ({ {k: high[k] for k in BEFORE} }; before {BEFORE})')
        house = [high[k] for k in ('r1_highway_from_the_shoulder', 'r2_service_road_and_pad')]
        check(max(house) <= 340, f'by the house SARO keeps the grounds\' budget of 340 ({house})')
        more = {k: (low[k], high[k]) for k in high if low[k] > high[k]}
        check(not more, f'Low draws no more than High anywhere ({low})')

        # ---------- the sheen and the decals, High and Low ----------
        check(hi_sheen['decals'] >= 6 and hi_sheen['shown'] == hi_sheen['decals'], f"High shows the wear decals of every area ({hi_sheen['shown']} of {hi_sheen['decals']})")
        check(hi_sheen['std'] > 0 and hi_sheen['stdWet'] == hi_sheen['std'], f"High's wet surfaces are compiled with the sheen ({hi_sheen['stdWet']} of {hi_sheen['std']} programs)")
        check(lo_sheen['shown'] == 0 and lo_sheen['lam'] > 0 and lo_sheen['lamWet'] == 0,
              f"Low hides the decals ({lo_sheen['shown']} shown) and its cheap copies go without the sheen ({lo_sheen['lamWet']} of {lo_sheen['lam']} programs have it)")
        check(hi_sheen['lot'] == [1024] and lo_sheen['lot'] == [1024], f"the lots' asphalt is one 1024 px picture to 8 m, High ({hi_sheen['lot']}) and Low ({lo_sheen['lot']}) alike")
        g_hi, g_lo = {g[0]: g for g in hi_sheen['grass']}, {g[0]: g for g in lo_sheen['grass']}
        check(len(g_hi) >= 2 and all(g[3] == -1 and g[2] - g[1] > 3000 for g in g_hi.values()) and all(g_lo[k][3] == g_lo[k][1] for k in g_hi if k in g_lo) and len(g_lo) == len(g_hi),
              'the dry grass is merged into the ground (no draw call), all of it drawn on High and a third on Low: ' + ', '.join(f"{k} {(g[2] - g[1]) // 6 * 3 // 2} blades" for k, g in g_hi.items()))
        await ev("S47.setQuality('high'); S47.tick(0.1)")
        await stop_driving()
        # what it adds: the same view with the sheen turned off, on the highway under SARO's lamps
        # (the stretch between the lamps and the eye, where their reflection lies: a narrow column
        # where the road is damp, so the pixels it brightens are counted, not the mean, which the
        # picture's exposure moves the other way)
        from PIL import Image, ImageChops
        CROP = (int(W * 0.52), int(H * 0.47), int(W * 0.75), int(H * 0.62))
        def lit_by(a, b): return sum(ImageChops.subtract(Image.open(a).convert('L').crop(CROP), Image.open(b).convert('L').crop(CROP)).histogram()[9:])
        await go(VIEWS[0]); await ev("S47.tick(1.0)"); await ev("S47.tick(0.05)"); await pg.screenshot(path=f'{OUT}/sheen_on.png')
        await ev("S47.scene.traverse((o) => { const m = o.material; if (m && m.userData && m.userData.wet && m.userData.wet0 === undefined) { m.userData.wet0 = m.userData.wet.value.x; m.userData.wet.value.x = 0; } }); S47.tick(0.05)")
        await pg.screenshot(path=f'{OUT}/sheen_off.png')
        await ev("S47.scene.traverse((o) => { const m = o.material; if (m && m.userData && m.userData.wet0 !== undefined) { m.userData.wet.value.x = m.userData.wet0; delete m.userData.wet0; } }); S47.tick(0.05)")
        on, off = lit_by(f'{OUT}/sheen_on.png', f'{OUT}/sheen_off.png'), lit_by(f'{OUT}/sheen_off.png', f'{OUT}/sheen_on.png')
        check(on > 500 and on > off * 2.5, f'the damp sheen catches SARO\'s lamps on the highway: {on} pixels of the road there brighter with it, {off} darker')
        # and not the truck's own headlights: on the highway out of town, in the beam ahead
        await go(EXTRA[1]); await ev("S47.tick(0.05)"); await pg.screenshot(path=f'{OUT}/beam_on.png')
        await ev("S47.scene.traverse((o) => { const m = o.material; if (m && m.userData && m.userData.wet && m.userData.wet0 === undefined) { m.userData.wet0 = m.userData.wet.value.x; m.userData.wet.value.x = 0; } }); S47.tick(0.05)")
        await pg.screenshot(path=f'{OUT}/beam_off.png')
        await ev("S47.scene.traverse((o) => { const m = o.material; if (m && m.userData && m.userData.wet0 !== undefined) { m.userData.wet.value.x = m.userData.wet0; delete m.userData.wet0; } }); S47.tick(0.05)")
        await stop_driving()
        CROP = (int(W * 0.3), int(H * 0.47), int(W * 0.7), int(H * 0.56))
        beam = lit_by(f'{OUT}/beam_on.png', f'{OUT}/beam_off.png')
        check(beam < 150, f"the truck's own headlights lay no sheen on the road ahead ({beam} pixels in the beam brighter with it)")
        # and it adds no colour the half-float picture cannot hold (memory.md: one NaN blooms over the screen)
        hdr = {}
        for v in (VIEWS[0], VIEWS[3], VIEWS[5]):
            await go(v); hdr[v[0][:2]] = await ev("S47.hdrStats()")
        await stop_driving()
        check(all(h['over'] == 0 and h['max'] < 60 for h in hdr.values()), 'no colour over the half-float limit or not a number with the sheen: ' + ', '.join(f"{k} {h['over']} over, brightest {h['max']:.1f}" for k, h in hdr.items()))

        # ---------- the survey track's ruts: in the physics and in the mesh, the same ----------
        ruts = await ev("""(() => { const r = S47.world.road, X = 8000, x = -262, ys = [];
          for (let z = 492; z <= 499.5; z += 0.02) ys.push([z, r.height(X + x, z)]);
          // the crown: highest point near the middle; the ruts: the lowest each side of it
          const mid = ys.slice(100, 300).reduce((a, b) => b[1] > a[1] ? b : a);
          const left = ys.filter((p) => p[0] < mid[0] && p[0] > mid[0] - 1.6).reduce((a, b) => b[1] < a[1] ? b : a);
          const right = ys.filter((p) => p[0] > mid[0] && p[0] < mid[0] + 1.6).reduce((a, b) => b[1] < a[1] ? b : a);
          const trk = S47.scene.getObjectByName('surveyTrack'), rc = new S47.three.Raycaster(), off = [];
          for (const p of [left, mid, right]) { rc.set(new S47.three.Vector3(X + x, 50, p[0]), new S47.three.Vector3(0, -1, 0)); const h = rc.intersectObject(trk)[0]; off.push(h ? h.point.y - p[1] : 99); }
          return { depth: mid[1] - (left[1] + right[1]) / 2, apart: right[0] - left[0], off }; })()""")
        await ev("S47.world.enter('saro'); S47.tick(0.05)")
        check(0.03 < ruts['depth'] < 0.08 and 1.4 < ruts['apart'] < 2.0 and max(abs(v) for v in ruts['off']) < 0.012,
              f"the survey track has two ruts {ruts['apart']:.2f} m apart, {ruts['depth'] * 100:.1f} cm down, where the truck drives and where it is drawn (mesh against physics {', '.join(f'{v * 100:.1f}' for v in ruts['off'])} cm)")

        # ---------- the new things by the road stand clear of the way the truck goes ----------
        clear = await ev("""(() => { const r = S47.world.road, R = r.route, bad = [];
          const seg = (px, pz, a, b) => { const dx = b.x - a.x, dz = b.z - a.z, t = Math.max(0, Math.min(1, ((px - a.x) * dx + (pz - a.z) * dz) / (dx * dx + dz * dz || 1))); return Math.hypot(a.x + dx * t - px, a.z + dz * t - pz); };
          let n = 0;
          for (const o of r.obstacles) { if (o.kind !== 'circle') continue; n++;
            let d = Infinity; for (let i = 0; i < R.length - 1; i++) d = Math.min(d, seg(o.x, o.z, R[i], R[i + 1]));
            if (Math.abs(o.x - 8000) < 6.2 && o.z > -120 && o.z < 2700) d = Math.min(d, 0);   // on the highway itself
            if (d < 2.2 + o.r) bad.push([+(o.x - 8000).toFixed(1), +o.z.toFixed(1), +d.toFixed(2)]); }
          return { n, bad }; })()""")
        check(not clear['bad'], f"nothing the truck can hit stands on the highway or within reach of the way to the station ({clear['n']} posts, rocks and pipes; {clear['bad']})")

        # ---------- on foot, with the real movement code ----------
        async def walk(points, limit=9000):
            return await ev(f'''(()=>{{const pts={json.dumps(points)}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.15) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > {limit}) return [false, x, z, p.pos.x, p.pos.z]; }} }}
              return [true, p.pos.x, p.pos.z]; }})()''')
        await ev("S47.world.enter('saro'); S47.player.place(9.8, 17.6, Math.PI); S47.tick(0.1)")
        r = await walk([[9.8, 20.5], [8.6, 23.3], [4.0, 24.5], [-6.0, 24.5], [-16.0, 27.0], [-24.0, 36.0], [-24.0, 300.0], [-24.0, 640.0], [-31.0, 662.0], [-34.5, 690.0], [-39.1, 692.0], [-39.1, 717.0]])
        check(r[0], 'down the ramp, along the service road over its oil and rubber, down the highway to the motel and over its lot to the stalls' + ('' if r[0] else f' (stuck going to {r[1]},{r[2]} at {r[3]:.2f},{r[4]:.2f})'))
        await ev("S47.world.enter('diner'); S47.world.placeAtDiner('arrive'); S47.tick(0.3)")
        r = await walk([[X - 10, 2021], [X - 8, 2045], [X - 7.6, 2066], [X - 13, 2096], [X - 13.9, 2100], [X - 13.9, 2111], [X - 13.1, 2114], [X - 13.1, 2180], [X + 17.6, 2190], [X + 17.8, 2060]])
        check(r[0], "Caliche on foot: the diner's lot, the gas station's apron by the pumps, the west sidewalk past the laundromat, across and up the east side" + ('' if r[0] else f' (stuck going to {r[1]},{r[2]} at {r[3]:.2f},{r[4]:.2f})'))
        await ev("S47.world.enter('saro'); S47.player.place(0, 0, 0); S47.tick(0.1)")
        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        bad = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not bad, 'console clean' + ('' if not bad else ': ' + '; '.join(bad[:4])))
        json.dump(checks, open(f'{OUT}/checks.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        if fails: raise SystemExit(1)
asyncio.run(main())
