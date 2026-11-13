#include "S47Freeform.h"
#include "S47Audio.h"
#include "S47Flow.h"
#include "S47FreeformTerminal.h"
#include "S47Gamepad.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Observing.h"
#include "S47SkyAcquire.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47WebAudio.h"
#include "SS47SkyChart.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Widgets/Layout/SBox.h"

// RX-ACQUIRE-SKY-20261113 (-> M3, Tom 13 November: "ACQUIRE og TUNE er nesten identisk ... da virker det mer som et reelt søk blant
// stjernene"): the session's part of the receiver's ACQUIRE page. The rule is FS47SkyAcquire (S47SkyAcquire.h), the picture is
// SS47SkyChart; here is what joins them to the job: what the array is doing, when a job's source counts as acquired, the hand-over
// to TUNE, the sounds, and the way through for bots and scripts (AcquireSky).
//
// One job, one fix. A job's fix is held from the lock on (FJob::bSky, saved while the job is Pointed) and is asked for once: taking
// the array elsewhere and back, or coming back to the job, does not ask again. A job back at the start (never pointed, or its
// pointing thrown away) starts over. Recording needs the fix where the rule is live (SkyRuleLive: a real game; a developer run
// keeps the old three conditions unless a test asks with TestSkyRule), so every fixture that points, tunes and records still does.
namespace
{
	namespace SkyT = S47FreeformTerminal;
	const TCHAR* const S47SkySoundNames[] = { TEXT("sweep_tick"), TEXT("converge"), TEXT("line_lock"), TEXT("lock_ring"), TEXT("lock_confirm"), TEXT("lock_confirm_open"), TEXT("blurt"), TEXT("key_blip"), TEXT("hint") };
	const TCHAR* const S47SkySoundDir = TEXT("RxAcquire/Audio");
	FString S47SkySoundKey(const TCHAR* Name) { return FString(TEXT("rxa:")) + Name; }
}

bool FS47FreeformSession::SkyRuleLive() const { return bTestSky || LiveFlow(); }

bool FS47FreeformSession::SkyAcquired() const
{
	const FJob* SkyJob = CurrentJob();
	return SkyJob && (SkyJob->bSky || SkyJob->Stage > EJobStage::Pointed || !SkyRuleLive());
}

void FS47FreeformSession::SyncSkyFix()
{
	const FJob* SkyJob = CurrentJob();
	if (!SkyJob) { SkyFix.Reset(); SkyFixJob.Empty(); return; }
	if (!SkyFix.IsValid()) { SkyFix = MakeShared<FS47SkyAcquire>(); SkyFixJob.Empty(); }
	const bool bFixHeld = SkyJob->bSky || SkyJob->Stage > EJobStage::Pointed;
	const FVector2D CardPlace(TargetAz(), TargetEl());
	// One search a job. A new job, a job whose card has moved (the day's third card swapped), or a job back at the start with a
	// chart that still shows a fix: the search begins again.
	if (SkyFixJob != SkyJob->Id || !SkyFix->CardAt().Equals(FS47SkyAcquire::ClampToChart(CardPlace), 1e-6) || (SkyFix->Locked() && SkyJob->Stage == EJobStage::New))
	{
		SkyFixJob = SkyJob->Id;
		SkyFix->Begin(HashCombine(GetTypeHash(SkyJob->Id), GetTypeHash(JobSeed)), CardPlace, SkyJob->Source, SkyOdd());
	}
	// What the array is doing: each antenna on remote, where it points and where it stands (metres east and north of the station).
	TArray<FS47SkyAcquire::FDish> Antennas;
	const bool bCommanded = SkyJob->Stage >= EJobStage::Pointed && HasPower();
	if (M && M->Web)
	{
		for (int32 I = 0; I < M->Web->DishCount() && I < FS47SkyAcquire::DishCount; ++I)
		{
			FS47SkyAcquire::FDish D;
			D.bOn = M->Web->DishRemoteAvailable(I); D.Point = FVector2D(M->Web->DishAz(I), M->Web->DishEl(I)); D.Ground = FVector2D(M->Web->DishX(I), -M->Web->DishZ(I));
			Antennas.Add(D);
		}
	}
	SkyFix->SetOdd(SkyOdd());
	SkyFix->SetArray(Antennas, bCommanded && PointReady() && Settle >= 1, bCommanded);
	if (bFixHeld && !SkyFix->Locked()) SkyFix->ForceLocked();
}

void FS47FreeformSession::LoadSkySounds()
{
	if (bSkySoundsTried || !M || !IsValid(M->Audio) || !M->Audio->Ctx.IsValid()) return;
	bSkySoundsTried = true;
	// Synthesized by Tools/rxacquire/make_sounds.py into Content/RxAcquire/Audio, read from disk as the other raw recordings are
	// (staged with the build, DefaultGame.ini). A file that is not there leaves its sound to the terminal's own beeps (SkySound).
	const FString Folder = FPaths::ProjectContentDir() / S47SkySoundDir;
	int32 Loaded = 0;
	for (const TCHAR* Name : S47SkySoundNames)
	{
		const FString BufKey = S47SkySoundKey(Name);
		if (M->Audio->Buf.Contains(BufKey)) { ++Loaded; continue; }
		TArray<uint8> Bytes;
		TSharedPtr<FS47AudioBuffer> Decoded;
		if (FFileHelper::LoadFileToArray(Bytes, *((Folder / Name) + TEXT(".wav")), FILEREAD_Silent)) Decoded = S47Wa::DecodeWav(Bytes);
		if (Decoded.IsValid()) { M->Audio->Buf.Add(BufKey, Decoded); ++Loaded; }
	}
	UE_LOG(LogTemp, Display, TEXT("S47: receiver sky chart: %d of %d sounds from Content/%s (any other falls back to the terminal's beeps)"), Loaded, int32(UE_ARRAY_COUNT(S47SkySoundNames)), S47SkySoundDir);
}

void FS47FreeformSession::SkySound(uint32 Events, int32 Blip)
{
	if ((!Events && Blip < 0) || !M || !IsValid(M->Audio) || !SkyFix.IsValid()) return;
	LoadSkySounds();
	US47Audio* Speaker = M->Audio;
	// Each sound from its file when it is there, else the terminal's own beep in its place. All of it quiet: heard every job.
	auto Play = [Speaker](const TCHAR* Name, float Gain, float Rate, TFunctionRef<void()> Else)
	{
		const FString BufKey = S47SkySoundKey(Name);
		if (Speaker->Buf.Contains(BufKey)) { FS47Sound O; O.Gain = Gain; O.Rate = Rate; Speaker->Sfx(BufKey, O); }
		else Else();
	};
	using FSky = FS47SkyAcquire;
	const float Near = float(SkyFix->Closeness());
	if (Blip == 0) Play(TEXT("key_blip"), .5f, 1.f, [&] { Speaker->Beep(1480.f, .016f, .030f, 0, TEXT("square")); });
	if (Blip == 1 || (Events & FSky::EvBlurt)) Play(TEXT("blurt"), .55f, 1.f, [&] { Speaker->Beep(196.f, .16f, .045f, 0, TEXT("sawtooth")); Speaker->Beep(185.f, .16f, .045f, 0, TEXT("sawtooth")); });
	if (Events & FSky::EvSweep)
	{
		// the search tick, and under it, close in, the convergence tone that rises as the triangle shuts
		Play(TEXT("sweep_tick"), FMath::Lerp(.30f, .50f, Near), FMath::Lerp(.80f, 1.35f, Near * Near), [&] { Speaker->Beep(FMath::Lerp(420.f, 1350.f, Near), .016f, FMath::Lerp(.016f, .028f, Near), 0, TEXT("square")); });
		const float Close = FMath::Clamp((Near - .55f) / .45f, 0.f, 1.f);
		if (Close > 0) Play(TEXT("converge"), .16f + .34f * Close, FMath::Lerp(.75f, 1.6f, Close * Close), [&] { Speaker->Beep(FMath::Lerp(330.f, 990.f, Close * Close), .07f, .012f + .020f * Close, 0, TEXT("sine")); });
	}
	if (Events & FSky::EvLine)
	{
		const float Step = 1.f + .122f * FMath::Clamp(SkyFix->LastArm(), 0, FSky::ArmCount - 1);
		Play(TEXT("line_lock"), .55f, Step, [&] { Speaker->Beep(880.f * Step, .030f, .036f, 0, TEXT("square")); Speaker->Beep(1320.f * Step, .050f, .034f, .045f, TEXT("square")); });
	}
	if (Events & FSky::EvLineLost) Play(TEXT("key_blip"), .30f, .70f, [&] { Speaker->Beep(520.f, .030f, .020f, 0, TEXT("square")); });
	if (Events & FSky::EvHint) Play(TEXT("hint"), .50f, 1.f, [&] { Speaker->Beep(990.f, .05f, .030f, 0, TEXT("sine")); Speaker->Beep(990.f, .05f, .030f, .14f, TEXT("sine")); });
	if (Events & FSky::EvLockStart) Play(TEXT("key_blip"), .50f, 1.26f, [&] { Speaker->Beep(1760.f, .022f, .030f, 0, TEXT("square")); });
	if (Events & FSky::EvRing)
	{
		const float Step = FMath::Pow(1.122f, float(FMath::Clamp(SkyFix->RingsClosed() - 1, 0, FSky::ArmCount - 1)) * 2.f);
		Play(TEXT("lock_ring"), .50f, Step, [&] { Speaker->Beep(660.f * Step, .060f, .034f, 0, TEXT("sine")); });
	}
	if (Events & FSky::EvLocked)
	{
		// the unusual kind ends on its own cadence, one that does not come home; no word of it anywhere
		if (SkyFix->Odd()) Play(TEXT("lock_confirm_open"), .60f, 1.f, [&] { Speaker->Beep(1320.f, .10f, .040f, 0, TEXT("sine")); Speaker->Beep(1397.f, .22f, .036f, .12f, TEXT("sine")); });
		else Play(TEXT("lock_confirm"), .60f, 1.f, [&] { Speaker->Beep(1320.f, .10f, .040f, 0, TEXT("sine")); Speaker->Beep(1760.f, .22f, .040f, .12f, TEXT("sine")); });
	}
}

void FS47FreeformSession::SkyChartStep(double Dt)
{
	// Called by the chart once a painted frame (SS47SkyChart::OnPaint): only the model and its sounds. What a lock means for the job
	// (the record, the toast, the page) waits for the session's own step (StepSkyFix), outside the painting.
	SyncSkyFix();
	if (!SkyFix.IsValid()) return;
	SkySound(SkyFix->Step(Dt), -1);
}

void FS47FreeformSession::SkyHeld(FJob& HeldJob, bool bByHand)
{
	HeldJob.bSky = true;
	// What the chart showed is what the take will be: decided here if the tuned receiver has not said so yet (DecideTake).
	if (HeldJob.TakeDecided < 0 && HeldJob.CaseTest < 0 && Tutorial != 1) DecideTake(HeldJob);
	// By hand the page stays a moment on the held fix, then goes over to TUNE; a script goes at once.
	SkyHandAt = FPlatformTime::Seconds() + (bByHand ? 1.6 : 0.);
	Feedback = TEXT("RECEIVER (centre PC): Source acquired. Now tune the work card on the TUNE page.");
	Changed(Feedback);
}

void FS47FreeformSession::StepSkyFix()
{
	// A job back at the start has lost its fix with its pointing.
	for (FJob& Each : Work) if (Each.Stage == EJobStage::New && Each.bSky) Each.bSky = false;
	FJob* SkyJob = Job();
	if (SkyJob && SkyJob->Stage == EJobStage::Pointed && !SkyJob->bSky && SkyFix.IsValid() && SkyFixJob == SkyJob->Id && SkyFix->Locked()) SkyHeld(*SkyJob, true);
	if (SkyHandAt >= 0 && FPlatformTime::Seconds() >= SkyHandAt)
	{
		SkyHandAt = -1;
		// still on the chart, at the receiver: the page goes over to the frequency search
		if (SkyJob && SkyJob->bSky && SkyJob->Stage == EJobStage::Pointed && ActiveTerminal == 1 && ReceiverTab == 0 && TerminalOnGlass()) { ReceiverTab = 1; OpenReceiver(); }
	}
}

bool FS47FreeformSession::AcquireSky()
{
	FJob* SkyJob = Job();
	if (!SkyJob) return false;
	if (SkyJob->bSky || SkyJob->Stage > EJobStage::Pointed) return true;
	if (SkyJob->Stage != EJobStage::Pointed || !HasPower() || !PointReady() || Settle < 1) return false;
	SyncSkyFix();
	if (!SkyFix.IsValid() || !SkyFix->Solve()) return false;
	SkyHeld(*SkyJob, false);
	return true;
}

FString FS47FreeformSession::SkyPrompt() const
{
	const FJob* SkyJob = CurrentJob();
	if (!SkyJob || !SkyFix.IsValid()) return ReceiverNextAction().ToUpper();
	using EP = FS47SkyAcquire::EPhase;
	const EP Now = SkyFix->State();
	if (Now == EP::Locked) return SkyJob->Stage == EJobStage::Pointed ? FString(TEXT("FIX HELD. NEXT: THE TUNE PAGE, AND THE FREQUENCY ON THE WORK CARD.")) : ReceiverNextAction().ToUpper();
	if (Now == EP::Locking) return TEXT("ON THE CROSSING. HOLD: THE RECEIVER IS TAKING THE FIX.");
	// Not pointed, no power, too few antennas: the receiver's own line says where to go.
	if (Now == EP::NoBearings) return ReceiverNextAction().ToUpper();
	if (!SkyFix->Steady()) return TEXT("THE ARRAY IS STILL TURNING. THE BEARINGS WILL HOLD ONCE IT IS ON TARGET.");
	if (SkyFix->AssistLevel() > 0) return TEXT("ASSIST ON: THE RECEIVER IS CLOSING IN BY ITSELF. YOU CAN STILL STEER.");
	const TCHAR* How = S47Gamepad::UsingGamepad() ? TEXT("RIGHT STICK OR D-PAD") : TEXT("MOUSE OR ARROW KEYS");
	if (SkyFix->HintShown()) return FString::Printf(TEXT("HINT: FOLLOW THE CHEVRONS INTO THE AMBER TRIANGLE AND REST THERE. (%s)"), How);
	return FString::Printf(TEXT("STEER THE RETICLE INTO THE AMBER TRIANGLE, WHERE THE BEARINGS CROSS. (%s)"), How);
}

TSharedRef<SWidget> FS47FreeformSession::SkyPage(TSharedPtr<SWidget>& KeysTo)
{
	SyncSkyFix();
	const FJob* SkyJob = CurrentJob();
	const FString Name = SkyJob && FS47Observing::Runs().IsValidIndex(SkyJob->Source) ? FString(FS47Observing::Runs()[SkyJob->Source].Source).ToUpper() : FString(TEXT("SOURCE"));
	const TSharedRef<SS47SkyChart> Chart = SNew(SS47SkyChart).Model(SkyFix)
		.OnStep([this](double Dt) { SkyChartStep(Dt); })
		.OnKey([this](int32 Kind) { SkySound(0, Kind); })
		.Calm([this] { return M && M->Flow && M->Flow->bCalmFlash; });
	// The chart has the keys first while there is a search to steer (the arrow keys, the walk keys, a d-pad).
	if (SkyFix.IsValid() && !SkyFix->Locked()) KeysTo = Chart;
	// Without a pointer: five-degree steps always land near enough to be eased in (FS47SkyAcquire::CaptureRadius).
	auto Nudge = [this](double Az, double El) { return [this, Az, El] { SyncSkyFix(); if (SkyFix.IsValid()) SkyFix->Nudge(FVector2D(Az, El)); }; };
	const TAttribute<bool> CanSteer = TAttribute<bool>::CreateLambda([this] { return SkyFix.IsValid() && SkyFix->State() == FS47SkyAcquire::EPhase::Search; });
	TArray<S47Kit::FW> Rows;
	Rows.Add(SkyT::Panel(FString(), S47Kit::Col({
		SkyT::TitleRow(TEXT("SKY CHART / FIND THE SOURCE"), TAttribute<FString>::CreateLambda([this, Name] { return FString::Printf(TEXT("CARD: %s  |  AZ %.1f  EL %.1f  |  %d ANTENNAS"), *Name, TargetAz(), TargetEl(), AvailableDishCount()); })),
		SNew(SBox).HeightOverride(384)[Chart] })));
	Rows.Add(SkyT::Columns({
		{ 1.f, SkyT::Button(TEXT("AZ -5"), Nudge(-5, 0), false, CanSteer) }, { 1.f, SkyT::Button(TEXT("AZ +5"), Nudge(5, 0), false, CanSteer) },
		{ 1.f, SkyT::Button(TEXT("EL -5"), Nudge(0, -5), false, CanSteer) }, { 1.f, SkyT::Button(TEXT("EL +5"), Nudge(0, 5), false, CanSteer) },
		{ 1.6f, SkyT::Button(TEXT("TUNE >"), [this] { ReceiverTab = 1; OpenReceiver(); }, true, TAttribute<bool>::CreateLambda([this] { return SkyAcquired(); })) } }, 6));
	Rows.Add(SkyT::Banner(TAttribute<FString>::CreateLambda([this] { return SkyPrompt(); }),
		TAttribute<bool>::CreateLambda([this] { return SkyFix.IsValid() && SkyFix->State() == FS47SkyAcquire::EPhase::Search && !(M && M->Flow && M->Flow->bCalmFlash); })));
	return S47Kit::Col(Rows);
}

bool FS47FreeformSession::SkySelfTest(TArray<FString>& Lines)
{
	bool bOk = FS47SkyAcquire::SelfTest(Lines) & SS47SkyChart::SelfTest(Lines);
	auto Check = [&](bool bPass, const TCHAR* What) { bOk &= bPass; Lines.Add(FString(bPass ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("sky acquire: ") + What); };
	// The session's side, as far as it goes without a station round it (no array to settle: the model's own tests cover the lock).
	FS47FreeformSession S(nullptr); S.Begin(nullptr);
	const bool bJobs = S.Work.Num() >= 2 && S.CurrentJob() != nullptr;
	if (!bJobs) { Check(false, TEXT("a fresh session has jobs to test with")); S.Stop(); return false; }
	Check(!S.SkyRuleLive() && S.SkyAcquired() && !S.Work[0].bSky, TEXT("a developer run keeps the old rule: recording does not wait for the sky fix"));
	S.TestSkyRule();
	Check(S.SkyRuleLive() && !S.SkyAcquired() && !S.AcquireSky(), TEXT("where the rule is live a job that is not pointed has no fix, and the programmatic path gives none"));
	S.Work[0].Stage = EJobStage::Pointed;
	Check(!S.SkyAcquired() && !S.AcquireSky() && !S.Work[0].bSky, TEXT("a pointed job with no settled array: still no fix by the programmatic path"));
	S.Work[0].bSky = true;
	Check(S.SkyAcquired() && S.AcquireSky(), TEXT("a held fix is asked for once: acquired, and the programmatic path agrees"));
	S.Work[1].Stage = EJobStage::New; S.Work[1].bSky = true; // (cannot happen in play: a fix needs a pointed array; the save must not keep it)
	const auto Kept = S47J::Read(S47J::Write(S.Save()));
	FS47FreeformSession Again(nullptr); Again.Begin(Kept);
	Check(Again.Work.Num() == S.Work.Num() && Again.Work[0].Stage == EJobStage::Pointed && Again.Work[0].bSky && !Again.Work[1].bSky, TEXT("a pointed job's fix survives a Continue; a job that is not pointed has none to keep"));
	// a save from before the sky chart: no field, no fix, and none written back for it
	auto Old = S.Save();
	for (const auto& V : S47J::Arr(Old, TEXT("jobs"))) if (V && V->Type == EJson::Object) V->AsObject()->RemoveField(TEXT("sky"));
	FS47FreeformSession Before(nullptr); Before.Begin(S47J::Read(S47J::Write(Old)));
	bool bNone = Before.Work.Num() == S.Work.Num();
	for (const FJob& Each : Before.Work) bNone &= !Each.bSky;
	const auto Rewritten = Before.Save();
	for (const auto& V : S47J::Arr(Rewritten, TEXT("jobs"))) bNone &= V && V->Type == EJson::Object && !V->AsObject()->HasField(TEXT("sky"));
	Check(bNone, TEXT("a save from before the sky chart reads as it did: no fix held, none written"));
	// past recording the fix is no longer asked for; back at the start it is dropped
	S.Work[0].bSky = false; S.Work[0].Stage = EJobStage::Integrating;
	const bool bPast = S.SkyAcquired() && S.AcquireSky();
	S.Work[0].Stage = EJobStage::New; S.Work[0].bSky = true; S.StepSkyFix();
	Check(bPast && !S.Work[0].bSky && !S.Work[1].bSky && !S.SkyAcquired(), TEXT("a job past recording asks for no fix; a job back at the start has lost it"));
	Check(!S.SkyPrompt().IsEmpty() && !S.SkyOdd(), TEXT("the page has a line to say with no chart open, and an ordinary card is ordinary"));
	Before.Stop(); Again.Stop(); S.Stop();
	return bOk;
}
