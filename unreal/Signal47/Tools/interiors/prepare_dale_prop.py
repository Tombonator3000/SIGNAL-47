"""Normalize the selected Tripo office prop in Blender, preserving its atlas.

blender -b --python prepare_dale_prop.py -- chair
Source exports remain in Tom's private workspace, outside public Git.
"""
from pathlib import Path
import json, math, os, sys
import bpy
import numpy as np
from mathutils import Vector, Matrix

key = sys.argv[sys.argv.index('--') + 1] if '--' in sys.argv else 'chair'
specs = {'chair': ('SM_Dale_Chair_H31', .95), 'books': ('SM_Dale_BookStack_H31', .24)}
name, height = specs[key]
root = Path(os.environ.get('S47_DALE_MODEL_DIR', str(Path.home() / 'Documents/ChatGPT/Signal 47/miljografikk/2026-10-30-dale-realisme/models')))
source = root / key
fbx, = source.glob('*.fbx')
out = source / 'prepared'
out.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1
bpy.ops.import_scene.fbx(filepath=str(fbx))
objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
assert objects and all(o.data.uv_layers for o in objects)
bpy.ops.object.select_all(action='DESELECT')
for o in objects: o.select_set(True)
bpy.context.view_layer.objects.active = objects[0]
bpy.ops.object.join()
obj = bpy.context.object
# Bake the vendor's FBX axes once. The existing Unreal FBX recipe receives the
# same metre/Z-up profile as our other authored props, without axis guessing.
obj.data.transform(obj.matrix_world)
obj.matrix_world = Matrix.Identity(4)
points = np.array([v.co[:] for v in obj.data.vertices])
low, high = points.min(axis=0), points.max(axis=0)
assert high[2] > low[2]
factor = height / (high[2] - low[2])
centre = Vector(((low[0]+high[0])/2, (low[1]+high[1])/2, low[2]))
for v in obj.data.vertices: v.co = (v.co-centre) * factor
if key == 'chair':
    # The upper backrest defines the chair's back. Face the desk (+Three Z,
    # Blender -Y), so its back is visible from the office doorway.
    back = [v.co for v in obj.data.vertices if v.co.z > height*.78]
    x, y = sum(v.x for v in back)/len(back), sum(v.y for v in back)/len(back)
    obj.data.transform(Matrix.Rotation(math.pi/2-math.atan2(y,x), 4, 'Z'))
    points = np.array([v.co[:] for v in obj.data.vertices])
    centre = Vector(((points[:,0].min()+points[:,0].max())/2,
                     (points[:,1].min()+points[:,1].max())/2, points[:,2].min()))
    for v in obj.data.vertices: v.co -= centre
obj.name = name
obj.data.uv_layers[0].name = 'S47_Atlas'
obj.data.materials.clear()
mat = bpy.data.materials.new('M_'+name.removeprefix('SM_'))
obj.data.materials.append(mat)
for poly in obj.data.polygons: poly.material_index = 0
tri = obj.modifiers.new('DeliveryTriangles','TRIANGULATE')
bpy.ops.object.modifier_apply(modifier=tri.name)
textures = {}
for role, suffix in [('base_color','basecolor'),('roughness','roughness'),('metallic','metallic'),('normal','normal')]:
    files = [p for p in source.rglob('*') if p.is_file() and p.parent != out and p.stem.endswith('_'+suffix)]
    original, = files
    im = bpy.data.images.load(str(original), check_existing=False)
    im.colorspace_settings.name = 'sRGB' if role == 'base_color' else 'Non-Color'
    im.scale(2048,2048)
    if role == 'normal':
        # Tripo's Blender export is OpenGL tangent space. Our Unreal recipe uses
        # DirectX maps. This is a data conversion, not painted lighting.
        px = np.empty(2048*2048*4,dtype=np.float32)
        im.pixels.foreach_get(px)
        px[1::4] = 1-px[1::4]
        im.pixels.foreach_set(px)
    target = out / (role+'.png')
    im.filepath_raw = str(target); im.file_format = 'PNG'; im.save()
    bpy.data.images.remove(im)
    textures[role] = target.name
bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')), use_selection=True,
    object_types={'MESH'}, apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
    axis_forward='-Y', axis_up='Z', bake_anim=False, mesh_smooth_type='FACE', use_tspace=True)
points = [v.co[:] for v in obj.data.vertices]
data = {'name':name, 'triangles':len(obj.data.polygons), 'height_m':height,
        'bounds_blender_m':[[min(p[i] for p in points) for i in range(3)], [max(p[i] for p in points) for i in range(3)]],
        'material':{'name':mat.name, 'roughness':.65, 'metallic':0, 'normal_strength':1, 'textures':textures}}
(out/'prop.json').write_text(json.dumps(data,indent=2)+'\n')
bpy.ops.wm.save_as_mainfile(filepath=str(out/(name+'.blend')))
print('DALE PROP READY',key,data['triangles'],data['bounds_blender_m'])
