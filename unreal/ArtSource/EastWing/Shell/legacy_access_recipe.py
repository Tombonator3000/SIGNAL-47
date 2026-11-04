"""Pure source selectors for three relocated drums; immutable glTF coordinates."""
SOURCE_SHA='fd60fa957655d289cd05909020f2f39f85e8a8e9692f3c402d9418f9b3d1a5f9'
DX=-2.6
DRUM_BOUNDS=[(21.005,21.595,-.601,.311,6.455,7.045),
             (21.650,22.250,-.601,.311,6.300,6.900),
             (21.780,22.808,-.611,-.009,7.091,7.921)]
FEET=[[21.,21.6,6.45,7.05],[21.65,22.25,6.3,6.9],
      [21.781442517895076,22.818557482104925,7.080414960264715,7.919585039735285]]

def moved(name,points):
    if name=='g454':
        hits=[i for i,b in enumerate(DRUM_BOUNDS)
              if all(all(b[2*k]<=p[k]<=b[2*k+1] for k in range(3)) for p in points)]
        assert len(hits)<=1
        return hits[0] if hits else None
    # Contacts are separate planar quads. Select all corners using exact original bounds.
    bounds=[(20.849,21.751,6.299,7.201),(21.499,22.401,6.149,7.051),
            (21.747,22.853,6.831,8.169)]
    hits=[i for i,b in enumerate(bounds) if all(abs(p[1]+.588)<.00001 and
          b[0]<=p[0]<=b[1] and b[2]<=p[2]<=b[3] for p in points)]
    assert len(hits)<=1
    return hits[0] if hits else None

# EASTWING-01 (Claude): the old east-wall rain leader at z2.2 crossed the new east service door
# (z1.2..2.3); remove it and the streak stain it left on the wall.
def downpipe(points):
    return all(38.55<=x<=38.80 and 0.<=y<=3.62 and 2.08<=z<=2.32 for x,y,z in points)

def downpipe_stain(points):
    return all(38.58<=x<=38.64 and 0.<=y<=3.62 and 1.75<=z<=2.65 for x,y,z in points)

def removed(name,points,uvs):
    if name=='g455' and downpipe_stain(points):return 'downpipe_stain'
    if name=='g454' and downpipe(points):return 'downpipe'
    if name=='g455':
        return 'grime' if all(20.1<=x<=39.1 and -.7<=y<=3.7 and abs(abs(z)-5.512)<.0001 for x,y,z in points) and (all(p[2]>0 for p in points) or all(p[2]<0 for p in points)) else None
    def car(p):
        x,y,z=p
        return y>-.57 and ((25.8<=x<=28.2 and 6.6<=z<=12.6) or (29.8<=x<=32.2 and 6.55<=z<=13.25))
    if all(car(p) for p in points):return 'cars'
    for side in (-1,1):
        for k in range(8):
            a=21+2.25*k
            if all(a-.061<=x<=a+1.511 and .529<=y<=2.387 and
                   (5.4999<=z<=5.5501 if side>0 else -5.5501<=z<=-5.4999) for x,y,z in points):return 'windows'
    if all(19.8<=x<=40 and -1<=y<=.1 and 5.6<=z<=6.6 and abs(w-.1875)<.0001 and
           (abs(u-.5625)<.0001 or abs(u-.6875)<.0001) for (x,y,z),(u,w) in zip(points,uvs)):return 'plants'
    if all(y<-.1 and ((25.8<=x<=28.2 and 6.6<=z<=12.6) or (29.8<=x<=32.2 and 6.55<=z<=13.25)) and
           ((abs(u-.9375)<.0001 and abs(w-.9375)<.0001) or (abs(u-.3125)<.0001 and abs(w-.1875)<.0001))
           for (x,y,z),(u,w) in zip(points,uvs)):return 'wheel_remnants'
    return None
