"""Exact Blender FBX roundtrip and shop/A02 compatibility checks, no rendering."""
import bpy, json, struct, math
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
 if name=='SM_Souvenir_Glass':
  cy=(bounds[0][1]+bounds[1][1])/2
  check('Glass lies streetward of unique A02',cy<-4.69 and abs(cy+4.76)<.00005,glass_world_three_x=8025+cy,figure_world_three_x=8020.31)
materials=json.loads((ROOT/'materials.json').read_text())
paths=[ROOT/v for row in materials.values() for v in row.get('textures',{}).values()]
check('PBR files and linear material colors',len(paths)==12 and all(p.is_file() and p.read_bytes()[:8]==b'\x89PNG\r\n\x1a\n' for p in paths) and all(len(v['base_color_linear'])==4 for v in materials.values()),texture_dependencies=len(paths))
check('Runtime glass is translucent',materials['M_Souvenir_ClearGlass']['alpha']==.10)
b=(ROOT/'Saucer_Crash_1947.glb').read_bytes(); n=struct.unpack_from('<I',b,12)[0]; d=json.loads(b[20:20+n])
gcounts={node['name']:sum(d['accessors'][p['indices']]['count']//3 for p in d['meshes'][node['mesh']]['primitives']) for node in d['nodes'] if 'mesh' in node}
check('GLB exact piece and triangle counts',gcounts=={n:v['triangles'] for n,v in contract['parts'].items()},counts=gcounts)
check('Original facade and proxy anchors',contract['window_center_local_m']==[-1.7,-4.5,1.45] and contract['window_opening_m']==[2.2,1.7] and contract['door_center_local_m']==[2.8,-4.5,1.1] and contract['world_three_pivot_m']==[8025,4.952807474136353,2064])
proof=json.loads((ROOT/'mesh_cleanup.json').read_text())
check('Nondegenerate explicit triangulation preserves authored surfaces',all(v['visible_surface_identical'] and v['remaining_zero_area']==0 and v['material_slots_identical'] for v in proof.values()))
report={'tool':'Blender '+bpy.app.version_string,'status':'PASS' if all(v['result']=='PASS' for v in checks) else 'FAIL','passed':sum(v['result']=='PASS' for v in checks),'total':len(checks),'checks':checks,'runtime_note':'Source and FBX/GLB checks only. Parent owns Unreal import, A02/photo regression, closed door, terrain and day/dusk/night review.'}
(ROOT/'verification.json').write_text(json.dumps(report,indent=2)); print('SOUVENIR_VERIFY',json.dumps(report))
if report['status']!='PASS': raise RuntimeError('Souvenir verification failed')
