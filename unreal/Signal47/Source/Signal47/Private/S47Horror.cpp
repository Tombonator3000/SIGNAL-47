#include "S47Horror.h"
#include "S47HorrorVisual.h"
#include "S47Main.h"
#include "S47Evening.h"
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
#include "Framework/Application/SlateApplication.h"

namespace
{
 constexpr int32 InstalledRevision = 1;
 bool ExactVersion(const S47J::FObj& O,const TCHAR* Key,int32 Min,int32 Max)
 {
  const auto* V=O?O->Values.Find(Key):nullptr;
  if(!V || !V->IsValid() || (*V)->Type!=EJson::Number)return false;
  const double N=(*V)->AsNumber();return FMath::IsFinite(N) && N>=Min && N<=Max && FMath::FloorToDouble(N)==N;
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
}
void FS47Horror::Begin(const S47J::FObj& Saved,bool bNewCase)
{
 Stop();Pack.Reset();bEast=bCorridor=bBaseline=bReport=bComplete=bReferenceHeard=false;CallCursor=0;ReportStamp.Empty();ReferencePhoto.Empty();Revision=0;
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
  bWorldCall=S47J::Bool(Saved,TEXT("callActive")) && !bReport && Visual->HandSeen();
  CallCursor=FMath::Clamp(S47J::Int(Saved,TEXT("callCursor")),0,1);
  // Physical facts must be restored before the pack validates a pending result.
  Context();FString Error;
  if(!Pack.Load(S47J::Obj(Saved,TEXT("pack")),Error))
  {Pack.Reset();UE_LOG(LogTemp,Warning,TEXT("S47: horror dialogue restore rejected: %s"),*Error);}
 }
 Context();Refile();
 bResumeRequested=bWorldCall || Pack.CurrentScene()!=nullptr;
 bComplete=S47J::Bool(Saved,TEXT("sc01Complete")) && NormalComplete() && Visual->MarksChecked() && (!Visual->HandSeen() || Pack.Flag(TEXT("hr_sc01_window_check_presented")));
 if(bComplete)Visual->ReleaseLighting();
 UE_LOG(LogTemp,Display,TEXT("S47: horror route revision %d %s, SC01 %s"),Revision,Saved?TEXT("restored"):TEXT("new"),bComplete?TEXT("complete"):TEXT("active"));
}
bool FS47Horror::NormalComplete() const
{
 return bEnabled && M && M->FieldCamera && M->FieldCamera->bHave && !ReferencePhoto.IsEmpty() && bEast && bCorridor && bBaseline &&
  Visual && Visual->LampChangeChecked() && Visual->ScreenChecked();
}
bool FS47Horror::Busy() const {return bEnabled && Visual && (Visual->WindowPhase()==1 || bWorldCall || Pack.CurrentScene());}
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
 if(const auto* S=Pack.CurrentScene())
 {
  const auto* R=Pack.FindRegistration(S->Id);
  bool Valid=R && R->ScenarioIds.Contains(TEXT("SC01"));
  for(const auto& K:S->Requires.All)Valid=Valid && Pack.Flag(K);
  for(const auto& K:S->Requires.None)Valid=Valid && !Pack.Flag(K);
  if(!Valid){Pack.Cancel();bDialogueOpen=false;Spoken.Empty();if(Voice)Voice->Stop();}
 }
}
void FS47Horror::Refile()
{
 if(!bEnabled || !M || !M->Story) return;
 for(const FString& Id:Pack.Documents())
 {
  if(Id!=TEXT("HR_INITIAL_REPORT"))continue; // Later SC owners release their own papers.
  if(Id==TEXT("HR_INITIAL_REPORT") && !bReport) continue;
  if(M->Story->Docs().ContainsByPredicate([&](const FS47Doc& D){return D.Id==Id;})) continue;
  const auto* D=Pack.FindDocument(Id);if(!D)continue;
  FS47Doc Doc;Doc.Id=Id;Doc.Title=D->Title;Doc.Kind=D->Kind;Doc.Stamp=Id==TEXT("HR_INITIAL_REPORT")?ReportStamp:D->Stamp;
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
 if(!R || !S || !R->ScenarioIds.Contains(TEXT("SC01"))) return false;
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
 if(Visual->Use(Id)) return true;
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
 const FS47EveningScene* S=bWorldCall?Pack.FindScene(TEXT("SC01_FIRST_REPORT")):Pack.CurrentScene();
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
 M->UI->Open(S47Kit::Shell(M->UI,Title,S47Kit::Unread(S47Kit::Col(Rows)),820),[this]
 {ObserveVoice();bDialogueOpen=false;bResumeRequested=false;Spoken.Empty();if(Voice)Voice->Stop();Changed();});
 FocusPrimary(Primary);
}
void FS47Horror::AdvanceDialogue()
{
 ObserveVoice();if(Voice)Voice->Stop();M->UI->Close(true);bDialogueOpen=false;Spoken.Empty();
 if(bWorldCall)
 {++CallCursor;const auto* S=Pack.FindScene(TEXT("SC01_FIRST_REPORT"));if(!S || CallCursor>=S->Lines.Num())FinishCall();}
 else Pack.Advance();
 Changed();if(bWorldCall || Pack.CurrentScene())ShowDialogue();
}
void FS47Horror::FinishCall()
{
 // The actual phone transaction presented these exact response lines first.
 // Only a completed contact may publish its readback and retained result document.
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
 if(!M->ModalOpen() && !M->World->bBusy && !bDialogueOpen)
 {
  if(bWorldCall || Pack.CurrentScene()){if(bResumeRequested)ShowDialogue();}
  else if(!bComplete && !Pack.Flag(TEXT("hr_h01_brief_presented")) && M->World->Area==TEXT("saro")) Trigger(TEXT("H01_BRIEF"));
  else if(!bComplete && NormalComplete() && !Pack.Flag(TEXT("hr_h01_recorded"))) Trigger(TEXT("H01_CHECK"));
  else if(!bComplete && Visual->HandSeen() && Visual->MarksChecked() && !Pack.Flag(TEXT("hr_sc01_window_check_presented"))) Trigger(TEXT("SC01_WINDOW_CHECK"));
 }
 if(!bComplete && !M->World->bDriving && !M->Evening->Opening())M->UI->Objective(Objective());
}
FString FS47Horror::Objective() const
{
 if(!bEnabled || bComplete) return FString();
 if(!M->FieldCamera || !M->FieldCamera->bHave)return TEXT("EVENING ROUND // COLLECT THE CAMERA BESIDE THE EAST DOOR");
 if(!bEast || !bCorridor)return TEXT("EVENING ROUND // TRY THE EAST DOOR AND THE ANNEX EXIT FROM INSIDE");
 if(!bBaseline)return TEXT("CONTROL ROOM // CHECK 1419.900 MHz / AZ 042 / GAIN 55 / BW 48");
 if(!Visual->LampChangeChecked())return TEXT("NORTH WINDOW // COMPARE THE MARKED LAMP ON AND OFF, THROUGH GLASS AND DIRECTLY OUTSIDE");
 if(ReferencePhoto.IsEmpty())return TEXT("NORTH WINDOW // TAKE AN INSTANT-BACK REFERENCE OF THE MARKED LAMP WHILE IT IS ON");
 if(!Visual->ScreenChecked())return TEXT("NORTH WINDOW // CLOSE THE OPAQUE SHUTTER AND CHECK THAT IT BLOCKS THE LAMP");
 if(Visual->ScreenClosed())return TEXT("NORTH WINDOW // OPEN THE SHUTTER AND CHECK THE GLASS");
 if(!Visual->MarksChecked())return Visual->MarksPresent()?TEXT("NORTH WINDOW // CHECK THE OUTSIDE MARKS WITH THE ROOM LAMP ON AND OFF"):TEXT("EVENING ROUND // CHECK THE NORTH WINDOW BEFORE LEAVING");
 if(!DoorsSecured())return TEXT("CONTROL ROOM // CLOSE THE EAST DOOR AND THE ANNEX EXIT");
 return TEXT("SHIFT FOLDER // RECORD THE EVENING CHECKS. THE TELEPHONE IS AVAILABLE TO REPORT WHAT YOU SAW");
}
TArray<FS47Task> FS47Horror::Tasks() const
{
 if(!bEnabled || bComplete)return {};
 return {{TEXT("Collect the field camera"),M->FieldCamera && M->FieldCamera->bHave},
 {TEXT("Try the east door and annex exit from inside"),bEast && bCorridor},
 {TEXT("Check the normal receiver reference"),bBaseline},
 {TEXT("Take an instant-back reference of the marked lamp"),!ReferencePhoto.IsEmpty()},
 {TEXT("Compare the lamp directly, through glass and behind the shutter"),Visual->LampChangeChecked() && Visual->ScreenChecked()},
 {TEXT("Check the window and its outside marks"),Visual->MarksChecked()}};
}
S47J::FObj FS47Horror::Save() const
{
 if(!bEnabled)return nullptr;
 auto O=S47J::New();S47J::Set(O,TEXT("schemaVersion"),1);S47J::Set(O,TEXT("contentRevision"),Revision);
 S47J::Set(O,TEXT("scenario"),bComplete?TEXT("legacyContinuation"):TEXT("SC01"));
 S47J::Set(O,TEXT("sc01Complete"),bComplete);
 S47J::Set(O,TEXT("eastTried"),bEast);S47J::Set(O,TEXT("corridorTried"),bCorridor);S47J::Set(O,TEXT("baseline"),bBaseline);
 S47J::Set(O,TEXT("referencePhoto"),ReferencePhoto);
 S47J::Set(O,TEXT("reportAcknowledged"),bReport);S47J::Set(O,TEXT("reportStamp"),ReportStamp);
 S47J::Set(O,TEXT("daleReferenceHeard"),bReferenceHeard);S47J::Set(O,TEXT("callActive"),bWorldCall);S47J::Set(O,TEXT("callCursor"),CallCursor);
 S47J::Set(O,TEXT("visual"),Visual->Save());S47J::Set(O,TEXT("pack"),Pack.Save());return O;
}
bool FS47Horror::SelfTest(TArray<FString>& Lines)
{
 FString Error;const bool Good=FS47HorrorPack::SelfTest(Error);
 Lines.Add(Good?TEXT("horror pack: all 79 scene contracts and restore gates OK"):TEXT("horror pack FAILED: ")+Error);return FS47HorrorVisual::SelfTest(Lines) && Good;
}
