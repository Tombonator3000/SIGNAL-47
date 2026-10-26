"""Pure, original sky finite-math and capture-only material source.
Main-view recipe remains exact; captured ambient omits panorama/micro-noise.
This source alone does not prove a GPU fix or visual acceptance.
"""
import hashlib
import re

SOURCE_SHA256 = 'a60f875ea01b5295ac42970d1f2b59f87465ae214d3d889c58f445b93972295b'
FINITE_SHA256 = 'a87cc75ca55f45edd3b36466dd4a02820514db4282e4a21715d5b204d51eddbd'
CAPTURE_SHA256 = '9f046f77d38ba26541d0c1fc3431cead5dfcbf8258fa8515aaff33bfe26e3c39'


def sha256(code):
    return hashlib.sha256(code.encode('utf-8')).hexdigest()


HELPER = '''// S47 finite sky math: bounded normalization and a defined polar azimuth only.
struct S47SkyFinite {
 float3 unit3(float3 v) { return v * rsqrt(max(dot(v, v), 1e-12)); }
 float2 unit2(float2 v) { return v * rsqrt(max(dot(v, v), 1e-12)); }
 float azimuth(float z, float x) { return z*z + x*x > 1e-12 ? atan2(z, x) : 0.0; }
};
S47SkyFinite s47finite;
'''


def finite_code(old):
    replacements = []
    cursor, pieces = 0, []
    # Parse balanced arguments rather than using a regex on nested float constructors.
    for match in re.finditer(r'\bnormalize\(', old):
        start, end, depth = match.end(), match.end(), 1
        while depth:
            assert end < len(old), 'Unbalanced normalize expression'
            if old[end] == '(':
                depth += 1
            elif old[end] == ')':
                depth -= 1
            end += 1
        expression = old[start:end - 1]
        assert 'normalize(' not in expression, 'Nested normalization needs explicit review'
        dimension = 2 if expression.strip().startswith(('d.xz', 'SunDir.xz')) else 3
        new = f's47finite.unit{dimension}({expression})'
        replacements.append({'old': old[match.start():end], 'new': new})
        pieces.extend((old[cursor:match.start()], new))
        cursor = end
    pieces.append(old[cursor:])
    new = ''.join(pieces)
    guards = [
        (' / G[i].w;', ' / max(abs(G[i].w), 1e-6);'),
        ('float east = 0.5 + 0.5 * sa;', 'float east = saturate(0.5 + 0.5 * sa);'),
        ('atan2(d.z, d.x)', 's47finite.azimuth(d.z, d.x)'),
    ]
    for before, after in guards:
        assert new.count(before) == 1, f'Expected exactly one {before!r}'
        new = new.replace(before, after)
        replacements.append({'old': before, 'new': after})
    assert not re.search(r'\bnormalize\(', new), 'Unexpected unguarded normalize left'
    assert 'acos(clamp(' in new and 'asin(clamp(' in new
    return HELPER + new, replacements


BRANCH = '''// Capture-only low-frequency radiance; full finite main body follows unchanged.
[branch] if (IsCapture > 0.5) {
 float3 cu = -s47finite.unit3(V);
 float3 ct = float3(cu.x, cu.z, cu.y);
 float cc = cos(Yaw), cs = sin(Yaw);
 float3 cd = float3(ct.x*cc-ct.z*cs, ct.y, ct.x*cs+ct.z*cc);
 float ch = cd.y, lightC = saturate(Dawn), midC = saturate(Midday);
 float2 hdC = s47finite.unit2(cd.xz + float2(.0001,0));
 float2 sunC = s47finite.unit2(SunDir.xz + float2(.0001,0));
 float eastC = saturate(.5 + .5*dot(hdC,sunC));
 float sdC = saturate(dot(cd,s47finite.unit3(SunDir)));
 float horizonC = exp(-max(ch,0.0)*7.0);
 float3 baseC = lerp(float3(.030,.034,.060),float3(.006,.009,.024),smoothstep(-.02,.55,ch));
 baseC = lerp(baseC,HoleSky*2.0,.72*smoothstep(-.03,.14,ch));
 float glowC = .15 + .65*(max(Glow0.z,0.0)+max(Glow1.z,0.0)+max(Glow2.z,0.0));
 baseC += float3(.30,.15,.06)*exp(-max(ch,0.0)*22.0)*.30*glowC;
 float3 dawnC = lerp(lerp(float3(.20,.23,.30),float3(.62,.42,.26),pow(eastC,3.0)),
                     float3(.055,.085,.17),smoothstep(-.02,.45,ch));
 dawnC += float3(.55,.30,.12)*pow(eastC,8.0)*exp(-max(ch,0.0)*9.0)*.8;
 baseC = lerp(baseC*(1.0-.85*smoothstep(.1,.7,lightC)),dawnC,smoothstep(0.0,1.0,lightC)*.92);
 baseC += float3(1.0,.62,.30)*max(Sun,0.0)*pow(sdC,60.0)*.9;
 if(ch<0.0)baseC*=lerp(.5,1.0-.5*smoothstep(-.015,-.12,ch),smoothstep(0.0,1.0,lightC));
 float westC = pow(eastC,6.0);
 float3 nightC = lerp(float3(.004,.009,.022),float3(.018,.030,.055),horizonC);
 float3 blueC = lerp(float3(.034,.075,.165),float3(.21,.235,.29),horizonC);
 blueC += float3(.20,.065,.018)*westC*horizonC*(1.0-midC);
 float3 dayC = lerp(float3(.055,.19,.42),float3(.38,.50,.59),horizonC);
 float duskK = (1.0-midC)*smoothstep(.02,.22,lightC);
 float hb = exp(-max(ch,0.0)*8.0), hm = exp(-max(ch,0.0)*3.0);
 float3 duskC = lerp(float3(.050,.060,.125),float3(.16,.12,.16),hm);
 duskC = lerp(duskC,lerp(float3(.70,.34,.17),float3(.92,.38,.10),eastC*eastC),hb*lerp(.85,1.0,eastC));
 duskC *= .45+.85*lightC;
 blueC = lerp(blueC,duskC,duskK);
 float3 calibratedC = lerp(nightC,blueC,smoothstep(0.0,.20,lightC));
 calibratedC = lerp(calibratedC,dayC,midC);
 calibratedC += baseC*.32*(1.0-smoothstep(0.0,.12,lightC));
 calibratedC += float3(1.0,.72,.43)*max(Sun,0.0)*pow(sdC,130.0)*.20;
 float faC = saturate(dot(cd,s47finite.unit3(FlashDir)));
 float3 captureC = lerp(baseC,calibratedC,saturate(ConceptBlend));
 captureC += float3(.2,.25,.4)*max(Flash,0.0)*pow(faC,8.0);
 float cirDusk = (1.0-midC)*smoothstep(.02,.30,lightC)*(1.0-smoothstep(.85,1.0,lightC)*.5);
 float3 cirC = lerp(float3(.018,.022,.034),float3(.62,.66,.72),smoothstep(.15,.9,lightC));
 cirC = lerp(cirC,lerp(float3(.55,.30,.30),float3(1.0,.52,.26),eastC*eastC)*(.35+.9*lightC),cirDusk);
 float cirMean = .10*smoothstep(.04,.16,ch)*saturate(Cirrus)*saturate(VolClouds)*lerp(.35,.7,smoothstep(0.0,.3,lightC));
 return max(lerp(captureC,cirC,cirMean),float3(0,0,0));
}
'''


def capture_code(original):
    """Preserve the finite main body exactly, adding only a capture early-return."""
    assert sha256(original) == SOURCE_SHA256, 'Sky recipe changed; review capture source before regenerating'
    finite, _edits = finite_code(original)
    assert sha256(finite) == FINITE_SHA256
    combined = HELPER + BRANCH + finite[len(HELPER):]
    assert combined.replace(BRANCH, '', 1) == finite
    assert sha256(combined) == CAPTURE_SHA256
    return combined
