"""Blender CLI: deterministic texture-sampled Gaussians, NOT trained/reconstructed 3DGS.

Input is a locally generated GLB. Output uses explicit Unity RUF coordinates, a
matched mesh baseline, and an open front doorway. Source bytes are never changed.
"""
import bpy
import hashlib
import json
import math
from pathlib import Path
import struct
import sys
import numpy as np
from mathutils import Vector

source, destination = [Path(p).resolve() for p in sys.argv[sys.argv.index('--')+1:]]
destination.mkdir(parents=True, exist_ok=False)
source_bytes = source.read_bytes()
source_hash = hashlib.sha256(source_bytes).hexdigest()
json_length = struct.unpack_from('<I', source_bytes, 12)[0]
gltf = json.loads(source_bytes[20:20+json_length])
bin_start = 20+json_length+8
image = gltf['images'][0]
assert image['mimeType'] == 'image/jpeg'
view = gltf['bufferViews'][image['bufferView']]
texture_bytes = source_bytes[bin_start+view.get('byteOffset',0):bin_start+view.get('byteOffset',0)+view['byteLength']]
(destination/'station-texture.jpg').write_bytes(texture_bytes)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source))
objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
assert len(objects) == 1, 'This reviewed candidate adapter expects one mesh/material/texture.'
obj = objects[0]
mesh = obj.data
mesh.calc_loop_triangles()
assert len(mesh.materials) == 1
texture = next(n.image for n in mesh.materials[0].node_tree.nodes if n.type == 'TEX_IMAGE' and n.image)
# Access the original encoded color values; the splat renderer handles gamma conversion.
texture.colorspace_settings.name = 'Non-Color'
texels = np.empty(texture.size[0]*texture.size[1]*4, dtype=np.float32)
texture.pixels.foreach_get(texels)
texels = texels.reshape(texture.size[1],texture.size[0],4)
points = np.array([tuple(obj.matrix_world @ v.co) for v in mesh.vertices])
low, high = points.min(axis=0), points.max(axis=0)
size = high-low
positions = np.stack(((points[:,1]-(low[1]+high[1])/2)*8/size[1],
                     (points[:,2]-low[2])*2.8/size[2],
                     (high[0]-points[:,0])*6/size[0]), axis=1)
uv_layer = mesh.uv_layers.active
assert uv_layer is not None

def clip(poly, axis, value, less):
    """Split an interpolated position/UV polygon by a plane, returning inside/outside."""
    inside, outside = [], []
    if not poly: return inside, outside
    previous = poly[-1]
    prev_in = previous[axis] <= value if less else previous[axis] >= value
    for current in poly:
        curr_in = current[axis] <= value if less else current[axis] >= value
        if prev_in != curr_in:
            t = (value-previous[axis])/(current[axis]-previous[axis])
            point = previous + t*(current-previous)
            inside.append(point); outside.append(point)
        (inside if curr_in else outside).append(current)
        previous, prev_in = current, curr_in
    return inside, outside

triangles = []
cropped = 0
def emit(poly):
    for i in range(1,len(poly)-1):
        t = np.array([poly[0],poly[i],poly[i+1]])
        if np.linalg.norm(np.cross(t[1,:3]-t[0,:3], t[2,:3]-t[0,:3])) > 1e-10:
            triangles.append(t)

for tri in mesh.loop_triangles:
    # Blender -> Unity mapping reverses handedness: swap indices for outward faces.
    poly = [np.concatenate((positions[mesh.loops[i].vertex_index], np.array(uv_layer.data[i].uv))) for i in (tri.loops[0],tri.loops[2],tri.loops[1])]
    xyz = np.array(poly)[:,:3]
    if xyz[:,0].max() < -.70 or xyz[:,0].min() > .70 or xyz[:,1].min() > 2.15 or xyz[:,2].min() > .65:
        emit(poly); continue
    cropped += 1
    # Subtract doorway volume; no new fake back face or collision surface is added.
    remainder = poly
    for axis, value, less in [(0,-.70,False),(0,.70,True),(1,-.2,False),(1,2.15,True),(2,-.5,False),(2,.65,True)]:
        remainder, outside = clip(remainder,axis,value,less)
        emit(outside)
        if not remainder: break

tris = np.array(triangles)
crosses = np.cross(tris[:,1,:3]-tris[:,0,:3],tris[:,2,:3]-tris[:,0,:3])
areas = np.linalg.norm(crosses,axis=1)/2
normals = crosses/(areas[:,None]*2)
flat = tris.reshape(-1,5)
mesh_out = {'positions': flat[:,:3].ravel().round(7).tolist(),
            'normals': np.repeat(normals,3,axis=0).ravel().round(7).tolist(),
            'uv': flat[:,3:].ravel().round(7).tolist(),
            'triangles': list(range(len(flat)))}
(destination/'station-mesh.json').write_text(json.dumps(mesh_out,separators=(',',':'))+'\n')

count = 150000
rng = np.random.default_rng(470023)
selected = rng.choice(len(tris),size=count,p=areas/areas.sum())
r1, r2 = np.sqrt(rng.random(count)), rng.random(count)
weights = np.stack((1-r1,r1*(1-r2),r1*r2),axis=1)
samples = (tris[selected]*weights[:,:,None]).sum(axis=1)
uv = samples[:,3:]
tx = np.clip(uv[:,0]*(texture.size[0]-1),0,texture.size[0]-1)
ty = np.clip(uv[:,1]*(texture.size[1]-1),0,texture.size[1]-1)
x0,y0 = np.floor(tx).astype(int),np.floor(ty).astype(int)
x1,y1 = np.minimum(x0+1,texture.size[0]-1),np.minimum(y0+1,texture.size[1]-1)
fx,fy = (tx-x0)[:,None],(ty-y0)[:,None]
rgb = (texels[y0,x0,:3]*(1-fx)+texels[y0,x1,:3]*fx)*(1-fy)+(texels[y1,x0,:3]*(1-fx)+texels[y1,x1,:3]*fx)*fy
n = normals[selected]
# Rotating local +Z to the surface normal gives flattened surface Gaussians.
q = np.stack((1+n[:,2],-n[:,1],n[:,0],np.zeros(count)),axis=1)
singular = np.linalg.norm(q,axis=1) < 1e-6
q[singular] = [0,1,0,0]
q /= np.linalg.norm(q,axis=1)[:,None]
radius = math.sqrt(float(areas.sum())/count)*.7
rows = np.zeros((count,17),dtype='<f4')
rows[:,:3] = samples[:,:3]
rows[:,6:9] = (rgb-.5)/.28209479177387814
rows[:,9] = math.log(.95/.05)
rows[:,10:13] = [math.log(radius),math.log(radius),math.log(.003)]
rows[:,13:] = q
names = ['x','y','z','nx','ny','nz','f_dc_0','f_dc_1','f_dc_2','opacity','scale_0','scale_1','scale_2','rot_0','rot_1','rot_2','rot_3']
header = 'ply\nformat binary_little_endian 1.0\ncomment Deterministic texture sampled mesh; not trained or captured 3DGS. Unity RUF.\nelement vertex '+str(count)+'\n'+''.join('property float '+s+'\n' for s in names)+'end_header\n'
with (destination/'station-surface-splats.ply').open('wb') as f:
    f.write(header.encode());f.write(rows.tobytes())

audit = {'source_sha256': source_hash, 'source_creation': 'mE1DdyxhJQ',
         'source_type': 'Magnific Tripo v3.1 image-generated textured GLB',
         'output_type': 'Deterministically surface-sampled Gaussians, not trained 3DGS or a Marble world',
         'axes': 'Unity RUF; generated model +Blender X facade becomes Unity -Z facade',
         'normalization_meters': {'width':8,'depth':6,'height':2.8},
         'original_vertices': len(mesh.vertices), 'original_triangles': len(mesh.loop_triangles),
         'output_triangles': len(tris), 'doorway_triangles_considered': cropped,
         'doorway_cut': {'x':[-.70,.70],'y':[-.2,2.15],'z':[-.5,.65]},
         'surface_area_m2': float(areas.sum()), 'splats':count,'seed':470023,
         'tangent_radius_m':radius,'normal_radius_m':.003, 'opacity':.95,
         'texture': {'size':list(texture.size),'sample':'bilinear original encoded sRGB values','bytes_unchanged_from_glb':True},
         'files': {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in destination.iterdir() if p.is_file()}}
assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash
(destination/'conversion.json').write_text(json.dumps(audit,indent=2)+'\n')
print(json.dumps(audit,indent=2))
