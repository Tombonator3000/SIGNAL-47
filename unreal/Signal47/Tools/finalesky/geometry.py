"""Original normalized craft geometry; ordinary Python, no bpy or borrowed payload.

Closed solids have outward winding and their measured signed volume is checked.
The final .blend retains every named solid as an editable source object.
"""
import math


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


class Solid:
    def __init__(self, name, material):
        self.name, self.material = name, 'M_S47_Final' + material
        self.vertices, self.triangles, self.uv = [], [], []

    def face(self, points, uv=None):
        if uv is None:
            uv = [(p[0]*.5+.5, p[1]*.5+.5) for p in points]
        start = len(self.vertices)
        self.vertices.extend(points)
        for k in range(1, len(points)-1):
            self.triangles.append((start, start+k, start+k+1))
            self.uv.append((uv[0], uv[k], uv[k+1]))

    def volume(self):
        return sum(sum(a[k]*cross(b, c)[k] for k in range(3)) / 6
                   for a, b, c in ([self.vertices[i] for i in f] for f in self.triangles))


def lathe(name, profile, segments, material):
    s = Solid(name, material)
    rings = [[(r*math.cos(2*math.pi*i/segments), r*math.sin(2*math.pi*i/segments), z)
              for i in range(segments)] for z, r in profile]
    for k in range(len(rings)-1):
        for i in range(segments):
            j = (i+1) % segments
            s.face([rings[k][i], rings[k][j], rings[k+1][j], rings[k+1][i]],
                   [(i/segments, k/(len(profile)-1)), ((i+1)/segments, k/(len(profile)-1)),
                    ((i+1)/segments, (k+1)/(len(profile)-1)), (i/segments, (k+1)/(len(profile)-1))])
    s.face(rings[0][::-1]); s.face(rings[-1])
    return s


def wedge(name, r0, r1, a0, a1, z, height, material, steps=3):
    s = Solid(name, material)
    arc = [a0+(a1-a0)*i/steps for i in range(steps+1)]
    lo = [[(r*math.cos(a),r*math.sin(a),z-height/2)for a in arc]for r in (r0,r1)]
    hi = [[(x,y,z+height/2)for x,y,_ in ring]for ring in lo]
    # Cell-wise caps preserve the annular void. A fan over the concave outline would fill it.
    for i in range(steps):
        j=i+1
        s.face([lo[0][i],lo[0][j],lo[1][j],lo[1][i]])
        s.face([hi[1][i],hi[1][j],hi[0][j],hi[0][i]])
        s.face([lo[0][i],hi[0][i],hi[0][j],lo[0][j]])
        s.face([lo[1][i],lo[1][j],hi[1][j],hi[1][i]])
    s.face([lo[0][0],lo[1][0],hi[1][0],hi[0][0]])
    s.face([lo[1][-1],lo[0][-1],hi[0][-1],hi[1][-1]])
    return s


def build(lod):
    n = [192, 112, 64][lod]
    out = [lathe('Original_continuous_dished_hull',
                 [(-.092,.055),(-.086,.25),(-.068,.53),(-.039,.79),(-.015,.97),
                  (-.003,1),(.012,.993),(.029,.89),(.055,.66),(.077,.34),(.085,.045)], n, 'Hull')]
    for i, (r, z) in enumerate([(.985,-.008),(.9,-.025),(.57,-.069),(.30,-.085)]):
        out.append(lathe('Turned_concentric_girdle_' + str(i),
                         [(z-.0022,r-.0015),(z-.0016,r+.0019),(z+.0016,r+.0019),(z+.0022,r-.0015)], n, 'Ribs'))
    # Recessed segmented underside casings, independent radial cooling vanes and small light wells.
    for i in range(48):
        a = i*2*math.pi/48
        out.append(wedge('Underside_radial_casing_%02d'%i,.36,.72,a+.008,a+.096,-.062,.004,'Ribs',3 if lod==0 else 2))
        out.append(wedge('Underside_dark_louvre_%02d'%i,.745,.815,a+.015,a+.074,-.035,.003,'Recess',2))
        out.append(wedge('Rim_recess_%02d'%i,.976,.987,a+.024,a+.094,-.014,.0026,'Recess',2))
        if i % 3 == 0:
            out.append(wedge('Small_recessed_running_light_%02d'%i,.982,.990,a+.034,a+.059,-.0158,.001,'Light',2))
    out.append(lathe('Central_recessed_optical_well',[(-.096,.04),(-.094,.13),(-.089,.14),(-.087,.04)], max(32,n//2),'Recess'))
    out.append(lathe('Central_dim_prismatic_lens',[(-.097,.024),(-.096,.039),(-.094,.039)], max(24,n//2),'Light'))
    for s in out:
        assert s.volume() > 0, (s.name, 'closed solid inward/degenerate', s.volume())
        assert len(s.triangles) == len(s.uv)
    return out
