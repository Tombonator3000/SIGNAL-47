# The third one before the credits (story/theThird.ts; FORSLAG.md, 20 October, Valg for Tom 4, a and b;
# Tom: "Spenning og skummelt slår faste regler"). HISTORIE.md: in the whole 1947 photograph after
# the credits, three at the survey equipment, Nora, Tomás and a third with Reyes's silhouette, the
# SARO jacket with the band across the back. Shown twice before that now:
#   (a) FRAME 03 at STATION 01 (chapter three): when the shutter goes, someone in a SARO jacket stands
#       out in the dark past the fence on C's ground, back to the camera, and the band gives the flash
#       back: in the print itself (R4), faint.
#       - not while the night has something else going (one at a time): no band
#       - framed from where most come (the gate): on C's ground at the frame's edge nearest C's stakes
#       - framed with the C stakes in it: on their line (bandC)
#       - the print: a pale line there, brighter than round it; the photo is the same after Continue
#       - developed: the field prints can be looked at closer; elsewhere nothing, on the band a note,
#         once; the print on the drying line, zoomed: the band in words, no second note
#   chapter four, room 6: N. Vega turns PHOTO 03 face down without a word (a toast, a note, once),
#       on the table, or under PHOTO 04 from the same roll; without the band, nothing; Continue holds
#   (b) 05:29 on the old road (chapter six): where the night takes the truck, someone stands on the road
#       ahead where it will come to stand; the truck rolls out and stands 9 m short of it; back to the
#       cab, still; when the headlights wake the band takes their colours (blue, amber, green, red,
#       a flare as each comes); on the screen (it changes the picture); it never moves; gone in the
#       FLASH; the pull-out's first shot is where it stood; written once in the seen log (looked at);
#       the draw calls; with slow fades and still shots, no flare and let go over the first fade
#   - desktop 1280x720 and a phone 412x892 with touch; no console errors or warnings
# Usage: python3 tools/f_third_person.py OUTDIR
import asyncio, sys, json, os, math, io, base64
from PIL import Image
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
T = 5 * 3600 + 29 * 60
MILE = 1609.34
s_at = lambda mi: (mi - 8.15) * MILE
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

HELPERS = """(() => {
  const V = S47.three.Vector3;
  window.T = {
    toasts: [],
    hook() {
      const ui = S47.game.d.ui; if (ui.__hooked) return; ui.__hooked = true;
      const o = ui.toast.bind(ui); ui.toast = (t, s, c) => { T.toasts.push(t); return o(t, s, c); };
    },
    notes(re) { return S47.game.notes.filter((n) => new RegExp(re).test(n)); },
    toastsOf(re) { return T.toasts.filter((n) => new RegExp(re).test(n)); },
    hud() { const e = document.querySelector('.hud .examine'); return e && e.classList.contains('show') ? e.textContent : null; },
    // the station's terms (x east, z south) to the world's
    w(x, z) { const o = S47.world.site.group.position; return [x + o.x, z + o.z]; },
    // the print's pixels round a point of the photo: the mean there and in a ring round it
    lum(url, u, v) { return new Promise((ok) => { const im = new Image(); im.onload = () => {
      const c = document.createElement('canvas'); c.width = im.width; c.height = im.height; const g = c.getContext('2d', { willReadFrequently: true }); g.drawImage(im, 0, 0);
      const x = Math.round(28 + u * 960), y = Math.round(28 + v * 600);
      const at = (dx, dy, r) => { const d = g.getImageData(x + dx - r, y + dy - r, 2 * r + 1, 2 * r + 1).data; let s = 0; for (let i = 0; i < d.length; i += 4) s += (d[i] + d[i + 1] + d[i + 2]) / 3; return s / (d.length / 4); };
      ok({ core: at(0, 0, 1), ring: (at(0, -9, 2) + at(0, 9, 2) + at(-16, 0, 2) + at(16, 0, 2)) / 4 }); }; im.src = url; }); },
    // the third one on the road (chapter six)
    third() {
      const c = S47.ch6, f = c.third, d = S47.world.oldDrive, r = S47.world.oldRoad;
      const o = { e: S47.game.clock - %d, speed: d.speed, shown: !!(f && f.mesh.visible), stage: c.stage };
      if (!f) return o;
      const p = f.mesh.getWorldPosition(new V()), b = f.bandColor;
      o.at = [p.x, p.y, p.z]; o.dist = Math.hypot(p.x - d.pos.x, p.z - d.pos.z);
      o.ground = p.y - r.height(p.x, p.z); o.lane = r.where(p.x, p.z).o;
      // which way it faces against the way from the cab to it (1: its back to the cab)
      const fy = f.mesh.getWorldQuaternion(new S47.three.Quaternion()), fwd = new V(0, 0, -1).applyQuaternion(fy);
      const to = new V(p.x - d.pos.x, 0, p.z - d.pos.z).normalize(); o.back = fwd.x * to.x + fwd.z * to.z;
      o.band = [b.r, b.g, b.b]; o.fade = f.fade;
      return o;
    },
    // where it is on the screen: the box from the band up to the head, and the band's middle
    box() {
      const f = S47.ch6.third, cam = S47.camera; cam.updateMatrixWorld(true);
      const p = f.mesh.getWorldPosition(new V());
      const P = (y, dx = 0) => { const right = new V(1, 0, 0).applyQuaternion(cam.quaternion); const q = p.clone().setY(p.y + y).addScaledVector(right, dx).project(cam); return [(q.x + 1) / 2 * innerWidth, (1 - q.y) / 2 * innerHeight, q.z]; };
      const top = P(1.8), band = P(1.2), feet = P(0), l = P(1.0, -0.42), r = P(1.0, 0.42);
      return { x0: l[0], x1: r[0], y0: top[1], y1: Math.min(feet[1], innerHeight), band, top, feet, front: band[2] < 1 };
    },
  };
  return true;
})()""" % T

AUTO = """((v) => { const w = S47.world, road = w.oldRoad, drv = w.oldDrive;
  w.testInput = () => { const s = road.where(drv.pos.x, drv.pos.z).s; const tg = road.lane(s + 18);
    let err = Math.atan2(-(tg.x - drv.pos.x), -(tg.z - drv.pos.z)) - drv.heading; err = Math.atan2(Math.sin(err), Math.cos(err));
    return { steer: Math.max(-1, Math.min(1, -err * 2.2)), throttle: Math.max(-1, Math.min(1, (v - drv.speed) * 0.5)) }; }; })"""

async def page(b, w, h, touch, store=None):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    init = "try{localStorage.setItem('s47.serlingNarrator','false');localStorage.setItem('s47.characterVoices','false');" + ''.join(f"localStorage.setItem('{k}','{v}');" for k, v in (store or {}).items()) + "}catch{}"
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    await pg.evaluate("localStorage.clear()")
    await pg.add_init_script(init)
    await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    await start(pg, touch)
    return ctx, pg, errs

async def start(pg, touch, sel='button[data-a=start]'):
    if touch: await pg.tap(sel)
    else: await pg.click(sel)
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS); await pg.evaluate("T.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    await pg.evaluate("S47.hold = true; S47.curve.free = true; S47.tick(0.2)")

class Run:
    def __init__(self, pg, touch, tag): self.pg = pg; self.ev = pg.evaluate; self.touch = touch; self.tag = tag
    async def tick(self, s, n=1):
        for _ in range(n): await self.ev(f"S47.tick({s})")
    async def to(self, e):
        """Chapter six's event to e seconds in (small steps)."""
        await self.ev(f"(() => {{ let left = {e} - (S47.game.clock - {T}); while (left > 1e-4) {{ const s = Math.min(1 / 30, left); S47.tick(s); left -= s; }} }})()")
    async def shot(self, name):
        await self.pg.screenshot(path=f'{OUT}/{self.tag}_{name}.png')
    async def quiet(self): await self.ev("S47.nightEvents['wait'] = 1e9")
    async def tap(self, sel):
        await self.pg.locator(sel).first.scroll_into_view_if_needed()
        if self.touch:
            bb = await self.pg.locator(sel).first.bounding_box()
            await self.pg.touchscreen.tap(bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2)
        else: await self.pg.click(sel)
        await self.tick(0.1)
    async def tap_at(self, x, y):
        if self.touch: await self.pg.touchscreen.tap(x, y)
        else: await self.pg.mouse.click(x, y)
        await self.tick(0.05); await self.pg.wait_for_timeout(350)

    # ---------- chapter three: FRAME 03 ----------
    async def station(self):
        ev = self.ev
        await ev("S47.game.d.ui.close(); S47.jump('chapter3'); S47.tick(1.0); S47.setAbductions(false)"); await self.quiet()
        await ev("window.__p = S47.world.prepare('station01')")
        await self.pg.wait_for_function("!!S47.world.site", polling=300)
        await ev("S47.world.placeTruck('station01'); S47.world.enter('station01'); S47.world.placeAtStation(); S47.tick(0.3); S47.ch3.arrivedStation(); S47.tick(0.3)")
        await ev("S47.ch3.s.readE09a = true; S47.ch3.s.stage = 'station'; S47.game.d.ui.close(); S47.game.d.ui.clearToasts()")
    async def frame(self, spot):
        """Stand where FRAME 03 is taken from and take it: 'gate' (7 m from the footing and the post
        towards the arrival, as most come) or 'west' (west of them, the C stakes in the frame)."""
        ev = self.ev
        if spot == 'gate':
            await ev("""(() => { const s = S47.world.site, t = s.anchors.markerTarget, p = S47.player, a = s.anchors.arrive;
              const base = Math.atan2(a.x - t.x, a.z - t.z);
              for (let i = 0; i < 24; i++) { const ang = base + (i % 2 ? 1 : -1) * Math.ceil(i / 2) * Math.PI / 12;
                const x = t.x + Math.sin(ang) * 7, z = t.z + Math.cos(ang) * 7;
                if (!p.walkable(x, z)) continue;
                p.place(x, z, Math.atan2(-(t.x - x), -(t.z - z))); p.pitch = Math.atan2(t.y - p.eye, 7); S47.tick(0.05); return; } })()""")
        else:
            await ev("""(() => { const t = S47.world.site.anchors.markerTarget, p = S47.player, [x, z] = T.w(-1.5, -6.5);
              p.place(x, z, Math.atan2(-(t.x - x), -(t.z - z)) - 0.32); p.pitch = Math.atan2(t.y - p.eye, Math.hypot(t.x - x, t.z - z)); S47.tick(0.05); })()""")
        await ev("if (!S47.fcam.raised) S47.ch1.toggleCamera()"); await self.tick(0.3)
        status = await ev("(document.querySelector('.viewfinder .vf-status')||{}).textContent || ''")
        await ev("S47.ch1.shutter()"); await self.tick(1.0)
        await ev("if (S47.fcam.raised) S47.ch1.toggleCamera()"); await self.tick(0.2)
        return status, await ev("S47.ch3.s.f3 ? { targets: S47.ch3.s.f3.targets, url: S47.ch3.s.f3.url } : null")

    # ---------- chapter six: THE EVENT ----------
    async def to_line(self, speed=16):
        ev = self.ev
        await ev("S47.game.d.ui.close(); S47.jump('chapter6'); S47.tick(0.5)")
        for _ in range(80):
            await self.tick(0.25); await self.pg.wait_for_timeout(100)
            if await ev("S47.world.area === 'roswell' && S47.world.driving"): break
        await ev(f"""(() => {{ const r = S47.world.oldRoad, d = S47.world.oldDrive, s = {s_at(8.12)};
              const a = r.lane(s), b2 = r.lane(s + 4); d.place(a, Math.atan2(-(b2.x - a.x), -(b2.z - a.z))); }})()""")
        await self.tick(0.1)
        await ev(AUTO + f"({speed})")
        for _ in range(900):
            await ev("S47.tick(1 / 30)")
            if await ev("S47.ch6.stage") == 'event': break
    async def shows(self, name):
        """The frame with the third one and the same frame without it: in the box round it, how many
        grey levels it changes at most and on average (it is seen, not only placed)."""
        ev, pg = self.ev, self.pg
        await ev("S47.game.d.ui.clearToasts()"); await pg.wait_for_timeout(200)
        box = await ev("T.box()")
        a = Image.open(io.BytesIO(await pg.screenshot(path=f'{OUT}/{self.tag}_{name}.png'))).convert('RGB')
        # (the same frame without it: its step held for one frame of a millisecond)
        await ev("(() => { const c = S47.ch6; c.__ts = c.thirdStep; c.thirdStep = () => {}; c.third.mesh.visible = false; S47.world.oldRoad.thirdGlints(null); S47.tick(0.001); })()")
        b = Image.open(io.BytesIO(await pg.screenshot())).convert('RGB')
        await ev("(() => { const c = S47.ch6; c.thirdStep = c.__ts; delete c.__ts; S47.tick(0.001); })()")
        x0, x1 = max(0, int(min(box['x0'], box['x1']))), min(a.width, int(max(box['x0'], box['x1'])) + 1)
        y0, y1 = max(0, int(box['y0'])), min(a.height, int(box['y1']) + 1)
        if x1 <= x0 or y1 <= y0: return {'max': 0, 'mean': 0, 'box': box, 'px': None}
        ca, cb = a.crop((x0, y0, x1, y1)).convert('L'), b.crop((x0, y0, x1, y1)).convert('L')
        da = [abs(p - q) for p, q in zip(ca.getdata(), cb.getdata())]
        bx, by = int(box['band'][0]), int(box['band'][1])
        # the band's own colour on the screen: the brightest pixel near its middle
        best = max((a.getpixel((x, y)) for x in range(max(0, bx - 6), min(a.width, bx + 7)) for y in range(max(0, by - 4), min(a.height, by + 5))), key=sum)
        return {'max': max(da), 'mean': sum(da) / len(da), 'box': [x0, y0, x1, y1], 'px': best}

async def frame03(R):
    """(a) chapter three: FRAME 03 and the band, the darkroom, the drying line."""
    ev = R.ev
    await R.station()
    # one at a time: with something of the night's going (the quiet round the ninth cow), no band
    await ev("S47.ninth.quiet = 30")
    status, f = await R.frame('gate')
    check('READY' in status and f and 'band' not in f['targets'] and 'footing' in f['targets'], f'one at a time: FRAME 03 taken while the night has something else going has no band ({list(f["targets"]) if f else None})')
    await ev("S47.ninth.quiet = 0; S47.ch3.s.f3 = undefined; S47.tick(0.2)")
    # framed from where most come: at the frame's edge nearest C's stakes, on C's ground
    status, f = await R.frame('gate')
    t = f['targets'] if f else {}
    check('READY' in status and 'band' in t and 'bandC' not in t and 0.06 < t['band'][0] < 0.94 and 0.07 < t['band'][1] < 0.93,
          f'FRAME 03 from the gate side: the band is in the picture, on C\'s ground beside the stakes ({t.get("band")})')
    lum = await ev(f"T.lum(S47.ch3.s.f3.url, {t['band'][0]}, {t['band'][1]})") if 'band' in t else {'core': 0, 'ring': 0}
    check(lum['core'] > lum['ring'] + 25 and lum['core'] < 245, f'and in the print itself: a pale line there, faint ({lum["core"]:.0f} against {lum["ring"]:.0f} round it)')
    if 'band' in t:
        im = Image.open(io.BytesIO(base64.b64decode(f['url'].split(',')[1])))
        x, y = 28 + t['band'][0] * 960, 28 + t['band'][1] * 600
        im.save(f'{OUT}/{R.tag}_a01_frame03_gate.jpg')
        im.crop((int(x - 120), int(y - 70), int(x + 120), int(y + 70))).resize((720, 420), Image.LANCZOS).save(f'{OUT}/{R.tag}_a02_frame03_gate_close.png')
    # framed with the C stakes in it: on their line
    await ev("S47.ch3.s.f3 = undefined; S47.tick(0.2)")
    status, f = await R.frame('west')
    t = f['targets'] if f else {}
    check('READY' in status and 'band' in t and 'bandC' in t, f'FRAME 03 with the C stakes in the frame: the band stands on their line ({t.get("bandC")})')
    lum = await ev(f"T.lum(S47.ch3.s.f3.url, {t['band'][0]}, {t['band'][1]})") if 'band' in t else {'core': 0, 'ring': 0}
    check(lum['core'] > lum['ring'] + 25, f'and a pale line in the print there ({lum["core"]:.0f} against {lum["ring"]:.0f})')
    if 'band' in t:
        im = Image.open(io.BytesIO(base64.b64decode(f['url'].split(',')[1])))
        x, y = 28 + t['band'][0] * 960, 28 + t['band'][1] * 600
        im.save(f'{OUT}/{R.tag}_a03_frame03_stakes.jpg')
        im.crop((int(x - 120), int(y - 70), int(x + 120), int(y + 70))).resize((720, 420), Image.LANCZOS).save(f'{OUT}/{R.tag}_a04_frame03_stakes_close.png')
    check(await ev("!S47.scene.getObjectByName('third:film') && !(S47.ch3.s.f3.url === undefined)"), 'only on the film: nothing of it left in the scene after the shutter')
    check(len(await ev("T.notes('looked at closely')")) == 0, 'not written down when it is taken: it has not been seen')
    return f

async def darkroom(R, f3):
    """The field roll developed at SARO: the prints looked at closer, and the drying line."""
    ev = R.ev
    await ev(f"S47.ch3.s.f4 = Object.assign({{}}, S47.ch3.s.f3, {{ id: 'frame04', targets: {{ cable: [0.5, 0.5] }} }}); S47.ch3.s.stage = 'develop'; S47.ch3.s.dev = 0")
    await ev("S47.world.enter('saro'); S47.player.place(15.1, -4.6, 0); S47.player.pitch = -0.5; S47.tick(0.2); S47.game.d.ui.clearToasts()")
    for step in range(3):
        await ev("S47.game.d.inter.get('wetBench').use()"); await R.tick(0.1)
        await R.tap('.labpanel [data-a=act]'); await R.tick(2.6)
    await R.pg.wait_for_selector('.labpanel .lp-closer')
    check('look closer' in await ev("document.querySelector('.labpanel .lp-tip').textContent"), 'the field prints: they can be looked at closer')
    img = R.pg.locator('.labpanel [data-print="0"] img')
    bb = await img.bounding_box()
    band = f3['targets']['band']
    pxy = lambda u, v: (bb['x'] + (28 + u * 960) / 1016 * bb['width'], bb['y'] + (28 + v * 600) / 720 * bb['height'])
    # closer, away from the band: nothing written
    far = (0.15 if band[0] > 0.5 else 0.85, 0.85)
    await R.tap_at(*pxy(*far))
    check(await ev("document.querySelector('.labpanel [data-print=\"0\"] .lp-closer').classList.contains('close')") and len(await ev("T.notes('looked at closely')")) == 0,
          'closer on FRAME 03 away from the band: nothing written')
    await R.tap_at(*pxy(*far))   # back
    await R.tap_at(*pxy(band[0], band[1]))
    n = await ev("T.notes('looked at closely')")
    check(len(n) == 1 and 'gave the flash back' in n[0] and 'nobody out there' in n[0] and await ev("S47.townSeen.has('third:band')"), f'closer on the band: a note, in my words ({n[0][:70] if n else None})')
    await R.pg.wait_for_timeout(400)
    await R.shot('a05_prints_closer')
    await R.tap_at(*pxy(band[0], band[1])); await R.tap_at(*pxy(band[0], band[1]))
    check(len(await ev("T.notes('looked at closely')")) == 1, 'once: looked at again, not written again')
    await R.tap('.labpanel .lp-close'); await R.tick(1.2)
    # (a chapter card, then chapter four begins: wait for it to be over)
    for _ in range(60):
        await R.tick(0.5); await R.pg.wait_for_timeout(50)
        if await ev("S47.game.phase === 'ch4' && !S47.game.cinematic && !document.querySelector('.overlay, .chapter-card.on')"): break
    await ev("S47.game.d.ui.close()"); await R.pg.wait_for_timeout(1800)
    # the print on the drying line, zoomed
    await ev("S47.player.place(16.2, -1.8, 0); S47.tick(0.05)")
    await ev("""(() => { const p = S47.player, q = S47.yard.dryPrints[2].getWorldPosition(new S47.three.Vector3()); const c = S47.camera.position;
      p.yaw = Math.atan2(-(q.x - p.pos.x), -(q.z - p.pos.z)); p.pitch = Math.atan2(q.y - c.y, Math.hypot(q.x - c.x, q.z - c.z)); S47.tick(0.05); })()""")
    await ev("S47.input.zoomHeld = true"); await R.tick(0.3, 3)
    hud = await ev("T.hud()")
    check(await ev("S47.yard.dryPrints[2].visible") and hud is not None and 'FRAME 03 on the drying line' in hud and 'gave the flash back' in hud, f'FRAME 03 on the drying line, zoomed: the band in words ({(hud or "")[:60]})')
    await R.shot('a06_drying_line')
    await ev("S47.input.zoomHeld = false"); await R.tick(0.4)
    check(len(await ev("T.notes('looked at closely')")) == 1, 'and no second note')

async def room6(R, f3=None, band=True, fresh=False):
    """Chapter four, room 6, the talk open; FRAME 03 the one taken (with or without its band). fresh:
    the talk from the start again (the jump goes on with the chapter as it was saved)."""
    ev = R.ev
    await ev("S47.game.d.ui.close(); S47.jump('chapter4'); S47.tick(1.0)"); await R.quiet()
    if fresh: await ev("Object.assign(S47.ch4.s, { shown: false, p10: false, placed: [], down3: undefined, topics: { record: false, tomas: false, cable: false }, p11: false, mark: true })")
    if f3: await ev(f"(() => {{ const f = {json.dumps(f3)}; if (!{json.dumps(band)}) {{ delete f.targets.band; delete f.targets.bandC; }} S47.ch3.s.f3 = Object.assign({{ id: 'frame03', subject: 'STATION 01 / fixed point A', method: 'field', clock: 13200 }}, f); S47.ch3.s.dev = 3; }})()")
    await ev("S47.world.goRoom6()")
    await R.pg.wait_for_function("S47.world.area === 'room6' && !S47.world.busy", polling=200)
    await R.tick(0.3)
    await ev("S47.game.d.inter.get('r6:nora').use()"); await R.tick(0.2)
    await R.pg.wait_for_selector('.labpanel .talk')

AFTER = "(() => { const e = document.querySelector('.talk .talk-after'); if (!e) return null; const r = e.getBoundingClientRect(); const box = document.querySelector('.talk [data-reading-texts]'); return { text: e.textContent, op: +getComputedStyle(e).opacity, inView: r.top >= 0 && r.bottom <= innerHeight, read: box ? box.dataset.readingTexts.includes('face down') : false }; })()"

async def nora(R, f3):
    """Chapter four: she turns PHOTO 03 face down."""
    ev = R.ev
    # on the table with the others
    await room6(R, f3, fresh=True)
    check(await ev("!!document.querySelector('.talk [data-chip=frame03] img')"), 'room 6: PHOTO 03 can go on her table, the print on it')
    for c in ['frame01', 'frame02', 'e07', 'frame03']: await R.tap(f'.talk [data-chip={c}]')
    await R.tap('.talk [data-a=table]')
    a0 = await ev(AFTER)
    check(await ev("S47.ch4.s.shown") and a0 is not None and a0['op'] < 0.2, f'the evidence on the table: she lays it out, and it comes after her lines, not with them ({a0["op"] if a0 else None})')
    await R.pg.wait_for_timeout(3800)
    a = await ev(AFTER)
    n = await ev("T.notes('PHOTO 03 face down')")
    check(a is not None and 'turns it face down' in a['text'] and 'She does not say anything' in a['text'] and a['op'] > 0.9 and a['inView'] and len(n) == 1 and await ev("S47.ch4.s.down3"),
          f'she turns PHOTO 03 face down without a word, told under her lines, and a note ({(a or {}).get("text", "")[:60]})')
    await R.shot('a07_nora_face_down')
    lines = await ev("[...document.querySelectorAll('.talk-line:not(.talk-after)')].map(e => e.textContent).join(' | ')")
    check('face down' not in lines and 'I know that second line' in lines and not a['read'] and len(await ev("T.toastsOf('face down')")) == 0,
          'not one of her lines: they are as they were, and it is not read aloud with them')
    await R.tap('.talk [data-c=lamp]'); await R.tick(0.2)
    await R.tap('.talk [data-c=cable]'); await R.pg.wait_for_timeout(4200)
    check(await ev(AFTER) is None and len(await ev("T.notes('PHOTO 03 face down')")) == 1, 'once: PHOTO 04 from the same roll, and she does not do it again')
    # Continue
    await ev("S47.saveNow('manual', 0)"); await R.pg.wait_for_timeout(700)
    await R.pg.reload()
    await R.pg.wait_for_selector('button[data-a=cont]:not([disabled])')
    await start(R.pg, R.touch, 'button[data-a=cont]')
    await R.tick(0.5)
    check(await ev("S47.ch4.s.down3 === true") and len(await ev("T.notes('PHOTO 03 face down')")) == 1 and await ev("!!(S47.ch3.s.f3 && S47.ch3.s.f3.targets.band)"),
          'Continue: she has turned it, it is written once, and FRAME 03 still has its band')
    if await ev("S47.world.area") != 'room6':
        await ev("S47.world.goRoom6()"); await R.pg.wait_for_function("S47.world.area === 'room6' && !S47.world.busy", polling=200)
    await ev("S47.game.d.inter.get('r6:nora').use()"); await R.tick(0.2); await R.pg.wait_for_timeout(3600)
    check(await ev("!!document.querySelector('.labpanel .talk')") and await ev(AFTER) is None, 'Continue: back at her table, nothing again')
    # without the band: nothing
    await room6(R, f3, band=False, fresh=True)
    for c in ['frame01', 'frame02', 'e07', 'frame03']: await R.tap(f'.talk [data-chip={c}]')
    await R.tap('.talk [data-a=table]'); await R.pg.wait_for_timeout(3600)
    nb = await ev(AFTER)
    await R.tap('.talk [data-c=lamp]'); await R.tick(0.2)
    await R.tap('.talk [data-c=cable]'); await R.pg.wait_for_timeout(4200)
    check(nb is None and await ev(AFTER) is None and not await ev("S47.ch4.s.down3"), 'a FRAME 03 without the band: she does not turn it')
    # the band, but PHOTO 03 not on the table: under PHOTO 04
    await room6(R, f3, fresh=True)
    for c in ['frame01', 'frame02', 'e07']: await R.tap(f'.talk [data-chip={c}]')
    await R.tap('.talk [data-a=table]'); await R.pg.wait_for_timeout(3600)
    check(await ev(AFTER) is None, 'PHOTO 03 kept back from the table: not then')
    await R.tap('.talk [data-c=lamp]'); await R.tick(0.2)
    await R.tap('.talk [data-c=cable]'); await R.pg.wait_for_timeout(5200)
    a = await ev(AFTER)
    check(a is not None and 'under it, from the same roll' in a['text'] and a['op'] > 0.9, f'it comes under PHOTO 04 from the same roll, and she turns it ({(a or {}).get("text", "")[:50]})')
    await R.shot('a08_nora_roll')

async def event(R, steady=False, phone=False):
    """(b) chapter six: the third one in the headlights."""
    ev = R.ev
    await R.to_line(16)
    await R.to(0.3)
    s = await ev("T.third()")
    calls0 = await ev("S47.renderer.info.render.calls")
    check(s['stage'] == 'event' and s['shown'] and 25 < s['dist'] < 80 and abs(s['ground']) < 0.05 and abs(s['lane']) < 3.5,
          f'05:29:00, the night takes the truck: someone on the road ahead, where it will stand ({s["dist"]:.0f} m ahead, {s["lane"]:.1f} m from the middle of the road)')
    at0 = s['at']
    await R.to(5.0); await R.shot('b01_rolling_up')
    await R.to(9.6)
    s = await ev("T.third()")
    check(abs(s['speed']) < 0.05 and abs(s['dist'] - 9) < 0.7 and s['back'] > 0.75, f'the truck rolls out and stands 9 m short of it ({s["dist"]:.2f} m), its back to the cab ({s["back"]:.2f})')
    pre = await R.shows('b02_before_the_light')
    check(pre['max'] > 40, f'before the headlights wake it is there, a shape in the grey ({pre["max"]} grey levels at most)')
    seq = []
    flare = []
    for name, e, want in [('b03_blue', 11.3, 2), ('b04_amber', 13.1, 0), ('b05_green', 15.2, 1), ('b06_red', 17.6, 0)]:
        if name == 'b04_amber':
            # the flare as a colour comes (not with slow fades or still shots): at amber's coming, and after
            await R.to(12.2); flare.append(sum((await ev("T.third()"))['band']))
            await R.to(13.1); flare.append(sum((await ev("T.third()"))['band']))
        await R.to(e)
        s = await ev("T.third()")
        sh = await R.shows(name)
        bc = s['band']
        dom = max(range(3), key=lambda i: bc[i])
        if name == 'b04_amber': ok = bc[0] > bc[1] > bc[2] and bc[1] > 0.35 * bc[0]
        elif name == 'b06_red': ok = bc[0] > 4 * bc[1] and bc[0] > 4 * bc[2]
        else: ok = dom == want
        px = sh['px']
        seq.append((name, [round(v, 2) for v in bc], px))
        check(ok and s['shown'] and sh['max'] > 60, f'+{e - 0.3:.0f} s: the band in the headlights\' colour ({name[4:]}: {[round(v, 2) for v in bc]}; on the screen {px}, it changes up to {sh["max"]} grey levels)')
        if name == 'b03_blue': check(px and px[2] > px[0] + 30 and px[2] > 120, f'on the screen the band is blue there ({px})')
        if name == 'b06_red': check(px and px[0] > px[2] + 40 and px[0] > 120, f'on the screen the band is red there ({px})')
        if not phone and name == 'b03_blue':
            box = sh['box']; check((box[3] - box[1]) > 55, f'big enough to be someone: {box[3] - box[1]} px from the head to the bonnet')
    if phone:
        b = await ev("T.box()")
        check(b['y1'] - b['y0'] > 60 and 0.25 * 412 < b['band'][0] < 0.75 * 412, f'on the phone: {b["y1"] - b["y0"]:.0f} px tall, in the middle of the view ({b["band"][0]:.0f} px across)')
    f1, f2 = flare
    if steady: check(f1 < f2 * 1.2, f'slow fades: no flare as amber comes, the band only follows the light ({f1:.2f} against {f2:.2f})')
    else: check(f1 > f2 * 1.4, f'the band flares as amber comes ({f1:.2f} against {f2:.2f} after)')
    calls = await ev("S47.renderer.info.render.calls")
    check(calls <= 90 and calls - calls0 <= 2, f'draw calls with it standing there: {calls} (at most 90)')
    await R.to(24.4)
    check('Reference west' in ' '.join(await ev("T.toasts")) and (await ev("T.third()"))['shown'], 'the radio plays the night back, and it is still there')
    await R.to(27.4); await R.shot('b07_red_radio')
    await R.to(31.9)
    s = await ev("T.third()")
    check(s['shown'] and math.dist(s['at'], at0) < 1e-6, 'it has not moved in 31.9 s')
    if steady:
        await R.to(32.5)
        s = await ev("T.third()")
        check(s['shown'] and 0.2 < s['fade'] < 0.9, f'slow fades: it is let go over the first fade, not with a frame ({s["fade"]:.2f} at +32.5 s)')
        await R.shot('b08_letgo')
        await R.to(33.4)
        check(not (await ev("T.third()"))['shown'], 'and gone after it')
    else:
        await R.to(32.05)
        s = await ev("T.third()")
        fr = await ev("+getComputedStyle(document.querySelector('.ending-frame')).opacity")
        check(not s['shown'] and fr == 1, f'+32 s: gone in the FLASH, under the first frame ({fr})')
    seen = await ev("S47.seen.filter((e) => e.what === 'the third')")
    check(len(seen) == 1 and seen[0]['kind'] == 'scare' and seen[0]['line'] == 'C' and seen[0]['looking'] is True and seen[0]['clock'] == T,
          f'the seen log: once, a scare on C at 05:29, looked at ({seen})')
    # the pull-out's first shot stands where it stood
    await R.to(47.3); await R.tick(6.5)
    cam = await ev("S47.camera.position.toArray()")
    check(math.hypot(cam[0] - at0[0], cam[2] - at0[2]) < (0.8 if not steady else 1.2) and not await ev("!!(S47.ch6.third)"), f'the camera goes out to where it stood and looks back ({math.hypot(cam[0] - at0[0], cam[2] - at0[2]):.2f} m from its place)')
    await R.shot('b09_where_it_stood')
    return seq

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        allerr = []
        # ======================= desktop =======================
        ctx, pg, errs = await page(b, 1280, 720, False)
        R = Run(pg, False, 'd')
        f3 = await frame03(R)
        await darkroom(R, f3)
        # Continue in the photo lab: the same photograph
        url0 = await R.ev("S47.ch3.s.f3.url")
        await R.ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(700)
        await pg.reload(); await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await start(pg, False, 'button[data-a=cont]')
        check(await R.ev("S47.ch3.s.f3.url") == url0 and await R.ev("!!S47.ch3.s.f3.targets.band") and await R.ev("S47.townSeen.has('third:band')"),
              'Continue: FRAME 03 is the same photograph, band and all (R4), and it was noticed')
        await nora(R, f3)
        await event(R)
        allerr += errs; await ctx.close()
        # ======================= desktop, slow fades and still shots =======================
        ctx, pg, errs = await page(b, 1280, 720, False, {'s47.calmFlash': 'true', 's47.stillShots': 'true'})
        R = Run(pg, False, 'c')
        await event(R, steady=True)
        allerr += errs; await ctx.close()
        # ======================= phone =======================
        ctx, pg, errs = await page(b, 412, 892, True)
        R = Run(pg, True, 'p')
        f3p = await frame03(R)
        await darkroom(R, f3p)
        await event(R, phone=True)
        allerr += errs; await ctx.close()

        print('\n'.join(allerr[:30]) or 'no console errors/warnings')
        check(not allerr, 'console clean (desktop, slow fades, phone)')
        json.dump(checks, open(f'{OUT}/checks-third-person.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
