# The Mesa Diner (world/Diner.ts, by Codex) in the game, before its chapter is written: the
# area loads on its own, the player walks it with the real movement code from the truck to
# a stool and to the payphone, every hit box can be aimed at from somewhere a player can
# stand, the sign and the dawn can be switched, the draw calls stay in budget, and the
# player can go back to SARO. Screenshots from standing height, also of the payphone and
# the newspaper clipping (Codex's preview showed those only in part).
# Usage: python3 tools/diner.py OUTDIR [WxH]
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

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
        # let the first load finish fetching its sounds: a reload in the middle aborts them,
        # and the browser logs those aborted fetches as warnings (the other tests wait too)
        try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
        except Exception: pass
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        ev = pg.evaluate
        async def tick(s): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def calls(): return await ev("S47.renderer.info.render.calls")
        await ev("S47.hold = true; S47.tick(0.3)")
        check(await ev("S47.art().later.includes('dinerBooth') && !S47.art().loaded.includes('dinerWall')"), 'the diner\'s pictures wait until the diner is built')

        # ---------- in ----------
        await ev("window.__go = S47.world.goDiner('arrive')")
        for _ in range(80):
            await tick(0.1); await pg.wait_for_timeout(150)          # the module and its pictures load in real time
            if await ev("S47.world.area === 'diner'"): break
        await tick(0.5); await pg.wait_for_timeout(700)
        check(await ev("S47.world.area") == 'diner' and await ev("S47.world.diner.group.visible && !S47.ext.group.visible"), 'the diner is its own area; SARO is hidden')
        art = await ev("S47.art()")
        check(all(i in art['loaded'] for i in ['dinerSign', 'dinerMenu', 'dinerCounter', 'dinerFloor', 'photo1947', 'dinerBooth', 'dinerWall']), 'the diner brought its seven pictures (round 6 and 8, and the 1947 master from round 10)')
        a = await ev("S47.world.diner.anchors")
        check(await ev("Math.hypot(S47.player.pos.x - S47.world.diner.anchors.arrive.x, S47.player.pos.z - S47.world.diner.anchors.arrive.z)") < 0.2 and abs(await ev("S47.player.floorY")) < 0.01,
              'the player stands by the truck, on the ground (y 0)')
        await shot('g01_arrival'); c = await calls(); print(f'draw calls on arrival: {c}', flush=True)
        check(c < 120, f'draw calls on arrival under 120 ({c})')

        # walk with the real movement code (zones, colliders)
        async def walk(points):
            return await ev(f'''(()=>{{const pts={json.dumps(points)}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.15) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > 2400) return [false, p.pos.x, p.pos.z]; }} }}
              return [true, p.pos.x, p.pos.z]; }})()''')
        A = lambda k: [a[k]['x'], a[k]['z']]
        r = await walk([A('outside')])
        check(r[0], f'from the truck to the door ({r[1]:.2f}, {r[2]:.2f})')
        await ev("S47.world.diner.objs.door.rotation.y = Math.PI / 2; S47.tick(0.1)")   # the chapter will open it
        r = await walk([A('inside'), A('stool')])
        check(r[0], 'through the door and along the counter to the stool next to the driver')
        check(await ev("S47.world.indoors(S47.player.pos)"), 'inside counts as indoors (room tone, steps on the tiles)')
        o = await ev("[S47.world.diner.anchors.inside.x - 0.75, S47.world.diner.anchors.inside.z]")   # the diner's origin
        r = await walk([A('inside'), [o[0] - 1.2, o[1]], [o[0] - 1.2, o[1] + 7.9], A('phone')])
        check(r[0], f'from the door down the aisle between the counter and the booths to the payphone ({r[1]:.2f}, {r[2]:.2f})')
        r = await walk([[o[0] - 1.2, o[1] + 7.9], [o[0] - 1.2, o[1]], A('inside'), A('outside'), A('arrive')])
        check(r[0], 'and back out to the truck')

        # ---------- every hit box from a place a player can stand ----------
        # like Interaction: a ray from the eye through the middle of the screen, against the hit boxes
        # for each hit box: the spot Codex's preview used (local coordinates), the anchors, or
        # failing those the nearest place 0.9 to 2.3 m away where the player can stand
        SPOTS = {'door': (3.1, 0), 'counter': (-2.8, 0), 'coffee': (-2.8, 0), 'waitress': (-2.8, 0), 'driver': (-2.8, 0), 'menu': (-3.72, -2.4),
                 'clipping': (0.75, 0), 'payphone': (0.4, 8.05), 'jukebox': (-1.1, -6.8), 'window': (0.5, -1.15), 'booth': (-1.0, 3.05), 'sign': (14.2, -10), 'rig': (9.0, -11.3)}
        res = await ev('''(()=>{ const d=S47.world.diner, p=S47.player, out={};
          for (const id of Object.keys(d.proxies)) {
            const g=d.proxies[id]; g.updateMatrixWorld(true);
            const c=S47.camera.position.clone().setFromMatrixPosition((g.children[0]||g).matrixWorld);
            let best=null;
            for (let rad=0.9; rad<=2.3 && !best; rad+=0.35) for (let k=0;k<24 && !best;k++) {
              const ang=k/24*Math.PI*2, x=c.x+Math.cos(ang)*rad, z=c.z+Math.sin(ang)*rad;
              if (p.walkable(x, z)) best=[x,z];
            }
            out[id]=best;
          }
          return out; })()''')
        ok_pick = []
        for pid, spot in res.items():
            if pid in SPOTS: spot = [o[0] + SPOTS[pid][0], o[1] + SPOTS[pid][1]]
            if not spot: ok_pick.append((pid, False, None)); continue
            x, z = spot
            hit = await ev(f'''(()=>{{ const d=S47.world.diner, p=S47.player, cam=S47.camera; const g=d.proxies['{pid}'];
              const c=new cam.position.constructor().setFromMatrixPosition((g.children[0]||g).matrixWorld);
              p.place({x}, {z}, Math.atan2(-(c.x-({x})), -(c.z-({z})))); p.pitch=Math.atan2(c.y-(p.floorY+p.eye), Math.hypot(c.x-({x}), c.z-({z}))); S47.tick(0.02);
              const rc = S47.game.d.inter.ray;
              rc.setFromCamera({{x:0,y:0}}, cam); rc.far = 2.4;
              const hits = rc.intersectObjects(Object.values(d.proxies), false);
              return hits.length ? [Object.keys(d.proxies).find(k => d.proxies[k] === hits[0].object || d.proxies[k].children.includes(hits[0].object)), hits[0].distance] : null; }})()''')
            stand = await ev(f"S47.player.walkable({x}, {z})")
            ok_pick.append((pid, bool(stand and hit and hit[0] == pid), hit))
        bad = [x for x in ok_pick if not x[1]]
        check(not bad and len(ok_pick) == 13, f'all 13 hit boxes can be aimed at from a place the player can stand, within the usual 2.4 m ({bad or "all"})')

        # ---------- standing-height views ----------
        async def view(name, x, z, tx, ty, tz):
            await ev(f"(()=>{{const p=S47.player; p.place({x},{z},0); S47.tick(0.05); p.yaw=Math.atan2(-(({tx})-({x})), -(({tz})-({z}))); p.pitch=Math.atan2(({ty})-(p.floorY+p.eye), Math.hypot(({tx})-({x}),({tz})-({z}))); for(let i=0;i<4;i++) S47.tick(0.1);}})()")
            await pg.wait_for_timeout(300); await shot(name)
            return await calls()
        def at(pid):
            return ev(f"(()=>{{const g=S47.world.diner.proxies['{pid}']; const v=new S47.camera.position.constructor().setFromMatrixPosition((g.children[0]||g).matrixWorld); return [v.x,v.y,v.z];}})()")
        worst = 0
        out = A('outside'); ins = A('inside'); st = A('stool'); ph = A('phone')
        door = await at('door'); counter = await at('counter'); phone = await at('payphone'); clip = await at('clipping'); menu = await at('menu'); sign = await at('sign'); waitress = await at('waitress')
        worst = max(worst, await view('g02_outside', out[0] + 3, out[1] + 2.5, door[0], 1.4, door[2]))
        worst = max(worst, await view('g03_inside', ins[0], ins[1], counter[0], 1.0, counter[2] - 3))
        worst = max(worst, await view('g04_stool', st[0], st[1], waitress[0], 1.5, waitress[2]))
        worst = max(worst, await view('g05_menu', st[0], st[1], menu[0], menu[1], menu[2]))
        worst = max(worst, await view('g06_phone', ph[0], ph[1], phone[0], phone[1], phone[2]))
        r = await ev(f"(()=>{{ const p=S47.player; const x={phone[0]}, z={phone[2]}; let best=null; for(let d=1.4; d<=2.2 && !best; d+=0.2) for(let k=0;k<16 && !best;k++){{ const ax=x+Math.cos(k/16*Math.PI*2)*d, az=z+Math.sin(k/16*Math.PI*2)*d; if(p.walkable(ax,az)) best=[ax,az]; }} return best; }})()")
        if r: worst = max(worst, await view('g06b_phone_whole', r[0], r[1], phone[0], phone[1] - 0.2, phone[2]))
        # the clipping hangs on the inside of the front wall by the door: read it from inside, 1.2 to 1.6 m off
        r = await ev(f"(()=>{{ const p=S47.player; for (const d of [1.2, 1.4, 1.6]) {{ const x={clip[0]}-d, z={clip[2]}; if (p.walkable(x, z) && S47.world.indoors({{x, z}})) return [x, z]; }} return null; }})()")
        check(bool(r), 'the clipping can be read from inside, a step away')
        if r: worst = max(worst, await view('g07_clipping', r[0], r[1], clip[0], clip[1], clip[2]))
        worst = max(worst, await view('g08_sign', o[0] + 6.5, o[1] - 3, sign[0], sign[1] + 0.6, sign[2]))
        await ev("S47.world.diner.setDawn(1); S47.world.diner.setSignLit(false); S47.tick(0.2)")
        worst = max(worst, await view('g09_dawn_unlit', o[0] + 6.5, o[1] - 3, sign[0], sign[1] + 0.6, sign[2]))
        await ev("S47.world.diner.setDawn(0); S47.world.diner.setSignLit(true); S47.tick(0.2)")
        print(f'worst draw calls in the views: {worst}', flush=True)
        check(worst < 120, f'draw calls under 120 in every view ({worst})')

        # ---------- back ----------
        await ev("S47.world.enter('saro'); S47.player.place(1.55, 3.7, 0); S47.tick(0.3)")
        check(await ev("S47.world.area === 'saro' && !S47.world.diner.group.visible && S47.ext.group.visible"), 'back to SARO, the diner hidden')
        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        check(not errs, 'no console errors or warnings')
        json.dump(checks, open(f'{OUT}/checks-diner.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
    sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
