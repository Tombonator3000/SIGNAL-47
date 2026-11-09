#include "S47ShedGlow.h"
#include "S47FarmBuildings.h"
#include "S47FarmShedDoor.h"
#include "S47FarmWorld.h"
#include "S47Main.h"
#include "S47Omen.h"
#include "S47OutsideLight.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/GarbageCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47ShedGlow,Log,All);

// WEEKLY-CASE-20261109, patch 2. See S47ShedGlow.h. Lumen and the fog are measured on the GPU at night (the plan's patch 2 note);
// these are first values, all of them console variables.
namespace S47ShedGlowLocal
{
 TAutoConsoleVariable<float> CvLumens(TEXT("s47.shedglow.Lumens"),9000.f,TEXT("Shed glow: the main light inside, lumens at full."));
 TAutoConsoleVariable<float> CvLowLumens(TEXT("s47.shedglow.LowLumens"),1400.f,TEXT("Shed glow: the low light behind the door that makes the stripe under it, lumens."));
 TAutoConsoleVariable<float> CvScatter(TEXT("s47.shedglow.Scatter"),4.f,TEXT("Shed glow: the main light's VolumetricScatteringIntensity (the shafts in the yard's air)."));
 TAutoConsoleVariable<float> CvSpill(TEXT("s47.shedglow.Spill"),2.2f,TEXT("Shed glow: brightness of the spill on the ground under the door."));
 TAutoConsoleVariable<float> CvAir(TEXT("s47.shedglow.Air"),.32f,TEXT("Shed glow: the local fog round the shed, radial extinction per metre at full."));
 TAutoConsoleVariable<float> CvAirHeight(TEXT("s47.shedglow.AirHeight"),.12f,TEXT("Shed glow: the same fog, height extinction."));
 TAutoConsoleVariable<float> CvAirStart(TEXT("s47.shedglow.AirStart"),10.f,TEXT("Shed glow: r.LocalFogVolume.GlobalStartDistance in cm while lit, so the air shows from close by."));
 TAutoConsoleVariable<float> CvFogDistance(TEXT("s47.shedglow.FogDistance"),5000.f,TEXT("Shed glow: the volumetric fog's reach in cm while lit (as the massive light asks). 0: as the night has it."));
 TAutoConsoleVariable<float> CvOmenReach(TEXT("s47.shedglow.OmenReach"),110.f,TEXT("Shed glow: metres within which the lamp and the lantern feel it."));
 const FLinearColor Cold(.78f,.84f,1.f);
 constexpr int32 ShedSite=3;
 const TCHAR* const OmenId=TEXT("weekcase:shed");
 bool LiveEnough(US47Main* M){return !IsEngineExitRequested()&&!GExitPurge&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed);}
 double Ease(double K,double Want,double Up,double Down,double Dt){return Want>K?FMath::Min(Want,K+Up*Dt):FMath::Max(Want,K-Down*Dt);}
}

FS47ShedGlow::FLevel FS47ShedGlow::LevelFor(FS47WeekCase::EPhase Phase,double Age)
{
 FLevel L;
 switch(Phase)
 {
 case FS47WeekCase::EPhase::Phase1:L.Omen=.28;break;   // the yard's lamp blinks; nothing shows yet
 case FS47WeekCase::EPhase::Phase2:
  L.Stripe=FMath::Clamp(Age/2.,0.,1.);
  L.Shafts=FMath::Clamp((Age-StripeSecs)/RampSecs,0.,1.);
  L.Omen=.5+.25*L.Shafts;
  break;
 case FS47WeekCase::EPhase::Phase3:L.Stripe=1;L.Shafts=1;L.Omen=.8;break;
 default:break;
 }
 return L;
}

FS47ShedGlow::~FS47ShedGlow(){Stop();}

FVector FS47ShedGlow::RoadOf(double LocalX,double LocalY,double Up) const
{
 const TArray<S47FarmBuildings::FSite>& Sites=S47FarmBuildings::Sites();
 if(!Sites.IsValidIndex(S47ShedGlowLocal::ShedSite))return FVector::ZeroVector;
 const S47FarmBuildings::FSite& S=Sites[S47ShedGlowLocal::ShedSite];
 const FVector2D R=S47FarmBuildings::LocalToRoad(S,LocalX,LocalY);
 return FVector(R.X,S47FarmBuildings::Anchor(S).Y+Up,R.Y);
}

bool FS47ShedGlow::EnsureParts()
{
 if(!M||!IsValid(M->Web))return false;
 AS47Web* W=M->Web.Get();
 if(Inner.IsValid()&&Inner->GetOwner()==W)return true;
 // None yet, or the last world's: made here, in this one.
 bPartsTried=true;Air.Reset();Spill.Reset();SpillMid.Reset();
 const auto Light=[W](const TCHAR* Name,float Radius,float Scatter)
 {
  auto* L=NewObject<UPointLightComponent>(W,Name);
  L->SetupAttachment(W->GetRootComponent());L->SetMobility(EComponentMobility::Movable);
  L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(0.f);L->SetLightColor(S47ShedGlowLocal::Cold);
  L->SetAttenuationRadius(Radius);L->SetSourceRadius(4.f);L->SetSoftSourceRadius(0.f);
  // Shadowed, both: the light only gets out where the shed lets it.
  L->SetCastShadows(true);L->SetCastVolumetricShadow(true);L->SetVolumetricScatteringIntensity(Scatter);
  L->RegisterComponent();L->SetVisibility(false);
  return L;
 };
 Inner=Light(TEXT("S47ShedGlowInner"),1400.f,S47ShedGlowLocal::CvScatter.GetValueOnGameThread());
 Low=Light(TEXT("S47ShedGlowLow"),450.f,1.5f);
 auto* Glow=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow"));
 auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
 if(Glow&&Plane)
 {
  auto* C=NewObject<UStaticMeshComponent>(W,TEXT("S47ShedGlowSpill"));
  C->SetupAttachment(W->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Plane);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->bVisibleInReflectionCaptures=false;C->bVisibleInRayTracing=false;C->bAffectDistanceFieldLighting=false;
  auto* Mid=UMaterialInstanceDynamic::Create(Glow,W);
  Mid->SetVectorParameterValue(TEXT("Color"),S47ShedGlowLocal::Cold);Mid->SetScalarParameterValue(TEXT("Brightness"),0.f);
  C->SetMaterial(0,Mid);C->RegisterComponent();C->SetVisibility(false);
  Spill=C;SpillMid=Mid;
 }
 return Inner.IsValid();
}

void FS47ShedGlow::Hold(const TCHAR* Name,float Want)
{
 IConsoleVariable* V=IConsoleManager::Get().FindConsoleVariable(Name);
 if(!V)return;
 HeldCvars.Add({Name,V->GetFloat()});
 V->Set(Want,ECVF_SetByGameOverride);
}

void FS47ShedGlow::Acquire()
{
 if(bHolding)return;
 bHolding=true;
 // The massive light holds the same values when it is up; it goes first, and nothing here is taken back from under it.
 if(FS47OutsideLight::AnyShown())return;
 Hold(TEXT("r.LocalFogVolume.GlobalStartDistance"),S47ShedGlowLocal::CvAirStart.GetValueOnGameThread());
 if(GS47FloodFogDistance<0&&S47ShedGlowLocal::CvFogDistance.GetValueOnGameThread()>0)
 {
  GS47FloodFogDistance=S47ShedGlowLocal::CvFogDistance.GetValueOnGameThread();GS47FloodFogPhase=.3f;bFogAsked=true;
  if(M&&M->Web)M->Web->RefreshFog();
 }
 UE_LOG(LogS47ShedGlow,Display,TEXT("S47: shed glow: up, the yard's air held for its shafts"));
}

void FS47ShedGlow::Release()
{
 if(!bHolding)return;
 bHolding=false;
 for(const FHeld& H:HeldCvars)if(IConsoleVariable* V=IConsoleManager::Get().FindConsoleVariable(*H.Name))V->Set(H.Was,ECVF_SetByGameOverride);
 HeldCvars.Reset();
 if(bFogAsked){GS47FloodFogDistance=-1.f;GS47FloodFogPhase=-1.f;bFogAsked=false;if(M&&M->Web)M->Web->RefreshFog();}
}

void FS47ShedGlow::Step(double Dt,const FLevel& Want,bool bDoorGone)
{
 using namespace S47ShedGlowLocal;
 if(!M||!IsValid(M->Web))return;
 const double D=FMath::Clamp(Dt,0.,.1);T+=D;
 // The lamp and the lantern: one source every reader feels (S47Omen MissesFor lets "weekcase:" through to all of them).
 if(M->Omen)
 {
  FS47OmenSource Src;Src.Id=OmenId;Src.At=S47FarmWorld::RoadToArea(M->Web->Area(),RoadOf(0,0,1.2));Src.Severity=Want.Omen;Src.Reach=CvOmenReach.GetValueOnGameThread();
  if(Want.Omen>0){M->Omen->Post(Src,1.);bOmen=true;}
  else if(bOmen){M->Omen->Post(Src,0.);bOmen=false;}
 }
 KStripe=Ease(KStripe,Want.Stripe,1.,2.,D);
 KShafts=Ease(KShafts,Want.Shafts,bDoorGone?4.:1./RampSecs*1.6,1.5,D);
 const bool bWant=KStripe>.001||KShafts>.001;
 if(!bWant){if(bLit)Off();return;}
 if(!EnsureParts())return;
 if(!bLit){bLit=true;Acquire();}
 const FString Area=M->Web->Area();
 const auto At=[&](double X,double Y,double Up){return S47Three::ToUE(S47FarmWorld::RoadToArea(Area,RoadOf(X,Y,Up)));};
 // Not steady: it breathes, and now and then it nearly goes.
 const double Breath=.86+.1*FMath::Sin(T*7.3)*FMath::Sin(T*2.1+.4)+.04*FMath::Sin(T*23.);
 const double Dip=FMath::Frac(T*.37)<.04?.45:1.;
 const double K=FMath::Clamp(Breath*Dip,0.,1.2);
 const FVector2D H=FS47FarmShedDoor::HingeLocal();const double DoorX=H.X+FS47FarmShedDoor::LeafWidth*.5;
 if(UPointLightComponent* L=Inner.Get())
 {
  L->SetWorldLocation(At(-.3,-.2,1.75));
  L->SetIntensity(float(CvLumens.GetValueOnGameThread()*KShafts*K));L->SetVolumetricScatteringIntensity(CvScatter.GetValueOnGameThread());
  L->SetVisibility(KShafts>.001);
 }
 if(UPointLightComponent* L=Low.Get())
 {
  // Low behind the leaf: with the door shut, only the gap under it lets this one out.
  L->SetWorldLocation(At(DoorX,H.Y+.42,.07));
  L->SetIntensity(float(CvLowLumens.GetValueOnGameThread()*KStripe*K*(bDoorGone?.3:1.)));
  L->SetVisibility(KStripe>.001);
 }
 if(UStaticMeshComponent* C=Spill.Get())
 {
  const FVector Road=RoadOf(DoorX,H.Y-.24,0);
  const FVector OnGround(Road.X,S47FarmWorld::FloorRoad(Road.X,Road.Z)+.012,Road.Z);
  C->SetWorldLocation(S47Three::ToUE(S47FarmWorld::RoadToArea(Area,OnGround)));
  C->SetWorldRotation(FRotator(0,FMath::RadiansToDegrees(-S47FarmBuildings::Sites()[ShedSite].Yaw),0));
  C->SetWorldScale3D(bDoorGone?FVector(1.6,1.8,1):FVector(1.1,.42,1));
  if(UMaterialInstanceDynamic* Mid=SpillMid.Get())Mid->SetScalarParameterValue(TEXT("Brightness"),float(CvSpill.GetValueOnGameThread()*FMath::Max(KStripe,KShafts)*K));
  C->SetVisibility(true);
 }
 // The yard's air round the shed, thicker toward its front, where the shafts come out.
 ULocalFogVolumeComponent* F=Air.Get();
 if(F&&F->GetOwner()!=M->Web.Get())F=nullptr;
 if(!F)
 {
  F=NewObject<ULocalFogVolumeComponent>(M->Web.Get(),TEXT("S47ShedGlowAir"));
  F->SetupAttachment(M->Web->GetRootComponent());F->RegisterComponent();F->SetFogPhaseG(.45f);F->SetHeightFogFalloff(260.f);
  Air=F;
 }
 F->SetWorldLocation(At(0,-1.2,1.));F->SetWorldScale3D(FVector(6.5,6.5,1.7));
 F->SetRadialFogExtinction(float(CvAir.GetValueOnGameThread()*KShafts));F->SetHeightFogExtinction(float(CvAirHeight.GetValueOnGameThread()*KShafts));
 F->SetFogAlbedo(FLinearColor(.85f,.88f,.95f));F->SetVisibility(KShafts>.002);
}

void FS47ShedGlow::Off()
{
 KStripe=KShafts=0;
 if(bOmen&&M&&M->Omen&&S47ShedGlowLocal::LiveEnough(M)){FS47OmenSource Src;Src.Id=S47ShedGlowLocal::OmenId;M->Omen->Post(Src,0.);}
 bOmen=false;
 if(S47ShedGlowLocal::LiveEnough(M))
 {
  for(UPointLightComponent* L:{Inner.Get(),Low.Get()})if(L){L->SetIntensity(0.f);L->SetVisibility(false);}
  if(auto* C=Spill.Get())C->SetVisibility(false);
  if(auto* F=Air.Get())F->SetVisibility(false);
 }
 if(bLit)UE_LOG(LogS47ShedGlow,Display,TEXT("S47: shed glow: out"));
 bLit=false;
 Release();
}

void FS47ShedGlow::Stop()
{
 const bool bLive=S47ShedGlowLocal::LiveEnough(M);
 if(bLive)
 {
  Off();
  for(UPointLightComponent* L:{Inner.Get(),Low.Get()})if(L)L->DestroyComponent();
  if(auto* C=Spill.Get())C->DestroyComponent();
  if(auto* F=Air.Get())F->DestroyComponent();
 }
 else{bLit=false;bOmen=false;HeldCvars.Reset();bHolding=false;if(bFogAsked){GS47FloodFogDistance=-1.f;GS47FloodFogPhase=-1.f;bFogAsked=false;}}
 Inner.Reset();Low.Reset();Spill.Reset();SpillMid.Reset();Air.Reset();bPartsTried=false;
}

FString FS47ShedGlow::StateText() const
{
 return FString::Printf(TEXT("glow %s, stripe %.2f, shafts %.2f"),bLit?TEXT("lit"):TEXT("out"),KStripe,KShafts);
}

bool FS47ShedGlow::SelfTest(TArray<FString>& Lines)
{
 using EP=FS47WeekCase::EPhase;
 bool bDark=true;
 for(const EP P:{EP::Hook,EP::Day,EP::Night,EP::Phase1,EP::Out,EP::Done})for(double Age=0;Age<900;Age+=7.5)bDark&=!LevelFor(P,Age).Lit();
 bool bStripeFirst=true;
 for(double Age=.5;Age<StripeSecs;Age+=.25){const FLevel L=LevelFor(EP::Phase2,Age);bStripeFirst&=L.Stripe>0&&L.Shafts==0;}
 const FLevel Full=LevelFor(EP::Phase2,StripeSecs+RampSecs+1),Breach=LevelFor(EP::Phase3,0);
 const bool bThen=Full.Shafts==1&&Full.Stripe==1&&Breach.Shafts==1&&Breach.Lit();
 const bool bOmen=LevelFor(EP::Phase1,10).Omen>0&&LevelFor(EP::Phase1,10).Omen<LevelFor(EP::Phase2,10).Omen&&LevelFor(EP::Day,10).Omen==0&&LevelFor(EP::Out,10).Omen==0;
 const bool bGood=bDark&&bStripeFirst&&bThen&&bOmen;
 Lines.Add(FString(bGood?TEXT("PASS"):TEXT("FAIL"))+TEXT(" shed glow: dark outside phases two and three; in phase two the stripe under the door first, then the shafts; the lamp and lantern feel it from phase one, harder from two"));
 return bGood;
}
