// Unreal picture pass. The six modes share the existing fixed-exposure ACES scene.
// Display-referred texture/print treatment follows the curve; Slate remains clean.
// PictureMode selects: 0 Clean, 1 VHS, 2 Carpenter, 3 16 mm, 4 Super 8, 5 Bodycam 1986.
// D45-20261110 (D45-19): Bodycam 1986 is a chest-worn 1986 field camera taped to VHS. Signal order is
// optics (barrel lens), then the date generator, then the tape: sc() bends the scene and burns in the
// stamp, so every VHS tap, chroma tap and halo sees the bent, stamped picture. Lens/Cam/Stamp come
// from MPC_Body (FS47Bodycam); captures with Vhs=0 never reach the lens (the gate below).
struct W {
  float K, Dens, HazeK;
  float3 FogCol;
  bool Body;
  float Bend, Edge, Aspect, Osd;
  float2 Pan;
  float4 Stamp;
  float3 scene(float2 uv) {
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
  // D45-20261110: a 5x7 dot character generator, rows packed five bits each (rows 0-3 in x, 4-6 in y).
  uint2 glyph(uint c) {
    return c == 0u ? uint2(0xACE2Eu, 0x3A39u) /* 0 */ :
           c == 1u ? uint2(0x21184u, 0x3884u) /* 1 */ :
           c == 2u ? uint2(0x1062Eu, 0x7D04u) /* 2 */ :
           c == 3u ? uint2(0x1105Fu, 0x3A21u) /* 3 */ :
           c == 4u ? uint2(0x928C2u, 0x085Fu) /* 4 */ :
           c == 5u ? uint2(0x0FA1Fu, 0x3A21u) /* 5 */ :
           c == 6u ? uint2(0xF4106u, 0x3A31u) /* 6 */ :
           c == 7u ? uint2(0x2083Fu, 0x2108u) /* 7 */ :
           c == 8u ? uint2(0x7462Eu, 0x3A31u) /* 8 */ :
           c == 9u ? uint2(0x7C62Eu, 0x3041u) /* 9 */ :
           c == 10u ? uint2(0x20821u, 0x4208u) /* / */ :
           c == 11u ? uint2(0x03180u, 0x018Cu) /* : */ :
           c == 12u ? uint2(0xFC62Eu, 0x4631u) /* A */ :
           c == 13u ? uint2(0x8422Eu, 0x3A30u) /* C */ :
           c == 14u ? uint2(0x8C63Eu, 0x7A31u) /* D */ :
           c == 15u ? uint2(0xF421Fu, 0x7E10u) /* E */ :
           c == 16u ? uint2(0xF421Fu, 0x4210u) /* F */ :
           c == 17u ? uint2(0x2108Eu, 0x3884u) /* I */ :
           c == 18u ? uint2(0x84210u, 0x7E10u) /* L */ :
           c == 19u ? uint2(0xAD771u, 0x4631u) /* M */ :
           c == 20u ? uint2(0x8C62Eu, 0x3A31u) /* O */ :
           c == 21u ? uint2(0xF463Eu, 0x4210u) /* P */ :
           c == 22u ? uint2(0xF463Eu, 0x4654u) /* R */ :
           c == 23u ? uint2(0x7420Fu, 0x7821u) /* S */ : uint2(0u, 0u);
  }
  // "MM/DD/YY  HH:MM:SS PM" from Stamp: x month*100+day, y year (two digits), z hour(1-12)*10000+min*100+sec, w PM.
  uint dateChar(uint i) {
    uint md = uint(Stamp.x + 0.5), yy = uint(Stamp.y + 0.5) % 100u, t = uint(Stamp.z + 0.5);
    uint n = 0u, d = 0u;
    if (i < 2u) { n = md; d = i == 0u ? 1000u : 100u; }
    else if (i == 3u || i == 4u) { n = md; d = i == 3u ? 10u : 1u; }
    else if (i == 6u || i == 7u) { n = yy; d = i == 6u ? 10u : 1u; }
    else if (i >= 10u && i <= 17u && i != 12u && i != 15u) {
      uint k = i - 10u - (i > 12u ? 1u : 0u) - (i > 15u ? 1u : 0u);
      d = k == 0u ? 100000u : k == 1u ? 10000u : k == 2u ? 1000u : k == 3u ? 100u : k == 4u ? 10u : 1u;
      n = t;
    }
    if (d > 0u) return (n / d) % 10u;
    return i == 2u || i == 5u ? 10u : i == 12u || i == 15u ? 11u : i == 19u ? (Stamp.w > 0.5 ? 21u : 12u) : i == 20u ? 19u : 99u;
  }
  // "SARO FIELD CAM 2"
  uint unitChar(uint i) {
    return i == 0u ? 23u : i == 1u ? 12u : i == 2u ? 22u : i == 3u ? 20u : i == 5u ? 16u : i == 6u ? 17u : i == 7u ? 15u :
           i == 8u ? 18u : i == 9u ? 14u : i == 11u ? 13u : i == 12u ? 12u : i == 13u ? 19u : i == 15u ? 2u : 99u;
  }
  // q in dots (240 down the picture), right/top: the stamp's right edge and first row
  float ink(float2 q, float right, float top) {
    uint row = q.y >= top + 10.0 ? 1u : 0u;
    uint count = row == 0u ? 21u : 16u;
    float2 g = q - float2(right - float(count) * 6.0, top + float(row) * 10.0);
    if (g.x < 0.0 || g.y < 0.0 || g.y >= 7.0) return 0.0;
    uint cell = uint(floor(g.x / 6.0));
    if (cell >= count) return 0.0;
    uint gx = uint(floor(g.x - float(cell) * 6.0)), gy = uint(floor(g.y));
    if (gx > 4u) return 0.0;
    uint2 b = glyph(row == 0u ? dateChar(cell) : unitChar(cell));
    uint bits = gy < 4u ? (b.x >> (gy * 5u)) : (b.y >> ((gy - 4u) * 5u));
    return float((bits >> (4u - gx)) & 1u);
  }
  // x: the white characters, y: their dark edge (one dot down and right), top right of the frame
  float2 osd(float2 uv) {
    float2 q = uv * float2(Aspect, 1.0) * 240.0;
    float right = Aspect * 240.0 - 14.0, top = 11.0;
    if (q.y < top - 1.0 || q.y > top + 19.0 || q.x < right - 128.0 || q.x > right + 2.0) return 0;
    return float2(ink(q, right, top), ink(q - 1.0, right, top));
  }
  float3 sc(float2 uv) {
    if (!Body) return scene(uv);
    // Barrel lens: the corners stay the corners (no black edge), the centre is magnified and
    // straight lines bow. Bend is already scaled by strength and the lens gate.
    float2 p = (uv - 0.5) * float2(Aspect, 1.0);
    float2 at = 0.5 + (uv - 0.5) * (1.0 + Bend * dot(p, p)) / (1.0 + Bend * Edge);
    // Turning only: the picture leans with the pan and squeezes with the tilt (zero when still or calm).
    at += float2(Pan.x, Pan.y) * (uv.y - 0.5);
    float3 c = scene(at);
    [branch] if (Osd > 0.0001) {
      float2 o = osd(uv);
      c = lerp(c, c * 0.2, o.y * Osd * 0.85);
      c = lerp(c, float3(0.93, 0.95, 0.90), o.x * Osd);
    }
    return c;
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
float strength = saturate(Strength);
// Field-camera/TV captures temporarily lower the legacy Vhs gate to keep evidence clean.
bool lookOn = Mode >= 0.5 && Vhs > 0.00001 && strength > 0.00001;
bool body = lookOn && Mode > 4.5;
float calm = saturate(Calm);
float moving = 1.0 - calm;
float frame = floor(Time * 24.0) * moving;
float2 size = max(View.ViewSizeAndInvSize.xy, 1.0);
float2 pixel = 1.0 / size;
float aspect = size.x / size.y;
// D45-20261110: the lens goes into clean too and is scaled by strength, so a partial strength
// fades the geometry instead of cross-fading a straight and a bent picture (a double image).
float lensGate = body ? saturate(Lens.z) * strength : 0.0;
w.Body = body; w.Aspect = aspect; w.Edge = 0.25 * (aspect * aspect + 1.0);
w.Bend = max(Lens.x, 0.0) * lensGate;
w.Pan = body ? clamp(Cam.yz, -6.0, 6.0) * float2(0.004, 0.003) * moving * lensGate : 0;
w.Osd = body ? saturate(Cam.w) * strength : 0.0;
w.Stamp = Stamp;
float3 clean = w.sc(UV);
[branch] if (!lookOn) return clean;
float2 r = UV - 0.5;
float vignette = smoothstep(0.10, 0.5, dot(r, r));
float3 col;

// Bodycam 1986 is recorded on the same tape.
[branch] if (Mode < 1.5 || Mode > 4.5) {
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
// D45-20261110: the 1986 tube camera on top of the tape (Bodycam only).
[branch] if (body) {
  float2 pc = r * float2(aspect, 1.0);
  float edge = saturate(dot(pc, pc) / w.Edge); // 0 centre .. 1 corner
  // Colour fringing: red pushed out and blue pulled in, growing to the corners.
  float2 off = r * 2.0 * edge * max(Lens.y, 0.0) * lensGate * pixel;
  col.r += w.sc(UV + off).r - clean.r;
  col.b += w.sc(UV - off).b - clean.b;
  // Gain control: five fixed metering taps (the same for every pixel). The gain is capped at
  // 1.6 so the night stays dark: it brings noise, not daylight. Cam.x pumps it on a lamp's toggle.
  float meter = w.lum(w.scene(float2(0.5, 0.5))) + w.lum(w.scene(float2(0.3, 0.36))) + w.lum(w.scene(float2(0.7, 0.36)))
              + w.lum(w.scene(float2(0.3, 0.66))) + w.lum(w.scene(float2(0.7, 0.66)));
  float gain = clamp(0.20 / max(meter * 0.2, 0.02), 1.0, 1.6);
  gain = clamp(gain + clamp(Cam.x, -1.0, 1.0) * 0.35 * moving, 0.85, 1.6);
  col = max(col, 0.0);
  col = col * gain / (1.0 + (gain - 1.0) * col);
  float bl = w.lum(col);
  float dark = 1.0 - smoothstep(0.05, 0.55, bl);
  // The tube loses colour in the shadows.
  col = lerp(bl.xxx, col, lerp(0.65, 1.0, smoothstep(0.06, 0.40, bl)));
  // Coarse gain noise and colour blotches, stronger in the dark and with more gain.
  float2 coarse = float2(240.0 * aspect, 240.0);
  col += w.grain(UV, coarse, frame) * (0.010 + 0.030 * (gain - 1.0)) * (0.35 + 0.65 * dark);
  col += float3(w.grain(UV, coarse * 0.5, frame + 5.0), 0.0, w.grain(UV, coarse * 0.5, frame + 9.0)) * 0.010 * dark * (gain - 0.8);
  // Light trails and smear only while turning fast (Cam.y: the turn rate, rad/s).
  float pan = abs(Cam.y) * moving * lensGate;
  [branch] if (pan > 0.6) {
    float k = saturate((pan - 0.6) / 2.4);
    float2 back = float2(Cam.y > 0.0 ? -1.0 : 1.0, 0.0) * pixel * (size.y / 480.0);
    float3 trail = 0;
    [unroll] for (int s2 = 1; s2 <= 3; ++s2) trail += w.bright(UV + back * (5.0 * k * s2)) * (1.0 - 0.25 * s2);
    col += trail * 0.35 * k;
    col = lerp(col, w.sc(UV + back * (4.0 * k)), 0.35 * k);
  }
  // Wide-lens vignette, much stronger than the tape's own.
  col *= 1.0 - saturate(Lens.w) * lensGate * smoothstep(0.35, 1.0, edge);
}
return saturate(lerp(clean, col, strength));
