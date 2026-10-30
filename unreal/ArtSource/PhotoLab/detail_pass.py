"""Original fixed joinery, fasteners and manufacturing details for the ten lab parts.

Pure helper calls, no bpy, scene reset, exports, lights or interaction objects.
Coordinates retain the original PhotoLab's (x,height,z) metres and assembly anchors.
"""
import math


def append_detail(part, box, cyl, ring, pipe, mats):
    enamel, cream, steel, dark, rubber, paper, ivory = (
        mats[k] for k in ('AgedGreyGreen','CreamEnamel','BrushedStainless',
                         'Bakelite','Rubber','Manila','DialIvory'))

    def screw(name, at, axis='Z', radius=.003):
        # A shallow countersunk head and a true narrow dark slot, no floating decal.
        index = {'X':0,'Y':1,'Z':2}[axis[-1]]
        sign = -1 if axis.startswith('-') else 1
        a, b, face = list(at), list(at), list(at)
        a[index] -= sign*.0006; b[index] += sign*.0006; face[index] += sign*.0007
        cyl(name, a, b, radius, steel, 12)
        dimensions = [.00065,.00065,.00065]
        dimensions[(index+1)%3] = radius*1.35
        box(name+'_slot', face, dimensions, dark, 0)

    def collar(name, at, outer, inner, depth, axis='Y', mat=None):
        ring(name, at, outer, inner, depth, mat or steel, axis=axis, sides=20)

    if part == 'SM_PhotoLab_WetCabinetDetail':
        for i in range(6):
            x = 13.45+i*.70
            # Folded returns and a recessed label frame distinguish formed steel
            # from the original simple inset panel. Keep the live bench top clear.
            for sx in (-1,1):
                box('Cabinet_folded_return',(x+sx*.327,.43,-5.102),(.006,.702,.013),enamel,.001)
                for y in (.17,.69):
                    screw('Cabinet_hinge_leaf_fixing',(x+sx*.307,y,-5.084),radius=.0027)
            box('Cabinet_label_recess',(x,.59,-5.093),(.112,.039,.003),dark,.001)
            box('Cabinet_label_stock',(x,.59,-5.0905),(.095,.025,.001),ivory,.0003)
            for dx in (-.054,.054):
                box('Cabinet_label_retainer',(x+dx,.59,-5.089),(.005,.038,.002),steel,.0005)
            for hx in (x-.045,x+.045):
                collar('Pull_rose',(hx,.73,-5.087),.011,.007,.002,axis='Z')
    elif part == 'SM_PhotoLab_RinseSink':
        x,z = 17.78,-5.44
        collar('Drain_compression_ring',(x,.686,z),.029,.022,.013)
        collar('Trap_union',(x,.53,z),.028,.022,.020)
        collar('Faucet_escutcheon',(x,.938,-5.69),.031,.013,.008)
        collar('Open_aerator_rim',(x,1.14,-5.46),.017,.011,.012)
        for dx in (-.13,.13):
            collar('Valve_base_rose',(x+dx,.941,-5.68),.029,.016,.006)
        for sx in (-1,1):
            for sz in (-1,1):
                collar('Leg_adjustment_nut',(x+sx*.32,.055,z+sz*.22),.022,.018,.014)
        for dx in (-.27,.27):
            screw('Splashback_wall_fixing',(x+dx,1.07,-5.722),radius=.004)
        # Two seams at the bowl floor, below all original open-bowl test rays.
        for dx in (-.229,.229):
            cyl('Bowl_weld_seam',(x+dx,.730,z-.145),(x+dx,.730,z+.145),.001,steel,8)
    elif part == 'SM_PhotoLab_EnlargerDetail':
        # Annular bellows ridges leave the original optical path completely open.
        for y in (1.565,1.575,1.585):
            collar('Bellows_fold',(17,y,-5.45),.067,.052,.004,mat=dark)
        for y in (1.56,1.58):
            for i in range(20):
                a=i*math.tau/20
                cyl('Lens_ring_grip',(17+.063*math.cos(a),y-.004,-5.45+.063*math.sin(a)),
                    (17+.063*math.cos(a),y+.004,-5.45+.063*math.sin(a)),.0014,dark,6)
        for x in (16.905,17.095):
            box('Lamp_head_folded_seam',(x,1.887,-5.47),(.003,.004,.236),dark,.0004)
        box('Carrier_finger_recess',(16.822,1.611,-5.46),(.037,.002,.027),dark,.001)
        for y in (1.03,1.37,1.70):
            screw('Column_clamp_fixing',(17.046,y,-5.603),'X',.0035)
        collar('Focus_bearing',(17.142,1.53,-5.60),.017,.010,.006,axis='X')
    elif part == 'SM_PhotoLab_ProcessDetails':
        # Timer details stay on the existing bezel; they do not replace its state.
        collar('Timer_dial_lip',(16.825,1.007,-5.497),.049,.046,.002,axis='Z')
        cyl('Timer_hand_pivot',(16.825,1.007,-5.495),(16.825,1.007,-5.491),.004,dark,12)
        for x in (16.783,16.917):
            screw('Timer_case_fastener',(x,.953,-5.504),radius=.0024)
        for x in (14.0,14.7,15.4):
            for sx in (-1,1):
                for sz in (-1,1):
                    box('Tray_corner_seam',(x+sx*.248,.991,-5.4+sz*.185),(.012,.004,.026),rubber,.0008)
        collar('Cylinder_rolled_rim',(16.08,1.127,-5.70),.025,.020,.005,mat=cream)
        for i in range(6):
            box('Cylinder_half_graduation',(16.08,.99+i*.02,-5.674),(.006,.001,.001),dark,0)
    elif part == 'SM_PhotoLab_StorageDetail':
        for yy in (.5,1.1,1.7):
            for zz in (-5.3,-4.3,-3.3):
                for y in (yy-.165,yy-.047):
                    screw('Shelf_bracket_fixing',(12.865,y,zz),'X',.003)
        for zz,yy in ((-4.20,1.12),(-4.43,1.12),(-4.18,1.72)):
            box('Carton_lid_fold',(13.0,yy+.129,zz),(.177,.002,.162),paper,.0005)
            box('Carton_fold_shadow',(13.092,yy+.108,zz),(.001,.002,.153),dark,0)
            for dy in (-.010,0,.010):
                box('Inventory_label_rule',(13.097,yy+.078+dy,zz),(.0007,.001,.064),dark,0)
        for zz in (-4.13,-4.36):
            for i in range(16):
                a=i*math.tau/16
                cyl('Cap_moulded_rib',(13+.0245*math.cos(a),.754,zz+.0245*math.sin(a)),
                    (13+.0245*math.cos(a),.779,zz+.0245*math.sin(a)),.0017,dark,6)
            collar('Bottle_base_seam',(13,.525,zz),.0525,.049,.003,mat=dark)
    elif part == 'SM_PhotoLab_ArchiveDetail':
        for zz in (-4.10,-1.90):
            for y in (.648,.752):
                for dz in (-.19,.19):
                    screw('Archive_drawer_fold_fixing',(18.212,y,zz+dz),'-X',.0027)
            box('Drawer_bottom_fold',(18.242,.615,zz),(.010,.007,.43),steel,.001)
        for i in range(5):
            x=18.56+i*.064
            box('Sleeve_folded_spine',(x,1.016,-4.521),(.014,.213,.003),ivory,.0005)
            box('Sleeve_index_tab',(x,1.116,-4.40),(.014,.012,.039),ivory,.001)
    elif part == 'SM_PhotoLab_RecordsDetail':
        for xx in (15.73,18.27):
            for zz in (.13,.70):
                screw('Records_leg_rivet',(xx,.677,zz+.013),radius=.0028)
            box('Desk_underside_bracket',(xx,.696,.42),(.085,.033,.56),steel,.002)
        for i in range(3):
            x=17.67+i*.08
            box('Envelope_folded_flap',(x,.995,.58),(.018,.018,.205),paper,.001)
            box('Envelope_reinforced_edge',(x,.913,.475),(.018,.193,.003),ivory,.0005)
    elif part == 'SM_PhotoLab_SafelightHousing':
        for xx in (14.826,15.174):
            box('Safelight_case_gasket',(xx,2.45,-5.548),(.012,.15,.002),rubber,.0005)
        collar('Safelight_cable_gland',(15,2.55,-5.70),.013,.008,.010)
        for zz in (-2.535,-2.265):
            for xx in (15.815,15.985):
                screw('Ceiling_case_fastener',(xx,2.673,zz),'Y',.0028)
        for zz in (-5.45,-4.25,-3.05):
            box('Ceiling_conduit_clip',(15.9,2.736,zz),(.026,.009,.016),steel,.001)
    elif part == 'SM_PhotoLab_Ventilation':
        for xx in (18.335,18.765):
            box('Extractor_case_fold',(xx,2.24,-5.735),(.007,.405,.011),cream,.001)
        for yy in (1.281,1.379):
            screw('Extractor_switch_cover_fixing',(18.84,yy,-5.703),radius=.0025)
        collar('Extractor_conduit_gland',(18.84,1.397,-5.76),.014,.009,.010)
    elif part == 'SM_PhotoLab_UtilityDetail':
        for xx in (15.0,15.9,18.84):
            box('Junction_cover_seam',(xx,2.72,-5.738),(.078,.057,.001),dark,.001)
            box('Junction_removable_cover',(xx,2.72,-5.7365),(.074,.053,.002),cream,.001)
        for xx in (17.91,18.20,18.48):
            for dx in (-.008,.008):
                screw('Pipe_clip_wall_fastener',(xx+dx,.45,-5.766),radius=.002)
        for yy in (.5,1.1,1.7):
            for zz in (-5.515,-3.085):
                for xx in (12.87,13.13):
                    screw('Shelf_end_fold_rivet',(xx,yy-.014,zz+.008),radius=.0023)
    else:
        raise ValueError('No detail pass for '+part)
