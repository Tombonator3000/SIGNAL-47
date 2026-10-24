"""Exact Blender FBX roundtrip and shop/A02 compatibility checks, no rendering."""
import bpy, json, struct, math, sys, hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
contract=json.loads((ROOT/'model_contract.json').read_text()); checks=[]
def check(name,ok,**evidence): checks.append({'name':name,'result':'PASS' if ok else 'FAIL',**evidence})
for name,expected in contract['parts'].items():
 bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
 for m in list(bpy.data.materials): bpy.data.materials.remove(m)
 bpy.ops.import_scene.fbx(filepath=str(ROOT/'exports'/(name+'.fbx')),use_custom_normals=True)
 objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
 check(name+' separate mesh',len(objects)==1,count=len(objects))
 if len(objects)!=1: continue
 o=objects[0]; m=o.data; m.calc_loop_triangles()
 bounds=[[min(v.co[i] for v in m.vertices) for i in range(3)],[max(v.co[i] for v in m.vertices) for i in range(3)]]
 err=max(abs(bounds[j][i]-expected['bounds_local_m'][j][i]) for j in range(2) for i in range(3))
 check(name+' meter scale and common pivot',err<.00005 and o.location.length<.00005 and max(abs(v-1) for v in o.scale)<.00005,bounds_m=bounds,max_bounds_error_m=err)
 check(name+' exact triangle contract',len(m.loop_triangles)==expected['triangles'] and all(t.area>0 for t in m.loop_triangles) and all(len(p.vertices)==3 for p in m.polygons),triangles=len(m.loop_triangles),expected=expected['triangles'],zero_area=sum(t.area==0 for t in m.loop_triangles))
 check(name+' material order',[v.name for v in m.materials]==expected['materials'],slots=[v.name for v in m.materials])
 check(name+' UV coverage',len(m.uv_layers)>0 and all(len(layer.data)==len(m.loops) for layer in m.uv_layers),layers=[v.name for v in m.uv_layers],loops=len(m.loops))
 check(name+' finite normals',all(p.normal.length>.99 and all(math.isfinite(v) for v in p.normal) for p in m.polygons))
 if name=='SM_Souvenir_Shell':
  def hit(x,z): return o.ray_cast(Vector((x,-5.15,z)),Vector((0,1,0)),distance=1.0)[0]
  check('Original display is a real opening',not hit(-1.7,1.45) and hit(-3.4,1.45),window_clear=not hit(-1.7,1.45),adjacent_pier_solid=hit(-3.4,1.45))
  check('Existing shop door stays closed',hit(2.8,1.1))
  clear=all(not hit(x,z) for x in [-2.55,-2.1,-1.7,-1.3,-.85] for z in [.80,1.2,1.7,2.1])
  check('A02 window silhouette and photograph rays remain clear',clear,sampled_rays=20)
 if name=='SM_Souvenir_Interior':
  check('All interior stock remains behind the A02 plane',bounds[0][1]>-4.55,nearest_interior_y=bounds[0][1],figure_y=-4.69)
 if name=='SM_Souvenir_Glass':
  cy=(bounds[0][1]+bounds[1][1])/2
  check('Glass lies streetward of unique A02',cy<-4.69 and abs(cy+4.76)<.00005,glass_world_three_x=8025+cy,figure_world_three_x=8020.31)
materials=json.loads((ROOT/'materials.json').read_text())
paths=[ROOT/v for row in materials.values() for v in row.get('textures',{}).values()]
check('PBR files and linear material colors',len(paths)==26 and all(p.is_file() and p.read_bytes()[:8]==b'\x89PNG\r\n\x1a\n' for p in paths) and all(len(v['base_color_linear'])==4 for v in materials.values()),texture_dependencies=len(paths))
sizes={str(p.relative_to(ROOT)):list(struct.unpack_from('>II',p.read_bytes(),16)) for p in paths}
expected_sizes={'textures/souvenir_print_atlas_v3.png':[1254,1254],'textures/souvenir_labels_v3.png':[2048,1024]}
check('PBR and print atlas source resolutions',all(s==expected_sizes.get(name,[2048,2048]) for name,s in sizes.items()),sizes=sizes)
check('Runtime glass is translucent',materials['M_Souvenir_ClearGlass']['alpha']==.10)
b=(ROOT/'Saucer_Crash_1947.glb').read_bytes(); n=struct.unpack_from('<I',b,12)[0]; d=json.loads(b[20:20+n])
gcounts={node['name']:sum(d['accessors'][p['indices']]['count']//3 for p in d['meshes'][node['mesh']]['primitives']) for node in d['nodes'] if 'mesh' in node}
check('GLB exact piece and triangle counts',gcounts=={n:v['triangles'] for n,v in contract['parts'].items()},counts=gcounts)
check('Original facade and proxy anchors',contract['window_center_local_m']==[-1.7,-4.5,1.45] and contract['window_opening_m']==[2.2,1.7] and contract['door_center_local_m']==[2.8,-4.5,1.1] and contract['world_three_pivot_m']==[8025,4.952807474136353,2064])
proof=json.loads((ROOT/'mesh_cleanup.json').read_text())
check('Nondegenerate explicit triangulation preserves authored surfaces',all(v['visible_surface_identical'] and v['remaining_zero_area']==0 and v['material_slots_identical'] for v in proof.values()))
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Saucer_Crash_1947.blend'))
scene_names=[o.name for o in bpy.data.collections['EDITABLE_SOURCE_enable_to_edit'].objects]
features=['LargeHandPaintedFacadeName','RoughSawnPorchRafter','WeatheredCanopyUnderslat','FlakedLimeAtBase','RunoffAndCornerScar','CrumpledFoilMemento','WovenDesertBanner','CanvasSouvenirShirt','RackPostcard','UnevenTravelGuideStack','MineralSampleTin','FoldedDesertTeaTowel']
counts={prefix:sum(n.startswith('SRC_'+prefix) for n in scene_names) for prefix in features}
check('Authored concept details retained in editable source',all(counts.values()),counts=counts)
sign=bpy.data.objects.get('SRC_LargeHandPaintedFacadeName')
check('Large painted facade name fits original parapet',sign is not None and abs(sign.dimensions.x-10.65)<.001 and sign.location.y<-4.5 and sign.location.z>=2.95,width_m=sign.dimensions.x if sign else 0)
check('Source textures resolve after reopening',all(Path(bpy.path.abspath(i.filepath)).is_file() for i in bpy.data.images if i.source=='FILE'))
round_objects=[o for o in bpy.data.collections['EDITABLE_SOURCE_enable_to_edit'].objects if o.name.startswith(('SRC_MixedShelfMug_Cup','SRC_WindowMug_Cup','SRC_LooseShelfToyUFO_Disc','SRC_WindowToySaucer_Disc','SRC_DesertKeepsakeBottle'))]
check('Round merchandise has smooth shading and dense silhouette',len(round_objects)>20 and all(all(p.use_smooth for p in o.data.polygons) and len(o.data.vertices)>=300 for o in round_objects),objects=len(round_objects))
stock_prefixes=['PackedGiftBox','StandingTravelGuide','DesertKeepsakeBottle','ShelfPrintedPrice','DetailedNewMexicoRoadMap']
stock_counts={prefix:sum(n.startswith('SRC_'+prefix) for n in scene_names) for prefix in stock_prefixes}
check('Stock depth, printed map and price labels authored',all(stock_counts.values()) and stock_counts['PackedGiftBox']>=35 and stock_counts['StandingTravelGuide']>=40,counts=stock_counts)
check('Generated print original and prompt retained', (ROOT/'source_art/print_atlas_v3.prompt.txt').is_file() and (ROOT/'source_art/souvenir_print_atlas_v3.original.png').read_bytes()==(ROOT/'textures/souvenir_print_atlas_v3.png').read_bytes())
cup=bpy.data.objects.get('SRC_WindowMug_Cup'); table=bpy.data.objects.get('SRC_WindowDisplayTable')
cup_base=min((cup.matrix_basis@v.co).z for v in cup.data.vertices) if cup else None
table_top=max((table.matrix_basis@v.co).z for v in table.data.vertices) if table else None
check('Window mug rests on display table instead of floating',cup_base is not None and table_top is not None and abs(cup_base-table_top)<.00001,cup_base_z_m=cup_base,table_top_z_m=table_top)
if '--baseline' in sys.argv:
 def protected_geometry():
  prefixes=('ClosedDoor','DoorJamb','DoorHeader','DoorUpperGlass','DoorRaisedMoulding','DoorBrassPull','DoorFadedHours','DoorStoop','DisplayFrame','DisplayStoneSill','DisplayInnerSill','DisplayGlazing','FrontStuccoPier','WindowSillWall','WindowLintel','DoorLintel')
  rows=[]
  for o in sorted(bpy.data.collections['EDITABLE_SOURCE_enable_to_edit'].objects,key=lambda v:v.name):
   if o.type=='MESH' and o.name.startswith(tuple('SRC_'+p for p in prefixes)):
    rows.append((o.name,[list(r) for r in o.matrix_basis],[list(v.co) for v in o.data.vertices],[list(p.vertices) for p in o.data.polygons]))
  return len(rows),hashlib.sha256(json.dumps(rows,separators=(',',':')).encode()).hexdigest()
 actual=protected_geometry()
 baseline=Path(sys.argv[sys.argv.index('--baseline')+1]); bpy.ops.wm.open_mainfile(filepath=str(baseline))
 old=protected_geometry()
 check('Door, facade openings, frames and glass unchanged from approved source',actual==old,source_objects=actual[0],sha256=actual[1],baseline_sha256=old[1])
report={'tool':'Blender '+bpy.app.version_string,'status':'PASS' if all(v['result']=='PASS' for v in checks) else 'FAIL','passed':sum(v['result']=='PASS' for v in checks),'total':len(checks),'checks':checks,'runtime_note':'Source and FBX/GLB checks only. Parent owns Unreal import, A02/photo regression, closed door, terrain and day/dusk/night review.'}
(ROOT/'verification.json').write_text(json.dumps(report,indent=2)); print('SOUVENIR_VERIFY',json.dumps(report))
if report['status']!='PASS': raise RuntimeError('Souvenir verification failed')
