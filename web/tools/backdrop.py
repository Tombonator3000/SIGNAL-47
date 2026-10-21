# The painted horizon (world/backdrop.ts, ART_BRIEF.md round 16): one band of mesas, buttes and a
# far ridge round the whole map, on a cylinder that follows the camera, night and dawn crossfaded
# with the sky. Shoots the same views every time, looking at the horizon in each area at night and
# at dawn (SARO west, east, south and north, the highway from the truck, Caliche's street, the
# diner's lot, STATION 01 west and east, the old road at dawn both ways), and prints the draw calls
# of each. With --before DIR (an earlier run with --shots on main) it lays each view next to the
# one before (OUTDIR/<view>_pair.png) and holds the draw calls to it. Checks that the band is there
# in every view that shows the horizon (the picture with it against the same without it), that it
# is seamless where its ends meet (north: the columns across the join against those beside it, in
# the picture and in the texture), that it follows the sky's dawn and turns with the sky at
# STATION 01, that the far mesa rings it took over from are gone, what it costs (one draw call;
# SARO outside at most 340, the old road at most 90), its texture sizes on Ultra, High and Low,
# and no console errors. The art director's second pass (19 October): no pale line where the land
# ends under it at night (against the BEFORE shots), no fog bank under it at dawn on the old road,
# the mesas no lighter low on their bodies than at their tops, dark against the light towards the
# sun, the night horizon cold away from the lit places, the near mesas no lighter than the band,
# a sharp skyline, the sun's mesa in the band where the old road's ring of hills was, and the dip
# under a high camera.
# Usage: python3 tools/backdrop.py OUTDIR [WxH] [--shots] [--before DIR] [--low]
#   --shots: the views and their draw calls only, no checks (the BEFORE pictures on main)
#   --low: the views on Low as well
import asyncio, sys, json, os, math
from intro_flow import begin_opening
from playwright.async_api import async_playwright
from PIL import Image, ImageChops, ImageStat

args = [a for a in sys.argv[1:] if not a.startswith('--')]
SHOTS_ONLY = '--shots' in sys.argv
BEFORE_DIR = sys.argv[sys.argv.index('--before') + 1] if '--before' in sys.argv else None
if BEFORE_DIR in args: args.remove(BEFORE_DIR)
OUT = args[0]
W, H = (int(v) for v in (args[1] if len(args) > 1 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
X = 8000                                    # the road area's origin (World.ROAD_ORIGIN)
NIGHT = 1 * 3600 + 20 * 60                  # 01:20, the dead of the night
DAWN = 5 * 3600 + 20 * 60                   # 05:20, the grey of the dawn (SARO shows 70 per cent of it)
checks = []
def check(ok, what): checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

# (name, area, x, z, azimuth in degrees on the map (0 north, 90 east), pitch, clock). On foot in
# the area's own terms; 'truck' drives the road's truck (x, z road-local), 'oldroad' the old road's
# truck at a mile (x), turned round when the azimuth says west.
VIEWS = [
    ('b01_saro_west_night', 'saro', -14.0, 50.0, 262, 0.03, NIGHT),
    ('b02_saro_east_night', 'saro', -28.5, 150.0, 79, 0.03, NIGHT),
    ('b03_saro_south_highway', 'saro', -28.5, 92.0, 180, 0.03, NIGHT),
    ('b04_motel_north_join', 'saro', -20.0, 600.0, 0, 0.03, NIGHT),
    ('b05_highway_from_the_truck', 'truck', -1.8, 1180.0, 180, 0.0, NIGHT),
    ('b06_caliche_street_south', 'diner', X - 1.5, 2036.0, 180, 0.03, NIGHT),
    ('b07_diner_lot_north', 'diner', X + 3.0, 2013.0, 350, 0.03, NIGHT),
    ('b08_station_west', 'station01', 0, 0, 290, 0.03, NIGHT),
    ('b09_station_east', 'station01', 0, 0, 110, 0.03, NIGHT),
    ('b10_saro_east_dawn', 'saro', -28.5, 150.0, 79, 0.03, DAWN),
    ('b11_saro_west_dawn', 'saro', -14.0, 50.0, 262, 0.03, DAWN),
    ('b12_oldroad_east_dawn', 'oldroad', 4.0, 0, 70, 0.0, DAWN),
    ('b13_oldroad_west_dawn', 'oldroad', 4.0, 0, 250, 0.0, DAWN),
]
# (the views where the horizon shows: the band must change the picture there)
SHOWS = {v[0] for v in VIEWS}
# what a view may cost over the one before: the band is one draw call, and the rings it replaces one
EXTRA_CALLS = 1

def diff(a, b, box):
    A, B = Image.open(a).convert('RGB').crop(box), Image.open(b).convert('RGB').crop(box)
    return sum(ImageStat.Stat(ImageChops.difference(A, B)).mean) / 3

def pair(a, b, out):
    A, B = Image.open(a), Image.open(b)
    o = Image.new('RGB', (A.width, A.height * 2 + 6), (200, 0, 0))
    o.paste(A, (0, 0)); o.paste(B, (0, A.height + 6)); o.save(out)

HY = """(() => { const T = S47.three, c = S47.camera, d = new T.Vector3(); c.updateMatrixWorld(); c.getWorldDirection(d); d.y = 0; d.normalize();
  const p = c.position.clone().addScaledVector(d, 4000).project(c); return [(1 - p.y) / 2 * innerHeight, innerHeight / 2 / Math.tan(c.fov * Math.PI / 360)]; })()"""
# the views whose band is also shot alone (camera layer 5), for its body and skyline
ALONE = ('b01_saro_west_night', 'b10_saro_east_dawn', 'b11_saro_west_dawn', 'b13_oldroad_west_dawn')

def rowmean(px, y, x0, x1, ch=None):
    return sum((px[x, y] if ch is None else px[x, y][ch]) for x in range(x0, x1)) / (x1 - x0)

def edge_line(path, hy, cols=(0.1, 0.9)):
    # a pale line where the land ends under the band: the brightest row just under the horizon
    # against the band's foot just over it and the land a little nearer
    im = Image.open(path).convert('L'); w = im.width; px = im.load(); x0, x1 = int(w * cols[0]), int(w * cols[1]); hy = round(hy)
    line = max(rowmean(px, y, x0, x1) for y in range(hy - 1, hy + 7))
    above = sum(rowmean(px, y, x0, x1) for y in range(hy - 6, hy - 2)) / 4
    below = sum(rowmean(px, y, x0, x1) for y in range(hy + 12, hy + 18)) / 6
    return line - max(above, below), line, above, below

# (the band alone is shot against magenta: a pixel is the band's where it is far from magenta)
def mag(p): return min(p[0], p[2]) - p[1]
def lum(p): return 0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2]

def body_rise(path, hy, f):
    # the band alone: per column with a mesa over 1.2 degrees, the body at 0.45 to 0.75 degrees
    # (over the foot) against the body just under its top; the median of low minus high
    im = Image.open(path).convert('RGB'); w = im.width; px = im.load()
    lo0, lo1 = round(hy - math.tan(math.radians(0.75)) * f), round(hy - math.tan(math.radians(0.45)) * f)
    hi_edge = round(hy - math.tan(math.radians(1.2)) * f)
    out = []
    for x in range(0, w, 3):
        top = next((y for y in range(max(0, round(hy) - 160), round(hy)) if mag(px[x, y]) < 128), None)
        if top is None or top > hi_edge: continue
        up = sum(lum(px[x, y]) for y in range(top + 3, top + 7)) / 4
        lowv = sum(lum(px[x, y]) for y in range(lo0, lo1 + 1)) / (lo1 - lo0 + 1)
        out.append(lowv - up)
    out.sort()
    return (out[len(out) // 2] if out else None), len(out)

def skyline_width(path, hy):
    # the band alone: per column, how many rows the skyline takes from a fifth to four fifths of the way from magenta to the body
    im = Image.open(path).convert('RGB'); w = im.width; px = im.load(); out = []
    for x in range(0, w, 3):
        top = next((y for y in range(max(0, round(hy) - 160), round(hy)) if mag(px[x, y]) < 230), None)
        if top is None or top + 8 >= hy: continue
        body = mag(px[x, top + 6])
        if body > 60: continue
        out.append(sum(1 for y in range(top - 3, top + 6) if 0.2 < (255 - mag(px[x, y])) / (255 - body) < 0.8))
    out.sort()
    return (out[len(out) // 2] if out else None), len(out)

def seam(path, rows):
    # the step from one column to the next across the middle of the picture (where the join is when
    # the camera looks along it) against the steps beside it, in the band's rows
    im = Image.open(path).convert('L'); w = im.width; px = im.load()
    def step(x): return sum(abs(px[x + 1, y] - px[x, y]) for y in rows) / len(rows)
    mid = max(step(x) for x in range(w // 2 - 2, w // 2 + 2))
    around = sorted(step(x) for x in list(range(w // 2 - 120, w // 2 - 8)) + list(range(w // 2 + 8, w // 2 + 120)))
    return mid, around[len(around) // 2], around[int(len(around) * 0.98)]

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': W, 'height': H})).new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.add_init_script("addEventListener('DOMContentLoaded', () => { const s = document.createElement('style'); s.textContent = '.toasts, .lockhint { display: none !important; }'; document.head.appendChild(s); });")
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(400)
        ev = pg.evaluate
        await ev("S47.hold = true; S47.setAbductions(false); S47.jump('chapter1'); S47.tick(1.0)")
        await ev("S47.setPicture('off'); S47.setQuality('high'); S47.tick(0.1)")
        for area in ('station01', 'road', 'diner'):
            await ev(f"window.__p = S47.world.prepare('{area}')")
            for _ in range(200):
                if await ev(f"(() => {{ const w = S47.world; return '{area}' === 'station01' ? !!w.site : '{area}' === 'road' ? !!w.legs.road : !!w.town; }})()"): break
                await pg.wait_for_timeout(100)
        band = not SHOTS_ONLY and await ev("!!S47.backdrop")

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

        async def alone(path):
            # the band alone, against the scene's plain background (camera layer 5)
            await ev("document.getElementById('ui').style.visibility = 'hidden'; S47.backdrop.mesh.layers.enable(5); S47.camera.layers.set(5); window.__bg = S47.scene.background; S47.scene.background = new S47.three.Color(1, 0, 1); S47.tick(0)")
            await pg.screenshot(path=path)
            await ev("document.getElementById('ui').style.visibility = ''; S47.camera.layers.set(0); S47.backdrop.mesh.layers.disable(5); S47.scene.background = window.__bg; S47.tick(0)")

        async def go(v):
            name, area, x, z, az, pitch, clock = v
            await ev(f"S47.game.clock = {clock}")
            if area in ('saro', 'diner', 'station01'):
                await stop_driving()
                # the yaw that looks along the map's azimuth in this area (the station's terms are turned: World.skyYaw)
                pos = "(() => { const a = S47.world.site.anchors.arrive; return [a.x, a.z]; })()" if area == 'station01' else f"[{x}, {z}]"
                await ev(f"(() => {{ S47.world.enter('{area}'); const p = {pos}; S47.player.place(p[0], p[1], -{az} * Math.PI / 180 + S47.world.skyYaw); S47.player.pitch = {pitch}; }})()")
                await ev("S47.tick(0.6)")
            elif area == 'truck':
                if not await ev("S47.world.free === 'road' && S47.world.driving"):
                    await stop_driving()
                    await ev("S47.world.freeDrive('road')")
                    await until("S47.world.driving && S47.world.free === 'road'")
                await ev(f"(() => {{ const w = S47.world, L = w.legs.road; L.drive.place(new S47.three.Vector3({X + x}, L.area.height({X + x}, {z}), {z}), -{az} * Math.PI / 180); w['leg'] = 'road'; w.enter('road'); }})()")
                await ev("S47.tick(0.6)")
            else:
                if not await ev("S47.world.free === 'oldroad' && S47.world.driving"):
                    await stop_driving()
                    await ev("S47.world.freeDrive('oldroad')")
                    await until("S47.world.driving && S47.world.free === 'oldroad'", 240)
                # at a mile, in the right lane; turned round to look back west
                back = 'Math.PI' if 180 < az < 360 else '0'
                await ev(f"(() => {{ const w = S47.world, L = w.legs.roswell, p = w.oldRoad.poseAt({x}); L.drive.place(p.pos, p.heading + {back}); }})()")
                await ev("S47.tick(0.6)")

        high, low, shown, seams, hz = {}, {}, {}, {}, {}
        for v in VIEWS:
            await go(v)
            await pg.screenshot(path=f'{OUT}/{v[0]}.png')
            high[v[0]] = await calls()
            hz[v[0]] = await ev(HY)
            if band and v[0] in ALONE: await alone(f'{OUT}/{v[0]}_alone.png')
            line = f'{v[0]}: {high[v[0]]} draw calls on High'
            if band:
                # the same picture without the band: where the horizon shows, the band changes it
                await ev("S47.backdrop.mesh.visible = false; S47.backdrop.hold = true; S47.tick(0.02)")
                await pg.screenshot(path=f'{OUT}/{v[0]}_noband.png')
                await ev("S47.backdrop.hold = false; S47.tick(0.02)")
                # the strip round the horizon: the middle of the picture, a sixth of it high
                shown[v[0]] = diff(f'{OUT}/{v[0]}.png', f'{OUT}/{v[0]}_noband.png', (0, int(H * 0.36), W, int(H * 0.55)))
                line += f', the band changes the strip at the horizon by {shown[v[0]]:.2f} of 255'
            print(line, flush=True)
            if BEFORE_DIR and os.path.exists(f'{BEFORE_DIR}/{v[0]}.png'):
                pair(f'{BEFORE_DIR}/{v[0]}.png', f'{OUT}/{v[0]}.png', f'{OUT}/{v[0]}_pair.png')
        if '--low' in sys.argv:
            await ev("S47.setQuality('low'); S47.tick(0.1)")
            for v in VIEWS:
                await go(v)
                await pg.screenshot(path=f'{OUT}/{v[0]}_low.png')
                low[v[0]] = await calls()
            await ev("S47.setQuality('high'); S47.tick(0.1)")
        await stop_driving()
        json.dump({'high': high, 'low': low, 'size': [W, H], 'horizon': hz}, open(f'{OUT}/backdrop_calls.json', 'w'), indent=1)

        if not SHOTS_ONLY:
            check(band, 'the painted band is in the game (S47.backdrop)')
            if band:
                for k, d in shown.items():
                    check(d > 0.25, f'{k}: the band shows at the horizon ({d:.2f} of 255 changed in the strip)')
                # the join, north: in the picture (the camera looking along it) and in the texture itself
                info = await ev("S47.backdrop.info()")
                print('band:', json.dumps(info), flush=True)
                check(info['seam'] < max(6, 2.5 * info['seamAround']), f"the texture's ends meet: {info['seam']:.1f} across the join against {info['seamAround']:.1f} between neighbouring columns")
                for area, x, z, tag in (('saro', -20.0, 600.0, 'saro'), ('station01', 0, 0, 'station')):
                    for clock, when in ((NIGHT, 'night'), (DAWN, 'dawn')):
                        await go((f'j_{tag}_{when}', area, x, z, 0, 0.0, clock))
                        # the band alone, against the scene's plain background (camera layer 5)
                        await ev("document.getElementById('ui').style.visibility = 'hidden'; S47.backdrop.mesh.layers.enable(5); S47.camera.layers.set(5); S47.tick(0)")
                        path = f'{OUT}/join_{tag}_{when}.png'
                        await pg.screenshot(path=path)
                        await ev("document.getElementById('ui').style.visibility = ''; S47.camera.layers.set(0); S47.backdrop.mesh.layers.disable(5); S47.tick(0)")
                        rows = range(int(H * 0.40), int(H * 0.52))
                        mid, med, p98 = seam(path, rows)
                        seams[f'{tag}_{when}'] = (mid, med, p98)
                        check(mid <= max(4.0, p98 * 1.5), f'{tag} at {when}: no seam where the band\'s ends meet ({mid:.1f} across the join, {med:.1f} typical, {p98:.1f} at most beside it)')
                # the dawn: the band follows the sky's
                for clock in (NIGHT, DAWN, 5 * 3600 + 40 * 60):
                    await go(('d', 'saro', -14.0, 50.0, 262, 0.03, clock))
                    d = await ev("[S47.sky.uniforms.uDawn.value, S47.backdrop.uniforms.uDawn.value]")
                    check(abs(d[0] - d[1]) < 1e-6, f'at {clock // 3600:02d}:{clock % 3600 // 60:02d} the band crossfades with the sky (sky {d[0]:.2f}, band {d[1]:.2f})')
                # turned with the sky at STATION 01, and kept round the camera
                await go(('t', 'station01', 0, 0, 0, 0.0, NIGHT))
                r = await ev("[S47.backdrop.mesh.parent === S47.sky.group, S47.sky.group.rotation.y, S47.world.skyYaw, S47.sky.group.position.distanceTo(S47.camera.position)]")
                check(r[0] and abs(r[1] - r[2]) < 1e-6 and r[3] < 0.01, f'at STATION 01 the band turns with the sky ({r[1]:.3f} rad) and stays round the camera')
                # the far rings it replaced are gone; the near mesas by STATION 01 stay
                # (SARO's and the road's 'horizon' groups hold only the near mesas now, STATION 01 its own)
                rings = await ev("(() => { const out = []; S47.scene.traverse((o) => { if (o.name === 'horizon') out.push(o.children.map((c) => c.name)); }); let st = 0; S47.world.site.group.traverse((o) => { if (o.name === 'mesas') st++; }); return [out, st]; })()")
                check(len(rings[0]) == 2 and all(r == ['mesas:near'] for r in rings[0]) and rings[1] == 1, f'the far mesa rings are gone and the near mesas stay (SARO and the road: {rings[0]}, STATION 01: {rings[1]})')
                far = await ev("!!S47.world.oldRoad && S47.world.oldRoad.far === undefined && !S47.world.oldRoad.group.getObjectByProperty('renderOrder', -5)")
                check(far, "the old road's ring of hills is gone: the sun's mesa is the band's")

                # ---------- the art director's second pass ----------
                # no pale line where the land ends under it at night: at most 60 per cent of the one before, or a trace
                for k in ('b01_saro_west_night', 'b02_saro_east_night', 'b07_diner_lot_north', 'b09_station_east'):
                    hy = hz[k][0]
                    ex, ln, ab, bl = edge_line(f'{OUT}/{k}.png', hy)
                    msg = f'{k}: the land\'s far edge {ln:.0f} against the foot over it {ab:.0f} and the land under it {bl:.0f} (a line of {ex:+.1f})'
                    if BEFORE_DIR and os.path.exists(f'{BEFORE_DIR}/{k}.png'):
                        ex0 = edge_line(f'{BEFORE_DIR}/{k}.png', hy)[0]
                        check(ex <= max(4.0, 0.6 * ex0), msg + f', {ex0:+.1f} before')
                    else:
                        check(ex <= 12, msg)
                # at dawn on the old road: no fog bank under them; towards the sun dark against the light
                for k, most in (('b13_oldroad_west_dawn', 0.6), ('b12_oldroad_east_dawn', 0.5)):
                    hy = round(hz[k][0]); im = Image.open(f'{OUT}/{k}.png').convert('L'); px = im.load(); x0, x1 = int(W * 0.35), int(W * 0.95)
                    foot = sum(rowmean(px, y, x0, x1) for y in range(hy - 5, hy - 1)) / 4
                    sky = max(rowmean(px, y, x0, x1) for y in range(max(0, hy - 120), hy - 50))
                    check(foot <= most * sky, f'{k}: the foot of the band {foot:.0f} against the sky over it {sky:.0f} (at most {most:.0%})')
                # the mesas no lighter low on their bodies than at their tops (the band alone)
                for k in ALONE:
                    if k == 'b10_saro_east_dawn': continue
                    r, n = body_rise(f'{OUT}/{k}_alone.png', *hz[k])
                    check(r is not None and r <= 4.0, f'{k}: the body over the foot no lighter than under the top, within the picture\'s own grain ({r if r is None else round(r, 1)} of 255, {n} columns)')
                # a sharp skyline on High (the pictures are 128 rows): at most two rows from dark to the body
                r, n = skyline_width(f'{OUT}/b10_saro_east_dawn_alone.png', hz['b10_saro_east_dawn'][0])
                check(r is not None and r <= 2, f'the skyline takes {r} rows on High at {W}x{H} (median of {n} columns)')
                # the night horizon cold away from the lit places (none in view from the station looking east)
                for k in ('b09_station_east', 'b02_saro_east_night'):
                    hy = round(hz[k][0]); im = Image.open(f'{OUT}/{k}.png').convert('RGB'); px = im.load(); x0, x1 = int(W * 0.3), int(W * 0.7)
                    rgb = [sum(rowmean(px, y, x0, x1, ch) for y in range(hy - 50, hy - 30)) / 20 for ch in range(3)]
                    check(rgb[2] - rgb[0] >= 8, f'{k}: the sky over the band cold ({[round(c) for c in rgb]})')
                # the near mesas no lighter than the band's bodies at night, and less of the dawn's haze
                near = await ev("(() => { const m = S47.scene.getObjectByName('mesas:near'); return Math.max(...m.geometry.attributes.color.array); })()")
                check(near <= 0.016, f'the near mesas no lighter than the band ({near:.4f} at most, linear)')
                # the sun's mesa in the band: on the old road at 05:30, the band 0.45 degrees or more up at its azimuth
                await go(('s', 'oldroad', 4.0, 0, 70, 0.0, 5 * 3600 + 30 * 60))
                col = await ev(f"(() => {{ const T = S47.three, c = S47.camera, a = {81} * Math.PI / 180; c.updateMatrixWorld(); const p = c.position.clone().add(new T.Vector3(Math.sin(a), 0, -Math.cos(a)).multiplyScalar(4000)).project(c); return [(p.x + 1) / 2 * innerWidth, (1 - p.y) / 2 * innerHeight, innerHeight / 2 / Math.tan(c.fov * Math.PI / 360)]; }})()")
                await alone(f'{OUT}/sun_mesa_alone.png')
                im = Image.open(f'{OUT}/sun_mesa_alone.png').convert('RGB'); px = im.load()
                cx, hy, f = round(col[0]), col[1], col[2]
                rows = max(sum(1 for y in range(round(hy) - 40, round(hy)) if mag(px[x, y]) < 128) for x in range(cx - 2, cx + 3)) if 0 < cx < W else 0
                check(rows >= math.tan(math.radians(0.45)) * f, f'the sun\'s mesa in the band at azimuth 81: {rows} rows over the horizon ({math.tan(math.radians(0.6)) * f:.1f} for 0.6 degrees)')
                # the dip: none to speak of from the truck, atan(h / 20 km) from the air
                dip = await ev("S47.backdrop.uniforms.uDip.value")
                check(dip < 0.03, f'from the truck the band dips {dip:.3f} degrees')
                dip = await ev("(() => { const b = S47.backdrop, f = S47.scene.fog; b.update(f.color, f.density, true, 300, [], S47.camera.position, 0); return b.uniforms.uDip.value; })()")
                check(abs(dip - math.degrees(math.atan(300 / 20000))) < 0.005, f'300 m up it dips {dip:.3f} degrees')
                # the cost: one draw call; the budgets of the areas
                await go(('c', 'saro', -14.0, 50.0, 262, 0.03, NIGHT))
                on = await calls()
                await ev("S47.backdrop.mesh.visible = false; S47.backdrop.hold = true; S47.tick(0.02)")
                off = await calls()
                await ev("S47.backdrop.hold = false; S47.tick(0.02)")
                check(on - off == 1, f'the band is one draw call ({off} without it, {on} with it)')
                # (at the site; from the motel all of SARO is in view, 344 before the band: held to before above)
                saro = max(v for k, v in high.items() if k.startswith(('b01', 'b02', 'b03', 'b10', 'b11')))
                old = max(v for k, v in high.items() if 'oldroad' in k)
                check(saro <= 340, f'SARO outside at most 340 draw calls ({saro})')
                check(old <= 90, f'the old road at most 90 draw calls ({old})')
                # the textures: 4096 on High (a phone), 2048 on Low, all 8192 on Ultra (a computer), the same picture
                check(info['width'] == 4096 and info['art'], f"on High the painted band, {info['width']} wide ({info['source']})")
                for q, want in (('low', 2048), ('ultra', 8192)):
                    await ev(f"S47.setQuality('{q}'); S47.tick(0.1)")
                    w = (await ev("S47.backdrop.info()"))['width']
                    check(w == want, f"on {q.capitalize()} it is {w} wide")
                # on Ultra, through the occlusion and the bloom: nothing the picture's targets cannot hold (memory.md)
                for v in (VIEWS[0], VIEWS[9]):
                    await go(v)
                    await pg.screenshot(path=f'{OUT}/{v[0]}_ultra.png')
                    hdr = await ev("S47.hdrStats()")
                    check(hdr['over'] == 0, f"{v[0]} on Ultra: {hdr['over']} values out of range (brightest {hdr['max']:.1f})")
                await ev("S47.setQuality('high'); S47.tick(0.1)")
            if BEFORE_DIR and os.path.exists(f'{BEFORE_DIR}/backdrop_calls.json'):
                before = json.load(open(f'{BEFORE_DIR}/backdrop_calls.json'))['high']
                over = {k: (before[k], v) for k, v in high.items() if k in before and v > before[k] + EXTRA_CALLS}
                check(not over, f'no view costs more than {EXTRA_CALLS} draw call over before' + ('' if not over else f': {over}'))
        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        await b.close()
    json.dump({'checks': checks, 'seams': seams}, open(f'{OUT}/backdrop.json', 'w'), indent=1)
    ok = sum(1 for c in checks if c[0] == 'PASS')
    print(f'{ok} of {len(checks)} PASS')
    sys.exit(0 if ok == len(checks) else 1)

asyncio.run(main())
