"""Compare all four saved meshes, contracts/fades and packed PBR deps to R6."""
import bpy,json,hashlib,sys
from pathlib import Path
R=Path(__file__).resolve().parent;sys.path.insert(0,str(R))
from verify_glb import verify
hashes={};reference={}
for ob in bpy.data.objects:
    if ob.type!='MESH' or not ob.name.startswith('SM_Caliche_'):continue
    me=ob.data;colors=me.color_attributes['Color'];rows=[];reference[ob.name]={}
    for vertex,color in zip(me.vertices,colors.data):
        reference[ob.name].setdefault(tuple(round(x,6) for x in vertex.co),set()).add(color.color[3])
    for face in me.polygons:
        values=[]
        for li in face.loop_indices:
            loop=me.loops[li]
            values.append((tuple(me.vertices[loop.vertex_index].co),tuple(me.uv_layers[0].data[li].uv),tuple(colors.data[loop.vertex_index].color)))
        rows.append((me.materials[face.material_index].name,values))
    hashes[ob.name]=hashlib.sha256(json.dumps(rows,separators=(',',':')).encode()).hexdigest()
path=R/'r7_geometry_preservation.json'
if '--before' in sys.argv:
    path.write_text(json.dumps({'status':'BASELINE','before_all_material_faces':hashes},indent=2)+'\n')
else:
    report=json.loads(path.read_text());assert report['before_all_material_faces']==hashes,'R7 geometry,UV,RGBA ormaterialfaces changed'
    before=R/'evidence/r6-before-r7'
    unchanged=['model_contract.json','materials.json','wear_contact_samples.json','surface_samples.json']
    for name in unchanged:assert (before/name).read_bytes()==(R/name).read_bytes(),name
    for name in ['M_CS_CompactedUse','M_CS_ConcreteUse']:
        m=bpy.data.materials[name];bs=m.node_tree.nodes.get('Principled BSDF');alpha=bs.inputs['Alpha'].links[0].from_node
        assert alpha.type=='MATH' and alpha.operation=='MULTIPLY'
        assert {link.from_node.type for socket in alpha.inputs for link in socket.links}=={'TEX_IMAGE','VERTEX_COLOR'}
        assert bs.inputs['Specular IOR Level'].default_value==0
    images=[]
    for image in bpy.data.images:
        if image.source=='FILE':
            assert image.packed_file,image.name
            disk=Path(bpy.path.abspath(image.filepath));assert image.packed_file.data==disk.read_bytes(),image.name
            images.append(image.name)
    glb=verify(R,reference);(R/'glb_verification.json').write_text(json.dumps(glb,indent=2)+'\n')
    report.update(status='PASS',after_all_material_faces=hashes,unchanged_contract_files=unchanged,all_packed_images_match_current_disk=images,atlas_times_vertexalpha=True)
    path.write_text(json.dumps(report,indent=2)+'\n')
print('R7_PRESERVATION_PROBE_PASS',len(hashes),'before' if '--before' in sys.argv else 'after')
