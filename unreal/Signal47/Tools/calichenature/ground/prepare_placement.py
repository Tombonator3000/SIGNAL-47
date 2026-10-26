"""Bake original, irregular plant/stone placement with explicit protected routes."""
from pathlib import Path
from collections import defaultdict
import hashlib
import json
import math
import random
from terrain_contract import REGION,KEEP_CLEAR,ANCHOR,box,visual_y,floor_y,old_distance,patch_field

ROOT=Path(__file__).resolve().parent
KIT=ROOT.parent/'vegetation'
COUNTS={'sage':11000,'grass':14000,'yucca':1300,'juniper':800,'stone':8500}
CULL={'sage':220,'grass':90,'yucca':260,'juniper':1200,'stone':110}
# Authored foreground and middle-distance groups at the southern town edge.
# These are ordinary persistent world placements, covered by exactly the same
# road, building, collider and spacing masks as the wider baked field.
# Local X, world Z, yaw degrees, scale XYZ. Counts stay within each family quota.
HEROES={
    'SM_CN_Juniper_A':[(24,2255,37,1,1,.92)],
    'SM_CN_Juniper_B':[(25,2276,143,1,1,1)],
    'SM_CN_Yucca_A':[(16.8,2298,29,.95,1,.92)],
    'SM_CN_Yucca_B':[(11.5,2303.8,127,1,1,1)],
    'SM_CN_Sage_B':[(10.8,2299.5,72,1,1,.94)],
    'SM_CN_Grass_A':[(8.4,2301.2,164,.90,.85,1)],
    'SM_CN_Grass_B':[(8.7,2305.6,48,1,1,1.08)],
    'SM_CN_Caliche_A':[(7.9,2304.2,51,.55,.65,.64),(8.9,2306.3,203,.67,.62,.72)],
    'SM_CN_Caliche_B':[(10.2,2306.2,23,.80,.92,.78)],
}


def main():
    source=json.loads((KIT/'model_contract.json').read_text())
    protection=json.loads((ROOT/'protection.json').read_text())
    exclusions=list(KEEP_CLEAR)+[(18,61,2012,2061)]
    exclusions += [tuple(row['road_local_rect_xz']) for row in protection['rows'] if row['enabled']]
    # Collider rectangles cover original walls; complete house/own replacement
    # footprints in KEEP_CLEAR also keep plants out of their open interiors.
    bins=defaultdict(list)
    for b in exclusions:
        for i in range(math.floor((b[0]-5)/20),math.floor((b[1]+5)/20)+1):
            for j in range(math.floor((b[2]-5)/20),math.floor((b[3]+5)/20)+1):bins[i,j].append(b)
    families=[];taken=defaultdict(list);statistics={}
    def allowed(x,z,radius):
        if abs(x)<7+radius+.15 or old_distance(x,z)<17+radius+.15:return False
        if any(box(x,z,b,radius+.10) for b in bins[math.floor(x/20),math.floor(z/20)]):return False
        for i in range(math.floor((x-6)/4),math.floor((x+6)/4)+1):
            for j in range(math.floor((z-6)/4),math.floor((z+6)/4)+1):
                for px,pz,pr in taken[i,j]:
                    if math.hypot(x-px,z-pz)<.62*(pr+radius):return False
        return True
    # Larger silhouettes are placed first. Different species naturally share
    # patches; the spacing only prevents implausibly coincident root masses.
    ordered=sorted(source['parts'].items(),key=lambda item: {'juniper':0,'yucca':1,'sage':2,'grass':3,'stone':4}[item[1]['family']])
    for name,part in ordered:
        category=part['family']; peers=sum(p['family']==category for p in source['parts'].values())
        desired=COUNTS[category]//peers
        seed=int(hashlib.sha256(name.encode()).hexdigest()[:8],16)
        rng=random.Random(seed)
        bounds=part['bounds_local_m']
        rx=max(abs(bounds[0][0]),abs(bounds[1][0]));ry=max(abs(bounds[0][1]),abs(bounds[1][1]))
        rows=[];attempt=0
        for x,z,yaw,sx,sy,sz in HEROES.get(name,[]):
            radius=math.hypot(rx*sx,ry*sy)
            assert allowed(x,z,radius),(name,'authored group violates protected space',x,z)
            y=visual_y(x,z)-(.035*sz if category=='stone' else .012)
            rows.append([8000+x,round(y,6),z,yaw,sx,sy,sz])
            taken[math.floor(x/4),math.floor(z/4)].append((x,z,radius))
        while len(rows)<desired and attempt<desired*80:
            attempt+=1
            near=.40 if category=='juniper' else .82 if category in ('grass','stone') else .68
            if rng.random()<near:
                x=rng.choice((-1,1))*(7+((150 if category!='juniper' else 420)-7)*rng.random()**(2.15 if category in ('sage','grass','stone') else 1.25))
            else:x=rng.uniform(REGION[0]+5,REGION[1]-5)
            z=rng.uniform(REGION[2]+5,REGION[3]-5)
            density=patch_field(x,z)
            if rng.random()>(.15+.85*density):continue
            # Mineral fragments are mostly hand-sized chips, with a minority
            # of larger loose blocks. The initial all-boulder field was rejected.
            horizontal=(rng.uniform(.23,.62) if rng.random()<.84 else rng.uniform(.65,1.15)) if category=='stone' else rng.uniform(.72,1.28)
            sx=max(.2,horizontal*rng.uniform(.88,1.12));sy=max(.2,horizontal*rng.uniform(.88,1.12))
            sz=max(.2,horizontal*rng.uniform(.65,1.0)) if category=='stone' else rng.uniform(.78,1.24)
            radius=math.hypot(rx*sx,ry*sy)
            if not allowed(x,z,radius):continue
            y=visual_y(x,z)-(.035*sz if category=='stone' else .012)
            rows.append([round(8000+x,5),round(y,6),round(z,5),round(rng.uniform(0,360),3),round(sx,5),round(sy,5),round(sz,5)])
            taken[math.floor(x/4),math.floor(z/4)].append((x,z,radius))
        assert len(rows)>=desired*.90,(name,len(rows),desired,attempt)
        lods=part['lods']
        families.append({'id':name.removeprefix('SM_CN_'),'family':category,
            'mesh':f'/Game/CalicheStreet/NatureVegetation/{name}.{name}',
            'lod_triangles':[p['triangles'] for p in lods], 'cull_metres':CULL[category],
            'shadow':category!='grass' and name!='SM_CN_Caliche_A','seed':seed,'instances':rows})
        statistics[name]={'placed':len(rows),'authored_foreground':len(HEROES.get(name,[])),'attempts':attempt,'cull_metres':CULL[category]}
    probes=[]
    for x,z in [(-500,2205),(-99,1955),(60,2300),(72.5,2355),(88.125,2055),(200,2255),(400,1850),(599,2399),(7,2310),(-8,2150),(12,1880)]:
        probes.append([x,z,floor_y(x,z)])
    result={'version':1,'authoring':'Original deterministic Caliche nature placement, no runtime random scatter',
        'anchor_world_three_m':ANCHOR,'region_road_local_xz':REGION,
        'prototype_contract_sha256':hashlib.sha256((KIT/'model_contract.json').read_bytes()).hexdigest(),
        'original_road_manifest_sha256':protection['source_sha256'],'exclusions':exclusions,
        'protected_main_axis_radius_metres':7,'protected_full_old_road_radius_metres':17,
        'height_probes':probes,'families':families,'total_instances':sum(len(f['instances']) for f in families)}
    (ROOT/'nature-placement.json').write_text(json.dumps(result,separators=(',',':'))+'\n')
    (ROOT/'placement-summary.json').write_text(json.dumps({'status':'SOURCE_PLACEMENTS_PREPARED_NATIVE_UNVERIFIED',
        'total_instances':result['total_instances'],'families':statistics,'exclusions':len(exclusions),
        'notes':'All placements checked with circumscribed prototype radius, full OldStub geometry, all original collider rectangles, town walk/proxy/own building masks; no collider added.'},indent=2)+'\n')
    print('PLACEMENTS_PREPARED',result['total_instances'],len(families),'families')


if __name__=='__main__':main()
