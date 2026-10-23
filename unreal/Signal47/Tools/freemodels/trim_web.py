"""Cuts an exported building out of the merged web geometry (Unreal only), for a model of Tom's own
that takes its place (models.json "trim": the Caliche gas station).

The web version merges a group's static parts by material (kit.ts mergeStatic): the old gas station's
walls share their mesh with the rest of the town's cream walls, so it cannot be hidden by its node. Here
every triangle of the named group's meshes whose three corners lie inside one of the boxes (the
group's own metres, x0 x1 y0 y1 z0 z1) is left out, the materials named in "keep" excepted (the
apron's drape under the canopy). The meshes that lose triangles are written to one .glb, one node per
mesh named as the export names it (g<id>), and a report: { "g<id>": { "removed": n, "tris": the
original's triangles } }. S47Props swaps them in for the exported meshes of the same name under the
group (and only where the original still has that many triangles: a new export needs a new trim).

python3 -I trim_web.py EXPORT_DIR OUT_DIR '<trim json>'   (gltf_prep.py calls trim() itself)
"""
import json
import os
import struct
import sys


def _load(path):
	raw = open(path, 'rb').read()
	n = struct.unpack_from('<I', raw, 12)[0]
	return json.loads(raw[20:20 + n]), raw[28 + n:]


def _values(j, binary, index):
	a = j['accessors'][index]
	v = j['bufferViews'][a['bufferView']]
	fmt = {5121: 'B', 5123: 'H', 5125: 'I', 5126: 'f'}[a['componentType']]
	width = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
	stride = v.get('byteStride', struct.calcsize(fmt) * width)
	start = v.get('byteOffset', 0) + a.get('byteOffset', 0)
	return [struct.unpack_from('<' + fmt * width, binary, start + k * stride) for k in range(a['count'])]


def trim(cfg, export, out):
	"""cfg: {"group": handle, "keep": [material handles], "boxes": [[x0, x1, y0, y1, z0, z1], ...]}.
	Returns (glb path, report) or (None, {}) when nothing is cut."""
	man = json.load(open(os.path.join(export, 'manifest.json')))
	h = man['handles']
	group = h[cfg['group']]['node']
	keep = {h[k]['mat'] for k in cfg.get('keep', []) if k in h}
	boxes = cfg['boxes']
	geos = {}
	for n in man['nodes']:
		if n.get('p') == group and n.get('geo') is not None and not (set(n.get('mats', [])) & keep):
			geos['g%d' % n['geo']] = n['geo']

	def inside(p):
		return any(b[0] <= p[0] <= b[1] and b[2] <= p[1] <= b[3] and b[4] <= p[2] <= b[5] for b in boxes)

	report, chosen = {}, []
	os.makedirs(out, exist_ok=True)
	for old in os.listdir(out):
		if old.startswith('trim_') or old == 'trim.json':
			os.remove(os.path.join(out, old))
	for f in man['geoFiles']:
		if not any(f['from'] <= g < f['to'] for g in geos.values()):
			continue
		j, binary = _load(os.path.join(export, 'geo', f['file']))
		binary = bytearray(binary)
		for mi, mesh in enumerate(j['meshes']):
			if mesh.get('name') not in geos:
				continue
			removed = tris = 0
			for prim in mesh['primitives']:
				pos = _values(j, bytes(binary), prim['attributes']['POSITION'])
				idx = [v[0] for v in _values(j, bytes(binary), prim['indices'])]
				tris += len(idx) // 3
				kept = []
				for k in range(0, len(idx), 3):
					t = idx[k:k + 3]
					if all(inside(pos[i]) for i in t):
						removed += 1
					else:
						kept.extend(t)
				if len(kept) == len(idx) or not kept:
					continue
				while len(binary) % 4:
					binary += b'\0'
				off = len(binary)
				binary += struct.pack('<' + 'I' * max(1, len(kept)), *(kept or [0]))
				j['bufferViews'].append({'buffer': 0, 'byteOffset': off, 'byteLength': 4 * max(1, len(kept)), 'target': 34963})
				acc = dict(j['accessors'][prim['indices']])
				acc.update(bufferView=len(j['bufferViews']) - 1, byteOffset=0, componentType=5125, count=len(kept) or 3,
					min=[min(kept or [0])], max=[max(kept or [0])])
				j['accessors'].append(acc)
				prim['indices'] = len(j['accessors']) - 1
			if removed == tris:
				# all of it: the node is hidden instead (no empty mesh to import)
				report[mesh['name']] = {'removed': removed, 'tris': tris, 'hidden': True}
			elif removed:
				report[mesh['name']] = {'removed': removed, 'tris': tris}
				chosen.append((j, binary, mi))
		# one file per source file's meshes: written below once all are known
		if chosen and chosen[-1][0] is j:
			_write_part(j, binary, [c[2] for c in chosen if c[0] is j], out, f['file'])
	if not report:
		return None, {}
	parts = sorted(p for p in os.listdir(out) if p.startswith('trim_') and p.endswith('.glb'))
	json.dump({'report': report, 'files': parts, 'cfg': cfg}, open(os.path.join(out, 'trim.json'), 'w'), indent=1)
	return [os.path.join(out, p) for p in parts], report


def _write_part(j, binary, mesh_ids, out, src):
	os.makedirs(out, exist_ok=True)
	j = dict(j)
	j['meshes'] = [j['meshes'][i] for i in mesh_ids]
	j['nodes'] = [{'name': m['name'], 'mesh': k} for k, m in enumerate(j['meshes'])]
	j['scenes'] = [{'nodes': list(range(len(j['meshes'])))}]
	j['scene'] = 0
	for k in ('animations', 'skins', 'cameras'):
		j.pop(k, None)
	binary = bytes(binary) + b'\0' * (-len(binary) % 4)
	j['buffers'] = [dict(j['buffers'][0], byteLength=len(binary))]
	data = json.dumps(j, separators=(',', ':')).encode()
	data += b' ' * (-len(data) % 4)
	glb = struct.pack('<III', 0x46546c67, 2, 28 + len(data) + len(binary))
	glb += struct.pack('<II', len(data), 0x4e4f534a) + data
	glb += struct.pack('<II', len(binary), 0x004e4942) + binary
	open(os.path.join(out, 'trim_' + os.path.splitext(src)[0] + '.glb'), 'wb').write(glb)


if __name__ == '__main__':
	files, rep = trim(json.loads(sys.argv[3]), sys.argv[1], sys.argv[2])
	print(json.dumps(rep, indent=1))
