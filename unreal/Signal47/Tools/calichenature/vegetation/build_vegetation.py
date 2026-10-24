"""Original Caliche kit. Guarded Blender 5.2, two CPU threads, no implicit render."""
import sys,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
import bpy
from mathutils import Vector
from geometry import SPECS,build
import subprocess,os
bundled=Path('/home/tom/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3')
texture_python=os.environ.get('S47_VEGETATION_PYTHON',str(bundled) if bundled.is_file() else 'python3')
if '--reuse-textures' not in sys.argv:subprocess.run([texture_python,str(ROOT/'make_textures.py')],check=True)
assert (ROOT/'textures'/'botanical_color.png').read_bytes()[25]==6, 'CN3 requires generated RGBA atlas; do not reuse old RGB CN2 texture'
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
bpy.context.preferences.filepaths.save_version=0
EX=ROOT/'exports';EX.mkdir(exist_ok=True)
materials={};rows={}
for name,stem in [('M_CN_Foliage','botanical'),('M_CN_Bark','bark'),('M_CN_Stone','botanical')]:
 mat=bpy.data.materials.new(name);mat.use_nodes=True;mat.use_backface_culling=name!='M_CN_Foliage';mat.surface_render_method='DITHERED';p=mat.node_tree.nodes.get('Principled BSDF');p.inputs['Metallic'].default_value=0;p.inputs['Roughness'].default_value=.84;p.inputs['Specular IOR Level'].default_value=.22
 for role,socket in [('color','Base Color'),('roughness','Roughness'),('normal_gl','Normal')]:
  node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=bpy.data.images.load(str(ROOT/'textures'/(stem+'_'+role+'.png')),check_existing=True)
  if role=='color':color_node=node
  else:node.image.colorspace_settings.name='Non-Color'
  if role=='normal_gl':
   normal=mat.node_tree.nodes.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=.20;mat.node_tree.links.new(node.outputs['Color'],normal.inputs['Color']);mat.node_tree.links.new(normal.outputs['Normal'],p.inputs[socket])
  else:mat.node_tree.links.new(node.outputs['Color'],p.inputs[socket])
 if name=='M_CN_Foliage':
  clip=mat.node_tree.nodes.new('ShaderNodeMath');clip.operation='GREATER_THAN';clip.inputs[1].default_value=.333;clip.label='CN3 binary foliage cutout, glTF MASK'
  mat.node_tree.links.new(color_node.outputs['Alpha'],clip.inputs[0]);mat.node_tree.links.new(clip.outputs[0],p.inputs['Alpha'])
 materials[name]=mat
 rows[name]={'base_color_linear':[1,1,1,1],'metallic':0,'roughness':.84,'specular':.22,'normal_strength':.20,'uv_scale':1,'alpha':1,'alpha_mode':'opaque','two_sided':name=='M_CN_Foliage','emission':[0,0,0,1],'emission_strength':0,'textures':{'base_color':f'textures/{stem}_color.png','roughness':f'textures/{stem}_roughness.png','normal':f'textures/{stem}_normal_dx.png'},'normal_convention':'DirectX for native, explicit OpenGL map in Blender/GLB'}
 if name=='M_CN_Foliage':rows[name].update({'alpha_mode':'masked','opacity_from_base_color':True,'alpha_cutoff':.333,'mip_alpha_coverage':True})

def object_from(data,collection):
 mesh=bpy.data.meshes.new(data.name);mesh.from_pydata(data.vertices,[],data.faces);mesh.update();o=bpy.data.objects.new(data.name,mesh);collection.objects.link(o)
 slots=sorted(set(data.material))
 for name in slots:mesh.materials.append(materials[name])
 for p,name in zip(mesh.polygons,data.material):p.material_index=slots.index(name);p.use_smooth=name=='M_CN_Foliage'
 uv=mesh.uv_layers.new(name='UVMap')
 for loop in mesh.loops:uv.data[loop.index].uv=data.uv[loop.vertex_index]
 o['original_asset']='SIGNAL / 47 original Caliche vegetation';o['ground_pivot_m']=[0.,0.,0.]
 return o

def select(o):
 bpy.ops.object.select_all(action='DESELECT');o.hide_set(False);o.select_set(True);bpy.context.view_layer.objects.active=o

def export_fbx(o):
 select(o);bpy.ops.export_scene.fbx(filepath=str(EX/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')

main=bpy.data.collections.new('CALICHE_LOD0_gallery');scene.collection.children.link(main)
low=bpy.data.collections.new('AUTHORED_LOD1_LOD2');scene.collection.children.link(low)
editable=bpy.data.collections.new('EDITABLE_original_components');scene.collection.children.link(editable)
parts={};gallery=[]
for index,spec in enumerate(SPECS):
 lods=[]
 for lod in range(3):
  data=build(spec,lod);o=object_from(data,main if lod==0 else low);o['lod']=lod;o['family']=spec[1]
  stat=data.stats();assert stat['minimum_triangle_area_m2']>=5e-9,(data.name,stat)
  export_fbx(o)
  select(o);bpy.ops.export_scene.gltf(filepath=str(EX/(o.name+'.glb')),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
  stat.update({'name':o.name,'fbx':'exports/'+o.name+'.fbx','glb':'exports/'+o.name+'.glb','lod_index':lod,'origin':[0,0,0],'uv_layers':1});lods.append(stat)
  if lod==0:
   gallery.append(o);o.location=((index%4)*4.6,(index//4)*5.2,0)
   # Component source meshes remain physically distinct and fully editable.
   for ci,component in enumerate(data.components):
    end=data.components[ci+1]['face_start'] if ci+1<len(data.components) else len(data.faces)
    fs=data.faces[component['face_start']:end]
    if not fs:continue
    used=sorted({v for f in fs for v in f});lookup={v:i for i,v in enumerate(used)}
    from geometry import Mesh
    src=Mesh('SRC_'+spec[0]+'_'+component['name']+'_'+str(ci),[data.vertices[v] for v in used],[tuple(lookup[v] for v in f) for f in fs],[data.uv[v] for v in used],data.material[component['face_start']:end])
    object_from(src,editable)
  else:o.hide_set(True)
 parts['SM_CN_'+spec[0]]={**lods[0],'family':spec[1],'seed':spec[2],'design_height_m':spec[3],'design_width_m':spec[4],'lods':lods,'materials':lods[0]['materials']}
low.hide_render=True;editable.hide_render=True;editable.hide_viewport=True
bpy.ops.object.select_all(action='DESELECT')
for o in gallery:o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(ROOT/'caliche_vegetation.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
for image in bpy.data.images:
 if image.source=='FILE':image.filepath=bpy.path.relpath(image.filepath,start=str(ROOT))
scene.world=bpy.data.worlds.new('Preview_world');scene.world.color=(.08,.08,.08)
scene.render.engine='CYCLES';scene.render.threads_mode='FIXED';scene.render.threads=2;scene.cycles.samples=12
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'caliche_vegetation.blend'))
(ROOT/'materials.json').write_text(json.dumps(rows,indent=2))
(ROOT/'model_contract.json').write_text(json.dumps({'asset':'Original Caliche botanical and stone prototype kit','source_revision':'CN3-masked-small-leaf-needle-sprays','authoring_units':'metres, Blender Z up, local ground pivot0','fbx_axes':{'forward':'-Y','up':'Z'},'runtime_axis_probe':'Use same -Y/Z conversion proven by radio station; native owner validates actual bounds.','parts':parts,'alpha_strategy':'Original perforated RGBA microfoliage on curved 3D sprays; two-sided binary MASK cutoff0.333; grass/yucca/husk/stone tiles alpha1; no translucent or whole-plant billboard.','validation_status':'SOURCE_EXPORTED_NOT_YET_ROUNDTRIP_VERIFIED'},indent=2))
print('CALICHE_KIT_EXPORTED',json.dumps({n:[v['triangles'] for v in row['lods']] for n,row in parts.items()}))
