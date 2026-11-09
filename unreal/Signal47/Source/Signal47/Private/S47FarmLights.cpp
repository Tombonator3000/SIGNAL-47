#include "S47FarmLights.h"

#include "S47FarmBuildings.h"
#include "S47FarmHouse.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Glow.h"
#include "S47Happenings.h"
#include "S47Main.h"
#include "S47Omen.h"
#include "S47StreetLamps.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Weather.h"
#include "S47Web.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FarmLights,Log,All);

static TAutoConsoleVariable<float> CvBarn(TEXT("s47.farmlights.Barn"),300.f,TEXT("FARM-LIGHTS: the barn's bulb, lumens. 0: dark."));
static TAutoConsoleVariable<float> CvHouse(TEXT("s47.farmlights.House"),320.f,TEXT("FARM-LIGHTS: the old house's sitting room lamp, lumens. 0: dark."));
static TAutoConsoleVariable<float> CvYard(TEXT("s47.farmlights.Yard"),950.f,TEXT("FARM-LIGHTS: the yard's dusk-to-dawn lamp, candela. 0: dark."));
static TAutoConsoleVariable<float> CvSway(TEXT("s47.farmlights.Sway"),1.f,TEXT("FARM-LIGHTS: how hard the draught pushes the barn's bulb (1 as made)."));

namespace
{
 // ---- the farm's own measures (unreal/Signal47/Tools/farmbuildings/barn.py, unreal/ArtSource/Lived/house/build_house.py) ----
 // Barn, in its own metres (x east, y north, z up over its anchor): the tie beams over the aisle run at y = POSTS, their underside at
 // EAVE - .22; the loft starts at y .87. The bulb hangs from the second tie, off the rope with its hook (x .1), over the aisle.
 constexpr double BarnBulbX=2.3,BarnBulbY=-1.15,TieUnder=3.9-.22;
 // The yard lamp's bracket on the south gable (y -5.5, its boards a few cm proud), between the big door (to x 3.7) and the window
 // (from x 5.6), under the eave (3.9).
 constexpr double YardX=5.3,YardFace=-5.53,YardZ=3.75,ArmReach=1.2;
 // The sitting room's ceiling bowl (build_house.py: lathe at plan (9.3, 6.6), WH - .14, ceiling 2.6), the light just under it.
 constexpr double BowlPlanX=9.3,BowlPlanY=6.6,BowlUnder=2.6-.14,LampUnder=2.40;
 // The parts of the bulb along the cord, from its end: the shade (7 cm tall), the bulb's middle, the light in it.
 constexpr double ShadeMid=.035,BulbMid=.10;
 constexpr double Gravity=9.81,Damping=.6;
 // Mercury vapour as it reads at night: blue-green white; cold at the strike, bluer and dim.
 const FLinearColor Mercury(.66f,.86f,1.f),MercuryCold(.35f,.72f,1.f);
 constexpr double WarmSeconds=240,RestrikeSeconds=90; // game clock seconds

 double Hash(int64 N,uint32 Seed)
 {
  uint32 X=uint32(N*7919)^(Seed*0x9E3779B9u);
  X^=X>>16;X*=0x7feb352du;X^=X>>15;X*=0x846ca68bu;X^=X>>16;
  return double(X)/4294967295.*2.-1.;
 }
 // Smooth value noise, -1 to 1.
 double Noise(double T,uint32 Seed)
 {
  const double I=FMath::FloorToDouble(T),F=T-I,U=F*F*(3-2*F);
  return FMath::Lerp(Hash(int64(I),Seed),Hash(int64(I)+1,Seed),U);
 }

 const S47FarmBuildings::FSite& Barn(){return S47FarmBuildings::Sites()[0];}
 // A point in the barn's own metres, z over its anchor, as road-local three-space.
 FVector BarnLocal(double X,double Y,double Z)
 {
  const FVector2D P=S47FarmBuildings::LocalToRoad(Barn(),X,Y);
  return FVector(P.X,S47FarmBuildings::Anchor(Barn()).Y+Z,P.Y);
 }

 struct FPart{TWeakObjectPtr<UStaticMeshComponent> C;FVector At=FVector::ZeroVector;FQuat Q=FQuat::Identity;FVector Scale=FVector::OneVector;bool bMesh=false;};
 struct FGlowRow{int32 Node=-1,Index=-1;};
 struct FState
 {
  TWeakObjectPtr<AS47Web> Web;
  bool bBuilt=false,bTried=false,bShown=false;
  FString Area;
  double T=0;
  int32 Onsets=-1;
  // barn
  TWeakObjectPtr<UStaticMeshComponent> Cord,Shade,Bulb;
  TWeakObjectPtr<UMaterialInstanceDynamic> BulbMat;
  TWeakObjectPtr<UPointLightComponent> BarnLight;
  S47FarmLights::FSwing Swing;
  double BarnK=0,BarnNear=0;
  // yard
  TArray<FPart> YardParts;
  S47StreetLamps::FGlow YardGlow;
  TWeakObjectPtr<UMaterialInstanceDynamic> YardLensBox; // the box head's lens when the street lamps' models are missing
  TWeakObjectPtr<USpotLightComponent> YardLight;
  FVector YardAt=FVector::ZeroVector,YardAim=FVector::ZeroVector;
  double YardFrom=-1,YardK=0,YardNear=0;
  // house
  TWeakObjectPtr<UStaticMeshComponent> Bowl;
  TWeakObjectPtr<UMaterialInstanceDynamic> BowlMat;
  TWeakObjectPtr<UPointLightComponent> HouseLight;
  double HouseK=0,HouseNear=0;
  int32 EventOutNight=-1;
  FString HouseWhy;
  TArray<FGlowRow> Glows;
  double GlowRetry=0;
  bool bGlowTouched=false;
 };
 TMap<TWeakObjectPtr<US47Main>,TSharedPtr<FState>>& States(){static TMap<TWeakObjectPtr<US47Main>,TSharedPtr<FState>> S;return S;}
 FState* StateOf(US47Main* M)
 {
  TSharedPtr<FState>* S=States().Find(TWeakObjectPtr<US47Main>(M));
  return S&&S->IsValid()?S->Get():nullptr;
 }

 UStaticMesh* Mesh(const TCHAR* Path){return LoadObject<UStaticMesh>(nullptr,Path,nullptr,LOAD_NoWarn|LOAD_Quiet);}
 UMaterialInterface* Optional(const TCHAR* Package)
 {
  if(!FPackageName::DoesPackageExist(Package))return nullptr;
  return LoadObject<UMaterialInterface>(nullptr,*(FString(Package)+TEXT(".")+FPackageName::GetShortName(Package)),nullptr,LOAD_NoWarn|LOAD_Quiet);
 }

 bool Build(FState& S,AS47Web* W)
 {
  UStaticMesh* Cube=Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));UStaticMesh* Sphere=Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  UStaticMesh* Cylinder=Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));UStaticMesh* Cone=Mesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
  UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
  if(!Cube||!Sphere||!Cylinder||!Cone||!Base)return false;
  const auto Mat=[W,Base](const FLinearColor& Colour,float Rough,float Metal)
  {
   auto* D=UMaterialInstanceDynamic::Create(Base,W);
   D->SetVectorParameterValue(TEXT("Color"),Colour);D->SetScalarParameterValue(TEXT("Roughness"),Rough);D->SetScalarParameterValue(TEXT("Metalness"),Metal);
   D->SetScalarParameterValue(TEXT("NativeLighting"),1);D->SetScalarParameterValue(TEXT("EmissivePower"),0);
   return D;
  };
  const auto Part=[W](const TCHAR* Name,UStaticMesh* Shape,UMaterialInterface* Material,bool bShadow)
  {
   auto* C=NewObject<UStaticMeshComponent>(W,MakeUniqueObjectName(W,UStaticMeshComponent::StaticClass(),Name));
   C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(W->GetRootComponent());C->SetStaticMesh(Shape);
   if(Material)for(int32 I=0;I<C->GetNumMaterials();++I)C->SetMaterial(I,Material);
   C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetGenerateOverlapEvents(false);
   C->SetCanEverAffectNavigation(false);C->SetCastShadow(bShadow);C->SetVisibility(false);C->SetHiddenInGame(true);
   C->RegisterComponent();
   return C;
  };
  const auto Point=[W](const TCHAR* Name,const FLinearColor& Colour,float Radius,float Source)
  {
   auto* L=NewObject<UPointLightComponent>(W,MakeUniqueObjectName(W,UPointLightComponent::StaticClass(),Name));
   L->SetupAttachment(W->GetRootComponent());L->SetMobility(EComponentMobility::Movable);
   L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(0);L->SetLightColor(Colour);L->SetAttenuationRadius(Radius);
   L->SetSourceRadius(Source);L->SetCastShadows(true);L->SetVolumetricScatteringIntensity(0);L->SetVisibility(false);L->RegisterComponent();
   return L;
  };
  UMaterialInterface* Steel=Optional(TEXT("/Game/FarmBuildings/Materials/M_Farm_Steel"));
  if(!Steel)Steel=Mat(FLinearColor(.16f,.17f,.16f),.45f,.6f);
  const FLinearColor Warm=FLinearColor::MakeFromColorTemperature(2700.f);

  // ---- the barn: cord, tin shade, bulb, its light ----
  // The shade and the cord cast nothing: a bare bulb's light everywhere, so the posts' and the beams' shadows go up the walls as it swings.
  S.Cord=Part(TEXT("S47FarmLightBarnCord"),Cylinder,Mat(FLinearColor(.025f,.022f,.02f),.8f,0),false);
  S.Shade=Part(TEXT("S47FarmLightBarnShade"),Cone,Steel,false);
  auto* BulbMat=Mat(FLinearColor(.80f,.78f,.72f),.15f,0);S.BulbMat=BulbMat;
  S.Bulb=Part(TEXT("S47FarmLightBarnBulb"),Sphere,BulbMat,false);
  S.BarnLight=Point(TEXT("S47FarmLightBarnLight"),Warm,1000.f,3.f);

  // ---- the yard: the street lamps' wood arm and cobra head on the gable, boxes without them ----
  const FVector Out=S47FarmLights::YardOut(),Pivot=S47FarmLights::YardMount(),Head=Pivot+Out*ArmReach;
  const FQuat Q(FVector(0,1,0),FMath::Atan2(-Out.Z,Out.X));
  UStaticMesh* WoodArm=Mesh(TEXT("/Game/Signal47/Art/StreetLamps/SM_StreetLamp_WoodArm.SM_StreetLamp_WoodArm"));
  UStaticMesh* Cobra=WoodArm?Mesh(TEXT("/Game/Signal47/Art/StreetLamps/SM_StreetLamp_Head.SM_StreetLamp_Head")):nullptr;
  if(WoodArm&&Cobra)
  {
   auto* A=Part(TEXT("S47FarmLightYardArm"),WoodArm,nullptr,true);S47StreetLamps::MatchLodSlots(A);
   auto* H=Part(TEXT("S47FarmLightYardHead"),Cobra,nullptr,true);
   // STREETLAMP-GLOW: the lens on every LOD and its halo, as the street lamps' own
   S.YardGlow=S47StreetLamps::Dress(W,H);
   S.YardParts.Add({A,Pivot,Q,FVector::OneVector,true});S.YardParts.Add({H,Head,Q,FVector::OneVector,true});
   S.YardAt=Head+Q.RotateVector(S47StreetLamps::SpotLocal());
  }
  else
  {
   auto* Dark=Mat(FLinearColor(.09f,.09f,.085f),.6f,.3f);
   auto* Lens=Mat(FLinearColor(.5f,.55f,.55f),.3f,0);S.YardLensBox=Lens;
   const bool bAlongX=FMath::Abs(Out.X)>FMath::Abs(Out.Z);
   const auto Size=[bAlongX](double Along,double Up,double Across){return bAlongX?FVector(Along,Across,Up):FVector(Across,Along,Up);}; // UE: x, y (three z), z up
   S.YardParts.Add({Part(TEXT("S47FarmLightYardArm"),Cube,Dark,true),Pivot+Out*(ArmReach*.5),FQuat::Identity,Size(ArmReach,.06,.06),false});
   S.YardParts.Add({Part(TEXT("S47FarmLightYardHead"),Cube,Dark,true),Head+Out*.3+FVector(0,-.05,0),FQuat::Identity,Size(.62,.14,.32),false});
   S.YardParts.Add({Part(TEXT("S47FarmLightYardLens"),Cube,Lens,false),Head+Out*.32+FVector(0,-.13,0),FQuat::Identity,Size(.42,.03,.24),false});
   S.YardAt=Head+Out*.32+FVector(0,-.17,0);
  }
  S.YardAim=Head+Out*9;S.YardAim.Y=S47FarmWorld::FloorRoad(S.YardAim.X,S.YardAim.Z);
  {
   auto* L=NewObject<USpotLightComponent>(W,MakeUniqueObjectName(W,USpotLightComponent::StaticClass(),TEXT("S47FarmLightYardLight")));
   L->SetupAttachment(W->GetRootComponent());L->SetMobility(EComponentMobility::Movable);
   L->SetIntensityUnits(ELightUnits::Candelas);L->SetIntensity(0);L->SetLightColor(Mercury);
   L->SetInnerConeAngle(38.f);L->SetOuterConeAngle(70.f);L->SetAttenuationRadius(3200.f);L->SetSourceRadius(6.f);
   L->SetCastShadows(false);L->SetVolumetricScatteringIntensity(.3f);L->SetVisibility(false);L->RegisterComponent();
   S.YardLight=L;
  }

  // ---- the old house: the ceiling bowl's glass glowing, and the room's light under it ----
  auto* BowlMat=Mat(FLinearColor(.95f,.92f,.85f),.3f,0);S.BowlMat=BowlMat;
  S.Bowl=Part(TEXT("S47FarmLightHouseBowl"),Sphere,BowlMat,false);
  S.HouseLight=Point(TEXT("S47FarmLightHouseLight"),Warm,900.f,10.f);
  UE_LOG(LogS47FarmLights,Display,TEXT("S47: farm lights built: barn bulb at road %s, yard lamp (%s) at %s, house lamp at %s"),
   *S47FarmLights::BarnPivot().ToString(),WoodArm&&Cobra?TEXT("wood arm and cobra head"):TEXT("boxes"),*S.YardAt.ToString(),*S47FarmLights::HouseLamp().ToString());
  return true;
 }

 void Show(UStaticMeshComponent* C,bool bOn){if(C&&(C->IsVisible()!=bOn||C->bHiddenInGame==bOn)){C->SetVisibility(bOn);C->SetHiddenInGame(!bOn);}}
 void Dark(ULocalLightComponent* L){if(L){L->SetIntensity(0);if(L->IsVisible())L->SetVisibility(false);}}

 void PlaceStatic(FState& S,const FString& Area)
 {
  for(const FPart& P:S.YardParts)if(auto* C=P.C.Get())
  {
   const FVector At=S47FarmWorld::RoadToArea(Area,P.At);
   if(P.bMesh){double Mx[16];S47Three::Compose(At,P.Q,FVector::OneVector,Mx);C->SetWorldTransform(S47Three::MatToUE(Mx));}
   else{C->SetWorldLocationAndRotation(S47Three::ToUE(At),FRotator::ZeroRotator);C->SetWorldScale3D(P.Scale);}
  }
  if(auto* L=S.YardLight.Get())
  {
   const FVector At=S47FarmWorld::RoadToArea(Area,S.YardAt),Aim=S47FarmWorld::RoadToArea(Area,S.YardAim);
   L->SetWorldLocationAndRotation(S47Three::ToUE(At),S47Three::DirToUE(Aim-At).Rotation());
  }
  const FVector Lamp=S47FarmWorld::RoadToArea(Area,S47FarmLights::HouseLamp());
  if(auto* L=S.HouseLight.Get())L->SetWorldLocation(S47Three::ToUE(Lamp));
  if(auto* C=S.Bowl.Get())
  {
   // A flat lens of light at the bowl's underside (the bowl itself is in the house's model).
   C->SetWorldLocationAndRotation(S47Three::ToUE(Lamp+FVector(0,BowlUnder+.015-LampUnder,0)),FRotator::ZeroRotator);C->SetWorldScale3D(FVector(.2,.2,.045));
  }
 }

 // The exported glow sprites that S47FarmWorld moved to the house's window (its WindowGlowsAtHouse).
 void FindGlows(FState& S,AS47Web* W,double Dt)
 {
  if(S.Glows.Num()>=3)return;
  S.GlowRetry-=Dt;if(S.GlowRetry>0)return;
  S.GlowRetry=1;S.Glows.Reset();
  const FVector Window=S47FarmBuildings::EastWindow();
  for(const int32 Node:{991,1125,1312})if(const FS47GlowPoints* G=W->Glow(Node))for(int32 I=0;I<G->Num();++I)
   if(FVector::Dist(G->Pos[I],Window)<.05)S.Glows.Add({Node,I});
 }
 void SetGlows(FState& S,AS47Web* W,float Scale)
 {
  for(const FGlowRow& R:S.Glows)if(FS47GlowPoints* G=W->Glow(R.Node))G->SetOutputScale(R.Index,Scale);
  S.bGlowTouched=true;
 }

 void HideAll(FState& S,US47Main* M)
 {
  if(!S.bShown)return;
  S.bShown=false;
  for(UStaticMeshComponent* C:{S.Cord.Get(),S.Shade.Get(),S.Bulb.Get(),S.Bowl.Get()})Show(C,false);
  for(const FPart& P:S.YardParts)Show(P.C.Get(),false);
  Dark(S.BarnLight.Get());Dark(S.YardLight.Get());Dark(S.HouseLight.Get());
  if(S.YardGlow)S47StreetLamps::Shine(S.YardGlow,FLinearColor::Black);
  // The sprite as S47FarmWorld leaves it: lit at night (18:45 to 06:15) in Freeform, as exported elsewhere.
  if(S.bGlowTouched&&S.Web.IsValid())
  {
   const double Hour=M&&M->Freeform?FMath::Fmod(M->Freeform->Clock(),86400.)/3600.:20.;
   SetGlows(S,S.Web.Get(),M&&M->IsFreeform()&&!(Hour>=18.75||Hour<6.25)?0.f:1.f);
   S.bGlowTouched=false;
  }
 }
}

namespace S47FarmLights
{
 bool BarnOn(double Hour){Hour=FMath::Fmod(Hour+24.,24.);return Hour>=18.75||Hour<6.25;}
 // The photocell trips a little after sunset (18:27) and lets go before sunrise (05:30) is full day.
 bool YardOn(double Hour){Hour=FMath::Fmod(Hour+24.,24.);return Hour>=18.8||Hour<5.6;}
 bool HouseTimerOn(double Hour){Hour=FMath::Fmod(Hour+24.,24.);return Hour>=18.75&&Hour<22.5;}

 double Flicker(double Level,double T)
 {
  if(!FMath::IsFinite(Level)||Level<.12)return 1;
  const double L=FMath::Clamp((Level-.12)/.88,0.,1.);
  // The filament sags with the supply, and drops out in moments, more of them the nearer it is.
  const double Sag=1-.3*L,N=.5+.5*Noise(T*6.3,21),Gate=L*.6;
  const double Drop=N<Gate?FMath::Lerp(.04,.5,N/FMath::Max(Gate,1e-6)):1.;
  return FMath::Clamp(Sag*Drop,0.,1.);
 }

 bool StepSwing(FSwing& S,double Dt,double DriveA,double DriveB)
 {
  if(!(Dt>0))return false;
  // A pendulum with the air's drag on the shade; the push acts on the bulb at the cord's end.
  S.VA+=(-(Gravity/CordLength)*FMath::Sin(S.A)-Damping*S.VA+DriveA/CordLength)*Dt;
  S.VB+=(-(Gravity/CordLength)*FMath::Sin(S.B)-Damping*S.VB+DriveB/CordLength)*Dt;
  S.A+=S.VA*Dt;S.B+=S.VB*Dt;
  if(!FMath::IsFinite(S.A)||!FMath::IsFinite(S.B)||!FMath::IsFinite(S.VA)||!FMath::IsFinite(S.VB)){S=FSwing();return true;}
  const double R=FMath::Sqrt(S.A*S.A+S.B*S.B);
  if(R<=MaxSwing)return false;
  // The hard stop: never further out than this, and nothing more outwards.
  const double UA=S.A/R,UB=S.B/R;S.A=UA*MaxSwing;S.B=UB*MaxSwing;
  const double Outward=S.VA*UA+S.VB*UB;if(Outward>0){S.VA-=Outward*UA;S.VB-=Outward*UB;}
  return true;
 }

 FVector2D Drive(double T,double Strength,double DirX,double DirZ)
 {
  FVector2D Dir(DirX,DirZ);if(!Dir.Normalize())Dir=FVector2D(.6,-.8);
  // Gusts through the boards along the wind's way, about the cord's own beat (2.3 s), and a little across it.
  const double N1=Noise(T*.45,11),N2=Noise(T*.45+31.7,12),N3=Noise(T*.41+77.1,13);
  return FVector2D(Strength*(Dir.X*N1+.35*N2),Strength*(Dir.Y*N1+.35*N3));
 }

 double DriveStrength(double Wind,double Gust,double Near)
 {
  const auto C=[](double V){return FMath::IsFinite(V)?FMath::Clamp(V,0.,1.):0.;};
  // A barn is never still inside: the draught alone keeps it going a degree or so; a hard wind a few degrees more.
  return .3+.5*C(Wind)+.25*C(Gust)+.35*C(Near);
 }

 FVector BarnPivot(){return BarnLocal(BarnBulbX,BarnBulbY,TieUnder);}
 FVector YardMount(){return BarnLocal(YardX,YardFace,YardZ);}
 FVector YardOut()
 {
  const FVector2D A=S47FarmBuildings::LocalToRoad(Barn(),0,0),B=S47FarmBuildings::LocalToRoad(Barn(),0,-1);
  return FVector(B.X-A.X,0,B.Y-A.Y).GetSafeNormal();
 }
 FVector HouseLamp(){return S47FarmHouse::Point(BowlPlanX,BowlPlanY)+FVector(0,LampUnder,0);}

 void Update(US47Main* M,double Dt)
 {
  static const bool bOff=FParse::Param(FCommandLine::Get(),TEXT("S47FarmLightsOff"));
  if(bOff||!M||!M->Web||!M->Web->IsLoaded())return;
  for(auto It=States().CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
  TSharedPtr<FState>& Slot=States().FindOrAdd(TWeakObjectPtr<US47Main>(M));
  if(!Slot)Slot=MakeShared<FState>();
  FState& S=*Slot;
  AS47Web* W=M->Web.Get();
  if(S.Web.Get()!=W){S=FState();S.Web=W;}
  Dt=FMath::IsFinite(Dt)?FMath::Clamp(Dt,0.,.25):0.;
  const FString Area=W->Area();
  const bool bArea=Area==TEXT("saro")||Area==TEXT("road")||Area==TEXT("diner");
  const bool bShow=M->Mode==US47Main::EMode::Play&&M->IsFreeform()&&M->Freeform&&M->Player&&bArea&&S47FarmWorld::BuildingsShown(M);
  if(!bShow){HideAll(S,M);return;}
  if(!S.bBuilt)
  {
   if(S.bTried)return;
   S.bTried=true;S.bBuilt=Build(S,W);
   if(!S.bBuilt){UE_LOG(LogS47FarmLights,Warning,TEXT("S47: farm lights: the engine's basic shapes or the web material are missing; none built"));return;}
  }
  if(!S.bShown||S.Area!=Area){PlaceStatic(S,Area);S.Area=Area;}
  S.bShown=true;
  const bool bHeld=M->bPausedByMenu||M->ModalOpen();
  if(!bHeld)S.T+=Dt;
  const double Clock=M->Freeform->Clock(),Hour=FMath::Fmod(Clock,86400.)/3600.;
  const FVector Eye(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);
  const FS47Omen* Omen=M->Omen.Get();
  const auto Near=[Omen](const FVector& At){return Omen?Omen->SenseAt(At,FS47Omen::SenseWiden,nullptr,nullptr,ES47OmenChannel::Lamp):0.;};
  const auto Away=[&Eye](const FVector& At){return FVector::Dist(Eye,At);};

  // ---- the barn ----
  {
   const FVector Pivot=S47FarmWorld::RoadToArea(Area,BarnPivot());
   S.BarnNear=Near(Pivot);
   if(!bHeld)
   {
    const S47Weather::FLive& Wx=S47Weather::Live();
    const double Wind=Wx.bOn?Wx.Wind:0.,Gust=Omen?Omen->Ordinary(ES47Ordinary::Gust):0.;
    // Knocked when something new comes within the barn's sense (the beam it hangs from takes a weight, a door moves).
    if(Omen&&Omen->Onsets()!=S.Onsets)
    {
     if(S.Onsets>=0&&S.BarnNear>.05){const double A=Hash(Omen->Onsets(),5)*UE_DOUBLE_PI;S.Swing.VA+=FMath::Cos(A)*(.06+.08*S.BarnNear);S.Swing.VB+=FMath::Sin(A)*(.06+.08*S.BarnNear);}
     S.Onsets=Omen->Onsets();
    }
    const double Strength=DriveStrength(Wind,Gust,S.BarnNear)*FMath::Max(0.f,CvSway.GetValueOnGameThread());
    const int32 Steps=FMath::Clamp(FMath::CeilToInt(Dt*60.),1,15);const double H=Dt/Steps;
    for(int32 I=0;I<Steps;++I)
    {
     const double At=S.T-Dt+H*(I+1);const FVector2D D=Drive(At,Strength,Wx.bOn?Wx.DirX:.6,Wx.bOn?Wx.DirZ:-.8);
     StepSwing(S.Swing,H,D.X,D.Y);
    }
   }
   const double SA=FMath::Sin(S.Swing.A),SB=FMath::Sin(S.Swing.B);
   const FVector Down=FVector(SA,-FMath::Sqrt(FMath::Max(0.,1-SA*SA-SB*SB)),SB).GetSafeNormal();
   const FVector End=Pivot+Down*CordLength;
   const bool bNear=Away(Pivot)<150;
   const FRotator Up=FRotationMatrix::MakeFromZ(-S47Three::DirToUE(Down)).Rotator();
   if(bNear)
   {
    if(auto* C=S.Cord.Get()){C->SetWorldLocationAndRotation(S47Three::ToUE((Pivot+End)*.5),Up);C->SetWorldScale3D(FVector(.006,.006,CordLength));}
    if(auto* C=S.Shade.Get()){C->SetWorldLocationAndRotation(S47Three::ToUE(End+Down*ShadeMid),Up);C->SetWorldScale3D(FVector(.24,.24,.07));}
    if(auto* C=S.Bulb.Get()){C->SetWorldLocationAndRotation(S47Three::ToUE(End+Down*BulbMid),Up);C->SetWorldScale3D(FVector(.065,.065,.10));}
   }
   for(UStaticMeshComponent* C:{S.Cord.Get(),S.Shade.Get(),S.Bulb.Get()})Show(C,bNear);
   S.BarnK=BarnOn(Hour)?Flicker(S.BarnNear,S.T):0.;
   if(auto* D=S.BulbMat.Get())
   {
    D->SetScalarParameterValue(TEXT("EmissivePower"),float(3.*S.BarnK));
    D->SetVectorParameterValue(TEXT("Emissive"),S.BarnK>0?FLinearColor(1.f,.72f,.42f):FLinearColor::Black);
   }
   if(auto* L=S.BarnLight.Get())
   {
    const bool bLit=bNear&&Away(Pivot)<90&&S.BarnK>.001;
    if(bLit)L->SetWorldLocation(S47Three::ToUE(End+Down*BulbMid));
    L->SetIntensity(bLit?float(CvBarn.GetValueOnGameThread()*S.BarnK):0.f);
    if(L->IsVisible()!=bLit)L->SetVisibility(bLit);
   }
  }

  // ---- the yard ----
  {
   const FVector At=S47FarmWorld::RoadToArea(Area,S.YardAt);
   S.YardNear=Near(At);
   double Cold=0;
   if(!YardOn(Hour))S.YardFrom=-1;
   else
   {
    // When the photocell tripped tonight, from the clock: a lamp already on when the game is opened is warm already.
    if(S.YardFrom<0||Clock<S.YardFrom-RestrikeSeconds-1)S.YardFrom=Clock-FMath::Fmod((Hour-18.8+24.)*3600.,86400.);
    // Mercury goes out with a dip in the supply and will not strike again until it has cooled.
    if(S.YardNear>.45)S.YardFrom=FMath::Max(S.YardFrom,Clock+RestrikeSeconds);
    Cold=FMath::Clamp((Clock-S.YardFrom)/WarmSeconds,0.,1.);
   }
   const bool bStruck=YardOn(Hour)&&Clock>=S.YardFrom;
   double K=bStruck?.12+.88*FMath::SmoothStep(0.,1.,Cold):0.;
   if(K>0&&S.YardNear>.15)K*=1-.3*FMath::Clamp((S.YardNear-.15)/.3,0.,1.)*(.5+.5*Noise(S.T*9.1,31));
   S.YardK=K;
   const FLinearColor Colour=FMath::Lerp(MercuryCold,Mercury,float(Cold));
   const bool bNear=Away(At)<500;
   for(const FPart& P:S.YardParts)Show(P.C.Get(),bNear);
   if(S.YardGlow)S47StreetLamps::Shine(S.YardGlow,bNear&&K>0?Colour*float(6.*K):FLinearColor::Black);
   if(auto* D=S.YardLensBox.Get())
   {
    D->SetScalarParameterValue(TEXT("EmissivePower"),float(2.4*K));D->SetVectorParameterValue(TEXT("Emissive"),K>0?Colour:FLinearColor::Black);
   }
   if(auto* L=S.YardLight.Get())
   {
    const bool bLit=bNear&&K>.001;
    L->SetIntensity(bLit?float(CvYard.GetValueOnGameThread()*K):0.f);L->SetLightColor(Colour);
    // Shadows only close enough to see them: the truck, the cattle and one's own in the yard. It never moves, so they keep.
    const bool bShadow=bLit&&Away(At)<80;if(L->CastShadows!=bShadow)L->SetCastShadows(bShadow);
    if(L->IsVisible()!=bLit)L->SetVisibility(bLit);
   }
  }

  // ---- the old house ----
  {
   const FVector Lamp=S47FarmWorld::RoadToArea(Area,HouseLamp());
   const FVector Door=S47FarmWorld::RoadToArea(Area,S47FarmHouse::Point(6,10));
   S.HouseNear=Near(Lamp);
   // The night's "light in an empty house" when it is this one: on, until one comes up to it (S47Happenings, 18 m).
   bool bEvent=false;
   if(M->Freeform->Happenings)
   {
    FVector Site;
    if(M->Freeform->Happenings->SiteHere(TEXT("houselit"),Site)&&FVector2D::Distance(FVector2D(Site.X,Site.Z),FVector2D(Door.X,Door.Z))<30)
    {
     const int32 Night=FS47FreeformSession::NightOf(Clock);
     if(FVector2D::Distance(FVector2D(Site.X,Site.Z),FVector2D(Eye.X,Eye.Z))<18)S.EventOutNight=Night;
     bEvent=S.EventOutNight!=Night;
    }
   }
   const bool bTimer=HouseTimerOn(Hour);
   S.HouseWhy=bEvent?TEXT("the night's light in an empty house"):bTimer?TEXT("the timer, dusk to 22:30"):TEXT("off");
   double K=bTimer||bEvent?Flicker(S.HouseNear,S.T+17.):0.;
   if(S.HouseNear>.75)K=0; // the bulb goes with something that close
   S.HouseK=K;
   const double Far=Away(Lamp);
   Show(S.Bowl.Get(),Far<200);
   if(auto* D=S.BowlMat.Get())
   {
    D->SetScalarParameterValue(TEXT("EmissivePower"),float(2.2*K));D->SetVectorParameterValue(TEXT("Emissive"),K>0?FLinearColor(1.f,.78f,.5f):FLinearColor::Black);
   }
   if(auto* L=S.HouseLight.Get())
   {
    // Inside walls a light needs its shadows, or it leaks out through them: it is on only within sight of the window's light.
    const bool bLit=Far<160&&K>.001;
    L->SetIntensity(bLit?float(CvHouse.GetValueOnGameThread()*K):0.f);
    if(L->IsVisible()!=bLit)L->SetVisibility(bLit);
   }
   // The exported glow at the window: from afar only, where the room's own light is too small to read; near it was a ball of light.
   FindGlows(S,W,Dt);
   const double ToWindow=Away(S47FarmWorld::RoadToArea(Area,S47FarmBuildings::EastWindow()));
   SetGlows(S,W,float(K*FMath::SmoothStep(50.,110.,ToWindow)));
  }
 }

 void DevKick(US47Main* M,double VA,double VB)
 {
  if(FState* S=StateOf(M)){S->Swing.VA+=VA;S->Swing.VB+=VB;}
 }

 FString DevState(US47Main* M)
 {
  const FState* S=StateOf(M);
  if(!S||!S->bBuilt)return TEXT("farm lights: not built (Freeform, at the farm, with its buildings)");
  return FString::Printf(TEXT("farm lights: barn %.2f (swing %.1f deg, near %.2f), yard %.2f (near %.2f), house %.2f: %s (near %.2f), window glows %d"),
   S->BarnK,FMath::RadiansToDegrees(FMath::Sqrt(S->Swing.A*S->Swing.A+S->Swing.B*S->Swing.B)),S->BarnNear,S->YardK,S->YardNear,S->HouseK,*S->HouseWhy,S->HouseNear,S->Glows.Num());
 }

 bool SelfTest(TArray<FString>& Lines)
 {
  // The bulb: in the worst the barn gets (a hard wind, a gust, something at the door, knocked every half minute) it stays inside the
  // stop without needing it; still air keeps it going a little; a knock dies away.
  bool bSwing=true;double Peak=0,Rms=0;int32 Clamps=0,N=0;
  {
   FSwing Sw;const double Dt=1/30.,Worst=DriveStrength(1,1,1);
   for(double T=0,Kick=0;T<1200;T+=Dt)
   {
    if(T>=Kick){Sw.VA+=.12*.6;Sw.VB+=.12*.8;Kick+=30;}
    const FVector2D D=Drive(T,Worst,.6,-.8);Clamps+=StepSwing(Sw,Dt,D.X,D.Y);
    Peak=FMath::Max(Peak,FMath::Sqrt(Sw.A*Sw.A+Sw.B*Sw.B));
   }
   bSwing&=Clamps==0&&Peak<=MaxSwing&&Peak>.1;
  }
  {
   FSwing Sw;const double Dt=1/30.,Calm=DriveStrength(0,0,0);
   for(double T=0;T<1200;T+=Dt){const FVector2D D=Drive(T,Calm,.6,-.8);StepSwing(Sw,Dt,D.X,D.Y);Rms+=Sw.A*Sw.A+Sw.B*Sw.B;++N;}
   Rms=FMath::Sqrt(Rms/FMath::Max(1,N));
   bSwing&=Rms>.008&&Rms<.05;
  }
  {
   FSwing Sw;Sw.VA=.2;for(double T=0;T<12;T+=1/30.)StepSwing(Sw,1/30.,0,0);
   bSwing&=FMath::Abs(Sw.A)<.01&&FMath::Abs(Sw.VA)<.03;
  }
  Lines.Add(FString::Printf(TEXT("%s farm lights: the barn's bulb swings inside %.0f degrees in the worst (peak %.1f, %d stops), %.1f degrees in still air, a knock dies away"),
   bSwing?TEXT("PASS"):TEXT("FAIL"),FMath::RadiansToDegrees(MaxSwing),FMath::RadiansToDegrees(Peak),Clamps,FMath::RadiansToDegrees(Rms)));
  // Off by day, on by night; the house on its timer, dark in the small hours.
  const bool bHours=!BarnOn(12)&&!BarnOn(7)&&BarnOn(21)&&BarnOn(3)&&!YardOn(12)&&!YardOn(6)&&YardOn(19)&&YardOn(2)
   &&!HouseTimerOn(12)&&!HouseTimerOn(17)&&HouseTimerOn(20)&&!HouseTimerOn(23)&&!HouseTimerOn(1.5);
  Lines.Add(FString(bHours?TEXT("PASS"):TEXT("FAIL"))+TEXT(" farm lights: off by day, the barn and the yard on dusk to dawn, the house's timer dusk to 22:30"));
  // A lamp nothing is near does not flicker; one with something on it does, and never past full or under dark.
  bool bFlicker=true;double Mean=0;
  for(int32 I=0;I<400;++I){const double T=I*.137;bFlicker&=Flicker(0,T)==1&&Flicker(.1,T)==1;const double K=Flicker(1,T);bFlicker&=K>=0&&K<=1;Mean+=K;}
  bFlicker&=Mean/400<.85;
  Lines.Add(FString(bFlicker?TEXT("PASS"):TEXT("FAIL"))+TEXT(" farm lights: steady with nothing near, flickering and dimmed with something on them"));
  // Each light where it belongs: the bulb under its tie and over the aisle at its widest swing, clear of the posts and over head
  // height; the yard lamp out from the gable, outside the barn's walls; the house's under its ceiling, inside its walls.
  bool bBounds=true;
  if(S47FarmBuildings::Available()) // (without the barn's model there is nothing to hang them in, and none are built)
  {
   const FVector P=BarnPivot();double Floor=0;const bool bFloor=S47FarmBuildings::FloorAt(P.X,P.Z,Floor);
   bBounds&=bFloor&&S47FarmBuildings::Plots()[0].IsInside(FVector2D(P.X,P.Z))&&P.Y-Floor>3.4&&P.Y-Floor<3.8;
   for(int32 I=0;I<8;++I)
   {
    const double A=2*UE_DOUBLE_PI*I/8,SA=FMath::Sin(MaxSwing*FMath::Cos(A)),SB=FMath::Sin(MaxSwing*FMath::Sin(A));
    const FVector Down(SA,-FMath::Sqrt(1-SA*SA-SB*SB),SB),Bulb=P+Down*(CordLength+BulbMid+.05);
    bBounds&=S47FarmBuildings::Plots()[0].IsInside(FVector2D(Bulb.X,Bulb.Z))&&!S47FarmBuildings::Blocked(Bulb.X,Bulb.Z,.1)&&Bulb.Y-Floor>1.95;
   }
   const FVector Mount=YardMount(),Emit=Mount+YardOut()*(ArmReach+.42);
   bBounds&=FVector2D::Distance(FVector2D(Mount.X,Mount.Z),S47FarmBuildings::Plots()[0].GetCenter())<12&&!S47FarmBuildings::Blocked(Emit.X,Emit.Z,.1)
    &&YardOut().Z>.9&&Mount.Y-S47FarmWorld::FloorRoad(Emit.X,Emit.Z)>3.;
   const FVector H=HouseLamp();
   bBounds&=S47FarmHouse::Inside(H.X,H.Z)&&H.Y-S47FarmHouse::PadY()>2.2&&H.Y-S47FarmHouse::PadY()<2.5;
  }
  Lines.Add(FString(bBounds?TEXT("PASS"):TEXT("FAIL"))+TEXT(" farm lights: the bulb under its tie and clear of the posts at its widest, the yard lamp out from the gable, the house's lamp under its ceiling"));
  return bSwing&&bHours&&bFlicker&&bBounds;
 }
}
