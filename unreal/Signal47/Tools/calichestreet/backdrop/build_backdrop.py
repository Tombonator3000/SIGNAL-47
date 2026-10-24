"""Original stratified mesa context for Caliche. Blender Z-up metres, same ground anchor.
No collision and no changes to driving/walking terrain. Run Blender --background --python this_file.
"""
import bpy, json, math, random
from pathlib import Path
import numpy as np
from mathutils import Vector

ROOT=Path(__file__).resolve().parent
OUT=ROOT/'exports';OUT.mkdir(parents=True,exist_ok=True)
TEX=ROOT/'textures';TEX.mkdir(exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
rng=np.random.default_rng(4701986);size=1024
def noise(n):
 a=rng.random((n,n));a=np.pad(a,((0,1),(0,1)),mode='wrap')
 x=np.arange(size)*n/size;i=x.astype(int);f=x-i;f=f*f*(3-2*f)
 return ((a[i[:,None],i[None,:]]*(1-f)[None,:]+a[i[:,None],i[None,:]+1]*f[None,:])*(1-f)[:,None]+
         (a[i[:,None]+1,i[None,:]]*(1-f)[None,:]+a[i[:,None]+1,i[None,:]+1]*f[None,:])*f[:,None])
fine=noise(192);n=noise(12)*.45+noise(47)*.3+fine*.25
yy=np.arange(size)[:,None]/size
strata=np.clip(noise(5)*.65+noise(55)*.15+fine*.2,0,1)
height=n*.12+strata*.10
dx=np.gradient(height,axis=1)*1.5;dy=np.gradient(height,axis=0)*1.5
norm=np.stack((-dx,dy,np.ones_like(n)),axis=2);norm/=np.linalg.norm(norm,axis=2)[:,:,None]
def image(name,values,srgb):
 im=bpy.data.images.new(name,width=size,height=size,alpha=True)
 if not srgb:im.colorspace_settings.name='Non-Color'
 rgba=np.ones((size,size,4),dtype=np.float32)
 rgba[:,:,:3]=values if values.ndim==3 else values[:,:,None]
 im.pixels.foreach_set(rgba.ravel());im.filepath_raw=str(TEX/(name+'.png'));im.file_format='PNG';im.save()
 return 'textures/'+name+'.png',im
base=np.clip(np.array([.41,.335,.265])[None,None,:]*(.78+n[:,:,None]*.45)+strata[:,:,None]*.035,0,1)
maps={}
for role,values,srgb in [('base_color',base,True),('normal',norm*.5+.5,False),('roughness',.83+fine*.12,False)]:
 path,im=image('MesaRock_'+role,values,srgb);maps[role]=(path,im)
mats=[];rows=[]
for k,gain in enumerate([.86,1.,.93,1.09,1.02,.90]):
 name='M_Caliche_Mesa_'+str(k);m=bpy.data.materials.new(name);m.use_nodes=True
 bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(.31*gain,.25*gain,.20*gain,1)
 bs.inputs['Roughness'].default_value=.9
 for role,(path,im) in maps.items():
  t=m.node_tree.nodes.new('ShaderNodeTexImage');t.image=im
  if role=='normal':
   # DirectX image to Blender's OpenGL tangent normal.
   sep=m.node_tree.nodes.new('ShaderNodeSeparateXYZ');combine=m.node_tree.nodes.new('ShaderNodeCombineXYZ')
   inv=m.node_tree.nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1
   m.node_tree.links.new(t.outputs['Color'],sep.inputs[0]);m.node_tree.links.new(sep.outputs['Y'],inv.inputs[1])
   m.node_tree.links.new(sep.outputs['X'],combine.inputs['X']);m.node_tree.links.new(inv.outputs[0],combine.inputs['Y']);m.node_tree.links.new(sep.outputs['Z'],combine.inputs['Z'])
   nm=m.node_tree.nodes.new('ShaderNodeNormalMap');nm.inputs['Strength'].default_value=.35
   m.node_tree.links.new(combine.outputs[0],nm.inputs['Color']);m.node_tree.links.new(nm.outputs[0],bs.inputs['Normal'])
  else:m.node_tree.links.new(t.outputs['Color'],bs.inputs['Base Color' if role=='base_color' else 'Roughness'])
 mats.append(m);rows.append({'name':name,'base_color_linear':[.31*gain,.25*gain,.20*gain,1],
  'metallic':0,'roughness':.9,'normal_strength':.35,'uv_scale':1,'texture_tint_linear':[gain,gain*.99,gain*.97],
  'textures':{r:p[0] for r,p in maps.items()}})
v=[];faces=[];indices=[];uv=[]
def face(points,mi,coords):
 start=len(v);v.extend(reversed(points));faces.append(tuple(range(start,start+len(points))));indices.append(mi);uv.extend(reversed(coords))
mesas=[(460,115,155,125,106,47),(-410,45,165,145,122,48),
       (510,-515,215,155,138,49),(-460,-460,235,165,145,50),
       (-195,-770,135,150,116,51),(240,560,145,130,130,52),(-250,630,170,145,146,53)]
profiles=[(0,1.22),(.08,1.12),(.18,.97),(.26,.89),(.33,.85),(.36,.81),(.41,.77),(.44,.79),(.51,.70),(.64,.64),(.68,.67),(.74,.63),(.88,.59),(.93,.60),(1,.57)]
for cx,cz,rx,rz,H,seed in mesas:
 rr=random.Random(seed);count=112
 macro=[rr.uniform(-.11,.11) for _ in range(15)]
 vertical=[rr.uniform(-.045,.045) for _ in range(count)]
 def radial(i):
  u=i/count*len(macro);a=int(u)%len(macro);f=u-int(u)
  return macro[a]*(1-f)+macro[(a+1)%len(macro)]*f+vertical[i%count]
 rings=[]
 for j,(h,r) in enumerate(profiles):
  ring=[]
  for i in range(count):
   a=math.tau*i/count;f=radial(i);ledge=rr.uniform(-.012,.012)
   radius=r+f*(.65+.35*h)+ledge
   rockh=H*h+(rr.uniform(-.75,.75) if j<len(profiles)-1 else vertical[i]*9)
   ring.append((cx+math.cos(a)*rx*radius,-(cz+math.sin(a)*rz*radius),rockh))
  rings.append(ring)
 for j in range(len(rings)-1):
  mi=[1,0,2,1,4,0,3,1,2,5,1,3,2,4][j]
  for i in range(count):
   ni=(i+1)%count
   pts=[rings[j][i],rings[j][ni],rings[j+1][ni],rings[j+1][i]]
   # Winding is corrected below; local Y points toward negative world road Z.
   face(pts,mi,[(i/count*rx*.27,profiles[j][0]*H/13),((i+1)/count*rx*.27,profiles[j][0]*H/13),
                ((i+1)/count*rx*.27,profiles[j+1][0]*H/13),(i/count*rx*.27,profiles[j+1][0]*H/13)])
 center=(cx,-cz,H-.4)
 for i in range(count):
  pts=[center,rings[-1][i],rings[-1][(i+1)%count]]
  face(pts,2,[(p[0]/18,p[1]/18) for p in pts])
mesh=bpy.data.meshes.new('SM_Caliche_Mesas');mesh.from_pydata(v,[],faces);mesh.update()
for m in mats:mesh.materials.append(m)
for p,idx in zip(mesh.polygons,indices):p.material_index=idx
layer=mesh.uv_layers.new(name='UVMap')
for loop in mesh.loops:layer.data[loop.index].uv=uv[loop.vertex_index]
ob=bpy.data.objects.new('SM_Caliche_Mesas',mesh);bpy.context.collection.objects.link(ob)
bpy.context.view_layer.objects.active=ob;ob.select_set(True)
mod=ob.modifiers.new('Triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=mod.name)
for p in mesh.polygons:p.use_smooth=False
ob['anchor_world_three_m']=[8000,0,2060];ob['collision']='disabled_visual_context';ob['original']='Authored procedural mesa geology, no third-party data'
coords=[tuple(p.co) for p in ob.data.vertices]
contract={'parts':{ob.name:{'bounds_local_m':[[min(p[i] for p in coords) for i in range(3)],[max(p[i] for p in coords) for i in range(3)]],
 'triangles':len(ob.data.polygons),'materials':[m.name for m in mats]}}}
(ROOT/'model_contract.json').write_text(json.dumps(contract,indent=2));(ROOT/'materials.json').write_text(json.dumps(rows,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Caliche_Mesas.blend'))
bpy.ops.export_scene.fbx(filepath=str(OUT/'SM_Caliche_Mesas.fbx'),use_selection=True,apply_unit_scale=True,axis_forward='-Z',axis_up='Y',use_mesh_modifiers=True,mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False)
bpy.ops.export_scene.gltf(filepath=str(OUT/'SM_Caliche_Mesas.glb'),export_format='GLB',use_selection=True,export_apply=True,export_yup=True)
print('CALICHE MESA SOURCE',contract)
