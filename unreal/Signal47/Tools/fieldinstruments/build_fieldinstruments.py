"""Build portable original Blender source and all FBX/GLB LODs; no render or UE.
Root coordinator runs this in the machine's single heavy-job slot.
Example: blender -b -t 2 --python build_fieldinstruments.py -- --manifest /path/manifest.json
"""
import argparse,hashlib,json,math,sys
from pathlib import Path
import bpy,bmesh
from mathutils import Matrix,Vector
R=Path(__file__).resolve().parent;sys.path.insert(0,str(R))
from design import PARTS,REVISION,SITE_WORLD_THREE,BUDGET_TRIANGLES
from geometry import build

def args():
 p=argparse.ArgumentParser();p.add_argument('--manifest',type=Path,default=Path('/home/tom/SIGNAL-47/unreal/Signal47/Content/WebData/manifest.json'))
 return p.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])

def manifest_poses(path):
 if not path.exists():raise RuntimeError('Manifest required to preserve exact animated pivots: '+str(path))
 data=json.loads(path.read_text());nodes={n['i']:n for n in data['nodes']};cache={}
 def world(i):
  if i in cache:return cache[i]
  n=nodes[i];values=n['m'];local=Matrix([values[j:j+4] for j in range(0,16,4)]).transposed()
  cache[i]=(world(n['p'])@local) if n['p']>=0 else local
  return cache[i]
 c=Matrix(((1,0,0,0),(0,0,-1,0),(0,1,0,0),(0,0,0,1)))
 out={}
 for part,spec in PARTS.items():
  h=spec.get('parent')
  if not h:continue
  mat=world(data['handles'][h]['node']);mat.translation-=Vector(SITE_WORLD_THREE)
  out[part]=c@mat@c.inverted()
 return out

def main():
 a=args();poses=manifest_poses(a.manifest);dest=R/'exports';dest.mkdir(exist_ok=True)
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 for mat in list(bpy.data.materials):bpy.data.materials.remove(mat)
 scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
 rows=json.loads((R/'materials.json').read_text());mats={}
 for row in rows:
  m=bpy.data.materials.new(row['name']);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF')
  for key,val in [('Base Color',row['base_color_linear']),('Metallic',row['metallic']),('Roughness',row['roughness']),('Specular IOR Level',row['specular']),('Emission Color',row['emission']),('Emission Strength',row['emission_strength'])]:p.inputs[key].default_value=val
  for role,path in row.get('textures',{}).items():
   tex=m.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(R/path),check_existing=True);tex.extension='REPEAT';tex.image.colorspace_settings.name='sRGB' if role=='base_color' else 'Non-Color'
   if role=='normal':
    normal=m.node_tree.nodes.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=row['normal_strength'];m.node_tree.links.new(tex.outputs['Color'],normal.inputs['Color']);m.node_tree.links.new(normal.outputs['Normal'],p.inputs['Normal'])
   else:m.node_tree.links.new(tex.outputs['Color'],p.inputs['Base Color' if role=='base_color' else 'Roughness'])
  mats[row['name']]=m
 source=bpy.data.collections.new('EDITABLE_ORIGINAL_COMPONENTS');scene.collection.children.link(source);source.hide_render=True;source.hide_viewport=True
 inventory={};stats={};parts={};assembled=[]
 for lod in range(3):
  stats[str(lod)]={};data=build(lod)
  for name,geo in data.items():
   mesh=bpy.data.meshes.new(name+('_LOD'+str(lod) if lod else ''));mesh.from_pydata(geo.v,[],geo.f);mesh.update()
   slots=[row['name'] for row in rows if row['name'] in set(geo.mat)]
   for slot in slots:mesh.materials.append(mats[slot])
   uv=mesh.uv_layers.new(name='UVMap');groups={}
   for poly,material,coords,smooth,tag in zip(mesh.polygons,geo.mat,geo.uv,geo.smooth,geo.tags):
    poly.material_index=slots.index(material);poly.use_smooth=smooth
    for li,xy in zip(poly.loop_indices,coords):uv.data[li].uv=xy
    groups.setdefault(tag,[]).extend(poly.vertices)
   ob=bpy.data.objects.new(name+('_LOD'+str(lod) if lod else ''),mesh);scene.collection.objects.link(ob)
   bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
   if lod==0:
    for group,ids in groups.items():ob.vertex_groups.new(name=group[:63]).add(list(set(ids)),1,'REPLACE')
   mod=ob.modifiers.new('Explicit_stable_triangles','TRIANGULATE');mod.quad_method='FIXED';bpy.ops.object.modifier_apply(modifier=mod.name)
   bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7);bm.to_mesh(mesh);bm.free();mesh.update()
   tiny=[p.index for p in mesh.polygons if p.area<5e-9]
   if tiny:raise AssertionError((name,lod,'triangles too small for native import',len(tiny)))
   positions=[v.co for v in mesh.vertices];bounds=[[fn(v[k] for v in positions) for k in range(3)] for fn in [min,max]]
   spec=PARTS[name];offset=Vector(spec['offset']);islocal='parent' in spec
   three_offset=[offset.x,offset.z,-offset.y]
   world=[three_offset[k]+SITE_WORLD_THREE[k] for k in range(3)]
   row={'triangles':len(mesh.polygons),'vertices':len(mesh.vertices),'bounds_local_m':bounds,'materials':slots,'used_materials':slots,'origin':[0,0,0],'site_local_offset_m':list(offset),'site_world_three_m':world,'local':islocal,'parent_handle':spec.get('parent','world.site.group'),'role':spec['role'],'proxy_handle':spec.get('proxy'), 'state_material_handle':spec.get('state_material'),'bounds_expected_ue_cm':[[100*bounds[0][0],-100*bounds[1][1],100*bounds[0][2]],[100*bounds[1][0],-100*bounds[0][1],100*bounds[1][2]]]}
   stats[str(lod)][name]=row
   if lod==0:
    parts[name]=row;inventory[name]={'components':sorted(groups),'count':len(groups)}
    ob['original_source']=REVISION;ob['role']=spec['role'];ob['native_parent_handle']=row['parent_handle'];ob['native_local']=islocal;ob['component_inventory']=json.dumps(sorted(groups))
   suffix='' if lod==0 else '_LOD'+str(lod)
   # FBX units/axes match the established town/substation importer, including its yaw90 correction.
   bpy.ops.export_scene.fbx(filepath=str(dest/(name+suffix+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
   bpy.ops.export_scene.gltf(filepath=str(dest/(name+suffix+'.glb')),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
   if lod==0:
    if islocal:ob.matrix_world=poses[name]
    else:ob.location=offset
    editable=ob.copy();editable.data=ob.data.copy();editable.name='SRC_'+name;source.objects.link(editable);assembled.append(ob)
   else:bpy.data.objects.remove(ob,do_unlink=True)
 assert sum(r['triangles'] for r in parts.values())<=BUDGET_TRIANGLES
 for name,row in parts.items():row['lod_triangles']=[stats[str(i)][name]['triangles'] for i in range(3)]
 bpy.ops.object.select_all(action='DESELECT')
 for ob in assembled:ob.select_set(True)
 bpy.ops.export_scene.gltf(filepath=str(R/'STATION01_Field_Instruments.glb'),export_format='GLB',use_selection=True,export_materials='EXPORT',export_extras=True)
 for im in bpy.data.images:
  if im.source=='FILE':im.pack();im.filepath=bpy.path.relpath(im.filepath,start=str(R))
 # Preview camera/lights live in the source but are excluded from every game export.
 preview=bpy.data.collections.new('PREVIEW_ONLY');scene.collection.children.link(preview)
 camdata=bpy.data.cameras.new('Instrument_overview');cam=bpy.data.objects.new('Instrument_overview',camdata);preview.objects.link(cam);cam.location=(10,-13,10);target=Vector((-5,0,.8));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();camdata.lens=44;scene.camera=cam
 for name,loc,energy,size in [('Softbox',(0,-5,11),1400,9),('Bench_fill',(-11,-6,5),700,5)]:
  light=bpy.data.lights.new(name,'AREA');light.energy=energy;light.shape='DISK';light.size=size;ob=bpy.data.objects.new(name,light);preview.objects.link(ob);ob.location=loc;ob.rotation_euler=(Vector((-6,0,0))-ob.location).to_track_quat('-Z','Y').to_euler()
 scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
 contract={'status':'SOURCE_GENERATED_NATIVE_UNVERIFIED','source_revision':REVISION,'units':'metres','blender_up':'Z','front':'negativeY','mapping':'siteThree=(bx,bz,-by); Unreal=(100bx,-100by,100bz)','site_world_three_m':SITE_WORLD_THREE,'parts':parts,'lods':stats,'inventory':inventory,'total_LOD0_triangles':sum(r['triangles'] for r in parts.values()),'collision':'Native floor, evidence occluders and story proxies retained; no model collision generated','states':{'lamp_hood':{'parent':'world.site.objs.lampHood','covered_Three_rotX':0,'uncovered_Three_rotX':2.1,'translation_baked':False},'lamp_bulb':{'part':'SM_FieldKit_LampBulb','material_handle':'world.site.m.fieldBulb'},'receiver_dial':{'part':'SM_FieldKit_ReceiverDial','material_handle':'world.site.m.dial'},'handset':{'part':'SM_FieldKit_Handset','parent':'world.site.objs.fieldPhoneHandset'}},'preserved_handles':['world.site.proxies.'+s for s in ['transit','lamp','receiver','fieldPhone','timingLog','diagram','drawer','cable','footing','markerA','markerB','stakesC']]+['world.site.objs.drawer','world.site.objs.drawerInside'], 'drawer_clearance_three_m':{'x':[-10.69,-10.21],'y':[.63,.76],'z':[1.75,2.53]},'originality':'Original geometry and deterministic printed/PBR maps; no external raw assets. Shared helper shapes adapted from our own substation meshkit. DejaVu used only to rasterise authored labels.'}
 (R/'model_contract.json').write_text(json.dumps(contract,indent=2)+'\n')
 bpy.ops.wm.save_as_mainfile(filepath=str(R/'STATION01_Field_Instruments.blend'),compress=True)
 print('FIELD_INSTRUMENTS_BUILD_PASS',json.dumps({'parts':len(parts),'triangles':contract['total_LOD0_triangles'],'lod_totals':{k:sum(r['triangles'] for r in v.values()) for k,v in stats.items()}}))
if __name__=='__main__':main()
