#include "S47Observing.h"
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
  D.Page = TEXT("SARO / RX CONTROL 03   1419.600 MHz  BW 96 KHZ\n")
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
  if (GenStep == 1 && M && M->Audio) { FS47Sound S; S.Gain = .3f; S.At = GeneratorAt; S.bLoop = true; M->Audio->LoopSfx(TEXT("obsGenerator"), TEXT("engineIdle"), S); }
  // The stillness belongs to an unfinished Virgo A run only.
  if (StepNow == EStep::Integrating && Run == 3 && (HintMask & HintStill) && M && M->Audio && M->Audio->Night) M->Audio->Night->Still();
  return;
 }
 bEnabled = bNewCase && M && M->Story && M->Story->bEveningTimeline;
}

S47J::FObj FS47Observing::Save() const
{
 if (!bEnabled) return nullptr;
 auto O = S47J::New(); S47J::Set(O, TEXT("v"), Version); S47J::Set(O, TEXT("run"), Run); S47J::Set(O, TEXT("step"), int32(StepNow));
 S47J::Set(O, TEXT("elapsed"), Elapsed); S47J::Set(O, TEXT("hints"), HintMask); S47J::Set(O, TEXT("schedule"), bScheduleFiled); S47J::Set(O, TEXT("status"), HeldStatus);
 S47J::Set(O, TEXT("generator"), GenStep); S47J::Set(O, TEXT("archive"), Archive); S47J::Set(O, TEXT("yard"), Yard); S47J::Set(O, TEXT("doorVoice"), DoorVoice);
 return O;
}

bool FS47Observing::Active() const
{
 return Pending() && M && M->Story && M->Horror && M->Horror->Enabled() && M->Horror->BaselineChecked() &&
  M->Story->Rx.Stage == 1 && M->Story->Phase() == ES47Phase::Shift;
}

const FS47Profile* FS47Observing::ConsoleProfile() const { return Active() ? &Runs()[Run].P : nullptr; }

bool FS47Observing::ConsoleAction()
{
 if (!Active()) return false;
 FS47Rx& Rx = M->Story->Rx;
 if (StepNow == EStep::Integrating) { Rx.Status = FString::Printf(TEXT("INTEGRATING %s  /  HOLD"), Runs()[Run].Source).ToUpper(); return true; }
 if (StepNow == EStep::Data) { Rx.Status = TEXT("RUN COMPLETE  /  SEE THE OBJECTIVE"); return true; }
 if (ChorePending())
 {
  Rx.Status = Run == 1 ? TEXT("GENERATOR TEST DUE  /  REMOTE PANEL BY THE EAST DOOR")
   : Run == 2 ? TEXT("ARCHIVE FILING DUE  /  RUN BINDER IN THE RECORDS ROOM") : TEXT("ROUND DUE  /  ARRAY GATE");
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
 S.AimArray((R.P.Az[0] + R.P.Az[1]) / 2, R.El, 9);
 if (M->Audio && !Recorded(M->Audio, TEXT("obsArraySlew"), .45f)) M->Audio->Servo(2.4f, TOptional<FVector>());
 S.Rx.Status = FString::Printf(TEXT("INTEGRATING %s"), R.Source).ToUpper();
 StepNow = EStep::Integrating; Elapsed = 0; Tick = 0; Ticks = 0;
 M->UI->Toast(FString::Printf(TEXT("%s. The array swings onto %s. The chart pen starts its line."), *S.ClockText(false), R.Source), 4);
 UE_LOG(LogTemp, Display, TEXT("S47: observing run %d %s started at clock %.0f"), Run + 1, R.Id, S.Clock());
 Changed();
}

void FS47Observing::Step(double Dt)
{
 if (!bEnabled || !M || !M->Story || M->Mode != US47Main::EMode::Play || M->bPausedByMenu || !FMath::IsFinite(Dt) || Dt <= 0) return;
 if (Done()) { StepDoorVoice(); return; }
 if (!Active()) return;
 if (!bScheduleFiled)
 {
  bScheduleFiled = true; M->Story->AddDoc(Schedule());
  M->UI->Toast(TEXT("Dale's observing schedule is clipped under the console card: four runs before the 23:00 window round."), 5);
  Changed();
 }
 StepChores(Dt);
 if (StepNow != EStep::Integrating) return;
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
 if (Elapsed >= R.Seconds) Finish();
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
 if (Id == TEXT("obs:generator")) { UseGenerator(); return true; }
 if (Label(Id).IsEmpty()) return false;
 if (Id == TEXT("binder")) { UseBinder(); return true; }
 File();
 return true;
}

void FS47Observing::File()
{
 const FRun& R = Runs()[Run];
 AS47Story& S = *M->Story;
 const FS47Doc Doc = Chart(Run);
 S.AddDoc(Doc);
 S.Note(R.P.Note);
 if (M->Audio) M->Audio->Play(R.Data == FString(TEXT("printer")) ? TEXT("paper0") : TEXT("click"), .5f);
 // The run's own length passes on the clock; the next source waits for its slot.
 S.AdvanceClockTo(R.Slot + R.Minutes * 60, TEXT("observation data filed"));
 ++Run; StepNow = EStep::Tune; Elapsed = 0;
 S.Rx.Status = Run < Runs().Num() ? FString::Printf(TEXT("NEXT  /  %s"), Runs()[Run].Source).ToUpper() : HeldStatus;
 M->UI->Document(Doc);
 M->UI->Toast(Run < Runs().Num() ? FString::Printf(TEXT("%s. Chart filed. Next on the schedule: %s."), *S.ClockText(false), Runs()[Run].Source)
  : FString::Printf(TEXT("%s. The evening schedule is done. The north-window lamp round is next."), *S.ClockText(false)), 5);
 UE_LOG(LogTemp, Display, TEXT("S47: observing run %d %s filed at clock %.0f"), Run, R.Id, S.Clock());
 Changed();
}

FString FS47Observing::Objective() const
{
 if (!Active()) return FString();
 const FRun& R = Runs()[Run];
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
 return FString::Printf(TEXT("CONTROL ROOM // %s"), *R.P.Card);
}

TArray<FS47Task> FS47Observing::Tasks() const
{
 TArray<FS47Task> Out;
 if (!bEnabled) return Out;
 for (int32 I = 0; I < Runs().Num(); ++I)
 {
  if (I == 1) Out.Add({ TEXT("19:35 weekly generator test and transfer"), GenStep >= 3 });
  if (I == 2 && Archive >= 0) Out.Add({ TEXT("20:30 file the evening's charts in the records room"), Archive >= 2 });
  if (I == 3) Out.Add({ TEXT("21:40 round to the array gate"), Yard >= 2 });
  Out.Add({ Runs()[I].Task, Run > I });
 }
 return Out;
}

bool FS47Observing::ChorePending() const
{
 return StepNow == EStep::Tune && ((Run == 1 && GenStep < 3) || (Run == 2 && Archive >= 0 && Archive < 2) || (Run == 3 && Yard < 2));
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
 AS47Story& S = *M->Story;
 S.AdvanceClockTo(ArchiveStart, TEXT("evening records filing started"));
 const FS47Doc Doc = RunBinder();
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
FS47Observing::~FS47Observing() { if (Voice) Voice->Stop(); }

void FS47Observing::Stop()
{
 bEnabled = false;
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
 return bOk;
}
