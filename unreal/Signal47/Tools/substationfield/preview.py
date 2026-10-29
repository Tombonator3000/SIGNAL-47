"""Authoring preview: field kit with the existing original kits and the MEASURED native ground as context.
Context objects are never exported. Not game lighting; game screenshots remain the acceptance views.
"""
import bpy, json, math, sys
from pathlib import Path
from mathutils import Vector
R = Path(__file__).resolve().parent
sys.path.insert(0, str(R))
bpy.ops.wm.open_mainfile(filepath=str(R / 'Substation_Field.blend'))
scene = bpy.context.scene
ctx = bpy.data.collections.new('PREVIEW_CONTEXT'); scene.collection.children.link(ctx)
for folder, filename in [('substation', 'Caliche_Junction_Substation.blend'), ('substationdetail', 'Substation_Detail.blend')]:
    contract = json.loads((R.parent / folder / 'model_contract.json').read_text())
    with bpy.data.libraries.load(str(R.parent / folder / filename), link=False) as (a, b):
        b.objects = [n for n in contract['parts'] if n in a.objects]
    for ob in b.objects:
        if ob: ctx.objects.link(ob); ob.hide_render = False; ob.hide_set(False); ob.location = (0, 0, 0)
# Measured ground (world Three grid) -> B metres, for honest contact review.
G = json.loads((R / 'ground_probe.json').read_text())
verts, faces = [], []
nx, nz = G['nx'], G['nz']
for j in range(nz):
    for i in range(nx):
        x = G['x0'] + i * G['step']; z = G['z0'] + j * G['step']
        verts.append((7890 - x, z - 536.2, G['y'][j][i] + 1.10))
for j in range(nz - 1):
    for i in range(nx - 1):
        a = j * nx + i; faces.append((a, a + nx, a + nx + 1, a + 1))
me = bpy.data.meshes.new('PREVIEW_MEASURED_GROUND'); me.from_pydata(verts, [], faces); me.update()
gob = bpy.data.objects.new('PREVIEW_MEASURED_GROUND', me); ctx.objects.link(gob)
mat = bpy.data.materials.new('PREVIEW_DESERT'); mat.use_nodes = True
mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (.42, .31, .21, 1)
mat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = .95; me.materials.append(mat)
scene.world.use_nodes = True
scene.world.node_tree.nodes['Background'].inputs[0].default_value = (.55, .65, .78, 1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value = .5
bpy.ops.object.light_add(type='SUN', location=(3, -7, 15)); sun = bpy.context.object
sun.rotation_euler = (math.radians(55), math.radians(-25), math.radians(-35)); sun.data.energy = 3.2; sun.data.angle = .05
bpy.ops.object.camera_add(); cam = bpy.context.object; scene.camera = cam
scene.render.engine = 'CYCLES'; scene.cycles.samples = 24; scene.cycles.use_denoising = True
scene.render.resolution_x = 1280; scene.render.resolution_y = 720
out = R / 'Preview'; out.mkdir(exist_ok=True)
# B coords: world (7887.5, 522.8) eye 1.6 m above ground -> E06-like approach.
views = [('e06', (2.5, -13.4, 1.30), (5.5, -8.6, .30), 32),
         ('culvert', (7.2, -11.6, 0.90), (8.0, -8.5, .05), 40),
         ('fixture', (2.6, -10.6, 1.45), (4.0, -9.0, .80), 38),
         ('reference', (12.6, -17.6, 1.40), (9.8, -9.5, .40), 30),
         ('grade', (1.0, -17.0, 4.5), (5.2, -9.5, -.1), 30)]
for name, at, target, lens in views:
    cam.location = at; cam.rotation_euler = (Vector(target) - Vector(at)).to_track_quat('-Z', 'Y').to_euler(); cam.data.lens = lens
    scene.render.filepath = str(out / (name + '.png')); bpy.ops.render.render(write_still=True)
print('SUBSTATION_FIELD_PREVIEW_COMPLETE')
