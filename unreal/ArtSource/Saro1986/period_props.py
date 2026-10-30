"""Editable SARO field jacket and split, private Western Electric 500 phone.

Run Blender -b -t 2 --python period_props.py [-- --phone|--jacket].
The jacket is original geometry. The previously acquired CC BY phone and its
textures stay private; only split FBXs using the existing native materials are
written below S47_SARO_PERIOD_WORK (default ~/scratch-bm/.../prepared).
Three (x,height,z) maps to Blender (x,-z,height), as in Office's FBX recipe.
"""
from pathlib import Path
import json
import math
import os
import sys
import zipfile

ROOT = Path(__file__).resolve().parent
WORK = Path(os.environ.get("S47_SARO_PERIOD_WORK", str(Path.home()/
    "scratch-bm/Signal47-saro-1986-props-20261030/prepared")))
PHONE = Path.home()/"Assets/signal47-free/sketchfab/rotary_phone_we500.zip"


def bpy_tools():
    import bpy
    from mathutils import Vector

    def active(obj):
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj

    def xyz(p):
        return Vector((p[0], -p[2], p[1]))

    def finish(objects, name, target, keep_editable=False):
        bpy.ops.object.select_all(action="DESELECT")
        for obj in objects:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = objects[0]
        if keep_editable:
            bpy.ops.object.duplicate()
        bpy.ops.object.join()
        mesh = bpy.context.object
        mesh.name = name
        active(mesh)
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        tri = mesh.modifiers.new("Native_triangles", "TRIANGULATE")
        bpy.ops.object.modifier_apply(modifier=tri.name)
        target.parent.mkdir(parents=True, exist_ok=True)
        bpy.ops.export_scene.fbx(filepath=str(target), use_selection=True,
            object_types={"MESH"}, apply_unit_scale=True,
            apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y", axis_up="Z",
            bake_anim=False, mesh_smooth_type="FACE")
        bounds = [[float(fn(v.co[k] for v in mesh.data.vertices)) for k in range(3)]
                  for fn in (min, max)]
        export_path = target.relative_to(ROOT) if target.is_relative_to(ROOT) else target.relative_to(WORK)
        return mesh, {"asset": name, "fbx": str(export_path),
                      "triangles": len(mesh.data.polygons), "bounds_local_m": bounds,
                      "materials": [m.name for m in mesh.data.materials]}
    return bpy, Vector, active, xyz, finish


def phone():
    bpy, Vector, active, xyz, finish = bpy_tools()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1
    source = WORK/"source_private"
    source.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(PHONE) as z:
        for member in z.infolist():
            path = (source/member.filename).resolve()
            if not path.is_relative_to(source.resolve()):
                raise ValueError("Unsafe archive member")
        z.extractall(source)
    bpy.ops.import_scene.gltf(filepath=str(source/"scene.gltf"))
    objects = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    assert objects
    # Source already has physical metre scale. Freeze the entire glTF hierarchy
    # before detaching its meshes; source +Y is rear, so its front maps +Three Z.
    for obj in objects:
        matrix = obj.matrix_world.copy()
        obj.parent = None
        obj.matrix_world = matrix
        active(obj)
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    parts = {"SM_Saro_PhoneBody": [], "SM_Saro_PhoneHandset": []}
    for obj in objects:
        name = obj.name
        handset = (name.startswith("Handset_") and not name.startswith("Handset_Jack")) or \
                  name.startswith("Coiled_Cord") or name.startswith("Plug_Handset")
        parts["SM_Saro_PhoneHandset" if handset else "SM_Saro_PhoneBody"].append(obj)
    assert parts["SM_Saro_PhoneHandset"] and parts["SM_Saro_PhoneBody"]
    # The existing story rotates/hides room.handset at (0,.085,-.045) relative
    # to the telephone root. Put the new handset and its cable in those terms.
    anchor = xyz((0, .085, -.045))
    for obj in parts["SM_Saro_PhoneHandset"]:
        for vertex in obj.data.vertices:
            vertex.co -= anchor
    meshes = []
    for name, objects in parts.items():
        # Preserve dial/cord silhouette; reduce dense hidden feet and flat cases.
        for obj in objects:
            active(obj)
            original = sum(len(p.vertices)-2 for p in obj.data.polygons)
            if original > 900:
                dec = obj.modifiers.new("Metric_phone_game_mesh", "DECIMATE")
                dec.ratio = .30 if obj.name.startswith(("Coiled_Cord", "Handset_", "Rotary_Dial")) else .16
                dec.use_collapse_triangulate = True
                bpy.ops.object.modifier_apply(modifier=dec.name)
        mesh, row = finish(objects, name, WORK/(name+".fbx"))
        row["materials"] = [{"slot": n,
            "path": "/Game/Free/Office/rotary_phone_we500/Materials/"+n+"."+n}
            for n in row["materials"]]
        assert 0 < row["triangles"] < 60000
        meshes.append(row)
    (WORK/"phone-contract.json").write_text(json.dumps({
        "source": str(PHONE), "license": "CC BY 4.0; existing project credit Skyice",
        "private": True, "source_geometry_preserved": True,
        "units": "metres", "collision": "NoCollision; original phone/handset query nodes retained",
        "handset_parent_local_three_m": [0, .085, -.045], "meshes": meshes}, indent=2)+"\n")
    print("SARO PHONE READY", str(WORK/"phone-contract.json"), flush=True)


def jacket():
    bpy, Vector, active, xyz, finish = bpy_tools()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1
    mats = {}
    rows = [
        {"name": "M_Saro_JacketNavy", "base_color_linear": [.025, .037, .067, 1],
         "roughness": .94, "metallic": 0, "specular": .26,
         "normal_convention": "DirectX", "textures": {
             "roughness": "../Office/textures/M_Office_ChairFabric_rough.png",
             "normal": "../Office/textures/M_Office_ChairFabric_normal_dx.png"}},
        {"name": "M_Saro_JacketTape", "base_color_linear": [.36, .39, .37, 1],
         "roughness": .55, "metallic": .08, "specular": .45},
        {"name": "M_Saro_JacketThread", "base_color_linear": [.20, .23, .26, 1],
         "roughness": .91, "metallic": 0, "specular": .22},
    ]
    for row in rows:
        material = bpy.data.materials.new(row["name"])
        material.use_nodes = True
        p = material.node_tree.nodes.get("Principled BSDF")
        p.inputs["Base Color"].default_value = row["base_color_linear"]
        p.inputs["Roughness"].default_value = row["roughness"]
        p.inputs["Metallic"].default_value = row["metallic"]
        mats[row["name"]] = material
    navy, tape, thread = [mats[r["name"]] for r in rows]
    objects = []

    def surface(name, points, faces, material, solid=.0014, smooth=True):
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata([xyz(p) for p in points], [], faces)
        mesh.update()
        obj = bpy.data.objects.new(name, mesh)
        scene.collection.objects.link(obj)
        obj.data.materials.append(material)
        active(obj)
        if smooth:
            for face in mesh.polygons:
                face.use_smooth = True
        if solid:
            mod = obj.modifiers.new("Real_cloth_thickness_open_edges", "SOLIDIFY")
            mod.thickness = solid
            mod.offset = 0
            bpy.ops.object.modifier_apply(modifier=mod.name)
        objects.append(obj)
        return obj

    def line(name, points, material=thread, radius=.0011):
        curve = bpy.data.curves.new(name, "CURVE")
        curve.dimensions = "3D"
        curve.resolution_u = 2
        curve.bevel_depth = radius
        curve.bevel_resolution = 2
        spline = curve.splines.new("POLY")
        spline.points.add(len(points)-1)
        for p, v in zip(spline.points, points):
            p.co = (*xyz(v), 1)
        obj = bpy.data.objects.new(name, curve)
        scene.collection.objects.link(obj)
        obj.data.materials.append(material)
        active(obj)
        bpy.ops.object.convert(target="MESH")
        objects.append(obj)
        return obj

    levels = [(1.045,.218,.042), (1.080,.225,.047), (1.15,.222,.050),
              (1.24,.218,.052), (1.34,.215,.049), (1.44,.219,.049),
              (1.54,.224,.047), (1.62,.225,.044), (1.665,.195,.038),
              (1.705,.105,.027), (1.730,.059,.022)]

    def dimensions(y):
        for (a,w,d),(b,v,e) in zip(levels, levels[1:]):
            if a <= y <= b:
                t=(y-a)/(b-a)
                return w*(1-t)+v*t, (d*(1-t)+e*t)*.66
        w,d=levels[0][1:] if y < levels[0][0] else levels[-1][1:]
        return w,d*.66

    def ring_point(y, angle, lift=0):
        width, depth = dimensions(y)
        fold = .0055*math.sin(angle*9+y*18)+.0025*math.sin(angle*17-y*13)
        return (-.028-(depth+fold+lift)*math.cos(angle), y,
                (width+fold*.5)*math.sin(angle))

    def ring_strip(name, heights, material, lift=0):
        n=64
        points=[ring_point(y, opening(y)+(math.tau-2*opening(y))*i/n, lift)
                for y in heights for i in range(n+1)]
        faces=[(j*(n+1)+i,j*(n+1)+i+1,(j+1)*(n+1)+i+1,(j+1)*(n+1)+i)
               for j in range(len(heights)-1) for i in range(n)]
        return surface(name, points, faces, material)

    def opening(y):
        # A short unzipped V relaxes the empty upper fronts below the collar.
        return .028+.16*min(1,max(0,(y-1.50)/.17))

    ring_strip("Draped_torso_open_neck_and_hem", [r[0] for r in levels], navy)
    ring_strip("Reflective_body_tape_follows_folds", [1.242,1.250,1.282,1.290], tape, .0015)
    ring_strip("Folded_hem_edge", [1.046,1.058], thread, .0007)
    ring_strip("Standing_folded_collar", [1.721,1.738,1.754], navy, .003)

    for side in (-1,1):
        centers=[(-.027,1.627,side*.204,.079),(-.037,1.555,side*.253,.076),
                 (-.045,1.45,side*.280,.067),(-.053,1.34,side*.287,.061),
                 (-.052,1.23,side*.274,.057),(-.043,1.12,side*.259,.049),
                 (-.041,1.070,side*.257,.047)]
        if side < 0:
            # One unfilled sleeve has caught slightly against the body; the
            # shorter folded cuff breaks the mirror-symmetric mannequin shape.
            centers=[(-.027,1.627,-.204,.079),(-.038,1.555,-.251,.075),
                     (-.047,1.45,-.273,.067),(-.060,1.34,-.254,.060),
                     (-.070,1.25,-.219,.054),(-.071,1.18,-.188,.048),
                     (-.068,1.145,-.173,.046)]
        points=[]
        n=24
        for j,(x,y,z,r) in enumerate(centers):
            for i in range(n):
                a=math.tau*i/n
                ripple=.0055*math.sin(a*5+j*.9)+.002*math.sin(a*9-j)
                points.append((x+math.cos(a)*(r*.35+ripple),
                               y+.010*math.sin(a*3+j),
                               z+math.sin(a)*(r+ripple)))
        faces=[(j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i)
               for j in range(len(centers)-1) for i in range(n)]
        sleeve=surface("Empty_hanging_sleeve_%s"%side,points,faces,navy)
        # The cuff is a ring, never a cylinder cap or a box-shaped arm.
        cuff=[]
        for yoff in (0,.022):
            x,y,z,r=centers[-1]
            cuff.extend([(x+math.cos(math.tau*i/n)*r*.35,y+yoff,
                          z+math.sin(math.tau*i/n)*r) for i in range(n)])
        surface("Open_cuff_binding_%s"%side,cuff,
                [(i,(i+1)%n,n+(i+1)%n,n+i) for i in range(n)],thread)
        line("Shoulder_seam_%s"%side,
             [(-.076,1.66,side*.095),(-.076,1.64,side*.16),(-.061,1.61,side*.228)])
        # Cloth chest patch with a gently sagged face and separate folded flap.
        pts=[]
        for j in range(5):
            y=1.38+j*.030
            width,depth=dimensions(y)
            for i in range(6):
                z=side*(.065+i*.023)
                a=math.asin(min(.96,abs(z)/width))*side
                x=ring_point(y,a)[0]-.003-.003*math.sin(i*math.pi/5)*math.sin(j*math.pi/4)
                pts.append((x,y,z))
        surface("Sewn_chest_pocket_%s"%side,pts,
                [(j*6+i,j*6+i+1,(j+1)*6+i+1,(j+1)*6+i)
                 for j in range(4) for i in range(5)],navy,.001)
        line("Pocket_top_welt_%s"%side,[pts[24+i] for i in range(6)],thread,.0009)

    # Two raised zipper tapes follow the front seam; no filled-in neck opening.
    for side in (-1,1):
        line("Zipper_tape_%s"%side,
             [(ring_point(y,0)[0]-.002,y,side*.007) for y in (1.065,1.15,1.25,1.35,1.45,1.50)],
             thread,.0013)
        line("Soft_open_front_edge_%s"%side,
             [ring_point(y,side*opening(y),.001) for y in (1.50,1.54,1.58,1.62,1.66,1.70,1.73)],
             navy,.0014)
    line("Zipper_pull",[(-.085,1.43,-.003),(-.091,1.413,0),(-.087,1.403,.005)],tape,.0018)
    # A real hook behind the neck supports the cloth at the original wall point.
    line("Wall_hook",[(.042,1.775,0),(.005,1.775,0),(-.004,1.759,0),(.003,1.748,0)],tape,.005)

    # Plain SARO issue patch, no new story evidence or invented text.
    from mathutils import Matrix
    for i,letter in enumerate("SARO"):
        y,z=1.475,-.145+i*.014
        width,_=dimensions(y)
        def front(at):
            return ring_point(y,math.asin(at/width))[0]
        slope=(front(z+.0002)-front(z-.0002))/.0004
        horizontal=Vector((slope,-1,0)).normalized()
        up=Vector((0,0,1));normal=horizontal.cross(up).normalized()
        text=bpy.data.curves.new("SARO_sewn_letter_%d"%i,"FONT")
        text.body=letter;text.align_x="CENTER";text.size=.015;text.extrude=.00015
        obj=bpy.data.objects.new(text.name,text);scene.collection.objects.link(obj)
        obj.location=xyz((front(z)-.0015,y,z))
        obj.rotation_euler=Matrix((horizontal,up,normal)).transposed().to_euler()
        obj.data.materials.append(tape);active(obj)
        bpy.ops.object.convert(target="MESH");objects.append(obj)
    for obj in objects:
        active(obj)
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.cube_project(cube_size=.30)
        bpy.ops.object.mode_set(mode="OBJECT")
        # Keep both empty sleeves inside the original 64cm-wide jacket query.
        for vertex in obj.data.vertices:
            vertex.co.y *= .90
    mesh,contract=finish(objects,"SM_Saro_HangingJacket",ROOT/"Export/SM_Saro_HangingJacket.fbx",True)
    contract.update(units="metres",mapping="Blender (x,-z,y) to UE (100x,100z,100y)",
                    collision="NoCollision; original jacket query and saved visibility retained",
                    reuse_materials={},source="period_props.py",editable_components=len(objects))
    assert contract["triangles"]<30000
    ROOT.mkdir(parents=True,exist_ok=True)
    (ROOT/"jacket-materials.json").write_text(json.dumps(rows,indent=2)+"\n")
    (ROOT/"jacket-contract.json").write_text(json.dumps(contract,indent=2)+"\n")
    mesh.hide_set(True);mesh.hide_render=True
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/"Signal47_SaroHangingJacket.blend"))
    print("SARO JACKET READY",json.dumps({"triangles":contract["triangles"],"bounds":contract["bounds_local_m"]}),flush=True)


if __name__ == "__main__":
    args=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
    if not args or "--phone" in args:
        phone()
    if not args or "--jacket" in args:
        jacket()
