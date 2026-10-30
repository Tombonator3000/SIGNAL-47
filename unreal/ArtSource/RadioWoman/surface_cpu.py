"""Privat CPU-leser for R3s faktiske worldmesh. Ingen bpy eller filmutasjon."""
import heapq, math

def add(a,b): return tuple(x+y for x,y in zip(a,b))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def mul(a,s): return tuple(x*s for x in a)
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def length(a): return math.sqrt(dot(a,a))
def unit(a): return mul(a,1/length(a))
def closest(p,a,b,c):
    # Triangle regions, Real-Time Collision Detection, actual fixed topology.
    ab,ac,ap=sub(b,a),sub(c,a),sub(p,a)
    d1,d2=dot(ab,ap),dot(ac,ap)
    if d1<=0 and d2<=0:return a
    bp=sub(p,b);d3,d4=dot(ab,bp),dot(ac,bp)
    if d3>=0 and d4<=d3:return b
    vc=d1*d4-d3*d2
    if vc<=0 and d1>=0 and d3<=0:return add(a,mul(ab,d1/(d1-d3)))
    cp=sub(p,c);d5,d6=dot(ab,cp),dot(ac,cp)
    if d6>=0 and d5<=d6:return c
    vb=d5*d2-d1*d6
    if vb<=0 and d2>=0 and d6<=0:return add(a,mul(ac,d2/(d2-d6)))
    va=d3*d6-d5*d4
    if va<=0 and d4-d3>=0 and d5-d6>=0:return add(b,mul(sub(c,b),(d4-d3)/(d4-d3+d5-d6)))
    den=va+vb+vc
    if abs(den)<1e-24: return min((a,b,c),key=lambda q:dot(sub(q,p),sub(q,p)))
    return add(a,add(mul(ab,vb/den),mul(ac,vc/den)))

class Surface:
    def __init__(self,mesh,choose=None):
        self.verts=mesh['vertices_world_m'];self.tris=mesh['triangles']
        ids=[];self.bounds={};self.normal={}
        for i,t in enumerate(self.tris):
            vs=[self.verts[j] for j in t['vertices']]
            n=cross(sub(vs[1],vs[0]),sub(vs[2],vs[0]));area=length(n)
            if area<1e-14:continue
            n=mul(n,1/area)
            if choose and not choose(vs,n):continue
            ids.append(i);self.normal[i]=n
            self.bounds[i]=tuple((min(p[k] for p in vs),max(p[k] for p in vs)) for k in range(3))
        def tree(ids):
            bounds=tuple((min(self.bounds[i][k][0] for i in ids),max(self.bounds[i][k][1] for i in ids)) for k in range(3))
            if len(ids)<=10:return (bounds,ids,None,None)
            k=max(range(3),key=lambda k:bounds[k][1]-bounds[k][0])
            ids.sort(key=lambda i:sum(self.bounds[i][k]));m=len(ids)//2
            return (bounds,None,tree(ids[:m]),tree(ids[m:]))
        self.count=len(ids);self.tree=tree(ids)
    def nearest(self,p):
        def dbox(b):return sum(max(lo-x,0,x-hi)**2 for x,(lo,hi) in zip(p,b))
        seq=0;queue=[(0,seq,self.tree)];best=float('inf');out=None
        while queue:
            d,_,node=heapq.heappop(queue)
            if d>best:break
            if node[1] is not None:
                for i in node[1]:
                    if dbox(self.bounds[i])>best:continue
                    q=closest(p,*(self.verts[j] for j in self.tris[i]['vertices']))
                    dv=sub(p,q);ds=dot(dv,dv)
                    if ds<best:
                        best=ds;out=dict(point=q,normal=self.normal[i],triangle=i,polygon=self.tris[i]['polygon_index'],distance=math.sqrt(ds),signed_plane=dot(dv,self.normal[i]))
            else:
                for child in node[2:]:
                    dd=dbox(child[0])
                    if dd<=best:seq+=1;heapq.heappush(queue,(dd,seq,child))
        return out
    def ray_hits(self,p,d,limit):
        def box(b):
            lo,hi=0.,limit
            for k in range(3):
                if abs(d[k])<1e-12:
                    if p[k]<b[k][0] or p[k]>b[k][1]:return False
                else:
                    a,c=(b[k][0]-p[k])/d[k],(b[k][1]-p[k])/d[k]
                    if a>c:a,c=c,a
                    lo=max(lo,a);hi=min(hi,c)
                    if lo>hi:return False
            return True
        nodes=[self.tree];hits=[]
        while nodes:
            node=nodes.pop()
            if not box(node[0]):continue
            if node[1] is None:nodes.extend(node[2:]);continue
            for i in node[1]:
                if not box(self.bounds[i]):continue
                a,b,c=(self.verts[j] for j in self.tris[i]['vertices'])
                e1,e2=sub(b,a),sub(c,a);pv=cross(d,e2);det=dot(e1,pv)
                if abs(det)<1e-12:continue
                inv=1/det;tv=sub(p,a);u=dot(tv,pv)*inv
                if u< -1e-8 or u>1+1e-8:continue
                qv=cross(tv,e1);v=dot(d,qv)*inv
                if v< -1e-8 or u+v>1+1e-8:continue
                t=dot(e2,qv)*inv
                if t< -1e-8 or t>limit+1e-8:continue
                hits.append(dict(distance=t,point=add(p,mul(d,t)),normal=self.normal[i],triangle=i,polygon=self.tris[i]['polygon_index'],normal_dot=dot(d,self.normal[i])))
        return sorted(hits,key=lambda h:h['distance'])
