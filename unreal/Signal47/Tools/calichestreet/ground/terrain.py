"""Height contract ported from roadShape/roadTerrain for this 70 m town cell.

No web files are modified. The restricted domain excludes the survey track,
old-road asphalt and diner entrance transitions. Float32 grid heights and
the original triangle split are preserved, as in S47RoadTerrain::GridY.
"""
import math
import struct
from functools import lru_cache

ANCHOR = [8000.0, 0.0, 2060.0]
REGION = [-30.0, 40.0, 2020.0, 2090.0]

def smooth(a,b,x):
    t=max(0,min(1,(x-a)/(b-a)))
    return t*t*(3-2*t)

def base(x,z):
    return 1.1*math.sin(x*.0061+.7)*math.cos(z*.0047+.3)+.7*math.sin((x+z)*.0032+1.9)+.4*math.sin(z*.011-x*.004+.4)

B0=base(-1.8,0)

def raw(x,z):
    dx=x+250; dz=z-300; a=math.atan2(dz,dx)
    hills=smooth(1300,2400,math.hypot(dx,dz))*(12+9*math.sin(a*5+1.3)+5*math.sin(a*13))
    wash=math.exp(-(x+330+.25*(z-500))**2/520)*smooth(-150,60,z)
    return base(x,z)-B0-1.3*wash+hills

DINER_Y=raw(-11.05,1999.95)

def hlow(x,z):
    r=raw(x,z); d=math.hypot(x+11.05,z-1999.95)
    return r if d>110 else r+(DINER_Y-r)*(1-smooth(45,110,d))

def old_bearing(mi):
    # This cell only needs the old road's first 640 m at mi < 0.4.
    lead=63+27*(1-smooth(.02,.3,mi)); m=mi-.3
    return lead+(8*math.sin(m*2.3)+4*math.sin(m*5.1+1))*smooth(.3,1,mi)*(1-smooth(4.6,5.5,mi))

OLD=[]
_x=0.; _z=1999.95; _s0=-math.floor(8.15*1609.34/4)*4
for _s in range(0,641,4):
    OLD.append((_x,_z))
    _a=old_bearing(8.15+(_s0+_s+2)/1609.34)*math.pi/180
    _x+=math.sin(_a)*4; _z-=math.cos(_a)*4

@lru_cache(maxsize=50000)
def old_near(x,z):
    if x < -2 or x > 670 or abs(z-1999.95)>120:return math.inf
    best=math.inf
    for i,(a,b) in enumerate(OLD[:-1]):
        c,d=OLD[i+1]; dx=c-a; dz=d-b
        t=((x-a)*dx+(z-b)*dz)/(dx*dx+dz*dz)
        if i==0 and t<0:continue
        t=max(0,min(1,t)); best=min(best,math.hypot(a+t*dx-x,b+t*dz-z))
    return best

@lru_cache(maxsize=400)
def grid_h(x,z):
    hd=abs(x); od=old_near(x,z)
    detail=.16*math.sin(x*.13+1.7)*math.sin(z*.11+.3)+.08*math.sin(x*.31-z*.23+.9)
    meet=smooth(40,90,math.hypot(x+11.05,z-1999.95))
    h=hlow(x,z)+detail*smooth(0,15,min(hd-18,od-11))*meet
    if hd<16.5:h-=.45
    return struct.unpack('<f',struct.pack('<f',h))[0]

def grid_y(x,z):
    a=math.floor(x/10)*10; b=math.floor(z/10)*10
    u=(x-a)/10; v=(z-b)/10
    h00=grid_h(a,b); h10=grid_h(a+10,b); h01=grid_h(a,b+10); h11=grid_h(a+10,b+10)
    return h00+(h10-h00)*u+(h01-h00)*v if u+v<=1 else h11+(h01-h11)*(1-u)+(h10-h11)*(1-v)

def floor_y(x,z):
    assert -30.001<=x<=40.001 and 2019.999<=z<=2090.001,(x,z)
    ax=abs(x)
    if ax<=6:
        lift=.06+.06*(1-(ax/4.8)**2) if ax<=4.8 else .06*(6-ax)/1.2
        return hlow(x,z)+lift
    y=grid_y(x,z)
    if ax<=18:y=max(y,hlow(x,z)+(0 if ax<=16.5 else -.6*(ax-16.5)/1.5))
    return y

def pavement(x,z):
    return (-17.6<=x<=-4.72 and 2040<=z<=2090) or (-17.6<=x<=-14.2 and z<=2040) or (16.6<=x<=20.2 and z>=2024)

# Full footprints are intentionally kept clear, including existing door ramps.
BUILDINGS=[(-28.2,-16.4,2065.5,2088.7),(19.0,30.3,2029.5,2042.6),(17.9,30.3,2057.3,2070.7),(20.,36.,2089.,2090.)]
# A02's original and all later positions, studio photograph line, buzzer,
# payphone, Sentinel journal and door approaches. No upright dressing here.
PROTECTED=[(-18.6,-13.3,2062.,2072.3),(13.0,22.0,2028.,2047.5),(13.0,22.0,2057.0,2071.5)]

def in_box(x,z,b,pad=0):return b[0]-pad<=x<=b[1]+pad and b[2]-pad<=z<=b[3]+pad

def clear_surface(x,z):
    return not (abs(x)<4.45 or pavement(x,z) or any(in_box(x,z,b,.15) for b in BUILDINGS))

def clear_dressing(x,z,radius=.0):
    # The full main walking rectangle, parking apron, road and sidewalks are
    # free of new stones/vegetation, regardless of their collision setting.
    if -18.3-radius<=x<=20.9+radius:return False
    return not any(in_box(x,z,b,.75+radius) for b in BUILDINGS+PROTECTED)

def blender_point(x,z,lift=.008):return (x,-(z-2060),floor_y(x,z)+lift)
