"""Explicit source-side removal of microscopic bevel/text slivers before FBX import."""
import bmesh,math

MIN_AREA_M2=5e-9
MIN_EDGE_M=1e-6

def clean_geometry(ob):
    bm=bmesh.new();bm.from_mesh(ob.data)
    before=len(bm.faces);removed={}
    bad=[f for f in bm.faces if f.calc_area()<MIN_AREA_M2 or min(e.calc_length()for e in f.edges)<MIN_EDGE_M]
    for f in bad:
        name=ob.data.materials[f.material_index].name;removed[name]=removed.get(name,0)+1
    if bad:
        bmesh.ops.delete(bm,geom=bad,context='FACES')
        loose=[v for v in bm.verts if not v.link_faces]
        if loose:bmesh.ops.delete(bm,geom=loose,context='VERTS')
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    minimum_area=min(f.calc_area()for f in bm.faces);minimum_edge=min(e.calc_length()for f in bm.faces for e in f.edges)
    after=len(bm.faces);bm.to_mesh(ob.data);bm.free();ob.data.update()
    assert minimum_area>=MIN_AREA_M2 and minimum_edge>=MIN_EDGE_M,(ob.name,minimum_area,minimum_edge)
    return {'object':ob.name,'triangles_before':before,'triangles_after':after,'removed_by_material':removed,'minimum_area_m2':minimum_area,'minimum_edge_m':minimum_edge}
