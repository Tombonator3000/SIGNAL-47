#include "S47Observing.h"
#include "S47SaroWork.h"
#include "SS47Widgets.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateColorBrush.h"
#include "Rendering/DrawElements.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Horror.h"
#include "S47Docs.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Ambience.h"
#include "S47Web.h"
#include "S47Inter.h"
#include "S47Evening.h"
#include "S47RecordedDialogue.h"
#include "S47RadioVoice.h"
#include "S47Three.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Kit.h"
#include "Framework/Application/SlateApplication.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
 FS47Profile Card(const TCHAR* Stage, const TCHAR* Text, double F, double FTol, double G0, double G1, double B0, double B1, double A0, double A1, const TCHAR* Note)
 {
  FS47Profile P; P.Stage = Stage; P.Action = TEXT("START RUN"); P.Card = Text; P.bCardIsTaped = false;
  P.F = F; P.FTol = FTol; P.Gain[0] = G0; P.Gain[1] = G1; P.Bw[0] = B0; P.Bw[1] = B1; P.Az[0] = A0; P.Az[1] = A1; P.Note = Note;
  return P;
 }
 // Hint bits, in the order the evening reaches them.
 constexpr int32 HintTicks = 1, HintBroadband = 2, HintStill = 4, HintDrive = 8, HintTube = 16, HintGate = 32;
 const FVector GeneratorAt(4.3, .6, 8.4); // three.js, the standby generator in the pocket behind the control room
 constexpr int32 HintVoice = 64, HintReel = 128;
 const FVector RecorderAt(-1.14, 1.0, 10.22); // three.js, the old reel-to-reel on the records-room shelf
 const TCHAR* const DoorLine = TEXT("D01_HANDOVER_001"); // "Reyes? Dale. The logbook should be in front of you."
 constexpr double GenStart = -4 * 3600. - 25 * 60, GenDone = -4 * 3600. - 15 * 60, ArchiveStart = -3 * 3600. - 30 * 60, ArchiveDone = -3 * 3600. - 20 * 60, YardStart = -2 * 3600. - 20 * 60;
 // PSR B1133+16 folds at 1.18791 s. The stray ticks fall four, a gap, then seven.
 constexpr double PulsarPeriod = 1.18791;
 const double StrayTicks[] = { 9.30, 9.55, 9.80, 10.05, 11.10, 11.35, 11.60, 11.85, 12.10, 12.35, 12.60 };
 const FVector PrinterAt(5.62, .74, -.4); // three.js, the dot matrix printer on the east wall
 // Recorded sounds (web/src/assets/audio/obs_*, THIRD_PARTY_NOTICES.md); false when the build lacks them.
 bool Recorded(US47Audio* A, const TCHAR* Name, float Gain, float Rate = 1.f, TOptional<FVector> At = {})
 {
  if (!A) return false;
  FS47Sound S; S.Gain = Gain; S.Rate = Rate; S.At = At;
  const bool bPlayed = bool(A->Sfx(Name, S));
  static TSet<FString> Logged;
  if (bPlayed && !Logged.Contains(Name)) { Logged.Add(Name); UE_LOG(LogTemp, Display, TEXT("S47: observing recording %s played"), Name); }
  return bPlayed;
 }
}

// Positions are for the site on 13 April 1986 (MST): 3C 286 az 69 el 25 at 19:00,
// B1133+16 az 112 el 54 at 19:50, Virgo A az 146 el 65 at 22:05. The console turns 0..180.
const TArray<FS47Observing::FRun>& FS47Observing::Runs()
{
 static const TArray<FRun> List = {
  { TEXT("cal3c286"), TEXT("3C 286"), TEXT("19:00 pointing check on 3C 286"), TEXT("printer"), -5 * 3600., 35, 12, 25,
   Card(TEXT("OBS 1/4"), TEXT("SCHEDULE 1/4  19:00  POINTING CHECK 3C 286  /  CONTINUUM 1419.900 MHz  /  AZ 070  /  GAIN 50-60  /  BW 50-70"),
    1419.9, .02, 50, 60, 50, 70, 66, 74, TEXT("19:00. Pointing check on 3C 286. A clean cross, flux within two percent of the catalogue."))},
  { TEXT("psr1133"), TEXT("PSR B1133+16"), TEXT("19:50 timing check on pulsar B1133+16"), TEXT("printer"), -4 * 3600. - 10 * 60, 40, 18, 54,
   Card(TEXT("OBS 2/4"), TEXT("SCHEDULE 2/4  19:50  PULSAR TIMING PSR B1133+16  /  1419.600 MHz, OFF THE LINE  /  AZ 112  /  GAIN 65-80  /  BW 90-100"),
    1419.6, .03, 65, 80, 90, 100, 108, 116, TEXT("19:50. PSR B1133+16 on time, 1.18791 s. Some ticks on the speaker that were not the pulsar. Counted four, then seven."))},
  { TEXT("hidrift"), TEXT("HI strip 12"), TEXT("20:45 hydrogen-line drift scan, strip 12"), TEXT("opsTerminal"), -3 * 3600. - 15 * 60, 45, 20, 48,
   Card(TEXT("OBS 3/4"), TEXT("SCHEDULE 3/4  20:45  HI DRIFT SCAN, STRIP 12  /  HYDROGEN LINE 1420.380 MHz  /  AZ 090  /  GAIN 50-65  /  BW 10-24"),
    1420.38, .015, 50, 65, 10, 24, 86, 94, TEXT("20:45. Hydrogen drift scan, strip 12. Local arm at -3 km/s. 38 seconds of broadband noise at 21:12, flagged. Nothing in the aircraft log."))},
  { TEXT("vira"), TEXT("Virgo A"), TEXT("22:05 flux check on Virgo A"), TEXT("crtLeft"), -1 * 3600. - 55 * 60, 45, 14, 64,
   Card(TEXT("OBS 4/4"), TEXT("SCHEDULE 4/4  22:05  FLUX CHECK VIRGO A  /  CONTINUUM 1419.900 MHz  /  AZ 146  /  GAIN 45-60  /  BW 40-60"),
    1419.9, .02, 45, 60, 40, 60, 142, 150, TEXT("22:05. Virgo A, 198 Jy, phase stable. Drive 14 ran for three seconds with no command. The crickets stopped while it ran."))},
 };
 return List;
}

FS47Doc FS47Observing::Schedule()
{
 FS47Doc D; D.Id = TEXT("obs_schedule"); D.Title = TEXT("Observing schedule, 13 April"); D.Kind = TEXT("typed"); D.Stamp = TEXT("SARO OPS");
 D.Page = TEXT("SARO / OBSERVING SCHEDULE   SUN 04/13/86\n")
  TEXT("OPERATOR REYES   PROGRAM S-47  CAL / HI\n\n")
  TEXT("19:00  3C 286     POINTING CHECK   AZ 070\n")
  TEXT("19:50  B1133+16   PULSAR TIMING    AZ 112\n")
  TEXT("20:45  STRIP 12   HI DRIFT SCAN    AZ 090\n")
  TEXT("22:05  VIR A      FLUX CHECK       AZ 146\n")
  TEXT("23:00  NORTH WINDOW LAMP ROUND\n")
  TEXT("23:40  OVERNIGHT SURVEY\n\n")
  TEXT("TEAR OFF EVERY CHART. REDUCE THE DRIFT\n")
  TEXT("SCAN ON THE OPS TERMINAL. CHECK THE DRIVE\n")
  TEXT("LOG ON THE STATUS TERMINAL AFTER VIR A.\n")
  TEXT("~ the 1420.1 relay comes and goes. Notch it before the survey, not before. D.");
 D.Transcript = TEXT("Tonight's schedule, typed in the afternoon and clipped under the console card. Four ordinary runs: a pointing check, a pulsar, a hydrogen drift scan and a flux check, each set for when its source stands east of the meridian. Then the north-window lamp round at 23:00 and the unattended survey.\n\nIn Dale's pencil at the bottom: \"The 1420.1 relay comes and goes. Notch it before the survey, not before.\"");
 return D;
}

FS47Doc FS47Observing::Chart(int32 I)
{
 FS47Doc D; D.Kind = TEXT("printout"); D.Stamp = TEXT("13 APR 86");
 if (I == 0)
 {
  D.Id = TEXT("obs_chart_3c286"); D.Title = TEXT("Strip chart: 3C 286 pointing check");
  D.Page = TEXT("SARO / RX CONTROL 03   CONTINUUM 1419.900 MHz\n")
   TEXT("SRC 3C 286   RA 13 28 49.7  DEC +30 45 59 (1950)\n")
   TEXT("AZ 070.0  EL 25.1   BW 60 KHZ  GAIN 55   19:04-19:31\n\n")
   TEXT("FIVE-POINT CROSS      TA (K)\n")
   TEXT("  N  +0.5 DEG          0.41\n")
   TEXT("  S  -0.5 DEG          0.44\n")
   TEXT("  E  +0.5 DEG          0.43\n")
   TEXT("  W  -0.5 DEG          0.40\n")
   TEXT("  C   0.0              0.86\n\n")
   TEXT("OFFSET  AZ +0.04  EL -0.02 DEG   IN SPEC\n")
   TEXT("FLUX    14.6 JY   (BAARS 1977: 14.9)\n")
   TEXT("~ fine. D's pointing model still holds.");
  D.Transcript = TEXT("The five numbers of a pointing cross: the source is brightest in the middle and falls off evenly to each side, so the array points where the computer says it does. The flux is within two percent of the catalogue value. An ordinary first run.");
 }
 else if (I == 1)
 {
  D.Id = TEXT("obs_chart_b1133"); D.Title = TEXT("Strip chart: PSR B1133+16 timing");
  D.Page = WorkReference() + TEXT("\nSARO / RX CONTROL 03   1419.600 MHz  BW 96 KHZ\n")
   TEXT("PSR B1133+16   AZ 112.0  EL 54.3   19:52-20:28\n\n")
   TEXT("PERIOD   1.18791 S        CATALOGUE 1.187911\n")
   TEXT("DM       4.86              FOLDED 1800 PULSES\n")
   TEXT("PROFILE  DOUBLE, AS KNOWN   RESIDUAL +0.3 MS\n\n")
   TEXT("19:58:12-19:58:21  EXTRA TICKS, NOT FOLDED\n")
   TEXT("                   NOT AT THE PULSAR PERIOD\n")
   TEXT("~ counted them twice: 4, a gap, 7.\n")
   TEXT("~ multipath off the mesa? ask D.");
  D.Transcript = TEXT("The pulsar keeps perfect time, as it always has: 1.18791 seconds, the same as the catalogue. For nine seconds there were other ticks on the speaker that were not at its period. You counted them twice. Four, a gap, seven. Probably a reflection off the mesa.");
 }
 else if (I == 2)
 {
  D.Id = TEXT("obs_reduction_strip12"); D.Title = TEXT("Ops terminal: HI strip 12 reduction"); D.Stamp = TEXT("OPS 21:38");
  D.Page = TEXT("HI DRIFT SCAN  STRIP 12  DEC +30  AZ 090 EL 48\n")
   TEXT("CENTRE 1420.380 MHZ  BW 24 KHZ  128 CH\n")
   TEXT("20:45-21:30   45 MIN DRIFT\n\n")
   TEXT("PEAK   V(LSR)  -3.1 KM/S   TB 34 K   LOCAL ARM\n")
   TEXT("WING   V(LSR) -41   KM/S   TB  4 K\n")
   TEXT("BASELINE RMS 0.08 K   REST 1420.4058 MHZ\n\n")
   TEXT("FLAG   21:12:40-21:13:18   ALL CHANNELS\n")
   TEXT("       BROADBAND, 38 S. NOT IN AIRCRAFT LOG.\n")
   TEXT("~ no engine noise outside. a satellite? too slow.");
  D.Transcript = TEXT("The reduced drift scan: a clean hydrogen profile from the local arm, a weaker wing further out, a flat baseline. The terminal flags 38 seconds at 21:12 where every channel rose at once, as if something bright crossed the beam. Nothing is in the aircraft log, and there was no sound of an engine.");
 }
 else
 {
  D.Id = TEXT("obs_chart_vira"); D.Title = TEXT("Status terminal: Virgo A and drive log"); D.Stamp = TEXT("STATUS 22:47");
  D.Page = TEXT("SARO / RX CONTROL 03   CONTINUUM 1419.900 MHz\n")
   TEXT("VIR A (3C 274)   AZ 146.0  EL 64.8   22:07-22:46\n\n")
   TEXT("FLUX       198 JY   (CATALOGUE 210 +-10)\n")
   TEXT("PHASE RMS  4 DEG    STABLE\n")
   TEXT("TSYS       41 K\n\n")
   TEXT("22:24:05  AZ DRIVE 14  CURRENT 2.1 A  3 S\n")
   TEXT("          NO COMMAND ISSUED. ENCODER UNCHANGED.\n")
   TEXT("~ drive 14 again. the crickets stopped while it ran.");
  D.Transcript = TEXT("Virgo A, inside the catalogue range, the phases steady. Below the numbers the drive log has one line it should not: drive 14 drew current for three seconds with no command from the console, and its encoder never moved. Outside, the crickets had stopped while it ran.");
 }
 return D;
}

void FS47Observing::Begin(const S47J::FObj& Saved, bool bNewCase)
{
 Stop();
 bReplay = false; bEnabled = bScheduleFiled = false; Run = HintMask = Ticks = GenStep = Archive = Yard = DoorVoice = 0; GenT = 0; FlickerT = -1; HeldTubes.Reset(); StepNow = EStep::Tune; Elapsed = Tick = 0; HeldStatus.Empty();
 PulsarLoop = ELoopStage::Off; ControlT = ControlTick = 0; CompareFeedback.Empty();
 bWorkEnabled = bWorkOrderFiled = bAudioPanelOpen = bTelemetryPanelOpen = bAudioPlaying = bAudioReviewed = bPacketCollected = bWorkActive = false;
 AudioSelected = SelectedDish = 0; AudioCursor[0] = AudioCursor[1] = AudioCovered[0] = AudioCovered[1] = 0; AudioCoverage[0] = AudioCoverage[1] = 0;
 MarkStart = MarkEnd = -1; AudioFeedback.Empty();
 bS03Diagnosed = false;
 if (!ArrayWork) ArrayWork = MakeShared<FS47SaroWork>(M);
 ArrayWork->Begin(nullptr, false);
 if (Saved)
 {
  if (S47J::Int(Saved, TEXT("v")) != Version) return;
  bEnabled = true; bScheduleFiled = S47J::Bool(Saved, TEXT("schedule"));
  Run = FMath::Clamp(S47J::Int(Saved, TEXT("run")), 0, Runs().Num());
  StepNow = EStep(FMath::Clamp(S47J::Int(Saved, TEXT("step")), 0, 2));
  if (Run >= Runs().Num()) StepNow = EStep::Tune;
  Elapsed = FMath::Clamp(S47J::Num(Saved, TEXT("elapsed")), 0., 600.);
  HintMask = S47J::Int(Saved, TEXT("hints")); HeldStatus = S47J::Str(Saved, TEXT("status"));
  // A save on generator power comes back running on line power: the room lights are not saved here.
  GenStep = FMath::Clamp(S47J::Int(Saved, TEXT("generator")), 0, 3); if (GenStep == 2) GenStep = 1;
  // Version-one schedules written before the archive round keep an explicit exemption.
  // This is not filing or a heard reel: leave their document and hint facts untouched.
  Archive = Saved->HasField(TEXT("archive")) ? FMath::Clamp(S47J::Int(Saved, TEXT("archive")), -1, 2) : -1;
  Yard = FMath::Clamp(S47J::Int(Saved, TEXT("yard")), 0, 2);
  DoorVoice = FMath::Clamp(S47J::Int(Saved, TEXT("doorVoice")), 0, 2);
  // Missing state is an explicit legacy exemption. Never add a new job to a passed run.
  if (S47J::Int(Saved, TEXT("loopVersion"), 0) == 1)
  {
   const int32 Stage = S47J::Int(Saved, TEXT("loopStage"), -1);
   if (Stage >= int32(ELoopStage::AwaitPulsar) && Stage <= int32(ELoopStage::Filed))
   {
    PulsarLoop = ELoopStage(Stage);
    if (Run > 1 && PulsarLoop != ELoopStage::Filed) PulsarLoop = ELoopStage::Off;
    else if (Run < 1 || (Run == 1 && StepNow != EStep::Data)) PulsarLoop = ELoopStage::AwaitPulsar;
    ControlT = FMath::Clamp(S47J::Num(Saved, TEXT("controlElapsed")), 0., ControlSeconds);
    ControlTick = FMath::Fmod(ControlT, PulsarPeriod);
    CompareFeedback = S47J::Str(Saved, TEXT("loopFeedback"));
   }
  }
  // Only explicitly versioned workstation cases receive the added service and filing duties.
  if (S47J::Int(Saved, TEXT("workVersion"), 0) == 1)
  {
   bWorkEnabled = true;
   bWorkOrderFiled = S47J::Bool(Saved, TEXT("workOrder"));
   bAudioReviewed = S47J::Bool(Saved, TEXT("audioReviewed"));
   bPacketCollected = S47J::Bool(Saved, TEXT("packetCollected"));
   bS03Diagnosed = S47J::Bool(Saved, TEXT("s03Diagnosed"));
   AudioSelected = FMath::Clamp(S47J::Int(Saved, TEXT("audioSource")), 0, 1);
   AudioCursor[0] = FMath::Clamp(S47J::Num(Saved, TEXT("audioOriginal")), 0., Runs()[1].Seconds);
   AudioCursor[1] = FMath::Clamp(S47J::Num(Saved, TEXT("audioControl")), 0., ControlSeconds);
   AudioCovered[0] = FMath::Clamp(S47J::Num(Saved, TEXT("coveredOriginal")), 0., Runs()[1].Seconds);
   AudioCovered[1] = FMath::Clamp(S47J::Num(Saved, TEXT("coveredControl")), 0., ControlSeconds);
   AudioCoverage[0] = (1u << int32(AudioCovered[0])) - 1; AudioCoverage[1] = (1u << int32(AudioCovered[1])) - 1;
   MarkStart = FMath::Clamp(S47J::Num(Saved, TEXT("markStart"), -1), -1., Runs()[1].Seconds);
   MarkEnd = FMath::Clamp(S47J::Num(Saved, TEXT("markEnd"), -1), -1., Runs()[1].Seconds);
   bAudioReviewed &= AudioCoverageComplete() && MarkStart >= 0 && MarkStart <= StrayTicks[0] && MarkEnd >= StrayTicks[UE_ARRAY_COUNT(StrayTicks) - 1] && MarkEnd - MarkStart <= 6;
   bPacketCollected &= Run >= 2 && PulsarLoop == ELoopStage::Filed;
   AudioFeedback = S47J::Str(Saved, TEXT("audioFeedback"));
   ArrayWork->Begin(S47J::Obj(Saved, TEXT("workArray")), true);
   if (!ArrayWork->Enabled()) bWorkEnabled = false;
   if (Run >= Runs().Num()) ArrayWork->Stop(); // preserve completed service history without restoring old pointing
  }
  if (GenStep == 1 && M && M->Audio) { FS47Sound S; S.Gain = .3f; S.At = GeneratorAt; S.bLoop = true; M->Audio->LoopSfx(TEXT("obsGenerator"), TEXT("engineIdle"), S); }
  // The stillness belongs to an unfinished Virgo A run only.
  if (StepNow == EStep::Integrating && Run == 3 && (HintMask & HintStill) && M && M->Audio && M->Audio->Night) M->Audio->Night->Still();
  return;
 }
 bEnabled = bNewCase && M && M->Story && M->Story->bEveningTimeline;
 if (bEnabled)
 {
  PulsarLoop = ELoopStage::AwaitPulsar; bWorkEnabled = true; ArrayWork->Begin(nullptr, true);
 }
}

S47J::FObj FS47Observing::Save() const
{
 if (!bEnabled) return nullptr;
 auto O = S47J::New(); S47J::Set(O, TEXT("v"), Version); S47J::Set(O, TEXT("run"), Run); S47J::Set(O, TEXT("step"), int32(StepNow));
 S47J::Set(O, TEXT("elapsed"), Elapsed); S47J::Set(O, TEXT("hints"), HintMask); S47J::Set(O, TEXT("schedule"), bScheduleFiled); S47J::Set(O, TEXT("status"), HeldStatus);
 S47J::Set(O, TEXT("generator"), GenStep); S47J::Set(O, TEXT("archive"), Archive); S47J::Set(O, TEXT("yard"), Yard); S47J::Set(O, TEXT("doorVoice"), DoorVoice);
 if (PulsarLoop != ELoopStage::Off)
 {
  S47J::Set(O, TEXT("loopVersion"), 1); S47J::Set(O, TEXT("loopStage"), int32(PulsarLoop));
  S47J::Set(O, TEXT("controlElapsed"), ControlT); S47J::Set(O, TEXT("loopFeedback"), CompareFeedback);
 }
 if (bWorkEnabled)
 {
  S47J::Set(O, TEXT("workVersion"), 1); S47J::Set(O, TEXT("workOrder"), bWorkOrderFiled);
  S47J::Set(O, TEXT("audioReviewed"), bAudioReviewed); S47J::Set(O, TEXT("packetCollected"), bPacketCollected);
  S47J::Set(O, TEXT("s03Diagnosed"), bS03Diagnosed);
  S47J::Set(O, TEXT("audioSource"), AudioSelected);
  S47J::Set(O, TEXT("audioOriginal"), AudioCursor[0]); S47J::Set(O, TEXT("audioControl"), AudioCursor[1]);
  S47J::Set(O, TEXT("coveredOriginal"), AudioCovered[0]); S47J::Set(O, TEXT("coveredControl"), AudioCovered[1]);
  S47J::Set(O, TEXT("markStart"), MarkStart); S47J::Set(O, TEXT("markEnd"), MarkEnd);
  S47J::Set(O, TEXT("audioFeedback"), AudioFeedback);
  if (ArrayWork) if (const auto W = ArrayWork->Save()) O->SetObjectField(TEXT("workArray"), W);
 }
 return O;
}

bool FS47Observing::Active() const
{
 return Pending() && M && M->Story && M->Horror && M->Horror->Enabled() && M->Horror->BaselineChecked() &&
  M->Story->Rx.Stage == 1 && M->Story->Phase() == ES47Phase::Shift;
}

const FS47Profile* FS47Observing::ConsoleProfile() const
{
 if (!Active()) return nullptr;
 if (PulsarCheckActive() && PulsarLoop == ELoopStage::ControlRunning)
 {
  static const FS47Profile P = []
  {
   FS47Profile Ref = Runs()[1].P; Ref.Stage = TEXT("LOCAL RECEIVER CONTROL"); Ref.Action = TEXT("REFERENCE RUNNING");
   Ref.Card = TEXT("EXTERNAL INPUT TERMINATED / INTERNAL REFERENCE / 8 SECOND CONTROL / COLLECT THE CHART WHEN COMPLETE"); return Ref;
  }();
  return &P;
 }
 return &Runs()[Run].P;
}

bool FS47Observing::ConsoleAction()
{
 if (!Active()) return false;
 FS47Rx& Rx = M->Story->Rx;
 if (StepNow == EStep::Integrating) { Rx.Status = FString::Printf(TEXT("INTEGRATING %s  /  HOLD"), Runs()[Run].Source).ToUpper(); return true; }
 if (StepNow == EStep::Data) { Rx.Status = PulsarCheckActive() ? PulsarObjective() : FString(TEXT("RUN COMPLETE  /  SEE THE OBJECTIVE")); return true; }
 if (ChorePending())
 {
  Rx.Status = Objective();
  return true;
 }
 if (bWorkEnabled && (!ArrayWork || ArrayWork->CommandedRun() != Run || !ArrayWork->Ready() || !ArrayWork->CanMeasure()))
 {
  Rx.Status = ArrayWork && ArrayWork->ServiceRequired() ? ArrayWork->Objective() : TEXT("TRACKING TERMINAL // POINT THE ARRAY AND WAIT FOR ON TARGET");
  return true;
 }
 if (!Rx.Pass(Runs()[Run].P)) { Rx.Status = TEXT("OFF SOURCE  /  CHECK THE SCHEDULE CARD"); return true; }
 Start();
 return true;
}

void FS47Observing::Start()
{
 const FRun& R = Runs()[Run];
 AS47Story& S = *M->Story;
 HeldStatus = S.Rx.Status;
 // The source rises on the schedule's time. Earlier work does not wait in real time for it.
 S.AdvanceClockTo(R.Slot, TEXT("scheduled observation start"));
 if (bWorkEnabled && ArrayWork) ArrayWork->BeginScan();
 else S.AimArray((R.P.Az[0] + R.P.Az[1]) / 2, R.El, 9);
 if (!bWorkEnabled && M->Audio && !Recorded(M->Audio, TEXT("obsArraySlew"), .45f)) M->Audio->Servo(2.4f, TOptional<FVector>());
 S.Rx.Status = FString::Printf(TEXT("INTEGRATING %s"), R.Source).ToUpper();
 StepNow = EStep::Integrating; Elapsed = 0; Tick = 0; Ticks = 0;
 M->UI->Toast(bWorkEnabled ? FString::Printf(TEXT("%s. %s on target. Acquisition started; the chart pen records only settled pointing."), *S.ClockText(false), R.Source)
  : FString::Printf(TEXT("%s. The array swings onto %s. The chart pen starts its line."), *S.ClockText(false), R.Source), 4);
 UE_LOG(LogTemp, Display, TEXT("S47: observing run %d %s started at clock %.0f"), Run + 1, R.Id, S.Clock());
 Changed();
}

void FS47Observing::Step(double Dt)
{
 if (!bEnabled || !M || !M->Story || M->Mode != US47Main::EMode::Play || !FMath::IsFinite(Dt) || Dt <= 0) return;
 if (M->bPausedByMenu) { if (ArrayWork && ArrayWork->Enabled() && (bWorkActive || Active())) ArrayWork->Step(Dt, true); return; }
 if (Done()) { if (ArrayWork && ArrayWork->Enabled()) ArrayWork->Stop(); StepDoorVoice(); return; }
 if (!Active()) { bAudioPlaying = false; if (bWorkActive && ArrayWork) { ArrayWork->Stop(); bWorkActive = false; } return; }
 bWorkActive = true;
 if (bWorkEnabled && ArrayWork)
 {
  ArrayWork->SetContext(Run, Run == 1 && GenStep >= 3);
  ArrayWork->Step(Dt, (M->ModalOpen() && !bTelemetryPanelOpen) || M->Story->IsCinematic());
 }
 StepAudio(Dt);
 if (!bScheduleFiled)
 {
  bScheduleFiled = true; M->Story->AddDoc(Schedule());
  M->UI->Toast(TEXT("Dale's observing schedule is clipped under the console card: four runs before the 23:00 window round."), 5);
  Changed();
 }
 StepChores(Dt);
 StepPulsarControl(Dt);
 if (StepNow != EStep::Integrating || (M->ModalOpen() && !bTelemetryPanelOpen) || M->Story->IsCinematic()) return;
 if (bWorkEnabled && ArrayWork && !ArrayWork->CanMeasure()) return;
 const FRun& R = Runs()[Run];
 const double Was = Elapsed;
 Elapsed = FMath::Min(Elapsed + FMath::Min(Dt, .1), R.Seconds);
 M->Story->Rx.Status = FString::Printf(TEXT("INTEGRATING %s  %02d S"), R.Source, int32(FMath::CeilToDouble(R.Seconds - Elapsed))).ToUpper();
 const auto Crossed = [&](double At) { return Was < At && Elapsed >= At; };
 if (Run == 1 && M->Audio)
 {
  // The pulsar is audible on the monitor speaker; its beat never changes.
  Tick += Elapsed - Was;
  // Real audified pulses of PSR B0329+54 (Green Bank), retimed to this pulsar's period.
  while (Tick >= PulsarPeriod) { Tick -= PulsarPeriod; if (!Recorded(M->Audio, TEXT("obsPulsarPulse"), .4f)) M->Audio->Beep(820, .012f, .16f, 0, TEXT("square")); ++Ticks; }
  for (double At : StrayTicks) if (Crossed(At) && !Recorded(M->Audio, TEXT("obsPulsarPulse"), .14f, 1.7f)) M->Audio->Beep(1240, .008f, .07f, 0, TEXT("square"));
  if (Crossed(StrayTicks[0])) Hint(HintTicks);
 }
 if (Run == 2 && Crossed(8)) Hint(HintBroadband);
 if (Run == 3 && Crossed(5)) Hint(HintStill);
 if (Run == 3 && Crossed(9)) Hint(HintDrive);
 if (Elapsed >= R.Seconds && (!bWorkEnabled || !ArrayWork || ArrayWork->ScanComplete())) Finish();
}

void FS47Observing::Hint(int32 Bit)
{
 if (HintMask & Bit) return;
 HintMask |= Bit;
 if (Bit == HintTicks) M->UI->Toast(TEXT("Between the pulsar's ticks, other ticks. Fainter, out of its beat. Then only the pulsar again."), 5);
 if (Bit == HintBroadband)
 {
  M->Story->Rx.Status = TEXT("BROADBAND RFI  /  ALL CHANNELS  /  FLAGGED");
  Recorded(M->Audio, TEXT("obsBroadband"), .35f);
  M->UI->Toast(TEXT("The chart pen jumps across every channel and stays up. Like something bright passing through the beam. No engine sound outside."), 5);
 }
 if (Bit == HintStill)
 {
  if (M->Audio && M->Audio->Night) M->Audio->Night->Still();
  M->UI->Toast(TEXT("Outside, the crickets stop. All of them at once."), 4);
 }
 if (Bit == HintReel)
 {
  FS47Sound O; O.Gain = .45f; O.At = RecorderAt;
  if (M->Audio && !M->Audio->Sfx(TEXT("reel"), O)) M->Audio->Play(TEXT("click"), .5f);
  M->UI->Toast(TEXT("Behind you the old reel-to-reel on the shelf clicks once. Its take-up spool has moved a quarter turn. The transport is switched off."), 5);
 }
 if (Bit == HintGate) M->UI->Toast(TEXT("Behind you, chain-link rattles. The array gate you chained stands open. Nobody on the field."), 5);
 if (Bit == HintDrive)
 {
  if (M->Audio && !Recorded(M->Audio, TEXT("obsArrayDrive"), .55f)) M->Audio->Servo(3.f, TOptional<FVector>());
  M->UI->Toast(TEXT("Out in the array a drive motor runs for three seconds. Nothing on the console asked it to."), 5);
 }
 UE_LOG(LogTemp, Display, TEXT("S47: observing hint %d during run %d"), Bit, Run + 1);
 Changed();
}

void FS47Observing::Finish()
{
 const FRun& R = Runs()[Run];
 StepNow = EStep::Data; Elapsed = R.Seconds;
 if (Run == 3 && M->Audio && M->Audio->Night && M->Audio->Night->IsStill()) M->Audio->Night->Back(3);
 const bool Printer = R.Data == FString(TEXT("printer")), Status = R.Data == FString(TEXT("crtLeft"));
 M->Story->Rx.Status = Printer ? TEXT("RUN COMPLETE  /  CHART AT THE PRINTER") : Status ? TEXT("RUN COMPLETE  /  DRIVE LOG ON THE STATUS TERMINAL") : TEXT("RUN COMPLETE  /  REDUCE ON THE OPS TERMINAL");
 if (M->Audio && Printer && !Recorded(M->Audio, TEXT("obsChartPrinter"), .5f, 1.f, PrinterAt))
  M->Audio->Play(TEXT("printer"), .35f, M->Web ? M->Web->PropLocation(TEXT("printer")) : FVector::ZeroVector, 1.2f);
 M->UI->Toast(Printer ? FString::Printf(TEXT("%s done. The printer chatters out the strip chart."), R.Source) : Status ? FString(TEXT("Virgo A done. The status terminal on the left has logged a drive event.")) : FString(TEXT("Drift scan done. Reduce it on the operations terminal.")), 4);
 UE_LOG(LogTemp, Display, TEXT("S47: observing run %d %s integrated"), Run + 1, R.Id);
 Changed();
}

FString FS47Observing::Label(const FString& Id) const
{
 if (bWorkEnabled && Active())
 {
  if (ArrayWork) if (const FString L = ArrayWork->Label(Id); !L.IsEmpty()) return L;
  if (Id == TEXT("crtLeft")) return TEXT("Inspect array status and command history");
  if (Id == TEXT("crtRight")) return TEXT("Point the array for the scheduled observation");
  if (Id == TEXT("telex")) return TEXT("Read tonight's observing work order");
  if (Id == TEXT("decoder")) return AudioAvailable() ? FString(TEXT("Compare original and control recordings")) : FString(TEXT("Signal processor / recording inputs"));
  if (Id == TEXT("printer") && Run == 2 && Archive == 0)
   return bPacketCollected ? FString(TEXT("Registered packet collected / take it to the run binder")) : FString(TEXT("Collect the labelled observation packet"));
  if (Id == TEXT("binder") && Run == 2 && Archive == 0)
   return bPacketCollected ? FString(TEXT("Place the registered packet behind April 13")) : FString(TEXT("Run binder / collect the registered packet first"));
 }
 if (PulsarCheckActive())
 {
  if (Id == TEXT("rack")) return PulsarLoop == ELoopStage::Rack ? FString(TEXT("Test the receiver on its terminated input and internal reference"))
   : PulsarLoop == ELoopStage::ControlRunning ? FString(TEXT("Internal reference running: listen to the regular ticks"))
   : PulsarLoop == ELoopStage::ControlPrint || PulsarLoop == ELoopStage::Compare ? FString(TEXT("Receiver control finished: retain both charts")) : FString();
  if (Id == TEXT("printer")) return PulsarLoop == ELoopStage::AwaitPulsar ? FString(TEXT("Secure the pulsar strip chart with the extra ticks"))
   : PulsarLoop == ELoopStage::ControlPrint ? FString(TEXT("Collect the internal-reference control chart")) : FString();
  if (Id == TEXT("opsTerminal")) return PulsarLoop == ELoopStage::Compare ? FString(TEXT("Compare the pulsar and receiver-control charts")) : FString();
 }
 if (Id == TEXT("obs:generator"))
 {
  if (!Active() || Run != 1 || StepNow != EStep::Tune || GenStep >= 3) return FString(TEXT("Generator remote panel"));
  if (GenStep == 0) return TEXT("Start the weekly generator test");
  if (GenStep == 1) return GenT >= 4 ? TEXT("Transfer the load to the generator") : FString(TEXT("Generator warming up"));
  return GenT >= 3 ? TEXT("Transfer back to line power") : FString(TEXT("On generator power"));
 }
 if (Id == TEXT("binder")) return Active() && Run == 2 && StepNow == EStep::Tune && Archive == 0 ? FString(TEXT("File the evening's charts in the run binder")) : FString();
 if (!Active() || StepNow != EStep::Data || Id != Runs()[Run].Data) return FString();
 if (Id == TEXT("printer")) return FString::Printf(TEXT("Tear off the %s strip chart"), Runs()[Run].Source);
 return Id == TEXT("crtLeft") ? FString(TEXT("Read the array drive log")) : FString(TEXT("Reduce the hydrogen drift scan"));
}

bool FS47Observing::Use(const FString& Id)
{
 if (bWorkEnabled && Active())
 {
  if (Id == TEXT("obs:service") && ArrayWork && ArrayWork->ServiceRequired() && !bS03Diagnosed)
  {
   M->UI->Toast(TEXT("Inspect S-03 on the left status terminal before selecting local service."), 5); return true;
  }
  if (ArrayWork && ArrayWork->Use(Id)) return true;
  if (Id == TEXT("crtLeft")) { OpenArrayStatus(); return true; }
  if (Id == TEXT("crtRight")) { OpenTracking(); return true; }
  if (Id == TEXT("telex")) { OpenWorkOrder(); return true; }
  if (Id == TEXT("decoder")) { OpenAudioComparison(); return true; }
  if (Id == TEXT("printer") && Run == 2 && Archive == 0) { CollectPacket(); return true; }
 }
 if (Id == TEXT("obs:generator")) { UseGenerator(); return true; }
 if (PulsarCheckActive())
 {
  if (Id == TEXT("rack")) { if (PulsarLoop == ELoopStage::Rack) StartPulsarControl(); else if (M && M->UI) M->UI->Toast(PulsarObjective(), 4); return true; }
  if (Id == TEXT("printer"))
  {
   if (PulsarLoop == ELoopStage::AwaitPulsar) SecurePulsarChart();
   else if (PulsarLoop == ELoopStage::ControlPrint) CollectPulsarControl();
   else if (M && M->UI) M->UI->Toast(PulsarObjective(), 4);
   return true;
  }
  if (Id == TEXT("opsTerminal"))
  {
   if (PulsarLoop == ELoopStage::Compare) OpenPulsarComparison();
   else if (M && M->UI) M->UI->Toast(PulsarObjective(), 4);
   return true;
  }
 }
 if (Id == TEXT("binder") && bWorkEnabled && Active() && Run == 2 && Archive == 0 && !bPacketCollected)
 {
  M->UI->Toast(TEXT("The registry entry is ready. Collect the labelled packet at the printer before placing it in this binder."), 5); return true;
 }
 if (Label(Id).IsEmpty()) return false;
 if (Id == TEXT("binder")) { UseBinder(); return true; }
 File();
 return true;
}

void FS47Observing::File()
{
 const FRun& R = Runs()[Run];
 AS47Story& S = *M->Story;
 const bool bComparedPulsar = Run == 1 && PulsarLoop == ELoopStage::Filed;
 FS47Doc Doc = bComparedPulsar ? PulsarComparison() : Chart(Run);
 if (bComparedPulsar && bWorkEnabled)
 {
  Doc.Stamp = WorkReference() + TEXT(" / REGISTERED");
  Doc.Page.ReplaceInline(TEXT("NEXT: file the charts in the records-room run binder, then reduce HI strip 12."), TEXT("NEXT: collect the labelled packet at the printer and place it in the April 13 run binder, then reduce HI strip 12."));
  Doc.Page += FString::Printf(TEXT("\n\nMARKED ORIGINAL INTERVAL %.1f TO %.1f SECONDS.\nDIGITAL REGISTRATION COMPLETE. PHYSICAL FILING PENDING."), MarkStart, MarkEnd);
  Doc.Transcript = Doc.Page;
 }
 S.AddDoc(Doc);
 if (bComparedPulsar)
  S.Note(FString::Printf(TEXT("%s. Pulsar period normal. Extra ticks do not follow its beat. A short terminated-input/internal-reference check was regular, without extra ticks. Source unresolved. Both charts retained. %s"), *S.ClockText(false),
   bWorkEnabled ? TEXT("Registered SARO-860413-02; collect the labelled packet at the printer, then place its originals in the April 13 binder.") : TEXT("Next file them in the records-room binder.")));
 else S.Note(FString(R.P.Note));
 if (M->Audio) M->Audio->Play(R.Data == FString(TEXT("printer")) ? TEXT("paper0") : TEXT("click"), .5f);
 // The run's own length passes on the clock; the next source waits for its slot.
 S.AdvanceClockTo(R.Slot + R.Minutes * 60, TEXT("observation data filed"));
 ++Run; StepNow = EStep::Tune; Elapsed = 0;
 S.Rx.Status = bComparedPulsar && bWorkEnabled ? FString(TEXT("REGISTERED SARO-860413-02 / PACKET AT PRINTER / PHYSICAL FILING PENDING"))
  : Run < Runs().Num() ? FString::Printf(TEXT("NEXT  /  %s"), Runs()[Run].Source).ToUpper() : HeldStatus;
 M->UI->Document(Doc);
 M->UI->Toast(bComparedPulsar && bWorkEnabled ? FString(TEXT("Registration complete. Collect SARO-860413-02 at the printer, then place its originals in the records-room binder."))
  : Run < Runs().Num() ? FString::Printf(TEXT("%s. Chart filed. Next on the schedule: %s."), *S.ClockText(false), Runs()[Run].Source)
  : FString::Printf(TEXT("%s. The evening schedule is done. The north-window lamp round is next."), *S.ClockText(false)), 5);
 UE_LOG(LogTemp, Display, TEXT("S47: observing run %d %s filed at clock %.0f"), Run, R.Id, S.Clock());
 Changed();
}

bool FS47Observing::PulsarCheckActive() const
{
 return Active() && Run == 1 && StepNow == EStep::Data && PulsarLoop >= ELoopStage::AwaitPulsar && PulsarLoop < ELoopStage::Filed;
}

FString FS47Observing::PulsarObjective() const
{
 switch (PulsarLoop)
 {
  case ELoopStage::AwaitPulsar: return TEXT("PRINTER // SECURE THE PULSAR CHART BEFORE CHANGING THE RECEIVER INPUT");
  case ELoopStage::Rack: return TEXT("RECEIVER RACK // TEST THE TERMINATED INPUT WITH THE INTERNAL REFERENCE");
  case ELoopStage::ControlRunning: return TEXT("RECEIVER RACK // INTERNAL REFERENCE RUNNING. LISTEN; THE PRINTER WILL RECORD THE CHECK");
  case ELoopStage::ControlPrint: return TEXT("PRINTER // COLLECT THE INTERNAL-REFERENCE CONTROL CHART");
  case ELoopStage::Compare: return bWorkEnabled && !bAudioReviewed ? FString(TEXT("SIGNAL PROCESSOR // REVIEW BOTH RECORDINGS AND MARK THE EXTRA TICKS"))
   : FString(TEXT("OPERATIONS TERMINAL // COMPARE THE PULSAR AND CONTROL CHARTS, THEN REGISTER WHAT THEY SUPPORT"));
  default: return FString();
 }
}

FS47Doc FS47Observing::PulsarControl()
{
 FS47Doc D; D.Id = TEXT("obs_control_b1133"); D.Title = TEXT("Receiver control: terminated input and internal reference");
 D.Kind = TEXT("printout"); D.Stamp = TEXT("SARO / RX CHECK");
 D.Page = WorkReference() + TEXT("\nSARO / RECEIVER CONTROL AFTER PSR B1133+16\n\n")
  TEXT("EXTERNAL INPUT TERMINATED\nINTERNAL TIMING REFERENCE ENABLED\nOBSERVATION WINDOW 8 SECONDS\n\n")
  TEXT("REFERENCE TRAIN 1.18791 S / REGULAR\nEXTRA TICKS IN THIS CHECK: NONE\n\n")
  TEXT("The external input was disconnected for this short bench check.\n")
  TEXT("A quiet check does not identify the source of an intermittent event.\n")
  TEXT("Retain with the original pulsar chart for comparison.");
 D.Transcript = TEXT("The receiver was switched to a terminated input and its own internal timing reference for eight seconds. The reference pulses were regular and no extra ticks were recorded in this check. This narrows the observation; it does not prove where the earlier extra ticks came from.");
 return D;
}

FS47Doc FS47Observing::PulsarComparison()
{
 FS47Doc D; D.Id = TEXT("obs_compare_b1133"); D.Title = TEXT("Pulsar discrepancy: comparison filed");
 D.Kind = TEXT("typed"); D.Stamp = TEXT("SARO / REYES");
 D.Page = WorkReference() + TEXT("\nSARO / PULSAR DISCREPANCY COMPARISON\n\n")
  TEXT("1. The pulsar period agrees with the catalogue.\n")
  TEXT("2. The extra ticks do not follow that period.\n")
  TEXT("3. The short terminated-input/internal-reference check was regular.\n")
  TEXT("   No extra ticks were recorded during that check.\n\n")
  TEXT("SOURCE UNRESOLVED. A quiet control does not establish a sky source\n")
  TEXT("and does not exclude an intermittent local cause.\n\n")
  TEXT("Original pulsar and control charts retained.\nNEXT: file the charts in the records-room run binder, then reduce HI strip 12.");
 D.Transcript = TEXT("The pulsar itself kept its normal period. The extra ticks were outside that rhythm, and the subsequent short receiver control did not reproduce them. Their source remains unresolved. Both charts are retained. The next task is to file them in the records-room binder before the hydrogen drift scan.");
 return D;
}

void FS47Observing::SecurePulsarChart()
{
 if (!PulsarCheckActive() || PulsarLoop != ELoopStage::AwaitPulsar) return;
 AS47Story& S = *M->Story;
 const FS47Doc D = Chart(1); S.AddDoc(D);
 S.Note(FString::Printf(TEXT("%s. Secured the B1133+16 strip chart before changing the receiver input. The folded period is normal; the extra ticks fall outside it. Next: a local receiver control."), *S.ClockText(false)));
 S.AdvanceClockTo(Runs()[1].Slot + Runs()[1].Minutes * 60, TEXT("pulsar source chart secured"));
 PulsarLoop = ELoopStage::Rack;
 if (M->Audio) M->Audio->Play(TEXT("paper0"), .5f);
 M->UI->Document(D); M->UI->Toast(TEXT("Pulsar chart retained. Check the receiver rack on a terminated input before drawing a conclusion."), 5);
 UE_LOG(LogTemp, Display, TEXT("S47: SARO loop pulsar chart secured")); Changed();
}

void FS47Observing::StartPulsarControl()
{
 if (!PulsarCheckActive() || PulsarLoop != ELoopStage::Rack) return;
 PulsarLoop = ELoopStage::ControlRunning; ControlT = ControlTick = 0; CompareFeedback.Empty();
 M->Story->Rx.Status = TEXT("LOCAL CONTROL / INPUT TERMINATED / INTERNAL REFERENCE");
 if (M->Audio) M->Audio->Play(TEXT("rack_switch"), .5f, M->Web ? M->Web->PropLocation(TEXT("rack")) : FVector::ZeroVector);
 M->UI->Toast(TEXT("Input terminated. The internal reference clicks at a steady interval. Eight-second control recording started; collect the chart at the printer."), 5);
 UE_LOG(LogTemp, Display, TEXT("S47: SARO loop receiver control started")); Changed();
}

void FS47Observing::StepPulsarControl(double Dt)
{
 if (PulsarLoop != ELoopStage::ControlRunning || !PulsarCheckActive() || M->ModalOpen() || M->Story->IsCinematic()) return;
 const double Was = ControlT;
 ControlT = FMath::Min(ControlSeconds, ControlT + FMath::Min(Dt, .1));
 ControlTick += ControlT - Was;
 while (ControlTick >= PulsarPeriod)
 {
  ControlTick -= PulsarPeriod;
  if (M->Audio) M->Audio->Beep(820, .012f, .16f, 0, TEXT("square"));
 }
 M->Story->Rx.Status = FString::Printf(TEXT("INTERNAL REFERENCE / %02d S / REGULAR"), int32(FMath::CeilToDouble(ControlSeconds - ControlT)));
 if (ControlT < ControlSeconds) return;
 PulsarLoop = ELoopStage::ControlPrint;
 M->Story->Rx.Status = TEXT("CONTROL COMPLETE / RESTORED EXTERNAL INPUT / COLLECT CHART");
 if (M->Audio)
 {
  M->Audio->Play(TEXT("rack_switch"), .4f, M->Web ? M->Web->PropLocation(TEXT("rack")) : FVector::ZeroVector);
  if (!Recorded(M->Audio, TEXT("obsChartPrinter"), .5f, 1, PrinterAt)) M->Audio->Play(TEXT("printer"), .35f, PrinterAt, 1.2f);
 }
 M->UI->Toast(TEXT("Receiver control complete. The external input is restored. The printer has the eight-second reference chart."), 4);
 UE_LOG(LogTemp, Display, TEXT("S47: SARO loop receiver control completed")); Changed();
}

void FS47Observing::CollectPulsarControl()
{
 if (!PulsarCheckActive() || PulsarLoop != ELoopStage::ControlPrint) return;
 const FS47Doc D = PulsarControl(); M->Story->AddDoc(D);
 PulsarLoop = ELoopStage::Compare;
 if (M->Audio) M->Audio->Play(TEXT("paper0"), .5f);
 M->UI->Document(D); M->UI->Toast(bWorkEnabled ? TEXT("Both charts retained. At the signal processor, compare the original and control recordings before registering the result.") : TEXT("Both charts retained. Use the operations terminal to compare what they support."), 5);
 UE_LOG(LogTemp, Display, TEXT("S47: SARO loop control chart secured")); Changed();
}

void FS47Observing::OpenPulsarComparison()
{
 if (!PulsarCheckActive() || PulsarLoop != ELoopStage::Compare || !M->UI) return;
 if (bWorkEnabled && !bAudioReviewed)
 {
  M->UI->Toast(TEXT("Review both recordings at the signal processor and mark the extra ticks before registering a conclusion."), 5); return;
 }
 TArray<S47Kit::FW> Rows;
 if (bWorkEnabled) Rows.Add(S47Kit::Small(WorkReference() + TEXT(" / ORIGINAL + INTERNAL CONTROL / REGISTRATION PENDING")));
 Rows.Add(S47Kit::Text(TEXT("Compare the pointing check, pulsar chart and receiver control before filing.")));
 Rows.Add(S47Kit::Paper(TEXT("NORMAL REFERENCE / 3C 286 pointing check: clean cross, in specification.\n\nPULSAR CHART / period 1.18791 s, as catalogued. Extra ticks: four, a gap, seven; not at the pulsar period.\n\nLOCAL CONTROL / external input terminated, internal reference regular. No extra ticks in the eight-second check.")));
 if (!CompareFeedback.IsEmpty()) Rows.Add(S47Kit::Paper(CompareFeedback));
 Rows.Add(S47Kit::Question(TEXT("What can these records establish?")));
 const auto First = S47Kit::Button(TEXT("These are normal pulsar ticks."), [this] { ComparePulsar(TEXT("pulsar")); OpenPulsarComparison(); });
 Rows.Add(First);
 Rows.Add(S47Kit::Button(TEXT("The control proves a sky source."), [this] { ComparePulsar(TEXT("sky")); OpenPulsarComparison(); }));
 Rows.Add(S47Kit::Button(TEXT("Source unresolved. Retain both charts."), [this] { if (!ComparePulsar(TEXT("unresolved"))) OpenPulsarComparison(); }, true));
 M->UI->Open(S47Kit::Shell(M->UI, TEXT("SARO / PULSAR CONTROL COMPARISON"), S47Kit::Col(Rows)));
 if (FSlateApplication::IsInitialized()) FSlateApplication::Get().SetKeyboardFocus(First, EFocusCause::SetDirectly);
}

bool FS47Observing::ComparePulsar(const FString& Choice)
{
 if (!bEnabled || Run != 1 || StepNow != EStep::Data || PulsarLoop != ELoopStage::Compare || (M && !Active())) return false;
 if (bWorkEnabled && !bAudioReviewed) { CompareFeedback = TEXT("The recordings have not been compared and marked at the signal processor."); Changed(); return false; }
 if (Choice != TEXT("unresolved"))
 {
  CompareFeedback = Choice == TEXT("pulsar")
   ? TEXT("The pulsar's folded period agrees with the catalogue. The extra ticks fall outside that rhythm. Compare the separate entries on the original chart; the normal pointing reference does not turn those extra ticks into pulsar pulses.")
   : TEXT("A short quiet bench check does not prove a sky source. It did not reproduce the extra ticks, but an intermittent local cause is still possible. Keep the conclusion within what both charts establish.");
  Changed(); return false;
 }
 CompareFeedback.Empty(); PulsarLoop = ELoopStage::Filed;
 if (M && M->UI && M->Story)
 {
  M->UI->Close(true);
  File();
  UE_LOG(LogTemp, Display, TEXT("S47: SARO loop comparison filed; source unresolved"));
 }
 Changed(); return true;
}

// ---------- active workstation order, transport and registry ----------

FS47Doc FS47Observing::WorkOrder()
{
 FS47Doc D; D.Id = TEXT("obs_work_order"); D.Title = TEXT("Telex: evening observing work order"); D.Kind = TEXT("telex"); D.Stamp = TEXT("SARO OPS / 13 APR 86");
 D.Page = TEXT("SARO / EVENING WORK ORDER\n\nSTATUS TERMINAL: inspect individual drives and command history.\nTRACKING TERMINAL: select the scheduled source and command pointing.\nRECEIVER: start acquisition only after the array settles on target.\n\nAfter the generator transfer, verify S-03 remote availability.\nIf feedback fails, use the S-03 local cabinet in the east service yard.\n\nPULSAR REVIEW / SARO-860413-02\nKeep the original chart before changing the receiver input.\nRecord the internal-reference control separately. At the signal processor,\nreview both recordings and mark the extra ticks on the original timeline.\nRegister the supported conclusion at OPS, collect its labelled packet\nand place the original charts behind APRIL 13 in the records-room binder.\n\nThe receiver test is independent of routine drive service.\nA quiet control cannot establish the origin of an intermittent event.");
 D.Transcript = D.Page; return D;
}

void FS47Observing::OpenWorkOrder()
{
 if (!bWorkEnabled || !Active() || !M->UI) return;
 const FS47Doc D = WorkOrder(); M->Story->AddDoc(D); bWorkOrderFiled = true;
 M->UI->Document(D); Changed();
}

FString FS47Observing::DishStatus(int32 Index) const
{
 if (!M || !M->Web || Index < 0 || Index >= M->Web->DishCount()) return TEXT("NO ARRAY FEEDBACK");
 const AS47Web* W = M->Web;
 return FString::Printf(TEXT("%s  ACT %05.1f/%04.1f  CMD %05.1f/%04.1f  %s"), *W->DishId(Index), W->DishAz(Index), W->DishEl(Index), W->DishTargetAz(Index), W->DishTargetEl(Index),
  !W->DishRemoteAvailable(Index) ? TEXT("LOCAL / UNAVAILABLE") : W->DishMoving(Index) ? TEXT("MOVING") : TEXT("HOLD"));
}

void FS47Observing::SelectStatusDish(int32 Index)
{
 SelectedDish = FMath::Clamp(Index, 0, 26);
 if (M && M->Web && ArrayWork && ArrayWork->ServiceRequired() && M->Web->DishId(SelectedDish) == TEXT("S-03"))
 {
  bS03Diagnosed = true; Changed();
 }
 if (M && Active()) OpenArrayStatus();
}

void FS47Observing::OpenArrayStatus()
{
 if (!bWorkEnabled || !Active() || !ArrayWork || !M->UI) return;
 ArrayWork->SetContext(Run, Run == 1 && GenStep >= 3);
 const TArray<FString> Lines = ArrayWork->StatusRows();
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Small(TEXT("Select an antenna to inspect its actual pointing and local/remote state.")));
 Rows.Add(SNew(STextBlock).Text_Lambda([this] { return FText::FromString(DishStatus(SelectedDish)); }).Font(S47Style::Mono(18)).ColorAndOpacity(S47Kit::Css(0xe4be79)));
 Rows.Add(SNew(STextBlock).Text_Lambda([this] { return FText::FromString(ArrayWork->StateText()); }).Font(S47Style::Mono(16)).ColorAndOpacity(S47Kit::Css(0xc4bb9b)));
 TSharedPtr<SWidget> FileButton;
 if (StepNow == EStep::Data && Run == 3)
 {
  FileButton = S47Kit::Button(TEXT("File Virgo A drive log"), [this] { M->UI->Close(); File(); }, true);
  Rows.Add(FileButton.ToSharedRef());
 }
 TSharedRef<SScrollBox> List = SNew(SScrollBox);
 for (int32 I = 0; I < FMath::Min(27, Lines.Num()); ++I)
 {
  const int32 Index = I;
  List->AddSlot()[S47Kit::ButtonLive(TAttribute<FString>::CreateLambda([this, Index] { return DishStatus(Index); }), [this, Index] { SelectStatusDish(Index); }, Index == SelectedDish)];
 }
 Rows.Add(SNew(SBox).HeightOverride(235)[List]);
 if (Lines.Num() > 27)
 {
  TArray<FString> History;
  for (int32 I = FMath::Max(27, Lines.Num() - 3); I < Lines.Num(); ++I) History.Add(Lines[I]);
  Rows.Add(S47Kit::Small(FString::Join(History, TEXT("\n"))));
 }
 M->UI->Open(S47Kit::Shell(M->UI, TEXT("SARO / ARRAY STATUS AND COMMAND LOG"), S47Kit::Col(Rows), 940), [this] { bTelemetryPanelOpen = false; });
 bTelemetryPanelOpen = true;
 if (FileButton && FSlateApplication::IsInitialized()) FSlateApplication::Get().SetKeyboardFocus(FileButton, EFocusCause::SetDirectly);
}

void FS47Observing::OpenTracking()
{
 if (!bWorkEnabled || !Active() || !ArrayWork || !M->UI) return;
 ArrayWork->SetContext(Run, Run == 1 && GenStep >= 3);
 const FRun& R = Runs()[Run];
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Paper(FString::Printf(TEXT("SCHEDULE %d / %s\nCOMMAND AZ %.1f  EL %.1f"), Run + 1, R.Source, (R.P.Az[0] + R.P.Az[1]) / 2, R.El)));
 Rows.Add(SNew(STextBlock).Text_Lambda([this] { return FText::FromString(ArrayWork->StateText() + TEXT("\n") + DishStatus(SelectedDish)); }).Font(S47Style::Mono(18)).ColorAndOpacity(S47Kit::Css(0xe4be79)));
 Rows.Add(S47Kit::Small(Run == 0 ? TEXT("The pointing check samples centre, north, south, east and west after acquisition starts.")
  : Run == 2 ? TEXT("The drift scan holds the commanded direction while the sky crosses the beam.") : TEXT("Pointing is complete only when actual antenna feedback is steady on target.")));
 const auto Command = S47Kit::Button(TEXT("Point the array to this source"), [this] { PointScheduledRun(); }, true,
  StepNow == EStep::Tune && !ChorePending());
 Rows.Add(Command);
 Rows.Add(S47Kit::Small(TEXT("Live feedback continues here. Close to watch the dishes, then acquire at the receiver.")));
 M->UI->Open(S47Kit::Shell(M->UI, TEXT("SARO / TRACKING TERMINAL"), S47Kit::Col(Rows), 900), [this] { bTelemetryPanelOpen = false; });
 bTelemetryPanelOpen = true;
 if (FSlateApplication::IsInitialized()) FSlateApplication::Get().SetKeyboardFocus(Command, EFocusCause::SetDirectly);
}

bool FS47Observing::PointScheduledRun()
{
 if (!bWorkEnabled || !Active() || !ArrayWork || StepNow != EStep::Tune) return false;
 ArrayWork->SetContext(Run, Run == 1 && GenStep >= 3);
 if (ChorePending()) { M->UI->Toast(Objective(), 5); return false; }
 const FRun& R = Runs()[Run];
 M->Story->AdvanceClockTo(R.Slot, TEXT("scheduled source selected at tracking terminal"));
 const bool bOk = ArrayWork->CommandObservation(Run, (R.P.Az[0] + R.P.Az[1]) / 2, R.El, Run == 0);
 if (bOk)
 {
  M->UI->Close(); M->UI->Toast(TEXT("Pointing command sent. Watch the array move; the receiver waits for steady on-target feedback."), 5);
  Changed();
 }
 return bOk;
}

namespace
{
 class SS47ObservationTrace : public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SS47ObservationTrace) {} SLATE_ARGUMENT(FS47Observing*, Observer) SLATE_END_ARGS()
  void Construct(const FArguments& A) { Observer = A._Observer; White = MakeUnique<FSlateColorBrush>(FLinearColor::White); }
  virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(790, 88); }
  virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
  {
   if (!Observer) return Layer;
   const float W = G.GetLocalSize().X; const double Duration = Observer->AudioSource() == 0 ? 18. : 8.;
   const auto Box = [&](float X, float Y, float Dx, float Dy, FLinearColor Colour)
   { FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(Dx, Dy), FSlateLayoutTransform(FVector2f(X, Y))), White.Get(), ESlateDrawEffect::None, Colour); };
   Box(0, 0, W, 88, S47Kit::Css(0x07120c));
   const auto X = [&](double T) { return float(12 + FMath::Clamp(T / Duration, 0., 1.) * (W - 24)); };
   if (Observer->AudioSource() == 0 && Observer->AudioMarkedStart() >= 0 && Observer->AudioMarkedEnd() > Observer->AudioMarkedStart())
    Box(X(Observer->AudioMarkedStart()), 10, X(Observer->AudioMarkedEnd()) - X(Observer->AudioMarkedStart()), 57, S47Kit::Css(0x66582c, .55f));
   Box(12, 58, W - 24, 1, S47Kit::Css(0x346c49));
   for (double T = PulsarPeriod; T <= Duration; T += PulsarPeriod) Box(X(T), 23, 2, 35, S47Kit::Css(0x84c796));
   if (Observer->AudioSource() == 0) for (double T : StrayTicks) Box(X(T), 41, 2, 17, S47Kit::Css(0xf1b950));
   for (int32 T = 0; T <= int32(Duration); ++T) Box(X(T), 62, 1, T % 2 == 0 ? 8 : 4, S47Kit::Css(0x59705b));
   Box(X(Observer->AudioPosition()), 7, 2, 66, S47Kit::Css(0xf3efe1));
   FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(W, 15), FSlateLayoutTransform(FVector2f(12, 73))),
    FString::Printf(TEXT("0 s                                      %.0f s / retained recording timing"), Duration), S47Style::Mono(12), ESlateDrawEffect::None, S47Kit::Css(0xa3c4aa));
   return Layer + 2;
  }
 private:
  FS47Observing* Observer = nullptr;
  TUniquePtr<FSlateColorBrush> White;
 };
}

bool FS47Observing::AudioAvailable() const
{
 return bWorkEnabled && PulsarCheckActive() && PulsarLoop == ELoopStage::Compare;
}

bool FS47Observing::AudioCoverageComplete() const
{
 return AudioCovered[0] >= Runs()[1].Seconds && AudioCovered[1] >= ControlSeconds;
}

FString FS47Observing::AudioDisplay() const
{
 if (!AudioAvailable()) return FString();
 return FString::Printf(TEXT("%s %04.1f/%02d %s"), AudioSelected == 0 ? TEXT("ORG") : TEXT("REF"), AudioCursor[AudioSelected], AudioSelected == 0 ? 18 : 8,
  bAudioPlaying ? TEXT("PLAY") : bAudioReviewed ? TEXT("MARKED") : TEXT("PAUSE"));
}

double FS47Observing::AudioLevel() const
{
 if (!AudioAvailable() || !bAudioPlaying) return 0;
 const double T = AudioCursor[AudioSelected];
 double Level = FMath::Fmod(T, PulsarPeriod) < .10 ? .8 : .05;
 if (AudioSelected == 0) for (double At : StrayTicks) if (T >= At && T < At + .08) Level = FMath::Max(Level, .45);
 return Level;
}

void FS47Observing::OpenAudioComparison()
{
 if (!bWorkEnabled || !Active() || !M->UI) return;
 if (!AudioAvailable()) { M->UI->Toast(TEXT("Recording inputs: original receiver and internal reference. Retain both charts before comparing the recordings."), 5); return; }
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Small(WorkReference() + TEXT(" / TWO SEPARATE RECORDINGS / ORIGINAL RETAINED")));
 Rows.Add(SNew(STextBlock).Text_Lambda([this] { return FText::FromString(AudioDisplay()); }).Font(S47Style::Mono(19)).ColorAndOpacity(S47Kit::Css(0xe4be79)));
 Rows.Add(SNew(SS47ObservationTrace).Observer(this));
 Rows.Add(S47Kit::Row({
  S47Kit::Choice(TEXT("Original / 18 s"), TAttribute<bool>::CreateLambda([this] { return AudioSelected == 0; }), [this] { AudioAction(TEXT("original")); OpenAudioComparison(); }),
  S47Kit::Choice(TEXT("Internal control / 8 s"), TAttribute<bool>::CreateLambda([this] { return AudioSelected == 1; }), [this] { AudioAction(TEXT("control")); OpenAudioComparison(); }) }));
 Rows.Add(S47Kit::Row({ S47Kit::Button(TEXT("Play / pause"), [this] { AudioAction(bAudioPlaying ? TEXT("pause") : TEXT("play")); }, true),
  S47Kit::Button(TEXT("Rewind"), [this] { AudioAction(TEXT("rewind")); }) }));
 Rows.Add(S47Kit::Slider(TEXT("Review position / seconds"), 0, AudioSelected == 0 ? 18.f : 8.f, .1f,
  TAttribute<float>::CreateLambda([this] { return float(AudioCursor[AudioSelected]); }), [this](float At) { SeekAudio(At); }));
 Rows.Add(S47Kit::Row({ S47Kit::Button(TEXT("Mark interval start"), [this] { AudioAction(TEXT("mark_start")); }),
  S47Kit::Button(TEXT("Mark interval end"), [this] { AudioAction(TEXT("mark_end")); }) }));
 Rows.Add(SNew(STextBlock).Text_Lambda([this]
 {
  return FText::FromString(FString::Printf(TEXT("ORIGINAL %s / CONTROL %s / MARK %.1f to %.1f s\n%s"),
   AudioCoverage[0] == ((1u << 18) - 1) ? TEXT("reviewed") : TEXT("pending"), AudioCoverage[1] == ((1u << 8) - 1) ? TEXT("reviewed") : TEXT("pending"), MarkStart, MarkEnd,
   AudioFeedback.IsEmpty() ? TEXT("Review both traces, then mark the extra ticks on the original timeline.") : *AudioFeedback));
 }).Font(S47Style::Mono(15)).ColorAndOpacity(S47Kit::Css(0xc4bb9b)).AutoWrapText(true));
 Rows.Add(S47Kit::Button(TEXT("Retain marked comparison"), [this] { AudioAction(TEXT("confirm")); }));
 M->UI->Open(S47Kit::Shell(M->UI, TEXT("SARO / SIGNAL PROCESSOR / RECORDING COMPARISON"), S47Kit::Col(Rows), 920), [this]
 { bAudioPanelOpen = bAudioPlaying = false; Changed(); });
 bAudioPanelOpen = true;
}

void FS47Observing::SeekAudio(double Seconds)
{
 if (!AudioAvailable() || !FMath::IsFinite(Seconds)) return;
 bAudioPlaying = false;
 AudioCursor[AudioSelected] = FMath::Clamp(Seconds, 0., AudioSelected == 0 ? Runs()[1].Seconds : ControlSeconds);
 Changed();
}

bool FS47Observing::AudioAction(const FString& Action)
{
 if (!AudioAvailable()) return false;
 if (Action == TEXT("original") || Action == TEXT("control"))
 {
  AudioSelected = Action == TEXT("original") ? 0 : 1; bAudioPlaying = false;
 }
 else if (Action == TEXT("play"))
 {
  if (!bAudioPanelOpen) return false;
  if (AudioCursor[AudioSelected] >= (AudioSelected == 0 ? Runs()[1].Seconds : ControlSeconds)) AudioCursor[AudioSelected] = 0;
  bAudioPlaying = true;
 }
 else if (Action == TEXT("pause")) bAudioPlaying = false;
 else if (Action == TEXT("rewind")) { bAudioPlaying = false; AudioCursor[AudioSelected] = 0; }
 else if (Action == TEXT("mark_start") || Action == TEXT("mark_end"))
 {
  if (AudioSelected != 0) { AudioFeedback = TEXT("Mark the extra ticks on the original recording, not the control."); return false; }
  if (Action == TEXT("mark_start")) MarkStart = AudioCursor[0]; else MarkEnd = AudioCursor[0];
  bAudioReviewed = false;
 }
 else if (Action == TEXT("confirm"))
 {
  if (!AudioCoverageComplete()) { AudioFeedback = TEXT("Play both recordings through. Moving the review cursor does not play the missing interval."); Changed(); return false; }
  if (MarkStart < 0 || MarkStart > StrayTicks[0] || MarkEnd < StrayTicks[UE_ARRAY_COUNT(StrayTicks) - 1] || MarkEnd - MarkStart > 6)
  { AudioFeedback = TEXT("Mark the short group of extra ticks: four, a gap, then seven. Include both groups."); Changed(); return false; }
  const bool bFirstReview = !bAudioReviewed;
  bAudioReviewed = true; bAudioPlaying = false;
  if (bFirstReview) M->Story->Note(FString::Printf(TEXT("%s / %s. Reviewed original and control recordings. Marked %.1f to %.1f s on the original: extra ticks outside the pulsar period; the short internal control has none."), *M->Story->ClockText(false), *WorkReference(), MarkStart, MarkEnd));
  M->UI->Close(); M->UI->Toast(TEXT("Marked comparison retained. Register what it supports at the operations terminal."), 5);
 }
 else return false;
 AudioFeedback.Empty(); Changed(); return true;
}

void FS47Observing::CreditAudioRange(int32 Source, double Was, double Now)
{
 if (Source < 0 || Source > 1 || !FMath::IsFinite(Was) || !FMath::IsFinite(Now) || Now < Was || Was < 0) return;
 const double Duration = Source == 0 ? Runs()[1].Seconds : ControlSeconds;
 // A seek may move the cursor, but only continuous playback can extend the heard prefix.
 if (Was <= AudioCovered[Source] + 1e-7)
  AudioCovered[Source] = FMath::Clamp(FMath::Max(AudioCovered[Source], Now), 0., Duration);
 AudioCoverage[Source] = (1u << int32(AudioCovered[Source])) - 1;
}

void FS47Observing::StepAudio(double Dt)
{
 if (!bAudioPlaying || !bAudioPanelOpen || !AudioAvailable() || !M || !M->UI || !M->UI->Modal() || M->Story->IsCinematic()) return;
 const double Duration = AudioSelected == 0 ? Runs()[1].Seconds : ControlSeconds;
 const double Was = AudioCursor[AudioSelected];
 const double Now = FMath::Min(Duration, Was + FMath::Min(Dt, .1));
 AudioCursor[AudioSelected] = Now;
 CreditAudioRange(AudioSelected, Was, Now);
 if (M->Audio)
 {
  for (double At = PulsarPeriod; At <= Duration; At += PulsarPeriod)
   if (Was < At && Now >= At)
   {
    if (AudioSelected == 1 || !Recorded(M->Audio, TEXT("obsPulsarPulse"), .4f)) M->Audio->Beep(820, .012f, .16f, 0, TEXT("square"));
   }
  if (AudioSelected == 0) for (double At : StrayTicks)
   if (Was < At && Now >= At && !Recorded(M->Audio, TEXT("obsPulsarPulse"), .14f, 1.7f)) M->Audio->Beep(1240, .008f, .07f, 0, TEXT("square"));
 }
 if (Now >= Duration) { bAudioPlaying = false; Changed(); }
}

void FS47Observing::CollectPacket()
{
 if (!bWorkEnabled || !Active() || Run != 2 || Archive != 0 || PulsarLoop != ELoopStage::Filed) return;
 if (bPacketCollected) { M->UI->Toast(TEXT("The labelled packet is already collected. Place it behind April 13 in the records-room binder."), 4); return; }
 FS47Doc D; D.Id = TEXT("obs_packet_b1133"); D.Title = TEXT("Labelled observation packet: ") + WorkReference(); D.Kind = TEXT("printout"); D.Stamp = WorkReference();
 D.Page = WorkReference() + TEXT("\nREGISTRATION COMPLETE / PHYSICAL FILING PENDING\n\nORIGINAL: PSR B1133+16 timing chart and receiver recording.\nCONTROL: eight-second internal-reference chart and recording.\nMARK: ") + FString::Printf(TEXT("%.1f to %.1f s on the original.\n"), MarkStart, MarkEnd)
  + TEXT("RESULT: extra ticks unresolved. Both records retained.\n\nPLACE THE ORIGINALS WITH THIS COVER IN THE APRIL 13 RUN BINDER.");
 D.Transcript = D.Page; M->Story->AddDoc(D); bPacketCollected = true;
 if (M->Audio) M->Audio->Play(TEXT("paper0"), .5f);
 M->UI->Document(D); M->UI->Toast(TEXT("Labelled packet collected. Place it behind April 13 in the records-room run binder."), 5); Changed();
}

FString FS47Observing::Objective() const
{
 if (!Active()) return FString();
 if (PulsarCheckActive()) return PulsarObjective();
 const FRun& R = Runs()[Run];
 if (bWorkEnabled && ArrayWork && ArrayWork->ServiceRequired()) return bS03Diagnosed ? ArrayWork->Objective() : TEXT("STATUS TERMINAL // SELECT S-03 AND INSPECT ITS REMOTE-LINK FAULT BEFORE FIELD SERVICE");
 if (bWorkEnabled && Run == 2 && Archive == 0 && !bPacketCollected) return TEXT("PRINTER // COLLECT THE LABELLED PACKET SARO-860413-02, THEN PLACE IT IN THE RUN BINDER");
 if (Run == 1 && StepNow == EStep::Tune && GenStep < 3)
  return GenStep == 0 ? TEXT("CONTROL ROOM // 19:35 WEEKLY GENERATOR TEST ON THE REMOTE PANEL BY THE EAST DOOR")
   : GenStep == 1 ? TEXT("CONTROL ROOM // WHEN THE DIESEL SETTLES, TRANSFER THE LOAD TO THE GENERATOR") : TEXT("CONTROL ROOM // CHECK THE EMERGENCY LIGHTING, THEN TRANSFER BACK TO LINE POWER");
 if (Run == 2 && StepNow == EStep::Tune && Archive >= 0 && Archive < 2)
  return Archive == 0 ? TEXT("RECORDS ROOM // 20:30 FILE THE EVENING'S STRIP CHARTS IN THE RUN BINDER, DOWN THE CORRIDOR")
   : TEXT("RECORDS ROOM // BACK TO THE CONTROL ROOM FOR THE HYDROGEN STRIP");
 if (Run == 3 && StepNow == EStep::Tune && Yard < 2)
  return Yard == 0 ? (M->Web && M->Web->IsDoorOpen(TEXT("fieldGate")) ? TEXT("ARRAY GATE // 21:40 ROUND: THE ARRAY GATE IS OPEN. CLOSE IT") : TEXT("ARRAY GATE // 21:40 ROUND: WALK OUT TO THE ARRAY GATE AND CHECK THE CHAIN"))
   : TEXT("ARRAY GATE // RETURN TO THE CONTROL ROOM");
 if (StepNow == EStep::Integrating) return FString::Printf(TEXT("CONTROL ROOM // %s INTEGRATING. LISTEN TO THE MONITOR AND WATCH THE CHART"), R.Source).ToUpper();
 if (StepNow == EStep::Data) return R.Data == FString(TEXT("printer")) ? FString::Printf(TEXT("PRINTER // TEAR OFF THE %s STRIP CHART"), R.Source).ToUpper()
  : R.Data == FString(TEXT("crtLeft")) ? FString(TEXT("STATUS TERMINAL // READ THE ARRAY DRIVE LOG")) : FString(TEXT("OPERATIONS TERMINAL // REDUCE THE HYDROGEN DRIFT SCAN"));
 if (bWorkEnabled && ArrayWork && (!ArrayWork->HasCommand() || ArrayWork->CommandedRun() != Run))
  return FString::Printf(TEXT("TRACKING TERMINAL // SELECT %s AND POINT THE ARRAY"), R.Source).ToUpper();
 if (bWorkEnabled && ArrayWork && !ArrayWork->Ready()) return ArrayWork->StateText();
 return FString::Printf(TEXT("RECEIVER CONSOLE // ARRAY ON TARGET. %s"), *R.P.Card);
}

TArray<FS47Task> FS47Observing::Tasks() const
{
 TArray<FS47Task> Out;
 if (!bEnabled) return Out;
 for (int32 I = 0; I < Runs().Num(); ++I)
 {
  if (I == 1)
  {
   Out.Add({ TEXT("19:35 weekly generator test and transfer"), GenStep >= 3 });
   if (bWorkEnabled) Out.Add({ TEXT("Inspect S-03, service its local connector and verify remote pointing"), ArrayWork && ArrayWork->ServiceComplete() });
  }
  if (I == 2 && Archive >= 0) Out.Add({ TEXT("20:30 file the evening's charts in the records room"), Archive >= 2 });
  if (I == 3) Out.Add({ TEXT("21:40 round to the array gate"), Yard >= 2 });
  Out.Add({ Runs()[I].Task, Run > I });
  if (I == 1 && PulsarLoop != ELoopStage::Off)
  {
   Out.Add({ TEXT("Secure the pulsar chart before testing the receiver"), PulsarLoop >= ELoopStage::Rack });
   Out.Add({ TEXT("Run the terminated-input/internal-reference control at the receiver rack"), PulsarLoop >= ELoopStage::ControlPrint });
   Out.Add({ TEXT("Collect the control chart at the printer"), PulsarLoop >= ELoopStage::Compare });
   if (bWorkEnabled) Out.Add({ TEXT("Review original/control recordings and mark the extra-tick interval"), bAudioReviewed });
   Out.Add({ TEXT("Compare both charts on the operations terminal and register their limits"), PulsarLoop == ELoopStage::Filed });
   if (bWorkEnabled) Out.Add({ TEXT("Collect the labelled packet and place its originals in the April 13 binder"), bPacketCollected && Archive >= 1 });
  }
 }
 return Out;
}

bool FS47Observing::ChorePending() const
{
 return StepNow == EStep::Tune && ((Run == 1 && (GenStep < 3 || (bWorkEnabled && ArrayWork && ArrayWork->ServiceRequired()))) || (Run == 2 && Archive >= 0 && Archive < 2) || (Run == 3 && Yard < 2));
}

void FS47Observing::BuildPanel()
{
 if (bPanelBuilt || !M || !M->Web || !M->Inter || !M->Web->IsLoaded()) return;
 AS47Web* W = M->Web;
 const int32 Parent = W->Handle(TEXT("room.group"));
 const auto* ParentNode = W->Node(Parent);
 USceneComponent* ParentComp = ParentNode ? ParentNode->Comp.Get() : nullptr;
 auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 if (!ParentComp || !Cube || !Base) return;
 bPanelBuilt = true;
 auto* Mat = UMaterialInstanceDynamic::Create(Base, W);
 Mat->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(0x34473b)); Mat->SetScalarParameterValue(TEXT("Roughness"), .6f);
 Mat->SetScalarParameterValue(TEXT("NativeLighting"), 1); Mat->SetScalarParameterValue(TEXT("EmissivePower"), 0);
 auto* C = NewObject<UStaticMeshComponent>(W, TEXT("ObsGeneratorPanel"));
 C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(ParentComp); C->SetStaticMesh(Cube); C->SetMaterial(0, Mat);
 C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
 C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
 C->RegisterComponent(); C->SetWorldLocation(S47Three::ToUE(PanelPoint()));
 const FVector Scale(.05, .46, .34); C->SetWorldScale3D(S47Three::DirToUE(Scale));
 FS47Obj Obj = W->ObjAt(W->AdoptNode(C, Parent, TEXT("obs:generatorPanel"))); Obj.SetScale(Scale);
 auto* Text = NewObject<UTextRenderComponent>(W, TEXT("ObsGeneratorPanelText"));
 Text->SetMobility(EComponentMobility::Movable); Text->SetupAttachment(ParentComp);
 Text->SetText(FText::FromString(TEXT("STANDBY GEN\nREMOTE / ATS"))); Text->SetWorldSize(2.2f);
 Text->SetHorizontalAlignment(EHTA_Center); Text->SetVerticalAlignment(EVRTA_TextCenter);
 Text->SetTextRenderColor(FColor(214, 206, 176)); Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Text->RegisterComponent(); Text->SetWorldLocation(S47Three::ToUE(PanelPoint() + FVector(-.03, .08, 0)));
 Text->SetWorldRotation(FRotator(0, 90, 0)); W->AdoptNode(Text, Parent, TEXT("obs:generatorPanelText"));
 FS47Interactable It; It.Id = TEXT("obs:generator"); It.Object = Obj; It.Range = 2.4;
 It.Label = [this]() { return Label(TEXT("obs:generator")); };
 It.Use = [this] { UseGenerator(); };
 It.Examine = [] { return FString(TEXT("Remote start and automatic transfer switch for the standby diesel behind the control room. A clipboard on a nail: weekly test, start, transfer, log.")); };
 M->Inter->Add(It);
}

void FS47Observing::UseGenerator()
{
 if (Label(TEXT("obs:generator")).IsEmpty() || !Active() || Run != 1 || GenStep >= 3) { M->UI->Toast(TEXT("Standby generator remote panel. The weekly test is on the evening schedule."), 3); return; }
 AS47Story& S = *M->Story;
 if (GenStep == 0)
 {
  S.SetClock(FMath::Max(S.Clock(), GenStart));
  FS47Sound O; O.Gain = .5f; O.At = GeneratorAt; if (M->Audio) M->Audio->Sfx(TEXT("engineStart"), O);
  O.Gain = .3f; O.bLoop = true; O.When = 1.2f; if (M->Audio) M->Audio->LoopSfx(TEXT("obsGenerator"), TEXT("engineIdle"), O);
  GenStep = 1; GenT = 0;
  M->UI->Toast(FString::Printf(TEXT("%s. Remote start. Behind the wall the standby diesel turns over and settles."), *S.ClockText(false)), 4);
 }
 else if (GenStep == 1)
 {
  if (GenT < 4) { M->UI->Toast(TEXT("Wait for the diesel to settle."), 2); return; }
  HeldTubes.Reset();
  if (M->Web) { for (int32 K = 0; K < M->Web->TubeCount(); ++K) HeldTubes.Add(M->Web->TubeLit(K)); M->Web->SetTubes(false); if (M->Web->TubeCount()) M->Web->SetTube(0, true); }
  if (M->Audio) M->Audio->Play(TEXT("rack_switch"), .6f, M->Web ? M->Web->PropLocation(TEXT("rack")) : FVector::ZeroVector);
  GenStep = 2; GenT = 0;
  M->UI->Toast(TEXT("The transfer switch clunks. The room drops to the single emergency tube. Generator power: 120 V, 60 Hz."), 4);
 }
 else
 {
  if (GenT < 3) { M->UI->Toast(TEXT("Give the emergency lighting a moment on generator power."), 2); return; }
  if (M->Web) for (int32 K = 0; K < HeldTubes.Num() && K < M->Web->TubeCount(); ++K) M->Web->SetTube(K, HeldTubes[K]);
  if (M->Audio) { M->Audio->Play(TEXT("rack_switch"), .6f, M->Web ? M->Web->PropLocation(TEXT("rack")) : FVector::ZeroVector); M->Audio->Stop(TEXT("obsGenerator"), 1.5f); }
  GenStep = 3; FlickerT = 1.1;
  S.SetClock(FMath::Max(S.Clock(), GenDone));
  S.Note(TEXT("19:35. Weekly generator test: remote start, 120 V / 60 Hz, transfer to emergency lighting and back to line power. Normal."));
  M->UI->Toast(FString::Printf(TEXT("%s. Back on line power. Test logged. Next on the schedule: PSR B1133+16."), *S.ClockText(false)), 4);
 }
 Changed();
}

FS47Doc FS47Observing::RunBinder()
{
 FS47Doc D;
 D.Id = TEXT("obs_runbinder"); D.Title = TEXT("Run binder, April 1986"); D.Kind = TEXT("typed"); D.Stamp = TEXT("RUN BINDER");
 D.Page = TEXT("SARO RUN BINDER  /  APRIL 1986\n\n13 APR  19:00  3C 286 POINTING CHECK      FILED\n13 APR  19:50  PSR B1133+16 TIMING        FILED\n13 APR  20:45  HI STRIP 12               PENDING\n13 APR  22:05  VIRGO A PHASE CHECK       PENDING\n\n"
  "Older section, behind the divider:\n\n14 APR 1979  02:10  STATION 01 LAMP CHECK, A AND B NORMAL\n14 APR 1979  XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\nStamped in the margin: AMENDED / SERVICE COPY.");
 D.Transcript = TEXT("The run binder. Tonight's two charts go in behind the April tab. Further back, a page from April 1979 has one line typed over with X's. The stamp beside it says amended, service copy.");
 return D;
}

void FS47Observing::UseBinder()
{
 if (bWorkEnabled && !bPacketCollected) return;
 AS47Story& S = *M->Story;
 S.AdvanceClockTo(ArchiveStart, TEXT("evening records filing started"));
 FS47Doc Doc = RunBinder();
 if (bWorkEnabled)
 {
  Doc.Page += TEXT("\n\n") + WorkReference() + TEXT(" / APRIL 13 TAB\nORIGINAL + INTERNAL CONTROL + REGISTRATION COVER PHYSICALLY FILED.");
  Doc.Transcript = Doc.Page;
  S.Note(S.ClockText(false) + TEXT(". Placed the labelled packet ") + WorkReference() + TEXT(" behind April 13: original pulsar chart, internal-reference chart and registered conclusion. Digital index and physical originals now match."));
 }
 S.AddDoc(Doc);
 if (M->Audio) M->Audio->Play(TEXT("paper0"), .5f);
 Archive = 1;
 S.AdvanceClockTo(ArchiveDone, TEXT("evening records filing complete"));
 M->UI->Document(Doc);
 M->UI->Toast(FString::Printf(TEXT("%s. Charts filed. Next on the schedule: HI strip 12 at 20:45."), *S.ClockText(false)), 4);
 UE_LOG(LogTemp, Display, TEXT("S47: observing archive filed at clock %.0f"), S.Clock());
 Changed();
}

void FS47Observing::StepChores(double Dt)
{
 BuildPanel();
 if (GenStep == 1 || GenStep == 2) GenT += FMath::Min(Dt, .1);
 // After the transfer back, the emergency tube drops out for a moment on its own.
 if (FlickerT >= 0)
 {
  const double Was = FlickerT; FlickerT -= FMath::Min(Dt, .1);
  if (M->Web && M->Web->TubeCount() && HeldTubes.Num())
  {
   if (Was > .9 && FlickerT <= .9) { M->Web->SetTube(0, false); Hint(HintTube); }
   if (FlickerT < 0) M->Web->SetTube(0, HeldTubes[0]);
  }
 }
 if (Run == 2 && StepNow == EStep::Tune && Archive == 1 && M->Player && M->World && M->World->Area == TEXT("saro") &&
     FVector2D(M->Player->Pos.X - BinderPoint().X, M->Player->Pos.Z - BinderPoint().Z).Size() > 2.6)
 {
  Archive = 2; Hint(HintReel);
  M->Story->Note(TEXT("20:40. Filed the evening's charts in the run binder. On the way out the old reel-to-reel clicked and its spool moved. Switched off. Cold spool, probably settling."));
  M->UI->Objective(Objective());
 }
 if (Run != 3 || StepNow != EStep::Tune || Yard >= 2 || !M->Player || !M->World || M->World->Area != TEXT("saro") || !M->Web) return;
 const FVector P = M->Player->Pos;
 const double ToGate = FVector2D(P.X - GatePoint().X, P.Z - GatePoint().Z).Size();
 if (Yard == 0 && ToGate < 4.5 && !M->Web->IsDoorOpen(TEXT("fieldGate")))
 {
  Yard = 1; M->Story->SetClock(FMath::Max(M->Story->Clock(), YardStart));
  if (M->Audio && M->Audio->Night) M->Audio->Night->Hush(30, .02);
  M->UI->Toast(FString::Printf(TEXT("%s. The chain is through the gate and the cable trench covers are seated. Along the fence the insects stop."), *M->Story->ClockText(false)), 5);
  Changed();
 }
 else if (Yard == 1 && ToGate > 9)
 {
  M->Web->SetDoorById(TEXT("gate"), true, false);
  if (M->Audio) { FS47Sound O; O.Gain = .7f; O.At = GatePoint() + FVector(0, 1, 0); M->Audio->Sfx(TEXT("doorMetal"), O); }
  Yard = 2; Hint(HintGate);
  M->Story->Note(TEXT("21:45. Round to the array gate. Chained it. Walking back I heard chain-link behind me: the gate stood open again. No one on the field."));
  Changed();
 }
}

FS47Observing::FS47Observing(US47Main* InM) : M(InM)
{
 if (!M || !M->Audio) return;
 Voice = MakeShared<FS47RecordedDialogue>(M->Audio, TEXT("evening"), [](const FS47DialogueInput& I, FS47RadioVoice& Bank) -> TOptional<TArray<FS47RadioClip>>
 { return I.Lines.Num() == 1 ? Bank.Resolve(I.Lines[0].Text, 0, I.Voice) : TOptional<TArray<FS47RadioClip>>(); },
 [](const FS47RadioClip&) { return false; },
 [this](const FS47RadioClip&)
 {
  // Not the phone: the voice is on the far side of the visitor entrance.
  FS47RecordedDialogue::FOutput Out;
  if (bReplay) { Out.At = ReplayAt; Out.Channel = TEXT("physical"); return Out; } // SC04: the same words from another door
  FS47Obj Door;
  if (const auto* It = M && M->Inter ? M->Inter->Get(TEXT("exitDoor")) : nullptr) Door = It->Object;
  // Exported prologue doors use Web's fallback registry rather than Inter's added objects.
  if (!Door && M && M->Web) if (const auto* It = M->Web->Item(TEXT("exitDoor"))) Door = M->Web->ObjAt(It->Node);
  if (Door) Out.At = Door.WorldPosition() + FVector(-1.6, .2, 0);
  Out.Channel = Out.At.IsSet() ? TEXT("physical") : TEXT("unpositioned"); return Out;
 });
}
FS47Observing::~FS47Observing() { if (ArrayWork) ArrayWork->Stop(); if (Voice) Voice->Stop(); }

void FS47Observing::Stop()
{
 bEnabled = false; bAudioPlaying = bAudioPanelOpen = bTelemetryPanelOpen = false;
 if (ArrayWork) ArrayWork->Stop();
 if (Voice) Voice->Stop();
}

void FS47Observing::SetCharacterVoicesEnabled(bool bValue)
{
 if (Voice) Voice->SetEnabled(bValue);
}

bool FS47Observing::DoorVoicePlaying() const { return Voice && Voice->Playing(); }

void FS47Observing::ReplayDoorFragment(const FVector& At)
{
 if (!M || !M->Evening || !M->Audio || !Voice || !M->Audio->bCharacterVoices) return;
 const auto* Scene = M->Evening->Pack.FindScene(TEXT("D01_HANDOVER"));
 const FS47EveningLine* L = Scene ? Scene->Lines.FindByPredicate([](const FS47EveningLine& Line) { return Line.Id == DoorLine; }) : nullptr;
 if (!L) return;
 FS47DialogueInput In; In.Voice = L->VoiceProfile; In.Lines.Add({ L->Speaker, L->Text });
 ReplayAt = At; bReplay = true; Voice->SetEnabled(true); Voice->Play(In);
 UE_LOG(LogTemp, Display, TEXT("S47: SC04 door fragment %s again: %s"), DoorLine, *Voice->State());
}

void FS47Observing::StepDoorVoice()
{
 if (bReplay) { if (!Voice || (!Voice->Playing() && !Voice->Loading())) bReplay = false; return; }
 if (!M->World || M->World->Area != TEXT("saro") || (M->Horror && M->Horror->Complete())) { if (Voice && Voice->Playing()) Voice->Stop(); return; }
 if (DoorVoice >= 2 || !M->Player || !M->World || M->World->Area != TEXT("saro") || !M->Web || !M->Horror || !M->Horror->Enabled() || M->Horror->Complete()) return;
 const FVector P = M->Player->Pos;
 const bool bCorridor = P.Z > 5.0 && P.Z < 6.4 && P.X < -1.5 && P.X > -5.8;
 if (DoorVoice == 0 && bCorridor && !M->Web->IsDoorOpen(TEXT("exitDoor")))
 {
  DoorVoice = 1;
  const FS47EveningLine* L = nullptr;
  if (const auto* Scene = M->Evening ? M->Evening->Pack.FindScene(TEXT("D01_HANDOVER")) : nullptr)
   L = Scene->Lines.FindByPredicate([](const FS47EveningLine& Line) { return Line.Id == DoorLine; });
  if (L && Voice && M->Audio->bCharacterVoices) { FS47DialogueInput In; In.Voice = L->VoiceProfile; In.Lines.Add({ L->Speaker, L->Text }); Voice->SetEnabled(true); Voice->Play(In); UE_LOG(LogTemp, Display, TEXT("S47: observing door voice %s: %s"), DoorLine, *Voice->State()); }
  M->UI->Toast(TEXT("From outside the visitor entrance, Dale's voice: \"Reyes? Dale. The logbook should be in front of you.\""), 6);
  Hint(HintVoice);
 }
 else if (DoorVoice == 1 && M->Web->IsDoorOpen(TEXT("exitDoor")))
 {
  DoorVoice = 2;
  if (Voice) Voice->Stop();
  M->UI->Toast(TEXT("The steps outside are empty. The ramp light, the road, nobody. Dale went home at six."), 5);
  M->Story->Note(FString::Printf(TEXT("%s. Heard Dale's voice outside the visitor entrance, the first thing he said on the phone at six. Opened it: nobody on the steps."), *M->Story->ClockText(false)));
  Changed();
 }
}

void FS47Observing::Changed() { if (M) M->RequestAutosave(); }

bool FS47Observing::SelfTest(TArray<FString>& Lines)
{
 bool bOk = true;
 const auto Check = [&](bool Good, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s observing: %s"), Good ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= Good; };
 const auto& R = Runs();
 bool Ordered = R.Num() == 4, Reachable = true, Windows = true, Charts = true, ClearOfRelay = true;
 const FS47Profile& Relay = S47Signal::Profiles()[1];
 for (int32 I = 0; I < R.Num(); ++I)
 {
  if (I && R[I].Slot < R[I - 1].Slot + R[I - 1].Minutes * 60) Ordered = false;
  if (R[I].P.Az[0] < 0 || R[I].P.Az[1] > 180 || R[I].P.F < S47Signal::FMin || R[I].P.F > S47Signal::FMax) Reachable = false;
  FS47Rx Rx; Rx.Frequency = R[I].P.F; Rx.Gain = (R[I].P.Gain[0] + R[I].P.Gain[1]) / 2; Rx.Bandwidth = (R[I].P.Bw[0] + R[I].P.Bw[1]) / 2; Rx.Azimuth = (R[I].P.Az[0] + R[I].P.Az[1]) / 2;
  if (!Rx.Pass(R[I].P)) Windows = false;
  if (Rx.Pass(Relay)) ClearOfRelay = false;
  Rx.Azimuth = R[I].P.Az[1] + 6; if (Rx.Pass(R[I].P)) Windows = false;
  const FS47Doc D = Chart(I); if (D.Id.IsEmpty() || D.Page.IsEmpty() || D.Transcript.IsEmpty()) Charts = false;
 }
 Check(Ordered, TEXT("four runs in clock order without overlap"));
 Check(Reachable, TEXT("every run is inside the console's azimuth and frequency range"));
 Check(Windows, TEXT("each card's centre passes and an off-source azimuth fails"));
 Check(ClearOfRelay, TEXT("no run's setting is mistaken for the 1420.1 relay notch"));
 Check(R.Last().Slot + R.Last().Minutes * 60 <= -3600 - 10 * 60, TEXT("the last run is filed by 22:50, before the 23:00 window round"));
 Check(Charts && !Schedule().Page.IsEmpty(), TEXT("schedule and four papers exist"));
 Check(R[1].Seconds > StrayTicks[UE_ARRAY_COUNT(StrayTicks) - 1] && UE_ARRAY_COUNT(StrayTicks) == 11, TEXT("the stray ticks (four, a gap, seven) fall inside the pulsar run"));
 const auto LegacySchedule = [](int32 SavedRun)
 {
  auto O = S47J::New(); S47J::Set(O, TEXT("v"), Version); S47J::Set(O, TEXT("run"), SavedRun);
  S47J::Set(O, TEXT("step"), int32(EStep::Tune)); S47J::Set(O, TEXT("generator"), 3); S47J::Set(O, TEXT("yard"), 2);
  S47J::Set(O, TEXT("hints"), 17); return O; // deliberately no archive field or reel hint
 };
 const auto HasArchiveTask = [](const FS47Observing& O)
 { return O.Tasks().ContainsByPredicate([](const FS47Task& T) { return T.Text.Contains(TEXT("20:30")); }); };
 {
  FS47Observing Old(nullptr); Old.Begin(LegacySchedule(2), false);
  const auto Saved = S47J::Read(S47J::Write(Old.Save())); FS47Observing Again(nullptr); Again.Begin(Saved, false);
  Check(Old.Enabled() && Old.ArchiveStep() == -1 && Old.RunIndex() == 2 && !Old.ChorePending() && !HasArchiveTask(Old) && Old.Hints() == 17 &&
   S47J::Int(Saved, TEXT("archive")) == -1 && Again.ArchiveStep() == -1 && Again.RunIndex() == 2 && !Again.ChorePending() && !HasArchiveTask(Again) && Again.Hints() == 17,
   TEXT("legacy v1 run-two tuning remains unblocked and its explicit archive exemption survives JSON restore without a reel hint"));
 }
 {
  bool Preserved = true;
  for(int32 SavedRun : {3, 4})
  {
   FS47Observing Old(nullptr); Old.Begin(LegacySchedule(SavedRun), false);
   const auto Saved = S47J::Read(S47J::Write(Old.Save())); FS47Observing Again(nullptr); Again.Begin(Saved, false);
   Preserved &= Old.Enabled() && Old.ArchiveStep() == -1 && !Old.ChorePending() && !HasArchiveTask(Old) && Old.Hints() == 17 && Old.Done() == (SavedRun == 4) &&
    S47J::Int(Saved, TEXT("archive")) == -1 && Again.ArchiveStep() == -1 && Again.RunIndex() == SavedRun && !Again.ChorePending() && !HasArchiveTask(Again) && Again.Hints() == 17 && Again.Done() == (SavedRun == 4);
  }
  Check(Preserved, TEXT("legacy v1 Virgo and completed schedules gain no unreachable archive task or invented filing/reel facts after JSON restore"));
 }
 {
  auto Old = LegacySchedule(1); S47J::Set(Old, TEXT("step"), int32(EStep::Data));
  FS47Observing Legacy(nullptr); Legacy.Begin(Old, false);
  const auto Saved = S47J::Read(S47J::Write(Legacy.Save()));
  Check(Legacy.LoopStage() == ELoopStage::Off && !Saved->HasField(TEXT("loopVersion")),
   TEXT("an existing pulsar chart gains no retroactive receiver-control assignment"));
 }
 const auto LoopSave = [](ELoopStage Stage, double T)
 {
  auto O = S47J::New(); S47J::Set(O, TEXT("v"), Version); S47J::Set(O, TEXT("run"), Stage == ELoopStage::Filed ? 2 : 1);
  S47J::Set(O, TEXT("step"), int32(Stage == ELoopStage::Filed ? EStep::Tune : EStep::Data));
  S47J::Set(O, TEXT("archive"), 0); S47J::Set(O, TEXT("generator"), 3);
  S47J::Set(O, TEXT("loopVersion"), 1); S47J::Set(O, TEXT("loopStage"), int32(Stage));
  S47J::Set(O, TEXT("controlElapsed"), T); return O;
 };
 {
  bool Preserved = true;
  for (ELoopStage Stage : { ELoopStage::AwaitPulsar, ELoopStage::Rack, ELoopStage::ControlRunning, ELoopStage::ControlPrint, ELoopStage::Compare, ELoopStage::Filed })
  {
   FS47Observing O(nullptr); O.Begin(LoopSave(Stage, 3.25), false);
   FS47Observing Again(nullptr); Again.Begin(S47J::Read(S47J::Write(O.Save())), false);
   Preserved &= O.LoopStage() == Stage && Again.LoopStage() == Stage && Again.ControlElapsed() == 3.25 && Again.ArchiveStep() == 0;
  }
  Check(Preserved, TEXT("source, timed control, collected chart, comparison and filed stages retain their exact state through JSON restore"));
 }
 {
  FS47Observing O(nullptr); O.Begin(LoopSave(ELoopStage::Compare, ControlSeconds), false);
  const bool WrongPulsar = O.ComparePulsar(TEXT("pulsar"));
  FS47Observing Again(nullptr); Again.Begin(S47J::Read(S47J::Write(O.Save())), false);
  Check(!WrongPulsar && Again.LoopStage() == ELoopStage::Compare && Again.RunIndex() == 1 && Again.ControlElapsed() == ControlSeconds && !Again.LoopFeedback().IsEmpty(),
   TEXT("a wrong pulsar conclusion retains secured work and its corrective feedback through JSON restore"));
  const bool WrongSky = Again.ComparePulsar(TEXT("sky"));
  Check(!WrongSky && Again.LoopStage() == ELoopStage::Compare && Again.LoopFeedback().Contains(TEXT("does not prove a sky source")),
   TEXT("a quiet short control cannot establish the earlier ticks' source"));
  Check(Again.ComparePulsar(TEXT("unresolved")) && Again.LoopStage() == ELoopStage::Filed && Again.LoopFeedback().IsEmpty(),
   TEXT("only the explicit limited conclusion files the comparison"));
 }
 {
  auto Passed = LoopSave(ELoopStage::Rack, 0); S47J::Set(Passed, TEXT("run"), 2); S47J::Set(Passed, TEXT("step"), int32(EStep::Tune));
  FS47Observing O(nullptr); O.Begin(Passed, false);
  Check(O.LoopStage() == ELoopStage::Off && O.RunIndex() == 2 && !O.Save()->HasField(TEXT("loopVersion")),
   TEXT("a passed schedule never receives an unreachable unfinished control"));
  auto Unsupported = LoopSave(ELoopStage::Compare, ControlSeconds); S47J::Set(Unsupported, TEXT("loopVersion"), 2);
  O.Begin(Unsupported, false);
  Check(O.LoopStage() == ELoopStage::Off && !O.ComparePulsar(TEXT("unresolved")),
   TEXT("unsupported receiver-control state grants no invented comparison"));
 }
 Check(PulsarControl().Id != Chart(1).Id && PulsarComparison().Id != PulsarControl().Id && !PulsarComparison().Transcript.IsEmpty(),
  TEXT("the source chart, actual control and limited comparison remain distinct documents"));
 {
  FS47Observing Old(nullptr); Old.Begin(LoopSave(ELoopStage::Compare, ControlSeconds), false);
  Check(!Old.WorkEnabled() && !Old.Save()->HasField(TEXT("workVersion")),
   TEXT("already-started SARO loop cases gain no retroactive pointing, audio or packet requirement"));
  auto Future = LoopSave(ELoopStage::Compare, ControlSeconds); S47J::Set(Future, TEXT("workVersion"), 2);
  Old.Begin(Future, false);
  Check(!Old.WorkEnabled(), TEXT("unsupported workstation saves do not create new duties"));
 }
 {
  auto W = LoopSave(ELoopStage::Compare, ControlSeconds);
  S47J::Set(W, TEXT("workVersion"), 1); S47J::Set(W, TEXT("audioSource"), 1);
  S47J::Set(W, TEXT("audioOriginal"), 12.7); S47J::Set(W, TEXT("audioControl"), 4.5);
  S47J::Set(W, TEXT("coveredOriginal"), 12.7); S47J::Set(W, TEXT("coveredControl"), 4.5);
  S47J::Set(W, TEXT("markStart"), 9.0); S47J::Set(W, TEXT("markEnd"), 13.0);
  FS47Observing First(nullptr); First.Begin(W, false);
  FS47Observing Restored(nullptr); Restored.Begin(S47J::Read(S47J::Write(First.Save())), false);
  Check(Restored.WorkEnabled() && Restored.AudioSource() == 1 && Restored.AudioPosition() == 4.5 && !Restored.AudioPlaying()
   && Restored.AudioMarkedStart() == 9 && Restored.AudioMarkedEnd() == 13 && Restored.AudioCoverage[0] == ((1u << 12) - 1) && !Restored.AudioReviewed(),
   TEXT("an interrupted recording review retains each cursor, listened coverage and markers, and resumes paused"));
  Check(!Restored.ComparePulsar(TEXT("unresolved")) && Restored.LoopStage() == ELoopStage::Compare,
   TEXT("an unreviewed recording cannot acquire registration credit from the correct written answer"));
  Restored.AudioCursor[0] = 18; Restored.AudioCursor[1] = 8;
  Check(!Restored.AudioCoverageComplete(), TEXT("moving transport cursors to the end does not manufacture playback coverage"));
  Restored.CreditAudioRange(0, 17.99, 18); Restored.CreditAudioRange(1, 7.99, 8);
  Check(!Restored.AudioCoverageComplete() && Restored.AudioCovered[0] == 12.7 && Restored.AudioCovered[1] == 4.5,
   TEXT("seeking past an unheard gap and playing a boundary cannot manufacture recording coverage"));
  Restored.CreditAudioRange(0, 12.7, 18); Restored.CreditAudioRange(1, 4.5, 8);
  Check(Restored.AudioCoverageComplete(), TEXT("continuous playback completes independent original and control coverage"));
 }
 Check(Chart(1).Page.Contains(WorkReference()) && PulsarControl().Page.Contains(WorkReference()) && PulsarComparison().Page.Contains(WorkReference())
  && WorkOrder().Page.Contains(WorkReference()), TEXT("work order, original, control and registry use the same stable observation reference"));
 {
  auto Complete = LoopSave(ELoopStage::Filed, ControlSeconds); S47J::Set(Complete, TEXT("run"), 4); S47J::Set(Complete, TEXT("workVersion"), 1);
  auto Array = S47J::New(); S47J::Set(Array, TEXT("v"), 1); S47J::Set(Array, TEXT("service"), int32(FS47SaroWork::EService::Complete));
  Complete->SetObjectField(TEXT("workArray"), Array);
  FS47Observing DoneWork(nullptr); DoneWork.Begin(Complete, false);
  Check(DoneWork.Done() && DoneWork.WorkEnabled() && DoneWork.Work()->ServiceComplete() && !DoneWork.Work()->Enabled(),
   TEXT("a completed evening retains service history without reclaiming array motion from later story"));
 }
 bOk &= FS47SaroWork::SelfTest(Lines);
 return bOk;
}
