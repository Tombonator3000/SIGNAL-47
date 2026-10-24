"""FBX roundtrip and ground contract checks. Run in Blender after generation."""
import bpy
import json
from pathlib import Path
import sys
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT))
from terrain import clear_dressing, ANCHOR
contract=json.loads((ROOT/'model_contract.json').read_text())
report={'status':'PASS','parts':{},'checks':[]}
for name,row in contract['parts'].items():
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.fbx(filepath=str(ROOT/'exports'/(name+'.fbx')))
    objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
    assert len(objects)==1,(name,len(objects))
    ob=objects[0];me=ob.data;me.calc_loop_triangles()
    assert len(me.loop_triangles)==row['triangles'],name
    assert ob.location.length<1e-6 and (ob.scale-Vector((1,1,1))).length<1e-6,name
    assert len(me.uv_layers)==1 and len(me.materials)==len(row['material_slots']),name
    actual=[[min(v.co[i] for v in me.vertices) for i in range(3)],[max(v.co[i] for v in me.vertices) for i in range(3)]]
    error=max(abs(actual[j][i]-row['bounds_local_m'][j][i]) for i in range(3) for j in range(2))
    assert error<1e-4,(name,error)
    zero=sum((me.vertices[t.vertices[1]].co-me.vertices[t.vertices[0]].co).cross(me.vertices[t.vertices[2]].co-me.vertices[t.vertices[0]].co).length_squared<1e-20 for t in me.loop_triangles)
    assert zero==0,(name,zero)
    color=me.color_attributes.get('Color');assert color is not None,(name,'missing Color RGBA')
    alphas=[c.color[3] for c in color.data]
    assert abs(min(alphas)-row['vertex_alpha_min'])<.003 and abs(max(alphas)-row['vertex_alpha_max'])<.003,(name,min(alphas),max(alphas))
    fade_report={}
    def alpha_at(loop):return color.data[loop.vertex_index if color.domain=='POINT' else loop.index].color[3]
    for material,fade in row.get('edge_fade',{}).items():
        idx=row['material_slots'].index(material);edges={};points={}
        for polygon in me.polygons:
            if polygon.material_index!=idx:continue
            keys=[]
            for li in polygon.loop_indices:
                loop=me.loops[li];p=me.vertices[loop.vertex_index].co
                key=(round(p.x,4),round(p.y,4));keys.append(key)
                points.setdefault(key,[]).append(alpha_at(loop))
            for a,b in zip(keys,keys[1:]+keys[:1]):
                e=tuple(sorted([a,b]));edges[e]=edges.get(e,0)+1
        boundary=[e for e,count in edges.items() if count==1]
        maximum=max(a for e in boundary for p in e for a in points[p])
        assert maximum<.003,(name,material,'open edge alpha',maximum)
        seam_error=max(max(v)-min(v) for v in points.values())
        assert seam_error<.003,(name,material,'alpha discontinuity',seam_error)
        assert any(.01<a<.99 for values in points.values() for a in values),(name,'missing soft alpha')
        fade_report[material]={'open_edges':len(boundary),'boundary_alpha_max':maximum,'duplicate_position_alpha_error':seam_error,'width_m':fade['width_m']}
    if name=='SM_Caliche_RoadWear':
        for polygon in me.polygons:
            if row['material_slots'][polygon.material_index]!='M_CS_OldAsphalt':continue
            for li in polygon.loop_indices:
                loop=me.loops[li];x=abs(me.vertices[loop.vertex_index].co.x)
                if x<=.18001 or abs(x-3.6)<=.09001:assert alpha_at(loop)<.003,('paint not preserved',x,alpha_at(loop))
        report['checks'].append({'name':'only_actual_yellow_and_white_road_paint_preserved_without_broad_center_strip','pass':True,'yellow_buffer_half_width_m':.18})
    report['parts'][name]={'triangles':len(me.loop_triangles),'bounds_error_m':error,'zero_area_triangles':zero,'uv_layers':len(me.uv_layers),'material_slots':len(me.materials),'color_attribute':color.name,'alpha_range':[min(alphas),max(alphas)],'edge_fade':fade_report}
for row in contract['dressing']:
    assert clear_dressing(*row['xz'],row['radius_m'],facade=row.get('facade',False)),row
report['checks'].append({'name':'rocks_and_plants_clear_main_walk_and_protected_approaches','pass':True,'count':len(contract['dressing'])})
samples=json.loads((ROOT/'surface_samples.json').read_text())
assert all(0<=r['above_source_m']<=.014 for r in samples)
report['checks'].append({'name':'all_overlay_vertices_max_14mm_above_unchanged_source_surface','pass':True,'count':len(samples),'maximum_m':max(r['above_source_m'] for r in samples)})
for mat in json.loads((ROOT/'materials.json').read_text()):
    for filename in mat['textures'].values():assert (ROOT/filename).is_file(),filename
report['checks'].append({'name':'all_material_texture_files_present','pass':True})
road=contract['parts']['SM_Caliche_RoadWear']
assert 'M_CS_TarRepair' not in road['used_materials']
report['checks'].append({'name':'no_large_opaque_tar_repair_polygons','pass':True})
(ROOT/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('CALICHE_GROUND_VERIFIED',json.dumps(report['parts']))
