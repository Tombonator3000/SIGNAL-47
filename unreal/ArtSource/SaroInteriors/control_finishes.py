"""Independent control-room finishes, retaining the existing hex floor and equipment.

blender -b -t 2 --python control_finishes.py
python3 control_finishes.py --check
Only new Control assets are written. Office PBR sources remain read-only.
"""
from pathlib import Path
import copy
import json
import math
import sys

ROOT = Path(__file__).resolve().parent
OFFICE = ROOT.parent / "Office"
NAME = "SM_Saro_ControlRoomFinishes"
REUSE = {name: "/Game/Signal47/Art/Office/Materials/"+name for name in (
    "M_Office_CreamEnamel", "M_Office_BrushedMetal", "M_Office_BlackBakelite")}
FIXTURES = tuple((x, z) for x in (-3.5, 0, 3.5) for z in (-1.8, 1.8))


def material_rows():
    source = {r["name"]: r for r in json.loads((OFFICE/"materials.json").read_text())}
    choices = (
        ("M_Saro_Control_UpperPaint", "M_Office_SageWall", (.350, .385, .365), (.255, .282, .250)),
        ("M_Saro_Control_LowerPaint", "M_Office_SageWall", (.105, .155, .155), (.255, .282, .250)),
        ("M_Saro_Control_Acoustic", "M_Office_CeilingTile", (.500, .510, .470), (.46, .47, .43)),
        ("M_Saro_Control_RackSteel", "M_Office_CabinetEnamel", (.052, .066, .063), (.19, .23, .21)),
    )
    rows = []
    for name, original, target, color in choices:
        row = copy.deepcopy(source[original])
        row["name"] = name
        row["texture_tint_linear"] = [round(a/b, 7) for a, b in zip(target, color)]
        row["textures"] = {role: "../Office/"+path for role, path in row["textures"].items()}
        row["source_material"] = original
        row["source_use"] = "Read-only approved Office PBR, separate Control material"
        if name.endswith("RackSteel"):
            row["metallic"] = .25
        rows.append(row)
    return rows


def subtract(rect, hole):
    a, b, lo, hi = rect
    c, d, bot, top = hole
    c, d, bot, top = max(a, c), min(b, d), max(lo, bot), min(hi, top)
    if c >= d or bot >= top:
        return [rect]
    return [r for r in ((a, c, lo, hi), (d, b, lo, hi),
                         (c, d, lo, bot), (c, d, top, hi))
            if r[1]-r[0] > .003 and r[3]-r[2] > .003]


def layout():
    parts = []

    def quad(name, points, material):
        parts.append({"name": name, "points_three_m": points, "material": material})

    def box(name, at, size, material, bevel=.001):
        parts.append({"name": name, "at_three_m": at, "size_three_m": size,
                      "material": material, "bevel_m": bevel})

    def wall(name, rects, axis, plane, facing, material, holes=()):
        for i, rect in enumerate(rects):
            pieces = [rect]
            for hole in holes:
                pieces = [p for piece in pieces for p in subtract(piece, hole)]
            for j, (a, b, lo, hi) in enumerate(pieces):
                points = ([(a, lo, plane), (b, lo, plane), (b, hi, plane), (a, hi, plane)]
                          if axis == "z" else
                          [(plane, lo, b), (plane, lo, a), (plane, hi, a), (plane, hi, b)])
                quad(name+"_%d_%d" % (i, j), points if facing > 0 else list(reversed(points)), material)

    upper, lower = "M_Saro_Control_UpperPaint", "M_Saro_Control_LowerPaint"
    # Face positions are 3..7mm in front of the old wall faces. At the dado,
    # the original cap remains proud of the new paint rather than buried by it.
    wall("South_lower", [(-5.995, -2.935, .015, .944), (-1.865, 5.995, .015, .944)],
         "z", 4.477, -1, lower, holes=((3.582, 3.658, .2395, .3605),))
    # The bulletin board, Halley poster and their paper faces remain in front.
    # Reserved holes preserve the much flatter CORRIDOR sign, numbers and fund.
    south_holes = ((-2.653, -2.147, 2.307, 2.493),
                   (-1.363, -1.137, 1.472, 1.741),
                   (-3.273, -3.127, 1.082, 1.158),
                   (4.294, 4.466, 1.363, 1.489))
    wall("South_upper", [(-5.995, -2.935, 1.012, 2.995),
                          (-1.865, 5.995, 1.012, 2.995),
                          (-2.935, -1.865, 2.205, 2.995)],
         "z", 4.493, -1, upper, holes=south_holes)

    wall("West_lower", [(-4.49, 4.49, .015, .944)], "x", -5.977, 1, lower)
    wall("West_upper", [(-4.49, 4.49, 1.012, 2.995)], "x", -5.993, 1, upper)
    wall("East_lower", [(-4.49, 1.065, .015, .944), (2.135, 4.49, .015, .944)],
         "x", 5.977, -1, lower)
    wall("East_upper", [(-4.49, 1.065, 1.012, 2.995), (2.135, 4.49, 1.012, 2.995),
                         (1.065, 2.135, 2.205, 2.995)], "x", 5.993, -1, upper)
    # Real 10.8m-wide opening, sill and mullions stay completely uncovered.
    wall("North_below_sill", [(-5.995, 5.995, .015, .913)], "z", -4.493, 1, lower)
    wall("North_ends_head", [(-5.995, -5.445, .913, 2.995),
                             (5.445, 5.995, .913, 2.995),
                             (-5.445, 5.445, 2.805, 2.995)], "z", -4.493, 1, upper)

    # Thin acoustic finish at y2.997: the original leak overlays at2.996 remain
    # visible below it. Clip the panel grid around all six actual fixture bodies.
    ceiling_holes = [(x-.665, x+.665, z-.195, z+.195) for x, z in FIXTURES]

    def ceiling_pieces(rect):
        pieces = [rect]
        for hole in ceiling_holes:
            pieces = [p for piece in pieces for p in subtract(piece, hole)]
        return pieces

    for ix in range(20):
        for iz in range(15):
            x0, x1 = -6+ix*.6+.006, -6+(ix+1)*.6-.006
            z0, z1 = -4.5+iz*.6+.006, -4.5+(iz+1)*.6-.006
            for k, (a, b, c, d) in enumerate(ceiling_pieces((x0, x1, z0, z1))):
                quad("Acoustic_%02d_%02d_%d" % (ix, iz, k),
                     [(a, 2.997, c), (b, 2.997, c), (b, 2.997, d), (a, 2.997, d)],
                     "M_Saro_Control_Acoustic")
    strips = [(-6+i*.6-.005, -6+i*.6+.005, -4.49, 4.49) for i in range(1, 20)]
    strips += [(-5.99, 5.99, -4.5+i*.6-.005, -4.5+i*.6+.005) for i in range(1, 15)]
    for i, rect in enumerate(strips):
        for k, (a, b, c, d) in enumerate(ceiling_pieces(rect)):
            box("Ceiling_grid_%02d_%d" % (i, k), ((a+b)/2, 2.995, (c+d)/2),
                (b-a, .004, d-c), "M_Office_CreamEnamel", .0006)

    # The console top stays at .780, below all original live paper/card surfaces.
    # A rolled protective edge sits inside the existing collider z<=-3.42.
    box("Console_front_rolled_edge", (0, .755, -3.447), (6.90, .035, .010),
        "M_Office_BrushedMetal", .003)
    for x in (-3.448, 3.448):
        box("Console_end_lip_%s" % x, (x, .755, -3.9), (.008, .035, .892),
            "M_Office_BrushedMetal", .002)
    # Only the RX rack's outer rails gain depth. LED/status rows, power lever,
    # live service-record sheet, DSP, printer, telex and all screens stay intact.
    for z in (-2.699, -2.101):
        box("RX_outer_rail_%s" % z, (5.341, .975, z), (.012, 1.90, .018),
            "M_Saro_Control_RackSteel", .002)
        for y in (.075, .605, 1.305, 1.875):
            box("RX_rail_screw_%s_%s" % (z, y), (5.333, y, z), (.002, .006, .006),
                "M_Office_BrushedMetal", .0007)
    for y in (.025, 1.925):
        box("RX_rail_crosspiece_%s" % y, (5.341, y, -2.4), (.012, .018, .58),
            "M_Saro_Control_RackSteel", .002)
    # Saddles finish the existing east-wall conduit without adding another cable.
    for y in (2.20, 2.66):
        box("Conduit_saddle_%s" % y, (5.953, y, -2.25), (.048, .035, .068),
            "M_Office_BrushedMetal", .001)
    return parts


if __name__ == "__main__":
    if "--check" in sys.argv:
        parts = layout()
        assert len({p["name"] for p in parts}) == len(parts)
        assert all(min(p["size_three_m"]) > 0 for p in parts if "size_three_m" in p)
        assert all((ROOT/path).is_file() for row in material_rows() for path in row["textures"].values())
        print("Control layout:", len(parts), "components; hex floor, windows and equipment retained.")
    else:
        sys.path.insert(0, str(ROOT))
        from control_mesh import build
        build(ROOT, OFFICE, NAME, layout(), material_rows(), REUSE)
