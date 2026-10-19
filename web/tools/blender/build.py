# The Blender models, rebuilt from the game by one command (from web/):
#   python3 tools/blender/build.py [saro] [truck] [--size N] [--quality 88] [--no-build]
# (no name: both). For each:
# 1. the single-file build (npm run build:single, once), so the export sees the game as it is now
# 2. tools/blender/export.py: the game's own geometry to glTF (the house's boxes with what stands
#    round them and the site's lamps; the truck at the origin)
# 3. the model's script in Blender (headless): modelled, two UV sets, the masks (and the house's
#    lamps) baked with Cycles on the graphics card (OptiX), exported as .glb (saro_house.py, truck.py)
# 4. the check of the flat faces (glbpack.check): a big flat face with a bent normal (a seam
#    across a wall) or cut across two islands of the lightmap stops the build
# 5. the pictures as JPEGs into the .glb (each mask 4:4:4 as its materials' occlusion texture; the
#    house's lamps and its rooms as emissive textures, the truck's shade under it as a colour
#    map), what the game must know of the bake in the materials' extras; the truck's normals and
#    lightmap coordinates quantised (glbpack.py)
# Out: src/assets/models/<name>.glb (shipped, loaded only with ?blender: world/blenderModels.ts);
# the export, the pictures and the .blend in src/assets/art/production/blender/ (not shipped).
# BLENDER (default `blender`) and S47_PY (the Python with Playwright, default
# ~/.venvs/signal47/bin/python) can be set.
import json, os, subprocess, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import glbpack

TRUCK = {'body_paint', 'body_chrome', 'body_dark', 'shell_paint', 'shell_chrome', 'shell_dark'}
# per model: its pictures (file, a mask or a colour picture, the materials, the slot, the UV set:
# None for the lightmap's) and the extras (a JSON the script writes, the materials it goes to)
MODELS = {
    'saro': {'script': 'saro_house.py', 'name': 'saro_house', 'size': 1024, 'quantize': True,
             'pictures': [('saro_house_mask', True, {'house', 'wing', 'skirt'}, 'occlusionTexture', None),
                          ('saro_house_light', True, {'house', 'wing'}, 'emissiveTexture', None),
                          ('saro_house_rooms', False, {'glass_lit'}, 'emissiveTexture', 0)],
             'extras': ('saro_house_lamps', {'house', 'wing'})},
    'truck': {'script': 'truck.py', 'name': 'saro_truck', 'size': 1024, 'quantize': True,
              'pictures': [('saro_truck_mask', True, TRUCK, 'occlusionTexture', None),
                           ('saro_truck_wheel_mask', True, {'wheel'}, 'occlusionTexture', None),
                           ('saro_truck_shadow', True, {'shadow'}, 'baseColorTexture', 0)]},
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
    final = os.path.join(web, 'src/assets/models', name + '.glb')
    out = os.path.join(prod, name + '.new.glb')        # (the shipped one is replaced only when all is well)
    t1 = time.time()
    run(py, os.path.join(here, 'export.py'), what, prod)
    run(blender, '-b', '--factory-startup', '--python', os.path.join(here, M['script']), '--', prod, out, prod, str(size))
    problems = glbpack.check(out)
    if problems:
        print('\n'.join(problems[:20]))
        raise SystemExit(f'{name}: {len(problems)} flat faces are not flat or not whole in the lightmap (glbpack.check): the model is not written over')
    print(f'{name}: the flat faces are flat and whole in the lightmap')
    if M.get('quantize'): glbpack.quantize(out)
    for pic, mask, mats, slot, tc in M['pictures']:
        data = glbpack.jpeg(os.path.join(prod, pic + '.png'), quality, mask)
        open(os.path.join(prod, pic + '.jpg'), 'wb').write(data)
        glbpack.texture(out, data, mats, pic, slot, tc)
    if M.get('extras'):
        f, mats = M['extras']
        glbpack.extras(out, mats, json.load(open(os.path.join(prod, f + '.json'))))
    os.replace(out, final); out = final
    s = glbpack.summary(out)
    s['file_bytes'] = os.path.getsize(out)
    json.dump(s, open(os.path.join(prod, name + '_summary.json'), 'w'), indent=1)
    print(f"{os.path.relpath(out, web)}: {s['file_bytes'] / 1024:.0f} KB ({s['image_bytes'] / 1024:.0f} KB of it the masks), "
          f"{s['triangles']} triangles, {s['vertices']} vertices, meshes {s['meshes']}; {time.time() - t1:.0f} s")
print(f'all in {time.time() - t0:.0f} s')
