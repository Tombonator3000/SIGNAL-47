// Original SIGNAL / 47 Mantaflow cloud-tank field. Optional alternative to cloud_field.hlsl.
// Custom inputs 1..16: the original 13 plus TextureObject DensityA, DensityB, DensityC.
// Input 17: scalar Density, default 1; scales view extinction and light attenuation.
// Input 18: scalar DecodeScale, from the volumes' shared S47DensityScale metadata.
// Those objects must be Texture3D, linear/clamp/trilinear, with generated samplers
// DensityASampler, DensityBSampler, DensityCSampler. Every read uses SampleLevel 0.
// A/B/C are shared-coordinate baked frames: array[z,y,x], no axis flips,
// UV=(p+1)/2. DecodeScale restores the common atlas density scale (wavelet bake: 2.0).
// Runtime: exit faces via SetReverseCulling(true), two-sided=false, depth test disabled.
// Each pixel integrates once, including inside the proxy. SceneDepth clips opaque geometry.
struct S47FinalCloudVolume
{
 float hash(float3 p)
 {
  p=frac(p*float3(.1031,.1030,.0973));p+=dot(p,p.yxz+33.33);return frac((p.x+p.y)*p.z);
 }
 float noise(float3 p)
 {
  float3 i=floor(p),f=frac(p);f=f*f*(3-2*f);
  return lerp(lerp(lerp(hash(i),hash(i+float3(1,0,0)),f.x),lerp(hash(i+float3(0,1,0)),hash(i+float3(1,1,0)),f.x),f.y),
              lerp(lerp(hash(i+float3(0,0,1)),hash(i+float3(1,0,1)),f.x),lerp(hash(i+float3(0,1,1)),hash(i+1),f.x),f.y),f.z);
 }
 float3 sourcePoint(float3 p,float4 state)
 {
  // state = Phase, Growth, Side, frame coordinate. No engine Time or particle age.
  float bank=step(0,state.z),seconds=state.x+bank*9;
  float3 q=p/lerp(.70,1,saturate(state.y));
  // Opposite banks have a different baked phase as well as a mirrored cross-bank axis.
  q.y*=state.z<0?-1:1;
  float angle=.035*sin(seconds*.012)+.040*q.y+.022*sin(q.y*2.8+seconds*.006+bank);
  float sn=sin(angle),cs=cos(angle);
  float2 turn=q.xz;
  q.x=cs*turn.x-sn*turn.y;
  q.z=sn*turn.x+cs*turn.y;
  // Slow differential shear preserves the actual fluid folds and connected mass.
  q.x+=.016*sin(q.y*3.7+seconds*.010+bank*1.3);
  q.z+=.009*sin(q.y*5.1-q.x*2-seconds*.008);
  q.y+=.008*sin(q.z*4+q.x*3+seconds*.005);
  return q;
 }
 float read(Texture3D a,SamplerState aSampler,Texture3D b,SamplerState bSampler,
            Texture3D c,SamplerState cSampler,float3 p,float4 state,float decodeScale)
 {
  float3 edge=1-smoothstep(float3(.965,.965,.965),float3(.995,.995,.995),abs(p));
  float envelope=edge.x*edge.y*edge.z;
  if(envelope<.0001)return 0;
  float3 uv=sourcePoint(p,state)*.5+.5;
  // Explicit rejection makes even an accidentally wrapped sampler finite. The atlas
  // also has zero border texels; no ellipsoid or synthetic main mass is introduced.
  if(any(uv<=0)||any(uv>=1))return 0;
  float rho=0;
  [branch] if(state.w<1)
  {
   float va=Texture3DSampleLevel(a,aSampler,uv,0).r;
   float vb=Texture3DSampleLevel(b,bSampler,uv,0).r;
   rho=lerp(va,vb,state.w);
  }
  else
  {
   float vb=Texture3DSampleLevel(b,bSampler,uv,0).r;
   float vc=Texture3DSampleLevel(c,cSampler,uv,0).r;
   rho=lerp(vb,vc,state.w-1);
  }
  // One common decode, not per-frame renormalization. Suppress only the first
  // quantization steps so an 8-bit empty border does not become a grey veil.
  rho*=max(0,decodeScale);
  return max(0,rho)*smoothstep(0,.012,rho)*envelope*saturate(state.y);
 }
};
S47FinalCloudVolume field;
float distanceToProxy=max(length(WP-Camera),1);
float3 ray=(WP-Camera)/distanceToProxy;
float3 ro=Camera-BankCenter;
float3 extent=max(BankExtent,1);
float3 o=float3(dot(ro,Forward),dot(ro,Right),ro.z)/extent;
float3 d=float3(dot(ray,Forward),dot(ray,Right),ray.z)/extent;
float3 directionSign=float3(d.x>=0?1:-1,d.y>=0?1:-1,d.z>=0?1:-1);
float3 safeD=directionSign*max(abs(d),1e-10);
// Integrate the full padded cube so a newer fluid bake cannot outgrow an old crop.
// Empty density samples skip all normal/light probes below.
float3 integrationBounds=float3(1,1,1);
float3 t0=(-integrationBounds-o)/safeD,t1=(integrationBounds-o)/safeD;
float3 nearT=min(t0,t1),farT=max(t0,t1);
float enter=max(0,max(nearT.x,max(nearT.y,nearT.z)));
float leave=min(farT.x,min(farT.y,farT.z));
// SceneDepth and PixelDepth are linear view depth. The ratio converts the opaque
// hit to this ray's distance even though WP/PixelDepth belong to the exit face.
float solidDistance=SceneDepth/max(PixelDepth,1)*distanceToProxy;
leave=min(leave,solidDistance-1);
if(leave<=enter||Growth<=0)return float4(0,0,0,0);
float bank=step(0,Side);
float frameCoordinate=2*smoothstep(-35,72,Phase+bank*9);
float4 state=float4(Phase,Growth,Side,frameCoordinate);
int count=clamp((int)Samples,16,128);
float stepLength=(leave-enter)/count;
uint volumeWidth,volumeHeight,volumeDepth;
DensityA.GetDimensions(volumeWidth,volumeHeight,volumeDepth);
// A tetrahedron's diagonal has length sqrt(3). Each probe lies three source
// voxels from its centre: broad paint folds carry the light rather than fine noise.
float3 normalStep=(6.0/1.7320508)/max(float3(volumeWidth,volumeHeight,volumeDepth),1);
normalStep*=lerp(.70,1,saturate(Growth));
float jitter=field.hash(floor((Camera+ray*enter)*.023));
float3 lightDirection=normalize(float3(.61,-.34,.72));
float3 viewLocal=normalize(float3(dot(ray,Forward),dot(ray,Right),ray.z));
float forwardScatter=pow(saturate(dot(viewLocal,lightDirection)),4);
float densityMultiplier=max(.1,Density);
float extinction=.00034*densityMultiplier;
// At high art-directed optical densities, keep the wider light probes softer
// than the view extinction so selected thin edges retain readable transmission.
float lightExtinction=.00034*min(densityMultiplier,sqrt(densityMultiplier)*1.6);
float transmittance=1;
float3 color=0;
for(int i=0;i<128;++i)
{
 if(i>=count||transmittance<.006)break;
 float at=enter+(i+.12+.76*jitter)*stepLength;
 float3 p=o+d*at;
 float raw=field.read(DensityA,DensityASampler,DensityB,DensityBSampler,DensityC,DensityCSampler,p,state,DecodeScale);
 if(raw<.002)continue;
 // Small moving filaments only erode the low-density real fluid. They never
 // manufacture a new cloud outside the bake, and do not perturb the lighting normal.
 float filament=field.noise(p*24+float3(-Phase*.047,Phase*.017,Phase*.033)+bank*13.7);
 float thin=1-smoothstep(.07,.25,raw);
 float rho=max(0,raw-.012*(1-filament)*thin);
 if(rho<.001)continue;
 float alpha=1-exp(-rho*extinction*stepLength);

 // Four tetrahedral 3D probes form a bulk normal at a radius of three atlas voxels.
 // Actual texture density, including its mirrored/warped mapping, drives the normal;
 // there are no fine procedural derivative highlights or reflection/specular lobes.
 float3 e0=float3(1,-1,-1),e1=float3(-1,-1,1),e2=float3(-1,1,-1),e3=float3(1,1,1);
 float g0=field.read(DensityA,DensityASampler,DensityB,DensityBSampler,DensityC,DensityCSampler,p+e0*normalStep,state,DecodeScale);
 float g1=field.read(DensityA,DensityASampler,DensityB,DensityBSampler,DensityC,DensityCSampler,p+e1*normalStep,state,DecodeScale);
 float g2=field.read(DensityA,DensityASampler,DensityB,DensityBSampler,DensityC,DensityCSampler,p+e2*normalStep,state,DecodeScale);
 float g3=field.read(DensityA,DensityASampler,DensityB,DensityBSampler,DensityC,DensityCSampler,p+e3*normalStep,state,DecodeScale);
 float3 gradient=e0*g0+e1*g1+e2*g2+e3*g3;
 // Retain the original gradient-strength units for the existing surface-light
 // gate: increasing texture resolution must not halve its lighting response.
 gradient*=.027/normalStep;
 float3 normal=normalize(-gradient/extent+float3(0,0,1e-10));
 float surface=smoothstep(.015,.18,length(gradient));
 float wrap=saturate((dot(normal,lightDirection)+.22)/1.22);
 float contour=smoothstep(.28,.85,1-abs(dot(normal,viewLocal)));
 float skyWrap=saturate(normal.z*.5+.5);

 // Three physical light distances resolve local curling folds and connected dark cores.
 float rhoNear=field.read(DensityA,DensityASampler,DensityB,DensityBSampler,DensityC,DensityCSampler,p+lightDirection*1800/extent,state,DecodeScale);
 float rhoMid=field.read(DensityA,DensityASampler,DensityB,DensityBSampler,DensityC,DensityCSampler,p+lightDirection*6000/extent,state,DecodeScale);
 float rhoFar=field.read(DensityA,DensityASampler,DensityB,DensityBSampler,DensityC,DensityCSampler,p+lightDirection*14000/extent,state,DecodeScale);
 float opticalDepth=lightExtinction*(900*raw+3000*rhoNear+6100*rhoMid+4000*rhoFar);
 float lightThrough=exp(-opticalDepth*1.25);
 float exposed=smoothstep(.20,.66,lightThrough)*smoothstep(.05,.25,rho);
 float edgeLight=saturate((rho-rhoNear)*3)*exposed*surface*wrap;
 float deepFill=exp(-rho*3);
 float3 lit=float3(.004,.006,.010);
 lit+=float3(.020,.028,.039)*(.15+.85*skyWrap)*(.25+.75*deepFill)*(.25+.75*sqrt(lightThrough));
 // Broad illumination through the fluid carries the silver-grey folds. Keep the
 // density normal for occlusion, but avoid thin luminous contour veins.
 float bulkKey=pow(lightThrough,.60)*(.10+.25*wrap)*(.82+.18*forwardScatter);
 lit+=float3(.34,.39,.44)*(bulkKey+edgeLight*.02);

 // Sparse concealed amber light belongs to the paint near the inward lip.
 // Emission still integrates through real density; empty space never acquires a glow.
 float3 glowAt=(p-float3(-.17,-Side*.27,.035))/float3(.34,.25,.19);
 float source=exp(-dot(glowAt,glowAt)*2.7);
 float seams=exp(-rho*2.2)*(.15+.85*sqrt(lightThrough));
 lit+=float3(.92,.35,.085)*max(0,Glow)*source*seams*(.55+.45*filament);
 color+=transmittance*alpha*lit;
 transmittance*=1-alpha;
}
float opacity=saturate(1-transmittance);
return float4(color/max(opacity,1e-4),opacity);
