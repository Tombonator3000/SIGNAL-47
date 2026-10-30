"""Original 1986 records-room assets, tied to the unchanged native story anchors."""
REVISION = 'R2-used-SARO-records-room-1986'
BUDGET_TRIANGLES = 65000
ROOM_BOUNDS_THREE = [-6, -1, 6.8, 10.5]

# Ordinary storage only. Row 2's central bay remains reserved for the existing
# STATION 01 fall event and the independently animated fire drawer/reel.
SHELF_GAMEPLAY_BAY = {'row': 2, 'centre_z': -.05, 'half_width_z': .32}
SHELF_CONTENT_VARIANTS = {
 (0, 1): 'open', (0, 4): 'folders', (1, 2): 'gap', (1, 5): 'open',
 (2, 0): 'folders', (3, 4): 'open', (3, 6): 'gap',
}
BOOK_HEIGHTS = (.30, .265, .285, .25, .295, .275)
BOOK_WIDTHS = (.068, .060, .073, .057, .065, .070)
BOOK_LEAN_DEGREES = {1: -4., 9: 3.5}
# The separate, interactable ServiceBinder sits on the clear centre of the case.
TOP_BINDER_CLEAR_X = (-.30, .32)

def part(role, at, parent='annex.interior', local=False, **kw):
    return dict(role=role, site_world_three_m=at, parent_handle=parent,
                local=local, native_asset_root='/Game/ArchiveProps',
                retire_scope_handle='annex.interior', **kw)

PARTS = {
 'SM_ArchiveProps_ShelvingStorage': part('Five-tier bolted steel shelving and ordinary archive cartons', [-5.78,0,8.65], retire_geo_ids=[361,362,378,379], retire_node_names=['lic:archive_box','lic:archive_box_lid']),
 'SM_ArchiveProps_FilingCabinets': part('Three four-drawer cabinets at original footprints', [-4.46,0,10.17], retire_geo_ids=[335,336,337,338,401], retire_node_names=['lic:filing_cabinet']),
 'SM_ArchiveProps_CardIndex': part('Twelve-drawer maintenance index with one open card tray', [-1.25,0,7.95], retire_geo_ids=[329,330,331,332]),
 'SM_ArchiveProps_Bookcase': part('Low walnut bookcase and bound service-reference volumes', [-2.35,0,10.3], retire_geo_ids=[363,364,365,366,367]),
 'SM_ArchiveProps_WorkTable': part('Steel research table, original 0.77m working height', [-3.65,0,8.6], retire_geo_ids=[369], retire_triangle_regions=[{'geo':370,'bounds_three_m':[-4.51,-2.79,0,.73,8.17,9.03],'reason':'Only table legs. This merged steel mesh also includes corridor and other furniture, so do not retire it wholesale.'}]),
 'SM_ArchiveProps_FieldBox': part('Independent STATION 01 box following the original fall animation', [-5.62,1.135,8.6], 'annex.objs.fieldBoxModel', True, replaces=['annex.objs.fieldBoxModel']),
 'SM_ArchiveProps_FireDrawer': part('Open steel reel drawer, original sliding origin preserved', [-5.994,1.27,8.6], 'annex.arc.drawer', True, retire_geo_ids=[383,384,385,386]),
 'SM_ArchiveProps_Reel047Box': part('Separate grey 047 reel container, hidden when taken', [-6.154,1.186,8.6], 'annex.arc.reel', True, replaces=['annex.arc.reel'], preview_rotation_blender=[0,-.3,0]),
 'SM_ArchiveProps_Switchboard': part('KSU-12 metal shell and printed extension card; native TRK lamps retained', [-1.05,1.62,10.22], 'annex.arc.group', False, retire_geo_ids=[389,390,393], preserve_geo_ids=[391,392]),
 'SM_ArchiveProps_CallRecorder': part('Bracket-mounted mechanical strip recorder with native living paper', [-1.12,1.0,10.22], 'annex.arc.group', False, retire_geo_ids=[395,396,397,398], preserve_geo_ids=[400]),
 'SM_ArchiveProps_ServiceBinder': part('Cloth-bound service copies folder at original document anchor', [-2.32,.9,10.24], retire_geo_ids=[327,328], preview_rotation_blender=[0,0,.25], runtime_yaw_degrees=14.323944878),
}

PRESERVE_HANDLES = [
 'annex.interior','annex.objs.officeDoor','annex.objs.recordsDoor',
 'annex.objs.fieldBoxModel','annex.objs.fieldBox','annex.objs.binder',
 'annex.objs.lineage','annex.objs.workTable','annex.objs.recPhone',
 'annex.arc.group','annex.arc.drawer','annex.arc.reel','annex.arc.papers',
 'annex.arc.hit.drawer','annex.arc.hit.ksu','annex.arc.hit.recorder',
 'annex.arc.lamps.trk1','annex.arc.lamps.trk2','annex.arc.strip',
]
NATIVE_DYNAMIC = {
 'annex.arc.drawer': {'three_origin':[-5.994,1.27,8.6], 'travel_three':[.3,0,0]},
 'annex.arc.reel': {'parent':'annex.arc.drawer','three_local_origin':[-.16,-.084,0], 'rotation_three_z':.3, 'visibility':'archivePhone taking reel'},
 'annex.objs.fieldBoxModel': {'three_origin':[-5.62,1.135,8.6], 'motion':'archivePhone box falling; attach replacement locally'},
 'annex.arc.lamps.trk1': {'three_origin':[-1.105,1.732,10.103125], 'geo':391},
 'annex.arc.lamps.trk2': {'three_origin':[-1.105,1.732,10.1775], 'geo':392},
 'annex.arc.strip': {'three_origin':[-1.218,1.07,10.22], 'width_m':.056, 'geo':400, 'motion':'native runtime UV, length and curled rotation'},
 'annex.arc.papers': {'visibility':'native fall event', 'keep_geos':[387,388]},
}
