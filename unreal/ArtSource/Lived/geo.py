"""Small mesh builder for room-origin dressing. All coordinates in the game's own three.js metres (x, up, z); mapped to Blender (x,-z,y)."""
import math,random
import bmesh,bpy
from mathutils import Vector
class Geo:
    def __init__(self,seed=47):
        self.v=[];self.f=[];self.r=random.Random(seed)
    def vert(self,p):self.v.append((float(p[0]),float(p[1]),float(p[2])));return len(self.v)-1
    def face(self,idx,mat,smooth=False,uv=None):self.f.append((tuple(idx),mat,smooth,uv))
    def quad(self,pts,mat,uv=None,smooth=False):
        self.face([self.vert(p) for p in pts],mat,smooth,uv)
    def box(self,mat,at,size,yaw=0.,pitch=0.,roll=0.,bevel=0.):
        cx,cy,cz=at;hx,hy,hz=size[0]/2,size[1]/2,size[2]/2
        cy_,sy_=math.cos(yaw),math.sin(yaw);cp,sp=math.cos(pitch),math.sin(pitch);cr,sr=math.cos(roll),math.sin(roll)
        def T(x,y,z):
            # roll about z, pitch about x, yaw about y
            x,y=x*cr-y*sr,x*sr+y*cr
            y,z=y*cp-z*sp,y*sp+z*cp
            x,z=x*cy_+z*sy_,-x*sy_+z*cy_
            return (cx+x,cy+y,cz+z)
        if bevel<=0:
            P=[self.vert(T(sx*hx,sy*hy,sz*hz)) for sx in(-1,1) for sy in(-1,1) for sz in(-1,1)]
            # 0:--- 1:--+ 2:-+- 3:-++ 4:+-- 5:+-+ 6:++- 7:+++
            for q in((0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)):
                self.face([P[i] for i in q],mat)
            return
        b=min(bevel,hx*.9,hy*.9,hz*.9)
        P={}
        for sx in(-1,1):
            for sy in(-1,1):
                for sz in(-1,1):
                    P[(sx,sy,sz,0)]=self.vert(T(sx*(hx-b),sy*(hy-b),sz*hz))
                    P[(sx,sy,sz,1)]=self.vert(T(sx*(hx-b),sy*hy,sz*(hz-b)))
                    P[(sx,sy,sz,2)]=self.vert(T(sx*hx,sy*(hy-b),sz*(hz-b)))
        for sz in(-1,1):
            q=[P[(-1,-1,sz,0)],P[(1,-1,sz,0)],P[(1,1,sz,0)],P[(-1,1,sz,0)]]
            self.face(q if sz>0 else q[::-1],mat)
        for sy in(-1,1):
            q=[P[(-1,sy,-1,1)],P[(1,sy,-1,1)],P[(1,sy,1,1)],P[(-1,sy,1,1)]]
            self.face(q if sy>0 else q[::-1],mat)
        for sx in(-1,1):
            q=[P[(sx,-1,-1,2)],P[(sx,1,-1,2)],P[(sx,1,1,2)],P[(sx,-1,1,2)]]
            self.face(q if sx>0 else q[::-1],mat)
        # chamfers
        for sy in(-1,1):
            for sz in(-1,1):
                q=[P[(-1,sy,sz,0)],P[(1,sy,sz,0)],P[(1,sy,sz,1)],P[(-1,sy,sz,1)]]
                self.face(q if sy*sz>0 else q[::-1],mat)
        for sx in(-1,1):
            for sz in(-1,1):
                q=[P[(sx,-1,sz,0)],P[(sx,1,sz,0)],P[(sx,1,sz,2)],P[(sx,-1,sz,2)]]
                self.face(q if sx*sz<0 else q[::-1],mat)
        for sx in(-1,1):
            for sy in(-1,1):
                q=[P[(sx,sy,-1,1)],P[(sx,sy,1,1)],P[(sx,sy,1,2)],P[(sx,sy,-1,2)]]
                self.face(q if sx*sy>0 else q[::-1],mat)
        for sx in(-1,1):
            for sy in(-1,1):
                for sz in(-1,1):
                    q=[P[(sx,sy,sz,0)],P[(sx,sy,sz,1)],P[(sx,sy,sz,2)]]
                    self.face(q if sx*sy*sz>0 else q[::-1],mat)
    def lathe(self,mat,at,prof,sides=16,axis='y',cap=False):
        """prof: [(radius,height)...] from the bottom up."""
        cx,cy,cz=at;rings=[]
        for r,h in prof:
            ring=[]
            for i in range(sides):
                a=i/sides*2*math.pi;ca,sa=math.cos(a)*r,math.sin(a)*r
                if axis=='y':p=(cx+ca,cy+h,cz+sa)
                elif axis=='x':p=(cx+h,cy+ca,cz+sa)
                else:p=(cx+ca,cy+sa,cz+h)
                ring.append(self.vert(p))
            rings.append(ring)
        # Walking up the profile, the outside is on the right. (Worked out for each axis: y wants the other turn; a profile run backward along its axis, the other again.)
        rev=(axis=='y')!=((prof[-1][1]-prof[0][1])<0)
        for a,b in zip(rings,rings[1:]):
            for i in range(sides):
                q=[a[i],a[(i+1)%sides],b[(i+1)%sides],b[i]]
                self.face(q[::-1] if rev else q,mat,True)
        if cap:
            top=rings[-1];bot=rings[0]
            self.face(top[::-1] if rev else top,mat)
            self.face(bot if rev else bot[::-1],mat)
    def cyl(self,mat,at,r,h,axis='y',sides=12,cap=True):
        self.lathe(mat,at,[(r,0),(r,h)],sides,axis,cap)
    def tube(self,mat,path,r,sides=5):
        """A round cable through points. Frames carried along so it does not twist."""
        P=[Vector(p) for p in path];n=len(P)
        up=Vector((0,1,0));rings=[]
        prev_side=None
        for i,p in enumerate(P):
            t=(P[min(i+1,n-1)]-P[max(i-1,0)]).normalized()
            if prev_side is None:
                side=t.cross(up)
                if side.length<.01:side=t.cross(Vector((1,0,0)))
            else:side=prev_side-t*prev_side.dot(t)
            side.normalize();prev_side=side;nor=side.cross(t).normalized()
            ring=[self.vert(p+side*math.cos(a)*r+nor*math.sin(a)*r) for a in[j/sides*2*math.pi for j in range(sides)]]
            rings.append(ring)
        for a,b in zip(rings,rings[1:]):
            for i in range(sides):
                j=(i+1)%sides
                self.face([a[i],b[i],b[j],a[j]],mat,True)
    def plane_on(self,mat,axis,fixed,c,w,h,roll=0.,uv=None,flip=False):
        """Flat sheet on a wall or desk. axis 'x': on a wall of constant x, w along z, h up, facing +x (flip: -x).
        axis 'z': on a wall of constant z, w along x, h up, facing +z (flip: -z). axis 'y': lying flat at y=fixed, w along x, h along z, facing up. c = the two free coordinates."""
        cr,sr=math.cos(roll),math.sin(roll)
        def L(a,b):return(a*cr-b*sr,a*sr+b*cr)
        cor=[L(-w/2,-h/2),L(w/2,-h/2),L(w/2,h/2),L(-w/2,h/2)]
        if axis=='x':pts=[(fixed,c[1]+b,c[0]+(a if flip else -a)) for a,b in cor]   # facing +x reads with z running to the viewer's right as -z
        elif axis=='z':pts=[(c[0]+(-a if flip else a),c[1]+b,fixed) for a,b in cor]
        else:pts=[(c[0]+a,fixed,c[1]-b) for a,b in cor]
        self.quad(pts,mat,uv)
def build(geo,name,matrows=None,uvscale=None):
    me=bpy.data.meshes.new(name);ob=bpy.data.objects.new(name,me);bpy.context.scene.collection.objects.link(ob)
    bm=bmesh.new();uvl=bm.loops.layers.uv.new('UVMap')
    V=[bm.verts.new((x,-z,y)) for x,y,z in geo.v]
    bm.verts.ensure_lookup_table()
    mats={};tris=0
    for idx,mat,smooth,uv in geo.f:
        if mat not in mats:
            m=bpy.data.materials.get(mat) or bpy.data.materials.new(mat);me.materials.append(m);mats[mat]=len(mats)
        try:f=bm.faces.new([V[i] for i in idx])
        except ValueError:continue
        f.material_index=mats[mat];f.smooth=smooth;tris+=len(idx)-2
        if uv:
            for l,t in zip(f.loops,uv):l[uvl].uv=t
    bm.normal_update()
    for f in bm.faces:
        if any(l[uvl].uv.length_squared>0 for l in f.loops):continue
        n=f.normal
        ax=max(range(3),key=lambda i:abs(n[i]))
        for l in f.loops:
            c=l.vert.co
            t={0:(c.y,c.z),1:(c.x,c.z),2:(c.x,c.y)}[ax]
            k=uvscale.get(me.materials[f.material_index].name,1.) if uvscale else 1.
            l[uvl].uv=(t[0]/k,t[1]/k)
    bm.to_mesh(me);bm.free()
    return ob,tris
def export_fbx(ob,path):
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
    t=ob.modifiers.new('Tri','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=t.name)
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={"MESH"},apply_unit_scale=True,apply_scale_options="FBX_SCALE_UNITS",axis_forward="-Y",axis_up="Z",bake_anim=False,mesh_smooth_type="FACE")
    return len(ob.data.polygons)
