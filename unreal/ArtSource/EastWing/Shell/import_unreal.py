"""Import generated shell assets using the project's measured FBX/PBR recipe.

Plain Python validates sources. S47_EASTWING_SHELL_APPLY=1 enables native import
inside the caller's guarded editor. S47_EASTWING_SHELL_OUT selects the generated
source directory; S47_EASTWING_SHELL_REPORT selects a private result JSON.
"""
from pathlib import Path
import importlib.util
import json
import math
import os
import traceback

HERE=Path(__file__).resolve().parent
DEST='/Game/EastWing/Shell'
OWNER='S47EastWingShell_v1'
REPO=HERE.parents[3]
PHOTO_MANIFEST=REPO/'Unity/Assets/Signal47/Art/Visual10/source-manifest.json'
PHOTO_SLOTS={'EW_Shell_Stucco':'painted_plaster_wall',
             'EW_Shell_Concrete':'concrete_floor_02',
             'EW_Shell_Plinth':'concrete_floor_02'}
PHOTO_TILE_METRES=2.
MASONRY_MANIFEST=HERE/'Textures/ConcreteBlockWall/source-manifest.json'



def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    result=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


P=module('eastwing_props_plan',HERE.parent/'Props/import_unreal.py')
L=module('eastwing_shell_layout',HERE/'layout.py')
require=P.require


def photo_sources():
    """Use installed, attributed CC0 originals directly; never copy/download source maps."""
    rows=P.read(PHOTO_MANIFEST)
    result={}
    roles={'Diffuse':'base_color','nor_gl':'normal','Rough':'roughness'}
    for asset in sorted(set(PHOTO_SLOTS.values())):
        selected=[r for r in rows if r['asset']==asset and r['map'] in roles]
        require(len(selected)==3 and {r['map'] for r in selected}==set(roles),
                'Photographic PBR map set incomplete',asset)
        textures={}
        for row in selected:
            path=(REPO/row['installed']).resolve()
            require(path.is_relative_to(REPO/'Unity/Assets/Signal47/Art/Visual10/Textures'),
                    'Photographic map outside installed source',str(path))
            require(row['license']=='CC0-1.0' and row['resolution']=='2k',
                    'Unexpected photographic source license/resolution',asset)
            require(path.is_file() and P.digest(path)==row['sha256'],
                    'Photographic source missing or modified',str(path))
            textures[roles[row['map']]]=str(path)
        result[asset]={'textures':textures,'provenance':selected}
    local=P.read(MASONRY_MANIFEST)
    require(local['license']=='CC0-1.0' and local['asset']=='concrete_block_wall'
            and local['physical_tile_metres']==[2,2], 'Unexpected local masonry provenance')
    local_roles={'BaseColor':'base_color','NormalGL':'normal','Roughness':'roughness'}
    require(len(local['files'])==3 and {r['role'] for r in local['files']}==set(local_roles),
            'Local masonry map set incomplete')
    textures={}
    for row in local['files']:
        path=(MASONRY_MANIFEST.parent/row['file']).resolve()
        require(path.is_relative_to(MASONRY_MANIFEST.parent) and path.is_file(),
                'Missing or external masonry map',str(path))
        require(row['size']==[2048,2048] and path.stat().st_size==row['bytes']
                and P.digest(path)==row['sha256'], 'Masonry source differs from manifest',str(path))
        require(row['color_space']==('sRGB' if row['role']=='BaseColor' else 'linear'),
                'Unexpected masonry color space',row['role'])
        textures[local_roles[row['role']]]=str(path)
    result[local['asset']]={'textures':textures,'provenance':local}
    return result


def ue_bounds(bounds):
    lo,hi=bounds
    return [[lo[0]*100,-hi[1]*100,lo[2]*100],[hi[0]*100,-lo[1]*100,hi[2]*100]]


def make_plan():
    source_root=Path(os.environ.get('S47_EASTWING_SHELL_OUT',str(HERE/'exports'))).resolve()
    manifest_path=source_root/'shell-manifest.json'
    require(manifest_path.is_file(),'Generate the shell with build_shell.py before importing',str(manifest_path))
    manifest=P.read(manifest_path)
    require(manifest['static_origin_three_m']==[0,0,0],'Unexpected static origin')
    require(set(item['id'] for item in manifest['assets'])==set(L.ASSETS)
            and len(manifest['assets'])==len(L.ASSETS),'Unexpected shell asset set/count')
    require(P.digest(source_root/manifest['editable'])==manifest['editable_sha256'],'Editable source differs from manifest')
    surfaces={row['key']:row for row in P.read(HERE.parent/'Props/Textures/materials.json')['materials']}
    photos=photo_sources()
    models=[]
    shared={}
    checked_images=set()
    for item in manifest['assets']:
        source=(source_root/item['manifest']).resolve()
        require(source.is_relative_to(source_root),'Manifest path escapes generated source',str(source))
        row=P.read(source)
        require(row['asset']==item['id'],'Asset name differs',str(source))
        require(row['front']=='-Y' and row['up']=='Z','Unexpected Blender axes',row['asset'])
        require(row['anchor_three_m']==L.ANCHORS.get(row['asset'],[0,0,0]),'Wrong mesh anchor',row['asset'])
        require(item['anchor_three_m']==row['anchor_three_m'],'Manifest anchor differs',row['asset'])
        require(item['native_path']==DEST+'/'+row['asset'],'Unexpected native destination',item['native_path'])
        require(len(row['lods'])==2,'Expected three authored LODs',row['asset'])
        require(len(row['materials'])==len(set(row['materials']))
                and row['materials']==[r['name'] for r in row['material_parameters']],
                'Material slots are not unique/ordered',row['asset'])
        require(P.read(source_root/(row['asset']+'.license.json'))['id']=='LicenseRef-ProjectOwned',
                'Unexpected shell license',row['asset'])
        lods=[dict(row,file=row['fbx'],sha256=row['fbx_sha256'])]+row['lods']
        anchor=row['anchor_three_m']
        offset=[anchor[0]*100,anchor[2]*100,anchor[1]*100]
        for lod,details in enumerate(lods):
            path=(source_root/details['file']).resolve()
            require(path.is_relative_to(source_root) and path.is_file(),'Missing or external shell FBX',str(path))
            require(P.digest(path)==details['sha256'],'FBX changed',str(path))
            require(details['expected_native_triangles']==details['triangles']-details['exact_degenerate_triangles']
                    and details['expected_native_triangles']>0,'Invalid native triangle prediction',[row['asset'],lod])
            expected=ue_bounds(details['bounds_m'])
            world=[[p[0]*100,p[2]*100,p[1]*100] for p in details['world_bounds_three_m']]
            for side in range(2):
                require(max(abs(expected[side][k]+offset[k]-world[side][k]) for k in range(3))<.01,
                        'Local/global bounds disagree with anchor',[row['asset'],lod])
        materials=[]
        for parameters in row['material_parameters']:
            recipe,extra=P.material_recipe(parameters,source,surfaces)
            key=parameters.get('texture_key')
            if key:
                for role,channel in surfaces[key]['maps'].items():
                    if role not in ('BaseColor','Roughness','Metallic','NormalDX'):continue
                    path=HERE.parent/'Props/Textures'/channel['file']
                    if path not in checked_images:
                        require(P.digest(path)==channel['sha256'],'Shared PBR source changed',str(path))
                        checked_images.add(path)
            photo=('concrete_block_wall' if recipe['name']=='EW_Shell_Masonry'
                   else PHOTO_SLOTS.get(recipe['name']))
            provenance=None
            if photo:
                tile=surfaces[key]['physical_tile_metres']
                require(len(tile)==2 and tile[0]==tile[1],
                        'Photographic replacement requires isotropic authored UV density',key)
                # Existing UV is metres/authored_tile; multiplying by authored_tile/2
                # preserves the new source's two-metre repeat without changing any FBX.
                recipe.update(textures=photos[photo]['textures'],metallic=0.,
                              uv_scale=tile[0]/PHOTO_TILE_METRES,normal_strength=1.)
                extra={'normal_gl':True}
                if photo=='concrete_block_wall':
                    recipe['normal_strength']=.3
                    extra['cream_paint']=True
                provenance={'asset':photo,'physical_tile_metres':[2.,2.],
                            'source_manifest':str((MASONRY_MANIFEST if photo=='concrete_block_wall'
                                                   else PHOTO_MANIFEST).relative_to(REPO)),
                            'maps':photos[photo]['provenance']}
                key=None  # Must never mutate or reuse a shared Props material.
            spec={'recipe':recipe,'extra':extra,'surface_key':key,'provenance':provenance}
            signature=json.dumps(spec,sort_keys=True)
            require(recipe['name'] not in shared or shared[recipe['name']]==signature,
                    'Conflicting shell material recipe',recipe['name'])
            shared[recipe['name']]=signature
            materials.append(spec)
        models.append({'row':row,'source':str(source),'lods':lods,'materials':materials})
    return {'models':models,'source_root':str(source_root),'manifest':str(manifest_path)}


def import_plan(plan,report_path):
    import unreal
    require(all(hasattr(unreal,name) for name in ('GeometryScript_AssetUtils','GeometryScript_MeshQueries','GeometryScript_List')),
            'GeometryScripting must be enabled for render readback')
    E=unreal.EditorAssetLibrary
    M=unreal.MaterialEditingLibrary
    S=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
    R=module('eastwing_shell_measured_recipe',HERE.parents[2]/'Signal47/Tools/fieldintegration/import_recipe.py')
    R.check=lambda name,condition,detail=None:require(condition,name,detail)
    report={'status':'RUNNING','meshes':[],'materials':[],
            'scope':'Native shell geometry/materials only; world mounting, sign artwork, doors, collision use, lighting and gameplay remain integration work.'}
    report_path.parent.mkdir(parents=True,exist_ok=True)

    def flush():report_path.write_text(json.dumps(report,indent=2)+'\n')

    def own(path):
        if E.does_asset_exist(path):
            require(E.get_metadata_tag(E.load_asset(path),'S47Owner')==OWNER,
                    'Refusing to overwrite an unrelated native asset',path)

    def mark(asset):
        E.set_metadata_tag(asset,'S47Owner',OWNER)
        require(E.save_loaded_asset(asset,only_if_is_dirty=False),'Native save failed',asset.get_path_name())

    made={}

    def material(spec):
        recipe,extra=spec['recipe'],spec['extra']
        name=recipe['name']
        if name in made:return made[name]
        key=spec['surface_key']
        # Texture-key surfaces use the same unscaled UVs and full-strength PBR maps.
        # Scalar defaults are overridden by those maps; there is no tint/transmission/emission.
        if key and not extra and recipe.get('emission_strength',0)==0:
            shared_path=P.DEST+'/Surfaces/M_'+key
            if E.does_asset_exist(shared_path):
                shared=E.load_asset(shared_path)
                require(isinstance(shared,unreal.Material),'Shared surface is not a material',shared_path)
                require(E.get_metadata_tag(shared,'S47Owner')==P.OWNER,'Shared surface ownership differs',shared_path)
                made[name]=shared
                report['materials'].append({'slot':name,'asset':shared.get_path_name(),'reused':True})
                return shared
        dest=DEST+'/Materials'
        path=dest+'/'+name
        paths=[path]+[dest+'/Textures/T_'+name+'_'+role for role in recipe['textures']]
        for candidate in paths:own(candidate)
        R.TEXTURE_CACHE.clear()
        try:
            mat=R.material(recipe,dest,HERE)
        finally:
            for candidate in paths:
                if E.does_asset_exist(candidate):mark(E.load_asset(candidate))
        for node in M.get_material_expressions(mat):
            if isinstance(node,unreal.MaterialExpressionTextureSample):
                tex=node.get_editor_property('texture')
                if tex and tex.get_editor_property('compression_settings')==unreal.TextureCompressionSettings.TC_NORMALMAP:
                    tex.set_editor_property('flip_green_channel',bool(extra.get('normal_gl')))
                    require(tex.get_editor_property('flip_green_channel')==bool(extra.get('normal_gl')),
                            'Normal convention readback failed',tex.get_path_name())
                    mark(tex)
        if extra.get('cream_paint'):
            # Original is bare exterior block; pigment is an explicit project adaptation.
            samples={}
            for node in M.get_material_expressions(mat):
                if isinstance(node,unreal.MaterialExpressionTextureSample):
                    tex=node.get_editor_property('texture')
                    if tex:
                        for role in ('base_color','roughness'):
                            if tex.get_name()=='T_'+name+'_'+role:samples[role]=node
            require(set(samples)=={'base_color','roughness'},'Missing cream-paint source samples')
            def connect(a,out,b,pin):
                require(M.connect_material_expressions(a,out,b,pin),'Cream-paint connection failed',pin)
            gray=R.expression(mat,unreal.MaterialExpressionDesaturation)
            full=R.expression(mat,unreal.MaterialExpressionConstant,r=1.)
            connect(samples['base_color'],'RGB',gray,'')  # UE shortens the primary Input pin to unnamed.
            connect(full,'',gray,'Fraction')
            variation=R.expression(mat,unreal.MaterialExpressionMultiply,const_b=.25)
            connect(gray,'',variation,'A')
            pigment=R.expression(mat,unreal.MaterialExpressionAdd,const_b=.8)
            connect(variation,'',pigment,'A')
            cream=R.expression(mat,unreal.MaterialExpressionConstant3Vector,
                               constant=unreal.LinearColor(.68,.63,.49))
            color=R.expression(mat,unreal.MaterialExpressionMultiply)
            connect(pigment,'',color,'A')
            connect(cream,'',color,'B')
            require(M.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR),
                    'Cream-paint base-color connection failed')
            rough_scale=R.expression(mat,unreal.MaterialExpressionMultiply,const_b=.15)
            connect(samples['roughness'],'R',rough_scale,'A')
            rough=R.expression(mat,unreal.MaterialExpressionAdd,const_b=.70)
            connect(rough_scale,'',rough,'A')
            require(M.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS),
                    'Cream-paint roughness connection failed')
        if spec.get('provenance'):
            E.set_metadata_tag(mat,'S47SourceProvenance',json.dumps(spec['provenance'],sort_keys=True))
            E.set_metadata_tag(mat,'S47License','CC0-1.0')
        if extra.get('ior'):
            node=R.expression(mat,unreal.MaterialExpressionScalarParameter,parameter_name='IOR',default_value=extra['ior'])
            require(M.connect_material_property(node,'',unreal.MaterialProperty.MP_REFRACTION),'Glass IOR connection failed')
        require(not list(M.recompile_material(mat)),'Shell material compile failed',name)
        mark(mat)
        made[name]=mat
        report['materials'].append({'slot':name,'asset':mat.get_path_name(),'reused':False,
                                    'uv_scale':recipe['uv_scale'],'normal_gl':bool(extra.get('normal_gl')),
                                    'normal_strength':recipe.get('normal_strength'),
                                    'cream_paint':bool(extra.get('cream_paint')),
                                    'provenance':spec.get('provenance')})
        return mat

    flag='Interchange.FeatureFlags.Import.FBX'
    old=unreal.SystemLibrary.get_console_variable_int_value(flag)
    try:
        unreal.SystemLibrary.execute_console_command(None,flag+' 0')
        require(unreal.SystemLibrary.get_console_variable_int_value(flag)==0,'Legacy FBX translator required')
        for model in plan['models']:
            row=model['row']
            source=Path(model['source']).parent
            mats={spec['recipe']['name']:material(spec) for spec in model['materials']}
            own(DEST+'/'+row['asset'])
            mesh=R.import_mesh(source/row['fbx'],DEST)
            mark(mesh)
            mesh.set_editor_property('allow_cpu_access',True)
            for lod,item in enumerate(model['lods'][1:],1):
                require(S.import_lod(mesh,lod,str(source/item['file']))==lod,'Authored shell LOD import failed',[row['asset'],lod])
            slots=[str(slot.material_slot_name) for slot in mesh.get_editor_property('static_materials')]
            require(len(slots)==len(row['materials']) and set(slots)==set(row['materials']),
                    'Native shell material slots changed',[row['asset'],slots])
            for index,name in enumerate(slots):mesh.set_material(index,mats[name])
            nanite=mesh.get_editor_property('nanite_settings')
            nanite.enabled=False
            mesh.set_editor_property('nanite_settings',nanite)
            require(S.get_lod_count(mesh)==3,'Three shell LODs required',row['asset'])
            for lod in range(3):
                build=S.get_lod_build_settings(mesh,lod)
                build.set_editor_property('use_full_precision_u_vs',True)
                build.set_editor_property('remove_degenerates',False)
                S.set_lod_build_settings(mesh,lod,build)
            require(S.set_lod_screen_sizes(mesh,[1.,.42,.16]),'Authored shell LOD screen sizes failed')
            E.set_metadata_tag(mesh,'S47AnchorThreeMetres',json.dumps(row['anchor_three_m']))
            E.set_metadata_tag(mesh,'S47CollisionNote',row['collision'])
            E.set_metadata_tag(mesh,'S47License',row['license']['id'])
            mark(mesh)  # Finish the requested mesh rebuild before reading render triangles.
            lods=[]
            for lod,item in enumerate(model['lods']):
                triangles=mesh.get_num_triangles(lod)
                require(triangles==item['expected_native_triangles'],'Native shell triangle count differs',
                        [row['asset'],lod,triangles,item['expected_native_triangles']])
                for section in range(mesh.get_num_sections(lod)):
                    index=S.get_lod_material_slot(mesh,lod,section)
                    require(0<=index<len(slots),'Unbound shell LOD material section',[row['asset'],lod,section,index])
                options=unreal.GeometryScriptCopyMeshFromAssetOptions()
                requested=unreal.GeometryScriptMeshReadLOD()
                requested.set_editor_property('lod_type',unreal.GeometryScriptLODType.RENDER_DATA)
                requested.set_editor_property('lod_index',lod)
                dynamic,outcome=unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(mesh,unreal.DynamicMesh(),options,requested)
                require(outcome==unreal.GeometryScriptOutcomePins.SUCCESS,'Shell render geometry unavailable')
                _,values,_=unreal.GeometryScript_MeshQueries.get_all_vertex_positions(dynamic,True)
                points=[(p.x,p.y,p.z) for p in unreal.GeometryScript_List.convert_vector_list_to_array(values)]
                require(points and all(math.isfinite(v) for p in points for v in p),'Empty/nonfinite shell geometry')
                bounds=[[fn(p[k] for p in points) for k in range(3)] for fn in (min,max)]
                expected=ue_bounds(item['bounds_m'])
                error=max(abs(a-b) for aa,bb in zip(bounds,expected) for a,b in zip(aa,bb))
                require(error<.15,'Shell axis, scale or pivot mismatch',[row['asset'],lod,bounds,expected])
                lods.append({'lod':lod,'triangles':triangles,'source_triangles':item['triangles'],
                             'bounds_cm':bounds,'bounds_error_cm':error})
            body=mesh.get_editor_property('body_setup')
            geometry=body.get_editor_property('agg_geom') if body else None
            actual_hulls=len(geometry.get_editor_property('convex_elems')) if geometry else 0
            require(actual_hulls==row['collision_hulls'],'Segmented shell UCX count differs',
                    [row['asset'],actual_hulls,row['collision_hulls']])
            report['meshes'].append({'asset':mesh.get_path_name(),'anchor_three_m':row['anchor_three_m'],
                                     'slots':slots,'lods':lods,'collision_hulls':actual_hulls})
            flush()
        report['status']='PASS_NATIVE_SHELL_IMPORT'
        flush()
    except Exception:
        report['status']='FAIL'
        report['error']=traceback.format_exc()
        flush()
        raise
    finally:unreal.SystemLibrary.execute_console_command(None,flag+' '+str(old))
    return report


def main():
    plan=make_plan()
    if os.environ.get('S47_EASTWING_SHELL_APPLY')=='1':
        report_path=Path(os.environ.get('S47_EASTWING_SHELL_REPORT','/tmp/s47-eastwing-shell-import.json')).resolve()
        require(not report_path.is_relative_to(HERE),'Native result belongs outside shell source')
        report=import_plan(plan,report_path)
        print(report['status'],report_path,flush=True)
    else:
        print(json.dumps({'status':'PASS_SHELL_SOURCE_PLAN','models':len(plan['models']),
                          'fbx_files':sum(len(m['lods']) for m in plan['models']),
                          'destination':DEST,'native_import':'NOT_RUN'},indent=2))


if __name__=='__main__':main()
