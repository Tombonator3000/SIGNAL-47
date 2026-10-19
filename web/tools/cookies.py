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
# beam, the hot core and the broken arcs in the flashlight's pool seen straight down, every lit shader
# with it, the cut above the headlights' horizon, the panes on the sidewalk), the windows' light
# goes with the window (the studio after four, the sheriff's office while he is out, the first
# television after twenty to four) and comes up when the street lamps go out (event 8), the draw
# calls (at most two more than with them off at any view, Low no more than High, SARO outside
# within grounds.py's 340, the control room within artcheck's 300, the old road within
# chapter6.py's 90) and what a frame costs on Low with them and with none (the round pools, no
# flashlight pattern, no window light). Also writes the windows' picture (cookie_windows.png).
# Round two (the art director, 19 October): the flashlight's light over the old disc's pool at least
# 0.9 of the old disc's (the mean linear light above the dark, on the screen) and its bright beam
# as wide; the windows' light on the ground at most about half as bright as the room seen through
# the same glass; Caliche from the street (t01, t04: town.py's views); and pictures of the states
# that were only levels before (OUTDIR/st_*.png and st_sheet.jpg: a television on, flickering and
# off, the sheriff in and out, the studio on the air and after four, the street lamps out in
# event 8, dawn).
# --before: only the views and their draw calls (a build without the cookies), for comparing.
# --compare=DIR: a --before run's OUTDIR: the draw calls against it, and a side by side of each
# view before and after (<view>_ba.png, before left).
# Usage: python3 tools/cookies.py OUTDIR [WxH] [--before] [--compare=DIR] [--only=VIEW,...] [--cost]
import asyncio, sys, os, json, math
from playwright.async_api import async_playwright
from PIL import Image, ImageStat, ImageChops, ImageFilter, ImageDraw

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
# linear light on the screen: the sRGB curve undone, the luminance of the three channels
LIN = [((i / 255) / 12.92 if i <= 10 else (((i / 255) + 0.055) / 1.055) ** 2.4) for i in range(256)]
def linlum(path, step=2):
    """The picture's linear luminance, every step-th pixel each way: (values, width, height)."""
    im = Image.open(path).convert('RGB')
    if step > 1: im = im.resize((im.width // step, im.height // step), Image.NEAREST)
    d = im.tobytes()
    return [0.2126 * LIN[d[i]] + 0.7152 * LIN[d[i + 1]] + 0.0722 * LIN[d[i + 2]] for i in range(0, len(d), 3)], im.width, im.height
def energy(on, off, dark):
    """The flashlight's light on the screen against the old disc's, above the same frame with it
    off: over the old disc's pool (where it adds a tenth of its brightest or more), the mean light
    added (cookie on / the old disc), and the bright beam's width (where it adds a third of the old
    disc's own bright middle, the 90th percentile; as the square root of the areas)."""
    A, w, h = linlum(on); B, _, _ = linlum(off); D, _, _ = linlum(dark)
    a = [max(0.0, x - z) for x, z in zip(A, D)]; b = [max(0.0, y - z) for y, z in zip(B, D)]
    top = max(b) or 1e-6
    pool = [i for i, y in enumerate(b) if y > 0.1 * top]
    if len(pool) < 50: return 0, 0
    e = sum(a[i] for i in pool) / max(1e-9, sum(b[i] for i in pool))
    mid = sorted(b[i] for i in pool)[int(len(pool) * 0.9)]
    wa, wb = sum(1 for x in a if x > mid / 3), sum(1 for y in b if y > mid / 3)
    return e, math.sqrt(wa / max(1, wb))
def poly_mask(size, pts, shrink=1.0):
    """A mask of a polygon on the screen (pixels), drawn shrunk about its middle by shrink."""
    cx, cy = sum(p[0] for p in pts) / len(pts), sum(p[1] for p in pts) / len(pts)
    m = Image.new('L', size, 0)
    ImageDraw.Draw(m).polygon([(cx + (p[0] - cx) * shrink, cy + (p[1] - cy) * shrink) for p in pts], fill=255)
    return m
def hull(pts):
    """The convex hull of the points on the screen in front of the eye (the light's grid: the part
    of it that is in the picture, near enough)."""
    P = sorted(set((round(p[0], 1), round(p[1], 1)) for p in pts if -1 < p[2] < 1))
    if len(P) < 3: return P
    def side(o, a, b): return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lo, hi = [], []
    for p in P:
        while len(lo) >= 2 and side(lo[-2], lo[-1], p) <= 0: lo.pop()
        lo.append(p)
    for p in reversed(P):
        while len(hi) >= 2 and side(hi[-2], hi[-1], p) <= 0: hi.pop()
        hi.append(p)
    return lo[:-1] + hi[:-1]
def patch_vs_glass(on, off, shapes):
    """How bright a window's light on the ground looks against the room seen through its glass, in
    linear light: the light it adds (cookie on less off) in the brightest tenth of its patch,
    over the mean of the glass (on the screen, a little in from the frame)."""
    A, w, h = linlum(on, 1); B, _, _ = linlum(off, 1)
    g, p = [], []
    for sh in shapes:
        if any(q[2] > 1 or q[2] < -1 for q in sh['glass']) or len(hull(sh['patch'])) < 3: continue
        gm, pm = poly_mask((w, h), sh['glass'], 0.8).tobytes(), poly_mask((w, h), hull(sh['patch'])).tobytes()
        g += [A[i] for i in range(len(gm)) if gm[i]]
        p += [A[i] - B[i] for i in range(len(pm)) if pm[i]]
    if len(g) < 200 or len(p) < 200: return None
    p.sort(reverse=True)
    lit = p[:max(1, len(p) // 10)]
    return (sum(lit) / len(lit)) / max(1e-6, sum(g) / len(g)), sum(lit) / len(lit), sum(g) / len(g)

def beam_profile(on, off, dark, S=8):
    """The cookie's own picture straight down, the ground's texture divided out: the light it adds
    over the old disc's (linear, above the dark), where the old disc is at least a third of its
    brightest, by the distance from the pool's middle, in S sectors round it. From that the core
    (the brightest of the inner fifth against the middle, all round) and the arcs in each sector
    (how far its profile goes up and down round its own running mean, a part of its mean): the
    strongest sector's, and the weakest's against it (broken arcs, not even rings)."""
    A, w, h = linlum(on, 2); B, _, _ = linlum(off, 2); D, _, _ = linlum(dark, 2)
    b = [max(0.0, y - z) for y, z in zip(B, D)]; top = max(b) or 1e-6
    pts = [i for i in range(len(b)) if b[i] > top / 3]
    if len(pts) < 50: return 0, 0, 1
    cx, cy = sum(i % w for i in pts) / len(pts), sum(i // w for i in pts) / len(pts)
    R = int(math.sqrt(len(pts) / math.pi)) + 1
    bins = [[[0.0, 0] for _ in range(R)] for _ in range(S + 1)]
    for i in pts:
        dx, dy = i % w - cx, i // w - cy
        r = int(math.hypot(dx, dy)); k = int((math.atan2(dy, dx) + math.pi) / (2 * math.pi) * S) % S
        if r < R:
            x = max(0.0, A[i] - D[i]) / b[i]
            for kk in (k, S): bins[kk][r][0] += x; bins[kk][r][1] += 1
    def prof(k): return [s / n for s, n in bins[k] if n > 1]
    def ups(p):
        run = [sum(p[max(0, i - 3):i + 4]) / len(p[max(0, i - 3):i + 4]) for i in range(len(p))]
        inner = range(len(p) // 4, int(len(p) * 0.9))
        return math.sqrt(sum((p[i] - run[i]) ** 2 for i in inner) / max(1, len(inner))) / max(1e-6, sum(p) / len(p))
    allr = prof(S)
    if len(allr) < 12: return 0, 0, 1
    core = max(allr[:max(1, len(allr) // 5)]) / max(1e-6, allr[len(allr) // 2])
    arcs = [ups(prof(k)) for k in range(S)]
    return core, max(arcs), min(arcs) / max(1e-6, max(arcs))

# The fixed views: (name, kind, x, z, yaw, pitch, light[, clock]). kind: 'saro' on foot at SARO (its own
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
    # Caliche from the street, where players see it (town.py's t01 and t04, at three)
    ('t01_street_south', 'town', -1.3, 2012.4, math.pi, -0.02, False, 3 * 3600),
    ('t04_mid', 'town', 1.3, 2110.4, math.pi, 0.0, False, 3 * 3600),
]
# the views of a window and its light: (the area's window lights, the window)
WIN_OF = {'c1_caliche_sidewalk': ('town', 'laundroWin'), 'c3_caliche_gas': ('town', 'gasWin'), 'c4_caliche_east': ('town', 'studio'),
          'm1_motel_office': ('motel', 'office'), 's1_saro_windows': ('saro', 'room')}
FLASH = ['f1_flashlight_wall', 'f2_flashlight_ground', 'f4_flashlight_down']
LOWULTRA = ['f1_flashlight_wall', 'f2_flashlight_ground', 'f4_flashlight_down', 'h1_headlights_highway', 'h3_headlights_wall', 'c1_caliche_sidewalk']
# where in the picture each light's pattern lies (fractions of the picture): the beam, the road ahead, the sidewalk
CROP = {
    'f1_flashlight_wall': (0.25, 0.2, 0.85, 0.95), 'f2_flashlight_ground': (0.2, 0.25, 0.9, 1.0), 'f3_flashlight_corridor': (0.25, 0.2, 0.85, 0.95), 'f4_flashlight_down': (0.0, 0.0, 1.0, 1.0),
    'h1_headlights_highway': (0.0, 0.45, 1.0, 0.85), 'h2_headlights_track': (0.0, 0.45, 1.0, 0.85), 'h3_headlights_wall': (0.0, 0.2, 1.0, 0.85),
    'c1_caliche_sidewalk': (0.0, 0.45, 1.0, 1.0), 'c2_caliche_truck': (0.0, 0.45, 1.0, 0.9), 'c3_caliche_gas': (0.0, 0.45, 1.0, 1.0),
    'c4_caliche_east': (0.0, 0.45, 1.0, 1.0), 'm1_motel_office': (0.0, 0.45, 1.0, 1.0),
    't01_street_south': (0.0, 0.4, 1.0, 1.0), 't04_mid': (0.0, 0.4, 1.0, 1.0),
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

        async def go(v, clock=None):
            name, kind, x, z, yaw, pitch, light = v[:7]
            if clock is None: clock = v[7] if len(v) > 7 else NIGHT
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
            if v[0] in WIN_OF:
                # where the window's glass and its light are on the screen in this frame
                shapes[tag] = await ev(SHAPES, list(WIN_OF[v[0]]))
            if v[0] == 'f4_flashlight_down':
                # the pool's shape, at a third of the light: straight down at the feet the beam is
                # past white (the old disc as well), and white shows no core and no arcs
                await ev("S47.flashlight.light.intensity *= 0.3; S47.vhs.render(S47.scene, S47.camera, performance.now() / 1000)")
                await pg.screenshot(path=f'{OUT}/{tag}_dim.png')
                await ev("S47.cookies(false); S47.vhs.render(S47.scene, S47.camera, performance.now() / 1000)")
                await pg.screenshot(path=f'{OUT}/{tag}_dimoff.png')
                await ev("S47.cookies(true); S47.tick(0.1)")
            if v[0] in FLASH:
                # the same frame with the flashlight off: the dark the light is measured above
                await ev("S47.flashlight.on = false; S47.tick(0.6)")
                await pg.screenshot(path=f'{OUT}/{tag}_dark.png')
                await ev("S47.flashlight.on = true; S47.tick(0.6)")
            return n, n0

        calls_on, calls_off, again, shapes = {}, {}, {}, {}
        SHAPES = """(([name, id]) => { const W = S47.windowLights.find((w) => w.name === name); if (!W) return [];
          S47.camera.updateMatrixWorld();
          const P = (v) => { const p = v.clone().project(S47.camera); return [(p.x + 1) / 2 * innerWidth, (1 - p.y) / 2 * innerHeight, p.z]; };
          return W.shapes(id).map((s) => ({ glass: s.glass.map(P), patch: s.patch.map(P) })); })"""
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
        # straight down (at a third of the light): a hot core in the flood with faint broken arcs in
        # it, not one even disc (the cookie's picture over the old disc's, the ground divided out)
        for q in ('', '_low', '_ultra'):
            tag = 'f4_flashlight_down' + q
            core, arcs, even = beam_profile(f'{OUT}/{tag}_dim.png', f'{OUT}/{tag}_dimoff.png', f'{OUT}/{tag}_dark.png')
            check(core > 1.15 and 0.005 < arcs < 0.05 and even < 0.65, f"{q[1:] or 'high'}: straight down, a hot core over the flood ({core:.2f} of the profile's middle) and faint arcs in it, broken round about ({arcs:.4f} up and down in the strongest eighth, the weakest {even:.2f} of it)")
        # as much light as the old disc, and as wide a beam (the first cookie threw about half)
        for q in ('', '_low', '_ultra'):
            for v in FLASH:
                tag = v + q
                e, wd = energy(f'{OUT}/{tag}.png', f'{OUT}/{tag}_off.png', f'{OUT}/{tag}_dark.png')
                check(e >= 0.9 and wd >= 0.9, f"{q[1:] or 'high'}: {v}: the light over the old disc's pool {e:.2f} of the old disc's and the bright beam {wd:.2f} as wide (at least 0.9 each, linear light above the dark)")
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
        # dimmer than the room it comes from, seen through the same glass (linear light)
        for v, (area, wid) in WIN_OF.items():
            r = patch_vs_glass(f'{OUT}/{v}.png', f'{OUT}/{v}_off.png', shapes.get(v, []))
            if r is None: check(False, f"{v}: the window's glass and its light are both in the picture"); continue
            check(r[0] <= 0.55, f"{v}: the light on the ground at most about half the room's through the glass ({r[1]:.4f} added against {r[2]:.4f}: {r[0]:.2f})")
        # and from the street, where players see the town: the windows' light shows
        for v in ('t01_street_south', 't04_mid'):
            n = brighter(f'{OUT}/{v}.png', f'{OUT}/{v}_off.png', box_of(v), 2)
            check(n > 800, f"{v}: from the street the windows' light shows on the ground ({n} pixels brighter with it)")
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
        # (held back about half under a lamp now, two thirds before: on average over every point of
        # the town's window light it is a little)
        check(dk[0] < 1 and dk[1] >= dk[0] + 0.01, f"the windows' light is held back where the street lamps light the ground, and comes up when they go out (event 8: {dk[0]:.3f} to {dk[1]:.3f} on average)")

        # ---------- the states, in pictures (High, the cookies on) ----------
        # a window from the street in front of it: out metres out, side to its right (seen from outside)
        async def at_window(area, wid, out, side, clock, pitch=-0.22, n=0):
            await ev(f"S47.game.clock = {clock}")
            await ev(f"S47.world.enter('{'diner' if area == 'town' else area}'); S47.tick(0.2)")
            spot = await ev("""(([name, id, out, side, n]) => { const W = S47.windowLights.find((w) => w.name === name), s = W.shapes(id)[n];
              const a = new S47.three.Vector3(0, 1, 0).cross(s.out).normalize(), p = s.c.clone().addScaledVector(s.out, out).addScaledVector(a, side);
              const t = s.c.clone().addScaledVector(s.out, 1.2), yaw = Math.atan2(-(t.x - p.x), -(t.z - p.z));
              return [p.x, p.z, yaw]; })""", [area, wid, out, side, n])
            await ev(f"S47.player.place({spot[0]}, {spot[1]}, {spot[2]}); S47.player.pitch = {pitch}; S47.tick(0.4)")
        STATES = []
        async def state(name, label, win=None):
            await ev("S47.tick(0.1)")
            await pg.screenshot(path=f'{OUT}/{name}.png'); STATES.append((name, label))
            if win:
                # and with the windows' light off, for how bright it is against its room
                sh = await ev(SHAPES, list(win))
                await ev("S47.cookies(false); S47.vhs.render(S47.scene, S47.camera, performance.now() / 1000)")
                await pg.screenshot(path=f'{OUT}/{name}_off.png')
                await ev("S47.cookies(true); S47.tick(0.05)")
                r = patch_vs_glass(f'{OUT}/{name}.png', f'{OUT}/{name}_off.png', sh)
                if r: check(r[0] <= 0.55, f"{name}: the light on the ground at most about half the room's through the glass ({r[1]:.4f} added against {r[2]:.4f}: {r[0]:.2f})")
        await ev("S47.setQuality('high'); S47.cookies(true)")
        await stop_driving()
        # the first television: on, a moment later (it flickers), and off after twenty to four
        await at_window('town', 'tv1', 6.5, 2.0, NIGHT)
        await state('st_tv_on', 'tv1 02:20, on', ('town', 'tv1'))
        await ev("S47.tick(0.35)"); await state('st_tv_flicker', 'tv1 0.35 s later')
        await at_window('town', 'tv1', 6.5, 2.0, 3 * 3600 + 50 * 60)
        await state('st_tv_off', 'tv1 03:50, off')
        # the sheriff's office: in, and out on his round
        await at_window('town', 'sheriffWin', 6.0, -2.5, NIGHT)
        await state('st_sheriff_in', 'sheriff 02:20, in', ('town', 'sheriffWin'))
        await at_window('town', 'sheriffWin', 6.0, -2.5, 3 * 3600 + 20 * 60)
        await state('st_sheriff_out', 'sheriff 03:20, out')
        # the studio: on the air, and after four
        await at_window('town', 'studio', 6.0, 2.5, NIGHT)
        await state('st_studio_on', 'studio 02:20, on the air', ('town', 'studio'))
        await at_window('town', 'studio', 6.0, 2.5, 4 * 3600 + 10 * 60)
        await state('st_studio_four', 'studio 04:10, dimmed')
        # event 8: the street lamps go out, the windows' light comes up
        await go(next(x for x in VIEWS if x[0] == 't04_mid'))
        await state('st_event8_before', 'main street 03:00')
        await ev("S47.world.town.lampWave(S47.runTime() - 12); S47.tick(0.2)")
        await state('st_event8_out', 'event 8: lamps out')
        await ev("S47.world.town.lampWave(-1e9); S47.tick(0.2)")
        # dawn
        for v in ('c1_caliche_sidewalk', 'c4_caliche_east'):
            await go(next(x for x in VIEWS if x[0] == v), 5 * 3600 + 14 * 60)
            await state(f'st_dawn_{v[:2]}', f'{v[:2]} dawn 05:14')
        await ev(f"S47.game.clock = {NIGHT}; S47.tick(0.1)")
        # the light on the ground with each: the television's flickers and goes, the sheriff's and the
        # studio's go with them, the lamps out bring the sidewalk's up
        for x0, x1, what in (('st_tv_on', 'st_tv_off', "the television's light goes with it"), ('st_sheriff_in', 'st_sheriff_out', "the sheriff's goes out with him"),
                             ('st_studio_on', 'st_studio_four', "the studio's dims after four")):
            n = brighter(f'{OUT}/{x0}.png', f'{OUT}/{x1}.png', (0, int(H * 0.45), W, H), 3)
            check(n > 1000, f"in the pictures: {what} ({n} pixels darker after)")
        n = changed(f'{OUT}/st_tv_on.png', f'{OUT}/st_tv_flicker.png', (0, int(H * 0.45), W, H), 2)
        print(f"the television's flicker on the ground: {n} pixels changed in 0.35 s", flush=True)
        # one sheet of them, labelled
        th = [Image.open(f'{OUT}/{n}.png').resize((W // 3, H // 3)) for n, _ in STATES]
        cols = 4; rows = (len(th) + cols - 1) // cols
        sheet = Image.new('RGB', (cols * (W // 3 + 4), rows * (H // 3 + 22)), (16, 16, 16)); d = ImageDraw.Draw(sheet)
        for i, (im, (n, label)) in enumerate(zip(th, STATES)):
            x, y = (i % cols) * (W // 3 + 4), (i // cols) * (H // 3 + 22)
            sheet.paste(im, (x, y + 20)); d.text((x + 4, y + 4), label, fill=(230, 230, 220))
        sheet.save(f'{OUT}/st_sheet.jpg', quality=88)

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
