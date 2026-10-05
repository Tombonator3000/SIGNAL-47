# The music under the night (core/score.ts), on the Pages build served over HTTP (the
# single-file build has no music files beside it and plays none; that is checked too).
# The title screen's music on the first click, and gone when the night starts. A cue chosen by place (the yard, indoors, the truck), streamed from music/, playing, held
# back by a scene, and every cue file there with no 404.
# Usage: S47_URL=http://127.0.0.1:PORT/SIGNAL-47/ python3 tools/music.py
#        python3 tools/music.py            (the single-file build: no music, no requests)
import asyncio, sys, os
from playwright.async_api import async_playwright

URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
SINGLE = not os.environ.get('S47_URL')
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width': 960, 'height': 600}); pg.set_default_timeout(240000)
        errs, music = [], []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        pg.on('response', lambda r: music.append((r.url.split('/')[-1], r.status)) if '/music/' in r.url else None)
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL); await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        ev = pg.evaluate
        # the title screen: its music comes in with the first click
        check(not await ev("S47.audio['loops'].has('music')"), 'the title screen is quiet before anything is clicked')
        await pg.mouse.click(8, 8)
        await asyncio.sleep(1.5)
        check(await ev("S47.audio['loops'].has('music') && S47.audio.ctx.state === 'running'"), 'the first click brings in the title music')
        await pg.click('button[data-a=start]')
        await pg.wait_for_function("S47.started()")
        check(not await ev("S47.audio['loops'].has('music')"), 'and it stops when the night starts')
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter1'); S47.tick(1)")
        check(await ev("!!S47.score()"), 'the music starts with the night')
        await ev("S47.player.place(12, 6, 0); S47.tick(0.5)")
        if SINGLE:
            await ev("S47.score()['rest'] = 0; S47.tick(1)")
            await asyncio.sleep(2)
            check(await ev("S47.score().playing") is None and not music, 'the single-file build plays no cues and asks for no files')
        else:
            # the quiet before the first cue is over: the yard's cue comes in by itself
            await ev("S47.score()['rest'] = 0; S47.tick(0.5)")
            cue = await ev("S47.score().playing")
            check(cue == 'night', f'out in the yard at night the cue is the night ({cue})')
            await asyncio.sleep(4)
            st = await ev("(() => { const el = S47.score()['el']; return el ? { t: el.currentTime, err: !!el.error, paused: el.paused } : null; })()")
            check(st is not None and st['t'] > 1 and not st['err'], f'it streams from music/ and plays ({st})')
            # a scene holds the player: the music gets out of the way
            await ev("S47.game.cinematic = true; S47.tick(0.2)")
            await asyncio.sleep(3.2)
            await ev("S47.tick(0.1)")
            check(await ev("S47.score().playing") is None, 'held by a scene, it fades and stops')
            await ev("S47.game.cinematic = false; S47.tick(0.1)")
            # indoors and in the truck
            await ev("S47.score().stop(); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
            await ev("S47.score()['rest'] = 0; S47.score()['last'].clear(); S47.tick(0.5)")
            cue = await ev("S47.score().playing")
            check(cue == 'eerie', f'indoors the cue is quieter and stranger ({cue})')
            await ev("S47.score().stop(); S47.score()['rest'] = 0")
            await ev("S47.world.freeDrive('road'); S47.hold = false")
            await pg.wait_for_function("S47.world.driving")
            await ev("S47.hold = true; S47.score()['rest'] = 0; S47.tick(0.5)")
            cue = await ev("S47.score().playing")
            check(cue == 'drive', f'in the truck on the highway it is the drive ({cue})')
            # every cue file is there
            for c in ['night', 'eerie', 'tension', 'motel', 'drive', 'dawn']:
                await ev(f"S47.score().start('{c}')"); await asyncio.sleep(1.5)
            await ev("S47.score().stop()")
            bad = [m for m in music if m[1] not in (200, 206, 304)]
            names = sorted(set(m[0] for m in music))
            check(len(names) == 6 and not bad, f'all six cue files answered ({len(names)} files, {len(bad)} bad)')
        bad = [e for e in errs if 'GPU stall' not in e and 'Automatic fallback to software WebGL' not in e]
        print('\n'.join(bad) if bad else 'no console errors/warnings')
        await b.close()
    n = sum(1 for c in checks if c[0] == 'PASS')
    print(f'{n} of {len(checks)} PASS')
    sys.exit(0 if n == len(checks) and not bad else 1)

asyncio.run(main())
