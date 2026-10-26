"""Authored nature ground follows the unchanged native road grid. Blender only."""
from pathlib import Path
import json
import math
import sys
import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT))
from terrain_contract import GX,GZ,REGION,ANCHOR,grid_y,floor_y,visual_y,terrain_allowed,old_distance,smooth,blender_point,BUILDINGS,DINER,STREET,TOWN,box


def alpha_at(x,z,shoulder):
    a=smooth(0,20,min(x-REGION[0],REGION[1]-x,z-REGION[2],REGION[3]-z))
    a*=smooth(5,9,old_distance(x,z))
    if shoulder:
        a*=smooth(5,6.2,abs(x))*(1-smooth(16.5,19,abs(x)))
    else:
        a*=smooth(18.2,24,abs(x))
    for b in BUILDINGS+(DINER,STREET):
        distance=max(b[0]-x,x-b[1],b[2]-z,z-b[3])
        a*=smooth(.2,3,distance)
    return a


def tint(x,z):
    wash=math.sin(z*.017+x*.011+math.sin(x*.013)*1.5)
    # Broad mineral/erosion variation survives beyond the repeating 6m detail.
    # This is albedo variation, with no directional/baked illumination.
    broad=.79+.13*wash+.065*math.sin(x*.041-z*.009)
    return (broad,broad*(.97+.018*math.sin(x*.007)),broad*.93)


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene=bpy.context.scene
    scene.unit_settings.system='METRIC'
    scene.unit_settings.scale_length=1
    output=ROOT/'exports'
    output.mkdir(exist_ok=True)
    rows=json.loads((ROOT/'materials.json').read_text())
    materials=[]
    for row in rows:
        m=bpy.data.materials.new(row['name']);m.use_nodes=True
        bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.92
        vc=m.node_tree.nodes.new('ShaderNodeVertexColor');vc.layer_name='Color'
        m.node_tree.links.new(vc.outputs['Alpha'],bs.inputs['Alpha']);m.surface_render_method='DITHERED'
        for role,file in row['textures'].items():
            im=bpy.data.images.load(str(ROOT/file),check_existing=True)
            if role!='base_color':im.colorspace_settings.name='Non-Color'
            tex=m.node_tree.nodes.new('ShaderNodeTexImage');tex.image=im
            if role=='base_color':
                mix=m.node_tree.nodes.new('ShaderNodeMix');mix.data_type='RGBA';mix.blend_type='MULTIPLY';mix.inputs[0].default_value=1
                m.node_tree.links.new(tex.outputs['Color'],mix.inputs[6]);m.node_tree.links.new(vc.outputs['Color'],mix.inputs[7])
                m.node_tree.links.new(mix.outputs[2],bs.inputs['Base Color'])
            elif role=='roughness':m.node_tree.links.new(tex.outputs[0],bs.inputs['Roughness'])
            elif role=='normal':
                sep=m.node_tree.nodes.new('ShaderNodeSeparateXYZ');com=m.node_tree.nodes.new('ShaderNodeCombineXYZ')
                inv=m.node_tree.nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1
                m.node_tree.links.new(tex.outputs[0],sep.inputs[0]);m.node_tree.links.new(sep.outputs['Y'],inv.inputs[1])
                m.node_tree.links.new(sep.outputs['X'],com.inputs['X']);m.node_tree.links.new(inv.outputs[0],com.inputs['Y']);m.node_tree.links.new(sep.outputs['Z'],com.inputs['Z'])
                nm=m.node_tree.nodes.new('ShaderNodeNormalMap');nm.inputs['Strength'].default_value=.38
                m.node_tree.links.new(com.outputs[0],nm.inputs['Color']);m.node_tree.links.new(nm.outputs[0],bs.inputs['Normal'])
        materials.append(m)

    tiles={}
    def triangle(points,shoulder=False):
        cx=sum(p[0] for p in points)/3;cz=sum(p[1] for p in points)/3
        if not terrain_allowed(cx,cz):return
        alphas=[alpha_at(x,z,shoulder) for x,z in points]
        if max(alphas)<.0001:return
        tx=int((cx-REGION[0])//200);tz=int((cz-REGION[2])//200)
        key=(shoulder,tx,tz)
        tile=tiles.setdefault(key,{'vertices':[],'faces':[],'rgba':[],'uv':[]})
        start=len(tile['vertices'])
        for (x,z),a in zip(points,alphas):
            h=(visual_y(x,z) if shoulder or box(x,z,TOWN) else grid_y(x,z))+.016
            tile['vertices'].append(blender_point(x,z,h));tile['rgba'].append((*tint(x,z),a));tile['uv'].append((x/6,-z/6))
        # Road-local+Z is Blender-Y; reverse winding for upward surfaces.
        tile['faces'].append((start,start+2,start+1))

    xs=[x for x in GX if REGION[0]<=x<=REGION[1]]
    # Keep the outer native parent cell intact too. Clipping its X edge to
    # 600 m would create a different triangle diagonal at the alpha boundary.
    xs=sorted(set(xs+[max(x for x in GX if x<=REGION[0]),min(x for x in GX if x>=REGION[1])]))
    zs=[z for z in GZ if REGION[2]<=z<=REGION[3]]
    zs=sorted(set([REGION[2],REGION[3]]+zs))
    for xa,xb in zip(xs,xs[1:]):
        if xa>=-18.2 and xb<=18.2:continue
        step=2 if min(abs(xa),abs(xb))<42 else 5 if min(abs(xa),abs(xb))<120 else 18
        n=max(1,math.ceil(max(xb-xa,10)/step))
        for za,zb in zip(zs,zs[1:]):
            for j in range(n):
                for i in range(n):
                    x0=xa+(xb-xa)*i/n;x1=xa+(xb-xa)*(i+1)/n
                    z0=za+(zb-za)*j/n;z1=za+(zb-za)*(j+1)/n
                    triangle(((x0,z0),(x1,z0),(x0,z1)))
                    triangle(((x1,z1),(x0,z1),(x1,z0)))
    for sign in (-1,1):
        for iz in range(int(REGION[2]),int(REGION[3])):
            for ix in range(5,19):
                x0,x1=sign*ix,sign*(ix+1);z0,z1=float(iz),float(iz+1)
                if sign>0:
                    triangle(((x0,z0),(x1,z0),(x0,z1)),True);triangle(((x1,z1),(x0,z1),(x1,z0)),True)
                else:
                    triangle(((x1,z0),(x0,z0),(x1,z1)),True);triangle(((x0,z1),(x1,z1),(x0,z0)),True)
    contract={'anchor_world_three_m':ANCHOR,'region_road_local_xz':REGION,
              'collision':'NoCollision; unchanged native floor/road remains authoritative','lift_metres':.016,'parts':{}}
    for (shoulder,tx,tz),tile in sorted(tiles.items()):
        name=f'SM_CN_{"Shoulder" if shoulder else "Terrain"}_{tx}_{tz}'
        mesh=bpy.data.meshes.new(name);mesh.from_pydata(tile['vertices'],[],tile['faces']);mesh.update()
        mesh.materials.append(materials[int(shoulder)])
        uv=mesh.uv_layers.new(name='UVMap');col=mesh.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
        for i,rgba in enumerate(tile['rgba']):col.data[i].color=rgba
        for p in mesh.polygons:
            for li in p.loop_indices:uv.data[li].uv=tile['uv'][mesh.loops[li].vertex_index]
        ob=bpy.data.objects.new(name,mesh);scene.collection.objects.link(ob)
        coords=tile['vertices'];bounds=[[min(p[i] for p in coords) for i in range(3)],[max(p[i] for p in coords) for i in range(3)]]
        contract['parts'][name]={'triangles':len(mesh.polygons),'bounds_local_m':bounds,
            'materials':[materials[int(shoulder)].name],'used_materials':[materials[int(shoulder)].name],
            'vertex_color_attribute':'Color','vertex_alpha_min':min(p[3] for p in tile['rgba']),
            'vertex_alpha_max':max(p[3] for p in tile['rgba']),'origin_m':[0,0,0]}
        bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
        bpy.ops.export_scene.fbx(filepath=str(output/(name+'.fbx')),use_selection=True,apply_unit_scale=True,
            apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',colors_type='LINEAR',
            prioritize_active_color=True,mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False)
    contract['total_triangles']=sum(p['triangles'] for p in contract['parts'].values())
    assert contract['total_triangles']<300000,contract['total_triangles']
    (ROOT/'model_contract.json').write_text(json.dumps(contract,indent=2)+'\n')
    # Keep real image dependencies portable without modifying source pixels.
    for im in bpy.data.images:
        if im.source=='FILE':im.filepath='//'+str(Path(im.filepath).relative_to(ROOT))
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Caliche_Nature_Ground.blend'))
    bpy.ops.export_scene.gltf(filepath=str(output/'Caliche_Nature_Ground.glb'),use_selection=True,
        export_format='GLB',export_vertex_color='ACTIVE',export_all_vertex_colors=False)
    print('NATURE_GROUND_SOURCE',len(contract['parts']),contract['total_triangles'])


if __name__=='__main__':main()
