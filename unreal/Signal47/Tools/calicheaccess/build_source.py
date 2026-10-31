"""Editable playtest access overlay. Root runs Blender; original TB2/Homes sources stay intact.
blender -b -t 2 --python build_source.py -- --out /absolute/output
Exports the five complete original landmark interiors with separate movable leaves,
four simple furnished house interiors and leaves. No engine/process launch.
"""
import argparse, importlib.util, json, math, sys
from pathlib import Path
import bpy, bmesh
R=Path(__file__).resolve().parent
TB=R.parent/'townbuildings';sys.path.insert(0,str(TB))
from geometry import Mesh,table,chair,cabinet,trim_room
from design import BUILDINGS
ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,required=True)
a=ap.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
OUT=a.out.resolve();(OUT/'exports').mkdir(parents=True,exist_ok=True)
HOMES=json.loads((R.parents[2]/'ArtSource/CalicheHomes/source-contract.json').read_text())['houses']
rows={};buildings=[]
for ob in list(bpy.data.objects):bpy.data.objects.remove(ob,do_unlink=True)
bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1

def copyfaces(src,indices,name):
 m=Mesh(name);mapping={}
 for k in indices:
  f=src.f[k];face=[]
  for vi in f:
   if vi not in mapping:mapping[vi]=len(m.v);m.v.append(src.v[vi])
   face.append(mapping[vi])
  m.f.append(face);m.mat.append(src.mat[k]);m.uv.append(src.uv[k]);m.smooth.append(src.smooth[k]);m.tags.append(src.tags[k])
 return m

def split_door(data,family,b):
 x0,x1,y0,y1=b['footprint'];front=x1 if b['front']>0 else x0;dy,w,dh=b['door']
 # Entire leaf geometry, including upper glass and hardware; keep actual frame/jambs.
 door=Mesh('SM_Access_'+family+'_Door');kept={}
 for name,m in data.items():
  move=[]
  for k,face in enumerate(m.f):
   pts=[m.v[i] for i in face];center=[sum(p[j]for p in pts)/len(pts)for j in range(3)]
   tag=m.tags[k].lower()
   inleaf=abs(center[0]-front)<.48 and abs(center[1]-dy)<w/2-.055 and .025<center[2]<dh-.055
   fixed=any(t in tag for t in ['jamb','frame','casing','threshold','stoop','lamp','head_lining','head_cornice','plinth','canopy'])
   # Fixtures tagged chain/closed card move with the leaf; all geometry behind the leaf is kept.
   if inleaf and not fixed:move.append(k)
  leaf=copyfaces(m,move,'piece');door.merge(leaf)
  kept[name]=copyfaces(m,[k for k in range(len(m.f))if k not in set(move)],'SM_Access_'+name.removeprefix('SM_Town_'))
 assert len(door.f)>10,(family,'no complete entry leaf')
 # Move vertices to a stable lower hinge; UE runtime rotates this component, never the building.
 hinge=(front,dy-w/2+.055,0)
 door.v=[tuple(p[j]-hinge[j]for j in range(3))for p in door.v]
 return kept,door,hinge

def export(m,lod,root,family,role):
 if not m.f:return
 name=m.name+(''if lod==0 else '_LOD'+str(lod))
 me=bpy.data.meshes.new(name);me.from_pydata(m.v,[],m.f);me.update()
 used=sorted(set(m.mat))
 for mat in used:
  material=bpy.data.materials.get(mat) or bpy.data.materials.new(mat);me.materials.append(material)
 uv=me.uv_layers.new(name='UVMap')
 for p,ma,coords,smooth in zip(me.polygons,m.mat,m.uv,m.smooth):
  p.material_index=used.index(ma);p.use_smooth=smooth
  for li,t in zip(p.loop_indices,coords):uv.data[li].uv=t
 ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob);bpy.context.view_layer.objects.active=ob
 mod=ob.modifiers.new('triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=mod.name)
 bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-6);bm.to_mesh(ob.data);bm.free()
 bpy.ops.object.select_all(action='DESELECT');ob.select_set(True)
 bpy.ops.export_scene.fbx(filepath=str(OUT/'exports'/(name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
 row=rows.setdefault(m.name,dict(name=m.name,root=root,family=family,role=role,materials=used,lod_triangles=[]))
 row['lod_triangles'].append(len(ob.data.polygons))
 if lod>0:bpy.data.objects.remove(ob,do_unlink=True)

def furniture_colliders(m,root):
 # Independent closed fixture islands, never one broad rectangle around repeated chairs/pews.
 parent=list(range(len(m.v)))
 def find(i):
  while parent[i]!=i:parent[i]=parent[parent[i]];i=parent[i]
  return i
 positions={}
 for i,p in enumerate(m.v):
  key=tuple(round(v,6)for v in p)
  if key in positions:parent[find(i)]=find(positions[key])
  else:positions[key]=i
 for f in m.f:
  for v in f[1:]:parent[find(v)]=find(f[0])
 groups={}
 for k,f in enumerate(m.f):groups.setdefault(find(f[0]),[]).append(k)
 result=[]
 for faces in groups.values():
  tags=' '.join({m.tags[k].lower()for k in faces})
  if any(w in tags for w in ['floor','ceiling','skirting','paper','light','cable','wire','cord','mould','trim','stain']):continue
  pts=[m.v[i]for k in faces for i in m.f[k]];lo=[min(p[j]for p in pts)for j in range(3)];hi=[max(p[j]for p in pts)for j in range(3)]
  if hi[2]>.25 and lo[2]<1.55 and .12<hi[0]-lo[0]<8 and .12<hi[1]-lo[1]<8:
   rect=[root[0]+lo[0],root[0]+hi[0],root[2]-hi[1],root[2]-lo[1]]
   if rect not in result:result.append(rect)
 return result

for family in ['Sheriff','Laundry','Feed','Cinema','Church']:
 sp=importlib.util.spec_from_file_location('access_'+family,TB/'tb2'/(family.lower()+'.py'));module=importlib.util.module_from_spec(sp);sp.loader.exec_module(module)
 b=BUILDINGS[family];root=b['root'];x0,x1,y0,y1=b['footprint'];dy,w,dh=b['door'];front=x1 if b['front']>0 else x0
 for lod in range(3):
  original=module.build(lod);parts,leaf,hinge=split_door(original,family,b)
  # Supply continuous support under sheriff bench, trash can and entry, all at the authored floor.
  if family=='Sheriff':
   shell=parts['SM_Town_Sheriff_Shell'];shell.box('Supported_front_apron',(front+.73,0,-1.5),(1.8,12.8,3.0),'Concrete')
  for m in parts.values():export(m,lod,root,family,'static')
  export(leaf,lod,[root[0]+hinge[0],root[1],root[2]-hinge[1]],family,'door')
  if lod==0:
   floor=.95 if family=='Feed' else 0
   buildings.append(dict(id=family,root=root,footprint=[x0,x1,y0,y1],floor=floor,door=[front,dy,w,dh],front=b['front'],hinge=list(hinge),old_prefix='SM_Town_'+family+'_',interact={'Sheriff':'sheriffDoor','Laundry':'laundromat','Feed':'feed','Cinema':'cinema','Church':'churchDoor'}[family],furniture=furniture_colliders(next(m for n,m in original.items()if n.endswith('_Interior')),root)))

for family,h in HOMES.items():
 root=h['root_world_three_m'];x0,x1,y0,y1=h['footprint_blender_xy_m'];front=x1 if h['front']>0 else x0;dy=h['door_y_m'];w=.95;dh=2.1;hinge=(front,dy-w/2,0)
 for lod in range(3):
  inside=Mesh('SM_Access_'+family+'_Interior');trim_room(inside,x0+.18,x1-.18,y0+.18,y1-.18,2.8,lod,'Linoleum')
  table(inside,((x0+x1)/2,(y0+y1)/2),(1.2,.8),.76)
  chair(inside,(x0+x1)/2-.8,(y0+y1)/2,lod,yaw=-math.pi/2)
  cabinet(inside,x0+.65,y0+1.5,.7,.5,1.35,3,lod)
  # Bed/sofa with a clear path from the front door to the centre.
  inside.box('Guest_sofa_base',(x1-1.2,y1-1.1,.22),(1.6,.75,.44),'DarkWood');inside.rounded_box('Worn_sofa_seat',(x1-1.2,y1-1.1,.49),(1.65,.8,.16),.06,'Upholstery',2)
  inside.box('Worn_sofa_back',(x1-1.2,y1-.72,.8),(1.65,.13,.6),'Upholstery')
  leaf=Mesh('SM_Access_'+family+'_Door');leaf.box('Timber_panelled_entry',(0,w/2,dh/2),(.06,w,dh),'DarkWood')
  for z in [.45,1.45]:leaf.box('Recessed_timber_panel',(-.036,w/2,z),(.016,w-.18,.62),'Timber')
  leaf.lathe('Brass_entry_knob',(-.08,w-.1,1.02),[(0,.03),(.035,.03)],'Metal',8,axis=(1,0,0))
  export(inside,lod,root,family,'static');export(leaf,lod,[root[0]+hinge[0],root[1],root[2]-hinge[1]],family,'door')
  if lod==0:buildings.append(dict(id=family,root=root,footprint=[x0,x1,y0,y1],floor=0,door=[front,dy,w,dh],front=h['front'],hinge=list(hinge),legacy_door_holder=h['legacy_door_holder'],furniture=furniture_colliders(inside,root)))

# The existing souvenir shop is an original editable asset with a complete interior.
# Load copies only, rotate its local frame to the same world mapping as TB2.
source=R.parent/'calichestreet/souvenir/Saucer_Crash_1947.blend'
with bpy.data.libraries.load(str(source),link=False) as (available,wanted):
 wanted.objects=[n for n in available.objects if n.startswith('SM_Souvenir_')]
souvenir={}
for ob in wanted.objects:
 if not ob or ob.type!='MESH':continue
 m=Mesh(ob.name)
 m.v=[(v.co.y,-v.co.x,v.co.z)for v in ob.data.vertices]
 uv=ob.data.uv_layers.active
 for p in ob.data.polygons:
  m.f.append(list(p.vertices));m.mat.append(ob.data.materials[p.material_index].name);m.smooth.append(p.use_smooth);m.tags.append('Original_souvenir_piece');m.uv.append([tuple(uv.data[li].uv)for li in p.loop_indices])
 souvenir[ob.name]=m
root=[8025,4.952807474136353,2064]
b=dict(footprint=[-4.5,4.5,-6,6],front=-1,door=[-2.8,1.,2.15])
for lod in range(3):
 parts,leaf,hinge=split_door(souvenir,'Souvenir',b)
 for key,m in parts.items():
  m.name='SM_Access_Souvenir_'+key.removeprefix('SM_Souvenir_');export(m,lod,root,'Souvenir','static')
 export(leaf,lod,[root[0]+hinge[0],root[1],root[2]-hinge[1]],'Souvenir','door')
buildings.append(dict(id='Souvenir',root=root,footprint=b['footprint'],floor=0,door=[-4.5,-2.8,1.,2.15],front=-1,hinge=list(hinge),retire_props=['prop:souvenirShell','prop:souvenirInterior','prop:souvenirGlass'],interact='souvenirDoor',furniture=[]))
for ob in wanted.objects:
 if ob:bpy.data.objects.remove(ob,do_unlink=True)

payload=dict(version=1,status='SOURCE_READY_NATIVE_UNVERIFIED',parts=rows,buildings=buildings)
(OUT/'source.json').write_text(json.dumps(payload,indent=2)+'\n')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Caliche_Playtest_Access.blend'),compress=True)
print('CALICHE_ACCESS_SOURCE_READY',len(rows),len(buildings),flush=True)
