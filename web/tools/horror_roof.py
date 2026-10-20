# Light steps on the records room roof (story/roofSteps.ts; FORSLAG.md, 20 October, Valg for Tom,
# the extra choice): once in chapter two, at a quiet moment in the records room, light steps closer
# together than anyone walks come across the annex roof and stop right over the player; a long beat
# with the room's hum gone; once more, further off, out to the lip at a corner; the hum back. Then
# frost on the roof unit (and the lip at that corner), seen from outside with a closer look.
#   - not in chapter one, not before Ward's call, not from the corridor, not with a paper open
#   - comes by itself after about 20 s settled in the room (a paper read, standing still)
#   - the steps: across from more than 3.5 m off, the walking ones under 0.25 s apart, the last right
#     over the player (and it follows the player while it comes), nothing in the beat (10 s and more),
#     the second burst further off and ending on the lip, the hum held down through the beat and
#     back after, the tired tube steady and then stuttering, two captions, one note
#   - heard from above: an HRTF panner on the roof deck (3.22 m), the overhead step about as loud as
#     the player's own and the first ones well under it (measured in the game's nodes)
#   - the bought excerpt's eight slots each hold a whole hit (sfx/roof_steps.mp3), or Kenney's steps
#     stand in when it is not there (phone run: the bought buffer removed)
#   - saved: the save holds roof:steps and the corner; after Continue the frost is there, the steps
#     never come again, the note is not doubled
#   - from outside: the frost on the unit and on the lip, a closer look says so and writes it down once
#   - seen with the sound down: a few motes of dust out of the ceiling under the steps that come over
#     the room, while they come; the captions keep time (one while they come, one when they stop)
#   - the call to the key holder in the middle of them: they wait for the call, then finish, and the
#     chapter waits for them (with Missing time off: it is not an abduction)
#   - desktop 1280x720 and phone 412x892 with touch; no console errors or warnings
# Usage: python3 tools/horror_roof.py OUTDIR
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
IDB_GET = """((store, key) => new Promise((ok) => { const r = indexedDB.open('s47', 2);
  r.onsuccess = () => { const g = r.result.transaction(store).objectStore(store).get(key);
    g.onsuccess = () => { ok(g.result ?? null); r.result.close(); }; g.onerror = () => ok(null); }; r.onerror = () => ok(null); }))"""
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)
db = lambda x: 20 * math.log10(max(x, 1e-9))

HELPERS = """(() => {
  const V = S47.three.Vector3;
  window.T = {
    // stand at (x, z) and look at a point (the eye is the camera's after a tick)
    aim(x, z, tx, ty, tz) {
      const pl = S47.player; pl.place(x, z, 0); S47.tick(0.05);
      const c = S47.camera.position;
      pl.place(x, z, Math.atan2(-(tx - c.x), -(tz - c.z)));
      pl.pitch = Math.atan2(ty - c.y, Math.hypot(tx - c.x, tz - c.z)); S47.tick(0.05);
      return pl.walkable(x, z);
    },
    roof: () => ({ phase: S47.roofSteps.phase, corner: S47.roofSteps.corner, bought: S47.roofSteps.bought, log: S47.roofSteps.log.slice(),
      held: S47.audio.roomToneHeld, frost: S47.annex.frost.group.visible, at: S47.annex.frostAt,
      sw: S47.annex.frost.lips.sw.visible, se: S47.annex.frost.lips.se.visible }),
    toasts: [],
    hook() {
      const ui = S47.game.d.ui; if (ui.__hooked) return; ui.__hooked = true;
      const o = ui.toast.bind(ui); ui.toast = (t, s, c) => { T.toasts.push(t); return o(t, s, c); };
    },
    // the eight slots of the bought excerpt: where each hit is, and nothing outside 0.28 s of a slot
    slots() {
      const b = S47.audio.buf['lic:roof_steps']; if (!b) return null;
      const d = b.getChannelData(0), sr = b.sampleRate, out = [];
      for (let i = 0; i < 8; i++) {
        let pk = 0, at = 0, tail = 0, past = 0;
        const s0 = Math.round(i * 0.3 * sr), s1 = Math.round((i * 0.3 + 0.28) * sr), s2 = Math.min(d.length, Math.round((i + 1) * 0.3 * sr));
        for (let k = s0; k < s1; k++) { const v = Math.abs(d[k]); if (v > pk) { pk = v; at = (k - s0) / sr; } }
        for (let k = s1 - Math.round(0.03 * sr); k < s1; k++) tail = Math.max(tail, Math.abs(d[k]));
        for (let k = s1; k < s2; k++) past = Math.max(past, Math.abs(d[k]));
        out.push({ pk, at, tail, past });
      }
      return { dur: b.duration, slots: out };
    },
  };
  // a tap with a rough K-weighting into an analyser (as tools/licensed_sfx.py), and the loudest
  // 50 ms reading of what one call plays on the sfx bus
  const a = S47.audio, ctx = a.ctx;
  T.peak = async (fn, secs = 0.6) => {
    const inp = ctx.createGain(), hp = ctx.createBiquadFilter(), sh = ctx.createBiquadFilter(), an = ctx.createAnalyser(), z = ctx.createGain();
    hp.type = 'highpass'; hp.frequency.value = 38; sh.type = 'highshelf'; sh.frequency.value = 1700; sh.gain.value = 4;
    an.fftSize = 2048; z.gain.value = 0; inp.connect(hp); hp.connect(sh); sh.connect(an); an.connect(z); z.connect(ctx.destination);
    const keep = a.sfx; a.sfx = inp; try { fn(); } finally { a.sfx = keep; }
    const buf = new Float32Array(an.fftSize); let best = 0; const t0 = performance.now();
    while (performance.now() - t0 < secs * 1000) {
      await new Promise((r) => setTimeout(r, 15));
      an.getFloatTimeDomainData(buf); let s = 0; for (const v of buf) s += v * v; best = Math.max(best, Math.sqrt(s / buf.length));
    }
    inp.disconnect(); return best;
  };
  // the room's hum and the night through the walls (the ambience bus), median of 50 ms readings
  T.bed = async (secs = 1.5) => {
    const inp = ctx.createGain(), hp = ctx.createBiquadFilter(), sh = ctx.createBiquadFilter(), an = ctx.createAnalyser(), z = ctx.createGain();
    hp.type = 'highpass'; hp.frequency.value = 38; sh.type = 'highshelf'; sh.frequency.value = 1700; sh.gain.value = 4;
    an.fftSize = 2048; z.gain.value = 0; inp.connect(hp); hp.connect(sh); sh.connect(an); an.connect(z); z.connect(ctx.destination);
    a.amb.connect(inp);
    const buf = new Float32Array(an.fftSize), l = []; const t0 = performance.now();
    while (performance.now() - t0 < secs * 1000) {
      await new Promise((r) => setTimeout(r, 15));
      an.getFloatTimeDomainData(buf); let s = 0; for (const v of buf) s += v * v; l.push(Math.sqrt(s / buf.length));
    }
    a.amb.disconnect(inp); inp.disconnect(); l.sort((x, y) => x - y); return l[l.length >> 1];
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
        roof = lambda: ev("T.roof()")
        s2 = lambda k: ev(f"S47.ch2.s.{k}")
        notes = lambda: ev("S47.game.notes.slice()")
        await ev("S47.hold = true")
        # the night's other events stay out of the way (their own tests cover them)
        await ev("S47.nightEvents['wait'] = 1e9")

        # ---------- not in chapter one ----------
        await ev("S47.jump('chapter1'); S47.tick(1.0); S47.nightEvents['wait'] = 1e9")
        await ev("T.aim(-3.65, 7.75, -3.65, 0.8, 8.6)")
        await tick(1, 40)
        r = await roof()
        check(await ev("S47.game.phase") == 'ch1' and r['phase'] == 'idle' and not r['log'] and not r['frost'], 'chapter one: 40 s standing still in the records room, nothing on the roof')

        # ---------- chapter two, before Ward's call ----------
        await ev("S47.jump('chapter2'); S47.tick(1.5); S47.nightEvents['wait'] = 1e9")
        check(await ev("S47.game.phase") == 'ch2' and await s2('stage') == 'ward-call', 'chapter two begins with the supervisor line')
        await ev("T.aim(-3.65, 7.75, -3.65, 0.8, 8.6)")
        await tick(1, 30)
        check((await roof())['phase'] == 'idle', 'not before Ward has called: 30 s in the records room, nothing')
        await ev("S47.game.d.inter.get('phone').use()")
        await tick(1, 12)
        check(await s2('stage') == 'records', 'Ward called; the records are next')

        # ---------- the unit from outside, before ----------
        await ev("T.aim(-8.5, 20.0, -4.2, 3.85, 8.6)"); await tick(0.6)
        await ev("S47.game.d.ui.clearToasts()"); await pg.wait_for_timeout(400); await tick(0.1)
        await shot('desk_00_unit_before')
        await ev("S47.input.zoomHeld = true"); await tick(0.8)
        hud0 = await ev("(() => { const e = document.querySelector('.hud .examine'); return e && e.classList.contains('show') ? e.textContent : null; })()")
        check(hud0 and 'louvres furred with dust' in hud0 and not (await roof())['frost'], f'before: the unit on the roof is only dusty ({hud0})')
        await ev("S47.input.zoomHeld = false"); await tick(0.5)

        # ---------- not from the corridor ----------
        await ev("T.aim(-3.0, 5.7, -3.0, 2.0, 7.5)")
        await tick(1, 40)
        check((await roof())['phase'] == 'idle', 'standing in the south corridor for 40 s: nothing (only in the records room)')

        # ---------- not while a paper is open ----------
        await ev("T.aim(-2.05, 7.95, -1.3, 0.8, 7.95)")
        await ev("S47.game.d.inter.get('lineage').use()"); await tick(0.1)
        check(await ev("!!document.querySelector('.docview')"), 'the B-12 lineage card is open')
        await tick(1, 40)
        check((await roof())['phase'] == 'idle', 'with the card open for 40 s in the records room: nothing')
        await pg.click('.docview [data-a=back]'); await tick(0.1)

        # ---------- by itself, at the first quiet moment ----------
        await ev("T.aim(-3.65, 7.75, -3.65, 0.8, 8.6)")
        waited = 0.0
        while waited < 40 and (await roof())['phase'] == 'idle':
            await tick(0.5); waited += 0.5
        r = await roof()
        # (about 20 s of quiet in the room counts: a second or two of it came after Ward hung up)
        check(r['phase'] == 'come' and 16 <= waited <= 23, f'the steps begin by themselves after {waited:.1f} s more in the room with the card read and put back')
        # follow them in: a step back towards the door while they come (they go where the player goes)
        await tick(1.2)
        await ev("S47.player.place(-3.3, 7.45, S47.player.yaw); S47.tick(0.05)")
        motes, told = 0, False
        for _ in range(200):
            if (await roof())['phase'] != 'come': break
            motes = max(motes, await ev("S47.annex.dustLive"))
            j = ' '.join(await ev("T.toasts"))
            told = told or ('Footsteps on the roof' in j and 'right over you' not in j)
            await tick(0.1)
        check(motes >= 6, f'while they come, dust shakes out of the ceiling under them ({motes} motes in the air at most)')
        check(told, 'captioned while they come, before they stop')
        await tick(0.1)
        r = await roof()
        b1 = [l for l in r['log'] if l['burst'] == 1]
        gaps = [b1[i + 1]['t'] - b1[i]['t'] for i in range(len(b1) - 1)]
        walking = [g for g in gaps[:-3] if g < 0.5]
        check(r['phase'] == 'still' and len(b1) >= 12, f'they come across and stop: {len(b1)} steps')
        check(b1[0]['d'] > 3.5 and b1[-1]['d'] < 0.65, f'from {b1[0]["d"]:.1f} m off across the roof to right over the player ({b1[-1]["d"]:.2f} m)')
        check(len(walking) >= 8 and max(walking) < 0.25 and min(walking) > 0.12, f'closer together than anyone walks: {len(walking)} gaps of {min(walking):.3f} to {max(walking):.3f} s')
        check(any(g > 0.6 for g in gaps[:8]) and gaps[-3:] == sorted(gaps[-3:]), 'one hesitation early on, and the last steps slow down as they settle')
        check(math.hypot(b1[-1]['x'] + 3.3, b1[-1]['z'] - 7.45) < 0.65 and math.hypot(b1[-1]['x'] + 3.65, b1[-1]['z'] - 7.75) > 0.2, f'and it is over where the player went, not where they stood ({b1[-1]["x"]:.2f}, {b1[-1]["z"]:.2f})')
        check(r['frost'] and r['at'] in ('sw', 'se') and (r['sw'] if r['at'] == 'sw' else r['se']), f'the frost is on the roof now (corner {r["at"]})')
        check(await ev("!!S47.annex['dust'] && S47.annex['dust'].pts.visible && S47.annex.interior.visible"), 'a trickle of dust comes down from the ceiling where it stopped')
        # the moment: looking up at the ceiling, the caption on screen, the hum gone
        await ev("S47.player.pitch = 0.95; S47.player.yaw = Math.PI * 0.85; S47.tick(0.05)")
        await tick(1.2)
        await pg.wait_for_timeout(900)
        r = await roof()
        cap = ' '.join(await ev("T.toasts"))
        check(r['phase'] == 'still' and r['held'] < 0.05, f'the room\'s hum has dropped out (held at {r["held"]:.3f})')
        check(await ev("S47.annex['tubeHold']") == 'steady', 'the tired tube holds steady, no flicker, while it stands there')
        check('Footsteps on the roof' in cap and 'right over you' in cap, 'captioned: the steps on the roof, stopped right over you')
        await shot('desk_01_overhead')
        await ev("S47.player.pitch = 0.05; S47.tick(0.05)"); await tick(0.6)
        await shot('desk_01b_ahead')
        # the beat: nothing
        n0 = len(r['log'])
        for _ in range(30):
            await tick(0.5)
            if len((await roof())['log']) > n0: break
        r = await roof()
        b2 = [l for l in r['log'] if l['burst'] == 2]
        beat = b2[0]['t'] - b1[-1]['t'] if b2 else 0
        check(beat >= 10.0, f'nothing for a long beat: {beat:.1f} s between the last step and the next')
        for _ in range(60):
            if (await roof())['phase'] in ('after', 'done'): break
            await tick(0.1)
        r = await roof()
        b2 = [l for l in r['log'] if l['burst'] == 2]
        corner = {'sw': (-6.12, 10.62), 'se': (2.12, 10.62)}[r['at']]
        check(5 <= len(b2) <= 8 and min(l['d'] for l in b2) >= 1.5 and b2[-1]['d'] > b2[0]['d'], f'once more, further off: {len(b2)} steps from {b2[0]["d"]:.1f} to {b2[-1]["d"]:.1f} m away')
        check(math.hypot(b2[-1]['x'] - corner[0], b2[-1]['z'] - corner[1]) < 0.05, 'the last one is on the lip of the roof at the corner with the frost')
        g2 = [b2[i + 1]['t'] - b2[i]['t'] for i in range(len(b2) - 1)]
        check(max(g2) < 0.2, f'quicker as it goes: gaps {min(g2):.3f} to {max(g2):.3f} s')
        await pg.wait_for_timeout(600)
        check((await roof())['held'] < 0.05, 'the hum is still gone while it goes off the roof')
        await shot('desk_02_further')
        for _ in range(60):
            if (await roof())['phase'] == 'done': break
            await tick(0.1)
        await pg.wait_for_timeout(1500); await tick(0.1)
        r = await roof()
        cap = ' '.join(await ev("T.toasts"))
        roofNotes = [n for n in await notes() if 'Steps on the roof' in n]
        check(r['phase'] == 'done' and r['held'] > 0.85, f'the hum comes back afterwards ({r["held"]:.2f})')
        check('further off' in cap and 'Then nothing' in cap, 'captioned: again, further off, then nothing')
        check(len(roofNotes) == 1 and 'right over me' in roofNotes[0] and roofNotes[0][:2] in ('02', '03'), f'one note in the player\'s words ({roofNotes[0][:40] if roofNotes else None}...)')
        src = 'the bought excerpt' if r['bought'] else "Kenney's gravel steps (no bought folder in this build)"
        check(r['bought'] == await ev("!!S47.audio.buf['lic:roof_steps']"), f'the steps played from {src}')
        sl = await ev("T.slots()")
        if sl:
            ok = abs(sl['dur'] - 2.4) < 0.08 and all(s['pk'] > 0.3 and s['at'] < 0.12 and s['tail'] < 0.05 * s['pk'] and s['past'] < 0.02 for s in sl['slots'])
            check(ok, f'the bought excerpt: {sl["dur"]:.2f} s, each of 8 slots holds one whole hit (peaks {min(s["pk"] for s in sl["slots"]):.2f} to {max(s["pk"] for s in sl["slots"]):.2f}, at {max(s["at"] for s in sl["slots"]) * 1000:.0f} ms at the latest)')

        # ---------- how it sounds: from above, against the player's own step ----------
        await ev("T.aim(-3.65, 7.75, -3.65, 0.8, 8.6)"); await tick(0.2)
        pan = await ev("""(() => { const a = S47.audio, keep = a.ctx.createPanner; let m = null;
          a.ctx.createPanner = function () { const p = keep.call(this); m = p; return p; };
          try { S47.roofSteps['play']({ at: 0, x: -3.65, z: 7.75, slot: 0, gain: 1, burst: 1 }, 0); } finally { a.ctx.createPanner = keep; }
          return m ? { model: m.panningModel, y: m.positionY.value } : null; })()""")
        check(pan and pan['model'] == 'HRTF' and abs(pan['y'] - 3.22) < 0.01, f'placed on the roof deck with the HRTF panner ({pan})')
        pan2 = await ev("""(() => { const a = S47.audio, keep = a.ctx.createPanner, made = []; let osc = 0; const ko = a.ctx.createOscillator;
          a.ctx.createPanner = function () { const p = keep.call(this); made.push(p); return p; };
          a.ctx.createOscillator = function () { osc++; return ko.call(this); };
          try { S47.roofSteps['play']({ at: 0, x: -3.9, z: 8.6, slot: 1, gain: 1, burst: 1 }, 0); } finally { a.ctx.createPanner = keep; a.ctx.createOscillator = ko; }
          return { y: made[0].positionY.value, osc }; })()""")
        check(abs(pan2['y'] - 4.06) < 0.01 and pan2['osc'] == 4, f'over the table it is up on the roof unit\'s top, and the sheet metal rings ({pan2})')
        await pg.wait_for_timeout(500)
        own, over, far = [], [], []
        for k in range(3):
            own.append(await ev("T.peak(() => S47.audio.step('tile'))"))
            over.append(await ev(f"T.peak(() => S47.roofSteps['play']({{ at: 0, x: -3.65, z: 7.75, slot: {k}, gain: 1, burst: 1 }}, 0))"))
            far.append(await ev(f"T.peak(() => S47.roofSteps['play']({{ at: 0, x: 1.6, z: 5.6, slot: {k + 3}, gain: 0.88, burst: 1 }}, 0))"))
        # and Kenney's, standing in when the bought file is not there
        kn = []
        for k in range(3):
            kn.append(await ev(f"""(() => {{ const a = S47.audio, b = a.buf['lic:roof_steps']; delete a.buf['lic:roof_steps'];
              const p = T.peak(() => S47.roofSteps['play']({{ at: 0, x: -3.65, z: 7.75, slot: {k}, gain: 1, burst: 1 }}, 0));
              if (b) a.buf['lic:roof_steps'] = b; return p; }})()"""))
        bed = db(await ev("T.bed()"))
        o, v, f, kv = db(sum(own) / 3), db(sum(over) / 3), db(sum(far) / 3), db(sum(kn) / 3)
        print(f'levels: room hum {bed:.1f} dB, own step {o:.1f} dB, roof overhead {v:.1f} dB, roof at the far end {f:.1f} dB, Kenney\'s overhead {kv:.1f} dB')
        check(3.0 <= v - o <= 9.0, f'right overhead a little over the player\'s own step ({v - o:+.1f} dB)')
        check(v - bed >= 3.0, f'and over the room\'s hum before it is held down ({v - bed:+.1f} dB)')
        check(7.0 <= v - f <= 14.0, f'the first ones, across the roof, well under that ({f - v:+.1f} dB)')
        check(abs(kv - v) <= 2.5, f'Kenney\'s steps, standing in, at the same level ({kv - v:+.1f} dB)')

        # ---------- saved ----------
        await tick(0.2)
        meta = None
        for _ in range(60):
            meta = await ev("S47.saves.list(S47.caseId())[0] || null")
            rec = await ev(IDB_GET + f"('saves', '{meta['id']}')") if meta else None
            if rec and 'roof:steps' in rec['state']['town']: break
            await pg.wait_for_timeout(300); await tick(0.05)
        check(rec and 'roof:steps' in rec['state']['town'] and f"roof:{r['at']}" in rec['state']['town'], f'autosaved with the steps and the corner ({meta["place"] if meta else None})')
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev(HELPERS); await ev("T.hook(); S47.hold = true; S47.tick(0.5); S47.nightEvents['wait'] = 1e9")
        r2 = await roof()
        check(r2['phase'] == 'done' and r2['frost'] and r2['at'] == r['at'], 'after Continue: the frost is there and the steps are done')
        await ev("T.aim(-3.65, 7.75, -3.65, 0.8, 8.6)")
        await tick(1, 90)
        r2 = await roof()
        check(not r2['log'] and r2['phase'] == 'done' and len([n for n in await notes() if 'Steps on the roof' in n]) == 1, 'and 90 s standing still in the records room again: they never come back, one note')

        # ---------- the frost from outside ----------
        walk = await ev("T.aim(-8.5, 20.0, -4.2, 3.85, 8.6)")
        await tick(0.6)
        await ev("S47.game.d.ui.clearToasts()"); await pg.wait_for_timeout(400); await tick(0.1)
        where = await ev("S47.placeName()")
        check(walk and await ev("S47.audio['space']") == 'yard', f'outside south-west of the annex ({where})')
        await shot('desk_03_frost_outside')
        await ev("S47.input.zoomHeld = true"); await tick(0.8)
        hud = await ev("(() => { const e = document.querySelector('.hud .examine'); return e && e.classList.contains('show') ? e.textContent : null; })()")
        check(hud and 'white with frost' in hud, f'a closer look: {hud}')
        await shot('desk_04_frost_look')
        frostNotes = [n for n in await notes() if n.split('. ', 1)[-1].startswith('Frost on the unit')]
        check(len(frostNotes) == 1 and ('south-west' if r['at'] == 'sw' else 'south-east') in frostNotes[0], 'written down once')
        lip = {'sw': (-6.3, 3.12, 10.8), 'se': (2.3, 3.12, 10.8)}[r['at']]
        stand = {'sw': (-9.5, 14.5), 'se': (5.2, 14.5)}[r['at']]
        await ev(f"T.aim({stand[0]}, {stand[1]}, {lip[0]}, {lip[1]}, {lip[2]})"); await tick(0.8)
        hud2 = await ev("(() => { const e = document.querySelector('.hud .examine'); return e && e.classList.contains('show') ? e.textContent : null; })()")
        check(hud2 and 'lip of the roof' in hud2, f'and the lip at the corner: {hud2}')
        await shot('desk_05_lip_look')
        await ev("S47.input.zoomHeld = false"); await tick(0.5)
        # nothing said about the roof from inside, looking up through the ceiling
        await ev("T.aim(-3.65, 7.75, -4.2, 3.6, 8.6)"); await ev("S47.input.zoomHeld = true"); await tick(0.8)
        inner = await ev("(() => { const e = S47.game.d.inter.examine(S47.camera); return e ? e.it.id : null; })()")
        check(inner is None or not inner.startswith('look:roof'), f'from inside, the roof is not read through the ceiling ({inner})')
        await ev("S47.input.zoomHeld = false"); await tick(0.3)
        print('\n'.join(errs[:30]) or 'desktop: no console errors/warnings')
        check(not errs, 'desktop: console clean')
        await ctx.close()

        # ======================= the call to the key holder in the middle of them =======================
        ctx, pg, errs = await page(b, 1280, 720, False)
        ev = pg.evaluate
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        roof = lambda: ev("T.roof()")
        s2 = lambda k: ev(f"S47.ch2.s.{k}")
        await ev("S47.hold = true; S47.setAbductions(false); S47.jump('chapter2'); S47.tick(1.5); S47.nightEvents['wait'] = 1e9")
        await ev("S47.game.d.inter.get('phone').use()"); await tick(1, 12)
        # the board done (P04 and P05 recorded): the key holder is next, the room's last quiet moment
        await ev("Object.assign(S47.ch2.s, { p04: true, p05: true, stage: 'call-nora' })")
        await ev("T.aim(-1.75, 9.25, -1.12, 1.42, 9.25)")
        waited = 0.0
        while waited < 12 and (await roof())['phase'] == 'idle':
            await tick(0.25); waited += 0.25
        check((await roof())['phase'] == 'come' and waited <= 5, f'call-nora, Missing time off: the steps begin by themselves {waited:.1f} s into the quiet before the call')
        await tick(0.1, 10)
        await ev("S47.game.d.inter.get('recPhone').use()"); await tick(0.05)
        n0 = len((await roof())['log'])
        held = True
        for _ in range(80):
            if await s2('called'): break
            await tick(0.5)
            if await ev("S47.game.cinematic"): held = held and len((await roof())['log']) <= n0 + 1
        r = await roof()
        check(await s2('called') and held and r['phase'] == 'come', f'they wait while the line is open ({n0} steps before the call, {len(r["log"])} when she hangs up)')
        phase_at_done, card_early = None, False
        for _ in range(400):
            card_early = card_early or (await ev("!!document.querySelector('.chapter-card')") and (await roof())['phase'] != 'done')
            if (await roof())['phase'] == 'done': phase_at_done = await ev("S47.game.phase"); break
            await tick(0.1)
        r = await roof()
        b2 = [l for l in r['log'] if l['burst'] == 2]
        check(r['phase'] == 'done' and len(b2) >= 5 and r['frost'] and phase_at_done == 'ch2' and not card_early, f'after the call they finish (the beat, {len(b2)} further off, the frost), still in chapter two, no chapter card over them')
        for _ in range(80):
            if await ev("!!document.querySelector('.chapter-card')"): break
            await tick(0.25)
        check(await ev("!!document.querySelector('.chapter-card')"), 'then the chapter card')
        for _ in range(80):
            if await ev("S47.game.phase") == 'ch3' and not await ev("S47.game.cinematic"): break
            await tick(0.25)
        check(await ev("S47.game.phase") == 'ch3' and (await roof())['phase'] == 'done', 'and chapter three begins as before')
        print('\n'.join(errs[:30]) or 'call: no console errors/warnings')
        check(not errs, 'call: console clean')
        await ctx.close()

        # ======================= phone, touch (and Kenney's steps standing in) =======================
        ctx, pg, errs = await page(b, 412, 892, True)
        ev = pg.evaluate
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        roof = lambda: ev("T.roof()")
        await ev("S47.hold = true; S47.jump('chapter2'); S47.tick(1.5); S47.nightEvents['wait'] = 1e9")
        check(await ev("S47.input.touchMode"), 'phone: touch controls')
        await ev("S47.game.d.inter.get('phone').use()"); await tick(1, 12)
        await ev("T.aim(-5.05, 8.6, -5.55, 1.28, 8.6)")
        await ev("S47.game.d.inter.get('fieldBox').use()"); await tick(0.1)
        await pg.tap('.docview [data-a=back]'); await tick(0.2)
        # without the bought file: Kenney's steps, quickened, through the same roof
        await ev("delete S47.audio.buf['lic:roof_steps']")
        await ev("T.aim(-3.9, 9.2, -3.65, 0.8, 8.6)")
        started = await ev("S47.roofSteps.now(S47.roofIn())")
        check(started and (await roof())['phase'] == 'come', 'phone: the steps begin (developer hook, from the records room)')
        for _ in range(200):
            if (await roof())['phase'] != 'come': break
            await tick(0.1)
        r = await roof()
        b1 = [l for l in r['log'] if l['burst'] == 1]
        check(not r['bought'] and len(b1) >= 12 and b1[-1]['d'] < 0.65, f'phone: Kenney\'s gravel steps stand in for the bought ones, {len(b1)} of them, stopping right over the player')
        await ev("S47.player.pitch = 1.0; S47.player.yaw = Math.PI * 0.9; S47.tick(0.05)")
        await tick(1.3); await pg.wait_for_timeout(700)
        await shot('phone_01_overhead')
        cap = ' '.join(await ev("T.toasts"))
        check('Footsteps on the roof' in cap, 'phone: the caption is on screen')
        for _ in range(200):
            if (await roof())['phase'] == 'done': break
            await tick(0.2)
        r = await roof()
        check(r['phase'] == 'done' and len([l for l in r['log'] if l['burst'] == 2]) >= 5, 'phone: once more further off, then done')
        await ev("T.aim(-8.5, 20.0, -4.2, 3.85, 8.6)"); await tick(0.6)
        await ev("S47.game.d.ui.clearToasts()"); await pg.wait_for_timeout(400); await tick(0.1)
        await shot('phone_02_frost_outside')
        await ev("S47.input.zoomHeld = true"); await tick(0.8)
        hud = await ev("(() => { const e = document.querySelector('.hud .examine'); return e && e.classList.contains('show') ? e.textContent : null; })()")
        check(hud and 'white with frost' in hud, 'phone: a closer look at the frost')
        await shot('phone_03_frost_look')
        await ev("S47.input.zoomHeld = false"); await tick(0.3)
        print('\n'.join(errs[:30]) or 'phone: no console errors/warnings')
        check(not errs, 'phone: console clean')
        await ctx.close()

        json.dump(checks, open(f'{OUT}/checks-horror-roof.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
