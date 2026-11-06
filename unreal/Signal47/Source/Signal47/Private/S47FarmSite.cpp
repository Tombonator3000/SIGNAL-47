#include "S47FarmSite.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"
#include "S47Story.h"
#include "S47FreeformDirector.h"
#include "S47World.h"
#include "S47Sky.h"
#include "S47WorldLife.h"
#include "S47Glimpses.h"
#include "S47Night.h"
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
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
 // In the pasture, road-local so it rests on the same ground the player walks on.
 constexpr double CentreX=60,CentreZ=240;
 constexpr int32 MarkCount=9;
 FVector Size(double X,double Y,double Z){const FVector V=S47Three::DirToUE(FVector(X,Y,Z));return FVector(FMath::Abs(V.X),FMath::Abs(V.Y),FMath::Abs(V.Z));}
 FVector FarmCentre(){return S47FarmWorld::RoadToArea(TEXT("saro"),FVector(CentreX,S47FarmWorld::FloorRoad(CentreX,CentreZ),CentreZ));}
}

bool FS47FarmSite::Here() const{return M&&M->Web&&(M->Web->Area()==AreaId||(AreaId==TEXT("road")&&M->Web->Area()==TEXT("diner"))||(AreaId==TEXT("diner")&&M->Web->Area()==TEXT("road")));}

double FS47FarmSite::GroundAt(double X,double Y,double Z) const
{
 FHitResult Hit;FCollisionQueryParams Query(TEXT("S47FarmSiteGround"),true);
 if(M&&M->Web&&M->Web->GetWorld()&&M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(FVector(X,Y+30,Z)),S47Three::ToUE(FVector(X,Y-30,Z)),ECC_Visibility,Query))return S47Three::ToThree(Hit.ImpactPoint).Y;
 return Y;
}

bool FS47FarmSite::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web))return false;
 const int32 Saro=M->Web->Handle(TEXT("ext.group")),Road=M->Web->Handle(TEXT("world.legs.road.area.group"));if(Saro<0||Road<0)return false;
 auto* Disc=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Disc||!Base)return false;
 auto Piece=[&](const FString& Name)
 {
  auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web);Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.035,.028,.02));Mat->SetScalarParameterValue(TEXT("Roughness"),.95);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  auto* C=NewObject<UStaticMeshComponent>(M->Web,*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(Disc);C->SetMaterial(0,Mat);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();
  return C;
 };
 for(int32 I=0;I<MarkCount;++I)Marks.Add(Piece(FString::Printf(TEXT("S47FarmMark%d"),I)));
 // One aim target per exterior root, as the parcel has: the player's ray finds whichever area is shown.
 auto Target=[&](const TCHAR* Name,int32 Parent,FS47Obj& Out)
 {
  auto* C=Piece(Name);C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
  Out=M->Web->ObjAt(M->Web->AdoptNode(C,Parent,Name));Spots.Add(C);
 };
 Target(TEXT("ff:farmsite"),Saro,SpotSaro);Target(TEXT("ff:farmsite:road"),Road,SpotRoad);
 auto* Fog=NewObject<ULocalFogVolumeComponent>(M->Web,TEXT("S47FarmHaze"));Fog->SetupAttachment(M->Web->GetRootComponent());Fog->RegisterComponent();
 Fog->SetWorldScale3D(FVector(12,12,1)); // Unit radius is 5 m: low, sixty metres across.
 Fog->SetRadialFogExtinction(.06f);Fog->SetHeightFogExtinction(.2f);Fog->SetHeightFogFalloff(400.f);Fog->SetFogAlbedo(FLinearColor(.5f,.55f,.62f));Fog->SetFogEmissive(FLinearColor(.012f,.014f,.018f)); // Readable at night without any lamp on it.
 Haze=Fog;
 bBuilt=true;return true;
}

void FS47FarmSite::Layout()
{
 if(!bBuilt)return;
 // A tighter ring in town, where the open ground is a street front rather than a pasture.
 const bool bFarm=Place==TEXT("farm");const double Ring=bFarm?4.5:2.4,K=bFarm?1.:.6;
 for(int32 I=0;I<Marks.Num();++I)if(auto* C=Marks[I].Get())
 {
  const double A=2*PI*I/MarkCount,X=Middle.X+FMath::Cos(A)*Ring,Z=Middle.Z+FMath::Sin(A)*Ring;
  C->SetWorldLocation(S47Three::ToUE(FVector(X,GroundAt(X,Middle.Y,Z)+.015,Z)));C->SetWorldRotation(FRotator(0,-FMath::RadiansToDegrees(A),0));C->SetWorldScale3D(Size(1.5*K,.02,.8*K));
 }
 for(const auto& S:Spots)if(auto* C=S.Get())
 {
  C->SetWorldLocation(S47Three::ToUE(FVector(Middle.X,GroundAt(Middle.X,Middle.Y,Middle.Z)+.05,Middle.Z)));C->SetWorldRotation(FRotator(0,18,0));C->SetWorldScale3D(Size(1.6*K,.06,1.2*K));
 }
 if(auto* F=Haze.Get())F->SetWorldLocation(S47Three::ToUE(Middle+FVector(0,1,0)));
}

void FS47FarmSite::Show(bool bShow)
{
 bShown=bShow;
 for(const auto& C:Marks)if(C.IsValid()){C->SetVisibility(bShow);C->SetHiddenInGame(!bShow);}
 for(const auto& C:Spots)if(C.IsValid()){C->SetVisibility(bShow);C->SetHiddenInGame(!bShow);C->SetCollisionEnabled(bShow?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}
 if(auto* F=Haze.Get()){F->SetVisibility(bShow);F->SetHiddenInGame(!bShow);}
}

void FS47FarmSite::Begin(const S47J::FObj& Saved)
{
 Stop();if(!M||!IsValid(M->Web))return;
 if(Scene.Get()!=M->Web){bBuilt=false;bBound=false;Marks.Reset();Spots.Reset();Haze.Reset();SpotSaro={};SpotRoad={};}
 Scene=M->Web;Life=MakeShared<uint8>(0);bActive=true;bBound=false;
 ClearAt=Saved.IsValid()?S47J::Num(Saved,TEXT("clearAt")):0;SoundIn=20;
 Visit=Saved.IsValid()&&Saved->HasField(TEXT("visit"))?S47J::Int(Saved,TEXT("visit")):4;VisitAge=VisitDwell=0;
 Place=Saved.IsValid()?S47J::Str(Saved,TEXT("place")):FString();if(Place.IsEmpty())Place=TEXT("farm");
 Title=Saved.IsValid()?S47J::Str(Saved,TEXT("title")):FString();if(Title.IsEmpty())Title=TEXT("the Alderete place");
 AreaId=Saved.IsValid()?S47J::Str(Saved,TEXT("area")):FString();if(AreaId.IsEmpty())AreaId=TEXT("saro");
 const TArray<double> At=Saved.IsValid()?S47J::Nums(Saved,TEXT("at")):TArray<double>();
 Middle=At.Num()==3?FVector(At[0],At[1],At[2]):FarmCentre();
 if(!Build())return;
 const TWeakPtr<uint8> Guard=Life;
 if(IsValid(M->Inter))
 {
  FS47Interactable I;I.Id=TEXT("ff:farmsite");I.Object=SpotSaro;I.Range=3.2;
  I.Label=[this,Guard]{return Guard.IsValid()&&Here()?Label():FString();};
  I.Use=[this,Guard]{if(Guard.IsValid())Use();};
  I.Examine=I.Label;
  M->Inter->Add(I);M->Web->MarkUsable(SpotSaro.I,TEXT("ff:farmsite"));M->Web->MarkUsable(SpotRoad.I,TEXT("ff:farmsite"));bBound=true;
 }
 Layout();Show(Armed());
}

void FS47FarmSite::Stop()
{
 // Not while the engine or the owner is going away: the scene the body hangs in may already be gone.
 const bool bLive=!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->Web.Get());
 if(Visit>=2&&Visit<=3&&bLive&&M->Night&&M->Night->Glimpses)M->Night->Glimpses->DevClear();
 Visit=4;bActive=false;Life.Reset();Show(false);
}

S47J::FObj FS47FarmSite::Save() const
{
 const auto O=S47J::New();
 if(ClearAt>0){S47J::Set(O,TEXT("clearAt"),ClearAt);S47J::Set(O,TEXT("visit"),Visit==0?0:4);S47J::Set(O,TEXT("place"),Place);S47J::Set(O,TEXT("title"),Title);S47J::Set(O,TEXT("area"),AreaId);S47J::Set(O,TEXT("at"),TArray<double>{Middle.X,Middle.Y,Middle.Z});}
 return O;
}

void FS47FarmSite::ArmFarm(double Clock){Arm(Clock,TEXT("farm"),TEXT("the Alderete place"),TEXT("saro"),FarmCentre());}

void FS47FarmSite::Arm(double Clock,const FString& InPlace,const FString& InTitle,const FString& InArea,const FVector& Point)
{
 // Gone by the next 06:00: the morning is a separate, later look at the place.
 static constexpr double Day=86400,Morning=6*3600;
 ClearAt=FMath::FloorToDouble((Clock-Morning)/Day+1)*Day+Morning;
 Place=InPlace;Title=InTitle;AreaId=InArea;Middle=Point;
 // Not every night's lights leave something behind that comes back for you.
 Visit=FMath::FRand()<.6f?0:4;VisitDwell=0;
 if(Place!=TEXT("farm")&&M&&M->Web&&M->Web->GetWorld())
 {
  // The longest clear way out from the front, and a few metres along it.
  double Best=-1;FVector Way=FVector::ForwardVector;
  for(int32 I=0;I<12;++I)
  {
   const double A=2*PI*I/12;const FVector Dir(FMath::Cos(A),0,FMath::Sin(A));const FVector From=Point+FVector(0,.3,0);FHitResult Hit;
   FCollisionQueryParams Query(TEXT("S47FarmSiteOpen"),true);
   const bool Blocked=M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(From+Dir*1.2),S47Three::ToUE(From+Dir*16),ECC_Visibility,Query);
   const double Free=Blocked?Hit.Distance/100.:14.8;
   if(Free>Best){Best=Free;Way=Dir;}
  }
  Middle=Point+Way*(1.2+FMath::Clamp(Best-3,0.,7.));
 }
 SoundIn=15;Layout();Show(true);
 UE_LOG(LogTemp,Display,TEXT("S47: lights site %s armed in %s at %.1f %.1f %.1f until clock %.0f"),*Place,*AreaId,Middle.X,Middle.Y,Middle.Z,ClearAt);
}

void FS47FarmSite::Step(double Dt)
{
 if(!bActive||!M||!M->Freeform)return;
 if(ClearAt>0&&M->Freeform->Clock()>=ClearAt){ClearAt=0;Show(false);UE_LOG(LogTemp,Display,TEXT("S47: lights site cleared by morning"));return;}
 if(Visit>=1&&Visit<=3&&(!Armed()||!M->Web||!Here()||!M->Player)){EndVisit(false);return;}
 if(!Armed()||!M->Audio||!M->Player||!M->Web||!Here())return;
 const double Dist=FVector2D(M->Player->Pos.X-Middle.X,M->Player->Pos.Z-Middle.Z).Size();
 StepVisit(Dt,Dist);
 if(Dist>140)return;
 SoundIn-=Dt;if(SoundIn>0)return;
 SoundIn=FMath::FRandRange(22.f,48.f);
 // From the place itself: it gets louder and turns with the listener, it never follows them.
 const float Side=FMath::FRandRange(-6.f,6.f);
 M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.3f,.At=Middle+FVector(Side,1,-Side)});
}

void FS47FarmSite::StepVisit(double Dt,double Dist)
{
 auto G=M->Night?M->Night->Glimpses:TSharedPtr<FS47Glimpses>();if(!G||!M->World)return;
 const bool bDriving=M->World->bDriving;
 if(Visit==0)
 {
  // Only after dark, on foot and when it has stood about here a while; nothing else may be going on.
  const bool bDark=M->Life&&M->Life->Sky.IsValid()&&M->Life->Sky->Daylight()<.1;
  const bool bQuiet=M->Freeform->Director&&M->Freeform->Director->ActiveEvent().IsEmpty()&&G->Individuals().IsEmpty();
  VisitDwell=Dist<38&&!bDriving&&bDark&&bQuiet&&!M->ModalOpen()?VisitDwell+Dt:0;
  if(VisitDwell<12)return;
  Visit=1;VisitAge=0;bVisitSeen=false;UE_LOG(LogTemp,Display,TEXT("S47: site visit begins at %s"),*Place);
 }
 if(Visit<1||Visit>3)return;
 if(M->ModalOpen())return;
 const bool bFirst=VisitAge<=0;VisitAge+=Dt;
 if(Visit==1)
 {
  // The warning: the sound of the place, now close, and any radio breaking up. Time enough to go.
  if(bFirst)M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.55f,.At=FVector(M->Player->Pos.X,M->Player->FloorY+1,M->Player->Pos.Z-8)});
  if(VisitAge<6)return;
  if(bDriving||Dist>70){EndVisit(false);return;}
  if(FMath::Fmod(VisitAge,1.)>Dt)return; // tried about once a second
  if(G->ControlledSpawn(1,FS47RoswellBrain::EMode::Curious,false)>0){Visit=2;VisitAge=.001;UE_LOG(LogTemp,Display,TEXT("S47: site visit: a body came out of cover"));}
  else if(VisitAge>25){UE_LOG(LogTemp,Display,TEXT("S47: site visit: no cover to come from here"));EndVisit(false);}
  return;
 }
 if(Visit==2&&VisitAge>22){G->DevMode(FS47RoswellBrain::EMode::Hunt);Visit=3;UE_LOG(LogTemp,Display,TEXT("S47: site visit: it hunts now"));}
 for(const auto& I:G->Individuals())
 {
  bVisitSeen|=I.bRendered;
  if(I.State==FS47RoswellBrain::EState::Contact&&!bDriving){EndVisit(true);return;}
 }
 // Away from the place, it lets go.
 if(bDriving||Dist>70||VisitAge>120)EndVisit(false);
}

void FS47FarmSite::EndVisit(bool bContact)
{
 const bool bWas=Visit>=2;
 Visit=4;VisitDwell=0;
 if(M&&M->Night&&M->Night->Glimpses&&bWas)M->Night->Glimpses->DevClear();
 if(!M||!M->Freeform)return;
 UE_LOG(LogTemp,Display,TEXT("S47: site visit over at %s, contact %d, seen %d"),*Place,bContact,bVisitSeen);
 if(bVisitSeen&&!M->Freeform->HasFinding(FindingId()+TEXT("_visit")))
  M->Freeform->RecordEvidence(FindingId()+TEXT("_visit"),FString::Printf(TEXT("%s, after dark, the night of the lights. A small figure came out of cover and kept coming toward me%s. It says what I saw, not what it was."),*Title,bContact?TEXT(" until it reached me. I lost about fifteen minutes"):TEXT(" until I left")));
 if(bContact)
 {
  // Left where it happened, with the truck and the way home: nothing else is taken.
  M->Freeform->SpendTime(15*60.);
  if(M->UI){M->UI->Fade(true,TEXT("CONTACT LOST"));
   if(M->Story){TWeakObjectPtr<US47Main> Weak=M;M->Story->After(1.2,[Weak]{if(Weak.IsValid()&&Weak->UI){Weak->UI->Fade(false);Weak->UI->Toast(TEXT("You are on the ground where you stood. Your watch has moved on about fifteen minutes."),6);}},TEXT("site-visit"));}
  }
 }
 M->RequestAutosave();
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
 const bool bFarm=Place==TEXT("farm");
 if(bNew)M->Freeform->RecordEvidence(FindingId(),bFarm
  ?FString(TEXT("Pasture at the Alderete place, the night of the lights. Nine pressed marks in a ring about nine metres across, the grass darkened and laid flat. Cattle, a vehicle or a prank have not been ruled out. Worth a second look by daylight."))
  :FString::Printf(TEXT("Open ground outside %s, the night of the lights. Nine dark marks in a ring about five metres across. A vehicle, a spill or a prank have not been ruled out. Worth a second look by daylight."),*Title));
 M->UI->Open(S47Kit::Shell(M->UI,Title.ToUpper(),S47Kit::Col({
  S47Kit::Text(bFarm?TEXT("Nine marks in a ring, about nine metres across. The grass is laid flat and darkened, and it is dry here when the ground beside it is not."):TEXT("Nine dark marks in a ring, about five metres across. They do not smear when you scuff one, and the ground feels warm.")),
  S47Kit::Small(bNew?TEXT("Kept as a field record. It says where and what, not who or why."):TEXT("Already in your field records."))}),760));
 if(bNew)M->RequestAutosave();
 return true;
}
