# The truck's exact place in a save (World.truckSave, GameState.truckPose): left off the pad on
# SARO's grounds, on the diner's lot where the autopilot stopped it, and saved on a drive (then it
# goes back where the drive began, with the player). Save, reload, Continue: the truck stands as
# it was left, to the centimetre and the heading, its collider and its handle with it. A save
# without the pose (one from before 18 October 2026) puts it on its place as before.
# Usage: python3 tools/truckpose.py OUTDIR [WxH]
import asyncio, sys, os, json, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
results = []
def check(ok, msg):
    results.append((bool(ok), msg)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

POSE = """(() => { const w = S47.world, t = w.truckAt === 'diner' ? w['truck'] : w.truckAt === 'station01' ? w['parked'] : w.saroTruck;
  if (!t) return null; const g = t.group;
  const col = w.truckAt === 'diner' ? w['dinerCol'] : w.truckAt === 'station01' ? w.site.truckCollider : S47.yard.truckCol;
  return { at: w.truckAt, area: w.area, x: g.position.x, y: g.position.y, z: g.position.z, h: g.rotation.y, p: g.rotation.x, r: g.rotation.z,
    col: col ? [(col.minX + col.maxX) / 2, (col.minZ + col.maxZ) / 2] : null }; })()"""

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        ctx = await b.new_context(viewport={'width': W, 'height': H})
        pg = await ctx.new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.evaluate("localStorage.clear()")
        await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        ev = pg.evaluate
        async def tick(s, fps=30): await ev(f"S47.tick({s}, {fps})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def label(i): return await ev(f"(() => {{ const it = S47.game.d.inter.get('{i}'); return it ? it.label() : 'MISSING'; }})()")
        pose = lambda: ev(POSE)
        async def get_in(i):
            await ev(f"S47.game.d.inter.get('{i}').use()")
            for _ in range(240):
                await tick(0.25)
                if await ev("S47.world.driving"): break
                await pg.wait_for_timeout(100)
            await tick(0.5)
        async def stop():
            await ev("S47.world.autopilot = null")
            for _ in range(80):
                await tick(0.25)
                if abs(await ev("(() => { const w = S47.world, L = w['legs'][w.leg]; return L ? L.drive.speed : 0; })()")) < 0.3: break
            await tick(0.5)
        async def get_out():
            await ev("S47.world.getOutHere()")
            for _ in range(24):
                await tick(0.25)
                if not await ev("S47.world.driving"): break
            await tick(0.5)
        async def save_continue(strip=False):
            if strip:   # a save as it was before the pose: without the field
                await ev("(() => { const s = S47.saves, o = s.save.bind(s); s.save = (c, k, sl, st, card) => { delete st.truckPose; return o(c, k, sl, st, card); }; })()")
            await ev("S47.saveNow('manual', 0)")
            await pg.wait_for_timeout(800)
            await pg.reload()
            await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
            await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
            await ev("S47.hold = true; S47.setPicture('off'); S47.tick(0.5)")
        def same(a, c, tol=0.01):
            return a and c and a['at'] == c['at'] and all(abs(a[k] - c[k]) < tol for k in ['x', 'y', 'z']) and abs(a['h'] - c['h']) < 1e-4 \
                and abs(a['p'] - c['p']) < 1e-4 and abs(a['r'] - c['r']) < 1e-4
        def fmt(a): return f"{a['at']} ({a['x']:.2f}, {a['y']:.2f}, {a['z']:.2f}) h {a['h']:.3f}" if a else 'none'

        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        await ev("S47.hold = true; S47.tick(0.3); S47.setPicture('off')")
        await ev("S47.jump('chapter1'); S47.tick(1.0)")
        pad = await pose()

        # ---------- off the pad at SARO ----------
        await get_in('truck')
        await ev("S47.world['trip'].to = 'diner'; S47.world.autopilot = 5")
        await tick(6.0)
        await stop()
        await get_out()
        left = await pose()
        moved = math.hypot(left['x'] - pad['x'], left['z'] - pad['z'])
        check(left['at'] == 'saro' and moved > 6, f"left on SARO's grounds, {moved:.1f} m off the pad: {fmt(left)}")
        saved = await ev("S47.truckSave()")
        check(saved and saved['at'] == 'saro' and abs(saved['pose']['x'] - left['x']) < 1e-6, f"a save finds it there ({saved and saved['area']})")
        await save_continue()
        back = await pose()
        await shot('saro_continued')
        check(same(left, back), f"after Continue it stands where it was left: {fmt(back)}")
        check(back['col'] and math.hypot(back['col'][0] - back['x'], back['col'][1] - back['z']) < 0.05, f"its collider with it ({back['col']})")
        near = await ev("(() => { const p = S47.player.pos, g = S47.world.saroTruck.group.position; return Math.hypot(p.x - g.x, p.z - g.z); })()")
        check(near < 4 and await label('truck') == 'Drive', f"the player by its door ({near:.2f} m) and it offers a drive ({await label('truck')})")
        px = await ev("(() => { const v = S47.yard.objs.truck.getWorldPosition(new S47.three.Vector3()); return [v.x, v.z]; })()")
        check(math.hypot(px[0] - back['x'], px[1] - back['z']) < 0.05, 'its handle for Use with it')

        # ---------- an old save, without the pose: on the pad as before ----------
        await save_continue(strip=True)
        old = await pose()
        check(old['at'] == 'saro' and math.hypot(old['x'] - 8.9, old['z'] - 8.0) < 0.01 and abs(old['h'] - math.pi) < 1e-4, f"a save without the pose puts it on its pad: {fmt(old)}")

        # ---------- on the diner's lot ----------
        await get_in('truck')
        await ev("S47.world['trip'].to = 'diner'; S47.world.autopilot = 14")
        for _ in range(1200):
            if not await ev("S47.world.driving"): break
            await tick(0.5)
        await tick(1.0)
        lot = await pose()
        anchor = await ev("(() => { const a = S47.world.diner.anchors.truckPark; return [a.x, a.z, a.yaw]; })()")
        off = math.hypot(lot['x'] - anchor[0], lot['z'] - anchor[1])
        check(lot['at'] == 'diner' and lot['area'] == 'diner', f"driven to the diner and left on its lot, {off:.2f} m from where a save used to put it: {fmt(lot)}")
        await save_continue()
        back = await pose()
        await shot('diner_continued')
        check(same(lot, back), f"after Continue on the lot as it was left: {fmt(back)}")
        check(await label('dinerTruck') == 'Drive', f"and it offers a drive ({await label('dinerTruck')})")

        # ---------- saved on a drive: where the drive began ----------
        await get_in('dinerTruck')
        await ev("S47.world['trip'].to = 'saro'; S47.world.autopilot = 6")
        await tick(5.0)
        moving = await ev("S47.world.driving")
        st = await ev("(() => { const t = S47.truckSave(); return t ? { at: t.at, area: t.area, x: t.pose.x, z: t.pose.z } : null; })()")
        check(moving and st and st['at'] == 'diner' and st['area'] == 'diner' and abs(st['x'] - lot['x']) < 0.01, f"on a drive a save finds the truck where the drive began ({st})")
        await save_continue()
        back = await pose()
        check(same(lot, back) and back['area'] == 'diner' and not await ev("S47.world.driving"), f"after Continue it is on the lot again, and the player at the diner: {fmt(back)}")

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        await b.close()
    ok = sum(1 for r in results if r[0])
    json.dump([{'ok': r[0], 'msg': r[1]} for r in results], open(f'{OUT}/truckpose.json', 'w'), indent=1)
    print(f"{ok} of {len(results)} PASS")
    sys.exit(0 if ok == len(results) else 1)

asyncio.run(main())
