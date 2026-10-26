// Unreal picture pass. The five modes share the existing fixed-exposure ACES scene.
// Display-referred texture/print treatment follows the curve; Slate remains clean.
// PictureMode selects: 0 Clean, 1 VHS, 2 Carpenter, 3 16 mm, 4 Super 8.
struct W {
  float K, Dens, HazeK;
  float3 FogCol;
  float3 sc(float2 uv) {
    // Clamp to pixel centres so off-screen bloom taps never sample a neighbouring view.
    float2 edge = View.ViewSizeAndInvSize.zw * 0.5;
    float2 b = ViewportUVToBufferUV(clamp(uv, edge, 1.0 - edge));
    float3 c = max(SceneTextureLookup(b, 14, true).rgb, 0);
    c = any(isnan(c)) ? 0 : min(c, 10.0);
    [branch] if (Dens > 0) {
      float d = SceneTextureLookup(b, 1, false).r * 0.01;
      float f = d > 25000.0 ? 0.0 : 1.0 - exp(-(Dens * d) * (Dens * d));
      float s = SceneTextureLookup(b, 25, false).r;
      if (s > 0.5) {
        float cd = SceneTextureLookup(b, 13, false).r * 0.01;
        if (abs(cd - d) <= max(0.5, d * 0.002)) f *= s < 1.5 ? HazeK : 0.0;
      }
      c = lerp(c, FogCol, f);
    }
    c *= K;
    float3 v = float3(dot(float3(0.59719, 0.35458, 0.04823), c), dot(float3(0.07600, 0.90834, 0.01566), c), dot(float3(0.02840, 0.13383, 0.83777), c));
    v = (v * (v + 0.0245786) - 0.000090537) / (v * (0.983729 * v + 0.4329510) + 0.238081);
    float3 o = saturate(float3(dot(float3(1.60475, -0.53108, -0.07367), v), dot(float3(-0.10208, 1.10813, -0.00605), v), dot(float3(-0.00327, -0.07276, 1.07602), v)));
    float3 lo = o * 12.92, hi = 1.055 * pow(max(o, 1e-6), 1.0 / 2.4) - 0.055;
    return lerp(hi, lo, step(o, 0.0031308));
  }
  float hash(float2 p) {
    float3 q = frac(float3(p.xyx) * 0.1031);
    q += dot(q, q.yzx + 33.33);
    return frac((q.x + q.y) * q.z);
  }
  float lum(float3 c) { return dot(c, float3(0.299, 0.587, 0.114)); }
  float3 yiq(float3 c) { return float3(lum(c), dot(float3(0.596, -0.274, -0.322), c), dot(float3(0.211, -0.523, 0.312), c)); }
  float3 rgb(float3 c) { return float3(c.x + 0.956 * c.y + 0.621 * c.z, c.x - 0.272 * c.y - 0.647 * c.z, c.x - 1.106 * c.y + 1.703 * c.z); }
  float grain(float2 uv, float2 grid, float frame) {
    float2 p = floor(uv * grid);
    // Two centred distributions reduce the binary/salt-and-pepper look of uniform noise.
    return hash(p + frame * float2(17, 31)) + hash(p.yx + frame * float2(47, 13) + 59.7) - 1;
  }
  float3 bright(float2 uv) {
    float3 c = sc(uv);
    // Only strong sources scatter: no grey veil over the unlit parts of the scene.
    return max(c - 0.62, 0.0) * smoothstep(0.55, 0.92, lum(c));
  }
};
W w;
w.K = K; w.Dens = Dens * (1.0 - saturate(Native)); w.HazeK = HazeK; w.FogCol = FogCol;
float3 clean = w.sc(UV);
float strength = saturate(Strength);
// Field-camera/TV captures temporarily lower the legacy Vhs gate to keep evidence clean.
[branch] if (Mode < 0.5 || Vhs <= 0.00001 || strength <= 0.00001) return clean;
float calm = saturate(Calm);
float moving = 1.0 - calm;
float frame = floor(Time * 24.0) * moving;
float2 size = max(View.ViewSizeAndInvSize.xy, 1.0);
float2 pixel = 1.0 / size;
float aspect = size.x / size.y;
float2 r = UV - 0.5;
float vignette = smoothstep(0.10, 0.5, dot(r, r));
float3 col;

[branch] if (Mode < 1.5) {
  // NTSC/VHS is a band-limited luma/chroma signal, not radial lens aberration.
  float2 tapeSize = float2(480.0 * aspect, 480.0);
  float2 tp = 1.0 / tapeSize;
  float tapeLine = floor(UV.y * tapeSize.y);
  float field = floor(Time * 59.94) * moving;
  float jitter = sin(UV.y * 180.0 + Time * 2.1) * 0.13;
  jitter += (w.hash(float2(floor(tapeLine * 0.25), field)) - 0.5) * 0.28;
  // Head switching stays at the bottom edge and is suppressed by calm-picture mode.
  float head = smoothstep(0.974, 0.998, UV.y) * moving;
  float2 at = UV + float2((jitter * moving + head * sin(Time * 13 + tapeLine) * 1.5) * tp.x, 0);
  float3 q = w.yiq(w.sc(at));
  q.x = q.x * 0.64 + 0.18 * (w.lum(w.sc(at - float2(tp.x * 0.65, 0))) + w.lum(w.sc(at + float2(tp.x * 0.65, 0))));
  float2 chroma = 0;
  float weight = 0;
  [unroll] for (int i = -2; i <= 3; ++i) {
    float k = i == 0 || i == 1 ? 2.0 : 1.0;
    chroma += w.yiq(w.sc(at + float2((i * 1.45 + 0.9) * tp.x, 0))).yz * k;
    weight += k;
  }
  q.yz = chroma / weight * 0.88;
  col = w.rgb(q);
  float3 halo = 0;
  [unroll] for (int j = 0; j < 4; ++j) {
    float a = j * 1.5707963;
    halo += w.bright(at + float2(cos(a), sin(a)) * tp * 2.2);
  }
  col += halo * 0.075;
  float l = w.lum(col);
  col *= lerp(float3(0.94, 1.015, 1.035), float3(1.035, 1.005, 0.955), smoothstep(0.12, 0.7, l));
  col = col * 0.975 + float3(0.006, 0.007, 0.009);
  float noise = w.grain(UV, tapeSize, frame);
  col += noise * (0.013 + 0.006 * (1.0 - l));
  col *= 1.0 - 0.009 * (0.5 + 0.5 * cos(UV.y * tapeSize.y * 3.14159265));
  col += (w.hash(float2(tapeLine, field + 11)) - 0.5) * head * 0.025;
  col *= 1.0 - 0.045 * vignette;
} else {
  bool carpenter = Mode < 2.5;
  bool super8 = Mode >= 3.5;
  float2 at = UV;
  // Sub-pixel registration is a property of the small-film presets. The primary
  // Carpenter image stays stable and all presets retain the renderer's frame rate.
  if (!carpenter) {
    float weave = super8 ? 0.60 : 0.13;
    at += float2(w.hash(float2(frame, 7.1)), w.hash(float2(frame, 91.4))) * 2.0 * weave * pixel * moving - weave * pixel * moving;
  }
  col = w.sc(at);
  float3 nearGlow = 0, farGlow = 0;
  float radius = carpenter ? 2.4 : (super8 ? 3.6 : 2.8);
  // Two radii form a soft optical core and a broader red/orange film-base halo.
  // They are brightness-gated and share the fixed exposure of the source image.
  [unroll] for (int j = 0; j < 6; ++j) {
    float a = j * 1.04719755;
    float2 d = float2(cos(a), sin(a)) * pixel * radius;
    nearGlow += w.bright(at + d);
    farGlow += w.bright(at + d * 3.4);
  }
  nearGlow /= 6.0; farGlow /= 6.0;
  col += nearGlow * (carpenter ? 0.16 : 0.19);
  col += farGlow * float3(1.0, 0.23, 0.055) * (super8 ? 0.14 : 0.075);

  float l = w.lum(col);
  if (carpenter) {
    // Cold blue moonlight against warm practical lamps. Chroma-aware shadow masks
    // protect tungsten, skin and saturated indicator colours from a global teal wash.
    float chroma = max(col.r, max(col.g, col.b)) - min(col.r, min(col.g, col.b));
    float shadow = (1.0 - smoothstep(0.10, 0.48, l)) * smoothstep(0.008, 0.07, l);
    float neutral = 1.0 - smoothstep(0.08, 0.32, chroma);
    float warm = smoothstep(0.015, 0.22, col.r - col.b) * smoothstep(0.10, 0.55, l);
    col *= lerp(float3(1, 1, 1), float3(0.88, 0.98, 1.17), shadow * neutral * 0.85);
    col += float3(0.0, 0.0008, 0.0035) * shadow * neutral;
    col *= lerp(float3(1, 1, 1), float3(1.055, 1.005, 0.94), warm);
    col *= lerp(float3(1, 1, 1), float3(1.018, 1.004, 0.98), smoothstep(0.48, 0.95, l));
    // A restrained print contrast keeps detail above black; no broad crushed toe.
    col = max(col * 1.018 - 0.003, 0.0);
    col *= 1.0 - 0.085 * vignette;
  } else if (!super8) {
    // Dusty 16 mm field documentary: subdued dye, olive shadows, warm paper/highlights.
    col = lerp(l.xxx, col, 0.87);
    float shadow = 1.0 - smoothstep(0.08, 0.5, l);
    col *= lerp(float3(1.045, 1.005, 0.94), float3(0.975, 1.018, 0.985), shadow);
    col = col * 0.975 + float3(0.006, 0.007, 0.004);
    col *= 1.0 - 0.105 * vignette;
  } else {
    // Saturated reversal/home-movie palette and gentle exposure breathing.
    col = lerp(l.xxx, col, 1.08);
    col *= float3(1.055, 1.005, 0.915);
    col = max(col * 1.035 - 0.006, 0.0);
    col *= 1.0 + (w.hash(float2(frame, 37.6)) - 0.5) * 0.012 * moving;
    col *= 1.0 - 0.18 * vignette;
  }
  float grainHeight = carpenter ? 1080.0 : (super8 ? 540.0 : 810.0);
  float2 grid = float2(grainHeight * aspect, grainHeight);
  float grain = w.grain(UV, grid, frame) * 0.78 + w.grain(UV, grid * 0.63, frame + 13) * 0.22;
  // Light-dependent silver/dye texture. Black is not filled with white static.
  float envelope = smoothstep(0.005, 0.075, l) * (0.65 + 0.35 * (1.0 - smoothstep(0.4, 0.98, l)));
  col += grain * (carpenter ? 0.018 : (super8 ? 0.052 : 0.032)) * envelope;
}
return saturate(lerp(clean, col, strength));
