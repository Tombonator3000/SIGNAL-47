"""Editable research wall and incoming folders for the existing Dressing part.

Call build(box, cyl, curve, MATS) before finish('SM_Office_Dressing'). Retire the
old Noticeboard/Pinned_shift_sheet block in the caller: this is its replacement.
Helpers append to the caller's CURRENT collection; this module never finishes,
exports, imports bpy, or creates a ninth native asset type.

All dimensions use the project's Three (x, height, z) metres. SURFACES are actual
front-face art anchors, viewed from the room towards +X. Their local horizontal
direction is +Z before roll. Nine finished art bindings are required separately;
plain paper geometry is not visual acceptance of the research wall.
"""
from math import cos, radians, sin


def _surface(name, body_x, height, z, width, paper_height, roll, kind, thickness=.0006):
    angle = radians(roll)
    return {
        'name': 'office:research:' + name,
        'component': 'Research_sheet_' + name,
        'art_asset': 'T_OfficeResearch_' + name,
        'centre': (body_x - thickness / 2 - .0005, height, z),
        'body_centre': (body_x, height, z),
        'width_m': width,
        'height_m': paper_height,
        'depth_m': thickness,
        'normal_axis': (-1, 0, 0),
        'right_axis': (0, sin(angle), cos(angle)),
        'up_axis': (0, cos(angle), -sin(angle)),
        'roll_degrees': roll,
        'kind': kind,
        'requires_finished_art': True,
    }


# The central map is below the contact strip and the small field note at right.
# These deliberate overlaps are depth-separated; art planes use the same order.
SURFACES = (
    _surface('ArrayMap', 1.660, 1.74, 9.18, .61, .44, -2.3, 'working array map'),
    _surface('ClippingLeft', 1.660, 2.11, 8.47, .40, .29, 3.5, 'newspaper clipping', .00025),
    _surface('ClippingRight', 1.659, 2.14, 9.78, .39, .28, -4.0, 'newspaper clipping', .00025),
    _surface('FieldPhoto', 1.653, 1.69, 8.77, .25, .20, -3.0, 'field photograph'),
    _surface('PhotoPair', 1.660, 1.30, 8.46, .31, .23, 4.0, 'photo and caption card'),
    _surface('WorkingNote', 1.659, 1.26, 9.08, .25, .24, -5.0, 'handwritten working sheet'),
    _surface('FieldCard', 1.651, 1.64, 9.54, .32, .35, 2.0, 'field note card'),
    _surface('ContactStrip', 1.651, 2.01, 9.18, .35, .16, -1.5, 'photographic contact strip'),
    _surface('ShiftNote', 1.658, 1.29, 9.78, .32, .21, 3.0, 'folded shift note'),
)


def build(box, cyl, curve, materials):
    """Append only authored wall/folder geometry using existing material objects.

    materials is the existing build_office.MATS mapping. No labels, new findings
    or encounter clues are invented in the geometry; root binds reviewed art.
    The six live desktop papers and the two west-shelf papers remain separate.
    """
    def material(suffix):
        return materials['M_Office_' + suffix]

    wood = material('Walnut')
    cork = material('Cork')
    paper = material('Paper')
    brass = material('AgedBrass')
    steel = material('BrushedMetal')
    dark = material('BlackBakelite')
    thread = material('BinderBurgundy')
    blue = material('BinderBlue')

    # Wall inner surface is x=1.78. Rear spacers touch it; the board is supported
    # above the existing radiator (top about .675), not balanced on its pipework.
    board_y, board_z = 1.76, 9.16
    board_height, board_width = 1.34, 2.06
    for y in (1.17, 2.35):
        for z in (8.24, 10.08):
            box('Research_board_wall_spacer', (1.763, y, z), (.034, .065, .075), dark, .003)
    box('Research_board_backing', (1.716, board_y, board_z),
        (.062, board_height, board_width), wood, .004)
    box('Research_board_cork', (1.676, board_y, board_z),
        (.025, board_height - .06, board_width - .06), cork, .003)
    for y in (board_y - board_height / 2, board_y + board_height / 2):
        box('Research_board_horizontal_frame', (1.711, y, board_z),
            (.102, .035, board_width + .035), wood, .004)
    for z in (board_z - board_width / 2, board_z + board_width / 2):
        box('Research_board_vertical_frame', (1.711, board_y, z),
            (.102, board_height, .035), wood, .004)
        for y in (1.15, 2.37):
            cyl('Research_board_frame_screw', (1.661, y, z),
                (1.657, y, z), .0033, brass, 12)

    def sheet_point(surface, right, up, x=None):
        centre = surface['body_centre']
        r, u = surface['right_axis'], surface['up_axis']
        return (centre[0] if x is None else x,
                centre[1] + right * r[1] + up * u[1],
                centre[2] + right * r[2] + up * u[2])

    pins = {}
    for surface in SURFACES:
        sheet = box(surface['component'], surface['body_centre'],
                    (surface['depth_m'], surface['height_m'], surface['width_m']), paper, .00025)
        # Around Blender X: negative angle gives the declared Three right/up axes.
        sheet.rotation_euler.x = -radians(surface['roll_degrees'])
        key = surface['name'].split(':')[-1]
        pin = sheet_point(surface, -.12 * surface['width_m'], .43 * surface['height_m'],
                          surface['centre'][0] - .008)
        cyl('Research_pin_shaft_' + key, (1.677, pin[1], pin[2]), pin, .0012, steel, 12)
        cyl('Research_pin_head_' + key, pin, (pin[0] - .0025, pin[1], pin[2]), .0052, brass, 16)
        pins[key] = (pin[0] - .003, pin[1], pin[2])
        # Two lower corners of larger sheets are held against the cork. Small
        # cards retain a free lower edge; no repeated fake printed-line geometry.
        if surface['width_m'] >= .38:
            for sign in (-1, 1):
                tape = box('Research_paper_corner_tab',
                           sheet_point(surface, sign * .43 * surface['width_m'],
                                       -.42 * surface['height_m'], surface['centre'][0] - .001),
                           (.0008, .022, .032), cork, .0001)
                tape.rotation_euler.x = -radians(surface['roll_degrees'])

    # Three restrained, slightly sagging links between actual drawing pins.
    # They encode no new conclusion or encounter evidence.
    for index, (left, right) in enumerate((('ClippingLeft', 'ArrayMap'),
                                         ('ArrayMap', 'FieldCard'),
                                         ('FieldPhoto', 'WorkingNote'))):
        a, b = pins[left], pins[right]
        middle = (min(a[0], b[0]) - .001, (a[1] + b[1]) / 2 - .012,
                  (a[2] + b[2]) / 2)
        curve('Research_thread_' + str(index), [a, middle, b], .00085, thread)

    # Incoming field folders occupy only the rear-left tabletop margin. The
    # closest paper edge stays behind z=9.95, clear of all six interactive sheets,
    # the cup at (.91,9.97), pencils around (.67,10.0), and lamp at (-.24,9.70).
    # No floor dressing or west-shelf additions enter the access/private zones.
    def folder_piece(name, centre, offset, size, mat, yaw, bevel=.0008):
        c, s = cos(yaw), sin(yaw)
        dx, dy, dz = offset
        at = (centre[0] + dx * c + dz * s, centre[1] + dy,
              centre[2] - dx * s + dz * c)
        piece = box(name, at, size, mat, bevel)
        piece.rotation_euler.z = yaw
        return piece

    for index, (height, yaw_degrees, width, depth, cover) in enumerate((
            (.802, -3.0, .35, .14, cork),
            (.818, 2.0, .34, .135, blue),
            (.833, -.5, .33, .13, cork))):
        centre = (.22 + index * .004, height, 10.035 - index * .002)
        yaw = radians(yaw_degrees)
        prefix = 'Incoming_field_folder_' + str(index)
        folder_piece(prefix + '_lower_cover', centre, (0, 0, 0),
                     (width, .0012, depth), cover, yaw)
        folder_piece(prefix + '_fold', centre, (-width / 2 + .003, .006, 0),
                     (.006, .013, depth), cover, yaw, .0015)
        for layer in range(4):
            folder_piece(prefix + '_page_edge_' + str(layer), centre,
                         (.002 + (layer % 2) * .001, .002 + layer * .002, 0),
                         (width - .016, .0007, depth - .009 - (layer % 2) * .002), paper, yaw, .0001)
        folder_piece(prefix + '_upper_cover', centre, (.001, .011, 0),
                     (width - .004, .0012, depth - .003), cover, yaw)
        folder_piece(prefix + '_index_tab', centre, (.075, .0105, depth / 2 - .001),
                     (.052, .0015, .012), cover, yaw, .0004)
    box('Incoming_folder_clip_back', (.348, .846, 9.990), (.025, .008, .012), dark, .001)
    for x in (.339, .357):
        curve('Incoming_folder_clip_wire', [(x, .847, 9.986), (x, .861, 9.994),
                                            (x, .861, 10.009), (x, .847, 10.010)], .0012, steel)
