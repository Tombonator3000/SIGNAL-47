"""R6 saved-Blend preservation and actual glTF alpha contract proof."""
import bpy,json,hashlib,sys
from pathlib import Path
root=Path(__file__).resolve().parent;sys.path.insert(0,str(root))
from verify_glb import verify
rows={};reference={}
for ob in bpy.data.objects:
    if ob.type!='MESH' or not ob.name.startswith('SM_Caliche_'):continue
    me=ob.data;colors=me.color_attributes['Color'];reference[ob.name]={}
    for vertex,color in zip(me.vertices,colors.data):
        reference[ob.name].setdefault(tuple(round(x,6) for x in vertex.co),set()).add(color.color[3])
    faces=[]
    for face in me.polygons:
        if face.material_index>=13:continue
        values=[]
        for li in face.loop_indices:
            loop=me.loops[li]
            values.append((tuple(me.vertices[loop.vertex_index].co),tuple(me.uv_layers[0].data[li].uv),tuple(colors.data[loop.vertex_index].color)))
        faces.append((me.materials[face.material_index].name,values))
    rows[ob.name]=hashlib.sha256(json.dumps(faces,separators=(',',':')).encode()).hexdigest()
path=root/'r6_geometry_preservation.json';report=json.loads(path.read_text())
assert report['before_original_material_faces']==rows,'Existing mesh/UV/RGBA material faces changed'
report.update(after_original_material_faces=rows,status='PASS',existing_road_soil_fades_and_plants_rocks_unchanged=True)
for name in ['M_CS_CompactedUse','M_CS_ConcreteUse']:
    m=bpy.data.materials[name];bs=m.node_tree.nodes.get('Principled BSDF')
    alpha=bs.inputs['Alpha'].links[0].from_node
    assert alpha.type=='MATH' and alpha.operation=='MULTIPLY',name
    assert {x.from_node.type for socket in alpha.inputs for x in socket.links}=={'TEX_IMAGE','VERTEX_COLOR'},name
    assert bs.inputs['Specular IOR Level'].default_value==0,name
report['Blender_atlas_times_boundary_alpha']=True
contract=json.loads((root/'model_contract.json').read_text())
previous=json.loads(Path('/home/tom/scratch-bm/Signal47-caliche-concept/ground-before-r6/model_contract.json').read_text())
for name in ['SM_Caliche_RoadWear','SM_Caliche_Rocks','SM_Caliche_DryPlants']:
    assert previous['parts'][name]==contract['parts'][name],name
report['unchanged_other_three_part_contracts']=True
path.write_text(json.dumps(report,indent=2)+'\n')
glb=verify(root,reference)
(root/'glb_verification.json').write_text(json.dumps(glb,indent=2)+'\n')
print('R6_SOURCE_PRESERVATION_PROBE_PASS',len(rows))
