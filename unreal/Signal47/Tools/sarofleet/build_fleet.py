"""Run ONLY in a granted two-thread Blender slot. Licensed derivatives go to ignored WORK.
Source geometry is the inspected local glTF, never a procedural replacement pickup.
"""
import bpy,bmesh,json,sys,math,hashlib,shutil,re
from pathlib import Path
from mathutils import Matrix,Vector
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE))
from design import *
OUT=WORK/'exports';TEX=WORK/'textures';OUT.mkdir(parents=True,exist_ok=True);TEX.mkdir(exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
rows={};parts={};wheel_centres=[];own_sources=[];cleanup_proof=[];validation_proof=[]

def material(name,color,rough=.65,metal=0):
 m=bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
 rows[name]={'name':name,'base_color_linear':[*color,1],'roughness':rough,'metallic':metal};return m
M={k:material('M_Fleet_'+k,c,r,mt)for k,c,r,mt in [('Paint',(.59,.61,.51),.76,.12),('Dash',(.045,.055,.048),.77,0),('Vinyl',(.10,.125,.11),.74,0),('Chrome',(.38,.42,.42),.27,.85),('Rubber',(.022,.027,.025),.88,0),('Logo',(.12,.23,.23),.78,0),('Head',(.32,.35,.30),.24,.05),('Tail',(.32,.015,.008),.32,0)]}
for k in ['Head','Tail']:
 rows[M[k].name]['emission']=[1,.83,.6,1]if k=='Head'else[1,.025,.01,1];rows[M[k].name]['emission_strength']=0

def ownbox(name,at,size,mat,bevel=.012):
 bpy.ops.mesh.primitive_cube_add(size=1,location=blender(at));o=bpy.context.object;o.name=name;o.dimensions=(size[0],size[2],size[1]);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mat)
 if bevel:
  mod=o.modifiers.new('Soft formed edges','BEVEL');mod.width=bevel;mod.segments=3;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
 return o

def rod(name,a,b,r,mat,verts=12):
 a=Vector(blender(a));b=Vector(blender(b));bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=(b-a).length,location=(a+b)/2);o=bpy.context.object;o.name=name;o.rotation_mode='QUATERNION';o.rotation_quaternion=(b-a).to_track_quat('Z','Y');o.data.materials.append(mat);return o

def text(name,words,at,size,rotation,mat):
 bpy.ops.object.text_add(location=blender(at));o=bpy.context.object;o.name=name;o.data.body=words;o.data.size=size;o.data.extrude=.0003;o.data.align_x='CENTER';o.rotation_euler=rotation;o.data.materials.append(mat);bpy.ops.object.convert(target='MESH');return bpy.context.object

def join(name,objects):
 bpy.ops.object.select_all(action='DESELECT')
 objects=[o for o in objects if len(o.data.polygons)]
 assert objects,name
 if name in ['SM_Saro07_Cabin','SM_Saro07_Branding','SM_Saro07_HeadLens','SM_Saro07_TailLens']:
  source=bpy.data.collections.new('Editable_'+name);scene.collection.children.link(source);own_sources.append(source)
  for item in objects:
   editable=item.copy();editable.data=item.data.copy();source.objects.link(editable);editable.hide_render=True;editable.hide_viewport=True
 for o in objects:o.select_set(True)
 bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
 bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 return o

def split_faces(obj,name,predicate):
 # Triangle selection copies UV/material corner data via BMesh, no smart UV regeneration.
 o=obj.copy();o.data=obj.data.copy();scene.collection.objects.link(o);o.name=name
 bm=bmesh.new();bm.from_mesh(o.data);bm.faces.ensure_lookup_table();remove=[]
 for f in bm.faces:
  if not predicate(f):remove.append(f)
 bmesh.ops.delete(bm,geom=remove,context='FACES');bm.to_mesh(o.data);bm.free();return o

def imported(model):
 old=set(bpy.data.objects);bpy.ops.import_scene.gltf(filepath=str(FREE/'src'/model/('S47_'+model+'.gltf')))
 objs=[o for o in bpy.data.objects if o not in old and o.type=='MESH']
 for o in objs:
  world=o.matrix_world.copy();o.parent=None;o.matrix_world=Matrix.Identity(4);o.data.transform(world)
 return objs,[o for o in bpy.data.objects if o not in old and o.type!='MESH']

# C10 glTF imported Z-up coordinates (srcX,-srcZ,srcY) -> Blender(-srcZ,-srcX,srcY).
c10,empties=imported('pickup_c10_1966');body=[];shell=[];glass=[]
for o in c10:
 for v in o.data.vertices:
  q=v.co.copy();v.co=(q.y*C10_SCALE,-q.x*C10_SCALE-C10_OFFSET_Z,q.z*C10_SCALE)
 label=o.name.split('.')[0]
 if label in ['Object_14','Object_15']:
  for front in [True,False]:
   w=split_faces(o,'wheel',lambda f,front=front:(sum(v.co.y for v in f.verts)/len(f.verts)>-.1)==front)
   if not len(w.data.polygons):bpy.data.objects.remove(w,do_unlink=True);continue
   lo=Vector(tuple(min(v.co[i]for v in w.data.vertices)for i in range(3)));hi=Vector(tuple(max(v.co[i]for v in w.data.vertices)for i in range(3)));centre=(lo+hi)/2
   side='L'if centre.x<0 else'R';name='SM_Saro07_Wheel'+('F'if front else'R')+side
   # Wheels retain their source side and centre. Native animation rotates about this authored pivot.
   for v in w.data.vertices:v.co-=centre
   w.name=name;parts[name]=w;wheel_centres.append({'part':name,'three':[centre.x,float((hi.z-lo.z)/2),-centre.y],'radius_m':float((hi.z-lo.z)/2)})
 elif label=='Object_2':glass.append(o.copy());glass[-1].data=o.data.copy();scene.collection.objects.link(glass[-1])
 else:
  def interior(f):
   p=sum((v.co for v in f.verts),Vector())/len(f.verts)
   return abs(p.x)<.77 and -.64< -p.y <.49 and .43<p.z<1.59
  def upper(f):
   p=sum((v.co for v in f.verts),Vector())/len(f.verts)
   return p.z>1.13 and -.79< -p.y <.62
  def manufacturer_glyph(f):
   if label!='Object_5':return False
   p=sum((v.co for v in f.verts),Vector())/len(f.verts)
   # Measured disconnected CHEVROLET front-glyph components only; grille bars remain.
   return -.39<p.x<.41 and .764<p.z<.789 and -1.986< -p.y < -1.972
  body.append(split_faces(o,'body',lambda f:not interior(f)and not upper(f)and not manufacturer_glyph(f)))
  shell.append(split_faces(o,'shell',lambda f:not interior(f)and upper(f)and not manufacturer_glyph(f)))
for o in c10+empties:bpy.data.objects.remove(o,do_unlink=True)
parts['SM_Saro07_Body']=join('SM_Saro07_Body',body);parts['SM_Saro07_Shell']=join('SM_Saro07_Shell',shell);parts['SM_Saro07_Glass']=join('SM_Saro07_Glass',glass)
# Source body atlases contain brand marks (including a drinks logo). Replace ONLY imported paint
# material with our original fleet paint; real source body normals continue to provide panel detail.
repaints={}
for name,o in list(parts.items()):
 if 'Wheel'not in name:
  for i,m in enumerate(o.data.materials):
   if m and 'wire_006134006' in m.name:
    if m.name not in repaints:
     new=m.copy();new.name='M_C10_FleetPaint'
     for node in new.node_tree.nodes:
      if node.type=='TEX_IMAGE'and node.image and 'baseColor'in node.image.name:
       node.image=bpy.data.images.load(str(TEX/'C10_fleet_repaint.png'),check_existing=True)
     repaints[m.name]=new
    o.data.materials[i]=repaints[m.name]

# Original fitted 1986 bench, formed dash, door cards and realistic seam/vent hardware.
cab=[]
cab+=[ownbox('bench cushion',(0,.64,.24),(1.50,.20,.57),M['Vinyl'],.065),ownbox('bench backrest',(0,.91,.40),(1.50,.57,.12),M['Vinyl'],.055),ownbox('cab footwell',(0,.37,-.12),(1.58,.08,1.20),M['Rubber'],.025)]
for x in [i*.125 for i in range(-5,6)]:
 cab.append(rod('bench stitched piping',(x,.753,.005),(x,.753,.475),.0023,M['Dash'],6))
 cab.append(rod('backrest stitched piping',(x,.68,.328),(x,1.145,.328),.0023,M['Dash'],6))
cab += [ownbox('formed dash pad',(0,1.115,-.59),(1.63,.12,.31),M['Dash'],.035),ownbox('lower dash steel',(0,.95,-.54),(1.60,.26,.09),M['Paint'],.014)]
for x in [-.715,-.125]:cab.append(ownbox('cluster side rim',(x,1.165,-.401),(.018,.20,.024),M['Chrome'],.004))
for y in [1.065,1.265]:cab.append(ownbox('cluster horizontal rim',(-.42,y,-.401),(.60,.018,.024),M['Chrome'],.004))
# Faces/needles, radio, warning lamps, keys, steering and compass remain actual live originals.
# Leave their exact planes open; only a 12mm frame surrounds the active gauge rectangle.
for x in [-.73,.73]:
 cab.append(ownbox('louver frame',(x,1.055,-.435),(.13,.085,.028),M['Chrome'],.008))
 for z in range(5):cab.append(ownbox('louver vane',(x,1.023+z*.014,-.414),(.105,.004,.014),M['Dash'],.001))
for side in [-1,1]:
 x=side*.827
 cab.append(ownbox('pressed door card',(x,.86,-.04),(.033,.62,.99),M['Vinyl'],.025))
 cab.append(ownbox('armrest',(side*.786,.87,-.01),(.09,.065,.30),M['Dash'],.022))
 cab.append(rod('window crank',(side*.79,.94,-.32),(side*.79,.89,-.24),.006,M['Chrome']))
 cab.append(rod('window knob',(side*.79,.89,-.24),(side*.75,.89,-.24),.013,M['Dash']))
 cab.append(rod('inner door pull',(side*.79,1.02,.18),(side*.79,1.02,.30),.009,M['Chrome']))
 cab.append(rod('interior A pillar',(side*.72,1.13,-.64),(side*.68,1.60,-.50),.027,M['Dash']))
 cab.append(rod('interior roof rail',(side*.68,1.60,-.50),(side*.68,1.60,.46),.023,M['Dash']))
cab.append(ownbox('glovebox',( .48,.95,-.478),(.43,.19,.012),M['Dash'],.012));cab.append(ownbox('glove latch',(.48,1.0,-.466),(.07,.025,.014),M['Chrome'],.005))
cab.append(rod('steering column',(-.42,.88,-.50),(-.42,.985,-.24),.036,M['Dash']))
cab.append(rod('turn indicator stalk',(-.47,.985,-.27),(-.63,.99,-.26),.006,M['Chrome']))
cab.append(rod('column shift',(-.37,.985,-.27),(-.17,.945,-.22),.008,M['Chrome']))
cab.append(ownbox('rear view mirror',(0,1.57,-.393),(.19,.065,.018),M['Chrome'],.009))
parts['SM_Saro07_Cabin']=join('SM_Saro07_Cabin',cab)
# Original fleet branding covers source tailgate lettering, including any embossed manufacturer text.
logos=[ownbox('plain fleet tailgate panel',(0,.86,2.684),(1.27,.34,.024),M['Paint'],.012)]
for side in [-1,1]:
 logos.append(ownbox('fleet door paint panel',(side*.938,1.03,-.1),(.006,.34,.64),M['Paint'],.002))
 # Text lies on the door, normal faces outwards, local font X runs along vehicle length.
 rot=(math.pi/2,0,side*math.pi/2)
 logos.append(text('SARO door emblem','SARO',(side*.944,1.11,-.1),.135,rot,M['Logo']))
 logos.append(text('SARO service ID','FIELD SERVICE  /  07',(side*.945,.99,-.1),.029,rot,M['Logo']))
logos.append(text('fleet tailgate number','SARO  07',(0,.90,2.699),.12,(math.pi/2,0,math.pi),M['Logo']))
# Small plain grille badge covers original manufacturer's central marker.
logos.append(ownbox('plain grille badge',(0,.94,-2.09),(.17,.075,.018),M['Logo'],.008))
parts['SM_Saro07_Branding']=join('SM_Saro07_Branding',logos)
# Lens material parameters are driven from the same head/brake state as original instrument logic.
def fitted_lens(name,at,radius,mat):
 bpy.ops.mesh.primitive_cylinder_add(vertices=40,radius=radius,depth=.003,location=blender(at),rotation=(math.pi/2,0,0))
 o=bpy.context.object;o.name=name;o.data.materials.append(mat);return o
# Exact source front-lens bounds were measured in the actual prepared glTF. These
# thin round overlays sit 3-4mm outside its original fluted glass, not above the lamps.
head=[(-.720,.684,-2.069),(.728,.687,-2.066)]
tail=[(-.8764,.6635,2.393),(.8685,.6677,2.397)]
for key in ['Head','Tail']:
 rows[M[key].name]['alpha']=.18;M[key].node_tree.nodes.get('Principled BSDF').inputs['Alpha'].default_value=.18
parts['SM_Saro07_HeadLens']=join('SM_Saro07_HeadLens',[fitted_lens('fitted sealed beam overlay',at,.085,M['Head'])for at in head])
parts['SM_Saro07_TailLens']=join('SM_Saro07_TailLens',[fitted_lens('fitted round tail overlay',at,.045,M['Tail'])for at in tail])
# Two existing period vehicles in the old staff-bay poses. Model +X nose -> fleet Three -Z.
for model,name in [('fairheaven_lt80','SM_SaroPark_Sedan'),('el_alamo_78','SM_SaroPark_Pickup')]:
 objs,empty=imported(model)
 for o in objs:
  for v in o.data.vertices:q=v.co.copy();v.co=(-q.y,q.x,q.z)
 parts[name]=join(name,objs)
 for o in empty:bpy.data.objects.remove(o,do_unlink=True)

for m in bpy.data.materials:m.name=re.sub(r'[^A-Za-z0-9_]+','_',m.name)
portable_images={}
def portable_image(im):
 key=im.as_pointer()
 if key in portable_images:return portable_images[key]
 # Repack actual encoded source bytes, as Blender's own glTF importer does. FILE images
 # can be lazy and has_data remains false after reload until an image buffer is requested.
 # No pixel save/reload is needed and JPEG/PNG values remain byte-identical.
 if im.packed_file:raw=bytes(im.packed_file.data)
 else:
  source=Path(bpy.path.abspath(im.filepath));assert source.is_file(),('missing image file',im.name,str(source));raw=source.read_bytes()
 suffix='.png'if raw.startswith(b'\x89PNG')else'.jpg'if raw.startswith(b'\xff\xd8')else Path(im.filepath).suffix
 assert suffix in ['.png','.jpg','.jpeg'],('unsupported encoded image',im.name,suffix)
 dest=TEX/(Path(im.name).stem.replace('/','_')+suffix)
 if not dest.exists()or dest.read_bytes()!=raw:dest.write_bytes(raw)
 im.filepath_raw=str(dest);im.pack(data=raw,data_len=len(raw));portable_images[key]=dest;return dest
def socket_image(socket):
 # Follow only this semantic Principled input; specular/clearcoat images must never
 # be inferred as BaseColor just because they are present in the same node tree.
 pending=[link.from_node for link in socket.links];seen=set();images={}
 while pending:
  node=pending.pop()
  if node.as_pointer()in seen:continue
  seen.add(node.as_pointer())
  if node.type=='TEX_IMAGE'and node.image:images[node.image.as_pointer()]=node.image;continue
  pending.extend(link.from_node for inp in node.inputs for link in inp.links)
 assert len(images)<=1,('ambiguous texture source',socket.name,[im.name for im in images.values()])
 return next(iter(images.values()),None)

# Collect portable image pixels without changing source UVs or baking lighting.
# glTF OpenGL normal convention is declared explicitly for native green-channel inversion.
for o in parts.values():
 for m in o.data.materials:
  if m.name in rows:continue
  p=next((n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None)if m.use_nodes else None
  row={'name':m.name,'base_color_linear':list(p.inputs['Base Color'].default_value)if p else list(m.diffuse_color),'roughness':float(p.inputs['Roughness'].default_value)if p else .7,'metallic':float(p.inputs['Metallic'].default_value)if p else 0,'textures':{}}
  if m.name.startswith('14___Default') or 'Transperent'in m.name:row['alpha']=.16
  if p:
   row['alpha']=min(row.get('alpha',1),float(p.inputs['Alpha'].default_value))
   base=socket_image(p.inputs['Base Color']);normal=socket_image(p.inputs['Normal'])
   rough=socket_image(p.inputs['Roughness']);metal=socket_image(p.inputs['Metallic'])
   for role,im in [('base_color',base),('normal',normal)]:
    if im:row['textures'][role]=str(portable_image(im).relative_to(WORK))
   if base and p.inputs['Base Color'].links[0].from_node.type=='TEX_IMAGE':
    # A linked direct image ignores Principled's unused .8 UI default.
    row['base_color_linear']=[1,1,1,1]
   if rough and metal:
    assert rough==metal,('separate roughness/metallic maps need explicit conversion',m.name)
    row['packed_metallic_roughness']=str(portable_image(rough).relative_to(WORK))
   elif rough:row['textures']['roughness']=str(portable_image(rough).relative_to(WORK))
   row['normal_format']='OpenGL'
  rows[m.name]=row
for key in ['Paint','Vinyl','Dash']:
 row=rows[M[key].name];row['base_color_linear']=[1,1,1,1]
 row['textures']={'base_color':'textures/'+key+'_base.png','roughness':'textures/'+key+'_rough.png','normal':'textures/'+key+'_normalDX.png'};row['normal_format']='DirectX';row['normal_strength']=.35
 m=M[key];p=m.node_tree.nodes.get('Principled BSDF')
 for role,file in row['textures'].items():
  image=bpy.data.images.load(str(WORK/file),check_existing=True);image.colorspace_settings.name='sRGB'if role=='base_color'else'Non-Color'
  node=m.node_tree.nodes.new('ShaderNodeTexImage');node.image=image
  if role=='normal':
   # DirectX green is inverted for Blender's OpenGL tangent convention.
   sep=m.node_tree.nodes.new('ShaderNodeSeparateColor');comb=m.node_tree.nodes.new('ShaderNodeCombineColor');inv=m.node_tree.nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1
   m.node_tree.links.new(node.outputs['Color'],sep.inputs[0]);m.node_tree.links.new(sep.outputs['Red'],comb.inputs['Red']);m.node_tree.links.new(sep.outputs['Green'],inv.inputs[1]);m.node_tree.links.new(inv.outputs[0],comb.inputs['Green']);m.node_tree.links.new(sep.outputs['Blue'],comb.inputs['Blue'])
   norm=m.node_tree.nodes.new('ShaderNodeNormalMap');norm.inputs['Strength'].default_value=.35;m.node_tree.links.new(comb.outputs[0],norm.inputs['Color']);m.node_tree.links.new(norm.outputs['Normal'],p.inputs['Normal'])
  else:m.node_tree.links.new(node.outputs['Color'],p.inputs['Base Color'if role=='base_color'else'Roughness'])
# Original paint/cab materials use geometry wear and fine mesh seams, no baked lighting.
for m in bpy.data.materials:
 if m.use_nodes:
  for n in m.node_tree.nodes:
   if n.type=='TEX_IMAGE'and n.image:portable_image(n.image)

# Exact union: old staff-car bodies plus the sixteen obsolete wing window fittings.
# The new SARO facade owns all reveals, sills and blinds. Never hide the mixed g454
# parent: it also contains roof equipment, meters, drums, curbs and other site props.
original=set(bpy.data.objects);legacy_source=Path('/home/tom/scratch-bm/Signal47-web/geo/geo_003.glb');bpy.ops.import_scene.gltf(filepath=str(legacy_source))
loaded=[o for o in bpy.data.objects if o not in original];target=next((o for o in loaded if o.type=='MESH'and o.name=='g454'),None)
assert target,'immutable original g454 missing'
world=target.matrix_world.copy();target.parent=None;target.matrix_world=Matrix.Identity(4);target.data.transform(world)
bm=bmesh.new();bm.from_mesh(target.data);remove=[];uv=bm.loops.layers.uv.active
assert uv is not None and len(bm.faces)==13550,('immutable g454 face/UV guard',len(bm.faces))
# Exact source dressSaro.ts window apertures: x21+2.25*i ..22.45+2.25*i,
# y.58..2.34, at z+/-5.5. Include only their 6cm sill/4.5cm frame/5mm blinds.
window_rows=[]
for side,expected in [(-1,[108,204,48,48,48,48,48,144]),(1,[108,168,228,156,216,48,204,48])]:
 for index in range(8):
  a0=21+2.25*index;a1=a0+1.45
  window_rows.append({'side':'north'if side<0 else'south','index':index,'sign':side,'bounds_three_m':[a0-.061,a1+.061,.529,2.387,5.4999 if side>0 else-5.5501,5.5501 if side>0 else-5.4999],'expected_triangles':expected[index],'removed_triangles':0})
def source_three(v):return (float(v.co.x),float(v.co.z),float(-v.co.y))
def window_point(v,row):
 x,y,z=source_three(v);x0,x1,y0,y1,z0,z1=row['bounds_three_m'];return x0<=x<=x1 and y0<=y<=y1 and z0<=z<=z1
# A signature covers every retained corner's coordinate, UV, winding and material.
# No new UV projection or topology reduction is applied by this union selection.
def bm_signature(face):
 return (face.material_index,tuple((float(loop.vert.co.x),float(loop.vert.co.y),float(loop.vert.co.z),float(loop[uv].uv.x),float(loop[uv].uv.y))for loop in face.loops))
kept_signatures=[];car_removed=0;window_removed=0
for f in bm.faces:
 assert len(f.verts)==3
 def car(v):
  x,y,z=source_three(v)
  return y>-.57 and ((25.8<=x<=28.2 and 6.6<=z<=12.6)or(29.8<=x<=32.2 and 6.55<=z<=13.25))
 is_car=all(car(v)for v in f.verts)
 hits=[row for row in window_rows if all(window_point(v,row)for v in f.verts)]
 assert len(hits)<=1 and not(is_car and hits),('overlapping removal recipes',len(hits))
 if is_car:car_removed+=1;remove.append(f)
 elif hits:hits[0]['removed_triangles']+=1;window_removed+=1;remove.append(f)
 else:kept_signatures.append(bm_signature(f))
assert car_removed==976,('existing exact car trim changed',car_removed)
assert all(row['removed_triangles']==row['expected_triangles']for row in window_rows),window_rows
assert window_removed==1872 and len(kept_signatures)==10702
trim_removed=len(remove);bmesh.ops.delete(bm,geom=remove,context='FACES');bm.to_mesh(target.data);bm.free()
actual_uv=target.data.uv_layers.active
actual_signatures=[(face.material_index,tuple((float(target.data.vertices[target.data.loops[li].vertex_index].co.x),float(target.data.vertices[target.data.loops[li].vertex_index].co.y),float(target.data.vertices[target.data.loops[li].vertex_index].co.z),float(actual_uv.data[li].uv.x),float(actual_uv.data[li].uv.y))for li in face.loop_indices))for face in target.data.polygons]
assert sorted(actual_signatures)==sorted(kept_signatures),'Unrelated source corner/UV/winding/material changed'
window_trim_proof={'status':'SOURCE_INDEX_UNION_VERIFIED_NATIVE_UNVERIFIED','source_geo':454,'source_file_sha256':hashlib.sha256(legacy_source.read_bytes()).hexdigest(),'source_triangles':13550,'car_removed_triangles':car_removed,'window_removed_triangles':window_removed,'result_triangles':len(actual_signatures),'window_openings':window_rows,'retained_corner_uv_material_sha256':hashlib.sha256(json.dumps(sorted(actual_signatures),separators=(',',':')).encode()).hexdigest(),'retained_corner_uv_material_exact':True,'preserved':'Every triangle outside the two old car bodies and sixteen individual old window prisms; roof/details/lights/downspouts/meters/site props and all gameplay nodes retained.'}
(WORK/'legacy-dressing-trim-proof.json').write_text(json.dumps(window_trim_proof,indent=2)+'\n')
target.name='SM_SaroPark_LegacyTrim';parts[target.name]=target
for o in loaded:
 if o!=target:bpy.data.objects.remove(o,do_unlink=True)
contract={'revision':REVISION,'axis':'Blender metres Z-up; Three=(Bx,Bz,-By); body nose Three-Z; FBX -Y/Z force_front_x_axis','driver_eye_three':[-.42,1.48,.2],'parts':{},'wheels':sorted(wheel_centres,key=lambda w:w['part']),'parked':PARKED,'credits':CREDITS,'legacy_trim_removed_triangles':trim_removed,'legacy_window_trim':window_trim_proof}
for name,o in parts.items():
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
 bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.triangulate(bm,faces=list(bm.faces));bad=[f for f in bm.faces if f.calc_area()<1e-9]
 if bad:bmesh.ops.delete(bm,geom=bad,context='FACES')
 bm.to_mesh(o.data);bm.free()
 if not o.data.uv_layers:
  bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.02);bpy.ops.object.mode_set(mode='OBJECT')
 lo=[min(v.co[i]for v in o.data.vertices)for i in range(3)];hi=[max(v.co[i]for v in o.data.vertices)for i in range(3)]
 contract['parts'][name]={'asset_folder':OWN_ROOT if name in ['SM_Saro07_Cabin','SM_Saro07_Branding','SM_Saro07_HeadLens','SM_Saro07_TailLens'] else ROOT,'bounds_local_m':[lo,hi],'triangles':len(o.data.polygons),'materials':[m.name for m in o.data.materials],'lods':[]}
 for lod in range(3):
  e=o if lod==0 else o.copy()
  if lod:
   e.data=o.data.copy();scene.collection.objects.link(e);bpy.ops.object.select_all(action='DESELECT');e.select_set(True);bpy.context.view_layer.objects.active=e
   d=e.modifiers.new('Authored reduction','DECIMATE');d.ratio=1 if name=='SM_SaroPark_LegacyTrim' else [1,.52,.24][lod];bpy.ops.object.modifier_apply(modifier=d.name)
   bm=bmesh.new();bm.from_mesh(e.data);bmesh.ops.triangulate(bm,faces=list(bm.faces))
   bad=[f for f in bm.faces if f.calc_area()<1e-9]
   if bad:bmesh.ops.delete(bm,geom=bad,context='FACES')
   bm.to_mesh(e.data);bm.free()
  if name in ['SM_SaroPark_Sedan','SM_SaroPark_Pickup']:
   # UE native LOD0 removed 8 microscopic pickup faces although FBX roundtrip matched.
   # Apply measured staff-car weld/short-edge cleanup to EVERY authored LOD, including0.
   bm=bmesh.new();bm.from_mesh(e.data);bmesh.ops.triangulate(bm,faces=list(bm.faces))
   before_faces=len(bm.faces);before_verts=len(bm.verts)
   bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-5)
   bad=[f for f in bm.faces if f.calc_area()<5e-9 or any(edge.calc_length()<2e-5 for edge in f.edges)]
   if bad:bmesh.ops.delete(bm,geom=bad,context='FACES')
   cleanup_proof.append({'part':name,'lod':lod,'weld_tolerance_m':1e-5,'minimum_edge_m':2e-5,'minimum_area_m2':5e-9,'faces_before':before_faces,'faces_after':len(bm.faces),'verts_before':before_verts,'verts_after':len(bm.verts)})
   bm.to_mesh(e.data);bm.free()
  # Actual diagnostic proved Blender validate removes exactly the FBX-lost 3/5
  # duplicate/invalid LOD2 faces. Clean source geometry BEFORE contract/export.
  before_validate=sum(len(p.vertices)-2 for p in e.data.polygons)
  validated=e.data.validate(verbose=True,clean_customdata=False);e.data.update()
  after_validate=sum(len(p.vertices)-2 for p in e.data.polygons)
  validation_proof.append({'part':name,'lod':lod,'changed':validated,'triangles_before':before_validate,'triangles_after':after_validate})
  if lod==0:
   contract['parts'][name]['triangles']=after_validate
   contract['parts'][name]['bounds_local_m']=[[fn(v.co[i]for v in e.data.vertices)for i in range(3)]for fn in [min,max]]
  suffix=''if not lod else'_LOD'+str(lod)
  bpy.ops.object.select_all(action='DESELECT');e.select_set(True);bpy.context.view_layer.objects.active=e
  bpy.ops.export_scene.fbx(filepath=str(OUT/(name+suffix+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,add_leaf_bones=False,axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True)
  contract['parts'][name]['lods'].append({'lod':lod,'triangles':sum(len(p.vertices)-2 for p in e.data.polygons),'materials':[m.name for m in e.data.materials]})
  if lod:bpy.data.objects.remove(e,do_unlink=True)
# Axis probe allows root to prove the importer transform before native placement.
probe=ownbox('axis_probe',(2,6,-4),(.2,.6,.4),M['Paint'],0);bpy.ops.object.select_all(action='DESELECT');probe.select_set(True);bpy.context.view_layer.objects.active=probe
bpy.ops.export_scene.fbx(filepath=str(WORK/'axis_probe.fbx'),use_selection=True,axis_forward='-Y',axis_up='Z');bpy.data.objects.remove(probe,do_unlink=True)
contract['targeted_geometry_cleanup']=cleanup_proof
contract['mesh_validation_cleanup']=validation_proof
(WORK/'model_contract.json').write_text(json.dumps(contract,indent=2));(WORK/'materials.json').write_text(json.dumps(list(rows.values()),indent=2))
# Editable presentation assembly; individual FBX files above retain canonical zero pivots.
for wheel in wheel_centres:parts[wheel['part']].location=blender(wheel['three'])
parts['SM_SaroPark_Sedan'].location=(5.8,0,0);parts['SM_SaroPark_Pickup'].location=(-5.8,0,0)
parts['SM_SaroPark_LegacyTrim'].hide_render=True;parts['SM_SaroPark_LegacyTrim'].hide_set(True)
# Save the complete source first. Original-only public source is exported by a SEPARATE
# fresh process (export_own_addons.py); libraries.write(Scene) crashed Blender5.2.2 here.
scene['source_revision']=REVISION;scene['credits']=json.dumps(CREDITS)
bpy.ops.wm.save_as_mainfile(filepath=str(WORK/'SaroFleet.blend'))
bpy.ops.object.select_all(action='DESELECT')
for name,o in parts.items():
 if name!='SM_SaroPark_LegacyTrim':o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(WORK/'SaroFleet.glb'),export_format='GLB',use_selection=True,export_apply=True)
print('S47_FLEET_BUILD_READY '+str(WORK))
