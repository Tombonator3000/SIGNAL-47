# HENDELSER.md, events 2 and 10, on B (story/pasture.ts, drive/herd.ts, story/motelTv.ts).
# The herd in the pasture south of the motel: SARO's stretch of the highway and the road's have
# the same herd. Driven the whole way with the autopilot, at night: out to STATION 01 past them
# grazing (heads moving, every which way, no change while the truck is near); out there, far off,
# every cow faces az 026 and nothing moves; back along the track, the headlights on them: nine
# head, in the notebook, a mark on the map on B. At SARO, far off, one goes and a ring of frost
# lies where it stood; driven south to the diner past them: eight, their eyes in the beam, the
# finding as written. At the diner they graze again, eight and the ring.
# The office TV: the test card all night; once, with the player in the office and the truck
# driven onto the lot, the bare carrier for 47 s (the frame's shaders made ready meanwhile), then
# one frame of the lot from straight above with the truck in it, the finding as written, the test
# card again, and never again. Unseen (the player away) it can come back. Three marks on B: the
# line is drawn in. After a save and Continue none of it comes again, the truck is seen on the
# lot, and the pass by the herd is kept. What stops the truck in the pasture: the cows, the one
# that goes only while it is there, and the stock tank. Pictures of the herd in the headlights,
# the TV and its frame, and the map.
# Usage: python3 tools/cowstv.py OUTDIR [WxH]
import asyncio, sys, json, os, math, base64
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

HERD = "S47.world.herds.find((h) => h.area === '{a}')"
# every cow's facing on the ground (the body's -z), and all their matrices, of one herd
FACES = "((h) => { const a = h.bodies.instanceMatrix.array, out = []; for (let i = 0; i < h.count; i++) { const x = -a[i * 16 + 8], z = -a[i * 16 + 10], l = Math.hypot(x, z); out.push(l ? [x / l, z / l] : null); } return out; })"
MATS = "((h) => Array.from(h.heads.instanceMatrix.array).concat(Array.from(h.bodies.instanceMatrix.array)))"
STATE = """(() => { const w = S47.world, L = w.leg && w.legs[w.leg], D = L && L.drive;
  return { area: w.area, leg: w.leg, driving: w.driving, at: w.truckAt, x: D ? D.pos.x : null, z: D ? D.pos.z : null, clock: S47.game.clock }; })()"""
# the pixel of the TV's screen at (x, y), and the mean of the picture
PIX = "((x, y) => Array.from(S47.world.motel.tv.canvas.getContext('2d').getImageData(x, y, 1, 1).data.slice(0, 3)))"
FACING = (math.sin(math.radians(26)), -math.cos(math.radians(26)))

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': W, 'height': H})).new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
        except Exception: pass
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        ev = pg.evaluate
        async def tick(s, fps=30): await ev(f"S47.tick({s}, {fps})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        notes = lambda: ev("S47.game.notes.slice()")
        seen = lambda k: ev(f"S47.townSeen.has('{k}')")
        stage = lambda: ev("S47.pasture.stage")
        toasts = lambda: ev("[...document.querySelectorAll('.toast')].map((e) => e.textContent).join(' ')")
        async def wait_trip():
            for _ in range(240):
                await tick(0.25)
                if await ev("S47.world.driving"): return True
                await pg.wait_for_timeout(100)
            return False

        # (the night's one curve, story/curve.ts, stands aside in this test: each thing on its own timer; tools/f_curve.py has them together)
        # (the herd's own order: the ninth cow is not put down at STATION 01 here; tools/f_ninth_cow.py has her)
        await ev("S47.ninth.off = true")
        await ev("S47.hold = true; S47.curve.free = true; S47.tick(0.3); S47.jump('chapter3'); S47.tick(1.0); S47.setPicture('off')")
        # nothing else on B in the way: the lights along the highway (event 7) and the time away are off
        await ev("S47.lineLights.reset(true); S47.setAbductions(false)")
        await ev("window.__r = S47.world.prepare('road')")
        for _ in range(150):
            if await ev("S47.world.herds.length >= 2"): break
            await pg.wait_for_timeout(200)

        # ---------- the herd: one for each area, the same ----------
        hs = await ev("S47.world.herds.map((h) => [h.area, h.count, ...h.middle().toArray()])")
        sa = next(h for h in hs if h[0] == 'saro'); ro = next(h for h in hs if h[0] == 'road')
        check(sa[1] == ro[1] == 9 and abs(sa[2] + 24 - (ro[2] - 8000)) < 1e-6 and abs(sa[4] - 300 - ro[4]) < 1e-6 and abs(sa[3] - ro[3]) < 1e-6,
              f'SARO\'s stretch and the road have the same herd of nine, in the same place ({sa[2]:.1f}, {sa[4]:.1f} on SARO\'s map)')
        mid = (sa[2], sa[4])
        # what stops the truck there (the road's area; SARO's stretch keeps it between the fences)
        OBST = """(() => { const h = S47.world.herds.find((x) => x.area === 'road'), r = (x, z) => { const p = h.group.localToWorld(new S47.three.Vector3(x, 0, z));
          return S47.world.road.obstacles.filter((o) => o.kind === 'circle' && Math.hypot(o.x - p.x, o.z - p.z) < 0.3).map((o) => o.r); };
          return { cow0: r(-27.0, 538.5), cow4: r(-30.5, 557.5), tank: r(-45.0, 537.5) }; })()"""
        ob = await ev(OBST)
        check(ob['cow0'] == [1.15] and ob['cow4'] == [1.15] and ob['tank'] == [1.3], f'the cows stop the truck, all nine while they are there, and the stock tank too ({ob})')

        # ---------- grazing; driven out past them, nothing changes while the truck is by them ----------
        await ev("S47.game.clock = 3 * 3600 + 30 * 60"); await tick(2)
        m0 = await ev(f"{MATS}({HERD.format(a='saro')})"); await tick(2); m1 = await ev(f"{MATS}({HERD.format(a='saro')})")
        check(await stage() == 'graze' and m0 != m1, 'they graze, each its own way, heads moving')
        f = await ev(f"{FACES}({HERD.format(a='saro')})")
        check(len({(round(v[0], 2), round(v[1], 2)) for v in f if v}) >= 6, 'grazing, they face every which way')
        DIST = """(() => { const w = S47.world, h = w.herds.find((x) => x.area === (w.area === 'saro' ? 'saro' : 'road')); if (!h || (w.area !== 'saro' && w.area !== 'road')) return 1e9;
          return S47.camera.getWorldPosition(new S47.three.Vector3()).distanceTo(h.middle()); })()"""
        async def drive(start, what, until=None, shots=()):
            """Drive with the autopilot until parked (or until(), a JS condition); the stage and the
            distance to the herd every quarter second; pictures when the truck is near a z in shots."""
            await ev(start)
            ok = await wait_trip()
            await ev("S47.world.autopilot = 12")
            trace, taken, hit = [], set(), None
            for _ in range(1600):
                st = await ev(STATE)
                if not st['driving']: break
                trace.append((await stage(), await ev(DIST), st))
                for name, area, z in shots:
                    if name not in taken and st['area'] == area and st['z'] is not None and abs(st['z'] - z) < 6:
                        taken.add(name); await shot(name)
                if until and await ev(until):
                    # the toasts go on real time, and a picture takes a while here: read them first
                    hit = st; hit['toasts'] = await toasts(); await shot(f'{what}_seen'); break
                await tick(0.25)
            return ok, trace, hit
        await ev("S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.tick(0.1)")
        n0 = len(await notes())
        ok, trace, _ = await drive("S47.world.driveOut()", 'out', shots=[('c01_graze_headlights', 'saro', 760)])
        for _ in range(600):
            if not await ev("S47.world.driving"): break
            await tick(0.5)
        await tick(1.0)
        near = [t for t in trace if t[1] < 260]
        check(ok and len(near) > 20 and all(t[0] == 'graze' for t in near), f'driven past them at night: grazing all the while the truck is within 260 m ({len(near)} looks, nearest {min(t[1] for t in trace):.0f} m)')
        st = await ev(STATE)
        check(st['area'] == 'station01' and st['at'] == 'station01', 'on to STATION 01, parked by the gate')
        check(await stage() == 'still' and await seen('cows:on') and await ev("S47.pasture.passes") >= 1 and await seen('cows:by'), 'out there, far off, after the player has been by them once (kept with the case): the herd turns')
        f = await ev(f"{FACES}({HERD.format(a='saro')})")
        off = max(math.degrees(math.acos(max(-1, min(1, v[0] * FACING[0] + v[1] * FACING[1])))) for v in f if v)
        f2 = await ev(f"{FACES}({HERD.format(a='road')})")
        off2 = max(math.degrees(math.acos(max(-1, min(1, v[0] * FACING[0] + v[1] * FACING[1])))) for v in f2 if v)
        check(off < 0.5 and off2 < 0.5, f'every cow faces az 026, in both areas (at most {max(off, off2):.2f} degrees off)')
        m0 = await ev(f"{MATS}({HERD.format(a='road')})"); await tick(3); m1 = await ev(f"{MATS}({HERD.format(a='road')})")
        check(m0 == m1, 'and nothing moves at all')
        check(not any('herd' in n or '026' in n for n in (await notes())[n0:]), 'turning is not written down: nobody saw it')

        # ---------- driven back along the track: nine, in the headlights ----------
        n1 = len(await notes())
        ok, trace, at = await drive("S47.world.driveBack()", 'c02_back', until="S47.townSeen.has('ev:cows1')")
        nn = (await notes())[n1:]
        check(ok and at is not None and any('Nine head' in n and '026' in n and 'headlights' in n for n in nn), f'in the headlights, back along the track: written down ({nn[:1]})')
        if at: print(f"  seen from the {at['area']}'s truck at ({at['x']:.0f}, {at['z']:.0f}), {min(t[1] for t in trace):.0f} m from the herd at the nearest")
        check(await ev("[...S47.townSeen].some((k) => k.startsWith('mark:cows@'))"), 'a mark for the map')
        for _ in range(600):
            if not await ev("S47.world.driving"): break
            await tick(0.5)
        await tick(1.0)
        hh = await ev("S47.world.herds.map((h) => [h.still, h.missing, h.ring.visible])")
        check(await ev("S47.world.truckAt") == 'saro' and await stage() == 'short' and await seen('cows:short') and all(x == [True, 4, True] for x in hh),
              f'back at SARO, far off: one of them is gone, and a ring of frost where it stood ({hh})')

        # ---------- driven south to the diner: eight ----------
        n2 = len(await notes())
        EYES = "(() => { const w = S47.world, h = w.herds.find((x) => x.area === (w.area === 'saro' ? 'saro' : 'road')), c = h.glow.points.geometry.getAttribute('aCol'); let s = 0; for (const i of h.eyes) s += c.getX(i) + c.getY(i); return s; })()"
        ok, trace, at = await drive("S47.world.driveToDiner(() => {})", 'c03_south', until="S47.townSeen.has('ev:cows')")
        eyes = await ev(EYES) if at else 0
        nn = (await notes())[n2:]
        check(ok and at is not None and 'Pasture by the highway: the herd facing 026. One missing. A ring of frost in the grass.' in ' '.join(nn), f'driven south past them: the finding as written ({nn[-1:]})')
        check(at is not None and 'Eight of them' in at['toasts'], 'eight of them, there were nine')
        check(eyes > 0.1, f'coming down from the north into the way they face, their eyes shine back in the beam ({eyes:.2f})')
        if at: print(f"  seen from the {at['area']}'s truck at ({at['x']:.0f}, {at['z']:.0f})")
        for _ in range(900):
            if not await ev("S47.world.driving"): break
            await tick(0.5)
        await tick(1.0)
        hh = await ev("S47.world.herds.map((h) => [h.still, h.missing, h.ring.visible])")
        check(await ev("S47.world.truckAt") == 'diner' and await stage() == 'after' and all(x == [False, 4, True] for x in hh),
              f'at the diner, far off: the eight graze again, the ring stays ({hh})')
        ob = await ev(OBST)
        check(ob['cow0'] == [1.15] and ob['cow4'] == [0] and ob['tank'] == [1.3], f'where the ninth stood nothing stops the truck now ({ob})')
        mk = await ev("S47.mapMarks().map((m) => [m.kind, m.line, m.at ? m.at.spot : null, m.at ? m.at.m : null])")
        check(any(m[0] == 'cows' and m[1] == 'B' and m[2] == 'highway' and 815 < m[3] < 900 for m in mk), f'on the map on B, down the highway just past the track ({mk})')
        await ev("S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.tick(0.2)")

        # ---------- the office TV ----------
        motel = await ev("(() => { const m = S47.world.motel, v = m.tv.group.getWorldPosition(new S47.three.Vector3()), a = m.anchors.officeInside; return { tv: [v.x, v.y, v.z], a: [a.x, a.z] }; })()")
        tv, a = motel['tv'], motel['a']
        into = f"S47.player.place({a[0]}, {a[1]}, Math.atan2(-({tv[0]} - {a[0]}), -({tv[2]} - {a[1]}))); S47.player.pitch = 0.05; S47.tick(0.1)"
        # unseen: the carrier and the frame come while the player is far off; it may come again
        await ev("S47.game.clock = 4 * 3600 + 5 * 60")
        n3 = len(await notes())
        await ev("S47.motelTv.now()"); await tick(1)
        check(await ev("S47.motelTv.mode") == 'carrier', 'the carrier, with nobody there (the developer menu)')
        await tick(47)
        # only the TV's own note counts: the night's other directors (core/cutouts.ts, story/nightEvents.ts)
        # may write theirs in these 47 s
        new = (await notes())[n3:]
        check(await ev("S47.motelTv.mode") in ('frame', 'card') and not await seen('ev:tv') and not any('TV' in n or 'television' in n.lower() for n in new),
              f'nobody saw it: nothing written, and it is not spent ({new})')
        await tick(3)
        await ev("S47.game.clock = 4 * 3600 + 17 * 60")
        await ev(into); await tick(2)
        check(await ev("S47.motelTv.mode") == 'card' and not await ev("S47.abductIn().line"), 'in the office: the test card; the office is not out on the line')
        await tick(8)
        check(await ev("S47.motelTv.mode") == 'card', 'with the truck at SARO, nothing happens on it')
        # near the top: the round 15 card (motel/tv_testcard_blank.jpg) has its circle over the middle of the bars
        p1, p2, p3 = await ev(f"{PIX}(20, 8)"), await ev(f"{PIX}(56, 8)"), await ev(f"{PIX}(128, 8)")
        check(abs(p1[0] - p1[2]) < 25 and p1[0] > 60 and p2[0] > 70 and p2[2] < p2[0] * 0.5 and p3[1] > p3[0] + 30, f'on the screen the colour bars: grey, yellow, green ({p1}, {p2}, {p3})')
        check(await ev("S47.world.motel.flood.pos[S47.world.motel['slots'].tv].w") > 0.5, 'the set lights the office')
        await shot('t01_testcard')
        # the truck driven down to the motel and left on its lot, as in chapter four
        ok, _, _ = await drive("S47.world.driveToMotel(() => {})", 't_motel')
        for _ in range(200):
            if not await ev("S47.world.driving"): break
            await tick(0.5)
        await tick(1.0)
        st = await ev(STATE)
        check(ok and st['at'] == 'motel' and st['area'] == 'saro' and await ev("S47.world.saroTruck.group.visible"), f'driven to the motel and left on its lot: the truck stands there, seen ({st})')
        await ev(into)
        for _ in range(30):
            await tick(0.5)
            if await ev("S47.motelTv.mode") != 'card': break
        check(await ev("S47.motelTv.mode") == 'carrier' and await ev("S47.motelTv.busy") and 'blank grey screen' in await toasts(), 'the truck on the lot and the player in the office: the test card goes, the carrier')
        g = await ev(f"{PIX}(128, 96)")
        check(max(g) - min(g) < 20 and await ev("S47.musicWant() === null || true"), f'a grey field ({g})')
        await shot('t02_carrier')
        await tick(30)
        check(await ev("S47.motelTv.mode") == 'carrier', 'thirty seconds on, still the carrier')
        n4 = len(await notes())
        progs = await ev("S47.renderer.info.programs.length")
        # Measure the actual aerial render. The remaining carrier also lets ordinary main-view
        # shaders and the prewarm itself compile, so its whole 17.2 s count is diagnostic only.
        await ev("""(() => { const T = S47.motelTv, r = S47.renderer, shoot = T['shoot'];
          window.__tvCaptures = [];
          T['shoot'] = function (...args) {
            const before = r.info.programs.map((p) => p.id); let success = false;
            try { const frame = shoot.apply(this, args); success = !!frame; return frame; }
            finally { const after = r.info.programs.map((p) => p.id);
              window.__tvCaptures.push({ before: before.length, after: after.length, success,
                newProgramIds: after.filter((id) => !before.includes(id)) }); }
          };
          window.__restoreTvShoot = () => { T['shoot'] = shoot; };
        })(); 0""")
        try:
            await tick(17.2)
        finally:
            await ev("window.__restoreTvShoot(); 0")
        progs2 = await ev("S47.renderer.info.programs.length")
        captures = await ev("window.__tvCaptures")
        successful = [c for c in captures if c['success']]
        shader_counts = {'interval': {'before': progs, 'after': progs2}, 'captures': captures}
        print(f'INFO carrier-to-frame interval: {progs} to {progs2} programs; actual captures: {captures}', flush=True)
        check(await ev("S47.motelTv.mode") == 'frame' and await ev("!!S47.motelTv.frame"), 'after 47 s: one frame')
        check(bool(successful) and all(not c['newProgramIds'] for c in successful),
              f'its shaders were made during the carrier: no new shader inside an actual successful frame capture ({len(successful)} captures, {captures})')
        await shot('t03_frame_on_tv')
        fr = await ev("S47.motelTv.frame.toDataURL('image/png')")
        open(f'{OUT}/t04_frame.png', 'wb').write(base64.b64decode(fr.split(',', 1)[1]))
        lum = await ev("(() => { const c = S47.motelTv.frame, d = c.getContext('2d').getImageData(0, 0, c.width, c.height).data; let s = 0; for (let i = 0; i < d.length; i += 4) s += d[i] + d[i + 1] + d[i + 2]; return s / (d.length / 4) / 3; })()")
        check(lum > 18, f'the lot can be made out (mean {lum:.0f} of 255)')
        nn = (await notes())[n4:]
        check(any('Motel office TV. One frame of the lot from above. My truck in it.' in n for n in nn) and await seen('ev:tv'), f'the finding as written ({nn[-1:]})')
        check('Nothing up there could hold a camera' in await toasts(), 'nothing up there could hold a camera')
        # the truck is in it: the same picture with the truck gone differs where it stood
        # (where it stands in the picture: 11 px to the metre, north to the right, west at the top)
        diff = await ev("""(() => { const T = S47.motelTv, tr = S47.world.saroTruck.group, vis = tr.visible, q = S47.world.motel.group.worldToLocal(tr.position.clone());
          const cx = 160 + (15 - q.z) * 11, cy = 120 + (q.x + 36) * 11, a = T['shoot']().getContext('2d').getImageData(0, 0, 320, 240).data.slice();
          tr.visible = false; const b = T['shoot']().getContext('2d').getImageData(0, 0, 320, 240).data; tr.visible = vis;
          let inside = 0, outside = 0, ni = 0, no = 0;
          for (let y = 0; y < 240; y++) for (let x = 0; x < 320; x++) { const i = (y * 320 + x) * 4, d = Math.abs(a[i] - b[i]) + Math.abs(a[i + 1] - b[i + 1]) + Math.abs(a[i + 2] - b[i + 2]);
            if (Math.abs(x - cx) < 40 && Math.abs(y - cy) < 40) { inside += d; ni++; } else { outside += d; no++; } }
          return [inside / Math.max(1, ni), outside / no, vis, cx, cy]; })()""")
        check(diff[2] and diff[0] > 8 and diff[0] > 5 * diff[1], f'the truck is in the frame where it stands (difference {diff[0]:.1f} there at ({diff[3]:.0f}, {diff[4]:.0f}), {diff[1]:.2f} elsewhere)')
        await tick(3)
        check(await ev("S47.motelTv.mode") == 'card' and not await ev("S47.motelTv.busy"), 'then the test card again')
        await tick(60)
        check(await ev("S47.motelTv.mode") == 'card', 'once in a night')
        mk = await ev("S47.mapMarks().map((m) => [m.kind, m.line, m.at ? m.at.spot : null])")
        check(['tv', 'B', 'office'] in mk, 'on the map at the motel\'s office, on B')

        # ---------- three on B ----------
        nb = await notes()
        check(not any('Three on one line' in n for n in nb), 'two marks on B: no line yet')
        await ev("S47.townSeen.add('mark:lights@' + Math.round(S47.game.clock - 1800))"); await tick(1.5)
        nn = await notes()
        check(any('Three on one line' in n and 'turn-off to SARO' in n and 'south-west, through STATION 01' in n for n in nn) and await seen('lineB'), 'three on B: drawn in on the map, as the map has it')
        m = await ev("S47.game.notebookExtras().map")
        open(f'{OUT}/m01_map.png', 'wb').write(base64.b64decode(m.split(',', 1)[1]))

        # ---------- save and Continue: none of it again ----------
        await ev(into); await ev("S47.saveNow('manual', 0)")
        await pg.wait_for_timeout(600)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.ninth.off = true; S47.hold = true; S47.curve.free = true; S47.tick(0.5); S47.setAbductions(false)")
        for _ in range(100):
            if await ev("S47.world.herds.length >= 1"): break
            await pg.wait_for_timeout(200)
        await tick(0.5)
        hh = await ev("S47.world.herds.map((h) => [h.still, h.missing, h.ring.visible])")
        check(await stage() == 'after' and all(x == [False, 4, True] for x in hh) and await ev("S47.pasture.passes") == 1, f'after Continue the eight graze and the ring is there, and the pass is kept ({hh})')
        check(await ev("S47.world.truckAt") == 'motel' and await ev("S47.world.saroTruck.group.visible"), 'after Continue the truck is seen on the motel\'s lot')
        await ev(into)
        n5 = len(await notes())
        await tick(20)
        check(await ev("S47.world.truckAt") == 'motel' and await ev("S47.motelTv.mode") == 'card' and len(await notes()) == n5, 'and in the office with the truck on the lot, the test card stays')

        # (the test's own readbacks of the screen's canvas make Chrome suggest willReadFrequently)
        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e and 'willReadFrequently' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks, 'shaders': shader_counts}, open(f'{OUT}/cowstv.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
