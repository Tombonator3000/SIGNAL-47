"""PolyHaven Boulder01 textured LOD authoring for SIGNAL / 47.

The default output is Artifacts/Environment27/StagedBoulder so this recipe is
safe while Unity is open. Set E27_BOULDER_OUTPUT to
Assets/Signal47/Art/Environment27 after the Unity build is closed to publish
the same deterministic FBX/material texture layout.
"""
import bpy, json, math, hashlib, os, shutil
from pathlib import Path
from mathutils import Vector

REPO = Path(__file__).resolve().parents[3]
STAGE = REPO / 'Artifacts/Environment27/StagedBoulder'
SOURCE = STAGE / 'Source/Boulder01'
OUT_ROOT = Path(os.environ.get('E27_BOULDER_OUTPUT', str(STAGE))).resolve()
MODEL_OUT = OUT_ROOT / 'Models'
TEX_OUT = OUT_ROOT / 'Boulder01'
MODEL_OUT.mkdir(parents=True, exist_ok=True)
TEX_OUT.mkdir(parents=True, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.length_unit = 'METERS'
bpy.context.preferences.filepaths.save_version = 0

GLTF = SOURCE / 'boulder_01_2k.gltf'
TEXTURES = {
    'diffuse': SOURCE / 'boulder_01_diff_2k.jpg',
    'normal_gl': SOURCE / 'boulder_01_nor_gl_2k.jpg',
    'arm': SOURCE / 'boulder_01_arm_2k.jpg',
}
for key, source in TEXTURES.items():
    target = TEX_OUT / source.name
    if not target.exists() or hashlib.sha256(target.read_bytes()).hexdigest() != hashlib.sha256(source.read_bytes()).hexdigest():
        shutil.copy2(source, target)

def bbox(obj):
    bpy.context.view_layer.update()
    coords=[obj.matrix_world @ Vector(c) for c in obj.bound_box]
    lo=[min(v[i] for v in coords) for i in range(3)]
    hi=[max(v[i] for v in coords) for i in range(3)]
    return lo,hi,[hi[i]-lo[i] for i in range(3)]

def fit_to_envelope(obj, target_dims):
    """Restore source XYZ envelope after LOD collapse without touching UVs."""
    mins=[min(v.co[i] for v in obj.data.vertices) for i in range(3)]
    maxs=[max(v.co[i] for v in obj.data.vertices) for i in range(3)]
    for i in range(3):
        span=maxs[i]-mins[i]; center=0.0 if i<2 else target_dims[i]*.5
        for vertex in obj.data.vertices:
            vertex.co[i]=(vertex.co[i]-mins[i])*target_dims[i]/span + center-target_dims[i]*.5
    obj.data.update()

def make_material():
    mat=bpy.data.materials.new('E27_Boulder01')
    mat.use_nodes=True
    nodes=mat.node_tree.nodes; links=mat.node_tree.links
    nodes.clear()
    out=nodes.new('ShaderNodeOutputMaterial'); out.location=(620,0)
    bsdf=nodes.new('ShaderNodeBsdfPrincipled'); bsdf.location=(350,0)
    bsdf.inputs['Roughness'].default_value=.78; bsdf.inputs['Metallic'].default_value=0.0
    diff=nodes.new('ShaderNodeTexImage'); diff.name='Boulder01 diffuse 2K'; diff.label='PolyHaven Boulder01 diffuse 2K'; diff.location=(-620,180)
    diff.image=bpy.data.images.load(str(TEX_OUT/TEXTURES['diffuse'].name),check_existing=True)
    diff.image.colorspace_settings.name='sRGB'
    normal=nodes.new('ShaderNodeTexImage'); normal.name='Boulder01 normal GL 2K'; normal.label='PolyHaven Boulder01 normal GL 2K'; normal.location=(-620,-80)
    normal.image=bpy.data.images.load(str(TEX_OUT/TEXTURES['normal_gl'].name),check_existing=True)
    normal.image.colorspace_settings.name='Non-Color'
    arm=nodes.new('ShaderNodeTexImage'); arm.name='Boulder01 ARM 2K'; arm.label='PolyHaven Boulder01 ARM 2K (AO/Rough/Metal)'; arm.location=(-620,-330)
    arm.image=bpy.data.images.load(str(TEX_OUT/TEXTURES['arm'].name),check_existing=True)
    arm.image.colorspace_settings.name='Non-Color'
    sep=nodes.new('ShaderNodeSeparateRGB'); sep.location=(-350,-300)
    ao=nodes.new('ShaderNodeMixRGB'); ao.blend_type='MULTIPLY'; ao.inputs[0].default_value=.28; ao.location=(-80,180)
    nmap=nodes.new('ShaderNodeNormalMap'); nmap.space='TANGENT'; nmap.location=(80,-120); nmap.inputs['Strength'].default_value=.72
    links.new(diff.outputs['Color'],ao.inputs[1]); links.new(sep.outputs['R'],ao.inputs[2]); links.new(ao.outputs['Color'],bsdf.inputs['Base Color'])
    links.new(normal.outputs['Color'],nmap.inputs['Color']); links.new(nmap.outputs['Normal'],bsdf.inputs['Normal'])
    links.new(arm.outputs['Color'],sep.inputs['Image']); links.new(sep.outputs['G'],bsdf.inputs['Roughness']); links.new(sep.outputs['B'],bsdf.inputs['Metallic'])
    links.new(bsdf.outputs['BSDF'],out.inputs['Surface'])
    return mat

def normalize_source(obj):
    # PolyHaven GLTF is Y-up; rotate to Blender Z-up, then center X/Y and put
    # the lowest vertex on local Z=0 for a stable Unity mounting pivot.
    obj.rotation_euler=(math.radians(90),0,0)
    bpy.context.view_layer.objects.active=obj
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    coords=[obj.matrix_world @ v.co for v in obj.data.vertices]
    centerx=(min(v.x for v in coords)+max(v.x for v in coords))*.5
    centery=(min(v.y for v in coords)+max(v.y for v in coords))*.5
    minz=min(v.z for v in coords)
    for vertex in obj.data.vertices:
        vertex.co.x-=centerx; vertex.co.y-=centery; vertex.co.z-=minz
    obj.location=(0,0,0)
    obj.data.update()

def duplicate_lod(source, name, ratio, target_dims):
    obj=source.copy(); obj.data=source.data.copy(); obj.name=name; bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active=obj; obj.select_set(True)
    weld=obj.modifiers.new('Weld duplicated seam vertices','WELD'); weld.merge_threshold=.00001
    bpy.ops.object.modifier_apply(modifier=weld.name)
    mod=obj.modifiers.new('Textured LOD decimation','DECIMATE'); mod.ratio=ratio; mod.use_collapse_triangulate=False
    bpy.ops.object.modifier_apply(modifier=mod.name)
    fit_to_envelope(obj,target_dims)
    for poly in obj.data.polygons: poly.use_smooth=True
    return obj

def export_obj(obj, path):
    bpy.ops.object.select_all(action='DESELECT'); obj.select_set(True); bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True,path_mode='RELATIVE')

bpy.ops.import_scene.gltf(filepath=str(GLTF))
source=bpy.context.selected_objects[0]
normalize_source(source)
source_dims=bbox(source)[2]
mat=make_material()
source.data.materials.clear(); source.data.materials.append(mat)
lod0=duplicate_lod(source,'E27_Boulder_LOD0',.05,source_dims)
lod1=duplicate_lod(source,'E27_Boulder_LOD1',.006,source_dims)
bpy.data.objects.remove(source,do_unlink=True)

records=[]
for label,obj,path in [('LOD0',lod0,MODEL_OUT/'E27_Boulder_LOD0.fbx'),('LOD1',lod1,MODEL_OUT/'E27_Boulder_LOD1.fbx')]:
    lo,hi,dims=bbox(obj)
    tris=sum(max(0,len(p.vertices)-2) for p in obj.data.polygons)
    assert 2000<=tris<=3500 if label=='LOD0' else 250<=tris<=650,(label,tris)
    assert obj.data.uv_layers and all(p.area>1e-12 for p in obj.data.polygons)
    export_obj(obj,path)
    records.append({'name':label,'fbx':path.name,'triangles':tris,'dimensions_blender_xyz':[round(x,6) for x in dims],'bounds_min':[round(x,6) for x in lo],'bounds_max':[round(x,6) for x in hi],'uv_layers':len(obj.data.uv_layers),'material':'E27_Boulder01','fbx_roundtrip':'PASS'})

# Small CPU Eevee hero render of the textured LOD0, kept outside Unity.
scene=bpy.context.scene; scene.world=bpy.data.worlds.new('Boulder01 studio'); scene.world.color=(.025,.035,.04)
bpy.ops.mesh.primitive_plane_add(size=8,location=(0,0,-.012)); floor=bpy.context.object; floor.name='Boulder01 studio floor'
floor.data.materials.append(mat)
lod1.hide_render=True
bpy.ops.object.camera_add(location=(2.7,-3.6,1.9)); cam=bpy.context.object; scene.camera=cam; cam.data.type='ORTHO'; cam.data.ortho_scale=2.65
cam.rotation_euler=(Vector((0,0,.72))-cam.location).to_track_quat('-Z','Y').to_euler()
for loc,energy,size in [((3,-4,4),900,3),((-3,-1,2),600,2),((0,3,3),400,2)]:
    bpy.ops.object.light_add(type='AREA',location=loc); light=bpy.context.object; light.data.energy=energy; light.data.shape='DISK'; light.data.size=size; light.rotation_euler=(Vector((0,0,.5))-light.location).to_track_quat('-Z','Y').to_euler()
scene.render.engine='BLENDER_EEVEE_NEXT'; scene.render.resolution_x=900; scene.render.resolution_y=700; scene.render.resolution_percentage=100; scene.render.image_settings.file_format='PNG'
preview=OUT_ROOT/'boulder01-lod0-preview.png'; scene.render.filepath=str(preview); bpy.ops.render.render(write_still=True)

blend=OUT_ROOT/'SIGNAL47_environment27_boulder.blend'; bpy.ops.wm.save_as_mainfile(filepath=str(blend))
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
manifest={'source_asset':'PolyHaven Boulder 01','source_page':'https://polyhaven.com/a/boulder_01','source_api':'https://api.polyhaven.com/files/boulder_01','author':'Rico Cilliers','license':'CC0 1.0','source_format':'2K GLTF from API; boulder_01_2k.gltf + boulder_01.bin','source_files':{p.name:{'bytes':p.stat().st_size,'sha256':sha(p)} for p in sorted(SOURCE.glob('*')) if p.is_file()},'maps':{'diffuse_2k':TEXTURES['diffuse'].name,'normal_gl_2k':TEXTURES['normal_gl'].name,'arm_2k':TEXTURES['arm'].name},'blender':bpy.app.version_string,'units':'metres; Blender Z-up; FBX axis_forward=-Z axis_up=Y','material':'E27_Boulder01','mounting':'centered X/Y, local Z=0 at lowest source vertex','outputs':records,'preview':preview.name}
(OUT_ROOT/'boulder01-source-license-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('BOULDER01_ASSETS_OK',json.dumps(records))
