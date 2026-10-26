"""Original STATION 01 instrument set. Coordinates preserve Chapter 3's evidence."""
REVISION = 'R1-original-STATION01-field-instruments-1986'
SITE_WORLD_THREE = [0, 0, 8000]
# Meshes export around their own zero pivot. Offsets below assemble the Blender source.
# B(x,y,z) maps to site-local Three(x,z,-y), hence Unreal centimetres (100x,-100y,100z).
PARTS = {
 'SM_FieldKit_Transit': {'offset':[3,4,0], 'role':'tapered survey pier, levelling screws, engraved horizontal circle, telescope and optical lenses', 'proxy':'world.site.proxies.transit'},
 'SM_FieldKit_LampStand': {'offset':[1.6,2.6,0], 'role':'three-foot stand, telescopic mast, battery and wiring; no hood or bulb', 'proxy':'world.site.proxies.lamp'},
 'SM_FieldKit_LampHead': {'offset':[1.6,2.6,1.86], 'role':'reflector shell, cooling fins, yoke and hinge; parent local coordinates', 'parent':'world.site.objs.lampHead', 'display_rotation':[0,0,0]},
 'SM_FieldKit_LampBulb': {'offset':[1.6,2.6,1.86], 'role':'separate field lens driven by the existing fieldBulb state', 'parent':'world.site.objs.lampHead', 'state_material':'world.site.m.fieldBulb'},
 'SM_FieldKit_LampHood': {'offset':[1.6,2.495,1.995], 'role':'opaque tin blackout lid, local zero hinge; covered rotation X=0, uncovered Three X=2.1', 'parent':'world.site.objs.lampHood', 'native_local_three':[0,0,0], 'pivot':'existing hinge; no translation or animation baked'},
 'SM_FieldKit_Receiver': {'offset':[-11.85,-1.8,.8], 'role':'valve field receiver, open dial aperture, separate speaker, headphones, ventilated metal case, controls', 'proxy':'world.site.proxies.receiver'},
 'SM_FieldKit_ReceiverDial': {'offset':[-11.85,-1.8,.8], 'role':'separate printed slide-rule dial and S meter, driven together by existing dial state', 'state_material':'world.site.m.dial'},
 'SM_FieldKit_FieldPhone': {'offset':[-12.92,-2.55,1.45], 'role':'wall mounted oak magneto telephone, paired bells, speaking horn, crank and cord; no hanging receiver', 'proxy':'world.site.proxies.fieldPhone'},
 'SM_FieldKit_Handset': {'offset':[-12.82,-2.7,1.55], 'role':'hanging Bakelite earpiece and hook, retained independent for ringing and pickup', 'parent':'world.site.objs.fieldPhoneHandset', 'native_local_three':[0,0,0]},
 'SM_FieldKit_TimingBench': {'offset':[-11.45,-1.86,0], 'role':'three metre worn oak field bench, shelf, levelling feet and open drawer clearance; no static drawer front'},
 'SM_FieldKit_TimingLog': {'offset':[-11.05,-1.96,.804], 'role':'open 1947 timing ledger, readable printed chronology and pencil', 'proxy':'world.site.proxies.timingLog'},
 'SM_FieldKit_Diagram': {'offset':[-11.15,-1.537,1.78], 'role':'framed A/B/C arrangement in the existing clue location; no text over existing projection', 'proxy':'world.site.proxies.diagram'},
 'SM_FieldKit_EvidenceProps': {'offset':[-10.13,-1.67,.8], 'role':'original film can, unopened evidence sleeve and recorded compact cassette, cosmetic set dressing'},
}
ATLAS = {'receiver_dial':0, 'receiver_meter':1, 'receiver_plate':2, 'timing_log':3,
 'diagram':4, 'transit_card':5, 'telephone':6, 'evidence':7, 'film':8, 'cassette':9,
 'serial':10, 'safety':11, 'spares':12, 'station':13, 'inspection':14, 'blank':15}
BUDGET_TRIANGLES = 100000
