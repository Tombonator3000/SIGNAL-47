# A face on the film before 05:29 (FORSLAG.md, 20 October, "Valg for Tom" 5; core/filmFace.ts,
# core/album.ts, ui/album.ts, main.ts). Once in the night, the first free frame taken out of doors
# in chapter four keeps, at its edge and a foot from the lens, the side of a pale head and one
# black eye with the flash in it. Only on the print: the same spot taken again shows nothing.
# The caption is written once the print has come up, the game says once that it has, and the
# first close look at it, come up, quiets the night and the music and puts a line in the
# notebook. Kept with the album, so Continue brings it back and it never comes again.
#
# Checked: not in chapter two out of doors, not indoors in chapter four, not in chapter five; the
# first outdoor frame in chapter four has it, the next does not; it is on the film and not in the
# scene, and what is on the print is a pale head with a black eye and the flash in it where
# core/filmFace.ts put the eye; still coming up, no caption and no note; come up, the toast once;
# looked at big: the quiet, the note once, and the narration can still read the print and the
# whole notebook; saved and back with Continue, and no second face after it; the same on a phone
# with touch. Screenshots of the moment on desktop (1280x720) and phone (412x892).
# Usage: python3 tools/horror_face.py OUTDIR
import asyncio, sys, json, os, math, base64, re
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
FACE_LINE = 'Nobody was there when I took it.'
# the note: the time and the place of the frame, then the caption's words (both narrators have them)
NOTE_RE = r'^\d\d:\d\d, the highway south\. Nobody was there when I took it\.$'
NOTE_BIT = ', the highway south. ' + FACE_LINE
LAUNCH = ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required']
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

async def page(b, size, touch=False):
    ctx = await b.new_context(viewport={'width': size[0], 'height': size[1]}, has_touch=touch, is_mobile=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    return ctx, pg, errs

# brightness of a part of a print (u0, u1 across, v0, v1 down), 0 to 255
REGION = """(async ([url, u0, u1, v0, v1]) => { const i = new Image(); i.src = url; await i.decode();
  const c = document.createElement('canvas'); c.width = i.naturalWidth; c.height = i.naturalHeight;
  const g = c.getContext('2d', { willReadFrequently: true }); g.drawImage(i, 0, 0);
  const x0 = Math.floor(u0 * c.width), x1 = Math.ceil(u1 * c.width), y0 = Math.floor(v0 * c.height), y1 = Math.ceil(v1 * c.height);
  const d = g.getImageData(x0, y0, x1 - x0, y1 - y0).data; let s = 0;
  for (let k = 0; k < d.length; k += 4) s += d[k] * 0.3 + d[k + 1] * 0.59 + d[k + 2] * 0.11;
  return s / (d.length / 4); })"""
# the brightest pixel in that part, 0 to 255
REGION_MAX = REGION.replace('let s = 0;', 'let s = 0, m = 0;').replace('s += d[k] * 0.3 + d[k + 1] * 0.59 + d[k + 2] * 0.11;', 'm = Math.max(m, d[k] * 0.3 + d[k + 1] * 0.59 + d[k + 2] * 0.11);').replace('return s / (d.length / 4);', 'return m;')

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=LAUNCH)
        ctx, pg, errs = await page(b, (1280, 720))
        await pg.goto(URL)
        try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
        except Exception: pass
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_function('S47.started()', polling=200)
        ev = pg.evaluate
        async def tick(s=0.1): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def place(x, z, yaw, pitch=0.0): await ev(f"S47.player.place({x},{z},{yaw}); S47.player.pitch={pitch}; S47.tick(0.2)")
        async def raise_cam():
            if not await ev("S47.fcam.raised"): await pg.keyboard.press('KeyC'); await tick()
        async def lower_cam():
            if await ev("S47.fcam.raised"): await pg.keyboard.press('KeyC'); await tick()
        n_shots = lambda: ev("S47.album.shots.length")
        async def expose(name=None):
            n = await n_shots()
            await pg.keyboard.press('Space'); await tick()
            s = await ev("S47.album.shots.at(-1)") if await n_shots() > n else None
            if s and name: open(f'{OUT}/{name}.jpg', 'wb').write(base64.b64decode(s['url'].split(',', 1)[1]))
            return s
        last_face = lambda: ev("!!(S47.album.last && S47.album.last.face)")
        notes_with = lambda: ev(f"S47.game.notes.filter(n => n.includes({json.dumps(NOTE_BIT)})).length")
        toasts = lambda: ev("document.querySelector('.toasts').textContent")
        # no abduction or night event in the middle of the checks
        await ev("S47.hold = true; S47.setAbductions(false); S47.tick(0.3)")
        # where the eye of the last face went (core/filmFace.ts), kept before the next frame clears it
        await ev("""(() => { const a = S47.album, o = a.expose.bind(a); a.expose = (...x) => { const r = o(...x); if (a.last && a.last.eye) window.__faceEye = a.last.eye; return r; }; })()""")

        # ---------- chapter two, out of doors: no face ----------
        await ev("S47.jump('chapter2'); S47.tick(1.5); S47.album.restore(null)")
        await ev("S47.doors.set('east', true, true)"); await tick(0.5)
        await place(9.5, -6.0, 0.0, 0.1); await raise_cam()
        s = await expose()
        check(s is not None and await ev("S47.space()") == 'yard' and not await last_face() and not await ev("S47.album.data.face"),
              f'chapter two, out in the yard: a free frame, and no face ("{s and s["text"]}")')
        await lower_cam()

        # ---------- chapter four, indoors: no face ----------
        await ev("S47.jump('chapter4'); S47.tick(1.0); S47.album.restore(null); S47.setAbductions(false)")
        check(await ev("S47.game.phase") == 'ch4', 'chapter four')
        await place(0.0, 2.5, 0.0, -0.1); await raise_cam()
        s = await expose()
        check(s is not None and await ev("S47.space()") == 'room' and not await last_face() and not await ev("S47.album.data.face"),
              f'chapter four, indoors ({s and s["place"]}): no face')
        await lower_cam()

        # ---------- chapter four, on the highway's shoulder: the face ----------
        await place(-30.5, 200.0, math.pi, -0.02)
        check(await ev("S47.space()") == 'yard' and 'highway' in (await ev("S47.placeName()")).lower(), f'on the shoulder of the highway ({await ev("S47.placeName()")})')
        objs0 = await ev("(() => { let n = 0; S47.scene.traverse(() => n++); return n; })()")
        print('   before the shutter:', await ev("[document.querySelector('.fade').className, !!document.querySelector('.chapter-card'), !!S47.game.d.ui.modal, S47.game.cinematic]"), flush=True)
        await raise_cam()
        await pg.wait_for_timeout(400); await tick(0.05)
        await shot('d01_viewfinder_before')
        s = await expose('face_print')
        await pg.wait_for_timeout(400); await tick(0.05)
        await shot('d02_viewfinder_after_shutter')
        face_id = await ev("S47.album.data.face && S47.album.data.face.id")
        check(s is not None and await last_face() and face_id == s['id'], f'the first free frame out of doors in chapter four keeps the face ({face_id})')
        check(s and s['text'] == FACE_LINE and s.get('odd'), f'its caption: "{s and s["text"]}"')
        objs1 = await ev("(() => { let n = 0; S47.scene.traverse(() => n++); return n; })()")
        check(objs1 == objs0, f'nothing was added to the scene for it ({objs0} objects before, {objs1} after)')
        # the same spot again: on the film only, so the scene gives nothing there
        s2 = await expose('plain_print')
        check(s2 is not None and not await last_face() and await ev("S47.album.data.face.id") == face_id, 'the next free frame from the same spot has no face, and the face stays on the first')
        # it comes at one edge or the other: the brighter difference is its edge
        diffs = []
        # (the lit skull and brow over the eye, where the flash is strongest)
        for name, (u0, u1) in [('right', (0.76, 0.96)), ('left', (0.04, 0.24))]:
            bf, bp = await ev(REGION, [s['url'], u0, u1, 0.1, 0.38]), await ev(REGION, [s2['url'], u0, u1, 0.1, 0.38])
            diffs.append((bf - bp, name, bf, bp))
        d, edge, b_face, b_plain = max(diffs)
        other = min(diffs)
        check(d > 40 and abs(other[0]) < 12, f'on the print it is there, in the scene it is not: the {edge} edge {b_face:.0f} on the first print, {b_plain:.0f} on the second; the other edge the same on both ({other[2]:.0f} / {other[3]:.0f})')
        # and what is there is a pale head with a black eye in it and the flash in the eye, where
        # core/filmFace.ts says the eye is; on the second print there is only the night
        eye = await ev("window.__faceEye || null")
        if eye:
            u, v = eye['u'], eye['v']
            brow = [u - 0.06, u + 0.06, v - 0.2, v - 0.11]
            e_win = [u - 0.025, u + 0.025, v - 0.02, v + 0.02]
            b_f, b_p = await ev(REGION, [s['url'], *brow]), await ev(REGION, [s2['url'], *brow])
            e_f = await ev(REGION, [s['url'], *e_win])
            glint = await ev(REGION_MAX, [s['url'], u - 0.08, u + 0.08, v - 0.09, v + 0.05])
            check(b_f - b_p > 45 and b_f - e_f > 45 and glint - e_f > 50,
                  f'a pale head and a black eye with the flash in it (at u {u:.2f}, v {v:.2f}): the skin over the eye {b_f:.0f} (the night there {b_p:.0f}), the eye {e_f:.0f}, the flash in it {glint:.0f}')
        else: check(False, 'the album says where the eye is')
        await lower_cam()

        # ---------- still coming up: no caption yet, no note ----------
        await ev("S47.game.openNotebook()"); await pg.click('.notebook [data-t=photos]')
        cap = await ev(f"(() => {{ const b = document.querySelector('.notebook .al-print[data-f=\"{face_id}\"] .al-cap b'); return b ? b.textContent : null; }})()")
        check(cap == '', f'while the print is still coming up its caption is not written yet ({cap!r})')
        await pg.click(f'.notebook .al-print[data-f="{face_id}"]')
        await shot('d03_big_still_coming_up')
        check(not await ev("S47.album.data.face.seen") and await notes_with() == 0, 'looked at before it has come up: nothing yet')
        await pg.keyboard.press('Escape'); await tick(0.05)
        await ev("S47.game.d.ui.close()"); await tick()

        # ---------- come up: said once ----------
        await ev("S47.game.clock += 60; S47.tick(0.3)")
        check(not await ev("S47.album.data.face.up"), 'a minute later it is still coming up')
        await ev("S47.game.clock += 31; S47.tick(0.3)")
        t = await toasts()
        check(await ev("S47.album.data.face.up") and 'has come up' in t, f'a minute and a half later: "{t.strip()[:80]}"')
        await ev("S47.game.clock += 5; S47.tick(0.3)")
        n_up = await ev("[...document.querySelectorAll('.toasts > *')].filter(e => e.textContent.includes('has come up')).length")
        check(n_up <= 1, f'and only once ({n_up})')

        # ---------- the first close look ----------
        await ev("S47.game.openNotebook()"); await pg.click('.notebook [data-t=photos]')
        cap = await ev(f"document.querySelector('.notebook .al-print[data-f=\"{face_id}\"] .al-cap b').textContent")
        check(cap == FACE_LINE, f'come up, the caption is written: "{cap}"')
        await shot('d04_album_page')
        q0 = await ev("S47.game.d.audio.night ? S47.game.d.audio.night.quiet.gain.value : -1")
        await pg.click(f'.notebook .al-print[data-f="{face_id}"]')
        await pg.wait_for_timeout(1600); await tick(0.05)
        await shot('d05_big_face')
        q1 = await ev("S47.game.d.audio.night ? S47.game.d.audio.night.quiet.gain.value : -1")
        check(await ev("S47.album.data.face.seen") and await notes_with() == 1, 'looked at big, come up: seen, and one line in the notebook')
        note = await ev(f"S47.game.notes.find(n => n.includes({json.dumps(NOTE_BIT)}))")
        check(note is not None and re.match(NOTE_RE, note), f'the line: "{note}"')
        # the narration (Serling or Kinski, when it is on) has the words: on the print, and the whole notebook still reads
        reads = await ev(f"""(() => {{ const r = S47.game.d.ui.readAloud; return r ? [r.available([{json.dumps(FACE_LINE)}]), r.available(S47.game.notes)] : null; }})()""")
        check(reads is None or (reads[0] and reads[1]), f'the narration can read the caption and every line in the notebook ({reads})')
        check(q0 < 0 or (q1 < 0.5 and q1 < q0), f'the night goes quiet while it is looked at (the night\'s level {q0:.2f} before, {q1:.2f} after)')
        check(await ev("S47.faceQuiet()") > 20, f'and the music is held ({await ev("S47.faceQuiet()"):.1f} s left)')
        big_cap = await ev("document.querySelector('.al-view .al-big .al-cap b').textContent")
        check(big_cap == FACE_LINE, 'the big print has the caption too')
        await pg.keyboard.press('Escape'); await tick(0.05)
        await pg.click(f'.notebook .al-print[data-f="{face_id}"]'); await tick(0.05)
        check(await notes_with() == 1, 'looked at again: no second line')
        await pg.keyboard.press('Escape'); await tick(0.05)
        await ev("S47.game.d.ui.close()"); await tick()

        # ---------- saved, and back with Continue: no second face ----------
        url0 = s['url']
        await ev("S47.saveNow('manual', 0)")
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.setAbductions(false); S47.tick(0.5)")
        f = await ev("S47.album.data.face")
        back = await ev(f"(S47.album.shots.find(s => s.id === {json.dumps(face_id)}) || {{}}).url")
        check(f and f['id'] == face_id and f.get('up') and f.get('seen'), f'Continue brings the face back as it was ({f})')
        check(back == url0, 'with its print, the same picture')
        check(await notes_with() == 1, 'and the line in the notebook, once')
        check(await ev("S47.game.phase") == 'ch4', 'still chapter four')
        await place(-30.5, 330.0, math.pi, -0.02); await raise_cam()
        s3 = await expose()
        check(s3 is not None and not await last_face() and await ev("S47.album.data.face.id") == face_id, 'out on the shoulder again after Continue: no second face')
        await lower_cam()
        await ev("S47.game.clock += 120; S47.tick(0.3)")
        await ev("S47.game.openNotebook()"); await pg.click('.notebook [data-t=photos]')
        await pg.click(f'.notebook .al-print[data-f="{face_id}"]'); await pg.wait_for_timeout(300); await tick(0.05)
        check(await notes_with() == 1 and await ev("S47.faceQuiet()") <= 0, 'looked at again after Continue: no new line, no new quiet')
        await pg.keyboard.press('Escape'); await ev("S47.game.d.ui.close()"); await tick()

        # ---------- chapter five, out of doors, with no face taken in chapter four: none ----------
        await ev("S47.jump('chapter5'); S47.tick(1.0); S47.album.restore(null); S47.setAbductions(false)")
        await ev("S47.doors.set('east', true, true)"); await tick(0.5)
        await place(9.5, -6.0, 0.0, 0.1); await raise_cam()
        s = await expose()
        check(s is not None and await ev("S47.game.phase") == 'ch5' and await ev("S47.space()") == 'yard' and not await last_face() and not await ev("S47.album.data.face"),
              'chapter five, out in the yard, with no face taken in chapter four: none')
        await lower_cam()

        print('\n'.join(errs[:30]) or 'no console errors/warnings (desktop)', flush=True)
        check(not errs, 'console clean (desktop)')
        await ctx.close()

        # ---------- a phone: the Camera button, a tap for the shutter, a tap on the print ----------
        ctx, pg, errs = await page(b, (412, 892), touch=True)
        ev = pg.evaluate
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]')
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        await pg.tap('button[data-a=start]'); await begin_opening(pg, touch=True); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.setAbductions(false); S47.tick(0.3); S47.jump('chapter4'); S47.tick(1.0)")
        await ev("S47.album.restore(null); S47.setAbductions(false); S47.game.d.ui.close(true); S47.tick(0.1)")
        await ev("S47.player.place(-30.5, 420, Math.PI); S47.player.pitch = -0.02; S47.tick(0.3)")
        check(await ev("S47.space()") == 'yard', 'phone: on the shoulder of the highway, chapter four')
        await pg.tap('.touch .camera'); await ev("S47.tick(0.1)")
        check(await ev("S47.fcam.raised"), 'phone: the Camera button raises the camera')
        await pg.touchscreen.tap(206, 446); await ev("S47.tick(0.1)")
        check(await ev("S47.album.shots.length") == 1 and await ev("!!S47.album.data.face && S47.album.last.face"), 'phone: a tap takes the free frame, and it keeps the face')
        ph = await ev("S47.album.shots[0]")
        open(f'{OUT}/face_print_phone.jpg', 'wb').write(base64.b64decode(ph['url'].split(',', 1)[1]))
        await pg.wait_for_timeout(600); await ev("S47.tick(0.05)")   # past the shutter's flash
        await pg.screenshot(path=f'{OUT}/p01_viewfinder_after_shutter.png')
        await pg.tap('.touch .camera'); await ev("S47.tick(0.1)")
        await ev("S47.game.clock += 92; S47.tick(0.3)")
        check('has come up' in await ev("document.querySelector('.toasts').textContent"), 'phone: the print has come up')
        await pg.screenshot(path=f'{OUT}/p02_come_up_toast.png')
        await pg.tap('.touch .notes'); await ev("S47.tick(0.1)")
        await pg.tap('.notebook [data-t=photos]')
        await pg.screenshot(path=f'{OUT}/p03_album_page.png')
        await pg.tap('.notebook .nb-page.on .al-print')
        await pg.wait_for_timeout(1200)
        check(await ev("!!document.querySelector('.al-view .al-big img')") and await ev("S47.album.data.face.seen"), 'phone: a tap opens the print big, and it is seen')
        check(await ev(f"S47.game.notes.filter(n => n.includes({json.dumps(NOTE_BIT)})).length") == 1, 'phone: one line in the notebook')
        await pg.screenshot(path=f'{OUT}/p04_big_face.png')
        await pg.tap('.al-view .al-back')
        check(await ev("!document.querySelector('.al-view') && !!document.querySelector('.notebook')"), 'phone: Back puts it back in the album')
        print('\n'.join(errs[:30]) or 'no console errors/warnings (phone)', flush=True)
        check(not errs, 'console clean (phone)')

        json.dump(checks, open(f'{OUT}/checks.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
