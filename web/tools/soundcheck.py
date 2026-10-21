# The sound over time (Tom, 16 October: the sound keeps dropping out). Listens to the master
# output in real time in four places: the control room, outside in the yard at night, in the
# truck on its pad with the radio on, and driving down the highway with the radio on. For each:
# the level every 100 ms, and the drop-outs (the level falling under a quarter of what it was
# the two seconds before, for 0.3 s or more); and how many sound nodes and how many changes to
# sound parameters the game makes each second and per frame (a leak shows as a number that keeps growing).
# A headless browser cannot hear the glitches a busy machine makes (UNVERIFIED on real
# speakers); this finds the drop-outs the game makes itself, and runaway work for the sound.
# Usage: python3 tools/soundcheck.py OUTDIR [seconds per place]
import asyncio, sys, os, json, statistics
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
SECS = float(sys.argv[2]) if len(sys.argv) > 2 else 40
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
results = []
def check(ok, msg):
    results.append(bool(ok)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

# count every node made and every change asked of a parameter, from the start
COUNT = """(() => {
  const s = window.__snd = { nodes: 0, params: 0, starts: 0, frames: 0 };
  const raf = window.requestAnimationFrame.bind(window);
  window.requestAnimationFrame = (callback) => raf((time) => { s.frames++; callback(time); });
  const P = (window.AudioContext || window.webkitAudioContext).prototype;
  for (const k of Object.getOwnPropertyNames(BaseAudioContext.prototype)) {
    if (!k.startsWith('create')) continue;
    const f = BaseAudioContext.prototype[k];
    if (typeof f !== 'function') continue;
    BaseAudioContext.prototype[k] = function (...a) { s.nodes++; return f.apply(this, a); };
  }
  for (const k of ['setValueAtTime', 'linearRampToValueAtTime', 'exponentialRampToValueAtTime', 'setTargetAtTime', 'setValueCurveAtTime']) {
    const f = AudioParam.prototype[k];
    AudioParam.prototype[k] = function (...a) { s.params++; return f.apply(this, a); };
  }
  const st = AudioScheduledSourceNode.prototype.start;
  AudioScheduledSourceNode.prototype.start = function (...a) { s.starts++; return st.apply(this, a); };
  void P;
})();"""
# the level of the master output every 100 ms, in real time
LISTEN = """(() => {
  const a = S47.audio, ctx = a.ctx;
  const an = ctx.createAnalyser(); an.fftSize = 2048; a.master.connect(an);
  const buf = new Float32Array(an.fftSize);
  window.__lv = [];
  window.__lvTimer = setInterval(() => { an.getFloatTimeDomainData(buf); let s = 0; for (const v of buf) s += v * v; window.__lv.push(Math.sqrt(s / buf.length)); }, 100);
})()"""

def dropouts(lv):
    out, run = [], 0
    for i in range(20, len(lv)):
        before = statistics.median(lv[i - 20:i])
        if before > 0.003 and lv[i] < before * 0.25:
            run += 1
        else:
            if run >= 3: out.append((round((i - run) / 10, 1), run / 10))
            run = 0
    return out

async def main():
    os.makedirs(OUT, exist_ok=True)
    report = {}
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': 960, 'height': 600})).new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };" + COUNT)
        await pg.goto(URL)
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(800)
        ev = pg.evaluate
        print(f"latency: base {await ev('S47.audio.ctx.baseLatency')}, output {await ev('S47.audio.ctx.outputLatency || 0')}", flush=True)
        await ev(LISTEN)

        async def place(name, setup):
            await ev("S47.hold = true"); await ev(setup); await ev("S47.hold = false")
            await pg.wait_for_timeout(1500)
            n0 = await ev("({...window.__snd, t: performance.now()})")
            await ev("window.__lv = []")
            await pg.wait_for_timeout(SECS * 1000)
            lv = await ev("window.__lv.slice()")
            n1 = await ev("({...window.__snd, t: performance.now()})")
            secs = (n1['t'] - n0['t']) / 1000
            rate = {k: round((n1[k] - n0[k]) / secs, 1) for k in ('nodes', 'params', 'starts', 'frames')}
            d = dropouts(lv)
            med = statistics.median(lv) if lv else 0
            per_frame = (n1['params'] - n0['params']) / max(1, n1['frames'] - n0['frames'])
            report[name] = {'median': med, 'p10': sorted(lv)[len(lv) // 10] if lv else 0, 'max': max(lv) if lv else 0, 'dropouts': d, 'per_second': rate, 'params_per_frame': per_frame, 'level': [round(v, 4) for v in lv]}
            print(f"{name}: level median {med:.4f}, p10 {report[name]['p10']:.4f}, max {report[name]['max']:.4f}; drop-outs {len(d)} {d[:6]}; per second {rate}; params/frame {per_frame:.2f}", flush=True)
            return report[name]

        r = await place('control room', "S47.jump('chapter1'); S47.tick(1.0); S47.player.place(0, 1.5, 0); S47.tick(0.2)")
        r = await place('yard at night', "S47.world.enter('saro'); S47.player.place(-20, 30, Math.PI); S47.tick(0.3)")
        await ev("S47.hold = true; S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(60):
            await ev("S47.tick(0.25, 20)")
            if await ev("S47.world.driving"): break
        await pg.keyboard.press('r'); await ev("S47.tick(0.5, 20)")
        r = await place('truck, radio on, standing', "S47.tick(0.1)")
        r = await place('driving, radio on', "S47.world.autopilot = 13")
        await ev("clearInterval(window.__lvTimer)")

        # the drop-outs a player would hear, and whether the work for the sound keeps growing
        bad = {k: v['dropouts'] for k, v in report.items() if len(v['dropouts']) > max(1, SECS / 30)}
        check(not bad, f'no more than one drop-out in {SECS / 30:.0f} half-minutes anywhere ({bad})')
        nodes = {k: v['per_second']['nodes'] for k, v in report.items()}
        check(all(v < 60 for v in nodes.values()), f'sound nodes made per second stay modest ({nodes})')
        # Listener (9), engine (8), tyres (5) and receiver (4) already make 26
        # calls/frame. The old 1500/s budget failed even with the recorded host
        # disabled at 60 fps. tools/artbell-cost.py measured On/Off/On at
        # 29.23/29.50/29.50 calls/frame with no resync or re-decode. Normalize to
        # actual rendered frames; 36 allows about 20% above that measured base.
        params = {k: round(v['params_per_frame'], 2) for k, v in report.items()}
        check(all(v['per_second']['frames'] > 0 and v['params_per_frame'] < 36 for v in report.values()),
              f'changes to sound parameters stay below 36 per rendered frame ({params})')
        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump(report, open(f'{OUT}/soundcheck.json', 'w'), indent=1)
        print(f'{sum(results)} of {len(results)} PASS')
        await b.close()
        sys.exit(0 if all(results) else 1)

asyncio.run(main())
