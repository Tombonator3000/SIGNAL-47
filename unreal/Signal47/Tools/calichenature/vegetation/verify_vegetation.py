"""Real FBX/GLB roundtrip checks for all 36 authored meshes. Never renders."""
import bpy,json,math,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
contract=json.loads((ROOT/'model_contract.json').read_text());checks=[]
import os,subprocess
bundled=Path('/home/tom/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3')
texture_python=os.environ.get('S47_VEGETATION_PYTHON',str(bundled) if bundled.is_file() else 'python3')
subprocess.run([texture_python,str(ROOT/'verify_mask_textures.py')],check=True)
mask_report=json.loads((ROOT/'alpha_texture_verification.json').read_text())
def check(name,ok,**evidence):checks.append({'name':name,'result':'PASS' if ok else 'FAIL',**evidence})
for main,row in contract['parts'].items():
 check(main+' three monotonically reduced authored levels',len(row['lods'])==3 and row['lods'][0]['triangles']>row['lods'][1]['triangles']>row['lods'][2]['triangles'])
 for lod in row['lods']:
  bpy.ops.wm.read_factory_settings(use_empty=True)
  bpy.ops.import_scene.fbx(filepath=str(ROOT/lod['fbx']),use_custom_normals=True)
  meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];check(lod['name']+' one imported mesh',len(meshes)==1)
  if len(meshes)!=1:continue
  o=meshes[0];me=o.data;me.calc_loop_triangles();bounds=[[min(v.co[i] for v in me.vertices) for i in range(3)],[max(v.co[i] for v in me.vertices) for i in range(3)]]
  err=max(abs(bounds[j][i]-lod['bounds_local_m'][j][i]) for j in range(2) for i in range(3))
  check(lod['name']+' neutral metre pivot and physical bounds',o.location.length<1e-6 and max(abs(x-1) for x in o.scale)<1e-6 and err<.00005 and abs(bounds[0][2])<1e-6,bounds_m=bounds,max_error_m=err)
  check(lod['name']+' exact triangle count and UE area survival',len(me.loop_triangles)==lod['triangles'] and min(t.area for t in me.loop_triangles)>=5e-9,triangles=len(me.loop_triangles),minimum_triangle_area_m2=min(t.area for t in me.loop_triangles))
  check(lod['name']+' expected material order max two', [m.name for m in me.materials]==lod['materials'] and len(me.materials)<=2,materials=[m.name for m in me.materials])
  check(lod['name']+' finite normals and nonempty UVs',len(me.uv_layers)==1 and all(math.isfinite(q) for v in me.uv_layers.active.data for q in v.uv) and all(p.normal.length>.99 for p in me.polygons))
  raw=(ROOT/lod['glb']).read_bytes();n=struct.unpack_from('<I',raw,12)[0];g=json.loads(raw[20:20+n]);tris=sum(g['accessors'][p['indices']]['count']//3 for m in g['meshes'] for p in m['primitives'])
  check(lod['name']+' portable GLB exact triangle count',tris==lod['triangles'],triangles=tris)
  check(lod['name']+' GLB uses masked microfoliage or opaque stone',lod['name'] in mask_report['glb'] and mask_report['status']=='PASS')
materials=json.loads((ROOT/'materials.json').read_text())
check('Three shared materials with binary foliage mask and no translucent plant',set(materials)=={'M_CN_Foliage','M_CN_Bark','M_CN_Stone'} and materials['M_CN_Foliage']['alpha_mode']=='masked' and materials['M_CN_Foliage']['opacity_from_base_color'] is True and materials['M_CN_Foliage']['alpha_cutoff']==.333 and all(r['alpha']==1 for r in materials.values()) and all(materials[n]['alpha_mode']=='opaque' for n in ['M_CN_Bark','M_CN_Stone']))
check('Actual source and embedded RGBA retain perforations and opaque solid tiles',mask_report['status']=='PASS' and len(mask_report['glb'])==36,proof='alpha_texture_verification.json')
paths={str(ROOT/p) for row in materials.values() for p in row['textures'].values()}
check('Portable PBR texture files exist',all(Path(p).is_file() and Path(p).read_bytes()[:8]==b'\x89PNG\r\n\x1a\n' for p in paths),files=sorted(paths))
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'caliche_vegetation.blend'))
check('Editable original source components preserved',bpy.data.collections.get('EDITABLE_original_components') is not None and len(bpy.data.collections['EDITABLE_original_components'].objects)>100,count=len(bpy.data.collections['EDITABLE_original_components'].objects))
foliage=bpy.data.materials['M_CN_Foliage'];clip=[n for n in foliage.node_tree.nodes if n.type=='MATH' and n.operation=='GREATER_THAN'];alpha=foliage.node_tree.nodes['Principled BSDF'].inputs['Alpha']
check('Saved source uses glTF-compatible binary cutoff shader',len(clip)==1 and abs(clip[0].inputs[1].default_value-.333)<1e-6 and alpha.is_linked and alpha.links[0].from_node==clip[0] and foliage.use_backface_culling is False)
check('All 36 named source levels preserved',all(lod['name'] in bpy.data.objects for row in contract['parts'].values() for lod in row['lods']))
report={'source_revision':contract['source_revision'],'tool':'Blender '+bpy.app.version_string,'status':'PASS' if all(c['result']=='PASS' for c in checks) else 'FAIL','passed':sum(c['result']=='PASS' for c in checks),'total':len(checks),'checks':checks,'scope':'Actual source and FBX/GLB roundtrip. Native instancing, screen-size LOD transitions and botanical visual quality require actual game review.'}
(ROOT/'verification.json').write_text(json.dumps(report,indent=2));print('VEGETATION_VERIFY',json.dumps(report));assert report['status']=='PASS'

if report['status']=='PASS':
 contract['validation_status']='SOURCE_AND_36_FBX_36_GLB_ROUNDTRIP_PASS'
 contract['visual_status']='UNVERIFIED: actual completed botanical preview requires human/root review'
 (ROOT/'model_contract.json').write_text(json.dumps(contract,indent=2))
