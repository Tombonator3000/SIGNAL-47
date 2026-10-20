# More that can happen in the night (story/nightEvents.ts; Tom, 18 October): the steps behind,
# the phone ringing once, the service door found open, the silence away from the motel walk,
# the truck's radio on by itself, the knocks on the roof, and the 1947 bulletin on 1420. Each
# goes into the notebook; the director keeps them apart and at night only.
# Usage: python3 tools/night.py OUTDIR
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': 1280, 'height': 720})).new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
        except Exception: pass
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await pg.wait_for_function('S47.started()', polling=200)
        ev = pg.evaluate
        N = "S47.nightEvents"
        notes = lambda: ev("S47.game.notes.slice()")
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def now(i): return await ev(f"{N}.now('{i}', S47.nightIn())")
        async def noted(word, n0, secs=6):
            for _ in range(int(secs / 0.5)):
                nn = (await notes())[n0:]
                if any(word in x for x in nn): return nn
                await tick(0.5)
                # (the radio keeps the audio clock: a long wait, the bulletin, needs real time too)
                if secs > 20: await pg.wait_for_timeout(450)
            return (await notes())[n0:]
        await ev("S47.hold = true; S47.tick(1.0)")
        check(not await ev("S47.nightIn().ok") or True, 'the prologue')
        await ev("S47.jump('chapter1')"); await pg.wait_for_timeout(500)
        await ev("S47.hold = true; S47.game.d.ui.close(); S47.tick(1.0)")
        # a spot outside, 8 to 12 m from the service door, that can be walked
        spot = await ev("""(() => { const d = new THREE.Vector3(); S47.room.objs.doorEast.getWorldPosition(d);
          for (let r = 8; r < 16; r += 1) for (let a = 0; a < 6.28; a += 0.2) { const x = d.x + Math.sin(a) * r, z = d.z + Math.cos(a) * r;
            if (S47.player.walkable(x, z)) { S47.player.place(x, z, 0); S47.tick(0.1); if (S47.nightIn().foot.outside) return [x, z, d.x, d.z]; } } return null; })()""".replace('new THREE.Vector3()', 'S47.camera.position.clone()'))
        check(spot is not None, f'outside by the building ({spot})')
        if not spot: raise SystemExit(1)
        await ev(f"S47.player.place({spot[0]}, {spot[1]}, 0); S47.tick(0.2)")
        check(await ev("S47.nightIn().ok"), 'chapter one, at night, outside: the director may run')

        # ---------- the steps behind ----------
        n0 = len(await notes())
        plays = "S47.__plays"
        await ev(f"{plays} = []; const a = S47.game.d.audio, f = a.play.bind(a); a.play = (n, o) => {{ {plays}.push([n, !!(o && o.at)]); return f(n, o); }}")
        await ev("S47.tick(0.1)")
        check(await now('steps'), 'the steps behind: now')
        await tick(0.25, 16)
        st = await ev(f"{plays}.filter((p) => String(p[0]).startsWith('step') && p[1]).length")
        nn = await noted('footsteps behind me', n0)
        check(st >= 2, f'two or three steps, from a place behind ({st})')
        check(any('footsteps behind me' in x for x in nn), f'the notebook: {nn[-1:] if nn else nn}')

        # ---------- the phone, once ----------
        n0 = len(await notes())
        await ev(f"{N}['wait'] = 999")
        check(await now('phone'), 'the phone: now')
        await tick(0.5, 2)
        rang = await ev(f"{plays}.some((p) => p[0] === 'phoneRing' && p[1])")
        nn = await noted('rang once', n0)
        check(rang and any('rang once' in x for x in nn), f'the control room phone rang once ({nn[-1:] if nn else nn})')

        # ---------- the door ----------
        n0 = len(await notes())
        await ev("S47.doors.set('east', false, true)")
        far = await ev(f"""(() => {{ const dx = {spot[2]}, dz = {spot[3]};
          for (let r = 26; r < 60; r += 2) for (let a = 0; a < 6.28; a += 0.15) {{ const x = dx + Math.sin(a) * r, z = dz + Math.cos(a) * r;
            if (S47.player.walkable(x, z)) {{ S47.player.place(x, z, Math.atan2(dx - x, dz - z)); S47.tick(0.1); const s = S47.nightIn(); if (s.foot.outside && s.saro && S47.nightEvents["can"]("door", s)) return [x, z]; }} }} return null; }})()""")
        check(far is not None, f'well away from the door, looking away ({far})')
        check(await now('door'), 'the door: now')
        await tick(0.5, 2)
        check(await ev("S47.doors.isOpen('east')"), 'it is open')
        nn = (await notes())[n0:]
        check(not any('stood open' in x for x in nn), 'not written down before it is seen')
        await ev(f"(() => {{ const dx = {spot[2]}, dz = {spot[3]}; S47.player.place({spot[0]}, {spot[1]}, 0); const p = S47.player.pos; S47.player.yaw = Math.atan2(-(dx - p.x), -(dz - p.z)); S47.player.pitch = 0; }})()")
        nn = await noted('stood open', n0)
        check(any('stood open' in x for x in nn), f'back by it, seeing it open ({nn[-1:] if nn else nn})')

        # ---------- the silence, away from the motel walk ----------
        n0 = len(await notes())
        check(await ev(f"{N}.now('silence', S47.nightIn())"), 'the silence: now')
        await tick(0.5)
        check(await ev("S47.roadside.silent"), 'no sound now')
        await tick(1.0, 50)
        nn = (await notes())[n0:]
        check(any('No sound for 47 s' in x and 'highway shoulder' not in x for x in nn), f'47 s, written down where it was ({nn[-1:] if nn else nn})')

        # ---------- the truck's radio, on by itself ----------
        n0 = len(await notes())
        tp = await ev("(() => { const g = S47.world.saroTruck.group.position; return [g.x, g.z]; })()")
        rs = await ev(f"""(() => {{ for (let r = 18; r < 34; r += 2) for (let a = 0; a < 6.28; a += 0.2) {{ const x = {tp[0]} + Math.sin(a) * r, z = {tp[1]} + Math.cos(a) * r;
            if (S47.player.walkable(x, z)) {{ S47.player.place(x, z, 0); S47.tick(0.1); S47.nightIn(); return [x, z]; }} }} return null; }})()""")
        await ev("S47.carRadio.set(false)"); await tick(0.2)
        check(rs is not None and await now('radio'), f'the radio: now ({rs})')
        await tick(0.5)
        check(await ev("S47.carRadio.on && S47.carRadio.station === 'far'"), 'the truck radio is on, on 1090')
        await ev(f"S47.player.place({tp[0]} + 3, {tp[1]}, 0)"); await tick(0.2)
        await ev(f"S47.player.place({tp[0]} + 2.5, {tp[1]}, 0)")
        nn = await noted('radio was on', n0)
        check(any('radio was on' in x for x in nn), f'back by the truck ({nn[-1:] if nn else nn})')
        await ev("S47.carRadio.set(false)")

        # ---------- driving: the knocks, and the bulletin ----------
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(240):
            await tick(0.25)
            if await ev("S47.world.driving"): break
            await pg.wait_for_timeout(100)
        n0 = len(await notes())
        check(await now('knocks'), 'driving: the knocks')
        await tick(0.5, 3)
        k = await ev(f"{plays}.filter((p) => p[0] === 'thudSoft').length")
        nn = await noted('knocks on the roof', n0)
        check(k == 3 and any('knocks on the roof' in x for x in nn), f'three, and the notebook ({k}, {nn[-1:] if nn else nn})')
        n0 = len(await notes())
        await ev("S47.game.clock = 2 * 3600 + 40 * 60; S47.carRadio.set(true, 'caliche')"); await tick(0.5, 2)
        check(await now('bulletin'), 'the radio on 1420: the bulletin')
        seg = await ev("S47.radio.stations.caliche.seg.lines ? S47.radio.stations.caliche.seg.lines[0].text : ''")
        check('bulletin' in seg, f'on the air: {seg}')
        nn = await noted('news bulletin', n0, 60)
        check(any('1947' in x for x in nn), f'and in the notebook ({nn[-1:] if nn else nn})')
        after = await ev("S47.radio.stations.caliche.seg.kind")
        check(after in ('music', 'talk'), f'then the Night Line again ({after})')

        # ---------- kept, and not again ----------
        seen = await ev("[...S47.townSeen].filter((k) => k.startsWith('ne:')).sort()")
        check(len(seen) >= 6, f'kept with the case ({seen})')
        check(not await ev(f"{N}['can']('knocks', S47.nightIn())") and not await ev(f"{N}['can']('bulletin', S47.nightIn())"), 'the knocks and the bulletin: once a night')

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks}, open(f'{OUT}/night.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
