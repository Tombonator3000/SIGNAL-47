"""Independent Blender reimport of actual archive exports. No source mutation.

Writes only verification.json. Root schedules this after export in its guard slot.
"""
import bpy,json,math,struct
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parent
contract=json.loads((R/'model_contract.json').read_text());results=[]
def check(name,ok,**facts):
 results.append({'name':name,'pass':bool(ok),**facts})
 if not ok:
  (R/'verification.json').write_text(json.dumps({'status':'FAIL','checks':results},indent=2)+'\n');raise AssertionError(name,facts)
(R/'verification.json').write_text(json.dumps({'status':'RUNNING'},indent=2)+'\n')
for lod in [0,1,2]:
 for name,row in contract['lods'][str(lod)].items():
  suffix=''if lod==0 else'_LOD'+str(lod)
  bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
  for mat in list(bpy.data.materials):bpy.data.materials.remove(mat)
  bpy.ops.import_scene.fbx(filepath=str(R/'exports'/(name+suffix+'.fbx')),use_custom_normals=True)
  objects=[o for o in bpy.context.scene.objects if o.type=='MESH'];check(name+suffix+' one FBX mesh',len(objects)==1)
  ob=objects[0];me=ob.data;me.calc_loop_triangles();points=[ob.matrix_world@v.co for v in me.vertices]
  bounds=[[fn(p[k]for p in points)for k in range(3)]for fn in [min,max]];err=max(abs(a-b)for aa,bb in zip(bounds,row['bounds_local_m'])for a,b in zip(aa,bb));slots=[m.name for m in me.materials]
  check(name+suffix+' FBX pivot/metres/triangles/materials',err<.0001 and ob.location.length<1e-5 and max(abs(s-1)for s in ob.scale)<1e-5 and len(me.loop_triangles)==row['triangles'] and slots==row['materials'] and sorted({slots[p.material_index]for p in me.polygons})==row['used_materials'],max_bounds_error_m=err,triangles=len(me.loop_triangles),materials=slots)
  check(name+suffix+' valid face UV and normals',len(me.uv_layers)==1 and all(math.isfinite(v)for uv in me.uv_layers[0].data for v in uv.uv) and all(p.normal.length>.99 for p in me.polygons) and min(t.area for t in me.loop_triangles)>=5e-9)
  raw=(R/'exports'/(name+suffix+'.glb')).read_bytes();check(name+suffix+' GLB binary header',raw[:4]==b'glTF' and struct.unpack_from('<I',raw,4)[0]==2)
  n=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+n]);binary=raw[28+n:];triangles=0;points=[]
  for mesh in doc['meshes']:
   for pr in mesh['primitives']:
    triangles+=doc['accessors'][pr['indices']]['count']//3;a=doc['accessors'][pr['attributes']['POSITION']];view=doc['bufferViews'][a['bufferView']];offset=view.get('byteOffset',0)+a.get('byteOffset',0);stride=view.get('byteStride',12)
    for i in range(a['count']):
     x,y,z=struct.unpack_from('<fff',binary,offset+i*stride);points.append(Vector((x,-z,y)))
    check(name+suffix+' GLB primitive has UV','TEXCOORD_0'in pr['attributes'])
  bounds=[[fn(p[k]for p in points)for k in range(3)]for fn in [min,max]];err=max(abs(a-b)for aa,bb in zip(bounds,row['bounds_local_m'])for a,b in zip(aa,bb))
  check(name+suffix+' GLB matches FBX contract',triangles==row['triangles'] and err<.0001 and sorted(m['name']for m in doc['materials'])==row['used_materials'],triangles=triangles,max_bounds_error_m=err)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False);bpy.ops.import_scene.fbx(filepath=str(R/'exports/axis_probe.fbx'));ob=next(o for o in bpy.context.scene.objects if o.type=='MESH');ps=[ob.matrix_world@v.co for v in ob.data.vertices]
center=[(min(p[k]for p in ps)+max(p[k]for p in ps))*.5 for k in range(3)];size=[max(p[k]for p in ps)-min(p[k]for p in ps)for k in range(3)]
check('asymmetric FBX probe',max(abs(a-b)for a,b in zip(center,[2,4,6]))<1e-5 and max(abs(a-b)for a,b in zip(size,[.2,.4,.6]))<1e-5,center_m=center,size_m=size)
bpy.ops.wm.open_mainfile(filepath=str(R/'SARO_Archive.blend'));objects={o.name:o for o in bpy.context.scene.objects if o.name in contract['parts']};check('eleven saved modular meshes',len(objects)==11)
for name,row in contract['parts'].items():
 at=row['site_world_three_m'];expected=Vector((at[0],-at[2],at[1]));ob=objects[name]
 check(name+' saved assembly single offset',(ob.location-expected).length<1e-5 and len(ob.vertex_groups)>0)
source=bpy.data.collections.get('EDITABLE_ORIGINAL_ARCHIVE');check('editable source assembly preserved',source is not None and source.hide_viewport and source.hide_render and len(source.objects)==11)
packed=[]
for im in bpy.data.images:
 if im.source!='FILE':continue
 disk=Path(bpy.path.abspath(im.filepath));check(im.name+' exact packed original bytes',im.packed_file is not None and disk.is_file() and im.packed_file.data==disk.read_bytes());packed.append(im.name)
check('eighteen original textures packed',len(packed)==18,count=len(packed))
check('native dynamic split preserved',contract['parts']['SM_ArchiveProps_FireDrawer']['parent_handle']=='annex.arc.drawer' and contract['parts']['SM_ArchiveProps_Reel047Box']['parent_handle']=='annex.arc.reel' and contract['parts']['SM_ArchiveProps_CallRecorder']['preserve_geo_ids']==[400] and contract['parts']['SM_ArchiveProps_Switchboard']['preserve_geo_ids']==[391,392])
report={'status':'PASS','scope':'Actual33FBX/33GLB, asymmetric probe, eleven saved source parts, component groups and eighteen packed original textures. No native/runtime/visual acceptance.','checks':results,'check_count':len(results),'packed_textures':packed,'source_revision':contract['source_revision'],'native_visual_status':'UNVERIFIED'}
(R/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print('ARCHIVEPROPS_VERIFY_PASS',len(results))
