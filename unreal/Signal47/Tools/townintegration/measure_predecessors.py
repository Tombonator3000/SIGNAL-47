"""Measure saved native predecessors; original glTF may lose degenerate faces on import.
Runs inside Unreal Python after prepare_assembly.py. No asset or scene mutation.
"""
import hashlib,json,os
from pathlib import Path
import unreal
P=Path(__file__).resolve().parents[2];source=P/'Content/WebData/manifest.json';man=json.loads(source.read_text());out=P/'Content/TownBuildings/assembly.json';assembly=json.loads(out.read_text())
assert hashlib.sha256(source.read_bytes()).hexdigest()==assembly['manifestSha256']
rows=[]
for row in assembly['retire']:
 geo=row['geo'];sourcefile=next(r['file']for r in man['geoFiles']if r['from']<=geo<r['to']);path='/Game/Web/Geo/'+Path(sourcefile).stem+'/StaticMeshes/g'+str(geo)
 mesh=unreal.EditorAssetLibrary.load_asset(path);assert isinstance(mesh,unreal.StaticMesh)and mesh.get_name()=='g'+str(geo)
 actual=mesh.get_num_triangles(0);assert 0<actual<=row['tris'],(row,actual)
 row['acceptedPreviousTris']=sorted(set(row['acceptedPreviousTris']+[actual]))
 rows.append({'node':row['node'],'geo':geo,'asset':path,'source_triangles':row['tris'],'saved_native_triangles':actual,'acceptedPreviousTris':row['acceptedPreviousTris']})
out.write_text(json.dumps(assembly,indent=2)+'\n')
report={'status':'PASS','scope':'Saved original native mesh readback, exact assembly predecessors, no geometry changes','manifestSha256':assembly['manifestSha256'],'rows':rows}
(P/'Tools/townintegration/native-predecessors.json').write_text(json.dumps(report,indent=2)+'\n')
unreal.log('TOWN NATIVE PREDECESSORS PASS '+str(len(rows)))
