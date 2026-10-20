# The bought models in the game (world/licensedModels.ts; the .glbs in the private repo cloned to
# src/assets/licensed, models/MANIFEST.md there). With the folder in the build, checks that every
# area's models load (SARO's control room, records room and yard, the motel office, room 6, the
# diner and Caliche, STATION 01), that each stands where the code's prop it replaces stood (its
# box near the code's, its foot on the same floor), inside its room, with the code's prop hidden
# (and back, with the model hidden, when S47.models.set(false)), and in every view, after its frames
# are drawn, what is drawn (the scene's visible chain, as kit.followNear leaves it before a picture):
# no code prop a model stands in for while the models are on, no model while they are off, and the
# view's own models among what is drawn; that the rays still find the
# terminals and their screens sit over the models' glass; prints the draw calls and triangles of
# every view on High and Low with and without the models, the load times, the files' sizes and the
# textures on the graphics card, and takes the same pictures with and without (SHOTS/<view>-on.png,
# -off.png, and -cmp.png side by side). Without the folder (or with ?nomodels in S47_URL) it checks
# that nothing of them is there, and with --baseline FILE (a run without, of this build with
# ?nomodels or of the build before them) that every view costs what it did there.
# Writes OUTDIR/report.json (its "views" are a baseline for the run without). No console errors.
# Usage: python3 tools/licensed_models.py OUTDIR [--shots DIR] [--baseline report.json]
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

args = sys.argv[1:]
OUT = args[0]
SHOTS = args[args.index('--shots') + 1] if '--shots' in args else os.path.join(OUT, 'shots')
BASE = args[args.index('--baseline') + 1] if '--baseline' in args else None
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what): checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

# name, area, the group whose terms the numbers are in (JS), camera (x, y, z), looking at (x, y, z)
# (in the town the heights are over the ground there)
VIEWS = [
    ('c01_console_right', 'saro', 'null', (2.0, 1.45, -2.75), (2.75, 0.95, -3.95)),
    ('c02_console_mid', 'saro', 'null', (-0.9, 1.5, -2.6), (-0.9, 0.95, -4.0)),
    ('c03_west_desk', 'saro', 'null', (-4.25, 1.45, 2.3), (-5.6, 0.95, 2.25)),
    ('c04_cabinets', 'saro', 'null', (-4.1, 1.55, -0.6), (-5.7, 0.9, -0.6)),
    ('c05_room', 'saro', 'null', (4.6, 1.7, 3.6), (-2.0, 0.8, -2.5)),
    ('r01_shelves', 'saro', 'null', (-2.4, 1.55, 7.4), (-5.6, 1.0, 8.9)),
    ('r02_cabinets', 'saro', 'null', (-3.1, 1.5, 8.0), (-4.3, 0.7, 10.2)),
    ('r03_fieldbox', 'saro', 'null', (-4.5, 1.5, 8.2), (-5.6, 1.25, 8.6)),
    ('y01_yard_chair', 'saro', 'null', (8.4, 1.4, 4.4), (6.8, 0.4, 3.0)),
    ('o01_office', 'saro', 'S47.world.motel.group', (-42.2, 1.6, 23.6), (-45.2, 1.0, 26.6)),
    ('o02_office_chair', 'saro', 'S47.world.motel.group', (-43.3, 1.5, 24.2), (-42.45, 0.6, 27.1)),
    ('m01_room6', 'room6', 'S47.world.room6.group', (-1.2, 1.6, 2.6), (0.2, 0.7, -1.2)),
    ('m02_dresser', 'room6', 'S47.world.room6.group', (0.3, 1.5, 0.2), (-1.9, 0.7, -0.9)),
    ('m03_bathroom', 'room6', 'S47.world.room6.group', (0.6, 1.55, 0.4), (-0.8, 0.9, -2.9)),
    ('m04_table', 'room6', 'S47.world.room6.group', (-0.4, 1.6, 0.6), (0.8, 0.6, 1.8)),
    ('d01_booths', 'diner', 'S47.world.diner.group', (-0.6, 1.6, -1.2), (0.7, 0.7, -5.0)),
    ('d02_walk_bench', 'diner', 'S47.world.diner.group', (5.2, 1.6, 3.4), (2.4, 0.5, 5.0)),
    ('t01_laundromat', 'diner', 'S47.world.town.group', (-10.5, 1.7, 2129.5), (-14.6, 0.6, 2132.1)),
    ('t02_gas_chair', 'diner', 'S47.world.town.group', (-13.5, 1.7, 2068.6), (-16.6, 0.6, 2070.3)),
    ('t03_souvenir_chair', 'diner', 'S47.world.town.group', (16.6, 1.7, 2058.0), (19.9, 0.6, 2059.6)),
    ('t04_house_door', 'diner', 'S47.world.town.group', (-15.6, 1.7, 2242.6), (-19.0, 1.1, 2244.4)),
    ('t05_house_door', 'diner', 'S47.world.town.group', (21.6, 1.7, 2157.8), (24.5, 1.1, 2159.4)),
    ('s01_hut', 'station01', 'S47.world.site.group', (-10.4, 1.55, 3.9), (-11.9, 0.8, 2.0)),
    ('s02_battery', 'station01', 'S47.world.site.group', (2.3, 1.2, -1.2), (1.28, -0.05, -2.24)),
]
# the rooms (in their groups' terms) the models inside must stay in: x0, x1, z0, z1 (a door may sit in its wall)
ROOMS = {'control': (-6.0, 6.0, -4.5, 4.5), 'annex': (-6.02, -1.0, 6.8, 10.5), 'office': (-45.6, -41.2, 22.2, 27.6), 'room6': (-2.25, 2.25, -3.0, 3.0), 'hut': (-13.0, -9.0, 1.5, 4.5)}
# how far a model's box may stand from the code's it replaces (m, in plan; the foot in height):
# the boxes on the shelves keep the code's places; the terminals (their keyboards in front) and
# the benches (a model of its own size) stand further
# the receiver stack has the headphones and the microphone in front of it now, the scope stands at the
# desk's end (the picks), the terminals' keyboards in front
# (the laundromat's bench stands 35 cm south of the code's, so the woman sits inside its arm)
NEAR_XZ = {'crt_terminal': 0.2, 'garden_bench': 0.4, 'door_six_panel': 0.12, 'archive_box': 0.12, 'console_kit': 0.4, 'oscilloscope': 0.15}
# and the feet: the code's dresser stood 4 cm off the floor, the town's door frames went 8 cm under it
FOOT = {'dresser': 0.05, 'door_six_panel': 0.09}
# the models each view must draw (with the models on)
VIEW_MODELS = {'c01_console_right': ['crt_terminal', 'console_kit'], 'c02_console_mid': ['crt_terminal'], 'c03_west_desk': ['crt_terminal', 'oscilloscope'],
    'c04_cabinets': ['filing_cabinet', 'archive_box'], 'c05_room': ['crt_terminal', 'filing_cabinet'], 'r01_shelves': ['archive_box', 'filing_cabinet'],
    'r02_cabinets': ['filing_cabinet', 'folding_chair'], 'r03_fieldbox': ['archive_box'], 'y01_yard_chair': ['folding_chair'], 'o01_office': ['bookcase', 'lamp_marble'],
    'o02_office_chair': ['dining_chair'], 'm01_room6': ['lamp_orange', 'nightstand'], 'm02_dresser': ['dresser'], 'm03_bathroom': ['door_six_panel', 'lamp_orange'],
    'm04_table': ['dining_table', 'dining_chair', 'lamp_orange'], 'd01_booths': ['cafe_table'], 'd02_walk_bench': ['garden_bench'], 't01_laundromat': ['garden_bench'],
    't04_house_door': ['door_six_panel'], 't05_house_door': ['door_six_panel'], 's01_hut': ['weathered_chair'], 's02_battery': ['car_battery']}

async def main():
    os.makedirs(OUT, exist_ok=True); os.makedirs(SHOTS, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width': 1280, 'height': 800}); pg.set_default_timeout(300000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await pg.wait_for_timeout(400)
        ev = pg.evaluate
        await ev("S47.hold = true; S47.jump('chapter1'); S47.tick(1.0); S47.setQuality('high'); S47.setAbductions(false)")
        await pg.add_style_tag(content='#ui { visibility: hidden !important; }')
        # (a build from before the models has no S47.models: as one without the folder)
        info = await ev("S47.models ? ({ inBuild: S47.models.inBuild, wanted: S47.models.wanted, files: S47.models.files }) : ({ inBuild: false, wanted: false, files: [], old: true })")
        on = info['inBuild'] and info['wanted']
        print('models in the build:', info['inBuild'], 'wanted:', info['wanted'], len(info['files']), 'files', flush=True)

        async def wait_loaded():
            st = []
            for _ in range(200):
                st = await ev("S47.models.status().swaps.filter(s => s.wants.length).map(s => [s.name, s.loaded, s.failed])")
                if all(x[1] or x[2] for x in st): break
                await pg.wait_for_timeout(250)
            return st
        if not info.get('old'): await wait_loaded()
        # the later areas, built as the story gets there
        for area in ['room6', 'diner', 'station01']:
            await ev(f"S47.world.prepare('{area}')")
        st = await wait_loaded() if not info.get('old') else []
        status = await ev("S47.models ? S47.models.status() : { swaps: [], files: {}, textures: [], texBytes: 0, tris: 0 }")

        async def frame(area, grp, cam, at, models=True, q='high'):
            """The view: its area entered, the player near (the annex is drawn only from in it), the
            camera set, one picture drawn; its draw calls and triangles."""
            return await ev(f"""(() => {{
              const T = S47.three, g = {grp}, w = (p) => g ? g.localToWorld(new T.Vector3(...p)) : new T.Vector3(...p);
              if (S47.world.area !== '{area}') S47.world.enter('{area}');
              if (S47.models) S47.models.set({str(models).lower()}); S47.setQuality('{q}');
              const c = w({list(cam)}), a = w({list(at)});
              if ({'true' if 'town' in grp else 'false'}) {{ c.y += S47.world.town.floorAt(c.x, c.z); a.y += S47.world.town.floorAt(a.x, a.z); }}
              S47.player.place(c.x, c.z, Math.atan2(-(a.x - c.x), -(a.z - c.z))); S47.tick(0.05);
              const cm = S47.camera; cm.position.copy(c); cm.lookAt(a); cm.updateMatrixWorld();
              // (a frame drawn first: what followNear and the area set before a picture holds after it)
              S47.renderer.render(S47.scene, cm); S47.tick(0.02); cm.position.copy(c); cm.lookAt(a); cm.updateMatrixWorld();
              S47.renderer.info.reset(); S47.renderer.render(S47.scene, cm);
              // what was drawn: the objects whose whole chain up to the scene is visible
              const vis = new Set(); S47.scene.traverseVisible((o) => vis.add(o));
              const holders = [], models = new Set();
              if (S47.models) for (const s of S47.models.swaps) {{
                for (const h of s.holders) if (vis.has(h)) holders.push(s.name + ':' + h.userData.licensed);
                for (const m of s.models) m.traverse((x) => {{ if (x.isMesh && vis.has(x) && x.name.startsWith('lic:')) models.add(x.userData.model); }});
              }}
              return {{ calls: S47.renderer.info.render.calls, tris: S47.renderer.info.render.triangles, textures: S47.renderer.info.memory.textures, geometries: S47.renderer.info.memory.geometries,
                drawnHolders: holders, drawnModels: [...models].sort() }};
            }})()""")

        views = []
        if not on:
            # ---------- without the models ----------
            check(all(not s['wants'] and not s['holders'] and not s['models'] for s in status['swaps']), f'no area wants a model, holds a code prop apart or has a model ({len(status["swaps"])} areas)')
            n = await ev("(() => { let n = 0; S47.scene.traverse(o => { if (o.name.startsWith('lic:')) n++; }); return n; })()")
            check(n == 0, f'nothing of the models in the scene ({n})')
            for name, area, grp, cam, at in VIEWS:
                for q in ['high', 'low']:
                    r = await frame(area, grp, cam, at, True, q)
                    views.append(dict(view=name, quality=q, **r))
                    if q == 'high': await pg.screenshot(path=f'{SHOTS}/{name}-without.png')
            if BASE:
                base = {(v['view'], v['quality'], v.get('models', 'off')): v for v in json.load(open(BASE))['views']}
                # the same draw calls; the triangles within half a percent (the moths, the tumbleweeds and
                # the town's cars are not where they were: two runs of one build differ by 0.2 percent)
                diff = [(v['view'], v['quality'], v['calls'], v['tris'], base.get((v['view'], v['quality'], 'off'))) for v in views]
                same = [d for d in diff if d[4] and d[2] == d[4]['calls'] and abs(d[3] - d[4]['tris']) <= 0.005 * d[4]['tris']]
                check(len(same) == len(diff), f'every view costs what the baseline\'s does: the same draw calls, the triangles within 0.5 % ({len(same)}/{len(diff)}){": " + str([d[:4] for d in diff if d not in same][:6]) if len(same) < len(diff) else ""}')
        else:
            # ---------- with them ----------
            bad = [s for s in st if not s[1]]
            check(not bad, f'every area\'s models load ({[s[0] for s in st]}){" FAILED " + str(bad) if bad else ""}')
            got = set(m for s in status['swaps'] for m in s['wants'])
            check(got == set(info['files']), f'all {len(info["files"])} files are used ({sorted(set(info["files"]) - got)} unused)')
            # where the models stand against the code's props
            where = await ev("""(() => {
              const T = S47.three, out = [];
              for (const s of S47.models.swaps) {
                // the swap's models' boxes in the terms of the area's group (instances one by one)
                const root = s.holders[0] ?? s.models[0];
                if (!root) continue;
                let area = root; while (area.parent && !(area.parent.isScene)) area = area.parent;
                area.updateMatrixWorld(true);
                const inv = area.matrixWorld.clone().invert(), by = {};
                const put = (k, box) => { (by[k] ??= new T.Box3()).union(box); };
                for (const o of s.models) o.traverse((x) => {
                  if (!x.isMesh) return;
                  const k = x.userData.model ?? x.name.replace(/^lic:/, '');
                  if (!x.geometry.boundingBox) x.geometry.computeBoundingBox();
                  const n = x.isInstancedMesh ? x.count : 1, m = new T.Matrix4(), w = new T.Matrix4();
                  for (let i = 0; i < n; i++) {
                    if (x.isInstancedMesh) x.getMatrixAt(i, m); else m.identity();
                    w.multiplyMatrices(inv, x.matrixWorld).multiply(m);
                    put(k, x.geometry.boundingBox.clone().applyMatrix4(w));
                  }
                });
                const code = {};
                for (const h of s.holders) {
                  const k = h.userData.licensed;
                  const box = new T.Box3();
                  h.traverse((x) => { if (x.isMesh && x.material && x.material.visible !== false) { if (!x.geometry.boundingBox) x.geometry.computeBoundingBox(); box.union(x.geometry.boundingBox.clone().applyMatrix4(new T.Matrix4().multiplyMatrices(inv, x.matrixWorld))); } });
                  if (!box.isEmpty()) (code[k] ??= new T.Box3()).union(box);
                }
                out.push({ swap: s.name, models: Object.fromEntries(Object.entries(by).map(([k, b]) => [k, [b.min.toArray(), b.max.toArray()]])),
                  code: Object.fromEntries(Object.entries(code).map(([k, b]) => [k, [b.min.toArray(), b.max.toArray()]])),
                  holdersHidden: s.holders.every((h) => !h.visible), modelsShown: s.models.every((m) => m.visible) });
              }
              return out;
            })()""")
            alias = {'crt_keyboard': 'crt_terminal', 'filing_cabinet_ctrl': 'filing_cabinet', 'filing_cabinet_rec': 'filing_cabinet', 'archive_box_lid': 'archive_box',
                     'signal_analyzer': 'console_kit', 'vu_meter_1': 'vu_meter', 'vu_meter_2': 'vu_meter', 'lamp_orange_1': 'lamp_orange', 'lamp_orange_2': 'lamp_orange',
                     'lamp_marble_1': 'lamp_marble', 'lamp_marble_2': 'lamp_marble'}
            placed = []
            for s in where:
                check(s['holdersHidden'] and s['modelsShown'], f'{s["swap"]}: the code\'s props hidden, the models shown')
                models = {}
                for k, bx in s['models'].items():
                    k = alias.get(k, k)
                    if k in models: bx = [[min(a, c) for a, c in zip(models[k][0], bx[0])], [max(a, c) for a, c in zip(models[k][1], bx[1])]]
                    models[k] = bx
                for k, mb in models.items():
                    cb = s['code'].get(k)
                    mc = [(mb[0][i] + mb[1][i]) / 2 for i in range(3)]
                    room = ROOMS.get(s['swap'])
                    inside = True
                    if room and k != 'door_six_panel':
                        inside = mb[0][0] >= room[0] - 0.02 and mb[1][0] <= room[1] + 0.02 and mb[0][2] >= room[2] - 0.02 and mb[1][2] <= room[3] + 0.02
                    if cb:
                        cc = [(cb[0][i] + cb[1][i]) / 2 for i in range(3)]
                        dxz = math.hypot(mc[0] - cc[0], mc[2] - cc[2]); dy = mb[0][1] - cb[0][1]
                        tol = NEAR_XZ.get(k, 0.1)
                        ok = dxz <= tol and abs(dy) <= FOOT.get(k, 0.03) and inside
                        placed.append(dict(swap=s['swap'], model=k, dxz=round(dxz, 3), dfoot=round(dy, 3), size=[round(mb[1][i] - mb[0][i], 3) for i in range(3)], code=[round(cb[1][i] - cb[0][i], 3) for i in range(3)], inside=inside))
                        check(ok, f'{s["swap"]}/{k}: stands where the code\'s did ({dxz:.3f} m off in plan, foot {dy:+.3f} m; model {placed[-1]["size"]}, code {placed[-1]["code"]}){"" if inside else " OUTSIDE ITS ROOM"}')
                    else:
                        placed.append(dict(swap=s['swap'], model=k, at=[round(v, 2) for v in mc], foot=round(mb[0][1], 3), size=[round(mb[1][i] - mb[0][i], 3) for i in range(3)], inside=inside))
                        check(inside, f'{s["swap"]}/{k}: added at {placed[-1]["at"]}, foot at {placed[-1]["foot"]}{"" if inside else " OUTSIDE ITS ROOM"}')
            # the feet of what is added on the town's ground and the station's (on the ground under them)
            feet = await ev("""(() => {
              const t = S47.world.town, s = S47.world.site, out = {};
              for (const sw of S47.models.swaps) for (const o of sw.models) o.traverse((x) => {
                if (!x.isMesh || x.isInstancedMesh) return;
                const b = new S47.three.Box3().setFromObject(x), c = b.getCenter(new S47.three.Vector3());
                if (sw.name === 'town' && x.name !== 'lic:door_six_panel') out[x.name] = [b.min.y, t.floorAt ? t.floorAt(c.x, c.z) : null];
                if (sw.name === 'station' ) out[x.name] = [b.min.y, null];
              });
              return out;
            })()""")
            print('feet (model, ground):', feet, flush=True)
            # the rays: the terminals are still found, from the console chair and the west desk
            rays = await ev("""(() => {
              const T = S47.three, cm = S47.camera, out = {};
              S47.world.enter('saro');
              for (const [id, c, a] of [['crtCenter', [0.05, 1.25, -3.0], [0, 1.15, -3.9]], ['crtRight', [2.1, 1.25, -3.0], [2.17, 1.0, -3.9]], ['opsTerminal', [-4.6, 1.25, 2.5], [-5.6, 1.1, 2.48]]]) {
                cm.position.set(...c); cm.lookAt(new T.Vector3(...a)); cm.updateMatrixWorld();
                const it = S47.game.d.inter.pick(cm), ex = S47.game.d.inter.examine(cm);
                out[id] = [it ? it.id : null, ex ? ex.it.id : null];
              }
              return out;
            })()""")
            check(all(v[0] == k and v[1] == k for k, v in rays.items()), f'the rays find the terminals and a closer look reads them ({rays})')
            # the screens laid over the glass, and back on the code's terminals without the models
            scr = await ev("""(() => {
              const r = S47.room, s = (c) => c.group.children.find((m) => m.material === c.mat);
              const a = s(r.crtCenter), on = [a.position.y, a.position.z, a.scale.x];
              S47.models.set(false); const off = [a.position.y, a.position.z, a.scale.x]; S47.models.set(true);
              return { on, off };
            })()""")
            check(abs(scr['off'][0] - 0.27) < 1e-6 and abs(scr['off'][1] - 0.187) < 1e-6 and abs(scr['on'][0] - 0.3974) < 0.01, f'the CRT screen over the model\'s glass ({scr["on"]}), back on the code\'s terminal without it ({scr["off"]})')
            tog = await ev("""(() => { S47.models.set(false); const a = S47.models.swaps.every((s) => s.holders.every((h) => h.visible) && s.models.every((m) => !m.visible));
              S47.models.set(true); const b = S47.models.swaps.every((s) => s.holders.every((h) => !h.visible) && s.models.every((m) => m.visible)); return [a, b]; })()""")
            check(tog[0] and tog[1], f'S47.models.set(false) brings the code\'s props back and hides the models, set(true) the other way ({tog})')
            # the pictures and the costs, with and without, High and Low
            for name, area, grp, cam, at in VIEWS:
                for q in ['high', 'low']:
                    for m in [True, False]:
                        r = await frame(area, grp, cam, at, m, q)
                        views.append(dict(view=name, quality=q, models='on' if m else 'off', **r))
                        if q == 'high': await pg.screenshot(path=f'{SHOTS}/{name}-{"on" if m else "off"}.png')
                        if q == 'low' and m: await pg.screenshot(path=f'{SHOTS}/{name}-low-on.png')
            # what is drawn, after the frames: the code's props not with the models, the models not without
            # them, and each view's own models there (a code prop in a near-only group shows again)
            hv = [(v['view'], v['quality'], v['drawnHolders']) for v in views if v['models'] == 'on' and v['drawnHolders']]
            check(not hv, f'with the models on no code prop they stand in for is drawn in any view ({len([v for v in views if v["models"] == "on"])} views){": " + str(hv[:5]) if hv else ""}')
            mv = [(v['view'], v['quality'], v['drawnModels']) for v in views if v['models'] == 'off' and v['drawnModels']]
            check(not mv, f'with the models off none is drawn ({len([v for v in views if v["models"] == "off"])} views){": " + str(mv[:5]) if mv else ""}')
            miss = [(v['view'], v['quality'], sorted(set(VIEW_MODELS.get(v['view'], [])) - set(v['drawnModels']))) for v in views if v['models'] == 'on' and set(VIEW_MODELS.get(v['view'], [])) - set(v['drawnModels'])]
            check(not miss, f'every view draws its own models ({sum(len(x) for x in VIEW_MODELS.values())} in {len(VIEW_MODELS)} views, High and Low){": MISSING " + str(miss[:6]) if miss else ""}')
            # the pictures on a phone and on Low: 512 a side at most, the small props' 256, and on High
            # (the computer's) back as built
            caps = {}
            for q in ['low', 'high']:
                await ev(f"S47.setQuality('{q}')"); await ev("S47.models.fitted()")
                st2 = await ev("S47.models.status()")
                caps[q] = dict(capped=st2['capped'], mb=round(st2['texBytes'] / 1e6, 1), most=max(max(t['w'], t['h']) for t in st2['textures']), n=len(st2['textures']))
            check(caps['low']['capped'] and caps['low']['most'] <= 512 and caps['low']['mb'] <= 20, f'on Low the models\' pictures are capped: at most {caps["low"]["most"]} a side, {caps["low"]["mb"]} MB ({caps["low"]["n"]} pictures)')
            check(not caps['high']['capped'] and caps['high']['most'] == 1024 and abs(caps['high']['mb'] - status['texBytes'] / 1e6) < 0.2, f'on High they are as built again: {caps["high"]["most"]} a side, {caps["high"]["mb"]} MB (as loaded: {status["texBytes"] / 1e6:.1f} MB)')
            # Ultra (the PC's tier: shadows, the normal maps of the game's own pictures) takes them too
            for name, area, grp, cam, at in [v for v in VIEWS if v[0] in ('c01_console_right', 'm01_room6', 's01_hut')]:
                r = await frame(area, grp, cam, at, True, 'ultra')
                views.append(dict(view=name, quality='ultra', models='on', **r))
                await pg.screenshot(path=f'{SHOTS}/{name}-ultra-on.png')
            await ev("S47.setQuality('high')")
            await ev("S47.models.set(true)")
            for name, *_ in VIEWS:
                on_, off_ = [next(v for v in views if v['view'] == name and v['quality'] == 'high' and v['models'] == k) for k in ('on', 'off')]
                lo_on, lo_off = [next(v for v in views if v['view'] == name and v['quality'] == 'low' and v['models'] == k) for k in ('on', 'off')]
                print(f'  {name:20s} High calls {off_["calls"]:4d} -> {on_["calls"]:4d}  tris {off_["tris"]:7d} -> {on_["tris"]:7d}   Low calls {lo_off["calls"]:4d} -> {lo_on["calls"]:4d}', flush=True)
            ctl = [v for v in views if v['view'].startswith('c') and v['models'] == 'on']
            check(all(v['calls'] <= 300 for v in ctl), f'the control room\'s views within 300 draw calls with the models (most {max(v["calls"] for v in ctl)})')
            din = [v for v in views if v['view'].startswith('d') and v['models'] == 'on']
            check(all(v['calls'] < 120 for v in din), f'the diner\'s views under 120 draw calls with the models (most {max(v["calls"] for v in din)})')
            twn = [v for v in views if v['view'].startswith('t') and v['models'] == 'on']
            check(all(v['calls'] <= 300 for v in twn), f'the town\'s views within 300 draw calls with the models (most {max(v["calls"] for v in twn)})')
            loads = {s['name']: s['ms'] for s in status['swaps'] if s['wants']}
            print('load (ms, from the start of each area\'s load to its models in):', loads, flush=True)
            print('files (ms, bytes):', {k: (round(v['ms']), v['bytes']) for k, v in status['files'].items()}, flush=True)
            print(f'textures: {len(status["textures"])}, {status["texBytes"] / 1e6:.1f} MB on the graphics card with mipmaps; triangles of all the models placed: {status["tris"]}', flush=True)
            json.dump(dict(placed=placed, feet=feet, rays=rays, caps=caps), open(f'{OUT}/placed.json', 'w'), indent=1)
            # side by side, without on the left
            try:
                from PIL import Image
                for name, *_ in VIEWS:
                    a, c = Image.open(f'{SHOTS}/{name}-off.png'), Image.open(f'{SHOTS}/{name}-on.png')
                    im = Image.new('RGB', (a.width + c.width, a.height)); im.paste(a, (0, 0)); im.paste(c, (a.width, 0))
                    im.resize((im.width // 2, im.height // 2)).save(f'{SHOTS}/{name}-cmp.png')
            except Exception as e: print('no side-by-side pictures:', e)
        check(not errs, f'no console errors or warnings ({errs[:4]})')
        json.dump(dict(url=URL, info=info, status=status, views=views, checks=checks, errors=errs), open(f'{OUT}/report.json', 'w'), indent=1)
        await b.close()
    fails = [c for c in checks if c[0] == 'FAIL']
    print(f'{len(checks) - len(fails)}/{len(checks)} passed')
    sys.exit(1 if fails else 0)

asyncio.run(main())
