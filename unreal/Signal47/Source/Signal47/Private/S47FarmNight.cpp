#include "S47FarmNight.h"
#include "S47Audio.h"
#include "S47FarmHouse.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "Components/SpotLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/UObjectIterator.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
 const TCHAR* LoopKey=TEXT("farm-wind");
 constexpr double FarmX=84,FarmZ=492,FarmReach=190; // Middle of the farm in the saro frame, and how far its sounds carry.
 FVector Size(double X,double Y,double Z){const FVector V=S47Three::DirToUE(FVector(X,Y,Z));return FVector(FMath::Abs(V.X),FMath::Abs(V.Y),FMath::Abs(V.Z));}
 // The substation's own frame (Tools/substation): identity panel at (0,-7.28,3.25), facing the road side.
 FVector Substation(double X,double Y,double Z){return FVector(7890,-1.10,536.20)+FVector(-X,Z,Y);}
}

FVector FS47FarmNight::EntryLampRoad(){return FVector(15,-1,190);}
FVector FS47FarmNight::TrafoLampWorld(){return FVector(7990,-1,512);}
FVector FS47FarmNight::SignWorld(){return Substation(0,-7.28,3.25);}
FVector FS47FarmNight::SignLampWorld(){return Substation(0,-8.5,4.7);}

bool FS47FarmNight::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web))return false;
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Cube||!Base)return false;
 auto Part=[&](FLamp& L,const FString& Name,const FVector& Offset,const FVector& Scale,const FLinearColor& Color)
 {
  auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web);Mat->SetVectorParameterValue(TEXT("Color"),Color);Mat->SetScalarParameterValue(TEXT("Roughness"),.6);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  auto* C=NewObject<UStaticMeshComponent>(M->Web,*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(Cube);C->SetMaterial(0,Mat);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->SetWorldScale3D(Scale);C->RegisterComponent();
  L.Parts.Add(C);L.Offsets.Add(Offset);return Mat;
 };
 auto Beam=[&](FLamp& L,const FString& Name,float Cone,float Reach)
 {
  auto* S=NewObject<USpotLightComponent>(M->Web,*Name);S->SetupAttachment(M->Web->GetRootComponent());S->SetMobility(EComponentMobility::Movable);
  S->SetIntensityUnits(ELightUnits::Candelas);S->SetIntensity(0);S->SetInnerConeAngle(Cone*.6f);S->SetOuterConeAngle(Cone);S->SetAttenuationRadius(Reach*100);S->SetCastShadows(false);S->SetVisibility(false);S->RegisterComponent();
  L.Light=S;
 };
 const FLinearColor Steel(.09,.09,.085);
 // The roadside poles are one instanced model. New lamps use the pole standing at the turn-off, and its height.
 UStaticMesh* PoleMesh=nullptr;FVector PoleScale=FVector::OneVector;double PoleTop=7.6;
 for(TObjectIterator<UInstancedStaticMeshComponent> It;It;++It)
 {
  auto* C=*It;if(C->GetWorld()!=M->Web->GetWorld()||!C->GetStaticMesh()||!C->GetStaticMesh()->GetName().Contains(TEXT("telephone_poles_Small_Pole")))continue;
  FTransform T;if(C->GetInstanceCount()>0&&C->GetInstanceTransform(0,T,true))
  {
   PoleMesh=C->GetStaticMesh();PoleScale=T.GetScale3D();
   const FBox Box=PoleMesh->GetBoundingBox();PoleTop=FMath::Clamp(Box.Max.Z*PoleScale.Z/100.,5.,12.)-.8;
   break;
  }
 }
 UE_LOG(LogTemp,Display,TEXT("S47: farm lamps use pole mesh %s, head at %.1f m"),*GetNameSafe(PoleMesh),PoleTop);
 auto LampOn=[&](FLamp& L,const FString& Tag,const FVector& Foot,const FVector& Toward,float Candela)
 {
  // An arm from the pole's top toward the road, its sodium head at the end.
  const FVector Dir=(Toward-Foot).GetSafeNormal2D();const FVector Top=Foot+FVector(0,PoleTop,0);
  const FVector ArmAt=Top+FVector(Dir.X,0,Dir.Z)*1.2,HeadAt=Top+FVector(Dir.X,-.1,Dir.Z)*2.4;
  const bool bAlongX=FMath::Abs(Dir.X)>FMath::Abs(Dir.Z);
  Part(L,TEXT("S47")+Tag+TEXT("Arm"),ArmAt,bAlongX?Size(2.5,.09,.09):Size(.09,.09,2.5),Steel);
  L.Head=Part(L,TEXT("S47")+Tag+TEXT("Head"),HeadAt,bAlongX?Size(.72,.13,.34):Size(.34,.13,.72),Steel);
  L.At=HeadAt+FVector(0,-.1,0);L.Aim=FVector(Toward.X,Foot.Y+.5,Toward.Z);L.Candela=Candela;
  Beam(L,TEXT("S47")+Tag+TEXT("Light"),66,36);
 };
 { // On the pole already standing by the farm's turn-off, reaching over the entrance.
  FLamp L;L.bRoadLocal=true;
  LampOn(L,TEXT("FarmLamp"),EntryLampRoad(),FVector(15,0,S47RoadTerrain::RanchZ(15)),1100);Lamps.Add(L);
 }
 { // Where the service track to the substation leaves the highway: no pole stands on that side, so one of the same kind does.
  FLamp L;
  LampOn(L,TEXT("TrafoLamp"),TrafoLampWorld(),FVector(7997,0,520),1100);
  if(PoleMesh)
  {
   auto* C=NewObject<UStaticMeshComponent>(M->Web,TEXT("S47TrafoLampPole"));C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(PoleMesh);
   C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetWorldScale3D(PoleScale);C->RegisterComponent();
   L.Parts.Add(C);L.Offsets.Add(TrafoLampWorld());
  }
  Lamps.Add(L);
 }
 { // A gooseneck lamp over the substation's identity panel, so the name reads from the road.
  FLamp L;L.At=SignLampWorld();L.Aim=SignWorld();L.Candela=38;
  Part(L,TEXT("S47SignLampArm"),Substation(0,-7.85,4.45),Size(.05,.05,1.3),Steel);
  L.Head=Part(L,TEXT("S47SignLampHead"),Substation(0,-8.5,4.45),Size(.42,.09,.26),Steel);
  Beam(L,TEXT("S47SignLampLight"),58,12);Lamps.Add(L);
 }
 bBuilt=true;return true;
}

void FS47FarmNight::Place(const FString& Area)
{
 const bool bRoadSide=Area==TEXT("road")||Area==TEXT("diner");
 for(FLamp& L:Lamps)
 {
  const bool bShown=L.bRoadLocal?(bRoadSide||Area==TEXT("saro")):bRoadSide;
  auto To=[&](const FVector& P){return L.bRoadLocal?S47FarmWorld::RoadToArea(Area,P):P;};
  for(int32 I=0;I<L.Parts.Num();++I)if(auto* C=L.Parts[I].Get())
  {
   if(bShown)C->SetWorldLocation(S47Three::ToUE(To(L.Offsets[I])));
   C->SetVisibility(bShown);C->SetHiddenInGame(!bShown);
  }
  if(auto* S=L.Light.Get())if(bShown)S->SetWorldLocationAndRotation(S47Three::ToUE(To(L.At)),S47Three::DirToUE(L.Aim-L.At).Rotation());
 }
}

void FS47FarmNight::SetOn(bool bNow,const FString& Area)
{
 bOn=bNow;const bool bRoadSide=Area==TEXT("road")||Area==TEXT("diner");
 for(FLamp& L:Lamps)
 {
  const bool bShown=L.bRoadLocal?(bRoadSide||Area==TEXT("saro")):bRoadSide;
  if(auto* S=L.Light.Get()){S->SetIntensity(bNow&&bShown?L.Candela:0);S->SetVisibility(bNow&&bShown);}
  if(auto* Mat=L.Head.Get()){Mat->SetVectorParameterValue(TEXT("Emissive"),bNow?FLinearColor(1,.68,.34)*40.f:FLinearColor::Black);Mat->SetScalarParameterValue(TEXT("EmissivePower"),bNow?1.f:0.f);}
 }
}

void FS47FarmNight::Stop()
{
 if(bWind&&M&&M->Audio)M->Audio->Stop(LoopKey,.5f);bWind=false;
 for(FLamp& L:Lamps)
 {
  for(const auto& C:L.Parts)if(C.IsValid()){C->SetVisibility(false);C->SetHiddenInGame(true);}
  if(auto* S=L.Light.Get()){S->SetIntensity(0);S->SetVisibility(false);}
 }
 LastArea.Empty();bOn=false;
}

void FS47FarmNight::Step(double Dt)
{
 if(!M||!IsValid(M->Web)||!M->Freeform)return;
 if(Scene.Get()!=M->Web){Scene=M->Web;bBuilt=false;Lamps.Reset();LastArea.Empty();}
 if(!Build())return;
 const FString Area=M->Web->Area();
 const double Hour=FMath::Fmod(M->Freeform->Clock(),86400.)/3600.;const bool bNight=Hour>=18.75||Hour<6.25;
 const bool bAreaChanged=Area!=LastArea;
 if(bAreaChanged){Place(Area);LastArea=Area;}
 if(bAreaChanged||bNight!=bOn)SetOn(bNight,Area);
 StepSound(Dt,bNight,Area);
}

void FS47FarmNight::StepSound(double Dt,bool bNight,const FString& Area)
{
 if(!M->Audio||!M->Player)return;
 const double Dist=Area==TEXT("saro")?FVector2D(M->Player->Pos.X-FarmX,M->Player->Pos.Z-FarmZ).Size():1e9;
 const bool bAt=Dist<FarmReach;
 if(bAt!=bWind)
 {
  bWind=bAt;
  if(bAt){FS47Sound S;S.Gain=.34f;S.bLoop=true;M->Audio->LoopSfx(LoopKey,TEXT("wind"),S);UE_LOG(LogTemp,Display,TEXT("S47: farm sounds on, %.0f m out"),Dist);}
  else M->Audio->Stop(LoopKey,1.5f);
 }
 if(!bAt)return;
 SoundIn-=Dt;if(SoundIn>0)return;
 SoundIn=FMath::FRandRange(bNight?7.f:11.f,bNight?19.f:26.f);
 // Each has a place, so it turns and fades as the player moves: never a sound that follows them.
 struct FNoise{const TCHAR* Name;double RoadX,RoadZ;float Gain;float RateLow,RateHigh;bool bNightOnly;};
 static const FNoise Noises[]={
  {TEXT("doorCreak0"),118,176,.8f,.55f,.8f,false}, // the loose sheet on the shed
  {TEXT("doorCreak1"),98,221,.8f,.6f,.85f,false}, // the old house, its door moving
  {TEXT("doorMetal"),150,200,.6f,.7f,.95f,false},  // something metal in the far shed
  {TEXT("gust"),60,240,.7f,.9f,1.1f,false},
  {TEXT("owl"),150,262,.85f,.92f,1.05f,true},
  {TEXT("dog0"),30,160,.6f,.85f,1.f,true},
  {TEXT("crickets"),70,225,.38f,.95f,1.05f,true}};
 TArray<const FNoise*> Pick;for(const auto& N:Noises)if(bNight||!N.bNightOnly)Pick.Add(&N);
 const FNoise& N=*Pick[FMath::RandRange(0,Pick.Num()-1)];
 const FVector At=S47FarmWorld::RoadToArea(TEXT("saro"),FVector(N.RoadX,S47FarmWorld::FloorRoad(N.RoadX,N.RoadZ)+1.5,N.RoadZ));
 const float Rate=FMath::FRandRange(N.RateLow,N.RateHigh);
 // Near noises are placed where they are; beyond the 60 m the panner carries, the far voice is what reaches the ear.
 const FVector Eye(M->Player->Pos.X,M->Player->FloorY+1.6,M->Player->Pos.Z);
 if(FVector::Dist(Eye,At)<45){FS47Sound S;S.Gain=N.Gain;S.Rate=Rate;S.At=At;M->Audio->Sfx(N.Name,S);}
 else M->Audio->Far(N.Name,At,N.Gain,Rate);
 ++Sounds;
}
