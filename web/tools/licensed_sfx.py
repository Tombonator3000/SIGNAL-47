# The bought sounds (Ultimate SFX for Games, Function Loops), which live in a private repo cloned
# to src/assets/licensed (core/Audio.ts, `lic:` buffers). With the folder: every one decodes, is
# heard where it was meant (the radios' static and crackle, the TV's hum, the pressure under the
# warning, a call out in the dark, the payphone's open line, the doors, the keys, the pour, the
# switches, the credits), and each is close in level to what it stands in for, measured in the
# game's own nodes through an AnalyserNode behind a rough K-weighting. Without it: the game sounds
# as before (white noise in the radios, the sawtooth on the TV, the old doors and switches, no
# pressure bed, no extra call) and nothing breaks. Either way the payphone's second call has the
# engine on the line (the truck's own CC0 start and shut-off).
# Usage: python3 tools/licensed_sfx.py OUTDIR [with|without]   (S47_URL for the Pages build;
# without the second argument it says which it found)
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
WANT = sys.argv[2] if len(sys.argv) > 2 else None
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
NAMES = ['call_tone', 'door_heavy', 'door_latch', 'door_light', 'handset_down', 'keys', 'line_hush', 'pour', 'pressure',
         'rack_switch', 'radio_crackle_long', 'radio_crackle_short', 'radio_static', 'roof_steps', 'small_switch', 'tv_hum']
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)
db = lambda x: 20 * math.log10(max(x, 1e-9))

# In the page: a tap with a rough K-weighting (high-pass 38 Hz, +4 dB shelf over 1.7 kHz) into an
# analyser kept alive by a silent path to the output; levels() reads its RMS (a 340 ms window)
# every 50 ms for `secs` of real time. The bus swap (with()) sends what a call makes to the tap.
HELPERS = """(() => {
  const a = S47.audio, ctx = a.ctx;
  window.__tap = () => {
    const inp = ctx.createGain(), hp = ctx.createBiquadFilter(), sh = ctx.createBiquadFilter(), an = ctx.createAnalyser(), z = ctx.createGain();
    hp.type = 'highpass'; hp.frequency.value = 38; hp.Q.value = 0.5; sh.type = 'highshelf'; sh.frequency.value = 1700; sh.gain.value = 4;
    an.fftSize = 16384; z.gain.value = 0; inp.connect(hp); hp.connect(sh); sh.connect(an); an.connect(z); z.connect(ctx.destination);
    return { inp, an };
  };
  window.__levels = async (an, secs) => {
    const d = new Float32Array(an.fftSize), out = [], t0 = performance.now();
    while (performance.now() - t0 < secs * 1000) {
      await new Promise((r) => setTimeout(r, 50));
      an.getFloatTimeDomainData(d); let s = 0; for (const v of d) s += v * v; out.push(Math.sqrt(s / d.length));
    }
    return out;
  };
  // the loudest window of a sound played through play() (or anything that plays on a.sfx) into a tap
  window.__peak = async (fn, secs) => {
    const t = __tap(), keep = a.sfx; a.sfx = t.inp;
    try { fn(); } finally { a.sfx = keep; }
    const l = await __levels(t.an, secs); return Math.max(...l);
  };
  window.__played = [];
  if (!a.__spied) {
    a.__spied = true;
    const play = a.play.bind(a); a.play = (n, o) => { __played.push(n); return play(n, o); };
    const st = a.staticSrc.bind(a); window.__static = [];
    a.staticSrc = () => { const r = st(); __static.push(r ? (r.src.buffer === a.buf['lic:radio_static'] ? 'lic' : r.src.buffer === a.noiseBuffer() ? 'white' : 'other') : null); return r; };
  }
  return true;
})()"""

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': 1280, 'height': 720})).new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL); await pg.wait_for_function('window.S47')
        await pg.evaluate("localStorage.clear()"); await pg.reload(); await pg.wait_for_function('window.S47')
        await pg.click('button[data-a=start]'); await pg.wait_for_function('S47.started() && S47.audio.ready', polling=200)
        ev = pg.evaluate
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        await ev("S47.jump('chapter1')"); await pg.wait_for_timeout(500)
        # (the night's one curve, story/curve.ts, stands aside in this test: each thing on its own timer; tools/f_curve.py has them together)
        await ev("S47.hold = true; S47.curve.free = true; S47.game.d.ui.close(); S47.tick(1.0); S47.nightEvents['wait'] = 1e9")
        await ev(HELPERS)

        # ---------- which are here, and do they decode ----------
        have = await ev("Object.entries(S47.audio.buf).filter(([k]) => k.startsWith('lic:')).map(([k, v]) => [k.slice(4), v.duration, v.numberOfChannels])")
        names = sorted(h[0] for h in have)
        mode = WANT or ('with' if names else 'without')
        lic = mode == 'with'
        print(f'mode: {mode}; decoded: {[(h[0], round(h[1], 2), h[2]) for h in have]}')
        if lic:
            check(names == sorted(NAMES), f'all {len(NAMES)} bought sounds are in the build and decoded ({len(names)}: missing {sorted(set(NAMES) - set(names))}, extra {sorted(set(names) - set(NAMES))})')
            d = {h[0]: h for h in have}
            check(all(d[n][1] > 0.2 for n in names), 'each has sound in it (0.24 s or more)')
            loops = {n: round(d[n][1], 2) for n in ('radio_static', 'tv_hum', 'pressure', 'line_hush') if n in d}
            check(loops.get('radio_static', 0) > 13 and loops.get('pressure', 0) > 30 and abs(loops.get('tv_hum', 0) - 6.58) < 0.02 and loops.get('line_hush', 0) > 18, f'the loops are their full length (WAV, no padding): {loops}')
            check(d.get('pressure', [0, 0, 1])[2] == 2 and all(d[n][2] == 1 for n in names if n != 'pressure'), 'mono, but the pressure (not placed) in stereo')
        else:
            check(not names, f'no bought sounds in this build ({names})')
        fall = await ev("[S47.audio.or('door_heavy', 'doorMetal'), S47.audio.or('call_tone'), S47.audio.lic('tv_hum') ? 1 : 0]")
        check(fall == (['lic:door_heavy', 'lic:call_tone', 1] if lic else ['doorMetal', '', 0]), f'or() and lic() pick {"the bought" if lic else "the old"} sounds ({fall})')

        # ---------- the doors of SARO ----------
        await ev("__played.length = 0; S47.doors.set('east', false, true); S47.doors.set('south', false, true); S47.doors.toggle('east'); S47.tick(1.2); S47.doors.toggle('east'); S47.tick(1.2)")
        await ev("S47.doors.toggle('south'); S47.tick(1.2); S47.doors.toggle('south'); S47.tick(1.2)")
        pl = await ev("__played.slice()")
        want = ['lic:door_latch', 'lic:door_heavy', 'lic:door_light'] if lic else ['switch', 'doorMetal', 'doorClose']
        check(all(w in pl for w in want) and (not lic or not any(x in pl for x in ('switch', 'doorMetal', 'doorClose'))), f'opening and shutting the east and south doors: {pl}')

        # ---------- levels against the old sounds, one call each ----------
        if lic:
            pairs = [('door_heavy', 'doorMetal', 0.32, 0, 3), ('door_light', 'doorClose', 0.4, 0, 3), ('door_latch', 'switch', 0.55, -4, 5),
                     ('rack_switch', 'switch', 0.9, 0, 4), ('small_switch', 'switch', 0.3, 0, 4), ('handset_down', 'thudSoft', 0.5, 0, 3)]
            for new, old, g, aim, tol in pairs:
                a = await ev(f"__peak(() => S47.audio.play('lic:{new}', {{ gain: {g} }}), 1.0)")
                o = await ev(f"__peak(() => S47.audio.play('{old}', {{ gain: {g} }}), 1.0)")
                dd = db(a) - db(o)
                check(abs(dd - aim) <= tol, f'{new} against {old} at gain {g}: {dd:+.1f} dB (aim {aim:+d} within {tol})')
            # the pour: the recording against the synth's band of noise
            a = await ev("__peak(() => S47.audio.pour(), 2.2)")
            o = await ev("(async () => { const a = S47.audio, k = a.buf['lic:pour']; delete a.buf['lic:pour']; try { return await __peak(() => a.pour(), 2.2); } finally { a.buf['lic:pour'] = k; } })()")
            check(abs(db(a) - db(o)) <= 6, f'the recorded pour against the synth pour: {db(a) - db(o):+.1f} dB at its loudest (it starts louder and tails off)')
        await ev("__played.length = 0; S47.audio.pour()")
        check((await ev("__played.slice()")) == (['lic:pour'] if lic else []), 'Audio.pour(): the recording' if lic else 'Audio.pour(): the synth, as before')

        # ---------- the truck: the keys, then its radio ----------
        await ev("__played.length = 0; __static.length = 0; S47.game.d.inter.get('truck').use()")
        for _ in range(240):
            await tick(0.25)
            if await ev("S47.world.driving"): break
            await pg.wait_for_timeout(100)
        pl = await ev("__played.slice()")
        check(await ev("S47.world.driving") and (('lic:keys' in pl) == lic), f'in the truck and started: {"the keys jingle" if lic else "no keys (as before)"} ({pl[:6]})')
        await ev("S47.game.clock = 1 * 3600 + 30 * 60; S47.carRadio.reset(); S47.carRadio.set(true, 'caliche'); S47.tick(0.3)")
        st = await ev("__static.slice()")
        check(st[-1:] == (['lic'] if lic else ['white']), f"the dash radio's static: {'the bought hiss with crackle' if lic else 'white noise'} ({st})")
        cr = await ev("S47.carRadio.cracks")
        check((cr >= 1) == lic, f'switched on: {"a crack of static with the sweep" if lic else "no crack"} ({cr})')
        # the interference, forced now
        await ev("(() => { const z = S47.carRadio['fuzz']; z.next = 0.01; z.left = 0; })(); S47.tick(0.2)")
        cr2 = await ev("S47.carRadio.cracks")
        ink = await ev("S47.carRadio.interference")
        check(ink > 0 and ((cr2 > cr) == lic), f'the interference comes ({ink:.2f}) {"with a recorded crash of static" if lic else "with the synth swell only"} ({cr} to {cr2})')
        # the static's level through the radio's own band-pass and speaker: bought against white noise
        BED = """(async (stat) => {
          const n = S47.carRadio['n'];
          for (const g of [n.sig.gain, n.stat.gain, n.out.gain, n.cab.frequency]) g.cancelScheduledValues(0);
          n.sig.gain.value = 0; n.stat.gain.value = stat; n.out.gain.value = 1; n.cab.frequency.value = 20000;
          const t = __tap(); n.out.connect(t.inp); await new Promise((r) => setTimeout(r, 300));
          const l = await __levels(t.an, 4.0); n.out.disconnect(t.inp); return l;
        })"""
        bed = await ev(f"{BED}(0.4)")
        if lic:
            white = await ev(f"""(async () => {{ const a = S47.audio, k = a.buf['lic:radio_static']; delete a.buf['lic:radio_static'];
              try {{ S47.carRadio.reset(); S47.carRadio.set(true, 'caliche'); S47.tick(0.1); return await {BED}(0.4); }}
              finally {{ a.buf['lic:radio_static'] = k; S47.carRadio.reset(); S47.carRadio.set(true, 'caliche'); S47.tick(0.1); }} }})()""")
            m = lambda l: sum(l) / len(l)
            dd = db(m(bed)) - db(m(white))
            check(abs(dd) <= 1.5, f'the bought static as loud as the white noise it replaces, through the radio: {dd:+.1f} dB')
            # a crash at depth 0.7 over the bed as it is at that depth (0.03 + 0.42 x 0.7)
            for name, ln in (('long', 1.2), ('short', 0.5)):
                base = await ev(f"{BED}(0.32)")
                crash = await ev(f"(async () => {{ const n = S47.carRadio['n'], t = __tap(); n.out.connect(t.inp); S47.carRadio['crack']({ln}, 0.7); const l = await __levels(t.an, {ln} + 0.5); n.out.disconnect(t.inp); return l; }})()")
                up = db(max(crash)) - db(sum(base) / len(base))
                check(1 <= up <= 9, f'the {name} crash stands {up:+.1f} dB over the static at that moment (aim about +4)')
        else:
            check(sum(bed) / len(bed) > 1e-3, f'the white-noise static is heard through the radio ({db(sum(bed) / len(bed)):.1f} dB)')
        # the other AM sets: the filing cabinets', STATION 01's receiver, the cab radio of chapter six
        await ev("__static.length = 0; const v = S47.player.pos.clone(); S47.audio.amRadio(v); S47.audio.radio(v); S47.audio.cabRadio('talk')")
        st = await ev("__static.slice()")
        await ev("S47.audio.amRadio(null); S47.audio.radio(null); S47.audio.cabRadio(null)")
        check(st == (['lic'] * 3 if lic else ['white'] * 3), f'the filing-cabinet radio, the STATION 01 receiver and the cab radio have the same static ({st})')
        await ev("S47.carRadio.set(false)")
        await pg.keyboard.press('e')
        for _ in range(24):
            await tick(0.25)
            if not await ev("S47.world.driving"): break

        # ---------- the motel office's television: the carrier's hum ----------
        await ev("S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.tick(0.2)")
        motel = await ev("(() => { const m = S47.world.motel, v = m.tv.group.getWorldPosition(new S47.three.Vector3()), a = m.anchors.officeInside; return { tv: [v.x, v.y, v.z], a: [a.x, a.z] }; })()")
        tv, a0 = motel['tv'], motel['a']
        await ev(f"S47.game.clock = 4 * 3600 + 5 * 60; S47.player.place({a0[0]}, {a0[1]}, 0); S47.tick(0.3); S47.motelTv.now(); S47.tick(1.0)")
        snd = await ev("(() => { const s = S47.motelTv['snd']; return s ? [S47.motelTv.mode, s.humK] : null; })()")
        check(snd is not None and snd[0] == 'carrier' and snd[1] == (1.8 if lic else 1), f'the bare carrier on the set: {"the recorded hum" if lic else "the sawtooth"} ({snd})')
        HUM = """(async () => { const s = S47.motelTv['snd']; s.hum.gain.cancelScheduledValues(0); s.hum.gain.value = 0.11 * s.humK;
          const t = __tap(); s.hum.connect(t.inp); await new Promise((r) => setTimeout(r, 300)); const l = await __levels(t.an, 1.5); s.hum.disconnect(t.inp); return l; })"""
        if lic:
            h = await ev(f"{HUM}()")
            saw = await ev(f"""(async () => {{ const a = S47.audio, k = a.buf['lic:tv_hum']; delete a.buf['lic:tv_hum'];
              try {{ S47.motelTv.silence(); S47.tick(0.1); const r = await {HUM}(); return [r, S47.motelTv['snd'].humK]; }}
              finally {{ a.buf['lic:tv_hum'] = k; S47.motelTv.silence(); S47.tick(0.1); }} }})()""")
            m = lambda l: sum(l) / len(l)
            dd = db(m(h)) - db(m(saw[0]))
            check(saw[1] == 1 and abs(dd) <= 3, f'the recorded hum against the sawtooth it replaces: {dd:+.1f} dB (K-weighted RMS)')
        await ev("S47.motelTv.reset()")   # the test card again (the carrier holds the time away back)

        # ---------- out in the dark: the seventh call ----------
        name = await ev("(() => { const r = Math.random; Math.random = () => 0.95; try { S47.calls['before'] = null; S47.calls['call'](S47.camera); } finally { Math.random = r; } return S47.calls.last && S47.calls.last.name; })()")
        check(name == ('lic:call_tone' if lic else 'alienTemple'), f'the calls out in the dark: {"one more, the single note" if lic else "the five and the horn, as before"} ({name})')

        # ---------- the payphone in Caliche, the second call ----------
        await ev("window.__p = S47.world.prepare('diner')")
        for _ in range(150):
            if await ev("!!S47.world.town && !!S47.world.diner"): break
            await pg.wait_for_timeout(200)
        await ev("S47.world.enter('diner'); S47.tick(0.3)")
        pp = await ev("!!S47.townEvents['town']")
        check(pp, 'the town is there, and the payphone with it')
        # (no frames while the line is open: away from the phone it would hang up)
        # at the phone, once the call out in the dark above has died away
        await ev("(() => { const v = S47.game.d.inter.get('town:payphone').object.getWorldPosition(new S47.three.Vector3()); S47.player.place(v.x + 1.2, v.z, 0); S47.camera.position.set(v.x + 1.2, v.y, v.z); S47.audio.listener(S47.camera); })()")
        await pg.wait_for_timeout(4000)
        await ev("__played.length = 0; window.__bus = __tap(); S47.audio.sfx.connect(__bus.inp); (() => { const te = S47.townEvents; te.call = 2; te.now = 'ringing'; te['answer'](); })()")
        bus = await ev("__levels(__bus.an, 6.0).then((l) => { S47.audio.sfx.disconnect(__bus.inp); return l; })")
        line = await ev("(() => { const l = S47.audio['loops'].get('town-line'); return l ? (l.src.buffer === S47.audio.buf['lic:line_hush'] ? 'line_hush' : l.src.buffer === S47.audio.buf.gust ? 'gust' : 'other') : null; })()")
        pl = await ev("__played.slice()")
        check(line == ('line_hush' if lic else 'gust'), f'the open line after the engine: {line}')
        check(pl.count('engineStart') == 2 and pl.count('engineOff') == 1, f'far off on the line, the engine turns over twice and dies ({[x for x in pl if x.startswith("engine")]})')
        # 50 ms steps: the engine from 0.4 to 4 s, then only the line (the clicks start at 14 s)
        eng, after = max(bus[10:80]), sum(bus[95:118]) / len(bus[95:118])
        check(4 <= db(eng) - db(after) <= 16, f'the engine is small on the line: {db(eng):.1f} dB at its loudest, {db(eng) - db(after):+.1f} dB over the open line after it')
        if lic:
            LINE = """(async () => { const l = S47.audio['loops'].get('town-line'), t = __tap(); l.gain.connect(t.inp); const r = await __levels(t.an, 2.4); l.gain.disconnect(t.inp); return r; })"""
            hush = await ev(f"{LINE}()")
            await ev("S47.townEvents['stopAll'](); S47.tick(0.2)")
            gust = await ev(f"""(async () => {{ const a = S47.audio, k = a.buf['lic:line_hush']; delete a.buf['lic:line_hush'];
              try {{ const te = S47.townEvents; te.call = 2; te.now = 'ringing'; te['answer'](); return await {LINE}(); }}
              finally {{ a.buf['lic:line_hush'] = k; }} }})()""")
            m = lambda l: sum(l) / len(l)
            dd = db(m(hush)) - db(m(gust))
            check(abs(dd) <= 3, f'the hush on the line against the gust it replaces: {dd:+.1f} dB')
        await ev("S47.townEvents['stopAll'](); S47.tick(0.2)")

        # ---------- the credits ----------
        cred = await ev("(() => { const el = S47.game.d.ui.endcard({ lines: [], buttons: [], credits: true }); const t = el.querySelector('.credits').textContent; el.remove(); return t; })()")
        check(('Function Loops' in cred) == lic, f'the title credits {"name" if lic else "do not name"} Ultimate SFX for Games')

        # ---------- the time away: the pressure under the warning ----------
        A = "S47.abduction()"
        await ev("S47.jump('chapter2'); S47.tick(1.0)")
        await ev("S47.game.d.ui.close(); S47.game.clock = 2 * 3600 + 58 * 60; S47.tick(0.2)")
        await ev("S47.world.enter('saro'); S47.player.place(-24, 200, Math.PI); S47.tick(0.3)")
        await ev(f"{A}.pressure = 5; S47.tick(0.5)")
        bed0 = await ev(f"(() => {{ const b = {A}['bed']; return [{A}.stage, b ? b.gain.gain.value : null]; }})()")
        if bed0[0] != 'warning': print('abductIn', await ev("(() => { const s = S47.abductIn(); return [s.line, s.enabled, s.nextFixed, S47.game.phase, S47.townEvents.now, S47.motelTv.busy]; })()"))
        check(bed0[0] == 'warning' and ((bed0[1] is not None) == lic), f'the warning: {"the pressure starts under it" if lic else "no bed, the synth tone later (as before)"} ({bed0})')
        wl = await ev(f"{A}['warnLen']")
        await tick(0.5, int(wl * 0.9 / 0.5))
        bed1 = await ev(f"(() => {{ const b = {A}['bed'], t = {A}['tone']; return [b ? b.gain.gain.value : null, t ? t.gain.gain.value : null]; }})()")
        if lic:
            check(bed0[1] is not None and bed1[0] is not None and bed1[0] > bed0[1] * 5 and bed1[1] is not None, f'nine tenths through ({wl:.0f} s): the pressure has come up ({bed0[1]} to {bed1[0]}), the tone over it ({bed1[1]})')
            lv = await ev(f"""(async () => {{ const b = {A}['bed'], t = {A}['tone'], x = __tap(), y = __tap(); b.gain.connect(x.inp); t.gain.connect(y.inp);
              const l1 = await __levels(x.an, 1.0), l2 = await __levels(y.an, 1.0); b.gain.disconnect(x.inp); t.gain.disconnect(y.inp);
              return [l1.reduce((s, v) => s + v) / l1.length, l2.reduce((s, v) => s + v) / l2.length]; }})()""")
            check(lv[0] > 1e-3, f'the pressure is heard ({db(lv[0]):.1f} dB) beside the tone ({db(lv[1]):.1f} dB, K-weighted; most of the tone is below what small speakers play)')
        await ev("S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.5)")
        check(await ev(f"{A}.stage") == 'calm' and await ev(f"{A}['bed']") is None, 'indoors before the light: it passes over and the pressure ebbs away')
        await ev(f"S47.player.place(-24, 200, Math.PI); S47.tick(0.3); {A}.pressure = 5; {A}['data'].last = -1e9; S47.tick(0.5)")
        for _ in range(200):
            if await ev(f"{A}.stage") == 'taken': break
            await tick(0.5)
        check(await ev(f"{A}.stage") == 'taken' and await ev(f"{A}['bed']") is None and await ev("S47.audio.ctx.state") == 'suspended', 'taken: the pressure is cut with the rest of the sound')
        for _ in range(120):
            if await ev(f"{A}.stage") in ('back', 'calm'): break
            await tick(0.5)
        check(await ev(f"{A}.stage") in ('back', 'calm') and await ev(f"{A}['bed']") is None, 'awake again: no pressure left over')

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'mode': mode, 'checks': checks}, open(f'{OUT}/licensed_sfx.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS ({mode})")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
