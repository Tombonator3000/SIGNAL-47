# The Blender models, rebuilt from the game by one command (from web/):
#   python3 tools/blender/build.py [saro] [truck] [--size N] [--quality 88] [--no-build]
# (no name: both). For each:
# 1. the single-file build (npm run build:single, once), so the export sees the game as it is now
# 2. tools/blender/export.py: the game's own geometry to glTF (the house's boxes with what stands
#    round them; the truck at the origin)
# 3. the model's script in Blender (headless): modelled, two UV sets, the masks baked with
#    Cycles on the graphics card (OptiX), exported as .glb (saro_house.py, truck.py)
# 4. each mask as a 4:4:4 JPEG into the .glb as its materials' occlusion texture; the truck's
#    normals and lightmap coordinates quantised (glbpack.py)
# Out: src/assets/models/<name>.glb (shipped, loaded only with ?blender: world/blenderModels.ts);
# the export, the masks and the .blend in src/assets/art/production/blender/ (not shipped).
# BLENDER (default `blender`) and S47_PY (the Python with Playwright, default
# ~/.venvs/signal47/bin/python) can be set.
import json, os, subprocess, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import glbpack

TRUCK = {'body_paint', 'body_chrome', 'body_dark', 'shell_paint', 'shell_chrome', 'shell_dark'}
MODELS = {
    'saro': {'script': 'saro_house.py', 'name': 'saro_house', 'size': 2048, 'masks': [('saro_house_mask', {'house', 'wing'})]},
    'truck': {'script': 'truck.py', 'name': 'saro_truck', 'size': 1024, 'quantize': True,
              'masks': [('saro_truck_mask', TRUCK), ('saro_truck_wheel_mask', {'wheel'})]},
}
names, i = [], 1
while i < len(sys.argv):
    if sys.argv[i].startswith('--'): i += 1 if sys.argv[i] == '--no-build' else 2; continue
    names.append(sys.argv[i]); i += 1
names = names or list(MODELS)
opt = lambda k, d: sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d
quality = int(opt('--quality', 88))
here = os.path.dirname(os.path.abspath(__file__))
web = os.path.dirname(os.path.dirname(here))
prod = os.path.join(web, 'src/assets/art/production/blender')
py = os.environ.get('S47_PY', os.path.expanduser('~/.venvs/signal47/bin/python'))
blender = os.environ.get('BLENDER', 'blender')
os.makedirs(prod, exist_ok=True); os.makedirs(os.path.join(web, 'src/assets/models'), exist_ok=True)
t0 = time.time()
def run(*cmd):
    print('>', ' '.join(cmd), flush=True)
    subprocess.run(cmd, cwd=web, check=True)

if '--no-build' not in sys.argv: run('npm', 'run', 'build:single')
for what in names:
    M = MODELS[what]
    name, size = M['name'], int(opt('--size', M['size']))
    out = os.path.join(web, 'src/assets/models', name + '.glb')
    t1 = time.time()
    run(py, os.path.join(here, 'export.py'), what, prod)
    run(blender, '-b', '--factory-startup', '--python', os.path.join(here, M['script']), '--', prod, out, prod, str(size))
    if M.get('quantize'): glbpack.quantize(out)
    for mask, mats in M['masks']:
        data = glbpack.jpeg(os.path.join(prod, mask + '.png'), quality)
        open(os.path.join(prod, mask + '.jpg'), 'wb').write(data)
        glbpack.occlusion(out, data, mats, mask)
    s = glbpack.summary(out)
    s['file_bytes'] = os.path.getsize(out)
    json.dump(s, open(os.path.join(prod, name + '_summary.json'), 'w'), indent=1)
    print(f"{os.path.relpath(out, web)}: {s['file_bytes'] / 1024:.0f} KB ({s['image_bytes'] / 1024:.0f} KB of it the masks), "
          f"{s['triangles']} triangles, {s['vertices']} vertices, meshes {s['meshes']}; {time.time() - t1:.0f} s")
print(f'all in {time.time() - t0:.0f} s')
