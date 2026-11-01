"""Original fitted Jeep cabin. Run only in root's granted Blender slot.

Reads the existing private prepared Jeep; writes a separate private derivative.
Original GLB, original.blend, prepared.blend and the C10 cabin are never written.
"""
from pathlib import Path
import argparse
import copy
import json
import math
import random
import shutil
import struct
import sys
import zlib

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import prepare as P

DEFAULT_SOURCE = Path.home() / 'Documents/ChatGPT/Signal 47/modeller/tripo-fleet-20261031/prepared/classic_jeep'
DEFAULT_OUTPUT = DEFAULT_SOURCE.parent.parent / 'interior-20261101'
DEST = '/Game/Licensed/TripoFleet/classic_jeep'
PREFIX = 'SM_TripoJeep_'
EYE = (-.41, 1.70, .65)
HUB = (-.41, 1.29, .08)
WHEEL_AXIS_B = (0., -.827475774, .561501419)
DIALS = {
    'speed': {'part': PREFIX + 'SpeedFace', 'at_three': [-.04, 1.26, -.125], 'radius_m': .095,
              'atlas_center_px': [256, 88], 'atlas_radius_px': 74, 'needle_length_m': .095*70/74*.88,
              'sweep_degrees': [-120, 120]},
    'fuel': {'part': PREFIX + 'FuelFace', 'at_three': [.16, 1.31, -.125], 'radius_m': .036,
             'atlas_center_px': [88, 98], 'atlas_radius_px': 44, 'needle_length_m': .036*40/44*.88,
             'sweep_degrees': [-60, 60]},
    'temp': {'part': PREFIX + 'TempFace', 'at_three': [.28, 1.31, -.125], 'radius_m': .036,
             'atlas_center_px': [424, 98], 'atlas_radius_px': 44, 'needle_length_m': .036*40/44*.88,
             'sweep_degrees': [-60, 60]},
}
RADIO = {'part': PREFIX + 'RadioFace', 'at_three': [.20, 1.095, -.125],
         'size_m': [.30, .05625], 'atlas_crop_px': [0, 160, 512, 256]}
ANCHORS = {'version': 1, 'coordinates': P.COORDINATES, 'driver_eye_three_m': EYE,
           'dial_normal_three': [0, 0, 1], 'needle_axis_three': [0, 0, 1],
           'needle_offset_three_m': [0, 0, .003], 'dials': DIALS, 'radio': RADIO,
           'wheel': {'part': PREFIX + 'Steering', 'hub_three_m': HUB, 'radius_m': .185,
                     'axis_blender': WHEEL_AXIS_B, 'axis_three': [0, WHEEL_AXIS_B[2], -WHEEL_AXIS_B[1]],
                     'mesh_pivot': [0, 0, 0], 'mesh_axis_baked': True},
           'atlas': {'size_px': [512, 256], 'uv': '(pixelX/512, 1-pixelY/256) in Blender preview',
                     'native_export_uv': '(pixelX/512, pixelY/256); live faces only, compensates FBX V conversion plus SharedMid.SetMap(true)',
                     'native_material_override': 'SharedMid(MGauge.Id) / existing live DashRT; SetMap(true)'},
           'parts': {'body': PREFIX + 'InteriorBody', 'interior': PREFIX + 'Interior'},
           'compass_card_three_m': [-.28, 1.530, -.20],
           'compass_yaw_radians': -.151765, 'compass_radius_m': .047,
           'compass_foot_bottom_three_m': [-.28, 1.470, -.20],
           'keys_three_m': [-.30, 1.14, -.08],
           'fallback': 'Replace original Jeep body only when all seven new assets and live-face material override are available.'}


def B(three):
    return (three[0], -three[2], three[1])


def png(path, width, height, pixels):
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    raw = b''.join(b'\0' + bytes(pixels[y * width * 3:(y + 1) * width * 3]) for y in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
                     + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


FONT = {
 '0':['01110','10011','10101','10101','11001','10001','01110'],
 '1':['00100','01100','00100','00100','00100','00100','01110'],
 '2':['01110','10001','00001','00010','00100','01000','11111'],
 '3':['11110','00001','00001','01110','00001','00001','11110'],
 '4':['00010','00110','01010','10010','11111','00010','00010'],
 '5':['11111','10000','10000','11110','00001','00001','11110'],
 '6':['01110','10000','10000','11110','10001','10001','01110'],
 '7':['11111','00001','00010','00100','01000','01000','01000'],
 '8':['01110','10001','10001','01110','10001','10001','01110'],
 '9':['01110','10001','10001','01111','00001','00001','01110'],
 'M':['10001','11011','10101','10101','10001','10001','10001'],
 'P':['11110','10001','10001','11110','10000','10000','10000'],
 'H':['10001','10001','10001','11111','10001','10001','10001'],
 'F':['11111','10000','10000','11110','10000','10000','10000'],
 'U':['10001','10001','10001','10001','10001','10001','01110'],
 'E':['11111','10000','10000','11110','10000','10000','11111'],
 'L':['10000','10000','10000','10000','10000','10000','11111'],
 'T':['11111','00100','00100','00100','00100','00100','00100'],
 'C':['01111','10000','10000','10000','10000','10000','01111'],
 'A':['01110','10001','10001','11111','10001','10001','10001'],
 'R':['11110','10001','10001','11110','10100','10010','10001'],
 '/':['00001','00001','00010','00100','01000','10000','10000'],
 ':':['00000','00100','00100','00000','00100','00100','00000'],
 ' ':['00000']*7,
}


def atlas(path):
    """Small original preview image; runtime replaces it with the real live atlas."""
    width, height = 512, 256
    image = [7, 10, 9] * width * height
    ink = (201, 224, 201)
    def pixel(x, y, color=ink):
        x, y = round(x), round(y)
        if 0 <= x < width and 0 <= y < height:
            at = (y * width + x) * 3
            image[at:at + 3] = color
    def line(x0, y0, x1, y1, color=ink):
        steps = max(1, round(max(abs(x1-x0), abs(y1-y0))*2))
        for i in range(steps+1):
            f = i / steps
            pixel(x0+(x1-x0)*f, y0+(y1-y0)*f, color)
    def text(words, x, y, scale=1):
        x -= (len(words)*6-1)*scale/2
        y -= 3.5*scale
        for c in words:
            for j, row in enumerate(FONT.get(c, FONT[' '])):
                for i, bit in enumerate(row):
                    if bit == '1':
                        for dy in range(scale):
                            for dx in range(scale):pixel(x+i*scale+dx, y+j*scale+dy)
            x += 6*scale
    for kind, cx, cy, radius, start, end in [('MPH',256,88,70,-120,120), ('FUEL',88,98,40,-60,60), ('TEMP',424,98,40,-60,60)]:
        count = 34 if radius > 50 else 8
        for i in range(count+1):
            a = math.radians(start+(end-start)*i/count)
            r0 = radius-(9 if i%2 == 0 else 5)
            line(cx+math.sin(a)*r0, cy-math.cos(a)*r0, cx+math.sin(a)*radius, cy-math.cos(a)*radius)
        labels = [(i/85, str(i)) for i in range(0,81,10)] if radius > 50 else [(0,'E'),(.5,'1/2'),(1,'F')] if kind=='FUEL' else [(0,'C'),(1,'H')]
        for f, label in labels:
            a=math.radians(start+(end-start)*f);r=radius-(21 if radius>50 else 16)
            text(label,cx+math.sin(a)*r,cy-math.cos(a)*r,2 if radius>50 else 1)
        text(kind,cx,cy+(26 if radius>50 else 16),1)
    for y in range(160,256):
        for x in range(512):pixel(x,y,(17,20,18))
    text('AM   FM',178,180)
    for i, number in enumerate(['88','92','96','100','104','108']):text(number,90+i*45,210)
    for x in range(77,333,8):line(x,225,x,231)
    text('18:30',421,208,2)
    png(path,width,height,image)


def srgb(x):
    return 12.92*x if x <= .0031308 else 1.055*x**(1/2.4)-.055


def grain_maps(root, name, color, roughness):
    rng=random.Random(47011+sum(ord(c)for c in name));size=128
    noise=[rng.random() for _ in range(size*size)]
    color_pixels=[];rough_pixels=[];normal_pixels=[]
    for y in range(size):
        for x in range(size):
            n=noise[y*size+x]-.5;wear=1 if (x*31+y*13)%997 < 3 else 0
            color_pixels.extend(round(255*max(0,min(1,srgb(v*(1+n*.075)+wear*.012)))) for v in color)
            r=round(255*max(0,min(1,roughness+n*.08+wear*.05)));rough_pixels.extend([r]*3)
            dx=(noise[y*size+(x+1)%size]-noise[y*size+(x-1)%size])*.028
            dy=(noise[((y+1)%size)*size+x]-noise[((y-1)%size)*size+x])*.028
            normal_pixels.extend([round(127.5-dx*127.5),round(127.5-dy*127.5),255])
    files={}
    for role, pixels in [('base_color',color_pixels),('roughness',rough_pixels),('normal',normal_pixels)]:
        relative='textures/'+name+'_'+role+'.png';png(root/relative,size,size,pixels);files[role]=relative
    return files


def source_trim(face, material_name):
    # Whole source triangles only, below the windscreen and inside the open tub.
    if 'Glass' in material_name:
        return False
    points=[v.co for v in face.verts]
    def box(low, high):return all(all(a <= p[i] <= b for i,(a,b)in enumerate(zip(low,high)))for p in points)
    protected_frame=any(p.y>.18 and p.z>1.445 for p in points)
    wheel=box((-.74,-.38,1.10),(-.14,.28,1.53)) and not protected_frame
    dash=box((-.77,.025,.95),(.77,.30,1.435))
    gear=box((-.17,-.52,.65),(.17,.12,1.17))
    return wheel or dash or gear


def build(source, destination, preview=False):
    import bpy
    import bmesh
    from mathutils import Matrix, Vector
    source=source.resolve();destination=destination.resolve()
    P.require(not destination.is_relative_to(P.REPO) and not destination.is_relative_to(source)
              and not source.is_relative_to(destination), 'Use a separate private output directory')
    input_blend=source/'prepared.blend';input_contract=json.loads((source/'model_contract.json').read_text())
    protected={str(p):P.sha(p)for p in [input_blend,source/'original.blend',Path(input_contract['source_glb'])]}
    model=destination/'classic_jeep';(model/'exports').mkdir(parents=True,exist_ok=True);(model/'textures').mkdir(exist_ok=True)
    bpy.ops.wm.open_mainfile(filepath=str(input_blend))
    bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
    for obj in list(bpy.context.scene.objects):
        if obj.type in {'LIGHT','CAMERA'}:bpy.data.objects.remove(obj,do_unlink=True)
    materials=copy.deepcopy(json.loads((source/'materials.json').read_text()))
    rename={};source_materials={m.name:m for m in bpy.data.materials}
    def copy_source_file(filename):
        original=(source/filename).resolve();target=(model/filename).resolve()
        P.require(original.is_relative_to(source) and target.is_relative_to(model), 'Source texture path escapes private model directory')
        target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(original,target)
    for row in materials:
        original=row['name'];new=original+'_CabTrim';rename[original]=new;row['name']=new
        clone=source_materials[original].copy();clone.name=new
        for role,filename in row.get('textures',{}).items():
            copy_source_file(filename)
        for role in ('packed_metallic_roughness','occlusion'):
            if row.get(role):copy_source_file(row[role])
    parts={};trim_rows=[]
    body_name=PREFIX+'InteriorBody'
    for lod in range(3):
        suffix=''if lod==0 else'_LOD'+str(lod);obj=bpy.data.objects['SM_TripoFleet_ClassicJeep'+suffix]
        obj.name=body_name+suffix
        for i,m in enumerate(list(obj.data.materials)):obj.data.materials[i]=bpy.data.materials[rename[m.name]]
        before=len(obj.data.polygons);bm=bmesh.new();bm.from_mesh(obj.data)
        remove=[f for f in bm.faces if source_trim(f,obj.data.materials[f.material_index].name)]
        P.require(5 <= len(remove) < before*.22,'Source interior selection missed or crosses too much body in LOD'+str(lod))
        bmesh.ops.delete(bm,geom=remove,context='FACES');bm.to_mesh(obj.data);bm.free();P.clean_unused(obj)
        measured=P.measurements(obj);relative='exports/'+obj.name+'.fbx';P.export_fbx(obj,model/relative)
        parts.setdefault(body_name,dict(measured,asset_folder=DEST,lods=[]))['lods'].append(dict(measured,lod=lod,fbx=relative))
        trim_rows.append({'lod':lod,'source_triangles':before,'removed_triangles':len(remove),'retained_triangles':measured['triangles'],
                          'protected':'windscreen glass/frame, seats, body outside exact interior boxes; original files unchanged'})
        obj.hide_render=lod>0;obj.hide_set(lod>0)

    atlas_path=model/'textures/Jeep_Atlas_Preview.png';atlas(atlas_path)
    mats={};objects=[]
    def mat(key,color,rough=.65,metal=0,textured=False):
        name='M_JeepInterior_'+key;m=bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF')
        p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
        row={'name':name,'base_color_linear':[*color,1],'roughness':rough,'metallic':metal,'textures':{},'normal_format':'OpenGL'}
        if textured:
            row['textures']=grain_maps(model,name,color,rough);row['base_color_linear']=[1,1,1,1];row['roughness']=1.
            for role,filename in row['textures'].items():
                im=bpy.data.images.load(str(model/filename),check_existing=True);im.colorspace_settings.name='sRGB'if role=='base_color'else'Non-Color';im.pack()
                tex=m.node_tree.nodes.new('ShaderNodeTexImage');tex.image=im
                if role=='normal':
                    normal=m.node_tree.nodes.new('ShaderNodeNormalMap');m.node_tree.links.new(tex.outputs['Color'],normal.inputs['Color']);m.node_tree.links.new(normal.outputs[0],p.inputs['Normal'])
                else:m.node_tree.links.new(tex.outputs['Color'],p.inputs['Base Color'if role=='base_color'else'Roughness'])
        materials.append(row);mats[key]=m;return m
    mat('CreamPaint',(.48,.46,.39),.69,.08,True);mat('DashRubber',(.022,.027,.024),.84,0,True)
    mat('Plastic',(.018,.022,.02),.64);mat('Steel',(.19,.21,.20),.43,.80)
    mat('Chrome',(.47,.49,.47),.25,.88);mat('Ink',(.69,.72,.61),.62)
    mat('Amber',(.33,.13,.018),.54);mat('Leather',(.12,.061,.031),.72)
    live=mat('LiveAtlas',(1,1,1),.62);live_row=materials[-1];live_row['textures']={'base_color':'textures/Jeep_Atlas_Preview.png'};live_row['runtime_live_atlas']=True
    image=bpy.data.images.load(str(atlas_path));image.pack();shader=live.node_tree.nodes.get('Principled BSDF');sample=live.node_tree.nodes.new('ShaderNodeTexImage');sample.image=image
    live.node_tree.links.new(sample.outputs['Color'],shader.inputs['Base Color']);live.node_tree.links.new(sample.outputs['Color'],shader.inputs['Emission Color']);shader.inputs['Emission Strength'].default_value=.14

    def box(label,at,size,key='CreamPaint',bevel=.008):
        bpy.ops.mesh.primitive_cube_add(size=1,location=at);o=bpy.context.object;o.name=label;o.dimensions=size
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mats[key])
        if bevel:
            mod=o.modifiers.new('formed rounded edges','BEVEL');mod.width=bevel;mod.segments=2;bpy.ops.object.modifier_apply(modifier=mod.name)
        objects.append(o);return o
    def rod(label,a,b,r,key='Steel',segments=16):
        a,b=Vector(a),Vector(b);bpy.ops.mesh.primitive_cylinder_add(vertices=segments,radius=r,depth=(b-a).length,location=(a+b)/2)
        o=bpy.context.object;o.name=label;o.rotation_mode='QUATERNION';o.rotation_quaternion=(b-a).to_track_quat('Z','Y');o.data.materials.append(mats[key]);objects.append(o);return o
    def label(words,at,size=.012,key='Ink'):
        bpy.ops.object.text_add(location=at);o=bpy.context.object;o.name='Label_'+words;o.data.body=words;o.data.size=size;o.data.align_x='CENTER';o.data.resolution_u=2
        o.rotation_euler=(math.pi/2,0,0);o.data.materials.append(mats[key]);bpy.ops.object.convert(target='MESH');objects.append(o);return o
    def disk(label_,at,r,key='Plastic',depth=.006,segments=48):
        return rod(label_,(at[0],at[1]+depth/2,at[2]),(at[0],at[1]-depth/2,at[2]),r,key,segments)
    def ring(label_,at,r,tube=.005,key='Chrome',rotation=(math.pi/2,0,0),major=64,minor=8):
        bpy.ops.mesh.primitive_torus_add(major_segments=major,minor_segments=minor,location=at,rotation=rotation,major_radius=r,minor_radius=tube)
        o=bpy.context.object;o.name=label_;o.data.materials.append(mats[key]);objects.append(o);return o
    def join(name,items):
        P.select(items);bpy.ops.object.join();obj=bpy.context.object;obj.name=name
        bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
        triangulate=obj.modifiers.new('export triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=triangulate.name)
        if not obj.data.uv_layers:
            obj.data.uv_layers.new(name='UVMap')
        P.clean_unused(obj);return obj

    box('flat pressed dash steel',(0,.195,1.225),(1.60,.07,.34),'CreamPaint',.014)
    box('formed soft upper pad',(0,.185,1.425),(1.62,.21,.09),'DashRubber',.023)
    box('lower folded dash lip',(0,.17,1.053),(1.57,.08,.024),'Steel',.006)
    for side in (-1,1):
        box('dashboard end cap',(side*.795,.19,1.23),(.025,.10,.32),'DashRubber',.01)
        box('rear tub inner trim',(side*.762,-.79,1.01),(.023,.43,.23),'Leather',.017)
        rod('inner tub rounded rail',(side*.754,-1.0,1.135),(side*.754,-.57,1.135),.014,'DashRubber')
        box('floor footwell liner',(side*.41,-.015,.60),(.48,.59,.027),'DashRubber',.012)
        for i in range(7):box('ribbed floor liner grip',(side*.41,-.24+i*.07,.617),(.43,.012,.007),'Plastic',.002)
    # Existing functional compass/keys mount to these actual physical supports.
    # The runtime retains only the turning card. Make an open shell here, with
    # the same real window/foot dimensions; no static duplicate compass card.
    centre=Vector(B(ANCHORS['compass_card_three_m']));yaw=ANCHORS['compass_yaw_radians'];radius=.047
    azimuths=sorted(set([-math.pi,math.pi,-.95,.95]+[-math.pi+i*2*math.pi/48 for i in range(49)]))
    elevations=sorted(set([-math.pi/2,math.pi/2,-.5,.5]+[-math.pi/2+i*math.pi/18 for i in range(19)]))
    vertices=[]
    for elevation in elevations:
        for azimuth in azimuths:
            x=radius*math.sin(azimuth)*math.cos(elevation);z=radius*math.cos(azimuth)*math.cos(elevation)
            offset=(x*math.cos(yaw)+z*math.sin(yaw),radius*math.sin(elevation),-x*math.sin(yaw)+z*math.cos(yaw))
            vertices.append(tuple(centre+Vector(B(offset))))
    faces=[];columns=len(azimuths)
    for j in range(len(elevations)-1):
        for i in range(columns-1):
            if abs((azimuths[i]+azimuths[i+1])/2)<.95 and abs((elevations[j]+elevations[j+1])/2)<.5:continue
            faces.append((j*columns+i,j*columns+i+1,(j+1)*columns+i+1,(j+1)*columns+i))
    mesh=bpy.data.meshes.new('open compass shell');mesh.from_pydata(vertices,[],faces);mesh.materials.append(mats['Plastic'])
    shell=bpy.data.objects.new('open compass ball housing',mesh);bpy.context.scene.collection.objects.link(shell)
    P.select([shell]);solid=shell.modifiers.new('thin compass shell wall','SOLIDIFY');solid.thickness=.0012;bpy.ops.object.modifier_apply(modifier=solid.name);objects.append(shell)
    box('compass foot',tuple(centre+Vector((0,0,-.051))),(.050,.050,.018),'Plastic',.003)
    for delta in (-.017,.017):
        rod('compass fixed drum end',tuple(centre+Vector((0,0,delta-.00075))),tuple(centre+Vector((0,0,delta+.00075))),.0405,'Plastic',48)
    box('ignition barrel bracket',(-.30,.130,1.14),(.050,.068,.052),'CreamPaint',.008)
    disk('ignition chrome barrel',(-.30,.084,1.14),.013,'Chrome',.020,24)
    box('ignition key slot',(-.30,.073,1.14),(.0025,.002,.015),'Plastic',.001)
    # Real steering shaft, collapsible boot and dash grommet; wheel is separate.
    hub=Vector(B(HUB));axis=Vector(WHEEL_AXIS_B)
    rod('steering column',(hub-axis*.38),hub-axis*.045,.025,'Plastic',24)
    for j in range(6):ring('column bellows',hub-axis*(.22+j*.012),.031,.003,'DashRubber',rotation=axis.to_track_quat('Z','Y').to_euler(),major=24)
    rod('column mounting bracket',(-.41,.21,1.02),tuple(hub-axis*.26),.018,'Steel')
    for name,datum in DIALS.items():
        at=B(datum['at_three']);r=datum['radius_m']
        disk(name+' black gauge cup',(at[0],at[1]+.012,at[2]),r+.009,'Plastic',.021)
        ring(name+' polished thin bezel',(at[0],at[1]+.001,at[2]),r+.004,.004)
        ring(name+' inset rubber seal',(at[0],at[1]+.006,at[2]),r+.009,.0025,'DashRubber')
    # Additional period oil/volts gauges are static until a runtime signal exists.
    for x,title,number in [(.16,'OIL','40'),(.28,'VOLTS','13')]:
        disk(title+' black face',(x,.129,1.205),.029,'Plastic',.008)
        ring(title+' bezel',(x,.122,1.205),.031,.003)
        label(title,(x,.119,1.212),.008);label(number,(x,.119,1.195),.009)
        for i in range(7):
            a=math.radians(-110+i*220/6);a0=(x+math.sin(a)*.022,.118,1.205+math.cos(a)*.022);a1=(x+math.sin(a)*.026,.118,1.205+math.cos(a)*.026)
            rod('static gauge tick',a0,a1,.0006,'Ink',6)
    # Openings are formed with strips, not a solid box covering the live face.
    rx,ry,rz=B(RADIO['at_three']);rw,rh=RADIO['size_m']
    box('radio back casing',(rx,ry+.047,rz),(rw+.032,.075,rh+.028),'Plastic',.006)
    for side in (-1,1):
        box('radio side bezel',(rx+side*(rw/2+.006),ry+.002,rz),(.012,.018,rh+.02),'Chrome',.002)
        box('radio horizontal bezel',(rx,ry+.002,rz+side*(rh/2+.005)),(rw+.024,.018,.01),'Chrome',.002)
        disk('radio tuning knob',(rx+side*.131,ry-.015,rz),.013,'Plastic',.021,24)
        ring('radio knurled knob edge',(rx+side*.131,ry-.026,rz),.013,.0018,'Steel',major=24,minor=6)
    box('glovebox recessed seam',(.59,.144,1.235),(.37,.008,.207),'DashRubber',.009)
    box('glovebox pressed door',(.59,.133,1.235),(.348,.012,.184),'CreamPaint',.006)
    disk('glovebox lock',(.59,.121,1.285),.009,'Chrome',.005,16)
    box('glovebox latch',(.59,.113,1.268),(.043,.018,.009),'Plastic',.003)
    label('SARO  /  FIELD SERVICE',(.59,.122,1.188),.014)
    for x,title in [(-.72,'LIGHTS'),(-.61,'WIPER')]:
        label(title,(x,.147,1.300),.009)
        disk(title+' pull switch',(x,.125,1.26),.013,'Plastic',.025,24)
        disk(title+' chrome washer',(x,.152,1.26),.017,'Chrome',.003,24)
    for x in (-.70,.68):
        box('demist vent frame',(x,.147,1.37),(.14,.03,.035),'Plastic',.005)
        for i in range(5):box('demist louver',(x-.054+i*.027,.127,1.37),(.007,.006,.027),'Steel',.001)
    for x,title in [(-.67,'HEAT'),(-.53,'DEF'),(-.39,'FAN')]:
        label(title,(x,.145,1.14),.0085)
        box('heater slider slot',(x,.137,1.107),(.08,.012,.008),'Plastic',.001)
        box('heater slider knob',(x-.014,.118,1.107),(.015,.026,.014),'Steel',.003)
    # Three pedals, small rubber grip ribs and visible mounting arms.
    for x,width,z in [(-.57,.065,.73),(-.43,.074,.75),(-.27,.045,.72)]:
        rod('pedal arm',(x,.23,.93),(x,-.03,z),.009,'Steel',12)
        pedal=box('pedal rubber grip',(x,-.045,z),(width,.045,.105),'DashRubber',.007)
        pedal.rotation_euler.x=math.radians(-18)
        for i in range(4):box('pedal grip rib',(x,-.071,z-.036+i*.022),(width*.82,.009,.006),'Plastic',.001)
    for x,foot,top,title in [(0.,(-.02,-.12,.64),(.02,-.36,.98),'1 3\n2 4 R'),(.13,(.12,-.11,.64),(.14,-.25,.83),'2H\n4H N 4L')]:
        box('transmission rubber boot',(x,-.13,.65),(.14,.17,.09),'DashRubber',.018)
        rod('gear selector stalk',foot,top,.009,'Steel',16)
        bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=10,radius=.025,location=top);o=bpy.context.object;o.name='Gear selector knob';o.data.materials.append(mats['Plastic']);objects.append(o)
        label(title,(top[0],top[1]-.022,top[2]+.005),.007)
    interior=join(PREFIX+'Interior',objects[:]);objects.clear()

    # Separate steering: geometry is baked around local zero and its real axis.
    rotation=Vector((0,0,1)).rotation_difference(axis)
    ring('steering grained rim',(0,0,0),.185,.014,'DashRubber',rotation=(0,0,0),major=72,minor=10)
    for degrees in (90,210,330):
        a=math.radians(degrees);radial=Vector((math.cos(a),math.sin(a),0))
        rod('three spoke satin wheel',radial*.045,radial*.172,.012,'Steel',12)
        rod('spoke inset black strip',radial*.060+Vector((0,0,.009)),radial*.155+Vector((0,0,.009)),.004,'Plastic',8)
    rod('steering hub',(0,0,-.024),(0,0,.020),.047,'Plastic',48)
    rod('horn button',(0,0,.019),(0,0,.027),.035,'Leather',48)
    steering=join(PREFIX+'Steering',objects[:]);objects.clear();steering.data.transform(rotation.to_matrix().to_4x4())

    def live_face(name,datum):
        x,y,z=B(datum['at_three']);radius=datum['radius_m'];cx,cy=datum['atlas_center_px'];pr=datum['atlas_radius_px'];steps=96
        verts=[(x,y,z)]+[(x+radius*math.cos(i*2*math.pi/steps),y,z+radius*math.sin(i*2*math.pi/steps))for i in range(steps)]
        verts += [(a,b+.0005,c)for a,b,c in verts]
        faces=[(0,i+1,(i+1)%steps+1)for i in range(steps)]
        faces += [(steps+1,steps+1+(i+1)%steps+1,steps+1+i+1)for i in range(steps)]
        faces += [(i+1,steps+1+i+1,steps+1+(i+1)%steps+1,(i+1)%steps+1)for i in range(steps)]
        mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.materials.append(live);uv=mesh.uv_layers.new(name='S47LiveAtlas')
        for poly in mesh.polygons:
            for loop in poly.loop_indices:
                p=mesh.vertices[mesh.loops[loop].vertex_index].co;u=(p.x-x)/radius;v=(p.z-z)/radius
                uv.data[loop].uv=((cx+u*pr)/512,1-(cy-v*pr)/256) if poly.index < steps else (4/512,1-4/256)
        obj=bpy.data.objects.new(name,mesh);bpy.context.scene.collection.objects.link(obj);return obj
    def cleanup_authored_microfaces(obj):
        # The compass latitude grid converges at the two poles. Its pole quads
        # triangulate to one real and one zero-area face; FBX/native discards
        # the latter. Measured glovebox-seam bevels also yielded two slivers
        # of 1.75e-11/4.22e-11 m2 (next-smallest face 4.04e-9). Use a uniform
        # authored minimum-feature area of 1e-10 m2: a 10-micrometre square.
        # This cleans invisible geometry before strict counts/probes, never
        # changes native tolerances, and never touches the source body.
        def f32(value):return struct.unpack('<f',struct.pack('<f',value))[0]
        def native_degenerate(face):
            # Unreal stores imported render positions in float32 centimetres.
            # The centimetre conversion can merge two distinct Blender floats;
            # mirror that conversion instead of raising an area tolerance.
            xyz=[(f32(v.co.x*100),f32(-v.co.y*100),f32(v.co.z*100))for v in face.verts]
            if len(set(xyz))<3:return True
            a=[f32(xyz[1][i]-xyz[0][i])for i in range(3)]
            b=[f32(xyz[2][i]-xyz[0][i])for i in range(3)]
            cross=[f32(f32(a[j]*b[k])-f32(a[k]*b[j]))for j,k in ((1,2),(2,0),(0,1))]
            return all(value==0 for value in cross)
        mesh=bmesh.new();mesh.from_mesh(obj.data)
        degenerate=[face for face in mesh.faces if len(face.verts)==3 and
                    (face.calc_area()<1e-10 or native_degenerate(face))]
        if degenerate:
            bmesh.ops.delete(mesh,geom=degenerate,context='FACES')
            mesh.to_mesh(obj.data)
            print('S47_JEEP_AUTHORED_MICROFACES_REMOVED',obj.name,len(degenerate),flush=True)
        mesh.free()

    own=[interior,steering]+[live_face(d['part'],d)for d in DIALS.values()]
    x,y,z=B(RADIO['at_three']);w,h=RADIO['size_m']
    radio=box(PREFIX+'RadioFace',(x,y+.00025,z),(w,.0005,h),'LiveAtlas',0)
    uv=radio.data.uv_layers.active
    for poly in radio.data.polygons:
        for li in poly.loop_indices:
            co=radio.matrix_world@radio.data.vertices[radio.data.loops[li].vertex_index].co
            uv.data[li].uv=((co.x-x)/w+.5,1-(160+(.5-(co.z-z)/h)*96)/256) if poly.normal.y < -.5 else (4/512,1-4/256)
    radio=join(PREFIX+'RadioFace',[radio]);objects.clear();own.append(radio)
    for original in own:
        for lod,ratio in enumerate((1.,.60,.32)):
            obj=original if lod==0 else original.copy()
            if lod:
                obj.data=original.data.copy();bpy.context.scene.collection.objects.link(obj);obj.name=original.name+'_LOD'+str(lod)
            if original==steering:obj.location=(0,0,0)
            P.select([obj]);tri=obj.modifiers.new('export triangulation','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
            if lod and original in (interior,steering):P.reduce_mesh(obj,ratio,'authored interior distance detail')
            if original in (interior,steering):cleanup_authored_microfaces(obj)
            P.clean_unused(obj);bpy.context.view_layer.update();measured=P.measurements(obj);relative='exports/'+obj.name+'.fbx'
            # Native FBX conversion and the live SharedMid each flip V. Export
            # live face crops in the compensating convention, while preserving
            # the editable Blender preview and every other PBR texture's UVs.
            is_live_face=original.name in {d['part']for d in DIALS.values()}|{RADIO['part']}
            saved_uvs=[(layer,[tuple(loop.uv)for loop in layer.data])for layer in obj.data.uv_layers] if is_live_face else []
            try:
                for layer,values in saved_uvs:
                    for loop,uv in zip(layer.data,values):loop.uv=(uv[0],1-uv[1])
                P.export_fbx(obj,model/relative)
            finally:
                for layer,values in saved_uvs:
                    for loop,uv in zip(layer.data,values):loop.uv=uv
            parts.setdefault(original.name,dict(measured,asset_folder=DEST,lods=[]))['lods'].append(dict(measured,lod=lod,fbx=relative))
            if original==steering:obj.location=Vector(B(HUB))
            obj.hide_render=lod>0;obj.hide_set(lod>0)
    # The file is editable. Runtime needles are deliberately not baked/exported.
    preview_collection=bpy.data.collections.new('PreviewOnly_Needles');bpy.context.scene.collection.children.link(preview_collection)
    for key,datum in DIALS.items():
        x,y,z=B(datum['at_three']);a=math.radians(-105 if key=='speed'else 0);length=datum['needle_length_m']
        needle=rod('Preview needle '+key,(x,y-.003,z),(x+math.sin(a)*length,y-.003,z+math.cos(a)*length),.0015,'Amber',8)
        for collection in list(needle.users_collection):collection.objects.unlink(needle)
        preview_collection.objects.link(needle)
    for image in bpy.data.images:
        if image.source=='FILE' and not image.packed_file:image.pack()
    P.write_json(model/'anchors.json',ANCHORS)
    P.write_json(model/'materials.json',materials)
    contract={'version':1,'coordinates':P.COORDINATES,'parts':parts,'status':'PREPARED_NATIVE_UNVERIFIED',
              'source_glb':input_contract['source_glb'],'source_sha256':input_contract['source_sha256'],
              'source_blend':str(input_blend),'source_trim':trim_rows,'anchors':ANCHORS,
              'scope':'Own fitted Jeep interior plus precise source interior cleanup; no C10 cabin, no source wheel/body redesign.'}
    P.write_json(model/'model_contract.json',contract)
    P.write_json(destination/'manifest.json',{'version':1,'coordinates':P.COORDINATES,'models':[{
        'id':'classic_jeep','name':body_name,'output_dir':str(model),'contract':'classic_jeep/model_contract.json',
        'materials':'classic_jeep/materials.json','source_glb':input_contract['source_glb'],
        'source_sha256':input_contract['source_sha256'],'status':'PREPARED_NATIVE_UNVERIFIED'}]})
    bpy.context.scene['S47JeepInteriorAnchors']=json.dumps(ANCHORS)
    bpy.ops.wm.save_as_mainfile(filepath=str(model/'jeep-interior.blend'))
    P.require(all(P.sha(Path(path))==digest for path,digest in protected.items()),'An immutable input changed')
    if preview:render_preview(model)
    print('S47_JEEP_INTERIOR_READY',str(destination/'manifest.json'),{k:[l['triangles']for l in v['lods']]for k,v in parts.items()},flush=True)


def render_preview(root):
    import bpy
    from mathutils import Vector
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=12;scene.cycles.max_bounces=3;scene.cycles.use_denoising=True
    scene.render.threads_mode='FIXED';scene.render.threads=2;scene.render.resolution_x=960;scene.render.resolution_y=540;scene.render.resolution_percentage=100
    scene.world=bpy.data.worlds.new('Jeep interior preview world');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.10,.13,.16,1);scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.3
    for at,power,size in [((1,-2,3),600,2),((-2,-1,2),350,2),((0,2,3),700,2)]:
        data=bpy.data.lights.new('interior preview softbox','AREA');data.energy=power;data.size=size;obj=bpy.data.objects.new(data.name,data);scene.collection.objects.link(obj);obj.location=at;obj.rotation_euler=(Vector((0,.1,1.2))-obj.location).to_track_quat('-Z','Y').to_euler()
    camera=bpy.data.cameras.new('inspected Jeep driver camera');camera.lens=24;camera.clip_start=.02;obj=bpy.data.objects.new(camera.name,camera);scene.collection.objects.link(obj);scene.camera=obj
    for key,position,target in [('driver',B(EYE),(-.30,.15,1.38)),('dash-detail',(.02,-.45,1.44),(.03,.125,1.26)),('open-cabin',(-2,-2,2.8),(0,0,1.1))]:
        obj.location=position;obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(root/(key+'.png'));bpy.ops.render.render(write_still=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--source',type=Path,default=DEFAULT_SOURCE);parser.add_argument('--output',type=Path,default=DEFAULT_OUTPUT);parser.add_argument('--preview',action='store_true')
    options=parser.parse_args(sys.argv[sys.argv.index('--')+1:]if'--'in sys.argv else sys.argv[1:])
    build(options.source,options.output,options.preview)


if __name__=='__main__':main()
