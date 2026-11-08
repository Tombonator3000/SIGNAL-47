#include "S47DoorVisit.h"
#include "S47EventPlaces.h"
#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47CalicheAccess.h"
#include "S47FarmHouse.h"
#include "S47FarmSite.h"
#include "S47FieldRadio.h"
#include "S47Flashlight.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47Glimpses.h"
#include "S47IndoorDread.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Rhythm.h"
#include "S47Taken.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/GarbageCollection.h"

namespace S47DoorVisitLocal
{
 constexpr int32 PieceCount=3;
 const FVector HouseFrame(-24,0,300); // The old house stands in the farm's part of the SARO area.
 FString TownTitle(const FString& Id)
 {
  if(Id==TEXT("Sheriff"))return TEXT("the sheriff's office");
  if(Id==TEXT("Laundry"))return TEXT("the laundromat");
  if(Id==TEXT("Feed"))return TEXT("the feed store");
  if(Id==TEXT("Cinema"))return TEXT("the cinema");
  if(Id==TEXT("Church"))return TEXT("the church");
  if(Id==TEXT("Souvenir"))return TEXT("the souvenir shop");
  return Id.StartsWith(TEXT("H0"))?TEXT("an empty house in town"):TEXT("a building in town");
 }
 FString MarksText(FS47DoorVisit::EEnding Ending)
 {
  switch(Ending)
  {
  case FS47DoorVisit::EEnding::Held:return TEXT("Three pale marks in a row along the wall outside the door, evenly spaced and going away from it. None at the door itself.");
  case FS47DoorVisit::EEnding::Opened:return TEXT("One pale mark on the ground at the threshold, square to the door. Nothing either side of it.");
  default:return TEXT("Two pale marks side by side just inside the threshold, facing the room. None outside the door.");
  }
 }
 // The way out of the old house: the inner doors have a room behind them, not the outside.
 bool Outer(const FString& Name){return Name.StartsWith(TEXT("north"))||Name.StartsWith(TEXT("south"));}
 FString Upper(FString Text){if(!Text.IsEmpty())Text[0]=FChar::ToUpper(Text[0]);return Text;}
}

void FS47DoorVisit::Begin(const S47J::FObj& Saved)
{
 Stop();
 RollDay=S47J::Int(Saved,TEXT("rollDay"));LastDay=S47J::Int(Saved,TEXT("lastDay"));bTonight=S47J::Bool(Saved,TEXT("tonight"));
 InsideFor=0;WaitFor=bTonight?20:0;TestRoll=-1;LastBefore=LastDay;RetryDay=0;
 Trace=EEnding::None;TraceAt.Reset();TraceArea.Empty();TraceTitle.Empty();TraceDay=0;TraceUntil=0;
 const S47J::FObj Left=S47J::Obj(Saved,TEXT("trace"));
 const TArray<double> Spots3=S47J::Nums(Left,TEXT("at"));
 const int32 Kind=S47J::Int(Left,TEXT("kind"));
 if(Left.IsValid()&&Kind>=1&&Kind<=3&&Spots3.Num()>=3&&Spots3.Num()%3==0)
 {
  Trace=EEnding(Kind);TraceArea=S47J::Str(Left,TEXT("area"));TraceTitle=S47J::Str(Left,TEXT("title"));TraceDay=S47J::Int(Left,TEXT("day"));TraceUntil=S47J::Num(Left,TEXT("until"));
  for(int32 I=0;I+2<Spots3.Num()&&TraceAt.Num()<S47DoorVisitLocal::PieceCount;I+=3)TraceAt.Add(FVector(Spots3[I],Spots3[I+1],Spots3[I+2]));
 }
 if(!M||!IsValid(M->Web.Get()))return;
 if(Scene.Get()!=M->Web.Get()){bBuilt=false;Pieces.Reset();Spots.Reset();SpotSaro={};SpotRoad={};Sight.Init(M);}
 Scene=M->Web.Get();Life=MakeShared<uint8>(0);Sight.Init(M);
 if(!Build())return;
 const TWeakPtr<uint8> Guard=Life;
 if(IsValid(M->Inter))
 {
  FS47Interactable I;I.Id=TEXT("ff:doormark");I.Object=SpotSaro;I.Range=2.6;
  I.Label=[this,Guard]{return Guard.IsValid()?MarksLabel():FString();};
  I.Use=[this,Guard]{if(Guard.IsValid())UseMarks();};
  I.Examine=I.Label;
  M->Inter->Add(I);M->Web->MarkUsable(SpotSaro.I,TEXT("ff:doormark"));M->Web->MarkUsable(SpotRoad.I,TEXT("ff:doormark"));
 }
 LayoutMarks();ShowMarks(false);
}

void FS47DoorVisit::LetGo()
{
 if(Target.bHouse){if(M&&M->Freeform&&M->Freeform->HouseDoors)M->Freeform->HouseDoors->PoseOver(Target.Index,-1);}
 else if(!Target.Name.IsEmpty())S47CalicheAccess::PoseDoor(Target.Name,-1);
 if(M&&M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(0,TEXT("door"));
 if(Owner)Owner->HideDoorBody();
 bBodyOut=false;
}

void FS47DoorVisit::Stop()
{
 // Not while the engine or the owner is going away: the scene the door hangs in may already be gone.
 const bool bLive=!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->Web.Get());
 if(bLive&&Now!=EStage::None)LetGo();
 Now=EStage::None;bBodyOut=false;
 if(bLive)ShowMarks(false);
 Life.Reset();
}

S47J::FObj FS47DoorVisit::Save() const
{
 // One under way is not saved: Continue finds a quiet room, and that night's is spent.
 const auto O=S47J::New();
 if(RollDay>0)S47J::Set(O,TEXT("rollDay"),RollDay);
 if(LastDay>0)S47J::Set(O,TEXT("lastDay"),LastDay);
 if(bTonight)S47J::Set(O,TEXT("tonight"),true);
 if(Trace!=EEnding::None&&!TraceAt.IsEmpty())
 {
  const auto R=S47J::New();TArray<double> At;for(const FVector& P:TraceAt)At.Append({P.X,P.Y,P.Z});
  S47J::Set(R,TEXT("kind"),int32(Trace));S47J::Set(R,TEXT("area"),TraceArea);S47J::Set(R,TEXT("title"),TraceTitle);S47J::Set(R,TEXT("day"),TraceDay);S47J::Set(R,TEXT("until"),TraceUntil);S47J::Set(R,TEXT("at"),At);
  S47J::Set(O,TEXT("trace"),R);
 }
 return O;
}

bool FS47DoorVisit::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web.Get()))return false;
 AS47Web* W=M->Web.Get();
 const int32 Saro=W->Handle(TEXT("ext.group")),Road=W->Handle(TEXT("world.legs.road.area.group"));if(Saro<0||Road<0)return false;
 auto* Disc=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Disc||!Base)return false;
 const auto Piece=[&](const FString& Name)
 {
  auto* Mat=UMaterialInstanceDynamic::Create(Base,W);Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.42,.40,.36));Mat->SetScalarParameterValue(TEXT("Roughness"),.95);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  auto* C=NewObject<UStaticMeshComponent>(W,*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(W->GetRootComponent());C->SetStaticMesh(Disc);C->SetMaterial(0,Mat);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();
  return C;
 };
 for(int32 I=0;I<S47DoorVisitLocal::PieceCount;++I)Pieces.Add(Piece(FString::Printf(TEXT("S47DoorMark%d"),I)));
 // One aim target per exterior root, as the marks at a site have: the player's ray finds whichever area is shown.
 const auto Aim=[&](const TCHAR* Name,int32 Parent,FS47Obj& Out)
 {
  // Not drawn, and only the player's aim finds it: what is seen is the marks themselves.
  auto* C=Piece(Name);C->SetVisibility(false);C->SetHiddenInGame(true);C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
  Out=W->ObjAt(W->AdoptNode(C,Parent,Name));Spots.Add(C);
 };
 Aim(TEXT("ff:doormark"),Saro,SpotSaro);Aim(TEXT("ff:doormark:road"),Road,SpotRoad);
 bBuilt=true;return true;
}

bool FS47DoorVisit::MarksHere() const
{
 if(Trace==EEnding::None||TraceAt.IsEmpty()||!M||!M->Web)return false;
 const FString Area=M->Web->Area();
 return Area==TraceArea||(Area==TEXT("road")&&TraceArea==TEXT("diner"))||(Area==TEXT("diner")&&TraceArea==TEXT("road"));
}

void FS47DoorVisit::ShowMarks(bool bShow)
{
 bShown=bShow;
 for(int32 I=0;I<Pieces.Num();++I)if(auto* C=Pieces[I].Get()){const bool bThis=bShow&&TraceAt.IsValidIndex(I);C->SetVisibility(bThis);C->SetHiddenInGame(!bThis);}
 for(const auto& S:Spots)if(auto* C=S.Get())C->SetCollisionEnabled(bShow?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
}

void FS47DoorVisit::LayoutMarks()
{
 if(!bBuilt||TraceAt.IsEmpty())return;
 const FVector Flat=S47Three::DirToUE(FVector(.34,.012,.2));
 const FVector Scale(FMath::Abs(Flat.X),FMath::Abs(Flat.Y),FMath::Abs(Flat.Z));
 for(int32 I=0;I<Pieces.Num()&&I<TraceAt.Num();++I)if(auto* C=Pieces[I].Get())
 {
  C->SetWorldLocation(S47Three::ToUE(TraceAt[I]+FVector(0,.014,0)));C->SetWorldScale3D(Scale);
 }
 // The aim target lies under the first of them, a little wider, so that looking at any of it is enough.
 const FVector Wide=S47Three::DirToUE(FVector(.7,.03,.7));
 for(const auto& S:Spots)if(auto* C=S.Get())
 {
  C->SetWorldLocation(S47Three::ToUE(TraceAt[0]+FVector(0,.012,0)));C->SetWorldScale3D(FVector(FMath::Abs(Wide.X),FMath::Abs(Wide.Y),FMath::Abs(Wide.Z)));
 }
}

double FS47DoorVisit::GroundNear(const FVector& At) const
{
 if(M&&M->Web&&M->Web->GetWorld())
 {
  FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47DoorMarkGround),true);
  if(M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(At+FVector(0,1.2,0)),S47Three::ToUE(At-FVector(0,2.5,0)),ECC_Visibility,Q))return S47Three::ToThree(Hit.ImpactPoint).Y;
 }
 return At.Y;
}

void FS47DoorVisit::PutMarks(EEnding Ending)
{
 const FVector Side(-Target.Out.Z,0,Target.Out.X);
 const FVector Mid=Target.Middle,Out=Target.Out;
 TraceAt.Reset();
 if(Ending==EEnding::Held)TraceAt={Mid+Out*.7+Side*1.5,Mid+Out*.7+Side*2.2,Mid+Out*.7+Side*2.9};
 else if(Ending==EEnding::Opened)TraceAt={Mid+Out*.55};
 else TraceAt={Mid-Out*.55+Side*.18,Mid-Out*.55-Side*.18};
 for(FVector& P:TraceAt)P.Y=GroundNear(P);
 Trace=Ending;TraceArea=Target.Area;TraceTitle=Target.Title;TraceDay=M&&M->Freeform?M->Freeform->Day():0;
 // Gone by the next noon: time to go and look by daylight.
 static constexpr double TraceWhole=86400,TraceNoon=12*3600;
 const double Clock=M&&M->Freeform?M->Freeform->Clock():0;
 TraceUntil=FMath::FloorToDouble((Clock-TraceNoon)/TraceWhole+1)*TraceWhole+TraceNoon;
 LayoutMarks();ShowMarks(MarksHere());
}

FString FS47DoorVisit::MarksLabel() const
{
 if(!bShown||!MarksHere())return FString();
 return M&&M->Freeform&&M->Freeform->HasFinding(FString::Printf(TEXT("door-marks-%d"),TraceDay))?TEXT("Pale mark / already noted"):TEXT("Pale mark / look closer");
}

bool FS47DoorVisit::UseMarks()
{
 if(!bShown||!MarksHere()||!M||!M->UI||!M->Freeform)return false;
 const FString Key=FString::Printf(TEXT("door-marks-%d"),TraceDay);
 const FString Text=S47DoorVisitLocal::MarksText(Trace);
 const bool bNew=!M->Freeform->HasFinding(Key);
 if(bNew)M->Freeform->RecordEvidence(Key,FString::Printf(TEXT("%s, by a door that was tried in the night. %s An animal or a prank have not been ruled out."),*S47DoorVisitLocal::Upper(TraceTitle),*Text));
 M->UI->Open(S47Kit::Shell(M->UI,TraceTitle.ToUpper(),S47Kit::Col({
  S47Kit::Text(Text+TEXT(" They do not smear when you scuff one.")),
  S47Kit::Small(bNew?TEXT("Kept as a field record. It says where and what, not who or why."):TEXT("Already in your field records."))}),760));
 if(bNew)M->RequestAutosave();
 return true;
}

bool FS47DoorVisit::FindDoor(FTarget& Out) const
{
 const FString Area=M->Web->Area();
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 const auto Toward=[&](const FVector& Middle,const FVector& Normal){return FVector::DotProduct(FVector(Middle.X-Feet.X,0,Middle.Z-Feet.Z),Normal)>=0?Normal:-Normal;};
 // Only a door on the list of places (S47EventPlaces) is one it comes to: the old house's, a room in town. The list names it.
 const FS47EventPlaces* Places=M->Freeform?M->Freeform->EventPlaces.Get():nullptr;
 if(Area==TEXT("saro"))
 {
  const FS47EventPlace* House=Places?Places->WithDoor(TEXT("house")):nullptr;
  if(Places&&(!House||!House->Has(ES47PlaceTrait::Door)))return false;
  const FVector Frame=S47DoorVisitLocal::HouseFrame;
  const auto Doors=M->Freeform->HouseDoors;
  // Within the walls, and at a door to the outside: what comes through it comes from out there. The veranda has a roof, but
  // from there the far side of the door is the inside of the house.
  if(!Doors||!S47FarmHouse::Inside(Feet.X-Frame.X,Feet.Z-Frame.Z))return false;
  double Best=12;bool bFound=false;
  for(int32 I=0;I<Doors->Num();++I)if(!Doors->IsOpen(I)&&S47DoorVisitLocal::Outer(Doors->NameOf(I)))
  {
   const FVector Middle=Doors->Middle(I);
   const double D=FVector2D::Distance(FVector2D(Middle.X,Middle.Z),FVector2D(Feet.X,Feet.Z));
   if(D<1.6||D>=Best)continue;
   Best=D;bFound=true;
   Out.bHouse=true;Out.Index=I;Out.Name=Doors->NameOf(I);Out.Key=TEXT("ff:olddoor:")+Out.Name;Out.Title=House?House->Title:FString(TEXT("the old house"));Out.Area=Area;
   Out.Middle=Middle;Out.Out=Toward(Middle,Doors->Normal(I));
  }
  return bFound;
 }
 if(Area==TEXT("road")||Area==TEXT("diner"))
 {
  FString Id;FVector Middle,Normal;
  if(!S47CalicheAccess::RoomAt(Feet,Id)||S47CalicheAccess::IsOpen(Id)||!S47CalicheAccess::DoorAt(Id,Middle,Normal))return false;
  if(FVector2D::Distance(FVector2D(Middle.X,Middle.Z),FVector2D(Feet.X,Feet.Z))<1.2)return false;
  const FS47EventPlace* Room=Places?Places->WithDoor(Id):nullptr;
  if(Places&&(!Room||!Room->Has(ES47PlaceTrait::Door)))return false;
  Out.bHouse=false;Out.Index=INDEX_NONE;Out.Name=Id;Out.Key=TEXT("access:")+Id;Out.Title=Room?Room->Title:S47DoorVisitLocal::TownTitle(Id);Out.Area=Area;
  Out.Middle=Middle;Out.Out=Toward(Middle,Normal);
  return true;
 }
 return false;
}

void FS47DoorVisit::StepDraw(double Dt)
{
 FTarget Found;
 const bool bHere=M->Freeform->TheirHoursNow(M->Freeform->Clock())&&Owner&&Owner->DoorQuiet()&&M->Freeform->Taken&&M->Freeform->Taken->Indoors()&&FindDoor(Found);
 InsideFor=bHere?InsideFor+Dt:0;
 if(!bHere)return;
 const int32 Day=M->Freeform->Day();
 // The first night is not drawn at all: nothing is spent of it, and nothing comes.
 if(Day<FirstDay)return;
 if(RollDay!=Day)
 {
  if(InsideFor<25)return;
  RollDay=Day;
  const float Roll=TestRoll>=0?TestRoll:FMath::FRand();
  bTonight=MayCome(Day,LastDay)&&Roll<ComeChance;WaitFor=InsideFor+FMath::FRandRange(4.f,35.f);
  UE_LOG(LogTemp,Display,TEXT("S47: door visit draw for day %d: %d, in %.0f s"),Day,bTonight,WaitFor-InsideFor);
  M->RequestAutosave();
 }
 if(!bTonight||InsideFor<WaitFor)return;
 // The draw stands: it waits its turn, and a night's rule that stops it for good ends with the night.
 if(M->Rhythm&&!M->Rhythm->Allow(TEXT("door_visit")))return;
 bTonight=false;LastBefore=LastDay;LastDay=Day;
 BeginAt(Found);
}

void FS47DoorVisit::BeginAt(const FTarget& Found)
{
 Target=Found;Now=EStage::Trying;T=AllT=0;SoundIn=0;HushIn=0;Noticed=0;Shake=0;Beat=0;bBodyOut=bSawBody=bLightComes=bSawOpen=bCutShort=false;
 if(M->Web)Sight.Init(M);
 // A door in town gives its height in the street's terms; the room can stand well above that.
 if(M->Player&&FMath::Abs(Target.Middle.Y-M->Player->FloorY)>1.5)Target.Middle.Y=M->Player->FloorY;
 if(M->Rhythm)M->Rhythm->Began(TEXT("door_visit"));
 if(auto* Places=M->Freeform?M->Freeform->EventPlaces.Get():nullptr)
 {
  const FS47EventPlace* P=Places->WithDoor(Target.bHouse?FString(TEXT("house")):Target.Name);
  if(P)Places->Used(P->Id,TEXT("door_visit"));
  FS47EventPlaces::LogPick(TEXT("door_visit"),P,FString::Printf(TEXT("the door %s"),*Target.Key));
 }
 UE_LOG(LogTemp,Display,TEXT("S47: door visit starts at %s, door %s, %.1f m off, at clock %.0f"),*Target.Title,*Target.Key,FVector::Dist(Target.Middle,FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z)),M->Freeform->Clock());
 M->RequestAutosave();
}

bool FS47DoorVisit::DoorHere() const
{
 FTarget Found;
 return M&&M->Player&&M->Web&&M->Freeform&&FindDoor(Found);
}

FString FS47DoorVisit::DevStart()
{
 if(!M||!M->Freeform||!M->Player||!M->Web)return TEXT("no Freeform world");
 if(Active())return TEXT("one is under way");
 if(!Owner||!Owner->DoorQuiet())return TEXT("something else is going on");
 if(!M->Freeform->Taken||!M->Freeform->Taken->Indoors())return TEXT("it needs a roof over you");
 FTarget Found;
 if(!FindDoor(Found))return TEXT("no shut outside door within 12 m in a room it knows: the old house, or a room in town");
 LastBefore=LastDay;LastDay=M->Freeform->Day();bTonight=false;
 BeginAt(Found);
 return FString();
}

void FS47DoorVisit::DevEnd()
{
 if(!Active())return;
 if(DoorOpenNow())SetDoorQuietly(false);
 LetGo();Now=EStage::None;bLightComes=false;
}

bool FS47DoorVisit::DoorOpenNow() const
{
 if(Target.bHouse)return M&&M->Freeform&&M->Freeform->HouseDoors&&M->Freeform->HouseDoors->IsOpen(Target.Index);
 return S47CalicheAccess::IsOpen(Target.Name);
}

void FS47DoorVisit::PoseDoor(double K)
{
 if(Target.bHouse){if(M->Freeform->HouseDoors)M->Freeform->HouseDoors->PoseOver(Target.Index,K);}
 else S47CalicheAccess::PoseDoor(Target.Name,K);
}

void FS47DoorVisit::SetDoorQuietly(bool bOpen)
{
 if(Target.bHouse){if(M->Freeform->HouseDoors)M->Freeform->HouseDoors->SetOpenQuietly(Target.Index,bOpen);}
 else S47CalicheAccess::SetOpenQuietly(Target.Name,bOpen);
}

void FS47DoorVisit::Handle(float Gain)
{
 Shake=1;
 if(M->Audio)M->Audio->Sfx(M->Audio->Or(TEXT("door_latch"),TEXT("switch")),FS47Sound{.Gain=Gain,.Rate=FMath::FRandRange(.8f,1.05f),.At=Target.Middle+FVector(0,1,0)});
}

void FS47DoorVisit::BeginOpening()
{
 Now=EStage::Opening;T=0;
 SetDoorQuietly(true);
 if(M->Audio)M->Audio->Sfx(TEXT("doorCreak0"),FS47Sound{.Gain=.85f,.Rate=.6f,.At=Target.Middle+FVector(0,1,0)});
 // A night that has already lit a room may light this one, and it has its own way out. Under a roof only, so it warns.
 const double Clock=M->Freeform->Clock();
 bLightComes=Owner&&Owner->HasFloodRoom()&&Clock-Owner->FloodRoomClock()<6*3600.&&M->Freeform->Taken&&M->Freeform->Taken->Indoors();
 if(!bLightComes)PutBodyOut();
}

void FS47DoorVisit::PutBodyOut()
{
 BodyFeet=Target.Middle+Target.Out*.62;BodyFeet.Y=GroundNear(BodyFeet);bBodyOut=true;
 if(M->Night)
 {
  const auto In=M->Night->GlimpseIn(true);
  BodyYaw=FMath::Atan2(In.Cam.Pos.X-BodyFeet.X,In.Cam.Pos.Z-BodyFeet.Z);
 }
 if(Owner)Owner->ShowDoorBody(BodyFeet,BodyYaw,false,AllT);
}

bool FS47DoorVisit::DoorInSight() const
{
 if(!M||!M->Night)return false;
 const auto In=M->Night->GlimpseIn(true);
 // A little on the player's side of the leaf, so that the door itself does not count as what hides it.
 const FVector Spot=Target.Middle-Target.Out*.25+FVector(0,1.1,0);
 const FVector N=In.Cam.Project(Spot);
 return N.Z<1&&FMath::Abs(N.X)<.96&&FMath::Abs(N.Y)<.96&&Sight.ClearSight(In.Cam.Pos,Spot);
}

// Something bigger came first. Never seen open: it was never here, the door is shut as it was, and the night is handed back.
// Seen open, it cannot have been nothing: the door stays, what stands there goes by itself, and the record says how little was seen.
void FS47DoorVisit::GiveWay()
{
 const FVector2D Feet2D(M->Player->Pos.X,M->Player->Pos.Z);
 const bool bInWay=FVector2D::Distance(Feet2D,FVector2D(Target.Middle.X,Target.Middle.Z))<1.4;
 if(DoorOpenNow()&&(bSawOpen||bInWay))
 {
  UE_LOG(LogTemp,Display,TEXT("S47: door visit cut short in view, the door stays open"));
  bCutShort=true;bLightComes=false;
  PoseDoor(-1); // the rest of its swing, by itself
  if(bBodyOut){Now=EStage::Leaving;T=0;}
  else Finish(EEnding::Walked);
  return;
 }
 UE_LOG(LogTemp,Display,TEXT("S47: door visit given up for something bigger, nobody saw the door move"));
 if(DoorOpenNow())SetDoorQuietly(false);
 LetGo();Now=EStage::None;bLightComes=false;
 LastDay=LastBefore;
 // It may try once more tonight, when things are quiet again.
 const int32 Day=M->Freeform->Day();
 if(RetryDay!=Day){RetryDay=Day;bTonight=true;WaitFor=FMath::FRandRange(40.f,90.f);}
 M->RequestAutosave();
}

void FS47DoorVisit::StepVisit(double Dt)
{
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 const FString AreaNow=M->Web?M->Web->Area():FString();
 const bool bSameArea=AreaNow==Target.Area||((AreaNow==TEXT("road")||AreaNow==TEXT("diner"))&&(Target.Area==TEXT("road")||Target.Area==TEXT("diner")));
 const bool bGone=!bSameArea||(M->World&&M->World->bDriving)||FVector::Dist(Feet,Target.Middle)>45;
 // Gone from there: it has the door to itself, and nobody is left to see it. The door is found open, later.
 if(bGone)
 {
  if(Now==EStage::Trying||Now==EStage::Waiting)SetDoorQuietly(true);
  bLightComes=false; // nobody there for it to come to
  if(Now!=EStage::Holding)Finish(EEnding::Walked);else Finish(EEnding::Held);
  return;
 }
 if(Now==EStage::Opening||Now==EStage::Standing||Now==EStage::Leaving)bSawOpen|=DoorInSight();
 // Already going is going: that is over in a moment whatever else has come.
 if(Owner&&!Owner->DoorQuiet()&&Now!=EStage::Leaving)
 {
  GiveWay();
  if(Now==EStage::None)return;
 }
 T+=Dt;AllT+=Dt;
 Shake=FMath::Max(0.,Shake-Dt*3.5);
 // Everything else goes quiet so that this can be heard, a stretch at a time.
 HushIn-=Dt;
 if(HushIn<=0&&M->Audio&&M->Audio->Night.IsValid()){HushIn=3;M->Audio->Night->Hush(5,.05,.8,3);}
 const bool bCalm=M->Flow&&M->Flow->bCalmFlash;
 if(M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(!bCalm&&Now==EStage::Trying?.3:0.,TEXT("door"));
 switch(Now)
 {
 case EStage::Trying:
  // Three turns of the handle, a pause, three more. The leaf shifts in its frame each time.
  SoundIn-=Dt;
  if(SoundIn<=0){Handle(.85f);++Beat;SoundIn=Beat%3==0?FMath::FRandRange(1.6f,2.4f):.34f;}
  PoseDoor(.022*Shake*FMath::Abs(FMath::Sin(AllT*31.)));
  if(T>=TrySeconds){Now=EStage::Waiting;T=0;SoundIn=.6;PoseDoor(0);if(M->FieldRadio)M->FieldRadio->Talk(Target.Middle+Target.Out*.6+FVector(0,1.2,0));}
  break;
 case EStage::Waiting:
  // Nothing moves. Something small against the other side, over and over, until the door does something.
  SoundIn-=Dt;
  if(SoundIn<=0&&M->Audio){SoundIn=FMath::FRandRange(1.1f,1.7f);M->Audio->Sfx(TEXT("stepConcrete0"),FS47Sound{.Gain=.3f,.Rate=.55f,.At=Target.Middle+Target.Out*.4+FVector(0,.3,0)});}
  if(T>=WaitSeconds)BeginOpening();
  break;
 case EStage::Holding:
  // It tries harder for a moment, then stops, and steps go away along the wall.
  if(T<1.6)
  {
   SoundIn-=Dt;if(SoundIn<=0){Handle(1.f);SoundIn=.24;}
   PoseDoor(.03*Shake*FMath::Abs(FMath::Sin(AllT*38.)));
  }
  else
  {
   PoseDoor(0);
   if(Beat<3&&T>=1.9+Beat*.5&&M->Audio)
   {
    const FVector Side(-Target.Out.Z,0,Target.Out.X);
    M->Audio->Sfx(TEXT("stepConcrete0"),FS47Sound{.Gain=.4f,.Rate=.8f,.At=Target.Middle+Target.Out*.7+Side*(1.+Beat*.8)});
    ++Beat;
   }
  }
  if(T>=HoldSeconds)Finish(EEnding::Held);
  break;
 case EStage::Opening:
 {
  const double U=FMath::Clamp(T/OpenSeconds,0.,1.);
  PoseDoor(U*U*(3.-2.*U));
  if(bBodyOut&&Owner){Owner->ShowDoorBody(BodyFeet,BodyYaw,false,AllT);bSawBody|=Owner->DoorBodyRendered()&&U>.35;}
  if(T>=OpenSeconds)
  {
   if(bLightComes)
   {
    // Begun first, so that the record says a light came only if one is coming. If not, the door has what any other has.
    if(M->Freeform->Taken&&M->Freeform->Taken->Begin(ES47Taker::Roswell,Target.Title)){Finish(EEnding::Walked);return;}
    bLightComes=false;PutBodyOut();
   }
   PoseDoor(1);Now=EStage::Standing;T=0;Noticed=0;
  }
  break;
 }
 case EStage::Standing:
 case EStage::Leaving:
 {
  // Shut on it: what is behind a shut door is nobody's business.
  if(!DoorOpenNow()||!M->Night){Finish(EEnding::Walked);return;}
  const auto In=M->Night->GlimpseIn(true);
  const FVector Eye=BodyFeet+FVector(0,1.05,0);
  const FVector N=In.Cam.Project(Eye);
  const bool bInView=N.Z<1&&FMath::Abs(N.X)<.96&&FMath::Abs(N.Y)<.96;
  const double Off=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(In.Cam.Forward,(Eye-In.Cam.Pos).GetSafeNormal()),-1.,1.)));
  // Through an open door its own frame and leaf do not count as what hides the one standing in it (the cinema's glass doors keep their collision shut).
  const bool bClear=Sight.ClearSight(In.Cam.Pos,Eye)||(DoorOpenNow()&&Sight.ClearSight(In.Cam.Pos,Target.Middle-Target.Out*.25+FVector(0,1.1,0)));
  if(Owner)bSawBody|=bInView&&bClear&&Owner->DoorBodyRendered();
  if(Now==EStage::Standing)
  {
   // Restless while it waits to be seen.
   if(Owner)Owner->ShowDoorBody(BodyFeet,BodyYaw,false,AllT,ES47Clip::Frustrated);
   Noticed=bInView&&bClear&&Off<20?Noticed+Dt:FMath::Max(0.,Noticed-Dt*.4);
   // Looked at, it goes. Never looked at, it goes when nobody can see it go.
   if(Noticed>.45||T>30){Now=EStage::Leaving;T=0;if(M->Audio)M->Audio->Sfx(TEXT("stepConcrete0"),FS47Sound{.Gain=.25f,.Rate=.8f,.At=BodyFeet});}
  }
  else
  {
   // A body with clips takes being seen badly, for a moment, facing the one who looked. Then it goes like the other.
   if(Owner&&Owner->DoorBodyHasClips()&&T<LookedBackSeconds)Owner->ShowDoorBody(BodyFeet,BodyYaw,false,AllT,ES47Clip::Angry);
   else
   {
    BodyFeet+=Target.Out*(1.9*Dt);BodyFeet.Y=FMath::FInterpTo(BodyFeet.Y,GroundNear(BodyFeet),Dt,10.);
    BodyYaw=FMath::Atan2(Target.Out.X,Target.Out.Z);
    if(Owner)Owner->ShowDoorBody(BodyFeet,BodyYaw,true,AllT);
   }
   if(!bInView||!bClear||T>6)Finish(EEnding::Walked);
  }
  break;
 }
 default:
  break;
 }
}

bool FS47DoorVisit::Pressed(const FString& DoorKey)
{
 if(Now==EStage::None||DoorKey!=Target.Key||!M||!M->Freeform)return false;
 switch(Now)
 {
 case EStage::Trying:
  Now=EStage::Holding;T=0;Beat=0;SoundIn=.2;
  if(M->Audio)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.8f,.At=Target.Middle+FVector(0,1,0)});
  return true;
 case EStage::Holding:
  return true;
 case EStage::Waiting:
  // The sound stops as the door moves, and there is nothing there. It opens the ordinary way.
  LetGo();
  if(Target.bHouse){if(M->Freeform->HouseDoors)M->Freeform->HouseDoors->Use(Target.Index);}
  else S47CalicheAccess::UseDoor(Target.Name);
  Finish(EEnding::Opened);
  return true;
 case EStage::Opening:
  // Pushed shut before it got any further: that counts as holding it. Someone in the doorway cannot shut it: the door says
  // so, and this goes on as if nothing had been pressed.
  if(Target.bHouse){if(M->Freeform->HouseDoors)M->Freeform->HouseDoors->Use(Target.Index);}
  else S47CalicheAccess::UseDoor(Target.Name);
  if(DoorOpenNow())return true;
  bLightComes=false;
  Finish(EEnding::Held);
  return true;
 default:
  return false; // Open now, and an ordinary door.
 }
}

FString FS47DoorVisit::LabelFor(const FString& DoorKey) const
{
 if(Now==EStage::None||DoorKey!=Target.Key)return FString();
 switch(Now)
 {
 case EStage::Trying:return TEXT("Hold the door");
 case EStage::Holding:return TEXT("Holding the door");
 case EStage::Waiting:return TEXT("Open the door");
 case EStage::Opening:return TEXT("Push it shut");
 default:return FString();
 }
}

FString FS47DoorVisit::DoorKeyOf(const FString& Usable) const
{
 if(Now==EStage::None||Usable.IsEmpty())return FString();
 // In town the street's old proxy (town:cinema) stands around the same door and opens it without asking this.
 return Usable==Target.Key||(!Target.bHouse&&S47CalicheAccess::SameDoor(Usable,Target.Name))?Target.Key:FString();
}

void FS47DoorVisit::Finish(EEnding Ending)
{
 LetGo();
 Now=EStage::None;
 if(M&&M->Rhythm)M->Rhythm->Ended(TEXT("door_visit"));
 PutMarks(Ending);
 if(!M||!M->Freeform)return;
 const FString Key=FString::Printf(TEXT("door-visit-%d"),M->Freeform->Day());
 const FString Where=S47DoorVisitLocal::Upper(Target.Title);
 FString What;
 if(Ending==EEnding::Held)What=TEXT("Something tried the handle of a shut door and waited. I held the door and it stopped, and steps went away along the wall. This records what I heard, not what it was.");
 else if(Ending==EEnding::Opened)What=TEXT("Something tried the handle of a shut door and waited. I opened it. Nobody on the threshold, and the sound stopped as the door moved. This records what I found, not what it was.");
 else if(bLightComes)What=M->Freeform->Taken&&M->Freeform->Taken->Stage()==FS47Taken::EStage::Flood
  ?TEXT("Something tried the handle of a shut door. I did not hold it. The door came open by itself and a light came after it. This records what I saw, not what it was.")
  :TEXT("Something tried the handle of a shut door. I did not hold it. The door came open by itself, and then there was someone on the roof. This records what I heard, not what it was.");
 else if(bCutShort)What=bSawBody
  ?TEXT("Something tried the handle of a shut door. I did not hold it. The door came open by itself and something stood there, and then there was too much else going on and it was gone. This records what I saw, not what it was.")
  :TEXT("Something tried the handle of a shut door. I did not hold it. The door came open by itself, and then there was too much else going on to see what it did. This records what I saw, not what it was.");
 else if(bSawBody)What=TEXT("Something tried the handle of a shut door. I did not hold it. The door came open by itself, slowly, and something stood there until I looked straight at it. This records what I saw, not what it was.");
 else What=TEXT("Something tried the handle of a shut door. I did not hold it and I did not stay to see what it did. This records what I heard, not what it was.");
 if(!M->Freeform->HasFinding(Key))M->Freeform->RecordEvidence(Key,FString::Printf(TEXT("%s, in the small hours. %s"),*Where,*What));
 UE_LOG(LogTemp,Display,TEXT("S47: door visit over at %s, ending %d, seen %d, %d marks"),*Target.Title,int32(Ending),bSawBody,TraceAt.Num());
 M->RequestAutosave();
}

bool FS47DoorVisit::Step(double Dt)
{
 if(!M||!M->Freeform||!M->Web||!M->Player)return false;
 // Morning takes what it left, and the marks are shown only where they are.
 if(TraceUntil>0&&M->Freeform->Clock()>=TraceUntil){Trace=EEnding::None;TraceAt.Reset();TraceUntil=0;ShowMarks(false);M->RequestAutosave();}
 const bool bHere=MarksHere();
 if(bHere!=bShown){LayoutMarks();ShowMarks(bHere);}
 if(Now==EStage::None)
 {
  StepDraw(Dt);
  return Now!=EStage::None;
 }
 StepVisit(Dt);
 return true;
}

bool FS47DoorVisit::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s door visit: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 Check(MayCome(5,0),TEXT("a case that has never had one may have one"));
 Check(!MayCome(1,0)&&MayCome(2,0),TEXT("nothing at the door on the first night"));
 Check(!MayCome(5,5)&&!MayCome(6,5)&&MayCome(7,5),TEXT("at most one in two nights"));
 FS47DoorVisit Old(nullptr,nullptr);Old.Begin(nullptr);
 Check(!Old.Active()&&Old.Save()->Values.Num()==0&&Old.MarksOut()==0,TEXT("an older case has none under way and nothing to save"));
 const auto Saved=S47J::New();const auto Left=S47J::New();
 S47J::Set(Left,TEXT("kind"),int32(EEnding::Held));S47J::Set(Left,TEXT("area"),TEXT("saro"));S47J::Set(Left,TEXT("title"),TEXT("the old house"));S47J::Set(Left,TEXT("day"),4);S47J::Set(Left,TEXT("until"),360000.);
 S47J::Set(Left,TEXT("at"),TArray<double>{1.,2.,3.,4.,5.,6.,7.,8.,9.});
 S47J::Set(Saved,TEXT("trace"),Left);S47J::Set(Saved,TEXT("rollDay"),4);S47J::Set(Saved,TEXT("lastDay"),4);
 FS47DoorVisit Copy(nullptr,nullptr);Copy.Begin(S47J::Read(S47J::Write(Saved)));
 Check(Copy.MarksOut()==3&&Copy.TraceKind()==EEnding::Held&&Copy.LastDayOf()==4&&Copy.RollDayOf()==4,TEXT("the marks and the draw survive Continue"));
 FS47DoorVisit Again(nullptr,nullptr);Again.Begin(Copy.Save());
 Check(S47J::Write(Copy.Save())==S47J::Write(Again.Save()),TEXT("saving again changes nothing"));
 int32 OuterDoors=0;for(const auto& Plan:S47FarmHouse::DoorPlans())if(S47DoorVisitLocal::Outer(Plan.Name))++OuterDoors;
 Check(OuterDoors==2&&!S47DoorVisitLocal::Outer(TEXT("hallW0"))&&!S47DoorVisitLocal::Outer(TEXT("bath0")),TEXT("in the old house it comes only to the two doors to the outside"));
 Check(S47DoorVisitLocal::MarksText(EEnding::Held)!=S47DoorVisitLocal::MarksText(EEnding::Opened)&&S47DoorVisitLocal::MarksText(EEnding::Opened)!=S47DoorVisitLocal::MarksText(EEnding::Walked),TEXT("each ending has its own marks and its own text"));
 return OK;
}
