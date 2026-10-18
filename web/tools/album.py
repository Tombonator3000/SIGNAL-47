# The free camera and the album (core/album.ts, story/albumSubjects.ts, ui/album.ts; todo.md,
# after They Are Here). Once the camera is had, the shutter takes a free frame anywhere a case
# frame is not to be had; the case frames keep the shutter at their subjects. Each free frame is
# captioned from what is in it (the place, the clock, the most prominent known things, and the
# things that should not be there: the cardboard alien, a glimpse at the edge of the view, the
# someone in a SARO jacket where the time was lost, someone on the film only), 36 in a night, kept
# with the case in IndexedDB, and shown in the notebook's album; a print opens big with a click
# or a tap. Continue brings the album back. Storage sizes are printed.
# Usage: python3 tools/album.py OUTDIR [WxH]
#        S47_NO_IDB=1 python3 tools/album.py OUTDIR   (no IndexedDB: everything in localStorage)
import asyncio, sys, json, os, math, base64
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
NO_IDB = os.environ.get('S47_NO_IDB') == '1'
IDB_GET = """((store, key) => new Promise((ok) => { const r = indexedDB.open('s47', 2);
  r.onsuccess = () => { const g = r.result.transaction(store).objectStore(store).get(key);
    g.onsuccess = () => { ok(g.result ?? null); r.result.close(); }; g.onerror = () => ok(null); }; r.onerror = () => ok(null); }))"""
IDB_KEYS = """((store) => new Promise((ok) => { const r = indexedDB.open('s47', 2);
  r.onsuccess = () => { const g = r.result.transaction(store).objectStore(store).getAllKeys();
    g.onsuccess = () => { ok(g.result ?? []); r.result.close(); }; g.onerror = () => ok([]); }; r.onerror = () => ok([]); }))"""
NOBODY = 'Nobody was there when I took it.'
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

LAUNCH = ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required']

async def page(b, touch=False, size=(W, H)):
    ctx = await b.new_context(viewport={'width': size[0], 'height': size[1]}, has_touch=touch, is_mobile=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    if NO_IDB: await pg.add_init_script("Object.defineProperty(window, 'indexedDB', { value: undefined });")
    return ctx, pg, errs

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=LAUNCH)
        ctx, pg, errs = await page(b)
        await pg.goto(URL)
        try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
        except Exception: pass
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await pg.wait_for_function('S47.started()', polling=200)
        ev = pg.evaluate
        async def tick(s=0.1): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def place(x, z, yaw, pitch=0.0): await ev(f"S47.player.place({x},{z},{yaw}); S47.player.pitch={pitch}; S47.tick(0.05)")
        async def look_at(px, pz, tx, ty, tz, dyaw=0.0):
            yaw = math.atan2(-(tx - px), -(tz - pz)) + dyaw
            pitch = math.atan2(ty - 1.62 - await ev("S47.player.floorY || 0"), math.hypot(tx - px, tz - pz))
            await place(px, pz, yaw, pitch)
        async def raise_cam():
            if not await ev("S47.fcam.raised"): await pg.keyboard.press('KeyC'); await tick()
        async def lower_cam():
            if await ev("S47.fcam.raised"): await pg.keyboard.press('KeyC'); await tick()
        n_shots = lambda: ev("S47.album.shots.length")
        last = lambda: ev("S47.album.shots.at(-1)")
        vf_free = lambda: ev("document.querySelector('.vf-free').textContent")
        vf_status = lambda: ev("document.querySelector('.vf-status').textContent")
        async def expose(name=None):
            n = await n_shots()
            await pg.keyboard.press('Space'); await tick()
            s = await last() if await n_shots() > n else None
            if s: print(f"   frame: \"{s['text']}\" / {s['place']} / {len(s['url']) / 1024:.0f} kB / {json.dumps(await ev('S47.album.last'))}", flush=True)
            # the free frame itself, to look at
            if s and name: open(f'{OUT}/{name}.jpg', 'wb').write(base64.b64decode(s['url'].split(',', 1)[1]))
            return s

        # ---------- chapter one: the camera, and the case frame keeps the shutter at its subject ----------
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter1'); S47.tick(1.2)")
        await ev("S47.album.restore(null)")
        await raise_cam()
        check(not await ev("S47.fcam.raised"), 'before the camera is had, C raises nothing')
        await place(4.7, 0.75, -1.5708, -0.55)
        await ev("S47.game.d.inter.get('fieldCamera').use()"); await tick(0.2)
        check(await ev("S47.fcam.have"), 'field camera collected')
        await raise_cam()
        check('FREE FRAME // 36 LEFT' in await vf_free() and await ev("document.querySelector('.vf-free').classList.contains('ok')"), f'viewfinder: a free frame is ready, 36 left ({await vf_free()})')
        s = await expose()
        check(s is not None and s['place'] == 'Control room' and s['url'].startswith('data:image/jpeg'), 'in the control room the shutter takes a free frame')
        check(await ev("!S47.ch1.s.f1"), 'and no case frame')
        await shot('a01_control_room_vf')
        await lower_cam()
        # the motor log, then the apron: aimed away from S-03 it is a refused case frame, not a free one
        await ev("S47.doors.set('east', true, true)"); await tick(0.5)
        await place(11.2, -11.0, -1.5708, -0.2)
        await ev("S47.game.d.inter.get('motorBus').use()"); await tick(0.2)
        await pg.keyboard.press('KeyE'); await tick(0.2)
        check(await ev("S47.ch1.s.log"), 'S-03 log read')
        await place(10.4, -11.6, 3.14, 0.0)
        await raise_cam()
        n0, rej0 = await n_shots(), await ev("S47.ch1.s.rejected")
        check('ALIGN' in await vf_status() and 'LEFT' in await vf_free() and not await ev("document.querySelector('.vf-free').classList.contains('ok')"), f'on the apron the case frame has the shutter ({await vf_status()} / {await vf_free()})')
        await pg.keyboard.press('Space'); await tick()
        toast = await ev("document.querySelector('.toasts').textContent")
        check(await n_shots() == n0 and await ev("S47.ch1.s.rejected") == rej0 + 1 and 'No film used' in toast, 'misaimed on the apron: refused, no free frame taken')
        dish = await ev("(()=>{const d=S47.ext.dishes.find(d=>d.id==='S-03'); const c=S47.ext.dishArray.feedPosition(d).lerp(new S47.three.Vector3(d.x, 12.4*d.scale, d.z), 0.5); return [c.x,c.y,c.z]})()")
        await lower_cam()
        yaw = math.atan2(-(dish[0] - 10.4), -(dish[2] + 11.6)); pitch = math.atan2(dish[1] - 1.62, math.hypot(dish[0] - 10.4, dish[2] + 11.6)) - 0.06
        await place(10.4, -11.6, yaw, pitch); await raise_cam()
        check(await vf_status() == 'REFERENCE IN FRAME // READY', 'S-03 framed from the apron')
        await pg.keyboard.press('Space'); await tick(0.8)
        check(await ev("!!S47.ch1.s.f1 && S47.ch1.s.stage === 'develop-first'") and await n_shots() == n0, 'frame 01 exposed as before, and the album untouched')
        await lower_cam()
        # off the apron, waiting on the lab: a free frame again
        await place(9.0, -4.0, 0.0, 0.1); await raise_cam()
        s = await expose()
        check(s is not None and await ev("!S47.ch1.s.f2"), f'off the apron with frame 01 taken: a free frame ({await vf_status()})')
        await lower_cam()

        # ---------- chapter two at SARO: what the captions name ----------
        await ev("S47.jump('chapter2'); S47.tick(1.5)")
        await ev("S47.album.restore(null)")
        check(await ev("S47.fcam.have") and await ev("S47.game.phase") == 'ch2', 'chapter two: the camera is had')
        await ev("S47.doors.set('east', true, true)"); await tick(0.5)
        # the receivers in the control room
        rack = await ev("(()=>{const p=S47.room.objs.rack.getWorldPosition(new S47.three.Vector3()); return [p.x,p.y,p.z]})()")
        await look_at(rack[0], rack[2] + 3.0 if rack[2] < 0 else rack[2] - 3.0, rack[0], rack[1], rack[2])
        await raise_cam(); s = await expose('f_room')
        check(s and s['place'] == 'Control room' and s['text'] != '', f"control room: something named ({s and s['text']})")
        await lower_cam()
        # the antennas from the yard
        await place(9.5, -6.0, 0.0, 0.18); await raise_cam(); s = await expose('f_yard')
        check(s and ('antennas' in s['text'] or 'S-' in s['text']) and s['place'] == 'Service yard', f"the yard looking north: the antennas ({s and s['text']})")
        await shot('a02_yard_vf')
        await lower_cam()
        # the truck on its pad
        tk = await ev("(()=>{const t=S47.world.truckHere(); if(!t) return null; const p=t.group.position; return [p.x,p.y,p.z]})()")
        check(tk is not None, 'the truck is here')
        if tk:
            await look_at(9.5, -6.0, tk[0], tk[1] + 1.0, tk[2])
            await raise_cam(); s = await expose()
            check(s and 'truck' in s['text'], f"the truck from the yard, {math.hypot(tk[0] - 9.5, tk[2] + 6):.0f} m ({s and s['text']}, {s and s['place']})")
            await lower_cam()
        # the sign of Sierra Motor Court from the highway's shoulder
        sign = await ev("(()=>{const o=S47.scene.getObjectByName('Sierra Motor Court'); if(!o) return null; const p=o.getWorldPosition(new S47.three.Vector3()); return [p.x,p.y,p.z]})()")
        if sign:
            await look_at(-26.0, 120.0, sign[0], sign[1], sign[2]); await tick(0.3)
            await raise_cam(); s = await expose()
            check(s and 'Sierra Motor Court' in s['text'], f"the motel's sign down the highway ({s and s['text']}, {s and s['place']})")
            await lower_cam()

        # ---------- the things that should not be there ----------
        # the cardboard alien, put out of the view, then turned to (core/cutouts.ts)
        # (it is put anywhere the player can walk, so a wall may stand between: put it again until it can be seen)
        placed = framed = False
        for _ in range(30):
            await place(9.5, -6.0, 0.0, 0.0)
            await ev("S47.cutouts.reset(); S47.cutouts.place(S47.cutoutIn())")
            if not await ev("S47.cutouts.up === 'foot'"): continue
            placed = True
            c = await ev("[S47.cutouts.position.x, S47.cutouts.position.y, S47.cutouts.position.z]")
            await look_at(9.5, -6.0, c[0], c[1] + 0.8, c[2], dyaw=0.12)
            await raise_cam()
            if any('cardboard' in o for o in (await ev("S47.album.peek()"))['odd']): framed = True; break
            await lower_cam()
        check(placed, 'the cardboard alien is put somewhere out of the view')
        check(framed, 'and it can be framed from the yard')
        if framed:
            s = await expose('f_cutout')
            check(s and 'cardboard alien' in s['text'] and s.get('odd'), f"photographed: the cardboard alien gets its own line ({s and s['text']})")
            await shot('a03_cutout_vf')
            await lower_cam()
            await ev("S47.cutouts.reset()")
        # one of them, at the edge of the view (core/glimpses.ts). Its places at SARO stand 180 m and
        # more off behind the far antennas: on the print that is a speck of three lines, not written
        # about (the glimpse kept by the film is tried in Caliche, below)
        await place(-6.0, -30.0, 0.0, 0.0)
        far = await ev("S47.glimpses.now(S47.glimpseIn(), 'S-11')")
        if far:
            g = await ev("(()=>{const g=S47.scene.getObjectByName('glimpse'); return [g.position.x, g.position.y, g.position.z]})()")
            await look_at(-6.0, -30.0, g[0], g[1] + 0.6, g[2], dyaw=math.radians(28))
            await raise_cam(); await tick(0.05)
            d = math.hypot(g[0] + 6.0, g[2] + 30.0)
            check(await ev("!!S47.glimpses.up") and NOBODY not in (await ev("S47.album.peek()"))['odd'], f'a glimpse {d:.0f} m off is only a speck on the print: no line for it')
            await lower_cam()
        else: check(False, 'a glimpse at S-11 seen from the walk')
        await ev("S47.glimpses.reset()")
        # someone on the film only, standing at the edge of the frame out in the night
        await place(9.5, -6.0, 0.6, 0.0); await raise_cam()
        await ev("S47.album.forceGhost = true")
        s = await expose('f_film_only')
        lg = await ev("S47.album.last")
        check(s and lg['ghost'] and NOBODY in s['text'], f"on the film only: \"{s and s['text']}\" (placed: {lg and lg['ghost']})")
        check(await ev("!S47.scene.getObjectByName('album:ghost').visible"), 'and not there to the eye after the exposure')
        await shot('a05_after_ghost')
        await lower_cam()
        # the someone in a SARO jacket where the time was lost, and the ring of frost (story/abduction.ts)
        await ev("""S47.abduction().restore({ count: 1, last: S47.game.clock - 3000, spots: [{ scene: 'saro', take: [-40, S47.world.grounds.floorAt(-40, -70), -70], back: [-12, S47.world.grounds.floorAt(-12, -40), -40], clock: S47.game.clock - 3000 }] })""")
        await place(9.5, -6.0, 0.0, 0.0); await tick(0.2)
        check(await ev("S47.scene.children.some(o => o.name === 'abduct:record' && o.visible)"), 'the recording stands where the time was lost')
        await look_at(-4.0, -30.0, -40.0, await ev("S47.world.grounds.floorAt(-40, -70)") + 1.1, -70.0)
        await raise_cam(); s = await expose('f_jacket')
        check(s and 'SARO jacket' in s['text'], f"photographed from {math.hypot(36, 40):.0f} m: \"{s and s['text']}\"")
        await lower_cam()
        await look_at(-4.0, -30.0, -12.0, await ev("S47.world.grounds.floorAt(-12, -40)"), -40.0, dyaw=0.5)
        await raise_cam(); s = await expose('f_ring')
        check(s and 'ring of flattened grass' in s['text'], f"and the ring: \"{s and s['text']}\"")
        await lower_cam()
        await ev("S47.abduction().restore(null)")
        check(await ev("S47.album.left") == 36 - await n_shots(), 'every free frame counts on the roll')

        # ---------- the roll runs out ----------
        await place(9.5, -6.0, 0.0, 0.1); await raise_cam()
        t0 = await ev("performance.now()")
        while await ev("S47.album.left") > 0:
            await ev("S47.player.yaw += 0.17; S47.tick(0.05)")
            if not await expose(): break
        dt = (await ev("performance.now()") - t0) / 1000
        check(await n_shots() == 36 and await ev("S47.album.left") == 0, f'36 free frames in a night ({await n_shots()})')
        print(f'the last frames took {dt:.1f} s in the software renderer', flush=True)
        check('NO FREE FRAMES LEFT' in await vf_free(), f'viewfinder: none left ({await vf_free()})')
        await pg.keyboard.press('Space'); await tick()
        check(await n_shots() == 36 and 'No free frames left' in await ev("document.querySelector('.toasts').textContent"), 'the 37th: no film')
        await lower_cam()
        sizes = await ev("S47.album.shots.map(s => s.url.length)")
        widths = await ev("Promise.all(S47.album.shots.slice(0, 3).map(s => new Promise(ok => { const i = new Image(); i.onload = () => ok(i.naturalWidth); i.src = s.url; })))")
        print(f'free frames: {len(sizes)}, {sum(sizes) / 1024:.0f} kB as data URLs, {min(sizes) / 1024:.1f} to {max(sizes) / 1024:.1f} kB each, {widths[0]} px wide', flush=True)
        check(all(w == (320 if NO_IDB else 640) for w in widths), f'{"320" if NO_IDB else "640"} px wide ({widths})')
        check(max(sizes) < (40000 if NO_IDB else 120000), f'small enough to keep ({max(sizes) / 1024:.0f} kB at most)')

        # ---------- the album in the notebook ----------
        await ev("S47.game.openNotebook()")
        await pg.click('.notebook [data-t=photos]')
        check((await ev("document.querySelector('.notebook [data-t=photos]').textContent")).startswith('Album'), 'the Photos tab is the album')
        check(await ev("document.querySelectorAll('.notebook .nb-page.on .al-print').length") == 36, 'the album shows the 36 free frames')
        caps = await ev("[...document.querySelectorAll('.notebook .nb-page.on .al-print .al-cap')].slice(0, 4).map(c => c.textContent.trim())")
        print('captions:', caps, flush=True)
        check(all(c for c in caps), 'each print has a caption')
        await shot('a06_album')
        await pg.click('.notebook .nb-page.on .al-print >> nth=2')
        check(await ev("!!document.querySelector('.al-view .al-big img')"), 'a click opens the print big')
        src0 = await ev("document.querySelector('.al-view .al-big img').src.length")
        await shot('a07_big')
        big_cap = lambda: ev("document.querySelector('.al-view .al-big .al-cap').textContent.trim()")
        check(await big_cap() == caps[2], 'the third print, big')
        await pg.keyboard.press('ArrowRight'); await tick(0.05)
        check(await big_cap() == caps[3] and await ev("!!document.querySelector('.notebook')"), 'the arrow key turns to the next print, the notebook stays')
        await pg.keyboard.press('Escape'); await tick(0.05)
        check(await ev("!document.querySelector('.al-view') && !!document.querySelector('.notebook')"), 'Escape puts the print back and leaves the album open')
        await pg.click('.notebook .nb-page.on .al-print >> nth=0')
        await pg.click('.al-view', position={'x': 8, 'y': H - 8})
        check(await ev("!document.querySelector('.al-view') && !!document.querySelector('.notebook')"), 'a click outside the print puts it back')
        await ev("S47.game.d.ui.close()"); await tick()
        kit = await ev("S47.kitItems().find(k => k.name === 'Field camera')")
        check(kit and '0 of 36 free frames left' in kit['state'], f"the kit says so ({kit and kit['state']})")

        # ---------- saved with the case, and back with Continue ----------
        urls = await ev("S47.album.shots.map(s => s.url)")
        texts = await ev("S47.album.shots.map(s => s.text)")
        m = await ev("S47.saveNow('manual', 0)")
        if NO_IDB:
            raw = await ev(f"localStorage.getItem('s47.save.{m['id']}') || ''")
            total = await ev("(() => { let n = 0; for (let i = 0; i < localStorage.length; i++) { const k = localStorage.key(i); n += k.length + (localStorage.getItem(k) || '').length; } return n; })()")
            rec = json.loads(raw)
            refs = [s['url'] for s in rec['state']['album']['shots']]
            check(len(refs) == 36 and all(r.startswith('ls:p') for r in refs), 'without IndexedDB the save points at the 36 prints in localStorage')
            check(len(json.dumps(rec['state'])) < 60000, 'the state stays small without IndexedDB too')
            keys = await ev("Object.keys(localStorage).filter(k => k.startsWith('s47.photo.'))")
            stored = await ev(f"JSON.parse(localStorage.getItem('s47.photo.{refs[5][3:]}'))")
            check(stored == urls[5], 'a print is stored once in localStorage, under its content key')
            print(f'save in localStorage: {len(raw) / 1024:.0f} kB; localStorage in all: {total / 1024:.0f} kB', flush=True)
            # how many saves of this night fit: the three manual ones and three autosaves
            ok_saves, fail = 1, None
            for kind, slot in [('manual', 1), ('manual', 2), ('auto', 0), ('auto', 1), ('auto', 2)]:
                try: await ev(f"S47.saveNow('{kind}', {slot})"); ok_saves += 1
                except Exception as e: fail = str(e).split('\n')[0]; break
            total = await ev("(() => { let n = 0; for (let i = 0; i < localStorage.length; i++) { const k = localStorage.key(i); n += k.length + (localStorage.getItem(k) || '').length; } return n; })()")
            print(f'{ok_saves} of 6 saves written, localStorage {total / 1024:.0f} kB in all{"; then: " + fail if fail else ""}', flush=True)
            check(ok_saves == 6, f'all six saves of a case fit with a full roll ({ok_saves})')
            # Replacing the old autosaves can also free prints from the earlier chapter-one run.
            after_keys = await ev("Object.keys(localStorage).filter(k => k.startsWith('s47.photo.'))")
            check(set(after_keys) <= set(keys), 'six saves store no print twice (unused older prints may be removed)')
        else:
            rec = await ev(IDB_GET + f"('saves', '{m['id']}')")
            state = rec['state']
            size = len(json.dumps(state))
            refs = [s['url'] for s in state['album']['shots']]
            check(len(refs) == 36 and all(r.startswith('idb:p') for r in refs), 'the save points at the 36 prints in IndexedDB')
            check(size < 60000, f'and stays small ({size / 1024:.1f} kB)')
            keys = await ev(IDB_KEYS + "('photos')")
            stored = await ev(IDB_GET + f"('photos', '{refs[5][4:]}')")
            check(stored == urls[5], 'a print is stored once, under its content key')
            check(len(set(r[4:] for r in refs)) <= len(keys), f'{len(keys)} photographs in IndexedDB')
            m2 = await ev("S47.saveNow('manual', 1)")
            check(len(await ev(IDB_KEYS + "('photos')")) == len(keys), 'a second save stores no print twice')
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)")
        back = await ev("S47.album.shots.map(s => s.url)")
        check(back == urls, f'Continue brings the album back, every print as it was ({len(back)})')
        check(await ev("S47.album.shots.map(s => s.text)") == texts and await ev("S47.album.left") == 0, 'with its captions and an empty roll')
        await ev("S47.game.openNotebook()")
        await pg.click('.notebook [data-t=photos]')
        check(await ev("document.querySelectorAll('.notebook .nb-page.on .al-print img').length") == 36, 'and in the notebook')
        await ev("S47.game.d.ui.close()")

        # ---------- Mesa Diner and Caliche's street (the people of story/caliche.ts) ----------
        await ev("S47.album.restore(null); window.__p = S47.world.prepare('diner')")
        for _ in range(150):
            if await ev("!!S47.world.town && !!S47.world.diner"): break
            await pg.wait_for_timeout(200)
        await ev("S47.world.enter('diner'); S47.world.placeAtDiner('inside'); S47.tick(0.3)")
        w = await ev("(()=>{const p=S47.world.diner.proxies.waitress.getWorldPosition(new S47.three.Vector3()); return [p.x,p.y,p.z]})()")
        pp = await ev("[S47.player.pos.x, S47.player.pos.z]")
        await look_at(pp[0], pp[1], w[0], w[1], w[2]); await raise_cam()
        s = await expose('f_diner')
        check(s and s['place'] == 'Mesa Diner' and 'waitress' in s['text'], f"inside the diner: \"{s and s['text']}\" ({s and s['place']})")
        await lower_cam()
        await ev("S47.world.enter('diner'); S47.player.place(7998.6, 2040.3, Math.PI); S47.player.pitch = 0.02; S47.tick(0.4)")
        await raise_cam()
        s = await expose('f_street')
        check(s and s['place'].startswith('Caliche') and s['text'], f"Caliche's street: \"{s and s['text']}\" ({s and s['place']})")
        await shot('a11_street_vf')
        await lower_cam()
        # one of them out in the dark round Caliche, 50 to 75 m off, at the edge of the view: on the
        # print it is a figure, and the caption says so
        caught = None
        for spot in ['town-mast', 'town-west', 'diner-west', 'town-south']:
            for side in [1, -1]:
                await ev("S47.glimpses.reset(); S47.player.place(7998.6, 2040.3, Math.PI); S47.player.pitch = 0; S47.tick(0.05)")
                # face roughly towards the spot first, so it is in the view to be put there
                sp = await ev(f"""(() => {{ const s = {{ 'town-mast': [8047, 2052], 'town-west': [7938, 2086], 'diner-west': [7944, 1992], 'town-south': [8003, 2296] }}['{spot}'];
                  const p = S47.player.pos; S47.player.yaw = Math.atan2(-(s[0] - p.x), -(s[1] - p.z)); S47.tick(0.05);
                  return S47.glimpses.now(S47.glimpseIn(), '{spot}'); }})()""")
                if not sp: continue
                g = await ev("(()=>{const g=S47.scene.getObjectByName('glimpse'); return [g.position.x, g.position.y, g.position.z]})()")
                pp = await ev("[S47.player.pos.x, S47.player.pos.z]")
                await look_at(pp[0], pp[1], g[0], g[1] + 0.6, g[2], dyaw=side * math.radians(28))
                await raise_cam(); await tick(0.05)
                if await ev("S47.glimpses.up") and NOBODY in (await ev("S47.album.peek()"))['odd']: caught = (spot, side, round(math.hypot(g[0] - pp[0], g[2] - pp[1]))); break
                await lower_cam()
            if caught: break
        check(caught is not None, f'a glimpse round Caliche can be framed at the edge of the view ({caught})')
        if caught:
            s = await expose('f_glimpse')
            check(s and NOBODY in s['text'], f"the frame keeps it: \"{s and s['text']}\"")
            check(await ev("!!S47.glimpses.up"), 'the figure on screen did not notice the camera')
            await shot('a12_glimpse_vf')
            await lower_cam()
        await ev("S47.glimpses.reset()")

        print('\n'.join(errs[:30]) or 'no console errors/warnings', flush=True)
        if errs: check(False, 'console clean (desktop)')
        await ctx.close()

        # ---------- a phone: the Camera button, a tap for the shutter, a tap on a print ----------
        ctx, pg, errs = await page(b, touch=True, size=(844, 390))
        ev = pg.evaluate
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]')
        await pg.evaluate("localStorage.clear()")
        await pg.tap('button[data-a=start]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter2'); S47.tick(1.5)")
        await ev("S47.album.restore(null); S47.game.d.ui.close(true); S47.tick(0.1)")
        await ev("S47.player.place(0, 2.5, 0); S47.player.pitch = -0.1; S47.tick(0.1)")
        check(await ev("!document.querySelector('.touch .camera').hidden"), 'phone: the Camera button is there')
        await pg.tap('.touch .camera'); await ev("S47.tick(0.1)")
        check(await ev("S47.fcam.raised"), 'phone: the Camera button raises the camera')
        await pg.touchscreen.tap(422, 195); await ev("S47.tick(0.1)")
        check(await ev("S47.album.shots.length") == 1, 'phone: a tap takes the free frame')
        await pg.screenshot(path=f'{OUT}/a08_phone_vf.png')
        await pg.tap('.touch .camera'); await ev("S47.tick(0.1)")
        await pg.tap('.touch .notes'); await ev("S47.tick(0.1)")
        await pg.tap('.notebook [data-t=photos]')
        await pg.tap('.notebook .nb-page.on .al-print')
        check(await ev("!!document.querySelector('.al-view .al-big img')"), 'phone: a tap opens the print big')
        await pg.screenshot(path=f'{OUT}/a09_phone_big.png')
        await pg.tap('.al-view .al-back')
        check(await ev("!document.querySelector('.al-view') && !!document.querySelector('.notebook')"), 'phone: Back puts it back in the album')
        await pg.screenshot(path=f'{OUT}/a10_phone_album.png')
        print('\n'.join(errs[:30]) or 'no console errors/warnings', flush=True)
        if errs: check(False, 'console clean (phone)')

        json.dump(checks, open(f'{OUT}/checks{"-noidb" if NO_IDB else ""}.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
