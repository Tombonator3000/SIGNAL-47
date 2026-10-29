#include "S47Evening.h"
#include "S47EveningVisual.h"
#include "S47Takeaway.h"
#include "S47Substation.h"
#include "S47Horror.h"
#include "S47Observing.h"
#include "S47Anomaly.h"
#include "S47Office.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47Audio.h"
#include "S47Flow.h"
#include "S47Kit.h"
#include "S47Inter.h"
#include "S47UI.h"
#include "S47Media.h"
#include "S47Radio.h"
#include "S47Score.h"
#include "S47RecordedDialogue.h"
#include "S47Chapter.h"
#include "S47Timeline.h"
#include "Misc/CommandLine.h"

namespace
{
 const TCHAR* FiledRoutineObjective(const AS47Story& Story)
 {
  if (!Story.BankIn()) return TEXT("CONTROL ROOM // RESTORE RX BANK 3");
  return Story.Rx.Stage == 0 ? TEXT("CONTROL ROOM // CALIBRATE ON 1419.900 MHz")
   : TEXT("CONTROL ROOM // NOTCH THE HIGHWAY RELAY NEAR 1420.1 TO START THE ROUTINE SURVEY");
 }
}

FS47Evening::FS47Evening(US47Main* InM) : M(InM)
{
 FString Error;
 if (!Pack.Initialize(Error) || !RadioPack.Initialize(Error)) UE_LOG(LogTemp, Error, TEXT("S47: evening pack: %s"), *Error);
 Visual = MakeUnique<FS47EveningVisual>(M);
 Takeaway = MakeUnique<FS47Takeaway>(M, Pack);
 Voice = MakeShared<FS47RecordedDialogue>(M->Audio, TEXT("evening"), [](const FS47DialogueInput& I, FS47RadioVoice& Bank) -> TOptional<TArray<FS47RadioClip>>
 {
  if (I.Lines.Num() != 1) return {};
  return Bank.Resolve(I.Lines[0].Text, 0, I.Voice);
 }, [this](const FS47RadioClip&) { const auto S = Pack.CurrentScene(); return S && (S->Channel == TEXT("phone") || S->Channel == TEXT("walkie") || S->Channel == TEXT("radio")); }, [this](const FS47RadioClip&)
 {
  FS47RecordedDialogue::FOutput Out;
  if (const auto* S=Pack.CurrentScene())
  {
   Out.Channel=S->Channel; Out.bRadioBus=S->Channel==TEXT("radio");
   if (S->Channel==TEXT("physical"))
    if (const auto* I=M->Inter->Get(TEXT("diner:waitress"))) Out.At=I->Object.WorldPosition();
  }
  return Out;
 });
 RadioVoice = MakeShared<FS47RecordedDialogue>(M->Audio, TEXT("evening"), [](const FS47DialogueInput& I, FS47RadioVoice& Bank) -> TOptional<TArray<FS47RadioClip>>
 {
  if (I.Lines.Num() != 1) return {};
  return Bank.Resolve(I.Lines[0].Text, 0, I.Voice);
 }, [](const FS47RadioClip&) { return true; }, [](const FS47RadioClip&)
 {
  FS47RecordedDialogue::FOutput Out; Out.bRadioBus=true; Out.Channel=TEXT("car-radio"); return Out;
 }); // Receiver bandwidth followed by the player's radio volume bus.
 // Physical triggers are explicit: a choice-only scene can never become a global use target.
 const auto Previous = M->World->OnParked;
 M->World->OnParked = [this, Previous](const FString& Place) { if (Previous && !Opening()) Previous(Place); Parked(Place); };
}
FS47Evening::~FS47Evening()
{
 // UObject purge order can destroy AS47Web's node arrays before US47Main.
 // Stop() is for a live night/title transition; destruction only releases owned audio.
 if (RadioCaptionAlive) *RadioCaptionAlive=false;
 if (Voice) Voice->Stop();
 if (RadioVoice) RadioVoice->Stop();
 if (Music) { Music->Stop(); Music->Disconnect(); }
 if (MusicGain) MusicGain->Disconnect();
}

bool FS47Evening::MealBusy() const { return bEnabled && Takeaway && Takeaway->MealBusy(); }

bool FS47Evening::ServicePending() const
{
 if (NightSequence()) return FeederOutage() && Pack.Flag(TEXT("handover_done"));
 return bEnabled && bServiceRoute && !bServiceFiled && Pack.Flag(TEXT("handover_done")) &&
  M && M->Story && M->Story->bEveningTimeline && M->Story->Phase() == ES47Phase::Shift;
}

bool FS47Evening::AtShiftFolder() const
{
 return M && M->World && M->Player && M->Web && M->Mode == US47Main::EMode::Play &&
  !M->World->bDriving && M->World->Area == TEXT("saro") &&
  M->Web->Rect(TEXT("room.bounds")).Contains(M->Player->Pos);
}

FString FS47Evening::ServiceObjective() const
{
 if (!ServicePending()) return FString();
 if (NightSequence())
 {
  if (!bFeederCallAnswered) return TEXT("SARO // ANSWER THE COUNTY CALL. CALICHE JUNCTION FEEDER IS OUT; K3 WILL NOT HOLD");
  return M->World && M->World->Area == TEXT("saro")
   ? TEXT("CALICHE JUNCTION // TAKE SARO 07 TO THE SIGNED GRAVEL TURNOFF. RESTORE THE FEEDER TO BRING THE ARRAY BACK")
   : TEXT("CALICHE JUNCTION // READ THE JOURNAL, RUN THE ENCLOSED CHECK AND VERIFY THE MARKED LAMP TO RESTORE THE FEEDER");
 }
 if (M->Substation && M->Substation->HasNormalResult())
  return AtShiftFolder() ? TEXT("SHIFT FOLDER // FILE THE COMPLETED INSPECTION CARD, THEN START THE ROUTINE SURVEY")
   : TEXT("SARO // RETURN TO THE CONTROL-ROOM SHIFT FOLDER WITH THE COMPLETED INSPECTION CARD");
 if (M->World && M->World->Area == TEXT("saro"))
  return TEXT("CALICHE JUNCTION // TAKE SARO 07 SOUTH TO THE SIGNED GRAVEL TURNOFF. CHECK THE INSPECTION LAMP OUTSIDE THE FENCE");
 return TEXT("CALICHE JUNCTION // READ THE JOURNAL, RUN THE ENCLOSED SERVICE CHECK, THEN COMPARE THE MARKED LAMP");
}

TArray<FS47Task> FS47Evening::ServiceTasks() const
{
 if (!ServicePending()) return {};
 if (NightSequence()) return {
  {TEXT("Answer the county call about the failed Caliche Junction feeder"), bFeederCallAnswered},
  {TEXT("Drive SARO 07 to Caliche Junction after midnight"), bServiceDriveSkipped},
  {TEXT("Read the local journal, run the check and verify the marked lamp to restore SARO power"), false}};
 const bool Journal = M->Substation && M->Substation->HasJournal();
 const bool Compared = M->Substation && M->Substation->HasNormalResult();
 return {{TEXT("Read the service journal at Caliche Junction's marked inspection front"), Journal},
  {TEXT("Run the local check and compare its completed result with the marked lamp"), Compared},
  {TEXT("Return to SARO and file the inspection card in the control-room shift folder before starting the overnight survey"), false}};
}
void FS47Evening::ValidateServiceRestore()
{
 if (NightSequence())
 {
  // Only this route's actual T01 result can restore its feeder. A missing result resumes the repair.
  if (bFeederRestored && (!M || !M->Substation || !M->Substation->HasNormalResult())) bFeederRestored = false;
  if (M && M->Story) M->Story->ApplyFeederPower(FeederOutage());
  return;
 }
 if (!bEnabled || !bServiceRoute || !bServiceFiled) return;
 if (!M || !M->Substation || !M->Substation->HasNormalResult() || !Pack.Flag(TEXT("trafo_normal_checked")))
 {
  // Begin runs before Substation::Begin. Never borrow the previous case's result.
  bServiceFiled = false;
  UE_LOG(LogTemp, Warning, TEXT("S47: T01 filed credit rejected after restore: current case has no complete physical inspection record"));
 }
}

void FS47Evening::SignalOutage()
{
 if (!NightSequence() || bFeederFailed || !M || !M->Story) return;
 bFeederFailed = true; bFeederRestored = bFeederCallAnswered = bServiceDriveSkipped = false;
 M->Story->ApplyFeederPower(true);
 M->Story->ConsoleLog(TEXT("00:13:47 SIGNAL TRANSIENT / CALICHE FEEDER LOST / K3 OPEN / ARRAY DRIVE OFF"), S47Timeline::Residual(true));
 M->Story->Note(TEXT("00:13:47. The residual struck at the same instant as the feeder failed. K3 stays open. The array cannot move. Emergency illumination only."));
 RingAt = M->T + .8;
 M->UI->Objective(ServiceObjective());
 M->RequestAutosave();
}
void FS47Evening::AnswerFeederCall()
{
 if (!FeederOutage() || bFeederCallAnswered || bDialogueOpen) return;
 StopRing(); bDialogueOpen = true;
 if (M->Audio) M->Audio->Play(TEXT("handset_up"), .5f, M->Web->PropLocation(TEXT("phone")));
 M->Web->SetHandsetVisible(false);
 FS47Doc D; D.Id = TEXT("EV_FEEDER_CALL"); D.Title = TEXT("County power dispatch / Caliche Junction"); D.Kind = TEXT("Telephone transcript"); D.Stamp = M->Story->ClockText(true);
 D.Page = TEXT("COUNTY POWER DISPATCH\n\nThe Caliche Junction feeder opened at thirteen minutes past midnight. We have lost the SARO branch. Your emergency circuit will hold the room lights, but it cannot drive the array.\n\nTake SARO 07 south to the signed gravel turnoff. Read the station journal, run the enclosed service check and verify the marked lamp. Keep outside the fence. When the circuit is stable, the feeder can be brought back.\n\nDo not keep forcing K3. It will not hold until the supply is restored.");
 D.Transcript = D.Page;
 M->UI->Document(D, [this, D]
 {
  bDialogueOpen = false; bFeederCallAnswered = true; M->Web->SetHandsetVisible(true);
  if (M->Audio) M->Audio->Play(TEXT("handset_down"), .5f, M->Web->PropLocation(TEXT("phone")));
  M->Story->AddDoc(D); M->Story->Note(TEXT("County dispatch confirmed the SARO feeder fault at Caliche Junction. Take SARO 07 and restore the supply before trying to track the signal."));
  M->UI->Objective(ServiceObjective()); M->RequestAutosave();
 });
}
void FS47Evening::NightStep()
{
 if (!NightSequence() || !M || !M->Story || !M->World || M->bPausedByMenu) return;
 // The measured lamp result commits the supply change even while its record is open.
 // H02 presentation still owns the separate SC02 gate in Horror::StepSC02.
 if (FeederOutage() && bFeederCallAnswered && M->Substation && M->Substation->HasNormalResult())
 {
  // A physical journey on foot must not leave the later encounter unreachable.
  if (!bServiceDriveSkipped)
  {
   bServiceDriveSkipped = true;
   M->Story->AdvanceClockTo(S47Timeline::FeederDrive, TEXT("Caliche Junction reached on foot"));
  }
  bFeederRestored = true; StopRing();
  M->Story->ApplyFeederPower(false);
  M->Story->ConsoleLog(TEXT("CALICHE FEEDER STABLE / SARO SUPPLY RESTORED / K3 CLOSED"));
  M->Story->Note(M->Story->ClockText(true) + TEXT(". The checked circuit held. SARO power and the array drive are back. The unlogged residual is still waiting at the receiver."));
  M->UI->Toast(TEXT("Feeder stable. SARO power restored. K3 holds and the array can move again."), 6);
  M->UI->Objective(TEXT("CALICHE JUNCTION // KEEP THE COMPLETED FEEDER RECORD. RETURN TO SARO 07"));
  M->RequestAutosave();
 }
 if (M->ModalOpen() || M->Story->IsCinematic()) return;
 if (!bFeederFailed)
 {
  if (M->Horror && M->Horror->BaselineChecked()) M->Story->AdvanceClockTo(S47Timeline::Calibration, TEXT("receiver reference complete"));
  if (M->Observing && M->Observing->Done()) M->Story->AdvanceClockTo(-3600, TEXT("evening schedule filed / north-window round"));
  return;
 }
 if (FeederOutage() && !bFeederCallAnswered && M->World->Area == TEXT("saro") && !bRing && M->T >= RingAt)
 {
  bRing = true;
  M->Audio->Loop(TEXT("evening:ring"), TEXT("phone_ring"), .8f, M->Web->PropLocation(TEXT("phone")));
 }
 if (FeederOutage() && bFeederCallAnswered && !bServiceDriveSkipped && M->World->bDriving && M->World->Area == TEXT("road"))
 {
  bServiceDriveSkipped = true;
  M->Story->AdvanceClockTo(S47Timeline::FeederDrive, TEXT("drive to Caliche Junction"));
  M->Story->Toast(TEXT("14 APRIL 1986 / 01:35. Caliche Junction feeder call."), 5);
  M->RequestAutosave();
 }
}

FVector FS47Evening::PersonalCarPosition() const { return Visual->ParkedPosition(); }
double FS47Evening::PersonalCarHeading() const { return Visual->ParkedHeading(); }

void FS47Evening::StopRing()
{
 if (bRing && M->Audio) M->Audio->Stop(TEXT("evening:ring"),0.05f);
 bRing = false;
 if (M->Web) M->Web->SetHandsetRattle(false);
}
void FS47Evening::Stop()
{
 if (RadioCaptionAlive) *RadioCaptionAlive=false;
 RadioCaptionAlive.Reset(); bRadioCaptionShown=false;
 StopRing(); if (Voice) Voice->Stop();
 if (RadioVoice) RadioVoice->Stop();
 if (Music) { Music->Stop(); Music->Disconnect(); Music.Reset(); }
 if (MusicGain) { MusicGain->Disconnect(); MusicGain.Reset(); }
 bEnabled = false; bDialogueOpen = false; bRadioRecorded = false; SpokenLine.Empty(); RadioSpokenLine.Empty();
 bServiceRoute = bServiceFiled = bInvestigationOnly = false;
 bNightSequence = bFeederFailed = bFeederRestored = bFeederCallAnswered = bServiceDriveSkipped = false;
 if (M && M->Story) M->Story->ApplyFeederPower(false);
 if (M && M->Web) M->Web->SetHandsetVisible(true);
 if (M && M->Story) M->Story->bOpeningHeld = false;
 if (M && M->World) M->World->bCommuteVehicle = false;
 if (Takeaway) Takeaway->Stop();
 if (Visual) {Visual->SetParked(false);Visual->Hide();}
}
void FS47Evening::Begin(const S47J::FObj& Saved, bool bPastOpening)
{
 Stop(); Pack.Reset(); RadioPack.Reset(); bEnabled = true;
 // Versioned opt-in belongs only to new nights. Missing/unknown versions retain the old route.
 bServiceRoute = !Saved && !bPastOpening; bServiceFiled = false;
 bNightSequence = !Saved && !bPastOpening;
 bWorkStarted = false; bRadioOn = true; RadioHold = RadioRest = MusicPosition = 0;
 FString Error;
 if (Saved)
 {
  if (!Pack.Load(S47J::Obj(Saved,TEXT("pack")),Error)) { bEnabled=false; M->Story->bOpeningHeld=false; M->UI->Toast(TEXT("The evening state could not be restored: ")+Error,12); UE_LOG(LogTemp, Error,TEXT("S47: evening restore: %s"),*Error); return; }
  const int32 RouteVersion = S47J::Int(Saved,TEXT("serviceRouteVersion"));
  bNightSequence = RouteVersion == 2; bServiceRoute = RouteVersion == 1 || bNightSequence;
  bFeederFailed = bNightSequence && S47J::Bool(Saved, TEXT("feederFailed"));
  bFeederRestored = bFeederFailed && S47J::Bool(Saved, TEXT("feederRestored"));
  bFeederCallAnswered = bFeederFailed && S47J::Bool(Saved, TEXT("feederCallAnswered"));
  bServiceDriveSkipped = bFeederCallAnswered && S47J::Bool(Saved, TEXT("serviceDriveSkipped"));
  bServiceFiled = bServiceRoute && S47J::Bool(Saved,TEXT("serviceFiled")) && Pack.Flag(TEXT("trafo_normal_checked"));
  if (S47J::Has(Saved,TEXT("radioPack"))) RadioPack.Load(S47J::Obj(Saved,TEXT("radioPack")),Error);
  bRadioOn = S47J::Bool(Saved,TEXT("radioOn"),true);
  RadioHold = S47J::Num(Saved,TEXT("radioHold")); RadioRest = S47J::Num(Saved,TEXT("radioRest"));
  MusicPosition = S47J::Num(Saved,TEXT("musicPosition"));
 }
 if (!Saved && bPastOpening)
 {
  auto State=Pack.Save(); auto Flags=S47J::Obj(State,TEXT("flags"));
  for (const TCHAR* K : {TEXT("intro_arrived"),TEXT("intro_completed"),TEXT("handover_done")}) S47J::Set(Flags,K,true);
  S47J::Set(State,TEXT("phase"),TEXT("early_shift")); Pack.Load(State,Error);
 }
 M->Story->bEveningTimeline = true;
 M->Story->bOpeningHeld = Opening();
 bool bMigratedArrival=false;
 if (Commute())
 {
  // Fresh commute begins before the shift. Continue keeps the clock actually saved.
  M->Story->SetClock(S47J::Num(Saved,TEXT("clock"),-6*3600-40*60));
  const S47J::FObj Car = S47J::Obj(Saved,TEXT("car"));
  if (!Saved || S47J::Bool(Car,TEXT("driving"))) M->World->BeginCommute(Car);
  else
  {
   M->World->bCommuteVehicle = true;
   const FString At = S47J::Str(Car,TEXT("at"),TEXT("diner"));
   const auto P = S47J::New();
   for (const TCHAR* K : {TEXT("x"),TEXT("y"),TEXT("z"),TEXT("h")}) S47J::Set(P,K,S47J::Num(Car,K));
   S47J::Set(P,TEXT("at"),At);
   if (At==TEXT("saro") && S47J::Int(Saved,TEXT("arrivalParkingVersion"))<2)
   {
    const FVector Old(S47J::Num(P,TEXT("x")),S47J::Num(P,TEXT("y")),S47J::Num(P,TEXT("z")));
    Visual->RestorePersonal(Old,S47J::Num(P,TEXT("h")),0);
    bMigratedArrival=!Visual->ParkedPosition().Equals(Old,.01);
    if(bMigratedArrival){const FVector Bay=FS47EveningVisual::ArrivalPark();S47J::Set(P,TEXT("x"),Bay.X);S47J::Set(P,TEXT("y"),Bay.Y);S47J::Set(P,TEXT("z"),Bay.Z);S47J::Set(P,TEXT("h"),0.0);}
   }
   M->World->PlaceTruck(At,P);
   M->World->Enter(S47J::Str(Saved,TEXT("area"),TEXT("diner")));
  }
 }
 else
 {
  const auto Visitor=S47J::Obj(Saved,TEXT("personalPark"));
  if (Visitor) Visual->RestorePersonal(FVector(S47J::Num(Visitor,TEXT("x")),S47J::Num(Visitor,TEXT("y")),S47J::Num(Visitor,TEXT("z"))),S47J::Num(Visitor,TEXT("h")),S47J::Int(Saved,TEXT("arrivalParkingVersion")));
  Visual->SetParked(true);
  if (Opening()) { M->Story->SetClock(S47J::Num(Saved,TEXT("clock"),-21600)); RingAt = M->T; }
  bWorkStarted = Pack.Flag(TEXT("handover_done"));
 }
 Takeaway->Begin(S47J::Obj(Saved,TEXT("takeaway")));
 // A previously rejected stationary save already on the visitor lot now completes
 // the same arrival transaction. Keep its actual car pose; no diner/phone shortcut.
 if(bMigratedArrival || (Saved && Commute() && !M->World->bDriving && M->World->Area==TEXT("saro") && M->World->TruckOnLot())) Parked(TEXT("saro"));
 Context(); Refile();
 if (Opening()) M->UI->Objective(Commute() ? TEXT("EVENING SHIFT // DRIVE NORTH TO SARO. STOP AT MESA DINER IF YOU FEEL LIKE SUPPER.") : TEXT("SARO // ANSWER DALE'S CALL IN THE CONTROL ROOM"));
 if (Pack.CurrentScene()) Resume();
 UE_LOG(LogTemp,Display,TEXT("S47: evening %s, phase %s, scene %s"),Saved ? TEXT("restored") : TEXT("new"),*Pack.GetPhase(),Pack.CurrentScene()?*Pack.CurrentScene()->Id:TEXT("none"));
}

void FS47Evening::Context()
{
 if (!bEnabled || !M->Story) return;
 const FString K = M->Story->ChapterKey;
 auto State = [&](const TCHAR* Ch) { const US47Chapter* C = M->Story->Chapter(Ch); return C && M->Story->Begun.Contains(Ch) ? C->SaveState() : S47J::FObj(); };
 Pack.SetExternal(TEXT("ctx_signal_present"),M->Story->bResidualVisible && M->Story->bEveningTimeline);
 Pack.SetExternal(TEXT("ctx_archive_open"),M->Story->Begun.Contains(TEXT("ch2")));
 Pack.SetExternal(TEXT("ctx_nora_contacted"),S47J::Bool(State(TEXT("ch4")),TEXT("greeted")));
 Pack.SetExternal(TEXT("ctx_all_night_active"),K == TEXT("ch5"));
 Pack.SetExternal(TEXT("ctx_power_available"),M->Story->bPowered && !M->Story->bDark);
 Pack.SetExternal(TEXT("ctx_room_quiet"),!M->ModalOpen() && !M->Story->IsCinematic());
 Pack.SetExternal(TEXT("ctx_map_solved"),S47J::Bool(State(TEXT("ch5")),TEXT("p13")));
 // Transformer ctx_* remain false until its actual world controller supplies the results.
 if (!Opening() && !Pack.CurrentScene())
 {
  Pack.SetPhase(K==TEXT("ch5")||K==TEXT("ch6")?TEXT("all_night"):K==TEXT("ch3")||K==TEXT("ch4")?TEXT("field"):K==TEXT("ch2")?TEXT("archive"):M->Story->bResidualVisible?TEXT("signal"):TEXT("early_shift"));
 }
}
void FS47Evening::BeginInvestigation(const S47J::FObj& Saved)
{
 Stop(); Pack.Reset(); RadioPack.Reset(); bEnabled = bInvestigationOnly = true;
 bWorkStarted = true;
 FString Error;
 if (Saved && !Pack.Load(S47J::Obj(Saved, TEXT("pack")), Error))
 {
  bEnabled = false;
  UE_LOG(LogTemp, Error, TEXT("S47: investigation restore: %s"), *Error);
  return;
 }
 // Legacy chapters retain their own clock, vehicle and radio; no synthetic D01 completion.
 Context(); Refile();
 if (Pack.CurrentScene()) Resume();
}

bool FS47Evening::CommitWorldScene(const FString& Id)
{
 if (!bEnabled || !M || !M->Story) return false;
 Context();
 FString Actual = Id;
 if (Id.StartsWith(TEXT("D02_")))
 {
  if (Pack.GetPhase() == TEXT("field")) Actual = TEXT("D02_FIELD_") + Id.Mid(4);
  else if (Pack.GetPhase() == TEXT("all_night")) Actual = TEXT("D02_NIGHT_") + Id.Mid(4);
 }
 const auto* Scene = Pack.FindScene(Actual);
 if (!Scene || !Scene->Choices.IsEmpty() || !Pack.CanStart(Actual) || !Pack.Start(Actual)) return false;
 while (Pack.CurrentLine()) Pack.Advance();
 Changed();
 return true;
}

void FS47Evening::ValidateInvestigationRestore()
{
 if (!bEnabled) return;
 const bool bSheet = M->Office && M->Office->HasFieldSheet();
 Pack.ValidateWorldRestore(M->Office && M->Office->HasKey(), M->Office && M->Office->WasUnlocked(),
  bSheet, bSheet && M->Substation && M->Substation->ReferenceMatched(),
  bSheet && M->Substation && M->Substation->FollowupVerified());
 TArray<FString> Read;
 if (M->Office) for (const FString& Id : FS47Office::LibraryDocumentIds()) if (M->Office->DocumentRead(Id)) Read.Add(Id);
 Pack.ValidateOfficeReadDocuments(Read);
 Context(); Refile();
}

void FS47Evening::Refile()
{
 if (!M->Story) return;
 for (const FString& Id : Pack.Documents()) if (const auto* D = Pack.FindDocument(Id))
 {
  // The physical controller owns the measured card, stamp and reserved photograph.
  if (Id == TEXT("EV_TRAFO_FOLLOWUP")) continue;
  // Restore office credits only after the physical controller has validated this case.
  if (Id.StartsWith(TEXT("EV_DALE_")) && (!M->Office || !M->Office->WasUnlocked())) continue;
  if (FS47Office::LibraryDocumentIds().Contains(Id) && (!M->Office || !M->Office->DocumentRead(Id))) continue;
  // Never replace an already saved paper/photo or its stamp when resuming a case.
  if (M->Story->Docs().ContainsByPredicate([&](const FS47Doc& Doc){return Doc.Id==Id;})) continue;
  FS47Doc Doc; Doc.Id=D->Id; Doc.Title=D->Title; Doc.Kind=D->Kind; Doc.Stamp=D->Stamp;
  Doc.Page=FString::Join(D->Body,TEXT("\n\n")); Doc.Transcript=Doc.Page; M->Story->AddDoc(Doc);
 }
}
void FS47Evening::Changed() { Context(); Refile(); M->RequestAutosave(); }

bool FS47Evening::Trigger(const FString& Scene)
{
 if (!bEnabled) return false;
 Context(); if (!Pack.Start(Scene)) return false;
 Changed(); ShowDialogue(); return true;
}
void FS47Evening::Resume()
{
 bDialogueOpen = false; bResumeRequested=true; SpokenLine.Empty();
 if (const auto* S=Pack.CurrentScene(); S && S->Channel==TEXT("phone")) {StopRing();M->Web->SetHandsetVisible(false);}
}
void FS47Evening::ShowDialogue()
{
 // Repeated use/capture of the current conversation must not replace its panel:
 // UI::Open closes the previous panel and its close callback stops the voice.
 if (bDialogueOpen && M->ModalOpen()) return;
 const auto* S = Pack.CurrentScene(); if (!S || !M->UI) { AfterDialogue(); return; }
 if (!Pack.CurrentLine() && S->Choices.IsEmpty()) { Pack.Advance(); Changed(); AfterDialogue(); return; }
 bDialogueOpen = true;
 TArray<S47Kit::FW> Rows;
 const auto* L=Pack.CurrentLine();
 FString Title=S->Channel==TEXT("phone")?TEXT("TELEPHONE"):S->Channel==TEXT("walkie")?TEXT("WALKIE-TALKIE"):S->Channel==TEXT("radio")?TEXT("RADIO"):TEXT("MESA DINER");
 if (S->Id.StartsWith(TEXT("T02_"))) Title = TEXT("CALICHE JUNCTION / LOCAL FOLLOW-UP");
 if (L)
 {
  if (L->bRecord) Rows.Add(S47Kit::Unread(S47Kit::Sub(L->Speaker)));
  FS47DialogueInput Input; Input.Voice=L->VoiceProfile; Input.Lines.Add({L->Speaker,L->Text});
  const bool Available = L->bRecord && Voice->Available(Input);
  Voice->SetEnabled(M->Audio->bCharacterVoices);
  if (L->bRecord && (!Available || !M->Audio->bCharacterVoices)) Rows.Add(S47Kit::Small(!Available ? TEXT("Recording unavailable / subtitles only") : TEXT("Character voices off")));
  if (SpokenLine!=L->Id) { SpokenLine=L->Id; if (Available) { Voice->Play(Input); VoiceStartedAt=Voice->StartedAt(); } }
  // In a manual conversation the caption begins at measured speech onset, then
  // stays as a readable transcript until Continue. The audio clock freezes in pause.
  const auto Ctx=M->Audio->Ctx;
  const double CaptionAt=VoiceStartedAt+L->SpeechStart;
  const FString Text=L->Text;
  const bool bTimed=Available && M->Audio->bCharacterVoices && Voice->Playing() && L->Duration>0;
  Rows.Add(S47Kit::Unread(S47Kit::Text(TAttribute<FString>::CreateLambda([Ctx,CaptionAt,Text,bTimed]{return bTimed && Ctx && Ctx->CurrentTime()<CaptionAt ? FString() : Text;}),TAttribute<int32>(0))));
  Rows.Add(S47Kit::Button(TEXT("Continue"),[this]{Voice->Stop(); M->UI->Close(true); bDialogueOpen=false; Pack.Advance(); Changed(); ShowDialogue();},true));
 }
 else
 {
  Rows.Add(S47Kit::Sub(TEXT("Your reply")));
  for (const auto* Choice : Pack.Choices())
  {
   const FString Id=Choice->Id;
   Rows.Add(S47Kit::Button(Choice->Label,[this,Id]
   {
    if (!Pack.Select(Id)) return;
    // Choice and pending reply already committed in Pack before requesting disk save.
    M->UI->Close(true); bDialogueOpen=false; Changed(); ShowDialogue();
   },Rows.Num()==1));
  }
 }
 const FString Current=S->Id;
 if (Pack.Flag(TEXT("food_bought")) && !Pack.Flag(TEXT("intro_arrived"))) Rows.Add(S47Kit::Small(Pack.Flag(TEXT("arrival_order_burger"))?TEXT("Burger, fries and coffee / one paid order"):TEXT("Sandwich, chips and coffee / one paid order")));
 // This panel owns its character queue. The generic description reader must
 // neither offer a second Kinski narration nor label paid-order UI as unrecorded.
 M->UI->Open(S47Kit::Shell(M->UI,Title,S47Kit::Unread(S47Kit::Col(Rows)),820,true),[this,Current]{bDialogueOpen=false; SpokenLine.Empty(); Voice->Stop(); Changed();});
}
void FS47Evening::AfterDialogue()
{
 bDialogueOpen=false; SpokenLine.Empty(); if (Voice) Voice->Stop();
 if (Pack.Flag(TEXT("handover_done")) && !bWorkStarted)
 {
  bWorkStarted=true; StopRing(); M->Web->SetHandsetVisible(true);
  M->Audio->Play(TEXT("handset_down"),0.5f,M->Web->PropLocation(TEXT("phone")));
  M->Story->BeginEveningShift(); Refile(); Context();
  // The handover's archived day copy becomes available now, just as on Continue.
  if (M->Anomaly) M->Anomaly->Changed();
  M->UI->Objective(ServicePending() ? ServiceObjective() : FString(TEXT("CONTROL ROOM // POWER THE RACK AND START THE ROUTINE SURVEY"))); Changed();
 }
 else if (Commute()) M->UI->Objective(TEXT("EVENING SHIFT // RETURN TO YOUR CAR, THEN DRIVE NORTH TO SARO"));
}
void FS47Evening::Answer()
{
 if (!Opening() || Commute()) return;
 StopRing(); M->Audio->Play(TEXT("handset_up"),0.5f,M->Web->PropLocation(TEXT("phone")));
 M->Web->SetHandsetVisible(false);
 if (Pack.CurrentScene()) ShowDialogue(); else Trigger(TEXT("D01_HANDOVER"));
}
bool FS47Evening::Use(const FString& Id)
{
 if (!bEnabled) return false;
 if (MealBusy()) return true;
 if (Id == TEXT("phone") && FeederOutage() && !bFeederCallAnswered) { AnswerFeederCall(); return true; }
 if (Id == TEXT("logbook") && M->Substation) M->Substation->TryFileFollowup();
 if (Takeaway->Use(Id)) return true;
 if ((ServicePending() || (NightSequence() && bFeederRestored && !bServiceFiled)) && Id == TEXT("logbook") && AtShiftFolder() && M->Substation &&
     M->Substation->HasNormalResult() && Pack.Flag(TEXT("trafo_normal_checked")) && !Pack.CurrentScene())
 {
  // One physical filing in the existing folder. Its normal comparison view still opens below.
  bServiceFiled = true;
  M->Story->Note(FString::Printf(TEXT("%s. Back at SARO, filed the Caliche Junction inspection card. The local panel and marked lamp agreed at %s. This records that circuit, not the town supply or SARO K3."),
   *M->Story->ClockText(false), *M->Substation->NormalCompletionStamp()));
  M->UI->Toast(NightSequence() ? TEXT("Feeder restoration record filed. The unlogged residual can now be tracked.") : TEXT("Local inspection card filed. The receiver calibration and overnight survey can continue."), 4);
  M->UI->Objective(NightSequence() ? TEXT("CONTROL ROOM // TRACK THE UNLOGGED RESIDUAL NEAR 1420.4 MHz") : FiledRoutineObjective(*M->Story));
  Changed();
  // Do not consume this use: the existing shift-folder/N01 panel is the only panel opened.
 }
 if (Id==TEXT("evening:radio") && Commute()) {bRadioOn=!bRadioOn;M->UI->Toast(bRadioOn?TEXT("Local evening radio on"):TEXT("Radio off"),2); Changed();return true;}
 // Physical Operations access is available during arrival too. The Office owns
 // range, lock, drawer and document checks; the phone remains the opening goal.
 if (Id.StartsWith(TEXT("office:")) || Id==TEXT("officeDoor")) return false;
 if (Opening())
 {
  if (!Commute() && Id==TEXT("phone")) { Answer(); return true; }
  if (Commute() && (Id.Contains(TEXT("waitress")) || Id.Contains(TEXT("counter")) || Id.Contains(TEXT("menu"))))
  {
   if (Pack.CurrentScene()) ShowDialogue();
   else if (!Trigger(TEXT("I02_DINER_HELLO"))) M->UI->Toast(TEXT("Your order is settled. Have a good shift."),3);
   return true;
  }
  if (Commute() && (Id.Contains(TEXT("truck"),ESearchCase::IgnoreCase)))
  {
   if (Pack.CurrentScene()) ShowDialogue();
   else M->World->DriveBack();
   return true;
  }
  if (Id.Contains(TEXT("Door"),ESearchCase::IgnoreCase) || Id==TEXT("east") || Id==TEXT("south") || Id==TEXT("exit")) return false;
  M->UI->Toast(Commute()?TEXT("Drive north to SARO when you are ready. You can order supper at the diner counter."):TEXT("Dale is calling the control-room phone."),4);
  return true;
 }
 return false;
}
void FS47Evening::Parked(const FString& Place)
{
 if (!Commute()) return;
 if (Place==TEXT("saro") && M->World->TruckOnLot())
 {
  Pack.SetPhase(TEXT("handover")); if (!Pack.Start(TEXT("I11_SARO_ARRIVAL"))) return;
  Pack.Advance(); // empty world-triggered node commits arrival once
  M->Story->SetClock(-21600);
  if (const auto* T=M->World->TruckHere()) Visual->ParkPersonal(T->Position,T->Rotation.Y);
  M->World->FinishCommute(); Visual->SetParked(true);
  M->UI->Toast(Pack.String(TEXT("ARRIVAL_SHIFT_CARD")),5);
  M->UI->Objective(TEXT("SARO // ANSWER THE CONTROL-ROOM PHONE")); RingAt=M->T;
  Changed();
 }
 else Changed();
}

void FS47Evening::RadioStep(double Dt)
{
 if (Commute()) { if (auto* T=M->World->CabTruck()) T->SetRadio(S47Radio::DialX(1420),bRadioOn); }
 const bool Audible=Commute() && M->World->bDriving && bRadioOn && !M->ModalOpen();
 if (RadioVoice->Enabled()!=M->Audio->bCharacterVoices)
 {
  RadioVoice->SetEnabled(M->Audio->bCharacterVoices); RadioSpokenLine.Empty(); bRadioRecorded=false;
 }
 // The toast can expire while the receiver is paused. Republish the same line
 // on return, retaining its unread duration and cursor rather than skipping it.
 if (!Audible)
 {
  if (RadioCaptionAlive) *RadioCaptionAlive=false;
  if (RadioVoice->Playing() || RadioVoice->Loading()) RadioVoice->Stop();
  RadioSpokenLine.Empty(); bRadioRecorded=false; bRadioCaptionShown=false;
 }
 const auto* S=RadioPack.CurrentScene();
 if (Audible && !S && RadioRest<=0)
 {
  RadioPack.Start(RadioPack.Flag(TEXT("arrival_radio_heard"))?TEXT("I10_BACK_ON_ROAD"):TEXT("I01_DRIVE"));
  S=RadioPack.CurrentScene();
 }
 if (Audible)
 {
  if (S)
  {
   if (const auto* L=RadioPack.CurrentLine())
   {
    if (RadioSpokenLine!=L->Id)
    {
     if (RadioCaptionAlive) *RadioCaptionAlive=false;
     RadioCaptionAlive=MakeShared<bool>(true); bRadioCaptionShown=false;
     RadioSpokenLine=L->Id;
     FS47DialogueInput Input; Input.Voice=L->VoiceProfile; Input.Lines.Add({L->Speaker,L->Text});
     const auto Found=L->bRecord?RadioVoice->Bank->Resolve(L->Text,0,L->VoiceProfile):TOptional<TArray<FS47RadioClip>>();
     const bool Available=Found.IsSet();
     bRadioRecorded=false;
     if (Available && M->Audio->bCharacterVoices)
     {
      // Restart the current recorded line after an interruption; never skip its cursor.
      RadioVoice->Play(Input);
      RadioStartedAt=RadioVoice->StartedAt();
      const FS47DialogueStatus ClipStatus=RadioVoice->Status();
      if (ClipStatus.Clips.Num()==1 && ClipStatus.Clips[0].Duration>0 && RadioVoice->Playing())
      {
       RadioHold=ClipStatus.Clips[0].Duration; bRadioRecorded=true; // actual decoded PCM duration
      }
     }
     if (!bRadioRecorded)
     {
      // A muted existing take uses its measured catalog duration. The pending silent
      // placeholder uses a readability hold, which is never reported as take timing.
      if (Available && !M->Audio->bCharacterVoices) RadioHold=(*Found)[0].Duration;
      else if (RadioHold<=0) RadioHold=FMath::Clamp(2.5+L->Text.Len()/20.0,4.0,11.0);
     }
     if (!bRadioRecorded)
     {
      const FString CaptionState=Available?(M->Audio->bCharacterVoices?TEXT("recording unavailable / subtitles only"):TEXT("character voices off")):TEXT("recording unavailable / subtitles only");
      M->UI->Toast(TEXT("RADIO: LOCAL EVENING HOST [")+CaptionState+TEXT("] / ")+L->Text,float(RadioHold));
      bRadioCaptionShown=true;
     }
    }
    if (bRadioRecorded && !bRadioCaptionShown && M->Audio->Ctx->CurrentTime()>=RadioStartedAt+L->SpeechStart)
    {
     const auto Ctx=M->Audio->Ctx; const auto Alive=RadioCaptionAlive;
     const double End=RadioStartedAt+(L->SpeechEnd>0?L->SpeechEnd:L->Duration);
     M->UI->Toast(TEXT("RADIO: LOCAL EVENING HOST / ")+L->Text,float(FMath::Max(0.01,End-Ctx->CurrentTime())),[Ctx,Alive]{return *Alive?Ctx->CurrentTime():1e20;});
     bRadioCaptionShown=true;
    }
    RadioHold-=Dt;
    const bool Finished=bRadioRecorded?(!RadioVoice->Playing() && !RadioVoice->Loading()):RadioHold<=0;
    if (Finished) { RadioVoice->Stop(); bRadioRecorded=false; RadioPack.Advance(); RadioSpokenLine.Empty(); if (!RadioPack.CurrentScene()) RadioRest=45; Changed(); }
   }
   else { RadioPack.Advance(); RadioRest=45; Changed(); }
  }
  else RadioRest=FMath::Max(0.0,RadioRest-Dt);
 }
 // The same licensed track the existing radio uses for its cheerful two-step.
 if (!Music && Commute() && M->Audio->Ctx)
 {
  Music=FS47MediaSource::Open(M->Audio->Ctx.ToSharedRef(),TEXT("radio/bama_country.mp3"));
  if (Music)
  {
   MusicGain=M->Audio->Ctx->CreateGain(); MusicGain->Gain.SetValue(0.23);
   Music->Connect(MusicGain); MusicGain->Connect(M->Audio->RadioBus);
   Music->Seek(MusicPosition); Music->Play();
  }
 }
 if (Music)
 {
  const bool Quiet=!Audible || RadioPack.CurrentScene();
  Music->Pause(Quiet);
  if (!Quiet && Music->CurrentTime()>=Music->Duration()-0.2) Music->Seek(0);
  MusicPosition=Music->CurrentTime();
 }
 if (!Commute() && Music) { Music->Stop(); Music->Disconnect(); Music.Reset(); MusicGain->Disconnect(); MusicGain.Reset(); }
}
void FS47Evening::Step(double Dt)
{
 if (!bEnabled || M->Mode!=US47Main::EMode::Play) return;
 if (bInvestigationOnly)
 {
  Context();
  if (bResumeRequested && !M->ModalOpen() && !M->World->bBusy) { bResumeRequested=false; ShowDialogue(); }
  return;
 }
 if (!bBindings && M->Inter->Get(TEXT("diner:waitress")))
 {
  bBindings=true;
  for (const FString& Id : {FString(TEXT("diner:waitress")),FString(TEXT("diner:counter")),FString(TEXT("diner:menu")),FString(TEXT("dinerTruck"))})
  {
   if (auto* I=M->Inter->Get(Id))
   {
    auto Label=I->Label; auto Examine=I->Examine;
    I->Label=[this,Id,Label]{return Commute()?FString(Id==TEXT("dinerTruck")?TEXT("Drive your car to SARO"):TEXT("Order supper / talk to the waitress")):Label?Label():FString();};
    I->Examine=[this,Id,Examine]{return Commute()?FString(Id==TEXT("dinerTruck")?TEXT("Your car. SARO 07 is waiting at work."):TEXT("Supper is being served. Coffee is fresh.")):Examine?Examine():FString();};
   }
  }
 }
 if (bResumeRequested && !M->ModalOpen() && !M->World->bBusy) { bResumeRequested=false; ShowDialogue(); }

 Context(); NightStep();
 // SC02's active meeting at Caliche owns the objective line until contact is broken.
 if (M->Horror && M->Horror->SC02Meeting()) {}
 else if (ServicePending() && !M->World->bDriving) M->UI->Objective(ServiceObjective());
 else if (NightSequence() && bFeederRestored && M->World->Area == TEXT("saro") && !M->World->bDriving &&
          (!M->Horror || !M->Horror->HasTasks()) && M->Story->Phase() == ES47Phase::Residual)
  M->UI->Objective(TEXT("CONTROL ROOM // TRACK THE UNLOGGED RESIDUAL NEAR 1420.4 MHz"));
 else if (bEnabled && bServiceRoute && bServiceFiled && M->Story->bEveningTimeline && !M->World->bDriving)
 {
  // Continue must restore the next receiver action as well as the filed evidence.
  if (M->Story->Phase() == ES47Phase::Shift) M->UI->Objective(FiledRoutineObjective(*M->Story));
  else if (M->Story->Phase() == ES47Phase::Survey) M->UI->Objective(FString());
 }
 if (Opening() && !Commute() && !M->ModalOpen() && !M->bPausedByMenu) M->Story->SetClock(S47Timeline::AdvanceStoryClock(M->Story->Clock(),Dt,true));
 if (Opening() && !Commute() && !bRing && !Pack.CurrentScene() && M->T>=RingAt)
 {
  bRing=true; M->Audio->Loop(TEXT("evening:ring"),TEXT("phone_ring"),0.8f,M->Web->PropLocation(TEXT("phone")));
  M->Web->SetHandsetRattle(true);
 }
 if (bRing) M->Web->SetHandsetRattle(true);
 RadioStep(Dt);
 FS47Truck* Carrier=M->World->CabTruck(); if (!Carrier) Carrier=M->World->TruckHere();
 if(Commute())
 {
  if(M->World->Area==TEXT("saro") && Carrier && Carrier->Position.Z<120)
   M->UI->Objective(TEXT("SARO // STOP ANYWHERE IN THE VISITOR PARKING AREA, THEN USE THE ENTRANCE BY THE RAMP."));
  else M->UI->Objective(M->World->bDriving?TEXT("EVENING SHIFT // DRIVE NORTH TO SARO. STOP AT MESA DINER IF YOU FEEL LIKE SUPPER."):TEXT("EVENING SHIFT // RETURN TO YOUR CAR, THEN DRIVE NORTH TO SARO"));
 }
 Visual->Tick(Commute(),M->World->bDriving,Carrier,Pack.Flag(TEXT("intro_arrived")),Pack.Flag(TEXT("food_bought")),Pack.Flag(TEXT("food_taken")),Pack.Flag(TEXT("food_eaten")),Pack.CurrentScene() && Pack.CurrentScene()->Id==TEXT("I08_WATER"),Pack.Flag(TEXT("arrival_order_burger")));
 if (!TestMode.IsEmpty() && !M->World->bBusy) { const FString Mode=TestMode; TestMode.Empty(); BootstrapTest(Mode); }
 if (bDialogueOpen && !M->ModalOpen()) bDialogueOpen=false;
 ControlTest(Dt);
 Takeaway->Step(Dt);
 ControlEntranceTest(Dt);
 ControlTakeawayTest(Dt);
 ControlNightSequenceTest(Dt);
}
S47J::FObj FS47Evening::Save() const
{
 auto O=S47J::New(); S47J::Set(O,TEXT("takeaway"),Takeaway->Save()); S47J::Set(O,TEXT("pack"),Pack.Save()); S47J::Set(O,TEXT("radioPack"),RadioPack.Save());
 S47J::Set(O,TEXT("arrivalParkingVersion"),2);
 S47J::Set(O,TEXT("investigationOnly"),bInvestigationOnly);
 S47J::Set(O,TEXT("serviceRouteVersion"),bNightSequence ? 2 : bServiceRoute ? 1 : 0);
 S47J::Set(O,TEXT("feederFailed"),bFeederFailed); S47J::Set(O,TEXT("feederRestored"),bFeederRestored);
 S47J::Set(O,TEXT("feederCallAnswered"),bFeederCallAnswered); S47J::Set(O,TEXT("serviceDriveSkipped"),bServiceDriveSkipped);
 S47J::Set(O,TEXT("serviceFiled"),bServiceRoute && bServiceFiled);
 S47J::Set(O,TEXT("clock"),M->Story->Clock()); S47J::Set(O,TEXT("area"),M->World->Area);
 S47J::Set(O,TEXT("radioOn"),bRadioOn); S47J::Set(O,TEXT("radioHold"),RadioHold); S47J::Set(O,TEXT("radioRest"),RadioRest);
 S47J::Set(O,TEXT("musicPosition"),Music?Music->CurrentTime():MusicPosition);
 if (Pack.Flag(TEXT("intro_arrived"))) {auto P=S47J::New(); const FVector V=Visual->ParkedPosition(); S47J::Set(P,TEXT("x"),V.X);S47J::Set(P,TEXT("y"),V.Y);S47J::Set(P,TEXT("z"),V.Z);S47J::Set(P,TEXT("h"),Visual->ParkedHeading());S47J::Set(O,TEXT("personalPark"),P);}
 const auto* T=M->World->CabTruck(); if (!T) T=M->World->TruckHere();
 if (Commute() && T)
 {
  auto C=S47J::New(); S47J::Set(C,TEXT("driving"),M->World->bDriving);
  S47J::Set(C,TEXT("leg"),M->World->bDriving?M->World->Leg:M->World->Area);
  S47J::Set(C,TEXT("at"),M->World->TruckAt);
  S47J::Set(C,TEXT("x"),T->Position.X); S47J::Set(C,TEXT("y"),T->Position.Y); S47J::Set(C,TEXT("z"),T->Position.Z); S47J::Set(C,TEXT("h"),T->Rotation.Y);
  S47J::Set(O,TEXT("car"),C);
 }
 return O;
}

void FS47Evening::BootstrapTest(const FString& Mode)
{
 // Deterministic control-scenario setup uses the same state transactions as the visible UI.
 // Drive tests still use real World controller/parking; shortcut cameras are labelled setups.
 if (M->World->bBusy) {TestMode=Mode;return;}
 if (Mode==TEXT("commute")) return;
 auto Lines=[this]{while(Pack.CurrentLine()) Pack.Advance();};
 if (Mode==TEXT("diner") || Mode==TEXT("order") || Mode==TEXT("paid") || Mode==TEXT("burger") || Mode==TEXT("sandwich") || Mode==TEXT("water"))
 {
  M->World->StopDriving(); M->World->bCommuteVehicle=true; M->World->PlaceTruck(TEXT("diner"));
  M->World->Enter(TEXT("diner")); M->World->PlaceAtDiner(TEXT("stool"));
  Pack.Start(TEXT("I02_DINER_HELLO"));
  if (Mode!=TEXT("diner"))
  {
   Lines(); Pack.Select(Mode==TEXT("water")?TEXT("I02_WATER"):TEXT("I02_MENU"));
   if (Mode!=TEXT("water"))
   {
    Lines(); Pack.Select(Mode==TEXT("sandwich")?TEXT("I03_SANDWICH"):TEXT("I03_BURGER"));
    if (Mode==TEXT("burger") || Mode==TEXT("sandwich")) {Lines();Pack.Select(Mode==TEXT("sandwich")?TEXT("I05_EAT"):TEXT("I04_TAKE"));}
   }
  }
  Changed(); ShowDialogue(); return;
 }
 if (Mode==TEXT("parking") || Mode==TEXT("phone") || Mode==TEXT("dusk"))
 {
  M->World->StopDriving(); M->World->bCommuteVehicle=true;
  auto Car=S47J::New();const FVector Bay=FS47EveningVisual::ArrivalPark();
  S47J::Set(Car,TEXT("x"),Bay.X);S47J::Set(Car,TEXT("y"),Bay.Y);S47J::Set(Car,TEXT("z"),Bay.Z);S47J::Set(Car,TEXT("h"),0.0);S47J::Set(Car,TEXT("at"),TEXT("saro"));
  M->World->PlaceTruck(TEXT("saro"),Car); M->World->Enter(TEXT("saro"));
  Parked(TEXT("saro"));
  const FVector Spawn=M->Web->SpawnThree(); M->Player->Place(Spawn.X,Spawn.Z,Spawn.Y);
  if (Mode==TEXT("phone")) Answer();
  if (Mode==TEXT("dusk")) {M->Story->SetClock(-5*3600-25*60);M->Player->Place(2,20,0);}
 }
}
bool FS47Evening::SelfTest(TArray<FString>& Lines)
{
 FString Error; const bool Ok=FS47EveningPack::SelfTest(Error);
 Lines.Add((Ok?FString(TEXT("PASS")):FString(TEXT("FAIL")))+TEXT(" EveningShift pack runtime control scenarios: ")+Error);
 bool Valid=Ok;
 auto Check=[&](bool Good,const TCHAR* What){Valid&=Good;Lines.Add(FString(Good?TEXT("PASS evening vehicles: "):TEXT("FAIL evening vehicles: "))+What);};
 FS47EveningVisual V(nullptr);
 const FVector Bay=FS47EveningVisual::ArrivalPark(), Moved(-12,-0.62,14);
 V.SetParked(false);V.SetParked(true);Check(V.ParkedPosition().Equals(Bay),TEXT("missing old visitor pose restores the clear visitor bay"));
 V.ParkPersonal(Bay,0);Check(V.ParkedPosition().Equals(Bay),TEXT("visitor bay is outside entrance, annex and service pad"));
 V.ParkPersonal(Moved,UE_DOUBLE_PI);Check(V.ParkedPosition().Equals(Moved),TEXT("moving the car to clear ground preserves its pose"));
 for (const FVector& P : {FVector(2,0,8),FVector(8.9,0,8),FVector(8,0,1),FVector(4,0,4)})
 {
  V.ParkPersonal(P,0.7);Check(V.ParkedPosition().Equals(Bay) && FMath::IsNearlyZero(V.ParkedHeading()),TEXT("unsafe old parking pose restores the clear visitor bay"));
 }
 for(double X:{-15.8,-12.8,-9.8})for(double H:{0.0,UE_DOUBLE_PI})
  Check(FS47EveningVisual::FitsArrivalArea(FVector(X,-.6,14.3),H),TEXT("all three visitor bays accept forward and reverse parking without food"));
 for(double X:{-18.2,-16.8,-14.3,-11.3,-7.8})for(double Z:{10.5,14.3,18.8,20.5})for(double H:{0.0,.7,UE_DOUBLE_PI/2,UE_DOUBLE_PI})
 {
  const FVector P(X,-.6,Z);V.ParkPersonal(P,H);V.RestorePersonal(V.ParkedPosition(),V.ParkedHeading(),2);
  Check(FS47EveningVisual::FitsArrivalArea(P,H) && V.ParkedPosition().Equals(P) && FMath::IsNearlyEqual(V.ParkedHeading(),H),TEXT("whole visitor area including paint, edges and transverse headings completes arrival and retains saved pose"));
 }
 for(const FVector& P:{FVector(-18.201,-.6,15),FVector(-7.799,-.6,15),FVector(-13,-.6,10.499),FVector(-13,-.6,20.501),FVector(8.9,0,8),FVector(-22.2,-.6,15),FVector(-9,-.3,5.7)})
  Check(!FS47EveningVisual::FitsArrivalArea(P,0),TEXT("outside visitor area, highway, service pad and entrance ramp do not complete arrival"));
 V.ParkPersonal(FVector(-14.3,-.6,18.5),UE_DOUBLE_PI/2);
 Check(V.ParkedPosition().Equals(FVector(-14.3,-.6,18.5)) && FMath::IsNearlyEqual(V.ParkedHeading(),UE_DOUBLE_PI/2),TEXT("cross-line transverse arrival retains actual car pose"));
 V.RestorePersonal(FVector(-14.3,-.6,18.5),UE_DOUBLE_PI/2,2);
 Check(V.ParkedPosition().Equals(FVector(-14.3,-.6,18.5)) && FMath::IsNearlyEqual(V.ParkedHeading(),UE_DOUBLE_PI/2),TEXT("cross-line transverse arrival survives current save restore"));
 V.RestorePersonal(FVector(4.5,-.62,14),UE_DOUBLE_PI,0);Check(V.ParkedPosition().Equals(Bay)&&FMath::IsNearlyZero(V.ParkedHeading()),TEXT("legacy fallback save migrates to marked visitor parking"));
 V.RestorePersonal(Moved,.2,2);Check(V.ParkedPosition().Equals(Moved)&&FMath::IsNearlyEqual(V.ParkedHeading(),.2),TEXT("current safe custom pose survives restore"));
 FS47Truck T;T.SetPose(FVector(9,0.2,15),FVector(0.25,0.7,-0.18));T.SetParkPose(FVector(8.9,0,8),UE_DOUBLE_PI);
 Check(T.Position.Equals(FVector(8.9,0,8)) && T.Rotation.Equals(FVector(0,UE_DOUBLE_PI,0)),TEXT("canonical service parking drops inherited ramp pitch and roll"));
 FS47EveningPack Supper;
 FString SupperError;
 const auto MealCheck=[&](bool Good,const TCHAR* What){Valid&=Good;Lines.Add(FString(Good?TEXT("PASS takeaway transaction: "):TEXT("FAIL takeaway transaction: "))+What);};
 MealCheck(Supper.Initialize(SupperError) && !Supper.EatTakeaway(),TEXT("no unpaid meal can be consumed"));
 Supper.Start(TEXT("I02_DINER_HELLO")); while(Supper.CurrentLine()) Supper.Advance(); Supper.Select(TEXT("I02_MENU"));
 while(Supper.CurrentLine()) Supper.Advance(); Supper.Select(TEXT("I03_BURGER"));
 MealCheck(Supper.Flag(TEXT("food_bought")) && !Supper.EatTakeaway() && Supper.GetPendingChoice()==TEXT("I03_BURGER"),TEXT("paid pending order cannot be consumed before handover"));
 while(Supper.CurrentLine()) Supper.Advance(); Supper.Select(TEXT("I04_TAKE")); while(Supper.CurrentLine()) Supper.Advance();
 MealCheck(Supper.Flag(TEXT("food_taken")) && Supper.EatTakeaway() && Supper.Flag(TEXT("food_eaten")) && !Supper.EatTakeaway(),TEXT("the one takeaway portion is consumed exactly once"));
 const auto SupperSaved=Supper.Save(); FS47EveningPack SupperRestored; SupperRestored.Initialize(SupperError);
 MealCheck(SupperRestored.Load(SupperSaved,SupperError) && SupperRestored.Flag(TEXT("food_taken")) && SupperRestored.Flag(TEXT("food_eaten")) && !SupperRestored.EatTakeaway(),TEXT("save restore cannot create a second serving"));
 return FS47Takeaway::SelfTest(Lines) && Valid;
}

bool FS47Evening::PhysicalSpeech(FString& Clip, double& Seconds) const
{
 Clip.Reset(); Seconds = 0;
 const FS47EveningScene* S = bEnabled ? Pack.CurrentScene() : nullptr;
 const FS47EveningLine* L = S ? Pack.CurrentLine() : nullptr;
 if (!S || S->Channel != TEXT("physical") || !L || !L->bRecord || L->VoiceProfile != TEXT("waitress") || !Voice) return false;
 return Voice->ActiveClip(Clip, Seconds);
}
