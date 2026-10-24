// SIGNAL / 47 volumetric clouds (SKY.md). Procedural, no textures: one thin layer (about 2.9 to
// 4.5 km) of broken alto/stratocumulus; the cirrus above it is painted on the dome (atmosphere.hlsl).
// Inputs: WP world position (cm), Alt normalised altitude in layer, Cam camera position (cm),
// Cover/Cirrus 0..1 from the story clock, Wind (xy km drift, z evolution), Clear0/Clear1 sky
// directions kept open for story beats (xyz direction, w strength), Ext extinction per metre,
// Mode 0 = full density sample, 1 = conservative density (cheap weather only).
struct S47C {
 float h(float3 p) { p = frac(p * float3(.1031, .1030, .0973)); p += dot(p, p.yxz + 33.33); return frac((p.x + p.y) * p.z); }
 float n(float3 x) {
  float3 i = floor(x), f = frac(x); f = f * f * (3. - 2. * f);
  return lerp(lerp(lerp(h(i), h(i + float3(1, 0, 0)), f.x), lerp(h(i + float3(0, 1, 0)), h(i + float3(1, 1, 0)), f.x), f.y),
              lerp(lerp(h(i + float3(0, 0, 1)), h(i + float3(1, 0, 1)), f.x), lerp(h(i + float3(0, 1, 1)), h(i + float3(1, 1, 1)), f.x), f.y), f.z);
 }
 float n2(float2 x) {
  float2 i = floor(x), f = frac(x); f = f * f * (3. - 2. * f);
  return lerp(lerp(h(float3(i, 7.)), h(float3(i + float2(1, 0), 7.)), f.x), lerp(h(float3(i + float2(0, 1), 7.)), h(float3(i + float2(1, 1), 7.)), f.x), f.y);
 }
 float f2(float2 x) { float a = 0., w = .5; for (int k = 0; k < 4; k++) { a += w * n2(x); x = x * 2.03 + float2(17.1, 9.7); w *= .5; } return a / .9375; }
 float f3(float3 x) { float a = 0., w = .5; for (int k = 0; k < 3; k++) { a += w * n(x); x = x * 2.11 + float3(5.3, 1.7, 9.2); w *= .5; } return a / .875; }
};
S47C c;
float2 P = WP.xy * 1e-5 + Wind.xy;
float3 dir = normalize(WP - Cam);
// Ragged, soft edges so a kept-open patch reads as a natural gap, not a cut circle.
float rag = (c.n2(P * 1.7 + 11.3) - .5) * .035;
float open = 1. - Clear0.w * smoothstep(.950, .990, dot(dir, Clear0.xyz) + rag);
open *= 1. - Clear1.w * smoothstep(.955, .993, dot(dir, Clear1.xyz) + rag);
// The layer thins toward the horizon (concept): far clouds break up so the twilight band shows.
open *= 1. - .8 * smoothstep(10e5, 26e5, length(WP.xy - Cam.xy));
float cover = saturate(Cover);
// Broad banks (about 10 km) carry broken cells (about 1.6 km); the threshold sets the sky fraction.
float bank = c.f2(P * .10 + float2(3.7, 1.9));
float cells = c.f2(P * .62 + float2(.3, 8.1));
float w = bank * .55 + cells * .45;
float thr = .55 + .30 * (.5 - cover);
float weather = smoothstep(thr - .09, thr + .13, w) * open;
// Flat dark bases, low rounded tops: alto/stratocumulus, a few hundred metres thick, thicker where dense.
float top = .22 + .58 * weather;
if (Mode > .5) return float3(weather * step(Alt, top), 0, 0);
float d = 0.;
if (weather > 0. && Alt < top) {
 float prof = smoothstep(0., .06, Alt) * (1. - smoothstep(top * .35, top, Alt));
 d = weather * prof;
 float3 Q = float3(WP.xy * 1e-5 + Wind.xy * 1.15, WP.z * 1e-5 * 1.6 + Wind.z) * 1.8;
 float det = c.n(Q) * .65 + c.n(Q * 2.3 + 4.1) * .35;
 d = saturate((d - det * .45) / max(.1, 1. - det * .45));
}
return float3(d * Ext, 0, d);
