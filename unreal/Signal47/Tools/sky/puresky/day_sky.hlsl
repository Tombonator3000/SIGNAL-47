// Day-only photograph. The original night material is a separate runtime asset.
float pureW = saturate(PureSkyWeight);
float3 pureRadiance = float3(0, 0, 0);
[branch] if (pureW > 0.0) {
    float3 pu = -s47finite.unit3(V);
    float3 pt = float3(pu.x, pu.z, pu.y);
    float pyc = cos(Yaw), pys = sin(Yaw);
    float3 pd = float3(pt.x*pyc-pt.z*pys, pt.y, pt.x*pys+pt.z*pyc);
    // Match the photographed warm horizon to the current solar azimuth without
    // tilting the horizon or changing the original story/world coordinates.
    float pa = s47finite.azimuth(pd.z, pd.x);
    float psa = s47finite.azimuth(SunDir.z, SunDir.x);
    float2 puv = float2(frac(0.5 + (pa-psa+PureSkyYaw)/6.28318530718),
                        acos(clamp(pd.y, -1.0, 1.0))/3.14159265359);
    // Explicit LOD keeps the reflection-capture path finite and low frequency.
    float mip = IsCapture > 0.5 ? 6.0 : 0.0;
    pureRadiance = max(Texture2DSampleLevel(PureSkyTexture, PureSkyTextureSampler, puv, mip).rgb, 0.0);
    // Remove the compact photographed sun locally. The surrounding photographic
    // clouds/glow remain, while a single clock-driven sun can actually set.
    float2 sourceDelta = float2(frac(puv.x-PureSkySunUV.x+0.5)-0.5, puv.y-PureSkySunUV.y);
    sourceDelta *= float2(6.28318530718*0.99673, 3.14159265359);
    float diskRemoval = 1.0-smoothstep(0.013, 0.035, length(sourceDelta));
    [branch] if (diskRemoval > 0.0) {
        float3 leftSky = Texture2DSampleLevel(PureSkyTexture, PureSkyTextureSampler,
                                              float2(frac(puv.x-0.01),puv.y),mip).rgb;
        float3 rightSky = Texture2DSampleLevel(PureSkyTexture, PureSkyTextureSampler,
                                               float2(frac(puv.x+0.01),puv.y),mip).rgb;
        pureRadiance = lerp(pureRadiance, max((leftSky+rightSky)*0.5,0.0), diskRemoval);
    }
    pureRadiance = min(pureRadiance * PureSkyGain, float3(16, 16, 16));
    // Exposure is calibrated to the existing game. Only the day photograph dims;
    // the original Dawn/Evening values and night recipe are never modified.
    pureRadiance *= saturate(Dawn);
    float pureSd = saturate(dot(pd, s47finite.unit3(SunDir)));
    float3 sunMain = float3(1.0,0.62,0.30)*max(Sun,0.0)*pow(pureSd,60.0)*0.9;
    float3 sunCaliche = float3(1.0,0.72,0.43)*max(Sun,0.0)*pow(pureSd,130.0)*0.20;
    if (IsCapture < 0.5) {
        sunMain += float3(1.0,0.62,0.30)*max(Sun,0.0)*pow(pureSd,900.0)*3.0;
        sunMain += float3(4.0,3.2,2.2)*max(Sun,0.0)*smoothstep(0.99996,0.99999,pureSd);
        sunCaliche += float3(1.0,0.72,0.43)*max(Sun,0.0)*smoothstep(0.99996,0.99999,pureSd)*3.0;
    }
    pureRadiance += lerp(sunMain,sunCaliche,saturate(ConceptBlend));
    [branch] if (pureW >= 1.0) return pureRadiance;
}
