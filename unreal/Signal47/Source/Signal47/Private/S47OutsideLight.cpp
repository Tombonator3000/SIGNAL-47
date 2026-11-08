#include "S47OutsideLight.h"
#include "S47CalicheAccess.h"
#include "S47Main.h"
#include "S47SaroNight.h"
#include "S47SaroPower.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UObjectIterator.h"

float GS47FloodFogDistance=-1.f;
float GS47FloodFogPhase=-1.f;

namespace
{
 TAutoConsoleVariable<float> CvCandelas(TEXT("s47.flood.Candelas"),30000.f,TEXT("Massive light: candelas at full."));
 TAutoConsoleVariable<float> CvScatter(TEXT("s47.flood.Scatter"),.45f,TEXT("Massive light: VolumetricScatteringIntensity. 0 takes it out of the volumetric fog."));
 TAutoConsoleVariable<float> CvIndirect(TEXT("s47.flood.Indirect"),.5f,TEXT("Massive light: Lumen bounce, the only thing that lifts the rest of the room."));
 TAutoConsoleVariable<float> CvSource(TEXT("s47.flood.Source"),9.f,TEXT("Massive light: source radius in cm. Small: a window's edge stays an edge."));
 TAutoConsoleVariable<float> CvCone(TEXT("s47.flood.Cone"),36.f,TEXT("Massive light: outer half angle in degrees. The inner is two thirds of it."));
 TAutoConsoleVariable<float> CvSweep(TEXT("s47.flood.Sweep"),.17f,TEXT("Massive light: how far the source drifts, as a share of its distance."));
 TAutoConsoleVariable<float> CvCore(TEXT("s47.flood.Core"),2.4f,TEXT("Massive light: radius in m of what can be seen out there. 0: nothing to see."));
 TAutoConsoleVariable<float> CvCoreGlow(TEXT("s47.flood.CoreGlow"),60.f,TEXT("Massive light: emissive power of the source itself."));
 TAutoConsoleVariable<float> CvHalo(TEXT("s47.flood.Halo"),1.4f,TEXT("Massive light: brightness of the glare round the source."));
 TAutoConsoleVariable<float> CvAirRadial(TEXT("s47.flood.AirRadial"),.15f,TEXT("Room air under the light: extinction per metre at the middle. Rooms with their own haze add theirs."));
 TAutoConsoleVariable<float> CvAirTown(TEXT("s47.flood.AirTown"),.45f,TEXT("The same, where no room has any haze of its own: everywhere but SARO's area."));
 TAutoConsoleVariable<float> CvAirHeight(TEXT("s47.flood.AirHeight"),.03f,TEXT("Room air under the light: extinction per metre low down."));
 TAutoConsoleVariable<float> CvAirStart(TEXT("s47.flood.AirStart"),10.f,TEXT("r.LocalFogVolume.GlobalStartDistance in cm while it lasts. The engine's 1000 keeps a room's air 10 m from the eye, so no shaft shows from inside."));
 TAutoConsoleVariable<float> CvFogDistance(TEXT("s47.flood.FogDistance"),5000.f,TEXT("Volumetric fog reach in cm while it lasts. The grid's slices go into the rooms. 0: as the night has it."));
 TAutoConsoleVariable<float> CvFogPhase(TEXT("s47.flood.FogPhase"),.3f,TEXT("Volumetric fog scattering distribution while it lasts. Forward: shafts burn when looked along. Below 0: as the night has it."));
 TAutoConsoleVariable<int32> CvGridPixel(TEXT("s47.flood.GridPixel"),4,TEXT("r.VolumetricFog.GridPixelSize while it lasts. 0: as the graphics tier has it."));
 TAutoConsoleVariable<int32> CvGridZ(TEXT("s47.flood.GridZ"),128,TEXT("r.VolumetricFog.GridSizeZ while it lasts. 0: as the graphics tier has it."));
 TAutoConsoleVariable<float> CvHistory(TEXT("s47.flood.History"),.7f,TEXT("r.VolumetricFog.HistoryWeight while it lasts: the sweep leaves no trail. Below 0: as it is."));
 TAutoConsoleVariable<int32> CvDust(TEXT("s47.flood.Dust"),420,TEXT("Motes in the air round the eye. Lit by the same light, so they only show in the shafts. 0: none."));
 TAutoConsoleVariable<float> CvDustSize(TEXT("s47.flood.DustSize"),.9f,TEXT("A mote's side in cm."));
 TAutoConsoleVariable<int32> CvDouse(TEXT("s47.flood.Douse"),1,TEXT("The room's own lamps go out while it lasts."));

 constexpr double DustHalfX=5.5,DustHalfY=1.5,DustHalfZ=5.5;
 constexpr double LampReachM=14,DustRoomM=14;
 constexpr int32 DustTracesPerFrame=40;

 double Wrap(double V,double Half){const double Span=Half*2;return V-Span*FMath::FloorToDouble((V+Half)/Span);}
 bool LiveEnough(US47Main* Main){return !IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(Main)&&!Main->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed);}
}

int32 FS47OutsideLight::ShownNow=0;

FS47OutsideLight::~FS47OutsideLight(){Stop();}

FVector FS47OutsideLight::Swept(const FVector& From,const FVector& Aim,double Seconds)
{
 const FVector Along=From-Aim;
 const double Dist=Along.Size();
 const FVector Side=FVector::CrossProduct(Along,FVector(0,1,0)).GetSafeNormal();
 const double Reach=Dist*CvSweep.GetValueOnGameThread();
 return From+Side*(Reach*FMath::Sin(Seconds*.42))+FVector(0,1,0)*(Reach*.35*FMath::Sin(Seconds*.29+1.));
}

bool FS47OutsideLight::FindWayIn(US47Main* Main,const FVector& Chest,FVector& OutFrom)
{
 if(!Main||!Main->Web||!Main->Web->GetWorld())return false;
 UWorld* World=Main->Web->GetWorld();
 const FCollisionQueryParams Query(SCENE_QUERY_STAT(S47OutsideLightWayIn),true);
 for(const double Dist:{20.,14.,28.})for(const double Up:{7.,4.5,2.5,10.})for(int32 I=0;I<24;++I)
 {
  const double A=I*UE_DOUBLE_PI/12+.13;
  const FVector Try=Chest+FVector(FMath::Cos(A)*Dist,Up,FMath::Sin(A)*Dist);
  const FVector Start=S47Three::ToUE(Try),End=S47Three::ToUE(Chest);
  FHitResult Hit;
  if(World->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Query)&&Hit.Distance<FVector::Dist(Start,End)-30.)continue;
  if(World->LineTraceSingleByChannel(Hit,End,Start,ECC_Visibility,Query)&&Hit.Distance<FVector::Dist(Start,End)-30.)continue;
  OutFrom=Try;
  return true;
 }
 return false;
}

void FS47OutsideLight::Hold(const TCHAR* Name,float Want)
{
 IConsoleVariable* V=IConsoleManager::Get().FindConsoleVariable(Name);
 if(!V)return;
 HeldCvars.Add({Name,V->GetFloat()});
 // Below the console in priority: -ExecCmds still decides, for trying other values.
 V->Set(Want,ECVF_SetByGameOverride);
}

void FS47OutsideLight::Acquire(const FVector& Aim)
{
 ++ShownNow;
 Hold(TEXT("r.LocalFogVolume.GlobalStartDistance"),CvAirStart.GetValueOnGameThread());
 if(M->Web&&M->Web->GetGraphicsSettings().Fog==2)
 {
  if(CvGridPixel.GetValueOnGameThread()>0)Hold(TEXT("r.VolumetricFog.GridPixelSize"),float(CvGridPixel.GetValueOnGameThread()));
  if(CvGridZ.GetValueOnGameThread()>0)Hold(TEXT("r.VolumetricFog.GridSizeZ"),float(CvGridZ.GetValueOnGameThread()));
 }
 if(CvHistory.GetValueOnGameThread()>=0)Hold(TEXT("r.VolumetricFog.HistoryWeight"),CvHistory.GetValueOnGameThread());
 GS47FloodFogDistance=CvFogDistance.GetValueOnGameThread();GS47FloodFogPhase=CvFogPhase.GetValueOnGameThread();bFogAsked=true;
 if(M->Web)M->Web->RefreshFog();
 if(CvDouse.GetValueOnGameThread()==0)return;
 S47CalicheAccess::SetLampLevel(0);
 if(M->SaroNight&&M->SaroNight->Power&&M->Web&&M->Web->Area()==TEXT("saro")&&!M->SaroNight->Power->Out()){M->SaroNight->Power->Cut();bPowerCut=true;}
 // Whatever is lit near the room by its own lamp, however it is driven: the flashlight stays with the player.
 UWorld* World=M->Web?M->Web->GetWorld():nullptr;
 const FVector Here=S47Three::ToUE(Aim);
 for(TObjectIterator<ULightComponent> It;It;++It)
 {
  ULightComponent* L=*It;
  if(!IsValid(L)||L->GetWorld()!=World||!L->bAffectsWorld||L==Spot.Get()||L->GetLightType()==LightType_Directional||L->GetFName()==FName(TEXT("S47Flashlight")))continue;
  if(FVector::Dist(L->GetComponentLocation(),Here)>LampReachM*100.)continue;
  L->bAffectsWorld=false;L->MarkRenderStateDirty();Doused.Add(L);
 }
 UE_LOG(LogTemp,Display,TEXT("S47: massive light: %d lamps out near the room, power cut %d"),Doused.Num(),bPowerCut);
}

void FS47OutsideLight::Release()
{
 for(const FHeld& H:HeldCvars)if(IConsoleVariable* V=IConsoleManager::Get().FindConsoleVariable(*H.Name))V->Set(H.Was,ECVF_SetByGameOverride);
 HeldCvars.Reset();
 ShownNow=FMath::Max(0,ShownNow-1);
 if(bFogAsked){GS47FloodFogDistance=-1.f;GS47FloodFogPhase=-1.f;bFogAsked=false;if(M&&M->Web)M->Web->RefreshFog();}
 for(const auto& Weak:Doused)if(ULightComponent* L=Weak.Get()){L->bAffectsWorld=true;L->MarkRenderStateDirty();}
 Doused.Reset();
 if(CvDouse.GetValueOnGameThread()!=0)S47CalicheAccess::SetLampLevel(1);
 if(bPowerCut&&M&&M->SaroNight&&M->SaroNight->Power)M->SaroNight->Power->Restore();
 bPowerCut=false;
}

bool FS47OutsideLight::EnsureParts()
{
 if(!M||!M->Web)return false;
 AS47Web* W=M->Web.Get();
 if(!Spot.IsValid())
 {
  // A new world: nothing of the last one's is left.
  bPartsTried=false;DustSeed.Reset();DustLit.Reset();
  auto* L=NewObject<USpotLightComponent>(W,TEXT("S47OutsideLight"));
  L->SetupAttachment(W->GetRootComponent());L->SetMobility(EComponentMobility::Movable);
  L->SetIntensityUnits(ELightUnits::Candelas);L->SetUseInverseSquaredFalloff(true);
  L->SetCastShadows(true);L->SetCastVolumetricShadow(true);L->ShadowResolutionScale=2.f;
  L->SetSpecularScale(.5f);L->RegisterComponent();L->SetVisibility(false);
  Spot=L;
 }
 if(!bPartsTried)
 {
  bPartsTried=true;
  auto* Lit=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
  auto* Glow=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow"));
  auto* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
  auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
  const auto Quiet=[](UStaticMeshComponent* C)
  {
   C->SetMobility(EComponentMobility::Movable);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetGenerateOverlapEvents(false);
   C->SetCanEverAffectNavigation(false);C->SetCastShadow(false);C->bAffectDistanceFieldLighting=false;C->bAffectDynamicIndirectLighting=false;
   C->bVisibleInReflectionCaptures=false;C->bVisibleInRealTimeSkyCaptures=false;C->bVisibleInRayTracing=false;
  };
  // What is out there, for an eye that looks at the window: seen through the glass and nowhere else, since it is drawn in the world.
  if(Lit&&Sphere)
  {
   auto* C=NewObject<UStaticMeshComponent>(W,TEXT("S47OutsideLightCore"));
   C->SetupAttachment(W->GetRootComponent());Quiet(C);C->SetStaticMesh(Sphere);
   auto* Mid=UMaterialInstanceDynamic::Create(Lit,W);
   Mid->SetVectorParameterValue(TEXT("Color"),FLinearColor::White);Mid->SetScalarParameterValue(TEXT("NativeLighting"),1.f);Mid->SetScalarParameterValue(TEXT("EmissivePower"),0.f);
   C->SetMaterial(0,Mid);C->RegisterComponent();C->SetVisibility(false);Core=C;CoreMid=Mid;
  }
  if(Glow&&Plane)
  {
   auto* C=NewObject<UStaticMeshComponent>(W,TEXT("S47OutsideLightHalo"));
   C->SetupAttachment(W->GetRootComponent());Quiet(C);C->SetStaticMesh(Plane);
   auto* Mid=UMaterialInstanceDynamic::Create(Glow,W);
   Mid->SetVectorParameterValue(TEXT("Color"),FLinearColor::White);Mid->SetScalarParameterValue(TEXT("Brightness"),0.f);
   C->SetMaterial(0,Mid);C->RegisterComponent();C->SetVisibility(false);Halo=C;HaloMid=Mid;
  }
  // Dust is ordinary lit matter, not a glow: out of the light a mote is dark, in it it burns, and the windows' shadows cut it.
  // Instances of a cube, not a new Niagara system: it needs no new asset and a patch can carry it.
  if(Lit&&Cube)
  {
   auto* C=NewObject<UInstancedStaticMeshComponent>(W,TEXT("S47OutsideLightDust"));
   C->SetupAttachment(W->GetRootComponent());Quiet(C);C->SetStaticMesh(Cube);
   auto* Mid=UMaterialInstanceDynamic::Create(Lit,W);
   Mid->SetVectorParameterValue(TEXT("Color"),FLinearColor(.92f,.9f,.85f));Mid->SetScalarParameterValue(TEXT("Roughness"),.55f);Mid->SetScalarParameterValue(TEXT("NativeLighting"),1.f);Mid->SetScalarParameterValue(TEXT("EmissivePower"),0.f);
   C->SetMaterial(0,Mid);C->RegisterComponent();C->SetVisibility(false);Dust=C;DustMid=Mid;
  }
 }
 return Spot.IsValid();
}

void FS47OutsideLight::StepCore(const FFrame& Frame)
{
 const double Radius=CvCore.GetValueOnGameThread();
 const float K=float(FMath::Clamp(Frame.K,0.,1.5));
 const FVector At=S47Three::ToUE(Frame.From);
 FVector Eye=At+FVector(1000,0,-500);
 if(UWorld* World=M->Web->GetWorld())if(auto* PC=World->GetFirstPlayerController())if(PC->PlayerCameraManager)Eye=PC->PlayerCameraManager->GetCameraLocation();
 if(auto* C=Core.Get())
 {
  const bool bOn=Radius>.01&&K>.001;
  C->SetVisibility(bOn);
  if(bOn)
  {
   C->SetWorldLocation(At);C->SetWorldScale3D(FVector(Radius*2));
   if(auto* Mid=CoreMid.Get()){Mid->SetVectorParameterValue(TEXT("Color"),Frame.Color);Mid->SetScalarParameterValue(TEXT("EmissivePower"),K*CvCoreGlow.GetValueOnGameThread());}
  }
 }
 if(auto* C=Halo.Get())
 {
  const bool bOn=Radius>.01&&K>.001&&CvHalo.GetValueOnGameThread()>0;
  C->SetVisibility(bOn);
  if(bOn)
  {
   const FVector ToEye=(Eye-At).GetSafeNormal();
   C->SetWorldLocation(At+ToEye*FMath::Min(Radius*100.+40.,FVector::Dist(Eye,At)*.5));
   C->SetWorldRotation(FRotationMatrix::MakeFromZ(ToEye.IsNearlyZero()?FVector::ForwardVector:ToEye).Rotator());
   C->SetWorldScale3D(FVector(Radius*8.,Radius*8.,1.));
   if(auto* Mid=HaloMid.Get()){Mid->SetVectorParameterValue(TEXT("Color"),Frame.Color);Mid->SetScalarParameterValue(TEXT("Brightness"),K*CvHalo.GetValueOnGameThread());}
  }
 }
}

void FS47OutsideLight::StepDust(const FFrame& Frame)
{
 auto* C=Dust.Get();
 if(!C)return;
 const int32 Count=FMath::Clamp(CvDust.GetValueOnGameThread(),0,4000);
 // Air round someone at the room: out in the yard it would be specks a metre from the eye.
 FVector Eye=S47Three::ToUE(Frame.Aim);
 if(UWorld* World=M->Web->GetWorld())if(auto* PC=World->GetFirstPlayerController())if(PC->PlayerCameraManager)Eye=PC->PlayerCameraManager->GetCameraLocation();
 const bool bOn=Count>0&&Frame.K>.001&&FVector::Dist(Eye,S47Three::ToUE(Frame.Aim))<DustRoomM*100.;
 C->SetVisibility(bOn);
 if(!bOn)return;
 if(DustSeed.Num()!=Count)
 {
  FRandomStream Rand(4711);
  DustSeed.SetNum(Count);DustMoved.SetNum(Count);DustLit.Init(0.f,Count);DustNext=0;
  C->ClearInstances();
  for(FVector& Seed:DustSeed)Seed=FVector(Rand.FRandRange(-1.f,1.f),Rand.FRandRange(-1.f,1.f),Rand.FRandRange(-1.f,1.f));
  for(int32 I=0;I<Count;++I)C->AddInstance(FTransform::Identity,true);
 }
 const FVector EyeThree=S47Three::ToThree(Eye);
 // A few each frame: is this one in the light? It shows only there, and comes and goes softly as the shadows pass.
 if(UWorld* World=M->Web->GetWorld())
 {
  const FCollisionQueryParams Query(SCENE_QUERY_STAT(S47OutsideLightDust),true);
  const FVector Source=S47Three::ToUE(Frame.From);
  for(int32 N=0;N<DustTracesPerFrame&&N<Count;++N)
  {
   DustNext=(DustNext+1)%Count;
   FHitResult Hit;
   DustLit[DustNext]=World->LineTraceSingleByChannel(Hit,DustMoved[DustNext].GetLocation(),Source,ECC_Visibility,Query)?0.f:1.f;
  }
 }
 const double Side=CvDustSize.GetValueOnGameThread()*.01,T=Frame.Seconds;
 for(int32 I=0;I<Count;++I)
 {
  const FVector& S=DustSeed[I];
  // A slow turning of the air, each on its own: rising a little where the light warms it.
  const double P=I*.61;
  const FVector Place(S.X*DustHalfX+T*.03*S.Z+.16*FMath::Sin(T*.37+P),S.Y*DustHalfY+T*.018+.08*FMath::Sin(T*.53+P*1.3),S.Z*DustHalfZ-T*.025*S.X+.16*FMath::Cos(T*.31+P*.7));
  const FVector Three(EyeThree.X+Wrap(Place.X-EyeThree.X,DustHalfX),EyeThree.Y+Wrap(Place.Y-EyeThree.Y+.2,DustHalfY),EyeThree.Z+Wrap(Place.Z-EyeThree.Z,DustHalfZ));
  DustMoved[I]=FTransform(FRotator(T*37.+I*11.,T*23.+I*7.,T*29.+I*3.).Quaternion(),S47Three::ToUE(Three),FVector(Side*(.6+.4*FMath::Abs(S.Y))*DustLit[I]));
 }
 C->BatchUpdateInstancesTransforms(0,DustMoved,true,true,true);
}

void FS47OutsideLight::Show(const FFrame& Frame)
{
 if(!EnsureParts())return;
 if(!bShown)
 {
  bShown=true;
  Acquire(Frame.Aim);
 }
 USpotLightComponent* L=Spot.Get();
 const FVector FromUE=S47Three::ToUE(Frame.From),AimUE=S47Three::ToUE(Frame.Aim);
 const float Reach=float(FVector::Dist(FromUE,AimUE)+2500.);
 const float Cone=CvCone.GetValueOnGameThread(),Source=CvSource.GetValueOnGameThread();
 L->SetWorldLocation(FromUE);L->SetWorldRotation((AimUE-FromUE).Rotation());
 L->SetOuterConeAngle(Cone);L->SetInnerConeAngle(Cone*.66f);L->SetAttenuationRadius(Reach);
 L->SetSourceRadius(Source);L->SetSoftSourceRadius(0.f);
 L->SetLightColor(Frame.Color);L->SetIntensity(float(CvCandelas.GetValueOnGameThread()*Frame.K));
 L->SetVolumetricScatteringIntensity(CvScatter.GetValueOnGameThread());L->SetIndirectLightingIntensity(CvIndirect.GetValueOnGameThread());
 const bool bShadowed=!Frame.bThroughRoof;
 if(bool(L->CastShadows)!=bShadowed)L->SetCastShadows(bShadowed);
 L->SetVisibility(Frame.K>.001);
 StepCore(Frame);
 StepDust(Frame);
}

void FS47OutsideLight::Hide()
{
 if(auto* L=Spot.Get()){L->SetIntensity(0);L->SetVisibility(false);}
 if(auto* C=Core.Get())C->SetVisibility(false);
 if(auto* C=Halo.Get())C->SetVisibility(false);
 if(auto* C=Dust.Get())C->SetVisibility(false);
 if(bShown){bShown=false;Release();}
}

void FS47OutsideLight::SetAir(double K,const FVector& At,const FLinearColor& Albedo,bool bRoom)
{
 if(!M||!M->Web)return;
 auto* F=Air.Get();
 if(!F)
 {
  if(K<=0)return;
  F=NewObject<ULocalFogVolumeComponent>(M->Web.Get(),TEXT("S47OutsideLightAir"));
  F->SetupAttachment(M->Web->GetRootComponent());F->RegisterComponent();
  F->SetFogPhaseG(.5f);F->SetHeightFogFalloff(320.f);
  Air=F;
 }
 F->SetWorldLocation(S47Three::ToUE(At));
 // A room's own measure: wide enough for any room a player stands in, and no higher than a ceiling and a little.
 F->SetWorldScale3D(bRoom?FVector(2.4,2.4,.72):FVector(1.5,1.5,.5));
 const bool bOwnHaze=M->Web->Area()==TEXT("saro");
 F->SetRadialFogExtinction(float(K*(!bRoom?1.1f:bOwnHaze?CvAirRadial.GetValueOnGameThread():CvAirTown.GetValueOnGameThread())));
 F->SetHeightFogExtinction(float(K*(bRoom?CvAirHeight.GetValueOnGameThread():.7f)));
 F->SetFogAlbedo(Albedo);
 F->SetVisibility(K>.002);
}

void FS47OutsideLight::Stop()
{
 const bool bLive=LiveEnough(M);
 if(bLive)
 {
  Hide();
  if(auto* C=Spot.Get())C->DestroyComponent();
  if(auto* C=Air.Get())C->DestroyComponent();
  if(auto* C=Core.Get())C->DestroyComponent();
  if(auto* C=Halo.Get())C->DestroyComponent();
  if(auto* C=Dust.Get())C->DestroyComponent();
 }
 else{bShown=false;GS47FloodFogDistance=-1.f;GS47FloodFogPhase=-1.f;}
 Spot.Reset();Air.Reset();Core.Reset();Halo.Reset();Dust.Reset();CoreMid.Reset();HaloMid.Reset();DustMid.Reset();
 DustSeed.Reset();DustMoved.Reset();DustLit.Reset();HeldCvars.Reset();Doused.Reset();bPartsTried=false;bPowerCut=false;bFogAsked=false;
}
