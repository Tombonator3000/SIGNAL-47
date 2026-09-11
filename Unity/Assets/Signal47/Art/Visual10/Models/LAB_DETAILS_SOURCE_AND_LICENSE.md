# Visual 10 laboratory detail meshes

Original geometry authored for SIGNAL / 47. No external assets or paid services.
Rebuild with Unity/Blender/Source/visual10_lab_details.py. Editable source:
Unity/Blender/SIGNAL47_visual10_lab_details.blend. Project ownership applies.

V10_TwinFluorescent has two individual glass tubes, separate sockets, enamel
reflectors and metal frame. Map CH09_WarmDiffuser only to low warm emission;
do not make housing glow. Fixture nominal bounds .25 x .95 x .10 metres in
Blender XYZ; long Y becomes Unity Z. Tube underside origin is at Blender Z=0.
Use a separate Unity light source; the FBX itself contains no realtime light.
Other objects have worktop origin, Blender Z-up, -Z forward/Y up FBX export.
Material slots use CH09_Cream, CH09_Steel, CH09_Rubber, CH09_WarmDiffuser.
Measuring jug/funnel contain actual open interiors; photo tongs have two
spring arms and separate gripping pads. See lab-details-verification.json
for measured exported dimensions, triangles, UV and reimport checks.
Unity integration, final materials and runtime appearance remain to verify.
