"""Original ordinary shelf contents; pure plan and existing box callback only.

Call build(box, MATS) while SM_Office_Dressing is CURRENT. Coordinates are
Three (x, height, z) metres. No bpy, source imports, text, findings or export.
The caller's box returns an object with Blender rotation_euler.z. All boxes
use zero bevel, retaining the predictable twelve-triangle primitive budget.
"""
from math import cos, radians, sin


SHELVES = {
    'middle': {'top': 1.6475, 'ceiling': 2.1425},
    'upper': {'top': 2.1775, 'ceiling': 2.71},
}
RESERVED = (
    {'name': 'existing archive box', 'bounds': ((-.775, 1.65, 8.33), (-.485, 1.96, 8.80))},
    # Conservative full-depth exclusion: do not depend on the private stack's
    # exact X footprint. Its entire given Z band stays free on the upper shelf.
    {'name': 'privateFolderStack', 'bounds': ((-.795, 2.1775, 8.49), (-.485, 2.2325, 8.97))},
)
MATERIALS = ('Paper', 'BinderBlue', 'BinderOlive', 'BinderBurgundy',
             'BinderFadedGray', 'BinderTan')


def plan():
    """Return deterministic editable pieces and measured support/clearance proof."""
    pieces, groups = [], []

    def group(name, shelf, x, z):
        result = {'name': name, 'shelf': shelf, 'x': x, 'z': z,
                  'support_y': SHELVES[shelf]['top'], 'cursor_y': SHELVES[shelf]['top'],
                  'components': []}
        groups.append(result)
        return result

    def piece(g, label, centre, size, material, yaw):
        c, s = cos(yaw), sin(yaw)
        dx, dy, dz = centre
        at = (g['x'] + dx * c + dz * s, dy, g['z'] - dx * s + dz * c)
        half = (abs(c) * size[0] / 2 + abs(s) * size[2] / 2,
                size[1] / 2, abs(s) * size[0] / 2 + abs(c) * size[2] / 2)
        bounds = tuple(tuple(at[k] + sign * half[k] for k in range(3)) for sign in (-1, 1))
        row = {'name': 'Shelf_' + g['name'] + '_' + label, 'centre': at, 'size': size,
               'material': 'M_Office_' + material, 'yaw_radians': yaw, 'bounds': bounds}
        pieces.append(row)
        g['components'].append(row)

    def book(g, index, thickness, width, cover, degrees, offset=0.):
        # Proper hardback/manual construction: two covers, inset page block,
        # rounded-spine substitute with two narrow binding bands, no fake text.
        y, depth, skin = g['cursor_y'], .252, .0018
        yaw = radians(degrees)
        for side, height in (('lower_cover', y + skin / 2), ('upper_cover', y + thickness - skin / 2)):
            piece(g, str(index) + '_' + side, (offset, height, 0), (depth, skin, width), cover, yaw)
        piece(g, str(index) + '_page_block', (offset - .004, y + thickness / 2, 0),
              (depth - .016, thickness - 2 * skin, width - .009), 'Paper', yaw)
        piece(g, str(index) + '_bound_spine', (offset + depth / 2 - .003, y + thickness / 2, 0),
              (.006, thickness, width), cover, yaw)
        for end, z in enumerate((-.32 * width, .32 * width)):
            piece(g, str(index) + '_binding_band_' + str(end),
                  (offset + depth / 2 + .00035, y + thickness / 2, z),
                  (.0007, max(.004, thickness - .006), .003), 'BinderTan', yaw)
        g['cursor_y'] += thickness

    def folder(g, index, thickness, width, degrees, offset=0., cover='BinderTan'):
        y, yaw, depth, skin = g['cursor_y'], radians(degrees), .247, .00065
        for side, height in (('back', y + skin / 2), ('front', y + thickness - skin / 2)):
            piece(g, str(index) + '_' + side, (offset, height, 0), (depth, skin, width), cover, yaw)
        piece(g, str(index) + '_enclosed_pages', (offset - .002, y + thickness / 2, 0),
              (depth - .010, thickness - 2 * skin, width - .010), 'Paper', yaw)
        piece(g, str(index) + '_index_tab', (offset - .036, y + skin / 2, width / 2 + .004),
              (.052, skin, .008), cover, yaw)
        g['cursor_y'] += thickness

    def bundle(g, index, width, depth, degrees, layers=6):
        # A compact loose bundle, with actual layered edges and restrained shifts.
        yaw, base = radians(degrees), g['cursor_y']
        for layer in range(layers):
            height = .0009 if layer % 2 else .0012
            piece(g, str(index) + '_loose_leaf_' + str(layer),
                  ((layer % 3 - 1) * .0012, base + height / 2, (layer % 2) * .0015),
                  (depth - (layer % 3) * .001, height, width - (layer % 2) * .002), 'Paper', yaw)
            base += height
        g['cursor_y'] = base

    # The original carton ends at z8.80; these are two separate work groups
    # beyond its right edge, with a visible gap instead of filling every inch.
    g = group('middle_reference_manuals', 'middle', -.647, 9.035)
    for args in ((0, .027, .263, 'BinderBlue', -2.0, 0.),
                 (1, .034, .242, 'BinderFadedGray', 1.5, -.003),
                 (2, .021, .254, 'BinderBurgundy', -.7, .004)):
        book(g, *args)
    bundle(g, 'top_reading_pages', .215, .230, 1.)
    g = group('middle_pending_folders', 'middle', -.645, 9.492)
    book(g, 0, .025, .244, 'BinderOlive', 2.1)
    folder(g, 'working_folder', .008, .228, -2.2, -.002)
    folder(g, 'returned_folder', .006, .213, .8, .002, 'BinderFadedGray')
    bundle(g, 'sorted_pages', .198, .221, -1.4, 5)

    # Upper work groups lie beyond the entire private-folder Z interval. A
    # short ordinary slip bundle at the opposite end uses the narrow free bay.
    g = group('upper_kept_slips', 'upper', -.646, 8.367)
    bundle(g, 'slips', .094, .225, -1.0, 8)
    g = group('upper_service_volumes', 'upper', -.648, 9.166)
    for args in ((0, .033, .262, 'BinderTan', 1.0, 0.),
                 (1, .026, .251, 'BinderBlue', -1.7, -.003),
                 (2, .022, .240, 'BinderOlive', 2.0, .002),
                 (3, .029, .259, 'BinderBurgundy', -.6, -.001)):
        book(g, *args)
    folder(g, 'insert_folder', .007, .224, 1.5)
    g = group('upper_recent_returns', 'upper', -.643, 9.524)
    folder(g, 'lower_folder', .010, .228, -1.3)
    book(g, 'thin_manual', .018, .224, 'BinderFadedGray', 1.9, -.002)
    bundle(g, 'loose_returns', .207, .227, -.9, 7)

    for g in groups:
        rows = g.pop('components')
        bounds = tuple(tuple(fn(row['bounds'][side][k] for row in rows) for k in range(3))
                       for side, fn in ((0, min), (1, max)))
        low, high = bounds
        assert -.795 <= low[0] < high[0] <= -.485, (g['name'], 'shelf depth', bounds)
        assert 8.30 <= low[2] < high[2] <= 9.70, (g['name'], 'shelf length', bounds)
        assert abs(low[1] - g['support_y']) < 1e-9, (g['name'], 'unsupported stack')
        assert high[1] < SHELVES[g['shelf']]['ceiling'], (g['name'], 'upper clearance')
        gaps = {}
        for reserved in RESERVED:
            a, b = reserved['bounds']
            # Positive separation on one axis suffices for disjoint AABBs.
            gap = max(a[k] - high[k] for k in range(3))
            gap = max(gap, max(low[k] - b[k] for k in range(3)))
            assert gap > 0, (g['name'], 'reserved overlap', reserved['name'])
            gaps[reserved['name']] = gap
        g.update(bounds=bounds, headroom_m=SHELVES[g['shelf']]['ceiling'] - high[1],
                 reserved_separation_m=gaps, components=len(rows))
    triangle_bound = len(pieces) * 12
    assert triangle_bound < 4000
    return {'units': 'Three metres (x,height,z)', 'pieces': pieces, 'groups': groups,
            'component_count': len(pieces), 'unbevelled_box_triangles': triangle_bound,
            'scope': 'Only two existing west shelf tops; no floor, lower binders or live-paper geometry'}


def build(box, materials):
    """Append planned boxes to caller CURRENT; return JSON-compatible metadata."""
    metadata = plan()
    required = {piece['material'] for piece in metadata['pieces']}
    missing = sorted(required - set(materials))
    if missing:
        raise KeyError('Missing existing Office materials: ' + ', '.join(missing))
    for piece in metadata['pieces']:
        obj = box(piece['name'], piece['centre'], piece['size'], materials[piece['material']], 0.)
        obj.rotation_euler.z = piece['yaw_radians']
    return metadata
