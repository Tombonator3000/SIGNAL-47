// Original SIGNAL / 47 finite cloud-tank field. Surface/Translucent/Unlit, centimetres.
// Phase is Chapter6's explicit seconds, never engine Time or particle lifetime.
// Preserve all 13 Custom inputs. SceneDepth terminates integration at opaque geometry.
// Runtime renders the cube's exit faces once with SetReverseCulling(true),
// two-sided disabled and material depth testing disabled. This also works inside
// the proxy; SceneDepth below still clips the volume against opaque geometry.
struct S47FinalCloudField
{
 float hash(float3 p)
 {
  p=frac(p*float3(.1031,.1030,.0973));p+=dot(p,p.yxz+33.33);return frac((p.x+p.y)*p.z);
 }
 // Value and analytic first derivatives share the same eight lattice hashes.
 // This gives the raymarch a detailed density normal without six extra marches.
 float4 noiseGradient(float3 p)
 {
  float3 cell=floor(p),f=frac(p);
  float3 u=f*f*(3-2*f),du=6*f*(1-f);
  float a=hash(cell),b=hash(cell+float3(1,0,0));
  float c=hash(cell+float3(0,1,0)),e=hash(cell+float3(1,1,0));
  float g=hash(cell+float3(0,0,1)),h=hash(cell+float3(1,0,1));
  float j=hash(cell+float3(0,1,1)),k=hash(cell+1);
  float z0=lerp(lerp(a,b,u.x),lerp(c,e,u.x),u.y);
  float z1=lerp(lerp(g,h,u.x),lerp(j,k,u.x),u.y);
  float gx=lerp(lerp(b-a,e-c,u.y),lerp(h-g,k-j,u.y),u.z)*du.x;
  float gy=lerp(lerp(c-a,e-b,u.x),lerp(j-g,k-h,u.x),u.z)*du.y;
  return float4(lerp(z0,z1,u.z),gx,gy,(z1-z0)*du.z);
 }
 float4 join(float4 a,float4 b,float k)
 {
  float h=saturate(.5+.5*(b.x-a.x)/k);
  return float4(lerp(b.x,a.x,h)-k*h*(1-h),lerp(b.yzw,a.yzw,h));
 }
 float4 ellipsoid(float3 p,float3 center,float3 radii)
 {
  float3 v=(p-center)/radii;
  float r=max(length(v),.0001),radius=min(radii.x,min(radii.y,radii.z));
  return float4((r-1)*radius,v/r/radii*radius);
 }
 float4 medium(float3 p,float seconds,float growth,float side)
 {
  float3 edge=1-smoothstep(float3(.84,.84,.82),float3(.985,.985,.975),abs(p));
  float envelope=edge.x*edge.y*edge.z;
  if(envelope<.0001)return float4(0,0,0,1);
  float variant=step(0,side);
  float scale=lerp(.38,.97,saturate(growth));
  float3 q=(p-float3(.06*(1-growth),0,-.08*(1-growth)))/scale;
  float3 seed=lerp(float3(4.3,17.1,8.6),float3(31.8,3.7,22.4),variant);
  float4 broad=noiseGradient(q*1.9+seed+float3(seconds*.0035,0,-seconds*.002));
  float3 w=q;
  w.x+=.13*sin(q.y*3.7+broad.x*3.0+seconds*.008);
  w.y+=.06*sin(q.x*3.1+q.z*2.7+seed.x);
  w.z+=.16*(broad.x-.5)+.065*sin(q.y*4.2+seed.y);

  // Connected paint shelves rather than one inflated cap. Narrow joins preserve
  // the overhanging steps; a rear stalk connects all of the rolling front lips.
  float4 shape=ellipsoid(w,float3(.28,.02,-.20),float3(.45,.78,.39));
  shape=join(shape,ellipsoid(w,float3(-.25+.06*variant,-.40,.17+.06*variant),float3(.51,.43,.29)),.08);
  shape=join(shape,ellipsoid(w,float3(-.33-.03*variant,.17-.10*variant,.09),float3(.49,.48,.30)),.09);
  shape=join(shape,ellipsoid(w,float3(-.03+.04*variant,.60,.27-.08*variant),float3(.43,.28,.26)),.07);
  shape=join(shape,ellipsoid(w,float3(.10,-.04,.46-.07*variant),float3(.43,.58,.22)),.075);
  shape=join(shape,ellipsoid(w,float3(-.34,-.09,-.27),float3(.40,.61,.18)),.065);
  float4 undercut=ellipsoid(w,float3(-.58,.01,-.075),float3(.35,.74,.115));
  if(-undercut.x-.012>shape.x)shape=-undercut-float4(.012,0,0,0);

  // A differential vortex folds the paint back underneath its forward lip.
  // Only the internal field advects; the large connected shelves remain heavy.
  float2 curl=w.xz-float2(-.23,.12);
  float angle=side*(.55*w.y+seconds*.023*exp(-dot(curl,curl)*2.6));
  float sn=sin(angle),cs=cos(angle);
  float3 roll=float3(cs*curl.x-sn*curl.y,w.y,sn*curl.x+cs*curl.y);
  // Roughly compensate for the authored bank's wide/shallow physical aspect.
  // Rotate and permute octave domains so folds do not share long lattice rills.
  float3 metric=float3(1,1.30,.69);
  float3 m=roll*metric;
  float3 puff=float3(.8*m.x+.6*m.z,m.y,-.6*m.x+.8*m.z);
  float4 billow=noiseGradient(puff*3.8+seed+float3(seconds*.006,0,-seconds*.004));
  float4 folds=noiseGradient(puff.yzx*8.7+seed.yzx+float3(-seconds*.011,seconds*.003,seconds*.008));
  float4 small=noiseGradient(puff.zxy*19.1+seed.zxy+float3(-seconds*.026,seconds*.009,seconds*.022));
  float3 filamentFrequency=float3(34,28,38);
  float3 flow=puff*filamentFrequency+seed*1.7+float3(-seconds*.071,seconds*.014,seconds*.049);
  flow.x+=.38*sin(puff.z*5.5+puff.y*3+seconds*.036);
  float4 fine=noiseGradient(flow);
  // Multiscale density relief is strong enough to alter the silhouette as well
  // as the normal. Fine erosion lives on the paint's surface, not throughout a fog box.
  float relief=.36*(.51-billow.x)+.155*(.49-folds.x)+.052*(.49-small.x)+.012*(.5-fine.x);
  float eroded=shape.x+relief;
  float core=1-smoothstep(-.041,.022,eroded);
  float paint=smoothstep(.20,.78,.53*billow.x+.31*folds.x+.16*small.x);
  float filaments=smoothstep(.61,.79,.58*small.x+.42*fine.x);
  float fringe=(1-smoothstep(.012,.085,shape.x+.28*(.5-billow.x)))*(1-core)*filaments*.14;
  float rho=core*(.68+.85*paint)+fringe;
  // Transport follows the rounded bulk, not every microscopic filament normal.
  // Fine relief still erodes density, but cannot turn the whole skin to silver.
  float3 puffGradient=-.36*3.8*.75*billow.yzw-.155*8.7*.42*folds.wyz-.052*19.1*.10*small.zwy;
  float3 reliefGradient=float3(.8*puffGradient.x-.6*puffGradient.z,puffGradient.y,.6*puffGradient.x+.8*puffGradient.z)*metric;
  float3 gradient=shape.yzw+float3(cs*reliefGradient.x+sn*reliefGradient.z,reliefGradient.y,-sn*reliefGradient.x+cs*reliefGradient.z);
  return float4(max(0,rho)*envelope*saturate(growth),gradient/scale);
 }
 float density(float3 p,float seconds,float growth,float side)
 {
  return medium(p,seconds,growth,side).x;
 }
};
S47FinalCloudField field;
float distanceToProxy=max(length(WP-Camera),1);
float3 ray=(WP-Camera)/distanceToProxy;
float3 ro=Camera-BankCenter;
float3 extent=max(BankExtent,1);
float3 o=float3(dot(ro,Forward),dot(ro,Right),ro.z)/extent;
float3 d=float3(dot(ray,Forward),dot(ray,Right),ray.z)/extent;
float3 directionSign=float3(d.x>=0?1:-1,d.y>=0?1:-1,d.z>=0?1:-1);
float3 safeD=directionSign*max(abs(d),1e-10);
float3 t0=(-1-o)/safeD,t1=(1-o)/safeD;
float3 nearT=min(t0,t1),farT=max(t0,t1);
float enter=max(0,max(nearT.x,max(nearT.y,nearT.z)));
float leave=min(farT.x,min(farT.y,farT.z));
// Both inputs are linear view depth. Their ratio recovers the opaque ray distance
// at this pixel, including off-axis FOV, the cockpit and the pullout cameras.
float solidDistance=SceneDepth/max(PixelDepth,1)*distanceToProxy;
leave=min(leave,solidDistance-1);
if(leave<=enter||Growth<=0)return float4(0,0,0,0);
int count=clamp((int)Samples,12,64);
float stepLength=(leave-enter)/count;
float transmittance=1;
float3 color=0;
float jitter=field.hash(floor((Camera+ray*enter)*.023));
// One consistent cold source across both banks, from above and behind the mass.
float3 lightDirection=normalize(float3(.67,-.34,.72));
float3 viewLocal=normalize(float3(dot(ray,Forward),dot(ray,Right),ray.z));
float forwardScatter=pow(saturate(dot(viewLocal,lightDirection)),4);
for(int i=0;i<64;++i)
{
 if(i>=count||transmittance<.006)break;
 float at=enter+(i+.12+.76*jitter)*stepLength;
 float3 p=o+d*at;
 float4 medium=field.medium(p,Phase,Growth,Side);
 float rho=medium.x;
 if(rho<.001)continue;
 float alpha=1-exp(-rho*.000245*stepLength);
 // Analytic density normals reveal folds without a glossy/specular lobe.
 // Convert the gradient from bank coordinates to physical local centimetres.
 float3 normal=normalize(medium.yzw/extent+float3(0,0,1e-10));
 float wrap=saturate((dot(normal,lightDirection)+.15)/1.15);
 float skyWrap=saturate(normal.z*.5+.5);
 float contour=smoothstep(.30,.86,1-abs(dot(normal,viewLocal)));
 // Local folds occlude at 32m; the 135m probe keeps a connected dark interior.
 float rhoNear=field.density(p+lightDirection*3200/extent,Phase,Growth,Side);
 float rhoFar=field.density(p+lightDirection*13500/extent,Phase,Growth,Side);
 float lightThrough=exp(-(rho*.22+rhoNear*.70+rhoFar*.95)*3.15);
 float exposed=smoothstep(.18,.58,lightThrough)*smoothstep(.09,.40,rho);
 float edgeLight=saturate((rho-rhoNear)*2.1)*exposed*wrap;
 float deepFill=exp(-rho*3.0);
 float3 lit=float3(.004,.006,.009);
 lit+=float3(.034,.044,.057)*(.12+.88*skyWrap)*(.20+.80*deepFill)*(.30+.70*sqrt(lightThrough));
 lit+=float3(.38,.49,.61)*(lightThrough*(.025+.58*wrap*wrap)*(.20+.80*contour)*(.67+.33*forwardScatter)+edgeLight*.08);
 // A narrow concealed source washes only folds near the inward lip. The warm
 // transmission belongs to thin paint; opaque cores retain their cold dark mass.
 float3 glowAt=(p-float3(-.02,-Side*.31,.13+.04*step(0,Side)))/float3(.44,.25,.25);
 float source=exp(-dot(glowAt,glowAt)*2.45);
 float seams=exp(-rho*2.3)*(.12+.88*saturate(1-rhoFar));
 lit+=float3(1.16,.46,.12)*max(0,Glow)*source*seams;
 color+=transmittance*alpha*lit;
 transmittance*=1-alpha;
}
float opacity=saturate(1-transmittance);
// Standard straight-alpha output: RGB is unpremultiplied exactly once here.
return float4(color/max(opacity,1e-4),opacity);
