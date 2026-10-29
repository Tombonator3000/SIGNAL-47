"""Caliche home dimensions from immutable original geometry. No DCC dependency."""
import hashlib
import json
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
PROJECT = TOOLS.parents[1]
PACKAGE = PROJECT.parent / 'ArtSource' / 'CalicheHomes'
CONTRACT_PATH = PACKAGE / 'source-contract.json'
CONTRACT = json.loads(CONTRACT_PATH.read_text())
HOUSES = CONTRACT['houses']
PREFIX = 'SM_Homes_'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def front_x(house):
    x0, x1, _, _ = house['footprint_blender_xy_m']
    return x1 if house['front'] > 0 else x0


def world(house, point):
    x, y, z = house['root_world_three_m']
    return (x + point[0], y + point[2], z - point[1])


def apertures(house):
    """Wall voids leave original door/casing and image planes unobstructed."""
    door_w, door_h = CONTRACT['door_surround_clear_size_m']
    win_w, win_h = CONTRACT['window_size_m']
    dy, wy, wz = house['door_y_m'], house['window_y_m'], CONTRACT['window_center_z_m']
    return [(dy - door_w / 2, dy + door_w / 2, -.10, door_h),
            (wy - win_w / 2, wy + win_w / 2, wz - win_h / 2, wz + win_h / 2)]


def part_specs():
    return {
        PREFIX + home + '_' + role: {
            'house': home, 'chunk': role, 'site_world_three_m': row['root_world_three_m'],
            'parent_handle': CONTRACT['parent_handle'], 'offset': [0, 0, 0],
            'front': row['front'], 'original_road_rect_m': row['original_road_rect_m'],
            'collision': CONTRACT['native_policy']['collision'],
            'cast_shadow': True, 'ray_tracing': True, 'nanite': False}
        for home, row in HOUSES.items() for role in CONTRACT['parts']
    }


PARTS = part_specs()
