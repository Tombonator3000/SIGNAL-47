"""EASTWING original asset authoring helpers; project PhotoLab FBX convention.
Blender metres, Z up, front -Y. Not a runtime integration script.
"""
from pathlib import Path
import bpy, math, json, hashlib
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
TEX=ROOT/'Textures'
CURRENT=None
BEFORE=set()
CATALOG=[]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system='METRIC'
bpy.context.scene.unit_settings.scale_length=1

def material(name,color=(.3,.3,.3,1),rough=.5,metal=0,texture=None):
    m=bpy.data.materials.get(name)
    if m:return m
    m=bpy.data.materials.new(name);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=(*color[:3],1)
    p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
    m.diffuse_color=(*color[:3],1)
    if texture:
        m['eastwing_texture_key']=texture
        for suffix,socket in [('BaseColor','Base Color'),('Roughness','Roughness'),('Metallic','Metallic'),('NormalGL','Normal')]:
            path=TEX/(texture+'_'+suffix+'.png')
            if not path.exists():raise FileNotFoundError(path)
            im=bpy.data.images.load(str(path),check_existing=True)
            im.colorspace_settings.name='sRGB' if suffix=='BaseColor' else 'Non-Color'
            n=m.node_tree.nodes.new('ShaderNodeTexImage');n.image=im
            if suffix=='NormalGL':
                nm=m.node_tree.nodes.new('ShaderNodeNormalMap');m.node_tree.links.new(n.outputs['Color'],nm.inputs['Color']);m.node_tree.links.new(nm.outputs['Normal'],p.inputs['Normal'])
            else:m.node_tree.links.new(n.outputs['Color'],p.inputs[socket])
    return m

def assign(o,name,mat):
    o.name=name
    if mat:o.data.materials.append(mat)
    return o

def box(name,loc,size,mat,bevel=.005):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Manufactured edge radius','BEVEL');mod.width=bevel;mod.segments=3
        mod=o.modifiers.new('Weighted corner normals','WEIGHTED_NORMAL')
    return assign(o,name,mat)

def cyl(name,loc,radius,depth,mat,vertices=48,rotation=(0,0,0)):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=depth,location=loc,rotation=rotation)
    o=assign(bpy.context.object,name,mat)
    mod=o.modifiers.new('Machined rim','BEVEL');mod.width=min(radius*.08,.002);mod.segments=2
    for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
    return o

def sphere(name,loc,scale,mat):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=16,radius=1,location=loc)
    o=bpy.context.object;o.scale=scale;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    for p in o.data.polygons:p.use_smooth=True
    return assign(o,name,mat)

def rod(name,a,b,radius,mat):
    d=Vector(b)-Vector(a);o=cyl(name,(Vector(a)+Vector(b))/2,radius,d.length,mat)
    o.rotation_mode='QUATERNION';o.rotation_quaternion=d.to_track_quat('Z','Y');return o

def text(name,body,loc,size,mat,rotation=(math.pi/2,0,0)):
    bpy.ops.object.text_add(location=loc,rotation=rotation);o=bpy.context.object;o.name=name
    o.data.body=body;o.data.size=size;o.data.align_x='CENTER';o.data.align_y='CENTER';o.data.extrude=.00015;o.data.resolution_u=3
    if mat:o.data.materials.append(mat)
    return o

def start_asset(name):
    global CURRENT,BEFORE
    CURRENT=name;BEFORE={o.as_pointer() for o in bpy.data.objects}

def select(objs):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:o.hide_set(False);o.select_set(True)
    bpy.context.view_layer.objects.active=objs[0]

def finish_asset(name,outdir,description,sources=[],license_id='LicenseRef-ProjectOwned'):
    global CATALOG
    outdir=Path(outdir);outdir.mkdir(parents=True,exist_ok=True)
    originals=[o for o in bpy.context.scene.objects if o.as_pointer() not in BEFORE and o.type in {'MESH','CURVE','FONT'}]
    assert originals, name
    coll=bpy.data.collections.new(name+'_EDITABLE');bpy.context.scene.collection.children.link(coll)
    for o in originals:
        for c in list(o.users_collection):c.objects.unlink(o)
        coll.objects.link(o)
    # A .blend library containing the editable collection, original parts, modifiers and materials.
    blend=outdir/(name+'.blend');bpy.data.libraries.write(str(blend),{coll},fake_user=True,compress=True,path_remap='RELATIVE')
    dg=bpy.context.evaluated_depsgraph_get();copies=[]
    for o in originals:
        me=bpy.data.meshes.new_from_object(o.evaluated_get(dg),depsgraph=dg)
        c=bpy.data.objects.new(o.name+'_EXPORT',me);bpy.context.scene.collection.objects.link(c);c.matrix_world=o.matrix_world.copy();copies.append(c)
        if not me.uv_layers:
            select([c]);bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.cube_project(cube_size=1);bpy.ops.object.mode_set(mode='OBJECT')
    select(copies);bpy.ops.object.convert(target='MESH');bpy.ops.object.join();mesh=bpy.context.object;mesh.name=name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    # Physical planar UV density for shared tileable materials; preserve all imported UVs otherwise.
    specs=json.loads((TEX/'materials.json').read_text())['materials']
    spans={r['key']:r['physical_tile_metres'] for r in specs if r.get('tileable')}
    uv=mesh.data.uv_layers.active.data
    for poly in mesh.data.polygons:
        mat=mesh.data.materials[poly.material_index] if poly.material_index<len(mesh.data.materials) else None
        key=mat.get('eastwing_texture_key') if mat else None
        if key not in spans:continue
        span=spans[key];axis=max(range(3),key=lambda k:abs(poly.normal[k]));axes=[k for k in range(3) if k!=axis]
        for li in poly.loop_indices:
            co=mesh.data.vertices[mesh.data.loops[li].vertex_index].co
            uv[li].uv=(co[axes[0]]/span[0],co[axes[1]]/span[1])
    tri=mesh.modifiers.new('Native triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
    points=[v.co for v in mesh.data.vertices];lo=[min(v[i] for v in points) for i in range(3)];hi=[max(v[i] for v in points) for i in range(3)]
    assert all(math.isfinite(v) for p in points for v in p)
    assert len(mesh.data.uv_layers)>0
    dims=[hi[i]-lo[i] for i in range(3)]
    collision=box('UCX_'+name+'_00',[(lo[i]+hi[i])/2 for i in range(3)],dims,None,0)
    collision.display_type='WIRE';collision.hide_render=True
    def export(path,objects):
        select(objects);bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,mesh_smooth_type='FACE',path_mode='RELATIVE',use_mesh_modifiers=True)
    export(outdir/(name+'.fbx'),[mesh,collision]);bpy.data.objects.remove(collision,do_unlink=True)
    lods=[]
    for i,ratio in [(1,.5),(2,.25)]:
        c=mesh.copy();c.data=mesh.data.copy();bpy.context.scene.collection.objects.link(c);c.name=name+'_LOD'+str(i)
        mod=c.modifiers.new('LOD reduction','DECIMATE');mod.ratio=ratio
        select([c]);bpy.ops.object.modifier_apply(modifier=mod.name)
        path=outdir/(c.name+'.fbx');export(path,[c]);lods.append({'file':path.name,'triangles':len(c.data.polygons)});bpy.data.objects.remove(c,do_unlink=True)
    textures=[]
    material_records=[]
    for m in mesh.data.materials:
        if m and m.use_nodes:
            bs=m.node_tree.nodes.get('Principled BSDF')
            rec={'name':m.name,'texture_key':m.get('eastwing_texture_key'),'maps':{}}
            if bs:
                for socket in ['Base Color','Roughness','Metallic','Transmission Weight','IOR','Emission Color','Emission Strength']:
                    val=bs.inputs[socket].default_value
                    rec[socket]=list(val) if hasattr(val,'__len__') else float(val)
            material_records.append(rec)
            for n in m.node_tree.nodes:
                if n.type=='TEX_IMAGE' and n.image:
                    p=Path(bpy.path.abspath(n.image.filepath))
                    if p.exists():
                        textures.append(str(p));rec['maps'][p.name]=str(p)
    row={'asset':name,'description':description,'fbx':name+'.fbx','editable':blend.name,'units':'metres; FBX unit metadata centimetres conversion','front':'-Y','up':'Z','pivot':[0,0,0],'bounds_m':[lo,hi],'dimensions_m':dims,'triangles':len(mesh.data.polygons),'uv_layers':len(mesh.data.uv_layers),'materials':[m.name for m in mesh.data.materials if m],'material_parameters':material_records,'textures':sorted(set(textures)),'lods':lods,'collision':'UCX bounding box: integrator must disable for movable/interactive detail and replace with fitted collision for open shelving','license':{'id':license_id,'origin':'Original project-authored geometry unless source attribution below states otherwise. No external asset license implied.','sources':sources},'runtime_status':'NOT_IMPORTED; Claude owns integration'}
    for suffix in ['.fbx','.blend']:row[suffix[1:]+'_sha256']=hashlib.sha256((outdir/(name+suffix)).read_bytes()).hexdigest()
    (outdir/(name+'.asset.json')).write_text(json.dumps(row,indent=2)+'\n')
    (outdir/(name+'.license.json')).write_text(json.dumps({'asset':name,**row['license']},indent=2)+'\n')
    for o in originals:o.hide_render=True;o.hide_set(True)
    mesh.hide_render=True;mesh.hide_set(True)
    CATALOG.append((mesh,row,outdir))
    return mesh

def render_catalog(outdir,title):
    """Low-cost contact thumbnails first. No runtime lighting claims."""
    outdir=Path(outdir);outdir.mkdir(parents=True,exist_ok=True)
    sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=12;sc.cycles.use_denoising=True;sc.render.threads_mode='FIXED';sc.render.threads=4
    sc.render.resolution_x=384;sc.render.resolution_y=384;sc.render.resolution_percentage=100
    sc.world=bpy.data.worlds.new('EastWingStudio');sc.world.use_nodes=True;sc.world.node_tree.nodes['Background'].inputs[0].default_value=(.18,.18,.18,1)
    sc.view_settings.view_transform='AgX'
    for ob in sc.objects:ob.hide_render=True
    bpy.ops.object.camera_add();cam=bpy.context.object;cam.hide_render=False;sc.camera=cam;cam.data.type='ORTHO'
    lights=[]
    for loc,power,size in [((3,-4,6),700,5),((-4,-1,3),450,4),((0,4,5),600,3)]:
        bpy.ops.object.light_add(type='AREA',location=loc);l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=size;l.rotation_euler=(Vector((0,0,.5))-l.location).to_track_quat('-Z','Y').to_euler();lights.append(l)
    rows=[]
    for mesh,row,where in CATALOG:
        mesh.hide_render=False;d=max(row['dimensions_m']);center=Vector([(row['bounds_m'][0][i]+row['bounds_m'][1][i])/2 for i in range(3)])
        cam.location=center+Vector((1.7,-2.7,-1.7 if 'Fluorescent' in row['asset'] else 1.7))*d;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=max(.15,d*1.65)
        for l in lights:l.hide_render=False
        dest=where/(row['asset']+'_preview.png');sc.render.filepath=str(dest);bpy.ops.render.render(write_still=True);mesh.hide_render=True;rows.append(str(dest))
    (outdir/'catalog.json').write_text(json.dumps({'title':title,'preview_images':rows,'assets':[r for _,r,_ in CATALOG]},indent=2)+'\n')
