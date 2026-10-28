"""Original six-family architecture builder. Root owns the exclusive DCC slot.
Run: blender -b -t 2 --python build_townbuildings.py
No render/editor/native launch. Uses the accepted legacy FBX metres/axis recipe.
"""
import bpy,bmesh,json,sys,math
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parent;sys.path.insert(0,str(R))
from design import BUILDINGS,PARTS,REVISION,PROTECTED_RUNTIME,ROOM6_ROOT,ROOM6_PROTECTED,MOTEL_PROTECTED
import town_geometry,motel_geometry,importlib.util
# TB2: the five town landmarks come from tb2/<family>.py; the motel stays on motel_geometry.
TB2={}
for _family in ['Sheriff','Laundry','Feed','Cinema','Church']:
 _spec=importlib.util.spec_from_file_location('tb2_'+_family.lower(),R/'tb2'/(_family.lower()+'.py'));TB2[_family]=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(TB2[_family])
ROWS=json.loads((R/'materials.json').read_text())
for _extra in sorted((R/'tb2').glob('materials_*.json')):ROWS+=json.loads(_extra.read_text())
ROW={r['name']:r for r in ROWS};OUT=R/'exports';OUT.mkdir(exist_ok=True)
parts={};lodstats={str(k):{}for k in range(3)};families={}
def clear():
 for ob in list(bpy.data.objects):bpy.data.objects.remove(ob,do_unlink=True)
 for c in list(bpy.data.collections):bpy.data.collections.remove(c)
 for m in list(bpy.data.materials):bpy.data.materials.remove(m)
 for im in list(bpy.data.images):bpy.data.images.remove(im)
 for scene in list(bpy.data.scenes)[1:]:bpy.data.scenes.remove(scene)
 scene=bpy.data.scenes[0];scene.name='Architecture';bpy.context.window.scene=scene
 scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
 return scene

def material(name):
 if name in bpy.data.materials:return bpy.data.materials[name]
 r=ROW[name];m=bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=r['base_color_linear'];p.inputs['Metallic'].default_value=r['metallic'];p.inputs['Roughness'].default_value=r['roughness'];p.inputs['Specular IOR Level'].default_value=r.get('specular',.28);p.inputs['Emission Color'].default_value=r.get('emission',[0,0,0,1]);p.inputs['Emission Strength'].default_value=r.get('emission_strength',0);m.use_backface_culling=not r.get('two_sided',False)
 if r.get('alpha',1)<1:p.inputs['Alpha'].default_value=r['alpha'];m.surface_render_method='DITHERED'
 for role,path in r.get('textures',{}).items():
  t=m.node_tree.nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(R/path),check_existing=True);t.image.colorspace_settings.name='sRGB'if role=='base_color'else'Non-Color'
  if role=='normal':
   # Native maps areDirectX. Flipgreen only in the editable Blender/OpenGL preview graph.
   sep=m.node_tree.nodes.new('ShaderNodeSeparateXYZ');inv=m.node_tree.nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1;comb=m.node_tree.nodes.new('ShaderNodeCombineXYZ');normal=m.node_tree.nodes.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=r.get('normal_strength',.2)
   m.node_tree.links.new(t.outputs['Color'],sep.inputs[0]);m.node_tree.links.new(sep.outputs['X'],comb.inputs['X']);m.node_tree.links.new(sep.outputs['Y'],inv.inputs[1]);m.node_tree.links.new(inv.outputs[0],comb.inputs['Y']);m.node_tree.links.new(sep.outputs['Z'],comb.inputs['Z']);m.node_tree.links.new(comb.outputs[0],normal.inputs['Color']);m.node_tree.links.new(normal.outputs[0],p.inputs['Normal'])
  elif role=='base_color':
   rgb=m.node_tree.nodes.new('ShaderNodeMixRGB');rgb.blend_type='MULTIPLY';rgb.inputs[0].default_value=1;rgb.inputs[2].default_value=r.get('texture_tint_linear',[1,1,1,1]);m.node_tree.links.new(t.outputs['Color'],rgb.inputs[1]);m.node_tree.links.new(rgb.outputs[0],p.inputs['Base Color'])
  else:m.node_tree.links.new(t.outputs['Color'],p.inputs['Roughness'])
 return m

def export_fbx(path):
 bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
def glb(path):
 # Selection is per scene. Limit glTF to the active scene so the other Motel
 # scene's last selection cannot leak into a per-part or family export.
 bpy.ops.export_scene.gltf(filepath=str(path),export_format='GLB',use_selection=True,use_active_scene=True,export_materials='EXPORT',export_extras=True)
def bounds(ob):return [[fn(v.co[k]for v in ob.data.vertices)for k in range(3)]for fn in [min,max]]

for family in BUILDINGS:
 scene=clear();scene.name=family+'_Court'if family=='Motel'else family
 src=bpy.data.collections.new('EDITABLE_ORIGINAL_'+family.upper());scene.collection.children.link(src);src.hide_render=True;src.hide_viewport=True
 roomscene=None
 if family=='Motel':
  roomscene=bpy.data.scenes.new('Room6');roomscene.unit_settings.system='METRIC';roomscene.unit_settings.scale_length=1
  roomsrc=bpy.data.collections.new('EDITABLE_ORIGINAL_ROOM6');roomscene.collection.children.link(roomsrc);roomsrc.hide_render=True;roomsrc.hide_viewport=True
 family_names=[]
 for lod in range(3):
  data=motel_geometry.build(lod)if family=='Motel'else TB2[family].build(lod)
  for name,shape in data.items():
   spec=PARTS[name];isroom=spec['site_world_three_m']==ROOM6_ROOT;target=roomscene if isroom else scene;bpy.context.window.scene=target
   suffix=''if lod==0 else'_LOD'+str(lod);mesh=bpy.data.meshes.new(name+suffix);mesh.from_pydata(shape.v,[],shape.f);mesh.update()
   used=[r['name']for r in ROWS if r['name']in set(shape.mat)]
   for mat in used:mesh.materials.append(material(mat))
   uv=mesh.uv_layers.new(name='UVMap');groups={}
   for p,mat,coords,smooth,tag in zip(mesh.polygons,shape.mat,shape.uv,shape.smooth,shape.tags):
    p.material_index=used.index(mat);p.use_smooth=smooth
    for li,t in zip(p.loop_indices,coords):uv.data[li].uv=t
    groups.setdefault(tag,[]).extend(p.vertices)
   ob=bpy.data.objects.new(name+suffix,mesh);target.collection.objects.link(ob);bpy.context.view_layer.objects.active=ob;ob.select_set(True)
   if lod==0:
    for tag,ids in groups.items():ob.vertex_groups.new(name=tag[:63]).add(sorted(set(ids)),1,'REPLACE')
   mod=ob.modifiers.new('Explicit_triangles','TRIANGULATE');mod.quad_method='FIXED';bpy.ops.object.modifier_apply(modifier=mod.name)
   before=len(ob.data.vertices);bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-6);bm.to_mesh(ob.data);bm.free();ob.data.update()
   assert all(p.area>=5e-9 for p in ob.data.polygons),(name,lod,'Unreal unstable triangle')
   u=ob.data.uv_layers.active.data;minuv=math.inf
   for p in ob.data.polygons:
    a,b,c=[u[li].uv for li in p.loop_indices];minuv=min(minuv,abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))*.5)
   assert minuv>1e-12,(name,lod,'collapsed triangleUV',minuv)
   row={'triangles':len(ob.data.polygons),'bounds_local_m':bounds(ob),'materials':used,'used_materials':sorted(used),'site_local_offset_m':spec['offset'],'site_world_three_m':spec['site_world_three_m'],'parent_handle':spec['parent_handle'],'origin':[0,0,0],'vertices_before_weld':before,'vertices_after_weld':len(ob.data.vertices),'minimum_uv_area':minuv,'building':family,'chunk':spec['chunk']};lodstats[str(lod)][name]=row
   if lod==0:
    parts[name]=row;family_names.append(name);ob['original_source']=REVISION;ob['world_mapping']='worldThree=(rootX+Bx,rootY+Bz,rootZ-By)';ob['site_world_three_m']=spec['site_world_three_m'];ob['role']=spec['role'];ob['collision_authority']='Original gameplay zones/colliders preserved; no new admission';ob['component_inventory']=json.dumps(sorted(groups))
    copy=ob.copy();copy.data=ob.data.copy();copy.name='SRC_'+name;(roomsrc if isroom else src).objects.link(copy)
   bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);export_fbx(OUT/(name+suffix+'.fbx'))
   if lod==0:glb(OUT/(name+'.glb'))
   else:bpy.data.objects.remove(ob,do_unlink=True)
 for target in [scene]+([roomscene]if roomscene else[]):
  bpy.context.window.scene=target;bpy.ops.object.select_all(action='DESELECT')
  for ob in target.objects:
   if ob.name in parts:ob.select_set(True)
  glb(R/(target.name+'.glb'))
 # Pack only the maps actually used by this family, with relative editable dependency paths.
 for im in bpy.data.images:
  if im.source=='FILE':im.pack();im.filepath=bpy.path.relpath(im.filepath,start=str(R))
 bpy.context.window.scene=scene
 scene['source_revision']=REVISION;scene['family']=family;scene['native_visual_status']='UNVERIFIED'
 scene['existing_gameplay_objects_retained']=json.dumps(PROTECTED_RUNTIME)
 path=R/(family+'.blend');bpy.ops.wm.save_as_mainfile(filepath=str(path),compress=True)
 families[family]={'blend':path.name,'scenes':[scene.name]+([roomscene.name]if roomscene else[]),'editable_collections':[src.name]+([roomsrc.name]if roomscene else[]),'parts':family_names,'site_world_three_m':BUILDINGS[family]['root'],'source_style':BUILDINGS[family]['style']}
 print('TOWN_FAMILY_SOURCE_READY',family,sum(parts[n]['triangles']for n in family_names))
for name,row in parts.items():row['lod_triangles']=[lodstats[str(k)][name]['triangles']for k in range(3)]
# Identical measured pivot probe to every accepted original-model import.
bpy.context.window.scene=scene;bpy.ops.object.select_all(action='DESELECT');bpy.ops.mesh.primitive_cube_add(size=1,location=(2,4,6));probe=bpy.context.object;probe.dimensions=(.2,.4,.6);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);export_fbx(OUT/'axis_probe.fbx');bpy.data.objects.remove(probe,do_unlink=True)
contract={'status':'SOURCE_GENERATED_NATIVE_UNVERIFIED','source_revision':REVISION,'units':'metres','blender_up':'Z','mapping':'worldThree=(part.rootX+Bx,part.rootY+Bz,part.rootZ-By), runtime yaw0','native_import_policy':{'force_front_x_axis':True,'coordinate_system_policy':'MATCH_UP_FORWARD_AXES','import_rotation_yaw':90},'parts':parts,'lods':lodstats,'families':families,'total_LOD0_triangles':sum(r['triangles']for r in parts.values()),'axis_probe_source_center_m':[2,4,6],'axis_probe_source_dimensions_m':[.2,.4,.6],'axis_probe_expected_UE_center_cm':[200,-400,600],'protected_runtime':PROTECTED_RUNTIME,'room6_protected_blender_m':ROOM6_PROTECTED,'motel_protected_blender_m':MOTEL_PROTECTED,'top_authorities_m':{'Town':0,'Feed_dock_and_interior':.95,'Motel_Court':0,'Room6':0,'Room6_table':.810,'Motel_counter':1.020},'collision':'No new game admission; original floor/collider/proxy remains authority. Shell/Interior usable as complex visibility queries only when integration explicitly opts in.','originality':'All authored geometry and nine shared procedural PBR families original. Read-only reuse of our own substation mesh primitives. Existing own bed and live media/phone/lamp/NPC are retained, not duplicated.'}
assert len(parts)==28;assert contract['total_LOD0_triangles']<250000
(R/'model_contract.json').write_text(json.dumps(contract,indent=2)+'\n');print('TOWNBUILDINGS_BUILD_PASS',json.dumps({'parts':len(parts),'families':len(families),'triangles':contract['total_LOD0_triangles'],'lod_totals':{k:sum(r['triangles']for r in d.values())for k,d in lodstats.items()}}))
