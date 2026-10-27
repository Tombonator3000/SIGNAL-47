#!/usr/bin/env python3
"""Original 1986 diner uniform, constructed cloth surfaces fitted to an exact body.

Run with Blender in the project's sequential job guard:
  blender -b -P build_uniform.py -- /private/waitress_body.fbx /private/output

The public source contains no MetaHuman geometry. The input body/armature, fitting
scene and skinned exports are licensed/private working assets and must stay in the
private output directory, never alongside this script in public Git.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Vector


PALETTE = {
    'M_Uniform_Blue': ((.29, .43, .62, 1), .78),
    'M_Uniform_White': ((.82, .80, .72, 1), .78),
    'M_Apron': ((.87, .85, .77, 1), .83),
    'M_Uniform_Seam': ((.19, .30, .46, 1), .80),
    'M_Uniform_Shoe': ((.025, .022, .021, 1), .43),
    'M_Uniform_Sole': ((.012, .011, .010, 1), .86),
    'M_Uniform_Button': ((.70, .69, .63, 1), .34),
}
PARTS = []
REPORT = {'authoring': 'original constructed garment surfaces; no copied body faces',
          'front_axis': '-Y', 'cloth_simulation': False, 'parts': []}


def material(name):
    result = bpy.data.materials.get(name)
    if result:
        return result
    result = bpy.data.materials.new(name)
    colour, roughness = PALETTE[name]
    result.diffuse_color = colour
    result.use_nodes = True
    bsdf = result.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = colour
    bsdf.inputs['Roughness'].default_value = roughness
    return result


def mesh(name, vertices, faces, mat, thickness=.0015, smooth=True):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    obj = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(material(mat))
    for poly in data.polygons:
        poly.use_smooth = smooth
    # A stable authored UV projection for future fabric maps, independent of body UVs.
    uv = data.uv_layers.new(name='GarmentUV')
    for poly in data.polygons:
        for loop in poly.loop_indices:
            p = data.vertices[data.loops[loop].vertex_index].co
            uv.data[loop].uv = (p.x * 3, p.z * 3)
    if thickness:
        mod = obj.modifiers.new('Cloth edge thickness', 'SOLIDIFY')
        mod.thickness = thickness
        mod.offset = 0
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
        bpy.ops.object.modifier_apply(modifier=mod.name)
        obj.select_set(False)
    PARTS.append(obj)
    REPORT['parts'].append({'name': name, 'material': mat,
                            'vertices': len(obj.data.vertices), 'faces': len(obj.data.polygons)})
    return obj


def rings(name, rows, mat, count=64, thickness=.0015):
    """Rows are (centre x/y/z, half width, front depth, back depth, fold amplitude)."""
    if rows[0][0].z > rows[-1][0].z:
        rows = list(reversed(rows))
    verts = []
    for centre, width, front, back, fold in rows:
        for i in range(count):
            a = i * math.tau / count
            co, si = math.cos(a), math.sin(a)
            # A soft, slightly squared cloth section: no anatomical breast moulding.
            x = math.copysign(abs(co) ** .82, co) * width
            y = math.copysign(abs(si) ** .82, si) * (back if si >= 0 else front)
            f = 1 + fold * math.cos(10 * a)
            verts.append((centre.x + x * f, centre.y + y * f, centre.z))
    faces = []
    for j in range(len(rows) - 1):
        for i in range(count):
            n = (i + 1) % count
            faces.append((j * count + i, j * count + n, (j + 1) * count + n, (j + 1) * count + i))
    return mesh(name, verts, faces, mat, thickness)


def lerp_profile(z, rows, column):
    if z <= rows[0][0]:
        return rows[0][column]
    for a, b in zip(rows, rows[1:]):
        if z <= b[0]:
            t = (z - a[0]) / max(.0001, b[0] - a[0])
            return a[column] * (1 - t) + b[column] * t
    return rows[-1][column]


def ribbon(name, centres, width, mat, lateral=Vector((1, 0, 0))):
    verts = []
    for p in centres:
        verts += [tuple(p - lateral * width / 2), tuple(p + lateral * width / 2)]
    return mesh(name, verts, [(2*i, 2*i+1, 2*i+3, 2*i+2) for i in range(len(centres)-1)], mat)


def make_preview(body, output):
    scene = bpy.context.scene
    # Body is a fitting dummy, not an exported uniform surface.
    m = bpy.data.materials.new('PRIVATE_FittingSkin')
    m.diffuse_color = (.52, .37, .28, 1)
    m.use_nodes = True
    m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = m.diffuse_color
    m.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = .72
    body.data.materials.clear()
    body.data.materials.append(m)
    for f in body.data.polygons:
        f.material_index = 0
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 20
    scene.render.resolution_x, scene.render.resolution_y = 800, 1000
    scene.render.resolution_percentage = 100
    scene.world = bpy.data.worlds.new('Private preview world')
    scene.world.use_nodes = True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value = (.13, .15, .18, 1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value = .45
    for name, loc, energy, size in [('Key',(-2.4,-3.3,3.5),480,3),('Fill',(2.4,-1.8,2.3),240,2.5),('Rim',(1.0,2.0,2.8),300,2)]:
        light = bpy.data.lights.new(name, 'AREA')
        light.energy, light.shape, light.size = energy, 'DISK', size
        obj = bpy.data.objects.new(name, light)
        scene.collection.objects.link(obj)
        obj.location = loc
        obj.rotation_euler = (Vector((0,0,.9))-obj.location).to_track_quat('-Z','Y').to_euler()
    camera = bpy.data.objects.new('PreviewCamera', bpy.data.cameras.new('PreviewCamera'))
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.data.type, camera.data.ortho_scale = 'ORTHO', 1.75
    for name, angle in [('front',0),('three_quarter',32)]:
        a = math.radians(angle)
        camera.location = (math.sin(a)*3.5,-math.cos(a)*3.5,.86)
        camera.rotation_euler = (Vector((0,0,.82))-camera.location).to_track_quat('-Z','Y').to_euler()
        scene.render.filepath = str(output / ('uniform_'+name+'.png'))
        bpy.ops.render.render(write_still=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('body_fbx', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--no-preview', action='store_true')
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    public_root = Path(__file__).resolve().parent
    output = args.output.expanduser().resolve()
    if output == public_root or public_root in output.parents:
        raise SystemExit('Fitting/exports contain a licensed skeleton: choose a PRIVATE output directory')
    output.mkdir(parents=True,exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(args.body_fbx),automatic_bone_orientation=False,ignore_leaf_bones=False)
    arm = next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    body = max((o for o in bpy.context.scene.objects if o.type=='MESH'),key=lambda o:len(o.data.vertices))
    J = lambda n: arm.matrix_world @ arm.data.bones[n].head_local
    pelvis, neck = J('pelvis'), J('neck_01')
    shoulder_z = (J('upperarm_l').z+J('upperarm_r').z)/2
    waist_z = J('spine_01').z
    knee_z = (J('calf_l').z+J('calf_r').z)/2
    if not (.75 < pelvis.z < 1.15 and 1.25 < neck.z < 1.65):
        raise SystemExit('Unexpected FBX scale; do not compensate by guessing')
    points = [body.matrix_world @ v.co for v in body.data.vertices]
    # Measure only scalar extents. No input-body faces are copied into the garment.
    def distance_segment(p,a,b):
        ab=b-a
        t=max(0.0,min(1.0,(p-a).dot(ab)/max(ab.length_squared,1e-8)))
        return (p-(a+ab*t)).length
    arm_segments=[(J('upperarm_'+s),J('lowerarm_'+s)) for s in 'lr']+[(J('lowerarm_'+s),J('hand_'+s)) for s in 'lr']
    trunk_segments=[(pelvis,neck)]+[(J('thigh_'+s),J('calf_'+s)) for s in 'lr']
    trunk_points=[p for p in points if min(distance_segment(p,a,b) for a,b in arm_segments) > min(distance_segment(p,a,b) for a,b in trunk_segments)*.90]
    torso = [p for p in trunk_points if waist_z-.04<p.z<shoulder_z]
    front = min(p.y for p in torso)-.025
    back = max(p.y for p in torso)+.025
    centre_y = (front+back)/2
    front_d = centre_y-front
    back_d = back-centre_y
    def extent_at(z,band=.025):
        section=[p for p in trunk_points if abs(p.z-z)<band]
        if not section:
            section=sorted(trunk_points,key=lambda p:abs(p.z-z))[:100]
        return (max(abs(p.x-pelvis.x) for p in section)+.020,
                centre_y-min(p.y for p in section)+.023,
                max(p.y for p in section)-centre_y+.023)
    waist_w,waist_f,waist_b=extent_at(waist_z)
    chest_w,chest_f,chest_b=extent_at(shoulder_z-.12,.06)
    hip_z=pelvis.z-.10
    hip_points=[p for p in trunk_points if pelvis.z-.21<p.z<pelvis.z+.015]
    hip_w=max(abs(p.x-pelvis.x) for p in hip_points)+.022
    hip_f=centre_y-min(p.y for p in hip_points)+.025
    hip_b=max(p.y for p in hip_points)-centre_y+.025
    width=max(chest_w,.185)
    shoulder_w=max(width*.99,(abs(J('upperarm_l').x-pelvis.x)+abs(J('upperarm_r').x-pelvis.x))/2+.025)
    profile = [
        (waist_z-.045,waist_w,waist_f,waist_b),
        (waist_z+.045,max(waist_w,chest_w*.94),max(waist_f,chest_f*.98),max(waist_b,chest_b)),
        (waist_z+.15,chest_w,chest_f,chest_b),
        (shoulder_z-.035,shoulder_w,max(chest_f*.95,front_d*.93),max(chest_b,back_d*.97)),
        (shoulder_z+.018,shoulder_w*.98,front_d*.84,back_d*.85),
        (neck.z+.004,.065,.070,.067),
    ]
    profile.sort(key=lambda r:r[0])
    top = rings('Loose_blue_shirt',[(Vector((pelvis.x,centre_y,z)),w,f,b,0) for z,w,f,b in profile],'M_Uniform_Blue')
    def front_at(x,z,offset=.005):
        w=lerp_profile(z,profile,1)
        depth=lerp_profile(z,profile,2)
        # Surface matches the softly squared section; a small clearance prevents z fighting.
        fraction=min(.985,abs((x-pelvis.x)/w))
        factor=max(.08,1-fraction**(2/.82))**(.82/2)
        return centre_y-depth*factor-offset
    def P(x,z,offset=.005):return Vector((pelvis.x+x,front_at(pelvis.x+x,z,offset),z))
    sleeve_parts=[]
    for side in 'lr':
        start,end=J('upperarm_'+side),J('lowerarm_'+side)
        axis=(end-start).normalized()
        u=Vector((0,-1,0));u=(u-axis*u.dot(axis)).normalized()
        v=axis.cross(u).normalized()
        length=(end-start).length*.46
        # A small tucked-in cap grows below the shirt shoulder. A full broad ring
        # behind the shoulder would make the pointed sci-fi shoulder seen in V3.
        values=[(-.045,.050),(-.015,.068),(length*.30,.079),(length,.071)]
        for name,vals,mat in [('Short_sleeve_'+side,values,'M_Uniform_Blue'),('White_cuff_'+side,[(length-.012,.074),(length+.009,.073)],'M_Uniform_White')]:
            verts=[];n=48
            for t,radius in vals:
                centre=start+axis*t
                for i in range(n):
                    a=i*math.tau/n;verts.append(tuple(centre+u*(math.cos(a)*radius)+v*(math.sin(a)*radius)))
            faces=[(j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i) for j in range(len(vals)-1) for i in range(n)]
            sleeve_parts.append(mesh(name,verts,faces,mat))
    # A real neck band and SHORT pointed leaves. Triangles follow the shirt
    # surface instead of a large non-planar n-gon that folds into loose shards.
    collar_z=neck.z-.012
    rings('White_neck_collar_band',[(Vector((pelvis.x,centre_y,z)),.068,.073,.070,0) for z in [collar_z,neck.z+.006]],'M_Uniform_White',64,.0012)
    for sign,label in [(-1,'right'),(1,'left')]:
        coords=[P(sign*.025,neck.z-.015,.005),P(sign*.060,neck.z-.009,.006),
                P(sign*.115,shoulder_z+.016,.006),P(sign*.071,shoulder_z-.050,.006),P(sign*.014,shoulder_z+.003,.006)]
        # Straight chords through a convex shirt disappear inside it. Subdivide each
        # leaf triangle in x/z and project EVERY point onto the actual cloth surface.
        verts,faces=[],[]
        divisions=8
        for triangle in [(0,1,2),(0,2,3),(0,3,4)]:
            a,b,c=[coords[k] for k in triangle]
            grid={}
            for row in range(divisions+1):
                for col in range(divisions-row+1):
                    q=a+(b-a)*(row/divisions)+(c-a)*(col/divisions)
                    q.y=front_at(q.x,q.z,.009)
                    grid[row,col]=len(verts);verts.append(tuple(q))
            for row in range(divisions):
                for col in range(divisions-row):
                    faces.append((grid[row,col],grid[row+1,col],grid[row,col+1]))
                    if col<divisions-row-1:
                        faces.append((grid[row+1,col],grid[row+1,col+1],grid[row,col+1]))
        mesh('White_point_collar_'+label,verts,faces,'M_Uniform_White',.0012,True)
    # Modest service-dress skirt. Analytic cloth folds remain stable without cloth simulation.
    hem_z=knee_z-.065
    skirt_profile=[(hem_z,max(hip_w*1.09,.245),max(hip_f*1.08,.16),max(hip_b*1.08,.16)),
                   (hip_z,hip_w,hip_f,hip_b),
                   (waist_z+.020,waist_w*1.015,waist_f*1.015,waist_b*1.015)]
    skirt_profile.sort(key=lambda row:row[0])
    def skirt_surface(x,z,offset=0):
        sw=lerp_profile(z,skirt_profile,1);sf=lerp_profile(z,skirt_profile,2)
        fraction=min(.985,abs(x/sw));factor=max(.08,1-fraction**(2/.82))**(.82/2)
        return centre_y-sf*factor-offset
    skirt_rows=[]
    for i in range(25):
        t=i/24;z=waist_z+.015+(hem_z-waist_z-.015)*t
        skirt_rows.append((Vector((pelvis.x,centre_y,z)),lerp_profile(z,skirt_profile,1),lerp_profile(z,skirt_profile,2),lerp_profile(z,skirt_profile,3),.007*t*t))
    skirt = rings('Blue_A_line_skirt',skirt_rows,'M_Uniform_Blue',96)
    rings('Skirt_hem',[(Vector((pelvis.x,centre_y,hem_z+z)),skirt_profile[0][1],skirt_profile[0][2],skirt_profile[0][3],.007) for z in [0,.008]],'M_Uniform_Seam',96,.0012)
    # The apron is explicitly authored on NEGATIVE Y, unlike the previous normal-selection approach.
    bib_bottom,bib_top=waist_z+.008,shoulder_z-.125
    grid=[];rows,cols=13,18
    for j in range(rows):
        t=j/(rows-1);z=bib_bottom+(bib_top-bib_bottom)*t;half=.136-.018*t
        for i in range(cols):grid.append(tuple(P(-half+2*half*i/(cols-1),z,.012)))
    apron_bib=mesh('White_apron_bib',grid,[(j*cols+i,j*cols+i+1,(j+1)*cols+i+1,(j+1)*cols+i) for j in range(rows-1) for i in range(cols-1)],'M_Apron',.002)
    for sign,label in [(-1,'right'),(1,'left')]:
        x=sign*.114
        # The band passes over the actual sloped shoulder, rather than through
        # the shirt or behind the missing head fitting dummy.
        topz=shoulder_z+.018+(neck.z+.004-shoulder_z-.018)*(shoulder_w*.98-abs(x))/(shoulder_w*.98-.065)
        centres=[P(x,bib_top,.008),P(x,shoulder_z-.012,.008),
                 P(x,shoulder_z+.030,.008),Vector((pelvis.x+x,centre_y,topz+.007)),
                 Vector((pelvis.x+x,centre_y+back_d+.009,shoulder_z-.035)),
                 Vector((pelvis.x+x,centre_y+waist_b+.010,waist_z+.018))]
        ribbon('White_apron_shoulder_strap_'+label,centres,.027,'M_Apron')
    rings('White_apron_waist_band',[(Vector((pelvis.x,centre_y,z)),waist_w*1.035,waist_f*1.035+.007,waist_b*1.035+.007,0) for z in [waist_z-.009,waist_z+.016]],'M_Apron',64,.0012)
    apron=[];rows,cols=20,24
    for j in range(rows):
        t=j/(rows-1);z=waist_z+.003+(hem_z+.075-waist_z)*t
        half=waist_w*.80+(hip_w*.80-waist_w*.80)*min(1,t*2)
        if t>.85:half-=.017*((t-.85)/.15)**2
        for i in range(cols):
            x=-half+2*half*i/(cols-1)
            y=skirt_surface(x,z,.008)
            apron.append((pelvis.x+x,y,z))
    apron_skirt=mesh('White_apron_front_skirt',apron,[(j*cols+i,j*cols+i+1,(j+1)*cols+i+1,(j+1)*cols+i) for j in range(rows-1) for i in range(cols-1)],'M_Apron',.002)
    # Thin pocket panels follow the apron rather than sitting on a flat plane
    # in front of it. A 2 mm lifted top is enough to read as an opening.
    for sign,label in [(-1,'right'),(1,'left')]:
        z=waist_z-.16;x=sign*.088;verts=[];nr,nc=7,8
        for j in range(nr):
            t=j/(nr-1);zz=z-.045+.085*t
            half=.041 if t>.12 else .035+.006*t/.12
            for i in range(nc):
                xx=x-half+2*half*i/(nc-1)
                verts.append((pelvis.x+xx,skirt_surface(xx,zz,.010+.001*t),zz))
        mesh('Apron_pocket_'+label,verts,[(j*nc+i,j*nc+i+1,(j+1)*nc+i+1,(j+1)*nc+i) for j in range(nr-1) for i in range(nc-1)],'M_Apron',.0008)
    # Feet are covered by simple closed service shoes. No bare foot or giant flat prop.
    shoes=[]
    for side in 'lr':
        foot,ball=J('foot_'+side),J('ball_'+side)
        forward=ball-foot;forward.z=0;forward.normalize()
        lateral=Vector((0,0,1)).cross(forward).normalized()
        other=J('foot_'+('r' if side=='l' else 'l'))
        region=[p for p in points if p.z<.15 and (Vector((p.x-foot.x,p.y-foot.y))).length < (Vector((p.x-other.x,p.y-other.y))).length]
        low=min((p.z for p in region),default=0)
        def foot_section(z,band=.025):
            pts=[p for p in region if abs(p.z-z)<band]
            if not pts:pts=sorted(region,key=lambda p:abs(p.z-z))[:30]
            lat=[p.dot(lateral) for p in pts];lon=[p.dot(forward) for p in pts]
            lc=(min(lat)+max(lat))/2;fc=(min(lon)+max(lon))/2
            half=(max(lat)-min(lat))/2+.008;length=(max(lon)-min(lon))/2+.010
            centre=lateral*lc+forward*fc;centre.z=z
            # Fit a rounded rectangle to all measured toe/heel points. A plain
            # ellipse clipped the corners of the toes in V3.
            fit=max([1]+[((abs((p-centre).dot(lateral)/half)**4+abs((p-centre).dot(forward)/length)**4)**.25) for p in pts])
            return centre,half*fit,length*fit
        base_c,base_w,base_len=foot_section(low+.020,.035)
        sole_levels=[(Vector((base_c.x,base_c.y,low-.006)),base_w+.002,base_len+.002),
                     (Vector((base_c.x,base_c.y,low+.012)),base_w+.002,base_len+.002)]
        upper_levels=[(Vector((base_c.x,base_c.y,low+.010)),base_w,base_len)]+[foot_section(low+z,.026) for z in [.035,.060,.085,.110,.135]]
        for name,levels,mat in [('Shoe_upper_'+side,upper_levels,'M_Uniform_Shoe'),('Rubber_sole_'+side,sole_levels,'M_Uniform_Sole')]:
            verts=[];n=48
            for centre,half,length in levels:
                for i in range(n):
                    a=i*math.tau/n;si,co=math.sin(a),math.cos(a)
                    p=centre+lateral*(math.copysign(abs(co)**.5,co)*half)+forward*(math.copysign(abs(si)**.5,si)*length);verts.append(tuple(p))
            faces=[(j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i) for j in range(len(levels)-1) for i in range(n)]
            faces.append(tuple(reversed(range(n))))
            faces=[tuple(reversed(face)) for face in faces]
            obj=mesh(name,verts,faces,mat,0)
            shoes.append((obj,'foot_'+side))
    shoe_objects={obj for obj,bone in shoes}
    # Transfer only deformation weights; these are fitting data, not copied mesh geometry.
    for obj in PARTS:
        if obj in shoe_objects:
            bone=next(b for o,b in shoes if o==obj)
            group=obj.vertex_groups.new(name=bone);group.add(list(range(len(obj.data.vertices))),1,'REPLACE')
        else:
            for g in body.vertex_groups:obj.vertex_groups.new(name=g.name)
            dt=obj.modifiers.new('Fitted deformation weights','DATA_TRANSFER');dt.object=body
            dt.use_vert_data=True;dt.data_types_verts={'VGROUP_WEIGHTS'};dt.vert_mapping='POLYINTERP_NEAREST'
            dt.layers_vgroup_select_src='ALL';dt.layers_vgroup_select_dst='NAME'
            bpy.context.view_layer.objects.active=obj;obj.select_set(True)
            bpy.ops.object.modifier_apply(modifier=dt.name)
            if obj in [skirt,apron_skirt] or obj.name=='Skirt_hem':
                pelvis_group=obj.vertex_groups.get('pelvis') or obj.vertex_groups.new(name='pelvis')
                for v in obj.data.vertices:
                    # A standing A-line has a common waist. Softly reduce leg separation.
                    for g in v.groups:g.weight*=.35
                    pelvis_group.add([v.index],.65,'ADD')
            bpy.ops.object.vertex_group_limit_total(group_select_mode='ALL',limit=4)
            bpy.ops.object.vertex_group_normalize_all(lock_active=False)
            obj.select_set(False)
        mod=obj.modifiers.new('MetaHuman body leader deformation','ARMATURE');mod.object=arm
        matrix=obj.matrix_world.copy();obj.parent=arm;obj.matrix_world=matrix
    bpy.ops.object.select_all(action='DESELECT')
    for obj in PARTS:obj.select_set(True)
    bpy.context.view_layer.objects.active=top
    bpy.ops.object.join()
    uniform=bpy.context.view_layer.objects.active;uniform.name='SKM_Waitress_Uniform'
    for poly in uniform.data.polygons:poly.use_smooth=True
    REPORT['measurements']={'pelvis':list(pelvis),'neck':list(neck),'shoulder_z':shoulder_z,'waist_z':waist_z,'hem_z':hem_z,'half_width':width,'front_y':front,'back_y':back}
    REPORT['uniform']={'vertices':len(uniform.data.vertices),'faces':len(uniform.data.polygons),'materials':[m.name for m in uniform.data.materials]}
    REPORT['input_body_sha256']=hashlib.sha256(args.body_fbx.read_bytes()).hexdigest()
    REPORT['source_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    REPORT['output_is_private']=True
    bpy.ops.wm.save_as_mainfile(filepath=str(output/'waitress_uniform_fitting.blend'))
    if not args.no_preview:make_preview(body,output)
    bpy.ops.object.select_all(action='DESELECT')
    uniform.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=uniform
    bpy.ops.export_scene.fbx(filepath=str(output/'SKM_Waitress_Uniform.fbx'),use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,bake_anim=False,mesh_smooth_type='FACE',use_armature_deform_only=False)
    (output/'uniform_report.json').write_text(json.dumps(REPORT,indent=2)+'\n')
    print('UNIFORM_CONSTRUCTED',json.dumps(REPORT))


if __name__=='__main__':main()
