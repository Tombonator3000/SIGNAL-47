#include "S47Observing.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Horror.h"
#include "S47Docs.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Ambience.h"
#include "S47Web.h"

namespace
{
 FS47Profile Card(const TCHAR* Stage, const TCHAR* Text, double F, double FTol, double G0, double G1, double B0, double B1, double A0, double A1, const TCHAR* Note)
 {
  FS47Profile P; P.Stage = Stage; P.Action = TEXT("START RUN"); P.Card = Text; P.bCardIsTaped = false;
  P.F = F; P.FTol = FTol; P.Gain[0] = G0; P.Gain[1] = G1; P.Bw[0] = B0; P.Bw[1] = B1; P.Az[0] = A0; P.Az[1] = A1; P.Note = Note;
  return P;
 }
 // Hint bits, in the order the evening reaches them.
 constexpr int32 HintTicks = 1, HintBroadband = 2, HintStill = 4, HintDrive = 8;
 // PSR B1133+16 folds at 1.18791 s. The stray ticks fall four, a gap, then seven.
 constexpr double PulsarPeriod = 1.18791;
 const double StrayTicks[] = { 9.30, 9.55, 9.80, 10.05, 11.10, 11.35, 11.60, 11.85, 12.10, 12.35, 12.60 };
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
  { TEXT("vira"), TEXT("Virgo A"), TEXT("22:05 flux check on Virgo A"), TEXT("printer"), -1 * 3600. - 55 * 60, 45, 14, 64,
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
  TEXT("SCAN ON THE OPS TERMINAL.\n")
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
  D.Id = TEXT("obs_chart_vira"); D.Title = TEXT("Strip chart: Virgo A flux check");
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
 bEnabled = bScheduleFiled = false; Run = HintMask = Ticks = 0; StepNow = EStep::Tune; Elapsed = Tick = 0; HeldStatus.Empty();
 if (Saved)
 {
  if (S47J::Int(Saved, TEXT("v")) != Version) return;
  bEnabled = true; bScheduleFiled = S47J::Bool(Saved, TEXT("schedule"));
  Run = FMath::Clamp(S47J::Int(Saved, TEXT("run")), 0, Runs().Num());
  StepNow = EStep(FMath::Clamp(S47J::Int(Saved, TEXT("step")), 0, 2));
  if (Run >= Runs().Num()) StepNow = EStep::Tune;
  Elapsed = FMath::Clamp(S47J::Num(Saved, TEXT("elapsed")), 0., 600.);
  HintMask = S47J::Int(Saved, TEXT("hints")); HeldStatus = S47J::Str(Saved, TEXT("status"));
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
 if (StepNow == EStep::Data) { Rx.Status = Runs()[Run].Data == FString(TEXT("printer")) ? TEXT("RUN COMPLETE  /  CHART AT THE PRINTER") : TEXT("RUN COMPLETE  /  REDUCE ON THE OPS TERMINAL"); return true; }
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
 S.SetClock(FMath::Max(S.Clock(), R.Slot));
 S.AimArray((R.P.Az[0] + R.P.Az[1]) / 2, R.El, 9);
 if (M->Audio) M->Audio->Servo(2.4f, TOptional<FVector>());
 S.Rx.Status = FString::Printf(TEXT("INTEGRATING %s"), R.Source).ToUpper();
 StepNow = EStep::Integrating; Elapsed = 0; Tick = 0; Ticks = 0;
 M->UI->Toast(FString::Printf(TEXT("%s. The array swings onto %s. The chart pen starts its line."), *S.ClockText(false), R.Source), 4);
 UE_LOG(LogTemp, Display, TEXT("S47: observing run %d %s started at clock %.0f"), Run + 1, R.Id, S.Clock());
 Changed();
}

void FS47Observing::Step(double Dt)
{
 if (!bEnabled || !M || !M->Story || M->Mode != US47Main::EMode::Play || M->bPausedByMenu || !FMath::IsFinite(Dt) || Dt <= 0) return;
 if (!Active()) return;
 if (!bScheduleFiled)
 {
  bScheduleFiled = true; M->Story->AddDoc(Schedule());
  M->UI->Toast(TEXT("Dale's observing schedule is clipped under the console card: four runs before the 23:00 window round."), 5);
  Changed();
 }
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
  while (Tick >= PulsarPeriod) { Tick -= PulsarPeriod; M->Audio->Beep(820, .012f, .16f, 0, TEXT("square")); ++Ticks; }
  for (double At : StrayTicks) if (Crossed(At)) M->Audio->Beep(1240, .008f, .07f, 0, TEXT("square"));
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
  M->UI->Toast(TEXT("The chart pen jumps across every channel and stays up. Like something bright passing through the beam. No engine sound outside."), 5);
 }
 if (Bit == HintStill)
 {
  if (M->Audio && M->Audio->Night) M->Audio->Night->Still();
  M->UI->Toast(TEXT("Outside, the crickets stop. All of them at once."), 4);
 }
 if (Bit == HintDrive)
 {
  if (M->Audio) M->Audio->Servo(3.f, TOptional<FVector>());
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
 const bool Printer = R.Data == FString(TEXT("printer"));
 M->Story->Rx.Status = Printer ? TEXT("RUN COMPLETE  /  CHART AT THE PRINTER") : TEXT("RUN COMPLETE  /  REDUCE ON THE OPS TERMINAL");
 if (M->Audio && Printer) M->Audio->Play(TEXT("printer"), .35f, M->Web ? M->Web->PropLocation(TEXT("printer")) : FVector::ZeroVector, 1.2f);
 M->UI->Toast(Printer ? FString::Printf(TEXT("%s done. The printer chatters out the strip chart."), R.Source) : FString(TEXT("Drift scan done. Reduce it on the operations terminal.")), 4);
 UE_LOG(LogTemp, Display, TEXT("S47: observing run %d %s integrated"), Run + 1, R.Id);
 Changed();
}

FString FS47Observing::Label(const FString& Id) const
{
 if (!Active() || StepNow != EStep::Data || Id != Runs()[Run].Data) return FString();
 return Id == TEXT("printer") ? FString::Printf(TEXT("Tear off the %s strip chart"), Runs()[Run].Source) : FString(TEXT("Reduce the hydrogen drift scan"));
}

bool FS47Observing::Use(const FString& Id)
{
 if (Label(Id).IsEmpty()) return false;
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
 if (M->Audio) M->Audio->Play(Run == 2 ? TEXT("click") : TEXT("paper0"), .5f);
 // The run's own length passes on the clock; the next source waits for its slot.
 S.SetClock(FMath::Max(S.Clock(), R.Slot + R.Minutes * 60));
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
 if (StepNow == EStep::Integrating) return FString::Printf(TEXT("CONTROL ROOM // %s INTEGRATING. LISTEN TO THE MONITOR AND WATCH THE CHART"), R.Source).ToUpper();
 if (StepNow == EStep::Data) return R.Data == FString(TEXT("printer")) ? FString::Printf(TEXT("PRINTER // TEAR OFF THE %s STRIP CHART"), R.Source).ToUpper() : FString(TEXT("OPERATIONS TERMINAL // REDUCE THE HYDROGEN DRIFT SCAN"));
 return FString::Printf(TEXT("CONTROL ROOM // %s"), *R.P.Card);
}

TArray<FS47Task> FS47Observing::Tasks() const
{
 TArray<FS47Task> Out;
 if (!bEnabled) return Out;
 for (int32 I = 0; I < Runs().Num(); ++I) Out.Add({ Runs()[I].Task, Run > I });
 return Out;
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
 return bOk;
}
