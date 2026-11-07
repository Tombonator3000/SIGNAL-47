#include "S47FarmSite.h"
#include "S47Taken.h"
#include "S47Aftermath.h"
#include "S47PaleBody.h"
#include "S47Three.h"
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
#include "S47Flashlight.h"
#include "S47FieldRadio.h"
#include "S47IndoorDread.h"
#include "S47Ambience.h"
#include "S47Flow.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/PoseableMeshComponent.h"
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

bool FS47FarmSite::TheirHours(double Clock)
{
 const double Hour=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;
 return Hour>=2&&Hour<6;
}

int32 FS47FarmSite::PickWho(const FString& InPlace,double Hour,float Roll)
{
 const bool bFarm=InPlace==TEXT("farm");
 const float Crab=bFarm?.30f:.12f,Tall=Hour>=3?(bFarm?.20f:.15f):0.f;
 return Roll<Crab?1:Roll<Crab+Tall?2:0;
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
 if(Scene.Get()!=M->Web){bBuilt=false;bBound=false;Marks.Reset();Spots.Reset();Haze.Reset();VisitBody.Reset();CrabBody.Reset();TallBody.Reset();bBodyOut=false;SpotSaro={};SpotRoad={};}
 Scene=M->Web;Life=MakeShared<uint8>(0);bActive=true;bBound=false;
 ClearAt=Saved.IsValid()?S47J::Num(Saved,TEXT("clearAt")):0;SoundIn=20;
 Visit=Saved.IsValid()&&Saved->HasField(TEXT("visit"))?S47J::Int(Saved,TEXT("visit")):4;VisitAge=VisitDwell=0;bNudged=S47J::Bool(Saved,TEXT("nudged"));bStallSet=false;HushIn=0;
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
 if(bLive&&Visit>=1&&Visit<=3&&M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(0,TEXT("visit"));
 if(bLive&&Visit>=1&&Visit<=3)LetAttentionGo();
 if(bLive&&bStallSet&&M->World)M->World->bKeyStall=false;
 bStallSet=false;
 if(bLive)HideBody();
 bBodyOut=false;
 Visit=4;bActive=false;Life.Reset();Show(false);
}

S47J::FObj FS47FarmSite::Save() const
{
 const auto O=S47J::New();
 if(ClearAt>0){S47J::Set(O,TEXT("clearAt"),ClearAt);S47J::Set(O,TEXT("visit"),Visit==0?0:4);S47J::Set(O,TEXT("place"),Place);S47J::Set(O,TEXT("title"),Title);S47J::Set(O,TEXT("area"),AreaId);S47J::Set(O,TEXT("at"),TArray<double>{Middle.X,Middle.Y,Middle.Z});S47J::Set(O,TEXT("nudged"),bNudged);}
 return O;
}

void FS47FarmSite::ArmFarm(double Clock){Arm(Clock,TEXT("farm"),TEXT("the Alderete place"),TEXT("saro"),FarmCentre());}

void FS47FarmSite::Arm(double Clock,const FString& InPlace,const FString& InTitle,const FString& InArea,const FVector& Point)
{
 // Gone by the next noon: time for the second look by daylight that the note asks for.
 static constexpr double Day=86400,Morning=12*3600;
 if(Visit>=1&&Visit<=3)EndVisit(false);
 ClearAt=FMath::FloorToDouble((Clock-Morning)/Day+1)*Day+Morning;
 Place=InPlace;Title=InTitle;AreaId=InArea;Middle=Point;
 // Not every night's lights leave something behind that comes back for you.
 Visit=FMath::FRand()<.6f&&Clock>=CalmUntil?0:4;VisitDwell=0;
 // Marks made in the evening get word later, when something can come of it; made now, the tip was word enough.
 bNudged=TheirHours(Clock);
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

void FS47FarmSite::SayNudge()
{
 bNudged=true;
 if(!M)return;
 const FString Tip=FString::Printf(TEXT("A second call about %s. The caller says the lights have not gone, and won't go out to look."),*Title);
 if(!(M->FieldRadio&&M->FieldRadio->Message(Tip))&&M->UI)M->UI->Toast(TEXT("RADIO: ")+Tip,7);
 M->RequestAutosave();
}

bool FS47FarmSite::WakeCall()
{
 if(!M||!M->Freeform||!WillStandAt(M->Freeform->Clock()))return false;
 SayNudge();return true;
}

void FS47FarmSite::Step(double Dt)
{
 if(!bActive||!M||!M->Freeform)return;
 if(ClearAt>0&&M->Freeform->Clock()>=ClearAt){ClearAt=0;Show(false);UE_LOG(LogTemp,Display,TEXT("S47: lights site cleared by morning"));return;}
 // A menu holds all of it: no sound starts, no second counts.
 if(M->bPausedByMenu||M->ModalOpen())return;
 // Evening's marks, and now the hour they can come: a second report of the same place, for whoever went home.
 if(Visit==0&&!bNudged&&Armed()&&TheirHours(M->Freeform->Clock())&&M->Player&&(!Here()||FVector2D(M->Player->Pos.X-Middle.X,M->Player->Pos.Z-Middle.Z).Size()>100))
 {
  SayNudge();
 }
 if(Visit>=1&&Visit<=3&&(!Armed()||!M->Web||!Here()||!M->Player)){EndVisit(bVisitTaken||(M->Freeform->Taken&&M->Freeform->Taken->Busy()));return;}
 if(!Armed()||!M->Audio||!M->Player||!M->Web||!Here())return;
 const double Dist=FVector2D(M->Player->Pos.X-Middle.X,M->Player->Pos.Z-Middle.Z).Size();
 StepVisit(Dt,Dist);
 // Everything out here goes quiet first; a stretch at a time, so that it comes back on its own whatever ends this.
 if(VisitHushed())
 {
  HushIn-=Dt;
  if(HushIn<=0&&M->Audio->Night.IsValid()){HushIn=3;M->Audio->Night->Hush(5,.05,.8,3);}
 }
 else HushIn=0;
 // A lamp with a weak contact, while it lasts.
 if(Visit>=1&&Visit<=3&&M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(.25+.3*FMath::Abs(FMath::Sin(VisitAge*1.3)),TEXT("visit"));
 if(Dist>140)return;
 SoundIn-=Dt;if(SoundIn>0)return;
 SoundIn=FMath::FRandRange(22.f,48.f);
 // From the place itself: it gets louder and turns with the listener, it never follows them.
 const float Side=FMath::FRandRange(-6.f,6.f);
 M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.3f,.At=Middle+FVector(Side,1,-Side)});
}

double FS47FarmSite::VisitBodyDistance() const
{
 if(!M||!M->Player)return -1;
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 if(bBodyOut)return FVector::Dist(BodyAt,Feet);
 if(M->Night&&M->Night->Glimpses){const auto V=M->Night->Glimpses->Individuals();if(!V.IsEmpty())return FVector::Dist(V[0].Pos,Feet);}
 return -1;
}

bool FS47FarmSite::PutBody(const FVector& Near)
{
 if(!M||!M->Web||!M->Player)return false;
 const bool bTall=VisitWho==2;
 TSharedPtr<FS47PaleBody>& Own=bTall?TallBody:CrabBody;
 if(!Own)
 {
  Own=MakeShared<FS47PaleBody>();Own->CreatureAsset=bTall?S47KindBody::Tall:S47KindBody::Crab;
  if(bTall)Own->Style=ES47PaleStyle::Tall;
 }
 if(!Own->bBuilt&&!Own->Build(M->Web.Get(),TEXT("#root"),bTall?TEXT("SiteVisitTall"):TEXT("SiteVisitCrab")))return false;
 if(!Own->Creature.Ready())return false;
 // Whatever was out goes first, by its own kind, before this one takes over.
 HideBody();
 VisitBody=Own;BodyKind=VisitWho;
 // Always out of the view, so nothing pops in, and with a clear line once the player turns. The tall one stands far
 // off in the open, to be seen across the ground; the crab starts nearer, low. As far as the land allows.
 const FVector Eye=Near+FVector(0,1.6,0);
 const double Head=bTall?1.7:.5;
 const TArray<double> Far=bTall?TArray<double>{58.,44.,30.,18.}:TArray<double>{20.,14.,10.};
 for(const double Dist:Far)for(const double Turn:{double(PI),2.4,-2.4,1.7,-1.7})
 {
  const double Yaw=M->Player->Yaw+Turn;
  FVector At=Near+FVector(-FMath::Sin(Yaw)*Dist,0,-FMath::Cos(Yaw)*Dist);
  // On the ground there, not on a roof over it.
  At.Y=GroundAt(At.X,Near.Y,At.Z);
  if(At.Y>Near.Y+3||At.Y<Near.Y-5)continue;
  FHitResult Block;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SiteBodySight),true);
  if(M->Web->GetWorld()->LineTraceSingleByChannel(Block,S47Three::ToUE(Eye),S47Three::ToUE(At+FVector(0,Head,0)),ECC_Visibility,Q))continue;
  BodyAt=At;BodyYaw=FMath::Atan2(Near.Z-At.Z,Near.X-At.X);BodyClock=0;bBodyOut=true;return true;
 }
 return false;
}

double FS47FarmSite::VisitHeadHeight() const
{
 if(!bBodyOut||!VisitBody)return -1;
 auto* Skin=VisitBody->Creature.Mesh.Get();if(!Skin)return -1;
 return S47Three::ToThree(Skin->GetBoneLocationByName(TEXT("head"),EBoneSpaces::WorldSpace)).Y-BodyAt.Y;
}

void FS47FarmSite::StepBody(double Dt,double Speed)
{
 if(!bBodyOut||!VisitBody||!M||!M->Player||!M->Web)return;
 BodyClock+=Dt;
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 // Held by a lamp or a sound: it faces that, and its only way of moving is back from it.
 const bool bHeldNow=Attention.Holding();
 const FVector FaceAt=bHeldNow?Attention.LookAt():Feet;
 if(bHeldNow)Speed=Attention.BackSpeed();
 const double Face=FMath::Atan2(FaceAt.Z-BodyAt.Z,FaceAt.X-BodyAt.X);
 BodyYaw+=FMath::UnwindRadians(Face-BodyYaw)*FMath::Min(1.,Dt*2.5);
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SiteBodyWay),true);
 for(const auto& Part:VisitBody->All)if(auto* C=Part.Get())Q.AddIgnoredComponent(C);
 // Straight for the player, a little round what is in the way. Walls hold it, and doors give time.
 if(Speed>0)for(const double Off:{0.,.8,-.8,1.5,-1.5})
 {
  const double Y=BodyYaw+Off;const FVector Dir=FVector(FMath::Cos(Y),0,FMath::Sin(Y))*(bHeldNow?-1.:1.);
  FHitResult Hit;
  const FVector A=BodyAt+FVector(0,.55,0);
  if(M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(A),S47Three::ToUE(A+Dir*.6),ECC_Visibility,Q))continue;
  BodyAt+=Dir*Speed*Dt;break;
 }
 FHitResult Floor;
 if(M->Web->GetWorld()->LineTraceSingleByChannel(Floor,S47Three::ToUE(BodyAt+FVector(0,1.5,0)),S47Three::ToUE(BodyAt-FVector(0,3,0)),ECC_Visibility,Q))
  BodyAt.Y=FMath::FInterpTo(BodyAt.Y,S47Three::ToThree(Floor.ImpactPoint).Y,Dt,12.);
 if(VisitWho==1)VisitBody->ApplyCrab(true,BodyAt,BodyYaw,Face,BodyClock,BodyClock/1.6,Speed>2.);
 else VisitBody->Apply(true,BodyAt,BodyYaw,Face,Speed>0?.85:0.,BodyClock*3,BodyClock,FS47PaleBody::EPose::Walk);
 bVisitSeen|=VisitBody->Rendered();
}

void FS47FarmSite::HideBody()
{
 if(VisitBody&&VisitBody->bBuilt&&bBodyOut)
 {
  if(BodyKind==1)VisitBody->ApplyCrab(false,BodyAt,BodyYaw,BodyYaw,BodyClock,1,false);
  else VisitBody->Apply(false,BodyAt,BodyYaw,BodyYaw,0,0,BodyClock,FS47PaleBody::EPose::Walk);
 }
 bBodyOut=false;
}

void FS47FarmSite::StepVisit(double Dt,double Dist)
{
 auto G=M->Night?M->Night->Glimpses:TSharedPtr<FS47Glimpses>();if(!G||!M->World)return;
 auto Tk=M->Freeform->Taken;if(!Tk)return;
 const bool bDriving=M->World->bDriving;
 // In the truck with the engine dead is not away: the visit goes on round it until the key turns. Nothing reaches into
 // the cab, though, so a stalled truck is frightening, and safe.
 const bool bAway=bDriving&&!M->World->Trouble.WantsKey();
 if(Visit==0)
 {
  // Only after dark, on foot and when it has stood about here a while; nothing else may be going on.
  const bool bDark=M->Life&&M->Life->Sky.IsValid()&&M->Life->Sky->Daylight()<.1;
  // Something at a door indoors counts too: it goes first, and this waits its turn.
  const bool bQuiet=M->Freeform->Director&&M->Freeform->Director->ActiveEvent().IsEmpty()&&G->Individuals().IsEmpty()&&!Tk->Busy()&&!(M->Freeform->IndoorDread&&M->Freeform->IndoorDread->DoorActive());
  const double Clock=M->Freeform->Clock();
  VisitDwell=Dist<55&&!bDriving&&bDark&&bQuiet&&TheirHours(Clock)&&Clock>=CalmUntil&&!M->ModalOpen()?VisitDwell+Dt:0;
  if(VisitDwell<12)return;
  const float Roll=TestRoll>=0?TestRoll:FMath::FRand();TestRoll=-1;
  Visit=1;VisitAge=0;bVisitSeen=false;bVisitTaken=false;VisitWho=PickWho(Place,FMath::Fmod(Clock,86400.)/3600.,Roll);
  UE_LOG(LogTemp,Display,TEXT("S47: site visit begins at %s, kind %d"),*Place,VisitWho);
 }
 if(Visit<1||Visit>3)return;
 if(M->ModalOpen())return;
 const ES47Taker Who=VisitWho==1?ES47Taker::Crab:VisitWho==2?ES47Taker::Tall:ES47Taker::Roswell;
 // Taken: all else waits, and when they are back, it is over.
 if(Tk->Stage()==FS47Taken::EStage::Lift||Tk->Stage()==FS47Taken::EStage::Away){bVisitTaken=true;HideBody();LetAttentionGo();if(!G->Individuals().IsEmpty())G->DevClear();return;}
 if(bVisitTaken){EndVisit(true);return;}
 const bool bFirst=VisitAge<=0;VisitAge+=Dt;
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 // Under a roof it starts overhead, also when they have only just run in there.
 // Twice at most: when the light has given up on one window, it tries another.
 if(bFirst)
 {
  VisitRounds=0;VisitHunt=0;VisitPatience=120;LetAttentionGo();Attention.Reset();bAttentionFeet=false;bWasDriving=bDriving;bWasRunning=false;bSawTurn=false;bSawSetTurn=false;
  RadioSeen=M->FieldRadio?M->FieldRadio->MessagesHeard():0;
  // Sometimes the set speaks up while it is out there, whether the player wants it to or not.
  RadioIn=!bTestHoldHunt&&FMath::FRand()<.35f?FMath::FRandRange(10.f,30.f):-1.;
 }
 if(VisitRounds<2&&!bDriving&&!Tk->Busy()&&FMath::Fmod(VisitAge,1.)<=Dt&&Tk->Indoors()&&Tk->Begin(Who,Title))++VisitRounds;
 if(Visit==1)
 {
  // The warning: the sound of the place, now close, and any radio breaking up. Time enough to go.
  if(bFirst)
  {
   M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.55f,.At=FVector(M->Player->Pos.X,M->Player->FloorY+1,M->Player->Pos.Z-8)});
   // The truck will not start now, the first time it is tried.
   M->World->bKeyStall=true;bStallSet=true;
  }
  if(VisitAge<6)return;
  if(bAway||Dist>80){EndVisit(false);return;}
  if(VisitWho==0)
  {
   if(FMath::Fmod(VisitAge,1.)>Dt)return; // tried about once a second
   if(G->ControlledSpawn(1,FS47RoswellBrain::EMode::Curious,false)>0){Visit=2;VisitAge=.001;UE_LOG(LogTemp,Display,TEXT("S47: site visit: a body came out of cover"));}
   else if(VisitAge>25){UE_LOG(LogTemp,Display,TEXT("S47: site visit: no cover to come from here"));EndVisit(false);}
   return;
  }
  // The tall one stands off, to be seen; the crab comes along the ground.
  if(FMath::Fmod(VisitAge,1.)>Dt)return;
  if(PutBody(Feet)){Visit=2;VisitAge=.001;UE_LOG(LogTemp,Display,TEXT("S47: site visit: kind %d is here, %.0f m off"),VisitWho,FVector::Dist(BodyAt,Feet));}
  else if(VisitAge>25){EndVisit(false);}
  return;
 }
 bool bNothingToGoOn=false;
 StepAttention(Dt,bDriving,bNothingToGoOn);
 // A lamp on it buys a moment. A player with the lamp out, standing still, gives it nothing to go on: no hunt comes of
 // that, and it lets go in half the time. Moving, or with the lamp on, it goes as it always has.
 VisitHunt+=Attention.Holding()||bTestHoldHunt||bNothingToGoOn?0.:Dt;
 VisitPatience-=Dt*(bNothingToGoOn&&!Tk->Busy()?2.:1.);
 if(VisitWho==0)
 {
  if(Visit==2&&VisitHunt>22){G->DevMode(FS47RoswellBrain::EMode::Hunt);Visit=3;VisitPatience=FMath::Max(VisitPatience,60.);UE_LOG(LogTemp,Display,TEXT("S47: site visit: it hunts now"));}
  for(const auto& I:G->Individuals())
  {
   bVisitSeen|=I.bRendered;
   if(I.State==FS47RoswellBrain::EState::Contact&&!bDriving){G->DevClear();Tk->TakeNow(Who,Title);bVisitTaken=true;return;}
  }
 }
 else
 {
  const double To=FVector::Dist(BodyAt,Feet);
  // The tall one first stands still and lets itself be found, then walks, slower as it nears. Its light goes before it.
  const double Speed=VisitWho==1?(BodyClock<1.6?0.:To<13?3.2:1.2):(BodyClock<12?0.:To>30?1.35:.75);
  StepBody(Dt,Speed);
  if(VisitWho==2&&VisitRounds<2&&!bDriving&&!Tk->Busy()&&To<(VisitRounds==0?26.:10.)&&Tk->Begin(Who,Title))++VisitRounds;
  if(To<(VisitWho==1?1.9:1.5)&&!bDriving){HideBody();Tk->TakeNow(Who,Title);bVisitTaken=true;return;}
 }
 // Away from the place, it lets go.
 if(bAway||Dist>80||VisitPatience<=0)EndVisit(false);
}

void FS47FarmSite::LetAttentionGo()
{
 if(!M)return;
 if(M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(0,TEXT("attention"));
 if(M->Night&&M->Night->Glimpses)M->Night->Glimpses->HoldAttention({},0,false);
}

void FS47FarmSite::StepAttention(double Dt,bool bDriving,bool& bNothingToGoOn)
{
 auto G=M->Night?M->Night->Glimpses:TSharedPtr<FS47Glimpses>();auto Lamp=M->Night?M->Night->Flashlight:TSharedPtr<FS47Flashlight>();
 if(!G||!Lamp)return;
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 double Pace=bAttentionFeet&&Dt>0?FVector::Dist(Feet,AttentionFeet)/Dt:0.;
 if(Pace>12)Pace=0; // Put somewhere, not walking there.
 AttentionFeet=Feet;bAttentionFeet=true;
 const auto In=M->Night->GlimpseIn(true);
 const bool bLampOn=Lamp->Intensity()>0;
 bNothingToGoOn=!bTestNoticed&&!bLampOn&&Pace<.3&&!bDriving;
 // The body out now: the small one's own, or the moved one's.
 TOptional<FVector> Eye=VisitBodyEye();
 if(VisitWho==0){const auto V=G->Individuals();if(V.IsEmpty())Eye.Reset();else Eye=V[0].Eye;}
 // No body now, nothing to hold: the lamp is not left flickering for it.
 if(!Eye.IsSet()){LetAttentionGo();return;}
 const bool bCalm=M->Flow&&M->Flow->bCalmFlash;
 FS47VisitAttention::FIn See;
 See.bCalm=bCalm;
 See.bLampOn=bLampOn;See.Lamp=In.Cam.Pos;See.BodyDist=FVector::Dist(*Eye,Feet);
 const FVector ToBody=*Eye-In.Cam.Pos;const double Reach=ToBody.Size();
 if(bLampOn&&Reach<FS47VisitAttention::BeamReach&&Reach>.1&&FVector::DotProduct(In.Cam.Forward,ToBody/Reach)>.9&&M->Web&&M->Web->GetWorld())
 {
  // The body itself takes no trace; anything real in between stops the beam.
  FHitResult Block;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47AttentionBeam),true);
  See.bBeam=!M->Web->GetWorld()->LineTraceSingleByChannel(Block,S47Three::ToUE(In.Cam.Pos),S47Three::ToUE(*Eye),ECC_Visibility,Q)||FVector::Dist(S47Three::ToThree(Block.ImpactPoint),*Eye)<1.;
 }
 // What it can hear, as it begins: running feet, the truck's door and starter, a voice on the set.
 const bool bRunning=Pace>2.4;
 const int32 RadioNow=M->FieldRadio?M->FieldRadio->MessagesHeard():0;
 if(bRunning&&!bWasRunning&&See.BodyDist<40){See.bNoise=true;See.Sound=Feet;}
 if(bDriving&&!bWasDriving&&See.BodyDist<60){See.bNoise=true;See.Sound=Feet;}
 // The set is the player's to carry, but not their noise: nobody chose that voice.
 const bool bSetSpoke=RadioNow!=RadioSeen&&See.BodyDist<30;
 if(bSetSpoke){See.bNoise=true;See.Sound=In.Cam.Pos-FVector(0,.4,0);}
 bWasRunning=bRunning;bWasDriving=bDriving;RadioSeen=RadioNow;
 const auto Before=Attention.Doing();
 Attention.Step(Dt,See,AttentionRoll>=0?AttentionRoll:FMath::FRand());
 Lamp->Flicker(Attention.BlinkLevel(bCalm),TEXT("attention"));
 if(Attention.Doing()!=Before)
 {
  UE_LOG(LogTemp,Display,TEXT("S47: attention: %s, %.1f m off, lamp %d"),FS47VisitAttention::Name(Attention.Doing()),See.BodyDist,bLampOn);
  // Something for the ear when it stops: one small step, from where it is.
  if(Before==FS47VisitAttention::EDo::None&&M->Audio)M->Audio->Sfx(TEXT("stepConcrete0"),FS47Sound{.Gain=.2f,.Rate=.7f,.At=*Eye-FVector(0,1,0)});
  // The lamp's weak contact gives way altogether, for most of the moment. Calm flashes: it only unsettles it.
  if(Attention.Doing()==FS47VisitAttention::EDo::Blink&&!bCalm)Lamp->FlickerOut(.75);
  // A turn is only for the record if it was there to be seen. A lamp on it has been aimed at it, so that needs no asking.
  if(Attention.Doing()==FS47VisitAttention::EDo::Turn)
  {
   bool bRendered=bBodyOut&&VisitBody&&VisitBody->Rendered();
   if(VisitWho==0){const auto V=G->Individuals();bRendered=!V.IsEmpty()&&V[0].bRendered;}
   if(bSetSpoke)bSawSetTurn|=bRendered;else bSawTurn|=bRendered;
  }
 }
 if(VisitWho==0)
 {
  // A lamp is on the one it found; a noise is for all of them to hear.
  const auto Bodies=G->Individuals();
  if(Attention.Holding())G->HoldAttention(Attention.LookAt(),Attention.BackSpeed(),Attention.Heard(),Attention.Heard()||Bodies.IsEmpty()?int32(INDEX_NONE):Bodies[0].Id);
  else if(Before!=FS47VisitAttention::EDo::None)G->HoldAttention({},0,false);
 }
 // The set speaks up by itself; what that does is above, next time round.
 if(RadioIn>0&&M->FieldRadio&&M->FieldRadio->Owned()){RadioIn-=Dt;if(RadioIn<=0)M->FieldRadio->Message(TEXT("A voice on the set, too broken to make out."));}
}

void FS47FarmSite::EndVisit(bool bTaken)
{
 const bool bWas=Visit>=2;
 Visit=4;VisitDwell=0;bVisitTaken=false;
 LetAttentionGo();
 // Over: the lamp steadies, and a truck not yet tried starts again.
 if(M&&M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(0,TEXT("visit"));
 if(bStallSet&&M&&M->World)M->World->bKeyStall=false;
 bStallSet=false;
 if(M&&M->Freeform&&M->Freeform->Taken&&!bTaken)M->Freeform->Taken->GiveUp();
 HideBody();
 if(M&&M->Night&&M->Night->Glimpses&&bWas)M->Night->Glimpses->DevClear();
 if(!M||!M->Freeform)return;
 UE_LOG(LogTemp,Display,TEXT("S47: site visit over at %s, taken %d, seen %d"),*Place,bTaken,bVisitSeen);
 // Something really came, at the farm: the herd may be one short in the morning.
 if((bWas||bTaken)&&Place==TEXT("farm")&&M->Freeform->Aftermath)M->Freeform->Aftermath->AfterFarmVisit(M->Freeform->Clock());
 if(bVisitSeen&&!bTaken&&!M->Freeform->HasFinding(FindingId()+TEXT("_visit")))
 {
  // Only what the lamp and the noise really did, and only if it was seen at all (bVisitSeen).
  FString Did;
  if(Attention.bDidBack)Did+=TEXT(" It stopped when my lamp found it, looked back along the beam and drew away.");
  if(Attention.bDidBlink)Did+=TEXT(" My lamp went out for a moment while it looked at me.");
  if(bSawTurn)Did+=TEXT(" It turned toward a noise I made.");
  if(bSawSetTurn)Did+=TEXT(" It turned toward my field set when a voice came over it.");
  M->Freeform->RecordEvidence(FindingId()+TEXT("_visit"),FString::Printf(TEXT("%s, after dark, the night of the lights. Something came toward me until I left.%s This records what I saw, not what it was."),*Title,*Did));
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
