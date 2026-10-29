"""Small CPU authoring-plan check. Not Blender/native/visual acceptance."""
import json
from collections import Counter
from pathlib import Path
from geometry import build, cross


def main():
    rows=[]
    for lod in range(3):
        shapes=build(lod)
        for s in shapes:
            edges=Counter()
            for f in s.triangles:
                a,b,c=(s.vertices[i]for i in f)
                normal=cross(tuple(b[k]-a[k]for k in range(3)),tuple(c[k]-a[k]for k in range(3)))
                assert sum(v*v for v in normal)>1e-20,(s.name,'degenerate')
                points=[tuple(round(x,12)for x in s.vertices[i])for i in f]
                for x,y in zip(points,points[1:]+points[:1]):edges[x,y]+=1
            assert all(edges[a,b]==edges[b,a]==1 for a,b in edges),(s.name,'nonmanifold or inconsistent outward winding')
        rows.append(dict(lod=lod,solids=len(shapes),triangles=sum(len(s.triangles)for s in shapes),
                         min_signed_volume_m3=min(s.volume()for s in shapes),
                         materials=sorted({s.material for s in shapes})))
    assert rows[0]['triangles']>rows[1]['triangles']>rows[2]['triangles']
    assert all(len(r['materials'])==4 for r in rows)
    print(json.dumps(dict(status='CPU_AUTHORING_PLAN_PASS_NOT_DCC',lods=rows,native=False,runtime=False,visual=False),indent=2))


if __name__=='__main__':main()
