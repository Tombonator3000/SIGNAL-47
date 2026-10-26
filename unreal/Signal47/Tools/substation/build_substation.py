"""Build original Blender source and game chunks. No render or Unreal launch.
Requires original textures, then Blender5.2.2 background with two threads.
"""
import bpy,bmesh,json,sys,math,hashlib
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parent;sys.path.insert(0,str(R))
from design import PARTS,FLOORS,SITE_WORLD_THREE,WORLD_YAW_DEGREES,REVISION,BUDGET_TRIANGLES
from geometry import build
OUT=R/'exports';OUT.mkdir(exist_ok=True);bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
for m in list(bpy.data.materials):bpy.data.materials.remove(m)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
rows=json.loads((R/'materials.json').read_text());mats={}
for row in rows:
 m=bpy.data.materials.new(row['name']);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=row['base_color_linear'];p.inputs['Metallic'].default_value=row['metallic'];p.inputs['Roughness'].default_value=row['roughness'];p.inputs['Specular IOR Level'].default_value=row['specular'];p.inputs['Emission Color'].default_value=row['emission'];p.inputs['Emission Strength'].default_value=row['emission_strength']
 for role,path in row.get('textures',{}).items():
  t=m.node_tree.nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(R/path),check_existing=True);t.extension='REPEAT';t.image.colorspace_settings.name='sRGB' if role=='base_color' else 'Non-Color'
  if role=='normal':
   sep=m.node_tree.nodes.new('ShaderNodeSeparateXYZ');inv=m.node_tree.nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1;comb=m.node_tree.nodes.new('ShaderNodeCombineXYZ');normal=m.node_tree.nodes.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=row['normal_strength']
   m.node_tree.links.new(t.outputs['Color'],sep.inputs[0]);m.node_tree.links.new(sep.outputs['X'],comb.inputs['X']);m.node_tree.links.new(sep.outputs['Y'],inv.inputs[1]);m.node_tree.links.new(inv.outputs[0],comb.inputs['Y']);m.node_tree.links.new(sep.outputs['Z'],comb.inputs['Z']);m.node_tree.links.new(comb.outputs[0],normal.inputs['Color']);m.node_tree.links.new(normal.outputs[0],p.inputs['Normal'])
  else:m.node_tree.links.new(t.outputs['Color'],p.inputs['Base Color' if role=='base_color' else 'Roughness'])
 mats[row['name']]=m
source=bpy.data.collections.new('EDITABLE_ORIGINAL_COMPONENTS');scene.collection.children.link(source);source.hide_viewport=True;source.hide_render=True
parts={};lodstats={};inventory={}
for lod in [0,1,2]:
 geometry=build(lod);lodstats[str(lod)]={}
 for name,data in geometry.items():
  offset=Vector(PARTS[name]['offset']);mesh=bpy.data.meshes.new(name+('_LOD'+str(lod) if lod else ''));mesh.from_pydata([Vector(p)-offset for p in data.v],[],data.f);mesh.update()
  used=[row['name'] for row in rows if row['name'] in set(data.mat)]
  for mat in used:mesh.materials.append(mats[mat])
  uv=mesh.uv_layers.new(name='UVMap')
  tagverts={}
  for poly,mat,coords,smooth,tag in zip(mesh.polygons,data.mat,data.uv,data.smooth,data.tags):
   poly.material_index=used.index(mat);poly.use_smooth=smooth
   for li,t in zip(poly.loop_indices,coords):uv.data[li].uv=t
   tagverts.setdefault(tag,[]).extend(poly.vertices)
  ob=bpy.data.objects.new(name+('_LOD'+str(lod) if lod else ''),mesh);scene.collection.objects.link(ob);bpy.context.view_layer.objects.active=ob;ob.select_set(True)
  mod=ob.modifiers.new('Explicit_triangle_export','TRIANGULATE');mod.quad_method='FIXED';bpy.ops.object.modifier_apply(modifier=mod.name)
  assert all(p.area>=5e-9 for p in ob.data.polygons),(name,lod,'triangle below UE import threshold')
  # Store authoring groups before welding so BMesh preserves deform weights.
  if lod==0:
   for tag,ids in tagverts.items():ob.vertex_groups.new(name=tag[:63]).add(list(set(ids)),1,'REPLACE')
  before_weld=len(ob.data.vertices);bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-6);bm.to_mesh(ob.data);bm.free();ob.data.update()
  assert all(p.area>=5e-9 for p in ob.data.polygons),(name,lod,'weld created tiny triangle')
  coords=[tuple(v.co) for v in ob.data.vertices];bounds=[[fn(p[k] for p in coords) for k in range(3)] for fn in [min,max]];tris=len(ob.data.polygons)
  row={'triangles':tris,'bounds_local_m':bounds,'materials':used,'used_materials':sorted(used),'site_local_offset_m':list(offset),'origin':[0,0,0],'vertices_before_weld':before_weld,'vertices_after_weld':len(ob.data.vertices)};lodstats[str(lod)][name]=row
  if lod==0:
   ob['original_source']=REVISION;ob['site_local_offset_m']=list(offset);ob['role']=PARTS[name]['role'];ob['collision_authority']='Native runtime owns floor/collision; visual mesh has no generated collider';ob['component_inventory']=json.dumps({k:len(v) for k,v in tagverts.items()})
   original=ob.copy();original.data=ob.data.copy();original.name='SRC_'+name;source.objects.link(original);original.location=offset
   parts[name]=row;inventory[name]={'role':PARTS[name]['role'],'components':sorted(tagverts),'component_count':len(tagverts)}
  bpy.ops.object.select_all(action='DESELECT');ob.select_set(True)
  suffix='' if lod==0 else '_LOD'+str(lod)
  bpy.ops.export_scene.fbx(filepath=str(OUT/(name+suffix+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
  if lod==0:
   bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
   ob.location=offset
  else:bpy.data.objects.remove(ob,do_unlink=True)
assert sum(row['triangles'] for row in parts.values())<=BUDGET_TRIANGLES
# Export assembled backup without editable duplicates or preview lights.
bpy.ops.object.select_all(action='DESELECT')
for ob in scene.objects:
 if ob.type=='MESH' and ob.name in PARTS:ob.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(R/'Caliche_Junction_Substation.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
# Asymmetric probe has noSM prefix and is absent from the production source.
bpy.ops.object.select_all(action='DESELECT');bpy.ops.mesh.primitive_cube_add(size=1,location=(2,4,6));probe=bpy.context.object;probe.name='axis_probe';probe.dimensions=(.2,.4,.6);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
bpy.ops.export_scene.fbx(filepath=str(OUT/'axis_probe.fbx'),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False);bpy.data.objects.remove(probe,do_unlink=True)
for image in bpy.data.images:
 if image.source=='FILE':image.pack();image.filepath=bpy.path.relpath(image.filepath,start=str(R))
for name,row in parts.items():row['lod_triangles']=[lodstats[str(i)][name]['triangles'] for i in [0,1,2]]
contract={'status':'SOURCE_GENERATED_NATIVE_UNVERIFIED','source_revision':REVISION,'units':'metres','blender_up':'Z','front':'negativeY','site_world_three_m':SITE_WORLD_THREE,'world_yaw_degrees':WORLD_YAW_DEGREES,'mapping':'worldThree=(7890-bx,-1.10+bz,536.20+by)','parts':parts,'lods':lodstats,'floor_surfaces':FLOORS,'inventory':inventory,'three_HV_terminal_centres_site_m':[[-1.04,.05,5.97],[0,.05,5.97],[1.04,.05,5.97]],'control_face_site_m':[5,-8.935,1.2],'operator_site_m':[5,-10.4,0],'lever_rotation_axis_local':'X','gate_rotation_axis_local':'Z','gate_clear_width_m':4.4,'axis_probe_expected_UE_center_cm':[200,-400,600],'axis_probe_source_center_m':[2,4,6],'axis_probe_source_dimensions_m':[.2,.4,.6],'collision':'Native integration owns explicit floors/proxies; no collision generated in authoring','total_LOD0_triangles':sum(row['triangles'] for row in parts.values()),'originality':'All geometry, maps and fictional signage original; no external asset dependencies.'}
(R/'model_contract.json').write_text(json.dumps(contract,indent=2)+'\n')
bpy.ops.wm.save_as_mainfile(filepath=str(R/'Caliche_Junction_Substation.blend'),compress=True)
print('SUBSTATION_BUILD_PASS',json.dumps({'parts':len(parts),'total_triangles':contract['total_LOD0_triangles'],'lod_totals':{k:sum(x['triangles'] for x in v.values()) for k,v in lodstats.items()}}))
