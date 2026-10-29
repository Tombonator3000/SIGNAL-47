#include "S47SC04.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Docs.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Web.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Seen.h"
#include "S47Flow.h"
#include "S47Horror.h"
#include "S47HorrorSC03.h"
#include "S47Observing.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SC04, Log, All);

namespace
{
 constexpr double Arrive = 2 * 3600. + 55 * 60; // the gate, on the signed night axis
 constexpr double WalkSpeed = 1.15, ChaseSpeed = 2.1, SeeTime = .35, LoseTime = 2.6, Reach = .9, GrabHold = .7;
 constexpr double BodyRadius = .27; // The same measured clearance as the annex graph and ordinary Walker.
 constexpr double SightRange = 14, SightCos = .342, ListenCos = .7, LureCos = .87, WalkHear = 9, RunHear = 14;
 constexpr double ListenTime = 5, LureTime = 12, FadeTime = 1.2;
 enum : int32 { NOffice, NCorrSouth, NCorrRec, NExit, NStoop, NRamp, NWestMid, NWestNorth, NRecIn, NRecEast, NRecNE, NRecW, NRecSW, NControl };
 // Where the binder is read from, and where a capture starts the annex again.
 const FVector2D BinderStand(-2.3, 9.45);
 const FVector OfficePhone(.45, 1.1, 6.6);
 const FVector OfficeDoor(.45, 1.2, 6.9); // the wrong door: behind it is an office nobody has a key to but Ward
 double Wrap(double A) { while (A > PI) A -= 2 * PI; while (A < -PI) A += 2 * PI; return A; }
 double YawTo(const FVector2D& D) { return FMath::Atan2(D.Y, D.X); } // UE yaw from three x/z
 // The door zones of walks.saro (exitDoor, recordsDoor, southDoor): the openings between the rooms.
 struct FGap { const TCHAR* Door; double X0, X1, Z0, Z1, Plane; bool bX; };
 struct FDoorNoise { const TCHAR* Door; double Reach; };
 const FGap Gaps[] = {{TEXT("exitDoor"), -6.9, -5.0, 5.25, 6.15, -6.27, true}, {TEXT("recordsDoor"), -4.38, -3.42, 5.6, 7.7, 6.85, false},
  {TEXT("doorSouth"), -2.88, -1.92, 3.6, 5.8, 4.72, false}, {TEXT("doorEast"), 5.0, 7.1, 1.12, 2.08, 6.06, true}};
 // A door the player moves is a sound at its opening: the heavy outer doors carry, the light inner ones less.
 const double GapReach[] = {14, 7, 7, 14};
 // FS47HorrorThreat's Liang-Barsky test: does segment A-B touch the rectangle?
 bool Through(const FVector2D& A, const FVector2D& B, const FGap& R)
 {
  double T0 = 0, T1 = 1; const FVector2D D = B - A;
  auto Clip = [&T0, &T1](double P, double Q)
  {
   if (FMath::Abs(P) < 1e-12) return Q >= 0;
   const double Rt = Q / P;
   if (P < 0) { if (Rt > T1) return false; if (Rt > T0) T0 = Rt; }
   else { if (Rt < T0) return false; if (Rt < T1) T1 = Rt; }
   return true;
  };
  return Clip(-D.X, A.X - R.X0) && Clip(D.X, R.X1 - A.X) && Clip(-D.Y, A.Y - R.Z0) && Clip(D.Y, R.Z1 - A.Y);
 }
 bool CrossesDoor(const FVector2D& A, const FVector2D& Z, const FGap& G)
 {
  const double From = (G.bX ? A.X : A.Y) - G.Plane, To = (G.bX ? Z.X : Z.Y) - G.Plane;
  if ((From > 0 && To > 0) || (From < 0 && To < 0) || FMath::Abs(To - From) < 1e-8) return false;
  const double K = -From / (To - From), Across = G.bX ? FMath::Lerp(A.Y, Z.Y, K) : FMath::Lerp(A.X, Z.X, K);
  return K >= 0 && K <= 1 && Across >= (G.bX ? G.Z0 : G.X0) && Across <= (G.bX ? G.Z1 : G.X1);
 }
 S47J::FObj SourceSave(const FS47Doc& D)
 {
  auto O = S47J::New(); S47J::Set(O,TEXT("id"),D.Id); S47J::Set(O,TEXT("title"),D.Title); S47J::Set(O,TEXT("kind"),D.Kind);
  S47J::Set(O,TEXT("stamp"),D.Stamp); S47J::Set(O,TEXT("page"),D.Page); S47J::Set(O,TEXT("transcript"),D.Transcript);
  S47J::Set(O,TEXT("image"),D.Image); S47J::Set(O,TEXT("paper"),D.Paper); S47J::Set(O,TEXT("paperRatio"),D.PaperRatio);
  TArray<TSharedPtr<FJsonValue>> Sheets;
  for (const auto& S : D.Sheets)
  {
   auto Row = S47J::New(); S47J::Set(Row,TEXT("page"),S.Page); S47J::Set(Row,TEXT("paper"),S.Paper);
   S47J::Set(Row,TEXT("strip"),S.Strip); S47J::Set(Row,TEXT("term"),S.Term); Sheets.Add(MakeShared<FJsonValueObject>(Row));
  }
  O->SetArrayField(TEXT("sheets"),Sheets); return O;
 }
 FS47Doc SourceLoad(const S47J::FObj& O)
 {
  FS47Doc D; D.Id=S47J::Str(O,TEXT("id")); D.Title=S47J::Str(O,TEXT("title")); D.Kind=S47J::Str(O,TEXT("kind"));
  D.Stamp=S47J::Str(O,TEXT("stamp")); D.Page=S47J::Str(O,TEXT("page")); D.Transcript=S47J::Str(O,TEXT("transcript"));
  D.Image=S47J::Str(O,TEXT("image")); D.Paper=S47J::Str(O,TEXT("paper")); D.PaperRatio=S47J::Num(O,TEXT("paperRatio"));
  for (const auto& V:S47J::Arr(O,TEXT("sheets"))) if (V && V->Type==EJson::Object)
  {
   const auto S=V->AsObject(); D.Sheets.Add({S47J::Str(S,TEXT("page")),S47J::Str(S,TEXT("paper")),S47J::Str(S,TEXT("strip")),S47J::Str(S,TEXT("term"))});
  }
  return D;
 }
}

const TArray<FS47SC04::FNode>& FS47SC04::Nodes()
{
 static const TArray<FNode> N = {
  {TEXT("office"), FVector2D(.4, 5.75)}, {TEXT("corridorSouth"), FVector2D(-2.4, 6.05)}, {TEXT("corridorRecords"), FVector2D(-3.9, 6.05)},
  {TEXT("exit"), FVector2D(-5.3, 5.7)}, {TEXT("stoop"), FVector2D(-7.0, 5.7)}, {TEXT("rampEnd"), FVector2D(-11.1, 5.7)},
  {TEXT("westMid"), FVector2D(-11.1, 0.0)}, {TEXT("westNorth"), FVector2D(-9.0, -7.5)},
  {TEXT("recordsIn"), FVector2D(-3.9, 7.5)}, {TEXT("recordsEast"), FVector2D(-2.3, 7.6)}, {TEXT("recordsBinder"), FVector2D(-2.2, 9.6)},
  {TEXT("recordsWest"), FVector2D(-5.0, 7.7)}, {TEXT("recordsBox"), FVector2D(-5.0, 9.2)}, {TEXT("controlDoor"), FVector2D(-2.4, 3.6)}};
 return N;
}

const TArray<TPair<int32, int32>>& FS47SC04::Edges()
{
 // Measured free (walks.saro zones less colliders, 0.27 m clearance); the ramp is left past its rails.
 static const TArray<TPair<int32, int32>> E = {{NOffice, NCorrSouth}, {NCorrSouth, NCorrRec}, {NCorrRec, NExit}, {NExit, NStoop}, {NStoop, NRamp},
  {NRamp, NWestMid}, {NWestMid, NWestNorth}, {NCorrRec, NRecIn}, {NRecIn, NRecEast}, {NRecEast, NRecNE}, {NRecIn, NRecW}, {NRecW, NRecSW}, {NCorrSouth, NControl}};
 return E;
}

const TCHAR* FS47SC04::BodyName(EBody In)
{
 switch (In)
 {
 case EBody::Away: return TEXT("away"); case EBody::Door: return TEXT("door"); case EBody::Enter: return TEXT("enter");
 case EBody::Search: return TEXT("search"); case EBody::Listen: return TEXT("listen"); case EBody::Lured: return TEXT("lured");
 case EBody::Chase: return TEXT("chase"); case EBody::Grab: return TEXT("grab"); default: return TEXT("leave");
 }
}

int32 FS47SC04::Room(const FVector2D& At)
{
 if (At.Y < 4.6 && At.Y > -4.4 && At.X > -5.9 && At.X < 5.9) return 0;
 if (At.Y >= 4.6 && At.Y < 6.6 && At.X > -6.0 && At.X < 2.0) return 1;
 if (At.Y >= 6.6 && At.Y < 10.5 && At.X > -5.9 && At.X < -1.2) return 2;
 return 3;
}

// ---------- saving ----------

void FS47SC04::Begin(const S47J::FObj& Saved)
{
 Stop();
 P = EStep::Drive; F = FFacts(); SourcesVersion = 1; SourcePapers.Reset();
 if (!Saved) return;
 P = EStep(FMath::Clamp(S47J::Int(Saved, TEXT("step")), 0, int32(EStep::Done)));
 F.bSchedule = S47J::Bool(Saved, TEXT("schedule")); F.bStrip = S47J::Bool(Saved, TEXT("strip"));
 F.bJournal = S47J::Bool(Saved, TEXT("journal")); F.bBinder = S47J::Bool(Saved, TEXT("binder"));
 F.bEscaped = S47J::Bool(Saved, TEXT("escaped")); F.bLure = S47J::Bool(Saved, TEXT("lure"));
 F.Captures = FMath::Max(0, S47J::Int(Saved, TEXT("captures")));
 SourcesVersion = S47J::Int(Saved,TEXT("sourcesVersion"),0);
 if (SourcesVersion == 0 || SourcesVersion == 1) for (const auto& V:S47J::Arr(Saved,TEXT("sources"))) if (V && V->Type==EJson::Object)
 {
  FS47Doc D=SourceLoad(V->AsObject());
  if (!D.Id.IsEmpty() && !SourcePapers.ContainsByPredicate([&](const FS47Doc& Pp){return Pp.Id==D.Id;})) SourcePapers.Add(MoveTemp(D));
 }
 Refile();
 // A meeting is never saved half run: a load during it starts the annex again at the binder.
 if (P == EStep::Hunt) bRestartHunt = true;
}

S47J::FObj FS47SC04::Save() const
{
 auto O = S47J::New();
 S47J::Set(O, TEXT("step"), int32(P));
 S47J::Set(O, TEXT("schedule"), F.bSchedule); S47J::Set(O, TEXT("strip"), F.bStrip); S47J::Set(O, TEXT("journal"), F.bJournal);
 S47J::Set(O, TEXT("binder"), F.bBinder); S47J::Set(O, TEXT("escaped"), F.bEscaped); S47J::Set(O, TEXT("lure"), F.bLure);
 S47J::Set(O, TEXT("captures"), F.Captures);
 S47J::Set(O,TEXT("sourcesVersion"),SourcesVersion);
 TArray<TSharedPtr<FJsonValue>> Sources;
 for (const auto& D:SourcePapers) Sources.Add(MakeShared<FJsonValueObject>(SourceSave(D)));
 O->SetArrayField(TEXT("sources"),Sources);
 return O;
}

void FS47SC04::Stop()
{
 B = EBody::Away; bShown = false; Path.Reset(); bLureRinging = false; FadeT = -1; bRestartHunt = false;
 bHavePlayer = false; bBodyUnavailable = false; BlockedDoor.Reset();
 Pale.Apply(false, FVector::ZeroVector, 0, 0, 0, 0, 0, FS47PaleBody::EPose::Walk);
}

void FS47SC04::KeepSources()
{
 if (!M || !M->Story) return;
 // Own exact copies of the actual transfer folder, not freshly reconstructed templates.
 // This includes earlier acquired logs/charts with their real stamps and image/sheet payloads.
 for (const FS47Doc& D:M->Story->Docs())
  if (!D.Id.IsEmpty() && !SourcePapers.ContainsByPredicate([&](const FS47Doc& Pp){return Pp.Id==D.Id;})) SourcePapers.Add(D);
}

void FS47SC04::Refile()
{
 if (M && M->Story) for (const FS47Doc& D:SourcePapers) M->Story->AddDoc(D);
}

bool FS47SC04::SourcesReady() const
{
 if (SourcesVersion == 0) return true; // Explicit compatibility: old flags never fabricate missing papers.
 return SourcesVersion == 1
  && SourcePapers.ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("sc04_schedule") && !D.Page.IsEmpty();})
  && SourcePapers.ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("sc04_strip") && !D.Page.IsEmpty();});
}

void FS47SC04::SetStep(EStep Next)
{
 if (P == Next) return;
 UE_LOG(LogS47SC04, Display, TEXT("S47: SC04 step %d -> %d"), int32(P), int32(Next));
 P = Next;
 if (M) M->RequestAutosave(false);
}

// ---------- the originals ----------

FS47Doc FS47SC04::ScheduleDoc()
{
 FS47Doc D; D.Id = TEXT("sc04_schedule"); D.Title = TEXT("Automatic schedule, held"); D.Kind = TEXT("typed"); D.Stamp = TEXT("AUTO HOLD");
 D.Page = TEXT("SARO OPS / AUTOMATIC SCHEDULE\n\nSERIES 06:00 SURVEY ........ HELD\nAUTO SLEW ................... HELD\nAUTO PRINT .................. HELD\nRECEIVER .................... ON / PASSIVE\nSTRIP RECORDER .............. ON\n\nHOLD KEYED AT THE OPS TERMINAL BY: R. REYES\nREASON: MISSING-PERSON REPORT, CALICHE. NOTHING RUNS UNATTENDED.");
 D.Transcript = TEXT("The automatic schedule is held at the ops terminal. The survey, the slews and the printer stop; the receiver stays on, passive, and the strip recorder keeps writing.");
 return D;
}

FS47Doc FS47SC04::StripDoc(const FString& Taken)
{
 const FString When = Taken.IsEmpty() ? FString(TEXT("02:31")) : Taken;
 FS47Doc D; D.Id = TEXT("sc04_strip"); D.Title = TEXT("Receiver strip chart, 00:00 to 02:58"); D.Kind = TEXT("typed"); D.Stamp = TEXT("STRIP / RX 3");
 D.Page = FString::Printf(TEXT("RECEIVER STRIP / RX BANK 3 / 13-14 APRIL\n\n00:13  pen to zero. Feeder out, K3 open.\n01:52  pen back. Power restored from T01.\n%s  pen to the stop for forty seconds. No slew, no command, nothing on the log.\n02:58  torn off by R. Reyes for the county.\n\nPencil, in the margin: same minute as the fuel stop."), *When);
 D.Transcript = FString::Printf(TEXT("Tonight's strip off the receiver. The pen drops to zero at the outage, comes back when T01 is restored, and at %s it goes to the stop for forty seconds with nothing on the command log. The same minute Ortiz was taken."), *When);
 return D;
}

FString FS47SC04::Label(const FString& Id) const
{
 const bool bWork = P == EStep::Collect || P == EStep::Hunt || P == EStep::Calm;
 if (!bWork) return FString();
 if (Id == TEXT("opsTerminal") && !F.bSchedule) return TEXT("Hold the automatic schedule");
 if (Id == TEXT("printer") && !F.bStrip) return TEXT("Tear off tonight's strip chart");
 if (Id == TEXT("logbook") && !F.bJournal) return TEXT("Take the shift journal for the report");
 if (Id == TEXT("binder") && !F.bBinder && P == EStep::Collect) return TEXT("Take the run binder for the report");
 if (Id == TEXT("recPhone") && P == EStep::Hunt && !bLureRinging && B != EBody::Chase && B != EBody::Grab && B != EBody::Away) return TEXT("Ring the operations office");
 return FString();
}

bool FS47SC04::Use(const FString& Id)
{
 if (Label(Id).IsEmpty() || !M || !M->Story || !M->UI) return false;
 AS47Story& S = *M->Story;
 if (Id == TEXT("opsTerminal"))
 {
  F.bSchedule = true; S.AddDoc(ScheduleDoc());
  if (M->Audio) M->Audio->Play(TEXT("click"), .5f);
  M->UI->Toast(TEXT("AUTO HOLD. The survey, the slews and the printer stop. The receiver stays on, passive."), 4);
  S.Note(FString::Printf(TEXT("%s. Held the automatic schedule at the ops terminal. Nothing runs by itself now; the receiver listens, passive."), *S.ClockText(false)));
 }
 else if (Id == TEXT("printer"))
 {
  F.bStrip = true;
  const FString Taken = M->Horror && M->Horror->SC03 ? M->Horror->SC03->Facts().TakenStamp : FString();
  const FS47Doc D = StripDoc(Taken); S.AddDoc(D);
  if (M->Audio) M->Audio->Play(TEXT("paper0"), .5f);
  M->UI->Document(D);
  S.Note(FString::Printf(TEXT("%s. Tore tonight's strip off the receiver. At %s the pen went to the stop for forty seconds. Nothing on the command log."), *S.ClockText(false), Taken.IsEmpty() ? TEXT("02:31") : *Taken));
 }
 else if (Id == TEXT("logbook"))
 {
  F.bJournal = true;
  if (M->Audio) M->Audio->Play(TEXT("paper0"), .45f);
  M->UI->Toast(TEXT("The shift journal and the T01 journal from the junction, together, for the county."), 4);
  S.Note(FString::Printf(TEXT("%s. Took the shift journal. The T01 journal from the junction goes with it."), *S.ClockText(false)));
 }
 else if (Id == TEXT("binder"))
 {
  F.bBinder = true;
  if (M->Audio) M->Audio->Play(TEXT("paper0"), .5f);
  M->UI->Toast(TEXT("The run binder, the evening's charts behind the April tab, where you filed them at 20:30."), 4);
  S.Note(FString::Printf(TEXT("%s. Took the run binder from the records room for the county."), *S.ClockText(false)));
  StartHunt(false);
 }
 else if (Id == TEXT("recPhone"))
 {
  F.bLure = true; bLureRinging = true; LureT = 0;
  if (M->Audio)
  {
   M->Audio->Play(TEXT("click"), .4f);
   // Four rings behind the office door, dulled by it (FS47ArchivePhone::Ring's through-the-wall ring).
   for (int32 I = 0; I < 4; ++I) { FS47Sound O; O.Gain = .9f; O.Lp = 1800.f; O.At = OfficePhone; O.When = 1.f + I * 3.2f; O.Dur = 2.4f; M->Audio->Sfx(TEXT("phoneRing"), O); }
  }
  M->UI->Toast(TEXT("You dial the operations office. Down the corridor, behind its door, the phone starts ringing."), 4);
  UE_LOG(LogS47SC04, Display, TEXT("S47: SC04 lure: the operations office rings"));
 }
 KeepSources(); M->RequestAutosave(false);
 return true;
}

FString FS47SC04::Objective() const
{
 switch (P)
 {
 case EStep::Drive: return TEXT("SC04 // DRIVE BACK TO SARO. THE DEPUTY NEEDS THE RECEIVER LOG FOR THE MISSING-PERSON REPORT");
 case EStep::Hunt: return TEXT("ANNEX // SOMETHING CAME OUT OF THE OPERATIONS OFFICE. GET TO THE CONTROL ROOM AND SHUT ITS DOORS");
 default: break;
 }
 if (P == EStep::Calm && F.bSchedule && F.bStrip && F.bJournal && F.bBinder) return TEXT("CONTROL ROOM // SHUT THE CORRIDOR AND SERVICE DOORS. STAY IN COVER");
 TArray<FString> Left;
 if (!F.bSchedule) Left.Add(TEXT("HOLD THE SCHEDULE AT THE OPS TERMINAL"));
 if (!F.bStrip) Left.Add(TEXT("TEAR OFF THE STRIP CHART"));
 if (!F.bJournal) Left.Add(TEXT("TAKE THE SHIFT JOURNAL"));
 if (!F.bBinder) Left.Add(TEXT("FETCH THE RUN BINDER FROM THE RECORDS ROOM"));
 return FString(TEXT("SARO // SECURE THE ORIGINALS: ")) + FString::Join(Left, TEXT(", "));
}

TArray<FS47Task> FS47SC04::Tasks() const
{
 TArray<FS47Task> Out;
 Out.Add({ TEXT("Drive back to SARO with the deputy behind you"), P != EStep::Drive });
 if (P == EStep::Drive) return Out;
 Out.Add({ TEXT("Hold the automatic schedule at the ops terminal"), F.bSchedule });
 Out.Add({ TEXT("Tear off tonight's strip chart at the printer"), F.bStrip });
 Out.Add({ TEXT("Take the shift journal from the desk"), F.bJournal });
 Out.Add({ TEXT("Fetch the run binder from the records room"), F.bBinder });
 if (F.bBinder) Out.Add({ TEXT("Get to the control room and shut its doors"), F.bEscaped });
 return Out;
}

// ---------- the step ----------

bool FS47SC04::Step(double Dt)
{
 if (!M || !M->Story || !M->World) return false;
 AS47Story& S = *M->Story;
 const bool bPlay = M->Mode == US47Main::EMode::Play && !M->bPausedByMenu;
 if (FadeT >= 0 && bPlay)
 {
  FadeT += Dt;
  if (FadeT >= FadeTime) { FadeT = -1; if (M->UI) M->UI->Fade(false); }
 }
 if (bRestartHunt && M->World->Area == TEXT("saro") && M->Player && bPlay) { bRestartHunt = false; StartHunt(true); }
 switch (P)
 {
 case EStep::Drive:
  if (M->World->Area == TEXT("saro") && !M->World->bDriving && bPlay)
  {
   S.SetClock(FMath::Max(S.Clock(), Arrive));
   M->UI->Toast(TEXT("At the gate the deputy's radio cuts in: something at Ortiz's wagon. He swings round. \"Get me that log, Reyes. Lock the door behind you.\""), 7);
   S.Note(FString::Printf(TEXT("%s. Back at SARO. The deputy followed me to the gate and was called back to Ortiz's wagon. Going in alone for the originals."), *S.ClockText(false)));
   KeepSources();
   SetStep(EStep::Collect);
  }
  break;
 case EStep::Hunt: StepHunt(Dt); break;
 case EStep::Calm:
  if (bPlay && F.bSchedule && F.bStrip && F.bJournal && F.bBinder && SourcesReady() && PlayerSafe()) { Finish(); return true; }
  break;
 case EStep::Done: return SourcesReady();
 default: break;
 }
 return P == EStep::Done && SourcesReady();
}

void FS47SC04::Finish()
{
 AS47Story& S = *M->Story;
 if (M->Web) M->Web->SetDoorOpen(TEXT("exitDoor"), false, false);
 M->UI->Toast(TEXT("The phone. Ward: \"Stay in the control room, Reyes. The series is stopped. Leave the receiver on, passive. Nothing runs by itself now.\""), 7);
 S.Note(FString::Printf(TEXT("%s. Locked in the control room with the originals. Ward on the line: the series is stopped. Staying in cover while the deputy is busy; the receiver stays on, passive."), *S.ClockText(false)));
 SetStep(EStep::Done);
 UE_LOG(LogS47SC04, Display, TEXT("S47: SC04 complete; captures %d, lure %d"), F.Captures, F.bLure ? 1 : 0);
}

bool FS47SC04::PlayerInControl() const
{
 return M && M->Player && M->World && M->World->Area == TEXT("saro") && Room(FVector2D(M->Player->Pos.X, M->Player->Pos.Z)) == 0;
}

bool FS47SC04::PlayerSafe() const
{
 return PlayerInControl() && M->Web && !M->Web->IsDoorOpen(TEXT("doorSouth")) && !M->Web->IsDoorOpen(TEXT("doorEast"));
}

// ---------- the annex meeting ----------

void FS47SC04::StartHunt(bool bRetry)
{
 SetStep(EStep::Hunt);
 T = bRetry ? 1.0 : 0; bVoice = false; bBanged = false; bListened = false; bLureRinging = false; SeeAcc = LostT = 0; Patrol = 0;
 Pos = Nodes()[NOffice].At; Y = 0; BodyYaw = HeadYaw = PI; Gait = 0; Path.Reset(); PendingNoise.Reset(); LastDoors.Reset();
 Enter(EBody::Away);
 UE_LOG(LogS47SC04, Display, TEXT("S47: SC04 annex meeting %s"), bRetry ? TEXT("starts again at the binder") : TEXT("starts"));
}

void FS47SC04::Enter(EBody Next)
{
 if (B != Next) UE_LOG(LogS47SC04, Display, TEXT("S47: SC04 body %s -> %s at %.2f %.2f"), BodyName(B), BodyName(Next), Pos.X, Pos.Y);
 B = Next; StateT = 0; Path.Reset();
}

bool FS47SC04::EdgeOpen(int32 A, int32 Bn) const
{
 const auto Is = [A, Bn](int32 X, int32 Z) { return (A == X && Bn == Z) || (A == Z && Bn == X); };
 if (!M || !M->Web) return true;
 if (Is(NExit, NStoop)) return M->Web->IsDoorOpen(TEXT("exitDoor"));
 if (Is(NCorrRec, NRecIn)) return M->Web->IsDoorOpen(TEXT("recordsDoor"));
 if (Is(NCorrSouth, NControl)) return M->Web->IsDoorOpen(TEXT("doorSouth"));
 return true;
}

int32 FS47SC04::Nearest(const FVector2D& At) const
{
 int32 Best = NCorrRec; double D = 1e9;
 const int32 R = Room(At);
 for (int32 I = 0; I < Nodes().Num(); ++I)
 {
  const double Dist = FVector2D::Distance(Nodes()[I].At, At) + (Room(Nodes()[I].At) == R ? 0 : 50);
  if (Dist < D) { D = Dist; Best = I; }
 }
 return Best;
}

void FS47SC04::RouteTo(int32 Goal)
{
 // Breadth-first over the open edges, from the node nearest the body.
 Path.Reset();
 const int32 Start = Nearest(Pos);
 TArray<int32> Prev; Prev.Init(-2, Nodes().Num());
 TArray<int32> Q = {Start}; Prev[Start] = -1;
 for (int32 H = 0; H < Q.Num(); ++H)
  for (const auto& E : Edges())
  {
   const int32 A = Q[H], N = E.Key == A ? E.Value : E.Value == A ? E.Key : -1;
   if (N < 0 || Prev[N] != -2 || !EdgeOpen(A, N)) continue;
   Prev[N] = A; Q.Add(N);
  }
 if (Prev[Goal] == -2) return;
 for (int32 N = Goal; N >= 0; N = Prev[N]) Path.Insert(N, 0);
 if (Path.Num() > 1 && FVector2D::Distance(Pos, Nodes()[Path[0]].At) < .2) Path.RemoveAt(0);
}

bool FS47SC04::Walk(double Speed, double Dt)
{
 if (Path.IsEmpty()) { Gait = FMath::Max(0.0, Gait - Dt * 3); return true; }
 const FVector2D To = Nodes()[Path[0]].At, D = To - Pos;
 BlockedDoor = ClosedDoorTo(To);
 if (!BlockedDoor.IsEmpty())
 {
  // The player can close an edge after RouteTo. Do not consume even one step through that leaf.
  BodyYaw = HeadYaw = YawTo(D); Gait = FMath::Max(0.0,Gait-Dt*3);
  UE_LOG(LogS47SC04,Display,TEXT("S47: SC04 route stopped at newly closed %s"),*BlockedDoor);
  Enter(EBody::Listen); return false;
 }
 const double Dist = D.Size(), StepLen = Speed * Dt;
 const FVector2D Candidate = Dist > StepLen ? Pos + D / Dist * StepLen : To;
 if (!ClearWalk(Pos,Candidate))
 {
  Gait=FMath::Max(0.0,Gait-Dt*3); Enter(EBody::Listen);
  UE_LOG(LogS47SC04,Display,TEXT("S47: SC04 route stopped by active scene collider or walk-zone boundary")); return false;
 }
 BodyYaw = YawTo(D); HeadYaw = BodyYaw;
 Gait = FMath::Min(1.0, Gait + Dt * 3); Stride += StepLen * 2.1;
 if (Dist <= StepLen) { Pos = To; Path.RemoveAt(0); return Path.IsEmpty(); }
 Pos += D / Dist * StepLen;
 return false;
}

FString FS47SC04::ClosedDoorTo(const FVector2D& Target) const
{
 if (!M || !M->Web) return FString();
 for (const auto& G:Gaps) if (!M->Web->IsDoorOpen(G.Door) && CrossesDoor(Pos,Target,G)) return G.Door;
 return FString();
}

bool FS47SC04::ClearWalk(const FVector2D& From, const FVector2D& To) const
{
 if (!M || !M->Web || !M->Player) return false;
 // Walkable checks zones, not furniture. Clip the complete swept footprint against the
 // CURRENT colliders (including opened leaves), so even a long frame cannot cross a thin prop.
 for (const auto& C:M->Web->Colliders()) if (C.bEnabled)
 {
  const FGap Expanded={nullptr,C.MinX-BodyRadius,C.MaxX+BodyRadius,C.MinZ-BodyRadius,C.MaxZ+BodyRadius,0,false};
  if (Through(From,To,Expanded)) return false;
 }
 const int32 Samples=FMath::Clamp(FMath::CeilToInt(FVector2D::Distance(From,To)/.05),1,64);
 for (int32 I=0;I<=Samples;++I)
 {
  const FVector2D At=FMath::Lerp(From,To,double(I)/Samples);
  if (!M->Player->Walkable(At.X,At.Y)) return false;
 }
 return true;
}

void FS47SC04::LogSight() const
{
 if (!M || !M->Player || !M->Web || !M->Web->GetWorld()) return;
 const FS47Cam Cam = FS47Cam::Of(M->Player);
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC04SightLog), true);
 for (const auto& C : Pale.All) if (const UStaticMeshComponent* Part = C.Get()) Q.AddIgnoredComponent(Part);
 TArray<FHitResult> Hits;
 M->Web->GetWorld()->LineTraceMultiByChannel(Hits, S47Three::ToUE(Eye()), S47Three::ToUE(Cam.Pos), ECC_Visibility, Q);
 FString List;
 for (const FHitResult& H : Hits) if (const UPrimitiveComponent* C = H.GetComponent())
 {
  const auto* Node = M->Web->Node(M->Web->NodeOf(C));
  List += FString::Printf(TEXT(" [%s node %s vis %d at %.0fcm]"), *C->GetName(), Node ? *Node->Name : TEXT("-"), C->IsVisible(), H.Distance);
 }
 UE_LOG(LogS47SC04, Display, TEXT("S47: SC04 sight from %s to player %s:%s"), *Eye().ToString(), *Cam.Pos.ToString(), List.IsEmpty() ? TEXT(" clear") : *List);
}

bool FS47SC04::SeesPlayer() const
{
 if (!M || !M->Player || !M->Web || !M->Web->GetWorld() || !bShown) return false;
 const FS47Cam Cam = FS47Cam::Of(M->Player);
 const FVector E = Eye();
 const FVector2D D(Cam.Pos.X - E.X, Cam.Pos.Z - E.Z);
 const double Dist = D.Size();
 if (Dist > SightRange) return false;
 // Between rooms only through an open doorway the line actually crosses: the exported walls have
 // gaps at some jambs that a trace slips through, and no player would call that being seen.
 const FVector2D From(E.X, E.Z), To(Cam.Pos.X, Cam.Pos.Z);
 const int32 RA = Room(From), RB = Room(To);
 if (RA != RB)
 {
  const auto Open = [this, &From, &To](int32 G) { return M->Web->IsDoorOpen(Gaps[G].Door) && Through(From, To, Gaps[G]); };
  const auto Pair = [RA, RB](int32 X, int32 Z) { return (RA == X && RB == Z) || (RA == Z && RB == X); };
  const bool bOk = (Pair(1, 3) && Open(0)) || (Pair(1, 2) && Open(1)) || (Pair(0, 1) && Open(2)) || (Pair(0, 2) && Open(1) && Open(2)) || (Pair(0, 3) && Open(3));
  if (!bOk) return false;
 }
 const double Cone = B == EBody::Lured ? LureCos : B == EBody::Listen ? ListenCos : SightCos; // a ringing phone holds its attention ahead
 if (Dist > .4 && FVector2D::DotProduct(FVector2D(FMath::Cos(HeadYaw), FMath::Sin(HeadYaw)), D / Dist) < Cone) return false;
 // FS47HorrorThreat::ClearLine's rule: visible solid surfaces stop sight; proxies and see-through materials do not.
 AS47Web* W = M->Web.Get();
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC04Sight), true);
 for (const auto& C : Pale.All) if (const UStaticMeshComponent* Part = C.Get()) Q.AddIgnoredComponent(Part);
 const FVector A = S47Three::ToUE(E), Bp = S47Three::ToUE(Cam.Pos);
 for (int32 I = 0; I < 16; ++I)
 {
  FHitResult H;
  if (!W->GetWorld()->LineTraceSingleByChannel(H, A, Bp, ECC_Visibility, Q)) return true;
  const UPrimitiveComponent* C = H.GetComponent(); if (!C) return false;
  bool Solid = C->IsVisible();
  const auto* Node = W->Node(W->NodeOf(C));
  if (Node && Node->Name.StartsWith(TEXT("proxy"))) Solid = false;
  if (const auto* Mat = C->GetMaterial(FMath::Max<int32>(0, H.ElementIndex)))
  {
   float Opacity = 1; Mat->GetScalarParameterValue(TEXT("Opacity"), Opacity);
   if (Mat->GetBlendMode() == BLEND_Additive || (Mat->GetBlendMode() == BLEND_Translucent && Opacity < .5f)) Solid = false;
  }
  if (Solid) return false;
  Q.AddIgnoredComponent(C);
 }
 return false;
}

TOptional<FVector2D> FS47SC04::Heard(double Dt)
{
 const FVector2D Pl(M->Player->Pos.X, M->Player->Pos.Z);
 const double Speed = bHavePlayer ? FVector2D::Distance(Pl, LastPlayer) / FMath::Max(Dt, 1e-3) : 0;
 LastPlayer = Pl; bHavePlayer = true; NoiseCool -= Dt;
 if (Speed < .4 || Speed > 12 || NoiseCool > 0) return {}; // still, or placed
 // A shut door takes most of a footstep: each closed door between the rooms divides the reach by three.
 const int32 A = Room(Pos), Bp = Room(Pl);
 int32 Shut = 0;
 const auto Closed = [this](const TCHAR* Door) { return M->Web && !M->Web->IsDoorOpen(Door); };
 const auto Between = [A, Bp](int32 X, int32 Z) { return (A == X && Bp == Z) || (A == Z && Bp == X); };
 if (Between(1, 2) && Closed(TEXT("recordsDoor"))) ++Shut;
 if (Between(0, 1) && Closed(TEXT("doorSouth"))) ++Shut;
 if (Between(0, 2)) Shut += int32(Closed(TEXT("recordsDoor"))) + int32(Closed(TEXT("doorSouth")));
 if (Between(1, 3) && Closed(TEXT("exitDoor"))) ++Shut;
 if (Between(0, 3) && Closed(TEXT("doorEast"))) ++Shut;
 if (Between(2, 3)) Shut += int32(Closed(TEXT("recordsDoor"))) + int32(Closed(TEXT("exitDoor")));
 const double Range = (Speed > 2.5 ? RunHear : WalkHear) / FMath::Pow(3.0, Shut);
 if (FVector2D::Distance(Pl, Pos) > Range) return {};
 NoiseCool = .5;
 return Pl;
}

double FS47SC04::Ground() const
{
 // Indoors the floor is level at zero; outside the stoop and the ramp step down to the yard.
 if (Room(Pos) != 3 || !M || !M->Web || !M->Web->GetWorld()) return 0;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC04Ground), true);
 for (const auto& C : Pale.All) if (const UStaticMeshComponent* Part = C.Get()) Q.AddIgnoredComponent(Part);
 FHitResult H;
 if (!M->Web->GetWorld()->LineTraceSingleByChannel(H, S47Three::ToUE(Pos.X, Y + 1.5, Pos.Y), S47Three::ToUE(Pos.X, Y - 3, Pos.Y), ECC_Visibility, Q)) return Y;
 return S47Three::ToThree(H.ImpactPoint).Y;
}

void FS47SC04::Capture()
{
 ++F.Captures;
 if (M->UI) M->UI->Fade(true);
 FadeT = 0;
 if (M->Audio) M->Audio->Gasp();
 if (M->Player) { M->Player->Place(BinderStand.X, BinderStand.Y, FMath::Atan2(1.6, 2.6)); M->Player->Pitch = -.06; }
 if (M->Web) M->Web->SetDoorOpen(TEXT("recordsDoor"), true, true);
 UE_LOG(LogS47SC04, Display, TEXT("S47: SC04 capture %d; the annex starts again at the binder, originals kept, clock unchanged"), F.Captures);
 StartHunt(true);
 M->RequestAutosave(false);
}

void FS47SC04::Sounds(double Before)
{
 if (!M->Audio || !bShown) return;
 if (FMath::FloorToDouble(Stride / PI) != FMath::FloorToDouble(Before / PI))
 {
  const auto Pick = M->Audio->StepPick(Room(Pos) == 3 ? TEXT("concrete") : TEXT("tile"));
  FS47Sound O; O.Gain = Pick.Gain * (B == EBody::Chase ? 1.f : .75f); O.At = FVector(Pos.X, Y + .05, Pos.Y);
  M->Audio->Sfx(Pick.Name, O);
 }
}

void FS47SC04::StepHunt(double Dt)
{
 const bool bHeld = !M->Player || M->Mode != US47Main::EMode::Play || M->bPausedByMenu || M->ModalOpen() || M->Story->IsCinematic() || FadeT >= 0 ||
  M->World->Area != TEXT("saro") || (M->Flow && M->Flow->bStarting);
 if (!Pale.bBuilt && !Pale.Build(M->Web.Get(), TEXT("room.group"), TEXT("SC04Pale")))
 {
  if (!bBodyUnavailable) UE_LOG(LogS47SC04,Error,TEXT("S47: SC04 body unavailable; encounter remains incomplete, no escape credited"));
  bBodyUnavailable = true; bShown = false; return;
 }
 bBodyUnavailable = false;
 if (!bHeld)
 {
  T += Dt; Clock += Dt;
  const double Before = Stride;
  Think(Dt);
  if (bShown) Y = Ground();
  Sounds(Before);
 }
 Pale.Apply(bShown, FVector(Pos.X, Y, Pos.Y), BodyYaw, HeadYaw, Gait, Stride, Clock,
  B == EBody::Chase ? FS47PaleBody::EPose::Chase : B == EBody::Grab ? FS47PaleBody::EPose::Grab : B == EBody::Listen || B == EBody::Lured ? FS47PaleBody::EPose::Listen : FS47PaleBody::EPose::Walk);
}

void FS47SC04::Think(double Dt)
{
 StateT += Dt;
 const FVector2D Pl(M->Player->Pos.X, M->Player->Pos.Z);
 if (bLureRinging) { LureT += Dt; if (LureT >= LureTime) bLureRinging = false; }
 // Dale's door fragment, word for word, from behind the operations office: the wrong door.
 if (!bVoice && T >= 1.5)
 {
  bVoice = true;
  if (M->Observing) M->Observing->ReplayDoorFragment(OfficeDoor);
  M->UI->Toast(TEXT("From behind the operations office door, Dale's voice, the same words and the same pause: \"Reyes? Dale. The logbook should be in front of you.\" Only Ward has a key to that office."), 7);
 }
 // A door the player moves is heard at its opening; before the body is in, it remembers where.
 TOptional<FVector2D> DoorNoise;
 for (int32 G = 0; G < UE_ARRAY_COUNT(Gaps); ++G)
 {
  const bool bOpen = M->Web->IsDoorOpen(Gaps[G].Door);
  const bool* Was = LastDoors.Find(Gaps[G].Door);
  if (Was && *Was != bOpen)
  {
   const FVector2D At((Gaps[G].X0 + Gaps[G].X1) * .5, (Gaps[G].Z0 + Gaps[G].Z1) * .5);
   if (!bShown) PendingNoise = At;
   else if (FVector2D::Distance(At, Pos) <= GapReach[G]) DoorNoise = At;
  }
  LastDoors.Add(Gaps[G].Door, bOpen);
 }
 const bool bSees = bShown && SeesPlayer();
 SeeAcc = bSees ? SeeAcc + Dt : FMath::Max(0.0, SeeAcc - Dt);
 TOptional<FVector2D> Sound = bShown ? Heard(Dt) : TOptional<FVector2D>();
 if (DoorNoise) Sound = DoorNoise;
 if (bSees) { LastKnown = Pl; LostT = 0; }
 const bool bSafe = PlayerSafe();
 // The patrol from the office: the corridor, the records room if its door is open, the fire exit,
 // and outside along the west wall if that door stands open, then back to the office.
 static const int32 Round[] = {NCorrSouth, NCorrRec, NRecIn, NRecEast, NRecNE, NRecSW, NRecW, NRecIn, NCorrRec, NExit,
  NStoop, NRamp, NWestMid, NWestNorth, NWestMid, NRamp, NStoop, NExit, NCorrRec, NCorrSouth, NOffice};
 const auto NextPatrol = [this]()
 {
  while (Patrol < UE_ARRAY_COUNT(Round))
  {
   const int32 N = Round[Patrol++];
   const int32 R = Room(Nodes()[N].At);
   if (R == 2 && !M->Web->IsDoorOpen(TEXT("recordsDoor"))) continue;
   if (R == 3 && !M->Web->IsDoorOpen(TEXT("exitDoor")) && Room(Pos) != 3) continue;
   RouteTo(N); if (!Path.IsEmpty() || FVector2D::Distance(Pos, Nodes()[N].At) < .2) return true;
  }
  return false;
 };
 // Go to the sound, then carry the round on from there: the fire exit's sound leads it outside.
 const auto Investigate = [this](const FVector2D& At)
 {
  LastKnown = At; const int32 N = Nearest(At); RouteTo(N);
  for (int32 I = 0; I < UE_ARRAY_COUNT(Round); ++I) if (Round[I] == N) { Patrol = I + 1; break; }
 };
 const auto Vanish = [this]()
 {
  bShown = false; Enter(EBody::Away); T = -1e9; // it does not come back this round
  F.bEscaped = true;
  if (M->Audio) M->Audio->Play(TEXT("door_latch"), .5f, S47Three::ToUE(OfficeDoor));
  M->Story->Note(FString::Printf(TEXT("%s. Something walked the annex and went back into the operations office. The latch clicked. Nobody has a key to that office but Ward."), *M->Story->ClockText(false)));
  UE_LOG(LogS47SC04, Display, TEXT("S47: SC04 body went back into the office; escape"));
  SetStep(EStep::Calm);
 };
 const FVector BangAt = Room(Pos) == 3 ? FVector(6.1, 1.2, 1.6) : FVector(-2.4, 1.2, 4.7);
 switch (B)
 {
 case EBody::Away:
  if (T >= 5.0)
  {
   if (M->Audio) M->Audio->Play(TEXT("door_latch"), .6f, S47Three::ToUE(OfficeDoor));
   bShown = true; Pos = Nodes()[NOffice].At; BodyYaw = HeadYaw = PI; Enter(EBody::Door);
  }
  break;
 case EBody::Door:
  // Out of the office door and one second quite still, then the head turns down the corridor.
  HeadYaw = StateT > 1.0 ? YawTo(Pl - Pos) : BodyYaw;
  if (SeeAcc >= SeeTime && StateT > 1.0) { LogSight(); Enter(EBody::Chase); break; }
  if (StateT > 2.0)
  {
   Enter(EBody::Search); Patrol = 0;
   if (PendingNoise) { Investigate(*PendingNoise); PendingNoise.Reset(); } else NextPatrol();
  }
  break;
 case EBody::Search:
 case EBody::Leave:
 {
  if (B == EBody::Search && SeeAcc >= SeeTime) { LogSight(); Enter(EBody::Chase); break; }
  if (B == EBody::Search && bLureRinging && !bSafe) { Enter(EBody::Lured); RouteTo(NOffice); break; }
  if (B == EBody::Search && Sound) Investigate(*Sound);
  if (Walk(B == EBody::Leave ? WalkSpeed * 1.2 : WalkSpeed, Dt))
  {
   // A shut records door on the first pass: it stops and listens before going on.
   if (B == EBody::Search && !bListened && FVector2D::Distance(Pos, Nodes()[NCorrRec].At) < .25 && !M->Web->IsDoorOpen(TEXT("recordsDoor")))
   { bListened = true; Enter(EBody::Listen); BodyYaw = HeadYaw = YawTo(FVector2D(-3.9, 6.8) - Pos); break; }
   if (B == EBody::Leave || !NextPatrol())
   {
    if (FVector2D::Distance(Pos, Nodes()[NOffice].At) < .3) Vanish();
    else { Enter(EBody::Leave); RouteTo(NOffice); }
   }
  }
  break;
 }
 case EBody::Listen:
  if (SeeAcc >= SeeTime) { Enter(EBody::Chase); break; }
  if (bLureRinging && !bSafe) { Enter(EBody::Lured); RouteTo(NOffice); break; }
  if (Sound && Room(*Sound) == 2 && FVector2D::Distance(Pos, Nodes()[NCorrRec].At) < .4)
  {
   M->Web->SetDoorOpen(TEXT("recordsDoor"), true, false); LastDoors.Add(TEXT("recordsDoor"), true);
   UE_LOG(LogS47SC04, Display, TEXT("S47: SC04 body heard steps behind the records door and opens it"));
   Enter(EBody::Search); Investigate(*Sound); break;
  }
  if (Sound && Room(*Sound) == 0 && !bBanged) { bBanged = true; if (M->Audio) M->Audio->Play(TEXT("thud"), .7f, S47Three::ToUE(BangAt)); }
  if (StateT > ListenTime) { Enter(EBody::Search); if (!NextPatrol()) { Enter(EBody::Leave); RouteTo(NOffice); } }
  break;
 case EBody::Lured:
  if (SeeAcc >= SeeTime) { Enter(EBody::Chase); break; }
  if (Walk(WalkSpeed * 1.3, Dt)) BodyYaw = HeadYaw = PI / 2; // facing the ringing office door (+z)
  if (Path.IsEmpty() && !bLureRinging && StateT > LureTime) Vanish(); // it goes in after the ringing
  break;
 case EBody::Chase:
 {
  if (bSees) LostT = 0; else LostT += Dt;
  if (bSafe && Room(Pos) != 0)
  {
   // The door was shut in its face: one blow, a listen, and it goes.
   Enter(EBody::Listen); bBanged = true;
   if (M->Audio) M->Audio->Play(TEXT("thud"), .8f, S47Three::ToUE(BangAt));
   break;
  }
  if (LostT > LoseTime)
  {
   // Lost: search where it last knew, then the round goes on from the node it is nearest.
   Enter(EBody::Search); Investigate(LastKnown);
   break;
  }
  // Straight at the player inside one room; along the graph between rooms.
  const FVector2D Goal = LastKnown;
  const FVector2D D = Goal - Pos; const double Dist = D.Size(), StepLen = FMath::Min(ChaseSpeed*Dt,Dist);
  const FVector2D Candidate = Dist > 1e-8 ? Pos + D / Dist * StepLen : Pos;
  if (Room(Goal) == Room(Pos) && ClosedDoorTo(Goal).IsEmpty() && ClearWalk(Pos,Candidate))
  {
   BodyYaw = HeadYaw = YawTo(D); Gait = FMath::Min(1.4, Gait + Dt * 4); Stride += StepLen * 2.1;
   Pos = Candidate;
   Path.Reset();
  }
  else
  {
   if (Path.IsEmpty() || StateT > .5) { RouteTo(Nearest(Goal)); StateT = 0; }
   Walk(ChaseSpeed, Dt);
  }
  if (!M->World->bDriving && FVector2D::Distance(Pos, Pl) < Reach && bSees) { Enter(EBody::Grab); BodyYaw = HeadYaw = YawTo(Pl - Pos); }
  break;
 }
 case EBody::Grab:
  if (M->World->bDriving || FVector2D::Distance(Pos,Pl) >= Reach || !bSees || !ClosedDoorTo(Pl).IsEmpty() || (bSafe && Room(Pos)!=0))
  { Enter(EBody::Chase); break; }
  if (StateT >= GrabHold) { bShown = false; Capture(); }
  break;
 default: break;
 }
}

// ---------- self-test ----------

void FS47SC04::TestComplete() { P = EStep::Done; Stop(); }

bool FS47SC04::SelfTest(TArray<FString>& Lines)
{
 bool Ok = true;
 const auto Check = [&](bool Good, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s SC04: %s"), Good ? TEXT("PASS") : TEXT("FAIL"), What)); Ok &= Good; };
 bool Rooms = Room(Nodes()[NControl].At) == 0;
 for (int32 I = NOffice; I <= NExit; ++I) Rooms &= Room(Nodes()[I].At) == 1;
 for (int32 I = NStoop; I <= NWestNorth; ++I) Rooms &= Room(Nodes()[I].At) == 3;
 for (int32 I = NRecIn; I <= NRecSW; ++I) Rooms &= Room(Nodes()[I].At) == 2;
 Check(Rooms, TEXT("every graph node lies in the room it names"));
 bool Short = true; for (const auto& E : Edges()) Short &= FVector2D::Distance(Nodes()[E.Key].At, Nodes()[E.Value].At) < 8;
 Check(Edges().Num() == 13 && Short, TEXT("thirteen edges: corridor, records room, the west wall outside, three doors one edge each"));
 Check(Room(FVector2D(7.5, 1.6)) == 3 && Room(FVector2D(4.3, 1.6)) == 0, TEXT("the east landing is outside and the service door leads into the control room"));
 Check(Room(FVector2D(-2.4,-12)) == 3 && Room(FVector2D(-4,680)) == 3 && Room(FVector2D(-3.9,11)) == 3,
  TEXT("north perimeter, motel and land beyond records are outside the bounded annex"));
 Check(CrossesDoor(FVector2D(-5.3,5.7),FVector2D(-7,5.7),Gaps[0])
  && CrossesDoor(FVector2D(-3.9,6.05),FVector2D(-3.9,7.5),Gaps[1])
  && CrossesDoor(FVector2D(-2.4,3.6),FVector2D(-2.4,6.05),Gaps[2])
  && !CrossesDoor(FVector2D(-4,6.05),FVector2D(-2,6.05),Gaps[2]),
  TEXT("actual door planes distinguish a crossing from walking beside a broad doorway zone"));
 FS47SC04 A(nullptr); A.P = EStep::Hunt; A.F.bSchedule = A.F.bBinder = true; A.F.Captures = 2; A.F.bLure = true;
 FS47SC04 Bk(nullptr); Bk.Begin(S47J::Read(S47J::Write(A.Save())));
 Check(Bk.P == EStep::Hunt && Bk.bRestartHunt && Bk.F.bSchedule && Bk.F.bBinder && !Bk.F.bStrip && !Bk.F.bJournal && Bk.F.Captures == 2 && Bk.F.bLure && !Bk.F.bEscaped,
  TEXT("collected originals and captures round-trip; a saved meeting restarts at the binder"));
 FS47SC04 Fresh(nullptr); Fresh.Begin(nullptr);
 Check(Fresh.P == EStep::Drive && Fresh.Label(TEXT("binder")).IsEmpty() && Fresh.Tasks().Num() == 1, TEXT("a new SC04 starts with the drive and offers nothing in the annex yet"));
 Check(StripDoc(TEXT("02:31")).Page.Contains(TEXT("02:31")) && !ScheduleDoc().Page.IsEmpty(), TEXT("the strip names the minute Ortiz was taken"));
 FS47SC04 Papers(nullptr); Papers.P=EStep::Calm; Papers.F.bSchedule=Papers.F.bStrip=Papers.F.bJournal=Papers.F.bBinder=true;
 FS47Doc Prior; Prior.Id=TEXT("prior-original"); Prior.Title=TEXT("Original title"); Prior.Kind=TEXT("hand"); Prior.Stamp=TEXT("ACTUAL 21:37");
 Prior.Page=TEXT("actual raw page"); Prior.Transcript=TEXT("separate transcript"); Prior.Image=TEXT("image-reference"); Prior.Paper=TEXT("paper-art"); Prior.PaperRatio=.71;
 Prior.Sheets.Add({TEXT("page two"),TEXT("paper two"),TEXT("strip two"),TEXT("term two")});
 Papers.SourcePapers={ScheduleDoc(),StripDoc(TEXT("02:37")),Prior};
 FS47SC04 Restored(nullptr); Restored.Begin(S47J::Read(S47J::Write(Papers.Save())));
 Check(Restored.SourcesReady() && Restored.SourcePapers.Num()==3
  && S47J::Write(SourceSave(Restored.SourcePapers[2]))==S47J::Write(SourceSave(Prior))
  && Restored.SourcePapers[1].Page==Papers.SourcePapers[1].Page,
  TEXT("owned source payload preserves actual strip minute, stamps, transcript, art and every sheet through JSON/Begin"));
 const auto Legacy=S47J::Read(TEXT("{\"step\":4,\"schedule\":true,\"strip\":true,\"journal\":true,\"binder\":true,\"escaped\":true}"));
 Restored.Begin(Legacy);
 Check(Restored.P==EStep::Done && Restored.F.bEscaped && Restored.SourcesVersion==0 && Restored.SourcePapers.IsEmpty(),
  TEXT("legacy completion is retained without manufacturing missing originals or timestamps"));
 Restored.Begin(S47J::Read(TEXT("{\"step\":4,\"sourcesVersion\":1,\"schedule\":true,\"strip\":true,\"journal\":true,\"binder\":true,\"escaped\":true,\"sources\":[]}")));
 Check(!Restored.SourcesReady() && Restored.SourcePapers.IsEmpty(),TEXT("new source-version flags cannot replace absent acquired source payload"));
 return Ok;
}
