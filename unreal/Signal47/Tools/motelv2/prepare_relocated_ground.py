"""Re-cut only original ground g4/g663 and asphalt g456 for current native slabs.

Ordinary Python, immutable exported GLBs in; a new private trim packet out.
All existing position/normal/UV/colour bytes remain as an unchanged prefix.
Independent serialized triangle rays prove excavation and preserved surroundings.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prepare_trims as P


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--export', type=Path, required=True)
    parser.add_argument('--layout', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    export, out = args.export.resolve(), args.out.resolve()
    assert not out.exists(), 'Use a fresh private output directory'
    out.mkdir(parents=True)
    manifest = json.loads((export / 'manifest.json').read_text())
    assert P.sha((export / 'manifest.json').read_bytes()) == '69fcf95db3adf8eff76274f59b03f94e662936a147f1ee7389630c1a8a0925c9'
    nodes = {n['i']:n for n in manifest['nodes']}
    layout = json.loads(args.layout.read_text())
    assert layout['rear_pool']['world_offset_z_m'] == 25
    rectangles = [f['rect'] for f in layout['floors']] + [layout['rear_pool']['water_rect']]
    openings = P.disjoint_rectangles(rectangles)
    rows, files = {}, []
    for geo, node_id, parent, translation, material in [(4,7,6,[0,0,0],5),(663,953,952,[-24,0,300],416)]:
        source = next(r for r in manifest['geoFiles'] if r['from'] <= geo < r['to'])
        doc, original = P.helper._load(export / 'geo' / source['file'])
        binary = bytearray(original)
        index, mesh = next((i,m) for i,m in enumerate(doc['meshes']) if m.get('name') == 'g'+str(geo))
        name = mesh['name']
        assert nodes[node_id]['p'] == parent and nodes[node_id]['geo'] == geo
        assert nodes[parent]['m'][12:15] == translation
        holes = [[a-translation[0],b-translation[0],c-translation[2],d-translation[2]] for a,b,c,d in openings]
        result = P.clip_native_footprints(doc, binary, mesh, holes)
        filename = 'trim_relocated_' + source['file']
        P.helper._write_part(doc,binary,[index],str(out),'relocated_'+source['file'])
        rows[name] = {**result, 'node':node_id,'geo':geo,'parent':parent,
            'accepted_previous_tris':[result['source_tris']], 'asset':f'/Game/MotelV2/Trim/{name}.{name}',
            'original_material_indices':[0],'source_material_ids':[material],'hidden':False,'fbx':f'exports/{name}.fbx'}
        files.append(filename)
        _, serialized = P.helper._load(out / filename)
        assert serialized[:len(original)] == original
    row, filename, asphalt = P.prepare_asphalt(export,out,manifest,nodes,openings,expected_result_triangles=None)
    rows['g456'] = row
    files.append(filename)
    proof = P.verify_ground_holes(out,files,rows)
    data = {'version':1,'status':'PREPARED_NATIVE_UNVERIFIED','meshes':rows,'files':files,
            'file_sha256':{f:P.sha((out/f).read_bytes()) for f in files},
            'ground_openings_world_xz_m':openings,'ground_hole_verification':proof,
            'legacy_asphalt_verification':asphalt,'runtime_layout_sha256':P.sha(args.layout.read_bytes())}
    (out/'trim-manifest.json').write_text(json.dumps(data,indent=2)+'\n')
    print('MOTEL_RELOCATED_GROUND_PREPARED',json.dumps({n:r['result_tris'] for n,r in rows.items()}))


if __name__ == '__main__':
    main()
