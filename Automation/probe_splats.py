#!/usr/bin/env python3
"""Original synthetic Gaussian-splat format probe; never modifies Unity assets."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import time


def make_fixture(path):
    # Author Y-up, then encode PlayCanvas's PLY convention (Z rotation 180°).
    # This is a metre-scale synthetic floor and box, not a photographic scan.
    points = []
    for i in range(20):
        for j in range(20):
            points.append((-.95 + i*.1, 0, -.95 + j*.1,
                           .25 + i*.005, .3 + j*.003, .32))
    for axis in range(3):
        others = [a for a in range(3) if a != axis]
        for sign in (-1, 1):
            for i in range(8):
                for j in range(8):
                    xyz = [0., 0., 0.]
                    xyz[axis] = sign*.35
                    xyz[others[0]] = -.35 + i*.1
                    xyz[others[1]] = -.35 + j*.1
                    xyz[1] += .42
                    points.append((*xyz, .65 + axis*.05, .32 + i*.008, .12 + j*.006))
    names = ['x', 'y', 'z', 'nx', 'ny', 'nz', 'f_dc_0', 'f_dc_1',
             'f_dc_2', 'opacity', 'scale_0', 'scale_1', 'scale_2',
             'rot_0', 'rot_1', 'rot_2', 'rot_3']
    header = ('ply\nformat binary_little_endian 1.0\n'
              'comment SIGNAL47 original synthetic format fixture; not a scan\n'
              f'element vertex {len(points)}\n' +
              ''.join(f'property float {n}\n' for n in names) + 'end_header\n')
    with path.open('wb') as f:
        f.write(header.encode('ascii'))
        for x, y, z, r, g, b in points:
            f.write(struct.pack('<17f', -x, -y, z, 0, 0, 0,
                                *((c-.5)/.28209479177387814 for c in (r, g, b)),
                                math.log(.95/.05), *([math.log(.06)]*3), 1, 0, 0, 0))
    return len(points)


def read_ply(path):
    data = path.read_bytes()
    end = data.index(b'end_header\n') + len(b'end_header\n')
    lines = data[:end].decode('ascii').splitlines()
    if 'format binary_little_endian 1.0' not in lines:
        raise ValueError('Probe expects binary little-endian float PLY')
    count = int(next(x for x in lines if x.startswith('element vertex ')).split()[-1])
    props = [x.split() for x in lines if x.startswith('property ')]
    if not all(x[1] == 'float' for x in props):
        raise ValueError('Unexpected non-float property')
    names = [x[-1] for x in props]
    stride = len(names)*4
    if len(data)-end != count*stride:
        raise ValueError('Unexpected PLY payload size')
    return [dict(zip(names, struct.unpack_from(f'<{len(names)}f', data, end+i*stride)))
            for i in range(count)]


def glb_summary(path):
    data = path.read_bytes()
    magic, version, total = struct.unpack_from('<4sII', data)
    size, kind = struct.unpack_from('<I4s', data, 12)
    if magic != b'glTF' or version != 2 or total != len(data) or kind != b'JSON':
        raise ValueError('Invalid GLB container')
    j = json.loads(data[20:20+size])
    primitives = [p for m in j.get('meshes', []) for p in m['primitives']]
    return {'extensions_used': j.get('extensionsUsed', []),
            'extensions_required': j.get('extensionsRequired', []),
            'primitives': len(primitives),
            'modes': [p.get('mode', 4) for p in primitives],
            'indexed_triangles': sum(j['accessors'][p['indices']]['count']//3
                                     for p in primitives if p.get('mode', 4) == 4 and 'indices' in p)}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--node', required=True, type=Path)
    p.add_argument('--cli', required=True, type=Path)
    p.add_argument('--output', required=True, type=Path)
    p.add_argument('--gpu', default='0')
    a = p.parse_args()
    a.node, a.cli, a.output = a.node.resolve(), a.cli.resolve(), a.output.resolve()
    a.output.mkdir(parents=True, exist_ok=False)
    result = {'scope': 'synthetic format/renderer/collision probe; no Unity integration or FPS test',
              'expected_count': make_fixture(a.output/'fixture.ply'), 'runs': [], 'checks': {}}

    def run(label, *args):
        start = time.monotonic()
        cmd = [str(a.node), str(a.cli), *args]
        try:
            r = subprocess.run(cmd, cwd=a.output, capture_output=True, text=True, timeout=60)
            stdout, stderr, code = r.stdout, r.stderr, r.returncode
        except subprocess.TimeoutExpired as e:
            stdout, stderr, code = str(e.stdout or ''), str(e.stderr or ''), 124
        (a.output/f'{label}.stdout.txt').write_text(stdout)
        (a.output/f'{label}.stderr.txt').write_text(stderr)
        result['runs'].append({'label': label, 'arguments': args, 'exit_code': code,
                               'elapsed_seconds': round(time.monotonic()-start, 3)})
        print(f'{label}: exit {code}', flush=True)
        return code == 0

    run('version', '--version')
    run('gpu-list', '--list-gpus')
    run('source-info', 'fixture.ply', '--info', 'json', 'null')
    sog_ok = run('compress-cpu', '-g', 'cpu', 'fixture.ply', 'fixture.sog')
    if sog_ok and run('decompress', 'fixture.sog', 'roundtrip.ply'):
        src, dst = read_ply(a.output/'fixture.ply'), read_ply(a.output/'roundtrip.ply')
        finite = all(math.isfinite(v) for row in dst for v in row.values())
        def distance(u, v):
            return math.sqrt(sum((u[k]-v[k])**2 for k in ('x', 'y', 'z')))
        # Bidirectional nearest-point distance tolerates Morton reordering and
        # duplicate points at face edges, but detects missing spatial regions.
        max_error = max(max(min(distance(u, v) for v in dst) for u in src),
                        max(min(distance(v, u) for u in src) for v in dst))
        result['roundtrip'] = {'source_count': len(src), 'output_count': len(dst),
                               'finite_values': finite, 'max_position_error_m': max_error,
                               'tolerance_m': .001}
        result['checks']['roundtrip'] = len(src) == len(dst) == result['expected_count'] and finite and max_error < .001
    if run('splat-glb', 'fixture.ply', 'fixture.glb'):
        g = glb_summary(a.output/'fixture.glb')
        result['splat_glb'] = g
        result['checks']['splat_extension'] = 'KHR_gaussian_splatting' in g['extensions_used']
    if run('collision', '-g', a.gpu, 'fixture.ply', '--voxel-size', '0.1',
           '--voxel-opacity', '0.3', '--collision-mesh', 'faces', 'fixture.voxel.json'):
        g = glb_summary(a.output/'fixture.collision.glb')
        result['collision_glb'] = g
        result['checks']['triangle_collision'] = g['indexed_triangles'] > 0 and all(m == 4 for m in g['modes']) and 'KHR_gaussian_splatting' not in g['extensions_used']
    run('render', '-g', a.gpu, 'fixture.sog', '--camera-pos', '2,1.7,-2',
        '--camera-target', '0,0.25,0', '--resolution', '640x480',
        '--background', '0.035,0.045,0.065', 'preview.webp')
    result['files'] = {f.name: {'bytes': f.stat().st_size,
                              'sha256': hashlib.sha256(f.read_bytes()).hexdigest()}
                       for f in sorted(a.output.iterdir()) if f.is_file()}
    result['all_commands_succeeded'] = all(r['exit_code'] == 0 for r in result['runs'])
    result['automated_checks_passed'] = len(result['checks']) == 3 and all(result['checks'].values())
    (a.output/'verification.json').write_text(json.dumps(result, indent=2)+'\n')
    return 0 if result['all_commands_succeeded'] and result['automated_checks_passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
