"""Fresh FBX/GLB and editable Blend proof. No native acceptance or rendering."""
import bpy,json,math,struct,hashlib
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parent;contract=json.loads((R/'model_contract.json').read_text());materials=json.loads((R/'materials.json').read_text());byname={m['name']:m for m in materials};checks=[]
def check(name,ok,**extra):
 checks.append({'name':name,'pass':bool(ok),**extra})
 if not ok:
  (R/'verification.json').write_text(json.dumps({'status':'FAIL','checks':checks},indent=2)+'\n');raise AssertionError(name,extra)
(R/'verification.json').write_text(json.dumps({'status':'RUNNING'},indent=2)+'\n')
for lod in [0,1,2]:
 for name,expected in contract['lods'][str(lod)].items():
  bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
  for m in list(bpy.data.materials):bpy.data.materials.remove(m)
  suffix='' if lod==0 else '_LOD'+str(lod)
  bpy.ops.import_scene.fbx(filepath=str(R/'exports'/(name+suffix+'.fbx')),use_custom_normals=True)
  obs=[o for o in bpy.context.scene.objects if o.type=='MESH'];check(name+suffix+' one mesh',len(obs)==1)
  ob=obs[0];me=ob.data;me.calc_loop_triangles();points=[ob.matrix_world@v.co for v in me.vertices];bounds=[[fn(p[k] for p in points) for k in range(3)] for fn in [min,max]];error=max(abs(x-y) for a,b in zip(bounds,expected['bounds_local_m']) for x,y in zip(a,b))
  check(name+suffix+' exact triangles',len(me.loop_triangles)==expected['triangles'] and all(len(p.vertices)==3 for p in me.polygons),actual=len(me.loop_triangles),expected=expected['triangles'])
  check(name+suffix+' meter bounds and neutral pivot',error<.00008 and ob.location.length<1e-5 and max(abs(s-1) for s in ob.scale)<1e-5,max_bounds_error_m=error,pivot_m=list(ob.location),scale=list(ob.scale))
  slots=[m.name for m in me.materials];used=sorted({slots[p.material_index] for p in me.polygons});check(name+suffix+' ordered used materials',slots==expected['materials'] and used==expected['used_materials'],slots=slots,used=used)
  minimum=min(t.area for t in me.loop_triangles);check(name+suffix+' UE stable area and unit normals',minimum>=5e-9 and all(p.normal.length>.99 and all(math.isfinite(x) for x in p.normal) for p in me.polygons),minimum_triangle_area_m2=minimum)
  check(name+suffix+' actual UV layer',len(me.uv_layers)==1 and all(math.isfinite(x) for v in me.uv_layers[0].data for x in v.uv))
  minimum_uv=math.inf
  for t in me.loop_triangles:
   a,b,c=[me.uv_layers[0].data[i].uv for i in t.loops];minimum_uv=min(minimum_uv,abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))*.5)
  check(name+suffix+' noncollapsed UVs',minimum_uv>1e-12,minimum_uv_area=minimum_uv)
# Source asymmetry remains measurable independently of native integration.
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False);bpy.ops.import_scene.fbx(filepath=str(R/'exports/axis_probe.fbx'));ob=next(o for o in bpy.context.scene.objects if o.type=='MESH');ps=[ob.matrix_world@v.co for v in ob.data.vertices];center=[(min(p[k] for p in ps)+max(p[k] for p in ps))*.5 for k in range(3)];dims=[max(p[k] for p in ps)-min(p[k] for p in ps) for k in range(3)]
check('asymmetric FBX source probe',max(abs(x-y) for x,y in zip(center,[2,4,6]))<1e-5 and max(abs(x-y) for x,y in zip(dims,[.2,.4,.6]))<1e-5,center_m=center,dimensions_m=dims)
for name,expected in contract['parts'].items():
 raw=(R/'exports'/(name+'.glb')).read_bytes();size=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+size]);binary=raw[28+size:];triangles=0;points=[]
 for mesh in doc['meshes']:
  for primitive in mesh['primitives']:
   check(name+' GLB UV primitive','TEXCOORD_0' in primitive['attributes']);triangles+=doc['accessors'][primitive['indices']]['count']//3;a=doc['accessors'][primitive['attributes']['POSITION']];view=doc['bufferViews'][a['bufferView']];start=view.get('byteOffset',0)+a.get('byteOffset',0);stride=view.get('byteStride',12)
   for i in range(a['count']):x,y,z=struct.unpack_from('<fff',binary,start+i*stride);points.append((x,-z,y))
 bounds=[[fn(p[k] for p in points) for k in range(3)] for fn in [min,max]];error=max(abs(x-y) for a,b in zip(bounds,expected['bounds_local_m']) for x,y in zip(a,b));check(name+' actual GLB geometry',triangles==expected['triangles'] and error<.00008,triangles=triangles,max_bounds_error_m=error)
 check(name+' actual GLB material set',sorted(m['name'] for m in doc['materials'])==expected['used_materials'])
 for m in doc['materials']:
  if byname[m['name']].get('textures'):check(name+' '+m['name']+' GLB PBR maps','baseColorTexture' in m['pbrMetallicRoughness'] and 'metallicRoughnessTexture' in m['pbrMetallicRoughness'] and 'normalTexture' in m)
bpy.ops.wm.open_mainfile(filepath=str(R/'Caliche_Junction_Substation.blend'))
objects={o.name:o for o in bpy.context.scene.objects if o.name in contract['parts']};check('ten separate runtime modules',len(objects)==10)
for name,row in contract['parts'].items():check(name+' saved site assembly offset',(objects[name].location-Vector(row['site_local_offset_m'])).length<1e-6)
transformer=objects['SM_Substation_Transformer'];me=transformer.data;ceramic=[p for p in me.polygons if me.materials[p.material_index].name=='M_Substation_PorcelainBrown'];loops=sum(len(p.vertices) for p in ceramic);vertices={i for p in ceramic for i in p.vertices}
# Seven porcelain lathes intentionally close with flat end caps: threeHV
# and threeLV at24sides plus one12side breather. Two(n-2) cap triangles each.
# Their284 cap triangles must remain flat and vertical-axis facing; all13,176
# non-cap body triangles must be smooth on genuinely shared vertices.
flat_caps=[p for p in ceramic if not p.use_smooth]
body=[p for p in ceramic if p.use_smooth]
expected_caps=6*2*(24-2)+2*(12-2)
check('porcelain genuinely shared topology',len(vertices)<loops*.65,shared_vertices=len(vertices),corners=loops)
check('porcelain intended smooth bodies and flat end caps',len(flat_caps)==expected_caps and len(body)==len(ceramic)-expected_caps and all(abs(p.normal.z)>.99999 for p in flat_caps) and all(abs(p.normal.z)<.99999 for p in body),smooth_body_triangles=len(body),flat_cap_triangles=len(flat_caps),expected_flat_caps=expected_caps)
normal_diff=sum(1 for p in ceramic for i in p.loop_indices if (me.corner_normals[i].vector-p.normal).length>1e-4);check('porcelain actual rounded corner normals',normal_diff>500,nonflat_corner_normals=normal_diff)
check('editable named component groups retained',len(transformer.vertex_groups)>30 and all(o.vertex_groups for o in objects.values()))
source=bpy.data.collections.get('EDITABLE_ORIGINAL_COMPONENTS');check('original editable assembly preserved',source is not None and source.hide_render and source.hide_viewport and len(source.objects)==10)
packed=[]
for im in bpy.data.images:
 if im.source!='FILE':continue
 disk=Path(bpy.path.abspath(im.filepath));check(im.name+' current packed image',im.packed_file is not None and im.packed_file.data==disk.read_bytes());packed.append({'name':im.name,'sha256':hashlib.sha256(disk.read_bytes()).hexdigest(),'dimensions':list(im.size)})
check('fifteen original PBR images packed',len(packed)==15,count=len(packed))
check('separate controllable amber lens',len(objects['SM_Substation_StatusLamp'].data.materials)==1 and objects['SM_Substation_StatusLamp'].data.materials[0].name=='M_Substation_LampAmber')
check('locked world and ground contract',contract['site_world_three_m']==[7890,-1.10,536.20] and contract['world_yaw_degrees']==180 and contract['floor_surfaces']['control_apron']['z_at_y_ends']==[-.25,0])
report={'status':'PASS','source_revision':contract['source_revision'],'checks':checks,'check_count':len(checks),'total_LOD0_triangles':contract['total_LOD0_triangles'],'lod_totals':{k:sum(v['triangles'] for v in rows.values()) for k,rows in contract['lods'].items()},'packed_textures':packed,'native_visual_status':'UNVERIFIED','scope':'Fresh actual FBX for all30LOD chunks and probe, actual binaryGLB geometry/PBR, saved original Blend and smooth ceramic normals. No Unreal or render.'};(R/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print('SUBSTATION_SOURCE_VERIFY_PASS',report['check_count'])
