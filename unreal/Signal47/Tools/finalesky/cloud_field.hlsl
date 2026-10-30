// Original SIGNAL / 47 finite cloud-tank field. Surface/Translucent/Unlit, centimetres.
// Time is Chapter6's explicit Phase input, never engine Time or particle lifetime.
// Ray integration ends at opaque SceneDepth: a cloud behind the hull cannot paint over it.
struct S47FinalCloudField
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
 float fbm(float3 p)
 {
  return .57*noise(p)+.28*noise(p*2.07+float3(11,7,3))+.15*noise(p*4.19-float3(5,2,9));
 }
 float density(float3 p,float seconds,float growth,float side)
 {
  // Thick coherent ellipsoid with a folded, projecting lip. Mirror the fold between banks;
  // preserve a high open sky channel and keep the field inside the finite proxy bounds.
  float envelope=1-smoothstep(.77,1,length(p/float3(.97,.97,.85)));
  float scale=lerp(.42,1,growth);float3 q=p/max(.1,scale);
  float a=side*(seconds*.038+q.y*.42);
  float2 swirl=float2(cos(a)*q.x-sin(a)*q.z,sin(a)*q.x+cos(a)*q.z);
  float2 folded=swirl+float2(.13*sin(q.y*4+seconds*.026),.11*cos(q.x*3-seconds*.023));
  float3 rolling=float3(folded.x,q.y,folded.y);
  float body=1-smoothstep(.58,1.01,length(q/float3(1,.99,.79)));
  float lip=1-smoothstep(.45,.93,length((q-float3(-.27,0,.18))/float3(.79,.94,.53)));
  float n=fbm(rolling*3.5+float3(seconds*.014,side*5,seconds*.009));
  float detail=fbm(rolling*8.3-float3(seconds*.012,0,seconds*.019));
  float clumps=smoothstep(.25,.73,n+.17*detail);
  return saturate((body*.78+lip*.57)*(.38+.83*clumps))*envelope*growth;
 }
};
S47FinalCloudField field;
float3 ray=normalize(WP-Camera);
float3 ro=Camera-BankCenter;
float3 o=float3(dot(ro,Forward),dot(ro,Right),ro.z)/max(BankExtent,1);
float3 d=float3(dot(ray,Forward),dot(ray,Right),ray.z)/max(BankExtent,1);
float3 safeD=sign(d+1e-15)*max(abs(d),1e-12);
float3 t0=(-1-o)/safeD,t1=(1-o)/safeD;
float3 nearT=min(t0,t1),farT=max(t0,t1);
float enter=max(0,max(nearT.x,max(nearT.y,nearT.z)));
float leave=min(farT.x,min(farT.y,farT.z));
// Both SceneDepth and PixelDepth are linear view depth. The ratio recovers ray distance
// at this exact pixel, including off-axis FOV/cockpit/pullout cameras.
float solidDistance=SceneDepth/max(PixelDepth,1)*length(WP-Camera);
leave=min(leave,solidDistance-1);
if(leave<=enter||Growth<=0)return float4(0,0,0,0);
int count=clamp((int)Samples,12,64);
float step=(leave-enter)/count;
float transmittance=1;float3 color=0;
float jitter=field.hash(floor(WP*.018));
for(int i=0;i<64;++i)
{
 if(i>=count||transmittance<.008)break;
 float at=enter+(i+.25+.5*jitter)*step;
 float3 p=o+d*at;
 float rho=field.density(p,Phase,Growth,Side);
 float alpha=1-exp(-rho*.00019*step);
 // Cold edge light and warmer local glow, not a global light or physical fog claim.
 float edge=saturate(1-rho*1.5);
 float upper=saturate(p.z*.5+.5);
 float3 lit=lerp(float3(.012,.019,.031),float3(.18,.23,.28),.34*upper+.35*edge);
 float rear=exp(-dot(p-float3(.25,-Side*.15,.04),p-float3(.25,-Side*.15,.04))*2.1);
 lit+=float3(.20,.115,.055)*Glow*rear;
 color+=transmittance*alpha*lit;
 transmittance*=1-alpha;
}
float opacity=saturate(1-transmittance);
// Standard straight-alpha output, integrated color divided by accumulated opacity.
return float4(color/max(opacity,1e-4),opacity);
