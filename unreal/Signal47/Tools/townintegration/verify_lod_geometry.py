"""Read saved native MeshDescriptions for every authored LOD; never imports/saves assets.
Root launches the existing guarded editor. S47_TOWN_LOD_REPORT is a fresh JSON path.
Optional S47_GEOMETRY_SOURCE_DIR and S47_GEOMETRY_ASSET_ROOT select another kit.
Uses UE5.8 public reflected StaticMesh/MeshDescription methods, not LOD0 aggregate bounds.
"""
from pathlib import Path
import os,json,math,runpy,traceback,hashlib
import unreal
TOOLS=Path(__file__).resolve().parent.parent
R=Path(os.environ.get('S47_GEOMETRY_SOURCE_DIR',str(TOOLS/'townbuildings'))).resolve()
ASSET_ROOT=os.environ.get('S47_GEOMETRY_ASSET_ROOT','/Game/TownBuildings').rstrip('/')
OUT=Path(os.environ['S47_TOWN_LOD_REPORT'])
C=json.loads((R/'model_contract.json').read_text())
FBX=runpy.run_path(str(TOOLS/'calichenature/verify_unreal.py'))['fbx_nodes']
TOL=.025 # centimetres, .25mm; does not hide an axis/translation mismatch.
report={'status':'RUNNING','source_dir':str(R),'asset_root':ASSET_ROOT,'method':'Saved per-LOD StaticMeshDescription actual vertex positions against actual authored FBX control points, expected UE=(100x,-100y,100z)','tolerance_cm':TOL,'checks':[],'meshes':[],'scope':'Read-only saved source MeshDescriptions; render build settings recorded separately. Runtime visual acceptance remains separate.'}
def write():OUT.parent.mkdir(parents=True,exist_ok=True);OUT.write_text(json.dumps(report,indent=2)+'\n')
def check(label,ok,detail=None):report['checks'].append({'name':label,'pass':bool(ok),'detail':detail});write();return bool(ok)
def child(n,name):return next((c for c in n['children']if c['name']==name),None)
def source(name,lod):
 p=R/'exports'/(name+(''if lod==0 else'_LOD'+str(lod))+'.fbx');roots=FBX(p);obj=next(n for n in roots if n['name']=='Objects');geoms=[n for n in obj['children']if n['name']=='Geometry'and n['properties'][-1]=='Mesh'];assert len(geoms)==1,(p,'one authored FBX geometry expected')
 v=child(geoms[0],'Vertices')['properties'][0];pos=[(v[k]*100,-v[k+1]*100,v[k+2]*100)for k in range(0,len(v),3)];return pos,hashlib.sha256(p.read_bytes()).hexdigest()
def bounds(points):return [[f(p[k]for p in points)for k in range(3)]for f in [min,max]]
def error(a,b):return max(abs(x-y)for u,v in zip(a,b)for x,y in zip(u,v))
def hashcell(p):return tuple(math.floor(x/TOL)for x in p)
def missing(a,b):
 cells={}
 for p in b:cells.setdefault(hashcell(p),[]).append(p)
 fail=[];worst=0.
 for p in a:
  k=hashcell(p);nearest=math.inf
  for dx in [-1,0,1]:
   for dy in [-1,0,1]:
    for dz in [-1,0,1]:
     for q in cells.get((k[0]+dx,k[1]+dy,k[2]+dz),[]):nearest=min(nearest,max(abs(x-y)for x,y in zip(p,q)))
  if nearest>TOL:
   if len(fail)<8:fail.append(p)
  elif nearest>worst:worst=nearest
 return fail,worst
DIRECTIONS=[(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1),(.37,.71,.59),(-.61,.23,.77)]
def samples(points):return [max(points,key=lambda p:sum(a*b for a,b in zip(p,d)))for d in DIRECTIONS]
def native_vertices(desc):
 n=desc.get_vertex_count();out=[];idx=0
 # Imported meshes use compact IDs. Validity check also handles occasional sparse IDs.
 while len(out)<n and idx<n*4+64:
  vid=unreal.VertexID();assert vid.import_text('(IDValue='+str(idx)+')')
  if desc.is_vertex_valid(vid):
   p=desc.get_vertex_position(vid);out.append((p.x,p.y,p.z))
  idx+=1
 assert len(out)==n,('incomplete vertex ID scan',len(out),n)
 return out
try:
 S=(unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem))
 for name,p in C['parts'].items():
  mesh=unreal.load_asset(ASSET_ROOT+'/'+name);assert isinstance(mesh,unreal.StaticMesh),name
  row={'name':name,'lods':[]}
  for lod in range(3):
   label=name+' LOD'+str(lod);d=mesh.get_static_mesh_description(lod)
   if not check(label+' saved MeshDescription present',d is not None):continue
   expected,sha=source(name,lod);actual=native_vertices(d);wanted=C['lods'][str(lod)][name];nativebounds=bounds(actual);expectedbounds=bounds(expected)
   # Raw authored FBX coordinates are independently checked against its accepted roundtrip contract.
   lo,hi=wanted['bounds_local_m'];contractbounds=[[lo[0]*100,-hi[1]*100,lo[2]*100],[hi[0]*100,-lo[1]*100,hi[2]*100]]
   check(label+' authored raw FBX matches roundtrip contract',error(expectedbounds,contractbounds)<TOL,{'fbx_sha256':sha,'bounds_cm':expectedbounds,'contract_cm':contractbounds})
   boundsok=check(label+' actual saved LOD bounds and orientation',error(nativebounds,expectedbounds)<TOL,{'actual_cm':nativebounds,'expected_cm':expectedbounds,'max_error_cm':error(nativebounds,expectedbounds)})
   tris=d.get_triangle_count();check(label+' actual saved LOD triangles',tris==wanted['triangles'],{'actual':tris,'expected':wanted['triangles']})
   bad,dist=missing(actual,expected);reverse,rdist=missing(expected,actual)
   check(label+' complete bidirectional actual vertex orientation',not bad and not reverse,{'unexpected_native_vertices_cm':bad,'missing_authored_vertices_cm':reverse,'max_matched_error_cm':max(dist,rdist)})
   ss=samples(expected);samplebad,_=missing(ss,actual);check(label+' asymmetric extrema samples',not samplebad,{'expected_UE_samples_cm':ss,'missing_cm':samplebad})
   # Gives a concrete diagnosis if a conventional 90/180 axis mismatch is present.
   alternatives={}
   if not boundsok or bad:
    for angle in [90,180,270]:
     co=math.cos(math.radians(angle));si=math.sin(math.radians(angle));rot=[(co*x-si*y,si*x+co*y,z)for x,y,z in expected]
     mb,_=missing(samples(rot),actual);alternatives[str(angle)]={'sample_matches':not mb,'bounds_error_cm':error(bounds(rot),nativebounds)}
   settings=S.get_lod_build_settings(mesh,lod);scale=settings.get_editor_property('build_scale3d');buildscale=[scale.x,scale.y,scale.z]
   check(label+' render build scale stays unit',max(abs(v-1)for v in buildscale)<1e-6,buildscale)
   row['lods'].append({'lod':lod,'vertices':len(actual),'triangles':tris,'bounds_cm':nativebounds,'expected_bounds_cm':expectedbounds,'source_fbx_sha256':sha,'diagnostic_z_rotations':alternatives,'build_scale3d':buildscale})
  report['meshes'].append(row);write()
 report['status']='PASS'if all(c['pass']for c in report['checks'])else'FAIL';write()
 unreal.log('TOWN_SAVED_LOD_GEOMETRY_'+report['status']+' '+str(len(report['checks']))+' checks')
 if report['status']!='PASS':raise AssertionError('Saved per-LOD geometry mismatch; exact bounds/representatives are in '+str(OUT))
except Exception:
 report['status']='FAIL';report['error']=traceback.format_exc();write();raise
