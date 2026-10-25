"""Read-only Blender/FBX/GLB roundtrip audit. Run only by root in its reserved slot.
Does not save imported files or change authoring source. Writes verification.json only.
"""
from pathlib import Path
import json,math,struct,sys,hashlib
import bpy
from mathutils import Vector
R=Path(__file__).resolve().parent
TOL=2e-5

def canonical(name):
 import re
 return re.sub(r'\.\d{3}$','',name)

def bounds(vertices):return [[fn(v[k] for v in vertices) for k in range(3)] for fn in [min,max]]
def delta(a,b):return max(abs(a[i][j]-b[i][j]) for i in range(2) for j in range(3))
def clear():
 for ob in list(bpy.data.objects):bpy.data.objects.remove(ob,do_unlink=True)
 for m in list(bpy.data.materials):
  if m.users==0:bpy.data.materials.remove(m)

def inspect_meshes(objs):
 pts=[];tri=0;uvcount=0;slots=set();finite=True;normal_ok=True
 for ob in objs:
  mesh=ob.data;mesh.calc_loop_triangles();tri+=len(mesh.loop_triangles)
  pts.extend(ob.matrix_world@v.co for v in mesh.vertices)
  used={p.material_index for p in mesh.polygons}
  slots.update(canonical(mesh.materials[i].name) for i in used if i<len(mesh.materials) and mesh.materials[i])
  finite&=all(all(math.isfinite(c) for c in v.co) for v in mesh.vertices)
  normal_ok&=all(math.isfinite(p.normal.length) and p.normal.length>.5 for p in mesh.polygons)
  if not mesh.uv_layers:finite=False
  else:
   uvcount+=len(mesh.uv_layers.active.data)
   finite&=all(all(math.isfinite(c) for c in uv.uv) for uv in mesh.uv_layers.active.data)
 return {'triangles':tri,'bounds_local_m':bounds(pts),'used_materials':sorted(slots),'uv_loop_count':uvcount,'finite_coordinates_uv':bool(finite),'valid_normals':bool(normal_ok),'objects':len(objs)}

def inspect_glb(path):
 raw=path.read_bytes();length,kind=struct.unpack_from('<II',raw,12);doc=json.loads(raw[20:20+length]);offset=20+length;bl,bt=struct.unpack_from('<II',raw,offset);binary=memoryview(raw)[offset+8:offset+8+bl]
 def acc(index):
  a=doc['accessors'][index];bv=doc['bufferViews'][a['bufferView']];num={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];fmt='<'+{5121:'B',5123:'H',5125:'I',5126:'f'}[a['componentType']]*num;stride=bv.get('byteStride',struct.calcsize(fmt));base=bv.get('byteOffset',0)+a.get('byteOffset',0)
  return [struct.unpack_from(fmt,binary,base+j*stride) for j in range(a['count'])]
 pts=[];tris=0;uvs=0;slots=set();finite=True;normal_ok=True
 for mesh in doc['meshes']:
  for p in mesh['primitives']:
   if p.get('mode',4)!=4:raise AssertionError((str(path),'non-triangle primitive'))
   a=p['attributes'];pos=acc(a['POSITION']);pts.extend(Vector((v[0],-v[2],v[1])) for v in pos)
   tris+=len(acc(p['indices']))//3 if 'indices' in p else len(pos)//3
   uv=acc(a['TEXCOORD_0']);uvs+=len(uv);finite&=all(math.isfinite(c) for row in pos+uv for c in row)
   normals=acc(a['NORMAL']);normal_ok&=all(.5<sum(c*c for c in row)<1.5 for row in normals)
   slots.add(canonical(doc['materials'][p['material']]['name']))
 images=doc.get('images',[])
 return {'triangles':tris,'bounds_local_m':bounds(pts),'used_materials':sorted(slots),'uv_vertex_count':uvs,'finite_coordinates_uv':bool(finite),'valid_normals':bool(normal_ok),'embedded_images':sum('bufferView' in i for i in images),'external_image_uris':[i.get('uri') for i in images if 'uri' in i]}

def main():
 contract=json.loads((R/'model_contract.json').read_text());checks=[];failures=[]
 bpy.ops.wm.open_mainfile(filepath=str(R/'STATION01_Field_Instruments.blend'))
 images=[i for i in bpy.data.images if i.source=='FILE']
 portable={'file_images':len(images),'all_packed':all(i.packed_file or i.packed_files for i in images),'all_relative':all(i.filepath.startswith('//') for i in images),'metric':bpy.context.scene.unit_settings.system=='METRIC','unit_scale':bpy.context.scene.unit_settings.scale_length}
 if not portable['all_packed'] or not portable['all_relative'] or not portable['metric'] or portable['unit_scale']!=1:failures.append({'portable_source':portable})
 for name,row in contract['parts'].items():
  ob=bpy.data.objects.get(name);src=bpy.data.objects.get('SRC_'+name)
  ok=ob is not None and src is not None and len(ob.vertex_groups)>0
  if ok:
   measured=bounds([v.co for v in ob.data.vertices]);ok=delta(measured,row['bounds_local_m'])<TOL and all(abs(v-1)<1e-6 for v in ob.scale)
  if not ok:failures.append({'source_part':name,'reason':'missing editable source, component groups, unit scale or local bounds mismatch'})
 for lod in range(3):
  for name,expected in contract['lods'][str(lod)].items():
   suffix='' if lod==0 else '_LOD'+str(lod)
   for format in ['glb','fbx']:
    path=R/'exports'/(name+suffix+'.'+format)
    if format=='fbx':
     clear();bpy.ops.import_scene.fbx(filepath=str(path),use_custom_normals=True);measured=inspect_meshes([o for o in bpy.context.scene.objects if o.type=='MESH'])
    else:measured=inspect_glb(path)
    measured.update(part=name,lod=lod,format=format,bounds_max_error_m=delta(measured['bounds_local_m'],expected['bounds_local_m']))
    measured['pass']=measured['triangles']==expected['triangles'] and measured['bounds_max_error_m']<TOL and measured['used_materials']==sorted(expected['materials']) and measured['finite_coordinates_uv'] and measured['valid_normals']
    if format=='glb':measured['pass']&=not measured['external_image_uris']
    checks.append(measured)
    if not measured['pass']:failures.append(measured)
 result={'status':'PASS' if not failures else 'FAIL','scope':'Read-only portable Blender source plus 39 FBX imports and 39 GLB accessor audits. No Unreal or packaged runtime validation.','portable_source':portable,'checks':checks,'failures':failures,'source_sha256':hashlib.sha256((R/'STATION01_Field_Instruments.blend').read_bytes()).hexdigest(),'contract_sha256':hashlib.sha256((R/'model_contract.json').read_bytes()).hexdigest()}
 (R/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print('FIELD_INSTRUMENTS_SOURCE_VERIFY',result['status'],len(checks),'exports',len(failures),'failures')
 if failures:raise AssertionError(json.dumps(failures[:3]))
if __name__=='__main__':main()
