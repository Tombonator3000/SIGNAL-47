# The nights SARO printed a negative range are the nights Caliche lost time, Earl's reel from
# 9/22/81, the tow ticket and the driver's watch (story/lostNights.ts, core/reelTape.ts; FORSLAG.md,
# 20 October, item 11 and small tweak 4).
#   - not in chapter one, and not before the exceptions binder has been read: the binder is a plain
#     card then; from chapter two, read, it lies on Night Shift's cork with a pin on each printout
#   - Caliche, real conversations: Earl (the calls, the reel he never played; no date to give him
#     yet), the attendant (the old road, then the carbon of the tow ticket: in Papers, his own
#     recorded words), the woman on the bench (her husband), the deputy (the green pins)
#   - the board: the green pins and what the three said as cards with a pin per person; a wrong night
#     falls slack, two lines of one card say so; the binder's -38 to the attendant's green pin holds
#     with "The night SARO printed -38, the attendant lost two hours on the old road."; -36 and -34
#     likewise, and with the third the question comes onto the cork, open, with nothing to record:
#     "-39 tonight. Who loses time tonight?"; one note in the notebook; the threads are kept
#   - Earl again, before four: "Tell him the date: September 22, 1981". Walked away from mid-reel, it
#     is as if it had not been asked (no note, no paper, it can be asked again). Heard to the end: the
#     boy's three lines, word for word as tonight's call, in his recorded voice; two holes on the tape,
#     each as long as what Earl said tonight, with no hiss in them (measured on the sound); then "I
#     answered him tonight." "He wasn't answering me."; one note quoting it, the reel in Papers, the
#     boy will not call again tonight; asked once; Earl's new lines say they have no recording
#   - the reel and tonight's boy on the board: "Earl answered a recording."
#   - after a time away, the minutes are under the question in the player's own hand
#   - Continue: the threads, the question, the papers and the reel come back; nothing is written twice
#   - the diner, chapter five: a closer look at the driver: his watch says two minutes to twelve (one
#     note); asked about it; shown the tow ticket, he does not remember a tow; the watch to the binder's
#     -38 holds
#   - desktop 1280x720 and phone 412x892 with touch; no console errors or warnings
# Usage: python3 tools/f_lost_nights.py OUTDIR
import asyncio, sys, json, os, math
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

BOY = ["Hello? Is this the radio? There's a star with a tail on it, real low in the south. A little tail.",
       "I'm calling from my dad's place, out on the ranch. There's a light over the mesa too. It isn't moving.",
       "He's out with the men. It's hot out. It's July."]
N85 = 'The night SARO printed -38, the attendant lost two hours on the old road.'
Q39 = '-39 tonight. Who loses time tonight?'

HELPERS = """(() => {
  window.T = {
    toasts: [],
    hook() {
      const ui = S47.game.d.ui; if (ui.__hooked) return; ui.__hooked = true;
      const o = ui.toast.bind(ui); ui.toast = (t, s, c) => { T.toasts.push(t); return o(t, s, c); };
    },
    has(k) { return S47.townSeen.has(k); },
    count(re) { return S47.game.notes.filter((n) => new RegExp(re).test(n)).length; },
    lines() { return [...document.querySelectorAll('.talk-line')].map((e) => { const w = e.querySelector('.who'); const c = e.cloneNode(true); c.querySelector('.who')?.remove(); return { who: w ? w.textContent : '', text: c.textContent.trim() }; }); },
    choices() { return [...document.querySelectorAll('.talk [data-c]')].map((b) => b.dataset.c); },
    docs() { return S47.game.docs.map((d) => d.id); },
    // the sound on the effects bus, sampled: [AudioContext time, RMS]
    listen() {
      const a = S47.audio, ctx = a.ctx; if (T.an) return;
      const an = ctx.createAnalyser(); an.fftSize = 2048; a.sfx.connect(an); T.an = an; T.rms = [];
      const buf = new Float32Array(an.fftSize);
      const am = ctx.createAnalyser(); am.fftSize = 2048; a.master.connect(am); T.am = am; T.mrms = [];
      T.iv = setInterval(() => {
        an.getFloatTimeDomainData(buf); let s = 0; for (const v of buf) s += v * v; T.rms.push([ctx.currentTime, Math.sqrt(s / buf.length)]);
        am.getFloatTimeDomainData(buf); s = 0; for (const v of buf) s += v * v; T.mrms.push([ctx.currentTime, Math.sqrt(s / buf.length)]);
      }, 30);
    },
    rmsIn(a, b) { const v = T.rms.filter(([t]) => t >= a && t <= b).map(([, r]) => r); return v.length ? v.reduce((x, y) => x + y, 0) / v.length : null; },
    mIn(a, b) { const v = T.mrms.filter(([t]) => t >= a && t <= b).map(([, r]) => r); return v.length ? v.reduce((x, y) => x + y, 0) / v.length : null; },
  };
  return true;
})()"""

async def page(b, w, h, touch):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    init = "try{localStorage.setItem('s47.serlingNarrator','false');}catch{}"
    await pg.add_init_script(init)
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
    except Exception: pass
    await pg.evaluate("localStorage.clear()")
    await pg.add_init_script(init)
    await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    if touch: await pg.tap('button[data-a=start]'); await begin_opening(pg, touch=True)
    else: await pg.click('button[data-a=start]'); await begin_opening(pg)
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS)
    await pg.evaluate("T.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    return ctx, pg, errs

class Run:
    def __init__(self, pg, touch=False): self.pg = pg; self.ev = pg.evaluate; self.touch = touch
    async def tick(self, s, n=1):
        for _ in range(n): await self.ev(f"S47.tick({s})")
    async def shot(self, name): await self.pg.screenshot(path=f'{OUT}/{name}.png')
    async def quiet(self): await self.ev("S47.nightEvents['wait'] = 1e9; S47.setAbductions(false)")
    async def tap(self, sel):
        if self.touch:
            bb = await self.pg.locator(sel).first.bounding_box()
            await self.pg.touchscreen.tap(bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2)
        else: await self.pg.click(sel)
    async def choose(self, c):
        await self.pg.locator(f'.talk [data-c="{c}"]').scroll_into_view_if_needed()
        await self.tap(f'.talk [data-c="{c}"]'); await self.tick(0.05)
    async def close(self): await self.ev("S47.game.d.ui.close(true)"); await self.tick(0.1)
    async def town(self, clock):
        await self.ev("async () => { await S47.world.prepare('diner'); S47.world.enter('diner'); }")
        await self.ev(f"S47.game.clock = {clock}; S47.world.town.update(S47.runTime(), S47.game.clock); S47.tick(0.05)")
    async def talk(self, who):
        await self.ev(f"S47.game.d.inter.get('town:{who}').use()"); await self.tick(0.05)
        await self.pg.wait_for_selector('.labpanel .talk')
    # the case board
    async def board(self, region='ns'):
        await self.ev("S47.game.d.ui.close(true); S47.game.openBoard()"); await self.tick(0.05)
        await self.ev(f"S47.game.board.view.look('{region}')"); await self.tick(0.05)
    async def say(self): return await self.ev("(document.querySelector('.board-wrap [data-say]')||{}).textContent || ''")
    async def has(self, sel): return await self.ev(f"!!document.querySelector({json.dumps(sel)})")
    async def thread(self, a, b):
        for node in (a, b):
            await self.ev(f'S47.game.board.view.show({json.dumps(node)})')
            await self.tap(f'[data-node="{node}"]')
        return await self.say()
    async def links(self): return await self.ev("(S47.game.ns.ln || []).slice()")

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        # ======================= desktop =======================
        ctx, pg, errs = await page(b, 1280, 720, False)
        R = Run(pg); ev = pg.evaluate; tick = R.tick
        await ev("S47.hold = true")

        # ---------- not in chapter one ----------
        await ev("S47.jump('chapter1'); S47.tick(1.0)"); await R.quiet()
        await ev("S47.game.d.inter.get('excBinder').use()"); await tick(0.05); await R.close()
        check('binder' in await ev("T.docs()") and await ev("S47.lostNights.books().length") == 0, 'chapter one: the binder read, but its book is not on the board yet')
        await R.board('ns')
        check(not await R.has('[data-node="ln.b85"]') and await R.has('.bcard[data-card=binder]'), 'chapter one: the binder is a plain card')
        await R.close()

        # ---------- chapter two ----------
        await ev("S47.jump('chapter2'); S47.tick(1.0)"); await R.quiet()
        check(await ev("S47.game.phase") == 'ch2' and await ev("S47.lostNights.books().length") == 0, 'chapter two, binder not read: nothing on the board')
        await ev("S47.game.d.inter.get('excBinder').use()"); await tick(0.05); await R.close()
        check(await ev("S47.lostNights.books().length") == 1, 'chapter two, the binder read: Night Shift\'s book is on the board')
        await R.board('ns')
        pins = [await R.has(f'[data-node="ln.{k}"]') for k in ('b81', 'b83', 'b85')]
        check(all(pins) and not await R.has('.bcard[data-card=binder]') and not await R.has('[data-card="ln.pins"]') and not await R.has('[data-q="ln.q39"]'),
              f'the binder with a pin on each printout; nothing from Caliche yet, no question ({pins})')
        await R.shot('d01_binder')
        await R.close()

        # ---------- Caliche ----------
        await R.town(2 * 3600 + 58 * 60)
        # Earl, the real way: aimed at the buzzer, E
        await ev("""(() => { const p = S47.player, at = S47.world.town.proxies.amDoor.getWorldPosition(S47.camera.position.clone());
          p.place(at.x - 1.7, at.z, -Math.PI / 2); p.pitch = Math.atan2(at.y - (p.floorY + p.eye), 1.7); S47.tick(0.05); })()""")
        aim = await ev("(() => { const it = S47.game.d.inter.update(S47.camera); return it ? it.id : null; })()")
        await pg.keyboard.press('e'); await tick(0.05)
        await pg.wait_for_selector('.labpanel .talk')
        check(aim == 'town:amDoor' and '1420 AM / EARL' in await pg.locator('.labpanel h3').inner_text(), f'Earl at the studio door, aimed and E ({aim})')
        await R.choose('calls'); await R.choose('tape')
        ch = await ev("T.choices()")
        check(await ev("T.has('ask:host:tape')") and 'reel' not in ch, f'Earl: one reel he never played; no date to give him yet ({ch})')
        await R.close()
        # the attendant: the old road, then the ticket
        await R.talk('gasWindow')
        check('tow' not in await ev("T.choices()"), 'the attendant: no ticket before the old road')
        await R.choose('old'); await R.choose('tow')
        ls = await ev("T.lines()")
        att = [l['text'] for l in ls if l['who'] == 'THE ATTENDANT']
        voiced = await ev(f"S47.game.d.ui.npcDialogue.available({{ voice: 'stanton', lines: {json.dumps([{'who': 'THE ATTENDANT', 'text': t} for t in att])} }})")
        check('towslip' in await ev("T.docs()") and 'The tow ticket is in Papers.' in await ev("T.toasts") and await ev("T.count('carbon of the tow ticket')") == 1,
              'the tow ticket from his spike: in Papers, one note')
        check(len(att) == 3 and voiced, f'his three lines are ones he has a recording for ({att})')
        await pg.wait_for_timeout(4000)
        await R.shot('d02_attendant_ticket')
        await R.close()
        await ev("S47.game.d.ui.document(S47.game.docs.find((d) => d.id === 'towslip'), () => {})"); await tick(0.05)
        await pg.wait_for_timeout(600)
        check('Where\'s the other 2 hrs?' in await ev("document.querySelector('.docview').textContent"), 'the ticket in Papers: the hours that do not add up, in his pencil')
        await R.shot('d02b_tow_ticket')
        await R.close()
        # the woman on the bench, and the deputy (in, at twenty to three)
        await R.talk('woman'); await R.choose('radio'); await R.choose('husband'); await R.close()
        await ev("S47.game.clock = 2 * 3600 + 40 * 60; S47.world.town.update(S47.runTime(), S47.game.clock); S47.tick(0.05)")
        await R.talk('sheriffDoor'); await R.choose('map'); await R.choose('green'); await R.close()
        await ev("S47.game.clock = 2 * 3600 + 59 * 60; S47.world.town.update(S47.runTime(), S47.game.clock); S47.tick(0.05)")
        check(await ev("T.has('ask:deputy:green') && T.has('ask:radio_woman:husband')"), 'the deputy\'s green pins and the woman\'s husband, asked')

        # ---------- the board ----------
        await R.board('ns')
        cards = {k: await R.has(f'[data-node="ln.{k}"]') for k in ('g81', 'g83', 'g85', 'w81', 'w83', 'w85', 'tq', 'th')}
        check(all(cards.values()), f'on the board: the green pins, what the three said, the ticket, a pin each ({cards})')
        r = await R.thread('ln.b83', 'ln.g85')
        check('Not the same night' in r and 'ln.b83|ln.g85' not in await R.links(), f'-36 to the attendant: not the same night ({r})')
        r = await R.thread('ln.b81', 'ln.b85')
        check('same record' in r, f'two printouts of the binder: the same record ({r})')
        r = await R.thread('ln.b85', 'ln.g85')
        check(r == N85 and 'ln.b85|ln.g85' in await R.links() and await ev("T.has('ln:85')") and not await R.has('[data-q="ln.q39"]'),
              f'-38 to the green pin at the gas station holds: "{r}"')
        r = await R.thread('ln.b83', 'ln.w83')
        check('forty-seven minutes' in r and await ev("T.has('ln:83')"), f'-36 to the woman on the bench ({r})')
        check(not await R.has('[data-q="ln.q39"]'), 'two nights: no question yet')
        r = await R.thread('ln.b81', 'ln.w81')
        check(r.endswith('And a question with it: ' + Q39) and await R.has('[data-q="ln.q39"]') and await ev("T.has('ln:81')"),
              f'-34 to Earl\'s boy, and the question comes with it ({r})')
        check('Earl still has that call on reel. The binder has the minute: 01:52.' in r, 'the 1981 note says where that night is kept: the way back to Earl')
        q = await ev("document.querySelector('[data-q=\"ln.q39\"]').textContent")
        st = await ev("document.querySelector('.board-status').textContent")
        check(Q39 in q and 'OPEN' in q and 'Nobody has an answer for it. Not yet.' in q and not await R.has('[data-record="ln.q39"]') and st.startswith('OPEN:') and '-39' in st.split('/')[0],
              f'the question: open, nothing to record ({st})')
        check(await ev("T.count('Three nights SARO printed a negative range')") == 1, 'one line in the notebook for it')
        r = await R.thread('ln.b85', 'ln.tq')
        check('11:58' in r and 'watch' in r, f'-38 to the ticket\'s remarks: the minute ({r})')
        await ev("S47.game.board.view.look('ns')"); await tick(0.05)
        await R.shot('d03_board_nights')
        await R.close()

        # ---------- Earl, the date: walked away from ----------
        # (heard tonight: the boy's call on the Night Line, as main.ts writes it down when it is heard)
        await ev("S47.townSeen.add('ev:boy'); S47.game.note('02:31. Night Line, 1420. A boy calling about a star with a tail, low in the south, and a light over the mesa. From his father\\'s ranch. He said it was July.')")
        await R.talk('amDoor')
        ch = await ev("T.choices()")
        check('reel' in ch and 'boy' in ch, f'Earl: the date can be given him now ({ch})')
        await ev("T.listen()")
        await R.choose('reel')
        await pg.wait_for_function("T.lines().some((l) => l.who === 'REEL, 9/22/81')", timeout=30000, polling=100)
        check(await ev("S47.reelTape.busy") and await ev("T.choices()") == ['bye'] and not await R.has('[data-reading-kind="earl-dialogue"]'),
              'the reel plays; the conversation waits for it (no control for Earl\'s voice: he says nothing)')
        check(not await ev("T.has('ask:host:reel')") and not await ev("S47.lostNights.reelReady()"),
              'while it plays it is not yet asked (a save now would offer it again), and it cannot be started twice')
        await R.choose('bye')
        await tick(0.1)
        check(not await ev("S47.reelTape.busy") and not await ev("T.has('ask:host:reel')") and not await ev("T.has('ln:reel')")
              and await ev("T.count('I answered him tonight')") == 0 and 'witness-reel' not in await ev("T.docs()"),
              'walked away from mid-reel: nothing kept, no note, no paper')

        # ---------- heard to the end ----------
        await R.talk('amDoor')
        check('reel' in await ev("T.choices()"), 'and the date can be given him again')
        await R.choose('reel')
        setup = await ev("T.lines()")
        check(any('September twenty-second, 1981' in l['text'] for l in setup) and not any(l['who'] == 'EARL' for l in setup), 'the date given; he goes in to thread it')
        earl_status = await ev("(document.querySelector('[data-reading-kind=\"earl-dialogue\"] [data-reading-status]') || {}).textContent || ''")
        await pg.wait_for_function("T.lines().filter((l) => l.who === 'REEL, 9/22/81').length >= 2", timeout=40000, polling=100)
        await R.shot('d04_reel_playing')
        await pg.wait_for_function("!S47.reelTape.busy", timeout=60000, polling=200)
        await tick(0.05)
        ls = await ev("T.lines()")
        cues = await ev("S47.reelTape.cues")
        t0 = await ev("S47.reelTape.startedAt")
        boys = [c for c in cues if c['kind'] == 'boy']
        holes = [c for c in cues if c['kind'] == 'hole']
        check([c['text'] for c in boys] == BOY, 'on the reel: the boy\'s three lines, word for word as tonight\'s call')
        host = await ev("""[\"That's Halley's comet, son. It's there all week. You're up late.\", 'Is your dad up?'].map((t) => {
          const c = S47.radio.hostVoice ? S47.radio.hostBank.resolve(t, 0) : null; return c ? c.reduce((a, x) => a + x.duration, 0) : null; })""")
        check(len(holes) == 2 and all(h is not None and abs(holes[i]['secs'] - h) < 0.01 for i, h in enumerate(host))
              and all(abs(boys[i + 1]['at'] - (holes[i]['at'] + holes[i]['secs']) - 0.7) < 0.01 for i in range(2)),
              f'two holes on the tape, each as long as Earl\'s answer tonight ({[round(h["secs"], 2) for h in holes]} s; recorded host {host})')
        check(await ev("S47.reelTape.voiced"), 'the boy in his own recorded voice (the radio\'s caller bank)')
        # the sound: in the holes there is no hiss, only the deck turning
        lead = await ev(f"T.rmsIn({t0} + {boys[0]['at']} - 1.2, {t0} + {boys[0]['at']} - 0.15)")
        hole_rms = [await ev(f"T.rmsIn({t0} + {h['at']} + 0.15, {t0} + {h['at']} + {h['secs']} - 0.1)") for h in holes]
        voice = [await ev(f"T.rmsIn({t0} + {c['at']} + 0.3, {t0} + {c['at']} + {c['secs']} - 0.3)") for c in boys]
        ok = lead and all(hole_rms) and all(voice) and all(h < lead * 0.75 for h in hole_rms) and min(voice) > lead * 1.5
        check(bool(ok), f'measured: the tape\'s hiss before he speaks {lead and round(lead, 4)}, in the holes {[round(h or 0, 4) for h in hole_rms]}, the boy {[round(v or 0, 4) for v in voice]}')
        mlead = await ev(f"T.mIn({t0} + {boys[0]['at']} - 1.2, {t0} + {boys[0]['at']} - 0.15)")
        mhole = [await ev(f"T.mIn({t0} + {h['at']} + 0.15, {t0} + {h['at']} + {h['secs']} - 0.1)") for h in holes]
        check(bool(mlead and all(mhole) and all(h < mlead * 0.6 for h in mhole)),
              f'the whole mix (the night, the radios, the music too) drops in the holes: {mlead and round(mlead, 4)} before, {[round(h or 0, 4) for h in mhole]} in them')
        rl = [l['text'] for l in ls]
        check('I answered him tonight.' in rl and "He wasn't answering me." in rl and 'He left room for me anyway.' in rl and any('Word for word what you heard on the Night Line tonight' in t for t in rl),
              'then Earl: "I answered him tonight." "He wasn\'t answering me."')
        seen = await ev("S47.seen.filter((e) => e.what === 'reel').map((e) => [e.kind, e.end || null, e.looking])")
        check(seen == [['event', 'away', None], ['event', 'heard', None]], f'the seen log has the reel twice: walked away from, then heard ({seen})')
        earl_after = await ev("(document.querySelector('[data-reading-kind=\"earl-dialogue\"] [data-reading-status]') || {}).textContent || ''")
        check('could not be played' not in earl_after and (earl_after == '' or 'No recording' in earl_after), f'his new lines are text: "{earl_after}"')
        await R.shot('d05_reel_after')
        await R.close()
        await ev("S47.game.d.ui.document(S47.game.docs.find((d) => d.id === 'witness-reel'), () => {})"); await tick(0.05)
        await pg.wait_for_timeout(600)
        await R.shot('d05b_reel_paper')
        await R.close()
        await R.talk('amDoor')
        check(await ev("T.count('I answered him tonight. He wasn\\'t answering me.')") == 1 and 'witness-reel' in await ev("T.docs()")
              and await ev("T.has('ln:reel') && S47.radio.boyCalled") and 'Witness note filed in Papers.' in await ev("T.toasts"),
              'one note with his words, the reel in Papers, the boy will not call again tonight')
        ch = await ev("T.choices()")
        check('reel' not in ch and 'boy' not in ch, f'asked once; and he does not say he has not played it ({ch})')
        await R.close()
        await R.talk('amDoor')
        check('reel' not in await ev("T.choices()"), 'not offered again')
        await R.close()

        # ---------- the reel and tonight's boy ----------
        await R.board('ns')
        r = await R.thread('ln.reel', 'ln.boy')
        check('Earl answered a recording.' in r, f'the reel and tonight\'s boy: {r}')
        r = await R.thread('ln.reel', 'ln.w81')
        r2 = await R.thread('ln.boy', 'ln.g81')
        n81 = await ev("[...document.querySelectorAll('.bnote')].map((n) => n.textContent).find((t) => t.includes('-34')) || ''")
        check('He played it for me tonight.' in r and 'never' not in r and 'the same words' in r2 and 'never' not in r2
              and 'off the air' in n81 and 'still has' not in n81,
              f'once he has played it, nothing on the board says he never did ({r} / {r2} / {n81})')
        # a time away: the minutes under the question, in the player's hand
        await ev("S47.game.note('03:04 to 03:41. Nothing.'); S47.game.board.refresh()"); await tick(0.05)
        q = await ev("document.querySelector('[data-q=\"ln.q39\"]').textContent")
        hand = await ev("(document.querySelector('[data-q=\"ln.q39\"] .bq-else .pencil') || {}).textContent || ''")
        check('Under it, in my own hand:' in q and hand == '03:04 to 03:41' and 'I do not remember writing it.' in q, f'after a time away, the answer is under it in my own hand ({hand})')
        await ev("S47.game.board.view.look('ns')"); await tick(0.05)
        await R.shot('d06_board_after_reel')
        await R.close()

        # ---------- Continue ----------
        links_before = sorted(await R.links())
        notes_before = await ev("S47.game.notes.length")
        await ev("S47.saveNow('manual', 0)")
        await pg.wait_for_timeout(800)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev(HELPERS); await ev("T.hook()"); await ev("S47.hold = true; S47.tick(0.5)"); await R.quiet()
        await pg.add_style_tag(content='.lockhint{display:none !important}')
        check(sorted(await R.links()) == links_before and await ev("S47.lostNights.books().length") == 1, f'Continue: the threads are back ({len(links_before)})')
        d = await ev("T.docs()")
        check('towslip' in d and 'witness-reel' in d and await ev("S47.radio.boyCalled") and await ev("S47.game.notes.length") == notes_before,
              'the ticket and the reel in Papers, the boy has called, no note doubled')
        await R.board('ns')
        check(await R.has('[data-q="ln.q39"]') and await R.has('[data-node="ln.reel"]'), 'the question and the reel on the board again')
        inside = await ev("""(() => { const r = S47.game.boardSave.pos; const ns = { x: 40, y: 80, w: 1180, h: 760 };
          return ['ln.tow', 'ln.reel', 'ln.boy'].map((id) => r[id] && r[id][0] >= ns.x && r[id][0] < ns.x + ns.w && r[id][1] >= ns.y && r[id][1] < ns.y + ns.h); })()""")
        check(all(inside), f'the ticket, the reel and tonight\'s boy lie on Night Shift\'s patch with the rest ({inside})')
        await R.close()
        await tick(0.5, 10)
        check(await ev("S47.game.notes.length") == notes_before, 'nothing written again after Continue')

        # ---------- the diner, chapter five ----------
        await ev("S47.jump('chapter5'); S47.tick(1.0)"); await R.quiet()
        await ev("S47.game.d.inter.get('excBinder').use()"); await tick(0.05); await R.close()
        for k in ['met:attendant', 'ask:attendant:old', 'ask:attendant:tow']: await ev(f"S47.townSeen.add('{k}')")
        await ev("async () => { await S47.world.prepare('diner'); S47.world.enter('diner'); S47.world.placeAtDiner('inside'); }")
        await ev("S47.ch5.s.arrived = true; S47.ch5.s.stage = 'diner'; S47.tick(0.1)")
        st = await ev("S47.world.diner.anchors['stool']")
        await ev(f"S47.player.place({st['x']}, {st['z']}, 0); S47.tick(0.05)")
        v = await ev("(() => { const v = S47.world.diner.proxies['driver'].getWorldPosition(S47.camera.position.clone()); return [v.x, v.y, v.z]; })()")
        await ev(f"(() => {{ const p = S47.player; p.yaw = Math.atan2(-({v[0]} - p.pos.x), -({v[2]} - p.pos.z)); p.pitch = Math.atan2({v[1]} - (p.floorY + p.eye), Math.hypot({v[0]} - p.pos.x, {v[2]} - p.pos.z)); S47.tick(0.05); }})()")
        ex = await ev("(() => { const e = S47.game.d.inter.examine(S47.camera); return e ? e.text : null; })()")
        check(ex and 'two minutes to twelve' in ex and await ev("T.has('ln:watch')") and await ev("T.count('his wristwatch says two minutes to twelve')") == 1,
              f'a closer look at the driver: his watch ({ex})')
        await ev("S47.game.d.inter.examine(S47.camera)")
        check(await ev("T.count('his wristwatch says two minutes to twelve')") == 1, 'written down once')
        await ev("S47.game.d.inter.get('diner:driver').use()"); await tick(0.1)
        ch = await ev("T.choices()")
        check('ln:watch' in ch and 'ln:tow' not in ch, f'the driver: his watch; the ticket only once he has told about October ({ch})')
        await R.choose('ln:watch')
        check(any('Two minutes to midnight' in l['text'] for l in await ev("T.lines()")), 'his watch quit the same minute as his rig')
        await R.choose('october');
        check('ln:tow' in await ev("T.choices()"), 'now the ticket can be shown')
        await R.choose('ln:tow')
        ls = [l['text'] for l in await ev("T.lines()")]
        check('Nobody towed me. She came back on by herself, and I drove her in.' in ls and await ev("T.count('He does not remember a tow')") == 1, 'shown the ticket: nobody towed him')
        check(any('at twenty-five past four' in t for t in ls), 'what Reyes reads him off the ticket is the ticket\'s time (in at 4:25)')
        await pg.wait_for_timeout(5000)
        await R.shot('d07_driver_ticket')
        await R.close()
        await R.board('ns')
        r = await R.thread('ln.b85', 'ln.watch')
        check('11:58' in r and 'watch' in r, f'the watch and the binder\'s -38 ({r})')
        await ev("S47.game.board.view.show('ln.watch')"); await tick(0.05)
        await R.shot('d08_board_watch')
        await R.close()
        check(not errs, f'desktop: no console errors or warnings ({errs[:4]})')
        await ctx.close()

        # ======================= phone =======================
        ctx, pg, errs = await page(b, 412, 892, True)
        R = Run(pg, True); ev = pg.evaluate; tick = R.tick
        await ev("S47.hold = true")
        await ev("S47.jump('chapter2'); S47.tick(1.0)"); await R.quiet()
        # a date from Caliche, the binder not read: the binder lies there, not on the table yet
        await ev("S47.townSeen.add('met:host'); S47.townSeen.add('ask:host:calls'); S47.townSeen.add('ask:host:tape')")
        await R.board('ns')
        away = await ev("(document.querySelector('.bcard.away[data-card=\"ln.binder\"]') || {}).textContent || ''")
        check('over the printer' in away and await R.has('[data-node="ln.w81"]') and not await R.has('[data-node="ln.b81"]'),
              f'phone: Earl\'s date and no binder read: the binder is a card not on the table yet ({away.strip()[:90]})')
        await R.close()
        await ev("S47.game.d.inter.get('excBinder').use()"); await tick(0.05); await R.close()
        for k in ['met:deputy', 'ask:deputy:map', 'ask:deputy:green', 'met:host', 'ask:host:calls', 'ask:host:tape', 'met:attendant', 'ask:attendant:old']:
            await ev(f"S47.townSeen.add('{k}')")
        await R.board('ns')
        r1 = await R.thread('ln.b85', 'ln.g85'); r2 = await R.thread('ln.b83', 'ln.g83'); r3 = await R.thread('ln.b81', 'ln.g81')
        check(r1 == N85 and Q39 in r3 and await R.has('[data-q="ln.q39"]'), 'phone: three threads tied with taps, the question comes')
        await ev("S47.game.board.view.show('ln.g85')"); await tick(0.05)
        await R.shot('p01_board')
        over = await ev("document.documentElement.scrollWidth > innerWidth + 1")
        check(not over, 'phone: no horizontal scroll with the board open')
        await R.close()
        await R.town(3 * 3600 + 5 * 60)
        await R.talk('amDoor')
        await R.choose('reel')
        await pg.wait_for_function("T.lines().filter((l) => l.who === 'REEL, 9/22/81').length >= 1", timeout=40000, polling=100)
        await pg.wait_for_timeout(600)
        vis = await ev("(() => { const l = [...document.querySelectorAll('.talk-line')].pop().getBoundingClientRect(); return l.bottom <= innerHeight + 1 && l.top >= 0; })()")
        check(vis, 'phone: the newest line of the reel is in view as it comes')
        await R.shot('p02_reel_playing')
        await pg.wait_for_function("!S47.reelTape.busy", timeout=60000, polling=200)
        await pg.wait_for_timeout(1500)
        rl = [l['text'] for l in await ev("T.lines()")]
        check('He wasn\'t answering me.' in rl and any('That\'s what he said tonight' in t for t in rl), 'phone: heard to the end; not having heard it tonight, Earl says so')
        await R.shot('p03_reel_after')
        check(not await ev("document.documentElement.scrollWidth > innerWidth + 1"), 'phone: no horizontal scroll in the conversation')
        await R.close()
        check(not errs, f'phone: no console errors or warnings ({errs[:4]})')
        await ctx.close()
        await b.close()
    fails = [c for c in checks if c[0] == 'FAIL']
    print(f'\n{len(checks) - len(fails)} of {len(checks)} PASS')
    with open(f'{OUT}/results.json', 'w') as f: json.dump(checks, f, indent=1)
    sys.exit(1 if fails else 0)

asyncio.run(main())
