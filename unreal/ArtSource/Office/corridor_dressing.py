"""Measured SARO corridor fittings, appended to SM_Office_Room by its caller.

Pure callback library: no bpy, scene reset, import, export, light or collision creation.
Coordinates are Three metres (x, height, z), matching build_office.py helpers.
Source: Annex.ts shell/corridor, exported annex nodes 478/483 and doors south,
records/exit; Operations alone has the native +0.50m front recess.
"""
from math import ceil

MATERIALS = {
    "wall": "M_Office_SageWall", "trim": "M_Office_CreamEnamel",
    "wood": "M_Office_Walnut", "steel": "M_Office_CabinetEnamel",
    "metal": "M_Office_BrushedMetal", "dark": "M_Office_BlackBakelite",
    "tile": "M_Office_FloorLinoleum", "ceiling": "M_Office_CeilingTile",
    "diffuser": "M_Office_Diffuser", "paper": "M_Office_Paper",
    "diffuser_west": "M_Office_CorridorTubeWest", "diffuser_east": "M_Office_CorridorTubeEast",
}

# Apertures are immutable circulation contracts, not decoration-sized approximations.
OPENINGS = (
    {"id": "control", "wall": "north", "span_m": (-2.9, -1.9), "top_m": 2.12, "wall_m": 4.805},
    {"id": "records", "wall": "south", "span_m": (-4.4, -3.4), "top_m": 2.10, "wall_m": 6.598},
    {"id": "fire_exit", "wall": "west", "span_m": (5.22, 6.18), "top_m": 2.12, "wall_m": -6.0},
    {"id": "restrooms", "wall": "east", "span_m": (5.22, 6.18), "top_m": 2.10, "wall_m": 2.0},
    {"id": "operations", "wall": "recessed_south", "span_m": (-.08, 1.0), "top_m": 2.12, "wall_m": 7.10},
)

LIGHTS = tuple({
    "id": ident, "position_three_m": (x, 2.696, 5.7),
    "normal_three": (0, -1, 0), "axis_u_three": (1, 0, 0), "axis_v_three": (0, 0, 1),
    "extent_u_m": 1.24, "extent_v_m": .30, "suggested_lumens": 750,
    "suggested_kelvin": 3800, "circuit": "existing_annex_power",
    "legacy_flood_set": "annex", "legacy_pool_slot": slot,
    "legacy_diffuser_node": node, "legacy_diffuser_geo": geo,
    "integration": "Replace the old native fixture emitter/pool; never add a second simultaneous light.",
} for ident, x, slot, node, geo in (
    ("corridor_west", -3.7, 0, 484, 316), ("corridor_east", .2, 1, 485, 317)))

SIGNS = (
    {"id": "control", "text": "CONTROL ROOM", "position_three_m": (-2.4, 2.405, 4.854), "normal_three": (0, 0, 1), "size_m": (.66, .20), "text_height_cm": 3.3},
    {"id": "records", "text": "RECORDS", "position_three_m": (-3.9, 2.405, 6.548), "normal_three": (0, 0, -1), "size_m": (.57, .20), "text_height_cm": 3.3},
    {"id": "fire_exit", "text": "FIRE EXIT", "position_three_m": (-5.956, 2.55, 5.7), "normal_three": (1, 0, 0), "size_m": (.43, .16), "text_height_cm": 2.7, "retain_original_battery_exit_sign": True},
    {"id": "restrooms", "text": "RESTROOMS", "position_three_m": (1.924, 2.405, 5.7), "normal_three": (-1, 0, 0), "size_m": (.57, .20), "text_height_cm": 3.1},
    {"id": "operations", "text": "OPERATIONS", "position_three_m": (.46, 2.43, 7.052), "normal_three": (0, 0, -1), "size_m": (.65, .20), "text_height_cm": 3.3},
)

# Reserve both lamp bodies and the original displaced maintenance tile/cabling.
CEILING_OPENINGS = ((-4.39, -3.01, 5.47, 5.93), (-.49, .89, 5.47, 5.93),
                    (-2.02, -1.12, 5.53, 6.32))


def _subtract(rect, hole):
    """Return non-overlapping rectangles outside an axis-aligned ceiling opening."""
    x0, x1, z0, z1 = rect
    a, b, c, d = hole
    a, b, c, d = max(x0, a), min(x1, b), max(z0, c), min(z1, d)
    if a >= b or c >= d:
        return [rect]
    return [r for r in ((x0, a, z0, z1), (b, x1, z0, z1),
                        (a, b, z0, c), (a, b, d, z1)) if r[1]-r[0] > .012 and r[3]-r[2] > .012]


def _ceiling_rects(rect):
    pieces = [rect]
    for hole in CEILING_OPENINGS:
        pieces = [p for piece in pieces for p in _subtract(piece, hole)]
    return pieces


def build(box, cyl, curve, materials):
    """Append profiled fittings through Office helpers and return a plain geometry receipt.

    Call before finish('SM_Office_Room'). Materials are the existing full-name MATS
    dictionary. All low trim stays in the original skirt envelope; no pillar,
    collider, leaf, interactive proxy or free-standing obstacle is introduced.
    """
    missing = sorted(set(MATERIALS.values()) - set(materials))
    if missing:
        raise ValueError("Corridor materials missing: " + ", ".join(missing))
    parts = []

    def b(name, at, size, material, bevel=.002):
        if min(size) <= 0:
            raise ValueError("Nonpositive corridor component: " + name)
        name = "Corridor_" + name
        parts.append({"name": name, "at_m": tuple(at), "size_m": tuple(size), "material": MATERIALS[material]})
        return box(name, at, size, materials[MATERIALS[material]], bevel)

    def screw(name, a, endpoint):
        cyl("Corridor_" + name, a, endpoint, .0035, materials[MATERIALS["metal"]], 10)

    def panel_x(name, x0, x1, z, facing):
        # Two-tone lower painted wall; the old upper plaster and its notices remain.
        b(name + "_lower_paint", ((x0+x1)/2, .64, z), (x1-x0, 1.08, .012), "wall")
        b(name + "_skirt_body", ((x0+x1)/2, .067, z+facing*.002), (x1-x0, .134, .022), "dark", .004)
        b(name + "_skirt_top_bead", ((x0+x1)/2, .140, z+facing*.001), (x1-x0, .014, .020), "trim", .003)
        b(name + "_dado_cap", ((x0+x1)/2, 1.198, z+facing*.001), (x1-x0, .032, .020), "trim", .003)

    # Original north/south skirt faces: z4.830 and z6.575. Do not project beyond
    # those faces into the enlarged z4.84..6.57 movement rectangle.
    for i, (a, c) in enumerate(((-5.96, -2.94), (-1.86, 1.96))):
        panel_x("North_%d" % i, a, c, 4.817, 1)
    for i, (a, c) in enumerate(((-5.96, -4.45), (-3.35, -.96))):
        panel_x("Records_front_%d" % i, a, c, 6.591, -1)
    # New Operations wall faces, outside its opening; hardware remains Office-owned.
    for i, (a, c) in enumerate(((-.82, -.10), (1.02, 1.75))):
        panel_x("Operations_recess_%d" % i, a, c, 7.111, -1)

    # Return trims outside west/east apertures; the exit hinge/leaf stays untouched.
    for side, x in (("West", -5.986), ("East", 1.986)):
        for i, (a, c) in enumerate(((4.83, 5.17), (6.23, 6.57))):
            b(side+"_skirt_%d" % i, (x, .067, (a+c)/2), (.022, .134, c-a), "dark", .004)
            b(side+"_cap_%d" % i, (x, .140, (a+c)/2), (.020, .014, c-a), "trim", .003)

    # Low covered cable route with a deliberate portal bypass. All strips lie
    # behind the north skirt face; the rise and crosspiece avoid the open door.
    for i, (a, c) in enumerate(((-5.90, -2.95), (-1.85, 1.86))):
        b("Low_raceway_%d" % i, ((a+c)/2, .235, 4.819), (c-a, .060, .020), "trim", .003)
        b("Raceway_lid_seam_%d" % i, ((a+c)/2, .251, 4.830), (c-a-.03, .0015, .001), "dark", 0)
    for x in (-2.95, -1.85):
        b("Portal_raceway_riser_%s" % x, (x, 1.245, 4.819), (.050, 2.08, .020), "trim", .003)
    b("Portal_raceway_bridge", (-2.4, 2.285, 4.819), (1.15, .060, .020), "trim", .003)
    b("Low_raceway_junction", (-.64, .235, 4.820), (.125, .110, .022), "steel", .004)
    for x in (-.686, -.594):
        screw("Junction_screw_%s" % x, (x, .235, 4.832), (x, .235, 4.834))
    curve("Corridor_Outlet_flex_tail", [(-.49,.235,4.825),(-.45,.235,4.825),(-.43,.29,4.825),(-.43,.34,4.825)], .003, materials[MATERIALS["dark"]])
    b("Outlet_plate", (-.43, .375, 4.818), (.080,.100,.020), "trim", .003)

    # Outer profiled surrounds stop outside every measured opening. No stop,
    # gasket or narrowing jamb is added to the moving door's clearance envelope.
    for ident, a, c, top, z, facing in (("Control",-2.9,-1.9,2.12,4.818,1),
                                       ("Records",-4.4,-3.4,2.10,6.588,-1)):
        for x in (a-.037, c+.037):
            b(ident+"_surround_leg_%s" % x, (x,top/2,z), (.060,top,.024), "wood", .004)
            b(ident+"_surround_bead_%s" % x, (x,top/2,z+facing*.013), (.030,top,.008), "trim", .002)
        b(ident+"_surround_head", ((a+c)/2,top+.040,z), (c-a+.134,.060,.024), "wood", .004)
        b(ident+"_surround_head_bead", ((a+c)/2,top+.040,z+facing*.013), (c-a+.105,.028,.008), "trim", .002)
    for ident, x, facing in (("Exit",-5.985,1),("Restroom",1.955,-1)):
        for z in (5.18,6.22):
            b(ident+"_steel_surround_%s" % z, (x,1.065,z), (.025,2.13,.060), "steel", .004)
        b(ident+"_steel_header", (x,2.17,5.7), (.025,.060,1.16), "steel", .004)

    # 30cm 1986 VCT modules, a millimetre below Office's existing tile tops.
    # The existing floor remains the visible recessed grout, with no raised step.
    for ix in range(ceil(8.0/.3)):
        a, c = -6+ix*.3, min(2.,-6+(ix+1)*.3)
        for iz in range(6):
            z0,z1=4.8+iz*.3,4.8+(iz+1)*.3
            b("VCT_%02d_%02d" % (ix,iz), ((a+c)/2,.0025,(z0+z1)/2), (c-a-.002,.005,z1-z0-.002), "tile", .0006)
    b("Bay_floor_joint", (.475,.0025,6.605), (2.59,.005,.012), "tile", .0005)

    # Suspended acoustic ceiling, cut around both lamps and the real wiring hatch.
    for ix in range(ceil(8.0/.6)):
        a,c=-6+ix*.6,min(2.,-6+(ix+1)*.6)
        for iz in range(3):
            z0,z1=4.8+iz*.6,4.8+(iz+1)*.6
            for n,(l,r,near,far) in enumerate(_ceiling_rects((a+.012,c-.012,z0+.012,z1-.012))):
                b("Acoustic_%02d_%d_%d" % (ix,iz,n), ((l+r)/2,2.781,(near+far)/2), (r-l,.022,far-near), "ceiling", .001)
    # Narrow rectangular strips use the same clipping, so neither a lamp nor the
    # displaced maintenance tile receives a spurious grid bar through its centre.
    strips=[(-6+i*.6-.012,-6+i*.6+.012,4.8,6.6) for i in range(14)]
    strips += [(1.988,2.012,4.8,6.6)]
    strips += [(-6,2,4.8+i*.6-.012,4.8+i*.6+.012) for i in range(4)]
    for i,rect in enumerate(strips):
        for n,(a,c,z0,z1) in enumerate(_ceiling_rects(rect)):
            b("Ceiling_T_flange_%d_%d" % (i,n), ((a+c)/2,2.767,(z0+z1)/2), (c-a,.010,z1-z0), "trim", .001)
            sx,sz=c-a,z1-z0
            if sx<sz:
                sx=.006
            else:
                sz=.006
            b("Ceiling_T_web_%d_%d" % (i,n), ((a+c)/2,2.785,(z0+z1)/2), (sx,.026,sz), "steel", .0008)

    for lamp in LIGHTS:
        x,_,z=lamp["position_three_m"]; ident=lamp["id"]
        b(ident+"_ceiling_backplate", (x,2.795,z), (1.38,.010,.46), "steel", .003)
        for end in (-1,1):
            b(ident+"_endcap_%d" % end, (x+end*.657,2.755,z), (.045,.090,.405), "trim", .006)
        for edge in (-1,1):
            b(ident+"_side_rail_%d" % edge, (x,2.755,z+edge*.190), (1.29,.065,.025), "steel", .004)
            b(ident+"_opal_lens_%d" % edge, (x,2.715,z+edge*.085), (1.24,.018,.145),
              "diffuser_west" if lamp["legacy_pool_slot"] == 0 else "diffuser_east", .007)
            screw(ident+"_cover_screw_%d" % edge, (x+edge*.59,2.709,z), (x+edge*.59,2.707,z))

    # Fixed enamel wayfinding plates; text is supplied natively at SIGNS anchors.
    # FIRE EXIT sits above, rather than over, the functioning battery EXIT sign.
    for sign in SIGNS:
        x,y,z=sign["position_three_m"]; nx,_,nz=sign["normal_three"]; w,h=sign["size_m"]
        depth=.012
        size=(w,h,depth) if nz else (depth,h,w)
        at=(x-nx*.009,y,z-nz*.009)
        b(sign["id"]+"_sign_back", at, size, "dark", .005)
        inset=(w-.014,h-.014,.003) if nz else (.003,h-.014,w-.014)
        b(sign["id"]+"_sign_enamel", (x-nx*.003,y,z-nz*.003), inset, "paper", .003)

    return {"components": parts, "lights": LIGHTS, "signs": SIGNS,
            "floor_top_m": .005, "ceiling_underside_m": 2.770,
            "collision": "none; caller retains original walk rectangles, colliders and moving doors",
            "pending_native": "Bind sign text and replace existing lamp sources from LIGHTS; do not duplicate live lights."}
