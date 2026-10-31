"""Sierra V2 native production export. Root owns the exclusive Blender/UE slot.

Run Blender --background --threads 4 --python-exit-code 1 --python export_sierra.py
Optional after --: --select SM_Sierra_Office,SM_Sierra_Sign --resolution 2048 --no-bake
--inventory writes selection/adaptation only, no bake/export. --no-bake is geometry
diagnostics only and cannot produce a ready material manifest.

The source .blend is read-only. Evaluated authoring geometry is copied into a
fresh production scene; world-coordinate procedural shaders are baked before
replacing them with portable PBR atlas materials. Original cars, KELVORA,
live doors/documents/phones, all presentation terrain and source lights stay out.
"""
import argparse, bpy, bmesh, hashlib, json, math, re, shutil, sys
from pathlib import Path
from mathutils import Matrix, Vector

R=Path(__file__).resolve().parent
DEFAULT_SOURCE=R.parent/'test-v2/Sierra_Motor_Court_Test_V2_Kelvora.blend'
ap=argparse.ArgumentParser()
ap.add_argument('--source',type=Path,default=DEFAULT_SOURCE)
ap.add_argument('--out',type=Path,default=R/'output')
ap.add_argument('--select',default='')
ap.add_argument('--resolution',type=int,default=2048)
ap.add_argument('--inventory',action='store_true')
ap.add_argument('--no-bake',action='store_true')
ap.add_argument('--samples',type=int,default=1)
args=ap.parse_args(sys.argv[sys.argv.index('--')+1:] if '--'in sys.argv else [])
OUT=args.out;OUT.mkdir(parents=True,exist_ok=True)
(OUT/'exports').mkdir(exist_ok=True);(OUT/'textures').mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(args.source))
source_scene=next(s for s in bpy.data.scenes if any(o.name=='Room 6 door leaf' for o in s.objects))
bpy.context.window.scene=source_scene;bpy.context.view_layer.update()
dg=bpy.context.evaluated_depsgraph_get()

def bbox(ob):
 vs=[ob.matrix_world@Vector(v)for v in ob.bound_box]
 return [Vector([fn(v[k]for v in vs)for k in range(3)])for fn in [min,max]]

def cleanbase(name):return re.sub(r'\.\d{3}$','',name)

def selection(ob):
 if ob.type not in {'MESH','CURVE','FONT','SURFACE'}:return None,'nongeometry'
 name=cleanbase(ob.name);colls=' '.join(c.name for c in ob.users_collection)
 if 'Native room6 lining only' in colls:return 'SM_Sierra_Room6Lining','original study walnut lining and thin carpet only'
 if 'Original parked cars' in colls or '07 Detailed' in colls:return None,'existing cars retained in runtime'
 if ob.name.startswith('Retained original') or any(p.name.startswith('Retained original')for p in ancestors(ob)):return None,'existing car hierarchy'
 if 'Existing KELVORA' in colls and not name.startswith(('Existing TV bookcase','Office book spine')):return None,'existing KELVORA retained in runtime'
 if name.startswith(('Desert ground','Ground around','Distant low','Eroded distant','North vehicle arrival apron','Roadside two lane road','Road double yellow','Road shoulder white')):return None,'presentation world/road'
 if name.startswith(('Room 6 door leaf','Room 6 floor')):return None,'live room6 door/unused guest floor'
 if name.startswith('Room ') and name.endswith(' floor'):return None,'unseen guest room floor'
 if name.startswith(('Public telephone','Telephone black handset','Phone cord turn','Office hours on glass')):return None,'fake exterior telephone or moved glazing label'
 if name.startswith(('V2 Reception register','V2 Reception ledger','V2 Reception ballpoint','V2 Reception telephone','V2 Reception coiled telephone','V2 Reception key','V2 Reception motel key','V2 Reception brass key','V2 Reception tag number','V2 Reception bell','V2 Reception brass service bell','V2 Reception mail rack','V2 Reception mail shelf','V2 Reception waiting envelope')):return None,'original live office props/anchors retained'
 if name.startswith(('Registration papers','Room key board','Key label','Key hook','Room key tag')):return None,'old authoring fake office props'
 lo,hi=bbox(ob);center=(lo+hi)*.5
 if abs(center.y+32.13)<.8 and name.startswith(('Window glass','Window dark recessed return','Gathered physical curtain')):return None,'existing live room6 window and interior view retained'
 if abs(center.y+30.1)<.56 and 2.29<center.x<2.42 and name.startswith(('Recessed painted door panel','Brass lock','Round motel knob','Door peephole','Door hinge')):return None,'room6 live leaf hardware'
 if 'V2 Desert ground and garden' in colls:
  # Keep only near-court authored pots/plants. Never export the 3.6km plane.
  if lo.x < -5 or hi.x>15.7 or lo.y < -45 or hi.y>8:return None,'distant presentation vegetation'
  if lo.x<5 and hi.x>-8 and lo.y<5 and hi.y>3.4:return None,'plant intersects corrected north pool walk'
  return 'SM_Sierra_Planting','local court planting'
 if '06 One Sierra' in colls:return ('SM_Sierra_GlowSign' if any(is_glow(m)for m in ob.data.materials if m) else 'SM_Sierra_Sign'),'single roadside sign'
 if '09 Revised pool' in colls:
  if name.startswith('Pool water surface'):return 'SM_Sierra_PoolWater','translucent water'
  if name.startswith(('Pool fence','Pool slender','Pool gate')):return 'SM_Sierra_PoolFence','pool enclosure'
  if name.startswith(('Lounger','Pool side table','Pool patio','Patio','Pool orange','Rescue','Pool hooked','Pool rules','Pool pump')):return 'SM_Sierra_PoolFurniture','pool furniture and safety props'
  return 'SM_Sierra_PoolDeck','pool bowl deck and fixtures'
 if name.startswith(('Faded pedestrian crossing','Pool entrance footpath','Drive garden kerb','Poolside gravel bed')):return None,'retired east pool route'
 if name.startswith('Flush asphalt seam') and hi.x>15.65:return None,'seam beyond corrected court'
 if 'Native restore office' in colls:return 'SM_Sierra_Office','exact original live prop support'
 if 'Native restored pool walk' in colls:return 'SM_Sierra_CourtDetail','continuous north and west pool approach'
 if 'V2 Court' in colls:
  if name=='Continuous parking and drive':return 'SM_Sierra_CourtSurface','tiled asphalt surface'
  return 'SM_Sierra_CourtDetail','parking paint wheelstops and paths'
 if any(is_glass(m)for m in ob.data.materials if m):return 'SM_Sierra_Glass','separate glazing'
 if name.startswith('Lantern opal glass') and abs(center.y+30.1)<.2:return 'SM_Sierra_GlowRoom06','independent runtime room6 porch lamp'
 if any(is_glow(m)for m in ob.data.materials if m):return ('SM_Sierra_GlowOffice' if ('Reception' in colls or center.y>-3.6) else 'SM_Sierra_GlowExterior'),'separate practical diffusers'
 if 'V2 Reception interior' in colls or name.startswith(('Existing TV bookcase','Office book spine')):return 'SM_Sierra_Office','reception originals'
 if '02 Reception' in colls:return 'SM_Sierra_Office','office shell and service detail'
 if hi.y-lo.y>8:return 'SM_Sierra_RoofShell','continuous roof/rear structure'
 if center.y>-3.6:return 'SM_Sierra_Office','office front architecture'
 room=max(1,min(8,int(round((-center.y-.1)/5))))
 return 'SM_Sierra_Room%02d'%room,'guest facade module'

def ancestors(o):
 while o.parent:o=o.parent;yield o

def principled(m):return next((n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None)if m and m.use_nodes else None
def is_glass(m):
 p=principled(m)
 return bool(p and p.inputs['Transmission Weight'].default_value>.2)
def is_glow(m):
 p=principled(m)
 return bool(p and p.inputs['Emission Strength'].default_value>.1)

def adaptation(ob):
 name=cleanbase(ob.name)
 # Rear of the middle room row, 25m south of the former lobby-side pool.
 if any('09 Revised pool' in c.name for c in ob.users_collection):return Matrix.Translation(Vector((11,-54,0)))@Matrix.Rotation(math.pi,4,'Z')
 if name=='Continuous parking and drive':return Matrix.Translation(Vector((5.1,0,0)))@Matrix.Diagonal(Vector((10.6/12.9,1,1,1)))@Matrix.Translation(Vector((-5.1,0,0)))
 if name=='V2 Reception counter laminate top':
  return Matrix.Translation(Vector((-.8,-.35,.985)))@Matrix.Diagonal(Vector((1.16/.98,3.34/3.08,.07/.055,1)))@Matrix.Translation(Vector((.2,.7,-.99)))
 if name.startswith(('V2 Reception counter','V2 Reception lamp','V2 Reception linen shade','V2 Reception staff drawer','V2 Reception drawer pull')):
  # Old source counter top is1.020m, original documents are1.027m.
  return Matrix.Translation(Vector((-.8,-.35,.0025)))@Matrix.Diagonal(Vector((1.16/.98,3.34/3.08,1,1)))@Matrix.Translation(Vector((.2,.7,0)))
 if name.startswith(('Existing TV bookcase','Office book spine')):return Matrix.Translation(Vector((0,0,.053)))
 return Matrix.Identity(4)

def add_runtime_supports():
 # Author only the static supporting geometry at existing gameplay anchors.
 # This changes the in-memory source copy, never the input .blend on disk.
 coll=bpy.data.collections.new('Native restore office');source_scene.collection.children.link(coll)
 def finish(ob,name,mat):
  ob.name=name
  for c in list(ob.users_collection):c.objects.unlink(ob)
  coll.objects.link(ob);ob.data.materials.append(bpy.data.materials[mat]);return ob
 wood='Reception dark walnut vertical grain';trim='Reception oiled walnut horizontal grain';metal='Reception aged brushed brass';dark='Rubber and shadow gaps';paper='Reception cream paper'
 def box(name,center,size,mat):
  bpy.ops.mesh.primitive_cube_add(size=1,location=center);o=bpy.context.object;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
  return finish(o,name,mat)
 def tube(name,a,b,r,mat,segments=12):
  av,bv=Vector(a),Vector(b);delta=bv-av
  bpy.ops.mesh.primitive_cylinder_add(vertices=segments,radius=r,depth=delta.length,location=(av+bv)*.5)
  o=bpy.context.object;o.rotation_euler=delta.to_track_quat('Z','Y').to_euler();return finish(o,name,mat)
 box('Native Keyboard oak back',(-2.1,1.67,1.67),(.075,1.55,.63),wood)
 for room in range(1,9):
  row=0 if room<=4 else 1;col=(room-1)%4;yy=2.22-col*.36;z=1.83-row*.3
  bpy.ops.object.text_add(location=(-2.055,yy,z+.075));o=bpy.context.object;o.data.body=str(room);o.data.size=.07;o.data.extrude=.0004
  o.rotation_euler=Matrix(((0,0,1),(1,0,0),(0,1,0))).to_euler();finish(o,'Native key number '+str(room),paper)
  if room!=6:
   tube('Native key hook '+str(room),(-2.055,yy,z-.04),(-2.005,yy,z-.065),.008,metal,5)
   box('Native static brass key '+str(room),(-1.99,yy,z-.12),(.012,.055,.12),metal)
 box('Native wall telephone base',(.23,2.61,1.46),(.32,.16,.52),trim)
 box('Native telephone shelf',(.23,2.56,1.17),(.37,.25,.04),wood)
 bpy.ops.mesh.primitive_torus_add(major_segments=24,minor_segments=6,location=(.23,2.514,1.48),major_radius=.065,minor_radius=.013,rotation=(math.pi/2,0,0));finish(bpy.context.object,'Native rotary dial',metal)
 tube('Native telephone mouthpiece stem',(.23,2.51,1.73),(.23,2.42,1.69),.018,dark,6)
 coll=bpy.data.collections.new('Native restored pool walk');source_scene.collection.children.link(coll)
 concrete='Warm poured concrete'
 box('Native west pool approach',(-7.3,-25.75,-.06),(1.2,11.5,.12),concrete)
 # Keep the lobby's north path. Route around the rear wall, not through guest rooms.
 box('Native north pool approach',(-1.5,4.2,-.06),(13,1.6,.12),concrete)
 box('Native rear pool approach',(-4.6,-8.3,-.06),(1.2,23.4,.12),concrete)
 box('Native rear pool link',(-6,-20.8,-.06),(4,1.6,.12),concrete)
 # Join east gate at x-8 with no thin sand gap to the west walking strip.
 box('Native pool gate threshold',(-7.9,-30.7,-.06),(.2,1.6,.12),concrete)
 bpy.context.view_layer.update()

add_runtime_supports()
room_study=bpy.data.scenes.get('V2 Room interior study')
if room_study:
 lining_collection=bpy.data.collections.new('Native room6 lining only');source_scene.collection.children.link(lining_collection)
 for ob in list(room_study.objects):
  if ob.name.startswith(('R6 Vertical walnut veneer panel','R6 Walnut ceiling moulding','R6 Walnut ceiling end moulding','R6 Carpet surface')):
   lining_collection.objects.link(ob)
bpy.context.view_layer.update()
dg=bpy.context.evaluated_depsgraph_get()

selected={};excluded=[]
for ob in source_scene.objects:
 key,reason=selection(ob)
 row={'name':ob.name,'reason':reason}
 if key:selected.setdefault(key,[]).append(ob)
 else:excluded.append(row)
inventory={'source':str(args.source),'source_sha256':hashlib.sha256(args.source.read_bytes()).hexdigest(),'chunks':{k:[o.name for o in v]for k,v in sorted(selected.items())},'excluded':excluded,'adaptations':['Counter/slats/desk lamp moved/scaled to original counter and1.020m top','Bookcase raised .053m to support existing KELVORA at original shelf','Original runtime papers, key6, phone, TV and Room6 leaf remain external'],'runtime_modified':False}
(OUT/'selection.json').write_text(json.dumps(inventory,indent=2)+'\n')
if args.inventory:print('SIERRA_INVENTORY',json.dumps({k:len(v)for k,v in selected.items()}),flush=True);raise SystemExit(0)
chosen=set(args.select.split(','))if args.select else set(selected)
assert chosen<=set(selected),(chosen-set(selected))
scene=bpy.data.scenes.new('Sierra production exports');scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=args.samples;scene.cycles.use_denoising=False
scene.render.threads_mode='FIXED';scene.render.threads=4;scene.render.bake.use_clear=True;scene.render.bake.margin=8
scene.render.bake.normal_space='TANGENT';scene.render.bake.normal_r='POS_X';scene.render.bake.normal_g='NEG_Y';scene.render.bake.normal_b='POS_Z'
bpy.context.window.scene=scene

def eval_copy(ob):
 # The depsgraph belongs to the untouched source scene.
 ev=ob.evaluated_get(dg);data=bpy.data.meshes.new_from_object(ev,preserve_all_data_layers=True,depsgraph=dg)
 data.transform(adaptation(ob)@ob.matrix_world);data.update()
 copy=bpy.data.objects.new('Production_'+ob.name,data);scene.collection.objects.link(copy)
 return copy

def activate(ob):
 bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob

def combine(name,obs):
 bpy.ops.object.select_all(action='DESELECT')
 for ob in obs:ob.select_set(True)
 bpy.context.view_layer.objects.active=obs[0];bpy.ops.object.join();ob=bpy.context.object;ob.name=name;ob.data.name=name
 # Explicit triangulation and microscopic degenerate cleanup for reliable FBX.
 bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7)
 bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=1e-7);bmesh.ops.triangulate(bm,faces=list(bm.faces));bm.to_mesh(ob.data);bm.free();ob.data.update()
 assert len(ob.data.polygons)>0
 return ob

def atlas_uv(ob):
 activate(ob)
 for layer in list(ob.data.uv_layers):ob.data.uv_layers.remove(layer)
 ob.data.uv_layers.new(name='UVMap')
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
 bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.004,area_weight=.1,scale_to_bounds=True)
 bpy.ops.object.mode_set(mode='OBJECT')

def image(name,res,role):
 im=bpy.data.images.new(name,width=res,height=res,alpha=False,float_buffer=False)
 im.colorspace_settings.name='sRGB'if role=='base_color'else'Non-Color';im.file_format='PNG';im.filepath_raw=str(OUT/'textures'/(name+'.png'))
 return im

def connection(tree,input_socket,node_output):
 if input_socket.is_linked:tree.links.new(input_socket.links[0].from_socket,node_output)
 else:node_output.default_value=input_socket.default_value

def bake_maps(ob,res):
 originals=list(ob.data.materials);copies=[];material_indices=[p.material_index for p in ob.data.polygons]
 for m in originals:
  assert m and principled(m),(ob.name,'missing Principled',m.name if m else None)
  copies.append(m.copy())
 ob.data.materials.clear()
 for m in copies:ob.data.materials.append(m)
 for p,index in zip(ob.data.polygons,material_indices):p.material_index=index
 assert [p.material_index for p in ob.data.polygons]==material_indices, 'Bake material assignment changed'
 result={}
 for role in ['base_color','orm','normal']:
  im=image('T_'+ob.name+'_'+role,res,role);saved=[]
  for m in copies:
   nt=m.node_tree;n=nt.nodes;p=principled(m);out=next(a for a in n if a.type=='OUTPUT_MATERIAL' and a.is_active_output)
   target=n.new('ShaderNodeTexImage');target.image=im;n.active=target
   original_link=out.inputs['Surface'].links[0].from_socket if out.inputs['Surface'].is_linked else None
   temporary=[]
   if role!='normal':
    emit=n.new('ShaderNodeEmission');temporary.append(emit)
    if role=='base_color':connection(nt,p.inputs['Base Color'],emit.inputs['Color'])
    else:
     combine_node=n.new('ShaderNodeCombineXYZ');temporary.append(combine_node);combine_node.inputs['X'].default_value=1
     connection(nt,p.inputs['Roughness'],combine_node.inputs['Y']);connection(nt,p.inputs['Metallic'],combine_node.inputs['Z']);nt.links.new(combine_node.outputs[0],emit.inputs['Color'])
    emit.inputs['Strength'].default_value=1;nt.links.new(emit.outputs[0],out.inputs['Surface'])
   saved.append((nt,out,original_link,temporary,target))
  activate(ob);print('BAKE_START',ob.name,role,res,flush=True)
  bpy.ops.object.bake(type='NORMAL'if role=='normal'else'EMIT',margin=8,use_clear=True)
  im.save();result[role]='textures/'+Path(im.filepath_raw).name
  for nt,out,old,temp,target in saved:
   if old:nt.links.new(old,out.inputs['Surface'])
   for node in temp:nt.nodes.remove(node)
   nt.nodes.remove(target)
  print('BAKE_DONE',ob.name,role,flush=True)
 return result

def portable_material(name,textures=None,**values):
 m=bpy.data.materials.new(name);m.use_nodes=True;p=principled(m)
 color=values.get('base_color_linear',[.5,.5,.5,1]);p.inputs['Base Color'].default_value=color
 p.inputs['Metallic'].default_value=values.get('metallic',0);p.inputs['Roughness'].default_value=values.get('roughness',.6)
 row={'name':name,'base_color_linear':color,'metallic':values.get('metallic',0),'roughness':values.get('roughness',.6),'specular':.35,'textures':textures or {},'two_sided':values.get('two_sided',False),'normal_convention':'DIRECTX',**values}
 if textures:
  nt=m.node_tree
  for role,path in textures.items():
   t=nt.nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(OUT/path),check_existing=True);t.image.colorspace_settings.name='sRGB'if role=='base_color'else'Non-Color'
   if role=='base_color':
    if 'texture_tint_linear' in values:
     mix=nt.nodes.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY';mix.inputs[0].default_value=1;mix.inputs[2].default_value=values['texture_tint_linear'];nt.links.new(t.outputs['Color'],mix.inputs[1]);nt.links.new(mix.outputs[0],p.inputs['Base Color'])
    else:nt.links.new(t.outputs['Color'],p.inputs['Base Color'])
   if role=='orm':
    sep=nt.nodes.new('ShaderNodeSeparateXYZ');nt.links.new(t.outputs['Color'],sep.inputs[0]);nt.links.new(sep.outputs['Y'],p.inputs['Roughness']);nt.links.new(sep.outputs['Z'],p.inputs['Metallic'])
   if role=='normal':
    # PNG is DirectX for native; invert green in the saved Blender inspection scene.
    sep=nt.nodes.new('ShaderNodeSeparateXYZ');nt.links.new(t.outputs['Color'],sep.inputs[0]);inv=nt.nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1;nt.links.new(sep.outputs['Y'],inv.inputs[1]);co=nt.nodes.new('ShaderNodeCombineXYZ');nt.links.new(sep.outputs['X'],co.inputs['X']);nt.links.new(inv.outputs[0],co.inputs['Y']);nt.links.new(sep.outputs['Z'],co.inputs['Z']);normal=nt.nodes.new('ShaderNodeNormalMap');nt.links.new(co.outputs[0],normal.inputs['Color']);nt.links.new(normal.outputs[0],p.inputs['Normal'])
 if values.get('emission_strength'):
  p.inputs['Emission Strength'].default_value=values['emission_strength'];p.inputs['Emission Color'].default_value=values.get('emission',[1,1,1,1])
  if values.get('emission_from_base_color') and p.inputs['Base Color'].is_linked:m.node_tree.links.new(p.inputs['Base Color'].links[0].from_socket,p.inputs['Emission Color'])
 if values.get('alpha',1)<1:p.inputs['Alpha'].default_value=values['alpha'];m.surface_render_method='DITHERED'
 return m,row

def mesh_bounds(ob):return [[fn(v.co[k]for v in ob.data.vertices)for k in range(3)]for fn in [min,max]]

def export_fbx(ob,path):
 activate(ob);bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')

def asphalt_material(ob):
 # Tiled true source map, metres/2.38095. The large ground is never stretched to an atlas.
 src=Path('/home/tom/SIGNAL-47/web/src/assets/art/road/tex_oldroad_asphalt.jpg');dst=OUT/'textures/T_Sierra_Asphalt.jpg';shutil.copy2(src,dst)
 uv=ob.data.uv_layers.active or ob.data.uv_layers.new(name='UVMap')
 for poly in ob.data.polygons:
  for li in poly.loop_indices:
   v=ob.data.vertices[ob.data.loops[li].vertex_index].co;uv.data[li].uv=(v.x*.42,v.y*.42)
 return portable_material('M_MotelV2_CourtSurface',{'base_color':'textures/'+dst.name},roughness=.94,metallic=0,texture_tint_linear=[.24,.25,.25,1])

manifest_path=OUT/'manifest.json';materials_path=OUT/'materials.json'
manifest=json.loads(manifest_path.read_text())if args.select and manifest_path.exists()else {'parts':{}}
mats={r['name']:r for r in json.loads(materials_path.read_text())}if args.select and materials_path.exists()else {}
done=[]
for name in sorted(chosen):
 print('CHUNK_START',name,len(selected[name]),flush=True)
 copies=[eval_copy(o)for o in selected[name]];ob=combine(name,copies)
 if name=='SM_Sierra_CourtSurface':mat,row=asphalt_material(ob)
 elif name=='SM_Sierra_Glass':
  atlas_uv(ob);mat,row=portable_material('M_MotelV2_Glass',base_color_linear=[.16,.23,.21,1],roughness=.08,metallic=0,alpha=.18,two_sided=True)
 elif name=='SM_Sierra_PoolWater':
  atlas_uv(ob);maps={}if args.no_bake else bake_maps(ob,1024)
  mat,row=portable_material('M_MotelV2_PoolWater',maps,base_color_linear=[.08,.26,.24,1],roughness=.07,metallic=0,alpha=.55,two_sided=True,water_surface=True)
 else:
  atlas_uv(ob);res=4096 if name=='SM_Sierra_RoofShell' else args.resolution
  maps={}if args.no_bake else bake_maps(ob,res)
  values={'two_sided':name=='SM_Sierra_Planting'}
  if name.startswith('SM_Sierra_Glow'):values.update(emission_strength=2.5,emission=[1,.65,.35,1],emission_from_base_color=True)
  mat,row=portable_material('M_MotelV2_'+name[len('SM_Sierra_'):],maps,**values)
 ob.data.materials.clear();ob.data.materials.append(mat)
 for p in ob.data.polygons:p.material_index=0
 mats[row['name']]=row
 # All LOD0 meshes are explicitly triangulated. The same FBX orientation applies to every LOD.
 lodcounts=[len(ob.data.polygons)];lodrows=[{'triangles':len(ob.data.polygons),'bounds_local_m':mesh_bounds(ob),'materials':[mat.name]}]
 export_fbx(ob,OUT/'exports'/(name+'.fbx'))
 for lod,ratio in [(1,.50),(2,.22)]:
  lower=ob.copy();lower.data=ob.data.copy();lower.name=name+'_LOD'+str(lod);scene.collection.objects.link(lower);activate(lower)
  mod=lower.modifiers.new('Authored distance simplification','DECIMATE');mod.ratio=ratio;mod.use_collapse_triangulate=True
  bpy.ops.object.modifier_apply(modifier=mod.name);lodcounts.append(len(lower.data.polygons));lodrows.append({'triangles':len(lower.data.polygons),'bounds_local_m':mesh_bounds(lower),'materials':[mat.name]});export_fbx(lower,OUT/'exports'/(lower.name+'.fbx'));bpy.data.objects.remove(lower,do_unlink=True)
 rowpart={'name':name,'materials':[mat.name],'triangles':len(ob.data.polygons),'vertices':len(ob.data.vertices),'lod_triangles':lodcounts,'lods':lodrows,'bounds_local_m':mesh_bounds(ob),'source_objects':[o.name for o in selected[name]],'site_world_three_m':([0,0,-8000] if name=='SM_Sierra_Room6Lining' else [-45.9,-2.0985683853163195,674.9]),'parent_handle':('world.room6.group' if name=='SM_Sierra_Room6Lining' else 'world.motel.group'),'collision_role':'visual_only'if name in {'SM_Sierra_Glass','SM_Sierra_GlowOffice','SM_Sierra_GlowExterior','SM_Sierra_GlowSign','SM_Sierra_GlowRoom06','SM_Sierra_PoolWater','SM_Sierra_Planting'}else'complex_visibility_and_explicit_gameplay_colliders','source_geometry_sha256':hashlib.sha256((OUT/'exports'/(name+'.fbx')).read_bytes()).hexdigest()}
 manifest['parts'][name]=rowpart;done.append(ob)
 manifest.update(status='GEOMETRY_ONLY_UNVERIFIED'if args.no_bake else'SOURCE_BAKED_NATIVE_UNVERIFIED',source=str(args.source),source_sha256=inventory['source_sha256'],units='metres',mapping='worldThree=(-45.9+Bx,H+Bz,674.9-By)',native_import_policy={'force_front_x_axis':True,'coordinate_system_policy':'MATCH_UP_FORWARD_AXES','import_rotation_yaw':90},normal_convention='DIRECTX',runtime_modified=False)
 manifest_path.write_text(json.dumps(manifest,indent=2)+'\n');materials_path.write_text(json.dumps(list(mats.values()),indent=2)+'\n')
 # Prior chunks are excluded from subsequent bake visibility and selection.
 ob.hide_render=True
 print('CHUNK_DONE',name,lodcounts,flush=True)

# Incremental updates must keep the complete editable production model. Append
# unchanged LOD0 objects from the last completed package rather than saving a
# misleading .blend that contains only the selected updates.
if args.select:
 unchanged=set(manifest['parts'])-set(chosen)
 previous_blend=OUT/'Sierra_Production.blend'
 if unchanged:
  assert previous_blend.exists(), 'Incremental manifest has unchanged chunks but no completed production blend; run a full export first'
  with bpy.data.libraries.load(str(previous_blend),link=False)as(data_from,data_to):
   available=set(data_from.objects)
   assert unchanged<=available, ('Prior production blend missing chunks',sorted(unchanged-available))
   data_to.objects=sorted(unchanged)
  for retained in data_to.objects:
   assert retained is not None
   scene.collection.objects.link(retained);retained.hide_render=False;done.append(retained)
  assert {o.name for o in done}==set(manifest['parts']), 'Incomplete incremental production model'
  print('INCREMENTAL_RETAINED',len(unchanged),flush=True)

# Asymmetric axis probe identical to the project's proven native-import contract.
bpy.ops.object.select_all(action='DESELECT');bpy.ops.mesh.primitive_cube_add(size=1,location=(2,4,6));probe=bpy.context.object;probe.dimensions=(.2,.4,.6);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);export_fbx(probe,OUT/'exports/axis_probe.fbx');bpy.data.objects.remove(probe,do_unlink=True)
for ob in done:ob.hide_render=False
for im in bpy.data.images:
 if im.source=='FILE' and im.filepath.startswith(str(OUT)):im.pack()
# Keep only the production scenes and their dependencies. Blender5.2's scene
# libraries.write path segfaulted during finalization; use normal mainfile save
# after explicit provenance cleanup instead.
save_scenes={scene}
room_lining=next((o for o in done if o.name=='SM_Sierra_Room6Lining'),None)
if room_lining:
 room_preview=bpy.data.scenes.new('Room6 production lining');room_preview.unit_settings.system='METRIC';room_preview.unit_settings.scale_length=1
 scene.collection.objects.unlink(room_lining);room_preview.collection.objects.link(room_lining);save_scenes.add(room_preview)
print('FINALIZE_CLEANUP_START',flush=True)
dg=None
bpy.context.window.scene=scene
for old_scene in list(bpy.data.scenes):
 if old_scene not in save_scenes:bpy.data.scenes.remove(old_scene)
keep_objects=set(done)
for old_object in list(bpy.data.objects):
 if old_object not in keep_objects:bpy.data.objects.remove(old_object,do_unlink=True)
keep_materials={m for o in done for m in o.data.materials if m}
for old_material in list(bpy.data.materials):
 if old_material not in keep_materials:bpy.data.materials.remove(old_material,do_unlink=True)
keep_images={n.image for m in keep_materials for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image}
for old_image in list(bpy.data.images):
 if old_image not in keep_images:bpy.data.images.remove(old_image,do_unlink=True)
keep_meshes={o.data for o in done}
for old_mesh in list(bpy.data.meshes):
 if old_mesh not in keep_meshes:bpy.data.meshes.remove(old_mesh,do_unlink=True)
for collection in list(bpy.data.collections):bpy.data.collections.remove(collection)
for group in [bpy.data.curves,bpy.data.lights,bpy.data.cameras,bpy.data.texts,bpy.data.worlds]:
 for block in list(group):group.remove(block,do_unlink=True)
assert set(bpy.data.objects)==keep_objects
assert set(bpy.data.materials)==keep_materials
for im in keep_images:
 if not im.packed_file:im.pack()
 if im.filepath:im.filepath=bpy.path.relpath(im.filepath,start=str(OUT))
bpy.context.view_layer.update()
print('FINALIZE_SAVE_START',len(done),len(keep_images),flush=True)
production_tmp=OUT/'Sierra_Production.building.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(production_tmp),compress=True,relative_remap=False)
production_tmp.replace(OUT/'Sierra_Production.blend')
print('FINALIZE_SAVE_DONE',flush=True)
print('SIERRA_EXPORT_COMPLETE',json.dumps({'chunks':len(done),'materials':len(mats),'out':str(OUT),'status':manifest['status']}),flush=True)
