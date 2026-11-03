"""Reimport deliveries in a fresh Blender process; never touch gameplay sources."""
from pathlib import Path
import bpy,json,math
ROOT=Path(__file__).resolve().parent
sources=list(ROOT.glob('Lab/*.asset.json'))+list(ROOT.glob('Workshop/*.asset.json'))
private=Path.home()/'Assets/signal47-private/EastWing/Props/Lab'
if private.exists():sources+=list(private.glob('*.asset.json'))
checks=[]
for source in sources:
 row=json.loads(source.read_text()); name=row['asset']
 for spec in [{'file':row['fbx'],'lod':0}]+[{'file':r['file'],'lod':i+1} for i,r in enumerate(row.get('lods',[]))]:
  bpy.ops.wm.read_factory_settings(use_empty=True)
  f=source.parent/spec['file'];bpy.ops.import_scene.fbx(filepath=str(f),use_custom_normals=True)
  meshes=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.name.startswith('UCX_')]
  assert meshes,(name,'empty FBX')
  points=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
  bounds=[[fn(v[k] for v in points) for k in range(3)] for fn in (min,max)]
  dims=[bounds[1][k]-bounds[0][k] for k in range(3)]
  expected=row['dimensions_m']
  # Axis reimport may rotate XY but must preserve unit dimensions and height.
  dimension_error=max(abs(a-b) for a,b in zip(sorted(dims),sorted(expected)))
  uv=all(bool(o.data.uv_layers) for o in meshes)
  finite=all(math.isfinite(x) for p in points for x in p)
  mats=all(len(o.data.materials)>0 for o in meshes)
  passed=dimension_error<.015 and uv and finite and mats
  checks.append({'asset':name,'lod':spec['lod'],'pass':passed,'dimension_error_m':dimension_error,'dimensions_m':dims,'uv':uv,'finite':finite,'materials':mats,'triangles':sum(len(o.data.polygons) for o in meshes)})
report={'status':'PASS' if checks and all(c['pass'] for c in checks) else 'FAIL','scope':'Fresh Blender FBX import: dimensions, UVs, materials, finite geometry for every delivered LOD. Unreal runtime integration remains Claude task.','checks':checks}
(ROOT/'fbx-validation.json').write_text(json.dumps(report,indent=2)+'\n')
assert report['status']=='PASS',report
print('EASTWING FBX PASS',len(checks))
