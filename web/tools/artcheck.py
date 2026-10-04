"""Artwork loading, High/Low parity and fixed runtime views (not an input/FPS test).
Usage: python3 tools/artcheck.py shots/art
S47_URL can select the ordinary build served under /SIGNAL-47/.
"""
import asyncio, json, os, sys
from pathlib import Path
from playwright.async_api import async_playwright

OUT = Path(sys.argv[1] if len(sys.argv) > 1 else 'shots/art')
URL = os.environ.get('S47_URL') or Path('dist-single/index.html').resolve().as_uri()
ART_COUNT = 28  # 16 from PR #30, 6 round 3 surfaces from PR #31, vinyl and vending from PR #34, 4 motel images from PR #39
LATER = 55      # STATION 01's surfaces (PR #34), the track gravel and the diner's five (PR #35, the clipping photo now replaced by the 1947 master), the 1947 papers (PR #39), the diner's two (PR #45), the 38 round 9 maps that load only in Ultra (PR #51), and round 10's master photo, Halley poster and two fanfold sheets (PR #54)
VIEWS = [
    ('desk', 2.6, 3.5, 0, -0.50),
    ('room', -4.7, 3.5, -0.65, -0.16),
    ('window', 0.05, -2.2, 0, 0.06),
    ('map', -4.5, -3.0, 1.57, 0.0),
    ('listen', -4.4, -0.6, 1.57, 0.15),
    ('saro', 4.4, 3.1, -1.57, 0.04),
    ('yard', 9.5, -8.2, 0, -0.12),
    ('cabinet', 10.7, -11.0, -1.57, -0.26),
    ('lab', 14.5, -1.4, 0.3, -0.12),
    # round 3 surfaces with the words drawn in code
    ('b12', 9.9, -16.6, 0, 0.40),
    ('r07', 7.22, -18.22, 0.5, -0.33),
    ('fieldmap', 17.0, -0.55, 3.1416, 0.08),
    ('apron', 9.5, -9.3, 0, -0.6),
    ('labsigns', 14.7, -3.8, 0, 0.1),
    # chapter two: the corridor and the records room
    ('corridor', 0.8, 5.7, 1.5708, 0.0),
    ('archive', -2.0, 7.6, 2.135, -0.1),
    # round 4 (PR #34): the vending front with COLD DRINKS drawn in code
    ('vending', 0.2, 5.1, -2.1, -0.3),
]

async def main():
    OUT.mkdir(parents=True, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width': 1280, 'height': 800})
        pg.set_default_timeout(120000)
        errors, checks, views = [], [], []
        pg.on('pageerror', lambda e: errors.append(str(e)))
        pg.on('console', lambda m: errors.append(m.text) if m.type in ('error', 'warning') else None)
        await pg.add_init_script('HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };')
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]')
        art = await pg.evaluate('S47.art()')
        start = [i for i in art['loaded'] if i not in art['later']]
        checks.append((f'all {ART_COUNT} runtime images loaded', len(start) == art['expected'] == ART_COUNT))
        # the Halley poster (round 10) is a later image the control room asks for right after the start
        early = set(art['later']) & set(art['loaded'])
        checks.append((f'{LATER} later images wait for their areas (the poster may already be on its way)', len(art['later']) == LATER and early <= {'halleyPoster'}))
        sky = await pg.evaluate("""(()=>{ let t = null; S47.sky.group.traverse(o => { const u = o.material && o.material.uniforms; if (u && u.uPanorama) t = u.uPanorama.value; });
          return t && { name: t.name, w: t.image.width, h: t.image.height }; })()""")
        checks.append(('sky runtime limited to 2K, painted stars taken out', bool(sky) and sky['w'] == 2048 and sky['h'] == 1024 and sky['name'] == 'art/sky (stars removed)'))
        await pg.click('button[data-a=start]')
        await pg.evaluate("S47.hold=true; S47.jump('chapter1'); S47.game.d.ui.close(); S47.tick(1)")
        maps_by_quality = []
        for q in ['high', 'low']:
            await pg.evaluate(f"S47.setQuality('{q}')")
            maps_by_quality.append(await pg.evaluate("""(()=>{const maps=[]; S47.scene.traverse(o=>{
                for(const m of (Array.isArray(o.material)?o.material:[o.material])) {
                    if(m?.map) maps.push(m.map.uuid);
                }});return maps.sort()})()"""))
            for name, x, z, yaw, pitch in VIEWS:
                await pg.evaluate(f'S47.player.place({x},{z},{yaw}); S47.player.pitch={pitch}; S47.tick(0.1)')
                await pg.screenshot(path=str(OUT / f'{name}-{q}.png'))
                info = await pg.evaluate('({calls:S47.renderer.info.render.calls,triangles:S47.renderer.info.render.triangles,textures:S47.renderer.info.memory.textures})')
                views.append(dict(view=name, quality=q, **info))
            # Diagnostic exterior camera, separate from the walkable player route.
            await pg.evaluate("S47.camera.position.set(-18,3,18); S47.camera.lookAt(-30.4,5.8,30.2); S47.camera.updateMatrixWorld(); S47.renderer.render(S47.scene,S47.camera)")
            await pg.screenshot(path=str(OUT / f'motel-{q}.png'))
        checks.append(('High/Low reuse identical texture maps', maps_by_quality[0] == maps_by_quality[1]))
        checks.append(('room draw calls at most 300', all(v['calls'] <= 300 for v in views if v['view'] in ('desk', 'room', 'window'))))
        checks.append(('no browser or shader errors', not errors))
        expected_failures = []
        # On HTTP, exercise missing-file feedback and the actual retry button.
        if URL.startswith('http'):
            await pg.route('**/*tex_wall_paint*', lambda route: route.abort())
            await pg.reload()
            await pg.get_by_text('The station artwork could not load.', exact=False).wait_for()
            checks.append(('failed image shows retry instead of a black screen', await pg.get_by_role('button', name='TRY AGAIN').is_visible()))
            expected_failures = errors[:]
            errors.clear()
            await pg.unroute('**/*tex_wall_paint*')
            await pg.get_by_role('button', name='TRY AGAIN').click()
            await pg.wait_for_selector('button[data-a=start]')
            checks.append(('retry recovers complete artwork', len((await pg.evaluate('S47.art()'))['loaded']) == ART_COUNT))
            checks.append(('retry has no unexpected errors', not errors))
        report = dict(url=URL, renderer='Chromium SwiftShader, not hardware FPS', checks=checks, art=art, views=views, errors=errors, expected_failure_errors=expected_failures)
        (OUT / 'report.json').write_text(json.dumps(report, indent=2))
        for name, ok in checks: print(('PASS ' if ok else 'FAIL ') + name, flush=True)
        await b.close()
        if not all(ok for _, ok in checks): raise SystemExit(1)

asyncio.run(main())
