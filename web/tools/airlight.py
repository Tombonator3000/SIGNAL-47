# Light in the air (world/airlight.ts): faint cones under the sodium lamps and a low haze on the
# open ground at night, on High and Ultra, none on Low. At SARO (the yard, a highway lamp close
# by, the west lot), at the motel and in Caliche's street: the night with and without it side by
# side (OUTDIR/*_pair.png), what it costs in draw calls (one for an area's cones, one for the
# haze), that it lightens the picture only a little (memory.md: the floods once turned the night
# into day), that a cone is there under a lamp, none on Low, none indoors, none at dawn, and that
# a lamp of Caliche's that goes out (event 8) takes its cone with it. No colour values the
# picture's targets cannot hold on Ultra.
# Usage: python3 tools/airlight.py OUTDIR [WxH]
import asyncio, sys, os, json
from intro_flow import begin_opening
from playwright.async_api import async_playwright
from PIL import Image, ImageStat

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
results = []
def check(ok, msg):
    results.append((bool(ok), msg)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

def lum(path, box=None):
    im = Image.open(path).convert('L')
    if box: im = im.crop(box)
    return ImageStat.Stat(im).mean[0]

def pair(a, b, out):
    A, B = Image.open(a), Image.open(b)
    o = Image.new('RGB', (A.width * 2 + 6, A.height), (200, 0, 0))
    o.paste(A, (0, 0)); o.paste(B, (A.width + 6, 0)); o.save(out)

AIR = "(() => { const a = S47.air(); return { cones: a.cones.map((c) => c.mesh.visible), haze: a.haze.mesh.visible, n: a.cones.map((c) => c.lamps.length) }; })()"

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width': W, 'height': H}); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]')
        ev = pg.evaluate
        await ev("S47.setQuality('high')"); await pg.wait_for_timeout(1500)
        a = await ev(AIR)
        await pg.screenshot(path=f'{OUT}/title_high.png')
        check(a['cones'][0] and a['haze'] and a['n'][0] >= 14, f"behind the title: the cones under SARO's {a['n'][0]} sodium lamps and the haze")
        await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(400)
        await ev("S47.hold = true; S47.tick(0.3); S47.setPicture('off')")
        await ev("S47.jump('chapter1'); S47.tick(1.0)")

        async def spot(name, x, z, yaw, pitch, q='high', box=None):
            await ev(f"S47.setQuality('{q}'); S47.player.place({x}, {z}, {yaw}); S47.player.pitch = {pitch}; S47.world.airOff = false; S47.tick(0.4)")
            on = await ev("S47.renderer.info.render.calls"); st = await ev(AIR)
            await pg.screenshot(path=f'{OUT}/{name}_{q}_on.png')
            await ev("S47.world.airOff = true; S47.tick(0.1)")
            off = await ev("S47.renderer.info.render.calls")
            await pg.screenshot(path=f'{OUT}/{name}_{q}_off.png')
            await ev("S47.world.airOff = false; S47.tick(0.1)")
            pair(f'{OUT}/{name}_{q}_on.png', f'{OUT}/{name}_{q}_off.png', f'{OUT}/{name}_{q}_pair.png')
            l1, l0 = lum(f'{OUT}/{name}_{q}_on.png'), lum(f'{OUT}/{name}_{q}_off.png')
            loc = (lum(f'{OUT}/{name}_{q}_on.png', box), lum(f'{OUT}/{name}_{q}_off.png', box)) if box else None
            return {'on': on, 'off': off, 'l1': l1, 'l0': l0, 'loc': loc, 'st': st}

        # SARO: the yard, a highway lamp close by (the cone in a box under it), the west lot
        r = await spot('yard', 9.5, -17.5, 0.35, 0.06)
        check(r['on'] - r['off'] <= 2 and r['st']['haze'], f"the yard: {r['on'] - r['off']} draw calls more ({r['off']} without), the haze on")
        check(r['l1'] - r['l0'] < 4, f"and the picture only a little lighter ({r['l0']:.1f} to {r['l1']:.1f} of 255)")
        # the lamp's head projects to about x 660, y 300 at 1280x800; under it down to the ground
        bx = (int(W * 0.47), int(H * 0.40), int(W * 0.56), int(H * 0.60))
        r = await spot('lamp', -12, 22, 0.42, 0.12, box=bx)
        check(r['loc'][0] - r['loc'][1] > 0.6, f"a highway lamp close by: a cone under it ({r['loc'][1]:.1f} to {r['loc'][0]:.1f} in the box under the lamp)")
        check(r['l1'] - r['l0'] < 4, f"the whole picture: {r['l0']:.1f} to {r['l1']:.1f}")
        r = await spot('west', -14, 50, 0.15, 0.04)
        check(r['on'] - r['off'] <= 2 and r['l1'] - r['l0'] < 5, f"the west lot towards the array: {r['on'] - r['off']} draw calls, {r['l0']:.1f} to {r['l1']:.1f}")
        r = await spot('motel', -27, 630, 3.3, 0.02)
        check(r['on'] - r['off'] <= 2 and r['l1'] - r['l0'] < 4, f"the motel from the shoulder: {r['on'] - r['off']} draw calls, {r['l0']:.1f} to {r['l1']:.1f}")
        # Ultra: the same, through the bloom; nothing out of range
        r = await spot('lamp', -12, 22, 0.42, 0.12, q='ultra', box=bx)
        hdr = await ev("S47.hdrStats()")
        check(r['l1'] - r['l0'] < 5 and hdr['over'] == 0, f"on Ultra: {r['l0']:.1f} to {r['l1']:.1f}, {hdr['over']} values out of range (brightest {hdr['max']:.1f})")
        # Low: none of it, and not drawn
        r = await spot('lamp', -12, 22, 0.42, 0.12, q='low')
        check(not any(r['st']['cones']) and not r['st']['haze'] and r['on'] == r['off'], f"on Low: no cones, no haze, the same draw calls ({r['on']}, {r['off']})")
        await ev("S47.setQuality('high'); S47.tick(0.2)")
        # indoors: the haze stays outside (the cones are outside anyway)
        await ev("S47.player.place(0, -1, 0); S47.player.pitch = 0; S47.tick(0.4)")
        st = await ev(AIR)
        check(not st['haze'], 'in the control room: no haze')
        # at dawn it has gone
        clock = await ev("S47.game.clock")
        await ev("S47.game.clock = 6 * 3600 + 10 * 60; S47.player.place(-14, 50, 0.15); S47.tick(1.0)")
        st = await ev(AIR)
        await pg.screenshot(path=f'{OUT}/dawn.png')
        check(not any(st['cones']) and not st['haze'], f"at 06:10: no cones and no haze ({await ev('S47.sky.uniforms.uDawn.value'):.2f} dawn)")
        await ev(f"S47.game.clock = {clock}; S47.tick(0.5)")

        # Caliche: the street lamps' cones, and one that goes out takes its cone with it
        await ev("S47.world.goDiner('outside')")
        for _ in range(60):
            await pg.wait_for_timeout(250)
            if await ev("S47.world.area === 'diner' && !!S47.world.town"): break
        await ev("S47.tick(0.5)")
        r = await spot('town', 8001.5, 2095, 3.14159, 0.03)
        n = await ev("S47.air().town.lamps.length")
        check(n == 7 and r['st']['cones'][-1] and r['st']['haze'] and r['on'] - r['off'] <= 2, f"Caliche: cones under its {n} street lamps, {r['on'] - r['off']} draw calls, {r['l0']:.1f} to {r['l1']:.1f}")
        await ev("S47.world.town.lampWave(S47.runTime()); S47.tick(1.5)")
        ks = await ev("(() => { const c = S47.air().town; return c.lamps.map((L) => L.k()); })()")
        ons = await ev("S47.world.town['lamps'].map((L) => L.on)")
        await pg.screenshot(path=f'{OUT}/town_wave.png')
        # the cones are read south to north from the glow points; the lamps keep their own order
        check(sorted(ks) == sorted([1.0 if o else 0.0 for o in ons]) and 0 in [round(k) for k in ks], f"event 8: a lamp goes out and its cone with it ({ks}, {ons})")

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        await b.close()
    ok = sum(1 for r in results if r[0])
    json.dump([{'ok': r[0], 'msg': r[1]} for r in results], open(f'{OUT}/airlight.json', 'w'), indent=1)
    print(f"{ok} of {len(results)} PASS")
    sys.exit(0 if ok == len(results) else 1)

asyncio.run(main())
