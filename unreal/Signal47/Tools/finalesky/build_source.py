"""Guarded Blender entry point. New output directory only; never edit an existing source package."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import sys
import bpy

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from geometry import build


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def dump(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True)+'\n')


def digest(mesh):
    mesh.calc_loop_triangles()
    return hashlib.sha256(json.dumps(dict(vertices=[list(v.co) for v in mesh.vertices],
        triangles=[list(t.vertices) for t in mesh.loop_triangles],
        materials=[p.material_index for p in mesh.polygons],
        uv=[list(d.uv) for d in mesh.uv_layers.active.data],
        normals=[list(n.vector) for n in mesh.corner_normals]), separators=(',',':')).encode()).hexdigest()


def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args(sys.argv[sys.argv.index('--')+1:])
    out=a.output.resolve(); assert not out.exists(), 'fresh, separate output only'
    contract_path=HERE.parents[2]/'ArtSource/FinaleSky/source-contract.json'
    contract=json.loads(contract_path.read_text())
    out.mkdir(parents=True); (out/'exports').mkdir()
    for ob in list(bpy.data.objects): bpy.data.objects.remove(ob,do_unlink=True)
    bpy.context.scene.unit_settings.system='METRIC'; bpy.context.scene.unit_settings.scale_length=1
    archive=bpy.data.collections.new('EDITABLE_ORIGINAL_SOLIDS'); bpy.context.scene.collection.children.link(archive)
    archive.hide_viewport=archive.hide_render=True
    mats={}
    for role,row in contract['materials'].items():
        m=bpy.data.materials.new('M_S47_Final'+role); m.use_nodes=True; m.use_fake_user=True
        p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=row['color']
        p.inputs['Roughness'].default_value=row['roughness'];p.inputs['Metallic'].default_value=row['metallic']
        p.inputs['Emission Color'].default_value=row['color'];p.inputs['Emission Strength'].default_value=row['glow'];mats[m.name]=m
    lods=[]
    for lod in range(3):
        solids=build(lod); vertices=[]; faces=[]; uv=[]; material=[]
        for s in solids:
            start=len(vertices); vertices.extend(s.vertices);faces.extend(tuple(start+i for i in f)for f in s.triangles)
            uv.extend(s.uv);material.extend([s.material]*len(s.triangles))
            if lod==0:
                data=bpy.data.meshes.new('SRC_'+s.name);data.from_pydata(s.vertices,[],s.triangles);data.materials.append(mats[s.material]);data.update()
                source_uv=data.uv_layers.new(name='UVMap')
                for poly,coords in zip(data.polygons,s.uv):
                    poly.use_smooth=True
                    for li,tc in zip(poly.loop_indices,coords):source_uv.data[li].uv=tc
                orig=bpy.data.objects.new('SRC_'+s.name,data);archive.objects.link(orig);orig['signed_volume_m3']=s.volume();orig['authorship']='Original SIGNAL / 47'
        name=contract['mesh']+(''if lod==0 else '_LOD'+str(lod)); data=bpy.data.meshes.new(name)
        data.from_pydata(vertices,[],faces); data.update()
        for mat in contract['material_slots']:data.materials.append(mats[mat])
        layer=data.uv_layers.new(name='UVMap')
        for poly,coords,mat in zip(data.polygons,uv,material):
            poly.material_index=contract['material_slots'].index(mat);poly.use_smooth=True
            for li,tc in zip(poly.loop_indices,coords):layer.data[li].uv=tc
        ob=bpy.data.objects.new(name,data);bpy.context.scene.collection.objects.link(ob)
        ob['source_revision']=contract['revision'];ob['radius_m']=1;ob['runtime_radius_m']=680
        bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
        fbx=out/'exports'/f'{name}.fbx'
        bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH'},apply_unit_scale=True,
            apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
        assert fbx.is_file()
        lods.append(dict(name=name,fbx=str(fbx.relative_to(out)),fbx_sha256=sha(fbx),triangles=len(faces),vertices=len(vertices),
            slots=contract['material_slots'],mesh_sha256=digest(data),solids=len(solids),minimum_signed_volume_m3=min(s.volume()for s in solids)))
        ob.hide_render=lod!=0;ob.hide_set(lod!=0)
    source=out/'FinaleSky.blend';bpy.ops.wm.save_as_mainfile(filepath=str(source))
    before={r['name']:r['mesh_sha256']for r in lods}
    bpy.ops.wm.open_mainfile(filepath=str(source))
    actual={name:digest(bpy.data.objects[name].data)for name in before};assert actual==before, 'saved/reopened source payload differs'
    refs=[]
    for row in lods:
        mesh=bpy.data.objects[row['name']].data;mesh.calc_loop_triangles();records=[]
        for t in mesh.loop_triangles:
            ps=[];uv=[]
            # FbxStaticMeshImport.cpp creates the triangle from CornerInstanceIDs unchanged;
            # ConvertPos reflects Y into UE's clockwise front-face convention. Do not reverse it.
            for k in (0,1,2):
                p=mesh.vertices[t.vertices[k]].co;tc=mesh.uv_layers.active.data[t.loops[k]].uv
                ps.append([p.x*100,-p.y*100,p.z*100]);uv.append([tc.x,1-tc.y])
            records.append(dict(p=ps,uv=uv,material=mesh.materials[t.material_index].name))
        refs.append(dict(lod=len(refs),triangles=records))
    reference=out/'reopened-native-reference.json.gz'
    with gzip.GzipFile(filename=str(reference),mode='wb',mtime=0)as f:f.write(json.dumps(dict(schema=1,lods=refs),separators=(',',':')).encode())
    inputs={str(p.relative_to(HERE.parents[3])):sha(p)for p in [Path(__file__),HERE/'geometry.py',HERE/'cloud_field.hlsl',contract_path]}
    dump(out/'manifest.json',dict(schema=1,status='SOURCE_REOPEN_PASS_NATIVE_UNVERIFIED',revision=contract['revision'],contract=contract,
        source_blend=dict(path=source.name,bytes=source.stat().st_size,sha256=sha(source)),lods=lods,
        native_reference=dict(path=reference.name,bytes=reference.stat().st_size,sha256=sha(reference)),source_inputs=inputs,
        actual_source_reopen=True,native=False,runtime=False,visual=False))
    print('S47 FINALE SKY SOURCE REOPEN PASS',flush=True)


if __name__=='__main__':main()
