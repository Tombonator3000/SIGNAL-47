"""Blender export only. Root owns scheduling, guard, rendering and native jobs."""
import bpy, bmesh, json, sys
from pathlib import Path
R=Path(__file__).resolve().parent;sys.path.insert(0,str(R))
from design import REVISION,PARTS,BUDGET_TRIANGLES,PRESERVE_HANDLES,NATIVE_DYNAMIC,ROOM_BOUNDS_THREE
from geometry import build,T
OUT=R/'exports';OUT.mkdir(exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
for mat in list(bpy.data.materials):bpy.data.materials.remove(mat)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
rows=json.loads((R/'materials.json').read_text());mats={}
for row in rows:
 m=bpy.data.materials.new(row['name']);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=row['base_color_linear'];p.inputs['Metallic'].default_value=row['metallic'];p.inputs['Roughness'].default_value=row['roughness'];p.inputs['Specular IOR Level'].default_value=row['specular'];m.use_backface_culling=True
 for role,path in row.get('textures',{}).items():
  t=m.node_tree.nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(R/path),check_existing=True);t.image.colorspace_settings.name='sRGB'if role=='base_color'else'Non-Color';t.extension='REPEAT'
  if role=='normal':
   sep=m.node_tree.nodes.new('ShaderNodeSeparateXYZ');inv=m.node_tree.nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1;comb=m.node_tree.nodes.new('ShaderNodeCombineXYZ');normal=m.node_tree.nodes.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=row['normal_strength']
   for a,ao,b,bi in [(t,'Color',sep,0),(sep,'X',comb,'X'),(sep,'Y',inv,1),(inv,0,comb,'Y'),(sep,'Z',comb,'Z'),(comb,0,normal,'Color'),(normal,'Normal',p,'Normal')]:m.node_tree.links.new(a.outputs[ao],b.inputs[bi])
  else:m.node_tree.links.new(t.outputs['Color'],p.inputs['Base Color'if role=='base_color'else'Roughness'])
 mats[row['name']]=m
source=bpy.data.collections.new('EDITABLE_ORIGINAL_ARCHIVE');scene.collection.children.link(source);source.hide_viewport=True;source.hide_render=True
lodstats={};parts={}
for lod in [0,1,2]:
 lodstats[str(lod)]={}
 for name,data in build(lod).items():
  suffix=''if lod==0 else'_LOD'+str(lod);mesh=bpy.data.meshes.new(name+suffix);mesh.from_pydata(data.v,[],data.f);mesh.update();used=[r['name']for r in rows if r['name']in set(data.mat)]
  for mat in used:mesh.materials.append(mats[mat])
  uv=mesh.uv_layers.new(name='UVMap');groups={}
  for poly,mat,coords,smooth,tag in zip(mesh.polygons,data.mat,data.uv,data.smooth,data.tags):
   poly.material_index=used.index(mat);poly.use_smooth=smooth
   for li,t in zip(poly.loop_indices,coords):uv.data[li].uv=t
   groups.setdefault(tag,[]).extend(poly.vertices)
  ob=bpy.data.objects.new(name+suffix,mesh);scene.collection.objects.link(ob);bpy.context.view_layer.objects.active=ob;ob.select_set(True)
  if lod==0:
   for tag,ids in groups.items():ob.vertex_groups.new(name=tag[:63]).add(list(set(ids)),1,'REPLACE')
  mod=ob.modifiers.new('Explicit_triangles','TRIANGULATE');mod.quad_method='FIXED';bpy.ops.object.modifier_apply(modifier=mod.name)
  bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-6);bm.to_mesh(ob.data);bm.free();ob.data.update()
  assert all(p.area>=5e-9 for p in ob.data.polygons),(name,lod,'degenerate triangle')
  points=[tuple(v.co)for v in ob.data.vertices];bounds=[[f(p[k]for p in points)for k in range(3)]for f in [min,max]]
  stat={'triangles':len(ob.data.polygons),'bounds_local_m':bounds,'materials':used,'used_materials':sorted(used),'origin':[0,0,0],**PARTS[name]};lodstats[str(lod)][name]=stat
  ob['original_source']=REVISION;ob['role']=PARTS[name]['role'];ob['native_parent']=PARTS[name]['parent_handle'];ob['component_inventory']=json.dumps(sorted(groups))
  bpy.ops.object.select_all(action='DESELECT');ob.select_set(True)
  bpy.ops.export_scene.fbx(filepath=str(OUT/(name+suffix+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
  bpy.ops.export_scene.gltf(filepath=str(OUT/(name+suffix+'.glb')),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
  if lod==0:
   parts[name]=stat;copy=ob.copy();copy.data=ob.data.copy();copy.name='SRC_'+name;source.objects.link(copy);copy.location=T(PARTS[name]['site_world_three_m']);copy.rotation_euler=PARTS[name].get('preview_rotation_blender',[0,0,0]);ob.location=copy.location;ob.rotation_euler=copy.rotation_euler
  else:bpy.data.objects.remove(ob,do_unlink=True)
for name,row in parts.items():row['lod_triangles']=[lodstats[str(i)][name]['triangles']for i in [0,1,2]]
total=sum(x['triangles']for x in parts.values());assert total<=BUDGET_TRIANGLES,(total,BUDGET_TRIANGLES)
bpy.ops.object.select_all(action='DESELECT')
for ob in scene.objects:
 if ob.name in PARTS:ob.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(R/'SARO_Archive.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
bpy.ops.object.select_all(action='DESELECT');bpy.ops.mesh.primitive_cube_add(size=1,location=(2,4,6));probe=bpy.context.object;probe.name='axis_probe';probe.dimensions=(.2,.4,.6);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
bpy.ops.export_scene.fbx(filepath=str(OUT/'axis_probe.fbx'),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False);bpy.data.objects.remove(probe,do_unlink=True)
for im in bpy.data.images:
 if im.source=='FILE':im.pack();im.filepath=bpy.path.relpath(im.filepath,start=str(R))
contract={'status':'SOURCE_GENERATED_NATIVE_UNVERIFIED','source_revision':REVISION,'units':'metres','blender_up':'Z','mapping':'Blender=(three.x,-three.z,three.y); imported UE=(100bx,-100by,100bz)','asset_root':'/Game/ArchiveProps','parts':parts,'lods':lodstats,'total_LOD0_triangles':total,'room_bounds_three_xxzz':ROOM_BOUNDS_THREE,'preserve_handles':PRESERVE_HANDLES,'native_dynamic':NATIVE_DYNAMIC,'collision':'Existing room rectangles, door zones and all interaction proxies stay authoritative. No new collision.','axis_probe_expected_UE_center_cm':[200,-400,600],'axis_probe_expected_UE_dimensions_cm':[20,40,60],'originality':'Original geometry and textures; project-owned substation mesh primitives and system DejaVu raster labels only.','verification':'Generated source is not native import, gameplay or visual acceptance.'}
(R/'model_contract.json').write_text(json.dumps(contract,indent=2)+'\n');bpy.ops.wm.save_as_mainfile(filepath=str(R/'SARO_Archive.blend'),compress=True)
print('ARCHIVEPROPS_BUILD_PASS',json.dumps({'parts':len(parts),'triangles':total,'lod_totals':{k:sum(x['triangles']for x in v.values())for k,v in lodstats.items()}}))
