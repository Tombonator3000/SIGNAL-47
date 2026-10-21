# Life in the night (world/nightlife.ts): moths round the lamps and the porch lights (and resting
# on the walls by them), bats at the lamps the player walks under in the early night, dust in the
# flashlight's beam and in the headlights on the survey track, far lights out on the land (a car
# on a county road now and then, ranch and trailer windows, a siding's lamps, Roswell's glow),
# tumbleweeds lying where the wind left them and now and then one across the highway in a gust.
# Calm, rare and cheap: at fixed views at SARO, the motel, Caliche and on the track it shoots the
# night on High (OUTDIR/<view>.png), and the same frame with the life switched off beside it
# (OUTDIR/<view>_pair.png), and checks what it costs in draw calls (at most 15 more per area, the
# budgets of the control room and SARO's grounds kept, and every view at most SHARED more than
# before the round for its five parts together), that it is there where it should be (a
# lamp's moths, the dust in a beam, the car moving, the tumbleweed rolling) and not where it should
# not (indoors, at dawn, when a lamp goes out, bats late in the night), that Low has less of it and
# no more draw calls, that the moths and the dust still show on a phone's screen (844x390) and at
# the adaptive resolution's lowest (pixel ratio 0.7), that the headlights' dust is thick only when
# the truck slows, that the tumbleweed has no collider and is never hit (the player walks on
# through it), and no colour values out of range on Ultra that are not there without it.
# --before: only the views and their draw calls (a build without nightlife.ts), for comparing.
# --compare=DIR: a --before run's OUTDIR; SARO's views with the life off must be the same picture
# as before it (the same frames: the life draws nothing then and changes nothing in the night).
# Usage: python3 tools/env_atmosphere.py OUTDIR [WxH] [--before] [--compare=DIR]
import asyncio, sys, os, json, math
from intro_flow import begin_opening
from playwright.async_api import async_playwright
from PIL import Image, ImageStat, ImageChops

args = [a for a in sys.argv[1:] if not a.startswith('--')]
BEFORE = '--before' in sys.argv
COMPARE = next((a.split('=', 1)[1] for a in sys.argv[1:] if a.startswith('--compare=')), None)
OUT = args[0]
W, H = (int(v) for v in (args[1] if len(args) > 1 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
results = []
def check(ok, msg):
    results.append((bool(ok), msg)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

def lum(path, box=None):
    im = Image.open(path).convert('L')
    if box: im = im.crop(box)
    return ImageStat.Stat(im).mean[0]

def changed(a, b, box=None, thr=6):
    """How many pixels differ by more than thr (of 255) between two pictures."""
    A, B = Image.open(a).convert('L'), Image.open(b).convert('L')
    if box: A, B = A.crop(box), B.crop(box)
    return ImageChops.difference(A, B).point(lambda v: 255 if v > thr else 0).histogram()[255]

def pair(a, b, out):
    A, B = Image.open(a), Image.open(b)
    o = Image.new('RGB', (A.width * 2 + 6, A.height), (200, 0, 0))
    o.paste(A, (0, 0)); o.paste(B, (A.width + 6, 0)); o.save(out)

# The fixed views (SARO's terms, the town's in the scene's): x, z, yaw, pitch
VIEWS = {
    'hwylamp': (-16.0, 44.0, 0.474, 0.42),     # a highway lamp on the shoulder, 9 m off
    'door':    (16.5, 8.0, 0.77, 0.18),        # the lamps over the east door
    'east':    (-16.0, 222.0, -1.5708, 0.0),   # out over the land east of the highway
    'motel':   (-36.0, 690.0, 1.5708, 0.12),   # the lamps over the motel's room doors
    'flash':   (-14.0, 50.0, 0.15, -0.12),     # the flashlight in the dark of the west lot
    'room':    (0.0, -1.0, 0.0, 0.0),          # the control room (its budget)
    'far':     (-30.0, 100.0, 0.62, 0.02),     # out north-west over the land from the west lot (far lights)
    'town':    (8000.5, 2114.0, 2.51, 0.36),   # under a street lamp in Caliche
}
LIFE = "(() => { const L = S47.world.life; return L ? L.stats() : null; })()"
# the draw calls of each view on High before the round (--before, main at 048fcd1, 19 October, the
# graphics card), and what the round's five parts may add to a view together (the same in
# env_ground, env_roads, env_buildings, env_interiors and env_atmosphere). Each part was held to 15
# in its own branch, but they meet in the same views: a part's share of a view is about six (the
# ground cover's five meshes; the life's moths, far lights and lying tumbleweeds; the roads' wear
# and lots; a lot's props, stains and pools; a room's things and dirt), five of them thirty. A view
# over it has a part costing twice its share, or a sixth thing in it: a real overshoot.
BEFORE_ROUND = {'hwylamp': 136, 'door': 298, 'east': 27, 'motel': 49, 'flash': 322, 'room': 112, 'far': 73, 'town': 41, 'track': 54}
SHARED = 30

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
        await ev("S47.setQuality('high')")
        await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(400)
        await ev("S47.hold = true; S47.tick(0.3); S47.setPicture('off')")
        # the pictures without the HUD and the captions: the night itself
        await pg.add_style_tag(content='#ui { visibility: hidden !important; }')
        await ev("S47.jump('chapter1'); S47.tick(1.0)")
        # the same hour for every view: 02:40, the early night (the bats are out)
        await ev("S47.game.clock = 2 * 3600 + 40 * 60; S47.tick(0.5)")
        has = bool(await ev("!!S47.world.life")) and not BEFORE
        # the car and the tumbleweed come when this test asks for them, not by chance in between
        if has: await ev("S47.world.life['carT'] = 1e9; S47.world.life['weedT'] = 1e9")
        calls, stats = {}, {}

        async def setlife(on):
            if has: await ev(f"S47.world.life.off = {'false' if on else 'true'}")

        async def offframe():
            """The same frame without the life: nothing of the night stepped on, drawn again (before
            the life came in, the frame drawn again all the same, so both runs draw the same frames)."""
            if not has: return await ev("(() => { S47.renderer.info.reset(); S47.vhs.render(S47.scene, S47.camera, performance.now() / 1000); return S47.renderer.info.render.calls; })()")
            return await ev("""(() => { const L = S47.world.life, t = S47.runTime(); L.off = true; S47.world.lifeStep(0, t);
              S47.renderer.info.reset(); S47.vhs.render(S47.scene, S47.camera, performance.now() / 1000); const n = S47.renderer.info.render.calls;
              L.off = false; S47.world.lifeStep(0, t); return n; })()""")

        async def view(name, q='high', light=False, settle=0.6, tag=None, both=True):
            """The view with the life (and the same frame without it): draw calls, pictures, stats."""
            x, z, yaw, pitch = VIEWS[name]
            tag = (tag or name) + ('' if q == 'high' else '_' + q)
            await setlife(True)
            await ev(f"S47.setQuality('{q}'); S47.player.place({x}, {z}, {yaw}); S47.player.pitch = {pitch}; S47.flashlight.have = true; S47.flashlight.on = {'true' if light else 'false'}; S47.tick({settle})")
            n = await ev("S47.renderer.info.render.calls")
            await pg.screenshot(path=f'{OUT}/{tag}.png')
            st = await ev(LIFE) if has else None
            if not both: return n, None, st
            n0 = await offframe()
            await pg.screenshot(path=f'{OUT}/{tag}_off.png')
            if has: pair(f'{OUT}/{tag}.png', f'{OUT}/{tag}_off.png', f'{OUT}/{tag}_pair.png')
            return n, n0, st

        # ---------- SARO ----------
        for name in ['hwylamp', 'door', 'east', 'motel', 'flash', 'room', 'far']:
            n, n0, st = await view(name, light=name in ('flash', 'room'))
            calls[name] = n
            if has: calls[name + '_off'] = n0; stats[name] = st

        if has and COMPARE:
            # with the life off the night is the picture it was before the life came in, the same
            # frames of the same night (but for the little that differs from one run to the next:
            # two runs of the same build differ by up to about a hundred pixels this much)
            for name in ['hwylamp', 'door', 'east', 'motel', 'flash', 'room']:
                d = changed(f'{COMPARE}/{name}_off.png', f'{OUT}/{name}_off.png', thr=20)
                check(d <= 150, f"{name}: the life off is the picture before it ({d} pixels differ by more than 20 of 255)")
        if has:
            st = stats['hwylamp']
            print('life:', json.dumps(st))
            # the lamps the moths fly round: SARO's sodium heads, its door lamps and the motel's;
            # the porch lights have their walls and moths resting on them
            check(st['saro']['lamps'] >= 30 and st['saro']['visible'] and st['saro']['batsDrawn'] > 0,
                  f"SARO: moths round {st['saro']['lamps']} lamps ({st['saro']['moths']} moths, {st['saro']['bats']} bats, all drawn at 02:40)")
            check(st['saro']['porch'] >= 8 and st['saro']['resting'] >= 2 * st['saro']['porch'] and st['saro']['wallMs'] < 400,
                  f"SARO: {st['saro']['porch']} porch lights on their walls, {st['saro']['resting']} moths resting by them (found in {st['saro']['wallMs']} ms)")
            check(st['saro']['weeds'] >= 1 and st['far']['lights'] >= 12, f"SARO: tumbleweeds lying about ({st['saro']['weeds']} meshes drawn), {st['far']['lights']} far lights")
            # the far lights out north-west (a ranch, a siding's lamps, a trailer's television)
            near = changed(f'{OUT}/far.png', f'{OUT}/far_off.png', (0, int(H * 0.4), W, int(H * 0.6)), thr=4)
            check(near >= 12, f"far lights over the land to the north-west: {near} pixels")
            for name in ['hwylamp', 'door', 'east', 'motel', 'flash', 'far']:
                d = calls[name] - calls[name + '_off']
                check(0 <= d <= 15, f"{name}: {d} draw calls more with the life ({calls[name + '_off']} without, {calls[name]} with)")
            check(calls['room'] - calls['room_off'] <= 3 and calls['room'] <= 300, f"the control room with the flashlight on: {calls['room'] - calls['room_off']} more ({calls['room']})")
            check(max(calls['hwylamp'], calls['door'], calls['flash']) <= 340, f"SARO's grounds within tools/grounds.py's 340 ({calls['hwylamp']}, {calls['door']}, {calls['flash']})")
            # a lamp's moths: pixels round the highway lamp's head change with them, the picture as a whole hardly
            near = changed(f'{OUT}/hwylamp.png', f'{OUT}/hwylamp_off.png', (int(W * 0.30), int(H * 0.3), int(W * 0.75), int(H * 0.95)))
            whole = lum(f'{OUT}/hwylamp.png') - lum(f'{OUT}/hwylamp_off.png')
            check(near >= 8 and abs(whole) < 1.0, f"the highway lamp: {near} pixels round its head show the moths and bats, the picture {whole:+.2f} of 255 lighter")
            for name, what in [('door', "the east door's lamps"), ('motel', "the motel's door lamps")]:
                near = changed(f'{OUT}/{name}.png', f'{OUT}/{name}_off.png')
                check(near >= 4, f"{what}: {near} pixels show moths")
            # the dust in the flashlight's beam (the middle of the picture), on with the light only
            near = changed(f'{OUT}/flash.png', f'{OUT}/flash_off.png', (int(W * 0.3), int(H * 0.25), int(W * 0.8), int(H * 0.9)), thr=3)
            check(near >= 6 and stats['flash']['motes']['flash'], f"the flashlight: dust in the beam ({near} pixels)")
            check(stats['room']['motes']['flash'] and not stats['room']['saro']['visible'], 'in the control room: dust in the beam, the moths outside not drawn')
            await ev("S47.flashlight.on = false; S47.tick(0.3)")
            st = await ev(LIFE)
            check(not st['motes']['flash'], 'with the flashlight off, no dust drawn')

            # ---------- Low: less of it, never more draw calls ----------
            n, n0, st = await view('hwylamp', q='low')
            check(n - n0 <= calls['hwylamp'] - calls['hwylamp_off'] and st['saro']['moths'] * 0.55 < st['saro']['drawn'] < st['saro']['moths'] * 0.85 and st['saro']['batsDrawn'] == 0,
                  f"Low: {st['saro']['drawn']} of {st['saro']['moths']} moths and no bats drawn, {n - n0} draw calls more ({n})")
            n, n0, st = await view('flash', q='low', light=True)
            check(st['motes']['flashN'] < st['motes']['flashMax'] and n - n0 <= calls['flash'] - calls['flash_off'], f"Low: {st['motes']['flashN']} of {st['motes']['flashMax']} motes in the beam, {n - n0} draw calls more")
            near = changed(f'{OUT}/flash_low.png', f'{OUT}/flash_low_off.png', (int(W * 0.3), int(H * 0.25), int(W * 0.8), int(H * 0.9)), thr=3)
            check(near >= 6, f"Low: the dust shows in the beam ({near} pixels)")
            await ev("S47.flashlight.on = false; S47.setQuality('high'); S47.tick(0.2)")

            # ---------- a phone's screen, and the adaptive resolution at its lowest ----------
            # (the points' sizes go with the lines drawn: here is where they would be lost first)
            for vw, vh, pr in [(844, 390, 1.0), (844, 390, 0.7)]:
                await pg.set_viewport_size({'width': vw, 'height': vh})
                await ev(f"S47.renderer.setPixelRatio({pr}); dispatchEvent(new Event('resize')); S47.tick(0.1)")
                sfx = f'_{vw}x{vh}' + ('' if pr == 1.0 else f'_pr{int(pr * 10)}')
                for name, light in [('hwylamp', False), ('motel', False), ('flash', True)]:
                    await view(name, light=light, tag=name + sfx)
                    d = changed(f'{OUT}/{name}{sfx}.png', f'{OUT}/{name}{sfx}_off.png', thr=3 if name == 'flash' else 6)
                    check(d >= (6 if name == 'flash' else 10), f"{vw}x{vh} at pixel ratio {pr}: {d} pixels of life in the {name} view")
                await ev("S47.flashlight.on = false; S47.tick(0.1)")
            await pg.set_viewport_size({'width': W, 'height': H})
            await ev("S47.renderer.setPixelRatio(1); dispatchEvent(new Event('resize')); S47.tick(0.1)")
            # the bats at the highway lamp, a few frames over a minute
            seen = 0
            for k in range(6):
                await ev("S47.tick(9, 30)")
                n, n0, _ = await view('hwylamp', tag=f'hwylamp_bat{k}', settle=0.05)
                if changed(f'{OUT}/hwylamp_bat{k}.png', f'{OUT}/hwylamp_bat{k}_off.png', (int(W * 0.2), 0, int(W * 0.9), int(H * 0.9))) > 0: seen += 1
            check(seen >= 3, f"the highway lamp's life shows in {seen} of 6 frames over a minute")

            # ---------- late, and at dawn ----------
            await ev("S47.game.clock = 4 * 3600 + 5 * 60; S47.tick(0.5)")
            n, _, st = await view('hwylamp', tag='hwylamp_0405', both=False)
            check(st['saro']['visible'] and st['saro']['batsDrawn'] == 0 and st['activity']['moth'] < 0.8,
                  f"04:05: fewer moths (activity {st['activity']['moth']:.2f}), the bats gone ({st['activity']['bat']:.2f})")
            await ev("S47.game.clock = 6 * 3600 + 10 * 60; S47.tick(1.0)")
            await ev("S47.world.life.carNow(); S47.tick(0.5)")
            n, _, st = await view('hwylamp', tag='hwylamp_dawn', both=False)
            check(not st['saro']['visible'] and not st['far']['car'], f"06:10: no moths and no far car ({st['activity']})")
            await ev("S47.game.clock = 2 * 3600 + 40 * 60; S47.tick(0.5)")

            # ---------- the far car on the county road ----------
            await view('east', both=False)
            await ev("S47.world.life.carNow(); S47.tick(0.2)")
            c0 = (await ev(LIFE))['far']
            n, _, _ = await view('east', tag='east_car', settle=0.2, both=False)
            await ev("S47.tick(6, 30)")
            c1 = (await ev(LIFE))['far']
            moved = math.hypot(c1['carAt'][0] - c0['carAt'][0], c1['carAt'][1] - c0['carAt'][1])
            check(c0['car'] and 80 < moved < 200 and c0['carDist'] > 900, f"a car on the county road, {c0['carDist']:.0f} m off, {moved:.0f} m along it in 6 s")
            check(n - calls['east'] <= 1, f"and no draw call more for it ({n})")

            # ---------- a tumbleweed across the highway ----------
            # on foot it comes only where it is seen: not on the dark shoulder half way to the motel
            # (unless the flashlight is on), and under the highway lamps by the site
            cols = await ev("S47.player.colliders.length")
            await ev("S47.flashlight.on = false; S47.player.place(-17, 300, 3.14159); S47.player.pitch = 0.0; S47.tick(0.2)")
            dark = await ev("S47.world.life.tumbleNow()")
            await ev("S47.flashlight.on = true; S47.tick(0.3)")
            torch = await ev("S47.world.life.tumbleNow()")
            check(not dark and torch, f"in the dark half way to the motel: none ({dark}), with the flashlight on: one in its beam ({torch})")
            await ev("S47.flashlight.on = false; S47.player.place(-16, 44, 1.5708); S47.player.pitch = 0.05; S47.tick(0.2)")
            ok = False
            for _ in range(6):
                ok = await ev("S47.world.life.tumbleNow()")
                if ok: break
            await ev("S47.tick(0.1)")
            t0 = (await ev(LIFE))['tumble']
            for k in range(3):
                await ev("S47.tick(1.2, 30)")
                await pg.screenshot(path=f'{OUT}/tumble_{k}.png')
            t1 = (await ev(LIFE))['tumble']
            rolled = math.hypot(t1['at'][0] - t0['at'][0], t1['at'][2] - t0['at'][2])
            check(ok and t0['on'] and rolled > 4 and t1['spin'] > t0['spin'], f"a tumbleweed rolls {rolled:.1f} m in 3.6 s, turning over ({[round(v, 1) for v in t0['at']]} to {[round(v, 1) for v in t1['at']]})")
            check(abs(t1['at'][1] - t1['ground']) < 1.2, f"on the ground ({t1['at'][1]:.2f}, the ground {t1['ground']:.2f})")
            check(await ev("S47.player.colliders.length") == cols, 'no collider of its own')
            hit = await ev("""(() => { const L = S47.world.life, m = L.tumbleMesh(), rc = new S47.three.Raycaster();
              const o = S47.camera.position.clone(), d = m.getWorldPosition(new S47.three.Vector3()).sub(o).normalize();
              rc.set(o, d); return rc.intersectObject(m, true).length; })()""")
            check(hit == 0, 'and never hit by a ray (the crosshair, the ground under the feet)')
            # the player walks on along the shoulder, the tumbleweed about
            await ev("S47.player.place(-17, 300, 3.14159); S47.tick(0.1); S47.world.life.tumbleNow(true); S47.tick(0.1)")
            r = await ev("""(() => { const p = S47.player; p.place(-17, 300, 3.14159); let n = 0;
              while (p.pos.z < 330) { p.yaw = Math.PI; p.update(1/30, 0, -1, false); S47.world.lifeStep(1/30, S47.runTime()); if (++n > 900) break; }
              return [p.pos.x, p.pos.z, n, S47.world.life.stats().tumble.on]; })()""")
            check(r[1] >= 330 and r[3], f"walking down the shoulder with one rolling about: 30 m in {r[2]} steps ({r[0]:.1f}, {r[1]:.1f})")

        # ---------- Caliche ----------
        await ev("S47.world.goDiner('outside')")
        for _ in range(80):
            await pg.wait_for_timeout(250)
            if await ev("S47.world.area === 'diner' && !!S47.world.town"): break
        await ev("S47.tick(0.5)")
        n, n0, st = await view('town')
        calls['town'] = n
        if has:
            calls['town_off'] = n0
            check(st['road']['lamps'] >= 7 and st['road']['visible'] and 0 <= n - n0 <= 15, f"Caliche: moths round {st['road']['lamps']} lamps, {n - n0} draw calls more ({n})")
            check(st['road']['weeds'] >= 1 and st['road']['batsDrawn'] >= 3, f"Caliche: tumbleweeds lying about ({st['road']['weeds']}), {st['road']['batsDrawn']} bats ({st['road']['porch']} porch lights, {st['road']['resting']} resting)")
            near = changed(f'{OUT}/town.png', f'{OUT}/town_off.png', (int(W * 0.25), 0, int(W * 0.75), int(H * 0.6)))
            check(near >= 8, f"under the street lamp: {near} pixels show moths and bats")
            # Ultra: no colour value out of range that is not there without the life, in the same frame
            await view('town', q='ultra', both=False)
            hdr = await ev("(() => { const t = S47.runTime(), a = S47.hdrStats(); S47.world.life.off = true; S47.world.lifeStep(0, t); const b = S47.hdrStats(); S47.world.life.off = false; S47.world.lifeStep(0, t); return [a, b]; })()")
            check(hdr[0]['over'] <= hdr[1]['over'], f"Ultra in Caliche: {hdr[0]['over']} values out of range with the life, {hdr[1]['over']} without (the brightest {hdr[0]['max']:.1f})")
            await ev("S47.setQuality('high'); S47.tick(0.2)")
            # event 8: a lamp that goes out has no moths in its light
            await ev("S47.world.town.lampWave(S47.runTime()); S47.tick(1.5)")
            k = await ev("S47.world.life.lampK('road')")
            ons = await ev("S47.world.town['lamps'].map((L) => L.on)")
            dark, out = sum(1 for v in k if v < 0.05), sum(1 for o in ons if not o)
            check(out > 0 and dark >= out, f"event 8: the moths of a lamp that goes out go dark ({dark} dark, {out} lamps out)")
            await ev("S47.tick(30, 30)")

        # ---------- the survey track in the headlights ----------
        await ev("S47.world.driveOut(true)")
        for _ in range(240):
            await ev("S47.tick(0.25)")
            if await ev("S47.world.driving"): break
            await pg.wait_for_timeout(100)
        await ev("S47.world.autopilot = 11")
        kind = None
        for _ in range(240):
            await ev("S47.tick(0.5, 30)")
            kind = await ev("(() => { const w = S47.world, L = w.legs[w.leg]; return L ? L.drive.surface.kind : null; })()")
            if kind == 'gravel': break
        await ev("S47.tick(4, 30)")
        n = await ev("S47.renderer.info.render.calls")
        await pg.screenshot(path=f'{OUT}/track.png')
        calls['track'] = n
        if has:
            st = await ev(LIFE)
            n0 = await offframe()
            await pg.screenshot(path=f'{OUT}/track_off.png')
            pair(f'{OUT}/track.png', f'{OUT}/track_off.png', f'{OUT}/track_pair.png')
            calls['track_off'] = n0
            m = st['motes']
            check(kind == 'gravel' and m['head'] and m['specks'] > 0.3 and m['cloud'] < 0.25 and n - n0 <= 15,
                  f"on the survey track's gravel at speed: fine specks in the headlights ({m['specks']:.2f}), the cloud behind ({m['cloud']:.2f}), {n - n0} draw calls more ({n})")
            # it stops: the cloud it trails catches up and hangs in the near part of the beam
            await ev("S47.world.autopilot = null; S47.world.testInput = { steer: 0, throttle: -1 }")
            for _ in range(10):
                await ev("S47.tick(0.25, 30)")
                if await ev("Math.abs(S47.world.legs[S47.world.leg].drive.speed) < 0.3"): break
            await ev("S47.world.testInput = { steer: 0, throttle: 0 }; S47.tick(0.6, 30)")
            await pg.screenshot(path=f'{OUT}/track_stop.png')
            st = await ev(LIFE)
            await offframe()
            await pg.screenshot(path=f'{OUT}/track_stop_off.png')
            pair(f'{OUT}/track_stop.png', f'{OUT}/track_stop_off.png', f'{OUT}/track_stop_pair.png')
            check(st['motes']['cloud'] > 0.3, f"stopped: the dust cloud catches up ({st['motes']['cloud']:.2f})")
            await ev("S47.world.testInput = null")

        print('draw calls:', json.dumps(calls))
        json.dump(calls, open(f'{OUT}/calls.json', 'w'), indent=1)
        if has:
            # with the rest of the round in the same views: over main before it
            more = {k: calls[k] - v for k, v in BEFORE_ROUND.items() if k in calls}
            check(len(more) == len(BEFORE_ROUND) and all(v <= SHARED for v in more.values()), f'every view at most {SHARED} draw calls over main before the round, its five parts together: {more}')
        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        await b.close()
    ok = sum(1 for r in results if r[0])
    json.dump([{'ok': r[0], 'msg': r[1]} for r in results], open(f'{OUT}/env_atmosphere.json', 'w'), indent=1)
    print(f"{ok} of {len(results)} PASS")
    sys.exit(0 if ok == len(results) else 1)

asyncio.run(main())
