# The radio on the way (story/tripRadio.ts, Tom's plan): every trip has its own lines at marks
# along it, in metres driven, said on the station the truck's radio is tuned to in that station's
# voices (the Night Line's host and callers on 1420, Roswell's morning man after five), and
# captioned. Each is heard once a night; a mark with more than one gives the next the next time.
# The station's own programme goes on in between, and the lines never come over the boy's call,
# the 1947 bulletin or the sign-off at four. With the radio off nothing is said.
#
#   1  chapter one, SARO to the diner on 1420, with the 1947 bulletin on the way
#   2  the diner back to SARO, with the boy's call on the way
#   3  SARO to STATION 01 (chained), the radio off for a stretch
#   4  chapter three, the drive out to STATION 01
#   5  chapter three, back to SARO with the film, into the sign-off at four
#   6  chapter five, SARO to the diner on 1230 after Roswell signs on
#
# Usage: python3 tools/tripradio.py OUTDIR
import asyncio, sys, os, json, math, time
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
results = []
def check(ok, msg):
    results.append(bool(ok)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

# one step: on by a second, and what the radio and the trip are doing now
STEP = """((s) => { S47.tick(s, 20); const w = S47.world, t = w.tripNow, R = S47.carRadio, st = S47.radio.stations[R.station], g = st.seg, last = S47.tripRadio().last;
  const l0 = g.kind === 'talk' ? g.lines[0] : null;
  return { driving: w.driving, area: w.area, metres: t ? t.metres : null, bound: t ? t.bound : null, seg: g.kind, slotted: g.kind === 'talk' && !!g.slotted,
    by: l0 ? (l0.by || 'host') : null, first: l0 && typeof l0.text === 'string' ? l0.text : '', free: st.free, own: st['cfg'].loop.includes(g), station: R.station, on: R.on,
    last: last ? last.id + '@' + Math.round(last.metres) : null, clock: S47.game.clock, r1420: S47.nightIn().radio1420,
    audioTime: S47.audio.ctx?.currentTime ?? null, archive: st.archiveStatus }; })"""

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': 1280, 'height': 720})).new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        # Legacy timing/voice baseline; new defaults are exercised in ensemble.py/serling.py.
        await pg.add_init_script("try{localStorage.setItem('s47.serlingNarrator','false');localStorage.setItem('s47.characterVoices','false');}catch{}")
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await pg.wait_for_function('S47.started()', polling=200)
        ev = pg.evaluate
        async def tick(s, fps=20): await ev(f"S47.tick({s}, {fps})")
        toasts = lambda: ev("window.__toasts.filter((t) => typeof t === 'string')")

        await ev("S47.hold = true; S47.tick(0.3); S47.setAbductions(false); S47.radio.boyCalled = true; S47.jump('chapter1'); S47.tick(1.0)")
        # Route and slot regressions fast-forward the original procedural mode;
        # real host/caller/bulletin timing is covered by artbell-radio.py and
        # radiocast-radio.py; the latter observes all 28 new recordings.
        await ev("S47.radio.setHostVoice(false); S47.radio.setCallerVoice(false)")
        async def route_step():
            # The procedural bulletin also occupies 47 real audio seconds. The
            # route can fast-forward between broadcasts, but cannot drive past
            # every remaining mark while those scheduled syllables are playing.
            target = await ev("""() => {
              const st=S47.radio.stations.caliche,ctx=S47.audio.ctx;
              return st.archiveStatus?.state==='playing' && ctx
                ? ctx.currentTime+1 : null;
            }""")
            if target is not None:
                await pg.wait_for_function("target => S47.audio.ctx.currentTime >= target",
                                           arg=target, polling=20, timeout=10000)
            return await ev(f"{STEP}(1.0)")
        await ev("() => { window.__toasts = []; const u = S47.game.d.ui, o = u.toast.bind(u); u.toast = (t, ...args) => { window.__toasts.push(t); return o(t, ...args); }; }")
        # the night's own events wait (one is set off by hand below)
        await ev("S47.nightEvents['wait'] = 1e9")
        plan = await ev("S47.tripRadio().plan")
        mark_of = {i: m for m in plan for i in m['ids']}
        first_of = {i: f for m in plan for i, f in zip(m['ids'], m['first'])}
        by_of = {i: w for m in plan for i, w in zip(m['ids'], m['by'])}
        nlines = await ev("S47.tripRadio().lines")
        check(40 <= nlines <= 70 and len(mark_of) >= 25, f"the programme: {nlines} lines in {len(mark_of)} pieces at {len(plan)} marks")
        heard_all = []   # every piece said in the whole run, in order

        async def drive(name, use, to, station, speed=18, budget=420, at=None):
            """Get in (use: an interaction id, or JS that starts the trip), the radio on `station`,
            the autopilot to `to`; every second the state, and at(metres, state) for the test's own
            doings. Returns the samples and the pieces said on the way."""
            await ev("S47.nightEvents['wait'] = 1e9")
            if use.startswith('S47'): await ev(use)
            else: await ev(f"S47.game.d.inter.get('{use}').use()")
            for _ in range(240):
                await tick(0.25)
                if await ev("S47.world.driving"): break
                await pg.wait_for_timeout(100)
            await ev(f"S47.carRadio.set({'true' if station else 'false'}, '{station or 'caliche'}'); S47.world['trip'].to = '{to}'; S47.world.autopilot = {speed}")
            # (the last piece of the drive before is not this one's)
            shoot = False
            samples, said, prev, t0 = [], [], await ev("(() => { const l = S47.tripRadio().last; return l ? l.id + '@' + Math.round(l.metres) : null; })()"), time.time()
            for i in range(budget):
                s = await route_step()
                s['t'] = i
                samples.append(s)
                if s['last'] and s['last'] != prev:
                    prev = s['last']; pid, m = s['last'].split('@')
                    said.append((pid, int(m), s['clock'], i)); heard_all.append(pid)
                    if len(heard_all) == 2: shoot = True
                elif shoot: await pg.screenshot(path=f'{OUT}/{name}_caption.png'); shoot = False
                if at and s['metres'] is not None: await at(s['metres'], s)
                if not s['driving']: break
            # a piece taken just before the lot is said once the talk before it is over
            for _ in range(14): samples.append({**await route_step(), 't': len(samples)})
            ids = [x[0] for x in said]
            print(f"  {name}: {round(max([s['metres'] or 0 for s in samples] + [0]))} m in {time.time() - t0:.0f} s, said {[(x[0], x[1], round(x[2]) % 86400) for x in said]}", flush=True)
            return samples, said

        def in_marks(said):
            bad = [(i, m) for i, m, _, _ in said if not (mark_of[i]['at'] <= m <= mark_of[i]['at'] + mark_of[i]['reach'])]
            return not bad, bad
        def in_order(said):
            ats = [mark_of[i]['at'] for i, _, _, _ in said]
            return ats == sorted(ats)
        def loop_between(samples):
            """Between two of the trip's pieces the station's own programme is heard."""
            runs, cur = [], None
            for s in samples:
                if s['slotted']:
                    if cur is None or s['first'] != cur: runs.append(('trip', s['first'])); cur = s['first']
                else:
                    if not runs or runs[-1][0] != 'own': runs.append(('own', s['seg']))
                    cur = None
            trips = [k for k, (a, _) in enumerate(runs) if a == 'trip']
            return all(trips[k + 1] - trips[k] >= 2 for k in range(len(trips) - 1))
        def then_record(samples):
            """After each of the trip's pieces the station goes on with a record (the talk that was
            next waits), so the host and the callers do not come one after the other."""
            after = []
            for a, b2 in zip(samples, samples[1:]):
                if a['slotted'] and not b2['slotted'] and b2['own']: after.append(b2['seg'])
            return bool(after) and all(x == 'music' for x in after), after
        async def captioned(said):
            # each piece's line in a caption, a caller's as a caller's and the host's as the radio's
            ts = await toasts()
            def shown(i):
                t = [x for x in ts if first_of[i] in x]
                return bool(t) and t[0].startswith('RADIO, a caller: "' if by_of[i] else 'RADIO: "')
            miss = [i for i, _, _, _ in said if first_of[i] and not shown(i)]
            return not miss, miss

        # ---------- 1: chapter one, SARO to the diner, the bulletin on the way ----------
        bul = {'t0': None, 't1': None}
        async def at1(metres, s):
            if bul['t0'] is None and metres > 1250 and s['free'] and s['r1420']:
                ok = await ev("S47.nightEvents.now('bulletin', S47.nightIn())")
                bul['t0'] = s['t'] if ok else -1
            if bul['t0'] is not None and bul['t0'] >= 0 and bul['t1'] is None and s['by'] != 'a bulletin' and s['t'] > bul['t0'] + 2: bul['t1'] = s['t']
            if 1150 < metres < 1250 and 'roam' not in bul:
                # a drive of the player's own (no place given) south past the track's mouth: bound for the diner
                bul['roam'] = await ev("(() => { const tr = S47.world['trip'], to = tr.to; tr.to = null; const b = S47.world.tripNow.bound; tr.to = to; return b; })()")
        # At 18 m/s the full 47 s archive can end at the diner's entrance. This route tests
        # recovery and another optional mark, so leave travel time for the normal 10 s calm
        # period and a record fade as well; the same distance and strict cue checks remain.
        s1, said1 = await drive('d1_saro_diner', 'truck', 'diner', 'caliche', speed=12, at=at1)
        ids1 = [x[0] for x in said1]
        check(s1[-1]['area'] == 'diner' and len(said1) >= 4 and set(ids1) <= {'out1', 'roof', 'oldroad', 'coffee', 'door'},
              f"1: SARO to the diner on 1420, the Night Line's lines for the way ({ids1})")
        ok, bad = in_marks(said1); check(ok and in_order(said1), f"1: each at its mark along the way, in order ({[(i, m) for i, m, _, _ in said1]}{'; off: ' + str(bad) if bad else ''})")
        ok, miss = await captioned(said1); check(ok, f"1: captioned like the rest of the station{'; missing ' + str(miss) if miss else ''}")
        check(loop_between(s1), '1: the station\'s own programme in between')
        ok, after = then_record(s1); check(ok, f'1: after each piece, a record ({after})')
        check(bul.get('roam') == 'diner', f"1: driven anywhere, south past the track's mouth the trip is bound for the diner ({bul.get('roam')})")
        during = [s for s in s1 if bul['t0'] is not None and bul['t0'] >= 0 and bul['t0'] <= s['t'] < (bul['t1'] or 10**9)]
        nextp = [s['t'] for s in s1 if bul['t1'] and s['t'] >= bul['t1'] and s['slotted']]
        check(bul['t0'] and bul['t0'] > 0 and during and any(s['by'] == 'a bulletin' for s in during) and not any(s['slotted'] for s in during),
              f"1: the 1947 bulletin cuts in on 1420, and nothing of the trip's is said over it ({len(during)} s)")
        check(nextp and nextp[0] - bul['t1'] >= 7, f"1: after it the Night Line's own programme first ({(nextp[0] - bul['t1']) if nextp else None} s before the next of the trip's)")
        check(all(not s['r1420'] for s in s1 if s['slotted']), '1: while a trip\'s line is on, the bulletin could not cut in (nightIn radio1420 false)')

        # ---------- where a drive of the player's own is bound (drive/legs.ts boundOn, loaded with the first trip) ----------
        b_ = await ev("""(() => { const B = S47.world['mods'].legs.boundOn, N = 0, S = Math.PI, W = Math.PI / 2, E = -Math.PI / 2;
          return [B('road', 8000 - 1.8, 300, S, false), B('road', 8000 - 1.8, 800, S, false), B('road', 8000 + 1.8, 300, N, false), B('road', 8000 + 1.8, 1200, N, false),
            B('road', 8000 - 240, 496, W, false), B('road', 8000 - 240, 496, E, false), B('road', 8000 - 240, 496, E, true),
            B('station01', -19, 8200, N, false), B('station01', -19, 8200, S, false), B('saro', -24, 300, N, false), B('saro', -24, 300, S, false), B('saro', 9, 8, N, false)]; })()""")
        want = [None, 'diner', 'saro', None, 'station01', None, 'station01', 'station01', None, 'saro', None, None]
        check(b_ == want, f"bound by the way the truck goes: south past the track's mouth the diner, north the way SARO, in along the track STATION 01, else open ({b_})")

        # ---------- 2: the diner back to SARO, the boy's call on the way ----------
        boy = {'t0': None, 't1': None}
        async def at2(metres, s):
            if boy['t0'] is None and metres > 450 and s['free']:
                # (he calls from 02:26 on; chapter one begins at 02:16)
                await ev("(() => { const R = S47.radio, st = R.stations.caliche; S47.game.clock = Math.max(S47.game.clock, 2 * 3600 + 27 * 60); R.boyCalled = false; st['start'](st['refill'](S47.game.clock), S47.game.clock); })()")
                boy['t0'] = s['t']
            if boy['t0'] is not None and boy['t1'] is None and s['t'] > boy['t0'] + 3 and (s['own'] or s['slotted']): boy['t1'] = s['t']   # his call over: the station's own again
            if metres > 1750 and 'roam' not in boy:
                # north of the track's mouth (road z 513), driven anywhere: bound for SARO
                boy['roam'] = await ev("(() => { const tr = S47.world['trip'], to = tr.to; tr.to = null; const b = S47.world.tripNow.bound; tr.to = to; return b; })()")
        s2, said2 = await drive('d2_diner_saro', 'dinerTruck', 'saro', 'caliche', at=at2)
        ids2 = [x[0] for x in said2]
        check(s2[-1]['area'] == 'saro' and 'watch' in ids2 and set(ids2) <= {'watch', 'dishes'}, f"2: the diner back to SARO: the way's own ({ids2})")
        ok, bad = in_marks(said2); check(ok and in_order(said2), f"2: at the marks ({[(i, m) for i, m, _, _ in said2]})")
        ok, miss = await captioned(said2); check(ok, f"2: captioned{'; missing ' + str(miss) if miss else ''}")
        during = [s for s in s2 if boy['t0'] is not None and boy['t0'] <= s['t'] < (boy['t1'] or 10**9)]
        check(boy['t0'] is not None and any(s['by'] == 'a boy' for s in during) and not any(s['slotted'] for s in during),
              f"2: the boy calls the Night Line on the way, and nothing of the trip's is said over his call ({len(during)} s)")
        check('A boy calling' in ' '.join(await ev("S47.game.notes")), '2: his call goes in the notebook as before')
        check(boy.get('roam') == 'saro', f"2: driven anywhere north of the track's mouth, the trip is bound for SARO ({boy.get('roam')})")

        # ---------- 3: SARO to STATION 01 before chapter three, the radio off for a stretch ----------
        off = {'off': None, 'on': None}
        async def at3(metres, s):
            if off['off'] is None and metres > 600: await ev("S47.carRadio.set(false)"); off['off'] = s['t']
            if off['on'] is None and metres > 1300: await ev("S47.carRadio.set(true, 'caliche')"); off['on'] = s['t']
        s3, said3 = await drive('d3_saro_station', 'truck', 'station01', 'caliche', speed=16, at=at3)
        ids3 = [x[0] for x in said3]
        check(s3[-1]['area'] == 'station01' and 'out2' in ids3 and 'out1' not in ids3 and 'roof' not in ids3,
              f"3: leaving SARO again: the next of the mark's, nothing heard before again ({ids3})")
        quiet = [x for x in said3 if off['off'] is not None and off['off'] <= x[3] < (off['on'] or 10**9)]
        check(off['off'] is not None and not quiet and 'chain' not in ids3, f"3: with the radio off nothing is said, and the caller about the chain is driven past ({ids3})")
        check('lamps' in ids3, f"3: on again near the gate: the host on the survey crews' lamps ({ids3})")

        # ---------- 4: chapter three, the drive out ----------
        await ev("S47.jump('chapter3'); S47.tick(1.0); S47.nightEvents['wait'] = 1e9")
        s4, said4 = await drive('d4_ch3_out', 'truck', 'station01', 'caliche', speed=16)
        ids4 = [x[0] for x in said4]
        check(s4[-1]['area'] == 'station01' and len(said4) >= 3 and set(ids4) <= {'c3out', 'fence', 'gate', 'marks'},
              f"4: chapter three, out to STATION 01: the cut fence, the little gate, the two marks and the lamp ({ids4})")
        ok, bad = in_marks(said4); check(ok and in_order(said4), f"4: at the marks ({[(i, m) for i, m, _, _ in said4]})")
        ok, miss = await captioned(said4); check(ok, f"4: captioned{'; missing ' + str(miss) if miss else ''}")
        ok, after = then_record(s4); check(ok and loop_between(s4), f'4: the programme in between, a record after each piece ({after})')

        # ---------- 5: back to SARO with the film, into the sign-off ----------
        late = {'set': False}
        async def at5(metres, s):
            if not late['set'] and metres > 600 and s['last'] and s['last'].startswith('film') and not s['slotted']:
                await ev("S47.game.clock = 3 * 3600 + 59 * 60"); late['set'] = True
        await ev("S47.game.clock = 3 * 3600 + 50 * 60; window.__toasts = []")
        s5, said5 = await drive('d5_ch3_back', 'S47.world.driveBack()', 'saro', 'caliche', speed=16, at=at5)
        ids5 = [x[0] for x in said5]
        check('film' in ids5 and 'late' not in ids5, f"5: back with the film: the photo counter; nothing in the last minutes before four ({ids5})")
        so = [s for s in s5 if s['seg'] in ('anthem', 'carrier', 'dead') or 'signing off' in (s['first'] or '') or 'Thank you for staying up' in (s['first'] or '')]
        check(late['set'] and so and not any(s['slotted'] for s in so) and 'signing off' in ' | '.join(await toasts()),
              f"5: the Night Line signs off at four on the way, captioned, with nothing of the trip's over it ({[s['seg'] for s in so][:4]})")
        check(not any(s['slotted'] for s in s5 if s['clock'] >= 4 * 3600), '5: and after four nothing on 1420')

        # ---------- 6: chapter five, to the diner on 1230 ----------
        await ev("S47.jump('chapter5'); S47.tick(1.0); S47.nightEvents['wait'] = 1e9")
        # (the truck on B stands aside: after room 6 its engine dies at the turn-off and waits for a key
        # no autopilot turns, story/truckOnB.ts, tools/f_car_b.py)
        await ev("S47.truckB.off = true")
        for _ in range(30):
            await tick(5.0)
            if await ev("S47.radio.stations.roswell.free"): break
        check(await ev("S47.radio.stations.roswell.free"), f"6: Roswell has signed on and is in its morning programme ({(await ev('S47.radio.state()'))['roswell']['seg']})")
        s6, said6 = await drive('d6_ch5_diner', 'truck', 'diner', 'roswell')
        ids6 = [x[0] for x in said6]
        check(s6[-1]['area'] == 'diner' and len(said6) >= 3 and set(ids6) <= {'m1', 'm2', 'm3', 'm4'}, f"6: chapter five on 1230: the morning man's lines for the way ({ids6})")
        ok, bad = in_marks(said6); check(ok and in_order(said6), f"6: at the marks ({[(i, m) for i, m, _, _ in said6]})")
        ok, miss = await captioned(said6); check(ok, f"6: captioned{'; missing ' + str(miss) if miss else ''}")
        ok, after = then_record(s6); check(ok and loop_between(s6), f'6: the programme in between, a record after each piece ({after})')

        # ---------- once a night, and kept with the case ----------
        check(len(heard_all) == len(set(heard_all)), f"nothing said twice in the night ({heard_all})")
        kept = await ev("[...S47.townSeen].filter((k) => k.startsWith('tr:')).map((k) => k.slice(3)).sort()")
        check(kept == sorted(set(heard_all)), f"kept with the case ({len(kept)} in the town's set)")

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'results': results, 'heard': heard_all, 'd1_samples': s1, 'bulletin': bul}, open(f'{OUT}/tripradio.json', 'w'), indent=1)
        print(f'{sum(results)} of {len(results)} PASS')
        await b.close()
        sys.exit(0 if all(results) else 1)

asyncio.run(main())
