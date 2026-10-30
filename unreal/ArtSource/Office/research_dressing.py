"""Editable research wall and incoming folders for the existing Dressing part.

Call build(box, cyl, curve, MATS) before finish('SM_Office_Dressing'). Retire the
old Noticeboard/Pinned_shift_sheet block in the caller: this is its replacement.
Helpers append to the caller's CURRENT collection; this module never finishes,
exports, imports bpy, or creates a ninth native asset type.

All dimensions use the project's Three (x, height, z) metres. SURFACES are actual
front-face art anchors, viewed from the room towards +X. Their local horizontal
direction is +Z before roll. Finished art bindings are required separately;
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
    # Older full sheets sit behind the original nine. Foreground contact prints
    # and short paper fragments bridge their edges without concealing the map.
    # These are existing routine copies and undecided observations, not findings.
    _surface('SkyWatchFlyer', 1.662, 2.205, 8.92, .27, .36, 2.0, '1984 community sky-watch flyer'),
    _surface('SkyLogLower', 1.661, 1.27, 8.77, .23, .28, -2.4, 'existing sky observation note'),
    _surface('ServiceLogLower', 1.662, 1.31, 9.39, .23, .33, 1.8, '1986 local service copy'),
    _surface('RadioSchedule', 1.662, 1.81, 8.33, .25, .33, -3.0, '1986 public radio schedule'),
    _surface('HandoverCopy', 1.662, 2.13, 8.25, .16, .23, 3.5, '1986 operations handover'),
    _surface('RadioQuietStrip', 1.662, 2.315, 9.65, .46, .15, -1.0, 'reference sign print'),
    _surface('MaintenanceTag', 1.647, 1.46, 9.88, .16, .24, -4.0, 'unfilled maintenance tag'),
    _surface('ServiceRequest', 1.662, 1.70, 10.035, .20, .31, 3.0, 'blank service request'),
    _surface('ReceiverPhoto', 1.649, 1.91, 9.84, .28, .20, -3.0, 'ordinary receiver reference photo'),
    _surface('ArrayReversePhoto', 1.649, 1.94, 8.63, .28, .20, 4.0, 'ordinary dish reference photo'),
    _surface('RoadPhotoLower', 1.649, 1.415, 8.23, .15, .15, -5.0, 'ordinary service road print'),
    _surface('DishPhotoHeader', 1.649, 2.315, 9.25, .24, .15, 3.0, 'ordinary array contact print'),
    _surface('IndexSlip', 1.648, 1.45, 9.14, .20, .11, -3.0, 'existing working-index fragment'),
    _surface('ClippingsStrip', 1.649, 2.30, 8.51, .34, .13, -2.0, 'existing newspaper heading fragment'),
    _surface('ScheduleRows', 1.646, 2.11, 9.38, .22, .17, 2.0, 'ordinary schedule working copy'),
    _surface('SkySketch', 1.647, 1.48, 8.66, .24, .16, 3.0, 'existing skylog pencil sketch'),
    _surface('MapDetailLeft', 1.647, 1.78, 8.54, .20, .15, 2.0, 'existing array-map detail'),
    _surface('ServiceLogMargin', 1.646, 1.96, 10.03, .18, .13, -4.0, 'ordinary service-log excerpt'),
    _surface('NotePencilFragment', 1.647, 1.20, 9.55, .18, .11, 2.0, 'existing handwritten note fragment'),
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

    # Rear desktop now uses the existing native telephone and case-file props.
