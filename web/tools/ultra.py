# The PC tier (core/ultra.ts): real shadows from the nearest lamps, ambient occlusion and
# bloom. Switches to Ultra, visits the control room, the yard, the records room and room 6,
# and takes the same views on High for comparison. Usage: python3 tools/ultra.py OUTDIR [WxH]
# The software renderer says nothing about frame rate; that is UNVERIFIED until measured on
# a real PC.
import asyncio, math, sys, json, os
from playwright.async_api import async_playwright
OUT = sys.argv[1]; W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what): checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'])
        pg = await b.new_page(viewport={'width': W, 'height': H}); pg.set_default_timeout(300000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]')
        check(await pg.evaluate("S47.ultra.on") is False, 'an automated browser starts on High, not Ultra')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(300)
        ev = pg.evaluate
        await ev("S47.hold = true")
        async def tick(s): await ev(f"S47.tick({s})")
        async def look(x, z, yaw, pitch=0.0): await ev(f"S47.player.place({x},{z},{yaw}); S47.player.pitch={pitch};")
        async def pair(name, setup):
            for q in ('ultra', 'high'):
                await ev(f"S47.setQuality('{q}')"); await setup(); await tick(0.6)
                calls = await ev("S47.renderer.info.render.calls")
                await pg.screenshot(path=f'{OUT}/{name}_{q}.png')
                print(f'{name} {q}: {calls} draw calls', flush=True)
        await ev("S47.jump('residual')"); await tick(1)
        await ev("S47.game.d.inter.get('rack').use()"); await tick(2.5)
        async def desk(): await look(1.2, 3.6, -0.6, -0.25)
        await pair('u01_control_room', desk)
        await ev("S47.setQuality('ultra')"); await tick(0.2)
        check(await ev("S47.renderer.shadowMap.enabled") and await ev("S47.room.lights.lamps[0].castShadow"), 'Ultra: shadow maps on, the desk lamp casts shadows')
        # round 9 (Codex): normal and roughness maps on the 19 surfaces, Ultra only (three.js NoColorSpace is '')
        await pg.wait_for_function("S47.ultra.mapped > 0", timeout=60000)
        mapped = await ev("S47.ultra.mapped")
        floor = await ev("""(()=>{ let r = null; S47.scene.traverse(o => { const m = o.material; if (!r && m && m.map && m.map.name === 'art/floor') r = m; });
          return r && { n: r.normalMap && r.normalMap.name, cs: r.normalMap && r.normalMap.colorSpace, rcs: r.roughnessMap && r.roughnessMap.colorSpace,
            rep: r.normalMap && r.normalMap.repeat.toArray().join(',') === r.map.repeat.toArray().join(','), rough: r.roughness }; })()""")
        check(mapped >= 10 and floor and floor['n'] == 'art/floorN' and floor['cs'] == '' and floor['rcs'] == '' and floor['rep'] and floor['rough'] == 1,
              f'Ultra: {mapped} materials get the round 9 maps; the floor: data colour space, same repeat as its picture, roughness factor 1')
        twin = await ev("""(()=>{ let r = null; S47.scene.traverse(o => { const m = o.material; if (!r && m && m.map && m.map.userData && m.map.userData.dataTwin) r = m; });
          return r && !!r.normalMap && r.normalMap.colorSpace === '' && r.normalMap.repeat.y === r.map.repeat.y; })()""")
        check(twin, 'the road past SARO (a canvas made from the asphalt) gets maps drawn the same way')
        await ev("S47.jump('chapter1')"); await tick(2)
        async def yard(): await look(9.0, -2.0, -0.4, -0.3)
        await pair('u02_yard', yard)
        await ev("S47.setQuality('ultra')"); await look(9.0, -2.0, -0.4, -0.3); await tick(0.6)
        lit = await ev("S47.ultra['spots'].filter(s => s.intensity > 0).length")
        check(lit >= 1, f'Ultra: the lamps nearest the yard are real lights with shadows ({lit})')
        off = await ev("(()=>{ let n = 0; for (const h of S47.ultra['held']) if (h && h.set.pos[h.i].w < 0) n++; return n; })()")
        check(off == lit, 'and their fake pools are switched off meanwhile')
        await ev("S47.setPicture('off')"); await tick(0.4); await pg.screenshot(path=f'{OUT}/u03_yard_clean_ultra.png')
        await ev("S47.setPicture('vhs')")
        await ev("S47.jump('chapter2')"); await tick(1.5)
        await ev("S47.doors.set('south', true, true); S47.doors.set('records', true, true)")
        async def records(): await look(-1.6, 9.6, math.atan2(-(-3.6 + 1.6), -(6.8 - 9.6)), -0.12)
        await pair('u04_records', records)
        await ev("S47.setQuality('high')"); await tick(0.3)
        check(not await ev("S47.renderer.shadowMap.enabled") and await ev("S47.ultra['spots'].every(s => s.intensity === 0)"), 'back on High: no shadow maps, the fake lamps are back')
        back = await ev("""(()=>{ let n = 0, bad = 0; S47.scene.traverse(o => { const m = o.material; if (m && m.map && m.map.name === 'art/floor') { n++; if (m.normalMap || m.roughnessMap || m.roughness === 1) bad++; } }); return [n, bad]; })()""")
        check(await ev("S47.ultra.mapped") == 0 and back[1] == 0, f'and the maps are off again, the floor back to its own roughness ({back})')
        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        if errs: check(False, 'console clean')
        json.dump(checks, open(f'{OUT}/checks.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        if fails: raise SystemExit(1)
asyncio.run(main())
