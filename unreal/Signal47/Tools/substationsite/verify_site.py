"""Small actual-export validator:21FBX chunks,7GLBs, exact lines and packedPBR.
No Unreal, render, driving suite or native visual acceptance.
"""
import bpy,json,math,struct
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parent;contract=json.loads((R/'model_contract.json').read_text());results=[]
def check(name,ok,**facts):
 results.append({'name':name,'pass':bool(ok),**facts})
 if not ok:
  (R/'verification.json').write_text(json.dumps({'status':'FAIL','checks':results},indent=2)+'\n');raise AssertionError(name,facts)
(R/'verification.json').write_text(json.dumps({'status':'RUNNING'},indent=2)+'\n')
for lod in [0,1,2]:
 for name,row in contract['lods'][str(lod)].items():
  bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
  for mat in list(bpy.data.materials):bpy.data.materials.remove(mat)
  suffix='' if lod==0 else '_LOD'+str(lod);bpy.ops.import_scene.fbx(filepath=str(R/'exports'/(name+suffix+'.fbx')),use_custom_normals=True);objects=[o for o in bpy.context.scene.objects if o.type=='MESH'];check(name+suffix+' single chunk',len(objects)==1)
  ob=objects[0];me=ob.data;me.calc_loop_triangles();ps=[ob.matrix_world@v.co for v in me.vertices];bounds=[[fn(p[k] for p in ps) for k in range(3)] for fn in [min,max]];error=max(abs(x-y) for a,b in zip(bounds,row['bounds_local_m']) for x,y in zip(a,b));slots=[m.name for m in me.materials];minimum=min(t.area for t in me.loop_triangles)
  check(name+suffix+' actual FBX metres/pivot/triangles/materials',error<.0001 and ob.location.length<1e-5 and max(abs(s-1) for s in ob.scale)<1e-5 and len(me.loop_triangles)==row['triangles'] and slots==row['materials'] and sorted({slots[p.material_index] for p in me.polygons})==row['used_materials'] and minimum>=5e-9,max_bounds_error_m=error,triangles=len(me.loop_triangles),minimum_area_m2=minimum)
  check(name+suffix+' UV and normals',len(me.uv_layers)==1 and all(math.isfinite(x) for v in me.uv_layers[0].data for x in v.uv) and all(p.normal.length>.99 for p in me.polygons))
  minuv=math.inf
  for t in me.loop_triangles:
   a,b,c=[me.uv_layers[0].data[i].uv for i in t.loops];minuv=min(minuv,abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))*.5)
  check(name+suffix+' noncollapsed texture triangles',minuv>1e-12,minimum_uv_area=minuv)
# Actual source-coordinate asymmetry, before the independent nativeprobe.
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False);bpy.ops.import_scene.fbx(filepath=str(R/'exports/axis_probe.fbx'));ob=next(o for o in bpy.context.scene.objects if o.type=='MESH');ps=[ob.matrix_world@v.co for v in ob.data.vertices];center=[(min(p[k] for p in ps)+max(p[k] for p in ps))*.5 for k in range(3)];size=[max(p[k] for p in ps)-min(p[k] for p in ps) for k in range(3)];check('actual asymmetric axisprobe',max(abs(a-b) for a,b in zip(center,[2,4,6]))<1e-5 and max(abs(a-b) for a,b in zip(size,[.2,.4,.6]))<1e-5,center_m=center,size_m=size)
linepoints=[]
for name,row in contract['parts'].items():
 raw=(R/'exports'/(name+'.glb')).read_bytes();n=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+n]);binary=raw[28+n:];triangles=0;points=[]
 for mesh in doc['meshes']:
  for p in mesh['primitives']:
   triangles+=doc['accessors'][p['indices']]['count']//3;a=doc['accessors'][p['attributes']['POSITION']];view=doc['bufferViews'][a['bufferView']];offset=view.get('byteOffset',0)+a.get('byteOffset',0);stride=view.get('byteStride',12);actual=[]
   for i in range(a['count']):x,y,z=struct.unpack_from('<fff',binary,offset+i*stride);actual.append(Vector((x,-z,y)))
   points+=actual
   if name=='SM_SubstationSite_Lines' and doc['materials'][p['material']]['name']=='M_SubstationSite_DarkSteel':linepoints+=actual
   check(name+' GLB primitive UV','TEXCOORD_0' in p['attributes'])
 bounds=[[fn(p[k] for p in points) for k in range(3)] for fn in [min,max]];error=max(abs(x-y) for a,b in zip(bounds,row['bounds_local_m']) for x,y in zip(a,b));check(name+' actual GLB matches FBX contract',triangles==row['triangles'] and error<.0001 and sorted(m['name'] for m in doc['materials'])==row['used_materials'],triangles=triangles,max_bounds_error_m=error)
# LOD0 GLB conductor ring centroids independently prove actual start, sagged
# midpoint and finish. Geometry uses even16/8/4 subdivisions at all threeLODs.
linepoints=[Vector(p) for p in set(tuple(p) for p in linepoints)]
for i,span in enumerate(contract['line_spans']):
 for key in ['start','midpoint','end']:
  target=Vector(span[key]);ring=[p for p in linepoints if abs((p-target).length-.014)<3e-5];check('span%s %s conductor exists'%(i,key),len(ring)>=6,ring_vertices=len(ring))
  center=sum(ring,Vector())/len(ring);error=(center-target).length;check('span%s %s actual ring centre'%(i,key),error<3e-5,centre_error_m=error)
 sag=(span['start'][2]+span['end'][2])*.5-span['midpoint'][2];check('span%s exact sag'%i,abs(sag-span['sag_m'])<1e-12,sag_m=sag)
check('nine physically terminated phases',len(contract['line_spans'])==9 and contract['line_spans'][0]['start']==[-1.04,4.65,7.85] and contract['pole_burial_m']==.35 and contract['guy_ground_anchor_site_m']==[0,90,-.342236688])
bpy.ops.wm.open_mainfile(filepath=str(R/'Substation_Site.blend'));objects={o.name:o for o in bpy.context.scene.objects if o.name in contract['parts']};check('seven modular saved meshes',len(objects)==7)
for name,row in contract['parts'].items():check(name+' single offset in saved assembly',(objects[name].location-Vector(row['site_local_offset_m'])).length<1e-5)
for name,ob in objects.items():
 if not name.startswith('SM_SubstationSite_Pole_'):continue
 me=ob.data;porcelain=[p for p in me.polygons if me.materials[p.material_index].name=='M_SubstationSite_PorcelainBrown'];flat=[p for p in porcelain if not p.use_smooth];smooth=[p for p in porcelain if p.use_smooth]
 check(name+' real shared rounded ceramic',len({v for p in porcelain for v in p.vertices})<sum(len(p.vertices) for p in porcelain)*.65 and len(flat)==3*2*(20-2) and len(smooth)>100 and all(abs(p.normal.z)>.99999 for p in flat) and sum((me.corner_normals[i].vector-p.normal).length>1e-4 for p in smooth for i in p.loop_indices)>300)
source=bpy.data.collections.get('EDITABLE_ORIGINAL_SITE');check('editable source groups preserved',source is not None and source.hide_render and source.hide_viewport and len(source.objects)==7 and all(len(o.vertex_groups)>0 for o in objects.values()))
packed=[]
for im in bpy.data.images:
 if im.source!='FILE':continue
 disk=Path(bpy.path.abspath(im.filepath));check(im.name+' exact original packed pixels',im.packed_file is not None and im.packed_file.data==disk.read_bytes());packed.append(im.name)
check('original and reused PBR packed',len(packed)==21,count=len(packed));check('LOD0 bounded budget',contract['total_LOD0_triangles']<=35000,total_triangles=contract['total_LOD0_triangles'])
report={'status':'PASS','checks':results,'check_count':len(results),'source_revision':contract['source_revision'],'lod_totals':{k:sum(r['triangles'] for r in rows.values()) for k,rows in contract['lods'].items()},'packed_textures':packed,'native_visual_status':'UNVERIFIED','scope':'Actual21LOD FBXs,7binaryGLBs, conductor endpoint/midpoint ring centres, sag, asymmetricprobe, savedBlend/UV/smooth ceramics and packedPBR; no native launch.'};(R/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print('SUBSTATION_SITE_VERIFY_PASS',len(results))
