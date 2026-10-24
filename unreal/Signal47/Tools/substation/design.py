"""Original Caliche Junction Substation, source-only physical authoring contract."""
REVISION='R1-original-Caliche-Junction-1986'
SITE_WORLD_THREE=[7890,-1.10,536.20]
WORLD_YAW_DEGREES=180
PARTS={
 'SM_Substation_Transformer':{'offset':[0,0,0],'role':'oil tank, separate radiator banks, porcelain, conservator, valves'},
 'SM_Substation_Buswork':{'offset':[0,0,0],'role':'galvanized rear support, three phase conductors and insulators'},
 'SM_Substation_Control':{'offset':[0,0,0],'role':'external service cabinet and original printed front'},
 'SM_Substation_StatusLamp':{'offset':[4.62,-8.925,1.70],'role':'separate amber lens, native material state'},
 'SM_Substation_Lever':{'offset':[5.52,-8.94,1.15],'role':'separate pivoted mechanical handle; native state'},
 'SM_Substation_Fence':{'offset':[0,0,0],'role':'posts, rails, real geometric chainlink; south gate gap'},
 'SM_Substation_Gate_L':{'offset':[-2.2,-7.2,0],'role':'closed left leaf, hinge pivot, optional native rotation'},
 'SM_Substation_Gate_R':{'offset':[2.2,-7.2,0],'role':'closed right leaf, hinge pivot, optional native rotation'},
 'SM_Substation_Pad':{'offset':[0,0,0],'role':'transformer foundation, external cabinet plinth and sloped apron'},
 'SM_Substation_Gravel':{'offset':[0,0,0],'role':'20x16m gravel bed with buried0.65m contact skirt'},
}
FLOORS={'transformer_pad':{'x':[-4,4],'y':[-3,3],'top_z':0},
 'control_plinth':{'x':[4.1,5.9],'y':[-9.25,-7.75],'top_z':0},
 'control_apron':{'x':[3.2,6.8],'y':[-14,-9.25],'z_at_y_ends':[-.25,0]},
 'gravel':{'x':[-10,10],'y':[-8,8],'top_z':-.08,'buried_bottom_z':-.65}}
ATLAS_CELLS={'station':0,'danger':1,'nameplate':2,'service':3,'panel':4,'temperature':5,'oil_level':6,'serial':7}
BUDGET_TRIANGLES=110000
