#include "S47Predecessor.h"
#include "S47Album.h"
#include "S47Audio.h"
#include "S47Docs.h"
#include "S47FieldRadio.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47TownUse.h"
#include "S47Townsfolk.h"
#include "S47Visitors.h"
#include "S47FreeformCases.h"
#include "S47FreeformTerminal.h"
#include "S47FreeformWorld.h"
#include "S47GameStyle.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Legs.h"
#include "S47Mail.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47NightLog.h"
#include "S47Tape.h"
#include "S47WardReport.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47PredecessorText.h"
#include "S47RadioTips.h"
#include "S47Relief.h"
#include "S47RoadTerrain.h"
#include "S47SaroSave.h"
#include "S47Story.h"
#include "S47Taken.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "SS47Widgets.h"
#include "Widgets/Text/STextBlock.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/GarbageCollection.h"

static_assert(FS47Predecessor::CarriedWarn==S47PredecessorText::CarriedWarn,"Ward names the carried line's limit from the same sum");
static_assert(FS47Predecessor::TakenTooOften==3,"WeekNote's words for three stretches of lost time follow TakenTooOften");

namespace
{
 namespace T=S47FreeformTerminal;
 namespace X=S47PredecessorText;
 const TCHAR* const Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
 const TCHAR* const WagonBody=TEXT("/Game/Free/Vehicles/fairheaven_sw84/StaticMeshes/fairheaven_sw84_body.fairheaven_sw84_body");
 const TCHAR* const WagonDoor=TEXT("/Game/Free/Vehicles/fairheaven_sw84/StaticMeshes/fairheaven_sw84_door_driver.fairheaven_sw84_door_driver");
 const TCHAR* const WagonId=TEXT("wagon");
 const TCHAR* const RadioKey=TEXT("reyes-wagon-radio");
 const TCHAR* const RingKey=TEXT("reyes-phone-ring");
 // The storage room's west wall, between the stocked rack and the partition. x east, y up, z south: Three's terms.
 const FVector LockerA(26.31,0,-.75),LockerB(26.31,0,-.27),BoardAt(26.09,1.5,.16);
 // Off the old grade, west of the highway: road-local, as FS47Places has it.
 constexpr double WagonX=-300,WagonZ=830,WagonHeading=1.15,RadioReach=45;
 const TCHAR* const LongDays[]={TEXT("MONDAY"),TEXT("TUESDAY"),TEXT("WEDNESDAY"),TEXT("THURSDAY"),TEXT("FRIDAY"),TEXT("SATURDAY"),TEXT("SUNDAY")};
 const TCHAR* const LongMonths[]={TEXT("JANUARY"),TEXT("FEBRUARY"),TEXT("MARCH"),TEXT("APRIL"),TEXT("MAY"),TEXT("JUNE"),TEXT("JULY"),TEXT("AUGUST"),TEXT("SEPTEMBER"),TEXT("OCTOBER"),TEXT("NOVEMBER"),TEXT("DECEMBER")};
 const TCHAR* const OutcomeNames[]={TEXT(""),TEXT("stays"),TEXT("knows"),TEXT("closed-originals"),TEXT("closed"),TEXT("gone")};
 UStaticMeshComponent* Piece(AS47Web* Web,const TCHAR* Asset,const FString& Name,uint32 Colour)
 {
  auto* Mesh=LoadObject<UStaticMesh>(nullptr,Asset);if(!Mesh||!Web)return nullptr;
  auto* C=NewObject<UStaticMeshComponent>(Web,*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(Web->GetRootComponent());C->SetStaticMesh(Mesh);C->SetCastShadow(true);
  if(Colour!=0)
  {
   if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque")))
   {auto* Mat=UMaterialInstanceDynamic::Create(Base,Web);Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Colour));Mat->SetScalarParameterValue(TEXT("Roughness"),.85f);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);C->SetMaterial(0,Mat);}
  }
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();
  return C;
 }
 // Wagon 23 is white (the bible). The model's paint has no colour of its own to set, so the body's slots get a plain white of the
 // project's; glass, trim, tyres and badges keep their own. Story's wagon is not touched.
 int32 PaintWhite(AS47Web* Web,UStaticMeshComponent* C)
 {
  auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
  if(!C||!Web||!Base)return 0;
  int32 Painted=0;
  for(int32 I=0;I<C->GetNumMaterials();++I)
  {
   const UMaterialInterface* Was=C->GetMaterial(I);
   if(!Was||!Was->GetName().Contains(TEXT("Bodymat")))continue;
   auto* Mat=UMaterialInstanceDynamic::Create(Base,Web);Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(0xd9d7cf));Mat->SetScalarParameterValue(TEXT("Roughness"),.5f);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
   C->SetMaterial(I,Mat);++Painted;
  }
  return Painted;
 }
 // Text that wraps at the panel's own width from the first frame. S47Kit's AutoWrapText learns its width a frame late, and with
 // several paragraphs in a column the rows below it were laid out over the lines it had not yet found room for.
 constexpr float CssPx=1.25f; // the CSS pixel, as S47Kit has it
 S47Kit::FW Para(const FString& S,float PanelWidth,bool bSmall=false)
 {
  const float Wrap=(PanelWidth-44.f)*CssPx;
  return S47Kit::Readable(SNew(STextBlock).Text(FText::FromString(S)).Font(S47Style::Body((bSmall?13.5f:15.5f)*CssPx)).WrapTextAt(Wrap).LineHeightPercentage(1.15f)
   .ColorAndOpacity(bSmall?S47Kit::Css(0xa59e90):S47Kit::Css(0xddd5c2)).Margin(FMargin(0,bSmall?8.f:6.f,0,bSmall?2.f:6.f)));
 }
 void Pickable(UStaticMeshComponent* C)
 {
  C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
 }
}

// ---------- what the assessment goes by ----------
int32 FS47Predecessor::Points(const FTally& C)
{
 // Concluded: only lines with something behind them (Count). A filing on nothing is Guessed and earns nothing.
 int32 P=2*FMath::Min(C.Concluded,3);
 P+=C.Dishes>=24?2:C.Dishes>=20?1:0;
 // Records that can be read: most of what is filed, or at least half of it.
 if(C.Filed>0)P+=C.Clean*10>=C.Filed*7?2:C.Clean*2>=C.Filed?1:0;
 P+=C.Photos>=3?1:0;
 // What the land gave at night, kept as records of different kinds.
 P+=C.Land>=8?2:C.Land>=4?1:0;
 // M7: the count of cards earns nothing past the floor (FiledFloor, in Decide): "Filing cards is not what I am asked about".
 // Night logs her office has had and initialed: a station that reports its nights counts a little, once.
 P+=C.Logs>=LogsForPoint?1:0;
 // Tapes with something on them nobody heard: a recording is a record she can send on.
 P+=C.Tapes>=2?1:0;
 P+=FS47WardReport::PointsFor(C.Trust);
 return P;
}
FS47Predecessor::EOutcome FS47Predecessor::Decide(const FTally& C)
{
 // After "Not yet" only a further stretch of lost time sends somebody else (M7).
 if(C.Taken>=TakenTooOften&&C.Taken>C.Reprieved)return EOutcome::Gone;
 // Routine alone does not keep the station (M7): at least one case carried through, as her Monday notes say.
 const bool bWorks=C.Concluded>=CasesToStay&&C.Arrears<FS47FreeformSession::ArrearsCap&&C.Filed>=FiledFloor&&Points(C)>=PointsToStay;
 if(bWorks)return EOutcome::Stays;
 if(C.Known>=KnownEnough&&C.bWagon&&C.bGap)return EOutcome::KnowsEnough;
 if(C.Originals>0||C.Photos>0)return EOutcome::ClosedOriginals;
 return EOutcome::ClosedNothing;
}
const TCHAR* FS47Predecessor::NameOf(EOutcome O){return OutcomeNames[FMath::Clamp(int32(O),0,5)];}
FS47Predecessor::EOutcome FS47Predecessor::OutcomeOf(const FString& Name)
{
 for(int32 I=1;I<6;++I)if(Name==OutcomeNames[I])return EOutcome(I);
 return EOutcome::None;
}
FS47Predecessor::FTally FS47Predecessor::Count() const
{
 FTally C;C.Known=Known();C.bWagon=Found(WagonId);C.bGap=Found(TEXT("gap"));
 if(!M||!M->Freeform)return C;
 const auto& F=*M->Freeform;
 for(const auto& J:F.Jobs())
 {
  if(J.CaseTest<0&&J.Stage==FS47FreeformSession::EJobStage::Filed)
  {
   ++C.Filed;
   // A recording from before its quality was kept is given the benefit of the doubt.
   if(J.RecordingSoundVersion<2||J.RecordingQuality>=CleanQuality)++C.Clean;
  }
  if(J.bPrinted||!J.OriginalPage.IsEmpty())++C.Originals;
 }
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const auto* K=F.CaseFile().Get(Line);K&&K->bConcluded)
 {
  // A mystery takes a filing on any evidence (M7): it counts only with a control recorded or the other place looked at too.
  // The same sum the case board warns by before a filing (critic round 5): FS47FreeformSession::CaseSupported.
  if(F.CaseSupported(Line))++C.Concluded;else ++C.Guessed;
 }
 C.Dishes=F.AvailableDishCount();C.Arrears=F.Arrears();C.Taken=F.TimesTaken();C.Land=F.NightRecordKinds();C.Reprieved=TakenAtReprieve;
 if(F.NightLog)C.Logs=F.NightLog->ReadCount();
 if(F.Tape)C.Tapes=F.Tape->OddHeardCount();
 // The last week's report counts if it went whole, read or not (M7: it is due the day before she assesses).
 if(F.WardReport)C.Trust=F.WardReport->TrustAtReview();
 // Prints made in the camera: an enlargement is the same frame again, and is not counted twice (M7).
 if(M->Night&&M->Night->Album)for(const FS47AlbumShot& P:M->Night->Album->Shots())if(!P.bUnavailable&&!P.Url.IsEmpty()&&P.IsDeveloped()&&P.Enlarged.IsEmpty())++C.Photos;
 return C;
}
int32 FS47Predecessor::Known() const
{
 int32 N=0;
 for(const X::FTagDef& D:X::Tags())if(Seen.Contains(D.Doc))++N;
 // M7: wagon 23's trip meter and the thermos it led to count as well.
 for(const TCHAR* Id:{TEXT("locker"),WagonId,TEXT("gap"),TEXT("talk_deputy_ortiz"),TEXT("talk_ward_reyes"),TEXT("trip"),TEXT("talk_attendant_thermos")})if(Seen.Contains(Id))++N;
 return N;
}

// ---------- a game's life ----------
void FS47Predecessor::Begin(const S47J::FObj& Saved,bool bOlderGame,int32 Today)
{
 Stop();Life=MakeShared<uint8>(0);
 Seen.Reset();HeardOf.Reset();Tray.Reset();Outcome=EOutcome::None;bLeft=bPayReturned=bLetterTold=bRinging=bClosed=bStowed=bSawTutorial=bLegacyReprieve=bLegacyRead=false;CloseDay=0;LeftDay=0;LeftClock=0;NewsIn=8;TakenAtReprieve=GoneNoticed=KeptDay=0;bOutcomeRead=false;Salute.Reset();SaluteTime=0;SaluteStage=0;RingLeft=0;NoraNext=0;RadioOffDay=-1;NoraState=0;DaleDay=0;bBound=bBoundLand=false;
 DaleAnswered=DaleAnsweredDay=DaleMisses=DaleMissDay=DaleHoursTold=0;DaleLastClock=-1e9;DaleRetryClock=0;bDaleMissed=false;
 LateBulletinDay=LateThermosDay=LatePostDay=LateAwayDay=0;LateIn=0;LateRackAt=-1;LateMinuteAt=0;bLateLoop=bCairnHeard=bLateNow=false;LateLoopName.Empty();
 if(Saved.IsValid()&&Saved->Values.Num()>0)
 {
  bOlder=S47J::Bool(Saved,TEXT("older"));OpensDay=FMath::Max(1,S47J::Int(Saved,TEXT("opens"),1));AssessDay=FMath::Max(OpensDay,S47J::Int(Saved,TEXT("assess"),FirstAssess));WeekNoted=FMath::Max(0,S47J::Int(Saved,TEXT("weekNoted")));
  for(const FString& Id:S47J::Strs(Saved,TEXT("seen")))Seen.Add(Id);
  for(const FString& Id:S47J::Strs(Saved,TEXT("heard")))HeardOf.Add(Id);
  Tray=S47J::Strs(Saved,TEXT("tray"));
  Outcome=OutcomeOf(S47J::Str(Saved,TEXT("outcome")));RadioOffDay=S47J::Int(Saved,TEXT("radioOff"),-1);NoraState=FMath::Clamp(S47J::Int(Saved,TEXT("nora")),0,2);bPayReturned=S47J::Bool(Saved,TEXT("payReturned"));DaleDay=S47J::Int(Saved,TEXT("daleDay"));
  bLeft=S47J::Bool(Saved,TEXT("left"));LeftDay=S47J::Int(Saved,TEXT("leftDay"));LeftClock=S47J::Num(Saved,TEXT("leftClock"));
  CloseDay=FMath::Max(0,S47J::Int(Saved,TEXT("closeDay")));bClosed=S47J::Bool(Saved,TEXT("closed"));
  TakenAtReprieve=FMath::Max(0,S47J::Int(Saved,TEXT("reprieve")));GoneNoticed=FMath::Max(0,S47J::Int(Saved,TEXT("goneNoticed")));KeptDay=FMath::Max(0,S47J::Int(Saved,TEXT("keptDay")));
  // M7's weeks three and four (S47PredecessorLate.cpp).
  LateBulletinDay=FMath::Clamp(S47J::Int(Saved,TEXT("bulletin")),0,36500);LateThermosDay=FMath::Clamp(S47J::Int(Saved,TEXT("thermosDay")),0,36500);
  LatePostDay=FMath::Clamp(S47J::Int(Saved,TEXT("postDay")),0,36500);LateAwayDay=FMath::Clamp(S47J::Int(Saved,TEXT("awayDay")),0,36500);
  // A save from before the letter's reading was kept: read, unless it still lies in the tray, or (critic round 5) in the box at the
  // road or with the postman, which the first Step looks at once the post is loaded (SettleLegacy).
  bOutcomeRead=Outcome!=EOutcome::None&&(Saved->HasField(TEXT("outcomeRead"))?S47J::Bool(Saved,TEXT("outcomeRead")):!Tray.Contains(TEXT("assessment")));
  bLegacyRead=Outcome!=EOutcome::None&&!Saved->HasField(TEXT("outcomeRead"));
  // A "Not yet" from before M7 moved only the day (critic round 5): the times taken it gave the nights back at are taken on the first
  // Step, so that it is not undone the next morning.
  bLegacyReprieve=!Saved->HasField(TEXT("reprieve"))&&Outcome==EOutcome::None&&AssessDay>FirstAssessDay();
  // A closing outcome from before the notice was kept: the notice runs from the assessment.
  if(CloseDay==0&&(Outcome==EOutcome::KnowsEnough||Outcome==EOutcome::ClosedOriginals||Outcome==EOutcome::ClosedNothing))CloseDay=AssessDay+NoticeDays;
  // Reading one's own letter again after a Continue is fine; being told it is there, is not.
  bLetterTold=Seen.Contains(TEXT("letter"));
 }
 else if(bOlderGame)
 {
  // A save from before all this: it keeps its calendar (S47GameStyle), so nothing here can open before the 14th has gone by, and
  // its thirty days start now rather than at the start of a game already under way.
  bOlder=true;OpensDay=FMath::Max(Today,3);AssessDay=OpensDay+FirstAssess;WeekNoted=(Today-1)/7;
 }
 else{bOlder=false;OpensDay=1;AssessDay=FirstAssess;WeekNoted=0;}
 if(M&&IsValid(M->Web.Get())&&Scene.Get()!=M->Web.Get()){bBuiltSaro=bBuiltLand=false;KeptLabel.Reset();Land.Reset();WagonSpot=FSpot();Shown.Reset();Hidden.Reset();LockerParts.Reset();LockerTwoParts.Reset();BoardParts.Reset();}
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: opens day %d, assessment day %d, %d found%s"),OpensDay,AssessDay,Seen.Num(),bOlder?TEXT(", a save from before it"):TEXT(""));
}
void FS47Predecessor::Stop()
{
 Life.Reset();bRinging=false;bBound=bBoundLand=false;
 BackTo.Empty();LateRackAt=-1;bCairnHeard=false;
 // The UObject purge may have taken the scene before this plain C++ object goes, as in FS47FreeformSession::Stop.
 if(IsEngineExitRequested()||IsGarbageCollecting()||!IsValid(M)||M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed))return;
 if(IsValid(M->Audio)){if(bRadioLoop)M->Audio->Stop(RadioKey,.2f);M->Audio->Stop(RingKey,.1f);}
 LateQuiet();
 bRadioLoop=false;
 for(auto& P:Shown)if(auto* C=P.Get())C->SetVisibility(false);
 if(auto* L=KeptLabel.Get())L->SetVisibility(false);
 // A run up after KEPT OPEN broken off (critic round 5): each dish of it is sent back to where it was pointed, not left at the zenith.
 if(SaluteStage>0&&IsValid(M->Web.Get())&&Scene.Get()==M->Web.Get())for(const FSaluteDish& D:Salute)if(D.I<M->Web->DishCount()&&M->Web->DishRemoteAvailable(D.I))M->Web->PointDish(D.I,D.Az,D.El,3);
 Salute.Reset();SaluteStage=0;SaluteTime=0;
 if(IsValid(M->Web.Get()))
 {
  for(auto& R:M->Web->CollidersMutable(TEXT("saro")))if(R.Id==TEXT("ff:reyes-lockers"))R.bEnabled=false;
  for(const TCHAR* A:{TEXT("road"),TEXT("diner")})if(M->Web->HasWalkArea(A))for(auto& R:M->Web->CollidersMutable(A))if(R.Id==TEXT("ff:wagon23"))R.bEnabled=false;
 }
}
S47J::FObj FS47Predecessor::Save() const
{
 auto O=S47J::New();
 S47J::Set(O,TEXT("opens"),OpensDay);S47J::Set(O,TEXT("assess"),AssessDay);S47J::Set(O,TEXT("weekNoted"),WeekNoted);
 if(bOlder)S47J::Set(O,TEXT("older"),true);
 if(!Seen.IsEmpty()){TArray<FString> L=Seen.Array();L.Sort();S47J::Set(O,TEXT("seen"),L);}
 if(!HeardOf.IsEmpty()){TArray<FString> L=HeardOf.Array();L.Sort();S47J::Set(O,TEXT("heard"),L);}
 if(!Tray.IsEmpty())S47J::Set(O,TEXT("tray"),Tray);
 if(Outcome!=EOutcome::None)S47J::Set(O,TEXT("outcome"),NameOf(Outcome));
 if(RadioOffDay>=0)S47J::Set(O,TEXT("radioOff"),RadioOffDay);
 if(NoraState>0)S47J::Set(O,TEXT("nora"),NoraState);
 if(bPayReturned)S47J::Set(O,TEXT("payReturned"),true);
 if(DaleDay>0)S47J::Set(O,TEXT("daleDay"),DaleDay);
 if(bLeft){S47J::Set(O,TEXT("left"),true);S47J::Set(O,TEXT("leftDay"),LeftDay);S47J::Set(O,TEXT("leftClock"),LeftClock);}
 if(CloseDay>0)S47J::Set(O,TEXT("closeDay"),CloseDay);
 if(bClosed)S47J::Set(O,TEXT("closed"),true);
 if(TakenAtReprieve>0)S47J::Set(O,TEXT("reprieve"),TakenAtReprieve);
 if(GoneNoticed>0)S47J::Set(O,TEXT("goneNoticed"),GoneNoticed);
 if(KeptDay>0)S47J::Set(O,TEXT("keptDay"),KeptDay);
 if(Outcome!=EOutcome::None)S47J::Set(O,TEXT("outcomeRead"),bOutcomeRead);
 if(LateBulletinDay>0)S47J::Set(O,TEXT("bulletin"),LateBulletinDay);
 if(LateThermosDay>0)S47J::Set(O,TEXT("thermosDay"),LateThermosDay);
 if(LatePostDay>0)S47J::Set(O,TEXT("postDay"),LatePostDay);
 if(LateAwayDay>0)S47J::Set(O,TEXT("awayDay"),LateAwayDay);
 return O;
}
bool FS47Predecessor::Live() const{return M&&M->Freeform&&(bTest||M->Freeform->RulesLive());}
void FS47Predecessor::SettleLegacy(int32 Taken,bool bAssessmentPosted,int32 Today)
{
 if(bLegacyReprieve)
 {
  // The nights given back are kept: only a further stretch after this sends somebody else (GoneDue), not the next morning.
  bLegacyReprieve=false;
  if(TakenAtReprieve==0&&Taken>=TakenTooOften)
  {
   TakenAtReprieve=Taken;
   UE_LOG(LogTemp,Display,TEXT("S47: predecessor: a \"Not yet\" from before M7 kept at %d takings, assessment day %d"),TakenAtReprieve,AssessDay);
  }
 }
 if(bLegacyRead)
 {
  bLegacyRead=false;
  // Still in the box at the road, or on its way: not read, and OPS does not give it away.
  if(bOutcomeRead&&bAssessmentPosted)bOutcomeRead=false;
  // KEPT OPEN read before the finale was kept: it is had now (StepKept), from the day it was decided or today.
  if(Outcome==EOutcome::Stays&&bOutcomeRead&&!Seen.Contains(TEXT("kept"))){Seen.Add(TEXT("kept"));KeptDay=FMath::Max(AssessDay,Today);}
 }
}
bool FS47Predecessor::SaluteTarget(int32 I,double& Az,double& El) const
{
 if(SaluteStage<=0)return false;
 for(const FSaluteDish& D:Salute)if(D.I==I){Az=D.Az;El=D.El;return true;}
 return false;
}
void FS47Predecessor::Step(double Dt)
{
 FS47AssessMonthTest::Step(*this,Dt);
 if(!Live())return;
 auto& F=*M->Freeform;
 // A save from before M7, once the post is loaded (critic round 5).
 if(bLegacyReprieve||bLegacyRead)SettleLegacy(F.TimesTaken(),F.Mail&&(F.Mail->Holds(TEXT("ward:assessment"))||F.Mail->Coming(TEXT("ward:assessment"))),F.Day());
 if(F.InTutorial())bSawTutorial=true;
 BuildSaro();BuildLand();Bind();
 {
  // Placed once when built, and measured again only near the player, when there is ground there to measure.
  const FVector Road=S47Legs::ORIGINS(TEXT("road"));
  const auto Close=[&](const FSpot& P){return M->Player&&FVector2D(M->Player->Pos.X-(Road.X+P.X),M->Player->Pos.Z-(Road.Z+P.Z)).Size()<300;};
  for(FSpot& P:Land)if(!P.bSeated&&Close(P))Seat(P);
  if(!WagonSpot.bSeated&&!WagonSpot.Parts.IsEmpty()&&Close(WagonSpot))SeatWagon();
 }
 StepRadio();
 if(!BackTo.IsEmpty()&&!M->ModalOpen())
 {
  const FString Where=BackTo;BackTo.Empty();
  if(Where==TEXT("locker"))OpenLocker();else if(Where==TEXT("wagon"))OpenWagon();
 }
 if(bLeft)
 {
  // Gone, and the save with it, until the keys are taken back: a click beside the page does not put you back in the room.
  if(M->UI&&!M->ModalOpen()&&!M->bPausedByMenu&&!(M->Flow&&M->Flow->bStarting))OpenLeft();
  return;
 }
 StepRing(Dt);
 StepClosing();
 StepKept(Dt);
 if(F.Held())return;
 StepNews(Dt);
 StepLate(Dt);
}
void FS47Predecessor::StepNews(double Dt)
{
 auto& F=*M->Freeform;
 NewsIn-=Dt;if(NewsIn>0)return;NewsIn=2;
 if(M->World&&M->World->bBusy)return;
 const int32 Today=F.Day();
 if(Today<OpensDay)return;
 const FString Letter=TEXT("letter");
 // The first letter is in the box at the road when the shift starts, with the flag up (or in the tray, without post).
 if(!Seen.Contains(Letter)&&!Tray.Contains(Letter)&&!InMailbox(Letter))Send(Letter,true);
 // Ward's office is shut from 21:00 to 08:00: his letter and his answers wait in the tray and are told the next morning.
 const bool bWardOpen=WardAvailable();
 // Not during the first shift, which clears its own messages at the end: told after it, wherever the letter lies (M7).
 if(bWardOpen&&!bLetterTold&&!F.InTutorial()&&(Tray.Contains(Letter)||InMailbox(Letter)))
 {
  bLetterTold=true;
  // Critic round 5: right after the first shift the toast FIRST SHIFT COMPLETE and the HUD's card already say where the letter is;
  // a third time is noise. Told by toast after a Continue, or when the card is on something else.
  const bool bCardSays=bSawTutorial&&InMailbox(Letter)&&FS47FreeformSession::LetterWaits(F.Objective());
  if(M->UI&&!bCardSays)M->UI->Toast(InMailbox(Letter)?TEXT("There is a letter from Dr. Ward's office in the mailbox at the road."):TEXT("There is a letter from Dr. Ward's office in the OPS tray."),6);
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: Ward's letter %s, day %d, %s"),InMailbox(Letter)?TEXT("in the mailbox"):TEXT("in the tray"),Today,*DateText(Today));
  M->RequestAutosave();
 }
 // A third stretch of lost time is answered before anything else that morning (M7).
 StepWarnings();
 if(Outcome==EOutcome::Gone)return;
 // The Monday sheet; after KEPT OPEN, a shorter one with nothing to win.
 const int32 Week=(Today-1)/7;
 if(bWardOpen&&Week>WeekNoted&&Today>=OpensDay&&(Outcome==EOutcome::None||KeptOpen()))
 {
  WeekNoted=Week;const FString Id=FString::Printf(TEXT("week-%d"),Week);
  const FTally C=Count();
  const double Pace=double(C.Filed)/FMath::Max(1.,double(Today-OpensDay));
  F.RecordEvidence(TEXT("ward-")+Id,KeptOpen()?X::KeptNote(Week,C.Filed,C.Dishes,C.Arrears,DateText(FiscalEndDay()),C.Taken,C.Arrears>=CarriedWarn?LeverWords(C):FString())
   :X::WeekNote(Week,C.Filed,C.Concluded,C.Dishes,C.Clean,C.Arrears,FMath::Max(0,AssessDay-Today),DateText(AssessDay),Points(C),PointsToStay,C.Taken,C.Land,Decide(C)==EOutcome::Stays,LeverWords(C),C.Reprieved));
  // By post, with the next round (the flag says so), or in the tray where there is none.
  const bool bByPost=F.Mail&&F.Mail->Live();
  Send(Id);
  if(M->UI&&!bByPost)M->UI->Toast(TEXT("Dr. Ward's Monday note is in the OPS tray."),5);
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: week %d note, filed %d, pace %.2f, points %d of %d, taken %d"),Week,C.Filed,Pace,Points(C),PointsToStay,C.Taken);
  M->RequestAutosave();
 }
 // Day 22: the county's inspector writes ahead of the assessment (M7), about the first thing he will look at.
 if(bWardOpen&&Outcome==EOutcome::None&&Today>=OpensDay+InspectorAfter&&Today<AssessDay&&!Seen.Contains(TEXT("inspector")))
 {
  Seen.Add(TEXT("inspector"));
  const FTally C=Count();
  const int32 Weakest=C.Concluded<CasesToStay?0:C.Dishes<20?1:C.Filed<FiledFloor?2:C.Arrears>=CarriedWarn?3:C.Filed>0&&C.Clean*2<C.Filed?4:5;
  F.RecordEvidence(TEXT("ward-inspector"),X::Inspector(DateText(Today),DateText(AssessDay),Weakest));
  const bool bByPost=F.Mail&&F.Mail->Live();
  Send(TEXT("inspector"));
  if(M->UI&&!bByPost)M->UI->Toast(TEXT("A letter from the county inspector is in the OPS tray."),5);
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: the county inspector writes on day %d, first about %d"),Today,Weakest);
  M->RequestAutosave();
 }
 // The case's sheets turn up Reyes's note, once it is open.
 for(const X::FCaseNote& N:X::CaseNotes())
 {
  if(Seen.Contains(N.Doc)||Tray.Contains(N.Doc))continue;
  for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(FString(FS47FreeformCases::Def(Line).Title)==N.CaseTitle)
   if(const auto* K=F.CaseFile().Get(Line);K&&K->bOpen)
   {
    Tray.Add(N.Doc);
    if(M->UI)M->UI->Toast(TEXT("A note in Reyes's hand turned up among the old sheets for this case. It is in the OPS tray."),6);
    UE_LOG(LogTemp,Display,TEXT("S47: predecessor: case %s turned up %s"),N.CaseTitle,N.Doc);
    M->RequestAutosave();
   }
 }
 // What the air has said.
 // The CB lines about the carrier off the old grade and the white wagon on the flat (S47RadioTips.cpp, cb35 and cb36).
 if(F.Tips&&(F.Tips->Said().Contains(TEXT("cb35"))||F.Tips->Said().Contains(TEXT("cb36"))))Hear(WagonId);
 // Somebody who knows a little more than the rest telephones, once, when the player has gone far enough to be noticed.
 if(NoraState==0&&Known()>=5&&!bRinging)
 {
  const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.;
  if(Hour>=20&&Hour<23.5&&M->World&&M->World->Area==TEXT("saro")&&!M->World->bDriving&&!M->OneAtATime()&&F.Clock()>=NoraNext)
  {
   bRinging=true;RingLeft=45;NoraNext=F.Clock()+86400.;
   if(M->Audio){FS47Sound O;O.Gain=.45f;O.bLoop=true;M->Audio->LoopSfx(RingKey,TEXT("phoneRing"),O);}
   if(M->UI)M->UI->Toast(TEXT("The control room phone is ringing."),5);
   UE_LOG(LogTemp,Display,TEXT("S47: predecessor: the phone rings: %d known"),Known());
  }
 }
 StepDale();
 // The assessment. It costs nothing that was secured; it only reads it.
 if(Today>=AssessDay&&Outcome==EOutcome::None&&!F.InTutorial()&&!M->OneAtATime()&&!(M->World&&M->World->bDriving))
 {
  Outcome=Decide(Count());Decided();
  M->RequestAutosave(true);
 }
}
bool FS47Predecessor::Meets(const TCHAR* Needs) const
{
 if(!Needs)return true;
 const FString Need=Needs;
 if(Need==TEXT("known6"))return Known()>=6;
 if(!M||!M->Freeform)return Seen.Contains(Need);
 const auto& F=*M->Freeform;
 if(Need==TEXT("taken"))return F.TimesTaken()>0;
 if(Need==TEXT("taken2"))return F.TimesTaken()>=TakenTooOften-1;
 if(Need==TEXT("filed1")||Need==TEXT("filed10"))return Count().Filed>=(Need==TEXT("filed1")?1:10);
 if(Need==TEXT("branch1"))return F.Field&&F.Field->ArrayWear().bActive&&F.Field->ArrayWear().Restored.Num()>0;
 if(Need==TEXT("assess3"))return Outcome==EOutcome::None&&AssessDay-F.Day()<=3&&AssessDay>=F.Day();
 if(Need==TEXT("late")){const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.;return Hour>=2&&Hour<4.5;}
 // M7, weeks two to four: what has really gone on. The visitors' own flags (FS47Visitors::Has), somebody in Caliche gone from their
 // post, an assessment filed on any case, a case open, the array still half down, the last week before the assessment.
 if(Need.StartsWith(TEXT("visit:")))return F.Visitors&&F.Visitors->Has(Need.Mid(6));
 if(Need==TEXT("town:away"))return !LateAway().IsEmpty();
 if(Need==TEXT("filed:case")){for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const auto* K=F.CaseFile().Get(Line);K&&K->bConcluded)return true;return false;}
 // "case:6": open and not yet filed; "case:6+minute_chart": and that trace kept.
 if(Need.StartsWith(TEXT("case:"))){FString Num=Need.Mid(5),Kept;if(!Need.Mid(5).Split(TEXT("+"),&Num,&Kept))Num=Need.Mid(5);const auto* K=F.CaseFile().Get(FCString::Atoi(*Num));return K&&K->bOpen&&!K->bConcluded&&(Kept.IsEmpty()||F.HasFinding(Kept));}
 // Wagon 23 seen and its call-out read in the first weeks, and the trip meter not yet: somebody has been at it since.
 if(Need==TEXT("trip_due"))return Seen.Contains(WagonId)&&Seen.Contains(TEXT("callout"))&&!Seen.Contains(TEXT("trip"))&&F.Day()>=LateTripDay;
 if(Need==TEXT("array_low"))return F.AvailableDishCount()<20;
 if(Need==TEXT("assess7"))return Outcome==EOutcome::None&&AssessDay-F.Day()<=7&&AssessDay-F.Day()>3;
 return Seen.Contains(Need);
}
const S47PredecessorText::FBark* FS47Predecessor::NextBark() const
{
 const int32 Today=M&&M->Freeform?M->Freeform->Day():1;
 for(const X::FBark& B:X::Barks())if(!Seen.Contains(FString(TEXT("bark_"))+B.Id)&&Today>=B.FromDay&&Meets(B.Needs))return &B;
 return nullptr;
}
void FS47Predecessor::StepDale()
{
 auto& F=*M->Freeform;
 if(F.InTutorial()||DaleDay==F.Day()||!M->FieldRadio||M->ModalOpen())return;
 // One hail out at a time, and after one that nobody answered, a while before the next.
 if(M->FieldRadio->Hailing()||F.Clock()<DaleRetryClock)return;
 // Not into the middle of something: an event under way, a light that is taking someone, or something near enough to break up the
 // set. His call is a noise of its own, and the player did not choose it.
 if(M->OneAtATime()||(F.Taken&&F.Taken->Busy())||M->FieldRadio->PresenceLevel()>.01)return;
 // Not on top of another message: wait until the last one has had its time on the screen.
 if(M->UI){const FString Last=M->UI->LastDescription();if(Last!=ToastSeen){ToastSeen=Last;ToastAt=FPlatformTime::Seconds();}}
 if(FPlatformTime::Seconds()-ToastAt<9)return;
 const X::FBark* B=NextBark();if(!B)return;
 // WALKIE-USE-20261108: he hails first, and says it only to somebody who answers (T). No set in hand, or the set on another
 // channel: it waits, and is on a message slip by the phone (UsePhone).
 const FString Id=B->Id;const TWeakPtr<uint8> Guard=Life;
 FS47FieldRadio::FHail Words;Words.First=X::WalkieLine(TEXT("hail"));Words.Again=X::WalkieLine(TEXT("hail_again"));Words.GiveUp=X::WalkieLine(TEXT("hail_gone"));
 if(!M->FieldRadio->Hail(Words,[this,Guard,Id]{if(Guard.IsValid())SayBark(Id);},[this,Guard]{if(Guard.IsValid())MissedHail();}))return;
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: Dale hails on the walkie for %s, day %d"),*Id,F.Day());
}
void FS47Predecessor::SayBark(const FString& Id)
{
 if(!M||!M->FieldRadio||!M->Freeform)return;
 const X::FBark* B=nullptr;for(const X::FBark& Each:X::Barks())if(Id==Each.Id)B=&Each;
 // Already on the slip by the phone, or no longer his to say: a word of his own instead.
 FString Line=B&&!Seen.Contains(FString(TEXT("bark_"))+B->Id)?FString(B->Line):X::WalkieLine(TEXT("evening"));
 if(bDaleMissed)Line=X::WalkieLine(TEXT("finally"))+TEXT(" ")+Line;
 if(!M->FieldRadio->Message(FString(TEXT("DALE: "))+Line))return;
 bDaleMissed=false;DaleMisses=0;
 if(B){Seen.Add(FString(TEXT("bark_"))+B->Id);DaleDay=M->Freeform->Day();}
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: Dale on the walkie: %s, day %d"),*Id,M->Freeform->Day());
 M->RequestAutosave();
}
void FS47Predecessor::MissedHail()
{
 if(!M||!M->Freeform)return;
 auto& F=*M->Freeform;
 if(DaleMissDay!=F.Day()){DaleMissDay=F.Day();DaleMisses=0;}
 ++DaleMisses;bDaleMissed=true;
 // He tries again in a while; twice unanswered and it waits for the slip by the phone, or tomorrow.
 DaleRetryClock=F.Clock()+45*60.;
 if(DaleMisses>=2)
 {
  DaleDay=F.Day();
  // His line goes on the slip by the phone (UsePhone): the control room phone rings once, short, for whoever is in the building (M7).
  if(!bRinging&&M->Audio&&M->World&&M->World->Area==TEXT("saro")&&!M->World->bDriving)M->Audio->Sfx(TEXT("phoneRing"),FS47Sound{.Gain=.3f});
 }
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: Dale's hail went unanswered (%d today)"),DaleMisses);
}
const TCHAR* FS47Predecessor::DaleReply(const FDaleCall& C)
{
 const double HourNow=FMath::IsFinite(C.Hour)?FMath::Fmod(FMath::Max(0.,C.Hour),24.):20.;
 // Asleep, or on the road home: nobody at his bench, and nothing comes back.
 if((HourNow>=3.5&&HourNow<7.5)||(HourNow>=17.&&HourNow<18.))return nullptr;
 if(C.Answered>=4)return nullptr;
 if(C.Answered==3)return TEXT("enough");
 if(C.SinceLast<20*60.)return TEXT("again");
 if(C.bMissed)return TEXT("missed");
 if(C.bDriving)return TEXT("driving");
 if(C.bLostTime)return TEXT("hours");
 if(HourNow>=2.&&HourNow<3.5)return C.bInside?TEXT("late_in"):TEXT("late_out");
 if(HourNow<2.)return TEXT("midnight");
 if(HourNow<17.)return TEXT("day");
 if(C.bAtStation)return TEXT("station");
 if(C.bArrayDown)return TEXT("array");
 if(C.bFarOut)return TEXT("far");
 return TEXT("evening");
}
FString FS47Predecessor::AnswerWalkie()
{
 if(!Live()||!M||!M->Freeform||bLeft)return FString();
 auto& F=*M->Freeform;
 const double ClockNow=F.Clock();const int32 Today=F.Day();
 if(DaleAnsweredDay!=Today){DaleAnsweredDay=Today;DaleAnswered=0;}
 FDaleCall C;
 C.Hour=FMath::Fmod(FMath::Max(0.,ClockNow),86400.)/3600.;
 C.bDriving=M->World&&M->World->bDriving;
 C.bAtStation=!C.bDriving&&FS47FieldRadio::CbAtHand(M);
 C.bInside=F.Taken&&F.Taken->Indoors();
 C.bFarOut=!C.bInside&&!C.bDriving&&M->Web&&M->Web->Area()!=TEXT("saro");
 C.bMissed=bDaleMissed;C.bLostTime=F.TimesTaken()>DaleHoursTold;C.bArrayDown=F.AvailableDishCount()<24;
 C.SinceLast=ClockNow-DaleLastClock;C.Answered=DaleAnswered;
 const TCHAR* Id=DaleReply(C);
 if(!Id)
 {
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: Dale does not answer the walkie at %.1f h (%d today)"),C.Hour,DaleAnswered);
  return FString();
 }
 ++DaleAnswered;DaleLastClock=ClockNow;
 // A line of his that is due he says now, in place of the grumble: he was going to call anyway. Not to a call he has had enough of.
 const X::FBark* Due=FCString::Strcmp(Id,TEXT("again"))!=0&&FCString::Strcmp(Id,TEXT("enough"))!=0&&DaleDay!=Today&&!F.InTutorial()?NextBark():nullptr;
 if(Due)
 {
  Seen.Add(FString(TEXT("bark_"))+Due->Id);DaleDay=Today;bDaleMissed=false;DaleMisses=0;
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: called on the walkie, Dale says his due line %s"),Due->Id);
  M->RequestAutosave();
  return FString(TEXT("DALE: "))+Due->Line;
 }
 if(FCString::Strcmp(Id,TEXT("missed"))==0){bDaleMissed=false;DaleMisses=0;}
 if(FCString::Strcmp(Id,TEXT("hours"))==0)DaleHoursTold=F.TimesTaken();
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: Dale answers the walkie: %s at %.1f h"),Id,C.Hour);
 return FString(TEXT("DALE: "))+X::WalkieLine(Id);
}
FString FS47Predecessor::RelayLine(const FString& Tip) const
{
 if(!Live()||Tip.IsEmpty()||bLeft)return FString();
 return FString::Printf(TEXT("DALE: %s %s %s"),*X::WalkieLine(TEXT("relay_pre")),*Tip,*X::WalkieLine(TEXT("relay_post")));
}
FString FS47Predecessor::ServiceBand()
{
 if(!Live()||!M||!M->Player||!M->World||!RadioOn()||bLeft)return FString();
 if(M->World->Area!=TEXT("road")&&M->World->Area!=TEXT("diner"))return FString();
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 const double SpotX=WagonSpot.Parts.IsEmpty()?WagonX:WagonSpot.X,SpotZ=WagonSpot.Parts.IsEmpty()?WagonZ:WagonSpot.Z;
 // x east, z south: north is -z.
 const double ToEast=Road.X+SpotX-M->Player->Pos.X,ToNorth=-(Road.Z+SpotZ-M->Player->Pos.Z);
 const double CarrierDist=FMath::Sqrt(ToEast*ToEast+ToNorth*ToNorth);
 if(CarrierDist>1500)return FString();
 static const TCHAR* const Ways[]={TEXT("north"),TEXT("northeast"),TEXT("east"),TEXT("southeast"),TEXT("south"),TEXT("southwest"),TEXT("west"),TEXT("northwest")};
 const int32 Way=((FMath::RoundToInt(FMath::Atan2(ToEast,ToNorth)/(UE_DOUBLE_PI/4))%8)+8)%8;
 const bool bFirst=!HeardOf.Contains(WagonId);
 if(bFirst)Hear(WagonId);
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: wagon 23's carrier on the walkie's service band, %.0f m, to the %s%s"),CarrierDist,Ways[Way],bFirst?TEXT(", first time"):TEXT(""));
 return FString::Printf(TEXT("An open carrier on the service band, nobody on it. Strongest toward the %s.%s"),Ways[Way],bFirst?TEXT(" Somebody's set, keyed open and left."):TEXT(""));
}
void FS47Predecessor::StepWarnings()
{
 // Two stretches of lost time: Ward says in writing what a third does, before it can (TakenTooOften). Dale says it over the walkie
 // (the "twice" bark).
 auto& F=*M->Freeform;
 if(F.TimesTaken()>=TakenTooOften-1&&!Seen.Contains(TEXT("ward-taken"))&&Outcome==EOutcome::None)
 {
  Seen.Add(TEXT("ward-taken"));
  F.RecordEvidence(TEXT("ward-taken"),X::TakenWarning(DateText(F.Day()),F.TimesTaken()));
  const bool bByPost=F.Mail&&F.Mail->Live();
  Send(TEXT("ward-taken"));
  if(M->UI)M->UI->Toast(bByPost?TEXT("Dr. Ward has written about the hours in your log. The letter is in the mail."):TEXT("Dr. Ward has written about the hours in your log. It is in the OPS tray."),6);
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: warned after %d takings"),F.TimesTaken());
  M->RequestAutosave();
 }
 // M7: what the warning said happens, the morning after it is noticed, not on the assessment day. After "Not yet", only a
 // further stretch does it.
 const int32 Taken=F.TimesTaken();
 if(Outcome==EOutcome::None&&Taken>=TakenTooOften&&Taken>TakenAtReprieve)
 {
  if(GoneNoticed==0){GoneNoticed=F.Day();UE_LOG(LogTemp,Display,TEXT("S47: predecessor: taken %d times, noticed on day %d: somebody else has the nights tomorrow"),Taken,GoneNoticed);M->RequestAutosave();}
  else if(GoneDue(Taken,TakenAtReprieve,GoneNoticed,F.Day())&&WardAvailable()&&!F.InTutorial()&&!M->OneAtATime()&&!M->ModalOpen()&&!(M->World&&M->World->bDriving))
  {
   GoneNoticed=0;AssessDay=F.Day();Outcome=EOutcome::Gone;
   UE_LOG(LogTemp,Display,TEXT("S47: predecessor: the morning after the third stretch of lost time: the next operator starts, day %d"),F.Day());
   Decided();M->RequestAutosave(true);
  }
 }
}
void FS47Predecessor::StepClosing()
{
 auto& F=*M->Freeform;
 const bool bClosing=Outcome==EOutcome::KnowsEnough||Outcome==EOutcome::ClosedOriginals||Outcome==EOutcome::ClosedNothing;
 if(!bClosed&&bClosing&&CloseDay>0&&F.Day()>=CloseDay&&!F.InTutorial()&&!M->OneAtATime()&&!(M->World&&M->World->bDriving)&&!(M->Flow&&M->Flow->bStarting))Close();
 if(!bClosed||!M->Web||M->Web->Area()!=TEXT("saro"))return;
 // Stowed and off remote, every frame it is loaded: nothing at the station can point it again.
 for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I))M->Web->SetDishRemoteAvailable(I,false);
 if(!bStowed){bStowed=true;M->Web->PointDishes(0,90,2);}
}
void FS47Predecessor::StepKept(double Dt)
{
 if(!M||!M->Freeform)return;
 auto& F=*M->Freeform;
 // The second locker carries the operator who stays (the reverse of OpenLeft's "somebody else's name goes on the door").
 if(auto* L=KeptLabel.Get())L->SetVisibility(KeptOpen()&&Seen.Contains(TEXT("kept")));
 if(!KeptOpen()||!Seen.Contains(TEXT("kept")))return;
 // A reply by post to the routed pay, a few days on (M7; FS47Mail::PostWard, or the tray where there is no post).
 if(bPayReturned&&!Seen.Contains(TEXT("pay_reply_sent"))&&F.Day()>=KeptDay+PayReplyDays&&WardAvailable())
 {
  Seen.Add(TEXT("pay_reply_sent"));
  const bool bByPost=F.Mail&&F.Mail->Live();
  Send(TEXT("pay_reply"));
  if(M->UI&&!bByPost)M->UI->Toast(TEXT("A letter has come by way of Dr. Ward's office. It is in the OPS tray."),5);
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: an answer to the routed pay, day %d"),F.Day());
  M->RequestAutosave();
 }
 // The array: the reverse of the stow (StepClosing parks it and takes it off remote). Once, the first quiet daytime moment with the
 // player at the station: Dale runs every dish on remote up toward the zenith together and back to where it was pointed. Nothing
 // is left moved; a dish the player commands in between is let be.
 if(Seen.Contains(TEXT("kept-salute"))||!IsValid(M->Web.Get())||M->Web->Area()!=TEXT("saro"))return;
 auto& W=*M->Web;
 constexpr double Up=84,Slew=3,Longest=45;
 if(SaluteStage==0)
 {
  const double Hour=FMath::Fmod(FMath::Max(0.,F.Clock()),86400.)/3600.;
  if(Hour<8.5||Hour>=19.||F.AudioPlaying()||W.DishesMoving()||M->OneAtATime()||M->ModalOpen()||F.Held()||(M->World&&(M->World->bDriving||M->World->bBusy)))return;
  Salute.Reset();
  for(int32 I=0;I<W.DishCount();++I)if(W.DishRemoteAvailable(I)){FSaluteDish D;D.I=I;D.Az=W.DishTargetAz(I);D.El=W.DishTargetEl(I);Salute.Add(D);W.PointDish(I,D.Az,Up,Slew);}
  if(Salute.IsEmpty()){Seen.Add(TEXT("kept-salute"));return;}
  if(M->Audio)M->Audio->Sfx(TEXT("rack_switch"),FS47Sound{.Gain=.7f});
  SaluteStage=1;SaluteTime=0;
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: kept open: Dale runs %d dishes up from his bench"),Salute.Num());
  return;
 }
 // A page open holds the array (FS47FreeformSession::ArrayMotionHeld): the run waits for it.
 if(M->ModalOpen()||F.Held())return;
 SaluteTime+=Dt;
 if(SaluteStage==1&&(SaluteTime>Longest||(SaluteTime>2&&!W.DishesMoving())))
 {
  for(const FSaluteDish& D:Salute)if(W.DishRemoteAvailable(D.I)&&FMath::IsNearlyEqual(W.DishTargetEl(D.I),Up,.01))W.PointDish(D.I,D.Az,D.El,Slew);
  SaluteStage=2;SaluteTime=0;return;
 }
 if(SaluteStage==2&&(SaluteTime>Longest||(SaluteTime>2&&!W.DishesMoving())))
 {
  SaluteStage=0;Salute.Reset();Seen.Add(TEXT("kept-salute"));
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: kept open: the array is back where it was pointed"));
  M->RequestAutosave();
 }
}
int32 FS47Predecessor::FiscalEndDay() const
{
 if(!M||!M->Freeform)return AssessDay+34;
 const FDateTime First=M->Freeform->DateOfDay(1);
 return 1+int32((FDateTime(1986,6,30)-FDateTime(First.GetYear(),First.GetMonth(),First.GetDay())).GetTotalDays()+.5);
}
FString FS47Predecessor::LeverWords(const FTally& C) const
{
 return X::Levers(C.Logs>=LogsForPoint,C.Tapes>=2,FS47WardReport::PointsFor(C.Trust),C.Photos>=3,C.Guessed,C.Arrears,FS47FreeformSession::ArrearsCap);
}
void FS47Predecessor::Close()
{
 auto& F=*M->Freeform;
 bClosed=true;bStowed=false;
 const FString Page=X::ClosedNow(DateText(F.Day()));
 F.RecordEvidence(TEXT("saro-closed"),Page);
 Tray.AddUnique(TEXT("closed"));
 if(M->Audio)M->Audio->Sfx(TEXT("rack_switch"),FS47Sound{.Gain=.8f});
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: SARO closed on day %d: county power off, array stowed, truck tagged for the district"),F.Day());
 M->RequestAutosave(true);
 if(!M->UI)return;
 const TWeakPtr<uint8> Guard=Life;
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Paper(Page));
 Rows.Add(S47Kit::Button(TEXT("Walk the station one more time"),[this,Guard]{if(Guard.IsValid()&&M&&M->UI)M->UI->Close();},true));
 Rows.Add(S47Kit::Button(TitleButton(),[this,Guard]{if(Guard.IsValid())ToTitle(false);}));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO IS CLOSED"),S47Kit::Col(Rows),900));
}
void FS47Predecessor::StepRing(double Dt)
{
 if(!bRinging)return;
 RingLeft-=Dt;
 if(RingLeft<=0){bRinging=false;if(M->Audio)M->Audio->Stop(RingKey,.1f);UE_LOG(LogTemp,Display,TEXT("S47: predecessor: nobody answered; it will ring another night"));}
}
void FS47Predecessor::Decided()
{
 auto& F=*M->Freeform;const FTally C=Count();
 bOutcomeRead=false;
 const bool bMutedOutcome=Outcome==EOutcome::Gone;
 // A closing outcome gives a week's notice, and then it happens (StepClosing).
 if(Outcome==EOutcome::KnowsEnough||Outcome==EOutcome::ClosedOriginals||Outcome==EOutcome::ClosedNothing)CloseDay=F.Day()+NoticeDays;
 const FString Text=OutcomeLetter(Outcome);
 F.RecordEvidence(AssessKey(),Text);
 // The assessment comes by post like her other letters; the one to the next operator is read at once, as before.
 const bool bByPost=!bMutedOutcome&&F.Mail&&F.Mail->Live();
 if(bMutedOutcome)Tray.AddUnique(TEXT("assessment"));else Send(TEXT("assessment"));
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: assessment day %d: %s (filed %d, concluded %d, dishes %d, arrears %d, originals %d, photos %d, known %d, taken %d); nothing removed%s"),F.Day(),NameOf(Outcome),C.Filed,C.Concluded,C.Dishes,C.Arrears,C.Originals,C.Photos,C.Known,C.Taken,bByPost?TEXT(", by post"):TEXT(""));
 if(M->UI)
 {
  if(bMutedOutcome)OpenAssessment();
  else M->UI->Toast(bByPost?TEXT("Dr. Ward has made her assessment. It comes in the mail."):TEXT("Dr. Ward's assessment is in the OPS tray."),6);
 }
}

// ---------- dates and papers ----------
FString FS47Predecessor::DateText(int32 Day) const
{
 if(!M||!M->Freeform)return FString();
 const FDateTime D=M->Freeform->DateOfDay(Day);
 return FString::Printf(TEXT("%s, %s %d"),LongDays[int32(D.GetDayOfWeek())],LongMonths[D.GetMonth()-1],D.GetDay());
}
FString FS47Predecessor::LetterText() const
{
 if(!M||!M->Freeform)return FString();
 const FDateTime Dated=M->Freeform->DateOfDay(OpensDay)-FTimespan::FromDays(bOlder?0:3);
 const FString Line=FString::Printf(TEXT("%s %d, 1986"),LongMonths[Dated.GetMonth()-1],Dated.GetDay());
 return X::Letter(bOlder,Line,DateText(AssessDay));
}
FString FS47Predecessor::OutcomeLetter(EOutcome O) const
{
 const FTally C=Count();
 const FString Dated=DateText(M&&M->Freeform?M->Freeform->Day():AssessDay);
 const FString CloseOn=DateText(CloseDay>0?CloseDay:(M&&M->Freeform?M->Freeform->Day():AssessDay)+NoticeDays);
 switch(O)
 {
  case EOutcome::Stays:return X::Stays(Dated,C.Filed,C.Concluded,C.Dishes,C.Arrears,C.Known>=KnownEnough,X::Followed(Found(WagonId),Found(TEXT("talk_deputy_ortiz")),bPayReturned,LateThermosDay>0));
  case EOutcome::KnowsEnough:return X::KnowsEnough(Dated,C.Filed,C.Concluded,C.Dishes,C.Arrears,CloseOn);
  case EOutcome::ClosedOriginals:return X::ClosedOriginals(Dated,C.Filed,C.Concluded,C.Dishes,C.Arrears,C.Originals,C.Photos,CloseOn);
  case EOutcome::ClosedNothing:return X::ClosedNothing(Dated,C.Filed,C.Concluded,C.Dishes,C.Arrears,CloseOn);
  case EOutcome::Gone:return X::NextOperator(DateText((M&&M->Freeform?M->Freeform->Day():AssessDay)+3),Dated,C.Originals,C.Photos,C.Filed);
  default:return FString();
 }
}
FS47Doc FS47Predecessor::MakeDoc(const FString& Id) const
{
 FS47Doc D;D.Id=TEXT("reyes_")+Id;D.Kind=TEXT("typed");D.Stamp=TEXT("SARO / DUTY OFFICER");
 if(Id==TEXT("letter")){D.Title=TEXT("Letter from Dr. Ward");D.Page=LetterText();}
 else if(Id.StartsWith(TEXT("week-"))&&M&&M->Freeform){D.Title=FString::Printf(TEXT("Dr. Ward, %s"),*Id.Replace(TEXT("week-"),TEXT("week ")));D.Page=M->Freeform->FindingText(TEXT("ward-")+Id);}
 else if(Id==TEXT("assessment")&&M&&M->Freeform){D.Title=TEXT("Dr. Ward's assessment");D.Page=M->Freeform->FindingText(AssessKey());}
 else if(Id==TEXT("ward-taken")&&M&&M->Freeform){D.Title=TEXT("Dr. Ward, about the lost hours");D.Page=M->Freeform->FindingText(TEXT("ward-taken"));}
 else if(Id==TEXT("inspector")&&M&&M->Freeform){D.Title=TEXT("The county inspector, about the assessment");D.Stamp=TEXT("SOCORRO COUNTY");D.Page=M->Freeform->FindingText(TEXT("ward-inspector"));}
 else if(Id==TEXT("closed")&&M&&M->Freeform){D.Title=TEXT("Closing order in effect");D.Stamp=TEXT("SARO / COUNTY");D.Page=M->Freeform->FindingText(TEXT("saro-closed"));}
 else if(const X::FDocDef* Def=X::Doc(Id)){D.Title=Def->Title;D.Kind=Def->Kind;D.Stamp=Def->Stamp;D.Page=Def->Page;}
 D.Transcript=D.Page;
 return D;
}
void FS47Predecessor::ShowDoc(const FS47Doc& Doc,const FString& Back)
{
 if(!M||!M->UI)return;
 if(M->Story)M->Story->AddDoc(Doc);
 if(Back.IsEmpty()){M->UI->Document(Doc);return;}
 const TWeakPtr<uint8> Guard=Life;
 M->UI->Document(Doc,[this,Guard,Back]{if(Guard.IsValid())BackTo=Back;});
}
void FS47Predecessor::Mark(const FString& Id,const FString& Summary)
{
 const bool bNew=!Seen.Contains(Id);Seen.Add(Id);
 if(M&&M->Freeform&&!Summary.IsEmpty())M->Freeform->RecordEvidence(TEXT("reyes_")+Id,Summary);
 if(bNew)
 {
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: found %s, %d known"),*Id,Known());
  if(M)M->RequestAutosave();
 }
}
void FS47Predecessor::Hear(const FString& Id)
{
 if(Id.IsEmpty()||HeardOf.Contains(Id))return;
 HeardOf.Add(Id);
 if(Id!=WagonId&&M&&M->Freeform&&M->Freeform->Places)M->Freeform->Places->Hear(Id);
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: points to %s"),*Id);
 if(M)M->RequestAutosave();
}
void FS47Predecessor::ReadDoc(const FString& Id,const FString& Back)
{
 Tray.Remove(Id);
 const X::FDocDef* Def=X::Doc(Id);
 Mark(Id,Def?Def->Summary:TEXT(""));
 if(Def&&Def->Next)Hear(Def->Next);
 ShowDoc(MakeDoc(Id),Back);
}
void FS47Predecessor::OpenGap(){ReadDoc(TEXT("gap"));}
void FS47Predecessor::Send(const FString& Id,bool bNow)
{
 if(M&&M->Freeform&&M->Freeform->Mail&&M->Freeform->Mail->PostWard(Id,bNow))return;
 Tray.AddUnique(Id);
}
void FS47Predecessor::FromMail(const FString& Id)
{
 if(Id.IsEmpty())return;
 Tray.AddUnique(Id);
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: %s taken out of the mailbox, into the OPS tray"),*Id);
 if(M)M->RequestAutosave();
}
FString FS47Predecessor::PaperTitle(const FString& Id) const{return MakeDoc(Id).Title;}
bool FS47Predecessor::InMailbox(const FString& Id) const{return M&&M->Freeform&&M->Freeform->Mail&&M->Freeform->Mail->Holds(TEXT("ward:")+Id);}
FString FS47Predecessor::ArchiveLabel() const{return Live()?FString(TEXT("April's shift binder / the night of the 13th")):FString();}

// ---------- OPS ----------
void FS47Predecessor::OpsRows(TArray<TSharedRef<SWidget>>& Rows)
{
 if(!Live()||Tray.IsEmpty())return;
 TArray<S47Kit::FW> Col;
 for(const FString& Id:Tray)
 {
  const FString Title=MakeDoc(Id).Title;
  Col.Add(T::Button(TEXT("READ / ")+Title,[this,Id]
  {
   if(Id==TEXT("assessment")){OpenAssessment();return;}
   ReadDoc(Id);
  },true));
 }
 Rows.Add(T::Panel(TEXT("IN THE TRAY"),S47Kit::Col(Col)));
}
void FS47Predecessor::OpsKeys(TArray<TSharedRef<SWidget>>& Rows)
{
 if(!Live())return;
 Rows.Add(T::Button(TEXT("HAND IN YOUR KEYS / LEAVE SARO"),[this]{OpenKeys();}));
}
FString FS47Predecessor::StatusLine() const
{
 // Made, and not yet read: OPS does not give it away before the letter does (M7).
 if(Outcome!=EOutcome::None&&Outcome!=EOutcome::Gone&&!bOutcomeRead)return TEXT("UNDER REVIEW / ASSESSMENT MADE");
 switch(Outcome)
 {
  case EOutcome::Stays:return TEXT("KEPT OPEN");
  case EOutcome::KnowsEnough:case EOutcome::ClosedOriginals:case EOutcome::ClosedNothing:return bClosed?FString(TEXT("CLOSED")):FString::Printf(TEXT("CLOSING %s"),*DateText(CloseDay)).ToUpper();
  default:break;
 }
 if(!M||!M->Freeform)return TEXT("UNDER REVIEW");
 return FString::Printf(TEXT("UNDER REVIEW / DAY %d OF %d"),FMath::Clamp(M->Freeform->Day()-OpensDay+1,1,AssessDay-OpensDay+1),AssessDay-OpensDay+1);
}
FString FS47Predecessor::AccountLine() const{return Live()?StatusLine():FString(TEXT("IN SERVICE"));}
FString FS47Predecessor::TaskLine() const
{
 if(!Live())return FString();
 if(Tray.Contains(TEXT("letter")))return TEXT("OPS / There is a letter from Dr. Ward's office in the tray.");
 if(InMailbox(TEXT("letter")))return TEXT("MAILBOX / A letter from Dr. Ward's office is in the box at the road.");
 if(Tray.Contains(TEXT("assessment")))return TEXT("OPS / Dr. Ward's assessment is in the tray.");
 if(InMailbox(TEXT("assessment")))return TEXT("MAILBOX / Dr. Ward's assessment is in the box at the road.");
 // Critic round 5: her other papers while they wait, in the box or unread in the tray (ReadDoc takes them out of it): the inspector,
 // her letter about the lost hours, and the last Monday note, which says what the review still wants.
 if(InMailbox(TEXT("inspector")))return TEXT("MAILBOX / The county inspector's letter is in the box at the road.");
 if(Tray.Contains(TEXT("inspector")))return TEXT("OPS / There is a letter from the county inspector in the tray.");
 {
  const FString Monday=FString::Printf(TEXT("week-%d"),WeekNoted);
  for(const FString& Id:{FString(TEXT("ward-taken")),Monday})
  {
   const bool bNote=Id==Monday;
   if(InMailbox(Id))return bNote?TEXT("MAILBOX / Dr. Ward's Monday note is in the box at the road."):TEXT("MAILBOX / A letter from Dr. Ward's office is in the box at the road.");
   if(Tray.Contains(Id))return TEXT("OPS / There is a letter from Dr. Ward's office in the tray.");
  }
 }
 if(Outcome==EOutcome::None&&M&&M->Freeform)return FString::Printf(TEXT("SARO under review / Dr. Ward assesses the station on %s."),*DateText(AssessDay));
 // After the letter (M7): what the station is now, and until when.
 if(KeptOpen()&&M&&M->Freeform)return FString::Printf(TEXT("SARO KEPT OPEN / yours through %s."),*DateText(FiscalEndDay()));
 if(bOutcomeRead&&!bClosed&&CloseDay>0&&M&&M->Freeform&&(Outcome==EOutcome::KnowsEnough||Outcome==EOutcome::ClosedOriginals||Outcome==EOutcome::ClosedNothing))return FString::Printf(TEXT("SARO CLOSES %s / until then, the station is yours."),*DateText(CloseDay));
 return FString();
}
FString FS47Predecessor::ReviewLine() const
{
 if(!Live()||Outcome!=EOutcome::None)return FString();
 return FString::Printf(TEXT("\nSARO UNDER REVIEW. ASSESSMENT %s."),*DateText(AssessDay));
}
bool FS47Predecessor::WardAvailable() const{return !M||!M->Freeform||WardHours(M->Freeform->Clock());}
bool FS47Predecessor::UsePhone()
{
 if(!Live()||!M||!M->UI)return false;
 if(bRinging)
 {
  bRinging=false;NoraState=1;if(M->Audio)M->Audio->Stop(RingKey,.05f);
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: the phone answered"));
  ShowTalk(TEXT("nora"),FString());
  return true;
 }
 // Who to call: the duty officer, or the service line. Whatever Dale meant to say over a walkie you did not have is on a slip.
 TArray<S47Kit::FW> Rows;
 const TWeakPtr<uint8> Guard=Life;
 if(const X::FBark* B=NextBark())
 {
  Rows.Add(Para(TEXT("A message slip by the phone, in capitals, pressed hard enough to go through:"),760.f,true));
  Rows.Add(S47Kit::Paper(FString(B->Line).ToUpper()));
  Seen.Add(FString(TEXT("bark_"))+B->Id);DaleDay=M->Freeform?M->Freeform->Day():DaleDay;
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: Dale's slip by the phone: %s"),B->Id);
  M->RequestAutosave();
 }
 Rows.Add(Para(TEXT("The operations line. Two numbers on a card taped to the phone."),760.f));
 Rows.Add(S47Kit::Button(TEXT("Dale, field maintenance / the service line"),[this,Guard]{if(Guard.IsValid())ShowTalk(TEXT("dale"),FString());},true));
 const bool bWard=WardAvailable();
 if(!bWard)Rows.Add(Para(TEXT("A note under the card: \"Dr. Ward's office takes calls from 08:00 to 21:00. Anything urgent goes on the slip and waits for the morning.\""),760.f,true));
 Rows.Add(S47Kit::Button(bWard?FString(TEXT("Dr. Ward, duty officer")):FString(TEXT("Dr. Ward, duty officer / office closed until 08:00")),[this,Guard]{if(Guard.IsValid()&&WardAvailable())ShowTalk(TEXT("ward"),FString());},false,bWard));
 // TOWN-USE-20261108: Caliche's station takes tips on its request line, any hour (S47TownUse, FS47RadioTips).
 if(M->Freeform&&M->Freeform->TownUse)Rows.Add(S47Kit::Button(TEXT("1420 AM, the request line in Caliche"),[this,Guard]{if(Guard.IsValid()&&M&&M->Freeform&&M->Freeform->TownUse)M->Freeform->TownUse->OpenRadio(true);}));
 Rows.Add(S47Kit::Button(TEXT("Put it down"),[this]{if(M&&M->UI)M->UI->Close();}));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO OPERATIONS LINE"),S47Kit::Col(Rows),760));
 return true;
}

// ---------- people ----------
bool FS47Predecessor::Talk(const FString& Who)
{
 if(!M||!M->UI)return false;
 // Gone from their post (TOWNSFOLK-20261108): the empty place, and what the others say.
 if(M->Freeform&&M->Freeform->Townsfolk&&M->Freeform->Townsfolk->ShowAway(Who))return true;
 if(!X::Person(Who))
 {
  M->UI->Toast(TEXT("They look up, see the jacket, and go back to what they were doing."),4);
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: no words for %s in Freeform"),*Who);
  return true;
 }
 ShowTalk(Who,FString());
 return true;
}
void FS47Predecessor::ShowTalk(const FString& Who,const FString& TopicId)
{
 const X::FPerson* P=X::Person(Who);if(!P||!M||!M->UI||!M->Freeform)return;
 TArray<S47Kit::FW> Rows;
 const auto Say=[&Rows](const X::FSay& L)
 {
  const FString Line(L.Line);
  if(FCString::Strlen(L.Who)==0)Rows.Add(Para(Line,860.f,true));
  else Rows.Add(Para(FString(L.Who)+TEXT(": ")+Line,860.f));
 };
 const X::FTopic* Topic=TopicId.IsEmpty()?nullptr:P->Topics.FindByPredicate([&](const X::FTopic& Q){return TopicId==Q.Id;});
 if(!Topic)
 {
  for(const X::FSay& L:P->First)Say(L);
  // Today's one thing: what they remember of you, or what is going round town (TOWNSFOLK-20261108).
  if(M->Freeform->Townsfolk){const FString Said=M->Freeform->Townsfolk->Gossip(Who);if(!Said.IsEmpty())Rows.Add(Para(Said,860.f));}
 }
 else if(Who==TEXT("ward")&&TopicId==TEXT("review"))
 {
  const FTally C=Count();
  const auto Line=[&Rows](const FString& Text){Rows.Add(Para(Text,860.f));};
  Line(TEXT("YOU: Where does the review stand?"));
  if(Outcome==EOutcome::None)
  {
   Line(FString::Printf(TEXT("DR. WARD: %d filed, %d of them clean, %d cases carried through, the array at %d of 27, $%d carried. I assess on %s. I need something I can put in front of the county, and it has to be true."),C.Filed,C.Clean,C.Concluded,C.Dishes,C.Arrears,*DateText(AssessDay)));
   // What else she reads, in her words (M7): the same as on the Monday sheet.
   Line(TEXT("DR. WARD: ")+LeverWords(C));
  }
  else Line(TEXT("DR. WARD: You've had my letter."));
 }
 else
 {
  for(const X::FSay& L:Topic->Says)Say(L);
  const bool bNew=!Seen.Contains(FString::Printf(TEXT("talk_%s_%s"),*Who,*TopicId));
  Mark(FString::Printf(TEXT("talk_%s_%s"),*Who,*TopicId),Topic->Note?FString(Topic->Note):FString());
  if(Topic->Hears)Hear(Topic->Hears);
  if(bNew&&Who==TEXT("nora")&&NoraState<2)NoraState=2;
 }
 Rows.Add(S47Kit::Section(TEXT("ASK")));
 const TWeakPtr<uint8> Guard=Life;
 for(const X::FTopic& Q:P->Topics)
 {
  if(!Meets(Q.Needs))continue;
  if(TopicId==Q.Id)continue;
  const FString Id=Q.Id,Name=Who;
  Rows.Add(S47Kit::Button(Q.Label,[this,Guard,Name,Id]{if(Guard.IsValid())ShowTalk(Name,Id);}));
 }
 Rows.Add(S47Kit::Button(TEXT("Leave it there"),[this]{if(M&&M->UI)M->UI->Close();}));
 M->UI->Open(S47Kit::Shell(M->UI,P->Title,S47Kit::Col(Rows),860,true));
}

// ---------- the locker ----------
void FS47Predecessor::OpenLocker()
{
 if(!M||!M->UI||!M->Freeform)return;
 Mark(TEXT("locker"),TEXT("Reyes's locker at SARO. A mug, half a pack of cigarettes, a list for a Monday, a letter not sent, and mail nobody has opened."));
 TArray<S47Kit::FW> Rows;
 const TWeakPtr<uint8> Guard=Life;
 Rows.Add(Para(TEXT("A strip of label tape on the door: REYES. It isn't locked. It smells of old coffee and machine oil, and of nothing else."),860.f));
 Rows.Add(Para(TEXT("A white mug with a chip in the rim and a ring of coffee gone to varnish. Half a soft pack of cigarettes, gone square from a jacket pocket, and matches from the Mesa Diner. A spare work shirt on the hook. No jacket."),860.f,true));
 const auto Read=[&](const TCHAR* Id,const TCHAR* Label)
 {
  const FString Doc=Id;
  Rows.Add(S47Kit::Button(FString(Label)+(Seen.Contains(Doc)?TEXT(" (read)"):TEXT("")),[this,Guard,Doc]{if(Guard.IsValid())ReadDoc(Doc,TEXT("locker"));}));
 };
 Read(TEXT("note"),TEXT("A fuel slip with a list on the back"));
 Read(TEXT("list"),TEXT("A clipboard from Dale, field maintenance"));
 Read(TEXT("home"),TEXT("An envelope, stamped, not mailed"));
 Read(TEXT("pay"),TEXT("A pay envelope: FINAL, NOT CALLED FOR"));
 if(!Seen.Contains(TEXT("mail")))
 {
  Rows.Add(Para(TEXT("Mail has been pushed through the vent since. An insurance notice, an overdue card from the Socorro library, and a letter with a handwritten address, sealed."),860.f,true));
  Rows.Add(S47Kit::Button(TEXT("Open the sealed letter. It isn't yours"),[this,Guard]{if(Guard.IsValid())ReadDoc(TEXT("mail"),TEXT("locker"));}));
 }
 else Rows.Add(S47Kit::Button(TEXT("The letter you opened"),[this,Guard]{if(Guard.IsValid())ReadDoc(TEXT("mail"),TEXT("locker"));}));
 if(Seen.Contains(TEXT("pay")))
 {
  if(bPayReturned)Rows.Add(Para(TEXT("The pay envelope has gone to Ward's office, with your initials on the routing slip."),860.f,true));
  else Rows.Add(S47Kit::Button(TEXT("Route the pay envelope back to Ward's office"),[this,Guard]
  {
   if(!Guard.IsValid())return;
   bPayReturned=true;UE_LOG(LogTemp,Display,TEXT("S47: predecessor: Reyes's final pay sent back to Ward"));
   if(M->UI)M->UI->Toast(TEXT("It goes in the outgoing tray. Somebody should have to look at it."),4);
   if(M)M->RequestAutosave();OpenLocker();
  }));
 }
 Rows.Add(S47Kit::Button(TEXT("Close the door"),[this]{if(M&&M->UI)M->UI->Close();}));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("LOCKER / REYES"),S47Kit::Col(Rows),860));
}

// ---------- wagon 23 ----------
bool FS47Predecessor::RadioOn() const{return !M||!M->Freeform||RadioOffDay!=M->Freeform->Day();}
void FS47Predecessor::OpenWagon()
{
 if(!M||!M->UI||!M->Freeform)return;
 const bool bFirst=!Seen.Contains(WagonId);
 Mark(WagonId,TEXT("Wagon 23, on the flat off the old grade. A white district wagon under a county tag, door open, radio on, nobody near it."));
 Hear(WagonId);
 auto& F=*M->Freeform;
 TArray<S47Kit::FW> Rows;
 const TWeakPtr<uint8> Guard=Life;
 Rows.Add(Para(TEXT("A white station wagon with an amber bar on the roof and DISTRICT MAINTENANCE 23 on the doors. It stands nose out into nothing, on the flat, and you can't make out a track leading to it. The driver's door is open. Dust on the seat, and the seat run well forward."),860.f));
 Rows.Add(Para(TEXT("A county tag wired to the mirror: S.O. HOLD. DO NOT MOVE. In the dust on the dash, a clean ring where something round has stood."),860.f,true));
 if(RadioOn())
 {
  Rows.Add(Para(RadioOffDay>=0?FString::Printf(TEXT("The radio is on again. You switched it off yourself, on %s. Service band, squelch open, nothing on it but a carrier."),*DateText(RadioOffDay)):FString(TEXT("The radio is on. Service band, squelch open, nothing on it but a carrier. The dome light is dead. The battery ought to be.")),860.f));
  Rows.Add(S47Kit::Button(TEXT("Switch the radio off"),[this,Guard]
  {
   if(!Guard.IsValid()||!M->Freeform)return;
   RadioOffDay=M->Freeform->Day();
   if(M->Audio)M->Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.5f});
   UE_LOG(LogTemp,Display,TEXT("S47: predecessor: wagon radio off, day %d"),RadioOffDay);
   M->RequestAutosave();OpenWagon();
  }));
 }
 else Rows.Add(Para(TEXT("You switched the radio off. It clicks as it cools."),860.f,true));
 Rows.Add(S47Kit::Button(FString(TEXT("The clipboard on the passenger seat"))+(Seen.Contains(TEXT("callout"))?TEXT(" (read)"):TEXT("")),[this,Guard]{if(Guard.IsValid())ReadDoc(TEXT("callout"),TEXT("wagon"));}));
 // M7, Reyes's trail after wagon 23: from the third week somebody has wiped the dust off the trip meter, and only off that.
 if((F.Day()>=LateTripDay||bLateNow)&&Seen.Contains(TEXT("callout")))
 {
  Rows.Add(Para(Seen.Contains(TEXT("trip"))?FString(TEXT("The trip meter, wiped clean in the dust of the dash."))
   :M->Freeform->FindingDay(TEXT("reyes_wagon"))>=LateTripDay?FString(TEXT("Somebody has wiped the dust off the dash, and only off the trip meter.")):FString(TEXT("Somebody has been here since you were. The dust on the dash is wiped off the trip meter, and only off that.")),860.f,true));
  Rows.Add(S47Kit::Button(FString(TEXT("Read the trip meter"))+(Seen.Contains(TEXT("trip"))?TEXT(" (read)"):TEXT("")),[this,Guard]{if(Guard.IsValid())ReadDoc(TEXT("trip"),TEXT("wagon"));}));
 }
 Rows.Add(S47Kit::Button(TEXT("Leave it how you found it"),[this]{if(M&&M->UI)M->UI->Close();}));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("WAGON 23"),S47Kit::Col(Rows),860));
 if(bFirst&&F.Day()>0)UE_LOG(LogTemp,Display,TEXT("S47: predecessor: wagon 23 found on day %d"),F.Day());
}

// ---------- the assessment ----------
void FS47Predecessor::OpenAssessment()
{
 if(!M||!M->UI||!M->Freeform)return;
 auto& F=*M->Freeform;
 Tray.Remove(TEXT("assessment"));
 // Read: OPS may say it now (StatusLine). Kept open, the first time: the rack clicks over onto next year's meter (M7, the reverse
 // of the closing order's), and the station's finale follows (StepKept).
 const bool bFirstRead=!bOutcomeRead&&Outcome!=EOutcome::None;
 bOutcomeRead=Outcome!=EOutcome::None;
 if(bFirstRead&&Outcome==EOutcome::Stays&&!Seen.Contains(TEXT("kept")))
 {
  Seen.Add(TEXT("kept"));KeptDay=F.Day();
  if(M->Audio)M->Audio->Sfx(TEXT("rack_switch"),FS47Sound{.Gain=.8f});
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: kept open, letter read on day %d"),KeptDay);
 }
 // In a real game the letter is what was written on the day; the developer's menu asks for each in turn.
 const FString Letter=bTest||F.FindingText(AssessKey()).IsEmpty()?OutcomeLetter(Outcome):F.FindingText(AssessKey());
 FS47Doc D=MakeDoc(TEXT("assessment"));D.Page=Letter;D.Transcript=Letter;if(M->Story)M->Story->AddDoc(D);
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Paper(Letter));
 const TWeakPtr<uint8> Guard=Life;
 const auto Back=[this,Guard](const TCHAR* Label,bool bPrimary)
 {
  return S47Kit::Button(Label,[this,Guard]{if(Guard.IsValid()&&M&&M->UI)M->UI->Close();},bPrimary);
 };
 const auto Keys=[this,Guard]{return S47Kit::Button(TEXT("Hang your keys on the board and go"),[this,Guard]{if(Guard.IsValid())OpenKeys();});};
 switch(Outcome)
 {
 case EOutcome::Stays:
  Rows.Add(S47Kit::Button(TEXT("Go on"),[this,Guard]{if(Guard.IsValid())OpenKept();},true));Rows.Add(Keys());break;
 case EOutcome::KnowsEnough:
  Rows.Add(Para(X::WhatYouKnow(),900.f));
  Rows.Add(Back(TEXT("Stay out the notice"),true));Rows.Add(Keys());break;
 case EOutcome::ClosedOriginals:case EOutcome::ClosedNothing:
  Rows.Add(Back(TEXT("Stay out the notice"),true));Rows.Add(Keys());break;
 case EOutcome::Gone:
  Rows.Add(Para(TEXT("You read this over someone's shoulder. It was opened on the desk you sat at, by the operator who starts tonight. Nobody at the station expects you back. Your originals and photographs are where you left them."),900.f,true));
  Rows.Add(S47Kit::Button(TEXT("Start the shift as the new operator"),[this,Guard]{if(Guard.IsValid())ToTitle(true);},true));
  Rows.Add(S47Kit::Button(TitleButton(),[this,Guard]{if(Guard.IsValid())ToTitle(false);}));
  // Once (critic round 5): after one "Not yet" a further stretch of lost time ends it, as "There will not be a second time" said.
  if(OffersNotYet())Rows.Add(S47Kit::Button(TEXT("Not yet. Go back to the station, and Ward's review with it"),[this,Guard]
  {
   if(!Guard.IsValid()||!M->Freeform)return;
   NotYet();if(M->UI)M->UI->Close();
  }));
  break;
 default:Rows.Add(Back(TEXT("Back to work"),true));break;
 }
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: assessment read: %s"),NameOf(Outcome));
 M->UI->Open(S47Kit::Shell(M->UI,Outcome==EOutcome::Gone?TEXT("TO THE NIGHT OPERATOR, SARO"):TEXT("ASSESSMENT"),S47Kit::Col(Rows),900));
 M->RequestAutosave();
}

// ---------- the keys ----------
void FS47Predecessor::NotYet()
{
 // Once (critic round 5): an old page or a stray call after the first reprieve gives no second one.
 if(!M||!M->Freeform||!OffersNotYet())return;
 // A real reprieve (M7): only a further stretch of lost time sends somebody else; otherwise the review goes by the work. Never
 // sooner than the first assessment was set for.
 TakenAtReprieve=M->Freeform->TimesTaken();GoneNoticed=0;
 Outcome=EOutcome::None;bOutcomeRead=false;AssessDay=FMath::Max(M->Freeform->Day()+14,FirstAssessDay());
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: gone taken back at %d takings: next assessment day %d"),TakenAtReprieve,AssessDay);
 M->RequestAutosave();
}
void FS47Predecessor::OpenKept()
{
 if(!M||!M->UI||!M->Freeform)return;
 auto& F=*M->Freeform;const FTally C=Count();
 TArray<S47Kit::FW> Rows;
 Rows.Add(Para(FString::Printf(TEXT("%s, 1986. SARO stays open through the end of the fiscal year, %s. Somewhere behind the control room a relay has clicked over onto the county's new meter."),*DateText(KeptDay>0?KeptDay:F.Day()),*DateText(FiscalEndDay())),940.f));
 Account(Rows,C,940.f,true);
 Rows.Add(Para(TEXT("The files on Reyes and on Mara Ortiz stay with the county. The station stays with you. Three jobs come up on the OPS terminal each morning at 08:00, as before, and Dr. Ward's notes come on Mondays."),940.f,true));
 const TWeakPtr<uint8> Guard=Life;
 Rows.Add(S47Kit::Button(TEXT("Back to work. The station is yours"),[this]{if(M&&M->UI)M->UI->Close();},true));
 Rows.Add(S47Kit::Button(TitleButton(),[this,Guard]{if(Guard.IsValid())ToTitle(false);}));
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: the page after KEPT OPEN"));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO STAYS OPEN"),S47Kit::Col(Rows),940));
}
void FS47Predecessor::OpenKeys()
{
 if(!M||!M->UI||!M->Freeform)return;
 TArray<S47Kit::FW> Rows;
 const TWeakPtr<uint8> Guard=Life;
 Rows.Add(Para(TEXT("A plywood board with six brass hooks, three of them numbered. Hook 1 has nothing on it, and hasn't for some time. Your keys are in your hand."),760.f));
 Rows.Add(Para(TEXT("You don't owe anybody a phone call. If you hang them up, your shift here is over, and you get in your own car and go."),760.f));
 Rows.Add(Para(TEXT("This ends the game here, with what you have done so far. Your save stays, and you can take the keys back."),760.f,true));
 Rows.Add(S47Kit::Button(TEXT("Hang your keys on the hook and leave SARO"),[this,Guard]{if(Guard.IsValid())Leave();},true));
 Rows.Add(S47Kit::Button(TEXT("Keep the keys"),[this]{if(M&&M->UI)M->UI->Close();}));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("THE KEY BOARD"),S47Kit::Col(Rows),760));
}
void FS47Predecessor::Leave()
{
 if(!M||!M->Freeform)return;
 auto& F=*M->Freeform;
 bLeft=true;LeftDay=F.Day();LeftClock=F.Clock();
 if(M->Audio)M->Audio->Sfx(TEXT("keys"),FS47Sound{.Gain=.6f});
 const FTally C=Count();
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: keys on the board, day %d: filed %d, originals %d, photos %d, known %d, %s"),LeftDay,C.Filed,C.Originals,C.Photos,C.Known,NameOf(Outcome));
 M->RequestAutosave(true);
 OpenLeft();
}
void FS47Predecessor::OpenLeft()
{
 if(!M||!M->UI||!M->Freeform)return;
 auto& F=*M->Freeform;const FTally C=Count();
 const int32 Day=LeftDay>0?LeftDay:F.Day();
 const int32 Seconds=int32(FMath::Fmod(FMath::Max(0.,LeftClock),86400.));
 TArray<S47Kit::FW> Rows;
 Rows.Add(Para(FString::Printf(TEXT("%s, 1986. %02d:%02d. The keys swing a little on the hook, and stop."),*DateText(Day),Seconds/3600,(Seconds/60)%60),940.f));
 Account(Rows,C,940.f);
 // What becomes of SARO, from the numbers you leave.
 FString Saro;
 const EOutcome Would=Outcome!=EOutcome::None?Outcome:Decide(C);
 if(Outcome==EOutcome::Stays||(Outcome==EOutcome::None&&Would==EOutcome::Stays))Saro=TEXT("Ward reads the sheet you left and hires a relief operator out of Albuquerque. SARO stays open through the summer. Somebody else's name goes on the door next to Reyes's.");
 else if(Outcome==EOutcome::None)Saro=FString::Printf(TEXT("With nobody on nights, the assessment doesn't take long. On %s Dr. Ward signs the closing order. The dishes are stowed, the power is cut at the county line, and the binder goes in a box with your originals in it."),*DateText(AssessDay));
 else Saro=TEXT("The closing order was already signed. The dishes are stowed a little early, the power is cut at the county line, and the binder goes in a box with your originals in it.");
 Rows.Add(Para(Saro,940.f));
 Rows.Add(Para(C.Taken>0?FString::Printf(TEXT("It's a long way to anywhere. You drive it with the radio off. When they ask, you don't mention the hours you can't account for. %s."),C.Taken==1?TEXT("There were some"):TEXT("There were more than a few")):FString(TEXT("It's a long way to anywhere. You drive it with the radio off. Nobody stops you. In the first town with a phone, you think about it, and you buy a coffee instead.")),940.f,true));
 const TWeakPtr<uint8> Guard=Life;
 Rows.Add(Para(TEXT("Your save stays. You can take the keys back and go on from where you left."),940.f,true));
 Rows.Add(S47Kit::Button(TEXT("Take the keys back"),[this,Guard]{if(Guard.IsValid())TakeKeysBack();},true));
 Rows.Add(S47Kit::Button(TEXT("Return to the title"),[this,Guard]{if(Guard.IsValid())ToTitle(false);}));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("YOU LEFT SARO"),S47Kit::Col(Rows),940));
}
void FS47Predecessor::Account(TArray<TSharedRef<SWidget>>& Rows,const FTally& C,float Width,bool bStaying) const
{
 if(!M||!M->Freeform)return;
 const auto& F=*M->Freeform;
 // What you leave behind: only what is really so.
 TArray<FString> Open;int32 Unfiled=0;
 for(int32 Line=0;Line<FS47FreeformCases::CaseCount;++Line)if(const auto* K=F.CaseFile().Get(Line);K&&K->bOpen&&!K->bConcluded)Open.Add(FS47FreeformCases::Def(Line).Title);
 for(const auto& J:F.Jobs())if(J.CaseTest<0&&J.Stage>FS47FreeformSession::EJobStage::New&&J.Stage<FS47FreeformSession::EJobStage::Filed)++Unfiled;
 FString Behind=C.Filed>0?(bStaying?FString::Printf(TEXT("On file: %d jobs, and %d begun and not finished."),C.Filed,Unfiled):FString::Printf(TEXT("You leave %d jobs filed and %d begun and not finished."),C.Filed,Unfiled)):FString(bStaying?TEXT("Nothing is on file yet."):TEXT("You leave nothing filed."));
 Behind+=C.Originals+C.Photos>0?FString::Printf(TEXT(" %d originals in the binder and %d photographs in the album, all of them where they belong."),C.Originals,C.Photos):FString(TEXT(" No originals in the binder and no photographs in the album."));
 Behind+=FString::Printf(TEXT(" The array stands at %d of 27. The budget is $%d, and $%d is carried on the sheet."),C.Dishes,F.Budget(),C.Arrears);
 if(!Open.IsEmpty())Behind+=FString::Printf(TEXT(" Still open on the case board: %s."),*FString::Join(Open,TEXT(", ")));
 Rows.Add(Para(Behind,Width));
 // Reyes: what you did and did not go into.
 int32 Tags=0;for(const X::FTagDef& D:X::Tags())if(Seen.Contains(D.Doc))++Tags;
 FString About;
 if(!Seen.Contains(TEXT("locker")))About=TEXT("You never opened the locker next to yours. It still has Reyes's name on it.");
 else
 {
  About=Seen.Contains(TEXT("mail"))?TEXT("The letter you opened is back in Reyes's locker, with the twenty still in it."):TEXT("Reyes's mail is still in the locker, sealed.");
  About+=bPayReturned?TEXT(" The final pay has gone back to Ward, with your initials on the slip."):Seen.Contains(TEXT("pay"))?TEXT(" The final pay is where you found it, not called for."):TEXT("");
 }
 if(Tags>0)About+=FString::Printf(TEXT(" You read %d of Reyes's survey tags%s."),Tags,Tags<X::Tags().Num()?TEXT(", and the rest are still wired where they were left"):TEXT(""));
 if(Seen.Contains(WagonId))About+=RadioOn()?TEXT(" Wagon 23 is still on the flat, with its radio on."):TEXT(" Wagon 23 is still on the flat. You switched its radio off. You know how much that's worth.");
 Rows.Add(Para(About,Width));
}
void FS47Predecessor::TakeKeysBack()
{
 bLeft=false;LeftDay=0;LeftClock=0;
 if(!M)return;
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: keys taken back"));
 if(M->UI){M->UI->Close();M->UI->Toast(TEXT("You take your keys back off the hook."),4);}
 M->RequestAutosave(true);
}
bool FS47Predecessor::AwayFromStation() const
{
 return M&&M->Flow&&!M->Flow->bDevRun&&M->IsFreeform()&&!S47SaroSave::InStation(M);
}
FString FS47Predecessor::TitleButton() const
{
 return AwayFromStation()?FString(TEXT("Return to the title. Continue goes back to the log filed when you left SARO")):FString(TEXT("Return to the title. This save stays"));
}
void FS47Predecessor::ToTitle(bool bNextOperator)
{
 if(!M||!M->Flow)return;
 FString Err;
 // M7-E: the log is filed only inside the building (S47SaroSave::StationBlock). Away from it nothing is written now, and Continue
 // goes back to the departure filed on leaving, as the button said.
 const bool bAway=AwayFromStation();
 if(bAway){UE_LOG(LogTemp,Display,TEXT("S47: predecessor: to the title away from SARO: nothing filed, Continue returns to the last departure"));}
 else if(!M->Flow->bDevRun&&!M->Flow->AutosaveNow(&Err)){if(M->UI)M->UI->Toast(TEXT("Could not save just now: ")+Err,5);return;}
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: to the title%s"),bNextOperator?TEXT(", as the next operator"):TEXT(""));
 US47Flow* Flow=M->Flow;
 Flow->ToTitle();
 if(bNextOperator)Flow->RequestNewFreeform();
}

// ---------- the world ----------
void FS47Predecessor::BindOne(const FString& Id,const FS47Obj& Obj,double Range,TFunction<FString()> Label,TFunction<void()> Use)
{
 if(!Obj||!M||!IsValid(M->Inter)||!M->Web)return;
 const TWeakPtr<uint8> Guard=Life;
 FS47Interactable I;I.Id=Id;I.Object=Obj;I.Range=Range;
 I.Label=[Guard,Label]{return Guard.IsValid()?Label():FString();};
 I.Use=[Guard,Use]{if(Guard.IsValid())Use();};
 I.Examine=I.Label;
 M->Inter->Add(I);M->Web->MarkUsable(Obj.I,Id);
}
bool FS47Predecessor::BuildSaro()
{
 if(bBuiltSaro)return true;
 if(!M||!M->Freeform||!M->Freeform->Field||!IsValid(M->Web.Get())||!M->Web->IsLoaded()||!IsValid(M->Inter))return false;
 const int32 Parent=M->Web->Handle(TEXT("ext.group"));if(Parent<0)return false;
 auto& Field=*M->Freeform->Field;
 Scene=M->Web;
 const auto Stage=[&](const FString& Name,const FVector& At,const FVector& Size,uint32 Colour)->FS47Obj
 {
  FS47Obj O=Field.StageBox(Name,At,Size,Colour,Parent);
  if(O)if(const auto* N=M->Web->Node(O.I))if(N->Comp.IsValid())Shown.Add(N->Comp.Get());
  return O;
 };
 // A bank of two, simple shapes: they want a real model some day.
 const auto Locker=[&](const FString& Name,const FVector& At,TArray<FS47Obj>& Into)->FS47Obj
 {
  const FS47Obj Body=Stage(Name,At+FVector(0,.91,0),FVector(.46,1.82,.46),0x56635a);
  Into.Add(Stage(Name+TEXT("Door"),At+FVector(.235,.91,0),FVector(.015,1.72,.41),0x4c5850));
  for(int32 I=0;I<3;++I)Into.Add(Stage(FString::Printf(TEXT("%sVent%d"),*Name,I),At+FVector(.245,1.55+I*.05,0),FVector(.005,.015,.22),0x1c201d));
  Into.Add(Stage(Name+TEXT("Latch"),At+FVector(.25,1.,.15),FVector(.02,.13,.025),0x9a9d94));
  return Body;
 };
 LockerObj=Locker(TEXT("S47ReyesLocker"),LockerA,LockerParts);
 LockerTwoObj=Locker(TEXT("S47NextLocker"),LockerB,LockerTwoParts);
 LockerParts.Add(Stage(TEXT("S47ReyesLockerLabel"),LockerA+FVector(.247,1.74,0),FVector(.004,.045,.24),0xd8d4c6));
 BoardObj=Stage(TEXT("S47KeyBoard"),BoardAt,FVector(.02,.24,.34),0x7a5a3a);
 for(int32 I=0;I<6;++I)BoardParts.Add(Stage(FString::Printf(TEXT("S47KeyHook%d"),I),BoardAt+FVector(.025,I<3?.05:-.05,-.11+(I%3)*.11),FVector(.03,.012,.012),0xb09a5a));
 auto* Name=NewObject<UTextRenderComponent>(M->Web.Get(),TEXT("S47ReyesLockerName"));
 Name->SetMobility(EComponentMobility::Movable);Name->SetupAttachment(M->Web->GetRootComponent());
 Name->SetText(FText::FromString(TEXT("REYES")));Name->SetWorldSize(3.6f);Name->SetHorizontalAlignment(EHTA_Center);Name->SetVerticalAlignment(EVRTA_TextCenter);Name->SetTextRenderColor(FColor(30,30,28));
 Name->SetCollisionEnabled(ECollisionEnabled::NoCollision);Name->RegisterComponent();
 Name->SetWorldLocation(S47Three::ToUE(LockerA+FVector(.251,1.74,0)));Name->SetWorldRotation(FRotator(0,0,0));
 Shown.Add(Name);
 // After KEPT OPEN (StepKept): the second locker is the operator's. Not among Shown: StepKept says when it shows.
 auto* Ours=NewObject<UTextRenderComponent>(M->Web.Get(),TEXT("S47NextLockerName"));
 Ours->SetMobility(EComponentMobility::Movable);Ours->SetupAttachment(M->Web->GetRootComponent());
 Ours->SetText(FText::FromString(TEXT("NIGHT OPS")));Ours->SetWorldSize(3.2f);Ours->SetHorizontalAlignment(EHTA_Center);Ours->SetVerticalAlignment(EVRTA_TextCenter);Ours->SetTextRenderColor(FColor(216,212,198));
 Ours->SetCollisionEnabled(ECollisionEnabled::NoCollision);Ours->RegisterComponent();
 Ours->SetWorldLocation(S47Three::ToUE(LockerB+FVector(.251,1.74,0)));Ours->SetWorldRotation(FRotator(0,0,0));Ours->SetVisibility(false);
 KeptLabel=Ours;
 auto& Rects=M->Web->CollidersMutable(TEXT("saro"));
 if(!Rects.ContainsByPredicate([](const FS47Rect& R){return R.Id==TEXT("ff:reyes-lockers");}))Rects.Add({LockerA.X-.24,LockerA.X+.24,LockerA.Z-.24,LockerB.Z+.24,TEXT("ff:reyes-lockers"),true});
 bBuiltSaro=true;bBound=false;
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: locker bank and key board at the station"));
 return true;
}
bool FS47Predecessor::BuildLand()
{
 if(bBuiltLand)return true;
 if(!M||!M->Freeform||!M->Freeform->Places||!M->Player||!IsValid(M->Web.Get())||!IsValid(M->Inter))return false;
 const int32 Root=M->Web->Handle(TEXT("world.legs.road.area.group"));if(Root<0)return false;
 auto* Web=M->Web.Get();
 Scene=M->Web;
 // A SARO survey marker: a white lath, a blue flag, a manila tag, and a little more to point at than that.
 for(const X::FTagDef& D:X::Tags())
 {
  double PX=0,PZ=0;if(!M->Freeform->Places->Spot(D.Place,PX,PZ))continue;
  FSpot Spot;Spot.Id=D.Doc;Spot.X=PX+D.OffX;Spot.Z=PZ+D.OffZ;
  const auto Part=[&](const FString& Suffix,const FVector& Local,const FVector& Size,uint32 Colour)->UStaticMeshComponent*
  {
   auto* C=Piece(Web,Cube,FString::Printf(TEXT("S47Reyes_%s_%s"),D.Doc,*Suffix),Colour);if(!C)return nullptr;
   C->SetWorldScale3D(FVector(Size.X,Size.Z,Size.Y));Spot.Parts.Add(C);Spot.Local.Add(Local);Spot.Turn.Add(FRotator::ZeroRotator);Shown.Add(C);
   return C;
  };
  Part(TEXT("Lath"),FVector(0,.65,0),FVector(.05,1.3,.022),0xd8d4c6);
  Part(TEXT("Flag"),FVector(.13,1.2,0),FVector(.24,.14,.006),0x2f5f9a);
  Part(TEXT("Tag"),FVector(.05,.85,.016),FVector(.075,.13,.004),0xc9b27d);
  // The lath alone is hard to point at. This is what the eye's ray finds; nobody sees it.
  if(auto* Hit=Part(TEXT("Pick"),FVector(0,.8,0),FVector(1.,1.7,1.),0))
  {
   Pickable(Hit);Hit->SetHiddenInGame(true);Shown.Pop();Hidden.Add(Hit);
   Spot.Hit=Web->ObjAt(Web->AdoptNode(Hit,Root,FString::Printf(TEXT("S47Reyes_%s_Hit"),D.Doc)));
  }
  Land.Add(Spot);
 }
 // Wagon 23: the same district wagon Story's call-out comes in, and nobody in it.
 bool bClear=false;double WX=WagonX,WZ=WagonZ;
 for(double R=0;R<=260&&!bClear;R+=26)for(int32 I=0;I<(R>0?8:1)&&!bClear;++I)
 {
  const double TX=WagonX+FMath::Cos(I*UE_DOUBLE_PI/4)*R,TZ=WagonZ+FMath::Sin(I*UE_DOUBLE_PI/4)*R;
  if(!S47OpenLand::InRim(TX,TZ,20)||S47OpenLand::InSaroCore(TX,TZ,15)||S47OpenLand::InStationCore(TX,TZ,15)||FMath::Abs(TX)<16||S47Relief::HillStops(TX,TZ,5))continue;
  WX=TX;WZ=TZ;bClear=true;
 }
 WagonSpot=FSpot();WagonSpot.Id=WagonId;WagonSpot.X=WX;WagonSpot.Z=WZ;
 if(auto* Body=Piece(Web,WagonBody,TEXT("S47Wagon23"),0))
 {
  WagonSpot.Parts.Add(Body);Shown.Add(Body);int32 Painted=PaintWhite(Web,Body);
  if(auto* Door=Piece(Web,WagonDoor,TEXT("S47Wagon23Door"),0)){WagonSpot.Parts.Add(Door);Shown.Add(Door);Painted+=PaintWhite(Web,Door);}
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: wagon 23 painted white: %d slots"),Painted);
  for(int32 I=0;I<3;++I)if(auto* Bar=Piece(Web,Cube,FString::Printf(TEXT("S47Wagon23Bar%d"),I),I==1?0x9aa3a6:0xc9862a)){WagonSpot.Parts.Add(Bar);Shown.Add(Bar);}
  for(int32 Side=0;Side<2;++Side)
  {
   auto* L=NewObject<UTextRenderComponent>(Web,*FString::Printf(TEXT("S47Wagon23Label%d"),Side));
   L->SetMobility(EComponentMobility::Movable);L->SetupAttachment(Web->GetRootComponent());L->SetText(FText::FromString(TEXT("DISTRICT\nMAINTENANCE  23")));L->SetWorldSize(9.f);
   L->SetHorizontalAlignment(EHTA_Center);L->SetVerticalAlignment(EVRTA_TextCenter);L->SetTextRenderColor(FColor(28,54,92));L->SetCollisionEnabled(ECollisionEnabled::NoCollision);L->RegisterComponent();
   WagonSpot.Parts.Add(L);Shown.Add(L);
  }
  if(auto* Hit=Piece(Web,Cube,TEXT("S47Wagon23Pick"),0))
  {
   Pickable(Hit);Hit->SetHiddenInGame(true);WagonSpot.Parts.Add(Hit);Hidden.Add(Hit);
   WagonSpot.Hit=Web->ObjAt(Web->AdoptNode(Hit,Root,TEXT("S47Wagon23Hit")));
  }
 }
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: wagon 23 stands at road %.0f %.0f%s; %d survey tags"),WX,WZ,bClear?TEXT(""):TEXT(" (no clear ground found: as planned)"),Land.Num());
 for(FSpot& P:Land)Seat(P);
 if(!WagonSpot.Parts.IsEmpty())SeatWagon();
 bBuiltLand=true;bBoundLand=false;
 return true;
}
void FS47Predecessor::Bind()
{
 if(!M||!IsValid(M->Web.Get())||(bBound&&bBoundLand))return;
 for(auto& P:Shown)if(auto* C=P.Get())C->SetVisibility(true);
 if(bBuiltSaro&&!bBound)
 {
 bBound=true;
 for(auto& R:M->Web->CollidersMutable(TEXT("saro")))if(R.Id==TEXT("ff:reyes-lockers"))R.bEnabled=true;
 if(LockerObj)
 {
  BindOne(TEXT("ff:reyes:locker"),LockerObj,2.6,[this]{return Seen.Contains(TEXT("locker"))?FString(TEXT("Locker: REYES")):FString(TEXT("Locker: REYES / open it"));},[this]{OpenLocker();});
  for(const FS47Obj& O:LockerParts)if(O)M->Web->MarkUsable(O.I,TEXT("ff:reyes:locker"));
 }
 if(LockerTwoObj)
 {
  BindOne(TEXT("ff:reyes:locker2"),LockerTwoObj,2.6,[this]{return KeptOpen()?FString(TEXT("Locker: NIGHT OPS")):FString(TEXT("Locker: no name on it"));},[this]{if(M&&M->UI)M->UI->Toast(KeptOpen()?TEXT("Ward's office has been by with the label tape. It's yours now."):TEXT("Empty, and clean. It's yours if you want it. Nobody has told you so."),4);});
  for(const FS47Obj& O:LockerTwoParts)if(O)M->Web->MarkUsable(O.I,TEXT("ff:reyes:locker2"));
 }
 if(BoardObj)
 {
  BindOne(TEXT("ff:reyes:keys"),BoardObj,2.6,[this]{return bLeft?FString():FString(TEXT("Key board / hang up your keys"));},[this]{OpenKeys();});
  for(const FS47Obj& O:BoardParts)if(O)M->Web->MarkUsable(O.I,TEXT("ff:reyes:keys"));
 }
 }
 if(!bBuiltLand||bBoundLand)return;
 bBoundLand=true;
 for(const FSpot& P:Land)
 {
  const FString Doc=P.Id;
  BindOne(TEXT("ff:reyes:")+Doc,P.Hit,3.4,[this,Doc]{return Seen.Contains(Doc)?FString(TEXT("A SARO survey tag / in Reyes's hand")):FString(TEXT("A SARO survey tag / read it"));},[this,Doc]{ReadDoc(Doc);});
 }
 BindOne(TEXT("ff:reyes:wagon"),WagonSpot.Hit,4.5,[this]{return Seen.Contains(WagonId)?FString(TEXT("Wagon 23")):FString(TEXT("A white wagon / look closer"));},[this]{OpenWagon();});
 for(const TCHAR* A:{TEXT("road"),TEXT("diner")})if(M->Web->HasWalkArea(A))for(auto& R:M->Web->CollidersMutable(A))if(R.Id==TEXT("ff:wagon23"))R.bEnabled=true;
}
double FS47Predecessor::GroundAt(double RoadX,double RoadZ,const FSpot& Spot,bool& bHit) const
{
 bHit=false;
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 double Y=Road.Y+S47RoadTerrain::SurfaceY(RoadX,RoadZ);
 if(!M||!IsValid(M->Web.Get()))return Y;
 if(UWorld* Wld=M->Web->GetWorld())
 {
  FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47PredecessorGround),true);
  for(const auto& P:Spot.Parts)if(auto* C=Cast<UPrimitiveComponent>(P.Get()))Q.AddIgnoredComponent(C);
  if(Wld->LineTraceSingleByChannel(Hit,S47Three::ToUE(FVector(Road.X+RoadX,Y+30,Road.Z+RoadZ)),S47Three::ToUE(FVector(Road.X+RoadX,Y-30,Road.Z+RoadZ)),ECC_Visibility,Q)){Y=S47Three::ToThree(Hit.ImpactPoint).Y;bHit=true;}
 }
 return Y;
}
void FS47Predecessor::Seat(FSpot& Spot)
{
 if(!M||!M->Player||!IsValid(M->Web.Get()))return;
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 const FVector World(Road.X+Spot.X,0,Road.Z+Spot.Z);
 const double Near=FVector2D(M->Player->Pos.X-World.X,M->Player->Pos.Z-World.Z).Size();
 bool bHit=false;const double Y=GroundAt(Spot.X,Spot.Z,Spot,bHit);
 for(int32 I=0;I<Spot.Parts.Num();++I)if(auto* C=Spot.Parts[I].Get())C->SetWorldLocation(S47Three::ToUE(FVector(World.X,Y,World.Z)+Spot.Local[I]));
 Spot.bSeated=bHit&&Near<300;
}
void FS47Predecessor::SeatWagon()
{
 if(!M||!M->Player||!IsValid(M->Web.Get()))return;
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 bool bHit=false;const double Y=GroundAt(WagonSpot.X,WagonSpot.Z,WagonSpot,bHit);
 const FVector At(Road.X+WagonSpot.X,Y,Road.Z+WagonSpot.Z);
 const FQuat Car=S47Three::Euler(0,WagonHeading,0);
 const FVector Up(0,1,0),Right=Car.RotateVector(FVector(1,0,0)),Fwd=Car.RotateVector(FVector(0,0,-1));
 const auto Pose=[](USceneComponent* C,const FVector& Where,const FQuat& Q)
 {
  if(!C)return;double E[16];S47Three::Compose(Where,Q,FVector::OneVector,E);
  C->SetWorldTransform(S47Three::MatToUE(E));C->AddLocalRotation(FRotator(0,-90,0));
 };
 int32 I=0;
 const auto Next=[&]()->USceneComponent*{return WagonSpot.Parts.IsValidIndex(I)?WagonSpot.Parts[I++].Get():nullptr;};
 Pose(Next(),At,Car);
 {
  // The driver's door stands open, as it was found.
  const FVector Hinge(-.73,.72,-.86);const FQuat Leaf=S47Three::Euler(0,-1.05,0);
  Pose(Next(),At+Car.RotateVector(Hinge-Leaf.RotateVector(Hinge)),Car*Leaf);
 }
 for(int32 B=0;B<3;++B)if(auto* Bar=Next())
 {
  Bar->SetWorldLocation(S47Three::ToUE(At+Up*1.52+Right*((B-1)*.42)+Fwd*.25));Bar->SetWorldRotation(FRotator(0,-FMath::RadiansToDegrees(WagonHeading)-90,0));Bar->SetWorldScale3D(FVector(.36,.12,.09));
 }
 for(int32 Side=0;Side<2;++Side)if(auto* L=Next())
 {
  const double S=Side==0?1:-1;
  L->SetWorldLocation(S47Three::ToUE(At+Up*.55+Right*(S*.93)+Fwd*.6));L->SetWorldRotation(FRotator(0,-FMath::RadiansToDegrees(WagonHeading)+(S>0?0:180),0));
 }
 if(auto* Pick=Next())
 {
  Pick->SetWorldLocation(S47Three::ToUE(At+Up*.9));Pick->SetWorldRotation(FRotator(0,-FMath::RadiansToDegrees(WagonHeading)-90,0));Pick->SetWorldScale3D(FVector(5.1,1.9,1.8));
 }
 for(const TCHAR* A:{TEXT("road"),TEXT("diner")})
 {
  if(!M->Web->HasWalkArea(A))continue;
  auto& Rects=M->Web->CollidersMutable(A);
  int32 Index=Rects.IndexOfByPredicate([](const FS47Rect& R){return R.Id==TEXT("ff:wagon23");});
  if(Index==INDEX_NONE){FS47Rect R;R.Id=TEXT("ff:wagon23");Index=Rects.Add(R);}
  const double CH=FMath::Abs(FMath::Cos(WagonHeading)),SH=FMath::Abs(FMath::Sin(WagonHeading)),HalfW=.95,HalfL=2.55;
  const double Ex=CH*HalfW+SH*HalfL,Ez=SH*HalfW+CH*HalfL;
  Rects[Index].MinX=At.X-Ex;Rects[Index].MaxX=At.X+Ex;Rects[Index].MinZ=At.Z-Ez;Rects[Index].MaxZ=At.Z+Ez;Rects[Index].bEnabled=true;
 }
 const double Near=FVector2D(M->Player->Pos.X-At.X,M->Player->Pos.Z-At.Z).Size();
 WagonSpot.bSeated=bHit&&Near<300;
}
void FS47Predecessor::StepRadio()
{
 if(!M||!M->Player||!IsValid(M->Audio))return;
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 const FVector At(Road.X+WagonSpot.X,M->Player->Pos.Y,Road.Z+WagonSpot.Z);
 const double Dist=FVector2D(M->Player->Pos.X-At.X,M->Player->Pos.Z-At.Z).Size();
 const bool bNear=!WagonSpot.Parts.IsEmpty()&&Dist<RadioReach&&RadioOn()&&!bLeft&&M->World&&(M->World->Area==TEXT("road")||M->World->Area==TEXT("diner"));
 if(bNear&&!bRadioLoop)
 {
  // The set's own static, from what the game already has. Nothing new was recorded.
  FS47Sound O;O.Gain=.22f;O.bLoop=true;O.At=At;
  M->Audio->LoopSfx(RadioKey,TEXT("radio_static"),O);bRadioLoop=true;
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: wagon 23's radio in earshot, %.0f m"),Dist);
 }
 else if(!bNear&&bRadioLoop){M->Audio->Stop(RadioKey,.3f);bRadioLoop=false;}
}
void FS47Predecessor::FillMap(FS47MapView& View) const
{
 if(!Live()||WagonSpot.Parts.IsEmpty())return;
 const bool bKept=Found(WagonId);
 if(!bKept&&!Heard(WagonId))return;
 double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);
 FS47MapPin P;P.East=WagonSpot.X;P.South=WagonSpot.Z-SaroZ;P.Label=TEXT("wagon 23");P.bHeard=!bKept;View.Pins.Add(P);
}
bool FS47Predecessor::WorldSpot(const FString& Id,FString& OutArea,double& OutX,double& OutZ) const
{
 if(Id==TEXT("locker")){OutArea=TEXT("saro");OutX=LockerA.X+1.2;OutZ=LockerA.Z;return true;}
 if(Id==TEXT("keys")){OutArea=TEXT("saro");OutX=BoardAt.X+1.3;OutZ=BoardAt.Z;return true;}
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 const FSpot* P=Id==WagonId?&WagonSpot:Land.FindByPredicate([&](const FSpot& Q){return Q.Id==Id;});
 if(!P)return false;
 OutArea=TEXT("road");OutX=Road.X+P->X;OutZ=Road.Z+P->Z;
 return true;
}

// ---------- the developer's menu and the tests ----------
void FS47Predecessor::TestOn(){bTest=true;BuildSaro();BuildLand();Bind();}
void FS47Predecessor::TestLetter(){TestOn();ReadDoc(TEXT("letter"));}
void FS47Predecessor::TestWeek()
{
 TestOn();if(!M||!M->Freeform)return;
 const int32 Week=++WeekNoted;const FString Id=FString::Printf(TEXT("week-%d"),Week);
 const FTally C=Count();
 M->Freeform->RecordEvidence(TEXT("ward-")+Id,X::WeekNote(Week,C.Filed,C.Concluded,C.Dishes,C.Clean,C.Arrears,FMath::Max(0,AssessDay-M->Freeform->Day()),DateText(AssessDay),Points(C),PointsToStay,C.Taken,C.Land,Decide(C)==EOutcome::Stays,LeverWords(C),C.Reprieved));
 ReadDoc(Id);
}
void FS47Predecessor::TestRing()
{
 TestOn();bRinging=true;RingLeft=45;
 if(M&&M->Audio){FS47Sound O;O.Gain=.45f;O.bLoop=true;M->Audio->LoopSfx(RingKey,TEXT("phoneRing"),O);}
 if(M&&M->UI)M->UI->Toast(TEXT("The control room phone is ringing."),5);
}
bool FS47Predecessor::TestBark()
{
 TestOn();if(!M||!M->FieldRadio||!M->Freeform)return false;
 M->FieldRadio->TestOwn();
 const X::FBark* B=NextBark();if(!B)return false;
 const bool bSaid=M->FieldRadio->Message(FString(TEXT("DALE: "))+B->Line);
 if(bSaid)Seen.Add(FString(TEXT("bark_"))+B->Id);
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: test: Dale on the walkie: %s"),B->Id);
 return bSaid;
}
bool FS47Predecessor::TestHail()
{
 TestOn();if(!M||!M->FieldRadio||!M->Freeform)return false;
 M->FieldRadio->TestOwn();
 const X::FBark* B=NextBark();
 const FString Id=B?FString(B->Id):FString();const TWeakPtr<uint8> Guard=Life;
 FS47FieldRadio::FHail Words;Words.First=X::WalkieLine(TEXT("hail"));Words.Again=X::WalkieLine(TEXT("hail_again"));Words.GiveUp=X::WalkieLine(TEXT("hail_gone"));
 const bool bOut=M->FieldRadio->Hail(Words,[this,Guard,Id]{if(Guard.IsValid())SayBark(Id);},[this,Guard]{if(Guard.IsValid())MissedHail();});
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: test: Dale hails%s, %s"),bOut?TEXT(""):TEXT(" (not heard)"),Id.IsEmpty()?TEXT("nothing due"):*Id);
 return bOut;
}
void FS47Predecessor::TestOutcome(EOutcome O)
{
 TestOn();if(!M||!M->Freeform)return;
 Outcome=O;
 const FString Text=OutcomeLetter(O);
 UE_LOG(LogTemp,Display,TEXT("S47: predecessor: test outcome %s"),NameOf(O));
 FS47Doc D=MakeDoc(TEXT("assessment"));D.Page=Text;D.Transcript=Text;if(M->Story)M->Story->AddDoc(D);
 // The letter the player would have had: replaced, not added to, in a test.
 OpenAssessment();
}
void FS47Predecessor::TestLeave(){TestOn();Leave();}
void FS47Predecessor::TestNextDay(){if(M&&M->Freeform)M->Freeform->TestAdvance(86400);}
void FS47Predecessor::TestAssessNow()
{
 TestOn();if(!M||!M->Freeform)return;
 auto& F=*M->Freeform;
 const int32 Days=FMath::Max(0,AssessDay-F.Day());
 if(Days>0)F.TestAdvance(Days*86400.);
 Outcome=EOutcome::None;NewsIn=0;Outcome=Decide(Count());Decided();
}
void FS47Predecessor::TestFound(const FString& Id){TestOn();if(M)ReadDoc(Id);}
bool FS47Predecessor::TestRadioOff(){if(!M||!M->Freeform)return false;RadioOffDay=M->Freeform->Day();return !RadioOn();}

// ---------- what may be said ----------
TArray<FString> FS47Predecessor::AllText()
{
 TArray<FString> Out;
 for(const X::FDocDef& D:X::Docs()){Out.Add(D.Title);Out.Add(D.Page);Out.Add(D.Summary);}
 for(const X::FPerson& P:X::People())
 {
  for(const X::FSay& L:P.First)Out.Add(L.Line);
  for(const X::FTopic& Q:P.Topics){Out.Add(Q.Label);if(Q.Note)Out.Add(Q.Note);for(const X::FSay& L:Q.Says)Out.Add(L.Line);}
 }
 for(const X::FBark& B:X::Barks())Out.Add(B.Line);
 for(const X::FWalkieLine& L:X::WalkieLines())Out.Add(L.Line);
 const FString D=TEXT("MONDAY, JUNE 2");
 Out.Add(X::Letter(false,D,D));Out.Add(X::Letter(true,D,D));Out.Add(X::WeekNote(1,3,0,20,3,5,20,D,1,PointsToStay,0));Out.Add(X::WeekNote(2,9,2,21,9,0,13,D,7,PointsToStay,1));Out.Add(X::WeekNote(3,9,2,21,9,0,6,D,4,PointsToStay,2));
 Out.Add(X::Stays(D,30,2,25,0,true));Out.Add(X::KnowsEnough(D,3,0,20,5,D));Out.Add(X::WhatYouKnow());Out.Add(X::ClosedOriginals(D,3,0,20,5,6,2,D));Out.Add(X::ClosedNothing(D,0,0,20,5,D));Out.Add(X::NextOperator(D,D,6,2,3));
 Out.Add(X::TakenWarning(D,2));Out.Add(X::ClosedNow(D));
 // M7: the levers in her words, the notes after KEPT OPEN, what the Stays letter says of what was found, and the inspector.
 const FString AllLevers=X::Levers(false,false,0,false,1,60,80)+X::Levers(true,true,2,true,0,0,80)+X::Levers(false,false,-1,false,0,0,80);
 Out.Add(AllLevers);Out.Add(X::WeekNote(4,20,1,25,18,45,2,D,6,PointsToStay,3,2,false,AllLevers));Out.Add(X::WeekNote(4,20,1,25,18,0,2,D,7,PointsToStay,0,0,true,AllLevers));
 Out.Add(X::KeptNote(5,30,26,0,D,1,AllLevers));Out.Add(X::Stays(D,30,2,25,0,true,X::Followed(true,true,true,true)));
 for(int32 W=0;W<6;++W)Out.Add(X::Inspector(D,D,W));
 Out.Add(TEXT("Return to the title. Continue goes back to the log filed when you left SARO"));
 return Out;
}
bool FS47Predecessor::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s predecessor: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 // The line: the outside only. Nothing from Story's far side, in any paper or any mouth.
 static const TCHAR* const Far[]={TEXT("forty-seven"),TEXT("forty seven"),TEXT("47"),TEXT("tomas"),TEXT("tomás"),TEXT("vega"),TEXT("silhouette"),TEXT("cable"),TEXT("the cut"),TEXT("sight line"),TEXT("closing point"),TEXT("kinski"),TEXT("alien"),TEXT("ufo"),TEXT("saucer"),TEXT("abduct"),TEXT("taken"),TEXT("photograph of"),TEXT("8.15"),TEXT("old road"),TEXT("the event"),TEXT("chevy"),TEXT("chevrolet"),TEXT("winnebago"),TEXT("cadillac"),TEXT("pepsi"),TEXT("tyre"),TEXT("windscreen"),TEXT("grey"),TEXT("programme")};
 bool bClean=true;FString Offender;
 for(const FString& Text:AllText())
 {
  const FString Low=Text.ToLower();
  for(const TCHAR* W:Far)if(Low.Contains(W)){bClean=false;Offender=FString(W)+TEXT(" in: ")+Text.Left(60);}
 }
 Check(bClean,TEXT("no paper and no person says anything from Story's far side"));
 {
  // Night logs her office has initialed count a little, once (M7): one point from LogsForPoint, and no more for more of them.
  FTally Usual;Usual.Filed=12;Usual.Clean=10;
  FTally Enough=Usual;Enough.Logs=LogsForPoint;FTally Many=Usual;Many.Logs=30;FTally Few=Usual;Few.Logs=LogsForPoint-1;
  Check(Points(Few)==Points(Usual)&&Points(Enough)==Points(Usual)+1&&Points(Many)==Points(Usual)+1,TEXT("initialed night logs count toward the assessment: one point, once"));
  FTally Cards=Usual;Cards.Filed=60;Cards.Clean=60;FTally Guess=Usual;Guess.Guessed=2;
  Check(Points(Cards)==Points(Usual)&&Points(Guess)==Points(Usual),TEXT("a pile of cards past the floor earns nothing, nor does a filing with nothing behind it"));
  FTally OneTape=Usual;OneTape.Tapes=1;FTally TwoTapes=Usual;TwoTapes.Tapes=2;
  Check(Points(OneTape)==Points(Usual)&&Points(TwoTapes)==Points(Usual)+1,TEXT("two tapes with something on them nobody heard count one point"));
 }
 if(!bClean)Lines.Add(FString(TEXT("FAIL predecessor: "))+Offender);
 // Reyes is only ever Reyes.
 bool bNeutral=true;
 for(const FString& Text:AllText())
 {
  const FString Low=Text.ToLower();
  for(const TCHAR* W:{TEXT("reyes, he"),TEXT("reyes, she"),TEXT("reyes's wife"),TEXT("reyes's husband"),TEXT("mr. reyes"),TEXT("ms. reyes"),TEXT("mrs. reyes")})bNeutral&=!Low.Contains(W);
 }
 Check(bNeutral,TEXT("Reyes is not given a sex or a first name"));
 // The trail: each tag sits at a place that exists, and each points on, to the last.
 TSet<FString> Tagged;bool bOnward=true;int32 Ends=0;
 for(const X::FTagDef& D:X::Tags())
 {
  Tagged.Add(D.Place);const X::FDocDef* Doc=X::Doc(D.Doc);
  bOnward&=Doc&&FS47Places::Def(D.Place)!=nullptr;
  if(Doc&&Doc->Next){bOnward&=FS47Places::Def(Doc->Next)!=nullptr;}else ++Ends;
 }
 Check(bOnward&&Ends==1,TEXT("every tag is at a real place and points on to the next, and one is the last"));
 int32 Places=0;for(const auto& D:FS47Places::Defs())if(FString(D.Id)!=TEXT("motel"))++Places;
 Check(Tagged.Num()==X::Tags().Num()&&Tagged.Num()>=FMath::Min(Places,8)&&X::Tags().Num()<=10,TEXT("eight to ten notes out on the land, one at each place"));
 bool bAllDocs=true;for(const X::FDocDef& D:X::Docs())bAllDocs&=FCString::Strlen(D.Page)>40&&FCString::Strlen(D.Summary)>20&&(!D.Next||FS47Places::Def(D.Next));
 Check(bAllDocs,TEXT("each paper has its text, its note for the records, and a real place if it points to one"));
 bool bCases=true;for(const X::FCaseNote& N:X::CaseNotes()){bool bFound=false;for(int32 L=0;L<FS47FreeformCases::CaseCount;++L)bFound|=FString(FS47FreeformCases::Def(L).Title)==N.CaseTitle;bCases&=bFound&&X::Doc(N.Doc);}
 Check(bCases,TEXT("the notes among a case's sheets are tied to cases that exist"));
 // Dale.
 FS47Predecessor Quiet(nullptr);Quiet.Begin(nullptr,false,1);
 TSet<FString> BarkIds;bool bBarks=true;for(const X::FBark& B:X::Barks()){BarkIds.Add(B.Id);bBarks&=FCString::Strlen(B.Line)>10&&FCString::Strlen(B.Line)<190;}
 Check(bBarks&&BarkIds.Num()==X::Barks().Num()&&!Quiet.Meets(TEXT("letter"))&&Quiet.Meets(nullptr),TEXT("Dale's lines fit a toast, each once, and each waits for what it answers"));
 const X::FPerson* DalePerson=X::Person(TEXT("dale"));
 bool bBack=false;for(const X::FTopic& Q:DalePerson?DalePerson->Topics:TArray<X::FTopic>())bBack|=Q.Needs&&FString(Q.Needs)==TEXT("known6");
 Check(DalePerson&&DalePerson->Topics.Num()>=6&&bBack,TEXT("Dale has several talks, and what he lets slip waits until you have gone far enough"));
 {
  // WALKIE-USE-20261108: called on the walkie, he answers by the hour and by what the player is doing, or not at all.
  const auto Reply=[](double Hour,TFunction<void(FDaleCall&)> Set){FDaleCall Call;Call.Hour=Hour;if(Set)Set(Call);const TCHAR* R=DaleReply(Call);return FString(R?R:TEXT("-"));};
  const TFunction<void(FDaleCall&)> Plain=[](FDaleCall&){};
  Check(Reply(4.,Plain)==TEXT("-")&&Reply(6.9,Plain)==TEXT("-")&&Reply(17.5,Plain)==TEXT("-")&&Reply(20.,[](FDaleCall& Call){Call.Answered=4;})==TEXT("-"),TEXT("asleep, on his way home, or had enough: Dale does not answer the walkie"));
  Check(Reply(20.,[](FDaleCall& Call){Call.bAtStation=true;})==TEXT("station")&&Reply(20.,[](FDaleCall& Call){Call.bDriving=true;})==TEXT("driving")&&Reply(21.,[](FDaleCall& Call){Call.bFarOut=true;})==TEXT("far")&&Reply(22.,Plain)==TEXT("evening"),TEXT("in the evening his answer follows where the player is and what they are doing"));
  Check(Reply(2.5,[](FDaleCall& Call){Call.bInside=true;})==TEXT("late_in")&&Reply(2.5,Plain)==TEXT("late_out")&&Reply(.5,Plain)==TEXT("midnight")&&Reply(10.,Plain)==TEXT("day"),TEXT("after two, around midnight and in the day he says something else"));
  Check(Reply(20.,[](FDaleCall& Call){Call.SinceLast=300;})==TEXT("again")&&Reply(20.,[](FDaleCall& Call){Call.Answered=3;})==TEXT("enough")&&Reply(20.,[](FDaleCall& Call){Call.bMissed=true;Call.bDriving=true;})==TEXT("missed")&&Reply(23.,[](FDaleCall& Call){Call.bLostTime=true;})==TEXT("hours"),TEXT("called again too soon, too often, after his own call went unanswered, or after lost hours"));
  bool bLines=true;TSet<FString> LineIds;
  for(const X::FWalkieLine& L:X::WalkieLines()){LineIds.Add(L.Id);bLines&=FCString::Strlen(L.Line)>10&&FCString::Strlen(L.Line)<190;}
  for(const TCHAR* Want:{TEXT("hail"),TEXT("hail_again"),TEXT("hail_gone"),TEXT("finally"),TEXT("again"),TEXT("enough"),TEXT("missed"),TEXT("driving"),TEXT("hours"),TEXT("late_in"),TEXT("late_out"),TEXT("midnight"),TEXT("day"),TEXT("station"),TEXT("array"),TEXT("far"),TEXT("evening"),TEXT("relay_pre"),TEXT("relay_post")})bLines&=!X::WalkieLine(Want).IsEmpty();
  Check(bLines&&LineIds.Num()==X::WalkieLines().Num(),TEXT("every walkie reply has its words, each id once, and they fit a toast"));
  FS47Predecessor NoGame(nullptr);
  Check(NoGame.AnswerWalkie().IsEmpty()&&NoGame.RelayLine(TEXT("A rancher reports lights.")).IsEmpty()&&NoGame.ServiceBand().IsEmpty(),TEXT("without a Freeform game there is no Dale on the walkie and no carrier on the band"));
 }
 // The assessment.
 FTally Grind;Grind.Filed=60;Grind.Clean=60;Grind.Photos=5;Grind.Dishes=14;
 Check(Decide(Grind)!=EOutcome::Stays,TEXT("sixty cards and nothing else carried through is not enough to stay"));
 FTally Good;Good.Filed=FiledFloor;Good.Clean=FiledFloor;Good.Concluded=2;Good.Dishes=14;
 Check(Decide(Good)==EOutcome::Stays,TEXT("two cases carried through and readable records: SARO stays"));
 FTally Mended;Mended.Filed=FiledFloor;Mended.Clean=FiledFloor;Mended.Concluded=1;Mended.Dishes=25;
 Check(Decide(Mended)==EOutcome::Stays,TEXT("one case and a mended array: SARO stays too"));
 FTally Sloppy=Good;Sloppy.Clean=2;Check(Decide(Sloppy)!=EOutcome::Stays,TEXT("not on records nobody can read"));
 FTally Idle=Good;Idle.Filed=FiledFloor-1;Idle.Clean=Idle.Filed;Check(Decide(Idle)!=EOutcome::Stays,TEXT("not without the work itself"));
 {
  // Ward's Monday remark promises what the assessment will do.
  bool bTrue=true;
  for(const FTally& Ty:{Grind,Good,Mended,Sloppy})
  {
   const bool bStays=Decide(Ty)==EOutcome::Stays;
   const FString W=X::WeekNote(2,Ty.Filed,Ty.Concluded,Ty.Dishes,Ty.Clean,0,10,TEXT("MONDAY"),Points(Ty),PointsToStay,0,0,bStays);
   const bool bPraise=W.Contains(TEXT("I could defend it"));
   bTrue&=bPraise==bStays;
  }
  Check(bTrue,TEXT("Ward praises the week only when it would pass the assessment"));
  Check(X::WeekNote(2,5,0,14,5,0,10,TEXT("MONDAY"),0,PointsToStay,2).Contains(TEXT("somebody else on nights")),TEXT("the Monday sheet names the lost hours, and what a third would do"));
 }
 FTally Owing=Good;Owing.Arrears=FS47FreeformSession::ArrearsCap;Check(Decide(Owing)!=EOutcome::Stays,TEXT("not with the sheet at its limit"));
 FTally Knows;Knows.Known=KnownEnough;Knows.bWagon=Knows.bGap=true;Check(Decide(Knows)==EOutcome::KnowsEnough,TEXT("closed, but knowing what can be known"));
 FTally NoWagon=Knows;NoWagon.bWagon=false;NoWagon.Originals=1;Check(Decide(NoWagon)==EOutcome::ClosedOriginals,TEXT("without the wagon it is not enough; the originals remain"));
 Check(Decide(FTally())==EOutcome::ClosedNothing,TEXT("nothing done, nothing kept"));
 FTally Gone=Good;Gone.Taken=TakenTooOften;Check(Decide(Gone)==EOutcome::Gone,TEXT("taken too many times: somebody else starts the shift"));
 {
  // M7: routine alone does not keep the station. The whole month done by the card: three jobs a day, all of them clean, the array
  // at 27, every night sheet telexed, every weekly report whole, a few photographs, and no case carried through.
  FTally Routine;Routine.Filed=81;Routine.Clean=81;Routine.Dishes=27;Routine.Logs=27;Routine.Trust=4;Routine.Photos=4;Routine.Land=3;
  Check(Points(Routine)>=PointsToStay&&Decide(Routine)!=EOutcome::Stays,TEXT("the month done by the card, with no case carried through, does not keep SARO open"));
  FTally Guessed=Routine;Guessed.Guessed=2;Check(Decide(Guessed)!=EOutcome::Stays,TEXT("nor with two filings that have nothing behind them"));
  FTally Carried=Routine;Carried.Concluded=1;Check(Decide(Carried)==EOutcome::Stays,TEXT("with one case carried through, the same month does"));
  const FString Note=X::WeekNote(3,81,0,27,81,0,9,TEXT("MONDAY"),Points(Routine),PointsToStay,0,0,false,FString());
  Check(!Note.Contains(TEXT("I could defend it"))&&Note.Contains(TEXT("case carried through")),TEXT("and Ward's Monday note says what is missing, not that it would pass"));
  // The third stretch of lost time: answered the morning after it is noticed; after "Not yet", only a further one does it.
  Check(!GoneDue(3,0,0,9)&&!GoneDue(3,0,9,9)&&GoneDue(3,0,9,10)&&!GoneDue(2,0,9,10)&&!GoneDue(3,3,9,10)&&GoneDue(4,3,9,10),TEXT("a third taking sends somebody else the morning after, and after a reprieve only a fourth"));
  FTally Reprieved=Good;Reprieved.Taken=TakenTooOften;Reprieved.Reprieved=TakenTooOften;
  Check(Decide(Reprieved)==EOutcome::Stays,TEXT("after \"Not yet\" with no new taking, the review goes by the work"));
  FTally Again=Reprieved;Again.Taken=TakenTooOften+1;Check(Decide(Again)==EOutcome::Gone,TEXT("and a further taking ends it"));
  Check(X::WeekNote(5,20,1,25,18,0,9,TEXT("MONDAY"),6,PointsToStay,3,0,false,FString(),3).Contains(TEXT("There will not be a second time"))&&!X::WeekNote(5,20,1,25,18,0,9,TEXT("MONDAY"),6,PointsToStay,3,0,false,FString(),3).Contains(TEXT("If it happens again")),TEXT("after the reprieve her Monday note does not promise what she has already done"));
  Check(X::WeekNote(5,20,1,25,18,0,9,TEXT("MONDAY"),6,PointsToStay,3).Contains(TEXT("I meant it"))&&!X::WeekNote(5,20,1,25,18,0,9,TEXT("MONDAY"),6,PointsToStay,3).Contains(TEXT("I did it")),TEXT("with the third noticed and no \"Not yet\" yet, she does not say she has already acted"));
  {FTally Owes=Routine;Owes.Concluded=1;const FString OwesNote=X::WeekNote(3,81,1,27,81,0,9,TEXT("MONDAY"),Points(Owes),PointsToStay,0,0,false,FString());
   Check(!OwesNote.Contains(TEXT("What is missing is a case")),TEXT("with a case carried through, her note does not say the case is missing"));}
  // Every lever she counts has her word on the Monday sheet; the carried line's limit is named from CarriedWarn.
  const FString Lev=X::Levers(false,false,0,false,1,CarriedWarn,FS47FreeformSession::ArrearsCap);
  Check(Lev.Contains(TEXT("night sheets"))&&Lev.Contains(TEXT("tape"))&&Lev.Contains(TEXT("weekly report"))&&Lev.Contains(TEXT("photographs"))&&Lev.Contains(TEXT("nothing behind it"))&&Lev.Contains(FString::Printf(TEXT("Past $%d"),FS47FreeformSession::ArrearsCap))
   &&!X::Levers(true,true,1,true,0,CarriedWarn-1,FS47FreeformSession::ArrearsCap).Contains(TEXT("Past $")),TEXT("her Monday sheet has a word for each lever, and names the carried line's limit from $40"));
  Check(X::Stays(TEXT("MONDAY"),20,1,25,0,false,X::Followed(true,true,true)).Contains(TEXT("wagon 23"))&&X::Followed(false,false,false).IsEmpty(),TEXT("the Stays letter says what came of what was found, and nothing when nothing was"));
 }
 {
  // M7: OPS does not give the outcome away before the letter is read.
  auto St=S47J::New();S47J::Set(St,TEXT("opens"),1);S47J::Set(St,TEXT("assess"),30);S47J::Set(St,TEXT("outcome"),TEXT("stays"));S47J::Set(St,TEXT("outcomeRead"),false);S47J::Set(St,TEXT("reprieve"),3);S47J::Set(St,TEXT("goneNoticed"),12);S47J::Set(St,TEXT("keptDay"),31);
  FS47Predecessor Unread(nullptr);Unread.Begin(S47J::Read(S47J::Write(St)),false,30);
  S47J::Set(St,TEXT("outcomeRead"),true);FS47Predecessor Read(nullptr);Read.Begin(S47J::Read(S47J::Write(St)),false,30);
  Check(Unread.StatusLine()==TEXT("UNDER REVIEW / ASSESSMENT MADE")&&!Unread.OutcomeRead()&&Read.StatusLine()==TEXT("KEPT OPEN")&&Read.KeptOpen(),TEXT("the status says ASSESSMENT MADE until the letter is read, and KEPT OPEN after"));
  const auto Back2=Read.Save();
  Check(S47J::Int(Back2,TEXT("reprieve"))==3&&S47J::Int(Back2,TEXT("goneNoticed"))==12&&S47J::Int(Back2,TEXT("keptDay"))==31&&S47J::Bool(Back2,TEXT("outcomeRead")),TEXT("the reprieve, a noticed taking and the day it was kept survive a Continue"));
  auto Old=S47J::New();S47J::Set(Old,TEXT("opens"),1);S47J::Set(Old,TEXT("outcome"),TEXT("closed"));S47J::Set(Old,TEXT("tray"),TArray<FString>{TEXT("assessment")});
  FS47Predecessor Older2(nullptr);Older2.Begin(S47J::Read(S47J::Write(Old)),false,31);
  Check(!Older2.OutcomeRead()&&Older2.StatusLine().Contains(TEXT("ASSESSMENT MADE")),TEXT("a save from before this, with the letter still in the tray, is not told the outcome either"));
  Check(Unread.TitleButton()==TEXT("Return to the title. This save stays"),TEXT("without a game, the title button does not claim the player is away"));
 }
 {
  // Critic round 5. "Not yet" once: a save with a reprieve in it offers no second one.
  auto Rp=S47J::New();S47J::Set(Rp,TEXT("opens"),1);S47J::Set(Rp,TEXT("assess"),44);S47J::Set(Rp,TEXT("reprieve"),3);
  FS47Predecessor Once(nullptr);Once.Begin(S47J::Read(S47J::Write(Rp)),false,31);
  FS47Predecessor NewGame(nullptr);NewGame.Begin(nullptr,false,1);
  Check(NewGame.OffersNotYet()&&!Once.OffersNotYet()&&Once.AssessmentDay()==44&&!Once.bLegacyReprieve,TEXT("\"Not yet\" is given once: after a reprieve the letter to the next operator has no second one"));
  // Saves from before M7: a "Not yet" without its reprieve keeps its nights; a KEPT OPEN read has its finale; one still in the box
  // at the road is not read.
  auto Old7=S47J::New();S47J::Set(Old7,TEXT("opens"),1);S47J::Set(Old7,TEXT("assess"),44);
  FS47Predecessor Reprieved7(nullptr);Reprieved7.Begin(S47J::Read(S47J::Write(Old7)),false,31);
  const bool bWaited=Reprieved7.bLegacyReprieve;Reprieved7.SettleLegacy(3,false,31);
  Check(bWaited&&Reprieved7.TakenAtReprieve==3&&!GoneDue(3,Reprieved7.TakenAtReprieve,31,32)&&GoneDue(4,Reprieved7.TakenAtReprieve,31,32)&&!Reprieved7.OffersNotYet(),TEXT("a \"Not yet\" from before M7 keeps its nights: only a further stretch of lost time ends it"));
  auto Kept7=S47J::New();S47J::Set(Kept7,TEXT("opens"),1);S47J::Set(Kept7,TEXT("assess"),30);S47J::Set(Kept7,TEXT("outcome"),TEXT("stays"));
  FS47Predecessor ReadKept(nullptr);ReadKept.Begin(S47J::Read(S47J::Write(Kept7)),false,33);ReadKept.SettleLegacy(0,false,33);
  FS47Predecessor Posted(nullptr);Posted.Begin(S47J::Read(S47J::Write(Kept7)),false,31);Posted.SettleLegacy(0,true,31);
  Check(ReadKept.KeptOpen()&&ReadKept.Found(TEXT("kept"))&&ReadKept.KeptDay==33&&!Posted.OutcomeRead()&&Posted.StatusLine()==TEXT("UNDER REVIEW / ASSESSMENT MADE")&&!Posted.Found(TEXT("kept")),
   TEXT("a KEPT OPEN read before M7 gets its finale, and one still in the box at the road is not read yet"));
  // The run up after KEPT OPEN is saved as where each dish goes back to, never as the zenith.
  FS47Predecessor Run(nullptr);Run.Begin(nullptr,false,30);
  FSaluteDish Dish;Dish.I=4;Dish.Az=123;Dish.El=37;Run.Salute.Add(Dish);Run.SaluteStage=1;
  double Az=0,El=0;const bool bRun=Run.SaluteTarget(4,Az,El);const double GotAz=Az,GotEl=El;const bool bOther=Run.SaluteTarget(5,Az,El);
  Run.SaluteStage=0;
  Check(bRun&&GotAz==123&&GotEl==37&&!bOther&&!Run.SaluteTarget(4,Az,El),TEXT("a save during the run up after KEPT OPEN keeps where each dish was pointed"));
  // Ward's first letter says what keeps the station open: a case carried through as well as the work.
  Check(X::Letter(false,TEXT("MAY 1"),TEXT("MAY 28")).Contains(TEXT("case board"))&&X::Letter(true,TEXT("MAY 1"),TEXT("MAY 28")).Contains(TEXT("case board")),TEXT("her first letter names the case board, not the routine alone"));
  // M7, weeks three and four (S47PredecessorLate.cpp).
  auto St=S47J::New();S47J::Set(St,TEXT("opens"),1);S47J::Set(St,TEXT("assess"),30);S47J::Set(St,TEXT("bulletin"),13);S47J::Set(St,TEXT("thermosDay"),18);S47J::Set(St,TEXT("postDay"),16);S47J::Set(St,TEXT("awayDay"),17);
  S47J::Set(St,TEXT("seen"),TArray<FString>{TEXT("trip"),TEXT("talk_attendant_thermos")});
  FS47Predecessor Late(nullptr);Late.Begin(S47J::Read(S47J::Write(St)),false,19);
  const double Minute=Late.MinuteClock();
  Check(FS47FreeformSession::NightOf(Minute)==15&&FMath::IsNearlyEqual(FMath::Fmod(Minute,86400.)/3600.,50/60.,1e-6),TEXT("the Sky Watch's printed minute is ten to one on the third night from the postmark"));
  const auto Back3=Late.Save();
  Check(S47J::Int(Back3,TEXT("bulletin"))==13&&S47J::Int(Back3,TEXT("thermosDay"))==18&&S47J::Int(Back3,TEXT("postDay"))==16&&S47J::Int(Back3,TEXT("awayDay"))==17&&Late.Known()==2,TEXT("the bulletin, the thermos, an emptied post and the trip meter survive a Continue, and the trail after wagon 23 counts in what is known"));
  FS47Predecessor None3(nullptr);None3.Begin(nullptr,false,1);
  Check(None3.MinuteClock()==0&&!None3.Save()->HasField(TEXT("bulletin")),TEXT("no printed minute before the bulletin"));
  // The trail after wagon 23: the trip meter, the attendant's thermos once it has been read, and the letter that answers it.
  const X::FPerson* Att=X::Person(TEXT("attendant"));
  const X::FTopic* Thermos=Att?Att->Topics.FindByPredicate([](const X::FTopic& Q){return FString(Q.Id)==TEXT("thermos");}):nullptr;
  Check(X::Doc(TEXT("trip"))&&X::Doc(TEXT("ortiz_reply"))&&X::Doc(TEXT("skywatch"))&&Thermos&&Thermos->Needs&&FString(Thermos->Needs)==TEXT("trip")&&!None3.Meets(Thermos->Needs),TEXT("wagon 23's trip meter leads to the attendant's thermos, and its letter and the Sky Watch's bulletin have their papers"));
  Check(X::Followed(false,false,false,true).Contains(TEXT("thermos"))&&!X::Followed(true,true,true,false).Contains(TEXT("thermos")),TEXT("the Stays letter says the thermos went home only when it did"));
  // Dale, weeks two to four: a line for each thing that has really gone on, after the first week's.
  int32 Late2=0;for(const X::FBark& B:X::Barks())Late2+=B.FromDay>=5?1:0;
  bool bLateNeeds=true;for(const TCHAR* Id:{TEXT("skywatch"),TEXT("van"),TEXT("copter"),TEXT("away"),TEXT("filed"),TEXT("minute"),TEXT("post"),TEXT("raked"),TEXT("halfway"),TEXT("trip"),TEXT("thermos"),TEXT("inspector"),TEXT("week4"),TEXT("wiped")})
  {const X::FBark* B=X::Barks().FindByPredicate([Id](const X::FBark& Q){return FCString::Strcmp(Q.Id,Id)==0;});bLateNeeds&=B&&B->Needs&&!None3.Meets(B->Needs);}
  Check(Late2>=14&&bLateNeeds,TEXT("Dale has fourteen lines for weeks two to four, and each waits for what it answers"));
 }
 for(int32 I=0;I<6;++I)Check(OutcomeOf(NameOf(EOutcome(I)))==EOutcome(I),TEXT("the outcome's name comes back as itself"));
 // Saves.
 FS47Predecessor Fresh(nullptr);Fresh.Begin(nullptr,false,1);
 Check(Fresh.AssessmentDay()==FirstAssess&&Fresh.Result()==EOutcome::None&&!Fresh.HasLeft(),TEXT("a new game is assessed on day 30"));
 FS47Predecessor Older(nullptr);Older.Begin(nullptr,true,1);
 Check(Older.AssessmentDay()==3+FirstAssess,TEXT("a save from day 1 of the old calendar: nothing before the 14th has gone by"));
 FS47Predecessor Later(nullptr);Later.Begin(nullptr,true,12);
 Check(Later.AssessmentDay()==12+FirstAssess,TEXT("a save under way gets its thirty days from now"));
 Check(S47J::Bool(Later.Save(),TEXT("older"))&&S47J::Int(Later.Save(),TEXT("weekNoted"))==1,TEXT("and no back pay of Monday sheets"));
 auto O=S47J::New();S47J::Set(O,TEXT("opens"),1);S47J::Set(O,TEXT("assess"),30);S47J::Set(O,TEXT("seen"),TArray<FString>{TEXT("locker"),TEXT("wagon"),TEXT("gap"),TEXT("tag_mine")});S47J::Set(O,TEXT("outcome"),TEXT("closed-originals"));S47J::Set(O,TEXT("left"),true);S47J::Set(O,TEXT("leftDay"),9);S47J::Set(O,TEXT("radioOff"),8);
 FS47Predecessor Back(nullptr);Back.Begin(S47J::Read(S47J::Write(O)),true,20);
 Check(Back.Known()==4&&Back.Found(TEXT("tag_mine"))&&Back.Result()==EOutcome::ClosedOriginals&&Back.HasLeft(),TEXT("what was found, the outcome and leaving survive a Continue"));
 const auto Again=Back.Save();Check(S47J::Bool(Again,TEXT("left"))&&S47J::Str(Again,TEXT("outcome"))==TEXT("closed-originals"),TEXT("and are written again"));
 Check(Back.ClosingDay()==30+NoticeDays&&!Back.StationClosed(),TEXT("an older closing outcome gets its week's notice, not a date that has gone by"));
 S47J::Set(O,TEXT("closeDay"),37);S47J::Set(O,TEXT("closed"),true);
 FS47Predecessor Shut(nullptr);Shut.Begin(S47J::Read(S47J::Write(O)),true,40);
 Check(Shut.StationClosed()&&Shut.ClosingDay()==37&&S47J::Bool(Shut.Save(),TEXT("closed")),TEXT("a closed station stays closed after a Continue"));

 // Dr. Ward takes calls from 08:00 until 21:00 and never at night (WARD-PHONE-20261108); the clock runs on past midnight.
 const auto Stamp=[](int32 Day,double Hour){return double(Day)*86400.+Hour*3600.;};
 Check(WardHours(Stamp(0,8))&&WardHours(Stamp(3,12))&&WardHours(Stamp(3,20.99))&&!WardHours(Stamp(3,21))&&!WardHours(Stamp(3,23.5))&&!WardHours(Stamp(4,0))&&!WardHours(Stamp(4,7.99))&&WardHours(Stamp(4,8)),TEXT("Dr. Ward is on the phone from 08:00 to 21:00 and not at night, on any day"));
 Check(Shut.WardAvailable(),TEXT("without a game running the office is not claimed to be shut"));
 return OK;
}
