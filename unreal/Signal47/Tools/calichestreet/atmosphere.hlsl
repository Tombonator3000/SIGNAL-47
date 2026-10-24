// Caliche concept study: layered dry-air blue hour and broken high cloud.
// Existing night panorama, solar direction, star hole and thunder remain the underlying sky.
float light = saturate(Dawn);
float horizon = exp(-max(h,0.0)*7.0);
float west = pow(saturate(dot(normalize(d.xz+float2(.0001,0)),normalize(SunDir.xz+float2(.0001,0)))*.5+.5),6.0);
float3 nightSky = lerp(float3(.004,.009,.022),float3(.018,.030,.055),horizon);
float3 blueHour = lerp(float3(.034,.075,.165),float3(.21,.235,.29),horizon);
blueHour += float3(.20,.065,.018)*west*horizon*(1.0-Midday);
float3 daySky = lerp(float3(.055,.19,.42),float3(.38,.50,.59),horizon);
float3 calibrated = lerp(nightSky,blueHour,smoothstep(0.0,.20,light));
calibrated = lerp(calibrated,daySky,Midday);
// Plane projection makes the cloud base horizontal and foreshortens the distant banks.
float2 plane = d.xz / max(.12,d.y+.15);
float3 cp = float3(plane.x*.7,plane.y*.7,4.71);
float bank = nz.fbm(cp+float3(1.4,5.7,0));
float detail = nz.fbm(cp*3.8+float3(8.1,0,2.4));
float cloud = smoothstep(.43,.64,bank*.78+detail*.22);
cloud *= smoothstep(-.015,.065,h)*(1.0-smoothstep(.72,.99,h));
float3 cloudNight = float3(.012,.020,.037);
float3 cloudDusk = lerp(float3(.045,.075,.125),float3(.17,.145,.145),west*horizon);
float3 cloudDay = lerp(float3(.29,.38,.49),float3(.72,.76,.77),smoothstep(.48,.72,bank));
float3 cloudCol = lerp(cloudNight,cloudDusk,smoothstep(0.0,.18,light));
cloudCol = lerp(cloudCol,cloudDay,Midday);
calibrated = lerp(calibrated,cloudCol,cloud*.72);
// Keep the painted Milky Way at night; broken cloud reduces its contrast locally.
calibrated += col*.32*(1.0-smoothstep(.0,.12,light))*(1.0-cloud*.8);
calibrated += float3(1.0,.72,.43)*Sun*(pow(max(sd,0.0),130.0)*.20+smoothstep(.99996,.99999,sd)*3.0);
calibrated += float3(.2,.25,.4)*Flash*pow(fa,8.0);
return lerp(col,calibrated,saturate(ConceptBlend));
