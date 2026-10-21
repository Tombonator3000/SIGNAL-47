# Out of the truck where it stands by something to walk to (Tom, 19 October: "går ikke å gå ut
# av bil ved kuene eller bilen som er forlatt ved veikanten eller ved radiomastene"): the truck
# stopped on the highway by the herd in the pasture (event 2), by the deputy's car on the
# shoulder north of Caliche (event 12) and among the masts north of SARO's fence. At each: Get
# out is offered (the prompt, and the class that shows the Use button on a phone), it is
# refused while the truck moves, E (or the touch button) gets out, the truck stays where it
# was left, the player stands on the ground by its door at the ground's height, walks with the
# real movement code (Player.update) to the thing (the cows stop the player, the patrol car's
# door and its text, a drive cabinet), the walk ends at the fences, the opening in the array's
# fence and the ends of the stretch, and the player gets back in at the truck (the crosshair
# finds it) and drives on. The cows are reached on chapter three's own drive to STATION 01,
# which has not built the diner's area the road is walked in: it is built behind the black. A
# save made by the herd, 1.5 km from the diner's lot, finds the truck and the player there.
# Usage: python3 tools/getout.py OUTDIR [WxH]
# Set S47_URL to test another build (the Pages build served over HTTP, say).
import asyncio, sys, os, math
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
X = 8000   # the road area's origin (World.ROAD_ORIGIN)
checks = []
def check(ok, what):
    checks.append(bool(ok)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

# Walk to each point in turn with Player.update: [ok, x, z, stuck x, stuck z, lowest, highest floor].
WALK = '''(pts) => { const p = S47.player; let lo = Infinity, hi = -Infinity;
  for (const [x, z] of pts) { let n = 0; const lim = Math.hypot(p.pos.x - x, p.pos.z - z) / p.speed * 30 * 1.6 + 120;
    while (Math.hypot(p.pos.x - x, p.pos.z - z) > 0.12) {
      p.yaw = Math.atan2(-(x - p.pos.x), -(z - p.pos.z)); p.update(1 / 30, 0, -1, false);
      if (n > 20) { lo = Math.min(lo, p.floorY); hi = Math.max(hi, p.floorY); }
      if (++n > lim) return [false, x, z, p.pos.x, p.pos.z, lo, hi]; } }
  return [true, p.pos.x, p.pos.z, 0, 0, lo, hi]; }'''
# Walk on towards a point for a while (into something that stops the player): where they end.
PUSH = '''([x, z, steps]) => { const p = S47.player;
  for (let n = 0; n < steps; n++) { p.yaw = Math.atan2(-(x - p.pos.x), -(z - p.pos.z)); p.update(1 / 30, 0, -1, false); }
  return [p.pos.x, p.pos.z]; }'''
STATE = """(() => { const w = S47.world, L = w.leg && w['legs'][w.leg], D = L && L.drive;
  return { area: w.area, leg: w.leg, driving: w.driving, at: w.truckAt, speed: D ? D.speed : 0,
    out: w.canGetOut(), cls: document.documentElement.classList.contains('can-out'),
    prompt: (document.querySelector('.hud .prompt.show') || {}).textContent || '' }; })()"""
# the truck being driven now, put on the spot (world x, z; heading 0 is north), standing still
PLACE = '''([x, z, h]) => { const w = S47.world, L = w['legs'][w.leg]; L.drive.place(new S47.three.Vector3(x, L.area.height ? L.area.height(x, z) : 0, z), h); }'''
TRUCK = '''(() => { const w = S47.world, t = w.driving ? w['legs'][w.leg].truck : (w.truckAt === 'diner' ? w.truck : w.truckAt === 'station01' ? w['parked'] : w.saroTruck);
  const g = t.group.position; return [g.x, g.z, t.group.rotation.y]; })()'''
# the player on the ground: [x, z, floorY, the ground there, walkable]
FOOT = '''(() => { const p = S47.player, w = S47.world; const g = w.area === 'diner' ? w['road'].height(p.pos.x, p.pos.z) : w.grounds.floorAt(p.pos.x, p.pos.z);
  return [p.pos.x, p.pos.z, p.floorY, g, p.walkable(p.pos.x, p.pos.z)]; })()'''

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
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_function('S47.started()', polling=200)
        ev = pg.evaluate
        async def tick(s, fps=30): await ev(f"S47.tick({s}, {fps})")
        async def shot(name, light=True):
            await ev(f"S47.flashlight.on = {'true' if light else 'false'}; S47.tick(0.3)")
            await pg.screenshot(path=f'{OUT}/{name}.png')
        async def label(i): return await ev(f"(() => {{ const it = S47.game.d.inter.get('{i}'); return it ? it.label() : 'MISSING'; }})()")
        async def use(i): await ev(f"S47.game.d.inter.get('{i}').use()")
        state = lambda: ev(STATE)
        truck = lambda: ev(TRUCK)
        foot = lambda: ev(FOOT)
        async def walk(pts): return await ev(WALK, pts)
        async def push(x, z, steps=200): return await ev(PUSH, [x, z, steps])
        async def place(x, z, yaw=0.0): await ev(f"S47.player.place({x},{z},{yaw}); S47.tick(0.05)")
        async def aim(x, y, z):
            await ev(f"(() => {{ const p = S47.player, e = p.floorY + p.eye; p.yaw = Math.atan2(-({x} - p.pos.x), -({z} - p.pos.z));"
                     f" p.pitch = Math.atan2({y} - e, Math.hypot({x} - p.pos.x, {z} - p.pos.z)); p.update(0.001, 0, 0, false); S47.tick(0.05); }})()")
        picked = lambda: ev("(() => { const it = S47.game.d.inter.update(S47.camera); return it ? it.id : null; })()")
        keys = lambda: ev("(S47.kitItems().find((k) => k.name === 'Truck keys') || {}).state")
        last_toast = lambda: ev("(() => { const t = [...document.querySelectorAll('.toasts .toast')]; return t.length ? t[t.length - 1].textContent : ''; })()")
        async def until(js, secs=60, step=0.25):
            """Tick (and wait real time: a trip or an area loads over the network) until js is true."""
            for _ in range(int(secs / step)):
                if await ev(js): return True
                await tick(step); await pg.wait_for_timeout(60)
            return await ev(js)
        async def get_in(use_id):
            await use(use_id)
            return await until("S47.world.driving && !S47.world['busy']")
        async def moving_refused(what):
            """Drive off a little: no Get out while the truck moves, and E does not get out."""
            await ev("S47.world.testInput = { steer: 0, throttle: 1 }"); await tick(1.5)
            st = await state()
            await pg.keyboard.press('e'); await tick(0.5)
            st2 = await state()
            await ev("S47.world.testInput = null")
            check(abs(st['speed']) > 0.6 and not st['out'] and not st['cls'] and 'Get out' not in st['prompt'] and st2['driving'],
                  f"{what}: moving ({st['speed']:.1f} m/s) Get out is not offered, and E does not get out")
        async def offered(what):
            st = await state()
            check(st['driving'] and abs(st['speed']) < 0.6 and st['out'] and st['cls'] and 'Get out' in st['prompt'],
                  f"{what}: stopped, Get out is offered (prompt \"{st['prompt']}\", the phone's Use button shown: {st['cls']})")
            return st
        async def got_out(what, spot, area, at, ground_tol=0.06):
            st = await state()
            tx, tz, _ = await truck()
            fx, fz, fy, gy, wk = await foot()
            moved = math.hypot(tx - spot[0], tz - spot[1])
            check(not st['driving'] and st['area'] == area and st['at'] == at and moved < 0.6,
                  f"{what}: out of the truck, which stays where it was left ({st['area']}, at {st['at']}, {moved:.2f} m from the spot)")
            near = math.hypot(fx - tx, fz - tz)
            check(wk and near < 3.6 and abs(fy - gy) < ground_tol, f"{what}: the player stands by its door ({near:.2f} m) on the ground (floor {fy:.3f}, ground {gy:.3f})")
            return fx, fz

        await ev("S47.hold = true; S47.tick(0.3); S47.setPicture('off')")
        # nothing else turning up meanwhile (each has its own test)
        quiet = "S47.cutouts.reset(); S47.cutouts['wait'] = 1e9; S47.glimpses.reset(); S47.glimpses['wait'] = 1e9; S47.nightEvents['wait'] = 1e9; S47.setAbductions(false); S47.world.trouble.random = false"

        # ================= the cows: chapter three's own drive to STATION 01 =================
        await ev("S47.jump('chapter3'); S47.tick(1.0)")
        await ev(quiet)
        await ev("S47.game.clock = 2 * 3600 + 50 * 60; S47.tick(0.2)")
        check(await label('truck') == 'Drive to STATION 01' and await ev("S47.world.diner === null"),
              f"cows: chapter three, the truck offers its drive ({await label('truck')}); the diner's area is not built")
        check(await get_in('truck'), 'cows: in the truck on the chapter\'s drive')
        await ev("S47.world.autopilot = 14")
        ok = await until(f"S47.world.leg === 'road' && S47.world['legs'].road.drive.pos.z > 490", secs=240)
        await ev("S47.world.autopilot = null")
        check(ok and await ev("S47.world.diner === null"), f"cows: driven down the highway into the road's area to the pasture, the diner still not built ({await ev('S47.world.leg')})")
        await moving_refused('cows')
        spot = (X - 2.4, 548.0)
        await ev(PLACE, [spot[0], spot[1], math.pi]); await tick(0.5)
        await offered('cows')
        await shot('c01_cows_from_cab')
        await pg.keyboard.press('e')
        out = await until("!S47.world.driving && !S47.world['busy'] && S47.world.area === 'diner'", secs=90)
        await tick(0.6)
        check(out and await ev("!!S47.world.diner && !!S47.world.town"), 'cows: E gets out; the diner\'s area (the road on foot) was built behind the black')
        fx, fz = await got_out('cows', spot, 'diner', 'diner')
        check(await ev("S47.placeName()") == 'The highway south' and await ev("S47.surface()") == 'concrete' and await keys() == 'Where I left it',
              f"cows: on the highway: \"{await ev('S47.placeName()')}\", footsteps {await ev('S47.surface()')}, the truck keys: {await keys()}")
        mid = await ev("(() => { const m = S47.world.herds.find((h) => h.area === 'road').middle(); return [m.x, m.z]; })()")
        d0 = math.hypot(fx - mid[0], fz - mid[1])
        # over to the fence, across the cattle guard and into the pasture
        r = await walk([[fx, spot[1] + 4.5], [X - 15, spot[1] + 4.5], [X - 15, 520.5], [X - 21, 520], [X - 26, 524], [X - 27, 531]])
        fx, fz, fy, gy, wk = await foot()
        check(r[0] and wk and abs(fy - gy) < 0.06, f"cows: walked to the fence, over the cattle guard and into the pasture ({fx - X:.1f}, {fz:.1f}; floor {fy:.3f}, ground {gy:.3f})" + ('' if r[0] else f' stuck at {r[3] - X:.1f}, {r[4]:.1f}'))
        check(await ev("S47.placeName()") == 'The pasture by the highway' and await ev("S47.surface()") == 'dirt',
              f"cows: \"{await ev('S47.placeName()')}\", footsteps {await ev('S47.surface()')}")
        cow = await ev("(() => { const v = S47.world.herds.find((h) => h.area === 'road').cowAt(0); return [v.x, v.z]; })()")
        x, z = await push(cow[0], cow[1] + 6, 150)
        dc = math.hypot(x - cow[0], z - cow[1]); d1 = math.hypot(x - mid[0], z - mid[1])
        check(1.2 < dc < 1.8 and d1 < d0 - 15, f"cows: walking into a cow, it stops the player {dc:.2f} m from it ({d0:.0f} m from the herd's middle at the truck, {d1:.0f} m now)")
        await aim(cow[0], fy + 1.1, cow[1])
        await shot('c02_among_the_cows')
        st0 = await ev("S47.pasture.stage")
        await ev("S47.pasture.now()"); await tick(2.0)
        check(await ev("S47.pasture.stage") == st0, f"cows: standing among them the herd does not change ({st0}: they are near, as from the road)")
        # what stops the player: the fence (from the highway and from the pasture), the end of the stretch
        await place(X - 15, 560); x, _ = await push(X - 30, 560)
        await place(X - 25, 560); x2, _ = await push(X - 10, 560)
        await place(X - 10, 500); _, z3 = await push(X - 10, 470)
        await place(X - 150, 560); x4, _ = await push(X - 200, 560)
        check(x - X > -20.7 and x2 - X < -21.3 and z3 > 490.2 and x4 - X > -159.8,
              f"cows: the right-of-way fence stops the player both ways but at the guard ({x - X:.2f}, {x2 - X:.2f}), and the walk ends north ({z3:.1f}) and west ({x4 - X:.1f})")
        # back to the truck: the crosshair finds it, and it offers the chapter's drive on
        tx, tz, th = await truck()
        await place(X - 15, tz + 4.5)
        r = await walk([[tx + 2.6, tz + 4.5], [tx + 2.6, tz + 0.5]])
        await aim(tx, 1.2 + (await foot())[3], tz)
        pk = await picked(); lb = await label('dinerTruck')
        check(r[0] and pk == 'dinerTruck' and lb == 'Drive to STATION 01', f"cows: back at the truck the crosshair finds it ({pk}): \"{lb}\"")
        # ---------- a save there: the truck and the player by the herd after Continue ----------
        fx, fz, *_ = await foot()
        await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(800)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)"); await ev(quiet)
        st = await state()
        tx2, tz2, _ = await truck()
        px, pz, *_ = await foot()
        check(st['area'] == 'diner' and st['at'] == 'diner' and math.hypot(tx2 - tx, tz2 - tz) < 0.3 and math.hypot(px - fx, pz - fz) < 0.3,
              f"cows: a save made there and Continue: the truck ({math.hypot(tx2 - tx, tz2 - tz):.2f} m) and the player ({math.hypot(px - fx, pz - fz):.2f} m) are by the herd again, 1.5 km from the diner's lot")
        check(await label('dinerTruck') == 'Drive to STATION 01', f"cows: and the truck still offers the chapter's drive ({await label('dinerTruck')})")
        check(await get_in('dinerTruck'), 'cows: back in the truck')
        z0 = (await truck())[1]
        await ev("S47.world.autopilot = 10"); await tick(3.0); await ev("S47.world.autopilot = null")
        st = await state()
        tx3, tz3, _ = await truck()
        check(st['driving'] and st['leg'] == 'road' and math.hypot(tx3 - tx, tz3 - tz) > 5, f"cows: and drives on towards STATION 01 ({math.hypot(tx3 - tx, tz3 - tz):.1f} m on the way)")
        await ev("S47.world.stopDriving(); S47.tick(0.2)")

        # ================= the masts: a drive of the player's own, north through the array =================
        await ev("S47.jump('chapter1'); S47.tick(1.0)")
        await ev(quiet)
        await ev("S47.game.clock = 2 * 3600 + 40 * 60; S47.tick(0.2)")
        check(await label('truck') == 'Drive', f"masts: chapter one, the truck offers a drive ({await label('truck')})")
        check(await get_in('truck'), 'masts: in the truck')
        await ev(PLACE, [-22.2, -64.0, 0.0]); await tick(0.3)
        await moving_refused('masts')
        spot = (-22.2, -64.0)
        await ev(PLACE, [spot[0], spot[1], 0.0]); await tick(0.5)
        await offered('masts')
        await shot('m01_masts_from_cab')
        # out by the phone's Use button this time (the same as E)
        await ev("document.querySelector('.tbtn.use').click()")
        await until("!S47.world.driving && !S47.world['busy']", secs=20)
        await tick(0.6)
        fx, fz = await got_out('masts', spot, 'saro', 'saro', 0.03)
        pn = await ev("S47.placeName()")
        check(pn.startswith('The array') and await ev("S47.surface()") == 'concrete' and await keys() == 'Where I left it',
              f"masts: on the highway in the array: \"{pn}\", footsteps {await ev('S47.surface()')}, the truck keys: {await keys()}")
        cab = await ev("(() => { const c = S47.world.field.cabinets.find((c) => c.a.id === 'S-02'); return [c.a.x, c.a.z, c.a.cab.x, c.a.cab.z, c.proxy.position.y]; })()")
        d0 = math.hypot(fx - cab[0], fz - cab[1])
        # up the highway to S-02's apron, off the asphalt, to its drive cabinet
        stand = await ev(f"""(() => {{ const p = S47.player, a = S47.world.field.cabinets.find((c) => c.a.id === 'S-02').a, fx = Math.sin(a.cab.ry), fz = Math.cos(a.cab.ry);
          const free = (x, z) => p.walkable(x, z) && !p.colliders.some((c) => x + p.radius > c.minX && x - p.radius < c.maxX && z + p.radius > c.minZ && z - p.radius < c.maxZ);
          for (let d = 1.0; d < 2.2; d += 0.05) {{ const x = a.cab.x + fx * d, z = a.cab.z + fz * d; if (free(x, z)) return [x, z]; }} return null; }})()""")
        r = await walk([[-23.0, -56.5], [-21.0, -57.2], [stand[0], -57.2], stand]) if stand else [False, 0, 0, fx, fz]
        fx, fz, fy, gy, wk = await foot()
        d1 = math.hypot(fx - cab[0], fz - cab[1])
        check(r[0] and wk and abs(fy + 0.6) < 0.02 and d1 < d0 - 4, f"masts: walked from the truck to the drive cabinet of S-02 ({d0:.1f} m from the antenna at the truck, {d1:.1f} m now; floor {fy:.3f})" + ('' if r[0] else f' stuck at {r[3]:.1f}, {r[4]:.1f}'))
        await aim(cab[2], cab[4], cab[3])
        pk = await picked(); lb = await label('field:S-02')
        check(pk == 'field:S-02' and lb and 'S-02' in lb, f"masts: the crosshair finds the cabinet ({pk}): \"{lb}\"")
        await use('field:S-02'); await tick(0.3)
        t = await last_toast()
        check('S-02' in t and '026' in t, f"masts: and it reads out: \"{t[:70]}\"")
        await shot('m02_cabinet_S-02')
        # the walk on the highway: from a row and back, not out through the fence's opening, not past the truck's way
        await place(-24.0, -30.0); _, z1 = await push(-24.0, -10.0)
        await place(-24.0, -190.0); _, z2 = await push(-24.0, -230.0)
        await place(-24.0, -60.0); x3, _ = await push(-40.0, -60.0)
        check(z1 < -20.1 and z2 > -200.8 and x3 > -28.4, f"masts: the walk on the highway ends at the fence ({z1:.2f}: on foot the gate is the way out), at the end of the truck's way ({z2:.1f}) and at its edge ({x3:.2f})")
        r = await walk([[-24.0, -100.0], [-24.0, -138.0], [-12.0, -138.0], [-1.0, -138.0]])
        check(r[0], f"masts: from the highway along row 2 to the spine ({r[1]:.1f}, {r[2]:.1f})" + ('' if r[0] else f' stuck at {r[3]:.1f}, {r[4]:.1f}'))
        # back to the truck: the crosshair finds it, and it offers a drive again
        tx, tz, th = await truck()
        r = await walk([[-1.0, -138.0], [-24.0, -138.0], [-24.0, tz - 4], [tx - 2.4, tz + 0.5]])
        await aim(tx, 0.6, tz)
        pk = await picked(); lb = await label('truck')
        check(r[0] and pk == 'truck' and lb == 'Drive', f"masts: back at the truck the crosshair finds it ({pk}): \"{lb}\"")
        check(await get_in('truck'), 'masts: back in the truck')
        await ev("S47.world.testInput = { steer: 0, throttle: 1 }"); await tick(2.5); await ev("S47.world.testInput = null")
        st = await state(); tx2, tz2, _ = await truck()
        check(st['driving'] and st['leg'] == 'saro' and math.hypot(tx2 - tx, tz2 - tz) > 3, f"masts: and drives on ({math.hypot(tx2 - tx, tz2 - tz):.1f} m)")
        await ev("S47.world.stopDriving(); S47.tick(0.2)")

        # ================= the deputy's car on the shoulder (event 12) =================
        await ev("S47.game.clock = 3 * 3600 + 20 * 60; S47.world.placeTruck('diner'); S47.world.enter('diner'); S47.world.placeAtDiner('arrive'); S47.tick(0.5)")
        check(await ev("S47.world.town.carOut"), "car: at 03:20, with the deputy out, his car stands on the shoulder north of town")
        check(await get_in('dinerTruck'), 'car: in the truck on the diner\'s lot')
        await ev(PLACE, [X + 2.4, 1740.0, 0.0]); await tick(0.3)
        await moving_refused('car')
        spot = (X + 2.4, 1703.0)
        await ev(PLACE, [spot[0], spot[1], 0.0]); await tick(0.5)
        await offered('car')
        await shot('d01_car_from_cab')
        await pg.keyboard.press('e')
        await until("!S47.world.driving && !S47.world['busy']", secs=20)
        await tick(0.6)
        fx, fz = await got_out('car', spot, 'diner', 'diner')
        check(await ev("S47.placeName()") == 'The highway north of Caliche' and await ev("S47.surface()") == 'concrete',
              f"car: \"{await ev('S47.placeName()')}\", footsteps {await ev('S47.surface()')}")
        car = [X + 6.9, 1690.0]
        d0 = math.hypot(fx - car[0], fz - car[1])
        r = await walk([[X + 4.6, 1698.0], [X + 4.8, 1691.0]])
        fx, fz, fy, gy, wk = await foot()
        d1 = math.hypot(fx - car[0], fz - car[1])
        check(r[0] and wk and abs(fy - gy) < 0.06 and d1 < d0 - 8, f"car: walked up to the open door ({d0:.1f} m from the car at the truck, {d1:.1f} m now; floor {fy:.3f}, ground {gy:.3f})")
        await aim(car[0], gy + 0.8, car[1])
        pk = await picked(); lb = await label('town:patrolCar')
        check(pk == 'town:patrolCar' and lb == 'Patrol car', f"car: the crosshair finds it ({pk}): \"{lb}\"")
        await use('town:patrolCar'); await tick(0.2)
        t = await last_toast()
        check('Nobody' in t, f"car: and the door hangs open: \"{t[:70]}\"")
        await shot('d02_the_car')
        x, _ = await push(X + 12, 1690.0, 120)
        await place(X + 10, 1660); _, z2 = await push(X + 10, 1600)
        await place(X + 10, 1700); x3, _ = await push(X + 40, 1700)
        check(x - X < 5.65 and z2 > 1650.2 and x3 - X < 16.3, f"car: the car is in the way ({x - X:.2f}), and the walk ends 40 m on ({z2:.1f}) and at the verge ({x3 - X:.1f})")
        tx, tz, th = await truck()
        r = await walk([[tx + 2.6, tz - 0.5]])
        await aim(tx, gy + 1.2, tz)
        pk = await picked(); lb = await label('dinerTruck')
        check(r[0] and pk == 'dinerTruck' and lb == 'Drive', f"car: back at the truck the crosshair finds it ({pk}): \"{lb}\"")
        check(await get_in('dinerTruck'), 'car: back in the truck')
        await ev("S47.world.testInput = { steer: 0, throttle: 1 }"); await tick(2.5); await ev("S47.world.testInput = null")
        st = await state(); tx2, tz2, _ = await truck()
        check(st['driving'] and st['leg'] == 'road' and math.hypot(tx2 - tx, tz2 - tz) > 3, f"car: and drives on ({math.hypot(tx2 - tx, tz2 - tz):.1f} m)")
        # elsewhere on the highway the road is still driven, not walked
        await ev(PLACE, [X - 2.4, 1200.0, math.pi]); await tick(0.5)
        st = await state()
        check(st['driving'] and not st['out'] and not st['cls'], 'car: stopped on the open highway between them, Get out is not offered (nothing there to walk to)')
        await ev("S47.world.stopDriving(); S47.tick(0.2)")

        bad = [e for e in errs if 'favicon' not in e]
        check(not bad, 'no console errors or warnings' + ('' if not bad else ': ' + '; '.join(bad[:4])))
        print(f'{sum(checks)} of {len(checks)} PASS')
        await b.close()
        sys.exit(0 if all(checks) else 1)

asyncio.run(main())
