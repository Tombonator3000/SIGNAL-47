"""Stage authored JSON contracts, without launching the editor or changing assets."""
from pathlib import Path
import hashlib
import json
import shutil

ROOT = Path(__file__).resolve().parent
PROJECT = ROOT.parents[2]


def main():
    ground = json.loads((ROOT / 'model_contract.json').read_text())
    placement = json.loads((ROOT / 'nature-placement.json').read_text())
    assert 1000 <= placement['total_instances'] <= 80000
    assert len(placement['families']) == 12
    assert all(name.startswith('SM_CN_') for name in ground['parts'])
    assert ground['total_triangles'] < 300000
    props = {
        '_doc': 'Original Caliche nature overlays. No collision, no ground traces; existing road and walker heights remain authoritative. The separate baked HISM kit is loaded by BuildCalicheNature.',
        'area': 'diner', 'parent': 'world.town.group', 'walk': 'road',
        'leg': '', 'grounds': ['diner', 'road'],
        'props': [{
            'id': 'streetGroundNature' + name.removeprefix('SM_CN_'),
            'model': f'/Game/CalicheStreet/NatureGround/{name}.{name}',
            'at': ground['anchor_world_three_m'], 'yaw': 0,
            'ground': False, 'trace': False,
        } for name in sorted(ground['parts'])],
    }
    target = PROJECT / 'Content/CalicheStreet/Nature/nature-placement.json'
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(ROOT / 'nature-placement.json', target)
    (PROJECT / 'Config/Props/caliche_nature.json').write_text(json.dumps(props, indent=2) + '\n')
    assert hashlib.sha256(target.read_bytes()).digest() == hashlib.sha256((ROOT / 'nature-placement.json').read_bytes()).digest()
    print('NATURE_JSON_STAGED', len(props['props']), placement['total_instances'])


if __name__ == '__main__':
    main()
