"""Prepare the CC0 Poly Haven camera without replacing the downloaded source."""
import bpy, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
ART=ROOT/'Unity/Assets/Signal47/Art/ThirdParty/PolyHaven/Camera_01'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system='METRIC'
bpy.ops.import_scene.fbx(filepath=str(ROOT/'Artifacts/Research/Camera08/Camera_01-original.fbx'))
camera=bpy.data.objects['Camera_01']
# Keep the loose strap editable, but do not export a hanging strap through the shelf.
for material in camera.data.materials:
    material.use_nodes=True
    nodes=material.node_tree.nodes; nodes.clear()
    output=nodes.new('ShaderNodeOutputMaterial'); shader=nodes.new('ShaderNodeBsdfPrincipled')
    material.node_tree.links.new(shader.outputs['BSDF'],output.inputs['Surface'])
    part=material.name.removeprefix('Camera_01_')
    if part=='lens':
        shader.inputs['Base Color'].default_value=(.045,.09,.12,1);shader.inputs['Metallic'].default_value=.65;shader.inputs['Roughness'].default_value=.12
        continue
    for suffix, socket in [('diff','Base Color'),('roughness','Roughness'),('metallic','Metallic')]:
        texture=nodes.new('ShaderNodeTexImage');texture.image=bpy.data.images.load(str(ART/(part+'_'+suffix+'.jpg')),check_existing=True)
        if suffix!='diff':texture.image.colorspace_settings.name='Non-Color'
        material.node_tree.links.new(texture.outputs['Color'],shader.inputs[socket])
    texture=nodes.new('ShaderNodeTexImage');texture.image=bpy.data.images.load(str(ART/(part+'_nor_gl.png')),check_existing=True);texture.image.colorspace_settings.name='Non-Color'
    normal=nodes.new('ShaderNodeNormalMap');material.node_tree.links.new(texture.outputs['Color'],normal.inputs['Color']);material.node_tree.links.new(normal.outputs['Normal'],shader.inputs['Normal'])
bpy.ops.object.select_all(action='DESELECT');camera.select_set(True);bpy.context.view_layer.objects.active=camera
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
# Place the origin on the base and retain native metre scale / authored UVs.
corners=[camera.matrix_world@Vector(c) for c in camera.bound_box]
bpy.context.scene.cursor.location=((min(v.x for v in corners)+max(v.x for v in corners))/2,(min(v.y for v in corners)+max(v.y for v in corners))/2,min(v.z for v in corners))
bpy.ops.object.origin_set(type='ORIGIN_CURSOR');camera.location=(0,0,0)
bpy.ops.export_scene.fbx(filepath=str(ART/'FieldCamera08.fbx'),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True,path_mode='STRIP')
report=dict(blender=bpy.app.version_string,source='https://polyhaven.com/a/Camera_01',license='CC0-1.0',objects=1,triangles=sum(len(p.vertices)-2 for p in camera.data.polygons),uv_layers=len(camera.data.uv_layers),dimensions_m=list(camera.dimensions),materials=[m.name for m in camera.data.materials],changes='Base origin, metre scale; loose strap retained in editable source but omitted from runtime export. Original geometry and UVs preserved.')
(ART/'blender-verification.json').write_text(json.dumps(report,indent=2))
for image in bpy.data.images:
    if image.source=='FILE':image.filepath=bpy.path.relpath(image.filepath,start=str(ROOT/'Unity/Blender'))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Unity/Blender/SIGNAL47_field_camera.blend'))
print('FIELD_CAMERA_BLENDER_PASS',json.dumps(report))
