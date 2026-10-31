// Inputs: original VertexNormalWS MacroN, original final normal transformed to
// world space as NormalWS, and FinalePlanarNormal. No PixelNormalWS feedback.
float3 n = normalize(MacroN);
// cos(4deg)..cos(2deg): full correction through 2deg, none from 4deg.
float gate = smoothstep(0.9975640503, 0.9993908270, n.z);
float blend = saturate(FinalePlanarNormal) * gate;
if (blend <= 0.0)
    return NormalWS;

float3 target = normalize(lerp(n, float3(0.0, 0.0, 1.0), blend));
float3 v = cross(n, target);
float c = clamp(dot(n, target), -1.0, 1.0);
// Minimal Rodrigues rotation of the WHOLE original normal: its micro angle to
// the macro normal survives. The gate limits rotation to <4deg, so 1+c >1.997.
return NormalWS + cross(v, NormalWS) + cross(v, cross(v, NormalWS)) / (1.0 + c);
