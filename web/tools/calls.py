# Out in the dark (core/calls.ts, Tom 16 October): on foot outside at night with the flashlight
# on, something sounds now and then: the night goes quiet first, the flashlight stutters, and the
# call comes from far off, behind or to one side, never where the beam is. Not with the light
# off, not indoors, not in the truck. Checks the sounds are loaded, which are played, where, and
# that the light and the night come back.
# Usage: python3 tools/calls.py OUTDIR [WxH]
import asyncio, sys, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
results = []
def check(ok, msg):
    results.append(bool(ok)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

ALLOWED = {'alienVoice', 'alienRadio', 'alienCalls', 'alienSong', 'alienTemple', 'alienHorn'}

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
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        ev = pg.evaluate
        async def tick(s, fps=30): await ev(f"S47.tick({s}, {fps})")
        LAST = "(() => { const l = S47.calls.last; return l && { name: l.name, dist: l.dist, off: l.off }; })()"

        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter1'); S47.tick(1.0)")
        names = await ev("Object.keys(S47.game.d.audio.buf).filter((k) => k.startsWith('alien'))")
        check(set(names) == ALLOWED, f"the six sounds are loaded ({sorted(names)})")
        # into the truck for the flashlight on the passenger seat, and out again in the yard
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(80):
            await tick(0.25)
            if await ev("S47.world.driving"): break
        n0 = await ev("S47.calls.count")
        await ev("S47.calls.now()"); await tick(3)
        check(await ev("S47.calls.count") == n0, 'in the truck: nothing')
        await ev("S47.world.getOutHere()"); await tick(0.5)
        check(not await ev("S47.world.driving") and await ev("S47.flashlight.have"), 'out in the yard with the flashlight')
        # the light off: nothing
        await ev("S47.flashlight.on = false; S47.calls.now()"); await tick(4)
        check(await ev("S47.calls.count") == n0, 'the flashlight off: nothing comes')
        # the light on: the quiet, the stutter, then the call, out of the beam and far
        await ev("window.__hush = []; const n = S47.audio.night, o = n.hush.bind(n); n.hush = (s) => { window.__hush.push(s); return o(s); }; 0")
        await ev("S47.flashlight.on = true; S47.calls.now()"); await tick(0.9)
        hushed = await ev("window.__hush")
        flick = await ev("S47.flashlight['flick']")
        check(10 in hushed and flick > 0.3 and await ev("S47.calls.count") == n0, f"first the night goes quiet ({hushed}) and the light stutters ({flick:.2f}), before anything sounds")
        await tick(1.5)
        l = await ev(LAST)
        check(await ev("S47.calls.count") == n0 + 1 and l and l['name'] in ALLOWED, f"then a sound out in the dark ({l})")
        check(l and l['off'] >= 1.55 and 34 <= l['dist'] <= 141, f"behind or off to the side, never in the beam, and far ({l and round(l['off'], 2)} rad, {l and round(l['dist'])} m)")
        await pg.screenshot(path=f'{OUT}/call.png')
        await tick(14)
        check(await ev("S47.flashlight['flick']") == 0, 'afterwards the light is steady again')
        # the quiet itself, in real time (an audio parameter does not follow the game's ticks)
        await ev("S47.audio.night.hush(1)"); await pg.wait_for_timeout(900)
        low = await ev("S47.audio.night['quiet'].gain.value")
        await pg.wait_for_timeout(6000)
        back = await ev("S47.audio.night['quiet'].gain.value")
        check(low < 0.4 and back > 0.85, f"the quiet: the night drops away ({low:.2f}) and comes back ({back:.2f})")
        nxt = await ev("S47.calls.next")
        check(80 <= nxt <= 225, f"and the next is minutes away ({nxt:.0f} s)")
        # many in a row: only the allowed sounds, not the same twice running
        seen, prev, twice = [], None, False
        for _ in range(16):
            await ev("S47.calls.now()"); await tick(2.2)
            n = (await ev(LAST))['name']
            if n == prev: twice = True
            prev = n; seen.append(n)
        check(set(seen) <= ALLOWED and len(set(seen)) >= 3 and not twice, f"sixteen more: {len(set(seen))} different, never the same twice running")
        # indoors: nothing
        await ev("(() => { const b = S47.room.bounds; S47.player.place((b.minX + b.maxX) / 2, (b.minZ + b.maxZ) / 2, 0); })()"); await tick(1)
        n1 = await ev("S47.calls.count")
        await ev("S47.calls.now()"); await tick(4)
        sp = await ev("S47.audio['space']")
        check(sp != 'yard' and await ev("S47.calls.count") == n1, f"indoors ({sp}): nothing")
        bad = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not bad, 'no console errors or warnings' + ('' if not bad else ': ' + '; '.join(bad[:4])))
        print(f'{sum(results)} of {len(results)} PASS')
        await b.close()
        sys.exit(0 if all(results) else 1)

asyncio.run(main())
