# The ninth cow (story/ninthCow.ts, world/ninthCow.ts; FORSLAG.md, 20 October, the order's item 6;
# Tom: "Spenning og skummelt slår faste regler"). HENDELSER.md, event 2: nine Herefords in the
# pasture south of the motel stand facing 026, and the next time there are eight and a ring of frost
# where the ninth stood. She is found in chapter three at STATION 01, in the same pasture's west end,
# inside the chained fence, on her side on B between the transit and the comparison vane, her head
# on 026: no blood anywhere, an oval of the jaw to the bone, the tongue, a round piece of her side
# cut out clean, frost on her open eye, no tracks in the frost round her. Her tag is the herd's.
#   - not before chapter three: at the station in chapter two she is not there, the herd is as it was
#   - chapter three: not put down while her place is in view; turned away, she is, and at the same
#     moment she goes from the pasture (cows:short, the herd's missing one hidden)
#   - one at a time: during another 47 (the silence) she is not found, not even with a closer look;
#     after it, looked at in the flashlight, she is: once, a toast and a note, the mark at STATION 01
#     on B on the map, a line in the seen log (looked at), the night quiet round her
#   - a closer look: her head (the eye, the jaw, the tongue, the tag ALDERETE 25, a note once), her
#     side (the cut), the frost (no tracks); kneeling by her: cold, hot metal, a note once
#   - a free frame of her: the line under the print
#   - the board: her card with three pins and the ring in the pasture (not seen yet); a wrong thread
#     does not hold, the tag to the ring does, and is kept
#   - driven back with the autopilot: the herd in the headlights, eight, and the ring; the toast and
#     the note say where the ninth is; the herd's mark comes with it, and the seen log has it once
#   - Continue: she lies there, found, the thread kept, nothing written again; a new night: not there
#   - the herd's tags in the pasture, read with a closer look on foot
#   - desktop 1280x720, and a phone 412x892 with touch on Low: found in the light, the zoom, Use;
#     no console errors or warnings
# Usage: python3 tools/f_ninth_cow.py OUTDIR
import asyncio, sys, json, os, math, base64
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
TAG = 'ALDERETE 25'
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
    site() { return S47.world.site; },
    o() { return S47.world.site.group.position; },
    // stand at station-local (x, z), looking at one of her parts (or turned `turn` radians away from it)
    at(x, z, part, turn = 0, pitch = null) {
      const o = T.o(), t = S47.world.site.ninthAt(part), p = S47.player, wx = x + o.x, wz = z + o.z;
      p.place(wx, wz, Math.atan2(-(t.x - wx), -(t.z - wz)) + turn); S47.tick(0.05);
      const c = S47.camera.position;
      p.pitch = pitch ?? Math.atan2(t.y - c.y, Math.hypot(t.x - c.x, t.z - c.z)); S47.tick(0.05);
      return [p.pos.x - o.x, p.pos.z - o.z];
    },
    // stand d metres from her part, on walkable ground clear of colliders, looking at it
    near(part, d, from) {
      const p = S47.player, r = p.radius + 0.02, o = T.o(), t = S47.world.site.ninthAt(part);
      const free = (x, z) => p.walkable(x, z) && !p.colliders.some((c) => x > c.minX - r && x < c.maxX + r && z > c.minZ - r && z < c.maxZ + r);
      for (let i = 0; i < 24; i++) {
        const ang = from + (i % 2 ? 1 : -1) * Math.ceil(i / 2) * Math.PI / 12;
        const x = t.x + Math.sin(ang) * d, z = t.z + Math.cos(ang) * d;
        if (!free(x, z)) continue;
        return T.at(x - o.x, z - o.z, part);
      }
      return null;
    },
    aimed() { const it = S47.game.d.inter.update(S47.camera); return it ? it.id : null; },
    hud() { const e = document.querySelector('.hud .examine'); return e && e.classList.contains('show') ? e.textContent : null; },
    has(k) { return S47.townSeen.has(k); },
    keys(re) { return [...S47.townSeen].filter((k) => new RegExp(re).test(k)); },
    count(re) { return S47.game.notes.filter((n) => new RegExp(re).test(n)).length; },
    herds() { return S47.world.herds.map((h) => h.missing); },
  };
  return true;
})()"""

async def page(b, w, h, touch):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("try{localStorage.setItem('s47.serlingNarrator','false');localStorage.setItem('s47.characterVoices','false');}catch{}")
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
    except Exception: pass
    await pg.evaluate("localStorage.clear()")
    await pg.add_init_script("try{localStorage.setItem('s47.serlingNarrator','false');localStorage.setItem('s47.characterVoices','false');}catch{}")
    await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    if touch: await pg.tap('button[data-a=start]')
    else: await pg.click('button[data-a=start]')
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS)
    await pg.evaluate("T.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    return ctx, pg, errs

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        # ======================= desktop =======================
        ctx, pg, errs = await page(b, 1280, 720, False)
        ev = pg.evaluate
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        notes = lambda: ev("S47.game.notes.slice()")
        has = lambda k: ev(f"T.has('{k}')")
        state = lambda: ev("S47.ninth.state")
        async def quiet(): await ev("S47.nightEvents['wait'] = 1e9")
        async def zoom(secs=0.9):
            await ev("S47.input.zoomHeld = true"); await tick(secs)
            t = await ev("T.hud()")
            await ev("S47.input.zoomHeld = false"); await tick(0.4)
            return t
        # (the night's curve stands aside; nothing of the night's own comes in the middle of the checks)
        await ev("S47.hold = true; S47.curve.free = true")
        await quiet()

        # ---------- not before chapter three ----------
        await ev("S47.jump('chapter2'); S47.tick(1.0); S47.setAbductions(false)"); await quiet()
        herd2 = await ev("S47.pasture.stage")
        await ev("window.__p = S47.world.prepare('station01')")
        await pg.wait_for_function("!!S47.world.site", polling=300)
        await ev("S47.world.enter('station01'); S47.tick(0.2)")
        await ev("T.at(-11.0, 3.2, 'middle', Math.PI)")   # in the hut: her place out of sight
        await tick(0.5, 4)
        check(await ev("S47.game.phase") == 'ch2' and await state() == 'not yet' and not await ev("T.site().ninthDown")
              and not await has('cows:short') and await ev("S47.pasture.stage") == herd2,
              f'chapter two, at the station, her place out of sight: she is not there, and the herd is as it was ({herd2})')
        await ev("S47.world.enter('saro'); S47.tick(0.2)")

        # ---------- chapter three: put down only out of sight, and gone from the pasture at once ----------
        await ev("S47.jump('chapter3'); S47.tick(1.0); S47.setAbductions(false)"); await quiet()
        herd0 = await ev("S47.pasture.stage")
        await ev("S47.world.enter('station01'); T.at(1.0, -2.2, 'middle'); S47.tick(0.1)")
        for _ in range(6): await tick(0.5)
        d0 = await ev("T.site().ninthAt('middle').distanceTo(S47.camera.position)")
        check(await state() == 'not yet' and not await ev("T.site().ninthDown") and not await has('cows:short'),
              f'chapter three, {d0:.1f} m from her place and looking at it: nothing is put down there while it can be seen')
        await shot('desk_01_her_place_before')
        await ev("T.at(1.0, -2.2, 'middle', Math.PI)"); await tick(0.3)
        check(await state() == 'down' and await ev("T.site().ninthDown") and await has('nine:down'), 'turned away from it: she is lying there now')
        hm = await ev("T.herds()")
        check(await has('cows:on') and await has('cows:short') and await ev("S47.pasture.stage") == 'short' and await ev("S47.pasture.ninthGone")
              and all(m == 4 for m in hm), f'and at the same moment she is gone from the pasture: the eight and the ring (the herd was {herd0}; the herds built: {hm})')
        check(await ev("T.site().colliders.some((c) => { const t = S47.world.site.ninthAt('middle'); return t.x > c.minX && t.x < c.maxX && t.z > c.minZ && t.z < c.maxZ; })"),
              'she stops the player (a collider round her)')

        # ---------- one at a time: not found during another 47 ----------
        await ev("S47.roadside.silenceNow({ hush: (s, l) => S47.audio.night?.hush(s, l), flicker: () => {} })"); await tick(0.2)
        check(await ev("S47.oneAtATime()") and await ev("S47.roadside.silent"), 'the silence (47 s), one of the night\'s things, is going')
        await ev("S47.flashlight.have = true; S47.flashlight.on = true")
        st = await ev("T.near('middle', 4.0, 0.6)")
        await tick(0.5, 6)
        z0 = await zoom()
        check(st is not None and await state() == 'down' and not await has('nine:found') and (z0 is None or 'cow' not in z0),
              f'during it, 4 m from her in the flashlight for 3 s and a closer look: she is not found ({z0})')
        # (turned away from her while it lasts)
        await ev("(() => { T.near('middle', 4.0, 0.6); const o = T.o(); T.at(S47.player.pos.x - o.x, S47.player.pos.z - o.z, 'middle', Math.PI); })()")
        for _ in range(60):
            if not await ev("S47.roadside.silent"): break
            await tick(1.0)
        check(await state() == 'down', 'turned away while it lasts: still not found')
        n0 = len(await notes())
        await ev("T.near('middle', 4.0, 0.6)"); await tick(0.3, 3)
        nn = (await notes())[n0:]
        found = [n for n in nn if 'dead on her side' in n]
        tt = ' '.join(await ev("T.toasts"))
        check(not await ev("S47.roadside.silent") and await state() == 'found' and len(found) == 1, f'after it, looked at in the flashlight: found, once ({len(found)} notes)')
        check(found and 'No blood anywhere' in found[0] and 'between the transit and the comparison vane' in found[0] and 'her head to 026' in found[0]
              and 'No tracks in the frost' in found[0] and 'The vehicle gate is chained.' in found[0], f'the note: on B, no blood, cut out clean, no tracks ({found[:1]})')
        check('a cow, on her side in a patch of frost. She does not move.' in tt, 'the toast: a cow on her side in a patch of frost, not moving')
        marks = await ev("S47.mapMarks().filter((m) => m.kind === 'ninth')")
        check(len(marks) == 1 and marks[0]['line'] == 'B' and (marks[0].get('at') or {}).get('spot') == 'station01', f'a mark on the map at STATION 01, on B ({marks})')
        seen = await ev("S47.seen.filter((e) => e.what === 'ninth')")
        check(len(seen) == 1 and seen[0]['kind'] == 'event' and seen[0]['line'] == 'B' and seen[0]['looking'] is True and seen[0]['area'] == 'station01',
              f'the seen log: one line, an event on B, looked at ({seen})')
        check(await ev("S47.ninth.quiet") > 20 and await ev("S47.ninth.books().length") == 1 and await ev("S47.oneAtATime()") and await ev("S47.musicHeld()"),
              'the night goes quiet round her (the crickets, the music, and nothing else of the night\'s starts), and she is on the board')
        await shot('desk_02_found')

        # ---------- a closer look, and kneeling by her ----------
        n1 = len(await notes())
        await ev("T.near('head', 2.3, 0.4)")
        zh = await zoom(1.0)
        await ev("S47.input.zoomHeld = true"); await tick(1.0); await shot('desk_03_zoom_head'); await ev("S47.input.zoomHeld = false"); await tick(0.4)
        check(zh and 'Frost on her open eye' in zh and 'down to the bone' in zh and 'the tongue is gone' in zh and TAG in zh, f'her head: the eye, the jaw, the tongue, the tag ({zh})')
        await zoom(0.6)
        tagn = [n for n in (await notes())[n1:] if TAG in n]
        check(len(tagn) == 1 and 'pasture by the highway' in tagn[0], f'her tag, written down once: the herd by the highway ({tagn})')
        await ev("T.near('body', 2.6, -1.2)")
        zb = await zoom(1.0)
        check(zb and 'chassis punch' in zb and 'Not a drop of blood' in zb, f'her side: the round cut, clean, no blood ({zb})')
        await ev("S47.input.zoomHeld = true"); await tick(1.0); await shot('desk_04_zoom_side'); await ev("S47.input.zoomHeld = false"); await tick(0.4)
        # the frost: a point of it between me and her
        fz = await ev("""(() => { const o = T.o(), m = S47.world.site.ninthAt('middle'), p = S47.player;
          const dx = 1.0, dz = 0.62, l = Math.hypot(dx, dz), fx = m.x + dx / l * 2.4, fz = m.z + dz / l * 2.4, x = m.x + dx / l * 5.2, z = m.z + dz / l * 5.2;
          p.place(x, z, Math.atan2(-(fx - x), -(fz - z))); S47.tick(0.05); const c = S47.camera.position;
          p.pitch = Math.atan2(m.y - 0.45 - c.y, Math.hypot(fx - c.x, fz - c.z)); S47.tick(0.05); return [x - o.x, z - o.z]; })()""")
        zf = await zoom(1.0)
        check(zf and 'No tracks in it' in zf and 'not pressed down' in zf, f'the frost round her: unbroken, no tracks, the grass under her not pressed ({zf})')
        await ev("T.near('body', 1.9, 0.9)"); await tick(0.2)
        aim = await ev("T.aimed()")
        lab = await ev(f"S47.game.d.inter.get({json.dumps(aim)})?.label()") if aim else None
        check(aim in ('s1ninth:body', 's1ninth:head') and lab == 'Dead cow', f'by her: "Dead cow" to use ({aim}, {lab})')
        n2 = len(await notes())
        await ev(f"S47.game.d.inter.get({json.dumps(aim)}).use()"); await tick(0.2)
        await ev(f"S47.game.d.inter.get({json.dumps(aim)}).use()"); await tick(0.2)
        tt = await ev("T.toasts.slice(-2)")
        nn = (await notes())[n2:]
        check(len(tt) == 2 and 'colder than the ground' in tt[0] and 'hot metal' in tt[0] and 'Still cold' in tt[1] and len(nn) == 1 and 'after an arc' in nn[0],
              f'knelt by: cold, the smell of hot metal, one note; again: still cold ({tt}, {len(nn)} note)')
        await shot('desk_05_knelt')

        # ---------- a free frame of her ----------
        await ev("T.near('middle', 5.0, 0.3)"); await tick(0.2)
        if not await ev("S47.fcam.raised"): await pg.keyboard.press('KeyC'); await tick(0.2)
        k = await ev("S47.album.shots.length")
        await pg.keyboard.press('Space'); await tick(0.3)
        shotd = await ev("S47.album.shots.at(-1)") if await ev("S47.album.shots.length") > k else None
        if shotd: open(f'{OUT}/desk_06_print.jpg', 'wb').write(base64.b64decode(shotd['url'].split(',', 1)[1]))
        if await ev("S47.fcam.raised"): await pg.keyboard.press('KeyC'); await tick(0.2)
        check(shotd is not None and 'Not a drop of blood.' in shotd['text'] and shotd.get('odd'), f'a free frame of her: "{shotd and shotd["text"]}"')

        # ---------- the board ----------
        await ev("S47.game.openBoard('STATION 01', 'ch3')"); await tick(0.2)
        await pg.wait_for_selector('.board-wrap')
        cards = await ev("[...document.querySelectorAll('.bcard')].map((c) => [c.dataset.card, c.classList.contains('away'), c.textContent.replace(/\\s+/g, ' ').trim()])")
        cd = {c[0]: c for c in cards}
        check('nine' in cd and not cd['nine'][1] and await ev("document.querySelectorAll('[data-card=nine] .bline').length") == 3 and TAG in cd['nine'][2],
              'on the board: her card, with a pin on each of three lines (the tag, the cuts, the frost)')
        check('nine.ring' in cd and cd['nine.ring'][1] and 'Not on the table yet. The pasture by the highway, where she stood.' in cd['nine.ring'][2],
              'and the ring in the pasture: not on the table yet, "The pasture by the highway, where she stood."')
        bsay = lambda: ev("(document.querySelector('.board-wrap [data-say]')||{}).textContent || ''")
        async def thread(a, b):
            for n in (a, b):
                await ev(f"S47.game.board.view.show('{n}')"); await pg.wait_for_timeout(80)
                await pg.click(f'[data-node="{n}"]')
            return await bsay()
        r0 = await thread('nine.cut', 'nine.frost')
        check('same record' in r0 and not await ev("T.keys('^nl:').length"), f'two of her own lines: the same record, no thread ({r0})')
        await ev("S47.game.board.view.show('nine')"); await pg.wait_for_timeout(150)
        await shot('desk_07_board_before')
        await pg.click('.board-wrap .lp-close'); await tick(0.2)

        # ---------- the way back: the eight, and the ring ----------
        n3 = len(await notes())
        await ev("S47.world.placeTruck('station01'); S47.world.enter('station01'); S47.world.placeAtStation(); S47.tick(0.3)")
        await ev("S47.world.driveBack()")
        for _ in range(240):
            await tick(0.25)
            if await ev("S47.world.driving"): break
            await pg.wait_for_timeout(100)
        await ev("S47.world.autopilot = 12")
        cowsAt, t0 = None, 0.0
        for _ in range(1200):
            await tick(0.25); t0 += 0.25
            if t0 % 20 < 0.3: await quiet()
            if cowsAt is None and await has('ev:cows'): cowsAt = t0; await shot('desk_08_eight_in_the_headlights')
            if not await ev("S47.world.driving"): break
        for _ in range(8): await tick(0.5)
        nn = (await notes())[n3:]
        jt = ' '.join(await ev("T.toasts"))
        eight = [n for n in nn if 'One missing. A ring of frost in the grass.' in n]
        check(cowsAt is not None and len(eight) == 1 and 'lying at STATION 01' in eight[0] and 'She is lying inside the fence at STATION 01.' in jt,
              f'driven back: the herd in the headlights, eight, and the ring, {cowsAt} s out; the note and the toast say where the ninth is ({eight[:1]})')
        cm = await ev("T.keys('^mark:cows@')")
        cs = await ev("S47.seen.filter((e) => e.what === 'cows' || e.what === 'cows2').map((e) => e.what)")
        check(len(cm) == 1 and cs == ['cows2'] and await ev("S47.mapMarks().filter((m) => m.kind === 'cows').length") == 1,
              f'the eight without the nine first: the herd\'s mark on the map, and one line in the seen log ({cm}, {cs})')
        check(await ev("S47.world.area") == 'saro', 'back at SARO')
        # the board, now the ring is seen
        await ev("S47.game.openBoard('SARO', 'ch3')"); await tick(0.2)
        await pg.wait_for_selector('.board-wrap')
        check(await ev("!!document.querySelector('[data-card=\"nine.ring\"]:not(.away)')"), 'on the board: the ring in the pasture, on the table now')
        r1 = await thread('nine.cut', 'nine.ring')
        check('Not that one' in r1 and not await ev("T.keys('^nl:').length"), f'her cuts to the ring: does not hold ({r1})')
        r2 = await thread('nine.tag', 'nine.ring')
        check('nothing walked her here' in r2 and await ev("T.keys('^nl:')") == ['nl:nine.ring|nine.tag'], f'her tag to the ring: it holds, and is kept ({r2})')
        await ev("S47.game.board.view.show('nine')"); await pg.wait_for_timeout(150)
        await shot('desk_10_board')
        await pg.click('.board-wrap .lp-close'); await tick(0.2)
        # with T. Vega's field book read (story/fieldBook.ts): her frost to his Jul 3 and 4
        await ev("S47.ch3.s.fb = { at: S47.game.clock, times: {}, had: [], told: ['cows2'], links: [] }")
        await ev("S47.game.openBoard('SARO', 'ch3')"); await tick(0.2)
        await pg.wait_for_selector('.board-wrap')
        r3 = await thread('nine.frost', 'fb.cows')
        check('He counted them too, and never wrote down where the ninth went.' in r3 and 'nl:fb.cows|nine.frost' in await ev("T.keys('^nl:')"),
              f'with his field book read: her frost to his "Nine head. Eight.", it holds ({r3})')
        await ev("S47.game.board.view.show('fb')"); await pg.wait_for_timeout(150)
        await shot('desk_10b_board_fieldbook')
        await pg.click('.board-wrap .lp-close'); await tick(0.2)

        # ---------- Continue ----------
        await ev("S47.game.d.ui.close()"); await tick(0.3)
        await ev("S47.saveNow('manual', 0)")
        await pg.wait_for_timeout(800)
        notes_before = await notes()
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev(HELPERS); await ev("T.hook()"); await ev("S47.hold = true; S47.curve.free = true; S47.tick(0.5)"); await quiet()
        await pg.add_style_tag(content='.lockhint{display:none !important}')
        await ev("S47.setAbductions(false)")
        check(await state() == 'found' and sorted(await ev("T.keys('^nl:')")) == ['nl:fb.cows|nine.frost', 'nl:nine.ring|nine.tag'] and len(await notes()) == len(notes_before),
              'Continue: she is found, the thread is kept, no note is doubled')
        await ev("window.__p = S47.world.prepare('station01')")
        await pg.wait_for_function("!!S47.world.site", polling=300)
        await ev("S47.world.enter('station01'); S47.world.placeAtStation(); S47.tick(0.3)")
        await ev("S47.flashlight.have = true; S47.flashlight.on = true; T.near('middle', 4.0, 0.6)"); await tick(0.5, 6)
        check(await ev("T.site().ninthDown") and len(await notes()) == len(notes_before) and await ev("T.keys('^mark:ninth@').length") == 1
              and await ev("S47.seen.filter((e) => e.what === 'ninth').length") == 1, 'at the station she lies there still; looked at again, nothing is written again')
        await shot('desk_09_after_continue')

        # ---------- the herd's tags, on foot among them ----------
        await ev("S47.world.enter('saro'); S47.tick(0.2)")
        hz = await ev("""(() => { const h = S47.world.herds.find((x) => x.area === 'saro') || S47.world.herds[0]; if (!h) return null;
          const m = h.lookAreas[2]; const p = m.getWorldPosition(new S47.three.Vector3()); return [p.x, p.y, p.z, h.area]; })()""")
        zt = None
        if hz and hz[3] == 'saro':
            # (the pasture's own ground is the road's: a cow examined from SARO's stretch of it, as the
            # player standing on the shoulder by the fence sees them)
            await ev(f"""(() => {{ const P = S47.player, t = new S47.three.Vector3({hz[0]}, {hz[1]}, {hz[2]}); const x = t.x + 3.5, z = t.z;
              P.colliders = []; P.zones = [{{ id: 't', minX: x - 2, maxX: x + 2, minZ: z - 2, maxZ: z + 2, enabled: true }}]; P.place(x, z, Math.atan2(-(t.x - x), -(t.z - z))); S47.tick(0.05);
              const c = S47.camera.position; P.pitch = Math.atan2(t.y - c.y, Math.hypot(t.x - c.x, t.z - c.z)); S47.tick(0.05); }})()""")
            zt = await zoom(1.0)
        check(zt is not None and 'A yellow tag in her left ear: ALDERETE 18.' in zt,
              f'a cow of the eight, a closer look: her tag, the same yellow tags, ALDERETE ({zt})')
        await ev("S47.world.enter('saro'); S47.tick(0.2)")
        bad = [e for e in errs if 'favicon' not in e]
        check(not bad, f'desktop: no console errors or warnings ({bad[:4]})')
        await ctx.close()

        # ---------- a new night: not there ----------
        ctx, pg, errs = await page(b, 1280, 720, False)
        ev = pg.evaluate
        await ev("S47.hold = true; S47.curve.free = true; S47.nightEvents['wait'] = 1e9")
        await ev("S47.jump('chapter3'); S47.tick(1.0); S47.setAbductions(false)")
        await ev("window.__p = S47.world.prepare('station01')")
        await pg.wait_for_function("!!S47.world.site", polling=300)
        await ev("S47.world.enter('station01'); T.at(1.0, -2.2, 'middle'); S47.tick(0.3)")
        check(await ev("S47.ninth.state") == 'not yet' and not await ev("T.site().ninthDown") and not await ev("T.has('nine:found')"),
              'a new night (a new case): she is not lying there until she is put down')
        # chapter three's own drive out, for real: she lies there when the truck comes in, behind the hut from the pad
        await ev("S47.world.enter('saro'); S47.world.placeTruck('saro'); S47.tick(0.1); S47.world.driveOut()")
        for _ in range(240):
            await ev("S47.tick(0.25)")
            if await ev("S47.world.driving"): break
            await pg.wait_for_timeout(100)
        await ev("S47.world.autopilot = 13")
        downAt = None
        for _ in range(900):
            await ev("S47.tick(0.5); S47.nightEvents['wait'] = 1e9")
            if downAt is None and await ev("S47.ninth.down"):
                downAt = await ev("(() => { const t = S47.world.site.ninthAt('middle'), c = S47.camera.getWorldPosition(new S47.three.Vector3()); return [S47.world.area, Math.round(c.distanceTo(t))]; })()")
            if not await ev("S47.world.driving"): break
        await ev("S47.tick(1.0)")
        check(await ev("S47.world.area") == 'station01' and downAt is not None and downAt[0] == 'station01' and downAt[1] > 110 and await ev("S47.pasture.stage") == 'short',
              f'driven out for real: she is put down while the truck is still out on the track ({downAt} m), and the herd is short of her')
        check(await ev("S47.ninth.state") == 'down', 'parked on the pad: the hut stands between the truck and her, not found yet')
        await pg.screenshot(path=f'{OUT}/desk_11_arrival_pad.png')
        bad = [e for e in errs if 'favicon' not in e]
        check(not bad, f'a new night: no console errors or warnings ({bad[:4]})')
        await ctx.close()

        # ======================= phone, Low =======================
        ctx, pg, errs = await page(b, 412, 892, True)
        ev = pg.evaluate
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        await ev("S47.hold = true; S47.curve.free = true; S47.nightEvents['wait'] = 1e9; S47.setQuality('low')")
        await ev("S47.jump('chapter3'); S47.tick(1.0); S47.setAbductions(false); S47.nightEvents['wait'] = 1e9")
        await ev("window.__p = S47.world.prepare('station01')")
        await pg.wait_for_function("!!S47.world.site", polling=300)
        await ev("S47.world.enter('station01'); T.at(-11.0, 3.2, 'middle', Math.PI); S47.tick(0.5)")   # in the hut
        check(await ev("S47.ninth.state") == 'down', 'phone: in the hut, out of sight of her place: she is put down')
        await ev("S47.flashlight.have = true; S47.flashlight.on = false; T.near('middle', 9.5, -2.4)"); await tick(0.3, 3)
        check(await ev("S47.ninth.state") == 'down', 'phone: 9.5 m off in the dark, the light off: not found yet')
        await shot('phone_01_dark')
        await pg.tap('.tbtn.light'); await tick(0.3, 3)
        check(await ev("S47.flashlight.on") and await ev("S47.ninth.state") == 'found', 'phone: Light, and in it she is found')
        await shot('phone_02_found_low')
        await ev("T.near('head', 2.3, 0.4)")
        await ev("S47.input.zoomHeld = true"); await tick(1.0)
        zp = await ev("T.hud()")
        await shot('phone_03_zoom_head')
        await ev("S47.input.zoomHeld = false"); await tick(0.4)
        check(zp and TAG in zp, f'phone: Zoom on her head ({zp})')
        await ev("T.near('body', 1.9, 0.9)"); await tick(0.2)
        await pg.tap('.tbtn.use'); await tick(0.3)
        tt = await ev("T.toasts.slice(-1)")
        check(tt and 'colder than the ground' in tt[0], f'phone: Use by her: cold ({tt})')
        await shot('phone_04_knelt')
        fits = await ev("(() => { const e = document.querySelector('.toasts'); const r = e.getBoundingClientRect(); return r.right <= innerWidth + 1 && r.left >= -1; })()")
        check(fits, 'phone: the words on the screen')
        bad = [e for e in errs if 'favicon' not in e]
        check(not bad, f'phone: no console errors or warnings ({bad[:4]})')
        await ctx.close()
        await b.close()
    n = sum(1 for c in checks if c[0] == 'PASS')
    print(f'\n{n}/{len(checks)} PASS', flush=True)
    with open(f'{OUT}/results.json', 'w') as fh: json.dump(checks, fh, indent=1)
    sys.exit(0 if n == len(checks) else 1)

asyncio.run(main())
