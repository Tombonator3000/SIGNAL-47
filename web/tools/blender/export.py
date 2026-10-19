# The game's own geometry out to glTF, for Blender (tools/blender/build.py runs this first).
# Starts the single-file build like the tests do, and writes with three's GLTFExporter (bundled
# here with esbuild, the game itself does not carry it), all in the site's metres:
#   saro: the house as the game builds it, the meshes named house:* (ControlRoom.outside,
#         RecordsAnnex.outside, Exterior.house: the walls' outsides, the roofs, the plinths, the
#         east wing), and round it, as `ctx`, what stands near enough to shade it (the yard, the
#         photo lab, the grounds' things and the dressing, the control room's inside seen through
#         its window), only for the bake. Also <name>.json: what the Blender script needs to know
#         that only the running game knows (which of the wing's painted windows are lit).
#   truck: SARO 07 (drive/Truck.ts) standing at the origin, unturned, as the game builds it: the
#         merged meshes of its body, its shell and its cab, named <group>:<material> after the
#         truck's own materials (body:paint, shell:glass, cab:vinyl, ...), and its one wheel
#         (`wheel`, axis X, the rim's face to +X, as the four instances draw it).
# Usage (from web/, the game's Python with Playwright): python tools/blender/export.py saro|truck OUTDIR
import asyncio, base64, json, os, subprocess, sys
from playwright.async_api import async_playwright

WHAT, OUT = sys.argv[1], sys.argv[2]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
GPU = os.environ.get('S47_GPU')

def exporter_js():
    src = "import { GLTFExporter } from 'three/examples/jsm/exporters/GLTFExporter.js'; window.S47GLTFExporter = GLTFExporter;"
    return subprocess.run(['node_modules/.bin/esbuild', '--bundle', '--format=iife', '--minify', '--log-level=warning', '--sourcefile=x.js'],
                          input=src, capture_output=True, text=True, check=True).stdout

# In the page: the named house meshes and the context near them, as plain meshes in world space
# (their geometry as built, a material that only carries the name), then the glb as base64.
SARO = r"""async () => {
  const T = S47.three, sc = S47.scene;
  sc.updateMatrixWorld(true);
  const out = new T.Scene();
  const put = (src, name) => {
    const m = new T.Mesh(src.geometry, new T.MeshStandardMaterial({ name }));
    m.name = name;
    src.matrixWorld.decompose(m.position, m.quaternion, m.scale);
    out.add(m);
  };
  const house = [];
  sc.traverse((o) => { if (o.isMesh && o.name.startsWith('house:')) house.push(o); });
  for (const o of house) put(o, o.name);
  // the context: what can shade the house's walls in the bake, within the yard round it
  const region = new T.Box3(new T.Vector3(-16, -2, -16), new T.Vector3(48, 10, 22));
  const roots = [S47.ext.group, S47.yard.group, S47.annex.group, S47.world.grounds.group, S47.room.group];
  const seen = new Set(), bb = new T.Box3();
  let ctx = 0;
  for (const r of roots) r.traverse((o) => {
    if (!o.isMesh || o.isInstancedMesh || seen.has(o) || o.name.startsWith('house:')) return;
    for (let p = o; p; p = p.parent) if (p === S47.annex.interior) return;
    seen.add(o);
    const mats = Array.isArray(o.material) ? o.material : [o.material];
    if (mats.some((m) => !m || m.visible === false || m.transparent || m.isMeshBasicMaterial)) return;
    if (o.layers.mask === (1 << 31)) return;
    if (!o.geometry.boundingBox) o.geometry.computeBoundingBox();
    bb.copy(o.geometry.boundingBox).applyMatrix4(o.matrixWorld);
    const s = bb.getSize(new T.Vector3());
    if (!bb.intersectsBox(region) || s.x > 200 || s.z > 200) return;
    put(o, 'ctx'); ctx++;
  });
  // the wing's painted windows that are lit (Exterior.ts: core/textures facadeWindows, eight a side;
  // the south face's picture runs west to east, the north face's east to west)
  const wing = sc.getObjectByName('house:wing');
  const lit = (mi, i) => {
    const img = wing.material[mi].map.image, g = img.getContext('2d');
    const u = (2.25 * i + 1.125) / 18, x = Math.floor((mi === 4 ? u : 1 - u) * img.width);
    const px = g.getImageData(x, Math.floor(0.51 * img.height), 1, 1).data;
    return px[0] > 150;
  };
  const facts = { litSouth: [0, 1, 2, 3, 4, 5, 6, 7].map((i) => lit(4, i)), litNorth: [0, 1, 2, 3, 4, 5, 6, 7].map((i) => lit(5, i)),
    house: house.map((o) => o.name), ctx };
  const glb = await new window.S47GLTFExporter().parseAsync(out, { binary: true });
  let bin = '';
  const u8 = new Uint8Array(glb);
  for (let i = 0; i < u8.length; i += 0x8000) bin += String.fromCharCode.apply(null, u8.subarray(i, i + 0x8000));
  return { facts, glb: btoa(bin) };
}"""

# In the page: SARO's truck (World.saroTruck) moved to the origin for the export and put back.
TRUCK = r"""async () => {
  const T = S47.three, t = S47.world.saroTruck;
  const at = { p: t.group.position.clone(), r: t.group.rotation.clone() };
  t.group.position.set(0, 0, 0); t.group.rotation.set(0, 0, 0);
  t.setDriving(false); t.setWheelDrops([0, 0, 0, 0]); t.update(0, 0, 0);
  t.group.updateMatrixWorld(true);
  const names = new Map(Object.entries(t.m).map(([k, m]) => [m, k]));
  const out = new T.Scene(), list = [];
  const put = (src, name) => {
    const m = new T.Mesh(src.geometry, new T.MeshStandardMaterial({ name }));
    m.name = name;
    src.matrixWorld.decompose(m.position, m.quaternion, m.scale);
    out.add(m); list.push(name);
  };
  for (const [part, g] of [['body', t.body], ['shell', t.shell], ['cab', t.cab]]) {
    g.traverse((c) => {
      if (!c.isMesh || c.isInstancedMesh || c === t.radioHit) return;
      const n = names.get(c.material);
      if (!n) return;
      put(c, `${part}:${n}`);
    });
  }
  const w = new T.Mesh(t.wheels.geometry, new T.MeshStandardMaterial({ name: 'wheel' }));
  w.name = 'wheel'; out.add(w); list.push('wheel');
  const glb = await new window.S47GLTFExporter().parseAsync(out, { binary: true });
  t.group.position.copy(at.p); t.group.rotation.copy(at.r); t.update(0, 0, 0);
  let bin = '';
  const u8 = new Uint8Array(glb);
  for (let i = 0; i < u8.length; i += 0x8000) bin += String.fromCharCode.apply(null, u8.subarray(i, i + 0x8000));
  return { facts: { meshes: list }, glb: btoa(bin) };
}"""
JS = {'saro': SARO, 'truck': TRUCK}

async def main():
    os.makedirs(OUT, exist_ok=True)
    js = exporter_js()
    async with async_playwright() as p:
        args = ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist']
        b = await p.chromium.launch(args=args)
        pg = await b.new_page(viewport={'width': 640, 'height': 400}); pg.set_default_timeout(300000)
        errs = []
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await pg.wait_for_timeout(300)
        await pg.evaluate("S47.hold = true; S47.jump('chapter1'); S47.tick(0.5)")
        await pg.add_script_tag(content=js)
        if WHAT not in JS: raise SystemExit(f'nothing to export for {WHAT}')
        await pg.wait_for_function("!!S47.world.saroTruck")
        r = await pg.evaluate(JS[WHAT])
        await b.close()
    if errs: print('\n'.join(errs)); raise SystemExit(1)
    with open(os.path.join(OUT, f'{WHAT}_game.glb'), 'wb') as f: f.write(base64.b64decode(r['glb']))
    with open(os.path.join(OUT, f'{WHAT}_game.json'), 'w') as f: json.dump(r['facts'], f, indent=1)
    if WHAT == 'saro':
        print(f"exported {WHAT}: {', '.join(r['facts']['house'])} and {r['facts']['ctx']} meshes round them;",
              f"lit windows south {r['facts']['litSouth']}, north {r['facts']['litNorth']}")
    else:
        print(f"exported {WHAT}: {', '.join(r['facts']['meshes'])}")

asyncio.run(main())
