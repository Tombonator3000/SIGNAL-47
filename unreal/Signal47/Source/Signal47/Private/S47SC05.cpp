#include "S47SC05.h"
#include "S47Main.h"
#include "S47Spine.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Web.h"

namespace
{
 S47J::FObj WritePaper(const FS47Doc& D)
 {
  auto O = S47J::New();
  S47J::Set(O,TEXT("id"),D.Id); S47J::Set(O,TEXT("title"),D.Title); S47J::Set(O,TEXT("kind"),D.Kind);
  S47J::Set(O,TEXT("stamp"),D.Stamp); S47J::Set(O,TEXT("page"),D.Page); S47J::Set(O,TEXT("transcript"),D.Transcript);
  S47J::Set(O,TEXT("image"),D.Image); S47J::Set(O,TEXT("paper"),D.Paper); S47J::Set(O,TEXT("ratio"),D.PaperRatio);
  TArray<TSharedPtr<FJsonValue>> Sheets;
  for (const auto& S : D.Sheets) { auto V=S47J::New(); S47J::Set(V,TEXT("page"),S.Page); S47J::Set(V,TEXT("paper"),S.Paper); S47J::Set(V,TEXT("strip"),S.Strip); S47J::Set(V,TEXT("term"),S.Term); Sheets.Add(MakeShared<FJsonValueObject>(V)); }
  O->SetArrayField(TEXT("sheets"),Sheets); return O;
 }
 FS47Doc ReadPaper(const S47J::FObj& O)
 {
  FS47Doc D; D.Id=S47J::Str(O,TEXT("id")); D.Title=S47J::Str(O,TEXT("title")); D.Kind=S47J::Str(O,TEXT("kind"));
  D.Stamp=S47J::Str(O,TEXT("stamp")); D.Page=S47J::Str(O,TEXT("page")); D.Transcript=S47J::Str(O,TEXT("transcript"));
  D.Image=S47J::Str(O,TEXT("image")); D.Paper=S47J::Str(O,TEXT("paper")); D.PaperRatio=S47J::Num(O,TEXT("ratio"));
  for (const auto& V:S47J::Arr(O,TEXT("sheets"))) { auto Q=V->AsObject(); if(Q) D.Sheets.Add({S47J::Str(Q,TEXT("page")),S47J::Str(Q,TEXT("paper")),S47J::Str(Q,TEXT("strip")),S47J::Str(Q,TEXT("term"))}); }
  return D;
 }
}

void FS47SC05::Begin(const S47J::FObj& Saved, bool bFresh)
{
 Stop(); bEnabled=bFresh; bDone=bLineDead=bRoom=bCeramic=bCompared=bEncoderRead=bMoveLogged=false;
 DeadAt=RoomAt=0; DeadGt=RoomAfter=LastGt=-1; Papers.Reset(); Notes.Reset();
 if (Saved && S47J::Int(Saved,TEXT("v"))==1)
 {
  bEnabled=true; bDone=S47J::Bool(Saved,TEXT("done")); bLineDead=S47J::Bool(Saved,TEXT("lineDead"));
  bRoom=S47J::Bool(Saved,TEXT("room")); bCeramic=S47J::Bool(Saved,TEXT("ceramic"));
  bCompared=S47J::Bool(Saved,TEXT("compared")); bEncoderRead=S47J::Bool(Saved,TEXT("encoderRead")); bMoveLogged=S47J::Bool(Saved,TEXT("moveLogged"));
  DeadAt=S47J::Num(Saved,TEXT("deadAt")); RoomAt=S47J::Num(Saved,TEXT("roomAt")); RoomAfter=S47J::Num(Saved,TEXT("roomAfter"),-1);
  Notes=S47J::Strs(Saved,TEXT("notes"));
  for (const auto& V:S47J::Arr(Saved,TEXT("papers"))) { const auto O=V->AsObject(); if(O) { FS47Doc D=ReadPaper(O); if(!D.Id.IsEmpty() && !D.Page.IsEmpty()) Papers.Add(D); } }
  if(M && M->Story && S47J::Has(Saved,TEXT("notes")))M->Story->NotesMutable()=Notes;
 }
 Refile();
}
void FS47SC05::Stop() { bEnabled=false; LastGt=-1; }
bool FS47SC05::Active() const
{
 return bEnabled && !bDone && M && M->Story && M->Spine && M->Story->ChapterKey==TEXT("prologue") && M->Spine->Gate(FS47Spine::SC04)==2;
}
S47J::FObj FS47SC05::Save() const
{
 if(!bEnabled) return nullptr;
 auto O=S47J::New(); S47J::Set(O,TEXT("v"),1); S47J::Set(O,TEXT("done"),bDone); S47J::Set(O,TEXT("lineDead"),bLineDead);
 S47J::Set(O,TEXT("room"),bRoom); S47J::Set(O,TEXT("ceramic"),bCeramic); S47J::Set(O,TEXT("compared"),bCompared); S47J::Set(O,TEXT("encoderRead"),bEncoderRead); S47J::Set(O,TEXT("moveLogged"),bMoveLogged);
 S47J::Set(O,TEXT("deadAt"),DeadAt); S47J::Set(O,TEXT("roomAt"),RoomAt); S47J::Set(O,TEXT("roomAfter"),RoomAfter); S47J::Set(O,TEXT("notes"),M && M->Story ? M->Story->Notes() : Notes);
 TArray<TSharedPtr<FJsonValue>> A; for(const auto& D:Papers) A.Add(MakeShared<FJsonValueObject>(WritePaper(D))); O->SetArrayField(TEXT("papers"),A); return O;
}
void FS47SC05::Keep(const FS47Doc& D)
{
 const int32 I=Papers.IndexOfByPredicate([&](const FS47Doc& X){return X.Id==D.Id;}); if(I<0) Papers.Add(D); else Papers[I]=D;
 if(M && M->Story) M->Story->AddDoc(D);
}
void FS47SC05::Note(const FString& Text) { Notes.AddUnique(Text); if(M && M->Story) M->Story->Note(Text); }
void FS47SC05::Refile()
{
 if(!bEnabled || !M || !M->Story) return;
 // AfterPrologue's legacy fallback describes all 27 dishes. The revised route owns only S-03 here.
 auto& N=M->Story->NotesMutable(); N.Remove(TEXT("Every dish moved to az 026, el 32. Console shows no control command.")); N.Remove(TEXT("All dishes left their scheduled track and aligned together."));
 for(const auto& D:Papers) M->Story->AddDoc(D);
 for(const auto& S:Notes) M->Story->Note(S);
}
void FS47SC05::LineDead()
{
 if(!Active()) return;
 bLineDead=true; DeadAt=M->Story->Clock(); DeadGt=M->Story->Gt();
 if(const auto* D=M->Story->Docs().FindByPredicate([](const FS47Doc& X){return X.Id==TEXT("printout");})) { const FS47Doc Copy=*D; Keep(Copy); }
}
void FS47SC05::RoomEvent()
{
 if(!Active() || !bLineDead || DeadGt<0) return;
 RoomAt=M->Story->Clock(); RoomAfter=M->Story->Gt()-DeadGt;
 bRoom=RoomAfter>=46.99 && RoomAfter<47.5;
 if(!bRoom) { UE_LOG(LogTemp,Error,TEXT("S47: SC05 actual room interval rejected: %.6f"),RoomAfter); return; }
 Note(FString::Printf(TEXT("%s. The room repeated the dead line after %.2f seconds. The printer, impact and ceramic break were reproduced in this room."),*M->Story->ClockText(),RoomAfter));
}
void FS47SC05::CeramicShattered() { if(Active() && bRoom) bCeramic=true; }
void FS47SC05::Step(double Dt)
{
 if(!Active()) { LastGt=-1; return; }
 const double Gt=M->Story->Gt(), StepGt=LastGt>=0?FMath::Clamp(Gt-LastGt,0.,.1):0.; LastGt=Gt;
 if(!bRoom || !M->Web || M->Mode!=US47Main::EMode::Play) return;
 if(!bMoveLogged) { M->Story->ConsoleLog(TEXT("S-03 ENCODER MOTION / NO CONTROL COMMAND")); bMoveLogged=true; }
 // Move the actual S-03 encoder and geometry only. The full array-following test belongs to SC08.
 if(M->Web->DishCount()>2)
 {
  const double Az=M->Web->DishAz(2), El=M->Web->DishEl(2), D=4.4*StepGt;
  M->Web->SnapDish(2,Az+FMath::Clamp(26.-Az,-D,D),El+FMath::Clamp(32.-El,-D*.6,D*.6));
 }
 MaybeFinish();
}
FString FS47SC05::Label(const FString& Id) const
{
 if(!Active()) return FString();
 if(Id==TEXT("clock") && bRoom) return TEXT("Compare the call with the room clock");
 if(Id==TEXT("crtLeft") && bRoom) return TEXT("Read S-03 against the held schedule");
 return FString();
}
bool FS47SC05::Use(const FString& Id)
{
 if(!Active() || !bRoom) return false;
 if(Id==TEXT("clock"))
 {
  if(!bCeramic || M->Web->IsMugVisible()) { M->Story->Toast(TEXT("The ceramic has not finished falling. Compare the complete sequence.")); return true; }
  bCompared=true; FS47Doc D; D.Id=TEXT("sc05_room"); D.Title=TEXT("The call and the room"); D.Kind=TEXT("hand"); D.Stamp=M->Story->ClockText();
  D.Page=FString::Printf(TEXT("SARO / ROOM CONTROL / 14 APR 1986\n\nLINE DEAD: %s MST\nROOM IMPACT: %s MST\nMEASURED INTERVAL: %.3f SECONDS\n\nOn the line: printer, impact, breath, ceramic breaking. In this room: the same order, forty-seven seconds after the line died. The mug is broken on the floor. No voice answered a new question.\n\nR. REYES"),*S47Text::ClockText(DeadAt),*S47Text::ClockText(RoomAt),RoomAfter);
  D.Transcript=TEXT("The phone carried the room forty-seven seconds before it happened. The room itself is the control."); Keep(D);
  if(M->UI) M->UI->Document(D); M->RequestAutosave(false); return true;
 }
 if(Id==TEXT("crtLeft"))
 {
  if(M->Web->DishCount()<=2 || FMath::Abs(M->Web->DishAz(2)-26.)>.05 || FMath::Abs(M->Web->DishEl(2)-32.)>.05) { M->Story->Toast(TEXT("S-03 is still moving. No control command is recorded.")); return true; }
  bEncoderRead=true; FS47Doc D; D.Id=TEXT("sc05_s03"); D.Title=TEXT("S-03 encoder check"); D.Kind=TEXT("printout"); D.Stamp=M->Story->ClockText();
  D.Page=FString::Printf(TEXT("SARO / S-03 STATUS / 14 APR 1986\n\nCALIBRATION REFERENCE: AZ 042\nACTUAL ENCODER: AZ %06.2f / EL %05.2f\nAUTO SCHEDULE: HELD\nCONTROL COMMAND FOR THIS MOTION: NONE\n\nThis is one antenna. Its direction differs from the held reference. Cause unassigned."),M->Web->DishAz(2),M->Web->DishEl(2));
  D.Transcript=TEXT("S-03 reads 026 instead of the held 042 reference. No command explains the movement. This is a question about control, not an explanation."); Keep(D);
  if(M->UI) M->UI->Document(D); M->RequestAutosave(false); return true;
 }
 return false;
}
void FS47SC05::MaybeFinish()
{
 if(!bCompared || !bEncoderRead || !bCeramic || !bRoom || (M->UI && M->UI->Modal())) return;
 const bool bPrint=Papers.ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("printout") && !D.Page.IsEmpty();});
 if(!bPrint) return;
 bDone=true; Note(TEXT("Ward is still following Ortiz's missing-person report. The call does not explain the body or Ortiz. Reyes will test whether a photograph preserves the extra reference at B-12."));
 M->Story->AdvanceClockTo(3*3600+25*60,TEXT("SC05 room comparison complete")); M->Story->CompleteRevisedCall();
}
FString FS47SC05::Objective() const
{
 if(!bRoom) return TEXT("SARO // ISOLATE THE EXISTING RESIDUAL, SECURE THE SOLVE PRINTOUT AND ANSWER THE PHONE");
 if(!bCompared) return TEXT("SARO // COMPARE THE DEAD LINE WITH THE ROOM CLOCK AND THE BROKEN MUG");
 return TEXT("SARO // CHECK S-03 ON THE STATUS TERMINAL AGAINST THE HELD 042 REFERENCE");
}
TArray<FS47Task> FS47SC05::Tasks() const
{
 return {{TEXT("Isolate the existing 1420.405 MHz residual"),M && M->Story && M->Story->Rx.Stage>=3},
  {TEXT("Secure the direction solve printout and answer the outside line"),bLineDead},
  {TEXT("Compare the actual room event with the dead line"),bCompared},
  {TEXT("Read S-03 against the held reference"),bEncoderRead}};
}
bool FS47SC05::SelfTest(TArray<FString>& Out)
{
 FS47Doc D; D.Id=TEXT("source"); D.Title=TEXT("actual"); D.Kind=TEXT("hand"); D.Stamp=TEXT("03:16:02"); D.Page=TEXT("measured source"); D.Transcript=TEXT("a"); D.Image=TEXT("b"); D.Paper=TEXT("c"); D.PaperRatio=1.47; D.Sheets.Add({TEXT("page"),TEXT("paper"),TEXT("strip"),TEXT("term")});
 const auto A=WritePaper(D), B=WritePaper(ReadPaper(A)); const bool Good=S47J::Write(A)==S47J::Write(B);
 Out.Add(FString::Printf(TEXT("%s SC05 owned paper preserves every field and sheet"),Good?TEXT("PASS"):TEXT("FAIL"))); return Good;
}
