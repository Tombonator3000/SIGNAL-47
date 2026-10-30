"""Own FinaleSky import/fresh-reader. Root runs through unreal_guard; this file starts no process.

Environment S47_FINALE_JOB is a fresh job JSON. Apply writes ONLY seven owned
assets. Verify is separate-PID and read-only for Content; a PASS binding is written
to a requested external path and staged by root, never manufactured in runtime.
"""
from collections import defaultdict
import gzip
import hashlib
import importlib.util
import itertools
import json
import math
import os
from pathlib import Path
import sys
import traceback
import unreal

HERE=Path(__file__).resolve().parent
PROJECT=HERE.parents[1]
E=unreal.EditorAssetLibrary;M=unreal.MaterialEditingLibrary;T=unreal.AssetToolsHelpers.get_asset_tools()
S=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)or unreal.new_object(unreal.StaticMeshEditorSubsystem)
DEST='/Game/FinaleSky';ROLES=('Hull','Ribs','Recess','Light')
OWN_NAMES={'SM_S47_FinalCraft','M_S47_FinalSurface','M_S47_FinalCloud'}|{'MI_S47_Final'+r for r in ROLES}


def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb')as f:
        while b:=f.read(8*1024*1024):h.update(b)
    return h.hexdigest()


def read(path):return json.loads(Path(path).read_text())
def dump(path,value):Path(path).write_text(json.dumps(value,indent=2)+'\n')


def snapshot():
    root=PROJECT/'Content'
    return {str(p.relative_to(root)):sha(p)for p in root.rglob('*')if p.is_file()}


def protected_equal(before,after,allow_owned):
    def owned(name):
        p=Path(name)
        return len(p.parts)==2 and p.parts[0]=='FinaleSky' and p.stem in OWN_NAMES and p.suffix in ('.uasset','.uexp','.ubulk')
    keys=set(before)|set(after)
    return all(before.get(k)==after.get(k)for k in keys if not(allow_owned and owned(k)))


def load(name):
    path=DEST+'/'+name
    return E.load_asset(path)if E.does_asset_exist(path)else None


def new_material(name):
    mat=load(name)or T.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew())
    assert isinstance(mat,unreal.Material)
    for node in list(M.get_material_expressions(mat)):M.delete_material_expression(mat,node)
    return mat


def node(mat,cls,**props):
    n=M.create_material_expression(mat,cls)
    for k,v in props.items():n.set_editor_property(k,v)
    return n


def custom(mat,code,inputs,out):
    n=node(mat,unreal.MaterialExpressionCustom,code=code,output_type=out)
    rows=[]
    for name,_,_ in inputs:
        r=unreal.CustomInput();r.set_editor_property('input_name',name);rows.append(r)
    n.set_editor_property('inputs',rows)
    for name,source,output in inputs:assert M.connect_material_expressions(source,output,n,name)
    return n


def scalar(mat,name,value):return node(mat,unreal.MaterialExpressionScalarParameter,parameter_name=name,default_value=value)
def vector(mat,name,value):return node(mat,unreal.MaterialExpressionVectorParameter,parameter_name=name,default_value=unreal.LinearColor(*value))


def make_materials(contract):
    mat=new_material('M_S47_FinalSurface');mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_DEFAULT_LIT);mat.set_editor_property('two_sided',False)
    color=vector(mat,'Color',(.1,.1,.1,1));rough=scalar(mat,'Roughness',.5);metal=scalar(mat,'Metallic',.75);glow=scalar(mat,'Glow',0)
    uv=node(mat,unreal.MaterialExpressionTextureCoordinate,coordinate_index=0)
    # Original machined wear: small radial seams and brushed variation; no borrowed texture.
    pat=custom(mat,'float seam=pow(abs(sin(UV.x*301.5929)),44.);float grain=sin(UV.x*4777.+sin(UV.y*717.))*sin(UV.y*3991.);return C*(.89+.065*grain-.18*seam);',
               [('UV',uv,''),('C',color,'RGB')],unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    emission=custom(mat,'return C*K;',[('C',color,'RGB'),('K',glow,'')],unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    for n,prop in [(pat,unreal.MaterialProperty.MP_BASE_COLOR),(rough,unreal.MaterialProperty.MP_ROUGHNESS),(metal,unreal.MaterialProperty.MP_METALLIC),(emission,unreal.MaterialProperty.MP_EMISSIVE_COLOR)]:assert M.connect_material_property(n,'',prop)
    M.recompile_material(mat);assert E.save_loaded_asset(mat)
    instances={}
    for role,row in contract['materials'].items():
        name='MI_S47_Final'+role
        mi=load(name)or T.create_asset(name,DEST,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        M.set_material_instance_parent(mi,mat)
        # UE5.8's setters return false even when successful. Actual values below are the evidence.
        M.set_material_instance_vector_parameter_value(mi,'Color',unreal.LinearColor(*row['color']))
        for key in ('roughness','metallic','glow'):M.set_material_instance_scalar_parameter_value(mi,key.title(),row[key])
        assert E.save_loaded_asset(mi);instances['M_S47_Final'+role]=mi
    cloud=new_material('M_S47_FinalCloud');cloud.set_editor_property('material_domain',unreal.MaterialDomain.MD_SURFACE)
    cloud.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT);cloud.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    cloud.set_editor_property('two_sided',False);cloud.set_editor_property('translucency_pass',unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    cloud.set_editor_property('use_translucency_vertex_fog',False)
    inputs=[('WP',node(cloud,unreal.MaterialExpressionWorldPosition),''),('Camera',node(cloud,unreal.MaterialExpressionCameraPositionWS),''),
            ('SceneDepth',node(cloud,unreal.MaterialExpressionSceneDepth),''),('PixelDepth',node(cloud,unreal.MaterialExpressionPixelDepth),'')]
    for name,value in [('BankCenter',(0,0,0,0)),('BankExtent',(42000,56000,25500,0)),('Forward',(1,0,0,0)),('Right',(0,1,0,0))]:inputs.append((name,vector(cloud,name,value),'RGB'))
    for name,value in [('Phase',0),('Growth',0),('Side',1),('Samples',56),('Glow',.12)]:inputs.append((name,scalar(cloud,name,value),''))
    field=custom(cloud,(HERE/'cloud_field.hlsl').read_text(),inputs,unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    rgb=node(cloud,unreal.MaterialExpressionComponentMask,r=True,g=True,b=True,a=False)
    alpha=node(cloud,unreal.MaterialExpressionComponentMask,r=False,g=False,b=False,a=True)
    assert M.connect_material_expressions(field,'',rgb,'Input')and M.connect_material_expressions(field,'',alpha,'Input')
    assert M.connect_material_property(rgb,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)and M.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
    M.recompile_material(cloud);assert E.save_loaded_asset(cloud)
    return instances


def source_gate(root):
    manifest=read(root/'manifest.json');assert manifest['status']=='SOURCE_REOPEN_PASS_NATIVE_UNVERIFIED'and manifest['actual_source_reopen']is True
    assert manifest['revision']=='S47-FINALE-SKY-1'and len(manifest['lods'])==3
    assert manifest['contract']==read(HERE.parents[2]/'ArtSource/FinaleSky/source-contract.json')
    for rel,pin in manifest['source_inputs'].items():assert sha(PROJECT.parents[1]/rel)==pin,('changed authoring input',rel)
    for row in manifest['lods']:assert sha(root/row['fbx'])==row['fbx_sha256']
    for key in ('source_blend','native_reference'):
        row=manifest[key];p=root/row['path'];assert p.stat().st_size==row['bytes']and sha(p)==row['sha256']
    return manifest


def apply(root,manifest,report):
    spec=importlib.util.spec_from_file_location('s47_finale_existing_fbx',PROJECT/'Tools/motelv2/native.py')
    fbx=importlib.util.module_from_spec(spec);spec.loader.exec_module(fbx);fbx.DEST=DEST
    instances=make_materials(manifest['contract'])
    mesh=fbx.import_mesh(root/manifest['lods'][0]['fbx'],DEST,'SM_S47_FinalCraft',report)
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        name=str(slot.get_editor_property('imported_material_slot_name'));report.check('actual named imported material slot '+name,name in instances)
        mesh.set_material(i,instances[name])
    for lod in (1,2):report.check('own authored LOD'+str(lod),S.import_lod(mesh,lod,str(root/manifest['lods'][lod]['fbx']))==lod)
    mesh.set_editor_property('allow_cpu_access',True)
    nano=mesh.get_editor_property('nanite_settings');nano.set_editor_property('enabled',False);mesh.set_editor_property('nanite_settings',nano)
    for lod in range(3):
        b=S.get_lod_build_settings(mesh,lod)
        for key,value in [('use_full_precision_u_vs',True),('remove_degenerates',False),('recompute_normals',False),('recompute_tangents',True),('use_mikk_t_space',True)]:b.set_editor_property(key,value)
        S.set_lod_build_settings(mesh,lod,b)
        for section in range(mesh.get_num_sections(lod)):S.enable_section_collision(mesh,False,lod,section)
    report.check('three owned authored screens',S.set_lod_screen_sizes(mesh,[1.,.35,.12]))
    report.check('save only own craft mesh',E.save_loaded_asset(mesh))


def actual(output):
    assert not output.exists()
    unreal.SystemLibrary.execute_console_command(None,'S47.FinaleSkyAssets "'+str(output)+'"')
    assert output.is_file(),'compiled own read-only reader did not complete'
    return read(output)


def compare_triangles(expected,actual):
    # One-to-one oriented matching tolerates legal vertex IDs/order/splits, never reversed winding.
    # Native float32 rounding at this 100cm source is bounded by0.001cm. UVs by1e-6.
    buckets=defaultdict(list)
    def cell(t):return tuple(math.floor(sum(p[k]for p in t['p'])/3/.01)for k in range(3))
    for i,t in enumerate(expected):buckets[(t['material'],cell(t))].append(i)
    used=set();maxp=maxuv=0
    for t in actual:
        matches=[];c=cell(t)
        for off in itertools.product((-1,0,1),repeat=3):
            for i in buckets.get((t['material'],tuple(c[k]+off[k]for k in range(3))),[]):
                if i in used:continue
                e=expected[i]
                for r in range(3):
                    p=max(abs(t['p'][j][k]-e['p'][(j+r)%3][k])for j in range(3)for k in range(3))
                    uv=max(abs(t['uv'][j][k]-e['uv'][(j+r)%3][k])for j in range(3)for k in range(2))
                    if p<=.001 and uv<=1e-6:matches.append((i,p,uv));break
        assert matches,('actual triangle has no oriented position/UV/named-material source',t)
        i,p,uv=min(matches,key=lambda x:(x[1],x[2],x[0]));used.add(i);maxp=max(maxp,p);maxuv=max(maxuv,uv)
    assert len(used)==len(expected)==len(actual),('incomplete source triangle multiset',len(used),len(expected),len(actual))
    return dict(triangles=len(used),max_position_error_cm=maxp,max_uv_error=maxuv,missing=0)


def verify(root,manifest,data,report):
    report.check('compiled actual own native inventory',data['status']=='MEASURED'and data['mesh']['allowCPUAccess']and len(data['mesh']['lods'])==3)
    mesh=load('SM_S47_FinalCraft');slots=list(mesh.get_editor_property('static_materials'))
    names=[str(s.get_editor_property('imported_material_slot_name'))for s in slots]
    report.check('four distinct actual named native slots',len(names)==len(set(names))==4 and set(names)==set(manifest['contract']['material_slots']),names)
    report.check('actual mesh slots keep correct own material instance by imported name',all(data['mesh']['materials'][i]==DEST+'/MI_S47_Final'+name.removeprefix('M_S47_Final')+'.MI_S47_Final'+name.removeprefix('M_S47_Final')for i,name in enumerate(names)))
    with gzip.open(root/manifest['native_reference']['path'],'rt')as f:reference=json.load(f)
    report.check('complete source/saved/render LOD inventory',len(reference['lods'])==len(data['savedSource']['lods'])==len(data['mesh']['lods'])==len(manifest['lods'])==3)
    results=[]
    for lod,(src,saved,render)in enumerate(zip(reference['lods'],data['savedSource']['lods'],data['mesh']['lods'])):
        report.check('actual LOD identity '+str(lod),src['lod']==saved['lod']==render['lod']==lod)
        triangles=[dict(p=[p['v']for p in t['p']],uv=[u['v']for u in t['uv']],material=t['material'])for t in saved['triangles']]
        results.append(compare_triangles(src['triangles'],triangles))
        report.check('LOD'+str(lod)+' actual saved/render/source triangle counts',len(triangles)==render['triangles']==manifest['lods'][lod]['triangles'])
        for g in saved['groups']:
            report.check('LOD'+str(lod)+' actual group name -> global slot',g['name']in names and 0<=g['slot']<len(names) and names[g['slot']]==g['name'],g)
        report.check('LOD'+str(lod)+' actual sections route all four named materials',set(s['material']for s in render['sections'])==set(range(4)) and all(not s['collision']for s in render['sections']))
    report.data['oriented_source_parity']=results
    for role,row in manifest['contract']['materials'].items():
        mi=load('MI_S47_Final'+role);a=next(x for x in data['materials']if x['path']==mi.get_path_name())
        report.check(role+' actual parent/Color/PBR/Glow',a['read']and a['parent']==DEST+'/M_S47_FinalSurface.M_S47_FinalSurface'and max(abs(x-y)for x,y in zip(a['color'],row['color']))<1e-6 and all(abs(a[k]-row[k])<1e-6 for k in ('roughness','metallic','glow')),a)
    cloud=load('M_S47_FinalCloud');expressions=list(M.get_material_expressions(cloud))
    codes=[n.get_editor_property('code')for n in expressions if isinstance(n,unreal.MaterialExpressionCustom)]
    report.check('actual saved finite field is exact original HLSL',codes==[(HERE/'cloud_field.hlsl').read_text()])
    report.check('actual depth-connected material nodes',sum(isinstance(n,unreal.MaterialExpressionSceneDepth)for n in expressions)==1 and sum(isinstance(n,unreal.MaterialExpressionPixelDepth)for n in expressions)==1)


def main():
    jobpath=Path(os.environ['S47_FINALE_JOB']).resolve();job=read(jobpath)
    out=Path(job['report']).resolve();assert not out.exists()and out.parent.is_dir()
    root=Path(job['source']).resolve();mode=job['mode'];assert mode in ('apply','verify')
    spec=importlib.util.spec_from_file_location('s47_finale_report',PROJECT/'Tools/motelv2/native.py');base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
    report=base.Report(out,'FinaleSky-'+mode);before=snapshot();report.data.update(pid=os.getpid(),job_sha256=sha(jobpath),protected_before_sha256=before);report.flush()
    try:
        manifest=source_gate(root)
        if mode=='apply':apply(root,manifest,report)
        data=actual(out.parent/(out.stem+'-actual.json'));verify(root,manifest,data,report)
        after=snapshot();report.check('all non-owned Content bytes unchanged'if mode=='apply'else 'fresh reader changed no Content bytes',protected_equal(before,after,mode=='apply'))
        report.data.update(assets=data,native_files_sha256={k:v for k,v in after.items()if k.startswith('FinaleSky/')},manifest_sha256=sha(root/'manifest.json'),status='PASS',native_only=True,runtime=False,visual=False)
        if mode=='verify':
            previous=read(job['import_report']);report.check('independent actual import PID',previous['status']=='PASS'and previous['pid']!=os.getpid()and previous['native_files_sha256']==report.data['native_files_sha256']and previous['manifest_sha256']==report.data['manifest_sha256'])
            old=previous['assets']['mesh'];report.check('complete saved/render LOD payload identical between PIDs',old==data['mesh'])
            binding=dict(schema=1,status='PASS',importPID=previous['pid'],freshPID=os.getpid(),source_manifest_sha256=sha(root/'manifest.json'),
                         native_files_sha256=report.data['native_files_sha256'],import_report_sha256=sha(job['import_report']),assets={k:v for k,v in data.items()if k!='savedSource'},scope='native source/payload only; visuals/GPU/whole drive UNVERIFIED')
            path=Path(job['binding']);assert not path.exists()and path.parent.is_dir();dump(path,binding)
        report.flush();print('S47 FINALE SKY '+mode.upper()+' PASS',flush=True);unreal.log('S47 FINALE SKY '+mode.upper()+' PASS')
    except Exception:
        report.data.update(status='FAIL',error=traceback.format_exc(),protected_after_failure_sha256=snapshot());report.flush();raise


if __name__=='__main__':main()
