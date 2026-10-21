# T. Vega's field book and the envelope for plate 4 (story/fieldBook.ts, ui/fieldBookView.ts;
# FORSLAG.md, 20 October, item 3 and small tweak 3). In the drawer under the bench in the hut at
# STATION 01: T. VEGA / FIELD BOOK 3 / JUNE-JULY 47. June is the survey; July is tonight: the
# three lights along B, the herd on 026 (nine, then eight), three knocks on the roof of the car,
# the lamps in Caliche with the dogs, the county car with nobody in it, in the same words, and at
# the same minute where the player has already seen it. Last page: "Three on plate 4. There were
# two of us." What the player has not seen yet comes on the way back. On the board each line has
# a card from tonight; the thread holds: "He wrote it in July 1947. I saw it tonight." The
# envelope on the motel office's counter, held up to the lamp: the transit, and three by it.
#   - not before chapter three; the drawer is in the hut, below the lamp, and can be aimed at
#   - driven out for real with the autopilot: the lights along B come on the way (their mark)
#   - opened and closed before the last page: not read (no note, nothing kept), the drawer stays
#     out, and the book keeps the minutes it had when it was first opened
#   - read to the end: the lights' page has their minute, the rest none; one note, one toast; the
#     book in Papers; kept with chapter three's part of the case; the herd turns (it is far)
#   - the board: the book with six line pins, a card for each thing from tonight (the seen one
#     pinned, the others away: "In his book. Not yet in mine."); the right thread holds with the
#     note, a wrong one does not; the thread is kept
#   - not while anything else is going on: driving with the night not free, no knocks
#   - driven back with the autopilot: the knocks on the cab roof on the track (once), the herd in
#     the headlights, nine; each followed by its line from the book (note and toast); the lights
#     do not come again
#   - Continue: the book, its threads and what was told come back, nothing comes twice, the drawer
#     is out at the station, the book opens from Papers in its own view at the page it was left at
#   - the envelope: on the office counter, any chapter; held up to the lamp it changes, writes one
#     note, goes in Papers and on the board; the thread to the last page holds
#   - the drawer used twice in a moment slides out once; the knocks wait for 30 s with nothing else
#     going on; northbound the lights stand ahead of abreast, as southbound
#   - desktop 1280x720, phone 412x892 and a phone on its side (844x390) with touch: the whole page
#     on the screen; no console errors or warnings
# Usage: python3 tools/f_fieldbook.py OUTDIR
import asyncio, sys, json, os, math, re
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
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
    at(o) { const v = o.getWorldPosition(new V()); return [v.x, v.y, v.z]; },
    // stand `d` metres from a point on the floor, on walkable ground clear of colliders, looking at it
    stand(t, d, from) {
      const p = S47.player, r = p.radius + 0.02;
      const free = (x, z) => p.walkable(x, z) && !p.colliders.some((c) => x > c.minX - r && x < c.maxX + r && z > c.minZ - r && z < c.maxZ + r);
      for (let i = 0; i < 24; i++) {
        const ang = from + (i % 2 ? 1 : -1) * Math.ceil(i / 2) * Math.PI / 12;
        for (const k of [1, 0.85, 1.2, 0.7]) {
          const x = t[0] + Math.sin(ang) * d * k, z = t[2] + Math.cos(ang) * d * k;
          if (!free(x, z)) continue;
          p.place(x, z, Math.atan2(-(t[0] - x), -(t[2] - z))); S47.tick(0.05);
          const c = S47.camera.position;
          p.pitch = Math.atan2(t[1] - c.y, Math.hypot(t[0] - c.x, t[2] - c.z)); S47.tick(0.05);
          return [x, z];
        }
      }
      return null;
    },
    aimed() { const it = S47.game.d.inter.update(S47.camera); return it ? it.id : null; },
    fb() { const s = S47.ch3.s.fb; return s ? JSON.parse(JSON.stringify(s)) : null; },
    has(k) { return S47.townSeen.has(k); },
    count(re) { return S47.game.notes.filter((n) => new RegExp(re).test(n)).length; },
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
    if touch: await pg.tap('button[data-a=start]'); await begin_opening(pg, touch=True)
    else: await pg.click('button[data-a=start]'); await begin_opening(pg)
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS)
    await pg.evaluate("T.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')   # no pointer lock in the test
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
        async def click(sel): await pg.click(sel)
        notes = lambda: ev("S47.game.notes.slice()")
        fb = lambda: ev("T.fb()")
        has = lambda k: ev(f"T.has('{k}')")
        async def quiet(): await ev("S47.nightEvents['wait'] = 1e9")   # the night's other events out of the way
        # (the night's curve stands aside: the knocks come on the book's own terms here; story/curve.ts)
        await ev("S47.hold = true; S47.curve.free = true")
        # (the herd's own order with the book: the ninth cow is not put down at STATION 01 here; tools/f_ninth_cow.py has her)
        await ev("S47.ninth.off = true")
        await quiet()

        # ---------- not before chapter three ----------
        await ev("S47.jump('chapter2'); S47.tick(1.0); S47.setAbductions(false)"); await quiet()
        await ev("window.__p = S47.world.prepare('station01')")
        await pg.wait_for_function("!!S47.world.site", polling=300)
        await ev("S47.world.enter('station01'); S47.tick(0.2)")
        dr = await ev("T.at(S47.world.site.proxies.drawer)")
        lab = await ev("S47.game.d.inter.get('s1drawer').label()")
        check(await ev("S47.game.phase") == 'ch2' and lab is None and not await ev("S47.world.site.drawerOpen"), f'chapter two: the drawer in the hut is shut and offers nothing ({lab})')
        await ev("S47.world.enter('saro'); S47.tick(0.2)")

        # ---------- chapter three: driven out for real ----------
        await ev("S47.jump('chapter3'); S47.tick(1.0); S47.setAbductions(false)"); await quiet()
        check(await ev("S47.game.phase") == 'ch3' and await ev("S47.ch3.s.stage") == 'to-truck' and await fb() is None, 'chapter three starts; the book has not been read')
        await ev("S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.tick(0.1)")
        await ev("S47.world.driveOut()")
        for _ in range(240):
            await tick(0.25)
            if await ev("S47.world.driving"): break
            await pg.wait_for_timeout(100)
        await ev("S47.world.autopilot = 13")
        for _ in range(900):
            await tick(0.5)
            await quiet()
            if not await ev("S47.world.driving"): break
        await tick(1.0)
        lights = await ev("[...S47.townSeen].filter((k) => k.startsWith('mark:lights@')).map((k) => Number(k.split('@')[1]))")
        lt = await ev(f"(() => {{ const s = {lights[0] if lights else 0}; const h = Math.floor(s / 3600) % 24, m = Math.floor(s / 60) % 60; return String(h).padStart(2, '0') + ':' + String(m).padStart(2, '0'); }})()")
        check(await ev("S47.world.area") == 'station01' and await has('ev:lights') and len(lights) == 1,
              f'driven out to STATION 01 with the autopilot; the three lights along B came on the way, at {lt}')
        herd0 = await ev("S47.pasture.stage")
        print(f'  the herd before the book: {herd0}', flush=True)

        # ---------- the drawer, in the hut ----------
        dr = await ev("T.at(S47.world.site.proxies.drawer)")
        st = await ev(f"T.stand([{dr[0]}, {dr[1]}, {dr[2]}], 0.95, 0)")
        aim = await ev("T.aimed()")
        lab = await ev("S47.game.d.inter.get('s1drawer').label()")
        check(st is not None and aim == 's1drawer' and lab == 'Open the drawer', f'in the hut, below the lamp: the drawer can be aimed at ({aim}, "{lab}")')
        await ev("S47.input.zoomHeld = true"); await tick(0.8)
        hud = await ev("(() => { const e = document.querySelector('.hud .examine'); return e && e.classList.contains('show') ? e.textContent : null; })()")
        await ev("S47.input.zoomHeld = false"); await tick(0.4)
        check(hud and 'drawer under the bench' in hud, f'a closer look at it ({hud})')
        await shot('desk_01_drawer_shut')
        n0 = len(await notes())
        # (Use twice in a moment, as a player pressing E again while it slides: it opens once)
        await ev("S47.game.d.inter.get('s1drawer').use(); S47.game.d.inter.get('s1drawer').use()")
        for _ in range(8): await tick(0.2)
        await pg.wait_for_selector('.fbook')
        check(await ev("S47.world.site.drawerOpen") and await ev("T.toasts.filter((t) => t.includes('field book in green cloth')).length") == 1
              and await ev("document.querySelectorAll('.fbook').length") == 1, 'it slides out, once: a field book in green cloth, and it opens')
        hand = lambda: ev("document.querySelector('.fbook [data-hand]').textContent")
        cover = await hand()
        check('T. VEGA' in cover and 'FIELD BOOK 3' in cover and 'JUNE - JULY 47' in cover, 'the first page: T. VEGA / FIELD BOOK 3 / JUNE - JULY 47')
        await shot('desk_02_cover')
        # put back at page 3: not read
        await click('.fbook [data-a=next]'); await click('.fbook [data-a=next]')
        p3 = await hand()
        check(p3.startswith(f'Jul 1. {lt}.') and 'Then they turned off along B and went out.' in p3, f'page 3: the lights, at the minute I had them ({p3[:40]})')
        await click('.fbook [data-a=back]'); await tick(0.3)
        check(await fb() is None and len(await notes()) == n0, 'put back before the last page: not read, nothing written')
        check(await ev("S47.world.site.drawerOpen") and await ev("S47.game.d.inter.get('s1drawer').label()") == "T. Vega's field book", 'the drawer stays out, the book in it')
        # something seen meanwhile does not change it: the county car, through the hooks
        await ev("S47.townSeen.add('ev:car'); S47.townSeen.add('mark:car@' + Math.round(S47.game.clock)); S47.game.note('03:58. The deputy\\'s car on the shoulder of the highway north of Caliche: engine running, the door open, nobody in it.')")
        n0 = len(await notes())
        await ev("S47.game.d.inter.get('s1drawer').use()"); await tick(0.2)
        await pg.wait_for_selector('.fbook')
        nb = await ev("document.querySelector('.fbook [data-where]').textContent")
        check(nb == 'Page 1 of 9', f'opened again at the start ({nb})')
        pages = []
        for i in range(9):
            pages.append(await hand())
            if i in (3, 6): await shot(f'desk_03_page{i}')
            if i < 8:
                if i % 2: await click('.fbook [data-a=next]')
                else:
                    await ev("dispatchEvent(new KeyboardEvent('keydown', { code: 'ArrowRight' }))")
                await pg.wait_for_timeout(120)
        check(pages[6].startswith('Jul 7. The county car') and 'Nobody in it.' in pages[6], 'the car\'s page has no minute: the book is as it was when it was first opened')
        check(pages[3].startswith('Jul 3. The pasture') and '026. Nine head.' in pages[3] and 'Jul 4. Eight.' in pages[3], 'page 4: the herd on 026, nine; and the next day, eight')
        check(pages[4].startswith('Jul 5. Coming back') and 'Three knocks on the roof of the car, slow, from above.' in pages[4], 'page 5: three knocks on the roof of the car')
        check('Main Street out one by one' in pages[5] and 'same order' in pages[5], 'page 6: the lamps and the dogs')
        check(pages[8] == 'Three on plate 4.\nThere were two of us.', 'the last page: "Three on plate 4. There were two of us."')
        check(await ev("document.querySelector('.fbook [data-a=next]').disabled") and await ev("document.querySelector('.fbook [data-page]').classList.contains('last')"), 'it is the last page')
        # its two lines are two lines (the sentence does not break)
        rows = await ev("(() => { const h = document.querySelector('.fbook [data-hand]'), r = document.createRange(); r.selectNodeContents(h); return new Set([...r.getClientRects()].map((x) => Math.round(x.top))).size; })()")
        check(rows == 2, f'the two sentences stand on two lines ({rows})')
        await shot('desk_04_last_page')
        await click('.fbook [data-a=back]'); await tick(0.3)
        f = await fb()
        nn = (await notes())[n0:]
        check(f is not None and f['times'] == {'lights': lt} and sorted(f['had']) == ['car', 'lights'], f'read: kept with chapter three, the minute it has, and what had been seen ({f and f["times"]}, {f and f["had"]})')
        check(len(nn) == 1 and "T. Vega's field book, June and July 1947" in nn[0] and 'three lights along B' in nn[0] and 'the times he has are mine' in nn[0]
              and 'Not yet.' in nn[0] and 'Three on plate 4. There were two of us.' in nn[0], f'one note in my hand ({nn[:1]})')
        check('In July 1947, he wrote down tonight.' in ' '.join(await ev("T.toasts")), 'and the toast: "In July 1947, he wrote down tonight."')
        check('fieldbook' in await ev("S47.game.docs.map((d) => d.id)"), 'the book is in Papers')
        await tick(0.5, 4)
        gap = await ev(f"S47.game.clock - {lights[0] if lights else 0}")
        check(await ev("S47.pasture.stage") == 'still' and await ev("S47.lineLights.anyWay") and (herd0 != 'graze' or gap < 600),
              f'what it has comes on the way back: the herd has turned out there, far off ({herd0} before the book, {gap:.0f} s after the lights, so by the book), and the lights could come northbound')

        # ---------- the board ----------
        await ev("S47.game.openBoard('STATION 01', 'ch3')"); await tick(0.2)
        await pg.wait_for_selector('.board-wrap')
        cards = await ev("[...document.querySelectorAll('.bcard')].map((c) => [c.dataset.card, c.classList.contains('away'), c.textContent.replace(/\\s+/g, ' ').trim()])")
        cd = {c[0]: c for c in cards}
        check('fb' in cd and not cd['fb'][1] and await ev("document.querySelectorAll('[data-card=fb] .bline').length") == 6, 'on the board: the field book, with a pin on each of its six lines')
        check('tn.lights' in cd and not cd['tn.lights'][1] and 'turned off along B' in cd['tn.lights'][2], 'tonight\'s lights, from my notes, pinned')
        check(all(cd.get(k) and cd[k][1] and 'Not yet in mine.' in cd[k][2] for k in ('tn.cows', 'tn.knocks', 'tn.lamps')), 'the herd, the knocks and the lamps: not on the table yet, "In his book. Not yet in mine."')
        check('tn.car' in cd and not cd['tn.car'][1], 'the car, seen before it was read: pinned')
        bsay = lambda: ev("(document.querySelector('.board-wrap [data-say]')||{}).textContent || ''")
        async def thread(a, b):
            # a click on one pin and then the other, each brought into view first (the board pans)
            for n in (a, b):
                await ev(f"S47.game.board.view.show('{n}')"); await pg.wait_for_timeout(80)
                await pg.click(f'[data-node="{n}"]')
            return await bsay()
        r1 = await thread('fb.knocks', 'tn.lights')
        check('Not the same thing' in r1 and not f['links'], f'his knocks to my lights: does not hold ({r1})')
        r2 = await thread('fb.lights', 'tn.lights')
        f = await fb()
        check('He wrote it in July 1947. I saw it tonight.' in r2 and f['links'] == ['fb.lights|tn.lights'], f'his lights to mine: it holds ({r2})')
        r3 = await thread('fb.car', 'tn.car')
        check('He wrote it in July 1947. I saw it tonight.' in r3 and await ev("document.querySelectorAll('.bnote').length") >= 2, 'his car to mine: it holds; the notes stay on the board')
        await ev("S47.game.board.view.show('fb.knocks')"); await pg.wait_for_timeout(150)
        await shot('desk_05_board')
        await click('.board-wrap .lp-close'); await tick(0.2)

        # ---------- one thing at a time: no knocks while the night is not free ----------
        await ev("(() => { for (let i = 0; i < 80; i++) S47.fieldBook.update(1, { ok: false, driving: true, knock: () => { window.__k = true; return true; } }); })()")
        check(not await ev("!!window.__k"), 'driving 80 s with something else going on (the night not free): the knocks do not come')

        # ---------- the way back ----------
        n1 = len(await notes())
        await ev("S47.world.driveBack()")
        for _ in range(240):
            await tick(0.25)
            if await ev("S47.world.driving"): break
            await pg.wait_for_timeout(100)
        await ev("S47.world.autopilot = 12")
        t0, knockAt, knockArea, cowsAt, toldK, toldC = 0.0, None, None, None, False, False
        for _ in range(1200):
            await tick(0.25); t0 += 0.25
            if t0 % 20 < 0.3: await quiet()
            if knockAt is None and await has('ne:knocks1'): knockAt, knockArea = t0, await ev("S47.world.area + '/' + (S47.world.leg || '')")
            if cowsAt is None and await has('ev:cows1'): cowsAt = t0; await shot('desk_06_herd_on_the_way_back')
            if not await ev("S47.world.driving"): break
        for _ in range(12): await tick(0.5)
        nn = (await notes())[n1:]
        jt = ' '.join(await ev("T.toasts"))
        check(knockAt is not None and 18 <= knockAt <= 75 and await ev("T.count('Three knocks on the roof of the cab')") == 1,
              f'on the way back, {knockAt} s out of the gate ({knockArea}): three knocks on the roof of the cab, once')
        check(any('July 5, 1947: three knocks on the roof of the car. He wrote it in July 1947. I saw it tonight.' in n for n in nn)
              and 'T. Vega, July 5, 1947: "Three knocks on the roof of the car, slow, from above."' in jt, 'and then his line for it: a note, and the toast')
        check(cowsAt is not None and any('Nine head' in n for n in nn) and any('July 3, 1947: the herd on 026, nine head' in n for n in nn)
              and 'Took a bearing on them: 026. Nine head.' in jt, f'the herd in the headlights, nine, {cowsAt} s out; and his line for it')
        check(await ev("[...S47.townSeen].filter((k) => k.startsWith('mark:lights@')).length") == 1 and await ev("T.count('turned off along B and went out')") == 1,
              'the lights, seen already, do not come again')
        check(await ev("S47.world.area") == 'saro', 'back at SARO')
        told = (await fb())['told']
        check(sorted(told) == ['cows', 'knocks'], f'what was told is kept ({told})')

        # ---------- Continue ----------
        await ev("S47.game.d.ui.close()"); await tick(0.3)
        await ev("S47.saveNow('manual', 0)")
        await pg.wait_for_timeout(800)
        notes_before = await notes()
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev(HELPERS); await ev("T.hook()"); await ev("S47.ninth.off = true; S47.hold = true; S47.tick(0.5)"); await quiet()
        await pg.add_style_tag(content='.lockhint{display:none !important}')
        f = await fb()
        check(f is not None and f['times'] == {'lights': lt} and sorted(f['told']) == ['cows', 'knocks'] and sorted(f['links']) == ['fb.car|tn.car', 'fb.lights|tn.lights'],
              f'Continue: the book, its minutes, what was told and both threads come back ({f and f["links"]})')
        check('fieldbook' in await ev("S47.game.docs.map((d) => d.id)") and len(await notes()) == len(notes_before), 'the book is in Papers again, no note is doubled')
        await ev("S47.setAbductions(false)")
        await tick(0.5, 30)
        check(await ev("T.count('field book')") == sum(1 for n in notes_before if 'field book' in n), 'nothing of it is written again after Continue')
        # the book from the notebook: its own view, at the page it was put back at
        await ev("S47.game.openNotebook()"); await tick(0.1)
        await click('.notebook [data-t=papers]')
        await click('.notebook [data-d=fieldbook]'); await tick(0.1)
        check(await ev("!!document.querySelector('.fbook') && document.querySelector('.fbook [data-where]').textContent") == 'Page 9 of 9', 'from Papers it opens in its own view, at the last page')
        await click('.fbook [data-a=back]'); await tick(0.2)
        await ev("window.__p = S47.world.prepare('station01')")
        await pg.wait_for_function("!!S47.world.site", polling=300)
        await ev("S47.world.enter('station01'); S47.world.placeAtStation(); S47.tick(0.3)")
        check(await ev("S47.world.site.drawerOpen"), 'at the station the drawer is out, as it was left')
        await ev("S47.world.enter('saro'); S47.tick(0.2)")

        # ---------- the envelope on the office counter ----------
        env = await ev("T.at(S47.world.motel.proxies.envelope)")
        ofs = await ev("(() => { const a = S47.world.motel.anchors.officeInside; return [a.x, a.z]; })()")
        st = await ev(f"T.stand([{env[0]}, {env[1]}, {env[2]}], 0.9, Math.atan2({ofs[0]} - {env[0]}, {ofs[1]} - {env[2]}))")
        aim = await ev("T.aimed()")
        lab = await ev("S47.game.d.inter.get('court:envelope').label()")
        check(st is not None and aim == 'court:envelope' and lab == 'Sealed envelope', f'in the motel office: the envelope on the counter ({aim}, "{lab}")')
        await shot('desk_07_office_counter')
        n2 = len(await notes())
        await ev("S47.game.d.inter.get('court:envelope').use()"); await tick(0.1)
        await pg.wait_for_selector('.fenv'); await pg.wait_for_timeout(400)
        t1 = await ev("document.querySelector('.fenv .text').textContent")
        check('Plate 4. 1 print. 35c.' in t1 and 'never opened' in t1 and not await has('env:lamp'), 'its face: never opened, "Plate 4. 1 print. 35c."; nothing written yet')
        await shot('desk_08_envelope')
        await click('.fenv [data-a=lamp]'); await pg.wait_for_timeout(1400)
        t2 = await ev("document.querySelector('.fenv .text').textContent")
        check(await ev("document.querySelector('.fenv').classList.contains('up')") and 'three people standing by it' in t2 and 'There were two of them' in t2,
              'held up to the lamp: a tripod and three people by it, the third not as sharp; there were two of them')
        await shot('desk_09_envelope_lamp')
        await click('.fenv [data-a=lamp]'); await click('.fenv [data-a=lamp]'); await pg.wait_for_timeout(200)
        await click('.fenv [data-a=back]'); await tick(0.2)
        nn = (await notes())[n2:]
        check(await has('env:lamp') and len(nn) == 1 and 'Plate 4. 1 print. 35c.' in nn[0] and 'three small figures' in nn[0] and 'two of them' in nn[0], f'one note, however often it is held up ({nn})')
        check('env4' in await ev("S47.game.docs.map((d) => d.id)") and await ev("S47.game.d.inter.get('court:envelope').label()") == 'Envelope, plate 4', 'in Papers; the label says what it is now')
        await ev("S47.game.openBoard()"); await tick(0.2)
        await pg.wait_for_selector('.board-wrap')
        r4 = await thread('fb.plate', 'env4')
        check('three by the tripod' in r4 and 'two of them' in r4, f'on the board: the last page to the envelope, it holds ({r4})')
        await click('.board-zoom [data-z=out]'); await click('.board-zoom [data-z=out]'); await ev("S47.game.board.view.show('fb.plate')"); await pg.wait_for_timeout(150)
        await shot('desk_10_board_plate')
        await click('.board-wrap .lp-close'); await tick(0.2)
        # the lights northbound: only once the book has them, and not seen (event 7 is southbound)
        LL = "(() => { const L = S47.lineLights, V = S47.three.Vector3, tr = new V(-24, 0, 236), o = { ok: false, truck: tr, clock: S47.game.clock, ground: () => 0 }; L.reset(false); L.anyWay = %s; L.update(0.1, o); o.ok = true; tr.z = 235; L.update(0.1, o); tr.z = 234; L.update(0.1, o); const st = L.state, z = S47.scene.getObjectByName('lineLights').geometry.getAttribute('position').getZ(0); L.reset(true); return [st, z < tr.z]; })()"
        nb = await ev(LL % 'true')
        check((await ev(LL % 'false'))[0] == 'none' and nb[0] == 'pace' and nb[1], f'northbound, the lights along B come only when the book has foretold them, and stand a little ahead, as they do southbound ({nb})')
        bad = [e for e in errs if 'favicon' not in e]
        check(not bad, f'desktop: no console errors or warnings ({bad[:4]})')
        await ctx.close()

        # ======================= phone =======================
        ctx, pg, errs = await page(b, 412, 892, True)
        ev = pg.evaluate
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        await ev("S47.ninth.off = true; S47.hold = true; S47.nightEvents['wait'] = 1e9")
        await ev("S47.jump('chapter3'); S47.tick(1.0); S47.setAbductions(false); S47.nightEvents['wait'] = 1e9")
        # seen tonight before the station, through the hooks: the herd (nine) and the lamps in Caliche
        await ev("""(() => { const s = S47.townSeen, c1 = 3 * 3600 + 21 * 60 + 5, c2 = 3 * 3600 + 33 * 60 + 40;
          s.add('ev:cows1'); s.add('mark:cows@' + c1); s.add('cows:on'); s.add('ev:lamps'); s.add('mark:lamps@' + c2);
          S47.game.note('03:21. The pasture south of the motel: the whole herd standing facing the same way, north-north-east, on 026. Nine head. Not one of them moved, with the headlights right on them.');
          S47.game.note('03:33. Main Street, Caliche: the street lights out one by one, south to north, and the dogs barking in the same order. Back on in the same order.');
          S47.game.clock = 3 * 3600 + 50 * 60; S47.pasture.reset(); })()""")
        await ev("window.__p = S47.world.prepare('station01')")
        await pg.wait_for_function("!!S47.world.site", polling=300)
        await ev("S47.world.placeTruck('station01'); S47.world.enter('station01'); S47.world.placeAtStation(); S47.tick(0.3); S47.ch3.arrivedStation(); S47.tick(0.3)")
        dr = await ev("T.at(S47.world.site.proxies.drawer)")
        await ev(f"T.stand([{dr[0]}, {dr[1]}, {dr[2]}], 0.95, 0)")
        # a tap on the drawer, where it is on the screen
        xy = await ev(f"(() => {{ const v = new S47.three.Vector3({dr[0]}, {dr[1]}, {dr[2]}).project(S47.camera); return [(v.x + 1) / 2 * innerWidth, (1 - v.y) / 2 * innerHeight]; }})()")
        check(await ev(f"(() => {{ const it = S47.game.d.inter.pick(S47.camera, new S47.three.Vector2({(xy[0] / 412) * 2 - 1}, {-(xy[1] / 892) * 2 + 1}), 2.6); return it && it.id; }})()") == 's1drawer',
              'phone: a tap where the drawer is picks it')
        await ev("S47.game.d.inter.get('s1drawer').use()")
        for _ in range(8): await ev("S47.tick(0.2)")
        await pg.wait_for_selector('.fbook')
        await shot('phone_01_cover')
        await pg.tap('.fbook [data-a=next]'); await pg.wait_for_timeout(150)
        await pg.tap('.fbook [data-a=next]'); await pg.wait_for_timeout(150)
        await pg.tap('.fbook [data-a=next]'); await pg.wait_for_timeout(250)
        hand = lambda: ev("document.querySelector('.fbook [data-hand]').textContent")
        p4 = await hand()
        check(p4.startswith('Jul 3. 03:21.') and 'Jul 4. Eight.' in p4, f'phone: the herd\'s page has my minute, and "Eight." without one ({p4[:30]})')
        fits = await ev("(() => { const p = document.querySelector('.fbook [data-page]'), h = p.querySelector('.hand'); return h.offsetTop + h.offsetHeight <= p.clientHeight + 1 && p.getBoundingClientRect().right <= innerWidth; })()")
        check(fits, 'phone: the page\'s writing fits on the page, and the page on the screen')
        await shot('phone_02_herd_page')
        # a tap on the right of the page turns it too
        box = await pg.locator('.fbook [data-page]').bounding_box()
        for _ in range(5):
            await pg.tap('.fbook [data-page]', position={'x': box['width'] * 0.8, 'y': box['height'] * 0.5}); await pg.wait_for_timeout(150)
        check(await hand() == 'Three on plate 4.\nThere were two of us.', 'phone: taps on the page turn it to the last page')
        rows = await ev("(() => { const h = document.querySelector('.fbook [data-hand]'), r = document.createRange(); r.selectNodeContents(h); return new Set([...r.getClientRects()].map((x) => Math.round(x.top))).size; })()")
        check(rows == 2, f'phone: the two sentences stand on two lines ({rows})')
        await shot('phone_03_last_page')
        await pg.tap('.fbook [data-a=back]'); await ev("S47.tick(0.3)")
        f = await fb()
        check(f is not None and f['times'] == {'cows': '03:21', 'lamps': '03:33'}, f'phone: read, with the herd\'s and the lamps\' minutes ({f and f["times"]})')
        await ev("S47.game.openBoard('STATION 01', 'ch3')"); await ev("S47.tick(0.2)")
        await pg.wait_for_selector('.board-wrap')
        # (the board is panned and zoomed on a phone; the pins are clicked where they are, the
        # same click handler a tap ends in)
        for n in ('fb.lamps', 'tn.lamps'):
            await ev(f"S47.game.board.view.show('{n}')"); await pg.wait_for_timeout(80)
            await pg.tap(f'[data-node="{n}"]')
        r = await ev("(document.querySelector('.board-wrap [data-say]')||{}).textContent || ''")
        check('He wrote it in July 1947. I saw it tonight.' in r, f'phone: tap, tap: his lamps to mine, it holds ({r})')
        await shot('phone_04_board')
        await pg.tap('.board-wrap .lp-close'); await ev("S47.tick(0.2)")
        await ev("S47.world.enter('saro'); S47.tick(0.2); S47.fieldBook.useEnvelope(); S47.tick(0.1)")
        await pg.wait_for_selector('.fenv')
        await pg.tap('.fenv [data-a=lamp]'); await pg.wait_for_timeout(1400)
        await shot('phone_05_envelope_lamp')
        check(await ev("document.querySelector('.fenv').classList.contains('up') && document.querySelector('.fenv .page').getBoundingClientRect().right <= innerWidth"), 'phone: held up to the lamp, on the screen')
        await pg.tap('.fenv [data-a=back]')
        bad = [e for e in errs if 'favicon' not in e]
        check(not bad, f'phone: no console errors or warnings ({bad[:4]})')
        await ctx.close()

        # ======================= phone on its side =======================
        ctx, pg, errs = await page(b, 844, 390, True)
        ev = pg.evaluate
        await ev("S47.ninth.off = true; S47.hold = true; S47.jump('chapter3'); S47.tick(1.0); S47.setAbductions(false); S47.nightEvents['wait'] = 1e9")
        await ev("S47.fieldBook.openBook(0)"); await pg.wait_for_selector('.fbook'); await pg.wait_for_timeout(300)
        GEO = """(() => { const p = document.querySelector('.fbook [data-page]').getBoundingClientRect(), h = document.querySelector('.fbook [data-hand]').getBoundingClientRect();
          const n = document.querySelector('.fbook [data-a=next]').getBoundingClientRect(), t = document.querySelector('.fbook .text').getBoundingClientRect();
          return p.top >= -1 && p.bottom <= innerHeight + 1 && h.top >= p.top && h.bottom <= p.bottom + 1 && p.right <= t.left + 2 && n.bottom <= innerHeight + 1; })()"""
        fit = []
        for i in range(9):
            fit.append(await ev(GEO))
            if i == 3: await pg.screenshot(path=f'{OUT}/land_01_herd_page.png')
            if i < 8: await pg.tap('.fbook [data-a=next]'); await pg.wait_for_timeout(150)
        check(all(fit), f'phone on its side (844x390): each page whole on the screen, its writing on it, the words beside it ({fit})')
        await pg.tap('.fbook [data-a=back]'); await ev("S47.tick(0.3)")
        await ev("S47.world.enter('saro'); S47.tick(0.2); S47.fieldBook.useEnvelope(); S47.tick(0.1)")
        await pg.wait_for_selector('.fenv')
        await pg.tap('.fenv [data-a=lamp]'); await pg.wait_for_timeout(1400)
        await pg.screenshot(path=f'{OUT}/land_02_envelope_lamp.png')
        check(await ev("(() => { const r = document.querySelector('.fenv .page').getBoundingClientRect(); return r.top >= -1 && r.bottom <= innerHeight + 1 && r.right <= innerWidth; })()"), 'on its side: the envelope against the lamp whole on the screen')
        await pg.tap('.fenv [data-a=back]')
        bad = [e for e in errs if 'favicon' not in e]
        check(not bad, f'phone on its side: no console errors or warnings ({bad[:4]})')
        await ctx.close()
        await b.close()
    n = sum(1 for c in checks if c[0] == 'PASS')
    print(f'\n{n}/{len(checks)} PASS', flush=True)
    with open(f'{OUT}/results.json', 'w') as fh: json.dump(checks, fh, indent=1)
    sys.exit(0 if n == len(checks) else 1)

asyncio.run(main())
