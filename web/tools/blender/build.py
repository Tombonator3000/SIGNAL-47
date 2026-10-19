# The Blender models, rebuilt from the game by one command (from web/):
#   python3 tools/blender/build.py saro [--size 2048] [--quality 88] [--no-build]
# 1. the single-file build (npm run build:single), so the export sees the game as it is now
# 2. tools/blender/export.py: the game's own boxes to glTF, with what stands round them
# 3. tools/blender/saro_house.py in Blender (headless): modelled, two UV sets, the mask baked
#    with Cycles on the graphics card (OptiX), exported as .glb
# 4. the mask as a 4:4:4 JPEG into the .glb as its occlusion texture (glbpack.py)
# Out: src/assets/models/<name>.glb (shipped, loaded only with ?blender: world/blenderModels.ts);
# the export, the mask and the .blend in src/assets/art/production/blender/ (not shipped).
# BLENDER (default `blender`) and S47_PY (the Python with Playwright, default
# ~/.venvs/signal47/bin/python) can be set.
import json, os, subprocess, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import glbpack

MODELS = {'saro': ('saro_house.py', 'saro_house')}
what = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith('-') else 'saro'
opt = lambda k, d: sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d
size, quality = int(opt('--size', 2048)), int(opt('--quality', 88))
script, name = MODELS[what]
here = os.path.dirname(os.path.abspath(__file__))
web = os.path.dirname(os.path.dirname(here))
prod = os.path.join(web, 'src/assets/art/production/blender')
out = os.path.join(web, 'src/assets/models', name + '.glb')
py = os.environ.get('S47_PY', os.path.expanduser('~/.venvs/signal47/bin/python'))
blender = os.environ.get('BLENDER', 'blender')
os.makedirs(prod, exist_ok=True); os.makedirs(os.path.dirname(out), exist_ok=True)
t0 = time.time()
def run(*cmd):
    print('>', ' '.join(cmd), flush=True)
    subprocess.run(cmd, cwd=web, check=True)

if '--no-build' not in sys.argv: run('npm', 'run', 'build:single')
run(py, os.path.join(here, 'export.py'), what, prod)
run(blender, '-b', '--factory-startup', '--python', os.path.join(here, script), '--', prod, out, prod, str(size))
data = glbpack.jpeg(os.path.join(prod, name + '_mask.png'), quality)
open(os.path.join(prod, name + '_mask.jpg'), 'wb').write(data)
glbpack.occlusion(out, data, {'house', 'wing'}, name + '_mask')
s = glbpack.summary(out)
s['file_bytes'] = os.path.getsize(out)
json.dump(s, open(os.path.join(prod, name + '_summary.json'), 'w'), indent=1)
print(f"{os.path.relpath(out, web)}: {s['file_bytes'] / 1024:.0f} KB ({s['image_bytes'] / 1024:.0f} KB of it the {size} px mask), "
      f"{s['triangles']} triangles, {s['vertices']} vertices, meshes {s['meshes']}; {time.time() - t0:.0f} s")
