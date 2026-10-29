"""FIX-07 Caliche substation field kit. Original geometry; not electrical installation guidance.

Blender site metres B (x,y,z), Z up, map to world Three (7890-x, -1.10+z, 536.20+y),
the same root/yaw 180 placement as Tools/substation and Tools/substationdetail.
"""
REVISION = 'R1-Caliche-substation-field-20261029'
SITE_WORLD_THREE = [7890, -1.10, 536.20]
WORLD_YAW_DEGREES = 180
PARTS = {
 'SM_SubstationField_Culvert': 'precast concrete outlet endwall, corrugated pipe, wings, apron and riprap at the existing T02 culvert proxy',
 'SM_SubstationField_Reference': 'T02 reference post with two painted sleeves, reference sign post and flush survey slab at the standing mark',
 'SM_SubstationField_TestFixture': 'olive enclosure for the existing T02 windows/handles, legs, piers, bracket and conduit',
 'SM_SubstationField_ApronGrade': 'measured gravel grade around ramp, pockets, bed front and culvert berm meeting native visible ground',
}
BUDGET_TRIANGLES = 20000
# Existing native placement authority (Tools/substation/design.py). Never moved.
RAMP = (3.2, 6.8, -14.0, -9.25)          # top z -0.25 at y-14 to 0 at y-9.25
PLINTH = (4.1, 5.9, -9.25, -7.75)        # top z 0
BED = (-10.0, 10.0, -8.0, 8.0)           # gravel top z -0.08
POCKETS = ((3.2, 4.1, -9.25, -8.0), (5.9, 6.8, -9.25, -8.0))
# T02 proxies (S47SubstationFollowup.cpp) converted to B. Feature points must stay where they are.
CULVERT_BOX = (7.15, 8.85, -8.8, -8.0)   # proxy footprint x7881.15..7882.85, z527.4..528.4
CULVERT_FEATURE = (8.0, -8.4, 0.45)      # world (7882,-.65,527.8)
PIPE_CENTER_Z, PIPE_RADIUS = 0.20, 0.30
BAND_POST = (9.65, -8.55)                # world (7880.35, 527.65)
BAND_CENTERS_Z = (1.01, 1.51)            # world y -.09 and .41, sleeves .20 tall
REFERENCE_MARKER = (11.0, -14.1)         # world (7879, 522.1)
FOOT_MARK = (11.0, -15.7)                # world (7879, 520.5)
FIXTURE_FACE_Y = -9.065                  # enclosure front, 5 mm into the existing indicator windows
INDICATORS_X = (3.6, 3.95, 4.3)
