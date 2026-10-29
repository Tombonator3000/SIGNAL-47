"""Original SARO frames, sharing the already authored Office materials and FBX recipe.

Library use: append_frame(box, cyl, materials, origin=(0, 0, 0), style="service").
Importing this file never imports bpy, resets a scene, reads a blend, or exports.
Standalone: blender -b -Y -t 2 --python build_frames.py
All helper coordinates are Three (x, height, z), in metres.
"""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parent
OFFICE = ROOT.parent / "Office"
LEAF_ASSET = "/Game/Signal47/Art/Office/SM_Office_Door.SM_Office_Door"
MATERIAL_ROOT = "/Game/Signal47/Art/Office/Materials"
MATERIALS = {
    "paint": "M_Office_CabinetEnamel", "wood": "M_Office_Walnut",
    "cream": "M_Office_CreamEnamel", "metal": "M_Office_BrushedMetal",
    "brass": "M_Office_AgedBrass", "rubber": "M_Office_BlackBakelite",
}
PART_NAMES = {"service": "SM_DoorFrame_Service", "archive": "SM_DoorFrame_Archive"}


def frame_contract(width=1.0, height=2.07, leaf_bottom=.012, gap=.004,
                   depth=.18, nominal_thickness=.045):
    """Closed leaf x=0..width, midplane z=0; +z is the rear/stop side."""
    if min(width, height, depth) <= 0 or not 0 < gap < .012:
        raise ValueError("Invalid metric doorway dimensions")
    # Office's main core is 55mm; its separate edge lipping reaches +/-32mm.
    # A stop based only on the requested 45mm core would intersect that lipping.
    depth_scale = nominal_thickness / .055
    perimeter_rear = .032 * depth_scale
    contact = perimeter_rear + .002
    if not contact + .025 < depth * .5:
        raise ValueError("Frame is too shallow for the actual leaf perimeter")
    return {
        "leaf_width_m": width, "leaf_height_m": height, "leaf_bottom_m": leaf_bottom,
        "leaf_core_thickness_m": nominal_thickness, "office_leaf_core_thickness_m": .055,
        "office_leaf_perimeter_rear_m": .032, "leaf_depth_scale": depth_scale,
        "scaled_leaf_perimeter_rear_m": perimeter_rear,
        "side_and_head_clearance_m": gap, "stop_clearance_m": .002,
        "stop_contact_z_m": contact, "solid_stop_front_z_m": contact + .002,
        "frame_depth_m": depth, "threshold_top_m": leaf_bottom - gap,
        "hinge_origin_three_m": [0, 0, 0], "leaf_midplane_z_m": 0,
        "opening_x_m": [-gap, width + gap],
        "opening_y_m": [leaf_bottom - gap, leaf_bottom + height + gap],
        "scale_contract": {
            "FrameScaleX": "actual leaf width / leaf_width_m",
            "FrameScaleY": "actual leaf height / leaf_height_m",
            "ue_scale": "(FrameScaleX, 1, FrameScaleY)",
            "leaf_location_three_m": [width * .5, leaf_bottom + height * .5, 0],
            "leaf_mesh": LEAF_ASSET,
        },
    }


def append_frame(box, cyl, materials, origin=(0, 0, 0), width=1.0, height=2.07,
                 style="service", prefix=None, leaf_bottom=.012, depth=.18):
    """Append real profiled jambs/stop/gasket/threshold through Office-compatible helpers.

    materials maps the original M_Office_* names to existing Blender materials.
    The passed box/cyl helpers retain ownership of their objects and collections.
    This function does not add a leaf, moving hardware, collision, light or label.
    """
    if style not in PART_NAMES:
        raise ValueError("style must be service or archive")
    c = frame_contract(width, height, leaf_bottom=leaf_bottom, depth=depth)
    prefix = prefix or PART_NAMES[style]
    objects = []
    def at(p):
        return tuple(float(a) + float(b) for a, b in zip(origin, p))
    def mat(role):
        return materials[MATERIALS[role]]
    def b(name, point, size, role, bevel=.0015):
        o = box(prefix + "_" + name, at(point), size, mat(role), bevel=bevel)
        objects.append(o)
        return o
    def rod(name, a, z, radius, role, segments=16):
        o = cyl(prefix + "_" + name, at(a), at(z), radius, mat(role), segments=segments)
        objects.append(o)
        return o
    gap = c["side_and_head_clearance_m"]
    contact = c["stop_contact_z_m"]
    top = leaf_bottom + height + gap
    jamb = .036 if style == "service" else .045
    trim = .048 if style == "service" else .075
    lip = .012 if style == "service" else .014
    stop_depth = .018 if style == "service" else .021
    core_role = "paint" if style == "service" else "wood"
    # Box sections are separate physical joinery/folded-return components.
    # The lip overlaps 8mm into its backing and creates a real stepped rabbet.
    handle_y = leaf_bottom + height*.5 - .025 * height / 2.07
    for side in ("left", "right", "head"):
        def section(name, radial, radial_size, z, z_size, role, bevel=.0015, along=None):
            start,end = along or ((-gap-jamb,width+gap+jamb) if side == "head" else (0,top))
            length = end-start
            if side == "left":
                pos, size = (-gap-radial, (start+end)*.5, z), (radial_size, length, z_size)
            elif side == "right":
                pos, size = (width+gap+radial, (start+end)*.5, z), (radial_size, length, z_size)
            else:
                pos, size = ((start+end)*.5, top+radial, z), (length, radial_size, z_size)
            return b(side+"_"+name, pos, size, role, bevel)
        if side == "head":
            section("jamb_body", jamb*.5, jamb, 0, depth, core_role, .002)
        else:
            # Existing Office hinge barrels extend 13mm beyond the leaf edge.
            # Build genuine pockets from separate solid cheeks/backing instead
            # of placing an uncut jamb through the reused moving hardware.
            if side == "left":
                centers=[leaf_bottom+height*(.5+offset/2.07) for offset in (-.8,0,.8)]
                pockets=[(y-.075*height/2.07,y+.075*height/2.07,.018) for y in centers]
                pocket_depth,notch_z=.018,.016
            else:
                # The reused 130mm latch face reaches 5mm past the core edge.
                # A shallow faceplate rebate clears that last millimetre beyond
                # the 4mm reveal; only the central bolt socket is full depth.
                half_face=.069*height/2.07
                half_socket=.038*height/2.07
                pockets=[(handle_y-half_face,handle_y-half_socket,.002),
                         (handle_y-half_socket,handle_y+half_socket,.016),
                         (handle_y+half_socket,handle_y+half_face,.002)]
                pocket_depth,notch_z=.016,.014
            section("outer_backing", (pocket_depth+jamb)*.5, jamb-pocket_depth,
                    0, depth, core_role, .0015)
            cursor=0
            for n,(start,end,recess) in enumerate(pockets):
                if start > cursor+1e-7:
                    section("cheek_"+str(n), pocket_depth*.5,pocket_depth,0,depth,
                            core_role,.001,along=(cursor,start))
                if recess < pocket_depth:
                    section("pocket_"+str(n)+"_shallow_backing",
                            (recess+pocket_depth)*.5,pocket_depth-recess,0,depth,
                            core_role,.0006,along=(start,end))
                for face in (-1,1):
                    section("pocket_"+str(n)+"_return_"+str(face),
                            recess*.5,recess,
                            face*(depth*.5+notch_z)*.5,depth*.5-notch_z,
                            core_role,.0008,along=(start,end))
                cursor=end
            section("cheek_end",pocket_depth*.5,pocket_depth,0,depth,
                    core_role,.001,along=(cursor,top))
        section("rebated_stop", (.008-lip)*.5, .008+lip,
                contact+.002+stop_depth*.5, stop_depth, core_role, .001)
        # Its frontmost surface is exactly 2mm behind the scaled edge lipping.
        section("compressible_seal", -lip*.48, lip*.64,
                contact+.001, .002, "rubber", .0003)
        for face in (-1, 1):
            face_z = face*(depth*.5+.007)
            section("architrave_"+str(face), trim*.5, trim, face_z, .014,
                    core_role, .002)
            if style == "archive":
                section("outer_moulding_"+str(face), trim-.007, .014,
                        face*(depth*.5+.017), .012, "wood", .003)
                section("inner_bead_"+str(face), .006, .009,
                        face*(depth*.5+.015), .008, "cream", .002)
        # Countersunk fixings sit on the cheek, behind the moving leaf's front face.
        if side != "head":
            sign = -1 if side == "left" else 1
            x = -gap if side == "left" else width+gap
            for n, y in enumerate((.22, top*.5, top-.18)):
                rod(side+"_jamb_screw_"+str(n), (x-sign*.0003,y,-.052),
                    (x+sign*.0015,y,-.052), .0033, "metal", 12)
        else:
            for n, x in enumerate((width*.19,width*.81)):
                rod("head_jamb_screw_"+str(n), (x,top-.0003,-.052),
                    (x,top+.0015,-.052), .0033, "metal", 12)
    threshold_top = c["threshold_top_m"]
    threshold_role = "metal" if style == "service" else "brass"
    b("threshold", (width*.5, threshold_top-.006, 0),
      (width+2*gap, .012, depth+.025), threshold_role, .0025)
    for n, z in enumerate((-.055,-.028,.028,.055)):
        b("threshold_grip_"+str(n), (width*.5,threshold_top-.00015,z),
          (width-.08,.0003,.0018), "rubber", .0001)
    for n, x in enumerate((.08,width-.08)):
        rod("threshold_flush_screw_"+str(n), (x,threshold_top-.002,0),
            (x,threshold_top,0), .0043, "metal", 12)
    # Four separate lips around the latch aperture; dark recessed backing is
    # behind the cheek plane, never an invented active lock/gameplay object.
    inner_x = width+gap
    for name, y, z, sy, sz in (
        ("top",handle_y+.041,0,.020,.036),
        ("bottom",handle_y-.041,0,.020,.036),
        ("front",handle_y,-.014,.062,.008),
        ("rear",handle_y,.014,.062,.008),
    ):
        b("strike_"+name, (inner_x+.0023,y,z), (.0006,sy,sz), "metal", .0002)
    b("strike_socket_shadow", (inner_x+.0163,handle_y,0),
      (.001,.059,.021), "rubber", .0003)
    for n, y in enumerate((handle_y-.041,handle_y+.041)):
        rod("strike_flush_screw_"+str(n), (inner_x+.002,y,0),
            (inner_x+.0043,y,0), .003, "metal", 12)
    c.update(style=style, name=PART_NAMES[style], origin_three_m=list(origin),
             materials=sorted({m.name for o in objects for m in o.data.materials})
             if objects and hasattr(objects[0], "data") else [],
             component_count=len(objects))
    return objects, c


def build():
    """Standalone generation only; no rendering or Unreal launch."""
    import bpy
    from mathutils import Vector
    source = OFFICE / "Signal47_Operations.blend"
    if not source.is_file():
        raise FileNotFoundError("Build the existing Office source first: "+str(source))
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1
    with bpy.data.libraries.load(str(source), link=False) as (available, loaded):
        missing = set(MATERIALS.values()) - set(available.materials)
        if missing:
            raise ValueError("Missing original Office materials: "+str(sorted(missing)))
        loaded.materials = sorted(set(MATERIALS.values()))
    mats = {m.name: m for m in loaded.materials}
    # Keep the original texture files; no new textures or copied graphs on disk.
    for image in bpy.data.images:
        if image.source == "FILE":
            original = OFFICE / "textures" / Path(image.filepath).name
            if not original.is_file():
                raise FileNotFoundError("Missing original Office texture "+str(original))
            image.filepath = "//../Office/textures/" + original.name
    export = bpy.data.collections.new("03_NATIVE_EXPORTS")
    scene.collection.children.link(export)
    out = ROOT / "Export"
    out.mkdir(parents=True, exist_ok=True)
    parts, configs = {}, {}
    active_collection = None
    def active(o):
        bpy.ops.object.select_all(action="DESELECT")
        o.select_set(True)
        bpy.context.view_layer.objects.active = o
    def put(o, name, material):
        o.name = name
        for col in list(o.users_collection):
            col.objects.unlink(o)
        active_collection.objects.link(o)
        o.data.materials.append(material)
        return o
    def xyz(p):
        return Vector((p[0],-p[2],p[1]))
    def box(name, point, size, material, bevel=.0015):
        bpy.ops.mesh.primitive_cube_add(size=1, location=xyz(point))
        o = put(bpy.context.object,name,material)
        o.dimensions = (size[0],size[2],size[1])
        active(o)
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        if bevel:
            mod=o.modifiers.new("Physical_edge_radius","BEVEL")
            mod.width=min(bevel,min(size)*.24)
            mod.segments=3
            bpy.ops.object.modifier_apply(modifier=mod.name)
            mod=o.modifiers.new("Weighted_corner_normals","WEIGHTED_NORMAL")
            mod.keep_sharp=True
            bpy.ops.object.modifier_apply(modifier=mod.name)
        return o
    def cyl(name, a, b, radius, material, segments=16):
        aa,bb=xyz(a),xyz(b)
        d=bb-aa
        bpy.ops.mesh.primitive_cylinder_add(vertices=segments,radius=radius,
            depth=d.length,location=(aa+bb)/2)
        o=put(bpy.context.object,name,material)
        o.rotation_euler=d.to_track_quat("Z","Y").to_euler()
        active(o)
        bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
        for polygon in o.data.polygons:
            polygon.use_smooth=len(polygon.vertices)==4
        return o
    for style,name in PART_NAMES.items():
        active_collection=bpy.data.collections.new("01_EDIT_"+style)
        scene.collection.children.link(active_collection)
        originals,config=append_frame(box,cyl,mats,style=style)
        configs[name]=config
        for o in originals:
            active(o)
            bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        bpy.ops.object.select_all(action="DESELECT")
        for o in originals:
            o.select_set(True)
        bpy.context.view_layer.objects.active=originals[0]
        bpy.ops.object.duplicate()
        copies=list(bpy.context.selected_objects)
        bpy.context.view_layer.objects.active=copies[0]
        bpy.ops.object.join()
        joined=bpy.context.object
        joined.name=name
        for col in list(joined.users_collection):
            col.objects.unlink(joined)
        export.objects.link(joined)
        active(joined)
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.cube_project(cube_size=1)
        bpy.ops.object.mode_set(mode="OBJECT")
        tri=joined.modifiers.new("Triangulated_delivery","TRIANGULATE")
        bpy.ops.object.modifier_apply(modifier=tri.name)
        bounds=[[float(fn(v.co[k] for v in joined.data.vertices)) for k in range(3)]
                for fn in (min,max)]
        if len(joined.data.polygons)>10000:
            raise ValueError("Unexpected frame triangle budget: "+name)
        bpy.ops.export_scene.fbx(filepath=str(out/(name+".fbx")),use_selection=True,
            object_types={"MESH"},apply_unit_scale=True,
            apply_scale_options="FBX_SCALE_UNITS",axis_forward="-Y",axis_up="Z",
            bake_anim=False,mesh_smooth_type="FACE",path_mode="STRIP")
        parts[name]={"triangles":len(joined.data.polygons),"bounds_local_m":bounds,
                     "materials":[m.name for m in joined.data.materials],
                     "original_components":len(originals)}
        joined.hide_render=True
        joined.hide_set(True)
        # Both prototypes share their true hinge origin; show one at a time.
        active_collection.hide_viewport=style=="archive"
        active_collection.hide_render=style=="archive"
    contract={"units":"metres","mapping":"Blender (x,-z,y) to UE (100x,100z,100y)",
              "authorship":"Original Office-compatible beveled joinery and hardware",
              "parts":parts,"frames":configs,"leaf_asset":LEAF_ASSET,
              "office_material_root":MATERIAL_ROOT,
              "office_source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
              "collision":"None; existing runtime doors/proxies retain authority"}
    (ROOT/"model_contract.json").write_text(json.dumps(contract,indent=2)+"\n")
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/"Signal47_DoorFrames.blend"))
    print("S47 DOOR FRAME SOURCE COMPLETE "+json.dumps({n:p["triangles"] for n,p in parts.items()}))


if __name__ == "__main__":
    build()
