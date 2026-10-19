# Light cookies (Tom, 19 October: «Lys-cookies først: lommelykta, frontlysene og vinduene i
# Caliche»): the texture a light throws, on the three lights the player looks at most.
#   the flashlight (core/Flashlight.ts, core/cookies.ts): the uneven beam of a cheap flashlight
#     (a hot core a little off the middle, faint rings from the reflector, the filament's dark
#     spot, a ragged edge, a little colour at the rim), worked out in three.js's own loop over the
#     spot lights where the beam falls (no SpotLight.map: that costs every lit pixel). It is a real
#     light on every tier, so Low, High and Ultra show the same picture.
#   the headlights (kit.floodlit, Truck.headlightFloods): the three pools of the driven truck in
#     an area's floods are drawn through two sealed-beam lamps' pattern (a wide flat spread, a
#     brighter band just under the horizon, little light above it, faint streaks from the flutes),
#     also in the copies of them in the diner's, the town's and the motel's floods.
#   the windows (world/windowLight.ts): the lit windows of Caliche and the motel's office and
#     room 6 lay their panes, bars, blinds or curtain on the ground in front of them, one mesh and
#     one draw call per area, off with the window, weaker where a street lamp already lights it.
# Shoots fixed views on High (OUTDIR/<view>.png): the flashlight on a wall and on the ground at
# SARO and in the records corridor, the headlights on the highway, on the survey track and on the
# church's white front, Caliche from the sidewalk and from the truck, the gas station, the motel's
# office, SARO's windows side, the control room and the old road; Low and Ultra for a few. With
# the cookies in the build, also the same frame with them off beside it (<view>_pair.png, off
# left), and checks: the pattern is there on Low, High and Ultra (what the cookie changes in the
# beam, the hot core and the rings in the flashlight's pool seen straight down, every lit shader
# with it, the cut above the headlights' horizon, the panes on the sidewalk), the windows' light
# goes with the window (the studio after four, the sheriff's office while he is out, the first
# television after twenty to four) and comes up when the street lamps go out (event 8), the draw
# calls (at most two more than with them off at any view, Low no more than High, SARO outside
# within grounds.py's 340, the control room within artcheck's 300, the old road within
# chapter6.py's 90) and what a frame costs on Low with them and with none (the round pools, no
# flashlight pattern, no window light). Also writes the windows' picture (cookie_windows.png).
# --before: only the views and their draw calls (a build without the cookies), for comparing.
# --compare=DIR: a --before run's OUTDIR: the draw calls against it, and a side by side of each
# view before and after (<view>_ba.png, before left).
# Usage: python3 tools/cookies.py OUTDIR [WxH] [--before] [--compare=DIR] [--only=VIEW,...] [--cost]
import asyncio, sys, os, json, math
from playwright.async_api import async_playwright
from PIL import Image, ImageStat, ImageChops, ImageFilter

args = [a for a in sys.argv[1:] if not a.startswith('--')]
BEFORE = '--before' in sys.argv
COMPARE = next((a.split('=', 1)[1] for a in sys.argv[1:] if a.startswith('--compare=')), None)
# --only=c1,h3: just these views on High, their pictures and pairs, no checks (for looking)
ONLY = next((a.split('=', 1)[1].split(',') for a in sys.argv[1:] if a.startswith('--only=')), None)
# --cost: only what a frame costs on Low (run it with nothing else on the graphics card: gslot.sh --alone)
COST = '--cost' in sys.argv
OUT = args[0]
W, H = (int(v) for v in (args[1] if len(args) > 1 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
X = 8000                                    # the road area's origin (World.ROAD_ORIGIN)
NIGHT = 2 * 3600 + 20 * 60                  # 02:20: the deputy in, the Night Line on the air, both televisions on
results = []
def check(ok, msg):
    results.append((bool(ok), msg)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

def gray(path, box=None):
    im = Image.open(path).convert('L')
    return im.crop(box) if box else im
def lum(path, box=None): return ImageStat.Stat(gray(path, box)).mean[0]
def changed(a, b, box=None, thr=6):
    """How many pixels differ by more than thr (of 255) between two pictures."""
    return ImageChops.difference(gray(a, box), gray(b, box)).point(lambda v: 255 if v > thr else 0).histogram()[255]
def brighter(a, b, box=None, thr=6):
    """How many pixels of a are brighter than in b by more than thr."""
    return sum(ImageChops.subtract(gray(a, box), gray(b, box)).histogram()[thr + 1:])
def pair(a, b, out):
    A, B = Image.open(a), Image.open(b)
    o = Image.new('RGB', (A.width * 2 + 6, A.height), (200, 0, 0))
    o.paste(A, (0, 0)); o.paste(B, (A.width + 6, 0)); o.save(out)
def profile(path, mask):
    """The pool of light seen straight down: its middle (where the pool of `mask`, the same frame
    without the cookie, is), the mean light at each distance from it out to the pool's edge, and
    from that the core (the brightest of the inner fifth against the flood at half way) and the
    rings (how far the profile goes up and down round its own running mean, a part of the flood)."""
    g = gray(path).filter(ImageFilter.GaussianBlur(2)); m = gray(mask)
    w, h = g.size; px, mx = g.load(), m.load()
    pts = [(x, y) for y in range(0, h, 4) for x in range(0, w, 4) if mx[x, y] > 90]
    if len(pts) < 50: return 0, 0
    cx, cy = sum(p[0] for p in pts) / len(pts), sum(p[1] for p in pts) / len(pts)
    R = int(math.sqrt(len(pts) * 16 / math.pi))
    prof = []
    for r in range(0, R, 2):
        n = max(12, int(r * 0.6)); s = 0; k = 0
        for i in range(n):
            x, y = int(cx + r * math.cos(i * 2 * math.pi / n)), int(cy + r * math.sin(i * 2 * math.pi / n))
            if 0 <= x < w and 0 <= y < h: s += px[x, y]; k += 1
        prof.append(s / max(1, k))
    if len(prof) < 12: return 0, 0
    flood = prof[len(prof) // 2]
    core = max(prof[:max(1, len(prof) // 5)]) / max(1, flood)
    run = [sum(prof[max(0, i - 4):i + 5]) / len(prof[max(0, i - 4):i + 5]) for i in range(len(prof))]
    inner = range(len(prof) // 5, int(len(prof) * 0.85))
    rings = math.sqrt(sum((prof[i] - run[i]) ** 2 for i in inner) / len(inner)) / max(1, flood)
    return core, rings

def spread(path, mask, box, at=60):
    """How uneven the light is inside the old disc's pool (the pixels of `mask` brighter than at):
    the difference between the 85th and the 25th percentile of the luminance there, a little
    blurred (the hot core against the flood round it, not the grain of the ground)."""
    g, m = gray(path, box).filter(ImageFilter.GaussianBlur(2)), gray(mask, box)
    v = sorted(a for a, b in zip(g.getdata(), m.getdata()) if b > at)
    return (v[int(len(v) * 0.85)] - v[int(len(v) * 0.25)]) if len(v) > 100 else 0

# The fixed views: (name, kind, x, z, yaw, pitch, light). kind: 'saro' on foot at SARO (its own
# terms), 'town' on foot in the road area (x road-local), 'truck' and 'track' driving the road's
# truck (x, z road-local, yaw its heading), 'oldroad' the old road's truck at its start, 'motel'
# on foot at the motel (x, z from its corner, MotelFront.at).
VIEWS = [
    ('f1_flashlight_wall', 'saro', 30.0, -9.2, math.pi, -0.08, True),        # the east wing's north wall, 3.7 m off
    ('f2_flashlight_ground', 'saro', -14.0, 50.0, 0.15, -0.55, True),        # the dark of the west lot, at the feet
    ('f3_flashlight_corridor', 'saro', 1.4, 5.7, math.pi / 2, -0.12, True),  # down the records corridor
    ('f4_flashlight_down', 'saro', -14.0, 50.0, 0.15, -1.5, True),          # straight down at the ground of the west lot (the profile)
    ('h1_headlights_highway', 'truck', -1.8, 1640.0, math.pi, 0.0, False),   # the highway north of Caliche, heading south
    ('h2_headlights_track', 'track', -262.0, 495.6, math.pi / 2 + 0.03, 0.0, False),   # the survey track
    ('h3_headlights_wall', 'truck', 8.6, 2095.5, -math.pi / 2, 0.0, False),  # the church's white front, 12 m ahead
    ('c1_caliche_sidewalk', 'town', -9.0, 2124.0, 2.43, -0.25, False),       # the laundromat's windows and the sidewalk under them
    ('c2_caliche_truck', 'truck', -2.2, 2088.0, math.pi - 0.22, 0.0, False),  # down the main street from the truck
    ('c3_caliche_gas', 'town', -8.0, 2079.0, 0.98, -0.25, False),            # the gas station's office window on its apron
    ('c4_caliche_east', 'town', 14.5, 2031.0, -2.42, -0.25, False),          # the studio's window, the Night Line on the air
    ('m1_motel_office', 'motel', -35.5, 30.0, 0.97, -0.25, False),           # the motel's office window, from the lot
    ('s1_saro_windows', 'saro', 8.0, -9.0, 1.98, -0.25, False),              # the control room's windows from the slab's corner
    ('s2_control_room', 'saro', 0.0, -1.0, 0.0, 0.0, False),                 # the control room (its budget)
    ('o1_old_road', 'oldroad', 0, 0, 0, 0.0, False),                         # the old road in the headlights (its budget)
]
LOWULTRA = ['f1_flashlight_wall', 'f2_flashlight_ground', 'f4_flashlight_down', 'h1_headlights_highway', 'h3_headlights_wall', 'c1_caliche_sidewalk']
# where in the picture each light's pattern lies (fractions of the picture): the beam, the road ahead, the sidewalk
CROP = {
    'f1_flashlight_wall': (0.25, 0.2, 0.85, 0.95), 'f2_flashlight_ground': (0.2, 0.25, 0.9, 1.0), 'f3_flashlight_corridor': (0.25, 0.2, 0.85, 0.95), 'f4_flashlight_down': (0.0, 0.0, 1.0, 1.0),
    'h1_headlights_highway': (0.0, 0.45, 1.0, 0.85), 'h2_headlights_track': (0.0, 0.45, 1.0, 0.85), 'h3_headlights_wall': (0.0, 0.2, 1.0, 0.85),
    'c1_caliche_sidewalk': (0.0, 0.45, 1.0, 1.0), 'c2_caliche_truck': (0.0, 0.45, 1.0, 0.9), 'c3_caliche_gas': (0.0, 0.45, 1.0, 1.0),
    'c4_caliche_east': (0.0, 0.45, 1.0, 1.0), 'm1_motel_office': (0.0, 0.45, 1.0, 1.0),
}
def box_of(name): c = CROP[name]; return (int(W * c[0]), int(H * c[1]), int(W * c[2]), int(H * c[3]))

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
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(400)
        ev = pg.evaluate
        # the pictures without the HUD, the toasts and the captions: the night itself
        await pg.add_style_tag(content='#ui { visibility: hidden !important; }')
        await ev("S47.hold = true; S47.setAbductions(false); S47.jump('chapter1'); S47.tick(1.0)")
        await ev("S47.setPicture('off'); S47.setQuality('high'); S47.tick(0.1)")
        # nothing strange tonight in these pictures: no flicker in the flashlight, no event in town
        await ev("S47.flashlight.flicker = () => {}; S47.flashlight.have = true")
        await ev("S47.world.prepare('diner')")
        for _ in range(150):
            if await ev("!!S47.world.town && !!S47.world.diner && !!S47.world.road"): break
            await pg.wait_for_timeout(200)
        has = (not BEFORE) and bool(await ev("typeof S47.cookies === 'function'"))
        print('the cookies are ' + ('in the build' if has else 'not in the build (only the views)'), flush=True)
        # the life's car and tumbleweed come when asked, not by chance between two frames
        await ev("if (S47.world.life) { S47.world.life['carT'] = 1e9; S47.world.life['weedT'] = 1e9; }")

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
        motel = await ev("S47.world.motel.at")

        async def go(v, clock=NIGHT):
            name, kind, x, z, yaw, pitch, light = v
            await ev(f"S47.game.clock = {clock}")
            await ev(f"S47.flashlight.on = {'true' if light else 'false'}")
            if kind in ('saro', 'town', 'motel'):
                await stop_driving()
                if kind == 'motel': x, z = motel['x'] + x, motel['z'] + z
                if kind == 'town': x += X
                await ev(f"S47.world.enter('{'diner' if kind == 'town' else 'saro'}'); S47.player.place({x}, {z}, {yaw}); S47.player.pitch = {pitch}; S47.tick(0.6)")
            elif kind in ('truck', 'track'):
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

        AGAIN = "(() => { S47.renderer.info.reset(); S47.vhs.render(S47.scene, S47.camera, performance.now() / 1000); return S47.renderer.info.render.calls; })()"
        async def shoot(v, q='high'):
            """The view: its picture and draw calls; with the cookies, the same frame without them too."""
            tag = v[0] + ('' if q == 'high' else '_' + q)
            if has: await ev("S47.cookies(true)")
            await ev(f"S47.setQuality('{q}')")
            await go(v)
            await ev("S47.tick(0.1)")
            n = await calls()
            await pg.screenshot(path=f'{OUT}/{tag}.png')
            if not has: return n, None
            # (the draw calls of the same frame drawn again with them and without: a tick's frame
            # also draws the shadow maps due then, on Ultra)
            n1 = await ev(AGAIN)
            await ev("S47.cookies(false)")
            n0 = await ev(AGAIN)
            again[tag] = n1
            await pg.screenshot(path=f'{OUT}/{tag}_off.png')
            await ev("S47.cookies(true)")
            pair(f'{OUT}/{tag}_off.png', f'{OUT}/{tag}.png', f'{OUT}/{tag}_pair.png')
            return n, n0

        calls_on, calls_off, again = {}, {}, {}
        async def frame_cost(names, q='low', cycles=8):
            # with them, and as the night was before them: the flashlight's plain disc (its decay
            # off the cookie's mark), the round headlight pools, no window light; the
            # same frame drawn 60 times, `cycles` times each way in turn, the median
            cost = {}
            for name in names:
                v = next(x for x in VIEWS if x[0] == name)
                await ev(f"S47.setQuality('{q}')"); await go(v); await ev("S47.tick(0.2)")
                cost[name] = await ev("""(async (cycles) => { const gl = S47.renderer.getContext(), px = new Uint8Array(4), r = { on: [], off: [] };
                  const run = (n) => { const t0 = performance.now(); for (let i = 0; i < n; i++) S47.vhs.render(S47.scene, S47.camera, i / 60); gl.readPixels(0, 0, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, px); return (performance.now() - t0) / n; };
                  const set = (on) => { S47.cookies(on); };
                  set(false); run(10); set(true); run(10);
                  for (let k = 0; k < cycles; k++) for (const on of [true, false]) { set(on); run(3); r[on ? 'on' : 'off'].push(run(60)); }
                  set(true); const med = (a) => a.sort((x, y) => x - y)[a.length >> 1]; return { on: med(r.on), off: med(r.off) }; })""", cycles)
            await ev("S47.setQuality('high')")
            return cost
        if COST:
            for q in ('low', 'high'):
                c = await frame_cost(['f2_flashlight_ground', 'f4_flashlight_down', 'h1_headlights_highway', 'h2_headlights_track', 'c1_caliche_sidewalk', 'c2_caliche_truck'], q, 16)
                print(f"{q}, ms per frame (median of 16 runs of 60, with the cookies and without any):", flush=True)
                for k, x in c.items(): print(f"  {k}: {x['on']:.3f} with, {x['off']:.3f} without ({(x['on'] / x['off'] - 1) * 100:+.1f} %)", flush=True)
            await b.close(); return
        if ONLY:
            for v in VIEWS:
                if any(v[0].startswith(o) for o in ONLY): await shoot(v)
            await b.close(); return
        for v in VIEWS:
            n, n0 = await shoot(v)
            calls_on[v[0]] = n; calls_off[v[0]] = n0
            print(f"{v[0]}: {n} draw calls on High" + (f" ({n0} with the cookies off)" if n0 is not None else ''), flush=True)
        for q in ('low', 'ultra'):
            for v in VIEWS:
                if v[0] not in LOWULTRA: continue
                n, n0 = await shoot(v, q)
                calls_on[f'{v[0]}_{q}'] = n; calls_off[f'{v[0]}_{q}'] = n0
                print(f"{v[0]}: {n} draw calls on {q}" + (f" ({n0} with the cookies off)" if n0 is not None else ''), flush=True)
        await ev("S47.setQuality('high'); S47.tick(0.1)")
        json.dump({'on': calls_on, 'off': calls_off}, open(f'{OUT}/calls.json', 'w'), indent=1)

        if COMPARE:
            for v in VIEWS:
                for q in ('', '_low', '_ultra'):
                    a, bb = f'{COMPARE}/{v[0]}{q}.png', f'{OUT}/{v[0]}{q}.png'
                    if os.path.exists(a) and os.path.exists(bb): pair(a, bb, f'{OUT}/{v[0]}{q}_ba.png')
            before = json.load(open(f'{COMPARE}/calls.json'))['on']
            more = {k: (calls_on[k], before[k]) for k in calls_on if k in before and calls_on[k] > before[k] + 2}
            check(not more, f"at most two draw calls more than before the cookies at every view ({ {k: calls_on[k] - before[k] for k in calls_on if k in before} } more)")

        # ---------- the budgets ----------
        saro = [calls_on[k] for k in ('f1_flashlight_wall', 'f2_flashlight_ground', 's1_saro_windows')]
        check(max(saro) <= 340, f"SARO outside keeps grounds.py's budget of 340 ({saro})")
        check(calls_on['s2_control_room'] <= 300, f"the control room keeps artcheck's 300 ({calls_on['s2_control_room']})")
        check(calls_on['o1_old_road'] <= 90, f"the old road keeps chapter6.py's 90 ({calls_on['o1_old_road']})")
        lowmore = {k: (calls_on[k + '_low'], calls_on[k]) for k in LOWULTRA if calls_on[k + '_low'] > calls_on[k]}
        check(not lowmore, f"Low draws no more than High at the views of both ({lowmore or 'none more'})")
        if not has:
            print('\n'.join(errs[:20]) or 'no console errors/warnings')
            await b.close()
            fails = [m for ok, m in results if not ok]
            print(f"\n{len(results) - len(fails)}/{len(results)} PASS (views only)")
            sys.exit(1 if fails else 0)

        more = {k: (again[k], calls_off[k]) for k in again if again[k] > calls_off[k] + 2}
        check(not more, f"the cookies cost at most two draw calls at any view, the same frame with them and without ({ {k: again[k] - calls_off[k] for k in again} } more with them)")

        # ---------- the flashlight ----------
        # the pictures themselves (OUTDIR/cookie_*.png)
        import base64
        for name, js in (('windows', "S47.world.town.windowLights.mesh.material.uniforms.uMap.value.image.toDataURL()"),):
            url = await ev(js)
            open(f'{OUT}/cookie_{name}.png', 'wb').write(base64.b64decode(url.split(',', 1)[1]))
        st = await ev("S47.cookieStats()")
        print('stats:', json.dumps(st), flush=True)
        check(st['flashlight']['cookie'] and not st['flashlight']['map'], f"the flashlight throws its cookie in the shaders, with no SpotLight.map ({st['flashlight']})")
        for q in ('', '_low', '_ultra'):
            tag = 'f1_flashlight_wall' + q
            n = changed(f'{OUT}/{tag}.png', f'{OUT}/{tag}_off.png', box_of('f1_flashlight_wall'))
            check(n > 3000, f"{q[1:] or 'high'}: the flashlight's pool on the wall is the cookie's ({n} pixels differ)")
        for q in ('', '_low', '_ultra'):
            tag = 'f2_flashlight_ground' + q
            n = changed(f'{OUT}/{tag}.png', f'{OUT}/{tag}_off.png', box_of('f2_flashlight_ground'))
            check(n > 3000, f"{q[1:] or 'high'}: and on the ground ({n} pixels differ)")
        # straight down: a hot core in a dimmer flood with rings in it, not one even disc
        for q in ('', '_low', '_ultra'):
            tag = 'f4_flashlight_down' + q
            on, off = f'{OUT}/{tag}.png', f'{OUT}/{tag}_off.png'
            (c1, r1), (c0, r0) = profile(on, off), profile(off, off)
            print(f"{q[1:] or 'high'}: the pool straight down: core {c1:.2f} and rings {r1:.4f} against {c0:.2f} and {r0:.4f} without", flush=True)
            check(c1 > c0 + 0.05 and r1 > r0 * 2, f"{q[1:] or 'high'}: straight down, a hot core over the flood ({c1:.2f} against {c0:.2f} without) and rings in it ({r1:.4f} against {r0:.4f})")
        n = changed(f'{OUT}/f3_flashlight_corridor.png', f'{OUT}/f3_flashlight_corridor_off.png', box_of('f3_flashlight_corridor'))
        check(n > 1500, f"and down the corridor ({n} pixels differ)")
        # the programs: Low's Lambert copies draw it as High's standard ones do, and none pays for a
        # spot light map (three.js writes the number of maps into the shader's text: spotLightMap[ 1 ])
        progs = await ev("""(() => { const gl = S47.renderer.getContext(), o = { lam: 0, lamC: 0, std: 0, stdC: 0, maps: 0 };
          for (const p of S47.renderer.info.programs) { const s = gl.getShaderSource(p.fragmentShader) || '';
            const lam = s.includes('#define LAMBERT'), std = s.includes('#define STANDARD'), c = s.includes('s47FlashCookie( spotLight');
            if (/spotLightMap\\[ [1-9]/.test(s)) o.maps++;
            if (lam) { o.lam++; if (c) o.lamC++; } else if (std) { o.std++; if (c) o.stdC++; } }
          return o; })()""")
        check(progs['lam'] > 0 and progs['lamC'] == progs['lam'] and progs['std'] > 0 and progs['stdC'] == progs['std'] and progs['maps'] == 0,
              f"every lit program draws the flashlight's cookie: Low {progs['lamC']} of {progs['lam']}, High and Ultra {progs['stdC']} of {progs['std']}; with a spot light map {progs['maps']}")

        # ---------- the headlights ----------
        check(len(st['head']) >= 4, f"the headlight pools are drawn through the sealed beams in {len(st['head'])} flood sets ({', '.join(st['head'])})")
        for q in ('', '_low', '_ultra'):
            tag = 'h1_headlights_highway' + q
            n = changed(f'{OUT}/{tag}.png', f'{OUT}/{tag}_off.png', box_of('h1_headlights_highway'))
            check(n > 3000, f"{q[1:] or 'high'}: the headlights' pattern on the highway ({n} pixels differ)")
        n = changed(f'{OUT}/h2_headlights_track.png', f'{OUT}/h2_headlights_track_off.png', box_of('h2_headlights_track'))
        check(n > 3000, f"and on the survey track ({n} pixels differ)")
        # on the church's front: little above the lamps' horizon, the band just under it
        for q in ('', '_low', '_ultra'):
            tag = 'h3_headlights_wall' + q
            on, off = f'{OUT}/{tag}.png', f'{OUT}/{tag}_off.png'
            top = (int(W * 0.3), int(H * 0.2), int(W * 0.7), int(H * 0.4))
            a, z = lum(on, top), lum(off, top)
            n = changed(on, off, box_of('h3_headlights_wall'))
            check(n > 3000 and a < z, f"{q[1:] or 'high'}: on a wall the beams cut off above the horizon ({a:.1f} against {z:.1f} without, {n} pixels differ)")
        n = changed(f'{OUT}/c2_caliche_truck.png', f'{OUT}/c2_caliche_truck_off.png', box_of('c2_caliche_truck'))
        check(n > 1500, f"and in Caliche's own floods, from the truck ({n} pixels differ)")

        # ---------- the windows ----------
        check(st['windows'].get('town', 0) >= 8 and st['windows'].get('motel', 0) >= 2, f"the windows that lay their light on the ground: {st['windows']}")
        for q in ('', '_low', '_ultra'):
            tag = 'c1_caliche_sidewalk' + q
            n = brighter(f'{OUT}/{tag}.png', f'{OUT}/{tag}_off.png', box_of('c1_caliche_sidewalk'), 3)
            check(n > 2000, f"{q[1:] or 'high'}: the laundromat's windows on the sidewalk ({n} pixels brighter with them)")
        for v in ('c3_caliche_gas', 'c4_caliche_east', 'm1_motel_office'):
            n = brighter(f'{OUT}/{v}.png', f'{OUT}/{v}_off.png', box_of(v), 3)
            check(n > 1500, f"{v}: the window's light on the ground ({n} pixels brighter with it)")
        # off with the window: the studio after four (the Night Line off the air), the sheriff's while he is out
        # (the town's night runs while the player is in its area)
        await stop_driving()
        await ev(f"S47.world.enter('diner'); S47.player.place({X - 9}, 2124, 2.43); S47.tick(0.2)")
        lv = await ev(f"""(() => {{ const T = S47.world.town, out = {{}};
          for (const [k, c] of [['night', {NIGHT}], ['late', 3 * 3600 + 20 * 60], ['four', 4 * 3600 + 10 * 60]]) {{ S47.game.clock = c; S47.tick(0.1); out[k] = T.windowLights.levels(); }}
          S47.game.clock = {NIGHT}; S47.tick(0.1); return out; }})()""")
        print('window levels:', json.dumps(lv), flush=True)
        check(lv['night']['studio'] > 0.5 and lv['four']['studio'] < lv['night']['studio'] * 0.5, f"the studio's light on the street dims when the Night Line goes off the air ({lv['night']['studio']:.2f} to {lv['four']['studio']:.2f})")
        check(lv['night']['sheriffWin'] > 0.5 and lv['late']['sheriffWin'] == 0, f"the sheriff's window lays none while he is out ({lv['night']['sheriffWin']:.2f} to {lv['late']['sheriffWin']:.2f})")
        check(lv['night']['tv1'] > 0 and lv['four']['tv1'] == 0, f"nor the first television's after twenty to four ({lv['four']['tv1']:.2f})")
        # weaker under a street lamp: the lamps out (event 8) and the sidewalk's patterns stronger
        dk = await ev("""(() => { const T = S47.world.town, a = T.windowLights.darkness(); T.lampWave(S47.runTime() - 12); S47.tick(0.1);
          const b = T.windowLights.darkness(); T.lampWave(-1e9); S47.tick(0.1); return [a, b]; })()""")
        check(dk[1] >= dk[0] + 0.02, f"the windows' light is held back where the street lamps light the ground, and comes up when they go out (event 8: {dk[0]:.2f} to {dk[1]:.2f})")

        # ---------- what a frame costs on Low ----------
        # (the graphics card, with other tests on it now and then: --cost alone is steadier; a
        # phone's own cost is UNVERIFIED)
        cost = await frame_cost(['f2_flashlight_ground', 'h1_headlights_highway', 'c1_caliche_sidewalk', 'c2_caliche_truck'])
        print('Low, ms per frame (median of eight runs of 60, with the cookies and without any; the graphics card, not a phone):', json.dumps(cost), flush=True)
        worst = max(c['on'] - c['off'] for c in cost.values())
        check(all(c['on'] <= c['off'] * 1.05 + 0.05 for c in cost.values()), f"no measurable cost on Low: at most {worst:+.3f} ms a frame with them ({ {k: f"{c['on'] - c['off']:+.3f} of {c['off']:.3f}" for k, c in cost.items()} })")

        await stop_driving()
        bad = [e for e in errs if 'favicon' not in e]
        check(not bad, 'no console errors or warnings' + (': ' + ' | '.join(bad[:6]) if bad else ''))
        await b.close()
    fails = [m for ok, m in results if not ok]
    print(f"\n{len(results) - len(fails)}/{len(results)} PASS")
    sys.exit(1 if fails else 0)

asyncio.run(main())
