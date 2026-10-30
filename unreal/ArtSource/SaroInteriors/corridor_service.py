"""Flush, non-interactive SARO corridor service fittings in Three metres.

Pure layout data for corridor_finishes.layout(), with no Blender dependency.
The return grille sits between the Control Room surround and the noticeboard;
the low coupling closes an existing raceway cover joint. Nothing projects into
the z4.84..6.57 walking strip or adds a new clue, sign, collider or light.
"""

MATERIALS = (
    "M_Office_CabinetEnamel",
    "M_Office_CreamEnamel",
    "M_Office_BrushedMetal",
    "M_Office_BlackBakelite",
)


def service_parts():
    """Return ordinary wall-supported boxes/quads understood by the caller."""
    parts = []
    enamel, cream, metal, dark = MATERIALS

    def box(name, at, size, material, bevel=.0007):
        parts.append({"name": "Service_" + name,
                      "at_three_m": tuple(at), "size_three_m": tuple(size),
                      "material": material, "bevel_m": bevel})

    def chip(name, x, y, width, height, z):
        # Exposed metal stays on its supporting door skin, never in free space.
        parts.append({"name": "Service_" + name,
                      "points_three_m": [(x-width*.5, y-height*.5, z),
                                         (x+width*.5, y-height*.30, z),
                                         (x+width*.34, y+height*.5, z),
                                         (x-width*.32, y+height*.24, z)],
                      "material": metal})

    # Recessed 48x56cm return/filter access cover. Its back intersects the
    # retained wall; its front-most edge is z4.83899, behind the clear zone.
    x, y = -1.54, 1.61
    box("Return_frame_back", (x, y, 4.8355), (.480, .560, .0030), enamel)
    box("Return_door_reveal", (x, y, 4.8371), (.452, .532, .0006), dark, 0)
    box("Return_door_face", (x, y, 4.8374), (.446, .526, .0008), enamel, .00015)
    for side in (-1, 1):
        box("Return_side_rim_%d" % side, (x+side*.232, y, 4.8380),
            (.012, .554, .0017), cream, .0003)
        box("Return_end_rim_%d" % side, (x, y+side*.272, 4.8380),
            (.458, .012, .0017), cream, .0003)

    # Nine broad louvres read as a real service grille from the corridor view.
    # The dark interior is behind their lips, not a sticker on the painted wall.
    box("Return_dark_vent_interior", (x, 1.620, 4.8379),
        (.370, .282, .0004), dark, 0)
    for i in range(9):
        height = 1.500+i*.030
        box("Return_louvre_%02d" % i, (x, height, 4.8382),
            (.360, .016, .0014), enamel, .00025)
        box("Return_louvre_lip_%02d" % i, (x, height-.007, 4.8389),
            (.356, .0012, .00015), metal, 0)

    # A slotted latch and two short hinge straps explain how the face opens.
    box("Return_latch_plate", (x+.160, 1.405, 4.8382),
        (.037, .035, .0014), metal, .0005)
    box("Return_latch_slot", (x+.160, 1.405, 4.83895),
        (.020, .0017, .00008), dark, 0)
    for i, height in enumerate((1.43, 1.79)):
        box("Return_hinge_strap_%d" % i, (x-.218, height, 4.8382),
            (.018, .044, .0014), metal, .0003)
    for i, (dx, dy) in enumerate(((-.215, -.252), (.215, -.252),
                                 (-.215, .252), (.215, .252))):
        box("Return_retaining_screw_%d" % i, (x+dx, y+dy, 4.8383),
            (.007, .007, .0012), metal, .0005)
        box("Return_screw_slot_%d" % i, (x+dx, y+dy, 4.83895),
            (.0044, .0008, .00008), dark, 0)
    chip("Return_lower_door_edge_worn", x-.150, 1.349, .031, .006, 4.83784)
    chip("Return_latch_edge_worn", x+.167, 1.386, .019, .004, 4.83784)
    chip("Return_upper_hinge_edge_worn", x-.210, 1.815, .012, .010, 4.83784)

    # Existing north raceway centre is y.235, front z4.829. This cover patch
    # overlaps that retained cover by 1mm and ends before the portal riser.
    box("Raceway_replacement_cover", (-3.185, .235, 4.8325),
        (.240, .061, .009), cream, .0010)
    for side in (-1, 1):
        box("Raceway_cover_coupling_%d" % side,
            (-3.185+side*.117, .235, 4.8375),
            (.008, .070, .0024), metal, .00045)
        box("Raceway_coupling_slot_%d" % side,
            (-3.185+side*.117, .235, 4.83876),
            (.004, .0012, .00008), dark, 0)
    box("Raceway_upper_lid_seam", (-3.185, .256, 4.8373),
        (.218, .0018, .0006), dark, 0)
    chip("Raceway_cover_edge_worn", -3.222, .263, .032, .004, 4.83704)

    # Compact layout invariant, independent of Blender or Unreal availability.
    assert len({part["name"] for part in parts}) == len(parts)
    for part in parts:
        if "at_three_m" in part:
            at, size = part["at_three_m"], part["size_three_m"]
            assert min(size) > 0
            assert at[2]+size[2]*.5 < 4.84
        else:
            assert max(point[2] for point in part["points_three_m"]) < 4.84
    return parts


if __name__ == "__main__":
    print("Corridor service fittings:", len(service_parts()),
          "wall-supported components, no walking-zone intrusion")
