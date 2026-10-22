# Prepares the free models (~/Assets/signal47-free) for import_free.py, without Unreal:
#   - unpacks every Sketchfab zip into a work folder, links each Poly Haven model's files there;
#   - reads both CREDITS.txt (title, author, licence, link);
#   - measures each glTF (world bounds of its meshes, triangles, materials' alpha modes);
#   - writes a copy of the glTF whose scene has one new root node: the model scaled to its real
#     size (glTF is metres, +Y up; some exports are in centimetres or inches) and moved so its
#     pivot is at the middle of its base. Packs of separate props are written as one glTF per prop.
#   - writes plan.json for import_free.py (credits.py reads the same CREDITS.txt for the credits).
# Pure Python (the system python3 and Unreal's both run it).
# Usage: python3 -I gltf_prep.py <work folder> [--report]
import glob
import json
import math
import os
import re
import shutil
import sys
import zipfile

SRC = os.environ.get('S47_FREE', os.path.expanduser('~/Assets/signal47-free'))
HERE = os.path.dirname(os.path.abspath(__file__))

# name: (category, real size in metres of its largest side or None to trust the file, split into props)
# The sizes are what such a thing measures; a model is only rescaled when its file is off by more than
# a quarter (a unit mistake), so a model that is right keeps its author's scale.
MODELS = json.load(open(os.path.join(HERE, 'models.json')))


# ---------- glTF maths (column-major 4x4 as nested lists, rows first) ----------
def mmul(a, b):
	return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def node_matrix(n):
	if 'matrix' in n:
		m = n['matrix']
		return [[m[c * 4 + r] for c in range(4)] for r in range(4)]
	t = n.get('translation', [0, 0, 0])
	x, y, z, w = n.get('rotation', [0, 0, 0, 1])
	s = n.get('scale', [1, 1, 1])
	r = [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
		[2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
		[2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]]
	return [[r[i][0] * s[0], r[i][1] * s[1], r[i][2] * s[2], t[i]] for i in range(3)] + [[0, 0, 0, 1]]


IDENT = [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]


def measure(g, roots):
	"""World bounds (lo, hi), triangles and mesh count under the given root nodes."""
	nodes = g.get('nodes', [])
	lo, hi = [1e30] * 3, [-1e30] * 3
	tris = 0
	meshes = 0

	def walk(i, parent, depth):
		nonlocal tris, meshes
		if depth > 64:
			return
		n = nodes[i]
		w = mmul(parent, node_matrix(n))
		if 'mesh' in n:
			meshes += 1
			for p in g['meshes'][n['mesh']]['primitives']:
				a = g['accessors'][p['attributes']['POSITION']]
				mn, mx = a.get('min'), a.get('max')
				if mn and mx:
					for c in range(8):
						v = (mx[0] if c & 1 else mn[0], mx[1] if c & 2 else mn[1], mx[2] if c & 4 else mn[2])
						for k in range(3):
							x = w[k][0] * v[0] + w[k][1] * v[1] + w[k][2] * v[2] + w[k][3]
							lo[k] = min(lo[k], x)
							hi[k] = max(hi[k], x)
				if p.get('mode', 4) == 4:
					tris += (g['accessors'][p['indices']]['count'] if 'indices' in p else a['count']) // 3
		for c in n.get('children', []):
			walk(c, w, depth + 1)

	for r in roots:
		walk(r, IDENT, 0)
	return lo, hi, tris, meshes


def branch(g):
	"""The scene's roots, followed down through single empty nodes to where the model branches.
	Returns (path of matrices above the branch, the branching node ids)."""
	nodes = g.get('nodes', [])
	scene = g['scenes'][g.get('scene', 0)]
	cur, above = list(scene['nodes']), IDENT
	while len(cur) == 1 and 'mesh' not in nodes[cur[0]] and nodes[cur[0]].get('children'):
		above = mmul(above, node_matrix(nodes[cur[0]]))
		cur = list(nodes[cur[0]]['children'])
	return above, cur


def safe(s):
	s = re.sub(r'[^A-Za-z0-9]+', '_', s).strip('_')
	return s or 'part'


def read_credits():
	"""Both CREDITS.txt files: {local name: {title, author, licence, link, source}}."""
	out = {}
	ph = os.path.join(SRC, 'polyhaven', 'CREDITS.txt')
	for line in open(ph, encoding='utf-8'):
		f = [x.strip() for x in line.split(' | ')]
		if len(f) < 4:
			continue
		name = f[3].rstrip('/').rsplit('/', 1)[-1]
		out[name] = {'title': f[0], 'author': f[1], 'licence': 'CC0', 'link': f[3], 'source': 'polyhaven'}
	sf = os.path.join(SRC, 'sketchfab', 'CREDITS.txt')
	for i, line in enumerate(open(sf, encoding='utf-8')):
		# a title may itself hold " | " (Cobra 139XLR CBRadio | Low Poly, ...): read from the right
		f = [x.strip() for x in line.rstrip('\n').split(' | ')]
		if i == 0 or len(f) < 5:
			continue
		name, link, lic, author = f[-1], f[-2], f[-3], f[-4]
		title = ' | '.join(f[:-4])
		out[name] = {'title': title, 'author': author, 'licence': 'CC BY 4.0' if 'Attribution' in lic else lic, 'link': link, 'source': 'sketchfab'}
	return out


def source_gltf(name, credit, work):
	"""Unpacks or links the model into work/src/<name>; returns its .gltf path."""
	d = os.path.join(work, 'src', name)
	if credit['source'] == 'sketchfab':
		z = os.path.join(SRC, 'sketchfab', name + '.zip')
		stamp = os.path.join(d, '.from')
		key = '%d %d' % (os.path.getsize(z), int(os.path.getmtime(z)))
		if not (os.path.isfile(stamp) and open(stamp).read() == key):
			shutil.rmtree(d, ignore_errors=True)
			with zipfile.ZipFile(z) as zf:
				for m in zf.infolist():
					# never write outside the folder (a zip may hold ../ names)
					target = os.path.realpath(os.path.join(d, m.filename))
					if not target.startswith(os.path.realpath(d) + os.sep):
						raise RuntimeError('%s: unsafe path %s' % (z, m.filename))
				zf.extractall(d)
			open(stamp, 'w').write(key)
	else:
		src = os.path.join(SRC, 'polyhaven', name)
		os.makedirs(d, exist_ok=True)
		for f in os.listdir(src):
			if f.endswith('.gltf'):
				continue
			link = os.path.join(d, f)
			if not os.path.lexists(link):
				os.symlink(os.path.join(src, f), link)
		g = glob.glob(os.path.join(src, '*.gltf'))
		dst = os.path.join(d, os.path.basename(g[0]))
		shutil.copyfile(g[0], dst)
	found = sorted(glob.glob(os.path.join(d, '*.gltf')))
	found = [f for f in found if not os.path.basename(f).startswith('S47_')]
	if not found:
		raise RuntimeError('%s: no .gltf' % name)
	return found[0]


def prune(h, nodes, root, out_name):
	"""Keeps only the nodes under root, and the meshes and materials they use (importers bring in
	every node and mesh of a file, in the scene or not), with their indices renumbered."""
	order, stack = [], [root]
	while stack:
		i = stack.pop()
		order.append(i)
		stack.extend(reversed(nodes[i].get('children', [])))
	nmap = {old: new for new, old in enumerate(order)}
	meshes = sorted(set(nodes[i]['mesh'] for i in order if 'mesh' in nodes[i]))
	mmap = {old: new for new, old in enumerate(meshes)}
	mats = sorted(set(p['material'] for m in meshes for p in h['meshes'][m]['primitives'] if 'material' in p))
	tmap = {old: new for new, old in enumerate(mats)}
	out_nodes = []
	for i in order:
		n = dict(nodes[i])
		for k in ('skin', 'camera', 'weights'):
			n.pop(k, None)
		if 'children' in n:
			n['children'] = [nmap[c] for c in n['children']]
		if 'mesh' in n:
			n['mesh'] = mmap[n['mesh']]
		out_nodes.append(n)
	out_meshes = []
	for m in meshes:
		mesh = dict(h['meshes'][m])
		mesh['primitives'] = [dict(p, material=tmap[p['material']]) if 'material' in p else p for p in mesh['primitives']]
		out_meshes.append(mesh)
	h['nodes'] = out_nodes
	h['meshes'] = out_meshes
	if 'materials' in h:
		h['materials'] = json.loads(json.dumps([h['materials'][i] for i in mats]))
		# the textures (and pictures) the kept materials use: a pack's prop does not bring the others'
		infos = []

		def find(o, key=''):
			if isinstance(o, dict):
				if 'index' in o and key.lower().endswith('texture'):
					infos.append(o)
				for k, v in o.items():
					find(v, k)
			elif isinstance(o, list):
				for v in o:
					find(v, key)
		find(h['materials'])
		used = sorted(set(i['index'] for i in infos))
		xmap = {old: new for new, old in enumerate(used)}
		for i in infos:
			i['index'] = xmap[i['index']]
		textures = json.loads(json.dumps([h['textures'][i] for i in used])) if 'textures' in h else []
		sources = []
		for t in textures:
			if 'source' in t:
				sources.append(t)
			for e in t.get('extensions', {}).values():
				if isinstance(e, dict) and 'source' in e:
					sources.append(e)
		imgs = sorted(set(s['source'] for s in sources))
		imap = {old: new for new, old in enumerate(imgs)}
		for s in sources:
			s['source'] = imap[s['source']]
		if 'textures' in h:
			h['textures'] = textures
		if 'images' in h:
			h['images'] = [h['images'][i] for i in imgs]
	for k in ('skins', 'cameras'):
		h.pop(k, None)
	h['scenes'] = [{'name': out_name, 'nodes': [0]}]
	h['scene'] = 0


def write_part(g, path, part_roots, above, scale, lo, hi, out_name):
	"""A copy of g whose scene is one new root (scale, pivot at the base's middle) over part_roots."""
	h = dict(g)
	# a node may have one parent only: the part's roots leave the chain above the branch
	moved = set(part_roots)
	nodes = [dict(n, children=[c for c in n['children'] if c not in moved]) if 'children' in n else n for n in g.get('nodes', [])]
	cx, cz = (lo[0] + hi[0]) / 2, (lo[2] + hi[2]) / 2
	# the matrices above the branch are kept in a node of their own under the new root
	mid = {'name': 'S47_above', 'children': list(part_roots),
		'matrix': [above[r][c] for c in range(4) for r in range(4)]}
	nodes.append(mid)
	root = {'name': out_name, 'children': [len(nodes) - 1], 'scale': [scale] * 3,
		'translation': [-cx * scale, -lo[1] * scale, -cz * scale]}
	nodes.append(root)
	prune(h, nodes, len(nodes) - 1, out_name)
	# Interchange imports the animations too (as skeletal meshes): none of these should move
	h.pop('animations', None)
	out = os.path.join(os.path.dirname(path), 'S47_' + out_name + '.gltf')
	json.dump(h, open(out, 'w'))
	return out


def prepare(work, report=False):
	credits = read_credits()
	plan = []
	missing = sorted(set(credits) - set(MODELS))
	if missing:
		print('S47FREE no entry in models.json: ' + ', '.join(missing))
	for name in sorted(credits):
		if name not in MODELS:
			continue
		cfg = MODELS[name]
		c = credits[name]
		try:
			path = source_gltf(name, c, work)
			g = json.load(open(path, encoding='utf-8'))
		except Exception as e:  # a broken download must not stop the others
			print('S47FREE FAIL %s: %s' % (name, e))
			plan.append({'name': name, 'error': str(e)})
			continue
		filter_nodes(g, cfg)
		alpha_fix = fix_alpha(g)
		above, roots = branch(g)
		lo, hi, tris, meshes = measure(g, g['scenes'][g.get('scene', 0)]['nodes'])
		big = max(hi[k] - lo[k] for k in range(3))
		want = cfg.get('size')
		scale = 1.0
		if cfg.get('scale'):
			scale = cfg['scale']
		elif want and big > 0 and not (0.75 < want / big < 1.333):
			scale = want / big
		alpha = sorted(set(m.get('alphaMode', 'OPAQUE') for m in g.get('materials', [])))
		parts = []
		if cfg.get('split'):
			for r in roots:
				plo, phi, ptris, _ = measure(g, [r])
				if ptris == 0:
					continue
				# measured with the matrices above the branch
				wlo, whi = transform_box(above, plo, phi)
				pname = safe(g['nodes'][r].get('name', 'part%d' % r))
				pname = re.sub(r'_?(Object_\d+|fbx|obj|FBX)$', '', pname) or 'part%d' % r
				base = pname
				k = 2
				while any(p['name'] == pname for p in parts):
					pname = '%s_%d' % (base, k)
					k += 1
				parts.append({'name': pname, 'roots': [r], 'lo': wlo, 'hi': whi, 'tris': ptris})
		else:
			parts.append({'name': name, 'roots': roots, 'lo': lo, 'hi': hi, 'tris': tris})
		entry = dict(c, name=name, category=cfg['category'], scale=scale, alpha=alpha, meshes=meshes, masked=alpha_fix,
			raw_size_m=[round(hi[k] - lo[k], 4) for k in range(3)], parts=[])
		for p in parts:
			out_name = p['name'] if not cfg.get('split') else '%s_%s' % (name, p['name'])
			f = write_part(g, path, p['roots'], above, scale, p['lo'], p['hi'], safe(out_name))
			size = [(p['hi'][k] - p['lo'][k]) * scale for k in range(3)]
			# in glTF's own axes (x, depth z, height y); free_models.json has the bounds in Unreal's
			nanite = cfg.get('nanite', p['tris'] >= NANITE_TRIS and 'BLEND' not in alpha)
			entry['parts'].append({'asset': safe(out_name), 'gltf': f, 'tris': p['tris'], 'nanite': bool(nanite),
				'size_cm': {'x': round(size[0] * 100, 1), 'depth': round(size[2] * 100, 1), 'height': round(size[1] * 100, 1)}})
		plan.append(entry)
		if report:
			print('%-24s %-8s raw %7.2f x %7.2f x %7.2f m  scale %-8.4g tris %7d parts %2d %s %s' % (
				name, cfg['category'], hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2], scale, tris, len(parts),
				[p['name'] for p in parts][:6] if cfg.get('split') else '', alpha) + (' masked %d' % len(alpha_fix) if alpha_fix else '') +
				(' NANITE' if any(q['nanite'] for q in entry['parts']) else ''))
	json.dump(plan, open(os.path.join(work, 'plan.json'), 'w'), indent=1)
	return plan


NANITE_TRIS = 50000
# A translucent material that is really glass stays translucent; the rest of the BLEND materials
# (Sketchfab marks many cut-outs and plain opaque parts so) become masked at 0.5: no sorting, no
# overdraw, and Nanite takes masked materials but not translucent ones (r.Nanite.AllowTranslucency 0).
GLASS = re.compile(r'glass|glas\b|window|windshield|windscreen|lens|transparent|clear|vidrio|crystal|screen_?glass|plexi|perspex|cover_?trans|neon_?tube', re.I)


def fix_alpha(g):
	changed = []
	for m in g.get('materials', []):
		if m.get('alphaMode') != 'BLEND':
			continue
		pbr = m.get('pbrMetallicRoughness', {})
		a = pbr.get('baseColorFactor', [1, 1, 1, 1])[3]
		ext = m.get('extensions', {})
		if GLASS.search(m.get('name', '')) or a < 0.98 or 'KHR_materials_transmission' in ext:
			continue
		m['alphaMode'] = 'MASK'
		m['alphaCutoff'] = 0.5
		changed.append(m.get('name', '?'))
	return changed


def filter_nodes(g, cfg):
	"""Leaves out the mesh nodes the model's keep / drop lists exclude (they stay as empty nodes)."""
	keep, drop = cfg.get('keep'), set(cfg.get('drop', []))
	for n in g.get('nodes', []):
		if 'mesh' in n and ((keep is not None and n.get('name') not in keep) or n.get('name') in drop):
			del n['mesh']


def transform_box(m, lo, hi):
	a, b = [1e30] * 3, [-1e30] * 3
	for c in range(8):
		v = (hi[0] if c & 1 else lo[0], hi[1] if c & 2 else lo[1], hi[2] if c & 4 else lo[2])
		for k in range(3):
			x = m[k][0] * v[0] + m[k][1] * v[1] + m[k][2] * v[2] + m[k][3]
			a[k] = min(a[k], x)
			b[k] = max(b[k], x)
	return a, b


if __name__ == '__main__':
	prepare(sys.argv[1], '--report' in sys.argv)
