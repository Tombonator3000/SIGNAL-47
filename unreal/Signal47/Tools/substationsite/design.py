"""Audited site additions. Blender metres, Zup; common parent matches substation."""
REVISION='R1-original-substation-site-1986'
SITE_WORLD_THREE=[7890,-1.10,536.20]
WORLD_YAW_DEGREES=180
PHASE_X=[-1.04,0,1.04]
PHASE_HEIGHT=9.2
POLES={'A':[0,21.8,-.240012],'B':[0,53.8,-.228311],'C':[0,85.8,-.301798]}
GUY_ANCHOR=[0,90.0,-.342236688]
SHED=[6.5,4.9,0]
EQUIPMENT=[6.8,2.65,-.08]
PARTS={**{'SM_SubstationSite_Pole_'+k:{'offset':v,'role':'weathered timber pole, crossarm, three ceramic phases, hardware'} for k,v in POLES.items()},
 'SM_SubstationSite_Shed':{'offset':SHED,'role':'locked corrugated maintenance shed'},
 'SM_SubstationSite_Equipment':{'offset':EQUIPMENT,'role':'cable drums and old toolbox'},
 'SM_SubstationSite_Lines':{'offset':[0,0,0],'role':'three exact phase spans and terminal guy'},
 'SM_SubstationSite_Ground':{'offset':[0,0,0],'role':'sparse stone contact dressing on known gravel and pole foot'} }
SPANS=[{'from':'bus','to':'A','sag_m':.65},{'from':'A','to':'B','sag_m':.9},{'from':'B','to':'C','sag_m':.9}]
BUS_ENDPOINTS=[[x,4.65,7.85] for x in PHASE_X]
BUDGET_TRIANGLES=35000
GROUND_AUTHORITY='Native audited original geo_007.glb/g873; no new terrain plane or collider. Existing kit gravel topBz-.08.'
