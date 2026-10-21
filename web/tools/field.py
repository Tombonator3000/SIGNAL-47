# The antenna field, walked (world/Field.ts, world/arrayLayout.ts; FORSLAG.md, section 4): the walk
# gate in the array fence at z -20 keeps the field shut until it is opened; then, with the real
# movement code (Player.update), from the windows side through the gate, up the spine and out along
# every row and spur to all 27 drive cabinets, and back. At each cabinet the crosshair finds it
# (the real pick), it is used (the readout behind the glass, and a line), and a closer look says
# something. The highway through the array is walked too (World.canGetOut: the truck left on it,
# tools/getout.py), but not out through the fence's opening. Off the gravel is not walked, the
# bases and cabinets are in the way, the ground is flat under the whole array (the dishes stand
# on it), the glimpse spots behind S-11, S-14, S-19
# and S-22 still work from 40 m out and stand on the ground, and the field is dark at night
# without the flashlight. Screenshots on the way; draw calls looking north.
# Usage: python3 tools/field.py OUTDIR [WxH]
import asyncio, sys, json, os, math
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

# Walk to each point in turn with Player.update; [ok, x, z, ...] like tools/grounds.py, and the
# lowest and highest floor under the player on the way (after the eye has settled).
WALK = '''(pts) => { const p = S47.player; let lo = Infinity, hi = -Infinity;
  for (const [x, z] of pts) { let n = 0; const lim = Math.hypot(p.pos.x - x, p.pos.z - z) / p.speed * 30 * 1.6 + 120;
    while (Math.hypot(p.pos.x - x, p.pos.z - z) > 0.12) {
      p.yaw = Math.atan2(-(x - p.pos.x), -(z - p.pos.z)); p.update(1 / 30, 0, -1, false);
      if (n > 20) { lo = Math.min(lo, p.floorY); hi = Math.max(hi, p.floorY); }
      if (++n > lim) return [false, x, z, p.pos.x, p.pos.z, lo, hi]; } }
  return [true, p.pos.x, p.pos.z, 0, 0, lo, hi]; }'''

# The way to every cabinet along the tracks: up the spine, and out along each row and back; for
# each cabinet the point on its track, the point level with where one stands at it, and that stand
# (1 m or a little more in front of its door, wherever it can be stood).
ROUTE = '''(() => { const F = S47.world.field, p = S47.player, SX = -1;
  const free = (x, z) => p.walkable(x, z) && !p.colliders.some((c) => x + p.radius > c.minX && x - p.radius < c.maxX && z + p.radius > c.minZ && z - p.radius < c.maxZ);
  const stops = [];
  for (const cab of F.cabinets) { const a = cab.a, fx = Math.sin(a.cab.ry), fz = Math.cos(a.cab.ry);
    let S = null; for (let d = 1.0; d < 1.8 && !S; d += 0.05) { const x = a.cab.x + fx * d, z = a.cab.z + fz * d; if (free(x, z)) S = [x, z]; }
    const along = a.from === 'spine' ? [[SX, a.z], [S[0], a.z]] : a.spur ? [[a.x, a.from], [a.x, S[1]]] : [[S[0], a.from]];
    stops.push({ id: a.id, z: a.from === 'spine' ? a.z : a.from, x: a.x, along, stand: S, at: [a.cab.x, a.cab.z] }); }
  // in the order they are passed going north: the spine's own, then each row west and east of it
  const zs = [...new Set(stops.map((s) => s.z))].sort((u, v) => v - u), legs = [];
  for (const z of zs) {
    const here = stops.filter((s) => s.z === z), west = here.filter((s) => s.x < SX).sort((u, v) => v.x - u.x), east = here.filter((s) => s.x >= SX).sort((u, v) => u.x - v.x);
    for (const side of [west, east]) for (const s of side) legs.push({ id: s.id, go: [[SX, z], ...s.along, s.stand], back: [...s.along].reverse(), at: s.at });
    if (legs.length) legs[legs.length - 1].back.push([SX, z]);
  }
  return legs; })()'''

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': W, 'height': H})).new_page(); pg.set_default_timeout(300000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_function('S47.started()', polling=200)
        ev = pg.evaluate
        await ev("S47.hold = true; S47.jump('chapter1')"); await pg.wait_for_timeout(300)
        await ev("S47.hold = true; S47.game.d.ui.close(); S47.tick(1.0); S47.setPicture('off')")
        # nothing else turning up in the pictures (core/cutouts.ts, core/glimpses.ts have their own tests)
        await ev("S47.cutouts.reset(); S47.cutouts['wait'] = 1e9; S47.glimpses.reset(); S47.glimpses['wait'] = 1e9")
        await ev("S47.game.clock = 2 * 3600 + 40 * 60; S47.flashlight.have = true; S47.flashlight.on = false; S47.tick(0.2)")
        async def place(x, z, yaw=0.0, pitch=0.0): await ev(f"S47.player.place({x},{z},{yaw}); S47.player.pitch={pitch}; S47.tick(0.05)")
        async def walk(pts): return await ev(WALK, pts)
        async def push(x0, z0, x1, z1, steps=240):
            await place(x0, z0)
            return await ev(f'''(()=>{{const p=S47.player;
              for (let n=0; n<{steps}; n++) {{ p.yaw = Math.atan2(-({x1}-p.pos.x), -({z1}-p.pos.z)); p.update(1/30, 0, -1, false); }}
              return [p.pos.x, p.pos.z]; }})()''')
        async def shot(name, light=False, secs=0.5):
            await ev(f"S47.flashlight.on = {'true' if light else 'false'}; S47.tick({secs})")
            await pg.screenshot(path=f'{OUT}/{name}.png')
            return await ev("S47.renderer.info.render.calls")
        async def aim(x, y, z):
            await ev(f"(() => {{ const p = S47.player, e = p.floorY + p.eye; p.yaw = Math.atan2(-({x} - p.pos.x), -({z} - p.pos.z));"
                     f" p.pitch = Math.atan2({y} - e, Math.hypot({x} - p.pos.x, {z} - p.pos.z)); p.update(0.001, 0, 0, false); }})()")
        picked = lambda: ev("(() => { const it = S47.game.d.inter.update(S47.camera); return it ? it.id : null; })()")
        last_toast = lambda: ev("(() => { const t = [...document.querySelectorAll('.toasts .toast')]; return t.length ? t[t.length - 1].textContent : ''; })()")
        examine = lambda: ev("document.querySelector('.hud .examine').textContent")
        async def zoom_text():
            await ev("S47.input.zoomHeld = true; S47.tick(0.6)")
            t = await examine()
            await ev("S47.input.zoomHeld = false; S47.tick(0.4)")
            return t

        # ---------- the layout ----------
        lay = await ev("""(() => { const F = S47.world.field; return { n: F.cabinets.length, ids: F.cabinets.map((c) => c.a.id),
          zones: F.zones.length, dishes: S47.ext.dishes.map((d) => d.id + '@' + d.x + ',' + d.z),
          inter: F.cabinets.every((c) => !!S47.game.d.inter.get('field:' + c.a.id) && !!S47.game.d.inter.get('mast:' + c.a.id)) }; })()""")
        want = [f'S-{i:02d}' for i in range(1, 28)]
        check(lay['n'] == 27 and sorted(lay['ids']) == want and lay['inter'], f"a drive cabinet at each of the 27 antennas, each with its interaction and a pedestal to look at ({lay['n']})")
        same = await ev("S47.world.field.cabinets.every((c, i) => { const d = S47.ext.dishes[i]; return d.id === c.a.id && d.x === c.a.x && d.z === c.a.z; })")
        check(same, 'the field and the antennas are built from one layout (arrayLayout.ts)')

        # ---------- the gate, shut ----------
        await ev("S47.doors.set('gate', false, true); S47.tick(0.1)")
        x, z = await push(-1.0, -16.0, -1.0, -40.0)
        check(z > -19.6, f'the gate shut: the windows side ends at the fence ({x:.2f}, {z:.2f})')
        x, z = await push(-24.0, -16.0, -24.0, -40.0)
        check(z > -19.6, f'and the highway north through the fence is not walked either ({x:.2f}, {z:.2f})')
        await place(-1.0, -18.4)
        await aim(-1.0, 0.2, -20.0)
        lab = await ev("S47.game.d.inter.get('fieldGate').label()")
        check(await picked() == 'fieldGate' and lab == 'Open array gate', f'from the windows side the crosshair finds the gate: "{lab}"')
        t = await zoom_text()
        check('walk gate into the array' in t, f'a closer look at the gate: "{t[:60]}"')
        await shot('f01_gate_shut', light=True)
        await ev("S47.game.d.inter.get('fieldGate').use(); S47.tick(1.2)")
        check(await ev("S47.doors.isOpen('gate') && S47.doors.states().gate === true"), 'used: the gate swings open into the field (and is saved with the doors)')
        await place(-1.0, -12.0, 0.0, -0.08)
        calls = {'windows_north': await shot('f02_gate_open_from_windows', light=False)}

        # ---------- through it, to all 27 cabinets and back ----------
        await place(-1.0, -16.0)
        r = await walk([[-1.0, -19.0], [-1.0, -22.0]])
        check(r[0] and r[2] < -21.5, f'through the gate on foot, onto the spine ({r[1]:.2f}, {r[2]:.2f})')
        check(await ev("S47.placeName()") == 'The array', f'"The array" ({await ev("S47.placeName()")})')
        legs = await ev(ROUTE)
        check(len(legs) == 27 and all(l['go'][-1] for l in legs), f'a way along the tracks to every cabinet ({len(legs)})')
        reached, used, named, read, looked, lo, hi, bad = 0, 0, 0, 0, 0, 9.0, -9.0, []
        shots = {'S-05': 'f04_cabinet_S-05', 'S-11': 'f05_cabinet_S-11', 'S-20': 'f06_cabinet_S-20', 'S-27': 'f07_cabinet_S-27', 'S-03': 'f03_junction_S-03'}
        for leg in legs:
            cid = leg['id']
            r = await walk(leg['go'])
            if not r[0]: bad.append(f'{cid}: stuck going to {r[1]:.1f},{r[2]:.1f} at {r[3]:.2f},{r[4]:.2f}'); break
            reached += 1; lo = min(lo, r[5]); hi = max(hi, r[6])
            # the crosshair on the cabinet's door, the real pick
            cy = await ev(f"S47.world.field.cabinets.find((c) => c.a.id === '{cid}').proxy.position.y")
            await aim(leg['at'][0], cy, leg['at'][1])
            got = await picked()
            if got == f'field:{cid}':
                await ev(f"S47.game.d.inter.get('field:{cid}').use()")
                used += 1
                lit = await ev("(() => { const F = S47.world.field; return F.lit ? [F.lit.cab.a.id, S47.scene.getObjectByName('fieldReadout').visible, F.lit.lines()] : null; })()")
                toast = await last_toast()
                if cid == 'S-03':
                    if lit is None and 'motor bus' in toast: read += 1
                    else: bad.append(f'S-03 junction: {lit} "{toast}"')
                elif lit and lit[0] == cid and lit[1] and 'AZ 026.0' in lit[2]['rows'][0] and lit[2]['warn'] == 'NO CONTROL COMMAND' and f'{cid}. Encoder az 026.0, el 32.0' in toast and 'Commands received: 0' in toast:
                    read += 1
                else: bad.append(f'{cid}: readout {lit} "{toast}"')
            else: bad.append(f'{cid}: the crosshair found {got}')
            if await ev("S47.placeName()") == f'The array, {cid}': named += 1
            if cid in ('S-05', 'S-11', 'S-14', 'S-22', 'S-27', 'S-03'):
                t = await zoom_text()
                if (cid == 'S-03' and 'cable junction' in t) or f'Drive cabinet {cid}' in t: looked += 1
                else: bad.append(f'{cid}: closer look "{t}"')
            if cid in shots:
                await shot(shots[cid], light=True)
            r = await walk(leg['back'])
            if not r[0]: bad.append(f'{cid}: stuck on the way back at {r[3]:.2f},{r[4]:.2f}'); break
            lo = min(lo, r[5]); hi = max(hi, r[6])
        check(reached == 27, f'walked to all 27 cabinets along the tracks ({reached})' + ('' if not bad else ': ' + '; '.join(bad[:3])))
        check(used == 27 and read == 27, f'at each the crosshair finds the cabinet and it reads out (az 026.0, el 32.0, no command; S-03 points to its motor bus) ({used}, {read})')
        check(named == 27, f'the place is named for the antenna there ({named})')
        check(looked == 6, f'a closer look at the cabinets says something ({looked} of 6 tried)')
        check(-0.62 < lo and hi < -0.58, f'the ground under the player all the way: {lo:.3f} to {hi:.3f}')
        r = await walk([[-1.0, -24.0], [-1.0, -19.0], [-1.0, -16.0]])
        check(r[0], 'and back down the spine and out through the gate')

        # the pedestal, from its apron
        await place(-1.0, -42.0)
        await walk([[-1.0, -88.0], [1.0, -88.0], [3.0, -92.0]])
        await aim(6.0, 2.0, -88.0)
        t = await zoom_text()
        check('S-04. It points where the other twenty-six point: az 026.0, el 32.0' in t, f'a closer look at an antenna: "{t[:80]}"')

        # ---------- what stops the player ----------
        stops = []
        for (a, b2, ok, what) in [
            ((-1.0, -100.0), (-12.0, -100.0), lambda x, z: x > -2.4, 'off the spine into the desert'),
            ((40.0, -74.0), (40.0, -60.0), lambda x, z: z < -72.5, 'off a row'),
            ((-24.0, -30.0), (-24.0, -10.0), lambda x, z: z < -20.1, 'south along the highway out through the opening in the fence'),
            ((-24.0, -190.0), (-24.0, -230.0), lambda x, z: z > -200.8, 'north along the highway past the end of the truck\'s way'),
            ((-24.0, -100.0), (-40.0, -100.0), lambda x, z: x > -28.4, 'off the highway into the desert'),
            ((-10.0, -195.5), (-10.0, -210.0), lambda x, z: z > -196.9, 'into the base of S-11 from its row'),
            ((44.0, -91.0), (47.0, -91.0), lambda x, z: x < 45.95, 'through the cabinet of S-05')]:
            x, z = await push(a[0], a[1], b2[0], b2[1])
            if not ok(x, z): stops.append(f'{what} (ended at {x:.2f},{z:.2f})')
        check(not stops, 'the desert between the tracks, the ends of the highway, the bases and the cabinets stop the player' + ('' if not stops else ': ' + '; '.join(stops)))
        # the highway through the array is walked (the truck can be left on it, World.canGetOut): from a row along it to the next
        await place(-24.0, -195.0)
        r = await walk([[-24.0, -150.0], [-24.0, -138.0], [-12.0, -138.0]])
        check(r[0], f'from where row 3 crosses the highway, along it to row 2 and onto the row ({r[1]:.1f}, {r[2]:.1f})' + ('' if r[0] else f' stuck at {r[3]:.1f}, {r[4]:.1f}'))
        # the gate shut behind the player: the field cannot be left until it is opened again
        await place(-1.0, -24.0)
        await ev("S47.game.d.inter.get('fieldGate').use(); S47.tick(1.2)")
        x, z = await push(-1.0, -24.0, -1.0, -10.0)
        check(not await ev("S47.doors.isOpen('gate')") and z < -20.3, f'shut from inside, the gate keeps the player in ({z:.2f})')
        await place(-1.0, -20.6, math.pi)
        await aim(-1.0, 0.2, -20.0)
        check(await picked() == 'fieldGate', 'and it can be opened again from the field side')
        await ev("S47.game.d.inter.get('fieldGate').use(); S47.tick(1.2)")
        await place(-1.0, -20.0)
        await ev("S47.game.d.inter.get('fieldGate').use(); S47.tick(1.2)")
        check(await ev("S47.doors.isOpen('gate')"), 'standing in the gateway it will not shut')

        # ---------- the ground and the antennas on it ----------
        g = await ev("""(() => { const rc = S47.game.d.inter.ray, v = S47.camera.position, ground = S47.ext.group.children.find((o) => o.geometry && o.geometry.type === 'RingGeometry');
          const at = (x, z) => { rc.set(v.clone().set(x, 60, z), v.clone().set(0, -1, 0)); rc.far = 120; const h = rc.intersectObject(ground, false)[0]; return h ? h.point.y : null; };
          const pts = []; for (const c of S47.world.field.cabinets) { const a = c.a; pts.push([a.x, a.z], [a.cab.x, a.cab.z], [a.apron.minX, a.apron.minZ], [a.apron.maxX, a.apron.maxZ]); }
          const ys = pts.map(([x, z]) => at(x, z)); return [Math.min(...ys), Math.max(...ys), ys.filter((y) => y === null).length, S47.world.grounds.floorAt(-60, -420)]; })()""")
        check(g[2] == 0 and -0.63 < g[0] and g[1] < -0.61, f'the desert is flat under the whole array, out to S-25, S-26 and S-27 ({g[0]:.3f} to {g[1]:.3f}): the bases stand on it')
        check(abs(g[3] + 0.6) < 0.01, f'and the floor there is the ground ({g[3]:.2f})')
        fl = await ev("[[-1, -19.8], [-1, -20.6], [-1, -25], [-1, -38], [-1, -60], [9.5, -36.5], [-48, -70], [-230, -310], [210, -320], [-60, -426]].map(([x, z]) => S47.world.grounds.floorAt(x, z))")
        check(all(abs(y + 0.6) < 0.005 for y in fl), f'the floor is the ground from the gateway to the far aprons ({min(fl):.2f} to {max(fl):.2f})')

        # ---------- them, behind the antennas ----------
        await place(-2.0, -24.0, -0.75)
        await ev("S47.player.update(0.001, 0, 0, false)")
        ok = await ev("S47.glimpses.now(S47.glimpseIn(), 'S-11')")
        fy = await ev("S47.scene.getObjectByName('glimpse').position.y")
        check(ok and abs(fy + 0.6) < 0.02, f'from the gate the figure can still stand behind S-11, now on the ground (y {fy:.2f}; it stood at 0 before)')
        await ev("S47.glimpses.reset(); S47.glimpses['wait'] = 1e9")
        near = []
        for sid, (sx, sz) in {'S-11': (-12, -208), 'S-14': (43, -238), 'S-19': (-42, -308), 'S-22': (8, -368)}.items():
            # the nearest a player can stand to it now
            d = await ev(f"""(() => {{ let best = 1e9; for (const z of S47.world.field.zones) {{ if (z.enabled === false) continue;
              const x = Math.max(z.minX + .27, Math.min(z.maxX - .27, {sx})), q = Math.max(z.minZ + .27, Math.min(z.maxZ - .27, {sz})); best = Math.min(best, Math.hypot(x - {sx}, q - {sz})); }} return best; }})()""")
            near.append(f'{sid} {d:.1f} m')
        print('closest a player can stand to each glimpse spot:', ', '.join(near))
        await place(-1.0, -186.0, 0.4)
        await ev("S47.player.update(0.001, 0, 0, false)")
        check(not await ev("S47.glimpses.now(S47.glimpseIn(), 'S-11')"), 'walked up to within 40 m of S-11, it is never put there (the 40 m rule holds in the field)')
        await place(-1.0, -255.0, 0.35)
        await ev("S47.player.update(0.001, 0, 0, false)")
        ok = await ev("S47.glimpses.now(S47.glimpseIn(), 'S-19')")
        d = await ev("S47.scene.getObjectByName('glimpse').position.distanceTo(S47.camera.position)")
        check(ok and d > 40, f'out in the field, behind S-19 from {d:.0f} m')
        await shot('f08_glimpse_S-19', light=False)
        await ev("S47.glimpses.reset(); S47.glimpses['wait'] = 1e9")

        # ---------- dark at night; the flashlight ----------
        await place(-1.0, -250.0, 0.0, -0.25)
        await shot('f09_dark_no_light', light=False)
        await shot('f10_dark_flashlight', light=True)
        lum = await ev("""(async () => { const out = [];
          for (const on of [false, true]) { S47.flashlight.on = on; S47.tick(0.4); const c = S47.renderer.domElement;
            const k = document.createElement('canvas'); k.width = 64; k.height = 40; const g = k.getContext('2d'); S47.tick(0.05);
            g.drawImage(c, c.width * .3, c.height * .55, c.width * .4, c.height * .35, 0, 0, 64, 40);
            const d = g.getImageData(0, 0, 64, 40).data; let s = 0; for (let i = 0; i < d.length; i += 4) s += d[i] + d[i + 1] + d[i + 2]; out.push(s / (64 * 40 * 3)); }
          return out; })()""")
        check(lum[0] < 40 and lum[1] > lum[0] * 1.8, f'out in the field the gravel ahead is dark at night ({lum[0]:.0f}) and shows in the flashlight ({lum[1]:.0f})')

        # ---------- before the antennas turned (the prologue) ----------
        await ev("S47.jump('residual'); S47.tick(0.5); S47.game.d.ui.close(); S47.tick(0.2)")
        await ev("S47.doors.set('gate', true, true)")
        await place(44.0, -84.9)
        await ev("S47.game.d.inter.get('field:S-05').use()")
        t = await last_toast()
        check("on the console's track" in t, f"before 02:15 the cabinets read the console's track: {t}")
        await ev("S47.jump('chapter1'); S47.tick(1.0); S47.game.d.ui.close()")
        await ev("S47.cutouts.reset(); S47.cutouts['wait'] = 1e9; S47.glimpses.reset(); S47.glimpses['wait'] = 1e9; S47.game.clock = 2 * 3600 + 40 * 60")

        # ---------- the cost ----------
        await ev("S47.doors.set('gate', true, true)")
        await place(3.0, -15.0, 0.1, 0.02); calls['north_from_walk'] = await shot('f11_north_from_the_walk')
        await place(-1.0, -40.0, 0.0, -0.05); calls['spine'] = await shot('f12_spine_north', light=True)
        await place(-1.0, -315.0, 1.57, -0.05); calls['far_row'] = await shot('f13_far_row_west', light=True)
        await place(0.0, -2.0, 0.0, 0.0); calls['control_room'] = await shot('f14_from_the_console')
        print('draw calls:', calls)
        check(max(calls.values()) <= 300, f'draw calls at most {max(calls.values())} on High looking out over the field (budget about 300)')

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks, 'calls': calls}, open(f'{OUT}/field.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
