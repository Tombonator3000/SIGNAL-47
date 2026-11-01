"""Import only the new Jeep cabin derivative. Root runs serially after Blender.

Plan outside Unreal:
  python3 import_interior.py --manifest /private/interior-20261101/manifest.json
Editor execution: S47_JEEP_INTERIOR_MANIFEST, S47_JEEP_INTERIOR_APPLY=1 and
S47_JEEP_INTERIOR_REPORT=/private/native-import.json. No automatic execution on import.
"""
from pathlib import Path
import argparse
import json
import os
import sys

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import import_assets as fleet

PARTS={'SM_TripoJeep_'+suffix for suffix in (
    'InteriorBody','Interior','Steering','SpeedFace','FuelFace','TempFace','RadioFace')}


def plan(path):
    data=fleet.make_plan(path,{'classic_jeep'})
    model=data['models'][0]
    fleet.require({p['name']for p in model['parts']}==PARTS,
                  'Cabin import must contain exactly the seven new derivative meshes')
    for row in model['materials']:
        fleet.require(row['name'].startswith('M_JeepInterior_') or row['name'].endswith('_CabTrim'),
                      'Original fallback material must not be overwritten',row['name'])
    anchors_path=Path(model['root'])/'anchors.json'
    fleet.require(anchors_path.is_file(),'Missing measured cabin anchors')
    anchors=json.loads(anchors_path.read_text())
    fleet.require(anchors['driver_eye_three_m']==[-.41,1.70,.65] and
                  anchors['dial_normal_three']==[0,0,1],'Unexpected fitted cabin coordinate frame')
    data['runtime_notes']={
        'atomic_parts':sorted(PARTS),
        'face_override':'Speed/Fuel/Temp/Radio faces receive the existing live MGauge MID; UVs crop the same DashRT.',
        'fallback':'Keep the complete previous Jeep until every new cabin/body mesh is loaded.',
        'anchors':str(anchors_path)}
    return data


def main():
    if 'unreal' in sys.modules or os.environ.get('S47_JEEP_INTERIOR_APPLY')=='1':
        fleet.require(os.environ.get('S47_JEEP_INTERIOR_APPLY')=='1','Native cabin import needs explicit apply flag')
        filename=os.environ.get('S47_JEEP_INTERIOR_MANIFEST')
        report=os.environ.get('S47_JEEP_INTERIOR_REPORT')
        fleet.require(filename and report,'Set private cabin manifest and report paths')
        fleet.import_candidates(plan(filename),report)
    else:
        parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--manifest',required=True)
        args=parser.parse_args()
        data=plan(args.manifest)
        print(json.dumps({'status':'PLAN_ONLY','parts':data['runtime_notes']['atomic_parts'],
                          'anchors':data['runtime_notes']['anchors']},indent=2))


if __name__=='__main__':main()
