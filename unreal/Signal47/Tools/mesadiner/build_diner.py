"""Original complete diner, same tested FBX/PBR recipe as the accepted substation.
Only run after the parent's explicit heavy-job grant. No render/native launch.
"""
import bpy,bmesh,json,sys,math
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parent;sys.path.insert(0,str(R))
from diner_geometry import PARTS as DINER_PARTS,ROOT
import diner_geometry
exterior_geometry=None
if (R/'exterior_geometry.py').exists():
 import exterior_geometry
REVISION='R1-original-Mesa-Diner-1986';SITE_WORLD_THREE=ROOT;WORLD_YAW_DEGREES=0;BUDGET_TRIANGLES=125000
PARTS=dict(DINER_PARTS)
def build(lod):
 data=diner_geometry.build(lod)
 if exterior_geometry:
  additions=exterior_geometry.build(lod)
  for name in additions:PARTS.setdefault(name,{'offset':[0,0,0],'role':'Original exterior truck, sign, parking and bench module'})
  data.update(additions)
 return data
OUT=R/'exports';OUT.mkdir(exist_ok=True);bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
for m in list(bpy.data.materials):bpy.data.materials.remove(m)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
rows=json.loads((R/'materials.json').read_text());mats={}
for row in rows:
 m=bpy.data.materials.new(row['name']);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=row['base_color_linear'];p.inputs['Metallic'].default_value=row['metallic'];p.inputs['Roughness'].default_value=row['roughness'];p.inputs['Specular IOR Level'].default_value=row.get('specular',.3);p.inputs['Emission Color'].default_value=row.get('emission',[0,0,0,1]);p.inputs['Emission Strength'].default_value=row.get('emission_strength',0)
 m.use_backface_culling=not row.get('two_sided',False)
 if row.get('alpha',1)<1:
  p.inputs['Alpha'].default_value=row['alpha'];m.surface_render_method='DITHERED'
 for role,path in row.get('textures',{}).items():
  t=m.node_tree.nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(R/path),check_existing=True);t.extension='REPEAT';t.image.colorspace_settings.name='sRGB' if role=='base_color' else 'Non-Color'
  if role=='normal':
   sep=m.node_tree.nodes.new('ShaderNodeSeparateXYZ');inv=m.node_tree.nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1;comb=m.node_tree.nodes.new('ShaderNodeCombineXYZ');normal=m.node_tree.nodes.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=row.get('normal_strength',.2)
   m.node_tree.links.new(t.outputs['Color'],sep.inputs[0]);m.node_tree.links.new(sep.outputs['X'],comb.inputs['X']);m.node_tree.links.new(sep.outputs['Y'],inv.inputs[1]);m.node_tree.links.new(inv.outputs[0],comb.inputs['Y']);m.node_tree.links.new(sep.outputs['Z'],comb.inputs['Z']);m.node_tree.links.new(comb.outputs[0],normal.inputs['Color']);m.node_tree.links.new(normal.outputs[0],p.inputs['Normal'])
  else:
   m.node_tree.links.new(t.outputs['Color'],p.inputs['Base Color' if role=='base_color' else 'Roughness'])
   if role=='base_color' and row.get('opacity_from_base_color'):
    m.surface_render_method='DITHERED'
    if row.get('alpha_mode') in ['masked','dithered_masked']:
     clip=m.node_tree.nodes.new('ShaderNodeMath');clip.operation='GREATER_THAN';clip.inputs[1].default_value=row.get('alpha_cutoff',.333)
     m.node_tree.links.new(t.outputs['Alpha'],clip.inputs[0]);m.node_tree.links.new(clip.outputs[0],p.inputs['Alpha'])
    else:m.node_tree.links.new(t.outputs['Alpha'],p.inputs['Alpha'])
 mats[row['name']]=m
source=bpy.data.collections.new('EDITABLE_ORIGINAL_DINER');scene.collection.children.link(source);source.hide_viewport=True;source.hide_render=True
parts={};lodstats={}
for lod in [0,1,2]:
 lodstats[str(lod)]={}
 for name,data in build(lod).items():
  # Canonical door is local to its hinge. Other modules carry root-site coordinates.
  offset=Vector(PARTS[name]['offset']);suffix='' if lod==0 else '_LOD'+str(lod);mesh=bpy.data.meshes.new(name+suffix);mesh.from_pydata(data.v,[],data.f);mesh.update()
  used=[r['name'] for r in rows if r['name'] in set(data.mat)]
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
  before=len(ob.data.vertices);bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-6);bm.to_mesh(ob.data);bm.free();ob.data.update()
  assert all(p.area>=5e-9 for p in ob.data.polygons),(name,lod,'UE unstable triangle')
  points=[tuple(v.co) for v in ob.data.vertices];bounds=[[f(p[k] for p in points) for k in range(3)] for f in [min,max]]
  row={'triangles':len(ob.data.polygons),'bounds_local_m':bounds,'materials':used,'used_materials':sorted(used),'site_local_offset_m':list(offset),'origin':[0,0,0],'vertices_before_weld':before,'vertices_after_weld':len(ob.data.vertices)};lodstats[str(lod)][name]=row
  if lod==0:
   parts[name]=row;ob['original_source']=REVISION;ob['site_local_offset_m']=list(offset);ob['role']=PARTS[name]['role'];ob['collision_authority']='Original diner walk zones and colliders retained; visual authored replacement';ob['component_inventory']=json.dumps(sorted(groups));copy=ob.copy();copy.data=ob.data.copy();copy.name='SRC_'+name;source.objects.link(copy);copy.location=offset
  bpy.ops.object.select_all(action='DESELECT');ob.select_set(True)
  bpy.ops.export_scene.fbx(filepath=str(OUT/(name+suffix+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
  if lod==0:
   bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True);ob.location=offset
  else:bpy.data.objects.remove(ob,do_unlink=True)
for name,row in parts.items():row['lod_triangles']=[lodstats[str(i)][name]['triangles'] for i in [0,1,2]]
total=sum(r['triangles'] for r in parts.values());assert total<=BUDGET_TRIANGLES,(total,BUDGET_TRIANGLES)
bpy.ops.object.select_all(action='DESELECT')
for ob in scene.objects:
 if ob.type=='MESH' and ob.name in PARTS:ob.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(R/'Mesa_Diner.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
bpy.ops.object.select_all(action='DESELECT');bpy.ops.mesh.primitive_cube_add(size=1,location=(2,4,6));probe=bpy.context.object;probe.name='axis_probe';probe.dimensions=(.2,.4,.6);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
bpy.ops.export_scene.fbx(filepath=str(OUT/'axis_probe.fbx'),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False);bpy.data.objects.remove(probe,do_unlink=True)
for im in bpy.data.images:
 if im.source=='FILE':im.pack();im.filepath=bpy.path.relpath(im.filepath,start=str(R))
contract={'status':'SOURCE_GENERATED_NATIVE_UNVERIFIED','source_revision':REVISION,'site_world_three_m':SITE_WORLD_THREE,'world_yaw_degrees':WORLD_YAW_DEGREES,'mapping':'worldThree=(7979.95+bx,4.309006389843889+bz,1999.95-by)','units':'metres','blender_up':'Z','front':'positiveX','parts':parts,'lods':lodstats,'total_LOD0_triangles':total,'collision':'Original diner zones/proxies preserved; authored visual meshes no collision except explicit opened door proxy','axis_probe_source_center_m':[2,4,6],'axis_probe_source_dimensions_m':[.2,.4,.6],'axis_probe_expected_UE_center_cm':[200,-400,600],'door':{'pivot_site_m':[1.9,-.58,0],'baked_open_angle_degrees':-105,'runtime_additional_yaw_degrees':0,'opening_y_m':[-.6,.6]},'protected_dynamic_objects':['coffeePot','cup','regularCup','clipping','payphoneHandset','waitress','driver'],'top_authorities_m':{'floor':0,'counter':1.0,'booth_table':.8025,'bench':.46},'originality':'All geometry/maps/signage original; shared original substation mesh recipe read-only.'}
(R/'model_contract.json').write_text(json.dumps(contract,indent=2)+'\n');bpy.ops.wm.save_as_mainfile(filepath=str(R/'Mesa_Diner.blend'),compress=True)
print('MESA_DINER_BUILD_PASS',json.dumps({'parts':len(parts),'triangles':total,'lod_totals':{k:sum(v['triangles'] for v in rows.values()) for k,rows in lodstats.items()}}))
