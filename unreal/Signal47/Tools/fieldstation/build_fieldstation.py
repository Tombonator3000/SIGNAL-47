"""Original STATION01 architecture source. Run only under root's exclusive DCC slot.
blender -b -t 2 --python build_fieldstation.py
The generator exports and saves sources; it never imports native assets or renders.
"""
import bpy
import bmesh
import json
import math
import sys
from pathlib import Path
R=Path(__file__).resolve().parent
sys.path.insert(0,str(R))
import geometry
from design import APERTURES,COVERAGE,EXCLUDED,HUT,PARTS,PROTECTED_RUNTIME,REVISION,ROOT_THREE

ROWS=json.loads((R/'materials.json').read_text());ROW={r['name']:r for r in ROWS}
OUT=R/'exports';OUT.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.name='Station01_Field_Hut'
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
source=bpy.data.collections.new('EDITABLE_ORIGINAL_STATION01');scene.collection.children.link(source)
source.hide_viewport=True;source.hide_render=True
lodstats={str(i):{} for i in range(3)};parts={}

def material(name):
    if name in bpy.data.materials:return bpy.data.materials[name]
    row=ROW[name];m=bpy.data.materials.new(name);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=row['base_color_linear'];p.inputs['Metallic'].default_value=row['metallic'];p.inputs['Roughness'].default_value=row['roughness'];p.inputs['Specular IOR Level'].default_value=row.get('specular',.28)
    m.use_backface_culling=not row.get('two_sided',False)
    if row.get('alpha',1)<1:p.inputs['Alpha'].default_value=row['alpha'];m.surface_render_method='DITHERED'
    for role,path in row.get('textures',{}).items():
        tex=m.node_tree.nodes.new('ShaderNodeTexImage');tex.label=role
        tex.image=bpy.data.images.load(str(R/path),check_existing=True)
        tex.image.colorspace_settings.name='sRGB' if role=='base_color' else 'Non-Color'
        if role=='normal':
            sep=m.node_tree.nodes.new('ShaderNodeSeparateXYZ');inv=m.node_tree.nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1
            comb=m.node_tree.nodes.new('ShaderNodeCombineXYZ');normal=m.node_tree.nodes.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=row.get('normal_strength',.2)
            for a,b in [(tex.outputs['Color'],sep.inputs[0]),(sep.outputs['X'],comb.inputs['X']),(sep.outputs['Y'],inv.inputs[1]),(inv.outputs[0],comb.inputs['Y']),(sep.outputs['Z'],comb.inputs['Z']),(comb.outputs[0],normal.inputs['Color']),(normal.outputs[0],p.inputs['Normal'])]:m.node_tree.links.new(a,b)
        elif role=='base_color':
            mix=m.node_tree.nodes.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY';mix.inputs[0].default_value=1;mix.inputs[2].default_value=row.get('texture_tint_linear',[1,1,1,1]);m.node_tree.links.new(tex.outputs['Color'],mix.inputs[1]);m.node_tree.links.new(mix.outputs[0],p.inputs['Base Color'])
        else:m.node_tree.links.new(tex.outputs['Color'],p.inputs['Roughness'])
    return m

def fbx(path):
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
def glb(path):bpy.ops.export_scene.gltf(filepath=str(path),export_format='GLB',use_selection=True,use_active_scene=True,export_materials='EXPORT',export_extras=True)
def bounds(ob):return [[fn(v.co[k] for v in ob.data.vertices) for k in range(3)] for fn in [min,max]]

for lod in range(3):
    for name,shape in geometry.build(lod).items():
        spec=PARTS[name];suffix='' if lod==0 else '_LOD'+str(lod)
        mesh=bpy.data.meshes.new(name+suffix);mesh.from_pydata(shape.v,[],shape.f);mesh.update()
        used=[r['name'] for r in ROWS if r['name'] in set(shape.mat)]
        for name_m in used:mesh.materials.append(material(name_m))
        uv=mesh.uv_layers.new(name='UVMap');groups={}
        for face,mat,coords,smooth,tag in zip(mesh.polygons,shape.mat,shape.uv,shape.smooth,shape.tags):
            face.material_index=used.index(mat);face.use_smooth=smooth
            for li,t in zip(face.loop_indices,coords):uv.data[li].uv=t
            groups.setdefault(tag,[]).extend(face.vertices)
        ob=bpy.data.objects.new(name+suffix,mesh);scene.collection.objects.link(ob)
        bpy.context.view_layer.objects.active=ob;ob.select_set(True)
        if lod==0:
            for tag,ids in groups.items():ob.vertex_groups.new(name=tag[:63]).add(sorted(set(ids)),1,'REPLACE')
        tri=ob.modifiers.new('Explicit_stable_triangles','TRIANGULATE');tri.quad_method='FIXED';bpy.ops.object.modifier_apply(modifier=tri.name)
        mesh=ob.data
        bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-6);bm.to_mesh(mesh);bm.free();mesh.update()
        assert all(p.area>=5e-9 for p in mesh.polygons),(name,lod,'unstable native triangle')
        u=mesh.uv_layers.active.data;minuv=math.inf
        for p in mesh.polygons:
            a,b,c=[u[i].uv for i in p.loop_indices];minuv=min(minuv,abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))*.5)
        assert minuv>1e-12,(name,lod,'collapsed UV')
        isdoor=name.endswith('_Door')
        row={'triangles':len(mesh.polygons),'bounds_local_m':bounds(ob),'materials':used,'lod':lod,'origin':[0,0,0], 'local':isdoor,'at':[0,0,0],**spec,'site_local_offset_m':[0,0,0],'minimum_uv_area':minuv,'component_inventory':sorted(groups)}
        lodstats[str(lod)][name]=row
        if lod==0:
            parts[name]=row
            ob['original_source']=REVISION;ob['role']=spec['role'];ob['parent_handle']=spec['parent_handle'];ob['native_visual_status']='UNVERIFIED';ob['world_mapping']='Three=(rootX+Bx,rootY+Bz,rootZ-By)';ob['component_inventory']=json.dumps(sorted(groups))
            copy=ob.copy();copy.data=mesh.copy();copy.name='SRC_'+name;source.objects.link(copy)
        bpy.ops.object.select_all(action='DESELECT');ob.select_set(True)
        fbx(OUT/(name+suffix+'.fbx'));glb(OUT/(name+suffix+'.glb'))
        if lod>0:bpy.data.objects.remove(ob,do_unlink=True)

for name,row in parts.items():row['lod_triangles']=[lodstats[str(i)][name]['triangles'] for i in range(3)]
bpy.ops.object.select_all(action='DESELECT')
for name in parts:
    if name.endswith('_Door'):continue
    bpy.data.objects[name].select_set(True)
# Door is placed only in the assembled preview copy, never in per-part exports.
door=bpy.data.objects['SM_FieldStation_Hut_Door']
door.location=geometry.B(-9.45,0,4.48);door.rotation_euler[2]=-1.75;door.select_set(True)
glb(R/'Station01_Field_Hut.glb')
door.location=(0,0,0);door.rotation_euler=(0,0,0)
# A source preview uses a parent empty, keeping the native door part at true zero.
hinge=bpy.data.objects.new('PREVIEW_hutDoorLeaf_existing_game_pivot',None);scene.collection.objects.link(hinge)
hinge.location=geometry.B(-9.45,0,4.48);hinge.rotation_euler[2]=-1.75;door.parent=hinge
hinge['runtime_parent_handle']='world.site.objs.hutDoorLeaf'
for im in bpy.data.images:
    if im.source=='FILE':im.pack();im.filepath=bpy.path.relpath(im.filepath,start=str(R))
scene['original_source']=REVISION;scene['protected_runtime']=json.dumps(PROTECTED_RUNTIME);scene['excluded_runtime_objects']=json.dumps(EXCLUDED)
bpy.ops.wm.save_as_mainfile(filepath=str(R/'Station01_Field_Hut.blend'),compress=True)
# Probe uses the same measured policy as townbuildings and Mesa Diner.
bpy.ops.object.select_all(action='DESELECT');bpy.ops.mesh.primitive_cube_add(size=1,location=(2,4,6));probe=bpy.context.object;probe.dimensions=(.2,.4,.6)
bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);fbx(OUT/'axis_probe.fbx');bpy.data.objects.remove(probe,do_unlink=True)
contract={'status':'SOURCE_GENERATED_NATIVE_UNVERIFIED','source_revision':REVISION,'units':'metres','blender_up':'Z',
          'mapping':'worldThree=(rootX+Bx,rootY+Bz,rootZ-By); door vertices are hinge-local, inherit existing parent transform',
          'native_import_policy':{'force_front_x_axis':True,'coordinate_system_policy':'MATCH_UP_FORWARD_AXES','import_rotation_yaw':90},
          'parts':parts,'lods':lodstats,'total_LOD0_triangles':sum(p['triangles'] for p in parts.values()),
          'axis_probe_source_center_m':[2,4,6],'axis_probe_source_dimensions_m':[.2,.4,.6],'axis_probe_expected_UE_center_cm':[200,-400,600],
          'site_world_three_m':ROOT_THREE,'hut':HUT,'apertures':APERTURES,'coverage':COVERAGE,'protected_runtime':PROTECTED_RUNTIME,'excluded':EXCLUDED,
          'collision':'Existing hut floor, walking zones, colliders and interaction proxies remain authority. No new collision or story admission implied.',
          'originality':'Original geometry and labels; read-only use of own substation mesh primitives and own townbuildings procedural PBR maps.',
          'preview_door':'Saved .blend presents the open door under PREVIEW_hutDoorLeaf_existing_game_pivot. Per-part FBX/GLB stay hinge-local at zero.'}
(R/'model_contract.json').write_text(json.dumps(contract,indent=2)+'\n')
print('FIELDSTATION_BUILD_PASS',json.dumps({'parts':len(parts),'triangles':contract['total_LOD0_triangles'],'lod_totals':{lod:sum(p['triangles'] for p in rows.values()) for lod,rows in lodstats.items()}}))
