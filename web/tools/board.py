# The case board (ui/Board.ts, story/caseBoard.ts, story/boardRules.ts) in a headless browser:
# the Board entry in the notebook and the B key, everything in the journal on the board as
# cards, moving cards with the mouse and keeping where they lie (closing the board, saving,
# Continue), threads pulled from a pin and tied by two taps, notes, wrong threads that fall
# slack, RECORD for P04 (chapter two), P06, P08 and P09 (chapter three: the field record that
# used to be a panel with tabs) and P11 and P12 (chapter four: no more three sentences), the
# threads the story draws for P10, panning and zooming, and a phone held both ways with touch:
# tapping two pins, dragging a card and the cork with a finger, and pinching.
# Also covers chapter one's reference, alternative hypotheses, controlled exposures,
# unfinished threads after Continue, old saves without threads, and the panel fallback.
# Starts from S47.jump('chapter1') through ('chapter4'), with small stand-in photographs.
# Usage: python3 tools/board.py OUTDIR
import asyncio, sys, json, os
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
ARGS = ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required']
# a small grey picture for the field frames, which the test does not take with the camera
GREY = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAAAAAA6fptVAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=='
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

async def start(b, viewport, touch=False, clear=True):
    ctx = await b.new_context(viewport=viewport, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
    except Exception: pass
    if clear:
        await pg.evaluate("localStorage.clear()")
        await pg.reload()
    return ctx, pg, errs

class Board:
    def __init__(self, pg): self.pg = pg; self.ev = pg.evaluate
    async def say(self): return await self.ev("(document.querySelector('.board-wrap [data-say]')||{}).textContent || ''")
    async def has(self, sel): return await self.ev(f"!!document.querySelector('{sel}')")
    async def centre(self, sel):
        bb = await self.pg.locator(sel).first.bounding_box()
        return bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2
    # tap one end and then the other, with the mouse
    async def thread(self, a, b):
        await self.pg.click(f'[data-node="{a}"]'); await self.pg.click(f'[data-node="{b}"]')
        return await self.say()
    async def thread_visible(self, a, b, touch=False):
        for node in (a, b):
            await self.ev(f'S47.game.board.view.show({json.dumps(node)})')
            if touch:
                x, y = await self.centre(f'[data-node="{node}"]'); await self.pg.touchscreen.tap(x, y)
            else: await self.pg.click(f'[data-node="{node}"]')
        return await self.say()
    async def reveal(self, sel):
        # Open questions have no evidence pin for show(). Pan to the actual element.
        x, y = await self.centre(sel)
        bb = await self.pg.locator('[data-view]').bounding_box()
        vx, vy = bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2
        await self.pg.mouse.move(vx, vy); await self.pg.mouse.wheel(x - vx, y - vy)
        await self.pg.wait_for_timeout(100)
    async def record(self, q, touch=False):
        await self.reveal(f'[data-record="{q}"]')
        if touch:
            x, y = await self.centre(f'[data-record="{q}"]'); await self.pg.touchscreen.tap(x, y)
        else: await self.pg.click(f'[data-record="{q}"]')
    # pull the thread with the mouse, from the pin of a to b
    async def pull(self, a, b):
        x0, y0 = await self.centre(f'[data-pull="{a}"]'); x1, y1 = await self.centre(f'[data-node="{b}"]')
        await self.pg.mouse.move(x0, y0); await self.pg.mouse.down()
        for i in range(1, 11): await self.pg.mouse.move(x0 + (x1 - x0) * i / 10, y0 + (y1 - y0) * i / 10)
        await self.pg.mouse.up()
        return await self.say()
    async def drag(self, sel, dx, dy):
        x0, y0 = await self.centre(sel)
        await self.pg.mouse.move(x0, y0); await self.pg.mouse.down()
        for i in range(1, 9): await self.pg.mouse.move(x0 + dx * i / 8, y0 + dy * i / 8)
        await self.pg.mouse.up()
    async def view(self): return await self.ev("S47.game.board.view.view()")
    async def close(self): await self.pg.click('.board-wrap .lp-close'); await self.ev("S47.tick(0.1)")

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=ARGS)
        ctx, pg, errs = await start(b, {'width': 1280, 'height': 800})
        errlists = [errs]
        ev = pg.evaluate
        B = Board(pg)
        async def tick(s): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(500)

        # ---------- chapter one: the same board, and the original story functions ----------
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter1'); S47.tick(1.0)")
        await pg.keyboard.press('KeyB'); await tick(0.1)
        check(await B.has('.board-overlay') and not await B.has('.bq[data-q=P01]'), 'chapter one: B opens the board before any photograph, without a finding to record')
        check(await B.has('.bcard[data-card=refsheet].away') and not await B.has('[data-node="refsheet.b12"]'), 'the installation sheet must be read in the photo lab, not taken from the board')
        check(not (await ev("S47.ch1.book.record('P01')"))['ok'] and not await ev("S47.ch1.s.ref"), 'a finding cannot be recorded with no supporting thread')
        await B.close()
        await ev(f"""const s = S47.ch1.s; s.stage = 'interpret-first'; s.camera = true; s.log = true; s.dev1 = 3;
          s.f1 = {{ id: 'frame01', subject: '', method: 'baseline', clock: 0, url: '{GREY}', targets: {{vane: [0.5, 0.5]}} }};
          S47.ch1.applyWorld(); S47.game.openBoard()""")
        check(await B.has('.bq[data-q=P01]') and not await B.has('[data-node="frame01.detail"]'), 'a developed print needs its detail marked at the archive before it has comparison pins')
        await B.close()
        await ev("S47.ch1.s.mark1 = [0.5, 0.5]; S47.game.d.inter.get('archive').use(); S47.tick(0.1)")
        check(await B.has('.board-overlay') and await ev("S47.ch1.s.sheet") and not await B.has('[data-r]'), 'reading the reference file lays out the chapter-one board instead of the old choices')
        r = await B.thread_visible('frame01.detail', 'refsheet.r07')
        check('THREE HORIZONTAL BARS' in r and await ev("S47.ch1.s.wrongRef") == 1 and await B.has('.board-threads path.slack'), 'R-07 contradicts the marked detail: the old reply, one mistake and a slack thread')
        r = await B.thread_visible('refsheet.b12', 'refsheet.r07')
        check('same record' in r and await ev("S47.ch1.s.wrongRef") == 1, 'two lines on the installation sheet do not make a finding or count as another wrong answer')
        for _ in range(3): await pg.click('[data-hint=P01]')
        third = await B.say()
        await pg.click('[data-hint=P01]')
        check(await B.say() == third and 'Thread' not in third, 'the fourth chapter-one hint waits for two more misses after the third hint')
        await B.thread_visible('s03log.heading', 'refsheet.r07')
        await B.thread_visible('s03log.zero', 'refsheet.b12')
        await pg.click('[data-hint=P01]')
        check('thread its marked detail' in await B.say(), 'after two more misses the fourth hint names the useful thread')
        r = await B.thread_visible('frame01.detail', 'refsheet.b12')
        check('RECORD it' in r and not await ev("S47.ch1.s.ref"), 'P01 needs RECORD after tying the matching reference')
        await B.record('P01'); await tick(0.1)
        check(await ev("S47.ch1.s.ref") and await B.has('.bq.done[data-q=P01]') and await B.has('.bq[data-q=P02]'), 'RECORD calls the original B-12 match and opens the working hypothesis')
        r = await B.thread_visible('hyp.command', 's03log.zero')
        check('ZERO commands' in r and await ev("S47.ch1.s.wrongHyp") == 1, 'the second-command hypothesis keeps its original rejection')
        await B.thread_visible('hyp.light', 'refsheet.lamp')
        await B.thread_visible('hyp.drift', 'refsheet.drive')
        check(await ev("S47.ch1.s.links.filter(k => k.startsWith('hyp.')).length") == 1
              and 'encoder drift' in await ev("document.querySelector('.bq[data-q=P02] .bq-slots').textContent"), 'a different testable hypothesis replaces the earlier draft thread and its note')
        await B.close()
        await ev("S47.saveNow('manual', 0)"); await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])'); await pg.click('button[data-a=cont]')
        await pg.wait_for_function('S47.started()', polling=200); await ev("S47.hold = true; S47.tick(0.5); S47.game.openBoard()")
        check(await ev("S47.ch1.s.hyp === null && S47.ch1.s.stage === 'interpret-first' && S47.ch1.s.sheet")
              and await B.has('[data-record=P02]'), 'Continue keeps the read sheet and the unrecorded hypothesis thread; it does not advance the story')
        await B.record('P02'); await tick(0.1)
        check(await ev("S47.ch1.s.hyp === 'drift' && S47.ch1.s.stage === 'choose-control'"), 'RECORD calls the original encoder-drift hypothesis and releases the B-12 field test')
        await B.thread_visible('hyp.light', 'refsheet.lamp')
        check(await ev("S47.ch1.s.hyp === 'drift' && !S47.ch1.s.links.includes('hyp.light|refsheet.lamp')"), 'a recorded working hypothesis and its thread cannot change later')
        r = await B.thread_visible('hyp.command', 's03log.zero')
        check('working hypothesis is on file' in r and await ev("S47.ch1.s.wrongHyp") == 1, 'reviewing an already recorded hypothesis keeps the original reply and does not add a mistake')
        await B.close()
        await ev(f"""const s = S47.ch1.s; s.method = 'active'; s.observed = true; s.dev2 = 3; s.stage = 'compare-exposures';
          s.f2 = {{ id: 'frame02', subject: '', method: 'active', clock: 0, url: '{GREY}', targets: {{vane: [0.5, 0.5]}} }};
          S47.ch1.applyWorld(); S47.game.openBoard()""")
        check(not await B.has('[data-node="frame02.extra"]') and not (await ev("S47.ch1.conclude('second')"))['ok'], 'FRAME 02 still has to be marked before its conclusion can be recorded')
        await ev("S47.ch1.s.mark2 = [0.5, 0.5]; S47.game.board.refresh()")
        r = await B.thread_visible('frame02.extra', 'observation.lamp')
        check('vane moved to a known angle' in r and await ev("S47.ch1.s.wrongConcl") == 1, 'the active control gives the original lamp rejection')
        r = await B.thread_visible('frame02.extra', 's03log.zero')
        check('do not support two antenna commands' in r and await ev("S47.ch1.s.wrongConcl") == 2, 'the two-command conclusion gives the original reply and counts once')
        await B.thread_visible('frame01.extra', 'frame02.extra')
        check(not await B.has('[data-record=P03]'), 'P03 requires the direct control observation as well as the two prints')
        await B.thread_visible('observation.stripe', 'frame02.detail'); await B.record('P03'); await tick(0.1)
        check(await ev("S47.ch1.s.concluded && S47.ch1.s.stage === 'file-report' && !S47.ch1.s.filed"), 'RECORD calls the original two-exposure finding; filing the report is still a separate action')
        check('finding' in await ev("S47.game.docs.map(d => d.id)") and not await B.has('.bcard[data-card=finding]'), 'the finding is in the journal and only once on the board, as P03')
        r = await B.thread_visible('frame02.extra', 's03log.zero')
        check('Finding recorded' in r and await ev("S47.ch1.s.wrongConcl") == 2, 'reviewing a recorded conclusion does not count another mistake')
        await ev("S47.game.board.view.look('ch1')")
        await shot('b00_board_ch1_recorded')
        await B.close()
        await ev("delete S47.ch1.s.links; delete S47.ch1.s.sheet; S47.saveNow('manual', 0)")
        await pg.reload(); await pg.wait_for_selector('button[data-a=cont]:not([disabled])'); await pg.click('button[data-a=cont]')
        await pg.wait_for_function('S47.started()', polling=200); await ev("S47.hold = true; S47.tick(0.5); S47.game.openBoard()")
        check(await ev("S47.ch1.s.links.length === 4 && S47.ch1.s.links.includes('hyp.drift|refsheet.drive') && !S47.ch1.s.links.includes('hyp.light|refsheet.lamp')")
              and await pg.locator('.bq.done[data-q=P01], .bq.done[data-q=P02], .bq.done[data-q=P03]').count() == 3, 'an old save without threads restores every finding and the actual encoder-drift hypothesis')
        await B.close()
        await ev("S47.ch1.testBoardHook = S47.ch1.d.board; S47.ch1.d.board = undefined; S47.ch1.openReference()")
        check(await B.has('[data-r="B-12"]') and await B.has('[data-h=light]'), 'without a board hook the original reference panel remains available')
        await ev("S47.game.d.ui.close(); S47.ch1.openCompare()")
        check(await B.has('[data-c=second]') and not await B.has('[data-a=board]'), 'without a board hook the original two-exposure panel remains available')
        await ev("S47.ch1.d.board = S47.ch1.testBoardHook; S47.game.d.ui.close(true)")
        await ctx.close()
        ctx, pg, errs2 = await start(b, {'width': 1280, 'height': 800})
        errlists.append(errs2)
        ev = pg.evaluate; B = Board(pg)
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(500)

        # ---------- chapter two: the notebook, the board, the archive records ----------
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter2'); S47.tick(1.0)")
        await ev("S47.game.openNotebook()")
        check(await B.has('.notebook .nb-tabs .nb-boardtab'), 'the notebook has a Board entry first in its row of tabs')
        await pg.click('.notebook .nb-boardtab'); await tick(0.1)
        check(await B.has('.board-overlay') and not await B.has('.notebook'), 'Board opens the case board')
        check('CASE BOARD' in await ev("document.querySelector('.board-wrap h3').textContent")
              and 'P04' in await ev("document.querySelector('.board-status').textContent"), 'the board is the case board, with P04 and P05 open')
        ids = await ev("[...document.querySelectorAll('.bcard')].map(c => c.dataset.card)")
        check(all(i in ids for i in ('log', 's03log', 'refsheet', 'observation', 'e07', 'e06', 'rx', 'b12', 'e08')) and await B.has('.bq.done[data-q=P03]'),
              f'everything in the journal is on the board, with the archive records ({len(ids)} cards)')
        check('printout' not in ids and await B.has('.bcard[data-card=e07].away'), 'the printout is the RX card; a record not found yet says where it is')
        labels = await ev("[...document.querySelectorAll('.board-label')].map(l => l.textContent)")
        check(labels == ['NIGHT SHIFT', '1  THE SECOND EXPOSURE', '2  THE AMENDED RECORD'], f'each part of the night so far has its patch of cork ({labels})')
        await shot('b01_board_ch2_empty')
        await B.close()
        # the records, read (as chapter2.py reads them on the shelves)
        await ev("S47.ch2.s.read = { original: true, amended: true, lineage: true, index: true }; S47.ch2.applyWorld(); S47.tick(0.1)")
        await pg.keyboard.press('KeyB'); await tick(0.1)
        check(await B.has('.board-overlay'), 'B opens the board from the game')
        await pg.keyboard.press('KeyB'); await tick(0.1)
        check(not await B.has('.board-overlay'), 'and closes it')
        await ev("S47.game.openNotebook()"); await pg.keyboard.press('KeyB'); await tick(0.1)
        check(await B.has('.board-overlay'), 'B in the notebook goes to the board')
        check(await B.has('[data-node="e07.abc"]') and await B.has('.bcard[data-card=e07] .bcard-img'), 'E07 lies on the board with its drawing and its lines')
        v0 = await B.view()
        check(0.6 < v0['s'] < 1.3, f"opened on the archive's patch at a readable size (scale {v0['s']:.2f})")

        # moving a card with the mouse, by its head (the printout, which then covers nothing)
        left0 = await ev("parseFloat(document.querySelector('.bcard[data-card=rx]').style.left)")
        await B.drag('.bcard[data-card=rx] .bcard-head span', -120, -40)
        left1 = await ev("parseFloat(document.querySelector('.bcard[data-card=rx]').style.left)")
        moved = left1 - left0
        check(abs(moved + 120 / v0['s']) < 3, f'a card follows the mouse ({moved:.0f} board px for -120 screen px)')
        check(await ev("JSON.stringify(S47.game.boardSave.pos.rx)") is not None, 'and its place is kept in the case')
        # the motor log now has evidence lines; its head still moves the card
        sl0 = await ev("parseFloat(document.querySelector('.bcard[data-card=s03log]').style.top)")
        await ev("S47.game.board.view.look('ch1')"); await tick(0.05)
        await B.drag('.bcard[data-card=s03log] .bcard-head span', 0, 80)
        sl1 = await ev("parseFloat(document.querySelector('.bcard[data-card=s03log]').style.top)")
        check(sl1 - sl0 > 40, 'a paper from chapter one moves when dragged by its head')
        await ev("S47.game.board.view.look('ch2')"); await tick(0.05)

        # P04: a thread pulled from a pin, a wrong one, then two taps
        prior_threads = await ev("document.querySelectorAll('[data-threads] path.thread').length")
        r = await B.pull('e07.abc', 'e06.omit')
        check('leaves it out' in r and '(P04: 1 of 2)' in r and await ev("document.querySelectorAll('.bq[data-q=P04] .bq-slots li.on').length") == 1,
              'a thread pulled from the three references to the omitted sight line: C goes into P04')
        check(await ev("document.querySelectorAll('[data-threads] path.thread').length") == prior_threads + 1, 'and stays on the board, red')
        r = await B.thread('e06.fault', 'e07.sig')
        slack = await B.has('.board-threads path.slack'); w4 = await ev("S47.ch2.s.wrong04")
        check('shared signature' in r and w4 == 1 and slack, f'a wrong thread falls slack with the Unity reply and counts ({r[:50]!r}, wrong {w4}, slack {slack})')
        # across the board (zoomed out): a paper from chapter one and a line from 1947
        await pg.click('.board-zoom [data-z=all]'); await tick(0.05)
        r = await B.thread('s03log.heading', 'e07.head')
        check('do not say anything' in r and await ev("S47.ch2.s.wrong04") == 1, 'two cards that say nothing about each other: a short reply, nothing counted')
        await ev("S47.game.board.view.look('ch2')"); await tick(0.05)
        r = await B.thread('e07.mark', 'e06.fault')
        check('RECORD it' in r and await B.has('[data-record=P04]'), 'the retained mark and the development fault fill P04: RECORD')
        await shot('b02_p04_ready')
        await pg.click('[data-record=P04]'); await tick(0.1)
        check(await ev("S47.ch2.s.p04") and 'p04' in await ev("S47.game.docs.map(d => d.id)") and await B.has('.bq.done[data-q=P04] > .pin'), 'P04 recorded; the finding card has a pin of its own now')
        check('p04' not in await ev("[...document.querySelectorAll('.bcard')].map(c => c.dataset.card)"), 'the finding is its question card, not a second card')
        r = await B.pull('e07.sig', 'e06.sig')
        check(await B.has('.bnote[data-note="n.sig"]'), 'a thread that answers nothing leaves a note on the board')
        # zoom with the buttons and the wheel, pan with the cork
        await pg.click('.board-zoom [data-z=all]'); await tick(0.05)
        v2 = await B.view()
        check(v2['s'] < v0['s'] * 0.6, f"ALL shows the whole board (scale {v2['s']:.2f})")
        await shot('b03_whole_board')
        await pg.click('.board-zoom [data-z=in]'); v3 = await B.view()
        check(abs(v3['s'] / v2['s'] - 1.25) < 0.01, 'the + button zooms in')
        await pg.mouse.move(640, 400); await pg.keyboard.down('Control'); await pg.mouse.wheel(0, -200); await pg.keyboard.up('Control')
        v4 = await B.view()
        check(v4['s'] > v3['s'], 'ctrl and the wheel zoom too')
        await ev("S47.game.board.view.look('ch2')")
        tx0 = (await B.view())['tx']
        empty = await ev("""(() => { const v = document.querySelector('[data-view]').getBoundingClientRect();
          for (let y = v.top + 20; y < v.bottom - 20; y += 12) for (let x = v.left + 20; x < v.right - 20; x += 12) {
            const e = document.elementFromPoint(x, y); if (e && e.classList.contains('board')) return [x, y]; } return null; })()""")
        await pg.mouse.move(empty[0], empty[1]); await pg.mouse.down(); await pg.mouse.move(empty[0] + 150, empty[1], steps=6); await pg.mouse.up()
        check(abs((await B.view())['tx'] - tx0 - 150) < 2, 'dragging the cork moves the view')
        await B.close()
        pos = await ev("S47.game.boardSave.pos.rx")
        await ev("S47.game.openBoard()"); await tick(0.05)
        check(await ev("parseFloat(document.querySelector('.bcard[data-card=rx]').style.left)") == pos[0], 'closed and opened again, the card lies where it was left')
        await B.close()

        # saved with the case, back after Continue
        await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(600)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)")
        await ev("S47.game.openBoard()"); await tick(0.05)
        check(await ev("JSON.stringify(S47.game.boardSave.pos.rx)") == json.dumps(pos, separators=(',', ':'))
              and await ev("parseFloat(document.querySelector('.bcard[data-card=rx]').style.left)") == pos[0], 'after Continue the cards lie where they were saved')
        check(await B.has('.bq.done[data-q=P04]') and await B.has('.bnote[data-note="n.sig"]') and await ev("S47.ch2.s.links.length") == 3, 'and the threads and notes are back')
        await B.close()
        await ctx.close()

        # ---------- chapter three: the field record on the board ----------
        ctx, pg, errs3 = await start(b, {'width': 1280, 'height': 800})
        errlists.append(errs3); ev = pg.evaluate; B = Board(pg)
        async def tick(s): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(500)
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter3'); S47.tick(1.0)")
        await ev("const s = S47.ch3.s; s.stage = 'station'; s.arrived = true; s.readE09a = true; S47.ch3.applyWorld()")
        await ev("S47.game.openBoard()"); await tick(0.05)
        check(await B.has('.bq[data-q=P06]') and await B.has('.bq[data-q=P07]') and await B.has('.bq[data-q=P08]') and await B.has('.bq[data-q=P09]'), 'chapter three: P06 to P09 are on the board')
        check(await B.has('.bcard[data-card=frame03].away') and await B.has('.bcard[data-card=e10].away') and await B.has('[data-node="e09a.a"]'),
              'E09A is read; FRAME 03 and E10 say where they are')
        await ev(f"S47.ch3.s.f3 = {{ id: 'frame03', subject: '', method: 'field', clock: 0, url: '{GREY}', targets: {{}} }}; S47.game.board.refresh()")
        check(await B.has('.bcard[data-card=frame03].k-sealed'), 'FRAME 03, exposed, lies on the board sealed until it is developed')
        r = await B.thread('e09a.b', 'frame03')
        check('B is still in its footing' in r and await ev("S47.ch3.s.wrong06") == 1, 'P06: B is answered and counts')
        r = await B.pull('e09a.a', 'frame03')
        check('RECORD it' in r, 'A on E09A tied to FRAME 03: P06 can be recorded')
        await pg.click('[data-record=P06]'); await tick(0.1)
        check(await ev("S47.ch3.s.p06") and 'p06' in await ev("S47.game.docs.map(d => d.id)"), 'P06 recorded through the chapter\'s own rule')
        await ev(f"const s = S47.ch3.s; s.cableInspected = true; s.f4 = {{ id: 'frame04', subject: '', method: 'field', clock: 0, url: '{GREY}', targets: {{}} }}; S47.game.board.refresh()")
        check(not await B.has('[data-node="P07"]') and 'Made at the field lamp' in await ev("document.querySelector('.bq[data-q=P07]').textContent"), 'P07 is made at the lamp, not on the board')
        await ev("const s = S47.ch3.s; s.lampCovered = true; s.lampObserved = true; s.p07 = true; S47.ch3.file(S47.ch3.docFinding('p07')); S47.game.board.refresh()")
        r = await B.thread('frame04', 'e09a.a')
        check('do not identify the moved fixed point' in r, 'FRAME 04 against A: the Unity reply')
        r = await B.thread('frame04', 'P07')
        check('RECORD it' in r, 'FRAME 04 tied to the null test: P08 can be recorded')
        await pg.click('[data-record=P08]'); await tick(0.1)
        check(await ev("S47.ch3.s.p08"), 'P08 recorded')
        await ev("const s = S47.ch3.s; s.readE10 = true; s.heard = true; S47.ch3.applyWorld(); S47.game.board.refresh()")
        r = await B.thread('hut.rx', 'e10.relay')
        check('Do not infer a new event' in r and await ev("S47.ch3.s.wrong09") == 1, 'P09: the relay line is answered with the Unity reply')
        r = await B.thread('hut.rx', 'e10.carrier')
        check('RECORD it' in r, 'the receiver tied to the same words in E10')
        await pg.click('[data-record=P09]'); await tick(0.1)
        check(await ev("S47.ch3.s.p09") and await ev("S47.ch3.s.stage") == 'call', 'P09 recorded: the field telephone will ring')
        await shot('b04_board_ch3')
        await B.close()
        await ctx.close()

        # ---------- chapter four: her papers on the board ----------
        ctx, pg, errs4 = await start(b, {'width': 1280, 'height': 800})
        errlists.append(errs4); ev = pg.evaluate; B = Board(pg)
        async def tick(s): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(500)
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter4'); S47.tick(1.0)")
        await ev(f"""const s = S47.ch1.s; s.mark1 = [0.5, 0.5]; s.mark2 = [0.5, 0.5];
          s.f1 = {{ id: 'frame01', subject: '', method: 'baseline', clock: 0, url: '{GREY}', targets: {{}} }};
          s.f2 = {{ id: 'frame02', subject: '', method: 'passive', clock: 0, url: '{GREY}', targets: {{}} }};
          S47.ch1.applyWorld()""")
        await ev("""const s = S47.ch4.s; s.stage = 'room'; s.arrived = true; s.greeted = true; s.shown = true; s.p10 = true;
          s.topics = { record: true, tomas: true, cable: true }; s.tone = 'mild'; s.line = true;
          S47.ch4.give('correction'); S47.ch4.give('letter'); S47.ch4.give('card'); S47.ch4.file(S47.ch4.docFinding('p10'))""")
        await ev("S47.game.openBoard()"); await tick(0.05)
        check(await B.has('.bq.done[data-q=P10]') and await B.has('[data-node="frame02.detail"]')
              and await ev("document.querySelectorAll('[data-threads] path.thread.auto').length") == 2,
              'P10 from the talk keeps both automatic threads, to the control print with line pins and the 1947 record')
        check(await B.has('[data-node="e12.why"]') and await B.has('[data-node="r6.line"]'), 'her correction, his letter, his card and the return line are on the board')
        await pg.click('.board-zoom [data-z=all]'); await tick(0.05)
        r = await B.thread('e12.dev', 'e06.fault')
        check('not a development fault' in r and await ev("S47.ch4.s.wrong11") == 1, 'P11: a development fault is answered from her own correction')
        r = await B.thread('e12.why', 'P04')
        check('(P11: 1 of 2)' in r, 'her reason tied to P04, across the board')
        r = await B.thread('e12.cut', 'P08')
        check('RECORD it' in r, 'the deliberate interruption tied to the cut cable: P11 can be recorded')
        await ev("S47.game.board.view.look('ch4')"); await tick(0.05)
        await pg.click('[data-record=P11]'); await tick(0.1)
        check(await ev("S47.ch4.s.p11") and 'p11' in await ev("S47.game.docs.map(d => d.id)"), 'P11 recorded')
        r = await B.thread('r6.line', 'e13.twice')
        check('same fragment' in r and await ev("S47.ch4.s.wrong12") == 1, 'P12: "he answered" is refused')
        await pg.click('.board-zoom [data-z=all]'); await tick(0.05)
        r = await B.thread('r6.line', 'e10.carrier')
        check('RECORD it' in r, 'the return line tied to the 1947 voice in E10')
        await ev("S47.game.board.view.look('ch4')"); await tick(0.05)
        await pg.click('[data-record=P12]'); await tick(0.1)
        check(await ev("S47.ch4.s.p12") and 'p12' in await ev("S47.game.docs.map(d => d.id)"), 'P12 recorded')
        await shot('b05_board_ch4')
        await B.close()
        # the talk takes it up again
        await ev("S47.ch4.openTalk()"); await tick(0.1)
        text = await ev("document.querySelector('.talk').textContent")
        check('It is not him' in text and 'end' in await ev("[...document.querySelectorAll('.talk [data-c]')].map(b => b.dataset.c)"), 'back at her table she answers, and the last question is there')
        await ev("S47.game.d.ui.close(true)")
        await ctx.close()

        # ---------- a phone, both ways, with touch ----------
        for (w, h, name) in ((844, 390, 'landscape'), (390, 844, 'portrait')):
            ctx, pg, errsp = await start(b, {'width': w, 'height': h}, touch=True)
            errlists.append(errsp); ev = pg.evaluate; B = Board(pg)
            async def tick(s): await ev(f"S47.tick({s})")
            cdp = await ctx.new_cdp_session(pg)
            async def touch(*paths):
                # one finger per path; each path a list of points, all the same length
                n = len(paths[0])
                pts = lambda i: [{'x': p[i][0], 'y': p[i][1], 'id': k} for k, p in enumerate(paths)]
                await cdp.send('Input.dispatchTouchEvent', {'type': 'touchStart', 'touchPoints': pts(0)})
                for i in range(1, n): await cdp.send('Input.dispatchTouchEvent', {'type': 'touchMove', 'touchPoints': pts(i)})
                await cdp.send('Input.dispatchTouchEvent', {'type': 'touchEnd', 'touchPoints': []})
            await pg.wait_for_selector('button[data-a=start]')
            await pg.tap('button[data-a=start]'); await begin_opening(pg, touch=True); await pg.wait_for_timeout(500)
            await ev(f"""S47.hold = true; S47.tick(0.3); S47.jump('chapter1'); S47.tick(1.0);
              const s = S47.ch1.s; s.stage = 'interpret-first'; s.camera = true; s.log = true; s.sheet = true; s.dev1 = 3; s.mark1 = [0.5, 0.5];
              s.f1 = {{ id: 'frame01', subject: '', method: 'baseline', clock: 0, url: '{GREY}', targets: {{}} }};
              S47.ch1.applyWorld(); S47.game.openNotebook()""")
            await pg.tap('.notebook .nb-boardtab'); await tick(0.1)
            r = await B.thread_visible('frame01.detail', 'refsheet.r07', touch=True)
            check('THREE HORIZONTAL BARS' in r and await ev("S47.ch1.s.wrongRef") == 1, f'{name}: two finger taps reject R-07 on the chapter-one board')
            await B.thread_visible('frame01.detail', 'refsheet.b12', touch=True); await B.record('P01', touch=True)
            check(await ev("S47.ch1.s.ref"), f'{name}: two taps and RECORD match B-12')
            hyp = 'light' if name == 'landscape' else 'drift'
            await B.thread_visible(f'hyp.{hyp}', 'refsheet.lamp' if hyp == 'light' else 'refsheet.drive', touch=True)
            await B.record('P02', touch=True)
            check(await ev(f"S47.ch1.s.hyp === '{hyp}' && S47.ch1.s.stage === 'choose-control'"), f'{name}: a hypothesis is recorded by touch')
            await ev(f"""const s = S47.ch1.s; s.method = '{'passive' if hyp == 'light' else 'active'}'; s.observed = true;
              s.dev2 = 3; s.stage = 'compare-exposures'; s.mark2 = [0.5, 0.5];
              s.f2 = {{ id: 'frame02', subject: '', method: s.method, clock: 0, url: '{GREY}', targets: {{}} }};
              S47.ch1.applyWorld(); S47.game.board.refresh()""")
            await B.thread_visible('frame01.extra', 'frame02.extra', touch=True)
            await B.thread_visible('observation.stripe', 'frame02.detail', touch=True)
            await B.record('P03', touch=True); await tick(0.1)
            check(await ev("S47.ch1.s.concluded && S47.ch1.s.stage === 'file-report'"), f'{name}: the two-exposure finding is recorded by touch')
            check(await ev("document.querySelector('.bq[data-q=P03]').getBoundingClientRect().width <= document.querySelector('[data-view]').clientWidth"), f'{name}: the two-exposure question fits the phone width at readable scale')
            await B.reveal('.bq[data-q=P03]')
            await pg.screenshot(path=f'{OUT}/b00_ch1_finding_phone_{name}.png')
            await ev("S47.game.board.view.look('ch1')")
            await pg.screenshot(path=f'{OUT}/b00_ch1_phone_{name}.png')
            await B.close()
            await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter2'); S47.tick(1.0)")
            await ev("S47.ch2.s.read = { original: true, amended: true, lineage: true, index: true }; S47.ch2.applyWorld(); S47.tick(0.1)")
            await ev("S47.game.openNotebook()")
            await pg.tap('.notebook .nb-boardtab'); await tick(0.1)
            v = await B.view()
            check(await B.has('.board-overlay') and v['s'] >= 0.79, f'{name}: the board opens from the notebook at a readable size (scale {v["s"]:.2f})')
            fits = await ev("(() => { const r = document.querySelector('.board-wrap').getBoundingClientRect(); return r.right <= innerWidth + 1 && r.bottom <= innerHeight + 1 && document.documentElement.scrollWidth <= innerWidth; })()")
            check(fits, f'{name}: the board fits the screen, nothing to scroll sideways')
            lh = await ev("document.querySelector('[data-node=\"e07.abc\"]').getBoundingClientRect().height")
            check(lh >= 34, f'{name}: a line to tap is {lh:.0f} px high')
            await pg.screenshot(path=f'{OUT}/b06_phone_{name}.png')
            # two taps tie a thread; the board is moved to bring the second line into view
            await ev("S47.game.board.view.show('e07.abc')")
            x, y = await B.centre('[data-node="e07.abc"]'); await pg.touchscreen.tap(x, y)
            check('goes with' in await B.say(), f'{name}: a tap chooses one end')
            await ev("S47.game.board.view.show('e06.omit')")
            x, y = await B.centre('[data-node="e06.omit"]'); await pg.touchscreen.tap(x, y)
            r = await B.say()
            check('leaves it out' in r and await ev("document.querySelectorAll('.bq[data-q=P04] .bq-slots li.on').length") == 1, f'{name}: a second tap ties the thread, and the note goes into P04')
            # a finger on the cork moves the view
            empty = await ev("""(() => { const v = document.querySelector('[data-view]').getBoundingClientRect();
              for (let y = v.top + 10; y < v.bottom - 10; y += 8) for (let x = v.left + 10; x < v.right - 10; x += 8) {
                const e = document.elementFromPoint(x, y); if (e && e.classList.contains('board')) return [x, y]; } return null; })()""")
            t0 = await B.view()
            await touch([(empty[0] - 60 * i / 8, empty[1] - 40 * i / 8) for i in range(9)])
            t1 = await B.view()
            check(abs(t1['tx'] - t0['tx']) + abs(t1['ty'] - t0['ty']) > 20, f'{name}: a finger on the cork moves the view')
            # a card dragged with a finger
            sel = '.bcard[data-card=s03log] .bcard-head span'
            await B.reveal(sel)
            x, y = await B.centre(sel)
            under = await ev(f"(() => {{ const e = document.elementFromPoint({x}, {y}); return e ? e.className + ' in ' + ((e.closest('[data-id]') || {{dataset: {{}}}}).dataset.id) : null; }})()")
            l0 = await ev("parseFloat(document.querySelector('.bcard[data-card=s03log]').style.top)")
            await touch([(x, y + 40 * i / 8) for i in range(9)])
            l1 = await ev("parseFloat(document.querySelector('.bcard[data-card=s03log]').style.top)")
            check(l1 - l0 > 30, f'{name}: a card follows a finger ({l1 - l0:.0f} board px; under the finger: {under})')
            # two fingers pinch the board smaller
            await ev("S47.game.board.view.look('ns')")
            await ev('new Promise(resolve => requestAnimationFrame(resolve))')
            s0 = (await B.view())['s']
            # Buttons handle their own touches. Start both fingers on the board, clear of
            # TEXT / RECORD / Hint. Touch hit testing can favour a nearby button even if
            # elementFromPoint hits the card, so use bare cork with a margin around it.
            vr = await ev("""(() => {
              const r = document.querySelector('[data-view]').getBoundingClientRect();
              const cx = r.left + r.width / 2, cy = r.top + r.height / 2;
              const hit = (x, y) => { const e = document.elementFromPoint(x, y), b = e?.closest('button');
                const f = e?.closest('.bcard-full');
                const cork = !!e && (e.classList.contains('board') || e.hasAttribute('data-view'));
                const clear = [-32, 0, 32].every(dx => [-32, 0, 32].every(dy =>
                  !document.elementFromPoint(x + dx, y + dy)?.closest('button')));
                return { tag: e?.tagName, button: b?.textContent, card: e?.closest('[data-card]')?.getAttribute('data-card'),
                  ok: cork && clear && !!e?.closest('[data-view]') && !b && !(f && f.scrollHeight > f.clientHeight + 2) }; };
              const old = [hit(cx - 120, cy), hit(cx + 120, cy + 10)];
              for (let d = 0; d < r.height / 2 - 24; d += 12) for (const sign of [1, -1]) {
                const y = cy + d * sign;
                const chosen = [hit(cx - 120, y), hit(cx + 120, y + 10)];
                if (chosen.every(p => p.ok)) return { at: [cx, y], old, chosen };
              }
              return { at: null, old };
            })()""")
            print(f'{name} pinch hit test: {json.dumps(vr)}', flush=True)
            check(vr['at'] is not None, f'{name}: both pinch fingers start on the board, clear of buttons')
            if vr['at'] is None: raise AssertionError('No board surface for a two-finger pinch')
            vx, vy = vr['at']
            await ev("""(() => { const v=document.querySelector('[data-view]'); window.pinchTargets=[];
              window.pinchCapture=e=>{ if(e.pointerType==='touch') pinchTargets.push({tag:e.target.tagName,
                button:e.target.closest('button')?.textContent, cork:e.target.classList.contains('board') || e.target.hasAttribute('data-view')}); };
              v.addEventListener('pointerdown',pinchCapture,true); })()""")
            await touch([(vx - 120 + 80 * i / 8, vy) for i in range(9)], [(vx + 120 - 80 * i / 8, vy + 10) for i in range(9)])
            targets=await ev("(() => { document.querySelector('[data-view]').removeEventListener('pointerdown',pinchCapture,true); return pinchTargets; })()")
            check(len(targets)==2 and all(t.get('cork') and not t.get('button') for t in targets), f'{name}: both native touches land on the cork ({targets})')
            s1 = (await B.view())['s']
            check(s1 < s0 * 0.85, f'{name}: two fingers pinch the board smaller ({s0:.2f} to {s1:.2f})')
            await B.close()
            await ctx.close()

        errs = [e for group in errlists for e in group]
        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        if errs: check(False, 'console clean')
        json.dump(checks, open(f'{OUT}/checks-board.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
