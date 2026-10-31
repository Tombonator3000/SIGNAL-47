"""Pack a baked OpenVDB density into linear PNG slices, without rendering.

Run inside the existing Blender Python environment (numpy + openvdb), or call
pack_cache from bake_clouds.py. No simulator/editor process is launched here.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import sys
import zlib

import numpy as np


def log(message):
    print('S47 CLOUD PACK ' + message, flush=True)


def write_png(path, pixels, bit_depth=8):
    """Linear 8/16-bit grayscale PNG. PNG's 16-bit samples are big endian."""
    if bit_depth not in (8,16):
        raise ValueError('PNG bit depth must be 8 or 16')
    pixels = np.ascontiguousarray(pixels, dtype=np.uint8 if bit_depth == 8 else '>u2')
    height, width = pixels.shape
    def chunk(kind, data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    rows = b''.join(b'\0'+row.tobytes() for row in pixels)
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+
                     chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,bit_depth,0,0,0,0))+
                     chunk(b'IDAT',zlib.compress(rows,6))+chunk(b'IEND',b''))


def load_density(path, grid_name='density'):
    try:
        import openvdb as vdb
    except ImportError:
        import pyopenvdb as vdb
    loaded = vdb.readAll(str(path))
    grids = loaded[0] if isinstance(loaded, tuple) else loaded
    matches = [grid for grid in grids if grid.name == grid_name]
    if len(matches) != 1:
        raise RuntimeError('Expected exactly one %s grid in %s; found %s' % (grid_name,path,[g.name for g in grids]))
    grid = matches[0]
    low, high = [np.array(c, dtype=np.int64) for c in grid.evalActiveVoxelBoundingBox()]
    dims = high-low+1
    if np.any(dims <= 0) or int(np.prod(dims)) > 256**3:
        raise RuntimeError('Empty or unexpectedly large density dimensions: ' + str(dims))
    xyz = np.zeros(tuple(int(v) for v in dims), dtype=np.float32)
    grid.copyToArray(xyz, ijk=tuple(int(v) for v in low))
    if not np.isfinite(xyz).all() or float(xyz.max()) <= .000001:
        raise RuntimeError('Density grid is empty/nonfinite: ' + str(path))
    # Verify the binding's real numpy axis order against independent VDB reads.
    # Include its densest voxel, so all-zero corners cannot hide an axis swap.
    accessor = grid.getConstAccessor()
    probes = [np.unravel_index(int(np.argmax(xyz)), xyz.shape),
              (0,0,0), tuple((dims//2).tolist()),
              tuple(((dims-1)*np.array([.23,.57,.81])).astype(int).tolist())]
    for local in probes:
        ijk = tuple(int(low[k]+local[k]) for k in range(3))
        if not np.isclose(float(xyz[local]), float(accessor.getValue(ijk)), atol=1e-6, rtol=1e-5):
            raise RuntimeError('OpenVDB numpy XYZ ordering differs at ' + str(ijk))
    transform = {}
    try:
        transform = dict(index_origin_world=list(grid.transform.indexToWorld((0,0,0))),
                         voxel_size=list(grid.transform.voxelSize()))
    except (AttributeError, TypeError):
        pass
    return xyz.transpose(2,1,0).copy(), dict(source=str(path), grid=grid_name,
            active_min_xyz=low.tolist(), active_max_xyz=high.tolist(),
            actual_dimensions_xyz=dims.tolist(), array_order_verified='XYZ against VDB accessor', **transform)


def resample_axis(values, positions, axis):
    lo = np.floor(positions).astype(np.int64)
    hi = lo+1
    weight = (positions-lo).astype(np.float32)
    shape = [1,1,1]
    shape[axis] = len(positions)
    low = np.take(values,np.clip(lo,0,values.shape[axis]-1),axis=axis)
    high = np.take(values,np.clip(hi,0,values.shape[axis]-1),axis=axis)
    low *= ((lo >= 0)&(lo < values.shape[axis])).reshape(shape)
    high *= ((hi >= 0)&(hi < values.shape[axis])).reshape(shape)
    return low*(1-weight.reshape(shape))+high*weight.reshape(shape)


def fit_cube(values, source_min, cube_min, edge, size):
    result = values
    for xyz_axis, array_axis in ((0,2),(1,1),(2,0)):
        positions = (cube_min[xyz_axis]-.5+(np.arange(size)+.5)*edge/size-source_min[xyz_axis])
        result = resample_axis(result,positions,array_axis)
    result = np.maximum(result,0).astype(np.float32)
    # The shared two-voxel empty margin is retained as a hard zero at the border.
    result[[0,-1],:,:] = 0
    result[:,[0,-1],:] = 0
    result[:,:,[0,-1]] = 0
    return result


def slice_atlas(cube, columns):
    size = cube.shape[0]
    if cube.shape != (size,size,size) or size % columns:
        raise ValueError('Expected a cubic grid with an integral tile count')
    rows = size//columns
    atlas = cube.reshape(rows,columns,size,size).transpose(0,2,1,3).reshape(rows*size,columns*size)
    restored = atlas.reshape(rows,size,columns,size).transpose(0,2,1,3).reshape(size,size,size)
    if not np.array_equal(restored,cube):
        raise RuntimeError('Slice atlas XYZ/Z tile ordering did not roundtrip')
    return atlas


def pack_cache(cache, output, frames, size=128, bit_depth=None, cache_kind='data'):
    bit_depth = (16 if size == 256 else 8) if bit_depth is None else bit_depth
    if bit_depth not in (8,16):
        raise ValueError('Atlas bit depth must be 8 or 16')
    if cache_kind not in ('data','noise'):
        raise ValueError('Cache kind must be data or noise')
    # DNA_fluid_types.h names the upres grid density_noise and its cache
    # fluid_noise. Do not fall back to the base density when noise was requested.
    prefix = 'fluid_'+cache_kind+'_'
    grid_name = 'density_noise' if cache_kind == 'noise' else 'density'
    cache, output = Path(cache).resolve(), Path(output).resolve()
    if output.exists():
        raise FileExistsError('Fresh atlas directory required: ' + str(output))
    candidates = list(cache.rglob(prefix+'*.vdb'))
    if not candidates:
        raise FileNotFoundError('No '+prefix+'*.vdb files under '+str(cache))
    source = []
    for frame in frames:
        matches = [p for p in candidates if (m := re.search(r'(\d+)\.vdb$',p.name)) and int(m.group(1)) == frame]
        if len(matches) != 1:
            raise RuntimeError('Expected one data VDB for frame %d, found %s' % (frame,matches))
        log('READ kind=%s grid=%s frame=%d file=%s' % (cache_kind,grid_name,frame,matches[0]))
        values, metadata = load_density(matches[0],grid_name)
        metadata['frame'] = frame
        source.append((values,metadata))
    low = np.min([meta['active_min_xyz'] for _,meta in source],axis=0)-2
    high = np.max([meta['active_max_xyz'] for _,meta in source],axis=0)+2
    edge = int(np.max(high-low+1))
    cube_min = low-(edge-(high-low+1))//2
    # Shared spatial bounds and density normalization prevent breathing/jumping
    # between selected frames. Empty space is padded, never stretched per axis.
    cubes = [fit_cube(values,np.array(meta['active_min_xyz']),cube_min,edge,size) for values,meta in source]
    density_scale = max(float(cube.max()) for cube in cubes)
    if density_scale <= .000001:
        raise RuntimeError('All fitted density cubes are empty')
    output.mkdir(parents=True,exist_ok=False)
    columns = 8 if size == 64 else 16
    manifest = dict(schema=1, authorship='Original SIGNAL / 47 Mantaflow density',
        cache_kind=cache_kind,density_grid=grid_name,
        dimensions_xyz=[size,size,size], tiles_xy=[columns,size//columns],
        png_size_xy=[columns*size,(size//columns)*size], linear=True, channels=1, bit_depth=bit_depth,
        density_decode='sample.r * density_scale', density_scale=density_scale,
        array_order='density[z,y,x]; x varies fastest',
        atlas_order='tile=(z % columns,z // columns); PNG row inside tile is increasing y; no flips',
        source_cube_min_xyz=cube_min.tolist(), source_cube_edge_voxels=edge,
        source_sampling='isotropic trilinear fit into shared cube, two source voxels of empty padding',
        border='outermost texels are zero; texture address clamp; sRGB must be false', frames=[])
    for cube, (_,metadata) in zip(cubes,source):
        frame = metadata['frame']
        name = 'density-%04d' % frame
        np.save(output/(name+'.npy'),cube,allow_pickle=False)
        levels = 255 if bit_depth == 8 else 65535
        quantized = np.rint(np.clip(cube/density_scale,0,1)*levels).astype(np.uint8 if bit_depth == 8 else np.uint16)
        atlas = slice_atlas(quantized,columns)
        write_png(output/(name+'.png'),atlas,bit_depth)
        # Cheap source preview: front / side / top orthographic maximum-density
        # projections, with height up in the first two. This is not a render.
        preview = np.concatenate((np.flipud(quantized.max(axis=2)),
                                  np.flipud(quantized.max(axis=1)),
                                  np.flipud(quantized.max(axis=0))),axis=1)
        preview = np.rint(preview.astype(np.float32)*(255/levels)).astype(np.uint8)
        write_png(output/(name+'-BLENDER-density-projections.png'),preview)
        metadata.update(png=name+'.png', float_cube=name+'.npy',
                        source_preview=name+'-BLENDER-density-projections.png',
                        source_preview_bit_depth=8,
                        source_preview_type='Blender Mantaflow density MIP, front/side/top; not a render or Unreal image',
                        fitted_peak=float(cube.max()), occupied_voxels=int(np.count_nonzero(quantized)))
        manifest['frames'].append(metadata)
        log('WROTE frame=%d atlas=%dx%d grid=%d^3 bit_depth=%d source_xyz=%s occupied=%d' %
            (frame,atlas.shape[1],atlas.shape[0],size,bit_depth,metadata['actual_dimensions_xyz'],metadata['occupied_voxels']))
    (output/'atlas.json').write_text(json.dumps(manifest,indent=2)+'\n')
    log('COMPLETE ' + str(output/'atlas.json'))
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--frames', type=int, nargs='+', required=True)
    parser.add_argument('--size', type=int, choices=(64,128,192,256), default=128)
    parser.add_argument('--bit-depth', type=int, choices=(8,16), help='Default: 16 for size 256, otherwise 8')
    parser.add_argument('--cache-kind', choices=('data','noise'), default='data')
    argv = sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else sys.argv[1:]
    args = parser.parse_args(argv)
    # Nine picks are opt-in through the explicit frame list; existing three-pick callers stay unchanged.
    if not 1 <= len(args.frames) <= 9:
        parser.error('Select one to nine frames')
    pack_cache(args.cache,args.output,sorted(set(args.frames)),args.size,args.bit_depth,args.cache_kind)


if __name__ == '__main__':
    main()
