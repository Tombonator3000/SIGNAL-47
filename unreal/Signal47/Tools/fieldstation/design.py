"""STATION 01 original hut, in the existing site's physical metre coordinates."""
REVISION = 'FS1-original-station01-hut'
ROOT_THREE = [0, 0, 8000]
HUT = {'x0': -13., 'x1': -9., 'z0': 1.5, 'z1': 4.5, 'thickness': .2, 'height': 2.6}
APERTURES = {
    'south_door': {'x': [-10.5, -9.4], 'z': [4.5, 4.7], 'y': [0, 2.0]},
    'south_window': {'x': [-12.3, -11.3], 'z': [4.5, 4.7], 'y': [.95, 1.85]},
    'east_window': {'x': [-9., -8.8], 'z': [2.3, 3.3], 'y': [.95, 1.85]},
}
PARTS = {
    'SM_FieldStation_Hut_Shell': {'parent_handle': 'world.site.group', 'offset': [0,0,0], 'site_world_three_m': ROOT_THREE, 'role': 'Aperture-partitioned stucco, flashing, roof, meter, plinth, threshold and plaque'},
    'SM_FieldStation_Hut_Interior': {'parent_handle': 'world.site.interior', 'offset': [0,0,0], 'site_world_three_m': ROOT_THREE, 'role': 'Plaster and dado lining, existing-floor-height timber boards, beams and cable clips'},
    'SM_FieldStation_Hut_Glass': {'parent_handle': 'world.site.group', 'offset': [0,0,0], 'site_world_three_m': ROOT_THREE, 'role': 'Eight actual thin panes in two open wall apertures'},
    'SM_FieldStation_Hut_Props': {'parent_handle': 'world.site.interior', 'offset': [0,0,0], 'site_world_three_m': ROOT_THREE, 'role': 'Steel cupboard, open timber shelving and survey chair at existing furniture anchors'},
    'SM_FieldStation_Hut_Door': {'parent_handle': 'world.site.objs.hutDoorLeaf', 'offset': [0,0,0], 'site_world_three_m': ROOT_THREE, 'role': 'Independent physical timber door and hardware, local hinge origin; inherit existing door animation', 'parent_pivot_site_three_m': [-9.45,0,4.48], 'initial_parent_rotation_three_y_rad': -1.75},
}
PROTECTED_RUNTIME = [
    'world.site.group', 'world.site.interior', 'world.site.colliders[*]',
    'world.site.occluders[*]', 'world.site.proxies.*', 'world.site.anchors.*',
    'world.site.objs.hutDoorLeaf transform', 'world.site.objs.drawer',
    'world.site.objs.drawerInside', 'world.site.objs.fieldPhoneHandset transform',
    'world.site.glow.points', 'world.site.m.hutBulb', 'world.site.m.wallBulb',
    'world.site.m.lampBulb', 'world.site.m.fieldBulb', 'world.site.m.dial',
    'world.site.m.receiverDial', 'world.site.m.paneOut', 'world.site.m.paneIn',
]
# Box volumes are envelopes for an integrator audit, never a permission to delete
# every triangle inside them: phone/log/receiver/light/interactive content overlaps.
COVERAGE = {
    'Shell': {'site_three_min': [-13.50,-.25,1.00], 'site_three_max': [-8.50,3.8,5.45]},
    'Interior': {'site_three_min': [-13.0,-.03,1.50], 'site_three_max': [-9.0,2.60,4.50]},
    'Props': {
        'cabinet': {'site_three_min': [-12.99,0,3.45], 'site_three_max': [-12.51,1.80,4.35]},
        'chair': {'site_three_center': [-12.,0,3.05], 'yaw_three_y_rad': .6},
        'shelf': {'site_three_min': [-12.95,1.37,1.5], 'site_three_max': [-12.05,1.85,1.77]},
    },
}
EXCLUDED = ['receiver','bench','desk lamp','ceiling bulb and shade','external working lamps',
            'field telephone and handset','timing log','1947 diagram','drawer and book',
            'transit and sight lines','generator shed','yard/fence/pole/track','mission markers']
