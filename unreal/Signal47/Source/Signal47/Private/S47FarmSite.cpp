#include "S47FarmSite.h"
#include "S47Audio.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
 // In the pasture, road-local so it rests on the same ground the player walks on.
 constexpr double CentreX=60,CentreZ=240,MarkRing=4.5;
 FVector Size(double X,double Y,double Z){const FVector V=S47Three::DirToUE(FVector(X,Y,Z));return FVector(FMath::Abs(V.X),FMath::Abs(V.Y),FMath::Abs(V.Z));}
 FVector OnGround(double RoadX,double RoadZ,double Lift){return S47FarmWorld::RoadToArea(TEXT("saro"),FVector(RoadX,S47FarmWorld::FloorRoad(RoadX,RoadZ)+Lift,RoadZ));}
}

FVector FS47FarmSite::Centre() const{return OnGround(CentreX,CentreZ,0);}

bool FS47FarmSite::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web))return false;
 const FS47Obj Room=M->Web->Obj(TEXT("room.group"));if(!Room)return false;
 const auto* Node=M->Web->Node(Room.I);if(!Node||!Node->Comp.IsValid())return false;
 auto* Disc=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Disc||!Base)return false;
 auto Piece=[&](const FString& Name,const FVector& At,const FVector& Scale,double Turn)
 {
  auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web);Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.035,.028,.02));Mat->SetScalarParameterValue(TEXT("Roughness"),.95);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  auto* C=NewObject<UStaticMeshComponent>(M->Web,*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(Node->Comp.Get());C->SetStaticMesh(Disc);C->SetMaterial(0,Mat);
  C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldRotation(FRotator(0,Turn,0));C->SetWorldScale3D(Scale);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();
  Marks.Add(C);return C;
 };
 // A ring of pressed, darkened grass: regular enough to look made, not tidy enough to explain.
 for(int32 I=0;I<9;++I)
 {
  const double A=2*PI*I/9,X=CentreX+FMath::Cos(A)*MarkRing,Z=CentreZ+FMath::Sin(A)*MarkRing;
  Piece(FString::Printf(TEXT("S47FarmMark%d"),I),OnGround(X,Z,.015),Size(1.5,.02,.8),-FMath::RadiansToDegrees(A));
 }
 auto* Middle=Piece(TEXT("S47FarmMarkMiddle"),OnGround(CentreX,CentreZ,.05),Size(1.6,.06,1.2),18);
 Middle->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Middle->SetCollisionResponseToAllChannels(ECR_Ignore);Middle->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Middle->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
 Spot=M->Web->ObjAt(M->Web->AdoptNode(Middle,Room.I,TEXT("ff:farmsite")));
 auto* Fog=NewObject<ULocalFogVolumeComponent>(M->Web,TEXT("S47FarmHaze"));Fog->SetupAttachment(Node->Comp.Get());Fog->RegisterComponent();
 Fog->SetWorldLocation(S47Three::ToUE(OnGround(CentreX,CentreZ,1)));Fog->SetWorldScale3D(FVector(12,12,1)); // Unit radius is 5 m: low, sixty metres across the pasture.
 Fog->SetRadialFogExtinction(.06f);Fog->SetHeightFogExtinction(.2f);Fog->SetHeightFogFalloff(400.f);Fog->SetFogAlbedo(FLinearColor(.5f,.55f,.62f));Fog->SetFogEmissive(FLinearColor(.012f,.014f,.018f)); // Readable at night without any lamp on it.
 Haze=Fog;
 bBuilt=true;return true;
}

void FS47FarmSite::Show(bool bShow)
{
 bShown=bShow;
 for(const auto& C:Marks)if(C.IsValid()){C->SetVisibility(bShow);C->SetHiddenInGame(!bShow);}
 if(Marks.Num())if(auto* Middle=Marks.Last().Get())Middle->SetCollisionEnabled(bShow?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 if(auto* F=Haze.Get()){F->SetVisibility(bShow);F->SetHiddenInGame(!bShow);}
}

void FS47FarmSite::Begin(const S47J::FObj& Saved)
{
 Stop();if(!M||!IsValid(M->Web))return;
 if(Scene.Get()!=M->Web){bBuilt=false;bBound=false;Marks.Reset();Haze.Reset();Spot={};}
 Scene=M->Web;Life=MakeShared<uint8>(0);bActive=true;bBound=false;
 ClearAt=Saved.IsValid()?S47J::Num(Saved,TEXT("clearAt")):0;SoundIn=20;
 if(!Build())return;
 const TWeakPtr<uint8> Guard=Life;
 if(IsValid(M->Inter))
 {
  FS47Interactable I;I.Id=TEXT("ff:farmsite");I.Object=Spot;I.Range=3.2;
  I.Label=[this,Guard]{return Guard.IsValid()?Label():FString();};
  I.Use=[this,Guard]{if(Guard.IsValid())Use();};
  I.Examine=I.Label;
  M->Inter->Add(I);M->Web->MarkUsable(Spot.I,TEXT("ff:farmsite"));bBound=true;
 }
 Show(Armed());
}

void FS47FarmSite::Stop(){bActive=false;Life.Reset();Show(false);}

S47J::FObj FS47FarmSite::Save() const{const auto O=S47J::New();if(ClearAt>0)S47J::Set(O,TEXT("clearAt"),ClearAt);return O;}

void FS47FarmSite::Arm(double Clock)
{
 // Gone by the next 06:00: the morning is a separate, later look at the place.
 static constexpr double Day=86400,Morning=6*3600;
 ClearAt=FMath::FloorToDouble((Clock-Morning)/Day+1)*Day+Morning;
 SoundIn=15;Show(true);
 UE_LOG(LogTemp,Display,TEXT("S47: farm site armed until clock %.0f"),ClearAt);
}

void FS47FarmSite::Step(double Dt)
{
 if(!bActive||!M||!M->Freeform)return;
 if(ClearAt>0&&M->Freeform->Clock()>=ClearAt){ClearAt=0;Show(false);UE_LOG(LogTemp,Display,TEXT("S47: farm site cleared by morning"));return;}
 if(!Armed()||!M->Audio||!M->Player||!M->Web||M->Web->Area()!=TEXT("saro"))return;
 const FVector C=Centre();const double Dist=FVector2D(M->Player->Pos.X-C.X,M->Player->Pos.Z-C.Z).Size();
 if(Dist>140)return;
 SoundIn-=Dt;if(SoundIn>0)return;
 SoundIn=FMath::FRandRange(22.f,48.f);
 // From the place itself: it gets louder and turns with the listener, it never follows them.
 const float Side=FMath::FRandRange(-6.f,6.f);
 M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.3f,.At=C+FVector(Side,1,-Side)});
}

FString FS47FarmSite::Label() const
{
 if(!bActive||!Armed())return FString();
 return M&&M->Freeform&&M->Freeform->HasFinding(FindingId())?TEXT("Pressed ground / already noted"):TEXT("Pressed ground / look closer");
}

bool FS47FarmSite::Use()
{
 if(!bActive||!Armed()||!M||!M->UI||!M->Freeform)return false;
 const bool bNew=!M->Freeform->HasFinding(FindingId());
 if(bNew)M->Freeform->RecordEvidence(FindingId(),TEXT("Pasture at the Alderete place, the night of the lights. Nine pressed marks in a ring about nine metres across, the grass darkened and laid flat. Cattle, a vehicle or a prank have not been ruled out. Worth a second look by daylight."));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("ALDERETE PASTURE"),S47Kit::Col({
  S47Kit::Text(TEXT("Nine marks in a ring, about nine metres across. The grass is laid flat and darkened, and it is dry here when the ground beside it is not.")),
  S47Kit::Small(bNew?TEXT("Kept as a field record. It says where and what, not who or why."):TEXT("Already in your field records."))}),760));
 if(bNew)M->RequestAutosave();
 return true;
}
