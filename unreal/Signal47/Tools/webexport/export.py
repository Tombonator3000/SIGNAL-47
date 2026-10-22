#!/usr/bin/env python3
"""Exports the web game's whole scene for the Unreal version: starts dist-single/index.html with the
developer menu, loads every area, runs export.js in the page and writes
  OUT/manifest.json     nodes, materials, textures, lights, floods, handles, items, walks, areas
  OUT/geo/geo_NNN.glb   the geometries, a batch per file, mesh "g<id>" per geometry
  OUT/tex/tNNN.png      the textures (flipped where three flips them)
Usage: export.py OUT   (run with the signal47 venv; S47_GPU=1 for the GPU)"""
import asyncio, base64, json, os, sys
from playwright.async_api import async_playwright

OUT = os.path.abspath(sys.argv[1])
WEB = os.path.expanduser('~/SIGNAL-47/web')
URL = 'file://' + os.path.join(WEB, 'dist-single/index.html')
HERE = os.path.dirname(os.path.abspath(__file__))
BATCH = 120

async def main():
    os.makedirs(os.path.join(OUT, 'geo'), exist_ok=True)
    os.makedirs(os.path.join(OUT, 'tex'), exist_ok=True)
    async with async_playwright() as p:
        args = ['--use-angle=gl', '--ignore-gpu-blocklist'] if os.environ.get('S47_GPU') else ['--use-angle=swiftshader', '--enable-unsafe-swiftshader']
        b = await p.chromium.launch(args=args + ['--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width': 1280, 'height': 800}); pg.set_default_timeout(600000)
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL + '?dev')
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('.devbtn')
        await pg.click('.devmenu button.dev:text-is("1 The Second Exposure")')
        await pg.wait_for_function("S47.game && S47.game.phase === 'ch1'")
        # every area, built once (World.prepare and the old road's own loader)
        await pg.evaluate("""async () => { const w = S47.world;
          for (const a of ['station01', 'road', 'diner', 'room6']) await w.prepare(a);
          if (w.ensureOldRoad) await w.ensureOldRoad();
          // SARO's own leg of the drives (its obstacles, and its headlight slots in SARO's floods)
          if (w.ensureSaroLeg) await w.ensureSaroLeg();
          w.enter('saro'); }""")
        await pg.add_script_tag(path=os.path.join(HERE, 'export.js'))
        info = await pg.evaluate("S47X.collect()")
        lights = info.pop('lightsList')
        print('scene:', info, flush=True)
        nodes = []
        for k in range(0, info['nodes'], 400):
            nodes += await pg.evaluate(f"S47X.nodes({k}, {k + 400})")
        # extras() first: it marks which materials use which flood set
        extras = await pg.evaluate("S47X.extras()")
        if os.environ.get('S47_HANDLES_ONLY'):
            # a quick look at the named handles, without the geometry and textures
            with open(os.path.join(OUT, 'handles.json'), 'w') as f: json.dump(extras['handles'], f, indent=0)
            print('handles', len(extras['handles']), flush=True)
            await b.close()
            return
        mats = await pg.evaluate("S47X.materials()")
        texmeta = await pg.evaluate("S47X.textureMeta()")
        ng = await pg.evaluate("S47X.geoCount()")
        files = []
        for k in range(0, ng, BATCH):
            data = await pg.evaluate(f"S47X.glb({k}, {k + BATCH})")
            name = f'geo_{k // BATCH:03d}.glb'
            with open(os.path.join(OUT, 'geo', name), 'wb') as f: f.write(base64.b64decode(data))
            files.append({'file': name, 'from': k, 'to': min(k + BATCH, ng)})
            print('geo', name, flush=True)
        for t in texmeta:
            url = await pg.evaluate(f"S47X.texture({t['id']})")
            if not url: t['missing'] = True; continue
            with open(os.path.join(OUT, 'tex', f"t{t['id']:03d}.png"), 'wb') as f: f.write(base64.b64decode(url.split(',', 1)[1]))
        print('textures', len(texmeta), flush=True)
        # the sky per area, from where the player stands there (the old road is at dawn)
        skies = {}
        for area, at in (('saro', [3.4, 1.62, 1.4]), ('station01', None), ('road', None), ('roswell', None)):
            pos = await pg.evaluate(f"""(async () => {{ const w = S47.world; try {{ w.enter('{area}'); }} catch (e) {{ return null; }}
              await new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)));
              const c = S47.camera.position; return [c.x, c.y, c.z]; }})()""")
            if at is None: at = pos
            if at is None: continue
            url = await pg.evaluate(f"S47X.sky({json.dumps(at)}, 6144)")  # 17 pixels a degree: the stars about as big as on a 1080p screen
            with open(os.path.join(OUT, 'tex', f'sky_{area}.png'), 'wb') as f: f.write(base64.b64decode(url.split(',', 1)[1]))
            skies[area] = {'at': at, 'file': f'sky_{area}.png'}
            print('sky', area, at, flush=True)
        await pg.evaluate("S47.world.enter('saro')")
        manifest = {'skies': skies, 'info': info, 'nodes': nodes, 'materials': mats, 'textures': texmeta, 'lights': lights, 'geoFiles': files, **extras}
        with open(os.path.join(OUT, 'manifest.json'), 'w') as f: json.dump(manifest, f)
        print('manifest written:', len(nodes), 'nodes,', len(mats), 'materials,', len(texmeta), 'textures,', len(lights), 'lights,', len(extras['items']), 'items,', len(extras['floods']), 'flood sets', flush=True)
        await b.close()

asyncio.run(main())
