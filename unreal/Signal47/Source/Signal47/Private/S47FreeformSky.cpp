#include "S47Freeform.h"
#include "S47Audio.h"
#include "S47FieldRadio.h"
#include "S47Flow.h"
#include "S47FreeformTerminal.h"
#include "S47FreeformWorld.h"
#include "S47Gamepad.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Observing.h"
#include "S47RxLook.h"
#include "S47RxSearch.h"
#include "S47Script.h"
#include "S47SkyAcquire.h"
#include "S47SkySphere.h"
#include "S47Story.h"
#include "S47World.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47WebAudio.h"
#include "SS47RxLook.h"
#include "SS47ScreenView.h"
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

namespace
{
	// RX-LOOK gjennomgang 14 Nov: the receiver chart's sidereal time: the card's own (S47SkySphere::CardLst) for the job in hand, else
	// the station's (from the game's clock). It never touches the real sky.
	double S47SkyChartSidereal(const FS47SkyAcquire* Chart, double Station)
	{
		const double Card = Chart && Chart->CardRun() >= 0 ? S47SkySphere::CardLst(Chart->CardRun()) : -1.;
		return Card >= 0 ? Card : Station;
	}
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
	// (The model object is kept for the session's life once made: an open chart draws the same one the session steps.)
	if (!SkyJob) { SkyFixJob.Empty(); return; }
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

void FS47FreeformSession::StepRxSearch(double Dt)
{
	// RX-SEARCH-SOUND-20261113 (S47RxSearch.h): heard where the receiver's monitor is heard (at the centre PC on the glass, at SARO,
	// powered, not held), on ACQUIRE while the bearings are steady and the fix is not yet held, on TUNE once the fix is held. Any other
	// page, a closed terminal, the pause: it fades out. Under speech, the phone and a recording it ducks.
	if (!M || !IsValid(M->Audio)) return;
	using EUse = S47RxSearch::EUse;
	FS47RxSearch::FIn In;
	const FJob* SearchJob = CurrentJob();
	const bool bHeard = SearchJob && HasPower() && M->Story && M->Story->Rx.bPowered && !Held() && TerminalOnGlass() && ActiveTerminal == 1
		&& M->World && M->World->Area == TEXT("saro");
	if (bHeard && ReceiverTab == 0 && SearchJob->Stage == EJobStage::Pointed && SkyFix.IsValid() && SkyFixJob == SearchJob->Id)
	{
		using EP = FS47SkyAcquire::EPhase;
		const EP Now = SkyFix->State();
		if ((Now == EP::Search && SkyFix->Steady()) || Now == EP::Locking || Now == EP::Locked)
		{
			// taking the fix (the rings closing) is the closest the search gets; the held fix fades it into the lock confirm
			In.Use = EUse::Acquire; In.Closeness = Now == EP::Search ? SkyFix->Closeness() : 1.; In.bLocked = Now == EP::Locked;
		}
	}
	else if (bHeard && ReceiverTab == 1 && (SearchJob->Stage == EJobStage::Pointed || SearchJob->Stage == EJobStage::Integrating) && SkyAcquired() && PointReady() && Settle >= 1)
	{
		const FS47Profile P = EffectiveProfile(*SearchJob);
		In.Use = EUse::Tune; In.Closeness = M->Story->Rx.Quality(P); In.bLocked = M->Story->Rx.Pass(P);
	}
	In.bDuck = bAudioPlaying || M->Audio->PhonePlaying() || (M->FieldRadio.IsValid() && M->FieldRadio->DaleSpeaking());
	In.bCalm = M->Flow && M->Flow->bCalmFlash;
	if (In.Use == EUse::Off && !RxSearch.IsValid()) return; // nothing heard yet: nothing loaded
	if (!RxSearch.IsValid()) RxSearch = MakeShared<FS47RxSearch>();
	RxSearch->Frame(M->Audio, In, Dt);
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
	// LOOP-CAREFUL-PAYS-20261113 (-> M3, M2): a fix the assist pulled in (it had begun before the lock) earns no clean bonus on this card;
	// the hint's chevrons cost nothing. The programmatic way through (AcquireSky: bots, tests, scripts) counts as a hand fix, so that the
	// economy and the card player pay as before, and a discrepancy it registers narrows its bearing as a hand fix would.
	const bool bAssisted = bByHand && SkyFix.IsValid() && SkyFix->AssistLevel() > 0;
	HeldJob.SkyBy = bAssisted ? 2 : 1;
	// What the chart showed is what the take will be: decided here if the tuned receiver has not said so yet (DecideTake).
	if (HeldJob.TakeDecided < 0 && HeldJob.CaseTest < 0 && Tutorial != 1) DecideTake(HeldJob);
	// By hand the page stays a moment on the held fix, then goes over to TUNE; a script goes at once.
	SkyHandAt = FPlatformTime::Seconds() + (bByHand ? 1.6 : 0.);
	Feedback = bAssisted ? TEXT("RECEIVER (centre PC): Source acquired by the assist. FIX BY ASSIST / NO CLEAN BONUS on this card. Now tune the work card on the TUNE page.")
		: TEXT("RECEIVER (centre PC): Source acquired. Now tune the work card on the TUNE page.");
	Changed(Feedback);
}

void FS47FreeformSession::StepSkyFix()
{
	// A job back at the start has lost its fix with its pointing.
	for (FJob& Each : Work) if (Each.Stage == EJobStage::New && (Each.bSky || Each.SkyBy != 0)) { Each.bSky = false; Each.SkyBy = 0; }
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
	// LOOP-CAREFUL-PAYS-20261113: an assisted fix says so, once, plainly, for as long as the chart shows it
	if (Now == EP::Locked && SkyJob->SkyBy == 2 && SkyJob->Stage == EJobStage::Pointed) return TEXT("FIX BY ASSIST / NO CLEAN BONUS ON THIS CARD. NEXT: THE TUNE PAGE.");
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

double FS47FreeformSession::SiderealDeg() const
{
	// RX-LOOK-20261113: the world's own clock and date (WorldClock counts from midnight of the first calendar day), Mountain time,
	// at the valley's longitude: the same sky the game's moon and sun are worked out for (S47Weather).
	return S47SkySphere::LstDeg(DateOfDay(1).GetJulianDay() + (WorldClock + S47SkySphere::UtcBehindHours * 3600.) / 86400.);
}

TSharedRef<FS47RxFeed> FS47FreeformSession::RxFeed() const
{
	return MakeShared<FS47RxFeed>([this] { return RxLive(); });
}

FS47RxLive FS47FreeformSession::RxLive() const
{
	// RX-LOOK-20261113: see S47Freeform.h. Read every frame by the receiver's frame; nothing is decided or changed here.
	FS47RxLive Look;
	// (RX-LOOK gjennomgang 14 Nov: the clock by the rule for a clock in player text, S47RxLook::ClockLine; the other terminals keep theirs)
	Look.Clock = S47RxLook::ClockLine(TerminalClock());
	// "SARO OPS V4.2  TERM 3  OPR NIGHT", less the name the header draws large
	Look.Term = TerminalStation(1);
	Look.Term.RemoveFromStart(TEXT("SARO OPS "));
	Look.bCalm = M && M->Flow && M->Flow->bCalmFlash;
	// RX-LOOK gjennomgang 14 Nov: the chart's sidereal time, held at the card's (S47SkySphere::CardLst), so that the card's label, the
	// readout and the stars agree whenever the player looks; with no card the station's own. The real sky outside is not this.
	Look.LstDeg = S47SkyChartSidereal(SkyFix.Get(), SiderealDeg());
	Look.Dishes = AvailableDishCount(); Look.Rate = ArrayMeasurementRate();
	const bool bWornArray = Field && Field->ArrayWear().bActive;
	// (the idle picture's own words for it, TerminalLines)
	Look.RateWord = Look.Dishes < (bWornArray ? FS47ArrayWear::MinDishes : 24) ? TEXT("BLOCKED") : Look.Dishes < 27 ? (bWornArray && Field->ArrayWear().DownCount() > 0 ? TEXT("WORN") : TEXT("DEGRADED")) : TEXT("FULL");
	const bool bPowerNow = HasPower();
	// where the array really points: the mean of the antennas on remote (the azimuth as a direction, so that 359 and 1 are north)
	if (M && M->Web && bPowerNow)
	{
		double SumSin = 0, SumCos = 0, SumEl = 0; int32 Answering = 0;
		for (int32 I = 0; I < M->Web->DishCount(); ++I)
		{
			if (!M->Web->DishRemoteAvailable(I)) continue;
			const double DishAzRad = FMath::DegreesToRadians(M->Web->DishAz(I));
			SumSin += FMath::Sin(DishAzRad); SumCos += FMath::Cos(DishAzRad); SumEl += M->Web->DishEl(I); ++Answering;
		}
		if (Answering > 0)
		{
			Look.bPointing = true; Look.El = SumEl / Answering;
			Look.Az = S47SkySphere::Wrap360(FMath::RadiansToDegrees(FMath::Atan2(SumSin, SumCos)));
		}
	}
	Look.Scope.bPowered = bPowerNow && M && M->Story && M->Story->Rx.bPowered;
	if (M && M->Story) { Look.Scope.Tuned = M->Story->Rx.Frequency; Look.Scope.Gain = M->Story->Rx.Gain; Look.Scope.Bandwidth = M->Story->Rx.Bandwidth; }
	S47RxLook::FStateIn In;
	In.bPower = bPowerNow;
	if (const FJob* LookJob = CurrentJob())
	{
		const FS47Profile LookCard = EffectiveProfile(*LookJob);
		Look.bJob = true; Look.CardMhz = LookCard.F;
		Look.Source = FS47Observing::Runs().IsValidIndex(LookJob->Source) ? FString(FS47Observing::Runs()[LookJob->Source].Source).ToUpper() : FString();
		Look.Scope.Card = LookCard.F; Look.Scope.FTol = LookCard.FTol; Look.Scope.Seed = GetTypeHash(LookJob->Id);
		const bool bTunedNow = M && M->Story && M->Story->Rx.Pass(LookCard);
		const bool bFixNow = SkyAcquired();
		const bool bChartLive = SkyFix.IsValid() && SkyFixJob == LookJob->Id;
		// What of the source reaches the tube: the carrier quality for the card, the array on target (as the receiver's monitor
		// lets it grow over the slew, ReceiverApproach), and the fix: before it the array is not phased up and the line is
		// faint, a little less so the nearer the reticle stands to the crossing.
		double Phased = bFixNow ? 1. : .15;
		if (!bFixNow && bChartLive && SkyFix->HasBearings()) Phased = .2 + .5 * SkyFix->Closeness();
		Look.Scope.Strength = (M && M->Story ? M->Story->Rx.Quality(LookCard) : 0.) * ReceiverApproach() * Phased;
		In.bJob = true; In.Stage = int32(LookJob->Stage); In.bNotUp = EveningGated(*LookJob);
		In.bBlocked = M && M->Web && LookJob->Stage <= EJobStage::Integrating && ArrayBlocked();
		In.bPointReady = PointReady(); In.bSettled = Settle >= 1; In.bSky = bFixNow; In.bTuned = bTunedNow;
		In.bSkyLocking = bChartLive && SkyFix->State() == FS47SkyAcquire::EPhase::Locking;
		// (the idle picture's own two conditions for ANOMALY DETECTED, TerminalLines: never earlier, never longer)
		In.bAnomalyAhead = LookJob->Stage == EJobStage::Pointed && ReceiverReady() && AnomalyAhead();
		In.bPlainOnTape = PlainAnomaly(*LookJob) && LookJob->Stage >= EJobStage::Integrating && LookJob->Stage <= EJobStage::Recorded && !LookJob->bAudioReviewed;
		In.bReference = LookJob->bControl && LookJob->ControlAt < 8 && LookJob->Phenomenon != TEXT("wrong_source");
		In.Elapsed = LookJob->Elapsed; In.Seconds = SourceSeconds(); In.ReferenceAt = LookJob->ControlAt;
		// the waterfall marks a tick only as the tape passes it, while the take is being written (as the tape strip shows it)
		Look.bRecording = LookJob->Stage == EJobStage::Integrating && bTunedNow && In.bPointReady && In.bSettled;
		Look.Elapsed = LookJob->Elapsed; Look.Seconds = SourceSeconds();
		if (LookJob->Stage == EJobStage::Integrating) Look.Ticks = LookJob->TraceTicks;
	}
	Look.State = S47RxLook::StateOf(In);
	// (D45-20261110: while the array comes round the receiver says how far it has to go, as its old line beside the tabs did)
	if (Look.State.Id == S47RxLook::EState::Slewing) Look.State.Text = S47RxLook::SlewWords(ArrayErrorDeg());
	return Look;
}

TSharedRef<SWidget> FS47FreeformSession::SkyPage(TSharedPtr<SWidget>& KeysTo, TSharedRef<FS47RxFeed> Feed, TSharedPtr<float> BodyHigh)
{
	SyncSkyFix();
	const FJob* SkyJob = CurrentJob();
	const FString Name = SkyJob && FS47Observing::Runs().IsValidIndex(SkyJob->Source) ? FString(FS47Observing::Runs()[SkyJob->Source].Source).ToUpper() : FString(TEXT("SOURCE"));
	const TSharedRef<SS47SkyChart> Chart = SNew(SS47SkyChart).Model(SkyFix)
		.OnStep([this](double Dt) { SkyChartStep(Dt); })
		.OnKey([this](int32 Kind) { SkySound(0, Kind); })
		.Calm([this] { return M && M->Flow && M->Flow->bCalmFlash; })
		.Sidereal([this] { return S47SkyChartSidereal(SkyFix.Get(), SiderealDeg()); })
		.Source(Name);
	// The chart has the keys first while there is a search to steer (the arrow keys, the walk keys, a d-pad).
	if (SkyFix.IsValid() && !SkyFix->Locked()) KeysTo = Chart;
	// Without a pointer: five-degree steps, the way the key points on the glass, always land near enough to be eased in
	// (FS47SkyAcquire::CaptureRadius). RX-LOOK gjennomgang 14 Nov: the step keys take no keyboard focus, so a click on one leaves the
	// arrow keys with the chart. (The tabs and TUNE > build the page again, and the screen gives the keys to the new page's target;
	// on TUNE there is nothing to steer, and the arrows walking between its dial keys is the screen's ordinary way round it.)
	auto Nudge = [this, Chart](double Across, double Up) { return [this, Chart, Across, Up] { SyncSkyFix(); if (SkyFix.IsValid()) SkyFix->Nudge(Chart->SkyStep(FVector2D(Across, Up)) * 5.); }; };
	const TAttribute<bool> CanSteer = TAttribute<bool>::CreateLambda([this] { return SkyFix.IsValid() && SkyFix->State() == FS47SkyAcquire::EPhase::Search; });
	// RX-LOOK-20261113 (Tom's reference): the sphere as the main panel, the waterfall, the spectrum and the tracking readouts in a
	// column to its right, the I and Q strips under them with the step keys and the way on. The main row takes what the frame's
	// body has left over the strip, so the page fills the glass and never asks to be scrolled.
	// (round 2: the strip is four units higher and the step keys a little wider, for letters of the next step's size)
	constexpr float StripHigh = 64.f, RightWide = 338.f;
	const TAttribute<FOptionalSize> MainHigh = TAttribute<FOptionalSize>::CreateLambda([BodyHigh] { return FOptionalSize(FMath::Max(240.f, (BodyHigh.IsValid() ? *BodyHigh : 520.f) - StripHigh - 8.f)); });
	const TSharedRef<SWidget> SkyMain = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
		[
			SkyT::RxPanel(TEXT("EQUATORIAL CELESTIAL SPHERE"), SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)[Chart],
				TAttribute<FString>::CreateLambda([this] { return FString::Printf(TEXT("CARD AZ %s  EL %s"), *S47RxLook::Num(TargetAz(), 1), *S47RxLook::Num(TargetEl(), 1)); }))
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).WidthOverride(RightWide)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(.27f).Padding(0, 0, 0, 6)
				[SkyT::RxPanel(TEXT("SIGNAL WATERFALL"), SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)[SNew(SS47RxFall).Feed(Feed)],
					TAttribute<FString>::CreateLambda([Feed] { return FString::Printf(TEXT("+/- %s kHz"), *S47RxLook::Num(1000. * S47RxLook::HalfSpan(Feed->Now().Scope.FTol))); }))]
				+ SVerticalBox::Slot().FillHeight(.30f).Padding(0, 0, 0, 6)
				[SkyT::RxPanel(TEXT("FFT SPECTRUM"), SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)[SNew(SS47RxFft).Feed(Feed)],
					TAttribute<FString>::CreateLambda([Feed] { return FString::Printf(TEXT("BW %s kHz"), *S47RxLook::Num(Feed->Now().Scope.Bandwidth)); }))]
				+ SVerticalBox::Slot().FillHeight(.43f)
				[SkyT::RxPanel(TEXT("ACQUISITION / TRACKING"), SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)[SNew(SS47RxTrack).Feed(Feed).Model(SkyFix).View([Weak = TWeakPtr<SS47SkyChart>(Chart)] { const TSharedPtr<SS47SkyChart> Pinned = Weak.Pin(); return Pinned.IsValid() ? Pinned->OpenView() : SS47SkyChart::PlotFor(FVector2f(716, 440)).View; })])]
			]
		];
	const TSharedRef<SWidget> SkyStrip = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
		[SkyT::RxPanel(TEXT("I (IN-PHASE)"), SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)[SNew(SS47RxIq).Feed(Feed)],
			TAttribute<FString>::CreateLambda([Feed] { return FString::Printf(TEXT("LOCK %s %%"), *S47RxLook::Num(100. * FMath::Clamp(Feed->Now().Scope.Strength, 0., 1.))); }))]
		+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
		[SkyT::RxPanel(TEXT("Q (QUADRATURE)"), SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)[SNew(SS47RxIq).Feed(Feed).bQuadrature(true)])]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).WidthOverride(270)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1).Padding(0, 0, 0, 4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1)
					+ SHorizontalBox::Slot().FillWidth(1).Padding(3, 0)[SkyT::RxKey(TEXT("UP 5\u00b0"), Nudge(0, 1), false, CanSteer, 22, false)]
					+ SHorizontalBox::Slot().FillWidth(1)
				]
				+ SVerticalBox::Slot().FillHeight(1)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1)[SkyT::RxKey(TEXT("LEFT 5\u00b0"), Nudge(-1, 0), false, CanSteer, 22, false)]
					+ SHorizontalBox::Slot().FillWidth(1).Padding(3, 0)[SkyT::RxKey(TEXT("DOWN 5\u00b0"), Nudge(0, -1), false, CanSteer, 22, false)]
					+ SHorizontalBox::Slot().FillWidth(1)[SkyT::RxKey(TEXT("RIGHT 5\u00b0"), Nudge(1, 0), false, CanSteer, 22, false)]
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0)
		[
			SNew(SBox).WidthOverride(96)[SkyT::RxKey(TEXT("TUNE >"), [this] { ReceiverTab = 1; OpenReceiver(); }, true, TAttribute<bool>::CreateLambda([this] { return SkyAcquired(); }), 24)]
		];
	return S47Kit::Col({ SNew(SBox).HeightOverride(MainHigh)[SkyMain], SNew(SBox).HeightOverride(StripHigh)[SkyStrip] }, 6.f);
}

bool FS47FreeformSession::SkySelfTest(TArray<FString>& Lines)
{
	bool bOk = FS47SkyAcquire::SelfTest(Lines) & SS47SkyChart::SelfTest(Lines);
	// RX-LOOK-20261113: the sphere's arithmetic and star table, the frame's words, effects and scopes, and the painted parts' own rules
	bOk &= S47SkySphere::SelfTest(Lines);
	bOk &= S47RxLook::SelfTest(Lines);
	bOk &= S47RxLookPaint::SelfTest(Lines);
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
	// Review, 13 November: this check used to set the job's flag itself and then ask for it, which could not fail. Now the fix is
	// made the way the chart makes it: the session's own model for this job (SyncSkyFix), an array standing on the card and
	// steady (the self-test has no station to ask, so the model is told so directly), the model's way through (Solve), and the
	// session's step (StepSkyFix), which is what turns the model's lock into the job's fix and says how it was made.
	const double Tick = 1. / 60.;
	auto Standing = [](FVector2D Toward)
	{
		TArray<FS47SkyAcquire::FDish> Dishes;
		for (int32 I = 0; I < FS47SkyAcquire::DishCount; ++I)
		{
			FS47SkyAcquire::FDish D; D.bOn = true; D.Point = Toward;
			const double Arm = FMath::DegreesToRadians(120. * FS47SkyAcquire::ArmOf(I)), Reach = 40. * (1 + I % (FS47SkyAcquire::DishCount / FS47SkyAcquire::ArmCount));
			D.Ground = FVector2D(FMath::Sin(Arm) * Reach, FMath::Cos(Arm) * Reach);
			Dishes.Add(D);
		}
		return Dishes;
	};
	const FVector2D OnCard(S.TargetAz(), S.TargetEl());
	S.SyncSkyFix();
	const bool bModel = S.Job() == &S.Work[0] && S.SkyFix.IsValid() && S.SkyFixJob == S.Work[0].Id && !S.SkyFix->HasBearings() && !S.SkyFix->Locked() && !S.SkyFix->Solve();
	S.StepSkyFix();
	const bool bNoneYet = !S.Work[0].bSky && S.Work[0].SkyBy == 0 && !S.SkyAcquired();
	if (S.SkyFix.IsValid()) S.SkyFix->SetArray(Standing(OnCard), true, true);
	const bool bSolved = S.SkyFix.IsValid() && S.SkyFix->Solve() && S.SkyFix->Locked();
	const bool bNotByItself = !S.Work[0].bSky && !S.SkyAcquired(); // the model's lock alone is not yet the job's fix
	S.StepSkyFix();
	Check(bModel && bNoneYet && bSolved && bNotByItself && S.Work[0].bSky && S.Work[0].SkyBy == 1 && S.SkyAcquired() && S.AcquireSky(),
		*FString::Printf(TEXT("a fix made on the chart: no bearings, no fix; the model locked and the session's step taken, the job's fix is held and counted as made by hand (how it was made: %d)"), S.Work[0].SkyBy));
	S.SkyHandAt = -1; // (by hand the page would go over to TUNE a moment later: there is no page here)
	S.Work[1].Stage = EJobStage::New; S.Work[1].bSky = true; // (cannot happen in play: a fix needs a pointed array; the save must not keep it)
	const auto Kept = S47J::Read(S47J::Write(S.Save()));
	FS47FreeformSession Again(nullptr); Again.Begin(Kept);
	Check(Again.Work.Num() == S.Work.Num() && Again.Work[0].Stage == EJobStage::Pointed && Again.Work[0].bSky && Again.Work[0].SkyBy == 1 && !Again.Work[1].bSky, TEXT("a pointed job's fix and how it was made survive a Continue; a job that is not pointed has none to keep"));
	{
		// the same job back at the start and pointed again, and this time nobody touches the chart: the fix is dropped with the
		// pointing, the search begins again, the assist brings the reticle in, and the fix is counted as assisted
		S.Work[0].Stage = EJobStage::New; S.StepSkyFix(); S.SyncSkyFix();
		const bool bDropped = !S.Work[0].bSky && S.Work[0].SkyBy == 0 && S.SkyFix.IsValid() && !S.SkyFix->Locked();
		S.Work[0].Stage = EJobStage::Pointed;
		double Waited = 0;
		for (; S.SkyFix.IsValid() && Waited < 60. && !S.SkyFix->Locked(); Waited += Tick) { S.SkyFix->SetArray(Standing(OnCard), true, true); S.SkyFix->Step(Tick); }
		const bool bLockedAlone = S.SkyFix.IsValid() && S.SkyFix->Locked() && !S.Work[0].bSky;
		S.StepSkyFix();
		Check(bDropped && bLockedAlone && Waited > FS47SkyAcquire::AssistAfter && S.Work[0].bSky && S.Work[0].SkyBy == 2,
			*FString::Printf(TEXT("back at the start the fix is dropped; pointed again and left alone, the assist takes the fix after %.1f s and it is counted as assisted (how it was made: %d)"), Waited, S.Work[0].SkyBy));
		S.SkyHandAt = -1; // (by hand the page would go over to TUNE a moment later: there is no page here)
	}
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
	{ // RX-LOOK-20261113: the frame's reading, as far as it goes without a station round it
		const FS47RxLive Bare = S.RxLive();
		// RX-LOOK gjennomgang 14 Nov: the header's clock as the frame draws it, at a minute that would read 47 and one that would not
		// (it used to be compared with the terminal's own clock, which it was a copy of)
		const double ClockWas = S.WorldClock;
		S.WorldClock = FMath::FloorToDouble(ClockWas / 86400.) * 86400. + 23. * 3600. + 47. * 60. + 12.;
		const FString At47 = S.RxLive().Clock, Terminal47 = S.TerminalClock();
		S.WorldClock = FMath::FloorToDouble(ClockWas / 86400.) * 86400. + 21. * 3600. + 14. * 60.;
		const FString At14 = S.RxLive().Clock, Terminal14 = S.TerminalClock();
		S.WorldClock = ClockWas;
		const bool bClock = Terminal47.EndsWith(TEXT("23:47")) && At47 == Terminal47.LeftChop(5) + TEXT("23:48") && !S47Script::SaysFortySeven(At47) && At14 == Terminal14 && At14.EndsWith(TEXT("21:14"))
			&& S47RxLook::ClockLine(TEXT("28-APR-86 08:15")) == TEXT("28-APR-86 08:16") && S47RxLook::ClockLine(TEXT("NO CLOCK")) == TEXT("NO CLOCK");
		// RX-LOOK gjennomgang 14 Nov: the chart's sidereal time is the card's and does not follow the clock
		S.SyncSkyFix();
		const double ChartAt = S.RxLive().LstDeg;
		S.WorldClock = ClockWas + 3. * 3600.;
		const double ChartLater = S.RxLive().LstDeg, StationLater = S.SiderealDeg();
		S.WorldClock = ClockWas;
		const bool bHeld = S.SkyFix.IsValid() && S.SkyFix->CardRun() >= 0 && ChartAt == S47SkySphere::CardLst(S.SkyFix->CardRun()) && ChartLater == ChartAt && FMath::Abs(S47SkySphere::Wrap180(StationLater - S.SiderealDeg())) > 40.;
		Check(bHeld, *FString::Printf(TEXT("the receiver's chart stands at the card's sidereal time (%.2f h) whatever the clock; the station's own moves on (%.2f h three hours later)"), ChartAt / 15., StationLater / 15.));
		Check(bClock, *FString::Printf(TEXT("the receiver's header clock never reads 47: the terminal's %s is drawn %s, its %s as it is"), *Terminal47, *At47, *Terminal14));
		const bool bSky = FMath::IsFinite(S.SiderealDeg()) && S.SiderealDeg() >= 0 && S.SiderealDeg() < 360;
		// the first morning at 08:00, the session's own first date (28 April 1986: a little after 22 h 18 min of sidereal time)
		FS47FreeformSession Fresh(nullptr); Fresh.Begin(nullptr);
		const FDateTime FirstDate = Fresh.DateOfDay(1);
		const double Morning = Fresh.SiderealDeg() / 15., Almanac = S47SkySphere::LstOf(FirstDate.GetYear(), FirstDate.GetMonth(), FirstDate.GetDay(), Fresh.Clock()) / 15.;
		Fresh.Stop();
		Check(Bare.bJob && Bare.CardMhz > 1419. && Bare.CardMhz < 1421. && !Bare.Source.IsEmpty() && Bare.Clock == S47RxLook::ClockLine(S.TerminalClock()) && Bare.Term.StartsWith(TEXT("V4.2")) && !Bare.bPointing && !Bare.bRecording && Bare.Ticks.IsEmpty() && Bare.Scope.Strength == 0.,
			TEXT("the receiver's frame reads the job's card, the terminal's clock and name; with no station round it there is no pointing, no source on the scopes and nothing recorded"));
		Check(bSky && FMath::Abs(Morning - Almanac) < 1e-6 && Morning > 21. && Morning < 23., *FString::Printf(TEXT("the station's sidereal time follows the world's clock and date (the first morning: %.2f h)"), Morning));
		// the frame on the glass: with its own row of keys the screen has no second one, and what the frame's rows leave is the
		// body every tab has to fit (the sky chart fills it; TUNE, RECORD and DATA are lower)
		const float Glass = SS47ScreenView::ContentHigh(true), Body = SkyT::RxBodyRoom(Glass);
		Check(Glass > SS47ScreenView::ContentHigh(false) + 20.f && Body >= 560.f && Body < Glass - 200.f, *FString::Printf(TEXT("the receiver's frame leaves %.0f of the glass's %.0f units for the page"), Body, Glass));
		// the banner never says more than the receiver's own line: with the job back at the start it asks for the pointing
		Check(Bare.State.Id == S47RxLook::EState::Point || Bare.State.Id == S47RxLook::EState::NotUp || Bare.State.Id == S47RxLook::EState::NoPower, TEXT("a job that is not pointed: the banner asks for the array, or says why not"));
	}
	Before.Stop(); Again.Stop(); S.Stop();
	return bOk;
}
