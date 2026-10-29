#include "S47Horror.h"
#include "S47HorrorVisual.h"
#include "S47HorrorThreat.h"
#include "S47HorrorSC03.h"
#include "S47Main.h"
#include "S47Evening.h"
#include "S47Timeline.h"
#include "S47Story.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Web.h"
#include "S47FieldCamera.h"
#include "S47Flow.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Kit.h"
#include "S47Chapter.h"
#include "S47RecordedDialogue.h"
#include "S47RadioVoice.h"
#include "S47Album.h"
#include "S47Night.h"
#include "S47Substation.h"
#include "S47Observing.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
 bool ExactVersion(const S47J::FObj& O,const TCHAR* Key,int32 Min,int32 Max)
 {
  const auto* V=O?O->Values.Find(Key):nullptr;
  if(!V || !V->IsValid() || (*V)->Type!=EJson::Number)return false;
  const double N=(*V)->AsNumber();return FMath::IsFinite(N) && N>=Min && N<=Max && FMath::FloorToDouble(N)==N;
 }
 // A saved SC02 flag is only a JSON boolean; strings and numbers never become facts.
 bool SC02Bool(const S47J::FObj& O,const TCHAR* Key,bool& Out)
 {
  const auto* V=O?O->Values.Find(Key):nullptr;
  if(!V)return true; // absent: false
  if(!V->IsValid() || (*V)->Type!=EJson::Boolean)return false;
  Out=(*V)->AsBool();return true;
 }
 void FocusPrimary(const TSharedPtr<SWidget>& Primary)
 {
  if(!Primary || !Primary->SupportsKeyboardFocus() || !FSlateApplication::IsInitialized())return;
  FSlateApplication::Get().SetAllUserFocus(Primary,EFocusCause::SetDirectly);
  // A line transition can run inside the previous button's key reply. Apply
  // focus again after that reply has finished restoring the viewport's focus.
  const TWeakPtr<SWidget> Weak=Primary;
  FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak](float)
  {if(const auto P=Weak.Pin();P && FSlateApplication::IsInitialized())FSlateApplication::Get().SetAllUserFocus(P,EFocusCause::SetDirectly);return false;}));
 }
}

FS47Horror::FS47Horror(US47Main* InM) : M(InM)
{
 FString Error;
 if (!Pack.Initialize(Error)) UE_LOG(LogTemp,Error,TEXT("S47: Horror pack initialization failed: %s"),*Error);
 Visual=MakeUnique<FS47HorrorVisual>(M);
 Threat=MakeUnique<FS47HorrorThreat>(M);
 SC03=MakeUnique<FS47HorrorSC03>(M,this);
 Voice=MakeShared<FS47RecordedDialogue>(M->Audio,TEXT("horror"),
  [](const FS47DialogueInput& I,FS47RadioVoice& Bank)->TOptional<TArray<FS47RadioClip>>
  { return I.Lines.Num()==1 ? Bank.Resolve(I.Lines[0].Text,0,I.Voice) : TOptional<TArray<FS47RadioClip>>(); },
  [this](const FS47RadioClip&){const auto* S=Pack.CurrentScene();return bWorldCall || (S && S->Channel!=TEXT("physical") && S->Channel!=TEXT("ui"));},
  [this](const FS47RadioClip&){FS47RecordedDialogue::FOutput O;O.Channel=bWorldCall?TEXT("phone"):Pack.CurrentScene()?Pack.CurrentScene()->Channel:TEXT("physical");return O;});
}
FS47Horror::~FS47Horror() { if(Voice) Voice->Stop(); }
void FS47Horror::Stop()
{
 bEnabled=false;bDialogueOpen=false;bWorldCall=false;bResumeRequested=false;Spoken.Empty();
 if(Voice) Voice->Stop();
 if(Visual) Visual->Stop();
 if(Threat) Threat->Stop();
 if(SC03) SC03->Begin(nullptr,false);
 CallScene=TEXT("SC01_FIRST_REPORT");CallQueue.Reset();
}
void FS47Horror::Begin(const S47J::FObj& Saved,bool bNewCase)
{
 Stop();Pack.Reset();bEast=bCorridor=bBaseline=bReport=bComplete=bReferenceHeard=false;CallCursor=0;ReportStamp.Empty();ReferencePhoto.Empty();Revision=0;
 SC02=ESC02::Off;bThreatSeen=bRetry=bH02Attempt=bAttemptPending=bCheckpointPending=false; // rebuilt by BeginSC02
 if(!Saved && !bNewCase) {Visual->Begin(nullptr,false);return;}
 if(Saved && (!ExactVersion(Saved,TEXT("schemaVersion"),1,1) || !ExactVersion(Saved,TEXT("contentRevision"),1,InstalledRevision)))
 {UE_LOG(LogTemp,Warning,TEXT("S47: unsupported horror route revision; retained old case flow"));Visual->Begin(nullptr,false);return;}
 bEnabled=true;Revision=Saved?S47J::Int(Saved,TEXT("contentRevision")):InstalledRevision;
 Visual->Begin(S47J::Obj(Saved,TEXT("visual")),true);
 if(!Visual->Enabled() || !Visual->Ready())
 {bEnabled=false;Pack.Reset();Visual->Stop();UE_LOG(LogTemp,Warning,TEXT("S47: invalid or unavailable SC01 world; existing case route retained without new gates"));return;}
 if(Saved)
 {
  bEast=S47J::Bool(Saved,TEXT("eastTried"));bCorridor=S47J::Bool(Saved,TEXT("corridorTried"));bBaseline=S47J::Bool(Saved,TEXT("baseline"));
  const FString PhotoId=S47J::Str(Saved,TEXT("referencePhoto"));
  if(M->Night && M->Night->Album && M->Night->Album->Shots().ContainsByPredicate([&](const FS47AlbumShot& Shot){return Shot.Id==PhotoId && !Shot.bUnavailable && Shot.Url.StartsWith(TEXT("data:image/")) && Shot.Targets.ContainsByPredicate([](const TPair<FString,FVector2D>& T){return T.Key==TEXT("horror:normalLamp");});})) ReferencePhoto=PhotoId;
  if(!PhotoId.IsEmpty() && ReferencePhoto.IsEmpty())
  {bEnabled=false;Pack.Reset();Visual->Stop();UE_LOG(LogTemp,Warning,TEXT("S47: missing SC01 original photo; existing case progression retained without new gates"));return;}
  // Unreleased older cohorts could spend their whole instant-back roll before
  // this mandatory exposure. Keep every print and their existing case route.
  if(ReferencePhoto.IsEmpty() && M->Night && M->Night->Album && M->Night->Album->Left()==0)
  {bEnabled=false;Pack.Reset();Visual->Stop();UE_LOG(LogTemp,Warning,TEXT("S47: exhausted earlier SC01 roll without reference; existing case progression retained without new gates"));return;}
  bReferenceHeard=S47J::Bool(Saved,TEXT("daleReferenceHeard"));ReportStamp=S47J::Str(Saved,TEXT("reportStamp"));
  bReport=S47J::Bool(Saved,TEXT("reportAcknowledged")) && Visual->HandSeen() && !ReportStamp.IsEmpty();
  bWorldCall=S47J::Bool(Saved,TEXT("callActive")) && (S47J::Str(Saved,TEXT("callScene")).StartsWith(TEXT("SC03_")) || (!bReport && Visual->HandSeen()));
  CallScene=S47J::Str(Saved,TEXT("callScene"));if(CallScene.IsEmpty() || !Pack.FindScene(CallScene))CallScene=TEXT("SC01_FIRST_REPORT");
  CallQueue.Reset();if(const auto* Q=Saved->Values.Find(TEXT("callQueue"));Q && Q->IsValid() && (*Q)->Type==EJson::Array)for(const auto& V:(*Q)->AsArray())if(V.IsValid() && V->Type==EJson::String && Pack.FindScene(V->AsString()))CallQueue.Add(V->AsString());
  const auto* CallLines=Pack.FindScene(CallScene);
  CallCursor=FMath::Clamp(S47J::Int(Saved,TEXT("callCursor")),0,CallLines?FMath::Max(0,CallLines->Lines.Num()-1):1);
  // Physical facts must be restored before the pack validates a pending result.
  Context();FString Error;
  if(!Pack.Load(S47J::Obj(Saved,TEXT("pack")),Error))
  {Pack.Reset();UE_LOG(LogTemp,Warning,TEXT("S47: horror dialogue restore rejected: %s"),*Error);}
 }
 // Completion reads restored facts only; Context/Refile below do not depend on it.
 bComplete=S47J::Bool(Saved,TEXT("sc01Complete")) && NormalComplete() && Visual->MarksChecked() && (!Visual->HandSeen() || Pack.Flag(TEXT("hr_sc01_window_check_presented")));
 BeginSC02(Saved);
 SC03->Begin(S47J::Obj(Saved,TEXT("sc03")),Revision>=3 && SC02==ESC02::Escaped);SC03Seen=SC03->Changes();
 if(bWorldCall && CallScene.StartsWith(TEXT("SC03_")) && !SC03->Live()){bWorldCall=false;CallQueue.Reset();CallScene=TEXT("SC01_FIRST_REPORT");}
 Context();Refile();
 bResumeRequested=bWorldCall || Pack.CurrentScene()!=nullptr;
 if(bComplete)Visual->ReleaseLighting();
 UE_LOG(LogTemp,Display,TEXT("S47: horror route revision %d %s, SC01 %s, SC02 %s"),Revision,Saved?TEXT("restored"):TEXT("new"),bComplete?TEXT("complete"):TEXT("active"),PhaseName(SC02));
}
bool FS47Horror::NormalComplete() const
{
 // A new evening's schedule is part of the normal work: the window is not armed before 23:00.
 return bEnabled && M && M->FieldCamera && M->FieldCamera->bHave && !ReferencePhoto.IsEmpty() && bEast && bCorridor && bBaseline &&
  Visual && Visual->LampChangeChecked() && Visual->ScreenChecked() && !(M->Observing && M->Observing->Pending());
}
// The SC02 meeting holds the night's older events (abduction, truck taking) while the player is at the site.
bool FS47Horror::Busy() const {return bEnabled && Visual && (Visual->WindowPhase()==1 || bWorldCall || Pack.CurrentScene() || (SC02==ESC02::Encounter && AtSite()) || (SC03 && SC03->Busy()));}
bool FS47Horror::DoorsSecured() const
{
 if(!M || !M->Web)return false;
 const auto Doors=M->Web->DoorStates();return bEast && bCorridor && !Doors.FindRef(TEXT("east")) && !Doors.FindRef(TEXT("exit"));
}
void FS47Horror::Context()
{
 Pack.ClearExternal();
 if(!bEnabled || !M || !M->Evening || !M->Story) return;
 const bool Ready=M->Evening->Pack.Flag(TEXT("handover_done"));
 Pack.SetExternal(TEXT("ctx_hr_h01_ready"),Ready);
 Pack.SetExternal(TEXT("ctx_hr_h01_checked"),NormalComplete());
 Pack.SetExternal(TEXT("ctx_hr_h01_insufficient"),Ready && !NormalComplete());
 Pack.SetExternal(TEXT("ctx_hr_sc01_window_seen"),Visual && Visual->HandSeen());
 Pack.SetExternal(TEXT("ctx_hr_sc01_marks_checked"),Visual && Visual->MarksChecked());
 Pack.SetExternal(TEXT("ctx_hr_sc01_report_readback"),bReport);
 Pack.SetExternal(TEXT("ctx_hr_sc01_dale_reference_heard"),bReferenceHeard);
 if(SC02Live())
 {
  // World facts only: the T01 controller, the service front and the measured body. Dialogue never sets these.
  const bool Result=M->Substation && M->Substation->HasNormalResult();
  Pack.SetExternal(TEXT("ctx_hr_h02_ready"),bComplete && AtServiceFront());
  Pack.SetExternal(TEXT("ctx_hr_h02_checked"),Result);
  Pack.SetExternal(TEXT("ctx_hr_h02_insufficient"),bH02Attempt && !Result);
  Pack.SetExternal(TEXT("ctx_hr_sc02_t01_complete"),Result);
  Pack.SetExternal(TEXT("ctx_hr_sc02_threat_visible"),bThreatSeen && (SC02==ESC02::Encounter || SC02==ESC02::Escaped));
  Pack.SetExternal(TEXT("ctx_hr_sc02_escape_complete"),SC02==ESC02::Escaped);
  Pack.SetExternal(TEXT("ctx_hr_sc02_local_retry"),bRetry && SC02==ESC02::Encounter);
 }
 if(Revision>=3 && SC03 && (SC03->Live() || SC03->Complete()))
 {
  // World facts from FS47HorrorSC03 and completed calls only.
  const auto& F=SC03->Facts();
  Pack.SetExternal(TEXT("ctx_hr_sc03_dispatch_readback"),F.bDispatch);Pack.SetExternal(TEXT("ctx_hr_sc03_ortiz_assigned"),F.bAssigned);
  Pack.SetExternal(TEXT("ctx_hr_sc03_ortiz_identified"),F.bIdentified);Pack.SetExternal(TEXT("ctx_hr_sc03_lift_observed"),F.bObserved);
  Pack.SetExternal(TEXT("ctx_hr_sc03_safe_retreat"),F.bRetreat);Pack.SetExternal(TEXT("ctx_hr_sc03_traces_checked"),SC03->TracesChecked());
  Pack.SetExternal(TEXT("ctx_hr_sc03_missing_readback"),F.bMissing);Pack.SetExternal(TEXT("ctx_hr_sc03_local_order_suspended"),F.bLocalOrder);
  Pack.SetExternal(TEXT("ctx_hr_sc03_deputy_at_stop"),F.bDeputyHere);
 }
 if(const auto* S=Pack.CurrentScene())
 {
  const auto* R=Pack.FindRegistration(S->Id);
  bool Valid=SceneAllowed(R);
  for(const auto& K:S->Requires.All)Valid=Valid && Pack.Flag(K);
  for(const auto& K:S->Requires.None)Valid=Valid && !Pack.Flag(K);
  // Flow restores the pose after Horror::Begin. A pending SC02 scene is judged once the
  // case has started; SC01 scenes keep today's immediate check.
  const bool Restoring=Revision>=2 && R && (R->ScenarioIds.Contains(TEXT("SC02")) || R->ScenarioIds.Contains(TEXT("SC03"))) && M->Flow && M->Flow->bStarting;
  if(!Valid && !Restoring){Pack.Cancel();bDialogueOpen=false;Spoken.Empty();if(Voice)Voice->Stop();}
 }
}
void FS47Horror::Refile()
{
 if(!bEnabled || !M || !M->Story) return;
 for(const FString& Id:Pack.Documents())
 {
  const bool SC03Doc=Id==TEXT("HR_DISPATCH_RECEIPT") || Id==TEXT("HR_HELPER_TRACE_NOTE") || Id==TEXT("HR_MISSING_RECEIPT") || Id==TEXT("HR_LOCAL_HOLD_RECEIPT");
  if(Id!=TEXT("HR_INITIAL_REPORT") && Id!=TEXT("HR_TRAFO_ENCOUNTER") && !SC03Doc)continue; // Later SC owners release their own papers.
  if(SC03Doc && (!SC03 || Revision<3))continue;
  if(Id==TEXT("HR_INITIAL_REPORT") && !bReport) continue;
  if(Id==TEXT("HR_TRAFO_ENCOUNTER") && (SC02!=ESC02::Escaped || EscapeStamp.IsEmpty())) continue; // only an actual escape
  if(M->Story->Docs().ContainsByPredicate([&](const FS47Doc& D){return D.Id==Id;})) continue;
  const auto* D=Pack.FindDocument(Id);if(!D)continue;
  FS47Doc Doc;Doc.Id=Id;Doc.Title=D->Title;Doc.Kind=D->Kind;Doc.Stamp=Id==TEXT("HR_INITIAL_REPORT")?ReportStamp:Id==TEXT("HR_TRAFO_ENCOUNTER")?EscapeStamp:D->Stamp;
  if(SC03Doc){const auto& F=SC03->Facts();Doc.Stamp=Id==TEXT("HR_DISPATCH_RECEIPT")?F.DispatchStamp:Id==TEXT("HR_HELPER_TRACE_NOTE")?F.TakenStamp:Id==TEXT("HR_MISSING_RECEIPT")?F.MissingStamp:F.OrderStamp;}
  Doc.Page=FString::Join(D->Body,TEXT("\n\n"));Doc.Transcript=Doc.Page;M->Story->AddDoc(Doc);
 }
}
void FS47Horror::Changed(){Context();Refile();if(M)M->RequestAutosave();}
void FS47Horror::CalibrationChecked()
{
 if(!bEnabled || !M || !M->Story || M->Story->Rx.Stage!=0 || !M->Story->Rx.Pass())return;
 bBaseline=true;Changed();
}
void FS47Horror::Photo(const FS47AlbumShot& Shot)
{
 if(!bEnabled || !ReferencePhoto.IsEmpty() || Shot.bUnavailable || !Shot.Url.StartsWith(TEXT("data:image/")) ||
    !Shot.Targets.ContainsByPredicate([](const TPair<FString,FVector2D>& T){return T.Key==TEXT("horror:normalLamp");}))return;
 ReferencePhoto=Shot.Id;M->Story->Note(TEXT("A normal reference of the marked lamp is retained in the instant-back album."));Changed();
}
void FS47Horror::TagPhoto(FS47AlbumShot& Shot)
{
 // Album invokes this hook only after an actual rendered exposure. Its fallback
 // JPEG cannot receive this proof, and the photo lens is narrower than the view.
 if(!ReferenceInPhotoView())return;
 const auto P=M->FieldCamera->Project(FS47HorrorVisual::LampPoint());
 Shot.Targets.Emplace(TEXT("horror:normalLamp"),FVector2D(P.U,P.V));
}
bool FS47Horror::ReferenceInPhotoView() const
{
 return bEnabled && M && M->FieldCamera && Visual && Visual->ReferenceInView() &&
  M->FieldCamera->InFrame(FS47HorrorVisual::LampPoint(),.06,.94,.06,.94);
}
void FS47Horror::DoorChanged(const FString& Id,bool bOpen)
{
 if(!bEnabled || !bOpen || !M || !M->Player || !M->World || M->World->Area!=TEXT("saro") || M->World->bDriving ||
    !M->Evening || !M->Evening->Pack.Flag(TEXT("handover_done")) || (M->Flow && M->Flow->bRestoringNow)) return;
 // Actual door transitions from the inside, never a drawing or loaded door state.
 const auto P=M->Player->Pos;
 if(Id==TEXT("east") && M->Web->Rect(TEXT("room.bounds")).Contains(P)) bEast=true;
 if(Id==TEXT("exit") && (M->Web->Rect(TEXT("annex.zone.corridor")).Contains(P) || M->Web->Rect(TEXT("annex.zone.exitDoor")).Contains(P))) bCorridor=true;
 Changed();
}
bool FS47Horror::Trigger(const FString& Id)
{
 if(!bEnabled || bWorldCall || Pack.CurrentScene()) return false;
 const auto* R=Pack.FindRegistration(Id);const auto* S=Pack.FindScene(Id);
 if(!R || !S || !SceneAllowed(R)) return false;
 Context();if(!Pack.SetPhase(S->Phase) || !Pack.Start(Id)) return false;
 Changed();ShowDialogue();return true;
}
void FS47Horror::Telephone()
{
 if(bWorldCall || Pack.CurrentScene()) {ShowDialogue();return;}
 TArray<S47Kit::FW> Rows;
 if(!Pack.Flag(TEXT("hr_h01_brief_presented")))
  Rows.Add(S47Kit::Button(TEXT("Ask Dale about the evening round"),[this]{M->UI->Close(true);Trigger(TEXT("H01_BRIEF"));},true));
 if(Visual->HandSeen() && !bReport)
  Rows.Add(S47Kit::Button(TEXT("Report the person outside the window to dispatch"),[this]
  {M->UI->Close(true);bWorldCall=true;CallCursor=0;Changed();ShowDialogue();},Rows.IsEmpty()));
 if(bReport) Rows.Add(S47Kit::Text(TEXT("Your window report is open. Dispatch has its location and time.")));
 if(Rows.IsEmpty()) Rows.Add(S47Kit::Text(TEXT("The line is available. Finish the ordinary reference checks and try both exits.")));
 const TSharedPtr<SWidget> Primary=Rows.IsEmpty()?nullptr:TSharedPtr<SWidget>(Rows[0]);
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO / CONTACTS"),S47Kit::Col(Rows),760));
 FocusPrimary(Primary);
}
bool FS47Horror::Use(const FString& Id)
{
 if(!bEnabled || !M || !M->Story || !M->Evening || !M->Evening->Pack.Flag(TEXT("handover_done"))) return false;
 // H02: a real use of the marked lamp before a completed result is judged after Substation answers it.
 if(Id==TEXT("substation:lamp") && SC02==ESC02::T01 && AtServiceFront() && M->Substation && M->Substation->HasJournal() && !M->Substation->HasNormalResult()) bAttemptPending=true;
 if(Visual->Use(Id)) return true;
 if(Revision>=3 && SC03 && SC03->Live())
 {
  if(Id==TEXT("town:payphone")){if(bWorldCall || Pack.CurrentScene()){ShowDialogue();return true;}const auto Calls=SC03->CallScenes();if(Calls.IsEmpty())return false;StartCall(Calls);return true;}
  if(Id.StartsWith(TEXT("horror:")) && SC03->Use(Id))
  {
   Changed();
   if(Id==TEXT("horror:ortiz"))Trigger(TEXT("SC03_ORTIZ_IDENTIFY"));
   else if(Id==TEXT("horror:deputy"))Trigger(TEXT("SC03_RECEIVER_QUESTION"));
   return true;
  }
 }
 if(Id==TEXT("phone") && M->Story->Phase()==ES47Phase::Shift && (!bComplete || !bReport)) {Telephone();return true;}
 if(Id==TEXT("logbook") && M->Story->Phase()==ES47Phase::Shift && !bComplete)
 {
  if(bWorldCall || Pack.CurrentScene()){ShowDialogue();return true;}
  if(!Pack.CurrentScene())
  {
   if(NormalComplete() && !Pack.Flag(TEXT("hr_h01_recorded"))) return Trigger(TEXT("H01_CHECK"));
   if(!NormalComplete()) return Trigger(TEXT("H01_INCOMPLETE"));
  }
 }
 return false;
}
void FS47Horror::ObserveVoice()
{
 if(!bVoiceAvailable || !Voice || !M->Audio || !M->Audio->Ctx) return;
 const bool Audible=M->Audio->bCharacterVoices && M->Audio->Master && M->Audio->Master->Gain.Value()>0 &&
  M->Audio->SfxBus && M->Audio->SfxBus->Gain.Value()>0;
 if(!Audible || Voice->State()==TEXT("error"))bAudibleSoFar=false;
 const auto Levels=Voice->AudioLevels();
 if(Spoken==TEXT("H01_BRIEF_002") && bAudibleSoFar && VoiceLength>0 && Voice->State()==TEXT("idle") &&
    Levels.ClipId==Pack.RecordingId(Spoken) && Levels.SourceSamples>0 && Levels.RoutedSamples>0 &&
    Levels.SourcePeak>0 && Levels.RoutedPeak>0 && Levels.BusGain>0 && M->Audio->Ctx->CurrentTime()-VoiceAt>=VoiceLength)
  bReferenceHeard=true;
}
void FS47Horror::ShowDialogue()
{
 if(bDialogueOpen && M->ModalOpen()) return;
 const FS47EveningScene* S=bWorldCall?Pack.FindScene(CallScene):Pack.CurrentScene();
 const FS47EveningLine* L=bWorldCall?(S && S->Lines.IsValidIndex(CallCursor)?&S->Lines[CallCursor]:nullptr):Pack.CurrentLine();
 if(!S) return;
 if(!L && S->Choices.IsEmpty()){if(bWorldCall)FinishCall();else Pack.Advance();Changed();return;}
 bDialogueOpen=true;bResumeRequested=false;TArray<S47Kit::FW> Rows;TSharedPtr<SWidget> Primary;
 if(L)
 {
  if(!L->Speaker.IsEmpty()) Rows.Add(S47Kit::Sub(Pack.ActorName(L->Speaker)));
  Rows.Add(S47Kit::Unread(S47Kit::Text(L->Text)));
  FS47DialogueInput Input;Input.Voice=L->VoiceProfile;Input.Lines.Add({L->Speaker,L->Text});
  Voice->SetEnabled(M->Audio->bCharacterVoices);bVoiceAvailable=L->bRecord && Voice->Available(Input);
  if(Spoken!=L->Id)
  {
   Spoken=L->Id;VoiceLength=0;bAudibleSoFar=true;Voice->SetLevelDiagnostics(true);
   if(bVoiceAvailable){Voice->Play(Input);VoiceAt=Voice->StartedAt();const auto State=Voice->Status();if(State.Clips.Num()==1)VoiceLength=State.Clips[0].Duration;}
  }
  Rows.Add(S47Kit::Button(TEXT("Continue"),[this]{AdvanceDialogue();},true));
  Primary=Rows.Last();
 }
 else for(const auto* C:Pack.Choices())
 {
  const FString Id=C->Id;Rows.Add(S47Kit::Button(C->Label,[this,Id]{if(Pack.Select(Id)){M->UI->Close(true);bDialogueOpen=false;Changed();ShowDialogue();}},Rows.IsEmpty()));if(!Primary)Primary=Rows.Last();
 }
 const FString Title=S->Channel==TEXT("phone")?TEXT("TELEPHONE"):S->Channel==TEXT("walkie")?TEXT("WALKIE-TALKIE"):TEXT("SHIFT RECORD");
 M->UI->Open(S47Kit::Shell(M->UI,Title,S47Kit::Unread(S47Kit::Col(Rows)),820,true),[this]
 {ObserveVoice();bDialogueOpen=false;bResumeRequested=false;Spoken.Empty();if(Voice)Voice->Stop();Changed();});
 FocusPrimary(Primary);
}
void FS47Horror::AdvanceDialogue()
{
 ObserveVoice();if(Voice)Voice->Stop();M->UI->Close(true);bDialogueOpen=false;Spoken.Empty();
 if(bWorldCall)
 {++CallCursor;const auto* S=Pack.FindScene(CallScene);if(!S || CallCursor>=S->Lines.Num())FinishCall();}
 else Pack.Advance();
 Changed();if(bWorldCall || Pack.CurrentScene())ShowDialogue();
}
void FS47Horror::FinishCall()
{
 // The actual phone transaction presented these exact response lines first.
 // Only a completed contact may publish its readback and retained result document.
 if(CallScene!=TEXT("SC01_FIRST_REPORT"))
 {
  // SC03: the completed call is the fact; then the presented scene is registered once.
  bWorldCall=false;SC03->CallFinished(CallScene);Context();
  if(const auto* S=Pack.FindScene(CallScene); S && Pack.SetPhase(S->Phase) && Pack.Start(S->Id))
   for(int32 I=0;I<S->Lines.Num();++I) Pack.Advance();
  if(!CallQueue.IsEmpty()){CallScene=CallQueue[0];CallQueue.RemoveAt(0);CallCursor=0;bWorldCall=true;}
  else CallScene=TEXT("SC01_FIRST_REPORT");
  Changed();return;
 }
 bWorldCall=false;bReport=Visual->HandSeen();ReportStamp=M->Story->ClockText(true);Context();
 const auto* S=Pack.FindScene(TEXT("SC01_FIRST_REPORT"));
 if(bReport && S && Pack.SetPhase(S->Phase) && Pack.Start(S->Id))
  for(int32 I=0;I<S->Lines.Num();++I) Pack.Advance();
 Changed();
}
void FS47Horror::Step(double Dt)
{
 if(!bEnabled || !M || M->Mode!=US47Main::EMode::Play) return;
 Visual->Step(Dt);ObserveVoice();Context();
 if(!M->Evening || !M->Evening->Pack.Flag(TEXT("handover_done")) || M->bPausedByMenu) return;
 // StagePassed witnesses the actual calibration before Rx advances to interference.
 if(NormalComplete()) Visual->ArmWindow();
 if(!bComplete && NormalComplete() && DoorsSecured() && (!Visual->HandSeen() || Pack.Flag(TEXT("hr_sc01_window_check_presented"))) && Visual->MarksChecked())
 {bComplete=true;Visual->ReleaseLighting();M->Story->Note(Visual->HandSeen()?TEXT("The evening reference and the available exits have been checked. The marks are on the outside of the glass."):TEXT("The evening reference and the exits have been checked. Marks remain on the outside of the glass; their cause was not seen."));Changed();}
 StepSC02(Dt);
 StepSC03(Dt);
 if(!M->ModalOpen() && !M->World->bBusy && !bDialogueOpen)
 {
  if(bWorldCall || Pack.CurrentScene()){if(bResumeRequested)ShowDialogue();}
  else if(!bComplete && !Pack.Flag(TEXT("hr_h01_brief_presented")) && M->World->Area==TEXT("saro")) Trigger(TEXT("H01_BRIEF"));
  else if(!bComplete && NormalComplete() && !Pack.Flag(TEXT("hr_h01_recorded"))) Trigger(TEXT("H01_CHECK"));
  else if(!bComplete && Visual->HandSeen() && Visual->MarksChecked() && !Pack.Flag(TEXT("hr_sc01_window_check_presented"))) Trigger(TEXT("SC01_WINDOW_CHECK"));
  else if(SC03Live() && SC03->Phase()!=FS47HorrorSC03::EPhase::Report){if(SC03->Facts().bRetreat && !Pack.Flag(TEXT("hr_sc03_helper_taken_presented")))Trigger(TEXT("SC03_HELPER_TAKEN"));else if(SC03->TracesChecked() && !Pack.Flag(TEXT("hr_sc03_helper_traces_presented")))Trigger(TEXT("SC03_HELPER_TRACES"));}
  else if(bComplete && SC02Live()) TriggerSC02();
 }
 if(!bComplete && !M->World->bDriving && !M->Evening->Opening())M->UI->Objective(Objective());
 else if(SC02Meeting() || SC03Live())M->UI->Objective(Objective());
}
FString FS47Horror::Objective() const
{
 if(!bEnabled) return FString();
 if(bComplete)
 {
  if(SC03Live()) return SC03->Objective();
  if(!SC02Meeting()) return FString();
  if(M->World && M->World->bDriving) return TEXT("CALICHE JUNCTION // DRIVE CLEAR OF THE SERVICE FRONT");
  if(!bThreatSeen) return TEXT("CALICHE JUNCTION // RETURN TO SARO 07 ON THE GROUND OUTSIDE THE FENCE");
  if(Threat->State()==FS47HorrorThreat::EState::Chase) return TEXT("CALICHE JUNCTION // BREAK THE LINE OF SIGHT BEHIND SOLID COVER");
  return TEXT("CALICHE JUNCTION // KEEP SOLID COVER BETWEEN YOU AND THE BODY. REACH SARO 07 OUTSIDE THE FENCE AND DRIVE");
 }
 if(!M->FieldCamera || !M->FieldCamera->bHave)return TEXT("EVENING ROUND // COLLECT THE CAMERA BESIDE THE EAST DOOR");
 if(!bEast || !bCorridor)
 {
  // A door left open (a save, or opened from outside) gives no inside opening; say what to do (FIX-06).
  const auto Doors=M->Web?M->Web->DoorStates():TMap<FString,bool>();
  if(!bEast && Doors.FindRef(TEXT("east")))return TEXT("EVENING ROUND // THE EAST SERVICE DOOR STANDS OPEN. CLOSE IT, THEN OPEN IT FROM INSIDE THE CONTROL ROOM");
  if(!bCorridor && Doors.FindRef(TEXT("exit")))return TEXT("EVENING ROUND // THE VISITOR ENTRANCE STANDS OPEN. CLOSE IT, THEN OPEN IT FROM THE CORRIDOR");
  if(!bEast)return bCorridor?TEXT("EVENING ROUND // FROM INSIDE THE CONTROL ROOM, OPEN THE EAST SERVICE DOOR"):TEXT("EVENING ROUND // FROM INSIDE, OPEN THE EAST SERVICE DOOR AND THE VISITOR ENTRANCE AT THE WEST END OF THE CORRIDOR");
  return TEXT("EVENING ROUND // FROM THE CORRIDOR, OPEN THE VISITOR ENTRANCE AT ITS WEST END");
 }
 if(!bBaseline)return TEXT("CONTROL ROOM // CHECK 1419.900 MHz / AZ 042 / GAIN 55 / BW 48");
 if(M->Observing && M->Observing->Active())return M->Observing->Objective();
 if(!Visual->LampChangeChecked())return TEXT("NORTH WINDOW // FROM INSIDE, SWITCH THE MARKED LAMP OFF AND ON WHILE WATCHING IT THROUGH THE GLASS");
 if(ReferencePhoto.IsEmpty())return TEXT("NORTH WINDOW // TAKE AN INSTANT-BACK REFERENCE OF THE MARKED LAMP WHILE IT IS ON");
 if(!Visual->ScreenChecked())return TEXT("NORTH WINDOW // CLOSE THE OPAQUE SHUTTER AND CHECK THAT IT BLOCKS THE LAMP");
 if(Visual->ScreenClosed())return TEXT("NORTH WINDOW // OPEN THE SHUTTER AND CHECK THE GLASS");
 if(!Visual->MarksChecked())return Visual->MarksPresent()?TEXT("NORTH WINDOW // CHECK THE OUTSIDE MARKS WITH THE ROOM LAMP ON AND OFF"):TEXT("NORTH WINDOW // THE GLASS IS CLEAR. LOWER THE SHADE FOR THE NIGHT");
 if(!DoorsSecured())return TEXT("CONTROL ROOM // CLOSE THE EAST SERVICE DOOR AND THE VISITOR ENTRANCE");
 return TEXT("SHIFT FOLDER // RECORD THE EVENING CHECKS. THE TELEPHONE IS AVAILABLE TO REPORT WHAT YOU SAW");
}
TArray<FS47Task> FS47Horror::Tasks() const
{
 if(!bEnabled)return {};
 if(bComplete)
 {
  if(SC03Live()){TArray<FS47Task> Out;for(const auto& Row:SC03->Tasks())Out.Add({Row.Key,Row.Value});return Out;}
  if(!SC02Meeting())return {};
  return {{TEXT("Compare the marked lamp with the completed service check"),M->Substation && M->Substation->HasNormalResult()},
  {TEXT("Reach SARO 07 by a route outside the fence"),M->World && M->World->bDriving},
  {TEXT("Drive clear of Caliche Junction"),false}};
 }
 TArray<FS47Task> Out={{TEXT("Collect the field camera"),M->FieldCamera && M->FieldCamera->bHave},
 {TEXT("Open the service door and the visitor entrance from inside"),bEast && bCorridor},
 {TEXT("Check the normal receiver reference"),bBaseline}};
 if(M->Observing && bBaseline)Out.Append(M->Observing->Tasks());
 Out.Append({{TEXT("Take an instant-back reference of the marked lamp"),!ReferencePhoto.IsEmpty()},
 {TEXT("Switch the lamp off and on through the glass, then check it behind the shade"),Visual->LampChangeChecked() && Visual->ScreenChecked()},
 {TEXT("Check the window and its outside marks"),Visual->MarksChecked()}});
 return Out;
}
S47J::FObj FS47Horror::Save() const
{
 if(!bEnabled)return nullptr;
 auto O=S47J::New();S47J::Set(O,TEXT("schemaVersion"),1);S47J::Set(O,TEXT("contentRevision"),Revision);
 S47J::Set(O,TEXT("scenario"),ScenarioName(Revision,bComplete,SC02));
 S47J::Set(O,TEXT("sc01Complete"),bComplete);
 S47J::Set(O,TEXT("eastTried"),bEast);S47J::Set(O,TEXT("corridorTried"),bCorridor);S47J::Set(O,TEXT("baseline"),bBaseline);
 S47J::Set(O,TEXT("referencePhoto"),ReferencePhoto);
 S47J::Set(O,TEXT("reportAcknowledged"),bReport);S47J::Set(O,TEXT("reportStamp"),ReportStamp);
 S47J::Set(O,TEXT("daleReferenceHeard"),bReferenceHeard);S47J::Set(O,TEXT("callActive"),bWorldCall);S47J::Set(O,TEXT("callCursor"),CallCursor);
 if(CallScene!=TEXT("SC01_FIRST_REPORT")){S47J::Set(O,TEXT("callScene"),CallScene);TArray<TSharedPtr<FJsonValue>> Q;for(const auto& Q0:CallQueue)Q.Add(MakeShared<FJsonValueString>(Q0));O->SetArrayField(TEXT("callQueue"),Q);}
 S47J::Set(O,TEXT("visual"),Visual->Save());S47J::Set(O,TEXT("pack"),Pack.Save());
 if(Revision>=2)
 {
  // A meeting in progress is saved as its safe checkpoint, never as a half chase.
  FSC02Save S;S.Phase=SC02==ESC02::Legacy && Threat && Threat->Unavailable()?ESC02::T01:SC02;S.bAttempt=bH02Attempt;S.bRetry=bRetry;S.bSeen=bThreatSeen;S.EscapeStamp=EscapeStamp;
  S.Checkpoint=Threat?Threat->CheckpointId():FString();S47J::Set(O,TEXT("sc02"),WriteSC02(S));
 }
 if(Revision>=3 && SC03 && SC03->Phase()!=FS47HorrorSC03::EPhase::Off)S47J::Set(O,TEXT("sc03"),SC03->Save());
 if(Revision>=3 && SC03Live())S47J::Set(O,TEXT("scenario"),TEXT("SC03"));
 return O;
}
bool FS47Horror::SelfTest(TArray<FString>& Lines)
{
 FString Error;const bool Good=FS47HorrorPack::SelfTest(Error);
 Lines.Add(Good?TEXT("horror pack: all 77 scene contracts and restore gates OK"):TEXT("horror pack FAILED: ")+Error);
 const bool VisualGood=FS47HorrorVisual::SelfTest(Lines);
 const bool RouteGood=SC02SelfTest(Lines);
 const bool ThreatGood=FS47HorrorThreat::SelfTest(Lines);
 const bool SC03Good=FS47HorrorSC03::SelfTest(Lines);
 return VisualGood && Good && RouteGood && ThreatGood && SC03Good;
}

// ---------- SC02: return to the truck, meeting 1 (revision 2 only) ----------

bool FS47Horror::HasTasks() const {return bEnabled && (!bComplete || SC02Meeting() || SC03Live());}
bool FS47Horror::SC03Live() const {return bEnabled && Revision>=3 && SC03 && SC03->Live();}
bool FS47Horror::SC02Meeting() const {return bEnabled && bComplete && SC02==ESC02::Encounter && Threat && AtSite();}
bool FS47Horror::SC02Escaped() const {return bEnabled && SC02==ESC02::Escaped;}
bool FS47Horror::SceneAllowed(const FS47HorrorSceneRegistration* R) const
{
 if(R && Revision>=3 && SC03 && (SC03->Live() || SC03->Complete()) && R->ScenarioIds.Contains(TEXT("SC03")))return true;
 return R && AllowsScenario(Revision,SC02,R->ScenarioIds);
}
bool FS47Horror::AllowsScenario(int32 InRevision,ESC02 Phase,const TArray<FString>& ScenarioIds)
{
 // SC01 scenes keep today's rule. SC02 scenes exist only for a revision 2 case in a live SC02 phase.
 if(ScenarioIds.Contains(TEXT("SC01")))return true;
 return InRevision>=2 && (Phase==ESC02::T01 || Phase==ESC02::Encounter || Phase==ESC02::Escaped) && ScenarioIds.Contains(TEXT("SC02"));
}
FString FS47Horror::ScenarioName(int32 InRevision,bool bSC01Complete,ESC02 Phase)
{
 if(!bSC01Complete)return TEXT("SC01");
 if(InRevision>=2 && (Phase==ESC02::T01 || Phase==ESC02::Encounter))return TEXT("SC02");
 return TEXT("legacyContinuation");
}
FS47Horror::ESC02 FS47Horror::AfterSC01(int32 InRevision,bool bThreatAvailable)
{
 if(InRevision<2)return ESC02::Off;
 return bThreatAvailable?ESC02::T01:ESC02::Legacy; // missing pale body: the existing story continues, and that boundary is saved
}
const TCHAR* FS47Horror::PhaseName(ESC02 Phase)
{
 switch(Phase)
 {
 case ESC02::Off: return TEXT("sc01");
 case ESC02::T01: return TEXT("t01");
 case ESC02::Encounter: return TEXT("encounter");
 case ESC02::Escaped: return TEXT("escaped");
 case ESC02::Unwitnessed: return TEXT("unwitnessed");
 case ESC02::Legacy: return TEXT("legacy");
 }
 return TEXT("sc01");
}
bool FS47Horror::ParsePhase(const FString& Text,ESC02& Out)
{
 for(const ESC02 Candidate:{ESC02::Off,ESC02::T01,ESC02::Encounter,ESC02::Escaped,ESC02::Unwitnessed,ESC02::Legacy})
  if(Text==PhaseName(Candidate)){Out=Candidate;return true;}
 return false;
}
S47J::FObj FS47Horror::WriteSC02(const FSC02Save& In)
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("phase"),PhaseName(In.Phase));
 S47J::Set(O,TEXT("attempt"),In.bAttempt);S47J::Set(O,TEXT("retry"),In.bRetry);S47J::Set(O,TEXT("threatSeen"),In.bSeen);
 S47J::Set(O,TEXT("escapeStamp"),In.EscapeStamp);S47J::Set(O,TEXT("checkpoint"),In.Checkpoint);
 return O;
}
bool FS47Horror::ReadSC02(const S47J::FObj& O,FSC02Save& Out)
{
 Out=FSC02Save();
 if(!O || !ExactVersion(O,TEXT("v"),1,1))return false;
 const auto* P=O->Values.Find(TEXT("phase"));
 if(!P || !P->IsValid() || (*P)->Type!=EJson::String || !ParsePhase((*P)->AsString(),Out.Phase))return false;
 if(!SC02Bool(O,TEXT("attempt"),Out.bAttempt) || !SC02Bool(O,TEXT("retry"),Out.bRetry) || !SC02Bool(O,TEXT("threatSeen"),Out.bSeen))return false;
 Out.EscapeStamp=S47J::Str(O,TEXT("escapeStamp"));Out.Checkpoint=S47J::Str(O,TEXT("checkpoint"));
 return true;
}
bool FS47Horror::AtServiceFront() const
{
 // Same front as FS47Substation::Here: on foot in the road/diner walk within 12 m of the marked control.
 return M && M->Substation && M->Substation->Ready() && M->World && M->Player && M->Mode==US47Main::EMode::Play &&
  (M->World->Area==TEXT("road") || M->World->Area==TEXT("diner")) && !M->World->bDriving &&
  FVector2D(M->Player->Pos.X-7885,M->Player->Pos.Z-527.3).Size()<12;
}
bool FS47Horror::AtSite() const
{
 if(!M || !M->World || !M->Player)return false;
 const FVector2D C=FS47HorrorThreat::SiteCentre();
 if(M->World->bDriving){const FS47Truck* T=M->World->CabTruck();return T && FVector2D::Distance(FVector2D(T->Position.X,T->Position.Z),C)<80;}
 return (M->World->Area==TEXT("road") || M->World->Area==TEXT("diner")) && FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),C)<80;
}
void FS47Horror::BeginSC02(const S47J::FObj& Saved)
{
 SC02=ESC02::Off;bH02Attempt=bAttemptPending=bAttemptShowing=bRetry=bRetryShowing=bThreatSeen=bCheckpointPending=false;EscapeStamp.Empty();
 if(Threat)Threat->Stop();
 if(Revision<2 || !Threat)return;
 if(!Threat->Ready())Threat->Build();
 FSC02Save Stored;
 if(const auto Field=S47J::Obj(Saved,TEXT("sc02")); Field && !ReadSC02(Field,Stored))
 {Stored=FSC02Save();UE_LOG(LogTemp,Warning,TEXT("S47: SC02 save rejected; no meeting or escape credit restored"));}
 SC02=Stored.Phase;bH02Attempt=Stored.bAttempt;bRetry=Stored.bRetry;bThreatSeen=Stored.bSeen;EscapeStamp=Stored.EscapeStamp;
 // Re-derive against the restored modules: Substation::Begin and the SC01 facts are already in place.
 const bool Result=M->Substation && M->Substation->HasNormalResult();
 if(SC02!=ESC02::Off && SC02!=ESC02::Legacy && !bComplete)
 {UE_LOG(LogTemp,Warning,TEXT("S47: SC02 phase without completed SC01 rejected"));SC02=ESC02::Off;}
 if((SC02==ESC02::Encounter || SC02==ESC02::Escaped || SC02==ESC02::Unwitnessed) && !Result)
 {UE_LOG(LogTemp,Warning,TEXT("S47: SC02 meeting without a restored T01 result rejected; T01 part resumes"));SC02=ESC02::T01;}
 if(SC02==ESC02::Escaped && (!bThreatSeen || EscapeStamp.IsEmpty()))
 {UE_LOG(LogTemp,Warning,TEXT("S47: SC02 escape without witnessed forewarning or stamp rejected; meeting resumes at its checkpoint"));SC02=ESC02::Encounter;}
 if(SC02==ESC02::Off && bComplete)SC02=AfterSC01(Revision,!Threat->Unavailable());
 if(Threat->Unavailable() && (SC02==ESC02::T01 || SC02==ESC02::Encounter))
 {UE_LOG(LogTemp,Warning,TEXT("S47: SC02 pale body unavailable; case continues in the existing story"));SC02=ESC02::Legacy;}
 if(SC02!=ESC02::T01)bH02Attempt=false;
 if(SC02!=ESC02::Encounter)bRetry=false;
 if(SC02!=ESC02::Encounter && SC02!=ESC02::Escaped)bThreatSeen=false;
 if(SC02!=ESC02::Escaped)EscapeStamp.Empty();
 if(SC02==ESC02::Encounter){Threat->Start(bThreatSeen);bCheckpointPending=true;}
 // A restored pending response keeps its one-time fact until it is acknowledged.
 if(const auto* Scene=Pack.CurrentScene()){bRetryShowing=bRetry && Scene->Id==TEXT("SC02_RETRY");bAttemptShowing=bH02Attempt && Scene->Id==TEXT("H02_INCOMPLETE");}
}
void FS47Horror::StepSC02(double Dt)
{
 if(Revision<2 || !Threat)return;
 if(!Threat->Ready() && !Threat->Unavailable())Threat->Build();
 if(SC02==ESC02::Off && bComplete)
 {SC02=AfterSC01(Revision,!Threat->Unavailable());UE_LOG(LogTemp,Display,TEXT("S47: SC01 complete; revision %d continues with SC02 %s"),Revision,PhaseName(SC02));Changed();}
 if(Threat->Unavailable() && (SC02==ESC02::T01 || SC02==ESC02::Encounter))
 {SC02=ESC02::Legacy;Threat->Stop();UE_LOG(LogTemp,Warning,TEXT("S47: SC02 pale body unavailable; case continues in the existing story"));Changed();}
 if(SC02==ESC02::T01)
 {
  const bool Result=M->Substation && M->Substation->HasNormalResult();
  if(bAttemptPending){bAttemptPending=false;if(M->Substation && M->Substation->HasJournal() && !Result){bH02Attempt=true;Changed();}}
  if(bAttemptShowing && (!Pack.CurrentScene() || Pack.CurrentScene()->Id!=TEXT("H02_INCOMPLETE"))){bAttemptShowing=false;bH02Attempt=false;Changed();}
  if(Result && bH02Attempt){bH02Attempt=false;Changed();}
  // Meeting 1 starts only after the T01 result is secured, its record presented and every panel closed.
  const bool NightReady = !M->Story->NightSequence() ||
   (M->Evening && M->Evening->FeederCallAnswered() && M->Evening->DriveTimeSkipped() && !M->Story->FeederOutage());
  if(Result && NightReady && Threat->Ready() && Pack.Flag(TEXT("hr_h02_recorded")) && AtServiceFront() && !M->ModalOpen() && !bDialogueOpen && !bWorldCall && !Pack.CurrentScene())
  {if(M->Story->NightSequence())M->Story->AdvanceClockTo(S47Timeline::FirstEncounter,TEXT("first encounter after feeder restoration"));SC02=ESC02::Encounter;Threat->Start(false);UE_LOG(LogTemp,Display,TEXT("S47: SC02 meeting 1 starts after secured T01 result %s"),*M->Substation->NormalCompletionStamp());Changed();}
 }
 else if(SC02==ESC02::Encounter)
 {
  if(bCheckpointPending && M->Flow && !M->Flow->bStarting && !M->World->bBusy)
  {
   // Continue in the middle of the meeting resumes at its safe checkpoint, never in a half chase.
   bCheckpointPending=false;
   if(!M->World->bDriving && AtSite()){const FVector2D P=Threat->Checkpoint();M->Player->Place(P.X,P.Y,0);M->Player->Pitch=-.06;}
   Threat->Start(bThreatSeen); // ground height and route checks read the restored world, not the pre-restore pose
  }
  Threat->Step(Dt);
  if(Threat->Seen() && !bThreatSeen){bThreatSeen=true;Changed();}
  if(Threat->ConsumeCapture())LocalRetry(TEXT("capture"));
  else if(!M->World->bDriving && AtSite() && FS47HorrorThreat::InYard(FVector2D(M->Player->Pos.X,M->Player->Pos.Z)))LocalRetry(TEXT("closed yard"));
  if(bRetryShowing && (!Pack.CurrentScene() || Pack.CurrentScene()->Id!=TEXT("SC02_RETRY"))){bRetryShowing=false;bRetry=false;Changed();}
  // Driving clear also ends a meeting whose body never appeared (for example after Continue in the cab).
  if(M->World->bDriving && (Threat->Withdrawn() || (!Threat->Appeared() && !AtSite())))
  {
   if(bThreatSeen)
   {
    SC02=ESC02::Escaped;EscapeStamp=M->Story->ClockText(true);bRetry=bRetryShowing=false;
    if(!Threat->Appeared())Threat->Stop(); // nothing to withdraw; it must not appear behind the departed truck
    M->Story->Note(TEXT("Caliche Junction: a pale body moved at the service front and turned towards sound. Reached SARO 07 outside the closed yard and drove clear. The completed lamp check remains on its card."));
   }
   else {SC02=ESC02::Unwitnessed;Threat->Stop();} // no sighting, no encounter paper
   UE_LOG(LogTemp,Display,TEXT("S47: SC02 contact broken from SARO 07; %s"),PhaseName(SC02));
   Changed();
  }
 }
 else if(SC02==ESC02::Escaped && Threat->Active()){Threat->Step(Dt);if(Threat->Gone())Threat->Stop();}
}
void FS47Horror::TriggerSC02()
{
 const bool Driving=M->World->bDriving;
 if(SC02==ESC02::Escaped){if(!Driving || FMath::Abs(M->World->DrivingSpeed())<.5)Trigger(TEXT("SC02_ESCAPE"));return;}
 if(SC02==ESC02::Encounter)
 {
  if(Driving || !AtSite())return;
  if(bRetry){if(Trigger(TEXT("SC02_RETRY")))bRetryShowing=true;return;}
  const auto State=Threat->State();
  if(bThreatSeen && State!=FS47HorrorThreat::EState::Chase && State!=FS47HorrorThreat::EState::Capture)Trigger(TEXT("SC02_RETURN_WARNING"));
  return;
 }
 if(SC02==ESC02::T01)
 {
  if (M->Story->NightSequence() && (Driving || !AtServiceFront() || !M->Evening || !M->Evening->FeederCallAnswered() || !M->Evening->DriveTimeSkipped())) return;
  if(Trigger(TEXT("H02_INCOMPLETE"))){bAttemptShowing=true;return;}
  if(Trigger(TEXT("H02_CHECK")))return;
  Trigger(TEXT("H02_BRIEF"));
 }
}
void FS47Horror::LocalRetry(const TCHAR* Why)
{
 // Local only: no FS47Abduction, FS47TruckTaken or station taking and no clock change.
 // T01, SARO 07 where it stands and the single shift-folder filing are untouched.
 if(M->Player && !M->World->bDriving){const FVector2D P=Threat->Checkpoint();M->Player->Place(P.X,P.Y,0);M->Player->Pitch=-.06;}
 Threat->Reset();bRetry=true;bRetryShowing=false;
 if(M->Audio)M->Audio->Gasp();
 UE_LOG(LogTemp,Display,TEXT("S47: SC02 local retry after %s at %s; T01 %s; clock unchanged"),Why,*Threat->CheckpointId(),M->Substation && M->Substation->HasNormalResult()?TEXT("kept"):TEXT("absent"));
 Changed();
}
bool FS47Horror::SC02SelfTest(TArray<FString>& Lines)
{
 bool Good=true;
 auto Check=[&](bool Value,const TCHAR* What){Good&=Value;Lines.Add(FString::Printf(TEXT("SC02 route %s %s"),Value?TEXT("PASS"):TEXT("FAIL"),What));};
 Check(ScenarioName(1,false,ESC02::Off)==TEXT("SC01") && ScenarioName(1,true,ESC02::Off)==TEXT("legacyContinuation") && AfterSC01(1,true)==ESC02::Off,
  TEXT("revision 1 keeps SC01 then legacyContinuation and never starts SC02"));
 Check(!AllowsScenario(1,ESC02::Off,{TEXT("SC02")}) && !AllowsScenario(1,ESC02::T01,{TEXT("SC02")}) && AllowsScenario(1,ESC02::Off,{TEXT("SC01")}),
  TEXT("revision 1 refuses SC02 scenes and keeps SC01 scenes"));
 Check(AfterSC01(2,true)==ESC02::T01 && ScenarioName(2,true,ESC02::T01)==TEXT("SC02") && ScenarioName(2,true,ESC02::Encounter)==TEXT("SC02") && AllowsScenario(2,ESC02::T01,{TEXT("SC02")}),
  TEXT("revision 2 continues from completed SC01 into SC02"));
 Check(AfterSC01(2,false)==ESC02::Legacy && ScenarioName(2,true,ESC02::Legacy)==TEXT("legacyContinuation") && !AllowsScenario(2,ESC02::Legacy,{TEXT("SC02")}) && !AllowsScenario(2,ESC02::Off,{TEXT("SC02")}),
  TEXT("missing SC02 assets keep the existing story and the boundary is saved"));
 FSC02Save Saved;Saved.Phase=ESC02::Encounter;Saved.bRetry=true;Saved.bSeen=true;Saved.Checkpoint=TEXT("shed");
 FSC02Save Back;
 Check(ReadSC02(WriteSC02(Saved),Back) && Back.Phase==ESC02::Encounter && Back.bRetry && Back.bSeen && !Back.bAttempt && Back.Checkpoint==TEXT("shed") && Back.EscapeStamp.IsEmpty(),
  TEXT("SC02 phase, safe checkpoint and meeting status round-trip"));
 bool AllPhases=true;ESC02 Parsed=ESC02::Off;
 for(const ESC02 Candidate:{ESC02::Off,ESC02::T01,ESC02::Encounter,ESC02::Escaped,ESC02::Unwitnessed,ESC02::Legacy})AllPhases&=ParsePhase(PhaseName(Candidate),Parsed) && Parsed==Candidate;
 Check(AllPhases && !ParsePhase(TEXT("chase"),Parsed),TEXT("every SC02 phase has one saved name and a half chase is no phase"));
 const auto TextFlag=WriteSC02(Saved);S47J::Set(TextFlag,TEXT("threatSeen"),TEXT("true"));
 const auto Future=WriteSC02(Saved);S47J::Set(Future,TEXT("v"),2);
 Check(!ReadSC02(TextFlag,Back) && !ReadSC02(Future,Back) && !ReadSC02(nullptr,Back) && Back.Phase==ESC02::Off && !Back.bSeen,
  TEXT("strings, unknown versions and absent fields never manufacture SC02 facts"));
 FS47HorrorPack Data;FString Error;
 if(Data.Initialize(Error))
 {
  const TArray<FString> Ids=Data.SceneIdsForScenario(TEXT("SC02"));
  const TArray<FString> Want={TEXT("H02_BRIEF"),TEXT("H02_CHECK"),TEXT("H02_INCOMPLETE"),TEXT("SC02_RETURN_WARNING"),TEXT("SC02_ESCAPE"),TEXT("SC02_RETRY")};
  const TArray<FString> Facts={TEXT("ctx_hr_h02_ready"),TEXT("ctx_hr_h02_checked"),TEXT("ctx_hr_h02_insufficient"),TEXT("ctx_hr_sc02_t01_complete"),
   TEXT("ctx_hr_sc02_threat_visible"),TEXT("ctx_hr_sc02_escape_complete"),TEXT("ctx_hr_sc02_local_retry")};
  bool Same=Ids.Num()==Want.Num();for(const FString& Id:Want)Same&=Ids.Contains(Id);
  bool Hosted=true;
  for(const FString& Id:Ids)if(const auto* Scene=Data.FindScene(Id))for(const FString& K:Scene->Requires.All)if(K.StartsWith(TEXT("ctx_")))Hosted&=Facts.Contains(K);
  const TArray<FString> External=Data.ExternalIds();bool Declared=true;for(const FString& K:Facts)Declared&=External.Contains(K);
  Check(Same && Hosted && Declared,TEXT("the six SC02 scenes read only the seven host-owned SC02 world facts"));
 }
 else Check(false,TEXT("horror pack initializes for the SC02 checks"));
 return Good;
}

// ---------- SC03: the help that came (revision 3 only) ----------

void FS47Horror::StartCall(const TArray<FString>& Scenes)
{
 if(Scenes.IsEmpty() || bWorldCall || Pack.CurrentScene())return;
 M->UI->Close(true);CallScene=Scenes[0];CallQueue=Scenes;CallQueue.RemoveAt(0);CallCursor=0;bWorldCall=true;
 Changed();ShowDialogue();
}
void FS47Horror::StepSC03(double Dt)
{
 if(Revision<3 || !SC03)return;
 // SC02's escape presented in full: the report from Caliche is owed.
 if(SC02==ESC02::Escaped && SC03->Phase()==FS47HorrorSC03::EPhase::Off && Pack.Flag(TEXT("hr_sc02_escape_presented")) && !Pack.CurrentScene())
 {if(M->Story->NightSequence())M->Story->AdvanceClockTo(S47Timeline::TownReport,TEXT("Caliche report after escape"));SC03->Start();UE_LOG(LogTemp,Display,TEXT("S47: SC02 escape presented; revision %d continues with SC03"),Revision);}
 if(!SC03->Live())return;
 SC03->Step(Dt);
 if(Pack.Flag(TEXT("hr_sc03_receiver_question_presented")) && !Pack.CurrentScene())SC03->ScenePresented(TEXT("SC03_RECEIVER_QUESTION"));
 if(SC03->Changes()!=SC03Seen){SC03Seen=SC03->Changes();Changed();}
}
