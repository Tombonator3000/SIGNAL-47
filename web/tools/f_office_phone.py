# The crank telephone in the motel office plays the player back (story/officePhone.ts; FORSLAG.md,
# 20 October, item 7).
#   - nothing before chapter four, nor in chapter four once room 6 is over; the prop says what it
#     always said
#   - blocked: with the office television on its carrier (one thing at a time) the player walks in
#     and nothing rings, also when the set is done and the player stays; walked out and in again (the
#     next entry) it rings; while it rings the set does not start (one at a time, the curve free)
#   - walked to for real from the lot: on the walk at the door it rings, its own bell (not the
#     payphone's recording), the handset rattling, a toast; the seen log has it, the curve lands on it
#   - the frames 4.2 s apart; answered with E aimed at it: nobody speaks; the line in order: the
#     carrier, the boards, the door, a shutter, "Another." exactly 4.2 s later, the steps back, the
#     hut's bell, the lift, Nora's first line from the earpiece (her recording), someone listening,
#     the handset down; measured on the sound: a click at each shutter, the gap between them quieter
#   - heard: one note ("as far apart as FRAME 03 and FRAME 04", "I took two frames out there",
#     "That was me, at 03:58", "Now it has me."), kept ('ev:office'), the seen log says heard; on the
#     board the line's card, its shutters tied to FRAME 04 hold, the thread is kept
#   - once: walked out and in again, it does not ring; lifted, the carrier and nothing else
#   - Continue: kept, the thread kept, no ring, nothing written twice
#   - a long wait between the frames is drawn in (20 s, 5 minutes); hung up on, or walked off the end
#     of the cord: nothing written, it rings on the next entry
#   - without the frames (the developer menu's chapter four): the steps, the door and the call, no
#     shutters, and a note without the frames
#   - phone 412x892 with touch: it rings as the player comes in, the Use button answers it, the
#     captions on the screen; no console errors or warnings
# Usage: python3 tools/f_office_phone.py OUTDIR
import asyncio, sys, json, os, math
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
    m() { return S47.world.motel; },
    // a point given in the terms the court was built in (MotelFront.ts), in the world
    w(x, z) { const a = T.m().at; return [x + a.x, z + a.z]; },
    phone() { const v = T.m().proxies.officePhone.getWorldPosition(new V()); return [v.x, v.y, v.z]; },
    door() { return T.w(-41.1, 24.1); },
    // walk with the player's own movement towards a point (the court's terms), a step a frame
    walk(x, z, stop = 0.15, max = 600) {
      const p = S47.player, [tx, tz] = T.w(x, z);
      for (let n = 0; n < max; n++) {
        if (Math.hypot(p.pos.x - tx, p.pos.z - tz) < stop) return true;
        p.yaw = Math.atan2(-(tx - p.pos.x), -(tz - p.pos.z)); p.update(1 / 30, 0, -1, false); S47.tick(1 / 30);
      }
      return false;
    },
    // out of the office through its door onto the lot, out of earshot (clear of the truck by the office);
    // and back in (the door is the way)
    out() { return T.walk(-42.6, 24.1) && T.walk(-39.6, 24.1) && T.walk(-30, 16); },
    in() { return T.walk(-39.6, 24.1) && T.walk(-42.6, 24.1); },
    // in front of the telephone, looking at it
    face() {
      const p = S47.player, [x, z] = T.w(-43.05, 23.35), t = T.phone();
      p.place(x, z, Math.atan2(-(t[0] - x), -(t[2] - z))); S47.tick(0.05);
      const c = S47.camera.position; p.pitch = Math.atan2(t[1] - c.y, Math.hypot(t[0] - c.x, t[2] - c.z)); S47.tick(0.05);
    },
    aimed() { const it = S47.game.d.inter.update(S47.camera); return it ? it.id : null; },
    frames(a, b) {
      const c = document.createElement('canvas'); c.width = 8; c.height = 6; const url = c.toDataURL('image/jpeg');
      const s = S47.ch3.s;
      if (a === null) { delete s.f3; delete s.f4; return; }
      s.f3 = { id: 'frame03', subject: 'STATION 01 / fixed point A', method: 'field', clock: a, url, targets: {} };
      s.f4 = { id: 'frame04', subject: 'STATION 01 / cable break', method: 'field', clock: b, url, targets: {} };
    },
    forget() { for (const k of [...S47.townSeen]) if (k === 'ev:office' || k.startsWith('office@') || k.startsWith('ol:')) S47.townSeen.delete(k); S47.officePhone.reset(); },
    ph() { const o = S47.officePhone; return { state: o.state, result: o.result, heard: o.heard, busy: o.busy, voiced: o.voiced, rings: o.rings,
      plan: o.plan && { gap: o.plan.gap, delta: o.plan.delta, shutters: o.plan.shutters, end: o.plan.end, kinds: o.plan.cues.map((c) => c.kind) },
      shown: o.shown.slice(), bell: !!o.bell, startedAt: o.startedAt }; },
    office() { return S47.game.notes.filter((n) => n.includes('The crank telephone in the motel office')); },
    seen() { return S47.seen.filter((e) => e.what === 'officephone').map((e) => ({ end: e.end, line: e.line, looking: e.looking, clock: e.clock })); },
    // the effects bus, sampled: [AudioContext time, peak]
    listen() {
      const a = S47.audio, ctx = a.ctx; if (T.an) return true;
      const an = ctx.createAnalyser(); an.fftSize = 1024; a.sfx.connect(an); T.an = an; T.pk = [];
      const buf = new Float32Array(an.fftSize);
      T.iv = setInterval(() => { an.getFloatTimeDomainData(buf); let m = 0; for (const v of buf) m = Math.max(m, Math.abs(v)); T.pk.push([ctx.currentTime, m]); }, 10);
      return true;
    },
    peak(a, b) { const v = T.pk.filter(([t]) => t >= a && t <= b).map(([, r]) => r); return v.length ? Math.max(...v) : null; },
    mean(a, b) { const v = T.pk.filter(([t]) => t >= a && t <= b).map(([, r]) => r); return v.length ? v.reduce((x, y) => x + y, 0) / v.length : null; },
  };
  return true;
})()"""

async def page(b, w, h, touch, voices=True):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    init = "try{localStorage.setItem('s47.serlingNarrator','false');" + ("" if voices else "localStorage.setItem('s47.characterVoices','false');") + "}catch{}"
    await pg.add_init_script(init)
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
    except Exception: pass
    await pg.evaluate("localStorage.clear()")
    await pg.add_init_script(init)
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
        ph = lambda: ev("T.ph()")
        async def quiet(): await ev("S47.nightEvents['wait'] = 1e9; S47.setAbductions(false)")
        async def play_out(limit=70, shots=None):
            """Ticks the game while the call plays on the line's own clock (real time) to its end."""
            t0 = await ev("S47.audio.ctx.currentTime"); taken = set()
            while await ev("S47.officePhone.state") == 'line':
                await tick(0.1); await pg.wait_for_timeout(60)
                e = await ev("S47.audio.ctx.currentTime") - t0
                for at, name in (shots or []):
                    if e >= at and name not in taken:
                        taken.add(name)
                        if name == '_turn':
                            await ev("const p = S47.player, [x, z] = T.door(); p.yaw = Math.atan2(-(x - p.pos.x), -(z - p.pos.z)); p.pitch = 0; S47.tick(0.05)")
                        else: await shot(name)
                if e > limit: return False
            return True
        await ev("S47.hold = true")

        # ---------- not before chapter four ----------
        await ev("S47.jump('chapter3'); S47.tick(1.0)"); await quiet()
        await ev("S47.world.enter('saro'); const [x, z] = T.w(-30, 16); S47.player.place(x, z, 0); S47.tick(0.1)")
        await ev("T.walk(-39.6, 24.1)"); await ev("T.walk(-42.6, 24.1)"); await tick(3.0)
        st = await ph()
        check(await ev("S47.game.phase") == 'ch3' and st['state'] == 'idle' and st['rings'] == 0, f'chapter three: walked into the motel office, nothing rings ({st["state"]})')

        # ---------- chapter four ----------
        await ev("S47.jump('chapter4'); S47.tick(1.0)"); await quiet()
        await ev("T.frames(3 * 3600 + 51 * 60 + 12.4, 3 * 3600 + 51 * 60 + 16.6)")
        await ev("S47.game.note('03:58. N. Vega called the field telephone. The line still rings in the motel office. She wants the original record and my film, developed, in room 6.')")
        await ev("S47.world.enter('saro'); S47.world.placeTruck('motel'); S47.game.clock = 4 * 3600 + 31 * 60; S47.tick(0.1)")
        check(await ev("S47.game.phase") == 'ch4' and await ev("S47.ch4.s.stage") == 'to-motel', 'chapter four, on the way to room 6')
        # once room 6 is over: nothing
        await ev("S47.ch4.s.stage = 'leave'; const [x, z] = T.w(-30, 16); S47.player.place(x, z, 0); S47.tick(0.1)")
        await ev("T.walk(-39.6, 24.1)"); await tick(3.0)
        check((await ph())['rings'] == 0, 'room 6 over: at the office door, nothing rings')
        await ev("S47.ch4.s.stage = 'to-motel'")

        # ---------- blocked: the office television on its carrier ----------
        await ev("const [x, z] = T.w(-30, 16); S47.player.place(x, z, 0); S47.tick(0.1); S47.motelTv.now(); S47.tick(0.1)")
        check(await ev("S47.motelTv.busy") and await ev("S47.oneAtATime()"), 'the office television is on its carrier (one of the night\'s things is going)')
        await ev("T.walk(-39.6, 24.1)"); await ev("T.walk(-42.6, 24.1)"); await tick(3.0)
        st = await ph()
        check(st['state'] == 'idle' and st['rings'] == 0, f'walked in while the set is on its carrier: the telephone does not ring ({st["state"]})')
        await ev("S47.motelTv.reset(); S47.tick(0.1)")
        await tick(3.0)
        check((await ph())['rings'] == 0, 'the set back on its test card, the player still in the office: it waits for the next time they come')
        check(await ev("T.out()") and not await ev("S47.officeIn().near"), 'walked back out onto the lot, out of earshot of the door')
        # the set may start on its own now (the curve free), but not while the telephone rings
        await ev("S47.curve.free = true")
        rang = await ev("T.walk(-39.6, 24.1)") and False
        for _ in range(40):
            await tick(0.1)
            if (await ph())['state'] == 'ringing': rang = True; break
        await ev("T.walk(-41.9, 24.1)")
        st = await ph()
        check(rang and st['state'] == 'ringing', 'the next time the player comes to the office door, walking: it rings')
        check(await ev("T.toasts.some((t) => t.includes('At the other end of the line, someone is turning the handle.'))"), 'the toast: "The crank telephone in the office is ringing. At the other end of the line, someone is turning the handle."')
        bell = await ev("(() => { const b = S47.officePhone.bell; return b ? { buf: b.src.buffer.duration, loop: b.src.loop, same: b.src.buffer === S47.audio.buf.phoneRing } : null; })()")
        check(bell and bell['loop'] and not bell['same'] and abs(bell['buf'] - 7.2) < 0.05, f'its own bell: the two gongs, made for it, looping, not the payphone\'s recording ({bell})')
        # from the doorway, a look at it before going in
        await ev("const p = S47.player, [x, z] = T.w(-41.6, 24.4), t = T.phone(); p.place(x, z, Math.atan2(-(t[0] - x), -(t[2] - z))); p.pitch = -0.05; S47.tick(0.05)")
        await ev("S47.game.d.ui.clearToasts()"); await pg.wait_for_timeout(300)
        await shot('desk_00_ringing_doorway')
        await ev("T.walk(-42.6, 24.1)")
        # in the office with the truck on the lot, a whole turn of its pattern (7.5 s): the handset
        # rattles in the bursts, and the television does not start
        rot = []
        for _ in range(75):
            await tick(0.1); rot.append(await ev("S47.world.motel.objs.officePhoneHandset.rotation.z"))
        check(max(abs(r) for r in rot) > 0.01 and sum(1 for r in rot if r == 0) > 5, f'the handset rattles on its hook in the bursts, still between them ({max(abs(r) for r in rot):.3f})')
        check(await ev("S47.motelTv.mode") == 'card', 'it rings on in the office with the truck on the lot: the television does not start')
        await ev("S47.curve.free = false")
        sn = await ev("T.seen()")
        check(len(sn) == 1 and sn[0]['looking'] is None, f'the seen log has it as it rings, heard only ({sn})')
        check(await ev("S47.curve.landed && S47.curve.landed.what") == 'officephone', 'the night\'s curve lands on it (a felt peak)')
        await ev("T.face()")
        check(await ev("T.aimed()") == 'court:officePhone' and await ev("S47.game.d.inter.get('court:officePhone').label()") == 'Answer the field telephone',
              'in front of it: "Answer the field telephone"')
        await ev("S47.game.d.ui.clearToasts()"); await tick(0.2); await pg.wait_for_timeout(300)
        await shot('desk_01_ringing')

        # ---------- answered ----------
        await ev("T.listen()")
        n0 = await ev("S47.game.notes.length")
        await pg.keyboard.press('KeyE'); await tick(0.1)
        st = await ph()
        check(st['state'] == 'line' and not st['bell'] and not await ev("S47.world.motel.objs.officePhoneHandset.visible"), 'E answers it: the bell stops, the handset off its hook')
        check(st['plan'] and abs(st['plan']['delta'] - 4.2) < 0.01 and abs(st['plan']['gap'] - 4.2) < 0.01 and abs(st['plan']['shutters'][1] - st['plan']['shutters'][0] - 4.2) < 0.01,
              f'the two shutters as far apart on the line as FRAME 03 and FRAME 04 were taken: 4.2 s ({st["plan"] and st["plan"]["shutters"]})')
        check(await ev("S47.game.d.inter.get('court:officePhone').label()") == 'Hang up', 'on the line: "Hang up"')
        # the player may look round while listening: at the open door of the office, at the night
        later = st['plan']['shutters'][1] + 9.6
        ok = await play_out(shots=[(st['plan']['shutters'][1] + 0.6, 'desk_02_another'), (later - 0.4, '_turn'), (later, 'desk_03_lifted_door')])
        st = await ph()
        check(ok and st['state'] == 'idle' and st['result'] == 'heard' and st['heard'], f'played to the end, heard ({st["result"]}, {st["plan"]["end"]:.1f} s)')
        shown = st['shown']
        want = ['Nobody on the line. A carrier, very weak', 'Boards creak out there', 'A door moves on its hinges', 'Outside, not far off: a shutter.', 'Another.',
                'the telephone in the hut is ringing. Nobody is in there.', 'Someone comes in across the boards', 'Someone picks it up.', '"So you went out there."', 'Someone is listening.', 'The handset goes down out there.']
        idx = [next((i for i, s in enumerate(shown) if w in s), -1) for w in want]
        check(all(i >= 0 for i in idx) and idx == sorted(idx), f'the captions, in order: the carrier, the boards, the door, a shutter, "Another.", the bell in the empty hut, the steps back to it, the lift, her first line, listening, the handset down ({len(shown)})')
        check(not any(t.startswith(('N. VEGA:', 'THE LINE:')) for t in shown), 'nobody speaks on the line itself')
        check(st['voiced'], 'her first line comes from her recording, small, from the earpiece out there')
        # on the sound: a click at each shutter, quieter between them
        s0, s1 = [st['startedAt'] + x for x in st['plan']['shutters']]
        pk = await ev(f"[T.peak({s0} - 0.03, {s0} + 0.12), T.peak({s1} - 0.03, {s1} + 0.12), T.mean({s0} + 1.2, {s1} - 1.2), T.peak({s0} + 1.2, {s1} - 1.2)]")
        print(f'  shutter peaks {pk}', flush=True)
        check(pk[0] and pk[1] and pk[2] is not None and pk[0] > 2 * pk[2] and pk[1] > 2 * pk[2] and pk[0] > pk[3] and pk[1] > pk[3],
              f'measured on the effects bus: a click at each shutter, louder than anything in the gap between ({pk[0]:.3f}, {pk[1]:.3f} against {pk[3]:.3f})')
        notes = (await ev("S47.game.notes.slice()"))[n0:]
        on = [n for n in notes if 'The crank telephone in the motel office' in n]
        check(len(on) == 1 and 'as far apart as FRAME 03 and FRAME 04' in on[0] and 'I took two frames out there.' in on[0] and 'That was me, at 03:58.' in on[0] and on[0].endswith('Now it has me.'),
              f'one note: {on[:1]}')
        check(await ev("S47.townSeen.has('ev:office')") and await ev("[...S47.townSeen].some((k) => k.startsWith('office@'))"), 'kept with the town\'s part of the case')
        sn = await ev("T.seen()")
        check(len(sn) == 1 and sn[0]['end'] == 'heard', f'the seen log: heard ({sn})')
        check(await ev("S47.world.motel.objs.officePhoneHandset.visible") and 'You put the handset back on its hook.' in await ev("T.toasts"), 'the handset back on its hook')

        # ---------- the board ----------
        await ev("S47.game.openBoard('SIERRA MOTOR COURT', 'ch4')"); await tick(0.2)
        await pg.wait_for_selector('.board-wrap')
        card = await ev("(() => { const c = document.querySelector('[data-card=ol]'); return c ? { lines: c.querySelectorAll('.bline').length, text: c.textContent.replace(/\\s+/g, ' ') } : null; })()")
        check(card and card['lines'] == 3 and 'Outside, a shutter, then another.' in card['text'], f'on the board: the office line, a pin on its three lines ({card and card["lines"]})')
        bsay = lambda: ev("(document.querySelector('.board-wrap [data-say]')||{}).textContent || ''")
        async def thread(a, c):
            for n in (a, c):
                await ev(f"S47.game.board.view.show('{n}')"); await pg.wait_for_timeout(80)
                await pg.click(f'[data-node="{n}"]')
            return await bsay()
        r = await thread('ol.shutters', 'frame04')
        check('my two frames at STATION 01, as far apart as I took them' in r and await ev("S47.townSeen.has('ol:frame04|ol.shutters')"), f'its shutters tied to FRAME 04: it holds, and it is kept ({r})')
        r = await thread('ol.boards', 'e09a.a')
        check('do not say anything about each other' in r, f'its steps tied to the transit record: nothing ({r})')
        await ev("S47.game.board.view.show('ol.shutters')"); await pg.wait_for_timeout(200)
        await shot('desk_04_board')
        await ev("S47.game.d.ui.close(true)"); await tick(0.2)

        # ---------- once ----------
        walked = await ev("T.out()") and await ev("T.in()"); await tick(4.0)
        check(walked and (await ph())['rings'] == 1, 'walked out and back in: it does not ring again')
        await ev("T.face(); S47.game.d.inter.get('court:officePhone').use()"); await tick(0.2)
        check(await ev("T.toasts[T.toasts.length - 1]") == 'The carrier, very weak, and nothing on it. You put the handset back.', 'lifted again: the carrier, and nothing on it')

        # ---------- Continue ----------
        await tick(3.0)
        await ev("S47.saveNow('manual', 0)")
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev(HELPERS); await ev("T.hook(); S47.hold = true; S47.tick(0.3)"); await quiet()
        st = await ph()
        check(st['heard'] and await ev("S47.townSeen.has('ol:frame04|ol.shutters')") and len(await ev("T.office()")) == 1, 'Continue: heard, the thread kept, the note once')
        await ev("const [x, z] = T.w(-30, 16); S47.player.place(x, z, 0); S47.tick(0.1)")
        await ev("T.walk(-39.6, 24.1)"); await ev("T.walk(-42.6, 24.1)"); await tick(3.0)
        check((await ph())['rings'] == 0 and len(await ev("T.office()")) == 1, 'Continue: at the office it does not ring; nothing written twice')

        # ---------- the long waits, drawn in; hung up; walked off the end of the cord ----------
        await ev("T.forget(); T.frames(14000, 14020)"); await ev("S47.officePhone.ringNow(); T.face()")
        await pg.keyboard.press('KeyE'); await tick(0.1)
        g20 = (await ph())['plan']['gap']
        await pg.keyboard.press('KeyE'); await tick(0.1)
        st = await ph()
        check(st['state'] == 'idle' and st['result'] == 'away' and not st['heard'] and len(await ev("T.office()")) == 1,
              'hung up (E again): put back, nothing written, not heard')
        await tick(11)
        await ev("T.frames(14000, 14300)"); await ev("S47.officePhone.ringNow(); T.face()")
        await pg.keyboard.press('KeyE'); await tick(0.1)
        g300 = (await ph())['plan']['gap']
        check(8 < g20 < g300 < 14, f'a long wait between the frames is drawn in: 20 s on the line as {g20:.2f} s, 5 minutes as {g300:.2f} s (at most 14)')
        # (south past the counter, between it and the office door's leaf)
        await ev("T.walk(-43.0, 25.8)")
        st = await ph()
        check(st['state'] == 'idle' and st['result'] == 'away' and 'The cord will not reach.' in ' '.join(await ev("T.toasts")), 'walked off the end of the cord: the handset goes back')
        sn = await ev("T.seen()")
        check([e['end'] for e in sn][-2:] == ['away', 'away'], f'the seen log: put down, twice ({[e["end"] for e in sn]})')
        await tick(11)
        walked = await ev("T.out()"); await tick(0.5)
        walked = walked and await ev("T.walk(-39.6, 24.1)")
        for _ in range(30):
            await tick(0.1)
            if (await ph())['state'] == 'ringing': break
        check(walked and (await ph())['state'] == 'ringing', f'not heard: the next time the player comes to the office, it rings ({walked}, {(await ph())["state"]})')

        # ---------- without the frames ----------
        await ev("T.walk(-42.6, 24.1); T.frames(null); T.face()")
        n0 = await ev("S47.game.notes.length")
        await pg.keyboard.press('KeyE'); await tick(0.1)
        st = await ph()
        check(st['plan']['gap'] is None and 'shutter' not in st['plan']['kinds'] and 'board' in st['plan']['kinds'] and 'door' in st['plan']['kinds'] and 'lift' in st['plan']['kinds'],
              'without FRAME 03 and FRAME 04: the steps, the door and the call, and no shutter')
        ok = await play_out()
        st = await ph()
        notes = [n for n in (await ev("S47.game.notes.slice()"))[n0:] if 'The crank telephone in the motel office' in n]
        check(ok and st['result'] == 'heard' and len(notes) == 1 and 'FRAME' not in notes[0] and 'Someone walking on the boards, the hut door.' in notes[0] and 'That was me' in notes[0],
              f'heard: a note without the frames ({notes[:1]})')
        check(not any('shutter' in s for s in st['shown']), 'and no shutter in the captions')
        print('\n'.join(errs[:30]) or 'desktop: no console errors/warnings')
        check(not errs, 'desktop: console clean')
        await ctx.close()

        # ======================= phone, touch, the captions only =======================
        ctx, pg, errs = await page(b, 412, 892, True, voices=False)
        ev = pg.evaluate
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        ph = lambda: ev("T.ph()")
        await ev("S47.hold = true; S47.jump('chapter4'); S47.tick(1.0)"); await quiet()
        check(await ev("S47.input.touchMode"), 'phone: touch controls')
        await ev("T.frames(3 * 3600 + 51 * 60 + 12.4, 3 * 3600 + 51 * 60 + 18.1)")
        await ev("S47.world.enter('saro'); const [x, z] = T.w(-30, 16); S47.player.place(x, z, 0); S47.tick(0.1)")
        await ev("T.walk(-39.6, 24.1)")
        for _ in range(30):
            await tick(0.1)
            if (await ph())['state'] == 'ringing': break
        check((await ph())['state'] == 'ringing', 'phone: it rings as the player comes to the office door')
        await ev("T.walk(-42.6, 24.1); T.face()"); await tick(0.3)
        use = pg.locator('.touch .use')
        check(await use.is_visible() and await ev("T.aimed()") == 'court:officePhone', 'phone: the Use button, aimed at the telephone')
        await ev("S47.game.d.ui.clearToasts()"); await pg.wait_for_timeout(300)
        await shot('phone_01_ringing')
        bb = await use.bounding_box()
        await pg.touchscreen.tap(bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2); await tick(0.1)
        st = await ph()
        check(st['state'] == 'line' and abs(st['plan']['gap'] - 5.7) < 0.01, f'phone: Use answers it; the shutters 5.7 s apart ({st["plan"] and st["plan"]["gap"]})')
        t0 = await ev("S47.audio.ctx.currentTime"); taken = set()
        cap_on = False
        while await ev("S47.officePhone.state") == 'line':
            await tick(0.1); await pg.wait_for_timeout(60)
            e = await ev("S47.audio.ctx.currentTime") - t0
            if e >= st['plan']['shutters'][0] + 0.5 and 'a' not in taken:
                taken.add('a'); await shot('phone_02_shutter')
                cap_on = await ev("[...document.querySelectorAll('.toast')].some((t) => t.textContent.includes('a shutter'))")
            if e >= st['plan']['shutters'][1] + 6.0 and 'b' not in taken: taken.add('b'); await shot('phone_03_comes_in')
            if e > 70: break
        st = await ph()
        check(cap_on, 'phone: the caption is on the screen')
        check(st['result'] == 'heard' and not st['voiced'] and any('"So you went out there."' in s for s in st['shown']),
              'phone: heard to the end; without the character voices her line is the caption alone')
        check(len(await ev("T.office()")) == 1, 'phone: one note')
        await ev("S47.game.d.ui.clearToasts()"); await tick(0.5)
        await ev("S47.game.openNotebook()"); await tick(0.2)
        await pg.click('.notebook [data-t=notes]'); await pg.wait_for_timeout(300)
        await ev("(() => { const l = [...document.querySelectorAll('.notebook li')].filter((e) => e.textContent.includes('The crank telephone in the motel office')).pop(); if (l) l.scrollIntoView({ block: 'end' }); })()"); await pg.wait_for_timeout(250)
        await shot('phone_04_note')
        await ev("S47.game.d.ui.close(true)")
        print('\n'.join(errs[:30]) or 'phone: no console errors/warnings')
        check(not errs, 'phone: console clean')
        await ctx.close()

        json.dump(checks, open(f'{OUT}/checks-office-phone.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
