"""SARO fleet adaptation contract. All dimensions metres; original controller anchors retained."""
from pathlib import Path
import os
WORK=Path(os.environ.get('S47_FLEET_WORK','/home/tom/scratch-bm/Signal47-production-models/fleet'))
FREE=Path(os.environ.get('S47_FREE_WORK','/home/tom/scratch-bm/Signal47-free'))
ROOT='/Game/Free/ServiceFleet'
OWN_ROOT='/Game/SaroFleet'
C10_SCALE=2.06/2.1205409469334904
C10_OFFSET_Z=.30
REVISION='SF1-original-cab-C10-adaptation'
# Prepared C10 has nose -X; this rotation (not reflection) gives Three nose -Z.
def c10_three(p):return (-p[2]*C10_SCALE,p[1]*C10_SCALE,p[0]*C10_SCALE+C10_OFFSET_Z)
def blender(p):return (p[0],-p[2],p[1])
PARKED=[{'name':'reyesSedan','model':'fairheaven_lt80','at':[27,-.60,9.6],'heading':0}, {'name':'sitePickup','model':'el_alamo_78','at':[31,-.60,9.9],'heading':.04}]
# Legacy dressing fronts +Z at yawPI; imported fleet fronts -Z, hence headings0/.04.
CREDITS=[{'model':'pickup_c10_1966','title':'Chevrolet C10 Stepside1966','author':'Raphael Baget','license':'CC BY4.0','url':'https://sketchfab.com/3d-models/0a2ae2eea2cf4073a843ee7a381c74af'}, {'model':'fairheaven_lt80','title':"Fairheaven LT80","author":"Daniel Zhabotinsky","license":"CC BY4.0","url":"https://sketchfab.com/3d-models/e2678da920cc4be68dbc193727919ffb"}, {'model':'el_alamo_78','title':"El Alamo78","author":"Daniel Zhabotinsky","license":"CC BY4.0","url":"https://sketchfab.com/3d-models/41b3a5c0bc534817b3311c3736c403e6"}]
