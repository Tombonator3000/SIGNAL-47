#include "S47FarmSite.h"
#include "S47AnimalWarn.h"
#include "S47Animals.h"
#include "S47EventScript.h"
#include "S47Noise.h"
#include "S47EventPlaces.h"
#include "S47Rhythm.h"
#include "S47Taken.h"
#include "S47NightRhythm.h"
#include "S47AlienKinds.h"
#include "S47Openings.h"
#include "S47FieldLife.h"
#include "S47Aftermath.h"
#include "S47PaleBody.h"
#include "S47Three.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"
#include "S47Story.h"
#include "S47FreeformDirector.h"
#include "S47World.h"
#include "S47Sky.h"
#include "S47Sneak.h"
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
#include "S47Drive.h"
#include "S47FieldRadio.h"
#include "S47IndoorDread.h"
#include "S47Ambience.h"
#include "S47Flow.h"
#include "S47OpenLand.h"
#include "S47Visitors.h"
#include "S47IdleLure.h"
#include "S47NightTiers.h"
#include "S47Attention.h"
#include "S47Truck.h"
#include "S47EngineSound.h"
#include "S47WeekCase.h"
#include "S47Happenings.h"
#include "S47Script.h"
#include "S47NightTiers.h"
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

// The full tier of the first weeks (S47NightTiers) until six: a session's own night (FS47FreeformSession::TheirHoursNow) creeps in the fourth.
bool FS47FarmSite::TheirHours(double Clock)
{
 const double Hour=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;
 return S47NightTiers::Allows(ES47Allowed::Full,Hour,1)&&Hour<FS47NightRhythm::EndHour;
}

double FS47FarmSite::VisitDeadline(double StartClock)
{
 constexpr double Day=86400,Dawn=FS47NightRhythm::EndHour*3600;
 const double Start=FMath::Max(0.,StartClock),Days=FMath::FloorToDouble(Start/Day);
 const double NextDawn=Start-Days*Day<Dawn?Days*Day+Dawn:(Days+1)*Day+Dawn;
 return FMath::Min(NextDawn,Start+MaxVisitHours*3600);
}

FString FS47FarmSite::FindingId() const
{
 if(Place==TEXT("field"))return FString::Printf(TEXT("field_night%d"),M&&M->Freeform?M->Freeform->Day():0);
 return Place==TEXT("farm")?FString(TEXT("farm_trace")):TEXT("lights_trace_")+Place;
}
bool FS47FarmSite::OutOnFoot(const FString& Area,double FromStation,bool bDriving,bool bRoof)
{
 if(bDriving||bRoof)return false;
 return Area==TEXT("road")||Area==TEXT("diner")||(Area==TEXT("saro")&&FromStation>=220); // As RecordEvidence tells the station's ground from the rest.
}
float FS47FarmSite::FieldChanceFor(int32 Night,int32 Misses,bool bLit)
{
 if(Night<2)return 0.f;
 return FMath::Min(.9f,FS47NightRhythm::FieldChance(Night)+FieldMissBonus*float(FMath::Max(0,Misses))+(bLit?.25f:0.f));
}
void FS47FarmSite::StepFieldDraw(double Dt)
{
 auto& F=*M->Freeform;
 if(!F.RulesLive()||bField||(Visit>=1&&Visit<=3)||!M->Player||!M->Web||!M->World||!F.Taken)return;
 const double Clock=F.Clock();const int32 Night=FS47FreeformSession::NightOf(Clock);
 if(FieldNight==Night||Night<2||!F.TheirHoursNow(Clock)||Clock<CalmUntil){FieldDwell=0;return;}
 const auto G=M->Night?M->Night->Glimpses:TSharedPtr<FS47Glimpses>();
 const FVector2D Foot(M->Player->Pos.X,M->Player->Pos.Z);
 const bool bAtMarks=Armed()&&Here()&&FVector2D(Foot.X-Middle.X,Foot.Y-Middle.Z).Size()<140; // Those marks have their own visit.
 const bool bQuiet=G&&F.Director&&F.Director->ActiveEvent().IsEmpty()&&G->Individuals().IsEmpty()&&!F.Taken->Busy()&&!(F.IndoorDread&&F.IndoorDread->DoorActive())&&!FS47WeekCase::HoldsFarm(M);
 if(bAtMarks||!bQuiet||M->ModalOpen()||!OutOnFoot(M->Web->Area(),Foot.Size(),M->World->bDriving,F.Taken->Indoors())){FieldDwell=0;return;}
 // NIGHT-20261110: not on the big night's own night, and not ahead of the landing tonight's slip is out for: the night's one pointing-out
 // comes first (the visit held the slot from 02:14 to morning and the big night and the pointed landing never came, play n14).
 if(F.Happenings)
 {
  FString LeadEvent,LeadTitle;double LeadX=0,LeadZ=0;bool bLeadAhead=false;
  const bool bBig=F.Happenings->BigOwns(Night);
  if(bBig||(F.Happenings->OutTonight(Night,LeadEvent,LeadTitle,LeadX,LeadZ,bLeadAhead)&&bLeadAhead&&LeadEvent==TEXT("landing")))
  {
   if(FieldHeldNight!=Night){FieldHeldNight=Night;UE_LOG(LogTemp,Display,TEXT("S47: field visit: waits tonight, night %d (%s)"),Night,bBig?TEXT("the big night's own"):TEXT("the landing a slip may point at comes first"));}
   FieldDwell=0;return;
  }
 }
 // Active seconds, as their hours are: F10's natural night at x30 runs the hours by in forty real seconds, and a real-time minute
 // of standing out there never came (krit9field: nine nights in the open, no draw at all).
 FieldDwell+=Dt*(F.Director?F.Director->TestPaceNow():1.);if(FieldDwell<FieldDwellSeconds)return;
 // Before the draw: a night's one is not spent on a moment the rules would not let it come.
 if(M->Rhythm&&!M->Rhythm->Allow(TEXT("field_visit")))return;
 // One draw a night, the game's own: going indoors and out again does not throw it twice.
 FieldNight=Night;FieldDwell=0;
 const float Roll=FieldRoll>=0?FieldRoll:FS47NightRhythm::Roll(F.Seed(),Night,0x4649454cu);FieldRoll=-1;
 M->RequestAutosave();
 const bool bLit=F.TruckLamp&&F.TruckLamp->LitTonight(Night);
 const float Chance=FieldChanceFor(Night,FieldMisses,bLit);
 if(Roll>=Chance){++FieldMisses;UE_LOG(LogTemp,Display,TEXT("S47: field visit: nothing comes tonight, night %d (roll %.2f of %.2f, %d out since the last)"),Night,Roll,Chance,FieldMisses);return;}
 FieldMisses=0;
 if(M->Rhythm)M->Rhythm->Began(TEXT("field_visit"));
 Home={Place,Title,AreaId,Middle,Visit};bField=true;
 const FString Area=M->Web->Area();
 Place=TEXT("field");AreaId=Area;Middle=FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 Title=Area==TEXT("saro")?TEXT("the open ground past the SARO fence"):Area==TEXT("diner")?TEXT("the edge of Caliche"):TEXT("the shoulder of the county road");
 // Wherever the player has walked out to: named for the nearest place on the list, which has had something tonight.
 if(F.EventPlaces)
 {
  const FS47EventPlace* Near=F.EventPlaces->Nearest(500);
  if(Near){Title=FString::Printf(TEXT("the open ground near %s"),*Near->Title);F.EventPlaces->Used(Near->Id,TEXT("field_visit"));}
  FS47EventPlaces::LogPick(TEXT("field_visit"),Near,TEXT("where the player is on foot"));
 }
 Visit=1;VisitUntil=0;VisitAge=VisitDwell=0;bVisitSeen=false;bVisitTaken=false;
 VisitWho=PickWho(GroundKind(),FMath::Fmod(Clock,86400.)/3600.,FMath::FRand());
 // D48-20261110 (D46-17): leaned toward the night's own kind (S47AlienKinds), as the take on the receiver sounded it.
 VisitWho=S47AlienKinds::LeanVisit(VisitWho,S47AlienKinds::NightLean(F.Seed(),Night),FMath::Fmod(Clock,86400.)/3600.,FS47NightRhythm::Roll(F.Seed(),Night,0x4c45414eu));
 UE_LOG(LogTemp,Display,TEXT("S47: field visit begins from the night's own draw, kind %d at %s, night %d"),VisitWho,*Title,Night);
}
void FS47FarmSite::RestoreHome()
{
 if(!bField)return;
 bField=false;Place=Home.Place;Title=Home.Title;AreaId=Home.Area;Middle=Home.Middle;Visit=ClearAt>0?Home.Visit:4;VisitDwell=0;
}
bool FS47FarmSite::Hours(double Clock) const{return M&&M->Freeform?M->Freeform->TheirHoursNow(Clock):TheirHours(Clock);}
FString FS47FarmSite::SecondCallLine(int32 Index,const FString& About)
{
 // Written as dispatch would pass them on: nobody at the other end knows anything.
 switch(FMath::Clamp(Index,0,SecondCallLines-1))
 {
  case 0:return FString::Printf(TEXT("A second call about %s. The caller says the lights have not gone, and won't go out to look."),*About);
  case 1:return FString::Printf(TEXT("Another call about %s. Same man. He says they are lower now."),*About);
  case 2:return FString::Printf(TEXT("Somebody else called about %s. She wants to know who is working out there at this hour."),*About);
  default:return FString::Printf(TEXT("A call about %s. Could be nothing. The man had been drinking and said so."),*About);
 }
}
void FS47FarmSite::StepEmptyTip()
{
 // A calm night, no marks anywhere, and somebody rings anyway. Drive out and there is nothing there.
 auto& F=*M->Freeform;
 const double Clock=F.Clock();const int32 Night=FS47FreeformSession::NightOf(Clock);
 if(EmptyTipNight==Night||!F.RulesLive()||!F.QuietNight(Night)||!F.NightStarted(Clock))return;
 EmptyTipNight=Night;
 if(FS47NightRhythm::Roll(F.Seed(),Night,0x54495053u)>=.4f)return;
 // About somewhere on the list, by the game's seed and the night: the farm no likelier than a wash out on the land.
 FS47PlaceNeed Need;Need.Kind=TEXT("empty_tip");Need.Any=ES47PlaceTrait::Open|ES47PlaceTrait::Town|ES47PlaceTrait::Farm|ES47PlaceTrait::Lot;Need.MinM=150;Need.FarShare=.5f;
 const FS47PlacePick Pick=F.EventPlaces?F.EventPlaces->Pick(Need):FS47PlacePick();
 const FString About=Pick.IsSet()?F.EventPlaces->TitleOf(Pick.Id):FString(TEXT("the Alderete place"));
 const FString Tip=SecondCallLine(int32(FS47NightRhythm::Roll(F.Seed(),Night,0x4c494e45u)*SecondCallLines)%SecondCallLines,About);
 FS47FieldRadio::Report(M,Tip); // CB-WALKIE-20261108: the CB at hand, else Dale passes it on over the walkie
 FS47EventPlaces::LogPick(TEXT("empty_tip"),F.EventPlaces&&Pick.IsSet()?F.EventPlaces->Find(Pick.Id):nullptr,TEXT("a call on a calm night: nothing there"));
 M->RequestAutosave();
}
int32 FS47FarmSite::PickWho(const FString& InPlace,double Hour,float Roll)
{
 const bool bFarm=InPlace==TEXT("farm"),bOpen=InPlace==TEXT("field");
 const float Crab=bOpen?.45f:bFarm?.30f:.12f,Tall=Hour>=3?(bOpen?.25f:bFarm?.20f:.15f):0.f;
 return Roll<Crab?1:Roll<Crab+Tall?2:0;
}

bool FS47FarmSite::Raked() const
{
 const FS47FreeformSession* F=M&&M->Freeform?M->Freeform.Get():nullptr;
 return F&&Armed()&&!bField&&F->Visitors&&F->EventPlaces&&F->Visitors->Cleared(Place,F->EventPlaces->LastNight(Place));
}
const FS47EventPlace* FS47FarmSite::PlaceOnList() const
{
 return M&&M->Freeform&&M->Freeform->EventPlaces?M->Freeform->EventPlaces->Find(Place):nullptr;
}
bool FS47FarmSite::OpenGround() const
{
 if(Place==TEXT("field"))return true;
 const FS47EventPlace* P=PlaceOnList();
 return P&&P->Has(ES47PlaceTrait::Open)&&!P->HasAny(ES47PlaceTrait::Town);
}
FString FS47FarmSite::GroundKind() const
{
 // What the ground is, not which place: far open land is where the crab comes, stock ground next, town least.
 if(Place==TEXT("field"))return TEXT("field");
 const FS47EventPlace* P=PlaceOnList();
 if(!P)return Place;
 if(P->Has(ES47PlaceTrait::Open)&&P->Has(ES47PlaceTrait::Far)&&!P->HasAny(ES47PlaceTrait::Farm|ES47PlaceTrait::Town))return TEXT("field");
 if(P->HasAny(ES47PlaceTrait::Farm)||(P->Has(ES47PlaceTrait::Open)&&!P->HasAny(ES47PlaceTrait::Town)))return TEXT("farm");
 return TEXT("town");
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
 // Wide on open ground, as in a pasture; tighter where the open ground is a street front.
 const bool bWide=OpenGround();const double Ring=bWide?4.5:2.4,K=bWide?1.:.6;
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
 Stop();FieldNight=Saved.IsValid()?FMath::Max(0,S47J::Int(Saved,TEXT("fieldNight"))):0;FieldDwell=0;
 FieldMisses=Saved.IsValid()?FMath::Clamp(S47J::Int(Saved,TEXT("fieldMisses")),0,20):0;
 if(!M||!IsValid(M->Web))return;
 if(Scene.Get()!=M->Web){bBuilt=false;bBound=false;Marks.Reset();Spots.Reset();Haze.Reset();VisitBody.Reset();CrabBody.Reset();TallBody.Reset();bBodyOut=false;SpotSaro={};SpotRoad={};}
 Scene=M->Web;Life=MakeShared<uint8>(0);bActive=true;bBound=false;
 ClearAt=Saved.IsValid()?S47J::Num(Saved,TEXT("clearAt")):0;SoundIn=20;
 Visit=Saved.IsValid()&&Saved->HasField(TEXT("visit"))?S47J::Int(Saved,TEXT("visit")):4;VisitAge=VisitDwell=0;bNudged=S47J::Bool(Saved,TEXT("nudged"));bFalseCall=S47J::Bool(Saved,TEXT("falseCall"));EmptyTipNight=Saved.IsValid()?FMath::Max(0,S47J::Int(Saved,TEXT("emptyTip"))):0;bStallSet=false;HushIn=0;
 bCheckedEmpty=S47J::Bool(Saved,TEXT("checkedEmpty"));bYielded=S47J::Bool(Saved,TEXT("yielded"));bFigureAsked=false;FigureIn=0;
 // EVIDENCE-DOSSIER slice 2: who came to the marks (absent in an older save: nobody known)
 MarksWho=Saved.IsValid()&&Saved->HasField(TEXT("marksBy"))?FMath::Clamp(S47J::Int(Saved,TEXT("marksBy")),-1,2):-1;MarksWhoNight=Saved.IsValid()?FMath::Max(0,S47J::Int(Saved,TEXT("marksByNight"))):0;
 bAnimalsWent=bAnimalsTagged=false;bDogsHushed=true;AnimalsWho=-1;
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
 if(bLive)EndLure();
 FrostTruck=nullptr;LureLampTruck=nullptr;Lure.Reset();
 if(bLive)HideBody();
 bBodyOut=false;
 RestoreHome();
 Visit=4;VisitUntil=0;bActive=false;Life.Reset();Show(false);
}

S47J::FObj FS47FarmSite::Save() const
{
 const auto O=S47J::New();
 if(EmptyTipNight>0)S47J::Set(O,TEXT("emptyTip"),EmptyTipNight);
 if(bFalseCall&&ClearAt>0)S47J::Set(O,TEXT("falseCall"),true);
 if(FieldNight>0)S47J::Set(O,TEXT("fieldNight"),FieldNight);
 if(FieldMisses>0)S47J::Set(O,TEXT("fieldMisses"),FieldMisses);
 // Saved in the middle of a field visit: the marks as they stand, and the visit is over on Continue.
 const FVector SavedAt=bField?Home.Middle:Middle;
 if(ClearAt>0&&bCheckedEmpty)S47J::Set(O,TEXT("checkedEmpty"),true);
 if(ClearAt>0&&bYielded)S47J::Set(O,TEXT("yielded"),true);
 if(ClearAt>0&&MarksWho>=0){S47J::Set(O,TEXT("marksBy"),MarksWho);S47J::Set(O,TEXT("marksByNight"),MarksWhoNight);} // EVIDENCE-DOSSIER slice 2
 if(ClearAt>0){S47J::Set(O,TEXT("clearAt"),ClearAt);S47J::Set(O,TEXT("visit"),(bField?Home.Visit:Visit)==0?0:4);S47J::Set(O,TEXT("place"),bField?Home.Place:Place);S47J::Set(O,TEXT("title"),bField?Home.Title:Title);S47J::Set(O,TEXT("area"),bField?Home.Area:AreaId);S47J::Set(O,TEXT("at"),TArray<double>{SavedAt.X,SavedAt.Y,SavedAt.Z});S47J::Set(O,TEXT("nudged"),bNudged);}
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
 bYielded=bCheckedEmpty=bFigureAsked=false;FigureIn=0;
 // Not every night's lights leave something behind that comes back for you.
 const float Comes=M&&M->Freeform?M->Freeform->SiteVisitChance():.6f;
 Visit=FMath::FRand()<Comes&&Clock>=CalmUntil?0:4;VisitDwell=0;
 // A second call means nothing by itself: about half the sites with nothing coming get one too.
 bFalseCall=Visit==4&&M&&M->Freeform&&M->Freeform->RulesLive()&&FMath::FRand()<.5f;
 // Marks made in the evening get word later, when something can come of it; made now, the tip was word enough.
 bNudged=Hours(Clock);
 // Open ground with nothing built on it takes the marks where the place is; anything with a front of its own, out from it.
 const FS47EventPlace* OnList=PlaceOnList();
 const bool bBare=OnList&&OnList->Prop.IsEmpty()&&OnList->Has(ES47PlaceTrait::Open)&&!OnList->HasAny(ES47PlaceTrait::Indoors|ES47PlaceTrait::Town);
 if(!bBare&&M&&M->Web&&M->Web->GetWorld())
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
 FS47EventPlaces::LogPick(TEXT("marks"),OnList,Visit==0?TEXT("something may come to them"):TEXT("nothing comes to them"));
}

void FS47FarmSite::SayNudge()
{
 bNudged=true;
 if(!M)return;
 const FString Tip=SecondCallLine(FMath::RandRange(0,SecondCallLines-1),Title);
 FS47FieldRadio::Report(M,Tip); // CB-WALKIE-20261108: the CB at hand, else Dale passes it on over the walkie
 M->RequestAutosave();
}

bool FS47FarmSite::WakeCall()
{
 if(!M||!M->Freeform||!WillStandAt(M->Freeform->Clock()))return false;
 SayNudge();return true;
}

void FS47FarmSite::Step(double Dt)
{
 StepFrost(Dt);
 if(!bActive||!M||!M->Freeform)return;
 // VISIT-DAWN-20261113: by the clock, not by its own seconds. A natural night at speed (or a long stand-off) ran the visit's
 // real-time patience across a whole day: begun 03:52, it still held the ground, and the take came, at 20:19 the next evening.
 if(Visit>=1&&Visit<=3)
 {
  const double Now=M->Freeform->Clock();
  if(VisitUntil<=0)VisitUntil=VisitDeadline(Now);
  else if(Now>=VisitUntil)
  {
   const auto Tk=M->Freeform->Taken;
   const bool bTaking=bVisitTaken||(Tk&&(Tk->Stage()==FS47Taken::EStage::Lift||Tk->Stage()==FS47Taken::EStage::Away));
   if(!bTaking)
   {
    UE_LOG(LogTemp,Display,TEXT("S47: %s visit over by the clock at %s, day %d %02d:%02d: it does not outlast the night"),bField?TEXT("field"):TEXT("site"),*Place,M->Freeform->Day(),int32(FMath::Fmod(Now,86400.)/3600.),int32(FMath::Fmod(Now,3600.)/60.));
    EndVisit(false);
   }
  }
 }
 if(ClearAt>0&&M->Freeform->Clock()>=ClearAt){ClearAt=0;if(bField)Home.Visit=4;Show(false);UE_LOG(LogTemp,Display,TEXT("S47: lights site cleared by morning"));return;}
 // MILITARY-20261108: they went over this ground first (S47Visitors): the marks are gone, and the raked circle stands where they were.
 if(Raked())
 {
  if(bShown)
  {
   Show(false);
   double RoadX=0,RoadZ=0;
   if(S47OpenLand::RoadOf(AreaId==TEXT("diner")?FString(TEXT("road")):AreaId,Middle.X,Middle.Z,RoadX,RoadZ))M->Freeform->Visitors->RakeAt(Place,M->Freeform->EventPlaces->LastNight(Place),RoadX,RoadZ);
   UE_LOG(LogTemp,Display,TEXT("S47: lights site %s gone over by the military: marks hidden"),*Place);
  }
  // PT-FEEDER-20261109: come out to it and found it gone over: checked, and empty (the raked circle is its own record).
  if(!bCheckedEmpty&&!bField&&M->Player&&M->World&&!M->World->bDriving&&Here()&&FVector2D(M->Player->Pos.X-Middle.X,M->Player->Pos.Z-Middle.Z).Size()<=ArriveWithin)
  {bCheckedEmpty=true;UE_LOG(LogTemp,Display,TEXT("S47: lights site %s checked: gone over, nothing left"),*Place);M->RequestAutosave();}
  StepFieldDraw(Dt);return;
 }
 // A menu holds all of it: no sound starts, no second counts.
 if(M->bPausedByMenu||M->ModalOpen())return;
 // Evening's marks, and now the hour they can come: a second report of the same place, for whoever went home.
 if(!Armed())StepEmptyTip();
 if(!bField&&(Visit==0||bFalseCall)&&!bNudged&&Armed()&&M->Freeform->NightStarted(M->Freeform->Clock())&&M->Player&&(!Here()||FVector2D(M->Player->Pos.X-Middle.X,M->Player->Pos.Z-Middle.Z).Size()>100))
 {
  SayNudge();
 }
 StepFieldDraw(Dt);
 if(Visit>=1&&Visit<=3&&(!Live()||!M->Web||!Here()||!M->Player)){EndVisit(bVisitTaken||(M->Freeform->Taken&&M->Freeform->Taken->Busy()));return;}
 if(!Live()||!M->Audio||!M->Player||!M->Web||!Here())return;
 const double Dist=FVector2D(M->Player->Pos.X-Middle.X,M->Player->Pos.Z-Middle.Z).Size();
 StepVisit(Dt,Dist);
 StepArrival(Dist);
 StepFigure(Dt,Dist);
 // Everything out here goes quiet first; a stretch at a time, so that it comes back on its own whatever ends this.
 if(VisitHushed())
 {
  HushIn-=Dt;
  if(HushIn<=0&&M->Audio->Night.IsValid()){HushIn=3;M->Audio->Night->Hush(5,.05,.8,3);}
 }
 else HushIn=0;
 // A lamp with a weak contact, while it lasts.
 if(Visit>=1&&Visit<=3&&M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(.25+.3*FMath::Abs(FMath::Sin(VisitAge*1.3)),TEXT("visit"));
 if(Dist>140||bField)return;
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

UPrimitiveComponent* FS47FarmSite::VisitPhotoBody() const
{
 // D48-20261110: the crab or the tall one, only while it is out and actually drawn (the small ones are the glimpses' own bodies).
 if(!bBodyOut||!VisitBody||!VisitBody->Rendered())return nullptr;
 return VisitBody->Creature.Mesh.Get();
}
TArray<UPrimitiveComponent*> FS47FarmSite::PhotoMarks() const
{
 TArray<UPrimitiveComponent*> Out;
 for(const auto& C:Marks)if(UStaticMeshComponent* P=C.Get())if(P->IsRegistered()&&P->IsVisible()&&!P->bHiddenInGame)Out.Add(P);
 return Out;
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

ES47Clip FS47FarmSite::VisitClipNow() const
{
 return bBodyOut&&VisitBody&&VisitBody->Creature.ClipMotion?VisitBody->Creature.ClipMotion->Playing():ES47Clip::Num;
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
 const FVector FaceAt=bHeldNow?Attention.LookAt():LureFace?*LureFace:Feet; // (IDLE-LURE: at the truck it looks where it goes, or at the truck)
 if(bHeldNow)Speed=Attention.BackSpeed();
 const double Face=FMath::Atan2(FaceAt.Z-BodyAt.Z,FaceAt.X-BodyAt.X);
 if(!((VisitWho==1||VisitWho==2)&&RuleBreak.Facing()))BodyYaw+=FMath::UnwindRadians(Face-BodyYaw)*FMath::Min(1.,Dt*2.5); // (N1110-6: stopped, it faces away until it turns)
 if(VisitWho==2)
 {
  // Far off it watches, looks about, and waits, in long turns. Held in a lamp close by, and only when it is really seen, it
  // may once a visit, and not often, put a finger to its mouth. It stands for as long as that takes.
  const double Turn=FMath::Fmod(BodyClock,34.);
  VisitBody->Stand=Turn<13?ES47Clip::Observe:Turn<26?ES47Clip::LookAround:ES47Clip::Idle;
  // Some visits it sits while it lets itself be found, facing the player, too far off for a lamp to show there is nothing
  // under it. Once up it stays up, however far the player then goes.
  bSitsFar=bSitsFar&&Speed<=0&&FVector::Dist(BodyAt,Feet)>=SitBeyond;
  if(bSitsFar)
  {
   VisitBody->Stand=ES47Clip::Sit;
   if(!bSatLogged){bSatLogged=true;UE_LOG(LogTemp,Display,TEXT("S47: site visit: the tall one sits, %.0f m off"),FVector::Dist(BodyAt,Feet));}
  }
  // The first time a lamp finds it, it stops dead for the length of that, whatever it was doing.
  FreezeLeft=FMath::Max(0.,FreezeLeft-Dt);
  if(!bFreezeTried&&ShhLeft<=0&&Attention.Doing()==FS47VisitAttention::EDo::Watch&&VisitBody->Rendered()&&VisitBody->Creature.Clips&&VisitBody->Creature.Clips->Has(ES47Clip::Freeze))
  {
   bFreezeTried=true;FreezeLeft=VisitBody->Creature.Clips->Seconds(ES47Clip::Freeze);
   UE_LOG(LogTemp,Display,TEXT("S47: site visit: the tall one freezes in the lamp, %.0f m off"),FVector::Dist(BodyAt,Feet));
   // EVIDENCE-DOSSIER slice 2: what the lamp does to this one, once in the game, kept beside the two rules and tagged like them. Only
   // when it was seen in the lamp (the attention's Watch, rendered): stopped dead in the first of the light.
   if(M->Freeform)
   {
    if(!M->Freeform->HasFinding(TEXT("rule-tall-froze")))M->Freeform->RecordEvidence(TEXT("rule-tall-froze"),FString::Printf(TEXT("%s. Something tall, out past where I stood. The moment my lamp found it, it stopped dead in the middle of a step, and it stood like that while the light was on it."),*S47Script::PlayerClock(M->Freeform->Clock()))); // REVIEW (dossier slice 2): never a :47
    M->Freeform->TagEvidence(TEXT("rule-tall-froze"),S47Dossier::EKind::Tall,S47Dossier::EType::Lamp);
   }
  }
  ShhLeft=FMath::Max(0.,ShhLeft-Dt);
  if(!bShhTried&&FreezeLeft<=0&&Attention.Doing()==FS47VisitAttention::EDo::Watch&&FVector::Dist(BodyAt,Feet)<ShhWithin&&VisitBody->Rendered()&&VisitBody->Creature.Clips&&VisitBody->Creature.Clips->Has(ES47Clip::Shh))
  {
   bShhTried=true;
   if(FMath::FRand()<.4f)ShhLeft=VisitBody->Creature.Clips->Seconds(ES47Clip::Shh);
  }
  if(ShhLeft>0||FreezeLeft>0)Speed=0;
 }
 // D45-20261110: walking at the player and near, not held back by a lamp: the music hears that as being come at (FS47FieldRadio).
 bBodyComing=Speed>0&&!bHeldNow&&!LureGoal.IsSet()&&FVector::Dist(BodyAt,Feet)<13.;
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
 VisitBody->Act=ShhLeft>0?ES47Clip::Shh:FreezeLeft>0?ES47Clip::Freeze:ES47Clip::Num;
 if(VisitWho==1)VisitBody->ApplyCrab(true,BodyAt,BodyYaw,Face,BodyClock,BodyClock/1.6,Speed>2.);
 else VisitBody->Apply(true,BodyAt,BodyYaw,Face,Speed>0?.85:0.,BodyClock*3,BodyClock,FS47PaleBody::EPose::Walk);
 {const bool bSeenBefore=bVisitSeen;bVisitSeen|=VisitBody->Rendered();if(!bSeenBefore&&bVisitSeen&&!bTestNoticed)FS47Happenings::TellOther(M,TEXT("visit"),Place);} // M1-MEASURE-20261113
}

void FS47FarmSite::HideBody()
{
 if(VisitBody&&VisitBody->bBuilt&&bBodyOut)
 {
  if(BodyKind==1)VisitBody->ApplyCrab(false,BodyAt,BodyYaw,BodyYaw,BodyClock,1,false);
  else VisitBody->Apply(false,BodyAt,BodyYaw,BodyYaw,0,0,BodyClock,FS47PaleBody::EPose::Walk);
 }
 bBodyOut=false;bBodyComing=false;
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
  // WEEKLY-CASE-20261109: nor the week's case, from its call at ten to two to its end (an ordinary visit would start on top of it).
  const bool bQuiet=M->Freeform->Director&&M->Freeform->Director->ActiveEvent().IsEmpty()&&G->Individuals().IsEmpty()&&!Tk->Busy()&&!(M->Freeform->IndoorDread&&M->Freeform->IndoorDread->DoorActive())&&!FS47WeekCase::HoldsFarm(M);
  const double Clock=M->Freeform->Clock();
  // Parked with a spotlight on, they know about it from further off.
  const bool bLampOut=bDriving&&M->Freeform->TruckLamp&&M->Freeform->TruckLamp->On()&&M->World->CabDrive()&&FMath::Abs(M->World->CabDrive()->Speed)<.5;
  VisitDwell=(bLampOut?Dist<130:Dist<55&&!bDriving)&&bDark&&bQuiet&&Hours(Clock)&&Clock>=CalmUntil&&!M->ModalOpen()?VisitDwell+Dt:0;
  if(VisitDwell<12)return;
  if(M->Rhythm&&!M->Rhythm->Allow(TEXT("site_visit")))return;
  if(M->Rhythm)M->Rhythm->Began(TEXT("site_visit"));
  const bool bNaturalWho=TestRoll<0;const float Roll=TestRoll>=0?TestRoll:FMath::FRand();TestRoll=-1;
  Visit=1;VisitUntil=0;VisitAge=0;bVisitSeen=false;bVisitTaken=false;VisitWho=PickWho(GroundKind(),FMath::Fmod(Clock,86400.)/3600.,Roll);
  // D48-20261110 (D46-17): the night's own kind leans on it (S47AlienKinds); a test's set roll keeps the kind it asked for.
  if(bNaturalWho){const int32 WhoNight=FS47FreeformSession::NightOf(Clock);VisitWho=S47AlienKinds::LeanVisit(VisitWho,S47AlienKinds::NightLean(M->Freeform->Seed(),WhoNight),FMath::Fmod(Clock,86400.)/3600.,FS47NightRhythm::Roll(M->Freeform->Seed(),WhoNight,0x4c45414eu));}
  UE_LOG(LogTemp,Display,TEXT("S47: site visit begins at %s, kind %d"),*Place,VisitWho);
  if(M->Freeform->EventPlaces){M->Freeform->EventPlaces->Used(Place,TEXT("site_visit"));FS47EventPlaces::LogPick(TEXT("site_visit"),PlaceOnList(),FString::Printf(TEXT("kind %d"),VisitWho));}
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
  bShhTried=false;ShhLeft=0;bFreezeTried=false;FreezeLeft=0;bSatLogged=false;
  bSitsFar=(SitRoll>=0?SitRoll:FMath::FRand())<SitChance;SitRoll=-1;
  VisitRounds=0;VisitHunt=0;VisitPatience=120;RuleBreak.Reset();StillHeld=0;LetAttentionGo();Attention.Reset();EndLure();Lure.Reset();LureNoiseIn=0;bAttentionFeet=false;bWasDriving=bDriving;bWasRunning=false;bSawTurn=false;bSawSetTurn=false;
  RadioSeen=M->FieldRadio?M->FieldRadio->MessagesHeard():0;
  // Sometimes the set speaks up while it is out there, whether the player wants it to or not.
  RadioIn=!bTestHoldHunt&&FMath::FRand()<.35f?FMath::FRandRange(10.f,30.f):-1.;
 }
 // D45-20261110 (Tom, d45: at the feed store the tall one never showed; he had stepped under the dock's roof and the light came for
 // him while it still stood 58 m off). The tall one and the crab must be out, past their first wait, and near before the light comes
 // over a roof: the tall one within 26 m (where it is on its feet), the crab within 14. The small ones come from cover, as before.
 // Shut in where no body can be put in sight at all (18 s of the visit and still none out), the light comes over the roof as it did.
 const bool bRoofNear=VisitWho==0||(bBodyOut&&BodyClock>=(VisitWho==2?TallStands:1.6)&&FVector::Dist(BodyAt,Feet)<(VisitWho==2?SitBeyond:14.))
  ||(Visit==1&&!bBodyOut&&VisitAge>=18.);
 if(VisitRounds<2&&!bDriving&&!Tk->Busy()&&bRoofNear&&!RuleBreak.Active()&&FMath::Fmod(VisitAge,1.)<=Dt&&Tk->Indoors()&&Tk->Begin(Who,Title))++VisitRounds; // (N1110-6: not while it is breaking the rule: going inside is the way out)
 if(Visit==1)
 {
  // The warning: the sound of the place, now close, and any radio breaking up. Time enough to go.
  if(bFirst)
  {
   M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.55f,.At=FVector(M->Player->Pos.X,M->Player->FloorY+1,M->Player->Pos.Z-8)});
   // The truck will not start now, the first time it is tried.
   M->World->bKeyStall=true;bStallSet=true;
   WarnAnimals(); // EVIDENCE-DOSSIER slice 2: the animals first, by who comes
  }
  StepAnimals(bAway);
  if(VisitAge<6)return;
  if(bAway||Dist>80){EndVisit(false);return;}
  // Out in the open there may be no cover for the small ones to come from. Something else comes instead.
  if(VisitWho==0&&bField&&VisitAge>12){VisitWho=1;UE_LOG(LogTemp,Display,TEXT("S47: field visit: no cover near, the crab instead"));}
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
 // IDLE-LURE-20261112: the truck left running near it may draw the body away for a while (S47IdleLure.h).
 StepLure(Dt);
 // A lamp on it buys a moment. A player with the lamp out, standing still, gives it nothing to go on: no hunt comes of
 // that, and it lets go in half the time. Moving, or with the lamp on, it goes as it always has.
 // SNEAK-20261108: low, with the lamp out, it has less to go on while the player moves too (S47Sneak.h): slower, not stopped.
 const bool bLampLit=M->Night&&M->Night->Flashlight&&M->Night->Flashlight->Intensity()>0;
 const double LowK=M->Player?M->Player->Sneak():0.;
 VisitHunt+=Attention.Holding()||bTestHoldHunt||bNothingToGoOn||Lure.Active()?0.:Dt*S47Sneak::HuntPace(LowK,bLampLit);
 VisitPatience-=Dt*(bNothingToGoOn&&!Tk->Busy()?2.:1.);
 if(VisitWho==0)
 {
  // D48-20261110 (D46-17): curious, they keep off and watch longer before they come (S47AlienKinds::FTemper::HuntAfter, was 22 s).
  if(Visit==2&&VisitHunt>S47AlienKinds::Temper(ES47AlienKind::Roswell).HuntAfter){G->DevMode(FS47RoswellBrain::EMode::Hunt);Visit=3;VisitPatience=FMath::Max(VisitPatience,60.);UE_LOG(LogTemp,Display,TEXT("S47: site visit: it hunts now"));}
  // N1110 (N1110-6): the rule, taught once: lamp off and still, and the small one loses the player and looks past (the field record is the proof it was learned)
  StillHeld=bNothingToGoOn&&Visit==2&&!G->Individuals().IsEmpty()?StillHeld+Dt:0.;
  if(StillHeld>=5.&&M->Freeform&&!M->Freeform->HasFinding(TEXT("rule-still-held")))
  {
   if(M->UI)M->UI->Thought(TEXT("It has lost me. The lamp is off, I am standing still, and it is looking past me."),5.f);
   M->Freeform->RecordEvidence(TEXT("rule-still-held"),FString::Printf(TEXT("%s. A small one came near the place I stood. I had the lamp off and I kept still, and it looked past me. Lamp off, stand still, and the small ones pass."),*S47Script::PlayerClock(M->Freeform->Clock())));
   M->Freeform->TagEvidence(TEXT("rule-still-held"),S47Dossier::EKind::Small,S47Dossier::EType::Lamp); // EVIDENCE-DOSSIER: what the lamp does, in its file
   UE_LOG(LogTemp,Display,TEXT("S47: rule taught: lamp off and still, the small one passed"));
  }
  for(const auto& I:G->Individuals())
  {
   {const bool bSeenBefore=bVisitSeen;bVisitSeen|=I.bRendered;if(!bSeenBefore&&bVisitSeen&&!bTestNoticed)FS47Happenings::TellOther(M,TEXT("visit"),Place);} // M1-MEASURE-20261113
   if(I.State==FS47RoswellBrain::EState::Contact&&!bDriving){G->DevClear();Tk->TakeNow(Who,Title);bVisitTaken=true;return;}
  }
 }
 else
 {
  const double To=FVector::Dist(BodyAt,Feet);
  // N1110 (N1110-6): once in the game, from night 4 and after two, the Pale One does not pass the player who has learned to stand still in the dark
  // (S47RuleBreak.h): it stops, turns its head, and comes on at a walk. The record is the log and spends it.
  if((VisitWho==1||VisitWho==2)&&M->Freeform&&!Lure.Active()) // IDLE-LURE: not while it is at the truck. N1110 (pace): the Tall One may break the rule too, not only the crab
  {
   FS47RuleBreak::FIn Rule;
   const double Clock=M->Freeform->Clock();
   Rule.Night=FS47FreeformSession::NightOf(Clock);Rule.Hour=FMath::Fmod(Clock,86400.)/3600.;Rule.bPale=VisitWho==1;Rule.bTall=VisitWho==2;
   Rule.bLearned=M->Freeform->HasFinding(TEXT("rule-still-held"));Rule.bSpent=bRuleBrokenThisSession||M->Freeform->HasFinding(TEXT("rule-pale-broken"));
   Rule.bStillDark=bNothingToGoOn;Rule.bInside=Tk->Indoors();Rule.Dist=To;
   RuleBreak.Step(Dt,Rule);
   if(RuleBreak.TakeBroke())
   {
    bRuleBrokenThisSession=true; // (spent for the session even if the finding cap refuses the record)
    if(M->UI)M->UI->Thought(TEXT("It has stopped. It is facing away."),3.f);
    // the record says what was done only if it was: still and dark, and the stop is the moment of the break
    if(bNothingToGoOn)M->Freeform->RecordEvidence(TEXT("rule-pale-broken"),FString::Printf(TEXT("%s. I did what had worked: lamp off, standing still. This one was not small like the other. It stopped dead and turned to where I was. It did not pass. Standing still is not enough."),*S47Script::PlayerClock(Clock)));
    // EVIDENCE-DOSSIER: the low one's own answer to standing still. The tall one may break the rule as well (N1110), and that is not its mark.
    if(bNothingToGoOn&&VisitWho==1)M->Freeform->TagEvidence(TEXT("rule-pale-broken"),S47Dossier::EKind::Low,S47Dossier::EType::Lamp);
    UE_LOG(LogTemp,Display,TEXT("S47: rule broken once: the %s stopped %.0f m off and turned (night %d)"),VisitWho==2?TEXT("Tall One"):TEXT("Pale One"),To,Rule.Night);
   }
   if(RuleBreak.TakeTurned()&&M->UI)M->UI->Thought(TEXT("It is looking at me. Move."),4.f);
   if(RuleBreak.Over())
   {
    if(M->UI&&RuleBreak.EndedInside())M->UI->Thought(TEXT("It stopped at the door. After a while it went."),4.5f);
    else if(M->UI&&RuleBreak.EndedByTime())M->UI->Thought(TEXT("It stopped. After a while it went."),4.5f);
    EndVisit(false);return;
   }
  }
  // The tall one first stands still and lets itself be found, then walks, slower as it nears. Its light goes before it.
  double Speed=(VisitWho==1?(BodyClock<1.6?0.:To<13?3.2:1.2):(BodyClock<TallStands?0.:To>30?1.35:.75))*S47Sneak::ApproachPace(LowK,bLampLit);
  if(VisitWho==1||VisitWho==2)Speed=RuleBreak.Speed(Speed);
  // IDLE-LURE-20261112: at the truck it walks to where the lure wants it, at a curious walk, and stands there.
  if(Lure.Active())Speed=LureGoal&&FVector2D::Distance(FVector2D(BodyAt.X,BodyAt.Z),FVector2D(LureGoal->X,LureGoal->Z))>FS47IdleLure::Reached*.6?FS47IdleLure::WalkMps*(Lure.Part()==FS47IdleLure::EPart::Drawn?1.4:1.):0.;
  StepBody(Dt,Speed);
  // A lamp reaches 25 m. Held on it for the first time it gets to freeze before their light comes on, not under it.
  const bool bLampFirst=VisitWho==2&&!bFreezeTried&&M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn;
  if(VisitWho==2&&BodyClock>=TallStands&&VisitRounds<2&&!bDriving&&!Tk->Busy()&&!RuleBreak.Active()&&FreezeLeft<=0&&To<(VisitRounds==0?(bLampFirst?14.:26.):10.)&&Tk->Begin(Who,Title))++VisitRounds;
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
 // WEEKLY-CASE-20261109, patch 3: the starter turning over in a dead truck is heard as getting in is, each turn once.
 {static int32 TurnsSeen=0;const int32 Turns=M->World&&M->World->Trouble.Kind()==ES47Trouble::Stall?M->World->Trouble.Turns:0;if(bDriving&&Turns>TurnsSeen&&See.BodyDist<60){See.bNoise=true;See.Sound=Feet;}TurnsSeen=Turns;}
 // The set is the player's to carry, but not their noise: nobody chose that voice.
 const bool bSetSpoke=RadioNow!=RadioSeen&&See.BodyDist<30;
 if(bSetSpoke){See.bNoise=true;See.Sound=In.Cam.Pos-FVector(0,.4,0);}
 // PHYSICS-20261108: something thrown or dropped that it can hear (S47Noise), each one once: it turns to where it fell
 {static uint32 NoiseSeen=0;if(const auto Fell=S47Noise::Latest(.4);Fell&&Fell->Serial!=NoiseSeen&&Fell->What!=FS47IdleLure::NoiseName&&FVector::Dist(Fell->At,*Eye)<=S47Noise::Reach(Fell->Loudness)*1.5){NoiseSeen=Fell->Serial;See.bNoise=true;See.Sound=Fell->At;}}
 bWasRunning=bRunning;bWasDriving=bDriving;RadioSeen=RadioNow;
 const auto Before=Attention.Doing();
 // Frozen, it is still only looking: what comes after the look waits until it can move again.
 if(FreezeLeft<=0)Attention.Step(Dt,See,AttentionRoll>=0?AttentionRoll:FMath::FRand());
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
 if(M&&M->Rhythm)M->Rhythm->Ended(bField?TEXT("field_visit"):TEXT("site_visit"));
 const bool bWas=Visit>=2;
 Visit=4;VisitDwell=0;VisitUntil=0;bVisitTaken=false;
 LetAttentionGo();
 EndLure();
 // Over: the lamp steadies, and a truck not yet tried starts again.
 if(M&&M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(0,TEXT("visit"));
 if(bStallSet&&M&&M->World)M->World->bKeyStall=false;
 bStallSet=false;
 if(M&&M->Freeform&&M->Freeform->Taken&&!bTaken)M->Freeform->Taken->GiveUp();
 HideBody();
 if(M&&M->Night&&M->Night->Glimpses&&bWas)M->Night->Glimpses->DevClear();
 if(!M||!M->Freeform){RestoreHome();return;}
 UE_LOG(LogTemp,Display,TEXT("S47: site visit over at %s, taken %d, seen %d"),*Place,bTaken,bVisitSeen);
 // EVIDENCE-DOSSIER slice 2: a body was out at these marks: by daylight its prints are among them, and the counter reads by it
 if(bWas&&!bField&&Armed()){MarksWho=VisitWho;MarksWhoNight=FS47FreeformSession::NightOf(M->Freeform->Clock());}
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
  if(Lure.bDidStrike)Did+=TEXT(" It went to my truck, where I had left it running with the lights on, and struck the hood. The engine died.");
  else if(Lure.bDidTouch)Did+=TEXT(" It went to my truck, where I had left it running with the lights on, and put its hand on the hood. The lights stuttered.");
  FString Page=FString::Printf(TEXT("%s, after dark%s. Something came toward me.%s Then it was gone."),*Title,bField?TEXT(""):TEXT(", the night of the lights"),*Did);Page[0]=FChar::ToUpper(Page[0]);
  M->Freeform->RecordEvidence(FindingId()+TEXT("_visit"),Page);
  // EVIDENCE-DOSSIER: it was seen (bVisitSeen), and the visit knows which one came: its shape, in its file.
  M->Freeform->TagEvidence(FindingId()+TEXT("_visit"),S47Dossier::KindOfVisit(VisitWho),S47Dossier::EType::Shape);
 }
 RestoreHome();
 M->RequestAutosave();
}

bool FS47FarmSite::ByDaylight(double Clock){const double H=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;return H>=6&&H<19;}
int32 FS47FarmSite::DaylightRead(int32 Seed,const FString& InPlace,int32 Day)
{
 const float R=FS47NightRhythm::Roll(Seed,Day,FCrc::StrCrc32(*InPlace));
 return R<.4f?0:R<.75f?1:2;
}
FString FS47FarmSite::DaylightId() const{return FString::Printf(TEXT("%s_daylight_d%d"),*FindingId(),M&&M->Freeform?M->Freeform->Day():0);}
FString FS47FarmSite::DaylightText(int32 Read,bool bFarm)
{
 if(bFarm)switch(Read)
 {
  case 0:return TEXT("Pasture at the Alderete place, by daylight. Hoof prints through most of the ring, and a length of baling wire dragged across it. Cattle would do this. All nine marks still lie the same way around.");
  case 1:return TEXT("Pasture at the Alderete place, by daylight. Nine shallow saucers, the same depth to a finger, the dirt pressed rather than dug. No tracks lead up to the ring, and there are plenty beside it.");
  default:return TEXT("Pasture at the Alderete place, by daylight. The soil in each mark has a thin crust that glitters like sugar. Under the middle one, a sliver of something, light as foil and cold. Kept for the test bench.");
 }
 switch(Read)
 {
  case 0:return TEXT("The marks by daylight. Oil and a tire scuff in two of them. The other seven are only dark.");
  case 1:return TEXT("The marks by daylight. No fainter than they were in the night, and the edges still sharp.");
  default:return TEXT("The marks by daylight. The surface in each has gone glassy and lifts in flakes. One flake is thinner than the rest, light as foil and cold. Kept for the test bench.");
 }
}
int32 FS47FarmSite::MarksBy() const
{
 // These marks: laid no later than the night it came (SiteNight), and not a field visit's open ground
 return MarksWho>=0&&Armed()&&!bField&&MarksWhoNight>=SiteNight()?MarksWho:-1;
}
FString FS47FarmSite::PrintsText(int32 Who)
{
 // EVIDENCE-DOSSIER slice 2: the table's GROUND column, by daylight. The low one's and the tall one's as their prints on the road's
 // shoulder are drawn (S47Hap::RoadTrackParts: fours a hand apart, about two and a half feet between them; single prints 1.15 m apart).
 switch(Who)
 {
  case 0:return TEXT(" Between the marks, small bare prints, narrow, and here and there a stone turned over with its damp side up.");
  case 1:return TEXT(" Across the ring, small prints in fours, close together, a hand apart, and at the edge they go off at a run.");
  case 2:return TEXT(" Going away from the ring, long narrow prints, a long stride apart, nearly four feet.");
  default:return FString();
 }
}
void FS47FarmSite::WarnAnimals()
{
 // EVIDENCE-DOSSIER slice 2: the animals the game has, by who comes (FS47AnimalWarn::ForVisit): a covey goes up ahead of the tall one,
 // a coyote stands and stares and then runs ahead of the crab, and ahead of the small ones the dogs bark and then all stop. Out on
 // foot only, as the warnings for the night's events are; the coyote's and the covey's own scripts say what is thought and kept.
 bAnimalsWent=false;bAnimalsTagged=false;bDogsHushed=true;AnimalsWho=VisitWho;
 if(!M||!M->Player||!M->World||M->World->bDriving||!M->Freeform)return;
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 FVector Way=Middle-Feet;Way.Y=0;
 if(Way.SizeSquared()<4.)Way=FVector(FMath::FRandRange(-1.f,1.f),0,FMath::FRandRange(-1.f,1.f));
 const FVector Threat=Feet+Way.GetSafeNormal()*60.;
 const FS47AnimalWarn::EKind Ahead=FS47AnimalWarn::ForVisit(VisitWho);
 FString Why;
 if(Ahead!=FS47AnimalWarn::EKind::None)
 {
  if(!M->Animals||!M->Animals->Warn(Ahead==FS47AnimalWarn::EKind::Birds,Threat,FS47AnimalWarn::Lead,Why)){UE_LOG(LogTemp,Display,TEXT("S47: site visit: no animals ahead of it here (%s)"),*Why);return;}
  FS47EventCtx Ctx;S47EventScript::Play(M,FS47AnimalWarn::ScriptOf(Ahead),Ctx);
 }
 else if(M->Audio)
 {
  // the dogs off that way: a few barks, then nothing (StepAnimals stops them and the night with them)
  for(int32 K=0;K<4;++K){FS47Sound O;O.At=Threat+FVector(FMath::FRandRange(-30.f,30.f),2,FMath::FRandRange(-30.f,30.f));O.Gain=.3f;O.Rate=FMath::FRandRange(.9f,1.1f);O.When=.5f*K;M->Audio->Sfx(K%2?TEXT("dog1"):TEXT("dog0"),O);}
  bDogsHushed=false;
 }
 else return;
 bAnimalsWent=true;
 FS47Happenings::TellOther(M,TEXT("animal warning"),Place); // M1-MEASURE-20261113: put near the player, heard or seen
 UE_LOG(LogTemp,Display,TEXT("S47: site visit: the animals first (%s)"),Ahead==FS47AnimalWarn::EKind::Birds?TEXT("a covey goes up"):Ahead==FS47AnimalWarn::EKind::Coyote?TEXT("a coyote stands and stares"):TEXT("the dogs bark, then stop"));
}
void FS47FarmSite::StepAnimals(bool bAway)
{
 if(!bAnimalsWent||!M||!M->Freeform)return;
 if(!bDogsHushed&&VisitAge>=2.2)
 {
  bDogsHushed=true;
  if(M->Audio&&M->Audio->Night)M->Audio->Night->Hush(25,.03,.15,4.);
  if(M->UI)M->UI->Thought(TEXT("Even the dogs went quiet."),4.f);
 }
 // Seen or heard by now, still there: it is the file's, by who came
 if(!bAnimalsTagged&&VisitAge>=3.&&!bAway)
 {
  bAnimalsTagged=true;
  M->Freeform->TagEvidence(FindingId()+FString::Printf(TEXT("_animals_d%d"),FS47FreeformSession::NightOf(M->Freeform->Clock())),S47Dossier::KindOfVisit(AnimalsWho),S47Dossier::EType::Animals);
 }
}
FString FS47FarmSite::Label() const
{
 if(!bActive||!Armed()||bField||Raked())return FString();
 if(M&&M->Freeform&&ByDaylight(M->Freeform->Clock()))return M->Freeform->HasFinding(DaylightId())?TEXT("Pressed ground / already looked at by daylight"):TEXT("Pressed ground / look again by daylight");
 return M&&M->Freeform&&M->Freeform->HasFinding(FindingId())?TEXT("Pressed ground / already noted"):TEXT("Pressed ground / look closer");
}

bool FS47FarmSite::Use()
{
 if(!bActive||!Armed()||bField||Raked()||!M||!M->UI||!M->Freeform)return false;
 const bool bNew=!M->Freeform->HasFinding(FindingId());
 const bool bFarm=Place==TEXT("farm");
 if(ByDaylight(M->Freeform->Clock()))
 {
  const FString Id=DaylightId();const int32 Read=DaylightRead(M->Freeform->Seed(),Place,M->Freeform->Day());
  // EVIDENCE-DOSSIER slice 2: what came to the marks left its prints among them; read on the page, they are its file's GROUND
  const int32 Who=MarksBy();
  const bool bFirst=!M->Freeform->HasFinding(Id);const FString Page=DaylightText(Read,bFarm)+PrintsText(Who);
  if(Who>=0)M->Freeform->TagEvidence(Id+TEXT("_prints"),S47Dossier::KindOfVisit(Who),S47Dossier::EType::Ground,MarksWhoNight);
  if(bFirst)
  {
   M->Freeform->RecordEvidence(Id,Page);
   if(Read==2)M->Freeform->AddItem(TEXT("fragment"),1);
   UE_LOG(LogTemp,Display,TEXT("S47: daylight look at %s: reading %d"),*Place,Read);
   M->RequestAutosave();
  }
  M->UI->Open(S47Kit::Shell(M->UI,Title.ToUpper(),S47Kit::Col({S47Kit::Text(Page),S47Kit::Small(bFirst?TEXT("Kept as a field record. The marks won't last past noon."):TEXT("Already in your field records."))}),760)); // D45-20261110: plain words
  return true;
 }
 if(bNew)M->Freeform->RecordEvidence(FindingId(),bFarm
  ?FString(TEXT("Pasture at the Alderete place, the night of the lights. Nine pressed marks in a ring about thirty feet across, the grass darkened and laid flat. Cattle could do it, or a truck, or kids. Worth a second look by daylight."))
  :FString::Printf(TEXT("Open ground %s %s, the night of the lights. Nine dark marks in a ring about %s feet across. A truck could have stood here, or something got spilled. Worth a second look by daylight."),OpenGround()?TEXT("at"):TEXT("outside"),*Title,OpenGround()?TEXT("thirty"):TEXT("fifteen")));
 TArray<S47Kit::FW> Rows={
  S47Kit::Text(bFarm?TEXT("Nine marks in a ring, about thirty feet across. The grass is laid flat and darkened, and it is dry here when the ground beside it is not."):OpenGround()?TEXT("Nine dark marks in a ring, about thirty feet across. The dirt in each is pressed down smooth and flat."):TEXT("Nine dark marks in a ring, about fifteen feet across. Each one is pressed smooth into the ground.")),
  S47Kit::Small(bNew?TEXT("Kept as a field record."):TEXT("Already in your field records."))};
 if(M->Freeform->TripCamera&&M->Freeform->TripCamera->CanSet())
 {
  const FString Where=Title;
  Rows.Add(S47Kit::Button(TEXT("Leave the trip camera by the marks until morning"),[this,Where]{M->Freeform->TripCamera->SetHere(Where,true);if(M->UI)M->UI->Close(true);}));
 }
 M->UI->Open(S47Kit::Shell(M->UI,Title.ToUpper(),S47Kit::Col(Rows),760));
 if(bNew)M->RequestAutosave();
 return true;
}

// ---------------------------------------------------------------- PT-FEEDER-20261109: what a reported place gives
int32 FS47FarmSite::SiteNight() const{return ClearAt>0?FS47FreeformSession::NightOf(ClearAt)-1:0;}
bool FS47FarmSite::CheckedIn(const FS47FreeformSession& F,const FString& InPlace,int32 Night,bool bEmpty)
{
 if(bEmpty||F.HasFinding(ArrivalKey(InPlace,Night)))return true;
 // The marks looked at, at night or by daylight (the morning after is still the marks' night until eight).
 const FString Marks=InPlace==TEXT("farm")?FString(TEXT("farm_trace")):TEXT("lights_trace_")+InPlace;
 return F.HasFinding(Marks)||F.HasFinding(FString::Printf(TEXT("%s_daylight_d%d"),*Marks,Night))||F.HasFinding(FString::Printf(TEXT("%s_daylight_d%d"),*Marks,Night+1));
}
bool FS47FarmSite::Checked() const
{
 if(bField||!Armed()||!M||!M->Freeform)return false;
 return CheckedIn(*M->Freeform,Place,SiteNight(),bCheckedEmpty);
}
void FS47FarmSite::YieldTo(const TCHAR* Why)
{
 // A meeting already under way is not cut off: the rhythm keeps the big night from starting on top of one anyway.
 if(bField||ClearAt<=0||(Visit>=1&&Visit<=3))return;
 const bool bHad=Visit==0||bFalseCall||!bNudged;
 Visit=4;bFalseCall=false;bNudged=true;bYielded=true;
 if(bHad)UE_LOG(LogTemp,Display,TEXT("S47: lights site %s yields to %s: the marks stay, nothing comes to them tonight"),*Place,Why);
 if(M)M->RequestAutosave();
}
// The time words, from the clock: at dusk to eight, after dark to ten, late in the evening to midnight, past midnight to two, the small
// hours to six, and by day the morning (to noon) or by daylight. A night tier at dusk has nothing to do with the clock's word (CRIT9).
FString FS47FarmSite::ArrivalWhen(int32 Tier,double Hour)
{
 if(Hour<0)
 {
  static const TCHAR* const ByTier[ArrivalTiers]={TEXT(", after dark"),TEXT(", past midnight"),TEXT(", in the small hours"),TEXT(", in the morning")};
  return ByTier[FMath::Clamp(Tier,0,ArrivalTiers-1)];
 }
 const double H=FMath::Fmod(Hour,24.);
 if(H>=6&&H<19)return H<12?TEXT(", in the morning"):TEXT(", by daylight");
 if(H>=19&&H<20)return TEXT(", at dusk");
 if(H>=20&&H<22)return TEXT(", after dark");
 if(H>=22)return TEXT(", late in the evening");
 return H<2?TEXT(", past midnight"):TEXT(", in the small hours");
}
FString FS47FarmSite::ArrivalText(int32 Tier,int32 Pick,const FString& InTitle,double Hour)
{
 // Written as the player would note it: what is there, never what it means. The night's hour decides how much is there.
 static const TCHAR* const Lines[ArrivalTiers][ArrivalPicks]={
  {TEXT("{t}{when}. A few steps off, nine dark patches in the dirt, and the air over them smells of hot dust and struck matches, like a fuse gone. Somebody's truck, maybe."),
   TEXT("{t}{when}. A ring of dark patches in the dirt a few steps off, and a smell like a blown transformer, stronger inside the ring than out of it.")},
  {TEXT("{t}{when}. Round the dark patches in the dirt the grass is singed brown at the tips, every blade on the side toward the middle. No fire ring, no ash, no bottles."),
   TEXT("{t}{when}. The brush beside the dark patches in the dirt is scorched on one side only, as if something hot had stood low over them. The ground in the ring is dry, the ground beside it damp.")},
  {TEXT("{t}{when}. A jackrabbit lies at the edge of the dark patches in the dirt, on its side with its eyes open, not a mark on it. The ground under it is still warm."),
   TEXT("{t}{when}. Three sparrows lie at the edge of the dark patches in the dirt, side by side, not a mark on them. Nothing has been at them. The air in the ring smells of struck matches.")},
  {TEXT("{t}{when}. Nine dark patches in the dirt, the grass round them singed brown at the tips. A jackrabbit lies dead at the edge of them, not a mark on it."),
   TEXT("{t}{when}. Nine dark patches in the dirt. The brush beside them is scorched on one side only, and dead moths lie all over the ring.")}};
 FString Page=FString(Lines[FMath::Clamp(Tier,0,ArrivalTiers-1)][FMath::Clamp(Pick,0,ArrivalPicks-1)]).Replace(TEXT("{t}"),*InTitle).Replace(TEXT("{when}"),*ArrivalWhen(Tier,Hour));
 if(!Page.IsEmpty())Page[0]=FChar::ToUpper(Page[0]);
 return Page;
}
FString FS47FarmSite::ArrivalToast(int32 Tier)
{
 static const TCHAR* const Says[ArrivalTiers]={
  TEXT("FIELD / A smell of struck matches, and dark patches in the dirt a few steps off. Kept in your notebook; look closer at the marks."),
  TEXT("FIELD / Singed grass round dark patches in the dirt a few steps off. Kept in your notebook; look closer at the marks."),
  TEXT("FIELD / Something dead at the edge of dark patches in the dirt a few steps off. Kept in your notebook; look closer at the marks."),
  TEXT("FIELD / The marks by daylight, a few steps off, and what the night left by them. Kept in your notebook.")};
 return Says[FMath::Clamp(Tier,0,ArrivalTiers-1)];
}
void FS47FarmSite::StepArrival(double Dist)
{
 if(bField||!Armed()||Raked()||!M||!M->Freeform||!M->World||M->World->bDriving||Dist>ArriveWithin)return;
 auto& F=*M->Freeform;
 const int32 Night=SiteNight();const FString Key=ArrivalKey(Place,Night);
 if(F.HasFinding(Key))return;
 const double Clock=F.Clock();
 const int32 Tier=ByDaylight(Clock)?3:int32(S47NightTiers::AtClock(Clock,FS47FreeformSession::NightOf(Clock)));
 const int32 Pick=FMath::Clamp(int32(FS47NightRhythm::Roll(F.Seed(),Night,FCrc::StrCrc32(*Place)^0x41525256u)*ArrivalPicks),0,ArrivalPicks-1);
 F.RecordEvidence(Key,ArrivalText(Tier,Pick,Title,FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.));
 if(M->UI)M->UI->Toast(ArrivalToast(Tier),7);
 UE_LOG(LogTemp,Display,TEXT("S47: lights site %s: arrival record kept (tier %d, pick %d, %.0f m from the marks), visit %d"),*Place,Tier,Pick,Dist,Visit);
 M->RequestAutosave();
}
void FS47FarmSite::StepFigure(double Dt,double Dist)
{
 // Nothing is coming to these marks (the dice said so), and the hour allows a glimpse: a while after the player has found them, a
 // figure may stand far off past them (FS47Happenings' horizon_figure, asked as the night asks it: the rhythm, the tier, on foot).
 // Once for these marks. Not when the big night has the night (YieldTo).
 if(bFigureAsked||bField||bYielded||Visit!=4||!Armed()||Raked()||!M||!M->Freeform||!M->World||M->World->bDriving||Dist>60)return;
 auto& F=*M->Freeform;const double Clock=F.Clock();
 if(ByDaylight(Clock)||!S47NightTiers::Allows(ES47Allowed::Glimpse,FMath::Fmod(Clock,86400.)/3600.,FS47FreeformSession::NightOf(Clock)))return;
 if(!F.HasFinding(ArrivalKey(Place,SiteNight()))){FigureIn=0;return;}
 FigureIn+=Dt;if(FigureIn<20)return;
 bFigureAsked=true;
 FString Why;const bool bCame=F.Happenings&&F.Happenings->Request(TEXT("horizon_figure"),Why);
 UE_LOG(LogTemp,Display,TEXT("S47: lights site %s: a figure far off past the marks: %s"),*Place,bCame?TEXT("it stands there"):*(TEXT("not tonight, ")+Why));
}

// IDLE-LURE-20261112 (Tom, 12 Nov): the truck left idling near a visit (US47World::IdleTruckNow) draws the body out now to it for a
// while: round it, into its lamps, a hand on the hood (the lamps falter, the engine coughs, a hand in the frost), and left alone
// there some visits it strikes the hood and the engine stalls. FS47IdleLure decides; this measures and carries it out.
void FS47FarmSite::StepLure(double Dt)
{
 LureGoal.Reset();LureFace.Reset();
 if(!M||!M->World||!M->Player||!M->Freeform||Lure.Over()){if(bLureLamp)RestoreLureLamp();return;}
 auto G=M->Night?M->Night->Glimpses:TSharedPtr<FS47Glimpses>();
 FS47Truck* Truck=M->World->IdleTruckNow();
 // The body out now: the small one's own brain, or the moved one.
 TOptional<FVector> Body;int32 SmallId=INDEX_NONE;bool bHunting=false;
 if(VisitWho==0){if(G){const auto V=G->Individuals();if(!V.IsEmpty()){Body=V[0].Pos;SmallId=V[0].Id;bHunting=V[0].Mode==FS47RoswellBrain::EMode::Hunt;}}}
 else if(bBodyOut)Body=BodyAt;
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 const auto Flat=[](const FVector& From,const FVector& To){return FVector2D::Distance(FVector2D(From.X,From.Z),FVector2D(To.X,To.Z));};
 const double Clock=M->Freeform->Clock();
 FVector Mid=FVector::ZeroVector,Fwd(0,0,-1),Lamps=FVector::ZeroVector;
 if(Truck)
 {
  // (the drive's heading: forward is -sin, -cos, as FS47DriveController moves it)
  Mid=Truck->Position;const double Heading=Truck->Rotation.Y;Fwd=FVector(-FMath::Sin(Heading),0,-FMath::Cos(Heading));
  LureHood=Mid+Fwd*(Truck->HalfLength()+.55);Lamps=Mid+Fwd*(Truck->HalfLength()+3.2);
 }
 using EL=FS47IdleLure::EPart;
 const auto GoalOf=[&](EL Part)->TOptional<FVector>
 {
  if(!Truck||!Body)return {};
  switch(Part)
  {
  case EL::Drawn:{FVector Side=FVector(Body->X-Mid.X,0,Body->Z-Mid.Z).GetSafeNormal();if(Side.IsNearlyZero())Side=-Fwd;return Mid+Side*FS47IdleLure::RingRadius;}
  case EL::Circle:{const double Round=LureRingFrom+Lure.RingTurn();return Mid+FVector(FMath::Cos(Round),0,FMath::Sin(Round))*FS47IdleLure::RingRadius;}
  case EL::Beam:return Lamps;
  case EL::Touch:case EL::Wait:case EL::Irked:return LureHood;
  default:return {};
  }
 };
 FS47IdleLure::FIn In;
 In.bIdling=Truck!=nullptr;
 // Inside a visit already in their hours; this says so again by the tiers (a developer's test visit is let through).
 In.bHours=bTestNoticed||S47NightTiers::Allows(ES47Allowed::Full,FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.,FS47FreeformSession::NightOf(Clock));
 In.bFree=Body.IsSet()&&Visit==2&&!bHunting&&!RuleBreak.Active()&&(VisitWho==0||BodyClock>=(VisitWho==2?TallStands:1.6));
 In.bPlayerNear=M->World->bDriving||(Truck&&Flat(Feet,Mid)<FS47IdleLure::PlayerNear);
 In.BodyToTruck=Truck&&Body?Flat(*Body,Mid):100.;
 {const TOptional<FVector> Now=GoalOf(Lure.Part());In.ToGoal=Now&&Body?Flat(*Body,*Now):100.;}
 const EL Was=Lure.Part();
 Lure.Step(Dt,In,LureRoll>=0?LureRoll:FMath::FRand());
 if(Lure.Part()!=Was)
 {
  if(Was==EL::None)LureRoll=-1;
  if(Lure.Part()==EL::Circle&&Body)LureRingFrom=FMath::Atan2(Body->Z-Mid.Z,Body->X-Mid.X);
  UE_LOG(LogTemp,Display,TEXT("S47: idle lure: part %d, kind %d, %.1f m from the truck, player %.1f m from it"),int32(Lure.Part()),VisitWho,In.BodyToTruck,Truck?Flat(Feet,Mid):-1.);
 }
 // The hand on the hood: a handprint in the frost on the glass, the engine coughs twice, the lamps falter (below), a soft knock.
 if(Lure.TakeTouched()&&Truck)
 {
  Truck->SetFrost(FMath::Max(Truck->Frosted(),.6));Truck->SetFrostHand(true);FrostTruck=Truck;FrostDriven=0;
  if(FS47EngineSound* Eng=M->World->EngineSound())Eng->Cough(.9);
  LureCoughIn=1.4;
  if(M->Audio)M->Audio->Sfx(TEXT("metalTap"),FS47Sound{.Gain=.3f,.Rate=.6f,.At=LureHood+FVector(0,1,0)});
  if(M->Freeform->Attention)M->Freeform->Attention->Add(3,TEXT("a visit's body at the idling truck"));
  if(M->UI&&Flat(Feet,Mid)<90)M->UI->Thought(TEXT("Something is at the truck. The lights just stuttered."),4.f);
  if(Flat(Feet,Mid)<90&&!bTestNoticed)FS47Happenings::TellOther(M,TEXT("idle lure"),Place); // M1-MEASURE-20261113: told by the same thought
  UE_LOG(LogTemp,Display,TEXT("S47: idle lure: a hand on the hood, frost hand, the lamps falter"));
 }
 if(LureCoughIn>0)
 {
  LureCoughIn-=Dt;
  if(LureCoughIn<=0&&Truck)if(FS47EngineSound* Eng=M->World->EngineSound())Eng->Cough(.6);
 }
 const bool bCalm=M->Flow&&M->Flow->bCalmFlash;
 if(Truck&&Lure.Part()==EL::Touch)
 {
  if(LureLampTruck&&LureLampTruck!=Truck)RestoreLureLamp();
  Truck->SetLightLevel(Lure.LampLevel(bCalm));bLureLamp=true;LureLampTruck=Truck;
 }
 else if(bLureLamp)RestoreLureLamp();
 // Left alone at it: a heavy bang, a dent, and the engine stalls, dark. The key starts it again, not the first time (bKeyStall,
 // kept past the visit's end: bStallSet no longer clears it). Never more than that.
 if(Lure.TakeStruck()&&Truck)
 {
  if(M->Audio)
  {
   const FVector Bang=LureHood+FVector(0,.9,0);
   M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.95f,.Rate=.55f,.At=Bang});
   M->Audio->Sfx(TEXT("metalTap"),FS47Sound{.Gain=.5f,.Rate=.45f,.At=Bang,.When=.04f});
   M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.8f,.Rate=.5f,.At=Bang,.When=.32f});
  }
  if(bLureLamp)RestoreLureLamp();
  M->World->bKeyStall=true;bStallSet=false;
  M->World->EndIdle(true);
  if(M->Freeform->Attention)M->Freeform->Attention->Add(6,TEXT("a visit's body struck the idling truck"));
  if(M->UI&&Flat(Feet,Mid)<90)M->UI->Thought(TEXT("A bang from the truck. The engine has stopped, and the lights with it."),5.f);
  if(Flat(Feet,Mid)<90&&!bTestNoticed)FS47Happenings::TellOther(M,TEXT("idle lure"),Place); // M1-MEASURE-20261113
  M->Freeform->RecordEvidence(FindingId()+TEXT("_dent"),FString::Printf(TEXT("%s, %s. I had left the truck running with the lights on. Something came to it, went round it and stood in the lights, then struck the hood. The engine died. There is a dent in the hood the size of a hand, and a handprint in the frost on the glass."),*S47Script::PlayerClock(Clock),*Title));
  UE_LOG(LogTemp,Display,TEXT("S47: idle lure: it struck the hood, the engine stalled (the key wants turning)"));
 }
 if(Lure.TakeLetGo()){EndLure();UE_LOG(LogTemp,Display,TEXT("S47: idle lure: over, touched %d, struck %d"),Lure.bDidTouch,Lure.bDidStrike);return;}
 if(!Lure.Active()||!Body)return;
 const TOptional<FVector> Goal=GoalOf(Lure.Part());
 if(VisitWho!=0)
 {
  // The moved body: StepBody walks it there and turns it (it looks where it goes, then at the truck).
  if(Goal){LureGoal=*Goal;LureFace=Flat(*Body,*Goal)>FS47IdleLure::Reached*.6?*Goal:(Lure.Part()==EL::Beam?LureHood:Mid);}
  else{LureGoal=*Body;LureFace=LureHood;}
  return;
 }
 if(!G)return;
 // The small one has its own brain: it hears the engine (S47Noise) and goes to look, a stretch at a time; there, it stands and faces
 // the truck as if it heard something in it.
 const double Far=Goal?Flat(*Body,*Goal):0.;
 if(Goal&&Far>FS47IdleLure::Reached)
 {
  if(bLureHeld){bLureHeld=false;G->HoldAttention({},0,false);}
  LureNoiseIn-=Dt;
  if(LureNoiseIn<=0)
  {
   LureNoiseIn=1.;
   FVector Crumb=*Goal;
   if(Far>14)Crumb=*Body+FVector(Goal->X-Body->X,0,Goal->Z-Body->Z)/Far*14.;
   Crumb.Y=GroundAt(Crumb.X,Mid.Y,Crumb.Z);
   S47Noise::Make(Crumb+FVector(0,.3,0),2.,FS47IdleLure::NoiseName);
  }
 }
 else if(!Attention.Holding())
 {
  G->HoldAttention((Lure.Part()==EL::Beam?LureHood:Mid)+FVector(0,.9,0),0,true,SmallId);bLureHeld=true;
 }
}

void FS47FarmSite::EndLure()
{
 if(bLureLamp)RestoreLureLamp();
 if(bLureHeld){bLureHeld=false;if(M&&M->Night&&M->Night->Glimpses)M->Night->Glimpses->HoldAttention({},0,false);}
 LureGoal.Reset();LureFace.Reset();LureCoughIn=-1;
}

FS47Truck* FS47FarmSite::LiveTruck(FS47Truck* T) const
{
 return T&&M&&IsValid(M->World)&&M->World->OwnsTruck(T)?T:nullptr;
}

void FS47FarmSite::RestoreLureLamp()
{
 // On the truck that was dimmed, not on whatever idles now: got into, or died dark, IdleTruckNow is null (critic 12 Nov).
 bLureLamp=false;
 if(FS47Truck* T=LiveTruck(LureLampTruck))T->SetLightLevel(1);
 LureLampTruck=nullptr;
}

// The hand stays in the frost until the truck is driven again; then the defroster takes it in a few seconds.
void FS47FarmSite::StepFrost(double Dt)
{
 if(!FrostTruck||!M||!IsValid(M->World)||!M->World->bDriving)return;
 // Critic 12 Nov: a truck that is no longer the world's is let go, never touched.
 if(!LiveTruck(FrostTruck)){FrostTruck=nullptr;return;}
 FrostDriven+=Dt;
 const double K=FMath::Clamp(.6*(1.-(FrostDriven-4.)/6.),0.,.6);
 if(M->World->CabTruck()==FrostTruck)FrostTruck->SetFrost(K);
 if(K<=0){FrostTruck->SetFrost(0);FrostTruck->SetFrostHand(false);FrostTruck=nullptr;}
}
