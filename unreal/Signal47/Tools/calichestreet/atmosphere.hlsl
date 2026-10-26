// Caliche concept study: layered dry-air blue hour and broken high cloud.
// Existing night panorama, solar direction, star hole and thunder remain the underlying sky.
float light = saturate(Dawn);
// SKY.md: thin cirrus above the volumetric layer, painted on the dome everywhere it is on. It keeps
// the sunset colours longest; at night only a faint grey veil over the stars.
float2 cpl = d.xz / max(.05, d.y + .03);
float2 cro = float2(dot(cpl, float2(.8, .6)), dot(cpl, float2(-.6, .8))) + CirrusDrift.xy;
float cirS = nz.fbm(float3(cro * float2(.30, 3.6), 2.3)) * .75 + nz.fbm(float3(cro * float2(1.1, 9.0), 6.1)) * .25;
float cirA = saturate((cirS - lerp(.70, .50, saturate(Cirrus))) / .16) * smoothstep(.04, .16, d.y) * saturate(Cirrus) * saturate(VolClouds);
float cirWest = pow(saturate(dot(normalize(d.xz + float2(.0001, 0)), normalize(SunDir.xz + float2(.0001, 0))) * .5 + .5), 2.0);
float cirDusk = (1.0 - Midday) * smoothstep(.02, .30, light) * (1.0 - smoothstep(.85, 1.0, light) * .5);
float3 cirC = lerp(float3(.018, .022, .034), float3(.62, .66, .72), smoothstep(.15, .9, light));
cirC = lerp(cirC, lerp(float3(.55, .30, .30), float3(1.0, .52, .26), cirWest) * (.35 + .9 * light), cirDusk);
cirA *= lerp(.35, .7, smoothstep(.0, .3, light));
if (ConceptBlend < .001) return lerp(col, cirC, cirA);
float horizon = exp(-max(h,0.0)*7.0);
float west = pow(saturate(dot(normalize(d.xz+float2(.0001,0)),normalize(SunDir.xz+float2(.0001,0)))*.5+.5),6.0);
float3 nightSky = lerp(float3(.004,.009,.022),float3(.018,.030,.055),horizon);
float3 blueHour = lerp(float3(.034,.075,.165),float3(.21,.235,.29),horizon);
blueHour += float3(.20,.065,.018)*west*horizon*(1.0-Midday);
float3 daySky = lerp(float3(.055,.19,.42),float3(.38,.50,.59),horizon);
// SKY.md, the dusk concept: a broad orange-to-peach band on the horizon, strongest under the set
// sun but warm all round, fading through mauve to a dusty violet-blue overhead.
float westB = saturate(dot(normalize(d.xz+float2(.0001,0)),normalize(SunDir.xz+float2(.0001,0)))*.5+.5);
float duskK = (1.0-Midday)*smoothstep(.02,.22,light);
float hb = exp(-max(h,0.0)*8.0), hm = exp(-max(h,0.0)*3.0);
float3 duskSky = lerp(float3(.050,.060,.125),float3(.16,.12,.16),hm);
duskSky = lerp(duskSky,lerp(float3(.70,.34,.17),float3(.92,.38,.10),westB*westB),hb*lerp(.85,1.0,westB));
duskSky *= .45+.85*light;
blueHour = lerp(blueHour,duskSky,duskK);
float3 calibrated = lerp(nightSky,blueHour,smoothstep(0.0,.20,light));
calibrated = lerp(calibrated,daySky,Midday);
// Plane projection makes the cloud base horizontal and foreshortens the distant banks.
float2 plane = d.xz / max(.12,d.y+.15);
float3 cp = float3(plane.x*.7,plane.y*.7,4.71);
float bank = nz.fbm(cp+float3(1.4,5.7,0));
float detail = nz.fbm(cp*3.8+float3(8.1,0,2.4));
// Smaller broken altocumulus cells sit on the broad bank instead of one blurred blob.
float cells = nz.fbm(cp*15.0+float3(2.1,8.7,4.2));
float cloud = smoothstep(.46,.60,bank*.62+detail*.23+cells*.15);
cloud *= smoothstep(-.015,.065,h)*(1.0-smoothstep(.72,.99,h));
// SKY.md: with the volumetric layer on, the painted bank stays only below it, where the dome hides the real clouds.
cloud *= 1.0 - saturate(VolClouds)*(.65+.35*smoothstep(.06,.11,h));
float3 cloudNight = float3(.012,.020,.037);
float3 cloudDusk = lerp(float3(.022,.038,.067),float3(.17,.135,.12),west*horizon);
float3 cloudDay = lerp(float3(.29,.38,.49),float3(.72,.76,.77),smoothstep(.48,.72,bank));
float3 cloudCol = lerp(cloudNight,cloudDusk,smoothstep(0.0,.18,light));
cloudCol = lerp(cloudCol,cloudDay,Midday);
calibrated = lerp(calibrated,cloudCol,cloud*.87);
// Keep the painted Milky Way at night; broken cloud reduces its contrast locally.
calibrated += col*.32*(1.0-smoothstep(.0,.12,light))*(1.0-cloud*.8);
calibrated += float3(1.0,.72,.43)*Sun*(pow(max(sd,0.0),130.0)*.20+smoothstep(.99996,.99999,sd)*3.0);
calibrated += float3(.2,.25,.4)*Flash*pow(fa,8.0);
return lerp(lerp(col,calibrated,saturate(ConceptBlend)),cirC,cirA);
