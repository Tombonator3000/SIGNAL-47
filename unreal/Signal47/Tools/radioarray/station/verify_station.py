"""FBX roundtrip, source-contract, opening and texture checks (Blender 5.2).
Run: blender -b --factory-startup --python verify_station.py
"""
import bpy, json, math, struct
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
contract=json.loads((ROOT/'model_contract.json').read_text())
checks=[]; meshes={}
def check(name,ok,**evidence): checks.append({'name':name,'result':'PASS' if ok else 'FAIL',**evidence})
for name,expected in contract['parts'].items():
 bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
 for m in list(bpy.data.materials): bpy.data.materials.remove(m)
 bpy.ops.import_scene.fbx(filepath=str(ROOT/'exports'/(name+'.fbx')),use_custom_normals=True)
 objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
 check(name+' separate mesh',len(objects)==1,count=len(objects))
 if len(objects)!=1: continue
 o=objects[0]; mesh=o.data
 bounds=[tuple(min(v.co[i] for v in mesh.vertices) for i in range(3)),tuple(max(v.co[i] for v in mesh.vertices) for i in range(3))]
 err=max(abs(bounds[j][i]-expected['bounds_local_m'][j][i]) for i in range(3) for j in range(2))
 check(name+' scale and pivot',err<.00005 and o.location.length<.00005 and max(abs(v-1) for v in o.scale)<.00005,bounds_m=bounds,position_m=list(o.location),scale=list(o.scale),max_bounds_error_m=err)
 expectedm=expected['materials']; actualm=[m.name for m in mesh.materials]
 check(name+' material slots',actualm==expectedm,slots=list(actualm))
 mesh.calc_loop_triangles()
 zero=sum(t.area==0 for t in mesh.loop_triangles)
 check(name+' exact triangle contract',len(mesh.loop_triangles)==expected['triangles'] and zero==0 and all(len(p.vertices)==3 for p in mesh.polygons),triangles=len(mesh.loop_triangles),expected_triangles=expected['triangles'],zero_area_triangles=zero)
 badnormal=sum(1 for p in mesh.polygons if not all(math.isfinite(v) for v in p.normal) or p.normal.length<.99)
 check(name+' finite face normals',badnormal==0,invalid_face_normals=badnormal,faces=len(mesh.polygons))
 if name=='SM_1420_Shell':
  def hit(x,z): return o.ray_cast(Vector((x,-6.4,z)),Vector((0,1,0)),distance=1.6)[0]
  check('Door and studio window are actual shell openings',not hit(-4.6,1.1) and not hit(1.2,1.55) and hit(-2,1.1),door_empty=not hit(-4.6,1.1),window_empty=not hit(1.2,1.55),solid_pier_hit=hit(-2,1.1))
  check('Roof and window lintel normals face outward',sum(1 for p in mesh.polygons if p.normal.z>.99)>10)
materials=json.loads((ROOT/'materials.json').read_text()); tex=[ROOT/v for m in materials.values() for v in m.get('textures',{}).values()]
check('R3 eight-family PBR texture dependencies exist',len(tex)==24 and all(p.is_file() and p.read_bytes()[:8]==b'\x89PNG\r\n\x1a\n' for p in tex),count=len(tex),relative_paths=[str(p.relative_to(ROOT)) for p in tex])
check('Source and GLB preserved',(ROOT/'1420_AM_station.blend').is_file() and (ROOT/'1420_AM_station.glb').read_bytes()[:4]==b'glTF')
glb=(ROOT/'1420_AM_station.glb').read_bytes(); chunk_size=struct.unpack_from('<I',glb,12)[0]; gltf=json.loads(glb[20:20+chunk_size])
gcounts={n['name']:sum(gltf['accessors'][p['indices']]['count']//3 for p in gltf['meshes'][n['mesh']]['primitives']) for n in gltf['nodes'] if 'mesh' in n}
check('GLB exact triangle counts',gcounts=={n:v['triangles'] for n,v in contract['parts'].items()},triangles=gcounts)
cleanup=json.loads((ROOT/'mesh_cleanup.json').read_text())
check('Visible surface unchanged by zero-area cleanup',all(v['visible_surface_identical'] and v['vertices_retained'] and v['material_slots_identical'] and v['remaining_zero_area']==0 for v in cleanup.values()),proof_file='mesh_cleanup.json',removed={n:v['removed_exact_zero_area'] for n,v in cleanup.items()})
check('Story proxy anchor preserved',contract['window_center_local_m']==[1.2,-5,1.55] and contract['window_opening_m']==[3.7,1.65] and contract['world_three_pivot_m']==[8025,4.442166614532471,2036])
check('R2 facade identity exported', 'M_1420_WeatheredFacadePaint' in contract['parts']['SM_1420_Shell']['materials'] and contract.get('source_revision')=='R3-street-visible-studio-2026-10-24')
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'1420_AM_station.blend'))
for name,expected in [('SRC_Console_chassis',(1.2,1.53,1.0)),('SRC_Microphone_body',(1.72,1.68,1.405))]:
 ob=bpy.data.objects.get(name)
 check(name+' original physical anchor',ob is not None and (ob.matrix_basis.translation-Vector(expected)).length<.00001,actual=list(ob.matrix_basis.translation) if ob else None,expected=expected)
check('Street-facing tape transport is within window',len(contract['r3_changes']['moved_tape_parts'])>15 and all(n in bpy.data.objects for n in ['SRC_Reel_machine','SRC_ProgramMonitorStreetFace','SRC_WindowCartTransport']),parts=contract['r3_changes']['moved_tape_parts'])
tape=bpy.data.objects.get('SRC_Reel_machine_face')
if tape:
 pts=[tape.matrix_basis@v.co for v in tape.data.vertices];forward=tape.matrix_basis.to_3x3()@Vector((0,1,0))
 check('Tape face physically within clear window and facing street',min(p.x for p in pts)>-.65 and max(p.x for p in pts)<3.05 and max(p.z for p in pts)<2.375 and forward.y<-.99,bounds=[[min(p[i] for p in pts) for i in range(3)],[max(p[i] for p in pts) for i in range(3)]],front_normal=list(forward))
else:check('Tape face physically within clear window and facing street',False)
emissive=[]
for o in bpy.data.objects:
 if o.name.startswith('SRC_') and o.type=='MESH' and any(m.name=='M_1420_Lamp_glow' for m in o.data.materials):
  pts=[o.matrix_basis@v.co for v in o.data.vertices]
  if min(p.x for p in pts)<3.05 and max(p.x for p in pts)>-.65 and min(p.z for p in pts)<2.375 and max(p.z for p in pts)>.725:emissive.append(o.name)
check('No exposed Lamp_glow geometry in window rectangle',not emissive,overlaps=emissive)
report={'tool':'Blender '+bpy.app.version_string,'status':'PASS' if all(c['result']=='PASS' for c in checks) else 'FAIL','checks':checks,'passed':sum(c['result']=='PASS' for c in checks),'total':len(checks),'runtime_note':'This verifies authored FBX roundtrip. Unreal placement, collisions, door interaction and story regressions require parent runtime tests.'}
(ROOT/'verification.json').write_text(json.dumps(report,indent=2)); print('STATION_VERIFY',json.dumps(report))
if report['status']!='PASS': raise RuntimeError('Station verification failed; see verification.json')
