"""UE 5.8 import of original static props, PBR, LODs and video-compatible screen.
Destination: /Game/Kelvora. Run with legacy FBX importer (Interchange FBX flag off).
Only assets under the destination are created/updated. Game maps/code are untouched.
"""
import unreal,json,traceback,os
from pathlib import Path
R=Path(os.environ.get('S47_KELVORA_SOURCE', str(Path.home()/'Desktop/Retro TV og radio')));DEST='/Game/Kelvora';REPORT=R/'Verification/unreal-import.json'
T=unreal.AssetToolsHelpers.get_asset_tools();E=unreal.EditorAssetLibrary;ML=unreal.MaterialEditingLibrary;SUB=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
report={'engine':unreal.SystemLibrary.get_engine_version(),'checks':{},'assets':[]}
def save():REPORT.write_text(json.dumps(report,indent=2))
def ck(name,ok,detail=None):
 report['checks'][name]={'pass':bool(ok),'detail':detail};save();assert ok,name

def imp(path,name,opts=None):
 t=unreal.AssetImportTask();t.filename=str(path);t.destination_path=DEST;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
 if opts:t.options=opts;t.factory=unreal.FbxFactory()
 T.import_asset_tasks([t]);a=[unreal.load_asset(p) for p in t.imported_object_paths];ck(name+'_imported',bool(a));return a[0]
def tex(name,role):
 t=imp(R/'Textures'/(name+'.png'),name);t.set_editor_property('srgb',role=='color')
 if role=='normal':t.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('flip_green_channel',False)
 elif role=='masks':t.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS)
 E.save_loaded_asset(t);return t

def mat(name):
 m=unreal.load_asset(DEST+'/'+name) or T.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew());ML.delete_all_material_expressions(m);return m

def node(m,cls,**kw):
 n=ML.create_material_expression(m,cls)
 for k,v in kw.items():n.set_editor_property(k,v)
 return n
def connect(a,b,inp,out=''):
 ok=ML.connect_material_expressions(a,out,b,inp)
 if not ok:raise RuntimeError('Material connection failed: '+str(a.get_class().get_name())+' -> '+str(b.get_class().get_name())+'.'+inp)
 return ok

def sample(m,t,role='color',param=None):
 cls=unreal.MaterialExpressionTextureSampleParameter2D if param else unreal.MaterialExpressionTextureSample
 n=node(m,cls,texture=t,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role=='normal' else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS if role=='masks' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
 if param:n.set_editor_property('parameter_name',param)
 return n
try:
 E.make_directory(DEST)
 meshes={}
 for label,expected,height in [('TV',3,41.55),('Radio',2,41.65)]:
  name='SM_Kelvora_'+label;ui=unreal.FbxImportUI();ui.automated_import_should_detect_type=False;ui.import_as_skeletal=False;ui.import_mesh=True;ui.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;ui.import_materials=False;ui.import_textures=False
  d=ui.static_mesh_import_data;d.set_editor_property('combine_meshes',True);d.set_editor_property('auto_generate_collision',False);d.set_editor_property('generate_lightmap_u_vs',True);d.convert_scene=True;d.convert_scene_unit=True;d.force_front_x_axis=True;d.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
  mesh=imp(R/'Unreal'/(name+'.fbx'),name,ui);ck(name+'_static_mesh',isinstance(mesh,unreal.StaticMesh));ck(name+'_material_slots',len(mesh.get_editor_property('static_materials'))==expected,len(mesh.get_editor_property('static_materials')));ck(name+'_height',abs(mesh.get_bounds().box_extent.z*2-height)<.2,float(mesh.get_bounds().box_extent.z*2));ck(name+'_cabinet_collision',SUB.get_simple_collision_count(mesh)>0,SUB.get_simple_collision_count(mesh))
  for lod in [1,2]:ck(name+'_LOD'+str(lod),SUB.import_lod(mesh,lod,str(R/'Unreal'/(name+'_LOD'+str(lod)+'.fbx')))==lod)
  ck(name+'_three_lods',SUB.get_lod_count(mesh)==3,SUB.get_lod_count(mesh));meshes[label]=mesh
  maps={role:tex('T_Kelvora_'+label+'_'+suffix,role) for role,suffix in [('color','BaseColor'),('normal','Normal_DX'),('masks','ORM')]}
  m=mat('M_Kelvora_'+label+'_Body');a=sample(m,maps['color']);b=sample(m,maps['normal'],'normal');c=sample(m,maps['masks'],'masks');ML.connect_material_property(a,'RGB',unreal.MaterialProperty.MP_BASE_COLOR);ML.connect_material_property(b,'RGB',unreal.MaterialProperty.MP_NORMAL)
  for channel,prop in [('R',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),('G',unreal.MaterialProperty.MP_ROUGHNESS),('B',unreal.MaterialProperty.MP_METALLIC)]:ML.connect_material_property(c,channel,prop)
  ML.recompile_material(m);E.save_loaded_asset(m);mesh.set_material(0,m)
 # Fixed printed radio scale. It is a different slot from the worn metal/bakelite atlas.
 dt=tex('T_Kelvora_Radio_Dial','color');dm=mat('M_Kelvora_Radio_Dial');dn=sample(dm,dt);ML.connect_material_property(dn,'RGB',unreal.MaterialProperty.MP_BASE_COLOR);rough=node(dm,unreal.MaterialExpressionConstant,r=.27);ML.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS);ML.recompile_material(dm);E.save_loaded_asset(dm);meshes['Radio'].set_material(1,dm)
 # Screen material uses the same parameter contract as S47's existing TV canvas writers.
 black=tex('T_Kelvora_Black','color');screen=mat('M_Kelvora_TV_Screen');screen.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT);uv=node(screen,unreal.MaterialExpressionTextureCoordinate,coordinate_index=0)
 # FBX imports V top-down; the game's existing canvas writers apply EmV=(0,-1,1).
 flip=node(screen,unreal.MaterialExpressionMultiply);connect(uv,flip,'A');axis=node(screen,unreal.MaterialExpressionConstant2Vector,r=1.,g=-1.);connect(axis,flip,'B')
 shifted=node(screen,unreal.MaterialExpressionAdd);connect(flip,shifted,'A');offset=node(screen,unreal.MaterialExpressionConstant2Vector,r=0.,g=1.);connect(offset,shifted,'B');uv=shifted
 one=node(screen,unreal.MaterialExpressionConstant,r=1.);uv3=node(screen,unreal.MaterialExpressionAppendVector);connect(uv,uv3,'A');connect(one,uv3,'B')
 u=node(screen,unreal.MaterialExpressionVectorParameter,parameter_name='EmU',default_value=unreal.LinearColor(1,0,0,0));v=node(screen,unreal.MaterialExpressionVectorParameter,parameter_name='EmV',default_value=unreal.LinearColor(0,1,0,0))
 du=node(screen,unreal.MaterialExpressionDotProduct);dv=node(screen,unreal.MaterialExpressionDotProduct);connect(uv3,du,'A');connect(u,du,'B',out='RGB');connect(uv3,dv,'A');connect(v,dv,'B',out='RGB')
 coords=node(screen,unreal.MaterialExpressionAppendVector);connect(du,coords,'A');connect(dv,coords,'B');image=sample(screen,black,param='EmissiveMap');connect(coords,image,'UVs')
 em=node(screen,unreal.MaterialExpressionVectorParameter,parameter_name='Emissive',default_value=unreal.LinearColor(1,1,1,1));enabled=node(screen,unreal.MaterialExpressionScalarParameter,parameter_name='UseEmissiveMap',default_value=0.)
 mul=node(screen,unreal.MaterialExpressionMultiply);connect(image,mul,'A',out='RGB');connect(em,mul,'B',out='RGB');power=node(screen,unreal.MaterialExpressionMultiply);connect(mul,power,'A');connect(enabled,power,'B');ML.connect_material_property(power,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 tint=node(screen,unreal.MaterialExpressionVectorParameter,parameter_name='Color',default_value=unreal.LinearColor(.014,.022,.020,1));ML.connect_material_property(tint,'RGB',unreal.MaterialProperty.MP_BASE_COLOR);rough=node(screen,unreal.MaterialExpressionConstant,r=.18);ML.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS);ML.recompile_material(screen);E.save_loaded_asset(screen);meshes['TV'].set_material(1,screen)
 # Separate controllable power pilot.
 led=mat('M_Kelvora_TV_Power');col=node(led,unreal.MaterialExpressionVectorParameter,parameter_name='Emissive',default_value=unreal.LinearColor(.8,.002,.001,1));ML.connect_material_property(col,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR);E.save_loaded_asset(led);meshes['TV'].set_material(2,led)
 for label,mesh in meshes.items():E.save_loaded_asset(mesh);ck(label+'_assigned_materials',all(x.material_interface for x in mesh.get_editor_property('static_materials')))
 ck('TV_screen_slot_1',meshes['TV'].get_editor_property('static_materials')[1].material_interface==screen)
 report['assets']=list(E.list_assets(DEST,recursive=True,include_folder=False));report['status']='PASS';save();unreal.log('KELVORA IMPORT PASS')
except Exception:
 report['status']='FAIL';report['error']=traceback.format_exc();save();unreal.log_error(report['error']);raise
