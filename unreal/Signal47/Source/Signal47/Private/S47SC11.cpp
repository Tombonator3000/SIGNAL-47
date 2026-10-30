#include "S47SC11.h"
#include "S47SC04.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Docs.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Seen.h"
#include "S47Three.h"
#include "S47Flow.h"
#include "S47HorrorPack.h"
#include "Engine/World.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SC11, Log, All);
namespace
{
 enum { Office, ControlDoor, Corridor, RecordsDoor, Exit, Stoop, Ramp, West, NorthWest,
  North, NorthSteps, EastSteps, EastWalk, Landing, EastDoor, Control, ControlEast, Truck, Records, LandingGap, TruckGap, RecordsEast, RecordsNE, RecordsWest, RecordsSW, OfficeCabSouth, OfficeCabWest };
 const FVector BeamSources[] = {FVector(-10, 7, -17), FVector(6, 8, -18), FVector(18, 6, 2)};
 const FVector BeamTargets[] = {FVector(-3.6, 1.7, -2), FVector(.2, 1.7, -1), FVector(3, 1.6, 1.6)};
 const FVector2D RestartAt(1.2, .3);
 struct FGap { const TCHAR* Door; double X0, X1, Z0, Z1, Plane; bool bX; };
 const FGap Gaps[] = {{TEXT("exitDoor"),-6.9,-5,5.25,6.15,-6.27,true}, {TEXT("recordsDoor"),-4.38,-3.42,5.6,7.7,6.85,false},
  {TEXT("doorSouth"),-2.88,-1.92,3.6,5.8,4.72,false}, {TEXT("doorEast"),5,7.1,1.12,2.08,6.06,true}};
 bool Through(const FVector2D& A,const FVector2D& Z,const FGap& G)
 {
  double Lo=0,Hi=1;const FVector2D D=Z-A;
  const auto Clip=[&Lo,&Hi](double P,double Q){if(FMath::Abs(P)<1e-8)return Q>=0;const double R=Q/P;if(P<0)Lo=FMath::Max(Lo,R);else Hi=FMath::Min(Hi,R);return Lo<=Hi;};
  return Clip(-D.X,A.X-G.X0)&&Clip(D.X,G.X1-A.X)&&Clip(-D.Y,A.Y-G.Z0)&&Clip(D.Y,G.Z1-A.Y);
 }
 bool CrossesDoor(const FVector2D& A,const FVector2D& Z,const FGap& G)
 {
  const double From=(G.bX?A.X:A.Y)-G.Plane,To=(G.bX?Z.X:Z.Y)-G.Plane;
  if((From>0&&To>0)||(From<0&&To<0)||FMath::Abs(To-From)<1e-8)return false;
  const double K=-From/(To-From);const double Across=G.bX?FMath::Lerp(A.Y,Z.Y,K):FMath::Lerp(A.X,Z.X,K);
  return K>=0&&K<=1&&Across>=(G.bX?G.Z0:G.X0)&&Across<=(G.bX?G.Z1:G.X1);
 }
 double YawTo(const FVector2D& D) { return FMath::Atan2(D.Y, D.X); }
 FS47Doc Paper(const TCHAR* Id, const TCHAR* Title, const TCHAR* Stamp, const FString& Page)
 {
  FS47Doc D; D.Id = Id; D.Title = Title; D.Stamp = Stamp; D.Kind = TEXT("typed"); D.Page = Page; D.Transcript = Page; return D;
 }
 FS47Doc RemotePaper(const TCHAR* Authored,const TCHAR* Id,const TCHAR* SourceId,const TCHAR* Stamp)
 {
  // Reuse the full authored independent records, including raw optical readings,
  // uncertainty, power/feed controls and checked clocks. These are story data,
  // never measurements claimed from this machine or from a remote live service.
  static const FS47HorrorPack Pack=[](){FS47HorrorPack P;FString Error;if(!P.Initialize(Error))UE_LOG(LogS47SC11,Error,TEXT("S47: SC11 source pack unavailable: %s"),*Error);return P;}();
  const auto* Raw=Pack.FindDocument(Authored);
  if(!Raw){UE_LOG(LogS47SC11,Error,TEXT("S47: SC11 authored source missing: %s"),Authored);return {};}
  FString Body=FString::Join(Raw->Body,TEXT("\n\n"));
  // Shift complete HH:MM:SS tokens once. Substring replacement can otherwise
  // mistake the tail of 04:04:00 for 04:00 and corrupt the interval endpoint.
  for(int32 I=0;I+8<=Body.Len();++I)
  {
   const bool Boundary=(I==0||!FChar::IsDigit(Body[I-1]))&&(I+8==Body.Len()||!FChar::IsDigit(Body[I+8]));
   const bool Time=Body[I]=='0'&&(Body[I+1]=='3'||Body[I+1]=='4')&&Body[I+2]==':'&&Body[I+5]==':'
    &&FChar::IsDigit(Body[I+3])&&FChar::IsDigit(Body[I+4])&&FChar::IsDigit(Body[I+6])&&FChar::IsDigit(Body[I+7]);
   if(Boundary&&Time){++Body[I+1];I+=7;}
  }
  FS47Doc D;D.Id=Id;D.Title=Raw->Title;D.Kind=Raw->Kind;D.Stamp=Stamp;
  D.Page=FString(TEXT("SOURCE ID / "))+SourceId+TEXT("\nCOMPLETE COPY RECEIVED AT SARO 05:04 MST / 14 APR 1986\n\n")+Body;
  D.Transcript=D.Page;return D;
 }
}

const TArray<FS47SC11::FNode>& FS47SC11::Nodes()
{
 // Reuse the measured SC04 corridor/rooms. The front crossing follows gNorth,
 // gSteps and the east walk in walks.saro, rather than cutting the wall corner.
 static const TArray<FNode> N = {
  {TEXT("office"), {.4,5.75}}, {TEXT("control-door"), {-2.4,3.6}}, {TEXT("corridor"), {-2.4,6.05}},
  {TEXT("records-door"), {-3.9,6.05}}, {TEXT("exit"), {-5.3,5.7}}, {TEXT("stoop"), {-7,5.7}},
  {TEXT("ramp"), {-11.1,5.7}}, {TEXT("west"), {-11.1,0}}, {TEXT("north-west"), {-9,-7.5}},
  {TEXT("north"), {0,-12}}, {TEXT("north-steps"), {6,-12}}, {TEXT("east-steps"), {8.7,-12}},
  {TEXT("east-walk"), {8.7,-7.5}}, {TEXT("landing"), {8.7,1.6}}, {TEXT("east-door"), {6.9,1.6}},
  {TEXT("control"), {1.2,.3}}, {TEXT("control-east"), {4.7,1.6}}, {TEXT("truck"), {7.2,4.0}}, {TEXT("records"), {-3.9,7.5}},
  {TEXT("landing-gap"), {9.95,1.6}}, {TEXT("truck-gap"), {9.95,4.0}}, {TEXT("records-east"),{-2.3,7.6}},
  {TEXT("records-ne"),{-2.2,9.6}}, {TEXT("records-west"),{-5.,7.7}}, {TEXT("records-sw"),{-5.,9.2}},
  {TEXT("office-cabinet-south"),{-1.5,5.6}}, {TEXT("office-cabinet-west"),{-1.5,6.05}}};
 return N;
}

const TArray<TPair<int32,int32>>& FS47SC11::Edges()
{
 static const TArray<TPair<int32,int32>> E = {{Office,OfficeCabSouth},{OfficeCabSouth,OfficeCabWest},{OfficeCabWest,Corridor},{ControlDoor,Corridor},{Corridor,RecordsDoor},
  {RecordsDoor,Records},{RecordsDoor,Exit},{Exit,Stoop},{Stoop,Ramp},{Ramp,West},{West,NorthWest},
  {NorthWest,North},{North,NorthSteps},{NorthSteps,EastSteps},{EastSteps,EastWalk},{EastWalk,Landing},
  {Landing,EastDoor},{Landing,LandingGap},{LandingGap,TruckGap},{TruckGap,Truck},{EastDoor,ControlEast},{ControlEast,Control},{Control,ControlDoor},
  {Records,RecordsEast},{RecordsEast,RecordsNE},{Records,RecordsWest},{RecordsWest,RecordsSW}};
 return E;
}

int32 FS47SC11::Room(const FVector2D& At)
{
 // SC04's indoor classifier predates a complete circuit around the north wall.
 // Keep its measured jambs but bound it in z before testing any annex room.
 if(At.Y<=-4.4||At.Y>=10.5)return 3;
 return FS47SC04::Room(At);
}

FS47Doc FS47SC11::ReceiverDoc(bool bComplete, const FString& EscapedAt)
{
 FString Page = TEXT("SOURCE ID / SARO07-860414-RX01\nSARO / RECEIVER BANK 3 / 14 APR 1986 / MST\n"
  "ORIGINAL STRIP AND COMMAND LOG. Passive receiver, local station clock. This is not the network retransmission.\n\n"
  "04:56:09  first boundary\n04:56:56  second boundary\n04:57:43  third boundary\n"
  "No slew command. The three boundaries retain their order. Compare the separate clocks; do not call them exactly simultaneous.\n\n");
 if (bComplete)
  Page += FString::Printf(TEXT("REYES / FIELD ADDENDUM, %s MST\nThe local task light was switched off at its own control. The light through the openings continued. The room was observed with that local source off. The originals were carried out of SARO.\n"
   "ctx_hr_h07_escape_complete / WORLD ESCAPE RECORDED\nThe addendum completes this local source; it does not alter the receiver timestamps."), *EscapedAt);
 else Page += TEXT("LOCAL ORIGINAL SECURED. The source remains incomplete until the light check, the room observation and the escape have actually happened.");
 return Paper(bComplete ? TEXT("HR_SARO_ORIGINAL") : TEXT("HR_SARO_RX_SECURED"), TEXT("SARO receiver original"), TEXT("SARO / RX 3"), Page);
}

FS47Doc FS47SC11::EventDoc()
{
 return Paper(TEXT("HR_SARO_EVENT_STARTED"), TEXT("Event sheet, started"), TEXT("SARO / REYES"),
  TEXT("SARO / EVENT SHEET / 14 APR 1986\n\nSource original: SARO07-860414-RX01\n"
   "Automatic schedule held. Receiver passive. Originals kept.\n\nLocal lamp check: NOT YET ENTERED\nRoom observation: NOT YET ENTERED\nDeparture: NOT YET ENTERED\n\n"
   "An unfinished working sheet. No escape or observation is certified by this form."));
}

FS47Doc FS47SC11::HondoDoc()
{
 return RemotePaper(TEXT("HR_HONDO_LOCAL"),TEXT("HR_HONDO_REMOTE"),TEXT("HONDO-860414-LOG01"),TEXT("HONDO / COMPLETE 05:04 / 14 APR 1986"));
}

FS47Doc FS47SC11::Site11Doc()
{
 return RemotePaper(TEXT("HR_SITE11_LOCAL"),TEXT("HR_SITE11_REMOTE"),TEXT("SITE11-860414-TR01"),TEXT("SITE 11 / COMPLETE 05:04 / 14 APR 1986"));
}

void FS47SC11::Begin(const S47J::FObj& Saved)
{
 Stop(); F = FFacts(); P = EStep::Return; T = Clock = ObserveT = 0;EscapedAt.Reset();bSavedLampOff=false;
 if (!Saved || S47J::Int(Saved,TEXT("v")) != 1) return;
 P = EStep(FMath::Clamp(S47J::Int(Saved,TEXT("step")),0,int32(EStep::Done)));
 F.Receiver=S47J::Bool(Saved,TEXT("receiver")); F.Event=S47J::Bool(Saved,TEXT("event"));
 F.Hondo=S47J::Bool(Saved,TEXT("hondo")); F.Site11=S47J::Bool(Saved,TEXT("site11"));
 F.CheckedLocalLight=S47J::Bool(Saved,TEXT("checkedLocalLight")); F.ObservedRoom=S47J::Bool(Saved,TEXT("observedRoom"));
 F.Escaped=S47J::Bool(Saved,TEXT("ctx_hr_h07_escape_complete")); F.Captures=FMath::Max(0,S47J::Int(Saved,TEXT("captures")));
 EscapedAt=S47J::Str(Saved,TEXT("escapedAt"));
 bSavedLampOff=S47J::Bool(Saved,TEXT("localLampOff"));
 // As SC04: an in-progress capture/search resumes at its local checkpoint, with
 // collected records and observations retained. It never writes an escape fact.
 if(P==EStep::Hunt)bRestoreHunt=true;
 if(P==EStep::Done&&!F.Escaped)P=EStep::Collect;
 Refile();
}

S47J::FObj FS47SC11::Save() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("step"),int32(P));
 S47J::Set(O,TEXT("receiver"),F.Receiver);S47J::Set(O,TEXT("event"),F.Event);S47J::Set(O,TEXT("hondo"),F.Hondo);S47J::Set(O,TEXT("site11"),F.Site11);
 S47J::Set(O,TEXT("checkedLocalLight"),F.CheckedLocalLight);S47J::Set(O,TEXT("observedRoom"),F.ObservedRoom);
 S47J::Set(O,TEXT("ctx_hr_h07_escape_complete"),F.Escaped);S47J::Set(O,TEXT("escapedAt"),EscapedAt);S47J::Set(O,TEXT("localLampOff"),LocalLightOff()||bSavedLampOff);S47J::Set(O,TEXT("captures"),F.Captures);return O;
}

void FS47SC11::Refile()
{
 if(!M||!M->Story)return;
 if(F.Receiver)M->Story->AddDoc(ReceiverDoc(false));
 if(F.Event)M->Story->AddDoc(EventDoc());
 if(F.Hondo)M->Story->AddDoc(HondoDoc());
 if(F.Site11)M->Story->AddDoc(Site11Doc());
 if(F.Escaped)M->Story->AddDoc(ReceiverDoc(true,EscapedAt));
}

void FS47SC11::Stop()
{
 bShown=false;bHavePlayer=false;bDriveStarted=false;bRestoreHunt=false;Path.Reset();WaitingDoor.Reset();FadeT=-1;
 Pale.Apply(false,FVector::ZeroVector,0,0,0,0,0,FS47PaleBody::EPose::Walk);
 for(auto& Beam:Beams)if(Beam.IsValid())Beam->SetVisibility(false);
 if(bLampCaptured&&LocalLamp)LocalLamp.SetIntensity(LampBefore);
 bLampCaptured=false;LocalLamp=FS47Light();
}

void FS47SC11::SetStep(EStep Next)
{
 if(P==Next)return;P=Next;if(M)M->RequestAutosave(false);
 UE_LOG(LogS47SC11,Display,TEXT("S47: SC11 phase %d"),int32(P));
}

void FS47SC11::BuildLights()
{
 if(!M||!M->Web)return;
 if(!bLampCaptured)
 {
  LocalLamp=M->Web->Light(TEXT("room.lights.lamps[0]"));
  if(LocalLamp){LampBefore=LocalLamp.Intensity();bLampCaptured=true;if(bSavedLampOff){LocalLamp.SetIntensity(0);bSavedLampOff=false;}}
 }
 if(Beams.Num()==UE_ARRAY_COUNT(BeamSources))return;
 for(int32 I=Beams.Num();I<UE_ARRAY_COUNT(BeamSources);++I)
 {
  auto L=NewObject<USpotLightComponent>(M->Web.Get(),FName(*FString::Printf(TEXT("SC11OutsideSearch%d"),I)));
  L->SetupAttachment(M->Web->GetRootComponent());L->SetMobility(EComponentMobility::Movable);
  L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(1800);L->SetLightColor(FLinearColor(.72f,.82f,1));
  L->SetAttenuationRadius(4200);L->SetInnerConeAngle(7);L->SetOuterConeAngle(12);L->SetCastShadows(true);
  L->RegisterComponent();L->SetWorldLocation(S47Three::ToUE(BeamSources[I]));Beams.Add(L);
 }
}

bool FS47SC11::OutsideLightStillOn() const
{
 return Beams.Num()==3&&!Beams.ContainsByPredicate([](const auto& L){return !L.IsValid()||!L->IsVisible()||L->Intensity<=100;});
}

bool FS47SC11::LocalLightOff() const { return bLampCaptured&&LocalLamp&&LocalLamp.Intensity()<=.0001f; }

void FS47SC11::StepLights(double Dt)
{
 Clock+=Dt;BuildLights();
 for(int32 I=0;I<Beams.Num();++I)if(Beams[I].IsValid())
 {
  const bool Show=(P==EStep::Collect||P==EStep::Hunt)&&M->World&&M->World->Area==TEXT("saro");
  Beams[I]->SetVisibility(Show);if(!Show)continue;
  FVector Target=BeamTargets[I];
  Target.X+=FMath::Sin(Clock*(.24+I*.035)+I*2.1)*(I==2?1.8:3.0);
  Target.Z+=FMath::Sin(Clock*.17+I)*1.2;
  if(I==0&&P==EStep::Hunt&&SeeT>.1)Target=FVector(LastKnown.X,1.6,LastKnown.Y);
  Beams[I]->SetWorldRotation((S47Three::ToUE(Target)-S47Three::ToUE(BeamSources[I])).Rotation());
 }
}

void FS47SC11::ObserveRoom(double Dt)
{
 if(F.ObservedRoom||!F.CheckedLocalLight||!LocalLightOff()||!OutsideLightStillOn()||!M->Player||M->ModalOpen())return;
 const auto Cam=FS47Cam::Of(M->Player);
 const FVector Window=M->Web->Vec(TEXT("room.windowLights.wins[1].c"));
 const bool Looking=Room(FVector2D(Cam.Pos.X,Cam.Pos.Z))==0&&S47Facing(Cam,Window,12,35)&&ClearSight(Cam.Pos,Window);
 ObserveT=Looking?ObserveT+Dt:0;
 if(ObserveT<.8)return;
 F.ObservedRoom=true;
 M->Story->Note(FString::Printf(TEXT("%s. The task light is off. The moving light through the front windows stays on, crossing the room independently."),*M->Story->ClockText(false)));
 M->Story->AdvanceClockTo(RemoteReady,TEXT("SC11 local source and room checked; remote sheets complete"));
 if(M->UI)M->UI->Toast(TEXT("The printer finishes. Hondo Valley. Site 11. Two separate records. Take both."),5);
 M->RequestAutosave(false);
}

FString FS47SC11::Label(const FString& Id) const
{
 if(P==EStep::Hunt&&(Id==TEXT("opsTerminal")||Id==TEXT("printer")||Id==TEXT("logbook")))return TEXT("All records secured / get out of SARO");
 if(P!=EStep::Collect||!M||!M->Story)return FString();
 if(Id==TEXT("opsTerminal"))return !F.Receiver?TEXT("Secure the receiver original"):!F.CheckedLocalLight?TEXT("Switch off the local task light"):LocalLightOff()?TEXT("Restore the local task light"):TEXT("Switch off the local task light");
 if(Id==TEXT("logbook")&&!F.Event)return TEXT("Start the event sheet and keep it with the original");
 if(Id==TEXT("printer")&&(!F.Hondo||!F.Site11))return M->Story->Clock()<RemoteReady?TEXT("Remote sheets still arriving; complete at 05:04"):!F.Hondo?TEXT("Take the completed Hondo Valley sheet"):TEXT("Take the completed Site 11 sheet");
 return FString();
}

bool FS47SC11::Use(const FString& Id)
{
 if(Label(Id).IsEmpty()||!M->UI||!M->Web)return false;
 if(P==EStep::Hunt){M->UI->Toast(TEXT("All four records are in the folder. Get them out."),3);return true;}
 AS47Story& S=*M->Story;
 if(Id==TEXT("opsTerminal"))
 {
  if(!F.Receiver){F.Receiver=true;auto D=ReceiverDoc(false);S.AddDoc(D);M->UI->Document(D);S.AdvanceClockTo(Arrive+60,TEXT("SC11 receiver original secured"));}
  else
  {
   BuildLights();if(!LocalLamp||!bLampCaptured){UE_LOG(LogS47SC11,Error,TEXT("S47: SC11 local task-light source unavailable"));return true;}
   const bool Off=!LocalLightOff();LocalLamp.SetIntensity(Off?0:LampBefore);
   if(Off&&LocalLightOff()&&OutsideLightStillOn())
   {
    F.CheckedLocalLight=true;S.AdvanceClockTo(Arrive+3*60,TEXT("SC11 independent local task-light check"));
    M->UI->Toast(TEXT("The task light goes out. The light moving through the front windows does not. Look across the room."),5);
   }
  }
 }
 else if(Id==TEXT("logbook")){F.Event=true;auto D=EventDoc();S.AddDoc(D);M->UI->Document(D);S.AdvanceClockTo(Arrive+2*60,TEXT("SC11 event sheet started"));}
 else if(Id==TEXT("printer"))
 {
  if(S.Clock()<RemoteReady){M->UI->Toast(TEXT("The sheets are still coming through. Complete copies at 05:04. Check the local source while they arrive."),4);return true;}
  const bool IsHondo=!F.Hondo;const FS47Doc D=IsHondo?HondoDoc():Site11Doc();
  if(D.Id.IsEmpty()){UE_LOG(LogS47SC11,Error,TEXT("S47: SC11 missing completed remote record; no collection credit"));return true;}
  if(IsHondo)F.Hondo=true;else F.Site11=true;S.AddDoc(D);M->UI->Document(D);
  if(M->Audio)M->Audio->Play(TEXT("paper0"),.5f);
 }
 M->RequestAutosave(false);return true;
}

FString FS47SC11::Objective() const
{
 if(P==EStep::Return)return TEXT("SC11 // RETURN TO SARO. SECURE THE RECEIVER ORIGINAL AND THE TWO REMOTE RECORDS");
 if(P==EStep::Hunt)return TEXT("SARO // TAKE THE RECORDS OUT. THE EAST DOOR IS SHORT BUT EXPOSED; THE ANNEX AND WEST RAMP GIVE COVER");
 if(P==EStep::Done)return TEXT("SC11 // ORIGINALS OUT. GET TO THE MESA DINER");
 if(!F.Receiver)return TEXT("SARO // SECURE THE RECEIVER ORIGINAL AT THE OPS TERMINAL");
 if(!F.Event)return TEXT("SARO // START THE EVENT SHEET AT THE SHIFT JOURNAL");
 if(!F.CheckedLocalLight)return TEXT("SARO // CHECK THE LOCAL TASK LIGHT AT THE OPS TERMINAL");
 if(!F.ObservedRoom)return TEXT("SARO // WITH THE TASK LIGHT OFF, LOOK TOWARD THE FRONT WINDOWS");
 return TEXT("SARO // TAKE BOTH COMPLETED REMOTE SHEETS FROM THE PRINTER. NOTHING STAYS BEHIND");
}

TArray<FS47Task> FS47SC11::Tasks() const
{
 return {{TEXT("Return to SARO"),P!=EStep::Return},{TEXT("Secure the local receiver original"),F.Receiver},
  {TEXT("Start and keep the event sheet"),F.Event},{TEXT("Switch off the local task light and observe the other light"),F.CheckedLocalLight&&F.ObservedRoom},
  {TEXT("Secure Hondo's completed independent record, after 05:04"),F.Hondo},{TEXT("Secure Site 11's completed independent record, after 05:04"),F.Site11},
  {TEXT("Carry all the originals out of SARO"),F.Escaped}};
}

bool FS47SC11::Step(double Dt)
{
 if(!M||!M->Story||!M->World||!M->Web)return false;
 const bool Play=M->Mode==US47Main::EMode::Play&&!M->bPausedByMenu&&!(M->Flow&&M->Flow->bStarting);
 if(P==EStep::Done)return true;
 if(P==EStep::Return&&M->World->Area==TEXT("saro")&&!M->World->bDriving&&M->Player&&Room(FVector2D(M->Player->Pos.X,M->Player->Pos.Z))!=3&&Play)
 {
  M->Story->AdvanceClockTo(Arrive,TEXT("SC11 returned to SARO"));SetStep(EStep::Collect);
  M->UI->Toast(TEXT("The receiver has kept writing. The remote copies finish at 05:04. Take the originals; leave nothing you need behind."),6);
 }
 if(P==EStep::Collect||P==EStep::Hunt)StepLights(Play&&!M->ModalOpen()?Dt:0);
 if(!Play)return false;
 if(FadeT>=0){FadeT+=Dt;if(FadeT>1.2){FadeT=-1;if(M->UI)M->UI->Fade(false);}}
 if(bRestoreHunt&&M->World->Area==TEXT("saro")&&M->Player){bRestoreHunt=false;StartHunt(true);}
 if(P==EStep::Collect&&!M->ModalOpen())
 {
  ObserveRoom(Dt);
  if(SourcesSecured()&&F.CheckedLocalLight&&F.ObservedRoom)StartHunt(false);
 }
 if(P==EStep::Hunt)StepHunt(Dt);
 return P==EStep::Done;
}

void FS47SC11::StartHunt(bool bRetry)
{
 if(!SourcesSecured()||!F.CheckedLocalLight||!F.ObservedRoom)return;
 SetStep(EStep::Hunt);T=StateT=SeeT=LostT=DoorWait=0;Patrol=0;
 Pos=Nodes()[Office].At;Y=0;Yaw=HeadYaw=PI;Gait=Stride=0;bShown=false;bHavePlayer=false;bDriveStarted=false;
 Path.Reset();WaitingDoor.Reset();LastDoors.Reset();Enter(EBody::Approach);
 for(const auto& G:Gaps)LastDoors.Add(G.Door,M->Web->IsDoorOpen(G.Door));
 if(!bRetry)
 {
  M->Story->Note(FString::Printf(TEXT("%s. All four records secured before leaving the desk. Several lights search the building. A latch moves in the operations office."),*M->Story->ClockText(false)));
  M->UI->Toast(TEXT("Something is in the annex. All the records are in your folder. The east door is short and exposed; the corridor and west ramp give cover. Get out with them."),7);
 }
 UE_LOG(LogS47SC11,Display,TEXT("S47: SC11 hunt %s; originals kept; clock %.2f"),bRetry?TEXT("local retry"):TEXT("begins"),M->Story->Clock());
}

void FS47SC11::Enter(EBody Next)
{
 if(B!=Next)UE_LOG(LogS47SC11,Display,TEXT("S47: SC11 body %d -> %d at %.2f %.2f"),int32(B),int32(Next),Pos.X,Pos.Y);
 B=Next;StateT=0;Path.Reset();WaitingDoor.Reset();DoorWait=0;
}

int32 FS47SC11::Nearest(const FVector2D& At) const
{
 int32 Best=0;double Dist=1e9;
 for(int32 I=0;I<Nodes().Num();++I)
 {
  const double D=FVector2D::Distance(At,Nodes()[I].At)+(Room(At)==Room(Nodes()[I].At)?0:100);
  if(D<Dist){Dist=D;Best=I;}
 }
 return Best;
}

FString FS47SC11::ClosedDoorTo(const FVector2D& Target) const
{
 if(!M||!M->Web)return FString();
 for(const auto& G:Gaps)if(!M->Web->IsDoorOpen(G.Door)&&CrossesDoor(Pos,Target,G))return G.Door;
 return FString();
}

void FS47SC11::RouteTo(int32 Goal)
{
 Path.Reset();if(!Nodes().IsValidIndex(Goal))return;
 const int32 Start=Nearest(Pos),Count=Nodes().Num();
 TArray<int32> Prev;Prev.Init(-1,Count);TArray<double> Cost;Cost.Init(1e12,Count);Cost[Start]=0;
 // Short physical routes remain possible behind closed doors, but an open detour is preferred.
 // The body must then wait and visibly operate that actual door in Walk; no closed-door teleport.
 for(int32 Pass=0;Pass<Count;++Pass)for(const auto& E:Edges())for(int32 Dir=0;Dir<2;++Dir)
 {
  const int32 A=Dir?E.Value:E.Key,Z=Dir?E.Key:E.Value;double Extra=0;
  for(const auto& G:Gaps)if(M&&M->Web&&!M->Web->IsDoorOpen(G.Door)&&CrossesDoor(Nodes()[A].At,Nodes()[Z].At,G))Extra+=7;
  const double New=Cost[A]+FVector2D::Distance(Nodes()[A].At,Nodes()[Z].At)+Extra;
  if(New<Cost[Z]){Cost[Z]=New;Prev[Z]=A;}
 }
 if(Cost[Goal]>=1e11)return;
 for(int32 N=Goal,Guard=0;N>=0&&Guard<Count;++Guard){Path.Insert(N,0);if(N==Start)break;N=Prev[N];}
 if(Path.Num()>1&&FVector2D::Distance(Pos,Nodes()[Path[0]].At)<.2)Path.RemoveAt(0);
}

bool FS47SC11::ClearWalk(const FVector2D& From,const FVector2D& To) const
{
 if(!M || !M->Web || !M->Player)return false;
 constexpr double Radius=.27;
 for(const auto& C:M->Web->Colliders())if(C.bEnabled)
 {
  const FGap Expanded={TEXT("body"),C.MinX-Radius,C.MaxX+Radius,C.MinZ-Radius,C.MaxZ+Radius,0,false};
  if(Through(From,To,Expanded))return false;
 }
 const int32 Samples=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(From,To)/.05));
 for(int32 I=0;I<=Samples;++I){const auto At=FMath::Lerp(From,To,double(I)/Samples);if(!M->Player->Walkable(At.X,At.Y))return false;}
 return true;
}

bool FS47SC11::Walk(double Speed,double Dt)
{
 if(Path.IsEmpty()){Gait=FMath::Max(0.,Gait-Dt*3);return true;}
 const FVector2D To=Nodes()[Path[0]].At,D=To-Pos;const double Dist=D.Size();
 const FString Shut=ClosedDoorTo(To);
 if(!Shut.IsEmpty())for(const auto& G:Gaps)if(Shut==G.Door)
 {
  const FVector2D Center((G.X0+G.X1)*.5,(G.Z0+G.Z1)*.5);
  if(FVector2D::Distance(Pos,Center)<1.8)
  {
   if(WaitingDoor!=Shut)
   {
    WaitingDoor=Shut;DoorWait=0;
    if(M->Audio)M->Audio->Play(TEXT("thud"),.65f,S47Three::ToUE(Center.X,1.2,Center.Y));
   }
   DoorWait+=Dt;Gait=FMath::Max(0.,Gait-Dt*3);HeadYaw=Yaw=YawTo(Center-Pos);
   if(DoorWait<3.8)return false;
   M->Web->SetDoorOpen(Shut,true,false);LastDoors.Add(Shut,true);WaitingDoor.Reset();DoorWait=0;
   UE_LOG(LogS47SC11,Display,TEXT("S47: SC11 body opens actual %s after waiting in front of it"),*Shut);
  }
 }
 Yaw=HeadYaw=YawTo(D);const double Len=FMath::Min(Dist,Speed*Dt);
 const FVector2D Candidate=Dist>.00001?Pos+D/Dist*Len:To;
 if(!ClearWalk(Pos,Candidate)){Gait=FMath::Max(0.,Gait-Dt*3);return false;}
 Gait=FMath::Min(1.,Gait+Dt*3);Stride+=Len*2.1;
 if(Dist<=Len+.00001){Pos=To;Path.RemoveAt(0);return Path.IsEmpty();}
 Pos+=D/Dist*Len;return false;
}

bool FS47SC11::ClearSight(const FVector& From,const FVector& To) const
{
 if(!M||!M->Web||!M->Web->GetWorld())return false;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC11Sight),true);
 for(const auto& Part:Pale.All)if(Part.IsValid())Q.AddIgnoredComponent(Part.Get());
 const FVector A=S47Three::ToUE(From),Z=S47Three::ToUE(To);
 for(int32 I=0;I<24;++I)
 {
  FHitResult H;if(!M->Web->GetWorld()->LineTraceSingleByChannel(H,A,Z,ECC_Visibility,Q))return true;
  const auto* C=H.GetComponent();if(!C)return false;bool Solid=C->IsVisible();
  const auto* N=M->Web->Node(M->Web->NodeOf(C));if(N&&N->Name.StartsWith(TEXT("proxy")))Solid=false;
  if(const auto* Mat=C->GetMaterial(FMath::Max<int32>(0,H.ElementIndex)))
  {
   float Opacity=1;Mat->GetScalarParameterValue(TEXT("Opacity"),Opacity);
   if(Mat->GetBlendMode()==BLEND_Additive||(Mat->GetBlendMode()==BLEND_Translucent&&Opacity<.5f))Solid=false;
  }
  if(Solid)return false;Q.AddIgnoredComponent(C);
 }
 return false;
}

bool FS47SC11::SeesPlayer() const
{
 if(!bShown||!M||!M->Player||M->World->bDriving)return false;
 const FVector Cam=FS47Cam::Of(M->Player).Pos;const FVector2D Player(Cam.X,Cam.Z),D=Player-Pos;const double Dist=D.Size();
 if(Dist>15)return false;
 const int32 A=Room(Pos),Z=Room(Player);
 if(A!=Z)
 {
  const auto Open=[this,&Player](int32 G){return M->Web->IsDoorOpen(Gaps[G].Door)&&Through(Pos,Player,Gaps[G]);};
  const auto Pair=[A,Z](int32 X,int32 Yp){return (A==X&&Z==Yp)||(A==Yp&&Z==X);};
  if(!((Pair(1,3)&&Open(0))||(Pair(1,2)&&Open(1))||(Pair(0,1)&&Open(2))||(Pair(0,2)&&Open(1)&&Open(2))||(Pair(0,3)&&Open(3))))return false;
 }
 if(Dist>.4&&FVector2D::DotProduct(FVector2D(FMath::Cos(HeadYaw),FMath::Sin(HeadYaw)),D/Dist)<.342)return false;
 return ClearSight(FVector(Pos.X,Y+1.9,Pos.Y),Cam);
}

bool FS47SC11::BeamSeesPlayer(int32 Index) const
{
 if(!Beams.IsValidIndex(Index)||!Beams[Index].IsValid()||!M||!M->Player||M->World->bDriving)return false;
 const auto* L=Beams[Index].Get();if(!L->IsVisible())return false;
 const FVector Eye=M->Player->EyeUE(),D=Eye-L->GetComponentLocation();const double Dist=D.Size();
 if(Dist>L->AttenuationRadius||Dist<1)return false;
 if(FVector::DotProduct(L->GetForwardVector(),D/Dist)<FMath::Cos(FMath::DegreesToRadians(L->OuterConeAngle)))return false;
 return ClearSight(S47Three::ToThree(L->GetComponentLocation()),S47Three::ToThree(Eye));
}

TOptional<FVector2D> FS47SC11::Heard(double Dt)
{
 const FVector2D Player(M->Player->Pos.X,M->Player->Pos.Z);
 const double Speed=bHavePlayer?FVector2D::Distance(Player,LastPlayer)/FMath::Max(.001,Dt):0;
 LastPlayer=Player;bHavePlayer=true;HeardCooldown-=Dt;
 TOptional<FVector2D> Noise;
 for(const auto& G:Gaps)
 {
  const bool Open=M->Web->IsDoorOpen(G.Door);const bool* Was=LastDoors.Find(G.Door);
  const FVector2D At((G.X0+G.X1)*.5,(G.Z0+G.Z1)*.5);
  if(Was&&*Was!=Open&&FVector2D::Distance(Pos,At)<14)Noise=At;
  LastDoors.Add(G.Door,Open);
 }
 if(Noise)return Noise;
 if(Speed<.4||Speed>12||HeardCooldown>0)return {};
 int32 Shut=0;
 for(const auto& G:Gaps)if(!M->Web->IsDoorOpen(G.Door)&&Through(Pos,Player,G))++Shut;
 const double Range=(Speed>2.5?15.:9.)/FMath::Pow(3.,Shut);
 if(FVector2D::Distance(Pos,Player)>Range)return {};
 HeardCooldown=.5;return Player;
}

double FS47SC11::Ground() const
{
 if(Room(Pos)!=3||!M||!M->Web||!M->Web->GetWorld())return 0;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC11Ground),true);for(const auto& C:Pale.All)if(C.IsValid())Q.AddIgnoredComponent(C.Get());
 FHitResult H;if(!M->Web->GetWorld()->LineTraceSingleByChannel(H,S47Three::ToUE(Pos.X,Y+1.5,Pos.Y),S47Three::ToUE(Pos.X,Y-3,Pos.Y),ECC_Visibility,Q))return Y;
 return S47Three::ToThree(H.ImpactPoint).Y;
}

bool FS47SC11::AtEscape() const
{
 // Cover deliberately hides the pursuer. Escape must not require its torso to be
 // on the player's screen at the moment they reach safety.
 const bool BodyPresent=Pale.bBuilt&&Pale.All.Num()==FS47PaleBody::JointCount&&!Pale.All.ContainsByPredicate([](const auto& C){return !C.IsValid()||!C->IsVisible();});
 if(!M||!M->Player||!M->World||!bShown||!BodyPresent||T<3||!SourcesSecured()||!F.CheckedLocalLight||!F.ObservedRoom)return false;
 if(M->World->bDriving)
 {
  const FVector Cam=FS47Cam::Of(M->Player).Pos;
  return M->World->Area==TEXT("saro")&&bDriveStarted&&FVector2D::Distance(FVector2D(Cam.X,Cam.Z),FVector2D(DriveStart.X,DriveStart.Z))>5.;
 }
 const FVector Pp=M->Player->Pos;
 return M->World->Area==TEXT("saro")&&Room(FVector2D(Pp.X,Pp.Z))==3&&Pp.X>=6.6&&Pp.X<=13&&Pp.Z>=2.7&&Pp.Z<=12.4;
}

void FS47SC11::Capture()
{
 ++F.Captures;if(M->UI)M->UI->Fade(true);FadeT=0;if(M->Audio)M->Audio->Gasp();
 M->Player->Place(RestartAt.X,RestartAt.Y,0);M->Player->Pitch=-.04;
 M->Web->SetDoorOpen(TEXT("doorSouth"),false,true);M->Web->SetDoorOpen(TEXT("doorEast"),false,true);
 StartHunt(true);
 UE_LOG(LogS47SC11,Display,TEXT("S47: SC11 capture %d; restart inside SARO, originals retained; clock unchanged"),F.Captures);
 M->RequestAutosave(false);
}

void FS47SC11::Finish()
{
 if(F.Escaped)return;
 F.Escaped=true;EscapedAt=M->Story->ClockText(true);M->Story->AddDoc(ReceiverDoc(true,EscapedAt));
 M->Story->Note(FString::Printf(TEXT("%s. The local lamp was off; the other light continued. I observed the room and carried all the originals out. SARO, Hondo and Site 11 remain three separate sources."),*EscapedAt));
 SetStep(EStep::Done);Stop();
 if(M->UI)M->UI->Toast(TEXT("The originals are out. Get them to Mesa Diner. Ward must hear which sources they came from."),6);
 M->RequestAutosave(false);
 UE_LOG(LogS47SC11,Display,TEXT("S47: SC11 actual world escape complete, clock %s, captures %d"),*EscapedAt,F.Captures);
}

void FS47SC11::StepHunt(double Dt)
{
 const bool Held=!M->Player||M->Mode!=US47Main::EMode::Play||M->bPausedByMenu||M->ModalOpen()||M->Story->IsCinematic()||FadeT>=0||(M->Flow&&M->Flow->bStarting);
 if(!Pale.bBuilt&&!Pale.Build(M->Web.Get(),TEXT("#root"),TEXT("SC11Pale")))return; // Missing body cannot certify an escape.
 if(!Held)
 {
  T+=Dt;StateT+=Dt;const double Before=Stride;
  if(M->World->bDriving&&!bDriveStarted){bDriveStarted=true;DriveStart=FS47Cam::Of(M->Player).Pos;}
  if(AtEscape()){Finish();return;}
  const FVector Cam=FS47Cam::Of(M->Player).Pos;const FVector2D Player(Cam.X,Cam.Z);
  const bool Seen=SeesPlayer();bool Lit=false;for(int32 I=0;I<Beams.Num();++I)Lit|=BeamSeesPlayer(I);
  SeeT=Seen?SeeT+Dt:FMath::Max(0.,SeeT-Dt);const auto Noise=Heard(Dt);
  if(Seen||Lit){LastKnown=Player;LostT=0;}else LostT+=Dt;
  static const int32 Round[]={Corridor,ControlDoor,Control,ControlEast,EastDoor,Landing,EastWalk,EastSteps,NorthSteps,North,NorthWest,West,Ramp,Stoop,Exit,RecordsDoor,Records,RecordsDoor,Corridor,Office};
  const auto Search=[this](){RouteTo(Round[Patrol++%UE_ARRAY_COUNT(Round)]);};
  switch(B)
  {
  case EBody::Approach:
   if(StateT>2.2){bShown=true;if(M->Audio)M->Audio->Play(TEXT("door_latch"),.6f,S47Three::ToUE(Pos.X,1.2,Pos.Y));Enter(EBody::Search);Search();}
   break;
  case EBody::Search:
   if(SeeT>=.35){Enter(EBody::Chase);RouteTo(Nearest(LastKnown));break;}
   if(Lit||Noise){LastKnown=Lit?Player:*Noise;Enter(EBody::Listen);HeadYaw=YawTo(LastKnown-Pos);RouteTo(Nearest(LastKnown));break;}
   if(Walk(1.15,Dt))Search();
   break;
  case EBody::Listen:
   HeadYaw=YawTo(LastKnown-Pos);
   if(SeeT>=.35){Enter(EBody::Chase);RouteTo(Nearest(LastKnown));break;}
   if(Noise){LastKnown=*Noise;RouteTo(Nearest(LastKnown));}
   Walk(1.05,Dt);
   if(StateT>5){Enter(EBody::Search);Search();}
   break;
  case EBody::Chase:
  {
   if(LostT>2.6){Enter(EBody::Listen);RouteTo(Nearest(LastKnown));break;}
   const FVector2D D=LastKnown-Pos;const double Dist=D.Size();
   const FVector2D Candidate=Dist>.001?Pos+D/Dist*FMath::Min(2.05*Dt,Dist):Pos;
   if(Room(Pos)==Room(LastKnown)&&ClosedDoorTo(LastKnown).IsEmpty()&&ClearWalk(Pos,Candidate)&&ClearSight(FVector(Pos.X,Y+1.2,Pos.Y),FVector(LastKnown.X,1.2,LastKnown.Y)))
   {Pos=Candidate;Yaw=HeadYaw=YawTo(D);Gait=1.2;Stride+=2.05*Dt*2.1;}
   else{if(Path.IsEmpty())RouteTo(Nearest(LastKnown));Walk(2.05,Dt);}
   if(!M->World->bDriving&&FVector2D::Distance(Pos,Player)<.85&&ClearSight(FVector(Pos.X,Y+1.2,Pos.Y),Cam))Enter(EBody::Grab);
   break;
  }
  case EBody::Grab:
   if(M->World->bDriving||FVector2D::Distance(Pos,Player)>1.25||!ClearSight(FVector(Pos.X,Y+1.2,Pos.Y),Cam)){Enter(EBody::Chase);break;}
   Gait=0;HeadYaw=YawTo(Player-Pos);if(StateT>.7)Capture();
   break;
  }
  if(bShown)Y=Ground();
  if(bShown&&M->Audio&&FMath::FloorToDouble(Before/PI)!=FMath::FloorToDouble(Stride/PI))
  {
   const auto Pick=M->Audio->StepPick(Room(Pos)==3?TEXT("concrete"):TEXT("tile"));FS47Sound O;O.Gain=Pick.Gain*.8;O.At=FVector(Pos.X,Y+.05,Pos.Y);M->Audio->Sfx(Pick.Name,O);
  }
 }
 Pale.Apply(bShown&&M->World->Area==TEXT("saro"),FVector(Pos.X,Y,Pos.Y),Yaw,HeadYaw,Gait,Stride,Clock,
  B==EBody::Chase?FS47PaleBody::EPose::Chase:B==EBody::Grab?FS47PaleBody::EPose::Grab:B==EBody::Listen?FS47PaleBody::EPose::Listen:FS47PaleBody::EPose::Walk);
}

void FS47SC11::TestComplete()
{
 // A declared preceding-scenario fixture for SC12/whole-night boundary probes.
 F.Receiver=F.Event=F.Hondo=F.Site11=F.CheckedLocalLight=F.ObservedRoom=F.Escaped=true;
 P=EStep::Done;EscapedAt=TEXT("05:10:00");Refile();
}

bool FS47SC11::SelfTest(TArray<FString>& Lines)
{
 bool Good=true;const auto Check=[&](bool Ok,const TCHAR* Name){Lines.Add((Ok?TEXT("ok  "):TEXT("FAIL "))+FString(Name));Good&=Ok;};
 Check(RemoteReady==18240,TEXT("SC11 remote copies ready at 05:04"));
 Check(!ReceiverDoc(false).Page.Contains(TEXT("WORLD ESCAPE RECORDED")),TEXT("SC11 partial original never certifies escape"));
 Check(HondoDoc().Page.Contains(TEXT("04:56:55"))&&Site11Doc().Page.Contains(TEXT("04:56:56")),TEXT("SC11 independent clocks retain the middle-boundary difference"));
 Check(HondoDoc().Page.Contains(TEXT("18.4 km"))&&HondoDoc().Page.Contains(TEXT("480 m"))&&Site11Doc().Page.Contains(TEXT("620 m"))&&Site11Doc().Page.Contains(TEXT("23.8 km")),TEXT("SC11 complete authored optical tables and separate baselines retained"));
 Check(Site11Doc().Page.Contains(TEXT("04:57:42"))&&Site11Doc().Page.Contains(TEXT("disconnected"))&&HondoDoc().Page.Contains(TEXT("battery-backed")),TEXT("SC11 raw feed/power controls and authored third boundary retained"));
 Check(HondoDoc().Page.Contains(TEXT("04:56:00 to 05:04:00"))&&Site11Doc().Page.Contains(TEXT("04:56:00 to 05:04:00")),TEXT("SC11 full timestamp shift preserves both remote observation interval endpoints"));
 Check(!EventDoc().Page.Contains(TEXT("WORLD ESCAPE RECORDED")),TEXT("SC11 started event sheet remains incomplete"));
 FS47SC11 Source(nullptr);auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("step"),int32(EStep::Hunt));S47J::Set(O,TEXT("receiver"),true);
 Source.Begin(O);FS47SC11 Fresh(nullptr);Fresh.Begin(S47J::Read(S47J::Write(Source.Save())));
 Check(Fresh.Phase()==EStep::Hunt&&Fresh.Facts().Receiver&&!Fresh.Facts().Escaped,TEXT("SC11 hunt JSON restore retains collection without creating escape"));
 Check(Room(Nodes()[Control].At)==0&&Room(Nodes()[Records].At)==2&&Room(Nodes()[Stoop].At)==3,TEXT("SC11 route uses measured control, records and outside rooms"));
 Check(Room(Nodes()[North].At)==3&&Room(FVector2D(-3.,-12.))==3&&Room(FVector2D(-3.,-650.))==3,TEXT("SC11 front perimeter and distant motel never count as annex arrival"));
 Check(!CrossesDoor(FVector2D(-2.4,6.05),FVector2D(-5.3,5.7),Gaps[1])&&CrossesDoor(FVector2D(-3.9,6.05),FVector2D(-3.9,8.2),Gaps[1]),TEXT("SC11 passing the records doorway is distinct from crossing its actual closed plane"));
 TSet<int32> Reached={Control};for(int32 Pass=0;Pass<Nodes().Num();++Pass)for(const auto& E:Edges())if(Reached.Contains(E.Key)||Reached.Contains(E.Value)){Reached.Add(E.Key);Reached.Add(E.Value);}
 Check(Reached.Num()==Nodes().Num(),TEXT("SC11 both exits and the covered perimeter belong to one connected physical route"));
 return Good;
}
