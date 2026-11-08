#include "S47Lived.h"

#include "S47FarmHouse.h"
#include "S47Main.h"
#include "S47SaroNight.h"
#include "S47SaroPower.h"
#include "S47Sky.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47WorldLife.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/WeakObjectPtrTemplates.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Lived,Log,All);

namespace
{
 UStaticMesh* OptionalMesh(const TCHAR* Package)
 {
  if(!FPackageName::DoesPackageExist(Package))return nullptr;
  return LoadObject<UStaticMesh>(nullptr,*(FString(Package)+TEXT(".")+FPackageName::GetShortName(Package)),nullptr,LOAD_NoWarn|LOAD_Quiet);
 }
 // Small warm things in the dark, in the game's three.js metres. Every one has a thing in the room it comes from. A tube is 36 lumens here.
 struct FLivedLamp{const TCHAR* Name;FVector At;float Lumens,Radius;FLinearColor Colour;bool bDown,bShadow;};
 const FLivedLamp Lamps[]={
  {TEXT("LivedConsoleGooseneck"),FVector(-1.125,1.07,-3.94),9.f,340.f,FLinearColor(1.f,.60f,.28f),true,true},
  {TEXT("LivedStatusWall"),FVector(-4.35,1.75,4.16),.7f,240.f,FLinearColor(.72f,1.f,.55f),false,false},
  {TEXT("LivedHotplate"),FVector(4.38,1.02,4.02),1.1f,190.f,FLinearColor(1.f,.42f,.14f),false,false},
  {TEXT("LivedOldCabinet"),FVector(-5.28,1.84,-4.05),.5f,190.f,FLinearColor(.75f,1.f,.6f),false,false},
  // What the screens and the racks give off at night: faint, so the room reads and stays dark.
  {TEXT("LivedScreenSpillLeft"),FVector(-2.15,1.05,-3.55),.45f,230.f,FLinearColor(.55f,1.f,.7f),false,false},
  {TEXT("LivedScreenSpillMiddle"),FVector(0,1.05,-3.55),.45f,230.f,FLinearColor(.55f,1.f,.7f),false,false},
  {TEXT("LivedScreenSpillRight"),FVector(2.15,1.05,-3.55),.45f,230.f,FLinearColor(.55f,1.f,.7f),false,false},
  {TEXT("LivedRackGlow"),FVector(5.05,1.35,-2.4),.35f,200.f,FLinearColor(1.f,.55f,.45f),false,false},
  {TEXT("LivedConsoleEndGlow"),FVector(3.2,1.0,-4.0),.25f,160.f,FLinearColor(.6f,1.f,.6f),false,false},
 };
 // What the walker cannot go through: only things that stand out in the room.
 struct FLivedSolid{const TCHAR* Id;double X0,X1,Z0,Z1;};
 const FLivedSolid Solids[]={
  {TEXT("lived:oldCabinet"),-6,-5.38,-4.46,-3.64},
  {TEXT("lived:tapeShelf"),-6,-5.48,-2.42,-1.54},
  {TEXT("lived:boxes"),3.52,4.62,-4.5,-3.96},
 };
 struct FState
 {
  TWeakObjectPtr<AS47Web> Web;
  int32 FloorNode=-1,DressNode=-1,OutNode=-1;
  TWeakObjectPtr<UMaterialInstanceDynamic> Green,Amber,Red,Bulb;
  FLinearColor GreenOn=FLinearColor::Black,AmberOn=FLinearColor::Black,RedOn=FLinearColor::Black,BulbOn=FLinearColor::Black;
  TArray<TWeakObjectPtr<ULocalLightComponent>> Lights;
  TArray<TWeakObjectPtr<URectLightComponent>> HouseDay;
  TArray<TWeakObjectPtr<UPointLightComponent>> HouseFill;
  bool bColliders=false,bMissingSaid=false;
  double T=0,Retry=0,RedNext=0;float RedLevel=1;
 };
 UMaterialInstanceDynamic* LitMid(UStaticMeshComponent* C,const TCHAR* Slot,FLinearColor& On)
 {
  const int32 I=C?C->GetMaterialIndex(Slot):INDEX_NONE;
  if(I==INDEX_NONE)return nullptr;
  UMaterialInstanceDynamic* D=C->CreateDynamicMaterialInstance(I);
  if(D)D->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveFactor")),On);
  return D;
 }
 int32 PlaceRoomMesh(AS47Web* W,int32 Parent,UStaticMesh* Asset,const TCHAR* Name,bool bShadow,UStaticMeshComponent*& Out)
 {
  const FS47WebNode* N=W->Node(Parent);
  if(!N||!N->Comp.IsValid()||!Asset)return -1;
  UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(W,Name);
  C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(N->Comp.Get());C->SetStaticMesh(Asset);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetCastShadow(bShadow);
  W->AddInstanceComponent(C);C->RegisterComponent();C->SetWorldTransform(FTransform::Identity);
  Out=C;
  return W->AdoptNode(C,Parent,Name);
 }
 void BuildControl(FState& S,AS47Web* W)
 {
  const FS47Obj Room=W->Obj(TEXT("room.group"));
  if(!Room)return;
  if(S.FloorNode<0)if(UStaticMesh* Floor=OptionalMesh(TEXT("/Game/Lived/Control/SM_Lived_ControlFloor")))
  {
   UStaticMeshComponent* C=nullptr;
   S.FloorNode=PlaceRoomMesh(W,Room.I,Floor,TEXT("LivedControlFloor"),false,C);
  }
  if(S.DressNode<0)if(UStaticMesh* Dress=OptionalMesh(TEXT("/Game/Lived/Control/SM_Lived_ControlDressing")))
  {
   UStaticMeshComponent* C=nullptr;
   S.DressNode=PlaceRoomMesh(W,Room.I,Dress,TEXT("LivedControlDressing"),true,C);
   if(S.DressNode>=0)
   {
    S.Green=LitMid(C,TEXT("M_Lived_LampGreen"),S.GreenOn);S.Amber=LitMid(C,TEXT("M_Lived_LampAmber"),S.AmberOn);
    S.Red=LitMid(C,TEXT("M_Lived_LampRed"),S.RedOn);S.Bulb=LitMid(C,TEXT("M_Lived_BulbWarm"),S.BulbOn);
   }
  }
  if(S.DressNode<0)
  {
   if(!S.bMissingSaid)UE_LOG(LogS47Lived,Display,TEXT("S47: lived-in control room: no assets under /Game/Lived/Control; the room as it was"));
   S.bMissingSaid=true;
   return;
  }
  if(S.Lights.Num()==0)
  {
   const FS47WebNode* N=W->Node(Room.I);
   for(const FLivedLamp& L:Lamps)
   {
    ULocalLightComponent* Made=nullptr;
    if(L.bDown)
    {
     USpotLightComponent* Spot=NewObject<USpotLightComponent>(W,L.Name);
     Spot->SetInnerConeAngle(38.f);Spot->SetOuterConeAngle(68.f);
     Spot->SetupAttachment(N->Comp.Get());Spot->SetWorldRotation(FRotator(-90,0,0));
     Made=Spot;
    }
    else
    {
     UPointLightComponent* Point=NewObject<UPointLightComponent>(W,L.Name);
     Point->SetupAttachment(N->Comp.Get());
     Made=Point;
    }
    Made->SetMobility(EComponentMobility::Movable);Made->SetWorldLocation(S47Three::ToUE(L.At));
    Made->SetIntensityUnits(ELightUnits::Lumens);Made->SetIntensity(0);Made->SetLightColor(L.Colour);Made->SetAttenuationRadius(L.Radius);
    Made->SetCastShadows(L.bShadow);Made->SetVolumetricScatteringIntensity(0);Made->SetVisibility(false);
    W->AddInstanceComponent(Made);Made->RegisterComponent();
    S.Lights.Add(Made);
   }
  }
  if(!S.bColliders)
  {
   S.bColliders=true;
   for(const FLivedSolid& D:Solids)W->CollidersMutable(TEXT("saro")).Add({D.X0,D.X1,D.Z0,D.Z1,FString(D.Id),true});
   UE_LOG(LogS47Lived,Display,TEXT("S47: lived-in control room up: floor %s, dressing %s, %d small lamps, %d solids"),S.FloorNode>=0?TEXT("yes"):TEXT("no"),S.DressNode>=0?TEXT("yes"):TEXT("no"),S.Lights.Num(),int32(UE_ARRAY_COUNT(Solids)));
  }
 }
 // The one farmhouse by day (T16-15): its small deep windows let in too little for Lumen alone, so the sky comes in through each, as the control room's
 // windows do (S47SaroRooms WindowSky). Plan metres (S47FarmHouse.cpp Walls): the opening's middle on the inner face, and which way is in.
 struct FHouseWindow{double PlanX,PlanY,Width;FVector In;};
 const FHouseWindow HouseWindows[]={
  {2.5,.3,1.8,FVector(0,0,-1)},{8.,.3,1.2,FVector(0,0,-1)},{10.9,.3,.8,FVector(0,0,-1)},
  {2.2,8.7,1.8,FVector(0,0,1)},{8.9,8.7,1.8,FVector(0,0,1)},
  {.3,6.45,1.3,FVector(-1,0,0)},{11.7,6.85,1.3,FVector(1,0,0)},
 };
 constexpr float HouseWindowLumens=34.f;
 // What a room's walls and floor give back by day, which Lumen alone misses in rooms this small and closed: one soft fill under each ceiling,
 // the hall's too (it has no window of its own). Plan metres. Keeps the room readable to sunset without lighting it like a shop.
 const FVector2D HouseRooms[]={{2.7,2.35},{2.7,6.6},{6.,2.6},{6.,6.6},{8.35,2.35},{10.8,2.35},{9.3,6.6}};
 constexpr float HouseFillLumens=5.f;
 void BuildHouseDay(FState& S,AS47Web* W)
 {
  if(S.HouseDay.Num()||!S47FarmHouse::HasModel())return;
  for(int32 I=0;I<UE_ARRAY_COUNT(HouseWindows);++I)
  {
   const FHouseWindow& H=HouseWindows[I];
   // Road-local to the SARO area, as S47FarmHouseDoors puts its leaves.
   const FVector At=S47FarmHouse::Point(H.PlanX,H.PlanY)+FVector(-24,1.5,300)+H.In*.06;
   auto* L=NewObject<URectLightComponent>(W,*FString::Printf(TEXT("LivedHouseDay%d"),I));
   L->SetupAttachment(W->GetRootComponent());L->SetMobility(EComponentMobility::Movable);
   L->SetWorldLocation(S47Three::ToUE(At));L->SetWorldRotation(S47Three::DirToUE(H.In).Rotation());
   L->SetSourceWidth(float(H.Width*100));L->SetSourceHeight(115.f);L->SetBarnDoorAngle(70.f);L->SetBarnDoorLength(25.f);
   L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(0);L->SetAttenuationRadius(900.f);L->SetCastShadows(true);L->SetVolumetricScatteringIntensity(0);
   L->SetVisibility(false);W->AddInstanceComponent(L);L->RegisterComponent();
   S.HouseDay.Add(L);
  }
  for(int32 I=0;I<UE_ARRAY_COUNT(HouseRooms);++I)
  {
   auto* F=NewObject<UPointLightComponent>(W,*FString::Printf(TEXT("LivedHouseFill%d"),I));
   F->SetupAttachment(W->GetRootComponent());F->SetMobility(EComponentMobility::Movable);
   F->SetWorldLocation(S47Three::ToUE(S47FarmHouse::Point(HouseRooms[I].X,HouseRooms[I].Y)+FVector(-24,2.25,300)));
   F->SetIntensityUnits(ELightUnits::Lumens);F->SetIntensity(0);F->SetAttenuationRadius(430.f);F->SetSourceRadius(60.f);F->SetSoftSourceRadius(120.f);
   F->SetCastShadows(false);F->SetVolumetricScatteringIntensity(0);F->SetVisibility(false);W->AddInstanceComponent(F);F->RegisterComponent();
   S.HouseFill.Add(F);
  }
 }
 void ShowHouseDay(FState& S,US47Main* M)
 {
  if(!S.HouseDay.Num()&&!S.HouseFill.Num())return;
  const bool bArea=M->Mode==US47Main::EMode::Play&&M->IsFreeform()&&M->World&&M->World->Area==TEXT("saro");
  // Only near the house: seven shadowed rect lights cost nothing when they are off.
  const FVector Middle=S47FarmHouse::Point(6,4.5)+FVector(-24,0,300);
  const bool bNear=bArea&&M->Player&&FVector2D(M->Player->Pos.X-Middle.X,M->Player->Pos.Z-Middle.Z).Size()<45;
  const FS47Sky* Sky=M->Life&&M->Life->Sky.IsValid()?M->Life->Sky.Get():nullptr;
  // Full from morning to the late afternoon, then down with the light outside: Daylight() leans on the low sun, so lift it until it really fades.
  const float Day=Sky?float(FMath::Clamp(FMath::Sqrt(FMath::Clamp(Sky->Daylight(),0.,1.))*1.15,0.,1.)):0.f;
  const FLinearColor Tint=FMath::Lerp(FLinearColor(.92f,.96f,1.f),FLinearColor(1.f,.84f,.66f),Sky?float(Sky->Evening):0.f);
  for(auto& P:S.HouseDay)if(auto* L=P.Get())
  {
   const bool bOn=bNear&&Day>.002f;
   L->SetIntensity(bOn?HouseWindowLumens*Day:0.f);L->SetLightColor(Tint);L->SetVisibility(bOn);
  }
  for(auto& P:S.HouseFill)if(auto* F=P.Get())
  {
   const bool bOn=bNear&&Day>.002f;
   F->SetIntensity(bOn?HouseFillLumens*Day:0.f);F->SetLightColor(Tint*FLinearColor(1.f,.93f,.82f));F->SetVisibility(bOn);
  }
 }
 // SARO from outside (T16-15): block joints, coping, downspouts, tray, cooling. Room-origin in the site's terms, nothing of the doors or signs under it.
 void BuildOutside(FState& S,AS47Web* W)
 {
  if(S.OutNode>=0)return;
  const int32 Ext=W->Handle(TEXT("ext.group"));
  if(Ext<0)return;
  if(UStaticMesh* Out=OptionalMesh(TEXT("/Game/Lived/Saro/SM_Lived_SaroExterior")))
  {
   UStaticMeshComponent* C=nullptr;
   S.OutNode=PlaceRoomMesh(W,Ext,Out,TEXT("LivedSaroExterior"),true,C);
  }
 }
 void Show(FState& S,US47Main* M,AS47Web* W,double Dt)
 {
  const FS47Obj Room=W->Obj(TEXT("room.group"));
  const bool bVisible=M->Mode==US47Main::EMode::Play&&M->World&&M->World->Area==TEXT("saro")&&Room&&Room.Shown();
  if(S.FloorNode>=0)W->ObjAt(S.FloorNode).SetVisible(bVisible);
  if(S.DressNode>=0)W->ObjAt(S.DressNode).SetVisible(bVisible);
  if(S.OutNode>=0)
  {
   const FS47Obj Ext=W->ObjAt(W->Handle(TEXT("ext.group")));
   W->ObjAt(S.OutNode).SetVisible(M->Mode==US47Main::EMode::Play&&M->World&&M->World->Area==TEXT("saro")&&Ext&&Ext.Shown());
  }
  // SARO's own power (S47SaroPower.h): all of this goes with it, and comes back when the inside does.
  const float Power=M->SaroNight&&M->SaroNight->Power?float(FMath::Clamp(M->SaroNight->Power->Inner,0.,1.)):1.f;
  S.T+=Dt;
  // Amber: on, with a long breath out now and then. Red: a drive's busy lamp, so never regular.
  const float Amber=FMath::Fmod(S.T,7.3)<.55?.15f:1.f;
  if(S.T>=S.RedNext){S.RedLevel=FMath::FRand()<.45f?1.f:.08f;S.RedNext=S.T+FMath::FRandRange(.07,.5);}
  if(auto* D=S.Green.Get())D->SetVectorParameterValue(TEXT("EmissiveFactor"),S.GreenOn*Power);
  if(auto* D=S.Amber.Get())D->SetVectorParameterValue(TEXT("EmissiveFactor"),S.AmberOn*Power*Amber);
  if(auto* D=S.Red.Get())D->SetVectorParameterValue(TEXT("EmissiveFactor"),S.RedOn*Power*S.RedLevel);
  if(auto* D=S.Bulb.Get())D->SetVectorParameterValue(TEXT("EmissiveFactor"),S.BulbOn*Power);
  for(int32 I=0;I<S.Lights.Num();++I)if(auto* L=S.Lights[I].Get())
  {
   const float Lm=Lamps[I].Lumens*Power;
   L->SetIntensity(bVisible?Lm:0.f);L->SetVisibility(bVisible&&Lm>.001f);
  }
 }
}

void S47Lived::Update(US47Main* Main,double Dt)
{
 static const bool bOff=FParse::Param(FCommandLine::Get(),TEXT("S47LivedOff"));
 if(bOff||!Main||!Main->Web||!Main->Web->IsLoaded())return;
 static TMap<TWeakObjectPtr<US47Main>,TSharedPtr<FState>> States;
 for(auto It=States.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
 TSharedPtr<FState>& Slot=States.FindOrAdd(TWeakObjectPtr<US47Main>(Main));
 if(!Slot)Slot=MakeShared<FState>();
 FState& S=*Slot;
 AS47Web* W=Main->Web.Get();
 if(S.Web.Get()!=W){S=FState();S.Web=W;}
 S.Retry-=FMath::Max(0.,Dt);
 if(S.Retry<=0)
 {
  if((S.DressNode<0||!S.bColliders)&&!S.bMissingSaid)BuildControl(S,W);
  BuildOutside(S,W);
  BuildHouseDay(S,W);
  S.Retry=1;
 }
 Show(S,Main,W,Dt);
 ShowHouseDay(S,Main);
}
