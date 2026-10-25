"""Original editable station architecture; pure Python, no DCC or GPU side effects."""
import math
import random
import importlib.util
from pathlib import Path

_path = Path(__file__).resolve().parent.parent/'substation'/'meshkit.py'
_spec = importlib.util.spec_from_file_location('s47_fieldstation_base', _path)
base = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(base)
add, sub, mul, cross, unit = base.add, base.sub, base.mul, base.cross, base.unit

class Mesh(base.Mesh):
    def face(self, ps, mat, uv=None, smooth=False):
        super().face(ps,mat,uv,smooth)
        self.mat[-1] = self.mat[-1].replace('M_Substation_', 'M_FieldStation_',1)

    def merge(self, other, offset=(0,0,0), yaw=0):
        start=len(self.v);c,s=math.cos(yaw),math.sin(yaw)
        self.v.extend((offset[0]+p[0]*c-p[1]*s,offset[1]+p[0]*s+p[1]*c,offset[2]+p[2]) for p in other.v)
        self.f.extend([[start+i for i in f] for f in other.f])
        for key in ['mat','uv','smooth','tags']:getattr(self,key).extend(getattr(other,key))

    def label(self,name,center,width,height,cell,normal):
        self.tag=name;n=unit(normal);right=unit(cross((0,0,1),n));up=cross(n,right)
        ps=[add(center,add(mul(right,a*width/2),mul(up,b*height/2))) for a,b in [(-1,-1),(1,-1),(1,1),(-1,1)]]
        col,row=cell%2,cell//2;u0=(col+.025)/2;u1=(col+.975)/2;v0=1-(row+.975)/4;v1=1-(row+.025)/4
        self.face(ps,'PrintAtlas',[(u0,v0),(u1,v0),(u1,v1),(u0,v1)])

def B(x,y,z): return (x,-z,y)
def box3(m,name,p,size,mat):m.box(name,B(*p),(size[0],size[2],size[1]),mat)
def tube3(m,name,a,b,r,mat,n):m.tube(name,B(*a),B(*b),r,mat,n)
def seg(lod):return [20,12,8][lod]
def bound_box(m,name,x0,x1,y0,y1,z0,z1,mat):
    if min(x1-x0,y1-y0,z1-z0)>1e-7:box3(m,name,((x0+x1)/2,(y0+y1)/2,(z0+z1)/2),(x1-x0,y1-y0,z1-z0),mat)

def wall(m,side,start,end,holes,lod):
    """Partition masonry at aperture edges, with independent interior facing."""
    cuts=sorted(set([start,end]+[v for h in holes for v in h[:2]]))
    for a,b in zip(cuts,cuts[1:]):
        spans=sorted(h[2:] for h in holes if h[0]<(a+b)/2<h[1]);last=0
        for lo,hi in spans+[[2.6,2.6]]:
            if lo>last:
                if side=='south':bound_box(m,'Real_stucco_aperture_south',a,b,last,lo,4.5,4.7,'Stucco')
                if side=='north':bound_box(m,'Old_survey_north_wall',a,b,last,lo,1.3,1.5,'Stucco')
                if side=='west':bound_box(m,'Old_survey_west_wall',-13.2,-13,last,lo,a,b,'Stucco')
                if side=='east':bound_box(m,'Real_stucco_aperture_east',-9,-8.8,last,lo,a,b,'Stucco')
            last=max(last,hi)

def panel3(m,name,points,mat):
    m.tag=name;m.face([B(*p) for p in points],mat)

def lining(m,side,start,end,holes):
    cuts=sorted(set([start,end]+[v for h in holes for v in h[:2]]))
    for a,b in zip(cuts,cuts[1:]):
        spans=sorted(h[2:] for h in holes if h[0]<(a+b)/2<h[1]);last=0
        for lo,hi in spans+[[2.597,2.597]]:
            for bot,top,mat in [(last,min(lo,.9),'Dado'),(max(last,.9),lo,'Plaster')]:
                if top-bot<1e-5:continue
                if side=='south':p=[(a,bot,4.497),(a,top,4.497),(b,top,4.497),(b,bot,4.497)]
                if side=='north':p=[(a,bot,1.503),(b,bot,1.503),(b,top,1.503),(a,top,1.503)]
                if side=='west':p=[(-12.997,bot,a),(-12.997,top,a),(-12.997,top,b),(-12.997,bot,b)]
                if side=='east':p=[(-9.003,bot,a),(-9.003,bot,b),(-9.003,top,b),(-9.003,top,a)]
                panel3(m,'Mineral_wash_above_original_dark_dado',p,mat)
            if last<.9<lo:
                if side in ['south','north']:box3(m,'Dado_top_timber_rail',((a+b)/2,.9,4.484 if side=='south' else 1.516),(b-a,.035,.028),'DarkWood')
                else:box3(m,'Dado_top_timber_rail',(-12.984 if side=='west' else -9.016,.9,(a+b)/2),(.028,.035,b-a),'DarkWood')
            last=max(last,hi)

def window(m,glass,lod,side):
    south=side=='south';w,h=1.,.9;cx,cy,cz=(-11.8,1.4,4.6) if south else(-8.9,1.4,2.8)
    def at(u,v,d):return(cx+u,cy+v,cz+d) if south else(cx+d,cy+v,cz+u)
    def bx(name,u,v,d,su,sv,sd,mat):
        box3(m,name,at(u,v,d),(su,sv,sd) if south else(sd,sv,su),mat)
    for u in [-.47,.47]:bx('Painted_timber_window_jamb',u,0,0,.06,.9,.15,'TealPaint')
    for v in [-.42,.42]:bx('Painted_timber_window_rail',0,v,0,1.,.06,.15,'TealPaint')
    bx('Window_central_wood_mullion',0,0,.013,.033,.79,.055,'TealPaint')
    bx('Window_central_wood_transom',0,0,.013,.89,.035,.055,'TealPaint')
    bx('Projecting_weathered_timber_drip_sill',0,-.474,.10,1.14,.05,.27,'Timber')
    bx('Bent_metal_head_flashing',0,.48,.07,1.15,.022,.24,'RustTin')
    bx('Dark_open_sill_rebate',0,-.442,-.015,.88,.016,.085,'DarkWood')
    for u in [-.23,.23]:
        for v in [-.21,.21]:
            pts=[at(u+a*.212,v+b*.184,.014) for a,b in [(-1,-1),(1,-1),(1,1),(-1,1)]]
            if not south:pts=pts[::-1]
            panel3(glass,'Independent_thin_actual_glass_pane',pts,'Glass')
            if lod==0:
                for vv in [-.194,.194]:bx('Thin_glazing_putty',u,v+vv,.027,.423,.012,.012,'WhitePaint')
    if lod<2:
        for u in [-.46,.46]:
            for v in [-.30,.30]:
                # Tiny square cut-nail heads remain physical at LOD0/1 only.
                bx('Oxidized_window_fastener',u,v,.081,.012,.012,.005,'RustTin')

def shell(lod):
    m=Mesh('SM_FieldStation_Hut_Shell');g=Mesh('SM_FieldStation_Hut_Glass');n=seg(lod)
    box3(m,'Existing_plinth_top_at_zero',(-11,-.125,3),(5,.25,4),'Concrete')
    box3(m,'Existing_doorstep_top_minus_point07',(-9.95,-.16,5.21),(1.4,.18,.42),'Concrete')
    wall(m,'north',-13.2,-8.8,[],lod);wall(m,'west',1.5,4.5,[],lod)
    wall(m,'south',-13.2,-8.8,[[-12.3,-11.3,.95,1.85],[-10.5,-9.4,0,2]],lod)
    wall(m,'east',1.5,4.5,[[2.3,3.3,.95,1.85]],lod)
    # Finite low roof silhouette: roll-felt overlaps and folded edge flashing.
    box3(m,'Old_flat_roof_deck',(-11,2.68,3),(4.9,.16,3.9),'RoofFelt')
    for j in range([9,6,3][lod]):
        z=1.05+(j+.5)*3.9/[9,6,3][lod]
        box3(m,'Overlapped_roll_roof_seam',(-11,2.767,z),(4.86,.012,.028),'DarkMetal')
    for z in [1.045,4.955]:
        box3(m,'Oxidized_roof_edge_drip',(-11,2.66,z),(4.94,.115,.025),'RustTin')
        box3(m,'Folded_roof_edge_lip',(-11,2.724,z),(4.96,.022,.055),'Metal')
    for x in [-13.465,-8.535]:
        box3(m,'Oxidized_roof_edge_return',(x,2.66,3),(.025,.115,3.94),'RustTin')
        box3(m,'Folded_roof_return_lip',(x,2.724,3),(.055,.022,3.96),'Metal')
    m.lathe('Existing_stovepipe',B(-12.4,2.76,2),[(0,.075),(.63,.075),(.68,.10)],'DarkMetal',n)
    m.lathe('Stovepipe_vented_rain_cap',B(-12.4,3.35,2),[(0,.16),(.07,.045)],'RustTin',n,caps=False)
    box3(m,'Stovepipe_base_flashing',(-12.4,2.776,2),(.36,.016,.36),'RustTin')
    tube3(m,'Existing_service_weatherhead_mast',(-9.05,2.65,4.45),(-9.05,3.58,4.45),.03,'Metal',n)
    tube3(m,'Weatherhead_gooseneck',(-9.05,3.58,4.45),(-8.98,3.64,4.45),.046,'DarkMetal',n)
    tube3(m,'Old_electrical_service_conduit',(-8.77,1.72,4.25),(-8.77,2.55,4.25),.016,'Metal',max(6,n//2))
    box3(m,'Existing_folded_meter_base',(-8.75,1.55,4.25),(.10,.32,.24),'Metal')
    m.lathe('Round_utility_meter_rim',B(-8.695,1.58,4.25),[(0,.075),(.035,.075)],'DarkMetal',n,axis=(1,0,0))
    m.lathe('Weathered_meter_face',B(-8.658,1.58,4.25),[(0,.065),(.003,.065)],'Ceramic',n,axis=(1,0,0))
    if lod<2:
        for x in [-13.35,-12.6,-11.8,-11,-10.2,-9.4,-8.65]:
            for z in [1.04,4.96]:box3(m,'Roof_drip_cut_nail',(x,2.68,z),(.012,.012,.007),'DarkMetal')
    # Door reveals remain exactly within the original opening.
    for x in [-10.475,-9.425]:box3(m,'Door_reveal_timber_jamb',(x,1.,4.6),(.05,2.,.26),'TealPaint')
    box3(m,'Door_head_joinery',(-9.95,2.035,4.6),(1.14,.07,.26),'TealPaint')
    for x in [-10.54,-9.36]:box3(m,'External_door_casing',(x,1.04,4.716),(.08,2.08,.032),'TealPaint')
    box3(m,'External_door_head_casing',(-9.95,2.10,4.716),(1.26,.09,.032),'TealPaint')
    box3(m,'Worn_oak_threshold_top_point025',(-9.95,.0125,4.6),(1.,.025,.24),'Timber')
    window(m,g,lod,'south');window(m,g,lod,'east')
    # Original wall-lamp and all live light sources are retained by the integrator.
    box3(m,'Lamp_mount_timber_pattress',(-9.95,2.48,4.712),(.16,.18,.025),'DarkWood')
    box3(m,'Station01_mounted_enamel_plaque',(-9.95,2.262,4.718),(.64,.205,.026),'DarkMetal')
    m.label('STATION01_legible_original_label',B(-9.95,2.262,4.734),.60,.1875,0,(0,-1,0))
    box3(m,'Electrical_junction_box',(-9.1,1.25,4.76),(.26,.32,.12),'OliveEnamel')
    m.label('Junction_original_120volt_label',B(-9.1,1.27,4.824),.20,.125,1,(0,-1,0))
    tube3(m,'Junction_riser',(-9.1,.04,4.74),(-9.1,1.09,4.74),.025,'Metal',max(6,n//2))
    if lod<2:
        for yy in [.22,.72]:box3(m,'Galvanized_conduit_saddle',(-9.1,yy,4.766),(.075,.024,.013),'Metal')
        # Local plaster loss is modelled in the blank wall regions only.
        rng=random.Random(1947)
        for k in range([27,12,0][lod]):
            x=rng.uniform(-13.08,-10.7);y=rng.uniform(.10,.60);w=rng.uniform(.035,.19);h=rng.uniform(.025,.10)
            box3(m,'Exposed_old_masonry_chip',(x,y,4.702),(w,h,.007),'Masonry')
        for k in range([10,4,0][lod]):
            x=-13.03+k*.243
            tube3(m,'Hairline_stucco_crack',(x,.62,4.706),(x+.055,.81,4.706),.0028,'WeatherStain',4)
    # Contact grit sits on the plinth perimeter, never over the open doorway.
    if lod<2:
        rng=random.Random(8001)
        for k in range([38,15,0][lod]):
            x=rng.uniform(-13.45,-8.57);z=rng.choice([1.05,4.94])
            if z>4 and -10.7<x<-9.15:continue
            s=rng.uniform(.018,.055);box3(m,'Small_contact_mineral_grit',(x,s*.22,z+rng.uniform(-.025,.025)),(s,s*.42,s*.7),'Masonry')
    return m,g

def interior(lod):
    m=Mesh('SM_FieldStation_Hut_Interior')
    lining(m,'north',-13.,-9.,[]);lining(m,'west',1.5,4.5,[])
    lining(m,'south',-13.,-9.,[[-12.3,-11.3,.95,1.85],[-10.5,-9.4,0,2.]])
    lining(m,'east',1.5,4.5,[[2.3,3.3,.95,1.85]])
    # Authoritative floor height .004, matching the old mesh, with seams recessed.
    count=[22,15,10][lod]
    for k in range(count):
        x=-13+(k+.5)*4/count
        box3(m,'Worn_individual_floorboard',(x,-.007,3),(4/count-.002,.022,3),'FloorTimber' if k%5 else 'DarkWood')
        if lod==0:
            for z in [1.57,4.43]:box3(m,'Floor_cut_nail_head',(x,.0043,z),(.006,.001,.006),'DarkMetal')
    box3(m,'Old_limewash_ceiling',(-11,2.5795,3),(4,.035,3),'Ceiling')
    for z in [2.2,3.8]:box3(m,'Exposed_existing_roof_joist',(-11,2.54,z),(4,.12,.08),'Timber')
    for x in [-12.97,-9.03]:box3(m,'Timber_baseboard',(x,.060,3),(.045,.112,3),'DarkWood')
    box3(m,'North_timber_baseboard',(-11,.060,1.53),(4,.112,.045),'DarkWood')
    for a,b in [(-13,-10.5),(-9.4,-9)]:box3(m,'South_baseboard_stops_at_door',((a+b)/2,.060,4.47),(b-a,.112,.045),'DarkWood')
    for x in [-10.475,-9.425]:box3(m,'Door_inside_timber_jamb',(x,1.,4.481),(.065,2.,.04),'TealPaint')
    box3(m,'Door_inside_head_timber',(-9.95,2.034,4.481),(1.17,.065,.04),'TealPaint')
    # Small wiring run follows the old ceiling, avoiding all proxy/diagram faces.
    tube3(m,'Ceiling_cloth_service_line',(-12.96,2.48,2.55),(-11.,2.48,2.55),.005,'Rubber',6)
    if lod<2:
        for x in [-12.8,-12.4,-12.,-11.6,-11.2]:box3(m,'Old_wiring_ceramic_clip',(x,2.484,2.55),(.024,.012,.026),'Ceramic')
        # Grounded staining, not emissive edge decoration.
        for x in [-12.92,-12.53,-11.62,-9.14]:box3(m,'Subtle_plaster_repair',(x,2.35,1.506),(.035,.12,.004),'PlasterRepair')
    return m

def chair(lod):
    c=Mesh('Original_survey_chair');n=seg(lod)
    # Local chair faces toward the north-wall bench, with slatted timber seat.
    for i in range([5,4,3][lod]):
        x=-.21+(i+.5)*.42/[5,4,3][lod]
        c.rounded_box('Old_survey_seat_slat',(x,0,.455),(.42/[5,4,3][lod]-.005,.405,.028),.012,'Timber',2)
    for x in [-.175,.175]:
        for y in [-.165,.165]:c.tube('Worn_chair_tapered_leg',(x*1.12,y*1.12,.012),(x,y,.44),.0175,'DarkWood',max(6,n//2))
        c.tube('Chair_back_upright',(x,-.166,.42),(x,-.19,.88),.019,'Timber',max(6,n//2))
        c.tube('Chair_side_stretcher',(x,-.175,.22),(x,.175,.22),.012,'DarkWood',6)
    for zz in [.65,.81]:c.rounded_box('Shaped_survey_chair_back',(0,-.188,zz),(.42,.030,.065),.015,'Timber',2)
    c.tube('Front_chair_stretcher',(-.175,.175,.23),(.175,.175,.23),.012,'DarkWood',6)
    return c

def props(lod):
    m=Mesh('SM_FieldStation_Hut_Props');n=seg(lod)
    # Keep the exact old cabinet collision envelope x[-13,-12.5],z[3.43,4.37].
    box3(m,'Folded_sheet_cupboard_carcass',(-12.76,.89,3.9),(.46,1.78,.9),'OliveEnamel')
    for z in [3.677,4.123]:
        box3(m,'Independent_rolled_cupboard_door',(-12.523,.91,z),(.026,1.67,.421),'OliveEnamel')
        box3(m,'Cupboard_door_edge_reveal',(-12.507,.91,z-.209),(.008,1.67,.010),'DarkMetal')
        for y in [.30,1.48]:box3(m,'Cupboard_visible_hinge',(-12.495,y,z+.19),(.025,.055,.027),'Metal')
        tube3(m,'Cupboard_bent_pull',(-12.475,.96,z),(-12.475,1.10,z),.009,'Metal',max(6,n//2))
        if lod<2:
            for q in range(7):box3(m,'Cupboard_pressed_vent',(-12.507,.20+q*.033,z),(.006,.010,.21),'DarkMetal')
    m.label('Cupboard_archival_service_label',B(-12.505,1.46,3.68),.18,.065,2,(1,0,0))
    for z in [3.49,4.31]:box3(m,'Cupboard_masonry_isolation_foot',(-12.76,.024,z),(.39,.048,.045),'Rubber')
    m.merge(chair(lod),B(-12.,0,3.05),yaw=.6)
    # The original shelf is kept above the west end of the other agent's bench.
    box3(m,'Wall_shelf_at_original_height',(-12.5,1.48,1.63),(.9,.03,.26),'Timber')
    for x in [-12.9,-12.1]:
        box3(m,'Shelf_fixed_bracket',(x,1.4,1.6),(.028,.16,.20),'DarkMetal')
        tube3(m,'Shelf_diagonal_brace',(x,1.32,1.51),(x,1.46,1.72),.010,'DarkMetal',6)
    for k in range(5):
        x=-12.84+k*.16;h=.16+(k%3)*.04
        box3(m,'Bound_spare_valve_carton',(x,1.50+h/2,1.63),(.12,h,.12),'Cardboard' if k%2 else 'BookCloth')
        if lod<2:m.label('Archival_spares_carton_label',B(x,1.50+h/2,1.692),.085,.055,3,(0,-1,0))
    m.lathe('Original_shelf_tin_spares_can',B(-12.15,1.50,1.63),[(0,.055),(.16,.055),(.17,.059)],'Metal',n)
    if lod<2:
        for x in [-12.95,-12.7,-12.45,-12.2]:box3(m,'Shelf_front_cut_nail',(x,1.48,1.762),(.009,.009,.006),'DarkMetal')
    return m

def door(lod):
    """Local Three hinge [-9.45,0,4.48]; no initial yaw baked into vertices."""
    m=Mesh('SM_FieldStation_Hut_Door');n=seg(lod)
    # Original leaf spans Three x[-.98,0], y[.01,1.98], z[-.02,.02].
    box3(m,'Continuous_original_timber_door',(-.49,.995,0),(.98,1.97,.04),'DoorTimber')
    for x in [-.925,-.055]:box3(m,'Door_joinery_stile',(x,.995,-.027),(.095,1.94,.018),'TealPaint')
    for yy in [.10,.995,1.89]:box3(m,'Door_joinery_rail',(-.49,yy,-.027),(.85,.12,.018),'TealPaint')
    if lod<2:
        for x in [-.79,-.59,-.39,-.19]:box3(m,'Door_vertical_board_rebate',(x,.995,.021),(.006,1.83,.003),'DarkWood')
        box3(m,'Dull_kickplate',(-.49,.12,.026),(.79,.145,.008),'RustTin')
    for yy in [.30,.995,1.70]:
        tube3(m,'Door_hinge_barrel',(-.005,yy-.033,0),(-.005,yy+.033,0),.014,'Metal',max(6,n//2))
        box3(m,'Door_hinge_leaf',(-.031,yy,.022),(.06,.058,.006),'Metal')
    for s in [-1,1]:
        box3(m,'Door_handle_escutcheon',(-.90,1.0,s*.029),(.057,.125,.014),'DarkMetal')
        tube3(m,'Door_round_handle_spindle',(-.90,1.0,s*.035),(-.90,1.0,s*.080),.011,'Metal',max(6,n//2))
        tube3(m,'Door_rounded_lever',(-.90,1.0,s*.080),(-.82,1.0,s*.080),.012,'Metal',max(6,n//2))
    return m

def build(lod=0):
    sh,gl=shell(lod)
    return {m.name:m for m in [sh,interior(lod),gl,props(lod),door(lod)]}
