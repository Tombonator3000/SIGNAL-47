"""Import the lived-in dressing with the measured FBX/PBR recipe. -S47LivedSet=control|... -S47LivedRoot=<dir with Export, textures and json>.
Writes only under /Game/Lived/<Set>."""
from pathlib import Path
import json,re,sys
import unreal
PROJECT=Path('/home/tom/SIGNAL-47/unreal/Signal47')
sys.path.insert(0,str(PROJECT/'Tools/fieldintegration'))
import import_recipe as R
cl=unreal.SystemLibrary.get_command_line()
kind=re.search(r'-S47LivedSet=(\w+)',cl).group(1);SOURCE=Path(re.search(r'-S47LivedRoot=(\S+)',cl).group(1))
DEST='/Game/Lived/'+kind.capitalize()
R.REPORT=Path(unreal.Paths.project_saved_dir())/('lived-'+kind+'-import.json')
R.report={'status':'RUNNING','kind':kind,'checks':[],'meshes':[]};R.flush()
def truck_material(row):
    # The cab's own slot: its new atlas, with the source's lamps as S47TripoFleet and S47Fleet drive them through vertex colour: Emissive = Vertex.R * S47Headlamp + Vertex.G * S47Taillamp.
    M,E,T=R.M,R.E,R.T;dest=DEST+'/Materials';name=row['name']
    mat=E.load_asset(dest+'/'+name) if E.does_asset_exist(dest+'/'+name) else T.create_asset(name,dest,unreal.Material,unreal.MaterialFactoryNew())
    for x in list(M.get_material_expressions(mat)):M.delete_material_expression(mat,x)
    samples={}
    for role,fn in row['textures'].items():
        task=unreal.AssetImportTask();task.filename=str(SOURCE/fn);task.destination_path=dest+'/Textures';task.destination_name='T_'+name+'_'+role;task.replace_existing=True;task.automated=True;task.save=True
        T.import_asset_tasks([task]);tex=E.load_asset(task.destination_path+'/'+task.destination_name)
        tex.set_editor_property('srgb',role=='base_color')
        if role=='normal':tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP);tex.set_editor_property('flip_green_channel',False)
        E.save_loaded_asset(tex)
        samples[role]=R.expression(mat,unreal.MaterialExpressionTextureSample,texture=tex,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role=='normal' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    M.connect_material_property(samples['base_color'],'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    M.connect_material_property(samples['normal'],'RGB',unreal.MaterialProperty.MP_NORMAL)
    M.connect_material_property(R.expression(mat,unreal.MaterialExpressionConstant,r=.55),'',unreal.MaterialProperty.MP_ROUGHNESS)
    head=R.expression(mat,unreal.MaterialExpressionVectorParameter,parameter_name='S47Headlamp',default_value=unreal.LinearColor(0,0,0,1))
    tail=R.expression(mat,unreal.MaterialExpressionVectorParameter,parameter_name='S47Taillamp',default_value=unreal.LinearColor(0,0,0,1))
    vc=R.expression(mat,unreal.MaterialExpressionVertexColor)
    a=R.expression(mat,unreal.MaterialExpressionMultiply);R.check('head mask',M.connect_material_expressions(vc,'R',a,'A'));R.check('head',M.connect_material_expressions(head,'RGB',a,'B'))
    b=R.expression(mat,unreal.MaterialExpressionMultiply);R.check('tail mask',M.connect_material_expressions(vc,'G',b,'A'));R.check('tail',M.connect_material_expressions(tail,'RGB',b,'B'))
    c=R.expression(mat,unreal.MaterialExpressionAdd);R.check('add a',M.connect_material_expressions(a,'',c,'A'));R.check('add b',M.connect_material_expressions(b,'',c,'B'))
    M.connect_material_property(c,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    errors=list(M.recompile_material(mat));R.check(name+' compiles',not errors,errors);E.save_loaded_asset(mat)
    return mat
def decal_material(row):
    # A ground decal: base colour and roughness, opacity from the picture's alpha; projected by S47Lived onto whatever ground is there.
    M,E,T=R.M,R.E,R.T;dest=DEST+'/Materials';name=row['name']
    mat=E.load_asset(dest+'/'+name) if E.does_asset_exist(dest+'/'+name) else T.create_asset(name,dest,unreal.Material,unreal.MaterialFactoryNew())
    for x in list(M.get_material_expressions(mat)):M.delete_material_expression(mat,x)
    mat.set_editor_property('material_domain',unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    task=unreal.AssetImportTask();task.filename=str(SOURCE/row['texture']);task.destination_path=dest+'/Textures';task.destination_name='T_'+name;task.replace_existing=True;task.automated=True;task.save=True
    T.import_asset_tasks([task]);tex=E.load_asset(task.destination_path+'/'+task.destination_name);tex.set_editor_property('srgb',True);E.save_loaded_asset(tex)
    smp=R.expression(mat,unreal.MaterialExpressionTextureSample,texture=tex,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    M.connect_material_property(smp,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    M.connect_material_property(smp,'A',unreal.MaterialProperty.MP_OPACITY)
    M.connect_material_property(R.expression(mat,unreal.MaterialExpressionConstant,r=float(row['roughness'])),'',unreal.MaterialProperty.MP_ROUGHNESS)
    errors=list(M.recompile_material(mat));R.check(name+' compiles',not errors,errors);E.save_loaded_asset(mat)
    return mat
def lens_material(row):
    # The new head lamps' glass: they are their own slot, so the loop in S47TripoFleet picks them up by the parameter.
    M,E,T=R.M,R.E,R.T;dest=DEST+'/Materials';name=row['name']
    mat=E.load_asset(dest+'/'+name) if E.does_asset_exist(dest+'/'+name) else T.create_asset(name,dest,unreal.Material,unreal.MaterialFactoryNew())
    for x in list(M.get_material_expressions(mat)):M.delete_material_expression(mat,x)
    M.connect_material_property(R.expression(mat,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(*row['base_color'],1)),'',unreal.MaterialProperty.MP_BASE_COLOR)
    M.connect_material_property(R.expression(mat,unreal.MaterialExpressionConstant,r=.12),'',unreal.MaterialProperty.MP_ROUGHNESS)
    head=R.expression(mat,unreal.MaterialExpressionVectorParameter,parameter_name='S47Headlamp',default_value=unreal.LinearColor(0,0,0,1))
    M.connect_material_property(head,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    errors=list(M.recompile_material(mat));R.check(name+' compiles',not errors,errors);E.save_loaded_asset(mat)
    return mat
contract=json.loads((SOURCE/('lived-'+kind+'-contract.json')).read_text())
materials={}
for row in json.loads((SOURCE/('lived-'+kind+'-materials.json')).read_text()):
    if row.get('reuse'):
        m=R.E.load_asset(row['reuse']);R.check('reuse '+row['name'],isinstance(m,unreal.MaterialInterface));materials[row['name']]=m;continue
    if row.get('truck_body'):
        materials[row['name']]=truck_material(row);continue
    if row.get('decal'):
        materials[row['name']]=decal_material(row);continue
    if row.get('truck_lens'):
        materials[row['name']]=lens_material(row);continue
    materials[row['name']]=R.material(row,DEST+'/Materials',SOURCE)
for name,c in contract['meshes'].items():
    path=DEST+'/'+name
    if R.E.does_asset_exist(path):R.check('replace '+name,R.E.delete_asset(path))
    mesh=R.import_mesh(SOURCE/contract.get('export_dir','Export')/(name+'.fbx'),DEST)
    bound=[]
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        n=str(slot.material_slot_name);R.check('material '+n,n in materials);mesh.set_material(i,materials[n]);bound.append(n)
    R.check(name+' all materials',set(bound)==set(c['materials']),[sorted(bound),sorted(c['materials'])])
    R.check(name+' triangles (zero-radius lathe caps drop out)',.8*c['triangles']<=mesh.get_num_triangles(0)<=c['triangles'],mesh.get_num_triangles(0))
    low,high=c['bounds_local_m'];box=mesh.get_bounding_box()
    el=[low[0]*100,-high[1]*100,low[2]*100];eh=[high[0]*100,-low[1]*100,high[2]*100]
    al=[box.min.x,box.min.y,box.min.z];ah=[box.max.x,box.max.y,box.max.z]
    R.check(name+' shared origin',max(abs(a-b) for x,y in((al,el),(ah,eh)) for a,b in zip(x,y))<.5,{'min':al,'max':ah,'exp':[el,eh]})
    collide=c.get('collision')=='complex'
    n=mesh.get_editor_property('nanite_settings');n.enabled=False;mesh.set_editor_property('nanite_settings',n)
    mesh.set_editor_property('support_ray_tracing',True)
    body=mesh.get_editor_property('body_setup')
    if body:
        if collide:
            # The walker's walls are boxes in the plan; this is for what the crosshair and the torch can reach.
            body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        else:
            inst=body.get_editor_property('default_instance');inst.set_editor_property('collision_profile_name','NoCollision');body.set_editor_property('default_instance',inst)
    for lod in range(R.S.get_lod_count(mesh)):
        for s in range(mesh.get_num_sections(lod)):R.S.enable_section_collision(mesh,collide,lod,s)
        b=R.S.get_lod_build_settings(mesh,lod);b.set_editor_property('use_full_precision_u_vs',True);R.S.set_lod_build_settings(mesh,lod,b)
    if contract.get('lods'):
        o=unreal.EditorScriptingMeshReductionOptions();o.auto_compute_lod_screen_size=False
        o.reduction_settings=[unreal.EditorScriptingMeshReductionSettings(1.0,1.0),unreal.EditorScriptingMeshReductionSettings(.45,.32),unreal.EditorScriptingMeshReductionSettings(.14,.09)]
        R.S.set_lods(mesh,o);R.check(name+' three LODs',mesh.get_num_lods()==3 if hasattr(mesh,'get_num_lods') else R.S.get_lod_count(mesh)==3)
    R.check('saved '+name,R.E.save_loaded_asset(mesh))
    R.report['meshes'].append({'asset':mesh.get_path_name(),'triangles':c['triangles']})
R.report['status']='PASS';R.flush()
unreal.log('S47 LIVED IMPORT PASS '+kind)
