"""Root-only guarded UE execution; no original geometry/material assets modified.
S47_EASTWING_ACCESS_OUT points at generated FBX sources. Plain Python validates only.
S47_EASTWING_ACCESS_APPLY=1 imports; S47_EASTWING_ACCESS_REPORT is required then.
"""
import importlib.util,json,os,hashlib
from pathlib import Path
HERE=Path(__file__).resolve().parent
DEST='/Game/EastWing/Access'
OWNER='S47EastWingAccess_v1'
def require(ok,why):
    if not ok:raise RuntimeError(why)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def plan():
    root=Path(os.environ['S47_EASTWING_ACCESS_OUT']).resolve()
    data=json.loads((root/'access-manifest.json').read_text())
    require(data['source_sha256']=='fd60fa957655d289cd05909020f2f39f85e8a8e9692f3c402d9418f9b3d1a5f9' and data['delta_three_m']==[-2.6,0,0],'Unexpected original source or translation')
    require({p['name']:p['triangles'] for p in data['parts']}=={'SM_EW_AccessDressing':10256,'SM_EW_AccessStains':1220},'Unexpected derivative assets')
    for p in data['parts']:
        require(len(p['lods'])==3,'Need three authored LODs')
        require(p['moved_triangles']==([216]*3 if p['name']=='SM_EW_AccessDressing' else [2]*3),'Unexpected moved geometry')
        for row in p['lods']:
            file=(root/row['file']).resolve()
            require(file.is_relative_to(root) and sha(file)==row['sha256'],'Changed/external source FBX')
    return root,data

def apply(root,data):
    import unreal as U
    E=U.EditorAssetLibrary
    S=U.get_editor_subsystem(U.StaticMeshEditorSubsystem) or U.new_object(U.StaticMeshEditorSubsystem)
    path=HERE.parents[2]/'Signal47/Tools/fieldintegration/import_recipe.py'
    spec=importlib.util.spec_from_file_location('access_measured_recipe',path);R=importlib.util.module_from_spec(spec);spec.loader.exec_module(R)
    R.check=lambda label,ok,detail=None:require(ok,label+': '+str(detail))
    report={'status':'RUNNING','parts':[],'scope':'Native assets only; runtime binds original per-component atlas and moves three walking feet.'}
    output=Path(os.environ['S47_EASTWING_ACCESS_REPORT']).resolve()
    require(not output.is_relative_to(HERE),'Report must be private')
    output.parent.mkdir(parents=True,exist_ok=True)
    flag='Interchange.FeatureFlags.Import.FBX';old=U.SystemLibrary.get_console_variable_int_value(flag)
    try:
        U.SystemLibrary.execute_console_command(None,flag+' 0')
        for p in data['parts']:
            target=DEST+'/'+p['name']
            if E.does_asset_exist(target):require(E.get_metadata_tag(E.load_asset(target),'S47Owner')==OWNER,'Unrelated existing asset '+target)
            mesh=R.import_mesh(root/p['lods'][0]['file'],DEST)
            E.set_metadata_tag(mesh,'S47Owner',OWNER);E.save_loaded_asset(mesh,only_if_is_dirty=False)
            for lod in (1,2):require(S.import_lod(mesh,lod,str(root/p['lods'][lod]['file']))==lod,'LOD import failed')
            require(len(mesh.get_editor_property('static_materials'))==1,'Original atlas slot changed')
            mesh.set_material(0,E.load_asset('/Engine/EngineMaterials/DefaultMaterial'))
            mesh.set_editor_property('allow_cpu_access',True)
            nanite=mesh.get_editor_property('nanite_settings');nanite.enabled=False;mesh.set_editor_property('nanite_settings',nanite)
            for lod in range(3):
                settings=S.get_lod_build_settings(mesh,lod);settings.set_editor_property('use_full_precision_u_vs',True);settings.set_editor_property('remove_degenerates',False);S.set_lod_build_settings(mesh,lod,settings)
            require(S.set_lod_screen_sizes(mesh,[1.,.42,.16]),'LOD screen sizes')
            E.set_metadata_tag(mesh,'S47SourceProof',json.dumps(p,sort_keys=True))
            E.set_metadata_tag(mesh,'S47OriginalSourceSHA256',data['source_sha256'])
            E.set_metadata_tag(mesh,'S47DeltaThreeMetres',json.dumps(data['delta_three_m']))
            require(E.save_loaded_asset(mesh,only_if_is_dirty=False),'Save failed')
            counts=[mesh.get_num_triangles(lod) for lod in range(3)]
            require(S.get_lod_count(mesh)==3 and counts==[p['triangles']]*3,'Native count mismatch '+str(counts))
            b=mesh.get_bounds();lo,hi=p['bounds_blender_m']
            center=[(lo[0]+hi[0])*50,-(lo[1]+hi[1])*50,(lo[2]+hi[2])*50]
            size=[(hi[0]-lo[0])*100,(hi[1]-lo[1])*100,(hi[2]-lo[2])*100]
            observed=[b.origin.x,b.origin.y,b.origin.z,2*b.box_extent.x,2*b.box_extent.y,2*b.box_extent.z]
            require(max(abs(a-b) for a,b in zip(center+size,observed))<.25,'Axis/bounds mismatch')
            report['parts'].append({'asset':target,'native_lod_triangles':counts,'proof':p})
        report['status']='PASS_NATIVE_ACCESS_IMPORT'
    except Exception as error:
        report['status']='FAIL';report['error']=str(error);raise
    finally:
        output.write_text(json.dumps(report,indent=2)+'\n')
        U.SystemLibrary.execute_console_command(None,flag+' '+str(old))

if __name__=='__main__':
    root,data=plan()
    if os.environ.get('S47_EASTWING_ACCESS_APPLY')=='1':apply(root,data)
    else:print('PASS_ACCESS_SOURCE_PLAN',[(p['name'],p['triangles']) for p in data['parts']])
