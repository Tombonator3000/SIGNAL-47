"""Import original PhotoLab additive details through the measured project FBX/PBR recipe.
Run in an idle Unreal Editor commandlet. --verify-only checks saved assets without import.
Runtime components must also explicitly use NoCollision and existing light/power gates.
"""
from pathlib import Path
import hashlib
import json
import math
import sys
import traceback
import unreal

HERE=Path(__file__).resolve().parent
PROJECT=HERE.parents[1]
SOURCE=PROJECT.parent/'ArtSource/PhotoLab'
DEST='/Game/Signal47/Art/PhotoLab'
OWNED_MESHES=frozenset({
    'SM_PhotoLab_WetCabinetDetail','SM_PhotoLab_RinseSink','SM_PhotoLab_EnlargerDetail',
    'SM_PhotoLab_ProcessDetails','SM_PhotoLab_StorageDetail','SM_PhotoLab_ArchiveDetail',
    'SM_PhotoLab_RecordsDetail','SM_PhotoLab_SafelightHousing','SM_PhotoLab_Ventilation',
    'SM_PhotoLab_UtilityDetail',
})
sys.path.insert(0,str(HERE.parent/'fieldintegration'))
import import_recipe as R
R.REPORT=Path(unreal.Paths.project_saved_dir())/'photolab-native-import.json'
VERIFY_ONLY='--verify-only' in sys.argv or 'S47PhotoLabVerifyOnly' in unreal.SystemLibrary.get_command_line()
R.report={'status':'RUNNING','engine':unreal.SystemLibrary.get_engine_version(),
          'mode':'verify_saved_only' if VERIFY_ONLY else 'import_and_verify',
          'source':str(SOURCE),'asset_root':DEST,'checks':[],'meshes':[],'apertures':[]}
R.flush()

# The shared helper already performs precise import/texture/compile checks. Keep the receipt
# grouped by meaningful asset outcomes instead of duplicating hundreds of success entries.
def require(name, condition, detail=None):
    if not condition:
        R.report['checks'].append({'name':name,'pass':False,'detail':detail})
        R.flush()
        raise AssertionError((name,detail))
R.check=require

def checked(name,condition,detail=None):
    require(name,condition,detail)
    R.report['checks'].append({'name':name,'pass':True,'detail':detail})
    R.flush()

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def vector(p):return (p.x,p.y,p.z)
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])

def native_triangles(mesh):
    # Public UE5.8 reflected MeshDescription API; checks imported LOD0, not reduced render buffers.
    d=mesh.get_static_mesh_description(0)
    require(mesh.get_name()+' saved description',d is not None)
    n=d.get_triangle_count();out=[];index=0
    while len(out)<n and index<n*4+64:
        tid=unreal.TriangleID()
        require('TriangleID conversion',tid.import_text('(IDValue='+str(index)+')'))
        if d.is_triangle_valid(tid):
            # UE5.8 MeshDescriptionBase.cpp GetTriangleVertices SetNumUninitialized(3)
            # then Algo::Copy appends three more entries. Use indexed instances instead.
            ids=[d.get_vertex_instance_vertex(d.get_triangle_vertex_instance(tid,k)) for k in range(3)]
            out.append(tuple(vector(d.get_vertex_position(v)) for v in ids))
        index+=1
    require(mesh.get_name()+' complete triangle scan',len(out)==n)
    return out

def segment_hit(start,end,tri):
    # Two-sided finite-segment Moller-Trumbore in native centimetres.
    direction=sub(end,start);a,b,c=tri;e1,e2=sub(b,a),sub(c,a)
    h=cross(direction,e2);det=dot(e1,h)
    if abs(det)<1e-9:return False
    inv=1/det;s=sub(start,a);u=inv*dot(s,h)
    if u<-.000001 or u>1.000001:return False
    q=cross(s,e1);v=inv*dot(direction,q)
    if v<-.000001 or u+v>1.000001:return False
    t=inv*dot(e2,q)
    return .000001<t<.999999

def ue(p):return (p[0]*100,p[2]*100,p[1]*100)

def check_material(name,mat):
    require(name+' material',isinstance(mat,unreal.Material))
    nodes=[n for n in R.M.get_material_expressions(mat) if isinstance(n,unreal.MaterialExpressionVectorParameter)
           and str(n.get_editor_property('parameter_name'))=='EmissiveFactor']
    no_glow=False
    if nodes:
        c=nodes[0].get_editor_property('default_value');no_glow=len(nodes)==1 and max(abs(c.r),abs(c.g),abs(c.b))<1e-7
    checked(name+' compiled opaque nonemissive material',no_glow and mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE)

try:
    contract=json.loads((SOURCE/'model_contract.json').read_text())
    rows=json.loads((SOURCE/'materials.json').read_text())
    checked('Exact ten-part additive source',set(contract['parts'])==OWNED_MESHES and contract['legacy_removal_bounds']==[])
    R.report['contract_sha256']=sha(SOURCE/'model_contract.json')
    source_proof=SOURCE/'source-verification.json'
    require('Fresh source verification exists',source_proof.is_file())
    proof=json.loads(source_proof.read_text())
    checked('Fresh source/contract/aperture verification bound',proof['status']=='PASS' and
            proof['contract_sha256']==sha(SOURCE/'model_contract.json') and
            proof['source_sha256']==sha(SOURCE/'Signal47_PhotoLab_Detail.blend'))
    R.report['source_verification_sha256']=sha(source_proof)
    materials={}
    for row in rows:
        name=row['name']
        require(name+' no author emission',row.get('emission_strength',0)==0)
        mat=R.E.load_asset(DEST+'/Materials/'+name) if VERIFY_ONLY else R.material(row,DEST+'/Materials',SOURCE)
        check_material(name,mat);materials[name]=mat
    meshes={}
    for name,row in contract['parts'].items():
        path=DEST+'/'+name
        if VERIFY_ONLY:
            mesh=R.E.load_asset(path)
        else:
            fbx=SOURCE/'Export'/(name+'.fbx')
            require(name+' authored FBX available before refresh',fbx.is_file(),str(fbx))
            # The validated field/Office recipe replaces only the owned mesh:
            # atomic FBX reimport can retain a previous derived source description.
            # Keep native materials, textures and every gameplay identity intact.
            if R.E.does_asset_exist(path):
                existing=R.E.load_asset(path)
                require(name+' exact owned StaticMesh before refresh',
                        name in OWNED_MESHES and isinstance(existing,unreal.StaticMesh) and
                        existing.get_path_name()==path+'.'+name,
                        existing.get_path_name() if existing else path)
                existing=None
                checked(name+' replace only authored PhotoLab mesh',R.E.delete_asset(path))
            mesh=R.import_mesh(fbx,DEST)
        require(name+' native StaticMesh',isinstance(mesh,unreal.StaticMesh))
        if not VERIFY_ONLY:
            for i,slot in enumerate(mesh.get_editor_property('static_materials')):
                slot_name=str(slot.material_slot_name)
                require(name+' known slot',slot_name in materials,slot_name)
                mesh.set_material(i,materials[slot_name])
            settings=[]
            for ratio,screen in ((1.,1.),(.5,.25),(.2,.075)):
                setting=unreal.StaticMeshReductionSettings()
                setting.set_editor_property('percent_triangles',ratio)
                setting.set_editor_property('screen_size',screen)
                settings.append(setting)
            options=unreal.StaticMeshReductionOptions()
            options.set_editor_property('auto_compute_lod_screen_size',False)
            options.set_editor_property('reduction_settings',settings)
            require(name+' generate three LODs',R.S.set_lods(mesh,options)==3)
            nanite=mesh.get_editor_property('nanite_settings');nanite.enabled=False
            mesh.set_editor_property('nanite_settings',nanite)
            mesh.set_editor_property('support_ray_tracing',True)
            R.S.remove_collisions(mesh)
            body=mesh.get_editor_property('body_setup')
            require(name+' body setup',body is not None)
            default=body.get_editor_property('default_instance')
            default.set_editor_property('collision_profile_name','NoCollision')
            default.set_editor_property('collision_enabled',unreal.CollisionEnabled.NO_COLLISION)
            body.set_editor_property('default_instance',default)
            body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
            for lod in range(3):
                build=R.S.get_lod_build_settings(mesh,lod)
                build.set_editor_property('use_full_precision_u_vs',True)
                build.set_editor_property('remove_degenerates',False)
                R.S.set_lod_build_settings(mesh,lod,build)
            require(name+' saved',R.E.save_loaded_asset(mesh))
        slots=mesh.get_editor_property('static_materials')
        slot_names=[str(s.material_slot_name) for s in slots]
        checked(name+' exact bound material slots',set(slot_names)==set(row['materials']) and
                all(s.material_interface and s.material_interface.get_path_name().startswith(DEST+'/Materials/') for s in slots))
        bounds=mesh.get_bounds();lo,hi=row['bounds_local_m']
        expected_center=[(lo[0]+hi[0])*50,-(lo[1]+hi[1])*50,(lo[2]+hi[2])*50]
        actual_center=list(vector(bounds.origin));actual_size=[v*2 for v in vector(bounds.box_extent)]
        error=max([abs(x-y) for x,y in zip(actual_center,expected_center)]+
                  [abs(actual_size[k]-(hi[k]-lo[k])*100) for k in range(3)])
        checked(name+' centimetre bounds/orientation',error<.15,{'max_error_cm':error})
        tris=[mesh.get_num_triangles(lod) for lod in range(mesh.get_num_lods())]
        checked(name+' actual reduced LODs and RT policy',len(tris)==3 and tris[0]==row['triangles'] and tris[0]>tris[1]>tris[2]>0
                and not mesh.get_editor_property('nanite_settings').enabled and mesh.get_editor_property('support_ray_tracing'),tris)
        default=mesh.get_editor_property('body_setup').get_editor_property('default_instance')
        checked(name+' decorative collision disabled',default.get_editor_property('collision_enabled')==unreal.CollisionEnabled.NO_COLLISION
                and str(default.get_editor_property('collision_profile_name'))=='NoCollision')
        meshes[name]=mesh
        R.report['meshes'].append({'name':name,'asset':mesh.get_path_name(),'triangles':tris,'materials':slot_names,
            'center_cm':actual_center,'size_cm':actual_size,'fbx_sha256':sha(SOURCE/'Export'/(name+'.fbx'))})
    # Actual saved imported triangles, restricted to the relevant new housings/basin.
    names={'enlarger emission':'SM_PhotoLab_EnlargerDetail','wall diffuser':'SM_PhotoLab_SafelightHousing',
           'ceiling diffuser':'SM_PhotoLab_SafelightHousing','sink open bowl':'SM_PhotoLab_RinseSink'}
    triangles={name:native_triangles(meshes[name]) for name in set(names.values())}
    for ap in proof['apertures']:
        name=ap['name'];hits=0;off=ap['offset_m']
        for u,v in ((0,0),(off,0),(-off,0),(0,off),(0,-off)):
            delta=(u,0,v) if ap['offset_axis']=='Y' else (u,v,0)
            a=ue(tuple(x+y for x,y in zip(ap['start_three_m'],delta)))
            b=ue(tuple(x+y for x,y in zip(ap['end_three_m'],delta)))
            if any(segment_hit(a,b,t) for t in triangles[names[name]]):hits+=1
        checked(name+' imported LOD0 aperture',hits==0,{'rays':5,'blocked':hits})
        R.report['apertures'].append({'name':name,'mesh':names[name],'saved_lod0_clear':hits==0})
    R.report['status']='PASS'
    R.report['limitations']='Native saved LOD0 apertures verified; reduced render LOD openings and actual lighting/interactions require game-image review. Runtime components must also explicitly use NoCollision.'
    R.flush()
    unreal.log('S47 PHOTOLAB NATIVE IMPORT PASS '+str(len(meshes))+' parts')
except Exception:
    R.report['status']='FAIL';R.report['error']=traceback.format_exc();R.flush();raise
