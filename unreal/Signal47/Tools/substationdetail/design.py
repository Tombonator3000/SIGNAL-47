"""Original additive weather protection and contact detail; not electrical installation guidance."""
REVISION='R1-Caliche-substation-contact-20261028'
SITE_WORLD_THREE=[7890,-1.10,536.20]
WORLD_YAW_DEGREES=180
PARTS={
 'SM_SubstationDetail_GroundShoulder':{'offset':[0,0,0],'role':'measured feathered gravel shoulder; excludes approach, gate, T02 and culvert'},
 'SM_SubstationDetail_InspectionFittings':{'offset':[0,0,0],'role':'open lamp bezel, rainhood, fixings; original lens retained'},
 'SM_SubstationDetail_ServiceConduit':{'offset':[0,0,0],'role':'cabinet side junction and conduit, strain relief and straps'},
 'SM_SubstationDetail_FoundationContact':{'offset':[0,0,0],'role':'foundation anchors, drip details and restrained grit on known surfaces'},
 'SM_SubstationDetail_GravelContact':{'offset':[0,0,0],'role':'irregular aggregate contact clusters within existing gravel footprint'},
}
BUDGET_TRIANGLES=15000
LAMP_CENTER=[4.62,-8.925,1.70]
EMITTER_SITE=[4.62,-8.974,1.70]
LIGHT_AXIS=[0,-1,0]
MIN_APERTURE_RADIUS=.061
# Native existing bed, plinth and ramp are the only placement authority.
GROUND_CONTRACT={'gravel_bounds_xy':[-10,10,-8,8],'gravel_top_z':-.08,'plinth_bounds_xy':[4.1,5.9,-9.25,-7.75],'plinth_top_z':0,'apron_x':[3.2,6.8],'apron_y':[-14,-9.25],'apron_z':[-.25,0]}
